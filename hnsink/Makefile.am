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
AM_CXXFLAGS = -pthread -Wall -Wno-format -Wunused $(GST_CFLAGS) \
              -I${top_srcdir}/core -I${top_srcdir}/osal/include \
              -Wno-format -Wunused \
              -DUSE_CC_GETTIMEOFDAY -DRMF_OSAL_FEATURE_IPV6 \
              -DRMF_OSAL_LITTLE_ENDIAN -DRMF_STREAMER -DUSE_DTCP

AM_CFLAGS = $(AM_CXXFLAGS)
AM_LDFLAGS = $(GST_LIBS) -lDtcpMgr
lib_LTLIBRARIES = libhnsink.la
libhnsink_la_SOURCES = hnsink.cpp
libhnsink_la_includedir = $(includedir)
libhnsink_la_include_HEADERS = hnsink.h
