AC_INIT([libXcm], [0.4.2])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall foreign subdir-objects])
AC_PROG_CC
AM_PROG_CC_C_O
AC_DISABLE_STATIC
AM_PROG_LIBTOOL

PKG_CHECK_MODULES([libX11], [x11])
PKG_CHECK_MODULES([libXfixes], [xfixes])
PKG_CHECK_MODULES([libXmu], [xmu])

AC_CONFIG_FILES([Makefile src/Makefile xcm.pc])
AC_OUTPUT
