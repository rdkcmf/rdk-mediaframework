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



#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#ifdef GCC4_XXX
#include <vector>
#else
#include <vector.h>
#endif
#include <podimpl.h>
#include "cardUtils.h"
#include "rmf_osal_event.h"
#include "core_events.h"
#include "cmevents.h"
#include "appinfo.h"
#include <string.h>
//#include "hal_api.h"
//#include "vlMpeosTunerImpl.h"
#include "cablecarddriver.h"
#include <string.h>

#include "poddriver.h"
#include "cardmanager.h"
#include "applitest.h"
#include "ip_types.h"
#include "rmf_osal_mem.h"
#include <coreUtilityApi.h>
#include "cardUtils.h"
#include "podlowapi.h"
#include "vlEnv.h"

#if USE_SYSRES_MLT
#include "rpl_new.h"
#include "rpl_malloc.h"
#endif

#define __MTAG__ VL_CARD_MANAGER


#define M_APP_INFO_CLS           0x02
#define M_MMI_CLS            0x40
#define M_SAS_CLS                       0x90

#ifdef __cplusplus
extern "C" {
#endif

#ifdef USE_DSG
    void vlXchanDispatchFlowData(unsigned long lFlowId, unsigned long nSize, unsigned char * pData);
#endif // USE_DSG

//void (* pSendEventCB)(unsigned queue, LCSM_EVENT_INFO *eventInfo);
 static int vlg_HomingCardFirmwareUpgradeInProgress = 0;

extern int AM_POD_EJECTED( void );

extern int hostAddPrtReq(unsigned char *pSnmpMess, unsigned long size);
extern int mmiSndApdu(unsigned long tag, unsigned short dataLen, unsigned char *data);
extern int aiSndApdu(unsigned long tag, unsigned long dataLen,  unsigned char *data);
// RAJASREE 11/08/07 [start] to send raw apdu from java app
extern int aiSndRawApdu( unsigned long tag, unsigned long dataLen,  unsigned char *data);
// RAJASREE 11/08/07 [end]
extern int SAS_APDU_FROM_HOST(   short sessNbr,unsigned long tag, unsigned char * pApdu, unsigned short len );
extern unsigned long ndstytpe;
extern  int cardMibAccSnmpReq(unsigned char *pSnmpMess, unsigned long size);
static unsigned char vlg_PodInserted = false;
//unsigned long DEFAULT_DEBUG_PRINT_MASK = DEFAULT_DEBUG_PRINT_MASK;
int POD_STACK_SEND_TPDU(void * stBufferHandle,  CHANNEL_TYPE_T data_type);
int POD_STACK_SEND_ExtCh_Data(unsigned char * data,  unsigned long length, unsigned long flowID);

 int lcsm_flush_queue(unsigned queue);
 unsigned lcsm_send_event(LCSM_DEVICE_HANDLE handle, unsigned queue, LCSM_EVENT_INFO *eventInfo);
 unsigned lcsm_send_immediate_event(LCSM_DEVICE_HANDLE handle,unsigned queue, LCSM_EVENT_INFO *eventInfo);
 LCSM_TIMER_HANDLE lcsm_send_timed_event(LCSM_DEVICE_HANDLE  handle, unsigned queue,   LCSM_EVENT_INFO *eventInfo);
 LCSM_REGISTER_EVENT_RETURN_STATUS lcsm_postEvent(LCSM_DEVICE_HANDLE handle,LCSM_EVENT_INFO *eventInfo)
 ;
 LCSM_TIMER_STATUS lcsm_cancel_timed_event( LCSM_DEVICE_HANDLE  handle,   LCSM_TIMER_HANDLE timerHandle);
 LCSM_TIMER_STATUS lcsm_reschedule_timed_event( LCSM_DEVICE_HANDLE  handle,   LCSM_TIMER_HANDLE timerHandle, unsigned milliseconds);
int HAL_POD_STOP_CONFIGURE_CIPHER(unsigned char ltsid,unsigned short PrgNum,unsigned long *decodePid,unsigned long numpids);
 int vlClearCaResAllocatedForPrgIndex(unsigned char PrgIndex);
#ifdef __cplusplus
}
#endif

static int vlg_bUnitTestFlagDisablePodReadWrite = 0;
FLUX_TIMER fluxTimer = {
    0,   // nFirst
    0,   // nLast
    120, // tTimeThreshold
    0,   // nCount
    6    // nCountThreshold
};

unsigned char GetPodInsertedStatus()
{
//    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","<************************ vlg_PodInserted = %d\n", vlg_PodInserted);
    return vlg_PodInserted;
}

void SetPodInsertedStatus(unsigned char flag)
{
    vlg_PodInserted = flag;
}
int POD_STACK_SEND_TPDU(void * stBufferHandle,  CHANNEL_TYPE_T data_type)
{

    return cPOD_driver::POD_STACK_SEND_TPDU( stBufferHandle);
}

int POD_STACK_SEND_ExtCh_Data(unsigned char * data,  unsigned long length, unsigned long flowID)
{
    if(vlg_HomingCardFirmwareUpgradeInProgress == 1)
        return -1;

    return cPOD_driver::POD_STACK_SEND_ExtCh_Data( data,  length, flowID);
}




int lcsm_flush_queue(unsigned queue)
{
    cPOD_driver:: lcsm_flush_queue(queue);
    return 0;
}

unsigned lcsm_send_event(LCSM_DEVICE_HANDLE handle, unsigned queue, LCSM_EVENT_INFO *eventInfo)
{
    cPOD_driver:: lcsm_send_event(  handle,   queue,  eventInfo);
    return 0;
}

unsigned lcsm_send_immediate_event(LCSM_DEVICE_HANDLE handle,unsigned queue, LCSM_EVENT_INFO *eventInfo)
{
    cPOD_driver:: lcsm_send_immediate_event( handle,   queue,  eventInfo);
    return 0;
}

LCSM_TIMER_HANDLE lcsm_send_timed_event(LCSM_DEVICE_HANDLE  handle, unsigned queue,   LCSM_EVENT_INFO *eventInfo)
{
    cPOD_driver:: lcsm_send_event(  handle,   queue,  eventInfo);
    return 0;
}

LCSM_REGISTER_EVENT_RETURN_STATUS lcsm_postEvent(LCSM_DEVICE_HANDLE handle,LCSM_EVENT_INFO *eventInfo)
{
    cPOD_driver:: lcsm_postEvent(handle, eventInfo);
    return LCSM_REGISTER_EVENT_SUCCESS;
}

LCSM_TIMER_STATUS lcsm_cancel_timed_event( LCSM_DEVICE_HANDLE  handle,   LCSM_TIMER_HANDLE timerHandle)
{
    cPOD_driver:: lcsm_cancel_timed_event(handle, timerHandle);
    return LCSM_TIMER_OP_OK;
}

LCSM_TIMER_STATUS lcsm_reschedule_timed_event( LCSM_DEVICE_HANDLE  handle,   LCSM_TIMER_HANDLE timerHandle, unsigned milliseconds)
{
    cPOD_driver:: lcsm_reschedule_timed_event(handle, timerHandle ,milliseconds);
    return LCSM_TIMER_OP_OK;
}






//following APDU tags are for the Raw APDU handling by OCAP
int supportedAPDUTagArray[] = {APP_INFO_REQ_TAG,SERVER_QUERY_TAG, MMI_CLOSE_REQ_TAG,MMI_OPEN_CONF_TAG,MMI_CLOSE_CONF_TAG,
                 SAS_CONNECT_REQ_TAG,SAS_DATA_REQ_TAG,SAS_DATA_AV_TAG,SAS_DATA_CONF_TAG,SAS_DATA_QUERY_TAG,SAS_DATA_REPLY_TAG, SAS_ASYNC_MSG_TAG};



cPOD_driver::cPOD_driver(CardManager *cm)
{

    this->cm = cm;
this->hPODHandle = 0;
this->cablecard_device = 0;
}


cPOD_driver::cPOD_driver()
{
this->cm = NULL;
this->hPODHandle = 0;
this->cablecard_device = 0;
}

cPOD_driver::~cPOD_driver()
{

}
int cPOD_driver::CardManagerSendMsgEvent( char *pMessage, unsigned long length)
{
    LCSM_EVENT_INFO *pEvent=NULL;
    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    LCSM_EVENT_INFO eventInfo;
    char *pData;
    rmf_osal_event_handle_t event_handle;
    rmf_osal_event_params_t event_params = {0};
    memset(&eventInfo, 0, sizeof(eventInfo));
    if(pMessage == NULL || length == 0)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","cPOD_driver::CardManagerSendMsgEvent: Error !!! returning pMessage == NULL || length == 0 \n");
        return -1;
    }
	rmf_osal_memAllocP(RMF_OSAL_MEM_POD, length + 1,(void **)&pData);
    if(pData == NULL)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","cPOD_driver::CardManagerSendMsgEvent: Error !!! rmf_osal_memAllocP failed returned NULL \n");
        return -1;
    }
    memset(pData,0,length + 1);
    memcpy(pData,pMessage,length);

    eventInfo.event = VL_CDL_DISP_MESSAGE;
    eventInfo.dataPtr = pData;
        eventInfo.dataLength = length;
	
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","cPOD_driver::CardManagerSendMsgEvent: Posting the meesage:%s \n",eventInfo.dataPtr);

    rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(LCSM_EVENT_INFO),(void **)&pEvent);

    if(pEvent == NULL)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","cPOD_driver::CardManagerSendMsgEvent: Error !!! pEvent is  NULL !! \n");
        return -1;
    }
    memcpy(pEvent, &eventInfo, sizeof (LCSM_EVENT_INFO));
    //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," before dispatch((pfcEventCableCard *)em->new_event(pEvent) \n");

    event_params.priority = 0; //Default priority;
    event_params.data = (void *)pEvent;
    event_params.data_extension = 0;
    event_params.free_payload_fn = podmgr_freefn;
    rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_GATEWAY, VL_CDL_DISP_MESSAGE, 
		&event_params, &event_handle);

    rmf_osal_eventmanager_dispatch_event(em, event_handle);

    return 0;

}

void cPOD_driver::SendEventCallBack( unsigned queue,LCSM_EVENT_INFO *eventInfo){  // use pfc dispatch event mechanism as the event source is from POD stack.

    rmf_osal_event_category_t category;
    LCSM_EVENT_INFO *pEvent=NULL;
    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    rmf_osal_event_handle_t event_handle;
    rmf_osal_event_params_t event_params = {0};
	
    if (eventInfo == NULL)
        return;
    //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","cPOD_driver::SendEventCallBack,, Entered \n");

    //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," before ######### pEvent->GetCategory(queue) ######### queue:0x%X pEvent:0x%X\n",queue,pEvent);
    category = GetCategory(queue);

    //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," after ######### pEvent->GetCategory(queue) ######### category:0x%X\n pEvent:%X \n",category,pEvent);
    rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(LCSM_EVENT_INFO),(void **)&pEvent);

    memcpy(pEvent ,eventInfo, sizeof (LCSM_EVENT_INFO));


    event_params.priority = 0; //Default priority;
    event_params.data = (void *)pEvent;
    event_params.data_extension = 0;
    event_params.free_payload_fn = podmgr_freefn;


/*    memcpy(&pEvent->eventInfo ,eventInfo, sizeof (LCSM_EVENT_INFO));*/
    //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," before dispatch((pfcEventCableCard *)em->new_event(pEvent) \n");
    rmf_osal_event_create( category, eventInfo->event, 
		&event_params, &event_handle);

    rmf_osal_eventmanager_dispatch_event(em, event_handle);
    //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," after dispatch((pfcEventCableCard *)em->new_event(pEvent) \n");

}


#define    EXIT_REQUEST                0xff
void cPOD_driver::POD_CardDetection_cb(    DEVICE_HANDLE_t    hDevicehandle,
                                        unsigned long        cardType,
                                        unsigned char        eStatus,
                                        const void            *pData,
                                        unsigned long        ulDataLength,
                                        void                *pulDataRcvd
                                    )
{
    LCSM_EVENT_INFO eventInfo;
    LCSM_EVENT_INFO *pEvent;
    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    rmf_osal_event_handle_t event_handle;
    rmf_osal_event_params_t event_params = {0};	

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","CardManager: POD_CardDetection_cb = %x\n",eStatus);

    eventInfo.event = POD_ERROR; //Initialize event with POD_ERROR status

    ndstytpe = 0;//FALSE;
    if(eStatus == POD_CARD_INSERTED)
    {
        eventInfo.event = POD_INSERTED;
        vlg_PodInserted = true;
        vlg_HomingCardFirmwareUpgradeInProgress = 0;
        //AM_INIT();    // done in POD_STACK_INIT
        if(ulDataLength == 1)
        {
            if(*(unsigned char *)pData == NDS_POD )
            {
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","NDS POD detected by POD_STACK..%x\n", *(unsigned char *)pData);
                ndstytpe =  1;//TRUE;

            }
        }

        if(cardType == POD_MODE_SCARD)
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","POD_CARD_INSERTED. card Type is S-CARD\n");
             trans_CreateTC(1);
        }
        else
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","POD_CARD_INSERTED. card Type is M-CARD\n");
        }


        vl_gCardType = cardType;

        rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(LCSM_EVENT_INFO),(void **)&pEvent);
        pEvent->event = EXIT_REQUEST;
        pEvent->x     = 0;
        pEvent->y     = 0;
        pEvent->z     = 0;
        pEvent->dataPtr = NULL;
        pEvent->dataLength = 0;

        event_params.priority = 0; //Default priority;
        event_params.data = (void *)pEvent;
        event_params.data_extension = 0;
        event_params.free_payload_fn = podmgr_freefn;
        rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_RS_MGR, EXIT_REQUEST, 
    		&event_params, &event_handle);
    
        rmf_osal_eventmanager_dispatch_event(em, event_handle);

    }
    else if(eStatus == POD_CARD_REMOVED)
    {
        vlg_HomingCardFirmwareUpgradeInProgress = 0;
        vlg_PodInserted = false;
        eventInfo.event = POD_REMOVED;
        //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","before AM_POD_EJECTED\n");
        AM_POD_EJECTED();
        //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","after AM_POD_EJECTED\n");
        PostBootEvent(RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD, RMF_BOOTMSG_DIAG_MESG_CODE_ERROR, 0);
    }
    else if(eStatus == POD_CARD_ERROR)
    {
        vlg_PodInserted = false;
        eventInfo.event = POD_BAD_MOD_INSERT_EROR;
        vlg_HomingCardFirmwareUpgradeInProgress = 0;
    }

    eventInfo.dataPtr = NULL;
    eventInfo.dataLength = 0;
    SendEventCallBack( POD_MSG_QUEUE, &eventInfo);
}




void cPOD_driver::POD_Send_Poll_cb(DEVICE_HANDLE_t      hDevicehandle, const void         *pData){

    trans_Poll();

}



void cPOD_driver::POD_Data_Available_cb(DEVICE_HANDLE_t      hDevicehandle,  T_TC *pTPDU, const void *pData)
{
    trans_ProcessTPDU (pTPDU);
}


void cPOD_driver::POD_ExtCh_Data_Available_cb(DEVICE_HANDLE_t  hDevicehandle, ULONG lFlowId, const void  *pData, USHORT wSize)
{
#ifdef USE_DSG
    vlXchanDispatchFlowData(lFlowId, wSize, (unsigned char *)pData);
#else
    AM_EXTENDED_DATA(lFlowId,(unsigned char *)pData,wSize);
#endif
}


int cPOD_driver::initialize()
{
    int status=-1;


     AM_INIT();
    cablecard_device = get_cablecard_device();//pfc_kernel_global->get_cablecard_device();
    vlg_bUnitTestFlagDisablePodReadWrite = vl_env_get_bool(false, "UNIT_TEST_FLAG.DISABLE_POD_READ_WRITE");
    time(&fluxTimer.tLast);
    fluxTimer.tFirst = fluxTimer.tLast;

     if(cablecard_device)
    {

         hPODHandle = cablecard_device->open_device(CABLECARD_0);
        //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Number of cablecard devices = %d\n",cablecard_device->get_device_count());
        if(hPODHandle)
        {
            //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","cablecard_device=%x\n",cablecard_device);


             this->podStartResources(hPODHandle);

             this->cm->StartPODThreads(this->cm);

            cablecard_device->register_callback(NOTIFY_CARD_DETECTION_CB, (void *)POD_CardDetection_cb, CABLECARD_0);
            cablecard_device->register_callback(NOTIFY_POD_POLL_CMD_CB,  (void *)POD_Send_Poll_cb , CABLECARD_0);
            if(vlg_bUnitTestFlagDisablePodReadWrite == 0)
            {
                cablecard_device->register_callback(NOTIFY_DATA_AVAILABLE_CB,  (void *)POD_Data_Available_cb , CABLECARD_0);
                cablecard_device->register_callback(NOTIFY_EXTCH_DATA_AVAILABLE_CB,  (void *)POD_ExtCh_Data_Available_cb , CABLECARD_0);
            }

            // last api to be called
            cablecard_device->if_control(CABLECARD_0, VL_POD_IF_COMMAND_INIT_POD, NULL);
            status = 0;
        }
        else
            status = -1;
     }

    return status;
}






int cPOD_driver::POD_STACK_SEND_TPDU(void * stBufferHandle)
{
cableCardDriver *cablecard_device;
    cablecard_device = get_cablecard_device();//pfc_kernel_global->get_cablecard_device();

     if(cablecard_device)
    {

        cablecard_device->send_data( stBufferHandle ,  DATA_CHANNEL, CABLECARD_0);
    }
    return 0;
}

extern "C" unsigned char * PODMemAllocate(USHORT size);

int cPOD_driver::POD_STACK_SEND_ExtCh_Data(unsigned char * data,  unsigned long length, unsigned long flowID)
{
    //static POD_BUFFER stBufferHandle;
    POD_BUFFER stBufferHandle;
    cableCardDriver *cablecard_device;
    cablecard_device = get_cablecard_device();//pfc_kernel_global->get_cablecard_device();

    stBufferHandle.lSize = length;
    stBufferHandle.pOrigin = vlCardManager_PodMemAllocate(length);
    stBufferHandle.pData = stBufferHandle.pOrigin;
    memcpy(stBufferHandle.pOrigin, data, length);
    stBufferHandle.FlowID = flowID;
    if(cablecard_device)
    {
        cablecard_device->send_data( static_cast<void *>(&stBufferHandle), EXTENDED_DATA_CHANNEL, CABLECARD_0);
    }
    return 0;
}


void cPOD_driver::ConnectSourceToPOD(unsigned long tuner_handle, unsigned long videoPid)
{

    cableCardDriver* cablecard_device = get_cablecard_device();//pfc_kernel_global->get_cablecard_device();
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","cPOD_driver::ConnectSourceToPOD cablecard_device = %p\n", (void *)cablecard_device);
    if(cablecard_device)
    {
        cablecard_device->ConnectSourceToPOD(tuner_handle,CABLECARD_0,videoPid);
    }
}

extern "C" int cablecard_device_oob_control(unsigned long eCommand, void * pData)
{
    return cPOD_driver::oob_control(eCommand, pData);
}

int    cPOD_driver::oob_control(unsigned long eCommand, void * pData)
{

    cableCardDriver* cablecard_device = get_cablecard_device();//pfc_kernel_global->get_cablecard_device();
    //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","cPOD_driver::oob_control cablecard_device = %x\n", cablecard_device);
    if(cablecard_device)
    {
        return cablecard_device->oob_control(CABLECARD_0,eCommand, pData);
    }

    return -1;
}

extern "C" int cablecard_device_if_control(unsigned long eCommand, void * pData)
{
    return cPOD_driver::if_control(eCommand, pData);
}

int    cPOD_driver::if_control(unsigned long eCommand, void * pData)
{

    cableCardDriver* cablecard_device = get_cablecard_device();//pfc_kernel_global->get_cablecard_device();
    //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","cPOD_driver::if_control cablecard_device = %x\n", cablecard_device);
    if(cablecard_device)
    {
        return cablecard_device->if_control(CABLECARD_0,eCommand, pData);
    }

    return -1;
}

bool cPOD_driver::IsPOD_NDS()
{

    return  ndstytpe;

}


int cPOD_driver::lcsm_flush_queue( unsigned queue )
{
//  LCSM_EVENT_INFO eventInfo;


  return 0;
}

//unsigned lcsm_get_event( unsigned queue, LCSM_EVENT_INFO *eventInfo, int waitMilliseconds )
//{

//  return LCSM_REGISTER_EVENT_SUCCESS;
//}

/*unsigned lcsm_send_event( unsigned queue, LCSM_EVENT_INFO *eventInfo )
{

      eventInfo->returnStatus = -1; // bad queue
    }
    eventInfo->returnStatus = 0;
  return 0;
}
*/
unsigned cPOD_driver::lcsm_send_event( LCSM_DEVICE_HANDLE handle, unsigned queue, LCSM_EVENT_INFO *eventInfo )
{ // used from within POD stack to send events


    //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","lcsm_send_event queue=%d\n",queue);
    SendEventCallBack(queue, eventInfo);
    return LCSM_REGISTER_EVENT_SUCCESS;
}


unsigned cPOD_driver::lcsm_send_immediate_event(LCSM_DEVICE_HANDLE handle,unsigned queue, LCSM_EVENT_INFO *eventInfo )
{

        //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","lcsm_send_immediate_event queue=%d\n",queue);
    SendEventCallBack(queue, eventInfo);
    return LCSM_REGISTER_EVENT_SUCCESS;
    return 0;
}

LCSM_REGISTER_EVENT_RETURN_STATUS cPOD_driver::lcsm_postEvent( LCSM_DEVICE_HANDLE handle,LCSM_EVENT_INFO *eventInfo ){

//unsigned queue=;
    //SendEventCallBack(queue, eventInfo);
     return LCSM_REGISTER_EVENT_SUCCESS;

}

LCSM_TIMER_HANDLE cPOD_driver::lcsm_send_timed_event(    LCSM_DEVICE_HANDLE  handle, unsigned queue,   LCSM_EVENT_INFO *eventInfo )
{

//RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","lcsm_send_event queue=%d\n",queue);
    SendEventCallBack(queue, eventInfo);
    return LCSM_REGISTER_EVENT_SUCCESS;
/*  LCSM_TIMER_CELL   *x;
  LCSM_TIMER_HANDLE returnValue;


  x = getFreeTimerCell();
  if( x != NULL )
    {
      init_timer( &x->timer );
      x->event = *eventInfo;
      x->event.DstQueue = queue;
      x->timer.expires = jiffies + (milliseconds * HZ )/1000;
      x->timer.data     = (unsigned long )x;
      x->timer.function = timerFunction;
      add_timer( &x->timer );
      returnValue = x->timerHandle;
    }
  else
    {
      returnValue = LCSM_NO_TIMERS;
    }
  return returnValue;
*/
}




LCSM_TIMER_STATUS cPOD_driver::lcsm_cancel_timed_event(  LCSM_DEVICE_HANDLE  handle,   LCSM_TIMER_HANDLE timerHandle )
{
 /* LCSM_TIMER_CELL  *timerCell;
  LCSM_TIMER_STATUS returnValue;

  timerCell = getTimer( handle );
  if( timerCell != NULL )
    {
      if( timerCell->active > 0 )
    {
      del_timer_sync( &timerCell->timer );
      if( timerCell->event.dataPtr != NULL )
        {
          kfree( timerCell->event.dataPtr );
        }
          returnTimerCell( timerCell );
      returnValue = LCSM_TIMER_OP_OK;
    }
      else
    {
      returnValue = LCSM_TIMER_EXPIRED;
    }
    }
  else
    {
      returnValue = LCSM_NO_TIMERS;
    }
  return returnValue;
*/
    return LCSM_TIMER_OP_OK;
}

LCSM_TIMER_STATUS cPOD_driver::lcsm_reschedule_timed_event(   LCSM_DEVICE_HANDLE  handle,   LCSM_TIMER_HANDLE timerHandle, unsigned milliseconds )
{
/*
  LCSM_TIMER_CELL  *timerCell;
  LCSM_TIMER_STATUS returnValue;

  timerCell = getTimer( handle );
  if( timerCell != NULL )
    {
      if( timerCell->active > 0 )
    {

      mod_timer( &timerCell->timer, jiffies + ( newTimerUpdate * HZ )/1000 );
      returnValue = LCSM_TIMER_OP_OK;
    }
      else
    {
      returnValue = LCSM_TIMER_EXPIRED;
    }
    }
  else
    {
      returnValue = LCSM_NO_TIMERS;
    }
  return returnValue;
*/
    return LCSM_TIMER_OP_OK;
}



int cPOD_driver::podStartResources(LCSM_DEVICE_HANDLE h)
{
    int retval;

    retval = rsmgr_start(h);

    retval = appinfo_start(h);

    retval = mmi_start(h);

    retval = homing_start(h);

    retval = xchan_start(h);

    retval = systime_start(h);

    retval = host_start(h);

    retval = ca_start(h);


    retval = cardres_start(h);
retval = cardMibAcc_start(h);
    retval = hostAddPrt_start(h);    
    retval = headEndComm_start(h);
#ifdef USE_DSG
    retval = dsg_start(h);
#endif

#ifdef USE_IPDIRECT_UNICAST    
    retval = ipdirect_unicast_start(h);
#endif // USE_IPDIRECT_UNICAST    
    
    retval = cprot_start(h);

    retval = sas_start(h);

    retval = system_start(h);

    retval = feature_start(h);

    retval = diag_start(h);


    return retval;
}

int cPOD_driver::podStopResources()
{
    int retval;

    retval = appinfo_stop();
    retval = mmi_stop();
 //   retval = homing_stop();
    retval = xchan_stop();
    retval = systime_stop();
    retval = host_stop();
    retval = ca_stop();
//    retval = cprot_stop();
    retval = sas_stop();
    retval = system_stop();
    retval = feature_stop();
    retval = diag_stop();
    retval = rsmgr_stop();      // needs to be shutdown last ?
retval = headEndComm_stop();
retval = hostAddPrt_stop();
    return retval;
}


void    cPOD_driver::POD_Send_app_info_request(pod_appInfo_t    *pInfo){


 LCSM_EVENT_INFO  msg;
 pod_appInfo_t    *pPOD_appinfo;

     rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof (pod_appInfo_t), (void **)&pPOD_appinfo);

/*    memcpy(pPOD_appinfo,pInfo, sizeof (pod_appInfo_t));*/
    memcpy(pPOD_appinfo,pInfo, sizeof (pod_appInfo_t));
//        memset( &msg, 0, sizeof(msg) );
            memset( &msg, 0, sizeof(msg));
                msg.event      = POD_OPEN;
                msg.dataPtr    = pPOD_appinfo;//pInfo;
                msg.dataLength = sizeof(pod_appInfo_t);   //        sizeof( *pInfo );


    SendEventCallBack(POD_APPINFO_QUEUE, &msg);

}



unsigned cPOD_driver::nvmm_rawRead( unsigned nvRamOffset, unsigned length, unsigned char *localBuffer ){

//implement
    return 0;
}

unsigned cPOD_driver::nvmm_rawWrite( unsigned nvRamOffset, unsigned length, unsigned char *localBuffer ){

    return 0;
}
int cPOD_driver::PODSendCardMibAccSnmpReqAPDU( unsigned char *Message, unsigned long MsgLength)
{
    return cardMibAccSnmpReq(Message,MsgLength);
}
extern "C" int caSndApdu(unsigned short sesnum, unsigned long tag, unsigned short dataLen, unsigned char *data);
extern "C" int vlCPCaDescdata(unsigned char *pCaPmt,unsigned long CaPmtLen,void *pStrPointer/*, unsigned short numPids, int *pPids*/);

typedef struct _sCAElmPidInfo
{
  unsigned char     pidType;
  unsigned short    pid;
}stElmPidInfo;

#define IS_VIDEO_PID(type)  ((type == 0x1) \
                             || (type == 0x2)  \
                             || (type == 0x80) \
                             || (type == 0x1B))


static int vlCardStopConfigureCipher(unsigned char *pCaPmt,unsigned long CaPmtLen)
{
    #define MAX_NUM_OF_ELM_PIDS 10 
    unsigned char ltsid,*pElmDes;
    unsigned short  progNum,caDescLen;
    short EsInfoLen;
    int iRet,ii,len,DescIndex,indx=0,pidsindx=0;
    unsigned char PrgIndex;
    unsigned long decodePid[MAX_NUM_OF_ELM_PIDS], numpids;
    stElmPidInfo  ESPidInfo[MAX_NUM_OF_ELM_PIDS];
    

    memset(decodePid, 0, sizeof(unsigned long) * MAX_NUM_OF_ELM_PIDS);
    
    ltsid =  pCaPmt[2];
    progNum = (pCaPmt[3]<< 8 )| pCaPmt[4];
    
    
    caDescLen = (pCaPmt[8]<< 8 )| pCaPmt[9];


    if(CaPmtLen < (10 + caDescLen) )
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlCardStopConfigureCipher: Error in CaPmtLen:%d \n",CaPmtLen);
        return -1;
    }

    len = (int)(CaPmtLen - (10 + caDescLen));

    if(len == 0)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlCardStopConfigureCipher: Error##1 in len:%d \n",len);
        return -1;
    }
//Looking for elementary streams
//FIX for MOT7425-794. The length check is not required here as there could be less than 5 bytes if the ES_info_length is 0.
#if 0
// min number of expected bytes for Elm CA descriptor should be > 5 bytes(strmtype(1)+elmpid(2)+EsInfolen(2) ...from this point
    if(len <= 5)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlCardStopConfigureCipher: Error##2 in len:%d \n",len);
        //DBG_PRINT("vlCardStopConfigureCipher: Error##2 in len:%d \n",len);
        return -1;
    }
#endif

    pElmDes = &pCaPmt[(10 + caDescLen)];
    while(len >= 3)
    {
        if(pidsindx >= MAX_NUM_OF_ELM_PIDS )
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlCardStopConfigureCipher: ERROR: Num Of Elm Pids more than :%d\n",MAX_NUM_OF_ELM_PIDS);
            return -1;
        }
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," Elm Stream Type : 0x%X \n",pElmDes[0]);
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," Elm pid : 0x%X \n",(pElmDes[1] << 8 )| pElmDes[2]);


        //decodePid[pidsindx]= ((pElmDes[1] << 8 )| pElmDes[2])&0x1FFF;
        ESPidInfo[pidsindx].pidType = pElmDes[0];
        ESPidInfo[pidsindx].pid = ((pElmDes[1] << 8 )| pElmDes[2])&0x1FFF;

        pidsindx++;
        len = len - 3;

        if(len < 2)
            break;
        EsInfoLen = (pElmDes[3] << 8 )| pElmDes[4];
        len = len - 2;
        pElmDes = pElmDes + 5;
        if(EsInfoLen == 0)
            continue;

        // Now the length should be >= 6 ( tag(1)+len(1)+ CA_Systemid(2) + CA pid(2) )

        len = len - EsInfoLen;
        pElmDes = pElmDes + EsInfoLen;
    }
        
    numpids = pidsindx;

    int i, vidPidIndex = 0;
    int pidIndex=0;
    bool isVideoPid = false;

    for(i = 0; i < numpids; i ++)
    {
      if(IS_VIDEO_PID(ESPidInfo[i].pidType))
      {
        decodePid[0] = ESPidInfo[i].pid;
        vidPidIndex = i;
        isVideoPid = true;
        pidIndex =1;
        break;
      }
    }

    for(i = 0; i < numpids; i ++)
    {
       if((i == vidPidIndex) && (isVideoPid))
      {
        continue;
      }
      decodePid[pidIndex] = ESPidInfo[i].pid;
      pidIndex +=1;
    }

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","calling HAL_POD_STOP_CONFIGURE_CIPHER with ltsid:%d,progNum:%d,decodePid[0]:0x%X,numpids:%d\n",ltsid,progNum,decodePid[0],numpids);
    HAL_POD_STOP_CONFIGURE_CIPHER(ltsid,progNum,decodePid,numpids);

    return 0;
}

int cPOD_driver::PODSendCaPmtAPDU( short sesnId, unsigned long tag, unsigned char *apdu,
                                  unsigned short apduLength )
{
    unsigned short length, numLenBytes;
    unsigned char *pTempBuf;
    int iRet = -1;

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","cPOD_driver::PODSendCaPmtAPDU: Entered with Tag:%X\n",tag);
    if( (apdu == NULL) || (apduLength < 10) )
    {
      return iRet;
    }

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD",
            "cPOD_driver::PODSendCaPmtAPDU: Sending CA PMT  Dumping data\n");

    vlMpeosDumpBuffer(RDK_LOG_DEBUG, "LOG.RDK.POD", apdu, apduLength);

    //Remove the 3 bytes TAG and advance the apdu to the length field.
    apduLength -= 3;
    apdu += 3;

    //Check the length field
    pTempBuf = apdu;

    if (!(*pTempBuf & 0x80))                    // if MSB = 0, 1 byte of length
    {
        length = *pTempBuf & 0x7F;               // 0 .. 127
        numLenBytes = 1;
    }
    else
    {
        if ((*pTempBuf & 0x7F) == 1)            // 1 more byte following
        {
            pTempBuf++;
            length = *pTempBuf;                // 128 .. 255
            numLenBytes = 2;
        }
        else if ((*pTempBuf & 0x7F) == 2)    // 2 more bytes following
        {
            pTempBuf++;
            length = *pTempBuf << 8;
            pTempBuf++;
            length |= *pTempBuf;               // 256 .. 65534
            numLenBytes = 3;
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","%s: Too big length", __FUNCTION__);
            return -1;
        }
    }

    apduLength -= numLenBytes;
    apdu += numLenBytes;

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: apdu Length = %d, numLenBytes = %d \n",
            __FUNCTION__, apduLength, numLenBytes);
    vlMpeosDumpBuffer(RDK_LOG_DEBUG, "LOG.RDK.POD", apdu, apduLength);

    if (tag == RMF_PODMGR_CA_PMT_TAG)
    {
      if (apdu[7] == OK_DESCRAMBLING)
      {
        unsigned char *pData;
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD",
                "cPOD_driver::PODSendCaPmtAPDU: OK_DESCRAMBLING received \n");
        pData = (unsigned char *)malloc(apduLength);
        if(pData == NULL)
            return iRet;

        memcpy( pData, apdu, apduLength );
        iRet = vlCPCaDescdata( pData, apduLength, NULL );
        free(pData);
        if(iRet != 0)
        {
          RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD",
                  "cPOD_driver::PODSendCaPmtAPDU: ERROR !! Not sending the CA PMT APDU \n");
          return iRet;
        }
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD",
                "cPOD_driver::PODSendCaPmtAPDU: APDU to card with TAG:%X\n",tag);
        iRet = caSndApdu( sesnId, tag, apduLength, apdu );
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD",
                "cPOD_driver::PODSendCaPmtAPDU: Done. APDU to card with TAG:%X\n",tag);
      }
      else if (apdu[7] == CA_NO_SELECT_)
      {
        unsigned char prgIndex;
        prgIndex = apdu[0];

        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD",
                "cPOD_driver::PODSendCaPmtAPDU: APDU to card with TAG:%X\n",tag);
        iRet =  caSndApdu( sesnId, tag, apduLength, apdu );
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD",
                "cPOD_driver::PODSendCaPmtAPDU: Done... APDU to card with TAG:%X\n",tag);

        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD",
                "cPOD_driver::PODSendCaPmtAPDU: calling vlCardStopConfigureCipher\n");
        vlCardStopConfigureCipher( apdu, apduLength );

        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD",
                "cPOD_driver::PODSendCaPmtAPDU: Clear the CA RES table index for Program Index %d\n", prgIndex);
        vlClearCaResAllocatedForPrgIndex( prgIndex );
      }
      else
      {
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD",
                "cPOD_driver::PODSendCaPmtAPDU: APDU to card with TAG:%X\n",tag);
        iRet = caSndApdu( sesnId, tag, apduLength, apdu );
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD",
                "cPOD_driver::PODSendCaPmtAPDU: Done. APDU to card with TAG:%X\n",tag);
      }
    }
    else
    {
      RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD",
              "cPOD_driver::PODSendCaPmtAPDU: Entered with Unknown Tag:%X\n",tag);
    }

    return iRet;
}

int cPOD_driver::PODSendRawAPDU(short sesnId, unsigned long tag, unsigned char *apdu, unsigned short apduLength)
{

    int  RsClass;
    int iRet = -1;


//RsClass = (int)( (tag & 0xFFFF0000) >> 16);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","cPOD_driver::PODSendRawAPDU: tag=0x%08X \n",tag);

    switch(tag)
    {

        case APP_INFO_REQ_TAG:
            if(vlg_HomingCardFirmwareUpgradeInProgress == 1)
                break;
        case SERVER_QUERY_TAG:
        {
            iRet = aiSndApdu(tag, apduLength, apdu);
            break;
        }
        case MMI_CLOSE_REQ_TAG:
        case MMI_OPEN_CONF_TAG:
        case MMI_CLOSE_CONF_TAG:
        {
            iRet = mmiSndApdu( tag, apduLength, apdu);
            break;
        }
        case  SAS_CONNECT_REQ_TAG:
        case  SAS_DATA_REQ_TAG:
        case  SAS_DATA_AV_TAG:
        case  SAS_DATA_CONF_TAG:
        case  SAS_DATA_QUERY_TAG:
        case  SAS_DATA_REPLY_TAG:
        case  SAS_ASYNC_MSG_TAG:
        {
            if(vlg_HomingCardFirmwareUpgradeInProgress == 0)
                iRet = SAS_APDU_FROM_HOST( sesnId,tag, apdu, apduLength );
            break;
        }
        case HOST_ADDR_PRT_REQ_TAG:
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," Calling hostAddPrtReq ....... \n");
            if(vlg_HomingCardFirmwareUpgradeInProgress == 0)
                iRet = hostAddPrtReq(apdu,apduLength);
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," After Calling hostAddPrtReq ....... Ret :%d \n",iRet);
            break;
        }
        case CA_INFO_INQUR_TAG:
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "PODSendRawAPDU: CA_INFO_INQUR_TAG sending request \n");
            iRet =  caSndApdu(sesnId, tag, apduLength, apdu);
            break;
        }
        default:
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","PODSendRawAPDU: This resource is Not supported  \n");
            break;

        }
    }

    return iRet;

}
void cPOD_driver::PODTrackResetCount()
{
    time(&fluxTimer.tFirst);
    
    if ( difftime(fluxTimer.tFirst, fluxTimer.tLast) > fluxTimer.tTimeThreshold)
    {
        fluxTimer.tLast = fluxTimer.tFirst;
        fluxTimer.nCount = 0;
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","%s: Resetting pod reset counter controls\n", __FUNCTION__);
    }
    else
    {
        fluxTimer.nCount++;
        if (fluxTimer.nCount > fluxTimer.nCountThreshold)
        {
            RDK_LOG(RDK_LOG_WARN, "LOG.RDK.POD","%s: Pod resets exceed %d resets in %d seconds\n", __FUNCTION__, fluxTimer.nCountThreshold, fluxTimer.tTimeThreshold);
            PostBootEvent(RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD, RMF_BOOTMSG_DIAG_MESG_CODE_ERROR, 0);
            int nCableCardRecoveryRebootAttempts = vl_env_get_int(0, "FEATURE.CABLE_CARD_ERROR_RECOVERY.MAX_REBOOT_ATTEMPTS");
            rmf_RebootHostTillLimit(__FUNCTION__, "Excessive pod resets in short time interval.", "cable_card_error", nCableCardRecoveryRebootAttempts);
        }
    }
}
extern "C" unsigned short  lookupCaSessionValue(void);
extern "C" unsigned char vlCardResGetMaxElmStreamsSupportedByCard();
extern "C" unsigned char vlCardResGetMaxProgramsSupportedByCard();
extern "C" unsigned char vlCardResGetMaxTSSupportedByCard();

int cPOD_driver::PODGetCardResElmStrInfo( unsigned char *pNumElmStrms)
{
    unsigned char num;
    
    num = vlCardResGetMaxElmStreamsSupportedByCard();
    if(num == 0)
        return -1;
    
    *pNumElmStrms = num;
    
    return 0;
    
}
int cPOD_driver::PODGetCardResPrgInfo( unsigned char *pNumPrgms)
{
    unsigned char num;
    
    num = vlCardResGetMaxProgramsSupportedByCard();
    if(num == 0)
        return -1;
    
    *pNumPrgms = num;
    
    return 0;
    
}
int cPOD_driver::PODGetCardResTrStrInfo( unsigned char *pNumTrsps)
{
    unsigned char num;
    
    num = vlCardResGetMaxTSSupportedByCard();
    if(num == 0)
        return -1;
    
    *pNumTrsps = num;
    
    return 0;
    
}

int cPOD_driver::PODGetCASesId( unsigned short *pSessionId)
{
    unsigned short SesId = 0;
    SesId = lookupCaSessionValue();
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "PODGetCASesId: SesId:%d \n",SesId);
    if(SesId == 0)
        return -1;
    
    *pSessionId = SesId;
    
    return 0;
    
}
int cPOD_driver::POD_POLL_ON_SND (DEVICE_HANDLE_t hPODHandle, int  *pData)
{
    cableCardDriver *cablecard_device;
    cablecard_device = get_cablecard_device();//pfc_kernel_global->get_cablecard_device();
    if(cablecard_device)
    {
        return cablecard_device->if_control( hPODHandle, VL_POD_IF_COMMAND_POLL_ON_SND, pData);
    }
    return -1;
}
extern "C" int cPOD_driver_POD_INTERFACE_RESET (DEVICE_HANDLE_t hPODHandle, int  *pData)
{
    cableCardDriver *cablecard_device;
    cablecard_device = get_cablecard_device();
    if(cablecard_device)
    {
        return cablecard_device->if_control( hPODHandle, VL_POD_IF_COMMAND_RESET_INTERFACE, pData);
    }
    else
    {
        
    }
    return -1;
}
extern "C" int cPOD_driver_POD_RESET_PCMCIA(DEVICE_HANDLE_t hPODHandle, int *pData)
{
    cableCardDriver *cablecard_device;
    cablecard_device = get_cablecard_device();
    if(cablecard_device)
    {
        return cablecard_device->if_control( hPODHandle, VL_POD_IF_COMMAND_RESET_PCMCIA, pData);
    }
    return -1;
}

int cPOD_driver::POD_INTERFACE_RESET (DEVICE_HANDLE_t hPODHandle, int  *pData)
{
    cableCardDriver *cablecard_device;
    cablecard_device = get_cablecard_device();//pfc_kernel_global->get_cablecard_device();
    if(cablecard_device)
    {
        return cablecard_device->if_control( hPODHandle, VL_POD_IF_COMMAND_RESET_INTERFACE, pData);
    }
    return -1;
}
int cPOD_driver::POD_RESET_PCMCIA(DEVICE_HANDLE_t hPODHandle, int *pData)
{
    cableCardDriver *cablecard_device;
    cablecard_device = get_cablecard_device();//pfc_kernel_global->get_cablecard_device();
    if(cablecard_device)
    {
        return cablecard_device->if_control( hPODHandle, VL_POD_IF_COMMAND_RESET_PCMCIA, pData);
    }
    return -1;
}

int cPOD_driver::POD_CONFIG_CIPHER(unsigned char ltsid,unsigned short PrgNum,unsigned long *decodePid,unsigned long numpids,unsigned long DesKeyAHi,unsigned long DesKeyALo,unsigned long DesKeyBHi,unsigned long DesKeyBLo,void *pStrPtr)
{
    cableCardDriver *cablecard_device;
    cablecard_device = get_cablecard_device();//pfc_kernel_global->get_cablecard_device();
    if(cablecard_device)
    {
        return cablecard_device->configPodCipher(ltsid,PrgNum,decodePid,numpids,DesKeyAHi,DesKeyALo, DesKeyBHi, DesKeyBLo,pStrPtr);

    }
    return -1;
}
int cPOD_driver::POD_STOP_CONFIG_CIPHER(unsigned char ltsid,unsigned short PrgNum,unsigned long *decodePid,unsigned long numpids)
{
    cableCardDriver *cablecard_device;
    cablecard_device = get_cablecard_device();//pfc_kernel_global->get_cablecard_device();
    if(cablecard_device)
    {
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","POD_STOP_CONFIG_CIPHER: Calling stopconfigPodCipher \n");
        return cablecard_device->stopconfigPodCipher(ltsid,PrgNum,decodePid,numpids);

    }
    return -1;
}
extern "C" int HAL_POD_POLL_ON_SND (DEVICE_HANDLE_t hPODHandle, int  *pData)
{
 
    
    return cPOD_driver::POD_POLL_ON_SND( hPODHandle, pData);
}
#if 0
extern "C" int HAL_POD_INTERFACE_RESET (DEVICE_HANDLE_t hPODHandle, int  *pData)
{
    
    return cPOD_driver::POD_INTERFACE_RESET( hPODHandle, pData);
}
extern "C" int HAL_POD_RESET_PCMCIA(DEVICE_HANDLE_t hPODHandle, int *pData)
{
      rmf_osal_threadSleep(1000, 0);  
    return cPOD_driver::POD_RESET_PCMCIA( hPODHandle, pData);
}
#endif
extern "C" int HAL_POD_CONFIGURE_CIPHER(unsigned char ltsid,unsigned short PrgNum,unsigned long *decodePid,unsigned long numpids,unsigned long DesKeyAHi,unsigned long DesKeyALo,unsigned long DesKeyBHi,unsigned long DesKeyBLo,void *pStrPtr)
{
    return cPOD_driver::POD_CONFIG_CIPHER(ltsid,PrgNum,decodePid,numpids,DesKeyAHi, DesKeyALo, DesKeyBHi,DesKeyBLo,pStrPtr);
}

extern "C" int HAL_POD_STOP_CONFIGURE_CIPHER(unsigned char ltsid,unsigned short PrgNum,unsigned long *decodePid,unsigned long numpids)
{
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","HAL_POD_STOP_CONFIGURE_CIPHER: Calling cPOD_driver::POD_STOP_CONFIG_CIPHER \n");
    return cPOD_driver::POD_STOP_CONFIG_CIPHER(ltsid,PrgNum,decodePid,numpids);
}
extern "C" int HomingCardFirmwareUpgradeSetState(int state)
{
    vlg_HomingCardFirmwareUpgradeInProgress = state;
    return 0;
}
extern "C" int HomingCardFirmwareUpgradeGetState()
{
    return vlg_HomingCardFirmwareUpgradeInProgress;
}


