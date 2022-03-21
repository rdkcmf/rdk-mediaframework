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


/*
**
**
**  File: global_queues.h
**
**
*/

#ifndef _GLOBAL_QUEUES_H_
#define _GLOBAL_QUEUES_H_

typedef enum
{
  // San Diego Thread Queues
  SD_MAIN_CONTROL_QUEUE      = 0,
  SD_MAIN_BOARD_SERIAL_TX    = 1,
  SD_MAIN_BOARD_SERIAL_RX    = 2,
  SD_MR_CONTROL              = 3,
  SD_CLOSED_CAPTION_QUEUE    = 4,    
  SD_SCAN_SERVER             = 5,
  BURBANK_QUEUE              = 6,
  
  // for TDB
  TDB_XPORT_QUEUE            = 7,
  TDB_MSG_QUEUE              = 8,
  TDB_SUPPORT_QUEUE          = 9,
  SD_I2C_SERVER_QUEUE        = 10,

  // Kernel Thread Queues
  TRANSPORT_THREAD_QUEUE     = 11,
  MPEG_THREAD_QUEUE          = 12,
  MPEG_ISR_QUEUE             = 13,
  TDB_MULTI_SECTION          = 14,
  
  // TCP DEBUG MONITOR QUEUE
  TCP_MONITOR_QUEUE          = 15,
  CLOSED_CAPTION_QUEUE       = 16,

  SD_I2C_SERVER_QUEUE_ACK    = 17,

  IRB_QUEUE                  = 18,
  IRB_WAIT_QUEUE             = 19,

  // IEEE1394 Control
  SD_IEEE1394_CONTROL_QUEUE  = 20,

  // Burbank Interface Support
  BURBANK_MONITOR_QUEUE      = 21,

  // Graphics queue
  GRAPHICS_THREAD_QUEUE      = 22,

  // front end control
  SD_FRONTEND_CONTROL_QUEUE  = 23,

  // gateway to Burbank queue
  GATEWAY_QUEUE              = 24,

  // Emergency Alert System
  EAS_MSG_QUEUE              = 25,

  // Point of Deployment
  POD_MSG_QUEUE              = 26,

  // NVM Manager Thread Queue
  NVM_REQ_QUEUE              = 27,
  NVM_ACK_QUEUE              = 28,

  BLASTER_SERVER_QUEUE       = 29,

  SD_MB_RX_UART_QUEUE        = 30,
  SD_MB_TX_UART_QUEUE        = 31,
  SD_BB_ICD_MSG_QUEUE        = 32,
  PAXEL_QUEUE                = 33,
  NEW_FORTH_QUEUE            = 34,
  TDB_XPORT_TIMER_QUEUE      = 35,

  // POD RESOURCES QUEUES
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
    POD_CARD_RES_QUEUE  = 60,
    POD_DSG_QUEUE       = 61,
    POD_CARD_MIB_ACC_QUEUE = 62,
    POD_HOST_ADD_PRT_QUEUE = 63,
    POD_HEAD_END_COMM_QUEUE = 64,
    POD_IPDM_COMM_QUEUE = 65,
    POD_IPDIRECT_UNICAST_COMM_QUEUE = 66,
// OOB control queue
    SD_OOB_CONTROL      = 51,
    TDB_ASYNC_INTERFACE_INBAND = 52,
    TBD_ASYNC_INTERFACE_OUTBAND = 53,
    TBD_ASYNC_DEBUG_QUEUE       = 54,

// Copy Control Information Manager
    CCI_MGR_QUEUE = 55,

    POWER_MGNT_QUEUE = 56,
    POWER_MGNT_ALT_QUEUE = 57,

// Flash Signature Check queue
    SD_FLASHCK_QUEUE = 58,

// Watchdog queue
    POD_ERROR_QUEUE = 59,

    POD_IPDM_QUEUE = POD_IPDM_COMM_QUEUE,
    POD_IPDIRECT_UNICAST_QUEUE = POD_IPDIRECT_UNICAST_COMM_QUEUE,
       
  /*
   not free queues start at 60
   if we go over 59 please change
   parameters in lcsm_main
  */  



}SYSTEM_QUEUES;  //first 100 queues are automatically generated and initited/instansated
                 //as defined by LCSM_QUEUES
                 //plus can't grow into freequeues which starts at LCSM_FREE_QUEUE_START    = 60
                 //else update

#endif
