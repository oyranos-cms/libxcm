#include "XcmEdidParse.h"
#include "XcmDDC.h"
#include "Xcm.h"

void XcmDummy_()
{
  XcmEdidErrorToString(XCM_EDID_OK);
  XcmDDCErrorToString(XCM_DDC_OK);
  XcmColorServerCapabilities( NULL );
}
