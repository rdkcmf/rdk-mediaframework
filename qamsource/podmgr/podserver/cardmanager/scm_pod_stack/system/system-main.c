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
#include <string.h>

// MR includes
#include <pthread.h>
#include <global_event_map.h>
#include <global_queues.h>
#include "poddef.h"
#include <lcsm_log.h>
#include "core_events.h"
#include "cardUtils.h"
#include "rmf_osal_event.h"

#include "utils.h"
#include "appmgr.h"
#include "transport.h"
#include "session.h"
#include "podhighapi.h"
#include "system-main.h"
#include "dsgClientController.h"
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
// Prototypes
void systemProc(void*);

// Globals
static LCSM_DEVICE_HANDLE     handle = 0;
static pthread_t             systemThread;
static pthread_mutex_t         systemLoopLock;
static int                     loop;

static int Type1 = 1, Type2 = 2;//  Resource connection type1 and type2
static int vlg_ResoureceSesConnection = 0;
static USHORT systemControlSessionNumber = 0;
static int    systemControlSessionState  = 0;
// above
static ULONG  ResourceId    = 0;
static UCHAR  Tcid          = 0;
static ULONG  tmpResourceId = 0; //tmp for binding
static UCHAR  tmpTcid       = 0; //tmp for binding


int vlSystemGetResourceSesConectionType(void)
{
    return vlg_ResoureceSesConnection;
}
static USHORT //sesnum
lookupSysCtrlSessionValue(void)
{
    if(  (systemControlSessionState == 1) && (systemControlSessionNumber) )
           return systemControlSessionNumber;
//else
    return 0;
}

void AM_OPEN_SS_REQ_SYS_CONTROL(ULONG RessId, UCHAR Tcid)
{
    if (systemControlSessionState != 0)
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
void AM_SS_OPENED_SYS_CONTROL(USHORT SessNumber, ULONG lResourceId, UCHAR Tcid)
{
    if (systemControlSessionState != 0)
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
    systemControlSessionNumber = SessNumber;
    systemControlSessionState  = 1;
    if(ResourceId == M_SYSTEM_CONTROL_T1_ID)
        vlg_ResoureceSesConnection = Type1;
    else if(ResourceId == M_SYSTEM_CONTROL_T2_ID)
        vlg_ResoureceSesConnection = Type2;
//not sure we send this here, I believe its assumed to be successful
//AM_OPEN_SS_RSP( SESS_STATUS_OPENED, ResourceId, Tcid);
    return;
}

//after the fact, info-ing that closure has occured.
void AM_SS_CLOSED_SYS_CONTROL(USHORT SessNumber)
{
    if((systemControlSessionState)  && (systemControlSessionNumber)  && (systemControlSessionNumber != SessNumber ))
       {
           //don't reject but also request that the pod close (perhaps) this orphaned session
           AM_CLOSE_SS(systemControlSessionNumber);
       }
    ResourceId      = 0;
    Tcid            = 0;
    tmpResourceId   = 0;
    tmpTcid         = 0;
    systemControlSessionNumber = 0;
    systemControlSessionState  = 0;
    vlg_ResoureceSesConnection = 0;
}

// Inline functions
static inline void setLoop(int cont)
{
    pthread_mutex_lock(&systemLoopLock);
    if (!cont) {
        loop = 0;
    }
    else {
        loop = 1;
    }
    pthread_mutex_unlock(&systemLoopLock);
}

// Sub-module entry point
int system_start(LCSM_DEVICE_HANDLE h)
{
    int res;

    if (h < 1) {
        MDEBUG (DPM_ERROR, "ERROR:system: Invalid LCSM device handle.\n");
        // This failure should be logged by the application manager
        return -EINVAL;
    }
    // Assign handle
    handle = h;
    // Flush queue
//    lcsm_flush_queue(handle, POD_SYSTEM_QUEUE);

    //Initialize SYSTEM's loop mutex
//     if (pthread_mutex_init(&systemLoopLock, NULL) != 0) {
//         MDEBUG (DPM_ERROR, "ERROR:system: Unable to initialize mutex.\n");
//         return EXIT_FAILURE;
//     }

    /* Start SYSTEM processing thread
       For now just use default settings for the new thread. It's more important
       to be able to check the return value that would otherwise be swallowed by
       the pthread wrapper library call*/
//    res = (int) pthread_createThread((void *)&systemProcThread, NULL, DEFAULT_STACK_SIZE, 0 ); Hannah
    //res = pthread_create(&systemThread, NULL, (void *)&systemProcThread, NULL);
    //if (res == 0) {
    //    MDEBUG (DPM_ERROR, "ERROR:system: Unable to create thread.\n");
    ///    return EXIT_FAILURE;
    //}

    return EXIT_SUCCESS;
}

// Sub-module exit point
int system_stop()
{
    // Terminate message loop
    setLoop(0);

    // Wait for message processing thread to join
    if (pthread_join(systemThread, NULL) != 0) {
        MDEBUG (DPM_ERROR, "ERROR:system: pthread_join failed.\n");
    }
    return EXIT_SUCCESS;//Mamidi:042209
}
static unsigned long vlg_RegCliId;
int vlSysDsbBrdTunCVTCallBack(unsigned long             nRegistrationId,
                                      unsigned long             nAppData,
                                      VL_DSG_CLIENT_ID_ENC_TYPE eClientType,
                                      unsigned long long        nClientId,
                                      unsigned long             nBytes,
                                      unsigned char *           pData)
{
        rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
        rmf_osal_event_handle_t event_handle;
        rmf_osal_event_params_t event_params = {0};

        unsigned char *pTemp,*pPkcsCvt;
        unsigned char section_syntax_indicator;
        unsigned char private_indicator;
        unsigned long prv_section_len;


        //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlSysDsbBrdTunCVTCallBack: Entered into the callback function from Dsg \n");
        if( (pData == NULL) || (nBytes < 3) )
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlSysDsbBrdTunCVTCallBack: Error in Data received ....pData = 0x%X or nBytes:%d \n",pData, nBytes );
            return -1;
        }
        if( (vlg_RegCliId != nRegistrationId) && (vlg_RegCliId != 0) )
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD"," vlSysDsbBrdTunCVTCallBack: Callback Registration Ids not matching...vlg_RegCliId:%d != nRegistrationId:%d \n",vlg_RegCliId, nRegistrationId);
            return -1;
        }
        if(eClientType != VL_DSG_CLIENT_ID_ENC_TYPE_BCAST)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD"," vlSysDsbBrdTunCVTCallBack: eClientType:%d is not matching with VL_DSG_CLIENT_ID_ENC_TYPE_BCAST ... \n",eClientType);
            return -1;
        }
        if(nClientId != VL_DSG_BCAST_CLIENT_ID_XAIT)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD"," vlSysDsbBrdTunCVTCallBack: nClientId:%d is not matching with VL_DSG_BCAST_CLIENT_ID_XAIT \n",nClientId);
            return -1;
        }
        pTemp = pData;
        if( *pData == 0xD9 )
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD"," vlSysDsbBrdTunCVTCallBack: Table Id(0xD9) is not matching with received TableId:0x%X \n",*pData);
            return -1;
        }
        pData++;
        nBytes--;

        section_syntax_indicator = (*pData >> 7) & 1;
        private_indicator = (*pData >> 6) & 1;
        if((section_syntax_indicator != 0 ) || (private_indicator != 0))
        {
            //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlSysDsbBrdTunCVTCallBack: section_syntax_indicator=%d private_indicator=%d \n",section_syntax_indicator,private_indicator);
            return -1;
        }
        prv_section_len = ((pData[0] & 0x0F) << 8 ) | (pData[1]);

        if(prv_section_len > 4093)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlSysDsbBrdTunCVTCallBack: Error in prv_section_len=%d is > 4093\n",prv_section_len);
            return -1;
        }
        pData += 2;
        nBytes -= 2;

        if( (nBytes == 0) || (prv_section_len == 0) )
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlSysDsbBrdTunCVTCallBack: Error in Data received nBytes=%d  or prv_section_len:%d\n",nBytes,prv_section_len);
            return -1;
        }
        if(nBytes != prv_section_len)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlSysDsbBrdTunCVTCallBack: Error in Data received nBytes:%d not equals prv_section_len:%d\n",nBytes,prv_section_len);
            return -1;
        }
		rmf_osal_memAllocP(RMF_OSAL_MEM_POD, prv_section_len, (void **)&pPkcsCvt);
        if(pPkcsCvt == NULL)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlSysDsbBrdTunCVTCallBack: Error mem allocation failed\n");
            return -1;
        }
        memcpy(pTemp,pData,prv_section_len);

        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlSysDsbBrdTunCVTCallBack: Posting the Event CommonDownL_CVT_DSG  \n");

        event_params.priority = 0; //Default priority;
        event_params.data = (void *) pPkcsCvt;
        event_params.data_extension = prv_section_len;
        event_params.free_payload_fn = vlSystemFreeMem;
        rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_COM_DOWNL_MANAGER, CommonDownL_CVT_DSG , 
		&event_params, &event_handle);        	
        rmf_osal_eventmanager_dispatch_event(em, event_handle);

        return 0;
}

void system_init(void)
{
    REGISTER_RESOURCE(M_SYSTEM_CONTROL_ID, POD_SYSTEM_QUEUE, 1);

    SysCtlReadVendorData();

vlg_RegCliId =                                                   vlDsgRegisterClient((VL_DSG_CLIENT_CBFUNC_t)vlSysDsbBrdTunCVTCallBack,0,VL_DSG_CLIENT_ID_ENC_TYPE_BCAST,VL_DSG_BCAST_CLIENT_ID_XAIT);

}

void systemProc(void* par)
{
    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    rmf_osal_event_handle_t event_handle;
    char *ptr;
    LCSM_EVENT_INFO       *eventInfo = (LCSM_EVENT_INFO       *)par;
    //setLoop(1);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," ###### Entered systemProc \n");
  //  MDEBUG (DPM_ALWAYS, "Thread started, (PID=%d)\n", (int) getpid() );

    //while(loop) {
     //   lcsm_get_event( handle, POD_CA_QUEUE, &eventInfo, LCSM_WAIT_FOREVER );
        switch (eventInfo->event)
        {
                // POD requests open session (x=resId, y=tcId)
                case POD_OPEN_SESSION_REQ:
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ########### POD_OPEN_SESSION_REQ ####### \n");
            AM_OPEN_SS_REQ_SYS_CONTROL((uint32)eventInfo->x, (uint8)eventInfo->y);
                    break;

        // POD confirms open session (x=sessNbr, y=resId, z=tcId)
        case POD_SESSION_OPENED:
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ########### POD_OPEN_SESSION_REQ ####### \n");
            AM_SS_OPENED_SYS_CONTROL((uint16)eventInfo->x, (uint32)eventInfo->y, (uint8)eventInfo->z);

            rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_COM_DOWNL_MANAGER, CommonDownL_Sys_Ctr_Ses_Open , 
    		NULL, &event_handle);        	
            
            rmf_osal_eventmanager_dispatch_event(em, event_handle);
                    break;

        // POD closes session (x=sessNbr)
        case POD_CLOSE_SESSION:
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ########### POD_CLOSE_SESSION ####### \n");
            AM_SS_CLOSED_SYS_CONTROL((uint16)eventInfo->x);

            rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_COM_DOWNL_MANAGER, CommonDownL_Sys_Ctr_Ses_Close , 
    		NULL, &event_handle);        	

            rmf_osal_eventmanager_dispatch_event(em, event_handle);            
           break;

        // POD sends APDU (x=sessNbr, y=APDU buffer ptr, z=APDU byte length)
        case POD_APDU:
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ###### POD_APDU ######## \n");
                    ptr = (char *)eventInfo->y;    // y points to the complete APDU
            switch (*(ptr+2))
            {
                case POD_SYSTEM_HOSTINFO_REQ:
                    APDU_HostInfoReq ((USHORT)eventInfo->x, (UCHAR *)eventInfo->y, (USHORT)eventInfo->z);
                        break;
                case POD_SYSTEM_HOSTINFO_RESP:
                    APDU_HostInfoResponse ((USHORT)eventInfo->x, (UCHAR *)eventInfo->y, (USHORT)eventInfo->z);
                    break;
                case POD_SYSTEM_CODEVER_TABLE:
                case POD_SYSTEM_CODEVER_TABLE2:
                    APDU_CodeVerTable ((USHORT)eventInfo->x, (UCHAR *)eventInfo->y, (USHORT)eventInfo->z);
                    break;
                case POD_SYSTEM_CODEVER_TABLE_REPLY:
                    APDU_CodeVerTableReply ((USHORT)eventInfo->x, (UCHAR *)eventInfo->y, (USHORT)eventInfo->z);
                    break;
                case POD_SYSTEM_HOSTDOWNLOAD_CTL:
                    APDU_HostDownloadCtl (lookupSysCtrlSessionValue(), eventInfo->y, (USHORT)eventInfo->z);
                    break;
                //case POD_SYSTEM_HOSTDOWNLOAD_CMD:
                //    APDU_HostDownloadCmd ((USHORT)eventInfo->x, (UCHAR *)eventInfo->y, (USHORT)eventInfo->z);
                    //break;
            }
                break;
        case POD_SYSTEM_DOWNLD_CTL:
            APDU_HostDownloadCtl (lookupSysCtrlSessionValue(), eventInfo->y, (USHORT)eventInfo->z);
                    break;
            case EXIT_REQUEST:
                //setLoop(0);
                break;
        }
    //}
    //pthread_exit(NULL);
}


#ifdef __cplusplus
}
#endif

