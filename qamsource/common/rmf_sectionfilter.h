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
*/

#ifndef __RMF_SECTIONFILTER_H__
#define __RMF_SECTIONFILTER_H__

#include <stdint.h>
#include <atomic> 
//#include "vlUtil.h"
#include "rmf_symboltable.h"
#include "rmf_osal_sync.h"
#include "rmf_osal_event.h"
#include "rmf_sectionfilter_util.h"
#include "rmf_inbsimgr.h"


/**
 * @file rmf_sectionfilter.h
 * @brief File contains the section filter data.
 */


/**
 * @defgroup sectionfilter Section Filter
 * @ingroup Inband
 * @defgroup sectionfilterclass Section Filter Class
 * @ingroup sectionfilter
 * @defgroup sectionfiltermacros Section Filter Macros
 * @ingroup sectionfilter
 * @defgroup sectionfilterflags Section Filter Flags
 * @ingroup sectionfiltermacros
 * @defgroup sectionfilterevents Section Filter Events
 * @ingroup sectionfiltermacros
 * @defgroup sectionfilterreturnvalues Section Filter Return Values
 * @ingroup sectionfiltermacros
 */


/**
 * @addtogroup sectionfilterflags
 * Section Filtering option flags
 * @{
 */

/**
 * @brief Indicates that the associated operation should only be carried out if the filter is not in the cancelled state.
 */
#define RMF_SF_OPTION_IF_NOT_CANCELLED             (0x00000001)


/**
 * @brief Indicates that the associated operation should release the target element after performing the operation.
 */
#define RMF_SF_OPTION_RELEASE_WHEN_COMPLETE        (0x00000002)
/** @} */


/**
 * @addtogroup sectionfilterevents
 * Events generated from section filter component.
 * @{
 */
#define RMF_SF_EVENT_UNKNOWN                        (0x00001000)
#define RMF_SF_EVENT_SECTION_FOUND                  (RMF_SF_EVENT_UNKNOWN + 1)
#define RMF_SF_EVENT_LAST_SECTION_FOUND             (RMF_SF_EVENT_UNKNOWN + 2)
#define RMF_SF_EVENT_FILTER_CANCELLED               (RMF_SF_EVENT_UNKNOWN + 3)
#define RMF_SF_EVENT_FILTER_PREEMPTED               (RMF_SF_EVENT_UNKNOWN + 4)
#define RMF_SF_EVENT_SOURCE_CLOSED                  (RMF_SF_EVENT_UNKNOWN + 5)
#define RMF_SF_EVENT_OUT_OF_MEMORY                  (RMF_SF_EVENT_UNKNOWN + 6)
#define RMF_SF_EVENT_FILTER_AVAILABLE               (RMF_SF_EVENT_UNKNOWN + 7)
#define RMF_SF_EVENT_CA                             (RMF_SF_EVENT_UNKNOWN + 8)
/** @} */


/**
 * @addtogroup sectionfilterreturnvalues
 * Return values from section filter APIs.
 * @{
 */
#define RMF_SF_ERROR                                    (0x00002000)
#define RMF_SF_ERROR_FILTER_NOT_AVAILABLE               (RMF_SF_ERROR + 1)
#define RMF_SF_ERROR_INVALID_SECTION_HANDLE             (RMF_SF_ERROR + 2)
#define RMF_SF_ERROR_SECTION_NOT_AVAILABLE              (RMF_SF_ERROR + 3)
#define RMF_SF_ERROR_TUNER_NOT_TUNED                    (RMF_SF_ERROR + 4)
#define RMF_SF_ERROR_TUNER_NOT_AT_FREQUENCY             (RMF_SF_ERROR + 5)
#define RMF_SF_ERROR_UNSUPPORTED                        (RMF_SF_ERROR + 6)
#define RMF_SF_ERROR_CA                                 (RMF_SF_ERROR + 7)
#define RMF_SF_ERROR_CA_ENTITLEMENT                     (RMF_SF_ERROR_CA + 1)
#define RMF_SF_ERROR_CA_RATING                          (RMF_SF_ERROR_CA + 2)
#define RMF_SF_ERROR_CA_TECHNICAL                       (RMF_SF_ERROR_CA + 3)
#define RMF_SF_ERROR_CA_BLACKOUT                        (RMF_SF_ERROR_CA + 4)
#define RMF_SF_ERROR_CA_DIAG                            (RMF_SF_ERROR_CA + 5)
#define RMF_SF_ERROR_CA_DIAG_PAYMENT                    (RMF_SF_ERROR_CA_DIAG + 1)
#define RMF_SF_ERROR_CA_DIAG_RATING                     (RMF_SF_ERROR_CA_DIAG + 2)
#define RMF_SF_ERROR_CA_DIAG_TECHNICAL                  (RMF_SF_ERROR_CA_DIAG + 3)
#define RMF_SF_ERROR_CA_DIAG_PREVIEW                    (RMF_SF_ERROR_CA_DIAG + 4)

#define RMF_SF_INVALID_SECTION_HANDLE              (0)
/** @} */


/**
 * @defgroup sectionfilterdatastructures Section Filter Data Structures
 * @ingroup sectionfilter
 *
 * @{
 * @struct _rmf_Section_Data
 * @brief  This is a data structure used by section filter internally.
 * When the section filter reads the section, it stores the section details in this structure.
 */
typedef struct _rmf_Section_Data
{
	uint32_t sectionID;
	uint8_t* sectionData;
	uint16_t sectionLength;
} rmf_Section_Data;


/**
 * @enum _rmf_SectionFilterState_
 * @brief This enumeration represents the different states of section section filter.
 * At any point of time section filter will be in one of these states.
 */
typedef enum _rmf_SectionFilterState_
{
	/**
	 * Invalid State
	 */
	SECTFLT_INVALID = 0x100,
	SECTFLT_INITIALIZED,
	SECTFLT_CREATED,
	SECTFLT_STARTED,
	SECTFLT_STOPPED,
	SECTFLT_RELEASED
} rmf_SectionFilterState;


/**
 * @enum _RMF_SECTFLT_RESULT_
 * @brief This enumeration represents the various results offered by section filter.
 * During the section filtering attempt, it will throw any one result to the caller.
 */
typedef enum _RMF_SECTFLT_RESULT_
{
	RMF_SECTFLT_Success = 0x200,
	RMF_SECTFLT_Error,
	RMF_SECTFLT_NotReady,
	RMF_SECTFLT_NotStarted,
	RMF_SECTFLT_OutMem,
	RMF_SECTFLT_NoFilterRes,
	RMF_SECTFLT_InvalidParams
}RMF_SECTFLT_RESULT;


/**
 * @enum rmf_SectionFilterReqState
 * @brief This enumeration represents the request state of section filter.
 * Once the request is received for section filtering, the request state will be one of these states. 
 */
typedef enum rmf_SectionFilterReqState
{
    RMF_SF_REQSTATE_INVALID = 0x150,
    RMF_SF_REQSTATE_READY,
    RMF_SF_REQSTATE_MATCHED,
    RMF_SF_REQSTATE_CANCELLED
} rmf_SectionFilterReqState;


/**
 * @enum _rmf_FilterSourceType_
 * @brief This enumeration represents the various sources on which section filter takes place.
 */
typedef enum _rmf_FilterSourceType_
{
	/**
	 * Source is out-of-band/persistent tuner (e.g. ext chan, DSG broadcast tunnel)
	 */
	RMF_FILTER_SOURCE_OOB = 1,
	/**
	 * Source is an already-tuned in-band/adjustable tuner
	 */
	RMF_FILTER_SOURCE_INB,
	/**
	 * Source is a DSG application tunnel managed by section filtering
	 */
	RMF_FILTER_SOURCE_DSG_APPID,
	/**
	 * Source is a HN Stream session
	 */
	RMF_FILTER_SOURCE_HN_STREAM
}rmf_FilterSourceType;


/**
 * @struct rmf_FilterComponent
 * @brief This data structure describes a set of data values which can be compared against a target section.
 */
typedef struct rmf_FilterComponent
{
    uint32_t length; /**< Specifies the length, in bytes, of the mask and vals arrays. */
    uint8_t * mask; /**< Pointer to an array of bytes defining the mask to be applied
     *    to the target data. The array of bytes must be at least length bytes. */
    uint8_t * vals; /**< Pointer to an array of bytes defining the values to be
     *    compared against in the target data. The array of bytes must be
     *    at least length bytes. */
     rmf_FilterComponent() {mask = NULL; vals = NULL;}
} rmf_FilterComponent;


/**
 * @struct rmf_FilterSpec
 * @brief This data structure describes the terms which defines a filter. The filter specification
 * describes the conditional terms to be evaluated against the target RMFG
 * section. If the target section positive components and negative components are
 * considered as arbitrarily-long data words, the logical expression of the filter is:
 * <code>
 * ((sectiondata[0] & pos.mask) == pos.vals)
 * &&
 * !((sectiondata[0] & neg.mask) == neg.vals)
 *   </code>
 */
typedef struct rmf_FilterSpec
{
    rmf_FilterComponent pos; /**< Positive-match criteria */
    rmf_FilterComponent neg; /**< Negative-match criteria */
} rmf_FilterSpec;


/**
 * @struct rmf_sf_SectionRequest_s
 * @brief Stores parameters needed for fulfilling a section acquisition/filtering
 * request. A rmf_sf_sectionRequest instance is created for every call to
 * SetFilter() and serves as the binding mechanism between a
 * RI Stream, associated hardware & software filters, and the sections which
 * are matched. The lifetime of a ri_sf_sectionRequest is directly tied to
 * the lifetime of a filter which is identified by a uniqueID.
 */
typedef struct rmf_sf_SectionRequest_s
{
	// Current filter state
	rmf_SectionFilterReqState    state;

	// Client event queue data
	rmf_osal_eventqueue_handle_t                        requestorQueue;
	//void*                                               requestorACT;

	// Filter priority
	uint8_t                                     priority;

	// This keeps track of how many matches are remaining.
	// If matchesRemaining == 0 and request is in the READY
	// state, then this filter will match sections until
	// cancelled
	uint32_t                                    matchesRemaining;
	uint32_t                                        TunerID; //0xFFFF if the source type is not Tuner
	unsigned long long m_startTimeInms_SinceEpoch;
	RMFQueue* m_SectionsDataQueue; 
//	rmf_osal_eventqueue_handle_t                        sectionsDataQueue;
	void* 	pSectionFilterInfo;
	//Sync element to avoid delte section under processing 
	std::atomic<uint8_t> uiIsSectionUnderProcessing  = ATOMIC_VAR_INIT (0); 
} rmf_sf_SectionRequest_t;

/** Identifies the handle for a section. */
typedef uint32_t rmf_FilterSectionHandle;


/**
 * @enum rmf_filter_type
 * @brief This namespace creates enumeration to represent the different sources on which section filtering takes place.
 * 
 * @}
 */
namespace rmf_filters
{
   typedef enum rmf_filter_type
   {
       inband = 0,
       oob,
       dsg
   }rmf_filter_type;
};


/**
 * @class rmf_SectionFilter
 * @brief Abstraction class for Section Filter.
 * It provides information about available services in an interactive broadcast environment.
 * PAT & PMT acquisition is done using the MPE Section Filter subsystem.
 * It manages PAT/PMT acquisition and parsing from OOB, DSG application tunnels, tuners, and HN streams.
 * @ingroup sectionfilterclass
 */
class rmf_SectionFilter
{

public:
	rmf_SectionFilter();
	rmf_SectionFilter(void* pFilterParam);
	virtual ~rmf_SectionFilter();
	virtual RMF_SECTFLT_RESULT resume(uint32_t filterID);
	virtual RMF_SECTFLT_RESULT pause(uint32_t filterID);
        static rmf_SectionFilter* create(rmf_filters::rmf_filter_type filter,void* fParam);
	rmf_Error SetFilter(uint16_t pid, const rmf_FilterSpec * filterSpec, 
			rmf_osal_eventqueue_handle_t queueId,
			/*void *act,*/
			uint8_t filterPriority, 
			uint32_t timesToMatch, 
			uint32_t flags,
			uint32_t * uniqueId);
	rmf_Error CancelFilter(uint32_t uniqueId);
	rmf_Error GetSectionHandle(uint32_t uniqueId, uint32_t flags,
			rmf_FilterSectionHandle * sectionHandle);
	rmf_Error ReleaseFilter(uint32_t uniqueId);
	rmf_Error GetSectionSize(rmf_FilterSectionHandle sectionHandle,
			uint32_t * size);
	rmf_Error ReadSection(rmf_FilterSectionHandle sectionHandle,
			uint32_t offset, uint32_t byteCount, 
			uint32_t flags, uint8_t * buffer,
			uint32_t * bytesRead);
	rmf_Error ReleaseSection(rmf_FilterSectionHandle sectionHandle);

protected:
	uint32_t getNextSectionID(void);
	void section_avail_cb(uint32_t filterID, rmf_Section_Data* pSectionData);

	void* get_section_filter_info(uint32_t filterID);

	virtual RMF_SECTFLT_RESULT Create(uint16_t pid, uint8_t* pos_mask, uint8_t* pos_value, 
					uint16_t pos_length, uint8_t* neg_mask, uint8_t* neg_value, 
					uint16_t neg_length, void **pSectionFilterInfo, uint32_t* pFilterID) = 0;
	virtual RMF_SECTFLT_RESULT Start(uint32_t filterID) = 0;
	virtual RMF_SECTFLT_RESULT Release(uint32_t filterID) = 0;
	virtual RMF_SECTFLT_RESULT Stop(uint32_t filterID) = 0;
	virtual RMF_SECTFLT_RESULT Read(uint32_t filterID, rmf_Section_Data** ppSectionData) = 0;
protected:
	/**
	 * Section Filter Information
	 */
	uint32_t m_SourceID;
	uint32_t m_NumberMessagesRec;
	uint32_t m_NumberOverflowMessages;
	rmf_SectionFilterState m_filterState;
	//pthread_mutex_t m_sfMutex; // Change to rmf_osal_mutex 
        rmf_osal_Mutex m_sfMutex;
        rmf_osal_Mutex m_sIdMutex;
	/**
	 * Incoming section queue.  As sections are received from the platform, they are
	 * placed in queue waiting for the client to retrieve them
	 */
	rmf_osal_eventqueue_handle_t m_queueHandle;
        uint32_t m_eventPriority;
	//VLSymbolMapTable* m_sectionRequests;       // Key: filter ID (integer
	rmf_SymbolMapTable* m_sectionRequests;       // Key: filter ID (integer
	rmf_SymbolMapTable* m_sectionRequestsToRemove; // Key: filter ID (integer 

private:
        rmf_osal_Mutex m_sectionRequestMutex;
                                                 // Value: pointer to vl_sf_sectionRequest

        rmf_osal_Mutex m_outstandingSectionMutex;
	//VLSymbolMapTable* outstandingSections;   // Key: section ID (integer)
	rmf_SymbolMapTable* m_outstandingSections;   // Key: section ID (integer)
                                                 // Value: pointer to vl_Section_Data
private:
        rmf_Error cancelFilter(uint32_t filterID, int filterEvent);
	uint32_t getLowestPriorityFilter(uint32_t filterPriority);
	void getTimeInMilliSecond(unsigned long long *pMillisSinceEpoch);
	rmf_Error dispatchEvent(rmf_osal_eventqueue_handle_t queueHandle, uint32_t filterEvent, uint8_t priority, uint32_t filterID);
	rmf_Error ReleaseFilterRequest(uint32_t uiFilterReqHandle);
	rmf_Error ReleaseFromDeferredDeleteQueue();
	bool IsDeferredDeleteQueueEmpty();
};

#endif // !defined(__RMF_SECTIONFILTER_H__)
