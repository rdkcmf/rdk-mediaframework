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
RMF_DIR=$(RDK_DIR)/mediaframework
include $(RMF_DIR)/config.mak

CFLAGS?=  -Wno-format -Wunused -DUSE_CC_GETTIMEOFDAY -DRMF_OSAL_FEATURE_IPV6 -DRMF_OSAL_LITTLE_ENDIAN

ARFLAGS?=rcs
SRC_DIR=$(PWD)

CFLAGS+= -I$(PLATFORM_SDK)/usr/include $(INCLUDES)

.PHONY : all clean test libs
all: libs test

libs: $(LIBDIR)
	make -C sectionfilter
	make -C simgr/inband
	make -C simgr/oob
	make -C gstqamtunersrc
	make -C oobsimgrstub 
	make -C main
ifdef TEST_WITH_PODMGR
	make -C podmgr
endif

$(LIBDIR):
	mkdir -p $@

clean :
	make -C gstqamtunersrc clean
	make -C sectionfilter clean
	make -C simgr/inband clean
	make -C simgr/oob clean
	make -C main clean
	make -C oobsimgrstub clean
ifdef TEST_WITH_PODMGR
	make -C podmgr clean
endif
	make -C test clean

cleanall : clean
	make -C ../osal clean
	make -C ../hnsink clean

test : libs $(TEST_OBJ_DIR)
ifndef GENERATE_SI_CACHE_UTILITY
	make -C test
endif

testapp :
ifndef GENERATE_SI_CACHE_UTILITY
	make -C test qamsrctest	
	make -C test chanchangetest
	make -C test simgrtest 
endif

