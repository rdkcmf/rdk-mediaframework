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
                  -DQAMSRC_PMTBUFFER_PROPERTY \
                  -DQAMSRC_PATBUFFER_PROPERTY \
                  -DQAMSRC_CATBUFFER_PROPERTY

if ENABLE_PODMGR
LOCAL_DEFINES += -DUSE_POD -DMPOD_SUPPORT -DTEST_WITH_PODMGR -DUSE_POD_IPC
endif

if HEADLESS
LOCAL_DEFINES+= -DHEADLESS_GW
else
LOCAL_DEFINES+= -DHEADED_GW
endif

if USE_EMULATOR_QAM
SF_LIBS= -loobsectionfilter \
	 -linbsectionfilter
else
SF_LIBS=
endif

if ENABLE_EXTERNAL_CAS
LOCAL_DEFINES += -DUSE_EXTERNAL_CAS -DCAT_FUNCTIONALITY
endif

AM_CXXFLAGS = -pthread $(GST_CFLAGS) $(LOCAL_DEFINES) \
              -I${top_srcdir}/qamsource/podmgr/canh \
              -I${top_srcdir}/osal/include \
              -I${top_srcdir}/qamsource/podmgr/inc \
              -I${top_srcdir}/qamsource/simgr/inband/include \
              -I${top_srcdir}/qamsource/common \
              -I${top_srcdir}/qamsource/main \
              -I${top_srcdir}/core \
              -I${top_srcdir}/qamsource/oobsimgrstub

lib_LTLIBRARIES = libsectionfilter.la libinbsimanager.la
libsectionfilter_la_SOURCES= ./src/rmf_symboltable.cpp ./src/rmf_sectionfilter_util.cpp
libsectionfilter_la_LIBADD = -lrdkloggers $(SF_LIBS)
libinbsimanager_la_SOURCES = ./src/rmf_inbsimgr.cpp ./src/rmf_sectionfilter.cpp
libinbsimanager_la_LIBADD = libsectionfilter.la -lrdkloggers $(INBSECTIONFILTER_LIBS)

EXTRA_DIST = include/rmf_inbsi_common.h  \
             include/rmf_inbsimgr.h 

libinbsimanager_la_includedir = $(includedir)
libinbsimanager_la_include_HEADERS = $(EXTRA_DIST)
