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

SUBDIRS = 

LOCAL_DEFINES =   -DMERGE_INTEL \
                  -DENABLE_NEW_LOGSVR \
                  -DCMCST_EXT_X1 \
                  -DUSE_POD \
                  -DUSE_DSG \
                  -DUSE_CDL \
                  -DMPOD_SUPPORT \
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
                  -DTEST_WITH_PODMGR \
                  -DCOMMON_DOWNLOAD \
                  -DSNMP_IPC_CLIENT  \
                  -D$(MFR_NAME) \
		  -DHEADED_GW  \
                  -DIPPV_CLIENT_ENABLED  \
                  -DUSE_POD_IPC  \
		  -DUSE_PXYSRVC \
		  -DGLI \
	          -DHAVE_STDINT_H \
	          -DVL_USE_MEMORY_POOL_FOR_POD_WRITE

LOCAL_DEFINES += -DUSE_IPDIRECT_UNICAST


AM_CXXFLAGS = -pthread -Wall -Wno-format -Wunused $(GST_CFLAGS) $(OPENSSL_CFLAGS) $(LOCAL_DEFINES) \
	      -I${top_srcdir}/qamsource/podmgr/canh \
	      -I${top_srcdir}/canh \
	      -I${top_srcdir}/osal/include \
	      -I${top_srcdir}/qamsource/podmgr/inc \
              -I${top_srcdir}/podserver/inc \
	      -I${top_srcdir}/inc \
	      -I${top_srcdir}/qamsource/simgr/inband/include \
	      -I${top_srcdir}/qamsource/common \
	      -I${top_srcdir}/qamsource/main \
	      -I${top_srcdir}/core \
	      -I${top_srcdir}/qamsource/oobsimgrstub \
	      -I${top_srcdir}/qamsource/podmgr/podserver/inc \
	      -I${top_srcdir}/inc \
	      -I${top_srcdir}/qamsource/podmgr/podserver/cardmanager/scm_pod_stack/include \
	      -I${top_srcdir}/podserver/cardmanager/scm_pod_stack/include \
	      -I${top_srcdir}/cardmanager/scm_pod_stack/include \
	      -I${top_srcdir}/qamsource/podmgr/podserver/cardmanager/scm_pod_stack/appinfo \
	      -I${top_srcdir}/podserver/cardmanager/scm_pod_stack/appinfo \
	      -I${top_srcdir}/cardmanager/scm_pod_stack/appinfo \
	      -I${top_srcdir}/qamsource/podmgr/podserver/utils \
	      -I${top_srcdir}/podserver/utils \
	      -I${top_srcdir}/utils \
	      -I${top_srcdir}/snmp/snmpmanager \
	      -I${top_srcdir}/osal/utils/inc \
	      -I${top_srcdir}/qamsource/podmgr/podserver/cardmanager/scm_pod_stack/cardres \
	      -I${top_srcdir}/podserver/cardmanager/scm_pod_stack/cardres \
	      -I${top_srcdir}/cardmanager/scm_pod_stack/cardres \
	      -I${top_srcdir}/qamsource/podmgr/podserver/halbase \
	      -I${top_srcdir}/halbase \
	      -I${top_srcdir}/qamsource/podmgr/podserver/cardmanager \
	      -I${top_srcdir}/podserver/cardmanager \
	      -I${top_srcdir}/cardmanager \
	      -I${top_srcdir}/qamsource/podmgr/podserver/cardmanager/scm_pod_stack/dsg \
	      -I${top_srcdir}/podserver/cardmanager/scm_pod_stack/dsg \
	      -I${top_srcdir}/cardmanager/scm_pod_stack/dsg \
	      -I${top_srcdir}/qamsource/podmgr/podserver/cardmanager/scm_pod_stack/ipdirectUnicast \
	      -I${top_srcdir}/podserver/cardmanager/scm_pod_stack/ipdirectUnicast \
	      -I${top_srcdir}/cardmanager/scm_pod_stack/ipdirectUnicast \
	      -I${top_srcdir}/qamsource/podmgr/podserver/cardmanager/scm_pod_stack/mmi \
	      -I${top_srcdir}/podserver/cardmanager/scm_pod_stack/mmi \
	      -I${top_srcdir}/cardmanager/scm_pod_stack/mmi \
	      -I${top_srcdir}/qamsource/simgr/oob/include \
	      -I${top_srcdir}/qamsource/podmgr/podserver/commonDownloadManager \
	      -I${top_srcdir}/podserver/commonDownloadManager \
	      -I${top_srcdir}/commonDownloadManager \
	      -I${top_srcdir}/qamsource/podmgr/podserver/cardmanager/scm_pod_stack/diag  \
	      -I${top_srcdir}/podserver/cardmanager/scm_pod_stack/diag \
	      -I${top_srcdir}/cardmanager/scm_pod_stack/diag \
	      -I${top_srcdir}/qamsource/podmgr/podserver/cardmanager/scm_pod_stack/system \
	      -I${top_srcdir}/podserver/cardmanager/scm_pod_stack/system \
	      -I${top_srcdir}/cardmanager/scm_pod_stack/system \
	      -I${top_srcdir}/qamsource/podmgr/podserver/src \
	      -I${top_srcdir}/podserver/src \
	      -I${top_srcdir}/src 

if PODMGR_STUB_CODE
AM_CXXFLAGS += -I${top_srcdir}/qamsource/podmgr/podserver/cardmanager/scm_pod_stack/cacpstub \
	       -I${top_srcdir}/podserver/cardmanager/scm_pod_stack/cacpstub \
	       -I${top_srcdir}/cardmanager/scm_pod_stack/cacpstub
else
AM_CXXFLAGS += -I${top_srcdir}/qamsource/podmgr/podserver/cardmanager/scm_pod_stack/cacpPriv \
	       -I${top_srcdir}/podserver/cardmanager/scm_pod_stack/cacpPriv \
	       -I${top_srcdir}/cardmanager/scm_pod_stack/cacpPriv \
	       -I${top_srcdir}/qamsource/podmgr/podserver/cardmanager/scm_pod_stack/cacpPriv/cprot \
	       -I${top_srcdir}/podserver/cardmanager/scm_pod_stack/cacpPriv/cprot \
	       -I${top_srcdir}/cardmanager/scm_pod_stack/cacpPriv/cprot

endif


AM_CFLAGS = $(AM_CXXFLAGS) -x c++

lib_LTLIBRARIES = libpodserver.la
libpodserver_la_LDFLAGS = $(GST_LIBS)  -L${FSROOT}/usr/lib -L${FSROOT}/usr/local/lib -lrdkloggers -llog4c $(OPENSSL_LDFLAGS) -lcurl

libpodserver_la_SOURCES = utils/sys_error_string.c \
			  utils/buf_parse_utils.c \
			  utils/net_utils.c \
                          utils/rmfStaticMemoryPool.c \
			  cardmanager/scm_pod_stack/utils.c \
			  cardmanager/scm_pod_stack/ci.c \
			  cardmanager/scm_pod_stack/appinfo/appinfo-main.c \
			  cardmanager/scm_pod_stack/appinfo/ai_state_machine.c \
			  cardmanager/scm_pod_stack/appinfo/apdu_appinfo.c \
			  cardmanager/scm_pod_stack/systime/apdu_systime.c \
			  cardmanager/scm_pod_stack/systime/systime-main.c \
			  cardmanager/scm_pod_stack/transport.c \
			  cardmanager/scm_pod_stack/host/apdu_host.c \
			  cardmanager/scm_pod_stack/host/host-main.c \
			  cardmanager/scm_pod_stack/podlowapi.c \
			  cardmanager/scm_pod_stack/hostAddPrt/hostAddPrt-main.c \
			  cardmanager/scm_pod_stack/hostAddPrt/apdu_hostAddPrt.c \
			  cardmanager/scm_pod_stack/session.c \
			  cardmanager/scm_pod_stack/sas/sas-main.c \
			  cardmanager/scm_pod_stack/sas/apdu_sas.c \
			  cardmanager/scm_pod_stack/feature/feature-main.c \
			  cardmanager/scm_pod_stack/feature/apdu_feature.c \
			  cardmanager/scm_pod_stack/headendCommunication/HeadEndComm-main.c \
			  cardmanager/scm_pod_stack/cardres/cardres-main.c \
			  cardmanager/scm_pod_stack/cardres/apdu_cardres.c \
			  cardmanager/scm_pod_stack/homing/cs_queue_manager.c \
			  cardmanager/scm_pod_stack/homing/apdu_homing.c \
			  cardmanager/scm_pod_stack/homing/homing-main.c \
			  cardmanager/scm_pod_stack/homing/cs_state_machine.c \
			  cardmanager/scm_pod_stack/homing/homing_sm.c \
			  cardmanager/scm_pod_stack/podhighapi.c \
			  cardmanager/scm_pod_stack/dsg/vlDsgClientController.c \
			  cardmanager/scm_pod_stack/dsg/vlDsgObjectPrinter.c \
			  cardmanager/scm_pod_stack/dsg/vlDsgParser.c \
			  cardmanager/scm_pod_stack/dsg/dsg-main.c \
			  cardmanager/scm_pod_stack/dsg/vlDsgTunnelParser.c \
			  cardmanager/scm_pod_stack/dsg/vlDsgDispatchToClients.c \
			  cardmanager/scm_pod_stack/ipdirectUnicast/ipdirectUnicast-main.c \
			  cardmanager/scm_pod_stack/ipdirectUnicast/httpFileFetcher.cpp \
			  cardmanager/scm_pod_stack/ipdirectUnicast/IPDdownloadManager.cpp \
			  cardmanager/scm_pod_stack/ipdirectUnicast/IPDSocketManager.cpp \
			  cardmanager/scm_pod_stack/applitest.c \
			  cardmanager/scm_pod_stack/xchan/xchan-main.c \
			  cardmanager/scm_pod_stack/xchan/xchan_interface.c \
			  cardmanager/scm_pod_stack/xchan/apdu_xchan.c \
			  cardmanager/scm_pod_stack/rsmgr/rsmgr-main.c \
			  cardmanager/scm_pod_stack/rsmgr/apdu_rsmgr.c \
			  cardmanager/scm_pod_stack/mmi/apdu_mmi.c \
			  cardmanager/scm_pod_stack/mmi/mmi-main.c \
			  cardmanager/scm_pod_stack/system/system-main.c \
			  cardmanager/scm_pod_stack/diag/diag-main.c \
			  cardmanager/scm_pod_stack/cardMibAcc/apdu_cardMibAcc.c \
			  cardmanager/scm_pod_stack/cardMibAcc/cardMibAcc-main.c \
			  cardmanager/scm_pod_stack/appmgr.c \
			  cardmanager/scm_pod_stack/lcsm_log.c \
			  cardmanager/vlXchanFlowManager.c \
			  src/podmgrImpl.c \
			  src/pod_CAPMT_APDUCreate.c \
			  src/podmgr.c \
			  src/pod.c \
			  commonDownloadManager/vlMpeosCDLManagerIf.cpp \
			  commonDownloadManager/ccodeimagevalidator.cpp \
			  commonDownloadManager/tablecvt.cpp \
			  commonDownloadManager/bits_cdl.cpp \
			  commonDownloadManager/cdownloadcvtvalidationmgr.cpp \
			  commonDownloadManager/tablecvt_t1v1.cpp \
			  commonDownloadManager/tablecvt_t2v1.cpp \
			  commonDownloadManager/CommonDownloadMgr.cpp \
			  utils/vector.cpp \
			  utils/cmhash.cpp \
			  utils/vlEnv.cpp \
			  utils/cmThreadBase.cpp \
			  utils/list.cpp \
			  cardmanager/poddriver.cpp \
			  cardmanager/tunerdriver.cpp \
			  cardmanager/dsg.cpp \
			  cardmanager/ipdirect_unicast.cpp \
			  cardmanager/hostAddPrt.cpp \
			  cardmanager/cp_prot.cpp \
			  cardmanager/ca.cpp \
			  cardmanager/capmtparse.cpp \
			  cardmanager/host.cpp \
			  cardmanager/RsMgr.cpp \
			  cardmanager/gen_diag.cpp \
			  cardmanager/mmi.cpp \
			  cardmanager/oob_rdc.cpp \
			  cardmanager/cardmanager.cpp \
			  cardmanager/scm_pod_stack/diag/hostInfo.cpp \
			  cardmanager/genfeature.cpp \
			  cardmanager/display.cpp \
			  cardmanager/cardMibAcc.cpp \
			  cardmanager/cardres.cpp \
			  cardmanager/vlMpeosCardManagerIf.cpp \
			  cardmanager/headendComm.cpp \
			  cardmanager/syscontrol.cpp \
			  cardmanager/homing.cpp \
			  cardmanager/systime.cpp \
			  cardmanager/sas.cpp \
			  cardmanager/mmi_handler.cpp \
			  cardmanager/xchan.cpp \
			  cardmanager/cmevents.cpp \
			  cardmanager/podmain.cpp \
			  cardmanager/bits.cpp \
			  cardmanager/vlCardCaPmt.cpp \
			  halbase/cablecarddriver.cpp \
			  src/pfc_depend.cpp \
			  src/persistent.cpp \
			  src/podServer.cpp \
			  src/vlCCIInfo.cpp 
if PODMGR_STUB_CODE
libpodserver_la_SOURCES +=cardmanager/scm_pod_stack/cacpstub/cacp.c \
                          cardmanager/scm_pod_stack/cacpstub/apdu_systemGen.c \
                          cardmanager/scm_pod_stack/cacpstub/CommonDownloadGenericNV.cpp \
                          cardmanager/scm_pod_stack/cacpstub/apdu_diagGen.c \
                          cardmanager/scm_pod_stack/cacpstub/appinfoGen.cpp \
                          cardmanager/scm_pod_stack/cacpstub/dsgProxyServiceGen.cpp \
                          cardmanager/scm_pod_stack/cacpstub/hostGenericFeatures.cpp \
			  cardmanager/scm_pod_stack/cacpstub/cm_apiGen.cpp \
                          cardmanager/scm_pod_stack/cacpstub/initmodules.c
else
if !ENABLE_YOCTO
libpodserver_la_SOURCES +=cardmanager/scm_pod_stack/cacpPriv/ca/apdu_ca.c \
			  cardmanager/scm_pod_stack/cacpPriv/ca/ca-main.c \
			  cardmanager/scm_pod_stack/cacpPriv/cm_api.cpp \
			  cardmanager/scm_pod_stack/cacpPriv/cprot/cp_data.c \
			  cardmanager/scm_pod_stack/cacpPriv/cprot/cp_auth.c \
			  cardmanager/scm_pod_stack/cacpPriv/cprot/apps.c \
			  cardmanager/scm_pod_stack/cacpPriv/cprot/cp_file.c \
			  cardmanager/scm_pod_stack/cacpPriv/cprot/cp_proc.c \
			  cardmanager/scm_pod_stack/cacpPriv/cprot/apdu_cprot.c \
			  cardmanager/scm_pod_stack/cacpPriv/cprot/cp_certdata.c \
			  cardmanager/scm_pod_stack/cacpPriv/cprot/cp_test.c \
			  cardmanager/scm_pod_stack/cacpPriv/cprot/cprot-main.c \
                          cardmanager/scm_pod_stack/cacpPriv/init.c \
			  cardmanager/scm_pod_stack/cacpPriv/apdu_system.c \
			  cardmanager/scm_pod_stack/cacpPriv/apdu_diag.c \
			  cardmanager/scm_pod_stack/cacpPriv/appinfo.cpp \
			  cardmanager/scm_pod_stack/cacpPriv/vlDsgProxyService.cpp \
			  cardmanager/scm_pod_stack/cacpPriv/vlHostGenericFeatures.cpp \
            		  cardmanager/scm_pod_stack/cacpPriv/vl_mfr_writer.c \
			  cardmanager/scm_pod_stack/cacpPriv/CommonDownloadNVDataAccess.cpp
endif
endif

EXTRA_DIST = inc/AvOutInterfaceDefs.h \
             inc/coreUtilityApi.h \
             inc/hostGenericFeatures.h  \
             inc/podimpl.h	     \
             inc/sys_init.h \
             inc/bufParseUtilities.h \
             inc/diagMsgEvent.h \
             inc/hutils_sys.h \
             inc/podmgr.h \
             inc/traceStack.h \
             inc/cardManagerIf.h \
             inc/dsgClientController.h  \
             inc/ip_types.h \
             inc/xchanResApi.h \
             inc/cardUtils.h \
             inc/dsgProxyTypes.h  \
             inc/pod_types.h \
             inc/cm_api.h \
             inc/dsgResApi.h	\
             inc/netUtils.h \
             inc/cmevents.h \
             inc/dsg_types.h \
             inc/ipdirect_unicast_types.h \
             inc/persistentconfig.h \
             inc/snmpManagerIf.h \
             inc/core_events.h \
             inc/genfeature.h \
             inc/podIf.h \
             inc/snmp_types.h \
	     utils/cmhash.h \
             utils/cmThreadBase.h  \
             utils/cVector.h  \
             utils/rmfStaticMemoryPool.h \
             utils/si_util.h	\
             utils/utilityMacros.h  \
             utils/vlEnv.h \
             cardmanager/appinfo.h \
             cardmanager/dsg.h \
             cardmanager/ipdirect_unicast.h \
             cardmanager/mmi_handler.h  \
             cardmanager/syscontrol.h \
             cardmanager/ca.h	   \
             cardmanager/gen_diag.h     \
             cardmanager/oob.h		 \
             cardmanager/systime.h \
             cardmanager/capmtparse.h   \
             cardmanager/headendComm.h  \
             cardmanager/oob_rdc.h	 \
             cardmanager/tunerdriver.h \
             cardmanager/cardmanager.h  \
             cardmanager/homing.h       \
             cardmanager/poddriver.h	 \
             cardmanager/vlCardCaPmt.h \
             cardmanager/cardMibAcc.h   \
             cardmanager/hostAddPrt.h   \
             cardmanager/podmain.h	 \
             cardmanager/vlXchanFlowManager.h \
             cardmanager/cardres.h	   \
             cardmanager/host.h	      \
             cardmanager/RsMgr.h	 \
             cardmanager/xchan.h \
             cardmanager/cp_prot.h	   \
             cardmanager/mmi.h	      \
             cardmanager/sas.h 

#if PODMGR_STUB_CODE
#else
#EXTRA_DIST += cardmanager/scm_pod_stack/cacpPriv/vl_mfr_writer.c
#endif

EXTRA_DIST += cardmanager/scm_pod_stack/include/applitest.h \
             cardmanager/scm_pod_stack/include/nvm_apis.h \
             cardmanager/scm_pod_stack/include/appmgr.h		 \
             cardmanager/scm_pod_stack/include/ob_qpsk.h \
             cardmanager/scm_pod_stack/include/cardmanagergentypes.h  \
             cardmanager/scm_pod_stack/include/podapi.h \
             cardmanager/scm_pod_stack/include/cci_mgr.h		 \
             cardmanager/scm_pod_stack/include/poddef.h \
             cardmanager/scm_pod_stack/include/ci.h			 \
             cardmanager/scm_pod_stack/include/poddrv.h \
             cardmanager/scm_pod_stack/include/crypto_user.h		 \
             cardmanager/scm_pod_stack/include/poderror.h \
             cardmanager/scm_pod_stack/include/cs_queue_manager.h	 \
             cardmanager/scm_pod_stack/include/podhighapi.h \
             cardmanager/scm_pod_stack/include/cs_state_machine.h	 \
             cardmanager/scm_pod_stack/include/podlowapi.h \
             cardmanager/scm_pod_stack/include/global_event_map.h	 \
             cardmanager/scm_pod_stack/include/pod_support.h \
             cardmanager/scm_pod_stack/include/global_queues.h	 \
             cardmanager/scm_pod_stack/include/resourcetable.h \
             cardmanager/scm_pod_stack/include/hal_pod_api.h		 \
             cardmanager/scm_pod_stack/include/session.h \
             cardmanager/scm_pod_stack/include/lcsm_log.h		 \
             cardmanager/scm_pod_stack/include/transport.h \
             cardmanager/scm_pod_stack/include/link.h		 \
             cardmanager/scm_pod_stack/include/tsdecode.h \
             cardmanager/scm_pod_stack/include/mrerrno.h		 \
             cardmanager/scm_pod_stack/include/utils.h \
             cardmanager/scm_pod_stack/include/mr.h \
             halbase/cablecarddriver.h  \
             halbase/dsgdriver.h \
             commonDownloadManager/bits_cdl.h \
             commonDownloadManager/cvtdownloadtypes.h \
             commonDownloadManager/ccodeimagevalidator.h \
             commonDownloadManager/cvtfilterdata.h \
             commonDownloadManager/cdl_mgr_types.h \
             commonDownloadManager/DownloadEngine.h \
             commonDownloadManager/cdownloadcvtvalidationmgr.h \
             commonDownloadManager/pkcs7SinedDataInfo.h \
             commonDownloadManager/CommonDownloadDeferredDLMonitor.h  \
             commonDownloadManager/pkcs7utils.h \
             commonDownloadManager/CommonDownloadImageUpdate.h \
             commonDownloadManager/tablecvt.h \
             commonDownloadManager/CommonDownloadMgr.h \
             commonDownloadManager/tablecvt_t1v1.h \
             commonDownloadManager/CommonDownloadNVDataAccess.h	 \
             commonDownloadManager/tablecvt_t2v1.h \
             commonDownloadManager/CommonDownloadSnmpInf.h \
             commonDownloadManager/vlMpeosCdlIf.h \
             commonDownloadManager/CommonDownloadTriggerNotifier.h \
             commonDownloadManager/vlMpeosCDLManagerIf.h \
             cardmanager/scm_pod_stack/diag/diag-main.h  \
             cardmanager/scm_pod_stack/diag/hostInfo.h
             
if PODMGR_STUB_CODE
else
if !ENABLE_YOCTO
EXTRA_DIST += cardmanager/scm_pod_stack/cacpPriv/mfr_types.h \
              cardmanager/scm_pod_stack/cacpPriv/vl_mfr_writer.c
endif
endif
libpodserver_la_includedir = $(includedir)
libpodserver_la_include_HEADERS = $(EXTRA_DIST)
libpodserver_la_LDFLAGS += $(SNMP_LIBS) \
		$(QAM_LIBS) \
		$(SF_LIBS) \
		$(RMF_OSAL_LIBS) \
		$(IARMBUS_LIBS)

if PODMGR_STUB_CODE
libpodserver_la_LDFLAGS += -lhal
else
libpodserver_la_LDFLAGS += $(SOCINIT_LIBS)
if ENABLE_YOCTO
libpodserver_la_LDFLAGS += -lpodserver_priv
endif
endif

