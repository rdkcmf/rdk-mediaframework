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
AM_CXXFLAGS = -pthread -Wall $(GST_CFLAGS) -I${top_srcdir}/core -I${top_srcdir}/osal/include -DRMF_OSAL_LITTLE_ENDIAN -DUSE_DTCP
lib_LTLIBRARIES = libhnsource.la
libhnsource_la_SOURCES = hnsource.cpp
libhnsource_la_LDFLAGS = $(GST_LIBS)  -lrdkloggers -llog4c
libhnsource_la_LIBADD = $(top_builddir)/core/librmfbase.la
libhnsource_la_includedir = $(includedir)
libhnsource_la_include_HEADERS = hnsource.h
