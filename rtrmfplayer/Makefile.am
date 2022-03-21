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
AUTOMAKE_OPTIONS = subdir-objects
# ACLOCAL_AMFLAGS = -I cfg

SUBDIRS =

AM_CXXFLAGS = -g \
	-Wall -std=c++0x \
	-I$(top_srcdir) \
	$(CURL_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GST_CFLAGS) \
	$(JANSON_CFLAGS)

AM_CXXFLAGS += -DRT_PLATFORM_LINUX

bin_PROGRAMS = rtrmfplayer

rtrmfplayer_SOURCES = \
	main.cpp \
	glib_tools.cpp \
	glib_tools.h \
	rtrmfplayer.cpp \
	rtrmfplayer.h \
	mediaplayerdlna.cpp \
	mediaplayerdlna.h \
	mediaplayergeneric.cpp \
	mediaplayergeneric.h \
	logger.cpp \
	logger.h \
	intrect.h \
	mediaplayer.h

rtrmfplayer_LDFLAGS = \
	-lrmfbase \
	-lhnsource \
	-lmediaplayersink \
	-ldumpfilesink
if USE_PXCORE_SHARED_LIBS
rtrmfplayer_LDFLAGS += \
	-lrtRemote \
	-lrtCore
else
rtrmfplayer_LDFLAGS += \
	-lrtRemote_s \
	-lrtCore_s \
	-lpthread \
	-luuid \
	-ldl
endif
rtrmfplayer_LDFLAGS += \
	$(SOC_LIBS) \
	$(CURL_LIBS) \
	$(GLIB_LIBS) \
	$(GST_LIBS) \
	$(JANSON_LIBS)

# SOC_LIBS="-lIARMBus -lcanh -lsdvsectionfilter -lrmfsdvdiscovery -lrmfsdvconfiguration -ldvrmgrsoc"

if ENABLE_DIRECT_QAM
AM_CXXFLAGS += -DENABLE_DIRECT_QAM
rtrmfplayer_LDFLAGS += -lqamsrc
endif

if ENABLE_IPPV_CLIENT
AM_CXXFLAGS += -DIPPV_CLIENT_ENABLED
rtrmfplayer_LDFLAGS += -lippvsource
endif

if ENABLE_VOD_SRC
AM_CXXFLAGS += -DUSE_VODSRC
rtrmfplayer_LDFLAGS += -lvodsrc
endif

if ENABLE_RDK_LOGGER
AM_CXXFLAGS += -DUSE_RDK_LOGGER
rtrmfplayer_LDFLAGS += -lrdkloggers -llog4c
endif

if ENABLE_BREAKPAD
AM_CXXFLAGS += -DENABLE_BREAKPAD $(breakpad_CFLAGS)
rtrmfplayer_LDFLAGS += -lbreakpadwrapper $(breakpad_LIBS)
endif

## IPK Generation Support
IPK_GEN_PATH = $(abs_top_builddir)/ipk
IPK_GEN_STAGING_DIR=$(abs_top_builddir)/staging_dir
IPK_GEN_INPUT  = debian-binary control.tar.gz data.tar.gz
IPK_GEN_OUTPUT = rtrmfplayer.ipk

package_ipk:
	@echo "Generating IPK Package"
	$(MAKE) prefix=${abs_top_builddir}/staging_dir install
	$(mkdir_p) $(IPK_GEN_STAGING_DIR)/usr/bin/
	cp -af $(IPK_GEN_STAGING_DIR)/bin/* $(IPK_GEN_STAGING_DIR)/usr/bin/
	tar -czvf $(IPK_GEN_PATH)/data.tar.gz -C $(IPK_GEN_STAGING_DIR) usr
	tar -czvf $(IPK_GEN_PATH)/control.tar.gz -C $(IPK_GEN_PATH) control postinst prerm
	cd $(IPK_GEN_PATH) && ar cr $(IPK_GEN_OUTPUT) $(IPK_GEN_INPUT) && cd -
	rm -rf ${abs_top_builddir}/staging_dir
	rm -rf $(abs_top_builddir)/ipk/control.tar.gz
	rm -rf $(abs_top_builddir)/ipk/data.tar.gz
