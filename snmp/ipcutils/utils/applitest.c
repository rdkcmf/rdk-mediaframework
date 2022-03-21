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


#include "applitest.h"

#if USE_SYSRES_MLT
#include "mlt_malloc.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif 

#define POLYNOMIAL        0x04c11db7        // for Crc32
#define    CRC_TABLE_SIZE    256

static unsigned    guiCrcTable[CRC_TABLE_SIZE];

void GenerateCrcTable()
{
    register unsigned    crcAccum;
    register int    i, j; 
    
    for (i = 0; i < CRC_TABLE_SIZE; i++) 
    {
        crcAccum = (unsigned)i << 24;
        for (j = 0; j < 8; j++) 
        {
            if (crcAccum & 0x80000000)
            {
                crcAccum <<= 1;
                crcAccum  = (crcAccum ) ^ POLYNOMIAL;
            }
            else
               // crcAccum  = (crcAccum <<= 1);
             crcAccum <<= 1;
        }
        guiCrcTable[i] = crcAccum;
    }
}
/* ----------------------------------------------------------------------------
 *    Calculate the CRC32 on the data block one byte at a time.
 *    Accumulation is useful when computing over all ATM cells in an AAL5 packet.
 *    NOTE: The initial value of crcAccum must be 0xffffffff
 * ----------------------------------------------------------------------------
 */
unsigned long Crc32(unsigned crcAccum, unsigned char *pData, int len)
{
   static unsigned    long tableDone = 0;

    register int i, j;

    if ( !tableDone) 
    {
        GenerateCrcTable();
        tableDone = 1;
    }

    for (j = 0; j < len; j++ ) 
    {
        i = ((int)(crcAccum >> 24) ^ *pData++) & 0xff;
        crcAccum = (crcAccum << 8) ^ guiCrcTable[i];
    }
    return ~crcAccum;
}

/* ----------------------------------------------------------------------------
 *    Calculate the CRC32 on one single ATM cell's data block, one byte at a time.
 * ----------------------------------------------------------------------------
 */
unsigned Crc32Atm(unsigned char *pData)
{
    // ATM header not included in data block
    return Crc32(0xffffffff, pData, 44);
}


#ifdef __cplusplus
}
#endif 

