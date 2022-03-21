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
AM_CPPFLAGS = -pthread -Wall $(GST_CFLAGS)  -I${top_srcdir}/hnsource -I${top_srcdir}/osal/include \
	      -I${top_srcdir}/init -I${top_srcdir}/core -I${top_srcdir}/mediaplayersink -I${top_srcdir}/dtvsource \
              -DRMF_OSAL_FEATURE_IPV6 -DRMF_OSAL_LITTLE_ENDIAN -DUSE_HNSRC -DUSE_MEDIAPLAYERSINK \
	      -DGCC4_XXX $(QAM_CFLAGS) $(DVR_CFLAGS) $(DTV_CFLAGS) $(UUID_CFLAGS) $(GTHREAD_CFLAGS) $(CC_FLAGS) $(CC_DEFINES)

bin_PROGRAMS=rmfApp

if DVRMGR_ENABLE
rmfApp_SOURCES= help.cpp process_commands.cpp pipeline.cpp rmfApp.cpp main.cpp dvr_functions.cpp
else
rmfApp_SOURCES= help.cpp process_commands.cpp pipeline.cpp rmfApp.cpp main.cpp
endif

if CC_ENABLE
rmfApp_SOURCES+= closedcaption.cpp
endif

rmfApp_LDFLAGS= \
	-L${RDK_FSROOT_PATH}/usr/lib \
	-L${RDK_FSROOT_PATH}/usr/local/lib \
	-lrdkloggers \
	-llog4c \
	$(IARMBUS_LIBS) \
	$(DSMGR_LIBS) \
	$(SOC_LIBS) \
	-lmediaplayersink \
	-lrmfbase \
	-lrmfosal \
	-lrmfinit \
	-lhnsource \
	-lrmfosalutils \
	${DUMPFILESINK_LIB} \
	${HNSINK_LIB} \
	${TRANSCODER_LIB} \
	${RMFPROXYSVC_LIB} \
	${DVR_LIBS} \
	${QAM_LIBS} \
	${DTV_LIBS} \
	$(GLIB_LIBS) \
	$(GTHREAD_LIBS) \
	$(GST_LIBS) \
	$(GST_BASE_LIBS) \
	$(UUID_LIBS) \
	$(CC_LIBS)

#rmfApp_LDADD= ${PKG_CONFIG_SYSROOT_DIR}/usr/lib/libuuid.la

WRKDIR          =       .

### IPK_GENERATION_PACKAGE###
IPK_GEN_PATH = ./ipk
IPK_GEN_STAGING_DIR=$(WRKDIR)/staging_dir
IPK_GEN_INPUT  = debian-binary control.tar.gz data.tar.gz
IPK_GEN_OUTPUT = rmfApp.ipk

package_ipk:
	@echo "Generating IPK Package"
	$(MAKE) prefix=${abs_top_builddir}/staging_dir install
	$(mkdir_p) $(IPK_GEN_STAGING_DIR)/usr/bin
	cp -af $(abs_top_builddir)/rmfApp $(IPK_GEN_STAGING_DIR)/usr/bin/
	tar -czvf $(IPK_GEN_PATH)/data.tar.gz -C $(IPK_GEN_STAGING_DIR) usr
	tar -czvf $(IPK_GEN_PATH)/control.tar.gz -C $(IPK_GEN_PATH) control
	cd $(IPK_GEN_PATH) && ar cr $(IPK_GEN_OUTPUT) $(IPK_GEN_INPUT) && cd -
	rm -rf $(IPK_GEN_PATH)/data.tar.gz
	rm -rf $(IPK_GEN_PATH)/control.tar.gz
	rm -rf $(IPK_GEN_STAGING_DIR)
