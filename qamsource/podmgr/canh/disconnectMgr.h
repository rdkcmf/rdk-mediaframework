/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2018 RDK Management
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
#ifndef _DISCONNECT_MGR_H_
#define _DISCONNECT_MGR_H_

#include <rmf_osal_event.h>

/* events */
#define RMF_PODMGR_EVENT_EID_DISCONNECTED 0
#define RMF_PODMGR_EVENT_EID_CONNECTED 1

rmf_Error disconnectMgrInit( );
rmf_Error disconnectMgrUnInit( );
rmf_osal_Bool disconnectMgrGetAuthStatus( );
rmf_Error disconnectMgrSubscribeEvents( rmf_osal_eventqueue_handle_t queueId );
rmf_Error disconnectMgrUnSubscribeEvents( rmf_osal_eventqueue_handle_t queueId );

#endif

