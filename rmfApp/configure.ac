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
##########################################################################
dnl                                               -*- Autoconf -*-
dnl Process this file with autoconf to produce a configure script.

dnl AC_PREREQ([2.69])
AC_INIT(rmfapp, 1.0, ,rmfapp)
AC_CONFIG_SRCDIR([help.cpp])
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
HEADLESS=" "
QAM_CFLAGS=" "
CC_DEFINES=" "
QAM_LIBS=" "
CC_LIBS=" "
TRANSCODER_LIB=" "
RMFPROXYSVC_LIB=" "
DUMPFILESINK_LIB=" "
HNSINK_LIB=" "
DVR_LIBS=" "
DVR_CFLAGS=" "
DVRMGR_ENABLE=" "
CC_ENABLE=" "
DTV_CFLAGS=" "
DTV_LIBS=" "

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

AC_ARG_ENABLE([emulator-qam],
        AS_HELP_STRING([--enable-emulator-qam],[enable emulator-qam build plugin (default is no)]),
        [
          case "${enableval}" in
           yes) QAM_CFLAGS="-DUSE_QAMSRC -DQAMSRC_FACTORY"
		QAM_LIBS="-lqamsrc -loobsimgrstub -loobsicache -loobsimgrstub -loobsimanager -linbsimanager -lsectionfilter -linbsectionfilter -loobsectionfilter -ltinyxml -lxml2" ;;
           no) QAM_CFLAGS="" ;;
          *) AC_MSG_ERROR([bad value ${enableval} for --enable-emulator-qam ]) ;;
           esac
           ],
         [echo "emulator-qam is disabled"])
         
AC_ARG_ENABLE([ccaption],
              AS_HELP_STRING([--enable-ccaption],[enable ccaption (default is no)]),
              [
      		case "${enableval}" in
                 yes) CC_ENABLE=true CC_DEFINES="-DENABLE_CLOSEDCAPTION"
		      CC_LIBS="-lrdkCCReader -lrdkCCManager -lrdkCCGfx";;
        	 no)  AC_MSG_ERROR([Ccaption is disabled]) ;;
        	 *) AC_MSG_ERROR([bad value ${enableval} for --enable-cc]) ;;
       		esac
    	      ],
    	      [echo "closedcaption is disabled"])
AM_CONDITIONAL([CC_ENABLE], [test x$CC_ENABLE = xtrue])

AC_ARG_ENABLE([qam],
              AS_HELP_STRING([--enable-qam],[enable qam (default is no)]),
              [
      		case "${enableval}" in
                 yes) QAM_CFLAGS="-DUSE_QAMSRC -DQAMSRC_FACTORY"
		      QAM_LIBS="-lqamsrc -lnetsnmp -lnetsnmpagent -lnetsnmphelpers -lsectionfilter -linbsectionfilter -linbsimanager";;
        	 no)  AC_MSG_ERROR([qam is disabled]) ;;
        	 *) AC_MSG_ERROR([bad value ${enableval} for --enable-qam]) ;;
       		esac
    	      ],
    	      [echo "qam is disabled"])

AC_ARG_ENABLE([transcoderfilter],
              AS_HELP_STRING([--enable-transcoderfilter],[enable transcoderfilter plugin (default is no)]),
              [
                case "${enableval}" in
                 yes) TRANSCODER_LIB=" -ltranscoderfilter";;
                 no)  AC_MSG_ERROR([transcoderfilter plugin is disabled]) ;;
                 *) AC_MSG_ERROR([bad value ${enableval} for --enable-transcoderfilter]) ;;
                esac
              ],
              [echo "transcoderfilter plugin is disabled"])

AC_ARG_ENABLE([rmfproxyservice],
              AS_HELP_STRING([--enable-rmfproxyservice],[enable rmfproxyservice plugin (default is no)]),
              [
                case "${enableval}" in
                 yes) RMFPROXYSVC_LIB=" -lrmfproxyservice" ;;
                 no)  AC_MSG_ERROR([rmfproxyservice plugin is disabled]) ;;
                 *) AC_MSG_ERROR([bad value ${enableval} for --enable-rmfproxyservice]) ;;
                esac
              ],
              [echo "rmfproxyservice plugin is disabled"])

AC_ARG_ENABLE([dumpfilesink],
              AS_HELP_STRING([--enable-dumpfilesink],[enable dumpfilesink plugin (default is no)]),
              [
                case "${enableval}" in
                 yes) DUMPFILESINK_LIB="-ldumpfilesink";;
                 no)  AC_MSG_ERROR([dumpfilesink plugin is disabled]) ;;
                 *) AC_MSG_ERROR([bad value ${enableval} for --enable-dumpfilesink]) ;;
                esac
              ],
              [echo "dumpfilesink is disabled"])

AC_ARG_ENABLE([hnsink],
              AS_HELP_STRING([--enable-hnsink],[enable hnsink plugin (default is no)]),
              [
                case "${enableval}" in
                 yes) HNSINK_LIB=" -lhnsink";;
                 no)  AC_MSG_ERROR([hnsink plugin is disabled]) ;;
                 *) AC_MSG_ERROR([bad value ${enableval} for --enable-hnsink]) ;;
                esac
              ],
              [echo "hnsink plugin is disabled"])

AC_ARG_ENABLE([dvr],
              AS_HELP_STRING([--enable-dvr],[enable dvr plugin (default is no)]),
              [
                case "${enableval}" in
                 yes) DVRMGR_ENABLE=true DVR_LIBS="-ldvrmgr -ldvrsink  -ldvrsource -lTasseograph -ldvrmgrsoc"
		      DVR_CFLAGS="-DUSE_DVR" ;;
                 no)  DVRMGR_ENABLE=false AC_MSG_ERROR([dvr is disabled]) ;;
                 *) AC_MSG_ERROR([bad value ${enableval} for --enable-dvr]) ;;
                esac
              ],
              [echo "dvr is disabled"])
AM_CONDITIONAL([DVRMGR_ENABLE], [test x$DVRMGR_ENABLE = xtrue])

AC_ARG_ENABLE([dtv],
              AS_HELP_STRING([--enable-dtv],[enable dtvsource plugin (default is no)]),
              [
                case "${enableval}" in
                  yes) DTV_CFLAGS="-DUSE_DTVSRC"
                DTV_LIBS="-ldtvsource";; 
                  no)  AC_MSG_ERROR([dtvsource plugin is disabled]) ;;
                 *) AC_MSG_ERROR([bad value ${enableval} for --enable-dtv]) ;;
                esac
              ],
              [echo "dtvsource plugin is disabled"])

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

dnl GStreamer version. Use 0.10 by default
AC_ARG_ENABLE(gstreamer1,
              AS_HELP_STRING([--enable-gstreamer1],
                             [Use GStreamer 1.x]), ,
              [enable_gstreamer1=auto])

AC_MSG_NOTICE([Look for Gstreamer-1.4])
PKG_CHECK_MODULES([GST], [gstreamer-1.0 >= 1.4], have_gst1="yes", have_gst1="no")
AS_IF([test "x$have_gst1" = "xyes"], [
    GST_MAJORMINOR=1.0
    AC_DEFINE(USE_GST1, 1, [Build with GStreamer 1.x])
    ], [])

AS_IF([test "x$have_gst1" != "xyes"], [
  AC_MSG_NOTICE([GStreamer-1.4 not found; Looking for GStreamer-0.10 as alternate])
  GST_MAJORMINOR=0.10
  PKG_CHECK_MODULES([GST], [gstreamer-0.10 >= 0.10.28])
], [])


PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 0.10.28])
PKG_CHECK_MODULES([GTHREAD], [gthread-2.0 >= 0.10.28])
PKG_CHECK_MODULES([UUID],[uuid >= 1.41.12])
PKG_CHECK_MODULES([LOG4C],[log4c >= 1.2.3])

AC_SUBST(HEADLESS)
AC_SUBST(QAM_CFLAGS)
AC_SUBST(QAM_LIBS)
AC_SUBST(CC_DEFINES)
AC_SUBST(CC_LIBS)
AC_SUBST(TRANSCODER_LIB)
AC_SUBST(RMFPROXYSVC_LIB)
AC_SUBST(DUMPFILESINK_LIB)
AC_SUBST(HNSINK_LIB)
AC_SUBST(DVR_LIBS)
AC_SUBST(DVR_CFLAGS)
AC_SUBST(DVRMGR_ENABLE)
AC_SUBST(DTV_CFLAGS)
AC_SUBST(DTV_LIBS)
AC_CONFIG_FILES([Makefile
         cfg/Makefile])
AC_OUTPUT
