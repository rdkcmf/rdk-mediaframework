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

AUTOMAKE_OPTIONS = subdir-objects

SUBDIRS = 
LOCAL_DEFINES =   -DMERGE_INTEL \
                  -DENABLE_NEW_LOGSVR \
                  -DCMCST_EXT_X1 \
                  -DUSE_DSG \
                  -DUSE_CDL \
                  -DUSE_MFR \
                  -DUSE_TUNER \
                  -DNV_RAM_READ \
                  -DINTEL_PR13 \
                  -DINTEL_PR18 \
                  -DUSE_VIDSINK_API \
                  -g -O2 -Wall \
                  -fno-short-enums \
                  -ffloat-store \
                  -fno-strict-aliasing \
                  -DHAVE_STDINT_H \
                  -D_GNU_SOURCE \
                  -DMPE_FEATURE_DEBUG \
                  -DMPE_MEM_HANDLES \
                  -DRMF_OSAL_LITTLE_ENDIAN \
                  -DJGFX_DEV \
                  -D$(RMF_BUILD_OS) \
                  -D$(POD_PLATFORM) \
                  -DLINUX \
                  -DUSE_CC_GETTIMEOFDAY \
                  -DGCC4_XXX \
                  -DNUM_SURF=1 \
                  -DDEPRECATED_INTERFACE_SUPPORT \
                  -DUSE_NEW_MSPOD_DRIVER \
                  -DUSE_CDL_SERVICE \
                  -DCOMMON_DOWNLOAD \
                  -DSNMP_IPC_CLIENT  \
                  -D$(MFR_NAME) \
                  -DIPPV_CLIENT_ENABLED  \
                  -DUSE_PXYSRVC 

if ENABLE_PODMGR
LOCAL_DEFINES += -DUSE_POD -DMPOD_SUPPORT -DTEST_WITH_PODMGR -DUSE_POD_IPC
endif

if HEADLESS
LOCAL_DEFINES+= -DHEADLESS_GW
else
LOCAL_DEFINES+= -DHEADED_GW
endif

AM_CXXFLAGS = -pthread -Wall -Wno-format -Wunused $(GST_CFLAGS) $(LOCAL_DEFINES) $(YOCTO_FLAG) \
              -I${top_srcdir}/qamsource/podmgr/canh \
              -I${top_srcdir}/osal/include \
              -I${top_srcdir}/qamsource/podmgr/inc \
              -I${top_srcdir}/qamsource/podmgr/podserver/cardmanager/scm_pod_stack/cacpPriv \
              -I${top_srcdir}/qamsource/simgr/inband/include \
              -I${top_srcdir}/qamsource/common \
              -I${top_srcdir}/qamsource/main \
              -I${top_srcdir}/core \
              -I${top_srcdir}/qamsource/oobsimgrstub \
              -I${top_srcdir}/snmp/ipcclient \
              -I${top_srcdir}/qamsource/podmgr/podserver/inc \
              -I${top_srcdir}/qamsource/podmgr/podserver/commonDownloadManager \
              -I${top_srcdir}/snmp/ipcutils/utils \
              -I${top_srcdir}/snmp/snmpmanager \
              -I${top_srcdir}/osal/utils/inc \
              -I${top_srcdir}/osal/include  \
              -I${top_srcdir}/qamsource/podmgr/podserver/cardmanager \
              -I${top_srcdir}/platform/soc/halsnmp \
              -I${top_srcdir}/snmp/snmpinterface \
              -I${top_srcdir}/qamsource/podmgr/podserver/cardmanager/scm_pod_stack/diag \
              -I${top_srcdir}/../snmpinterface \
              -I${top_srcdir}/../ipcclient \
              -I${top_srcdir}/../ipcutils/utils

lib_LTLIBRARIES = libsnmpmanager.la
if ENABLE_YOCTO
libsnmpmanager_la_LDFLAGS = $(GST_LIBS)  -lrdkloggers -llog4c -lnetsnmp -lsecure_wrapper
else
libsnmpmanager_la_LDFLAGS = $(GST_LIBS)  -lrdkloggers -llog4c -lnetsnmp
endif
libsnmpmanager_la_LIBADD = ${top_builddir}/snmp/ipcutils/libipcutils.la \
                           ${top_builddir}/snmp/snmpinterface/libsnmpinterface.la
libsnmpmanager_la_SOURCES = system.cpp \
			    ocStbHostAVInterfaceTable.cpp \
			    ocStbHostInBandTunerTable.cpp \
			    ocStbHostDVIHDMITable.cpp \
			    ocStbHostComponentVideoTable.cpp \
			    ocStbHostRFChannelOutTable.cpp \
			    ocStbHostAnalogVideoTable.cpp \
			    ocStbHostProgramStatusTable.cpp \
			    ocStbHostMpeg2ContentTable.cpp \
			    ocStbHostMpeg4ContentTable.cpp \
			    ocStbHostVc1ContentTable.cpp \
			    vl_ocStbHost_GetData.cpp \
			    AvInterfaceGetdata.cpp \
			    ocStbHostSPDIfTable.cpp \
			    ocStbHostIEEE1394Table.cpp \
			    ocStbHostIEEE1394ConnectedDevicesTable.cpp \
			    ocStbHostSoftwareApplicationInfoTable.cpp \
			    ocStbHostSystemHomeNetworkTable.cpp \
			    ocStbHostSystemMemoryReportTable.cpp \
			    ocStbHostSystemTempTable.cpp \
			    ocStbHostCCAppInfoTable.cpp \
			    ifTable.cpp \
			    ipNetToPhysicalTable.cpp \
			    vividlogicmib.cpp \
			    tablevividlogicmib.cpp \
			    docsDevDateTime.cpp \
			    docsDevEvControl.cpp \
			    docsEventServer.cpp \
			    cardMibAccessSyncHandler.cpp \
			    vlSnmpClient.cpp \
			    vlSnmpEcmApi.cpp \
			    snmpCommunityTable.cpp \
			    snmpTargetAddrExtTable.cpp \
			    snmpTargetAddrTable.cpp \
			    vacmSecurityToGroupTable.cpp \
			    vacmAccessTable.cpp \
			    vacmViewTreeFamilyTable.cpp \
			    ocStbHostSpecificationsInfo.cpp \
			    TlvConfigObjs.cpp \
			    Tlvevent.cpp \
			    ocStbPanicDumpTrap.cpp \
			    snmpNotifyTable.cpp \
			    snmpTargetParamsTable.cpp \
			    host_scalar.cpp \
			    sysORTable.cpp \
			    vlSnmpBootEventHandler.cpp \
			    ocStbHostContentErrorSummaryInfo.cpp \
			    ocStbHostDVIHDMIAvailableVideoFormatTable.cpp	\
			    proxymib_GetData.cpp	\
			    xcaliburClient.cpp \
 			    stbHostUtils.cpp \
			    stbHostUtils_GetData.cpp

if PODMGR_STUB_CODE
libsnmpmanager_la_SOURCES += ../snmpmanager_stub/ocStbHostMibModule.cpp \
                             ../snmpmanager_stub/docsDevEventTableGen.cpp \
                             ../snmpmanager_stub/ocStbHostDumpTrapInfoGen.cpp \
                             ../snmpmanager_stub/snmp_hostGen.cpp \
                             ../snmpmanager_stub/snmpmanagerGen.cpp \
                             ../snmpmanager_stub/docsDevEvControlTableGen.cpp \
                             ../snmpmanager_stub/vlSnmpHostInfoGen.cpp
else
if !ENABLE_YOCTO
libsnmpmanager_la_SOURCES += ../snmpmanager_priv/ocStbHostMibModule.cpp \
                             ../snmpmanager_priv/docsDevEventTable.cpp \
                             ../snmpmanager_priv/ocStbHostDumpTrapInfo.cpp \
                             ../snmpmanager_priv/snmp_host.cpp \
                             ../snmpmanager_priv/snmpmanager.cpp \
                             ../snmpmanager_priv/docsDevEvControlTable.cpp \
                             ../snmpmanager_priv/vlSnmpHostInfo.cpp
endif
endif

EXTRA_DIST = AvInterfaceGetdata.h \
             ocStbHostContentErrorSummaryInfo.h \
             snmpNotifyTable.h \
             vacmViewTreeFamilyTable.h \
             cardMibAccessProxyServer.h  \
             ocStbHostDumpTrapInfo.h \
             snmpTargetAddrExtTable.h \
             vividlogicmib.h \
             docsDevDateTime.h \
             ocStbHostMibModule.h \
             snmpTargetAddrTable.h \
             vl_ocStbHost_GetData.h \
             docsDevEvControl.h \
             ocStbHostSpecificationsInfo.h \
             snmpTargetParamsTable.h \
             vlSnmpBootEventHandler.h \
             docsDevEvControlTable.h \
             ocStbPanicDumpTrap.h \
             sysORTable.h \
             vlSnmpClient.h \
             docsDevEventTable_enums.h \
             proxymib_GetData.h \
             system.h \
             vlSnmpEcmApi.h \
             docsDevEventTable.h \
             snmpAccessInclude.h \
             tablevividlogicmib.h \
             vlSnmpHostInfo.h \
             host_enums.h \
             snmpCommunityTable.h \
             TlvConfigObjs.h \
             xcaliburClient.h \
             host_scalar.h \
             snmp_host.h \
             Tlvevent.h \
             ifTable.h \
             SnmpIORM.h \
             vacmAccessTable.h \
             ipNetToPhysicalTable.h \
             snmpmanager.h \
             vacmSecurityToGroupTable.h

libsnmpmanager_la_includedir = $(includedir)
libsnmpmanager_la_include_HEADERS = $(EXTRA_DIST)
