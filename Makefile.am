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
	docs/X_Color_Management.txt \
	libXcm_control.in \
	examples/compiz/client.c \
	examples/compiz/color.c \
	examples/compiz/makefile \
	examples/edid-parse/edid_parse.c \
	examples/edid-parse/makefile \
	examples/net-color-region/netColorRegion2.c \
	examples/net-color-region/makefile

RPMARCH=`rpmbuild --showrc | awk '/^build arch/ {print $$4}'`

rpm:    dist-bzip2
	mkdir -p rpmdir/BUILD \
	rpmdir/SPECS \
	rpmdir/SOURCES \
	rpmdir/SRPMS \
	rpmdir/RPMS/$(RPMARCH)
	cp -f $(PACKAGE)-$(PACKAGE_VERSION).tar.bz2 rpmdir/SOURCES
	rpmbuild --clean -ba $(top_builddir)/$(PACKAGE).spec --define "_topdir $$PWD/rpmdir"
	@echo "============================================================"
	@echo "Finished - the packages are in rpmdir/RPMS and rpmdir/SRPMS!"

deb:    dist
	mkdir -p debian/deb/DEBIAN
	tar xvzf $(PACKAGE)-$(PACKAGE_VERSION).tar.gz
	(cd $(PACKAGE)-$(PACKAGE_VERSION); \
	./configure --prefix=/usr; \
	make DESTDIR=$(top_builddir)/debian/deb/DEBIAN install;)
	cp $(PACKAGE)-$(PACKAGE_VERSION)/$(PACKAGE)_control $(top_builddir)/debian/deb/DEBIAN/control
	$(RM) -R $(PACKAGE)-$(PACKAGE_VERSION)
	dpkg -b $(top_builddir)/debian/deb debian/$(PACKAGE)_$(PACKAGE_VERSION)-$(PACKAGE_RELEASE)_$(deb_arch).deb
	$(RM) -R $(top_builddir)/debian/deb
	echo ... $@ done

help:
	echo "... all :        build the binaries"
	echo "non default build targets:"
	echo "... check :      do a test build and run it"
	echo "... "
	echo "... install :    install the binaries, include files and data"
	echo "... install-exec: install only binaries"
	echo "... install-data: install only documentation"
	echo "... uninstall"
	echo "... clean :      remove binaries, object files and most processed data"
	echo "... distclean :  remove all files processed from configure scripts"
	echo "... rpm :        build RPM's"
	echo "... dist :       build a distribution tarball and place local"
	echo "  VARIABLES:"
	echo "... DESTDIR=\"/user/a/My Install Dir/\" install"
	echo "... BUILDDIR=../build_local"

pkgconfigdir = ${libdir}/pkgconfig
pkgconfig_DATA = xcm.pc
