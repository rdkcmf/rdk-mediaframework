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


 

  
#ifndef __PFC_WRAPPER_H__
#define __PFC_WRAPPER_H__



#ifdef __cplusplus
extern "C" {
#endif 


typedef unsigned char   UCHAR;
typedef unsigned short  USHORT;
typedef unsigned long   ULONG;
typedef int             BOOL; 
typedef unsigned long   DWORD;
typedef void            VOID;
#ifndef PVOID
typedef void *          PVOID;
#endif
typedef unsigned char * PUCHAR;
typedef unsigned long DEVICE_HANDLE_t;
typedef int LCSM_DEVICE_HANDLE;
typedef  long int  LCSM_TIMER_HANDLE;
//includes from POD STack
#include "transport.h"
#include "podhighapi.h"
#include "resourcetable.h"
#include "cablecarddriver.h"
#include <time.h>
///////////////////////


#ifdef __cplusplus
}
#endif  


typedef enum 
{                   /* Card Status */  
    POD_CARD_INSERTED = 0,   /* The cablecard is present/inserted */
    POD_CARD_REMOVED = 1,    /* The cablecard is removed*/ 
    POD_CARD_ERROR = 2,      /* CableCard generated a error*/
 }POD_STATUS_t;


typedef enum
  {
    LCSM_REGISTER_EVENT_SUCCESS         = 0,
    LCSM_REGISTER_EVENT_EVENT_NOT_FOUND = -1,
    LCSM_REGISTER_EVENT_QUEUE_NOT_FOUND = -2
  }LCSM_REGISTER_EVENT_RETURN_STATUS;
  
typedef struct _FLUX_TIMER
{
    time_t tFirst;
    time_t tLast;
    double tTimeThreshold;
    unsigned int nCount;
    unsigned int nCountThreshold;
} FLUX_TIMER;



#define CABLECARD_0    0
  
#if 0
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

#endif




typedef enum 
{
  LCSM_TIMER_OP_OK    =  0,
  LCSM_NO_TIMERS      = -1,
  LCSM_TIMER_EXPIRED  = -2,
}LCSM_TIMER_STATUS;

 


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

//Host addressable Property
#define HOST_ADDR_PRT_REQ_TAG       0x9f9f01
#define HOST_ADDR_PRT_RESP_TAG       0x9f9f02

//SAS APDUs
#define SAS_CONNECT_REQ_TAG       0x9f9a00
#define SAS_CONNECT_TAG           0x9f9a01
#define SAS_DATA_REQ_TAG       0x9f9a02
#define SAS_DATA_AV_TAG           0x9f9a03
#define SAS_DATA_CONF_TAG       0x9f9a04
#define SAS_DATA_QUERY_TAG       0x9f9a05
#define SAS_DATA_REPLY_TAG       0x9f9a06
#define SAS_ASYNC_MSG_TAG       0x9f9a07

#define CA_INFO_INQUR_TAG          0x9f8030
#define CA_PMT_TAG          0x9f8032

#define OK_DESCRAMBLING   0x01
#define CA_NO_SELECT_ 0x04
typedef enum
{
    UNKNOWN_POD  = 0,
    DK_HPNX_POD  = 1,
    SA_POD       = 2,
    MOT_POD      = 3,
    NDS_POD      = 4,
    FLASH_CARD   = 10
} POD_MANUFACTURER_t;
/*
typedef enum 
{                   // Card Modes
    POD_MODE_SCARD,   // The cablecard is a single stream card
    POD_MODE_MCARD    // The cablecard can handle multiple streams
    
} POD_MODE_t;  
*/
  
class cPOD_driver
{

public:

    cPOD_driver(CardManager *cm);
    cPOD_driver();
     
    ~cPOD_driver();
    
    int initialize();
    
     static void SendEventCallBack( unsigned queue,LCSM_EVENT_INFO *eventInfo);
 
    
    static void    POD_CardDetection_cb(DEVICE_HANDLE_t   hDevicehandle,
                                    unsigned long        cardType, 
                                    unsigned char      eStatus, 
                                    const void         *pData,
                                    unsigned long      ulDataLength,
                                    void               *pulDataRcvd);


    static void    POD_Send_Poll_cb(DEVICE_HANDLE_t      hDevicehandle, const void         *pData);
    
    static void    POD_Data_Available_cb(DEVICE_HANDLE_t      hDevicehandle,  T_TC *pTPDU, const void *pData);


    static void     POD_ExtCh_Data_Available_cb(DEVICE_HANDLE_t  hDevicehandle, ULONG lFlowId, const void  *pData, USHORT wSize);

 

    static int     POD_STACK_SEND_TPDU(void * stBufferHandle);

    static int    POD_STACK_SEND_ExtCh_Data(unsigned char * data,  unsigned long length, unsigned long flowID);
     
    static int POD_POLL_ON_SND (DEVICE_HANDLE_t hPODHandle, int  *pData);
    static int POD_INTERFACE_RESET (DEVICE_HANDLE_t hPODHandle, int  *pData);
    static int POD_RESET_PCMCIA(DEVICE_HANDLE_t hPODHandle, int  *pData);
    static int POD_CONFIG_CIPHER(unsigned char ltsid,unsigned short PrgNum,unsigned long *decodePid,unsigned long numpids,unsigned long DesKeyAHi,unsigned long DesKeyALo,unsigned long DesKeyBHi,unsigned long DesKeyBLo,void *pStrPtr);
    static int POD_STOP_CONFIG_CIPHER(unsigned char ltsid,unsigned short PrgNum,unsigned long *decodePid,unsigned long numpids);
    static void   ConnectSourceToPOD(unsigned long tuner_handle, unsigned long videoPid);
    static int    oob_control(unsigned long eCommand, void * pData);
    static int    if_control(unsigned long eCommand, void * pData);
    
        bool    IsPOD_NDS();
 

     static int lcsm_flush_queue( unsigned queue ); 

 
 
     static unsigned lcsm_send_event( LCSM_DEVICE_HANDLE handle, unsigned queue, LCSM_EVENT_INFO *eventInfo );

     static unsigned lcsm_send_immediate_event(LCSM_DEVICE_HANDLE handle,unsigned queue, LCSM_EVENT_INFO *eventInfo );

     static LCSM_REGISTER_EVENT_RETURN_STATUS lcsm_postEvent( LCSM_DEVICE_HANDLE handle,LCSM_EVENT_INFO *eventInfo );
  
     static LCSM_TIMER_HANDLE lcsm_send_timed_event(    LCSM_DEVICE_HANDLE  handle, unsigned queue,   LCSM_EVENT_INFO *eventInfo );
 
     static LCSM_TIMER_STATUS lcsm_cancel_timed_event(  LCSM_DEVICE_HANDLE  handle,   LCSM_TIMER_HANDLE timerHandle ); 

     static LCSM_TIMER_STATUS lcsm_reschedule_timed_event(   LCSM_DEVICE_HANDLE  handle,   LCSM_TIMER_HANDLE timerHandle, unsigned milliseconds );
 

    int podStartResources(LCSM_DEVICE_HANDLE h); 

    int podStopResources(); 
 
    static void    POD_Send_app_info_request(pod_appInfo_t    *pInfo);



    static unsigned nvmm_rawRead( unsigned nvRamOffset, unsigned length, unsigned char *localBuffer );

    static unsigned nvmm_rawWrite( unsigned nvRamOffset, unsigned length, unsigned char *localBuffer );



    static int PODSendRawAPDU(short sesnId, unsigned long tag, unsigned char *apdu, unsigned short apduLength);
    static int PODSendCardMibAccSnmpReqAPDU( unsigned char *Message, unsigned long MsgLength);
    static int CardManagerSendMsgEvent( char *pMessage, unsigned long length);
    static int PODGetCASesId( unsigned short *pSessionId);
    static int PODGetCardResElmStrInfo( unsigned char *pNumElmStrms);
    static int PODGetCardResPrgInfo( unsigned char *pNumPrgms);
    static int PODGetCardResTrStrInfo( unsigned char *pNumTrsps);
    static int PODSendCaPmtAPDU(short sesnId, unsigned long tag, unsigned char *apdu, unsigned short apduLength/*,uint16_t NumElmStrms,uint32_t *pElmStrmInfo*/);
    static void PODTrackResetCount();
    
    CardManager *cm;
    DEVICE_HANDLE_t        hPODHandle;
    
    cableCardDriver     *cablecard_device;
    
};




 

#endif


