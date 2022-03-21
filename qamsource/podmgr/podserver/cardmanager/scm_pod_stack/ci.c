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



#include <stdio.h>

#include "global_event_map.h"

#include "utils.h"
//#include "cis.h"
//#include "cimax.h"
//#include "module.h"
//#include "link.h"
#include "transport.h"
#include "session.h"
//#include "ci.h"
//#include "podlowapi.h"
//#include "i2c.h"
#include "podhighapi.h"
#include "appmgr.h"
#include "poddrv.h"

#ifdef __cplusplus
extern "C" {
#endif 


//UCHAR           bModuleState = MODULE_STATE_OUT;

#define MODULE_OUT_EVENT 0
#define MODULE_IN_EVENT  1

/* This is an arbitrary number (that we hope no one else uses).  
** If shell receives bogus event # it will simply print all parameters
** that came with this event.  We will use this arbitrary number
** so that we will know it came from us (the POD stack) */
#define POD_PRINT_SHELL_EVENT -65

static UCHAR PODAttemptClearRWErrors (void);

static BOOL bPodCheckEnabled_DEBUG = TRUE;

BOOL bHomingInProgress = FALSE;

/****************************************************************************************
Name:             ByteStreamGetLongWord
type:            function 
Description:     Utility function for performing conversion from UCHAR * to ULONG 

In:             UCHAR *stream : pointer on incoming stream
                
Data 
Out:            Nothing
Return value:    ULONG : The decoded ULONG 
*****************************************************************************************/
ULONG ByteStreamGetLongWord(const UCHAR *stream)
{
    return ((((ULONG)stream[0])<<24)|(((ULONG)stream[1])<<16)|(((ULONG)stream[2])<<8)|stream[3]);
}

/****************************************************************************************
Name:             ByteStreamPutLongWord
type:            function 
Description:     Conversion from ULONG to UCHAR * (4 bytes) 

In:             UCHAR *stream : pointer on outgoing data
                ULONG data    : The ULONG to translate
Data 
Out:            Nothing
Return value:    void
*****************************************************************************************/
void ByteStreamPutLongWord(UCHAR *stream, ULONG data)
{
    stream[0] = (UCHAR)(data>>24);
    stream[1] = (UCHAR)(data>>16);
    stream[2] = (UCHAR)(data>>8);
    stream[3] = (UCHAR)data;
}



/****************************************************************************************
Name:             ByteStreamGetWord
type:            function 
Description:     Conversion from UCHAR* to USHORT

In:             UCHAR *stream : pointer on the incoming data
                
Out:            Nothing
Return value:    USHORT result
*****************************************************************************************/
USHORT ByteStreamGetWord(const UCHAR *stream)
{
    return ((((USHORT)stream[0])<<8)|stream[1]);
}

/****************************************************************************************
Name:             ByteStreamGetWord24
type:            function 
Description:     Conversion from UCHAR* (3bytes) to a 24 bytes ULONG

In:             UCHAR *stream : pointer on the incoming data
                
Out:            Nothing
Return value:    USHORT result
*****************************************************************************************/
ULONG ByteStreamGetWord24(const UCHAR *stream)
{
    return ((((ULONG)stream[0])<<16)|(((ULONG)stream[1])<<8)|stream[2]);
}


/****************************************************************************************
Name:             ByteStreamPutWord
type:            function 
Description:     Conversion from USHORT to an UCHAR *

In:             UCHAR *stream : outgoing UCHAR*
                USHORT data : incoming USHORT
                
Out:            Nothing
Return value:    void
*****************************************************************************************/
void ByteStreamPutWord(UCHAR *stream, USHORT data)
{
    stream[0] = (UCHAR)(data>>8);
    stream[1] = (UCHAR)data;
}

/****************************************************************************************
Name:             ByteStreamPutWord24
type:            function 
Description:     Conversion from a 24 bytes ULONG to an UCHAR* (3bytes)

In:             UCHAR *stream : pointer on the incoming data
                
Out:            Nothing
Return value:    void
*****************************************************************************************/
void ByteStreamPutWord24(UCHAR *stream, ULONG data)
{
    stream[0] = (UCHAR)(data>>16);
    stream[1] = (UCHAR)(data>>8);
    stream[2] = (UCHAR)data;
}


/****************************************************************************************
Name:             ci_SetLength
type:            function 
Description:     Translate lSize in ASN1 code type and put it in pData

In:             UCHAR *pData : where is coded Size
                ULONG lSize : Size to code
                
Out:            Nothing
Return value:    UCHAR : Size of the size
*****************************************************************************************/
UCHAR ci_SetLength (UCHAR *pData, ULONG lSize)
{
UCHAR bSizeLength;
  if ( lSize < 0x00000080 )
  {
    pData[0] = (UCHAR)lSize;
    bSizeLength = 1;
  }
  else
  {
    /* Size indicator + length field size */
    pData[0]    = 0x80 + 2;

    /* ASN.1 Length field size (including size indicator) */
    bSizeLength = 1+2;
    ByteStreamPutWord (pData+1, (USHORT)lSize);
  }

  return bSizeLength;
}

/****************************************************************************************
Name:             ci_GetLength
type:            function 
Description:     Get the length (coded in ASN 1) 

In:             UCHAR *pData : pointer on the place where size code begins
                USHORT *plSize : The retrieved size
Data 
Out:            Nothing
Return value:    UCHAR Size of the size

modified:       03/25/04 - MLP -- Fixed TLV length computation

*****************************************************************************************/
UCHAR ci_GetLength (UCHAR *pData, USHORT *plSize)
{
    UCHAR i,bSizeLength;
    USHORT lSize = 0;

    if ( (pData[0] & 0x80) == 0 ) /* If == x80 then size is coded in 1 byte */
    {
        bSizeLength = 1;    
        lSize = (pData[0] & ~0x80); 
    }
    else /* Length is coded in more then 1 byte */
    {
        bSizeLength = pData[0] & ~0x80; /* Len is in low 7 bits */
            
        for (i=0;i<bSizeLength;i++)
        {            
            /* Add the size bytes together to compute final data size */
            lSize += (pData[1+i] << (8*(bSizeLength-1 -i)));
        }
        bSizeLength++;        // Don't forget to count the first byte
    }

    if ( plSize != NULL )
        *plSize = lSize;
    return bSizeLength;
}


#ifdef __cplusplus
}
#endif 



