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
 

#include "rmf_sicache.h"

#if USE_SYSRES_MLT
#include "rpl_malloc.h"
#include "rpl_new.h"
#endif


/**
 * @file rmf_sicache.cpp
 * @brief Service information database
 */


rmf_InbSiCache *rmf_InbSiCache::m_pSiCache = NULL;
uint32_t rmf_InbSiCache::cache_count = 0;

rmf_InbSiCache::rmf_InbSiCache()
{
    rmf_Error err = RMF_INBSI_SUCCESS;

    g_si_ts_entry = NULL;

    g_global_si_mutex = NULL;

    g_global_si_cond = NULL;
    
    g_cond_counter = 0;

    /* Create global mutex */
    err = rmf_osal_mutexNew(&g_global_si_mutex);
    if (err != RMF_INBSI_SUCCESS)
    {
        RDK_LOG(RDK_LOG_WARN, "LOG.RDK.SI",
                "<%s() Mutex creation failed, returning MPE_EMUTEX\n", __FUNCTION__);
        //return RMF_OSAL_EMUTEX;
    }

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI", "<%s><%d>: g_global_si_mutex: 0x%x\n", __FUNCTION__, __LINE__, g_global_si_mutex);
/*
     Global condition used to lock the SIDB.
     Used by readers and the writer to acquire
     exclusive lock.
     Initial state is set to TRUE. It means the condition is available
     to be acquired.
     'autoReset' is set to TRUE also, to indicate once a lock
     is acquired by a reader/writer it stays in that state until
     the lock is released by that reader/writer.
     */
    if (rmf_osal_condNew(TRUE, TRUE, &g_global_si_cond) != RMF_INBSI_SUCCESS)
    {
        //return RMF_OSAL_EMUTEX;
    }
}

rmf_InbSiCache::~rmf_InbSiCache()
{
    rmf_SiTransportStreamEntry *walker, *next;

    /* Clear the database entries */

    /* Use internal method release_si_entry() to delete service entries */
    for (walker = g_si_ts_entry; walker; walker = next)
    {
        next = walker->next;
        release_transport_stream_entry(walker);
    }
    
    g_si_ts_entry = NULL;

    /* delete mutex */
    rmf_osal_mutexAcquire(g_global_si_mutex);
    rmf_osal_mutexRelease(g_global_si_mutex);
    rmf_osal_mutexDelete(g_global_si_mutex);

    /* delete global condition object */
    rmf_osal_condDelete(g_global_si_cond);
}


/**
 * @brief This functionality is used to get inband SI cache.
 * If SiCache doesnot exist, it will create a new instance of rmf_InbSiCache.
 * Otherwise, it will return the exsiting sicache.
 *
 * @return rmf_Inbsicache
 * @retval m_pSiCache Returns a pointer to the instance of Inbsicache.
 */
rmf_InbSiCache *rmf_InbSiCache::getInbSiCache()
{
    if(m_pSiCache==NULL)
    {
           RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                       "%s(%d): Creating cache\n", __FUNCTION__, __LINE__);
        m_pSiCache = new rmf_InbSiCache();
        cache_count=0;
    }
    
       RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                       "%s(%d): cache_count: %d\n", __FUNCTION__, __LINE__, cache_count);
    cache_count++;

    return m_pSiCache;
}


/**
 * @brief It is used to delete the cache[data base] for InbandSI if its not used.
 * It decrements the cache count and after deleting the cache, it sets sicache to NULL.
 *
 * @return None.
 */
void rmf_InbSiCache::clearCache()
{
    cache_count--;

   RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                       "%s(%d): cache_count: %d\n", __FUNCTION__, __LINE__, cache_count);
    if(cache_count==0)
    {
       RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                       "%s(%d): deleting cache\n", __FUNCTION__, __LINE__);
        delete m_pSiCache;
        m_pSiCache = NULL;
    }
}


/**
 * @brief Synchronize Read/Write access to SI Cache.
 * SICache can be locked by multiple clients concurrently.
 * If any client has the lock for read, any lock for write (see @LockFroWrite()) will be blocked.
 * If any write lock is being held, this function will be blocked.
 * UnlockForRead() must be called to release the read lock.
 *
 * @param[in] void  No arguments required.
 *
 * @retval RMF_INBSI_SUCCESS If successfully acquired the lock for reading the cache information.
 */
rmf_Error rmf_InbSiCache::LockForRead(void)
{
    rmf_Error result = RMF_INBSI_SUCCESS;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<%s> \n", __FUNCTION__);

    /* Acquire global mutex */
    rmf_osal_mutexAcquire(g_global_si_mutex);

    /*
     For read only purposes acquire the lock if no module is
     currently reading. If some module is reading, just increment
     the counter.
     */
    if (g_cond_counter == 0)
    {
        rmf_osal_condGet(g_global_si_cond);
    }

    /* increment the counter in any case */
    g_cond_counter++;

    rmf_osal_mutexRelease(g_global_si_mutex);
    /* Release global mutex */

    //MPE_LOG(MPE_LOG_DEBUG, MPE_MOD_SI, "<mpe_siLockForRead> done\n");

    return result;
}


/**
 * @brief Each reader needs to acquire the read lock for inbandSiCache before reading.
 * If there is at least one reader reading the SI cache, the SICache will not be modified.
 * Multiple readers can read at the same time.
 *
 * @retval RMF_INBSI_SUCCESS If successfully released the lock.
 * @retval RMF_INBSI_LOCKING_ERROR Returned when a handle requested is already locked.
 */
rmf_Error rmf_InbSiCache::UnLock(void)
{
    rmf_Error result = RMF_INBSI_SUCCESS;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<%s> \n", __FUNCTION__);

    /*  Multiple modules can be reading at the
     same time from any given Si handle. The read
     counter is incremented every time a read
     is requested (by mpe_siLockForRead).
     The same counter is decremented when handle is
     released. If the read counter reaches zero then
     the condition is also released.
     */

    /* Acquire global mutex */
    rmf_osal_mutexAcquire(g_global_si_mutex);

    if (g_cond_counter == 0)
    {
        result = RMF_INBSI_LOCKING_ERROR;
    }
    else
    {
        g_cond_counter--;

        if (g_cond_counter == 0)
        {
            RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                    "Lock count has hit zero; releasing g_global_si_cond.\n");
            rmf_osal_condSet(g_global_si_cond);
        }
    }

    /* Release global mutex */
    rmf_osal_mutexRelease(g_global_si_mutex);

    return result;
}


/**
 * @brief Each writer needs to acquire the write lock for inbandSiCache before write.
 * Only 1 writer can write cache at any time.  No reader is allowed during writing.
 *
 * @return None
 */
void rmf_InbSiCache::LockForWrite(void)
{
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<%s> ...\n", __FUNCTION__);

    // SITP makes this call to acquire the global SIDB lock
    // block here until the condition object is set
    rmf_osal_condGet(g_global_si_cond);
}


/**
 * @brief It realeases the lock for writing.
 * SITP task makes the call to release the global SIDB lock.
 *
 * @return None
 */
void rmf_InbSiCache::ReleaseWriteLock(void)
{
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<%s> ...\n", __FUNCTION__);
    // SITP makes this call to release the global SIDB lock
    // release the lock
    rmf_osal_condSet(g_global_si_cond);
}

/*  Internal method to retrieve transport stream from the list given a frequency */
uintptr_t rmf_InbSiCache::get_transport_stream_handle(uint32_t freq,
        uint8_t mode)
{
    rmf_SiTransportStreamEntry *walker = NULL;
    uintptr_t retVal = RMF_INBSI_INVALID_HANDLE;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<%s> freq:%d, mode: %d\n", __FUNCTION__, freq, mode);

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<%s: %d> ..., g_si_ts_entry: 0x%x\n", __FUNCTION__, __LINE__, g_si_ts_entry);
    walker = g_si_ts_entry;
    while (walker)
    {
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<%s: %d> ..., walker->frequency: %d, freq: %d, walker->modulation_mode: %d, mode: %d\n", __FUNCTION__, __LINE__, walker->frequency, freq, walker->modulation_mode, mode);  
        if (walker->frequency == freq)
        {
            if (walker->modulation_mode == mode)
            {
                retVal = (uintptr_t) walker;
                RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<%s: %d> ..., Found transport stream handle for freq: %d, mode: %d, walker: 0x%x\n", __FUNCTION__, __LINE__, freq, mode, walker);
                break;
            }
        }

        walker = walker->next;
    }

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<%s: %d> returning retVal: 0x%x ...\n", __FUNCTION__, __LINE__, retVal);
    return retVal;
}


/**
 * @brief  Initialize a newly created transport stream entry with the default values.
 *
 * @param[in] ts_enrty Entry for the transport stream.
 * @param[in] Frequency Frequency value of the transport stream.
 *
 * @retval RMF_INBSI_SUCCESS If transport stream entry is successfully initialized.
 * @retval RMF_INBSI_INVALID_PARAMETER Returned when a transport stream entry is NULL.
 * @retval RMF_OSAL_EMUTEX If mutex creation is failed for initializing the transport stream entry.
 */
rmf_Error rmf_InbSiCache::init_transport_stream_entry(
        rmf_SiTransportStreamEntry *ts_entry, uint32_t Frequency)
{
    rmf_Error err = RMF_INBSI_SUCCESS;
    rmf_osal_TimeMillis timeMillis;

    if (ts_entry == NULL)
    {
        return RMF_INBSI_INVALID_PARAMETER;
    }

    ts_entry->programs = new list<rmf_SiProgramHandle>();
    ts_entry->services = new list<rmf_SiServiceHandle>();
    ts_entry->state = TSENTRY_NEW;
    ts_entry->frequency = Frequency;
    ts_entry->modulation_mode = 0;
    ts_entry->ts_id = TSID_UNKNOWN;
    ts_entry->ts_name = NULL;
    ts_entry->description = NULL;
    //ts_entry->ptime_transport_stream = gtime_transport_stream;
    rmf_osal_timeGetMillis(&(ts_entry->ptime_transport_stream));
    ts_entry->program_count = 0;
    ts_entry->siStatus = SI_NOT_AVAILABLE;
    ts_entry->pat_version = INIT_TABLE_VERSION; // default version
    ts_entry->pat_crc = 0;
    ts_entry->cvct_version = INIT_TABLE_VERSION; // default version
    ts_entry->cvct_crc = 0;
    ts_entry->check_crc = FALSE;
    ts_entry->pat_reference = 0;
    ts_entry->pat_program_list = NULL;
    ts_entry->next = NULL;
    ts_entry->pat_byte_array = NULL;
    ts_entry->list_lock = NULL;

    err = rmf_osal_mutexNew(&ts_entry->list_lock);
    if (err != RMF_SUCCESS)
    {
        RDK_LOG(RDK_LOG_WARN, "LOG.RDK.SI",
                "init_transport_stream_entry() Mutex creation failed, returning RMF_EMUTEX\n");

        return RMF_OSAL_EMUTEX;
    }

    return RMF_INBSI_SUCCESS;
}


rmf_SiTransportStreamHandle rmf_InbSiCache::create_transport_stream_handle(
        uint32_t freq, uint8_t mode)
{
    rmf_SiTransportStreamEntry *ts_entry = NULL;
    rmf_SiTransportStreamEntry *ts_walker = NULL;
    uintptr_t retVal = RMF_INBSI_INVALID_HANDLE;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<%s> freq: %d mode:%d\n", __FUNCTION__, freq,
            mode);

    if ((retVal = get_transport_stream_handle(freq, mode))
            == RMF_INBSI_INVALID_HANDLE)
    {
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<%s: %d> ... retVal == RMF_INBSI_INVALID_HANDLE, Creating new transport stream handle for freq: %d, mode: %d\n",
        __FUNCTION__, __LINE__, freq, mode);
        //RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "create_transport_stream_handle... \n");
        /* Create a new rmf_SiTransportStreamEntry node */
        if (RMF_INBSI_SUCCESS != rmf_osal_memAllocP(RMF_OSAL_MEM_SI_CACHE_INB,
                sizeof(rmf_SiTransportStreamEntry), (void **) &(ts_entry)))
        {
            RDK_LOG(
                    RDK_LOG_WARN,
                    "LOG.RDK.SI",
                    "<%s> Mem allocation failed, returning RMF_SI_OUT_OF_MEM...\n", __FUNCTION__);

            return retVal;
        }
        else
        {
            RDK_LOG(RDK_LOG_TRACE7, "LOG.RDK.SI", "ts created @ %p\n", ts_entry);
        }

        if (RMF_INBSI_SUCCESS != init_transport_stream_entry(ts_entry, freq))
        {
            RDK_LOG(RDK_LOG_TRACE7, "LOG.RDK.SI", "ts freed @ %p\n", ts_entry);
            rmf_osal_memFreeP(RMF_OSAL_MEM_SI_CACHE_INB, ts_entry);
            return (uintptr_t) NULL;
        }
        ts_entry->modulation_mode = mode;

        /* append new entry to the list */
        if ((g_si_ts_entry == NULL) || (g_si_ts_entry->frequency
                > ts_entry->frequency))
        {
            ts_entry->next = g_si_ts_entry;
            g_si_ts_entry = ts_entry;
        }
        else
        {
            int cnt=0;
            ts_walker = g_si_ts_entry;
            while (ts_walker)
            {
                if ((ts_walker->next == NULL) || (ts_walker->next->frequency
                        > ts_entry->frequency))
                {
                    ts_entry->next = ts_walker->next;
                    ts_walker->next = ts_entry;
                    break;
                }
                ts_walker = ts_walker->next;
            }
        }

        retVal = (uintptr_t) ts_entry;
    }
    else
    { // Dynamic entry created by tuning.
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<%s: %d> ...transport_stream_entry found. RetVal: 0x%x\n", __FUNCTION__, __LINE__, retVal);
        if (mode != 0)
        {
            if (((rmf_SiTransportStreamEntry *) retVal)->modulation_mode
                    == 0)
            {
                ((rmf_SiTransportStreamEntry *) retVal)->modulation_mode = mode;
            }
        }
    }
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<%s> exit 0x%x\n", __FUNCTION__, retVal);

    return retVal;
}

/**
 Initialize a newly created program info structure with default values
 */
rmf_Error rmf_InbSiCache::init_program_info(rmf_SiProgramInfo *pi)
{
    volatile rmf_Error retVal = RMF_INBSI_SUCCESS;

    if (pi == NULL)
    {
        retVal = RMF_INBSI_INVALID_PARAMETER;
    }
    else
    {
        // Start struct in a known state.
        pi->pmt_status = PMT_NOT_AVAILABLE;
        pi->program_number = (uint32_t) - 1;
        pi->pmt_pid = (uint32_t) - 1;
        pi->pmt_version = INIT_TABLE_VERSION; // default version
        pi->crc_check = false;
        pi->pmt_crc = 0;
        pi->saved_pmt_ref = NULL;
        pi->pmt_byte_array = NULL;
        pi->number_outer_desc = 0;
        pi->outer_descriptors = NULL;
        pi->number_elem_streams = 0;
        pi->elementary_streams = NULL;
        pi->pcr_pid = (uint32_t) - 1;
        pi->services = new list<rmf_SiServiceHandle>();
    }
    return retVal;
}


/**
 * @brief Internal method to create a new program info structure given a transport stream and program number.
 * It check for a valid transport stream and porogram number and initializes the program info structure with default values.
 *
 * @param[in] ts_entry Transport stream entry.
 * @param[in] prog_num Program Number of transport stream.
 *
 * @return pi rmf_SiProgramInfo
 * @retval pi Points to the reference of the rmf_SIProgramInfo.
 * @retval NULL If transport stream entry is not available or memory allocation has failed for programInfo structure.
 */
rmf_SiProgramInfo * rmf_InbSiCache::create_new_program_info(
        rmf_SiTransportStreamEntry *ts_entry, uint32_t prog_num)
{
    rmf_SiProgramInfo *pi = NULL;
    //rmf_SiProgramInfo pi;

    if (ts_entry == NULL)
    {
        RDK_LOG(RDK_LOG_WARN, "LOG.RDK.SI",
                "<create_new_program_info> Cannot create program for NULL transport stream!\n");
    }
    else
    {
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                "<create_new_program_info> Program number: %d for TS %d!\n",
                prog_num, ts_entry->frequency);

        if (prog_num > 65535)
        {
            RDK_LOG(
                    RDK_LOG_WARN,
                    "LOG.RDK.SI",
                    "<create_new_program_info> %d is not a valid program number!!\n",
                    prog_num);
        }
        else
        {
            if (RMF_INBSI_SUCCESS != rmf_osal_memAllocP(RMF_OSAL_MEM_SI_CACHE_INB,
                    sizeof(rmf_SiProgramInfo), (void **) &(pi)))
            {
                RDK_LOG(RDK_LOG_WARN, "LOG.RDK.SI",
                        "<create_new_program_info> Mem allocation failed, returning NULL...\n");
            }
            else
            {
                if (init_program_info(pi) != RMF_INBSI_SUCCESS)
                {
                    RDK_LOG(RDK_LOG_WARN, "LOG.RDK.SI",
                            "<create_new_program_info> Initialization failed, returning NULL...\n");
                    rmf_osal_memFreeP(RMF_OSAL_MEM_SI_CACHE_INB, pi);
                    pi = NULL;
                }
                else
                {
                    //LINK *lp;
                    //pi->program_number = prog_num;
                    //pi->stream = ts_entry;
                    pi->program_number = prog_num;
                    pi->stream = ts_entry;
                    //lp = llist_mklink((void *) pi);
                    //llist_append(ts_entry->programs, lp);
                    //ts_entry->programs.push_back(*pi);
                    ts_entry->programs->push_back((rmf_SiProgramHandle)pi);
                    ts_entry->program_count++;
                    RDK_LOG(
                            RDK_LOG_TRACE1,
                            "LOG.RDK.SI",
                            "<create_new_program_info> ts_entry->program_count %d\n",
                            ts_entry->program_count);
                }
            }
        }
    }
    return pi;
}

rmf_osal_Bool rmf_InbSiCache::find_program_in_ts_pat_programs(rmf_SiPatProgramList *pat_list,
        uint32_t pn)
{
    rmf_SiPatProgramList *walker = (rmf_SiPatProgramList *) pat_list;

    while (walker)
    {
        if (walker->programNumber == pn)
        {
            return TRUE;
        }
        walker = walker->next;
    }

    return FALSE;
}

/*
 Internal method to append elementary stream to the end of existing
 elementary stream list within a service
 */
void rmf_InbSiCache::append_elementaryStream(rmf_SiProgramInfo *pi,
        rmf_SiElementaryStreamList *new_elem_stream)
{
    rmf_SiElementaryStreamList *elem_list;

    /* Populating elementary stream info */
    elem_list = pi->elementary_streams;

    if (elem_list == NULL)
    {
        pi->elementary_streams = new_elem_stream;
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
    (pi->number_elem_streams)++;
}

void rmf_InbSiCache::mark_elementary_stream(rmf_SiElementaryStreamList *elem_stream)
{
    if (elem_stream == NULL)
    {
        return;
    }

    elem_stream->valid = FALSE;
}

/*
 Internal method to remove elementary stream from the existing
 elementary stream list within a service
 */
rmf_Error rmf_InbSiCache::remove_elementaryStream(rmf_SiElementaryStreamList **elem_stream_list_head,
        rmf_SiElementaryStreamList *rmv_elem_stream)
{
    int ret = RMF_INBSI_INVALID_PARAMETER;
    rmf_SiElementaryStreamList *elem_list;
    if (rmv_elem_stream == NULL || *elem_stream_list_head == NULL)
    {
        return ret;
    }
    if (*elem_stream_list_head == rmv_elem_stream)
    {
        *elem_stream_list_head = rmv_elem_stream->next;
        ret = RMF_INBSI_SUCCESS;
    }
    else
    {
        elem_list = (*elem_stream_list_head)->next;
        while (elem_list)
        {
            if (elem_list != rmv_elem_stream)
            {
                elem_list = elem_list->next;
            }
            else
            {
                elem_list->next = rmv_elem_stream->next;
                ret = RMF_INBSI_SUCCESS;
                break;
            }
        }
    }
    return ret;
}

void rmf_InbSiCache::delete_elementary_stream(rmf_SiElementaryStreamList *elem_stream)
{
    /* Parameter check */
    if (elem_stream == NULL)
    {
        return;
    }

    // No more references left, delete the elementary stream
    strcpy(elem_stream->associated_language, "");
    elem_stream->es_info_length = 0;
    elem_stream->elementary_PID = 0;
    elem_stream->service_comp_type = (rmf_SiServiceComponentType)0;

    // De-allocate service component name
    if (elem_stream->service_component_names != NULL)
    {
        rmf_SiLangSpecificStringList *walker =
                elem_stream->service_component_names;

        while (walker != NULL)
        {
            rmf_SiLangSpecificStringList *toBeFreed = walker;

            rmf_osal_memFreeP(RMF_OSAL_MEM_SI_CACHE_INB, walker->string);

            walker = walker->next;

            rmf_osal_memFreeP(RMF_OSAL_MEM_SI_CACHE_INB, toBeFreed);
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

    rmf_osal_memFreeP(RMF_OSAL_MEM_SI_CACHE_INB, elem_stream);
}

/**
 *  Internal Method to release elementary stream list within service entries
 *
 * <i>release_elementary_streams()</i>
 *
 * @param eStream is the elementary stream list to delete
 *
 *
 */
void rmf_InbSiCache::release_elementary_streams(rmf_SiElementaryStreamList *eStream)
{
    rmf_SiElementaryStreamList *tempEStreamPtr = eStream;
    rmf_SiElementaryStreamList *tempEStreamNextPtr = NULL;
    rmf_osal_Bool deleteEStrem = FALSE;

    while (tempEStreamPtr != NULL)
    {
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                "<%s> ref_count: %d\n",
                __FUNCTION__, tempEStreamPtr->ref_count);

        tempEStreamNextPtr = tempEStreamPtr->next;

        if (tempEStreamPtr->ref_count == 1)
        {
            RDK_LOG(
                    RDK_LOG_TRACE1,
                    "LOG.RDK.SI",
                    "<%s> deleting elementary steam 0x%p\n",
                    __FUNCTION__, tempEStreamPtr);

            strcpy(tempEStreamPtr->associated_language, "");
            tempEStreamPtr->es_info_length = 0;
            tempEStreamPtr->elementary_PID = 0;
            tempEStreamPtr->service_comp_type = (rmf_SiServiceComponentType)0;

            /* De-allocate service component name */
            if (tempEStreamPtr->service_component_names != NULL)
            {
                rmf_SiLangSpecificStringList *walker =
                        tempEStreamPtr->service_component_names;

                while (walker != NULL)
                {
                    rmf_SiLangSpecificStringList *toBeFreed = walker;

                    rmf_osal_memFreeP(RMF_OSAL_MEM_SI_CACHE_INB, walker->string);

                    walker = walker->next;

                    rmf_osal_memFreeP(RMF_OSAL_MEM_SI_CACHE_INB, toBeFreed);
                }

                tempEStreamPtr->service_component_names = NULL;
            }

            /* De-allocate descriptors */
            if (tempEStreamPtr->descriptors != NULL)
            {
                release_descriptors(tempEStreamPtr->descriptors);
                tempEStreamPtr->descriptors = NULL;
            }

            deleteEStrem = TRUE;
            tempEStreamPtr->number_desc = 0;
        }
        else if (tempEStreamPtr->ref_count > 1)
        {
            // Send event and mark it.
            mark_elementary_stream(tempEStreamPtr);

            tempEStreamPtr->ref_count++;

            /*send_service_component_event(tempEStreamPtr,
                    RMF_INBSI_CHANGE_TYPE_REMOVE);*/
        }

        if (deleteEStrem == TRUE)
        {
            rmf_osal_memFreeP(RMF_OSAL_MEM_SI_CACHE_INB, tempEStreamPtr);
            tempEStreamPtr = NULL;
            deleteEStrem = FALSE;
        }
        tempEStreamPtr = tempEStreamNextPtr;
    }
}

/**
 *  Internal Method to release descriptor list within service entries
 *
 * <i>release_descriptors()</i>
 *
 * @param desc is the descriptor list to delete
 *
 *
 */
void rmf_InbSiCache::release_descriptors(rmf_SiMpeg2DescriptorList *desc)
{
    rmf_SiMpeg2DescriptorList *tempDescPtr = desc;
    rmf_SiMpeg2DescriptorList *tempDescNextPtr = NULL;

    while (tempDescPtr != NULL)
    {
        tempDescPtr->tag = (rmf_SiDescriptorTag)0;
        tempDescPtr->descriptor_length = 0;

        //RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<release_descriptors> deleting descriptor 0x%x\n", tempDescPtr);

        if (tempDescPtr->descriptor_content)
        {
            rmf_osal_memFreeP(RMF_OSAL_MEM_SI_CACHE_INB, tempDescPtr->descriptor_content);
            tempDescPtr->descriptor_content = NULL;
        }
        tempDescNextPtr = tempDescPtr->next;
        rmf_osal_memFreeP(RMF_OSAL_MEM_SI_CACHE_INB, tempDescPtr);
        tempDescPtr = tempDescNextPtr;
    }
}


/*
 Internal method to get the service component type based on the elementary
 stream type.
 Service component types are generic types (ex: AUDIO, VIDEO, DATA etc.)
 Elementary stream types are specific (ex: MPEG1_AUDIO, MPEG2_VIDEO, RMF_SI_ELEM_DSM_CC_SECTIONS etc.)
 Hence if the elementary stream type is known the corresponding service
 component type id derived from it.
 */
void rmf_InbSiCache::get_serviceComponentType(rmf_SiElemStreamType stream_type,
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


// program info handle specific
/**
 * @brief It retrieves pmt version from the given program info handle.
 *
 * @param[in] program_handle si handle to get the pmt version from.
 * @param[out] pmt_version Output parameter to populate pmt version.
 *
 * @retval RMF_INBSI_SUCCESS If successfully retrieved pmt version.
 * @retval RMF_INBSI_INVALID_PARAMETER Returned when program_handle is invalid or pmt_version is NULL.
 * @retval RMF_INBSI_PMT_NOT_AVAILABLE Returned when the pmt info is not available.
 * @retval RMF_INBSI_PMT_NOT_AVAILABLE_YET Returned when PMT is not yet available, and will be available shortly.
 */
rmf_Error rmf_InbSiCache::GetPMTVersionForProgramHandle(
    rmf_SiProgramHandle program_handle, uint32_t* pmt_version)
{
    rmf_SiProgramInfo *pi = (rmf_SiProgramInfo *) program_handle;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<%s> program_handle:0x%x\n",
            __FUNCTION__,program_handle);

    /* Parameter check */
    if ((program_handle == RMF_INBSI_INVALID_HANDLE) || (pmt_version == NULL))
    {
        return RMF_INBSI_INVALID_PARAMETER;
    }

    *pmt_version = pi->pmt_version;

    /* If PSI is in the process of being acquired return from here
     until it is updated (disable acquiring lock) */
    if (pi->pmt_status == PMT_NOT_AVAILABLE)
    {
        return RMF_INBSI_PMT_NOT_AVAILABLE;
    }
    else if (pi->pmt_status == PMT_AVAILABLE_SHORTLY)
    {
        return RMF_INBSI_PMT_NOT_AVAILABLE_YET;
    }

    return RMF_INBSI_SUCCESS;

}


/**
 * @brief It retrieves pmt crc value from the given program handle.
 *
 * @param[in] program_handle program handle to get the pmt crc from.
 * @param[out] crc Output parameter to populate pmt crc.
 *
 * @retval RMF_INBSI_SUCCESS If successfully retrieved pmt version.
 * @retval RMF_INBSI_INVALID_PARAMETER Returned when program_handle is invalid or pmt_crc is NULL.
 * @retval RMF_INBSI_PMT_NOT_AVAILABLE Returned when the pmt info is not available.
 * @retval RMF_INBSI_PMT_NOT_AVAILABLE_YET Returned when PMT is not yet available, and will be available shortly.
 */
rmf_Error rmf_InbSiCache::GetPMTCRCForProgramHandle(rmf_SiProgramHandle program_handle,
    uint32_t* pmt_crc)
{
    rmf_SiProgramInfo *pi = (rmf_SiProgramInfo *) program_handle;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<%s> program_handle:0x%x\n",
            __FUNCTION__,program_handle);

    /* Parameter check */
    if ((program_handle == RMF_INBSI_INVALID_HANDLE) || (pmt_crc == NULL))
    {
        return RMF_INBSI_INVALID_PARAMETER;
    }

    *pmt_crc = pi->pmt_crc;

    /* If PSI is in the process of being acquired return from here
     until it is updated (disable acquiring lock) */
    if (pi->pmt_status == PMT_NOT_AVAILABLE)
    {
        return RMF_INBSI_PMT_NOT_AVAILABLE;
    }
    else if (pi->pmt_status == PMT_AVAILABLE_SHORTLY)
    {
        return RMF_INBSI_PMT_NOT_AVAILABLE_YET;
    }

    return RMF_INBSI_SUCCESS;
}


// transport stream handle specific
/**
 * @brief It retrieves the transport stream id for a given transport stream handle.
 * OCAP spec allows a default tsId to be returned if SI is in the process of being acquired.
 *
 * @param[in] ts_handle Transport stream handle to get the ts id.
 * @param[out] ts_id Output parameter to populate the ts id.
 *
 * @retval RMF_INBSI_SUCCESS If successfully retrieved the ts id.
 * @retval RMF_INBSI_INVALID_PARAMETER Returned when transport stream handle is NULL or ts_id is NULL.
 * @retval RMF_INBSI_PAT_NOT_AVAILABLE_YET Returned when Si information for the requested transport stream entry is not yet acquired.
 */
rmf_Error rmf_InbSiCache::GetTransportStreamIdForTransportStreamHandle(
    rmf_SiTransportStreamHandle ts_handle, uint32_t *ts_id)
{
    rmf_SiTransportStreamEntry *walker = NULL;

    /* Parameter check */
    if ((ts_handle == RMF_INBSI_INVALID_HANDLE) || (ts_id == NULL))
    {
        return RMF_INBSI_INVALID_PARAMETER;
    }

    RDK_LOG(
            RDK_LOG_TRACE1,
            "LOG.RDK.SI",
            "<%s> ts_handle:0x%x\n",
            __FUNCTION__, ts_handle);

    walker = (rmf_SiTransportStreamEntry *) ts_handle;

    *ts_id = TSID_UNKNOWN;

    // OCAP spec allows a default tsId to be returned if SI is in
    // the process of being acquired
    *ts_id = walker->ts_id;

    if (walker->ts_id == TSID_UNKNOWN
        && walker->siStatus == SI_AVAILABLE_SHORTLY)
        //&& walker->modulation_mode != MPE_SI_MODULATION_QAM_NTSC)
    {
        return RMF_INBSI_PAT_NOT_AVAILABLE_YET; // Only return this for digital streams
    }

    return RMF_INBSI_SUCCESS;
}


/**
 * @brief It retrieves the description for a given transport stream handle.
 *
 * @param[in] ts_handle Transport stream handle to get the description of TS.
 * @param[out] description Output parameter to populate the description.
 *
 * @retval RMF_INBSI_SUCCESS If successfully retrieved the description for transport stream.
 * @retval RMF_INBSI_INVALID_PARAMETER Returned when a transport stream handle requested is invalid or description is NULL.
 */
rmf_Error rmf_InbSiCache::GetDescriptionForTransportStreamHandle(
    rmf_SiTransportStreamHandle ts_handle, char **description)
{
    rmf_SiTransportStreamEntry *walker = NULL;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<%s> ts_handle:0x%x\n",
            __FUNCTION__, ts_handle);

    /* Parameter check */
    if ((ts_handle == RMF_INBSI_INVALID_HANDLE) || (description == NULL))
    {
        return RMF_INBSI_INVALID_PARAMETER;
    }

    walker = (rmf_SiTransportStreamEntry *) ts_handle;
    *description = walker->description;

    return RMF_INBSI_SUCCESS;
}


/**
 * @brief It retrieves the transport stream name for a given transport stream handle.
 *
 * @param[in] ts_handle Transport stream handle to get the name of TS.
 * @param[out] ts_name Output parameter to populate the name.
 *
 * @retval RMF_SUCCESS If successfully retrieved the name for the transport stream handle.
 * @retval RMF_INBSI_INVALID_PARAMETER Returned when a transport stream handle requested is invalid or transport stream name is NULL.
 */
rmf_Error rmf_InbSiCache::GetTransportStreamNameForTransportStreamHandle(
    rmf_SiTransportStreamHandle ts_handle, char **ts_name)
{
    rmf_SiTransportStreamEntry *walker = NULL;

    RDK_LOG(
            RDK_LOG_TRACE1,
            "LOG.RDK.SI",
            "<%s> ts_handle:0x%x\n",
            __FUNCTION__, ts_handle);

    /* Parameter check */
    if ((ts_handle == RMF_INBSI_INVALID_HANDLE) || (ts_name == NULL))
    {
        return RMF_INBSI_INVALID_PARAMETER;
    }

    walker = (rmf_SiTransportStreamEntry *) ts_handle;
    *ts_name = walker->ts_name;

    return RMF_SUCCESS;
}


/**
 * @brief It retrieves frequency for a given transport stream handle.
 *
 * @param[in] ts_handle Transport stream handle to get the frequency of TS.
 * @param[out] freq Output parameter to populate frequency.
 *
 * @retval RMF_INBSI_SUCCESS If successfully retrieved the frequency for the given ts handle.
 * @retval RMF_INBSI_INVALID_PARAMETER Returned when a transport stream hanlde requested is invalid or frequency is NULL.
 */
rmf_Error rmf_InbSiCache::GetFrequencyForTransportStreamHandle(
    rmf_SiTransportStreamHandle ts_handle, uint32_t* freq)
{
    rmf_SiTransportStreamEntry *walker = NULL;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<%s> ts_handle:0x%x\n",
            __FUNCTION__, ts_handle);
    /* Parameter check */
    if ((ts_handle == RMF_INBSI_INVALID_HANDLE) || (freq == NULL))
    {
        return RMF_INBSI_INVALID_PARAMETER;
    }

    *freq = 0;

    walker = (rmf_SiTransportStreamEntry *) ts_handle;

    *freq = walker->frequency;
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "%s()...freq = %d\n",
            __FUNCTION__, walker->frequency);

    return RMF_INBSI_SUCCESS;
}


/**
 * @brief It retrieves modulation format for a given transport stream handle.
 *
 * @param[in] ts_handle Transport stream handle to get the modulation format.
 * @param[out] mode Output parameter to populate the mode.
 *
 * @retval RMF_INBSI_SUCCESS If successfully retrieved the modulation format for the given transport stream handle.
 * @retval RMF_INBSI_INVALID_PARAMETER Returned when a transport stream handle requested is invalid or modulation mode is NULL.
 */
rmf_Error rmf_InbSiCache::GetModulationFormatForTransportStreamHandle(
    rmf_SiTransportStreamHandle ts_handle, uint8_t *mode)
{
    rmf_SiTransportStreamEntry *walker = NULL;

    /* Parameter check */
    if ((ts_handle == RMF_INBSI_INVALID_HANDLE) || (mode == NULL))
    {
        return RMF_INBSI_INVALID_PARAMETER;
    }

    walker = (rmf_SiTransportStreamEntry *) ts_handle;

    *mode = walker->modulation_mode;

    RDK_LOG(
            RDK_LOG_TRACE1,
            "LOG.RDK.SI",
            "<%s> returning mode:%d\n",
            __FUNCTION__, *mode);

    return RMF_INBSI_SUCCESS;
}


/**
 * @brief It retrieves the transport stream service information type from the given transport stream handle.
 * Service information types can be ATSC, DVB, SCTE or unknown.
 *
 * @param[in] ts_handle Transport stream handle to get the service information type.
 * @param[out] service_info_type Output parameter to populate the transport stream service info type.
 *
 * @retval RMF_INBSI_SUCCESS If successfully retrieved the service information type.
 * @retval RMF_INBSI_INVALID_PARAMETER Returned when a transport stream handle requested is invalid or service information type is NULL.
 */
rmf_Error rmf_InbSiCache::GetTransportStreamServiceInformationType(
    rmf_SiTransportStreamHandle ts_handle,
    rmf_SiServiceInformationType* service_info_type)
{
    /* Parameter check */
    if ((ts_handle == RMF_INBSI_INVALID_HANDLE) || (service_info_type == NULL))
    {
        return RMF_INBSI_INVALID_PARAMETER;
    }

    RDK_LOG(
            RDK_LOG_TRACE1,
            "LOG.RDK.SI",
            "<%s> ts_handle:0x%x\n",
            __FUNCTION__, ts_handle);
    *service_info_type = RMF_SI_SERVICE_INFORMATION_TYPE_SCTE_SI;

    return RMF_INBSI_SUCCESS;
}


/**
 * @brief It retrieves the transport stream's last update time for the given transport stream handle.
 *
 * @param[in] ts_handle Transport stream handle to get the last update time of ts.
 * @param[out] time Output parameter to populate the time transport stream was last updated.
 *
 * @retval RMF_SUCCESS If successfully retrieved the last update time for the ts handle.
 * @retval RMF_INBSI_INVALID_PARAMETER Returned when a transport stream hanlde requested is invalid or pTime is NULL.
 */
rmf_Error rmf_InbSiCache::GetTransportStreamLastUpdateTimeForTransportStreamHandle(
    rmf_SiTransportStreamHandle ts_handle, rmf_osal_TimeMillis *pTime)
{
    rmf_SiTransportStreamEntry *walker = NULL;

    /* Parameter check */
    if ((ts_handle == RMF_INBSI_INVALID_HANDLE) || (pTime == NULL))
    {
        return RMF_INBSI_INVALID_PARAMETER;
    }

    walker = (rmf_SiTransportStreamEntry *) ts_handle;
    *pTime = walker->ptime_transport_stream;

    return RMF_SUCCESS;
}


/**
 * @breif It retrieves PAT crc value for a given transport stream handle.
 *
 * @param[in] service_handle si handle to get the pat crc from.
 * @param[out] crc  Output parameter to populate pat crc.
 *
 * @retval RMF_INBSI_SUCCESS If successfully retrieved pat crc value.
 * @retval RMF_INBSI_INVALID_PARAMETER Returned when a transport stream handle requested is invalid or pat_crc is NULL.
 * @retval RMF_INBSI_PAT_NOT_AVAILABLE Returned when PAT info is not available.
 * @retval RMF_INBSI_PAT_NOT_AVAILABLE_YET Returned when PAT is not yet available, and will be available shortly.
 */
rmf_Error rmf_InbSiCache::GetPATCRCForTransportStreamHandle(
    rmf_SiTransportStreamHandle ts_handle, uint32_t* pat_crc)
{
    rmf_SiTransportStreamEntry *ts_entry = NULL;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<%s> ts_handle:0x%x\n", __FUNCTION__, ts_handle);
    /* Parameter check */
    if ((ts_handle == RMF_INBSI_INVALID_HANDLE) || (pat_crc == NULL))
    {
        return RMF_INBSI_INVALID_PARAMETER;
    }

    ts_entry = (rmf_SiTransportStreamEntry *) ts_handle;

    *pat_crc = 0;

    /* If PAT is in the process of being acquired return from here
     until it is updated (disable acquiring lock) */
    *pat_crc = ts_entry->pat_crc;

    if (ts_entry->siStatus == SI_NOT_AVAILABLE)
    {
        return RMF_INBSI_PAT_NOT_AVAILABLE;
    }
    else if (ts_entry->siStatus == SI_AVAILABLE_SHORTLY)
    {
        return RMF_INBSI_PAT_NOT_AVAILABLE_YET;
    }

    return RMF_INBSI_SUCCESS;
}


/**
 * @brief It retrieves PAT version for a given transport stream handle.
 *
 * @param[in] ts_handle ts handle to get the pat version from.
 * @param[out] pat_version Output parameter to populate pat version.
 *
 * @retval RMF_INBSI_SUCCESS If successfully retrieved pat version.
 * @retval RMF_INBSI_INVALID_PARAMETER Returned when a transport stream handle requested is invalid or pat version is NULL.
 * @retval RMF_INBSI_PAT_NOT_AVAILABLE Returned when PAT info is not available.
 * @retval RMF_INBSI_PAT_NOT_AVAILABLE_YET Returned when PAT is not yet available, and will be available shortly.
 */
rmf_Error rmf_InbSiCache::GetPATVersionForTransportStreamHandle(
    rmf_SiTransportStreamHandle ts_handle, uint32_t* patVersion)
{
    rmf_SiTransportStreamEntry *ts_entry = NULL;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<%s> ts_handle:0x%x\n",
            __FUNCTION__, ts_handle);

    /* Parameter check */
    if ((ts_handle == RMF_INBSI_INVALID_HANDLE) || (patVersion == NULL))
    {
        return RMF_INBSI_INVALID_PARAMETER;
    }

    ts_entry = (rmf_SiTransportStreamEntry *) ts_handle;

    *patVersion = ts_entry->pat_version;

    /* If PAT is in the process of being acquired return from here
     until it is updated (disable acquiring lock) */
    RDK_LOG(
            RDK_LOG_TRACE1,
            "LOG.RDK.SI",
            "<%s> returning patVersion:%d\n",
            __FUNCTION__, *patVersion);

    if (ts_entry->siStatus == SI_NOT_AVAILABLE)
    {
        return RMF_INBSI_PAT_NOT_AVAILABLE;
    }
    else if (ts_entry->siStatus == SI_AVAILABLE_SHORTLY)
    {
        return RMF_INBSI_PAT_NOT_AVAILABLE_YET;
    }

    return RMF_INBSI_SUCCESS;
}


/**
 * @brief It retrieves the transport stream entry list if it exists for the particular frequency
 * else it creates a new transport stream entry with default values.
 *
 * @param[in] freq Frequency to get the transport stream entry list.
 * @param[in] mode Modulation mode to get the transport stream entry list.
 * @param[out] ts_handle Output parameter to populate the transport stream entry list.
 *
 * @retval RMF_INBSI_SUCCESS If successfully retrieved the transport stream entry.
 * @retval RMF_INBSI_INVALID_PARAMETER Returned when a transport stream handle requested is invalid .
 * @retval RMF_INBSI_OUT_OF_MEM Returned when a transport stream handle requested is NULL.
 */
rmf_Error rmf_InbSiCache::GetTransportStreamEntryFromFrequencyModulation(uint32_t freq,
    uint8_t mode, rmf_SiTransportStreamHandle *ts_handle)
{
    rmf_SiTransportStreamEntry *ts = NULL;

    /* Parameter check */
    if (ts_handle == NULL)
    {
        return RMF_INBSI_INVALID_PARAMETER;
    }

    // create_transport_stream_handle() will return an existing ts, if it can, create new if it can't.
    ts = (rmf_SiTransportStreamEntry *) create_transport_stream_handle(freq,
            mode);
    if (ts == NULL)
    {
        RDK_LOG(
                RDK_LOG_WARN,
                "LOG.RDK.SI",
                "<%s> - Mem allocation (ts) failed, returning RMF_SI_OUT_OF_MEM...\n", __FUNCTION__);
        return RMF_INBSI_OUT_OF_MEM;
    }
    *ts_handle = (rmf_SiTransportStreamHandle) ts;

    return RMF_INBSI_SUCCESS;
}


/**
 * @brief It retrieves the program entry from the given frequency, program number and modulation mode.
 * Creating new TS entry if it doesnot exist and getting the program info from the corresponding transport stream entry.
 *
 * @param[in] frequency Frequency of TS to get the program entry.
 * @param[in] program_number Program number to get the program entry.
 * @param[in] mode Modulation mode.
 * @param[out] prog_handle Output parameter to populate the Program entry.
 *
 * @retval RMF_INBSI_SUCCESS If successfully retrieved program entry.
 * @retval RMF_INBSI_INVALID_PARAMETER Returned when a program handle requested is NULL or program number is exceeded or transport stream handle is invalid.
 * @retval RMF_INBSI_OUT_OF_MEM Returned when a memory allocation is failed to create a transport stream handle.
 */
rmf_Error rmf_InbSiCache::GetProgramEntryFromFrequencyProgramNumberModulation(
    uint32_t frequency, uint32_t program_number, uint8_t mode,
    rmf_SiProgramHandle *prog_handle)
{
    rmf_SiTransportStreamHandle ts_handle = RMF_INBSI_INVALID_HANDLE;
    rmf_Error retVal = RMF_INBSI_SUCCESS;

    if (prog_handle == NULL)
    {
        retVal = RMF_INBSI_INVALID_PARAMETER;
    }
    else
    {
        *prog_handle = RMF_INBSI_INVALID_HANDLE;

        if (program_number > 65535)
        {
            RDK_LOG(
                    RDK_LOG_WARN,
                    "LOG.RDK.SI",
                    "<%s> Bad parameter! %d %d %d %p...\n",
                    __FUNCTION__, frequency, program_number, mode, prog_handle);
            retVal = RMF_INBSI_INVALID_PARAMETER;
        }
        else
        {
            // create_transport_stream_handle() will return an existing ts, if it can, create new if it can't.
            ts_handle = (rmf_SiTransportStreamHandle) create_transport_stream_handle(
                    frequency, mode);
            if (ts_handle == RMF_INBSI_INVALID_HANDLE)
            {
                RDK_LOG(
                        RDK_LOG_WARN,
                        "LOG.RDK.SI",
                        "<%s> Mem allocation (ts) failed, returning RMF_INBSI_OUT_OF_MEM...\n", __FUNCTION__);
                retVal = RMF_INBSI_OUT_OF_MEM;
            }
            else
            {
                retVal = GetProgramEntryFromTransportStreamEntry(
                        ts_handle, program_number, prog_handle);
            }
        }
    }
    return retVal;

}


/**
 * @brief It retrieves the program entry from the given transport stream entry.
 * If programInfo doesnot exist for the transport stream entry, it will create new programInfo using the program number.
 *
 * @param[in] ts_handle Transport stream handle to get the program entry.
 * @param[in] program_number Program number of the ts handle.
 * @param[out] prog_handle Output parameter to populate the program entry.
 *
 * @retval RMF_INBSI_SUCCESS If successfully retrieved program entry.
 * @retval RMF_INBSI_INVALID_PARAMETER Returned when a program_handle requested is NULL or transport stream handle is invalid or program number is exceeded.
 * @retval RMF_INBSI_OUT_OF_MEM Returned when a memory allocation failed while creating a new program info.
 */
rmf_Error rmf_InbSiCache::GetProgramEntryFromTransportStreamEntry(
    rmf_SiTransportStreamHandle ts_handle, uint32_t program_number,
    rmf_SiProgramHandle *prog_handle)
{
    rmf_SiTransportStreamEntry *ts = (rmf_SiTransportStreamEntry *) ts_handle;
    rmf_SiProgramInfo *pi = NULL;
    rmf_Error retVal = RMF_INBSI_SUCCESS;
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<%s> 0x%x %d ...\n",
            __FUNCTION__, ts_handle, program_number);

    if (prog_handle == NULL)
    {
        retVal = RMF_INBSI_INVALID_PARAMETER;
    }
    else
    {
        *prog_handle = RMF_INBSI_INVALID_HANDLE;
        /* other Parameter check */
        if ((ts_handle == RMF_INBSI_INVALID_HANDLE) || (program_number > 65535))
        {
            RDK_LOG(
                    RDK_LOG_WARN,
                    "LOG.RDK.SI",
                    "<%s> Bad parameter! %d %d...\n",
                    __FUNCTION__, ts_handle, program_number);
            retVal = RMF_INBSI_INVALID_PARAMETER;
        }
        else
        {
            //if (ts->modulation_mode != RMF_MODULATION_QAM_NTSC)
            {
                //if (lp == NULL)
                if(ts->programs->empty())
                {
                    pi = create_new_program_info(ts, program_number);
                    if(pi == NULL)
                    {
                        RDK_LOG(
                                RDK_LOG_WARN,
                                "LOG.RDK.SI",
                                "<%s> Mem allocation (pi) failed, returning RMF_SI_OUT_OF_MEM...\n",
                                __FUNCTION__);
                        retVal = RMF_INBSI_OUT_OF_MEM;
                    }
                }
                else
                {
                    //LINK *lp = llist_first(ts->programs);
                    list<rmf_SiProgramHandle>::iterator piter;
                    piter = ts->programs->begin();
                    while (piter != ts->programs->end())
                    {
                        //pi = (rmf_SiProgramInfo *) llist_getdata(lp);
                        // We have the writelock, so list manipulation is safe.
                        //if (pi)
                        {
                            //RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<mpe_siGetProgramEntryFromTransportStreamEntry> pi: 0x%x pi->program_number: %d...\n", pi, pi->program_number);
                            pi = (rmf_SiProgramInfo*)(*piter);
                            if (pi->program_number == program_number)
                            {
                                break;
                            }
                            pi = NULL;
                        }
                        //lp = llist_after(lp);
                        piter++;
                    }

                    if(pi == NULL)
                    {
#if 0
                        rmf_SiPatProgramList* pat_list = (rmf_SiPatProgramList*) ts->pat_program_list;

                        rmf_osal_Bool found = find_program_in_ts_pat_programs(pat_list, program_number);
                        if(found)
#endif
                        {
                            pi = create_new_program_info(ts, program_number);
                            if(pi == NULL)
                            {
                                RDK_LOG(
                                        RDK_LOG_WARN,
                                        "LOG.RDK.SI",
                                        "<%s> Mem allocation (pi) failed, returning RMF_SI_OUT_OF_MEM...\n",
                                        __FUNCTION__);
                                retVal = RMF_INBSI_OUT_OF_MEM;
                            }
                        }
#if 0
                        else
                        {
                                RDK_LOG(
                                        RDK_LOG_WARN,
                                        "LOG.RDK.SI",
                                        "<%s> Given program number not found in PAT\n",
                                        __FUNCTION__);
                                if((ts->siStatus == SI_NOT_AVAILABLE) ||(ts->siStatus == SI_AVAILABLE_SHORTLY))
                                {
                                retVal = RMF_INBSI_PMT_NOT_AVAILABLE;
                                }
                                else
                                {
                                    retVal = RMF_INBSI_INVALID_PARAMETER;
                                }
                                    RDK_LOG(
                                                                                RDK_LOG_TRACE1,
                                                                                "LOG.RDK.SI",
                                                                                "<%s> ts->siStatus: %d, retVal: 0x%x\n",
                                                                                __FUNCTION__);
    
                        }
#endif
                    }
                }

                if (pi != NULL)
                {
                    *prog_handle = (rmf_SiProgramHandle) pi;
                    RDK_LOG(
                            RDK_LOG_TRACE1,
                            "LOG.RDK.SI",
                            "<%s> prog_handle: 0x%p ...\n",
                            __FUNCTION__, pi);
                }

            }
        }
    }
    return retVal;
}


/* PAT */

/**
 * @brief This function is used to add the transport stream Id for the specified ts_handle.
 * @param[in] ts_handle Transport stream handle to set the ts_id.
 * @param[in] ts_id t Input parameter whose value is set to the transport stream id.
 *
 * @return None
 */
void rmf_InbSiCache::SetTSIdForTransportStream(rmf_SiTransportStreamHandle ts_handle,
    uint32_t tsId)
{
    rmf_SiTransportStreamEntry *ts_entry =
            (rmf_SiTransportStreamEntry *) ts_handle;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<%s> freq:%d, tsId:%d\n",
            __FUNCTION__, ts_entry->frequency, tsId);

    ts_entry->ts_id = tsId;
    ts_entry->siStatus = SI_AVAILABLE; /* indicates a valid tsId being set */
}


/**
 * @brief To set the PAT version for a given transport stream.
 * It saves the existing pat program list as a reference.
 * Then it detaches the pat program list from the transport stream and allowing to add new pat program list.
 *
 * @param[in] ts_handle Transport stream handle to set PAT version.
 * @param[in] pat_version PAT version.
 *
 * @return None
 */
void rmf_InbSiCache::SetPATVersionForTransportStream(
    rmf_SiTransportStreamHandle ts_handle, uint32_t pat_version)
{
    rmf_SiTransportStreamEntry *ts_entry =
            (rmf_SiTransportStreamEntry *) ts_handle;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<%s> freq:%d, version:%d\n",
            __FUNCTION__, ts_entry->frequency, pat_version);

    if (ts_entry->pat_version != pat_version)
    {
        // No need to check crc
        ts_entry->check_crc = FALSE;
    }
    else
    {
        // version same, check crc
        ts_entry->check_crc = TRUE;
    }

    // Save exisiting pat program list as a reference
    if (ts_entry->pat_program_list != NULL)
    {
        ts_entry->pat_reference = (uintptr_t) ts_entry->pat_program_list;
    }

    // Un-couple pat programs from the transport stream
    // Allow new pat programs to be added
    ts_entry->pat_program_list = NULL;
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<%s> done..\n", __FUNCTION__);
    ts_entry->pat_version = pat_version;
}

/**
 Internal method used to compare an existing PAT program list with a new
 PAT program list. Program numbers found in both PATs are marked as such.
 This is done mainly to identify newly added programs and programs that
 existed in the old PAT but are dropped in the new version.
 */
void rmf_InbSiCache::compare_ts_pat_programs(rmf_SiPatProgramList *new_pat_list,
        rmf_SiPatProgramList *saved_pat_list, rmf_osal_Bool setNewProgramFlag)
{
    rmf_SiPatProgramList *walker_saved_pat_program;
    rmf_SiPatProgramList *walker_new_pat_program = new_pat_list;

    if ((new_pat_list == NULL) || (saved_pat_list == NULL))
        return;

    // program numbers in the newly acquired PAT are compared
    // with program numbers in the saved PAT
    while (walker_new_pat_program)
    {
        walker_saved_pat_program = saved_pat_list;
        while (walker_saved_pat_program)
        {
            if ((walker_new_pat_program->programNumber
                    == walker_saved_pat_program->programNumber)
                    && (walker_new_pat_program->matched != TRUE))
            {
                walker_saved_pat_program->matched = TRUE;
                if (setNewProgramFlag)
                    walker_new_pat_program->matched = TRUE;
            }
            walker_saved_pat_program = walker_saved_pat_program->next;
        }
        walker_new_pat_program = walker_new_pat_program->next;
    }
}

/*
 Internal method to handle signaling PAT program removed event
 */
void rmf_InbSiCache::signal_program_removed_event(rmf_SiTransportStreamEntry *ts_entry,
        rmf_SiPatProgramList *pat_list)
{
    rmf_SiPatProgramList *walker_saved_pat_program =
            (rmf_SiPatProgramList *) pat_list;

    // Existing PAT program list
    while (walker_saved_pat_program)
    {
        if (walker_saved_pat_program->matched == FALSE)
        {
            // program removed from the old PAT, send event
            rmf_SiProgramHandle prog_handle;

            if (GetProgramEntryFromTransportStreamEntry(
                    (rmf_SiTransportStreamHandle) ts_entry,
                    walker_saved_pat_program->programNumber, &prog_handle)
                    == RMF_INBSI_SUCCESS)
            {
#if 0
                rmf_SiServiceHandle service_handle;
                publicSIIterator iter;
                rmf_SiProgramInfo *pi = (rmf_SiProgramInfo *) prog_handle;
                // reset PMT status, and PMT version
                pi->pmt_status = PMT_NOT_AVAILABLE;
                pi->pmt_version = INIT_TABLE_VERSION;

                // Need to loop through services attached to the program here, and notify each one.
                // Now more than one service can point to a program.
                service_handle = mpe_siFindFirstServiceFromProgram(
                        (rmf_SiProgramHandle) prog_handle, (SIIterator*) &iter);

                while (service_handle != RMF_SI_INVALID_HANDLE)
                {
                    // Signal these program PMTs as removed so as to unblock any waiting callers
                    mpe_siNotifyTableChanged(IB_PMT, RMF_SI_CHANGE_TYPE_REMOVE,
                            service_handle);

                    service_handle = mpe_siGetNextServiceFromProgram(
                            (SIIterator*) &iter);
                }
#endif
            }
        }
        walker_saved_pat_program = walker_saved_pat_program->next;
    }
}


/**
 * @brief Method called by SITP to set PAT CRC for a transport stream identified by the frequency.
 * <ul>
 * <li> If pat version is same and crc is different, comparing an reference PAT program list with a new PAT program list and the match is found
 * they are marked TRUE in the old PAT list.
 * <li> Left over programs in the old list are 'removed' (signal PMT 'REMOVE' for these).
 * <li> Left over programs in the new list are 'added' (signal PMT 'ADD' - handled by SITP).
 * <li> Handle signaling PAT program removed event from the old PAT. Delete the saved PAT reference.
 * </ul>
 * <ul>
 * <li> If pat version is same and crc is also same, delete the newly acquired pat programs and restore the saved list.
 * <li> No new events are generated in this case.
 * </ul>
 * <ul>
 * <li> If pat version is different and check_crc is 'false', Compare newly acquired pat program list with existing program list.
 * <li> Handle signaling PAT program removed event from the old PAT. Delete the saved PAT reference.
 * </ul>
 * <ul> If initial acquisition and no saved reference,
 * <li> For newly added programs, event notification is handled when SITP calls mpe_siNotifyTableChanged() with a table type of PMT and change type of ADD.
 * </ul>
 *
 * @param[in] ts_handle Transport stream handle to set PAT CRC.
 * @param[in] crc PAT CRC value.
 *
 * @return None
 */
void rmf_InbSiCache::SetPATCRCForTransportStream(rmf_SiTransportStreamHandle ts_handle,
    uint32_t crc)
{
    rmf_SiTransportStreamEntry *ts_entry =
            (rmf_SiTransportStreamEntry *) ts_handle;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<%s> freq:%d, crc:%d\n",
            __FUNCTION__, ts_entry->frequency, crc);

    if ((ts_entry->pat_crc != 0) && (ts_entry->check_crc == TRUE))
    {
        // version same, crc different
        if (ts_entry->pat_crc != crc)
        {
            rmf_SiPatProgramList *walker_saved_pat_program, *next;

            // If a match is found they are marked TRUE in the old PAT list.
            // Left over programs in the old list are 'removed' (signal PMT 'REMOVE' for these)
            // Left over programs in the new list are 'added' (signal PMT 'ADD' - handled by SITP)

            compare_ts_pat_programs(ts_entry->pat_program_list,
                    (rmf_SiPatProgramList *) ts_entry->pat_reference, FALSE);

            // For programs removed from the old PAT, signal event
            signal_program_removed_event(ts_entry,
                    (rmf_SiPatProgramList *) ts_entry->pat_reference);

            // delete the saved PAT reference
            walker_saved_pat_program
                    = (rmf_SiPatProgramList *) ts_entry->pat_reference;
            while (walker_saved_pat_program)
            {
                next = walker_saved_pat_program->next;
                rmf_osal_memFreeP(RMF_OSAL_MEM_SI_CACHE_INB, walker_saved_pat_program);
                walker_saved_pat_program = next;
            }
        }
        else // Version is same, crc is also same
        {
            // Free the newly acquired pat programs, restore the saved list
            // No new events are generated in this case
            rmf_SiPatProgramList *walker_new_pat_program = NULL;
            rmf_SiPatProgramList *next = NULL;
            walker_new_pat_program
                    = (rmf_SiPatProgramList *) ts_entry->pat_program_list;
            while (walker_new_pat_program)
            {
                next = walker_new_pat_program->next;
                rmf_osal_memFreeP(RMF_OSAL_MEM_SI_CACHE_INB, walker_new_pat_program);
                walker_new_pat_program = next;
            }

            ts_entry->pat_program_list
                    = (rmf_SiPatProgramList *) ts_entry->pat_reference;
        }
    }
    else // Version different, check_crc is 'false'
    {
        rmf_SiPatProgramList *walker_saved_pat_program, *next;

        if (ts_entry->pat_reference) // If there is a saved reference
        {
            // Compare newly acquired pat program list with existing program list
            compare_ts_pat_programs(ts_entry->pat_program_list,
                    (rmf_SiPatProgramList *) ts_entry->pat_reference, FALSE);

            // Existing PAT program list
            // For programs removed from the old PAT, signal event
            signal_program_removed_event(ts_entry,
                    (rmf_SiPatProgramList *) ts_entry->pat_reference);

            // delete the saved references
            walker_saved_pat_program
                    = (rmf_SiPatProgramList *) ts_entry->pat_reference;
            while (walker_saved_pat_program)
            {
                next = walker_saved_pat_program->next;
                rmf_osal_memFreeP(RMF_OSAL_MEM_SI_CACHE_INB, walker_saved_pat_program);
                walker_saved_pat_program = next;
            }
        }
        else // Initial acquisition, no saved reference
        {
            // For newly added programs event notification is handled when
            // SITP calls mpe_siNotifyTableChanged() with a table type of PMT
            // and change type of ADD
        }
    }

    ts_entry->pat_crc = crc;
    ts_entry->check_crc = FALSE;
    ts_entry->siStatus = SI_AVAILABLE;
    ts_entry->pat_reference = 0;
}


/**
 * @brief  It is used to set the PAT program for transport stream from the given transport stream handle.
 * It sets the program number and pmt pid of the new pat program entry.
 *
 * @param[in] ts_handle transport stream handle to set the PAT program
 * @param[in] pmt_pid PMT PID for the respective elementary streams.
 * @param[in] program_number PAT  program number.
 *
 * @retval RMF_INBSI_SUCCESS If successfully PAT program is set.
 * @retval RMF_INBSI_INVALID_PARAMETER Returned when a transport stream handle requested  is invalid.
 * @retval RMF_INBSI_OUT_OF_MEM Returned When memory allocation is failed while creating a new pat program list.
 */
rmf_Error rmf_InbSiCache::SetPATProgramForTransportStream(
    rmf_SiTransportStreamHandle ts_handle, uint32_t program_number,
    uint32_t pmt_pid)
{
    rmf_SiTransportStreamEntry *ts_entry =
            (rmf_SiTransportStreamEntry *) ts_handle;
    rmf_SiPatProgramList *new_pat_program = NULL;
    rmf_SiPatProgramList *walker_pat_program = NULL;

    if (ts_handle == RMF_INBSI_INVALID_HANDLE)
    {
        RDK_LOG(RDK_LOG_FATAL, "LOG.RDK.SI",
                "<%s> invalid Transport Stream!!\n", __FUNCTION__);
        return RMF_INBSI_INVALID_PARAMETER;
    }

    RDK_LOG(
            RDK_LOG_TRACE1,
            "LOG.RDK.SI",
            "<%s> freq:%d program_number:%d pmt_pid:%d\n",
            __FUNCTION__, ts_entry->frequency, program_number, pmt_pid);

    if (RMF_INBSI_SUCCESS != rmf_osal_memAllocP(RMF_OSAL_MEM_SI_CACHE_INB,
            sizeof(rmf_SiPatProgramList), (void **) &(new_pat_program)))
    {
        RDK_LOG(RDK_LOG_WARN, "LOG.RDK.SI",
                "<%s> Mem allocation failed, returning RMF_SI_OUT_OF_MEM...\n", __FUNCTION__);

        return RMF_INBSI_OUT_OF_MEM;
    }
    new_pat_program->programNumber = program_number;
    new_pat_program->pmt_pid = pmt_pid;
    new_pat_program->matched = FALSE;
    new_pat_program->next = NULL;

    if (ts_entry->pat_program_list == NULL)
    {
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                "<%s> inserted patprogram at the top\n", __FUNCTION__);
        ts_entry->pat_program_list = new_pat_program;
    }
    else
    {
        walker_pat_program = ts_entry->pat_program_list;
        while (walker_pat_program)
        {
            if (walker_pat_program->next == NULL)
            {
                RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                        "<%s> inserted patprogram in the list\n", __FUNCTION__);
                walker_pat_program->next = new_pat_program;
                break;
            }
            walker_pat_program = walker_pat_program->next;
        }
    }

    return RMF_INBSI_SUCCESS;
}


/**
 * @brief To set the version of PAT for the given transport stream entry.
 *
 * @param[in] ts_handle Transport stream handle to set the version for.
 * @param[in] pat_version version of the program association table.
 *
 * @return  None
 */
void rmf_InbSiCache::SetPATVersion(rmf_SiTransportStreamHandle ts_handle,
    uint32_t pat_version)
{
    rmf_SiTransportStreamEntry *ts_entry =
            (rmf_SiTransportStreamEntry *) ts_handle;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<%s> ts_handle:0x%x patVersion:%d\n", ts_handle,
            __FUNCTION__, pat_version);

    /* Set the PATVersion */
    ts_entry->pat_version = pat_version;
}


/**
 * @brief To set the PMT Pid for the given program entry.
 *
 * @param[in] program_handle Program handle to set the PMT Pid for.
 * @param[in] pmt_pid program map table Pid to set.
 *
 * @return None
 */
void rmf_InbSiCache::SetPMTPid(rmf_SiProgramHandle program_handle, uint32_t pmt_pid)
{
    rmf_SiProgramInfo *pi = (rmf_SiProgramInfo *) program_handle;
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<%s> pmt_pid: %d\n",
            __FUNCTION__, pmt_pid);
    pi->pmt_pid = pmt_pid;
}


/**
 * @brief  To set the transport stream SI status to not available for the given transport stream handle when PSI info is requested but not acquired.
 *
 * @param[in] ts_handle transport stream handle to set the SI status as not available.
 *
 * @return None
 */
void rmf_InbSiCache::SetTSIdStatusNotAvailable(rmf_SiTransportStreamHandle ts_handle)
{
    rmf_SiTransportStreamEntry *ts_entry =
            (rmf_SiTransportStreamEntry *) ts_handle;
    ts_entry->siStatus = SI_NOT_AVAILABLE;
}


/**
 * @brief To set the transport stream SI status to available shortly for the given transport stream handle when PSI info is requested and acquired.
 *
 * @param[in] ts_handle Transport stream handle to set the SI status.
 *
 * @return None
 */
void rmf_InbSiCache::SetTSIdStatusAvailableShortly(rmf_SiTransportStreamHandle ts_handle)
{
    rmf_SiTransportStreamEntry *ts_entry =
            (rmf_SiTransportStreamEntry *) ts_handle;

    if (ts_entry->siStatus == SI_NOT_AVAILABLE)
    {
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                "<%s>  freq: %d \n",
                __FUNCTION__, ts_entry->frequency);
        ts_entry->siStatus = SI_AVAILABLE_SHORTLY;
    }
}


/**
 * @brief To set the PAT status to not available of the given transport stream handle.
 * This is performed under a lock by SITP.
 * If the status was previously set to available shortly, then it reset to not available.
 *
 * @param[in] transport_handle Transport stream handle to set the PAT status.
 *
 * @return None
 */
void rmf_InbSiCache::SetPATStatusNotAvailable(
        rmf_SiTransportStreamHandle transport_handle)
{
    rmf_SiTransportStreamEntry *ts_entry =
            (rmf_SiTransportStreamEntry *) transport_handle;
    // If the status was previously set to available shortly then reset to not available
    if (ts_entry->siStatus == SI_AVAILABLE_SHORTLY)
    {
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                "<%s> ...\n", __FUNCTION__);
        ts_entry->siStatus = SI_NOT_AVAILABLE;
    }
}


/**
 * @brief To set the PAT status to available of the given service handle's transport stream.
 *
 * @param[in] transport_handle Transport stream handle to set the PAT status.
 *
 * @return None
 */
void rmf_InbSiCache::SetPATStatusAvailable(rmf_SiTransportStreamHandle transport_handle)
{
    rmf_SiTransportStreamEntry *ts_entry =
            (rmf_SiTransportStreamEntry *) transport_handle;
    ts_entry->siStatus = SI_AVAILABLE;
}


/**
 * @brief To reset the PMT status to 'NOT_AVAILABLE' for all programs on transport stream which is not signaled in the newly acquired PAT.
 * This is performed under a WRITE lock by SITP.
 * Reset PMT status if the program number is not found in the recently acquired PAT.
 * If its status has been previously set to PMT_AVAILABLE_SHORTLY, then
 * it signals a PMT to remove the programs which is no longer contained in the PAT.
 *
 * @param[in] ts_handle Transport stream handle to set the PAT program status.
 *
 * @return None
 */
void rmf_InbSiCache::ResetPATProgramStatus(rmf_SiTransportStreamHandle ts_handle)
{
    rmf_SiTransportStreamEntry *ts_entry =
            (rmf_SiTransportStreamEntry *) ts_handle;
    uint32_t found = 0;
    uint32_t program_number = 0;
    //LINK *lp = NULL;
    list<rmf_SiProgramHandle>::iterator pi;
    //LINK *lp1 = NULL;
    //rmf_SiProgramInfo *pi = NULL;
    //rmf_SiServiceHandle service_handle = RMF_SI_INVALID_HANDLE;
    //SIIterator iter;

    rmf_osal_mutexAcquire(ts_entry->list_lock);
    //if (ts_entry->modulation_mode != RMF_MODULATION_QAM_NTSC)
    {
        rmf_SiPatProgramList* pat_list =
                (rmf_SiPatProgramList*) ts_entry->pat_program_list;

        //lp = llist_first(ts_entry->programs);
        pi = ts_entry->programs->begin();

        while (pi != ts_entry->programs->end())
        {
            rmf_SiProgramInfo *p = (rmf_SiProgramInfo*)(*pi);
            
            //pi = (rmf_SiProgramInfo *) llist_getdata(lp);
            // reset PMT status if the program number is not found in the recently
            // acquired PAT and if its status has been previously set to PMT_AVAILABLE_SHORTLY
            // and signal a PMT REMOVE for each of those programs no longer contained in the PAT
            found = find_program_in_ts_pat_programs(pat_list,
                    p->program_number);

            if (!found && (PMT_AVAILABLE_SHORTLY == p->pmt_status))
            {
                //SIIterator *Iter = NULL;
                //rmf_SiProgramInfo *programs = NULL;
                p->pmt_status = PMT_NOT_AVAILABLE;
                p->pmt_version = INIT_TABLE_VERSION;

                //RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<reset_PATProgram_status> PMT status set to not available ...(%d/%d)\n", ts_entry->frequency, pi->program_number);

                // Need to loop through services attached to the program here, and notify each one.
                // This is new, because now more than one service can point to a program.
                //
                //Iter = (SIIterator *) (&iter);

                //Iter->program = (rmf_SiProgramInfo *) pi;
                
#if 0
                programs = (rmf_SiProgramInfo *) pi;
                lp1 = llist_first(programs->services);
                while (lp1)
                {
                    service_handle = (rmf_SiServiceHandle) llist_getdata(lp1);
                    //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI", "<reset_PATProgram_status> service_handle: 0x%x\n", service_handle);
                    Iter->lastReturnedIndex = 0;

                    if (service_handle != RMF_SI_INVALID_HANDLE)
                    {
                        // Signal these program PMTs as removed so as to unblock any waiting callers
                        mpe_siNotifyTableChanged(IB_PMT,
                                RMF_SI_CHANGE_TYPE_REMOVE, service_handle);

                        RDK_LOG(
                                RDK_LOG_INFO,
                                "LOG.RDK.SI",
                                "<reset_PATProgram_status> pn: %d, service_handle: 0x%x\n",
                                pi->program_number, service_handle);
                    }
                    lp1 = llist_after(lp1);
                }
#endif
            } // End if(!found && (PMT_AVAILABLE_SHORTLY == pi->pmt_status))

            //lp = llist_after(lp);
            pi++;
        }
    }
    rmf_osal_mutexRelease(ts_entry->list_lock);
}


/* PMT */
/**
 * @brief To set the PCR Pid for the given program entry.
 *
 * @param[in] program_handle Program handle to set the PCR Pid for.
 * @param[in] pcr_pid Program clock reference Pid to set.
 *
 * @return None
 */
void rmf_InbSiCache::SetPCRPid(rmf_SiProgramHandle program_handle, uint32_t pcr_pid)
{
    rmf_SiProgramInfo *pi = (rmf_SiProgramInfo *) program_handle;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<%s> pcr_pid: %d\n",
            __FUNCTION__, pcr_pid);
    /* Set the PCRPid */
    pi->pcr_pid = pcr_pid;
}


/**
 * @brief  To set the PMT version for the given program entry.
 * It checks whether the PMT version is out of validity or not.It should be maximum 5 bits.
 * Suppose if we get the new PMT version when we have the reference PMT, delete the outer descriptor loop and
 * the elementary stream within a service.
 * New PMT is being populated under the lock and it sets the pmt status to available shortly.
 * It sets the PMTVersion, pcr pid and outer descriptors list also.
 *
 * @param[in] program_handle Program handle to set the PMT version for.
 * @param[in] pmt_version Program map table version to set.
 *
 * @retval RMF_INBSI_SUCCESS If successfully PMT version is set.
 * @retval RMF_INBSI_INVALID_PARAMETER Returned when a program_handle requested is invalid or pmt version is unused "0x1f(all bits on)".
 * @retval RMF_INBSI_OUT_OF_MEM Returned when a program_handle requested  is out of MEM [Memory].
 */
rmf_Error rmf_InbSiCache::SetPMTVersion(rmf_SiProgramHandle program_handle,
    uint32_t pmt_version)
{
    rmf_SiProgramInfo *pi = (rmf_SiProgramInfo *) program_handle;

    /* Parameter check */
    /* check the validity of PMT version(max 5 bits) */
    if ((program_handle == RMF_INBSI_INVALID_HANDLE) || (pmt_version > 0x1f))
    {
        return RMF_INBSI_INVALID_PARAMETER;
    }

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<%s>  siEntry version: %d, version: %d\n",
            __FUNCTION__, pi->pmt_version, pmt_version);

    /* There is a older copy of the PMT */

    if (pi->pmt_version != pmt_version)
    {
        rmf_SiElementaryStreamList *elem_stream_list = pi->elementary_streams;
        rmf_SiElementaryStreamList *saved_next = NULL;

        // We have an existing PMT, but received a new version
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<%s>\n", __FUNCTION__);

        // delete outer descriptor loop
        if (pi->outer_descriptors != NULL)
        {
            release_descriptors(pi->outer_descriptors);
            pi->outer_descriptors = NULL;

            pi->number_outer_desc = 0;
        }

        // No need to do crc check
        pi->crc_check = FALSE;

        while (elem_stream_list)
        {
            saved_next = elem_stream_list->next;

            if (elem_stream_list->ref_count == 1)
            {
                        remove_elementaryStream(&(pi->elementary_streams), elem_stream_list);
                delete_elementary_stream(elem_stream_list);
                        (pi->number_elem_streams)--;
            }
            else if (elem_stream_list->ref_count > 1)
            {
                mark_elementary_stream(elem_stream_list);

                // We are getting new PMT, send events for service existing component handles
                // For handles delivered to Java increment the ref count
                elem_stream_list->ref_count++;
                /*RDK_LOG(
                        RDK_LOG_TRACE1,
                        "LOG.RDK.SI",
                        "<%s> Send service comp event - incremented ref count... %d\n",
                        __FUNCTION__, elem_stream_list->ref_count);

                send_service_component_event(elem_stream_list,
                        RMF_SI_CHANGE_TYPE_REMOVE);*/
            }
            elem_stream_list = saved_next;
        }
    }
    else
    {

        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                "<%s> PMT version is same, check CRC...\n", __FUNCTION__);

        // PMT version seems to be same, do a crc check
        pi->crc_check = TRUE;
        if (RMF_INBSI_SUCCESS != rmf_osal_memAllocP(RMF_OSAL_MEM_SI_CACHE_INB,
                sizeof(rmf_SiPMTReference), (void **) &(pi->saved_pmt_ref)))
        {
            RDK_LOG(RDK_LOG_WARN, "LOG.RDK.SI",
                    "<%s> Mem allocation failed, returning RMF_INBSI_OUT_OF_MEM...\n", __FUNCTION__);
            return RMF_INBSI_OUT_OF_MEM;
        }
        pi->saved_pmt_ref->pcr_pid = pi->pcr_pid;
        pi->saved_pmt_ref->number_outer_desc = pi->number_outer_desc;
        pi->saved_pmt_ref->outer_descriptors_ref
                = (uintptr_t) pi->outer_descriptors;
        pi->saved_pmt_ref->number_elem_streams = pi->number_elem_streams;
        pi->saved_pmt_ref->elementary_streams_ref
                = (uintptr_t) pi->elementary_streams;
    }

    // Un-couple exisiting outer descriptors and
    // elementary streams from the service entry
    pi->outer_descriptors = NULL;

    pi->number_outer_desc = 0;

    if (pi->elementary_streams != NULL)
    {
        pi->elementary_streams = NULL;
    }
    pi->number_elem_streams = 0;

    // New PMT is being populated under the lock.
    pi->pmt_status = PMT_AVAILABLE_SHORTLY;

    /* Set the PMTVersion */
    pi->pmt_version = pmt_version;

    return RMF_INBSI_SUCCESS;
}


/**
 * @brief To set the outer descriptor for the given program entry.
 * Store PMT outer descriptors in raw format. Append descriptor at the end of existing list.
 * It checks the tag and length field in the new descriptor and it adds those values to the descriptor content.
 *
 * @param[in] program_handle Program handle to set the descriptor for.
 * @param[in] tag Descriptor tag field.
 * @param[in] length Length of the descriptor.
 * @param[in] content Descriptor content.
 *
 * @retval RMF_INBSI_SUCCESS If successfully outer descriptor is set.
 * @retval RMF_INBSI_INVALID_PARAMETER Returned when a program_handle requested is invalid.
 * @retval RMF_INBSI_OUT_OF_MEM Returned when a program_handle requested  is out of MEM [Memory].
 */
rmf_Error rmf_InbSiCache::SetOuterDescriptor(rmf_SiProgramHandle program_handle,
    rmf_SiDescriptorTag tag, uint32_t length, void *content)
{
    rmf_SiProgramInfo *pi = (rmf_SiProgramInfo *) program_handle;
    rmf_SiMpeg2DescriptorList *desc_list = NULL;
    rmf_SiMpeg2DescriptorList *new_desc = NULL;

    /* Parameter check */
    if ((program_handle ==RMF_INBSI_INVALID_HANDLE))
    {
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                "<%s> Handle is invalid.  Cannot insert\n", __FUNCTION__);
        return RMF_INBSI_INVALID_PARAMETER;
    }

    /* Store PMT outer descriptors in raw format. */
    /* Append descriptor at the end of existing list */
    if (RMF_INBSI_SUCCESS != rmf_osal_memAllocP(RMF_OSAL_MEM_SI_CACHE_INB,
            sizeof(rmf_SiMpeg2DescriptorList), (void **) &(new_desc)))
    {
        RDK_LOG(
                RDK_LOG_WARN,
                "LOG.RDK.SI",
                "<%s> Mem allocation failed, returning RMF_SI_OUT_OF_MEM...\n",__FUNCTION__);
        return RMF_INBSI_OUT_OF_MEM;
    }
    new_desc->tag = tag;
    new_desc->descriptor_length = (unsigned char) length;
    if (length > 0)
    {
        if (RMF_INBSI_SUCCESS != rmf_osal_memAllocP(RMF_OSAL_MEM_SI_CACHE_INB, (sizeof(uint8_t)
                * length), (void **) &(new_desc->descriptor_content)))
        {
            rmf_osal_memFreeP(RMF_OSAL_MEM_SI_CACHE_INB, new_desc);
            RDK_LOG(RDK_LOG_WARN, "LOG.RDK.SI",
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

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<%s> tag:%d length:%d new_desc:0x%p\n", __FUNCTION__, tag,
            length, new_desc);
    desc_list = pi->outer_descriptors;

    if (desc_list == NULL)
    {
        pi->outer_descriptors = new_desc;
    }
    else
    {
        while (desc_list->next != NULL)
        {
            desc_list = desc_list->next;
        }
        desc_list->next = new_desc;
    }
    pi->number_outer_desc++;

    return RMF_INBSI_SUCCESS;
}


/**
 * @brief It sets all the descriptor field for the given service entry such as language, component name, number of strings,
 * number of bytes, segments, mode, compression type and so on.
 * The language and component descriptor types that are parsed out, stored in raw format to support org.ocap.si package.
 * It adds the descriptor at the end of existing list.
 *
 * @param[in] program_handle Program handle to set the descriptors.
 * @param[in] elem_pid  Elementary stream pid.
 * @param[in] type Elementary stream type such as audio, video.
 * @param[in] tag Descriptor tag field.
 * @param[in] length Length of the descriptor.
 * @param[in] content Descriptor content.
 *
 * @retval RMF_INBSI_SUCCESS If successfully set the descriptor.
 * @retval RMF_INBSI_INVALID_PARAMETER Returned when a program_handle requested is invalid or elementary pid is zero.
 * @retval RMF_INBSI_OUT_OF_MEM Memory allocation failed while allocating memory for the new language specific string list.
 */
rmf_Error rmf_InbSiCache::SetDescriptor(rmf_SiProgramHandle program_handle,
    uint32_t elem_pid, rmf_SiElemStreamType type, rmf_SiDescriptorTag tag,
    uint32_t length, void *content)
{
    rmf_SiProgramInfo *pi = (rmf_SiProgramInfo *) program_handle;
    rmf_SiMpeg2DescriptorList *desc_list = NULL;
    rmf_SiMpeg2DescriptorList *new_desc = NULL;
    rmf_SiElementaryStreamList *elem_list = NULL;
    int done = 0;

    /* Parameter check */
    if ((program_handle == RMF_INBSI_INVALID_HANDLE) || (elem_pid == 0))
    {
        return RMF_INBSI_INVALID_PARAMETER;
    }

    /* Populate associated language from language_descriptor */

    /* Populate component name from component_name_descriptor */

    /* Implement populating elementary stream info */
    elem_list = pi->elementary_streams;
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
                //RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<mpe_siSetDescriptor> language descriptor found...\n");
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

                // Free any previously specified component names
                if (elem_list->service_component_names != NULL)
                {
                    rmf_SiLangSpecificStringList *walker =
                            elem_list->service_component_names;

                    while (walker != NULL)
                    {
                        rmf_SiLangSpecificStringList *toBeFreed = walker;
                        rmf_osal_memFreeP(RMF_OSAL_MEM_SI_CACHE_INB, walker->string);
                        walker = walker->next;
                        rmf_osal_memFreeP(RMF_OSAL_MEM_SI_CACHE_INB, toBeFreed);
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
                                    "LOG.RDK.SI",
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
                        if (RMF_INBSI_SUCCESS != rmf_osal_memAllocP(RMF_OSAL_MEM_SI_CACHE_INB,
                                sizeof(struct rmf_SiLangSpecificStringList),
                                (void **) &new_entry))
                        {
                            RDK_LOG(RDK_LOG_WARN, "LOG.RDK.SI",
                                    "<%s> Mem allocation failed, returning RMF_INBSI_OUT_OF_MEM...\n",__FUNCTION__);
                            return RMF_INBSI_OUT_OF_MEM;
                        }
                        if (RMF_INBSI_SUCCESS != rmf_osal_memAllocP(RMF_OSAL_MEM_SI_CACHE_INB,
                                strlen(mssString) + 1,
                                (void **) &(new_entry->string)))
                        {
                            rmf_osal_memFreeP(RMF_OSAL_MEM_SI_CACHE_INB, new_entry);
                            RDK_LOG(RDK_LOG_WARN, "LOG.RDK.SI",
                                    "<%s> Mem allocation failed, returning RMF_INBSI_OUT_OF_MEM...\n",__FUNCTION__);
                            return RMF_INBSI_OUT_OF_MEM;
                        }
                        strcpy(new_entry->string, mssString);
                        strncpy(new_entry->language, lang, 4);

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
            if (RMF_INBSI_SUCCESS != rmf_osal_memAllocP(RMF_OSAL_MEM_SI_CACHE_INB,
                    sizeof(rmf_SiMpeg2DescriptorList), (void **) &(new_desc)))
            {
                RDK_LOG(RDK_LOG_WARN, "LOG.RDK.SI",
                        "<%s> Mem allocation failed, returning RMF_INBSI_OUT_OF_MEM...\n",__FUNCTION__);
                return RMF_INBSI_OUT_OF_MEM;
            }
            new_desc->tag = tag;
            new_desc->descriptor_length = (uint8_t) length;
            //RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<mpe_siSetDescriptor> new_desc->tag: 0x%x, tag: 0x%x, length: 0x%x, new_descriptor_length: 0x%x\n",
            //      new_desc->tag, tag, length, new_desc->descriptor_length);
            if (length > 0)
            {
                if (RMF_SUCCESS != rmf_osal_memAllocP(RMF_OSAL_MEM_SI_CACHE_INB, (sizeof(uint8_t)
                        * length), (void **) &(new_desc->descriptor_content)))
                {
                    rmf_osal_memFreeP(RMF_OSAL_MEM_SI_CACHE_INB, new_desc);
                    return RMF_INBSI_OUT_OF_MEM;
                }
                memcpy(new_desc->descriptor_content, content, length);
            }
            else
            {
                new_desc->descriptor_content = NULL;
            }
            new_desc->next = NULL;

            //RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<mpe_siSetDescriptor> created new decriptor 0x%x for elem_list 0x%x \n", new_desc, elem_list);
            {
                uint32_t i = 0;
                for (i = 0; i < length; i++)
                    RDK_LOG(
                            RDK_LOG_TRACE1,
                            "LOG.RDK.SI",
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

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<%s> tag:0x%x length: %d new_desc:0x%p\n", __FUNCTION__, tag,
            length, new_desc);

    return RMF_INBSI_SUCCESS;
}


/**
 * @brief To set the elementary stream info length for the given elementary stream.
 *
 * @param[in] program_handle Program handle to set the es info length for.
 * @param[in] elem_pid Elementary stream pid.
 * @param[in] type Elementary stream types such as audio, video.
 * @param[in] es_info_length Length of es info length.
 *
 * @return None
 */
void rmf_InbSiCache::SetESInfoLength(rmf_SiProgramHandle program_handle,
    uint32_t elem_pid, rmf_SiElemStreamType type, uint32_t es_info_length)
{
    rmf_SiProgramInfo *pi = (rmf_SiProgramInfo *) program_handle;
    rmf_SiElementaryStreamList *elem_list = NULL;

    /* Implement populating elementary stream info */
    elem_list = pi->elementary_streams;
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
 * @brief To set the elementary stream info for the given program entry such as elementary stream pid, stream type,
 * language, service componenet type and so on.
 * It populates the elementary stream info and appends the new elementary stream info with the existing one.
 *
 * @param[in] program_handle Program handle to set the elementary stream info for.
 * @param[in] stream_type Elementary stream type.
 * @param[in] elem_pid Elementary stream pid.
 *
 * @retval RMF_INBSI_SUCCESS If successfully set the elementary stream.
 * @retval RMF_INBSI_INVALID_PARAMETER Returned when a program_handle requested  is invalid or elementary pid is zero.
 * @retval RMF_INBSI_OUT_OF_MEM  when memory allocation failed while allocating memory for the elementary stream list.
 */
rmf_Error rmf_InbSiCache::SetElementaryStream(rmf_SiProgramHandle program_handle,
    rmf_SiElemStreamType stream_type, uint32_t elem_pid)
{
    rmf_SiProgramInfo *pi = (rmf_SiProgramInfo *) program_handle;
    rmf_SiElementaryStreamList *new_elem_stream = NULL;
    rmf_SiServiceComponentType service_comp_type;
    //LINK *lp = NULL;

    /* Parameter check */
    if ((program_handle == RMF_INBSI_INVALID_HANDLE) || (elem_pid == 0))
    {
        return RMF_INBSI_INVALID_PARAMETER;
    }

    if (RMF_INBSI_SUCCESS != rmf_osal_memAllocP(RMF_OSAL_MEM_SI_CACHE_INB,
            sizeof(rmf_SiElementaryStreamList), (void **) &(new_elem_stream)))
    {
        RDK_LOG(
                RDK_LOG_WARN,
                "LOG.RDK.SI",
                "<%s> Mem allocation failed, returning RMF_INBSI_OUT_OF_MEM...\n", __FUNCTION__);
        return RMF_INBSI_OUT_OF_MEM;
    }

    new_elem_stream->elementary_PID = elem_pid;
    new_elem_stream->elementary_stream_type = stream_type;
    new_elem_stream->next = NULL;
    new_elem_stream->number_desc = 0;
    new_elem_stream->ref_count = 1;
    new_elem_stream->valid = TRUE;
#if 0
    lp = llist_first(pi->services);
    RDK_LOG(
            RDK_LOG_TRACE1,
            "LOG.RDK.SI",
            "<mpe_siSetElementaryStream> pi->services:0x%p service_count: %d\n",
            pi->services, pi->service_count);
    if (lp != NULL)
    {
        new_elem_stream->service_reference = (uint32_t) llist_getdata(lp);
        RDK_LOG(
                RDK_LOG_TRACE1,
                "LOG.RDK.SI",
                "<mpe_siSetElementaryStream> new_elem_stream:0x%p new_elem_stream->service_reference:0x%x\n",
                new_elem_stream, new_elem_stream->service_reference);
    }
    else
    {
        RDK_LOG(
                RDK_LOG_ERROR,
                "LOG.RDK.SI",
                "<mpe_siSetElementaryStream> : Elementary Stream PID %d does not have a Service mapped to it\n", elem_pid);
        new_elem_stream->service_reference = (uint32_t)NULL; 
    }
#endif
    new_elem_stream->descriptors = NULL;
    new_elem_stream->service_component_names = NULL;
    new_elem_stream->es_info_length = 0;
    rmf_osal_timeGetMillis(&(new_elem_stream->ptime_service_component));

    RDK_LOG(
            RDK_LOG_TRACE1,
            "LOG.RDK.SI",
            "<%s> type:0x%x, pid: 0x%x new_elem_stream:0x%p\n",
            __FUNCTION__, stream_type, elem_pid, new_elem_stream);

    /* Populate associated language after reading descriptors (language_descriptor) */
    /* For now set it to null */
    memset(new_elem_stream->associated_language, 0, 4);

    /* Based on the elementary stream type set the service component type */
    get_serviceComponentType(stream_type, &service_comp_type);

    new_elem_stream->service_comp_type = service_comp_type;

    /* Populating elementary stream info */
    append_elementaryStream(pi, new_elem_stream);

    //RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<mpe_siSetElementaryStream> created 0x%x\n", new_elem_stream);
    
    return RMF_INBSI_SUCCESS;
}


/**
 * @brief To set the PMT status of the specified program_handle to available.
 *
 * @param[in] program_handle Program handle to set the PMT status for.
 *
 * @return None
 */
void rmf_InbSiCache::SetPMTStatus(rmf_SiProgramHandle program_handle)
{
    rmf_SiProgramInfo *pi = (rmf_SiProgramInfo *) program_handle;
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<%s> ...program_handle:0x%x\n", __FUNCTION__, program_handle);

    pi->pmt_status = PMT_AVAILABLE;

    {
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                "<%s>  pi->pmt_status: %d\n", __FUNCTION__, pi->pmt_status);
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                "<%s>  pi->program_number: %d\n",
                 __FUNCTION__, pi->program_number);
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                "<%s>  pi->number_elem_streams: %d\n",
                 __FUNCTION__, pi->number_elem_streams);
    }

}


/**
 * @brief To set the PMT status to 'Avialable_shortly' for all programs on transport stream identified by 'frequency'.
 * This is performed under a WRITE lock by SITP.
 *
 * @param[in] ts_handle Transport stream handle to set the PMT status.
 *
 * @return None
 */
void rmf_InbSiCache::SetPMTStatusAvailableShortly(rmf_SiTransportStreamHandle ts_handle)
{
    rmf_SiTransportStreamEntry *ts_entry =
            (rmf_SiTransportStreamEntry *) ts_handle;

    //LINK *lp = NULL;
    list<rmf_SiProgramHandle>::iterator piter;
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<%s> ...\n", __FUNCTION__);

    rmf_osal_mutexAcquire(ts_entry->list_lock);
    //if (ts_entry->modulation_mode != RMF_MODULATION_QAM_NTSC)
    if(ts_entry->program_count > 0)
    {
        piter = ts_entry->programs->begin();
        //lp = llist_first(ts_entry->programs);
        while (piter != ts_entry->programs->end())
        {
            //rmf_SiProgramInfo *pi = (rmf_SiProgramInfo *) llist_getdata(lp);
            rmf_SiProgramInfo *pi = (rmf_SiProgramInfo*)(*piter);
            if (pi != NULL)
            {
                if ((PMT_NOT_AVAILABLE == pi->pmt_status))
                {
                    pi->pmt_status = PMT_AVAILABLE_SHORTLY;
                    RDK_LOG(
                            RDK_LOG_TRACE1,
                            "LOG.RDK.SI",
                            "<%s> PMT status set to available shortly...(%d/%d)\n",
                            __FUNCTION__, ts_entry->frequency, pi->program_number);
                }
            }
            //lp = llist_after(lp);
            piter++;
        }
    }
    rmf_osal_mutexRelease(ts_entry->list_lock);
}


/**
 * @brief To set the PMT status to not available.
 * This is performed under a WRITE lock.
 *
 * @param[in] pi_handle It is the handle to program info for which the PMT status is set to not available.
 *
 * @retval RMF_INBSI_SUCCESS If successfully set the PMT status.
 * @retval RMF_INBSI_INVALID_PARAMETER Returned when a program_handle requested is invalid or transport stream entry is NULL.
 * @retval RMF_INBSI_NOT_FOUND It is returned when a program_handle requested is not found.
 */
rmf_Error rmf_InbSiCache::SetPMTStatusNotAvailable(rmf_SiProgramHandle pi_handle)
{
    rmf_SiProgramInfo *pi = (rmf_SiProgramInfo *) pi_handle;
    rmf_SiTransportStreamEntry *ts = NULL;
    int found = 0;
    //LINK *lp = NULL;
    list<rmf_SiProgramHandle>::iterator lp;

    /* Parameter check */
    if ((pi_handle == RMF_INBSI_INVALID_HANDLE) || ((ts = pi->stream) == NULL))
    {
        return RMF_INBSI_INVALID_PARAMETER;
    }

    rmf_osal_mutexAcquire(ts->list_lock);

    //lp = llist_first(ts->programs);
    lp = ts->programs->begin();
    while (lp != ts->programs->end())
    {
        //pi = (rmf_SiProgramInfo *) llist_getdata(lp);
        pi = (rmf_SiProgramInfo*)(*lp);
        if (pi != NULL)
        {
            pi->pmt_status = PMT_NOT_AVAILABLE;
            pi->pmt_version = INIT_TABLE_VERSION;
            RDK_LOG(
                    RDK_LOG_TRACE1,
                    "LOG.RDK.SI",
                    "<%s> PMT status set to not available ..(%d/%d)\n",
                    __FUNCTION__, ts->frequency, pi->program_number);
            found = 1;
        }
        //lp = llist_after(lp);
        lp++;
    }
    rmf_osal_mutexRelease(ts->list_lock);

    if (!found)
    {
        //MPE_LOG(MPE_LOG_DEBUG, MPE_MOD_SI, "<mpe_siSetPMTStatusAvailableShortly> did not find any program entries...\n");
        return RMF_INBSI_NOT_FOUND;
    }
    return RMF_INBSI_SUCCESS;
}


/**
 * @brief It sets the PMT program status to not available.
 *
 * @param[in] pi_handle It is the handle to program info for which the PMT program status is set to not available.
 *
 * @retval RMF_INBSI_SUCCESS If successfully PMT program status is set.
 * @retval RMF_INBSI_INVALID_PARAMETER Returned when a program_handle requested is invalid.
 * @retval RMF_INBSI_NOT_FOUND Returned when a program_handle requested is not found.
 */
rmf_Error rmf_InbSiCache::SetPMTProgramStatusNotAvailable(rmf_SiProgramHandle pi_handle)
{
    rmf_SiProgramInfo *pi = (rmf_SiProgramInfo *) pi_handle;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<%s> pi_handle: 0x%8.8x\n",
            __FUNCTION__, pi_handle);

    /* Parameter check */
    if (pi_handle == RMF_INBSI_INVALID_HANDLE)
    {
        return RMF_INBSI_INVALID_PARAMETER;
    }

    if (pi != NULL)
    {
        pi->pmt_status = PMT_NOT_AVAILABLE;
        pi->pmt_version = INIT_TABLE_VERSION;
        RDK_LOG(
                RDK_LOG_TRACE1,
                "LOG.RDK.SI",
                "<%s> PMT status set to not available ..(program_number: %d)\n",
                __FUNCTION__, pi->program_number);
        return RMF_INBSI_SUCCESS;
    }

    return RMF_INBSI_NOT_FOUND;
}


/**
 * @brief To retrieve the PMT program status for a given program handle.
 *
 * @param[in] pi_handle It is the handle for program info to get the PMT program status.
 * @param[out] pmt_status PMT status if it is available or not.
 *
 * @retval RMF_INBSI_SUCCESS If successfully gets the PMT program status .
 * @retval RMF_INBSI_INVALID_PARAMETER Returned when program_handle is invalid.
 * @retval RMF_INBSI_NOT_FOUND Returned when a program_handle requested  is not found.
 */
rmf_Error rmf_InbSiCache::GetPMTProgramStatus(rmf_SiProgramHandle pi_handle, rmf_SiPMTStatus *pmt_status)
{
    rmf_SiProgramInfo *pi = (rmf_SiProgramInfo *) pi_handle;
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<%s> pi_handle: 0x%8.8x\n",
            __FUNCTION__, pi_handle);
    /* Parameter check */
    if (pi_handle == RMF_INBSI_INVALID_HANDLE)
    {
        return RMF_INBSI_INVALID_PARAMETER;
    }
    if (pi != NULL)
    {
        *pmt_status = pi->pmt_status;
        return RMF_INBSI_SUCCESS;
    }
    return RMF_INBSI_NOT_FOUND;
}


/**
 * @brief It reverts the PMT status identified by the program handle.
 * If it is available shortly, it sets pmt status to not available .
 *
 * @param[in] pi_handle It is the handle for program info to revert the PMT status.
 *
 * @retval RMF_INBSI_SUCCESS If successfully reverts the PMT status .
 * @retval RMF_INBSI_INVALID_PARAMETER Returned when program_handle is invalid or transport stream entry is NULL.
 * @retval RMF_INBSI_NOT_FOUND Returned when a program_handle requested is not found.
 */
rmf_Error rmf_InbSiCache::RevertPMTStatus(rmf_SiProgramHandle pi_handle)
{
    rmf_SiProgramInfo *pi = (rmf_SiProgramInfo *) pi_handle;
    rmf_SiTransportStreamEntry *ts = NULL;
    int found = 0;
    //LINK *lp = NULL;
    list<rmf_SiProgramHandle>::iterator lp;

    /* Parameter check */
    if ((pi_handle == RMF_INBSI_INVALID_HANDLE) || ((ts = pi->stream) == NULL))
    {
        return RMF_INBSI_INVALID_PARAMETER;
    }

    rmf_osal_mutexAcquire(ts->list_lock);

    //lp = llist_first(ts->programs);
    lp = ts->programs->begin();
    while (lp!=ts->programs->end())
    {
        //pi = (rmf_SiProgramInfo *) llist_getdata(lp);
        pi = (rmf_SiProgramInfo*)(*lp);
        if (pi != NULL)
        {
            if (PMT_AVAILABLE_SHORTLY == pi->pmt_status)
            {
                pi->pmt_status = PMT_NOT_AVAILABLE;
                RDK_LOG(
                        RDK_LOG_TRACE1,
                        "LOG.RDK.SI",
                        "<%s> PMT status set to not available ..(%d/%d)\n",
                        __FUNCTION__, ts->frequency, pi->program_number);
                found = 1;
            }
        }
        //lp = llist_after(lp);
        lp++;
    }
    rmf_osal_mutexRelease(ts->list_lock);

    if (!found)
    {
        return RMF_INBSI_NOT_FOUND;
    }
    return RMF_INBSI_SUCCESS;

}


/**
 * @brief To set the PMT CRC from the given service entry.
 * It sets the outer descriptor and elementary stream list from the PMT reference.
 * If CRC is same (version was also same), keep the old PMT itself.
 *
 * @param[in] pi_handle Service_handle to set the PMT CRC for.
 * @param[in] new_crc New PMT crc value.
 *
 * @return None.
 */
void rmf_InbSiCache::SetPMTCRC(rmf_SiProgramHandle pi_handle, uint32_t new_crc)
{
    rmf_SiProgramInfo *pi = (rmf_SiProgramInfo *) pi_handle;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<%s> crc ...0x%x / 0x%x\n", __FUNCTION__, new_crc, pi->pmt_crc);

    if ((pi->pmt_crc != 0) && (pi->crc_check == TRUE))
    {
        // PMT (with same version) and new CRC is received
        if (pi->pmt_crc != new_crc)
        {
            rmf_SiElementaryStreamList *saved_next = NULL;
            rmf_SiPMTReference *pmt_ref = pi->saved_pmt_ref;
            rmf_SiMpeg2DescriptorList* outer_desc =
                    (rmf_SiMpeg2DescriptorList*) pmt_ref->outer_descriptors_ref;
            rmf_SiElementaryStreamList
                    *elem_stream_list =
                            (rmf_SiElementaryStreamList *) pmt_ref->elementary_streams_ref;
            rmf_SiElementaryStreamList *new_elem_stream_list = NULL;

            // We have an existing PMT, but received a new version
            RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                    "<%s> New crc: %d\n", __FUNCTION__, new_crc);

            // delete saved outer descriptor loop
            if (outer_desc != NULL)
            {
                release_descriptors(outer_desc);
            }

            // delete saved elem stream loop
            while (elem_stream_list)
            {
                saved_next = elem_stream_list->next;

                if (elem_stream_list->ref_count == 1)
                {
                                remove_elementaryStream((rmf_SiElementaryStreamList **)(&pmt_ref->elementary_streams_ref), elem_stream_list);
                    delete_elementary_stream(elem_stream_list);
                                (pmt_ref->number_elem_streams)--;
                }
                else if (elem_stream_list->ref_count > 1)
                {
                    mark_elementary_stream(elem_stream_list);

                    // We are getting new PMT, send events for service existing component handles
                    // For handles delivered to Java increment the ref count
                    elem_stream_list->ref_count++;
                    //RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<mpe_siSetPMTVersion> Send service comp event - incremented ref count... %d\n", elem_stream_list->ref_count);

                    //send_service_component_event(elem_stream_list,
                    //      MPE_SI_CHANGE_TYPE_REMOVE);
                }

                elem_stream_list = saved_next;
            }

            RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                    "<%s> num_elem_streams... %d\n",
                     __FUNCTION__, pi->number_elem_streams);

            // Send 'ADD' event for each new elementary stream
            new_elem_stream_list = pi->elementary_streams;
            while (new_elem_stream_list)
            {
                new_elem_stream_list->ref_count++;
                //send_service_component_event(new_elem_stream_list,
                //      MPE_SI_CHANGE_TYPE_ADD);

                new_elem_stream_list = new_elem_stream_list->next;
            }
        }
        else
        {
            // CRC is same (version was also same), don't do anything
            // Keeping the old PMT

            rmf_SiPMTReference *pmt_ref = pi->saved_pmt_ref;

            rmf_SiElementaryStreamList *elem_stream_list =
                    pi->elementary_streams;
            rmf_SiElementaryStreamList *saved_next = NULL;

            RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                    "<%s> CRC hasn't changed...0x%x\n", __FUNCTION__, new_crc);

            // delete newly allocated outer desc and elem streams

            // delete outer descriptor loop
            if (pi->outer_descriptors != NULL)
            {
                release_descriptors(pi->outer_descriptors);
                pi->outer_descriptors = NULL;
            }

            pi->number_outer_desc = 0;

            while (elem_stream_list)
            {
                saved_next = elem_stream_list->next;

                if (elem_stream_list->ref_count == 1)
                {
                            remove_elementaryStream((&pi->elementary_streams), elem_stream_list);
                    delete_elementary_stream(elem_stream_list);
                                (pi->number_elem_streams)--;
                }
                else if (elem_stream_list->ref_count > 1)
                {
                    mark_elementary_stream(elem_stream_list);

                    // We are getting new PMT, send events for service existing component handles
                    // For handles delivered to Java increment the ref count
                    elem_stream_list->ref_count++;
                    //RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<mpe_siSetPMTVersion> Send service comp event - incremented ref count... %d\n", elem_stream_list->ref_count);

                    //send_service_component_event(elem_stream_list,
                    //      MPE_SI_CHANGE_TYPE_REMOVE);
                }

                elem_stream_list = saved_next;
            }

            // Restore old links
            pi->pcr_pid = pmt_ref->pcr_pid;
            pi->number_outer_desc = pmt_ref->number_outer_desc;
            pi->outer_descriptors
                    = (rmf_SiMpeg2DescriptorList*) pmt_ref->outer_descriptors_ref;
            pi->number_elem_streams = pmt_ref->number_elem_streams;
            pi->elementary_streams
                    = (rmf_SiElementaryStreamList *) pmt_ref->elementary_streams_ref;
        }

    }
    else if ((pi->pmt_crc == 0) && (pi->crc_check == FALSE))
    {
        // Initial acquisition of PMT
        // Send 'ADD' event for each new elementary stream
        rmf_SiElementaryStreamList *new_elem_stream_list = NULL;
        new_elem_stream_list = pi->elementary_streams;
        while (new_elem_stream_list)
        {
            new_elem_stream_list->ref_count++;
            //send_service_component_event(new_elem_stream_list,
            //      MPE_SI_CHANGE_TYPE_ADD);

            new_elem_stream_list = new_elem_stream_list->next;
        }

    }

    // update the crc
    pi->pmt_crc = new_crc;

    pi->crc_check = FALSE;

    if (pi->saved_pmt_ref != NULL)
    {
        // Free the memory allocated for pmt reference
        rmf_osal_memFreeP(RMF_OSAL_MEM_SI_CACHE_INB, pi->saved_pmt_ref);
        pi->saved_pmt_ref = NULL;
    }

}

void rmf_InbSiCache::release_program_info_entry(rmf_SiProgramInfo *pi)
{
    // Make sure all complex data items are accounted for, lest we leak memory.
    if (pi == NULL)
    {
        RDK_LOG(RDK_LOG_FATAL, "LOG.RDK.SI",
                "Can't release null ProgramInfoEntry!!");
        return;
    }
    
    if (pi->saved_pmt_ref != NULL)
    {
        if (pi->saved_pmt_ref->outer_descriptors_ref != 0)
        {
            release_descriptors(
                    (rmf_SiMpeg2DescriptorList*) pi->outer_descriptors);
            release_descriptors(
                    (rmf_SiMpeg2DescriptorList*) pi->saved_pmt_ref->outer_descriptors_ref);
        }
        while (pi->saved_pmt_ref->elementary_streams_ref != 0)
        {
            rmf_SiElementaryStreamList *elem_stream_list = (rmf_SiElementaryStreamList *)pi->saved_pmt_ref->elementary_streams_ref;
            remove_elementaryStream((rmf_SiElementaryStreamList **)&(pi->saved_pmt_ref->elementary_streams_ref), elem_stream_list);
            delete_elementary_stream(elem_stream_list);
            (pi->saved_pmt_ref->number_elem_streams)--;
        }
    rmf_osal_memFreeP(RMF_OSAL_MEM_SI_CACHE_INB, pi->saved_pmt_ref);
        pi->saved_pmt_ref = NULL;
    }
    if (pi->pmt_byte_array != NULL)
        rmf_osal_memFreeP(RMF_OSAL_MEM_SI_CACHE_INB, pi->pmt_byte_array);
    if (pi->outer_descriptors != NULL)
        release_descriptors(pi->outer_descriptors);
    while (pi->elementary_streams != NULL)
    {
        rmf_SiElementaryStreamList *elem_stream_list = pi->elementary_streams;
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<%s> release elem stream, %p\n", __FUNCTION__, elem_stream_list);
        remove_elementaryStream(&(pi->elementary_streams), elem_stream_list);
        delete_elementary_stream(elem_stream_list);
        (pi->number_elem_streams)--;
    }
    pi->elementary_streams = NULL;
    pi->services->clear();
    delete pi->services;
    rmf_osal_memFreeP(RMF_OSAL_MEM_SI_CACHE_INB, pi);
}

void rmf_InbSiCache::release_transport_stream_entry(rmf_SiTransportStreamEntry *ts_entry)
{
    rmf_SiTransportStreamEntry **this_list;

    // Make sure all complex data items are accounted for, lest we leak memory.
    if (ts_entry == NULL)
    {
        RDK_LOG(RDK_LOG_FATAL, "LOG.RDK.SI",
                "Can't release null TransportStreamEntry!!");
        return;
    }

    if (ts_entry->ts_name != NULL)
        rmf_osal_memFreeP(RMF_OSAL_MEM_SI_CACHE_INB, ts_entry->ts_name);

    if (ts_entry->description != NULL)
        rmf_osal_memFreeP(RMF_OSAL_MEM_SI_CACHE_INB, ts_entry->description);

    if (ts_entry->pat_reference != 0)
    {
        // delete the saved references
        rmf_SiPatProgramList *next, *walker_saved_pat_program =
                (rmf_SiPatProgramList *) ts_entry->pat_reference;
        while (walker_saved_pat_program)
        {
            next = walker_saved_pat_program->next;
            rmf_osal_memFreeP(RMF_OSAL_MEM_SI_CACHE_INB, walker_saved_pat_program);
            walker_saved_pat_program = next;
        }
    }

    if (ts_entry->pat_program_list != NULL)
    {
        // delete the saved references
        rmf_SiPatProgramList *next, *walker_saved_pat_program =
                ts_entry->pat_program_list;
        while (walker_saved_pat_program)
        {
            next = walker_saved_pat_program->next;
            rmf_osal_memFreeP(RMF_OSAL_MEM_SI_CACHE_INB, walker_saved_pat_program);
            walker_saved_pat_program = next;
        }
    }

    if (ts_entry->pat_byte_array != NULL)
    {
        rmf_osal_memFreeP(RMF_OSAL_MEM_SI_CACHE_INB, ts_entry->pat_byte_array);
    }

    /*if (llist_cnt(ts_entry->programs) != 0)
    {
        MPE_LOG(MPE_LOG_FATAL, MPE_MOD_SI, "deleting non-empty list!!");
    }
    llist_free(ts_entry->programs);*/
    list<rmf_SiProgramHandle>::iterator prog_iter;
    rmf_SiProgramInfo *pi = NULL;

    for(prog_iter=ts_entry->programs->begin();prog_iter!=ts_entry->programs->end();prog_iter++)
    {
        pi = (rmf_SiProgramInfo*)(*prog_iter);
        release_program_info_entry(pi);
    }
    ts_entry->programs->clear();
    delete ts_entry->programs;

    ts_entry->services->clear();
    delete ts_entry->services;
    rmf_osal_mutexDelete(ts_entry->list_lock);

    // Remove from linked list.
    // We should be holding the writerLock at a level above this, so it's
    // safe to do this last.

        this_list = &g_si_ts_entry;

    if (*this_list == ts_entry)
    {
        *this_list = ts_entry->next;
        ts_entry->next = NULL;
    }
    else
    {
        rmf_SiTransportStreamEntry *walker = *this_list;
        while ((walker != NULL) && (walker->next != ts_entry))
        { // Find the previous entry on list.
            walker = walker->next;
        }
        if (walker != NULL)
        { // unlink
            walker->next = ts_entry->next;
            ts_entry->next = NULL;
        }
        else
        {
            RDK_LOG(RDK_LOG_FATAL, "LOG.RDK.SI",
                    "Transport Stream Entry was NOT in linked list!!");
        }
    }
    RDK_LOG(RDK_LOG_TRACE7, "LOG.RDK.SI", "ts freed @ %p\n", ts_entry);
    rmf_osal_memFreeP(RMF_OSAL_MEM_SI_CACHE_INB, ts_entry);
}


/**
 * @brief It releases the program information entry from the given service hanlde.
 *
 * @param[in] pi_handle It is the handle for program info.
 *
 * @return None.
 */
#ifdef OPTIMIZE_INBSI_CACHE
void rmf_InbSiCache::ReleaseProgramInfoEntry(rmf_SiProgramHandle pi_handle)
{
    rmf_SiProgramInfo *pi = (rmf_SiProgramInfo *) pi_handle;
    rmf_SiTransportStreamEntry *ts_entry = (rmf_SiTransportStreamEntry*)pi->stream;
    release_program_info_entry(pi);
    ts_entry->programs->remove(pi_handle);
}


/**
 * @brief It releases the transport stream entry from the given transport stream handle.
 *
 * @param[in] ts_handle Transport stream handle.
 *
 * @return None.
 */
void rmf_InbSiCache::ReleaseTransportStreamEntry(rmf_SiTransportStreamHandle ts_handle)
{
        rmf_SiTransportStreamEntry *ts_entry =
            (rmf_SiTransportStreamEntry *) ts_handle;

        release_transport_stream_entry(ts_entry);
}
#endif

/**
 * Retrieve number of components for the given service handle
 *
 * <i>GetNumberOfComponentsForServiceHandle()</i>
 *
 * @param service_handle input si handle
 * @param num_components is the output param to populate number of components
 *
 * @return RMF_INBSI_SUCCESS if successful, else
 *          return appropriate rmf_Error
 *
 */
rmf_Error rmf_InbSiCache::GetNumberOfComponentsForServiceHandle(
                rmf_SiProgramHandle pi_handle, uint32_t *num_components)
{
    if((pi_handle == RMF_INBSI_INVALID_HANDLE) ||
       (num_components == NULL))
        return RMF_INBSI_INVALID_PARAMETER;
 
    rmf_SiProgramInfo *pi = (rmf_SiProgramInfo *) pi_handle;
    *num_components = pi->number_elem_streams;
    return RMF_INBSI_SUCCESS;
}

rmf_osal_Bool rmf_InbSiCache::CheckCADescriptors(rmf_SiProgramHandle pi_handle, uint16_t ca_system_id,
        uint32_t *numStreams, uint16_t *ecmPid, rmf_StreamDecryptInfo streamInfo[])
{
        rmf_SiMpeg2DescriptorList *desc_list_walker = NULL;
        rmf_SiElementaryStreamList *elem_stream_list = NULL;
        uint32_t numDescriptors = 0;
        rmf_osal_Bool caDescFound = FALSE;
        rmf_osal_Bool ecmPidFound = FALSE;
        rmf_SiProgramInfo *pi = NULL;
        int i=0;
        // caller has SI DB read lock

        pi = (rmf_SiProgramInfo*)pi_handle;

        if (pi == NULL) // could be analog
        {
                return FALSE;
        }

        numDescriptors = pi->number_outer_desc;
        desc_list_walker = pi->outer_descriptors;

        if(numDescriptors == 0)
        {
                RDK_LOG(RDK_LOG_TRACE1,
                                "LOG.RDK.POD",
                                "CheckCADescriptors: No outer descriptors..\n");
        }

        RDK_LOG(RDK_LOG_TRACE1,
                        "LOG.RDK.SI",
                        "CheckCADescriptors - numDescriptors: %d\n", numDescriptors );

        for (i = 0; i < numDescriptors; i++)
        {
                // an unexpected descriptor entry ?
                if (desc_list_walker == NULL)
                {
                        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD",
                                        "CheckCADescriptors: Unexpected null descriptor_entry !\n");
                        break;
                }

                if (desc_list_walker->tag == RMF_SI_DESC_CA_DESCRIPTOR
                                && desc_list_walker->descriptor_length > 0)
                {
                        uint16_t cas_id = ((uint8_t *) desc_list_walker->descriptor_content)[0] << 8
                                                           | ((uint8_t *) desc_list_walker->descriptor_content)[1];
            if(!ecmPidFound)
            {
                *ecmPid = ((uint8_t *) desc_list_walker->descriptor_content)[2] << 8
                                   | ((uint8_t *) desc_list_walker->descriptor_content)[3];

                *ecmPid &= 0x1FFF;
               ecmPidFound = TRUE;
            }

            RDK_LOG(RDK_LOG_TRACE1,
                    "LOG.RDK.SI",
                    "CheckCADescriptors - ecmPid: 0x%x\n", *ecmPid);

                        // Match the CA system id
                        if(cas_id == ca_system_id)
                        {
                                caDescFound = TRUE;
                        }
                } // END if (CA descriptor)
                desc_list_walker = desc_list_walker->next;
        } // END for (outer descriptors)

        i=0;
        // Walk inner descriptors
        elem_stream_list = pi->elementary_streams;
        while (elem_stream_list)
        {
                desc_list_walker = elem_stream_list->descriptors;
                while (desc_list_walker)
                {
                        if (desc_list_walker->tag == RMF_SI_DESC_CA_DESCRIPTOR)
                        {
                                uint16_t cas_id = ((uint8_t *) desc_list_walker->descriptor_content)[0] << 8
                                                                   | ((uint8_t *) desc_list_walker->descriptor_content)[1];

                if(!ecmPidFound)
                {
                    *ecmPid = ((uint8_t *) desc_list_walker->descriptor_content)[2] << 8
                                       | ((uint8_t *) desc_list_walker->descriptor_content)[3];
                    *ecmPid &= 0x1FFF;
                    ecmPidFound = TRUE;
                    RDK_LOG(RDK_LOG_TRACE1,
                            "LOG.RDK.SI",
                            "CheckCADescriptors - ecmPid: 0x%x\n", *ecmPid);
                }

                                // Match the CA system id
                                if(cas_id == ca_system_id)
                                {
                                        caDescFound = TRUE;
                                }
                        }
                        desc_list_walker = desc_list_walker->next;
                }

                streamInfo[i].pid = elem_stream_list->elementary_PID;
                streamInfo[i].status = 0; // CA_ENABLE_NO_VALUE

                RDK_LOG(RDK_LOG_INFO,
                                "LOG.RDK.SI",
                                "CheckCADescriptors - pid:%d ca_status:%d \n", elem_stream_list->elementary_PID, streamInfo[i].status);
                i++;

                elem_stream_list = elem_stream_list->next;
        }

        *numStreams = i;
        if(caDescFound)
        {
                return TRUE;
        }
        else
        {
                return FALSE;
        }

        // Caller has to unlock SI DB
}
