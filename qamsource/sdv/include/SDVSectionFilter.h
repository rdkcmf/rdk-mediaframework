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
* @file SDVSectionFilter.h
* @brief Encapusiation of logic to section filter MCMIS Configuration data.
* @Author Jason Bedard jasbedar@cisco.com
* @Date Sept 11 2014
*/
#ifndef SDVSECTIONFILTER_H_
#define SDVSECTIONFILTER_H_

#include "rmf_sectionfilter.h"
#include "rmf_error.h"
#include "rmf_osal_event.h"
#include "rmf_osal_thread.h"
#include <vector>
#include <string>
#include <semaphore.h>

/**
 * @addtogroup SDVMacros
 * @{
 */

#define SDV_MCP_PID 0x1fee              	//!< Well Known PID where MCP data can be found on SDV transports.
#define SDV_FILTER_PRIORITY 20          	//!< The priority of in band filter for SDV MCP data.
#define MCP_READ_TIMEOUT 2000000L       	//!< 2 second timeout for wait to get Section filter event
#define MCP_REFRESH_INTERVAL_MS (1000*60*5) //!< 5min polling for MCP Data; yes we are ignoring refresh MC ReadInterval in MC data

#define SDV_MAX_SECTION_SIZE (1024)     //!< The maximum size of a MCP Section
#define MAX_NUMBER_OF_SECTIONS 25       ///!< The maximum number of SDV MCP Sections supported.

#define SECTION_NUMBER_OFFSET 6         //!< Offset into a MCP Section identifying index for section is a list of sections
#define LAST_SECTION_NUMBER_OFFSET 7    //!< Offset into a MCP Section identifying what the last section number will be for a list of sections
#define VERSION_NUMBER_OFFSET 5         //!< Offset into a MCP Section identifying which version(Group) a section belongs to
#define VERSION_NUMBER_MASK 0x1F        //!< Mask applied to Byte containing version number to get the correct value.

#define SECTION_LENGTH_OFFSET 1             //!< Offset into section where length can be found
#define SECTION_LENGTH_BYTE_ONE_MASK 0xF    //!< Mask applied to first byte of section length to get correct value.


#define TABLE_ID_MC21 0xC9          //!< MMCIS Table Id */
#define TABLE_ID_MC21_MOTO 0xB9     //!< For Some reason MMCIS on Moto has a different table id */

#define TABLE_ID_OFFSET 0           //!< Offset into a MCP Section where table ID can be found
#define MOTO_EXTRA_BYTE_INDEX 3     //!< Location in MOTO MMCIS table where extra byte needs to be removed

/** @} */

/**
 * @class SDVSectionFilter
 *
 * @brief Encapsulation of logic to section filter MCMIS Configuration data.
 *
 * MCMIS Configuration also known as MCP data is carried on all SDV transports
 * and contains critical Configuration information needed by an SDV client to
 * communicate with the SDV Server and resolve tuning parameters.
 *
 * The MCP data is carried on a well known PID ( 0x1fee ) so there is no need to
 * look it up in the PMT.
 *
 * Because the MCP data can grow beyond the max section size of 1024 bytes it
 * is expects that data can be spread across multiple sections.  The SDVSectionFilter
 * will compile the multiple sections together and will only notify its listener
 * when all sections are received.
 *
 * After all sections have been successfully read the SectionFilter will continue
 * to re-read the MCP data after a backoff period of 10 seconds.  This is to prevent
 * CPU hogging.
 *
 * Attempts to read the MCP data will be done until the Section filter is stopped.
 */
class SDVSectionFilter {
public:

    /**
     * @struct sdv_mcp_data
     * @brief Data structure represing SDV MCP data
     * @ingroup SDVdatastructures
     */
    typedef struct sdv_mcp_data{
        uint8_t * data;         //!< SDV MCP Data */
        uint16_t size;          //!< The size in bytes of the MCP data */

	void dump();
        bool operator== (sdv_mcp_data& compare) const{
            if(size != compare.size){
                return false;
            }
            for(int i=0;i!=size;++i){
                if(data[i] != compare.data[i]){
                    return false;
                }
            }
            return true;
        }
    }SDV_MCP_DATA;

    /**
     * @struct sdv_section_filter_event
     * @brief Data structure representing event sent to Callback when MCP data has been read.
     * @ingroup SDVdatastructures
     */
    typedef struct sdv_section_filter_event{
        SDV_MCP_DATA data;        //!< Combined SDV MCP data encapsulating all sections of a common version */
        void * callbackInstance;  //!< Pointer to the instance of the class which registered for this event */
    }SDV_SECTION_FILTER_EVENT_t;

    /** @brief Section Filter callback Function template
     *
     * Template for callback function pointer which gets invoked when
     * MCP data is available.
     *
     * @param event the Section filter event containing MCP data
     */
    typedef void (*SDV_SECTION_FILTER_CALLBACK_t)(SDV_SECTION_FILTER_EVENT_t * event);
public:
    SDVSectionFilter(uint32_t tunerId, uint32_t frequency, unsigned long demuxHandle);

    ~SDVSectionFilter();

    rmf_Error start(void * callbackInstance, SDV_SECTION_FILTER_CALLBACK_t callback);

    void stop();


private:
    /**
     * @fn doLoop
     * @brief MCP monitor thread loop
     *
     * Will loop and check for SectionFiltering events on the event queue.
     * When an event is posted the MCP data will be inspected and merged with other
     * When sections of the same version.
     *
     * Once all sections have been obtained they will be combined and the registered listener
     * notified.
     *
     * After the listener is notified thread will wait 10 seconds before attempting to re-read
     * MCP data.
     */
    static void doLoop(void * sectionFilterInstance);

    rmf_Error waitForSection(SDV_MCP_DATA* newSection);

    /**
     * @fn handleNewSection
     * @brief MCP monitor thread loop
     *
     * Will loop and check for SectionFiltering events on the event queue.
     * Inspected and merge an incoming MCP Section with other sections of the same version.
     *
     * If all sections have been obtained they will be combined and the registered listener
     * notified.
     *
     *@param [in] newSection - new MCP Section to be processed.
     *@param return true if section used; false if section not used
     */
    bool handleNewSection(SDV_MCP_DATA* newSection);

    /**
     * @fn normalizeSection
     * @brief normalizes a new MCP Section removing extra bytes and resetting table Ids
     *
     * When on a MOTO network the table ID of the MCP Section is a non standered value and
     * an extra byte is present in the table.
     *
     * This function will normalize the data on all networks to the format specified in
     * in the MMCIS Specification
     *
     *@param [in] data - The current MCP Section to be normalized.
     */
    void normalizeSection(SDV_MCP_DATA* data);

    /**
     * @fn validateCRC
     * @brief performs a CRC Check on the new Section
     *
     * Using the CRC field from the data payload performs a SCRC
     * check to ensure sectionis not malformed.
     *
     *@param [in] data The current MCP Section to be validated.
     *
     *@return TRUE if section is valid otherwise false.
     */
    static bool validateCRC(SDV_MCP_DATA* data);

    /**
     * @fn versionCompleted
     * @brief Called when all MCP Sections are ready and will trigger event to listener
     *
     * @param [in] last_section_number that last section number in the completed version
     * @param [in] size the total size of all sections in the completed version
     */
    void versionCompleted(uint8_t last_section_number, uint32_t size);

    /**
     * @fn freeSectionData
     * @brief Free all section data referenced in m_pending_sections array
     */
    void freePendingSectionData();

    /**
     * @fn getSectionLength
     * @brief Get the byte length of the specified section
     * @details The length of the section is determined using the MCMIS data field.
     * The length returned by RMF SectionFilter was discovered to be unreliable.
     *
     * @param data - pointer to MCMIS section data
     * @return - return total length of section
     */
    uint16_t getSectionLength(uint8_t* data);

    rmf_Error startFilter();


private:

	#define FILTER_MASK 			(TABLE_ID_MC21 & TABLE_ID_MC21_MOTO) // Only the common bits between the possible table Ids
	#define FILTER_VAL				(TABLE_ID_MC21 & TABLE_ID_MC21_MOTO) // Only common bits should be left
	#define FILTER_NEGATIVE_MASK	0xFF
	#define FILTER_NEGATIVE_VAL		(TABLE_ID_MC21 & TABLE_ID_MC21_MOTO) // Make sure value is not ONLY the common bits between table Ids

    static uint32_t nextFilterId;   //Provides a unique ID for section filter and threads

    std::string m_filterName;		//!< String name for Section filter and Threads

    enum {
    	RUNNING,					//!< filter thread is running
    	STOPPING,					//!< filter thread is the process of stopping
    	STOPPED						//!< filter thread has stopped
    } m_state;						//!< state of filter thread

    rmf_SectionFilter *             m_inbandFilter; //!< Reference to inband filter used to read MCP data
    uint8_t filterMask[1];
    uint8_t filterVal[1];
    uint8_t filterNegativeMask[1];
    uint8_t filterNegativeVal[1];

    rmf_osal_ThreadId               m_threadId; //!< ID of the MCP monitoring thread.
    rmf_osal_eventqueue_handle_t    m_queueId;  //!< ID of event queued where Section filtering events from inband filter are posted.
    uint32_t                        m_filterId; //!< ID of Inband filter

    void *                          m_listInstance; //!< ID of Pointer to the instance of the callback function's class
    SDV_SECTION_FILTER_CALLBACK_t   m_listener;     //!< Function pointer to callback.

    SDV_MCP_DATA*                   m_pending_sections[MAX_NUMBER_OF_SECTIONS]; //!< Holds sections which are part of a group of sections with common version
    uint8_t                         m_version;                                  //!< Current version # of MCP sections known to this filter
    bool                            m_versionCompleted;                        	//!< If the current version being processed has been completed

    rmf_osal_Cond                   m_waitForRefreshConditional;     //!< Conditional used to wait on refresh attempts
    sem_t*                          threadStopSem;  //!< semaphore to wait for filter to be released in stop() method
};

#endif /* SDVSECTIONFILTER_H_ */
