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

AM_CXXFLAGS = -pthread -Wall -DRMF_OSAL_LITTLE_ENDIAN -DUSE_CC_GETTIMEOFDAY -DRMF_OSAL_FEATURE_IPV6 \
              -DSNMP_IPC_CLIENT -DGCC4_XXX $(GST_CFLAGS) $(GST_LIBS)  \
              -I${top_srcdir}/osal/include \
              -I${top_srcdir}/osal/utils/inc

lib_LTLIBRARIES = librmfbase.la
librmfbase_la_SOURCES = rmfbase.cpp

if OSAL_ENABLE
librmfbase_la_LIBADD = ${top_builddir}/osal/librmfosal.la
else
librmfbase_la_LIBADD =  -lrmfosal
endif

librmfbase_la_includedir = ${includedir}
librmfbase_la_include_HEADERS = rmfbase.h rmfcore.h rmfprivate.h rmfElementBuffer.h
