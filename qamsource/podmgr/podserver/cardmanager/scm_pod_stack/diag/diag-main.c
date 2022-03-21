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
#include <pthread.h>
#include <global_event_map.h>
#include <global_queues.h>
#include "poddef.h"
#include <lcsm_log.h>
//#include "podlowapi.h"
#include "utils.h"
#include "appmgr.h"
#include <rdk_debug.h>
//#include "diag-main.h"
#ifdef __cplusplus
extern "C" {
#endif
#define DIAG_MAX_OPEN_SESSIONS 1

// Prototypes
void diagProc(void*);

// Globals
static LCSM_DEVICE_HANDLE     handle = 0;
static pthread_t             diagThread;
static pthread_mutex_t         diagLoopLock;
static int                     loop;
static int                  openSessions = 0;
static int                  sessionNums[ DIAG_MAX_OPEN_SESSIONS ] = { 0, };
static unsigned long openedDiagRsId =  0;
extern int  APDU_DiagReq (USHORT sesnum, UCHAR * pApdu, USHORT len);

extern UCHAR   transId;
extern void AM_OPEN_SS_RSP( UCHAR Status, ULONG lResourceId, UCHAR Tcid);
// Inline functions
static inline void setLoop(int cont)
{
    pthread_mutex_lock(&diagLoopLock);
    if (!cont) {
        loop = 0;
    }
    else {
        loop = 1;
    }
    pthread_mutex_unlock(&diagLoopLock);
}

unsigned long vldiagGetOpenedRsId()
{
    return openedDiagRsId;
}
// Sub-module entry point
int diag_start(LCSM_DEVICE_HANDLE h)
{
    int res;

    if (h < 1) {
        MDEBUG (DPM_ERROR, "ERROR:diag: Invalid LCSM device handle.\n");
        // This failure should be logged by the application manager
        return -EINVAL;
    }
    // Assign handle
    handle = h;
    // Flush queue
//    lcsm_flush_queue(handle, POD_DIAG_QUEUE);

    //Initialize DIAG's loop mutex
 //    if (pthread_mutex_init(&diagLoopLock, NULL) != 0) {
 //        MDEBUG (DPM_ERROR, "ERROR:diag: Unable to initialize mutex.\n");
 //        return EXIT_FAILURE;
 //    }

    /* Start DIAG processing thread
       For now just use default settings for the new thread. It's more important
       to be able to check the return value that would otherwise be swallowed by
       the pthread wrapper library call*/
//    res = (int) pthread_createThread((void *)&diagProcThread, NULL, DEFAULT_STACK_SIZE, 0 ); Hannah
    //res = pthread_create(&diagThread, NULL, (void *)&diagProcThread, NULL);
    //if (res == 0) {
    //    MDEBUG (DPM_ERROR, "ERROR:diag: Unable to create thread.\n");
    //    return EXIT_FAILURE;
    //}

    return EXIT_SUCCESS;
}

// Sub-module exit point
int diag_stop()
{
    // Terminate message loop
    //setLoop(0);

    // Wait for message processing thread to join
    //if (pthread_join(diagThread, NULL) != 0) {
    //    MDEBUG (DPM_ERROR, "ERROR:diag: pthread_join failed.\n");
    //}
return EXIT_SUCCESS;//Mamidi:042209
}


void diag_init(void){
    REGISTER_RESOURCE(M_GEN_DIAG_ID, POD_DIAG_QUEUE, 1);

}

static void evOpenSessionReq( ULONG resId, UCHAR tcId )
{
    UCHAR status = 0;
    MDEBUG (DPM_RSMGR, "resId=0x%x tcId=%d openSessions=%d\n",
            resId, tcId, openSessions );

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","resId=0x%x tcId=%d openSessions=%d\n",
            resId, tcId, openSessions );

    if( openSessions < DIAG_MAX_OPEN_SESSIONS )
    {
    /*if( transId != tcId )
        {
            status = 0xF0;  // non-existent resource (on this transId)
            MDEBUG (DPM_ERROR, "ERROR: non-existent resource, transId in OPEN_SS=%d, exp=%d\n", tcId, transId );
        } else */
    /*if( RM_Get_Ver( RESRC_MGR_ID ) < RM_Get_Ver( resId ) )
        {
            status = 0xF2;  // this version < requested
            MDEBUG (DPM_ERROR, "ERROR: bad version=0x%x < req=0x%x\n",
                    RM_Get_Ver( RESRC_MGR_ID ), RM_Get_Ver( resId ) );
        } else*/
        {
            MDEBUG (DPM_RSMGR, "RM session %d opened\n", openSessions+1 );
        }
    } else
    {
        status = 0xF3;      // exists but busy
        MDEBUG (DPM_ERROR, "ERROR: max sessions opened=%d\n", openSessions );
    }

    AM_OPEN_SS_RSP( status, resId, transId );
}
static void evSessionOpened( USHORT sessNbr, ULONG resId, UCHAR tcId )
{
    int ii;
    MDEBUG (DPM_RSMGR, "sessNbr=%d resId=0x%x tcId=%d\n",
            sessNbr, resId, tcId );
openedDiagRsId = resId;
RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","sessNbr=%d resId=0x%x tcId=%d openSessions:%d\n",
            sessNbr, resId, tcId,openSessions );
    for( ii = 0; ii < DIAG_MAX_OPEN_SESSIONS; ii++ )
    {
        if( sessionNums[ ii ] == 0 )
        {
            // might want to save resId and/or tcId, too
            sessionNums[ ii ] = sessNbr;
            openSessions++;
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," ###### Ses Opened num:%d \n",openSessions);
       //     PODSleep (4);        // we just sent an AM_OPEN_SS_RSP apdu to the POD, so wait a while
//            sendProfInqApdu( sessNbr );
            break;
        }
    }
}


static int process_DIAG_CLOSE_SESS(uint16 sessNbr)
{
    openSessions = ( --openSessions < 0 )  ?  0 : openSessions;

    sessionNums[ openSessions ] = 0;
    openedDiagRsId = 0;
return EXIT_SUCCESS;//Mamidi:042209

}

void diagProc(void* par)
{
    char *ptr;
    LCSM_EVENT_INFO       *msg = (LCSM_EVENT_INFO       *)par;
        //setLoop(1);

   // MDEBUG (DPM_ALWAYS, "Thread started, (PID=%d)\n", (int) getpid() );

    //while(loop) {
      //  lcsm_get_event( handle, POD_DIAG_QUEUE, &eventInfo, LCSM_WAIT_FOREVER );
    switch ( msg->event)
    {
                // POD requests open session (x=resId, y=tcId)
        case POD_OPEN_SESSION_REQ:
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","diagProc: POD_OPEN_SESSION_REQ #### \n");
        evOpenSessionReq( (ULONG) msg->x, (UCHAR) msg->y );

                break;

    // POD confirms open session (x=sessNbr, y=resId, z=tcId)
    case POD_SESSION_OPENED:
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","diagProc: POD_SESSION_OPENED #### \n");
        evSessionOpened( (USHORT) msg->x, (ULONG) msg->y, (UCHAR) msg->z );
                break;

            // POD closes session (x=sessNbr)
    case POD_CLOSE_SESSION:
        process_DIAG_CLOSE_SESS(msg->x);
                break;

            // POD sends APDU (x=sessNbr, y=APDU buffer ptr, z=APDU byte length)
    case POD_APDU:
                ptr = (char *)msg->y;    // y points to the complete APDU
        switch (*(ptr+2))
        {
        case POD_DIAG_REQ:
            APDU_DiagReq ((USHORT)msg->x, (UCHAR *)msg->y, (USHORT)msg->z);
            break;

        case POD_DIAG_CNF:
            APDU_DiagCnf ((USHORT)msg->x, 0, (UCHAR *)msg->y, (USHORT)msg->z);
            break;
        }
                break;

        case EXIT_REQUEST:
            break;
    }
//}
    //pthread_exit(NULL);
}

#ifdef __cplusplus
}
#endif

