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

AM_CXXFLAGS = -pthread -Wall -Wno-format -Wunused $(GST_CFLAGS) $(CURL_CFLAGS) -I${top_srcdir}/qamsource/main -I${top_srcdir}/osal/include -I${top_srcdir}/core -DRMF_OSAL_LITTLE_ENDIAN -DGCC4_XXX

if RBI_ENABLE_ADCACHE
AM_CXXFLAGS += -DENABLE_AD_CACHE -DRT_PLATFORM_LINUX -std=c++11 -DENABLE_NODE_V_6_9 -DENABLE_RT_NODE -DENABLE_HTTP_CACHE -DRUNINMAIN -DPNG_APNG_SUPPORTED
AM_CXXFLAGS += -I${PKG_CONFIG_SYSROOT_DIR}$(includedir)/pxcore
endif

AM_CXXFLAGS += -I=/usr/include/wdmp-c -I=/usr/include

AM_CFLAGS = $(AM_CXXFLAGS) -x c++

if XONE_STB
AM_CXXFLAGS += -DXONE_STB
endif
lib_LTLIBRARIES = librbi.la librbifilter.la

librbi_la_SOURCES = rbi.cpp
librbi_la_LDFLAGS = $(CURL_LIBS)

if RBI_ENABLE_ADCACHE
librbi_la_LDFLAGS += -Wl,--whole-archive, ${PKG_CONFIG_SYSROOT_DIR}/usr/lib/libpxCore.a ${PKG_CONFIG_SYSROOT_DIR}/usr/lib/libpxscene.a, --no-whole-archive
endif

if OSAL_ENABLE
librbi_la_LIBADD = ${top_builddir}/osal/librmfosal.la
else
librbi_la_LIBADD = -lrmfosal
endif

if RBI_ENABLE_ADCACHE
librbi_la_LIBADD += -lrtCore
endif

librbi_la_LIBADD += -lrfcapi

librbi_la_includedir = $(includedir)
librbi_la_include_HEADERS = rbi.h rbirpc.h

librbifilter_la_SOURCES = RBIFilter.cpp
librbifilter_la_includedir = $(includedir)
librbifilter_la_include_HEADERS = RBIFilter.h

