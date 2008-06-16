
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <X11/extensions/Xfixes.h>

#include <cairo.h>
#include <cairo-xlib.h>

int main(int argc, char *argv[])
{
	Display *dpy = XOpenDisplay(NULL);

	int screen = DefaultScreen(dpy);
	Visual *vis = DefaultVisual(dpy, screen);

	Colormap cmap = XCreateColormap(dpy, RootWindow(dpy, screen), vis, AllocNone);

	XSetWindowAttributes attrs;
	attrs.colormap = cmap;
	attrs.border_pixel = 0;
	attrs.event_mask = StructureNotifyMask | ExposureMask;

	Window w = XCreateWindow(dpy, XRootWindow(dpy, screen), 200, 200, 300, 300, 5, 24,
				 InputOutput, DefaultVisual(dpy, screen),
				 CWBorderPixel | CWColormap | CWEventMask, &attrs);

	XMapWindow(dpy, w); 

	cairo_surface_t *surface = cairo_xlib_surface_create(dpy, w, DefaultVisual(dpy, screen), 300, 300);
	cairo_t *cr = cairo_create(surface);

	cairo_scale(cr, 300, 300);


	/**
	 * Color management
	 */

	XRectangle rec = { 100, 100, 150, 100 };
	XserverRegion reg = XFixesCreateRegion(dpy, &rec, 1);

	Atom cmRegionsAtom = XInternAtom(dpy, "_NET_CM_REGIONS", False);
	Atom cmPropertyType = XInternAtom(dpy, "_NET_CM_TYPE", False);
	XChangeProperty(dpy, w, cmRegionsAtom, cmPropertyType, 32, PropModeReplace, &reg, 1);

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
		}
	}


	XDestroyWindow(dpy, w);
	XCloseDisplay(dpy);

	return 0;
}
