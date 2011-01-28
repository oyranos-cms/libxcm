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
int main(int argc, char ** argv)
{
  Display * display = XOpenDisplay(NULL);

    Window w;

      XserverRegion reg = 0;
      XcolorRegion region;
      int error;
      XRectangle rec[2] = { { 0,0,0,0 }, { 0,0,0,0 } };

  if(argc != 6)
  {
    printf("Usage:\n\t%s decimal_xwinid x y width height\n", argv[0] );
    return 1;
  }

  sscanf( argv[1], "%i", (int*)&w );
  w = (Window) atoi(argv[1]);

      rec[0].x = atoi(argv[2]);
      rec[0].y = atoi(argv[3]);
      rec[0].width = atoi(argv[4]);
      rec[0].height = atoi(argv[5]);

      reg = XFixesCreateRegion( display, rec, 1);

      region.region = htonl(reg);
      memset( region.md5, 0, 16 );

      error = XcolorRegionInsert( display, w, 0, &region, 1 );

  XFlush( display );

  /** Closing the display object will destroy all XFixes regions automatically
   *  by Xorg. Therefore we loop here to keep the XFixes regions alive. */
  while(1) sleep(1);

  XCloseDisplay( display );
  return 0;
}
