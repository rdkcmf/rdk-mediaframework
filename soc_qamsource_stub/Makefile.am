#
# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2013 RDK Management
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
#

SUBDIRS = sectionfilter
DIST_SUBDIRS = sectionfilter

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
                  -DLINUX \
                  -DUSE_CC_GETTIMEOFDAY \
                  -DGCC4_XXX \
                  -DNUM_SURF=1 \
                  -DDEPRECATED_INTERFACE_SUPPORT \
                  -DUSE_NEW_MSPOD_DRIVER \
                  -DUSE_CDL_SERVICE \
                  -DCOMMON_DOWNLOAD \
                  -DSNMP_IPC_CLIENT  \
		  -DHEADED_GW  \
                  -DIPPV_CLIENT_ENABLED  \
		  -DUSE_PXYSRVC \
		  -DGLI \
	          -DHAVE_STDINT_H \
	          -DVL_USE_MEMORY_POOL_FOR_POD_WRITE

if ENABLE_PODMGR
LOCAL_DEFINES += -DUSE_POD -DMPOD_SUPPORT -DTEST_WITH_PODMGR -DUSE_POD_IPC
endif

LOCAL_DEFINES += -DUSE_IPDIRECT_UNICAST


AM_CXXFLAGS = -pthread -Wall -Wno-format -Wunused $(GST_CFLAGS) $(OPENSSL_CFLAGS) $(LOCAL_DEFINES) -Wwrite-strings
AM_CXXFLAGS += -I${PKG_CONFIG_SYSROOT_DIR}$(includedir) \
		-I${PKG_CONFIG_SYSROOT_DIR}$(includedir)/rdk/podmgr/

AM_CFLAGS = $(AM_CXXFLAGS) -x c++

lib_LTLIBRARIES = libhal.la
libhal_la_LDFLAGS = $(GST_LIBS)  -L${FSROOT}/usr/lib -L${FSROOT}/usr/local/lib -lrdkloggers -llog4c $(OPENSSL_LDFLAGS) -lcurl

libhal_la_SOURCES = halpod/halpod.cpp \
                    halsnmp/halsnmp.c
