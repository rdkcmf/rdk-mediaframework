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
		-I$(RMF_DIR)/osal/include \
                -I${RMF_DIR}/qamsource/podmgr/podserver/inc/ \
                -I${RMF_DIR}/qamsource/podmgr/podserver/utils/ \
                -I${RMF_DIR}/osal/utils/inc/ \
		-I${RMF_DIR}/qamsource/podmgr/podserver/commonDownloadManager/ \
		-I${RMF_DIR}/platform/soc/soc_qamsource/hal/hal_include/ \
		-I${RMF_DIR}/platform/soc/soc_qamsource/hal/halmfr/ \
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
		
CFLAGS += -DMERGE_INTEL \
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

LD_FLAGS+=  $(INTERNAL_LIBS)  -L$(BUILD_DIR)/lib -lipcutils -lsnmpmanager \
		-lhalsnmp -lsnmpinterface \
		-lipcclient
SRC_DIR=.
LIBNAME=snmpinterface
LIBFILE=lib$(LIBNAME).so
OBJ_DIR=$(BUILD_DIR)/objs_$(LIBNAME)

CFLAGS+= $(INCLUDES)

OBJS=$(OBJ_DIR)/snmpIntrfGetAvOutData.o \
	$(OBJ_DIR)/snmpIntrfGetTunerData.o \
#	$(OBJ_DIR)/vlCCIInfo.o \


.PHONY : all clean
all: $(LIBFILE)

$(LIBFILE): $(OBJ_DIR) $(OBJS)
	$(CC) $(OBJS) -shared -o $(LIBFILE)
	cp $(LIBFILE) $(OBJ_DIR)
	cp $(LIBFILE) $(BUILD_DIR)/lib/

$(OBJ_DIR)/%.o :$(SRC_DIR)/%.cpp
	$(CC) -c -o $@ $< $(CFLAGS) $(LD_FLAGS)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean :
	rm -rf $(OBJ_DIR) $(LIBFILE) *~

