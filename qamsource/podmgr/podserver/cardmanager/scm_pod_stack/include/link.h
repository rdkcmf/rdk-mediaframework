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




#ifndef        __LINK_H 
#define        __LINK_H 

#ifdef __cplusplus
extern "C" {
#endif 


#define LINK_SIZE_LPDU_HEADER            2
#define LINK_SIZE_LPDU_EXTENDED_HEADER    4




//#define MAX_LINK_BUF_SIZE  128
#define MAX_LINK_BUF_SIZE  256 /* SCTE28 says 256 is host minimum -- Also NDS requires at least 254 */
#define MAX_MPEG_SIZE      4096
#define MIN_LINK_BUF_SIZE  16   /* As per SCTE28, min buf size for either buffer */  

UCHAR   link_SendTPDU (UCHAR *pTpdu, USHORT lSize);

BOOL link_GetModuleResponse (void);
BOOL link_ReadExtended (void);

UCHAR link_WriteExtended (ULONG FlowId, UCHAR *pData, USHORT wSize);
UCHAR link_SendOOBLPDU (ULONG    FlowID, UCHAR *pData, USHORT wSize, UCHAR MoreLast);
UCHAR link_SendOOBData(ULONG FlowId, UCHAR *pData, USHORT lSize);

#ifdef __cplusplus
}
#endif 

#endif        //    __LINK_H 
