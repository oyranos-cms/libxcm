
#define GL_GLEXT_PROTOTYPES

#include <compiz-core.h>
#include <assert.h>
#include <string.h>

#include <lcms.h>

static const char *shaderSource =
	"!!ARBfp1.0"
	"TEMP input;"
	"TEMP output;"
	"TEX input, fragment.texcoord[0], texture[0], %s;"
	"MUL input, input, program.local[0];" // offset
	"ADD input, input, program.local[1];" // scale
	"TEX output, input, texture[1], 3D;"
	"MUL result.color, fragment.color, output;"
	"END"
;



typedef void (*dispatchObjectProc) (CompPlugin *plugin, CompObject *object, void *privateData);


static CompMetadata pluginMetadata;

static int corePrivateIndex;

typedef struct {
	int childPrivateIndex;

	ObjectAddProc objectAdd;
} PrivCore;

typedef struct {
	int childPrivateIndex;

	HandleEventProc handleEvent;
} PrivDisplay;

typedef struct {
	int childPrivateIndex;

	DrawWindowTextureProc drawWindowTexture;

	GLuint clutTexture;
	GLfloat scale, offset;
} PrivScreen;

typedef struct {
	int nRegions;
	Region *region;

	GLuint function;
} PrivWindow;


/**
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

#define __priv(o) compObjectGetPrivate((CompObject *) o)

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

static void clutGenerate(CompScreen *s)
{
	PrivScreen *ps = compObjectGetPrivate((CompObject *) s);

#define GRIDPOINTS 64
static GLushort clut[GRIDPOINTS][GRIDPOINTS][GRIDPOINTS][3];

	int r, g, b, n = GRIDPOINTS;

 	cmsHPROFILE sRGB = cmsCreate_sRGBProfile();
	cmsHPROFILE monitor = cmsOpenProfileFromFile("/home/tomc/profile.icc", "r");

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

static int getFragmentFunction(CompWindow *w, CompTexture *texture, const FragmentAttrib *attrib)
{
	PrivWindow *pw = compObjectGetPrivate((CompObject *) w);

	CompScreen *s = w->screen;

	if (pw->function)
		return pw->function;

	glGetError();

	(*s->genPrograms) (1, &pw->function);
	(*s->bindProgram) (GL_FRAGMENT_PROGRAM_ARB, pw->function);

	char programString[250];
	sprintf(programString, shaderSource, texture->target == GL_TEXTURE_2D ? "2D" : "RECT");
	(*s->programString) (GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, strlen (programString), programString);

	GLint errorPos;
	glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorPos);
	if (glGetError() != GL_NO_ERROR || errorPos != -1) {
		compLogMessage (NULL, "cm", CompLogLevelError, "failed to load fragment program: %d", errorPos);
	}

	(*s->bindProgram) (GL_FRAGMENT_PROGRAM_ARB, 0);

	return pw->function;
}

static void pluginHandleEvent(CompDisplay *d, XEvent *event)
{
	PrivDisplay *pd = compObjectGetPrivate((CompObject *) d);

	UNWRAP(pd, d, handleEvent);
	(*d->handleEvent) (d, event);
	WRAP(pd, d, handleEvent, pluginHandleEvent);
}

static void pluginDrawWindowTexture(CompWindow *w, CompTexture *texture, const FragmentAttrib *attrib, unsigned int mask)
{
	/* PrivWindow *pw = compObjectGetPrivate((CompObject *) w); */

	CompScreen *s = w->screen;
	PrivScreen *ps = compObjectGetPrivate((CompObject *) s);

	UNWRAP(ps, s, drawWindowTexture);
	(*s->drawWindowTexture) (w, texture, attrib, mask);
	WRAP(ps, s, drawWindowTexture, pluginDrawWindowTexture);

	/* only draw the window contents, not the decoration */
	if (texture != w->texture)
		return;

	int function = getFragmentFunction(w, texture, attrib);

	glEnable(GL_FRAGMENT_PROGRAM_ARB);
	(*s->bindProgram) (GL_FRAGMENT_PROGRAM_ARB, function);

	(*s->programLocalParameter4f) (GL_FRAGMENT_PROGRAM_ARB, 0,
		ps->scale, ps->scale, ps->scale, 1.0);
	(*s->programLocalParameter4f) (GL_FRAGMENT_PROGRAM_ARB, 1,
		ps->offset, ps->offset, ps->offset, 0.0);

	(*s->activeTexture) (GL_TEXTURE0_ARB);
	enableTexture(s, texture, COMP_TEXTURE_FILTER_FAST);

	(*s->activeTexture) (GL_TEXTURE0_ARB + 1);
	glEnable(GL_TEXTURE_3D);
	glBindTexture(GL_TEXTURE_3D, ps->clutTexture);

	GLfloat width = w->attrib.width, height = w->attrib.height;
	GLfloat y0 = texture->matrix.y0 * height;

	GLfloat verts[] = {
		texture->matrix.xx * width / 2, y0,
			w->attrib.x + width / 2, w->attrib.y,
		texture->matrix.xx * width / 2, y0 + texture->matrix.yy * height,
			w->attrib.x + width / 2, w->attrib.y + height,
		texture->matrix.xx * width, y0 + texture->matrix.yy * height,
			w->attrib.x + width, w->attrib.y + height,
		texture->matrix.xx * width, y0,
			w->attrib.x + width, w->attrib.y
	};

	glTexCoordPointer(2, GL_FLOAT, 4 * sizeof(GLfloat), verts);
	glVertexPointer(2, GL_FLOAT, 4 * sizeof(GLfloat), &verts[2]);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glEnable(GL_BLEND);

	glColor4us(0x4fff, 0x4fff, 0x4fff, 0xffff);
	glDrawArrays(GL_QUADS, 0, 4);

	glDisable(GL_TEXTURE_3D);
	(*s->activeTexture) (GL_TEXTURE0_ARB);

	disableTexture(s, texture);

	(*s->bindProgram) (GL_FRAGMENT_PROGRAM_ARB, 0);
	glDisable(GL_FRAGMENT_PROGRAM_ARB);

	glColor4us(0x4fff, 0x2fff, 0x2fff, 0x9fff);
	glLineWidth(3.0);
	glDrawArrays(GL_LINES, 0, 2);

	glColor4usv(defaultColor);

	glDisable(GL_BLEND);

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
}

static void pluginInitScreen(CompPlugin *plugin, CompObject *object, void *privateData)
{
	CompScreen *s = (CompScreen *) object;
	PrivScreen *ps = privateData;

	WRAP(ps, s, drawWindowTexture, pluginDrawWindowTexture);

	clutGenerate(s);
}

static void pluginInitWindow(CompPlugin *plugin, CompObject *object, void *privateData)
{
	/* CompWindow *w = (CompWindow *) object; */
	PrivWindow *pw = privateData;

	pw->nRegions = 0;
	pw->region = NULL;

	pw->function = 0;
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

