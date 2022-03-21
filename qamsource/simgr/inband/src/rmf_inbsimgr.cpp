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
 * @file rmf_inbsimgr.cpp
 * @brief This file contains function definations of the class rmf_InbSiMgr which parses
 * the live TS and when recordings are made to extract PSI information.
 */
#include <stdlib.h>

#include "rmf_inbsimgr.h"
#include "rmf_sectionfilter.h"

#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif

#define MAX_EVTQ_NAME_LEN 100
#define FILTER_MAX_SECTION_SIZE (4096)

#define      NO_PARSING_ERROR                   RMF_SUCCESS

#define      LENGTH_CRC_32                      4           //length in bytes of a CRC_32
#define      LEN_OCTET1_THRU_OCTET3             3             //byte count for the 1st 3 byte in a structure,
#define      LEN_SECTION_HEADER                 3             //byte count for the 1st 3 byte in PAT/PMT

#define      PROGRAM_ASSOCIATION_TABLE_ID       0x00    // PAT_ID
#ifdef CAT_FUNCTIONALITY
#define      CONDITIONAL_ACCESS_TABLE_ID        0x01    // CAT_ID
#endif
#define      PROGRAM_MAP_TABLE_ID               0x02    // PMT_ID

#define      TDT_TABLE_ID                       0x70
#define      TOT_TABLE_ID                       0x73
#define      TDT_TOT_PID                        0x0014   // TOD/TOT PID

#define      CAT_PID                            0x0001    // conditional acess table packet identifier
#define      MIN_VALID_PID                      0x0010   // minimum valid value for packet identifier
#define      MAX_VALID_PID                      0x1FFE   // maximum valid value for packet identifier
#define      NO_PCR_PID                         0x1FFF   // Use when no PID carries the PCR

#define      MIN_VALID_NONPRIVATE_STREAM_TYPE   0x01   // minimum valid value for a non-private stream type
#define      MAX_VALID_NONPRIVATE_STREAM_TYPE   0x87   // maximum valid value for a non-private stream type

#define      MIN_VALID_PRIVATE_STREAM_TYPE      0x88   // minimum valid value for a private stream type
#define      MAX_VALID_PRIVATE_STREAM_TYPE      0xFF   // maximum valid value for a private stream type
#define      MAX_IB_SECTION_LENGTH              1021   //the maximum value that can be assigned to a section length field
#define      MAX_PROGRAM_INFO_LENGTH            1023   //the maximum value that can be assigned to a
//PMT program_info_length field
#define      MAX_ES_INFO_LENGTH                 1023   //the maximum value that can be assigned to a
//PMT es_info_length field
#define RMF_INBSI_DEFAULT_TIMEOUT       2000000 //Default timeout for PSI sections (in microseconds)

#define RMF_INBSI_PAT_TIMEOUT_MS 1000 //Default timeout for PAT for blocking in getting pgm info (in milli seconds)

/**
 *                   GetBits (in: Source, LSBit, MSBit)
 * <ul>
 * <li> Note:
 * <li> 1) Bits within an octet or any other unit of storage (word,
 * longword, doubleword, quadword, etc.) range from 1 through N
 * (the maximum number of bits in the unit of storage).
 * <li>
 * <li> 2) Bit #1 in any storage unit is the "absolute" least significant
 * bit(LSBit). However, depending upon where a field (a series of
 * bits) starts within a storage unit, the "virtual" LSBit associated
 * with the field may take on a value from 1 thru N.
 * <li>
 * <li> 3) Bit #N in any storage unit is the "absolute" most significant
 * bit (MSBit). However, depending upon where a field (a series of
 * bits) ends within a storage unit, the "virtual" MSBit associated
 * with the field may take on a value from 1 thru N.
 * <li>
 * <li> 4) For any field within any storage unit, the following inequality
 * must always be observed:
 * <li> a.)  LSBit  <= (less than or equal to) MSBit.
 * <li>
 * <li> 5) This macro extracts a field of  N bits (starting at LSBit and
 * ending at MSBit) from argument Source, moves the extract field
 * into bits 1 thru N (where, N = 1 + (MSBit - LSBit)) of the register
 * before returning the extracted value to the caller.
 * <li>
 * <li> 6) This macro does not alter the contents of Source.
 * <li>
 * <li>
 * <li> return   (((((1 << (MSBit - LSBit + 1))-1) << (LSBit -1)) & Source)>> (LSBit -1))
 * <li>
 * <li>            |----------------------------| |------------| |----------||----------|
 * <li>                     Mask Generator          Mask Shift       Field       Right
 * <li>                                               Count      Extraction   Justifier
 * </ul>
 */
#define      GETBITS(Source, MSBit, LSBit)   ( \
            ((((1 << (MSBit - LSBit + 1))-1) << (LSBit -1)) & Source) >>(LSBit -1))

/**                    TestBits (in: Source, LSBit, MSBit)
 * <ul>
 * <li> Note:
 * <li> 1) Notes 1 thru 4 from GetBits are applicable.
 * <li>
 * <li> 2) Source is the storage unit containing the field (defined by
 * LSBit and MSBit) to be tested. The test is a non destructive
 * test to source.
 * <li> 3) If 1 or more of the tested bits are set to "1", the value
 * returned from TestBits will be greater than or equal to 1.
 * Otherwise, the value returned is equal to zero.
 * <li>
 * <li> return    ((((1 << (MSBit - LSBit + 1))-1) << (LSBit -1)) & Source)
 *
 * <li>            |----------------------------| |------------| |-------|
 * <li>                    Mask Generator           Mask Shift     Field
 * <li>                                               Count        Extraction
 * </ul>
 */
#define      TESTBITS(Source, MSBit, LSBit)   (\
         (((1 << (MSBit - LSBit + 1))-1) << (LSBit -1)) & Source)

#define PMT_LIST_DEFAULT_LEN 16
#define PMT_LIST_MAX_LEN 128

#define RMF_CASE_STREAM_TYPE_STRING(val)    case RMF_SI_ELEM_##val: pszValue = #val; break

rmf_osal_Bool rmf_InbSiMgr::m_NegativeFiltersEnabled = FALSE;
rmf_osal_Bool rmf_InbSiMgr::m_dumpPSITables = FALSE;

/**
 * @brief This function is a parameterised constructor which initialises section filter,
 * SI cache, event queues, SIDB registration lists, condition variable and their mutex's.
 *
 * @param[in] psiSource Contains values for frequency, modulation and also information to create section filter.
 *
 * @return None
 */
rmf_InbSiMgr::rmf_InbSiMgr(rmf_PsiSourceInfo psiSource)
{
    rmf_Error err = RMF_INBSI_SUCCESS;

        /*  Registered SI DB listeners and queues (queues are used at MPE level only) */
        g_inbsi_registration_list = NULL;
        g_inbsi_registration_list_mutex = NULL; /* sync primitive */
	g_sitp_psi_mutex = NULL;
	g_inbsi_pat_cond = NULL;
	isPaused = FALSE;

        /* Create global registration list mutex */
        err = rmf_osal_mutexNew(&g_inbsi_registration_list_mutex);
        if (err != RMF_INBSI_SUCCESS)
        {
                RDK_LOG(RDK_LOG_WARN, "LOG.RDK.INBSI",
                                "<%s: %s>: Mutex creation failed, returning RMF_OSAL_EMUTEX\n", PSIMODULE, __FUNCTION__);
                //return RMF_OSAL_EMUTEX;
        }

    /* Create global registration list mutex */
    err = rmf_osal_mutexNew(&g_sitp_psi_mutex);
    if (err != RMF_INBSI_SUCCESS)
    {
        RDK_LOG(RDK_LOG_WARN, "LOG.RDK.INBSI",
        "<%s: %s> sipt psi mutex creation failed, returning RMF_OSAL_EMUTEX\n", PSIMODULE, __FUNCTION__);
        //return MPE_EMUTEX;
    }

        /* Create PAT condition object */
    if (rmf_osal_condNew(TRUE, FALSE, &g_inbsi_pat_cond) != RMF_INBSI_SUCCESS)
        {
                RDK_LOG(RDK_LOG_WARN, "LOG.RDK.INBSI",
                                "<%s: %s>: Condition creation failed, returning RMF_OSAL_EMUTEX\n", PSIMODULE, __FUNCTION__);
                //return RMF_OSAL_EMUTEX;
        }

    m_bIsPATAvailable = FALSE;
    m_bIsPATFilterSet = FALSE;
    
#ifdef CAT_FUNCTIONALITY
    g_inbsi_cat_cond = NULL;
    if (rmf_osal_condNew(TRUE, FALSE, &g_inbsi_cat_cond) != RMF_INBSI_SUCCESS)
    {
      RDK_LOG(RDK_LOG_WARN, "LOG.RDK.INBSI", "<%s: %s>: Condition creation failed, returning RMF_OSAL_EMUTEX\n", PSIMODULE, __FUNCTION__);
      //return RMF_OSAL_EMUTEX;
    }
    m_catTable = NULL;
    m_catTableSize = 0;
    m_total_descriptor_length = 0;
    m_bIsCATAvailable = FALSE;
    m_bIsCATFilterSet = FALSE;
    catVersionNumber = INIT_TABLE_VERSION;
    catCRC = 0x0;
    num_cat_sections = 0;
#endif

    const char* useNegativeFilters = NULL;
    useNegativeFilters = rmf_osal_envGet("QAMSRC.PSI.NEGATIVE.FILTERS.ENABLED");
    if ( (useNegativeFilters != NULL) && (0 == strcmp(useNegativeFilters, "TRUE")))
    {
        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.INBSI","%s: negative filtering is enabled.\n" ,
            __FUNCTION__);
        m_NegativeFiltersEnabled = TRUE;
    }
   
    m_freq = psiSource.freq;
    m_modulation_mode = psiSource.modulation_mode;
    num_sections = 0;


    m_pInbSectionFilter = rmf_SectionFilter::create(rmf_filters::inband,(void*)(&psiSource));    

    m_bShutDown = FALSE;

#ifdef ENABLE_INB_SI_CACHE
    m_pSiCache = rmf_InbSiCache::getInbSiCache();
#endif

    psiMonitorThreadStop = new sem_t;

    if(RMF_INBSI_SUCCESS != rmf_osal_eventqueue_create ((const uint8_t* )"InbSFQueue",
        &m_FilterQueueId))
    {
        RDK_LOG(RDK_LOG_ERROR,
                "LOG.RDK.INBSI",
                "<%s: %s> - unable to create event queue,... terminating thread.\n",
                PSIMODULE, __FUNCTION__);
    }
    err  = sem_init( psiMonitorThreadStop, 0 , 0);
    if (0 != err )
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.INBSI", 
        "<%s: %s> - sem_init failed.\n", PSIMODULE, __FUNCTION__);
    }

#ifndef ENABLE_INB_SI_CACHE
    err = rmf_osal_memAllocP(RMF_OSAL_MEM_SI_INB, 
                            PMT_LIST_DEFAULT_LEN*sizeof(rmf_pmtInfo_t),
                            (void **) &m_pmtInfoList);
    if ( RMF_SUCCESS != err)
    {
        RDK_LOG(RDK_LOG_ERROR,
                "LOG.RDK.INBSI",
                "<%s: %s> - rmf_osal_memAllocP failed.\n",
                PSIMODULE, __FUNCTION__);
    }
    m_pmtListMaxSize = PMT_LIST_DEFAULT_LEN;
    m_pmtListLen = 0;
    patVersionNumber = INIT_TABLE_VERSION;
    pmtVersionNumber = INIT_TABLE_VERSION;
    outer_descriptors = NULL;
    elementary_streams = NULL;
    number_elem_streams = 0;
    number_outer_desc = 0;
    descriptors_in_use = FALSE;
    patCRC = 0;
#endif
    /* Create global registration list mutex */
    err = rmf_osal_mutexNew(&m_mediainfo_mutex);
    if (err != RMF_INBSI_SUCCESS)
    {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.INBSI",
                            "<%s: %s>: Mutex creation failed\n", PSIMODULE, __FUNCTION__);
    }

    m_tdtTable     = NULL;
    m_tdtTableSize = 0;
    m_totTable     = NULL;
    m_totTableSize = 0;

    const char* dumpPSIData = NULL;
    dumpPSIData = rmf_osal_envGet("QAMSRC.PSI.DUMP.TABLES");
    if ( (dumpPSIData != NULL) && (0 == strcmp(dumpPSIData, "TRUE")))
    {
        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.INBSI","%s: Will dump PSI data every time there is a table change.\n" ,
            __FUNCTION__);
        m_dumpPSITables = TRUE;
    }

    start_sithread();
}

/**
 * @brief This function is a default destructor which releases all the resources being used like section filter,
 * SI cache, elementary streams list, mutex and frees the memory being used.
 *
 * @return None
 */
rmf_InbSiMgr::~rmf_InbSiMgr()
{
    rmf_InbSiRegistrationListEntry *walker = NULL;
    rmf_InbSiRegistrationListEntry *toDelete = NULL;
        //sent Shutdown Event to worker thread.
    rmf_osal_event_params_t event_params = {0};
    rmf_osal_event_handle_t event_handle;
    rmf_Error ret = RMF_INBSI_SUCCESS;

    rmf_osal_mutexAcquire(g_sitp_psi_mutex);
    RDK_LOG( RDK_LOG_INFO, "LOG.RDK.INBSI","%s - calling cancel_all_filters().\n" ,
        __FUNCTION__); // Temp debug
    cancel_all_filters();
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.INBSI","%s - after cancel_all_filters().\n" ,
        __FUNCTION__); //Temp debug
    rmf_osal_mutexRelease(g_sitp_psi_mutex);

#ifdef ENABLE_INB_SI_CACHE
    rmf_SiTransportStreamHandle ts_handle = RMF_SI_INVALID_HANDLE;
    ret= m_pSiCache->GetTransportStreamEntryFromFrequencyModulation(m_freq, m_modulation_mode, &ts_handle);
    if((RMF_INBSI_SUCCESS == ret) && (RMF_INBSI_INVALID_HANDLE != ts_handle))
    {
        m_pSiCache->ResetPATProgramStatus(ts_handle);
        //m_pSiCache->SetPATStatusNotAvailable(ts_handle);
        m_pSiCache->SetTSIdStatusNotAvailable(ts_handle);
        m_pSiCache->SetPATVersion(ts_handle, INIT_TABLE_VERSION);
    }
#endif

    m_bIsPATAvailable = FALSE;
    m_bIsPATFilterSet = FALSE;

#ifdef CAT_FUNCTIONALITY
    rmf_osal_condDelete(g_inbsi_cat_cond);
    if (NULL != m_catTable)
    {
      rmf_osal_memFreeP( RMF_OSAL_MEM_SI_INB, m_catTable );
      m_catTable = NULL;
    }
    m_catTableSize = 0;
    m_total_descriptor_length = 0;
    m_bIsCATAvailable = FALSE;
    m_bIsCATFilterSet = FALSE;
#endif
    m_freq = 0;
    m_modulation_mode = 0;

#ifdef ENABLE_INB_SI_CACHE
    rmf_InbSiCache::clearCache();
#endif

    event_params.priority = 0xFF;

    ret = rmf_osal_event_create(RMF_OSAL_EVENT_CATEGORY_MPEG, RMF_PSI_EVENT_SHUTDOWN, &event_params, &event_handle);
    if(ret != RMF_INBSI_SUCCESS)
    {
        RDK_LOG(
                RDK_LOG_ERROR,
                "LOG.RDK.INBSI",
                "<%s: %s> Could not create event handle: %x\n",
                PSIMODULE, __FUNCTION__, ret);
        //return ret;
    }
    else
    {
            rmf_osal_eventqueue_add(m_FilterQueueId, event_handle);
    }
    

    RDK_LOG( RDK_LOG_INFO, "LOG.RDK.INBSI"," rmf_InbSiMgr::<%s: %s>:%d  waiting for PsiMonitor thread to exit...\n" ,
        PSIMODULE, __FUNCTION__, __LINE__);
    ret = sem_wait( psiMonitorThreadStop);
    if (0 != ret )
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.INBSI", 
        "<%s: %s> - sem_wait failed.\n", PSIMODULE, __FUNCTION__);
    }
    
    RDK_LOG( RDK_LOG_INFO, "LOG.RDK.INBSI"," rmf_InbSiMgr::<%s: %s>:%d  got psiMonitorThreadStop\n" ,
        PSIMODULE, __FUNCTION__, __LINE__);
    
    ret = sem_destroy( psiMonitorThreadStop);
    if (0 != ret )
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.INBSI", 
        "<%s:%s> - sem_destroy failed.\n", PSIMODULE, __FUNCTION__);
    }

    delete psiMonitorThreadStop;

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.INBSI", "%s: deleting INB section filter object.\n",
        __FUNCTION__);
    delete m_pInbSectionFilter;
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.INBSI", "%s: INB section filter deleted.\n",
        __FUNCTION__);

    if(RMF_INBSI_SUCCESS != rmf_osal_eventqueue_delete (m_FilterQueueId))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.INBSI", "<%s: %s> - unable to delete event queue.\n",
                    PSIMODULE, __FUNCTION__);
    }

    rmf_osal_mutexDelete(g_sitp_psi_mutex);

    rmf_osal_mutexAcquire(g_inbsi_registration_list_mutex);

    walker = g_inbsi_registration_list;
        while (walker)
        {
        toDelete = walker;
        toDelete->next = NULL;
                walker = walker->next;
        rmf_osal_memFreeP(RMF_OSAL_MEM_SI_INB, toDelete);
        }
    g_inbsi_registration_list = NULL;

    rmf_osal_mutexRelease(g_inbsi_registration_list_mutex);
    
    rmf_osal_mutexDelete(g_inbsi_registration_list_mutex);

    /* delete global PAT condition object */
    rmf_osal_condDelete(g_inbsi_pat_cond);

#ifndef ENABLE_INB_SI_CACHE
    if (NULL != m_pmtInfoList)
    {
        rmf_osal_memFreeP( RMF_OSAL_MEM_SI_INB, m_pmtInfoList );
    }
    release_descriptors( outer_descriptors);
    release_es_list( elementary_streams );
#endif

    rmf_osal_mutexDelete( m_mediainfo_mutex);

    if (m_tdtTable) {
      free(m_tdtTable);
    }

    if (m_totTable) {
      free(m_totTable);
    }
}

rmf_Error rmf_InbSiMgr::start_sithread(void)
{
    rmf_Error ret = RMF_INBSI_SUCCESS;
    rmf_osal_ThreadId stThreadId;

    // Need to include the code for populating environment variables
    ret = rmf_osal_threadCreate( PsiThreadFn, (void*)this, RMF_OSAL_THREAD_PRIOR_DFLT, RMF_OSAL_THREAD_STACK_SIZE, &stThreadId, "Psi_Thread");

    return ret;
}

void rmf_InbSiMgr::PsiThreadFn(void *arg)
{
    rmf_InbSiMgr *pInbSiMgr = (rmf_InbSiMgr*)arg;
    pInbSiMgr->PsiMonitor();
}

//void rmf_InbSiMgr::PsiMonitor(void)
void rmf_InbSiMgr::PsiMonitor()
{
    rmf_osal_event_handle_t eventHandle;
    rmf_osal_event_params_t eventParams;
    uint32_t eventType;
    int32_t timeout = RMF_INBSI_DEFAULT_TIMEOUT;
    uint32_t filterId;
    int ret = 0;
    //uint32_t *pFilterId;
    rmf_Error err = RMF_INBSI_SUCCESS;

        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.INBSI", "<%s: %s> - Enter\n",
            PSIMODULE, __FUNCTION__);

    while(!m_bShutDown)
    {

        if(RMF_INBSI_SUCCESS != (err = rmf_osal_eventqueue_get_next_event_timed (m_FilterQueueId, &eventHandle, 
                                     NULL, &eventType, &eventParams, timeout)))
            {
            if(err == RMF_OSAL_ENODATA)
            {
                if(!m_filter_list.empty())
                {
#ifdef ENABLE_INB_SI_CACHE
                    dispatch_event(RMF_SI_EVENT_SI_NOT_AVAILABLE_YET, 0, 0, 0, 0);
#else
                    dispatch_event(RMF_SI_EVENT_SI_NOT_AVAILABLE_YET , 0);
#endif
                }
                continue;
            }
            else
            {
                    RDK_LOG(RDK_LOG_ERROR,
                    "LOG.RDK.INBSI",
                    "<%s: %s> - unable to get event,... terminating thread. err = 0x%x\n",
                    PSIMODULE, __FUNCTION__, err);
                /*
                 ** Release global mutex
                 */
                        break;
            }
            }

        /*
         ** Acquire the global mutex.
         */
        rmf_osal_mutexAcquire(g_sitp_psi_mutex);

        if((eventType == RMF_SF_EVENT_SECTION_FOUND) || (eventType == RMF_SF_EVENT_LAST_SECTION_FOUND))
        {
                RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.INBSI", "<%s: %s> - event Rxd\n",
            PSIMODULE, __FUNCTION__);
            //pFilterId = (uint32_t*)eventParams.data;
            filterId = eventParams.data_extension;
            //process_section_from_filter(*pFilterId);
            process_section_from_filter(filterId);

            if(eventType == RMF_SF_EVENT_LAST_SECTION_FOUND)
            {
                        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.INBSI", "<%s: %s> - event RMF_SF_EVENT_LAST_SECTION_FOUND rxd\n",
            PSIMODULE, __FUNCTION__);
                
            //  m_pInbSectionFilter->ReleaseFilter(filterId);
                //m_filter_list.remove(*pFilterId);
                rmf_osal_condSet(g_inbsi_pat_cond);
            }
        }

        rmf_osal_event_delete(eventHandle);
        /*
         ** Release global mutex
         */
        rmf_osal_mutexRelease(g_sitp_psi_mutex);
        if ( RMF_PSI_EVENT_SHUTDOWN == eventType )
        {
            RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.INBSI",
                "<%s: %s> - RMF_PSI_EVENT_SHUTDOWN recieved, exiting\n",
            PSIMODULE, __FUNCTION__);
            m_bShutDown = TRUE;
        }
    }
    RDK_LOG( RDK_LOG_INFO, "LOG.RDK.INBSI"," rmf_InbSiMgr::<%s: %s>:%d  posting psiMonitorThreadStop and exiting\n" ,
        PSIMODULE, __FUNCTION__, __LINE__);
    
    
    ret = sem_post( psiMonitorThreadStop);
    if (0 != ret )
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.INBSI", 
        "<%s:%s> - sem_post failed.\n", PSIMODULE, __FUNCTION__);
    }
	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "%s:%d after sem_post()\n", __FUNCTION__, __LINE__); //Temp debug

}

rmf_Error rmf_InbSiMgr::parse_TDT(uint32_t section_size, uint8_t *section_data)
{
  if (section_size == 8) {
    rmf_osal_mutexAcquire(m_mediainfo_mutex);
    if ((m_tdtTableSize > section_size) || m_tdtTable == NULL) {
      if (m_tdtTable) {
        free(m_tdtTable);
      }
      m_tdtTable = (char*) malloc(section_size);
    }
    if (m_tdtTable) {
      memcpy(m_tdtTable, section_data, section_size);
      m_tdtTableSize = section_size;
    }
    rmf_osal_mutexRelease(m_mediainfo_mutex);

    NotifyTableChanged(TABLE_TYPE_TDT, RMF_SI_CHANGE_TYPE_MODIFY, 0);
  }
}

rmf_Error rmf_InbSiMgr::parse_TOT(uint32_t section_size, uint8_t *section_data)
{
  if (section_size >= 10) {
    rmf_osal_mutexAcquire(m_mediainfo_mutex);
    if ((m_totTableSize > section_size) || m_totTable == NULL) {
      if (m_totTable) {
        free(m_totTable);
      }
      m_totTable = (char*) malloc(section_size);
    }
    if (m_totTable) {
      memcpy(m_totTable, section_data, section_size);
      m_totTableSize = section_size;
    }
    rmf_osal_mutexRelease(m_mediainfo_mutex);
    NotifyTableChanged(TABLE_TYPE_TOT, RMF_SI_CHANGE_TYPE_MODIFY, 0);
  }
}

char* rmf_InbSiMgr::get_TDT(int *size) 
{
  char* res = NULL;
  rmf_osal_mutexAcquire(m_mediainfo_mutex);
  if (m_tdtTableSize != 0) {
    res = (char*) malloc(m_tdtTableSize);
    if (res) {
      memcpy(res, m_tdtTable, m_tdtTableSize);
      *size = m_tdtTableSize;
    }
  }
  rmf_osal_mutexRelease(m_mediainfo_mutex);
  return res;
}
char* rmf_InbSiMgr::get_TOT(int *size)
{
  char* res = NULL;
  rmf_osal_mutexAcquire(m_mediainfo_mutex);
  if (m_totTableSize != 0) {
    res = (char*) malloc(m_totTableSize);
    if (res) {
      memcpy(res, m_totTable, m_totTableSize);
      *size = m_totTableSize;
    }
  }
  rmf_osal_mutexRelease(m_mediainfo_mutex);
  return res;
}


void rmf_InbSiMgr::process_section_from_filter(uint32_t filterId)
{
    uint8_t sectionBuffer[FILTER_MAX_SECTION_SIZE];
    uint32_t sectionBufferSize = 0;
    rmf_Error retCode = RMF_INBSI_SUCCESS;

    rmf_FilterSectionHandle sectionHandle = RMF_SF_INVALID_SECTION_HANDLE;

    if(RMF_INBSI_SUCCESS != (retCode = m_pInbSectionFilter->GetSectionHandle(filterId,
                                                            0,
                                                            &sectionHandle)))
    {
        RDK_LOG(RDK_LOG_WARN, "LOG.RDK.FILTER",
                         "<%s: %s>: MPE error %d (0x%08x) retrieving section handle for filter %d\n", 
                         PSIMODULE, __func__, retCode, retCode, filterId);
        return;
    }

    if(RMF_INBSI_SUCCESS != (retCode = m_pInbSectionFilter->ReadSection(sectionHandle,
            0, FILTER_MAX_SECTION_SIZE,
            0,
            sectionBuffer,
            &sectionBufferSize)))
    {
        RDK_LOG( RDK_LOG_WARN, "LOG.RDK.FILTER",
                "<%s: %s>: error %d (0x%08x) reading from section handle 0x%08x for filter %d\n", 
                PSIMODULE, __func__, retCode, retCode, sectionHandle, filterId);
        m_pInbSectionFilter->ReleaseSection(sectionHandle);     
        return;
    }

    if(FALSE == handle_table(sectionBuffer, sectionBufferSize))
    {
        RDK_LOG(RDK_LOG_TRACE3, "LOG.RDK.INBSI", "<%s: %s> - processing table with id: %d\n", 
                PSIMODULE, __FUNCTION__, sectionBuffer[0]);
        switch (sectionBuffer[0])
        {
            case PROGRAM_ASSOCIATION_TABLE_ID: /* PAT Section */
                parse_and_update_PAT(sectionBufferSize, sectionBuffer);
                break;

            case PROGRAM_MAP_TABLE_ID: /* PMT Section */
                parse_and_update_PMT(sectionBufferSize, sectionBuffer);
                break;

#ifdef CAT_FUNCTIONALITY
            case CONDITIONAL_ACCESS_TABLE_ID: /* CAT Section */
                parse_and_update_CAT(sectionBufferSize, sectionBuffer);
                break;
#endif
            case TDT_TABLE_ID: 
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.INBSI", "Dumping PSI section - TDT %d\n", sectionBufferSize);
                parse_TDT(sectionBufferSize, sectionBuffer);
                break;
            
            case TOT_TABLE_ID: 
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.INBSI", "Dumping PSI section - TOT %d\n", sectionBufferSize);
                parse_TOT(sectionBufferSize, sectionBuffer);
                break;
            default:
                break;
        }
    }

    m_pInbSectionFilter->ReleaseSection(sectionHandle);
}
void rmf_InbSiMgr::dumpPSIData(uint8_t* section_data, uint32_t section_size)
{
    int counter  = 0;
    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.INBSI", "Dumping PSI section. Table ID: %d. Section size is %d. \n", section_data[0], section_size);
    for ( counter = 0; counter <= (section_size-8); counter+=8 )
    {            
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.INBSI", "0x%x 0x%x 0x%x 0x%x      0x%x 0x%x 0x%x 0x%x \n", 
            section_data[counter], section_data[counter+1], section_data[counter+2], section_data[counter+3], 
            section_data[counter+4], section_data[counter+5], section_data[counter+6], section_data[counter+7]);
    }
    switch( section_size % 8 )
    {
        case 7:
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.INBSI", "0x%x 0x%x 0x%x 0x%x      0x%x 0x%x 0x%x \n", 
                section_data[section_size -7], section_data[section_size - 6], section_data[section_size -5], 
                section_data[section_size - 4], section_data[section_size - 3], section_data[section_size -2], section_data[section_size - 1]);
            break;
        case 6:
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.INBSI", "0x%x 0x%x 0x%x 0x%x      0x%x 0x%x \n", 
                section_data[section_size - 6], section_data[section_size -5], 
                section_data[section_size - 4], section_data[section_size - 3], section_data[section_size -2], section_data[section_size - 1]);
            break;
        case 5:
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.INBSI", "0x%x 0x%x 0x%x 0x%x      0x%x\n", 
                section_data[section_size -5], 
                section_data[section_size - 4], section_data[section_size - 3], section_data[section_size -2], section_data[section_size - 1]);
            break;
        case 4:
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.INBSI", "0x%x 0x%x 0x%x 0x%x \n", 
                section_data[section_size - 4], section_data[section_size - 3], section_data[section_size -2], section_data[section_size - 1]);
            break;
        case 3:
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.INBSI", "0x%x 0x%x 0x%x \n", 
                section_data[section_size - 3], section_data[section_size -2], section_data[section_size - 1]);
            break;
        case 2:
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.INBSI", "0x%x 0x%x \n", 
                 section_data[section_size -2], section_data[section_size - 1]);
            break;
        case 1:
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.INBSI", "0x%x    \n", 
                section_data[section_size -1]);
            break;
    }
}

#ifdef CAT_FUNCTIONALITY
rmf_Error rmf_InbSiMgr::parse_and_update_CAT(uint32_t section_size, uint8_t *section_data)
{
    cat_table_struc1 *ptr_table_struc1;
    generic_descriptor_struc *ptr_descriptor_struc;

  #ifdef ENABLE_INB_SI_CACHE
    rmf_SiTransportStreamHandle ts_handle = RMF_SI_INVALID_HANDLE;
    uint32_t frequency = 0;
    uint32_t sidbVersionNumber = INIT_TABLE_VERSION;
    uint32_t sidbCrc32 = 0;
  #else
    rmf_Error ret;
  #endif

    uint8_t *ptr_input_buffer = NULL;
    uint8_t *ptr_cat_section_start = NULL;

    uint16_t version_number = 0;
    uint16_t current_next_indicator = 0;
    uint16_t transport_stream_id = 0;

    uint32_t section_length = 0;
    uint32_t real_section_length = 0;
    uint32_t crc = 0;
    uint16_t descriptor_tag = 0, descriptor_length = 0, total_descriptor_length = 0;
    rmf_Error cat_error = NO_PARSING_ERROR;

    //**********************************************************************************************
    // Get, parse and validate the first 64 bytes (Table_ID thru Last_Section_Number)of the CAT
    // table (see document ISO/IEC 13818-1:2000(E),pg.45) .
    //**********************************************************************************************

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.INBSI", "<%s: %s> - Entered method.\n", PSIMODULE, __FUNCTION__);
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.INBSI", "<%s: %s> - section_size = %d \n", PSIMODULE, __FUNCTION__, section_size);

    if(TRUE == m_dumpPSITables)
    {
      dumpPSIData(section_data, section_size);
    }

    RDK_LOG(RDK_LOG_TRACE3, "LOG.RDK.INBSI",
            "<%s: %s> - section_size = %d \n", PSIMODULE, __FUNCTION__,
            section_size);

    ptr_input_buffer = (uint8_t *)section_data;

    // QUESTION: Does the section_data include header??
    // If it does not we don't need to parse the section_length
    // field to validate

    ptr_table_struc1 = (cat_table_struc1 *) ptr_input_buffer;

    //**** validate table Id field ****
    if (ptr_table_struc1->table_id_octet1 != CONDITIONAL_ACCESS_TABLE_ID)
    {
        cat_error = 1;
        goto PARSING_DONE;
    }

    //**** validate section_syntax_indicator field ****
    if (!TESTBITS(ptr_table_struc1->section_length_octet2, 8, 8))
    {
        // bit not set to 1
        cat_error = 3;
        RDK_LOG(RDK_LOG_ERROR,
            "LOG.RDK.INBSI",
            "<%s: %s> - *** ERROR *** section_syntax_indicator is clear.\n",
            PSIMODULE, __FUNCTION__);
        goto PARSING_DONE;
    }

    //**** extract and validate section_length field (octets 2 & 3)       ****
    //**** note, bits 12 and 11 must be set to zero and max value is 1021.****

    real_section_length = section_length = ((GETBITS(ptr_table_struc1->section_length_octet2, 4, 1)) << 8) | ptr_table_struc1->section_length_octet3;

    if (section_length > MAX_IB_SECTION_LENGTH)
    {
        // error, section_length too.
        cat_error = 2;
        goto PARSING_DONE;
    }

    real_section_length -= (5 + 4); /* size of list of CA Descriptors */

    RDK_LOG(RDK_LOG_TRACE3, "LOG.RDK.INBSI",
            "<%s: %s> - section_length = %d, real_section_length: %d \n", PSIMODULE, __FUNCTION__,
            section_length, real_section_length);

    RDK_LOG(RDK_LOG_TRACE1,
            "LOG.RDK.INBSI",
            "<%s: %s> - value for num_cat_sections: %d\n",
                PSIMODULE, __FUNCTION__, num_cat_sections);

    if (num_cat_sections == 0)
    {
        if (NULL != m_catTable)
        {
          rmf_osal_memFreeP(RMF_OSAL_MEM_SI_INB, m_catTable);
          m_catTable = NULL;
          m_catTableSize = 0;
          m_total_descriptor_length = 0;
        }

        RDK_LOG(RDK_LOG_TRACE1,
                "LOG.RDK.INBSI",
                "<%s: %s> - ptr_table_struc1->last_section_number_octet8: %d\n",
                PSIMODULE, __FUNCTION__, ptr_table_struc1->last_section_number_octet8);
        num_cat_sections = ptr_table_struc1->last_section_number_octet8 + 1;
        RDK_LOG(RDK_LOG_TRACE1,
                "LOG.RDK.INBSI",
                "<%s: %s> - new value for num_cat_sections: %d\n",
                PSIMODULE, __FUNCTION__, num_cat_sections);
    }

    //**** extract and validate transport_stream_id field (octets 4 & 5) *****/
    transport_stream_id = (uint16_t)(((ptr_table_struc1->reserved_octet4) << 8) | ptr_table_struc1->reserved_octet5);

    RDK_LOG(RDK_LOG_TRACE3, "LOG.RDK.INBSI",
            "<%s: %s> - transport_stream_id = %d \n",
            PSIMODULE, __FUNCTION__,
            transport_stream_id);

    //**** validate 2 reserved bits ****
    if (GETBITS(ptr_table_struc1->current_next_indic_octet6, 8, 7) != 3)
    {
        // bits 8 and 7 are not set to 1 per ISO/IEC 13818-1:2000(E),pg.4, para 2.1.46)
        //cat_error = 4 ;
        RDK_LOG(RDK_LOG_TRACE3,
                "LOG.RDK.INBSI",
                "<%s: %s> - *** WARNING *** 2-Bit-Reserved Field #3 not set to all ones(1)\n.",
                PSIMODULE, __FUNCTION__);
    }

    //**** extract version_number (0-31) field ****

    version_number = (uint16_t)(GETBITS(ptr_table_struc1->current_next_indic_octet6, 6, 2));

    RDK_LOG(RDK_LOG_TRACE3, "LOG.RDK.INBSI",
            "<%s: %s> - version_number = %d \n", PSIMODULE, __FUNCTION__,
            version_number);
    //**** populate the table info array with the new CAT data ****

    //add_cat_entry(fsm_data, version_number);

    //**** extract current_next_indicator field ****
    current_next_indicator = (uint16_t)(GETBITS(ptr_table_struc1->current_next_indic_octet6, 1, 1));
    RDK_LOG(RDK_LOG_TRACE3, "LOG.RDK.INBSI",
            "<%s: %s> - current_next_indicator = %d\n.",
            PSIMODULE, __FUNCTION__, current_next_indicator);

    //**** section_length identifies the total number of bytes/octets remaining   ****
    //**** in the CAT after octet3. Before performing program number, network pid ****
    //**** and program map pid processing ,we must adjust section_length to       ****
    //**** account for having processed  octets4 thru octet8                      ****.
    section_length -= (sizeof(cat_table_struc1) - LEN_OCTET1_THRU_OCTET3);

    // It is possible that an empty CAT (no programs) is signaled sometimes. In
    // that case all we see is a header and a CRC.
    // Check if the length is less than LENGTH_CRC_32, then its an error.
    if (section_length < LENGTH_CRC_32)
    {
        // error, either a portion of header is missing (section_length <0) or
        // no data after header (section_length = 0) or  section_length is not
        // large enough to hold a CRC_2 and a program_number and pid pair.
        cat_error = 5;
        goto PARSING_DONE;
    }


#ifdef ENABLE_INB_SI_CACHE
    m_pSiCache->GetTransportStreamEntryFromFrequencyModulation(m_freq, m_modulation_mode, &ts_handle);
    RDK_LOG(RDK_LOG_TRACE3, "LOG.RDK.INBSI",
            "<%s: %s> - ts_handle = 0x%x \n", PSIMODULE, __FUNCTION__, ts_handle);

    m_pSiCache->LockForWrite();
    m_pSiCache->SetTSIdForTransportStream(ts_handle, transport_stream_id);
    m_pSiCache->ReleaseWriteLock();

    //sidbVersionNumber = getSidbVersionNumber(table_type, ts_handle);
    //sidbCrc32 = getSidbCRC(table_type, ts_handle);
    m_pSiCache->GetCATVersionForTransportStreamHandle(ts_handle, &sidbVersionNumber);
    m_pSiCache->GetCATCRCForTransportStreamHandle(ts_handle, &sidbCrc32);

    m_pSiCache->LockForWrite();
    m_pSiCache->SetCATVersionForTransportStream(ts_handle, version_number);
    m_pSiCache->ReleaseWriteLock();

    RDK_LOG(RDK_LOG_TRACE3,
            "LOG.RDK.INBSI",
            "<%s: %s> - sidb version = %d, sidb crc = 0x%x.\n",
            PSIMODULE, __FUNCTION__, sidbVersionNumber, sidbCrc32);
#endif

    RDK_LOG(RDK_LOG_TRACE3, "LOG.RDK.INBSI",
            "<%s: %s> - new version = %d.\n", PSIMODULE, __FUNCTION__,
            version_number);

    //**********************************************************************************************
    // Parse the remaining bytes/octets in the CAT.
    //**********************************************************************************************
    ptr_cat_section_start = ptr_input_buffer = (uint8_t *) (&section_data[sizeof(cat_table_struc1)]);

    //*****************************************************************************************
    //**** This is the "FOR N Loop" in spec (ISO/IEC 13818-1:2000(E),pg.43, table 2-25)
    //**** Within this loop we will extract and validate program_number and the
    //**** network_pid/program_map_pid.
    //*****************************************************************************************
    while (section_length > LENGTH_CRC_32)
    {
        ptr_descriptor_struc = (generic_descriptor_struc *) ptr_input_buffer;
        descriptor_tag = ptr_descriptor_struc->descriptor_tag_octet1;
        descriptor_length = ptr_descriptor_struc->descriptor_length_octet2;
        total_descriptor_length += (descriptor_length + 2); //total length = length + 2 bytes header (type + len)

        RDK_LOG(RDK_LOG_ERROR,
                "LOG.RDK.INBSI",
                "<%s: %s> - CAT descriptor - len %d\n",
                PSIMODULE, __FUNCTION__, descriptor_length);

        if (descriptor_tag != RMF_SI_DESC_CA_DESCRIPTOR)
        { // error, descriptor is not correct
            cat_error = 27;
            goto PARSING_DONE;
        }

        //**** account for the "N" (sizeof(generic_descriptor_struc) bytes/octet ****
        //**** that we just acquired/processed.                                  ****
        section_length -= sizeof(generic_descriptor_struc);
        ptr_input_buffer += sizeof(generic_descriptor_struc);

        if (descriptor_length > section_length)
        { // error, descriptor is outside of the elementary stream block .
            cat_error = 25;
            goto PARSING_DONE;
        }

        //**** Move the descriptor data into the SI database.             ****
        //**** Note: if descriptor_length is zero (0), the descriptor tag ****
        //**** and length will still be passed to the SIDB and zero bytes ****
        //**** will be written to the database.
#ifdef ENABLE_INB_SI_CACHE
        if (si_program_handle != RMF_SI_INVALID_HANDLE)
        {
            m_pSiCache->LockForWrite();
            (void) m_pSiCache->SetDescriptor(si_program_handle, elementary_pid,
                    (rmf_SiElemStreamType)stream_type, (rmf_SiDescriptorTag)descriptor_tag, descriptor_length,
                    ptr_input_buffer);
            m_pSiCache->ReleaseWriteLock();
        }
#endif
        //**** prepare to process the next descriptor ****
        section_length -= descriptor_length;
        ptr_input_buffer += descriptor_length;

        //**** prepare to process the next program_number and
        //**** unknown_pid (program_map_pid/network_pid) pair.
        if (section_length < LENGTH_CRC_32)
        {
          cat_error = 26;
          goto PARSING_DONE;
        }

    } // End while(section_length > LENGTH_CRC_32)

    RDK_LOG(RDK_LOG_TRACE3, "LOG.RDK.INBSI", "<%s: %s> - total_descriptor_length = %d\n", PSIMODULE, __FUNCTION__, total_descriptor_length);

    //*************************************************************************************************
    //* At this point, the code either terminated prematurely or successfully processed the PMT.
    //* We need to validate the present of the CRC_32 field and cleanup before returning to the caller.
    //*************************************************************************************************
    if ((section_length != LENGTH_CRC_32))
    {
        // error, either the last program_number and program_map_pid/network_pid) pair
        // had an incorrect number of bytes or the CRC_32 had an incorrect number of bytes.
        RDK_LOG(RDK_LOG_ERROR,
                "LOG.RDK.INBSI",
                "<%s: %s> - section length(%d) != LENGHT_CRC_32(%d) - error  - %d\n",
                PSIMODULE, __FUNCTION__, section_length, LENGTH_CRC_32, cat_error);
        cat_error = 13;
        goto PARSING_DONE;
    }

    RDK_LOG(RDK_LOG_TRACE3, "LOG.RDK.INBSI",
            "<%s: %s> - section_length = %u\n", PSIMODULE, __FUNCTION__,
            section_length);
    if (section_length == LENGTH_CRC_32)
    {
        crc = ((*ptr_input_buffer) << 24 | (*(ptr_input_buffer + 1)) << 16
                | (*(ptr_input_buffer + 2)) << 8 | (*(ptr_input_buffer + 3)));
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.INBSI",
                "<%s: %s> - CRC32 = 0x%08x\n", PSIMODULE, __FUNCTION__, crc);
    }

#ifdef ENABLE_INB_SI_CACHE
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.INBSI",
            "<%s: %s> - ts_handle = 0x%x\n", PSIMODULE, __FUNCTION__,
            ts_handle);

    m_pSiCache->LockForWrite();
    m_pSiCache->SetCATCRCForTransportStream(ts_handle, crc);
    m_pSiCache->ReleaseWriteLock();
#endif

PARSING_DONE:

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.INBSI",
            "<%s: %s> - Exiting with ErrorCode = %d\n",
            PSIMODULE, __FUNCTION__, cat_error);

#ifdef ENABLE_INB_SI_CACHE
    m_pSiCache->LockForWrite();
#endif

    if (cat_error != 0)
    {
#ifdef ENABLE_INB_SI_CACHE
        m_pSiCache->SetCATStatusNotAvailable(ts_handle);
#endif
    }
    else
    {
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.INBSI",
                "<%s: %s> - Calling SetCATStatus\n",
                PSIMODULE, __FUNCTION__);
#ifdef ENABLE_INB_SI_CACHE
        m_pSiCache->SetCATStatusAvailable(ts_handle);
        notifySIDB(TABLE_TYPE_CAT, sidbVersionNumber == INIT_TABLE_VERSION, sidbVersionNumber != version_number,sidbCrc32 != crc, ts_handle);
#else
        num_cat_sections--;
        RDK_LOG(RDK_LOG_TRACE1,
                "LOG.RDK.INBSI",
                "<%s: %s> - num_cat_sections: %d, m_bIsCATAvailable: %d\n",
                PSIMODULE, __FUNCTION__, num_cat_sections, m_bIsCATAvailable);

        if ((catVersionNumber == INIT_TABLE_VERSION) || (catVersionNumber != version_number) || (catCRC != crc ))
        {
            RDK_LOG(RDK_LOG_TRACE1,
                    "LOG.RDK.INBSI",
                    "<%s: %s> - Notifying CAT catVersionNumber=%d, version_number=%d catCRC=0x%x, crc=0x%x, m_catTableSize=%d\n",
                    PSIMODULE, __FUNCTION__, catVersionNumber, version_number, catCRC, crc, m_catTableSize);

            rmf_osal_mutexAcquire( m_mediainfo_mutex);

            if (total_descriptor_length > 0)
            {

              RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.INBSI", "<%s: %s> -  m_catTable=%p\n", PSIMODULE, __FUNCTION__, m_catTable);

              if (NULL == m_catTable)
              {
                  ret = rmf_osal_memAllocP(RMF_OSAL_MEM_SI_INB, total_descriptor_length, (void **) &m_catTable);
                  m_catTableSize = 0;
                  m_total_descriptor_length = 0;
              }
              else
              {
                  ret = rmf_osal_memReallocP(RMF_OSAL_MEM_SI_INB, m_catTableSize + total_descriptor_length, (void **) &m_catTable);
              }
              if (RMF_SUCCESS != ret)
              {
                  RDK_LOG(RDK_LOG_ERROR,
                  "LOG.RDK.INBSI",
                  "<%s: %s> - rmf_osal_memAllocP failed.\n",
                  PSIMODULE, __FUNCTION__);
                  rmf_osal_mutexRelease( m_mediainfo_mutex);
                  return 25;
              }

              RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.INBSI",
                      "<%s: %s> - m_catTableSize: %d, real_section_length: %d, total_descriptor_length=%d\n",
                      PSIMODULE, __FUNCTION__, m_catTableSize, real_section_length, total_descriptor_length);

              memcpy(&m_catTable[m_catTableSize], ptr_cat_section_start, total_descriptor_length);
              m_catTableSize += total_descriptor_length;
              m_total_descriptor_length += total_descriptor_length;
            }
            else
            {
              RDK_LOG(RDK_LOG_WARN, "LOG.RDK.INBSI", "<%s: %s> - No CA Descriptors in the CAT\n", PSIMODULE, __FUNCTION__);
            }

            ts_id = transport_stream_id;

            rmf_osal_mutexRelease( m_mediainfo_mutex);

            if (0 == num_cat_sections)
            {
                /*Table is fully acquired.*/
                 if(!m_bIsCATAvailable)
                 {
                    m_bIsCATAvailable = TRUE;
                    rmf_osal_condSet(g_inbsi_cat_cond);
                 }

                 RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.INBSI", "<%s: %s> - notifySIDB(CAT)\n", PSIMODULE, __FUNCTION__);
                 notifySIDB(TABLE_TYPE_CAT, catVersionNumber == INIT_TABLE_VERSION, catVersionNumber != version_number,catCRC != crc );
                 catVersionNumber = version_number;
                 catCRC = crc;
                 if(TRUE == m_NegativeFiltersEnabled)
                 {
                    configure_negative_filters(TABLE_TYPE_CAT);
                 }
            }
            else
            {
                catVersionNumber = version_number;
                catCRC = crc;
            }
        }
        else
        {
            if((num_cat_sections==0) && !m_bIsCATAvailable)
            {
                m_bIsCATAvailable = TRUE;
                rmf_osal_condSet(g_inbsi_cat_cond);
            }
        }
#endif

#ifdef ENABLE_INB_SI_CACHE
          m_pSiCache->ReleaseWriteLock();
#endif

    }
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.INBSI", "<%s: %s> - Exited method - : %d\n", PSIMODULE, __FUNCTION__, cat_error);
    return cat_error;
}
#else
rmf_Error rmf_InbSiMgr::parse_and_update_CAT(uint32_t section_size, uint8_t *section_data) { return RMF_INBSI_INVALID_PARAMETER; }
#endif

rmf_Error rmf_InbSiMgr::parse_and_update_PAT(uint32_t section_size, uint8_t *section_data)
{
    pat_table_struc1 *ptr_table_struc1;
    pat_table_struc2 *ptr_table_struc2;

#ifdef ENABLE_INB_SI_CACHE
    rmf_SiTransportStreamHandle ts_handle = RMF_SI_INVALID_HANDLE;
    uint32_t frequency = 0;
#else
    rmf_Error ret;
    rmf_pmtInfo_t pmtInfo[PMT_LIST_MAX_LEN];
#endif

    uint8_t *ptr_input_buffer = NULL;

    uint16_t version_number = 0;
    uint16_t current_next_indicator = 0;
    uint16_t unknown_pid = 0;
    uint16_t transport_stream_id = 0;
    uint16_t program_number = PROGRAM_NUMBER_UNKNOWN;

    uint32_t section_length = 0;
    uint32_t number_of_PMTs = 0;
    uint32_t crc = 0;
    rmf_Error pat_error = NO_PARSING_ERROR;
    uint32_t sidbVersionNumber = INIT_TABLE_VERSION;
    uint32_t sidbCrc32 = 0;
    rmf_osal_Bool reset = FALSE;
    rmf_osal_Bool primary_program = FALSE;
    uint32_t pn = PROGRAM_NUMBER_UNKNOWN;

    //***********************************************************************************************
    //**********************************************************************************************
    // Get,parse and validate the first 64 bytes (Table_ID thru Last_Section_Number)of the PAT
    // table (see document ISO/IEC 13818-1:2000(E),pg.43) .
    //**********************************************************************************************

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.INBSI",
            "<%s: %s> - Entered method.\n",
            PSIMODULE, __FUNCTION__);

    //ts_handle = fsm_data->ts_handle;

    /*for(i=0;i<section_size;i++)
    {
        RDK_LOG(RDK_LOG_TRACE3, "LOG.RDK.INBSI",
                "<%s: %s> - section_data[%d] = 0x%x \n", PSIMODULE, __FUNCTION__, i, section_data[i]);
    }
    */


    RDK_LOG(RDK_LOG_TRACE3, "LOG.RDK.INBSI",
            "<%s: %s> - section_size = %d \n", PSIMODULE, __FUNCTION__,
            section_size);

    ptr_input_buffer = (uint8_t *)section_data;

    // QUESTION: Does the section_data include header??
    // If it does not we don't need to parse the section_length
    // field to validate
    ptr_table_struc1 = (pat_table_struc1 *) ptr_input_buffer;

    //**** validate table Id field ****
    if (ptr_table_struc1->table_id_octet1 != PROGRAM_ASSOCIATION_TABLE_ID)
    {
        pat_error = 1;
        goto PARSING_DONE;
    }
    
    //**** validate section_syntax_indicator field ****
    if (!TESTBITS(ptr_table_struc1->section_length_octet2, 8, 8))
    {
        // bit not set to 1
        pat_error = 3;
        RDK_LOG(RDK_LOG_TRACE1,
            "LOG.RDK.INBSI",
            "<%s: %s> - *** ERROR *** section_syntax_indicator is clear.\n",
            PSIMODULE, __FUNCTION__);
        goto PARSING_DONE;
    }

    //**** extract and validate section_length field (octets 2 & 3)       ****
    //**** note, bits 12 and 11 must be set to zero and max value is 1021.****

    section_length = ((GETBITS(ptr_table_struc1->section_length_octet2, 4,
            1)) << 8) | ptr_table_struc1->section_length_octet3;

    if (section_length > MAX_IB_SECTION_LENGTH)
    {
        // error, section_length too.
        pat_error = 2;
        goto PARSING_DONE;
    }

    RDK_LOG(RDK_LOG_TRACE3, "LOG.RDK.INBSI",
            "<%s: %s> - section_length = %d \n", PSIMODULE, __FUNCTION__,
            section_length);

    // Subtract the tableId and section_length field's sizes
    // Should match the section_length parsed from PAT
    if(section_size-LEN_SECTION_HEADER != section_length)
    {
        // error, section_length invalid.
        pat_error = 3;
        goto PARSING_DONE;
    }

    RDK_LOG(RDK_LOG_TRACE1,
            "LOG.RDK.INBSI",
            "<%s: %s> - value for num_sections: %d\n",
                PSIMODULE, __FUNCTION__, num_sections);

    if(num_sections == 0)
    {
        RDK_LOG(RDK_LOG_TRACE1,
                "LOG.RDK.INBSI",
                "<%s: %s> - ptr_table_struc1->last_section_number_octet8: %d\n",
                PSIMODULE, __FUNCTION__, ptr_table_struc1->last_section_number_octet8);
        num_sections = ptr_table_struc1->last_section_number_octet8 +1;
        RDK_LOG(RDK_LOG_TRACE1,
                "LOG.RDK.INBSI",
                "<%s: %s> - new value for num_sections: %d\n",
                PSIMODULE, __FUNCTION__, num_sections);
    }

    //**** extract and validate transport_stream_id field (octets 4 & 5) *****/
    transport_stream_id = (uint16_t)(((ptr_table_struc1->transport_stream_id_octet4) << 8)
                    | ptr_table_struc1->transport_stream_id_octet5);

    RDK_LOG(RDK_LOG_TRACE3, "LOG.RDK.INBSI",
            "<%s: %s> - transport_stream_id = %d \n", 
            PSIMODULE, __FUNCTION__,
            transport_stream_id);

    //**** validate 2 reserved bits ****
    if (GETBITS(ptr_table_struc1->current_next_indic_octet6, 8, 7) != 3)
    {
        // bits 8 and 7 are not set to 1 per ISO/IEC 13818-1:2000(E),pg.4, para 2.1.46)
        //pat_error = 4 ;
        RDK_LOG(RDK_LOG_TRACE3,
                "LOG.RDK.INBSI",
                "<%s: %s> - *** WARNING *** 2-Bit-Reserved Field #3 not set to all ones(1)\n.",
                PSIMODULE, __FUNCTION__);
    }

    //**** extract version_number (0-31) field ****

    version_number = (uint16_t)(GETBITS(ptr_table_struc1->current_next_indic_octet6, 6, 2));

    RDK_LOG(RDK_LOG_TRACE3, "LOG.RDK.INBSI",
            "<%s: %s> - version_number = %d \n", PSIMODULE, __FUNCTION__,
            version_number);
    //**** populate the table info array with the new PAT data ****

    //add_pat_entry(fsm_data, version_number);

    //**** extract current_next_indicator field ****
    current_next_indicator = (uint16_t)(GETBITS(ptr_table_struc1->current_next_indic_octet6, 1, 1));
    RDK_LOG(RDK_LOG_TRACE3, "LOG.RDK.INBSI",
            "<%s: %s> - current_next_indicator = %d\n.",
            PSIMODULE, __FUNCTION__, current_next_indicator);

    //**** section_length identifies the total number of bytes/octets remaining   ****
    //**** in the PAT after octet3. Before performing program number, network pid ****
    //**** and program map pid processing ,we must adjust section_length to       ****
    //**** account for having processed  octets4 thru octet8                      ****.
    section_length -= (sizeof(pat_table_struc1) - LEN_OCTET1_THRU_OCTET3);

    // It is possible that an empty PAT (no programs) is signaled sometimes. In
    // that case all we see is a header and a CRC.
    // Check if the length is less than LENGTH_CRC_32, then its an error.
    if (section_length < LENGTH_CRC_32)
    {
        // error, either a portion of header is missing (section_length <0) or
        // no data after header (section_length = 0) or  section_length is not
        // large enough to hold a CRC_2 and a program_number and pid pair.
        pat_error = 5;
        goto PARSING_DONE;
    }

   /* sitp_ib_tune_info_t* tune_info = (sitp_ib_tune_info_t*) psi_params;
                rmf_osal_mutexRelease(g_sitp_psi_mutex);
    frequency = tune_info->frequency;
    pn = tune_info->program_number; // Tuned program
    mode = tune_info->modulation_mode;*/
    //table_type = IB_PAT;
    reset = TRUE;

#ifdef ENABLE_INB_SI_CACHE
    m_pSiCache->GetTransportStreamEntryFromFrequencyModulation(m_freq, m_modulation_mode, &ts_handle);
    RDK_LOG(RDK_LOG_TRACE3, "LOG.RDK.INBSI",
            "<%s: %s> - ts_handle = 0x%x \n", PSIMODULE, __FUNCTION__, ts_handle);

    m_pSiCache->LockForWrite();
    m_pSiCache->SetTSIdForTransportStream(ts_handle, transport_stream_id);
    m_pSiCache->ReleaseWriteLock();

    //sidbVersionNumber = getSidbVersionNumber(table_type, ts_handle);
    //sidbCrc32 = getSidbCRC(table_type, ts_handle);
    m_pSiCache->GetPATVersionForTransportStreamHandle(ts_handle, &sidbVersionNumber);
    m_pSiCache->GetPATCRCForTransportStreamHandle(ts_handle, &sidbCrc32);

    m_pSiCache->LockForWrite();    
    m_pSiCache->SetPATVersionForTransportStream(ts_handle, version_number);
    m_pSiCache->ReleaseWriteLock();
#endif

    RDK_LOG(RDK_LOG_TRACE3,
            "LOG.RDK.INBSI",
            "<%s: %s> - sidb verison = %d, sidb crc = 0x%x.\n",
            PSIMODULE, __FUNCTION__, sidbVersionNumber, sidbCrc32);
    RDK_LOG(RDK_LOG_TRACE3, "LOG.RDK.INBSI",
            "<%s: %s> - new verison = %d.\n", PSIMODULE, __FUNCTION__,
            version_number);

    //**********************************************************************************************
    // Parse the remaining bytes/octets in the PAT.
    //**********************************************************************************************
    ptr_input_buffer = (uint8_t *) (&section_data[sizeof(pat_table_struc1)]);

    //*****************************************************************************************
    //**** This is the "FOR N Loop" in spec (ISO/IEC 13818-1:2000(E),pg.43, table 2-25)
    //**** Within this loop we will extract and validate program_number and the
    //**** network_pid/program_map_pid.
    //*****************************************************************************************
    while (section_length > LENGTH_CRC_32)
    {
        ptr_table_struc2 = (pat_table_struc2 *) ptr_input_buffer;

        /*
         ** extract the program_number field (octets 1 & 2) and unknown_pid field
         ** ( octetes 3 & 4 ).
         */
        program_number = (uint16_t)(((ptr_table_struc2->program_number_octet1)
                << 8) | ptr_table_struc2->program_number_octet2);
        unknown_pid = (uint16_t)(((ptr_table_struc2->unknown_pid_octet3) << 8)
                | ptr_table_struc2->unknown_pid_octet4);

        if (GETBITS(unknown_pid, 16, 14) != 0x07)
        {
            // error, the 3 reserve bits (16-14, in program_map_pid/network_pid)
            // are not set to "1" // per ISO/IEC 13818-1:2000(E),pg.4, para 2.1.46)
            // pat_error = 6 ;
            RDK_LOG(RDK_LOG_TRACE3,
                    "LOG.RDK.INBSI",
                    "<%s: %s> - *** WARNING *** 3-Bit-Reserved Field #4 not set to all ones(1)\n.",
                    PSIMODULE, __FUNCTION__);
        }

        unknown_pid = (uint16_t)(unknown_pid & 0x1FFF);

        if (!((unknown_pid >= MIN_VALID_PID) && (unknown_pid <= MAX_VALID_PID)))
        {
            // error, invalid  unknown_pid (program_map_pid/network_pid) .
            pat_error = 7;
            goto PARSING_DONE;
        }

        if (program_number != 0)
        {

            if ( (patVersionNumber != version_number) || (INIT_TABLE_VERSION == patVersionNumber) )
            {
                //Dump program number for debug if PAT has been updated.
                RDK_LOG(RDK_LOG_TRACE1,
                        "LOG.RDK.INBSI",
                        "<%s: %s> - Processing Program_Number = %d, tuned program_number = %d.\n",
                        PSIMODULE, __FUNCTION__, program_number, pn);
             }

            RDK_LOG(RDK_LOG_TRACE1,
                    "LOG.RDK.INBSI",
                    "<%s: %s> - ProgramMap_PID = 0x%x(%d).\n",
                    PSIMODULE, __FUNCTION__, unknown_pid, unknown_pid);

            // Process each program found in PAT
        {
#ifdef ENABLE_INB_SI_CACHE
            m_pSiCache->LockForWrite();  
            (void) m_pSiCache->SetPATProgramForTransportStream(ts_handle,
                    program_number, unknown_pid);
            m_pSiCache->ReleaseWriteLock();

            rmf_SiProgramHandle prog_handle = RMF_SI_INVALID_HANDLE;
            (void) m_pSiCache->GetProgramEntryFromTransportStreamEntry(
                    ts_handle, program_number, &prog_handle);
            RDK_LOG(RDK_LOG_TRACE1,
                        "LOG.RDK.INBSI",
                        "<%s: %s> - frequency = %d, program_number = %d, modulation_mode = %d \n",
                        PSIMODULE, __FUNCTION__, m_freq,
                        program_number, m_modulation_mode);

                m_pSiCache->LockForWrite();  
                m_pSiCache->SetPMTPid(prog_handle, unknown_pid);
                m_pSiCache->ReleaseWriteLock();
#else
                if ( number_of_PMTs < PMT_LIST_MAX_LEN )
                {
                    pmtInfo[number_of_PMTs].pmt_pid = unknown_pid;
                    pmtInfo[number_of_PMTs].program_no= program_number;
                }
#endif
                /* increment Program_Number/PMT count */
                number_of_PMTs++;

                if(program_number == pn)
                {
                    RDK_LOG(RDK_LOG_TRACE1,
                            "LOG.RDK.INBSI",
                            "<%s: %s> - primary program number = %d.\n",
                            PSIMODULE, __FUNCTION__, program_number);

                    // Identify the primary program number
                    // (for DSG/HN/OOB there is only one program)
                    primary_program = TRUE;
                }
                else
                {
                    primary_program = FALSE;
                }

                RDK_LOG(RDK_LOG_TRACE1,
                        "LOG.RDK.INBSI",
                        "<%s: %s> - number_of_PMTS = %d\n",
                        PSIMODULE, __FUNCTION__, number_of_PMTs);
            }
        }
        else
        {
            //**** unknown_pid is a "network_pid", ignore it.
            // do nothing
            RDK_LOG(RDK_LOG_TRACE3,
                    "LOG.RDK.INBSI",
                    "<%s: %s> - Ignoring Program_Number = %d, Network_PID = x%x.\n",
                    PSIMODULE, __FUNCTION__, program_number, unknown_pid);
        }

        //**** prepare to process the next program_number and
        //**** unknown_pid (program_map_pid/network_pid) pair.
        if (section_length >= (sizeof(pat_table_struc2) + LENGTH_CRC_32))
        {
            section_length -= sizeof(pat_table_struc2);
            ptr_input_buffer += sizeof(pat_table_struc2);
        }

    } // End while(section_length > LENGTH_CRC_32)


    //*************************************************************************************************
    //* At this point, the code either terminated prematurely or successfully processed the PMT.
    //* We need to validate the present of the CRC_32 field and cleanup before returning to the caller.
    //*************************************************************************************************
    if ((section_length != LENGTH_CRC_32))
    {
        // error, either the last program_number and program_map_pid/network_pid) pair
        // had an incorrect number of bytes or the CRC_32 had an incorrect number of bytes.
        RDK_LOG(RDK_LOG_ERROR,
                "LOG.RDK.INBSI",
                "<%s: %s> - section length(%d) != LENGHT_CRC_32(%d) - error  - %d\n",
                PSIMODULE, __FUNCTION__, section_length, LENGTH_CRC_32, pat_error);
        pat_error = 13;
        goto PARSING_DONE;
    }

    RDK_LOG(RDK_LOG_TRACE3, "LOG.RDK.INBSI",
            "<%s: %s> - section_length = %u\n", PSIMODULE, __FUNCTION__,
            section_length);
    if (section_length == LENGTH_CRC_32)
    {
        crc = ((*ptr_input_buffer) << 24 | (*(ptr_input_buffer + 1)) << 16
                | (*(ptr_input_buffer + 2)) << 8 | (*(ptr_input_buffer + 3)));
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.INBSI",
                "<%s: %s> - CRC32 = 0x%08x\n", PSIMODULE, __FUNCTION__, crc);
    }

#ifdef ENABLE_INB_SI_CACHE
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.INBSI",
            "<%s: %s> - ts_handle = 0x%x\n", PSIMODULE, __FUNCTION__,
            ts_handle);

    m_pSiCache->LockForWrite();
    m_pSiCache->SetPATCRCForTransportStream(ts_handle, crc);
    m_pSiCache->ReleaseWriteLock();
#endif

PARSING_DONE:

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.INBSI",
            "<%s: %s> - Exiting with ErrorCode = %d\n",
            PSIMODULE, __FUNCTION__, pat_error);

#ifdef ENABLE_INB_SI_CACHE
    m_pSiCache->LockForWrite();
#endif

    if (pat_error != 0)
    {
#ifdef ENABLE_INB_SI_CACHE
        m_pSiCache->SetPATStatusNotAvailable(ts_handle);
#endif
    }
    else
    {
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.INBSI",
                "<%s: %s> - Calling SetPATStatus\n",
                PSIMODULE, __FUNCTION__);
#ifdef ENABLE_INB_SI_CACHE
        m_pSiCache->SetPATStatusAvailable(ts_handle);
        notifySIDB(TABLE_TYPE_PAT, sidbVersionNumber == INIT_TABLE_VERSION, sidbVersionNumber != version_number,sidbCrc32 != crc, ts_handle);
#else
        num_sections--;
        RDK_LOG(RDK_LOG_TRACE1,
                "LOG.RDK.INBSI",
                "<%s: %s> - num_sections: %d, m_bIsPATAvailable: %d\n",
                PSIMODULE, __FUNCTION__, num_sections, m_bIsPATAvailable);

        if ( (patVersionNumber == INIT_TABLE_VERSION) || (patVersionNumber != version_number) ||
            (patCRC != crc ))
        {
#if 0 //Ericz, remove
            printf("Dumping PAT\n");
            for(int ix = 0; ix < section_size; ix++)
            {
                printf("0x%x,  ", section_data[ix]);
            }
            printf("  End Dumping PAT\n");
#endif
            if(TRUE == m_dumpPSITables)
            {
                dumpPSIData(section_data, section_size);
            }

			
	                
            RDK_LOG(RDK_LOG_DEBUG,
                    "LOG.RDK.INBSI",
                    "<%s: %s> -  Notifying PAT .patVersionNumber= %d, version_number = %d patCRC=0x%x, crc=0x%x, number_of_PMTs=%d m_pmtListMaxSize=%d\n",
                    PSIMODULE, __FUNCTION__, patVersionNumber, version_number, patCRC, crc, number_of_PMTs, m_pmtListMaxSize);
            rmf_osal_mutexAcquire( m_mediainfo_mutex);
            if ( number_of_PMTs > m_pmtListMaxSize )
            {
                ret = rmf_osal_memReallocP(RMF_OSAL_MEM_SI_INB, 
                        number_of_PMTs*sizeof(rmf_pmtInfo_t),
                        (void **) &m_pmtInfoList);
                if ( RMF_SUCCESS != ret)
                {
                    RDK_LOG(RDK_LOG_ERROR,
                            "LOG.RDK.INBSI",
                            "<%s: %s> - rmf_osal_memReallocP failed.\n",
                            PSIMODULE, __FUNCTION__);
                    rmf_osal_mutexRelease( m_mediainfo_mutex);
                    return 25;
                }
                m_pmtListMaxSize = number_of_PMTs;
            }
            memcpy( m_pmtInfoList, pmtInfo, number_of_PMTs*sizeof(rmf_pmtInfo_t));
            m_pmtListLen = number_of_PMTs;
            ts_id = transport_stream_id;
            rmf_osal_mutexRelease( m_mediainfo_mutex); 
            
            if(0 == num_sections)
            {
                /*Table is fully acquired.*/
                 if(!m_bIsPATAvailable)
                 {
                    m_bIsPATAvailable = TRUE;
                    rmf_osal_condSet(g_inbsi_pat_cond);
                 }
                 notifySIDB(TABLE_TYPE_PAT, patVersionNumber == INIT_TABLE_VERSION, 
                    patVersionNumber != version_number,patCRC != crc );            
                 patVersionNumber = version_number;
                 patCRC = crc;
                 if(TRUE == m_NegativeFiltersEnabled)
                 {
                    configure_negative_filters(TABLE_TYPE_PAT);
                 }
            }
            else
            {
                patVersionNumber = version_number;
                patCRC = crc;
            }
        }
        else
        {
            if((num_sections==0) && !m_bIsPATAvailable)
            {
                    m_bIsPATAvailable = TRUE;
                    rmf_osal_condSet(g_inbsi_pat_cond);
            }
        }
#endif

        /*
         * Reset PMT status to 'NOT_AVAILABLE' for all programs on transport stream
         * not signaled in the newly acquired PAT
         */
        if (reset)
        {
            RDK_LOG(RDK_LOG_TRACE1,
                    "LOG.RDK.INBSI",
                    "<%s: %s> - Calling ResetPATProgramStatus\n",
                    PSIMODULE, __FUNCTION__);
#ifdef ENABLE_INB_SI_CACHE
            m_pSiCache->ResetPATProgramStatus(ts_handle);
#endif
        }


#ifdef ENABLE_INB_SI_CACHE    
    m_pSiCache->ReleaseWriteLock();
#endif

    }
    return pat_error;
}

/**
 * Retrieve PMT and update SI database.
 *
 * @param section_size The size of PMT section.
 * @param section_data The section for retrieving PMT data.
 * @return RMF_INBSI_ECOND if the table id is incorrect;
 *         RMF_INBSI_SUCCESS if succeeds;
 *         other error code on error condition.
 */
#if 0 /*Test code*/
static uint8_t pmtbuf[] = {0x2,  0xb0,  0xa4,  0x0,  0x3,  0xd5,  0x0,  0x0,  0xfa,  0x3f,  0xf0,  0x24,  0x5,  0x4,  0x47,  0x41,  0x39,  0x34,  0x10,  0x6,  0xc0,  0xaf,  0xc8,  0xc0,  0x2,  0x0,  0xa3,  0x11,  0x1,  0x65,  0x6e,  0x67,  0x1,  0x0,  0x0,  0x9,  0x57,  0x48,  0x59,  0x59,  0x20,  0x48,  0x44,  0x20,  0x31,  0xaa,  0x1,  0xff,  0x2,  0xfa,  0x3f,  0xf0,  0xc,  0x6,  0x1,  0x2,  0x86,  0x7,  0xe1,  0x65,  0x6e,  0x67,  0xc1,  0x7f,  0xff,  0x81,  0xfa,  0x40,  0xf0,  0x2b,  0x5,  0x4,  0x41,  0x43,  0x2d,  0x33,  0xa3,  0x15,  0x1,  0x65,  0x6e,  0x67,  0x1,  0x0,  0x0,  0xd,  0x50,  0x72,  0x69,  0x6d,  0x61,  0x72,  0x79,  0x20,  0x41,  0x75,  0x64,  0x69,  0x6f,  0x81,  0x6,  0x8,  0x28,  0x5,  0x0,  0x1f,  0x1,  0xa,  0x4,  0x65,  0x6e,  0x67,  0x0,  0x81,  0xfa,  0x41,  0xf0,  0x2d,  0x5,  0x4,  0x41,  0x43,  0x2d,  0x33,  0xa3,  0x17,  0x1,  0x65,  0x6e,  0x67,  0x1,  0x0,  0x0,  0xf,  0x53,  0x65,  0x63,  0x6f,  0x6e,  0x64,  0x61,  0x72,  0x79,  0x20,  0x41,  0x75,  0x64,  0x69,  0x6f,  0x81,  0x6,  0x8,  0x28,  0x45,  0x0,  0x1,  0x1,  0xa,  0x4,  0x65,  0x6e,  0x67,  0x0,  0xd5,  0x9c,  0x15,  0x35};
#endif
rmf_Error rmf_InbSiMgr::parse_and_update_PMT(uint32_t section_size, uint8_t *section_data)
{
    pmt_table_struc1 *ptr_table_struc1 = NULL;
    pmt_table_struc3 *ptr_table_struc3 = NULL;
    generic_descriptor_struc *ptr_descriptor_struc = NULL;

#ifdef ENABLE_INB_SI_CACHE
    rmf_SiProgramHandle si_program_handle = RMF_SI_INVALID_HANDLE; // a handle into this SI database element
    uint32_t sidbCrc32 = 0;
#else
     uint8_t * ptr_outer_desc = NULL;
    uint32_t outer_desc_len = 0;
    uint8_t * ptr_es_desc = NULL;
    uint32_t es_desc_len = 0;
#endif

    uint8_t *ptr_input_buffer = NULL;

    uint32_t es_info_length = 0, section_length = 0;
    uint32_t crc = 0;
    uint16_t program_number = PROGRAM_NUMBER_UNKNOWN, version_number = 0, pcr_pid = 0,
            program_info_length = 0, elementary_pid = 0, stream_type = 0,
            descriptor_tag = 0, descriptor_length = 0;

    rmf_Error pmt_error = NO_PARSING_ERROR;
    //rmf_Error retCode = 0;
    uint32_t sidbVersionNumber = INIT_TABLE_VERSION;
    //uint32_t frequency = 0;
    //uint32_t dsg_app_id = 0;
    //mpe_HnStreamSession hn_stream_session = NULL;
    //uint8_t mode = 0;
    //rmf_SiTableType table_type = OOB_PMT;

    //***********************************************************************************************
    //**********************************************************************************************
    // Get,parse and validate the first 96 bytes (Table_ID thru Program_Info_length)of the PMT
    // table (see document ISO/IEC 13818-1:2000(E),pg.46) .
    //**********************************************************************************************
    RDK_LOG(RDK_LOG_TRACE1,
            "LOG.RDK.INBSI",
            "<%s: %s> - Entered method parseAndUpdatePMT..\n",
            PSIMODULE, __FUNCTION__);
#if 0 /*Test code*/
    section_data = pmtbuf;
    section_size = sizeof(pmtbuf);
#endif
    ptr_input_buffer = (uint8_t *)section_data;

//    ts_handle = fsm_data->ts_handle;
    //RDK_LOG(RDK_LOG_TRACE1,
      //      "LOG.RDK.INBSI",
        //    "<%s: %s> - parseAndUpdatePMT: ts_handle: 0x%x\n",
          //  __FUNCTION__, ts_handle);

    ptr_table_struc1 = (pmt_table_struc1 *) ptr_input_buffer;

    //**** validate table Id field ****
    if (ptr_table_struc1->table_id_octet1 != PROGRAM_MAP_TABLE_ID)
    {
        pmt_error = 2;
        goto PARSING_DONE;
    }

    //**** validate section_syntax_indicator field ****
    if (!TESTBITS(ptr_table_struc1->section_length_octet2, 8, 8))
    {
        // bit not set to 1
        pmt_error = 3;
        goto PARSING_DONE;
    }

    //**** validate '0' bit field ****
    if (TESTBITS(ptr_table_struc1->section_length_octet2, 7, 7))
    {
        // bit not set to 0
        //pmt_error = 4 ;
        RDK_LOG(RDK_LOG_TRACE3,
                "LOG.RDK.INBSI",
                "<%s: %s> - *** WARNING *** 1-Bit-Zero Field #1 not set to all zeroes(0)\n.",
                PSIMODULE, __FUNCTION__);
    }

    //**** validate 2 reserved bits ****
    if (GETBITS(ptr_table_struc1->section_length_octet2, 6, 5) != 3)
    {
        // bits 5 and 6 are not set to 1 per ISO/IEC 13818-1:2000(E),pg.4, para 2.1.46)
        //pmt_error = 5 ;
        RDK_LOG(RDK_LOG_TRACE3,
                "LOG.RDK.INBSI",
                "<%s: %s> - *** WARNING *** 2-Bit-Reserved Field #2 not set to all ones(1)\n.",
                PSIMODULE, __FUNCTION__);
    }

    //**** extract and validate section_length field (octets 2 & 3)       ****
    //**** note, bits 12 and 11 must be set to zero and max value is 1021.****
    section_length = ((GETBITS(ptr_table_struc1->section_length_octet2, 4, 1))
            << 8) | ptr_table_struc1->section_length_octet3;

    if ((section_length > MAX_IB_SECTION_LENGTH) || (section_length
            < (sizeof(pmt_table_struc1) - LEN_OCTET1_THRU_OCTET3)))
    { // error, section_length too long or PMT header is too short.
        pmt_error = 6;
        goto PARSING_DONE;
    }

    // Subtract the tableId and section_length field's sizes
    // Should match the section_length parsed from PMT
    if(section_size-LEN_SECTION_HEADER != section_length)
    {
        // Length fields don't match
        pmt_error = 7;
        goto PARSING_DONE;
    }

    //**** extract and validate program_number field (octets 4 & 5) *****
    program_number = (uint16_t)(((ptr_table_struc1->program_number_octet4) << 8)
                    | ptr_table_struc1->program_number_octet5);

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.INBSI",
            "<%s: %s> - PMT's Program_Number = %d.\n",
            PSIMODULE, __FUNCTION__, program_number);

    //**** validate 2 reserved bits ****
    if (GETBITS(ptr_table_struc1->current_next_indic_octet6, 8, 7) != 3)
    {
        // bits 8 and 7 are not set to 1 per ISO/IEC 13818-1:2000(E),pg.4, para 2.1.46)
        //pmt_error = 8 ;
        RDK_LOG(RDK_LOG_TRACE3,
                "LOG.RDK.INBSI",
                "<%s: %s> - *** WARNING *** 2-Bit-Reserved Field #3 not set to all ones(1)\n.",
                PSIMODULE, __FUNCTION__);
    }

    //**** extract version_number (0-31) field ****
    version_number = (uint16_t)(GETBITS(
            ptr_table_struc1->current_next_indic_octet6, 6, 2));
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.INBSI",

            "<%s: %s> - PMT's version_number = %d.\n",
            PSIMODULE, __FUNCTION__, version_number);

    //**** validate section_number and last_section_number ****
    if (ptr_table_struc1->section_number_octet7 != 0)
    {
        pmt_error = 10;
        goto PARSING_DONE;
    }

    if (ptr_table_struc1->last_section_number_octet8 != 0)
    {
        pmt_error = 11;
        goto PARSING_DONE;
    }

    //**** validate 3 reserved bits ****
    if (GETBITS(ptr_table_struc1->pcr_pid_octet9, 8, 6) != 0x07)
    {
        // bits 8,7 and 6 are not set to 1 per ISO/IEC 13818-1:2000(E),pg.4, para 2.1.46)
        //  pmt_error = 12;
        RDK_LOG(RDK_LOG_TRACE3,
                "LOG.RDK.INBSI",
                "<%s: %s> - *** WARNING *** 3-Bit-Reserved Field #4 not set to all ones(1)\n.",
                PSIMODULE, __FUNCTION__);
    }

    //**** extract and validate 13 bit pcr_pid from octets 9 and 10 ****
    pcr_pid = (uint16_t)(((ptr_table_struc1->pcr_pid_octet9 & 0x1f) << 8)
            | ptr_table_struc1->pcr_pid_octet10);

    RDK_LOG(RDK_LOG_TRACE3, "LOG.RDK.INBSI",
            "<%s: %s> - Pcr_Pid= x%x.\n", PSIMODULE, __FUNCTION__, pcr_pid);

    if ((pcr_pid > CAT_PID) && (pcr_pid < MIN_VALID_PID))
    {
        //*** valid pcr pids are PMT_PID thru CAT_PID,MIN_VALID_PID thru MAX_VALID_PID
        //*** and NO_PCR_PID.  see ISO/IEC 13818-1:2000(E),pg.47, para 2.4.4.9,
        //*** pg. 19, table 2-3
        pmt_error = 13;
        goto PARSING_DONE;
    }

    //**** validate 4 reserved bits ****
    if (GETBITS(ptr_table_struc1-> program_info_length_octet11, 8, 5) != 0x0f)
    {
        // bits 8 thru are not set to 1 per ISO/IEC 13818-1:2000(E),pg.4, para 2.1.46)
        //pmt_error = 14 ;
        RDK_LOG(RDK_LOG_TRACE3,
                "LOG.RDK.INBSI",
                "<%s: %s> - *** WARNING *** 4-Bit-Reserved Field #5 not set to all ones(1)\n.",
                PSIMODULE, __FUNCTION__);
    }

    //**** extract and validate the 12 bit program_info_length field from octets 11 and 12 ****
    //**** note, bits 12 and 11 must be set to zero,hence, max value is (2**10) -1 = 1023. ****
    program_info_length = (uint16_t)(
            ((ptr_table_struc1->program_info_length_octet11 & 0x0f) << 8)
                    | ptr_table_struc1->program_info_length_octet12);

    RDK_LOG(RDK_LOG_TRACE3,
            "LOG.RDK.INBSI",
            "<%s: %s> program_info_length octet11 = %d, program_info_length_octet12 = %d\n.",
            PSIMODULE, __FUNCTION__, ptr_table_struc1->program_info_length_octet11,
            ptr_table_struc1->program_info_length_octet12);

    if (program_info_length > MAX_PROGRAM_INFO_LENGTH)
    {
        pmt_error = 15;
        goto PARSING_DONE;
    }

    //**** section_length identifies the total number of bytes/octets remaining ****
    //**** in the PMT after octet3. Before processing the descriptor section    ****
    //**** below,we must adjust section_length to account for having processed  ****
    //**** octets4 thru octet12.
    section_length -= (sizeof(pmt_table_struc1) - LEN_OCTET1_THRU_OCTET3);

    if ((section_length <= 0) || (program_info_length > section_length))
    { // error, section_length too short or program_info_length too long.
        pmt_error = 16;
        goto PARSING_DONE;
    }
    RDK_LOG(RDK_LOG_TRACE3,
            "LOG.RDK.INBSI",
            "<%s: %s> - Outer Descriptor Loop Length(program_info_length): %d\n",
            PSIMODULE, __FUNCTION__, program_info_length);

    //**********************************************************************************************
    // Get the descriptor octets which immediately following the program_info_length field.
    // The total number of descriptor bytes/octets is defined by program_info_length.
    // Note: for the june-07-2004 release, we will not process the block of descriptors.Hence,
    // it is necessary to jump over/ignore the descriptor block.
    //**********************************************************************************************

    section_length -= program_info_length;
    ptr_input_buffer = &section_data[sizeof(pmt_table_struc1)];

#ifdef ENABLE_INB_SI_CACHE
    // Fetch program handle from service handle, to ensure there's at least 1 service associated with the program.
    if (m_pSiCache->GetProgramEntryFromFrequencyProgramNumberModulation(
        m_freq, program_number, m_modulation_mode,
        &si_program_handle)== RMF_INBSI_SUCCESS)
    {
        (void) m_pSiCache->GetPMTVersionForProgramHandle(
            si_program_handle, &sidbVersionNumber);
        (void) m_pSiCache->GetPMTCRCForProgramHandle(si_program_handle,
            &sidbCrc32);
        RDK_LOG(RDK_LOG_TRACE3,
            "LOG.RDK.INBSI",
            "<%s: %s> - IB sidbVersionNumber = %d, sidbCrc32 = 0x%x\n",
                PSIMODULE, __FUNCTION__,
            sidbVersionNumber,
            sidbCrc32);

        m_pSiCache->LockForWrite();
        (void) m_pSiCache->SetPMTVersion(si_program_handle, version_number);
        m_pSiCache->SetPCRPid(si_program_handle, pcr_pid);
        m_pSiCache->ReleaseWriteLock();
    }

    // Parse the program element  outer descriptors.
    while (program_info_length)
    {
        // Get the descriptor tag and length and point to the contents.
        ptr_descriptor_struc = (generic_descriptor_struc *) ptr_input_buffer;
        descriptor_tag = ptr_descriptor_struc->descriptor_tag_octet1;
        descriptor_length = ptr_descriptor_struc->descriptor_length_octet2;
        ptr_input_buffer += sizeof(generic_descriptor_struc);

        RDK_LOG(RDK_LOG_TRACE3,
                "LOG.RDK.INBSI",
                "<%s: %s> - Call SetOuterDescriptor, setting outer tag: %d, length: %d.\n",
                PSIMODULE, __FUNCTION__, descriptor_tag, descriptor_length);
        if (si_program_handle != RMF_SI_INVALID_HANDLE)
        {
            // Set the descriptor in the SIDB.
            (void) m_pSiCache->SetOuterDescriptor(si_program_handle,
                    (rmf_SiDescriptorTag) descriptor_tag,
                    (uint32_t) descriptor_length, (void *) ptr_input_buffer);
        }

        // Adjust the program info length for the tag, length and contents.
        // Point the input buffer to the next descriptor.
        program_info_length = (uint16_t)(program_info_length
                - sizeof(generic_descriptor_struc));
        program_info_length = (uint16_t)(program_info_length
                - descriptor_length);
        ptr_input_buffer = ptr_input_buffer + descriptor_length;
    }
#else
    /*Store the pointer and size  of outer descriptor*/
    ptr_outer_desc = ptr_input_buffer;
    outer_desc_len = program_info_length;
    ptr_input_buffer+= program_info_length;
    ptr_es_desc = ptr_input_buffer;
    es_desc_len = section_length;
#endif


    RDK_LOG(RDK_LOG_TRACE3,
            "LOG.RDK.INBSI",
            "<%s: %s> - Setting length past descriptor data: %d, adjust section_length.\n",
            PSIMODULE, __FUNCTION__, section_length);

    //*****************************************************************************************
    //* This is the "FOR N1 Loop" in spec (ISO/IEC 13818-1:2000(E),pg.46, table 2-28)
    //* Note: the "- LENGTH_CRC_32" in the line immediately below is required to prevent
    //* the CRC_32 from being parsed as elementary stream header (aka: pmt_table_struc3) .
    //*****************************************************************************************

    for (; ((section_length - LENGTH_CRC_32) >= sizeof(pmt_table_struc3));)
    {
        ptr_table_struc3 = (pmt_table_struc3 *) ptr_input_buffer;

        //**** extract and validate the stream_type ****
        stream_type = ptr_table_struc3->stream_type_octet1;

        RDK_LOG(RDK_LOG_TRACE3, "LOG.RDK.INBSI",
                "<%s: %s> - Stream_Type = x%2x.\n", PSIMODULE, __FUNCTION__,
                stream_type);

        if (!(((stream_type >= MIN_VALID_NONPRIVATE_STREAM_TYPE)
                && (stream_type <= MAX_VALID_NONPRIVATE_STREAM_TYPE))
                || ((stream_type >= MIN_VALID_PRIVATE_STREAM_TYPE)
                        && (stream_type <= MAX_VALID_PRIVATE_STREAM_TYPE))))
        {
            RDK_LOG(RDK_LOG_WARN, "LOG.RDK.INBSI",
                    "<%s: %s> - Invalid Stream_Type = x%2x.\n",
                    PSIMODULE, __FUNCTION__, stream_type);
        }

#ifndef SA_NON_CONFORMANCE_ISSUE
        if ((pcr_pid == NO_PCR_PID) && (stream_type
                >= MIN_VALID_NONPRIVATE_STREAM_TYPE) && (stream_type
                <= MAX_VALID_NONPRIVATE_STREAM_TYPE))
        { //**** error,NO_PCR_PID may only be used with "private streams. ****
            //**** see ISO/IEC 13818-1:2000(E),pg.47, PCR_PID.              ****
            pmt_error = 20;
            goto PARSING_DONE;
        }
#endif

        //**** validate 3 reserved bits ****
        if (GETBITS(ptr_table_struc3->elementary_pid_octet2, 8, 6) != 0x07)
        {
            // bits 8,7 & 6 are not set to 1 per ISO/IEC 13818-1:2000(E),pg.4, para 2.1.46)
            //pmt_error = 21;
            //break ;
            RDK_LOG(RDK_LOG_TRACE3,
                    "LOG.RDK.INBSI",
                    "<%s: %s> - *** WARNING *** 3-Bit-Reserved Field #5 not set to all ones(1)\n.",
                    PSIMODULE, __FUNCTION__);
        }

        //**** get elementary_pid (bits 5-1 in octet2 and 8-1 in octet3 ****
        elementary_pid = (uint16_t)(((ptr_table_struc3->elementary_pid_octet2
                & 0x1f) << 8) | ptr_table_struc3->elementary_pid_octet3);

        RDK_LOG(RDK_LOG_TRACE3, "LOG.RDK.INBSI",
                "<%s: %s> - Elementary_Pid= x%x.\n", PSIMODULE, __FUNCTION__,
                elementary_pid);

        //**** validate 4 reserved bits ****
        if (GETBITS(ptr_table_struc3->es_info_length_octet4, 8, 5) != 0x0f)
        {
            // bits 8,7,6 & 5 are not set to 1 per ISO/IEC 13818-1:2000(E),pg.4, para 2.1.46)
            //pmt_error = 22;
            //break ;
            RDK_LOG(RDK_LOG_TRACE3,
                    "LOG.RDK.INBSI",
                    "<%s: %s> - *** WARNING *** 4-Bit-Reserved Field #6 not set to all ones(1)\n.",
                    PSIMODULE, __FUNCTION__);
        }

        //**** extract and validate the 12 bit es_info_length field from octets 4 & 5 ****
        //**** note, bits 12 and 11 must be set to zero,hence, max value is           ****
        //**** (2**10) -1 = 1023.                                                     ****
        es_info_length
                = ((ptr_table_struc3->es_info_length_octet4 & 0x0f) << 8)
                        | ptr_table_struc3->es_info_length_octet5;

        RDK_LOG(RDK_LOG_TRACE3, "LOG.RDK.INBSI",
                "<%s: %s> - es_info_length=%d.\n", PSIMODULE, __FUNCTION__,
                es_info_length);

        if (es_info_length > MAX_ES_INFO_LENGTH)
        {
            RDK_LOG(RDK_LOG_TRACE3,
                    "LOG.RDK.INBSI",
                    "<%s: %s> - *** WARNING *** bits 11 & 12 not 0\n",
                    PSIMODULE, __FUNCTION__);
            //pmt_error = 23 ;
            //goto PARSING_DONE;

            /* removing reserve bits */
            es_info_length = es_info_length & 0x03ff;
        }

        //**** update section_length and ptr to input buffer to account for the last ****
        //**** sizeof(pmt_table_struc3) octets (i.e.,stream_type thru es_info_len)   ****
        //**** and get the descriptor block associated with the elementary stream.   ****
        section_length -= sizeof(pmt_table_struc3);
        ptr_input_buffer += sizeof(pmt_table_struc3);

        if (es_info_length > section_length)
        { // error, descriptor(s) are outside of the PMT .
            RDK_LOG(RDK_LOG_TRACE3,
                    "LOG.RDK.INBSI",
                    "<%s: %s> - *** ERROR *** es_info_length(%d) > section_length(%d)\n",
                    PSIMODULE, __FUNCTION__, es_info_length, section_length);
            pmt_error = 24;
            goto PARSING_DONE;
        }

#ifdef ENABLE_INB_SI_CACHE
        if (si_program_handle != RMF_SI_INVALID_HANDLE)
        {
            m_pSiCache->LockForWrite();
        
            // allocates memory to hold the stream information, hence,
            // we only need to do it once per strea_type.
            (void) m_pSiCache->SetElementaryStream(si_program_handle, (rmf_SiElemStreamType)stream_type,
                    elementary_pid);
            //*** RR added on 09-09-04 in support of org.ocap.si
            m_pSiCache->SetESInfoLength(si_program_handle, elementary_pid,
                    (rmf_SiElemStreamType)stream_type, es_info_length);

            m_pSiCache->ReleaseWriteLock();
        }
#endif

        //*****************************************************************************************
        //* This is the "FOR N2 Loop" in spec (ISO/IEC 13818-1:2000(E),pg.46, table 2-28)
        //* Note: if es_info_length = 0, the N2 Loop won't be entered.Hence, no data associated
        //* with the elementary_stream defined by elementary_pid (in pmt_table_struc3) will be
        //* written to the SIDB.
        //*****************************************************************************************
        for (; es_info_length >= sizeof(generic_descriptor_struc);)
        {
            ptr_descriptor_struc
                    = (generic_descriptor_struc *) ptr_input_buffer;
            descriptor_tag = ptr_descriptor_struc->descriptor_tag_octet1;
            descriptor_length = ptr_descriptor_struc->descriptor_length_octet2;

            //**** account for the "N" (sizeof(generic_descriptor_struc) bytes/octet ****
            //**** that we just acquired/processed.                                  ****
            section_length -= sizeof(generic_descriptor_struc);
            ptr_input_buffer += sizeof(generic_descriptor_struc);
            es_info_length -= sizeof(generic_descriptor_struc);

            if (descriptor_length > es_info_length)
            { // error, descriptor is outside of the elementary stream block .
                pmt_error = 25;
                goto PARSING_DONE;
            }

            //**** Move the descriptor data into the SI database.             ****
            //**** Note: if descriptor_length is zero (0), the descriptor tag ****
            //**** and length will still be passed to the SIDB and zero bytes ****
            //**** will be written to the database.    
#ifdef ENABLE_INB_SI_CACHE
            if (si_program_handle != RMF_SI_INVALID_HANDLE)
            {
                m_pSiCache->LockForWrite();
                (void) m_pSiCache->SetDescriptor(si_program_handle, elementary_pid,
                        (rmf_SiElemStreamType)stream_type, (rmf_SiDescriptorTag)descriptor_tag, descriptor_length,
                        ptr_input_buffer);
                m_pSiCache->ReleaseWriteLock();
            }
#endif
            //**** prepare to process the next descriptor ****
            section_length -= descriptor_length;
            ptr_input_buffer += descriptor_length;
            es_info_length = es_info_length - descriptor_length;

            if ((es_info_length > 0) && (es_info_length
                    < sizeof(generic_descriptor_struc)))
            { // error, remaining octets/bytes can't hold a complete generic_descriptor_struc .
                pmt_error = 26;
                goto PARSING_DONE;
            }
        } //end of N2 Loop

    } //end_of N1 Loop

    //*************************************************************************************************
    //* At this point, the code either terminated prematurely or successfully processed the PMT.
    //* We need to validate the present of the CRC_32 field and cleanup before returning to the caller.
    //*************************************************************************************************
    RDK_LOG(RDK_LOG_TRACE3, "LOG.RDK.INBSI",
            "<%s: %s> - section_length = %d.\n", PSIMODULE, __FUNCTION__,
            section_length);

    if (section_length > LENGTH_CRC_32)
    {
        /*
         ** error, more data (pmt_table_struct3) remaining than just the CRC_32,
         ** see beginning of the "FOR N2 LOOp" above.
         */
        pmt_error = 27;
    }

    RDK_LOG(RDK_LOG_TRACE3, "LOG.RDK.INBSI",
            "<%s: %s> - ptr_input_buffer = 0x%p\n", PSIMODULE, __FUNCTION__,
            ptr_input_buffer);
    if (section_length == LENGTH_CRC_32)
    {
        crc = ((*ptr_input_buffer) << 24 | (*(ptr_input_buffer + 1)) << 16
                | (*(ptr_input_buffer + 2)) << 8 | (*(ptr_input_buffer + 3)));
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.INBSI",
                "<%s: %s> - CRC32 = 0x%08x\n", PSIMODULE, __FUNCTION__, crc);
    }

#ifdef ENABLE_INB_SI_CACHE
    if (si_program_handle != RMF_SI_INVALID_HANDLE)
    {
        m_pSiCache->LockForWrite();
        m_pSiCache->SetPMTCRC(si_program_handle, crc);
        m_pSiCache->ReleaseWriteLock();
    }
#endif

    PARSING_DONE:

#ifdef ENABLE_INB_SI_CACHE
    if (si_program_handle != RMF_SI_INVALID_HANDLE)
    {
        //mpe_SiServiceHandle service_handle;
        //publicSIIterator iter;

        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.INBSI",
                "<%s: %s> - pmt_error: %d\n", PSIMODULE, __FUNCTION__,
                pmt_error);

        if (pmt_error != 0)
        {
            m_pSiCache->LockForWrite();
            (void) m_pSiCache->SetPMTStatusNotAvailable(si_program_handle);
            m_pSiCache->ReleaseWriteLock();
            return pmt_error;
        }

        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.INBSI",
                "<%s: %s> - Calling mpe_siSetPMTStatus\n",
                PSIMODULE, __FUNCTION__);
        m_pSiCache->LockForWrite();
        m_pSiCache->SetPMTStatus(si_program_handle);
        m_pSiCache->ReleaseWriteLock();

            notifySIDB(TABLE_TYPE_PMT, sidbVersionNumber == INIT_TABLE_VERSION,
                    sidbVersionNumber != version_number,
                    sidbCrc32 != crc, si_program_handle);

    }
    //Print the pmt entries
#else
    if (pmt_error == 0)
    {
        if ( (pmtVersionNumber == INIT_TABLE_VERSION) ||
            (pmtVersionNumber != version_number)||
            (pmtCRC != crc))
        {
#if 0 //ericz, remove!
            printf("Dumping PMT\n");
            for(int ix = 0; ix < section_size; ix++)
            {
                printf("0x%x,  ", section_data[ix]);
            }
            printf("  End Dumping PMT\n");
#endif
            rmf_osal_mutexAcquire( m_mediainfo_mutex);
            if ( FALSE == descriptors_in_use)
            {
                release_descriptors( outer_descriptors);
                outer_descriptors = NULL;
                number_outer_desc = 0;
                release_es_list( elementary_streams );
                elementary_streams = NULL;
                number_elem_streams = 0;
#if 1
            // Parse the program element  outer descriptors.
                while (outer_desc_len)
                {
                    // Get the descriptor tag and length and point to the contents.
                    ptr_descriptor_struc = (generic_descriptor_struc *) ptr_outer_desc;
                    descriptor_tag = ptr_descriptor_struc->descriptor_tag_octet1;
                    descriptor_length = ptr_descriptor_struc->descriptor_length_octet2;
                    ptr_outer_desc += sizeof(generic_descriptor_struc);

                    RDK_LOG(RDK_LOG_TRACE3,
                            "LOG.RDK.INBSI",
                            "<%s: %s> - Call SetOuterDescriptor, setting outer tag: %d, length: %d.\n",
                            PSIMODULE, __FUNCTION__, descriptor_tag, descriptor_length);
                    // Set the descriptor in the SIDB.
                    SetOuterDescriptor((rmf_SiDescriptorTag) descriptor_tag,
                            (uint32_t) descriptor_length, (void *) ptr_outer_desc);

                    // Adjust the program info length for the tag, length and contents.
                    // Point the input buffer to the next descriptor.
                    outer_desc_len = (uint16_t)(outer_desc_len
                            - sizeof(generic_descriptor_struc));
                    outer_desc_len = (uint16_t)(outer_desc_len
                            - descriptor_length);
                    ptr_outer_desc = ptr_outer_desc + descriptor_length;
                }
                ptr_input_buffer = ptr_es_desc;
                section_length = es_desc_len;
                //parse and save ES descriptors
                for (; ((section_length - LENGTH_CRC_32) >= sizeof(pmt_table_struc3));)
                {
                    ptr_table_struc3 = (pmt_table_struc3 *) ptr_input_buffer;

                    //**** extract and validate the stream_type ****
                    stream_type = ptr_table_struc3->stream_type_octet1;

                    RDK_LOG(RDK_LOG_TRACE3, "LOG.RDK.INBSI",
                            "<%s: %s> - Stream_Type = x%2x.\n", PSIMODULE, __FUNCTION__,
                            stream_type);

                    if (!(((stream_type >= MIN_VALID_NONPRIVATE_STREAM_TYPE)
                            && (stream_type <= MAX_VALID_NONPRIVATE_STREAM_TYPE))
                            || ((stream_type >= MIN_VALID_PRIVATE_STREAM_TYPE)
                                    && (stream_type <= MAX_VALID_PRIVATE_STREAM_TYPE))))
                    {
                        RDK_LOG(RDK_LOG_WARN, "LOG.RDK.INBSI",
                                "<%s: %s> - Invalid Stream_Type = x%2x.\n",
                                PSIMODULE, __FUNCTION__, stream_type);
                    }

                    //**** validate 3 reserved bits ****
                    if (GETBITS(ptr_table_struc3->elementary_pid_octet2, 8, 6) != 0x07)
                    {
                        // bits 8,7 & 6 are not set to 1 per ISO/IEC 13818-1:2000(E),pg.4, para 2.1.46)
                        //pmt_error = 21;
                        //break ;
                        RDK_LOG(RDK_LOG_TRACE3,
                                "LOG.RDK.INBSI",
                                "<%s: %s> - *** WARNING *** 3-Bit-Reserved Field #5 not set to all ones(1)\n.",
                                PSIMODULE, __FUNCTION__);
                    }

                    //**** get elementary_pid (bits 5-1 in octet2 and 8-1 in octet3 ****
                    elementary_pid = (uint16_t)(((ptr_table_struc3->elementary_pid_octet2
                            & 0x1f) << 8) | ptr_table_struc3->elementary_pid_octet3);

                    RDK_LOG(RDK_LOG_TRACE3, "LOG.RDK.INBSI",
                            "<%s: %s> - Elementary_Pid= x%x.\n", PSIMODULE, __FUNCTION__,
                            elementary_pid);

                    //**** validate 4 reserved bits ****
                    if (GETBITS(ptr_table_struc3->es_info_length_octet4, 8, 5) != 0x0f)
                    {
                        // bits 8,7,6 & 5 are not set to 1 per ISO/IEC 13818-1:2000(E),pg.4, para 2.1.46)
                        //pmt_error = 22;
                        //break ;
                        RDK_LOG(RDK_LOG_TRACE3,
                                "LOG.RDK.INBSI",
                                "<%s: %s> - *** WARNING *** 4-Bit-Reserved Field #6 not set to all ones(1)\n.",
                                PSIMODULE, __FUNCTION__);
                    }

                    //**** extract and validate the 12 bit es_info_length field from octets 4 & 5 ****
                    //**** note, bits 12 and 11 must be set to zero,hence, max value is           ****
                    //**** (2**10) -1 = 1023.                                                     ****
                    es_info_length
                            = ((ptr_table_struc3->es_info_length_octet4 & 0x0f) << 8)
                                    | ptr_table_struc3->es_info_length_octet5;

                    RDK_LOG(RDK_LOG_TRACE3, "LOG.RDK.INBSI",
                            "<%s: %s> - es_info_length=%d.\n", PSIMODULE, __FUNCTION__,
                            es_info_length);

                    if (es_info_length > MAX_ES_INFO_LENGTH)
                    {
                        RDK_LOG(RDK_LOG_TRACE3,
                                "LOG.RDK.INBSI",
                                "<%s: %s> - *** WARNING *** bits 11 & 12 not 0\n",
                                PSIMODULE, __FUNCTION__);
                        /* removing reserve bits */
                        es_info_length = es_info_length & 0x03ff;
                    }

                    //**** update section_length and ptr to input buffer to account for the last ****
                    //**** sizeof(pmt_table_struc3) octets (i.e.,stream_type thru es_info_len)   ****
                    //**** and get the descriptor block associated with the elementary stream.   ****
                    section_length -= sizeof(pmt_table_struc3);
                    ptr_input_buffer += sizeof(pmt_table_struc3);
                   
                    if (es_info_length > section_length)
                    { // error, descriptor(s) are outside of the PMT .
                        RDK_LOG(RDK_LOG_TRACE3,
                                "LOG.RDK.INBSI",
                                "<%s: %s> - *** ERROR *** es_info_length(%d) > section_length(%d)\n",
                                PSIMODULE, __FUNCTION__, es_info_length, section_length);
                    }
                    SetElementaryStream( (rmf_SiElemStreamType)stream_type,
                        elementary_pid);
                    //*** RR added on 09-09-04 in support of org.ocap.si
                    SetESInfoLength( elementary_pid,
                        (rmf_SiElemStreamType)stream_type, es_info_length);

                    //*****************************************************************************************
                    //* This is the "FOR N2 Loop" in spec (ISO/IEC 13818-1:2000(E),pg.46, table 2-28)
                    //* Note: if es_info_length = 0, the N2 Loop won't be entered.Hence, no data associated
                    //* with the elementary_stream defined by elementary_pid (in pmt_table_struc3) will be
                    //* written to the SIDB.
                    //*****************************************************************************************
                    for (; es_info_length >= sizeof(generic_descriptor_struc);)
                    {
                        ptr_descriptor_struc
                                = (generic_descriptor_struc *) ptr_input_buffer;
                        descriptor_tag = ptr_descriptor_struc->descriptor_tag_octet1;
                        descriptor_length = ptr_descriptor_struc->descriptor_length_octet2;

                        //**** account for the "N" (sizeof(generic_descriptor_struc) bytes/octet ****
                        //**** that we just acquired/processed.                                  ****
                        section_length -= sizeof(generic_descriptor_struc);
                        ptr_input_buffer += sizeof(generic_descriptor_struc);
                        es_info_length -= sizeof(generic_descriptor_struc);

                        if (descriptor_length > es_info_length)
                        { // error, descriptor is outside of the elementary stream block .
                            break;
                        }

                        //**** Move the descriptor data into the SI database.             ****
                        //**** Note: if descriptor_length is zero (0), the descriptor tag ****
                        //**** and length will still be passed to the SIDB and zero bytes ****
                        //**** will be written to the database.    

                       SetDescriptor( elementary_pid,
                                (rmf_SiElemStreamType)stream_type, (rmf_SiDescriptorTag)descriptor_tag, descriptor_length,
                                ptr_input_buffer);
                        //**** prepare to process the next descriptor ****
                        section_length -= descriptor_length;
                        ptr_input_buffer += descriptor_length;
                        es_info_length = es_info_length - descriptor_length;

                        if ((es_info_length > 0) && (es_info_length
                                < sizeof(generic_descriptor_struc)))
                        { // error, remaining octets/bytes can't hold a complete generic_descriptor_struc .
                            break;
                        }
                    } //end of N2 Loop

                }
#endif
                RDK_LOG(RDK_LOG_INFO,
                        "LOG.RDK.INBSI",
                        "<%s: %s> - Notifying PMT old pmtVersionNumber=%d, version_number=%d pmtCRC=0x%x crc=0x%x\n",
                        PSIMODULE, __FUNCTION__, pmtVersionNumber, version_number, pmtCRC, crc);
                pcrPid = pcr_pid;

                notifySIDB(TABLE_TYPE_PMT, pmtVersionNumber == INIT_TABLE_VERSION,
                        pmtVersionNumber != version_number,
                        pmtCRC != crc);
                pmtCRC = crc;
                pmtVersionNumber = version_number;
                if(TRUE == m_NegativeFiltersEnabled)
                {
                    configure_negative_filters(TABLE_TYPE_PMT);
                }

#ifdef QAMSRC_PMTBUFFER_PROPERTY
                if ( section_size <= RMF_SI_PMT_MAX_SIZE )
                {
                    memcpy (pmtBuf, section_data, section_size);
                    pmtBufLen = section_size;
                }
                else
                {
                    RDK_LOG(RDK_LOG_ERROR,
                        "LOG.RDK.INBSI",
                        "<%s: %s> -section_size ( %d) greater than max size\n",
                        PSIMODULE, __FUNCTION__, section_size);
                }
#endif
            }
            rmf_osal_mutexRelease( m_mediainfo_mutex);
        }
    }
#endif
    RDK_LOG(RDK_LOG_TRACE1,
            "LOG.RDK.INBSI",
            "<%s: %s> - Exiting, Program# = %d  had ErrorCode = %d.\n",
            PSIMODULE, __FUNCTION__, program_number, pmt_error);

    return pmt_error;
}

/**
 * @brief This function gets the elementary streams and Mpeg2 descriptor list for the current frequency and modulation.
 * This information is fetched only if the SI cache is enabled. If the flag ENABLE_INB_SI_CACHE is not defined, then the
 * function will acquires the lock for the descriptors.
 *
 * @param[out] ppESList To be filled with the pointer to elementary streams list.
 * @param[out] ppMpeg2DescList To be filled with the pointer to Mpeg2 descriptor list.
 *
 * @return Returns Return value indicates success or failure of the call.
 * @retval RMF_SUCCESS Indicates the call is successful.
 * @retval RMF_INBSI_INVALID_PARAMETER Indicates NULL pointers were passed as parameters and call is not successfull.
 * @retval RMF_INBSI_PMT_NOT_AVAILABLE Indicates SI cache is disables and PMT is not available.
 */
rmf_Error rmf_InbSiMgr::GetDescriptors(rmf_SiElementaryStreamList ** ppESList, rmf_SiMpeg2DescriptorList** ppMpeg2DescList)
{
    rmf_Error ret = RMF_SUCCESS;
    if ( (NULL == ppESList ) || (NULL == ppMpeg2DescList))
    {
        RDK_LOG(
                RDK_LOG_ERROR,
                "LOG.RDK.INBSI",
                "<%s> Invalid Params\n", __FUNCTION__);
        return RMF_INBSI_INVALID_PARAMETER;
    }
#ifndef ENABLE_INB_SI_CACHE
    rmf_osal_mutexAcquire( m_mediainfo_mutex);
    *ppMpeg2DescList  = outer_descriptors;
    * ppESList = elementary_streams;
    descriptors_in_use = TRUE;
    rmf_osal_mutexRelease( m_mediainfo_mutex);
#else
    RDK_LOG(
                RDK_LOG_ERROR,
                "LOG.RDK.INBSI",
                "<%s> Not Implemented\n", __FUNCTION__);
    ret = RMF_INBSI_PMT_NOT_AVAILABLE;
#endif

    return ret;
}

/**
 * @brief This function is used to release the descriptor only when the flag ENABLE_INB_SI_CACHE
 * is not defined. The descriptors_in_use acts like a lock. When its TRUE it means the lock is
 * acquired and no modifications can be made to the descriptors and FALSE implies the lock is released.
 *
 * @return None
 */
void rmf_InbSiMgr::ReleaseDescriptors()
{
#ifndef ENABLE_INB_SI_CACHE
    rmf_osal_mutexAcquire( m_mediainfo_mutex);
    descriptors_in_use = FALSE;
    rmf_osal_mutexRelease( m_mediainfo_mutex);
#else
    RDK_LOG(
                RDK_LOG_ERROR,
                "LOG.RDK.INBSI",
                "<%s> Not Implemented\n", __FUNCTION__);
#endif
}

#ifndef ENABLE_INB_SI_CACHE

/**
 *  PMT
 *
 *  Set the es info length for the given elementary stream
 *
 * <i>SetESInfoLength()</i>
 *
 * @param service_handle is the service handle to set the descriptor for
 * @param program_info_length is the length of es info length
 *
 * @return RMF_INBSI_SUCCESS if successful, else
 *          return appropriate rmf_Error
 *
 */
void rmf_InbSiMgr::SetESInfoLength(uint32_t elem_pid, rmf_SiElemStreamType type, uint32_t es_info_length)
{
    rmf_SiElementaryStreamList *elem_list = NULL;

    /* Implement populating elementary stream info */
    elem_list = elementary_streams;
    while (elem_list)
    {
        if ((elem_list->elementary_PID == elem_pid)
                && (elem_list->elementary_stream_type == type))
        {
            elem_list->es_info_length = es_info_length;
            break;
        }
        elem_list = elem_list->next;
    }
}

/**
 *  PMT
 *
 *  Set the elementary stream info for the given program entry
 *
 * <i>SetElementaryStream()</i>
 *
 * @param program_handle is the program handle to set the elementary stream info for
 * @param stream_type is the elementary stream type
 * @param elem_pid is the elementary stream pid
 *
 * @return RMF_INBSI_SUCCESS if successful, else
 *          return appropriate rmf_Error
 *
 */
rmf_Error rmf_InbSiMgr::SetElementaryStream( rmf_SiElemStreamType stream_type, uint32_t elem_pid)
{
    rmf_SiElementaryStreamList *new_elem_stream = NULL;
    rmf_SiServiceComponentType service_comp_type;
    //LINK *lp = NULL;

    /* Parameter check */
    if (elem_pid == 0)
    {
        return RMF_INBSI_INVALID_PARAMETER;
    }

    if (RMF_INBSI_SUCCESS != rmf_osal_memAllocP(RMF_OSAL_MEM_SI_INB,
            sizeof(rmf_SiElementaryStreamList), (void **) &(new_elem_stream)))
    {
        RDK_LOG(
                RDK_LOG_WARN,
                "LOG.RDK.INBSI",
                "<%s> Mem allocation failed, returning RMF_INBSI_OUT_OF_MEM...\n", __FUNCTION__);
        return RMF_INBSI_OUT_OF_MEM;
    }

    new_elem_stream->elementary_PID = elem_pid;
    new_elem_stream->elementary_stream_type = stream_type;
    new_elem_stream->next = NULL;
    new_elem_stream->number_desc = 0;
    new_elem_stream->ref_count = 1;
    new_elem_stream->valid = TRUE;

    new_elem_stream->descriptors = NULL;
    new_elem_stream->service_component_names = NULL;
    new_elem_stream->es_info_length = 0;
    rmf_osal_timeGetMillis(&(new_elem_stream->ptime_service_component));

    RDK_LOG(
            RDK_LOG_TRACE1,
            "LOG.RDK.INBSI",
            "<%s> type:0x%x, pid: 0x%x new_elem_stream:0x%p\n",
            __FUNCTION__, stream_type, elem_pid, new_elem_stream);

    /* Populate associated language after reading descriptors (language_descriptor) */
    /* For now set it to null */
    memset(new_elem_stream->associated_language, 0, 4);

    /* Based on the elementary stream type set the service component type */
    get_serviceComponentType(stream_type, &service_comp_type);

    new_elem_stream->service_comp_type = service_comp_type;

    /* Populating elementary stream info */
    append_elementaryStream( new_elem_stream);

    //RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.INBSI", "<mpe_siSetElementaryStream> created 0x%x\n", new_elem_stream);
    
    return RMF_INBSI_SUCCESS;
}


/*
 Internal method to append elementary stream to the end of existing
 elementary stream list within a service
 */
void rmf_InbSiMgr::append_elementaryStream( rmf_SiElementaryStreamList *new_elem_stream)
{
    rmf_SiElementaryStreamList *elem_list;

    /* Populating elementary stream info */
    elem_list = elementary_streams;

    if (elem_list == NULL)
    {
        elementary_streams = new_elem_stream;
    }
    else
    {
        while (elem_list)
        {
            if (elem_list->next == NULL)
            {
                elem_list->next = new_elem_stream;
                break;
            }
            elem_list = elem_list->next;
        }
    }
    number_elem_streams++;
}


/*
 Internal method to get the service component type based on the elementary
 stream type.
 Service component types are generic types (ex: AUDIO, VIDEO, DATA etc.)
 Elementary stream types are specific (ex: MPEG1_AUDIO, MPEG2_VIDEO, RMF_SI_ELEM_DSM_CC_SECTIONS etc.)
 Hence if the elementary stream type is known the corresponding service
 component type id derived from it.
 */
void rmf_InbSiMgr::get_serviceComponentType(rmf_SiElemStreamType stream_type,
        rmf_SiServiceComponentType *comp_type)
{

    switch (stream_type)
    {
    case RMF_SI_ELEM_MPEG_1_VIDEO:
    case RMF_SI_ELEM_MPEG_2_VIDEO:
    case RMF_SI_ELEM_VIDEO_DCII:
    case RMF_SI_ELEM_AVC_VIDEO:

        *comp_type = RMF_SI_COMP_TYPE_VIDEO;
        break;

    case RMF_SI_ELEM_MPEG_1_AUDIO:
    case RMF_SI_ELEM_MPEG_2_AUDIO:
    case RMF_SI_ELEM_ATSC_AUDIO:
    case RMF_SI_ELEM_ENHANCED_ATSC_AUDIO:
    case RMF_SI_ELEM_AAC_ADTS_AUDIO:
    case RMF_SI_ELEM_AAC_AUDIO_LATM:

        *comp_type = RMF_SI_COMP_TYPE_AUDIO;
        break;

    case RMF_SI_ELEM_STD_SUBTITLE:

        *comp_type = RMF_SI_COMP_TYPE_SUBTITLES;
        break;

        /* Not sure about these?? */
    case RMF_SI_ELEM_DSM_CC_STREAM_DESCRIPTORS:
    case RMF_SI_ELEM_DSM_CC_UN:
    case RMF_SI_ELEM_MPEG_PRIVATE_DATA:
    case RMF_SI_ELEM_DSM_CC:
    case RMF_SI_ELEM_DSM_CC_MPE:
    case RMF_SI_ELEM_DSM_CC_SECTIONS:
    case RMF_SI_ELEM_METADATA_PES:

        *comp_type = RMF_SI_COMP_TYPE_DATA;
        break;

    case RMF_SI_ELEM_MPEG_PRIVATE_SECTION:
    case RMF_SI_ELEM_SYNCHRONIZED_DOWNLOAD:
    case RMF_SI_ELEM_METADATA_SECTIONS:
    case RMF_SI_ELEM_METADATA_DATA_CAROUSEL:
    case RMF_SI_ELEM_METADATA_OBJECT_CAROUSEL:
    case RMF_SI_ELEM_METADATA_SYNCH_DOWNLOAD:

        *comp_type = RMF_SI_COMP_TYPE_SECTIONS;
        break;

    default:
        *comp_type = RMF_SI_COMP_TYPE_UNKNOWN;
        break;
    }

}


/**
 *  PMT
 *
 *  Set the outer descriptor for the given program entry
 *
 * <i>SetOuterDescriptor()</i>
 *
 * @param program_handle is the program handle to set the descriptor for
 * @param tag is the descriptor tag field
 * @param length is the length of the descriptor
 * @param content is descriptor content
 *
 * @return RMF_INBSI_SUCCESS if successful, else
 *          return appropriate mpe_Error
 *
 */
rmf_Error rmf_InbSiMgr::SetOuterDescriptor(    rmf_SiDescriptorTag tag, 
                    uint32_t  length, void *content)
{
    rmf_SiMpeg2DescriptorList *desc_list = NULL;
    rmf_SiMpeg2DescriptorList *new_desc = NULL;

    /* Store PMT outer descriptors in raw format. */
    /* Append descriptor at the end of existing list */
    if (RMF_INBSI_SUCCESS != rmf_osal_memAllocP(RMF_OSAL_MEM_SI_INB,
            sizeof(rmf_SiMpeg2DescriptorList), (void **) &(new_desc)))
    {
        RDK_LOG(
                RDK_LOG_WARN,
                "LOG.RDK.INBSI",
                "<%s> Mem allocation failed, returning RMF_SI_OUT_OF_MEM...\n",__FUNCTION__);
        return RMF_INBSI_OUT_OF_MEM;
    }
    new_desc->tag = tag;
    new_desc->descriptor_length = (unsigned char) length;
    if (length > 0)
    {
        if (RMF_INBSI_SUCCESS != rmf_osal_memAllocP(RMF_OSAL_MEM_SI_INB, (sizeof(uint8_t)
                * length), (void **) &(new_desc->descriptor_content)))
        {
            rmf_osal_memFreeP(RMF_OSAL_MEM_SI_INB, new_desc);
            RDK_LOG(RDK_LOG_WARN, "LOG.RDK.INBSI",
                    "<%s> MEM Allocation 2 failed...\n", __FUNCTION__);
            return RMF_INBSI_OUT_OF_MEM;
        }
        memcpy(new_desc->descriptor_content, content, length);
    }
    else
    {
        new_desc->descriptor_content = NULL;
    }
    new_desc->next = NULL;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.INBSI",
            "<%s> tag:%d length:%d new_desc:0x%p\n", __FUNCTION__, tag,
            length, new_desc);
    if(RMF_SI_DESC_CA_DESCRIPTOR == tag)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.INBSI", "<%s> ca descriptor found. length:%d \n", __FUNCTION__, length);
    }
    desc_list = outer_descriptors;

    if (desc_list == NULL)
    {
        outer_descriptors = new_desc;
    }
    else
    {
        while (desc_list->next != NULL)
        {
            desc_list = desc_list->next;
        }
        desc_list->next = new_desc;
    }
    number_outer_desc++;

    return RMF_INBSI_SUCCESS;
}


rmf_Error rmf_InbSiMgr::SetDescriptor(  uint32_t elem_pid, 
    rmf_SiElemStreamType type, rmf_SiDescriptorTag tag,
    uint32_t length, void *content)
{
    rmf_SiMpeg2DescriptorList *desc_list = NULL;
    rmf_SiMpeg2DescriptorList *new_desc = NULL;
    rmf_SiElementaryStreamList *elem_list = NULL;
    int done = 0;

    /* Populate associated language from language_descriptor */

    /* Populate component name from component_name_descriptor */

    /* Implement populating elementary stream info */
    elem_list = elementary_streams;
    while (elem_list && !done)
    {
        if ((elem_list->elementary_PID == elem_pid)
                && (elem_list->elementary_stream_type == type))
        {
            /* If the descriptor type is language set the associated
             language field of the elementary stream from descriptor
             content
             */
            if (tag == RMF_SI_DESC_ISO_639_LANGUAGE)
            {
                RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.INBSI", "<mpe_siSetDescriptor> language descriptor found...\n");
                /* associated language field id 4 bytes */
                if (length > 0)
                {
                    memset(elem_list->associated_language, 0, 4);
                    memcpy(elem_list->associated_language, (void*) content, 4);
                }
            }
            /* If the descriptor type is component descriptor
             set the component name field of the elementary stream
             from descriptor content
             */
            else if (tag == RMF_SI_DESC_COMPONENT_NAME) // Multiple String Structure
            {
                int i;
                uint8_t numStrings;
                uint8_t* mss_ptr = (uint8_t*) content;
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.INBSI", "<mpe_siSetDescriptor> component name descriptor found...\n");

                // Free any previously specified component names
                if (elem_list->service_component_names != NULL)
                {
                    rmf_SiLangSpecificStringList *walker =
                            elem_list->service_component_names;

                    while (walker != NULL)
                    {
                        rmf_SiLangSpecificStringList *toBeFreed = walker;
                        rmf_osal_memFreeP(RMF_OSAL_MEM_SI_INB, walker->string);
                        walker = walker->next;
                        rmf_osal_memFreeP(RMF_OSAL_MEM_SI_INB, toBeFreed);
                    }

                    elem_list->service_component_names = NULL;
                }

                // MSS -- Number of strings
                numStrings = *mss_ptr;
                mss_ptr += sizeof(uint8_t);

                // MSS -- String loop
                for (i = 0; i < numStrings; ++i)
                {
                    int j;
                    uint8_t numSegments;
                    char lang[4];
                    char mssString[1024];

                    mssString[0] = '\0';

                    // MSS -- ISO639 Language Code
                    lang[0] = *((char*) mss_ptr);
                    mss_ptr += sizeof(char);
                    lang[1] = *((char*) mss_ptr);
                    mss_ptr += sizeof(char);
                    lang[2] = *((char*) mss_ptr);
                    mss_ptr += sizeof(char);
                    lang[3] = '\0';

                    // MSS -- Number of segments
                    numSegments = *((uint8_t*) mss_ptr);
                    mss_ptr += sizeof(uint8_t);

                    // MSS -- Segment loop
                    for (j = 0; j < numSegments; ++j)
                    {
                        uint8_t compressionType;
                        uint8_t mode;
                        uint8_t numBytes;

                        // MSS -- Segment compression type
                        compressionType = *((uint8_t*) mss_ptr);
                        mss_ptr += sizeof(uint8_t);

                        // MSS -- Segment mode
                        mode = *((uint8_t*) mss_ptr);
                        mss_ptr += sizeof(uint8_t);

                        // ONLY Mode 0 and CompressionType 0 supported for now
                        if (compressionType != 0 || mode != 0)
                        {
                            RDK_LOG(
                                    RDK_LOG_ERROR,
                                    "LOG.RDK.INBSI",
                                    "<%s> Invalid MSS segment mode or compression type!  Mode = %d, Compression = %d\n",
                                    __FUNCTION__, mode, compressionType);
                            break;
                        }

                        // MSS -- Number of segment bytes
                        numBytes = *((uint8_t*) mss_ptr);
                        mss_ptr += sizeof(uint8_t);

                        // Append this segment onto our string
                        strncat(mssString, (const char*) mss_ptr, numBytes);
                    }

                    // Add this new string to our list
                    if (strlen(mssString) > 0)
                    {
                        rmf_SiLangSpecificStringList* new_entry = NULL;

                        // Allocate a new lang-specific string
                        if (RMF_INBSI_SUCCESS != rmf_osal_memAllocP(RMF_OSAL_MEM_SI_INB,
                                sizeof(struct rmf_SiLangSpecificStringList),
                                (void **) &new_entry))
                        {
                            RDK_LOG(RDK_LOG_WARN, "LOG.RDK.INBSI",
                                    "<%s> Mem allocation failed, returning RMF_INBSI_OUT_OF_MEM...\n",__FUNCTION__);
                            return RMF_INBSI_OUT_OF_MEM;
                        }
                        if (RMF_INBSI_SUCCESS != rmf_osal_memAllocP(RMF_OSAL_MEM_SI_INB,
                                strlen(mssString) + 1,
                                (void **) &(new_entry->string)))
                        {
                            rmf_osal_memFreeP(RMF_OSAL_MEM_SI_INB, new_entry);
                            RDK_LOG(RDK_LOG_WARN, "LOG.RDK.INBSI",
                                    "<%s> Mem allocation failed, returning RMF_INBSI_OUT_OF_MEM...\n",__FUNCTION__);
                            return RMF_INBSI_OUT_OF_MEM;
                        }
                        strcpy(new_entry->string, mssString);
                        strncpy(new_entry->language, lang, 4);
                        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.INBSI", "<mpe_siSetDescriptor> name: %s, lang: %s\n", new_entry->string, new_entry->language);

                        // Add new list entry to the front of list for this service component
                        new_entry->next = elem_list->service_component_names;
                        elem_list->service_component_names = new_entry;
                    }
                }
            }

            /*
             Even for the above two descriptor types that are parsed out, store them in raw format
             also to support org.ocap.si package.
             */
            /* Append descriptor at the end of existing list */
            if (RMF_INBSI_SUCCESS != rmf_osal_memAllocP(RMF_OSAL_MEM_SI_INB,
                    sizeof(rmf_SiMpeg2DescriptorList), (void **) &(new_desc)))
            {
                RDK_LOG(RDK_LOG_WARN, "LOG.RDK.INBSI",
                        "<%s> Mem allocation failed, returning RMF_INBSI_OUT_OF_MEM...\n",__FUNCTION__);
                return RMF_INBSI_OUT_OF_MEM;
            }
            new_desc->tag = tag;
            new_desc->descriptor_length = (uint8_t) length;
            //RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.INBSI", "<mpe_siSetDescriptor> new_desc->tag: 0x%x, tag: 0x%x, length: 0x%x, new_descriptor_length: 0x%x\n",
            //      new_desc->tag, tag, length, new_desc->descriptor_length);
            if (length > 0)
            {
                if (RMF_SUCCESS != rmf_osal_memAllocP(RMF_OSAL_MEM_SI_INB, (sizeof(uint8_t)
                        * length), (void **) &(new_desc->descriptor_content)))
                {
                    rmf_osal_memFreeP(RMF_OSAL_MEM_SI_INB, new_desc);
                    return RMF_INBSI_OUT_OF_MEM;
                }
                memcpy(new_desc->descriptor_content, content, length);
            }
            else
            {
                new_desc->descriptor_content = NULL;
            }
            new_desc->next = NULL;

            //RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.INBSI", "<mpe_siSetDescriptor> created new decriptor 0x%x for elem_list 0x%x \n", new_desc, elem_list);
            {
                uint32_t i = 0;
                for (i = 0; i < length; i++)
                    RDK_LOG(
                            RDK_LOG_TRACE1,
                            "LOG.RDK.INBSI",
                            "<%s> descriptor_content: 0x%x...\n", __FUNCTION__, 
                            *((uint8_t*) new_desc->descriptor_content + i));
            }

            desc_list = elem_list->descriptors;
            if (desc_list == NULL)
            {
                elem_list->descriptors = new_desc;
                elem_list->number_desc++;
                break;
            }
            else
            {
                while (desc_list)
                {
                    if (desc_list->next == NULL)
                    {
                        desc_list->next = new_desc;
                        elem_list->number_desc++;
                        done = 1;
                        break;
                    }
                    desc_list = desc_list->next;
                }
            }
        }

        elem_list = elem_list->next;
    }

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.INBSI",
            "<%s> tag:0x%x length: %d new_desc:0x%p\n", __FUNCTION__, tag,
            length, new_desc);
    if(RMF_SI_DESC_CA_DESCRIPTOR == tag)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.INBSI", "<%s> ca descriptor found. length:%d \n", __FUNCTION__, length);
    }
    return RMF_INBSI_SUCCESS;
}

void rmf_InbSiMgr::delete_elementary_stream(rmf_SiElementaryStreamList *elem_stream)
{
    /* Parameter check */
    if (elem_stream == NULL)
    {
        return;
    }

    // De-allocate service component name
    if (elem_stream->service_component_names != NULL)
    {
        rmf_SiLangSpecificStringList *walker =
                elem_stream->service_component_names;

        while (walker != NULL)
        {
            rmf_SiLangSpecificStringList *toBeFreed = walker;

            rmf_osal_memFreeP(RMF_OSAL_MEM_SI_INB, walker->string);

            walker = walker->next;

            rmf_osal_memFreeP(RMF_OSAL_MEM_SI_INB, toBeFreed);
        }

        elem_stream->service_component_names = NULL;
    }

    // De-allocate descriptors
    if (elem_stream->descriptors != NULL)
    {
        release_descriptors(elem_stream->descriptors);
        elem_stream->descriptors = NULL;
    }
    elem_stream->number_desc = 0;

    rmf_osal_memFreeP(RMF_OSAL_MEM_SI_INB, elem_stream);
}

void rmf_InbSiMgr::release_descriptors(rmf_SiMpeg2DescriptorList *desc)
{
    rmf_SiMpeg2DescriptorList *tempDescPtr = desc;
    rmf_SiMpeg2DescriptorList *tempDescNextPtr = NULL;

    while (tempDescPtr != NULL)
    {
        tempDescPtr->tag = (rmf_SiDescriptorTag)0;
        tempDescPtr->descriptor_length = 0;

        //RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.INBSI", "<release_descriptors> deleting descriptor 0x%x\n", tempDescPtr);

        if (tempDescPtr->descriptor_content)
        {
            rmf_osal_memFreeP(RMF_OSAL_MEM_SI_INB, tempDescPtr->descriptor_content);
            tempDescPtr->descriptor_content = NULL;
        }
        tempDescNextPtr = tempDescPtr->next;
        rmf_osal_memFreeP(RMF_OSAL_MEM_SI_INB, tempDescPtr);
        tempDescPtr = tempDescNextPtr;
    }
}

void rmf_InbSiMgr::release_es_list( rmf_SiElementaryStreamList *es_list)
{
    rmf_SiElementaryStreamList* tempESDescPtr = es_list;
    rmf_SiElementaryStreamList* tempESDescNextPtr = NULL;
    while (tempESDescPtr != NULL)
    {
        tempESDescNextPtr = tempESDescPtr->next;
        delete_elementary_stream(tempESDescPtr);
        tempESDescPtr = tempESDescNextPtr;
    }
}

#endif

/*
 ** Notify SIDB base on the versioning and crc values of the table acquired.
 **
 ** If the version is different then the current version stored in the SIDB, and the current version
 ** in the SIDB is the default version call table acquired.
 **
 ** If the version is different then the current version stored in the SIDB, and the current version
 ** is not the default version call table updated.
 **
 ** If the version is the same and the crc32 value is different, then call table updated.
 **
 ** If the version is the same and the crc32 is the same - then error - should parse a table that is
 ** already acquired.
 **
 */
#ifdef ENABLE_INB_SI_CACHE
void rmf_InbSiMgr::notifySIDB(rmf_psi_table_type_e table_type,
        rmf_osal_Bool init_version, rmf_osal_Bool new_version, rmf_osal_Bool new_crc,
        rmf_SiGenericHandle si_handle)
#else
void rmf_InbSiMgr::notifySIDB(rmf_psi_table_type_e table_type, rmf_osal_Bool init_version, 
        rmf_osal_Bool new_version, rmf_osal_Bool new_crc)
#endif
{
    RDK_LOG(RDK_LOG_TRACE1,
            "LOG.RDK.INBSI",
            "<%s: %s> Enter - table_type: %d, init_version=%s, new_version=%s, new_crc=%s\n",
            PSIMODULE, __FUNCTION__, table_type, (init_version == true ? "true" : "false"),
            (new_version == true ? "true" : "false"), (new_crc == true ? "true" : "false"));

    if (true == new_version)
    {
        if (true == init_version)
        {
            /*
             ** SIDB doesn't have this table, so tell sidb that the table has
             ** been acquired and set the version and crc.
             */
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.INBSI",
                    "<%s: %s> - tabletype 0x%x - RMF_SI_CHANGE_TYPE_ADD\n",
                    PSIMODULE, __FUNCTION__, table_type);
#ifdef ENABLE_INB_SI_CACHE
            NotifyTableChanged(table_type, RMF_SI_CHANGE_TYPE_ADD, si_handle);
#else
            NotifyTableChanged(table_type, RMF_SI_CHANGE_TYPE_ADD, 0);
#endif

        }
        else
        {
            /*
             ** SIDB doesn't have this table, so tell sidb that the table has
             ** been acquired and set the version and crc.
             */
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.INBSI",
                    "<%s: %s> - tabletype 0x%x - RMF_SI_CHANGE_TYPE_MODIFY\n",
                    PSIMODULE, __FUNCTION__, table_type);
#ifdef ENABLE_INB_SI_CACHE
            NotifyTableChanged(table_type, RMF_SI_CHANGE_TYPE_MODIFY,
                    si_handle);
#else
            NotifyTableChanged(table_type, RMF_SI_CHANGE_TYPE_MODIFY,
                    0);
#endif
        }
    }
    else
    {
        /*
         ** Assert: Version has lapped itself
         */
        if (true == new_crc)
        {
            RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.INBSI",
                    "<%s: %s> - call mpe_siNotifyTableChanged\n",
                    PSIMODULE, __FUNCTION__);
#ifdef ENABLE_INB_SI_CACHE
            NotifyTableChanged(table_type, RMF_SI_CHANGE_TYPE_MODIFY,
                    si_handle);
#else
            NotifyTableChanged(table_type, RMF_SI_CHANGE_TYPE_MODIFY,
                    0);
#endif
        }
    }
}

/*
 Called from SITP when a table is acquired/updated. SI DB uses it to
 deliver events to registered callers.
 */
rmf_Error rmf_InbSiMgr::NotifyTableChanged(rmf_psi_table_type_e table_type,
        uint32_t changeType, uint32_t optional_data)
{
    uint32_t event = 0;
#ifdef ENABLE_INB_SI_CACHE
    uint32_t eventFrequency = 0;
    uint32_t eventProgramNumber = 0;
#endif

    uint32_t optionalEventData3 = changeType;

    if (table_type == 0) /* unknown table type */
    {
        return RMF_INBSI_INVALID_PARAMETER;
    }

    RDK_LOG(
            RDK_LOG_TRACE1,
            "LOG.RDK.INBSI",
            "<%s: %s> called...with tableType: %d, changeType: %d optional_data: 0x%x\n",
            PSIMODULE, __FUNCTION__, table_type, changeType, optional_data);

    switch (table_type)
    {
    case TABLE_TYPE_TDT:
    {
        event = RMF_SI_EVENT_IB_TDT_ACQUIRED;
        break;
    }
    case TABLE_TYPE_TOT:
    {
        event = RMF_SI_EVENT_IB_TOT_ACQUIRED;
        break;
    }
    case TABLE_TYPE_PAT:
    {
#ifdef ENABLE_INB_SI_CACHE
        rmf_SiTransportStreamEntry *ts =
                (rmf_SiTransportStreamEntry*) optional_data;
        if (ts != NULL)
#endif
        {
#ifdef ENABLE_INB_SI_CACHE
            eventFrequency = ts->frequency;
            RDK_LOG(
                    RDK_LOG_TRACE1,
                    "LOG.RDK.INBSI",
                    "<%s: %s>: IB_PAT event, ts_handle: 0x%p\n",PSIMODULE, __FUNCTION__, ts);
#endif

            if (changeType == RMF_SI_CHANGE_TYPE_ADD)
            {
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.INBSI",
                        "<%s: %s> Sending PAT_ACQUIRED event\n", PSIMODULE, __FUNCTION__);
                event = RMF_SI_EVENT_IB_PAT_ACQUIRED;
            }
            else if (changeType == RMF_SI_CHANGE_TYPE_MODIFY)
            {
                RDK_LOG(
                        RDK_LOG_INFO,
                        "LOG.RDK.INBSI",
                        "<%s: %s>: Sending RMF_SI_EVENT_IB_PAT_UPDATE event, changeType: RMF_SI_CHANGE_TYPE_MODIFY\n",
                        PSIMODULE, __FUNCTION__);
                event = RMF_SI_EVENT_IB_PAT_UPDATE;
            }
            else if (changeType == RMF_SI_CHANGE_TYPE_REMOVE)
            {
                RDK_LOG(
                        RDK_LOG_INFO,
                        "LOG.RDK.INBSI",
                        "<%s: %s>: Sending RMF_SI_EVENT_IB_PAT_UPDATE event, changeType: RMF_SI_CHANGE_TYPE_REMOVE\n",PSIMODULE, __FUNCTION__);
                event = RMF_SI_EVENT_IB_PAT_UPDATE; //??
            }
        }
#ifdef ENABLE_INB_SI_CACHE
        else
        {
            RDK_LOG(
                    RDK_LOG_ERROR,
                    "LOG.RDK.INBSI",
                    "<%s: %s> NULL transport_stream handle passed for RMF_SI_EVENT_IB_PAT_UPDATE event!\n",
                    PSIMODULE, __FUNCTION__);
            event = 0;
        }
#endif
    }
    break;

    case TABLE_TYPE_PMT:
    {
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.INBSI",
                "<%s: %s>: IB_PMT event\n", PSIMODULE, __FUNCTION__);

        if (changeType == RMF_SI_CHANGE_TYPE_ADD)
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.INBSI",
                    "<%s: %s>: Sending PMT_ACQUIRED event\n", PSIMODULE, __FUNCTION__);
            event = RMF_SI_EVENT_IB_PMT_ACQUIRED;
        }
        else if (changeType == RMF_SI_CHANGE_TYPE_MODIFY)
        {
            RDK_LOG(
                    RDK_LOG_INFO,
                    "LOG.RDK.INBSI",
                    "<%s: %s>: Sending RMF_SI_EVENT_IB_PMT_UPDATE event, changeType: RMF_SI_CHANGE_TYPE_MODIFY\n", PSIMODULE, __FUNCTION__);
            event = RMF_SI_EVENT_IB_PMT_UPDATE;
        }
        else if (changeType == RMF_SI_CHANGE_TYPE_REMOVE)
        {
            RDK_LOG(
                    RDK_LOG_INFO,
                    "LOG.RDK.INBSI",
                    "<%s: %s>: Sending RMF_SI_EVENT_IB_PMT_UPDATE event, changeType: RMF_SI_CHANGE_TYPE_REMOVE\n", PSIMODULE, __FUNCTION__);
            event = RMF_SI_EVENT_IB_PMT_UPDATE;
        }
        // Get the frequency and program number associated with the serviceHandle
        // provided by SITP
        {
#ifdef ENABLE_INB_SI_CACHE
            rmf_SiProgramInfo *program_info = (rmf_SiProgramInfo*)optional_data;

            if(program_info != NULL)
            {
                eventFrequency = program_info->stream->frequency;
                eventProgramNumber = program_info->program_number;
            }
#endif
        }
    }
    break;

#if 0
    case IB_CVCT:
    {
        mpe_Error err;
        mpe_SiServiceHandle serviceHandle = (mpe_SiServiceHandle) optional_data;

        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.INBSI",
                "<mpe_siNotifyTableUpdated> Sending RMF_SI_EVENT_IB_CVCT_UPDATE event\n");
        if (changeType == RMF_SI_CHANGE_TYPE_ADD)
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.INBSI",
                    "<mpe_siNotifyTableChanged> Sending RMF_SI_EVENT_IB_CVCT_ACQUIRED event\n");
            event = RMF_SI_EVENT_IB_CVCT_ACQUIRED;
        }
        else if (changeType == RMF_SI_CHANGE_TYPE_MODIFY)
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.INBSI",
                    "<mpe_siNotifyTableChanged> Sending RMF_SI_EVENT_IB_CVCT_UPDATE event\n");
            event = RMF_SI_EVENT_IB_CVCT_UPDATE;
        }
        else if (changeType == RMF_SI_CHANGE_TYPE_REMOVE)
        {
            event = RMF_SI_EVENT_IB_CVCT_UPDATE;
        }

        // Get the frequency and program number associated with the serviceHandle
        // provided by SITP
        err
                = mpe_siGetFrequencyForServiceHandle(serviceHandle,
                        &eventFrequency);
        if (err != RMF_SUCCESS)
        {
            RDK_LOG(
                    RDK_LOG_DEBUG,
                    "LOG.RDK.INBSI",
                    "<mpe_siNotifyTableChanged> Could not get frequency!  returned: 0x%x\n",
                    err);
            event = 0;
        }

    }
    break;
#endif

#ifdef CAT_FUNCTIONALITY
    case TABLE_TYPE_CAT:
    {
#ifdef ENABLE_INB_SI_CACHE
        rmf_SiTransportStreamEntry *ts = (rmf_SiTransportStreamEntry*) optional_data;
        if (ts != NULL)
#endif
        {
#ifdef ENABLE_INB_SI_CACHE
            eventFrequency = ts->frequency;
            RDK_LOG(
                    RDK_LOG_TRACE1,
                    "LOG.RDK.INBSI",
                    "<%s: %s>: IB_CAT event, ts_handle: 0x%p\n",PSIMODULE, __FUNCTION__, ts);
#endif

            if (changeType == RMF_SI_CHANGE_TYPE_ADD)
            {
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.INBSI",
                        "<%s: %s> Sending CAT_ACQUIRED event\n", PSIMODULE, __FUNCTION__);
                event = RMF_SI_EVENT_IB_CAT_ACQUIRED;
            }
            else if (changeType == RMF_SI_CHANGE_TYPE_MODIFY)
            {
                RDK_LOG(
                        RDK_LOG_INFO,
                        "LOG.RDK.INBSI",
                        "<%s: %s>: Sending RMF_SI_EVENT_IB_CAT_UPDATE event, changeType: RMF_SI_CHANGE_TYPE_MODIFY\n",
                        PSIMODULE, __FUNCTION__);
                event = RMF_SI_EVENT_IB_CAT_UPDATE;
            }
            else if (changeType == RMF_SI_CHANGE_TYPE_REMOVE)
            {
                RDK_LOG(
                        RDK_LOG_INFO,
                        "LOG.RDK.INBSI",
                        "<%s: %s>: Sending RMF_SI_EVENT_IB_CAT_UPDATE event, changeType: RMF_SI_CHANGE_TYPE_REMOVE\n",PSIMODULE, __FUNCTION__);
                event = RMF_SI_EVENT_IB_CAT_UPDATE; //??
            }
        }
#ifdef ENABLE_INB_SI_CACHE
        else
        {
            RDK_LOG(
                    RDK_LOG_ERROR,
                    "LOG.RDK.INBSI",
                    "<%s: %s> NULL transport_stream handle passed for RMF_SI_EVENT_IB_CAT_UPDATE event!\n",
                    PSIMODULE, __FUNCTION__);
            event = 0;
        }
#endif

    }
    break;
#endif

    default:
        event = 0;
        break;
    }

    if (event == 0)
        return RMF_INBSI_SUCCESS;

#ifdef ENABLE_INB_SI_CACHE
    dispatch_event(event, eventFrequency, eventProgramNumber, optional_data, optionalEventData3);
#else
    dispatch_event(event,   optionalEventData3);
#endif

    return RMF_INBSI_SUCCESS;
}

#ifdef ENABLE_INB_SI_CACHE
rmf_Error rmf_InbSiMgr::dispatch_event(uint32_t event, uint32_t eventFrequency, uint16_t eventProgramNumber, rmf_SiGenericHandle si_handle, uint32_t optionalEventData)
#else
rmf_Error rmf_InbSiMgr::dispatch_event(uint32_t event, uint32_t optionalEventData)
#endif
{
    rmf_osal_event_params_t event_params = {0};
    rmf_osal_event_handle_t event_handle;
    rmf_Error ret = RMF_INBSI_SUCCESS;
    rmf_InbSiRegistrationListEntry *walker = NULL;
#ifdef ENABLE_INB_SI_CACHE
    if(si_handle!= 0)
    {
        event_params.data = (void*)si_handle;
    }
#endif
     event_params.data_extension = optionalEventData;


    ret = rmf_osal_event_create(RMF_OSAL_EVENT_CATEGORY_MPEG, event, &event_params, &event_handle);
    if(ret != RMF_INBSI_SUCCESS)
    {
        RDK_LOG(
                RDK_LOG_TRACE1,
                "LOG.RDK.INBSI",
                "<%s: %s> Could not create event handle: %x\n",
                PSIMODULE, __FUNCTION__, ret);
        return ret;
    }

    /* Acquire registration list mutex */
    rmf_osal_mutexAcquire(g_inbsi_registration_list_mutex);

    walker = g_inbsi_registration_list;

    while (walker)
    {
        rmf_osal_Bool notifyWalker = false;
#ifdef ENABLE_INB_SI_CACHE
        /* If we received in-band PAT or PMT event, make sure that this client wants
         to receive the event */
        if (event == RMF_SI_EVENT_IB_PAT_ACQUIRED || event
                == RMF_SI_EVENT_IB_CVCT_ACQUIRED || event
                == RMF_SI_EVENT_IB_PAT_UPDATE || event
                == RMF_SI_EVENT_IB_CVCT_UPDATE)
        {
            if (walker->frequency == 0 || (walker->frequency == eventFrequency))
                notifyWalker = true;
        }
        else if (event == RMF_SI_EVENT_IB_PMT_ACQUIRED || event
                == RMF_SI_EVENT_IB_PMT_UPDATE)
        {
            if ((walker->frequency == 0 && walker->program_number == 0)
                    || (walker->frequency == eventFrequency
                            && walker->program_number == eventProgramNumber))
                notifyWalker = true;
        }
        else
        {
            notifyWalker = true;
        }
#else
        notifyWalker = true;
#endif
        if (notifyWalker)
        {
            ret = rmf_osal_eventqueue_add(walker->queueId, event_handle);
            if (ret != RMF_SUCCESS)
            {
                RDK_LOG(
                        RDK_LOG_TRACE1,
                        "LOG.RDK.INBSI",
                        "<%s: %s> Could not dispatch event: 0x%x\n",
                        ret);
                event = 0;
            }

    //      uint32_t termType = walker->edHandle->terminationType;

    //      mpe_eventQueueSend(walker->edHandle->eventQ, event,
    //              (void*) optional_data, (void*) walker->edHandle,
    //              optionalEventData3);
            

#if 0
            // Do we need to unregister this client?
            if (termType == MPE_ED_TERMINATION_ONESHOT || (termType
                    == MPE_ED_TERMINATION_EVCODE && event
                    == walker->terminationEvent))
            {
                add_to_unreg_list(walker->edHandle);
            }
#endif
        }

        walker = walker->next;
    }

#if 0
    /* Unregister all clients that have been added to unregistration list */
    unregister_clients();
#endif
    if(!g_inbsi_registration_list)
    {
        /*If there are no registered listeners, the event is not pushed to any queue. 
        Delete it. */
        rmf_osal_event_delete(event_handle);
    }


    /* Release registration list mutex */
    rmf_osal_mutexRelease(g_inbsi_registration_list_mutex);
    return RMF_SUCCESS;	
}

rmf_Error rmf_InbSiMgr::set_filter(rmf_PsiParam *psiParam, uint32_t *pFilterId)
{
    //rmf_Error retCode = RMF_INBSI_SUCCESS;
    rmf_FilterSpec filterSpec;
    rmf_Error ret;

  //  uint32_t userData = 0x0;
    uint8_t tid_mask[] = { 0xFF, 0, 0, 0, 0, 0x01 };
    uint8_t tid_val[]  = { 0,    0, 0, 0, 0, 0x01 };
    uint8_t ver_mask[] = { 0,    0, 0, 0, 0, 0x3E };
    uint8_t ver_val[]  = { 0,    0, 0, 0, 0, (INIT_TABLE_VERSION << 1) };

    RDK_LOG(RDK_LOG_TRACE2, "LOG.RDK.INBSI", "<%s: %s> - Enter - table_type = %d\n", PSIMODULE, __FUNCTION__,
            psiParam->table_type);

    /*
     ** Acquire Mutex
     */
    rmf_osal_mutexAcquire(g_sitp_psi_mutex);

    /* Set the filterspec values */
    filterSpec.pos.length = 6;
    filterSpec.neg.mask = ver_mask;
    filterSpec.neg.vals = ver_val;

    switch (psiParam->table_type)
    {
        case TABLE_TYPE_PAT:
            RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.INBSI",
                    "<%s: %s> - Setting a PAT table filter\n",
                    PSIMODULE, __FUNCTION__);
        tid_val[0] = PROGRAM_ASSOCIATION_TABLE_ID;
            filterSpec.pos.vals = tid_val;
            filterSpec.pos.mask = tid_mask;
            break;
#ifdef CAT_FUNCTIONALITY
        case TABLE_TYPE_CAT:
            RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.INBSI", "<%s: %s> - Setting a CAT table filter\n", PSIMODULE, __FUNCTION__);
            tid_val[0] = CONDITIONAL_ACCESS_TABLE_ID;
            //tid_mask[5] = 0x0;
            filterSpec.pos.vals = tid_val;
            filterSpec.pos.mask = tid_mask;
            break;
#endif
        case TABLE_TYPE_PMT:
       {
            /* Set the filterspec values */
            RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.INBSI",
                    "<%s: %s> - Setting a PMT table filter for pn: %d (0x%x)\n",
                    PSIMODULE, __FUNCTION__, psiParam->table_id_extension, psiParam->table_id_extension);
            tid_val[0] = PROGRAM_MAP_TABLE_ID;
            tid_val[3] = psiParam->table_id_extension >> 8;
            tid_val[4] = psiParam->table_id_extension;
            tid_mask[3] = 0xff;
            tid_mask[4] = 0xff;
            filterSpec.pos.mask = tid_mask;
            filterSpec.pos.vals = tid_val;
        }
            break;
        case TABLE_TYPE_TDT:
        {
            /* Set the filterspec values */
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.INBSI", "<%s: %s> - Setting a TDT table filter for pn: %d\n", PSIMODULE, __FUNCTION__, psiParam->table_id);
            tid_val[0]  = psiParam->table_id;
            tid_mask[0] = 0xFC;
            filterSpec.pos.length = 1;
            filterSpec.pos.mask = tid_mask;
            filterSpec.pos.vals = tid_val;
            filterSpec.neg.length = 0;
        }
            break;
        default:
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.INBSI",
                    "<%s: %s> - ERROR: Unknown table type\n",
                    PSIMODULE, __FUNCTION__);
        break;
    }

    if (psiParam->version_number == INIT_TABLE_VERSION)
    {
        filterSpec.neg.length = 0;
        RDK_LOG(RDK_LOG_TRACE2, "LOG.RDK.INBSI",
                "<%s: %s> - NOT setting a neg version filter\n",
                PSIMODULE, __FUNCTION__);
    }
    else
    {
        /* 6 elements in the neg array*/
        filterSpec.neg.length = 6;
        /* Set the version Number value at the version index*/
        filterSpec.neg.vals[PSI_VERSION_INDEX] = (uint8_t)(psiParam->version_number << 1);

        RDK_LOG(RDK_LOG_TRACE2,
                "LOG.RDK.INBSI",
                "<%s: %s> - Setting a neg version filter: version == %d\n",
                PSIMODULE, __FUNCTION__, psiParam->version_number);
    }

    ret = m_pInbSectionFilter->SetFilter(psiParam->pid, &filterSpec,
                                        m_FilterQueueId,
                                        psiParam->priority,
                                        0,
                                        0,
                                        pFilterId);
    if(ret == RMF_INBSI_SUCCESS) 
    {
            rmf_FilterInfo *pFilterInfo = NULL;
            if (rmf_osal_memAllocP(RMF_OSAL_MEM_SI_INB, sizeof(rmf_FilterInfo),
                        (void **) &pFilterInfo) != RMF_INBSI_SUCCESS)
            {
    		    rmf_osal_mutexRelease(g_sitp_psi_mutex);
                    return RMF_INBSI_OUT_OF_MEM;
            }
            
            memset(pFilterInfo, 0x0, sizeof(rmf_FilterInfo));

            pFilterInfo->pid = psiParam->pid;
            pFilterInfo->filterId = *pFilterId;
            if(psiParam->table_type == TABLE_TYPE_PMT)
                pFilterInfo->program_number = psiParam->table_id_extension;

            m_filter_list.push_back(pFilterInfo);
//        m_filter_list.push_back(*pFilterId);
    }
    else
    {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.INBSI",
                    "<%s: %s> - SetFilter Failed with error: %d\n",
                    PSIMODULE, __FUNCTION__, ret);
    }

    /*
     ** Acquire Mutex
     */
    rmf_osal_mutexRelease(g_sitp_psi_mutex);

    return ret;;

}

void rmf_InbSiMgr::cancel_all_filters()
{
    list<rmf_FilterInfo*>::iterator fiter;

    for(fiter = m_filter_list.begin();fiter != m_filter_list.end();fiter++)
    {
        rmf_FilterInfo *pFilterInfo = *fiter;
        m_pInbSectionFilter->ReleaseFilter(pFilterInfo->filterId);
        rmf_osal_memFreeP(RMF_OSAL_MEM_SI_INB, pFilterInfo);
    }

    m_filter_list.clear();
}

void rmf_InbSiMgr::configure_negative_filters(rmf_psi_table_type_e type)
{
    rmf_Error ret = RMF_INBSI_INVALID_PARAMETER;
    list<rmf_FilterInfo*>::iterator filter;
    rmf_PsiParam psiParam = { 
        TABLE_TYPE_PAT,
           0, //pid
           0, //program number
           patVersionNumber, //table version
           RMF_PSI_WAIT_REVISION, //priority
           PROGRAM_ASSOCIATION_TABLE_ID
           }; //initialized by default for PAT tables. 
    uint32_t filterId;
    
    rmf_osal_mutexAcquire(g_sitp_psi_mutex);

    if(TABLE_TYPE_PAT == type)
    {
        for(filter = m_filter_list.begin();filter != m_filter_list.end();filter++)
        {
            if ((0 == (*filter)->program_number) && ((*filter)->pid == 0))
            {
                RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.INBSI",  "%s: Releasing active PAT filter.\n",
                    __FUNCTION__);
                m_pInbSectionFilter->ReleaseFilter((*filter)->filterId);

                rmf_FilterInfo *pFilterListObject = *filter;
                filter = m_filter_list.erase(filter);
                rmf_osal_memFreeP(RMF_OSAL_MEM_SI_INB, pFilterListObject);

                RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.INBSI",  "%s: Setting negative filter for PAT.\n",
                    __FUNCTION__);
                ret = set_filter(&psiParam, &filterId);
                break;
            }
        }
    }
    else if (TABLE_TYPE_PMT == type)
    {
        for(filter = m_filter_list.begin();filter != m_filter_list.end();filter++)
        {
            if(0 < (*filter)->program_number)
            {
                RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.INBSI",  "%s: Releasing active PMT filter.\n",
                    __FUNCTION__);
                m_pInbSectionFilter->ReleaseFilter((*filter)->filterId);
                
                psiParam.pid = (*filter)->pid;
                psiParam.table_type = TABLE_TYPE_PMT;
                psiParam.table_id_extension = (*filter)->program_number;
                psiParam.version_number = pmtVersionNumber;
                psiParam.table_id = PROGRAM_MAP_TABLE_ID;

                rmf_FilterInfo *pFilterListObject = *filter;
                filter = m_filter_list.erase(filter);
                rmf_osal_memFreeP(RMF_OSAL_MEM_SI_INB, pFilterListObject);

                RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.INBSI",  "%s: Setting negative filter for PMT.\n",
                    __FUNCTION__);
                ret = set_filter(&psiParam, &filterId);
                break;
            }
        }
    }
#ifdef CAT_FUNCTIONALITY
    else if (TABLE_TYPE_CAT == type)
    {
        for(filter = m_filter_list.begin();filter != m_filter_list.end();filter++)
        {
            if ((0 == (*filter)->program_number) && ((*filter)->pid == CAT_PID))
            {
                RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.INBSI",  "%s: Releasing active CAT filter (id=%x).\n",
                    __FUNCTION__, (*filter)->filterId);
                m_pInbSectionFilter->ReleaseFilter((*filter)->filterId);

                psiParam.pid = (*filter)->pid;
                psiParam.table_type = TABLE_TYPE_CAT;
                psiParam.table_id_extension = 0;
                psiParam.version_number = catVersionNumber;
                psiParam.table_id = CONDITIONAL_ACCESS_TABLE_ID;

                rmf_FilterInfo *pFilterListObject = *filter;
                filter = m_filter_list.erase(filter);
                rmf_osal_memFreeP(RMF_OSAL_MEM_SI_INB, pFilterListObject);

                RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.INBSI",  "%s: Setting negative filter for CAT.\n",
                    __FUNCTION__);
                ret = set_filter(&psiParam, &filterId);
                break;
            }
        }
    }
#endif

    rmf_osal_mutexRelease(g_sitp_psi_mutex);
    
    if(RMF_SUCCESS != ret)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.INBSI", "%s: Error! table type %d. ret = 0x%x.\n",
            __FUNCTION__, type, ret);
    }
    else
    {
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.INBSI", "%s: table type: %d Success!\n",
            __FUNCTION__, type);
    }

    return;
}

/**
 * @brief This function gets the transport stream ID corresponding to the current frequency and modulation.
 * If the flag ENABLE_INB_SI_CACHE is not set then, it returns the transport stream ID directly without
 * searching in SI cache.
 *
 * @param[out] ptsid To be filled with transport stream ID
 *
 * @return Returns RMF_INBSI_SUCCESS on success, else returns RMF_INBSI_NOT_FOUND which indicates
 * transport stream handle not found for the specified frequency and modulation.
 */
rmf_Error rmf_InbSiMgr::GetTSID(uint32_t* ptsid)
{
    rmf_Error ret = RMF_INBSI_SUCCESS;

#ifdef ENABLE_INB_SI_CACHE
    rmf_SiTransportStreamHandle ts_handle;
    m_pSiCache->LockForRead();
    ret= m_pSiCache->GetTransportStreamEntryFromFrequencyModulation(m_freq, m_modulation_mode,
                &ts_handle);
    if((RMF_INBSI_SUCCESS == ret) && (RMF_INBSI_INVALID_HANDLE != ts_handle))
    {
    	ret = m_pSiCache->GetTransportStreamIdForTransportStreamHandle(ts_handle,ptsid);
    }
    else
    {
	ret = RMF_INBSI_NOT_FOUND;
    }
    m_pSiCache->UnLock();
#else
    *ptsid = ts_id;
#endif
    return ret;
}

rmf_Error rmf_InbSiMgr::get_pmtpid_for_program(uint16_t program_number, uint16_t *pmt_pid)
{
    rmf_Error ret = RMF_INBSI_SUCCESS;
#ifdef ENABLE_INB_SI_CACHE
    rmf_SiProgramHandle prog_handle = 0;
    rmf_SiProgramInfo *program_info = NULL;
#endif

    if(pmt_pid == NULL)
    {
        RDK_LOG(
                RDK_LOG_ERROR,
                "LOG.RDK.INBSI",
                "<%s: %s> Invalid Parameter, returning from function\n",
                PSIMODULE, __FUNCTION__);
        ret = RMF_INBSI_INVALID_PARAMETER;
        return ret;
    }
#ifdef ENABLE_INB_SI_CACHE
    ret = m_pSiCache->GetProgramEntryFromFrequencyProgramNumberModulation(m_freq, program_number, m_modulation_mode, &prog_handle);
    if(ret != RMF_INBSI_SUCCESS)
    {
        RDK_LOG(
                RDK_LOG_ERROR,
                "LOG.RDK.INBSI",
                "<%s: %s> Program Entry not found for program no: %d, Error: %x\n",
                PSIMODULE, __FUNCTION__, program_number, ret);
        ret = RMF_INBSI_NOT_FOUND;
        return ret;
    }

    program_info = (rmf_SiProgramInfo*)prog_handle;
    *pmt_pid = program_info->pmt_pid;
#else
    int i;
    rmf_osal_mutexAcquire( m_mediainfo_mutex);
    for ( i=0; i< m_pmtListLen; i++)
    {
        if (m_pmtInfoList[i].program_no == program_number)
        {
            *pmt_pid = m_pmtInfoList[i].pmt_pid;
            break;
        }
    }
    rmf_osal_mutexRelease( m_mediainfo_mutex);
    if ( i == m_pmtListLen )
    {
        RDK_LOG(
                RDK_LOG_ERROR,
                "LOG.RDK.INBSI",
                "<%s: %s> Program Entry not found for program no: %d\n",
                PSIMODULE, __FUNCTION__, program_number);
        return RMF_INBSI_NOT_FOUND;
    }
#endif

    return RMF_INBSI_SUCCESS;
}

/**
 * @brief This function is used to register for events generated form inband SI manager. On any PSI events
 * such as PAT, PMT acquisition and updates, the queue registered using this function shall be notified.
 *
 * @param[in] queueId Indicates the queue Id to be registered for PSI events.
 * @param[in] program_number Indicates program number to be registered for PSI events.
 *
 * @return Return value indicates success or failure of the call.
 * @retval RMF_INBSI_SUCCESS Indicates the call RegisterForPSIEvents is successful
 * @retval RMF_INBSI_OUT_OF_MEM Indicates the call RegisterForPSIEvents failed due to out of memory
 */
rmf_Error rmf_InbSiMgr::RegisterForPSIEvents(rmf_osal_eventqueue_handle_t queueId, uint16_t program_number)
{
        rmf_InbSiRegistrationListEntry *walker = NULL;
        rmf_InbSiRegistrationListEntry *new_list_member = NULL;

        if (rmf_osal_memAllocP(RMF_OSAL_MEM_SI_INB, sizeof(rmf_InbSiRegistrationListEntry),
                        (void **) &new_list_member) != RMF_INBSI_SUCCESS)
        {
                return RMF_INBSI_OUT_OF_MEM;
        }

        new_list_member->queueId = queueId;
    new_list_member->frequency = m_freq;
    new_list_member->program_number = program_number;
        new_list_member->next = NULL;

        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.INBSI",
            "RegisterForSIEvents called...with queueId: 0x%p\n",
                        queueId);

        /* Acquire registration list mutex */
        rmf_osal_mutexAcquire(g_inbsi_registration_list_mutex);

        if (g_inbsi_registration_list == NULL)
        {
                g_inbsi_registration_list = new_list_member;
        }
       else
        {
                walker = g_inbsi_registration_list;
                while (walker)
                {
                        if (walker->next == NULL)
                        {
                                walker->next = new_list_member;
                                break;
                        }
                        walker = walker->next;
                }
        }

        rmf_osal_mutexRelease(g_inbsi_registration_list_mutex);

    return RMF_INBSI_SUCCESS;
}

/**
 * @brief This function is used by outside entities to stop receiving events generated by inband SI manager.
 * It identifies the PSI event with the specified queueId and program_number, it removes the registered PSI events
 * from the list. And finally releases the memory allocated for it.
 *
 * @param[in] queueId Indicates the queue ID to be unregistered for PSI events.
 * @param[in] program_number Indicates the program number to be unregistered
 *
 * @return Return value indicates success or failure
 * @retval RMF_INBSI_SUCCESS Indicates the call UnRegisterForPSIEvents is successful
 * @retval RMF_OSAL_EINVAL Indicates there are no queue's registered for PSI events, hence the call is unsuccessful
 */
rmf_Error rmf_InbSiMgr::UnRegisterForPSIEvents(rmf_osal_eventqueue_handle_t queueId, uint16_t program_number)
{
        rmf_InbSiRegistrationListEntry *walker, *prev;
        rmf_InbSiRegistrationListEntry *to_delete = NULL;
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.INBSI", "%s - Acquiring mutex.\n",  __FUNCTION__);   //Temp debug

        /* Acquire registration list mutex */
        rmf_osal_mutexAcquire(g_inbsi_registration_list_mutex);
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.INBSI", "%s - Acquired mutex.\n",  __FUNCTION__);   //Temp debug
        /* Remove from the list */
        if (g_inbsi_registration_list == NULL)
        {
                rmf_osal_mutexRelease(g_inbsi_registration_list_mutex);
                return RMF_OSAL_EINVAL;
        }
        else if (g_inbsi_registration_list->queueId == queueId)
        {
                to_delete = g_inbsi_registration_list;
                g_inbsi_registration_list = g_inbsi_registration_list->next;
        }
        else
        {
                prev = walker = g_inbsi_registration_list;
                while (walker)
                {
                        if ((walker->queueId == queueId) && (walker->program_number == program_number))
                        {
                                to_delete = walker;
                                prev->next = walker->next;
                                break;
                        }
                        prev = walker;
                        walker = walker->next;
                }
        }

        rmf_osal_mutexRelease(g_inbsi_registration_list_mutex);
        /* Release registration list mutex */

        if (to_delete != NULL)
        {
                rmf_osal_memFreeP(RMF_OSAL_MEM_SI_INB, to_delete);
        }

        return RMF_INBSI_SUCCESS;
}

/**
 * @brief This function is used to set a filter for PAT table.
 * If ENABLE_INB_SI_CACHE flag is defined, it creates a TS handle corresponding to the frequency
 * and modulation if not already present. And then notifies PAT availability to SI cache.
 * It sets a filter for PAT table irrespective of ENABLE_INB_SI_CACHE flag.
 *
 * @return Returns TRUE if the call is successful and PAT filter is set, else returns FALSE.
 */
rmf_Error rmf_InbSiMgr::RequestTsInfo()
{
    uint32_t filterId = 0;
    rmf_PsiParam psiParam;
    rmf_Error ret = RMF_INBSI_SUCCESS;

#ifdef ENABLE_INB_SI_CACHE
    rmf_SiTransportStreamHandle ts_handle;
    rmf_SiTransportStreamEntry *ts_entry=NULL;
    
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.INBSI",
                       "<%s: %s>(%d): Enter\n", PSIMODULE, __FUNCTION__, __LINE__);

    //Check whether the required transport stream entry is already available in SI Cache
    m_pSiCache->LockForRead();
    ret= m_pSiCache->GetTransportStreamEntryFromFrequencyModulation(m_freq, m_modulation_mode,
                &ts_handle);    
    m_pSiCache->UnLock();
   
   if((RMF_INBSI_SUCCESS == ret) && (RMF_INBSI_INVALID_HANDLE != ts_handle))
   {
       ts_entry = (rmf_SiTransportStreamEntry*)ts_handle;
        if(SI_AVAILABLE == ts_entry->siStatus)
        {
    	    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.INBSI", "<%s: %s> - Sending PAT Event\n",
                PSIMODULE, __FUNCTION__);   
            m_bIsPATAvailable = TRUE;
            NotifyTableChanged(TABLE_TYPE_PAT, RMF_SI_CHANGE_TYPE_ADD, ts_handle);
        }
        else
            m_bIsPATAvailable = FALSE;
   }
   else
   {
        m_bIsPATAvailable = FALSE;
   }
#else
    m_bIsPATAvailable = FALSE;
#endif

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.INBSI", "<%s: %s> - Setting PAT Section Filter\n",
                PSIMODULE, __FUNCTION__);   
    num_sections = 0;
    psiParam.pid = 0x00;
    psiParam.table_type = TABLE_TYPE_PAT;   
    psiParam.version_number = INIT_TABLE_VERSION;
    psiParam.priority = RMF_PSI_FILTER_PRIORITY_INITIAL_PAT;
    psiParam.table_id = PROGRAM_ASSOCIATION_TABLE_ID;

    ret = set_filter(&psiParam, &filterId);

    m_bIsPATFilterSet = (ret == RMF_SUCCESS?TRUE:FALSE);

    return ret;
}

#ifdef CAT_FUNCTIONALITY
rmf_Error rmf_InbSiMgr::RequestTsCatInfo()
{
    uint32_t filterId = 0;
    rmf_PsiParam psiParam;
    rmf_Error ret = RMF_INBSI_SUCCESS;

#ifdef ENABLE_INB_SI_CACHE
    rmf_SiTransportStreamHandle ts_handle;
    rmf_SiTransportStreamEntry *ts_entry=NULL;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.INBSI",
                       "<%s: %s>(%d): Enter\n", PSIMODULE, __FUNCTION__, __LINE__);

    //Check whether the required transport stream entry is already available in SI Cache
    m_pSiCache->LockForRead();
    ret= m_pSiCache->GetTransportStreamEntryFromFrequencyModulation(m_freq, m_modulation_mode,
                &ts_handle);
    m_pSiCache->UnLock();

   if((RMF_INBSI_SUCCESS == ret) && (RMF_INBSI_INVALID_HANDLE != ts_handle))
   {
       ts_entry = (rmf_SiTransportStreamEntry*)ts_handle;
        if(SI_AVAILABLE == ts_entry->siStatus)
        {
          RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.INBSI", "<%s: %s> - Sending CAT Event\n", PSIMODULE, __FUNCTION__);
          m_bIsCATAvailable = TRUE;
          NotifyTableChanged(TABLE_TYPE_CAT, RMF_SI_CHANGE_TYPE_ADD, ts_handle);
        }
        else
            m_bIsCATAvailable = FALSE;
   }
   else
   {
        m_bIsCATAvailable = FALSE;
   }
#else
    m_bIsCATAvailable = FALSE;
#endif

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.INBSI", "<%s: %s> - Setting CAT Section Filter\n",
                PSIMODULE, __FUNCTION__);
    num_cat_sections = 0;
    psiParam.pid = CAT_PID;
    psiParam.table_type = TABLE_TYPE_CAT;
    psiParam.version_number = INIT_TABLE_VERSION;
    psiParam.priority = RMF_PSI_FILTER_PRIORITY_INITIAL_CAT;
    psiParam.table_id = CONDITIONAL_ACCESS_TABLE_ID;

    ret = set_filter(&psiParam, &filterId);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.INBSI", "<%s: %s> - Setting CAT Section Filter (id=%x , isset=%x, ret=%x)\n",
                PSIMODULE, __FUNCTION__, filterId, m_bIsCATFilterSet, ret);

    m_bIsCATFilterSet = (ret == RMF_SUCCESS?TRUE:FALSE);

    return ret;
}
#else
rmf_Error rmf_InbSiMgr::RequestTsCatInfo() { return RMF_INBSI_INVALID_PARAMETER; }
#endif

rmf_Error rmf_InbSiMgr::RequestTsTDTInfo(bool enable)
{
    uint32_t filterId = 0;
    rmf_PsiParam psiParam;
    rmf_Error ret = RMF_INBSI_SUCCESS;

    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.INBSI", "<%s: %s> - Setting TDT Section Filter\n",    PSIMODULE, __FUNCTION__);   
    
    if (enable) {
#ifdef CAT_FUNCTIONALITY
      num_cat_sections = 0;
#endif
      psiParam.pid            = TDT_TOT_PID;
      psiParam.table_type     = TABLE_TYPE_TDT;
      psiParam.version_number = INIT_TABLE_VERSION;
      psiParam.priority       = RMF_PSI_FILTER_PRIORITY_INITIAL_TDT_TOT;
      psiParam.table_id       = TDT_TABLE_ID | TOT_TABLE_ID;

      ret = set_filter(&psiParam, &filterId);
      RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.INBSI", "<%s: %s> - Setting TDT Section Filter (id=%x , ret=%x)\n", PSIMODULE, __FUNCTION__, filterId, ret);
    } else {
      list<rmf_FilterInfo*>::iterator fiter;

      rmf_osal_mutexAcquire(g_sitp_psi_mutex);
      for(fiter = m_filter_list.begin();fiter != m_filter_list.end();fiter++)
      {
        rmf_FilterInfo *pFilterInfo = *fiter;
        if (pFilterInfo->pid == TDT_TOT_PID) {
          m_filter_list.erase(fiter);
          m_pInbSectionFilter->ReleaseFilter(pFilterInfo->filterId);
          rmf_osal_memFreeP(RMF_OSAL_MEM_SI_INB, pFilterInfo);
          break;
        }
      }
      rmf_osal_mutexRelease(g_sitp_psi_mutex);
    }
    return ret;
}


/**
 * @brief This function gets the PAT information and checks if the PMT is available and then sets a filter for PMT
 * corresponding to the specified program number. It checks if PAT is available and the filter for it is set.
 * If PAT is not available, it waits till the timeout period and returns PAT not available.
 *
 * @param[in] program_number Indicates the program number for which PMT filter need to be set
 *
 * @return Return value indicates success or failure of the call.
 * @retval RMF_INBSI_SUCCESS Indicates the filter was set for the PMT and the call is successful
 * @retval RMF_INBSI_PAT_NOT_AVAILABLE Indicates PAT not available and the call is unsuccessful
 * @retval RMF_INBSI_INVALID_PARAMETER Indicates PMT PID not available for the program number and the call is unsuccessful
 * @retval RMF_INBSI_OUT_OF_MEM Indicates memory allocation for filter info failed and the call is unsuccessful
 * @retval RMF_OSAL_EINVAL Indicates, invalid parameter was passed while setting a section filter
 * @retval RMF_SF_ERROR_FILTER_NOT_AVAILABLE Indicates section filter unavailability or failure to start the section filter
 */
rmf_Error rmf_InbSiMgr::RequestProgramInfo(uint16_t program_number)
{
    uint16_t pmt_pid = 0;
    uint32_t filterId = 0;
    rmf_PsiParam psiParam;
#ifdef ENABLE_INB_SI_CACHE
    rmf_Error ret = RMF_INBSI_SUCCESS;
    rmf_SiProgramHandle prog_handle;
    rmf_SiPMTStatus pmt_status;
#endif

    if(!m_bIsPATAvailable)
    {
        if(!m_bIsPATFilterSet)
            RequestTsInfo();

        // Inband SI Manager makes this call to acquire the global PAT lock
            // block here until the condition object is set
        if(!m_bIsPATAvailable)
        {
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.INBSI", "<%s: %s> - Waiting for PAT\n",
                     PSIMODULE, __FUNCTION__);
                rmf_osal_condWaitFor(g_inbsi_pat_cond, RMF_INBSI_PAT_TIMEOUT_MS);
                if(!m_bIsPATAvailable)
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.INBSI", "<%s: %s> - PAT not available after timeout\n",
                        PSIMODULE, __FUNCTION__);
                    return RMF_INBSI_PAT_NOT_AVAILABLE;
                }
        }        
    }

#ifdef ENABLE_INB_SI_CACHE
    //Check whether the required program info is already available in SI Cache
    m_pSiCache->LockForRead();
    ret = m_pSiCache->GetProgramEntryFromFrequencyProgramNumberModulation(m_freq, program_number, m_modulation_mode, &prog_handle);

    if((RMF_INBSI_SUCCESS == ret) && (RMF_INBSI_INVALID_HANDLE != prog_handle))
    {
        ret = m_pSiCache->GetPMTProgramStatus(prog_handle, &pmt_status);
        if((RMF_INBSI_SUCCESS == ret) && (PMT_AVAILABLE == pmt_status))
        {
            NotifyTableChanged(TABLE_TYPE_PMT, RMF_SI_CHANGE_TYPE_ADD, prog_handle);
        }
        else
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.INBSI", "<%s: %s> - PMT Not Present pmt_status = %d ret= %d\n",
                            PSIMODULE, __FUNCTION__, pmt_status, ret );
        }
    }
    else
    {
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.INBSI", "<%s: %s> - Program Entry Not Present\n",
                        PSIMODULE, __FUNCTION__);
    }
    m_pSiCache->UnLock();
#endif
    if(RMF_INBSI_SUCCESS != get_pmtpid_for_program(program_number, &pmt_pid))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.INBSI", "<%s: %s> - PMT PID not available for program_number %d\n",
                PSIMODULE, __FUNCTION__, program_number);
        return RMF_INBSI_INVALID_PARAMETER;
    }

    psiParam.pid = pmt_pid;
    psiParam.table_type = TABLE_TYPE_PMT;
    psiParam.table_id_extension = program_number;
    psiParam.version_number = INIT_TABLE_VERSION;
    psiParam.priority = RMF_PSI_WAIT_INITIAL_PRIMARY_PMT;
    psiParam.table_id = PROGRAM_MAP_TABLE_ID;

#ifndef ENABLE_INB_SI_CACHE
    rmf_osal_mutexAcquire( m_mediainfo_mutex);
    pmtPid = pmt_pid;
    rmf_osal_mutexRelease( m_mediainfo_mutex);
#endif

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.INBSI", "<%s: %s> - Setting PMT Section Filter  for program_number %d (PID %d)\n",
                PSIMODULE, __FUNCTION__, program_number, pmt_pid);

    return set_filter(&psiParam, &filterId);
}

/**
 * @brief This function notifies the SIDB that a tune is about to happen for
 * the reqParams related service entry so, the state can be changed to
 * PMT_AVAILABLE_SHORTLY. All failure and cleanup state changes are handled
 * in the sitp module state machine. First get the freq and program number
 * from the current media info.
 *
 * @param[in] new_freq Indicates the new frequency to which the box is tuned
 * @param[in] new_mode Indicates the new modulation mode to which the box is tuned
 *
 * @return None
 */
void rmf_InbSiMgr::TuneInitiated(uint32_t new_freq, uint8_t new_mode)
{
#ifdef ENABLE_INB_SI_CACHE
    rmf_SiTransportStreamHandle ts_handle = RMF_INBSI_INVALID_HANDLE; // a handle into ths SI database
    rmf_SiProgramHandle prog_handle = RMF_INBSI_INVALID_HANDLE; // a handle into ths SI database
#endif
    //uint32_t freq = 0;
    //uint32_t freq = 0;
    //uint32_t progNum = 0;
    //uint32_t tuner_number = 0;
    //mpe_SiModulationMode modMode = 0;
    //uint8_t index = 0;

    /*
     ** Notify SIDB that a tune is about to happen for
     ** the reqParams related service entry so the state
     ** can be changed to PMT_AVAILABLE_SHORTLY.  All
     ** failure and cleanup state changes are handled
     ** in the sitp module state machine.
     ** First get the freq and program number from the current media info.
     */
    //tuner_number = tuneRequest->tunerId;
    //freq = tuneRequest->tuneParams.frequency;
    //progNum = tuneRequest->tuneParams.programNumber;
    //modMode = tuneRequest->tuneParams.qamMode;

    //index = tuner_number-1; // Index starts at 0

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.INBSI", "<%s: %s> - Enter.. new_freq:%d, new_mode: %d\n",
            PSIMODULE, __FUNCTION__, new_freq, new_mode);

    /*
     ** Acquire the global mutex.
     */
    rmf_osal_mutexAcquire(g_sitp_psi_mutex);

      /*
       ** Is the new frequency same as the current tuner frequency?? If not,
       ** set the previous SIDB Transport stream info to NOT_AVAILABLE (will be done at TUNE_STARTED)
       ** and set the new SIDB transport Stream info to AVAILABLE_SHORTLY.
       */
#ifdef ENABLE_INB_SI_CACHE
    if(m_freq != new_freq)
    {
        m_pSiCache->LockForWrite();
        m_pSiCache->GetTransportStreamEntryFromFrequencyModulation(new_freq, new_mode,
                &ts_handle);
        m_pSiCache->SetPMTStatusAvailableShortly(ts_handle);
        m_pSiCache->SetTSIdStatusAvailableShortly(ts_handle);
        
#ifdef OPTIMIZE_INBSI_CACHE
        //m_pSiCache->ReleaseTransportStreamEntry(ts_handle);
#endif
        m_pSiCache->ReleaseWriteLock();
    }
#endif

    /*
     ** Now set the new frequency and mode to Inband SI Manager.
     */
    m_freq = new_freq;
    m_modulation_mode = new_mode;

    // Clean all outstanding filters
    cancel_all_filters();

    /*
     ** Release the global mutex.
     */
    rmf_osal_mutexRelease(g_sitp_psi_mutex);
    
    RDK_LOG(RDK_LOG_TRACE2, "LOG.RDK.INBSI",
            "<%s: %s> - released mutex..\n", PSIMODULE, __FUNCTION__);

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.INBSI", "<%s: %s> - Exit..\n",
            PSIMODULE, __FUNCTION__);
    
}

const char* rmf_InbSiMgr::getStreamTypeString( int eValue )
{
    static char szString[32];
    const char * pszValue = "UNKNOWN";
    
    switch(eValue)
    {
        RMF_CASE_STREAM_TYPE_STRING(MPEG_1_VIDEO);
        RMF_CASE_STREAM_TYPE_STRING(MPEG_2_VIDEO);
        RMF_CASE_STREAM_TYPE_STRING(MPEG_1_AUDIO);
        RMF_CASE_STREAM_TYPE_STRING(MPEG_2_AUDIO);
        RMF_CASE_STREAM_TYPE_STRING(MPEG_PRIVATE_SECTION);
        RMF_CASE_STREAM_TYPE_STRING(MPEG_PRIVATE_DATA);
        RMF_CASE_STREAM_TYPE_STRING(MHEG);
        RMF_CASE_STREAM_TYPE_STRING(DSM_CC);
        RMF_CASE_STREAM_TYPE_STRING(H_222);
        RMF_CASE_STREAM_TYPE_STRING(DSM_CC_MPE);
        RMF_CASE_STREAM_TYPE_STRING(DSM_CC_UN);
        RMF_CASE_STREAM_TYPE_STRING(DSM_CC_STREAM_DESCRIPTORS);
        RMF_CASE_STREAM_TYPE_STRING(DSM_CC_SECTIONS);
        RMF_CASE_STREAM_TYPE_STRING(AUXILIARY);
        RMF_CASE_STREAM_TYPE_STRING(AAC_ADTS_AUDIO);
        RMF_CASE_STREAM_TYPE_STRING(ISO_14496_VISUAL);
        RMF_CASE_STREAM_TYPE_STRING(AAC_AUDIO_LATM);
        RMF_CASE_STREAM_TYPE_STRING(FLEXMUX_PES);
        RMF_CASE_STREAM_TYPE_STRING(FLEXMUX_SECTIONS);
        RMF_CASE_STREAM_TYPE_STRING(SYNCHRONIZED_DOWNLOAD);
        RMF_CASE_STREAM_TYPE_STRING(METADATA_PES);
        RMF_CASE_STREAM_TYPE_STRING(METADATA_SECTIONS);
        RMF_CASE_STREAM_TYPE_STRING(METADATA_DATA_CAROUSEL);
        RMF_CASE_STREAM_TYPE_STRING(METADATA_OBJECT_CAROUSEL);
        RMF_CASE_STREAM_TYPE_STRING(METADATA_SYNCH_DOWNLOAD);
        RMF_CASE_STREAM_TYPE_STRING(MPEG_2_IPMP);
        RMF_CASE_STREAM_TYPE_STRING(AVC_VIDEO);
        RMF_CASE_STREAM_TYPE_STRING(VIDEO_DCII);
        RMF_CASE_STREAM_TYPE_STRING(ATSC_AUDIO);
        RMF_CASE_STREAM_TYPE_STRING(STD_SUBTITLE);
        RMF_CASE_STREAM_TYPE_STRING(ISOCHRONOUS_DATA);
        RMF_CASE_STREAM_TYPE_STRING(ASYNCHRONOUS_DATA);
        RMF_CASE_STREAM_TYPE_STRING(ENHANCED_ATSC_AUDIO);
    }
    
    snprintf(szString, sizeof(szString), "%s (0x%02X)", pszValue, eValue);
    
    return szString;
}

#ifdef CAT_FUNCTIONALITY
rmf_Error rmf_InbSiMgr::GetCatTable(rmf_section_f section_fn, void* user_data)
{
    rmf_Error ret = RMF_INBSI_SUCCESS;

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.INBSI", "++%s(section_fn: %p, user_data: %p)\n", __FUNCTION__, section_fn, user_data);
    rmf_osal_mutexAcquire(m_mediainfo_mutex);
    if (section_fn)
    {
      section_fn(m_total_descriptor_length, m_catTable, user_data);
    }
    else
    {
      ret = RMF_INBSI_INVALID_PARAMETER;
    }
    rmf_osal_mutexRelease(m_mediainfo_mutex);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.INBSI", "--%s() - ret: %d\n", __FUNCTION__, ret);
    return ret;
}
#else
rmf_Error rmf_InbSiMgr::GetCatTable(rmf_section_f section_fn, void* user_data) { return RMF_INBSI_INVALID_PARAMETER; }
#endif

/**
 * @brief This function gets the media information like PCR Pid, PMT Pid and elementary stream type such as audio,
 * video and their Pid's corresponding to the specified program number.
 *
 * @param[in] program_number Indicates the program number against which media information is required
 * @param[out] pMediaInfo To be filled with media information corresponding to the specified program number
 *
 * @return Return value indicates success or failure
 * @retval RMF_INBSI_SUCCESS Indicates the call was successful and media information was fetched successfully
 * @retval RMF_INBSI_PMT_NOT_AVAILABLE Indicates PMT for the specified program number is not available
 * @retval RMF_INBSI_INVALID_PARAMETER
 * @retval RMF_INBSI_OUT_OF_MEM Indicates failure to allocate memory for either rmf_SiTransportStreamEntry or rmf_SiProgramInfo
 */
rmf_Error rmf_InbSiMgr::GetMediaInfo(uint16_t program_number, rmf_MediaInfo *pMediaInfo)
{
    rmf_Error ret = RMF_INBSI_SUCCESS;
#ifdef ENABLE_INB_SI_CACHE
    rmf_SiProgramHandle prog_handle;
    rmf_SiProgramInfo *pi = NULL;
//    uint8_t aPID_index = RMF_SI_MAX_APIDS;
#endif
    rmf_SiElementaryStreamList *pListEntry;
    int i=0;
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.INBSI", "<%s: %s> - Enter.. program_number:%d\n",
            PSIMODULE, __FUNCTION__, program_number);

    if(pMediaInfo == NULL)
    {
    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.INBSI", "<%s: %s>: pMediaInfo is NULL\n", 
                        PSIMODULE, __FUNCTION__);          
        return RMF_INBSI_INVALID_PARAMETER;
    }
#ifdef ENABLE_INB_SI_CACHE
    memset(pMediaInfo, 0, sizeof(rmf_MediaInfo));

    m_pSiCache->LockForRead();
    
    ret = m_pSiCache->GetProgramEntryFromFrequencyProgramNumberModulation(m_freq, program_number, m_modulation_mode,
        &prog_handle);
    
    if(ret == RMF_INBSI_SUCCESS)
    {
        pi = (rmf_SiProgramInfo*)prog_handle;
        if(pi->pmt_status == PMT_AVAILABLE)
        {
            pMediaInfo->pmt_pid = pi->pmt_pid;
            pMediaInfo->pcrPid   = pi->pcr_pid;
            pMediaInfo->pmt_crc32 = pi->pmt_crc;

            pMediaInfo->numMediaPids = (pi->number_elem_streams>RMF_SI_MAX_PIDS_PER_PGM)?RMF_SI_MAX_PIDS_PER_PGM:pi->number_elem_streams;

            pListEntry = pi->elementary_streams;

            i=0;
            while((NULL != pListEntry) && i < RMF_SI_MAX_PIDS_PER_PGM )
            {
                pMediaInfo->pPidList[i].pidType = pListEntry->elementary_stream_type;
                pMediaInfo->pPidList[i].pid = pListEntry->elementary_PID;
                RDK_LOG( RDK_LOG_INFO,"LOG.RDK.INBSI", "PID = %p, Stream Type = %s\n",
                         pListEntry->elementary_PID, getStreamTypeString(pListEntry->elementary_stream_type) );

                pListEntry = pListEntry->next;  
                i++;
            }
        }
            else
            {
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.INBSI", "<%s: %s>: PMT not available for this program # %d \n", 
                        PSIMODULE, __FUNCTION__, program_number);
                ret = RMF_INBSI_PMT_NOT_AVAILABLE;
            }
    }
    else
    {
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.INBSI", "<%s: %s>: PMT not available for this program # %d, ret: %d \n", 
                PSIMODULE, __FUNCTION__, program_number, ret);
    }

    m_pSiCache->UnLock();
#else
    rmf_osal_mutexAcquire( m_mediainfo_mutex);
    pMediaInfo->pmt_pid = pmtPid;
    pMediaInfo->pcrPid   = pcrPid;
    pMediaInfo->pmt_crc32 = pmtCRC;

    pMediaInfo->numMediaPids = (number_elem_streams>RMF_SI_MAX_PIDS_PER_PGM)?RMF_SI_MAX_PIDS_PER_PGM:number_elem_streams;;

    pListEntry = elementary_streams;
    while((NULL != pListEntry) && i < RMF_SI_MAX_PIDS_PER_PGM )
    {
        pMediaInfo->pPidList[i].pidType = pListEntry->elementary_stream_type;
        pMediaInfo->pPidList[i].pid = pListEntry->elementary_PID;
        RDK_LOG( RDK_LOG_INFO,"LOG.RDK.INBSI", "PID = %p, Stream Type = %s\n",
                pListEntry->elementary_PID, getStreamTypeString(pListEntry->elementary_stream_type) );

        pListEntry = pListEntry->next;  
        i++;
    }
    rmf_osal_mutexRelease( m_mediainfo_mutex);
#endif

    return ret;
}

#ifdef QAMSRC_PMTBUFFER_PROPERTY
/**
 * @brief This function copies the extracted PMT data to buf variable passed as input parameter.
 *
 * @param[out] buf Provides a pointer to copy the PMT buffer
 * @param[in] length Indicates the length of the parameter buf in bytes.
 *
 * @return Return value indicates success or failure of the call
 * @retval RMF_INBSI_SUCCESS Indicates the PMT buffer is copied to buf successfully
 * @retval RMF_INBSI_INVALID_PARAMETER Indicates, parameter buf is NULL and the call is unsuccesful
 */
rmf_Error rmf_InbSiMgr::GetPMTBuffer(uint8_t * buf, uint32_t* length)
{
    rmf_Error ret = RMF_INBSI_INVALID_PARAMETER;
    if(buf == NULL)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.INBSI", "<%s: %s>: buf is NULL\n", 
                        PSIMODULE, __FUNCTION__);          
        return ret;
    }
    rmf_osal_mutexAcquire( m_mediainfo_mutex);
    if (*length >= pmtBufLen )
    {
        memcpy(buf, pmtBuf, pmtBufLen);
        *length = pmtBufLen;
        ret = RMF_INBSI_SUCCESS;
    }
    rmf_osal_mutexRelease( m_mediainfo_mutex);
    return ret;
}
#endif

/**
 * @brief This function gets CA information like ECM pid, number of elementary streams and their PID's
 * corresponding to the specified program number and CAS ID.
 *
 * @param[in] program_number Indicates the program number against which the CA information is required
 * @param[in] ca_system_id Indicates CAS ID against which the CA information needs to be filled
 * @param[out] pCaInfo To be filled with CA information like ECM PID, number of elementary streams and their PID's
 *
 * @return Return value indicates success or failure of the call
 * @retval RMF_INBSI_SUCCESS Indicates the CA information was fetched successfuly
 * @retval RMF_INBSI_NO_CA_INFO_IN_PMT Indicates no CA information was found against the specified program number
 * @retval RMF_INBSI_INVALID_PARAMETER Indicates either invalid parameter passed to this funtion or to the functions called internally
 * @retval RMF_INBSI_OUT_OF_MEM Indicates failure to allocate memory for either rmf_SiTransportStreamEntry or rmf_SiProgramInfo
 * @retval RMF_INBSI_LOCKING_ERROR Indicates failure to release the read lock for SI cache
 * @retval RMF_OSAL_EINVAL Invalid parameter was passed while allocating memory to pStreamInfo
 * @retval RMF_OSAL_ENOMEM Indicates memory allocation to pStreamInfo failed
 */
rmf_Error rmf_InbSiMgr::GetCAInfoForProgram(uint16_t program_number, uint16_t ca_system_id, rmf_CaInfo_t *pCaInfo)
{
    rmf_Error retCode = RMF_INBSI_SUCCESS;
#ifdef ENABLE_INB_SI_CACHE
    rmf_SiProgramHandle prog_handle;
    rmf_SiProgramInfo *pi = NULL;
    int i=0;

    if(pCaInfo == NULL)
    {
    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.INBSI", "<%s: %s>: pMediaInfo is NULL\n", 
                        PSIMODULE, __FUNCTION__);          
        return RMF_INBSI_INVALID_PARAMETER;
    }

    memset(pCaInfo, 0, sizeof(rmf_CaInfo_t));

    m_pSiCache->LockForRead();

    retCode = m_pSiCache->GetProgramEntryFromFrequencyProgramNumberModulation(m_freq, program_number, m_modulation_mode,
        &prog_handle);
    if(retCode != RMF_INBSI_SUCCESS)
    {
         RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.INBSI",
                "<%s: %s>: PMT not found for program_number %d\n",
                PSIMODULE, __FUNCTION__, program_number);

        if (m_pSiCache->UnLock() != RMF_SUCCESS)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.INBSI",
                    "<%s::%s> - Could not unlock SI\n", PSIMODULE, __FUNCTION__);
        }
        return retCode;
   }
 
    if ((retCode = m_pSiCache->GetNumberOfComponentsForServiceHandle(prog_handle,
                &(pCaInfo->num_streams))) != RMF_SUCCESS)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.INBSI",
                "<%s: %s>: Could not get number of components for program_number %d\n",
                PSIMODULE, __FUNCTION__, program_number);

        if (m_pSiCache->UnLock() != RMF_SUCCESS)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.INBSI",
                    "<%s::%s> - Could not unlock SI\n", PSIMODULE, __FUNCTION__);
        }
        return retCode;
    }

    if ((retCode = rmf_osal_memAllocP(RMF_OSAL_MEM_SI_INB, (pCaInfo->num_streams * sizeof(rmf_StreamDecryptInfo)), (void**) &(pCaInfo->pStreamInfo))) != RMF_SUCCESS)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.INBSI",
                "<%s::%s> - Could not malloc memory for streamInfo \n",
                PSIMODULE, __FUNCTION__);

        if (m_pSiCache->UnLock() != RMF_SUCCESS)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.INBSI",
                    "<%s::%s> - Could not unlock SI\n", PSIMODULE, __FUNCTION__);
        }
        return retCode;
    }

    // Check if there are any CA descriptors for the given service handle
    // This method populates the input table with elementary stream Pids associated with
    // the service. The CA status field is set to unknown. It will
    // be filled when ca reply/update is received from CableCARD.
    if( m_pSiCache->CheckCADescriptors(prog_handle, ca_system_id,
                                          &(pCaInfo->num_streams), &(pCaInfo->ecmPid), pCaInfo->pStreamInfo) != TRUE)
    {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.INBSI",
                "<%s::%s> - No CA descriptors for program_number: %d (could be clear channel..)\n",
                PSIMODULE, __FUNCTION__, program_number);
            // CCIF 2.0 section 9.7.3
            // Even if the PMT does not contain any CA descriptors
            // a CA_PMT still needs to be sent to the card but with no CA descriptors
            // This path can no longer be used as an early return but used just
            // to retrieve the component information
        retCode = RMF_INBSI_NO_CA_INFO_IN_PMT;

        if(pCaInfo->pStreamInfo != NULL)
        {
          rmf_osal_memFreeP(RMF_OSAL_MEM_SI_INB, pCaInfo->pStreamInfo);
          pCaInfo->pStreamInfo = NULL;
        }

        if (m_pSiCache->UnLock() != RMF_INBSI_SUCCESS)
        {
          RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.INBSI",
                    "<%s::%s> - Could not unlock SI\n", PSIMODULE, __FUNCTION__);
        }
        return retCode;
    }

    m_pSiCache->UnLock();
#endif
    return retCode;
}

/**
 * @brief This function clear the PMT filter only. PAT filter remain same.
 * - It releases the filter set against the specified program number and deletes the entry in the list.
 * - It gets the program handle from SI cache corresponding to the specified program number
 * and sets the PMT status to not available. Then it sets the pmtVersionNumber to default value. This step
 * is performed only if ENABLE_INB_SI_CACHE compilation flag is set.
 *
 * @param[in] program_number Indicates the program number against which the information needs to be cleared.
 *
 * @return Returns RMF_INBSI_SUCCESS on success else returns RMF_INBSI_NOT_FOUND which indicates no filter
 * was set against the specified program number.
 */
rmf_Error rmf_InbSiMgr::ClearProgramInfo(uint16_t program_number)
{
    rmf_Error ret = RMF_INBSI_SUCCESS;

#ifdef ENABLE_INB_SI_CACHE
#ifdef OPTIMIZE_INBSI_CACHE
    rmf_SiProgramHandle prog_handle = RMF_INBSI_INVALID_HANDLE;
#endif
#endif

    list<rmf_FilterInfo*>::iterator fiter;
    bool found = FALSE;
    rmf_osal_mutexAcquire(g_sitp_psi_mutex);
    for(fiter = m_filter_list.begin();fiter != m_filter_list.end();fiter++)
    {
        rmf_FilterInfo *pFilterInfo = *fiter;
        if(pFilterInfo->program_number == program_number)
        {
            RDK_LOG(RDK_LOG_INFO ,"LOG.RDK.INBSI", "%s: Releasing filter for program # %d\n", __FUNCTION__, program_number); 
            m_pInbSectionFilter->ReleaseFilter(pFilterInfo->filterId);
            m_filter_list.remove(pFilterInfo);
            rmf_osal_memFreeP(RMF_OSAL_MEM_SI_INB, pFilterInfo);

#ifdef ENABLE_INB_SI_CACHE
#ifdef OPTIMIZE_INBSI_CACHE
            m_pSiCache->GetProgramEntryFromFrequencyProgramNumberModulation(m_freq, program_number, m_modulation_mode, &prog_handle);
            if(prog_handle!=RMF_INBSI_INVALID_HANDLE)
            {
                //m_pSiCache->ReleaseProgramInfoEntry(prog_handle);
                m_pSiCache->SetPMTStatusNotAvailable(prog_handle);
            }
#endif
#endif
            found = TRUE;
            break;
        }
    }
    rmf_osal_mutexRelease(g_sitp_psi_mutex);
    if(!found)
    {
       RDK_LOG(RDK_LOG_WARN, "LOG.RDK.INBSI", "No filter set for this program # %d\n", program_number); 
        ret = RMF_INBSI_NOT_FOUND;
    }
#ifndef ENABLE_INB_SI_CACHE
    pmtVersionNumber = INIT_TABLE_VERSION;
#endif
    return ret;
}

/**
 * @brief This function pauses all the filters assigned for the current frequency and modulation.
 * Internally stop function is called on all the filters to be paused.
 * - It first checks if the filters are paused already.
 * - If the filters are not paused already then it internally uses stop function to pause
 * all the fileters associated with the current frequency and modulation.
 * - Finally sets isPaused to TRUE indicating all the filters are paused.
 *
 * @return Returns RMF_INBSI_SUCCESS on success else returns appropriate rmf_Error codes.
 */
rmf_Error rmf_InbSiMgr::pause(void)
{
	rmf_Error result;
	rmf_Error ret = RMF_INBSI_SUCCESS;
	std::list<rmf_FilterInfo*>::iterator filter;
	rmf_osal_mutexAcquire(g_sitp_psi_mutex);
	if(!isPaused)
	{
		for(filter = m_filter_list.begin();filter != m_filter_list.end();filter++)
		{
			result = m_pInbSectionFilter->pause((*filter)->filterId);
			if(RMF_SECTFLT_Success != result)
			{
				ret = result;
			}
			isPaused = TRUE;
		}
	}
	rmf_osal_mutexRelease(g_sitp_psi_mutex);
	return ret;
}

/**
 * @brief This function resumes all the filters associated with the current frequency and modulation.
 * Internally start function is called on all the filters to be resumed.
 * - It first checks if the filters are paused to resume them back.
 * - It the filters are paused then it internally uses start function to resume all the filters
 * associated with the current frequency and modulation.
 * - Finally sets isPaused to FALSE indicating filters are not paused.
 *
 * @return Returns RMF_SECTFLT_Success on success else returns appropriate rmf_Error codes.
 */
rmf_Error rmf_InbSiMgr::resume(void)
{
	rmf_Error result;
	rmf_Error ret = RMF_INBSI_SUCCESS;
	std::list<rmf_FilterInfo*>::iterator filter;
	rmf_osal_mutexAcquire(g_sitp_psi_mutex);
	if(isPaused)
	{
		for(filter = m_filter_list.begin();filter != m_filter_list.end();filter++)
		{
			result = m_pInbSectionFilter->resume((*filter)->filterId);
			if(RMF_SECTFLT_Success != result)
			{
				ret = result;
			}
		}
		isPaused = FALSE;
	}
	rmf_osal_mutexRelease(g_sitp_psi_mutex);
	return ret;
}
