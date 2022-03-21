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

#include <utils.h>
#include <appmgr.h>
#include <podhighapi.h>
#include <applitest.h>

#include <mmi-main.h>
#include <memory.h>
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

// Globals
static LCSM_DEVICE_HANDLE     handle = 0;
static pthread_t             mmiThread;
static MMIState mmi = {
    state : MMI_UNINITIALIZED,
    loop : 0,
    dlgId : -1,
    openSessions : 0,
    sessionId : 0
};

// Inline fucntions
static inline void setLoop(int cont)
{
    pthread_mutex_lock(&(mmi.lock));
    if (!cont) {
        mmi.loop = 0;
    }
    else {
        mmi.loop = 1;
    }
    pthread_mutex_unlock(&(mmi.lock));
}


int mmi_init(void)
{
    mmi.loop = 0;
    mmi.dlgId = -1;
    mmi.openSessions = 0;
    mmi.sessionId = 0;
    if (pthread_mutex_init(&(mmi.lock), NULL) != 0) {
        MDEBUG (DPM_ERROR, "ERROR:mmi: Unable to initialize state mutex.\n");
        return EXIT_FAILURE;
    }

    mmiSetState(MMI_INITIALIZED);
    mmiSetState(MMI_STARTED);
    REGISTER_RESOURCE(M_MMI_ID, POD_MMI_QUEUE, 1);
    return EXIT_SUCCESS;
}


int mmiSetState(MMIStateType newState)
{
    int retval = EXIT_FAILURE;

    pthread_mutex_lock(&(mmi.lock));

    if (mmi.state == newState) {
        pthread_mutex_unlock(&(mmi.lock));
        return EXIT_SUCCESS;
    }

    switch (mmi.state)
    {
        case MMI_INITIALIZED:
                if (newState == MMI_STARTED) {
                    mmi.state = MMI_STARTED;
                    retval = EXIT_SUCCESS;
                }
            break;
        case MMI_STARTED:
                if (newState == MMI_SESSION_OPEN_REQUESTED) {
                    mmi.state = MMI_SESSION_OPEN_REQUESTED;
                    retval = EXIT_SUCCESS;
                }
            break;
        case MMI_SESSION_OPEN_REQUESTED:
                if (newState == MMI_SESSION_OPENED) {
                    mmi.state = MMI_SESSION_OPENED;
                    retval = EXIT_SUCCESS;
                }
            break;
        case MMI_SESSION_OPENED:
                if (newState == MMI_STARTED) {
                    mmi.state = MMI_STARTED;
                    retval = EXIT_SUCCESS;
                }
#if 0
                if (newState == MMI_DLG_OPEN_REQUESTED) {
                    mmi.state = MMI_DLG_OPEN_REQUESTED;
                    retval = EXIT_SUCCESS;
                }
#endif
            break;
#if 0
        case MMI_DLG_OPEN_REQUESTED:
                if (newState == MMI_DLG_OPENED) {
                    mmi.state = MMI_DLG_OPENED;
                    retval = EXIT_SUCCESS;
                }
                if (newState == MMI_SESSION_OPENED) {
                    mmi.state = MMI_SESSION_OPENED;
                    retval = EXIT_SUCCESS;
                }
            break;
        case MMI_DLG_OPENED:
                if (newState == MMI_SESSION_OPENED) {
                    mmi.state = MMI_SESSION_OPENED;
                    retval = EXIT_SUCCESS;
                }
            break;
#endif
    }
    pthread_mutex_unlock(&(mmi.lock));

    return retval;
}

// Sub-module entry point
int mmi_start(LCSM_DEVICE_HANDLE h)
{
    int res;

    MDEBUG ( DPM_MMI, "pod > mmi: Entering \n");

    if (h < 1) {
        MDEBUG (DPM_ERROR, "ERROR:mmi: Invalid LCSM device handle.\n");
        // This failure should be logged by the application manager
        return -EINVAL;
    }

    // Initialize MMI state machine
/*    if (mmi.state == MMI_UNINITIALIZED) {
        if (mmi_init() != EXIT_SUCCESS) {
            MDEBUG (DPM_ERROR, "ERROR:mmi: Unable to initialize.\n");
            return EXIT_FAILURE;
        }
    }
*/
    // Assign handle
    handle = h;
    // Flush queue
//    lcsm_flush_queue(handle, POD_MMI_QUEUE); Hannah

    /* Start MMI processing thread
       For now just use default settings for the new thread. It's more important
       to be able to check the return value that would otherwise be swallowed by
       the pthread wrapper library call*/
//    res = (int) pthread_createThread((void *)&mmiProcThread, NULL, DEFAULT_STACK_SIZE, 0 ); Hannah
    //res = pthread_create(&mmiThread, NULL, (void *)&mmiProcThread, NULL);
    //if (res == 0) {
    //    MDEBUG (DPM_ERROR, "ERROR:mmi: Unable to create thread.\n");
    //    return EXIT_FAILURE;
    //}

    return EXIT_SUCCESS;
}

// Sub-module exit point
int mmi_stop()
{
    // Terminate message loop
    setLoop(0);

    // Make sure we come back from blocking on the queue so we can exit
    LCSM_EVENT_INFO e;
    e.event = EXIT_REQUEST;
    e.dataPtr = NULL;
    e.dataLength = 0;

    lcsm_send_immediate_event(handle, POD_MMI_QUEUE, &e);

    // Wait for message processing thread to join
    if (pthread_join(mmiThread, NULL) != 0) {
        MDEBUG (DPM_ERROR, "ERROR:mmi: pthread_join failed.\n");
    }

    return EXIT_SUCCESS;
}




void mmiProc(void* par)
{
    char *ptr;
    LCSM_EVENT_INFO       *eventInfo = (LCSM_EVENT_INFO       *)par;
//    setLoop(1);

 //   MDEBUG (DPM_ALWAYS, "Thread started, (PID=%d)\n", (int) getpid() );



//    while(mmi.loop) {

//        lcsm_get_event(handle, POD_MMI_QUEUE, &eventInfo, LCSM_WAIT_FOREVER );   commented by Hannah

        switch (eventInfo->event) {

            // POD requests open session (x=resId, y=tcId)
            case POD_OPEN_SESSION_REQ:
                mmiOpenSessionReq( (unsigned long) eventInfo->x, (unsigned char) eventInfo->y );
                break;

            // POD confirms open session (x=sessNbr, y=resId, z=tcId)
            case POD_SESSION_OPENED:
                mmiSessionOpened( (unsigned short)eventInfo->x, eventInfo->y, eventInfo->z );
                break;

            // POD closes session (x=sessNbr)
            case POD_CLOSE_SESSION:
                mmiSessionClosed( eventInfo->x );
                break;

            // BB reply for POD_MMI_OPENREQ (x=dialog number, y = status)
            case POD_OPEN_MMI_CONF:
                mmiDlgOpenCnf((unsigned char)eventInfo->x, (unsigned char)eventInfo->y);
                break;

            // BB reply for POD_MMI_CLOSEREQ or BB notification to close dlg (x=dialog number)
            case POD_CLOSE_MMI_CONF:
                mmiDlgCloseCnf( (unsigned short)eventInfo->x);
                break;

            // POD sends APDU (x=sessNbr, y=APDU buffer ptr, z=APDU byte length)
            case POD_APDU:
                    ptr = (char *)eventInfo->y;    // y points to the complete APDU
                    switch (*(ptr+2)) {
                        // POD sends APDU (x=session id, y = apdu, z = length)
                        case POD_MMI_OPENREQ:
                                mmiDlgOpenReq((unsigned short)eventInfo->x, (void *)eventInfo->y, eventInfo->z);
                            break;
                        // POD sends APDU (x=session id, y = apdu, z = length)
                        case POD_MMI_CLOSEREQ:
                                mmiDlgCloseReq((unsigned short)eventInfo->x, (void *)eventInfo->y, eventInfo->z);
                            break;
                    }
                break;

            case EXIT_REQUEST:
                setLoop(0);
                break;
        }
 //   }
//    pthread_exit(NULL);
}


int mmiSndEvent(SYSTEM_EVENTS event, unsigned char *payload, unsigned short payload_len,
                unsigned int x, unsigned int y, unsigned int z)
{
    LCSM_EVENT_INFO *e;

    if(handle < 1) {
        MDEBUG (DPM_WARN, "pod > mmi: Kms interface not initialized.\n");
        return EXIT_FAILURE;
    }

	rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(LCSM_EVENT_INFO),(void **)&e);
    if (e == NULL) {
        MDEBUG (DPM_FATAL, "pod > mmi: Unable to allocate event.\n");
        return EXIT_FAILURE;
    }

    e->DstQueue = POD_MMI_QUEUE;
    e->dataLength = payload_len;
    e->dataPtr = (void*)payload;
    e->event = event;

    //gateway_send_event(handle, e);
      lcsm_send_event( handle, POD_MMI_QUEUE, e );

	rmf_osal_memFreeP(RMF_OSAL_MEM_POD, e);

    return EXIT_SUCCESS;
}

void mmiOpenSessionReq( unsigned long resId, unsigned char tcId )
{
    unsigned char status = 0;

    MDEBUG (DPM_MMI, "pod > mmi: \n");

    if( mmi.openSessions < MAX_OPEN_SESSIONS )
    {
       /* if( transId != tcId )
        {
            status = 0xF0;  // non-existent resource (on this transId)
            MDEBUG (DPM_ERROR, "ERROR: transId in OPEN_SS=%d, exp=%d\n", tcId, transId );
        } else if( RM_Get_Ver( MMI_ID ) < RM_Get_Ver( resId ) )
        {
            status = 0xF2;  // this version < requested
            MDEBUG (DPM_ERROR, "ERROR: version=0x%x < req=0x%x\n",
                    RM_Get_Ver( MMI_ID ), RM_Get_Ver( resId ) );
        } else*/
        {
            if (mmiSetState(MMI_SESSION_OPEN_REQUESTED) != EXIT_SUCCESS)
            {
                MDEBUG (DPM_ERROR, "ERROR:mmi: Unable to change state in OpenSessionReq.\n");
            }
            MDEBUG (DPM_HOMING, "session %d opened\n", mmi.openSessions );
        }
    } else
    {
        status = 0xF3;      // exists but busy
        MDEBUG (DPM_ERROR, "ERROR: max sessions opened=%d\n", mmi.openSessions );
    }
    AM_OPEN_SS_RSP( status, resId, transId );
}

void mmiSessionOpened( unsigned short sessNbr, unsigned long resId, unsigned long tcId )
{
    MDEBUG (DPM_MMI, "pod > mmi: \n");

    mmi.openSessions = 1;
    mmi.sessionId = sessNbr;
    if (mmiSetState(MMI_SESSION_OPENED) != EXIT_SUCCESS) {
        MDEBUG (DPM_FATAL, "pod > mmi: Unable to change state in OpenSession.\n");
    }
}

void mmiSessionClosed( uint32 sessNbr )
{
    MDEBUG (DPM_MMI, "pod > mmi:\n");

    mmi.openSessions = 0;
    mmi.sessionId = 0;
    if (mmiSetState(MMI_STARTED) != EXIT_SUCCESS) {
        MDEBUG (DPM_FATAL, "pod > mmi: Unable to change state in CloseSession.\n");
    }
}

void mmiDlgOpenReq(unsigned short sId, void *data, unsigned long length)
{
    unsigned char *ptr;

    unsigned long tag = 0;
    unsigned short len_field = 0;
    unsigned char *payload;
    unsigned char tagvalue[3] = {0x9f, 0x88, 0x20};    //

    MDEBUG (DPM_MMI, "pod > mmi:\n");

    // BB keeps state to make sure there's only one dialog

    ptr = (unsigned char *)data;
    // Get / check the tag to make sure we got a good packet
#if 0    // replaced by the following memcmp
    memcpy((&tag+1), ptr, 3*sizeof(unsigned char));
    if (tag != 0x9F8820) {
#endif
    if (memcmp(ptr, tagvalue, 3) != 0) {
        MDEBUG (DPM_FATAL, "pod > mmi: Non-matching tag in OpenReq.\n");
        return;
    }

#if 0
    // Now that we know we got a good packet, change state
    if (mmiSetState(MMI_DLG_OPEN_REQUESTED) != EXIT_SUCCESS) {
        MDEBUG (DPM_FATAL, "pod > mmi: Unable to change state in OpenReq.\n");
        return;
    }
    mmi.dlgId = 0;
#endif

    // Let's continue going about the parsing business
    ptr += 3;

    // Figure out length field
#if (1)

    ptr += bGetTLVLength ( ptr, &len_field );

#else /* Old hard coded way to find length -- remove */
    if ((*ptr & 0x82) == 0x82) { // two bytes
        ptr++;
        len_field = *(unsigned short *)(ptr);
        ptr += 2;
    }
    else if ((*ptr & 0x81) == 0x81) { // one byte
        ptr++;
        len_field = (unsigned char)(*ptr);
        ptr++;
    }
    else { // lower 7 bits
        len_field = (unsigned char)(*ptr & 0x7F);
        ptr++;
    }
#endif
    // Get the rest of the stuff
	rmf_osal_memAllocP(RMF_OSAL_MEM_POD, len_field,(void **)&payload);
    if (payload == NULL) {
        MDEBUG (DPM_FATAL, "pod > mmi: Unable to allocate memory for OpenReq.\n");
        mmiSetState(MMI_SESSION_OPENED);
        return;
    }
//    memcpy(payload, ptr, len_field*sizeof(unsigned char));
        memcpy(payload, ptr, len_field);

    // Now forward the thing to BB
    mmiSndEvent(POD_OPEN_MMI_REQUEST, payload, len_field, 0, 0, 0);

    //        rmf_osal_memFreeP(RMF_OSAL_MEM_POD,payload); Hannah - done in mmi.cpp in the cardmanager resource
//         rmf_osal_memFreeP(RMF_OSAL_MEM_POD,data);        // no need to free the APDU. It was passed on through a reusable circular buffer
                        // see AM_APDU_FROM_HOST()
}

static void mmiDlgOpenCnf(unsigned char dlgNum, unsigned char status)
{
    unsigned long tag;
    unsigned char data[2];

    MDEBUG (DPM_MMI, "pod > mmi:\n");

#if 0
    if (status == 0) {
        if (mmiSetState(MMI_DLG_OPENED) != EXIT_SUCCESS) {
            MDEBUG (DPM_FATAL, "pod > mmi: Unable to change state in OpenCnf.\n");
            return;
        }
        mmi.dlgId = dlgNum;
    }
    else {
        if (mmiSetState(MMI_SESSION_OPENED) != EXIT_SUCCESS) {
            MDEBUG (DPM_FATAL, "pod > mmi: Unable to change state in OpenCnf.\n");
            return;
        }
        mmi.dlgId = -1;
    }
#endif

    tag = 0x9F8821;
    data[0] = dlgNum;
    data[1] = status;
    mmiSndApdu(tag, 2 * sizeof(unsigned char), data);
}

static void mmiDlgCloseReq(unsigned short sId, void *data, unsigned long length)
{
    unsigned char *ptr;
    unsigned short dlgNum;

    MDEBUG (DPM_MMI, "pod > mmi: \n");

#if 0
    if (mmi.dlgId == -1) {
        // since there doesn't seem to be a dialog openened...
        return;
    }

    // Go back to MMI_SESSION_OPENED state
    if (mmiSetState(MMI_SESSION_OPENED) != EXIT_SUCCESS) {
        MDEBUG (DPM_FATAL, "pod > mmi: Unable to change state in CloseReq.\n");
        return;
    }
    mmi.dlgId = -1;
#endif

    // Extract session id from apdu
    ptr = (unsigned char *)data;
    dlgNum = (unsigned short)(*(ptr+3));

#if 0
    // Check session ID - just for fun
    if (mmi.dlgId != dlgNum) {
        MDEBUG (DPM_FATAL, "pod > mmi: Warning DlgId does not match.\n");
    }
#endif

    // Forward to BB
    mmiSndEvent(POD_CLOSE_MMI_REQUEST, NULL, 0, dlgNum, 0, 0);

//         rmf_osal_memFreeP(RMF_OSAL_MEM_POD,data);        // no need to free the APDU. It was passed on through a reusable circular buffer
                        // see AM_APDU_FROM_HOST()
}

static void mmiDlgCloseCnf(unsigned short dlgNum)
{
    unsigned long resp_tag;
    unsigned char resp_data[1];

    MDEBUG (DPM_MMI, "pod > mmi: \n");

#if 0
    // Go back to MMI_SESSION_OPENED state
    if (mmiSetState(MMI_SESSION_OPENED) != EXIT_SUCCESS) {
        MDEBUG (DPM_FATAL, "pod > mmi: Unable to change state in CloseReq.\n");
        return;
    }
    mmi.dlgId = -1;
#endif

    resp_tag = 0x9F8823;
    resp_data[0] = dlgNum;
    mmiSndApdu(resp_tag, sizeof(unsigned char), resp_data);
}

int mmiSndApdu(unsigned long tag, unsigned short dataLen, unsigned char *data)
{
    unsigned short alen;
    unsigned char papdu[MAX_APDU_HEADER_BYTES + dataLen];

    MDEBUG (DPM_MMI, "pod > mmi: \n");

    if (buildApdu(papdu, &alen, tag, dataLen, data ) == NULL) {
        MDEBUG (DPM_FATAL, "pod > mmi: Unable to build apdu.\n");
        return -1;
    }

    // don't send when we don't have a session open -bcp
    if (mmi.sessionId != 0)
    {
        AM_APDU_FROM_HOST(mmi.sessionId, papdu, alen);
    }

    return (int)mmi.sessionId;
}

#ifdef __cplusplus
}
#endif

