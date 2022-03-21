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
#include "mmi_handler.h"
//#include "pfcpluginbase.h"

#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif

#define __MTAG__ VL_CARD_MANAGER




    
cMMI_Handler::cMMI_Handler(CardManager *cm,char *name):CMThread(name)
{
    mmi_handler_mutex = 0;     
    this->cm = cm;
    this->dialog_number = 0;
    retry_count = 0;
    event_queue = 0;
    rmf_osal_mutexNew(&mmi_handler_mutex);
}
        
    
cMMI_Handler::~cMMI_Handler(){

	rmf_osal_mutexDelete(mmi_handler_mutex);

}
    
    
    
void cMMI_Handler::initialize(void){

rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
rmf_osal_eventqueue_handle_t eq;


    rmf_osal_eventqueue_create ( (const uint8_t* )"cMMI_Handler" ,
		&eq);

//PodMgrEventListener    *listener;


     rmf_osal_eventmanager_register_handler(
		em,
		eq, RMF_OSAL_EVENT_CATEGORY_CardManagerMMIEvents
		);

     rmf_osal_eventmanager_register_handler(
		em,
		eq, RMF_OSAL_EVENT_CATEGORY_CardManagerAIEvents
		);
         
        this->event_queue = eq;
         
        

}


 


void cMMI_Handler::run(void )
{
    cMMIControl *newMMIFlow=NULL;
    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    EventCableCard_MMI *event_mmi;
    EventCableCard_AI  *event_ai;
    rmf_osal_event_handle_t event_handle_rcv;
    rmf_osal_event_params_t event_params_rcv = {0};    	
    rmf_osal_event_category_t event_category;
    uint32_t  event_type;
              
    while (1) 
    {
        int result = 0;

        rmf_osal_eventqueue_get_next_event( event_queue,
		&event_handle_rcv, &event_category, &event_type, &event_params_rcv);
        
        rmf_osal_mutexAcquire(mmi_handler_mutex);
        if(this->cm->pCMIF->IsMMIhandlerPresent() == 0)
        {
            switch (event_category)
            {
                case RMF_OSAL_EVENT_CATEGORY_CardManagerMMIEvents:
                    event_mmi = (EventCableCard_MMI *)event_params_rcv.data;
                    if((event_type  == card_mmi_open_mmi_req) && (NULL != event_mmi)){ 
                        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","GOT OPEN_MMI\n");
			if(newMMIFlow)
			{
			    delete newMMIFlow;
			    newMMIFlow = NULL;
			}						
                        newMMIFlow = new cMMIControl;


                        newMMIFlow->setUrl(event_mmi->url,event_mmi->urlLength);
                        //newMMIFlow->dialogNum = 1; //if multi window is not supported ther can be only 1 dialog at a time. but need to make sure if this is not in use
                        newMMIFlow->openMMICnf(1/*newMMIFlow->dialogNum*/, 0, 0);
                        //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","SENDING SERVER_QUERY\n"); 
                        //newMMIFlow->serverQuery(1, newMMIFlow->getUrl(), newMMIFlow->getUrlLength(), 0,0); 
                                
                    }
                    else if((event_type)  == card_mmi_close_mmi_req)
                    {
                        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","GOT CLOSE_MMI\n");
                        if(newMMIFlow)
                        {
                            newMMIFlow->closeMMICnf(1/*newMMIFlow->dialogNum*/, 0); 
                            //VLFREE_INFO2(newMMIFlow);
                            delete newMMIFlow;
                            //Prasad 10/06/2004) - Make the pointer to NULL
                            newMMIFlow = NULL;
                        }    
                        
                    }
                    else if((event_type)  ==  card_mmi_open_mmi_cnf_sent)
                    {
                         
                        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","SENDING SERVER_QUERY\n"); 
                        retry_count = 0;
                        //Prasad 10/06/2004) - Check for NULL pointer
                        if( NULL != newMMIFlow)
                            newMMIFlow->serverQuery(1, newMMIFlow->getUrl(), newMMIFlow->getUrlLength(), 0, 0); 
                    }
                    
                    break; 
                case RMF_OSAL_EVENT_CATEGORY_CardManagerAIEvents:
                    //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","\n\n\n @@@@@@@@############## GOT DATA TO BE DISPLAYED  ########### \n\n\n");
                    event_ai = (EventCableCard_AI *)event_params_rcv.data;
                    if((event_type == card_server_reply) && (event_ai  != NULL))
                    {
                        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","GOT DATA TO BE DISPLAYED\n");
                        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","file length= %x\n", event_ai->fileLength);
                        if(event_ai->fileLength == 0)
                        {
                            //retry once
                            if(retry_count == 0)
                            {
                                retry_count = 1;
                            //    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","SENDING SERVER_QUERY  again\n"); 
                                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," >>>>>>>>>>>>>> Sending Query >>>>>>>>>>>>>>\n");
                            //    newMMIFlow->serverQuery(1, newMMIFlow->getUrl(), newMMIFlow->getUrlLength(), 0,0); 
                            }
                        
                        }
                        else if(event_ai->file)
                        {
                            event_ai->file[(event_ai->fileLength)] = 0;
                             RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s\n ",event_ai->file);    
                        }    
                        
                    
                    }
                    break;     
                default:
                
                    break;
            }
        }
        rmf_osal_mutexRelease(mmi_handler_mutex);
        rmf_osal_event_delete(event_handle_rcv);
    }


}









