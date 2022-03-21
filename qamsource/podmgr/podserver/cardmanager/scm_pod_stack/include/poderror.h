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



#ifndef _PODERROR_H_
#define _PODERROR_H_

#ifdef __cplusplus
extern "C" {
#endif 

    // values taken directly from SCTE-28 2003 App E Table E.1-A
#define PODERR_NO_READY          1  // READY signal does not go active
#define PODERR_BAD_CIS           2  // CIS values are incorrect
#define PODERR_DATA_RS_TIMEOUT   4  // POD fails to respond to RS bit on data ch
#define PODERR_EXT_RS_TIMEOUT    5  // POD fails to repsond to RS bit on ext ch
#define PODERR_DATA_BUFSZ        6  // POD negotiates data buffer too small
#define PODERR_EXT_BUFSZ         8  // POD negotiates ext buffer too small
#define PODERR_XPORT_TIMEOUT    10  // POD fails to respond to transport open
#define PODERR_PROF_TIMEOUT     17  // POD fails to respond to Profile_Inq
#define PODERR_CA_TIMEOUT       38  // POD fails to respond to CA_Inq
#define PODERR_WRITE_ERROR      51  // Write error occurs after transfer host to pod
#define PODERR_READ_ERROR       52  // Read error occurs after transfer host to pod
#define PODERR_POD_TIMEOUT      53  // POD fails to respond to any request in 5 seconds

    // hook into UI display
void reportPodError( int error );       // in pod/user/utils.c
void vlReportPodError(int error);

#ifdef __cplusplus
}
#endif 

#endif // _PODERROR_H_

