#include "XcmEdidParse.h"
#include "XcmDDC.h"
#include "Xcm.h"

void XcmDummy_()
{
  XcmEdidErrorToString(XCM_EDID_OK);
#if defined(HAVE_LINUX)
  XcmDDCErrorToString(XCM_DDC_OK);
#endif
#if defined(HAVE_X11)
  XcmColorServerCapabilities( NULL );
#endif
}
