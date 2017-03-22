# Redistribution and use is allowed according to the terms of the BSD license.
# Copyright (c) 2012-2014, Kai-Uwe Behrmann, <ku.b@gmx.de>

find_package(PkgConfig)
pkg_check_modules(XCM xcm)

FIND_PATH(XCM_INCLUDE_DIRS NAMES Xcm.h
	ONLY_CMAKE_FIND_ROOT_PATH
	)

FIND_LIBRARY(XCM_LIBRARY_DIR NAMES Xcm XcmEdid
	ONLY_CMAKE_FIND_ROOT_PATH
	)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(XCM
	REQUIRED_VARS XCM_LIBRARY_DIR XCM_INCLUDE_DIRS
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
