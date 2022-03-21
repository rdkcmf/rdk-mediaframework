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


if ENABLE_SDV
SUBDIRS = $(OSAL_SRC) core init hnsource sdv $(PLUGIN_SRC)
DIST_SUBDIRS = cfg $(OSAL_SRC) core init hnsource sdv $(PLUGIN_SRC)
else
SUBDIRS = $(OSAL_SRC) core init hnsource $(PLUGIN_SRC)
DIST_SUBDIRS = cfg $(OSAL_SRC) core init hnsource $(PLUGIN_SRC)
endif

distcleancheck_listfiles = *-libtool

WRKDIR          =       .

IPK_GEN_PATH = $(abs_top_builddir)/ipk
IPK_GEN_STAGING_DIR=$(WRKDIR)/staging_dir
IPK_GEN_INPUT  = debian-binary control.tar.gz data.tar.gz
IPK_GEN_OUTPUT = rmfgeneric.ipk

package_ipk:
	@echo "Generating IPK Package"
	$(MAKE) prefix=${abs_top_builddir}/staging_dir install
	$(mkdir_p) $(IPK_GEN_STAGING_DIR)/usr/lib
	cp -af $(IPK_GEN_STAGING_DIR)/lib/*.so* $(IPK_GEN_STAGING_DIR)/usr/lib/
	tar -czvf $(IPK_GEN_PATH)/data.tar.gz -C $(IPK_GEN_STAGING_DIR) usr
	tar -czvf $(IPK_GEN_PATH)/control.tar.gz -C $(IPK_GEN_PATH) control
	cd $(IPK_GEN_PATH) && ar cr $(IPK_GEN_OUTPUT) $(IPK_GEN_INPUT) && cd -
	rm -rf $(IPK_GEN_PATH)/data.tar.gz
	rm -rf $(IPK_GEN_PATH)/control.tar.gz
	rm -rf $(IPK_GEN_STAGING_DIR)
