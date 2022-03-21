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
 * @file SDVDiscovery.cpp
 * @brief Implementation of SDVDiscovery
 * @Author Jason Bedard jasbedar@cisco.com
 * @Date Sept 11 2014
 */
#include "SDVDiscovery.h"
#include "SDVDiscoveryImpl.h"
#include "libIBus.h"
#include "SDVSectionFilterRegistrar.h"
#include "SDVConfigObtainer.h"

#include <stdio.h>
#include <sstream>
#include <map>

#include <netinet/in.h>
#include <arpa/inet.h>

#include "rdk_debug.h"
#include "tinyxml.h"


static SDVDiscoveryImpl * instance = NULL;

/**
 * @ingroup sdvapi
 * @fn SDVDiscovery::getInstance()
 * @brief This API is used to get the singleton instance of SDVDiscoveryImpl. It creates and initialises the
 * SDVDiscoveryImpl object if not already created.
 *
 * @return instance Pointer to the SDVDiscoveryImpl instance.
 */
SDVDiscovery * SDVDiscovery::getInstance(){
    if(instance == NULL){
        instance = new SDVDiscoveryImpl();
    }
    return instance;
}

SDVDiscoveryImpl::SDVDiscoveryImpl() {
    rmf_osal_condNew(TRUE, FALSE, &m_waitForMCPCondition);
    rmf_osal_mutexNew(&m_dataMutex);
    rmf_osal_mutexNew(&m_scanListMutex);

    m_uriIndex = 0;
    m_triggerNewDiscovery = true;  // trigger autodiscovery on very 1st sdv tune
    m_mcp_data = NULL;
    m_ipv6AddressMapCount = 0;
    m_ipv6AddressMap = NULL;
}

/**
 * @ingroup sdvapi
 * @fn SDVDiscoveryImpl::start()
 * @brief This API is used to start the SDVDiscovery service.
 * <ul>
 * <li> It Registers an event handler with IARM bus for SDVAGENT_REQUEST_FRESH_MC_DATA and SDVAGENT_REQUEST_LATEST_MC_DATA
 * events, indicating MCP data is needed.
 * <li> Registers an observer for SDVDiscovery configuration file.
 * <li> Registers a callback function for section filter which shall be invoked when MCP data becomes available.
 * <li> If ENABLE_SDV_STARTUP_AUTODISCOVERY flag is defined then it performs an exhaustive MCP discovery which scans as
 * many locators as possible with in the timeout period untill the MCP data is available.
 *
 * @return None
 */
void SDVDiscoveryImpl::start(){
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVDiscovery] start()\n" );
    IARM_Result_t result = IARM_Bus_RegisterEventHandler(IARM_BUS_SDVAGENT_NAME, SDVAGENT_REQUEST_FRESH_MC_DATA, callbackRequestFreshMCData);
    if(result != IARM_RESULT_SUCCESS){
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "[SDVDiscovery] Failed to register SDVAGENT_REQUEST_FRESH_MC_DATA %d\n",result  );
    }
    result = IARM_Bus_RegisterEventHandler(IARM_BUS_SDVAGENT_NAME, SDVAGENT_REQUEST_LATEST_MC_DATA, callbackRequestLatestMCData);
    if(result != IARM_RESULT_SUCCESS){
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "[SDVDiscovery] Failed to register SDVAGENT_REQUEST_LATEST_MC_DATA %d\n",result  );
    }
    SDVConfigObtainer::getInstance()->setObserver(SDVConfigObtainer::SDVDiscovery, this, discoveryFileUpdated);
    SDVConfigObtainer::getInstance()->setObserver(SDVConfigObtainer::SDVIpv6AddressMap, this, ipv6AddressFileUpdated);
    SDVSectionFilterRegistrar::getInstance()->setSectionFilterListener(this, sectionFilterEvent);
#ifdef ENABLE_SDV_STARTUP_AUTODISCOVERY
    exhaustiveDiscovery();
#endif
}

/// For debug time stamping
uint64_t getNowTimeInMillis() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((uint64_t)tv.tv_sec * 1000) + ((uint64_t)tv.tv_usec / 1000);
}

uint32_t SDVDiscoveryImpl::getFrequency(uint32_t unionVal){
    return (uint32_t)((unionVal & 0x3FFFFFFF) * 1000);
}

uint32_t SDVDiscoveryImpl::getModulation(uint32_t unionVal){
    uint32_t bits = ((unionVal >> 30) & 0x3);
    switch(bits){
    case 0: //64
        return 8;
    case 1: //256
        return 16;
    case 2: //1024
        return 24;
    }
    return -1;
}

RMFResult SDVDiscoveryImpl::exhaustiveDiscovery(RMFQAMSrcImpl * qamsource) {
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVDiscovery] exhaustiveDiscovery() begin\n" );

	uint64_t startTime = getNowTimeInMillis();

	int discoveryTimeRemaining = m_max_scanTime;

    RMFResult result = RMF_RESULT_SUCCESS;
    discoveryTimeRemaining -= SDV_MCP_TUNE_TIMEOUT_MS;

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVDiscovery] exhaustiveDiscovery() discoveryTimeRemaining = %d\n", discoveryTimeRemaining);
    while (discoveryTimeRemaining > 0) {
        const char * locator = discoveryURIs.at(m_uriIndex).c_str();
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVDiscovery] exhaustiveDiscovery() attempting discovery; uriIndex=%d locator=%s\n", m_uriIndex, locator);
        result = tuneAndDiscover(qamsource, locator, SDV_MCP_TUNE_TIMEOUT_MS);
        if(result == RMF_SUCCESS) {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVDiscovery] exhaustiveDiscovery() MCP Data discovered; uriIndex=%d locator=%s\n", m_uriIndex, locator);
            break;
        }
        RDK_LOG(RDK_LOG_WARN, "LOG.RDK.QAMSRC", "[SDVDiscovery] exhaustiveDiscovery() MCP Data not found; uriIndex=%d locator=%s\n", m_uriIndex, locator);
        if (++m_uriIndex >= discoveryURIs.size()) {
            m_uriIndex = 0;
        }
        discoveryTimeRemaining -= SDV_MCP_TUNE_TIMEOUT_MS;
    }
    rmf_osal_condUnset(m_waitForMCPCondition);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVDiscovery] exhaustiveDiscovery() completed in %dms\n", (getNowTimeInMillis() - startTime));
    return result;
}

/**
 * @ingroup sdvapi
 * @fn SDVDiscoveryImpl::attemptDiscoveryIfNeeded(RMFQAMSrcImpl * qamsource)
 * @brief This API is used to perform the exhaustive discovery of MCP data by scanning as many locators as possible within the
 * maximum scan time defined.
 *
 * @param[in] qamsource Pointer to RMFQAMSrcImpl instance, used for all auto-discovery tunes.
 *
 * @return None
 */
void SDVDiscoveryImpl::attemptDiscoveryIfNeeded(RMFQAMSrcImpl * qamsource) {
    rmf_osal_mutexAcquire(instance->m_scanListMutex);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVDiscovery] attemptDiscoveryIfNeeded() triggerNewDiscovery=%d\n", m_triggerNewDiscovery);
	if (m_triggerNewDiscovery && !discoveryURIs.empty()) {
		exhaustiveDiscovery(qamsource);
    }
	rmf_osal_mutexRelease(instance->m_scanListMutex);
}

/**
 * @ingroup sdvapi
 * @fn SDVDiscoveryImpl::isMCPDataDiscovered()
 * @brief This API is used to check if MCP data is discovered or not.
 *
 * @return Returns RMF_SUCCESS if MCP data is available else returns RMF_OSAL_ENODATA.
 */
rmf_Error SDVDiscoveryImpl::isMCPDataDiscovered(){
    return (instance->m_mcp_data != NULL)?RMF_SUCCESS:RMF_OSAL_ENODATA;
}

RMFResult SDVDiscoveryImpl::tuneAndDiscover(RMFQAMSrcImpl * qamsource, const char * locator, uint32_t timeout){
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVDiscovery] tuneAndDiscover() create QAMSource with locator %s\n", locator);

    RMFResult result = qamsource->open(locator, NULL);
    if( result != RMF_SUCCESS ){
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "[SDVDiscovery] tuneAndDiscover() Source open failed: %d\n", result );
        return RMF_RESULT_FAILURE;
    }
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVDiscovery] tuneAndDiscover() wait %dms for MCP event\n", timeout);
    result = rmf_osal_condWaitFor(m_waitForMCPCondition, timeout);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVDiscovery] tuneAndDiscover() done waiting for MCP event; result=%d\n", result);

    qamsource->pause();
    qamsource->close();
    rmf_osal_condUnset(m_waitForMCPCondition);
    return result;
}
rmf_Error SDVDiscoveryImpl::discoveryFileUpdated(void * ptrObj, uint8_t *data, uint32_t size){
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVDiscovery] discoveryFileUpdated()!\n");
    if(size < SCAN_COUNT_OFFSET +1){
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "[SDVDiscovery] discovery file is too small()!\n");
        return RMF_OSAL_ENODATA;
    }

    uint8_t scan_element_count = data[SCAN_COUNT_OFFSET];
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVDiscovery] discoveryFileUpdated() file size = %d scan_element_count = %d\n", size, scan_element_count);
    if(size !=  SCAN_COUNT_OFFSET + 1 + (scan_element_count * sizeof(uint32_t))){
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "[SDVDiscovery] discovery file is corrupted!\n");
        return RMF_OSAL_ENODATA;
    }


    uint16_t parsed_max_sanTime = (ntohs( *((uint16_t*)&data[MAX_DISCOVERY_TIME_OFFSET]))) * 1000;
    instance->m_max_scanTime  = std::min<uint16_t>(parsed_max_sanTime, MAX_CONFIGURABLE_DISCOVERY_TIME_MS);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVDiscovery] discoveryFileUpdated() max_frequency_scan_time = %d\n", instance->m_max_scanTime);

    rmf_osal_mutexAcquire(instance->m_scanListMutex);

    instance->discoveryURIs.clear();
    instance->m_uriIndex = 0;

    uint32_t * scan_list_ptr = (uint32_t *)&data[SCAN_COUNT_OFFSET +1];
    for(int i=0;i!=scan_element_count;++i){
        uint32_t element = ntohl(scan_list_ptr[i]);
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVDiscovery] discoveryFileUpdated() \tscan_element[%d]= %zu!\n", i , element);
        uint32_t frequency = getFrequency(element);
        uint32_t modulation = getModulation(element);
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVDiscovery] discoveryFileUpdated() \tscan_element[%d] frequency = %zu!\n", i , frequency);
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVDiscovery] discoveryFileUpdated() \tscan_element[%d] modulation = %zu!\n", i , modulation);

        std::stringstream buffer;
        buffer<<"tune://rf?frequency="<<frequency<<"&modulation="<<modulation<<"&sdv=true";
        instance->discoveryURIs.push_back(buffer.str());
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVDiscovery] \tdiscoveryFileUpdated() url = %s\n",buffer.str().c_str());
    }
    rmf_osal_mutexRelease(instance->m_scanListMutex);
    return RMF_SUCCESS;
}

void SDVDiscoveryImpl::sectionFilterEvent(SDVSectionFilter::SDV_SECTION_FILTER_EVENT_t * event){
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVDiscovery] sectionFilterEvent()  Size = %d\n",  event->data.size);
    bool datachanged = false;

    rmf_osal_mutexAcquire(instance->m_dataMutex);

    if( instance->m_mcp_data  == NULL || !(*instance->m_mcp_data == event->data) ){
        datachanged = true;
        SDVSectionFilter::SDV_MCP_DATA * newData = new SDVSectionFilter::SDV_MCP_DATA();
        newData->size = event->data.size;
        newData->data = ( uint8_t *) malloc(sizeof( uint8_t) * event->data.size );
        memcpy(newData->data, event->data.data, event->data.size);

        if( instance->m_mcp_data != NULL ){
            free(instance->m_mcp_data->data);
            delete instance->m_mcp_data;
        }
        instance->m_mcp_data = newData;
    }

    if(datachanged || instance->m_triggerNewDiscovery ){
        instance->invokeMCPAvailableIARM();
    }else{
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVDiscovery] sectionFilterEvent() Data has not changed\n");
    }
    rmf_osal_condSet(((SDVDiscoveryImpl*)event->callbackInstance)->m_waitForMCPCondition);

    rmf_osal_mutexRelease(instance->m_dataMutex);
}

rmf_Error SDVDiscoveryImpl::ipv6AddressFileUpdated(void * ptrObj, uint8_t *data, uint32_t size){

	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVDiscovery] ipv6AddressFileUpdated() parsing new data.\n");

	//Map file to hold ipv4 to ipv6 mapping: key=IPV4 val=IPV6
	std::map<std::string, std::string > ipaddressesMap;

	TiXmlDocument doc;
    doc.Parse((const char*)data, 0, TIXML_DEFAULT_ENCODING);
    int xmlerror = doc.ErrorId();
    if (xmlerror == 0 ){
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVDiscovery] ipv6AddressFileUpdated() triggering address map XML File Parsing.\n");
        TiXmlElement *levelElement = doc.FirstChildElement("AddressMap");

        for (TiXmlElement* child = levelElement->FirstChildElement(); child != NULL; child = child->NextSiblingElement()) {
                std::string ipv4 = child->Attribute("ipv4");
                std::string  ipv6 =  child->Attribute("ipv6");
                ipaddressesMap[ipv4] = ipv6;
            }
    }else{
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVDiscovery] ipv6AddressFileUpdated() error in XML parsing.\n");
        return RMF_OSAL_ENODATA;
    }

    rmf_osal_mutexAcquire(instance->m_dataMutex);

    //Free any old data here since this is an update.
    if(instance->m_ipv6AddressMapCount > 0){
        free(instance->m_ipv6AddressMap);
    }
    instance->m_ipv6AddressMap = (SDVAGENT_IPV6_MAP_ELEMENT*) malloc(sizeof(SDVAGENT_IPV6_MAP_ELEMENT) * ipaddressesMap.size());
    int success_count = 0;
    for(std::map<std::string, std::string>::const_iterator it = ipaddressesMap.begin(); it != ipaddressesMap.end(); ++it){
        struct in6_addr ipv6;
        struct in_addr ipv4;

        if (inet_pton(AF_INET6, it->second.c_str(), &ipv6) == 0 || inet_aton(it->first.c_str(), &ipv4) == 0 ){
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVDiscovery] failed to parse ipv4[%s]->ipv6[%s] mapping\n", it->first.c_str(), it->second.c_str());
            continue;
        }

        //Copy data into the map.
        memcpy(&(instance->m_ipv6AddressMap[success_count].ipv6address[0]),&ipv6, sizeof(in6_addr));
        memcpy(&(instance->m_ipv6AddressMap[success_count].ipv4address[0]),&ipv4, sizeof(in_addr));
        ++success_count;

        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVDiscovery] ipv6AddressFileUpdated() \t ipv4:%s -> ipv6:%s()\n", it->first.c_str(), it->second.c_str());
    }

    instance->m_ipv6AddressMapCount = success_count;

    rmf_osal_mutexRelease(instance->m_dataMutex);

    return success_count > 0? RMF_SUCCESS: RMF_OSAL_ENODATA;
}


void SDVDiscoveryImpl::callbackRequestLatestMCData(const char *owner, IARM_EventId_t eventId, void *data, size_t len){
    rmf_osal_mutexAcquire(instance->m_dataMutex);

    if( instance->m_mcp_data != NULL){
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVDiscovery] callbackRequestLatestMCData() triggering RPC with boot time results.\n");
        instance->invokeMCPAvailableIARM();
    }else{
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVDiscovery] callbackRequestLatestMCData() setting m_triggerNewDiscovery to TRUE.\n");
        instance->m_triggerNewDiscovery = true;
    }

    rmf_osal_mutexRelease(instance->m_dataMutex);
}

void SDVDiscoveryImpl::callbackRequestFreshMCData(const char *owner, IARM_EventId_t eventId, void *data, size_t len){
    rmf_osal_mutexAcquire(instance->m_dataMutex);

    instance->m_mcp_data = NULL;
    instance->m_triggerNewDiscovery = true;

    rmf_osal_mutexRelease(instance->m_dataMutex);

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVDiscovery] callbackRequestFreshMCData() setting m_mcpResponsePending to TRUE.\n");
}

// Caller must acquire m_mcpDataMutex before calling this function
void SDVDiscoveryImpl::invokeMCPAvailableIARM(){
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVDiscovery] invokeMCPAvailableIARM()");

    //Constrcut the IARM paylaod.
    uint16_t ipAddressMapSize = sizeof(SDVAGENT_IPV6_MAP_ELEMENT) * m_ipv6AddressMapCount;
    uint16_t payload_size = m_mcp_data->size + ipAddressMapSize;
    uint16_t iarmMsgSize = sizeof(IARM_SDVAGENT_NEW_MC_DATA_PAYLOAD) + payload_size;

    IARM_SDVAGENT_NEW_MC_DATA_PAYLOAD * iarmMessage = (IARM_SDVAGENT_NEW_MC_DATA_PAYLOAD *) malloc(iarmMsgSize);
    iarmMessage->mcpDataSize = m_mcp_data->size;
    iarmMessage->ipv6MapElementCount = m_ipv6AddressMapCount;

    memcpy(iarmMessage->payload, m_mcp_data->data, m_mcp_data->size);
    memcpy(&iarmMessage->payload[m_mcp_data->size], m_ipv6AddressMap, ipAddressMapSize);


    IARM_Result_t result = IARM_Bus_Call(IARM_BUS_SDVAGENT_NAME, IARM_BUS_SDVAGENT_API_NEW_MC_DATA, iarmMessage, iarmMsgSize);
    if(result != IARM_RESULT_SUCCESS){
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "[SDVDiscovery] invokeMCPAvailableIARM() IARM RPC failed %d\n", result);
    }
    else
    {
        instance->m_triggerNewDiscovery = false;
    }

    free(iarmMessage);
}
