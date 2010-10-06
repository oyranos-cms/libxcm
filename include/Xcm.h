/*  @file Xcm.h
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

#ifndef __XCOLOR_H__
#define __XCOLOR_H__

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <stdint.h>
#include <arpa/inet.h>

#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** \addtogroup Xcm X Color Management Core API's

 *  @{
 */


/**
 *    The XcolorProfile typedefed structure
 * describes a single ICC profile. The 'md5' field is used to identify the
 * profile. The actual data follows right after the structure.
 */
typedef struct {
	uint8_t md5[16];  /**< ICC MD5 of the profile	        */
	uint32_t length;  /**< number of bytes following, network byte order */
} XcolorProfile;

/**
 *    The XcolorRegion typedefed structure
 * describes a single region with an attached profile. The region is
 * evaluated when the client enables the region and not when the region
 * is atttached to the window. This allows clients to update the region
 * (when the window is resized for example) by simply modifying the
 * XserverRegion and then sending a ClientMessage.
 */ 
typedef struct {
	uint32_t region;  /**< XserverRegion, network byte order */
	uint8_t md5[16];  /**< ICC MD5 of the associated profile	*/
} XcolorRegion;


/** Function  XcolorProfileUpload
 *  @brief    Uploads the profile into all screens of the display.
 *
 * Uploads the profile into all screens of the display. Profiles are
 * ref-counted inside the compositing manager, so make sure to call
 * XcolorProfileDelete() before your application exits or when you don't
 * need the profile anymore.
 */
int XcolorProfileUpload(Display *dpy, XcolorProfile *profile);

/** Function  XcolorProfileDelete
 *  @brief    Decreases the ref-count of the profile
 * 
 * You shouldn't use the profile
 * anymore after this call because it could have been removed from the
 * internal database in the compositing manager. If you do, nothing bad
 * will happen, just that color management won't work on regions that use
 * this profile.
 */
int XcolorProfileDelete(Display *dpy, XcolorProfile *profile);


/** Function  XcolorRegionInsert
 *  @brief    Inserts the supplied regions into the stack
 *
 * Inserts the supplied regions into the stack at the position 'pos', shifting
 * the existing profiles upwards in the stack. If 'pos' is beyond the stack end,
 * nothing will be done and the function returns '-1'.
 */
int XcolorRegionInsert(Display *dpy, Window win, unsigned long pos, XcolorRegion *region, unsigned long nRegions);

/** Function  XcolorRegionFetch
 *  @brief    Fetches the existing regions
 *
 * Fetches the existing regions and returns an array of 'XcolorRegion'. After
 * you're done, free the array using XFree(). The number of regions is put into
 * 'nRegions'.
 */
XcolorRegion *XcolorRegionFetch(Display *dpy, Window win, unsigned long *nRegions);

/** Function  XcolorRegionDelete
 *  @brief    Deletes regions
 *
 * Deletes 'count' regions starting at 'start' in the stack. If 'start + count'
 * in beyond the stack end, nothing will be done and the function returns '-1'.
 */
int XcolorRegionDelete(Display *dpy, Window win, unsigned long start, unsigned long count);


/** Function  XcolorRegionActivate
 *  @brief    Activates regions
 *
 * Activates 'count' regions starting at positiong 'start' in the stack. Unlike
 * the other functions it does not check whether 'start + count' extends beyond
 * the stack end. To disable all regions pass zero to 'count'.
 */
int XcolorRegionActivate(Display *dpy, Window win, unsigned long start, unsigned long count);

/** 
 *  @} *//*Xcm
 */

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */


#endif /* __XCOLOR_H__ */
