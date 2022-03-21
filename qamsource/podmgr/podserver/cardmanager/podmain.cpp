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
#include "cm_api.h"
#include "oob_rdc.h"
#include "xchanResApi.h"
#include "rmf_osal_sync.h"
/////////////////////////
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>
/////////////////////////////
#include "cardManagerIf.h"

#if USE_SYSRES_MLT
#include "rpl_malloc.h"
#include "rpl_new.h"
#endif

#define __MTAG__ VL_CARD_MANAGER

 

#ifdef __cplusplus
extern "C" {
#endif
    extern  unsigned char trans_CreateTC (unsigned char bNewTcId);
    extern  unsigned int AM_INIT();
#ifdef __cplusplus
}
#endif    

cPodPresenceEventListener::cPodPresenceEventListener(CardManager *c,char *name):CMThread(name)
{
    
    this->cm = c;
    this->event_queue = NULL;
    this->pCMextFlowObj = NULL;
}
    

void cPodPresenceEventListener::initialize(void){

    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();

//PodMgrEventListener    *listener;

        rmf_osal_eventqueue_handle_t eventqueue_handle;
        rmf_osal_eventqueue_create ( (const uint8_t* ) "cPodPresenceEventListener",
   		&eventqueue_handle);
        rmf_osal_eventmanager_register_handler(
   		em, eventqueue_handle, 
   		RMF_OSAL_EVENT_CATEGORY_CardManagerMMIEvents
   		);        	

        rmf_osal_eventmanager_register_handler(
   		em, eventqueue_handle, 
   		RMF_OSAL_EVENT_CATEGORY_CARD_STATUS
   		);   

        rmf_osal_eventmanager_register_handler(
   		em, eventqueue_handle, 
   		RMF_OSAL_EVENT_CATEGORY_CardManagerXChanEvents
   		);  
		    
        this->event_queue = eventqueue_handle;
        //listener = new PodMgrEventListener(cm, eq,"ResourceManagerThread);

        //listener->start();
        
}


void cPodPresenceEventListener::run(void ){
    
//cOOB   *pOOB_packet_listener_thread;    
    cExtChannelFlow *pXFlowObj;
    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    rmf_osal_event_handle_t event_handle_rcv, event_handle;
    rmf_osal_event_params_t event_params_rcv = {0}; 
    rmf_osal_event_category_t event_category;
    uint32_t event_type, evType = 0;
    LCSM_EVENT_INFO *pEventInfo; 
    
    pXFlowObj = 0;
    while (1)
    {
        int result = 0;

        rmf_osal_eventqueue_get_next_event( event_queue,
		&event_handle_rcv, &event_category, &event_type, &event_params_rcv);
		
        switch (event_category)
        {
            case RMF_OSAL_EVENT_CATEGORY_CARD_STATUS:
                pEventInfo = (LCSM_EVENT_INFO *) event_params_rcv.data;
                if( NULL != pEventInfo )
		  {
			if(pEventInfo->event == POD_INSERTED)
			{
			    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","POD INSERTED...\n");
			    //this->cm->StartPODThreads();
			    this->cm->cablecardFlag = CARD_INSERTED;
			    this->cm->CmcablecardFlag = CM_CARD_INSERTED;
			    evType = pfcEventCableCard_System::card_inserted_event;
			    vlMpeosSetCardPresent(1);
			}
			else if(pEventInfo->event == POD_REMOVED)
			{
			    vlMpeosSetCardReady(0);
			    vlMpeosSetCardPresent(0);
			    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","POD_REMOVED...\n");
			    //this->cm->StopPODThreads();
			    // Aug-30-2008 : moved these statements to vlXchanCancelQpskOobIpTask()

			    this->cm->cablecardFlag = CARD_REMOVED;
			    this->cm->CmcablecardFlag = CM_CARD_REMOVED;
			    evType = pfcEventCableCard_System::card_removed_event;
			}
			else if(pEventInfo->event == POD_BAD_MOD_INSERT_EROR)
			{
			    rmf_osal_event_handle_t event_handle;
			    vlMpeosSetCardReady(0);
			    vlMpeosSetCardPresent(0);
			    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","POD BAD POD INSERTED...\n");
			    //this->cm->StartPODThreads();
			    this->cm->cablecardFlag = CARD_ERROR;
			    this->cm->CmcablecardFlag = CM_CARD_ERROR;
			    evType = pfcEventCableCard_System::card_error_event;

			   rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_CARD_MANAGER, CardMgr_Bnd_Fail_Incompatible_Module, 
					NULL, &event_handle);               

			   rmf_osal_eventmanager_dispatch_event(em, event_handle);
			}

			rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_CardManagerSystemEvents, evType, 
				NULL, &event_handle);            
			rmf_osal_eventmanager_dispatch_event(em, event_handle);
		  }            
                break;
            case RMF_OSAL_EVENT_CATEGORY_CardManagerXChanEvents:
                switch (event_type)
                {
                    case card_xchan_resource_opened:
                    {
                        cExtChannelFlow::RequestExtChannelFlowWithRetries(this, CM_SERVICE_TYPE_MPEG_SECTION);
                    }
                    break;
                    
                    case card_xchan_start_oob_ip_over_qpsk:
                    {            
                        // Enabled: Aug-06-2007: For ext-chnl ip flow
                        // Jan-17-2008: Added config in etc/pfcrc
                        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","cPodPresenceEventListener::run: received card_xchan_start_oob_ip_over_qpsk\n");
                        if(vl_xchan_session_is_active())
                        {
                            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","cPodPresenceEventListener::run: checking if ip over qpsk is enabled\n");
//                            if(pfc_kernel_global->is_ip_over_qpsk_enabled())
				if(0)
                            {
                                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","cPodPresenceEventListener::run: checking if oob-ip thread is already running\n");
                                rmf_osal_mutexAcquire(vlXChanGetOobThreadLock());
                                {
                                    if(NULL == this->cm->pOOB_packet_listener_thread)
                                    {
                                        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","cPodPresenceEventListener::run: starting oob-ip thread\n");
                                        this->cm->pOOB_packet_listener_thread = new cOOB(this->cm,"POD-OOB-PacketListener-thread");


                                        this->cm->pOOB_packet_listener_thread->initialize();
                                        this->cm->pOOB_packet_listener_thread->start();
                                    }
                                }
                                rmf_osal_mutexRelease(vlXChanGetOobThreadLock());
                            }
                        }
                    }
                    break;
                    
                    case card_xchan_stop_oob_ip_over_qpsk:
                    {
                    }
                    break;
#ifdef USE_IPDIRECT_UNICAST
                    // IPDU register flow
                    case card_xchan_start_ipdu:
                    {
                        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","cPodPresenceEventListener::run: received card_xchan_start_ipdu\n");
                        cExtChannelFlow::RequestExtChannelFlowWithRetries(this, CM_SERVICE_TYPE_IPDU);
                    }
                    break;

                    case card_xchan_stop_ipdu:
                    {
                        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","cPodPresenceEventListener::run: received card_xchan_stop_ipdu\n");
                        // TOD IPDU
                    }
                    break;
#endif // USE_IPDIRECT_UNICAST
                }
                break;
             
            default:
                 
                break;
        }
        
        rmf_osal_event_delete(event_handle_rcv);
        
    }
}


