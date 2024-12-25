dnl configure.ac
dnl
dnl xfce4-mpc-plugin - mpd client plugin for the XFce4 panel
dnl

XDT_VERSION_INIT([0.5.4], [git])

AC_COPYRIGHT([Copyright (c) 2006-2024 The Xfce development team. All rights reserved.])
AC_INIT([xfce4-mpc-plugin], [xdt_version], [https://gitlab.xfce.org/panel-plugins/xfce4-mpc-plugin])
AC_REVISION([xdt_version_build])
AC_PREREQ([2.69])

AM_INIT_AUTOMAKE([1.8 dist-bzip2 tar-ustar no-dist-gzip foreign])
AC_CONFIG_HEADERS([config.h])
AM_MAINTAINER_MODE
AC_CONFIG_MACRO_DIRS([m4])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

dnl Check for basic programs
AC_PROG_CC
AC_PROG_INSTALL

dnl Initialize libtool
LT_PREREQ([2.2.6])
LT_INIT([disable-static])

dnl configure the panel plugin
XDT_CHECK_PACKAGE([GLIB], [glib-2.0], [2.60.0])
XDT_CHECK_PACKAGE([GTK], [gtk+-3.0], [3.22.0])
XDT_CHECK_PACKAGE([LIBXFCE4PANEL], [libxfce4panel-2.0], [4.16.0])
XDT_CHECK_PACKAGE([LIBXFCE4UI], [libxfce4ui-2], [4.16.0])

dnl check for libmpd presence
AC_ARG_WITH(libmpd,
    AS_HELP_STRING([--with-libmpd=path], [path to libmpd base directory]),
        LIBMPD_PATH="$withval", LIBMPD_PATH="")

use_libmpd=false

AC_MSG_CHECKING([whether --with-libmpd was used])
if test x"$LIBMPD_PATH" != x"" ; then
   if test x"$LIBMPD_PATH" = x"no" ; then
      AC_MSG_RESULT([yes, disabled])
      echo "* Build with libmpd: no, falling back to our implementation"
   else
      AC_MSG_RESULT([yes, using $LIBMPD_PATH])
      LIBMPD_CFLAGS="-I${LIBMPD_PATH}/include/"
      LIBMPD_LIBS="-L${LIBMPD_PATH}/lib/ -lmpd"
      use_libmpd=true
      AC_SUBST([LIBMPD_CFLAGS])
      AC_SUBST([LIBMPD_LIBS])
   fi
else
   AC_MSG_RESULT([no, falling back to pkg-config])
   XDT_CHECK_OPTIONAL_PACKAGE([LIBMPD], [libmpd], [0.12], [libmpd], [libmpd])
   if test x"$LIBMPD_FOUND" = x"yes"; then
      use_libmpd=true
      echo "* Build with libmpd: yes"
   else
      echo "* Build with libmpd: no, falling back to our implementation"
   fi
fi
AM_CONDITIONAL(USE_LIBMPD, [test "$use_libmpd" = true])
AS_IF([test "$use_libmpd" = true],
  AC_DEFINE([HAVE_LIBMPD], [1], [Define to 1 if libmpd is found]))
dnl Translations
GETTEXT_PACKAGE="$PACKAGE"
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Name of default gettext domain])
AC_SUBST([GETTEXT_PACKAGE])
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.19.8])

dnl Check for debugging support
XDT_FEATURE_DEBUG([xdt_debug_default])
AC_CONFIG_FILES([
Makefile
panel-plugin/Makefile
po/Makefile.in
])
AC_OUTPUT
