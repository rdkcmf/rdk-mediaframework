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
LOCAL_DEFINES =   -DUSE_TUNER \
                  -DUSE_QAMSRC \
                  -DRMF_OSAL_LITTLE_ENDIAN \
                  -g -O2 -Wall \
                  -fno-short-enums \
                  -ffloat-store \
                  -fno-strict-aliasing \
                  -D_GNU_SOURCE





AM_CXXFLAGS =  -fdata-sections -ffunction-sections -pthread $(GST_CFLAGS) $(LOCAL_DEFINES) \
              -I${top_srcdir}/osal/include \
              -I${top_srcdir}/qamsource/main \
              -I${top_srcdir}/core \
              -I${top_srcdir}/libmediaplayer

lib_LTLIBRARIES = libmediaplayer.la
libmediaplayer_la_LDFLAGS = -lrdkloggers -llog4c -lcanh -lrmfosal
libmediaplayer_la_SOURCES = libmediaplayer.cpp
libmediaplayer_la_LIBADD = ${top_builddir}/core/librmfbase.la ${top_builddir}/init/librmfinit.la ${top_builddir}/mediaplayersink/libmediaplayersink.la ${top_builddir}/qamsource/main/libqamsrc.la ${top_builddir}/sdv/configuration/librmfsdvconfiguration.la ${top_builddir}/sdv/discovery/librmfsdvdiscovery.la ${top_builddir}/qamsource/sdv/libsdvsectionfilter.la 

EXTRA_DIST = libmediaplayer.h

libmediaplayer_la_includedir = $(includedir)
libmediaplayer_la_include_HEADERS = $(EXTRA_DIST)


bin_PROGRAMS=mplayertestapp

mplayertestapp_SOURCES= mplayertestapp.cpp

mplayertestapp_LDFLAGS= \
	-L${RDK_FSROOT_PATH}/usr/lib \
	-L${RDK_FSROOT_PATH}/usr/local/lib \
	-lrdkloggers \
	-llog4c

mplayertestapp_LDADD = libmediaplayer.la
