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

#include "rmf_osal_event.h"

#include "cmevents.h"
#include "rdk_debug.h"

rmf_osal_event_category_t GetCategory(unsigned queue)
{

    //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", " Entered pfcEventCableCard::GetCategory \n");    
    if(queue == POD_RSMGR_QUEUE){
        return RMF_OSAL_EVENT_CATEGORY_RS_MGR;
    }
    if(queue == POD_APPINFO_QUEUE){
        return RMF_OSAL_EVENT_CATEGORY_APP_INFO;
    }
    if(queue == POD_MMI_QUEUE){
        return RMF_OSAL_EVENT_CATEGORY_MMI;
    }
    if(queue == POD_HOMING_QUEUE)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ##### GetCategory POD_HOMING_QUEUE ######## \n");
        return RMF_OSAL_EVENT_CATEGORY_HOMING;
    }
    if(queue == POD_XCHAN_QUEUE){
        return RMF_OSAL_EVENT_CATEGORY_XCHAN;
    }
    if(queue == POD_SYSTIME_QUEUE){
        return RMF_OSAL_EVENT_CATEGORY_SYSTIME;
    }
    if(queue == POD_HOST_QUEUE){
        return RMF_OSAL_EVENT_CATEGORY_HOST_CONTROL;
    }
    if(queue == POD_CA_QUEUE){
        return RMF_OSAL_EVENT_CATEGORY_CA;
    }
    if(queue == POD_CARD_RES_QUEUE){
        return RMF_OSAL_EVENT_CATEGORY_CARD_RES;
    }
    if(queue == POD_CARD_MIB_ACC_QUEUE){
        return RMF_OSAL_EVENT_CATEGORY_CARD_MIB_ACC;
    }
    if(queue == POD_HOST_ADD_PRT_QUEUE){
        return RMF_OSAL_EVENT_CATEGORY_HOST_APP_PRT;
    }
    if(queue == POD_HEAD_END_COMM_QUEUE){
        return RMF_OSAL_EVENT_CATEGORY_HEAD_END_COMM;
    }
    if(queue == POD_IPDIRECT_UNICAST_COMM_QUEUE){
        return RMF_OSAL_EVENT_CATEGORY_IPDIRECT;
    }
    if(queue == POD_DSG_QUEUE){
        return RMF_OSAL_EVENT_CATEGORY_DSG;
    }
    if(queue == POD_CPROT_QUEUE){
        return RMF_OSAL_EVENT_CATEGORY_CP;
    }
    if(queue == POD_SAS_QUEUE){
        return RMF_OSAL_EVENT_CATEGORY_SAS;
    }
    
    if(queue == POD_SYSTEM_QUEUE )
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ######## POD_SYSTEM_QUEUE ######### \n");
        return RMF_OSAL_EVENT_CATEGORY_SYS_CONTROL;
    }
    if(queue == POD_FEATURE_QUEUE ){
        return RMF_OSAL_EVENT_CATEGORY_GEN_FEATURE;
    }
    if(queue == POD_DIAG_QUEUE ){
        return RMF_OSAL_EVENT_CATEGORY_GEN_DIAGNOSTIC;
    }
    if(queue == POD_MSG_QUEUE){
        return RMF_OSAL_EVENT_CATEGORY_CARD_STATUS;
    }
    if(queue == GATEWAY_QUEUE)
    {
        return RMF_OSAL_EVENT_CATEGORY_GATEWAY;
    }
    if(queue == POD_ERROR_QUEUE){
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ####### POD_ERROR_QUEUE ######### \n");
        return RMF_OSAL_EVENT_CATEGORY_POD_ERROR;
    }
    //for other values do a default one
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," ######## Nothing ######### \n");
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","EventCableCard::GetCategory(): ERROR: Received Event in %d Queue returning PFC_EVENT_CATEGORY_NONE\n", queue);
return RMF_OSAL_EVENT_CATEGORY_NONE;
}



