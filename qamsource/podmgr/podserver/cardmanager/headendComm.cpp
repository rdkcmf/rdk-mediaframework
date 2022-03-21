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
#include "headendComm.h"
//#include "pmt.h"
#include "poddriver.h"
#include "cm_api.h"
//#include <string.h>
#define __MTAG__ VL_CARD_MANAGER


cHeadEndComm::cHeadEndComm(CardManager *cm,char *name):CMThread(name)
{    
    //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","cCardRes::cCardRes()\n");
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","Entered ..... cHeadEndComm::cHeadEndComm  ###################### \n");
    this->cm = cm;
    event_queue = 0;  
    
}
        
    
cHeadEndComm::~cHeadEndComm(){
 event_queue = 0;
}
    
    
    
void cHeadEndComm::initialize(void)
{

    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    rmf_osal_eventqueue_handle_t eventqueue_handle;
    rmf_osal_eventqueue_create ( (const uint8_t* ) "cHeadEndComm",
		&eventqueue_handle);

//    PodMgrEventListener *listener;
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","Entered ..... cHeadEndComm::initialize ###################### \n");
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","cHeadEndComm::initialize()\n");
	
    rmf_osal_eventmanager_register_handler(
         em, eventqueue_handle, RMF_OSAL_EVENT_CATEGORY_HEAD_END_COMM);
     
    this->event_queue = eventqueue_handle;
    //listener = new PodMgrEventListener(cm, eq,"ResourceManagerThread);

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","cHeadEndComm::initialize() eq=%x\n",eventqueue_handle);
    //listener->start();
    

}

//static vlCableCardCertInfo_t CardCertInfo;
void cHeadEndComm::run(void )
{
    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    rmf_osal_event_handle_t event_handle_rcv;
    rmf_osal_event_params_t event_params_rcv = {0}; 
    rmf_osal_event_category_t event_category;
    uint32_t event_type;
//    vlCableCardCertInfo_t CardCertInfo;

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","Entered ..... cHeadEndComm::run  ###################### \n");
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","cHeadEndComm::Run()\n");

    headEndComm_init();
    while (1)
    {
        int result = 0;

        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","cHeadEndComm::run  waiting for Event #################\n");
        rmf_osal_eventqueue_get_next_event( event_queue,
		&event_handle_rcv, &event_category, &event_type, &event_params_rcv);
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","############# cHeadEndComm::After run  waiting for Event #################\n");
        switch (event_category)
        {
            case RMF_OSAL_EVENT_CATEGORY_HEAD_END_COMM:
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","cHeadEndComm::PFC_EVENT_CATEGORY_HEAD_END_COMM\n");
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," ########### headEndCommProc:RMF_OSAL_EVENT_CATEGORY_HEAD_END_COMM ######### \n");
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," ########### calling headEndCommProc ########## \n");
                if ( event_params_rcv.data )
                {
                     headEndCommProc(event_params_rcv.data);
                }
//                GetCableCardCertInfo(&CardCertInfo);
                break;
            default:
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","headEndCommProc::default\n");
                break;
        }

        rmf_osal_event_delete(event_handle_rcv);
    }


}
