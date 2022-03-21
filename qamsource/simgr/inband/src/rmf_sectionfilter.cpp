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

/**
 * @file rmf_sectionfilter.cpp
 * @brief This file contains funtion definitions of the class rmf_SectionFilter which maintains
 * the section requests and the outstanding sections.
 */

#include <string.h>
#include <unistd.h>
#include "rmf_sectionfilter.h"
//#include "vlMalloc.h"
//#include "vlMemCpy.h"
//#include <mpeos_dbg.h>
#include "rdk_debug.h"
#include "rmf_osal_mem.h"
#include "rmf_osal_time.h"

#define INVALID_TUNER_ID                0xFFFF
#define MAX_SF_EVTQ_NAME_LEN        100
extern rmf_SectionFilter* createFilter(rmf_filters::rmf_filter_type ,void*);

/**
 * @brief This function creates an rmf_SectionFilter of specified type like inband or outband
 * for the provided frequency, modulation, demux handle and tuner ID.
 *
 * @param[in] filter Indicates the type of filter to be created.
 * @param[in] filterParam Indicates PSI source information like tunerId, frequency, demux handle and modulation.
 *
 * @return Returns pointer to rmf_SectionFilter.
 */
rmf_SectionFilter* rmf_SectionFilter::create(rmf_filters::rmf_filter_type filter,void* filterParam)
{
   return createFilter(filter,filterParam);
}

/**
 * @brief A default constructor to initialize the lists and mutexes used for the section ID's,
 * section request and outstanding section filters.
 * <ul>
 * <li> It initializes two lists. One for the section requests to be made and another for
 * providing the outstanding sections.
 * <li> It also initializes two mutexes for these lists and another for the section ID,
 * to ensure that not more than one filter will get the same section ID.
 * </ul>
 *
 * @return None
 */
rmf_SectionFilter::rmf_SectionFilter()
{
    m_sIdMutex = NULL;
    m_sectionRequestMutex = NULL;

    if(RMF_SUCCESS != rmf_osal_mutexNew(&m_sIdMutex))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILTER", "%s:: Could not create m_sIdMutex\n", __FUNCTION__);
    }

    if(RMF_SUCCESS != rmf_osal_mutexNew(&m_sectionRequestMutex))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILTER", "%s:: Could not create m_sectionRequestMutex\n", __FUNCTION__);
    }

    //sectionRequests = vl_symbolMapTable_new();
    m_sectionRequests = new rmf_SymbolMapTable();
    //Deferred  section delete requests since the filter request is under processing. 
    m_sectionRequestsToRemove =  new rmf_SymbolMapTable();  

    if(RMF_SUCCESS != rmf_osal_mutexNew(&m_outstandingSectionMutex))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILTER", "%s:: Could not create m_outstandingSectionMutex\n", __FUNCTION__);
    }

    //m_outstandingSections = vl_symbolMapTable_new();
    m_outstandingSections = new rmf_SymbolMapTable();
}

/**
 * @brief A parameterised constructor which initializes the lists and mutexes used in section filter.
 *
 * @param[in] pFilteParam Not used at present.
 *
 * @return None
 */
rmf_SectionFilter::rmf_SectionFilter(void* pFilteParam)
{
    if(RMF_SUCCESS != rmf_osal_mutexNew(&m_sIdMutex))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILTER", "%s:: Could not create m_sIdMutex\n", __FUNCTION__);
    }

    if(RMF_SUCCESS != rmf_osal_mutexNew(&m_sectionRequestMutex))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILTER", "%s:: Could not create m_sectionRequestMutex\n", __FUNCTION__);
    }

    //sectionRequests = vl_symbolMapTable_new();
    m_sectionRequests = new rmf_SymbolMapTable();
    //Requests would be deleted later since the filter request is under processing. 
    m_sectionRequestsToRemove =  new rmf_SymbolMapTable();  

    if(RMF_SUCCESS != rmf_osal_mutexNew(&m_outstandingSectionMutex))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILTER", "%s:: Could not create m_outstandingSectionMutex\n", __FUNCTION__);
    }

    //m_outstandingSections = vl_symbolMapTable_new();
    m_outstandingSections = new rmf_SymbolMapTable();

    // Create Mutex for protecting the Queue
    m_NumberMessagesRec             = 0;
    m_NumberOverflowMessages        = 0;
    m_filterState               = SECTFLT_INITIALIZED;

    //if(pthread_mutex_init(&pSecFilterObj->m_sfMutex, NULL) != RMF_SUCCESS)
    if(RMF_SUCCESS != rmf_osal_mutexNew(&m_sfMutex))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILTER", "%s:: Could not create m_sfMutex\n", __FUNCTION__);
    }
}

/**
 * @brief A default destructor used to release the resources. It deletes all the lists and the mutexes
 * used by the section filter.
 *
 * @return None
 */
rmf_SectionFilter::~rmf_SectionFilter(){

    int delQueueItr = 0;
	const int maxDelQueueItrLimit = 75;
    //pthread_mutex_lock(&pSectionFilterObj->m_sfMutex);
    rmf_osal_mutexAcquire(m_sfMutex);
    

    m_filterState   = SECTFLT_INVALID;
    
    //pthread_mutex_unlock(&pSectionFilterObj->m_sfMutex);
    rmf_osal_mutexRelease(m_sfMutex);
    //pthread_mutex_destroy(&(pSectionFilterObj->m_sfMutex)); // fix for mutex leak
    rmf_osal_mutexDelete(m_sfMutex); // fix for mutex leak
    
    rmf_osal_mutexAcquire(m_sIdMutex);
    rmf_osal_mutexRelease(m_sIdMutex);
    rmf_osal_mutexDelete(m_sIdMutex); // fix for mutex leak

    rmf_osal_mutexAcquire(m_sectionRequestMutex);
    rmf_osal_mutexRelease(m_sectionRequestMutex);
    rmf_osal_mutexDelete(m_sectionRequestMutex); // fix for mutex leak

    rmf_osal_mutexAcquire(m_outstandingSectionMutex);
    rmf_osal_mutexRelease(m_outstandingSectionMutex);
    rmf_osal_mutexDelete(m_outstandingSectionMutex); // fix for mutex leak

    delete m_sectionRequests;

    //75 X 20 milli seconds = Max approx 1.5 sec sleep
    for (delQueueItr = 0; delQueueItr < maxDelQueueItrLimit; delQueueItr++) {
        ReleaseFromDeferredDeleteQueue();
        //If nothing to release any more
        if (IsDeferredDeleteQueueEmpty()) {
            break;
        }
        //20 milli seconds sleep
        usleep (20000); 
    }
	//Max waited for approx 1.5 sec to release all. still if 
	//pending items to release. Create a log entry and exit
    if (maxDelQueueItrLimit == delQueueItr) {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILTER","Memory is lost from DeferredDeleteQueue\n");
    }

    delete m_sectionRequestsToRemove; 
    delete m_outstandingSections;
}

/**
 * @brief This function is used to get the section ID for the section filter.
 *
 * @return val Returns Section ID.
 */
uint32_t rmf_SectionFilter::getNextSectionID()
{
        static uint32_t val = 0;
    rmf_osal_mutexAcquire(m_sIdMutex);
    val++;
    if(65535 <= val) val = 1;
    rmf_osal_mutexRelease(m_sIdMutex);
    return val;
}

/**
 * @brief This function will create a filter and initiate filtering
 * for the section(s) originating from the section source that matches with the
 * designated filter specification.
 *
 * @param[in]
 *           pid Specifies PID value of the PSI table like PAT, PMT etc...
 * @param[in] filterSpec
 *           Pointer to the filter specification. This defines the
 *           criteria that the filter will evaluate the sections against.
 * @param[in] queueHandle
 *           Designates the queue in which to deliver filter events.
 * @param[in] filterPriority
 *           Designates the priority of the filter (1 being the highest)
 * @param[in] timesToMatch
 *           Designates the number of times the filter should match
 *           before stopping. A value of 0 indicates the filter
 *           should continue matching until cancelled.
 * @param[in] flags
 *           Specifies certain options for retrieving the handle.
 *           Should be 0 currently.
 * @param[out] uniqueId
 *           Designates a location to store the unique identifier used to
 *           identify the filter in event notifications and any
 *           subsequent operations.
 *
 * @return Return value indicates success or failure of the call.
 * @retval RMF_SUCCESS SetFilter is successful
 * @retval RMF_OSAL_EINVAL Indicates at least one input parameter has an invalid value
 * @retval RMF_ENOMEM Indicates insufficient memory
 * @retval RMF_SF_ERROR_FILTER_NOT_AVAILABLE Indicates insufficient filtering
 *  resources for the requested filter specification.
 * @retval RMF_SF_ERROR_TUNER_NOT_TUNED Indicates the specified filter source is not currently tuned
 * @retval RMF_SF_ERROR_TUNER_NOT_AT_FREQUENCY Indicates the specified filter source
 *  is not at the expected frequency.
 *
 * NOTE: When timesToMatch is greater than 1, it is expected that the MPEOS port will
 *       return consecutive sections that match the given criteria (i.e. will not
 *       skip sections). Failure to return consecutive matching sections will result
 *       in degraded performance for object/data carousels and DAVIC SectionFilter users.
 */
rmf_Error rmf_SectionFilter::SetFilter(uint16_t pid, const rmf_FilterSpec * filterSpec, rmf_osal_eventqueue_handle_t queueHandle, /*void *act,*/ uint8_t filterPriority, uint32_t timesToMatch, uint32_t flags, uint32_t * uniqueId)
{
    uint32_t filterID;
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.FILTER", "%s:: Called, timesToMatch = %d\n", __FUNCTION__, timesToMatch);
    rmf_sf_SectionRequest_t* pFilter_Request         = NULL;
    uint32_t SourceID  = 0;
    RMF_SECTFLT_RESULT returnCode                      = RMF_SECTFLT_Error;
    rmf_Error retOsal = RMF_SUCCESS;
    uint8_t qname[MAX_SF_EVTQ_NAME_LEN];
    void *pSectionFilterInfo = NULL;

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.FILTER", "%s - queueHandle: 0x%x\n", __FUNCTION__, queueHandle);
    if (filterSpec == NULL || uniqueId == NULL || filterPriority == 0)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILTER","%s -- parameter is NULL\n", __FUNCTION__);
        return RMF_OSAL_EINVAL;
    }

    int k = 0;
    if (filterSpec->pos.length > 0)
    {
        if( (NULL != filterSpec->pos.vals) && (NULL != filterSpec->pos.mask) )
        {
            for(k = 0; k < filterSpec->pos.length; k++)
            {
                RDK_LOG(RDK_LOG_TRACE5, "LOG.RDK.FILTER"," pos_value[%d] = 0x%x pos_mask[%d] = 0x%x\n", k, filterSpec->pos.vals[k],  k, filterSpec->pos.mask[k]);
            }
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILTER","Positive val/mask parameter is NULL\n");
            return RMF_OSAL_EINVAL;
        }
    }

    if (filterSpec->neg.length > 0)
    {
        if( (NULL != filterSpec->neg.vals) && (NULL != filterSpec->neg.mask) )
        {
            for(k = 0; k < filterSpec->neg.length; k++)
            {
                RDK_LOG(RDK_LOG_TRACE5, "LOG.RDK.FILTER"," neg_value[%d] = 0x%x neg_mask[%d] = 0x%x\n", k, filterSpec->neg.vals[k],  k, filterSpec->neg.mask[k]);
            }

        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILTER","Negative val/mask parameter is NULL\n");
            return RMF_OSAL_EINVAL;
        }
    }

    if(0 != flags)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.FILTER","%s:: Warning:: user passed flag != 0\n", __FUNCTION__);
    }

    // Allocate structure memory
    retOsal = rmf_osal_memAllocP(RMF_OSAL_MEM_FILTER, sizeof(rmf_sf_SectionRequest_t), (void**)&pFilter_Request);
    if ((NULL == pFilter_Request) || (retOsal != RMF_SUCCESS))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILTER","Could not allocate section request structure!\n");
        return retOsal;
    }

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILTER","%s:: pFilter_Request: 0x%x\n", __FUNCTION__, pFilter_Request);
    pFilter_Request->TunerID                                = INVALID_TUNER_ID; // Default value
    pFilter_Request->state                      = RMF_SF_REQSTATE_INVALID;
    pFilter_Request->requestorQueue                     = queueHandle;
    //pFilter_Request->requestorACT                       = act;
    pFilter_Request->priority                           = filterPriority;
    pFilter_Request->matchesRemaining           = timesToMatch;
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILTER","%s:: tableID: 0x%x, pFilter_Request->matchesRemaining: %d\n", __FUNCTION__, filterSpec->pos.vals[0], pFilter_Request->matchesRemaining);


    // Allocate Queue for collecting section data
    pFilter_Request->m_SectionsDataQueue = rmf_queue_new();
#if 0    
    snprintf( (char*)qname, MAX_SF_EVTQ_NAME_LEN, "sectionsDataQueue");;
    retOsal = rmf_osal_eventqueue_create(qname, &pFilter_Request->sectionsDataQueue);
    if(retOsal != RMF_SUCCESS)
    {
        rmf_osal_memFreeP(RMF_OSAL_MEM_FILTER, pFilter_Request);
        rmf_osal_mutexRelease(m_sectionRequestMutex);
        return retOsal;
    }
#endif
    // Keep trying to set this filter until we can't preempt any
    // lower priority filters
    do
    {
        // Attempt to set the filter
        returnCode = Create(pid, filterSpec->pos.mask, filterSpec->pos.vals, filterSpec->pos.length,
                filterSpec->neg.mask, filterSpec->neg.vals, filterSpec->neg.length, &pFilter_Request->pSectionFilterInfo, &filterID);
        // Failed to create the filter, so try to pre-empt a lower priority
        if (returnCode == RMF_SECTFLT_NoFilterRes)
        {
            // Iterate the SectionRequest list to find out a low priority Section Filter
            // Cancel the filter
            //                      VLSymbolMapTableIter    iter;
            //                      VLSymbolMapKey                  key;
            //                      rmf_sf_SectionRequest_t  *pRequest;
            rmf_osal_Bool filterPreempted = FALSE;

            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.FILTER","%s -- No filters available!  Checking for lower priority filters.  This priority = %d\n", __FUNCTION__, filterPriority);
            // Search through the list of current filter requests
            // looking for one with lower priority and cancel it.
            filterID = getLowestPriorityFilter(filterPriority);
            /*
             */
            if(-1 != filterID)
            {
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.FILTER", "%s -- Pre-empting this filter!, cancelling filterID = %d\n", __FUNCTION__, filterID);

                cancelFilter(filterID, RMF_SF_EVENT_FILTER_PREEMPTED);
                filterPreempted = TRUE;
            }

            // If we could not preempt any filters then we will not be
            // able to set this new filter
            if (!filterPreempted)
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILTER","%s -- No filters could be pre-empted!\n", __FUNCTION__);
		rmf_queue_free(pFilter_Request->m_SectionsDataQueue);
                // Release our mutex and free the section request
                rmf_osal_memFreeP(RMF_OSAL_MEM_FILTER, pFilter_Request);
                return RMF_SF_ERROR_FILTER_NOT_AVAILABLE;
            }
        }
        else 
        {// Success or other failures
            break;
        }
    }
    while (returnCode == RMF_SECTFLT_NoFilterRes);

    if(returnCode != RMF_SECTFLT_Success)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILTER","%s -- No filters could be creted!\n", __FUNCTION__);
	rmf_queue_free(pFilter_Request->m_SectionsDataQueue);
        // Release our mutex and free the section request
        rmf_osal_memFreeP(RMF_OSAL_MEM_FILTER, pFilter_Request);
        return RMF_SF_ERROR_FILTER_NOT_AVAILABLE;
    }

    //vl_symbolMapTable_insert(m_sectionRequests, (VLSymbolMapKey)filterID, pFilter_Request);
    pFilter_Request->state = RMF_SF_REQSTATE_READY;

    rmf_osal_mutexAcquire(m_sectionRequestMutex);

    m_sectionRequests->Insert((rmf_SymbolMapKey)filterID, pFilter_Request);

    rmf_osal_mutexRelease(m_sectionRequestMutex);

    // Section Filter is created successfully
    // Start the section filter now
    returnCode = Start(filterID);

    if (returnCode != RMF_SECTFLT_Success)
    {
        // Failed to start the section filter
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILTER","%s:: SECTFLT_Start not success\n", __FUNCTION__);

        ReleaseFilter(filterID);
        return RMF_SF_ERROR_FILTER_NOT_AVAILABLE;
    }
    // Section Filter Start is also successful..

    // Acquire this mutex so that we do not process any section
    // events until we have added the request structure
    //      DEBUG_INFO("%s:: inserting filter request\n", __FUNCTION__);
    // Insert the new request into our request table

    // Add starttime
    getTimeInMilliSecond(&(pFilter_Request->m_startTimeInms_SinceEpoch));


    // Return the filterID to the caller
    *uniqueId = filterID; //filterID;
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.FILTER", "%s:: End  filterID = %d\n", __FUNCTION__, filterID);

    return RMF_SUCCESS;
}

/**
 * @brief This function cancels the filter associated with the specified uniqueId.
 *
 * @param[in] uniqueId Identifies the filter to be cancelled.
 *
 * @return Return value indicates success or failure of the call.
 * @retval RMF_SUCCESS Indicates the call is successful.
 * @retval RMF_OSAL_EINVAL Indicates unique ID is invalid.
 */
rmf_Error rmf_SectionFilter::CancelFilter(uint32_t uniqueId)
{
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.FILTER", "%s:: Called for uniqueId %d \n", __FUNCTION__, uniqueId);

    rmf_Error ret_val = RMF_SUCCESS;
    //rmf_osal_mutexAcquire(m_sectionRequestMutex);

    ret_val = cancelFilter(uniqueId, RMF_SF_EVENT_FILTER_CANCELLED);

    //rmf_osal_mutexRelease(m_sectionRequestMutex);
    return ret_val;
}

/**
 * @brief This function reads the section data corresponding to the unique ID and inserts it
 * in the list of outstanding sections. And returns a handle/key to the inserted section data.
 *
 * @param[in] uniqueId To identify the section filtering request.
 * @param[in] flags Specifies options for retrieving the handle.
 * @param[out] pSectionHandle To be filled with section data's handle.
 *
 * @return Returns success or failure of the call.
 * @retval RMF_SUCCESS Indicates the call was successful and the handle is returned.
 * @retval RMF_OSAL_EINVAL Indicates input parameters are invalid.
 * @retval RMF_SF_ERROR_SECTION_NOT_AVAILABLE Indicates the section is not available or the
 * RMF_SF_OPTION_IF_NOT_CANCELLED option is indicated and the filter is cancelled.
 */
rmf_Error rmf_SectionFilter::GetSectionHandle(uint32_t uniqueId, uint32_t flags,
rmf_FilterSectionHandle * pSectionHandle)
{
    rmf_sf_SectionRequest_t* pFilter_Request = NULL;
    rmf_Section_Data*                pSectionData    = NULL;
    RMF_SECTFLT_RESULT               retCode                 = RMF_SECTFLT_Error;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILTER", "%s:: Called\n", __FUNCTION__);

    if (NULL == pSectionHandle)
    {
        return RMF_OSAL_EINVAL;
    }

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILTER", "%s:: uniqueId = %d, 0x%x\n", __FUNCTION__, uniqueId, uniqueId);

    rmf_osal_mutexAcquire(m_sectionRequestMutex);

    // Find the section request for this filter
    pFilter_Request = (rmf_sf_SectionRequest_t*)m_sectionRequests->Lookup(uniqueId);
        
    rmf_osal_mutexRelease(m_sectionRequestMutex);

    // Can not find section request
    if (NULL == pFilter_Request)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILTER","%s -- Could not find section request (%d)!\n",__FUNCTION__, uniqueId);
        return RMF_OSAL_EINVAL;
    }

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILTER", "%s:: pFilter_Request = 0x%x\n", __FUNCTION__, pFilter_Request);

    // Pull the next section off of the incoming section queue
    if(RMF_SF_REQSTATE_CANCELLED == pFilter_Request->state)
    {
        if (RMF_SF_OPTION_IF_NOT_CANCELLED == flags)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILTER","%s:: bad flags & returning  MPE_SF_ERROR_SECTION_NOT_AVAILABLE\n", __FUNCTION__);
            return RMF_SF_ERROR_SECTION_NOT_AVAILABLE;
        }
    }

    retCode = Read(uniqueId, &pSectionData);

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILTER","%s:: pSectionData: 0x%x\n", __FUNCTION__, pSectionData);


    // Is there a section available?
    if ((RMF_SECTFLT_Success != retCode) || (NULL == pSectionData))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILTER","%s:: pSectionData == NULL\n", __FUNCTION__);
        return RMF_SF_ERROR_SECTION_NOT_AVAILABLE;
    }

    rmf_osal_mutexAcquire(m_outstandingSectionMutex);

    // Add this section to our list of outstanding sections and return its ID as the section handle
    m_outstandingSections->Insert((rmf_SymbolMapKey)(pSectionData->sectionID), pSectionData);

    rmf_osal_mutexRelease(m_outstandingSectionMutex);

    // Return the sectionID as the handle
    *pSectionHandle = (rmf_FilterSectionHandle)(pSectionData->sectionID);

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILTER", "%s:: pSectionHandle = %d\n", __FUNCTION__, *pSectionHandle);

    return RMF_SUCCESS;
}

/**
 * @brief This function will release the filter and any associated resources including any sections that
 * have been matched but not yet retrieved.
 *
 * @param[in] uiFilterReqHandle Handle to the Filter_Request to relesae
 *
 * @return Return value indicates success or failure of the call.
 * @retval RMF_SUCCESS Indicates the call was successful and the filter is released.
 * @retval RMF_OSAL_EINVAL Indicates parameter is invalid
 */
rmf_Error rmf_SectionFilter::ReleaseFilterRequest(uint32_t uiFilterReqHandle) 
{
	rmf_Error eRet = RMF_OSAL_EINVAL;
    rmf_Section_Data *pSectionData = NULL;
	rmf_sf_SectionRequest_t* pFilter_Request = (rmf_sf_SectionRequest_t*)uiFilterReqHandle;
	if(pFilter_Request)
	{
#ifndef EXTERNAL_MEMORY_POOL_ALLOCATE
		if (NULL != pFilter_Request->pSectionFilterInfo)
		{
			//vlFree(m_pSectionFilterInfo);
			rmf_osal_memFreeP(RMF_OSAL_MEM_FILTER, pFilter_Request->pSectionFilterInfo); 
		}
#endif

		while(NULL != (pSectionData = (rmf_Section_Data*)rmf_queue_pop_head(pFilter_Request->m_SectionsDataQueue)))
		{
			if(pSectionData->sectionData)
			{
				rmf_osal_memFreeP(RMF_OSAL_MEM_FILTER, pSectionData->sectionData);
			}
			rmf_osal_memFreeP(RMF_OSAL_MEM_FILTER, pSectionData);
		}

		rmf_queue_clear(pFilter_Request->m_SectionsDataQueue);
		rmf_queue_free(pFilter_Request->m_SectionsDataQueue); // fix for memory leak and mutex leak
#if 0            
		rmf_osal_eventqueue_clear(pFilter_Request->sectionsDataQueue);
		rmf_osal_eventqueue_delete(pFilter_Request->sectionsDataQueue);
#endif            
		rmf_osal_memFreeP(RMF_OSAL_MEM_FILTER, pFilter_Request);
		eRet = RMF_SUCCESS;
	}
	return eRet;
}

/**
 * @brief This function will release the filter and any associated resources including any sections that
 * have been matched but not yet retrieved.
 *
 * @param[in] uiFilterReqHandle Handle to the Filter_Request to relesae
 *
 * @return Return value indicates success or failure of the call.
 * @retval RMF_SUCCESS Indicates the call was successful and the filter is released.
 * @retval RMF_OSAL_EINVAL Indicates parameter is invalid
 */
bool rmf_SectionFilter::IsDeferredDeleteQueueEmpty() 
{
    bool ret = true;
	rmf_SymbolMapTableIter    iter;
	rmf_SymbolMapKey          key;
	rmf_sf_SectionRequest_t  *pRequest;
	m_sectionRequestsToRemove->IterInit(&iter);

	while (NULL != (pRequest = (rmf_sf_SectionRequest_t*)m_sectionRequestsToRemove->IterNext(&iter, &key)))
	{
        if (1 == pRequest->uiIsSectionUnderProcessing) {
            ret = false;
        }
    }
    return ret;
}

/**
 * @brief This function will release the filter and any associated resources including any sections that
 * have been matched but not yet retrieved.
 *
 * @param[in] uiFilterReqHandle Handle to the Filter_Request to relesae
 *
 * @return Return value indicates success or failure of the call.
 * @retval RMF_SUCCESS Indicates the call was successful and the filter is released.
 * @retval RMF_OSAL_EINVAL Indicates parameter is invalid
 */
rmf_Error rmf_SectionFilter::ReleaseFromDeferredDeleteQueue() 
{
	rmf_Error eRet = RMF_OSAL_EINVAL;
	do {

        rmf_SymbolMapTableIter    iter;
        rmf_SymbolMapKey                  key;
        rmf_sf_SectionRequest_t  *pRequest;
        //vl_symbolMapTable_iter_init(&iter, m_sectionRequests);
        m_sectionRequestsToRemove->IterInit(&iter); 
		//Mutex locking is not done since insert and remove from same thread
        while (NULL != (pRequest = (rmf_sf_SectionRequest_t*)m_sectionRequestsToRemove->IterNext(&iter, &key)))
        {
			if (0 == pRequest->uiIsSectionUnderProcessing) {
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.FILTER","Removed from m_sectionRequestsToRemove pFilter_Request= %p\n", pRequest);
				//Request processing is finised. Remove fom the queue and delete the request
				//Mutex locking is not done since insert and remove from same thread
                m_sectionRequestsToRemove->Remove((rmf_SymbolMapKey)key);
				ReleaseFilterRequest((uint32_t) pRequest);
			}
        }
		eRet = RMF_SUCCESS;

	}while (0);
	return eRet;
}

/**
 * @brief This function will release the filter and any associated resources including any sections that
 * have been matched but not yet retrieved.
 *
 * @param[in] uniqueId Identifies the filter to be released.
 *
 * @return Return value indicates success or failure of the call.
 * @retval RMF_SUCCESS Indicates the call was successful and the filter is released.
 * @retval RMF_OSAL_EINVAL Indicates unique ID parameter passed as input is invalid.
 */
rmf_Error rmf_SectionFilter::ReleaseFilter(uint32_t uniqueId)
{
        rmf_Section_Data *pSectionData = NULL;
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.FILTER", "%s:: Called (not by mpeos) for uniqueId %d \n", __FUNCTION__, uniqueId);

        rmf_sf_SectionRequest_t* pFilter_Request = NULL;

        //      DEBUG_INFO("%s:: Entered\n", __FUNCTION__);
        rmf_osal_mutexAcquire(m_sectionRequestMutex);

        // Validate filterID
        //pFilter_Request = (rmf_sf_SectionRequest_t *)vl_symbolMapTable_lookup(m_sectionRequests, (VLSymbolMapKey)uniqueId);
        pFilter_Request = (rmf_sf_SectionRequest_t *)m_sectionRequests->Lookup((rmf_SymbolMapKey)uniqueId);

        rmf_osal_mutexRelease(m_sectionRequestMutex);

        if (NULL == pFilter_Request)
        {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILTER","%s:: pFilter_Request = NULL\n", __FUNCTION__);
                return RMF_OSAL_EINVAL;
        }

        unsigned long long endTime = 0;

        getTimeInMilliSecond(&endTime);

        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.FILTER","Elapsed time for filter %d is %lld msec\n", uniqueId, (endTime - pFilter_Request->m_startTimeInms_SinceEpoch));

        // Make sure the filter is canceled
        if (pFilter_Request->state != RMF_SF_REQSTATE_CANCELLED)
        {
        //              DEBUG_INFO("VL_mpeosImpl_filterRelease:: vl_CancelFilter calling\n");
                RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILTER","Calling cancelFilter\n");
                cancelFilter(uniqueId, RMF_SF_EVENT_FILTER_CANCELLED);
        }

        rmf_osal_mutexAcquire(m_sectionRequestMutex);
        // Remove the request structure
        //pFilter_Request = (rmf_sf_SectionRequest_t *)vl_symbolMapTable_remove(m_sectionRequests, (VLSymbolMapKey)uniqueId);
        pFilter_Request = (rmf_sf_SectionRequest_t *)m_sectionRequests->Remove((rmf_SymbolMapKey)uniqueId);

        rmf_osal_mutexRelease(m_sectionRequestMutex);
        //      DEBUG_INFO("VL_mpeosImpl_filterRelease:: freeing filter request...\n");
        
		if (pFilter_Request){

			//If filter request is processed now It will be pused to the temporary queue and will be removed once processing is done
			if (0 == pFilter_Request->uiIsSectionUnderProcessing) {
				ReleaseFilterRequest ((uint32_t)pFilter_Request);
			}
			else
			{
				/*If not already inserted insert*/
				if (NULL == m_sectionRequestsToRemove->Lookup((rmf_SymbolMapKey)uniqueId)) {
					RDK_LOG(RDK_LOG_INFO, "LOG.RDK.FILTER","Inserted to m_sectionRequestsToRemove pFilter_Request= %p\n", pFilter_Request);
					//Mutex locking is not done since insert and remove from same thread
					m_sectionRequestsToRemove->Insert((rmf_SymbolMapKey)uniqueId, pFilter_Request);
				}
			}
		}

		ReleaseFromDeferredDeleteQueue (); 

    return RMF_SUCCESS;
}

/**
 * @brief This function searches for the section data specified by the sectionHandle in the list of
 * outstanding sections and gets the length of the section.
 *
 * @param[in] sectionHandle To identify the section in the list of outstanding sections.
 * @param[out] pSize Pointer to be updated with the section length.
 *
 * @return Return value indicates success or failure of the call.
 * @retval RMF_SUCCESS Indicates call was successful.
 * @retval RMF_OSAL_EINVAL Indicates psize parameter passes was NULL.
 * @retval RMF_SF_ERROR_INVALID_SECTION_HANDLE Indicates an invalid section handle was specified.
 */
rmf_Error rmf_SectionFilter::GetSectionSize(rmf_FilterSectionHandle sectionHandle,
        uint32_t * pSize)
{
    rmf_Section_Data* pSectionData               = NULL;

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.FILTER", "%s:: Called\n", __FUNCTION__);

    if (sectionHandle == 0)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILTER","%s:: Invalid Secion Handle\n", __FUNCTION__);
        return RMF_SF_ERROR_INVALID_SECTION_HANDLE;
    }

    if (NULL == pSize)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILTER","%s:: Invalid params\n", __FUNCTION__);
        return RMF_OSAL_EINVAL;
    }

    rmf_osal_mutexAcquire(m_outstandingSectionMutex);

    // Retrieve the section size
    pSectionData = (rmf_Section_Data*)m_outstandingSections->Lookup((rmf_SymbolMapKey)sectionHandle);

    if (pSectionData == NULL)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILTER","%s:: pSectionData == NULL\n", __FUNCTION__);
        rmf_osal_mutexRelease(m_outstandingSectionMutex);
        return RMF_OSAL_EINVAL;
    }

    *pSize = pSectionData->sectionLength;

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.FILTER", "%s:: *pSize = %d\n", __FUNCTION__, *pSize);

    rmf_osal_mutexRelease(m_outstandingSectionMutex);

    return RMF_SUCCESS;
}

/**
 * @brief This function copies the section data from the list of outstanding sections,
 * corresponding to the specified sectionHandle and finally releases the section depending
 * on the flags parameter.
 *
 * @param[in] sectionHandle Identifies the handle assigned to the section data.
 * @param[in] offset Designates the offset to read from within the section.
 * @param[in] byteCount Designates the number of bytes to read from the section. This can be larger
 * than the size of the section (for example, the target buffer size).
 * @param[in] flags Indicates whether to delete the section data or not.
 * @param[out] buffer A pointer to the buffer where the section data is to be copied.
 * @param[out] bytesRead A pointer to the destination for the number of bytes actually copied.
 *
 * @return Return value specifies success or failure
 * @retval RMF_SUCCESS Indicates call is successful.
 * @retval RMF_OSAL_EINVAL Indicates buffer is null or byteCount is 0 or offset is equal/greater
 * than the section size.
 * @retval RMF_SF_ERROR_INVALID_SECTION_HANDLE Indicates an invalid section handle was specified.
 */
rmf_Error rmf_SectionFilter::ReadSection(rmf_FilterSectionHandle sectionHandle,
        uint32_t offset, uint32_t byteCount, uint32_t flags, uint8_t * buffer,
        uint32_t * bytesRead)
{
    rmf_Section_Data* pSectionData   = NULL;
    //      DEBUG_INFO_1("%s:: Called\n", __FUNCTION__);

    uint32_t bytesToCopy;
    uint32_t sectionLength;
    rmf_osal_mutexAcquire(m_outstandingSectionMutex);

    pSectionData = (rmf_Section_Data*)m_outstandingSections->Lookup((rmf_SymbolMapKey)sectionHandle);

    // Validate section handle
    if (NULL == pSectionData)
    {
        rmf_osal_mutexRelease(m_outstandingSectionMutex);
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILTER","%s:: invalid section handle (%d)\n", __FUNCTION__, sectionHandle);
        return RMF_SF_ERROR_INVALID_SECTION_HANDLE;
    }

    sectionLength = pSectionData->sectionLength;

    // Validate all other arguments
    if (buffer == NULL || offset > sectionLength || byteCount == 0)
    {
        rmf_osal_mutexRelease(m_outstandingSectionMutex);
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILTER","%s:: invalid params\n", __FUNCTION__);
        return RMF_OSAL_EINVAL;
    }

    // Validate offset
    if (offset >= sectionLength)
    {
        rmf_osal_mutexRelease(m_outstandingSectionMutex);
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILTER","%s:: offset larger than size (%d >= %d)\n", __FUNCTION__, offset, sectionLength);
        return RMF_OSAL_EINVAL;
    }

    // Determine the number of bytes to copy.
    bytesToCopy = (byteCount < (sectionLength-offset)) ?
        byteCount : sectionLength-offset;

    //      DEBUG_INFO("VL_mpeos_filterSectionRead:: section len = %d, Copying %d bytes from offset = %d\n", sectionLength, bytesToCopy, offset);
    // Copy the data
    //vlMemCpy2(buffer, (pSectionData->sectionData)+offset, bytesToCopy);
    memcpy(buffer, (pSectionData->sectionData)+offset, bytesToCopy);

    rmf_osal_mutexRelease(m_outstandingSectionMutex);

    // The contract for this function says that the user can pass NULL for
    // this pointer.
    if (bytesRead != NULL)
    {
        *bytesRead = bytesToCopy;
    }

    // Should we release the section?
    if (flags & RMF_SF_OPTION_RELEASE_WHEN_COMPLETE)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.FILTER", "%s:: MPE_SF_OPTION_RELEASE_WHEN_COMPLETE passed and calling ReleaseSection \n", __FUNCTION__);

        ReleaseSection(sectionHandle);
    }

    return RMF_SUCCESS;
}

/**
 * @brief This function removes the section identified by sectionHandle from the list of outstanding
 * sections and releases the memory used by this section.
 *
 * @param[in] sectionHandle Specifies the handle to the section to be released.
 * mpeos_filterGetSectionHandle()
 *
 * @return Return value indicates success or failure of the call.
 * @retval RMF_SUCCESS Indicates call was successful.
 * @retval RMF_SF_ERROR_INVALID_SECTION_HANDLE Indicates an invalid section handle was specified.
 */
rmf_Error rmf_SectionFilter::ReleaseSection(rmf_FilterSectionHandle sectionHandle)
{
    rmf_Section_Data* pSectionData               = NULL;

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.FILTER", "%s:: Called\n", __FUNCTION__);

    rmf_osal_mutexAcquire(m_outstandingSectionMutex);

    // Remove this section from our outstanding sections list
    pSectionData = (rmf_Section_Data *)m_outstandingSections->Remove((rmf_SymbolMapKey)sectionHandle);

    rmf_osal_mutexRelease(m_outstandingSectionMutex);

    if(pSectionData)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.FILTER", "%s:: Freeing pSectionData\n", __FUNCTION__);

        if(pSectionData->sectionData)
        {
            rmf_osal_memFreeP(RMF_OSAL_MEM_FILTER, pSectionData->sectionData);
        }
        rmf_osal_memFreeP(RMF_OSAL_MEM_FILTER, pSectionData);
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILTER","%s - invalid section handle (%d)\n",
                __FUNCTION__, sectionHandle);
        return RMF_SF_ERROR_INVALID_SECTION_HANDLE;
    }
    return RMF_SUCCESS;
}

/**
 * @brief This is a callback function to notify the application that a new section is available.
 * 
 * @param[in] filterID Indicates handle to identify the section filter in the list of section
 * requests to which the pSectionData needs to be added.
 * @param[in] pSectionData Points to the section data to be added.
 *
 * @return None
 */
void rmf_SectionFilter::section_avail_cb(uint32_t filterID, rmf_Section_Data* pSectionData)
{

#if 0
    if (m_cb_func) {
        printf("Filter cabllback intercepted\r\n");
        m_cb_func(filterID, pSectionData);
        return;
    }
#endif

    rmf_sf_SectionRequest_t* pFilter_Request;
    rmf_osal_Bool shouldCancelFilter = FALSE;
    int filterEvent = RMF_SF_EVENT_SECTION_FOUND;
    rmf_Error error;
	bool bIsSecProcessingStatusChanged = false; 

    if(NULL == pSectionData)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILTER", 
            "section_avail_cb:: pSectionData is NULL\n");        
        return;
    }
    
    rmf_osal_mutexAcquire(m_sectionRequestMutex);

     RDK_LOG(RDK_LOG_INFO, "LOG.RDK.FILTER", 
        "vl_section_avail_cb:: filterID = %d, TableID = 0x%x\n", 
        filterID, pSectionData->sectionData[0]);
    // Find the sectionRequest structure that corresponds to the given filter ID
    //pFilter_Request = (rmf_sf_SectionRequest_t*)vl_symbolMapTable_lookup(m_sectionRequests, (VLSymbolMapKey)filterID);
    pFilter_Request = (rmf_sf_SectionRequest_t*)m_sectionRequests->Lookup((rmf_SymbolMapKey)filterID);

	if (NULL!=pFilter_Request) { 
		//Setting the tag to avoid deleting of filter request under processing. Status is not locked since its modified inside the function 
		pFilter_Request->uiIsSectionUnderProcessing = 1;    
        bIsSecProcessingStatusChanged = true; 
	}

    rmf_osal_mutexRelease(m_sectionRequestMutex);

    // This filter must have already been released or cancelled.
    if (pFilter_Request == NULL || pFilter_Request->state == RMF_SF_REQSTATE_CANCELLED ||
            pFilter_Request->state == RMF_SF_REQSTATE_MATCHED)
    {
             RDK_LOG(RDK_LOG_INFO, "LOG.RDK.FILTER", 
        "vl_section_avail_cb:: pFilter_Request = 0x%x\n", pFilter_Request);
        if(pSectionData)
        {
            if (NULL != pSectionData->sectionData)
            {
                rmf_osal_memFreeP(RMF_OSAL_MEM_FILTER, pSectionData->sectionData);
            }
            rmf_osal_memFreeP(RMF_OSAL_MEM_FILTER, pSectionData);
            pSectionData = NULL;
        }

    	//Indicating filter processing finished 
		if (bIsSecProcessingStatusChanged) { 
			pFilter_Request->uiIsSectionUnderProcessing = 0; 
		}
        return;
    }


     RDK_LOG(RDK_LOG_INFO, "LOG.RDK.FILTER", 
        "vl_section_avail_cb:: filterID = %d, TableID = 0x%x, pFilter_Request->matchesRemaining: %d\n", 
        filterID, pSectionData->sectionData[0], pFilter_Request->matchesRemaining);

    // Check the times-to-match value.  If we have matched the last section,
    // go ahead and cancel the filter
    if (pFilter_Request->matchesRemaining > 0)
    {
        pFilter_Request->matchesRemaining--;
        if (pFilter_Request->matchesRemaining == 0) // Matched last section
        {
            pFilter_Request->state = RMF_SF_REQSTATE_MATCHED;
            filterEvent = RMF_SF_EVENT_LAST_SECTION_FOUND;

            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.FILTER", "%s -> RMF_SF_REQSTATE_MATCHED sending RMF_SF_EVENT_LAST_SECTION_FOUND for filterID = %d \n", __FUNCTION__, filterID);

            //shouldCancelFilter = TRUE;
        }
    }

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILTER","%s: pFilter_Request: 0x%x, pFilter_Request->sectionsDataQueue: 0x%x\n", __FUNCTION__, pFilter_Request, pFilter_Request->m_SectionsDataQueue);
#if 0
    if(RMF_SUCCESS != queue_push_tail(pFilter_Request->sectionsDataQueue, pSectionData))
#else
    if(-1 == rmf_queue_push_tail(pFilter_Request->m_SectionsDataQueue, pSectionData, 0, pSectionData))
#endif
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILTER","%s Failed to add section data to sectionsDataQueue (0x%x) \n", __FUNCTION__, pFilter_Request->m_SectionsDataQueue);
        
        if ( (shouldCancelFilter) || (pFilter_Request->matchesRemaining > 0))
        {
            pFilter_Request->matchesRemaining++;
            shouldCancelFilter = FALSE;
        }
            
        if(pSectionData)
        {
            if (NULL != pSectionData->sectionData)
            {
                rmf_osal_memFreeP(RMF_OSAL_MEM_FILTER, pSectionData->sectionData);
            }
            
            rmf_osal_memFreeP(RMF_OSAL_MEM_FILTER, pSectionData);
            pSectionData = NULL;
        }
		//Indicating filter processing finished 
		if (bIsSecProcessingStatusChanged) { 
			pFilter_Request->uiIsSectionUnderProcessing = 0; 
		}
        return;
    }

    #if 0
    if (NULL != pSectionData)
    {
        DumpBuffer(pSectionData->sectionData, pSectionData->sectionLength);
    }
    #endif

    // Post the event to the requestors event queue

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILTER", "Sending MPE_SF_EVENT_SECTION_FOUND or MPE_SF_EVENT_LAST_SECTION_FOUND for filterID = (dec) %d  Table ID 0x%x\n", filterID, pSectionData->sectionData[0]);
    //if ((error = mpeos_eventQueueSend(pFilter_Request->requestorQueue, filterEvent, (void*)filterID, pFilter_Request->requestorACT, 0)) != MPE_SUCCESS)
    if ((error = dispatchEvent(pFilter_Request->requestorQueue, filterEvent, pFilter_Request->priority, filterID)) != RMF_SUCCESS)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILTER","mpeos_eventQueueSend Failed with error %d \n", error);

        if ( (shouldCancelFilter) || (pFilter_Request->matchesRemaining > 0))
        {
            pFilter_Request->matchesRemaining++;
            shouldCancelFilter = FALSE;
        }
#if 0        
        pSectionData = (rmf_Section_Data*)queue_pop_head(pFilter_Request->sectionsDataQueue);
#else
        pSectionData = (rmf_Section_Data*)rmf_queue_pop_head(pFilter_Request->m_SectionsDataQueue);
#endif
        if(NULL != pSectionData)
        {
            if (NULL != pSectionData->sectionData)
            {
                rmf_osal_memFreeP(RMF_OSAL_MEM_FILTER, pSectionData->sectionData);
            }
            rmf_osal_memFreeP(RMF_OSAL_MEM_FILTER, pSectionData);
            pSectionData = NULL;
        }
    }
	//Indicating filter processing finished  
	if (bIsSecProcessingStatusChanged) {
		pFilter_Request->uiIsSectionUnderProcessing = 0; 
	}

    // Do we need to cancel the filter because we're fully matched?
    if (shouldCancelFilter)
    {
        // Do NOT dispatch an event
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.FILTER", "SF_REQSTATE_MATCHED Calling vl_CancelFilter for filterID %d (0x%x)\n", filterID, filterID);
        cancelFilter(filterID, RMF_SF_EVENT_UNKNOWN);
    }

}

/**********************************************************************************************************/

/**
 * @brief This function is used to get filterID whose priority is lowest among the list
 * of section requests compared to the priority specified by filterPriority parameter.
 * Where 1 is considered as highest priority.
 *
 * @param[in] filterPriority Indicates the priority of the filter to be compared.
 *
 * @return Returns filterID of the lowest priority filter.
 */
uint32_t rmf_SectionFilter::getLowestPriorityFilter(uint32_t filterPriority)
{
        uint32_t filterID = -1;
        //VLSymbolMapTableIter    iter;
        //VLSymbolMapKey                  key;
        rmf_SymbolMapTableIter    iter;
        rmf_SymbolMapKey                  key;
        rmf_sf_SectionRequest_t  *pRequest;
        rmf_osal_mutexAcquire(m_sectionRequestMutex); 
        //vl_symbolMapTable_iter_init(&iter, m_sectionRequests);
        m_sectionRequests->IterInit(&iter);
        
                        
        //while (NULL != (pRequest = (rmf_sf_SectionRequest_t*)vl_symbolMapTable_iter_next(&iter, &key)))
        while (NULL != (pRequest = (rmf_sf_SectionRequest_t*)m_sectionRequests->IterNext(&iter, &key)))
        {

                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.FILTER","getLowestPriorityFilter -- Checking if this filter can be pre-empted.  Prioriy = %d\n", pRequest->priority);

        // Check for lower priority (1 is the highest priority)
                if (pRequest->priority > filterPriority)
                {
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.FILTER","getLowestPriorityFilter -- Pre-empting this filter!\n");
                        filterPriority = pRequest->priority;
                        filterID = key;
                }
        }
        rmf_osal_mutexRelease(m_sectionRequestMutex);
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.FILTER","getLowestPriorityFilter:: filterID = %x\n", filterID);

        return filterID;
}

/**********************************************************************************************************/
// Internal routine for cancelling filters, handles the dispatching
// of the given event.  If the filterEvent is RMF_SF_EVENT_UNKNOWN,
// no event is dispatched.
/**********************************************************************************************************/

/**
 * @brief This function sets the filter state to RMF_SF_REQSTATE_CANCELLED and then releases
 * the filter. After releasing the filter, it dispatches an event indicated by the filterEvent
 * parameter to the requestor queue, only if it is not set to RMF_SF_EVENT_UNKNOWN.
 *
 * @param[in] filterID Indicates the filter to be cancelled.
 * @param[in] filterEvent Specifies an event to be sent to the requestor queue.
 *
 * @return Return value indicates success or failure.
 * @retval RMF_SUCCESS Indicates the call was successful.
 * @retval RMF_OSAL_EINVAL Indicates either Invalid filter ID or the filter is already in cancelled state.
 */
rmf_Error rmf_SectionFilter::cancelFilter(uint32_t filterID, int filterEvent)
{
        // ASSERT:  sectionRequestMutex is held by the caller
        rmf_Error error;
        rmf_sf_SectionRequest_t*         pFilter_Request         = NULL;

        rmf_osal_mutexAcquire(m_sectionRequestMutex);

        // Find the section request structure
        //pFilter_Request = (rmf_sf_SectionRequest_t*)vl_symbolMapTable_lookup(m_sectionRequests, (VLSymbolMapKey)filterID);
        pFilter_Request = (rmf_sf_SectionRequest_t*)m_sectionRequests->Lookup((rmf_SymbolMapKey)filterID);

        rmf_osal_mutexRelease(m_sectionRequestMutex);

        // If we can not find the request, or if it has already been
        // canceled, let the caller know
        if (pFilter_Request == NULL || pFilter_Request->state == RMF_SF_REQSTATE_CANCELLED)
        {
                return RMF_OSAL_EINVAL;
        }

        pFilter_Request->state = RMF_SF_REQSTATE_CANCELLED;

        Release(filterID);

        // Post correct event to mpeos queue
        if (filterEvent != RMF_SF_EVENT_UNKNOWN)
        {

                //if ((error = mpeos_eventQueueSend(pFilter_Request->requestorQueue,
                //                                                                        filterEvent,
                //                                                                        (void*)filterID, pFilter_Request->requestorACT,
                 //                                                                       0)) != MPE_SUCCESS)

        if ((error = dispatchEvent(pFilter_Request->requestorQueue, filterEvent, pFilter_Request->priority, filterID)) != RMF_SUCCESS)
                {
                        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILTER","Could not post cancelled/preempted event to client queue!\n");
                }
        }
        return RMF_SUCCESS;
}

void rmf_SectionFilter::getTimeInMilliSecond(unsigned long long *pMillisSinceEpoch)
{
        struct timeval tv;
        //unsigned long long millisSinceEpoch;

        gettimeofday (&tv, NULL);
        //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.FILTER", "Seconds = %ld\n", tv.tv_sec);
        //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.FILTER", "Micro Seconds = %ld\n", tv.tv_usec);

        *pMillisSinceEpoch = tv.tv_sec * (unsigned long long)1000;
        *pMillisSinceEpoch += tv.tv_usec / (unsigned long long)1000;
}

rmf_Error rmf_SectionFilter::dispatchEvent(rmf_osal_eventqueue_handle_t queueHandle, uint32_t filterEvent, uint8_t priority, uint32_t filterID)
{

        rmf_osal_event_params_t event_params = {0};
    rmf_osal_event_handle_t eventHandle;
    rmf_Error retOsal = RMF_SUCCESS;
                
    event_params.priority = priority;
    //event_params.data = (void*)&filterID;
    event_params.data_extension = filterID;

    do
    {
        retOsal = rmf_osal_event_create(RMF_OSAL_EVENT_CATEGORY_MPEG, filterEvent, &event_params, &eventHandle);
        if(retOsal != RMF_SUCCESS)
        {
            RDK_LOG(
                    RDK_LOG_DEBUG,
                    "LOG.RDK.JNI",
                    "<%s> Could not create event handle: %x\n",
                    __FUNCTION__, retOsal);
            return retOsal;
        }

        retOsal = rmf_osal_eventqueue_add(queueHandle, eventHandle);        
        if(retOsal != RMF_SUCCESS)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILTER","%s: Cannot create RMF OSAL Event (0x%x) to Queue (0x%x)\n", __FUNCTION__, eventHandle, queueHandle);
            rmf_osal_event_delete(eventHandle);
            break;
        }

    }while(0);

    return retOsal;
}

#if 0
rmf_Error rmf_SectionFilter::queue_push_tail(rmf_osal_eventqueue_handle_t queueHandle, void* pData)
{
        rmf_osal_event_params_t event_params = {0};
    rmf_osal_event_handle_t eventHandle;
    rmf_Error retOsal = RMF_SUCCESS;
    uint32_t filterEvent = RMF_SF_EVENT_UNKNOWN;

    do
    {
        event_params.data = pData;
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILTER","%s: event_params.data = 0x%x, pData = 0x%x\n", __FUNCTION__, event_params.data, pData);

        retOsal = rmf_osal_event_create(RMF_OSAL_EVENT_CATEGORY_MPEG, filterEvent, &event_params, &eventHandle);
        if(retOsal != RMF_SUCCESS)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILTER","%s: Cannot create RMF OSAL Event \n", __FUNCTION__);
            break;  
        }

        retOsal = rmf_osal_eventqueue_add(queueHandle, eventHandle);        
        if(retOsal != RMF_SUCCESS)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILTER","%s: Cannot add RMF OSAL Event \n", __FUNCTION__);
            rmf_osal_event_delete(eventHandle);
            break;
        }

    }while(0);

    return retOsal; 
}


void* rmf_SectionFilter::queue_pop_head(rmf_osal_eventqueue_handle_t queueHandle)
{
    rmf_osal_event_handle_t eventHandle;
    rmf_osal_event_params_t eventParams;
        uint32_t eventType;
    rmf_Error retOsal = RMF_SUCCESS;
    void *pData = NULL;

    retOsal = rmf_osal_eventqueue_try_get_next_event(queueHandle, &eventHandle, NULL, &eventType, &eventParams);
    if(retOsal != RMF_SUCCESS)
    {
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILTER","%s: No event found in queue, retOsal: %d\n", __FUNCTION__, retOsal);
    }
    else
    {
        pData = eventParams.data;
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.FILTER", "<%s> - eventParams.data: 0x%x, pData - 0x%x\n", __FUNCTION__, eventParams.data, pData);
        //rmf_osal_event_delete(eventHandle);
    }
        
    return pData;
}
#endif

/**
 * @brief This function is used to get the specified filter ID's SectionFilterInfo.
 *
 * @param[in] filterID Specifies the filter against which the pSectionFilterInfo is required.
 *
 * @return Returns SectionFilterInfo of the specified filter ID.
 * @retval pSectionFilterInfo Indicates the section filter info.
 * @retval NULL Indicates the filter ID is invalid.
 */
void* rmf_SectionFilter::get_section_filter_info(uint32_t filterID)
{
        rmf_sf_SectionRequest_t*         pFilter_Request         = NULL;

        // Find the section request structure
        //pFilter_Request = (rmf_sf_SectionRequest_t*)vl_symbolMapTable_lookup(m_sectionRequests, (VLSymbolMapKey)filterID);
        rmf_osal_mutexAcquire(m_sectionRequestMutex);
        pFilter_Request = (rmf_sf_SectionRequest_t*)m_sectionRequests->Lookup((rmf_SymbolMapKey)filterID);
        rmf_osal_mutexRelease(m_sectionRequestMutex);

    if(pFilter_Request != NULL)
        return pFilter_Request->pSectionFilterInfo;

    return NULL;
}

/**
 * @brief This function is used to resume the sectionFilter with the specified filter ID.
 *
 * @param[in] filterID Indicates the SectionFilter with filter ID to be resumed back.
 * @return Returns RMF_SECTFLT_Success on success else returns appropriate error codes of type RMF_SECTFLT_RESULT.
 */
RMF_SECTFLT_RESULT rmf_SectionFilter::resume(uint32_t filterID)
{
	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.FILTER","%s: Resuming section filter. ID: %d\n", __FUNCTION__, filterID);
	return Start(filterID);
}

/**
 * @brief This function is used to pause the section filter with the specified filter ID.
 *
 * @param[in] filterID Indicates the SectionFilter with filter ID to be paused.
 * @return Returns RMF_SECTFLT_Success on success else returns appropriate error codes of type RMF_SECTFLT_RESULT.
 */
RMF_SECTFLT_RESULT rmf_SectionFilter::pause(uint32_t filterID)
{
	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.FILTER","%s: Pausing section filter. ID: %d\n", __FUNCTION__, filterID);	
	return Stop(filterID);
}
