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


#include <unistd.h>
#include <list>

#include "ipcclient_impl.h"
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <linux/if_arp.h>
#include <netdb.h>
#include <string.h>

#define stricmp strcasecmp
#include "rmf_osal_event.h"
#include "vlSnmpHostInfo.h" //for scalar data module
#include "xchanResApi.h" // for Host ip info snmp
#include "vlHalSnmpUtils.h"
#include "utilityMacros.h"
#include "snmpmanager.h"

#include <pthread.h>
/***Start : Snmp related headers**********/
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <signal.h>

#include "snmpmanager.h"
#include "AvInterfaceGetdata.h"
#include "snmpIntrfGetAvOutData.h"
#include "snmpIntrfGetTunerData.h"
#ifdef USE_1394
#include "cNMDefines.h" // for 1394 port status Avlist
#include "vlpluginapp_hal1394api.h"
#include "vl_1394_api.h" //for 1394 certification
#include "AvcGeneralTypes.h" //for 1394 Firebus types
#endif //USE_1394
extern "C" unsigned char vl_isFeatureEnabled(unsigned char *featureString);

#include "cm_api.h" // M-Card certification
#include "snmp_types.h"
#include "dsg_types.h" //for DOCSIS and DSG status
#include "dsgResApi.h"
#include "vlSnmpEcmApi.h"

#include "vlpluginapp_halplatformapi.h"
#include "snmpManagerIf.h"
#include "CommonDownloadSnmpInf.h"
#include "vlMutex.h"
#include "vlEnv.h"
#include "rdk_debug.h"
#include "rmf_osal_sync.h"
#include "cdl_mgr_types.h" //for DOCSIS and DSG status

#if USE_SYSRES_MLT
#include "rpl_malloc.h"
#endif
#include<fstream>

extern "C" rmf_Error podImplGetCaSystemId(unsigned short * pCaSystemId);

#define vlAbs(v)      (((v)<0)?-(v):(v))

#define VL_VALUE_6_FROM_ARRAY(pArray)                                   \
   (((unsigned long long)((pArray)[0]&0xFF) << 40) |                    \
    ((unsigned long long)((pArray)[1]&0xFF) << 32) |                    \
    ((unsigned long long)((pArray)[2]&0xFF) << 24) |                    \
    ((unsigned long long)((pArray)[3]&0xFF) << 16) |                    \
    ((unsigned long long)((pArray)[4]&0xFF) <<  8) |                    \
    ((unsigned long long)((pArray)[5]&0xFF) <<  0) )


#define __MTAG__ VL_SNMP_MANAGER
#define VL_SNMP_EVENT_LOCK  "SNMPEventhandling"
// for data static filling only
#define API_OID_LENGTH(x)  (sizeof(x)/sizeof(apioid))

#define MAX_ROWS 15

// #define API_CHRMAX 256
// #define API_MAX_IOD_LENGTH 128
#define IF_TABLE_DESC_SIZE 3
char ifTableDesc[IF_TABLE_DESC_SIZE][API_CHRMAX]= {"OCHD2.1 Embedded IP 2-way Interface", "CableCARD Unicast IP Flow", "MultiMedia over Coax Alliance (MoCA) Interface"  };
int GetHostIdLuhn    ( unsigned char *pHostId,  char *pHostIdLu );
static char vlg_szMpeosFeature1394[]        = {"FEATURE.1394.SUPPORT"};
Status_t gcardsnmpAccCtl =  STATUS_FALSE;

#include "ocStbHostMibModule.h"
#include <time.h>
#define vlStrCpy(pDest, pSrc, nDestCapacity)            \
            strcpy(pDest, pSrc)

#define vlMemCpy(pDest, pSrc, nCount, nDestCapacity)            \
                                                  memcpy(pDest, pSrc, nCount)
                                                  
typedef struct _MPE_DIAG_CA_SYSTEM_ID
{
    unsigned short m_caSystemId;
    
}MPE_DIAG_CA_SYSTEM_ID;

typedef enum _MPE_DIAG_INFO_TYPE
{
    MPE_DIAG_INFO_TYPE_OCAP_APP_INFO_TABLE      = 0,
    MPE_DIAG_INFO_TYPE_USER_PREFERRED_LANGUAGE  = 1,
    MPE_DIAG_INFO_TYPE_JVM_INFO                 = 2,
    MPE_DIAG_INFO_TYPE_PAT_PMT_TIMEOUT          = 3,
    MPE_DIAG_INFO_TYPE_OOB_CAROUSEL_TIMEOUT     = 4,
    MPE_DIAG_INFO_TYPE_INBAND_CAROUSEL_TIMEOUT  = 5,
    MPE_DIAG_INFO_TYPE_CA_SYSTEM_ID             = 6,
    MPE_DIAG_INFO_TYPE_TRANSPORT_STREAM_ID      = 7,
    MPE_DIAG_INFO_TYPE_SI_DIAGNOSTICS           = 8,
    
}MPE_DIAG_INFO_TYPE;

long Avinterface::mTimeStamp_AVInterfaceTable = -1;
long Avinterface::mTimeStamp_IBTuner = -1;
long Avinterface::mTimeStamp_QPSK = -1;

static long vlg_timeCableCardAppInfoLastUpdated = 0;
static long vlg_timeCableCardInfoLastUpdated    = 0;
static long vlg_timeCableCardDetailsLastUpdated = 0;
static CardInfo_t                     vlg_cableCardInfo;
static SocStbHostSnmpProxy_CardInfo_t vlg_cableCardDetails;

//static pfcMutex SNMPAviMutex;

Avinterface::Avinterface()
{
    //ocStbHostSoftwareApplicationInfoTable
    /* Column values */
    SNMPocStbHostSoftwareApplicationInfoTable_t swapplication[1];
    //VL_ZERO_MEMORY(swapplication);
    memset(&swapplication, 0, sizeof(swapplication));
    //VL_ZERO_MEMORY(m_jvmInfo);
    memset(&m_jvmInfo, 0, sizeof(m_jvmInfo));
    vlStrCpy(swapplication[0].vl_ocStbHostSoftwareAppNameString, "Amber", API_CHRMAX);
    vlMemCpy(swapplication[0].vl_ocStbHostSoftwareAppVersionNumber, "1.0",strlen("1.0"), API_CHRMAX );
    swapplication[0].vl_ocStbHostSoftwareStatus   = loaded;
    /**Organization ID captured in the XAIT or AIT*/
    vlMemCpy(swapplication[0].vl_ocStbHostSoftwareOrganizationId,"01", strlen("22"), SORG_ID);
    /**This 16 bit field uniqTuely identifies the  application function as assigned by the responsible/owning organization identified in*/
    vlMemCpy(swapplication[0].vl_ocStbHostSoftwareApplicationId,"11", strlen("11"), SAID);
    /** "The field SHALL be set to Okay if the XAIT has been read successfully or Error if there was an error reading the table (e.g., CRC error)."*/
    swapplication[0].vl_ocStbHostSoftwareApplicationSigStatus = SoftwareAPP_SigStatus_okay;

    vlSet_ocStbHostSoftwareApplicationInfoTable_set_list(1, swapplication);

    //Disabling feature that causes lot of code-execution during POD resets.
    bool bEnableCableCardMibAccess = vl_env_get_bool(false, "SNMP.ENABLE_CABLE_CARD_MIB_ACCESS");
    gcardsnmpAccCtl = (bEnableCableCardMibAccess?STATUS_TRUE:STATUS_FALSE);
}

void Avinterface::getsystemUPtime(long *syst)
{
    if(NULL != syst)
    {
        *syst = get_uptime();
        //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "%s() : get_uptime() returned : %d\n", __FUNCTION__, *syst);
        if (*syst == -1)
        SNMP_DEBUGPRINT("time returned  error\n");

    }
    else
    {
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "%s() : NULL param\n", __FUNCTION__);
    }
}

bool Avinterface::IsUpdateReqd_CheckStatus(long & mcurrentTime, long timeInterval)
{
    long curTimeStamp;
    long timeDiff = 0;
    getsystemUPtime(&curTimeStamp);
    mcurrentTime > curTimeStamp ?
        (timeDiff = (mcurrentTime - curTimeStamp)) :
            (timeDiff = (curTimeStamp - mcurrentTime));

   if (timeDiff < timeInterval)
   {
       RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "IsUpdateReqd_CheckStatus : timeDiff = %d, mcurrentTime =%d\n", timeDiff,mcurrentTime);
       return false;
   }
   mcurrentTime = curTimeStamp;
   return true;
}
int Avinterface::vl_getHostAVInterfaceTable(SNMPocStbHostAVInterfaceTable_t* vl_AVInterfaceTable, int *nrows, bool &bDataUpdatedNow)
{
     VL_AUTO_LOCK(m_lock);
     ocStbHostAVInterfaceTable_S ocStbHostAVInterface[10];
     SNMP_DEBUGPRINT(" \n :::  Avinterface::vl_getHostAVInterfaceTable ::: 1\n");
     int sizeoflist = 0;
     long csystime;
 
     bDataUpdatedNow = true; //default value
 
 //    Check if update is required by checking the time stamp.
     /*if (! IsUpdateReqd_CheckStatus(mTimeStamp_AVInterfaceTable))
     {
 
         bDataUpdatedNow = false;
         return 1;
     }*/
 
    snmpAvOutInterface snmpAvOutIntrfObj;
     snmpAvOutIntrfObj.Snmp_getallAVdevicesindex(vl_AVInterfaceTable, &sizeoflist);
     *nrows = sizeoflist;
         if(sizeoflist == 0)
         {
            //auto_unlock_ptr(m_lock);
           *nrows = 1; // PXG2V2-1327 : Set the count to 1 to allow table initialization to complete.
         }
    getsystemUPtime(&csystime);
     mTimeStamp_AVInterfaceTable = csystime; //cGetSystemUpTime();
     //auto_unlock_ptr(m_lock);
        return 1;
}

int  Avinterface::vl_getHostInBandTunerTable(SNMPocStbHostInBandTunerTable_t*  vl_InBandTunerTable, bool &bDataUpdatedNow, unsigned long tunerplugAVindex)
{
//     //VL_AUTO_LOCK(m_lock);
//     long csystime;
// 
    snmpTunerInterface snmpTunerIntrfObj;
     if(snmpTunerIntrfObj.Snmp_getTunerdetails(vl_InBandTunerTable, tunerplugAVindex) == 0)
         {
           return 0;//if Snmp_getTunerdetails Fails
         }
//     getsystemUPtime(&csystime);
//         mTimeStamp_IBTuner = csystime; //cGetSystemUpTime();
   return 1;

}

static DVIHDMI3DCompatibilityControlType_t vlg_eDVIHDMI3DCompatibilityControlMode = DVIHDMI3DCompatibilityControl_passthru3D;
static Status_t vlg_eDVIHDMI3DCompatibilityMsgDisplayStatus = STATUS_FALSE;

int  Avinterface::vl_getocStbHostDVIHDMITableData(SNMPocStbHostDVIHDMITable_t* vl_DVIHDMITable, bool &bDataUpdatedNow, u_long vlghandle,AV_INTERFACE_TYPE   avintercacetype)
{

    VL_AUTO_LOCK(m_lock);
    snmpAvOutInterface snmpAvOutIntrfObj;
    SNMP_DEBUGPRINT("\n %s----- %dstart   \n",__FUNCTION__, __LINE__);
    snmpAvOutIntrfObj.snmp_get_DviHdmi_info(vlghandle, avintercacetype, vl_DVIHDMITable);
    vl_DVIHDMITable->vl_ocStbHostDVIHDMI3DCompatibilityControl = vlg_eDVIHDMI3DCompatibilityControlMode;
    vl_DVIHDMITable->vl_ocStbHostDVIHDMI3DCompatibilityMsgDisplay = vlg_eDVIHDMI3DCompatibilityMsgDisplayStatus;
    SNMP_DEBUGPRINT("\n %s----- %dEnd  \n",__FUNCTION__, __LINE__);
    //auto_unlock_ptr(m_lock);
    return 1;
}
int Avinterface::vl_setocStbHostDVIHDMI3DCompatibilityControl(long eCompatibilityType)
{
    vlg_eDVIHDMI3DCompatibilityControlMode = (DVIHDMI3DCompatibilityControlType_t)eCompatibilityType;
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "%s:: vlg_eDVIHDMI3DCompatibilityControlMode = %d.\n ",__FUNCTION__,vlg_eDVIHDMI3DCompatibilityControlMode);
    return 1;
}

int Avinterface::vl_setocStbHostDVIHDMI3DCompatibilityMsgDisplay(long eStatus)
{
    vlg_eDVIHDMI3DCompatibilityMsgDisplayStatus = (Status_t)eStatus;
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "%s:: vlg_eDVIHDMI3DCompatibilityMsgDisplayStatus = %d.\n ",__FUNCTION__,vlg_eDVIHDMI3DCompatibilityMsgDisplayStatus);
    return 1;
}

int Avinterface::vlGet_ocStbHostDVIHDMIVideoFormatTableData(SNMPocStbHostDVIHDMIVideoFormatTable_t* vl_DVIHDMIVideoFormatTable, int *pnCount)
{
        //Need a lower level API to get dynamic values.
    VL_AUTO_LOCK(m_lock);
    SNMP_DEBUGPRINT("\n %s----- %dstart  \n",__FUNCTION__, __LINE__);
        /**  Gives vl_ocStbHostDVIHDMIVideoFormatTableData info from haldisplay moudle*/
    snmpAvOutInterface snmpAvOutIntrfObj;
    snmpAvOutIntrfObj.snmp_get_ocStbHostDVIHDMIVideoFormatTableData(vl_DVIHDMIVideoFormatTable, pnCount);
    SNMP_DEBUGPRINT("\n %s----- %d, count = %d, End  \n",__FUNCTION__, __LINE__, *pnCount);    
    //auto_unlock_ptr(m_lock);
    return 1;
}

int Avinterface::vl_getocStbHostComponentVideoTableData(SNMPocStbHostComponentVideoTable_t *vl_ComponentVideoTable, bool &bDataUpdatedNow, u_long vlghandle,AV_INTERFACE_TYPE avintercacetype)
{

    VL_AUTO_LOCK(m_lock);
      /**  Gives the vl_ComponentVideoTable  from haldisplay moudle*/

    //SNMP_DEBUGPRINT("\n vl_ComponentVideoTable :: Calling the haldispaly moudle to get the data 1\n");
        //returns void so can't check the conditions to return fail condition
    snmpAvOutInterface snmpAvOutIntrfObj;
        snmpAvOutIntrfObj.snmp_get_ComponentVideo_info(vlghandle, avintercacetype, vl_ComponentVideoTable);

        //SNMP_DEBUGPRINT("\n vl_ComponentVideoTable ::Calling the haldispaly moudle to get the data 2\n");

    //auto_unlock_ptr(m_lock);
    return 1;
}

int  Avinterface::vl_getocStbHostRFChannelOutTableData(SNMPocStbHostRFChannelOutTable_t *vl_RFChannelOutTable, bool &bDataUpdatedNow, u_long vlghandle,AV_INTERFACE_TYPE   avintercacetype)
{
    VL_AUTO_LOCK(m_lock);
      /**  Gives the vl_RFChannelOutTable  from haldisplay moudle*/

        //SNMP_DEBUGPRINT("\n vl_RFChannelOutTable ::Calling the haldispaly moudle to get the data 1\n");
     //returns void so can't check the conditions to return fail condition
    snmpAvOutInterface snmpAvOutIntrfObj;
    snmpAvOutIntrfObj.snmp_get_RFChan_info(vlghandle, avintercacetype, vl_RFChannelOutTable);
     //SNMP_DEBUGPRINT("\n vl_RFChannelOutTable ::Calling the haldispaly moudle to get the data 2\n");
    //auto_unlock_ptr(m_lock);
   return 1;
}

int Avinterface::vlGet_ocStbHostAnalogVideoTable(SNMPocStbHostAnalogVideoTable_t *vl_AnalogVideoTable , bool &bDataUpdatedNow, u_long vlghandle, AV_INTERFACE_TYPE   avintercacetype)
{
    VL_AUTO_LOCK(m_lock);
     /**  Gives the vl_AnalogVideoTable  from haldisplay moudle*/
     //SNMP_DEBUGPRINT("\n vl_AnalogVideoTable ::Calling the haldispaly moudle to get the data 1\n");
     //returns void so can't check the conditions to return fail condition
    snmpAvOutInterface snmpAvOutIntrfObj;
         snmpAvOutIntrfObj.snmp_get_AnalogVideo_info(vlghandle, avintercacetype, vl_AnalogVideoTable);

     //SNMP_DEBUGPRINT("\n vl_AnalogVideoTable ::Calling the haldispaly moudle to get the data 2\n");
    //auto_unlock_ptr(m_lock);
   return 1;
}

int Avinterface::vlGet_ocStbHostSPDIfTableData(SNMPocStbHostSPDIfTable_t* vl_SPDIfTable, bool SPDIfTableTableUpdatedNow,  u_long vlghandle, AV_INTERFACE_TYPE   avinterfacetype )
{
     VL_AUTO_LOCK(m_lock);
   SNMP_DEBUGPRINT("%s: vlGet_ocStbHostSPDIfTableData\n", __FUNCTION__);

    snmpAvOutInterface snmpAvOutIntrfObj;
    snmpAvOutIntrfObj.snmp_get_Spdif_info(vlghandle, avinterfacetype, vl_SPDIfTable);
    //auto_unlock_ptr(m_lock);

  return 1;
}

/* To get the IEEE1394 Details*/
#ifdef USE_1394
int Avinterface::vlGet_ocStbHostIEEE1394TableData(IEEE1394Table_t* Ieee1394obj/*, int Iee1394port*/ )
{
    VL_AUTO_LOCK(m_lock);
     int pNum1394Interfaces;
     int Ieeeportindex;
     pNum1394Interfaces = 64;
     VL_1394_INTERFACE_INFO interfaceInfo;
     //vlMemSet(&interfaceInfo, 0, sizeof(interfaceInfo), sizeof(interfaceInfo));
     memset(&interfaceInfo, 0, sizeof(interfaceInfo));
     int result = -1;
#ifdef USE_1394
        if(vl_isFeatureEnabled((unsigned char *)vlg_szMpeosFeature1394))
        {
        result = CHAL1394_SnmpGet1394Table(&pNum1394Interfaces, &interfaceInfo);
    }
#endif // USE_1394

     if(result == 0)
     {

         //SNMP_DEBUGPRINT
         RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "vlGet_ocStbHostIEEE1394TableData:: %d interfaceInfo  portNum = %d\n numNodes = %d\n isXmtngData = %d\n dtcpStatus = %d\n usInLoop = %d \nisRoot = %d \nisCycleMaster = %d \nisIRM = %d \n ",__FUNCTION__,__LINE__, interfaceInfo.numNodes, interfaceInfo.isXmtngData, interfaceInfo.dtcpStatus, interfaceInfo.busInLoop, interfaceInfo.isRoot, interfaceInfo.isCycleMaster, interfaceInfo.isIRM);
        // RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "interfaceInfo[Ieeeportindex].numNodes :::::::: %d",interfaceInfo[Ieeeportindex].numNodes);
        Ieee1394obj->vl_ocStbHostIEEE1394ActiveNodes = interfaceInfo.numNodes;
        if(interfaceInfo.isXmtngData)
        {
            Ieee1394obj->vl_ocStbHostIEEE1394DataXMission = STATUS_TRUE;
        }
        else
        {
            Ieee1394obj->vl_ocStbHostIEEE1394DataXMission = STATUS_FALSE;
        }
        if(interfaceInfo.dtcpStatus)
        {
            Ieee1394obj->vl_ocStbHostIEEE1394DTCPStatus   =  STATUS_TRUE;
        }
        else
        {
            Ieee1394obj->vl_ocStbHostIEEE1394DTCPStatus   =  STATUS_FALSE;
        }
        if(interfaceInfo.busInLoop)
        {
              Ieee1394obj->vl_ocStbHostIEEE1394LoopStatus   =  STATUS_TRUE;
        }
        else
        {
                Ieee1394obj->vl_ocStbHostIEEE1394LoopStatus   =  STATUS_FALSE;
        }
        if(interfaceInfo.isRoot)
        {
                  Ieee1394obj->vl_ocStbHostIEEE1394RootStatus   = STATUS_TRUE;
        }
        else
        {
                   Ieee1394obj->vl_ocStbHostIEEE1394RootStatus   =  STATUS_FALSE;
        }
        if(interfaceInfo.isCycleMaster)
        {
                 Ieee1394obj->vl_ocStbHostIEEE1394CycleIsMaster = STATUS_TRUE;
        }
        else
        {
                 Ieee1394obj->vl_ocStbHostIEEE1394CycleIsMaster = STATUS_FALSE;
        }
        if(interfaceInfo.isIRM)
        {
               Ieee1394obj->vl_ocStbHostIEEE1394IRMStatus =   STATUS_TRUE ;
        }
        else
        {
            Ieee1394obj->vl_ocStbHostIEEE1394IRMStatus =     STATUS_FALSE;
        }
        Ieee1394obj->vl_ocStbHostIEEE1394AudioMuteStatus = STATUS_FALSE;
        Ieee1394obj->vl_ocStbHostIEEE1394VideoMuteStatus = STATUS_FALSE;

       //auto_unlock_ptr(m_lock);
        return 1;
     }
     else{

          RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "%s :FAILED TO GET CHAL1394_SnmpGet1394Table\n", __FUNCTION__);
          Ieee1394obj->vl_ocStbHostIEEE1394ActiveNodes = 0;
          Ieee1394obj->vl_ocStbHostIEEE1394DataXMission = STATUS_FALSE;
          Ieee1394obj->vl_ocStbHostIEEE1394DTCPStatus   =  STATUS_FALSE;
          Ieee1394obj->vl_ocStbHostIEEE1394LoopStatus   =  STATUS_FALSE;
          Ieee1394obj->vl_ocStbHostIEEE1394RootStatus   =  STATUS_FALSE;
          Ieee1394obj->vl_ocStbHostIEEE1394CycleIsMaster = STATUS_FALSE;
          Ieee1394obj->vl_ocStbHostIEEE1394IRMStatus =   STATUS_TRUE ;
          Ieee1394obj->vl_ocStbHostIEEE1394AudioMuteStatus = STATUS_FALSE;
          Ieee1394obj->vl_ocStbHostIEEE1394VideoMuteStatus = STATUS_FALSE;
        //auto_unlock_ptr(m_lock);
        return 1;
     }

}
      /* To get the IEEE1394ConnectedDevicesTableData*/
int Avinterface::vlGet_ocStbHostIEEE1394ConnectedDevicesTableData(IEEE1394ConnectedDevices_t* Ieee1394CDObj,  /*int Iee1394port ,*/ int* NumberofDvConnected)
{
    //SNMP_DEBUGPRINT("%s: vlGet_ocStbHostIEEE1394ConnectedDevicesTableData\n", __FUNCTION__);
    VL_AUTO_LOCK(m_lock);
    unsigned int pNumDevices = 0;
    int portconnectddevices = 0;
    VL_1394_CONNECTED_DEV_INFO *connDevInfo;
#define MAX1394NODES 64
    //connDevInfo = (VL_1394_CONNECTED_DEV_INFO *)malloc(MAX1394NODES * sizeof(VL_1394_CONNECTED_DEV_INFO));
    rmf_osal_memAllocP(RMF_OSAL_MEM_POD, MAX1394NODES * sizeof(VL_1394_CONNECTED_DEV_INFO), (void**)&connDevInfo);

    //vlMemSet(connDevInfo, 0, MAX1394NODES *sizeof(VL_1394_CONNECTED_DEV_INFO), MAX1394NODES *sizeof(VL_1394_CONNECTED_DEV_INFO));
    memset(connDevInfo, 0, MAX1394NODES *sizeof(VL_1394_CONNECTED_DEV_INFO));
    pNumDevices = MAX1394NODES;
    int result = -1;

#ifdef USE_1394
        if(vl_isFeatureEnabled((unsigned char *)vlg_szMpeosFeature1394))
        {
        result = CHAL1394_SnmpGet1394AllDevicesTable(&pNumDevices, connDevInfo);
    }
#endif // USE_1394
RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "CHAL1394_SnmpGet1394AllDevicesTable result = %d \n", result);

    if(result  == 0)
    {
RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "CHAL1394_SnmpGet1394AllDevicesTable pNumDevices = %d \n", pNumDevices);
        //*NumberofDvConnected = pNumDevices;

        if(pNumDevices > 64)
        {
            //auto_unlock_ptr(m_lock);
            return 0; //error it should be less than 64 connections , **normaly it will have 1 to 4 connection devices
        }

        for(unsigned int j= 0; j< pNumDevices; j++)
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP","%s %d \n portNum = %d \n nodeID = %d \n avInterfaceIndex = %d \n subunitType =  %d\n netguid = %d\n isA2DCapable = %d \n",__FUNCTION__,__LINE__,connDevInfo[j].portNum, connDevInfo[j].nodeId, connDevInfo[j].avInterfaceIndex, connDevInfo[j].subunitType, connDevInfo[j].netguid.guid, connDevInfo[j].isA2DCapable );
            SNMP_DEBUGPRINT("%s %d \n portNum = %d \n nodeID = %d \n avInterfaceIndex = %d \n subunitType =  %d\n netguid = %d\n isA2DCapable = %d \n",__FUNCTION__,__LINE__,connDevInfo[j].portNum, connDevInfo[j].nodeId, connDevInfo[j].avInterfaceIndex, connDevInfo[j].subunitType, connDevInfo[j].netguid.guid, connDevInfo[j].isA2DCapable );
          //  if( connDevInfo[j].portNum == Iee1394port )
          //  {
                switch(connDevInfo[j].subunitType)
                {

                    case AVC_SUBUNIT_TYPE_MONITOR:
                            Ieee1394CDObj[portconnectddevices].vl_ocStbHostIEEE1394ConnectedDevicesSubUnitType = SUBUNITTYPE_MONITOR;
                                        break;
                    case AVC_SUBUNIT_TYPE_AUDIO  :
                            Ieee1394CDObj[portconnectddevices].vl_ocStbHostIEEE1394ConnectedDevicesSubUnitType = SUBUNITTYPE_AUDIO;
                                    break;
                    case AVC_SUBUNIT_TYPE_PRINTER  :
                            Ieee1394CDObj[portconnectddevices].vl_ocStbHostIEEE1394ConnectedDevicesSubUnitType = SUBUNITTYPE_PRINTER;
                                    break;
                    case AVC_SUBUNIT_TYPE_DISC     :
                            Ieee1394CDObj[portconnectddevices].vl_ocStbHostIEEE1394ConnectedDevicesSubUnitType = SUBUNITTYPE_DISC;
                                    break;
                    case AVC_SUBUNIT_TYPE_TAPE     :
                            Ieee1394CDObj[portconnectddevices].vl_ocStbHostIEEE1394ConnectedDevicesSubUnitType = SUBUNITTYPE_TAPE;
                                    break;
                    case AVC_SUBUNIT_TYPE_TUNER    :
                            Ieee1394CDObj[portconnectddevices].vl_ocStbHostIEEE1394ConnectedDevicesSubUnitType = SUBUNITTYPE_TUNER;
                                    break;
                    case AVC_SUBUNIT_TYPE_CA       :
                            Ieee1394CDObj[portconnectddevices].vl_ocStbHostIEEE1394ConnectedDevicesSubUnitType = SUBUNITTYPE_CA;
                                    break;
                    case AVC_SUBUNIT_TYPE_CAMERA   :
                            Ieee1394CDObj[portconnectddevices].vl_ocStbHostIEEE1394ConnectedDevicesSubUnitType =  SUBUNITTYPE_CAMERA;
                                    break;
                    case AVC_SUBUNIT_TYPE_PANEL     :
                            Ieee1394CDObj[portconnectddevices].vl_ocStbHostIEEE1394ConnectedDevicesSubUnitType = SUBUNITTYPE_PANEL;
                                    break;
                    default :
                            Ieee1394CDObj[portconnectddevices].vl_ocStbHostIEEE1394ConnectedDevicesSubUnitType =   SUBUNITTYPE_OTHER;
                                    break;
                }

                /*vlMemCpy(Ieee1394CDObj[j].vl_ocStbHostIEEE1394ConnectedDevicesEui64, connDevInfo[j].netguid.guid, strlen(connDevInfo[j].netguid.guid), 255);
                */
                snprintf(Ieee1394CDObj[portconnectddevices].vl_ocStbHostIEEE1394ConnectedDevicesEui64,
                         sizeof(Ieee1394CDObj[portconnectddevices].vl_ocStbHostIEEE1394ConnectedDevicesEui64),
                                "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",connDevInfo[j].netguid.guid[0],connDevInfo[j].netguid.guid[1],connDevInfo[j].netguid.guid[2],connDevInfo[j].netguid.guid[3],connDevInfo[j].netguid.guid[4],connDevInfo[j].netguid.guid[5],connDevInfo[j].netguid.guid[6], connDevInfo[j].netguid.guid[7]);
                // memcpy(Ieee1394CDObj[j].vl_ocStbHostIEEE1394ConnectedDevicesEui64, connDevInfo[j].netguid.guid, VL_GUID_VALUE_LENGTH);

        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP","\n Guid: %s\n",Ieee1394CDObj[j].vl_ocStbHostIEEE1394ConnectedDevicesEui64);
                SNMP_DEBUGPRINT("\n Guid: %s\n",Ieee1394CDObj[j].vl_ocStbHostIEEE1394ConnectedDevicesEui64);

                if(connDevInfo[portconnectddevices].isA2DCapable)
                {
                Ieee1394CDObj[portconnectddevices].vl_ocStbHostIEEE1394ConnectedDevicesADSourceSelectSupport= ADSOURCESELECTSUPPORT_TRUE ;
                }
                else
                {
                    Ieee1394CDObj[portconnectddevices].vl_ocStbHostIEEE1394ConnectedDevicesADSourceSelectSupport = ADSOURCESELECTSUPPORT_FALSE    ;
                }
                ++portconnectddevices; // count of the connected devices to a particular port

                *NumberofDvConnected = portconnectddevices;
            //}//if condition for the PORT type "0" or "1" minimum 2 ports

        }//for lopp for conn devices
        //free(connDevInfo);
        rmf_osal_memFreeP(RMF_OSAL_MEM_POD, connDevInfo);
        if(portconnectddevices == 0)
        {
            //auto_unlock_ptr(m_lock);
            return 0;
        }
      /*SNMP_DEBUGPRINT*/RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "Pnumber of Device connected   pNumDevices= %d \n", *NumberofDvConnected);
        //auto_unlock_ptr(m_lock);
        return 1;
    }//if condition
    else{
         //free(connDevInfo);
         rmf_osal_memFreeP(RMF_OSAL_MEM_POD, connDevInfo);
          //auto_unlock_ptr(m_lock);
          return 0;
    }
}

#endif// USE_1394

/**
 *  Returns a "1" on sucess
 *  Returns "0" on any error
 *
 * @param vlg_programInfo       OUT - global struct contains info about
    unsigned long physicalSrcId;
    int pcrPid;
    int videoPid;
    int audioPid;
    int pgstatus;
    int activepgnumber;
    Decoder type
    SnmpMediaType_t mediaType;//disc or 1394 or dvr
    bool snmpVDecoder;   //tuner flow to vedio ports
    bool snmpADecoder;   //tuner flow to audio ports
*/

int Avinterface::vlGet_ocStbHostProgramStatusTableData(programInfo *vlg_programInfo, bool &bDataUpdatedNow , int *numberpg, int *vlg_Signal_Status)
{

    snmpAvOutInterface snmpAvOutIntrfObj;
        snmpAvOutIntrfObj.snmp_get_ProgramStatus_info(vlg_programInfo, numberpg, vlg_Signal_Status);

    bDataUpdatedNow = TRUE;

        return 1;
}


int Avinterface::vlGet_ocStbHostMpeg2ContentTableData(SNMPocStbHostMpeg2ContentTable_t *vl_Mpeg2ContentTable,  bool &bDataUpdatedNow , unsigned int pysicalID, int Mpegactivepgnumber)
{
    snmpAvOutInterface snmpAvOutIntrfObj;
        snmpAvOutIntrfObj.snmp_get_Mpeg2Content_info(vl_Mpeg2ContentTable, pysicalID, Mpegactivepgnumber);
    
    bDataUpdatedNow = TRUE;
        return 1;
}

/**------------ocStbHostSystemObjects--------------*/

 /**  ocStbHostSystemTempTable  */
int Avinterface::vlGet_ocStbHostSystemTempTableData(SNMPocStbHostSystemTempTable_t*sysTemperature, int *nitems )
{    
    VL_AUTO_LOCK(m_lock);

//    printf("vlGet_ocStbHostSystemTempTableData: BEGIN\n");
#if 0
    *nitems = 2;
   /**DUMMY values  OCHD2.1*/
          vlMemCpy(sysTemperature[0].vl_ocStbHostSystemTempDescr,"TEMPDESCR1",strlen("TEMPDESCR1"), API_CHRMAX);
          sysTemperature[0].vl_ocStbHostSystemTempDescr_len = strlen("TEMPDESCR1");
          sysTemperature[0].vl_ocStbHostSystemTempValue = 25;
          sysTemperature[0].vl_ocStbHostSystemTempLastUpdate = 12;
          sysTemperature[0].vl_ocStbHostSystemTempMaxValue = 99;

          vlMemCpy(sysTemperature[1].vl_ocStbHostSystemTempDescr,"TEMPDESCR2",strlen("TEMPDESCR2"), API_CHRMAX);
          sysTemperature[1].vl_ocStbHostSystemTempDescr_len = strlen("TEMPDESCR1");
          sysTemperature[1].vl_ocStbHostSystemTempValue = 25;
          sysTemperature[1].vl_ocStbHostSystemTempLastUpdate = 12;
          sysTemperature[1].vl_ocStbHostSystemTempMaxValue = 99;
#endif
    HOST_SYSTEM_SENSOR_TYPE sensorType[] =  {  HOST_SYSTEM_SENSOR_TYPE_CPU, HOST_SYSTEM_SENSOR_TYPE_HDD_INTERNAL, HOST_SYSTEM_SENSOR_TYPE_MAINBOARD};
    sTempDescrInfo  tempDescrInfo;
    
    for(int i = 0; i < MAX_NUM_TEMP_SENSOR_TYPES; i++)
    {
        HAL_SNMP_Get_HostSystemTempDetails(sensorType[i],  &tempDescrInfo);
        
        sysTemperature[i].vl_ocStbHostSystemTempDescr_len = tempDescrInfo.length;
        vlMemCpy(sysTemperature[i].vl_ocStbHostSystemTempDescr, tempDescrInfo.name , tempDescrInfo.length, API_CHRMAX);
        
        sysTemperature[i].vl_ocStbHostSystemTempValue       = tempDescrInfo.tempValue;
        sysTemperature[i].vl_ocStbHostSystemTempLastUpdate  = tempDescrInfo.tempLastUpdate;
        sysTemperature[i].vl_ocStbHostSystemTempMaxValue    = tempDescrInfo.tempMaxVal;
        
        *nitems += 1;
    }

  //auto_unlock_ptr(m_lock);
  return 1;
}


/**  ocStbHostSystemHomeNetworkTable  */
int Avinterface::vlGet_ocStbHostSystemHomeNetworkTableData(SNMPocStbHostSystemHomeNetworkTable_t* syshomeNW, int *nclients)
{
    VL_AUTO_LOCK(m_lock);
  *nclients = 1;
    /** DUMMY values OCHD2.1*/
    short int mac_temp[6] = {23,12,34,34,43,43};
    short int ip_temp[4] = { 19,16,23,12 };
    snprintf(syshomeNW[0].vl_ocStbHostSystemHomeNetworkClientMacAddress,
             sizeof(syshomeNW[0].vl_ocStbHostSystemHomeNetworkClientMacAddress),
                    "%02X:%02X:%02X:%02X:%02X:%02X",mac_temp[0],mac_temp[1],mac_temp[2],mac_temp[3],mac_temp[4],mac_temp[5]);

    snprintf(syshomeNW[0].vl_ocStbHostSystemHomeNetworkClientIpAddress,
             sizeof(syshomeNW[0].vl_ocStbHostSystemHomeNetworkClientIpAddress),
                    "%d.%d.%d.%d", ip_temp[0], ip_temp[1], ip_temp[2], ip_temp[3]);


    syshomeNW[0].vl_ocStbHostSystemHomeNetworkMaxClients = 12;
    syshomeNW[0].vl_ocStbHostSystemHomeNetworkHostDRMStatus = 1 ;
    syshomeNW[0].vl_ocStbHostSystemHomeNetworkConnectedClients = 1 ;

    syshomeNW[0].vl_ocStbHostSystemHomeNetworkClientMacAddress_len = 6*sizeof(char);
    syshomeNW[0].vl_ocStbHostSystemHomeNetworkClientIpAddress_len = strlen(syshomeNW[0].vl_ocStbHostSystemHomeNetworkClientIpAddress);

    syshomeNW[0].vl_ocStbHostSystemHomeNetworkClientDRMStatus = 1;

  //auto_unlock_ptr(m_lock);
  return 1;
}


/**  ocStbHostSystemMemoryReportTable  */
int Avinterface::vlGet_ocStbHostSystemMemoryReportTableData(SNMPocStbHostSystemMemoryReportTable_t* sysMemoryRep, int *nmemtype)
{
    int i = 0;
    VL_SNMP_SystemMemoryReportTable snmpSystemMemoryReport;
    memset(&snmpSystemMemoryReport, 0, sizeof(snmpSystemMemoryReport));
    snmpSystemMemoryReport.nEntries = VL_SNMP_HOST_MIB_MEMORY_TYPE_MAX;
    IPC_CLIENT_CHalSys_snmp_request(VL_SNMP_REQUEST_GET_SYS_SYSTEM_MEMORY_REPORT, &snmpSystemMemoryReport);

    *nmemtype = snmpSystemMemoryReport.nEntries;

    for(i = 0; (i < *nmemtype) && (i < VL_SNMP_HOST_MIB_MEMORY_TYPE_MAX); i++)
    {
        sysMemoryRep[i].vl_ocStbHostSystemMemoryReportMemoryType = (SystemMemorytype)(snmpSystemMemoryReport.aEntries[i].memoryType);
        sysMemoryRep[i].vl_ocStbHostSystemMemoryReportMemorySize = snmpSystemMemoryReport.aEntries[i].memSize;
    }
    return 1; //Mamidi:042209
}


/** ocStbHostCCAppInfoTable  */
int Avinterface::vlGet_ocStbHostCCAppInfoTableData(SNMPocStbHostCCAppInfoTable_t* CCAppInfolist,  int *nitems)
{
        VL_AUTO_LOCK(m_lock);
        /**DUMMY values OCHD2.1*/
        
    static int nAppInfoPages = 0;
        
    if (! IsUpdateReqd_CheckStatus(vlg_timeCableCardAppInfoLastUpdated, TIME_GAP_FOR_CARD_UPDATE_IN_SECONDS))
    {
        *nitems = nAppInfoPages;
        //auto_unlock_ptr(m_lock);
        return 1;
    }
    *nitems = 1;
    vlCableCardAppInfo_t *appInfo;
    //appInfo =  (vlCableCardAppInfo_t *)malloc(sizeof(vlCableCardAppInfo_t));
    rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(vlCableCardAppInfo_t), (void**)&appInfo);

        //vlMemSet(appInfo, 0, sizeof(vlCableCardAppInfo_t), sizeof(vlCableCardAppInfo_t));
        memset(appInfo, 0, sizeof(vlCableCardAppInfo_t));
        if( IPC_CLIENT_SNMPGetApplicationInfoWithPages(appInfo) != 0)
        {
            SNMP_DEBUGPRINT("SNMPGetApplicationInfo failed.\n");
        }
        else
        {
            int i = 0;
            nAppInfoPages = appInfo->CardNumberOfApps;
            *nitems = nAppInfoPages;
            SNMP_DEBUGPRINT("appInfo->CardNumberOfApps = %d\n", appInfo->CardNumberOfApps);
            for(i = 0; (i < appInfo->CardNumberOfApps) && (i < 12); i++)
            {                
                SNMP_DEBUGPRINT("appInfo->Apps[i].AppType = %d\n", appInfo->Apps[i].AppType);
                SNMP_DEBUGPRINT("appInfo->Apps[i].VerNum = %d\n", appInfo->Apps[i].VerNum);
                SNMP_DEBUGPRINT("appInfo->Apps[i].AppNameLen = %d\n", appInfo->Apps[i].AppNameLen);
                SNMP_DEBUGPRINT("appInfo->Apps[i].AppUrlLen = %d\n", appInfo->Apps[i].AppUrlLen);
				SNMP_DEBUGPRINT("appInfo->Apps[i].nSubPages = %d\n", appInfo->Apps[i].nSubPages);
				SNMP_DEBUGPRINT("appInfo->Apps[i].AppUrl = %s\n", appInfo->Apps[i].AppUrl);


                CCAppInfolist[i].vl_ocStbHostCCApplicationType = appInfo->Apps[i].AppType;
                CCAppInfolist[i].vl_ocStbHostCCApplicationVersion = appInfo->Apps[i].VerNum;
                vlMemCpy(CCAppInfolist[i].vl_ocStbHostCCApplicationName,appInfo->Apps[i].AppName , appInfo->Apps[i].AppNameLen, API_CHRMAX);
                CCAppInfolist[i].vl_ocStbHostCCApplicationName_len = appInfo->Apps[i].AppNameLen;
        //vlMemSet(CCAppInfolist[i].vl_ocStbHostCCAppInfoPage,0,sizeof(CCAppInfolist[i].vl_ocStbHostCCAppInfoPage),sizeof(CCAppInfolist[i].vl_ocStbHostCCAppInfoPage));
        memset(CCAppInfolist[i].vl_ocStbHostCCAppInfoPage,0,sizeof(CCAppInfolist[i].vl_ocStbHostCCAppInfoPage));
                vlMemCpy(CCAppInfolist[i].vl_ocStbHostCCAppInfoPage,appInfo->Apps[i].AppUrl, appInfo->Apps[i].AppUrlLen, sizeof(CCAppInfolist[i].vl_ocStbHostCCAppInfoPage));
                CCAppInfolist[i].vl_ocStbHostCCAppInfoPage_len = appInfo->Apps[i].AppUrlLen; // Required for correctness though not used.
                CCAppInfolist[i].vl_ocStbHostCCAppInfoPage_nSubPages = appInfo->Apps[i].nSubPages;
        }
    }
    //free(appInfo);
    rmf_osal_memFreeP(RMF_OSAL_MEM_POD, appInfo);
    getsystemUPtime(&vlg_timeCableCardAppInfoLastUpdated);

    //auto_unlock_ptr(m_lock);
    return 1;
}

int Avinterface::vlSet_ocStbHostJvmInfo(SJVMinfo_t * pJvmInfo)
{
    VL_AUTO_LOCK(m_lock);
    if(NULL != pJvmInfo)
    {
        m_jvmInfo = *pJvmInfo;
    }
    //auto_unlock_ptr(m_lock);
    return 1;
}


int Avinterface::vlGet_ocStbHostJvmInfo(SJVMinfo_t * pJvmInfo)
{
    VL_AUTO_LOCK(m_lock);
    if(NULL != pJvmInfo)
    {
        *pJvmInfo = m_jvmInfo;
    }
    //auto_unlock_ptr(m_lock);
    return 1;
}

using namespace std;
int Avinterface::vlSet_ocStbHostSwApplicationInfoTable_set_list(int nApps, SNMPHOSTSoftwareApplicationInfoTable_t * pAppInfolist)
{
    VL_AUTO_LOCK(m_lock);
    if(NULL != pAppInfolist)
    {
         int i = 0;
         string defaultApp[5] = {"ASetup", "ATuning", "VLDiagnostics", "VLAppLauncher", "mainXlet"};
         string avilableapp;
   
         m_hostSwAppInfo.clear();
         for(i = 0; i < nApps; i++)
         {
             SNMPocStbHostSoftwareApplicationInfoTable_t appInfo;
             memset(&appInfo,0,sizeof(appInfo));
             avilableapp.clear();
             avilableapp.insert(0,pAppInfolist[i].vl_ocStbHostSoftwareAppNameString);
             if((avilableapp == defaultApp[0]) || (avilableapp == defaultApp[1]) || (avilableapp == defaultApp[2]) || (avilableapp == defaultApp[3]) || (avilableapp == defaultApp[4]))
             {
                 //count_defaultapp++;
                 //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "defatult Application %s =  %s \t %s %d\n ",defaultApp[appidefault].c_str(),avilableapp.c_str(),  __FUNCTION__, __LINE__);
                 continue;
             }
             appInfo.vl_ocStbHostSoftwareApplicationInfoIndex               = pAppInfolist[i].vl_ocStbHostSoftwareApplicationInfoIndex;
             vlStrCpy(appInfo.vl_ocStbHostSoftwareAppNameString,              pAppInfolist[i].vl_ocStbHostSoftwareAppNameString, sizeof(appInfo.vl_ocStbHostSoftwareAppNameString));
             appInfo.vl_ocStbHostSoftwareAppNameString_len                  = strlen(pAppInfolist[i].vl_ocStbHostSoftwareAppNameString);
             vlStrCpy(appInfo.vl_ocStbHostSoftwareAppVersionNumber,           pAppInfolist[i].vl_ocStbHostSoftwareAppVersionNumber, sizeof(appInfo.vl_ocStbHostSoftwareAppVersionNumber));
             appInfo.vl_ocStbHostSoftwareAppVersionNumber_len               = strlen(pAppInfolist[i].vl_ocStbHostSoftwareAppVersionNumber);
             appInfo.vl_ocStbHostSoftwareStatus                             = pAppInfolist[i].vl_ocStbHostSoftwareStatus;;
             appInfo.vl_ocStbHostSoftwareOrganizationId[0]                = ((pAppInfolist[i].vl_ocStbHostSoftwareOrganizationId >> 24) & 0xFF); //4-BYTES;
             appInfo.vl_ocStbHostSoftwareOrganizationId[1]                = ((pAppInfolist[i].vl_ocStbHostSoftwareOrganizationId >> 16) & 0xFF); //4-BYTES;
             appInfo.vl_ocStbHostSoftwareOrganizationId[2]                = ((pAppInfolist[i].vl_ocStbHostSoftwareOrganizationId >>  8) & 0xFF); //4-BYTES;
             appInfo.vl_ocStbHostSoftwareOrganizationId[3]                = ((pAppInfolist[i].vl_ocStbHostSoftwareOrganizationId >>  0) & 0xFF); //4-BYTES;
             appInfo.vl_ocStbHostSoftwareApplicationId[0]                 = ((pAppInfolist[i].vl_ocStbHostSoftwareApplicationId  >>  8) & 0xFF); //2-BYTES;
             appInfo.vl_ocStbHostSoftwareApplicationId[1]                 = ((pAppInfolist[i].vl_ocStbHostSoftwareApplicationId  >>  0) & 0xFF); //2-BYTES;

             /*if(appInfo.vl_ocStbHostSoftwareStatus < loaded)
             {
                 appInfo.vl_ocStbHostSoftwareStatus = notLoaded;
             }*/

             if(0 == stricmp("Okay", pAppInfolist[i].vl_ocStbHostSoftwareApplicationSigStatus))
             {
                 appInfo.vl_ocStbHostSoftwareApplicationSigStatus = SoftwareAPP_SigStatus_okay;
             }
             else if(0 == stricmp("Error", pAppInfolist[i].vl_ocStbHostSoftwareApplicationSigStatus))
             {
                 appInfo.vl_ocStbHostSoftwareApplicationSigStatus = SoftwareAPP_SigStatus_error;
             }
             else
             {
                 appInfo.vl_ocStbHostSoftwareApplicationSigStatus = SoftwareAPP_SigStatus_other;
             }

             m_hostSwAppInfo.push_back(appInfo);
         }
    }
    //auto_unlock_ptr(m_lock);
    return 1;
}


int Avinterface::vlMpeosSet_ocStbHostSwApplicationInfoTable_set_list(int nApps, VLMPEOS_SNMPHOSTSoftwareApplicationInfoTable_t * pAppInfolist)
{
    VL_AUTO_LOCK(m_lock);
    if(NULL != pAppInfolist)
    {
         int i = 0;
         string defaultApp[5] = {"ASetup", "ATuning", "VLDiagnostics", "VLAppLauncher", "mainXlet"};
         string avilableapp;
   
         m_hostSwAppInfo.clear();
         for(i = 0; i < nApps; i++)
         {
             SNMPocStbHostSoftwareApplicationInfoTable_t appInfo;
             memset(&appInfo,0,sizeof(appInfo));
             avilableapp.clear();
             avilableapp.insert(0,pAppInfolist[i].vl_ocStbHostSoftwareAppNameString);
             if((avilableapp == defaultApp[0]) || (avilableapp == defaultApp[1]) || (avilableapp == defaultApp[2]) || (avilableapp == defaultApp[3]) || (avilableapp == defaultApp[4]))
             {
                 //count_defaultapp++;
                 //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "defatult Application %s =  %s \t %s %d\n ",defaultApp[appidefault].c_str(),avilableapp.c_str(),  __FUNCTION__, __LINE__);
                 continue;
             }
             appInfo.vl_ocStbHostSoftwareApplicationInfoIndex               = pAppInfolist[i].vl_ocStbHostSoftwareApplicationInfoIndex;
             vlStrCpy(appInfo.vl_ocStbHostSoftwareAppNameString,              pAppInfolist[i].vl_ocStbHostSoftwareAppNameString, sizeof(appInfo.vl_ocStbHostSoftwareAppNameString));
             appInfo.vl_ocStbHostSoftwareAppNameString_len                  = strlen(pAppInfolist[i].vl_ocStbHostSoftwareAppNameString);
             vlStrCpy(appInfo.vl_ocStbHostSoftwareAppVersionNumber,           pAppInfolist[i].vl_ocStbHostSoftwareAppVersionNumber, sizeof(appInfo.vl_ocStbHostSoftwareAppVersionNumber));
             appInfo.vl_ocStbHostSoftwareAppVersionNumber_len               = strlen(pAppInfolist[i].vl_ocStbHostSoftwareAppVersionNumber);
             appInfo.vl_ocStbHostSoftwareStatus                             = (SoftwareAPP_status)(pAppInfolist[i].vl_ocStbHostSoftwareStatus);
             appInfo.vl_ocStbHostSoftwareOrganizationId[0]                = ((pAppInfolist[i].vl_ocStbHostSoftwareOrganizationId >> 24) & 0xFF); //4-BYTES;
             appInfo.vl_ocStbHostSoftwareOrganizationId[1]                = ((pAppInfolist[i].vl_ocStbHostSoftwareOrganizationId >> 16) & 0xFF); //4-BYTES;
             appInfo.vl_ocStbHostSoftwareOrganizationId[2]                = ((pAppInfolist[i].vl_ocStbHostSoftwareOrganizationId >>  8) & 0xFF); //4-BYTES;
             appInfo.vl_ocStbHostSoftwareOrganizationId[3]                = ((pAppInfolist[i].vl_ocStbHostSoftwareOrganizationId >>  0) & 0xFF); //4-BYTES;
             appInfo.vl_ocStbHostSoftwareApplicationId[0]                 = ((pAppInfolist[i].vl_ocStbHostSoftwareApplicationId  >>  8) & 0xFF); //2-BYTES;
             appInfo.vl_ocStbHostSoftwareApplicationId[1]                 = ((pAppInfolist[i].vl_ocStbHostSoftwareApplicationId  >>  0) & 0xFF); //2-BYTES;

         /*if(appInfo.vl_ocStbHostSoftwareStatus < loaded)
             {
                 appInfo.vl_ocStbHostSoftwareStatus = notLoaded;
             }*/

             if(0 == stricmp("Okay", pAppInfolist[i].vl_ocStbHostSoftwareApplicationSigStatus))
             {
                 appInfo.vl_ocStbHostSoftwareApplicationSigStatus = SoftwareAPP_SigStatus_okay;
             }
             else if(0 == stricmp("Error", pAppInfolist[i].vl_ocStbHostSoftwareApplicationSigStatus))
             {
                 appInfo.vl_ocStbHostSoftwareApplicationSigStatus = SoftwareAPP_SigStatus_error;
             }
             else
             {
                 appInfo.vl_ocStbHostSoftwareApplicationSigStatus = SoftwareAPP_SigStatus_other;
             }

             m_hostSwAppInfo.push_back(appInfo);
         }
    }
    //auto_unlock_ptr(m_lock);
    return 1;
}

int Avinterface::vlSet_ocStbHostSoftwareApplicationInfoTable_set_list(int nApps, SNMPocStbHostSoftwareApplicationInfoTable_t * pAppInfolist)
{
    VL_AUTO_LOCK(m_lock);
    
    if(NULL != pAppInfolist)
    {
        int i = 0;
        m_hostSwAppInfo.clear();
        for(i = 0; i < nApps; i++)
        {
            m_hostSwAppInfo.push_back(pAppInfolist[i]);
        }
    }

    //auto_unlock_ptr(m_lock);
    return 1;
}

/** ocStbHostSoftwareApplicationInfoTable */
int
Avinterface::vlGet_ocStbHostSoftwareApplicationInfoTableData(SNMPocStbHostSoftwareApplicationInfoTable_t* swapplication, int *nswappl)
{
    VL_AUTO_LOCK(m_lock);
    int i = 0;
    *nswappl = vlMin(m_hostSwAppInfo.size(), MAX_ROWS);

    for(i = 0; i < *nswappl; i++)
    {
        swapplication[i] = m_hostSwAppInfo[i];

        vlStrCpy(swapplication[i].vl_ocStbHostSoftwareAppNameString, m_hostSwAppInfo[i].vl_ocStbHostSoftwareAppNameString, sizeof(swapplication[i].vl_ocStbHostSoftwareAppNameString));
        swapplication[i].vl_ocStbHostSoftwareAppNameString_len = strlen(swapplication[i].vl_ocStbHostSoftwareAppNameString);

        vlStrCpy(swapplication[i].vl_ocStbHostSoftwareAppVersionNumber, m_hostSwAppInfo[i].vl_ocStbHostSoftwareAppVersionNumber, sizeof(swapplication[i].vl_ocStbHostSoftwareAppVersionNumber));
        swapplication[i].vl_ocStbHostSoftwareAppVersionNumber_len = strlen(swapplication[i].vl_ocStbHostSoftwareAppVersionNumber);

        swapplication[i].vl_ocStbHostSoftwareApplicationInfoIndex = i+1;
    }

    //auto_unlock_ptr(m_lock);
    return 1;
}



/** ocStbHostSoftwareApplicationInfoTable */
void
Avinterface::vlGet_ocStbHostSoftwareApplicationSigstatus(SNMPocStbHostSoftwareApplicationInfoTable_t* swapplicationStaticstatus)
{
    VL_AUTO_LOCK(m_lock);
    //DUMMY values.
    swapplicationStaticstatus->vl_ocStbHostSoftwareApplicationInfoSigLastReadStatus =SOFTWAREAPPLICATIONINFOSIGLASTREADSTATUS_UNKNOWN;
    vlStrCpy(swapplicationStaticstatus->vl_ocStbHostSoftwareApplicationInfoSigLastReceivedTime,"00-00-0000" , sizeof(swapplicationStaticstatus->vl_ocStbHostSoftwareApplicationInfoSigLastReceivedTime));
    //auto_unlock_ptr(m_lock);
}

void Avinterface::vlGet_Serialnum(char* Serialno, int nChars)
{
           VL_AUTO_LOCK(m_lock);
        VL_SNMP_VAR_SERIAL_NUMBER obj;

        vlSnmpGetHostInfo(VL_SNMP_VAR_TYPE_ocStbHostSerialNumber, &obj);

        vlStrCpy(Serialno, obj.szSerialNumber, nChars);
        //auto_unlock_ptr(m_lock);
}
/*     ifTable  and  iPnettoPhysicalTable    */
extern int HOST_STB_ifAdminStatus_Disable;
extern int CABLE_CARD_ifAdminStatus_Disable;
char * getInterfaceNameFromConfig(const char * pszInterfaceTag)
{
    char szBuf[256];
    static char szEthName[32] = "";
    FILE * fp = fopen("/etc/device.properties", "r");
    if(NULL != fp)
    {
        while(NULL != fgets(szBuf, sizeof(szBuf), fp))
        {
            char * pszTag = NULL;
            if(NULL != (pszTag = strstr(szBuf, pszInterfaceTag)))
            {
                char * pszEqual = NULL;
                if(NULL != (pszEqual = strstr(pszTag, "=")))
                {
                    sscanf(pszEqual+1, "%s", szEthName);
                }
            }
        }
        fclose(fp);
    }
    
    return szEthName;
}
extern long vlg_tcMocaIpLastChange;

#define VL_COPY_V6_ADDRESS(type)                                                                                \
    memcpy(pHostIpInfo->aBytIpV6##type##Address, ipaddr, sizeof(pHostIpInfo->aBytIpV6##type##Address)        \
                                                           );       \
    strncpy(pHostIpInfo->szIpV6##type##Address, addr6, VL_IPV6_ADDR_STR_SIZE );            \
    pHostIpInfo->szIpV6##type##Address[ VL_IPV6_ADDR_STR_SIZE -1 ] = 0; \
    pHostIpInfo->nIpV6##type##Plen = plen
#define IPV6_ADDR_ANY           0x0000U
#define IPV6_ADDR_LOOPBACK      0x0010U
#define IPV6_ADDR_LINKLOCAL     0x0020U
#define IPV6_ADDR_SITELOCAL     0x0040U
#define IPV6_ADDR_COMPATv4      0x0080U
#define VL_ZERO_MEMORY(str) memset(&(str), 0, sizeof(str))
char * vlGetIpScopeName(int type)
{
    switch(type)
    {
        case IPV6_ADDR_ANY      : return "global";
        case IPV6_ADDR_LOOPBACK : return "loop";
        case IPV6_ADDR_LINKLOCAL: return "link";
        case IPV6_ADDR_SITELOCAL: return "site";
        case IPV6_ADDR_COMPATv4 : return "v4 compatible";
    }
    
    return "";
}

void vlSnmpGetHostIntefaceInfo(const char * pszInterfaceName, VL_HOST_IP_INFO * pHostIpInfo)
{
    if(NULL == pszInterfaceName)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SNMP","%s(): pszInterfaceName is NULL.\n", __FUNCTION__);
        return;
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SNMP","%s(): Getting Interface data for '%s'.\n", __FUNCTION__, pszInterfaceName);
    }
    if(NULL != pHostIpInfo)
    {
        memset(pHostIpInfo,0,sizeof(*pHostIpInfo));
        strcpy((char*)(pHostIpInfo->szIpAddress ), "0.0.0.0");
        strcpy((char*)(pHostIpInfo->szSubnetMask), "0.0.0.0");

        pHostIpInfo->iInstance = 0;
        strncpy(pHostIpInfo->szIfName, pszInterfaceName, VL_IF_NAME_SIZE);
        pHostIpInfo->szIfName [VL_IF_NAME_SIZE-1] = 0;

        pHostIpInfo->ipAddressType = VL_XCHAN_IP_ADDRESS_TYPE_IPV4;
        {//getip
            int result = 0;
            int  sock = socket(AF_INET, SOCK_DGRAM, 0);
            unsigned long ipAddress = 0;

            if(-1 != sock )
            {
                if( sock == 0 ) 
                {
           	 	RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SNMP",
           	 	"\n\n\n%s: Closed file descriptor 0 !!\n\n\n\n",__FUNCTION__);
                }				
                struct ifreq ifr;
                struct sockaddr_in sai;
                memset(&ifr,0,sizeof(ifr));
                memset(&sai,0,sizeof(sai));
                struct sockaddr_in  *sin  = (struct sockaddr_in  *)&(ifr.ifr_addr);
                struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&(ifr.ifr_addr);
                sai.sin_family = AF_INET;
                sai.sin_port = 0;

//                strcpy(ifr.ifr_name, pHostIpInfo->szIfName);
                strncpy(ifr.ifr_name, pHostIpInfo->szIfName, IFNAMSIZ);
                ifr.ifr_name [ IFNAMSIZ - 1 ] = 0;		

                
                sin->sin_family = AF_INET;
                if ((result = ioctl(sock, SIOCGIFADDR, &ifr)) < 0)
                {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP","%s(): Cannot get IPV4 addr for %s, ret %d\n", __FUNCTION__, pHostIpInfo->szIfName, result);
                    pHostIpInfo->ipAddressType = VL_XCHAN_IP_ADDRESS_TYPE_NONE;
                }
                else
                {
                    memcpy((char *)&sai, (char *) &ifr.ifr_ifru.ifru_addr, sizeof(struct sockaddr));
                    memcpy(pHostIpInfo->aBytIpAddress, &sai.sin_addr.s_addr, sizeof(sai.sin_addr.s_addr));
                    char * pIpAddr = inet_ntoa(sai.sin_addr);
                    if(NULL != pIpAddr)
                    {
//                        strcpy(pHostIpInfo->szIpAddress, pIpAddr);
                        strncpy( pHostIpInfo->szIpAddress, pIpAddr, VL_IP_ADDR_STR_SIZE );
                        pHostIpInfo->szIpAddress [VL_IP_ADDR_STR_SIZE - 1] = 0;
                     snprintf(pHostIpInfo->szIpAddress,sizeof(pHostIpInfo->szIpAddress), "%d.%d.%d.%d",
                            pHostIpInfo->aBytIpAddress[0], pHostIpInfo->aBytIpAddress[1],
                            pHostIpInfo->aBytIpAddress[2], pHostIpInfo->aBytIpAddress[3]);
                        //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP","%s(): Got IPV4 Address '%s'\n", __FUNCTION__, pHostIpInfo->szIpAddress);
                        ipAddress = VL_VALUE_4_FROM_ARRAY(pHostIpInfo->aBytIpAddress);
                    
                        pHostIpInfo->aBytIpV6v4Address[10] = 0xFF;
                        pHostIpInfo->aBytIpV6v4Address[11] = 0xFF;
                        pHostIpInfo->aBytIpV6v4Address[12] = pHostIpInfo->aBytIpAddress[0];
                        pHostIpInfo->aBytIpV6v4Address[13] = pHostIpInfo->aBytIpAddress[1];
                        pHostIpInfo->aBytIpV6v4Address[14] = pHostIpInfo->aBytIpAddress[2];
                        pHostIpInfo->aBytIpV6v4Address[15] = pHostIpInfo->aBytIpAddress[3];
                    
                        inet_ntop(AF_INET6,pHostIpInfo->aBytIpV6v4Address,
                                  pHostIpInfo->szIpV6v4Address,sizeof(pHostIpInfo->szIpV6v4Address));

                        //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP","%s(): Got IPV6 V4 Address '%s'\n", __FUNCTION__, pHostIpInfo->szIpV6v4Address);
                    }
                }

                ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
                if ((result = ioctl(sock, SIOCGIFNETMASK, &ifr)) < 0)
                {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP","%s(): Cannot get subnet mask for %s, ret %d\n", __FUNCTION__, pHostIpInfo->szIfName, result);
                }
                else
                {
                    memcpy((char *)&sai, (char *) &ifr.ifr_ifru.ifru_netmask, sizeof(struct sockaddr));
                    char * pSubnetMask = inet_ntoa(sai.sin_addr);
                    memcpy(pHostIpInfo->aBytSubnetMask, &sai.sin_addr.s_addr, sizeof(sai.sin_addr.s_addr));
                    if(NULL != pSubnetMask)
                    {
                        int nMaskLen = 96, iMask = 0;
                        unsigned long subnetMask = VL_VALUE_4_FROM_ARRAY(pHostIpInfo->aBytSubnetMask);
                        strncpy(pHostIpInfo->szSubnetMask, pSubnetMask, VL_IP_ADDR_STR_SIZE );
                        pHostIpInfo->szSubnetMask [VL_IP_ADDR_STR_SIZE - 1] = 0;	
                        									
                        snprintf(pHostIpInfo->szSubnetMask, sizeof(pHostIpInfo->szSubnetMask), "%d.%d.%d.%d",
                            pHostIpInfo->aBytSubnetMask[0], pHostIpInfo->aBytSubnetMask[1],
                            pHostIpInfo->aBytSubnetMask[2], pHostIpInfo->aBytSubnetMask[3]);
                        //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP","%s(): Got SubNetMask '%s'\n", __FUNCTION__, pHostIpInfo->szSubnetMask);
                        
                        // calculate V6 mask
                        while(0 != ((subnetMask<<iMask)&0xFFFFFFFF)) iMask++;
                        pHostIpInfo->nIpV6v4Plen = nMaskLen + iMask;
                        
                        //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP","%s(): Got IPV6 V4 Mask '%d'\n", __FUNCTION__, pHostIpInfo->nIpV6v4Plen);
                    }
                }

                if ((result = ioctl(sock, SIOCGIFHWADDR, &ifr)) < 0)
                {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP","%s():Cannot get MAC_ADDRESS for %s, ret %d\n", __FUNCTION__, pHostIpInfo->szIfName, result);
                }
                else
                {
                    unsigned char * pMacAddress = (unsigned char*)(ifr.ifr_hwaddr.sa_data);
                    memcpy(pHostIpInfo->aBytMacAddress, pMacAddress, sizeof(pHostIpInfo->aBytMacAddress));
                    snprintf(pHostIpInfo->szMacAddress, sizeof(pHostIpInfo->szMacAddress), "%02X:%02X:%02X:%02X:%02X:%02X",
                        pHostIpInfo->aBytMacAddress[0], pHostIpInfo->aBytMacAddress[1],
                        pHostIpInfo->aBytMacAddress[2], pHostIpInfo->aBytMacAddress[3],
                        pHostIpInfo->aBytMacAddress[4], pHostIpInfo->aBytMacAddress[5]);
                    //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP","%s(): Got MacAddress '%s'\n", __FUNCTION__, pHostIpInfo->szMacAddress);
                    
                    pHostIpInfo->aBytIpV6LinkAddress[ 0] = 0xFE;
                    pHostIpInfo->aBytIpV6LinkAddress[ 1] = 0x80;
                    pHostIpInfo->aBytIpV6LinkAddress[ 8] = ((pHostIpInfo->aBytMacAddress[0]) ^ (0x02));
                    pHostIpInfo->aBytIpV6LinkAddress[ 9] = pHostIpInfo->aBytMacAddress[1];
                    pHostIpInfo->aBytIpV6LinkAddress[10] = pHostIpInfo->aBytMacAddress[2];
                    pHostIpInfo->aBytIpV6LinkAddress[11] = 0xFF;
                    pHostIpInfo->aBytIpV6LinkAddress[12] = 0xFE;
                    pHostIpInfo->aBytIpV6LinkAddress[13] = pHostIpInfo->aBytMacAddress[3];
                    pHostIpInfo->aBytIpV6LinkAddress[14] = pHostIpInfo->aBytMacAddress[4];
                    pHostIpInfo->aBytIpV6LinkAddress[15] = pHostIpInfo->aBytMacAddress[5];
                    
                    inet_ntop(AF_INET6,pHostIpInfo->aBytIpV6LinkAddress,
                              pHostIpInfo->szIpV6LinkAddress,sizeof(pHostIpInfo->szIpV6LinkAddress));
                    
                    //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP","%s(): Got IPV6 Link Address '%s'\n", __FUNCTION__, pHostIpInfo->szIpV6LinkAddress);
                    
                    // calculate V6 mask
                    pHostIpInfo->nIpV6LinkPlen = 64;
                        
                    //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP","%s(): Got IPV6 Link Mask '%d'\n", __FUNCTION__, pHostIpInfo->nIpV6LinkPlen);
                }
                
                if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0)
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SNMP","%s():SIOCGIFFLAGS failed with error '%s'\n", __FUNCTION__, strerror(errno));
                }
                else
                {
                    pHostIpInfo->lFlags      = ifr.ifr_flags;
                    pHostIpInfo->lLastChange = 0;
                }
                
                close(sock);
            } // if(sock)
            {// IPV6
                #define PROC_IFINET6_PATH "/proc/net/if_inet6"
                
                                /* refer to: linux/include/net/ipv6.h */
                FILE *fp;
                char addr[16][4];
                unsigned long ipchar = 0;
                unsigned char ipaddr[16];
                unsigned int index, plen, scope, flags;
                char ifname[128];
                char ifCorrected[128];
                char addr6[128];
                char str[128];
                int i = 0;
    
                memset(str,0,sizeof(str));
                memset(ifCorrected,0,sizeof(ifCorrected));
                strncpy(ifCorrected, pHostIpInfo->szIfName, sizeof(ifCorrected)-1);
                char *pszColon = strchr(ifCorrected, ':');
                if(NULL != pszColon) *pszColon = '\0';

                if ((fp = fopen(PROC_IFINET6_PATH, "r")) != NULL)
                {
                    //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP","Loop\n");
                    while((fgets(str, sizeof(str), fp)) != NULL)
                    {
                        sscanf(str, 
                               "%2s%2s%2s%2s%2s%2s%2s%2s%2s%2s%2s%2s%2s%2s%2s%2s %02x %02x %02x %02x %8s",
                               addr[0],addr[1],addr[2],addr[3],addr[4],addr[5],addr[6],addr[7],
                               addr[8],addr[9],addr[10],addr[11],addr[12],addr[13],addr[14],addr[15],
                               &index, &plen, &scope, &flags, ifname);
            
                        if(0 != strcmp(ifCorrected, ifname)) continue;
                        
                        for(i = 0; i < 16; i++)
                        {
                            sscanf(addr[i], "%x", &ipchar);
                            ipaddr[i] = (unsigned char)(ipchar);
                        }
            
                        inet_ntop(AF_INET6, ipaddr, addr6, sizeof(addr6));
            
                        if(IPV6_ADDR_LINKLOCAL != scope)
                        {
                            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP","%s(): %8s: %39s/%d scope:%s\n", __FUNCTION__, ifname, addr6, plen, vlGetIpScopeName(scope));
                        }
                        
                        switch(scope)
                        {
                            case IPV6_ADDR_ANY:
                            {
                                VL_COPY_V6_ADDRESS(Global);
                                pHostIpInfo->ipAddressType = VL_XCHAN_IP_ADDRESS_TYPE_IPV6;
                            }
                            break;
                            
                            case IPV6_ADDR_LINKLOCAL:
                            {
                                VL_COPY_V6_ADDRESS(Link);
                            }
                            break;
                            
                            case IPV6_ADDR_SITELOCAL:
                            {
                                VL_COPY_V6_ADDRESS(Site);
                            }
                            break;
                            
                            case IPV6_ADDR_COMPATv4:
                            {
                                VL_COPY_V6_ADDRESS(v4);
                            }
                            break;
                        }
                    }// while
                    if(NULL != fp) fclose(fp);
                }// (NULL != fopen())
            }// IPV6
#if 1
            {// STATS
                #define PROC_NET_DEV_PATH "/proc/net/dev"
                
                FILE *fp;
                char str[256];
                char szIfName[32];
                char szInUnicastPkts[32];
                char szOutUnicastPkts[32];
                char szInUnicastBytes[32];
                char szOutUnicastBytes[32];
                char szInErrors[32];
                char szOutErrors[32];
                char szInDiscards[32];
                char szOutDiscards[32];
                char szDummy[32];
                
                VL_ZERO_MEMORY(str              );
                VL_ZERO_MEMORY(szIfName         );
                
                VL_ZERO_MEMORY(szInUnicastPkts  );
                VL_ZERO_MEMORY(szOutUnicastPkts );
                VL_ZERO_MEMORY(szInUnicastBytes );
                VL_ZERO_MEMORY(szOutUnicastBytes);
                VL_ZERO_MEMORY(szInErrors       );
                VL_ZERO_MEMORY(szOutErrors      );
                VL_ZERO_MEMORY(szInDiscards     );
                VL_ZERO_MEMORY(szOutDiscards    );
                
                VL_ZERO_MEMORY(szDummy          );

                if ((fp = fopen(PROC_NET_DEV_PATH, "r")) != NULL)
                {
                    //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP","Loop\n");
                    while((fgets(str, sizeof(str), fp)) != NULL)
                    {
                        char * pszColon = strchr(str, ':');
                        if(NULL == pszColon) continue;
                        *pszColon = ' ';
                        sscanf(str, 
                               "%s%s%s%s%s%s%s%s%s%s%s%s%s",
                               szIfName,
                               szInUnicastBytes, szInUnicastPkts,
                               szInErrors, szInDiscards,
                               szDummy, szDummy, szDummy, szDummy,
                               szOutUnicastBytes, szOutUnicastPkts,
                               szOutErrors, szOutDiscards);
            
                        if(0 != strcmp(pHostIpInfo->szIfName, szIfName)) continue;
                        /*RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP","%s: %s, %s, %s, %s, %s, %s, %s, %s, %s\n", __FUNCTION__, szIfName,
                               szInUnicastBytes, szInUnicastPkts,
                               szInErrors, szInDiscards,
                               szOutUnicastBytes, szOutUnicastPkts,
                               szOutErrors, szOutDiscards);*/
                        pHostIpInfo->stats.ifInUcastPkts    = strtoul(szInUnicastPkts   , NULL, 10);
                        pHostIpInfo->stats.ifOutUcastPkts   = strtoul(szOutUnicastPkts  , NULL, 10);
                        pHostIpInfo->stats.ifInOctets       = strtoul(szInUnicastBytes  , NULL, 10);
                        pHostIpInfo->stats.ifOutOctets      = strtoul(szOutUnicastBytes , NULL, 10);
                        pHostIpInfo->stats.ifInErrors       = strtoul(szInErrors        , NULL, 10);
                        pHostIpInfo->stats.ifOutErrors      = strtoul(szOutErrors       , NULL, 10);
                        pHostIpInfo->stats.ifInDiscards     = strtoul(szInDiscards      , NULL, 10);
                        pHostIpInfo->stats.ifOutDiscards    = strtoul(szOutDiscards     , NULL, 10);
                        /*RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP","%s: %s, %d, %d, %d, %d\n", __FUNCTION__, szIfName,
                               pHostIpInfo->stats.ifInOctets,
                               pHostIpInfo->stats.ifInUcastPkts,
                               pHostIpInfo->stats.ifOutOctets,
                               pHostIpInfo->stats.ifOutUcastPkts);*/
                    }// while
                    if(NULL != fp) fclose(fp);
                }// (NULL != fopen())
                else
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SNMP","%s():fopen('%s') failed with error '%s'\n", __FUNCTION__, PROC_NET_DEV_PATH, strerror(errno));
                }
            }// STATS
#endif
        }// getip
    }// (NULL != pHostIpInfo)
}

int Avinterface::vlGet_ifIpTableData(SNMPifTable_t* ifdata, SNMPipNetToPhysicalTable_t* ipnettophy, int *iftabelcheck)
{
         VL_AUTO_LOCK(m_lock);
    *iftabelcheck = 3;
   /**for HOST STB IFtable INFO*/

    long ipnetTimeStamp = 0;
    long cardipnetTimeStamp = 0;
    //entry->ifIndex;
    {
        /**--------HOST ADDRESS INFO----------*/

        /* iftable Indix*/
        ifdata[0].vl_ifIndex = 1;
        /*
         * Column values
        */
        ifdata[0].vl_ifindex = 1;//Index for IpNetToPhyscialTable
        ifdata[0].vl_ifDescr_len = strlen(ifTableDesc[0]);
        vlMemCpy(ifdata[0].vl_ifDescr,ifTableDesc[0] ,ifdata[0].vl_ifDescr_len, API_CHRMAX);
        ifdata[0].vl_ifType = IFTYPE_OTHER;
        ifdata[0].vl_ifMtu = 0;
        ifdata[0].vl_ifSpeed = 0;
        /*To get the cable HOST MAC and IP addresss call mfr modules*/
        VL_HOST_IP_INFO phostipinfo;
        IPC_CLIENT_vlXchanGetDsgIpInfo(&phostipinfo);
        //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "Avinterface::vlGet_ifIpTableData: Got IP Address: '%s'\n", phostipinfo.szIpAddress);
        VL_HOST_IP_STATS phostipstats;
        //vlMemSet(&phostipstats, 0, sizeof(phostipstats), sizeof(phostipstats));
        memset(&phostipstats, 0, sizeof(phostipstats));
        // Feb-09-2010: Stats from eCM not used in legacy IPU based network.
        if(IPC_CLIENT_vlIsDsgMode()){ vlSnmpEcmGetIfStats(&phostipstats); }
        ifdata[0].vl_ifMtu              = 0;  // Dec-02-2009: Changed to 0 as per CFR Table 14.3-2
        ifdata[0].vl_ifSpeed            = 0;  // Dec-02-2009: Changed to 0 as per CFR Table 14.3-2
        // Feb-19-2010: Inverted stats reporting as per customer request.
        ifdata[0].vl_ifInOctets         = phostipstats.ifOutOctets       ;
        ifdata[0].vl_ifInUcastPkts      = phostipstats.ifOutUcastPkts    ;
        ifdata[0].vl_ifInNUcastPkts     = phostipstats.ifOutNUcastPkts   ;
        ifdata[0].vl_ifInDiscards       = phostipstats.ifOutDiscards     ;
        ifdata[0].vl_ifInErrors         = phostipstats.ifOutErrors       ;
        ifdata[0].vl_ifInUnknownProtos  = phostipstats.ifOutUnknownProtos;
        ifdata[0].vl_ifOutOctets        = phostipstats.ifInOctets        ;
        ifdata[0].vl_ifOutUcastPkts     = phostipstats.ifInUcastPkts     ;
        ifdata[0].vl_ifOutNUcastPkts    = phostipstats.ifInNUcastPkts    ;
        ifdata[0].vl_ifOutDiscards      = phostipstats.ifInDiscards      ;
        ifdata[0].vl_ifOutErrors        = phostipstats.ifInErrors        ;

        ifdata[0].vl_ifPhysAddress_len = VL_MAC_ADDR_SIZE;
        ifdata[0].vl_ifAdminStatus = ((phostipinfo.lFlags&IFF_UP)?IFADMINSTATUS_UP:IFADMINSTATUS_DOWN);
        ifdata[0].vl_ifOperStatus = ((phostipinfo.lFlags&IFF_RUNNING)?IFOPERSTATUS_UP:IFOPERSTATUS_DOWN);
        ifdata[0].vl_old_ifAdminStatus = ifdata[0].vl_ifAdminStatus;
        ifdata[0].vl_ifLastChange = phostipinfo.lLastChange;
        //if-Table Mac details
        vlMemCpy(ifdata[0].vl_ifPhysAddress,phostipinfo.aBytMacAddress,VL_MAC_ADDR_SIZE, API_CHRMAX);
        getsystemUPtime(&ipnetTimeStamp);//for vl_ipNetToPhysicalLastUpdated
        ipnettophy[0].vl_ipNetToPhysicalLastUpdated = ipnetTimeStamp;

        //--IP-HOST info
        ipnettophy[0].vl_ipNetToPhysicalPhysAddress_len = MAC_BYTE_SIZE *sizeof(char);
        vlMemCpy(ipnettophy[0].vl_ipNetToPhysicalPhysAddress,phostipinfo.aBytMacAddress,VL_MAC_ADDR_SIZE , API_CHRMAX );

        //vlMemSet(ipnettophy[0].vl_ipNetToPhysicalNetAddress, 0, sizeof(ipnettophy[0].vl_ipNetToPhysicalNetAddress), sizeof(ipnettophy[0].vl_ipNetToPhysicalNetAddress));
        memset(ipnettophy[0].vl_ipNetToPhysicalNetAddress, 0, sizeof(ipnettophy[0].vl_ipNetToPhysicalNetAddress));
        ipnettophy[0].vl_ipNetToPhysicalNetAddress_len = 0;

        ipnettophy[0].vl_ipNetToPhysicalNetAddressType =  IPNETTOPHYSICALNETADDRESSTYPE_IPV4;
        ipnettophy[0].vl_ipNetToPhysicalNetAddress_len = sizeof(phostipinfo.aBytIpAddress);

        if( (192 == phostipinfo.aBytIpAddress[0]) &&
            (168 == phostipinfo.aBytIpAddress[1]) &&
            (100 == phostipinfo.aBytIpAddress[2]))
        {
            memset(&phostipinfo.aBytIpAddress,0,sizeof(phostipinfo.aBytIpAddress));
        }

        switch(phostipinfo.ipAddressType)
        {
            case VL_XCHAN_IP_ADDRESS_TYPE_NONE:
            {
                ipnettophy[0].vl_ipNetToPhysicalNetAddressType =  IPNETTOPHYSICALNETADDRESSTYPE_IPV4;
            }
            break;
            case VL_XCHAN_IP_ADDRESS_TYPE_IPV4:
            {
                ipnettophy[0].vl_ipNetToPhysicalNetAddressType =  IPNETTOPHYSICALNETADDRESSTYPE_IPV4;
                vlMemCpy(ipnettophy[0].vl_ipNetToPhysicalNetAddress, phostipinfo.aBytIpAddress, sizeof(phostipinfo.aBytIpAddress), API_CHRMAX);
                ipnettophy[0].vl_ipNetToPhysicalNetAddress_len = sizeof(phostipinfo.aBytIpAddress);
            }
            break;
            case VL_XCHAN_IP_ADDRESS_TYPE_IPV6:
            {
                ipnettophy[0].vl_ipNetToPhysicalNetAddressType =  IPNETTOPHYSICALNETADDRESSTYPE_IPV6;
                vlMemCpy(ipnettophy[0].vl_ipNetToPhysicalNetAddress, phostipinfo.aBytIpV6GlobalAddress, sizeof(phostipinfo.aBytIpV6GlobalAddress), API_CHRMAX);
                ipnettophy[0].vl_ipNetToPhysicalNetAddress_len = sizeof(phostipinfo.aBytIpV6GlobalAddress);
            }
            break;
            case VL_XCHAN_IP_ADDRESS_TYPE_IPV4Z:
            {
                ipnettophy[0].vl_ipNetToPhysicalNetAddressType =  IPNETTOPHYSICALNETADDRESSTYPE_IPV4Z;
                vlMemCpy(ipnettophy[0].vl_ipNetToPhysicalNetAddress, phostipinfo.aBytIpAddress, sizeof(phostipinfo.aBytIpAddress), API_CHRMAX);
                ipnettophy[0].vl_ipNetToPhysicalNetAddress_len = sizeof(phostipinfo.aBytIpAddress);
            }
            break;
            case VL_XCHAN_IP_ADDRESS_TYPE_IPV6Z:
            {
                ipnettophy[0].vl_ipNetToPhysicalNetAddressType =  IPNETTOPHYSICALNETADDRESSTYPE_IPV6Z;
                vlMemCpy(ipnettophy[0].vl_ipNetToPhysicalNetAddress, phostipinfo.aBytIpV6GlobalAddress, sizeof(phostipinfo.aBytIpV6GlobalAddress), API_CHRMAX);
                ipnettophy[0].vl_ipNetToPhysicalNetAddress_len = sizeof(phostipinfo.aBytIpV6GlobalAddress);
            }
            break;
            case VL_XCHAN_IP_ADDRESS_TYPE_DNS:
            {
                ipnettophy[0].vl_ipNetToPhysicalNetAddressType =  IPNETTOPHYSICALNETADDRESSTYPE_DNS;
            }
            break;
        }

        /*End To get the HOST MAC and IP addresss call mfr modules*/
        /* IF the Card IP will be active when below if conditon is not equeal to 0.0.0.0 then the below of IP-table and iftable will befilled*/

        /*index of IP */

        ipnettophy[0].vl_ipNetToPhysicalIfIndex = ifdata[0].vl_ifindex;
        /*
         * Column values
         */
        if(IFADMINSTATUS_UP == ifdata[0].vl_ifAdminStatus)
        {
            ipnettophy[0].vl_ipNetToPhysicalType = IPNETTOPHYSICALTYPE_LOCAL;
            ipnettophy[0].vl_ipNetToPhysicalState = IPNETTOPHYSICALSTATE_UNKNOWN;
            ipnettophy[0]. vl_ipNetToPhysicalRowStatus = IPNETTOPHYSICALROWSTATUS_ACTIVE;
        }
        else
        {
            ipnettophy[0].vl_ipNetToPhysicalType = IPNETTOPHYSICALTYPE_INVALID;
            ipnettophy[0].vl_ipNetToPhysicalState = IPNETTOPHYSICALSTATE_UNKNOWN;
            ipnettophy[0]. vl_ipNetToPhysicalRowStatus = IPNETTOPHYSICALROWSTATUS_NOTREADY;
        }/*For presnet i am checking for up and down state later it has to support remaing enum conditions */

        ifdata[0].vl_ifOutQLen=0;
        ifdata[0].vl_ifSpecific_len = strlen("");
        vlMemCpy(ifdata[0].vl_ifSpecific, " " ,ifdata[0].vl_ifSpecific_len, API_CHRMAX);
    }

    /**--if&Iptable---CABLECARD ADDRESS INFO-----*/
    {
        /* iftable Indix*/
        ifdata[1].vl_ifIndex = 2;
        /*
         * Column values
        */
        ifdata[1].vl_ifindex = 2;//Index for IpNetToPhyscialTable
        ifdata[1].vl_ifDescr_len = strlen(ifTableDesc[1]);
        vlMemCpy(ifdata[1].vl_ifDescr,ifTableDesc[1] ,ifdata[1].vl_ifDescr_len, API_CHRMAX);
        ifdata[1].vl_ifType = IFTYPE_OTHER;
        ifdata[1].vl_ifMtu = 0; // Dec-02-2009: Changed to 0 as per CFR Table 14.3-2
        ifdata[1].vl_ifSpeed = 0;
        /*To get the cable CARD MAC addresss call mfr modules*/
        VL_HOST_IP_INFO pcardipinfo;
		IPC_CLIENT_vlXchanGetDsgCableCardIpInfo(&pcardipinfo);
        //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "Avinterface::vlGet_ifIpTableData: Got IP Address: '%s'\n", pcardipinfo.szIpAddress);
        ifdata[1].vl_ifInOctets         = pcardipinfo.stats.ifInOctets       ;
        ifdata[1].vl_ifInUcastPkts      = pcardipinfo.stats.ifInUcastPkts    ;
        ifdata[1].vl_ifInNUcastPkts     = pcardipinfo.stats.ifInNUcastPkts   ;
        ifdata[1].vl_ifInDiscards       = pcardipinfo.stats.ifInDiscards     ;
        ifdata[1].vl_ifInErrors         = pcardipinfo.stats.ifInErrors       ;
        ifdata[1].vl_ifInUnknownProtos  = pcardipinfo.stats.ifInUnknownProtos;
        ifdata[1].vl_ifOutOctets        = pcardipinfo.stats.ifOutOctets      ;
        ifdata[1].vl_ifOutUcastPkts     = pcardipinfo.stats.ifOutUcastPkts   ;
        ifdata[1].vl_ifOutNUcastPkts    = pcardipinfo.stats.ifOutNUcastPkts  ;
        ifdata[1].vl_ifOutDiscards      = pcardipinfo.stats.ifOutDiscards    ;
        ifdata[1].vl_ifOutErrors        = pcardipinfo.stats.ifOutErrors      ;

        ifdata[1].vl_ifPhysAddress_len = ((pcardipinfo.lFlags&IFF_UP)?VL_MAC_ADDR_SIZE:0);
        ifdata[1].vl_ifAdminStatus = ((pcardipinfo.lFlags&IFF_UP)?IFADMINSTATUS_UP:IFADMINSTATUS_DOWN);
        ifdata[1].vl_ifOperStatus = ((pcardipinfo.lFlags&IFF_RUNNING)?IFOPERSTATUS_UP:IFOPERSTATUS_DOWN);
        if(IFOPERSTATUS_DOWN == ifdata[1].vl_ifOperStatus)
        {
         	if(IPC_CLIENT_vlXchanGetSocketFlowCount() > 0)
            {
                ifdata[1].vl_ifOperStatus = IFOPERSTATUS_NOTPRESENT;
            }
        }

        ifdata[1].vl_old_ifAdminStatus = ifdata[1].vl_ifAdminStatus;
        ifdata[1].vl_ifLastChange = pcardipinfo.lLastChange;
        //if-Table Mac details
        vlMemCpy(ifdata[1].vl_ifPhysAddress,pcardipinfo.aBytMacAddress,VL_MAC_ADDR_SIZE, API_CHRMAX);
        getsystemUPtime(&cardipnetTimeStamp);//for vl_ipNetToPhysicalLastUpdated
        ipnettophy[1].vl_ipNetToPhysicalLastUpdated = cardipnetTimeStamp;

        //--IP-HOST info
        ipnettophy[1].vl_ipNetToPhysicalPhysAddress_len = MAC_BYTE_SIZE *sizeof(char);
        vlMemCpy(ipnettophy[1].vl_ipNetToPhysicalPhysAddress,pcardipinfo.aBytMacAddress,VL_MAC_ADDR_SIZE , API_CHRMAX );

        //vlMemSet(ipnettophy[1].vl_ipNetToPhysicalNetAddress, 0, sizeof(ipnettophy[1].vl_ipNetToPhysicalNetAddress), sizeof(ipnettophy[1].vl_ipNetToPhysicalNetAddress));
        memset(ipnettophy[1].vl_ipNetToPhysicalNetAddress, 0, sizeof(ipnettophy[1].vl_ipNetToPhysicalNetAddress));
        ipnettophy[1].vl_ipNetToPhysicalNetAddress_len = 0;

        ipnettophy[1].vl_ipNetToPhysicalNetAddressType =  IPNETTOPHYSICALNETADDRESSTYPE_IPV4;
        ipnettophy[1].vl_ipNetToPhysicalNetAddress_len = sizeof(pcardipinfo.aBytIpAddress);

        switch(pcardipinfo.ipAddressType){
            case VL_XCHAN_IP_ADDRESS_TYPE_NONE:
                ipnettophy[1].vl_ipNetToPhysicalNetAddressType =  IPNETTOPHYSICALNETADDRESSTYPE_IPV4;
                break;
            case VL_XCHAN_IP_ADDRESS_TYPE_IPV4:
                ipnettophy[1].vl_ipNetToPhysicalNetAddressType =  IPNETTOPHYSICALNETADDRESSTYPE_IPV4;
                vlMemCpy(ipnettophy[1].vl_ipNetToPhysicalNetAddress, pcardipinfo.aBytIpAddress, sizeof(pcardipinfo.aBytIpAddress), API_CHRMAX);
                ipnettophy[1].vl_ipNetToPhysicalNetAddress_len = sizeof(pcardipinfo.aBytIpAddress);
                break;
            case VL_XCHAN_IP_ADDRESS_TYPE_IPV6:
                ipnettophy[1].vl_ipNetToPhysicalNetAddressType =  IPNETTOPHYSICALNETADDRESSTYPE_IPV6;
                vlMemCpy(ipnettophy[1].vl_ipNetToPhysicalNetAddress, pcardipinfo.aBytIpV6GlobalAddress, sizeof(pcardipinfo.aBytIpV6GlobalAddress), API_CHRMAX);
                ipnettophy[1].vl_ipNetToPhysicalNetAddress_len = sizeof(pcardipinfo.aBytIpV6GlobalAddress);
                break;
            case VL_XCHAN_IP_ADDRESS_TYPE_IPV4Z:
                ipnettophy[1].vl_ipNetToPhysicalNetAddressType =  IPNETTOPHYSICALNETADDRESSTYPE_IPV4Z;
                vlMemCpy(ipnettophy[1].vl_ipNetToPhysicalNetAddress, pcardipinfo.aBytIpAddress, sizeof(pcardipinfo.aBytIpAddress), API_CHRMAX);
                ipnettophy[1].vl_ipNetToPhysicalNetAddress_len = sizeof(pcardipinfo.aBytIpAddress);
                break;
            case VL_XCHAN_IP_ADDRESS_TYPE_IPV6Z:
                ipnettophy[1].vl_ipNetToPhysicalNetAddressType =  IPNETTOPHYSICALNETADDRESSTYPE_IPV6Z;
                vlMemCpy(ipnettophy[1].vl_ipNetToPhysicalNetAddress, pcardipinfo.aBytIpV6GlobalAddress, sizeof(pcardipinfo.aBytIpV6GlobalAddress), API_CHRMAX);
                ipnettophy[1].vl_ipNetToPhysicalNetAddress_len = sizeof(pcardipinfo.aBytIpV6GlobalAddress);
                break;
            case VL_XCHAN_IP_ADDRESS_TYPE_DNS:
                ipnettophy[1].vl_ipNetToPhysicalNetAddressType =  IPNETTOPHYSICALNETADDRESSTYPE_DNS;
                break;

        }

        /*End To get the cable CARD MAC addresss call mfr modules*/
        /* IF the Card IP will be active when below if conditon is not equeal to 0.0.0.0 then the below of IP-table and iftable will befilled*/

        /*index of IP */

        ipnettophy[1].vl_ipNetToPhysicalIfIndex = ifdata[1].vl_ifindex;
        /*
        * Column values
        */
        if(IFADMINSTATUS_UP == ifdata[1].vl_ifAdminStatus)
        {
            ipnettophy[1].vl_ipNetToPhysicalType = IPNETTOPHYSICALTYPE_LOCAL;
            ipnettophy[1].vl_ipNetToPhysicalState = IPNETTOPHYSICALSTATE_UNKNOWN;
            ipnettophy[1]. vl_ipNetToPhysicalRowStatus = IPNETTOPHYSICALROWSTATUS_ACTIVE;
        }
        else
        {
            ipnettophy[1].vl_ipNetToPhysicalType = IPNETTOPHYSICALTYPE_INVALID;
            ipnettophy[1].vl_ipNetToPhysicalState = IPNETTOPHYSICALSTATE_UNKNOWN;
            ipnettophy[1]. vl_ipNetToPhysicalRowStatus = IPNETTOPHYSICALROWSTATUS_NOTREADY;
        }/*For presnet i am checking for up and down state later it has to support remaing enum conditions */

        ifdata[1].vl_ifOutQLen=0;
        ifdata[1].vl_ifSpecific_len = strlen("");
        vlMemCpy(ifdata[1].vl_ifSpecific, " " ,ifdata[1].vl_ifSpecific_len, API_CHRMAX);
    }

#if 1
    /**--if&Iptable---MoCA ADDRESS INFO-----*/
    {
        /* iftable Indix*/
        ifdata[2].vl_ifIndex = 3;
        /*
         * Column values
        */
        ifdata[2].vl_ifindex = 3;//Index for IpNetToPhyscialTable
        ifdata[2].vl_ifDescr_len = strlen(ifTableDesc[2]);
        vlMemCpy(ifdata[2].vl_ifDescr,ifTableDesc[2] ,ifdata[2].vl_ifDescr_len, API_CHRMAX);
        ifdata[2].vl_ifType = IFTYPE_MOCAVERSION1;
        ifdata[2].vl_ifMtu = 1500; // Dec-02-2009: Changed to 0 as per CFR Table 14.3-2
        ifdata[2].vl_ifSpeed = 0; // Speed for MoCA is unspecified.
        VL_HOST_IP_INFO pmocaipinfo;
        vlSnmpGetHostIntefaceInfo(getInterfaceNameFromConfig("MOCA_INTERFACE"), &pmocaipinfo);
        /*To get the cable CARD MAC addresss call mfr modules*/
        //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "Avinterface::vlGet_ifIpTableData: Got IP Address: '%s'\n", pmocaipinfo.szIpAddress);
        ifdata[2].vl_ifInOctets         = pmocaipinfo.stats.ifInOctets       ;
        ifdata[2].vl_ifInUcastPkts      = pmocaipinfo.stats.ifInUcastPkts    ;
        ifdata[2].vl_ifInNUcastPkts     = pmocaipinfo.stats.ifInNUcastPkts   ;
        ifdata[2].vl_ifInDiscards       = pmocaipinfo.stats.ifInDiscards     ;
        ifdata[2].vl_ifInErrors         = pmocaipinfo.stats.ifInErrors       ;
        ifdata[2].vl_ifInUnknownProtos  = pmocaipinfo.stats.ifInUnknownProtos;
        ifdata[2].vl_ifOutOctets        = pmocaipinfo.stats.ifOutOctets      ;
        ifdata[2].vl_ifOutUcastPkts     = pmocaipinfo.stats.ifOutUcastPkts   ;
        ifdata[2].vl_ifOutNUcastPkts    = pmocaipinfo.stats.ifOutNUcastPkts  ;
        ifdata[2].vl_ifOutDiscards      = pmocaipinfo.stats.ifOutDiscards    ;
        ifdata[2].vl_ifOutErrors        = pmocaipinfo.stats.ifOutErrors      ;

        ifdata[2].vl_ifPhysAddress_len = ((pmocaipinfo.lFlags&IFF_UP)?VL_MAC_ADDR_SIZE:0);
        ifdata[2].vl_ifAdminStatus = ((pmocaipinfo.lFlags&IFF_UP)?IFADMINSTATUS_UP:IFADMINSTATUS_DOWN);
        ifdata[2].vl_ifOperStatus = ((pmocaipinfo.lFlags&IFF_RUNNING)?IFOPERSTATUS_UP:IFOPERSTATUS_DOWN);
        if(IFOPERSTATUS_DOWN == ifdata[2].vl_ifOperStatus)
        {
            if(IPC_CLIENT_vlXchanGetSocketFlowCount() > 0)
            {
                ifdata[2].vl_ifOperStatus = IFOPERSTATUS_NOTPRESENT;
            }
        }

        ifdata[2].vl_old_ifAdminStatus = ifdata[2].vl_ifAdminStatus;
        ifdata[2].vl_ifLastChange = vlg_tcMocaIpLastChange;
        //if-Table Mac details
        vlMemCpy(ifdata[2].vl_ifPhysAddress,pmocaipinfo.aBytMacAddress,VL_MAC_ADDR_SIZE, API_CHRMAX);
        getsystemUPtime(&cardipnetTimeStamp);//for vl_ipNetToPhysicalLastUpdated
        ipnettophy[2].vl_ipNetToPhysicalLastUpdated = cardipnetTimeStamp;

        //--IP-HOST info
        ipnettophy[2].vl_ipNetToPhysicalPhysAddress_len = MAC_BYTE_SIZE *sizeof(char);
        vlMemCpy(ipnettophy[2].vl_ipNetToPhysicalPhysAddress,pmocaipinfo.aBytMacAddress,VL_MAC_ADDR_SIZE , API_CHRMAX );

        //vlMemSet(ipnettophy[2].vl_ipNetToPhysicalNetAddress, 0, sizeof(ipnettophy[2].vl_ipNetToPhysicalNetAddress), sizeof(ipnettophy[2].vl_ipNetToPhysicalNetAddress));
        memset(ipnettophy[2].vl_ipNetToPhysicalNetAddress, 0, sizeof(ipnettophy[2].vl_ipNetToPhysicalNetAddress));
        ipnettophy[2].vl_ipNetToPhysicalNetAddress_len = 0;

        ipnettophy[2].vl_ipNetToPhysicalNetAddressType =  IPNETTOPHYSICALNETADDRESSTYPE_IPV4;
        ipnettophy[2].vl_ipNetToPhysicalNetAddress_len = sizeof(pmocaipinfo.aBytIpAddress);

        switch(pmocaipinfo.ipAddressType){
            case VL_XCHAN_IP_ADDRESS_TYPE_NONE:
                ipnettophy[2].vl_ipNetToPhysicalNetAddressType =  IPNETTOPHYSICALNETADDRESSTYPE_IPV4;
                break;
            case VL_XCHAN_IP_ADDRESS_TYPE_IPV4:
                ipnettophy[2].vl_ipNetToPhysicalNetAddressType =  IPNETTOPHYSICALNETADDRESSTYPE_IPV4;
                vlMemCpy(ipnettophy[2].vl_ipNetToPhysicalNetAddress, pmocaipinfo.aBytIpAddress, sizeof(pmocaipinfo.aBytIpAddress), API_CHRMAX);
                ipnettophy[2].vl_ipNetToPhysicalNetAddress_len = sizeof(pmocaipinfo.aBytIpAddress);
                break;
            case VL_XCHAN_IP_ADDRESS_TYPE_IPV6:
                ipnettophy[2].vl_ipNetToPhysicalNetAddressType =  IPNETTOPHYSICALNETADDRESSTYPE_IPV6;
                vlMemCpy(ipnettophy[2].vl_ipNetToPhysicalNetAddress, pmocaipinfo.aBytIpV6GlobalAddress, sizeof(pmocaipinfo.aBytIpV6GlobalAddress), API_CHRMAX);
                ipnettophy[2].vl_ipNetToPhysicalNetAddress_len = sizeof(pmocaipinfo.aBytIpV6GlobalAddress);
                break;
            case VL_XCHAN_IP_ADDRESS_TYPE_IPV4Z:
                ipnettophy[2].vl_ipNetToPhysicalNetAddressType =  IPNETTOPHYSICALNETADDRESSTYPE_IPV4Z;
                vlMemCpy(ipnettophy[2].vl_ipNetToPhysicalNetAddress, pmocaipinfo.aBytIpAddress, sizeof(pmocaipinfo.aBytIpAddress), API_CHRMAX);
                ipnettophy[2].vl_ipNetToPhysicalNetAddress_len = sizeof(pmocaipinfo.aBytIpAddress);
                break;
            case VL_XCHAN_IP_ADDRESS_TYPE_IPV6Z:
                ipnettophy[2].vl_ipNetToPhysicalNetAddressType =  IPNETTOPHYSICALNETADDRESSTYPE_IPV6Z;
                vlMemCpy(ipnettophy[2].vl_ipNetToPhysicalNetAddress, pmocaipinfo.aBytIpV6GlobalAddress, sizeof(pmocaipinfo.aBytIpV6GlobalAddress), API_CHRMAX);
                ipnettophy[2].vl_ipNetToPhysicalNetAddress_len = sizeof(pmocaipinfo.aBytIpV6GlobalAddress);
                break;
            case VL_XCHAN_IP_ADDRESS_TYPE_DNS:
                ipnettophy[2].vl_ipNetToPhysicalNetAddressType =  IPNETTOPHYSICALNETADDRESSTYPE_DNS;
                break;

        }

        /*End To get the cable CARD MAC addresss call mfr modules*/
        /* IF the Card IP will be active when below if conditon is not equeal to 0.0.0.0 then the below of IP-table and iftable will befilled*/

        /*index of IP */

        ipnettophy[2].vl_ipNetToPhysicalIfIndex = ifdata[2].vl_ifindex;
        /*
        * Column values
        */
        if(IFADMINSTATUS_UP == ifdata[2].vl_ifAdminStatus)
        {
            ipnettophy[2].vl_ipNetToPhysicalType = IPNETTOPHYSICALTYPE_LOCAL;
            ipnettophy[2].vl_ipNetToPhysicalState = IPNETTOPHYSICALSTATE_UNKNOWN;
            ipnettophy[2]. vl_ipNetToPhysicalRowStatus = IPNETTOPHYSICALROWSTATUS_ACTIVE;
        }
        else
        {
            ipnettophy[2].vl_ipNetToPhysicalType = IPNETTOPHYSICALTYPE_INVALID;
            ipnettophy[2].vl_ipNetToPhysicalState = IPNETTOPHYSICALSTATE_UNKNOWN;
            ipnettophy[2]. vl_ipNetToPhysicalRowStatus = IPNETTOPHYSICALROWSTATUS_NOTREADY;
        }/*For presnet i am checking for up and down state later it has to support remaing enum conditions */

        ifdata[2].vl_ifOutQLen=0;
        ifdata[2].vl_ifSpecific_len = strlen("");
        vlMemCpy(ifdata[2].vl_ifSpecific, " " ,ifdata[2].vl_ifSpecific_len, API_CHRMAX);
    }
#endif
    //auto_unlock_ptr(m_lock);
    return 1;
}


/*END OF TABLE*/
void Avinterface::vlGet_Hostid(char* HostID)
{
        VL_AUTO_LOCK(m_lock);
		IPC_CLIENT_SNMPGet_ocStbHostHostID(HostID, 255);
    //auto_unlock_ptr(m_lock);
}


void Avinterface::vlGet_Hostcapabilite(int* hostcap)
{
          VL_AUTO_LOCK(m_lock);
       VL_SNMP_VAR_HOST_CAPABILITIES obj;

        vlSnmpGetHostInfo(VL_SNMP_VAR_TYPE_ocStbHostCapabilities, &obj);

        *hostcap = obj.HostCapabilities_val;
        //auto_unlock_ptr(m_lock);
}

void Avinterface::vlGet_ocStbHostAvcSupport(Status_t* avcSupport)
{
    if(NULL != avcSupport) *avcSupport = STATUS_TRUE;
}

void Avinterface::vlGet_ocStbEasMessageState(EasInfo_t* EasobectsInfo)
{
    VL_AUTO_LOCK(m_lock);
        VL_SNMP_VAR_EAS_STATE_CONFIG Easobj;
        vlSnmpGetHostInfo(VL_SNMP_VAR_TYPE_ocStbEasMessageStateInfo, &Easobj);

        EasobectsInfo->EMSCodel = Easobj.EMStateCodel;
      EasobectsInfo->EMCCode  = Easobj.EMCountyCode;
     EasobectsInfo->EMCSCodel  = Easobj.EMCSubdivisionCodel;

       // SNMP_DEBUGPRINT("%s %d Eascode=%d Emc-code= %d EMCSC = %d \n",__FUNCTION__,__LINE__, EasobectsInfo->EMSCodel,EasobectsInfo->EMCCode,EasobectsInfo->EMCSCodel );

       // SNMP_DEBUGPRINT("%s %d Easobj.EMStateCodel=%d Emc-code= %d EMCSC = %d \n",__FUNCTION__,__LINE__, Easobj.EMStateCodel, Easobj.EMCountyCode, Easobj.EMCSubdivisionCodel );

    //auto_unlock_ptr(m_lock);
}

void getFirmwareReleaseDate(VL_SNMP_VAR_DEVICE_SOFTWARE * Softwareobj, struct tm *tm_val)
{
     string line = "";
     string strImageName = "";
     string strToFind = "Generated on ";
     ifstream versionFile ("/version.txt");
     bool bReadFromVersionFile = false;

     /* 
      ** Read from /version.txt file the 'Generated on ' string and then extract 
      ** the date-time information.
      ** If /version.txt is not available, then use default STACK-VERSION.txt
      ** provided information from vlSnmpGetHostInfo API available in Softwareobj
      ** parameters.
      */
     if (versionFile.is_open())
     {
	 while ( getline (versionFile,line) )
	 {
	     /* Read version.txt to match the 'Generated on ' string */
	     std::size_t found = line.find(strToFind);
	     if (found != std::string::npos)
	     {
		 strImageName = line.substr(strToFind.length());

		 RDK_LOG(RDK_LOG_INFO,"LOG.RDK.SNMP",
				 "Firmware Generated Date = %s, len = %d \n", 
				 strImageName.c_str(), strImageName.length());
		 /* This below check is to catch any incorrect date string in /version.txt */
		 if (strImageName.length() > 24) 
		 {
		     bReadFromVersionFile = true;	
		     /* Break from while loop as we have the got the required information */
		     break;
		 }
	     }
	 }
	 versionFile.close();
     }
     else
     {
	 RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP","Unable to open version.txt file \n");
     }

     /* Populate the date-time structure 'struct tm' from available data in /version.txt or STACK-VERSION.txt */ 

     /* 
      ** tm_time->tm_year needs to have the year value since 1900 and tm_time->tm_mon 
      ** needs to have the month value from 0 to 11, so here the we subtact 1900 and 1 
      ** from the respective year and month calculated values from version.txt or STACK_VERSION.txt
      */
     if(bReadFromVersionFile == true)
     {
	 RDK_LOG(RDK_LOG_INFO,"LOG.RDK.SNMP","Extracting date-time from version.txt file \n");
         /* Calculate the month value */
	 if (!(strncmp((strImageName.c_str() + 4), "Jan", 3)))
	     tm_val->tm_mon = 0;
	 else if (!(strncmp((strImageName.c_str() + 4), "Feb", 3)))
	     tm_val->tm_mon = 1;
	 else if (!(strncmp((strImageName.c_str() + 4), "Mar", 3)))
	     tm_val->tm_mon = 2;
	 else if (!(strncmp((strImageName.c_str() + 4), "Apr", 3)))
	     tm_val->tm_mon = 3;
	 else if (!(strncmp((strImageName.c_str() + 4), "May", 3)))
	     tm_val->tm_mon = 4;
	 else if (!(strncmp((strImageName.c_str() + 4), "Jun", 3)))
	     tm_val->tm_mon = 5;
	 else if (!(strncmp((strImageName.c_str() + 4), "Jul", 3)))
	     tm_val->tm_mon = 6;
	 else if (!(strncmp((strImageName.c_str() + 4), "Aug", 3)))
	     tm_val->tm_mon = 7;
	 else if (!(strncmp((strImageName.c_str() + 4), "Sep", 3)))
	     tm_val->tm_mon = 8;
	 else if (!(strncmp((strImageName.c_str() + 4), "Oct", 3)))
	     tm_val->tm_mon = 9;
	 else if (!(strncmp((strImageName.c_str() + 4), "Nov", 3)))
	     tm_val->tm_mon = 10;
	 else if (!(strncmp((strImageName.c_str() + 4), "Dec", 3)))
	     tm_val->tm_mon = 11;
	 else
	     tm_val->tm_mon = 0;

         /* Calculate the month value */
	 tm_val->tm_mday = atoi((strImageName.c_str() + 8));

         /* Calculate the hour value */
	 tm_val->tm_hour = atoi((strImageName.c_str() + 12));

         /* Calculate the minute value */
	 tm_val->tm_min = atoi((strImageName.c_str() + 15));

         /* Calculate the second value */
	 tm_val->tm_sec = atoi((strImageName.c_str() + 18));

         /* Calculate the year value */
	 tm_val->tm_year = vlAbs(atoi((strImageName.c_str() + 25)) - 1900);
     }
     else 
     {
	 RDK_LOG(RDK_LOG_INFO,"LOG.RDK.SNMP","Calculating date from STACK-VERSION.txt file \n");
	 tm_val->tm_mday = Softwareobj->nSoftwareFirmwareDay;
	 tm_val->tm_mon  = vlAbs(Softwareobj->nSoftwareFirmwareMonth-1);
	 tm_val->tm_year = vlAbs(Softwareobj->nSoftwareFirmwareYear-1900);
     }	

     /* 
      ** tm_val->>tm_year gives the year value since 1900 and tm_val->>tm_mon 
      ** gives the month value from 0 to 11, so for this debug print addition 
      ** of 1900 to year and 1 to month to print the correct value 
      */
     RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "Date: %d/%d/%d, Time: %d:%d \n",
		     tm_val->tm_year+1900, tm_val->tm_mon+1, tm_val->tm_mday,
		     tm_val->tm_hour, tm_val->tm_min);
}

void Avinterface::vlGet_ocStbHostDeviceSoftware(DeviceSInfo_t * DevSoftareInfo)
{    
     VL_AUTO_LOCK(m_lock);
     VL_SNMP_VAR_DEVICE_SOFTWARE Softwareobj;
     vlSnmpGetHostInfo(VL_SNMP_VAR_TYPE_ocStbHostDeviceSoftware, &Softwareobj);

     vlStrCpy(DevSoftareInfo->SFirmwareVersion,Softwareobj.SoftwareFirmwareVersion, sizeof(DevSoftareInfo->SFirmwareVersion));
     vlStrCpy(DevSoftareInfo->SFirmwareOCAPVersionl,Softwareobj.SoftwareFirmwareOCAPVersionl, sizeof(DevSoftareInfo->SFirmwareOCAPVersionl));
     /* 
      ** The data returned from vlSnmpGetHostInfo API for the time related 
      ** information under Softwareobj nSoftwareFirmwareDay, nSoftwareFirmwareMonth, 
      ** nSoftwareFirmwareYear is based on STACK-VERSION.txt. In RDK2.0, we do not
      ** STACK-VERSION.txt, so here we have the logic to read the date-time information
      ** from version.txt on the STB.
     */


     struct tm tm_time;
     memset(&tm_time, 0, sizeof(tm_time));

     getFirmwareReleaseDate(&Softwareobj, &tm_time);
      
     time_t installTime = 0;
     /* Convert the time from tm time-date structure to calender equivalent format */ 
     installTime = mktime(&tm_time);

     RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "asctime = %s \n, ctime = %s\n",
            asctime(&tm_time), ctime(&installTime));

     vlMemCpy(DevSoftareInfo->ocStbHostSoftwareFirmwareReleaseDate,
              date_n_time(&installTime, (unsigned int*)&DevSoftareInfo->ocStbHostSoftwareFirmwareReleaseDate_len),
                           API_CHRMAX, sizeof(DevSoftareInfo->ocStbHostSoftwareFirmwareReleaseDate));
	DevSoftareInfo->ocStbHostSoftwareFirmwareReleaseDate_len = 8;
     //auto_unlock_ptr(m_lock);
}

void Avinterface::vlGet_ocStbHostPower(HostPowerInfo_t* Hostpowerstatus)
{
    VL_AUTO_LOCK(m_lock);
    VL_SNMP_VAR_HOST_POWER_STATUS Hpowerobj;
    vlSnmpGetHostInfo(VL_SNMP_VAR_TYPE_ocStbHostPowerInfo, &Hpowerobj);

    switch(Hpowerobj.ocStbHostPowerStatus){
          case 1:
                  Hostpowerstatus->ocStbHostPowerStatus = S_POWERON;
                  break;
       case 2:
                   Hostpowerstatus->ocStbHostPowerStatus = S_STANDBY;
                  break;
          }

    switch(Hpowerobj.ocStbHostAcOutletStatus){
          case 1:
                  Hostpowerstatus->ocStbHostAcOutletStatus = S_UNSWITCHED;
                   break;
       case 2:
                   Hostpowerstatus->ocStbHostAcOutletStatus= S_SWITCHEDON;
                   break;
           case 3:
                   Hostpowerstatus->ocStbHostAcOutletStatus= S_SWITCHEDOFF;
                   break;
           case 4:
                   Hostpowerstatus->ocStbHostAcOutletStatus= S_NOTINSTALLED;
                   break;
          }
        //auto_unlock_ptr(m_lock);
}


void  Avinterface::vlGet_UserSettingsPreferedLanguage(char* UserSettingsPreferedLanguage)
{
          VL_AUTO_LOCK(m_lock);
        VL_SNMP_VAR_USER_PREFERRED_LANGUAGE usettingobj;
        vlSnmpGetHostInfo(VL_SNMP_VAR_TYPE_ocStbHostUserSettingsPreferedLanguage, &usettingobj);
        vlStrCpy(UserSettingsPreferedLanguage,usettingobj.UserSettingsPreferedLanguage, sizeof(UserSettingsPreferedLanguage));
        //auto_unlock_ptr(m_lock);
}


void Avinterface::vlGet_ocStbHostCardInfo(CardInfo_t* CardIpInfo)
{
     //char buffIP[255];
     //char buffMAC[255]
        VL_AUTO_LOCK(m_lock);
        
    if (! IsUpdateReqd_CheckStatus(vlg_timeCableCardInfoLastUpdated, TIME_GAP_FOR_CARD_UPDATE_IN_SECONDS))
    {
        *CardIpInfo = vlg_cableCardInfo;
        //auto_unlock_ptr(m_lock);
        return;
    }
    
    VL_HOST_CARD_IP_INFO Pcardinfo;
    vlCableCardAppInfo_t *appInfo;
    //appInfo =  (vlCableCardAppInfo_t *)malloc(sizeof(vlCableCardAppInfo_t));
    rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(vlCableCardAppInfo_t), (void**)&appInfo);
    cCardManagerIF *pCM = NULL;

    //vlMemSet(CardIpInfo->ocStbHostCardMACAddress, 0, VL_MAC_ADDR_SIZE, API_CHRMAX);
    memset(CardIpInfo->ocStbHostCardMACAddress, 0, VL_MAC_ADDR_SIZE);
    //VL_ZERO_STRUCT(appInfo);
    IPC_CLIENT_vlXchanGetHostCardIpInfo(&Pcardinfo);

//    if(pfc_kernel_global->get_resource("CardManager"))
        //vlMemSet(appInfo, 0, sizeof(vlCableCardAppInfo_t), sizeof(vlCableCardAppInfo_t));
        memset(appInfo, 0, sizeof(vlCableCardAppInfo_t));
        if (IPC_CLIENT_SNMPGetApplicationInfo(appInfo) != 0)
        {
            SNMP_DEBUGPRINT("SNMPGetApplicationInfo failed. Defaulting to legacy IPU flow.\n");
            // data type MacAddress is encoded as 6 byte octet string
            vlMemCpy(CardIpInfo->ocStbHostCardMACAddress, Pcardinfo.macAddress, VL_MAC_ADDR_SIZE, API_CHRMAX);
        }
        else
        {
            vlMemCpy(CardIpInfo->ocStbHostCardMACAddress, appInfo->CardMAC, VL_MAC_ADDR_SIZE, API_CHRMAX);
            if(0 == VL_VALUE_6_FROM_ARRAY(CardIpInfo->ocStbHostCardMACAddress))
            {
                SNMP_DEBUGPRINT("appInfo->CardMAC is zeros. Defaulting to legacy IPU flow.\n");
                // data type MacAddress is encoded as 6 byte octet string
                vlMemCpy(CardIpInfo->ocStbHostCardMACAddress, Pcardinfo.macAddress, VL_MAC_ADDR_SIZE, API_CHRMAX);
        }
    }

    vlMemCpy(CardIpInfo->ocStbHostCardIpAddress, Pcardinfo.ipAddress, sizeof(Pcardinfo.ipAddress), sizeof(CardIpInfo->ocStbHostCardIpAddress));
    // feb-08-2010: report zeros for card ip info if in legacy qpsk mode
    if(!IPC_CLIENT_vlIsDsgMode())
    {
        vlStrCpy(CardIpInfo->ocStbHostCardIpAddress, "0.0.0.0", sizeof(CardIpInfo->ocStbHostCardIpAddress));
    }
    SNMP_DEBUGPRINT(":::::::::::::::::CardMacAddInfo :: %s\n",CardIpInfo->ocStbHostCardMACAddress);
    SNMP_DEBUGPRINT(":::::::::::::::::CardIPinfo :: %s\n",CardIpInfo->ocStbHostCardIpAddress);
    switch(Pcardinfo.ipAddressType)
    {
        case VL_XCHAN_IP_ADDRESS_TYPE_NONE:
            CardIpInfo->ocStbHostCardIpAddressType =  S_CARDIPADDRESSTYPE_UNKNOWN;
            break;
        case VL_XCHAN_IP_ADDRESS_TYPE_IPV4:
            CardIpInfo->ocStbHostCardIpAddressType =  S_CARDIPADDRESSTYPE_IPV4;
            break;
        case VL_XCHAN_IP_ADDRESS_TYPE_IPV6:
            CardIpInfo->ocStbHostCardIpAddressType =  S_CARDIPADDRESSTYPE_IPV6;
            break;
        case VL_XCHAN_IP_ADDRESS_TYPE_IPV4Z:
            CardIpInfo->ocStbHostCardIpAddressType =  S_CARDIPADDRESSTYPE_IPV4Z;
            break;
        case VL_XCHAN_IP_ADDRESS_TYPE_IPV6Z:
            CardIpInfo->ocStbHostCardIpAddressType =  S_CARDIPADDRESSTYPE_IPV6Z;
            break;
        case VL_XCHAN_IP_ADDRESS_TYPE_DNS:
            CardIpInfo->ocStbHostCardIpAddressType =  S_CARDIPADDRESSTYPE_DNS;
            break;

    }
    //free(appInfo);
    rmf_osal_memFreeP(RMF_OSAL_MEM_POD, appInfo);
    
    vlg_cableCardInfo = *CardIpInfo;
    getsystemUPtime(&vlg_timeCableCardInfoLastUpdated);
}


void Avinterface::vlGet_ocstbHostInfo(SOcstbHOSTInfo_t* HostIpinfo)
{
    VL_AUTO_LOCK(m_lock);
    VL_HOST_IP_INFO phostipinfo;
    IPC_CLIENT_vlXchanGetDsgIpInfo(&phostipinfo);

    vlMemCpy(HostIpinfo->IPSubNetMask, phostipinfo.aBytSubnetMask, sizeof(phostipinfo.aBytSubnetMask), sizeof(HostIpinfo->IPSubNetMask));

    switch(phostipinfo.ipAddressType){
        case VL_XCHAN_IP_ADDRESS_TYPE_NONE:
            HostIpinfo->IPAddressType =  S_CARDIPADDRESSTYPE_UNKNOWN;
            break;
        case VL_XCHAN_IP_ADDRESS_TYPE_IPV4:
            HostIpinfo->IPAddressType =  S_CARDIPADDRESSTYPE_IPV4;
            break;
        case VL_XCHAN_IP_ADDRESS_TYPE_IPV6:
        {
            unsigned char v6mask[16];
            HostIpinfo->IPAddressType =  S_CARDIPADDRESSTYPE_IPV6;
            IPC_CLIENT_vlXchanGetV6SubnetMaskFromPlen(phostipinfo.nIpV6GlobalPlen, v6mask, sizeof(v6mask));
            vlMemCpy(HostIpinfo->IPSubNetMask, v6mask, sizeof(v6mask), sizeof(HostIpinfo->IPSubNetMask));
        }
        break;
        case VL_XCHAN_IP_ADDRESS_TYPE_IPV4Z:
            HostIpinfo->IPAddressType =  S_CARDIPADDRESSTYPE_IPV4Z;
            break;
        case VL_XCHAN_IP_ADDRESS_TYPE_IPV6Z:
        {
            unsigned char v6mask[16];
            HostIpinfo->IPAddressType =  S_CARDIPADDRESSTYPE_IPV6Z;
            IPC_CLIENT_vlXchanGetV6SubnetMaskFromPlen(phostipinfo.nIpV6GlobalPlen, v6mask, sizeof(v6mask));
            inet_ntop(AF_INET6,v6mask, HostIpinfo->IPSubNetMask,sizeof(HostIpinfo->IPSubNetMask));
        }
        break;
        case VL_XCHAN_IP_ADDRESS_TYPE_DNS:
            HostIpinfo->IPAddressType =  S_CARDIPADDRESSTYPE_DNS;
            break;

    }
    //auto_unlock_ptr(m_lock);
    
}
void Avinterface::vlGet_OcstbHostReboot(SOcstbHostRebootInfo_t* HostRebootInfo)
{
    VL_AUTO_LOCK(m_lock);
   // HostRebootInfo->rebootinfo  = S_powerup;
   // HostRebootInfo->rebootResetinfo = 0;
#if 1
        VL_SNMP_VAR_REBOOT_INFO rebootinfo;
        vlSnmpGetHostInfo(VL_SNMP_VAR_TYPE_OcstbHostRebootInfo, &rebootinfo);

      // SNMP_DEBUGPRINT("%s %d rebootinfo.HostRebootInfo = %d\n",__FUNCTION__,__LINE__, rebootinfo.HostRebootInfo);
       switch(rebootinfo.HostRebootInfo){

           case VL_SNMP_HOST_REBOOT_INFO_unknown:
                  HostRebootInfo->rebootinfo = S_unknown;
                   break;
           case VL_SNMP_HOST_REBOOT_INFO_davicDocsis:
                    HostRebootInfo->rebootinfo = S_davicDocsis;
                   break;
           case VL_SNMP_HOST_REBOOT_INFO_user:
                   HostRebootInfo->rebootinfo = S_user;
                   break;
           case VL_SNMP_HOST_REBOOT_INFO_system:
                    HostRebootInfo->rebootinfo = S_system;
                   break;
           case VL_SNMP_HOST_REBOOT_INFO_trap:
                    HostRebootInfo->rebootinfo = S_trap;
                   break;
           case VL_SNMP_HOST_REBOOT_INFO_silentWatchdog:
                    HostRebootInfo->rebootinfo = S_silentWatchdog;
                   break;
           case VL_SNMP_HOST_REBOOT_INFO_bootloader:
                    HostRebootInfo->rebootinfo = S_bootloader;
                   break;
           case VL_SNMP_HOST_REBOOT_INFO_powerup:
                    HostRebootInfo->rebootinfo = S_powerup;
                   break;
           case VL_SNMP_HOST_REBOOT_INFO_hostUpgrade:
                    HostRebootInfo->rebootinfo = S_hostUpgrade;
                   break;
           case VL_SNMP_HOST_REBOOT_INFO_hardware:
                    HostRebootInfo->rebootinfo = S_hardware;
                   break;
           case VL_SNMP_HOST_REBOOT_INFO_cablecardError:
                    HostRebootInfo->rebootinfo = S_cablecardError;
                   break;

          }

          if(rebootinfo.rebootResetinfo){

              HostRebootInfo->rebootResetinfo = STATUS_TRUE;
              /*SNMP_DEBUGPRINT*/RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "%s %d  STATUS_TRUE rebootinfo.rebootResetinfo = %d\n",__FUNCTION__,__LINE__, rebootinfo.rebootResetinfo);
          }
          else
          {
              HostRebootInfo->rebootResetinfo = STATUS_FALSE;
              /*SNMP_DEBUGPRINT*/RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "%s %d STATUS_FALSE rebootinfo.rebootResetinfo = %d\n",__FUNCTION__,__LINE__, rebootinfo.rebootResetinfo);

          }
        if(VL_SNMP_HOST_REBOOT_INFO_powerup == HostRebootInfo->rebootinfo || 
           VL_SNMP_HOST_REBOOT_INFO_hostUpgrade == HostRebootInfo->rebootinfo) 
          HostRebootInfo->BootStatus = completedSuccessfully;

#endif //if 0
    //auto_unlock_ptr(m_lock);

}
void Avinterface::vlSet_OcstbHostReboot()
{
    VL_AUTO_LOCK(m_lock);
    VL_SNMP_VAR_REBOOT_INFO rebootinfo;
    rebootinfo.rebootResetinfo = VL_SNMP_HOST_REBOOT_REQUEST_reboot; //rebootset;

    SNMP_DEBUGPRINT("%s %d vlSet_OcstbHostReboot = %d\n",__FUNCTION__,__LINE__, rebootinfo.rebootResetinfo);
        vlSnmpSetHostInfo(VL_SNMP_VAR_TYPE_OcstbHostRebootInfo_rebootResetinfo, &rebootinfo);
    //auto_unlock_ptr(m_lock);        
}


#define HOST_BYTES 5
void Avinterface::vlGet_HostCaTypeInfo(HostCATInfo_t* CAtypeinfo)
{
    VL_AUTO_LOCK(m_lock);

       unsigned int result = 0xff;

        HostCATypeInfo_t HostCaTypeInfo;
        //vlMemSet(&HostCaTypeInfo, 0, sizeof(HostCaTypeInfo), sizeof(HostCaTypeInfo));
        memset(&HostCaTypeInfo, 0, sizeof(HostCaTypeInfo));
        result = IPC_CLIENT_SNMPget_ocStbHostSecurityIdentifier(&HostCaTypeInfo);
        IPC_CLIENT_GetHostIdLuhn((unsigned char*)(HostCaTypeInfo.SecurityID), (char*)(CAtypeinfo->S_SecurityID));

       // snprintf(CAtypeinfo->S_CASystemID,
       //          sizeof(CAtypeinfo->S_CASystemID),
       //                 "%d",HostCaTypeInfo.CASystemID);
//       MPE_DIAG_CA_SYSTEM_ID mpeCaSystemId;
       unsigned short m_caSystemId = 0;	   
       //vlMemSet(&mpeCaSystemId, 0, sizeof(mpeCaSystemId), sizeof(mpeCaSystemId));
//       memset(&mpeCaSystemId, 0, sizeof(mpeCaSystemId));	   
       rmf_Error eRet = IPC_CLIENT_podImplGetCaSystemId(&(m_caSystemId));
       
       if((RMF_SUCCESS == eRet) && (0 != m_caSystemId))
       {
           HostCaTypeInfo.CASystemID = m_caSystemId;
       }
       
        snprintf(CAtypeinfo->S_CASystemID,
                 sizeof(CAtypeinfo->S_CASystemID),
                        "%02X%02X", ((HostCaTypeInfo.CASystemID >>8) & 0xFF) ,((HostCaTypeInfo.CASystemID >> 0) & 0xFF));

        switch(HostCaTypeInfo.HostCAtype){

           case 1:
                  CAtypeinfo->S_HostCAtype = S_CATYPE_OTHER;
                  break;
       case 2:
                   CAtypeinfo->S_HostCAtype = S_CATYPE_EMBEDDED;
                  break;
           case 3:
                    CAtypeinfo->S_HostCAtype = S_CATYPE_CABLECARD;
                  break;
           case 4:
                   CAtypeinfo->S_HostCAtype = S_CATYPE_DCAS;
                  break;
           default:
                CAtypeinfo->S_HostCAtype = S_CATYPE_OTHER;
                  break;
         }


        //SNMP_DEBUGPRINT("SampleXletViewer::decodePIDs:SNMPget_ocStbHostSecurityIdentifier returned result=%d\n",result);
    //auto_unlock_ptr(m_lock);
}


void Avinterface::vlGet_HostFirmwareDownloadStatus(FDownloadStatus_t* FirmwareStatus)
{
    VL_AUTO_LOCK(m_lock);
       unsigned long  dwfailed_c= 0;
       CDLMgrSnmpHostFirmwareStatus_t objfirmwarestatus;
       memset(&objfirmwarestatus,0,sizeof(CDLMgrSnmpHostFirmwareStatus_t));
//#ifdef USE_CDL
       IPC_CLIENT_CommonDownloadMgrSnmpHostFirmwareStatus(&objfirmwarestatus);


       FirmwareStatus->vl_FirmwareImageStatus = objfirmwarestatus.HostFirmwareImageStatus;//details the image status(CDLFirmwareImageStatus) recently downloaded
       FirmwareStatus->vl_FirmwareCodeDownloadStatus = objfirmwarestatus.HostFirmwareCodeDownloadStatus;//details the download status(CDLFirmwareCodeDLStatus_e) of the target image
       vlMemCpy(FirmwareStatus->vl_FirmwareCodeObjectName,objfirmwarestatus.HostFirmwareCodeObjectName, objfirmwarestatus.HostFirmwareCodeObjectNameSize, API_CHRMAX);

       //"The file name of the software image to be loaded into this device;
       FirmwareStatus->vl_FrimwareFailedStatus = objfirmwarestatus.HostFirmwareDownloadFailedStatus;//details failed
       //Download Fail count 
      // if(CLD_SNMP_SUCCESS == CommonDownloadMgrSnmpHostFirmwareDownloadFaileCount(&dwfailed_c))
       //{
        //   FirmwareStatus->vl_FirmwareDownloadFailedCount = dwfailed_c;
      // }
      // else
       {
           FirmwareStatus->vl_FirmwareDownloadFailedCount = NULL;
       }
//#endif // USE_CDL
    //auto_unlock_ptr(m_lock);

}


int Avinterface::vlGet_ocStbHostQpsk(ocStbHostQpsk_t* QpskObj)
{
    VL_AUTO_LOCK(m_lock);
	//  HARDCODED
	QpskObj->vl_ocStbHostQpskFDCFreq = 0;
	QpskObj->vl_ocStbHostQpskRDCFreq = 0;
	QpskObj->vl_ocStbHostQpskFDCBer = QPSKFDCBER_BERNOTAPPLICABLE; //QpskFDCBer_t
	QpskObj->vl_ocStbHostQpskFDCStatus = QPSKFDCSTATUS_NOTLOCKED; //QpskFDCStatus_t
	QpskObj->vl_ocStbHostQpskFDCBytesRead = 0;
	QpskObj->vl_ocStbHostQpskFDCPower = 0;
	QpskObj->vl_ocStbHostQpskFDCLockedTime = 0;
	QpskObj->vl_ocStbHostQpskFDCSNR = 0;
	QpskObj->vl_ocStbHostQpskAGC = 0;
	QpskObj->vl_ocStbHostQpskRDCPower = 0;
	QpskObj->vl_ocStbHostQpskRDCDataRate = QPSKRDCDATARATE_KBPS256; //QpskRDCDataRate_t

//     long csystime;
//     //Check if update is required by checking the time stamp.
//     if (! IsUpdateReqd_CheckStatus(mTimeStamp_QPSK))
//     {
//         //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "no need to update\n");
//         return 1;
//     }
//     pfcOOBTunerDriver    *oob_tuner_device;
//     memset(QpskObj, 0, sizeof(ocStbHostQpsk_t));
//     oob_tuner_device = pfc_kernel_global->get_oob_tuner_device();
//     //SNMP_DEBUGPRINT("ctuner_driver::ctuner_driver..%x\n",oob_tuner_device);
//     if(oob_tuner_device)
//      {
//     oob_tuner_device->Snmp_getQPSKTunerdetails(QpskObj);
//      }
//      else
//      {
//     SNMP_DEBUGPRINT("error getting oob_tuner_device\n");
//      }
//     getsystemUPtime(&csystime);
//     mTimeStamp_QPSK = csystime; //cGetSystemUpTime();
   return 1;
}
int Avinterface::vl_GetMCertificationInfo(vlMCardCertInfo_t
        *pgetMcardCertInfo, int *nitem)
{
        /** To Get M-CARD certification Info */

       // printf(" \n DEBUG  TEST %s %d \n ", __FUNCTION__ ,  __LINE__);        
        *nitem = 1; //Only one M-card details at present
         vlCableCardCertInfo_t  Mcardcertinfo;
         memset(pgetMcardCertInfo, 0, sizeof(vlCableCardCertInfo_t));
   
               if( IPC_CLIENT_SNMPGetCableCardCertInfo(&Mcardcertinfo) != 0)
               {
     
                      SNMP_DEBUGPRINT("\n SNMPGET Cable Card Cert Info Failed%s %d \n ", __FUNCTION__ ,  __LINE__);        
                 
               }
              else
              {
                 
                 vlMemCpy(pgetMcardCertInfo->vl_szDispString
                         ,Mcardcertinfo.szDispString,
                         strlen(Mcardcertinfo.szDispString),CARD_STR_SIZE); 
                 vlMemCpy(pgetMcardCertInfo->vl_szDeviceCertSubjectName,
                         Mcardcertinfo.szDeviceCertSubjectName,
                         strlen(Mcardcertinfo.szDeviceCertSubjectName), CARD_STR_SIZE);
                 
                 vlMemCpy(pgetMcardCertInfo->vl_szDeviceCertIssuerName
                         ,Mcardcertinfo.szDeviceCertIssuerName,
                         strlen(Mcardcertinfo.szDeviceCertIssuerName), CARD_STR_SIZE);
                 
                 vlMemCpy(pgetMcardCertInfo->vl_szManCertSubjectName
                         ,Mcardcertinfo.szManCertSubjectName,
                         strlen(Mcardcertinfo.szManCertSubjectName), CARD_STR_SIZE);
                 
                 vlMemCpy(pgetMcardCertInfo->vl_szManCertIssuerName,
                         Mcardcertinfo.szManCertIssuerName,
                         strlen(Mcardcertinfo.szManCertIssuerName), CARD_STR_SIZE);
                                 
               int ret =  snprintf(pgetMcardCertInfo->vl_acHostId,
                        CARD_STR_SIZE, "%02x:%02x:%02x:%02x:%02x",
                         Mcardcertinfo.acHostId[0], Mcardcertinfo.acHostId[1],
                         Mcardcertinfo.acHostId[2],Mcardcertinfo.acHostId[3],Mcardcertinfo.acHostId[4]); 
                  pgetMcardCertInfo->vl_acHostId[ret]=0; 
		  if(Mcardcertinfo.bCertAvailable)
		  {
			vlStrCpy(pgetMcardCertInfo->vl_bCertAvailable, "Yes", sizeof("Yes"));
		  }
		  else
		  {
			vlStrCpy(pgetMcardCertInfo->vl_bCertAvailable, "No", sizeof("No"));
		  }


		  if(Mcardcertinfo.bCertValid)
		  {
			vlStrCpy(pgetMcardCertInfo->vl_bCertValid, "Yes", sizeof("Yes"));
		  }
		  else
		  {
			vlStrCpy(pgetMcardCertInfo->vl_bCertValid, "No", sizeof("No"));
		  }

		  
		 if(Mcardcertinfo.bVerifiedWithChain)
		  {
			vlStrCpy(pgetMcardCertInfo->vl_bVerifiedWithChain, "Yes", sizeof("Yes"));
		  }
		  else
		  {
			vlStrCpy(pgetMcardCertInfo->vl_bVerifiedWithChain, "No", sizeof("No"));
		  }


		  if(Mcardcertinfo.bIsProduction)
		  {
			vlStrCpy(pgetMcardCertInfo->vl_bIsProduction, "Yes", sizeof("Yes"));
		  }
		  else
		  {
			vlStrCpy(pgetMcardCertInfo->vl_bIsProduction, "No", sizeof("No"));
		  }
	
                      
                     //SNMP_DEBUGPRINT("Mcardcertinfo.szDispString%s\n",Mcardcertinfo.szDispString);
                     //SNMP_DEBUGPRINT("Mcardcertinfo.szDeviceCertSubjectName%s\n",Mcardcertinfo.szDeviceCertSubjectName);
                     //SNMP_DEBUGPRINT("Mfr Name  =  %s\n",Mcardcertinfo.szDeviceCertIssuerName);
                     //SNMP_DEBUGPRINT("Model Name =  %s\n",Mcardcertinfo.szManCertSubjectName);
                     //SNMP_DEBUGPRINT("Model Name = %s\n", Mcardcertinfo.szManCertIssuerName);
                     printf("pgetMcardCertInfo->vl_acHostId = %s \n  Model Id = 0x%02X%02X%02X%02X\n", pgetMcardCertInfo->vl_acHostId, Mcardcertinfo.acHostId[0], Mcardcertinfo.acHostId[1],Mcardcertinfo.acHostId[2],  Mcardcertinfo.acHostId[3]);
                if(Mcardcertinfo.bCertAvailable)
                        SNMP_DEBUGPRINT("Certificate Available = YES\n");
                else
                       SNMP_DEBUGPRINT("Certificate Available = NO\n");
                if(Mcardcertinfo.bCertValid)
                     SNMP_DEBUGPRINT("Certificate Valid = YES\n");
                else
                    SNMP_DEBUGPRINT("Certificate Valid = NO\n");
     
           }
   return 1;
}
int Avinterface::vl_getCertificationInfo(Vividlogiccertification_t *Vl_CertificationInfo, int *nitem)
{
    //VL_AUTO_LOCK(m_lock);
    int countcert = 0;

    /** To Get Vividlogic certification Info */
//     VL_SNMP_CERTIFICATE_INFO ieee1394cert;
//     vlMemSet(&ieee1394cert, 0, sizeof(ieee1394cert), sizeof(ieee1394cert));
//     vlMemSet(&Vl_CertificationInfo[0].vl_VividlogicCertFullInfo, 0 , sizeof(Vl_CertificationInfo[0].vl_VividlogicCertFullInfo) , sizeof(Vl_CertificationInfo[0].vl_VividlogicCertFullInfo));
//     char temp1394[35]= "IEEE1394 CERTIFICATION INFO";
//     int ret1394 = ERROR_HAL_GENERAL;
// #ifdef USE_1394
//     ret1394 = vlSnmpGetVlCertificateInfo(&ieee1394cert);
// #endif // USE_1394
// 
//     if(HAL_SUCCESS == ret1394)
//     {
//         SNMP_DEBUGPRINT("ieee1394cert.szDispString :: %s\n",ieee1394cert.szDispString);
//         SNMP_DEBUGPRINT("ieee1394cert.szKeyInfoString ::%s\n",ieee1394cert.szKeyInfoString);
//         SNMP_DEBUGPRINT("Mfr Name = %s\n",ieee1394cert.szMfrName);
// 
//         SNMP_DEBUGPRINT("Model Name = %s\n",ieee1394cert.szModelName);
//         SNMP_DEBUGPRINT("Model Id = 0x%02X%02X%02X%02X\n", ieee1394cert.acModelId[0], ieee1394cert.acModelId[1], ieee1394cert.acModelId[2], ieee1394cert.acModelId[3]);
//         SNMP_DEBUGPRINT("Venodr Id = 0x%02X%02X%02X\n",ieee1394cert.acVendorId[0],ieee1394cert.acVendorId[1],ieee1394cert.acVendorId[2]);
//         SNMP_DEBUGPRINT("Device Id = 0x%02X%02X%02X%02X%02X\n",ieee1394cert.acDeviceId[0],ieee1394cert.acDeviceId[1],ieee1394cert.acDeviceId[2],ieee1394cert.acDeviceId[3],ieee1394cert.acDeviceId[4]);
//         SNMP_DEBUGPRINT("Guid = %02X%02X%02X%02X%02X%02X%02X%02X\n", ieee1394cert.acGUIDorMACADDR[0],ieee1394cert.acGUIDorMACADDR[1],ieee1394cert.acGUIDorMACADDR[2],ieee1394cert.acGUIDorMACADDR[3],ieee1394cert.acGUIDorMACADDR[4],ieee1394cert.acGUIDorMACADDR[5],ieee1394cert.acGUIDorMACADDR[6],ieee1394cert.acGUIDorMACADDR[7]);
// 
//         if(ieee1394cert.bCertAvailable)
//             SNMP_DEBUGPRINT("Certificate Available = YES\n");
//         else
//             SNMP_DEBUGPRINT("Certificate Available = NO\n");
// 
//         if(ieee1394cert.bCertValid)
//             SNMP_DEBUGPRINT("Certificate Valid = YES\n");
//         else
//             SNMP_DEBUGPRINT("Certificate Valid = NO\n");
// 
//         snprintf(Vl_CertificationInfo[0].vl_VividlogicCertFullInfo,
//                  sizeof(Vl_CertificationInfo[0].vl_VividlogicCertFullInfo),
//                         "KEY INFO: %s \nMFR NAME: %s \nMODEL NAME: %s \nMODEL ID: %02x:%02x:%02x:%02x \nVENDOR ID: %02x:%02x:%02x \nDevice ID: %02x:%02x:%02x:%02x:%02x \nGUID-ADDR: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x \nCERTIFICATION AVAILABLE(TRUE:1 FALSE:0): %d \nCERTIFICATION VALID(TRUE:1 FALSE:0): %d \n", ieee1394cert.szKeyInfoString, ieee1394cert.szMfrName, ieee1394cert.szModelName ,ieee1394cert.acModelId[0], ieee1394cert.acModelId[1], ieee1394cert.acModelId[2], ieee1394cert.acModelId[3] , ieee1394cert.acVendorId[0], ieee1394cert.acVendorId[1], ieee1394cert.acVendorId[2], ieee1394cert.acDeviceId[0],ieee1394cert.acDeviceId[1], ieee1394cert.acDeviceId[2], ieee1394cert.acDeviceId[3], ieee1394cert.acDeviceId[4], ieee1394cert.acGUIDorMACADDR[0],ieee1394cert.acGUIDorMACADDR[1], ieee1394cert.acGUIDorMACADDR[2], ieee1394cert.acGUIDorMACADDR[3], ieee1394cert.acGUIDorMACADDR[4], ieee1394cert.acGUIDorMACADDR[5], ieee1394cert.acGUIDorMACADDR[6], ieee1394cert.acGUIDorMACADDR[7], ieee1394cert.bCertAvailable, ieee1394cert.bCertValid);
// 
//         Vl_CertificationInfo[0].vl_VividlogicCertFullInfo_len = strlen(Vl_CertificationInfo[0].vl_VividlogicCertFullInfo);
//         }
//         else
//         {
//         char temp1394[35]= "IEEE1394 CERTIFICATION INFO";
//         char Nocertemp[35]="Certification Not Available";
//         snprintf(Vl_CertificationInfo[0].vl_VividlogicCertFullInfo,
//                  sizeof(Vl_CertificationInfo[0].vl_VividlogicCertFullInfo),
//                         "%s", Nocertemp);
//         Vl_CertificationInfo[0].vl_VividlogicCertFullInfo_len = strlen(Vl_CertificationInfo[0].vl_VividlogicCertFullInfo);
//         }
// 
//     /** To Get IEEE1394 DTCP certification Info */
// 
//     VL_SNMP_CERTIFICATE_INFO ieee1394DTCPcert;
//     vlMemSet(&ieee1394DTCPcert, 0, sizeof(ieee1394DTCPcert), sizeof(ieee1394DTCPcert));
//     vlMemSet(&Vl_CertificationInfo[5].vl_VividlogicCertFullInfo, 0 , sizeof(Vl_CertificationInfo[5].vl_VividlogicCertFullInfo) , sizeof(Vl_CertificationInfo[5].vl_VividlogicCertFullInfo));
//     char temp1394DTCP[35]= "IEEE1394 DTCP CERTIFICATION INFO";
// 
//     ret1394 = ERROR_HAL_GENERAL;
// 
// #ifdef USE_1394
//     ret1394 = vlSnmpGetDtcpCertificateInfo(&ieee1394DTCPcert);
// #endif // USE_1394
//     if(HAL_SUCCESS == ret1394)
//     {
//         SNMP_DEBUGPRINT("ieee1394cert.szDispString :: %s\n",ieee1394DTCPcert.szDispString);
//         SNMP_DEBUGPRINT("ieee1394cert.szKeyInfoString ::%s\n",ieee1394DTCPcert.szKeyInfoString);
//         if(ieee1394DTCPcert.bCertAvailable)
//             SNMP_DEBUGPRINT("Certificate Available = YES\n");
//         else
//             SNMP_DEBUGPRINT("Certificate Available = NO\n");
// 
//         if(ieee1394DTCPcert.bCertValid)
//             SNMP_DEBUGPRINT("Certificate Valid = YES\n");
//         else
//             SNMP_DEBUGPRINT("Certificate Valid = NO\n");
// 
//         if(ieee1394DTCPcert.bIsProduction)
//             SNMP_DEBUGPRINT("Production Certificate = YES\n");
//         else
//             SNMP_DEBUGPRINT("Production Certificate = NO\n");
// 
//               snprintf(Vl_CertificationInfo[5].vl_VividlogicCertFullInfo,
//                        sizeof(Vl_CertificationInfo[5].vl_VividlogicCertFullInfo),
//                               "KEY INFO: %s \nCERTIFICATION AVAILABLE(TRUE:1 FALSE:0): %d \nCERTIFICATION VALID(TRUE:1 FALSE:0): %d \nbVERIFIED WITH CHAIN(TRUE:1 FALSE:0): %d \nProduction Key(TRUE:1 FALSE:0): %d", ieee1394DTCPcert.szKeyInfoString, ieee1394DTCPcert.bCertAvailable, ieee1394DTCPcert.bCertValid, ieee1394DTCPcert.bVerifiedWithChain, ieee1394DTCPcert.bIsProduction);
// 
//               Vl_CertificationInfo[5].vl_VividlogicCertFullInfo_len = strlen(Vl_CertificationInfo[5].vl_VividlogicCertFullInfo);
//         }
//         else
//         {
//         char temp1394DTCP[35]= "IEEE1394 DTCP CERTIFICATION INFO";
//         char Nocertemp[35]="Certification Not Available";
//         snprintf(Vl_CertificationInfo[5].vl_VividlogicCertFullInfo,
//                  sizeof(Vl_CertificationInfo[5].vl_VividlogicCertFullInfo),
//                         "%s", Nocertemp);
//         Vl_CertificationInfo[5].vl_VividlogicCertFullInfo_len = strlen(Vl_CertificationInfo[5].vl_VividlogicCertFullInfo);
//         }
// 
// 
// //#if 0
//           /** To Get M-CARD certification Info */
// //                char temp1[35]= "M-CARD CERTIFICATION INFO";
// //              //vlStrCpy(temp1,"M-CARD CERTIFICATION INFO", strlen("M-CARD CERTIFICATION INFO"));
// //             char temp2[35]="Certificaiton Not Avilable";
// //
// //             snprintf(Vl_CertificationInfo[1].vl_VividlogicCertFullInfo, sizeof(Vl_CertificationInfo[1].vl_VividlogicCertFullInfo), " SzDispString: %s Details: %s", temp1,temp2);
// //             Vl_CertificationInfo[1].vl_VividlogicCertFullInfo_len = strlen(Vl_CertificationInfo[1].vl_VividlogicCertFullInfo);
// 
//     vlCableCardCertInfo_t  Mcardcertinfo;
//     /*cCardManagerIF *pCM = NULL;
//     if(pfc_kernel_global->get_resource("CardManager"))
//         pCM = cCardManagerIF::getInstance(pfc_kernel_global);
// 
//     vlMemSet(&Vl_CertificationInfo[1].vl_VividlogicCertFullInfo, 0 , sizeof(Vl_CertificationInfo[1].vl_VividlogicCertFullInfo) , sizeof(Vl_CertificationInfo[1].vl_VividlogicCertFullInfo));
// 
//      if(pCM)
//      {
//        if( pCM->SNMPGetCableCardCertInfo(&Mcardcertinfo) != 0)
//           {
// 
//             char temp1[35]= "M-CARD CERTIFICATION INFO";
//              //vlStrCpy(temp1,"M-CARD CERTIFICATION INFO", strlen("M-CARD CERTIFICATION INFO"), sizeof(temp1));
//             char temp2[35]="Certification Not Available";
// 
//             snprintf(Vl_CertificationInfo[1].vl_VividlogicCertFullInfo,
//                      sizeof(Vl_CertificationInfo[1].vl_VividlogicCertFullInfo),
//                             "Host Id: %02x:%02x:%02x:%02x:%02x \nCertificate Available(TRUE:1 FALSE:0):%d  \nCertificate Valid(TRUE:1 FALSE:0):%d   \nVerifiedWithChain (TRUE:1 FALSE:0) : %d  \nProduction Keys(TRUE:1 FALSE:0): %d \nDeviceCertSubjectName: %s \nDeviceCertIssuerName:%s  \nManCertSubjectName:%s  \nManCertIssuerName:%s \n",  Mcardcertinfo.acHostId[0], Mcardcertinfo.acHostId[1], Mcardcertinfo.acHostId[2], Mcardcertinfo.acHostId[3], Mcardcertinfo.acHostId[4], Mcardcertinfo.bCertAvailable, Mcardcertinfo.bCertValid,  Mcardcertinfo.bVerifiedWithChain,Mcardcertinfo.bIsProduction,Mcardcertinfo.szDeviceCertSubjectName, Mcardcertinfo.szDeviceCertIssuerName, Mcardcertinfo.szManCertSubjectName ,Mcardcertinfo.szManCertIssuerName);
// 
//          }
//          else
//          {
//              SNMP_DEBUGPRINT("Mcardcertinfo.szDispString%s\n",Mcardcertinfo.szDispString);
// 
//         SNMP_DEBUGPRINT("Mcardcertinfo.szDeviceCertSubjectName%s\n",Mcardcertinfo.szDeviceCertSubjectName);
//         SNMP_DEBUGPRINT("Mfr Name  = %s\n",Mcardcertinfo.szDeviceCertIssuerName);
// 
//         SNMP_DEBUGPRINT("Model Name = %s\n",Mcardcertinfo.szManCertSubjectName);
//         SNMP_DEBUGPRINT("Model Name = %s\n", Mcardcertinfo.szManCertIssuerName);
// 
//         SNMP_DEBUGPRINT("Model Id = 0x%02X%02X%02X%02X\n", Mcardcertinfo.acHostId[0], Mcardcertinfo.acHostId[1],Mcardcertinfo.acHostId[2], Mcardcertinfo.acHostId[3]);
// 
//         if(Mcardcertinfo.bCertAvailable)
//             SNMP_DEBUGPRINT("Certificate Available = YES\n");
//         else
//              SNMP_DEBUGPRINT("Certificate Available = NO\n");
// 
//         if(Mcardcertinfo.bCertValid)
//               SNMP_DEBUGPRINT("Certificate Valid = YES\n");
//         else
//                   SNMP_DEBUGPRINT("Certificate Valid = NO\n");
// 
//                 snprintf(Vl_CertificationInfo[1].vl_VividlogicCertFullInfo,
//                          sizeof(Vl_CertificationInfo[1].vl_VividlogicCertFullInfo),
//                                 "HOSTID: %02x:%02x:%02x:%02x:%02x \nCertificate Available(TRUE:1 FALSE:0):%d  \nCertificate Valid(TRUE:1 FALSE:0):%d   \nVerifiedWithChain (TRUE:1 FALSE:0) : %d  \nProduction Keys(TRUE:1 FALSE:0): %d \nDeviceCertSubjectName: %s \nDeviceCertIssuerName:%s  \nManCertSubjectName:%s  \nManCertIssuerName:%s \n",  Mcardcertinfo.acHostId[0], Mcardcertinfo.acHostId[1], Mcardcertinfo.acHostId[2], Mcardcertinfo.acHostId[3], Mcardcertinfo.acHostId[4], Mcardcertinfo.bCertAvailable, Mcardcertinfo.bCertValid,  Mcardcertinfo.bVerifiedWithChain,Mcardcertinfo.bIsProduction,Mcardcertinfo.szDeviceCertSubjectName, Mcardcertinfo.szDeviceCertIssuerName, Mcardcertinfo.szManCertSubjectName ,Mcardcertinfo.szManCertIssuerName);
// 
// 
//         }
// 
// 
//         }
// 
 #ifdef USE_CDL
          /** To Get COMMONDOWNLOAD certification Info */
     static CDLMgrSnmpCertificateStatus_t CDLCertInfo;
     static bool bRetrievedCommonDownloadCerts = false;
     static time_t lastTime = time(NULL);
     if(false == bRetrievedCommonDownloadCerts)
     {
         if(vlAbs(time(NULL) - lastTime) > 10)
         {
             lastTime = time(NULL);
             memset(&Vl_CertificationInfo[0].vl_VividlogicCertFullInfo, 0 , sizeof(Vl_CertificationInfo[2].vl_VividlogicCertFullInfo));
             memset(&CDLCertInfo, 0 , sizeof(CDLCertInfo));
             int iRet = IPC_CLIENT_CommonDownloadMgrSnmpCertificateStatus_check(&CDLCertInfo);
             if (COMM_DL_SUCESS == iRet)
             {
                 snprintf(Vl_CertificationInfo[0].vl_VividlogicCertFullInfo,
                          sizeof(Vl_CertificationInfo[2].vl_VividlogicCertFullInfo),
                                 "RootCACertificate Available(TRUE:1 FALSE:0): %d\nCvcCACertificate Available(TRUE:1 FALSE:0): %d\nDeviceCVCCertificate Available(TRUE:1 FALSE:0): %d \nRootCA Subject: %s\nRootCA Start Dt: %s\nRootCA End Dt: %s\nCvc CA Subject: %s\nCvc CA Start Dt: %s\nCVC CA End Dt: %s\nDevice CVC Subject:\n  %s\nDevice CVC Start Dt: %s\nDevice CVC End Dt: %s \nCerts Verified with chain(TRUE:1 FALSE:0): %d\n", CDLCertInfo.bCLRootCACertAvailable, CDLCertInfo.bCvcCACertAvailable, CDLCertInfo.bDeviceCVCCertAvailable, CDLCertInfo.pCLRootCASubjectName, CDLCertInfo.pCLRootCAStartDate, CDLCertInfo.pCLRootCAEndDate, CDLCertInfo.pCLCvcCASubjectName, CDLCertInfo.pCLCvcCAStartDate, CDLCertInfo.pCLCvcCAEndDate, CDLCertInfo.pDeviceCvcSubjectName, CDLCertInfo.pDeviceCvcStartDate, CDLCertInfo.pDeviceCvcEndDate, CDLCertInfo.bVerifiedWithChain);
                 Vl_CertificationInfo[0].vl_VividlogicCertFullInfo_len = strlen(Vl_CertificationInfo[2].vl_VividlogicCertFullInfo);
                 bRetrievedCommonDownloadCerts = true;
             }
         }
     }
     
     if(false == bRetrievedCommonDownloadCerts)
     {
         char temp11[]="COMMONDOWNLOAD CERTIFICATION INFO";
         char temp21[]="Not Available / Waiting for Initialization";
         snprintf(Vl_CertificationInfo[0].vl_VividlogicCertFullInfo,
                  sizeof(Vl_CertificationInfo[0].vl_VividlogicCertFullInfo),
                         " %s", temp21);
         Vl_CertificationInfo[0].vl_VividlogicCertFullInfo_len = strlen(Vl_CertificationInfo[0].vl_VividlogicCertFullInfo);
     }
 
 #endif // USE_CDL
//         /* To Get HDMI certification Info         
//         unsigned char status;
//     unsigned char bDisplayHandleFound = 0;
//     unsigned long   desiredDisplayHandle;
//     AVOutInterfaceInfo    AVOutInterfaceInfo;
// 
//     unsigned int numDisplays = pfc_kernel_global->get_display_device()->get_display_handle_count();
//     unsigned long *pDisplayHandles = (unsigned long*) malloc( numDisplays * sizeof(unsigned long));
// 
//     pfc_kernel_global->get_display_device()->get_display_handles(numDisplays, pDisplayHandles, 0);
// 
//     for(int i=0; i<numDisplays; i++)
//     {
//         pfc_kernel_global->get_display_device()->get_avOutinterface_info(pDisplayHandles[i],  &AVOutInterfaceInfo, 0);
// 
//         for (int j=0; j < AVOutInterfaceInfo.numAvOutPorts; j++)
//         {
//             if(AVOutInterfaceInfo.avOutPortInfo[j].avOutPortType == AV_INTERFACE_TYPE_HDMI)
//             {
//                 bDisplayHandleFound = 1;
//                 break;
//             }
//         }
// 
//         if(bDisplayHandleFound)
//         {
//             desiredDisplayHandle =  pDisplayHandles[i];
//             break;
//         }
//     }
// 
//     free(pDisplayHandles);
// 
//     if (!(bDisplayHandleFound))
//     {
//             char temp12[35]="HDMI CERTIFICATION INFO";
//            char temp22[35]="Certification Not Available";
//           snprintf(Vl_CertificationInfo[3].vl_VividlogicCertFullInfo,
//                    sizeof(Vl_CertificationInfo[3].vl_VividlogicCertFullInfo),
//                           " %s", temp22);
//           Vl_CertificationInfo[3].vl_VividlogicCertFullInfo_len = strlen(Vl_CertificationInfo[3].vl_VividlogicCertFullInfo);
//     }
//     else
//     {
//         SNMPocStbHostDVIHDMITable_t DVIHDMITable_obj;
//         vlMemSet(&DVIHDMITable_obj,0,sizeof(DVIHDMITable_obj),sizeof(DVIHDMITable_obj));
//         vlMemSet(&Vl_CertificationInfo[3].vl_VividlogicCertFullInfo, 0 , sizeof(Vl_CertificationInfo[3].vl_VividlogicCertFullInfo) , sizeof(Vl_CertificationInfo[3].vl_VividlogicCertFullInfo));
//         pfc_kernel_global->get_display_device()->snmp_get_DviHdmi_info(desiredDisplayHandle, AV_INTERFACE_TYPE_HDMI, &DVIHDMITable_obj);
// 
//         RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "\n\n\n DVIHDMITable_obj.vl_ocStbHostDVIHDMIHDCPStatus = %d",DVIHDMITable_obj.vl_ocStbHostDVIHDMIHDCPStatus);
//         int HDCPstatus = true;
// 
//         if (DVIHDMITable_obj.vl_ocStbHostDVIHDMIHDCPStatus == STATUS_TRUE)
//             status = 1;
//         else
//             status = 0;
// 
//         snprintf(Vl_CertificationInfo[3].vl_VividlogicCertFullInfo,
//                  sizeof(Vl_CertificationInfo[3].vl_VividlogicCertFullInfo),
//                         "HDCP ENABLE STATUS (TRUE:1 FALSE:0): %d", status);
//         Vl_CertificationInfo[3].vl_VividlogicCertFullInfo_len = strlen(Vl_CertificationInfo[3].vl_VividlogicCertFullInfo);
//     }
// 
//         /** To Get OCAP certification Info 
//           char temp3[35]="OCAP CERTIFICATION INFO";
//            char temp4[35]="Certification Not Available";
//           snprintf(Vl_CertificationInfo[4].vl_VividlogicCertFullInfo,
//                    sizeof(Vl_CertificationInfo[4].vl_VividlogicCertFullInfo),
//                           " %s", temp4);
//           Vl_CertificationInfo[4].vl_VividlogicCertFullInfo_len = strlen(Vl_CertificationInfo[4].vl_VividlogicCertFullInfo);
// 
//        /**----------  To Get DOCSIS and DSG STSTUS----------             */
// 
//            VL_DSG_STATUS dsgAndDocsisStatus;
//            vlDsgGetDocsisStatus(&dsgAndDocsisStatus) ;
//            VL_POD_IP_STATUS podIpStatus;
//            vlXchanGetPodIpStatus(&podIpStatus);
// 
//            snprintf(Vl_CertificationInfo[6].vl_VividlogicCertFullInfo,
//                     sizeof(Vl_CertificationInfo[6].vl_VividlogicCertFullInfo),
//                            "%s", podIpStatus.szPodIpStatus );
//            Vl_CertificationInfo[6].vl_VividlogicCertFullInfo_len = strlen(Vl_CertificationInfo[6].vl_VividlogicCertFullInfo);
//            SNMP_DEBUGPRINT("APDU_HANDLE_NewFlowReq: POD_IP_STATUS %s\n", Vl_CertificationInfo[6].vl_VividlogicCertFullInfo );
//            snprintf(Vl_CertificationInfo[7].vl_VividlogicCertFullInfo,
//                     sizeof(Vl_CertificationInfo[7].vl_VividlogicCertFullInfo),
//                            "%s", dsgAndDocsisStatus.szDsgStatus );
//            Vl_CertificationInfo[7].vl_VividlogicCertFullInfo_len = strlen(Vl_CertificationInfo[7].vl_VividlogicCertFullInfo);
//            SNMP_DEBUGPRINT("APDU_HANDLE_NewFlowReq: DSG-STATUS %s\n", Vl_CertificationInfo[7].vl_VividlogicCertFullInfo );
//            snprintf(Vl_CertificationInfo[8].vl_VividlogicCertFullInfo,
//                     sizeof(Vl_CertificationInfo[8].vl_VividlogicCertFullInfo),
//                            "%s", dsgAndDocsisStatus.szDocsisStatus );
//            Vl_CertificationInfo[8].vl_VividlogicCertFullInfo_len = strlen(Vl_CertificationInfo[8].vl_VividlogicCertFullInfo);
//            SNMP_DEBUGPRINT("APDU_HANDLE_NewFlowReq:DOCSIS-STATUS %s\n", Vl_CertificationInfo[8].vl_VividlogicCertFullInfo);
// 
//         *nitem = 9;
//#endif //fi 0
         //SNMP_DEBUGPRINT("sizeof the list is:: %d\n",*nitem );*/
      return 1;
}

void Avinterface::vlGet_ocStbHostCard_Details(SocStbHostSnmpProxy_CardInfo_t *vl_ProxyCardInfo)
{
    VL_AUTO_LOCK(m_lock);
    
    if (! IsUpdateReqd_CheckStatus(vlg_timeCableCardDetailsLastUpdated, TIME_GAP_FOR_CARD_UPDATE_IN_SECONDS))
    {
        *vl_ProxyCardInfo = vlg_cableCardDetails;
        //auto_unlock_ptr(m_lock);
        return;
    }
   
    vlCableCardAppInfo_t *appInfo;
    //appInfo =  (vlCableCardAppInfo_t *)malloc(sizeof(vlCableCardAppInfo_t));
    rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(vlCableCardAppInfo_t), (void**)&appInfo);

    vlCableCardIdBndStsOobMode_t oobInfo;

        //vlMemSet(appInfo, 0, sizeof(vlCableCardAppInfo_t), sizeof(vlCableCardAppInfo_t));
        memset(appInfo, 0, sizeof(vlCableCardAppInfo_t));
		if( IPC_CLIENT_SNMPGetApplicationInfo(appInfo) != 0)
        {
            SNMP_DEBUGPRINT("SNMPGetApplicationInfo failed.\n");
        }
        ///To get default values commented the "else"
    else 
    {
            int i = 0;
            char szRootOID[sizeof(vl_ProxyCardInfo->ocStbHostCardRootOid)];

            //vlMemSet(&oobInfo, 0, sizeof(oobInfo), sizeof(oobInfo));
            memset(&oobInfo, 0, sizeof(oobInfo));
       if( IPC_CLIENT_SNMPGetCardIdBndStsOobMode(&oobInfo) != 0)
            {
                SNMP_DEBUGPRINT("SNMPGetCardIdBndStsOobMode failed.\n");
            }

            vl_ProxyCardInfo->ocStbHostCardVersion[0] = (appInfo->CardVersion      >>8)&0xFF;// 2 buy cable card version number
            vl_ProxyCardInfo->ocStbHostCardVersion[1] = (appInfo->CardVersion      >>0)&0xFF;// 2 buy cable card version number
            vl_ProxyCardInfo->ocStbHostCardMfgId[0]   = (appInfo->CardManufactureId>>8)&0xFF;// 2 byte cable card Manufacture Id
            vl_ProxyCardInfo->ocStbHostCardMfgId[1]   = (appInfo->CardManufactureId>>0)&0xFF;// 2 byte cable card Manufacture Id

            //vlMemSet(vl_ProxyCardInfo->ocStbHostCardRootOid, 0, sizeof(vl_ProxyCardInfo->ocStbHostCardRootOid), sizeof(vl_ProxyCardInfo->ocStbHostCardRootOid));
            memset(vl_ProxyCardInfo->ocStbHostCardRootOid, 0, sizeof(vl_ProxyCardInfo->ocStbHostCardRootOid));
            //vlMemSet(vl_ProxyCardInfo->ocStbHostCardSerialNumber, 0, sizeof(vl_ProxyCardInfo->ocStbHostCardSerialNumber), sizeof(vl_ProxyCardInfo->ocStbHostCardSerialNumber));
            memset(vl_ProxyCardInfo->ocStbHostCardSerialNumber, 0, sizeof(vl_ProxyCardInfo->ocStbHostCardSerialNumber));
            //vlMemSet(vl_ProxyCardInfo->ocStbHostCardId, 0, sizeof(vl_ProxyCardInfo->ocStbHostCardId), sizeof(vl_ProxyCardInfo->ocStbHostCardId));
            memset(vl_ProxyCardInfo->ocStbHostCardId, 0, sizeof(vl_ProxyCardInfo->ocStbHostCardId));
            
            vlMemCpy(szRootOID, appInfo->CardRootOid, appInfo->CardRootOidLen, sizeof(szRootOID));
            vlMemCpy(vl_ProxyCardInfo->ocStbHostCardSerialNumber, appInfo->CardSerialNumber, appInfo->CardSerialNumberLen, sizeof(vl_ProxyCardInfo->ocStbHostCardSerialNumber));
            vlMemCpy(vl_ProxyCardInfo->ocStbHostCardId, oobInfo.CableCardId, sizeof(vl_ProxyCardInfo->ocStbHostCardId), sizeof(vl_ProxyCardInfo->ocStbHostCardId));

            vl_ProxyCardInfo->gProxyCardInfo            = (Bindstatis_t)(oobInfo.CableCardBindingStatus);
            vl_ProxyCardInfo->ocStbHostOobMessageMode   = (OobMessageMode_t)(oobInfo.OobMessMode);
            vl_ProxyCardInfo->CardRootOidLen            = appInfo->CardRootOidLen;
            szRootOID[appInfo->CardRootOidLen] = '\0';
            SNMP_DEBUGPRINT("CardRootOidLen = %d, ocStbHostCardRootOid=%s\n", strlen(szRootOID), szRootOID);
            {
                char * pszSavePtr = NULL;
                char * pszOid = strtok_r(szRootOID, ".", &pszSavePtr);
                vl_ProxyCardInfo->CardRootOidLen = 0;
                while(NULL != pszOid)
                {
                    vl_ProxyCardInfo->ocStbHostCardRootOid[vl_ProxyCardInfo->CardRootOidLen] = atoi(pszOid);
                    pszOid = strtok_r(NULL, ".", &pszSavePtr);
                    vl_ProxyCardInfo->CardRootOidLen++;
                }
            }
    }
        unsigned long  nLong = 0;
        unsigned char  nChar = 0;
        unsigned short nShort = 0;
        
        nLong = 0;
		if(0 != IPC_CLIENT_SNMPGetGenFetResourceId(&nLong)) nLong = 0;
        vl_ProxyCardInfo->ocStbHostCardOpenedGenericResource[0] = (nLong>>24)&0xFF;
        vl_ProxyCardInfo->ocStbHostCardOpenedGenericResource[1] = (nLong>>16)&0xFF;
        vl_ProxyCardInfo->ocStbHostCardOpenedGenericResource[2] = (nLong>> 8)&0xFF;
        vl_ProxyCardInfo->ocStbHostCardOpenedGenericResource[3] = (nLong>> 0)&0xFF;
        
        vl_ProxyCardInfo->ocStbHostCardTimeZoneOffset = 0;
    	if(0 != IPC_CLIENT_SNMPGetGenFetTimeZoneOffset(&(vl_ProxyCardInfo->ocStbHostCardTimeZoneOffset))) vl_ProxyCardInfo->ocStbHostCardTimeZoneOffset = 0;
        
        vl_ProxyCardInfo->ocStbHostCardDaylightSavingsTimeDelta[0] = 0;
		if(0 != IPC_CLIENT_SNMPGetGenFetDayLightSavingsTimeDelta(vl_ProxyCardInfo->ocStbHostCardDaylightSavingsTimeDelta)) vl_ProxyCardInfo->ocStbHostCardDaylightSavingsTimeDelta[0] = 0;
        
        vl_ProxyCardInfo->ocStbHostCardDaylightSavingsTimeEntry = 0;
    	if(0 != IPC_CLIENT_SNMPGetGenFetDayLightSavingsTimeEntry((unsigned long*)&(vl_ProxyCardInfo->ocStbHostCardDaylightSavingsTimeEntry))) vl_ProxyCardInfo->ocStbHostCardDaylightSavingsTimeEntry = 0;
        
        vl_ProxyCardInfo->ocStbHostCardDaylightSavingsTimeExit = 0;
    	if(0 != IPC_CLIENT_SNMPGetGenFetDayLightSavingsTimeExit((unsigned long*)&(vl_ProxyCardInfo->ocStbHostCardDaylightSavingsTimeExit))) vl_ProxyCardInfo->ocStbHostCardDaylightSavingsTimeExit = 0;
        
        nChar = 0; nShort = 0;

    	if(0 != IPC_CLIENT_SNMPGetGenFetEasLocationCode(&nChar, &nShort)) { nChar = 0; nShort = 0;}
        vl_ProxyCardInfo->ocStbHostCardEaLocationCode[0] = (nChar >> 0)&0xFF;
        vl_ProxyCardInfo->ocStbHostCardEaLocationCode[1] = (nShort>> 8)&0xFF;
        vl_ProxyCardInfo->ocStbHostCardEaLocationCode[2] = (nShort>> 0)&0xFF;
        
        if((0 == vl_ProxyCardInfo->ocStbHostCardEaLocationCode[0]) &&
           (0 == (0xF3&(vl_ProxyCardInfo->ocStbHostCardEaLocationCode[1]))) &&
           (0 == vl_ProxyCardInfo->ocStbHostCardEaLocationCode[2]))
        {
            vl_ProxyCardInfo->ocStbHostCardEaLocationCode[1] = 0;
        }
        
        nShort = 0;
    if(0 != IPC_CLIENT_SNMPGetGenFetVctId(&nShort)) nShort = 0;
        vl_ProxyCardInfo->ocStbHostCardVctId[0] = (nShort>> 8)&0xFF;
        vl_ProxyCardInfo->ocStbHostCardVctId[1] = (nShort>> 0)&0xFF;
        
        vl_ProxyCardInfo->ocStbHostCardCpAuthKeyStatus = (keyStatus_t)0;
    if(0 != IPC_CLIENT_SNMPGetCpAuthKeyStatus((int*)&(vl_ProxyCardInfo->ocStbHostCardCpAuthKeyStatus))) vl_ProxyCardInfo->ocStbHostCardCpAuthKeyStatus = (keyStatus_t)0;
        
        vl_ProxyCardInfo->ocStbHostCardCpCertificateCheck = (CpCertificateCheck_t)0;
    if(0 != IPC_CLIENT_SNMPGetCpCertCheck((int*)&(vl_ProxyCardInfo->ocStbHostCardCpCertificateCheck))) vl_ProxyCardInfo->ocStbHostCardCpCertificateCheck = (CpCertificateCheck_t)0;
        
        vl_ProxyCardInfo->ocStbHostCardCpCciChallengeCount = 0;
    if(0 != IPC_CLIENT_SNMPGetCpCciChallengeCount((unsigned long*)&(vl_ProxyCardInfo->ocStbHostCardCpCciChallengeCount))) vl_ProxyCardInfo->ocStbHostCardCpCciChallengeCount = 0;
        
        vl_ProxyCardInfo->ocStbHostCardCpKeyGenerationReqCount = 0;
    if(0 != IPC_CLIENT_SNMPGetCpKeyGenerationReqCount((unsigned long*)&(vl_ProxyCardInfo->ocStbHostCardCpKeyGenerationReqCount))) vl_ProxyCardInfo->ocStbHostCardCpKeyGenerationReqCount = 0;
        
        nLong = 0;
    if(0 != IPC_CLIENT_SNMPGetCpIdList(&nLong)) nLong = 0;
        vl_ProxyCardInfo->ocStbHostCardCpIdList[0] = (nLong>>24)&0xFF;
        vl_ProxyCardInfo->ocStbHostCardCpIdList[1] = (nLong>>16)&0xFF;
        vl_ProxyCardInfo->ocStbHostCardCpIdList[2] = (nLong>> 8)&0xFF;
        vl_ProxyCardInfo->ocStbHostCardCpIdList[3] = (nLong>> 0)&0xFF;
  
      // NEW mib ocStbHostCardSnmpAccessControl Default it will be true
        
    if(gcardsnmpAccCtl == STATUS_TRUE)
    {
        vl_ProxyCardInfo->ocStbHostCardSnmpAccessControl = STATUS_TRUE;
    }
    if(gcardsnmpAccCtl == STATUS_FALSE)
    {
        vl_ProxyCardInfo->ocStbHostCardSnmpAccessControl = STATUS_FALSE;
    }
    //SNMP_DEBUGPRINT("%d %s ---------------gcardsnmpAccCtl =%d  vl_ProxyCardInfo->ocStbHostCardSnmpAccessControl =%d \n",  __LINE__,__FUNCTION__, gcardsnmpAccCtl,  vl_ProxyCardInfo->ocStbHostCardSnmpAccessControl);
    //free(appInfo);
    rmf_osal_memFreeP(RMF_OSAL_MEM_POD, appInfo);
    
    vlg_cableCardDetails = *vl_ProxyCardInfo;
    getsystemUPtime(&vlg_timeCableCardDetailsLastUpdated);
    //auto_unlock_ptr(m_lock);
}

static void Avinterface_GetPhysMem(int * pPhysMemory, int * pAvailPhysMemory)
{
    FILE *fp = fopen("/proc/meminfo", "r");
    unsigned long mem   = 0;
    char szBuffer[32]; // dummy buffer
    if (NULL != fp)
    {
    	setvbuf(fp, (char *) NULL, _IONBF, 0);
        fseek(fp, 0, SEEK_SET);
        if (fscanf(fp, "%s%d%s%s%d", szBuffer, pPhysMemory, szBuffer, szBuffer, pAvailPhysMemory) <=0 )
        {
            SNMP_DEBUGPRINT("fscanf returned zero or negative value.\n");
        }
        fclose(fp);
    }
}


void Avinterface::vlGet_OcstbHostMemoryInfo(SMemoryInfo_t* HostMemoryInfo)
{
    VL_AUTO_LOCK(m_lock);
    VL_SNMP_HostMemoryInfo snmpHostMemoryInfo = {0};

    CHalSys_snmp_request(VL_SNMP_REQUEST_GET_SYS_HOST_MEMORY_INFO, &snmpHostMemoryInfo);
    HostMemoryInfo->AvilabelBlock         = snmpHostMemoryInfo.largestAvailableBlock;
    HostMemoryInfo->videoMemory           = snmpHostMemoryInfo.totalVideoMemory;
    HostMemoryInfo->AvailableVideoMemory  = snmpHostMemoryInfo.availableVideoMemory;
    Avinterface_GetPhysMem(&(HostMemoryInfo->systemMemory), &(HostMemoryInfo->AvailableSystemMemory));
    //auto_unlock_ptr(m_lock);
}

static int vlg_nOobCarouselTimeoutCount = 0;
static int vlg_nInbandCarouselTimeoutCount = 0;

unsigned int Avinterface::vlSet_ocStbHostOobCarouselTimeoutCount(int nTimeoutCount)
{
    VL_AUTO_LOCK(m_lock);
    vlg_nOobCarouselTimeoutCount = nTimeoutCount;
    //auto_unlock_ptr(m_lock);
    return 1;
}

unsigned int Avinterface::vlSet_ocStbHostInbandCarouselTimeoutCount(int nTimeoutCount)
{
    VL_AUTO_LOCK(m_lock);
    vlg_nInbandCarouselTimeoutCount = nTimeoutCount;
    //auto_unlock_ptr(m_lock);
    return 1;
}

unsigned int Avinterface::vlGet_ocStbHostOobCarouselTimeoutCount(int * pTimeoutCount)
{
    VL_AUTO_LOCK(m_lock);
    *pTimeoutCount = vlg_nOobCarouselTimeoutCount;
    //auto_unlock_ptr(m_lock);
    return 1;
}

unsigned int Avinterface::vlGet_ocStbHostInbandCarouselTimeoutCount(int * pTimeoutCount)
{
    VL_AUTO_LOCK(m_lock);
    *pTimeoutCount = vlg_nInbandCarouselTimeoutCount;
    //auto_unlock_ptr(m_lock);
    return 1;
}

static unsigned int vlg_nPatTimeout = 0;
static unsigned int vlg_nPmtTimeout = 0;

unsigned int Avinterface::vlSet_ocStbHostPatPmtTimeoutCount(unsigned long nPatTimeout,unsigned long nPmtTimeout)
{
    vlg_nPatTimeout = nPatTimeout;
    vlg_nPmtTimeout = nPmtTimeout;
    return 0;	
}

unsigned int Avinterface::vlGet_ocStbHostPatPmtTimeoutCount(unsigned long* pPATErrs,unsigned long* pPMTErrs)
{
    int nRet = 0;
    nRet = IPC_CLIENT_SNMPGetPatTimeoutCount(&vlg_nPatTimeout);
    if(RMF_SUCCESS == nRet)
    {
        *pPATErrs = vlg_nPatTimeout;
    }
    else
    {
        *pPATErrs = 0xFFFFFFFF;
    }
    nRet = IPC_CLIENT_SNMPGetPmtTimeoutCount(&vlg_nPmtTimeout);
    if(RMF_SUCCESS == nRet)
    {
        *pPMTErrs = vlg_nPmtTimeout;
    }
    else
    {
        *pPMTErrs = 0xFFFFFFFF;
    }
    return 1;
}
Avinterface::~Avinterface()
{

 // not to don
}

//#endif
