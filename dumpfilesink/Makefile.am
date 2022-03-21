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
AM_CXXFLAGS = -pthread -Wall $(GST_CFLAGS) -I${top_srcdir}/core -I${top_srcdir}/osal/include -DRMF_OSAL_LITTLE_ENDIAN
lib_LTLIBRARIES = libdumpfilesink.la
libdumpfilesink_la_SOURCES = dumpfilesink.cpp
libdumpfilesink_la_LDFLAGS = $(GST_LIBS)  -lrdkloggers -llog4c
libdumpfilesink_la_includedir = $(includedir)
libdumpfilesink_la_include_HEADERS = dumpfilesink.h
