
#define _BSD_SOURCE

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/keysymdef.h>

#include <X11/extensions/Xfixes.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/Xinerama.h>

#include <cairo.h>
#include <cairo-xlib.h>

#include <alpha/oyranos_alpha.h>

#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <Xcm.h>

static void *readFile(const char *path, unsigned long *nBytes)
{
	int fd = open(path, O_RDONLY);

	struct stat buf;
	fstat(fd, &buf);

        if(fd < 0) return NULL;

	void *ret = malloc(buf.st_size);
	read(fd, ret, buf.st_size);

	close(fd);

	*nBytes = buf.st_size;

	return ret;	
}

/* Force Xinerama screen detection instead of RandR. */
static int forceXinerama = 0;

int main(int argc, char *argv[])
{
	/* Parse commandline options. */
	int c;
	while ((c = getopt(argc, argv, "x")) != -1) {
		switch(c) {
		case 'x':
			forceXinerama = 1;
			break;
		default:
			printf("Usage: %s [-x]\n", argv[0]);
			return -1;
		}
	}

	/* Open the display and create our window. */
	Display *dpy = XOpenDisplay(NULL);

	int screen = DefaultScreen(dpy);
	Visual *vis = DefaultVisual(dpy, screen);

	Colormap cmap = XCreateColormap(dpy, RootWindow(dpy, screen), vis, AllocNone);

	XSetWindowAttributes attrs;
        memset( &attrs, 0, sizeof(XSetWindowAttributes) );
	attrs.colormap = cmap;
	attrs.border_pixel = 0;
	attrs.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;

	Window w = XCreateWindow(dpy, XRootWindow(dpy, screen), 200, 200, 300, 300, 5, 24,
				 InputOutput, DefaultVisual(dpy, screen),
				 CWBorderPixel | CWColormap | CWEventMask, &attrs);

	XMapWindow(dpy, w); 

	cairo_surface_t *surface = cairo_xlib_surface_create(dpy, w, DefaultVisual(dpy, screen), 300, 300);
	cairo_t *cr = cairo_create(surface);

	cairo_scale(cr, 300, 300);

        XFlush( dpy );

	/**
	 * Color management
	 */

	unsigned long activeOutput = 0, nOutputs = 0;
	char *outputName[4];
	
	/* To query the output layout first try to use RandR, if that fails try Xinerama. */
	int rrEventBase, rrErrorBase;
	if (XRRQueryExtension(dpy, &rrEventBase, &rrErrorBase) == True && !forceXinerama) {
                unsigned long i;
		XRRScreenResources *res = XRRGetScreenResources(dpy, XRootWindow(dpy, screen));

		printf("Found %d RandR outputs:\n", res->noutput);
		for (i = 0; i < res->noutput; ++i) {
			XRROutputInfo *oinfo = XRRGetOutputInfo(dpy, res, res->outputs[i]);
			outputName[i] = strdup(oinfo->name);
			printf("  %s\n", outputName[i]);
			XRRFreeOutputInfo(oinfo);
		}

		nOutputs = res->noutput;
		
		XRRFreeScreenResources(res);
	} else {
                int i;
		int xiEventBase, xiErrorBase;
		if (XineramaQueryExtension(dpy, &xiEventBase, &xiErrorBase) == False) {
			printf("Neither RandR nor Xinerama available!\n");
			return -1;
		}

		int nScreens;
		XineramaScreenInfo *screen = XineramaQueryScreens(dpy, &nScreens);

		printf("Found %d Xinerama screens:\n", nScreens);
		for (i = 0; i < nScreens; ++i) {
			char name[128];
			snprintf(name, 128, "Xinerama-Screen-%d", i);
			outputName[i] = strdup(name);
			printf("  %s\n", outputName[i]);
		}

		nOutputs = nScreens;

		XFree(screen);
	}
		

	Atom netColorTarget = XInternAtom(dpy, "_NET_COLOR_TARGET", False);
	XChangeProperty(dpy, w, netColorTarget, XA_STRING, 8, PropModeReplace, (unsigned char *) outputName[activeOutput], strlen(outputName[activeOutput]));

	unsigned long nBytes = 0;
	void *blob = readFile("profile.icc", &nBytes);
	oyProfile_s * p = oyProfile_FromFile( "./profile.icc", 0,0 );

	/* Create a XcolorProfile object that will be uploaded to the display.*/
	XcolorProfile *profile = malloc(sizeof(XcolorProfile) + nBytes);

	oyProfile_GetMD5(p, OY_FROM_PROFILE, (uint32_t*)profile->md5);
	profile->length = htonl(nBytes);
	memcpy(profile + 1, blob, nBytes);

	int result = XcolorProfileUpload(dpy, profile);
        if(result)
		printf("XcolorProfileUpload: %d\n", result);

	oyProfile_Release( &p );

	/* Upload the region to the window. */
	XRectangle rec[3] = { { 50, 25, 200, 175 }, { 25, 175, 100, 100 },{0,0,0,0} };
	XserverRegion reg = XFixesCreateRegion(dpy, rec, 2);

	XcolorRegion region;
	region.region = reg;
	memcpy(region.md5, profile->md5, 16);

	XcolorRegionInsert(dpy, w, 0, &region, 1);

	/* When the escape key is pressed, the application cleans up all resources and exits. */
	KeyCode escape = XKeysymToKeycode(dpy, XStringToKeysym("Escape"));

	/* When the delete key is pressed, the application switches the target output. */
        KeyCode delete = XKeysymToKeycode(dpy, XStringToKeysym("Delete"));


	for (;;) {
		XEvent event;
		XNextEvent(dpy, &event);

		if (event.type == Expose) {
			cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
			cairo_paint(cr);

			cairo_set_source_rgb(cr, 0, 0, 0);
			cairo_move_to(cr, 0, 0);
			cairo_line_to(cr, 1, 1);
			cairo_move_to(cr, 1, 0);
			cairo_line_to(cr, 0, 1);
			cairo_set_line_width(cr, 0.2);
			cairo_stroke(cr);

			cairo_rectangle(cr, 0, 0, 0.5, 0.5);
			cairo_set_source_rgba(cr, 1, 0, 0, 0.80);
			cairo_fill(cr);

			cairo_rectangle(cr, 0, 0.5, 0.5, 0.5);
			cairo_set_source_rgba(cr, 0, 1, 0, 0.60);
			cairo_fill(cr);

			cairo_rectangle(cr, 0.5, 0, 0.5, 0.5);
			cairo_set_source_rgba(cr, 0, 0, 1, 0.40);
			cairo_fill(cr);
		} else if (event.type == KeyPress) {
			if (event.xkey.keycode == escape) {
				break;
			} else if (event.xkey.keycode == delete) {
				if (++activeOutput == nOutputs)
					activeOutput = 0;

				XChangeProperty(dpy, w, netColorTarget, XA_STRING, 8, PropModeReplace, (unsigned char *) outputName[activeOutput], strlen(outputName[activeOutput]));

				printf("Changed target output to %s\n", outputName[activeOutput]);
			} else {
				static long count = 0;
				int res = 0;
			    
				count = (count + 1) % 2;
				res = XcolorRegionActivate(dpy, w, 0, count);

				printf("Activated regions 0 - %li %d\n", count, res);
			}
		}
	}

	/* Delete the profile. The ref-count inside the compiz plugin drops to zero
	 * and the profile resources will be freed. */
	profile->length = 0;
	XcolorProfileDelete(dpy, profile);

	XDestroyWindow(dpy, w);
	XCloseDisplay(dpy);

	return 0;
}

