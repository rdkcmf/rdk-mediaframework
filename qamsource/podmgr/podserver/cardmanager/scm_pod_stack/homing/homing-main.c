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
#include "appmgr.h"
#include "homing_sm.h"
#include <rdk_debug.h>
#ifdef __cplusplus
extern "C" {
#endif

// Prototypes
static inline void setLoop( int cont );
void homingProc( void* par );
int homing_start( LCSM_DEVICE_HANDLE h );
int homing_stop( void );

// Globals
static LCSM_DEVICE_HANDLE     handle = 0;
static pthread_t             homingThread;
static pthread_mutex_t         homingLoopLock;
static int                     loop;

// Inline functions
static inline void setLoop( int cont )
{
    pthread_mutex_lock(&homingLoopLock);
    if (!cont) {
        loop = 0;
    }
    else {
        loop = 1;
    }
    pthread_mutex_unlock(&homingLoopLock);
}

// Sub-module entry point
int homing_start( LCSM_DEVICE_HANDLE h )
{
    int res;
/*
    if (h < 1) {
        MDEBUG (DPM_ERROR, "ERROR:homing: Invalid LCSM device handle.\n");
        // This failure should be logged by the application manager
        return -EINVAL;
    }
    */
    // Assign handle
    handle = h;
    // Flush queue
//    lcsm_flush_queue(handle, POD_HOMING_QUEUE); Hannah

    //Initialize HOMING's loop mutex
    if (pthread_mutex_init(&homingLoopLock, NULL) != 0) {
        MDEBUG (DPM_ERROR, "ERROR:homing: Unable to initialize mutex.\n");
        return EXIT_FAILURE;
    }

    /* Start HOMING processing thread
       For now just use default settings for the new thread. It's more important
       to be able to check the return value that would otherwise be swallowed by
       the pthread wrapper library call*/
//    res = (int) pthread_createThread((void *)&homingProcThread, NULL, DEFAULT_STACK_SIZE, 0 ); Hannah
    //res = pthread_create(&homingThread, NULL, (void *)&homingProcThread, NULL);
    //if (res == 0) {
    //    MDEBUG (DPM_ERROR, "ERROR:homing: Unable to create thread.\n");
    //    return EXIT_FAILURE;
    //}

    return EXIT_SUCCESS;
}

// Sub-module exit point
int homing_stop( void )
{
    // Terminate message loop
    setLoop(0);

    // Wait for message processing thread to join
    //if (pthread_join(homingThread, NULL) != 0) {
    //    MDEBUG (DPM_ERROR, "ERROR:homing: pthread_join failed.\n");
    //}
    return 0;//Mamidi:042209
}

void hominginit()
{

     initializeHomingStateMachine();
     REGISTER_RESOURCE(M_HOMING_ID, POD_HOMING_QUEUE, 1);
}

void homingProc( void* par )
{
    unsigned char *ptr;
    
    LCSM_EVENT_INFO       *eventInfo = (LCSM_EVENT_INFO       *)par;
//    setLoop( 1 );

 //   MDEBUG (DPM_ALWAYS, "Thread started, (PID=%d)\n", (int) getpid() );
    
    // Register with resource manager


   

   // lcsm_register_event( handle, STANDBY_NOTIFICATION, POD_HOMING_QUEUE ); Hannah

//    while( loop ) 
    //{
        //lcsm_get_event(handle, POD_HOMING_QUEUE, &eventInfo, LCSM_WAIT_FOREVER ); //Hannah
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","eventInfo->event =%x\n",eventInfo->event);
        if ( eventInfo->event == EXIT_REQUEST )
        {
           // setLoop( 0 );
            return;//break;
        }
        else
        {   
           // if ( eventInfo->event == POD_APDU )
           // {
            //    ptr = (unsigned char*)eventInfo->y;    // y points to the complete APDU
            //    eventInfo->event = *(ptr+2);
            //    MDEBUG(DPM_HOMING, "got event %u \n", (unsigned)(eventInfo->event));
           // }
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," calling processHomingStateMachine\n");

            processHomingStateMachine( eventInfo );
        }
 //   }
    
   // lcsm_unregister_event( handle, STANDBY_NOTIFICATION, POD_HOMING_QUEUE ); Hannah

 //   shutdownHomingStateMachine();
//    pthread_exit( NULL );
}

#ifdef __cplusplus
}
#endif

