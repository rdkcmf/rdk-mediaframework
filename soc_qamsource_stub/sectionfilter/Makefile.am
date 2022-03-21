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

SUBDIRS =
AUTOMAKE_OPTIONS = subdir-objects
-include ${PKG_CONFIG_SYSROOT_DIR}${includedir}/platform_app.inc
AM_CXXFLAGS = -pthread -Wall -Wno-format -Wunused $(GST_CFLAGS) -DRMF_OSAL_LITTLE_ENDIAN -D_GNU_SOURCE -DGCC4_XXX \
              -DRMF_SF_SINGLE_THREAD_ENABLE -I./include $(NEXUS_CFLAGS)

AM_CFLAGS = $(AM_CXXFLAGS)

AM_LDFLAGS = $(GST_LIBS)  -L${FSROOT}/usr/lib -L${FSROOT}/usr/local/lib -lrdkloggers -llog4c

EXTRA_DIST = include/rmf_sectionfilter_oob.h \
             include/rmf_sectionfilter_inb.h

lib_LTLIBRARIES = libinbsectionfilter.la
lib_LTLIBRARIES +=liboobsectionfilter.la

libinbsectionfilter_la_SOURCES= src/rmf_sectionfilter_factory.cpp
liboobsectionfilter_la_SOURCES= src/rmf_sectionfilter_oob.cpp
liboobsectionfilter_la_includedir = $(includedir)
liboobsectionfilter_la_include_HEADERS = $(EXTRA_DIST)
