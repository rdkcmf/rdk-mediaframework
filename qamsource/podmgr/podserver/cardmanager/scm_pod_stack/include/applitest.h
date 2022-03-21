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




#ifndef _APPLITEST_H
#define _AAPLITEST_H

typedef enum 
{                   // Card Modes
    POD_MODE_SCARD,   // The cablecard is a single stream card
    POD_MODE_MCARD    // The cablecard can handle multiple streams
    
} POD_MODE_t; 

#define CRC_ACCUM       0xFFFFFFFF

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned char transId;  // transport connection ID declared in ../user/applitest.c
extern unsigned char vl_gCardType; // card Type (s-card or m-card) declared in ../user/applitest.c
unsigned long Crc32(unsigned crcAccum, unsigned char *pData, int len);
unsigned long Crc32Mpeg2(unsigned crcAccum, unsigned char *pData, int len);

#ifdef __cplusplus
}
#endif

/*========================================================================

    Following defines and types must be completed

==========================================================================*/

#define RSC_RESOURCEMANAGER 0x00010041
#define RSC_APPINFO            0x00020041
#define RSC_APPINFO2        0x00020042
#define RSC_CASUPPORT        0x00030041
#define RSC_HOSTCONTROL        0x00200041
#define RSC_DATETIME        0x00240041
#define RSC_MMI                0x00400041
#define RSC_LOWSPEEDCOM        0x00600001
#define RSC_EXTENDEDCHANNEL 0x00A00042

// Resource manager APDUs
#define APDU_PROFILE_ENQ    0x009F8010
#define APDU_PROFILE_REPLY  0x009F8011
#define APDU_PROFILE_CHANGE 0x009F8012


// Extended Channel APDU
#define APDU_NEW_FLOW_REQUEST 0x009F8E00

#endif // _APPLITEST_H
