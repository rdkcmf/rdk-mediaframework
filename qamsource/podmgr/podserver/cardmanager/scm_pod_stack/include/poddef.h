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
**  File: poddef.h
**
**
**
*/


#ifndef _PODDEF_H_
#define _PODDEF_H_

#ifdef __cplusplus
extern "C" {
#endif


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>



typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned long UINT32;
typedef signed char INT8;
typedef signed short INT16;
typedef signed long INT32;
typedef unsigned char BOOLEAN;
typedef UINT32 DEVICE_HANDLE_t;

 

typedef enum
  {
    LCSM_REGISTER_EVENT_SUCCESS         = 0,
    LCSM_REGISTER_EVENT_EVENT_NOT_FOUND = -1,
    LCSM_REGISTER_EVENT_QUEUE_NOT_FOUND = -2
  }LCSM_REGISTER_EVENT_RETURN_STATUS;



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

typedef int LCSM_DEVICE_HANDLE;



typedef  long int  LCSM_TIMER_HANDLE;
typedef enum 
{
  LCSM_TIMER_OP_OK    =  0,
  LCSM_NO_TIMERS      = -1,
  LCSM_TIMER_EXPIRED  = -2,
}LCSM_TIMER_STATUS;



#if 0
typedef enum
{
    UNKNOWN_POD  = 0,
    DK_HPNX_POD  = 1,
    SA_POD       = 2,
    MOT_POD      = 3,
    NDS_POD      = 4,
    FLASH_CARD   = 10
} POD_MANUFACTURER_t;
#endif


//   APDU tags - as shown in OC-SP-MC-IF-I02-040831

//Application Information APDUs
#define APP_INFO_REQ_TAG           0x9f8020
#define APP_INFO_CONF_TAG       0x9f8021
#define SERVER_QUERY_TAG       0x9f8022
#define SERVER_REPLY_TAG       0x9f8023


//MMI APDUs
#define MMI_OPEN_REQ_TAG       0x9f8820
#define MMI_CLOSE_REQ_TAG          0x9f8822
#define MMI_OPEN_CONF_TAG       0x9f8821
#define MMI_CLOSE_CONF_TAG       0x9f8823


//SAS APDUs
#define SAS_CONNECT_REQ_TAG       0x9f9a00
#define SAS_CONNECT_TAG           0x9f9a01
#define SAS_DATA_REQ_TAG       0x9f9a02
#define SAS_DATA_AV_TAG           0x9f9a03
#define SAS_DATA_CONF_TAG       0x9f9a04
#define SAS_DATA_QUERY_TAG       0x9f9a05
#define SAS_DATA_REPLY_TAG       0x9f9a06
#define SAS_ASYNC_MSG_TAG       0x9f9a07

typedef enum
{
DATA_CHANNEL,
EXTENDED_DATA_CHANNEL

}CHANNEL_TYPE_T;





//LCSM_REGISTER_EVENT_RETURN_STATUS lcsm_register_event( unsigned event, unsigned queue );
//LCSM_REGISTER_EVENT_RETURN_STATUS ( unsigned event, unsigned queue );
unsigned lcsm_send_event( LCSM_DEVICE_HANDLE handle, unsigned queue, LCSM_EVENT_INFO *eventInfo );
//unsigned lcsm_send_event( unsigned queue, LCSM_EVENT_INFO *eventInfo );
unsigned lcsm_send_immediate_event(  LCSM_DEVICE_HANDLE handle,unsigned queue, LCSM_EVENT_INFO *eventInfo );
//unsigned ( unsigned queue, LCSM_EVENT_INFO *eventInfo, int waitMilliseconds ); 
LCSM_REGISTER_EVENT_RETURN_STATUS lcsm_postEvent(LCSM_DEVICE_HANDLE handle,LCSM_EVENT_INFO *eventInfo );



LCSM_TIMER_HANDLE lcsm_send_timed_event(    LCSM_DEVICE_HANDLE  handle, unsigned queue,   LCSM_EVENT_INFO *eventInfo );
LCSM_TIMER_STATUS lcsm_cancel_timed_event(  LCSM_DEVICE_HANDLE  handle,   LCSM_TIMER_HANDLE timerHandle );
LCSM_TIMER_STATUS lcsm_reschedule_timed_event(   LCSM_DEVICE_HANDLE  handle,   LCSM_TIMER_HANDLE timerHandle, unsigned milliseconds ); 

// macros for sending messages to Burbank gateway
#define gateway_send_event( handle, eventInfo ) \
    lcsm_send_event( handle, GATEWAY_QUEUE, eventInfo )
    
#define gateway_send_immediate_event( handle, eventInfo ) \
    lcsm_send_immediate_event( handle, GATEWAY_QUEUE, eventInfo )
    

    
unsigned nvmm_rawRead( unsigned nvRamOffset, unsigned length, unsigned char *localBuffer );        
unsigned nvmm_rawWrite( unsigned nvRamOffset, unsigned length, unsigned char *localBuffer );    

//int POD_STACK_INIT(LCSM_DEVICE_HANDLE * h);

void POD_STACK_NOTIFY_CALLBACK(void *pFn);

#ifdef __cplusplus
}
#endif

#endif

 
