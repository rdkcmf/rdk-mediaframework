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

/**
 * @defgroup BOOT_EVNT Boot Event Handle
 * @ingroup SNMP_MGR
 * @ingroup BOOT_EVNT
 * @{
 */

/**
* \file vlSnmpBootEventHandler.h
*/

#ifndef VL_SNMP_BOOT_EVENT_HANDLER_H
#define VL_SNMP_BOOT_EVENT_HANDLER_H

#include "cmThreadBase.h"

#include "dsg_types.h" //TVL parse
#include "bufParseUtilities.h"//TLV217 parsing
#include <vector>
#include <string>
#include "utilityMacros.h"
#include "ip_types.h"
#include "rmf_osal_event.h"

using namespace std;

class CvlSnmpBootEventHandler:public CMThread
{
public:
    //! Constructor.
    CvlSnmpBootEventHandler();
    ~CvlSnmpBootEventHandler();
    int is_default();
    int initialize(void);
    int init_complete(void );
    //! Main thread routine. Pulls events from the #event_queue and dispatches them in #epgm.
    void run();
private:
    rmf_osal_eventqueue_handle_t m_pEvent_queue; //!< Event queue to listen to. Set by constructor.
};

#endif//VL_SNMP_BOOT_EVENT_HANDLER_H

/** @} */
