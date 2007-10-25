dnl configure.ac
dnl
dnl xfce4-mpc-plugin - mpd client plugin for the XFce4 panel
dnl

m4_define([mpc_version], [0.3.2])

AC_INIT([xfce4-mpc-plugin], [mpc_version],
	[landry@fr.homeunix.org])

MPC_VERSION=mpc_version()
AM_INIT_AUTOMAKE([xfce4-mpc-plugin], [$MPC_VERSION], [-Wall -Werror foreign])
AM_CONFIG_HEADER([config.h])

AM_MAINTAINER_MODE

dnl Check for UNIX variants
AC_AIX
AC_ISC_POSIX
AC_MINIX

dnl Check for basic programs
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_INTLTOOL

dnl Check for standard header files
AC_HEADER_STDC

dnl configure the panel plugin
XDT_CHECK_PACKAGE([LIBXFCE4PANEL], [libxfce4panel-1.0], [4.3.22])

dnl check for libmpd presence
AC_ARG_WITH(libmpd,
    AC_HELP_STRING([--with-libmpd=path], [path to libmpd base directory]),
        LIBMPD_PATH="$withval", LIBMPD_PATH="")

use_libmpd=false
AM_CONDITIONAL([HAVE_LIBMPD], [test "$use_libmpd" = true])
AC_DEFINE([HAVE_LIBMPD], [0], [no libmpd found])

AC_MSG_CHECKING([whether --with-libmpd was used])
if test x"$LIBMPD_PATH" != x"" ; then
   if test x"$LIBMPD_PATH" = x"no" ; then
      AC_MSG_RESULT([yes, disabled])
      echo "* Build with libmpd: no, falling back to our implementation"
   else
      AC_MSG_RESULT([yes, using $LIBMPD_PATH])
      LIBMPD_CFLAGS="-I${LIBMPD_PATH}/include/"
      LIBMPD_LIBS="-L${LIBMPD_PATH}/lib/ -lmpd"
      AC_DEFINE([HAVE_LIBMPD], [1], [libmpd found])
      use_libmpd=true
      AC_SUBST([LIBMPD_CFLAGS])
      AC_SUBST([LIBMPD_LIBS])
   fi
else
   AC_MSG_RESULT([no, falling back to pkg-config])
   XDT_CHECK_OPTIONAL_PACKAGE([LIBMPD], [libmpd], [0.12], [libmpd], [libmpd])
   if test x"$LIBMPD_FOUND" = x"yes"; then
      AC_DEFINE([HAVE_LIBMPD], [1], [libmpd found])
      use_libmpd=true
      echo "* Build with libmpd: yes"
   else
      echo "* Build with libmpd: no, falling back to our implementation"
   fi
fi
AM_CONDITIONAL(USE_LIBMPD, [test "$use_libmpd" = true])
dnl Translations
XDT_I18N([cs en_GB eu fr hu pl pt_PT sq])

dnl Check for debugging support
XDT_FEATURE_DEBUG()
AC_OUTPUT([
Makefile
panel-plugin/Makefile
po/Makefile.in
])

