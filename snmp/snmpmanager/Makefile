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
		-I${RMF_DIR}/snmp/snmpmanager


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

CFLAGS += $(PLATFORM_FLAGS)

LD_FLAGS+= -L$(BUILD_DIR)/lib -lipcutils \
		-lhalsnmp -lsnmpinterface \
		-lipcclient


SRC_DIR=.
LIBNAME=snmpmanager
LIBFILE=lib$(LIBNAME).so
OBJ_DIR=$(BUILD_DIR)/objs_$(LIBNAME)

CFLAGS+= $(INCLUDES)

OBJS=$(OBJ_DIR)/snmpmanager.o \
	$(OBJ_DIR)/ocStbHostMibModule.o \
        $(OBJ_DIR)/system.o \
        $(OBJ_DIR)/ocStbHostAVInterfaceTable.o \
        $(OBJ_DIR)/ocStbHostInBandTunerTable.o \
	$(OBJ_DIR)/ocStbHostDVIHDMITable.o \
	$(OBJ_DIR)/ocStbHostComponentVideoTable.o\
	$(OBJ_DIR)/ocStbHostRFChannelOutTable.o \
	$(OBJ_DIR)/ocStbHostAnalogVideoTable.o \
	$(OBJ_DIR)/ocStbHostProgramStatusTable.o \
	$(OBJ_DIR)/ocStbHostMpeg2ContentTable.o \
        $(OBJ_DIR)/vl_ocStbHost_GetData.o \
        $(OBJ_DIR)/vlSnmpHostInfo.o \
        $(OBJ_DIR)/AvInterfaceGetdata.o \
	$(OBJ_DIR)/ocStbHostSPDIfTable.o \
        $(OBJ_DIR)/ocStbHostIEEE1394Table.o \
        $(OBJ_DIR)/ocStbHostIEEE1394ConnectedDevicesTable.o\
        $(OBJ_DIR)/ocStbHostSoftwareApplicationInfoTable.o \
        $(OBJ_DIR)/ocStbHostSystemHomeNetworkTable.o \
        $(OBJ_DIR)/ocStbHostSystemMemoryReportTable.o \
        $(OBJ_DIR)/ocStbHostSystemTempTable.o \
        $(OBJ_DIR)/ocStbHostCCAppInfoTable.o \
        $(OBJ_DIR)/ifTable.o \
        $(OBJ_DIR)/ipNetToPhysicalTable.o \
        $(OBJ_DIR)/vividlogicmib.o \
        $(OBJ_DIR)/tablevividlogicmib.o \
        $(OBJ_DIR)/docsDevDateTime.o \
        $(OBJ_DIR)/docsDevEvControl.o \
        $(OBJ_DIR)/docsDevEvControlTable.o \
        $(OBJ_DIR)/docsDevEventTable.o \
        $(OBJ_DIR)/docsEventServer.o \
        $(OBJ_DIR)/cardMibAccessSyncHandler.o \
        $(OBJ_DIR)/vlSnmpClient.o \
        $(OBJ_DIR)/vlSnmpEcmApi.o \
        $(OBJ_DIR)/snmpCommunityTable.o \
        $(OBJ_DIR)/snmpTargetAddrExtTable.o \
        $(OBJ_DIR)/snmpTargetAddrTable.o \
        $(OBJ_DIR)/vacmSecurityToGroupTable.o \
        $(OBJ_DIR)/vacmAccessTable.o \
        $(OBJ_DIR)/vacmViewTreeFamilyTable.o \
        $(OBJ_DIR)/ocStbHostSpecificationsInfo.o \
        $(OBJ_DIR)/TlvConfigObjs.o \
        $(OBJ_DIR)/Tlvevent.o \
        $(OBJ_DIR)/ocStbHostDumpTrapInfo.o \
        $(OBJ_DIR)/ocStbPanicDumpTrap.o \
        $(OBJ_DIR)/snmpNotifyTable.o \
        $(OBJ_DIR)/snmpTargetParamsTable.o \
        $(OBJ_DIR)/host_scalar.o \
        $(OBJ_DIR)/snmp_host.o \
        $(OBJ_DIR)/sysORTable.o \
        $(OBJ_DIR)/vlSnmpBootEventHandler.o \
        $(OBJ_DIR)/ocStbHostContentErrorSummaryInfo.o \
        $(OBJ_DIR)/ocStbHostDVIHDMIAvailableVideoFormatTable.o	\
        $(OBJ_DIR)/proxymib_GetData.o	\
        $(OBJ_DIR)/xcaliburClient.o     \
        $(OBJ_DIR)/stbHostUtils.o	\
        $(OBJ_DIR)/stbHostUtils_GetData.o



.PHONY : all clean
all: $(LIBFILE)

$(LIBFILE): $(OBJ_DIR) $(OBJS)
	$(CC) $(OBJS) -shared -o $(LIBFILE) $(LD_FLAGS)
	cp $(LIBFILE) $(OBJ_DIR)
	cp $(LIBFILE) $(BUILD_DIR)/lib/

$(OBJ_DIR)/%.o :$(SRC_DIR)/%.cpp
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean :
	rm -rf $(OBJ_DIR) $(LIBFILE) *~

