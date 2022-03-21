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

/**
 * @file This file contains function which initializes and starts the thread for snmp
 * boot event handler.
 */

#include "ip_types.h"
#include "vlSnmpBootEventHandler.h"
#include "SnmpIORM.h"
#include "vlpluginapp_haldsgapi.h"
#include "utilityMacros.h"
#include "cardUtils.h"
#include "core_events.h"
#define __MTAG__ VL_SNMP_MANAGER
#include "ipcclient_impl.h"

#ifdef __cplusplus
extern "C" {
#endif
#include"docsDevEventTable_enums.h"
#include"snmpAccessInclude.h"

#ifdef __cplusplus
}
#endif

/**
 * @brief This function is a constructor which is used to create the event queue
 * for the SNMP boot handler.
 */
CvlSnmpBootEventHandler::CvlSnmpBootEventHandler():CMThread("vlSnmpBootEventHandler")
{
     
	 //= new pfcEventQueue("CvlSnmpBootEventHandler");
     //VLALLOC_INFO2(m_pEvent_queue, sizeof(pfcEventQueue));
        m_pEvent_queue = 0;
	 rmf_osal_eventqueue_create((const uint8_t *) "CvlSnmpBootEventHandler", &m_pEvent_queue);
}

/**
 * @brief This function is a destructor which is used to delete the event queue
 * for the SNMP boot handler.
 */
CvlSnmpBootEventHandler::~CvlSnmpBootEventHandler()
{
    //VLFREE_INFO2(m_pEvent_queue);
	rmf_osal_eventqueue_delete(m_pEvent_queue);
}

/**
 * @brief This function is used to initialize and register the event manager handler.
 * 
 * @return By default it returns 1 (as success condition) 
 */
int CvlSnmpBootEventHandler::initialize(void)
{
    // Moved queue registrations here.
    rmf_osal_eventmanager_handle_t em = 0;// vivek IPC_CLIENT_get_pod_event_manager();
    rmf_osal_eventmanager_register_handler(em, m_pEvent_queue, RMF_OSAL_EVENT_CATEGORY_BOOTMSG_DIAG);
    return 1;
}

/**
 * @brief This function is used to check the completion of init function and 
 * starts the thread.
 *
 * @return By default it returns 0.
 */
int CvlSnmpBootEventHandler::init_complete(void)
{
    SNMP_DEBUGPRINT(" \n\n CvlSnmpBootEventHandler::init_complete..\n");
    // Moved queue registrations to initialize().
		
    start();
    return 0;

}

/**
 * @brief This function is used to check the default handler.
 *
 * @return By default it returns 1 (as success condition)
 */
int CvlSnmpBootEventHandler::is_default ()
{
    SNMP_DEBUGPRINT("CvlSnmpBootEventHandler::is_default\n");
    return 1;
}

/**
 * @brief This function is used as thread routine whose main purpose is to pull
 * the events from event queue and dispatches them with proper ERRORCODE.
 *
 * @return None
 */
void CvlSnmpBootEventHandler::run()
{
    void* pEventData = NULL;
    rmf_osal_eventmanager_handle_t em = 0;// vivek IPC_CLIENT_get_pod_event_manager();
    //pfcEventBase *event=NULL;
    int result = 0;
    rmf_osal_event_handle_t event_handle;
    rmf_osal_event_params_t event_params = {0};
    rmf_osal_event_category_t event_category;
    uint32_t event_type;	

    while (1)
    {
        SNMP_DEBUGPRINT("\n CvlSnmpBootEventHandler ::: Start 1\n");
        if(m_pEvent_queue == NULL)
        {
            SNMP_DEBUGPRINT("\n FATAL FATAL: npEvent-queue is NULL  \n");
            break;
        }

	 rmf_osal_eventqueue_get_next_event(m_pEvent_queue, &event_handle, &event_category, &event_type, &event_params);
        SNMP_DEBUGPRINT("\n rmf_osal_eventqueue_get_next_event() :: event_params %0x \n", event_params);
//        pEventData = event_params.data;
//        RMF_BOOTMSG_DIAG_EVENT_INFO *pBootMsgEventInfo = (RMF_BOOTMSG_DIAG_EVENT_INFO*)pEventData;
        SNMP_DEBUGPRINT("CvlSnmpBootEventHandler   :run:event=0x%x,category=%d;type=%d\n",event_params, event_category, event_type);
        switch (event_category)
        {
            case RMF_OSAL_EVENT_CATEGORY_BOOTMSG_DIAG:
            {
                switch (event_type)
                {
                    case RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD_OOB:
                    {
                        switch(event_params.data_extension)//(pBootMsgEventInfo->msgCode)
                        {
                            case RMF_BOOTMSG_DIAG_MESG_CODE_COMPLETE:
                            {
                                EdocsDevEventhandling(ERRORCODE_B14_0);
                            }
                            break;
                            
                            default:
                            {
                                
                            }
                            break;
                        }
                    }
                    break;
                    
                    case RMF_BOOTMSG_DIAG_EVENT_TYPE_OCAP:
                    {
                        switch(event_params.data_extension)//(pBootMsgEventInfo->msgCode)
                        {
                            case RMF_BOOTMSG_DIAG_MESG_CODE_START:
                            {
                                EdocsDevEventhandling(ERRORCODE_B02_0);
                            }
                            break;
                            
                            case RMF_BOOTMSG_DIAG_MESG_CODE_COMPLETE:
                            {
                                EdocsDevEventhandling(ERRORCODE_B02_0);
                                rmf_osal_threadSleep(2000,0);
                                EdocsDevEventhandling(ERRORCODE_B21_0);
                            }
                            break;
                            
                            case RMF_BOOTMSG_DIAG_MESG_CODE_ERROR:
                            {
                                EdocsDevEventhandling(ERRORCODE_B22_0);
                            }
                            break;
                            
                            default:
                            {
                                
                            }
                            break;
                        }
                    }
                    break;
                    
                    case RMF_BOOTMSG_DIAG_EVENT_TYPE_OCAP_XAIT:
                    {
                        switch(event_params.data_extension)//(pBootMsgEventInfo->msgCode)
                        {
                            case RMF_BOOTMSG_DIAG_MESG_CODE_COMPLETE:
                            {
                                EdocsDevEventhandling(ERRORCODE_B23_0);
                            }
                            break;
                            
                            case RMF_BOOTMSG_DIAG_MESG_CODE_ERROR:
                            {
                                EdocsDevEventhandling(ERRORCODE_B24_0);
                            }
                            break;
                            
                            default:
                            {
                                
                            }
                            break;
                        }
                    }
                    break;
                    
                    case RMF_BOOTMSG_DIAG_EVENT_TYPE_OCAP_MONITOR:
                    {
                        switch(event_params.data_extension)//(pBootMsgEventInfo->msgCode)
                        {
                            case RMF_BOOTMSG_DIAG_MESG_CODE_COMPLETE:
                            {
                                EdocsDevEventhandling(ERRORCODE_B25_0);
                            }
                            break;
                            
                            case RMF_BOOTMSG_DIAG_MESG_CODE_ERROR:
                            {
                                EdocsDevEventhandling(ERRORCODE_B26_0);
                            }
                            break;
                            
                            default:
                            {
                                
                            }
                            break;
                        }
                    }
                    break;
                    
                    default:
                    {
                        SNMP_DEBUGPRINT("\n BOOTMSG_DIAG Event %d Received\n", event_type);
                    }
                    break;
                }
            }
            break;//PFC_EVENT_CATEGORY_CARD_MANAGER
            
            default:
            {
            }
            break;
        }

		rmf_osal_event_delete(event_handle);
          //#endif //if 0
    }//while

    SNMP_DEBUGPRINT("\n CvlSnmpBootEventHandler ::: End::::::::::: pfcEventManager for TLV217 parsing \n");
    /** End of pfcEventMgr For the TLV 217 parsing and config the SNMP*/
}
