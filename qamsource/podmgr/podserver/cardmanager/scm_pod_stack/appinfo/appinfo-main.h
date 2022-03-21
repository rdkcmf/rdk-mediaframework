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

    
#ifndef _APPINFO_MAIN_H_
#define _APPINFO_MAIN_H_

#include <podapi.h>


#ifdef __cplusplus
extern "C" {
#endif

/* This macro assumes that 'x' is a 2-byte data type */
#define BYTE_SWAP(x) ((((x) & 0x00ff) << 8) | (((x) & 0xff00) >> 8))
     
// Externs
extern unsigned long RM_Get_Ver( unsigned long id );

// Prototypes
void aiProc(void*);
void aiRcvAppInfo( pod_appInfo_t * );
static int aiSndEvent(SYSTEM_EVENTS, uint8 *, uint16, uint32 x, uint32 y, uint32 z);
static void aiOpenSessionReq( uint32 resId, uint8 tcId );
static void aiSessionOpened( uint16 sessNbr, uint32 resId, uint32 tcId );
static void aiSessionClosed( uint16 sessNbr );
/*static*/ int aiSndApdu(uint32 tag, uint32 dataLen, uint8 *data);

// RAJASREE 11/08/07 [start] to send raw apdu from java app
/*static*/ int aiSndRawApdu( uint32 tag, uint32 dataLen, uint8 *data);
// RAJASREE 11/08/07 [end] 

static void aiRcvAppInfoCnf(uint16 sessId, void * pApdu, uint32 apduLen);
void aiRcvServerQuery (uint8 *data, uint32 dataLen);
static void aiRcvServerReply (uint16 sessId, void *pApdu, uint32 apduLen);

/* Determines apdu len_field and payload start.
 * p should point to the beginning of the received apdu.
 * Upon return, p will point to the beginning of the payload data
 */
static inline uint16 getPayloadStart(uint8 **p)
{
    uint16 len = 0;

    *p += 3;
    
#if (1)
    *p += bGetTLVLength ( *p, &len );
#else /* Old way: Remove this code later */
        
    // Figure out length field
    if ((**p & 0x82) == 0x82) { // two bytes
        (*p)++;
            len = (uint16)BYTE_SWAP(*(uint16*)*p);
        *p += 2;
    }
    else if ((**p & 0x81) == 0x81) { // one byte
        (*p)++;
        len = (uint8)(**p);
        (*p)++;
    }
    else { // lower 7 bits
        len = (uint8)(**p & 0x7F);
        (*p)++;
    }
    
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s len = %d\n", __FUNCTION__, len);
    
#endif
    return len;
}

#ifdef __cplusplus
}
#endif


#endif //_APPINFO_MAIN_H_

