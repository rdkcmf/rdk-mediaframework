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



#ifndef _FEATURE_MAIN_H_
#define _FEATURE_MAIN_H_

#define MAX_OPEN_SESSIONS 1

// State machine
typedef enum _FCStateType {
    FC_UNINITIALIZED = 0,
    FC_INITIALIZED,
    FC_STARTED,
    FC_SESSION_OPEN_REQUESTED,
    FC_SESSION_OPENED,
    FC_DLG_OPEN_REQUESTED,
    FC_DLG_OPENED,
} FCStateType;

typedef struct _FCState {
    FCStateType state;
    int loop;
    int openSessions;
    unsigned short sessionId;
    pthread_mutex_t lock;
} FCState;

#endif //_FEATURE_MAIN_H_


