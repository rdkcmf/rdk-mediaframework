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



#include "hostAddPrt.h"
#include "hostAddPrt-main.h"
#include "podhighapi.h"
#include <memory.h>

#include "core_events.h"
#include "cardUtils.h"
#include "rmf_osal_event.h"
//#include "cardMibAcc.h"
#include <rdk_debug.h>
#ifdef __cplusplus
extern "C" {
#endif


#define handle hostAddPrtLCSMHandle
// Prototypes
void hostAddPrtProc(void*);

// Globals
LCSM_DEVICE_HANDLE           hostAddPrtLCSMHandle = 0;
LCSM_TIMER_HANDLE            hostAddPrtTimer = 0;
static pthread_t             hostAddPrtThread;
static pthread_mutex_t       hostAddPrtLoopLock;
static int                   loop;



// ******************* Session Stuff ********************************
//find our session number and are we open?
//scte 28 states that only 1 session active at a time
//this makes the implementation simple; we'll use two
//static globals
//jdmc
// below, may need functions to read/write with mutex pretection
static USHORT hostAddPrtSessionNumber = 0;
static int    hostAddPrtSessionState  = 0;
// above
static ULONG  ResourceId    = 0; 
static UCHAR  Tcid          = 0;       
static ULONG  tmpResourceId = 0; //tmp for binding
static UCHAR  tmpTcid       = 0; //tmp for binding

USHORT //sesnum
lookuphostAddPrtSessionValue(void)
{
    if(  (hostAddPrtSessionState == 1) && (hostAddPrtSessionNumber) )
           return hostAddPrtSessionNumber; 
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
void AM_OPEN_SS_REQ_HOST_ADR_PRT(ULONG RessId, UCHAR Tcid)
{
    if (hostAddPrtSessionState != 0)
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

void AM_SS_OPENED_HOST_ADR_PRT(USHORT SessNumber, ULONG lResourceId, UCHAR Tcid)
{
    if (hostAddPrtSessionState != 0)
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
    hostAddPrtSessionNumber = SessNumber;
    hostAddPrtSessionState  = 1;
//not sure we send this here, I believe its assumed to be successful
//AM_OPEN_SS_RSP( SESS_STATUS_OPENED, ResourceId, Tcid);
    return;
}

//after the fact, info-ing that closure has occured.
void AM_SS_CLOSED_HOST_ADD_PRT(USHORT SessNumber)
{
    if((hostAddPrtSessionState)  && (hostAddPrtSessionNumber)  && (hostAddPrtSessionNumber != SessNumber ))
       {
           //don't reject but also request that the pod close (perhaps) this orphaned session
           AM_CLOSE_SS(hostAddPrtSessionNumber);
       }
    ResourceId      = 0;
    Tcid            = 0;
    tmpResourceId   = 0;
    tmpTcid         = 0;
    hostAddPrtSessionNumber = 0;
    hostAddPrtSessionState  = 0;
}
// ##################### Execution Space ##################################
// ************************* Session Stuff **************************

// Inline functions
static inline void setLoop(int cont)
{
    pthread_mutex_lock(&hostAddPrtLoopLock);
    if (!cont)
    {
        loop = 0;
    }
    else
    {
        loop = 1;
    }
    pthread_mutex_unlock(&hostAddPrtLoopLock);
}

// Sub-module entry point
int hostAddPrt_start(LCSM_DEVICE_HANDLE h)
{
    int res;

    if (h < 1)
    {
        MDEBUG (DPM_ERROR, "ERROR:hostAddPrt_start: Invalid LCSM device handle.\n");
        // This failure should be logged by the application manager
        return -EINVAL;
    }
    // Assign handle
    hostAddPrtLCSMHandle = h;
    // Flush queue
 //   lcsm_flush_queue(cardresLCSMHandle, POD_CA_QUEUE); Hannah

    //Initialize CA's loop mutex
    if (pthread_mutex_init(&hostAddPrtLoopLock, NULL) != 0) 
    {
        MDEBUG (DPM_ERROR, "ERROR:hostAddPrt_start: Unable to initialize mutex.\n");
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}

// Sub-module exit point
int hostAddPrt_stop()
{
    if ( (hostAddPrtSessionState) && (hostAddPrtSessionNumber))
    {
       //don't reject but also request that the pod close (perhaps) this orphaned session
       AM_CLOSE_SS(hostAddPrtSessionNumber);
    }

    // Terminate message loop
    setLoop(0);

    // Wait for message processing thread to join
    if (pthread_join(hostAddPrtThread, NULL) != 0)
    {
        MDEBUG (DPM_ERROR, "ERROR:ca: hostAddPrt_stop hostAddPrt_stop failed.\n");
    }
    hostAddPrtLCSMHandle = 0;
    return EXIT_SUCCESS;//Mamidi:042209
}


void hostAddPrt_init(void)
{
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," ########## hostAddPrt_init:: calling REGISTER_RESOURCE ############## \n");
    REGISTER_RESOURCE(M_HOST_ADD_PRT, POD_HOST_ADD_PRT_QUEUE, 1);  // (resourceId, queueId, maxSessions )
}

int process_host_app_prt_Reply(uint16 sessNum, uint8 *apdu, uint16 apduLen)
{
   int ii;
 
    
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","process_host_app_prt_Reply : ENTERED @@@@@@@@@@@ \n");
    
   if(apdu == NULL || apduLen == 0)
   {
    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","process_host_app_prt_Reply: apdu == NULL || apduLen == 0 \n");
    return -1;
   }
    for(ii = 0; ii < apduLen; ii++)
    {
       RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","0x%02X ",apdu[ii]);
    if(((ii + 1) % 16) == 0)
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","\n");
    }
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","\n");

    return 0;
   
    
   
}

void hostAddPrtProc(void* par)
{
    char *ptr;
    LCSM_EVENT_INFO       *eventInfo = (LCSM_EVENT_INFO       *)par;

    setLoop(1);

//    handle = hostAddPrtLCSMHandle;        // because REGISTER_RESOURCE macro expects 'handle'

    switch (eventInfo->event)
    {

        // POD requests open session (x=resId, y=tcId)
    case POD_OPEN_SESSION_REQ:
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","hostAddPrtProc: << POD_OPEN_SESSION_REQ >>\n");
        AM_OPEN_SS_REQ_HOST_ADR_PRT((uint32)eventInfo->x, (uint8)eventInfo->y);
            break;

        // POD confirms open session (x=sessNbr, y=resId, z=tcId)
    case POD_SESSION_OPENED:
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","hostAddPrtProc: << POD_SESSION_OPENED >>\n");
        AM_SS_OPENED_HOST_ADR_PRT((uint16)eventInfo->x, (uint32)eventInfo->y, (uint8)eventInfo->z);
    /*    {
            unsigned char Buff[16];
            
            Buff[0] = 0x02;//No of properties
            Buff[1] = 0x04;//key length
            Buff[2] = '0x01';
            Buff[3] = '0x01';
            Buff[4] = '0x02';
            Buff[5] = '0x02';
            Buff[6] = 0x04;//key length
            Buff[7] = '0x03';
            Buff[8] = '0x03';
            Buff[9] = '0x04';
            Buff[10] = '0x04';
        hostAddPrtReq(Buff, 11);
        }*/
                break;

        // POD closes session (x=sessNbr)
    case POD_CLOSE_SESSION:
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","hostAddPrtProc: << POD_CLOSE_SESSION >>\n");
        AM_SS_CLOSED_HOST_ADD_PRT((uint16)eventInfo->x);
        {

        }
    }
    break;

    case POD_APDU:
        ptr = (char *)eventInfo->y;    // y points to the complete APDU
        switch (*(ptr+2))
        {
        case HOSTADRPRT_TAG_REQ:
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Received HOSTADRPRT_TAG_REQ \n");
            break;

        case HOSTADRPRT_TAG_REPLY:
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Received HOSTADRPRT_TAG_REPLY \n");
            process_host_app_prt_Reply((uint16)eventInfo->x, (uint8 *)eventInfo->y, (uint16)eventInfo->z);
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


