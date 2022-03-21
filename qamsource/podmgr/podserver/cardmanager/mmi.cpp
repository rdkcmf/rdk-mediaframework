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
#include "mmi.h"

#include <string.h>
#include "rmf_osal_mem.h"
#include "rmf_osal_sync.h"

#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif

#define __MTAG__ VL_CARD_MANAGER


//DECLARE_LOG_APPTYPE (LOG_APPTYPE_OCAP_PFC_PLUGINS_CORESERVICES_CARDMANAGER);




    
cMMI::cMMI(CardManager *cm,char *name):CMThread(name)
{
    mmiMutex = 0;
     rmf_osal_mutexNew(&mmiMutex);
    this->cm = cm;	
    this->event_queue = 0;
}
        
    
cMMI::~cMMI(){

     rmf_osal_mutexDelete(mmiMutex);


}
    
    
    
void cMMI::initialize(void){

    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    rmf_osal_eventqueue_handle_t eventqueue_handle;
	
    rmf_osal_eventqueue_create ( (const uint8_t* ) "cMMI",
		&eventqueue_handle);
    rmf_osal_eventmanager_register_handler(
		em,
		eventqueue_handle, RMF_OSAL_EVENT_CATEGORY_MMI
		);

     this->event_queue = eventqueue_handle;
}


 


void cMMI::run(void ){

    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    rmf_osal_event_handle_t event_handle_rcv;
    rmf_osal_event_params_t event_params_rcv = {0}; 
    rmf_osal_event_category_t event_category;
    uint32_t event_type;
    LCSM_EVENT_INFO *pEventInfo;
    EventCableCard_MMI *peventMMI;	

    mmi_init();
    /*******************************temporarily  for testing NDS POD Hannah*********************************************
     
    cMMIControl *newMMIFlow;
        newMMIFlow = new cMMIControl;

    
    
     
    ******************************************************************************************************************/
                        
    while (1)
    {
        int result = 0;

        rmf_osal_eventqueue_get_next_event( event_queue,
		&event_handle_rcv, &event_category, &event_type, &event_params_rcv);
        
        switch (event_category)
        {
            case RMF_OSAL_EVENT_CATEGORY_MMI:

                if (event_params_rcv.data == NULL)
                {
                     break;
                }                

                pEventInfo = (LCSM_EVENT_INFO *) event_params_rcv.data;
                if((pEventInfo->event)  == POD_OPEN_MMI_REQUEST){ 
                    
                        int dataLength = pEventInfo->dataLength;
                        int i;
                        uint8_t  *ptr = (uint8_t *)pEventInfo->dataPtr;

                        rmf_osal_event_handle_t event_handle;
                        rmf_osal_event_params_t event_params = {0};
			
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","POD_OPEN_MMI_REQUEST event : dataLen=%d\n",dataLength);
                        for(i =0; i < dataLength; i++)
                            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%x",*ptr++);
                         
                                        
                    
                    //send card_mmi_open_mmi_req event 
                        rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(EventCableCard_MMI),(void **)&peventMMI);
                        
                        peventMMI->urlLength = dataLength - 3;
                        peventMMI->url = new uint8_t[peventMMI->urlLength];
                        
                        memcpy(peventMMI->url ,(uint8_t *)(pEventInfo->dataPtr)+3,peventMMI->urlLength);
                        /****************************************************************************************************************** 
                        {
                        
                        
                        newMMIFlow->setUrl( peventMMI->url,peventMMI->urlLength);
                        newMMIFlow->dialogNum = 1;
                        newMMIFlow->openMMICnf(1, 0, 0);
                        ////VLFREE_INFO2(newMMIFlow);
                        //delete newMMIFlow;
                        }
                         ******************************************************************************************************************/
                    
                        peventMMI->dialogNum = -1;

                        event_params.priority = 0; //Default priority;
                        event_params.data = (void *)peventMMI;
                        event_params.data_extension = 0;
                        event_params.free_payload_fn = EventCableCard_MMI_free_fn;
                        rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_CardManagerMMIEvents, card_mmi_open_mmi_req, 
                    		&event_params, &event_handle);

                        rmf_osal_eventmanager_dispatch_event(em, event_handle);
                    
                            //fsend mmi_cnf... temporarily done from here
                        
                        
                        //Modified  by Prasad (10/04/2008).    dataPtr is allocated with malloc api.                    
                        //delete [] (uint8_t *)(event->eventInfo.dataPtr);
						rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pEventInfo->dataPtr);
                        break;        
                                    
                } 
                else if((pEventInfo->event)  == POD_CLOSE_MMI_REQUEST)
                {
                    rmf_osal_event_handle_t event_handle;
                    rmf_osal_event_params_t event_params = {0};
					
                    rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(EventCableCard_MMI),(void **)&peventMMI);
                    memset(peventMMI, 0, sizeof(*peventMMI));

                    peventMMI->dialogNum =     pEventInfo->x;

                    event_params.priority = 0; //Default priority;
                    event_params.data = (void *)peventMMI;
                    event_params.data_extension = 0;
                    event_params.free_payload_fn = EventCableCard_MMI_free_fn;
                    rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_CardManagerMMIEvents, card_mmi_close_mmi_req, 
                		&event_params, &event_handle);

                    rmf_osal_eventmanager_dispatch_event(em, event_handle);

                    /******************************************temp*********************
                    newMMIFlow->closeMMICnf(1, 0); //temp code
                    if(newMMIFlow->url)
                    {
                        //VLFREE_INFO2(newMMIFlow->url);
                        delete newMMIFlow->url;
                    }
                    ****************************************************************/
                    break;
                }
                else if(((pEventInfo->event)  == POD_APDU) && this->cm->IsOCAP())
                {
                    //Send Raw Response APDU to OCAP
                    if(this->cm->pCMIF->cb_fn)
                        //callback
                    {
                    
                        //lock mutex
                        rmf_osal_mutexAcquire(mmiMutex);
                        this->cm->pCMIF->cb_fn ((uint8_t *)pEventInfo->y,(uint16_t)pEventInfo->z );
                        
                        // unlock mutex
                        rmf_osal_mutexRelease(mmiMutex);
                    }
                    else //send event
                    {
                        EventCableCard_RawAPDU *pEventApdu;
                        //do a dispatch event here
                        rmf_osal_event_handle_t event_handle;
                        rmf_osal_event_params_t event_params = {0};
       
         		   rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(EventCableCard_RawAPDU),(void **)&pEventApdu);
                        //do a dispatch event here
    
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," >>>>>> Posting the event RMF_OSAL_EVENT_CATEGORY_CM_RAW_APDU \n");

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
                    break;
                }
                rmf_osal_mutexAcquire(mmiMutex);
                mmiProc((void *)pEventInfo);
                rmf_osal_mutexRelease(mmiMutex);
                
                if((pEventInfo->event)  == POD_OPEN_MMI_CONF/*POD_OPEN_MMI_REQUEST*/){ 

                    rmf_osal_event_handle_t event_handle;
					
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","after open_mmi_cnf\n");

                    rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_CardManagerMMIEvents, card_mmi_open_mmi_cnf_sent, 
                		NULL, &event_handle);

                    rmf_osal_eventmanager_dispatch_event(em, event_handle);
                    
                    /*******************************temporarily  for testing NDS POD Hannah*********************************************
     
                    newMMIFlow->serverQuery(1, newMMIFlow->url, newMMIFlow->urlLength, 0,0);
                    ****************************************************************/
                }
                
                break;
            default:
             
                break;
        }

        rmf_osal_event_delete(event_handle_rcv);
    }


}





//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Implementing class cMMIControl


cMMIControl::cMMIControl(){
    this->url = 0;
    this->urlLength = 0;
    this->dialogNum = 0;
    this->ltsid  = 0;
    this->streamType = 0;

}

cMMIControl::cMMIControl(uint8_t *urlStr,uint16_t length ){
     
    this->url = new uint8_t[length];


/*    memcpy(this->url, urlStr, length);*/
    memcpy(this->url, urlStr, length);
    this->urlLength = length;
    this->dialogNum = -1;
    this->ltsid  = 0;
    this->streamType = 0;
    
}

cMMIControl::~cMMIControl(){
    if(this->url != 0)
    {
        //VLFREE_INFO2(this->url);
        delete [] this->url;
    }
}


void cMMIControl::openMMICnf(int dialogNum, int status, int ltsid)
{
    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    rmf_osal_event_handle_t event_handle;
    rmf_osal_event_params_t event_params = {0}; 
    LCSM_EVENT_INFO *pEventInfo;
    
    rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(LCSM_EVENT_INFO),(void **)&pEventInfo);

    pEventInfo->event = POD_OPEN_MMI_CONF;
    pEventInfo->x     = dialogNum;
    pEventInfo->dataPtr     = NULL;
    pEventInfo->dataLength = 0;
    
    this->dialogNum      = dialogNum;
    pEventInfo->y     = status;

    event_params.priority = 0; //Default priority;
    event_params.data = (void *)pEventInfo;
    event_params.data_extension = 0;
    event_params.free_payload_fn = podmgr_freefn;
    rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_MMI, POD_OPEN_MMI_CONF, 
		&event_params, &event_handle);

    rmf_osal_eventmanager_dispatch_event(em, event_handle);
	
}

void cMMIControl::closeMMICnf(int dialogNum, int ltsid ){

    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    rmf_osal_event_handle_t event_handle;
    rmf_osal_event_params_t event_params = {0}; 
    LCSM_EVENT_INFO *pEventInfo;
    
    rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(LCSM_EVENT_INFO),(void **)&pEventInfo);

    pEventInfo->event = POD_CLOSE_MMI_CONF;
    pEventInfo->x     = dialogNum;
    pEventInfo->dataPtr     = NULL;
    pEventInfo->dataLength = 0;
	
    event_params.priority = 0; //Default priority;
    event_params.data = (void *)pEventInfo;
    event_params.data_extension = 0;
    event_params.free_payload_fn = podmgr_freefn;

    rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_MMI, POD_CLOSE_MMI_CONF, 
		&event_params, &event_handle);

    rmf_osal_eventmanager_dispatch_event(em, event_handle);

}

int cMMIControl::serverQuery(int dialogNum, uint8_t *url, uint16_t urlLength, uint8_t *header, uint16_t headerLength)
{
uint8_t *payload, *ptr;

    rmf_osal_event_handle_t event_handle;
    rmf_osal_event_params_t event_params = {0};
    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","cMMIControl::serverQuery: urlLen=%x\n",urlLength);
            // Prepare packet
            payload =  new uint8_t[5 + headerLength + urlLength];


            if (payload == NULL) {
                return -ENOMEM;
            }
            ptr = payload;
            *ptr = this->dialogNum;
            ptr++;
            *(ptr++) = (headerLength & 0xff00) >> 8;
            *(ptr++) = (headerLength & 0x00ff);
            if (headerLength > 0) {
/*                memcpy(ptr, header, headerLength);*/
                rmf_osal_memcpy(ptr, header, headerLength,
                		5 + headerLength + urlLength-3, headerLength );
                ptr += headerLength;
            }
            *(ptr++) = (urlLength & 0xff00) >> 8;
            *(ptr++) = (urlLength & 0x00ff);
            if (urlLength > 0) {
/*                memcpy(ptr, url, urlLength);*/
                rmf_osal_memcpy(ptr, url, urlLength, 
                		5 + headerLength + urlLength-5, urlLength );
            }

        LCSM_EVENT_INFO *pEventInfo;
        (void) rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(LCSM_EVENT_INFO), (void **) &pEventInfo);        


    pEventInfo->event = POD_SERVER_QUERY;
      
    pEventInfo->dataPtr     = payload;          // address of URL string
    pEventInfo->dataLength = 5 + headerLength + urlLength;


    event_params.priority = 0; //Default priority;
    event_params.data = (void *)pEventInfo;
    event_params.data_extension = sizeof(LCSM_EVENT_INFO);
    event_params.free_payload_fn = podmgr_freefn;
    rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_APP_INFO, POD_SERVER_QUERY, 
		&event_params, &event_handle);

    rmf_osal_eventmanager_dispatch_event(em, event_handle);
    	        
    return 0;
}




void cMMIControl::setUrl(uint8_t *ptr, uint16_t len)
{
    if(this->url)
    {
        //VLFREE_INFO2(this->url);
        delete [] this->url; 
    }
    this->url = new uint8_t[len];        


/*    memcpy(this->url, ptr, len);*/
    memcpy(this->url, ptr, len);
    this->urlLength = len; 

}
    
uint16_t cMMIControl::getUrlLength()
{
    return this->urlLength;

}


uint8_t * cMMIControl::getUrl(/*uint8_t *ptr, uint16_t length*/)
{
    //memcpy(ptr, this->url, length);
    return this->url;
}
 

int    cMMIControl::getDialogNum()
{

    return this->dialogNum;
}




void EventCableCard_MMI_free_fn(void *ptr){

    EventCableCard_MMI *pEvent = (EventCableCard_MMI *)ptr;
     
    if(pEvent)
    {
        if (pEvent->url) delete [] pEvent->url;
        rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pEvent);
    }
                 
}








