# -*- Makefile -*-

AM_CPPFLAGS = -I${top_srcdir}/include/X11/Xcm -I${top_builddir}/include/X11/Xcm ${libX11_CFLAGS} ${libXfixes_CFLAGS} ${libXmu_CFLAGS}
AM_CFLAGS   = -Wall

lib_LTLIBRARIES = libXcmEDID.la libXcmDDC.la libXcmX11.la libXcm.la

libXcmEDID_la_SOURCES = XcmEdidParse.c
EXTRA_SOURCES =
if HAVE_LINUX
libXcmDDC_la_SOURCES = XcmDDC.c
else
EXTRA_SOURCES += XcmDDC.c
endif
if HAVE_X11
libXcmX11_la_SOURCES = Xcm.c XcmEvents.c
else
EXTRA_SOURCES += Xcm.c XcmEvents.c
endif
libXcm_la_SOURCES = XcmDummy.c XcmInternal.h

libXcmX11_la_LIBADD  = \
			libXcmEDID.la
libXcm_la_LIBADD  = \
			libXcmX11.la \
			libXcmEDID.la \
			libXcmDDC.la
# NOT supposed to be the same as ${PACKAGE_VERSION}
libXcmX11_la_LDFLAGS = -lm ${libX11_LIBS} ${libXfixes_LIBS} ${libXmu_LIBS} -version-info ${LIBTOOL_VERSION}
libXcmEDID_la_LDFLAGS = -version-info ${LIBTOOL_VERSION}
libXcmDDC_la_LDFLAGS = -version-info ${LIBTOOL_VERSION}
libXcm_la_LDFLAGS = -L. -version-info ${LIBTOOL_VERSION}
