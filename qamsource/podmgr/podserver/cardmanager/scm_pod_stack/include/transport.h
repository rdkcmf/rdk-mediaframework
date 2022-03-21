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



#ifndef        __TRANSPORT_H
#define        __TRANSPORT_H

#ifdef __cplusplus
extern "C" {
#endif 

#define TRANS_TAG_SB                0x80
#define TRANS_TAG_RECEIVE           0x81
#define TRANS_TAG_CREATE_TC         0x82
#define TRANS_TAG_CREATE_TC_REPLY   0x83
#define TRANS_TAG_DELETE_TC         0x84
#define TRANS_TAG_DELETE_TC_REPLY   0x85
#define TRANS_TAG_REQUEST_TC        0x86
#define TRANS_TAG_NEW_TC            0x87
#define TRANS_TAG_ERROR             0x88
#define TRANS_TAG_DATA_LAST         0xA0
#define TRANS_TAG_DATA_MORE         0xA1

#define TRANS_STATE_NOT_USED        0x00
#define TRANS_STATE_IN_CREATION     0x01
#define TRANS_STATE_ACTIVE          0x02

#define TRANS_MAXSIZE_TPDU          512
#define TRANS_NBMAX_TC              5

 
typedef struct 
{
      UCHAR            TcId;         
      USHORT        lSize;        
      UCHAR            *pOrigin;    
      UCHAR            *pData;    
      ULONG            FlowID;
} POD_BUFFER;        
 
typedef struct
{
  UCHAR   bTag;
  USHORT  lDataSize;
  UCHAR   TcId;
  UCHAR   *pData;
  BOOL bDA;
}T_TPDU_IN;

typedef struct
{
  UCHAR  TcId;
  UCHAR  bState;
  ULONG  lSize;
  ULONG  lOffset;
  UCHAR  *pDataFixed;
  UCHAR  *pData;
}T_TC;

extern T_TC  e_pstTC [];
extern UCHAR e_bNbTC;

extern ULONG  ulTCCreateTimeMs;         /* Time when Transport connection was created. */


#define TRANSPORT_TIMEOUT  5000 /* 5 seconds */
extern BOOL   bTransportTimeoutEnabled; /* Flag Transport timeouts are enabled or disabled */
extern ULONG  ulLastTimeReceivedData;     /* Last time that the pod was written to on the data channel */
extern ULONG  ulPodPollResponseTime;    /* Time that the POD responded to our last time */

USHORT  trans_GetTpduSize (UCHAR *pData);
void    trans_ProcessTPDU (T_TC *pstTC);
BOOL    trans_CreateTPDU  (UCHAR bTag, UCHAR TcId, USHORT lPayloadSize, POD_BUFFER *pBufferHandle);
//BOOL    trans_QueueTPDU   (POD_BUFFER BufferHandle);    // replaced with addr - jdm
BOOL    trans_QueueTPDU   (POD_BUFFER * BufferHandle);
UCHAR   trans_GetTcId (UCHAR *pTpdu);
BOOL    trans_CreateTC (UCHAR bNewTcId);
UCHAR   trans_Communicate (UCHAR *pbTpdu, USHORT usSize);
BOOL    trans_Poll (void);
void    trans_Init (void);
void    trans_Exit (void);

void trans_OnTCClosed (UCHAR TcId);

#ifdef __cplusplus
}
#endif 

#endif        //    __TRANSPORT_H
