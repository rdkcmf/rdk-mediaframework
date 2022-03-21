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
#include "xchan.h"
#include "cm_api.h"
#include <string.h>
#include "vlXchanFlowManager.h"
#include "utilityMacros.h"
#include "xchanResApi.h"
#include "rmf_osal_mem.h"
#include "rmf_osal_sync.h"
#include <semaphore.h>
#include <string.h>
#include <sys/time.h>

#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif

#define __MTAG__ VL_CARD_MANAGER



#ifdef __cplusplus

extern "C" {

#endif
extern int POD_STACK_SEND_ExtCh_Data(unsigned char * data,  unsigned long length, unsigned long flowID);
extern  int HomingCardFirmwareUpgradeGetState();
#ifdef __cplusplus
}

#endif

static char * aFlowNames[] = {"MPEG FLOW", "IP_U FLOW", "IP_M FLOW", "DSG FLOW", "SOCKET FLOW", "IPDM Flow", "IPDU Flow"};

unsigned char cExtChannelThread::MutexValCount;
VL_XCHAN_FLOW_RESULT   vlg_eXchanFlowResult = (VL_XCHAN_FLOW_RESULT)(-1);
#define VL_RETURN_CASE_STRING(x)   case VL_XCHAN_FLOW_RESULT_##x: return #x;
extern "C" char * cExtChannelGetFlowResultName(VL_XCHAN_FLOW_RESULT status)
{
    static char szUnknown[100];

    switch(status)
    {
        VL_RETURN_CASE_STRING(GRANTED              );
        VL_RETURN_CASE_STRING(DENIED_FLOWS_EXCEEDED);
        VL_RETURN_CASE_STRING(DENIED_NO_SERVICE    );
        VL_RETURN_CASE_STRING(DENIED_NO_NETWORK    );
        VL_RETURN_CASE_STRING(DENIED_NET_BUSY      );
        VL_RETURN_CASE_STRING(DENIED_BAD_MAC_ADDR  );
        VL_RETURN_CASE_STRING(DENIED_NO_DNS        );
        VL_RETURN_CASE_STRING(DENIED_DNS_FAILED    );
        VL_RETURN_CASE_STRING(DENIED_PORT_IN_USE   );
        VL_RETURN_CASE_STRING(DENIED_TCP_FAILED    );
        VL_RETURN_CASE_STRING(DENIED_NO_IPV6       );

        default:
        {
        }
        break;
    }

    snprintf(szUnknown,sizeof(szUnknown), "Not Started / Unknown (%d)", status);
    return szUnknown;
}

rmf_osal_Mutex cExtChannelThread::ext_ch_req_flow_lock_mutex;


int                new_flow_cnf_status;
    int                new_flow_cnf_flowID;
    int                del_flow_cnf_flowID;
    int                new_flow_cnf_flows_Remaining;
    VL_IP_IF_PARAMS ipu_flow_params;

cExtChannelThread::cExtChannelThread(CardManager *cm,char *name):CMThread(name)
{
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "cExtChannelThread::cExtChannelThread()\n");
    event_queue = 0;
    extResourceStatus = EXTCH_RESOURCE_CLOSED;

    this->cm = cm;
this->new_flow_cnf_status = 0;
    this->new_flow_cnf_flowID = 0;
    this->del_flow_cnf_flowID = 0;
    this->new_flow_cnf_flows_Remaining = 0;
memset(&this->ipu_flow_params,0,sizeof(this->ipu_flow_params) );
    
    if(MutexValCount == 0)
    {
          rmf_osal_mutexNew(&ext_ch_req_flow_lock_mutex);
    }
    MutexValCount++;	


}


cExtChannelThread::~cExtChannelThread(){

    MutexValCount--;
    if(MutexValCount == 0)
    {
          rmf_osal_mutexDelete(ext_ch_req_flow_lock_mutex);
    }
    	
}



void cExtChannelThread::initialize(void)
{
    vlXchanInitFlowManager();
    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();

    //PodMgrEventListener    *listener;

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "cExtChannelThread::initialize()\n");

    rmf_osal_eventqueue_handle_t eventqueue_handle;
    rmf_osal_eventqueue_create ( (const uint8_t* ) "cExtChannelThread",
		&eventqueue_handle);
    rmf_osal_eventmanager_register_handler(
		em,
		eventqueue_handle, RMF_OSAL_EVENT_CATEGORY_XCHAN
		);
    rmf_osal_eventmanager_register_handler(
		em,
		eventqueue_handle, RMF_OSAL_EVENT_CATEGORY_POD_ERROR
		);

    this->event_queue = eventqueue_handle;
    //listener = new PodMgrEventListener(cm, eq,"ResourceManagerThread);

    //listener->start();
    new_flow_cnf_status = 0;
    new_flow_cnf_flowID = 0;
    del_flow_cnf_flowID = 0;
    new_flow_cnf_flows_Remaining = 0;

}


void cExtChannelThread::run(void )
{
    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    uint8_t                *bufPtr;
    cExtChannelFlow        *pFlowList = NULL;
    vector <cExtChannelFlow*>::iterator i_xCh;
    EventCableCard_XChan *pEventXch;					

    rmf_osal_event_handle_t event_handle, event_handle_rcv;
    rmf_osal_event_params_t event_params = {0}, event_params_rcv = {0};
    rmf_osal_event_category_t event_category;
    uint32_t event_type;	
    LCSM_EVENT_INFO *pEventInfo;
	
    //pfcEventMPEG::pfcEventType evType;
    //pfcEventMPEG *eventMPEG;
    int            length;
    int            offset;

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "cExtChannelThread::Run()\n");

    xchanInit();

    extResourceStatus = EXTCH_RESOURCE_CLOSED; //default value

    while (1)
    {
        int result = 0;

        rmf_osal_eventqueue_get_next_event( event_queue,
		&event_handle_rcv, &event_category, &event_type, &event_params_rcv);
        //RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "cExtChannelThread got new Event %d\n", event_category);

        switch (event_category)
        {
            case RMF_OSAL_EVENT_CATEGORY_XCHAN:

                //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "cExtChannelThread::RMF_OSAL_EVENT_CATEGORY_XCHAN\n");
                pEventInfo = (LCSM_EVENT_INFO *)event_params_rcv.data;
                if ( pEventInfo == NULL )	break;
                RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "cExtChannelThread got new RMF_OSAL_EVENT_CATEGORY_XCHAN and Event %d\n",pEventInfo->event);
                if((pEventInfo->event)  == ASYNC_TDB_OB_DATA)
                {  //OOB data available from POD

                    //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "OOB data available\n");
                    //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "size =0x%x\n", event->eventInfo.z);

                    bufPtr = (uint8_t *)pEventInfo->y;
                    length = pEventInfo->z;
                    offset = 0;
                    if(length == 0)
                        break;
                    //DumpBuffer(bufPtr, length);

                    rmf_osal_mutexAcquire(this->cm->pCMIF->extChMutex);

                    for (i_xCh = this->extChFlowList.begin (); i_xCh != this->extChFlowList.end (); i_xCh++)
                    {
                        pFlowList = *i_xCh;
                        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "cExtChannelThread::checking flowID =%x pFlowList=%p  tbid=%x\n",pFlowList->flowID,(void *)pFlowList,pFlowList->filter_value[0]);

                        if(pFlowList->flowID == pEventInfo->x)
                        { //if PID matches
                            {// DBG Throttle
                                static int nPktCount = 0;
                                nPktCount++;
                                if(0 == nPktCount%1000)
                                {
                                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "cExtChannelThread::OOB-flow ID is valid: %d packets received\n", nPktCount);
                                }
                            }

                            if(pFlowList->action == START_PID_COLLECTION)
                            { // if start has been specified for this flow

                                //check  for this iteration if the data corresponding to this TableID is present
                                //length = event->eventInfo.z;
                                //offset = 0;
                                //FilterTableSection(bufPtr, pFlowList,&offset, &length);
                                //if(length == 0)
                                //    continue; //goto next filter



                                {// DBG Throttle
                                    static int nPktCount = 0;
                                    nPktCount++;
                                    if(0 == nPktCount%1000)
                                    {
                                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "cExtChannelThread::OOB-data can be collected %d packets received\n",nPktCount);
                                    }
                                }


                                if(pFlowList->buffer_array != 0)
                                {
					 RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s:%d\n",__FUNCTION__,__LINE__);									
                                    length = (pFlowList->max_section_size >= length) ? length:pFlowList->max_section_size;
                                  memcpy(pFlowList->buffer_array, bufPtr + offset , length);    
                                }

                                if(pFlowList->cb_fn != 0)   //if callback has been specified
                                {
                                    if(pFlowList->buffer_array != 0) // data copied into caller provided buffer
                                    {
                                        pFlowList->cb_fn(pFlowList->cb_obj, 0, length, 0);
                                    }
                                    else // data provided in CardManager's buffer which should be copied   by caller within  the callback
                                    {
                                        pFlowList->cb_fn(pFlowList->cb_obj, bufPtr + offset , length, 0);
                                    }
                                }
                                else //Send a data available event if callback function is not available
                                {

                                 
                                    if(pFlowList->buffer_array == 0)
                                    {

                                        if(pFlowList->defaultBuffer)
                                        {
						 RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s:%d\n",__FUNCTION__,__LINE__);									
                                           memcpy(pFlowList->defaultBuffer,bufPtr + offset , length);                                        }
                                    }

                                    /*evType = CM_Mpeg_section_found;

                                    if (pFlowList->buffer_array)
                                    {

                                        eventMPEG = (pfcEventMPEG*)em->new_event(new pfcEventMPEG(RMF_OSAL_EVENT_CATEGORY_MPEG, evType, pFlowList->cb_obj, (int) length));

                                    }
                                    else
                                    {

                                        eventMPEG = (pfcEventMPEG*)em->new_event(new pfcEventMPEG(RMF_OSAL_EVENT_CATEGORY_MPEG, evType, pFlowList->cb_obj, pFlowList->defaultBuffer , (int) length));

                                    }

                                    em->dispatch(eventMPEG);*/
                                    event_params.priority = 0; //Default priority;
                                    event_params.data = (void*)pFlowList->defaultBuffer;
                                    event_params.data_extension = length;
                                    event_params.free_payload_fn = NULL;
                                    rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_MPEG, CM_Mpeg_section_found, 
                                		&event_params, &event_handle);
                                
                                    rmf_osal_eventmanager_dispatch_event(em, event_handle); 

                                }

                            }


                        }

                    }
                    // free any incoming OOB data in this state
                    if(bufPtr != 0)
						rmf_osal_memFreeP(RMF_OSAL_MEM_POD, bufPtr);//delete (bufPtr);

                    rmf_osal_mutexRelease(this->cm->pCMIF->extChMutex);


                }
                else if((pEventInfo->event)  == ASYNC_EC_FLOW_REQUEST_CNF)
                {
                    VL_POD_IP_STATUS ipStatus;
                    vlg_eXchanFlowResult = (VL_XCHAN_FLOW_RESULT)(pEventInfo->x);
                    vlXchanGetPodIpStatus(&ipStatus);
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "cExtChannelThread::ASYNC_EC_FLOW_REQUEST_CNF: '%s', flowtype = %d\n",
                           ipStatus.szPodIpStatus,
                           pEventInfo->returnStatus);


                    new_flow_cnf_status = (int)pEventInfo->x;//status field
                    new_flow_cnf_flowID = (int)(pEventInfo->y); //flowID
                    new_flow_cnf_flows_Remaining = (int)(pEventInfo->z);
                    if(pEventInfo->dataPtr)
                    {
                        ipu_flow_params = *((VL_IP_IF_PARAMS*)(pEventInfo->dataPtr));
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "cExtChannelThread::Got IP Address %d.%d.%d.%d\n",
                               ipu_flow_params.ipAddress[0], ipu_flow_params.ipAddress[1],
                               ipu_flow_params.ipAddress[2], ipu_flow_params.ipAddress[3]);
                    }
                    else
                    {
                        memset(ipu_flow_params.ipAddress, 0, sizeof(ipu_flow_params.ipAddress));
                    }

                    rmf_osal_mutexAcquire(this->cm->pCMIF->extChMutex);
                    {
                        for (i_xCh = this->extChFlowList.begin (); i_xCh < this->extChFlowList.end (); i_xCh++)
                        {
                            pFlowList = *i_xCh;
                            // Oct-10-2008: Bugfix: Check both in progress status and type of the requested flow
                            if( (pFlowList->status       == FLOW_CREATION_IN_PROGRESS    ) &&
                                (pFlowList->serviceType  == pEventInfo->returnStatus)    )
                            {
                                // Oct-10-2008: Bugfix: Update the params received from the confirmation
                                pFlowList->status = (XFlowStatus_t)(pEventInfo->x); //status field
                                pFlowList->flowID = (int)(pEventInfo->y); //flowID
                                pFlowList->flows_Remaining = (uint8_t)pEventInfo->z;
                                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "cExtChannelThread::ASYNC_EC_FLOW_REQUEST_CNF: Got Flow Confirmation for serviceType = %d, status = %d, flowid = %d, pFlowList = %p\n", pFlowList->serviceType, pFlowList->status, pFlowList->flowID, pFlowList);
                                break;
                            }
                        }
                    }
                    rmf_osal_mutexRelease(this->cm->pCMIF->extChMutex);
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "cExtChannelThread::ASYNC_EC_FLOW_REQUEST_CNF status=%x  flowID=%x flow Rem=%x\n",new_flow_cnf_status,new_flow_cnf_flowID,new_flow_cnf_flows_Remaining);

                     sem_post(&(this->cm->pCMIF->extChSem));
                }
                else if((pEventInfo->event)  == ASYNC_EC_FLOW_DELETE_CNF)
                {
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "ExtChanEventListener::ASYNC_EC_FLOW_DELETE_CNF\n");

                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", " flowID=%ld:  \n", pEventInfo->x );

                    del_flow_cnf_flowID =      pEventInfo->x;

                    // Can send a event from here to indicate delete conf
                     sem_post(&(this->cm->pCMIF->extChSem));
                }
                else if((pEventInfo->event)  == ASYNC_EC_FLOW_LOST_EVENT)
                {
                    //should we remove from List?
                    //first remove this flowID from the List
                    #if 0
                    for (i_xCh = this->extChFlowList.begin(); i_xCh < this->extChFlowList.end(); i_xCh++){
                             pFlowList = *i_xCh;
                            if(pFlowList->flowID ==  event->eventInfo.x){
                                if(pFlowList->dataPtr != 0)
                                {
                                    //VLFREE_INFO2(pFlowList->dataPtr);
                                    delete ( pFlowList->dataPtr);
                                }
                                this->extChFlowList.erase(i_xCh);
                            }
                    }
                    #endif
                    //Now send a notification about this Lost Flow ID with the reason byte
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "ExtChanEventListener::ASYNC_EC_FLOW_LOST_EVENT\n");

                    rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(EventCableCard_XChan),(void **)&pEventXch);                    					

                    pEventXch->flowID      = pEventInfo->x; //flowID
                    event_params.priority = 0; //Default priority;
                    event_params.data = (void *)pEventXch;
                    event_params.data_extension = 0;
                    event_params.free_payload_fn = EventCableCard_XChan_free_fn;
                    rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_CardManagerXChanEvents, card_xchan_lost_flow_ind, 
                		&event_params, &event_handle);
                
                    rmf_osal_eventmanager_dispatch_event(em, event_handle);
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", " flowID=%x:  \n", pEventXch->flowID );
                    //Send Lost Flow Confirmation to POD

                }
                else if((pEventInfo->event)  == ASYNC_OPEN) // notiifes if Ext CH resource is open
                {
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "Extended CHannel resource opened\n");
                    extResourceStatus = EXTCH_RESOURCE_OPEN;
                    rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_CardManagerXChanEvents, card_xchan_resource_opened, 
                		NULL, &event_handle);                
                    rmf_osal_eventmanager_dispatch_event(em, event_handle);

                    this->cm->cablecardFlag = CARD_READY;
                    this->cm->CmcablecardFlag = CM_CARD_READY;

                }
                else if((pEventInfo->event)  == ASYNC_CLOSE) //notiifes if Ext CH resource is closed
                {
                    extResourceStatus = EXTCH_RESOURCE_CLOSED;
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "Extended CHannel resource closed\n");

                }
                else if((pEventInfo->event)  == ASYNC_START_OOB_QPSK_IP_FLOW) // notiifes if Ext CH resource is open
                {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "cExtChannelThread::run: received ASYNC_START_OOB_QPSK_IP_FLOW posting card_xchan_start_oob_ip_over_qpsk\n");

                    rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_CardManagerXChanEvents, card_xchan_start_oob_ip_over_qpsk, 
                		NULL, &event_handle);                
                    rmf_osal_eventmanager_dispatch_event(em, event_handle);

                }
                else if((pEventInfo->event)  == ASYNC_STOP_OOB_QPSK_IP_FLOW) //notiifes if Ext CH resource is closed
                {
                    rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_CardManagerXChanEvents, card_xchan_stop_oob_ip_over_qpsk, 
                		NULL, &event_handle);                
                    rmf_osal_eventmanager_dispatch_event(em, event_handle);
                }
#ifdef USE_IPDIRECT_UNICAST
                // IPDU register flow
                else if((pEventInfo->event)  == ASYNC_START_IPDU_XCHAN_FLOW) // notiifes if Ext CH resource is open
                {
                    // IPDU The IPDU Extended channel registration has been decoupled from the MPEG OOB extendend flow
                    // but is not completely integrated into the POD Manager. The call to vlXchanRegisterNewFlow is
                    // the interim solution.
                    vlXchanRegisterNewFlow(VL_XCHAN_FLOW_TYPE_IPDIRECT_UNICAST, VL_DSG_BASE_HOST_IPDIRECT_UNICAST_SEND_FLOW_ID);
                    #if 0
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "cExtChannelThread::run: received ASYNC_START_IPDU_XCHAN_FLOW\n");
                    rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_CardManagerXChanEvents, card_xchan_start_ipdu,
                        NULL, &event_handle);
                    rmf_osal_eventmanager_dispatch_event(em, event_handle);
                    #endif

                }
                else if((pEventInfo->event)  == ASYNC_STOP_IPDU_XCHAN_FLOW) //notiifes if Ext CH resource is closed
                {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "cExtChannelThread::run: received ASYNC_STOP_IPDU_XCHAN_FLOW\n");
                    rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_CardManagerXChanEvents, card_xchan_stop_ipdu,
                        NULL, &event_handle);
                    rmf_osal_eventmanager_dispatch_event(em, event_handle);
                }
#endif // USE_IPDIRECT_UNICAST
                else
                {
                    //this->cm->pCMIF->extChMutex.lock();
                    xchanProtectedProc((void *)pEventInfo);
                     //this->cm->pCMIF->extChMutex.unlock();
                }

                break;


            case RMF_OSAL_EVENT_CATEGORY_POD_ERROR:
                pEventInfo = (LCSM_EVENT_INFO *)event_params_rcv.data;                		
                if ( pEventInfo == NULL )	break;				
                if((pEventInfo->event)  ==  POD_ERROR)
                {
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "POD ERROR received\n");
                    //find out which error has occurred
                    if(pEventInfo->x ==  POD_SESSION_NOT_FOUND)
                    {
                        rmf_osal_mutexAcquire(this->cm->pCMIF->extChMutex);
                        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "POD ERROR received - Session Not FOund\n");

                        for (i_xCh = this->extChFlowList.begin (); i_xCh < this->extChFlowList.end (); i_xCh++)
                        {
                            pFlowList = *i_xCh;
                            //Prasad (10/06/2008) - NULL CHECK
                            if(NULL == pFlowList)
                                continue;

                            if (pFlowList->status == FLOW_CREATION_IN_PROGRESS)
                                break;

                        }

                        if(NULL != pFlowList)
                          pFlowList->status =  FLOW_UNINITIALIZED; //status field

                        rmf_osal_mutexRelease(this->cm->pCMIF->extChMutex);

                        sem_post(&(this->cm->pCMIF->extChSem));                        

                    }
                }
                break;
            default:
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "ExtChanEventListener::default\n");
                break;
        }

        rmf_osal_event_delete(event_handle_rcv);
    }


}


/*takes the OOB data available and finds the offset to the requested section and the section length
length has complete data length on entering the function and returns the requested section's length*/
//TO DO : implement similar function for cases where multiple TableIDs are required

void cExtChannelThread::FilterTableSection(uint8_t *bufPtr, cExtChannelFlow *pFlowList,int *offset, int *length)
{

    int secLength;
    int ID;
    uint8_t  tableID = pFlowList->filter_value[0];
    cSimpleBitstream b(bufPtr, *length);

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "checking for tableID=%x , bufLen=%x\n",tableID,*length);
    while(b.get_pos() < (unsigned int)((*length)*8))
    {
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", " checking next section..\n");
        ID = b.get_bits(8);
        b.skip_bits(4);
        secLength = b.get_bits(12);
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "cExtChannelThread::FilterTableSection : tableID=%x :secLen=%x\n", ID,secLength);
        if( tableID == ID)
        {
            *length = secLength + 3;// 3 is for the header(tableid, length)
            *offset = (b.get_pos() ) - 24 ; //24 because we have already read the tableID and the secLength, hence need to step back
            return;

        }
        else
        {

            b.skip_bits( secLength * 8);
        }


    }
    //if we get here then the TableID has not been found in the data stream
    *offset = -1;
    *length = 0;

}


//-------------------------------------------------------------------------------------------------------------------------------------
// implementing class cExtChannelFlow
//

 /*
cExtChannelFlow::cExtChannelFlow(cCardManagerIF *pCMInterface):cb_fn(NULL),buffer_array(0),listener_eq(0),status(FLOW_UNINITIALIZED),flowID(INVALID_FLOW_ID),action(NO_ACTION),dataPtr(0),dataLength(0)
*/
cExtChannelFlow::cExtChannelFlow(cCardManagerIF *pCMInterface)
{
    memset( filter_value, 0, sizeof(filter_value) );
    memset( maskandmode, 0, sizeof(maskandmode));
    memset( maskandnotmode, 0, sizeof(maskandnotmode));
    is_buffer_free_cb = NULL;
    pid = 0;
    m_nRetries = 0;
    m_pPodPresenceEventListener = NULL;
	
    VL_ZERO_STRUCT(ipu_flow_params);
    this->pCMIF = pCMInterface;

    //Initialize the members
    cb_fn            = NULL;
    cb_obj            = NULL;
    buffer_array    = NULL;
    status            = FLOW_UNINITIALIZED;
    flowID            = INVALID_FLOW_ID;
    action            = NO_ACTION;
    dataPtr            = NULL;
    dataLength        = 0;
    optionBytesLength = 0;
    optionBytes        = NULL;
    defaultBuffer    = NULL;
    max_section_size = 0;
    numBuffs        = 0;

    serviceType = CM_SERVICE_TYPE_INVALID_FLOW_SERVICE_TYPE;
    dataPtr = NULL;
    buffer_array  = NULL;
    defaultBuffer = NULL;

    //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "cExtChannelFlow::cExtChannelFlow :status=%x\n",status);

}

cExtChannelFlow::cExtChannelFlow()
{
    VL_ZERO_STRUCT(ipu_flow_params);
    flowID = INVALID_FLOW_ID;

    //Initialize the members
    cb_fn            = NULL;
    cb_obj            = NULL;
    buffer_array    = NULL;
    status            = FLOW_UNINITIALIZED;
    flowID            = INVALID_FLOW_ID;
    action            = NO_ACTION;
    dataPtr            = NULL;
    dataLength        = 0;
    optionBytesLength = 0;
    optionBytes        = NULL;
    defaultBuffer    = NULL;
    max_section_size = 0;
    numBuffs        = 0;
    memset( filter_value, 0, sizeof(filter_value) );
    memset( maskandmode, 0, sizeof(maskandmode));
    memset( maskandnotmode, 0, sizeof(maskandnotmode));
    is_buffer_free_cb = NULL;
    pid = 0;
    pCMIF = NULL;
    m_nRetries = 0;
    m_pPodPresenceEventListener = NULL;
    serviceType = CM_SERVICE_TYPE_INVALID_FLOW_SERVICE_TYPE;
    dataPtr = NULL;
    buffer_array  = NULL;
    defaultBuffer = NULL;
}

cExtChannelFlow::~cExtChannelFlow()
{

}

void cExtChannelFlow::SetServiceType(Service_type_t serviceType)
{
    this->serviceType = serviceType;

}

Service_type_t cExtChannelFlow::GetServiceType(void)
{
    return this->serviceType;
}

int  cExtChannelFlow::GetPID()
{
    return this->pid;
}

void cExtChannelFlow::SetPID(int pid)
{
    this->pid = pid;
}

void cExtChannelFlow::SetMacAddress(uint8_t *addr)
{
/*    memcpy(this->macAddr, addr, 6);*/
    VL_ZERO_STRUCT(ipu_flow_params);
    memcpy(this->ipu_flow_params.macAddress, addr, VL_MAC_ADDR_SIZE);
}

int32_t cExtChannelFlow::GetIPAddress()
{
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "returning IP addr = %d.%d.%d.%d\n",
           this->ipu_flow_params.ipAddress[0],
           this->ipu_flow_params.ipAddress[1],
           this->ipu_flow_params.ipAddress[2],
           this->ipu_flow_params.ipAddress[3]);
    return *((int32_t*)(this->ipu_flow_params.ipAddress));
}

VL_IP_IF_PARAMS cExtChannelFlow::GetIpuParams()
{
    return this->ipu_flow_params;
}

void cExtChannelFlow::SetOptionByteLength(uint8_t length)
{
    this->optionBytesLength = length;
}

void cExtChannelFlow::SetOptionBytes(uint8_t  *data)// if length is not set before this call using SetOptionByteLength , return a error
{
    this->optionBytes = data;
}

void cExtChannelFlow::SendIPData(uint8_t *data, uint32_t length, uint32_t flowID)
{

    POD_STACK_SEND_ExtCh_Data( data, length,  flowID);

}

// adds this object to the extChannelList in cCardManagerIF
// status of adding this flow is sent through a event
//Note: this function does not check if the request is coming for a PID which has a flow currently open
//this should be done by TableManager or IOResourceManager
int  cExtChannelFlow::RequestExtChannelFlow()
{
    LCSM_EVENT_INFO      eventInfo;
    LCSM_EVENT_INFO *pEvent; //pfcEventCableCard         *pEvent,*pEvent2;
    rmf_osal_eventmanager_handle_t  em;
    cExtChannelFlow        *pFlowList;
    vector <cExtChannelFlow*>::iterator i_xCh;
    uint8_t            *apdu_buffer = 0;
    bool bIsAlreadyPresent = false;
    rmf_osal_event_handle_t event_handle;
    rmf_osal_event_params_t event_params = {0};

    unsigned long usec, sec, nsec;
    struct timespec time_spec;
    struct timeval time_val;
    struct timezone time_zone;
    int8_t sem_ret;

     memset( &eventInfo, 0, sizeof(eventInfo) );     	

    //pfcEventQueue   *eq = new pfcEventQueue();

    em = get_pod_event_manager();

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "cExtChannelFlow::RequestExtChannelFlow %s this=%p\n", aFlowNames[VL_SAFE_ARRAY_INDEX(this->serviceType, aFlowNames)], (void *)this);
    //do not send new flow request if resource has not been opened yet.or if POD not inserted yet
    if(!(this->pCMIF->CMInstance->pXChan->IsResourceOpen()))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "cExtChannelFlow::RequestExtChannelFlow Resource is not open\n");
        return EXTCH_RESOURCE_NOT_OPEN;
    }
    if(HomingCardFirmwareUpgradeGetState() == 1)
    {
    //    return EXTCH_RESOURCE_NOT_OPEN;
    }

    rmf_osal_mutexAcquire(this->pCMIF->CMInstance->pXChan->ext_ch_req_flow_lock_mutex);    //blocking other RequestExtChannelFlow

    this->flowID = INVALID_FLOW_ID; //will be filled with the correct value in new_flow_cnf

    this->status = FLOW_CREATION_IN_PROGRESS;

    //Prasad (10/06/2008) - Protect the list as it is accessed in  different thread contexts
    rmf_osal_mutexAcquire(this->pCMIF->extChMutex);

    for (i_xCh = this->pCMIF->CMInstance->pXChan->extChFlowList.begin (); i_xCh < this->pCMIF->CMInstance->pXChan->extChFlowList.end (); i_xCh++)
    {
        pFlowList = *i_xCh;
        if(NULL == pFlowList)
            continue;

        if(pFlowList == this)
        {
            bIsAlreadyPresent = true;
        }
    }

    if(false == bIsAlreadyPresent)
    {
        this->pCMIF->CMInstance->pXChan->extChFlowList.push_back(this);
    }

    //Prasad (10/06/2008) - Protect the list as it is accessed in  different thread contexts
    rmf_osal_mutexRelease(this->pCMIF->extChMutex);

    switch(this->serviceType)
    {

        case CM_SERVICE_TYPE_MPEG_SECTION:
        case CM_SERVICE_TYPE_IP_U:

            eventInfo.event = ASYNC_EC_FLOW_REQUEST;
            if(this->serviceType == CM_SERVICE_TYPE_MPEG_SECTION)
            {
                eventInfo.x = CM_SERVICE_TYPE_MPEG_SECTION;
                eventInfo.y = this->pid;
            }
            else if(this->serviceType == CM_SERVICE_TYPE_IP_U)
            {
                eventInfo.x = CM_SERVICE_TYPE_IP_U;
                apdu_buffer = new uint8_t[8 + this->optionBytesLength];
                apdu_buffer[0] = CM_SERVICE_TYPE_IP_U;
                memcpy(apdu_buffer+1, this->ipu_flow_params.macAddress, VL_MAC_ADDR_SIZE);
                apdu_buffer[7] = this->optionBytesLength;
                memcpy(apdu_buffer+8, this->optionBytes, this->optionBytesLength);
                eventInfo.y = (uint32_t) apdu_buffer;
            }

            eventInfo.z = POD_XCHAN_QUEUE;
            eventInfo.dataPtr = NULL;
            eventInfo.dataLength = 0;

            //Prasad (10/06/2008) - Protect the list as it is accessed in  different thread contexts
            rmf_osal_mutexAcquire(this->pCMIF->extChMutex);

            //if this PID has not been requested so far, allow this request to proceed
            for (i_xCh = this->pCMIF->CMInstance->pXChan->extChFlowList.begin (); i_xCh < this->pCMIF->CMInstance->pXChan->extChFlowList.end (); i_xCh++)
            {
                pFlowList = *i_xCh;
                //Prasad (10/06/2008) - NULL CHECK
                if(NULL == pFlowList)
                    continue;

                // we have already pushed this object into the list
                // we only need to find out if any other object already exists for IP service type as
                // only one IP flow can exist
                if(pFlowList == this)
                {
                    continue;
                }

                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "flowID=%x  pid=%x this->flowId=%x this->pid=%x flowlist=%p\n",pFlowList->flowID,pFlowList->pid, this->flowID,this->pid, (void *)pFlowList);

                if(this->serviceType == CM_SERVICE_TYPE_IP_U)
                {
                    if(pFlowList->serviceType == this->serviceType)
                    {    //do not allow another IP flow as normally there is only one IP flow
                        //Prasad (10/06/2008) - Protect the list as it is accessed in  different thread contexts
                        rmf_osal_mutexRelease(this->pCMIF->extChMutex);

                        rmf_osal_mutexRelease(this->pCMIF->CMInstance->pXChan->ext_ch_req_flow_lock_mutex);
                        return EXTCH_PID_FLOW_EXISTS; //hence do not open a new flow
                    }
                }
            }

            //Prasad (10/06/2008) - Protect the list as it is accessed in  different thread contexts
            rmf_osal_mutexRelease(this->pCMIF->extChMutex);

            break;

        case CM_SERVICE_TYPE_IP_M:
            break;

#ifdef USE_IPDIRECT_UNICAST
        case CM_SERVICE_TYPE_IPDU:	// IPDU register flow
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "cExtChannelFlow::RequestExtChannelFlow process CM_SERVICE_TYPE_IPDU.\n");
            eventInfo.event = ASYNC_EC_FLOW_REQUEST;
            eventInfo.x = CM_SERVICE_TYPE_IPDU;
            eventInfo.y = NULL;
            eventInfo.z = POD_XCHAN_QUEUE;
            eventInfo.dataPtr = NULL;
            eventInfo.dataLength = 0;
            break;
#endif // USE_IPDIRECT_UNICAST

        default:
            rmf_osal_mutexRelease(this->pCMIF->CMInstance->pXChan->ext_ch_req_flow_lock_mutex);
            return CM_ERROR_INVALID_SECTION_TYPE;
    }

    gettimeofday (&time_val, &time_zone);
    usec = (time_val.tv_usec + 1000); 
    sec = usec / 1000000;
    nsec = (usec % 1000000) * 1000;
    
    time_spec.tv_sec = time_val.tv_sec + sec;
    time_spec.tv_nsec = nsec;

    while(1)
    {
          if((sem_ret = sem_timedwait (&this->pCMIF->extChSem, &time_spec)) < 0) 
          {
               if(errno == EINTR) {
                      continue;				
               }
               else
               {
                     break;
               }
          }
    }

    //this->pCMIF->extChSem.timed_lock_usecs(1000); // Feb-03-2009: Bugfix: Clear out the semaphore first.
    //
    rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(LCSM_EVENT_INFO),(void **)&pEvent);
    memcpy(pEvent,&eventInfo, sizeof (LCSM_EVENT_INFO));

    event_params.priority = 0; //Default priority;
    event_params.data = (void *)pEvent;
    event_params.data_extension = 0;
    event_params.free_payload_fn = podmgr_freefn;
    rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_XCHAN, eventInfo.event, 
		&event_params, &event_handle);
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "\n Creating event RMF_OSAL_EVENT_CATEGORY_XCHAN\n");
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "\t pEvent->event= %d \n", pEvent->event);
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "\t pEvent->eventInfo.Service Type= %d \n", pEvent->x);
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "\t pEvent->eventInfo.Queue= %d \n", pEvent->z);

    rmf_osal_eventmanager_dispatch_event(em, event_handle);

	gettimeofday (&time_val, &time_zone);

	time_spec.tv_sec = time_val.tv_sec + 6;
	time_spec.tv_nsec = time_val.tv_usec  * 1000 ;
    while(1)
    {
          if((sem_ret = sem_timedwait (&this->pCMIF->extChSem, &time_spec)) < 0) 
          {
               if(errno == EINTR) {
                      continue;				
               }
               else if(errno == ETIMEDOUT) {
                      RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "Timed out- NO response for %s on Ext Channel\n", aFlowNames[VL_SAFE_ARRAY_INDEX(this->serviceType, aFlowNames)]);
                      break;
               }
               else
               {
                     break;
               }
          }
          else
          {
               break;
          }
    }

    //this->extChSem.lock();
    if(sem_ret == ETIMEDOUT)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "Timed out- NO response for %s on Ext Channel\n", aFlowNames[VL_SAFE_ARRAY_INDEX(this->serviceType, aFlowNames)]);
        this->status =  FLOW_UNINITIALIZED;  // since this flow was inserted into list, remove it.
#if 0
        this->pCMIF->extChMutex.lock();
        for (i_xCh = this->pCMIF->CMInstance->pXChan->extChFlowList.begin (); i_xCh < this->pCMIF->CMInstance->pXChan->extChFlowList.end(); i_xCh++){
                        pFlowList = *i_xCh;
                        if(pFlowList  == this){
                            if(pFlowList->dataPtr != 0)
                            {
                                //VLFREE_INFO2(pFlowList->dataPtr);
                                delete ( pFlowList->dataPtr);
                            }
                            this->pCMIF->CMInstance->pXChan->extChFlowList.erase(i_xCh);
                        }
                    }
        this->pCMIF->extChMutex.unlock();
 #endif
    }
    else
    {
        /* Oct-10-2008: BugFix: This information is already updated by cExtChannelThread so the following should not be called
        //this->status = (XFlowStatus_t)(this->pCMIF->CMInstance->pXChan->new_flow_cnf_status);   //status field
        //this->flowID = this->pCMIF->CMInstance->pXChan->new_flow_cnf_flowID; //flowID
        //this->flows_Remaining = this->pCMIF->CMInstance->pXChan->new_flow_cnf_flows_Remaining;
        */
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "cExtChannelFlow::RequestExtChannelFlow: pFlowList = %p, Received confirmation for flowType 0x%d, status %d, flowId = %d, flows_Remaining = %d\n", this, serviceType, status, flowID, flows_Remaining);
        if((FLOW_CREATED == this->status) && (CM_SERVICE_TYPE_IP_U == this->serviceType))
        {
            // save the MAC address
            memcpy(this->pCMIF->CMInstance->pXChan->ipu_flow_params.macAddress, this->ipu_flow_params.macAddress, VL_MAC_ADDR_SIZE);
            this->ipu_flow_params = this->pCMIF->CMInstance->pXChan->ipu_flow_params ;
        }
    }

    //PPR (10/04/2008)
    if(apdu_buffer)
    {
        //VLFREE_INFO2(apdu_buffer);
        delete [] apdu_buffer;
    }

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "cExtChannelFlow::RequestExtChannelFlow: %s status=%x\n", aFlowNames[VL_SAFE_ARRAY_INDEX(this->serviceType, aFlowNames)], this->status);

    rmf_osal_mutexRelease(this->pCMIF->CMInstance->pXChan->ext_ch_req_flow_lock_mutex);

    return    ((int)this->status);
}

void  cExtChannelFlow::vlRequestExtChannelFlowTask(void * arg)
{
    XFlowStatus_t retValue = (XFlowStatus_t)(FLOW_NOT_STARTED);
    cExtChannelFlow * pCMextFlowObj = ((cExtChannelFlow*)arg);
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD","vlRequestExtChannelFlowTask started \n");
	
    while((pCMextFlowObj->m_nRetries != 0) && (retValue != FLOW_CREATED))
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: Trying Flow Creation\n",__FUNCTION__);
        // Avoid any chance of tight loops
        rmf_osal_threadSleep(1000, 0);  
        
        // check for resource open before proceeding
        if(!(pCMextFlowObj->pCMIF->CMInstance->pXChan->IsResourceOpen()))
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "cExtChannelFlow::vlRequestExtChannelFlowTask Resource is not open ...exiting...\n");
            // if resource is not open then card is not inserted or is removed so quit
            pCMextFlowObj->DeleteExtChannelFlow();
            delete pCMextFlowObj;
            //VLFREE_INFO2(pCMextFlowObj);
            return;
        }
        
        // request flow
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: Requesting for flowtype = %d\n",__FUNCTION__, pCMextFlowObj->serviceType);
        retValue = (XFlowStatus_t)(pCMextFlowObj->RequestExtChannelFlow());
             
        if( retValue != FLOW_CREATED)  // cable card may not be in
        {    
            //this->DeleteFlow();
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","%s: %s FLOW Creation FAILED : retValue = %d, status = %x\n",__FUNCTION__, aFlowNames[VL_SAFE_ARRAY_INDEX(pCMextFlowObj->serviceType, aFlowNames)], retValue, pCMextFlowObj->GetStatus());
        }
        else // Flow already open
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: %s FLOW Creation SUCCEEDED, retValue = %d, status = %x; flowID=%x: flowRem=%x\n",__FUNCTION__, aFlowNames[VL_SAFE_ARRAY_INDEX(pCMextFlowObj->serviceType, aFlowNames)], retValue, pCMextFlowObj->GetStatus(),pCMextFlowObj->GetFlowID(), pCMextFlowObj->GetFlowsRemaining());
            //int remainingFlows =     this->pCMextFlowObj->GetFlowsRemaining();                
            pCMextFlowObj->m_pPodPresenceEventListener->pCMextFlowObj = pCMextFlowObj;
        }
        
        pCMextFlowObj->m_nRetries--;
    }
    
    return;
}

int cExtChannelFlow::RequestExtChannelFlowWithRetries(cPodPresenceEventListener * pPodPresenceEventListener,
                                            Service_type_t eServiceType,
                                            unsigned long nRetries)
{
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: Starting Flow Creation\n",__FUNCTION__);
    cExtChannelFlow * pCMextFlowObj = new cExtChannelFlow(pPodPresenceEventListener->cm->pCMIF);
    rmf_osal_ThreadId threadId = 0;

    pCMextFlowObj->SetPID(0x1FFC);
    pCMextFlowObj->SetServiceType(eServiceType);				// IPDU register flow

    pCMextFlowObj->buffer_array = NULL;
    pCMextFlowObj->cb_fn = NULL;
    pCMextFlowObj->cb_obj = NULL;//this;
    pCMextFlowObj->m_nRetries = nRetries;
    pCMextFlowObj->m_pPodPresenceEventListener = pPodPresenceEventListener;
    

    rmf_osal_threadCreate(vlRequestExtChannelFlowTask, pCMextFlowObj, 0, 0, &threadId, "vlRequestExtChannelFlowTask");
    return 0;	
}

int  cExtChannelFlow::DeleteExtChannelFlow() // deletes this object from the extChannelList
{
    LCSM_EVENT_INFO      eventInfo;
    LCSM_EVENT_INFO         *pEvent; //pfcEventCableCard         *pEvent,*pEvent2;
    rmf_osal_eventmanager_handle_t em;
    int        retValue=0;
    cExtChannelFlow        *pFlowList;
    vector <cExtChannelFlow*>::iterator i_xCh;
    rmf_osal_event_handle_t event_handle;
    rmf_osal_event_params_t event_params = {0};
    unsigned long usec, sec, nsec;
    struct timespec time_spec;
    struct timeval time_val;
    struct timezone time_zone;
    int8_t sem_ret;


	
    char * flow_types[] =
    {
        "MPEG_SECTION",
        "IP_U",
        "IP_M",
        "DSG",
        "",
    };

    memset( &eventInfo, 0, sizeof(eventInfo));
 
    if((serviceType >= 0) && (serviceType < sizeof(flow_types)/sizeof(flow_types[0])))
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "cExtChannelFlow::DeleteExtChannelFlow (%s)\n", flow_types[VL_SAFE_ARRAY_INDEX(serviceType, flow_types)]);
    }

    em = get_pod_event_manager();

    // Aug-01-2008: moved this statement below

    rmf_osal_mutexAcquire(this->pCMIF->CMInstance->pXChan->ext_ch_req_flow_lock_mutex);    //blocking other DeleteExtChannelFlow

    //mutex between this function and the xchan thread

    //Prasad (10/06/2008) - Protect the list as it is accessed in  different thread contexts
    rmf_osal_mutexAcquire(this->pCMIF->extChMutex);
    int size = 0;	
    for (i_xCh = this->pCMIF->CMInstance->pXChan->extChFlowList.begin (); i_xCh < this->pCMIF->CMInstance->pXChan->extChFlowList.end (); i_xCh++)
    {
        vector <cExtChannelFlow*>::iterator temp_i_xCh;
        pFlowList = *i_xCh;
        if(NULL == pFlowList)
            continue;

        if(pFlowList == this)
        {
            if(pFlowList->dataPtr != 0)
            {
                //VLFREE_INFO2(pFlowList->dataPtr);
                delete ( pFlowList->dataPtr);
                pFlowList->dataPtr = NULL;
            }

            if(1 < this->pCMIF->CMInstance->pXChan->extChFlowList.size())
            {
			temp_i_xCh = i_xCh - 1;
            }

            this->pCMIF->CMInstance->pXChan->extChFlowList.erase(i_xCh);
            if((CM_SERVICE_TYPE_IP_U == serviceType) && (!this->pCMIF->CMInstance->pXChan->extChFlowList.empty()))
            {
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "cExtChannelFlow::DeleteExtChannelFlow : removing IP_U flow 0x%08X from extChFlowList continuing search for duplicates...\n", pFlowList);
                i_xCh = temp_i_xCh;
                continue;
            }
            break;
        }
    }

    //Prasad (10/06/2008) - Protect the list as it is accessed in  different thread contexts
    rmf_osal_mutexRelease(this->pCMIF->extChMutex);

    //Prasad (10/06/2008) - Commented .
    //rmf_osal_mutexRelease(this->pCMIF->CMInstance->pXChan->ext_ch_req_flow_lock_mutex);

    // Aug-01-2008: moved this from above
    //do not send new flow request if resource has not been opened yet or if POD not inserted yet
    if(!(this->pCMIF->CMInstance->pXChan->IsResourceOpen()))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "cExtChannelFlow::DeleteExtChannelFlow: resource not open\n");
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "cExtChannelFlow::DeleteExtChannelFlow: resource not open (%s)\n", flow_types[VL_SAFE_ARRAY_INDEX(serviceType, flow_types)]);
        //Prasad (10/06/2008) - Added
        rmf_osal_mutexRelease(this->pCMIF->CMInstance->pXChan->ext_ch_req_flow_lock_mutex);
        return EXTCH_RESOURCE_NOT_OPEN;
    }

    //Prasad (10/06/2008) - Commented .
   // rmf_osal_mutexAcquire(this->pCMIF->CMInstance->pXChan->ext_ch_req_flow_lock_mutex); //blocking other DeleteExtChannelFlow

    if(this->flowID != INVALID_FLOW_ID) //if flowID is valid only then delete flow
    {
        // send del_flow_req to CableCard
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "cExtChannelFlow::DeleteExtChannelFlow(): sending del_flow_req flowtype = %d, flowID= %x  pid=%x\n",this->serviceType, this->flowID, this->pid);
        eventInfo.event = ASYNC_EC_FLOW_DELETE;
        eventInfo.x = 0;
        eventInfo.y =  this->flowID;
        eventInfo.z = POD_XCHAN_QUEUE;
        eventInfo.dataPtr = NULL;
        eventInfo.dataLength = 0;
        eventInfo.sequenceNumber = 0;

        rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(LCSM_EVENT_INFO),(void **)&pEvent);           
        memcpy(pEvent,&eventInfo, sizeof (LCSM_EVENT_INFO));

        event_params.priority = 0; //Default priority;
        event_params.data = (void *)pEvent;
        event_params.data_extension = 0;
        event_params.free_payload_fn = podmgr_freefn;
        rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_XCHAN, eventInfo.event, 
    		&event_params, &event_handle);
    
        rmf_osal_eventmanager_dispatch_event(em, event_handle);
		
	gettimeofday (&time_val, &time_zone);

	time_spec.tv_sec = time_val.tv_sec + 6;
	time_spec.tv_nsec = time_val.tv_usec  * 1000 ;
        //this->extChSem.lock();
        while(1)
        {
              if((sem_ret = sem_timedwait (&this->pCMIF->extChSem, &time_spec)) < 0) 
              {
                   if(errno == EINTR) {
                          continue;				
                   }
                   else
                   {
                         break;
                   }
              }
        }
        if(sem_ret == ETIMEDOUT)
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "NO response on Ext Channel\n");
            //Prasad (10/06/2008) - Added
            // unlock not required here as there is no return statement here//rmf_osal_mutexRelease(this->pCMIF->CMInstance->pXChan->ext_ch_req_flow_lock_mutex);
            retValue = EXTCH_NO_RESPONSE;
        }
        else if(this->pCMIF->CMInstance->pXChan->del_flow_cnf_flowID  == this->flowID) //if not this flowID could be a error condition or lost flow
        {
            retValue = 0;
        }
    }

    //Prasad (10/06/2008)
    rmf_osal_mutexRelease(this->pCMIF->CMInstance->pXChan->ext_ch_req_flow_lock_mutex);
    return    retValue;  //delete successful
}


int cExtChannelFlow::StartFiltering(void) //Ext Ch data pertaining to this flowID is ignored until StartFiltering
{
    int    retValue = CM_NO_ERROR;
    this->action = START_PID_COLLECTION;

    return retValue;
}

int cExtChannelFlow::StopFiltering()
{
    int retValue = CM_NO_ERROR;
    if(this->action == START_PID_COLLECTION)
        this->action = STOP_PID_COLLECTION;
    else
        retValue = CM_ERROR_FILTER_NOT_STARTED;

    return retValue;
}

void cExtChannelFlow::ReadData(uint8_t *extendedData, uint32_t dataLen)
{
#if 0
    vector <cExtChannelFlow*>::iterator i_xCh;
    uint8_t    *ptr;
        this->pCMIF->extChMutex.lock();
        for (i_xCh = this->pCMIF->CMInstance->pXChan->extChFlowList.begin(); i_xCh < this->pCMIF->CMInstance->pXChan->extChFlowList.end(); i_xCh++){
            if((*i_xCh)->flowID == this->flowID){
                ptr = (*i_xCh)->dataPtr;

            }

        }
        memcpy( extendedData, ptr, dataLen, datalen);//assuming this dataLen is for the complete length of the data
        //VLFREE_INFO2((*i_xCh)->dataPtr);
        delete ((*i_xCh)->dataPtr);
        (*i_xCh)->dataPtr = 0;

        this->pCMIF->extChMutex.unlock();
#endif
    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    rmf_osal_eventqueue_handle_t eventqueue_handle;
    uint32_t        length = 0;
    rmf_osal_event_handle_t event_handle_rcv;
    rmf_osal_event_params_t event_params_rcv = {0}; 
    rmf_osal_event_category_t event_category;
    uint32_t event_type;
    EventCableCard_XChan *pEvent;
		

    rmf_osal_eventqueue_create ( (const uint8_t* ) "cExtChannelFlow::ReadData",
		&eventqueue_handle);

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "cExtChannelFlow::ReadData()\n");
	
    rmf_osal_eventmanager_register_handler(
		em,
		eventqueue_handle, RMF_OSAL_EVENT_CATEGORY_XCHAN
		);
	
    while (1)
    {
    int result = 0;

        rmf_osal_eventqueue_get_next_event( eventqueue_handle,
		&event_handle_rcv, &event_category, &event_type, &event_params_rcv);

        switch (event_category)
        {
            case RMF_OSAL_EVENT_CATEGORY_CardManagerXChanEvents:
            {
                switch (event_type)
                {
                    case card_xchan_data_available:
                        if ( event_params_rcv.data == NULL ) break;
                        pEvent = (EventCableCard_XChan*) event_params_rcv.data;
                        length = pEvent->extChDataLength > dataLen? dataLen:pEvent->extChDataLength;
                         memcpy( extendedData, pEvent->extChDataPtr, length);

                        rmf_osal_event_delete(event_handle_rcv);
                        return;
                }
                break;
            }
//Removing the following cases since it doesnt perform any actions.
/*
            case PFC_EVENT_CATEGORY_NONE:
            case RMF_OSAL_EVENT_CATEGORY_SYSTEM:
            case PFC_EVENT_CATEGORY_INPUT:
            case PFC_EVENT_CATEGORY_PROCESS:
            case PFC_EVENT_CATEGORY_RECORD_IO:
            case PFC_EVENT_CATEGORY_TUNER:
            case PFC_EVENT_CATEGORY_APP:
            case RMF_OSAL_EVENT_CATEGORY_MPEG:
            case PFC_EVENT_CATEGORY_TMS:
            case PFC_EVENT_CATEGORY_MEDIA_ENGINE:
            case PFC_EVENT_CATEGORY_PID_SELECTION:
            case PFC_EVENT_CATEGORY_DSMCC_API_REQ:
            case PFC_EVENT_CATEGORY_DMSCC_API_RESPONSE:
            case PFC_EVENT_CATEGORY_PHOTO:
            case PFC_EVENT_CATEGORY_MUSIC:
            case PFC_EVENT_CATEGORY_MOVIE:
            case PFC_EVENT_CATEGORY_STARTUP:
            case PFC_EVENT_CATEGORY_CONFIG_MGR:
            case PFC_EVENT_CATEGORY_INPUT_RELEASE:
            case PFC_EVENT_CATEGORY_DISPLAY_MGR:
            case PFC_EVENT_CATEGORY_SM:
            case PFC_EVENT_CATEGORY_INPUT_REPEAT:
            case RMF_OSAL_EVENT_CATEGORY_CARD_STATUS:
            case RMF_OSAL_EVENT_CATEGORY_RS_MGR:
            case RMF_OSAL_EVENT_CATEGORY_APP_INFO:
            case RMF_OSAL_EVENT_CATEGORY_MMI:
            case RMF_OSAL_EVENT_CATEGORY_HOMING:
            case RMF_OSAL_EVENT_CATEGORY_XCHAN:
            case RMF_OSAL_EVENT_CATEGORY_SYSTIME:
            case RMF_OSAL_EVENT_CATEGORY_HOST_CONTROL:
            case RMF_OSAL_EVENT_CATEGORY_CA:
            case RMF_OSAL_EVENT_CATEGORY_CP:
            case RMF_OSAL_EVENT_CATEGORY_SAS:
            case RMF_OSAL_EVENT_CATEGORY_SYS_CONTROL:
            case RMF_OSAL_EVENT_CATEGORY_GEN_FEATURE:
            case RMF_OSAL_EVENT_CATEGORY_GEN_DIAGNOSTIC:
            case RMF_OSAL_EVENT_CATEGORY_GATEWAY:
            case RMF_OSAL_EVENT_CATEGORY_POD_ERROR:
            case RMF_OSAL_EVENT_CATEGORY_CARD_RES:
            case RMF_OSAL_EVENT_CATEGORY_DSG:
            case PFC_EVENT_CATEGORY_IO_MANAGER:
            case RMF_OSAL_EVENT_CATEGORY_TABLE_MANAGER:
            case PFC_EVENT_CATEGORY_VIDEO_RESOURCE:
            case PFC_EVENT_CATEGORY_TUNER_PLUGIN:
            case PFC_EVENT_CATEGORY_MPAUDIO_PLUGIN:
            case PFC_EVENT_CATEGORY_MPVIDEO_PLUGIN:
            case PFC_EVENT_CATEGORY_DFBAMP_PLUGIN:
            case PFC_EVENT_CATEGORY_PLAYPUMP_PLUGIN:
            case PFC_EVENT_CATEGORY_PLAYLISTMGR:
            case PFC_EVENT_CATEGORY_DISKMGR:
            case PFC_EVENT_CATEGORY_PDB:
            case PFC_EVENT_CATEGORY_IPA:
            case PFC_EVENT_CATEGORY_MEDIA_MANAGER:
            case PFC_EVENT_CATEGORY_UPNPLIB:
            case PFC_EVENT_CATEGORY_CLOSED_CAPTION:
            case PFC_EVENT_CATEGORY_AILIB:
            case PFC_EVENT_CATEGORY_MIR:
            case RMF_OSAL_EVENT_CATEGORY_COM_DOWNL_MANAGER:
            case PFC_EVENT_CATEGORY_DOWNL_ENGINE:
            case PFC_EVENT_CATEGORY_DOWNL_ENGINE_INBAND_TUNE:
            case RMF_OSAL_EVENT_CATEGORY_CARD_MANAGER:
            case PFC_EVENT_CATEGORY_EAS_MANAGER:
            case PFC_EVENT_CATEGORY_EAS_CLIENT:
            case PFC_EVENT_CATEGORY_PSIP:
            case PFC_EVENT_CATEGORY_TBLM_PAT:
            case PFC_EVENT_CATEGORY_TBLM_CAT:
            case PFC_EVENT_CATEGORY_TBLM_PMT:
            case PFC_EVENT_CATEGORY_TBLM_EIT:
            case PFC_EVENT_CATEGORY_TBLM_DATA:
            case PFC_EVENT_CATEGORY_TBLM_DSM_CC:
            case PFC_EVENT_CATEGORY_TBLM_NIT:
            case PFC_EVENT_CATEGORY_TBLM_MGT:
            case PFC_EVENT_CATEGORY_TBLM_TVCT:
            case PFC_EVENT_CATEGORY_TBLM_CVCT:
            case PFC_EVENT_CATEGORY_TBLM_RRT:
            case PFC_EVENT_CATEGORY_TBLM_STT:
            case PFC_EVENT_CATEGORY_TBLM_DCCT:
            case PFC_EVENT_CATEGORY_TBLM_DCCSCT:
            case PFC_EVENT_CATEGORY_TBLM_ETT:
            case PFC_EVENT_CATEGORY_TBLM_EAM:
            case PFC_EVENT_CATEGORY_TBLM_SDT:
            case PFC_EVENT_CATEGORY_TBLM_BAT:
            case PFC_EVENT_CATEGORY_TBLM_RST:
            case PFC_EVENT_CATEGORY_TBLM_TDT:
            case PFC_EVENT_CATEGORY_TBLM_TOT:
            case PFC_EVENT_CATEGORY_COM_RECV:
            case PFC_EVENT_CATEGORY_COM_SEND:
            case PFC_EVENT_CATEGORY_COM_MGR:
            case PFC_EVENT_CATEGORY_COM_TIMER:
            case PFC_EVENT_CATEGORY_TIMER:
            case PFC_EVENT_CATEGORY_CHSEL_INTERNAL:
            case PFC_EVENT_CATEGORY_CHSEL:
            case PFC_EVENT_CATEGORY_CHSEL_INTERNAL_CHSELET:
            case PFC_EVENT_CATEGORY_CHSEL_INTERNAL_CHSELECT:
            case PFC_EVENT_CATEGORY_CH_INFO:
            case PFC_EVENT_CATEGORY_CH_INFO_CMD:
            case PFC_EVENT_CATEGORY_CH_INFO_CMD_RES:
            case PFC_EVENT_CATEGORY_CH_SCAN:
            case PFC_EVENT_CATEGORY_SCAN_CMD:
            case PFC_EVENT_CATEGORY_DTMAPLCTL:
            case PFC_EVENT_CATEGORY_COMMMGRIF:
            case PFC_EVENT_CATEGORY_KEYINFOCTL:
            case PFC_EVENT_CATEGORY_PROGRAMTIMERCTL:
            case PFC_EVENT_CATEGORY_PROGINFOCTL:
            case PFC_EVENT_CATEGORY_GUI:
            case PFC_EVENT_CATEGORY_POWER_STATUS:
            case PFC_EVENT_CATEGORY_POWER_STATUS_RES:
            case PFC_EVENT_CATEGORY_USBCTL:
            case PFC_EVENT_CATEGORY_USBCTL_NOTIFY:
            case PFC_EVENT_CATEGORY_SECTION_FILTER:
            case PFC_EVENT_CATEGORY_TEST:
            {
            }
            break;*/

        }
        rmf_osal_event_delete(event_handle_rcv);


    }
    return;
}


uint8_t cExtChannelFlow::GetStatus(){

    return this->status;
}

uint32_t cExtChannelFlow::GetFlowID(){

    return this->flowID;
}

void cExtChannelFlow::ResetFlowID(){

    this->flowID = INVALID_FLOW_ID;
}

uint8_t     cExtChannelFlow::GetFlowsRemaining(){
    return flows_Remaining;
}



void EventCableCard_XChan_free_fn(void *ptr)
{

     EventCableCard_XChan *event = (EventCableCard_XChan *)ptr;
     if(event)
     {
          delete event->extChDataPtr;
	      rmf_osal_memFreeP(RMF_OSAL_MEM_POD, ptr);
     }

}

