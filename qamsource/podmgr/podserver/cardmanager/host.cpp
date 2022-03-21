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

#ifdef GCC4_XXX
#include <list>
#else
 #include <list.h>
#endif

//#include "pfcresource.h"
#include "cardUtils.h"
#include "cmhash.h"
#include "rmf_osal_event.h"
#include "rmf_osal_sync.h"
#include "core_events.h"
#include "cardmanager.h"
#include "cmevents.h"
#include "host.h"
//#include "pfcpluginbase.h"
#ifdef OOB_TUNE
//#include "pfcoobtunerdriver.h"
#endif
#include "tunerdriver.h"
//#include "tablemgr.h"
//#include "psipdb_hndlr.h"
//#include <string.h>
#include <string.h>
#ifdef GLI
#include "libIBus.h"
#include "sysMgr.h"
#endif
#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif

#ifdef USE_IPDIRECT_UNICAST
#include <rmf_pod.h>
#endif

#define __MTAG__ VL_CARD_MANAGER

//#define RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET",a, ...)    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", a, ## __VA_ARGS__ )

typedef enum {
    ChannelType_Analog,
    ChannelType_OnePart,
    ChannelType_TwoPart,
}ChannelType;

typedef struct _channelInfo
{
    int permanent;
    int physicalCh;
    int majorCh;
    int minorCh;
    int frequency;
    int modulation;
    int type;
    unsigned char bAppVirtualChan;//flag representing svct application vc number
    int sourceId;
    int programNumber;
    char description[100];
    char text[100];
    int pcrPid;
    int videoPid;
    int audioPid;
    int favoriteChannel;
    //int dbPrimaryKey;
    //columns for access_control,hidden into VCT Schema
    int access_controlled;
    int hidden;
    int chan_tsid;

    ChannelType chType;
    unsigned long primaryKey;


}ChannelInfo;



extern "C" void PostBootEvent(int eEventType, int nMsgCode, unsigned long ulData);

cHostControl::cHostControl(CardManager *cm,char *name):CMThread(name)
{
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","cHostControl::cHostControl()\n");
    hostControlMutex = 0;
    event_queue = 0;
    rmf_osal_mutexNew(&hostControlMutex);
    this->cm = cm;


}


cHostControl::~cHostControl()
{
    rmf_osal_mutexDelete(hostControlMutex);
}



void cHostControl::initialize(void)
{

    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();

    rmf_osal_eventqueue_handle_t eventqueue_handle;
    rmf_osal_eventqueue_create ( (const uint8_t* ) "cHostControl",
		&eventqueue_handle);
     rmf_osal_eventmanager_register_handler(
		em,
		eventqueue_handle, RMF_OSAL_EVENT_CATEGORY_HOST_CONTROL
		);

//PodMgrEventListener    *listener;

        //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","cHostControl::initialize()\n");
        this->event_queue = eventqueue_handle;

}


int cHostControl::GetModAndFreqForSourceId(unsigned short sourceId, unsigned long *pMod, unsigned long *pFreq)
{
    int result = -1;

#ifdef USE_HARVEST_MANAGER

    ChannelInfo serviceInfo;
    TableSourceEnum tunerInputType = TableSource_Cable;

    cPSIPDB_Handler* pPSIPDB_Handler = cPSIPDB_Handler::getInstance();

    memset((void *)&serviceInfo,0,sizeof(ChannelInfo));

    result = pPSIPDB_Handler->getServiceInfo(&tunerInputType, sourceId, serviceInfo);

    if(result != 0)
        return result;

    *pFreq  = serviceInfo.frequency;

    switch (serviceInfo.modulation)
    {
        case 8:// QAM 64
            *pMod = 1;
            break;
        case 16: // QAM 256
            *pMod = 2;
            break;
        default:
            *pMod = 1;
    }
#endif // USE_HARVEST_MANAGER

RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s:  sourceId = %d, sourceId = 0x%x, Modulation = %d, Freq = %d\n", __FUNCTION__, sourceId,sourceId, *pMod, *pFreq);
    return result;
}


#ifdef __cplusplus
extern "C"
{
  #define POD_HOMING_FIRMUPGRADE            0x95
  #define POD_HOMING_FIRMUPGRADE_REPLY    0x96
  #define POD_HOMING_FIRMUPGRADE_COMPLETE    0x97
  #define POD_HOMING_FIRMUPGRADE_TUNING_ACCEPTED 0x00 // Tuning accepted
  extern uint32_t getLtsId(uint8_t tunerId);
  static rmf_osal_eventqueue_handle_t m_queueId = 0;
  static int reqFrequency =-1;
  static int reqModulation = -1;
  static int reqSourceId = -1;
  typedef struct 
  {
	int freq;
	int modulation;
	int source_id;
	void *act;
  }cardFWUEventInfo_t ;

  static void *m_act = NULL;
  
#ifdef USE_IPDIRECT_UNICAST
  void vlMpeosPostInbandTuneRequestEvent(int frequency,int modulation,int sourceId)
  {
      rmf_Error err = RMF_SUCCESS;
      rmf_osal_event_handle_t Event;
      rmf_osal_event_params_t event_params;

      RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s(): frequency =%d and sourceId =%d\n", __FUNCTION__,frequency,sourceId);
      RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s(): eventQueueID =0x%x \n",__FUNCTION__,m_queueId);

      if(m_queueId == 0)
      {
	reqFrequency = frequency;
	reqModulation = modulation;
	reqSourceId = sourceId;
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s(): m_queueId = 0, not sending RMF_PODMGR_EVENT_INB_TUNE_START event \n", __FUNCTION__);
      }
      else
      {

       event_params.data = (void *)((sourceId != -1) ? sourceId : frequency);
       event_params.priority = 0;
       event_params.data_extension = (sourceId != -1) ? 0 : modulation;
       event_params.free_payload_fn = 0;

       err = rmf_osal_event_create(RMF_OSAL_EVENT_CATEGORY_POD, RMF_PODMGR_EVENT_INB_TUNE_START, &event_params, &Event);
       if( RMF_SUCCESS != err )
       {
           RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "rmf_osal_event_create failed in %s:%d\n",__FUNCTION__, __LINE__ );
       }
       err = rmf_osal_eventqueue_add( m_queueId, Event);
       if( RMF_SUCCESS != err )
       {
           RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "rmf_osal_eventmanager_dispatch_event failed in %s:%d\n", __FUNCTION__, __LINE__ );  
       }
      }
  }

  void vlMpeosPostInbandTuneReleaseEvent(int success)
  {

      rmf_osal_event_handle_t Event;
      rmf_osal_event_params_t event_params;
      rmf_Error err = RMF_SUCCESS;

      event_params.data = (void *)success;
      event_params.priority = 0;
      event_params.data_extension = 0;
      event_params.free_payload_fn = NULL;

      if(m_queueId != 0)
      {
          RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s() \n", __FUNCTION__);
          err = rmf_osal_event_create(RMF_OSAL_EVENT_CATEGORY_POD, RMF_PODMGR_EVENT_INB_TUNE_RELEASE, &event_params, &Event);
          if( RMF_SUCCESS != err )
	  {
              RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "rmf_osal_event_create failed in %s:%d\n",__FUNCTION__, __LINE__ );
	  }

          err = rmf_osal_eventqueue_add( m_queueId, Event);
          if( RMF_SUCCESS != err )
	  {
	      RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "rmf_osal_eventmanager_dispatch_event failed in %s:%d\n", __FUNCTION__, __LINE__ );
	  }
      }
      else
      {
          RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s(): m_queueId = 0, not sending RMF_PODMGR_EVENT_INB_TUNE_START event \n", __FUNCTION__);
      }
  }
#endif

  void vlMpeosPostHomingTuneRequestEvent(int frequency,int modulation,int sourceId)
  {
      rmf_Error err = RMF_SUCCESS;
      rmf_osal_event_handle_t Event;

	rmf_osal_event_params_t event_params;


      RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlMpeosPostHomingTuneRequestEvent frequency =%d and sourceId =%d\n",frequency,sourceId);
      RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlMpeosPostHomingTuneRequestEvent eventQueueID =0x%x \n",m_queueId);
      
      if(m_queueId == 0)
      {
	reqFrequency = frequency;
	reqModulation = modulation;
	reqSourceId = sourceId;
      }
      if(m_queueId != 0)
      {
/*
         cardFWUEventInfo_t *pEventInfo;

         rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(cardFWUEventInfo_t),(void **)&pEventInfo);

	  if(sourceId != -1)
	  {               
	        pEventInfo->act = m_act;
               pEventInfo->source_id = sourceId;
               pEventInfo->freq = 0;
               pEventInfo->modulation = 0;

	  }
	  else
	  {
             pEventInfo->act = m_act;
             pEventInfo->source_id = 0;
             pEventInfo->freq = frequency;
             pEventInfo->modulation = modulation;

	  }

       event_params.data = (void *) pEventInfo;	
       event_params.priority = 0;
       event_params.data_extension = 0;
       event_params.free_payload_fn = 0;//podmgr_freefn;	  //consumer has to free explicitly
*/

       event_params.data = (void *)((sourceId != -1) ? sourceId : frequency);	
       event_params.priority = 0;
       event_params.data_extension = (sourceId != -1) ? 0 : modulation;
       event_params.free_payload_fn = 0;
       
       rmf_osal_event_create(RMF_OSAL_EVENT_CATEGORY_POD, POD_HOMING_FIRMUPGRADE, &event_params, &Event);
       rmf_osal_eventqueue_add( m_queueId, Event);
	  
      }
  }
  
  void vlMpeosPostHomingCompleteEvent(int success)
  {

      rmf_osal_event_handle_t Event;      
      rmf_osal_event_params_t event_params;

      event_params.data = (void *)success;	
      event_params.priority = 0;
      event_params.data_extension = 0;
      event_params.free_payload_fn = NULL;
  
      if(m_queueId != 0)
      {
          rmf_osal_event_create(RMF_OSAL_EVENT_CATEGORY_POD, POD_HOMING_FIRMUPGRADE_COMPLETE, &event_params, &Event);
          rmf_osal_eventqueue_add( m_queueId, Event);
      }
	
  }
#ifdef __cplusplus
extern "C"
{
#endif
rmf_Error podmgrRegisterHomingQueue(rmf_osal_eventqueue_handle_t eventQueueID, void* act);
#ifdef __cplusplus
}
#endif
  rmf_Error podmgrRegisterHomingQueue(rmf_osal_eventqueue_handle_t eventQueueID, void* act)
  {
      RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","podmgrRegisterHomingQueue eventQueueID =0x%x \n",eventQueueID);
      
      if(m_queueId == 0)
      {
	m_queueId = eventQueueID;
	m_act = act;
	
	if((reqFrequency != -1 && reqModulation != -1) || reqSourceId != -1)
	{
	  vlMpeosPostHomingTuneRequestEvent(reqFrequency,reqModulation,reqSourceId);
	}
        return RMF_SUCCESS;
      }
      else
      {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","podmgrRegisterHomingQueue m_queueId is already initialized, m_queueId =0x%x \n",m_queueId);
	return RMF_OSAL_EINVAL;
      }
  }

  rmf_Error unRegisterHomingQueue(rmf_osal_eventqueue_handle_t eventQueueID, void* act)
  {
     if(m_queueId == eventQueueID)
     {
       RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","unRegisterHomingQueue eventQueueID =0x%x \n",eventQueueID);
       m_queueId = 0;
       m_act = NULL;
       return RMF_SUCCESS;
     }
     else
     {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","unRegisterHomingQueue, m_queueId != eventQueueID, m_queueId =0x%x \n",m_queueId);
        return RMF_OSAL_EINVAL;
     }
  }

#ifdef USE_IPDIRECT_UNICAST
  rmf_Error podmgrStartInbandTune(uint8_t  ltsId, int32_t status)
  {
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","podmgrStartInbandTune::ltsId =%d\n",ltsId);
    LCSM_EVENT_INFO      *pEventInfo;
    rmf_osal_event_handle_t Event;
    rmf_osal_event_params_t event_params;
    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    uint32_t retVal = RMF_SUCCESS;

    if(status == POD_HOMING_FIRMUPGRADE_TUNING_ACCEPTED)
    {
      RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","%s: vlMpeosImpl_TunerGetLTSID returned %d and status =%d \n", __FUNCTION__,ltsId, status);
    }
    else
    {
	ltsId = -1;
    }

    //sending status to POD
    retVal = rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(LCSM_EVENT_INFO),(void **)&pEventInfo);

    if ( RMF_SUCCESS == retVal )
    {
        pEventInfo->event =  FRONTEND_INBAND_TUNE_STATUS;
        pEventInfo->x = status;
        pEventInfo->y = ltsId;// Get the  LTSID and fill it here

        pEventInfo->dataPtr = NULL;
        pEventInfo->dataLength = 0;

        event_params.data = (void *) pEventInfo;
        event_params.priority = 0;
        event_params.data_extension = 0;
        event_params.free_payload_fn = podmgr_freefn;
        retVal = rmf_osal_event_create(RMF_OSAL_EVENT_CATEGORY_HOST_CONTROL, FRONTEND_INBAND_TUNE_STATUS, &event_params, &Event);
        if( RMF_SUCCESS != retVal )
        {
            RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "rmf_osal_event_create failed in %s:%d\n",__FUNCTION__, __LINE__ );
	    return RMF_OSAL_EINVAL;
        }
    	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","%s::eventInfo.x=%d,eventInfo.y=%d",__FUNCTION__,pEventInfo->x,pEventInfo->y);
    	retVal = rmf_osal_eventmanager_dispatch_event(em, Event);
        if( RMF_SUCCESS != retVal )
        {
            RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD","rmf_osal_eventmanager_dispatch_event failed in %s:%d\n", __FUNCTION__, __LINE__ );
	    return RMF_OSAL_EINVAL;
        }
    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD","%s: rmf_osal_memAllocP failed in %s:%d\n",__FUNCTION__, __LINE__);
	return RMF_OSAL_EINVAL;
    }
    return RMF_SUCCESS;
  }
#endif

  void podmgrStartHomingDownload(uint8_t  ltsId, int32_t status)
  {
    
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","mpeos_startHomingDownload::ltsId =%d\n",ltsId);
    LCSM_EVENT_INFO      *pEventInfo;
    rmf_osal_event_handle_t Event;
    rmf_osal_event_params_t event_params;
    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    
    if(status == POD_HOMING_FIRMUPGRADE_TUNING_ACCEPTED)
    {
//      ltsId = getLtsId(tunerId);
//      if(0 == ltsId) 
    //  {
	 // RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","%s: vlMpeosImpl_TunerGetLTSID returned error for TunerId %d \n", __FUNCTION__, tunerId);
//      }
      RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","%s: vlMpeosImpl_TunerGetLTSID returned %d and status =%d \n", __FUNCTION__,ltsId, status);
    }
    else
    {
	ltsId = -1;
    }

    //sending status to POD
    rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(LCSM_EVENT_INFO),(void **)&pEventInfo);

    pEventInfo->event =  FRONTEND_INBAND_TUNE_STATUS;
    pEventInfo->x = status;
    pEventInfo->y = ltsId;// Get the  LTSID and fill it here

    pEventInfo->dataPtr = NULL;
    pEventInfo->dataLength = 0;

    event_params.data = (void *) pEventInfo;	
    event_params.priority = 0;
    event_params.data_extension = 0;
    event_params.free_payload_fn = podmgr_freefn;
    rmf_osal_event_create(RMF_OSAL_EVENT_CATEGORY_HOST_CONTROL, FRONTEND_INBAND_TUNE_STATUS, &event_params, &Event);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","%s::eventInfo.x=%d,eventInfo.y=%d",__FUNCTION__,pEventInfo->x,pEventInfo->y); 
    rmf_osal_eventmanager_dispatch_event(em, Event);
  }
}
#endif //__cplusplusf


void cHostControl::run(void )
{
#ifdef OOB_TUNE
    TUNER_PARAMS    params;
#endif

	
    //unsigned long        status;

    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();


    //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","HostControlEventListener::Run()\n");
    hostControlInit();
    while (1)
    {
        rmf_osal_event_handle_t event_handle_rcv;
        rmf_osal_event_params_t event_params_rcv = {0}; 
        rmf_osal_event_category_t event_category;
        uint32_t event_type;    
        LCSM_EVENT_INFO      eventInfo, *pEventInfo;
		
        int result = 0;
        memset( &eventInfo, 0, sizeof(eventInfo) );
        rmf_osal_eventqueue_get_next_event( event_queue,
		&event_handle_rcv, &event_category, &event_type, &event_params_rcv);

        switch (event_category)
        {
            case RMF_OSAL_EVENT_CATEGORY_HOST_CONTROL:
#if 1
                pEventInfo = (LCSM_EVENT_INFO *) event_params_rcv.data;
                if( pEventInfo == NULL ) 
                {     
                       RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","%s: pEventInfo is NULL !!\n",__FUNCTION__);
                       break;				
                }
                if((pEventInfo->event)  == FRONTEND_OOB_DS_TUNE_REQUEST)
                {

                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","FRONTEND_OOB_DS_TUNE_REQUEST..\n");
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","freq=%ld\n",pEventInfo->x);
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","data-rate=%ld\n",pEventInfo->y);
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","spectrum =%ld\n",pEventInfo->z);
#ifdef OOB_TUNE
                    params.rx_tune_params.ulFreq = pEventInfo->x;
                    params.rx_tune_params.bSpinv = pEventInfo->z;
                    params.rx_tune_params.ulSymbolRate = (pEventInfo->y  * 1000)/2 ; //converting to symbol rate in KSymbols per sec

                    if(this->cm->ptuner_driver->rx_tune(& params) != 0)
                    {
                        rmf_osal_event_handle_t event_handle;
                        rmf_osal_event_params_t event_params = {0};	
                        LCSM_EVENT_INFO      *pEventSend = NULL;
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," NOT ...OOB_DS_QPSK_TUNE_OK....................\n" );
                        eventInfo.event =  FRONTEND_OOB_DS_TUNE_STATUS;
                        eventInfo.x = OOB_DS_QPSK_TUNE_FAILED;
                        eventInfo.y = 0;
                        eventInfo.z = POD_HOST_QUEUE;
                        eventInfo.dataPtr = NULL;
                        eventInfo.dataLength = 0;
			   eventInfo.sequenceNumber = 0;
						
                        rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(LCSM_EVENT_INFO),(void **)&pEventSend);
                        memcpy(pEventSend ,&eventInfo, sizeof (LCSM_EVENT_INFO));

                        event_params.priority = 0; //Default priority;
                        event_params.data = (void *)pEventSend;
                        event_params.data_extension = 0;
                        event_params.free_payload_fn = podmgr_freefn;

                        rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_HOST_CONTROL, eventInfo.event, 
                    		&event_params, &event_handle);
                        rmf_osal_eventmanager_dispatch_event(em, event_handle);

                        /*Send boot msg .  - Prasad (09/27/2009)*/
                        PostBootEvent(RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD_OOB, RMF_BOOTMSG_DIAG_MESG_CODE_ERROR, 1);
                        break;
                    }
                    else
#endif
                    {
                        rmf_osal_event_handle_t event_handle;     
                        rmf_osal_event_params_t event_params = {0};			
                        LCSM_EVENT_INFO      *pEventSend = NULL;						
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," OOB_DS_QPSK_TUNE_OK....................\n" );
                        eventInfo.event =  FRONTEND_OOB_DS_TUNE_STATUS;
                        eventInfo.x = OOB_DS_QPSK_TUNE_OK;
                        eventInfo.y = 0;
                        eventInfo.z = POD_HOST_QUEUE;
                        eventInfo.dataPtr = NULL;
                        eventInfo.dataLength = 0;

                        rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(LCSM_EVENT_INFO),(void **)&pEventSend);
                        memcpy(pEventSend ,&eventInfo, sizeof (LCSM_EVENT_INFO));

                        event_params.priority = 0; //Default priority;
                        event_params.data = (void *)pEventSend;
                        event_params.data_extension = 0;
                        event_params.free_payload_fn = podmgr_freefn;

                        rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_HOST_CONTROL, eventInfo.event, 
                    		&event_params, &event_handle);
                        rmf_osal_eventmanager_dispatch_event(em, event_handle);

                        /*Send boot msg .  - Prasad (09/27/2009)*/
                        PostBootEvent(RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD_OOB, RMF_BOOTMSG_DIAG_MESG_CODE_COMPLETE, 1);
#ifdef GLI
				IARM_Bus_SYSMgr_EventData_t eventData;					
				eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_DOCSIS;
				eventData.data.systemStates.state = 2;
				eventData.data.systemStates.error = 0;
				IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
#endif
                        break;

                    }

                }
                else if((pEventInfo->event)  == FRONTEND_OOB_US_TUNE_REQUEST)
                {
                    rmf_osal_event_handle_t event_handle;
                    rmf_osal_event_params_t event_params = {0};	
                    LCSM_EVENT_INFO      *pEventSend = NULL;							
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","FRONTEND_OOB_US_TUNE_REQUEST..\n");
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","freq=%ld\n",pEventInfo->x);
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","data-rate=%ld\n",pEventInfo->y);
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","power =%ld\n",pEventInfo->z);

#ifdef OOB_TUNE
                    params.tx_tune_params.ulFreq = pEventInfo->x;
                    // Change: Aug-06-2007: haltuner makes necessary conversions
                    params.tx_tune_params.ulPower = pEventInfo->z; //haltuner makes necessary conversions
                    params.tx_tune_params.ulSymbolRate = (pEventInfo->y * 1000) /2;

                    this->cm->ptuner_driver->tx_tune(& params);
#endif
                    //sending status to POD
                    eventInfo.event =  FRONTEND_OOB_US_TUNE_STATUS;

                    eventInfo.x = 0;
                    eventInfo.dataPtr = NULL;
                    eventInfo.dataLength = 0;
                    eventInfo.sequenceNumber = 0;
					
                    rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(LCSM_EVENT_INFO),(void **)&pEventSend);
                    memcpy(pEventSend ,&eventInfo, sizeof (LCSM_EVENT_INFO));

                    event_params.priority = 0; //Default priority;
                    event_params.data = (void *)pEventSend;
                    event_params.data_extension = 0;
                    event_params.free_payload_fn = podmgr_freefn;

                    rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_HOST_CONTROL, eventInfo.event, 
                		&event_params, &event_handle);
                    rmf_osal_eventmanager_dispatch_event(em, event_handle);

                    /*Send boot msg .  - Prasad (09/27/2009)*/
                    PostBootEvent(RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD_OOB, RMF_BOOTMSG_DIAG_MESG_CODE_COMPLETE, 2/*To represent TX_TUNE*/);
#ifdef GLI
			IARM_Bus_SYSMgr_EventData_t eventData;					
			eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_DOCSIS;
			eventData.data.systemStates.state = 2;
			eventData.data.systemStates.error = 0;
			IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
#endif

                    break;
                }

                else if((pEventInfo->event)  == FRONTEND_INBAND_TUNE_REQUEST)
                {
                    rmf_osal_event_handle_t event_handle;           
                    rmf_osal_event_params_t event_params = {0};											
                    unsigned short sourceId;
                    unsigned long modulation, freq;
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: FRONTEND_INBAND_TUNE_REQUEST..eventInfo.x=%ld, eventInfo.y =%ld, event->eventInfo.z =%ld \n", 
			      __FUNCTION__, pEventInfo->x, pEventInfo->y, pEventInfo->z);

                    if( (pEventInfo->x == 0) && (pEventInfo->y == 0) )
                    {
                        sourceId = pEventInfo->z;
                        vlMpeosPostHomingTuneRequestEvent(-1,-1,sourceId);	
#if 0						
                        modulation = 0;
                        freq = 0;
                        int result = GetModAndFreqForSourceId(sourceId, &modulation, &freq);
                        if(result != 0)
                        {
                            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","ERROR: Failed to get the Freq and Modulation for SourceID = %d\n", sourceId);
                            //break;
                        }

                        freq = freq / 1000; //Converting to Khz to make it same as the inband Homing tune based on mod and freq

                        pEventInfo->x = freq;
                        pEventInfo->y = modulation;
#endif						
                    }
                    else{
#ifdef USE_IPDIRECT_UNICAST
		           vlMpeosPostInbandTuneRequestEvent(pEventInfo->x * 1000,pEventInfo->y,-1);
#else
		           vlMpeosPostHomingTuneRequestEvent(pEventInfo->x * 1000,pEventInfo->y,-1);
#endif
		      }

#if 0
                    pfcTunerStatus status = this->cm->ptuner_driver->inband_tune((pEventInfo->x * 1000), pEventInfo->y);

                    //sending status to POD
                    eventInfo.event =  FRONTEND_INBAND_TUNE_STATUS;

                    eventInfo.x = (int)status;
                    eventInfo.y = 2;// Get the  LTSID and fill it here
                    eventInfo.dataPtr = NULL;
                    eventInfo.dataLength = 0;

                    rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(LCSM_EVENT_INFO),(void **)&pEventSend);
                    memcpy(&pEventSend ,&eventInfo, sizeof (LCSM_EVENT_INFO));

                    event_params.priority = 0; //Default priority;
                    event_params.data = (void *)pEventSend;
                    event_params.data_extension = 0;
                    event_params.free_payload_fn = podmgr_freefn;

                    rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_HOST_CONTROL, eventInfo.event, 
                		&event_params, &event_handle);
                    rmf_osal_eventmanager_dispatch_event(em, event_handle);
#endif					
                }
#ifdef USE_IPDIRECT_UNICAST
                else if((pEventInfo->event)  == FRONTEND_INBAND_TUNE_RELEASE)
                {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: FRONTEND_INBAND_TUNE_RELEASE ..eventInfo.x=%ld, eventInfo.y =%ld, event->eventInfo.z =%ld \n", 
			      __FUNCTION__, pEventInfo->x, pEventInfo->y, pEventInfo->z);
		    vlMpeosPostInbandTuneReleaseEvent(pEventInfo->x);
                }
#endif
                else
                {
                    rmf_osal_mutexAcquire(hostControlMutex);
                    hostProc(pEventInfo);
                    rmf_osal_mutexRelease(hostControlMutex);
                }
 #endif
                break;

            default:
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","HostControlEventListener::default\n");
                break;
        }

        rmf_osal_event_delete(event_handle_rcv);
    }

}








