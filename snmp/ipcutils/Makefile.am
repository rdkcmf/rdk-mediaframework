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

SUBDIRS = 
AM_CXXFLAGS = -pthread -Wall -Wno-format -Wunused $(GST_CFLAGS) -DRMF_OSAL_LITTLE_ENDIAN -D_GNU_SOURCE -DGCC4_XXX \
              -I${top_srcdir}/qamsource/podmgr/canh \
              -I${top_srcdir}/osal/include \
              -I${top_srcdir}/qamsource/podmgr/inc \
              -I${top_srcdir}/qamsource/simgr/inband/include \
              -I${top_srcdir}/qamsource/common \
              -I${top_srcdir}/qamsource/main \
              -I${top_srcdir}/core \
              -I${top_srcdir}/qamsource/oobsimgrstub \
              -I${top_srcdir}/qamsource/podmgr/podserver/inc \
              -I${top_srcdir}/snmp/ipcutils/utils 

AM_CFLAGS = $(AM_CXXFLAGS)

lib_LTLIBRARIES = libipcutils.la
libipcutils_la_LDFLAGS = $(GST_LIBS)  -lrdkloggers -llog4c

libipcutils_la_SOURCES = ${top_srcdir}/snmp/ipcutils/utils/bits_cdl.cpp \
			 ${top_srcdir}/snmp/ipcutils/utils/cmThreadBase.cpp \
			 ${top_srcdir}/snmp/ipcutils/utils/persistent.cpp \
			 ${top_srcdir}/snmp/ipcutils/utils/vlEnv.cpp \
			 ${top_srcdir}/snmp/ipcutils/utils/cmhash.cpp \
			 ${top_srcdir}/snmp/ipcutils/utils/list.cpp \
			 ${top_srcdir}/snmp/ipcutils/utils/vector.cpp \
			 ${top_srcdir}/snmp/ipcutils/utils/applitest.c \
			 ${top_srcdir}/snmp/ipcutils/utils/buf_parse_utils.c \
			 ${top_srcdir}/snmp/ipcutils/utils/net_utils.c \
			 ${top_srcdir}/snmp/ipcutils/utils/sys_error_string.c

EXTRA_DIST = utils/applitest.h  \
             utils/cmhash.h	 \
             utils/cVector.h   \
             utils/si_util.h  \
             utils/vlEnv.h \
             utils/bits_cdl.h   \
             utils/cmThreadBase.h  \
             utils/persistentconfig.h  \
             utils/utilityMacros.h

libipcutils_la_includedir = $(includedir)
libipcutils_la_include_HEADERS = $(EXTRA_DIST)
