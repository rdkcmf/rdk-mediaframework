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
RMF_DIR=$(RDK_DIR)/mediaframework
include $(RMF_DIR)/config.mak
BASE_DIR=$(RMF_DIR)/qamsource
#CFLAGS= -Wall -g
CFLAGS?=  -Wno-format -Wunused -DUSE_CC_GETTIMEOFDAY -DRMF_OSAL_FEATURE_IPV6 -DRMF_OSAL_LITTLE_ENDIAN
#-DRMF_OSAL_FEATURE_MEM_PROF
ARFLAGS?=rcs
SF_DIR=$(BASE_DIR)/sectionfilter
TUNER_DIR=$(BASE_DIR)/tuner
PODMGR_DIR=$(BASE_DIR)/podmgr
SRC_DIR=src
OPEN_LIBS=$(RDK_DIR)/opensource/libs/
TST_DIR=test
TST_BIN=inband_si_test
LIBNAME=inbsimanager
INBSIMGR_LIBNAME=inbsimanager
INBSICACHE_LIBNAME=inbsicache

INBSIMGR_LIBFILE=$(LIBDIR)/lib$(INBSIMGR_LIBNAME).so
INBSICACHE_LIBFILE=$(LIBDIR)/lib$(INBSICACHE_LIBNAME).so

ifdef OPTIMIZE_INBSI_CACHE
CFLAGS += -DOPTIMIZE_INBSI_CACHE
endif

OBJ_DIR=$(BUILD_DIR)/objs_$(INBSIMGR_LIBNAME)
TEST_OBJ_DIR=$(BUILD_DIR)/objs_$(TST_BIN)

INCLUDES+= -I include \
	   -I $(SF_DIR)/include \
	   -I $(TUNER_DIR)/include 

ifeq ($(USE_SYS_RESOURCE_MLT), y)
INCLUDES+= -I $(SYSRES_DIR)/sys_resource/include
endif

CFLAGS += $(INCLUDES)
INBSICACHE_OBJS= $(OBJ_DIR)/rmf_inbsicache.o

INBSIMGR_OBJS=$(OBJ_DIR)/rmf_inbsimgr.o

TESTS=$(TEST_OBJ_DIR)/rmf_inbsi_test.o

.PHONY : all clean
#all: test
all: $(INBSIMGR_LIBFILE)

$(INBSICACHE_LIBFILE): $(LIBDIR) $(OBJ_DIR) $(INBSICACHE_OBJS)
#	$(AR) $(ARFLAGS) $(LIBFILE) $(OBJS)
	$(CXX) -shared -L $(LIBDIR) -lc -lrt -lrmfosal -L $(RDK_DIR)/rdklogger/build/lib -lrdkloggers -o $@ $(CFLAGS) ${INBSICACHE_OBJS} $(SYSROOT_FLAG)

$(INBSIMGR_LIBFILE): $(LIBDIR) $(OBJ_DIR) $(INBSIMGR_OBJS)
#	$(AR) $(ARFLAGS) $(LIBFILE) $(OBJS)
#ifdef TEST_WITH_PODMGR
#	$(CC) -shared $(WHOLE_LIB_PATH) $(WHOLE_LIBS) -lpodmgr -lsectionfilter -linbsicache -lc -lrt -lrmfosal -o $@ $(CFLAGS) ${INBSIMGR_OBJS} $(SYSROOT_FLAG)
#else
	$(CC) -shared -L $(LIBDIR) -lsectionfilter -lrmfosal -L $(RDK_DIR)/rdklogger/build/lib -lrdkloggers -lc -lrt -o $@ $(CFLAGS) ${INBSIMGR_OBJS} $(SYSROOT_FLAG)
#endif

$(OBJ_DIR)/%.o :$(SRC_DIR)/%.cpp
	$(CC) -c -o $@ $< $(CFLAGS)
	
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)
	
$(LIBDIR):
	mkdir -p $(LIBDIR)

$(TEST_OBJ_DIR):
	mkdir -p $@

$(TEST_OBJ_DIR)/%.o :$(TST_DIR)/%.cpp
	$(CC) -c -o $@ $< $(CFLAGS)

test : $(TEST_OBJ_DIR) $(LIBFILE) $(TESTS)
#ifdef TEST_WITH_PODMGR
#	$(CC) -o $(BUILD_DIR)/$(TST_BIN) $(TESTS) -l$(LIBNAME) -lrmfosal  -lpodmgr -lsectionfilter -linbsicache $(WHOLE_LIB_PATH) $(WHOLE_LIBS) $(SYSROOT_FLAG)
#else
	$(CC) -o $(BUILD_DIR)/$(TST_BIN) $(TESTS) -l$(LIBNAME) -lrmfosal -lrdkloggers  -lsectionfilter -L $(LIBDIR) $(TST_FLAGS) $(SYSROOT_FLAG)
#endif

clean :
	rm -rf $(OBJ_DIR) $(INBSIMGR_LIBFILE) $(INBSICACHE_LIBFILE)

	
