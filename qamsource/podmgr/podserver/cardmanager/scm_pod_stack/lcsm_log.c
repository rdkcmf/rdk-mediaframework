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



#include <global_queues.h>
#include <global_event_map.h>
//#include <lcsm_event.h>
#include "poddef.h"
#include <lcsm_log.h>

// Add file name and revision tag to object code
#define REVISION  __FILE__ " $Revision: 1.2 $"
static char *lcsm_log_tag= REVISION;

int lcsm_send_log(LogInfo *logEvent)
{
    LCSM_EVENT_INFO event;

    event.DstQueue = SD_MAIN_CONTROL_QUEUE;
    event.dataLength = sizeof(LogInfo);
    event.dataPtr = logEvent;
    event.event = LOG_REQUEST;

    return lcsm_send_event(0,SD_MAIN_CONTROL_QUEUE, &event); //0 added by Hannah
}
