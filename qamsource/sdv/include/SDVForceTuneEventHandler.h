/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2014 RDK Management
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
* limitations under the License.i
* ============================================================================
* @file SDVForceTuneEventHandler.h
* @brief Event handler for SDV Agent Broadcast Events.
* @Author Spencer Gilroy (sgilroy@cisco.com)
* @Date Sept 11 2014
*/

/**
 * @defgroup SDV QAM Source - SDV
 * @ingroup QAM
 * @defgroup SDVClasses SDV Classes
 * Described the details about various classes used in this section and its functionalities
 * @ingroup SDV
 * @defgroup SDVdatastructures SDV Data Structures
 * @ingroup SDV
 * @defgroup SDVMacros SDV Macros
 * @ingroup SDV
 */

#ifndef SDVFORCETUNEEVENTHANDLER_H_
#define SDVFORCETUNEEVENTHANDLER_H_

#include <libIBus.h>
#include "sdv_iarm.h"
#include "rmfcore.h"
#include "rmf_error.h"

#include <map>

/**
 * @class SDVForceTuneEventHandler
 * @brief Handles IARM Broadcast Force Tune Events from the SDV Agent.
 * @ingroup SDVClasses
 */
class SDVForceTuneEventHandler {
public: 
    typedef void (*SDV_FORCE_TUNE_CALLBACK_t)(void * ptrObj, uint32_t sourceId);
    
    typedef struct sdv_force_tune_handler {
        void * ptrObj;
        SDV_FORCE_TUNE_CALLBACK_t callback;
    } SDV_FORCE_TUNE_HANDLER_t;

public:
    SDVForceTuneEventHandler();

    RMFResult start();

    RMFResult stop();

    RMFResult registerEventHandler(uint32_t tunerId, SDV_FORCE_TUNE_HANDLER_t);
 
    RMFResult unregisterEventHandler(uint32_t tunerId);

    ~SDVForceTuneEventHandler();

private:

    typedef std::map <uint32_t, SDV_FORCE_TUNE_HANDLER_t> FORCE_TUNE_HANDLER_MAP; 
    
    static FORCE_TUNE_HANDLER_MAP handlerMap;
    static pthread_mutex_t mapLock;
    
    /**
     * @fn handleBroadcastEvent
     * @brief which gets invoked when an event is broadcasted to the bus.
     *
     * @param [in] owner - owner of the event
     * @param [in] eventId - id of the received event
     * @param [in] data - pointer to event data
     * @param [in] len - size of event data
     */
    static IARM_Result_t handleBroadcastEvent(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
};

#endif /* SDVBROADCASTEVENTHANDLER_H_ */
