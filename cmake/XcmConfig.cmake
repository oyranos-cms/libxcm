# Redistribution and use is allowed according to the terms of the BSD license.
# Copyright (c) 2012-2017, Kai-Uwe Behrmann, <ku.b@gmx.de>

find_package(PkgConfig)
pkg_check_modules(XCM xcm)

FIND_PATH(XCM_INCLUDE_DIRS_FULL NAMES X11/Xcm/Xcm.h)
IF(XCM_INCLUDE_DIRS_FULL)
  STRING( REPLACE X11/Xcm/ "" XCM_INCLUDE_DIRS ${XCM_INCLUDE_DIRS_FULL} )
ENDIF(XCM_INCLUDE_DIRS_FULL)

IF(NOT XCM_LIBRARIES)
  FIND_LIBRARY(XCM_LIBRARY NAMES Xcm)
  IF(XCM_LIBRARY)
    SET(XCM_LIBRARIES ${XCM_LIBRARIES} ${XCM_LIBRARY})
  ENDIF(XCM_LIBRARY)
  FIND_LIBRARY(XCM_LIBRARY_DDC NAMES XcmDDC)
  IF(XCM_LIBRARY_DDC)
    SET(XCM_LIBRARIES ${XCM_LIBRARIES} ${XCM_LIBRARY_DDC})
  ENDIF(XCM_LIBRARY_DDC)
  FIND_LIBRARY(XCM_LIBRARY_EDID NAMES XcmEDID)
  IF(XCM_LIBRARY_EDID)
    SET(XCM_LIBRARIES ${XCM_LIBRARIES} ${XCM_LIBRARY_EDID})
  ENDIF(XCM_LIBRARY_EDID)
  FIND_LIBRARY(XCM_LIBRARY_X11 NAMES XcmX11)
  IF(XCM_LIBRARY_X11)
    SET(XCM_LIBRARIES ${XCM_LIBRARIES} ${XCM_LIBRARY_X11})
  ENDIF(XCM_LIBRARY_X11)
ENDIF(NOT XCM_LIBRARIES)
FIND_LIBRARY(XCM_LIBRARY_DIR NAMES Xcm XcmEDID)
IF(NOT XCM_LIBRARY_DIR AND XCM_LIBRARY_DIRS)
  SET( XCM_LIBRARY_DIR ${XCM_LIBRARY_DIRS} )
ENDIF()

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Xcm
	REQUIRED_VARS XCM_LIBRARY_DIR XCM_INCLUDE_DIRS XCM_LIBRARIES
	)


if (XCM_FOUND)
    set(HAVE_XCM TRUE)
    if (NOT Xcm_FIND_QUIETLY)
        message(STATUS "Found XCM: ${XCM_LIBRARY_DIRS} ${XCM_INCLUDE_DIRS} ${XCM_LDFLAGS}")
    endif (NOT Xcm_FIND_QUIETLY)
else (XCM_FOUND)
    if (NOT Xcm_FIND_QUIETLY)
        message(STATUS "Xcm was NOT found.")
    endif (NOT Xcm_FIND_QUIETLY)
    if (Xcm_FIND_REQUIRED)
        message(FATAL_ERROR "Could NOT find Xcm")
    endif (Xcm_FIND_REQUIRED)
endif (XCM_FOUND)
