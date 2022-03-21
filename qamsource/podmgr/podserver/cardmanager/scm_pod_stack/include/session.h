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



#ifndef        __SESSION_H
#define        __SESSION_H   

#ifdef __cplusplus
extern "C" {
#endif 

                                          /* HOST <--> MODULE */
#define SESS_TAG_OPEN_SESSION_REQ    0x91 /*      <--         */
#define SESS_TAG_OPEN_SESSION_RESP   0x92 /*       -->        */
#define SESS_TAG_CREATE_SESSION      0x93 /*       -->        */
#define SESS_TAG_CREATE_SESSION_RESP 0x94 /*      <--         */
#define SESS_TAG_CLOSE_SESSION_REQ   0x95 /*      <-->        */
#define SESS_TAG_CLOSE_SESSION_RESP  0x96 /*      <-->        */
#define SESS_TAG_SESSION_NUMBER      0x90 /*      <-->        */

// Close session status
#define SESS_STATUS_CLOSED        0x00
#define SESS_STATUS_NOT_ALLOCATED 0xF0

// Open Session status
#define SESS_STATUS_OPENED            0x00
#define SESS_STATUS_RES_DONT_EXIST    0xF0
#define SESS_STATUS_NOT_AVAIL        0xF1
#define SESS_STATUS_BAD_VERS        0xF2
#define SESS_STATUS_RES_BUSY        0xF3

typedef struct
{
  UCHAR  bTag;
  UCHAR  bSessionStatus;
  USHORT wSessionNb;
  ULONG  lResourceId;
  UCHAR  *pData;
  ULONG  lDataSize;
}T_SPDU;


//BOOL session_QueueSPDU (POD_BUFFER stBufferHandle);   // replaced with addr - jdm
BOOL session_QueueSPDU (POD_BUFFER * stBufferHandle);
BOOL session_CreateSPDU (UCHAR TcId, USHORT wSessionNb, USHORT lPayloadSize, POD_BUFFER *pBufferHandle);


void session_ProcessSPDU (POD_BUFFER *pstBufferHandle, UCHAR TcId);
void session_OnTCClosed (UCHAR TcId);
void session_Init (void);


void rsc_OnTCClosed (UCHAR TcId);
BOOL rsc_HandleCloseSessionReq (USHORT wSessionNb);
BOOL rsc_HandleOpenSessionReq (ULONG lResourceId, UCHAR TcId, USHORT *pwSessionNb, UCHAR *pbStatus);
void rsc_OnSessionOpened (USHORT wSessionNb);
void rsc_ProcessAPDU (POD_BUFFER *pstBufferHandle, USHORT wSessionNb);

#ifdef __cplusplus
}
#endif 

#endif        // __SESSION_H
