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
#include "core_events.h"
#include "cardmanager.h"
#include "cmevents.h"
#include "hostAddPrt.h"

//#include "pmt.h"
#include "poddriver.h"
#include "cm_api.h"
#include <string.h>

#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif

#define __MTAG__ VL_CARD_MANAGER


cHostAddPrt::cHostAddPrt(CardManager *cm,char *name):CMThread(name)
{
    //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","cCardRes::cCardRes()\n");
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","Entered ..... cHostAddPrt::cHostAddPrt  ###################### \n");
    this->cm = cm;
    this->event_queue = 0;	    
    
}
        
    
cHostAddPrt::~cHostAddPrt(){}
    
    
    
void cHostAddPrt::initialize(void)
{

    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    rmf_osal_eventqueue_handle_t eventqueue_handle;

    rmf_osal_eventqueue_create ( (const uint8_t* ) "cHostAddPrt",
		&eventqueue_handle);


//    PodMgrEventListener *listener;
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","Entered ..... cHostAddPrt::initialize ###################### \n");
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","cHostAddPrt::initialize()\n");

     rmf_osal_eventmanager_register_handler(
		em,
		eventqueue_handle, RMF_OSAL_EVENT_CATEGORY_HOST_APP_PRT
		);     
    this->event_queue = eventqueue_handle;

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","cHostAddPrt::initialize() eq=%x\n",eventqueue_handle);
    
}

//static vlCableCardCertInfo_t CardCertInfo;
void cHostAddPrt::run(void )
{
    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    rmf_osal_event_handle_t event_handle_rcv;
    rmf_osal_event_params_t event_params_rcv = {0}; 
    rmf_osal_event_category_t event_category;
    uint32_t event_type;
    LCSM_EVENT_INFO	*pEventInfo;

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","Entered ..... cHostAddPrt::run  ###################### \n");
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","cHostAddPrt::Run()\n");

    hostAddPrt_init();
    while (1)
    {
        int result = 0;

        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","#############cHostAddPrt::run  waiting for Event #################\n");
        rmf_osal_eventqueue_get_next_event( event_queue,
		&event_handle_rcv, &event_category, &event_type, &event_params_rcv);
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","#############cHostAddPrt::After run  waiting for Event #################\n");
        switch (event_category)
        {
            case RMF_OSAL_EVENT_CATEGORY_HOST_APP_PRT:
				
                pEventInfo = (LCSM_EVENT_INFO *)event_params_rcv.data;
                if (pEventInfo )				
                {
                     RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","cHostAddPrt::PFC_EVENT_CATEGORY_HOST_APP_PRT\n");
                     RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ########### hostAddPrtProc:RMF_OSAL_EVENT_CATEGORY_HOST_APP_PRT ######### \n");
                     
     
                     if(((pEventInfo->event)  == POD_APDU) && this->cm->IsOCAP())
                     {
                         //Send Raw Response APDU to OCAP
                         
                         {
                             EventCableCard_RawAPDU *pEventApdu;
                             //do a dispatch event here
                             rmf_osal_event_handle_t event_handle;
                             rmf_osal_event_params_t event_params = {0};
            
              		   rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(EventCableCard_RawAPDU),(void **)&pEventApdu);
                                 
                             pEventApdu->resposeData = new uint8_t[(uint16_t)pEventInfo->z];
     
     /*                        memcpy(pEventApdu->resposeData , (uint8_t *)event->eventInfo.y, (uint16_t)event->eventInfo.z);*/
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
                         //break;
                     }
                     RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ########### calling hostAddPrtProc ########## \n");
                     hostAddPrtProc((void *)pEventInfo);
     //                GetCableCardCertInfo(&CardCertInfo);
           	  }
                break;
            default:
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","hostAddPrtProc::default\n");
                break;
        }

        rmf_osal_event_delete(event_handle_rcv);
    }


}
