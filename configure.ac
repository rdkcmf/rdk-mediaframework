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
AC_CONFIG_SRCDIR([core/rmfbase.cpp])
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

PLUGIN_SRC=" "
OSAL_SRC=" "
DVRMGR_FLAGS=" "
HEADLESS=" "

ENABLE_SDV=" "
ENABLE_DVB=" "
DVRMGR_ENABLE=" "
CC_ENABLE=" "
RBI_ENABLE=" "
RBI_ENABLE_ADCACHE=" "
ENABLE_QAM=" "
ENABLE_EXTERNAL_CAS=" "
ENABLE_IPPVRC=" "
OSAL_ENABLE=" "
ENABLE_YOCTO=" "
YOCTO_FLAG=" "
BUILD_PODMGR=" "

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

dnl GStreamer version. Use 0.10 by default
AC_ARG_ENABLE(gstreamer1,
              AS_HELP_STRING([--enable-gstreamer1],
                             [Use GStreamer 1.x]), ,
              [enable_gstreamer1=auto])

AC_MSG_NOTICE([Look for Gstreamer-1.4])
PKG_CHECK_MODULES([GST], [gstreamer-1.0 >= 1.4, gstreamer-base-1.0 >= 1.4], have_gst1="yes", have_gst1="no")
AS_IF([test "x$have_gst1" = "xyes"], [
    GST_MAJORMINOR=1.0
    AC_DEFINE(USE_GST1, 1, [Build with GStreamer 1.x])
    ], [])

AS_IF([test "x$have_gst1" != "xyes"], [
  AC_MSG_NOTICE([GStreamer-1.4 not found; Looking for GStreamer-0.10 as alternate])
  GST_MAJORMINOR=0.10
  PKG_CHECK_MODULES([GST], [gstreamer-0.10 >= 0.10.28, gstreamer-base-0.10 >= 0.10.28])
], [])


AC_ARG_ENABLE([osal],
              AS_HELP_STRING([--enable-osal],[enable osal (default is no)]),
              [
      		case "${enableval}" in
        	 yes) OSAL_ENABLE=true OSAL_SRC="osal";;
        	 no)  AC_MSG_ERROR([osal is disabled]) ;;
        	 *) AC_MSG_ERROR([bad value ${enableval} for --enable-osal]) ;;
       		esac
    	      ],
    	      [echo "osal is disabled"])
AM_CONDITIONAL(OSAL_ENABLE, [[test x$OSAL_ENABLE = xtrue]])
AM_CONDITIONAL(OSAL_STANDALONE, [[test x$OSAL_ENABLE != xtrue]])

AC_ARG_ENABLE([mediaplayersink],
              AS_HELP_STRING([--enable-mediaplayersink],[enable mediaplayersink plugin (default is no)]),
              [
      		case "${enableval}" in
        	 yes) PLUGIN_SRC="$PLUGIN_SRC mediaplayersink";;
        	 no)  AC_MSG_ERROR([mediaplayersink plugin is disabled]) ;;
        	 *) AC_MSG_ERROR([bad value ${enableval} for --enable-dtcpdec]) ;;
       		esac
    	      ],
    	      [echo "mediaplayersink plugin is disabled"])
AC_ARG_ENABLE([rmfproxyservice],
              AS_HELP_STRING([--enable-rmfproxyservice],[enable rmfproxyservice plugin (default is no)]),
              [
      		case "${enableval}" in
        	 yes) PLUGIN_SRC="$PLUGIN_SRC rmfproxyservice" ;;
        	 no)  AC_MSG_ERROR([rmfproxyservice plugin is disabled]) ;;
        	 *) AC_MSG_ERROR([bad value ${enableval} for --enable-rmfproxyservice]) ;;
       		esac
    	      ],
    	      [echo "rmfproxyservice plugin is disabled"])
AC_ARG_ENABLE([transcoderfilter],
              AS_HELP_STRING([--enable-transcoderfilter],[enable transcoderfilter plugin (default is no)]),
              [
      		case "${enableval}" in
        	 yes) PLUGIN_SRC="$PLUGIN_SRC transcoderfilter";;
        	 no)  AC_MSG_ERROR([transcoderfilter plugin is disabled]) ;;
        	 *) AC_MSG_ERROR([bad value ${enableval} for --enable-transcoderfilter]) ;;
       		esac
    	      ],
    	      [echo "transcoderfilter plugin is disabled"])
AC_ARG_ENABLE([hnsink],
              AS_HELP_STRING([--enable-hnsink],[enable hnsink plugin (default is no)]),
              [
      		case "${enableval}" in
        	 yes) PLUGIN_SRC="$PLUGIN_SRC hnsink";;
        	 no)  AC_MSG_ERROR([hnsink plugin is disabled]) ;;
        	 *) AC_MSG_ERROR([bad value ${enableval} for --enable-hnsink]) ;;
       		esac
    	      ],
    	      [echo "hnsink plugin is disabled"])
AC_ARG_ENABLE([ippvsource],
              AS_HELP_STRING([--enable-ippvsource],[enable ippvsource plugin (default is no)]),
              [
      		case "${enableval}" in
        	 yes) ENABLE_IPPVSRC=true PLUGIN_SRC="$PLUGIN_SRC ippvsource"
                      DVRMGR_FLAGS="$DVRMGR_FLAGS -DIPPV_CLIENT_ENABLED";;
        	 no)  AC_MSG_ERROR([ippvsource plugin is disabled]) ;;
        	 *) AC_MSG_ERROR([bad value ${enableval} for --enable-ippvsource]) ;;
       		esac
    	      ],
    	      [echo "ippvsource plugin is disabled"])
AM_CONDITIONAL([ENABLE_IPPVSRC],[test x$ENABLE_IPPVSRC = xtrue])
AC_ARG_ENABLE([vodsource],
              AS_HELP_STRING([--enable-vodsource],[enable vodsource plugin (default is no)]),
              [
      		case "${enableval}" in
        	 yes) PLUGIN_SRC="$PLUGIN_SRC vodsource";;
        	 no)  AC_MSG_ERROR([vodsource plugin is disabled]) ;;
        	 *) AC_MSG_ERROR([bad value ${enableval} for --enable-vodsource]) ;;
       		esac
    	      ],
    	      [echo "vodsource plugin is disabled"])
AC_ARG_ENABLE([sdv],
              AS_HELP_STRING([--enable-sdv],[enable sdv (default is no)]),
              [
                case "${enableval}" in
                 yes) ENABLE_SDV=true PKG_CHECK_MODULES([CURL],[libcurl >= 7.19.6]);;
                 no)  AC_MSG_ERROR([sdv is disabled])
                      ENABLE_SDV=false;;
                 *) AC_MSG_ERROR([bad value ${enableval} for --enable-sdv]) ;;
                esac
              ],
              [echo "sdv is disabled"])
AM_CONDITIONAL([ENABLE_SDV], [test x$ENABLE_SDV = xtrue])
AC_ARG_ENABLE([dvb],
              AS_HELP_STRING([--enable-dvb],[enable dvb (default is no)]),
              [
                case "${enableval}" in
                 yes) ENABLE_DVB=true;;
                 no)  AC_MSG_ERROR([dvb is disabled])
                      ENABLE_DVB=false;;
                 *) AC_MSG_ERROR([bad value ${enableval} for --enable-dvb]) ;;
                esac
              ],
              [echo "dvb is disabled"])
AM_CONDITIONAL([ENABLE_DVB], [test x$ENABLE_DVB = xtrue])
AC_ARG_ENABLE([trh],
              AS_HELP_STRING([--enable-trh],[enable trh plugin (default is no)]),
              [
      		case "${enableval}" in
        	 yes) PLUGIN_SRC="$PLUGIN_SRC trh"
		      enable_trh=true
		      ;;
        	 no)  AC_MSG_ERROR([trh plugin is disabled])
		      enable_trh=false
		      ;;
        	 *) AC_MSG_ERROR([bad value ${enableval} for --enable-trh]) ;;
       		esac
    	      ],
    	      [echo "trh plugin is disabled"])
AC_ARG_ENABLE([qam],
              AS_HELP_STRING([--enable-qam],[enable qamsrc plugin (default is no)]),
              [
      		case "${enableval}" in
        	 yes) ENABLE_QAM=true PLUGIN_SRC="$PLUGIN_SRC qamsource"
                      DVRMGR_FLAGS="$DVRMGR_FLAGS -DUSE_QAMSRC";;
        	 no)  AC_MSG_ERROR([qamsrc plugin is disabled]) ;;
        	 *) AC_MSG_ERROR([bad value ${enableval} for --enable-qam]) ;;
       		esac
    	      ],
    	      [echo "qam is disabled"])
AM_CONDITIONAL([ENABLE_QAM],[test x$ENABLE_QAM = xtrue])
AC_ARG_ENABLE([external-cas],
              AS_HELP_STRING([--enable-external-cas],[enable external cas plugin (default is no)]),
              [
          case "${enableval}" in
           yes) ENABLE_EXTERNAL_CAS=true PLUGIN_SRC="$PLUGIN_SRC anycas";;
           no)  AC_MSG_ERROR([external cas plugin is disabled]) ;;
           *) AC_MSG_ERROR([bad value ${enableval} for --enable-external-cas]) ;;
          esac
            ],
            [echo "external cas is disabled"])
AM_CONDITIONAL([ENABLE_EXTERNAL_CAS],[test x$ENABLE_EXTERNAL_CAS = xtrue])
AC_ARG_ENABLE([emulator-qam],
              AS_HELP_STRING([--enable-emulator-qam],[enable emulator qamsrc plugin (default is no)]),
              [
                case "${enableval}" in
                 yes) PLUGIN_SRC="$PLUGIN_SRC qamsource"
                      BUILD_EMULATOR_QAM=yes
                      DVRMGR_FLAGS="$DVRMGR_FLAGS -DUSE_QAMSRC";;
                 no)  BUILD_EMULATOR_QAM=no
                     AC_MSG_NOTICE([qamsrc plugin is disabled]) ;;
                 *) AC_MSG_ERROR([bad value ${enableval} for --enable-emulator-qam]) ;;
                esac
              ],
              [echo "emulator qam is disabled"])
AM_CONDITIONAL(USE_EMULATOR_QAM,   test "x$BUILD_EMULATOR_QAM" = "xyes")

AC_ARG_ENABLE([snmp],
              AS_HELP_STRING([--enable-snmp],[enable snmp plugin (default is no)]),
              [
      		case "${enableval}" in
        	 yes) PLUGIN_SRC="$PLUGIN_SRC snmp";;
        	 no)  AC_MSG_ERROR([snmp plugin is disabled]) ;;
        	 *) AC_MSG_ERROR([bad value ${enableval} for --enable-snmp]) ;;
       		esac
    	      ],
    	      [echo "snmp is disabled"])
AC_ARG_ENABLE([dumpfilesink],
              AS_HELP_STRING([--enable-dumpfilesink],[enable dumpfilesink plugin (default is no)]),
              [
      		case "${enableval}" in
        	 yes) PLUGIN_SRC="$PLUGIN_SRC dumpfilesink";;
        	 no)  AC_MSG_ERROR([dumpfilesink plugin is disabled]) ;;
        	 *) AC_MSG_ERROR([bad value ${enableval} for --enable-dumpfilesink]) ;;
       		esac
    	      ],
    	      [echo "dumpfilesink is disabled"])
AC_ARG_ENABLE([rbi],
              AS_HELP_STRING([--enable-rbi],[enable rbi (default is no)]),
              [
      		case "${enableval}" in
        	 yes) RBI_ENABLE=true PLUGIN_SRC="$PLUGIN_SRC rbi" DVRMGR_FLAGS="$DVRMGR_FLAGS -DUSE_RBI" 
        	        PKG_CHECK_MODULES([CURL],[libcurl >= 7.19.6]);;
        	 no)  RBI_ENABLE= false AC_MSG_ERROR([rbi plugin is disabled]) ;;
        	 *) AC_MSG_ERROR([bad value ${enableval} for --enable-rbi]) ;;
       		esac
    	      ],
    	      [echo "rbi plugin is disabled"])
AC_ARG_ENABLE([rbidaemon],
              AS_HELP_STRING([--enable-rbidaemon],[enable rbidaemon (default is no)]),
              [
      		case "${enableval}" in
        	 yes) PLUGIN_SRC="$PLUGIN_SRC rbi/test"  PKG_CHECK_MODULES([CURL],[libcurl >= 7.19.6]);;
        	 no)  AC_MSG_ERROR([rbidaemon plugin is disabled]) ;;
        	 *) AC_MSG_ERROR([bad value ${enableval} for --enable-rbidaemon]) ;;
       		esac
    	      ],
    	      [echo "rbidaemon plugin is disabled"])
AC_ARG_ENABLE([rbiadcache],
        AS_HELP_STRING([--enable-rbiadcache],[enable rbiadcache]),
        [
          case "${enableval}" in
           yes) RBI_ENABLE_ADCACHE=true ;;
           no) RBI_ENABLE_ADCACHE=false ;;
          *) AC_MSG_ERROR([bad value ${enableval} for --enable-rbiadcache ]) ;;
           esac
           ],
         [echo "rbiadcache is enable"])
AM_CONDITIONAL([RBI_ENABLE_ADCACHE], [test x$RBI_ENABLE_ADCACHE = xtrue])
AM_CONDITIONAL([DVRMGR_ENABLE], [test x$DVRMGR_ENABLE = xtrue])
AM_CONDITIONAL([CC_ENABLE], [test x$CC_ENABLE = xtrue])
AC_ARG_ENABLE([podmgr],
              AS_HELP_STRING([--enable-podmgr],[enable podmgr support in qamsource (default is yes)]),
              [
               case "${enableval}" in
                yes) ENABLE_PODMGR=true;;
                no) ENABLE_PODMGR=false;;
                *) AC_MSG_ERROR([bad value ${enableval} for --enable-podmgr]) ;;
                       esac
             ],
             [echo "Podmgr support is enabled in QamSource"])

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

AC_ARG_ENABLE([xonestb],
        AS_HELP_STRING([--enable-xonestb],[enable XONE_STB]),
        [
          case "${enableval}" in
           yes) XONE_STB=true ;;
           no) XONE_STB=false ;;
          *) AC_MSG_ERROR([bad value ${enableval} for --enable-xonestb ]) ;;
           esac
           ],
         [echo "XONE_STB is enable"])
AM_CONDITIONAL([XONE_STB], [test x$XONE_STB = xtrue])

AC_ARG_ENABLE([dtv],
              AS_HELP_STRING([--enable-dtv],[enable dtvsource plugin (default is no)]),
              [
      		case "${enableval}" in
        	 yes) PLUGIN_SRC="$PLUGIN_SRC dtvsource"
		      enable_dtv=true
		      ;;
        	 no)  AC_MSG_ERROR([dtvsource plugin is disabled])
		      enable_dtv=false
		      ;;
        	 *) AC_MSG_ERROR([bad value ${enableval} for --enable-dtv]) ;;
       		esac
    	      ],
    	      [echo "dtvsource plugin is disabled"])

AM_CONDITIONAL([ENABLE_PODMGR], [test x$ENABLE_PODMGR = xtrue])
AM_CONDITIONAL(BUILD_PODMGR, [test -d qamsource/podmgr/canh])
AM_CONDITIONAL(BUILD_SUBDIR_PODSERVER, [test -d qamsource/podmgr/podserver])

m4_syscmd([test -d qamsource/podmgr/canh])
m4_if(m4_sysval,[0],[AC_CONFIG_FILES([qamsource/podmgr/Makefile])])
m4_if(m4_sysval,[0],[AC_CONFIG_FILES([qamsource/podmgr/podclient/Makefile])])
m4_if(m4_sysval,[0],[AC_CONFIG_FILES([qamsource/podmgr/canh/Makefile])])
m4_if(m4_sysval,[0],[BUILD_PODMGR=yes])

m4_syscmd([test -d qamsource/simgr/oob])
m4_if(m4_sysval,[0],[AC_CONFIG_FILES([qamsource/simgr/oob/Makefile])])
m4_if(m4_sysval,[0],[SUBDIR_OOB=oob])

m4_syscmd([test -d qamsource/podmgr/podserver])
m4_if(m4_sysval,[0],[AC_CONFIG_FILES([qamsource/podmgr/runpod/Makefile])])
m4_if(m4_sysval,[0],[AC_CONFIG_FILES([qamsource/podmgr/podserver/Makefile])])
m4_if(m4_sysval,[0],[SUBDIR_PODSERVER=podserver])

AC_ARG_ENABLE([yocto],
        AS_HELP_STRING([--enable-yocto],[enable yocto build]),
        [
          case "${enableval}" in
           yes) BUILD_PODMGR=no
                ENABLE_YOCTO=true
                YOCTO_FLAG=" -DYOCTO_BUILD "
                SUBDIR_PODSERVER= ;;
           no) AC_MSG_NOTICE([yocto build is disabled]);;
          *) AC_MSG_NOTICE([bad value ${enableval} for --enable-headless ]) ;;
           esac
           ],
         [echo "Yocto build is disabled"])
AM_CONDITIONAL([ENABLE_YOCTO], [test x$ENABLE_YOCTO = xtrue])
AC_SUBST(YOCTO_FLAG)
AM_CONDITIONAL([BUILD_PODMGR], [test x$BUILD_PODMGR = xno])
# Create Symlink for platforms that are non-yocto based.
AM_COND_IF([ENABLE_YOCTO],
           [],
           [rm -f snmp/snmpmanager_priv qamsource/podmgr/podserver/hal qamsource/podmgr/podserver/cardmanager/scm_pod_stack/cacpPriv
            $LN_S ../qamsource/podmgr/thirdparty/snmpmanager_priv snmp/snmpmanager_priv
            $LN_S ../../../platform/soc/soc_qamsource/hal qamsource/podmgr/podserver/hal
            $LN_S ../../../thirdparty/cacpPriv qamsource/podmgr/podserver/cardmanager/scm_pod_stack/cacpPriv
            ])

m4_syscmd([test -d snmp])
m4_if(m4_sysval,[0],[AC_CONFIG_FILES([snmp/Makefile])])
m4_if(m4_sysval,[0],[AC_CONFIG_FILES([snmp/snmpmanager/Makefile])])
m4_if(m4_sysval,[0],[AC_CONFIG_FILES([snmp/ipcutils/Makefile])])
m4_if(m4_sysval,[0],[AC_CONFIG_FILES([snmp/ipcclient/Makefile])])
m4_if(m4_sysval,[0],[AC_CONFIG_FILES([snmp/runsnmp/Makefile])])
m4_if(m4_sysval,[0],[AC_CONFIG_FILES([snmp/snmpinterface/Makefile])])

AM_CONDITIONAL(USE_TRM, test "x$enable_trh" = xtrue)
AM_CONDITIONAL(USE_DVR, test "x$DVRMGR_ENABLE" = xtrue)

PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 0.10.28])
PKG_CHECK_MODULES([XML2], [libxml-2.0 >= 2.7.8])
PKG_CHECK_MODULES([JANSON],[jansson >= 2.4])

AC_DEFINE(GLIB_VERSION_MIN_REQUIRED, GLIB_VERSION_2_30, [Ignore post 2.30 deprecations])

AC_SUBST(GST_MAJORMINOR)
AC_SUBST(PLUGIN_SRC)
AC_SUBST(DVRMGR_FLAGS)
AC_SUBST(CC_ENABLE)
AC_SUBST(ENABLE_SDV)
AC_SUBST(ENABLE_DVB)
AC_SUBST(DVRMGR_ENABLE)
AC_SUBST(OSAL_SRC)
AC_SUBST(OSAL_ENABLE)
AC_SUBST(ENABLE_PODMGR)
AC_SUBST(SUBDIR_PODSERVER)
AC_SUBST(SUBDIR_OOB)

AC_CONFIG_FILES([Makefile
         cfg/Makefile
         core/Makefile
         hnsource/Makefile
         init/Makefile
         osal/Makefile
	 osal/src/Makefile
	 osal/utils/Makefile
	 osal/utils/src/Makefile
	 osal/include/Makefile
	 osal/tools/Makefile
	 osal/tools/apicaller/Makefile
	 osal/tools/threadanalyzer/Makefile
	 rmfApp/Makefile
	 mediaplayersink/Makefile
     rmfproxyservice/Makefile
     transcoderfilter/Makefile
     hnsink/Makefile
     ippvsource/Makefile
     vodsource/Makefile
     trh/Makefile
     sdv/Makefile
     sdv/configuration/Makefile
     qamsource/simgr/Makefile
	 qamsource/simgr/inband/Makefile
	 qamsource/dvb/Makefile
	 qamsource/dvb/wrapper/Makefile
	 qamsource/dvb/simgr/Makefile
	 qamsource/dvb/source/Makefile
	 qamsource/Makefile
	 qamsource/sdv/Makefile
	 qamsource/oobsimgrstub/Makefile
	 qamsource/main/Makefile
	 anycas/Makefile
	 anycas/test/Makefile
     dumpfilesink/Makefile
     rbi/Makefile
     rbi/test/Makefile
     sdv/discovery/Makefile
     dtvsource/Makefile])
AC_OUTPUT
