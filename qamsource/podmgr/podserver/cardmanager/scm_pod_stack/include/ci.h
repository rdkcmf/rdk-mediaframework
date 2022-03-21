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


#ifndef __CI_H
#define __CI_H

#ifdef __cplusplus
extern "C" {
#endif 

#define CI_MAXSIZE_LENGTH_FIELD 5 /* 1 byte for size indicator and length field size
                                   * 4 byte (long) for the length value */

UCHAR   ci_SetLength (UCHAR *pData, ULONG lSize);
UCHAR   ci_GetLength (UCHAR *pData, USHORT *plSize);

void    ByteStreamPutWord24(UCHAR *stream, ULONG data);
ULONG   ByteStreamGetWord24(const UCHAR *stream);
USHORT  ByteStreamGetWord(const UCHAR *stream);
void    ByteStreamPutWord(UCHAR *stream, USHORT data);
ULONG   ByteStreamGetLongWord(const UCHAR *stream);
void    ByteStreamPutLongWord(UCHAR *stream, ULONG data);

/* Change the POD stack state */
void POD_ChangeState ( UCHAR bState );

/* Let homing control some operations */
void CIResetPCMCIA (void);
void CIResetPod (void);
void CITransTimeoutCtrl (BOOL bDisableTimeout);
void CIHomingStateChange (BOOL bHomingInProgress); // == TRUE if started, == FALSE if ended

/* Means homing currently in progress if TRUE */
extern BOOL bHomingInProgress;

/************************************************************/

typedef enum
{
    UNKNOWN_POD  = 0,
    DK_HPNX_POD  = 1,
    SA_POD       = 2,
    MOT_POD      = 3,
    NDS_POD      = 4,
    FLASH_CARD   = 10
} ePodManufacturer_t;
extern ePodManufacturer_t bPodManufacturer;


#ifdef __cplusplus
}
#endif 

#endif // __CI_H
