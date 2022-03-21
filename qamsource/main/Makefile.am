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
 

SUBDIRS = 
LOCAL_DEFINES =   -DMERGE_INTEL \
                  -DENABLE_NEW_LOGSVR \
                  -DCMCST_EXT_X1 \
                  -DUSE_DSG \
                  -DUSE_CDL \
                  -DUSE_MFR \
                  -DUSE_TUNER \
                  -DNV_RAM_READ \
                  -DINTEL_PR13 \
                  -DINTEL_PR18 \
                  -DUSE_VIDSINK_API \
                  -g -O2 -Wall \
                  -fno-short-enums \
                  -ffloat-store \
                  -fno-strict-aliasing \
                  -DHAVE_STDINT_H \
                  -D_GNU_SOURCE \
                  -DMPE_FEATURE_DEBUG \
                  -DMPE_MEM_HANDLES \
                  -DRMF_OSAL_LITTLE_ENDIAN \
                  -DJGFX_DEV \
                  -D$(RMF_BUILD_OS) \
                  -D$(POD_PLATFORM) \
                  -DLINUX \
                  -DUSE_CC_GETTIMEOFDAY \
                  -DGCC4_XXX \
                  -DNUM_SURF=1 \
                  -DDEPRECATED_INTERFACE_SUPPORT \
                  -DUSE_NEW_MSPOD_DRIVER \
                  -DUSE_CDL_SERVICE \
                  -DCOMMON_DOWNLOAD \
                  -DSNMP_IPC_CLIENT  \
                  -D$(MFR_NAME) \
                  -DIPPV_CLIENT_ENABLED  \
                  -DUSE_PXYSRVC  \
                  -DQAMSRC_PMTBUFFER_PROPERTY

if ENABLE_PODMGR
LOCAL_DEFINES += -DUSE_POD -DMPOD_SUPPORT -DTEST_WITH_PODMGR -DUSE_POD_IPC
endif

if HEADLESS
LOCAL_DEFINES+= -DHEADLESS_GW
else
LOCAL_DEFINES+= -DHEADED_GW
endif

AM_CXXFLAGS =  -fdata-sections -ffunction-sections -pthread $(GST_CFLAGS) $(LOCAL_DEFINES) \
              -I${top_srcdir}/qamsource/podmgr/canh \
              -I${top_srcdir}/osal/include \
              -I${top_srcdir}/qamsource/podmgr/inc \
              -I${top_srcdir}/qamsource/simgr/inband/include \
              -I${top_srcdir}/qamsource/common \
              -I${top_srcdir}/qamsource/main \
              -I${top_srcdir}/core \
              -I${top_srcdir}/qamsource/oobsimgrstub

lib_LTLIBRARIES = libqamsrc.la
libqamsrc_la_LDFLAGS = $(GST_LIBS) $(GST_BASE_LIBS) -lrdkloggers -llog4c -lIARMBus
libqamsrc_la_SOURCES = rmfqamsrc.cpp rmfqamsrcpriv.cpp

EXTRA_DIST = QAMSrcDiag.h  \
             rmfqamsrc.h  \
             rmfqamsrcpriv.h

if ENABLE_SDV
LOCAL_DEFINES+= -DUSE_SDVSRC
libqamsrc_la_SOURCES += rmfsdvsource.cpp
EXTRA_DIST += rmfsdvsource.h
AM_CXXFLAGS += -I$(RDK_PROJECT_ROOT_PATH)/sdvagent/src
AM_CXXFLAGS += -I${top_srcdir}/sdv/discovery
AM_CXXFLAGS += -I${top_srcdir}/sdv/configuration
AM_CXXFLAGS += -I${top_srcdir}/qamsource/sdv/include
endif

if ENABLE_DVB
LOCAL_DEFINES += -DUSE_DVBSRC
AM_CXXFLAGS += -I${top_srcdir}/qamsource/dvb/source/include
endif

libqamsrc_la_includedir = $(includedir)
libqamsrc_la_include_HEADERS = $(EXTRA_DIST)

libqamsrc_la_LIBADD = ${top_builddir}/qamsource/oobsimgrstub/liboobsimgrstub.la \
		${top_builddir}/qamsource/simgr/inband/libinbsimanager.la


if BUILD_PODMGR
libqamsrc_la_LIBADD += $(PODMGR_LIBS)
else
libqamsrc_la_LIBADD +=
endif

if USE_TRM
libqamsrc_la_LIBADD += -ltrm
LOCAL_DEFINES+= -DUSE_TRM
endif

if DVRMGR_ENABLE
LOCAL_DEFINES+=-DUSE_DVR
AM_CXXFLAGS += -I${top_srcdir}/dvr/common/dvrmgr
endif
