#
# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2011 RDK Management
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
####


RMF_DIR=$(RDK_DIR)/mediaframework
include $(RMF_DIR)/config.mak


.PHONY : all clean 
all: libs

libs: $(LIBDIR)
	make -C ipcutils
	#make -C halsnmp
	make -C snmpinterface
	make -C ipcclient
	make -C snmpmanager
	make -C runsnmp
$(LIBDIR):
	mkdir -p $@

clean :
	make -C ipcutils clean
	make -C ipcclient clean
	make -C snmpmanager clean
	#make -C halsnmp clean
	make -C snmpinterface clean
	make -C runsnmp clean
	rm -rf *~
