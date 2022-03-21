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
#include "core_events.h"
#include "cardmanager.h"
#include "cmevents.h"
#include "systime.h"
//#include "pfcpluginbase.h"
//#include "pmt.h"
//#include "tablemgr.h"
#define __MTAG__ VL_CARD_MANAGER




    
cSysTime::cSysTime(CardManager *cm,char *name):CMThread(name)
{
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","cSysTime::cSysTime()\n");
    event_queue = 0;
    this->cm = cm;
    
    
}
        
    
cSysTime::~cSysTime(){}
    
    
    
void cSysTime::initialize(void){

    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    rmf_osal_eventqueue_handle_t eventqueue_handle;

    rmf_osal_eventqueue_create ( (const uint8_t* ) "cSysTime",
		&eventqueue_handle);
	

//PodMgrEventListener    *listener;

        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","cSysTime::initialize()\n");
     rmf_osal_eventmanager_register_handler(
		em,
		eventqueue_handle, RMF_OSAL_EVENT_CATEGORY_SYSTIME
		);

     rmf_osal_eventmanager_register_handler(
		em,
		eventqueue_handle, RMF_OSAL_EVENT_CATEGORY_TABLE_MANAGER
		);

        this->event_queue = eventqueue_handle;
        //listener = new PodMgrEventListener(cm, eq,"ResourceManagerThread);

        
        //listener->start();
        

}


 


void cSysTime::run(void ){
    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    rmf_osal_event_handle_t event_handle_rcv;
    rmf_osal_event_params_t event_params_rcv = {0}; 
    rmf_osal_event_category_t event_category;
    uint32_t event_type;


    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","SysTimeEventListener::Run()\n");
    systimeInit(); 
    while (1)
    {
        int result = 0;

        rmf_osal_eventqueue_get_next_event( event_queue,
		&event_handle_rcv, &event_category, &event_type, &event_params_rcv);

        
        switch (event_category)
        {
            case RMF_OSAL_EVENT_CATEGORY_SYSTIME:
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","SysTimeEventListener::PFC_EVENT_CATEGORY_SYSTIME\n");
                if( event_params_rcv.data ) systimeProc(event_params_rcv.data);
                break;
            case RMF_OSAL_EVENT_CATEGORY_TABLE_MANAGER:
                switch (event_type)
                {
                    case TableManager_STT_available:
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","SysTimeEventListener: Received table_STT_available event \n");
                    systimeSTTUpdateEvent(0);
                    break;
                }
                break;
            default:
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","SysTimeEventListener::default\n");
                break;
        }

        rmf_osal_event_delete(event_handle_rcv);

    }


}


















