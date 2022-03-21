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


 
//----------------------------------------------------------------------------
//
#include <string.h>
#include "poddef.h"
#include <lcsm_log.h>

#include "tsdecode.h"  //Hannah - copied

#include "utils.h"
#include "transport.h"
#include "session.h"
#include "podhighapi.h"
#include "appmgr.h"
#include "applitest.h"
#include "cardres_app.h"

#include "global_event_map.h"
#include <string.h>
#include <rdk_debug.h>
#ifdef __cplusplus
extern "C" {
#endif

extern LCSM_DEVICE_HANDLE cardresLCSMHandle;
extern LCSM_TIMER_HANDLE  cardresTimer;


UCHAR  gcardresCurrentVersionNumber;
UCHAR  gcardresCurrentNextIndicator;

static UCHAR * //advanced pointer position
LengthIs(unsigned char *  ptr, USHORT * pLenField) //point to APDU Packet
{
    ptr += 3;
    // Figure out length field
#if (1)
    ptr += bGetTLVLength ( ptr, pLenField );
#else /* Old way to compute len: Save for now (just in case!) Delete later */
    if ((*ptr & 0x82) == 0x82) { // two bytes
        ptr++;
                //need to byte swap
                *pLenField = (  (((*(USHORT *)(ptr)) & 0x00ff) << 8) 
                              | (((*(USHORT *)(ptr)) & 0xff00) >> 8)
                             );
        ptr += 2;
    }
    else if ((*ptr & 0x81) == 0x81) { // one byte
        ptr++;
        *pLenField = (unsigned char)(*ptr);
        ptr++;
    }
    else { // lower 7 bits
        *pLenField = (unsigned char)(*ptr & 0x7F);
        ptr++;
    }
#endif    
    return (ptr);
}


/**************************************************
 *    APDU Functions to handle CA SUPPORT
 *************************************************/
int cardresSndApdu(unsigned short sesnum, unsigned long tag, unsigned short dataLen, unsigned char *data)
{
    unsigned short alen;
    unsigned char papdu[MAX_APDU_HEADER_BYTES + dataLen];

	 memset(papdu, 0,sizeof(papdu));
    if (buildApdu(papdu, &alen, tag, dataLen, data ) == NULL)
       {
       MDEBUG (DPM_ERROR, "ERROR:ca: Unable to build apdu.\n");
       return APDU_CARDRES_FAILURE;
       }
    AM_APDU_FROM_HOST(sesnum, papdu, alen);
    return APDU_CARDRES_SUCCESS;
}


#ifdef __cplusplus
}
#endif

