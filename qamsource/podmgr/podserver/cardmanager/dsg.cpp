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
#include <errno.h>


#include "cardUtils.h"
#include "cmhash.h"
#include "rmf_osal_event.h"
#include "core_events.h"
#include "cardmanager.h"
#include "cmevents.h"
#include "dsg.h"

#include "cm_api.h"
//#include <string.h>
#define __MTAG__ VL_CARD_MANAGER



extern "C" void dsg_init(void);
extern "C" void dsgProtectedProc(void* par); 

cDsgThread::cDsgThread(CardManager *cm, char *name):CMThread(name)
{
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","cDsgThread::cDsgThread()\n");
    this->cm = cm;
    event_queue = 0;	
    this->dsgResourceStatus = 0;
}
        
    
cDsgThread::~cDsgThread(){}
    

    
void cDsgThread::initialize(void)
{

    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    rmf_osal_eventqueue_handle_t eventqueue_handle;
    rmf_osal_eventqueue_create ( (const uint8_t* ) "cDsgThread",
		&eventqueue_handle);

     rmf_osal_eventmanager_register_handler(
		em,
		eventqueue_handle, RMF_OSAL_EVENT_CATEGORY_DSG
		);

    //PodMgrEventListener    *listener;
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," $$$$$$$$$$$$$$$$$$$$$$$$$$ cDsgThread::initialize $$$$$$$$$$$$$$$$$$$$ \n");
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","cDsgThread::initialize()\n");

    this->event_queue = eventqueue_handle;
}

void cDsgThread::run(void )
{
    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    rmf_osal_event_handle_t event_handle_rcv;
    rmf_osal_event_params_t event_params_rcv = {0}; 
    rmf_osal_event_category_t event_category;
    uint32_t event_type;

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","cDsgThread::Run()\n");
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," $$$$$$$$$$$$$$$ cDsgThread::run $$$$$$$$$$$$$$ \n");
#ifdef USE_DSG
//    dsg_test();
    dsg_init(); 
#endif
     
    while (1)
    {
        int result = 0;

        rmf_osal_eventqueue_get_next_event( event_queue,
		&event_handle_rcv, &event_category, &event_type, &event_params_rcv);

        switch (event_category)
        {
            case RMF_OSAL_EVENT_CATEGORY_DSG:
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," ######   RMF_OSAL_EVENT_CATEGORY_DSG ######### \n");
#ifdef USE_DSG
                dsgProtectedProc(event_params_rcv.data);
#endif
                break;
            default:
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","DsgEventListener::default\n");
                break;
        }
    
        rmf_osal_event_delete(event_handle_rcv);
    }


}

