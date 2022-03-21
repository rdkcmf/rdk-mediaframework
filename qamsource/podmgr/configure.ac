#
# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2018 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
dnl                                               -*- Autoconf -*-
dnl Process this file with autoconf to produce a configure script.

dnl AC_PREREQ([2.69])
AC_INIT(rmfpodmgr, 1.0, ,rmfcore)
AC_CONFIG_SRCDIR([canh/canhClient.h])
AM_CONFIG_HEADER(cfg/config.h)
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([cfg])
AM_INIT_AUTOMAKE([foreign no-dist-gzip dist-bzip2 1.9])
LT_INIT

dnl subdirectories.

dnl Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_CPP
AC_PROG_CXXCPP

HEADLESS=" "
SKIP_PODSERVER=" "
ENABLE_YOCTO=" "
AC_ARG_ENABLE([headless],
        AS_HELP_STRING([--enable-headless],[enable headless build]),
        [
          case "${enableval}" in
           yes) HEADLESS=true ;;
           no) HEADLESS=false ;;
          *) AC_MSG_ERROR([bad value ${enableval} for --enable-headless ]) ;;
           esac
           ],
         [echo "headless build is enable"])
AM_CONDITIONAL([HEADLESS], [test x$HEADLESS = xtrue])
AM_CONDITIONAL([SKIP_PODSERVER], [true = true])

dnl use pretty build output with automake >= 1.11
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])],
  [AM_DEFAULT_VERBOSITY=1
   AC_SUBST(AM_DEFAULT_VERBOSITY)])




dnl **********************************
dnl checks for dependencies
dnl **********************************
AC_HEADER_STDC

dnl Checks for header files.

dnl Checks for typedefs, structures, and compiler characteristics.
dnl AC_CHECK_HEADER_STDBOOL
AC_TYPE_UINT8_T

AM_CONDITIONAL(BUILD_SUBDIR_PODSERVER, [test -d podserver])

m4_syscmd([test -d podserver])
m4_if(m4_sysval,[0],[AC_CONFIG_FILES([runpod/Makefile])])
m4_if(m4_sysval,[0],[AC_CONFIG_FILES([podserver/Makefile])])
m4_if(m4_sysval,[0],[SUBDIR_PODSERVER=podserver])

AC_ARG_ENABLE([yocto],
        AS_HELP_STRING([--enable-yocto],[enable yocto build]),
        [
          case "${enableval}" in
           yes) ENABLE_YOCTO=true SUBDIR_PODSERVER= ;;
           no) AC_MSG_NOTICE([yocto build is disabled]);;
          *) AC_MSG_NOTICE([bad value ${enableval} for --enable-headless ]) ;;
           esac
           ],
         [echo "Yocto build is disabled"])
AM_CONDITIONAL([ENABLE_YOCTO], [test x$ENABLE_YOCTO = xtrue])

AC_ARG_ENABLE([stubcode],
        AS_HELP_STRING([--enable-stubcode],[enable building stub code]),
        [
          case "${enableval}" in
           yes) PODMGR_STUB_CODE=true ;;
           no) PODMGR_STUB_CODE=false ;;
          *) AC_MSG_ERROR([bad value ${enableval} for --enable-stubcode ]) ;;
           esac
           ],
         [echo "Stub code build is enable"])
AM_CONDITIONAL([PODMGR_STUB_CODE], [test x$PODMGR_STUB_CODE = xtrue])

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([podclient/Makefile])
AC_CONFIG_FILES([canh/Makefile])
AC_SUBST(SUBDIR_PODSERVER)
AC_SUBST(BUILD_SUBDIR_PODSERVER)
AC_OUTPUT

