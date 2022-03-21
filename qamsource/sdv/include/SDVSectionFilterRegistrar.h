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
* limitations under the License.
* ============================================================================
* @file SDVSectionFilterRegistrar.h
*
* @brief Registration point for SDVSection filters and
* the listener whom is interested in the data from them
*
* @Author Jason Bedard jasbedar@cisco.com
* @Date Sept 11 2014
*/
#ifndef SDVSECTIONFILTERREGISTRAR_H_
#define SDVSECTIONFILTERREGISTRAR_H_

#include "SDVSectionFilter.h"
#include <cstdlib>
#include <string.h>

/**
 * @class SDVSectionFilterRegistrar
 * @ingroup SDVClasses
 * @brief Singleton used to start/stop and listen to events from SDVSectionFitler
 * objects
 *
 * When a QAMSource tunes to a transport which is known or expected to carry
 * SDV MMCIS Configuration information it will create a SDVSectionFilter and
 * instruct the SDVSectionFilterRegistrar to start the filter.
 *
 * The SDVSectionFilterRegistrar will listen for events from all SDVSectionFilter
 * Objects which it has been requested to start.  When an event is received it will
 * verify the data integrity and pass the information along to a registered listener.
 *
 * The Registered listener will receive events each time the MCP data is received from
 * a filter regardless if the data has changed.
 *
 * When a QAMSource is closed it is expected to make a call to stopSectionFilter after which
 * the SDVSectionFilterRegistrar still stop the SDVSectionFilter.
 */
class SDVSectionFilterRegistrar {

public:

    static SDVSectionFilterRegistrar * getInstance();

    void setSectionFilterListener(void * callbackInstance, SDVSectionFilter::SDV_SECTION_FILTER_CALLBACK_t callback);

    rmf_Error startSectionFilter(SDVSectionFilter * sectionFilter);

    void stopSectionFilter(SDVSectionFilter * sectionFilter);
private:
    SDVSectionFilterRegistrar();

    /**
     * @brief Section Filter callback Function
     *
     * callback function which gets invoked when MCP data is available.
     *
     * @param event the Section filter event containing MCP data
     */
    static void sectionsReady(SDVSectionFilter::SDV_SECTION_FILTER_EVENT_t * event);
private:
    static SDVSectionFilterRegistrar * instance;                //!<Singleton instance pointer
    void * m_callbackInstance;                                  //!<Pointer to the instance of the class which registered for this event
    SDVSectionFilter::SDV_SECTION_FILTER_CALLBACK_t m_callback; //!<Function pointer to registered callback.
    rmf_osal_Mutex m_mutex;                                     //!<Mutex to control read/write of registered listener
};
#endif /* SDVSECTIONFILTERREGISTRAR_H_ */
