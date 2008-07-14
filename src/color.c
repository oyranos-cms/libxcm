
#define GL_GLEXT_PROTOTYPES

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <X11/extensions/Xfixes.h>

#include <compiz-core.h>

#include <assert.h>
#include <string.h>

#include <stdarg.h>
#include <lcms.h>

#include "xcolor.h"


typedef void (*dispatchObjectProc) (CompPlugin *plugin, CompObject *object, void *privateData);


typedef struct  {
	Region region;

	GLuint clutTexture;
	GLfloat scale, offset;
} ColorRegion;


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

	GLuint clutTexture;
	GLfloat scale, offset;
} PrivScreen;

typedef struct {
	unsigned long nRegions;
	Region *region;

	unsigned long enabled;
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
	if (o == NULL) {
		return &corePrivateIndex;
	} else {
		int *privateIndex = compObjectGetPrivate(o->parent);
		return o->privates[*privateIndex].ptr;
	}
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
	if (privateData == NULL);
		return;

	if (object->type > 0) {
		compObjectFreePrivateIndex(parent, object->type, *privateIndex);
	}

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

static void clutGenerate(CompScreen *s)
{
	PrivScreen *ps = compObjectGetPrivate((CompObject *) s);

#define GRIDPOINTS 64
static GLushort clut[GRIDPOINTS][GRIDPOINTS][GRIDPOINTS][3];

	int r, g, b, n = GRIDPOINTS;

 	cmsHPROFILE sRGB = cmsCreate_sRGBProfile();
	cmsHPROFILE monitor = cmsOpenProfileFromFile("/home/tomc/Desktop/FakeBRG.icc", "r");

	cmsHTRANSFORM xform = cmsCreateTransform(sRGB, TYPE_RGB_16, monitor, TYPE_RGB_16, INTENT_PERCEPTUAL, cmsFLAGS_NOTPRECALC);

	if (xform == NULL) {
		fprintf(stderr, "Failed to create transformation\n");
		return;
	}

	ps->scale = (double) (n - 1) / n;
	ps->offset = 1.0 / (2 * n);

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
	cmsCloseProfile(monitor);
	cmsCloseProfile(sRGB);

	glGenTextures (1, &ps->clutTexture);
	glBindTexture(GL_TEXTURE_3D, ps->clutTexture);

	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB16, n, n, n, 0, GL_RGB, GL_UNSIGNED_SHORT, clut);
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

	fprintf(stderr, "%lu profiles\n", count);

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

static void updateWindowRegions(CompDisplay *d, CompWindow *w)
{
	PrivDisplay *pd = compObjectGetPrivate((CompObject *) d);

	PrivWindow *pw = compObjectGetPrivate((CompObject *) w);
      
	if (pw->nRegions) {
		for (int i = 0; i < pw->nRegions; ++i)
			XDestroyRegion(pw->region[i]);

		pw->nRegions = 0;
		free(pw->region);
	}

	Atom actual;
	int format;
	unsigned long n, left, *data;

	int result = XGetWindowProperty(d->display, w->id, pd->netColorRegions, 0, ~0, False,
					pd->netColorType, &actual, &format, &n, &left, (unsigned char **) &data);

	if (result != Success)
		return;     

	pw->region = malloc(n * sizeof(Region));
	if (pw->region == NULL)
		return;

	pw->nRegions = n;

	compLogMessage(w->screen->display, "color", CompLogLevelWarn, "got %d regions", n);		
 
	for (int i = 0; i < n; ++i) {
		XserverRegion serverRegion = data[i];
		pw->region[i] = convertRegion(d->display, serverRegion);
		compLogMessage(d, "color", CompLogLevelWarn, " has %d rectangles", pw->region[i]->numRects);
	}

	XFree(data);
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
		}
		break;
	case ClientMessage:
		if (event->xclient.message_type == pd->netColorManagement) {
			CompWindow *w = findWindowAtDisplay (d, event->xclient.window);
			if (w == NULL)
				return;

			long enable = event->xclient.data.l[0];
			compLogMessage(d, "color", CompLogLevelWarn, "received color management request: %i", enable);

			updateWindowRegions(d, w);
			addWindowDamage(w);

			PrivWindow *pw = compObjectGetPrivate((CompObject *) w);
			pw->enabled = enable;
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
	PrivWindow *pw = compObjectGetPrivate((CompObject *) w);

	CompScreen *s = w->screen;
	PrivScreen *ps = compObjectGetPrivate((CompObject *) s);

	//compLogMessage(s->display, "color", CompLogLevelWarn, "draw window %p", w);

	if (pw->nRegions > 0) {
		glEnable(GL_STENCIL_TEST);
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

		w->vCount = w->indexCount = 0;
		(*w->screen->addWindowGeometry) (w, &w->matrix, 1, w->region, region);
		glClear(GL_STENCIL_BUFFER_BIT);

		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

		for (int i = 0; i < pw->nRegions; ++i) {
			glStencilFunc(GL_ALWAYS, i + 1, ~0);

			Region tmp = absoluteRegion(w, pw->region[i]);
		       
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
	}

	UNWRAP(ps, s, drawWindow);
	Bool status = (*s->drawWindow) (w, transform, attrib, region, mask);
	WRAP(ps, s, drawWindow, pluginDrawWindow);

	return status;
}

static void pluginDrawWindowTexture(CompWindow *w, CompTexture *texture, const FragmentAttrib *attrib, unsigned int mask)
{
	PrivWindow *pw = compObjectGetPrivate((CompObject *) w);
	
	CompScreen *s = w->screen;
	PrivScreen *ps = compObjectGetPrivate((CompObject *) s);

	if (texture != w->texture || pw->nRegions == 0 || pw->enabled == 0) {
		UNWRAP(ps, s, drawWindowTexture);
		(*s->drawWindowTexture) (w, texture, attrib, mask);
		WRAP(ps, s, drawWindowTexture, pluginDrawWindowTexture);

		return;
	}

	glEnable(GL_STENCIL_TEST);    
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	glStencilFunc(GL_EQUAL, 0, ~0);

	UNWRAP(ps, s, drawWindowTexture);
	(*s->drawWindowTexture) (w, texture, attrib, mask);
	WRAP(ps, s, drawWindowTexture, pluginDrawWindowTexture);

	for (int i = 0; i < pw->nRegions; ++i) {
		FragmentAttrib fa = *attrib;

		int param = allocFragmentParameters(&fa, 2);
		int unit = allocFragmentTextureUnits(&fa, 1);

		int function = getProfileShader(s, texture, param, unit);
		if (function)
			addFragmentFunction(&fa, function);

		glProgramEnvParameter4dARB(GL_FRAGMENT_PROGRAM_ARB, param + 0, ps->scale, ps->scale, ps->scale, 1.0);

		glProgramEnvParameter4dARB(GL_FRAGMENT_PROGRAM_ARB, param + 1, ps->offset, ps->offset, ps->offset, 0.0);

		(*s->activeTexture) (GL_TEXTURE0_ARB + unit);
		glEnable(GL_TEXTURE_3D);
		glBindTexture(GL_TEXTURE_3D, ps->clutTexture);
		(*s->activeTexture) (GL_TEXTURE0_ARB);

		glStencilFunc(GL_EQUAL, i + 1, ~0);

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
 *    Object Init Functions
 */

static void pluginInitCore(CompPlugin *plugin, CompObject *object, void *privateData)
{
}

static void pluginInitDisplay(CompPlugin *plugin, CompObject *object, void *privateData)
{
	CompDisplay *d = (CompDisplay *) object;
	PrivDisplay *pd = privateData;

	WRAP(pd, d, handleEvent, pluginHandleEvent);

	pd->netColorManagement = XInternAtom(d->display, "_NET_COLOR_MANAGEMENT", False);

	pd->netColorProfiles = XInternAtom(d->display, "_NET_COLOR_PROFILES", False);
	pd->netColorRegions = XInternAtom(d->display, "_NET_COLOR_REGIONS", False);
	pd->netColorType = XInternAtom(d->display, "_NET_COLOR_TYPE", False);
}

static void pluginInitScreen(CompPlugin *plugin, CompObject *object, void *privateData)
{
	CompScreen *s = (CompScreen *) object;
	PrivScreen *ps = privateData;

	WRAP(ps, s, drawWindow, pluginDrawWindow);
	WRAP(ps, s, drawWindowTexture, pluginDrawWindowTexture);

	ps->function = 0;
	clutGenerate(s);

	GLint stencilBits = 0;
	glGetIntegerv(GL_STENCIL_BITS, &stencilBits);
	if (stencilBits == 0)
		compLogMessage(s->display, "color", CompLogLevelWarn, "No stencil buffer. Region based color management disabled");
}

static void pluginInitWindow(CompPlugin *plugin, CompObject *object, void *privateData)
{
	/* CompWindow *w = (CompWindow *) object; */
	PrivWindow *pw = privateData;

	pw->nRegions = 0;
	pw->region = NULL;

	pw->enabled = 0;
}

static dispatchObjectProc dispatchInitObject[] = {
	pluginInitCore, pluginInitDisplay, pluginInitScreen, pluginInitWindow
};

/**
 *    Object Fini Functions
 */


static void pluginFiniCore(CompPlugin *plugin, CompObject *object, void *privateData)
{
}

static void pluginFiniDisplay(CompPlugin *plugin, CompObject *object, void *privateData)
{
	CompDisplay *d = (CompDisplay *) object;
	PrivDisplay *pd = privateData;

	UNWRAP(pd, d, handleEvent);
}

static void pluginFiniScreen(CompPlugin *plugin, CompObject *object, void *privateData)
{
	CompScreen *s = (CompScreen *) object;
	PrivScreen *ps = privateData;

	UNWRAP(ps, s, drawWindow);
	UNWRAP(ps, s, drawWindowTexture);
}

static void pluginFiniWindow(CompPlugin *plugin, CompObject *object, void *privateData)
{
}

static dispatchObjectProc dispatchFiniObject[] = {
	pluginFiniCore, pluginFiniDisplay, pluginFiniScreen, pluginFiniWindow
};


/**
 *    Plugin Interface
 */
static Bool pluginInit(CompPlugin *p)
{
	corePrivateIndex = allocateCorePrivateIndex();
	if (corePrivateIndex < 0)
		return FALSE;

	return TRUE;
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


static CompBool pluginInitObject(CompPlugin *p, CompObject *o)
{
	static const int privateSizes[] = {
		sizeof(PrivCore), sizeof(PrivDisplay), sizeof(PrivScreen), sizeof(PrivWindow)
	};

	void *privateData = compObjectAllocPrivate(getParent(o), o, privateSizes[o->type]);
	if (privateData == NULL)
		return FALSE;

	dispatchInitObject[o->type](p, o, privateData);

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

