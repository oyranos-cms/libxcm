
#define GL_GLEXT_PROTOTYPES

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <X11/extensions/Xfixes.h>
#include <X11/extensions/Xrandr.h>

#include <compiz-core.h>

#include <assert.h>
#include <string.h>

#include <stdarg.h>
#include <lcms.h>

#include "xcolor.h"

#define GRIDPOINTS 64
static GLushort clut[GRIDPOINTS][GRIDPOINTS][GRIDPOINTS][3];


typedef CompBool (*dispatchObjectProc) (CompPlugin *plugin, CompObject *object, void *privateData);


typedef struct  {
	Region region;

	GLuint clutTexture;
	GLfloat scale, offset;
} ColorRegion;


typedef struct {
	long x, y;
	unsigned long width, height;
	cmsHPROFILE profile;
} ColorOutput;


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
	Atom netColorType;
} PrivDisplay;

typedef struct {
	int childPrivateIndex;

	/* hooked functions */
	DrawWindowProc drawWindow;
	DrawWindowTextureProc drawWindowTexture;

	/* profiles attached to the screen */
	unsigned long nProfiles;
	XColorProfile **profile;

	/* compiz fragement function */
	int function, param, unit;

	/* XRandR */
	unsigned long nOutputs;
	ColorOutput *output;
} PrivScreen;

typedef struct {
	/* regions attached to the window */
	unsigned long nRegions;
	XColorRegion **region;

	/* active stack range */
	unsigned long active[2];

	/* local copies of the active regions */
	ColorRegion local[16];
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

	return ps->function;
}

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

static void *fetchProperty(Display *dpy, Window w, Atom prop, Atom type, unsigned long *n)
{
	Atom actual;
	int format;
	unsigned long left;
	unsigned char *data;

	int result = XGetWindowProperty(dpy, w, prop, 0, ~0, False, type, &actual, &format, n, &left, &data);
	if (result == Success)
		return (void *) data;

	return NULL;
}

static void updateScreenProfiles(CompScreen *s)
{
	PrivScreen *ps = compObjectGetPrivate((CompObject *) s);

	CompDisplay *d = s->display;
	PrivDisplay *pd = compObjectGetPrivate((CompObject *) d);

	/* free existing data structures */
	if (ps->nProfiles) {
		XFree(ps->profile[0]);
		free(ps->profile);
	}

	ps->nProfiles = 0;

	/* fetch the profiles */
	unsigned long nBytes;
	void *data = fetchProperty(d->display, s->root, pd->netColorProfiles, pd->netColorType, &nBytes);
	if (data == NULL)
		return;

	/* allocate list */
	unsigned long count = XColorProfileCount(data, nBytes);
	ps->profile = malloc(count * sizeof(XColorProfile *));
	if (ps->profile == NULL)
		goto out;

	/* fill in the pointers */
	XColorProfile *ptr = data;
	for (unsigned long i = 0; i < count; ++i) {
		ps->profile[i] = ptr;
		ptr = XColorProfileNext(ptr);
	}

	ps->nProfiles = count;

	return;

out:
	XFree(data);
}

static void updateWindowRegions(CompWindow *w)
{
	PrivWindow *pw = compObjectGetPrivate((CompObject *) w);

	CompDisplay *d = w->screen->display;
	PrivDisplay *pd = compObjectGetPrivate((CompObject *) d);

	/* free existing data structures */
	if (pw->nRegions) {
		XFree(pw->region[0]);
		free(pw->region);
	}

	pw->nRegions = 0;

	/* fetch the regions */
	unsigned long nBytes;
	void *data = fetchProperty(d->display, w->id, pd->netColorRegions, pd->netColorType, &nBytes);
	if (data == NULL)
		return;     

	/* allocate the list */
	unsigned long count = XColorRegionCount(data, nBytes);
	pw->region = malloc(count * sizeof(XColorRegion *));
	if (pw->region == NULL)
		goto out;

	/* fill in the pointers */
	XColorRegion *ptr = data;
	for (unsigned long i = 0; i < count; ++i) {
		pw->region[i] = ptr;
		ptr = XColorRegionNext(ptr);
	}

	pw->nRegions = count;

	return;

out:
	XFree(data);
}

static void *findProfileBlob(CompScreen *s, uuid_t uuid, unsigned long *nBytes)
{
	PrivScreen *ps = compObjectGetPrivate((CompObject *) s);

	for (unsigned long i = 0; i < ps->nProfiles; ++i) {
		if (uuid_compare(uuid, ps->profile[i]->uuid) == 0) {
			*nBytes = ntohl(ps->profile[i]->size);
			return ps->profile[i] + 1;
		}
	}

	return NULL;
}

static void activateRegions(CompWindow *w, unsigned long active[2])
{
	PrivWindow *pw = compObjectGetPrivate((CompObject *) w);
	PrivScreen *ps = compObjectGetPrivate((CompObject *) w->screen);

	/* free existing data structures */
	for (unsigned long i = 0; i < pw->active[1] - pw->active[0]; ++i) {
		XDestroyRegion(pw->local[i].region);
		pw->local[i].region = NULL;

		glDeleteTextures(1, &pw->local[i].clutTexture);
		pw->local[i].clutTexture = 0;
	}

	if (active[1] > pw->nRegions)
		return;

	memcpy(pw->active, active, 2 * sizeof(unsigned long));

	/* regenerate local region variables */
	for (unsigned long i = 0; i < active[1] - active[0]; ++i) {
		int r, g, b, n = GRIDPOINTS;

		unsigned long nBytes;
		void *data = findProfileBlob(w->screen, pw->region[i + pw->active[0]]->uuid, &nBytes);
		if (data == NULL){
			fprintf(stderr, "Profile not found!\n");
			continue;
		}

		cmsHPROFILE window = cmsOpenProfileFromMem(data, nBytes);
		cmsHTRANSFORM xform = cmsCreateTransform(window, TYPE_RGB_16, ps->output[0].profile, TYPE_RGB_16, INTENT_PERCEPTUAL, cmsFLAGS_NOTPRECALC);

		if (xform == NULL) {
			fprintf(stderr, "Failed to create transformation\n");
			continue;
		}

		pw->local[i].region = convertRegion(w->screen->display->display, ntohl(pw->region[i + pw->active[0]]->region));

		pw->local[i].scale = (double) (n - 1) / n;
		pw->local[i].offset = 1.0 / (2 * n);

		unsigned short in[3];
		for (r = 0; r < n; r++) {
			in[0] = floor ((double) r / (n - 1) * 65535.0 + 0.5);
			for (g = 0; g < n; g++) {
				in[1] = floor ((double) g / (n - 1) * 65535.0 + 0.5);
				for (b = 0; b < n; b++) {
					in[2] = floor ((double) b / (n - 1) * 65535.0 + 0.5);
					cmsDoTransform(xform, in, clut[b][g][r], 1);
				}
			}
		}

		cmsDeleteTransform(xform);
		cmsCloseProfile(window);

		glGenTextures(1, &pw->local[i].clutTexture);
		glBindTexture(GL_TEXTURE_3D, pw->local[i].clutTexture);

		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB16, n, n, n, 0, GL_RGB, GL_UNSIGNED_SHORT, clut);
	}
}

static void updateOutputConfiguration(CompScreen *s)
{
	PrivScreen *ps = compObjectGetPrivate((CompObject *) s);

	if (ps->nOutputs > 0) {
		for (unsigned long i = 0; i < ps->nOutputs; ++i)
			cmsCloseProfile(ps->output[i].profile);

		free(ps->output);
	}

	XRRScreenResources *res = XRRGetScreenResources(s->display->display, s->root);

	ps->nOutputs = res->noutput;
	ps->output = malloc(ps->nOutputs * sizeof(ColorOutput));
	for (unsigned long i = 0; i < ps->nOutputs; ++i) {
		XRROutputInfo *oinfo = XRRGetOutputInfo(s->display->display, res, res->outputs[i]);

		Atom actualType, outputProfile = XInternAtom(s->display->display, "PROFILE", False);
		int actualFormat, result;
		unsigned long n, left;
		unsigned char *data;

		result = XRRGetOutputProperty(s->display->display, res->outputs[i],
		      outputProfile, 0, ~0, False, False, AnyPropertyType, 
		      &actualType, &actualFormat, &n, &left, &data);

		if (result == Success && n > 0) {
			printf("Output '%s' has attached profile.\n", oinfo->name);
			ps->output[i].profile = cmsOpenProfileFromMem(data, n);
		} else {
			printf("Assuming sRGB profile for output '%s'\n", oinfo->name);
			ps->output[i].profile = cmsCreate_sRGBProfile();
		}

		if (oinfo->crtc != None) {
			XRRCrtcInfo *cinfo = XRRGetCrtcInfo(s->display->display, res, oinfo->crtc);

			ps->output[i].x = cinfo->x;
			ps->output[i].y = cinfo->x;
			ps->output[i].width = cinfo->width;
			ps->output[i].height = cinfo->height;

			XRRFreeCrtcInfo(cinfo);
		}

		XRRFreeOutputInfo(oinfo);
	}

	XRRFreeScreenResources(res);
}


static void pluginHandleEvent(CompDisplay *d, XEvent *event)
{
	PrivDisplay *pd = compObjectGetPrivate((CompObject *) d);

	UNWRAP(pd, d, handleEvent);
	(*d->handleEvent) (d, event);
	WRAP(pd, d, handleEvent, pluginHandleEvent);

	switch (event->type) {
	case PropertyNotify:
		if (event->xproperty.atom == pd->netColorProfiles) {
			CompScreen *s = findScreenAtDisplay(d, event->xproperty.window);
			updateScreenProfiles(s);
		} else if (event->xproperty.atom == pd->netColorRegions) {
			CompWindow *w = findWindowAtDisplay(d, event->xproperty.window);
			updateWindowRegions(w);
		}
		break;
	case ClientMessage:
		if (event->xclient.message_type == pd->netColorManagement) {
			CompWindow *w = findWindowAtDisplay (d, event->xclient.window);

			unsigned long active[2] = { event->xclient.data.l[0], event->xclient.data.l[1] };
			activateRegions(w, active);

			addWindowDamage(w);
		}
		break;
	default:
		if (event->type == d->randrEvent + RRScreenChangeNotify) {
			XRRScreenChangeNotifyEvent *rre = (XRRScreenChangeNotifyEvent *) event;
			CompScreen *s = findScreenAtDisplay(d, rre->root);
			updateOutputConfiguration(s);
		}
		break;
	}
}

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

	//compLogMessage(w->screen->display, "color", CompLogLevelWarn, "region %d %d %d %d", ret.extents.x1, ret.extents.x2, ret.extents.y1, ret.extents.y2);

	return &ret;
}

static Bool pluginDrawWindow(CompWindow *w, const CompTransform *transform, const FragmentAttrib *attrib, Region region, unsigned int mask)
{
	CompScreen *s = w->screen;
	PrivScreen *ps = compObjectGetPrivate((CompObject *) s);

	UNWRAP(ps, s, drawWindow);
	Bool status = (*s->drawWindow) (w, transform, attrib, region, mask);
	WRAP(ps, s, drawWindow, pluginDrawWindow);

	PrivWindow *pw = compObjectGetPrivate((CompObject *) w);
	if (pw->active[0] == pw->active[1])
		return status;

	glClear(GL_STENCIL_BUFFER_BIT);

	glEnable(GL_STENCIL_TEST);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	for (unsigned long i = 0; i < pw->active[1] - pw->active[0]; ++i) {
		if (pw->local[i].region == NULL)
			continue;

		glStencilFunc(GL_ALWAYS, i + 1, ~0);

		Region tmp = absoluteRegion(w, pw->local[i].region);
		       
		w->vCount = w->indexCount = 0;
		(*w->screen->addWindowGeometry) (w, &w->matrix, 1, tmp, region);

		if (w->vCount > 0) {
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			(*w->drawWindowGeometry) (w);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		}
	}

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDisable(GL_STENCIL_TEST);

	return status;
}

static void pluginDrawWindowTexture(CompWindow *w, CompTexture *texture, const FragmentAttrib *attrib, unsigned int mask)
{
	CompScreen *s = w->screen;
	PrivScreen *ps = compObjectGetPrivate((CompObject *) s);

	UNWRAP(ps, s, drawWindowTexture);
	(*s->drawWindowTexture) (w, texture, attrib, mask);
	WRAP(ps, s, drawWindowTexture, pluginDrawWindowTexture);

	PrivWindow *pw = compObjectGetPrivate((CompObject *) w);
	if (pw->active[0] == pw->active[1])
		return;

	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	for (unsigned long i = 0; i < pw->active[1] - pw->active[0]; ++i) {
		if (pw->local[i].region == NULL)
			continue;

		FragmentAttrib fa = *attrib;

		int param = allocFragmentParameters(&fa, 2);
		int unit = allocFragmentTextureUnits(&fa, 1);

		int function = getProfileShader(s, texture, param, unit);
		if (function)
			addFragmentFunction(&fa, function);

		glProgramEnvParameter4dARB(GL_FRAGMENT_PROGRAM_ARB, param + 0, pw->local[i].scale, pw->local[i].scale, pw->local[i].scale, 1.0);
		glProgramEnvParameter4dARB(GL_FRAGMENT_PROGRAM_ARB, param + 1, pw->local[i].offset, pw->local[i].offset, pw->local[i].offset, 0.0);

		(*s->activeTexture) (GL_TEXTURE0_ARB + unit);
		glEnable(GL_TEXTURE_3D);
		glBindTexture(GL_TEXTURE_3D, pw->local[i].clutTexture);
		(*s->activeTexture) (GL_TEXTURE0_ARB);

		glStencilFunc(GL_EQUAL, 1, ~0);

		UNWRAP(ps, s, drawWindowTexture);
		(*s->drawWindowTexture) (w, texture, &fa, mask);
		WRAP(ps, s, drawWindowTexture, pluginDrawWindowTexture);

		(*s->activeTexture) (GL_TEXTURE0_ARB + unit);
		glBindTexture(GL_TEXTURE_3D, 0);
		glDisable(GL_TEXTURE_3D);
		(*s->activeTexture) (GL_TEXTURE0_ARB);
	}

	glDisable(GL_STENCIL_TEST);

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
	pd->netColorType = XInternAtom(d->display, "_NET_COLOR_TYPE", False);

	return TRUE;
}

static CompBool pluginInitScreen(CompPlugin *plugin, CompObject *object, void *privateData)
{
	CompScreen *s = (CompScreen *) object;
	PrivScreen *ps = privateData;

	GLint stencilBits = 0;
	glGetIntegerv(GL_STENCIL_BITS, &stencilBits);
	if (stencilBits == 0)
		return FALSE;

	WRAP(ps, s, drawWindow, pluginDrawWindow);
	WRAP(ps, s, drawWindowTexture, pluginDrawWindowTexture);

	ps->nProfiles = 0;
	ps->function = 0;

	/* XRandR setup code */

	XRRSelectInput(s->display->display, s->root, RRScreenChangeNotifyMask);

	ps->nOutputs = 0;
	updateOutputConfiguration(s);

	return TRUE;
}

static CompBool pluginInitWindow(CompPlugin *plugin, CompObject *object, void *privateData)
{
	/* CompWindow *w = (CompWindow *) object; */
	PrivWindow *pw = privateData;

	pw->nRegions = 0;
	pw->active[0] = pw->active[1] = 0;

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

