#
# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2014 RDK Management
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
AC_INIT(rmfcore, 1.0, ,rmfcore)
AC_CONFIG_SRCDIR([halpod/halpod.cpp])
AM_CONFIG_HEADER(cfg/config.h)
AC_CONFIG_MACRO_DIR([cfg])
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

AC_ARG_ENABLE([podmgr],
        AS_HELP_STRING([--enable-podmgr],[enable podmgr support in qamsource stub]),
        [
          case "${enableval}" in
            yes) ENABLE_PODMGR=true;;
            no) ENABLE_PODMGR=false;;
            *) AC_MSG_ERROR([bad value ${enableval} for --enable-podmgr]) ;;
          esac
        ],
        [echo "Podmgr support is enabled in QamSource stub"])
AM_CONDITIONAL([ENABLE_PODMGR], [test x$ENABLE_PODMGR = xtrue])

AC_SUBST(ENABLE_PODMGR)

AC_CONFIG_FILES([Makefile
     sectionfilter/Makefile])
AC_OUTPUT
