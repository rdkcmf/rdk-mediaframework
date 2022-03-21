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
#include "homing.h"

#define __MTAG__ VL_CARD_MANAGER

cHoming::cHoming(CardManager *cm,char *name):CMThread(name)
{
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","cHoming::cHoming()\n");
    event_queue = 0;
    this->cm = cm;
}

cHoming::~cHoming(){}

void cHoming::initialize(void){

    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    rmf_osal_eventqueue_handle_t eventqueue_handle;
    rmf_osal_eventqueue_create ( (const uint8_t* ) "cHoming",
		&eventqueue_handle);
     rmf_osal_eventmanager_register_handler(
		em,
		eventqueue_handle, RMF_OSAL_EVENT_CATEGORY_HOMING
		);

     rmf_osal_eventmanager_register_handler(
		em,
		eventqueue_handle, RMF_OSAL_EVENT_CATEGORY_SYSTEM
		);

//PodMgrEventListener    *listener;

        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","cHoming::initialize()\n");
        this->event_queue = eventqueue_handle;
        //listener = new PodMgrEventListener(cm, eq,"ResourceManagerThread);


        //listener->start();


}

void cHoming::run(void )
{
    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    rmf_osal_event_handle_t event_handle_rcv;
    rmf_osal_event_params_t event_params_rcv = {0}; 
    rmf_osal_event_category_t event_category;
    uint32_t event_type;
    rmf_Error err;
	
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","###########   HomingEventListener::Run() ################\n");
    hominginit();
    while (1)
    {
        int result = 0;

        err = rmf_osal_eventqueue_get_next_event( event_queue,
		&event_handle_rcv, &event_category, &event_type, &event_params_rcv);

        if (RMF_SUCCESS != err)
            continue;

        switch (event_category)
        {
            case RMF_OSAL_EVENT_CATEGORY_HOMING:
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","HomingEventListener::PFC_EVENT_CATEGORY_HOMING\n");
                homingProc(event_params_rcv.data);
                break;
            /*case PFC_EVENT_CATEGORY_INPUT:
            if((int)event->get_type() == pfcEventInput::input_Power)
            {
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","\n\n##############################################################\n");
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","##############################################################\n");
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","HomingEventListener::Event Received PFC_EVENT_CATEGORY_INPUT\n");
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","##############################################################\n");
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","##############################################################\n\n");
                /// vlHomingHandlePowerState();
            }
                break;*/
            case RMF_OSAL_EVENT_CATEGORY_SYSTEM:
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","\n\n##############################################################\n");
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","##############################################################\n");
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","HomingEventListener::Event Received RMF_OSAL_EVENT_CATEGORY_SYSTEM.  Event Type = %d\n", event_type);
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","##############################################################\n");
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","##############################################################\n\n");
                if((int)event_type == system_power_standby)
                {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","HomingEventListener:: >>>>>>> Entered into system_power_standby <<<<<<<<<\n");
                    vlHomingHandlePowerState(0);
                }
                else if((int)event_type == system_power_on)
                {

                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","HomingEventListener::>>>>>>> Changed to system_power_on <<<<<<<<<\n");

                    vlHomingHandlePowerState(1);
                }
                break;
            default:
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","HomingEventListener::default\n");
                break;
        }

        rmf_osal_event_delete(event_handle_rcv);
    }
}
