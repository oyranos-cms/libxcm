/**
 *  @file     color.c
 *
 *  @brief    a compiz colour management plug-in
 *
 *  @author   Tomas Carnecky
 *  @par Copyright:
 *            2008 (C) Tomas Carnecky, 2009 (C) Kai-Uwe Behrmann
 *  @par License:
 *            new BSD <http://www.opensource.org/licenses/bsd-license.php>
 *  @since    2008/06/09
 */


#include <assert.h>

#define GL_GLEXT_PROTOTYPES
#define _BSD_SOURCE

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <X11/extensions/Xfixes.h>
#include <X11/extensions/Xrandr.h>

#include <compiz-core.h>

#include <assert.h>
#include <string.h>

#include <stdarg.h>
#include <lcms.h>

#include <Xcolor.h>


#define OY_COMPIZ_VERSION (COMPIZ_VERSION_MAJOR * 10000 + COMPIZ_VERSION_MINOR * 100 + COMPIZ_VERSION_MICRO)
#if OY_COMPIZ_VERSION < 800
#define oyCompLogMessage(disp_, plug_in_name, debug_level, format_, ... ) \
        compLogMessage( plug_in_name, debug_level, format_, __VA_ARGS__ )
#else
#define oyCompLogMessage(disp_, plug_in_name, debug_level, format_, ... ) \
        compLogMessage( plug_in_name, debug_level, format_, __VA_ARGS__ )
#endif

/* Uncomment the following line if you want to enable debugging output */
#define PLUGIN_DEBUG 1


/**
 * The 3D lookup texture has 64 points in each dimension, using 16 bit integers.
 * That means each active region will use 1.5MiB of texture memory.
 */
#define GRIDPOINTS 64
static GLushort clut[GRIDPOINTS][GRIDPOINTS][GRIDPOINTS][3];


typedef CompBool (*dispatchObjectProc) (CompPlugin *plugin, CompObject *object, void *privateData);


/**
 * When a profile is uploaded into the root window, the plugin fetches the property
 * and creates a lcms profile object. Each profile has a reference count to allow
 * clients to share profiles. When the ref-count drops to zero, the profile is released.
 */
typedef struct {
	uint8_t md5[16];
	cmsHPROFILE lcmsProfile;

	unsigned long refCount;
} PrivColorProfile;

/**
 * The XserverRegion is dereferenced only when the client sends a _NET_COLOR_MANAGEMENT
 * ClientMessage to its window. This allows clients to change the region as the window
 * is resized and later send _N_C_M to tell the plugin to re-fetch the region from the
 * server.
 * The profile is resolved as soon as the client uploads the regions into the window.
 * That means clients need to upload profiles first and then the regions. Otherwise
 * the plugin won't be able to find the profile and no color transformation will
 * be done.
 */
typedef struct {
	/* The server-side region. Only after the client sends a ClientMessage to the window
	 * it is fetched to compiz. */
	XserverRegion region;

	/* This is merely a reference to the lcms profile inside the profile registry. */
	cmsHPROFILE lcmsProfile;

	/* These members are only valid when this region is part of the
	 * active stack range. */
	Region xRegion;
	GLuint glTexture;
	GLfloat scale, offset;
} PrivColorRegion;

/**
 * Output profiles are currently only fetched using XRandR. For backwards compatibility
 * the code should fall back to root window properties (_ICC_PROFILE).
 */
typedef struct {
	char name[32];
	cmsHPROFILE lcmsProfile;
} PrivColorOutput;


static CompMetadata pluginMetadata;

static int corePrivateIndex;

typedef struct {
	int childPrivateIndex;

	ObjectAddProc objectAdd;
} PrivCore;

typedef struct {
	int childPrivateIndex;

	HandleEventProc handleEvent;

	/* ClientMessage sent by the application */
	Atom netColorManagement;

	/* Window properties */
	Atom netColorProfiles;
	Atom netColorRegions;
	Atom netColorTarget;
} PrivDisplay;

typedef struct {
	int childPrivateIndex;

	/* hooked functions */
	DrawWindowProc drawWindow;
	DrawWindowTextureProc drawWindowTexture;

	/* profiles attached to the screen */
	unsigned long nProfiles;
	PrivColorProfile *profile;

	/* compiz fragement function */
	int function, param, unit;

	/* XRandR outputs and the associated profiles */
	unsigned long nOutputs;
	PrivColorOutput *output;
} PrivScreen;

typedef struct {
	/* regions attached to the window */
	unsigned long nRegions;
	PrivColorRegion *region;

	/* active stack range */
	unsigned long active[2];

	/* active XRandR output */
	char *output;
} PrivWindow;


/**
 *    Private Data Allocation
 *
 * These are helper functions that really should be part of compiz. The private
 * data setup and handling currently requires macros and duplicates code all over
 * the place. These functions, along with the object setup code (at the very bottom
 * of this source file) make it much simpler.
 */

static void *compObjectGetPrivate(CompObject *o)
{
	if (o == NULL)
		return &corePrivateIndex;

	int *privateIndex = compObjectGetPrivate(o->parent);
	if (privateIndex == NULL)
		return NULL;

	return o->privates[*privateIndex].ptr;
}

static void *compObjectAllocPrivate(CompObject *parent, CompObject *object, int size)
{
	int *privateIndex = compObjectGetPrivate(parent);
	if (privateIndex == NULL)
		return NULL;

	int *privateData = malloc(size);
	if (privateData == NULL)
		return NULL;

	/* allocate an index for child objects */
	if (object->type < 3) {
		*privateData = compObjectAllocatePrivateIndex(object, object->type + 1);
		if (*privateData == -1) {
			free(privateData);
			return NULL;
		}
	}

	object->privates[*privateIndex].ptr = privateData;

	return privateData;
}

static void compObjectFreePrivate(CompObject *parent, CompObject *object)
{
	int *privateIndex = compObjectGetPrivate(parent);
	if (privateIndex == NULL)
		return;

	int *privateData = object->privates[*privateIndex].ptr;
	if (privateData == NULL)
		return;

	/* free index of child objects */
	if (object->type < 3)
		compObjectFreePrivateIndex(object, object->type + 1, *privateData);

	object->privates[*privateIndex].ptr = NULL;

	free(privateData);
}

/**
 * Xcolor helper functions. I didn't really want to put them into the Xcolor library.
 * Other window managers are free to copy those when needed.
 */

static inline XcolorProfile *XcolorProfileNext(XcolorProfile *profile)
{
	unsigned char *ptr = (unsigned char *) profile;
	return (XcolorProfile *) (ptr + sizeof(XcolorProfile) + ntohl(profile->length));
}

static inline unsigned long XcolorProfileCount(void *data, unsigned long nBytes)
{
	unsigned long count = 0;

	for (XcolorProfile *ptr = data; (intptr_t) ptr < (intptr_t)data + nBytes; ptr = XcolorProfileNext(ptr))
		++count;

	return count;
}

static inline XcolorRegion *XcolorRegionNext(XcolorRegion *region)
{
	unsigned char *ptr = (unsigned char *) region;
	return (XcolorRegion *) (ptr + sizeof(XcolorRegion));
}

static inline unsigned long XcolorRegionCount(void *data, unsigned long nBytes)
{
	return nBytes / sizeof(XcolorRegion);
}


/**
 * Helper function to convert a MD5 into a readable string.
 */
static const char *md5string(const uint8_t md5[16])
{
	static char buffer[33];

	for (int i = 0; i < 16; ++i)
		sprintf(buffer + i * 2, "%02x", md5[i]);

	return buffer;
}

/**
 * Here begins the real code
 */

static int getFetchTarget(CompTexture *texture)
{
	if (texture->target == GL_TEXTURE_2D) {
		return COMP_FETCH_TARGET_2D;
	} else {
		return COMP_FETCH_TARGET_RECT;
	}
}

/**
 * The shader is the same for all windows and profiles. It only depends on the
 * 3D texture and two environment variables.
 */
static int getProfileShader(CompScreen *s, CompTexture *texture, int param, int unit)
{
	PrivScreen *ps = compObjectGetPrivate((CompObject *) s);

	if (ps->function && ps->param == param && ps->unit == unit)
		return ps->function;

	if (ps->function)
		destroyFragmentFunction(s, ps->function);

	CompFunctionData *data = createFunctionData();

	addFetchOpToFunctionData(data, "output", NULL, getFetchTarget(texture));

	addDataOpToFunctionData(data, "MAD output, output, program.env[%d], program.env[%d];", param, param + 1);
	addDataOpToFunctionData(data, "TEX output, output, texture[%d], 3D;", unit);
	addColorOpToFunctionData (data, "output", "output");

	ps->function = createFragmentFunction(s, "color", data);
	ps->param = param;
	ps->unit = unit;

#if defined(PLUGIN_DEBUG)
	oyCompLogMessage(s->display, "color", CompLogLevelDebug, "Shader compiled: %d/%d/%d", ps->function, param, unit);
#endif

	return ps->function;
}

/**
 * Converts a server-side region to a client-side region.
 */
static Region convertRegion(Display *dpy, XserverRegion src)
{
	Region ret = XCreateRegion();

	int nRects = 0;
	XRectangle *rect = XFixesFetchRegion(dpy, src, &nRects);

	for (int i = 0; i < nRects; ++i) {
		XUnionRectWithRegion(&rect[i], ret, ret);
	}

	XFree(rect);

	return ret;
}

/**
 * Generic function to fetch a window property.
 */
static void *fetchProperty(Display *dpy, Window w, Atom prop, Atom type, unsigned long *n, Bool delete)
{
	Atom actual;
	int format;
	unsigned long left;
	unsigned char *data;

	int result = XGetWindowProperty(dpy, w, prop, 0, ~0, delete, type, &actual, &format, n, &left, &data);
#if defined(PLUGIN_DEBUG)
	printf( "%s:%d %s delete: %d %s %lu\n", __FILE__,__LINE__,
                XGetAtomName( dpy, prop ), delete,
                (result == Success) ? "fine" : "err", *n );
#endif
	if (result == Success)
		return (void *) data;

	return NULL;
}

static unsigned long screenProfileCount(PrivScreen *ps)
{
	unsigned long ret = 0;

	for (unsigned long i = 0; i < ps->nProfiles; ++i) {
		if (ps->profile[i].refCount > 0)
			++ret;
	}

	return ret;
}

/**
 * Search the profile database to find the profile with the given UUID.
 * Returns the array index at which the profile is, if not found then returns
 * PrivScreen::nProfiles
 */
static unsigned long findProfileIndex(CompScreen *s, const uint8_t md5[16])
{
	PrivScreen *ps = compObjectGetPrivate((CompObject *) s);

	for (unsigned long i = 0; i < ps->nProfiles; ++i) {
		if (ps->profile[i].refCount > 0 && memcmp(md5, ps->profile[i].md5, 16) == 0)
			return i;
	}

#if defined(PLUGIN_DEBUG)
	oyCompLogMessage(s->display, "color", CompLogLevelDebug, "Could not find profile with MD5 '%s'", md5string(md5));
#endif

	return ps->nProfiles;
}

/**
 * Called when new profiles have been attached to the root window. Fetches
 * these and saves them in a local database.
 */ 
static void updateScreenProfiles(CompScreen *s)
{
	PrivScreen *ps = compObjectGetPrivate((CompObject *) s);

	CompDisplay *d = s->display;
	PrivDisplay *pd = compObjectGetPrivate((CompObject *) d);

	/* Fetch the profiles */
	unsigned long nBytes;
        int screen = DefaultScreen( s->display->display );
	void *data = fetchProperty(d->display,
                                   XRootWindow( s->display->display, screen ),
                                   pd->netColorProfiles,
                                   XA_CARDINAL, &nBytes, True);
	if (data == NULL)
		return;

	/* Grow or shring the array as needed. */
	unsigned long count = XcolorProfileCount(data, nBytes);
	unsigned long usedSlots = screenProfileCount(ps);

	if (usedSlots + count > ps->nProfiles) {
 		PrivColorProfile *ptr = realloc(ps->profile, (usedSlots + count) * sizeof(PrivColorProfile));
		if (ptr == NULL)
			goto out;

		memset(ptr + ps->nProfiles, 0, (usedSlots + count - ps->nProfiles) * sizeof(PrivColorProfile));

		ps->nProfiles = usedSlots + count;
		ps->profile = ptr;
	} else if (usedSlots + count < ps->nProfiles / 2) {
		unsigned long index = 0;
		for (unsigned long i = 0; i < ps->nProfiles; ++i) {
			if (ps->profile[i].refCount > 0)
				memmove(ps->profile + index++, ps->profile + i, sizeof(PrivColorProfile));
		}

		assert(index == usedSlots);

 		PrivColorProfile *ptr = realloc(ps->profile, (ps->nProfiles / 2) * sizeof(PrivColorProfile));
		if (ptr == NULL)
			goto out;

		ps->nProfiles = ps->nProfiles / 2;
		ps->profile = ptr;

		memset(ptr + usedSlots, 0, (ps->nProfiles - usedSlots) * sizeof(PrivColorProfile));
	}

	/* Copy the profiles into the array, and create the lcms handles. */
	XcolorProfile *profile = data;
	for (unsigned long i = 0; i < count; ++i) {
		unsigned long index = findProfileIndex(s, profile->md5);

		/* XcolorProfile::length == 0 means the clients wants to delete the profile. */
		if (ntohl(profile->length) == 0) {
			/* Profile not found. Probably a ref-count issue inside clients. Should I throw a warning? */
			if (index == ps->nProfiles)
				continue;

			--ps->profile[index].refCount;

			/* If refcount drops to zero, destroy the lcms object. */
			if (ps->profile[index].refCount == 0)
				cmsCloseProfile(ps->profile[index].lcmsProfile);
		} else {
			if (index == ps->nProfiles) {
				/* Profile doesn't exist in our array, find a new free slot. */
				for (unsigned long i = 0; i < ps->nProfiles; ++i) {
					if (ps->profile[i].refCount > 0)
						continue;

					ps->profile[i].lcmsProfile = cmsOpenProfileFromMem(profile + 1, htonl(profile->length));

					/* If creating the lcms profile fails, don't try to parse any further profiles and just quit. */
					if (ps->profile[i].lcmsProfile == NULL) {
						oyCompLogMessage(d, "color", CompLogLevelWarn, "Couldn't create lcms profile%s", "");
						goto out;
					}

					memcpy(ps->profile[i].md5, profile->md5, 16);
					ps->profile[i].refCount = 1;

					break;
				}
			} else {
				/* Profile alreade exists in the array, just increase the ref count. */
				++ps->profile[index].refCount;
			}
		}

		profile = XcolorProfileNext(profile);
	}

#if defined(PLUGIN_DEBUG)
	oyCompLogMessage(d, "color", CompLogLevelDebug, "Updated screen profiles, %d existing plus %d updates leading to %d profiles in %d slots",
		       usedSlots, count, screenProfileCount(ps), ps->nProfiles);
#endif

out:
	XFree(data);
}

/**
 * Called when new regions have been attached to a window. Fetches these and
 * saves them in the local list.
 */
static void updateWindowRegions(CompWindow *w)
{
	PrivWindow *pw = compObjectGetPrivate((CompObject *) w);
	PrivScreen *ps = compObjectGetPrivate((CompObject *) w->screen);

	CompDisplay *d = w->screen->display;
	PrivDisplay *pd = compObjectGetPrivate((CompObject *) d);

	/* free existing data structures */
	for (unsigned long i = 0; i < pw->nRegions; ++i) {
		if (pw->region[i].glTexture != 0) {
			XDestroyRegion(pw->region[i].xRegion);
			glDeleteTextures(1, &pw->region[i].glTexture);
		}
	}

	if (pw->nRegions)
		free(pw->region);
   
	pw->nRegions = 0;

	/* fetch the regions */
	unsigned long nBytes;
	void *data = fetchProperty(d->display, w->id, pd->netColorRegions, XA_CARDINAL, &nBytes, False);
	if (data == NULL)
		return;     

	/* allocate the list */
	unsigned long count = XcolorRegionCount(data, nBytes);
	pw->region = malloc(count * sizeof(PrivColorRegion));
	if (pw->region == NULL)
		goto out;

	memset(pw->region, 0, count * sizeof(PrivColorRegion));

	/* fill in the pointers */
	XcolorRegion *region = data;
	for (unsigned long i = 0; i < count; ++i) {
		pw->region[i].region = region->region;

		/* Locate the lcms profile. If not availabe, simply set PrivColorRegion::lcmsRegion
		 * to NULL so that it will be ignored during the rendering. */
		unsigned long index = findProfileIndex(w->screen, region->md5);
		pw->region[i].lcmsProfile = index == ps->nProfiles ? NULL : ps->profile[index].lcmsProfile;

		region = XcolorRegionNext(region);
	}

	pw->nRegions = count;

#if defined(PLUGIN_DEBUG)
	oyCompLogMessage(d, "color", CompLogLevelDebug, "Updated window regions, %d total now", count);
#endif

out:
	XFree(data);
}

static void updateWindowStack(CompWindow *w, void *closure);

/**
 * Called when the window target (_NET_COLOR_TARGET) has been changed.
 */
static void updateWindowOutput(CompWindow *w)
{
	PrivWindow *pw = compObjectGetPrivate((CompObject *) w);

	CompDisplay *d = w->screen->display;
	PrivDisplay *pd = compObjectGetPrivate((CompObject *) d);

	if (pw->output)
		XFree(pw->output);

	unsigned long nBytes;
	pw->output = fetchProperty(d->display, w->id, pd->netColorTarget, XA_STRING, &nBytes, False);

#if defined(_NET_COLOR_DEBUG)
	oyCompLogMessage(d, "color", CompLogLevelDebug, "Updated window output, target is %s", pw->output);
#endif

	updateWindowStack(w, NULL);
}

/**
 * Search the output list and return the associated profile.
 */
static void *findOutputProfile(CompScreen *s, const char *name)
{
	PrivScreen *ps = compObjectGetPrivate((CompObject *) s);

	for (unsigned long i = 0; i < ps->nOutputs; ++i) {
		if (strcmp(name, ps->output[i].name) == 0) {
			return ps->output[i].lcmsProfile;
		}
	}

#if defined(PLUGIN_DEBUG)
	oyCompLogMessage(s->display, "color", CompLogLevelDebug, "Could not find profile for output '%s'", name);
#endif

	return NULL;
}

/**
 * Called when a ClientMessage is sent to the window. Update local copies
 * and profile links. This is where the regions are prepared and the 3D texture
 * generated.
 */
static void updateWindowStack(CompWindow *w, void *closure)
{
	PrivWindow *pw = compObjectGetPrivate((CompObject *) w);

	/* free existing data structures */
	for (unsigned long i = 0; i < pw->nRegions; ++i) {
		if (pw->region[pw->active[0] + i].glTexture != 0) {
			XDestroyRegion(pw->region[pw->active[0] + i].xRegion);
			glDeleteTextures(1, &pw->region[pw->active[0] + i].glTexture);

			pw->region[pw->active[0] + i].glTexture = 0;
		}
	}

	/* If the range extends beyond the stack size, disable any color transformations on that window. */
	if (pw->active[0] + pw->active[1] > pw->nRegions)
		pw->active[0] = pw->active[1] = 0;

	/* regenerate local region variables */
	for (unsigned long i = 0; i < pw->active[1]; ++i) {
		/* First try to create the transformation matrix. If that fails, simply skip to the next region.
		 * Because of PrivColorRegion::glTexture == 0 the region will later be skippend when rendering the window. */

		cmsHPROFILE srcProfile = pw->region[pw->active[0] + i].lcmsProfile;

		/* support device links from client to trasnport complex things */
		icProfileClassSignature pclass_sig = cmsGetDeviceClass( srcProfile );

		if (srcProfile == NULL)
			continue;
		int device_link = (pclass_sig == icSigLinkClass);

		cmsHPROFILE dstProfile = findOutputProfile(w->screen, pw->output);
		if (!device_link && dstProfile == NULL)
			continue;

		cmsHTRANSFORM xform = cmsCreateTransform(srcProfile, TYPE_RGB_16,
			device_link ? 0 : dstProfile, TYPE_RGB_16,
			INTENT_PERCEPTUAL, cmsFLAGS_NOTPRECALC);
		if (xform == NULL)
			continue;

		pw->region[pw->active[0] + i].scale = (GLfloat) (GRIDPOINTS - 1) / GRIDPOINTS;
		pw->region[pw->active[0] + i].offset = (GLfloat) 1.0 / (2 * GRIDPOINTS);

		unsigned short in[3];
		for (int r = 0; r < GRIDPOINTS; ++r) {
			in[0] = floor((double) r / (GRIDPOINTS - 1) * 65535.0 + 0.5);
			for (int g = 0; g < GRIDPOINTS; ++g) {
				in[1] = floor((double) g / (GRIDPOINTS - 1) * 65535.0 + 0.5);
				for (int b = 0; b < GRIDPOINTS; ++b) {
					in[2] = floor((double) b / (GRIDPOINTS - 1) * 65535.0 + 0.5);
					cmsDoTransform(xform, in, clut[b][g][r], 1);
				}
			}
		}

		cmsDeleteTransform(xform);

		/* Alright, everything succeeded, fetch the region and create the OpenGL texture. */
		pw->region[pw->active[0] + i].xRegion = convertRegion(w->screen->display->display, pw->region[pw->active[0] + i].region);
		
		glGenTextures(1, &pw->region[pw->active[0] + i].glTexture);
		glBindTexture(GL_TEXTURE_3D, pw->region[pw->active[0] + i].glTexture);

		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB16, GRIDPOINTS, GRIDPOINTS, GRIDPOINTS, 0, GL_RGB, GL_UNSIGNED_SHORT, clut);
	}

#if defined(PLUGIN_DEBUG)
	oyCompLogMessage(w->screen->display, "color", CompLogLevelDebug, "Created window transformation textures. Active range is between %d and %d (out of %d regions total)",
		       pw->active[0], pw->active[0] + pw->active[1], pw->nRegions);
#endif

	addWindowDamage(w);
}

/**
 * Called when XRandR output configuration (or properties) change. Fetch
 * output profiles (if available) or fall back to sRGB.
 */
static void updateOutputConfiguration(CompScreen *s, CompBool updateWindows)
{
	PrivScreen *ps = compObjectGetPrivate((CompObject *) s);
        int screen = DefaultScreen( s->display->display );

	if (ps->nOutputs > 0) {
		for (unsigned long i = 0; i < ps->nOutputs; ++i)
			cmsCloseProfile(ps->output[i].lcmsProfile);

		free(ps->output);
	}

	XRRScreenResources *res = XRRGetScreenResources(s->display->display,
                                    XRootWindow( s->display->display, screen ));

	ps->nOutputs = res->noutput;
	ps->output = malloc(ps->nOutputs * sizeof(PrivColorOutput));
	for (unsigned long i = 0; i < ps->nOutputs; ++i) {
		XRROutputInfo *oinfo = XRRGetOutputInfo(s->display->display, res, res->outputs[i]);
		strcpy(ps->output[i].name, oinfo->name);

		Atom actualType, outputProfile = XInternAtom(s->display->display, "_ICC_PROFILE", False);
		int actualFormat, result;
		unsigned long n, left;
		unsigned char *data;

		result = XRRGetOutputProperty(s->display->display, res->outputs[i],
		      outputProfile, 0, ~0, False, False, AnyPropertyType, 
		      &actualType, &actualFormat, &n, &left, &data);

		if (result == Success && n > 0) {
			oyCompLogMessage(s->display, "color", CompLogLevelInfo, "Output %s: extracted profile from RandR", oinfo->name);
			ps->output[i].lcmsProfile = cmsOpenProfileFromMem(data, n);
		} else {
			oyCompLogMessage(s->display, "color", CompLogLevelInfo, "Output %s: assuming sRGB profile", oinfo->name);
			ps->output[i].lcmsProfile = cmsCreate_sRGBProfile();
		}

		XRRFreeOutputInfo(oinfo);
	}

	XRRFreeScreenResources(res);

	if (updateWindows)
		forEachWindowOnScreen(s, updateWindowStack, NULL);

#if defined(PLUGIN_DEBUG)
	oyCompLogMessage(s->display, "color", CompLogLevelDebug, "Updated screen outputs, %d total now", ps->nOutputs);
#endif
}

/**
 * CompDisplay::handleEvent
 */
static void pluginHandleEvent(CompDisplay *d, XEvent *event)
{
	PrivDisplay *pd = compObjectGetPrivate((CompObject *) d);

	UNWRAP(pd, d, handleEvent);
	(*d->handleEvent) (d, event);
	WRAP(pd, d, handleEvent, pluginHandleEvent);

	switch (event->type) {
	case PropertyNotify:
#if defined(PLUGIN_DEBUG)
		if (event->xproperty.atom == pd->netColorProfiles ||
				event->xproperty.atom == pd->netColorRegions ||
				event->xproperty.atom == pd->netColorTarget )
			printf( "%s:%d PropertyNotify: %s\n", __FILE__,__LINE__,
  	         	XGetAtomName( event->xany.display, event->xproperty.atom ) );
#endif
		if (event->xproperty.atom == pd->netColorProfiles) {
			CompScreen *s = findScreenAtDisplay(d, event->xproperty.window);
			updateScreenProfiles(s);
		} else if (event->xproperty.atom == pd->netColorRegions) {
			CompWindow *w = findWindowAtDisplay(d, event->xproperty.window);
			updateWindowRegions(w);
		} else if (event->xproperty.atom == pd->netColorTarget) {
			CompWindow *w = findWindowAtDisplay(d, event->xproperty.window);
			updateWindowOutput(w);
		}
		break;
	case ClientMessage:
		if (event->xclient.message_type == pd->netColorManagement) {
#if defined(PLUGIN_DEBUG)
			printf( "%s:%d ClientMessage: %s\n", __FILE__,__LINE__,
  	         	XGetAtomName( event->xany.display, event->xclient.message_type) );
#endif
			CompWindow *w = findWindowAtDisplay (d, event->xclient.window);
			PrivWindow *pw = compObjectGetPrivate((CompObject *) w);

			unsigned long active[2] = { event->xclient.data.l[0], event->xclient.data.l[1] };
			memcpy(pw->active, active, 2 * sizeof(unsigned long));

			updateWindowStack(w, NULL);
		}
		break;
	default:
		if (event->type == d->randrEvent + RRNotify) {
			XRRNotifyEvent *rrn = (XRRNotifyEvent *) event;
			CompScreen *s = findScreenAtDisplay(d, rrn->window);
#if defined(PLUGIN_DEBUG)
			printf( "%s:%d XRRNotifyEvent\n", __FILE__,__LINE__ );
#endif
			updateOutputConfiguration(s, TRUE);
		}
		break;
	}
}

/**
 * Make region relative to the window. Uses static variables to prevent
 * allocating and freeing the Region in pluginDrawWindow().
 */
static Region absoluteRegion(CompWindow *w, Region region)
{
	static REGION ret;
	static BOX rects[128];

	ret.numRects = region->numRects;
	ret.rects = rects;

	memset(&ret.extents, 0, sizeof(ret.extents));

	for (int i = 0; i < region->numRects; ++i) {
		rects[i].x1 = region->rects[i].x1 + w->attrib.x;
		rects[i].x2 = region->rects[i].x2 + w->attrib.x;
		rects[i].y1 = region->rects[i].y1 + w->attrib.y;
		rects[i].y2 = region->rects[i].y2 + w->attrib.y;

		EXTENTS(&rects[i], &ret);
	}

	return &ret;
}

/**
 * CompScreen::drawWindow
 */
static Bool pluginDrawWindow(CompWindow *w, const CompTransform *transform, const FragmentAttrib *attrib, Region region, unsigned int mask)
{
	CompScreen *s = w->screen;
	PrivScreen *ps = compObjectGetPrivate((CompObject *) s);

	UNWRAP(ps, s, drawWindow);
	Bool status = (*s->drawWindow) (w, transform, attrib, region, mask);
	WRAP(ps, s, drawWindow, pluginDrawWindow);

	/* If no regions have been enabled, just return as we're done */
	PrivWindow *pw = compObjectGetPrivate((CompObject *) w);
	if (pw->active[0] == 0 && pw->active[1] == 0)
		return status;

	/* Clear the stencil buffer with zero */
	glClear(GL_STENCIL_BUFFER_BIT);

	/* Disable color mask as we won't want to draw anything */
	glEnable(GL_STENCIL_TEST);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	/* Replace the stencil value in places where we'd draw something */
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	for (unsigned long i = 0; i < pw->active[1]; ++i) {
		PrivColorRegion *reg = pw->region + pw->active[0] + i;
		if (reg->glTexture == 0)
			continue;

		/* Each region gets its own stencil value */
		glStencilFunc(GL_ALWAYS, i + 1, ~0);

		Region tmp = absoluteRegion(w, reg->xRegion);
		       
		w->vCount = w->indexCount = 0;
		(*w->screen->addWindowGeometry) (w, &w->matrix, 1, tmp, region);

		/* If the geometry is non-empty, draw the window */
		if (w->vCount > 0) {
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			(*w->drawWindowGeometry) (w);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		}
	}

	/* Reset the color mask */
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDisable(GL_STENCIL_TEST);

	return status;
}

/**
 * CompScreen::drawWindowTexture
 */
static void pluginDrawWindowTexture(CompWindow *w, CompTexture *texture, const FragmentAttrib *attrib, unsigned int mask)
{
	CompScreen *s = w->screen;
	PrivScreen *ps = compObjectGetPrivate((CompObject *) s);

	UNWRAP(ps, s, drawWindowTexture);
	(*s->drawWindowTexture) (w, texture, attrib, mask);
	WRAP(ps, s, drawWindowTexture, pluginDrawWindowTexture);

	PrivWindow *pw = compObjectGetPrivate((CompObject *) w);
	if (pw->active[0] == 0 && pw->active[1] == 0)
		return;

	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	/* Set up the shader */
	FragmentAttrib fa = *attrib;

	int param = allocFragmentParameters(&fa, 2);
	int unit = allocFragmentTextureUnits(&fa, 1);

	int function = getProfileShader(s, texture, param, unit);
	if (function)
		addFragmentFunction(&fa, function);

	for (unsigned long i = 0; i < pw->active[1]; ++i) {
		PrivColorRegion *reg = pw->region + pw->active[0] + i;
		if (reg->glTexture == 0)
			continue;

		/* Set the environment variables */
		glProgramEnvParameter4dARB(GL_FRAGMENT_PROGRAM_ARB, param + 0, reg->scale, reg->scale, reg->scale, 1.0);
		glProgramEnvParameter4dARB(GL_FRAGMENT_PROGRAM_ARB, param + 1, reg->offset, reg->offset, reg->offset, 0.0);

		/* Activate the 3D texture */
		(*s->activeTexture) (GL_TEXTURE0_ARB + unit);
		glEnable(GL_TEXTURE_3D);
		glBindTexture(GL_TEXTURE_3D, reg->glTexture);
		(*s->activeTexture) (GL_TEXTURE0_ARB);

		/* Only draw where the stencil value matches 'i + 1' */
		glStencilFunc(GL_EQUAL, i + 1, ~0);

		/* Now draw the window texture */
		UNWRAP(ps, s, drawWindowTexture);
		(*s->drawWindowTexture) (w, texture, &fa, mask);
		WRAP(ps, s, drawWindowTexture, pluginDrawWindowTexture);

		/* Deactivate the 3D texture */
		(*s->activeTexture) (GL_TEXTURE0_ARB + unit);
		glBindTexture(GL_TEXTURE_3D, 0);
		glDisable(GL_TEXTURE_3D);
		(*s->activeTexture) (GL_TEXTURE0_ARB);
	}

	glDisable(GL_STENCIL_TEST);

	/* Not sure if really necessary */
	addWindowDamage(w);
}


/**
 * This is really stupid, object->parent isn't inisialized when pluginInitObject()
 * is called. So this is a wrapper to get the parent because compObjectAllocPrivate()
 * needs it.
 */
static CompObject *getParent(CompObject *object)
{
	switch (object->type) {
	case 0:
		return NULL;
	case 1:
		return (CompObject *) &core;
	case 2:
		return (CompObject *) ((CompScreen *) object)->display;
	case 3:
		return (CompObject *) ((CompWindow *) object)->screen;
	default:
		return NULL;
	}
}

/**
 *    Object Init Functions
 */

static CompBool pluginInitCore(CompPlugin *plugin, CompObject *object, void *privateData)
{
	return TRUE;
}

static CompBool pluginInitDisplay(CompPlugin *plugin, CompObject *object, void *privateData)
{
	CompDisplay *d = (CompDisplay *) object;
	PrivDisplay *pd = privateData;

	if (d->randrExtension == False)
		return FALSE;

	WRAP(pd, d, handleEvent, pluginHandleEvent);

	pd->netColorManagement = XInternAtom(d->display, "_NET_COLOR_MANAGEMENT", False);

	pd->netColorProfiles = XInternAtom(d->display, "_NET_COLOR_PROFILES", False);
	pd->netColorRegions = XInternAtom(d->display, "_NET_COLOR_REGIONS", False);
	pd->netColorTarget = XInternAtom(d->display, "_NET_COLOR_TARGET", False);

	return TRUE;
}

static CompBool pluginInitScreen(CompPlugin *plugin, CompObject *object, void *privateData)
{
	CompScreen *s = (CompScreen *) object;
	PrivScreen *ps = privateData;
        int screen = DefaultScreen( s->display->display );

	GLint stencilBits = 0;
	glGetIntegerv(GL_STENCIL_BITS, &stencilBits);
	if (stencilBits == 0)
		return FALSE;

	WRAP(ps, s, drawWindow, pluginDrawWindow);
	WRAP(ps, s, drawWindowTexture, pluginDrawWindowTexture);

	ps->nProfiles = 0;
	ps->profile = NULL;

	ps->function = 0;

	/* XRandR setup code */

	XRRSelectInput(s->display->display,
                       XRootWindow( s->display->display, screen ),
                       RROutputPropertyNotifyMask);

	ps->nOutputs = 0;
	updateOutputConfiguration(s, FALSE);

	return TRUE;
}

static CompBool pluginInitWindow(CompPlugin *plugin, CompObject *object, void *privateData)
{
	/* CompWindow *w = (CompWindow *) object; */
	PrivWindow *pw = privateData;

	pw->nRegions = 0;
	pw->active[0] = pw->active[1] = 0;

	pw->output = NULL;

	return TRUE;
}

static dispatchObjectProc dispatchInitObject[] = {
	pluginInitCore, pluginInitDisplay, pluginInitScreen, pluginInitWindow
};

/**
 *    Object Fini Functions
 */


static CompBool pluginFiniCore(CompPlugin *plugin, CompObject *object, void *privateData)
{
	/* Don't crash if something goes wrong inside lcms */
	cmsErrorAction(LCMS_ERRC_WARNING);

	return TRUE;
}

static CompBool pluginFiniDisplay(CompPlugin *plugin, CompObject *object, void *privateData)
{
	CompDisplay *d = (CompDisplay *) object;
	PrivDisplay *pd = privateData;

	UNWRAP(pd, d, handleEvent);

	return TRUE;
}

static CompBool pluginFiniScreen(CompPlugin *plugin, CompObject *object, void *privateData)
{
	CompScreen *s = (CompScreen *) object;
	PrivScreen *ps = privateData;

	UNWRAP(ps, s, drawWindow);
	UNWRAP(ps, s, drawWindowTexture);

	return TRUE;
}

static CompBool pluginFiniWindow(CompPlugin *plugin, CompObject *object, void *privateData)
{
	return TRUE;
}

static dispatchObjectProc dispatchFiniObject[] = {
	pluginFiniCore, pluginFiniDisplay, pluginFiniScreen, pluginFiniWindow
};


/**
 *    Plugin Interface
 */
static CompBool pluginInit(CompPlugin *p)
{
	corePrivateIndex = allocateCorePrivateIndex();
	if (corePrivateIndex < 0)
		return FALSE;

	return TRUE;
}

static CompBool pluginInitObject(CompPlugin *p, CompObject *o)
{
	static const int privateSizes[] = {
		sizeof(PrivCore), sizeof(PrivDisplay), sizeof(PrivScreen), sizeof(PrivWindow)
	};

	void *privateData = compObjectAllocPrivate(getParent(o), o, privateSizes[o->type]);
	if (privateData == NULL)
		return TRUE;

	if (dispatchInitObject[o->type](p, o, privateData) == FALSE)
		compObjectFreePrivate(getParent(o), o);

	return TRUE;
}

static void pluginFiniObject(CompPlugin *p, CompObject *o)
{
	void *privateData = compObjectGetPrivate(o);
	if (privateData == NULL)
		return;

	dispatchFiniObject[o->type](p, o, privateData);
	compObjectFreePrivate(getParent(o), o);
}

static void pluginFini(CompPlugin *p)
{
	freeCorePrivateIndex(corePrivateIndex);
}

static CompMetadata *pluginGetMetadata(CompPlugin *p)
{
	return &pluginMetadata;
}

CompPluginVTable pluginVTable = {
	"color",
	pluginGetMetadata,
	pluginInit,
	pluginFini,
	pluginInitObject,
	pluginFiniObject,
	0,
	0
};

CompPluginVTable *getCompPluginInfo20070830(void)
{
	return &pluginVTable;
}

