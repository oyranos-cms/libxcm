/*  @file netColorRegion2.c
 *
 *  libXcm  Xorg Colour Management
 *
 *  @par Copyright:
 *            2010-2011 (C) Kai-Uwe Behrmann
 *
 *  @brief    net-color spec example
 *  @internal
 *  @author   Kai-Uwe Behrmann <ku.b@gmx.de>
 *  @par License:
 *            MIT <http://www.opensource.org/licenses/mit-license.php>
 *  @since    2010/04/15
 */

/* Use the net-color spec to explicitely opt-out in server side colour 
 * management */

#include <X11/Xlib.h> /* XOpenDisplay() */
#include <X11/extensions/Xfixes.h> /* XserverRegion */
#include <X11/Xcm/Xcm.h> /* XcolorRegion */
#include <stdio.h>  /* printf() */
#include <unistd.h> /* sleep() */

#include <alpha/oyranos_alpha.h>

int main(int argc, char ** argv)
{
  Display * display = XOpenDisplay(NULL);
  int result = 0;
  int need_wait = 1;

  char * blob = 0;
  size_t size = 0;
  oyProfile_s * p = 0;
  XcolorProfile * profile = 0;

  Window w;

  XserverRegion reg = 0;
  XcolorRegion region;
  int error;
  XRectangle rec[2] = { { 0,0,0,0 }, { 0,0,0,0 } };

  if(!(5 < argc && argc < 8))
  {
    printf( "Usage:\n\t%s decimal_xwinid x y width height [profile.icc]\n",
            argv[0] );
    printf( "Get a windows ID:\n\txwininfo -int | grep 'Window id:' | awk '{print $4}'\n");
    return 0;
  }

  /* Upload a ICC profile to X11 root window */
  if(argc == 7)
  {
    p = oyProfile_FromFile( argv[6], 0,0 );
    if(p)
    {
      blob = oyProfile_GetMem( p, &size, 0,0 );

      if(blob && size)
      {
        /* Create a XcolorProfile object that will be uploaded to the display.*/
        profile = malloc(sizeof(XcolorProfile) + size);

        oyProfile_GetMD5(p, OY_FROM_PROFILE, (uint32_t*)profile->md5);

        profile->length = htonl(size);
        memcpy(profile + 1, blob, size);

        result = XcolorProfileUpload( display, profile );
        if(result)
          printf("XcolorProfileUpload: %d\n", result);
      }
      oyProfile_Release( &p );
    }
  }

  /* upload a region to a X11 window */
  sscanf( argv[1], "%i", (int*)&w );
  w = (Window) atoi(argv[1]);

  rec[0].x = atoi(argv[2]);
  rec[0].y = atoi(argv[3]);
  rec[0].width = atoi(argv[4]);
  rec[0].height = atoi(argv[5]);

  reg = XFixesCreateRegion( display, rec, 1);

  region.region = htonl(reg);
  if(blob && size)
    memcpy(region.md5, profile->md5, 16);
  else
    memset( region.md5, 0, 16 );

  if(rec[0].x || rec[0].y || rec[0].width || rec[0].height)
    error = XcolorRegionInsert( display, w, 0, &region, 1 );
  else
  {
    unsigned long nRegions = 0;
    XcolorRegion * r = XcolorRegionFetch( display, w, &nRegions );
    if(nRegions && r)
    {
      error = XcolorRegionDelete( display, w, 0, nRegions );
      fprintf(stderr, "deleted %lu region%c\n", nRegions, nRegions==1?' ':'s');
      XFree( r ); r = 0;
    } else
    {
      fprintf(stderr, "no region to delete \n");
      need_wait = 0;
    }
  }

  XFlush( display );

  /** Closing the display object will destroy all XFixes regions automatically
   *  by Xorg. Therefore we loop here to keep the XFixes regions alive. */
  if(need_wait)
    while(1) sleep(2);

  XCloseDisplay( display );
  return 0;
}
