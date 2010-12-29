/*  @file Xcm.c
 *
 *  libXcm  Xorg Colour Management
 *
 *  @par Copyright:
 *            2008 (C) Tomas Carnecky
 *            2008-2010 (C) Kai-Uwe Behrmann
 *
 *  @brief    net-color spec helpers
 *  @internal
 *  @author   Tomas Carnecky
 *            Kai-Uwe Behrmann <ku.b@gmx.de>
 *  @par License:
 *            MIT <http://www.opensource.org/licenses/mit-license.php>
 *  @since    2008/04/00
 */

#include <Xcm.h>


int XcolorProfileUpload(Display *dpy, XcolorProfile *profile)
{
	/* XcolorProfile::length is in network byte-order, swap it now */
	uint32_t length = htonl(profile->length);
        int i;

	Atom netColorProfiles = XInternAtom(dpy, "_NET_COLOR_PROFILES", False);

	for (i = 0; i < ScreenCount(dpy); ++i) {
		XChangeProperty(dpy, XRootWindow(dpy, i), netColorProfiles, XA_CARDINAL, 8, PropModeAppend, (unsigned char *) profile, sizeof(XcolorProfile) + length);
	}

	return 0;
}

int XcolorProfileDelete(Display *dpy, XcolorProfile *profile)
{
	Atom netColorProfiles = XInternAtom(dpy, "_NET_COLOR_PROFILES", False);
        int i;

	/* To delete a profile, send the header with a zero-length. */
	profile->length = 0;

	for (i = 0; i < ScreenCount(dpy); ++i) {
		XChangeProperty(dpy, XRootWindow(dpy, i), netColorProfiles, XA_CARDINAL, 8, PropModeAppend, (unsigned char *) profile, sizeof(XcolorProfile));
	}

	return 0;
}



int XcolorRegionInsert(Display *dpy, Window win, unsigned long pos, XcolorRegion *region, unsigned long nRegions)
{
	Atom netColorRegions = XInternAtom(dpy, "_NET_COLOR_REGIONS", False);
	XcolorRegion *ptr;

	unsigned long nRegs;
	XcolorRegion *reg = XcolorRegionFetch(dpy, win, &nRegs);

	/* Security check to ensure that the client doesn't try to insert the regions
	 * to a position beyond the stack end. */
	if (pos > nRegs) {
		XFree(reg);
		return -1;
	}

	ptr = malloc((nRegs + nRegions) * sizeof(XcolorRegion));
	if (ptr == NULL) {
		XFree(reg);
		return -1;
	}

	memcpy(ptr, reg, nRegs * sizeof(XcolorRegion));

	/* Make space for the new regions and copy them to the array. */
	if (nRegs)
		memmove(ptr + pos + nRegs, ptr + pos, nRegions * sizeof(XcolorRegion));
	memcpy(ptr + pos, region, nRegions * sizeof(XcolorRegion));

	XChangeProperty(dpy, win, netColorRegions, XA_CARDINAL, 8, PropModeReplace, (unsigned char *) ptr, (nRegs + nRegions) * sizeof(XcolorRegion));

	XFree(reg);
	free(ptr);

	return 0;
}

XcolorRegion *XcolorRegionFetch(Display *dpy, Window win, unsigned long *nRegions)
{

	Atom actual, netColorRegions = XInternAtom(dpy, "_NET_COLOR_REGIONS", False);

	unsigned long left, nBytes;
	unsigned char *data;
       
	int format, result = XGetWindowProperty(dpy, win, netColorRegions, 0, ~0, False, XA_CARDINAL, &actual, &format, &nBytes, &left, &data);

	*nRegions = 0;
	if (result != Success)
		return NULL;

	*nRegions = nBytes / sizeof(XcolorRegion);
	return (XcolorRegion *) data;
}

int XcolorRegionDelete(Display *dpy, Window win, unsigned long start, unsigned long count)
{
	Atom netColorRegions = XInternAtom(dpy, "_NET_COLOR_REGIONS", False);

	unsigned long nRegions;
	XcolorRegion *region = XcolorRegionFetch(dpy, win, &nRegions);

	/* Security check to ensure that the client doesn't try to delete regions
	 * beyond the stack end. */
	if (start + count > nRegions) {
		XFree(region);
		return -1;
	}

	/* Remove the regions and close the gap. */
	memmove(region + start, region + start + count, (nRegions - start - count) * sizeof(XcolorRegion));

  if(nRegions - count)
  	XChangeProperty(dpy, win, netColorRegions, XA_CARDINAL, 8, PropModeReplace, (unsigned char *) region, (nRegions - count) * sizeof(XcolorRegion));
  else
    XDeleteProperty( dpy, win, netColorRegions );

  XFree(region);


	return 0;	
}

int XcolorRegionActivate(Display *dpy, Window win, unsigned long start, unsigned long count)
{
	XWindowAttributes xwa;
	Status status;

	/* Construct the XEvent. */
	XClientMessageEvent event;

	event.type = ClientMessage;
	event.window = win;
	event.message_type = XInternAtom(dpy, "_NET_COLOR_MANAGEMENT", False);
	event.format = 32;

	event.data.l[0] = start;
	event.data.l[1] = count;

	/* The ClientMessage has to be sent to the root window. Find the root window
	 * of the screen containing 'win'. */
        status = XGetWindowAttributes(dpy, RootWindow(dpy, 0), &xwa);
	if (status == 0)
		return -1;

	/* Uhm, why ExposureMask? */
	XSendEvent(dpy, xwa.root, False, ExposureMask, (XEvent *) &event);

	return 0;
}
