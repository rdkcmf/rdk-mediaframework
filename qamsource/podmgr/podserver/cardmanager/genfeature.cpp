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
#include "genfeature.h"

#define __MTAG__ VL_CARD_MANAGER


    
cGenFeatureControl::cGenFeatureControl(CardManager *cm,char *name):CMThread(name)
{
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","cGenFeatureControl::cGenFeatureControl()\n");
    event_queue = 0;
    this->cm = cm;
    
    
}
        
    
cGenFeatureControl::~cGenFeatureControl(){}
    
    
    
void cGenFeatureControl::initialize(void){

    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    rmf_osal_eventqueue_handle_t eventqueue_handle;
    rmf_osal_eventqueue_create ( (const uint8_t* ) "cGenFeatureControl",
		&eventqueue_handle);

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","cGenFeatureControl::initialize()\n");
    rmf_osal_eventmanager_register_handler(
		em,
		eventqueue_handle, RMF_OSAL_EVENT_CATEGORY_GEN_FEATURE
		);
//PodMgrEventListener    *listener;

        this->event_queue = eventqueue_handle;
        //listener = new PodMgrEventListener(cm, eq,"ResourceManagerThread);
        
        //listener->start();
}


 


void cGenFeatureControl::run(void ){
    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    rmf_osal_event_handle_t event_handle_rcv;
    rmf_osal_event_params_t event_params_rcv = {0}; 
    rmf_osal_event_category_t event_category;
    uint32_t event_type;
    LCSM_EVENT_INFO *pEventInfo;

     //    static int tagArray[3][7]={ 0x9f, 0x98 ,0x02,0x9f, 0x98, 0x03, 0x9f, 0x98, 0x04, 0x9f, 0x98, 0x05, 0x9f,0x98, 0x06,0x9f, 0x98 ,0x07,0x9f,0x98,0x08};
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","ScGenFeatureControlEventListener::Run()\n");
    feature_init();
    while (1)
    {
        int result = 0;

        rmf_osal_eventqueue_get_next_event( event_queue,
		&event_handle_rcv, &event_category, &event_type, &event_params_rcv);
        
        switch (event_category)
        {
            case RMF_OSAL_EVENT_CATEGORY_GEN_FEATURE:
                if( event_params_rcv.data )
                {
                     pEventInfo = (LCSM_EVENT_INFO *) event_params_rcv.data;
     				
                     RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","cGenFeatureControlEventListener::PFC_EVENT_CATEGORY_GEN_FEATURE\n");
                     if((pEventInfo->event)  == POD_FEATURE_PARAMS) // this is feature params from POD
                     { 
                     
                         RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","POD_FEATURE_PARAMS - event\n");
                     }
                     else if((pEventInfo->event)  ==  POD_FEATURE_LIST_REQ) //feature list (only feature IDs requested by POD)
                     {
                         RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","POD_FEATURE_LIST_REQ - event\n");
                     
                     }
                     else if((pEventInfo->event)  ==   POD_FEATURE_LIST_X)  // feature list from POD (only feature IDs)
                     {
                         RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","POD_FEATURE_LIST_X - event\n");
                     
                     }
                     else if((pEventInfo->event)  ==  POD_FEATURE_LIST_CHANGE ) //indicates that the POD feature List has changed
                     {
                         RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","POD_FEATURE_LIST_CHANGE - event\n");
                     
                     }
                     else if((pEventInfo->event)  ==  POD_FEATURE_PARAMS_REQ ) // POD requests for the feature params
                     {
                     
                         RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","POD_FEATURE_PARAMS_REQ - event\n");
                     }
                     featureProc(pEventInfo);
                }
                break;
            default:
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","cGenFeatureControlEventListener::default\n");
                break;
        }

        rmf_osal_event_delete(event_handle_rcv);
    }


}




/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Implementing class cFeatureControl

cFeatureControl::cFeatureControl()
{
    //this->cable_url_array = 0;
    //this->purchase_pin_chr = 0;
    //this->p_c_channel_value = 0;
    //this->p_c_pinData = 0;
    hostFeatureList = nullptr;
    cardFeatureList = nullptr;
}


 

cFeatureControl::~cFeatureControl()
{
}



void cFeatureControl::getHostFeatureList(uint8_t &featureList)   //complete list
{

//int i=0,j=0;

    


}
    
void cFeatureControl::getHostParam(int featureID, uint8_t &) //returns the values pertaining to this feature ID
{

}

bool cFeatureControl::setHostParam(int featureID, uint8_t &) //used to update the features by sending the APDU to CableCard //
{
    return 0;
}

void cFeatureControl::NotifyOCAP() //OCAP App seems to need to approve any feature change requested by the Host or POD
{

}


void cFeatureControl::BuildUpdatedFeatureList(uint8_t featureList) //list of updated features
{

    if(this->fc_rfOutputObj.IsSupported()  &&  (this->fc_rfOutputObj.GetFeatureState()  == CM_FEATURE_CHANGED))
    {
    
        
    }        

}


void cFeatureControl::BuildAllFeaturesList(uint8_t featureList) //list of all supported features
{

}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//implementing class cGenericFeatureBaseClass

 cGenericFeatureBaseClass::cGenericFeatureBaseClass()
 {
 buffer = NULL;
    featureID = 0xff;
    supported = 0;
    state = CM_FEATURE_DEFAULT;
    curr_bit_index = 0;
 }
    
cGenericFeatureBaseClass::cGenericFeatureBaseClass(uint8_t *destBuffer)
{
    buffer = destBuffer;
    featureID = 0xff;
    supported = 0;
    state = CM_FEATURE_DEFAULT;
    curr_bit_index = 0;
}


cGenericFeatureBaseClass::~cGenericFeatureBaseClass()
{
    buffer = 0;
}


    
bool    cGenericFeatureBaseClass::IsSupported()  // indicated whether host cares about this parameter 
{
    return this->supported;
}
    
void    cGenericFeatureBaseClass::Set_FeatureSupported()
{
    this->supported = 1;
}


void    cGenericFeatureBaseClass::Clear_FeatureSupported()
{

    this->supported =  0;
}


uint8_t cGenericFeatureBaseClass::GetFeatureID()
{
    return this->featureID;

}


void    cGenericFeatureBaseClass::SetFeatureState( State_t   state)
{
    this->state = state;

}
    

State_t    cGenericFeatureBaseClass::GetFeatureState()
{
    return this->state;

}


void cGenericFeatureBaseClass::skip_bits(int n)
{
    curr_bit_index += n;
}



int  cGenericFeatureBaseClass::put_bits(int32_t value,int n)
{
    int i;
    
    unsigned long in = value;
    uint8_t  temp;
    if(buffer == 0)
        return CM_BUFFER_NOT_ALLOCATED;
    for(i=0; i<n; i++) {
         
        int byte = curr_bit_index / 8;
        int bit = 7 - (curr_bit_index % 8);    /* msb first */
        temp = 1 << bit;
        if(value & 0x1)
        {
            buffer[byte] |= temp;    
        
        }
        else
        {
            buffer[byte] &= (0xff  - (temp));  //clearing the bit 
        }
        in >>= 1;
         
        curr_bit_index++;
        
    }
    return 0;
}



    

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//implementing class cRfOutput

 
cRfOutput::cRfOutput() 
{
this->rf_outputChannel = 0;
    this->rf_outputChannelUI = 0;
}

cRfOutput::cRfOutput(uint8_t rf_outputChannel, uint8_t  rf_outputChannelUI)
{
    this->rf_outputChannel = rf_outputChannel;
    this->rf_outputChannelUI = rf_outputChannelUI;
    this->Set_FeatureSupported();
    
}


cRfOutput::cRfOutput(uint8_t rf_outputChannel, uint8_t  rf_outputChannelUI, uint8_t *bufferPtr):cGenericFeatureBaseClass(bufferPtr)
{
    this->rf_outputChannel = rf_outputChannel;
    this->rf_outputChannelUI = rf_outputChannelUI;
    this->Set_FeatureSupported();
    
    
}

cRfOutput::~cRfOutput()
{

}

 




 





///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

 







