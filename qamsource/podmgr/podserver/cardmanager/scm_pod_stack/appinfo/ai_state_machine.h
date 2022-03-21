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



#ifndef _AI_STATE_H_
#define _AI_STATE_H_

#define MAX_OPEN_SESSIONS 1

#include <cardmanagergentypes.h>
#include <pthread.h>

#include <ai_state_machine_int.h>

#ifdef __cplusplus
extern "C" {
#endif 

typedef  int (*StateMgrType)(AIStateType);

typedef struct _AIState {
    AIStateType state;
    StateMgrType stateMgr;

    Bool exit;
    Bool sessionOpen;
    uint16 sessionId;
    uint8 *aInfo;
    uint16 aInfo_len;
    pthread_mutex_t lock;
} AIState;

static int StateMgr_Uninitialized (AIStateType newState);
static int StateMgr_Initialized (AIStateType newState);
static int StateMgr_Started (AIStateType newState);
static int StateMgr_SessOpenRequested (AIStateType newState);
static int StateMgr_SessOpened (AIStateType newState);
static int StateMgr_AppInfoSent (AIStateType newState);
static int StateMgr_AppInfoConfirmed (AIStateType newState);


#ifdef __cplusplus
}
#endif 

#endif //_AI_STATE_H_
