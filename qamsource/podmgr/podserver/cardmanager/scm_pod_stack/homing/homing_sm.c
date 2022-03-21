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


#include <string.h>
#include "poddef.h"
#include "lcsm_log.h"                   /* To support log file */

#include "cs_state_machine.h"
#include "global_queues.h"
#include "global_event_map.h"
#include "cardmanagergentypes.h"

#include "utils.h"
#include "appmgr.h"
#include "ci.h"   // for pod and pcmcia reset, CIHomingStateChange()

#include "transport.h"
#include "hal_pod_api.h"
#include <string.h>
#include "core_events.h"
#include "cardUtils.h"
#include "rmf_osal_event.h"
#include "sys_api.h"
#include <rdk_debug.h>
#include "rmf_osal_mem.h"

#if USE_SYSRES_MLT
#include "mlt_malloc.h"
#endif

#define __MTAG__ VL_CARD_MANAGER



#define POD_HOMING_DOWNLOAD_TIMEOUT     0x101
#define DISABLE_TRANSPORT_TIMEOUT   TRUE
#define ENABLE_TRANSPORT_TIMEOUT    FALSE
#define HOMING_STARTED  TRUE
#define HOMING_ENDED    FALSE
extern "C" void PostBootEvent(int eEventType, int nMsgCode, unsigned long ulData);
extern "C" void vlMpeosPostHomingCompleteEvent(int success);
#ifdef __cplusplus
extern "C" {
#endif
typedef enum
{
    SYSTEM_POWER_STATE_ON,
    SYSTEM_POWER_STATE_STANDBY,
    SYSTEM_POWER_STATE_UNKNOWN
}ePowerState;

// Externs
extern unsigned long RM_Get_Ver( unsigned long id );
extern void AM_OPEN_SS_RSP( UCHAR Status, ULONG lResourceId, UCHAR Tcid);

LCSM_DEVICE_HANDLE hmLCSMHandle;
static CS_SM_CONTROL *hmStateMachine = NULL;
static USHORT gSessNum = 0;
static LCSM_TIMER_HANDLE downloadTimeout = (LCSM_TIMER_HANDLE)0;
static Bool standbyState = FALSE;
static int openSessions=0;
static LCSM_TIMER_HANDLE vlg_TimeHandle  = 0;
static int vlg_CardFirmwareUpgradeStart= 0;
typedef enum
{
    HM_INITIAL_STATE        = 0,
    HM_SESSION_OPENED       = 1,
    HM_OPEN                 = 2,
    HM_FIRMWARE_UPGRADE     = 3,
    HM_STATE_MACHINE_NUMBER = 4,

} HM_STATES;
static int vlg_HomingSessState = 0;
//Disabled #define due to Jenkins build error // NOTIFY_IARM_CARD_FWDWLD // Please fix Jenkin's before enabling it back // TRUE
#ifndef HEADLESS_GW
// Re-enable code hidden by NOTIFY_IARM_CARD_FWDWLD.  If Jenkins build fails, disable next line.
#define NOTIFY_IARM_CARD_FWDWLD 1
#endif // HEADLESS_GW

// prototypes
static void Homing_OpenSessReq( ULONG resId, UCHAR tcId );
static void Homing_SessOpened( USHORT sessNbr, ULONG resId, UCHAR tcId );
static void Homing_CloseSession( USHORT sessNbr );
static int homing_openSessions( int mode, int value );
static void setTimeoutType( unsigned char timeout_type, unsigned short download_timeout_period, unsigned queue );

//--- local states
static void hmInitialState(     void *handle, unsigned queue, CS_SM_CONTROL *smControl, LCSM_EVENT_INFO *eventInfo );
static void hmSessionOpened(    void *handle, unsigned queue, CS_SM_CONTROL *smControl, LCSM_EVENT_INFO *eventInfo );
static void hmOpen(             void *handle, unsigned queue, CS_SM_CONTROL *smControl, LCSM_EVENT_INFO *eventInfo );
static void hmFirmwareUpgrade(  void *handle, unsigned queue, CS_SM_CONTROL *smControl, LCSM_EVENT_INFO *eventInfo );

#ifndef HEADLESS_GW
#ifdef NOTIFY_IARM_CARD_FWDWLD
#ifdef USE_IARM_BUS
	#include "libIBus.h"
	#include "sysMgr.h"
	static void notifyIARMCardFWDNLDEvent(IARM_Bus_SYSMGR_FWDNLDState_t state)
	{
		IARM_Bus_SYSMgr_EventData_t eventData;
        eventData.data.cardFWDNLD.status = state;
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","<<<<< IARM_BUS_SYSMGR_EVENT_CARD_FWDNLD Status :%d >>>> \n",state);
        IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t)IARM_BUS_SYSMGR_EVENT_CARD_FWDNLD,(void *)&eventData, sizeof(eventData));	
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," after <<<<< IARM_BUS_SYSMGR_EVENT_CARD_FWDNLD Status :%d >>>> \n",state);
	}
#else
	#include "uidev.h"
	static void notifyIARMCardFWDNLDEvent(UIDev_EventId_t eventId,char eventType)
	{
	  UIDev_EventData_t *eventData = NULL;
	  UIDev_EventData_t eventData1;
	  eventData=(UIDev_EventData_t *)&eventData1;
	  eventData->id = eventId;
	  eventData->data.cardFWDNLD.eventType = eventType;
	  eventData->data.cardFWDNLD.status=(char)eventType;
	  UIDev_PublishEvent(eventId, eventData);
         RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","<<<<< notifyIARMCardFWDNLDEvent >>>> \n");	  
	}
#endif
#endif //NOTIFY_IARM_CARD_FWDWLD
#endif //HEADLESS_GW

static void PrintFirmWareUpgradeParams(unsigned char upgrade_source, unsigned short download_time, unsigned char timeout_type, unsigned short download_timeout_period)
{
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","PrintFirmWareUpgradeParams: Homing Download Params \n");
RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","upgrade_source:%d \ndownload_time:%d \ntimeout_type:%d \ndownload_timeout_period:%d \n",upgrade_source, download_time, timeout_type,download_timeout_period);
}

static void  HomingDownloadTimeoutTrigger( LCSM_DEVICE_HANDLE  handle, unsigned queue,   LCSM_EVENT_INFO *eventInfo)
{
    lcsm_send_event(handle,queue,eventInfo);

}
static unsigned HomingGenTimedEvent( LCSM_DEVICE_HANDLE  handle, unsigned queue,   LCSM_EVENT_INFO *eventInfo, unsigned milliseconds,int count )
{

//Implement the Timer to call the function HomingDownloadTimeoutTriggerLCSM_DEVICE_HANDLE

    if ( downloadTimeout != (LCSM_TIMER_HANDLE)0 )
        {
            if(LCSM_TIMER_OP_OK == podResetTimer(vlg_TimeHandle,milliseconds))
                return milliseconds;
        }
    if(count > 1)
        count = 1;
    while(count)
    {
        vlg_TimeHandle = podSetTimer( queue, eventInfo, milliseconds );
        count--;
    }
    return milliseconds;

}
static void HomingCancelTimedEvent(LCSM_DEVICE_HANDLE  handle, unsigned timeout)
{
    podCancelTimer( vlg_TimeHandle );
    vlg_TimeHandle = 0;
    downloadTimeout = (LCSM_TIMER_HANDLE)0;
}
void initializeHomingStateMachine( void  )
{

    hmStateMachine = CS_SM_constructStateMachine( NULL, HM_STATE_MACHINE_NUMBER );

    CS_SM_addState( hmStateMachine,  HM_INITIAL_STATE,      hmInitialState );

    CS_SM_addState( hmStateMachine,  HM_SESSION_OPENED,     hmSessionOpened );

    CS_SM_addState( hmStateMachine,  HM_OPEN,               hmOpen );

    CS_SM_addState( hmStateMachine,  HM_FIRMWARE_UPGRADE,   hmFirmwareUpgrade );


    CS_SM_initializeStateMachine( NULL, POD_HOMING_QUEUE, hmStateMachine, HM_INITIAL_STATE );


//    hmLCSMHandle = lcsm_open_device(); Hannah
}

void shutdownHomingStateMachine( void )
{
   // lcsm_close_device( hmLCSMHandle ); Hannah
}
//extern void HAL_SYS_GetPowerState( unsigned char *pbPoweredOn);

void vlHomingHandlePowerState(int PowerState)
{
//       unsigned char PowerState;
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","vlHomingHandlePowerState: ______ Entered ________ \n");
    //rmf_osal_threadSleep(500, 0);  
    //HAL_SYS_GetPowerState(&PowerState);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD",">>>>>>>>> PowerState:%d <<<<<<<<<<< \n",PowerState);
    if ( (PowerState == 0) && (standbyState != TRUE))
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","vlHomingHandlePowerState: Entered Stanby Mode \n");
            standbyState = TRUE;
            if ( vlg_HomingSessState/*CS_SM_getCurrentState(hmStateMachine) == HM_SESSION_OPENED*/ )
            {
                APDU_OpenHoming( gSessNum, NULL, 0 );
        //CS_SM_changeState( NULL, POD_HOMING_QUEUE, hmStateMachine, HM_OPEN );
            }
        }
        // leaving standby
        else if( (PowerState == 1) && (standbyState != FALSE))
        {
           RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","vlHomingHandlePowerState: Entered Exiting Stanby Mode \n");
            standbyState = FALSE;
            if ( vlg_HomingSessState/*CS_SM_getCurrentState(hmStateMachine) == HM_OPEN*/  )
            {
                if(vlg_CardFirmwareUpgradeStart == 0)
                    APDU_HomingCancelled( gSessNum, NULL, 0 );
                //CS_SM_changeState( NULL, POD_HOMING_QUEUE, hmStateMachine, HM_SESSION_OPENED );
            }
        }

}
void processHomingStateMachine( LCSM_EVENT_INFO *eventInfo )
{
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","processHomingStateMachine: Entered 0x%X\n",eventInfo->event);
    switch ( eventInfo->event )
    {
        case POD_CLOSE_SESSION:
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","processHomingStateMachine: POD_CLOSE_SESSION \n");
            Homing_CloseSession( (USHORT)(eventInfo->x) );
            CS_SM_changeState( NULL, POD_HOMING_QUEUE, hmStateMachine, HM_INITIAL_STATE );
            break;
        }

        case POD_STANDBY_NOTIFICATION:
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","processHomingStateMachine: STANDBY_NOTIFICATION \n");
            if(HomingCardFirmwareUpgradeGetState() == 1)
                break;
            // entering standby
            if ( eventInfo->x == 0 )
            {
                MDEBUG (DPM_HOMING, "*** HOMING: enter standby *** \n");
                standbyState = TRUE;
                if ( CS_SM_getCurrentState(hmStateMachine) == HM_SESSION_OPENED )
                {
                    APDU_OpenHoming( gSessNum, NULL, 0 );
                    CS_SM_changeState( NULL, POD_HOMING_QUEUE, hmStateMachine, HM_OPEN );
                }
            }
            // leaving standby
            else
            {
                MDEBUG (DPM_HOMING, "*** HOMING: exit standby *** \n");
                standbyState = FALSE;
                if ( CS_SM_getCurrentState(hmStateMachine) == HM_OPEN )
                {
                    APDU_HomingCancelled( gSessNum, NULL, 0 );
                    CS_SM_changeState( NULL, POD_HOMING_QUEUE, hmStateMachine, HM_SESSION_OPENED );
                }
            }

            break;
        }

        case POD_APDU:
        {
            unsigned char *ptr;
            ptr = (unsigned char *)eventInfo->y;    // y points to the complete APDU

            switch (*(ptr+2))
            {
                case POD_HOMING_OPENREPLY:
                {
                    //CS_SM_changeState( handle, queue, smControl, HM_OPEN );
                    APDU_HomingActive( gSessNum, NULL, 0 );
                    return;
                }
            }

      //      break;
       }

       default:
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","processHomingStateMachine: HOMING: default *** \n");
            MDEBUG (DPM_HOMING, "*** HOMING: default *** \n");
            CS_SM_processEvent( NULL, POD_HOMING_QUEUE, hmStateMachine, eventInfo );
    }
}


static void hmInitialState( void *handle, unsigned queue, CS_SM_CONTROL *smControl, LCSM_EVENT_INFO *eventInfo )
{

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","hmInitialState: Entered \n");
    switch( eventInfo->event )
    {
    case CS_SM_INIT:
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","hmInitialState: CS_SM_INIT \n");
        MDEBUG (DPM_HOMING, "Homing_SM Initial STATE HAS BEEN ENTERED \n");
        break;

    case CS_SM_TERM:
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","hmInitialState: CS_SM_TERM \n");
        MDEBUG (DPM_HOMING, "Homing_SM Initial STATE HAS BEEN TERMINATED \n");
        break;

    case POD_OPEN_SESSION_REQ:
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","hmInitialState: POD_OPEN_SESSION_REQ \n");
        Homing_OpenSessReq( (ULONG)(eventInfo->x), (UCHAR)(eventInfo->y) );
        CS_SM_changeState( handle, queue, smControl, HM_SESSION_OPENED );
        break;

    default:
    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","hmInitialState: default \n");
        MDEBUG (DPM_ERROR, "ERROR: event %d not handled \n", (unsigned)(eventInfo->event));
        break;

    }
}
#if 0
// POD sends APDU (x=sessNbr, y=APDU buffer ptr, z=APDU byte length)
            case POD_APDU:
                ptr = (char *)eventInfo->y;    // y points to the complete APDU

                switch (*(ptr+2)) {
                    // APDU is Application_Info_Cnf
                    case POD_APPINFO_CNF:
#endif

static void hmSessionOpened( void *handle, unsigned queue, CS_SM_CONTROL *smControl, LCSM_EVENT_INFO *eventInfo )
{
    unsigned char text_length;
    unsigned char *text_body;
    unsigned char *ptr;
    unsigned char upgrade_source;
    unsigned short download_time;
    unsigned short ui16temp;
    unsigned char timeout_type;
    unsigned short download_timeout_period;
    LCSM_EVENT_INFO eventSend,*pEventSend;
    int returnValue;

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","hmSessionOpened: Entered \n");
    switch( eventInfo->event )
    {
    case CS_SM_INIT:
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","hmSessionOpened: CS_SM_INIT \n");
        MDEBUG (DPM_HOMING, "Homing_SM SessionOpened STATE HAS BEEN ENTERED \n");
        break;

    case CS_SM_TERM:
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","hmSessionOpened: CS_SM_TERM \n");
        MDEBUG (DPM_HOMING, "Homing_SM SessionOpened STATE HAS BEEN TERMINATED \n");
         break;

    case POD_SESSION_OPENED:
    {
    //unsigned char PowerState;
        SYS_POWER_STATE PowerState = SYS_POWER_STATE_UNKNOWN;
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","hmSessionOpened: POD_SESSION_OPENED \n");
        Homing_SessOpened( (USHORT)(eventInfo->x), (ULONG)(eventInfo->y), (UCHAR)(eventInfo->z) );
        // if resource is opened and we're in standby, immediately send open homing
#ifdef USE_HAL_DIAG
#ifdef MERGE_INTEL
    HAL_SYS_GetPowerState(&PowerState);
#else
#endif
/* moved below code to here in to endif  */
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","hmSessionOpened: PowerState:%d \n",PowerState);
    if(PowerState == SYS_POWER_STATE_ON)
    {
        standbyState = FALSE;
    }
    else if(PowerState == SYS_POWER_STATE_STANDBY)
    {
        standbyState = TRUE;
    }

        if ( standbyState == TRUE )
        {
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," hmSessionOpened: Sending APDU_OpenHoming request \n");
            APDU_OpenHoming( gSessNum, NULL, 0 );
        }

#endif // USE_HAL_DIAG
        break;
    }
      // POD sends APDU (x=sessNbr, y=APDU buffer ptr, z=APDU byte length)
    case POD_APDU:
         ptr = (unsigned char *)eventInfo->y;    // y points to the complete APDU

    switch (*(ptr+2))
    {


              case POD_HOMING_OPENREPLY:
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","hmSessionOpened: POD_HOMING_OPENREPLY \n");
            // pod acknowledges HOMING_OPEN, go to Open state and respond with a HOMING_ACTIVE APDU
            //returnValue = APDU_OpenHomingReply( (USHORT)(eventInfo->x), (UCHAR*)(eventInfo->y), (USHORT)(eventInfo->z) );
            /*if ( returnValue == EXIT_FAILURE )
            {
                    break;
            }*/
            CS_SM_changeState( handle, queue, smControl, HM_OPEN );
            APDU_HomingActive( gSessNum, NULL, 0 );
            break;

             case POD_HOMING_FIRMUPGRADE:
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","hmSessionOpened: Immediate POD_HOMING_FIRMUPGRADE \n");
#ifdef NOTIFY_IARM_CARD_FWDWLD
		#ifdef USE_IARM_BUS
			 notifyIARMCardFWDNLDEvent(IARM_BUS_SYSMGR_CARD_FWDNLD_START);
		#else	
		     notifyIARMCardFWDNLDEvent(UIDEV_EVENT_CARD_FWDNLD,UIDEV_EVENT_CARD_FWDNLD_START);
		#endif
#endif
            returnValue = APDU_FirmUpgrade( (USHORT)(eventInfo->x), (UCHAR*)(eventInfo->y), (USHORT)(eventInfo->z) );
            if ( returnValue == EXIT_FAILURE )
            {
                    break;
            }
            if ( (UCHAR*)(eventInfo->y) == NULL )
            {
                    break;
            }

            ptr = (UCHAR *)(eventInfo->y);
            ptr += 3;  // get past tag
        #if (1)
            ptr += bGetTLVLength( ptr, NULL );  // get past variable sized length field
        #else /* Old way to compute len: Save for now (just in case!) Delete later */
            ptr += getLengthFieldSize( ptr );  // get past variable sized length field
        #endif

               // get upgrade source
            upgrade_source = *ptr;
            // get download_time
            ptr += 1;
            download_time = (unsigned short)( ( (*ptr) << 8) | *(ptr+1) );
            //ptr--;
            //ui16temp = *ptr << 8;
            //download_time |= ui16temp;
            // get timeout_type
            ptr += 2;
            timeout_type = *ptr;
            // get download timeout period
            ptr += 1;
            download_timeout_period = (unsigned short)( ( (*ptr) << 8 ) | *(ptr+1) );
            //ptr--;
        //    ui16temp = *ptr << 8;
        //    download_timeout_period |= ui16temp;
        ptr += 2;

        PrintFirmWareUpgradeParams(upgrade_source,download_time,timeout_type,download_timeout_period);


            // since we're not in standby, display user_notification text
      //      ptr += 2;  // go to text_length field
            text_length = *ptr;// + 1;  // +1 for NULL termination
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," text Length:%d \n",text_length);
            ptr++;
			rmf_osal_memAllocP(RMF_OSAL_MEM_POD, text_length + 1,(void **)&text_body);
//        memset(text_body, 0 ,text_length * sizeof(unsigned char)+ 1);
        memset(text_body, 0 ,text_length * + 1);

            //memcpy( (void*)text_body, (const void*)ptr, text_length );
        memcpy( (void*)text_body, (const void*)ptr, text_length );
            // send notification message to BB
            eventSend.event = POD_HOMING_USER_NOTIFY_MESS;
            eventSend.dataPtr = text_body;
            eventSend.dataLength = text_length;
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," text_body: %s \n",text_body);
        pEventSend = &eventSend ;
            gateway_send_event( hmLCSMHandle, pEventSend);
         PostBootEvent(RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD_FWDNLD, RMF_BOOTMSG_DIAG_MESG_CODE_START, 0);
        // when pod receives upgrade_reply it can start firmware upgrade
            APDU_FirmUpgradeReply( gSessNum, NULL, 0 );

            // set a timeout in download_timeout_period
            setTimeoutType( timeout_type, download_timeout_period, queue );


        HomingCardFirmwareUpgradeSetState(1);

        CS_SM_changeState( handle, queue, smControl, HM_FIRMWARE_UPGRADE );

            break;

             default:
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","hmSessionOpened: default ##1 \n");
            MDEBUG (DPM_ERROR, "ERROR:event %d not handled \n", (unsigned)(eventInfo->event));
            break;
     }//switch (*(ptr+2))
         break;
     default:
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","hmSessionOpened: default ##2 \n");
            MDEBUG (DPM_ERROR, "ERROR:event %d not handled \n", (unsigned)(eventInfo->event));
            break;
    }//switch
}

static void hmOpen( void *handle, unsigned queue, CS_SM_CONTROL *smControl, LCSM_EVENT_INFO *eventInfo )
{
    int returnValue;
    unsigned char *ptr;
    unsigned char upgrade_source;
    unsigned short download_time;
    unsigned char timeout_type;
    unsigned short download_timeout_period;
    unsigned short ui16temp;
    LCSM_EVENT_INFO eventSend;
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","hmOpen: Entered \n");
    switch( eventInfo->event )
    {
    case CS_SM_INIT:
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","hmOpen: CS_SM_INIT \n");
        MDEBUG (DPM_HOMING, "Homing_SM Open STATE HAS BEEN ENTERED \n");
        break;

    case CS_SM_TERM:
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","hmOpen: CS_SM_TERM \n");
        MDEBUG (DPM_HOMING, "Homing_SM Open STATE HAS BEEN TERMINATED \n");
        break;
    case POD_APDU:
        ptr = (unsigned char *)eventInfo->y;    // y points to the complete APDU

    switch (*(ptr+2))
    {
            case POD_HOMING_COMPLETE:
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","hmOpen: POD_HOMING_COMPLETE \n");
            // pod sends to us when it no longer needs the homing function
            returnValue = APDU_HomingComplete( (USHORT)(eventInfo->x), (UCHAR*)(eventInfo->y), (USHORT)(eventInfo->z) );
            if ( returnValue == EXIT_FAILURE )
             {
                break;
            }
        HomingCardFirmwareUpgradeSetState(0);
            CS_SM_changeState( handle, queue, smControl, HM_SESSION_OPENED );
            break;

             case POD_HOMING_FIRMUPGRADE:
                 RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","hmOpen: delayed firmware upgrade POD_HOMING_FIRMUPGRADE \n");
            returnValue = APDU_FirmUpgrade( (USHORT)(eventInfo->x), (UCHAR*)(eventInfo->y), (USHORT)(eventInfo->z) );
            if ( returnValue == EXIT_FAILURE )
                {
                break;
            }
            if ( (UCHAR*)(eventInfo->y) == NULL )
            {
                break;
            }

            ptr = (UCHAR *)(eventInfo->y);
            ptr += 3;  // get past tag
        #if (1)
            ptr += bGetTLVLength( ptr, NULL );  // get past variable sized length field
        #else /* Old way to compute len: Save for now (just in case!) Delete later */
            ptr += getLengthFieldSize( ptr );  // get past variable sized length field
        #endif

               // get upgrade source
            upgrade_source = *ptr;
            // get download_time
            ptr += 1;
            download_time = (unsigned short)( ( (*ptr) << 8) | *(ptr+1) );
            //ptr--;
            //ui16temp = *ptr << 8;
            //download_time |= ui16temp;
            // get timeout_type
            ptr += 2;
            timeout_type = *ptr;
            // get download timeout period
            ptr += 1;
            download_timeout_period = (unsigned short)( ( (*ptr) << 8 ) | *(ptr+1) );
            //ptr--;
        //    ui16temp = *ptr << 8;
        //    download_timeout_period |= ui16temp;
        ptr += 2;

        PrintFirmWareUpgradeParams ( upgrade_source, download_time, timeout_type, download_timeout_period ) ;

            // when pod receives upgrade_reply it can start firmware upgrade
            APDU_FirmUpgradeReply( gSessNum, NULL, 0 );
        // set a timeout in download_timeout_period
            setTimeoutType( timeout_type, download_timeout_period, queue );


        HomingCardFirmwareUpgradeSetState(1);
            CS_SM_changeState( handle, queue, smControl, HM_FIRMWARE_UPGRADE );
            break;

            default:
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","hmOpen: default ##1\n");
            MDEBUG (DPM_ERROR, "ERROR: event %d not handled \n", (unsigned)(eventInfo->event));
            break;
      }//swithc
      break;
    default:
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","hmOpen: default ##2\n");
            MDEBUG (DPM_ERROR, "ERROR: event %d not handled \n", (unsigned)(eventInfo->event));
            break;
    }
}


static void hmFirmwareUpgrade( void *handle, unsigned queue, CS_SM_CONTROL *smControl, LCSM_EVENT_INFO *eventInfo )
{
    unsigned char *ptr;
    unsigned char upgrade_source;
    unsigned short download_time;
    unsigned char timeout_type;
    unsigned short download_timeout_period;
    int returnValue;
    unsigned short ui16temp;
    LCSM_EVENT_INFO eventSend,*pEventSend;
    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    rmf_osal_event_handle_t event_handle;



    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","hmFirmwareUpgrade: Entered \n");

    pEventSend = &eventSend;
    switch( eventInfo->event )
    {
    case CS_SM_INIT:
    HomingCardFirmwareUpgradeSetState(1);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","hmFirmwareUpgrade: CS_SM_INIT \n");
        MDEBUG (DPM_HOMING, "Homing_SM FirmwareUpgrade STATE HAS BEEN ENTERED \n");
        // tell pod stack we are beginning a firmware upgrade
     //   CIHomingStateChange( HOMING_STARTED );
        // make it known that we are beginning a software download

        vlg_CardFirmwareUpgradeStart = 1;
        eventSend.event = POD_HOST_FIRMUPGRADE_START;
        eventSend.dataPtr = NULL;
        eventSend.dataLength = 0;
      //  lcsm_postEvent( hmLCSMHandle, &eventSend );
    lcsm_send_event(  hmLCSMHandle, POD_HOST_QUEUE, pEventSend);
// Report the firmware Upgrate start to the pfc Stack
    gateway_send_event( hmLCSMHandle, pEventSend );

        break;

    case CS_SM_TERM:
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","hmFirmwareUpgrade: CS_SM_TERM \n");
        // tell pod stack we are ending a firmware upgrade
     //   CIHomingStateChange( HOMING_ENDED );
        // make sure transport timeout is enabled when we leave this state
       //CITransTimeoutCtrl( ENABLE_TRANSPORT_TIMEOUT );
        // tell mrTopControl to leave the homing state
        eventSend.event = MR_CHANNEL_CTRL_IDLE;
        eventSend.dataPtr = NULL;
        eventSend.dataLength = 0;
        lcsm_send_event( hmLCSMHandle, SD_MR_CONTROL, pEventSend );

        // make it known that we are ending a software download
        eventSend.event = POD_HOST_FIRMUPGRADE_END;
        eventSend.dataPtr = NULL;
        eventSend.dataLength = 0;
        vlg_CardFirmwareUpgradeStart = 0;
       lcsm_send_event(  hmLCSMHandle, POD_HOST_QUEUE, pEventSend);
//Report the firmware Upgrate start to the pfc Stack
    gateway_send_event( hmLCSMHandle, pEventSend );
    HomingCardFirmwareUpgradeSetState(0);
        MDEBUG (DPM_HOMING, "Homing_SM FirmwareUpgrade STATE HAS BEEN TERMINATED \n");
        break;
 // POD sends APDU (x=sessNbr, y=APDU buffer ptr, z=APDU byte length)
    case POD_APDU:
         ptr = (unsigned char *)eventInfo->y;    // y points to the complete APDU

    switch (*(ptr+2))
    {

        case POD_HOMING_FIRMUPGRADE_COMPLETE:

            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","hmFirmwareUpgrade: POD_HOMING_FIRMUPGRADE_COMPLETE \n");
            returnValue = APDU_FirmUpgradeComplete( (USHORT)(eventInfo->x), (UCHAR*)(eventInfo->y), (USHORT)(eventInfo->z) );
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","hmFirmwareUpgrade: after APDU_FirmUpgradeComplete \n");
	    #ifdef NOTIFY_IARM_CARD_FWDWLD
            		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","hmFirmwareUpgrade: calling IARM_BUS_SYSMGR_CARD_FWDNLD_COMPLETE \n");
			#ifdef USE_IARM_BUS
            			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","hmFirmwareUpgrade: Before calling IARM_BUS_SYSMGR_CARD_FWDNLD_COMPLETE \n");
				notifyIARMCardFWDNLDEvent(IARM_BUS_SYSMGR_CARD_FWDNLD_COMPLETE);
            			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","hmFirmwareUpgrade: after calling IARM_BUS_SYSMGR_CARD_FWDNLD_COMPLETE \n");
			#else	
            			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","hmFirmwareUpgrade: Before calling UIDEV_EVENT_CARD_FWDNLD_COMPLETE \n");
				notifyIARMCardFWDNLDEvent(UIDEV_EVENT_CARD_FWDNLD,UIDEV_EVENT_CARD_FWDNLD_COMPLETE);
            			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","hmFirmwareUpgrade: after calling UIDEV_EVENT_CARD_FWDNLD_COMPLETE \n");
			#endif
	    #endif
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","hmFirmwareUpgrade: executed IARM_BUS_SYSMGR_CARD_FWDNLD_COMPLETE \n");

            if ( returnValue == EXIT_FAILURE )
            {
                    CS_SM_changeState( handle, queue, smControl, HM_INITIAL_STATE );
                    vlMpeosPostHomingCompleteEvent(returnValue);
                    break;
            }

            // clear any timeout events
            if ( downloadTimeout != (LCSM_TIMER_HANDLE)0 )
            {
                    HomingCancelTimedEvent( hmLCSMHandle, downloadTimeout );
                    downloadTimeout = (LCSM_TIMER_HANDLE)0;
            }

            ptr = (UCHAR *)(eventInfo->y);
            ptr += 3;  // get past tag
        #if (1)
            ptr += bGetTLVLength( ptr, NULL );  // get past variable sized length field
        #else /* Old way to compute len: Save for now (just in case!) Delete later */
            ptr += getLengthFieldSize( ptr );  // get past variable sized length field
        #endif

        rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_CARD_MANAGER, CardMgr_Card_Image_DL_Complete, 
    		NULL, &event_handle);
    
        rmf_osal_eventmanager_dispatch_event(em, event_handle);
         vlMpeosPostHomingCompleteEvent(returnValue);		
         PostBootEvent(RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD_FWDNLD, RMF_BOOTMSG_DIAG_MESG_CODE_COMPLETE, 0);
        HomingCardFirmwareUpgradeSetState(0);
            // PCMCIA reset
            if ( *ptr == 0x0 )
            {
        //#ifdef HAL_POD
        //Implement the HAL_POD_RESET_PCMCIA
                //CIResetPCMCIA();
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","hmFirmwareUpgrade: Initiating the PCMCIA Reset ....\n");
          CS_SM_changeState( handle, queue, smControl, HM_INITIAL_STATE );
          HomingCancelTimedEvent( hmLCSMHandle, downloadTimeout );
          int ret = HAL_POD_RESET_PCMCIA(0,0);
	   RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","hmFirmwareUpgrade: PCMCIA Reset done with retCode = %d\n",ret);          		  
        //#endif

            }
            // POD reset
            else if ( *ptr == 0x1 )
            {
        //#ifdef HAL_POD
        //This is only for S-Card...
        // We are not supporting S-card so just do PCMCIA reset..
            //CIResetPod();
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","hmFirmwareUpgrade: Initiating the POD Reset ....\n");

            CS_SM_changeState( handle, queue, smControl, HM_INITIAL_STATE );
            HomingCancelTimedEvent( hmLCSMHandle, downloadTimeout );
            HAL_POD_RESET_PCMCIA(0,0);
            //HAL_POD_INTERFACE_RESET(0,0);
        //#endif
                //CS_SM_changeState( handle, queue, smControl, HM_INITIAL_STATE );
            }
            // no reset required
            else if ( *ptr == 0x2 )
            {
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","hmFirmwareUpgrade: No Reset.......\n");
                    if ( standbyState == TRUE )
                    CS_SM_changeState( handle, queue, smControl, HM_OPEN );
                    else
                    CS_SM_changeState( handle, queue, smControl, HM_SESSION_OPENED );
            }

            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","hmFirmwareUpgrade: standbyState:%d \n",standbyState);
            // if we're not in standby, clear the user notification message
            //if ( standbyState == FALSE )
            {
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","hmFirmwareUpgrade: standbyState:%d is FALSE\n",standbyState);
                    eventSend.event = POD_HOMING_USER_END_MESS;
                    eventSend.dataPtr = NULL;
                    eventSend.dataLength = 0;
                    gateway_send_event( hmLCSMHandle, &eventSend );
            }
            break;

            // if additional FIRMUPGRADES are received we need to update the timeout
           case POD_HOMING_FIRMUPGRADE:

               RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","hmFirmwareUpgrade: additional POD_HOMING_FIRMUPGRADE \n");
            returnValue = APDU_FirmUpgrade( (USHORT)(eventInfo->x), (UCHAR*)(eventInfo->y), (USHORT)(eventInfo->z) );
            if ( returnValue == EXIT_FAILURE )
            {
                    break;
            }
            if ( (UCHAR*)(eventInfo->y) == NULL )
            {
                    break;
            }

            ptr = (UCHAR *)(eventInfo->y);
            ptr += 3;  // get past tag
        #if (1)
            ptr += bGetTLVLength( ptr, NULL );  // get past variable sized length field
        #else /* Old way to compute len: Save for now (just in case!) Delete later */
            ptr += getLengthFieldSize( ptr );  // get past variable sized length field
        #endif


               // get upgrade source
            upgrade_source = *ptr;
            // get download_time
            ptr += 1;
            download_time = (unsigned short)( ( (*ptr) << 8) | *(ptr+1) );
            //ptr--;
            //ui16temp = *ptr << 8;
            //download_time |= ui16temp;
            // get timeout_type
            ptr += 2;
            timeout_type = *ptr;
            // get download timeout period
            ptr += 1;
            download_timeout_period = (unsigned short)( ( (*ptr) << 8 ) | *(ptr+1) );
            //ptr--;
        //    ui16temp = *ptr << 8;
        //    download_timeout_period |= ui16temp;
        ptr += 2;

        PrintFirmWareUpgradeParams(upgrade_source,download_time,timeout_type,download_timeout_period);
            if ( downloadTimeout != (LCSM_TIMER_HANDLE)0 )
            {
                    HomingCancelTimedEvent( hmLCSMHandle, downloadTimeout );
                    downloadTimeout = (LCSM_TIMER_HANDLE)0;
            }


            APDU_FirmUpgradeReply( gSessNum, NULL, 0 );
        // set a timeout in download_timeout_period
            setTimeoutType( timeout_type, download_timeout_period, queue );

        HomingCardFirmwareUpgradeSetState(1);
            break;



            default:
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","hmFirmwareUpgrade: default ##1 \n");
            MDEBUG (DPM_ERROR, "event %d not handled \n", (unsigned)(eventInfo->event));
            break;
    }//switch (*(ptr+2))
    break;
     case POD_HOMING_DOWNLOAD_TIMEOUT:

        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","hmFirmwareUpgrade: POD_HOMING_DOWNLOAD_TIMEOUT \n");
        downloadTimeout = (LCSM_TIMER_HANDLE)0;
        vlg_TimeHandle = 0;
            CS_SM_changeState( handle, queue, smControl, HM_INITIAL_STATE );

        PostBootEvent(RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD_FWDNLD, RMF_BOOTMSG_DIAG_MESG_CODE_ERROR, 0);
        vlMpeosPostHomingCompleteEvent(0);
            // pcmcia reset at timeout
            //CIResetPCMCIA();
        //#ifdef HAL_POD
        //Implement the HAL_POD_RESET_PCMCIA
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","hmFirmwareUpgrade:>>>>>>>>>> Initiating PCMCIA Reset <<<<<<<<<<<<<< \n");
        HAL_POD_RESET_PCMCIA(0,0);
        HomingCardFirmwareUpgradeSetState(0);
        //#endif
            // if we're not in standby, clear the user notification message
            if ( standbyState == FALSE )
            {
                eventSend.event = POD_HOMING_USER_END_MESS;
                eventSend.dataPtr = NULL;
                eventSend.dataLength = 0;
                gateway_send_event( hmLCSMHandle, &eventSend );
            }


            break;
    default:
    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","hmFirmwareUpgrade: default ##2 \n");
        MDEBUG (DPM_ERROR, "event %d not handled \n", (unsigned)(eventInfo->event));
           break;
    }//switch
}



/*
 *
 *  helper functions
 *
 */

extern UCHAR   transId;
static void Homing_OpenSessReq( ULONG resId, UCHAR tcId )
{
    UCHAR status = 0;
    MDEBUG (DPM_HOMING, "resId=0x%x tcId=%d openSessions=%d\n",
            resId, tcId, homing_openSessions(1,0) );

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","Homing_OpenSessReq: Entered \n");
    if( homing_openSessions(1,0) == 0 )    // homing session not opened yet
    {
      /*  if( transId != tcId )        // make sure we are using the transId already opened earlier
        {
            status = 0xF0;  // non-existent resource (on this transId)
            MDEBUG (DPM_ERROR, "ERROR: transId in OPEN_SS=%d, exp=%d\n", tcId, transId );
            return;
        }

        if( RM_Get_Ver( HOMING_ID ) < RM_Get_Ver( resId ) )
        {
            status = 0xF2;  // this version < requested
            MDEBUG (DPM_ERROR, "ERROR: version=0x%x < req=0x%x\n",
                    RM_Get_Ver( HOMING_ID ), RM_Get_Ver( resId ) );
        } else*/
        {
            MDEBUG (DPM_HOMING, "opening homing session (homing_openSessions=%d)\n", homing_openSessions(1,0)+1 );
        }
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","Homing_OpenSessReq: opening homing session (homing_openSessions=%d)\n", homing_openSessions(1,0)+1 );
    }
    else
    {
        status = 0xF3;      // exists but busy
        MDEBUG (DPM_ERROR, "ERROR: max sessions opened=%d\n", homing_openSessions(1,0) );
    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","Homing_OpenSessReq: ERROR: max sessions opened=%d\n", homing_openSessions(1,0) );
    }

    AM_OPEN_SS_RSP( status, resId, transId );
}

static void Homing_SessOpened( USHORT sessNbr, ULONG resId, UCHAR tcId )
{
    int value;

    gSessNum = sessNbr;
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","Homing_SessOpened: Entered \n");
    value = homing_openSessions(1, 0);    // retrieve value first
    value++;
    homing_openSessions(0, value);        // write value back
vlg_HomingSessState = 1;
    MDEBUG (DPM_HOMING, "homing session is opened: sessNbr=%d\n", sessNbr );
}


static void Homing_CloseSession( USHORT sessNbr )
{
    int value;

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","Homing_CloseSession: Entered \n");
    value = homing_openSessions(1, 0);    // retrieve value first
    if (value > 0)
    {
        value--;
        homing_openSessions(0, value);    // Close the session
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","Homing_CloseSession: value > 0 \n");
        MDEBUG (DPM_HOMING, "homing sessNbr=%d closed (homing_openSessions=%d)\n", sessNbr, homing_openSessions(1,0) );
        vlg_HomingSessState = 0;
        if ( downloadTimeout != (LCSM_TIMER_HANDLE)0 )
             HomingCancelTimedEvent(vlg_TimeHandle, 0);

    }
    else
        MDEBUG (DPM_ERROR, "ERROR:trying to close not opened homing session - failed!\n");

    gSessNum = 0;
}

/*
 *    This function keeps the homing openSessions counter within itself,
 * thus making it safer, since it is not accessible directly in the global space
 *    mode=0: save   mode=1: retrieve
 */
static int homing_openSessions( int mode, int value )
{

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","homing_openSessions: Entered \n");
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


#if (0)
// length_field() can vary from 1-3 bytes
// if the most significant byte is 1, then the length is > 1 byte
// if the first byte is 129, then the length is 2 bytes
// if the first byte is 130, then the length is 3 bytes
// returns length in bytes
static unsigned getLengthFieldSize( unsigned char *length )
{
    if ( *length == 0x81 )
    {
        return 2;
    }
    if ( *length == 0x82 )
    {
        return 3;
    }
    return 1;
}
#endif

static void setTimeoutType( unsigned char timeout_type, unsigned short download_timeout_period, unsigned queue )
{
    LCSM_EVENT_INFO eventSend;
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","setTimeoutType: Entered timeout_type:%d \n",timeout_type);
    eventSend.event = POD_HOMING_DOWNLOAD_TIMEOUT;
    eventSend.dataPtr = NULL;
    eventSend.dataLength = 0;

    switch ( timeout_type )
    {
        // both timeouts
    case 0x0:

//Transport timeout needs to be implemented
//        CITransTimeoutCtrl( ENABLE_TRANSPORT_TIMEOUT );
        // download_timeout_period of 0 means infinite timeout
    // Wait for the 5 sec time

    if(download_timeout_period > 15)
    {
        HAL_POD_POLL_ON_SND(0,0);
    }
    //SendAPDUProfileInq();
        if ( download_timeout_period == 0 )
        {
//Need to Implement the c Timer thread to get the time out trigger...
// Use the download_timeout_period time for trigger...
       // if(download_timeout_period > 5)
        //download_timeout_period = 5;
            HAL_POD_POLL_ON_SND(0,0);
        }
    else
    {
         downloadTimeout = HomingGenTimedEvent( hmLCSMHandle, queue, &eventSend, download_timeout_period*1000,1/*number of times*/ );
    }
        break;

        // transport timeout only
    case 0x1:
//Transport timeout needs to be implemented
//        CITransTimeoutCtrl( ENABLE_TRANSPORT_TIMEOUT );
    // Wait for the 5 sec time
    //sleep(5);
    HAL_POD_POLL_ON_SND(0,0);
    //SendAPDUProfileInq();
    // downloadTimeout = HomingGenTimedEvent( hmLCSMHandle, queue, &eventSend, 5*1000,1/*number of times*/ );
        break;

        // download timeout only
    case 0x2:
//        CITransTimeoutCtrl( DISABLE_TRANSPORT_TIMEOUT );
        // download_timeout_period of 0 means infinite timeout
        if ( download_timeout_period != 0 )
        {
//Need to Implement the c Timer thread to get the time out trigger...
// Use the download_timeout_period time for trigger...

            downloadTimeout = HomingGenTimedEvent( hmLCSMHandle, queue, &eventSend, download_timeout_period*1000,1/*number of times*/ );
        }
        break;

        // no timeout
    case 0x3:
//Transport timeout needs to be implemented
//        CITransTimeoutCtrl( DISABLE_TRANSPORT_TIMEOUT );
        break;

    default:
        ;
    }

}

#ifdef __cplusplus
}
#endif

