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
* @file SDVSectionFilterRegistrar.cpp
* @brief Implementation of SDVSectionFilterRegistrar.
* @Author Jason Bedard jasbedar@cisco.com
* @Date Sept 11 2014
*/
#include "include/SDVSectionFilterRegistrar.h"
#include "rdk_debug.h"

#define SDV_MCP_WAIT_TIMEOUT_MS 10000    //10 seconds

SDVSectionFilterRegistrar * SDVSectionFilterRegistrar::instance = NULL;

/**
 * @brief This function is used to get the singleton instance of SDVSectionFilterRegistrar.
 *
 * @return instance Pointer to singleton instance of SDVSectionFilterRegistrar is returned.
 */
SDVSectionFilterRegistrar * SDVSectionFilterRegistrar::getInstance(){
    if(SDVSectionFilterRegistrar::instance == NULL){
        SDVSectionFilterRegistrar::instance  = new SDVSectionFilterRegistrar();
    }
    return instance;
}

SDVSectionFilterRegistrar::SDVSectionFilterRegistrar(){
    rmf_Error err = rmf_osal_mutexNew(&m_mutex);
    if (err != RMF_SUCCESS){
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVSectionFilterRegistrar] Failed to create mutex : %d\n", err );
    }
}

/**
 * @brief This function creates an event queue and starts a section filter for MCP data
 * asynchronously and monitors it for MCP change events. It also updates the call back
 * function and call back instance which is used when MCP data is ready.
 *
 * @param[in] callbackInstance Pointer to the instance of the class whose callback function
 * needs to be invoked when MCP data is ready.
 * @param[in] callback function Pointer which gets invoked when MCP data is available.
 *
 * @return Return value indicates success or failure of the call.
 * @retval RMF_SUCCESS Indicates the section filter for MCP data was successfully created.
 * @retval RMF_OSAL_ENOMEM Indicates memory unavailability condition.
 * @retval RMF_OSAL_EINVAL Indicates invalid parameters passed to internally called functions.
 */
rmf_Error SDVSectionFilterRegistrar::startSectionFilter(SDVSectionFilter * sectionFilter){
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVSectionFilterRegistrar] startSectionFilter()\n");

    rmf_Error result = sectionFilter->start(this, sectionsReady);
    if (result != RMF_SUCCESS){
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "[SDVSectionFilterRegistrar] startSectionFilter()  Failed to start section filter : %d\n", result);
    }
    return result;
}

/**
 * @brief This function stops the section filerting operation for MCP data by releasing
 * the filter and deleting the queue.
 * <ul>
 * <li> It first sets the SDV section filter state to STOPPING and then releases the filter after terminating
 * the thread.
 * <li> It frees all the MCP sections received if we are stopping in middle of a version.
 * <li> It clears and deletes the queue, created to post section filtering events.
 * <li> Finally changes the state SDV section filter state to STOPPED
 * <li> Note:
 * <li> The section filtering will be stopped asyncrnously so it is possible a single event may
 * still be reviced even after stop() returns.
 * </ul>
 *
 * @param[in] sectionFilter Indicates the section filter to stop.
 *
 * @return None
 */
void SDVSectionFilterRegistrar::stopSectionFilter(SDVSectionFilter * sectionFilter){
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVSectionFilterRegistrar] stopSectionFilter()\n");
    sectionFilter->stop();
}

/**
 * @brief This function sets the callback which will be invoked when MCP data becomes available.
 *
 * @param[in] callbackInstance Pointer to the instance of the class which registered for this event.
 * @param[in] callback Function pointer to registered callback.
 *
 * @return None
 */
void SDVSectionFilterRegistrar::setSectionFilterListener(void * callbackInstance, SDVSectionFilter::SDV_SECTION_FILTER_CALLBACK_t callback){
    rmf_osal_mutexAcquire( m_mutex);
    this->m_callback = callback;
    this->m_callbackInstance = callbackInstance;
    rmf_osal_mutexRelease( m_mutex);
}

void SDVSectionFilterRegistrar::sectionsReady(SDVSectionFilter::SDV_SECTION_FILTER_EVENT_t * event){
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVSectionFilterRegistrar] sectionsReady() size = %d \n", event->data.size );

    SDVSectionFilterRegistrar * instance = ((SDVSectionFilterRegistrar*)event->callbackInstance);
    SDVSectionFilter::SDV_SECTION_FILTER_CALLBACK_t callback = NULL;

    rmf_osal_mutexAcquire( instance->m_mutex);
    if(instance->m_callback != NULL){
        event->callbackInstance = instance->m_callbackInstance;
        callback = instance->m_callback;
    }
    rmf_osal_mutexRelease( instance->m_mutex);

    if(callback != NULL){
        callback(event);
    }
}
