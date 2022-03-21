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
#include <sys_init.h>
#include "utils.h"
#include "appmgr.h"
#include "xchan_interface.h"
#include "podhighapi.h"

#include "core_events.h"

#include "utilityMacros.h"
#include "bufParseUtilities.h"
#include "dsg_types.h"
#include "dsg-main.h"
#include "vlXchanFlowManager.h"
#include "dsgResApi.h"
#include "xchanResApi.h"
#include "rmf_osal_sync.h"
#include "vlpluginapp_haldsgapi.h"
#ifdef GLI
#include "libIBus.h"
#include "sysMgr.h"
#endif
#ifdef __cplusplus
extern "C" {
#endif

// Externs
extern unsigned long RM_Get_Ver( unsigned long id );
#ifdef USE_IPDIRECT_UNICAST
// IPDU register flow
extern int bIPDirectUnicast;					// Run-time env variable indicates if IP Direct Unicast is enabled.
#endif // USE_IPDIRECT_UNICAST

// Prototypes
static void Xchan_OpenSessReq( ULONG resId, UCHAR tcId );
static void Xchan_SessOpened( USHORT sessNbr, ULONG resId, UCHAR tcId );
static void Xchan_CloseSession( USHORT sessNbr );
static int xchan_openSessions( int mode, int value );

int  APDU_HANDLE_NewFlowReq( USHORT sesnum, UCHAR *apkt, USHORT len );
int  APDU_HANDLE_DeleteFlowReq( USHORT sesnum, UCHAR *apkt, USHORT len );
int  APDU_HANDLE_LostFlowInd( USHORT sesnum, UCHAR *apkt, USHORT len );
int  APDU_HANDLE_LostFlowCnf( USHORT sesnum, UCHAR *apkt, USHORT len );
int  APDU_HANDLE_NewFlowCnf( USHORT sesnum, UCHAR *apkt, USHORT len );
int  APDU_HANDLE_DeleteFlowCnf( USHORT sesnum, UCHAR *apkt, USHORT len );

// Globals
static LCSM_DEVICE_HANDLE     handle = 0;
static pthread_t             xchanThread;
static pthread_mutex_t         xchanLoopLock;
static int                     loop;

extern int vlg_bDsgScanFailed;

int vlXchanPostOobAcquired()
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
#ifdef USE_DSG
    if(!vlIsDsgOrIpduMode())
    {
        /* Sep-28-2009: Done on Rx and Tx tune requests
        vlSendDsgEventToPFC(pfcEventCardMgr::CardMgr_OOB_Downstream_Acquired,
                    NULL, 0);
        vlDsgSendBootupEventToPFC(RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD_OOB, RMF_BOOTMSG_DIAG_MESG_CODE_COMPLETE, 0);
        */
        return 1;
    }
#endif
    return 0;
}

void vlXchanSendIPMdataToHost(unsigned long FlowID, int Size, unsigned char * pData)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "vlXchanSendIPMdataToHost: received IP Multicast data from card: flowid = 0x%06X\n", FlowID);
}

// Inline functions
static inline void setLoop(int cont)
{
    pthread_mutex_lock(&xchanLoopLock);
    if (!cont) {
        loop = 0;
    }
    else {
        loop = 1;
    }
    pthread_mutex_unlock(&xchanLoopLock);
}

// Sub-module entry point
int xchan_start(LCSM_DEVICE_HANDLE h)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    int res;

    if (h < 1) {
        MDEBUG (DPM_ERROR, "ERROR: Invalid LCSM device handle.\n");
        // This failure should be logged by the application manager
        return -EINVAL;
    }
    // Assign handle
    handle = h;
    // Flush queue
    //lcsm_flush_queue(handle, POD_XCHAN_QUEUE); Hannah

    //Initialize XCHAN's loop mutex
    if (pthread_mutex_init(&xchanLoopLock, NULL) != 0) {
        MDEBUG (DPM_ERROR, "ERROR: Unable to initialize mutex.\n");
        return EXIT_FAILURE;
    }
    /* Start XCHAN processing thread
       For now just use default settings for the new thread. It's more important
       to be able to check the return value that would otherwise be swallowed by
       the pthread wrapper library call*/
//    res = (int) pthread_createThread((void *)&xchanProcThread, NULL, DEFAULT_STACK_SIZE, 0 ); Hannah
    //res = pthread_create(&xchanThread, NULL, (void *)&xchanProcThread, NULL);
    //if (res == 0) {
    //    MDEBUG (DPM_ERROR, "ERROR: Unable to create thread.\n");
    ////    return EXIT_FAILURE;
    //}
    //MDEBUG (DPM_XCHAN, "EXIT_SUCCESS, s/b registering.\n");
#ifdef USE_DSG
    vlDsgResourceInit();
#endif
    vlXchanInitFlowManager();
    return EXIT_SUCCESS;
}

// Sub-module exit point
int xchan_stop()
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    // Terminate message loop
    setLoop(0);

    // Wait for message processing thread to join
    if (pthread_join(xchanThread, NULL) != 0) {
        MDEBUG (DPM_ERROR, "ERROR: pthread_join failed.\n");
    }
    return 0;//Mamidi:042209
}

void xchanInit(void){

		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    REGISTER_RESOURCE( M_EXT_CHAN_ID, POD_XCHAN_QUEUE, 1 );
}

void xchanUnprotectedProc(void *par)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    char *ptr;
    LCSM_EVENT_INFO       *eventInfo = (LCSM_EVENT_INFO       *)par;
    //setLoop(1);

  //  MDEBUG (DPM_ALWAYS, "Thread started, (PID=%d)\n", (int) getpid() );

    // Register with resource manager
    //REGISTER_RESOURCE( EXT_CHAN_ID, POD_XCHAN_QUEUE, 1 );

//    while (loop)
//    {
    //    lcsm_get_event( handle, POD_XCHAN_QUEUE, &eventInfo, LCSM_WAIT_FOREVER );
        switch (eventInfo->event)
        {
        // POD requests open session (x=resId, y=tcId)
        case POD_OPEN_SESSION_REQ:
            MDEBUG (DPM_XCHAN, "Xchan_OpenSessReq called\n");
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", " POD_OPEN_SESSION_REQ >>>> Extended Channel >>>>>>>>>>> \n");
            Xchan_OpenSessReq( (ULONG)eventInfo->x, (UCHAR)eventInfo->y );
            break;

        // POD confirms open session (x=sessNbr, y=resId, z=tcId)
        case POD_SESSION_OPENED:
            MDEBUG (DPM_XCHAN, "Xchan_SessOpened called\n");
            Xchan_SessOpened( (USHORT)eventInfo->x, (ULONG)eventInfo->y, (UCHAR)eventInfo->z );
            vlSetXChanSession((unsigned short)eventInfo->x);
            vlXchanClearFlowManager();
            if(vlXchanGetResourceId() <= VL_XCHAN_RESOURCE_ID_VER_4)
            {
#ifdef USE_DSG
                CHALDsg_Set_Config(VL_CM_DSG_DEVICE_INSTANCE, VL_DSG_PVT_TAG_DSG_CLEAN_UP, NULL);
                APDU_DSG2POD_InquireDsgMode((USHORT)eventInfo->x, vlMapToXChanTag((USHORT)eventInfo->x, VL_XCHAN_HOST2POD_INQUIRE_DSG_MODE));
#endif
            }
            break;

        // POD closes session (x=sessNbr)
        case POD_CLOSE_SESSION:
            MDEBUG (DPM_XCHAN, "Xchan_CloseSession called\n");
            Xchan_CloseSession( (USHORT)eventInfo->x );
            vlXchanClearFlowManager();
            if(vlXchanGetResourceId() <= VL_XCHAN_RESOURCE_ID_VER_4)
            {
#ifdef USE_DSG
                vlDsgResourceCleanup();
#endif
            }
            break;


            // these events are sent by transport
        // transport opens a flow (x=lcsmHandle, y=pid z=returnQueue)
        case ASYNC_EC_FLOW_REQUEST:
            MDEBUG (DPM_XCHAN, "sendFlowRequest called\n");
            if(eventInfo->x == 0 /*CM_SERVICE_TYPE_MPEG_SECTION*/) //MPEG_SECTION flow
            {
                RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s case ASYNC_EC_FLOW_REQUEST, CM_SERVICE_TYPE_MPEG_SECTION\n", __FUNCTION__);
                sendFlowRequest( 0,eventInfo->x, eventInfo->y, eventInfo->z );
            }
            else if(eventInfo->x == 1 /*CM_SERVICE_TYPE_IP_U*/) //IP_U flow
            {
                RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s case ASYNC_EC_FLOW_REQUEST, CM_SERVICE_TYPE_IP_U\n", __FUNCTION__);
                sendIPFlowRequest( 0,eventInfo->x, (unsigned char *)eventInfo->y, eventInfo->z );
            }
#ifdef USE_IPDIRECT_UNICAST
            else if(eventInfo->x == 6 /*CM_SERVICE_TYPE_IPDU*/) //IPDU flow, IPDU register flow
            {
                RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s case ASYNC_EC_FLOW_REQUEST, CM_SERVICE_TYPE_IPDU\n", __FUNCTION__);
                sendIPDUFlowRequest( 0,eventInfo->x, (unsigned char *)eventInfo->y, eventInfo->z );
            }
#endif // USE_IPDIRECT_UNICAST
            break;

        // transport deletes a flow (x=lcsmHandle, y=flowId z=returnQueue)
        case ASYNC_EC_FLOW_DELETE:
            MDEBUG (DPM_XCHAN, "sendDeleteFlowRequest called\n");
            sendDeleteFlowRequest( eventInfo->x, eventInfo->y, eventInfo->z );
            break;

        // POD sends APDU (x=sessNbr, y=APDU buffer ptr, z=APDU byte length)
        case POD_APDU:
        {
						RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s case POD_APDU \n", __FUNCTION__);
            VL_BYTE_STREAM_INST(POD_APDU, ParseBuf, eventInfo->y, eventInfo->z);
            ULONG ulTag = vlParse3ByteLong(pParseBuf);

            if(0x9F9100 == (ulTag&0xFFFF00))
            {
#ifdef USE_DSG
                dsgProtectedProc(par);
#endif
                return;
            }

            switch(ulTag)
            {
                case VL_XCHAN_POD2HOST_SET_DSG_MODE:
                {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "pod > Host CTL: VL_POD2HOST_SET_DSG_MODE attemped \n");
#ifdef USE_DSG
                    APDU_POD2DSG_SetDsgMode((unsigned short)eventInfo->x, (unsigned char *)eventInfo->y, (unsigned short)eventInfo->z);
                    static VL_DSG_OPERATIONAL_MODE eOperationalMode = VL_DSG_OPERATIONAL_MODE_SCTE55;
                    eOperationalMode = vlGetDsgMode();
                    vlSendDsgEventToPFC(CardMgr_DSG_Operational_Mode,
                                        &(eOperationalMode), sizeof(eOperationalMode));
                    switch(eOperationalMode)
                    {
                        case VL_DSG_OPERATIONAL_MODE_SCTE55:
                        {
                            /* Sep-28-2009: Done on Rx and Tx tune requests
                            vlDsgSendBootupEventToPFC(RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD_OOB, RMF_BOOTMSG_DIAG_MESG_CODE_START, 0);
                            */
                        }
                        break;
                        
                        case VL_DSG_OPERATIONAL_MODE_BASIC_TWO_WAY:
                        case VL_DSG_OPERATIONAL_MODE_ADV_TWO_WAY:
                        case VL_DSG_OPERATIONAL_MODE_IPDM:
                        case VL_DSG_OPERATIONAL_MODE_IPDIRECT_UNICAST:
                        {
                            vlDsgSendBootupEventToPFC(RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD_DSG_TWOWAY, RMF_BOOTMSG_DIAG_MESG_CODE_INIT, 0);
#ifdef GLI
				IARM_Bus_SYSMgr_EventData_t eventData;					
				eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_DSG_CA_TUNNEL;
				eventData.data.systemStates.state = 1;
				eventData.data.systemStates.error = vlg_bDsgScanFailed;
				IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
				eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_DSG_BROADCAST_CHANNEL;
				IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
				system("touch /tmp/dsg_ca_channel");
#endif
                        }
                        break;

                        case VL_DSG_OPERATIONAL_MODE_BASIC_ONE_WAY:
                        case VL_DSG_OPERATIONAL_MODE_ADV_ONE_WAY:
                        {
                            vlDsgSendBootupEventToPFC(RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD_DSG_ONEWAY, RMF_BOOTMSG_DIAG_MESG_CODE_INIT, 0);
#ifdef GLI
				IARM_Bus_SYSMgr_EventData_t eventData;					
				eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_DSG_CA_TUNNEL;
				eventData.data.systemStates.state = 1;
				eventData.data.systemStates.error = vlg_bDsgScanFailed;
				IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
				eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_DSG_BROADCAST_CHANNEL;
				IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
				system("touch /tmp/dsg_ca_channel");
#endif
                        }
                        break;
                    }
#endif
                }
                break;

                case VL_XCHAN_POD2HOST_DSG_ERROR:
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "pod > Host CTL: VL_POD2HOST_DSG_ERROR attemped \n");
#ifdef USE_DSG
                    APDU_POD2DSG_DsgError((unsigned short)eventInfo->x, (unsigned char *)eventInfo->y, (unsigned short)eventInfo->z);
#endif
                }
                break;

                case VL_XCHAN_POD2HOST_CONFIG_ADV_DSG:
                {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "pod > Host CTL: VL_POD2HOST_CONFIG_ADV_DSG attemped \n");
#ifdef USE_DSG
                    APDU_POD2DSG_ConfigAdvDsg((unsigned short)eventInfo->x, (unsigned char *)eventInfo->y, (unsigned short)eventInfo->z);
#endif
                }
                break;
                default:
                {
										RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s case default \n", __FUNCTION__);
                    if(((ulTag&0xFFFF00) != 0x9F8E00) || ((ulTag&0xFF) > 0x0B))
                    {
                        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "invalid TAG 0x%06X for XCHAN of length %d recieved\n", ulTag, eventInfo->z);
                    }
                }
                break;				
            }

            ptr = (char *)eventInfo->y;    // y points to the complete APDU
            switch (*(ptr+2))
            {
                // These APDU handling functions are called in response to transport engine requests
            case POD_XCHAN_NEWFLOW_REQ:
                MDEBUG (DPM_XCHAN, "APDU_HANDLE_NewFlowReq called\n");
                APDU_HANDLE_NewFlowReq ((USHORT)eventInfo->x, (UCHAR *)eventInfo->y, (USHORT)eventInfo->z);
                break;
            case POD_XCHAN_DELFLOW_REQ:
                MDEBUG (DPM_XCHAN, "APDU_HANDLE_DeleteFlowReq called\n");
                APDU_HANDLE_DeleteFlowReq ((USHORT)eventInfo->x, (UCHAR *)eventInfo->y, (USHORT)eventInfo->z);
                break;

            case POD_XCHAN_LOSTFLOW_IND:
                MDEBUG (DPM_XCHAN, "APDU_HANDLE_LostFlowInd called\n");
                APDU_HANDLE_LostFlowInd ((USHORT)eventInfo->x, (UCHAR *)eventInfo->y, (USHORT)eventInfo->z);
                break;

            case POD_XCHAN_LOSTFLOW_CNF:
                MDEBUG (DPM_XCHAN, "APDU_HANDLE_LostFlowCnf called\n");
                APDU_HANDLE_LostFlowCnf ((USHORT)eventInfo->x, (UCHAR *)eventInfo->y, (USHORT)eventInfo->z);
                break;

            case POD_XCHAN_NEWFLOW_CNF:
                MDEBUG (DPM_XCHAN, "APDU_HANDLE_NewFlowCnf called\n");
                APDU_HANDLE_NewFlowCnf ((USHORT)eventInfo->x, (UCHAR *)eventInfo->y, (USHORT)eventInfo->z);
                break;

            case POD_XCHAN_DELFLOW_CNF:
                MDEBUG (DPM_XCHAN, "APDU_HANDLE_DeleteFlowCnf called\n");
                APDU_HANDLE_DeleteFlowCnf ((USHORT)eventInfo->x, (UCHAR *)eventInfo->y, (USHORT)eventInfo->z);
                break;

            /*
            case POD_XCHAN_INQ_DSGMODE:
                MDEBUG (DPM_XCHAN, "APDU_InqDSGMode called\n");
                APDU_HANDLE_InqDSGMode ((USHORT)eventInfo->x, (UCHAR *)eventInfo->y, (USHORT)eventInfo->z);
                break;

            case POD_XCHAN_SET_DSGMODE:
                MDEBUG (DPM_XCHAN, "APDU_SetDSGMode called\n");
                APDU_HANDLE_SetDSGMode ((USHORT)eventInfo->x, (UCHAR *)eventInfo->y, (USHORT)eventInfo->z);
                break;

            case POD_XCHAN_DSGPKT_ERROR:
                MDEBUG (DPM_XCHAN, "APDU_DSGPacketError called\n");
                APDU_HANDLE_DSGPacketError ((USHORT)eventInfo->x, (UCHAR *)eventInfo->y, (USHORT)eventInfo->z);
                break;
*/
                default:
                {
										RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s switch (*(ptr+2) case default \n", __FUNCTION__);
                }
                break;				
            }
        }
        break;

        case EXIT_REQUEST:
            setLoop(0);
            break;
        }
//    }
//    pthread_exit(NULL);
}

void xchanProtectedProc( void * par )
{
#ifdef USE_DSG
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s-1 \n", __FUNCTION__);
    rmf_osal_mutexAcquire(vlDsgMainGetThreadLock());
    {
        xchanUnprotectedProc(par);
    }
    rmf_osal_mutexRelease(vlDsgMainGetThreadLock());
#else // USE_DSG
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s-2 \n", __FUNCTION__);
    xchanUnprotectedProc(par);
#endif // else USE_DSG
}

extern UCHAR   transId;
static void Xchan_OpenSessReq( ULONG resId, UCHAR tcId )
{
    UCHAR status = 0;
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    MDEBUG (DPM_XCHAN, "resId=0x%x tcId=%d openSessions=%d\n", resId, tcId, xchan_openSessions(1,0) );

    if( xchan_openSessions(1,0) == 0 )    // homing session not opened yet
    {
      /*  if( transId != tcId )        // make sure we are using the transId already opened earlier
        {
            status = 0xF0;  // non-existent resource (on this transId)
            MDEBUG (DPM_ERROR, "ERROR: transId in OPEN_SS=%d, exp=%d\n", tcId, transId );
            return;
        }

        if( RM_Get_Ver( EXT_CHAN_ID ) < RM_Get_Ver( resId ) )
        {
            status = 0xF2;  // this version < requested
            MDEBUG (DPM_ERROR, "ERROR: version=0x%x < req=0x%x\n", RM_Get_Ver( EXT_CHAN_ID ), RM_Get_Ver( resId ) );
        }
        else*/
        {
            MDEBUG (DPM_XCHAN, "opening extended channel session (xchan_openSessions=%d)\n",
                    xchan_openSessions(1,0)+1 );
        }
    }
    else
    {
        status = 0xF3;      // exists but busy
        MDEBUG (DPM_ERROR, "ERROR: max sessions opened=%d\n", xchan_openSessions(1,0) );
    }

    if(0 == status) vlXchanSetResourceId(resId);
    AM_OPEN_SS_RSP( status, resId, transId );
}

static void Xchan_SessOpened( USHORT sessNbr, ULONG resId, UCHAR tcId )
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    int value;

    value = xchan_openSessions(1, 0);    // retrieve value first
    value++;
    xchan_openSessions(0, value);// write value back

    MDEBUG (DPM_XCHAN, "extended channel session is opened: sessNbr=%d\n", sessNbr );
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "extended channel session is opened: sessNbr=%d\n", sessNbr );

    // initialization
    initializeSession1( sessNbr );

    if(vl_xchan_session_is_active())
    {
        vl_xchan_async_start_outband_processing();
        vl_xchan_async_start_ip_over_qpsk();
#ifdef USE_IPDIRECT_UNICAST
        // IPDU register flow
        vl_xchan_async_start_ipdu_processing();							// IP Direct processing
#endif // USE_IPDIRECT_UNICAST
    }
}


static void Xchan_CloseSession( USHORT sessNbr )
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    int value;

    vlXchanCancelQpskOobIpTask();

    vl_xchan_async_stop_outband_processing();
    vl_xchan_async_stop_ip_over_qpsk();
#ifdef USE_IPDIRECT_UNICAST
    vl_xchan_async_stop_ipdu_processing();			// IPDU register flow
#endif // USE_IPDIRECT_UNICAST

    value = xchan_openSessions(1, 0);    // retrieve value first
    if (value > 0)
    {
        value--;
        xchan_openSessions(0, value);    // write value back
        MDEBUG (DPM_XCHAN, "extended channel sessNbr=%d closed (xchan_openSessions=%d)\n",
                sessNbr, xchan_openSessions(1,0) );
    }
    else
        MDEBUG (DPM_ERROR, "ERROR: trying to close not opened extended channel session\n");
}


/*
 *    This function keeps the extended channel openSessions counter within itself,
 * thus making it safer, since it is not accessible directly in the global space
 *    mode=0: save   mode=1: retrieve
 */
static int xchan_openSessions( int mode, int value )
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    static int openSessions=0;

    if (mode==0)    // save value into system_openSessions
    {
        openSessions = value;
        return 0;
    }
    else
    {
        return (openSessions);
    }
}

int vl_xchan_session_is_active()
{
    int value = xchan_openSessions(1, 0);   // retrieve value first
    return (value > 0);
}

void vl_xchan_async_start_ip_over_qpsk()
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "vl_xchan_async_start_ip_over_qpsk: Checking XChan Session\n");
    if(vl_xchan_session_is_active())
    {
        if(vlhal_oobtuner_Capable())
        {
	        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "vl_xchan_async_start_ip_over_qpsk: xchan session is active... Starting IP Flow \n");
	        LCSM_EVENT_INFO eventInfo;
	        eventInfo.event = ASYNC_START_OOB_QPSK_IP_FLOW;
	        eventInfo.dataPtr = NULL;
	        eventInfo.dataLength = 0;
	        lcsm_send_event( 0, POD_XCHAN_QUEUE, &eventInfo );
        }
        else
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "vl_xchan_async_start_ip_over_qpsk: Not requesting IPU flow as vlhal_oobtuner_Capable() indicates no OOB tuner is available.\n");
        }
    }
}

void vl_xchan_async_stop_ip_over_qpsk()
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    LCSM_EVENT_INFO eventInfo;

    eventInfo.event = ASYNC_STOP_OOB_QPSK_IP_FLOW;
    eventInfo.dataPtr = NULL;
    eventInfo.dataLength = 0;
    lcsm_send_event( 0, POD_XCHAN_QUEUE, &eventInfo );
}

void vl_xchan_async_start_outband_processing()
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    LCSM_EVENT_INFO eventInfo;

    eventInfo.event = ASYNC_OPEN;
    eventInfo.dataPtr = NULL;
    eventInfo.dataLength = 0;
    lcsm_send_event( 0, POD_XCHAN_QUEUE, &eventInfo );
}


void vl_xchan_async_stop_outband_processing()
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    LCSM_EVENT_INFO eventInfo;

    eventInfo.event = ASYNC_CLOSE;
    eventInfo.dataPtr = NULL;
    eventInfo.dataLength = 0;
    lcsm_send_event( 0, POD_XCHAN_QUEUE, &eventInfo );
}

#ifdef USE_IPDIRECT_UNICAST
// IPDU register flow
void vl_xchan_async_start_ipdu_processing()
{
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    if(bIPDirectUnicast)
    {
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s-bIPDirectUnicast is enabled  \n", __FUNCTION__);
        if(vl_xchan_session_is_active())
        {
            RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s- Sending ASYNC_START_IPDU_XCHAN_FLOW event to POD_XCHAN_QUEUE.\n", __FUNCTION__);
            LCSM_EVENT_INFO eventInfo;
            eventInfo.event = ASYNC_START_IPDU_XCHAN_FLOW;
            eventInfo.dataPtr = NULL;
            eventInfo.dataLength = 0;
            lcsm_send_event( 0, POD_XCHAN_QUEUE, &eventInfo );
        }
    }
}

// IPDU register flow
void vl_xchan_async_stop_ipdu_processing()
{
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    if(bIPDirectUnicast)
    {
        LCSM_EVENT_INFO eventInfo;

        eventInfo.event = ASYNC_STOP_IPDU_XCHAN_FLOW;
        eventInfo.dataPtr = NULL;
        eventInfo.dataLength = 0;
        lcsm_send_event( 0, POD_XCHAN_QUEUE, &eventInfo );
    }
}
#endif // USE_IPDIRECT_UNICAST

#ifdef __cplusplus
}
#endif


