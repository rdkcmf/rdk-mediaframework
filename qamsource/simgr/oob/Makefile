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
CFLAGS?=  -Wno-format -Wunused -DUSE_CC_GETTIMEOFDAY -DRMF_OSAL_FEATURE_IPV6 -DRMF_OSAL_LITTLE_ENDIAN
#-DRMF_OSAL_FEATURE_MEM_PROF
ARFLAGS?=rcsi
INBSIMGR_DIR=$(BASE_DIR)/simgr/inband
SF_DIR=$(BASE_DIR)/sectionfilter
TUNER_DIR=$(BASE_DIR)/tuner
PODMGR_DIR=$(BASE_DIR)/podmgr

SRC_DIR=src

TST_DIR=test
TST_BIN=rmf_si_test

OOBSIMGR_LIBNAME=oobsimanager
OOBSICACHE_LIBNAME=oobsicache

#LIBFILE=$(LIBDIR)/lib$(LIBNAME).a
OOBSIMGR_LIBFILE=$(LIBDIR)/lib$(OOBSIMGR_LIBNAME).a
OOBSICACHE_LIBFILE=$(LIBDIR)/lib$(OOBSICACHE_LIBNAME).a
OBJ_DIR=$(BUILD_DIR)/objs_$(OOBSIMGR_LIBNAME)
TEST_OBJ_DIR=$(BUILD_DIR)/objs_$(TST_BIN)

INCLUDES+= -I include \
           -I $(BASE_DIR) \
	   -I $(SF_DIR)/include \
	   -I $(INBSIMGR_DIR)/include \
	   -I $(TUNER_DIR)/include \
	   -I $(PODMGR_DIR)/inc \
	   -I $(PODMGR_DIR)/podserver/inc \
	   -I $(RDK_DIR)/iarmmgrs/generic/sysmgr/include \
		-I $(RDK_DIR)/iarmbus/core/include 

CFLAGS += $(INCLUDES)

#ifdef TEST_WITH_PODMGR
#CFLAGS += -DTEST_WITH_PODMGR
#endif

OOBSICACHE_OBJS= $(OBJ_DIR)/rmf_oobsicache.o \
		$(OBJ_DIR)/rmf_sicache.o

OOBSIMGR_OBJS=$(OBJ_DIR)/rmf_oobsimgr.o \
	$(OBJ_DIR)/rmf_oobsiparser.o \
	$(OBJ_DIR)/rmf_oobsisttmgr.o \
	$(OBJ_DIR)/rmf_oobService.o

TESTS=$(TEST_OBJ_DIR)/rmf_si_test.o

.PHONY : all clean
#all: test

all: $(OOBSICACHE_LIBFILE) $(OOBSIMGR_LIBFILE)

$(OOBSICACHE_LIBFILE): $(LIBDIR) $(OBJ_DIR) $(OOBSICACHE_OBJS)
#ifdef USE_POD_IPC
#	$(CXX) -shared -L $(LIBDIR) -lc -lrt -o $@ $(CFLAGS) ${OOBSICACHE_OBJS} $(SYSROOT_FLAG)
#else
	$(AR) $(ARFLAGS) $(OOBSICACHE_LIBFILE) $(OOBSICACHE_OBJS) 
#endif

$(OOBSIMGR_LIBFILE): $(LIBDIR) $(OBJ_DIR) $(OOBSIMGR_OBJS)
#ifdef USE_POD_IPC
#	$(CXX) -shared -L $(LIBDIR)  -lsectionfilter -loobsicache -lc -lrt -o $@ $(CFLAGS) ${OOBSIMGR_OBJS} $(SYSROOT_FLAG)
#else
	$(AR) $(ARFLAGS) $@ $(OOBSIMGR_OBJS) 
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
#	$(CC) -o $(BUILD_DIR)/$(TST_BIN) $(TESTS) -l$(LIBNAME) -losal -linbsimanager -lsectionfilter -lpodmgr $(WHOLE_LIB_PATH) $(WHOLE_LIBS) -lpodmgr $(SYSROOT_FLAG)
#endif
clean :
	rm -rf $(OBJ_DIR) $(LIBFILE) $(OOBSIMGR_LIBFILE) $(OOBSICACHE_LIBFILE)

	
