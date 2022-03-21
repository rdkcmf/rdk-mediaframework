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


AM_CXXFLAGS = -pthread -Wall -DRMF_OSAL_LITTLE_ENDIAN $(GST_CFLAGS) $(GLIB_CFLAGS) -I${top_srcdir}/osal/include -I${top_srcdir}/core $(RMFINIT_CFLAGS)

lib_LTLIBRARIES = librmfinit.la
librmfinit_la_SOURCES = rmf_platform.cpp
librmfinit_la_LDFLAGS = $(GST_LIBS) $(GLIB_LIBS) 

if OSAL_ENABLE
librmfinit_la_LIBADD = ${top_builddir}/osal/librmfosal.la
else
librmfinit_la_LIBADD =  -lrmfosal
endif

EXTRA_DIST = rmf_platform.h
librmfinit_la_includedir = ${includedir}
librmfinit_la_include_HEADERS = rmf_platform.h
