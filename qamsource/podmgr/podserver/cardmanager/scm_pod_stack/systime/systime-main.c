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



#include <errno.h>
#include <time.h>

//#include <pthreadLibrary.h>
#include <pthread.h>
#include <global_event_map.h>
#include <global_queues.h>
#include "poddef.h"
#include <lcsm_log.h>
#include "cardUtils.h"
//#include "pfctime.h"
#include "utils.h"
//#include "cis.h"
//#include "module.h"
//#include "link.h"
#include "transport.h"
#include "session.h"
#include "appmgr.h"
#include "hal_pod_api.h"
#include "systime-main.h"
#include "podhighapi.h"
#include <memory.h>
#undef FUNAI_JEDI_LINUX_740X
#define FUNAI_JEDI_LINUX_740X

#include "rmf_osal_sync.h"
#include "rmf_osal_thread.h"

#include <rdk_debug.h>
#ifndef   MIN
#define MIN( x, y )     ( ( x < y ) ? x : y )
#endif // MIN
#ifdef __cplusplus
extern "C" {
#endif

#define  LCSM_WAIT_FOREVER  0 //Hannah


// Externs
extern unsigned long RM_Get_Ver( unsigned long id );

// Globals
static LCSM_DEVICE_HANDLE   handle = 0;
static pthread_t            systimeThread;
static pthread_mutex_t      systimeLoopLock;
static int                  loop;
static int vlg_SystemTimeStop = 1;
static int vlg_ReceivedStt = 1;
static int vlg_SysTimeInqRcvd = 0;    
    
static inline void setLoop(int cont)
{
    // 'loop' controls whether the thread continues to
    // loop through its message loop (1) or not (0)
    pthread_mutex_lock( &systimeLoopLock );     // enter critical section
    if( ! cont )
    {
        loop = 0;   // thread will exit!
    } else
    {
        loop = 1;   // let thread run
    }
    pthread_mutex_unlock( &systimeLoopLock );   // leave critical section
}

// debugging aid - dump APDU
static void dumpApdu( UCHAR * pApdu, USHORT len )
{
    int ii, jj;

    MDEBUG ( DPM_SYSTIME, "SystemTime APDU...len=%d\n", len );
    MDEBUG ( DPM_SYSTIME, "     0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n" );
    for( ii = 0; ii < len; ii += 0x10 )
    {
        MDEBUG2 ( DPM_SYSTIME, "%03x", ii );
        for( jj = 0; jj < 0x10  &&  ii+jj < len; jj++ )
            MDEBUG2 ( DPM_SYSTIME,  " %02x", pApdu[ ii+jj ] );
        MDEBUG2 ( DPM_SYSTIME, "\n");
    }
}


extern UCHAR  transId;          // set in App Mgr (applitest.c)
static USHORT sessionNbr = 0;   // non-zero if open session already

static Bool openSessionReq( ULONG resId, UCHAR tcId )
{
    Bool  status = FALSE;   // returned to caller
    UCHAR openStatus = 0;   // returned to POD
    MDEBUG (DPM_SYSTIME, "resId=0x%x tcId=%d sessionNbr=%d\n",
            resId, tcId, sessionNbr );

    if( sessionNbr == 0 )    // systime session not opened yet...
    {
        // make sure we are using the transId already opened earlier
        /*if( transId != tcId )
        {
            openStatus = 0xF0;  // non-existent resource (on this transId)
            MDEBUG (DPM_ERROR, "ERROR: tcId=%d transId=%d\n",
                    tcId, transId );
//            return FALSE;

        // make sure that requested resource version matches ours
        } else if( RM_Get_Ver( SYS_TIME_ID ) < RM_Get_Ver( resId ) )
        {
            openStatus = 0xF2;  // this version < requested
            MDEBUG (DPM_ERROR, "ERROR: version=0x%x < req=0x%x\n",
                    RM_Get_Ver( SYS_TIME_ID ), RM_Get_Ver( resId ) );

        // it's all good!
        } else*/
        {
            MDEBUG (DPM_SYSTIME, "opening session\n" );
            status = TRUE;
        }
    } else              // ...single allowable systime session already opened
    {
        openStatus = 0xF3;      // exists but busy
        MDEBUG (DPM_WARN, "max sessions already opened\n" );
    }
    AM_OPEN_SS_RSP( openStatus, resId, transId );
    return status;
}

static Bool sessionOpened( USHORT sessNbr, ULONG resId, UCHAR tcId )
{
    Bool status = FALSE;

    if( sessionNbr == 0 )
    {
        sessionNbr = sessNbr;   // it is now officially opened
        status = TRUE;
        MDEBUG (DPM_SYSTIME, "tcId=%d resId=0x%x sessionNbr=%d\n",
                tcId, resId, sessionNbr );
    } else
        MDEBUG (DPM_ERROR, "ERROR: POD opened bad session; sessNbr=%d, sessionNbr=%d\n",
                sessNbr, sessionNbr );

    return status;
}

static Bool sessionClosed( USHORT sessNbr )
{
    Bool status = FALSE;

    if( sessNbr  &&  sessionNbr == sessNbr )
    {
        sessionNbr = 0;         // it is now officially closed
        status = TRUE;
        MDEBUG( DPM_SYSTIME, "closing session %d\n",
                sessNbr );

    } else
        MDEBUG( DPM_ERROR, "ERROR: POD closed bad session; sessNbr=%d, sessionNbr=%d\n",
                sessNbr, sessionNbr );

    return status;
}


#define REGULAR_YEAR_SECONDS  ( 365 * 24 * 60 * 60 )
#define LEAP_YEAR_SECONDS     ( 366 * 24 * 60 * 60 )
// adjustment in seconds for difference between
//      Unix epoch (Jan 1, 1970 midnight UTC) and
//      DTV  epoch (Jan 6, 1980 midnight UTC)
#define EPOCH_ADJUSTMENT      ( ( REGULAR_YEAR_SECONDS * 8 ) + \
                                ( LEAP_YEAR_SECONDS * 2 )    + \
                                ( 5 * 24 * 60 * 60 ) )

static void getTime( UCHAR * ptr )
{
    time_t  tvec;
    UCHAR * tptr;
    int SystemTime;//Time in Sec Since Jan6,1980 12:00AM UTC
    UCHAR  GpsUtcOffset;
//     pfcTime *pfcgettime;

    time( &tvec );

    // adjust the count of seconds in Unix epoch (Jan 1, 1970 midnight UTC)
    //  to be the count of seconds in DTV  epoch (Jan 6, 1980 midnight UTC)
    tvec -= EPOCH_ADJUSTMENT;

    tptr = (UCHAR *) &tvec;
 // reverse the order of the 4 byte timedate while copying it to the passed arg
    ptr[3] = *tptr++;
    ptr[2] = *tptr++;
    ptr[1] = *tptr++;
    ptr[0] = *tptr++;

    ptr[4] = 0;
/*
    pfcgettime = pfc_kernel_global->get_pfctime();
    if(pfcgettime)
    {
        int utcoffset = pfcgettime->get_utcoffset();
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," >>>>> utcoffset:%d \n",utcoffset);
        ptr[4] = (UCHAR )utcoffset;
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD"," >>>>> pfcgettime:%X is NULL\n",pfcgettime);
    }
*/


}

static void sendTimeSignal( USHORT sesnum )
{
        // apdu field:   tag............    len.   sys time..  offset
    UCHAR apdu_out[] = { 0x9F, 0x84, 0x43,  0x05,  0, 0, 0, 0,   0 };
        // byte offset:     0     1     2      3   4  5  6  7    8

    // load the current time into the outgoing apdu template
    getTime( &apdu_out[ 4 ] );
//  AM_APDU_FROM_HOST (sesnum, apdu_out, 10);
    MDEBUG (DPM_SYSTIME, "to POD sesnum=%d\n",
            sesnum );
    AM_APDU_TO_POD( sesnum, apdu_out, sizeof(apdu_out) );
}

// we don't want to register as a resource with the resource manager
// unless/until we have been told that there is a good system time to
// report to the POD. Initially, when platform comes up, times is set
// to some default time (typically, Jan 1, 2000 midnight UTC). Sometime
// soon after, either the main board updates our time to something that
// might be considered to be current or the time is updated based on the
// current time in the PSIP System Time Table (STT). Either way, we really
// can't do much until we know that we have a good system time.
#define WAIT_FOR_GOOD_TIME  ( 30 * 1000/*ms*/ )   // 1/2min, but just a guess
#define MIN_UPDATES         ( 1 )  // min number of updates req'd
#define MAX_TIMEOUTS        ( 2 )  // after a minute, pack it in...

#if 0
// wait to begin normal processing until
//   1. a good system time is set or
//   2. we give up waiting
static Bool waitingForGoodTime( LCSM_EVENT_INFO       *pMsg )
{
//    LCSM_EVENT_INFO msg;
    Bool            status = FALSE;     // TRUE means good
    int             val=0;
    int             updates = 0;
    int             timeOuts = 0;
    Bool            keepWaiting = TRUE;

    MDEBUG (DPM_SYSTIME, "registering for 0x%x, loop=%d\n",
            SYSTEM_TIME_UPDATE, loop );
    // register for notifications whenever system time is updated
//    lcsm_register_event( handle, SYSTEM_TIME_UPDATE, POD_SYSTIME_QUEUE ); Hannah

    // continue waiting until we're shut down or we've received min number of
    // time update notifications
    while( loop &&  keepWaiting )
    {
        //memset( &msg, 0, sizeof( msg ) );
  //      val = lcsm_get_event( handle, POD_SYSTIME_QUEUE, &msg, WAIT_FOR_GOOD_TIME ); Hannah

        MDEBUG (DPM_SYSTIME, "val=%d pMsg->event=0x%x\n",
                val, pMsg->event );

        if( val == 0 )
        {
            // got an event to handle, so do it
            switch (pMsg->event)
            {
                // system time has been updated
                case SYSTEM_TIME_UPDATE:
                    updates++;
                    if( updates == 1 )  // first time
                    {
                        // tell resource manager that we're available now
                        REGISTER_RESOURCE( M_SYS_TIME_ID, POD_SYSTIME_QUEUE, 1 );
                        MDEBUG (DPM_SYSTIME, "SYSTEM_TIME_UPDATE seen...registered with RM\n" );
                    }
                    if( updates >= MIN_UPDATES )
                        keepWaiting = FALSE;
                    break;

                case EXIT_REQUEST:
                    setLoop( 0 );
                    break;

                default:
                    MDEBUG (DPM_WARN, "unexpected event 0x%x ignored\n",
                            pMsg->event );
                    break;
            }
        } else
        {
            if( ++timeOuts >= MAX_TIMEOUTS )
                keepWaiting = FALSE;
            MDEBUG (DPM_SYSTIME, "timed-out %d time(s)...\n",
                    timeOuts );
        }
    }
    // might want to unregister for SYSTEM_TIME_UPDATE ??
    MDEBUG (DPM_SYSTIME, "done waiting for good time...\n" );
    return status;
}

#endif 

void systimeInit(void)
{
    REGISTER_RESOURCE( M_SYS_TIME_ID, POD_SYSTIME_QUEUE, 1 );
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","\n########### systimeInit:: Create the object for sst time #########\n");

}

void vlSytemTimeTask(void *arg)
{
    unsigned short sesNum;
    unsigned long time;
    long param = (long)arg;
    sesNum = (param & 0xFFFF00 ) >> 8;
    time = param & 0xFF;
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","vlSytemTimeTask: Created with time:%d sesNum:%d\n",time,sesNum);
    while(1)
    {
        if(vlg_SystemTimeStop)
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","vlSytemTimeTask: Exiting the thread \n");
            break;
        }
        if(HomingCardFirmwareUpgradeGetState() == 0)
            sendTimeSignal(sesNum);
        rmf_osal_threadSleep(time*1000, 0);
        if(time == 0)
            break;

    }
}
void systimeSTTUpdateEvent(unsigned long param)
{
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","systimeSTTUpdateEvent: Received Event \n");
    //REGISTER_RESOURCE( M_SYS_TIME_ID, POD_SYSTIME_QUEUE, 1 );
    vlg_ReceivedStt = 1;
    
    if(sessionNbr && vlg_SysTimeInqRcvd)
    {
        sendTimeSignal((USHORT)sessionNbr);
    }
}
void systimeProc(void* par)
{
    char          * ptr;
    LCSM_EVENT_INFO       *msg = (LCSM_EVENT_INFO       *)par;
    //int             status;
    USHORT          sesnum = 0;
    UCHAR           apdu[10];
    ULONG           apduTag;
    UCHAR    interval;
    // we got an event to handle, so do it
    switch (msg->event)
    {
    // system time has been updated
          case SYSTEM_TIME_UPDATE:
          // we may want to do something more here ??
             break;

         // POD requests open session (x=resId, y=tcId)
          case POD_OPEN_SESSION_REQ:
              openSessionReq( (ULONG) msg->x, (UCHAR) msg->y );
              break;

         // POD confirms open session (x=sessNbr, y=resId, z=tcId)
          case POD_SESSION_OPENED:    // this usually does not happen
               sessionOpened( (USHORT) msg->x, (ULONG) msg->y, (UCHAR) msg->z );
               break;

          // POD closes session (x=sessNbr)
          case POD_CLOSE_SESSION:
               if( sessionClosed( (USHORT) msg->x ) )
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","systimeProc: POD_CLOSE_SESSION Received \n");
                  vlg_SystemTimeStop = 1;
        }
                  break;

    // POD sends APDU (x=sessNbr, y=APDU buffer ptr, z=APDU byte length)
          case POD_APDU:
                MDEBUG (DPM_SYSTIME, "ST_APDU_FROM_POD: sessNbr=%d len=%d\n",msg->x, msg->z );
                sesnum = (USHORT) msg->x;
                if( sesnum == sessionNbr )
                {
                        // 1st 4 bytes of APDU are 24-bit tag + 8-bit length
                       // memcpy( apdu, (UCHAR *)(msg->y), MIN( sizeof(apdu), msg->z ) );
             rmf_osal_memcpy( apdu, (UCHAR *)(msg->y), msg->z,
             					sizeof(apdu), msg->z );
                        apduTag = ( ( (uint32) apdu[ 0 ] ) << 16 )  |
                                  ( ( (uint32) apdu[ 1 ] ) <<  8 )  |
                                    ( (uint32) apdu[ 2 ] );

                        switch( apduTag )
                        {
                            case POD_SYSTIME_INQ:

                            vlg_SysTimeInqRcvd = 1;
                            if(vlg_ReceivedStt)
                            {
                                interval = apdu[ 4 ];   // byte 5 of APDU
                                sendTimeSignal( sesnum );
                                if(interval)
                                {
                                    long param;
                                    rmf_osal_ThreadId threadId = 0;
                                    param = (interval & 0xFF) | (sesnum  << 8 );
                                    vlg_SystemTimeStop = 0;
                                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","systimeProc: POD_SYSTIME_INQ Received..interval:%d \n",interval);
                                    rmf_osal_threadCreate(vlSytemTimeTask, (void *)param,
                                        250, 0x1000, &threadId, "vlSytemTimeTask");

                                }
                            }
                                break;

                            default:
                                MDEBUG (DPM_WARN, "unexpected APDU tag 0x%x ignored\n",
                                        apduTag );
                                break;
                    }
                 }
         else
         {
                        MDEBUG (DPM_WARN, "invalid APDU session number %d ignored, exp=%d\n",
                                sesnum, sessionNbr);
         }
                 break;

           case EXIT_REQUEST:
                 setLoop( 0 );
                 break;

           default:
                 MDEBUG (DPM_WARN, "unexpected event 0x%x ignored\n",  msg->event );
    }
}


    /* system time resource control - via main.c in pod/user */

// Sub-module entry point
int systime_start( LCSM_DEVICE_HANDLE h )
{
    int res;

    if( h < 1 )
    {
        MDEBUG ( DPM_SYSTIME, "ERROR: pod > systime: Invalid LCSM device handle.\n" );
        // This failure should be logged by the application manager
        return -EINVAL;
    }
    // Assign handle
    handle = h;
    // Flush queue
  //  lcsm_flush_queue(handle, POD_SYSTIME_QUEUE); Hannah

    //Initialize SYSTIME's loop mutex
    if( pthread_mutex_init( &systimeLoopLock, NULL ) != 0 )
    {
        MDEBUG ( DPM_SYSTIME, "ERROR: pod > systime: Unable to initialize mutex.\n" );
        return EXIT_FAILURE;
    }

    /* Start SYSTIME processing thread.
       For now just use default settings for the new thread. It's more important
       to be able to check the return value that would otherwise be swallowed by
       the pthread wrapper library call
     */
//    res = (int) pthread_createThread((void *)&systimeProcThread, NULL, DEFAULT_STACK_SIZE, 0 ); Hannah
    //status = pthread_create( &systimeThread, NULL, (void *) &systimeProcThread, NULL );
   // if( res == 0 )
 //   {
  //      MDEBUG ( DPM_SYSTIME, "ERROR: pod > systime: Unable to create //thread.\n" );
  //      return EXIT_FAILURE;
  //  }
    return EXIT_SUCCESS;
}

// Sub-module exit point
int systime_stop( void )
{
    // Terminate message loop
    setLoop( 0 );

    // Wait for message processing thread to join
    if( pthread_join( systimeThread, NULL ) != 0 )
    {
        MDEBUG ( DPM_SYSTIME, "ERROR: pod > systime: pthread_join failed.\n" );
    }
    return EXIT_SUCCESS;
}

#ifdef __cplusplus
}
#endif


