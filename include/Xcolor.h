
#ifndef __XCOLOR_H__
#define __XCOLOR_H__

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <stdint.h>
#include <arpa/inet.h>

#include <stdlib.h>
#include <string.h>


/**
 *    XcolorProfile
 * Describes a single ICC profile. The 'md5' field is used to identify the
 * profile. The actual data follows right after the structure.
 */
typedef struct {
	uint8_t md5[16];  /* MD5 of the profile		                */
	uint32_t length;  /* number of bytes following		        */
} XcolorProfile;

/**
 *    XcolorRegion
 * Describes a single region with an attached profile. The region is
 * evaluated when the client enables the region and not when the region
 * is atttached to the window. This allows clients to update the region
 * (when the window is resized for example) by simply modifying the
 * XserverRegion and then sending a ClientMessage.
 */ 
typedef struct {
	uint32_t region;  /* XserverRegion				*/
	uint8_t md5[16];  /* MD5 of the associated profile	        */
} XcolorRegion;


/**
 *    XcolorProfileUpload
 * Uploads the profile into all screens of the display. Profiles are
 * ref-counted inside the compositing manager so make sure to call
 * XcolorProfileDelete() before your application exits or when you don't
 * need the profile anymore.
 */
int XcolorProfileUpload(Display *dpy, XcolorProfile *profile);

/**
 *    XcolorProfileDelete
 * Decreases the ref-count of the profile. You shouldn't use the profile
 * anymore after this call because it could have been removed from the
 * internal database in the compositing manager. If you do, nothing bad
 * will happen, just that color management won't work on regions that use
 * this profile.
 */
int XcolorProfileDelete(Display *dpy, XcolorProfile *profile);


/**
 *    XcolorRegionInsert
 * Inserts the supplied regions into the stack at the position 'pos', shifting
 * the existing profiles upwards in the stack. If 'pos' is beyond the stack end,
 * nothing will be done and the function returns '-1'.
 */
int XcolorRegionInsert(Display *dpy, Window win, unsigned long pos, XcolorRegion *region, unsigned long nRegions);

/**
 *    XcolorRegionFetch
 * Fetches the existing regions and returns an array of 'XcolorRegion'. After
 * you're done, free the array using XFree(). The number of regions is put into
 * 'nRegions'.
 */
XcolorRegion *XcolorRegionFetch(Display *dpy, Window win, unsigned long *nRegions);

/**
 *    XcolorRegionDelete
 * Deletes 'count' regions starting at 'start' in the stack. If 'start + count' is
 * beyond the stack end, nothing will be done and the function returns '-1'.
 */
int XcolorRegionDelete(Display *dpy, Window win, unsigned long start, unsigned long count);


/**
 *    XcolorRegionActivate
 * Activates 'count' regions starting at positiong 'start' in the stack. Unlike
 * the other functions it does not check whether 'start + count' extends beyond
 * the stack end. To disable all regions pass zero to 'count'.
 */
int XcolorRegionActivate(Display *dpy, Window win, unsigned long start, unsigned long count);

#endif /* __XCOLOR_H__ */
