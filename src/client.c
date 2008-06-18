
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <X11/extensions/Xfixes.h>

#include <cairo.h>
#include <cairo-xlib.h>

#include <stdio.h>

int main(int argc, char *argv[])
{
	Display *dpy = XOpenDisplay(NULL);

	int screen = DefaultScreen(dpy);
	Visual *vis = DefaultVisual(dpy, screen);

	Colormap cmap = XCreateColormap(dpy, RootWindow(dpy, screen), vis, AllocNone);

	XSetWindowAttributes attrs;
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


	/**
	 * Color management
	 */

	/* Region 1, one rectangle */
	XRectangle rec1 = { 100, 50, 150, 80 };
	XserverRegion reg1 = XFixesCreateRegion(dpy, &rec1, 1);
       
	/* Region 2, two rectangles */
	XRectangle rec2[2] = { { 50, 200, 80, 50 }, { 50, 200, 50, 80 } };
	XserverRegion reg2 = XFixesCreateRegion(dpy, rec2, 2);

	Atom cmRegionsAtom = XInternAtom(dpy, "_NET_COLOR_REGIONS", False);
	Atom cmPropertyType = XInternAtom(dpy, "_NET_COLOR_TYPE", False);

	unsigned long data[2] = { reg1, reg2 };
	XChangeProperty(dpy, w, cmRegionsAtom, cmPropertyType, 32, PropModeReplace, (unsigned char *) &data, 2);

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
			XClientMessageEvent xev;
			static long enable = 0;

			xev.type = ClientMessage;
			xev.window = w;
			xev.message_type = XInternAtom(dpy, "_NET_COLOR_MANAGEMENT", False);
			xev.format = 32;

			++enable;
			xev.data.l[0] = enable % 2;

			XSendEvent(dpy, RootWindow(dpy, screen), False, ExposureMask, (XEvent *) &xev);

			printf("Sent color manangement request: %i\n", enable % 2);
		}
	}


	XDestroyWindow(dpy, w);
	XCloseDisplay(dpy);

	return 0;
}
