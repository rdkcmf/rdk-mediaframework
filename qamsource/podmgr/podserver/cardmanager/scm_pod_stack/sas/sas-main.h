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



#ifndef _SAS_MAIN_H_
#define _SAS_MAIN_H_

#include <utils.h>

    // Specific Application Support APDU tags - as shown in SCTE-28 2003 section 8.11.3
#define SAS_CONNECT_RQST          0x9f9a00
#define SAS_CONNECT_CNF           0x9f9a01
#define SAS_DATA_RQST             0x9f9a02
#define SAS_DATA_AV               0x9f9a03
#define SAS_DATA_CNF              0x9f9a04
#define SAS_SERVER_QUERY          0x9f9a05
#define SAS_SERVER_REPLY          0x9f9a06

#define MAX_OPEN_SESSIONS       ( 32 )      // for Specific Appl Support


// State machine
typedef enum
{
    SAS_UNINITIALIZED = 0,
    SAS_INITIALIZED,
    SAS_STARTED,
    SAS_SHUTDOWN
} sasState_t;

typedef struct
{
    sasState_t      state;  // SAS state
    int             loop;   // controls thread execution (when 0, thread will exit)
    pthread_mutex_t lock;   // protects contents of this structure
} sasControl_t;

#endif //_SAS_MAIN_H_

