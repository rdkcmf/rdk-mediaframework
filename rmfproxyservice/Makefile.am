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
AM_CXXFLAGS = -pthread -Wall -Wno-format -Wunused -DUSE_CC_GETTIMEOFDAY -DRMF_OSAL_FEATURE_IPV6 -DRMF_OSAL_LITTLE_ENDIAN -DUSE_VODSRC -DUSE_PXYSRVC -I${top_srcdir}/osal/include $(JANSON_CFLAGS) -I${top_srcdir}/core
AM_CXXFLAGS += -I${top_srcdir}/qamsource/podmgr/podserver/inc  -I${top_srcdir}/qamsource/podmgr/inc
AM_CFLAGS = $(AM_CXXFLAGS)
lib_LTLIBRARIES = librmfproxyservice.la
librmfproxyservice_la_SOURCES=rmfproxyservice.cpp  T2PClient.cpp
librmfproxyservice_la_LIBADD= $(sysreslib) -ljansson

if OSAL_ENABLE
librmfproxyservice_la_LIBADD += $(top_builddir)/osal/librmfosal.la
else
librmfproxyservice_la_LIBADD += -lrmfosal
endif

EXTRA_DIST = rmfproxyservice.h T2PClient.h rmf_gli.h
librmfproxyservice_la_includedir = $(includedir)
librmfproxyservice_la_include_HEADERS = $(EXTRA_DIST)
