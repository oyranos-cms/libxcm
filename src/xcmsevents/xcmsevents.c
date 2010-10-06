/** xcmevents.c
 *
 *  A small X11 colour management event observer.
 *
 *  @par License: 
 *             MIT <http://www.opensource.org/licenses/mit-license.php>
 *  @par Copyright:
 *             (c) 2009-2010 - Kai-Uwe Behrmann <ku.b@gmx.de>
 *
 */

#include <stdlib.h>
#include "XcmEvents.h"

int main(int argc, char *argv[])
{
  const char * display_name = getenv("DISPLAY");

  XcmeContext_s * c = XcmeContext_Create( display_name );

  for(;;)
  {
    XEvent event;
    XNextEvent( XcmeContext_DisplayGet( c ), &event);
    XcmeContext_InLoop( c, &event );
  }

  XcmeContext_Release( &c );

  return 0;
}

