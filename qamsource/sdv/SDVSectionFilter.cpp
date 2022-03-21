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
* @file SDVSectionFilter.cpp
* @brief Implementation of SDVSection Filter
* @Author Jason Bedard jasbedar@cisco.com
* @Date Sept 11 2014
*/
#include "include/SDVSectionFilter.h"

#include <stdio.h>
#include <stddef.h>
#include <sstream>
#include <string.h>
#include <assert.h>
#include "rdk_debug.h"


#define NEW_SECTION_IGNORED false
#define NEW_SECTION_PENDING true


#define CRC_TABLE   {   0x00000000, \
        0x04C11DB7, \
        0x09823B6E, \
        0x0D4326D9, \
        0x130476DC, \
        0x17C56B6B, \
        0x1A864DB2, \
        0x1E475005, \
        0x2608EDB8, \
        0x22C9F00F, \
        0x2F8AD6D6, \
        0x2B4BCB61, \
        0x350C9B64, \
        0x31CD86D3, \
        0x3C8EA00A, \
        0x384FBDBD }

uint32_t SDVSectionFilter::nextFilterId = 20;

/**
 * @brief It is a parameterised constructor to create and initialise SDVSectionFilter with specified configuration.
 * As part of initialisation, an instance of inband section filter, semaphore and a condition used to wait on
 * refresh attempts are also created.
 *
 * @param[in] tunerId ID of the tuner which is or will be tuned to SDV Transport.
 * @param[in] frequency Tuner frequency in Hz used for section filter creation.
 * @param[in] demuxHandle Demux Handle to be used for section filtering.
 *
 * @return None
 */
SDVSectionFilter::SDVSectionFilter(uint32_t tunerId, uint32_t frequency, unsigned long demuxHandle){
    m_state = STOPPED;

    rmf_PsiSourceInfo psiSource;
    psiSource.tunerId = tunerId;
    psiSource.freq = frequency;
    psiSource.dmxHdl = demuxHandle;

    memset(m_pending_sections,0,sizeof(m_pending_sections));
    m_version = -1;

    filterMask[0] = (uint8_t)FILTER_MASK;
    filterVal[0] = (uint8_t)FILTER_VAL;
    filterNegativeMask[0] = (uint8_t)FILTER_NEGATIVE_MASK;
    filterNegativeVal[0] = (uint8_t)FILTER_NEGATIVE_VAL;

	threadStopSem = new sem_t;
	assert(sem_init(threadStopSem, 0 , 0) == 0);

    std::ostringstream idStream;
    idStream << "SDVSectionFilter-" << nextFilterId++;
    this->m_filterName =  idStream.str();

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVSectionFilter] <init> id =%s freq=%d tunerId=%d demuxHandle=%lu \n ", m_filterName.c_str(), frequency, tunerId, demuxHandle);

    m_inbandFilter = rmf_SectionFilter::create(rmf_filters::inband,(void*)(&psiSource));

    rmf_Error err = rmf_osal_condNew(FALSE, FALSE, &m_waitForRefreshConditional);
    if (err != RMF_SUCCESS) {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "[SDVSectionFilter] <init> rmf_osal_condNew failed\n");
    }
}

/**
 * @brief This is a default destructor for SDVSectionFilter. It releases and frees all the memory
 * used for semaphore, section filter and condition creation.
 *
 * @return None
 */
SDVSectionFilter::~SDVSectionFilter() {
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVSectionFilter] <destroy> id =%s\n ", m_filterName.c_str() );

    rmf_osal_condDelete(m_waitForRefreshConditional);
    delete m_inbandFilter;

    sem_destroy(threadStopSem);
    delete threadStopSem;
}

/**
 * @brief This function creates a event queue for posting inband section filtering events and starts
 * the section filter for MCP pid asynchronously. And it also creates a thread, to monitor MCP change
 * events. It also registers the call back function and call back instance with SDVSectionFilter
 * which is used when MCP data is ready.
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
rmf_Error SDVSectionFilter::start(void * callbackInstance, SDV_SECTION_FILTER_CALLBACK_t callback){
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "[SDVSectionFilter] id = %s start()\n", m_filterName.c_str());

    m_listInstance = callbackInstance;
    m_listener = callback;

    rmf_Error ret = RMF_SUCCESS;
    rmf_osal_condUnset(m_waitForRefreshConditional);

    ret = rmf_osal_eventqueue_create ((const uint8_t *)m_filterName.c_str(), &m_queueId);

    if(RMF_SUCCESS !=  ret){
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "[SDVSectionFilter] start() failed to start the event queue\n");
        return ret;
    }

    m_state = RUNNING;

    if (startFilter() == RMF_SUCCESS) {
        ret = rmf_osal_threadCreate(doLoop, this,RMF_OSAL_THREAD_PRIOR_DFLT, RMF_OSAL_THREAD_STACK_SIZE, &m_threadId, m_filterName.c_str());
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "[SDVSectionFilter] start() monitor thread started with ID:%lu\n", m_threadId);
    }

    return ret;
}

rmf_Error SDVSectionFilter::startFilter() {
	rmf_FilterSpec spec;

	// SetFilter copies these elements, therefore the rmf_FilterSpec structure doesn't to be persisted during the life of the filter
    spec.pos.vals = filterVal;
    spec.pos.length = sizeof(filterVal);
    spec.pos.mask = filterMask;
    spec.neg.vals = filterNegativeVal;
    spec.neg.length = sizeof(filterNegativeVal);
    spec.neg.mask = filterNegativeMask;

    rmf_Error result = m_inbandFilter->SetFilter(SDV_MCP_PID, &spec, m_queueId, SDV_FILTER_PRIORITY, 0, 0, &m_filterId);
    if(RMF_SUCCESS != result) {
        RDK_LOG(RDK_LOG_FATAL, "LOG.RDK.QAMSRC", "[SDVSectionFilter] %s() SetFilter failed - %d \n", __FUNCTION__, result);
    }
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVSectionFilter] %s() filterId=%d val[0]=0x%x length=%d\n", __FUNCTION__, m_filterId, spec.pos.vals[0], spec.pos.length);
    return result;
}

void SDVSectionFilter::doLoop(void * sectionFilterInstance ) {
    SDVSectionFilter * instance = (SDVSectionFilter *) sectionFilterInstance;
    instance->m_versionCompleted = false;

    while(instance->m_state == RUNNING) {
        SDV_MCP_DATA* newSection = new SDV_MCP_DATA();
        newSection->data = new uint8_t[SDV_MAX_SECTION_SIZE];

        // If Attempt aborted or handler indicates new section was not used, delete it now
        if(RMF_SUCCESS != instance->waitForSection(newSection) || instance->handleNewSection(newSection) == NEW_SECTION_IGNORED){
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVSectionFilter] %s() new section not found\n",__FUNCTION__);
            delete newSection->data;
            delete newSection;
        }

        // If all sections for a version have been received, pause filter and sleep for interval
        if (instance->m_versionCompleted && instance->m_state == RUNNING) {
        	if (instance->m_inbandFilter->pause(instance->m_filterId) != RMF_SECTFLT_Success) {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "[SDVSectionFilter] %s() SectionFilter pause failed!\n", __FUNCTION__);
        	}
            instance->freePendingSectionData();

            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVSectionFilter] %s() sleep for MCP_REFRESH_INTERVAL_MS...\n", __FUNCTION__);
            rmf_osal_condWaitFor(instance->m_waitForRefreshConditional, MCP_REFRESH_INTERVAL_MS);
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVSectionFilter] %s() sleep has ended, state=%d\n",__FUNCTION__, instance->m_state);

            // No need to resume if we've been stopped
            if (instance->m_state == RUNNING)  {
                // Clear queue in case any events received between eventqueue_get_next_event_timed and pause
                rmf_osal_eventqueue_clear(instance->m_queueId);

				if (instance->m_inbandFilter->resume(instance->m_filterId) != RMF_SECTFLT_Success) {
					RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "[SDVSectionFilter] %s() SectionFilter resume failed!\n", __FUNCTION__);
				}
            }
        }
    } //while(state == RUNNING)

    if (sem_post(instance->threadStopSem) != 0) {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "[SDVSectionFilter] %s() sem_post failed!\n", __FUNCTION__);
    }

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "[SDVSectionFilter] %s() exit \n", __FUNCTION__);
}


rmf_Error SDVSectionFilter::waitForSection(SDV_MCP_DATA* newSection){
    rmf_Error retVal =  RMF_OSAL_ENODATA;
    while(m_state == RUNNING && retVal != RMF_SUCCESS){
        uint32_t eventType;
        rmf_osal_event_handle_t eventHandle;
        rmf_osal_event_params_t eventParams;
        rmf_Error result;

        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "[SDVSectionFilter] %s() wait for event from filter %d\n",__FUNCTION__, m_filterId);
        if ((result = rmf_osal_eventqueue_get_next_event_timed (m_queueId, &eventHandle, NULL,
        		&eventType, &eventParams, MCP_READ_TIMEOUT)) != RMF_SUCCESS) {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "[SDVSectionFilter] %s() eventqueue_get_next_event_timed failed - %d\n",__FUNCTION__, result);
            continue;
        }
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "[SDVSectionFilter] %s() received eventType %d\n",__FUNCTION__, eventType );
        if((eventType != RMF_SF_EVENT_SECTION_FOUND) && (eventType != RMF_SF_EVENT_LAST_SECTION_FOUND)){
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "[SDVSectionFilter] %s() Unknown event type %d\n",__FUNCTION__, eventType );
            rmf_osal_event_delete(eventHandle);
            continue;
        }
        rmf_FilterSectionHandle sectionHandle = RMF_SF_INVALID_SECTION_HANDLE;

        if(RMF_SUCCESS != (result = m_inbandFilter->GetSectionHandle(m_filterId, 20, &sectionHandle))){
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "[SDVSectionFilter] %s() error:%d retrieving section handle for filter %d\n",__FUNCTION__, result, m_filterId);
            rmf_osal_event_delete(eventHandle);
            continue;
        }

        uint32_t length;    // length returned by filter is not reliable, therefore value in this variable is ignored
        if(RMF_SUCCESS != (result = m_inbandFilter->ReadSection(sectionHandle,0, SDV_MAX_SECTION_SIZE, 0, newSection->data, &length))){
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "[SDVSectionFilter] %s() error %d reading from section handle 0x%08x for filter %d\n",
                    __FUNCTION__, result, sectionHandle, m_filterId);
            m_inbandFilter->ReleaseSection(sectionHandle);
            rmf_osal_event_delete(eventHandle);
            continue;
        }

        //If we made it this far we have the section
        retVal = RMF_SUCCESS;
        m_inbandFilter->ReleaseSection(sectionHandle);

        rmf_osal_event_delete(eventHandle);
    }
    return retVal;
}

bool SDVSectionFilter::handleNewSection(SDV_MCP_DATA* newSection){

    newSection->size = getSectionLength(newSection->data);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "[SDVSectionFilter] handleNewSection() section size=%d\n", newSection->size);

    if(SDV_MAX_SECTION_SIZE < newSection->size){
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "[SDVSectionFilter] handleNewSection() section length exceeds buffer size!\n");
        return NEW_SECTION_IGNORED;
    }

    // If CRC indicates invalid data, free section data and return so we can receive this section again or a new version
    if(!validateCRC(newSection)){
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "[SDVSectionFilter] handleNewSection() CRC Check failed\n");
        return NEW_SECTION_IGNORED;
    }

    // If new version detected, save new version number and free pending section data from previous version
    uint8_t version_number = newSection->data[VERSION_NUMBER_OFFSET] & VERSION_NUMBER_MASK;
    if(version_number != m_version){
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVSectionFilter] handleNewSection() version changed from %d to %d\n", m_version, version_number);
        freePendingSectionData();	// in case previous version was partially received
        m_version = version_number;
    }
    else if (m_versionCompleted){
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVSectionFilter] handleNewSection() version %d already completed\n", version_number);
        return NEW_SECTION_IGNORED;
    }

    //Make sure section conforms to MCMIS Specification
    normalizeSection(newSection);

    uint8_t section_number = newSection->data[SECTION_NUMBER_OFFSET];
    uint8_t last_section_number = newSection->data[LAST_SECTION_NUMBER_OFFSET];
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVSectionFilter] handleNewSection() section_number=%d last_section_number=%d\n", section_number, last_section_number);

    // If this section has already been received, free section data and return
    if(m_pending_sections[section_number] != NULL){
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVSectionFilter] handleNewSection() Ignoring duplicate section %d\n",section_number);
        return NEW_SECTION_IGNORED;
    }

    m_pending_sections[section_number] = newSection;
    bool moreNeeded = false;
    uint32_t totalSize = 0;
    for(int i=0;i<=last_section_number;i++){
        if(m_pending_sections[i] == NULL){
            moreNeeded = true;
            break;
        }
        totalSize+= m_pending_sections[i]->size;
    }

    if(!moreNeeded){
        versionCompleted(last_section_number,totalSize);
        m_versionCompleted = true;
    }
    else{
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "[SDVSectionFilter] handleNewSection() Waiting for more sections\n");
        m_versionCompleted = false;
    }
    return NEW_SECTION_PENDING;
}

void SDVSectionFilter::normalizeSection(SDV_MCP_DATA* data){
    uint8_t tableId = data->data[TABLE_ID_OFFSET];
    if(tableId == TABLE_ID_MC21_MOTO){
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVSectionFilter] normalizeSection() 2.1 Moto Section needs to be normalized\n");

        // 2.1 Moto Section data has an extra byte added (4th byte), remove it by copying remaining data over it
        memcpy(&data->data[MOTO_EXTRA_BYTE_INDEX], &data->data[MOTO_EXTRA_BYTE_INDEX+1], data->size - MOTO_EXTRA_BYTE_INDEX+1);
        --data->size;
        data->data[TABLE_ID_OFFSET] = TABLE_ID_MC21;
    }else{
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "[SDVSectionFilter] normalizeSection() Section does not need normalization\n");
    }
}

bool SDVSectionFilter::validateCRC(SDV_MCP_DATA* data){
    uint32_t table[] = CRC_TABLE;
    uint32_t crc = 0xffffffff;
    if (rdk_dbg_enabled("LOG.RDK.QAMSRC", RDK_LOG_DEBUG)) {
        data->dump();
    }

    for (int i = 0; i < data->size; ++i) {
        crc = ((uint32_t)(crc << 4)) ^ table[( ((uint32_t)(crc >> 28)) & 0xf) ^ (((uint32_t)(data->data[i] >> 4)) & 0xf)];
        crc = ((uint32_t)(crc << 4)) ^ table[(( (uint32_t)(crc >> 28)) & 0xf) ^ (data->data[i] & 0xf)];
    }

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "[SDVSectionFilter] validateCRC() crc = %d \n", crc);

    return crc == 0;
}

void SDVSectionFilter::versionCompleted(uint8_t last_section_number, uint32_t size){

    // Merge all sections sequentially into single buffer
    uint8_t* buffer = new uint8_t[size];
    uint32_t offset = 0;
    for(int i=0;i<=last_section_number;++i){
        memcpy(&buffer[offset], m_pending_sections[i]->data, m_pending_sections[i]->size);
        offset += m_pending_sections[i]->size;
    }

    // Prepare event for callback to listener
    SDV_MCP_DATA data;
    data.data = buffer;
    data.size = size;
    SDV_SECTION_FILTER_EVENT_t event;
    event.callbackInstance = m_listInstance;
    event.data = data;

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "[SDVSectionFilter] versionCompleted() send event to listener, data size = %d\n", data.size);
    m_listener(&event);
    delete buffer;
}

/**
 * @brief This function stops the section filerting operations by releasing the filter and deleting the queue.
 * - It first sets the SDV section filter state to STOPPING and then releases the filter.
 * - It frees all the MCP sections received if we are stopping in middle of a version.
 * - It clears and deletes the queue, created to post section filtering events.
 * - Finally changes the state SDV section filter state to STOPPED
 * @note The section filtering will be stopped asyncrnously so it is possible a single event may
 * still be reviced even after stop() returns
 *
 * @return None
 */
void SDVSectionFilter::stop(){
    if(m_state == RUNNING){
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVSectionFilter] %s() pause and cancel filter %d\n", __FUNCTION__, m_filterId);
        m_state = STOPPING;

    	if (m_inbandFilter->pause(m_filterId) != RMF_SECTFLT_Success) {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "[SDVSectionFilter] %s() SectionFilter pause failed!\n", __FUNCTION__);
    	}

        if(m_inbandFilter->CancelFilter(m_filterId) != RMF_SUCCESS) {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "[SDVSectionFilter] %s() CancelFilter failed!\n", __FUNCTION__);
        }

        rmf_osal_condSet(m_waitForRefreshConditional); 	// awaken doLoop thread in case it was sleeping

        // Wait for thread to terminate
	    if (sem_wait(threadStopSem) != 0) {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "[SDVSectionFilter] %s() sem_wait failed!\n", __FUNCTION__);
	    }

	    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[SDVSectionFilter] %s() ReleaseFilter %d\n", __FUNCTION__, m_filterId);
	    if (m_inbandFilter->ReleaseFilter(m_filterId) != RMF_SUCCESS) {
	        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "[SDVSectionFilter] %s() ReleaseFilter failed\n", __FUNCTION__);
	    }
	    freePendingSectionData();                 // just in case stopping in the middle of a version
	    rmf_osal_eventqueue_clear(m_queueId);
	    rmf_osal_eventqueue_delete(m_queueId);
	    m_state = STOPPED;

        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "[SDVSectionFilter] %s() completed\n", __FUNCTION__);
    }
}

uint16_t SDVSectionFilter::getSectionLength(uint8_t* data) {
    // Get length from MCMIS param because length returned by RDK Section filtering API is sometimes wrong
    uint16_t sectionLength =  (data[SECTION_LENGTH_OFFSET] & SECTION_LENGTH_BYTE_ONE_MASK) << 8
            | data[SECTION_LENGTH_OFFSET +1];

    // Include length size itself and data before length field to give us the total length of the data
    return sectionLength + SECTION_LENGTH_OFFSET + sizeof(uint16_t);
}

void SDVSectionFilter::freePendingSectionData() {
    for(int i=0; i < MAX_NUMBER_OF_SECTIONS; i++) {
        if(m_pending_sections[i] != NULL) {
            delete m_pending_sections[i]->data;
            delete m_pending_sections[i];
            m_pending_sections[i] = NULL;
        }
    }
}

/**
 * @brief This function is used to print the MCP data into the log file in hexadecimal format.
 *
 * @return None
 */
void SDVSectionFilter::SDV_MCP_DATA::dump(){
    char buff[200];
    uint8_t soffset =0;
    memset(buff, '\0', sizeof(buff));
    unsigned char *pc = (unsigned char*)data;
    for (int i = 0; i < size; i++) {
        if ((i % 16) == 0) {
            if (i != 0){
                RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "[SDVSectionFilter] [SDV_MCP_DATA]  %s\n", buff);
                memset(buff, '\0', sizeof(buff));
                soffset = 0;
            }
        }
        soffset += snprintf (&buff[soffset], sizeof(buff)-soffset," %02x", pc[i]);
    }
    if(size % 16 != 0){
        RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "[SDVSectionFilter] [SDV_MCP_DATA]  %s\n", buff);
    }
}
