
dnl                                               -*- Autoconf -*-
dnl Process this file with autoconf to produce a configure script.

dnl AC_PREREQ([2.69])
AC_INIT(anycas, 1.0, mnagendran@synamedia.com,anycas)

AC_CONFIG_SRCDIR([src/CASManagerImpl.cpp])
AC_CONFIG_SRCDIR([demoCas/demoCas.cpp])
LT_INIT

dnl *************************************************
dnl current:revision:age
dnl change (without API) = c:r+1:a
dnl change API = c+1:0:a
dnl add API = c+1:0:a+1
dnl remove API = c+1:0:0
dnl *************************************************
AC_CANONICAL_HOST
AM_INIT_AUTOMAKE([foreign no-dist-gzip dist-bzip2 1.9])
eval LIBTOOL_VERSION_INFO="2:0:0"
dnl *************************************************

dnl subdirectories.
AM_EXTRA_RECURSIVE_TARGETS([anycas])
dnl Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_CPP
AC_PROG_CXXCPP
AC_LIBTOOL_DLOPEN
AM_PROG_LIBTOOL

ANYCAS_CFLAGS=" "
ANYCAS_LIBS=" "

AC_SUBST(LIBTOOL_VERSION_INFO)

AC_ARG_ENABLE([anycas],
              AS_HELP_STRING([--enable-anycas],[enable anycas (default is no)]),
              [
      		case "${enableval}" in
                 yes) ANYCAS_CFLAGS="-DUSE_ANYCAS -DANYCAS_FACTORY"
		      ANYCAS_LIBS="-lanycas";;
        	 no)  AC_MSG_ERROR([anycas is disabled]) ;;
        	 *) AC_MSG_ERROR([bad value ${enableval} for --enable-anycas]) ;;
       		esac
    	      ],
    	      [echo "anycas is disabled"])

dnl use pretty build output with automake >= 1.11
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])],
  [AM_DEFAULT_VERBOSITY=1
   AC_SUBST(AM_DEFAULT_VERBOSITY)])


dnl **********************************
dnl checks for dependencies
dnl **********************************
AC_HEADER_STDC

dnl Checks for header files.
AC_CHECK_HEADERS([limits.h memory.h stdlib.h string.h sys/socket.h unistd.h])

dnl Checks for typedefs, structures, and compiler characteristics.
dnl AC_CHECK_HEADER_STDBOOL
AC_TYPE_UINT8_T

dnl Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset strdup strerror])

PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 0.10.28])
PKG_CHECK_MODULES([GTHREAD], [gthread-2.0 >= 0.10.28])
PKG_CHECK_MODULES([UUID],[uuid >= 1.41.12])
PKG_CHECK_MODULES([LOG4C],[log4c >= 1.2.3])

AC_SUBST(ANYCAS_CFLAGS)
AC_SUBST(ANYCAS_LIBS)
AC_CONFIG_FILES([Makefile test/Makefile])
AC_OUTPUT
