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
#include "cp_prot.h"

#define __MTAG__ VL_CARD_MANAGER








    
cCP_Protection::cCP_Protection(CardManager *cm,char *name):CMThread(name)
{
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","cCP_Protection::cCP_Protection()\n");
    event_queue = 0;		
    this->cm = cm;
    
    
}
        
    
cCP_Protection::~cCP_Protection(){}
    
    
    
void cCP_Protection::initialize(void){

    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    rmf_osal_eventqueue_handle_t eq;
    rmf_osal_eventqueue_create ( (const uint8_t* ) "cCP_Protection",
		&eq);    	

//PodMgrEventListener    *listener;

        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","cCP_Protection::initialize()\n");
        rmf_osal_eventmanager_register_handler(
     		em, eq, RMF_OSAL_EVENT_CATEGORY_CP);
        this->event_queue = eq;
        //listener = new PodMgrEventListener(cm, eq,"ResourceManagerThread);
        ////VLALLOC_INFO2(listener, sizeof(PodMgrEventListener));

        
        //listener->start();
        

}


 


void cCP_Protection::run(void ){
    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    LCSM_EVENT_INFO *pEventInfo=NULL;

    rmf_osal_event_handle_t event_handle_rcv;
    rmf_osal_event_params_t event_params_rcv = {0}; 
    rmf_osal_event_category_t event_category;
    uint32_t event_type;
	
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","cCP_ProtectionEventListener::Run()\n");
    cprot_init();
    while (1)
    {
        int result = 0;

        rmf_osal_eventqueue_get_next_event( event_queue,
		&event_handle_rcv, &event_category, &event_type, &event_params_rcv);
        
        switch (event_category)
        {
            case RMF_OSAL_EVENT_CATEGORY_CP:
                pEventInfo = (LCSM_EVENT_INFO *)	event_params_rcv.data;
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","cCP_ProtectionEventListener::PFC_EVENT_CATEGORY_CP\n");
                if( pEventInfo )  cprotProc((void *)pEventInfo);
                break;
            default:
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","cCP_ProtectionEventListener::default\n");
                break;
        }

        rmf_osal_event_delete(event_handle_rcv);
    }


}





































