# -*- Makefile -*-

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src

pkgincludedir = ${includedir}/X11/Xcm
pkginclude_HEADERS = \
	include/Xcm.h include/XcmDDC.h include/XcmEdidParse.h \
	include/XcmEvents.h include/XcmVersion.h

man_MANS = \
	doc/man/man3/Xcm.3 doc/man/man3/XcmDDC.3 doc/man/man3/XcmEdidParse.3 \
	doc/man/man3/XcmEvents.3 doc/man/man3/XcolorProfile.3 \
	doc/man/man3/XcolorRegion.3 libXcm.3

EXTRA_DIST = ${man_MANS} \
	Doxyfile.in \
	docs/X_Color_Management.txt

pkgconfigdir = ${libdir}/pkgconfig
pkgconfig_DATA = xcm.pc
