/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2011 RDK Management
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/




  
#ifndef __AVINTERFACE__
#define __AVINTERFACE__

//#include "pfckernel.h"
//#include "cmThreadBase.h"
//#include "pfccondition.h"
//#include "pfcresource.h"
#include "cmevents.h"

//#include "gateway.h"
#include <list>
#include <vector>
#include <string>


#include "SnmpIORM.h"
//#include "hal_api.h"
// #define API_CHRMAX 256
// #define API_MAX_IOD_LENGTH 128
 #include "snmpmanager.h"
#include "vlMutex.h"
#include "snmpManagerIf.h"
//#include "ocStbHostMibModule.h"
#include <string.h>
using namespace std;
#define TIME_GAP_FOR_UPDATE_IN_SECONDS 15
#define TIME_GAP_FOR_CARD_UPDATE_IN_SECONDS 5
// typedef u_long apioid;
// typedef struct vl_ocStbHostAVInterface{
//         u_long vl_ocStbHostAVInterfaceIndex;
//         apioid vl_ocStbHostAVInterfaceType[API_MAX_IOD_LENGTH];     //OID AutonomousType
//                 unsigned int  vl_ocStbHostAVInterfaceType_oidlenght;
//         char vl_ocStbHostAVInterfaceDesc[API_CHRMAX];                   //String :: SnmpAdminstring
//         long vl_ocStbHostAVInterfaceStatus;         //enum:: vl_AVInterfaceStatus
// }ocStbHostAVInterfaceTable_Data;

 
class Avinterface//JUL-11-2012: Deriving from SnmpManager not required //:public SnmpManager
{

   public:

    Avinterface();
    ~Avinterface();
        /* To get ocStbHostAVInterfaceTable_Data form pfc*/
         int vl_getHostAVInterfaceTable(SNMPocStbHostAVInterfaceTable_t* PtrAVInterfaceTable, int *nrows, bool  &bDataUpdatedNow);
     
        /* To get the tuner details form pfc*/
        int vl_getHostInBandTunerTable(SNMPocStbHostInBandTunerTable_t* vl_InBandTunerTable, bool &bDataUpdatedNow, unsigned long tunerplugAVindex);

        /* To get the DVIHDMI details form pfc*/ 
        int vl_getocStbHostDVIHDMITableData(SNMPocStbHostDVIHDMITable_t* vl_DVIHDMITable, bool &bDataUpdatedNow, u_long vlghandle,AV_INTERFACE_TYPE   avintercacetype);

        int vl_setocStbHostDVIHDMI3DCompatibilityControl(long eCompatibilityType);
        int vl_setocStbHostDVIHDMI3DCompatibilityMsgDisplay(long eStatus);

        /* To get the DVIHDMIVideoFormat details form pfc*/ 
        int vlGet_ocStbHostDVIHDMIVideoFormatTableData(SNMPocStbHostDVIHDMIVideoFormatTable_t* vl_DVIHDMIVideoFormatTable, int *pnCount);


        /* To get the ComponentVideo details form pfc*/
        int vl_getocStbHostComponentVideoTableData(SNMPocStbHostComponentVideoTable_t *vl_ComponentVideoTable, bool &bDataUpdatedNow, u_long vlghandle,AV_INTERFACE_TYPE   avintercacetype);

    /* To get the RF details form pfc*/
    int vl_getocStbHostRFChannelOutTableData(SNMPocStbHostRFChannelOutTable_t *vl_RFChannelOutTable, bool &bDataUpdatedNow, u_long vlghandle,AV_INTERFACE_TYPE   avintercacetype);

       /* To get the AnalogVideoTable details form pfc*/
        int vlGet_ocStbHostAnalogVideoTable(SNMPocStbHostAnalogVideoTable_t *vl_AnalogVideoTable , bool  &bDataUpdatedNow, u_long vlghandle, AV_INTERFACE_TYPE   avintercacetype);

    /* To get the SPDIFTable details form pfc*/
        int vlGet_ocStbHostSPDIfTableData(SNMPocStbHostSPDIfTable_t* vl_SPDIfTable,  bool SPDIfTableTableUpdatedNow,  u_long vlghandle, AV_INTERFACE_TYPE   avintercacetype );


        /* To get the ProgramStatus details form pfc*/
        int vlGet_ocStbHostProgramStatusTableData(programInfo *vlg_programInfo, bool &bDataUpdatedNow , int *numberpg, int *vlg_Signal_Status);
   
    /* To get the Mpeg2Stream details form pfc*/
    int vlGet_ocStbHostMpeg2ContentTableData(SNMPocStbHostMpeg2ContentTable_t *vl_Mpeg2ContentTable,  bool &bDataUpdatedNow, unsigned int pysicalID,int Mpegactivepgnumber);

#ifdef USE_1394
        /* To get the IEEE1394 Details*/
        int vlGet_ocStbHostIEEE1394TableData(IEEE1394Table_t* Ieee1394obj);
        /* To get the IEEE1394ConnectedDevicesTableData*/
        int vlGet_ocStbHostIEEE1394ConnectedDevicesTableData(IEEE1394ConnectedDevices_t* Ieee1394CDObj, int* NumberofPorts);
#endif

       int vlGet_ocStbHostSystemTempTableData(SNMPocStbHostSystemTempTable_t*sysTemperature, int *nitems );


        int vlGet_ocStbHostSystemHomeNetworkTableData(SNMPocStbHostSystemHomeNetworkTable_t* syshomeNW, int *nclients);
       
        int vlGet_ocStbHostSystemMemoryReportTableData(SNMPocStbHostSystemMemoryReportTable_t* sysMemoryRep, int *nmemtype);
         
        int vlGet_ocStbHostCCAppInfoTableData(SNMPocStbHostCCAppInfoTable_t* CCAppInfolist,  int *nitems);
        int 
         
        vlGet_ocStbHostSoftwareApplicationInfoTableData(SNMPocStbHostSoftwareApplicationInfoTable_t* swapplication, int * nswappl);

        /*     ifTable     iPnettoPhysicalTable     */
        int vlGet_ifIpTableData(SNMPifTable_t* ifdata, SNMPipNetToPhysicalTable_t* ipnettophy, int *iftabelcheck);

        /*scalar data function to get serial no, host id and host cap and card info*/
        
        void vlGet_Serialnum(char* Serialno, int nChars);
        void vlGet_Hostid(char* HostID);

        void vlGet_Hostcapabilite(int* hostcap);
      
        void vlGet_ocStbHostAvcSupport(Status_t* avcSupport);
  
        void vlGet_ocStbEasMessageState(EasInfo_t* EasobectsInfo);
       
        void vlGet_ocStbHostDeviceSoftware(DeviceSInfo_t * DevSoftareInfo);
      
        void vlGet_ocStbHostPower(HostPowerInfo_t* Hostpowerstatus);

        void vlGet_OcstbHostReboot(SOcstbHostRebootInfo_t* HostRebootInfo);

        void vlSet_OcstbHostReboot();
       
        void vlGet_ocStbHostCardInfo(CardInfo_t* CardIpInfo);

        void vlGet_ocstbHostInfo(SOcstbHOSTInfo_t* HostIpinfo);

        void vlGet_UserSettingsPreferedLanguage(char* UserSettingsPreferedLanguage);

        void vlGet_HostCaTypeInfo(HostCATInfo_t* CAtypeinfo);
  
        void vlGet_HostFirmwareDownloadStatus(FDownloadStatus_t* FirmwareStatus);

        void vlGet_ocStbHostSoftwareApplicationSigstatus(SNMPocStbHostSoftwareApplicationInfoTable_t* swapplicationStaticstatus);

        int vlGet_ocStbHostQpsk(ocStbHostQpsk_t* QpskObj);
       
        void vlGet_ocStbHostCard_Details(SocStbHostSnmpProxy_CardInfo_t *vl_ProxyCardInfo); 
         
        int vl_getCertificationInfo(Vividlogiccertification_t *Vl_CertificationInfo, int *nitem);
        
		int vl_GetMCertificationInfo(vlMCardCertInfo_t *pgetMcardCertInfo, int *nitem);

        void vlGet_OcstbHostMemoryInfo(SMemoryInfo_t* HostMemoryInfo);
        int vlSet_ocStbHostSoftwareApplicationInfoTable_set_list(int nApps, SNMPocStbHostSoftwareApplicationInfoTable_t * pAppInfolist);
        int vlSet_ocStbHostSwApplicationInfoTable_set_list(int nApps, SNMPHOSTSoftwareApplicationInfoTable_t * pAppInfolist);
       
        int vlMpeosSet_ocStbHostSwApplicationInfoTable_set_list(int nApps, VLMPEOS_SNMPHOSTSoftwareApplicationInfoTable_t * pAppInfolist);
        
        int vlGet_ocStbHostJvmInfo(SJVMinfo_t * pJvmInfo);
        int vlSet_ocStbHostJvmInfo(SJVMinfo_t * pJvmInfo);
        
        unsigned int vlGet_ocStbHostOobCarouselTimeoutCount(int * pTimeoutCount);

        unsigned int vlGet_ocStbHostInbandCarouselTimeoutCount(int * pTimeoutCount);
        
        unsigned int vlSet_ocStbHostOobCarouselTimeoutCount(int nTimeoutCount);
        
        unsigned int vlSet_ocStbHostInbandCarouselTimeoutCount(int nTimeoutCount);
        
        unsigned int vlGet_ocStbHostPatPmtTimeoutCount(unsigned long* pPATErrs,unsigned long* pPMTErrs);
        unsigned int vlSet_ocStbHostPatPmtTimeoutCount(unsigned long nPatTimeout,unsigned long nPmtTimeout);
        
        
        void getsystemUPtime(long *syst);
        bool IsUpdateReqd_CheckStatus(long & mcurrentTime, long timeInterval = TIME_GAP_FOR_UPDATE_IN_SECONDS);
  private:
       static long mTimeStamp_AVInterfaceTable;
        static long mTimeStamp_IBTuner;        
        static long mTimeStamp_QPSK;
     
        vector<SNMPocStbHostSoftwareApplicationInfoTable_t> m_hostSwAppInfo;
        SJVMinfo_t m_jvmInfo;
        vlMutex m_lock;
};



#endif //__AVINTERFACE__
