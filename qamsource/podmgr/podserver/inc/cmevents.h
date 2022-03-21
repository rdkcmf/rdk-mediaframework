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



/** \addtogroup Plugins */
/** \addtogroup TableManager Table Manager
* \ingroup Plugins
* Table manager.
* \{
*/

/**
* \file tablemgr.h \ref Plugins "[Plugin:"\ref TableManager "TableManager]" Table manager.
*/

#ifndef __CMEVENTS_H__
#define __CMEVENTS_H__

#include "cmThreadBase.h"
#include "rmf_osal_event.h"

class CardManager;
 

typedef enum{
    GATEWAY_QUEUE       = 24,
    POD_MSG_QUEUE    = 26,
    POD_APPINFO_QUEUE   = 36,
    POD_CA_QUEUE        = 37,
    POD_CPROT_QUEUE     = 38,
    POD_DIAG_QUEUE      = 39,
    POD_FEATURE_QUEUE   = 40,
    POD_HOMING_QUEUE    = 41,
    POD_HOST_QUEUE      = 42,
    POD_IPPV_QUEUE      = 43,
    POD_LOWSPD_QUEUE    = 44,
    POD_MMI_QUEUE       = 45,
    POD_RSMGR_QUEUE     = 46,
    POD_SAS_QUEUE       = 47,
    POD_SYSTEM_QUEUE    = 48,
    POD_SYSTIME_QUEUE   = 49,
    POD_XCHAN_QUEUE     = 50,
    // Watchdog queue
    POD_ERROR_QUEUE = 59,
     POD_CARD_RES_QUEUE = 60,
     POD_DSG_QUEUE       = 61,
    POD_CARD_MIB_ACC_QUEUE = 62,
    POD_HOST_ADD_PRT_QUEUE = 63,
    POD_HEAD_END_COMM_QUEUE = 64,
    POD_IPDM_COMM_QUEUE = 65,
    POD_IPDIRECT_UNICAST_COMM_QUEUE = 66,
}POD_EVENT_QUEUE;

#define POD_OFFSET                 0x11100
#define POD_ERROR_OFFSET       0x1C000
#define ASYNC_OFFSET               0x18000
#define FRONTEND_CONTROL_OFFSET     0x3500
#define CDL_OFFSET           0x31000

typedef enum
{

// POD events as defined in SCM POD stack
        /* API-related (via BB) */
    
    MR_CHANNEL_CHANGE_START       = 80,
    
    POD_OPEN             = POD_OFFSET,              // 0x11100
    POD_CLOSE,                      //                   11101

    POD_IB_TUNER_START,             //                   11102
    POD_IB_TUNER_BLOCK,             //                   11103
    POD_IB_TUNER_FREE,              //                   11104

    POD_SERVER_QUERY,               //                   11105
    POD_SERVER_REPLY,               //                   11106

    POD_OPEN_MMI_REQUEST,           //                   11107
    POD_MMI_DATA,                   //                   11108
    POD_MMI_SERVER_REPLY,           //                   11109

    POD_CA_HAS_BEEN_UPDATED,        //                   1110A
    POD_HOMING_USER_NOTIFY_MESS,            //                   1110B
    POD_HOMING_USER_END_MESS,           //                   1110C
    POD_HOST_FIRMUPGRADE_START,     //                   1110D
    POD_HOST_FIRMUPGRADE_END,      //                    1110E
    POD_DISPLAY_ERROR,              //                   1110F

    POD_CLOSE_MMI_REQUEST = POD_OFFSET + 0x10,      // 0x11110

    POD_ENTER_STANDBY     = POD_OFFSET + 0x20,      // 0x11120
    POD_EXIT_STANDBY,               //                   11121
    POD_CP_CARD_CERT_ERROR,        //                11122
    POD_STANDBY_NOTIFICATION,           //                11123
    POD_FEATURE_LIST_REQ = POD_OFFSET + 0x30,   // 0x11130
    POD_FEATURE_LIST_CNF,           //                   11131
    POD_FEATURE_LIST_X,             //                   11132
    POD_FEATURE_LIST_CHANGE,        //                   11133

    POD_FEATURE_PARAMS_REQ = POD_OFFSET + 0x40, // 0x11140
    POD_FEATURE_PARAMS_CNF,         //                   11141
    POD_FEATURE_PARAMS,             //                   11142
    POD_HOST_FEATURE_PARAM_CHANGE,            //11143
        /* state machine-related */
    POD_INITIALIZE        = POD_OFFSET + 0x50,      // 0x11150
    POD_RUN_STACK,                  //                   11201
    POD_INTERFACE_LOADED,           //                   11202
    POD_RESTART,                    //                   11203
    POD_TERMINATE,                  //                   11204
    
    POD_OPEN_MMI_CONF,           //                   11205
    POD_CLOSE_MMI_CONF,         //              11206
        /* pod resource-related */
    POD_RESOURCE_PRESENT  = POD_OFFSET + 0x200,      // 0x11300
        /* application manager originated */
    POD_OPEN_SESSION_REQ  = POD_RESOURCE_PRESENT + 0x70,      // 0x11370
    POD_SESSION_OPENED,             //                   11371
    POD_CLOSE_SESSION,              //                   11372
    POD_APDU,                       //                   11373
        /* application info additional */
    POD_AI_APPINFO_CNF,             //                   11374
    POD_HC_OOB_TUNE_RESP,           //                   11375
    POD_HC_IB_TUNE_RESP,            //                   11376

    POD_SET_EMI_VALUE,
    POD_TIMER_EXPIRES,              //                   11377
    POD_CA_PMT_READY,               //                   11378
    POD_INSERTED,            //                   11379
    POD_REMOVED    ,            //              11380
    POD_CP_PMT_READY,               //                   11381
    POD_SYSTEM_DOWNLD_CTL,       //                   11382
    POD_CA_NO_SELECT,                     //11383
		POD_IPDU_DOWNLOAD_TIMER,							//11384
		POD_IPDU_RECV_FLOW_DATA,							//11385
		POD_IPDU_UPLOAD_TIMER,  							//11386
    POD_UDP_SOCKET_DATA,                            //11387
    
    ASYNC_OPEN                          = ASYNC_OFFSET,
    ASYNC_CLOSE                         = ASYNC_OFFSET+1,
    ASYNC_TDB_OB_DATA                   = ASYNC_OFFSET+48,
    ASYNC_EC_FLOW_REQUEST               = ASYNC_OFFSET+35,
    ASYNC_EC_FLOW_REQUEST_CNF           = ASYNC_OFFSET+36,
    ASYNC_EC_FLOW_DELETE                = ASYNC_OFFSET+37,
    ASYNC_EC_FLOW_DELETE_CNF            = ASYNC_OFFSET+38, 
    ASYNC_EC_FLOW_LOST_EVENT            = ASYNC_OFFSET+39, 
    ASYNC_EC_FLOW_LOST_EVENT_CNF        = ASYNC_OFFSET+40, 
    ASYNC_START_OOB_QPSK_IP_FLOW        = ASYNC_OFFSET+53,
    ASYNC_STOP_OOB_QPSK_IP_FLOW         = ASYNC_OFFSET+54,
    // IPDU register flow
    ASYNC_START_IPDU_XCHAN_FLOW         = ASYNC_OFFSET+55,
    ASYNC_STOP_IPDU_XCHAN_FLOW          = ASYNC_OFFSET+56,
    
    
    POD_ERROR                    = POD_ERROR_OFFSET+10,
    POD_BAD_MOD_INSERT_EROR            = POD_ERROR_OFFSET+11,
    
    FRONTEND_OOB_DS_TUNE_REQUEST            = FRONTEND_CONTROL_OFFSET + 20,
    FRONTEND_OOB_DS_TUNE_STATUS             = FRONTEND_CONTROL_OFFSET + 21,
    FRONTEND_OOB_US_TUNE_REQUEST            = FRONTEND_CONTROL_OFFSET + 35,
    FRONTEND_OOB_US_TUNE_STATUS             = FRONTEND_CONTROL_OFFSET + 36,
    FRONTEND_OOB_US_TUNE_STATUS_SENT        = FRONTEND_CONTROL_OFFSET + 37,
    FRONTEND_INBAND_TUNE_REQUEST            = FRONTEND_CONTROL_OFFSET + 38,
    FRONTEND_INBAND_TUNE_STATUS             = FRONTEND_CONTROL_OFFSET + 39,
    FRONTEND_INBAND_TUNE_RELEASE         = FRONTEND_CONTROL_OFFSET + 40,

    VL_CDL_DISP_MESSAGE            = CDL_OFFSET, //0x3100    //0x31000
}POD_EVENTS;



typedef enum
{

    POD_SESSION_NOT_FOUND=0x10
    



}POD_ERROR_EVENT;





typedef struct
{
  unsigned SrcQueue;
  unsigned DstQueue;
  long int event;
  long int x;
  long int y;
  long int z;
  unsigned dataLength;
  void     *dataPtr;
  int      time;
  int      returnStatus;
  unsigned sequenceNumber;
}LCSM_EVENT_INFO;
    
rmf_osal_event_category_t GetCategory(unsigned queue);
#endif
