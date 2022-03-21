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
TOOLCHAIN_DIR=$(RDK_DIR)/sdk/toolchain/staging_dir
CC?=g++
AR?=ar
#CFLAGS= -Wall -g
CFLAGS?=  -Wno-format -Wunused -DUSE_CC_GETTIMEOFDAY -DRMF_OSAL_FEATURE_IPV6 -DRMF_OSAL_LITTLE_ENDIAN
#-DRMF_OSAL_FEATURE_MEM_PROF
ARFLAGS?=rcs
SRC_DIR=.

LIBNAME=qamsrc_test

IARM_PATH=$(RDK_DIR)/iarmbus

LIBFILE=$(LIBDIR)/lib$(LIBNAME).a
OBJ_DIR=$(BUILD_DIR)/objs_$(LIBNAME)

TXML_DIR=$(OPENSOURCE_SRC_DIR)/tinyxml
TXML_OBJS= $(TXML_DIR)/tinystr.o $(TXML_DIR)/tinyxmlerror.o $(TXML_DIR)/tinyxmlparser.o  $(TXML_DIR)/tinyxml.o

INCLUDES+=-I $(OPENSOURCE_SRC_DIR)/mongoose -I $(OPENSOURCE_SRC_DIR)/include -I $(OPENSOURCE_SRC_DIR)/include/mongoose  -I $(BASE_DIR)/main -I $(BASE_DIR)/oobsimgrstub -I $(RMF_DIR)/core -I $(BASE_DIR)/../mediaplayersink -I $(RMF_HNSINK_INCLUDE_DIR) \
			-I $(RMF_DIR)/hnsource \
			-I $(RDK_DIR)/gst-plugins/src/gst-dtcp-enc/include \
			-I${PLATFORM_SDK}/lib/glib-2.0/include \
			-I${PLATFORM_SDK}/include/glib-2.0 \
			-I${PLATFORM_SDK}/include/gstreamer-0.10 \
			-I${PLATFORM_SDK}/include/libxml2 \
			-I$(RMF_DIR)/dvr/common/dvrmgr \
			-I$(BASE_DIR)/simgr/inband/include \
			-I$(BASE_DIR)/sectionfilter/include \
			-I$(RMF_DIR)/init 
CFLAGS+= $(INCLUDES) $(SYSROOT_FLAG)

CHAN_TST_BIN=chanchangetest.out
QAM_TST_BIN=qamsrc_test.out
SI_TEST_BIN=simgrtest.out

RMF_PLAYER_OBJ:= $(RMF_DIR)/mediaplayersink/mediaplayersink.o \
				 $(RMF_DIR)/hnsource/hnsource.o \
				 $(RMF_DIR)/core/rmfbase.o \
				-L$(RDK_DIR)/opensource/lib \
				-lrmfosal -lrdkloggers -lrmfosalutils -llog4c \
				-L${PLATFORM_SDK}/lib \
				-L${PLATFORM_SDK}/lib/glib-2.0 \
				-L$(PLATFORM_SDK)/usr/lib \
				-L$(PLATFORM_SDK)/usr/local/lib \
				-L$(PLATFORM_SDK)/lib
			
LD_FLAGS:= -lqamsrc $(INBSILIBS) -lgstapp-0.10 -lgstreamer-0.10 -lglib-2.0 -lpthread $(PLATFORM_SHLIBS) -L$(RDK_DIR)/opensource/lib -L$(RDK_DIR)/sdk/fsroot/ramdisk/lib $(SOC_SHLIBS) -L $(TOOLCHAIN_DIR)/lib $(SYSROOT_FLAG)

LD_FLAGS += -L $(RDK_DIR)/opensource/lib -ltinyxml
LD_FLAGS += -L $(RMF_DIR)/dvr/common/tasseograph -lTasseograph
LD_FLAGS += -L $(RMF_DIR)/dvr/common/dvrmgr -ldvrmgr
LD_FLAGS += -L $(RDK_DIR)/opensource/lib -luuid

ifdef TEST_WITH_PODMGR
LD_FLAGS += $(OOBSILIBS) -lpodmgr $(WHOLE_LIB_PATH) -L$(RDK_DIR)/opensource/target-snmp/lib $(WHOLE_LIBS)  
ifdef USE_TRM
LD_FLAGS += -ltrh -ljansson
endif
endif

ifeq ($(USE_GLI),1)
LD_FLAGS +=-L$(IARM_PATH)/core -lIARMBus
ifeq ($(USE_DBUS),y)
LD_FLAGS += -ldbus-1
else
LD_FLAGS += -lfusiondale
endif
endif

ifeq ($(USE_SYS_RESOURCE_MLT), y)
LD_FLAGS+= -L $(SYSRES_DIR)/sys_resource/lib/platform/canmore -lsysResource
endif

ifeq ($(USE_SYS_RESOURCE_MON), y)
LD_FLAGS+= -L $(SYSRES_DIR)/sys_resource/lib/platform/canmore -lsysResource
endif


ifeq ($(USE_SYS_RESOURCE_MLT), y)
RMF_PLAYER_OBJ += -L $(SYSRES_DIR)/sys_resource/lib/platform/canmore -lsysResource
endif

ifeq ($(USE_SYS_RESOURCE_MON), y)
RMF_PLAYER_OBJ += -L $(SYSRES_DIR)/sys_resource/lib/platform/canmore -lsysResource
endif


OBJS=$(OBJ_DIR)/rmf_qamsrc_test.o $(OBJ_DIR)/testsink.o $(OBJ_DIR)/qamsrc_test_server.o

CHTESTBIN_OBJS=$(OBJ_DIR)/ChannelChangeTest.o

SITESTBIN_OBJS=$(OBJ_DIR)/SiManagerTest.o

.PHONY : all clean test 
all: $(LIBFILE) 
$(LIBFILE): $(LIBDIR) $(OBJ_DIR) $(OBJS) 
	$(AR) $(ARFLAGS) $(LIBFILE) $(OBJS) 


$(OBJ_DIR)/%.o :$(SRC_DIR)/%.cpp
	$(CC) -c -o $@ $< $(CFLAGS)
	
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)
	
$(LIBDIR):
	mkdir -p $(LIBDIR)
	
clean : 
	rm -rf $(LIBFILE) $(OBJS) $(OBJ_DIR) $(BUILD_DIR)/$(QAM_TST_BIN) $(BUILD_DIR)/$(CHAN_TST_BIN) $(BUILD_DIR)/$(SI_TEST_BIN)

qamsrctest : $(TESTBIN_OBJS)
	$(CC) -o $(BUILD_DIR)/$(QAM_TST_BIN) $(RMF_PLAYER_OBJ) $(LD_FLAGS)  -L $(LIBDIR) -L $(RDK_DIR)/rdklogger/build/lib -lrdkloggers $(LIBFILE) -loobsimgrstub -L$(LIBDIR) -linit $(SOC_SHLIBS) $(PLATFORM_SHLIBS) $(WHOLE_LIB_PATH) $(SYSROOT_FLAG)

chanchangetest : $(CHTESTBIN_OBJS)
	$(CC) -o $(BUILD_DIR)/$(CHAN_TST_BIN) $(RMF_PLAYER_OBJ) $(LD_FLAGS) -L $(LIBDIR)  -L $(RDK_DIR)/rdklogger/build/lib $(CHTESTBIN_OBJS) -lrdkloggers -L$(LIBDIR) $(SOC_SHLIBS) $(PLATFORM_SHLIBS) $(WHOLE_LIB_PATH) $(SYSROOT_FLAG)

simgrtest : $(SITESTBIN_OBJS)
	$(CC) -o $(BUILD_DIR)/$(SI_TEST_BIN) $(RMF_PLAYER_OBJ) $(LD_FLAGS) -L $(LIBDIR) -L $(RDK_DIR)/rdklogger/build/lib -lrdkloggers $(SITESTBIN_OBJS) -L$(LIBDIR) $(SOC_SHLIBS) $(PLATFORM_SHLIBS) $(WHOLE_LIB_PATH) -L$(RDK_DIR)/sdk/fsroot/ramdisk/lib $(INBSILIBS) -lrmfosal $(SYSROOT_FLAG)


