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
AM_CXXFLAGS = -pthread -Wall -Wno-format -Wunused $(GST_CFLAGS) -DRMF_OSAL_LITTLE_ENDIAN -D_GNU_SOURCE -DGCC4_XXX -I${top_srcdir}/qamsource/podmgr/canh -I${top_srcdir}/osal/include -I${top_srcdir}/qamsource/podmgr/inc -I${top_srcdir}/qamsource/simgr/inband/include -I${top_srcdir}/qamsource/common -I${top_srcdir}/qamsource/main -I${top_srcdir}/core -I${top_srcdir}/qamsource/oobsimgrstub

EXTRA_DIST = canhClient.h  disconnectMgr.h
lib_LTLIBRARIES = libcanh.la
libcanh_la_LDFLAGS = $(GST_LIBS)  -lrdkloggers -llog4c -lIARMBus
libcanh_la_SOURCES = canhClient.cpp disconnectMgr.cpp
libcanh_la_includedir = $(includedir)
libcanh_la_include_HEADERS = $(EXTRA_DIST)
