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
#include <lcsm_log.h>                   /* To support log file */

#include <cardmanagergentypes.h>
#include <utils.h>
#include <appmgr.h>
#include <podhighapi.h>
#include <applitest.h>

#include <ai_state_machine_int.h>
#include <appinfo-main.h>
#include <debug.h>
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
//#define DEBUGF(a, ...)    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", a, ## __VA_ARGS__ )
// Globals
static LCSM_DEVICE_HANDLE     handle = 0;
static pthread_t             aiThread;
static unsigned long vlg_CardAppInfoSesVer = 1;
int aiGetOpenedResId()
{
    
    return vlg_CardAppInfoSesVer;
    
}

// Sub-module entry point
int appinfo_start(LCSM_DEVICE_HANDLE h)
{
    int res;

    MDEBUG ( DPM_APPINFO, "ai:Called\n");

    if (h < 1) {
        MDEBUG (DPM_ERROR, "ERROR:ai: Invalid LCSM device handle.\n");
        // This failure should be logged by the application manager
        return -EINVAL;
    }

    // Initialize AI state machine
    //if (aiChangeState(AI_INITIALIZED) != EXIT_SUCCESS) {
       // MDEBUG (DPM_ERROR, "ERROR:ai: Unable to initialize.\n");
    //    return EXIT_FAILURE;
    //}

    // Assign handle
    handle = h;
    // Flush queue
    
#if (0) // MLP -- Don't flush the queue -- This causes a problem when
        // delayed pod stack init is performed to synchronize with BB code    
    //lcsm_flush_queue(handle, POD_APPINFO_QUEUE);
#endif

    /* Start AI processing thread
       For now just use default settings for the new thread. It's more important
       to be able to check the return value that would otherwise be swallowed by
       the pthread wrapper library call*/
//    res = (int) pthread_createThread((void *)&aiProcThread, NULL, DEFAULT_STACK_SIZE, 0 ); Hannah
    //res = pthread_create(&aiThread, NULL, (void *)&aiProcThread, NULL);
    //if (res == 0) {
    //    MDEBUG (DPM_ERROR, "ERROR:ai: Unable to create thread.\n");
    //    return EXIT_FAILURE;
    //}

    return EXIT_SUCCESS;
}

// Sub-module exit point
int appinfo_stop()
{
    MDEBUG ( DPM_APPINFO, "ai:Called \n");

    // Terminate message loop
    aiChangeState(AI_EXIT);

    // Make sure we come back from blocking on the queue so we can exit
    LCSM_EVENT_INFO e;
    e.event = EXIT_REQUEST;
    e.dataPtr    = NULL;      // no copying of dataPtr
    e.dataLength = 0;         // no copying of dataPtr

    lcsm_send_immediate_event(handle, POD_APPINFO_QUEUE, &e);

    // Wait for message processing thread to join
    //if (pthread_join(aiThread, NULL) != 0) {
    //    MDEBUG (DPM_ERROR, "ERROR:ai: pthread_join failed.\n");
    //}

    return EXIT_SUCCESS;
}


void aiInit(void ){

    if (aiChangeState(AI_INITIALIZED) != EXIT_SUCCESS) {
            MDEBUG (DPM_ERROR, "ERROR:ai: Unable to initialize.\n");
        return ;
    }
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","aiInit entry..\n");
    if (aiChangeState(AI_STARTED) != EXIT_SUCCESS) {
        MDEBUG (DPM_ERROR, "ERROR:ai: Unable to start.\n");
        return;
    }
 
    // Register with resource manager
    REGISTER_RESOURCE(M_APP_INFO_ID, POD_APPINFO_QUEUE, 1);    
}    

void aiProc(void* par)
{
    char *ptr;
    LCSM_EVENT_INFO       *eventInfo = (LCSM_EVENT_INFO       *)par;

 //   MDEBUG (DPM_ALWAYS, "Thread started, (PID=%d)\n", (int) getpid() );

/*    if (aiChangeState(AI_STARTED) != EXIT_SUCCESS) {
        MDEBUG (DPM_ERROR, "ERROR:ai: Unable to start.\n");
        return;
    }

    // Register with resource manager
    REGISTER_RESOURCE(APP_INFO_ID, POD_APPINFO_QUEUE, 1);
*/
   // MDEBUG ( DPM_APPINFO, "ai:Entering event loop.\n");

//    while(!aiExit()) {

//        lcsm_get_event(handle, POD_APPINFO_QUEUE, &eventInfo, LCSM_WAIT_FOREVER );  //Hannah

        switch (eventInfo->event) {

            // POD requests open session (x=resId, y=tcId)
            case POD_OPEN_SESSION_REQ:
                aiOpenSessionReq( (uint32) eventInfo->x, (uint8) eventInfo->y );
                break;

            // POD confirms open session (x=sessNbr, y=resId, z=tcId)
            case POD_SESSION_OPENED:
                aiSessionOpened( (uint16)eventInfo->x, eventInfo->y, eventInfo->z );
                break;

            // POD closes session (x=sessNbr)
            case POD_CLOSE_SESSION:
                aiSessionClosed( (uint16)eventInfo->x );
                break;

            // Send by Burbank (contains info to be sent to the POD when session is opened)
            case POD_OPEN:
             
                aiRcvAppInfo( (pod_appInfo_t *)eventInfo->dataPtr );
             
                break;

            case POD_SERVER_QUERY:
                aiRcvServerQuery ((uint8 *)eventInfo->dataPtr, (uint32)eventInfo->dataLength);
                break;

            // POD sends APDU (x=sessNbr, y=APDU buffer ptr, z=APDU byte length)
            case POD_APDU:
                ptr = (char *)eventInfo->y;    // y points to the complete APDU

                switch (*(ptr+2)) {
                    // APDU is Application_Info_Cnf
                    case POD_APPINFO_CNF:
                        aiRcvAppInfoCnf((uint16)eventInfo->x, (void *)eventInfo->y, eventInfo->z);
                        break;

                    // APDU is Server_reply
                    case POD_APPINFO_SERVERREPLY:
                        aiRcvServerReply((uint16)eventInfo->x, (void *)eventInfo->y, eventInfo->z);
                        break;
#if 0
                    // POD sends APDU (x=session id, y = apdu, z = length)
                    case POD_MMI_CLOSEREQ:
                        mmiDlgCloseReq((unsigned short)eventInfo.x, (void *)eventInfo.y, eventInfo.z);
                        break;
#endif
                    }
                break;

            case EXIT_REQUEST:
                aiChangeState(AI_EXIT);
                break;
        }
  //  }
//    pthread_exit(NULL);
}

int aiSndEvent(SYSTEM_EVENTS event, uint8 *payload, uint16 payload_len,
                uint32 x, uint32 y, uint32 z)
{
      LCSM_EVENT_INFO *e;

      MDEBUG ( DPM_APPINFO, "ai:Called \n");

      if(handle < 1) {
              MDEBUG (DPM_WARN, "ai:Kms interface not initialized.\n");
              //mainAddLogEntry(LOG_WARNING, "Kms interface not initiaized.\n");
              return EXIT_FAILURE;
      }

	rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(LCSM_EVENT_INFO), (void **)&e);
      if (e == NULL) {
              MDEBUG (DPM_WARN, "ai:Unable to allocate event.\n");
              //mainAddLogEntry(LOG_FATAL, "Unable to allocate event.\n");
              return EXIT_FAILURE;
      }

      e->DstQueue = POD_APPINFO_QUEUE;
      e->dataLength = payload_len;
      e->dataPtr = (void*)payload;
      e->event = event;

      RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","aiSndEvent:: calling lcsm_send_event\n");
      //gateway_send_event(handle, e); //Hannah
      lcsm_send_event( handle, POD_APPINFO_QUEUE, e );

      rmf_osal_memFreeP(RMF_OSAL_MEM_POD, e);

      return EXIT_SUCCESS;
}

void aiOpenSessionReq( uint32 resId, uint8 tcId )
{

    uint8 status = 0;
    
    MDEBUG ( DPM_APPINFO, "ai:Called \n");
     if( !aiSessionOpen() )
    {
      /*  if( transId != tcId )
        {
            status = 0xF0;  // non-existent resource (on this transId)
            MDEBUG (DPM_WARN, "WARN:ai::transId in OPEN_SS=%d, exp=%d\n", tcId, transId );
        } else if( RM_Get_Ver( APP_INFO_ID ) < RM_Get_Ver( resId ) )
        {
            status = 0xF2;  // this version < requested
            MDEBUG (DPM_WARN, "WARN:ai:version=0x%x < req=0x%x\n",
                    RM_Get_Ver( APP_INFO_ID ), RM_Get_Ver( resId ) );
        } else*/
        {
            if (aiChangeState(AI_SESSION_OPEN_REQUESTED) != EXIT_SUCCESS)
            {
                MDEBUG (DPM_FATAL, "FATAL:ai:Unable to change state in OpenSessionReq.\n");
            }
            MDEBUG (DPM_SS, "ai:session opened\n" );
        }
    } else
    {
        status = 0xF3;      // exists but busy
        MDEBUG (DPM_ERROR, "ERROR:ai: max sessions opened\n" );
    }
    AM_OPEN_SS_RSP( status, resId, transId );

}

void aiSessionOpened( uint16 sessNbr, uint32 resId, uint32 tcId )
{
    uint16 len;
    uint8 *data;

    MDEBUG ( DPM_APPINFO, "ai:Called \n");

    if (aiChangeState(AI_SESSION_OPENED) != EXIT_SUCCESS) {
        MDEBUG (DPM_ERROR, "ERROR:ai: Unable to change state to session opened.\n");
    }
    aiSetSessionId(sessNbr);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," ############ vlg_CardAppInfoSesVer :0x%X \n",resId);
    vlg_CardAppInfoSesVer = (int)resId;
    // Check if we already have the appinfo from BB
    if ((data = aiInfo(&len)) == NULL) {
        MDEBUG (DPM_ERROR, "ERROR:ai: Data is null.\n");        
        return;
    }
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","aiSessionOpened: requesting application_info_req >>>>>>>>\n");
    // If we do, send application_info_req
    if (aiSndApdu(0x9F8020, len, data) != EXIT_SUCCESS) {
        // do something here to recover..
        MDEBUG (DPM_ERROR, "ERROR:ai: Couldn't send APDU.\n");        
        
        return;
    }
    aiChangeState(AI_APPINFO_SENT);
}

void aiSessionClosed( uint16 sessNbr )
{

    MDEBUG ( DPM_APPINFO, "ai:Called \n");

    if (aiChangeState(AI_STARTED) != EXIT_SUCCESS) {
        MDEBUG (DPM_ERROR, "ERROR:ai: Unable to change state in CloseSession.\n");
    }
}

int aiSndApdu(uint32 tag, uint32 dataLen, uint8 *data)
{
    uint16 alen;
    uint8 papdu[MAX_APDU_HEADER_BYTES + dataLen];

    MDEBUG ( DPM_APPINFO, "ai:Called, tag = 0x%x, len = %d \n", tag, dataLen);

    if (buildApdu(papdu, &alen, tag, dataLen, data ) == NULL) {
        MDEBUG (DPM_ERROR, "ERROR:ai: Unable to build apdu.\n");
        return EXIT_FAILURE;
    }

    AM_APDU_FROM_HOST(aiSessionId(), papdu, alen);

    return EXIT_SUCCESS;
}

// RAJASREE 11/08/07 [start] to send raw apdu from java app
int aiSndRawApdu(uint32 tag, uint32 dataLen, uint8 *data)
{
        uint16 sesnId;
    sesnId = aiSessionId();
    AM_APDU_FROM_HOST(sesnId, data, dataLen);

    return (int)sesnId;
}
// RAJASREE 11/08/07 [end] 


void aiRcvAppInfo( pod_appInfo_t *data )
{
    uint16 payload_len = 9;
    uint16 len;
    static uint8 payload[20];
    uint8 *ptr;

    MDEBUG ( DPM_APPINFO, "ai:Called \n");
     RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","\n\n aiRcvAppInfo: Entered >>>>>>>>>>>>>..... \n");
  /*  if (aiInfo(&len) != NULL) {
                rmf_osal_memFreeP(RMF_OSAL_MEM_POD,data);
        data = NULL;
        MDEBUG (DPM_ERROR, "ERROR:ai: Data pointer is null.\n");        
        
        return;
    }
 */
    ptr = payload;
    
    //Note: BYTE_SWAP necessary for 16 bit values
    //*((uint16 *)ptr) = (uint16)BYTE_SWAP(data->displayCols);
    *(ptr++) = (data->displayCols & 0xff00) >> 8;
    *(ptr++) = (data->displayCols & 0x00ff);
    //*((uint16 *)ptr) = (uint16)BYTE_SWAP(data->displayRows);
    *(ptr++) = (data->displayRows & 0xff00) >> 8;
    *(ptr++) = (data->displayRows & 0x00ff);
    *ptr = data->vertScroll;
    ptr++;
    *ptr = data->horizScroll;
    ptr++;
    *ptr = data->multiWin;
    ptr++;
    *ptr = data->dataEntry;
    ptr++;
    *ptr = data->html;

    if (data->html) {
        payload_len += 5;
        ptr++;
        *ptr = data->link;
        ptr++;
        *ptr = data->form;
        ptr++;
        *ptr = data->table;
        ptr++;
        *ptr = data->list;
        ptr++;
        *ptr = data->image;
    }

    
    aiSetInfo(payload, payload_len);
     
    // Check if we already have a session open
    if (aiSessionOpen()) {
        // If we do, send application_info_req
        if (aiSndApdu(0x9F8020, payload_len, payload) != EXIT_SUCCESS) {
            // do something here to recover.
        }
        aiChangeState(AI_APPINFO_SENT);
    }
     
    rmf_osal_memFreeP(RMF_OSAL_MEM_POD, data);  // this is allocated outside in CardManager - appinfo code
    data = NULL;
}



void aiRcvAppInfoCnf(uint16 sessId, void *pApdu, uint32 apduLen)
{
    uint8 *ptr;

    uint32 tag = 0;
    uint16 len_field = 0;
    uint8 *payload;
    unsigned char tagvalue[3] = {0x9f, 0x80, 0x21};

    MDEBUG ( DPM_APPINFO, "ai:Called \n");

    if (pApdu == NULL) {
        MDEBUG (DPM_ERROR, "ERROR:ai: No data passed to AppInfoCnf.\n");
        return;
    }

    ptr = (uint8 *)pApdu;
    // Get / check the tag to make sure we got a good packet
    if (memcmp(ptr, tagvalue, 3) != 0) {
        MDEBUG (DPM_ERROR, "ERROR:ai: Non-matching tag in AppInfoCnf.\n");
        return;
    }

    len_field = getPayloadStart(&ptr);

	rmf_osal_memAllocP(RMF_OSAL_MEM_POD, len_field * sizeof(uint8), (void **)&payload);
    if (payload == NULL) {
        MDEBUG (DPM_ERROR, "ERROR:ai: Unable to allocate memory for AppInfoCnf.\n");
        return;
    }
//    memcpy(payload, ptr, len_field*sizeof(uint8));
        memcpy(payload, ptr, len_field*sizeof(uint8));

    // Now forward the thing to BB
    
    MDEBUG (DPM_ALWAYS, ERROR_COLOR "\n\n\nai:Sending POD_AI_APPINFO_CNF to BB\n\n\n\n" RESET_COLOR);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," Calling aiSndEvent POD_AI_APPINFO_CNF:%X \n",POD_AI_APPINFO_CNF);
    aiSndEvent(POD_AI_APPINFO_CNF, payload, len_field, 0, 0, 0);

    if (aiChangeState(AI_APPINFO_CONFIRMED) != EXIT_SUCCESS) {
        MDEBUG (DPM_ERROR, "ERROR:ai: Unable to change state in AppInfoCnf.\n");
    }
    //        rmf_osal_memFreeP(RMF_OSAL_MEM_POD,payload); Hannah
}

void aiRcvServerQuery (uint8 *data, uint32 dataLen)
{

    uint8 Data[100];
    MDEBUG ( DPM_APPINFO, "ai:Called \n");
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","aiRcvServerQuery : called \n");
    if (aiGetState() != AI_APPINFO_CONFIRMED) {
        MDEBUG (DPM_ERROR, "ERROR:ai: Incorrect state to send apdu in aiRcvQuery. Query dropped.\n");
        return;
    }
    //podapi has already packaged the payload.
    //So just send it!

    int i;
    MDEBUG (DPM_APPINFO, "ai:Data length is: %d\n", dataLen);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","ai:Data length is: %d\n", dataLen);
    //data[dataLen]=0;
    //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s\n", data);
    for (i=0; i < dataLen; i++)
    {
        MDEBUG2 (DPM_APPINFO, "%x ", data[i]);

    }
    //    dataLen = 27;
    MDEBUG2 (DPM_APPINFO, "\n");
      /*  Data[0]=0;//tans num
    Data[1]=0;//header len
    Data[2]=0;//header len
    Data[3]=0;//(unsigned char)dataLen;
    Data[4]=(unsigned char)dataLen;
    memcpy(&Data[5],"CableCARD///apps/diag.html",dataLen);*/
    if (aiSndApdu(0x9F8022, dataLen, data) != EXIT_SUCCESS) {
        MDEBUG (DPM_ERROR, "ERROR:ai: Unable to send apdu in aiRcvQuery.\n");
        return;
    }
}
int TestServerQuery(unsigned char *data, int dataLen)
{
    #define TEST_BUF_SIZE  100
    uint8 Data[TEST_BUF_SIZE];
    int i;
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","TestServerQuery : called \n");
    if (aiGetState() != AI_APPINFO_CONFIRMED) {
       RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "TestServerQuery:ERROR:ai: Incorrect state to send apdu in aiRcvQuery. Query dropped.\n");
        return -1;
    }
    //podapi has already packaged the payload.
    //So just send it!

    
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","TestServerQuery:Data length is: %d\n", dataLen);
    
    //data[dataLen]=0;
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","TestServerQuery : Received :%s\n", data);
    for (i=0; i < dataLen; i++)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%x ", data[i]);

    }
    
    MDEBUG2 (DPM_APPINFO, "\n");
        Data[0]=0;//tans num
    Data[1]=0;//header len
    Data[2]=0;//header len
    Data[3]=0;//(unsigned char)dataLen;
    Data[4]=(unsigned char)dataLen;
    //memcpy(&Data[5],data,dataLen);
    memcpy(&Data[5],data,dataLen);
    if (aiSndApdu(0x9F8022, (dataLen+5), Data) != EXIT_SUCCESS)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","TestServerQuery:ERROR:ai: Unable to send apdu in aiRcvQuery.\n");
        return -1;
    }
    return 0;
}
void aiRcvServerReply (uint16 sessId, void *pApdu, uint32 apduLen)
{
    uint8 *ptr;

    uint16 len_field = 0;
    uint8 *payload;
    unsigned char tagvalue[3] = {0x9f, 0x80, 0x23};

    MDEBUG (DPM_APPINFO, "ai:Called\n");

    if (pApdu == NULL) {
        MDEBUG (DPM_ERROR, "ERROR:ai: No data passed to AppInfoCnf.\n");
        return;
    }

    ptr = (uint8 *)pApdu;
    if (memcmp(ptr, tagvalue, 3) != 0) {
        MDEBUG (DPM_ERROR, "ERROR:ai: Non-matching tag in AppInfoCnf.\n");
        return;
    }

    len_field = getPayloadStart(&ptr);

	rmf_osal_memAllocP(RMF_OSAL_MEM_POD, len_field * sizeof(uint8), (void **)&payload);
    if (payload == NULL) {
        MDEBUG (DPM_ERROR, "ERROR:ai: Unable to allocate memory for AppInfoCnf.\n");
        return;
    }
    //memcpy(payload, ptr, len_field*sizeof(uint8));
    memcpy(payload, ptr, len_field*sizeof(uint8));

    
    MDEBUG (DPM_ALWAYS, ERROR_COLOR "\n\n\nai:Sending POD_SERVER_REPLY to BB\n\n\n\n" RESET_COLOR);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","aiRcvServerReply  payload len=%d\n",len_field);
    // Now forward the thing to BB
    aiSndEvent(POD_SERVER_REPLY, payload, len_field, 0, 0, 0);

    //        rmf_osal_memFreeP(RMF_OSAL_MEM_POD,payload); Hannah
}


#ifdef __cplusplus
}
#endif


