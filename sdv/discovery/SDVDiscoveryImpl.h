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
 * @file SDVDiscoveryImpl.h *
 * @brief Implementation class of SDVDiscovery interface *
 * @Author Jason Bedard jasbedar@cisco.com
 * @Date Sept 11 2014
 */
#ifndef SDVDISCOVERY_IMPL_H_
#define SDVDISCOVERY_IMPL_H_

#include <stdio.h>
#include <cstddef>
#include "sdv_iarm.h"
#include "libIBus.h"
#include <vector>
#include <stdint.h>
#include <map>

#include "SDVDiscovery.h"
#include "SDVSectionFilter.h"
#include "rmfcore.h"
#include "rmf_osal_thread.h"

/**
 * @defgroup SDVMacros SDV Macros
 */

#define SDV_MCP_TUNE_TIMEOUT_MS 2000                //!< Timeout for getting MCP on each tune

#define MAX_CONFIGURABLE_DISCOVERY_TIME_MS 11000  //!< Maximum configurable timeout for autodiscovery is millis

#define MAX_DISCOVERY_TIME_OFFSET 8          //!< Offset into SDV Config file where max sacn time in seconds can be found
#define SCAN_COUNT_OFFSET 13                 //!< Offset into SDV Config file where scan list count can be found


/**
 * @class SDVDiscoveryImpl
 * @brief Implementation of the SDVDiscovery interface
 */
class SDVDiscoveryImpl : public SDVDiscovery{
public:
    SDVDiscoveryImpl();

    void start();

    void attemptDiscoveryIfNeeded(RMFQAMSrcImpl * qamsource);

    rmf_Error isMCPDataDiscovered();

private:

    static uint32_t getFrequency(uint32_t unionVal);
    static uint32_t getModulation(uint32_t unionVal);

    /**
     * @fn callbackRequestFreshMCData
     * @brief Callback function for when SDVAgent signals it is in need of fresh MCP data.
     *
     * When this is invoked it is assumed that the SDVAgent is requesting
     * MCP data as a result of an error condition suspected to be because the MC Data has changed
     * since the last time it was obtained.  As a result cached MCP data will be cleared and
     * SDVDiscoveryImpl will move to a state where tune time discovery is needed on every tune
     * until MCP data can be re-acquired.
     *
     * @param owner is well-known name of the application sending the event.
     * @param eventID is the integer uniquely identifying the event within the sending application.
     * @param data is the composite carrying all input and output parameters of event.
     * @param len is the result of sizeof() applied on the event data data structure.
     */
    static void callbackRequestFreshMCData(const char *owner, IARM_EventId_t eventId, void *data, size_t len);

    /**
     * @fn callbackRequestLatestMCData
     * @brief Callback function for when SDVAgent signals it is in need of MCP data.
     *
     * When this is invoked it is assumed that the SDVAgent is requesting
     * MCP data as part of its boot up sequence. As a result any cached MCP
     * data will be sent over IARM to the Agent immediately.
     *
     * If No MCP data is yet available  SDVDiscoveryImpl will move to a state
     * where tune time discovery is needed on every tune until MCP data can be
     * re-acquired.
     *
     * @param owner is well-known name of the application sending the event.
     * @param eventID is the integer uniquely identifying the event within the sending application.
     * @param data is the composite carrying all input and output parameters of event.
     * @param len is the result of sizeof() applied on the event data data structure.
     */
    static void callbackRequestLatestMCData(const char *owner, IARM_EventId_t eventId, void *data, size_t len);

    /**
     * @fn sectionFilterEvent
     * @brief Section Filter callback Function
     *
     * Function which gets invoked when MCP data is available.
     *
     * @param event the Section filter event containing MCP data
     */
    static void sectionFilterEvent(SDVSectionFilter::SDV_SECTION_FILTER_EVENT_t * event);

    /**
     * @fn discoveryFileUpdated
     *
     * @brief Static callback for when the autodiscovery file has been
     * downloaded or has changed.
     *
     * @param [in] ptrObj NULL
     * @param [in] data binary data of the Autodiscovery configuration file
     * @param [in] size of configutation file in bytes
     *
     * @return RMF_SUCCESS if file is well formed. RMF_OSAL_ENODATA if file is corrupted
     */
    static rmf_Error discoveryFileUpdated(void * ptrObj, uint8_t *data, uint32_t size );


    /**
     * @fn ipv6AddressFileUpdated
     *
     * @brief Static callback for when the ipv6 Address file has been
     * downloaded or has changed.  This file contains a mapping of IPV4-IPV6 addresses
     * that can be used if the device only has access to MCMIS-2.1 but it running
     * on an IPV6 network.
     *
     * File is expected to be in the following format
     *
     * <AddressMap creationDate="2016-02-09T14:44:18.456Z" schemaVersion="1.0" controllerId="2509">
     *  <Address ipv4="10.8.155.119" ipv6="2001:578:30:562a:10:8:155:119"/>
     *  <Address ipv4="10.8.115.101" ipv6="2001:578:30:54b5:10:8:115:101"/>
     * </AddressMap>
     *
     * @param [in] ptrObj NULL
     * @param [in] data binary data of the Autodiscovery configuration file
     * @param [in] size of configuration file in bytes
     *
     * @return RMF_SUCCESS if file is well formed. RMF_OSAL_ENODATA if file is corrupted
     */
    static rmf_Error ipv6AddressFileUpdated(void * ptrObj, uint8_t *data, uint32_t size );

    /**
     * @fn exhaustiveDiscovery
     * @brief Performs an exhaustive MCP discovery.
     * @details Scans through as many locators as possible within SDV_DISCOVERY_TIMEOUT_MS.  Function will block
     * until an event is received.
     *
     * @param qamsource the qamsource to use for all the autodiscovery tunes.
     * The QAMSource is expected  to be in a closed state and will be returned
     * to a close state before returning.
     */
    RMFResult exhaustiveDiscovery(RMFQAMSrcImpl * qamsource);

    /**
     * @fn tuneAndDiscover
     * @brief Tune to a specific locator and wait for MCP Data
     *
     * Synchronously will tune to the given locator and wait for the next
     * MCP data event or until a timeout occurs
     *
     * @param qamsource the QAMSource to use for the autodiscovery tunes.
     * The QAMSource is expected  to be in a closed state and will be returned
     * to a close state before returning.
     */
    RMFResult tuneAndDiscover(RMFQAMSrcImpl * qamsource, const char * locator, uint32_t timeout);

    /**
     * @fn invokeMCPAvailableIARM
     * @brief Invoke IARM to deliver MCP data to SDVAgent.
     *
     * Send cached MCP data to the SDVAgent over IARM
     */
    void invokeMCPAvailableIARM();

private:
    std::vector<std::string> discoveryURIs; 	//!< Container for URIs to tune for discovering MC configuration file.
    uint16_t m_max_scanTime;                    //!< Value from configuration for for max time in milliseconds.
    rmf_osal_Cond m_waitForMCPCondition;    	//!< Condition used to wait for MCP events.
    uint8_t m_uriIndex;                        //!< Index into discovery URIs to determine which one to use on the next tune time discovery attempt.

    SDVSectionFilter::SDV_MCP_DATA* m_mcp_data;         //!< Pointer to the last MCP data known to SDVDiscovery.
    uint16_t m_ipv6AddressMapCount;                      //!< The number of IPV6 Address Map elements
    SDVAGENT_IPV6_MAP_ELEMENT * m_ipv6AddressMap;       //!< Array of SDVAGENT_IPV6_MAP_ELEMENT elements.

    rmf_osal_Mutex					m_dataMutex;
    rmf_osal_Mutex                  m_scanListMutex;
    bool                            m_triggerNewDiscovery;  //!< Flag to indicate MCP data is needed by SDVAgent.

};
#endif /* SDVDISCOVERY_IMPL_H_ */
