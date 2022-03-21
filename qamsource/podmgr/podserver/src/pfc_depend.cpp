/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2018 RDK Management
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_arp.h>
#include <linux/route.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "netUtils.h"
#include "rmf_osal_mem.h"
#include "vlMutex.h"
#include "cardUtils.h"
#include "cablecarddriver.h"

#include "rdk_debug.h"
#include "rmf_oobsisttmgr.h"
#include "hostGenericFeatures.h"
#include <podmgr.h>
#include <cm_api.h>
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "sys_api.h"

#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif

static volatile rmf_osal_eventmanager_handle_t  pod_event_manager;
static vlMutex g_pod_event_mgr;
static int POD_EVENT_MANAGER_CREATED = 0;
#define VL_INIT_TEXT    "init"
#define VL_COLD_INIT_TEXT    "cold.init."

rmf_Error create_pod_event_manager()
{VL_AUTO_LOCK ( g_pod_event_mgr );

	rmf_Error ret = RMF_SUCCESS;
	if(!POD_EVENT_MANAGER_CREATED)
	{
		rmf_osal_eventmanager_handle_t t_pod_event_manager;
		ret = rmf_osal_eventmanager_create( &t_pod_event_manager);
		if (RMF_SUCCESS != ret)
		{
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
							  "rmf_osal_eventmanager_create() failed\n");
			return ret;
		}
		pod_event_manager = t_pod_event_manager;
		POD_EVENT_MANAGER_CREATED = 1;
		return ret;
	}
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD",
							  "pod_event_manager already created\n");
	return ret;
}

rmf_Error delete_pod_event_manager()
{
	rmf_Error ret = RMF_SUCCESS;
	if(POD_EVENT_MANAGER_CREATED)
	{
		ret = rmf_osal_eventmanager_delete(pod_event_manager);
		if (RMF_SUCCESS != ret)
		{
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
							  "rmf_osal_eventmanager_delete() failed\n");
			return ret;
		}
		POD_EVENT_MANAGER_CREATED = 0;
		return ret;
	}
	RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
							  "Error!No pod_event_manager to delete\n");
	return ret;
}

rmf_osal_eventmanager_handle_t get_pod_event_manager()
{

	if(POD_EVENT_MANAGER_CREATED)
	{	
		/* Default event manager  */
		return pod_event_manager;
	}
	else
	{
		return NULL;
	}
}

void podmgr_freefn(void *data)
{

	rmf_osal_memFreeP(RMF_OSAL_MEM_POD, data);

}

int fpSetTextSimple(const char* pszText)
{

      RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","fp text Display = [%s]\n",pszText);
	  if(strcmp(VL_INIT_TEXT,pszText) == 0)
	  {
	  	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Got Init.Sending Event\n",pszText);
      	rmf_osal_eventmanager_handle_t  em = get_pod_event_manager();	  
      	rmf_osal_event_handle_t event_handle;
      	rmf_osal_event_params_t event_params = {0};
      	rmf_osal_event_category_t event_category;
      	int eEventType = RMF_BOOTMSG_DIAG_EVENT_TYPE_INIT;

      	event_params.data = NULL;
      	event_params.data_extension = RMF_BOOTMSG_DIAG_MESG_CODE_DAC_INIT;
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Creating Event\n",pszText);
      	rmf_osal_event_create(RMF_OSAL_EVENT_CATEGORY_BOOTMSG_DIAG, eEventType,&event_params, &event_handle);
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Sending Event\n",pszText);
		rmf_osal_eventmanager_dispatch_event(em, event_handle);
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Event Sent\n",pszText);
	  }
	  else if(strcmp(VL_COLD_INIT_TEXT,pszText) == 0)
	  {
	  	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Got Cold Init.Sending Event\n",pszText);
      	rmf_osal_eventmanager_handle_t  em = get_pod_event_manager();	  
      	rmf_osal_event_handle_t event_handle;
      	rmf_osal_event_params_t event_params = {0};
      	rmf_osal_event_category_t event_category;
      	int eEventType = RMF_BOOTMSG_DIAG_EVENT_TYPE_COLD_INIT;

      	event_params.data = NULL;
      	event_params.data_extension = RMF_BOOTMSG_DIAG_MESG_CODE_DAC_COLD_INIT;
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Creating Event\n",pszText);
      	rmf_osal_event_create(RMF_OSAL_EVENT_CATEGORY_BOOTMSG_DIAG, eEventType,&event_params, &event_handle);
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Sending Event\n",pszText);
		rmf_osal_eventmanager_dispatch_event(em, event_handle);
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Event Sent\n",pszText);	  
	  }
	  else
	  {
	  	//Do nothing
	  }
	  
	  return 0;

}

int fpEnableUpdates(int bEnable)
{

      RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","fp enable = [%s]\n", (bEnable == 1 ) ? "Enabled": "Disabled" );

      	rmf_osal_eventmanager_handle_t  em = get_pod_event_manager();	  
      	rmf_osal_event_handle_t event_handle;
      	rmf_osal_event_params_t event_params = {0};
      	rmf_osal_event_category_t event_category;
      	int eEventType = RMF_BOOTMSG_DIAG_EVENT_TYPE_FP_STATUS;

      	event_params.data = NULL;
      	event_params.data_extension = 
			(bEnable == 1 ) ?  RMF_BOOTMSG_DIAG_MESG_CODE_FP_TIME_ENABLE : RMF_BOOTMSG_DIAG_MESG_CODE_FP_TIME_DISABLE;
      	rmf_osal_event_create(RMF_OSAL_EVENT_CATEGORY_BOOTMSG_DIAG, eEventType,&event_params, &event_handle);
	rmf_osal_eventmanager_dispatch_event(em, event_handle);
  
	return 0;

}

void vlhal_stt_SetTimeZone(int featureId)
{
        //flag which indicates whether dst is in place and has been already recetimezoneinfo.nTimeZoneOffset/60;ived
        static int daylightflag = 0;
        static int timezoneinhours;
       rmf_OobSiSttMgr *sttMgr = rmf_OobSiSttMgr::getInstance();


        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "Generic feature changed event: FeatureId %d\n", featureId);

        if(featureId == VL_HOST_FEATURE_PARAM_Time_Zone)
        {
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "generic feature changed event. timezone\n");

                VL_POD_HOST_FEATURE_TIME_ZONE_OFFSET timezoneinfo;
                vlPodGetGenericFeatureData(VL_HOST_FEATURE_PARAM_Time_Zone, &timezoneinfo);
                // Validate the read timezoneoffset and if correct set the timezone
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "timezoneinfo.nTimeZoneOffset %x\n",timezoneinfo.nTimeZoneOffset);
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "timezoneinhours is %d\n",timezoneinhours);

                timezoneinhours = timezoneinfo.nTimeZoneOffset/60;
                if(timezoneinhours >= -12 && timezoneinhours <=12 )
                {
                        //Set the TZ environment variable
                        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "calling vlhal_stt_SetTimeZone with daylightflag %d timezoneinhours %d\n",daylightflag,timezoneinhours);
                        sttMgr->setTimeZone(timezoneinhours,daylightflag);
                        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "TZ is %s\n",getenv("TZ"));
                }
                else
                {
                        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "feature timezone falls in invalid range. Not setting it\n");
                }
        }
        else if(featureId == VL_HOST_FEATURE_PARAM_Daylight_Savings_Control)
        {
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "Generic feature changed event. dst\n");
#if 0
                VL_POD_HOST_FEATURE_DAYLIGHT_SAVINGS dstinfo;
                vlPodGetGenericFeatureData(VL_HOST_FEATURE_PARAM_Daylight_Savings_Control, &dstinfo);
                MPE_LOG(MPE_LOG_DEBUG, MPE_MOD_POD, "dstinfo.eDaylightSavingsType %d dstinfo.nDaylightSavingsDeltaMinutes %ld dstinfo.tmDaylightSavingsEntryTime %ld dstinfo.tmDaylightSavingsEntryTime %ld\n",dstinfo.eDaylightSavingsType,dstinfo.nDaylightSavingsDeltaMinutes,dstinfo.tmDaylightSavingsEntryTime,dstinfo.tmDaylightSavingsExitTime);

                if(dstinfo.eDaylightSavingsType == 0x02)
                {
                        if(dstinfo.nDaylightSavingsDeltaMinutes <= 60)
                        {
                                //Set the TZ environment variable
                                MPE_LOG(MPE_LOG_DEBUG, MPE_MOD_POD, "timezoneinhours is %d\n",timezoneinhours);
                                daylightflag = 1;
                                MPE_LOG(MPE_LOG_DEBUG, MPE_MOD_POD, "Calling vlhal_stt_SetTimeZone with daylightflag %d timezoneinhours %d\n",daylightflag,timezoneinhours);
                                setTimeZone(timezoneinhours,daylightflag);
                                MPE_LOG(MPE_LOG_DEBUG, MPE_MOD_POD, "TZ is %s\n",getenv("TZ"));
                        }
                }
#endif
        daylightflag = sttMgr->CheckToApplyDST();

        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: Calling setTimeZone with daylightflag %d timezoneinhours %d\n",__FUNCTION__, daylightflag,timezoneinhours);
        sttMgr->setTimeZone(timezoneinhours, daylightflag);
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "TZ is %s\n",getenv("TZ"));

        }
}

void getsystemUPtime(long *syst)
{
    if(NULL != syst)
    {
        *syst = get_uptime();
        //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s() : get_uptime() returned : %d\n", __FUNCTION__, *syst);
        if (*syst == -1)
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","time returned  error \n", __FUNCTION__);

    }
    else
    {
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s() : NULL param\n", __FUNCTION__);
    }
}

bool IsUpdateReqd_CheckStatus(long & mcurrentTime, long timeInterval)
{
    long curTimeStamp;
    long timeDiff = 0;
    getsystemUPtime(&curTimeStamp);
    mcurrentTime > curTimeStamp ?
        (timeDiff = (mcurrentTime - curTimeStamp)) :
            (timeDiff = (curTimeStamp - mcurrentTime));

   if (timeDiff < timeInterval)
   {
       RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "IsUpdateReqd_CheckStatus : timeDiff = %d, mcurrentTime =%d\n", timeDiff,mcurrentTime);
       return false;
   }
   mcurrentTime = curTimeStamp;
   return true;
}

#define TIME_GAP_FOR_CARD_UPDATE_IN_SECONDS 15

static long vlg_timeCableCardDetailsLastUpdated = 0;
#ifndef SNMP_IPC_CLIENT
Status_t gcardsnmpAccCtl =  STATUS_TRUE;
static SocStbHostSnmpProxy_CardInfo_t vlg_cableCardDetails;

void vlGet_ocStbHostCard_Details(SocStbHostSnmpProxy_CardInfo_t *vl_ProxyCardInfo)
{
//        only one user
//    VL_AUTO_LOCK(m_lock);
    
    if (! IsUpdateReqd_CheckStatus(vlg_timeCableCardDetailsLastUpdated, TIME_GAP_FOR_CARD_UPDATE_IN_SECONDS))
    {
        *vl_ProxyCardInfo = vlg_cableCardDetails;
        return;
    }
   
    vlCableCardAppInfo_t *appInfo;
	rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(vlCableCardAppInfo_t),(void **)&appInfo);

    vlCableCardIdBndStsOobMode_t oobInfo;

    cCardManagerIF *pCM = cCardManagerIF::getInstance();

    SNMP_DEBUGPRINT("pCM=0x%p\n", pCM);
    if(pCM)
    {
        memset(appInfo, 0, sizeof(vlCableCardAppInfo_t));
        if( pCM->SNMPGetApplicationInfo(appInfo) != 0)
        {
            SNMP_DEBUGPRINT("SNMPGetApplicationInfo failed.\n");
        }
        ///To get default values commented the "else"
//         else 
//         {
            int i = 0;
            char szRootOID[sizeof(vl_ProxyCardInfo->ocStbHostCardRootOid)];

            memset(&oobInfo, 0, sizeof(oobInfo));
            if( pCM->SNMPGetCardIdBndStsOobMode(&oobInfo) != 0)
            {
                SNMP_DEBUGPRINT("SNMPGetCardIdBndStsOobMode failed.\n");
            }

            vl_ProxyCardInfo->ocStbHostCardVersion[0] = (appInfo->CardVersion      >>8)&0xFF;// 2 buy cable card version number
            vl_ProxyCardInfo->ocStbHostCardVersion[1] = (appInfo->CardVersion      >>0)&0xFF;// 2 buy cable card version number
            vl_ProxyCardInfo->ocStbHostCardMfgId[0]   = (appInfo->CardManufactureId>>8)&0xFF;// 2 byte cable card Manufacture Id
            vl_ProxyCardInfo->ocStbHostCardMfgId[1]   = (appInfo->CardManufactureId>>0)&0xFF;// 2 byte cable card Manufacture Id

            memset(vl_ProxyCardInfo->ocStbHostCardRootOid, 0, sizeof(vl_ProxyCardInfo->ocStbHostCardRootOid));
            memset(vl_ProxyCardInfo->ocStbHostCardSerialNumber, 0, sizeof(vl_ProxyCardInfo->ocStbHostCardSerialNumber));
            memset(vl_ProxyCardInfo->ocStbHostCardId, 0, sizeof(vl_ProxyCardInfo->ocStbHostCardId));

            rmf_osal_memcpy(szRootOID, appInfo->CardRootOid, appInfo->CardRootOidLen, sizeof(szRootOID), sizeof(appInfo->CardRootOid) );
            rmf_osal_memcpy(vl_ProxyCardInfo->ocStbHostCardSerialNumber, appInfo->CardSerialNumber, appInfo->CardSerialNumberLen, sizeof(vl_ProxyCardInfo->ocStbHostCardSerialNumber), sizeof(appInfo->CardSerialNumber) );
            memcpy(vl_ProxyCardInfo->ocStbHostCardId, oobInfo.CableCardId, sizeof(vl_ProxyCardInfo->ocStbHostCardId));

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
//         }
        
        unsigned long  nLong = 0;
        unsigned char  nChar = 0;
        unsigned short nShort = 0;
        
        nLong = 0;
        if(0 != pCM->SNMPGetGenFetResourceId(&nLong)) nLong = 0;
        vl_ProxyCardInfo->ocStbHostCardOpenedGenericResource[0] = (nLong>>24)&0xFF;
        vl_ProxyCardInfo->ocStbHostCardOpenedGenericResource[1] = (nLong>>16)&0xFF;
        vl_ProxyCardInfo->ocStbHostCardOpenedGenericResource[2] = (nLong>> 8)&0xFF;
        vl_ProxyCardInfo->ocStbHostCardOpenedGenericResource[3] = (nLong>> 0)&0xFF;
        
        vl_ProxyCardInfo->ocStbHostCardTimeZoneOffset = 0;
        if(0 != pCM->SNMPGetGenFetTimeZoneOffset(&(vl_ProxyCardInfo->ocStbHostCardTimeZoneOffset))) vl_ProxyCardInfo->ocStbHostCardTimeZoneOffset = 0;
        
        vl_ProxyCardInfo->ocStbHostCardDaylightSavingsTimeDelta[0] = 0;
        if(0 != pCM->SNMPGetGenFetDayLightSavingsTimeDelta(vl_ProxyCardInfo->ocStbHostCardDaylightSavingsTimeDelta)) vl_ProxyCardInfo->ocStbHostCardDaylightSavingsTimeDelta[0] = 0;
        
        vl_ProxyCardInfo->ocStbHostCardDaylightSavingsTimeEntry = 0;
        if(0 != pCM->SNMPGetGenFetDayLightSavingsTimeEntry((unsigned long*)&(vl_ProxyCardInfo->ocStbHostCardDaylightSavingsTimeEntry))) vl_ProxyCardInfo->ocStbHostCardDaylightSavingsTimeEntry = 0;
        
        vl_ProxyCardInfo->ocStbHostCardDaylightSavingsTimeExit = 0;
        if(0 != pCM->SNMPGetGenFetDayLightSavingsTimeExit((unsigned long*)&(vl_ProxyCardInfo->ocStbHostCardDaylightSavingsTimeExit))) vl_ProxyCardInfo->ocStbHostCardDaylightSavingsTimeExit = 0;
        
        nChar = 0; nShort = 0;
        if(0 != pCM->SNMPGetGenFetEasLocationCode(&nChar, &nShort)) { nChar = 0; nShort = 0;}
        vl_ProxyCardInfo->ocStbHostCardEaLocationCode[0] = (nChar >> 0)&0xFF;
        vl_ProxyCardInfo->ocStbHostCardEaLocationCode[1] = (nShort>> 8)&0xFF;
        vl_ProxyCardInfo->ocStbHostCardEaLocationCode[2] = (nShort>> 0)&0xFF;
        
        nShort = 0;
        if(0 != pCM->SNMPGetGenFetVctId(&nShort)) nShort = 0;
        vl_ProxyCardInfo->ocStbHostCardVctId[0] = (nShort>> 8)&0xFF;
        vl_ProxyCardInfo->ocStbHostCardVctId[1] = (nShort>> 0)&0xFF;
        
        vl_ProxyCardInfo->ocStbHostCardCpAuthKeyStatus = (keyStatus_t)0;
        if(0 != pCM->SNMPGetCpAuthKeyStatus((int*)&(vl_ProxyCardInfo->ocStbHostCardCpAuthKeyStatus))) vl_ProxyCardInfo->ocStbHostCardCpAuthKeyStatus = (keyStatus_t)0;
        
        vl_ProxyCardInfo->ocStbHostCardCpCertificateCheck = (CpCertificateCheck_t)0;
        if(0 != pCM->SNMPGetCpCertCheck((int*)&(vl_ProxyCardInfo->ocStbHostCardCpCertificateCheck))) vl_ProxyCardInfo->ocStbHostCardCpCertificateCheck = (CpCertificateCheck_t)0;
        
        vl_ProxyCardInfo->ocStbHostCardCpCciChallengeCount = 0;
        if(0 != pCM->SNMPGetCpCciChallengeCount((unsigned long*)&(vl_ProxyCardInfo->ocStbHostCardCpCciChallengeCount))) vl_ProxyCardInfo->ocStbHostCardCpCciChallengeCount = 0;
        
        vl_ProxyCardInfo->ocStbHostCardCpKeyGenerationReqCount = 0;
        if(0 != pCM->SNMPGetCpKeyGenerationReqCount((unsigned long*)&(vl_ProxyCardInfo->ocStbHostCardCpKeyGenerationReqCount))) vl_ProxyCardInfo->ocStbHostCardCpKeyGenerationReqCount = 0;
        
        nLong = 0;
        if(0 != pCM->SNMPGetCpIdList(&nLong)) nLong = 0;
        vl_ProxyCardInfo->ocStbHostCardCpIdList[0] = (nLong>>24)&0xFF;
        vl_ProxyCardInfo->ocStbHostCardCpIdList[1] = (nLong>>16)&0xFF;
        vl_ProxyCardInfo->ocStbHostCardCpIdList[2] = (nLong>> 8)&0xFF;
        vl_ProxyCardInfo->ocStbHostCardCpIdList[3] = (nLong>> 0)&0xFF;
  
      // NEW mib ocStbHostCardSnmpAccessControl Default it will be true
        
    }
    if(gcardsnmpAccCtl == STATUS_TRUE)
    {
        vl_ProxyCardInfo->ocStbHostCardSnmpAccessControl = STATUS_TRUE;
    }
    if(gcardsnmpAccCtl == STATUS_FALSE)
    {
        vl_ProxyCardInfo->ocStbHostCardSnmpAccessControl = STATUS_FALSE;
    }
    //SNMP_DEBUGPRINT("%d %s ---------------gcardsnmpAccCtl =%d  vl_ProxyCardInfo->ocStbHostCardSnmpAccessControl =%d \n",  __LINE__,__FUNCTION__, gcardsnmpAccCtl,  vl_ProxyCardInfo->ocStbHostCardSnmpAccessControl);
	rmf_osal_memFreeP(RMF_OSAL_MEM_POD, appInfo);
    
    vlg_cableCardDetails = *vl_ProxyCardInfo;
    getsystemUPtime(&vlg_timeCableCardDetailsLastUpdated);
}

void vlGet_HostCaTypeInfo(HostCATInfo_t* CAtypeinfo)
{

    cCardManagerIF *pCM = cCardManagerIF::getInstance();

     if(pCM)
     {
       unsigned int result = 0xff;

        HostCATypeInfo_t HostCaTypeInfo;
        memset(&HostCaTypeInfo, 0, sizeof(HostCaTypeInfo));
        result = pCM->SNMPget_ocStbHostSecurityIdentifier(&HostCaTypeInfo);
        GetHostIdLuhn((unsigned char*)(HostCaTypeInfo.SecurityID), (char*)(CAtypeinfo->S_SecurityID));

       // snprintf(CAtypeinfo->S_CASystemID,
       //          sizeof(CAtypeinfo->S_CASystemID),
       //                 "%d",HostCaTypeInfo.CASystemID);

       podImplGetCaSystemId(&HostCaTypeInfo.CASystemID);
       
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
     }

}
#endif //SNMP_IPC_CLIENT


int checkTime(unsigned char one[12], unsigned char two[12])
{

int ret = -1;

{
    int ii;
RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","checkTime: In this function printing the Time  one and two \n");
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","Time One: ");
   for(ii = 0; ii < 12 ; ii++)
   {
       RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","0x%02X: ", one[ii]);
    //if(one[ii] == 0xFF)
    //    one[ii] = 0x30;
   }
RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","\n");
RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","Time two: ");
   for(ii = 0; ii < 12 ; ii++)
   {
       RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","0x%02X: ", two[ii]);
    //if(two[ii] == 0xFF)
    //    two[ii] = 0x30;
   }
RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","\n");
}

		long oneLong = atol((const char *)one);
		long twoLong = atol((const char *)two);

		if (oneLong == twoLong) {
			ret = 0;
		} else if (twoLong > oneLong) {
			ret = 1;
		}

RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","checkTime ## 8\n");

		return ret;

}
int verifySignedContent(void *code, int len, char bFile)
{
return 0;

}

extern "C" int rmf_RebootHostTillLimit(const char* pszRequestor, const char* pszReason, const char* pszCounterName, int nLimit)
{
    char szCounterFileName[64];
    snprintf(szCounterFileName, sizeof(szCounterFileName), "/opt/reboot_counter_%s.txt", pszCounterName);
    char szLimitReachedFileName[64];
    snprintf(szLimitReachedFileName, sizeof(szLimitReachedFileName), "/tmp/reboot_limit_reached_%s", pszCounterName);
    unsigned int nRecordedCount = 0;
    int bShouldReboot = 0;
    struct stat st;
    // get limit reached state
    int bLimitReached = ((0 == stat(szLimitReachedFileName, &st) && (0 != st.st_ino)));
    // read current count
    FILE * fp = fopen(szCounterFileName, "r");
    if(NULL != fp)
    {
        if ( fscanf(fp, "%u", &nRecordedCount) <=0 )
        {
            /* Unhandled error */
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SYS","vlMpeosRebootHostTillLimit() - fscanf returned zero or negative value... \n");
        }
        fclose(fp);
    }
    if(nRecordedCount >= nLimit)
    {
        if(!bLimitReached)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SYS","vlMpeosRebootHostTillLimit() requested by '%s' for '%s', counter '%s', is already '%d' and limit is '%d, skipping reboot...' \n", pszRequestor, pszReason, szCounterFileName, nRecordedCount, nLimit);
            FILE * fp = fopen(szLimitReachedFileName, "w");
            if(NULL != fp)
            {
                fprintf(fp, "limit_reached\n");
                fclose(fp);
            }
        }
        nRecordedCount = 0;
    }
    else
    {
        if(!bLimitReached)
        {
            // increment count
            nRecordedCount++;
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SYS","vlMpeosRebootHostTillLimit() requested by '%s' for '%s', counter '%s', is now '%d' and limit is '%d, will reboot...' \n", pszRequestor, pszReason, szCounterFileName, nRecordedCount, nLimit);
            bShouldReboot = 1;
        }
        else
        {
            nRecordedCount = 0;
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS","vlMpeosRebootHostTillLimit() requested by '%s' for '%s', counter '%s', limit of '%d reached, skipping reboot.' \n", pszRequestor, pszReason, szCounterFileName, nLimit);
        }
    }
    fp = fopen(szCounterFileName, "w");
    if(NULL != fp)
    {
        fprintf(fp, "%u\n", nRecordedCount);
        fflush(fp);
        fsync(fileno(fp));
        fclose(fp);
    }
    if(bShouldReboot)
    {
        // the following call does not return.
        HAL_SYS_Reboot(pszRequestor, pszReason);
    }
    
    return 0;
}

