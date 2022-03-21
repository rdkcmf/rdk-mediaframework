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

INCLUDES +=     -I${RDK_DIR}/opensource/target-snmp/include/ \
				-I${RDK_DIR}/mfrlibs	\
		-I$(RMF_DIR)/osal/include \
                -I${RMF_DIR}/qamsource/podmgr/podserver/inc/ \
                -I${RMF_DIR}/qamsource/podmgr/podserver/utils/ \
                -I${RMF_DIR}/osal/utils/inc/ \
		-I${RMF_DIR}/qamsource/podmgr/podserver/commonDownloadManager/ \
		-I${RMF_DIR}/platform/soc/soc_qamsource/hal/hal_include/ \
		-I${RMF_DIR}/platform/soc/soc_qamsource/hal/halplatform/ \
		-I${RMF_DIR}/platform/soc/halsnmp/ \
                -I${RMF_DIR}/snmp/snmpmanager/ \
                -I${RMF_DIR}/snmp/snmpinterface/ \
                -I${RMF_DIR}/snmp/ipcutils/ \
                -I${RMF_DIR}/snmp/ipcclient/ \
                -I${RMF_DIR}/snmp/ipcutils/utils \
		-I${RMF_DIR}/platform/soc/soc_qamsource/hal/haldsg/ \
		-I${RMF_DIR}/qamsource/podmgr/podserver/cardmanager/scm_pod_stack/diag/ \
		-I${RMF_DIR}/qamsource/podmgr/podserver/cardmanager/ \
		
BUILD_FLAGS += -DMERGE_INTEL \
                  -DENABLE_NEW_LOGSVR \
                  -DUSE_POD \
                  -DUSE_DSG \
                  -DUSE_CDL \
                  -DMPOD_SUPPORT \
                  -DUSE_MFR \
                  -DUSE_TUNER \
                  -DNV_RAM_READ \
                  -DENABLE_SNMP \
                  -DINTEL_PR13 \
                  -DINTEL_PR18 \
                  -DUSE_VIDSINK_API \
                  -DHAVE_STDINT_H \
                  -DMPE_FEATURE_DEBUG \
                  -DMPE_MEM_HANDLES \
                  -DRMF_OSAL_LITTLE_ENDIAN \
                  -DLINUX \
                  -DUSE_CC_GETTIMEOFDAY \
                  -DGCC4_XXX \
                  -DUSE_FULL_SNMP \

BUILD_FLAGS += $(PLATFORM_FLAGS)

#LD_FLAGS+=  $(INTERNAL_LIBS)  -L$(BUILD_DIR)/lib -lipcclient -lsnmpmanager \
		-lhalsnmp -lsnmpinterface \

LIBNAME=ipcutils
OBJ_DIR=$(BUILD_DIR)/objs_$(LIBNAME)
#ifdef USE_POD_IPC
LIBFILE=$(LIBDIR)/lib$(LIBNAME).so
#else
#LIBFILE=$(LIBDIR)/lib$(LIBNAME).a
#endif

IPC_UTILS ?= $(RMF_DIR)/snmp/ipcutils
UTILSSRCDIR = $(shell (find  . halmfr/ -type d | grep -v .svn))
UTILSSRCS_W_DIRS = $(foreach dir,$(UTILSSRCDIR),$(wildcard $(dir)/*.c))
UTILSSRCS_W_DIRS_CPP = $(foreach dir,$(UTILSSRCDIR),$(wildcard $(dir)/*.cpp))
UTILSSRCS = $(notdir $(UTILSSRCS_W_DIRS))
UTILSSRCS_CPP = $(notdir $(UTILSSRCS_W_DIRS_CPP))

vpath %.c ${UTILSSRCDIR}
vpath %.cpp ${UTILSSRCDIR}
vpath %.o $(OBJ_DIR)
C_OBJS = $(UTILSSRCS:%.c=${OBJ_DIR}/%.o)
CPP_OBJS = $(UTILSSRCS_CPP:%.cpp=${OBJ_DIR}/%.o)

INCLUDES +=	-I$(IPC_UTILS)/utils \
                -I$(IPC_UTILS)/halmfr \

.PHONY: all
all: $(LIBFILE)

$(LIBFILE): $(LIBDIR) $(OBJ_DIR) $(C_OBJS) $(CPP_OBJS)
	echo "CPP FILES: $(CPP_OBJS)"
	$(CC) -shared -o $(LIBFILE) $(C_OBJS) $(CPP_OBJS) $(BUILD_FLAGS) $(INCLUDES) $(LD_FLAGS) $(WHOLE_LIB_PATH) $(SYSROOT_FLAG)

#ifdef USE_POD_IPC 
	#$(CC) -shared -lc -lrt -o $@  $(INCLUDES) $(BUILD_FLAGS) -Wl,--start-group ${C_OBJS} ${CPP_OBJS} $(LD_FLAGS) -Wl,--end-group $(WHOLE_LIB_PATH) --sysroot=$(TOOLCHAIN_DIR)
#else
#	$(AR) $(ARFLAGS) $(LIBFILE) $(C_OBJS) $(CPP_OBJS)
#endif
#	cp $(LIBFILE) $(OBJ_DIR)
#	cp $(LIBFILE) $(BUILD_DIR)/lib/

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

$(LIBDIR):
	mkdir -p $(LIBDIR)

clean :
	rm -rf $(OBJ_DIR) $(LIBFILE) *~

$(OBJ_DIR)/%.o: %.cpp
	@echo '$(CC) $<'
	$(CC)  $(INCLUDES) $(BUILD_FLAGS) -c $^ -o $@

$(OBJ_DIR)/%.o: %.c
	@echo '$(CC) $<'
	$(CC) $(INCLUDES) $(BUILD_FLAGS) -c $^ -o $@
	

