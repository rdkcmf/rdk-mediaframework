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

SUBDIRS = src 
AM_CPPFLAGS = -pthread -Wall
lib_LTLIBRARIES= librmfosalutils.la
librmfosalutils_la_SOURCES=
librmfosalutils_la_LIBADD= \
	src/librmfosalutilssrc.la \
	-lstdc++

if OSAL_STANDALONE
librmfosalutils_la_LIBADD += ${top_builddir}/src/libsrc.la
else
librmfosalutils_la_LIBADD += ${top_builddir}/osal/src/libsrc.la
endif

EXTRA_DIST = inc/rmf_osal_resreg.h \
             inc/cVector.h \
             inc/rmf_osal_scriptexecutor.h \
             inc/rmf_osal_udpsocketserver.h  \
             inc/vlMap.h \
             inc/vlVector.h \
             inc/rmf_osal_tcpsocketserver.h  \
             inc/vlExportTypes.h \
             inc/vlMutex.h


librmfosalutils_la_LDFLAGS = -lrdkloggers

librmfosalutils_la_includedir = $(includedir)
librmfosalutils_la_include_HEADERS = $(EXTRA_DIST)
