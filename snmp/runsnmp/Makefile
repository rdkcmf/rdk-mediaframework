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
OPENSRC_PATH=${RDK_DIR}/opensource

OPENSSL_DIR ?= $(RDK_DIR)/opensource/src/openssl
OPENSSL_SRC_DIR ?= $(OPENSSL_DIR)

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
                -I${RMF_DIR}/snmp/ipcutils/utils \
                -I${RMF_DIR}/snmp/ipcclient/ \
                -I${RMF_DIR}/platform/soc/soc_qamsource/hal/haldsg/ \
                -I${RMF_DIR}/qamsource/podmgr/podserver/cardmanager/scm_pod_stack/diag/ \
                -I${RMF_DIR}/qamsource/podmgr/podserver/cardmanager/ \
		-I${RDK_DIR}/opensource/include/gnutls \
		-I${RDK_DIR}/opensource/src/gnutls/src \
		-I$(RMF_DIR)/ \
		-I${RDK_DIR}/sdk/fsroot/ramdisk/lib/ \


ifdef XG2V2_GW
#LD_FLAGS += -L${RDK_DIR}/sdk/fsroot/ramdisk/HOST_RPC/lib/dsg_rpc_cm2h
LD_FLAGS += -L${RDK_DIR}/sdk/fsroot/ramdisk/HOST_RPC/lib/dsg_rpc_h2cm
LD_FLAGS += -L${RDK_DIR}/sdk/fsroot/ramdisk/HOST_RPC/lib/htx_dsg_lib
endif

LD_FLAGS+=$(OPENSRC_LIBS)

SNMP_SUBAGENT_LFLAGS = -L$(OPENSRC_PATH)/target-snmp/lib  -lnetsnmpmibs -lnetsnmpagent -lnetsnmphelpers -lnetsnmp
LD_FLAGS+= $(SNMP_SUBAGENT_LFLAGS)

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

ifeq ($(USE_SYS_RESOURCE_MLT), y)
ifeq ($(RDK_PLATFORM_SOC),broadcom)
SYSRESOURCE_LIBS = -L $(SYSRES_DIR)/sys_resource/lib/platform/broadcom -lsysResource
else
SYSRESOURCE_LIBS = -L $(SYSRES_DIR)/sys_resource/lib/platform/canmore -lsysResource
endif
endif

ifeq ($(USE_SYS_RESOURCE_MON), y)
ifeq ($(RDK_PLATFORM_SOC),broadcom)
SYSRESOURCE_LIBS = -L $(SYSRES_DIR)/sys_resource/lib/platform/broadcom -lsysResource
else
SYSRESOURCE_LIBS = -L $(SYSRES_DIR)/sys_resource/lib/platform/canmore -lsysResource
endif
endif

WHOLE_LIBS=$(OPENSRC_LIBS) $(OPENSSL_LIBS) $(PLATFORM_SHLIBS) $(SOC_SHLIBS) $(SNMPSHLIBS) $(XG5_SHLIBS)

LD_FLAGS+= $(WHOLE_LIBS)
LD_FLAGS+= $(SYSRESOURCE_LIBS)

LD_FLAGS+=  $(INTERNAL_LIBS)  -L$(BUILD_DIR)/lib -lipcutils -lsnmpmanager \
		-lhalsnmp -lsnmpinterface \
		-lipcclient

SRC_DIR=.
LIBNAME=runSnmp
LIBFILE=$(LIBNAME)
OBJ_DIR=$(BUILD_DIR)/objs_$(LIBNAME)

CFLAGS+=-DCMCST_EXT_X1

ifdef INCLUDE_BREAKPAD
 INCLUDES+= -I $(OPENSOURCE_SRC_DIR)/google-breakpad/src
 INCLUDES+= -I $(OPENSOURCE_SRC_DIR)/../include/breakpad
 LD_FLAGS+= -lbreakpad_client
endif

CFLAGS+= $(INCLUDES)
OBJS+=$(OBJ_DIR)/runSnmp.o \
	
.PHONY : all clean
all: $(LIBFILE)


$(LIBFILE): $(LIBDIR) $(OBJ_DIR) $(OBJS) 
	$(CC) -o $(LIBFILE) $(OBJS) $(OPENSSL_SRC_DIR)/libcrypto.a -L$(RDK_DIR)/rdklogger/build/lib -lrdkloggers -lrmfosalutils -lrmfosal -llog4c -Wl,--start-group $(LD_FLAGS) -Wl,--end-group $(WHOLE_LIB_PATH) $(SYSROOT_FLAG)
	cp $(LIBNAME) $(OBJ_DIR)


$(OBJ_DIR)/%.o :$(SRC_DIR)/%.cpp
	$(CC) -c -o $@ $< $(CFLAGS)
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(LIBDIR):
	mkdir -p $(LIBDIR)

clean :
	rm -rf $(OBJ_DIR) $(LIBFILE) *~
