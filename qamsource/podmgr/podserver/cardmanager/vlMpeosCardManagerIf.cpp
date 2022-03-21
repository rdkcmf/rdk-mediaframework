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



#include "vlEnv.h"
#include "sys_init.h"
#include <stdint.h>
#include "cardManagerIf.h"
#include "podIf.h"
#include "halpodclass.h"
#include "cardmanager.h"
//#include "vlMpeosAutoLock.h"
#include "utilityMacros.h"
#include "bufParseUtilities.h"
#include "hostGenericFeatures.h"
#include "poddriver.h"
#include "xchanResApi.h"
#include "ip_types.h"
#include <coreUtilityApi.h>
#include <vector>
#include <podimpl.h>
#include <rmf_oobsicache.h>
#include <rmf_oobsimgr.h>

#include <rmf_osal_util.h>
#include <podmgr.h>
#include <rmf_osal_socket.h>
#include "vlpluginapp_halpodapi.h"
#include <semaphore.h>
#include "cardUtils.h"
#include <vlMutex.h>
#ifdef GLI
#include "libIBus.h"
#include "sysMgr.h"
#endif

#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif

#define __MTAG__ VL_CARD_MANAGER

#ifdef __cplusplus
//////////////////////////////////////////////////////////////////
// vlAutoLock : VL_MPEOS_AUTO_LOCK() implementation class
// (To avoid incorrect usage, do not use by itself)
// {use with VL_MPEOS_AUTO_LOCK() macro only}
//////////////////////////////////////////////////////////////////
class vlMpeosAutoLock
{
    rmf_osal_Mutex & m_rLock;
public:
    vlMpeosAutoLock(rmf_osal_Mutex & _rLock);
    ~vlMpeosAutoLock();
};

#define VL_MPEOS_AUTO_LOCK(lock)  vlMpeosAutoLock  _mpeos_Lock_##lock##_(lock);
//////////////////////////////////////////////////////////////////

#endif // __cplusplus


vlMpeosAutoLock::vlMpeosAutoLock(rmf_osal_Mutex & _rLock)
    : m_rLock(_rLock)
{
    rmf_osal_mutexAcquire(m_rLock);
}

vlMpeosAutoLock::~vlMpeosAutoLock()
{
    rmf_osal_mutexRelease(m_rLock);
}


extern "C" int vlCaGetMaxProgramsSupportedByHost();
extern "C" int vlGetCciCAenableFlagForPrgIndex(unsigned char PrgIndex, unsigned char Ltsid,unsigned short PrgNum, unsigned char *pCci,unsigned char *pCAEnable);
//extern "C" rmf_Error ChanMapReset(void);

static CardManager        * vlg_pCardManager     = NULL;
static cableCardDriver * vlg_pCableCardDevice = NULL;
static cCardManagerIF     * vlg_pCardManagerIf   = NULL;
#ifdef OOB_TUNE
static pfcOOBTunerDriver  * vlg_pOobTunerDevice  = NULL;
#endif
static rmf_PODDatabase    * vlg_podDB            = NULL;

static rmf_osal_eventqueue_handle_t       * vlg_RawApduEventQueue= NULL;

static rmf_osal_SocketHostEntry     *vlg_hostent   = NULL;
#define SAS_TESTING
#ifdef SAS_TESTING
static sem_t                    vlg_SASConnectEvent;
unsigned long                vlg_SASSessionId;
#endif
static rmf_osal_Bool podWasAlreadyReady=FALSE;
static unsigned char    vlg_CASesOpen = 0;
static unsigned char    vlg_CardResResourceOpen = 0;
struct VL_MPEOS_EVENT_REG_DATA
{
    VL_MPEOS_EVENT_REG_DATA();
    VL_MPEOS_EVENT_REG_DATA(rmf_osal_eventqueue_handle_t queueId, void * act);
    rmf_osal_eventqueue_handle_t m_queueId;
    void * m_act;
    bool operator == (const VL_MPEOS_EVENT_REG_DATA & rRight) const;
};

VL_MPEOS_EVENT_REG_DATA::VL_MPEOS_EVENT_REG_DATA()
{
    memset(this,0,sizeof(*this));
}

VL_MPEOS_EVENT_REG_DATA::VL_MPEOS_EVENT_REG_DATA(rmf_osal_eventqueue_handle_t queueId, void * act)
{
    memset(this,0,sizeof(*this));
    m_queueId = queueId;
    m_act     = act;
}

bool VL_MPEOS_EVENT_REG_DATA::operator == (const VL_MPEOS_EVENT_REG_DATA & rRight) const
{
    if((0     == memcmp(&m_queueId, &(rRight.m_queueId), sizeof(m_queueId))) &&
       (m_act == rRight.m_act))
    {
        return true;
    }
    return false;
}

static vector<VL_MPEOS_EVENT_REG_DATA> vlg_podRegistrations;
static vector<VL_MPEOS_EVENT_REG_DATA> vlg_CableCardRegistrations;

static void vl_cable_card_test_thread();

extern "C" void vlMpeosNotifyVCTIDChange()
{
    rmf_OobSiMgr *oobSi = rmf_OobSiMgr::getInstance();
    oobSi->notify_VCTID_change();
}

extern "C" void vlMpeosClearSiDatabase()
{
    //Aug-25-2010: Made it conditional due to stability issues
    if(vl_env_get_bool(false, "FEATURE.CLEAR_CHANNEL_MAP_ON_POD_REMOVED"))
    {  
        rmf_OobSiCache *oobSi = rmf_OobSiCache::getSiCache();
        RDK_LOG(RDK_LOG_WARN, "LOG.RDK.POD","%s: SI Channel Map is being cleared.\n", __FUNCTION__);
        oobSi->ChanMapReset();
    }
}

extern "C" void vlMpeosSIPurgeRDDTables()
{
    rmf_OobSiMgr *oobSi = rmf_OobSiMgr::getInstance();
    RDK_LOG(RDK_LOG_WARN, "LOG.RDK.POD","%s: SI Channel Map is being cleared.\n", __FUNCTION__);
    oobSi->rmf_si_purge_RDD_tables();
}

rmf_Error vlMpeosCardManagerInit()
{
    rmf_Error result = RMF_SUCCESS;
    static int bInitialized = 0;
    if(0 == bInitialized)
    {
        bInitialized = 1;
        if(NULL == vlg_pCableCardDevice)
        {
            vlg_pCableCardDevice = new CHALPod;
            //VLALLOC_INFO2(vlg_pCableCardDevice, sizeof(CHALPod));
            if(NULL != vlg_pCableCardDevice)
            {
                vlg_pCableCardDevice->initialize();

		  set_cablecard_device( vlg_pCableCardDevice );
            }
        }
        
        if(NULL == vlg_pCableCardDevice)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","%s: CHALPod alloc failed\n", __FUNCTION__);
            return RMF_OSAL_ENOMEM;
        }

#ifdef OOB_TUNE
        if(NULL == vlg_pOobTunerDevice)
        {
            vlg_pOobTunerDevice = new cHALOOBTune;
            //VLALLOC_INFO2(vlg_pOobTunerDevice, sizeof(cHALOOBTune));
            if(NULL != vlg_pOobTunerDevice)
            {
                vlg_pOobTunerDevice->initialize();
                vlMpeosCoreSetOobTunerDevice(vlg_pOobTunerDevice);
            }
        }
        
        if(NULL == vlg_pOobTunerDevice)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","%s: cHALOOBTune alloc failed\n", __FUNCTION__);
            return RMF_OSAL_ENOMEM;
        }
#endif        
        if(NULL == vlg_pCardManager)
        {
            vlg_pCardManager = new CardManager;
            //VLALLOC_INFO2(vlg_pCardManager, sizeof(CardManager));
            if(NULL != vlg_pCardManager)
            {
                vlg_pCardManager->initialize();
                vlg_pCardManager->init_complete();
                vlg_pCardManagerIf = cCardManagerIF::getInstance(/*pfc_kernel_global*/);
//                pfc_kernel_global->set_resource(vlg_pCardManager);
            }
        }
        
        if(NULL == vlg_pCardManager)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","%s: CardManager alloc failed\n", __FUNCTION__);
            return RMF_OSAL_ENOMEM;
        }


#ifdef SAS_TESTING
        //
        sem_init( &vlg_SASConnectEvent, 0 , 0);
#endif

        if(NULL == vlg_RawApduEventQueue)
        {
            rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(rmf_osal_eventqueue_handle_t),
				                                                     (void **)&vlg_RawApduEventQueue);
            rmf_osal_eventmanager_handle_t  em = get_pod_event_manager();
//           if(NULL != em)
//            {
            rmf_osal_eventqueue_create ( (const uint8_t* )"vlg_RawApduEventQueue" , vlg_RawApduEventQueue);                
            if(NULL != vlg_RawApduEventQueue)
            {
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","%s: Register for RAW APDU Event with Cablecard Manager\n", __FUNCTION__);
                rmf_osal_eventmanager_register_handler( em, *vlg_RawApduEventQueue,
	                                                           RMF_OSAL_EVENT_CATEGORY_CM_RAW_APDU);
            }
//            }
        }
    
        VL_HOST_IP_INFO hostIpInfo;
        vlXchanGetDsgIpInfo(&hostIpInfo);
        rmf_osal_envSet("MPE.SYS.RFMACADDR", hostIpInfo.szMacAddress);
        
        
    }
    
    return result;
}




rmf_Error vlMpeosPodInit(rmf_PODDatabase *podDB)
{
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","Calling vlMpeosSysInit()\n");
    vlg_podDB = podDB;
    int nRet;
    rmf_Error result = RMF_SUCCESS;

        
    if(NULL != vlg_podDB)
    {
        rmf_PODDatabase & podDB = *vlg_podDB;
        rmf_osal_Mutex & podLock = podDB.pod_mutex;
        VL_MPEOS_AUTO_LOCK(podLock);

        podDB.pod_isPresent = FALSE;
        podDB.pod_isReady   = FALSE;
        podDB.pod_appInfo   = NULL;
        podDB.pod_features  = NULL;
        
	memset(&(podDB.pod_featureParams),0,sizeof(podDB.pod_featureParams));
        //auto_unlock(&podLock);
		
    }
    
    static int bInitialized = 0;
    if(0 == bInitialized)
    {

        bInitialized = 1;
#if 0
#ifdef USE_POD		
        if(0 != (nRet = CHALPod_init()))
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","CHALPod_init() returned %d\n", nRet);
            return RMF_OSAL_ENODATA;
        }
#endif		
#endif 
        if(RMF_SUCCESS != (result = vlMpeosSysInit()))
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlMpeosSysInit() returned %d\n", result);
            return RMF_OSAL_ENODATA;
        }
     
        if(RMF_SUCCESS  != (result = vlMpeosCardManagerInit()))
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlMpeosCardManagerInit() returned %d\n", result);
            return RMF_OSAL_ENODATA;
        }


    }
    
    #if 0
    if(NULL != vlg_podDB)
    {
        rmf_PODDatabase & podDB = *vlg_podDB;
        rmf_osal_Mutex & podLock = podDB.pod_mutex;
        VL_MPEOS_AUTO_LOCK(podLock);

        podDB.pod_isPresent = FALSE;
        podDB.pod_isReady   = FALSE;
        podDB.pod_appInfo   = NULL;
        podDB.pod_features  = NULL;
        
        VL_ZERO_MEMORY(podDB.pod_featureParams);
    }
    #endif
    return RMF_SUCCESS;
}




rmf_Error vlMpeosSetCardPresent(int bIsPresent)
{
    if(NULL != vlg_podDB)
    {
        rmf_PODDatabase & podDB = *vlg_podDB;
        rmf_osal_Mutex & podLock = podDB.pod_mutex;
        VL_MPEOS_AUTO_LOCK(podLock);

        podDB.pod_isPresent = bIsPresent;
        if(bIsPresent)
        {
            vlMpeosPostPodEvent(RMF_PODSRV_EVENT_INSERTED, 0, 0);
        }
        else
        {
            vlMpeosPostPodEvent(RMF_PODSRV_EVENT_REMOVED, 0, 0);
            podWasAlreadyReady = TRUE;	    
            vlMpeosClearSiDatabase();
        }
        //auto_unlock(&podLock);

    }
    
    return RMF_SUCCESS;
}

rmf_Error vlMpeosSetCardReady(int bIsReady)
{
  
    if(NULL != vlg_podDB)
    {
        rmf_PODDatabase & podDB = *vlg_podDB;
        rmf_osal_Mutex & podLock = podDB.pod_mutex;
        VL_MPEOS_AUTO_LOCK(podLock);
        
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","vlMpeosSetCardReady: podDB.pod_isReady %d\n ", podDB.pod_isReady);
        if(bIsReady && (!podDB.pod_isReady))
        {
            if(podWasAlreadyReady)
	    	{
	      		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlMpeosSetCardReady: Post the RMF_POD_EVENT_RESET_PENDING event\n ");
	      		vlMpeosPostPodEvent(RMF_PODSRV_EVENT_RESET_PENDING, 0, 0);
	    	}
	    	podWasAlreadyReady = FALSE;
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlMpeosSetCardReady: Post the RMF_PODSRV_EVENT_READY event\n ");
            podDB.pod_isReady = bIsReady;
            vlMpeosPostPodEvent(RMF_PODSRV_EVENT_READY, 0, 0);
#ifdef GLI
            IARM_Bus_SYSMgr_EventData_t eventData;
            eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_CABLE_CARD;
            eventData.data.systemStates.state = 1;
            eventData.data.systemStates.error = 0;
            IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
#endif			
        }
        podDB.pod_isReady = bIsReady;
        //auto_unlock(&podLock);

    }
    else
    {
      RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlMpeosSetCardReady: vlg_podDB is NULL\n ");
    }
    
    return RMF_SUCCESS;
}


rmf_Error vlMpeosPostCCIChangeEvent(VlCardManagerCCIData_t *pInCci)
{

       vlMpeosPostPodEvent(RMF_PODSRV_EVENT_CCI_CHANGED, (void*)pInCci, 0);
	   return RMF_SUCCESS;
}

void postDSGIPEvent(uint8_t *ip)
{	
	vlMpeosPostPodEvent(RMF_PODSRV_EVENT_DSGIP_ACQUIRED, (void*)ip, 0);
}

rmf_Error vlMpeosPostCASystemEvent(int param)
{
    if(NULL != vlg_podDB)
    {
        rmf_PODDatabase & podDB = *vlg_podDB;
        rmf_osal_Mutex & podLock = podDB.pod_mutex;
        VL_MPEOS_AUTO_LOCK(podLock);
        
      if(param == 0)
      {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","GOT CARDRES OPEN\n ");
        vlg_CardResResourceOpen = 1;
      }
      else if(param == 1)
      {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","GOT CA RES OPEN\n ");
        vlg_CASesOpen = 1;
      }
      
      if(vlg_CardResResourceOpen && vlg_CASesOpen)
      {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: Post the RMF_PODSRV_EVENT_CA_SYSTEM_READY\n", __FUNCTION__);
        vlMpeosPostPodEvent(RMF_PODSRV_EVENT_CA_SYSTEM_READY, 0, 0);
      }

      //auto_unlock(&podLock);

  }
  else
  {
      RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlMpeosPostCASystemEvent: vlg_podDB is NULL\n ");
    }
    
    return RMF_SUCCESS;
}


rmf_Error vlMpeosPodRegister(rmf_osal_eventqueue_handle_t queueId, void* act)
{
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: Register Pod message queue.\n", __FUNCTION__);
    if(NULL != vlg_podDB)
    {
        rmf_PODDatabase & podDB = *vlg_podDB;
        rmf_osal_Mutex & podLock = podDB.pod_mutex;
        VL_MPEOS_AUTO_LOCK(podLock);

        VL_MPEOS_EVENT_REG_DATA newReg(queueId, act);
        unsigned int i = 0;
        for(i = 0; i < vlg_podRegistrations.size(); i++)
        {
            if(newReg == vlg_podRegistrations[i])
            {
                //auto_unlock(&podLock);
                return RMF_SUCCESS;
            }
        }
        
        vlg_podRegistrations.push_back(newReg);
/*        
        if(podDB.pod_isReady)
        {
            vlMpeosPostPodEvent(RMF_PODSRV_EVENT_READY, 0);
        }*/

        if(vlg_CardResResourceOpen && vlg_CASesOpen)
        {
          RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: Post the RMF_PODSRV_EVENT_READY\n", __FUNCTION__);
          vlMpeosPostPodEvent(RMF_PODSRV_EVENT_READY, 0, 0);
          RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: Post the RMF_PODSRV_EVENT_CA_SYSTEM_READY\n", __FUNCTION__);
          vlMpeosPostPodEvent(RMF_PODSRV_EVENT_CA_SYSTEM_READY, 0, 0);
        }

        //auto_unlock(&podLock);
		
    }
    
    return RMF_SUCCESS;
}

rmf_Error vlMpeosPodUnregister(void)
{
    if(NULL != vlg_podDB)
    {
        rmf_PODDatabase & podDB = *vlg_podDB;
        rmf_osal_Mutex & podLock = podDB.pod_mutex;
        VL_MPEOS_AUTO_LOCK(podLock);

        vlg_podRegistrations.clear();
        //auto_unlock(&podLock);

    }
    
    return RMF_SUCCESS;
}
rmf_Error vlMpeosPostPodEvent(int ePodEvent, void  *nEventData, uint32_t data_extension)
{

    rmf_Error err = RMF_SUCCESS;
    rmf_osal_event_params_t p_event_params = {
												0, /* Priority */
												nEventData,
												data_extension,
												NULL
	   										};
    rmf_osal_event_handle_t event_handle;

    if(NULL != vlg_podDB)
    {
        rmf_PODDatabase & podDB = *vlg_podDB;
        rmf_osal_Mutex & podLock = podDB.pod_mutex;
        VL_MPEOS_AUTO_LOCK(podLock);

        unsigned int i = 0; 
        for(i = 0; i < vlg_podRegistrations.size(); i++)
        {

		err = rmf_osal_event_create(RMF_OSAL_EVENT_CATEGORY_POD, 
										   ePodEvent, &p_event_params, &event_handle);
		if(err!= RMF_SUCCESS)
		{
			RDK_LOG(
					RDK_LOG_DEBUG,
					"LOG.RDK.POD",
					"Could not create event handle:\n");
		}
		err =  rmf_osal_eventqueue_add(vlg_podRegistrations[i].m_queueId, event_handle);

        }

    }
    
    return err;
}

rmf_Error vlMpeosPostFeatureChangedEvent(int eFeature)
{
    //Feb-24-2010: Commented due to lack of support in RI stack and due to interference with XAIT: //vlMpeosPostPodEvent(RMF_PODSRV_EVENT_GF_UPDATE, eFeature);

    //Apr-21-2010: Enabled the following api back to post the Generic Feature Update. Need to be tested to see whether
    //there are any side effects as specified in the above comment.
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","Sending generic feature to listener.... \n");
    vlMpeosPostPodEvent(RMF_PODSRV_EVENT_GF_UPDATE, (void *)eFeature, 0);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","After Sending generic feature to listener.... \n");
    return RMF_SUCCESS;
}

extern vlCardAppInfo_t CardAppInfo;

rmf_Error vlMpeosPostAppInfoChangedEvent()
{
  rmf_Error result;
  
  result = vlMpeosPodGetAppInfo(vlg_podDB);
  
  RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","vlMpeosPodGetAppInfo returned result: %d\n ", result);
    //if(VL_OS_API_RESULT_SUCCESS == vlMpeosPodGetAppInfo(vlg_podDB))
    if(result == RMF_SUCCESS)
    {
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","Calling vlMpeosSetCardReady \n ");
        vlMpeosSetCardReady(1);
    }
    vlMpeosPostPodEvent(RMF_PODSRV_EVENT_APPINFO_UPDATE, 0, 0);
    
    return result;
}

rmf_Error getCardMfgId( uint16_t *ai_manufacturerId)
{

	if(  NULL == vlg_podDB   )
	{
		return RMF_OSAL_ENODATA;
	}

	rmf_PODDatabase & podDB = *vlg_podDB;
	if( !podDB.pod_isReady )
	{
		return RMF_OSAL_ENODATA;		
	}
	
	rmf_osal_Mutex & podLock = podDB.pod_mutex;
	VL_MPEOS_AUTO_LOCK(podLock);

	rmf_Error ret = vlMpeosPodGetAppInfo( &podDB );		
	if( ( RMF_SUCCESS == ret ) && ( podDB.pod_appInfo != NULL ) )
	{
		*ai_manufacturerId = podDB.pod_appInfo->ai_manufacturerId;
		RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","%s: returning success \n ", __FUNCTION__);		
		return RMF_SUCCESS;
	}
	else
	{
		RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","%s: No data available \n ", __FUNCTION__);
		return RMF_OSAL_ENODATA;
	}
		
	return RMF_OSAL_ENODATA;
}

rmf_Error getCardVersionNo( uint16_t *ai_versionNumber)
{
	if(  NULL == vlg_podDB   )
	{
		return RMF_OSAL_ENODATA;
	}

	rmf_PODDatabase & podDB = *vlg_podDB;
	if( !podDB.pod_isReady )
	{
		return RMF_OSAL_ENODATA;		
	}
		
	rmf_osal_Mutex & podLock = podDB.pod_mutex;
	VL_MPEOS_AUTO_LOCK(podLock);

	rmf_Error ret = vlMpeosPodGetAppInfo( &podDB );		
	if( ( RMF_SUCCESS == ret ) && ( podDB.pod_appInfo != NULL ) )
	{
		*ai_versionNumber = podDB.pod_appInfo->ai_versionNumber;
		RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","%s: returning success \n ", __FUNCTION__);					
		return RMF_SUCCESS;
	}
	else
	{
		RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","%s: No data available \n ", __FUNCTION__);
		return RMF_OSAL_ENODATA;
	}
	
	return RMF_OSAL_ENODATA;
}

rmf_Error vlMpeosPodGetAppInfo(rmf_PODDatabase *podDB)
{
    if(NULL != vlg_podDB)
    {
        rmf_PODDatabase & podDB = *vlg_podDB;
        rmf_osal_Mutex & podLock = podDB.pod_mutex;
        VL_MPEOS_AUTO_LOCK(podLock);

        int noApps = 0;

        noApps = CardAppInfo.CardNumberOfApps;

        if((noApps > 0) && (noApps < 128))
        {
            int i = 0;
            rmf_Error ec;            /* PTV OCM Error condition code. */
            
            /* Scan application information structure (SCTE-28 Table 8.4-L) to determine total size. */
            int size = sizeof(podmgr_AppInfo) - 1;    /* Start with basic fields */
            for (i = 0;(( i < noApps) && ( i < VL_MAX_CC_APPS)) ; ++i)
            {
                /* Add: app type, app version, app name length */
                size += sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint8_t);

                /* Add number of bytes in name string. */
                size += CardAppInfo.Apps[i].AppNameLen;

                /* Add: url string size field + number of bytes in URL string. */
                size += sizeof(uint8_t) + CardAppInfo.Apps[i].AppUrlLen;
            }

            if(NULL == podDB.pod_appInfo)
            {
                if ((ec = rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(podmgr_AppInfo)+4, (void**)&podDB.pod_appInfo)) != RMF_SUCCESS)
                {
                    //auto_unlock(&podLock);
                
                    return RMF_OSAL_ENOMEM;
                }
                if(NULL != podDB.pod_appInfo)
                {
                    podDB.pod_appInfo->ai_apps = NULL;
                }
            }
            else
            {
                if ((ec = rmf_osal_memReallocP(RMF_OSAL_MEM_POD, sizeof(podmgr_AppInfo)+4, (void**)&podDB.pod_appInfo)) != RMF_SUCCESS)
                {
                    //auto_unlock(&podLock);
                    return RMF_OSAL_ENOMEM;
                }
            }

            if(NULL != podDB.pod_appInfo)
            {
                podDB.pod_appInfo->ai_manufacturerId = CardAppInfo.CardManufactureId;
                podDB.pod_appInfo->ai_versionNumber  = CardAppInfo.CardVersion;
                podDB.pod_appInfo->ai_appCount       = CardAppInfo.CardNumberOfApps;
  		  RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","%s: writing CardManufactureId %d\n ", __FUNCTION__, CardAppInfo.CardManufactureId);
  		  RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","%s: writing CardVersion %d\n ", __FUNCTION__, CardAppInfo.CardVersion);
  		  RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","%s: writing CardNumberOfApps %d\n ", __FUNCTION__, CardAppInfo.CardNumberOfApps);		  
		  
                if(NULL == podDB.pod_appInfo->ai_apps)
                {
                    if ((ec = rmf_osal_memAllocP(RMF_OSAL_MEM_POD, size+4, (void**)&podDB.pod_appInfo->ai_apps)) != RMF_SUCCESS)
	                {
	                    //auto_unlock(&podLock);
	                    return RMF_OSAL_ENOMEM;
	                }
                }
                else
                {
                    if ((ec = rmf_osal_memReallocP(RMF_OSAL_MEM_POD, size+4, (void**)&podDB.pod_appInfo->ai_apps)) != RMF_SUCCESS)
	                {
	                    //auto_unlock(&podLock);
	                    return RMF_OSAL_ENOMEM;
	                }
                }
            
                if(NULL != podDB.pod_appInfo->ai_apps)
                {
                    uint8_t * appinfo = (uint8_t*)(podDB.pod_appInfo->ai_apps);
                    VL_BYTE_STREAM_INST(appinfo, WriteBuf, appinfo, size);
                    for (i = 0; ((i < noApps) && (i< VL_MAX_CC_APPS)); ++i)
                    {
                        /* Add: app type, app version, app name length */
                        vlWriteByte  (pWriteBuf, CardAppInfo.Apps[i].AppType);
                        vlWriteShort (pWriteBuf, CardAppInfo.Apps[i].VerNum);
                        vlWriteByte  (pWriteBuf, CardAppInfo.Apps[i].AppNameLen);

                        /* Add number of bytes in name string. */
                        vlWriteBuffer(pWriteBuf, CardAppInfo.Apps[i].AppName, CardAppInfo.Apps[i].AppNameLen, VL_BYTE_STREAM_REMAINDER(pWriteBuf));

                        /* Add: url string size field + number of bytes in URL string. */
                        vlWriteByte (pWriteBuf, CardAppInfo.Apps[i].AppUrlLen);
                        vlWriteBuffer(pWriteBuf, CardAppInfo.Apps[i].AppUrl, CardAppInfo.Apps[i].AppUrlLen, VL_BYTE_STREAM_REMAINDER(pWriteBuf));
                    }
                    //auto_unlock(&podLock);
                    /* Set POD status flag. */
			RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","%s: returning success \n ", __FUNCTION__);
                    return RMF_SUCCESS;
                }
            }
        }
        //auto_unlock(&podLock);
    }
    return RMF_OSAL_ENODATA;
}

rmf_Error vlMpeosPodGetFeatures(rmf_PODDatabase *podDB)
{
    if(NULL != vlg_podDB)
    {
        rmf_PODDatabase & podDB = *vlg_podDB;
        rmf_osal_Mutex & podLock = podDB.pod_mutex;
        VL_MPEOS_AUTO_LOCK(podLock);

        VL_POD_HOST_FEATURE_ID_LIST featureList;
	memset(&featureList,0,sizeof(featureList));
        if(VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS == vlPodGetGenericFeatureList(&featureList))
        {
            int nFeatures = 0;
            nFeatures = featureList.nNumberOfFeatures;
            int i = 0;
            rmf_Error ec;            /* PTV OCM Error condition code. */
            
            /* Scan application information structure (SCTE-28 Table 8.4-L) to determine total size. */
            // not used // int size = sizeof(rmf_PodmgrFeatures) - 1;    /* Start with basic fields */
            // not used // size += nFeatures;

            if(NULL == podDB.pod_features)
            {
                if ((ec = rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(rmf_PodmgrFeatures)+4, (void**)&podDB.pod_features)) != RMF_SUCCESS)
                {
	                    //auto_unlock(&podLock);
	                    return RMF_OSAL_ENOMEM;
                }
                if(NULL != podDB.pod_features)
                {
                    podDB.pod_features->featureIds = NULL;
                }
            }
            else
            {
                if ((ec = rmf_osal_memReallocP(RMF_OSAL_MEM_POD, sizeof(rmf_PodmgrFeatures)+4, (void**)&podDB.pod_features)) != RMF_SUCCESS)
	                {
	                    //auto_unlock(&podLock);
	                    return RMF_OSAL_ENOMEM;
	                }
            }

            if(NULL != podDB.pod_features)
            {
                podDB.pod_features->number = 0;
                
                if(NULL == podDB.pod_features->featureIds)
                {
                    if ((ec = rmf_osal_memAllocP(RMF_OSAL_MEM_POD, nFeatures+4, (void**)&podDB.pod_features->featureIds)) != RMF_SUCCESS)
	                {
	                    //auto_unlock(&podLock);
	                    return RMF_OSAL_ENOMEM;
	                }
                }
                else
                {
                    if ((ec = rmf_osal_memReallocP(RMF_OSAL_MEM_POD, nFeatures+4, (void**)&podDB.pod_features->featureIds)) != RMF_SUCCESS)
	                {
	                    //auto_unlock(&podLock);
	                    return RMF_OSAL_ENOMEM;
	                }
                }
                
                if(NULL != podDB.pod_features->featureIds)
                {
                    podDB.pod_features->number = nFeatures;

                    for (i = 0; i < nFeatures; ++i)
                    {
                        /* Add: feature id */
                        podDB.pod_features->featureIds[i] = featureList.aFeatureId[i];
                    }

                    //auto_unlock(&podLock);
                    return RMF_SUCCESS;
                }
            }
        }
        //auto_unlock(&podLock);
		
    }
    
    return RMF_OSAL_ENODATA;
}

rmf_Error vlMpeosPodRealloc(void ** pAllocLocation, int nAllocSize)
{
    rmf_Error ec;            /* PTV OCM Error condition code. */
    
    if(NULL == *pAllocLocation)
    {
        if ((ec = rmf_osal_memAllocP(RMF_OSAL_MEM_POD, nAllocSize, pAllocLocation)) != RMF_SUCCESS)
            return RMF_OSAL_ENOMEM;
    }
    else
    {
        if ((ec = rmf_osal_memReallocP(RMF_OSAL_MEM_POD, nAllocSize, pAllocLocation)) != RMF_SUCCESS)
            return RMF_OSAL_ENOMEM;
    }
    
    if(NULL == *pAllocLocation) return RMF_OSAL_ENOMEM;
    
    return RMF_SUCCESS;
}


rmf_Error vlMpeosPodGetFeatureParam(rmf_PODDatabase *podDBPtr, uint32_t featureId)
{
    if(NULL != vlg_podDB)
    {
        rmf_PODDatabase & podDB = *vlg_podDB;
        rmf_osal_Mutex & podLock = podDB.pod_mutex;
        VL_MPEOS_AUTO_LOCK(podLock);
        
        int i = 0;

        switch(featureId)
        {
            case VL_HOST_FEATURE_PARAM_RF_Output_Channel        :
            {
                VL_POD_HOST_FEATURE_RF_CHANNEL Struct;
		memset(&Struct,0,sizeof(Struct));
                vlPodGetGenericFeatureData((VL_HOST_FEATURE_PARAM)(featureId), &Struct);
                podDB.pod_featureParams.rf_output_channel[0] = Struct.nRfOutputChannel;
                podDB.pod_featureParams.rf_output_channel[1] = Struct.bEnableRfOutputChannelUI;
            }
            break;

            case VL_HOST_FEATURE_PARAM_Parental_Control_PIN     :
            {
                VL_POD_HOST_FEATURE_PARENTAL_CONTROL_PIN Struct;
		memset(&Struct,0,sizeof(Struct));
                vlPodGetGenericFeatureData((VL_HOST_FEATURE_PARAM)(featureId), &Struct);
                
                int nAllocSize = Struct.nParentalControlPinSize+4;
                void ** pAllocLocation = (void**)&(podDB.pod_featureParams.pc_pin);
                
                if(RMF_SUCCESS != vlMpeosPodRealloc(pAllocLocation, nAllocSize))
                {
                    //auto_unlock(&podLock);                
                    return RMF_OSAL_ENOMEM;
                }
                
                uint8_t * pBytArray = (uint8_t*)(*pAllocLocation);
                VL_BYTE_STREAM_INST(pBytArray, WriteBuf, pBytArray, nAllocSize);

                vlWriteByte  (pWriteBuf, Struct.nParentalControlPinSize);
                vlWriteBuffer(pWriteBuf, Struct.aParentalControlPin, Struct.nParentalControlPinSize, VL_BYTE_STREAM_REMAINDER(pWriteBuf));
            }
            break;

            case VL_HOST_FEATURE_PARAM_Parental_Control_Settings:
            {
                VL_POD_HOST_FEATURE_PARENTAL_CONTROL_SETTINGS Struct;
		memset(&Struct,0,sizeof(Struct));
                vlPodGetGenericFeatureData((VL_HOST_FEATURE_PARAM)(featureId), &Struct);
                
                int nAllocSize = Struct.nParentalControlChannels*3+4;
                void ** pAllocLocation = (void**)&(podDB.pod_featureParams.pc_setting);
                
                if(RMF_SUCCESS != vlMpeosPodRealloc(pAllocLocation, nAllocSize))
                {
                    //auto_unlock(&podLock);                
                    return RMF_OSAL_ENOMEM;
                }
                
                uint8_t * pBytArray = (uint8_t*)(*pAllocLocation);
                VL_BYTE_STREAM_INST(pBytArray, WriteBuf, pBytArray, nAllocSize);

                vlWriteByte     (pWriteBuf, Struct.ParentalControlReset);
                vlWriteShort    (pWriteBuf, Struct.nParentalControlChannels);

                for(i = 0; i < Struct.nParentalControlChannels; i++)
                {
                    vlWrite3ByteLong(pWriteBuf, Struct.paParentalControlChannels[i]);
                }
            }
            break;

            case VL_HOST_FEATURE_PARAM_IPPV_PIN                 :
            {
                VL_POD_HOST_FEATURE_PURCHASE_PIN Struct;
		memset(&Struct,0,sizeof(Struct));
                vlPodGetGenericFeatureData((VL_HOST_FEATURE_PARAM)(featureId), &Struct);
                
                int nAllocSize = Struct.nPurchasePinSize+4;
                void ** pAllocLocation = (void**)&(podDB.pod_featureParams.ippv_pin);
                
                if(RMF_SUCCESS != vlMpeosPodRealloc(pAllocLocation, nAllocSize))
                {
                    //auto_unlock(&podLock);                
                    return RMF_OSAL_ENOMEM;
                }
                
                uint8_t * pBytArray = (uint8_t*)(*pAllocLocation);
                VL_BYTE_STREAM_INST(pBytArray, WriteBuf, pBytArray, nAllocSize);

                vlWriteByte  (pWriteBuf, Struct.nPurchasePinSize);
                vlWriteBuffer(pWriteBuf, Struct.aPurchasePin, Struct.nPurchasePinSize, VL_BYTE_STREAM_REMAINDER(pWriteBuf));
            }
            break;

            case VL_HOST_FEATURE_PARAM_Time_Zone                :
            {
                VL_POD_HOST_FEATURE_TIME_ZONE_OFFSET Struct;
		memset(&Struct,0,sizeof(Struct));
                vlPodGetGenericFeatureData((VL_HOST_FEATURE_PARAM)(featureId), &Struct);
                podDB.pod_featureParams.time_zone_offset[0] = (Struct.nTimeZoneOffset>>8)&0xFF;
                podDB.pod_featureParams.time_zone_offset[1] = (Struct.nTimeZoneOffset>>0)&0xFF;
            }
            break;

            case VL_HOST_FEATURE_PARAM_Daylight_Savings_Control :
            {
                VL_POD_HOST_FEATURE_DAYLIGHT_SAVINGS Struct;
		memset(&Struct,0,sizeof(Struct));
                vlPodGetGenericFeatureData((VL_HOST_FEATURE_PARAM)(featureId), &Struct);
                VL_BYTE_STREAM_INST(pBytArray, WriteBuf, podDB.pod_featureParams.daylight_savings, sizeof(podDB.pod_featureParams.daylight_savings));
                vlWriteByte  (pWriteBuf, Struct.eDaylightSavingsType);
                if(VL_POD_HOST_DAY_LIGHT_SAVINGS_USE_DLS == Struct.eDaylightSavingsType)
                {
                    vlWriteByte  (pWriteBuf, Struct.nDaylightSavingsDeltaMinutes);
                    vlWriteLong  (pWriteBuf, Struct.tmDaylightSavingsEntryTime);
                    vlWriteLong  (pWriteBuf, Struct.tmDaylightSavingsExitTime);
                }
            }
            break;

            case VL_HOST_FEATURE_PARAM_AC_Outlet                :
            {
                VL_POD_HOST_FEATURE_AC_OUTLET Struct;
		memset(&Struct,0,sizeof(Struct));
                vlPodGetGenericFeatureData((VL_HOST_FEATURE_PARAM)(featureId), &Struct);
                podDB.pod_featureParams.ac_outlet_ctrl = Struct.eAcOutletSetting;
            }
            break;

            case VL_HOST_FEATURE_PARAM_Language                 :
            {
                VL_POD_HOST_FEATURE_LANGUAGE Struct;
		memset(&Struct,0,sizeof(Struct));
                vlPodGetGenericFeatureData((VL_HOST_FEATURE_PARAM)(featureId), &Struct);
                podDB.pod_featureParams.language_ctrl[0] = Struct.szLanguageControl[0];
                podDB.pod_featureParams.language_ctrl[1] = Struct.szLanguageControl[1];
                podDB.pod_featureParams.language_ctrl[2] = Struct.szLanguageControl[2];
            }
            break;

            case VL_HOST_FEATURE_PARAM_Rating_Region            :
            {
                VL_POD_HOST_FEATURE_RATING_REGION Struct;
		memset(&Struct,0,sizeof(Struct));
                vlPodGetGenericFeatureData((VL_HOST_FEATURE_PARAM)(featureId), &Struct);
                podDB.pod_featureParams.ratings_region = Struct.eRatingRegion;
            }
            break;

            case VL_HOST_FEATURE_PARAM_Reset_PINS               :
            {
                VL_POD_HOST_FEATURE_RESET_PINS Struct;
		memset(&Struct,0,sizeof(Struct));
                vlPodGetGenericFeatureData((VL_HOST_FEATURE_PARAM)(featureId), &Struct);
                podDB.pod_featureParams.reset_pin_ctrl = Struct.eResetPins;
            }
            break;

            case VL_HOST_FEATURE_PARAM_Cable_URLS               :
            {
                VL_POD_HOST_FEATURE_CABLE_URLS Struct;
		memset(&Struct,0,sizeof(Struct));
                vlPodGetGenericFeatureData((VL_HOST_FEATURE_PARAM)(featureId), &Struct);
                
                int nAllocSize = 1; // nUrls
                void ** pAllocLocation = (void**)&(podDB.pod_featureParams.cable_urls);
                
                for(i = 0; i < Struct.nUrls; i++)
                {
                    nAllocSize += 1;  // eUrlType
                    nAllocSize += 1;  // nUrlLength
                    nAllocSize += Struct.paUrls[i].nUrlLength;  // szUrl
                }
                
                if(RMF_SUCCESS != vlMpeosPodRealloc(pAllocLocation, nAllocSize+4))
                {
                    //auto_unlock(&podLock);
                    return RMF_OSAL_ENOMEM;
                }
                
                uint8_t * pBytArray = (uint8_t*)(*pAllocLocation);
                VL_BYTE_STREAM_INST(pBytArray, WriteBuf, pBytArray, nAllocSize);

                podDB.pod_featureParams.cable_urls_length = nAllocSize;
                
                vlWriteByte     (pWriteBuf, Struct. nUrls);
                for(i = 0; i < Struct.nUrls; i++)
                {
                    vlWriteByte  (pWriteBuf, Struct.paUrls[i].eUrlType  );
                    vlWriteByte  (pWriteBuf, Struct.paUrls[i].nUrlLength);
                    vlWriteBuffer(pWriteBuf, Struct.paUrls[i].szUrl, Struct.paUrls[i].nUrlLength, VL_BYTE_STREAM_REMAINDER(pWriteBuf));
                }
            }
            break;

            case VL_HOST_FEATURE_PARAM_EAS_location_code        :
            {
                VL_POD_HOST_FEATURE_EAS Struct;
		memset(&Struct,0,sizeof(Struct));
                vlPodGetGenericFeatureData((VL_HOST_FEATURE_PARAM)(featureId), &Struct);
                podDB.pod_featureParams.ea_location[0] = Struct.EMStateCodel;
                podDB.pod_featureParams.ea_location[1] = ((Struct.EMCSubdivisionCodel<<4)&0xF0) | (0xC) | ((Struct.EMCountyCode>>8)&0x3);
                podDB.pod_featureParams.ea_location[2] = (Struct.EMCountyCode&0xFF);
            }
            break;

            case VL_HOST_FEATURE_PARAM_VCT_ID:
            {
                VL_POD_HOST_FEATURE_VCT_ID Struct;
		memset(&Struct,0,sizeof(Struct));
                vlPodGetGenericFeatureData((VL_HOST_FEATURE_PARAM)(featureId), &Struct);
                podDB.pod_featureParams.vct_id[0] = ((Struct.nVctId >> 8)&0xFF);
                podDB.pod_featureParams.vct_id[1] = ((Struct.nVctId >> 0)&0xFF);
            }
            break;

            case VL_HOST_FEATURE_PARAM_Turn_on_Channel          :
            {
                VL_POD_HOST_FEATURE_TURN_ON_CHANNEL Struct;
		memset(&Struct,0,sizeof(Struct));
                vlPodGetGenericFeatureData((VL_HOST_FEATURE_PARAM)(featureId), &Struct);
                podDB.pod_featureParams.turn_on_channel[0] = (0xE0 | (Struct.bTurnOnChannelDefined?0x10:0) | ((Struct.iTurnOnChannel>>8)&0xF));
                podDB.pod_featureParams.turn_on_channel[1] = (Struct.iTurnOnChannel&0x8);
            }
            break;

            case VL_HOST_FEATURE_PARAM_Terminal_Association     :
            {
                VL_POD_HOST_FEATURE_TERMINAL_ASSOCIATION Struct;
		memset(&Struct,0,sizeof(Struct));
                vlPodGetGenericFeatureData((VL_HOST_FEATURE_PARAM)(featureId), &Struct);
                
                int nAllocSize = Struct.nTerminalAssociationLength+4;
                void ** pAllocLocation = (void**)&(podDB.pod_featureParams.term_assoc);
                
                if(RMF_SUCCESS != vlMpeosPodRealloc(pAllocLocation, nAllocSize))
                {
                    //auto_unlock(&podLock);
                    return RMF_OSAL_ENOMEM;
                }

                podDB.pod_featureParams.term_assoc[0] = ((Struct.nTerminalAssociationLength>>8) & 0xFF);
                podDB.pod_featureParams.term_assoc[1] = ((Struct.nTerminalAssociationLength>>0) & 0xFF);
                memcpy(podDB.pod_featureParams.term_assoc+2, Struct.szTerminalAssociationName, Struct.nTerminalAssociationLength);
                
                int iSafeIndex = Struct.nTerminalAssociationLength;
                Struct.szTerminalAssociationName[iSafeIndex] = '\0';
            }
            break;

            case VL_HOST_FEATURE_PARAM_Common_Download_Group_Id:
            {
                VL_POD_HOST_FEATURE_CDL_GROUP_ID Struct;
		memset(&Struct,0,sizeof(Struct));
                vlPodGetGenericFeatureData((VL_HOST_FEATURE_PARAM)(featureId), &Struct);
                podDB.pod_featureParams.cdl_group_id[0] = ((Struct.common_download_group_id >> 8)&0xFF);
                podDB.pod_featureParams.cdl_group_id[1] = ((Struct.common_download_group_id >> 0)&0xFF);
            }
            break;

            case VL_HOST_FEATURE_PARAM_Zip_Code                 :
            {
                VL_POD_HOST_FEATURE_ZIP_CODE Struct;
		memset(&Struct,0,sizeof(Struct));
                vlPodGetGenericFeatureData((VL_HOST_FEATURE_PARAM)(featureId), &Struct);
                
                int nAllocSize = Struct.nZipCodeLength+4;
                void ** pAllocLocation = (void**)&(podDB.pod_featureParams.zip_code);
                
                if(RMF_SUCCESS != vlMpeosPodRealloc(pAllocLocation, nAllocSize))
                {
                    //auto_unlock(&podLock);
                    return RMF_OSAL_ENOMEM;
                }

                podDB.pod_featureParams.zip_code[0] = ((Struct.nZipCodeLength>>8) & 0xFF);
                podDB.pod_featureParams.zip_code[1] = ((Struct.nZipCodeLength>>0) & 0xFF);
                rmf_osal_memcpy(podDB.pod_featureParams.zip_code+2, Struct.szZipCode, Struct.nZipCodeLength,
							nAllocSize-2, VL_HOST_FEATURE_MAX_STR_SIZE );
                
                int iSafeIndex = Struct.nZipCodeLength;
                Struct.szZipCode[iSafeIndex] = '\0';
            }
            break;
        }
        //auto_unlock(&podLock);
        return RMF_SUCCESS;
    }
    
    return RMF_OSAL_ENODATA;
}


rmf_Error vlMpeosPodSetFeatureParam(uint32_t featureId, uint8_t *param, uint32_t size)
{
    if(NULL != vlg_podDB)
    {
        rmf_PODDatabase & podDB = *vlg_podDB;
        rmf_osal_Mutex & podLock = podDB.pod_mutex;
        VL_MPEOS_AUTO_LOCK(podLock);
        
        int i = 0;

        unsigned char * pBytArray = param;
        if(pBytArray == NULL)
        {
          //auto_unlock(&podLock);
          return RMF_OSAL_EINVAL;
        }

        switch(featureId)
        {
            case VL_HOST_FEATURE_PARAM_RF_Output_Channel        :
            {
                VL_POD_HOST_FEATURE_RF_CHANNEL Struct;
		memset(&Struct,0,sizeof(Struct));
                if(size == 2)
                {
                  Struct.nRfOutputChannel         = pBytArray[0];
                  Struct.bEnableRfOutputChannelUI = pBytArray[1];
                }

                vlPodSetGenericFeatureData((VL_HOST_FEATURE_PARAM)(featureId), &Struct, false);
            }
            break;

            case VL_HOST_FEATURE_PARAM_Parental_Control_PIN     :
            {
                VL_POD_HOST_FEATURE_PARENTAL_CONTROL_PIN Struct;
		memset(&Struct,0,sizeof(Struct));
                Struct.nParentalControlPinSize = pBytArray[0];
                rmf_osal_memcpy(Struct.aParentalControlPin, pBytArray+1, Struct.nParentalControlPinSize,
					VL_HOST_FEATURE_MAX_PIN_SIZE, Struct.nParentalControlPinSize );
                    
                vlPodSetGenericFeatureData((VL_HOST_FEATURE_PARAM)(featureId), &Struct, false);
            }
            break;

            case VL_HOST_FEATURE_PARAM_Parental_Control_Settings:
            {
                VL_POD_HOST_FEATURE_PARENTAL_CONTROL_SETTINGS Struct;
		memset(&Struct,0,sizeof(Struct));
                int nAllocSize = 3;
                VL_BYTE_STREAM_INST(pBytArray, TmpParseBuf, pBytArray, nAllocSize);
                Struct.ParentalControlReset     = vlParseByte     (pTmpParseBuf);
                Struct.nParentalControlChannels = vlParseShort    (pTmpParseBuf);
                
                nAllocSize = Struct.nParentalControlChannels*3+4;
                VL_BYTE_STREAM_INST(pBytArray, ParseBuf, pBytArray, nAllocSize);
                
                Struct.ParentalControlReset     = vlParseByte     (pParseBuf);
                Struct.nParentalControlChannels = vlParseShort    (pParseBuf);

                for(i = 0; i < Struct.nParentalControlChannels; i++)
                {
                    Struct.paParentalControlChannels[i] = vlParse3ByteLong(pParseBuf);
                }
                
                vlPodSetGenericFeatureData((VL_HOST_FEATURE_PARAM)(featureId), &Struct, false);
            }
            break;

            case VL_HOST_FEATURE_PARAM_IPPV_PIN                 :
            {
                VL_POD_HOST_FEATURE_PURCHASE_PIN Struct;
		memset(&Struct,0,sizeof(Struct));

                Struct.nPurchasePinSize = pBytArray[0];
                rmf_osal_memcpy(Struct.aPurchasePin, pBytArray+1, Struct.nPurchasePinSize,
					VL_HOST_FEATURE_MAX_PIN_SIZE, Struct.nPurchasePinSize );
                vlPodSetGenericFeatureData((VL_HOST_FEATURE_PARAM)(featureId), &Struct, false);
            }
            break;

            case VL_HOST_FEATURE_PARAM_Time_Zone                :
            {
                VL_POD_HOST_FEATURE_TIME_ZONE_OFFSET Struct;
		memset(&Struct,0,sizeof(Struct));

                Struct.nTimeZoneOffset = (pBytArray[0] << 8) | (pBytArray[1] << 0);
                vlPodSetGenericFeatureData((VL_HOST_FEATURE_PARAM)(featureId), &Struct, false);
            }
            break;

            case VL_HOST_FEATURE_PARAM_Daylight_Savings_Control :
            {
                VL_POD_HOST_FEATURE_DAYLIGHT_SAVINGS Struct;
		memset(&Struct,0,sizeof(Struct));
                VL_BYTE_STREAM_INST(pBytArray, ParseBuf, pBytArray, size);
                Struct.eDaylightSavingsType = (VL_POD_HOST_DAY_LIGHT_SAVINGS)vlParseByte(pParseBuf);
                if(VL_POD_HOST_DAY_LIGHT_SAVINGS_USE_DLS == Struct.eDaylightSavingsType)
                {
                    Struct.nDaylightSavingsDeltaMinutes = vlParseByte(pParseBuf);
                    Struct.tmDaylightSavingsEntryTime   = vlParseLong(pParseBuf);
                    Struct.tmDaylightSavingsExitTime    = vlParseLong(pParseBuf);
                }
                vlPodSetGenericFeatureData((VL_HOST_FEATURE_PARAM)(featureId), &Struct, false);
            }
            break;

            case VL_HOST_FEATURE_PARAM_AC_Outlet                :
            {
                VL_POD_HOST_FEATURE_AC_OUTLET Struct;
		memset(&Struct,0,sizeof(Struct));

                Struct.eAcOutletSetting = (VL_POD_HOST_AC_OUTLET)(pBytArray[0]);
                vlPodSetGenericFeatureData((VL_HOST_FEATURE_PARAM)(featureId), &Struct, false);
            }
            break;

            case VL_HOST_FEATURE_PARAM_Language                 :
            {
                VL_POD_HOST_FEATURE_LANGUAGE Struct;
		memset(&Struct,0,sizeof(Struct));

                Struct.szLanguageControl[0] = pBytArray[0];
                Struct.szLanguageControl[1] = pBytArray[1];
                Struct.szLanguageControl[2] = pBytArray[2];

                vlPodSetGenericFeatureData((VL_HOST_FEATURE_PARAM)(featureId), &Struct, false);
            }
            break;

            case VL_HOST_FEATURE_PARAM_Rating_Region            :
            {
                VL_POD_HOST_FEATURE_RATING_REGION Struct;
		memset(&Struct,0,sizeof(Struct));

                Struct.eRatingRegion = (VL_POD_HOST_RATING_REGION)(pBytArray[0]);
                vlPodSetGenericFeatureData((VL_HOST_FEATURE_PARAM)(featureId), &Struct, false);
            }
            break;

            case VL_HOST_FEATURE_PARAM_Reset_PINS               :
            {
                VL_POD_HOST_FEATURE_RESET_PINS Struct;
		memset(&Struct,0,sizeof(Struct));

                Struct.eResetPins = (VL_POD_HOST_RESET_PIN)(pBytArray[0]);
                vlPodSetGenericFeatureData((VL_HOST_FEATURE_PARAM)(featureId), &Struct, false);
            }
            break;

            case VL_HOST_FEATURE_PARAM_Cable_URLS               :
            {
                VL_POD_HOST_FEATURE_CABLE_URLS Struct;
		memset(&Struct,0,sizeof(Struct));

                {
                    int nAllocSize = pBytArray[0];

                    VL_BYTE_STREAM_INST(pBytArray, ParseBuf, pBytArray, nAllocSize);
                    
                    Struct.nUrls = vlParseByte    (pParseBuf);

                    for(i = 0; i < Struct.nUrls; i++)
                    {
                        Struct.paUrls[i].eUrlType   = (VL_POD_HOST_URL_TYPE) vlParseByte  (pParseBuf);
                        Struct.paUrls[i].nUrlLength = vlParseByte  (pParseBuf);
                        vlParseBuffer(pParseBuf, Struct.paUrls[i].szUrl, Struct.paUrls[i].nUrlLength, sizeof(Struct.paUrls[i].szUrl));
                        int iSafeIndex = SAFE_ARRAY_INDEX_UN(Struct.paUrls[i].nUrlLength, Struct.paUrls[i].szUrl);
                        Struct.paUrls[i].szUrl[iSafeIndex] = '\0';
                    }
                    
                    vlPodSetGenericFeatureData((VL_HOST_FEATURE_PARAM)(featureId), &Struct, false);
                }
            }
            break;

            case VL_HOST_FEATURE_PARAM_EAS_location_code        :
            {
                VL_POD_HOST_FEATURE_EAS Struct;
		memset(&Struct,0,sizeof(Struct));

                Struct.EMStateCodel = pBytArray[0];
                Struct.EMCSubdivisionCodel = ((pBytArray[1]>>4)&0xF);
                Struct.EMCountyCode = ((pBytArray[1]&0x3)<<8) | (pBytArray[2]);

                vlPodSetGenericFeatureData((VL_HOST_FEATURE_PARAM)(featureId), &Struct, false);
            }
            break;

            case VL_HOST_FEATURE_PARAM_VCT_ID:
            {
                VL_POD_HOST_FEATURE_VCT_ID Struct;
		memset(&Struct,0,sizeof(Struct));

                Struct.nVctId = (pBytArray[0] << 8 | pBytArray[1]);

                vlPodSetGenericFeatureData((VL_HOST_FEATURE_PARAM)(featureId), &Struct, false);
            }
            break;

            case VL_HOST_FEATURE_PARAM_Turn_on_Channel          :
            {
                VL_POD_HOST_FEATURE_TURN_ON_CHANNEL Struct;

		memset(&Struct,0,sizeof(Struct));
                Struct.bTurnOnChannelDefined = ((pBytArray[0]>>4) & 0x1);
                Struct.iTurnOnChannel        = (((pBytArray[0]&0xF)<<4) | pBytArray[1]);

                vlPodSetGenericFeatureData((VL_HOST_FEATURE_PARAM)(featureId), &Struct, false);
            }
            break;

            case VL_HOST_FEATURE_PARAM_Terminal_Association     :
            {
                VL_POD_HOST_FEATURE_TERMINAL_ASSOCIATION Struct;
		memset(&Struct,0,sizeof(Struct));

                int nAllocSize = ((pBytArray[0]<<8) | (pBytArray[1]<<0));

                Struct.nTerminalAssociationLength = nAllocSize;
                rmf_osal_memcpy(Struct.szTerminalAssociationName, pBytArray+2, Struct.nTerminalAssociationLength,
					VL_HOST_FEATURE_MAX_STR_SIZE, Struct.nTerminalAssociationLength );
                int iSafeIndex = VL_SAFE_ARRAY_INDEX(Struct.nTerminalAssociationLength+1, Struct.szTerminalAssociationName);
                Struct.szTerminalAssociationName[iSafeIndex] = '\0';
                
                vlPodSetGenericFeatureData((VL_HOST_FEATURE_PARAM)(featureId), &Struct, false);
            }
            break;

            case VL_HOST_FEATURE_PARAM_Common_Download_Group_Id:
            {
                VL_POD_HOST_FEATURE_CDL_GROUP_ID Struct;
		memset(&Struct,0,sizeof(Struct));

                Struct.common_download_group_id = (((pBytArray[0])<<8) | (pBytArray[1]<<0));

                vlPodSetGenericFeatureData((VL_HOST_FEATURE_PARAM)(featureId), &Struct, false);
            }
            break;

            case VL_HOST_FEATURE_PARAM_Zip_Code                 :
            {
                VL_POD_HOST_FEATURE_ZIP_CODE Struct;
		memset(&Struct,0,sizeof(Struct));

                int nAllocSize = ((pBytArray[0]<<8) | (pBytArray[1]<<0));

                Struct.nZipCodeLength = nAllocSize;
                rmf_osal_memcpy(Struct.szZipCode, pBytArray+2, Struct.nZipCodeLength,
							VL_HOST_FEATURE_MAX_STR_SIZE, Struct.nZipCodeLength );
                int iSafeIndex = VL_SAFE_ARRAY_INDEX(Struct.nZipCodeLength+1, Struct.szZipCode);
                Struct.szZipCode[iSafeIndex] = '\0';
                
                vlPodSetGenericFeatureData((VL_HOST_FEATURE_PARAM)(featureId), &Struct, false);
            }
            break;
        }
        //auto_unlock(&podLock);
        return RMF_SUCCESS;
    }
    
    return RMF_OSAL_ENODATA;
}


/**
 * The rmf_podmgrSASConnect() function shall establish a connection between a private
 * Host application and the corresponding POD Module vendor-specific application.
 *
 * @param appID specifies a unique identifier of the private Host application.
 * @param sessionID points to a location where the session ID can be returned. The session
 *          ID is established by the POD to enable further communications.
 * @return Upon successful completion, this function shall return RMF_SUCCESS. Otherwise,
 *          one of the errors below is returned.
 * <ul>
 * <li>     RMF_OSAL_ENOMEM - There was insufficient memory available to complete the operation.
 * <li>     RMF_OSAL_EINVAL - One or more parameters were out of range or unuseable.
 * </ul>
 */

rmf_Error rmf_podmgrSASConnect(uint8_t *appId, uint32_t *sessionId, uint16_t *version)
{
    (void)version;
    if(NULL != vlg_podDB)
    {
        //rmf_PODDatabase & podDB = *vlg_podDB;
        //rmf_osal_Mutex & podLock = podDB.pod_mutex;
        //auto_lock(&podLock);

        if(NULL != vlg_pCardManagerIf)
        {
            if((NULL != appId) && (NULL != sessionId))
            {
                vlg_pCardManagerIf->AssumeOCAPHandlers(true);
        
                int nRet = cPOD_driver::PODSendRawAPDU(-1, SAS_CONNECT_REQ_TAG, appId, 8);
                if(-1 != nRet)
                {
#ifdef SAS_TESTING
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Waiting for the SAS conf and then assign the sessionId\n");
                    //TESTING: Wait for the SAS conf and then assign the sessionId
                    long retValue = sem_wait(& vlg_SASConnectEvent);
/*
                    switch(retValue)
                    {
                      case C_SYS_OBJ_ACQUIRED:
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: C_SYS_OBJ_ACQUIRED\n", __FUNCTION__);
                        break;
                      case C_SYS_OBJ_TIMEDOUT:
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: C_SYS_OBJ_TIMEDOUT\n", __FUNCTION__);
                        break;
                      case C_SYS_OBJ_ERROR:
                        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","%s: C_SYS_OBJ_ERROR\n", __FUNCTION__);
                        break;
                      default:
                        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","%s: UNKNOWN RESULT\n", __FUNCTION__);
                        break;
                    }
*/
                    if(retValue != 0)
                          return RMF_OSAL_EBUSY;	

                    *sessionId = vlg_SASSessionId;
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: SessionId = 0x%x\n", __FUNCTION__, *sessionId);
#else
                    *sessionId = (uint32_t)(nRet);
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: SessionId = 0x%x\n", __FUNCTION__, *sessionId);
#endif //#ifdef SAS_TESTING
                    return RMF_SUCCESS;
                }
            }
        }
    }
    
    return RMF_OSAL_ENODATA;	
}

/**
 * The rmf_podmgrSASClose() function provides an optional API for systems that may
 * require additional work to maintain session resources when an application unregisters
 * its handler from an SAS application.  The implementation of this function may
 * need to: 1) update internal implementation resouces, 2) make an OS API call to
 * allow the OS to update session related resources or 3) do nothing since it's
 * entirely possible that the sessions can be maintained as "connected" upon
 * deregistration and simply reused if the same host or another host application makes a
 * later request to connect to the SAS application.
 *
 * @param sessionId the session identifier of the target SAS session (i.e. the Ocm_Sas_Handle).
 *
 * @return Upon successful completion, this function shall return RMF_SUCCESS. Otherwise,
 *          one of the errors below is returned.
 * <ul>
 * <li>     RMF_OSAL_EINVAL - One or more parameters were out of range or unuseable.
 * </ul>
 */

rmf_Error rmf_podmgrSASClose(uint32_t sessionId)
{
    if(NULL != vlg_podDB)
    {
        rmf_PODDatabase & podDB = *vlg_podDB;
        rmf_osal_Mutex & podLock = podDB.pod_mutex;
        VL_MPEOS_AUTO_LOCK(podLock);
    }
    
    return RMF_SUCCESS;
}


/**
 * The rmf_podmgrMMIConnect() function shall establish a connection with the MMI
 * resource on the POD device.
 *
 * @return Upon successful completion, this function shall return RMF_SUCCESS. Otherwise,
 *          one of the errors below is returned.
 * <ul>
 * <li>     RMF_OSAL_ENOMEM - There was insufficient memory available to complete the operation.
 * <li>     RMF_OSAL_EINVAL - One or more parameters were out of range or unuseable.
 * </ul>
 */
//rmf_Error vlMpeosPodMMIConnect(void)
rmf_Error rmf_podmgrMMIConnect(uint32_t *sessionId, uint16_t *version)
{
    RMF_OSAL_UNUSED_PARAM(version);
    pod_session_id sess = NULL;

    if(NULL != vlg_podDB)
    {
        rmf_PODDatabase & podDB = *vlg_podDB;
        rmf_osal_Mutex & podLock = podDB.pod_mutex;
        VL_MPEOS_AUTO_LOCK(podLock);

    }
    
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD",
            "%s: MMI session successfully opened. ID = %x!\n", __FUNCTION__,
            (uint32_t)sess);

    return RMF_SUCCESS;
}


/**
 * The rmf_podmgrMMIClose() function provides an optional API for systems that may
 * require additional work to maintain MMI resources when an application unregisters
 * its MMI handler from the MMI POD application.  The implementation of this function may
 * need to: 1) update internal implementation resouces, 2) make an OS API call to
 * allow the OS to update MMI session related resources or 3) do nothing since it's
 * entirely possible that the MMI sessions can be maintained as "connected" upon
 * deregistration.
 *
 * @return Upon successful completion, this function shall return RMF_SUCCESS. Otherwise,
 *          one of the errors below is returned.
 * <ul>
 * <li>     RMF_OSAL_EINVAL - One or more parameters were out of range or unuseable.
 * </ul>
 */

rmf_Error rmf_podmgrMMIClose(void)
{
    if(NULL != vlg_podDB)
    {
        rmf_PODDatabase & podDB = *vlg_podDB;
        rmf_osal_Mutex & podLock = podDB.pod_mutex;
        VL_MPEOS_AUTO_LOCK(podLock);

    }
    
    return RMF_SUCCESS;
}

/**
 * The rmf_podmgrCASConnect() function shall establish a connection between a private
 * Host application and the POD Conditional Access Support (CAS) resource.  It is the MPEOS call's responsibility
 * determine the correct resource ID based upon whether the card is M or S mode.
 *
 * @param sessionId points to a location where the session ID can be returned. The session
 *          ID is implementation dependent and represents the CAS session to the POD application.
 *
 * @return Upon successful completion, this function shall return RMF_SUCCESS. Otherwise,
 *          one of the errors below is returned.
 * <ul>
 * <li>     RMF_OSAL_ENOMEM - There was insufficient memory available to complete the operation.
 * <li>     RMF_OSAL_EINVAL - One or more parameters were out of range or unusable.
 * </ul>
 */

rmf_Error rmf_podmgrCASConnect(uint32_t * sessionId, uint16_t *version)
{
    (void)version;
    if(NULL != vlg_podDB)
    {
        int iRet;
        unsigned short SesId;

        rmf_PODDatabase & podDB = *vlg_podDB;
        rmf_osal_Mutex & podLock = podDB.pod_mutex;
        VL_MPEOS_AUTO_LOCK(podLock);
		
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","%s Entered \n",__FUNCTION__);
        if(vl_env_get_bool(false, "FEATURE.USE_RI_CA_IMPL"))
        {
            iRet = cPOD_driver::PODGetCASesId(&SesId );
            if(iRet == 0)
            {
                *sessionId = (uint32_t)SesId;
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: Success to get session ID:%d  :%d >>>>>>>> \n",__FUNCTION__,SesId,*sessionId); 
                    //auto_unlock(&podLock);
                return  RMF_SUCCESS;
				
            }
        }    
         //auto_unlock(&podLock);
    }
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s : returning Failed \n",__FUNCTION__);
    return RMF_OSAL_ENODATA;
}

/**
 * The rmf_podmgrCASClose() function provides an optional API for systems that may
 * require additional work to maintain session resources when an application unregisters
 * its handler from POD.  The implementation of this function may
 * need to: 1) update internal implementation resources, 2) make an OS API call to
 * allow the OS to update session related resources or 3) do nothing since it's
 * entirely possible that the sessions can be maintained as "connected" upon
 * deregistration and simply reused if the same host or another host application makes a
 * later request to connect to the CAS application.
 *
 * @param sessionId the session identifier of the target CAS session.
 *
 * @return Upon successful completion, this function shall return RMF_SUCCESS. Otherwise,
 *          one of the errors below is returned.
 * <ul>
 * <li>     RMF_OSAL_EINVAL - One or more parameters were out of range or unusable.
 * </ul>
 */

rmf_Error rmf_podmgrCASClose(uint32_t sessionId)
{
    if(NULL != vlg_podDB)
    {
        rmf_PODDatabase & podDB = *vlg_podDB;
        rmf_osal_Mutex & podLock = podDB.pod_mutex;
        VL_MPEOS_AUTO_LOCK(podLock);
    }
    
    return RMF_SUCCESS;
}

/**
 * The vlMpeosPodGetParam() function gets a resource parameter from the POD.
 *
 * @param paramId       MPE defined identifier.
 * @param paramValue    pointer to the parameter value.
 *
 * @return Upon successful completion, this function shall return RMF_SUCCESS. Otherwise,
 *          one of the errors below is returned.
 * <ul>
 * <li>     RMF_OSAL_ENOMEM - There was insufficient memory available to complete the operation.
 * <li>     RMF_OSAL_EINVAL - One or more parameters were out of range or unusable.
 * </ul>
 */
rmf_Error vlMpeosPodGetParam(rmf_PODMGRParamId paramId, uint16_t* paramValue)
{
    if(NULL != vlg_podDB)
    {
        unsigned char Param;
        int iRet;
        rmf_PODDatabase & podDB = *vlg_podDB;
        rmf_osal_Mutex & podLock = podDB.pod_mutex;
        VL_MPEOS_AUTO_LOCK(podLock);
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","vlMpeosPodGetParam: Entered >>>>>>>. \n");
        switch(paramId)
        {
            case RMF_PODMGR_PARAM_ID_MAX_NUM_ES:
            {
              RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlMpeosPodGetParam: RMF_PODMGR_PARAM_ID_MAX_NUM_ES >>>>>>>. \n");
              iRet = cPOD_driver::PODGetCardResElmStrInfo(&Param );
              RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlMpeosPodGetParam: num elm strms parm:%d \n",Param);
              if(iRet == 0)
              {
                *paramValue = (uint16_t)Param;
                //auto_unlock(&podLock);
                return RMF_SUCCESS;
              }
            }
            break;
            
            case RMF_PODMGR_PARAM_ID_MAX_NUM_PROGRAMS:
            {
              RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlMpeosPodGetParam: RMF_PODMGR_PARAM_ID_MAX_NUM_PROGRAMS >>>>>>>. \n");
              iRet = cPOD_driver::PODGetCardResPrgInfo(&Param );
              RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlMpeosPodGetParam: max progs parm:%d \n",Param);
              if(iRet == 0)
              {
                *paramValue = (uint16_t)Param;
                //auto_unlock(&podLock);
                return RMF_SUCCESS;
              }
            }
            break;
            
            case RMF_PODMGR_PARAM_ID_MAX_NUM_TS:
            {
              RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlMpeosPodGetParam: RMF_PODMGR_PARAM_ID_MAX_NUM_TS >>>>>>>. \n");
              iRet = cPOD_driver::PODGetCardResTrStrInfo(&Param );
              RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlMpeosPodGetParam: num transport strms parm:%d \n",Param);
              if(iRet == 0)
              {
                *paramValue = (uint16_t)Param;
                //auto_unlock(&podLock);
                return RMF_SUCCESS;
              }
            }
            break;
            
            default:
            break;
        }//switch
        //auto_unlock(&podLock);
        return RMF_OSAL_EINVAL;

    }
    
    return RMF_OSAL_ENODATA;
}

/**
 * The podReceiveAPDU() function retrieves the next available APDU from the POD.
 *
 * @param sessionId is a pointer for returning the associated session Id (i.e. Ocm_Sas_Handle).
 * @param apdu the next available APDU from the POD
 * @param len the length of the APDU in bytes
 * @return Upon successful completion, this function shall return RMF_SUCCESS. Otherwise,
 *          one of the errors below is returned.
 * <ul>
 * <li>     RMF_OSAL_ENOMEM - There was insufficient memory available to complete the operation.
 * <li>     RMF_OSAL_EINVAL - One or more parameters were out of range or unuseable.
 * <li>     RMF_OSAL_ENODATA - Indicates that the APDU received is actually the last APDU that
 *                       failed to be sent.
 * </ul>
 */
rmf_Error vlMpeosPodReceiveAPDU(uint32_t *sessionId, uint8_t **apdu, int32_t *len)
{
    if(NULL != vlg_podDB)
    {
        rmf_PODDatabase & podDB = *vlg_podDB;
        // not used // rmf_osal_Mutex & podLock = podDB.pod_mutex;
        // Mar-11-2010: Commented to avoid deadlock // auto_lock(&podLock);

        if(NULL != vlg_RawApduEventQueue)
        {
            rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
            EventCableCard_RawAPDU  *pEvent_raw;
            rmf_osal_event_handle_t event_handle;
            rmf_osal_event_params_t event_params = {0};
            rmf_osal_event_category_t event_category;
            uint32_t event_type;

            //for RMF_OSAL_EVENT_CATEGORY_CM_RAW_APDU   
            rmf_Error err = rmf_osal_eventqueue_get_next_event( *vlg_RawApduEventQueue,
		&event_handle, &event_category, &event_type, &event_params);
          
            if(RMF_SUCCESS != err) return RMF_OSAL_ENODATA;
            switch (event_category)
            {
                case RMF_OSAL_EVENT_CATEGORY_CM_RAW_APDU:
                {
                
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","vlMpeosPodReceiveAPDU(): event type = 0x%x\n", event_type);
                    pEvent_raw = (EventCableCard_RawAPDU *)event_params.data;
                    if(NULL == pEvent_raw) return RMF_OSAL_ENODATA;

                    if(event_type  == card_Raw_APDU_response_avl)
                    {
                        if(NULL != sessionId)
                        {
#ifdef SAS_TESTING
                if(NULL != pEvent_raw->resposeData)
                {
                  int ii;
                  unsigned char *apduBytes = pEvent_raw->resposeData;
                              RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","RMF_OSAL_EVENT_CATEGORY_CM_RAW_APDU: Dumping the Response data \n");
                  for(ii = 0; ii < pEvent_raw->dataLength; ii++)
                  {
                      RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","%02X ",apduBytes[ii]);
                      if( ( (ii + 1) % 16) == 0)
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","\n");
                  }
                              RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "Dumping data End\n");
                              RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","%s: Sending Response APDU TAG = 0x%02x%02x%02x\n", __FUNCTION__, apduBytes[0], apduBytes[1], apduBytes[2]);
                  if( (apduBytes[0] == 0x9F) && (apduBytes[1] == 0x9A) && (apduBytes[2] == 0x01))//SAS connect CNF
                  {
                    vlg_SASSessionId = pEvent_raw->sesnId;
                    sem_post(&vlg_SASConnectEvent);
                    break;
                  }
                  
                }
#endif
                            *sessionId = pEvent_raw->sesnId;

                            if(NULL != len)
                            {
                                *len = pEvent_raw->dataLength;
                                
                                if(NULL != apdu)
                                {
                                    rmf_Error ec;        /* Error condition code. */
                                    if ((ec = rmf_osal_memAllocP(RMF_OSAL_MEM_POD, *len, (void**)apdu)) != RMF_SUCCESS)
                                    {
                                        // memory leak fix
                                        rmf_osal_event_delete(event_handle);
                                        if((NULL != (*apdu)) && ((*len) > 0))
                                        {
                                            const char *mod = "LOG.RDK.POD";
                                            rdk_LogLevel lvl = RDK_LOG_TRACE1;
                                            if(rdk_dbg_enabled(mod, lvl))
                                            {
                                                RDK_LOG(lvl, mod,"CABLE_CARD_RAW_APDU: vlMpeosPodReceiveAPDU: Received APDU 0x%02X%02X%02X, length = %d.\n", (*apdu)[0], (*apdu)[1], (*apdu)[2], (*len));
                                                vlMpeosDumpBuffer(lvl, mod, (*apdu), (*len));
                                            }
                                        }
                                        return RMF_OSAL_ENOMEM;
                                    }
                                    
                                    if(NULL != *apdu)
                                    {
                                        if( pEvent_raw->resposeData ) 
                                        {
                                        		RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","Sending Response TAG : 0x%02X%02X%02X \n",pEvent_raw->resposeData[0], pEvent_raw->resposeData[1], pEvent_raw->resposeData[2]);
							memcpy(*apdu, pEvent_raw->resposeData, *len);
                                        }
                                        // memory leak fix
                                        rmf_osal_event_delete(event_handle);
                                        return RMF_SUCCESS;
                                    }
                                }
                            }
                        }
                    }                
                }
                break;
                
                default:
                {
                }
                break;
            } // switch

            rmf_osal_event_delete(event_handle);
        }
    }
    
    return RMF_OSAL_ENODATA;
}

/**
 * The rmf_podmgrReleaseAPDU() function
 *
 * This will release an APDU retrieved via podReceiveAPDU()
 *
 * @param apdu the APDU pointer retrieved via podReceiveAPDU()
 * @return Upon successful completion, this function shall return
 * <ul>
 * <li>     RMF_OSAL_EINVAL - The apdu pointer was invalid.
 * <li>     RMF_SUCCESS - The apdu was successfully deallocated.
 * </ul>
 */
 rmf_Error rmf_podmgrReleaseAPDU(uint8_t *apdu)
{
   if(NULL != vlg_podDB)
    {
        //rmf_PODDatabase & podDB = *vlg_podDB;
        //rmf_osal_Mutex & podLock = podDB.pod_mutex;
        //auto_lock(&podLock);

        if(NULL != apdu)
        {
            rmf_osal_memFreeP(RMF_OSAL_MEM_POD, apdu);
        }
    }
    
    return RMF_SUCCESS;
}

/**
 * The rmf_podmgrSendAPDU() function sends an APDU packet.
 *
 * @param sessionId is the native handle for the SAS session for the APDU.
 * @param apduTag APDU identifier
 * @param length is the length of the data buffer portion of the APDU
 * @param apdu is a pointer to the APDU data buffer
  *
 * @return Upon successful completion, this function shall return RMF_SUCCESS. Otherwise,
 *          one of the errors below is returned.
 * <ul>
 * <li>     RMF_OSAL_ENOMEM - There was insufficient memory available to complete the operation.
 * <li>     RMF_OSAL_EINVAL - One or more parameters were out of range or unuseable.
 * </ul>
 */
rmf_Error rmf_podmgrSendAPDU(uint32_t sessionId, uint32_t apduTag, uint32_t length, uint8_t *apdu)
{

    
    if(NULL != vlg_podDB)
    {
        rmf_PODDatabase & podDB = *vlg_podDB;
        rmf_osal_Mutex & podLock = podDB.pod_mutex;
        VL_MPEOS_AUTO_LOCK(podLock);

        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","rmf_podmgrSendAPDU: Entered apduTag:0x%X sessionId:%d\n",apduTag,sessionId);
        if(NULL != vlg_pCardManagerIf)
        {
            vlg_pCardManagerIf->AssumeOCAPHandlers(true);
        if(0x9F8030 == apduTag)
        {// ca_info_inq
          if(vl_env_get_bool(false, "FEATURE.USE_RI_CA_IMPL"))
          {
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","rmf_podmgrSendAPDU:enabling the CA APDU route \n");
        vlg_pCardManagerIf->AssumeCaAPDUHandlers(true);
          }
        }

            if((NULL != apdu) && (length > 0))
            {
                const char *mod = "LOG.RDK.POD";
                rdk_LogLevel lvl = RDK_LOG_TRACE1;
                if(rdk_dbg_enabled(mod, lvl))
                {
                    RDK_LOG(lvl, mod,"CABLE_CARD_RAW_APDU: vlMpeosPodSendAPDU: Sending APDU 0x%06X, length = %d.\n", apduTag, length);
                    vlMpeosDumpBuffer(lvl, mod, apdu, length);
                }
            }
            cPOD_driver::PODSendRawAPDU(sessionId, apduTag, apdu, length);
        }
        //auto_unlock(&podLock);		
    }
    
    return RMF_SUCCESS;
}

/**
 * The vlMpeosPodSendCaPmtAPDU() function sends an APDU packet.
 *
 * @param sessionId is the native handle for the SAS session for the APDU.
 * @param apduTag APDU identifier
 * @param length is the length of the data buffer portion of the APDU
 * @param apdu is a pointer to the APDU data buffer
  *
 * @return Upon successful completion, this function shall return RMF_SUCCESS. Otherwise,
 *          one of the errors below is returned.
 * <ul>
 * <li>     RMF_OSAL_ENOMEM - There was insufficient memory available to complete the operation.
 * <li>     RMF_OSAL_EINVAL - One or more parameters were out of range or unuseable.
 * </ul>
 */
rmf_Error vlMpeosPodSendCaPmtAPDU(uint32_t sessionId, uint32_t apduTag, uint32_t length, uint8_t *apdu/*,uint16_t NumElmStrms,uint32_t *pElmStrmInfo*/)
{
    rmf_Error ret = RMF_SUCCESS;	
    if(NULL != vlg_podDB)
    {
        rmf_PODDatabase & podDB = *vlg_podDB;
        rmf_osal_Mutex & podLock = podDB.pod_mutex;
        VL_MPEOS_AUTO_LOCK(podLock);
    if(vl_isFeatureEnabled((unsigned char*)"FEATURE.USE_RI_CA_IMPL"))
    {
          RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","vlMpeosPodSendCaPmtAPDU:  Entered >>>>>>>>>>>>..apduTag:0x%X.sessionId:%d. \n",apduTag,sessionId);
      if(NULL != vlg_pCardManagerIf)
      {
	   int32_t iRet = 0;	
          vlg_pCardManagerIf->AssumeCaAPDUHandlers(true);
              RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","vlMpeosPodSendCaPmtAPDU: calling PODSendCaPmtAPDU \n");
              iRet = cPOD_driver::PODSendCaPmtAPDU(sessionId, apduTag, apdu, length/*,NumElmStrms,pElmStrmInfo*/);
              if( iRet != 0 )
              {
			ret = RMF_OSAL_EBUSY;
		}
      }
    }
    //auto_unlock(&podLock);    
    }
    
    return ret;
}

rmf_Error vlMpeosPodCableCardRegister(rmf_osal_eventqueue_handle_t queueId, void *act)
{
    if(NULL != vlg_podDB)
    {
        rmf_PODDatabase & podDB = *vlg_podDB;
        rmf_osal_Mutex & podLock = podDB.pod_mutex;
        VL_MPEOS_AUTO_LOCK(podLock);

        VL_MPEOS_EVENT_REG_DATA newReg(queueId, act);
        unsigned int i = 0;
        for(i = 0; i < vlg_CableCardRegistrations.size(); i++)
        {
            if(newReg == vlg_CableCardRegistrations[i])
            {
               //auto_unlock(&podLock);
                return RMF_SUCCESS;
            }
        }
        
        vlg_CableCardRegistrations.push_back(newReg);
        //auto_unlock(&podLock);		
    }
    
    return RMF_SUCCESS;
}

rmf_Error vlMpeosPodCableCardUnRegister(rmf_osal_eventqueue_handle_t queueId, void *act)
{
    if(NULL != vlg_podDB)
    {
        rmf_PODDatabase & podDB = *vlg_podDB;
        rmf_osal_Mutex & podLock = podDB.pod_mutex;
        VL_MPEOS_AUTO_LOCK(podLock);

        VL_MPEOS_EVENT_REG_DATA oldReg(queueId, act);
        unsigned int i = 0;
        for(i = 0; i < vlg_CableCardRegistrations.size(); i++)
        {
            if(oldReg == vlg_CableCardRegistrations[i])
            {
                vlg_CableCardRegistrations.erase(vlg_CableCardRegistrations.begin() + i);
                //auto_unlock(&podLock);
                return RMF_SUCCESS;
            }
        }
        //auto_unlock(&podLock);
    }
    
    return RMF_SUCCESS;
}



void vl_cable_card_test_thread()
{
    rmf_osal_threadSleep(60000, 0);
    rmf_Error mpeError;
    unsigned char appid[] = {0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08};
    unsigned char dataquery[] =
    {
        0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x0A, 0x0B,
        0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x0A, 0x0B,
        0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x0A, 0x0B,
        0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x0A, 0x0B,
        0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x0A, 0x0B,
        0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x0A, 0x0B,
        0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x0A, 0x0B,
        0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x0A, 0x0B,
    };
    unsigned int sessionId = 0;
    unsigned int recvSesId = 0;
    unsigned char * pData = NULL;
    int nLen = 0;
    mpeError = rmf_podmgrSASConnect(appid, &sessionId, NULL);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: rmf_podmgrSASConnect returned 0x%08X, session = 0x%08X\n", __FUNCTION__, mpeError, sessionId);
    rmf_osal_threadSleep(10000, 0);
    mpeError = rmf_podmgrSendAPDU(sessionId, SAS_DATA_QUERY_TAG, 1, dataquery);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: rmf_podmgrSendAPDU() returned 0x%08X, session = 0x%08X\n", __FUNCTION__, mpeError, sessionId);
    mpeError = vlMpeosPodReceiveAPDU(&recvSesId, &pData, &nLen);
    // remove connect confirmation tag
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: vlMpeosPodReceiveAPDU() returned 0x%08X, session = 0x%08X, len = %d\n", __FUNCTION__, mpeError, sessionId, nLen);
    mpeError = vlMpeosPodReceiveAPDU(&recvSesId, &pData, &nLen);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: vlMpeosPodReceiveAPDU() returned 0x%08X, session = 0x%08X, len = %d\n", __FUNCTION__, mpeError, sessionId, nLen);
    if((NULL != pData) && (nLen > 0)) if(rdk_dbg_enabled("LOG.RDK.POD", RDK_LOG_INFO)) vlMpeosDumpBuffer(RDK_LOG_INFO, "LOG.RDK.POD", pData, nLen);
}

rmf_osal_SocketHostEntry* vlMpeosGetHostIpInfo(char *hostname, int namelen)
{
    if (vlg_hostent == NULL)
    {
        //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.NET", "vlg_hostent is NULL. Allocating and populating ....\n");    
        
        int err;
        
        if (hostname == NULL)
            return NULL;
            
        err = rmf_osal_memAllocP(RMF_OSAL_MEM_NET, sizeof(rmf_osal_SocketHostEntry),(void **)&vlg_hostent);
        if(err != RMF_SUCCESS)
        {
            RDK_LOG(RDK_LOG_WARN, "LOG.RDK.NET", "<rmf_osal_SocketHostEntry> Mem allocation failed. Returning NULL ...\n");    
            return NULL;
        }
        
   
        VL_HOST_IP_INFO phostipinfo;
        vlXchanGetDsgIpInfo(&phostipinfo);
    
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.NET", "vlMpeosXchanGetDsgIpInfo: IPAddress = '%s' IFName : '%s' addressType = %d\n", phostipinfo.szIpAddress, phostipinfo.szIfName, phostipinfo.ipAddressType);
        
                
            //ret->h_name = hostname;
        if(rmf_osal_memAllocP(RMF_OSAL_MEM_NET, namelen+1 , (void **)&(vlg_hostent->h_name)) != RMF_SUCCESS)
        {
            RDK_LOG(RDK_LOG_WARN, "LOG.RDK.NET", "<ret->h_name> Mem allocation failed.\n");    
            return NULL;
        }
            
        strcpy(vlg_hostent->h_name, hostname);
                
        switch (phostipinfo.ipAddressType)
        {
            case VL_XCHAN_IP_ADDRESS_TYPE_IPV4:
                vlg_hostent->h_addrtype = RMF_OSAL_SOCKET_AF_INET4;
                vlg_hostent->h_length = sizeof(phostipinfo.aBytIpAddress);
                vlg_hostent->h_aliases = NULL;
                    
                    // only one address for DOCSIS IP
                if(rmf_osal_memAllocP(RMF_OSAL_MEM_NET, 2 * sizeof(char *),(void **)&(vlg_hostent->h_addr_list)) != RMF_SUCCESS)
                {
                    RDK_LOG(RDK_LOG_WARN, "LOG.RDK.NET", "<rmf_osal_SocketHostEntry->h_addr_list> Mem allocation failed. Returning NULL ...\n");    
                    return NULL;
                }
                    
                if(rmf_osal_memAllocP(RMF_OSAL_MEM_NET, 40 * sizeof(char), (void **)&(vlg_hostent->h_addr_list[0])) != RMF_SUCCESS)
                {
                    RDK_LOG(RDK_LOG_WARN, "LOG.RDK.NET", "<rmf_osal_SocketHostEntry->h_addr_list[0]> Mem allocation failed. Returning NULL ...\n");    
                    return NULL;
                }
                //strncpy( (unsigned char *) ret->h_addr_list[0], phostipinfo.aBytIpAddress, sizeof(phostipinfo.aBytIpAddress));
                memcpy((unsigned char *)vlg_hostent->h_addr_list[0], phostipinfo.aBytIpAddress, sizeof(phostipinfo.aBytIpAddress));
                    
                vlg_hostent->h_addr_list[1] = NULL;
                
                if (NULL != vlg_hostent->h_addr_list[0])
                {
                    char *cp = vlg_hostent->h_addr_list[0];
    
                    //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.NET", " rmf_osal_SocketHostEntry.h_addr_list[0] ==  %d.%d.%d.%d\n", cp[0], cp[1], cp[2], cp[3]);
                }
                else
                {
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.NET",
                            "rmf_osal_SocketHostEntry.h_addr_list[%d] == NULL\n", 0);
                }
                break;
    #ifdef MPE_FEATURE_IPV6
            case VL_XCHAN_IP_ADDRESS_TYPE_IPV6:
                vlg_hostent->h_addrtype = RMF_OSAL_SOCKET_AF_INET6;
                vlg_hostent->h_length = sizeof(phostipinfo.aBytIpV6GlobalAddress);
                vlg_hostent->h_aliases = NULL;
                vlg_hostent->h_addr_list = NULL;
                break;
    #endif //MPE_FEATURE_IPV6
            default:
                vlg_hostent->h_addrtype = RMF_OSAL_SOCKET_AF_INET4;
                break;
        }
    }
    
    return vlg_hostent;
}

void PostBootEvent(int eEventType, int nMsgCode, unsigned long ulData){

      RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Boot Event = [%x] and code = [%x]",eEventType,nMsgCode);
      rmf_osal_eventmanager_handle_t  em = get_pod_event_manager();	  
      rmf_osal_event_handle_t event_handle;
      rmf_osal_event_params_t event_params = {0};
      rmf_osal_event_category_t event_category;
      uint32_t event_type;

      event_params.data = (void *)ulData;
      event_params.data_extension = nMsgCode;
      rmf_osal_event_create(RMF_OSAL_EVENT_CATEGORY_BOOTMSG_DIAG, eEventType, 
	  	 &event_params, &event_handle);
      rmf_osal_eventmanager_dispatch_event(em, event_handle);
      vlMpeosPostPodEvent(eEventType, (void *)ulData, nMsgCode);	  
}


