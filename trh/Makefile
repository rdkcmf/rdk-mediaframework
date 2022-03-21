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
ifeq ($(RDK_DIR),)
$(error RDK_DIR not set.  Please define RDK_DIR to point to base dir"!)
endif

RMF_DIR?=$(RDK_DIR)/mediaframework
include $(RMF_DIR)/config.mak
#CFLAGS= -Wall -g
CFLAGS?=  -Wno-format -Wunused -DUSE_CC_GETTIMEOFDAY -DRMF_OSAL_FEATURE_IPV6 -DRMF_OSAL_LITTLE_ENDIAN
#-DRMF_OSAL_FEATURE_MEM_PROF
ARFLAGS?=rcs
SRC_DIR=.
LIBNAME=trh

TST_DIR=test
TST_BIN=trh_test

LIBFILE=$(LIBDIR)/lib$(LIBNAME).so
OBJ_DIR=$(BUILD_DIR)/objs_$(LIBNAME)

INCLUDES+= -I . -I $(SRC_DIR)/include -I$(UTILS_DIR)/inc -I$(TOOLCHAIN_DIR)/include/glib-2.0 -I$(TOOLCHAIN_DIR)/lib/glib-2.0/include -I$(RDK_DIR)/opensource/src/log4c/src -I$(RDK_DIR)/opensource/include -I$(RDK_DIR)/opensource/include/uuid

CFLAGS += -DGCC4_XXX

CFLAGS+= $(INCLUDES)
OBJS=$(OBJ_DIR)/TunerReservationHelper.o

TRM_COMMON_DIR = $(RDK_DIR)/trm/common
TRM_COMMON_INC_DIR = $(TRM_COMMON_DIR)/include
TRM_COMMON_SRC_DIR = $(TRM_COMMON_DIR)/src

INCLUDES += -I$(TRM_COMMON_INC_DIR)

SOURCE_TRM = Activity.cpp ResponseStatus.cpp  TunerReservation.cpp Klass.cpp JsonEncoder.cpp JsonDecoder.cpp JanssonParser.cpp TunerState.cpp Util.cpp 

TSTLIBS+= -L$(LIBDIR) -l$(LIBNAME)
TSTLIBS+= -L$(LIBDIR) -lrmfosal  -lrmfosalutils -lsysResource 
TSTLIBS+= -L$(RDK_DIR)/opensource/lib  -llog4c -ljansson  -luuid -losal
TSTLIBS+= -lpthread -L${PLATFORM_SDK}/lib/glib-2.0 -lgthread-2.0 -lglib-2.0 

#OBJS += $(addprefix $(TRM_COMMON_SRC_DIR)/, $(SOURCE_TRM))
OBJS += $(addprefix $(OBJ_DIR)/, $(SOURCE_TRM:.cpp=.o))

TESTS=$(OBJ_DIR)/TRHTest.o

.PHONY : all clean test

ifdef USE_TRM
all: $(LIBFILE)
else
all:
	@echo "TRM is disabled, not building it"
endif

ifeq ($(RDK_PLATFORM_SOC),intel)
$(LIBFILE): $(LIBDIR) $(OBJ_DIR) $(OBJS) 
	$(CC) -shared -o $(LIBFILE) $(OBJS) -L$(LIBDIR) -L$(RMF_DIR)/../opensource/lib -Llib --sysroot=$(TOOLCHAIN_DIR)
endif

ifeq ($(RDK_PLATFORM_SOC),broadcom)
$(LIBFILE): $(LIBDIR) $(OBJ_DIR) $(OBJS) 
	$(CC) -shared -o $(LIBFILE) $(OBJS) -L$(LIBDIR) -L$(RMF_DIR)/../opensource/lib -Llib --sysroot=$(TOOLCHAIN_DIR)/mipsel-linux-uclibc/sys-root/
endif

$(OBJ_DIR)/%.o :$(SRC_DIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)/%.o :$(SRC_DIR)/%.cpp
	$(CXX) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)/%.o :$(TRM_COMMON_SRC_DIR)/%.cpp
	$(CXX) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)/%.o :$(TST_DIR)/%.cpp
	$(CXX) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)
	
$(LIBDIR):
	mkdir -p $(LIBDIR)

$(TEST_OBJ_DIR)/%.o :$(TST_DIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

clean :
	rm -rf $(OBJ_DIR) $(LIBFILE) $(BUILD_DIR)/$(TST_BIN) 

test :  $(LIBFILE)  $(TESTS)
	$(CC) -o $(BUILD_DIR)/$(TST_BIN) $(TESTS) $(TSTLIBS) --sysroot=$(TOOLCHAIN_DIR)
	
