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

#include "mrerrno.h"
#include "sas-main.h"
#include "utils.h"
#include "link.h"
#include "transport.h"
//#include "podlowapi.h"
#include "podhighapi.h"
#include "applitest.h"


#include "appmgr.h"

#include <string.h>
#include <rdk_debug.h>
#include "rmf_osal_mem.h"

#if USE_SYSRES_MLT
#include "mlt_malloc.h"
#endif

#define __MTAG__ VL_CARD_MANAGER

static LCSM_DEVICE_HANDLE   handle = 0;
static pthread_t            sasThread;
static sasControl_t         sasCtl = { SAS_UNINITIALIZED, 0, };
static int                  openSessions = 0;

#ifdef __cplusplus
extern "C" {
#endif

// Externs
extern unsigned long RM_Get_Ver( unsigned long id );

static USHORT    evGetAnyOpenSessionNumber();
static USHORT evGetUnusedOpenSessionNumber();
        /***  thread control functionality  ***/

static inline void setLoop( int cont )
{
    // NB: the thread soon dies after loop is set to 0!
    pthread_mutex_lock( &( sasCtl.lock ) );
    if( ! cont )
        sasCtl.loop = 0;
    else
        sasCtl.loop = 1;
    pthread_mutex_unlock( &( sasCtl.lock ) );
}

static int setState( sasState_t newState )
{
    int status = EXIT_FAILURE;

    pthread_mutex_lock( &( sasCtl.lock ) );

    switch( sasCtl.state )
    {
        case SAS_UNINITIALIZED:
            break;

        case SAS_INITIALIZED:
            if( newState == SAS_STARTED )
            {
                sasCtl.state = newState;
                status = EXIT_SUCCESS;
            }
            break;

        case SAS_STARTED:
            if( newState == SAS_SHUTDOWN )
            {
                sasCtl.state = newState;
                status = EXIT_SUCCESS;
            }
            break;

        default:
            MDEBUG (DPM_ERROR, "ERROR: unknown new state=%d\n", newState );
    }
    pthread_mutex_unlock( &( sasCtl.lock ) );
    return status;
}

// debugging aid - dump APDU prior to sending
static void dumpApdu( UCHAR * pApdu, USHORT len )
{
    int ii, jj;

    MDEBUG (DPM_SAS, "     0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n" );
    for( ii = 0; ii < len; ii += 0x10 )
    {
        MDEBUG2 (DPM_SAS, "%03x", ii );
        for( jj = 0; jj < 0x10  &&  ii+jj < len; jj++ )
            MDEBUG2 (DPM_SAS, " %02x", pApdu[ ii+jj ] );
        MDEBUG2 (DPM_SAS,"\n");
    }
}

/*static*/ int SAS_APDU_FROM_HOST( short sessNbr, unsigned long tag,UCHAR * pApdu, USHORT len )
{
//  MDEBUG (DPM_SAS, "sessNbr=%d, len=%d\n", sessNbr, len );
//  dumpApdu( pApdu, len );
          unsigned short alen;
    uint8 *papdu;
    if((SAS_CONNECT_RQST == tag) && (sessNbr == -1))
    {
        sessNbr = evGetUnusedOpenSessionNumber();
        if(sessNbr == FALSE)
        {
    
            sessNbr = evGetAnyOpenSessionNumber();
            if(sessNbr == FALSE)
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","SAS_APDU_FROM_HOST: Error!!!  There is no  sessions opened sessNbr:%d\n",sessNbr);
                return -1;
            }
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","SAS_APDU_FROM_HOST: @@@@ There is no unused sessions open sessNbr:%d\n",sessNbr);
        }
    }

	rmf_osal_memAllocP(RMF_OSAL_MEM_POD, (MAX_APDU_HEADER_BYTES) + len, (void **)&papdu);
    if(papdu == NULL)
    return -1;
    if(buildApdu(papdu, &alen, tag, len ,pApdu) == NULL)
    {
        //MDEBUG (DPM_ERROR, "ERROR:ai: Unable to build apdu.\n");
		rmf_osal_memFreeP(RMF_OSAL_MEM_POD, papdu);
        return -1;
    }

  //  AM_APDU_FROM_HOST( sessNbr, pApdu, len );
    AM_APDU_FROM_HOST( sessNbr, papdu, alen );
    rmf_osal_memFreeP(RMF_OSAL_MEM_POD, papdu);
    return (int)sessNbr;

}

        /***  event handlers  ***/
typedef struct
{
    USHORT session;
    UCHAR  inUse;
    UCHAR  valid;

}SASSesnInfo_t;

static SASSesnInfo_t sessions[ MAX_OPEN_SESSIONS ];
static void evOpenSessionReq( ULONG resId, UCHAR tcId )
{
    UCHAR status = 0;
    MDEBUG (DPM_SAS, "resId=0x%x tcId=%d openSessions=%d\n",
            resId, tcId, openSessions );


    if( openSessions < MAX_OPEN_SESSIONS )
    {
        MDEBUG (DPM_SAS, "transid=%x tcId=%x\n",transId,tcId);
      /*  if( transId != tcId )
        {
            status = 0xF0;  // non-existent resource (on this transId)
            MDEBUG (DPM_ERROR, "ERROR: transId in OPEN_SS=%d, exp=%d\n", tcId, transId );
        } else if( RM_Get_Ver( SPEC_APP_ID ) < RM_Get_Ver( resId ) )
        {
            status = 0xF2;  // this version < requested
            MDEBUG (DPM_ERROR, "ERROR: version=0x%x < req=0x%x\n",
                    RM_Get_Ver( SPEC_APP_ID ), RM_Get_Ver( resId ) );
        } else*/
        {
            MDEBUG (DPM_SAS, "SAS session %d opened\n", openSessions+1 );
        }
    } else
    {
        status = 0xF3;      // exists but busy
        MDEBUG (DPM_ERROR, "ERROR: max sessions opened=%d\n", openSessions );
    }
    MDEBUG (DPM_SAS,"calling AM_OPEN_SS_RSP\n");
    AM_OPEN_SS_RSP( status, resId, transId );

}

// we already verified that we can accommodate a new open session above,
// so we can just add the session number to our list here
static void evSessionOpened( USHORT sessNbr, ULONG resId, UCHAR tcId )
{
    int ii;

    MDEBUG (DPM_SAS, "sessNbr=%d resId=0x%x tcId=%d\n",
            sessNbr, resId, tcId );

    //  might want to save resId and/or tcId
    for( ii = 0; ii < MAX_OPEN_SESSIONS; ii++)
    {
        if( sessions[ii].session == 0 && sessions[ii].valid == 0)
        {
            openSessions++;
            sessions[ii].session = sessNbr;
        sessions[ii].valid = 1;
            break;
        }
    }
}

static USHORT    evGetAnyOpenSessionNumber()
{
        int ii;
    for( ii = 0; ii < MAX_OPEN_SESSIONS; ii++)
        {
            if( sessions[ ii ].session != 0 && sessions[ ii ].valid)
                return  sessions[ii].session;
    }
    return FALSE;
}
static USHORT    evGetUnusedOpenSessionNumber()
{
        int ii;
    for( ii = 0; ii < MAX_OPEN_SESSIONS; ii++)
        {
            if( (sessions[ ii ].session != 0) && (sessions[ ii ].valid == 1) && (sessions[ ii ].inUse == 0))
        {
            sessions[ ii ].inUse = 1;
                return  sessions[ii].session;
        }
    }
    return FALSE;
}

static void evCloseSession( USHORT sessNbr )
{
    int ii;

    for( ii = 0; ii < MAX_OPEN_SESSIONS; ii++)
    {
        if( sessions[ ii ].session == sessNbr )
        {
            sessions[ ii ].session =0;
        sessions[ ii ].valid = 0;
        sessions[ ii ].inUse = 0;

            // decrement but make sure it's not negative
            openSessions = ( --openSessions < 0 )  ?  0 : openSessions;

            // flush the queue when no more open sessions remain
            if( openSessions == 0 ){
             //   lcsm_flush_queue( handle, POD_RSMGR_QUEUE ); Hannah
        }
            break;
        }
    }
    MDEBUG (DPM_SAS, "sessNbr=%d openSessions=%d status=%s\n",
            sessNbr, openSessions,
            ( ii < MAX_OPEN_SESSIONS  ? "closed" : "Invalid Session" ) );
}


static void evApdu( USHORT sessNbr, UCHAR * pApdu, USHORT len )
{
    MDEBUG (DPM_SAS, "SAS_APDU_FROM_POD: sessNbr=%d len=%d\n", sessNbr, len );
    dumpApdu( pApdu, len );

    // 1st long int is 24-bit tag + length_field()
//    MDEBUG (DPM_SAS, "pApdu[0]=0x%x shift left by 16=0x%x\n",
//            pApdu[ 0 ], ( (uint32) pApdu[ 0 ]) << 16 );
    uint32 tag = ( ( (uint32) pApdu[ 0 ] ) << 16 )  |
                 ( ( (uint32) pApdu[ 1 ] ) <<  8 )  |
                   ( (uint32) pApdu[ 2 ] );

    // currently, the SAS is a "stub" that only supports
    // the opening of up to 32 sessions with the POD. Since
    // SAS is then host-driven, we should never see any
    // APDUs from the POD because we'll never send the
    // initiating APDU to the POD
    switch( tag )
    {
        default:
            MDEBUG (DPM_ERROR, "WARN: SAS APDU tag=0x%x not supported\n", tag );
            return;
    }
//            rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pApdu );  //  check how POD stack uses mem
}







// the specific application support thread control
void sasProc( void * par )
{
  //  int                   ii;
    LCSM_EVENT_INFO       *msg = (LCSM_EVENT_INFO       *)par;

   // setLoop( 1 );
    //setState( SAS_STARTED );

    //MDEBUG (DPM_ALWAYS, "Thread started, (PID=%d)\n", (int) getpid() );

    // any pre-processing prior to main message pump loop


    // Register with resource manager
    //REGISTER_RESOURCE(SPEC_APP_ID, POD_SAS_QUEUE, MAX_OPEN_SESSIONS);

    //while( sasCtl.loop )
    //{
      //  lcsm_get_event( handle, POD_SAS_QUEUE, &msg, LCSM_WAIT_FOREVER ); Hannah
         MDEBUG (DPM_SAS, "event received\n" );
        switch( msg->event )
        {
            // POD requests open session (x=resId, y=tcId)
            case POD_OPEN_SESSION_REQ:
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","POD_OPEN_SESSION_REQ\n");

             MDEBUG (DPM_SAS, "POD_OPEN_SESSION_REQ\n");
                evOpenSessionReq( (ULONG) msg->x, (UCHAR) msg->y );
                break;
            // POD confirms open session (x=sessNbr, y=resId, z=tcId)
            case POD_SESSION_OPENED:
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","POD_SESSION_OPENED\n");
            MDEBUG (DPM_SAS, "POD_SESSION_OPENED\n");
                evSessionOpened( (USHORT) msg->x, (ULONG) msg->y, (UCHAR) msg->z );
                break;
            // POD closes session (x=sessNbr)
            case POD_CLOSE_SESSION:
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","POD_CLOSE_SESSION\n");
        MDEBUG (DPM_SAS, "POD_CLOSE_SESSION\n");
                evCloseSession( (USHORT) msg->x );
                break;
            // POD sends APDU (x=sessNbr, y=APDU buffer ptr, z=APDU byte length)
            case POD_APDU:
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","POD_APDU\n");
            MDEBUG (DPM_SAS, "POD_APDU\n");
                evApdu( (USHORT) msg->x, (UCHAR *) msg->y, (USHORT) msg->z );
                break;
            // we've been commanded to bail
            // (sasCtl.loop will be cleared by now)
            case EXIT_REQUEST:
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","EXIT_REQUEST\n");
            MDEBUG (DPM_SAS, "EXIT_REQUEST\n");
                break;

            default:
            MDEBUG (DPM_SAS, "default path\n");
                MDEBUG (DPM_ERROR, "ERROR: invalid SAS msg received=0x%x\n", msg->event );
        }
    //}
    //pthread_exit( NULL );
}

        /***  sub-module control  ***/

int sas_init( )
{
int ii;
    sasCtl.state = SAS_INITIALIZED;
    sasCtl.loop = 0;
    if( pthread_mutex_init( &( sasCtl.lock ), NULL ) != 0 )
    {
        MDEBUG (DPM_ERROR, "ERROR: Cant initialize state mutex\n" );
        return EXIT_FAILURE;
    }
    setState( SAS_STARTED );
    for( ii = 0; ii < MAX_OPEN_SESSIONS; ii++ )
    {
         sessions[ ii ].session =0;
     sessions[ ii ].valid = 0;
     sessions[ ii ].inUse = 0;
    }

//  Register with resource manager
    REGISTER_RESOURCE(M_SPEC_APP_ID, POD_SAS_QUEUE, MAX_OPEN_SESSIONS);
    return EXIT_SUCCESS;
}

// Sub-module entry point
int sas_start( LCSM_DEVICE_HANDLE h )
{
    int res;
    int ii;

    MDEBUG (DPM_SAS, "STARTED\n" );
    if( h < 1 )
    {
        MDEBUG (DPM_ERROR, "ERROR: Invalid LCSM device handle=%d\n", h );
        // This failure should be logged by the application manager
        return -EINVAL;
    }
    // Assign handle
    handle = h;
    // Flush queue
 //   lcsm_flush_queue( handle, POD_SAS_QUEUE ); Hannah

    // Initialize SAS's control environment
   // sas_init();

   // for( ii = 0; ii < MAX_OPEN_SESSIONS; ii++ )
   //     sessions[ ii ] = 0;

    /*
     * Start SAS processing thread
     * For now just use default settings for the new thread. It's more important
     * to be able to check the return value that would otherwise be swallowed by
     * the pthread wrapper library call
     */
//    res = (int) pthread_createThread((void *)&sasProcThread, NULL, DEFAULT_STACK_SIZE, 0 ); Hannah
    //res = pthread_create( &sasThread, NULL, (void *)&sasProcThread, NULL );
 //   if( res == 0 )
 //   {
  //      MDEBUG (DPM_ERROR, "ERROR: Unable to create thread=%d\n", res );
  //      return EXIT_FAILURE;
  //  }
    return EXIT_SUCCESS;
}

// Sub-module exit point
int sas_stop()
{
    int status = EXIT_SUCCESS;

    // Terminate message loop
    setLoop( 0 ); setState( SAS_SHUTDOWN );

    // get control loop's attention NOW
    LCSM_EVENT_INFO msg;
    msg.event = EXIT_REQUEST;
    msg.dataPtr = NULL;
    msg.dataLength = 0;

    lcsm_send_immediate_event( handle, POD_SAS_QUEUE, &msg );

    // Wait for message processing thread to join
    if( pthread_join( sasThread, NULL ) != 0 )
    {
        MDEBUG (DPM_ERROR, "ERROR: pthread_join failed\n" );
        status = EXIT_FAILURE;
    }
    return status;
}

#ifdef __cplusplus
}
#endif

