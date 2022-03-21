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



#ifndef _AI_STATE_INT_H_
#define _AI_STATE_INT_H_

#ifdef __cplusplus
extern "C" {
#endif 

typedef enum _AIStateType {
    AI_UNINITIALIZED = 0,
    AI_INITIALIZED,
    AI_STARTED,
    AI_SESSION_OPEN_REQUESTED,
    AI_SESSION_OPENED,
    AI_APPINFO_SENT,
    AI_APPINFO_CONFIRMED,
    AI_EXIT
} AIStateType;

#ifdef _AI_STATE_H_
int aiChangeState(AIStateType);
#else
extern int aiChangeState(AIStateType);
extern Bool aiExit();
extern uint16 aiSessionId();
extern Bool aiSessionOpen();
extern void aiSetSessionId(uint16 sid);
extern uint8* aiInfo(uint16 *len);
extern void aiSetInfo(uint8 *data, uint16 len);
extern AIStateType aiGetState();
#endif


#ifdef __cplusplus
}
#endif 

#endif // _AI_STATE_INT_H_
