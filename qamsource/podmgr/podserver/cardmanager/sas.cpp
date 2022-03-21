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


#include "cardUtils.h"
#include "cmhash.h"
#include "rmf_osal_event.h"
#include "rmf_osal_sync.h"
#include "core_events.h"
#include "cardmanager.h"
#include "cmevents.h"
#include "sas.h"

#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif

#include <string.h>
#define __MTAG__ VL_CARD_MANAGER



    
cSAS::cSAS(CardManager *cm,char *name):CMThread(name)
{
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","cSAS::cSAS()\n");
    sasMutex = 0;	
    event_queue  = 0;	
    this->cm = cm;
    rmf_osal_mutexNew(&sasMutex);
    
}
        
    
cSAS::~cSAS(){

rmf_osal_mutexDelete(sasMutex);

}
    
    
    
void cSAS::initialize(void){

    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();

    rmf_osal_eventqueue_handle_t eventqueue_handle;
    rmf_osal_eventqueue_create ( (const uint8_t* ) "cSAS",
		&eventqueue_handle);
	
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","cSAS::initialize()\n");	
	
    rmf_osal_eventmanager_register_handler(
		em,
		eventqueue_handle, RMF_OSAL_EVENT_CATEGORY_SAS
		);

//PodMgrEventListener    *listener;

        this->event_queue = eventqueue_handle;
        //listener = new PodMgrEventListener(cm, eq,"ResourceManagerThread);

        
        //listener->start();
        

}


 


void cSAS::run(void ){
    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    LCSM_EVENT_INFO *pEventInfo=NULL;
    rmf_osal_event_handle_t event_handle_rcv;
    rmf_osal_event_params_t event_params_rcv = {0}; 
    rmf_osal_event_category_t event_category;
    uint32_t event_type;

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","SASEventListener::Run()\n");
    sas_init();
    while (1)
    {
        int result = 0;

        rmf_osal_eventqueue_get_next_event( event_queue,
		&event_handle_rcv, &event_category, &event_type, &event_params_rcv);

        pEventInfo = (LCSM_EVENT_INFO *)event_params_rcv.data;

        switch (event_category)
        {
            case RMF_OSAL_EVENT_CATEGORY_SAS:
                if( pEventInfo )				
                {
                    //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","SASEventListener::RMF_OSAL_EVENT_CATEGORY_SAS\n");
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "SASEventListener::PFC_EVENT_CATEGORY_SAS. event->eventInfo.event = %d\n" , pEventInfo->event);
                    if(((pEventInfo->event)  == POD_APDU) && this->cm->IsOCAP())
                    {
                        //Send Raw Response APDU to OCAP
                        if(this->cm->pCMIF->cb_fn)
                            //callback
                        {
                        
                            //lock mutex
                            rmf_osal_mutexAcquire(sasMutex);
                            this->cm->pCMIF->cb_fn ((uint8_t *)pEventInfo->y,(uint16_t)pEventInfo->z );
                            
                            // unlock mutex
                            rmf_osal_mutexRelease(sasMutex);
                        }
                        else //send pEventInfo
                        {
                            EventCableCard_RawAPDU *pEventApdu;
                            //do a dispatch pEventInfo here
                            rmf_osal_event_handle_t event_handle;
                            rmf_osal_event_params_t event_params = {0};
           
             		   rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(EventCableCard_RawAPDU),(void **)&pEventApdu);
        
                            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "SASEventListener: POST SAS RAW APDU to the queue\n");
                        
                            pEventApdu->resposeData = new uint8_t[(uint16_t)pEventInfo->z];
    
    /*                        memcpy(pEventApdu->resposeData , (uint8_t *)pEventInfo->eventInfo.y, (uint16_t)pEventInfo->eventInfo.z);*/
                            memcpy(pEventApdu->resposeData , (uint8_t *)pEventInfo->y, (uint16_t)pEventInfo->z);
                            pEventApdu->dataLength  =  (uint16_t)pEventInfo->z;
                            pEventApdu->sesnId = (short)pEventInfo->x;
    
                            event_params.priority = 0; //Default priority;
                            event_params.data = (void *)pEventApdu;
                            event_params.data_extension = sizeof(EventCableCard_RawAPDU);
                            event_params.free_payload_fn = EventCableCard_RawAPDU_free_fn;
                            rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_CM_RAW_APDU, card_Raw_APDU_response_avl, 
                        		&event_params, &event_handle);
                            rmf_osal_eventmanager_dispatch_event(em, event_handle);
                                            
                        }                        
                        break;
                    }
                    
                    rmf_osal_mutexAcquire(sasMutex);
                    sasProc((void *)pEventInfo);
                    rmf_osal_mutexRelease(sasMutex);
      	         }
                break;
            default:
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","SASEventListener::default\n");
                break;
        }

        rmf_osal_event_delete(event_handle_rcv);
    }


}





















