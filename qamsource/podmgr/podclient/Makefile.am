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
                  -DUSE_POD \
                  -DUSE_DSG \
                  -DUSE_CDL \
                  -DMPOD_SUPPORT \
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
                  -DTEST_WITH_PODMGR \
                  -DCOMMON_DOWNLOAD \
                  -DSNMP_IPC_CLIENT  \
                  -D$(MFR_NAME) \
		  -DHEADED_GW  \
                  -DIPPV_CLIENT_ENABLED  \
                  -DUSE_POD_IPC  \
		  -DUSE_PXYSRVC \
	          -DHAVE_STDINT_H 
						
if !BUILD_SUBDIR_PODSERVER
LOCAL_DEFINES +=-DNO_CIPHER
endif

LOCAL_DEFINES += -DUSE_IPDIRECT_UNICAST

AM_CXXFLAGS = -pthread -Wall -Wno-format -Wunused $(GST_CFLAGS) $(LOCAL_DEFINES) \
              -I${top_srcdir}/qamsource/podmgr/canh \
              -I${top_srcdir}/canh \
              -I${top_srcdir}/osal/include \
              -I${top_srcdir}/qamsource/podmgr/inc \
              -I${top_srcdir}/inc \
              -I${top_srcdir}/qamsource/simgr/inband/include \
              -I${top_srcdir}/qamsource/common \
              -I${top_srcdir}/qamsource/main \
              -I${top_srcdir}/core \
              -I${top_srcdir}/qamsource/oobsimgrstub \
              -I${top_srcdir}/trh \
              -I${top_srcdir}/qamsource/podmgr/podserver/inc \
              -I${top_srcdir}/podserver/inc \
              -I${top_srcdir}/osal/utils/inc \
              -I${top_srcdir}/qamsource/simgr/oob/include

lib_LTLIBRARIES = libpodmgr.la
libpodmgr_la_LDFLAGS = $(GST_LIBS)  -lrdkloggers -llog4c $(TRH_LIBS)
libpodmgr_la_SOURCES = inband_fwu.cpp podClient.cpp
if BUILD_SUBDIR_PODSERVER
libpodmgr_la_SOURCES+=cipher.cpp
endif

