define([MAJOR],[0])
define([MINOR],[5])
define([MICRO],[5])
AC_INIT([libXcm],[MAJOR.MINOR.MICRO],[https://gitlab.com/oyranos/libxcm/issues],[libXcm],[http://www.oyranos.org])
LIBTOOL_CURRENT=0
LIBTOOL_REVISION=5
LIBTOOL_AGE=0

AC_CONFIG_AUX_DIR([build-aux])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall foreign subdir-objects])
AC_PROG_CC
AM_PROG_CC_C_O
AC_DISABLE_STATIC
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
LT_INIT
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES])

AC_SUBST(LIBTOOL_VERSION)
AC_SUBST(XCM_PACKAGE_MAJOR)
AC_SUBST(XCM_PACKAGE_MINOR)
AC_SUBST(XCM_PACKAGE_MICRO)
AC_SUBST(PACKAGE_VERSION)
AC_SUBST(PACKAGE_RELEASE)
AC_SUBST(HAVE_X11)
AC_SUBST(PKG_CONFIG_LIBS_X11)
AC_SUBST(PKG_CONFIG_LIBS_DDC)
AC_SUBST(PKG_CONFIG_PRIVATE_X11)
AC_SUBST(PKG_CONFIG_PRIVATE_X11_PKG)
AC_SUBST(PKG_CONFIG_PRIVATE_DDC_PKG)
AC_SUBST(HAVE_LINUX)

XCM_PACKAGE_MAJOR=MAJOR
XCM_PACKAGE_MINOR=MINOR
XCM_PACKAGE_MICRO=MICRO
PACKAGE_RELEASE=0

LIBTOOL_VERSION=$LIBTOOL_CURRENT:$LIBTOOL_REVISION:$LIBTOOL_AGE

AC_MSG_CHECKING([wether to enable x11 support])
AC_ARG_WITH(x11,
    [  --with-x11           enable x11 support],
    with_x11=$withval,
    with_x11=yes)
AC_MSG_RESULT($with_x11)

if test "$with_x11" = "yes"; then
PKG_CHECK_EXISTS([x11], [
	PKG_CHECK_MODULES([libX11], [x11])
	PKG_CHECK_MODULES([libXfixes], [xfixes])
	AM_CONDITIONAL([HAVE_X11], [true])
        HAVE_X11="#define XCM_HAVE_X11 1"
        PKG_CONFIG_PRIVATE_X11="xproto x11"
        PKG_CONFIG_LIBS_X11="-lXcmX11"
        PKG_CONFIG_PRIVATE_X11_PKG="xcm-x11"
], [
	AC_MSG_CHECKING([for libX11])
	AC_MSG_RESULT([no])
	AM_CONDITIONAL([HAVE_X11], [false])
        HAVE_X11=
])
else
	AC_MSG_CHECKING([for libX11])
	AC_MSG_RESULT([no])
	AM_CONDITIONAL([HAVE_X11], [false])
	HAVE_X11=
fi

AC_PATH_PROGS(RPMBUILD, rpm, :)

LINUX="`uname | grep Linux | wc -l`"
if [[ $LINUX = "1" ]]; then
	HAVE_LINUX="#define XCM_HAVE_LINUX 1"
	AM_CONDITIONAL([HAVE_LINUX], [true])
	PKG_CONFIG_PRIVATE_DDC_PKG="xcm-ddc"
	PKG_CONFIG_LIBS_DDC="-lXcmDDC"
else
	HAVE_LINUX=
	AM_CONDITIONAL([HAVE_LINUX], [false])
fi

AC_CONFIG_FILES([Makefile src/Makefile extras/xcm.pc extras/xcm-edid.pc extras/xcm-ddc.pc extras/xcm-x11.pc doc/Doxyfile extras/libXcm.spec
include/X11/Xcm/XcmVersion.h])
AC_OUTPUT

if [[ "$HAVE_X11" = "" ]]; then
echo "HAVE_X11        =       no, X Color Management support skipped"
else
echo "HAVE_X11        =       yes (X Color Management)"
fi
if [[ "$HAVE_LINUX" = "" ]]; then
echo "HAVE_LINUX      =       no, DDC over i2c support skipped"
else
echo "HAVE_LINUX      =       yes (DDC over i2c)"
fi
echo "CFLAGS          =       $CFLAGS"
echo "CXXFLAGS        =       $CXXFLAGS"
echo "LDFLAGS         =       $LDFLAGS"
echo "prefix          =       $prefix"
echo "exec_prefix     =       $exec_prefix"
echo "bindir          =       $bindir"
echo "sbindir         =       $sbindir"
echo "libdir          =       $libdir"
echo "includedir      =       $includedir"
echo "datadir         =       $datadir"
echo "mandir          =       $mandir"
echo "pixmapdir       =       $pixmapdir"
echo "desktopdir      =       $desktopdir"
