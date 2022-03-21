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
#include <memory.h>
#include "utils.h"
#include "appmgr.h"
#include "feature-main.h"
#include "apdu_feature.h"
#include <string.h>
#include <rdk_debug.h>
#include "rmf_osal_mem.h"

#if USE_SYSRES_MLT
#include "mlt_malloc.h"
#endif

#define __MTAG__ VL_CARD_MANAGER

#ifdef __cplusplus
extern "C" {
#endif

// Externs
extern unsigned long RM_Get_Ver( unsigned long id );
extern void AM_OPEN_SS_RSP( UCHAR Status, ULONG lResourceId, UCHAR Tcid);
static unsigned char vlg_Version = 3;
// Prototypes
void featureProc(void*);

// Globals
static LCSM_DEVICE_HANDLE     handle = 0;
static pthread_t             featureThread;
static pthread_mutex_t         featureLoopLock;
static int                     loop;
static unsigned long fcResourceId;
static FCState fc = {
    state : FC_UNINITIALIZED,
    loop : 0,
    openSessions : 0,
    sessionId : 0
};



// Inline functions
static inline void setLoop(int cont)
{
    pthread_mutex_lock(&featureLoopLock);
    if (!cont) {
        loop = 0;
    }
    else {
        loop = 1;
    }
    pthread_mutex_unlock(&featureLoopLock);
}

// Sub-module entry point
int feature_start(LCSM_DEVICE_HANDLE h)
{
    int res;

    if (h < 1) {
        MDEBUG (DPM_ERROR, "ERROR:feature: Invalid LCSM device handle.\n");
        // This failure should be logged by the application manager
        return -EINVAL;
    }
    // Assign handle
    handle = h;
    // Flush queue
    //lcsm_flush_queue(handle, POD_FEATURE_QUEUE);

    //Initialize FEATURE's loop mutex
 //    if (pthread_mutex_init(&featureLoopLock, NULL) != 0) {
 //        MDEBUG (DPM_ERROR, "ERROR:feature: Unable to initialize mutex.\n");
 //        return EXIT_FAILURE;
 //    }

    /* Start FEATURE processing thread
       For now just use default settings for the new thread. It's more important
       to be able to check the return value that would otherwise be swallowed by
       the pthread wrapper library call*/
//    res = (int) pthread_createThread((void *)&featureProcThread, NULL, DEFAULT_STACK_SIZE, 0 ); Hannah
    //res = pthread_create(&featureThread, NULL, (void *)&featureProcThread, NULL);
    //if (res == 0) {
    //    MDEBUG (DPM_ERROR, "ERROR:feature: Unable to create thread.\n");
    //    return EXIT_FAILURE;
    //}

    return EXIT_SUCCESS;
}

// Sub-module exit point
int feature_stop()
{
    // Terminate message loop
    //setLoop(0);

    // Wait for message processing thread to join
    //if (pthread_join(featureThread, NULL) != 0) {
    //    MDEBUG (DPM_ERROR, "ERROR:feature: pthread_join failed.\n");
    //}

    return EXIT_SUCCESS;//Mamidi:042209
}



int featureSetState(FCStateType newState)
{
    int retval = EXIT_FAILURE;

    pthread_mutex_lock(&(featureLoopLock));

    if ( FC_INITIALIZED == newState) {
	  fc.state = FC_INITIALIZED;
        pthread_mutex_unlock(&(featureLoopLock));
        return EXIT_SUCCESS;
    }

    if (fc.state == newState) {
        pthread_mutex_unlock(&(featureLoopLock));
        return EXIT_SUCCESS;
    }

    switch (fc.state)
    {
        case FC_INITIALIZED:
                if (newState == FC_STARTED) {
                    fc.state = FC_STARTED;
                    retval = EXIT_SUCCESS;
                }
            break;
        case FC_STARTED:
                if (newState == FC_SESSION_OPEN_REQUESTED) {
                    fc.state = FC_SESSION_OPEN_REQUESTED;
                    retval = EXIT_SUCCESS;
                }
            break;
        case FC_SESSION_OPEN_REQUESTED:
                if (newState == FC_SESSION_OPENED) {
                    fc.state = FC_SESSION_OPENED;
                    retval = EXIT_SUCCESS;
                }
            break;
        case FC_SESSION_OPENED:
                if (newState == FC_STARTED) {
                    fc.state = FC_STARTED;
                    retval = EXIT_SUCCESS;
                }

            break;

    }
    pthread_mutex_unlock(&(featureLoopLock));

    return retval;
}

extern UCHAR   transId;
void featureOpenSessionReq( unsigned long resId, unsigned char tcId )
{
    unsigned char status = 0;

    MDEBUG (DPM_MMI, "pod > feature: \n");

    if( fc.openSessions < MAX_OPEN_SESSIONS )
    {
    /*    if( transId != tcId )
        {
            status = 0xF0;  // non-existent resource (on this transId)
            MDEBUG (DPM_ERROR, "ERROR: transId in OPEN_SS=%d, exp=%d\n", tcId, transId );
        } else if( RM_Get_Ver( GEN_FEATURE_ID ) < RM_Get_Ver( resId ) )
        {
            status = 0xF2;  // this version < requested
            MDEBUG (DPM_ERROR, "ERROR: version=0x%x < req=0x%x\n",
                    RM_Get_Ver( GEN_FEATURE_ID ), RM_Get_Ver( resId ) );
        } else*/

        if (featureSetState(FC_SESSION_OPEN_REQUESTED) != EXIT_SUCCESS)
    {
            MDEBUG (DPM_ERROR, "ERROR:feature: Unable to change state in OpenSessionReq.\n");
    }
    MDEBUG (DPM_HOMING, "session %d opened\n", fc.openSessions );
    vlg_Version = resId &0x3F;

    } else
    {
        status = 0xF3;      // exists but busy
        MDEBUG (DPM_ERROR, "ERROR: max sessions opened=%d\n", fc.openSessions );
    }
    AM_OPEN_SS_RSP( status, resId, transId );
}
unsigned char GetFeatureControlVer(void)
{
    return vlg_Version;
}


void featureSessionOpened( unsigned short sessNbr, unsigned long resId, unsigned long tcId )
{
    MDEBUG (DPM_MMI, "pod > feature: \n");
    featureGetList();
    fc.openSessions = 1;
    fc.sessionId = sessNbr;
    fcResourceId = resId;

    if (featureSetState(FC_SESSION_OPENED) != EXIT_SUCCESS) {
        MDEBUG (DPM_FATAL, "pod > feature: Unable to change state in OpenSession.\n");
    }
}

unsigned long fetureGetOpenedResourceId()
{

    if(fc.openSessions != 1)
        return 0;

    return fcResourceId;
}

void featureSessionClosed( uint32 sessNbr )
{
    MDEBUG (DPM_MMI, "pod > feature:\n");

    fc.openSessions = 0;
    fc.sessionId = 0;
    if (featureSetState(FC_STARTED) != EXIT_SUCCESS) {
        MDEBUG (DPM_FATAL, "pod > feature: Unable to change state in CloseSession.\n");
    }
}



int featureSndEvent(SYSTEM_EVENTS event, unsigned char *payload, unsigned short payload_len,
                unsigned int x, unsigned int y, unsigned int z)
{
    LCSM_EVENT_INFO *e;

    if(handle < 1) {
        MDEBUG (DPM_WARN, "pod > fc: Kms interface not initialized.\n");
        return EXIT_FAILURE;
    }

	rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(LCSM_EVENT_INFO),(void **)&e);
    if (e == NULL)
    {
        MDEBUG (DPM_FATAL, "pod > fc: Unable to allocate event.\n");
        return EXIT_FAILURE;
    }

    e->DstQueue = POD_FEATURE_QUEUE;
    e->dataLength = payload_len;
    e->dataPtr = (void*)payload;
    e->event = event;

    //gateway_send_event(handle, e);
    lcsm_send_event( handle, POD_FEATURE_QUEUE, e );

	rmf_osal_memFreeP(RMF_OSAL_MEM_POD, e);

    return EXIT_SUCCESS;
}

void feature_init(void){

    fc.loop = 0;

    fc.openSessions = 0;
    fc.sessionId = 0;
    if (pthread_mutex_init(&(fc.lock), NULL) != 0) {
        MDEBUG (DPM_ERROR, "ERROR:mmi: Unable to initialize state mutex.\n");
        return ;
    }

    featureSetState(FC_INITIALIZED);
    featureSetState(FC_STARTED);
    REGISTER_RESOURCE(M_GEN_FEATURE_ID, POD_FEATURE_QUEUE, 1);
}

void featureProc(void* par)
{
    char *ptr;
    LCSM_EVENT_INFO       *eventInfo = (LCSM_EVENT_INFO       *)par;
    //setLoop(1);

    //MDEBUG (DPM_ALWAYS, "Thread started, (PID=%d)\n", (int) getpid() );

    //while(loop) {
       // lcsm_get_event( handle, POD_CA_QUEUE, &eventInfo, LCSM_WAIT_FOREVER );
    switch (eventInfo->event)
    {

        // POD requests open session (x=resId, y=tcId)
    case POD_OPEN_SESSION_REQ:
            featureOpenSessionReq( (unsigned long) eventInfo->x, (unsigned char) eventInfo->y );
                   break;

    // POD confirms open session (x=sessNbr, y=resId, z=tcId)
    case POD_SESSION_OPENED:
        featureSessionOpened( (unsigned short)eventInfo->x, eventInfo->y, eventInfo->z );
                   break;

    // POD closes session (x=sessNbr)
    case POD_CLOSE_SESSION:
        featureSessionClosed( eventInfo->x );
        ResetCardFeatureList();
                   break;
    case POD_HOST_FEATURE_PARAM_CHANGE:
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","####### Received POD_HOST_FEATURE_PARAM_CHANGE event ############ featureID:%d \n",eventInfo->x);
        APDU_HostToCardFeatureParams(fc.sessionId,(unsigned char)eventInfo->x);
        break;
    // POD sends APDU (x=sessNbr, y=APDU buffer ptr, z=APDU byte length)
    case POD_APDU:
        ptr = (char *)eventInfo->y;    // y points to the complete APDU
//        APDU_Feature ((USHORT)eventInfo->x, (UCHAR *)eventInfo->y, (USHORT)eventInfo->z);
            switch (*(ptr+2))
        {
        case POD_FEATURE_LISTREQ:
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ########### POD_FEATURE_LISTREQ ########### \n");
            APDU_FeatureListReq ((USHORT)eventInfo->x, (UCHAR *)eventInfo->y, (USHORT)eventInfo->z);
            break;
        case POD_FEATURE_LIST:
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ########### POD_FEATURE_LIST ########### \n");
            APDU_FeatureList ((USHORT)eventInfo->x, (UCHAR *)eventInfo->y, (USHORT)eventInfo->z);
            break;
        case POD_FEATURE_LISTCNF:
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ########### POD_FEATURE_LISTCNF ########### \n");
            APDU_FeatureListCnf ((USHORT)eventInfo->x, (UCHAR *)eventInfo->y, (USHORT)eventInfo->z);
            break;
        case POD_FEATURE_LISTCHANGED:
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ########### POD_FEATURE_LISTCHANGED ########### \n");
            APDU_FeatureListChanged ((USHORT)eventInfo->x, (UCHAR *)eventInfo->y,(USHORT)eventInfo->z);
            break;
        case POD_FEATURE_PARMREQ:
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ########### POD_FEATURE_PARMREQ ########### \n");
            APDU_FeatureParmReqFromCard((USHORT)eventInfo->x, (UCHAR *)eventInfo->y, (USHORT)eventInfo->z);
            break;
        case POD_FEATURE_PARM:
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ########### POD_FEATURE_PARM ########### \n");
            APDU_FeatureParmFromCard ((USHORT)eventInfo->x, (UCHAR *)eventInfo->y, (USHORT)eventInfo->z);
            break;
        case POD_FEATURE_PARMCNF:
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ########### POD_FEATURE_PARMCNF ########### \n");
            APDU_FeatureParmCnfFromCard ((USHORT)eventInfo->x, (UCHAR *)eventInfo->y, (USHORT)eventInfo->z);
            break;
        }
                break;
        case EXIT_REQUEST:
            setLoop(0);
            break;
        }
    //}
    //pthread_exit(NULL);

}




 int  APDU_Feature (USHORT sesnum, UCHAR * apkt, USHORT len)
 {
static unsigned char tagArray[7][3]={ 0x9f,0x98, 0x02,0x9f, 0x98, 0x03, 0x9f, 0x98, 0x04, 0x9f, 0x98, 0x05, 0x9f, 0x98, 0x06,0x9f, 0x98, 0x07, 0x9f, 0x98, 0x08};
static unsigned int eventIDArray[] = {POD_FEATURE_LIST_REQ,POD_FEATURE_LIST_X, POD_FEATURE_LIST_CNF, POD_FEATURE_LIST_CHANGE, POD_FEATURE_PARAMS_REQ, POD_FEATURE_PARAMS,                                         POD_FEATURE_PARAMS_CNF};

    unsigned char *ptr;
    int i, j;

    unsigned short len_field = 0;
    unsigned char *payload;
    //unsigned char tagvalue[3] = {0x9f, 0x98, 0x02};    //


     j = (sizeof (eventIDArray))/(sizeof (int));
    ptr = (unsigned char *)apkt;

    for(i= 0; i < j ; i++)
    {



        if (memcmp(ptr, tagArray+(i*3), 3) == 0)
        {

            //MDEBUG (DPM_FATAL, "pod > fc: Non-matching tag in OpenReq.\n");
            //return EXIT_FAILURE;
            break;
        }else{

            if (i == (j - 1))
            {
                MDEBUG (DPM_FATAL, "pod > fc: Non-matching tag in OpenReq.\n");
                return EXIT_FAILURE;

            }
        }
    }
    // Let's continue going about the parsing business
    ptr += 3;

    // Figure out length field


        ptr += bGetTLVLength ( ptr, &len_field );


    // Get the rest of the stuff
	rmf_osal_memAllocP(RMF_OSAL_MEM_POD, len_field * sizeof(unsigned char), (void **)&payload);
    if (payload == NULL) {
        MDEBUG (DPM_FATAL, "pod > fc: Unable to allocate memory for OpenReq.\n");

        return EXIT_FAILURE;
    }

//    memcpy(payload, ptr, len_field*sizeof(unsigned char));
    memcpy(payload, ptr, len_field*sizeof(unsigned char));


    // Now forward the thing to BB
    featureSndEvent( (SYSTEM_EVENTS)eventIDArray[i], payload, len_field, 0, 0, 0);


    //        rmf_osal_memFreeP(RMF_OSAL_MEM_POD,payload); Hannah - done in mmi.cpp in the cardmanager resource
    return 1;
}






#ifdef __cplusplus
}
#endif


