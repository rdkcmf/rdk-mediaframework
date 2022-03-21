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



#ifndef _PODHIGHAPI_H_
#define _PODHIGHAPI_H_

#ifdef __cplusplus
extern "C" {
#endif 

typedef enum 
{
    POD_INIT_OK,
    POD_NOT_INITIALIZED,
    READ_FAILURE,
    WRITE_FAILURE,
    CANNOT_CREATE_TC,
    PROCESS_TO_POD_FAILURE

}POD_STATUS_ERROR;


// Following are the prototypes of the functions that must be implemented 
// in specific Host implementation

void AM_TC_CREATED(UCHAR Tcid);

void AM_SS_OPENED(USHORT SessNumber,ULONG lResourceId, UCHAR Tcid);

void AM_SS_CLOSED(USHORT SessNumber);

void AM_TC_DELETED(UCHAR Tcid);

void AM_APDU_FROM_POD(USHORT SessNumber, UCHAR * pApdu, USHORT Length);

int AM_APDU_TO_POD(USHORT SessNumber, UCHAR * pApdu, USHORT Length);

void AM_POD_STATUS(POD_STATUS_ERROR Status);

void AM_EXTENDED_DATA( ULONG FlowID, UCHAR* pData, USHORT Size);


// Following are the prototypes of the functions that are called 
// from host specific implementation

void AM_OPEN_SS_REQ(ULONG RessId, UCHAR Tcid);

void AM_OPEN_SS_RSP( UCHAR Status, ULONG lResourceId, UCHAR Tcid);

void AM_APDU_FROM_HOST(USHORT SessNumber, UCHAR * pApdu, USHORT Length);

void AM_CLOSE_SS(USHORT SessNumber);

void AM_EXTENDED_WRITE(UCHAR Tcid, ULONG FlowID, UCHAR * pData, USHORT Size);

DWORD AM_INIT (void);

BOOL AM_EXIT (DWORD TaskID);

void AM_DELETE_TC( UCHAR Tcid);

/*========================================================================

    Implementations in Applitest.c of the following functions are given as a sample

==========================================================================*/

void AM_TC_CREATED(UCHAR Tcid);

void AM_OPEN_SS_REQ(ULONG RessId, UCHAR Tcid);

void AM_SS_OPENED(USHORT SessNumber, ULONG lResourceId, UCHAR Tcid);

void AM_APDU_FROM_POD(USHORT SessNumber, UCHAR * pApdu, USHORT Length);

void AM_SS_CLOSED(USHORT SessNumber);

void AM_TC_DELETED(UCHAR Tcid);

void AM_POD_STATUS(POD_STATUS_ERROR Status);

void AM_EXTENDED_DATA(  ULONG FlowID, UCHAR* pData, USHORT Size);

#ifdef __cplusplus
}
#endif 

#endif // _PODHIGHAPI_H_

