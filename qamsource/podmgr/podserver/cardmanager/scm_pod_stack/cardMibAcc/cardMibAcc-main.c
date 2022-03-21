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


 
//----------------------------------------------------------------------------
//
// System includes
#include <errno.h>

#include <pthread.h>
#include <global_event_map.h>
#include <global_queues.h>
#include "poddef.h"
#include <lcsm_log.h>

#include "utils.h"
#include "link.h"
#include "transport.h"
#include "session.h"
#include "appmgr.h"
#include "core_events.h"



#include "cardMibAcc_app.h"
#include "cardMibAcc-main.h"
#include "podhighapi.h"
#include <memory.h>
#include <string.h>

#include "rmf_osal_event.h"
#include "cardUtils.h"
//#include "cardMibAcc.h"
#include <rdk_debug.h>
#ifdef __cplusplus
extern "C" {
#endif


#define handle cardMibAccLCSMHandle
// Prototypes
void cardMibAccProc(void*);

// Globals
LCSM_DEVICE_HANDLE           cardMibAccLCSMHandle = 0;
LCSM_TIMER_HANDLE            cardMibAccTimer = 0;
static pthread_t             cardMibAccThread;
static pthread_mutex_t       cardMibAccLoopLock;
static int                   loop;

extern UCHAR  gcardresCurrentVersionNumber;
extern UCHAR  gcardresCurrentNextIndicator;

unsigned char vlg_CardMibRootOidLen = 0;
unsigned char vlg_CardMibRootOid[256];
// ******************* Session Stuff ********************************
//find our session number and are we open?
//scte 28 states that only 1 session active at a time
//this makes the implementation simple; we'll use two
//static globals
//jdmc
// below, may need functions to read/write with mutex pretection
static USHORT cardMibAccSessionNumber = 0;
static int    cardMibAccSessionState  = 0;
// above
static ULONG  ResourceId    = 0; 
static UCHAR  Tcid          = 0;       
static ULONG  tmpResourceId = 0; //tmp for binding
static UCHAR  tmpTcid       = 0; //tmp for binding

USHORT //sesnum
lookupcardMibAccSessionValue(void)
{
    if(  (cardMibAccSessionState == 1) && (cardMibAccSessionNumber) )
           return cardMibAccSessionNumber; 
//else
    return 0;
}


// ##################### Execution Space ##################################
// below executes in the caller's execution thread space.
// this may or may-not be in CA thread and APDU processing Space. 
// can occur before or after ca_start and caProcThread
//
//this was triggered from POD via appMgr bases on ResourceID.
//appMgr has translated the ResourceID to that of CA.
void AM_OPEN_SS_REQ_CARD_MIB_ACC(ULONG RessId, UCHAR Tcid)
{
    if (cardMibAccSessionState != 0)
       {//we're already open; hence reject.
           AM_OPEN_SS_RSP( SESS_STATUS_RES_BUSY, RessId, Tcid);
    //may want to
    //don't reject but also request that the pod close (perhaps) this orphaned session
    //AM_CLOSE_SS(caSessionNumber);
        tmpResourceId = 0;
           tmpTcid = 0;
           return;
       }
    tmpResourceId = RessId;
    tmpTcid = Tcid;
    AM_OPEN_SS_RSP( SESS_STATUS_OPENED, RessId, Tcid);
    return;
}


//spec say we can't fail here, but we will test and reject anyway
//verify that this open is in response to our
//previous AM_OPEN_SS_RSP...SESS_STATUS_OPENED

void AM_SS_OPENED_CARD_MIB_ACC(USHORT SessNumber, ULONG lResourceId, UCHAR Tcid)
{
    if (cardMibAccSessionState != 0)
       {//we're already open; hence reject.
           AM_OPEN_SS_RSP( SESS_STATUS_RES_BUSY, lResourceId, Tcid);
           tmpResourceId = 0;
           tmpTcid = 0;
           return;
       }
    if (lResourceId != tmpResourceId)
       {//wrong binding hence reject.
           AM_OPEN_SS_RSP( SESS_STATUS_RES_BUSY, lResourceId, Tcid);
           tmpResourceId = 0;
           tmpTcid = 0;
           return;
       }
    if (Tcid != tmpTcid)
       {//wrong binding hence reject.
           AM_OPEN_SS_RSP( SESS_STATUS_RES_BUSY, lResourceId, Tcid);
           tmpResourceId = 0;
           tmpTcid = 0;
           return;
       }
    ResourceId      = lResourceId;
    Tcid            = Tcid;
    cardMibAccSessionNumber = SessNumber;
    cardMibAccSessionState  = 1;
    cardMibAccRootOidReq(SessNumber);
//not sure we send this here, I believe its assumed to be successful
//AM_OPEN_SS_RSP( SESS_STATUS_OPENED, ResourceId, Tcid);
    return;
}

//after the fact, info-ing that closure has occured.
void AM_SS_CLOSED_CARD_MIB_ACC(USHORT SessNumber)
{
    if((cardMibAccSessionState)  && (cardMibAccSessionNumber)  && (cardMibAccSessionNumber != SessNumber ))
       {
           //don't reject but also request that the pod close (perhaps) this orphaned session
           AM_CLOSE_SS(cardMibAccSessionNumber);
       }
    ResourceId      = 0;
    Tcid            = 0;
    tmpResourceId   = 0;
    tmpTcid         = 0;
    cardMibAccSessionNumber = 0;
    cardMibAccSessionState  = 0;
    vlg_CardMibRootOidLen = 0;
}
// ##################### Execution Space ##################################
// ************************* Session Stuff **************************

// Inline functions
static inline void setLoop(int cont)
{
    pthread_mutex_lock(&cardMibAccLoopLock);
    if (!cont)
    {
        loop = 0;
    }
    else
    {
        loop = 1;
    }
    pthread_mutex_unlock(&cardMibAccLoopLock);
}

// Sub-module entry point
int cardMibAcc_start(LCSM_DEVICE_HANDLE h)
{
    int res;

    if (h < 1)
    {
        MDEBUG (DPM_ERROR, "ERROR:ca: Invalid LCSM device handle.\n");
        // This failure should be logged by the application manager
        return -EINVAL;
    }
    // Assign handle
    cardMibAccLCSMHandle = h;
    // Flush queue
 //   lcsm_flush_queue(cardresLCSMHandle, POD_CA_QUEUE); Hannah

    //Initialize CA's loop mutex
    if (pthread_mutex_init(&cardMibAccLoopLock, NULL) != 0) 
    {
        MDEBUG (DPM_ERROR, "ERROR:cardres: Unable to initialize mutex.\n");
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}

// Sub-module exit point
int cardMibAcc_stop()
{
    if ( (cardMibAccSessionState) && (cardMibAccSessionNumber))
    {
       //don't reject but also request that the pod close (perhaps) this orphaned session
       AM_CLOSE_SS(cardMibAccSessionNumber);
    }

    // Terminate message loop
    setLoop(0);

    // Wait for message processing thread to join
    if (pthread_join(cardMibAccThread, NULL) != 0)
    {
        MDEBUG (DPM_ERROR, "ERROR:ca: pthread_join failed.\n");
    }
    cardMibAccLCSMHandle = 0;
    return EXIT_SUCCESS;//Mamidi:042209
}


void cardMibAcc_init(void)
{
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," ########## cardMibAcc_init:: calling REGISTER_RESOURCE ############## \n");
    REGISTER_RESOURCE(M_CARD_MIB_ACC_ID, POD_CARD_MIB_ACC_QUEUE, 1);  // (resourceId, queueId, maxSessions )
}
int process_Root_Oid_Reply(uint16 sessNum, uint8 *apdu, uint16 apduLen)
{
   int ii;
   

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","process_Root_Oid_Reply : ENTERED @@@@@@@@@@@ \n");
    


    for(ii = 0; ii < apduLen; ii++)
    {
       RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","0x%02X ",apdu[ii]);
    if(((ii + 1) % 16) == 0)
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","\n");
    }
  if((apduLen - 4) > 255)
    return-1;
    vlg_CardMibRootOidLen = apdu[3];
    rmf_osal_memcpy(vlg_CardMibRootOid,&apdu[4],(apduLen - 4), sizeof(vlg_CardMibRootOid), (apduLen - 4));
     
    vlg_CardMibRootOid[(apduLen - 4)] = 0;

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","\n Root Oid:%s \n",vlg_CardMibRootOid);
    {
        rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
        rmf_osal_event_handle_t event_handle;
        rmf_osal_event_params_t event_params = {0};    
    
        event_params.priority = 0; //Default priority;
        event_params.data = (void *)vlg_CardMibRootOid;
        event_params.data_extension = vlg_CardMibRootOidLen;
        event_params.free_payload_fn = NULL;
        rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_CARD_MANAGER, CardMgr_Card_Mib_Access_Root_OID, 
    		&event_params, &event_handle);
    
        rmf_osal_eventmanager_dispatch_event(em, event_handle);

    }
    return EXIT_SUCCESS;
}
int process_Snmp_Reply(uint16 sessNum, uint8 *apdu, uint16 apduLen)
{
   int ii;
 
    
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","process_Snmp_Reply : ENTERED @@@@@@@@@@@ \n");
    
   if(apdu == NULL || apduLen == 0)
   {
    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","process_Snmp_Reply: apdu == NULL || apduLen == 0 \n");
    return -1;
   }
    for(ii = 0; ii < apduLen; ii++)
    {
       RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","0x%02X ",apdu[ii]);
    if(((ii + 1) % 16) == 0)
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","\n");
    }
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","\n");

   
    return cardMibAccSnmpReply(apdu,apduLen);

   
}

void cardMibAccProc(void* par)
{
    char *ptr;
    LCSM_EVENT_INFO       *eventInfo = (LCSM_EVENT_INFO       *)par;

    setLoop(1);

//    handle = cardMibAccLCSMHandle;        // because REGISTER_RESOURCE macro expects 'handle'

    switch (eventInfo->event)
    {

        // POD requests open session (x=resId, y=tcId)
    case POD_OPEN_SESSION_REQ:
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","cardMibAccProc: << POD_OPEN_SESSION_REQ >>\n");
        AM_OPEN_SS_REQ_CARD_MIB_ACC((uint32)eventInfo->x, (uint8)eventInfo->y);
            break;

        // POD confirms open session (x=sessNbr, y=resId, z=tcId)
    case POD_SESSION_OPENED:
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","cardMibAccProc: << POD_SESSION_OPENED >>\n");
        AM_SS_OPENED_CARD_MIB_ACC((uint16)eventInfo->x, (uint32)eventInfo->y, (uint8)eventInfo->z);
                break;

        // POD closes session (x=sessNbr)
    case POD_CLOSE_SESSION:
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","cardMibAccProc: << POD_CLOSE_SESSION >>\n");
        AM_SS_CLOSED_CARD_MIB_ACC((uint16)eventInfo->x);
        {
            rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
            rmf_osal_event_handle_t event_handle;

            rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_CARD_MANAGER, CardMgr_Card_Mib_Access_Shutdown, 
        		NULL, &event_handle);        
            rmf_osal_eventmanager_dispatch_event(em, event_handle);

        }
    }
    break;

    case POD_APDU:
        ptr = (char *)eventInfo->y;    // y points to the complete APDU
        switch (*(ptr+2))
        {
        case POD_CARD_MIB_ACC_SNMP_REQ:
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Received POD_CARD_MIB_ACC_SNMP_REQ \n");
            break;

        case POD_CARD_MIB_ACC_SNMP_REPLY:
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Received POD_CARD_MIB_ACC_SNMP_REPLY \n");
            process_Snmp_Reply((uint16)eventInfo->x, (uint8 *)eventInfo->y, (uint16)eventInfo->z);
            break;

        case POD_CARD_MIB_ACC_ROOToid_REQ:
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Received POD_CARD_MIB_ACC_ROOToid_REQ \n");
            break;

        case POD_CARD_MIB_ACC_ROOToid_REPLY:
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Received POD_CARD_MIB_ACC_ROOToid_REPLY \n");
            process_Root_Oid_Reply((uint16)eventInfo->x, (uint8 *)eventInfo->y, (uint16)eventInfo->z);
            break;

    
            default:
                break;
        }
        break;



        case EXIT_REQUEST:
        //    setLoop(0);
            break;

        default:
            break;
        }

   // }
    //pthread_exit(NULL);
    setLoop(0);
}


#ifdef __cplusplus
}
#endif


