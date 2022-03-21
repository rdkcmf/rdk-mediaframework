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



// System includes
#include <errno.h>

// MR includes
//#include <pthreadLibrary.h>
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
#include "podhighapi.h"    
#include "podhighapi.h"
//#include "debug.h"
#include "hc_app.h"
#include "ob_qpsk.h"
#include "pod_support.h"
#include <string.h>
#include <rdk_debug.h>
#include <rmf_osal_thread.h>

#ifdef __cplusplus
extern "C" {
#endif

// Prototypes
void hostProc(void*);

extern void setFirmupgradeFlag( Bool on );
extern uint32 ndstytpe;

extern uint8  gExpectingOOBevent;
extern uint32 gExpectingIBevent;
extern uint32 gExpectingIBeventNext;

// Globals
static LCSM_DEVICE_HANDLE    handle = 0;
static pthread_t             hostThread;
static pthread_mutex_t       hostLoopLock;
static int                   loop;

extern int vlg_InbanTuneSuccess;
extern int vlg_InbanTuneStart;
// ******************* Session Stuff ********************************
//find our session number and are we open?
//scte 28 states that only 1 session active at a time
//this makes the implementation simple; we'll use two
//static globals
//jdmc
// below, may need functions to read/write with mutex pretection
static USHORT hcSessionNumber = 0;
static int    hcSessionState  = 0;
// above
static ULONG  ResourceId    = 0; 
static UCHAR  Tcid          = 0;       
static ULONG  tmpResourceId = 0; //tmp for binding
static UCHAR  tmpTcid       = 0; //tmp for binding

bool vlg_JavaCTPEnabled;

USHORT //sesnum
lookupHCSessionValue(void)
{
if (  (hcSessionState == 1)
   && (hcSessionNumber)
   )
   return hcSessionNumber; 
//else
return 0;
}


// ##################### Execution Space ##################################
// below executes in the caller's execution thread space.
// this may or may-not be in thread and APDU processing Space. 
// can occur before or after hc_start and hcProcThread
//
//this was triggered from POD via appMgr bases on ResourceID.
//appMgr has translated the ResourceID to that of CA.
void
AM_OPEN_SS_REQ_HC(ULONG RessId, UCHAR Tcid)
{
    if (hcSessionState != 0)
       {//we're already open; hence reject.
           AM_OPEN_SS_RSP( SESS_STATUS_RES_BUSY, RessId, Tcid);
    //may want to
    //don't reject but also request that the pod close (perhaps) this orphaned session
    //AM_CLOSE_SS(hcSessionNumber);

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
void
AM_SS_OPENED_HC(USHORT SessNumber, ULONG lResourceId, UCHAR Tcid)
{
   gExpectingOOBevent = 0;
   gExpectingIBevent = 0;
   gExpectingIBeventNext = 0;

if (hcSessionState != 0)
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
hcSessionNumber = SessNumber;
hcSessionState  = 1;
//not sure we send this here, I believe its assumed to be successful
//AM_OPEN_SS_RSP( SESS_STATUS_OPENED, ResourceId, Tcid);
return;
}

//after the fact, info-ing that closure has occured.
void
AM_SS_CLOSED_HC(USHORT SessNumber)
{
if (  (hcSessionState)
   && (hcSessionNumber)
   && (hcSessionNumber != SessNumber )
   )
   {
   //don't reject but also request that the pod close (perhaps) this orphaned session
   AM_CLOSE_SS(hcSessionNumber);
   }
ResourceId      = 0;
Tcid            = 0;
tmpResourceId   = 0;
tmpTcid         = 0;
hcSessionNumber = 0;
hcSessionState  = 0;
gExpectingOOBevent = 0;
gExpectingIBevent  = 0;
gExpectingIBeventNext = 0;
}
// ##################### Execution Space ##################################
// ************************* Session Stuff **************************




// Inline functions
static inline void setLoop(int cont)
{
    pthread_mutex_lock(&hostLoopLock);
    if (!cont) {
        loop = 0;
    }
    else {
        loop = 1;
    }
    pthread_mutex_unlock(&hostLoopLock);
}

// Sub-module entry point
int host_start(LCSM_DEVICE_HANDLE h)
{
    int res;

    MDEBUG (DPM_HOST, "pod > host : Entering \n");

    if (h < 1) {
        MDEBUG (DPM_ERROR, "ERROR:host: Invalid LCSM device handle.\n");
        // This failure should be logged by the application manager
        return -EINVAL;
    }
    // Assign handle
    handle = h;
    // Flush queue
 //   lcsm_flush_queue(handle, POD_HOST_QUEUE); Hannah

    //Initialize HOST's loop mutex
    if (pthread_mutex_init(&hostLoopLock, NULL) != 0) {
        MDEBUG (DPM_ERROR, "ERROR:host: Unable to initialize mutex.\n");
        return EXIT_FAILURE;
    }

    /* COMMENTED BY Hannah check if this is required
   if (  (hcSessionState)
      && (hcSessionNumber)
      )
      {
      //don't reject but also request that the pod close (perhaps) this orphaned session
      AM_CLOSE_SS(hcSessionNumber);
      }
*/

    /* Start HOST processing thread
       For now just use default settings for the new thread. It's more important
       to be able to check the return value that would otherwise be swallowed by
       the pthread wrapper library call*/
//    res = (int) pthread_createThread((void *)&hostProcThread, NULL, DEFAULT_STACK_SIZE, 0 ); Hannah
    //res = pthread_create(&hostThread, NULL, (void *)&hostProcThread, NULL);
   // if (res == 0) {
    //    MDEBUG (DPM_ERROR, "ERROR:host: Unable to create thread.\n");
    //    return EXIT_FAILURE;
    //}

    return EXIT_SUCCESS;
}

// Sub-module exit point
int host_stop()
{
    if (  (hcSessionState)
       && (hcSessionNumber)
       )
       {
       //don't reject but also request that the pod close (perhaps) this orphaned session
       AM_CLOSE_SS(hcSessionNumber);
       }


    // Terminate message loop
    setLoop(0);

    // Wait for message processing thread to join
    if (pthread_join(hostThread, NULL) != 0) {
        MDEBUG (DPM_ERROR, "ERROR:host: pthread_join failed.\n");
    }
    return EXIT_SUCCESS;//Mamidi:042209

}


void hostControlInit(void){

    REGISTER_RESOURCE(M_HOST_CTL_ID, POD_HOST_QUEUE, 1);  // (resourceId, queueId, maxSessions )

}

void hostProc(void* par)
{
    char *ptr;
    
    int  rtnVal;
    LCSM_EVENT_INFO       *eventInfo = (LCSM_EVENT_INFO       *)par;
    rtnVal = 0;
  //  setLoop(1);

 //   MDEBUG (DPM_ALWAYS, "Thread started, (PID=%d)\n", (int) getpid() );

    // Register with resource manager
    //REGISTER_RESOURCE(HOST_CTL_ID, POD_HOST_QUEUE, 1);  // (resourceId, queueId, maxSessions )

//    lcsm_register_event(handle, NDS_POD_DETECTED, POD_HOST_QUEUE ); Hannah
 //   lcsm_register_event(handle, POD_HOST_FIRMUPGRADE_START, POD_HOST_QUEUE );  Hannah
 //   lcsm_register_event(handle, POD_HOST_FIRMUPGRADE_END, POD_HOST_QUEUE );        Hannah
 //   MDEBUG (DPM_HOST, "pod > Host Control: Registered for post event \n");

  //  while(loop)
        {
   //     lcsm_get_event( handle, POD_HOST_QUEUE, &eventInfo, LCSM_WAIT_FOREVER );  Hannah

        if (  (eventInfo->event == POD_APDU)
           && (eventInfo->x /*sessNbrsesnum*/ != lookupHCSessionValue())
           )
           {
           rtnVal = APDU_HC_FAILURE;
           return;//continue;
           }
        //else sesnum is now set to proper value

        switch (eventInfo->event)
            {
            // POD requests open session (x=resId, y=tcId)
            case POD_OPEN_SESSION_REQ:
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," >>>> POD_OPEN_SESSION_REQ>> Host Control \n");
                AM_OPEN_SS_REQ_HC(eventInfo->x, eventInfo->y);
                break;

            // POD confirms open session (x=sessNbr, y=resId, z=tcId)
            case POD_SESSION_OPENED:
                AM_SS_OPENED_HC(eventInfo->x, eventInfo->y, eventInfo->z);
                break;

            // POD closes session (x=sessNbr)
            case POD_CLOSE_SESSION:
                AM_SS_CLOSED_HC(eventInfo->x);
                break;

            // 
            case NDS_POD_DETECTED:
                MDEBUG (DPM_HOST, "pod > Host Control: NDS_POD_DETECTED = 0x%x \n", eventInfo->x);
                ndstytpe = FALSE;
                if (eventInfo->x)
                   ndstytpe = TRUE;
                break;

            // POD sends APDU (x=sessNbr, y=APDU buffer ptr, z=APDU byte length)
            case POD_APDU:
                ptr = (char *)eventInfo->y;    // y points to the complete APDU
                switch (*(ptr+2))
                    {
                    case POD_HOST_OOBTX_TUNEREQ:  //0x9F, 0x84, 0x04, "OOB_TX_tune_req"
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","hostProc:  >>>>>>>>>>>> POD_HOST_OOBTX_TUNEREQ <<<<<<<<<<<<<<<<<< \n");
                        rtnVal = APDU_OOBTxTuneReq ((USHORT)eventInfo->x, (UCHAR *)eventInfo->y, (USHORT)eventInfo->z);  //(sesnum, apkt, len)
                        if (rtnVal == APDU_HC_FAILURE)
                           MDEBUG (DPM_WARN, "WARN: pod > Host CTL: BAD POD_HOST__OOBTX_TUNEREQ attemped \n");                       
                        break;
                    case POD_HOST_OOBTX_TUNECNF:  //0x9F, 0x84, 0x05, "OOB_TX_tune_cnf"
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","hostProc:  >>>>>>>>>>>> POD_HOST_OOBTX_TUNECNF <<<<<<<<<<<<<<<<<< \n");
                        rtnVal = APDU_OOBTxTuneCnf ((USHORT)eventInfo->x, (UCHAR *)eventInfo->y, (USHORT)eventInfo->z);  //(sesnum, apkt, len)
                        if (rtnVal == APDU_HC_FAILURE)
                           MDEBUG (DPM_WARN, "WARN: pod > Host CTL: BAD POD_HOST_OOB_TX_TUNECNF attemped \n");                         
                        break;
                    case POD_HOST_OOBRX_TUNEREQ: //0x9F, 0x84, 0x06, "OOB_RX_tune_req"
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","hostProc:  >>>>>>>>>>>> POD_HOST_OOBRX_TUNEREQ <<<<<<<<<<<<<<<<<< \n");
                        rtnVal = APDU_OOBRxTuneReq ((USHORT)eventInfo->x, (UCHAR *)eventInfo->y, (USHORT)eventInfo->z);  //(sesnum, apkt, len)
                        if (rtnVal == APDU_HC_FAILURE)
                           MDEBUG (DPM_WARN, "WARN: pod > Host CTL: BAD POD_HOST_OOBRX_TUNEREQ attemped \n");  
                        break;
                    case POD_HOST_OOBRX_TUNECNF: //0x9F, 0x84, 0x07, "OOB_RX_tune_cnf"
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","hostProc:  >>>>>>>>>>>> POD_HOST_OOBRX_TUNECNF <<<<<<<<<<<<<<<<<< \n");
                        rtnVal = APDU_OOBRxTuneCnf ((USHORT)eventInfo->x, (UCHAR *)eventInfo->y, (USHORT)eventInfo->z);  //(sesnum, apkt, len)
                        if (rtnVal == APDU_HC_FAILURE)
                           MDEBUG (DPM_WARN, "WARN: pod > Host CTL: BAD POD_HOST_OOBRX_TUNECNF attemped \n");  
                        break;
                    case POD_HOST_IBTUNE:  //0x9F, 0x84, 0x08, "inband_tune"
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","hostProc:  >>>>>>>>>>>> POD_HOST_IBTUNE <<<<<<<<<<<<<<<<<< \n");

#ifndef OOB_TUNE            
            if(vlg_JavaCTPEnabled == FALSE)
            {
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," >>>>>>>>>>>>>>>> vlg_JavaCTPEnabled = FALSE <<<<<<<<<<<<<<<<< \n");
                            rtnVal = APDU_InbandTune ((USHORT)eventInfo->x, (UCHAR *)eventInfo->y, (USHORT)eventInfo->z);    //(sesnum, apkt, len)
                            if (rtnVal == APDU_HC_FAILURE)
                                   MDEBUG (DPM_WARN, "WARN: pod > Host CTL: BAD POD_HOST_IBTUNE attemped \n");
            }
            else
            {
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," >>>>>>>>>>>>>>>> vlg_JavaCTPEnabled = TRUE <<<<<<<<<<<<<<<<< \n");
                RespInbandTuneCnf(INBAND_TUNE_TUNER_BUSY, eventInfo->y);
            }
#endif
                        break;
                    case POD_HOST_IBTUNE_CNF:  //0x9F, 0x84, 0x09, "inband_tune_cnf"
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","hostProc:  >>>>>>>>>>>> POD_HOST_IBTUNE_CNF <<<<<<<<<<<<<<<<<< \n");
                        APDU_InbandTuneCnf ((USHORT)eventInfo->x, (UCHAR *)eventInfo->y, (USHORT)eventInfo->z);
                        rtnVal = APDU_InbandTuneCnf ((USHORT)eventInfo->x, (UCHAR *)eventInfo->y, (USHORT)eventInfo->z); //(sesnum, apkt, len)
                        if (rtnVal == APDU_HC_FAILURE)
                           MDEBUG (DPM_WARN, "WARN: pod > Host CTL: BAD POD_HOST_IBTUNE_CNF attemped \n");                                
                        break;
                    default:
                        MDEBUG (DPM_WARN, "WARN: pod > Host CTL: Receive bad POD_APDU msg \n");
                        break;
                    }  //end APDU switch
                break;

            case POD_HC_OOB_TUNE_RESP:  // this is a timeout from ourselves
            case FRONTEND_OOB_DS_TUNE_STATUS: // this one is a tune (or timeout) from frontendControl
        {
         int retValue;
                 retValue = RespOOBRxTuneCnf(eventInfo->x); //send back ADPU response
                 break;  //break from case being FRONTEND_OOB_DS_TUNE_STATUS or POD_HC_OOB_TUNE_RESP
        }

         
            case FRONTEND_OOB_US_TUNE_STATUS: // this one is a tune (or timeout) from frontendControl
                 RespOOBTxTuneCnf(eventInfo->x); //send back ADPU response
                 break;  //break from case being FRONTEND_OOB_DS_TUNE_STATUS or POD_HC_OOB_TUNE_RESP         
         
            case POD_HC_IB_TUNE_RESP:  //this event can come from 2 sources, In-Band tune or TimeOut for IB tune to complete
            case MR_POD_HOMING_TUNE_FAILED:
                 RespInbandTuneCnf(eventInfo->y, eventInfo->sequenceNumber); //send back ADPU response
                 break;  //break from case being MR_CHANNEL_TUNE_FAILED or POD_HC_IB_TUNE_RESP
            case MR_CHANNEL_TUNE_SUCCEEDED:
                 RespInbandTuneCnf(OOB_DS_QPSK_TUNE_OK, eventInfo->sequenceNumber); //ie: success, send back ADPU response
                 break;  //break from case being MR_CHANNEL_TUNE_FAILED or POD_HC_IB_TUNE_RESP

            case POD_HOST_FIRMUPGRADE_START:
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," received >>>>>>>> POD_HOST_FIRMUPGRADE_START <<<<<<<<< \n");
                setFirmupgradeFlag( TRUE );
                break;
    
            case POD_HOST_FIRMUPGRADE_END:
                setFirmupgradeFlag( FALSE );
                break;
        case FRONTEND_INBAND_TUNE_STATUS:

        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," received >>>>>>>> FRONTEND_INBAND_TUNE_STATUS <<<<<<<<< \n");
//        if(vlg_InbanTuneStart == 1)
        {
                
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD",">>>>>>>>> Status:%d ltSid:%d \n",eventInfo->x,eventInfo->y );
            if(eventInfo->x == 0)
            {
                
                
                RespInbandTuneCnf(eventInfo->x, eventInfo->y);
                //RespInbandTuneCnf(0, 2);
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD",">>>>>>>>> Tune Success \n");
                rmf_osal_threadSleep(2000, 0);  
                vlg_InbanTuneStart = 0;
                vlg_InbanTuneSuccess = 1;
            }
            else
            {
                RespInbandTuneCnf(0, eventInfo->y);
                //RespInbandTuneCnf(1, 2);
                vlg_InbanTuneStart = 0;
            }
            
        }
        break;
            case EXIT_REQUEST:
                setLoop(0);
                break;
            case FRONTEND_OOB_US_TUNE_STATUS_SENT:
                break;
            default:
                MDEBUG (DPM_WARN, "WARN: pod > Host CTL: Receive unhandled POD msg 0x%x:%d to our POD_HC_QUEUE\n", eventInfo->event, (eventInfo->event-FRONTEND_CONTROL_OFFSET));
                break;

            }  //end msg switch eventInfo->event
        }  //end while
    //ndstytpe = FALSE; Hannah commented this
 //   lcsm_unregister_event( handle, NDS_POD_DETECTED, POD_HOST_QUEUE ); Hannah
 //   lcsm_unregister_event( handle, POD_HOST_FIRMUPGRADE_START, POD_HOST_QUEUE ); Hannah
  //  lcsm_unregister_event( handle, POD_HOST_FIRMUPGRADE_END, POD_HOST_QUEUE );    Hannah
  //  pthread_exit(NULL);
}

#ifdef __cplusplus
}
#endif

