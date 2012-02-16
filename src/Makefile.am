# -*- Makefile -*-

AM_CPPFLAGS = -I${top_srcdir}/include ${libX11_CFLAGS} ${libXfixes_CFLAGS} ${libXmu_CFLAGS}
AM_CFLAGS   = -Wall

lib_LTLIBRARIES = libXcm.la

libXcm_la_SOURCES = XcmEdidParse.c
if HAVE_X11
libXcm_la_SOURCES += Xcm.c XcmDDC.c XcmEvents.c
endif
libXcm_la_LIBADD  = -lm ${libX11_LIBS} ${libXfixes_LIBS} ${libXmu_LIBS}
# NOT supposed to be the same as ${PACKAGE_VERSION}
libXcm_la_LDFLAGS = -version-info ${LIBTOOL_VERSION}
