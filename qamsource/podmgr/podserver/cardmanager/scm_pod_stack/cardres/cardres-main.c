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



#include "cardres_app.h"
#include "cardres-main.h"
#include "podhighapi.h"
#include <memory.h>
#include <rdk_debug.h>
#include "rmf_osal_sync.h"
#include "rmf_osal_thread.h"
#include "rmf_osal_util.h"
#include "cardManagerIf.h"

#ifdef __cplusplus
extern "C" {
#endif


#define handle cardresLCSMHandle
// Prototypes
void cardresProc(void*);

// Globals
LCSM_DEVICE_HANDLE           cardresLCSMHandle = 0;
LCSM_TIMER_HANDLE            cardresTimer = 0;
static pthread_t             cardresThread;
static pthread_mutex_t       cardresLoopLock;
static int                   loop;

static rmf_osal_ThreadId     caProfileMonTaskID = 0;
       rmf_osal_Cond         caProfileCond;
static bool                  enableCardResWait = false;
static int                   cardResWaitTimeInSec=30;
void caProfileMonitor(void *);

extern UCHAR  gcardresCurrentVersionNumber;
extern UCHAR  gcardresCurrentNextIndicator;
extern int vlCaGetMaxProgramsSupportedByHost();

// ******************* Session Stuff ********************************
//find our session number and are we open?
//scte 28 states that only 1 session active at a time
//this makes the implementation simple; we'll use two
//static globals
//jdmc
// below, may need functions to read/write with mutex pretection
static USHORT cardresSessionNumber = 0;
static int    cardresSessionState  = 0;
// above
static ULONG  ResourceId    = 0; 
static UCHAR  Tcid          = 0;       
static ULONG  tmpResourceId = 0; //tmp for binding
static UCHAR  tmpTcid       = 0; //tmp for binding

// As of CCIP 2.0 I18 spec
static UCHAR   vlg_MaxNumTSSupportedByCard = 4;// default
static UCHAR   vlg_MaxNumProgramsSupportedByCard = 4;// default
static UCHAR   vlg_MaxNumElmStreamsSupportedByCard = 16;// default
static rmf_osal_Mutex vlg_vlCardResNumStrmsMutex;

static USHORT //sesnum
lookupCardResSessionValue(void)
{
    if(  (cardresSessionState == 1) && (cardresSessionNumber) )
           return cardresSessionNumber; 
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
void AM_OPEN_SS_REQ_CARD_RES(ULONG RessId, UCHAR Tcid)
{
    if (cardresSessionState != 0)
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
void AM_SS_OPENED_CARD_RES(USHORT SessNumber, ULONG lResourceId, UCHAR Tcid)
{
    if (cardresSessionState != 0)
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
    cardresSessionNumber = SessNumber;
    cardresSessionState  = 1;
//not sure we send this here, I believe its assumed to be successful
//AM_OPEN_SS_RSP( SESS_STATUS_OPENED, ResourceId, Tcid);
    return;
}

//after the fact, info-ing that closure has occured.
void AM_SS_CLOSED_CARD_RES(USHORT SessNumber)
{
    if((cardresSessionState)  && (cardresSessionNumber)  && (cardresSessionNumber != SessNumber ))
       {
           //don't reject but also request that the pod close (perhaps) this orphaned session
           AM_CLOSE_SS(cardresSessionNumber);
       }
    ResourceId      = 0;
    Tcid            = 0;
    tmpResourceId   = 0;
    tmpTcid         = 0;
    cardresSessionNumber = 0;
    cardresSessionState  = 0;
}
// ##################### Execution Space ##################################
// ************************* Session Stuff **************************
int vl_cardres_APDU_REQ_PIDS()
{
  unsigned char ReqPidData[4];
  unsigned long tag = 0x9FA016;
  unsigned short length_field = 2;
  unsigned char ltsid = 0;
  unsigned char pid_filtering_status = 0;

  ReqPidData[0] = ltsid;
  ReqPidData[1] = pid_filtering_status;

  cardresSndApdu(cardresSessionNumber,  tag, length_field, &ReqPidData[0]);
  return 1;
}

// Inline functions
static inline void setLoop(int cont)
{
    pthread_mutex_lock(&cardresLoopLock);
    if (!cont)
    {
        loop = 0;
    }
    else
    {
        loop = 1;
    }
    pthread_mutex_unlock(&cardresLoopLock);
}

// Sub-module entry point
int cardres_start(LCSM_DEVICE_HANDLE h)
{
    int res;

    if (h < 1)
    {
        MDEBUG (DPM_ERROR, "ERROR:ca: Invalid LCSM device handle.\n");
        // This failure should be logged by the application manager
        return -EINVAL;
    }
    // Assign handle
    cardresLCSMHandle = h;
    // Flush queue
 //   lcsm_flush_queue(cardresLCSMHandle, POD_CA_QUEUE); Hannah

    //Initialize CA's loop mutex
    if (pthread_mutex_init(&cardresLoopLock, NULL) != 0) 
    {
        MDEBUG (DPM_ERROR, "ERROR:cardres: Unable to initialize mutex.\n");
        return EXIT_FAILURE;
    }
    /*  COMMENTED BY Hannah - check if this is required
    if ((caSessionState) && (caSessionNumber)
    )
    {
    //don't reject but also request that the pod close (perhaps) this orphaned session
    AM_CLOSE_SS(caSessionNumber);
    }
    */
    /* Start CA processing thread
    For now just use default settings for the new thread. It's more important
    to be able to check the return value that would otherwise be swallowed by
    the pthread wrapper library call*/
    //    res = (int) pthread_createThread((void *)&caProcThread, NULL, DEFAULT_STACK_SIZE, 0 ); Hannah
    //res = pthread_create(&caThread, NULL, (void *)&caProcThread, NULL);
    //  if (res == 0) {
    //     MDEBUG (DPM_ERROR, "ERROR:ca: Unable to create thread.\n");
    //     return EXIT_FAILURE;
    // }

    const char* p = rmf_osal_envGet( "FEATURE.POD.ENABLE.CARDRES_PRF_WAIT" );
    if (NULL !=  p)
    {
        if (0 == strcmp(p, "TRUE"))
        {
            enableCardResWait = true;
        }
        else
        {
            enableCardResWait = false;
        }
    }

    const char* cardResWait = rmf_osal_envGet( "FEATURE.POD.CARDRES_PRF_WAIT_TIME" );
    if (NULL !=  cardResWait)
    {
        cardResWaitTimeInSec = atoi(cardResWait);
    }
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: enableCardResWait=%s cardResWaitTimeInSec=%dsec\n",
            __FUNCTION__, enableCardResWait?"TRUE":"FALSE", cardResWaitTimeInSec);

    rmf_Error ret = rmf_osal_condNew(TRUE, FALSE, &caProfileCond);
    if (RMF_SUCCESS != ret)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD",
             "%s: rmf_osal_condNew for caProfileCond failed.\n", __FUNCTION__);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// Sub-module exit point
int cardres_stop()
{
    if ( (cardresSessionState) && (cardresSessionNumber))
    {
       //don't reject but also request that the pod close (perhaps) this orphaned session
       AM_CLOSE_SS(cardresSessionNumber);
    }

    // Terminate message loop
    setLoop(0);

    // Wait for message processing thread to join
    if (pthread_join(cardresThread, NULL) != 0)
    {
        MDEBUG (DPM_ERROR, "ERROR:ca: pthread_join failed.\n");
    }
    cardresLCSMHandle = 0;

    rmf_osal_condDelete(caProfileCond);

    return 1;
}
#if 0
unsigned char vlCardIsCardResDataInitialized(UCHAR cardResParams)
{
 
    if(cardResParams == 0)
    {
      int count = 7;
      // The card res is not yet initialized .. wait for some time to get the data.
      while(count-- )
      {
        rmf_osal_threadSleep(5*1000, 0);  
        if(cardResParams != 0)
          break;
      }
    }
    return 0;
}
#endif

unsigned char vlCardResGetMaxTSSupportedByCard()
{
  
    //vlCardIsCardResDataInitialized(vlg_MaxNumTSSupportedByCard);
    return vlg_MaxNumTSSupportedByCard;
}
unsigned char vlCardResGetMaxProgramsSupportedByCard()
{
    unsigned char maxPrograms;
    
  //  vlCardIsCardResDataInitialized(vlg_MaxNumProgramsSupportedByCard);
    rmf_osal_mutexAcquire(vlg_vlCardResNumStrmsMutex);
    //return vlg_MaxNumProgramsSupportedByCard;
    maxPrograms = vlg_MaxNumProgramsSupportedByCard;
    rmf_osal_mutexRelease(vlg_vlCardResNumStrmsMutex);
    
    return maxPrograms;
}
unsigned char vlCardResGetMaxElmStreamsSupportedByCard()
{
    //vlCardIsCardResDataInitialized(vlg_MaxNumElmStreamsSupportedByCard);
    return vlg_MaxNumElmStreamsSupportedByCard;
}
void cardres_init(void)
{
    rmf_Error err;
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," ########## cardres_init:: calling REGISTER_RESOURCE ############## \n");
    REGISTER_RESOURCE(M_CARD_RES_ID, POD_CARD_RES_QUEUE, 1);  // (resourceId, queueId, maxSessions )
    err = rmf_osal_mutexNew(&vlg_vlCardResNumStrmsMutex);
    if(RMF_SUCCESS != err)
      RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD"," Error !! creating vlg_vlCardResNumStrmsMutex \n");
      
}

static int vlg_CardResResOpen = 0;
#if  0
int cardres_resopen()
{
  return vlg_CardResResOpen;
}
#endif

extern "C" int GetNumInBandTuners(int *pNumInbandTuners);

int process_STRM_PRF(uint16 sessNum, uint8 *apdu, uint16 apduLen)
{
    uint16 len;
    uint8  pApdu[MIN_APDU_HEADER_BYTES + 1];
    unsigned long Tag;
    unsigned char *pData;
    
//Get the max Transport streams supported by the host from the HAL
    //uint8 numOfStrmsUsed = CARDRES_HOST_TRAN_STRMS_SUPPORT;
    uint8 numOfStrmsUsed;
    //GetNumInBandTuners is not returning the correct value , so we are using the macro CARDRES_HOST_TRAN_STRMS_SUPPORT
    int numInbandTuners=CARDRES_HOST_TRAN_STRMS_SUPPORT;
  //  GetNumInBandTuners(&numInbandTuners);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","process_STRM_PRF:numInbandTuners:%d \n",numInbandTuners);
    numOfStrmsUsed = (uint8)numInbandTuners;

    if( (apdu == NULL) || (apduLen < 5))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","process_STRM_PRF: Error !! apdu = NULL or apduLen < 5 \n");
        return -1;
    }
    pData = (unsigned char *)apdu;
    Tag = (pData[0] << 16) | (pData[1] << 8) | pData[2];
    if( ( Tag != CARDRES_TAG_STRM_PRF ) ||  ( pData[3] != 1 ) )
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","process_STRM_PRF: Error !! Invalid Tag received from Card Tag:0x%08X or Len:%d not correct\n",Tag,pData[3]); 
        return -1;
    }
    
    vlg_MaxNumTSSupportedByCard = pData[4];

    

    if (buildApdu(pApdu, &len, CARDRES_TAG_STRM_PRF_CNF, 1, &numOfStrmsUsed) == NULL)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "pod > process_STRM_PRF: Unable to build apdu.\n");
    //cprotSndLogReq(LOG_WARNING, "pod > cp: Unable to build apdu.\n");
        return EXIT_FAILURE;
    }

    AM_APDU_FROM_HOST(sessNum, pApdu, len);

    return EXIT_SUCCESS;
}
int process_PRM_PRF(uint16 sessNum, uint8 *apdu, uint16 apduLen)
{
    uint16 len;
    uint8  pApdu[MIN_APDU_HEADER_BYTES ];    
      unsigned char *pData;
    unsigned long Tag;
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","process_PRM_PRF : Entered @@@@@@@@@@@\n");
     if( (apdu == NULL) || (apduLen < 5))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","process_PRM_PRF: Error !! apdu = NULL or apduLen < 5 \n");
        return -1;
    }
    pData = (unsigned char *)apdu;
    Tag = (pData[0] << 16) | (pData[1] << 8) | pData[2];
    if( ( Tag != CARDRES_TAG_PRG_PRF ) ||  ( pData[3] != 1 ) )
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","process_PRM_PRF:Error !! Invalid Tag received from Card Tag:0x%08X or Len:%d not correct\n",Tag,pData[3]); 
        return -1;
    }
    
    vlg_MaxNumProgramsSupportedByCard = pData[4];

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: MAX Programs that can be descrambled by Cablecard (obtained from Program Profile APDU) : %d \n", __FUNCTION__, vlg_MaxNumProgramsSupportedByCard); 
    if(vlg_MaxNumProgramsSupportedByCard > vlCaGetMaxProgramsSupportedByHost())
        vlg_MaxNumProgramsSupportedByCard = vlCaGetMaxProgramsSupportedByHost();

    if (buildApdu(pApdu, &len, CARDRES_TAG_PRG_PRF_CNF, 0, NULL) == NULL) 
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "pod > process_PRM_PRF: Unable to build apdu.\n");
        //cprotSndLogReq(LOG_WARNING, "pod > cp: Unable to build apdu.\n");
        return EXIT_FAILURE;
    }

    AM_APDU_FROM_HOST(sessNum, pApdu, len);

    
    return EXIT_SUCCESS;
}

int process_ES_PRF(uint16 sessNum, uint8 *apdu, uint16 apduLen)
{
    uint16 len;
    uint8  pApdu[MIN_APDU_HEADER_BYTES];    
    unsigned char *pData;
    unsigned long Tag;
     RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","process_ES_PRF: Entered @@@@@@@@@@@@@@@\n");
     if( (apdu == NULL) || (apduLen < 5))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","process_ES_PRF: Error !! apdu = NULL or apduLen < 5 \n");
        return -1;
    }
     pData = (unsigned char *)apdu;
    Tag = (pData[0] << 16) | (pData[1] << 8) | pData[2];
    if( ( Tag != CARDRES_TAG_ES_PRF ) ||  ( pData[3] != 1 ) )
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","process_ES_PRF: Error !!  Invalid Tag received from Card Tag:0x%08X or Len:%d not correct\n",Tag,pData[3]); 
        return -1;
    }
    
    vlg_MaxNumElmStreamsSupportedByCard = pData[4];
    if (buildApdu(pApdu, &len, CARDRES_TAG_ES_PRF_CNF, 0, NULL) == NULL)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","pod > process_ES_PRF: Unable to build apdu.\n");
        
        return EXIT_FAILURE;
    }

    AM_APDU_FROM_HOST(sessNum, pApdu, len);
    //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","  Requesting for  PIDS not to be filtered \n");
   // vl_cardres_APDU_REQ_PIDS();
    return EXIT_SUCCESS;
}
int process_REQ_PIDS_CNF(uint16 sessNum, uint8 *apdu, uint16 apduLen)
{
   int ii;
   

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","process_REQ_PIDS_CNF : ENTERED @@@@@@@@@@@ \n");
     RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","\n Printing REQ_PIDS_CNF APDU \n");
    for(ii = 0; ii < apduLen; ii++)
    {
       RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","0x%02X ",apdu[ii]);
    if(((ii + 1) % 16) == 0)
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","\n");
    }
    return EXIT_SUCCESS;
}

static int vl_gPOD_CARD_RES_PRM_PRF_received = 0;
static int vl_gPOD_CARD_RES_ES_PRF_received = 0;
static int vl_gPOD_CARD_RES_STRM_PRF_received = 0;

void cardresProc(void* par)
{
    char *ptr;
    LCSM_EVENT_INFO       *eventInfo = (LCSM_EVENT_INFO       *)par;
    rmf_Error err;
    //uint8 nvmAuthStatus;process_CP_OPEN_REQ
    // RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "Thread started, (PID=%d)\n", (int) getpid() );

    setLoop(1);

    //cprotSndLogReq(LOG_INFO, "pod > cprot: Message processing thread started.\n");
//    handle = cardresLCSMHandle;        // because REGISTER_RESOURCE macro expects 'handle'

#ifdef DEFER_REG_UNTIL_TIME_SET
//    lcsm_register_event( cpLcsmHandle, SYSTEM_TIME_UPDATE, POD_CPROT_QUEUE ); 
#else
//    REGISTER_RESOURCE(CP_ID, POD_CPROT_QUEUE, MAX_CP_OPEN_SESSIONS)
#endif

//temprarily commented by Hannah
/*
    // Read authentication status from NVM, and send to MR control
    nvmm_rawRead(NVRAM_CP_AUTH_STATUS, 1, &nvmAuthStatus);
    CPRDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","CP> Read NVRAM_CP_AUTH_STATUS = 0x%x\n", nvmAuthStatus); 
*/    
    //sendAuthenticatedStatus(nvmAuthStatus);
    //while(loop) {
    //    lcsm_get_event(cpLcsmHandle, POD_CPROT_QUEUE, &eventInfo, LCSM_WAIT_FOREVER );  //Hannah
    switch (eventInfo->event)
    {

        // POD requests open session (x=resId, y=tcId)
    case POD_OPEN_SESSION_REQ:
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","cardresProc: << POD_OPEN_SESSION_REQ >>\n");
        AM_OPEN_SS_REQ_CARD_RES((uint32)eventInfo->x, (uint8)eventInfo->y);
            break;

        // POD confirms open session (x=sessNbr, y=resId, z=tcId)
    case POD_SESSION_OPENED:
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","cardresProc: << POD_OPEN_SESSION_REQ >>\n");
        AM_SS_OPENED_CARD_RES((uint16)eventInfo->x, (uint32)eventInfo->y, (uint8)eventInfo->z);
/*        vlg_CardResResOpen = 1;
        //Post the CASystemEvent that indicates that the CARDRES resource is open.
        vlMpeosPostCASystemEvent(0);*/

        if (enableCardResWait && caProfileMonTaskID == 0)
        {
            err = rmf_osal_threadCreate(caProfileMonitor, NULL,
                                        RMF_OSAL_THREAD_PRIOR_DFLT, 4096,
                                        &caProfileMonTaskID,
                                        "CA_PROFILE_Mon_Thread");
            if (RMF_SUCCESS != err)
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD",
                    "%s: Failed to create caSystemMonitor thread\n", __FUNCTION__);
            }
        }
        break;

        // POD closes session (x=sessNbr)
    case POD_CLOSE_SESSION:
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","cardresProc: << POD_CLOSE_SESSION_REQ >>\n");
        AM_SS_CLOSED_CARD_RES((uint16)eventInfo->x);
        vlg_CardResResOpen = 0;
        vl_gPOD_CARD_RES_PRM_PRF_received = 0;
        vl_gPOD_CARD_RES_ES_PRF_received = 0;
                break;

        // POD sends APDU (x=sessNbr, y=APDU buffer ptr, z=APDU byte length)
    case POD_APDU:
        ptr = (char *)eventInfo->y;    // y points to the complete APDU
        switch (*(ptr+2))
        {
        case POD_CARD_RES_STRM_PRF:
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","Received POD_CARD_RES_STRM_PRF \n");
            process_STRM_PRF((uint16)eventInfo->x, (uint8 *)eventInfo->y, (uint16)eventInfo->z);
            vl_gPOD_CARD_RES_STRM_PRF_received = 1;
            break;

        case POD_CARD_RES_PRM_PRF:
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","Received POD_CARD_RES_PRM_PRF \n");
            process_PRM_PRF((uint16)eventInfo->x, (uint8 *)eventInfo->y, (uint16)eventInfo->z);
            vl_gPOD_CARD_RES_PRM_PRF_received = 1;
            break;

        case POD_CARD_RES_ES_PRF:
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","Received POD_CARD_RES_ES_PRF \n");
            process_ES_PRF((uint16)eventInfo->x, (uint8 *)eventInfo->y, (uint16)eventInfo->z);
            vl_gPOD_CARD_RES_ES_PRF_received = 1;
            break;

        case POD_CARD_RES_REQ_PIDS_CNF:
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","Received POD_CARD_RES_REQ_PIDS_CNF \n");
            process_REQ_PIDS_CNF((uint16)eventInfo->x, (uint8 *)eventInfo->y, (uint16)eventInfo->z);
            break;

        #if 0
            // POD sends APDU (x=session id, y = apdu, z = length)
            case POD_CPROT_OPENREQ:
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","cprotProc: << POD_CPROT_OPENREQ >>\n");
                process_CP_OPEN_REQ((uint16)eventInfo->x, (uint8 *)eventInfo->y, (uint16)eventInfo->z);
                break;

            case POD_CPROT_DATAREQ:
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","cprotProc: << POD_CPROT_DATAREQ >>\n");
                process_CP_DATA_REQ((uint16)eventInfo->x, (uint8 *)eventInfo->y, (uint16)eventInfo->z);
                break;

            case POD_CPROT_SYNCREQ:
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","cprotProc: << POD_CPROT_SYNCREQ >>\n");
                process_CP_SYNC_REQ((uint16)eventInfo->x, (uint8 *)eventInfo->y, (uint16)eventInfo->z);
                break;
         #endif
            default:
                break;
        }
        //Post the CASystemEvent that indicates that the CARDRES resource is open.
        if (enableCardResWait)
        {
            if (vl_gPOD_CARD_RES_PRM_PRF_received &&
                vl_gPOD_CARD_RES_ES_PRF_received &&
                vl_gPOD_CARD_RES_STRM_PRF_received)
            {
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: All profiles received\n",
                        __FUNCTION__);
                rmf_osal_condSet(caProfileCond);
            }
        }
        else if (vl_gPOD_CARD_RES_PRM_PRF_received && vl_gPOD_CARD_RES_ES_PRF_received)
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD",
                    "cardresProc: << Card is ready!! POSTING vlMpeosPostCASystemEvent >>\n");
          vlMpeosPostCASystemEvent(0);
        }

        break;

#ifdef DEFER_REG_UNTIL_TIME_SET
        case SYSTEM_TIME_UPDATE:
        {
            time_t tp;

            time(&tp);
            CPRDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","CP> SYSTEM_TIME_UPDATE -> register CP, Time = \n", asctime(localtime(&tp))); 
        }
        REGISTER_RESOURCE(CP_ID, POD_CPROT_QUEUE, MAX_CP_OPEN_SESSIONS)
    //    lcsm_unregister_event(cpLcsmHandle, SYSTEM_TIME_UPDATE, POD_CPROT_QUEUE);
        break;
#endif

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

void caProfileMonitor(void *p)
{
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: Waiting for CA Profile notification=%dsec\n",
            __FUNCTION__, cardResWaitTimeInSec);

    rmf_osal_condWaitFor(caProfileCond, cardResWaitTimeInSec*1000);

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: Profiles PRM=%d ES=%d STRM=%d\n",
            __FUNCTION__, vl_gPOD_CARD_RES_PRM_PRF_received,
            vl_gPOD_CARD_RES_ES_PRF_received, vl_gPOD_CARD_RES_STRM_PRF_received);

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD",
            "caProfileMonitor: << Card is ready!! POSTING vlMpeosPostCASystemEvent >>\n");
    vlMpeosPostCASystemEvent(0);
    caProfileMonTaskID = 0;
}

#ifdef __cplusplus
}
#endif


