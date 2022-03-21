/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2015 RDK Management
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
* Contributed by ARRIS Group, Inc.
* ============================================================================
*/

#include "rmf_dvbsimgr.h"

// Project's includes
#include "TDvbHelper.h"
#include "rmf_sectionfilter.h"

rmf_DvbSiMgr::rmf_DvbSiMgr(rmf_PsiSourceInfo psiSource)
    : rmf_InbSiMgr(psiSource)
{
    /* Let's create an event manager for the section parser to use */
    if(RMF_SUCCESS != rmf_osal_eventmanager_create(&m_EventMgr))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBSIMGR",
                "<%s: %s> - unable to create event manager\n",
                PSIMODULE, __FUNCTION__);
    }

    /* Time to create the section parser */
    TDvbInterface* dvbInterface((TDvbHelper::GetDvbHelperInstance())->GetDvbInterfaceInstance());
    rmf_osal_eventmanager_handle_t tmp_EventMgr(m_EventMgr);
    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBSIMGR", " %s tmp_EventMgr = %#X m_EventMgr = %#X \n", tmp_EventMgr, m_EventMgr);
    SectionParser = new TSectionParser();
    dvbInterface->RegiterToSectionParserNotifications(SectionParser);
    dvbInterface->SetEventHandlerInstance(m_EventMgr);
}

rmf_DvbSiMgr::~rmf_DvbSiMgr()
{
    cancel_all_filters();

    TDvbInterface* dvbInterface((TDvbHelper::GetDvbHelperInstance())->GetDvbInterfaceInstance());
    dvbInterface->UnRegiterToSectionParserNotifications();
    delete SectionParser;
    rmf_osal_eventmanager_delete(m_EventMgr);
}

rmf_Error rmf_DvbSiMgr::RegisterForDvbSiEvents(rmf_osal_eventqueue_handle_t queueId)
{
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.DVBSIMGR",
            "<%s> called, queueId: 0x%x\n", __FUNCTION__, queueId);

    /* Acquire registration list mutex */
    rmf_osal_mutexAcquire(g_inbsi_registration_list_mutex);

    rmf_osal_eventmanager_register_handler(m_EventMgr, queueId, RMF_OSAL_EVENT_CATEGORY_INB_FILTER);

    rmf_osal_mutexRelease(g_inbsi_registration_list_mutex);

    return RMF_INBSI_SUCCESS;
}

rmf_Error rmf_DvbSiMgr::UnRegisterForDvbSiEvents(rmf_osal_eventqueue_handle_t queueId)
{
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.DVBSIMGR",
            "<%s> called, queueId: 0x%x\n", __FUNCTION__, queueId);

    /* Acquire registration list mutex */
    rmf_osal_mutexAcquire(g_inbsi_registration_list_mutex);

    rmf_osal_eventmanager_unregister_handler(m_EventMgr, queueId);

    rmf_osal_mutexRelease(g_inbsi_registration_list_mutex);

    return RMF_INBSI_SUCCESS;
}

rmf_Error rmf_DvbSiMgr::RequestDvbInfo(vector<rmf_PsiParam>& filters)
{
    rmf_Error ret = RMF_INBSI_SUCCESS;
    uint32_t filterId = 0;

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.DVBSIMGR", "<%s: %s> - called\n", PSIMODULE, __FUNCTION__);
    for(auto it = filters.begin(), end = filters.end(); it != end; ++it)
    {
        ret = set_filter(&(*it), &filterId);
        if(RMF_SUCCESS != ret)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBSIMGR", "<%s: %s> - set_filter(pid = 0x%x, table_id = 0x%x) failed\n",
                    PSIMODULE, __FUNCTION__, it->pid, it->table_id);
            return ret;
        }
    }

    return ret;
}

bool rmf_DvbSiMgr::handle_table(uint8_t *data, uint32_t size)
{
    TDvbInterface* dvbInterface((TDvbHelper::GetDvbHelperInstance())->GetDvbInterfaceInstance());
    dvbInterface->ParseSiData(data, size);
    return FALSE;
}

void rmf_DvbSiMgr::cancel_all_filters()
{
    rmf_osal_mutexAcquire(g_sitp_psi_mutex);

    for(auto fiter = m_dvb_filter_list.begin();fiter != m_dvb_filter_list.end();fiter++)
    {
        rmf_FilterInfo *pFilterInfo = *fiter;
        m_pInbSectionFilter->ReleaseFilter(pFilterInfo->filterId);
        rmf_osal_memFreeP(RMF_OSAL_MEM_SI_INB, pFilterInfo);
    }

    m_dvb_filter_list.clear();

    rmf_InbSiMgr::cancel_all_filters();

    rmf_osal_mutexRelease(g_sitp_psi_mutex);
}

rmf_Error rmf_DvbSiMgr::pause(void)
{
    rmf_Error result;
    rmf_Error ret = RMF_INBSI_SUCCESS;

    // it's a recursive mutex
    rmf_osal_mutexAcquire(g_sitp_psi_mutex);
    if(!isPaused)
    {
        for(auto filter = m_dvb_filter_list.begin(); filter != m_dvb_filter_list.end(); ++filter)
        {
            result = m_pInbSectionFilter->pause((*filter)->filterId);
            if(RMF_SECTFLT_Success != result)
            {
                ret = result;
            }
        }
    }

    result = rmf_InbSiMgr::pause();
    if(RMF_SECTFLT_Success != result)
    {
        ret = result;
    }

    rmf_osal_mutexRelease(g_sitp_psi_mutex);

    return ret;
}

rmf_Error rmf_DvbSiMgr::resume(void)
{
    rmf_Error result;
    rmf_Error ret = RMF_INBSI_SUCCESS;

    // it's a recursive mutex
    rmf_osal_mutexAcquire(g_sitp_psi_mutex);
    if(isPaused)
    {
        for(auto filter = m_dvb_filter_list.begin(); filter != m_dvb_filter_list.end(); ++filter)
        {
            result = m_pInbSectionFilter->resume((*filter)->filterId);
            if(RMF_SECTFLT_Success != result)
            {
                ret = result;
            }
        }
    }

    result = rmf_InbSiMgr::resume();
    if(RMF_SECTFLT_Success != result)
    {       
        ret = result; 
    }

    rmf_osal_mutexRelease(g_sitp_psi_mutex);

    return ret;
}

rmf_Error rmf_DvbSiMgr::set_filter(rmf_PsiParam *psiParam, uint32_t *pFilterId)
{
    if(psiParam->table_type != TABLE_TYPE_DVB)
    {
        return rmf_InbSiMgr::set_filter(psiParam, pFilterId);
    }

    rmf_FilterSpec filterSpec;
    rmf_Error ret;
    uint8_t tid_mask[] = { 0xFF, 0, 0, 0, 0, 0x01 };
    uint8_t tid_val[]  = { psiParam->table_id, 0, 0, 0, 0, 0x01 };
    uint8_t ver_mask[] = { 0, 0, 0, 0, 0, 0x3E };
    uint8_t ver_val[]  = { 0, 0, 0, 0, 0, (INIT_TABLE_VERSION << 1) };

    // Acquire Mutex
    rmf_osal_mutexAcquire(g_sitp_psi_mutex);

    // Set the filterspec values
    filterSpec.neg.mask = ver_mask;
    filterSpec.neg.vals = ver_val;
    filterSpec.pos.vals = tid_val;
    filterSpec.pos.mask = tid_mask;

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.DVBSIMGR",
            "<%s: %s> - Setting a DVB table filter: pid = 0x%x, table_id = 0x%x, table_id_ext = 0x%x\n",
            PSIMODULE, __FUNCTION__, psiParam->pid, psiParam->table_id, psiParam->table_id_extension);
    if(psiParam->table_id == 0)
    {
        tid_mask[0] = 0;
    }

    if(psiParam->table_id_extension)
    {
        tid_val[3] = psiParam->table_id_extension >> 8;
        tid_val[4] = psiParam->table_id_extension;
        filterSpec.pos.length = 5;
    }
    else
    {
        filterSpec.pos.length = 1;
    }

    if(psiParam->version_number == INIT_TABLE_VERSION)
    {
        filterSpec.neg.length = 0;
        RDK_LOG(RDK_LOG_TRACE2, "LOG.RDK.DVBSIMGR",
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
                "LOG.RDK.DVBSIMGR",
                "<%s: %s> - Setting a neg version filter: version == %d\n",
                PSIMODULE, __FUNCTION__, psiParam->version_number);
    }

    // Can't create two filters for the same pid due to an existing limitation in rmf_SectionFilter_INB.
    // Need to check if there's a filter on that pid already or not.
    //  Consider changing rmf_SectionFilter_INB's implementation (ModifyFilter()).
    list<rmf_FilterInfo*>::iterator it;
    for(it = m_dvb_filter_list.begin(); it != m_dvb_filter_list.end(); ++it)
    {
        rmf_FilterInfo *pFilterInfo = *it;
        if(pFilterInfo->pid == psiParam->pid)
        {
            RDK_LOG(RDK_LOG_INFO ,"LOG.RDK.DVBSIMGR", "<%s> -  Found filter 0x%x with pid 0x%x\n",
                    __FUNCTION__, pFilterInfo->filterId, pFilterInfo->pid);
            RDK_LOG(RDK_LOG_INFO ,"LOG.RDK.DVBSIMGR", "<%s> -  Releasing filter 0x%x\n",
                    __FUNCTION__, pFilterInfo->filterId);
            m_pInbSectionFilter->ReleaseFilter(pFilterInfo->filterId);
            m_dvb_filter_list.remove(pFilterInfo);
            rmf_osal_memFreeP(RMF_OSAL_MEM_SI_INB, pFilterInfo);

            // Let's reset the filter spec to pass all tables on the pid
            tid_val[0] = 0;
            tid_mask[0] = 0;
            filterSpec.pos.length = 1;
            filterSpec.pos.vals = tid_val;
            filterSpec.pos.mask = tid_mask;
            filterSpec.neg.length = 0;

            break;
        }
    }

    ret = m_pInbSectionFilter->SetFilter(psiParam->pid, &filterSpec, m_FilterQueueId,
                                         psiParam->priority, 0, 0, pFilterId);
    if(ret == RMF_INBSI_SUCCESS)
    {
        rmf_FilterInfo *pFilterInfo = NULL;
        if(rmf_osal_memAllocP(RMF_OSAL_MEM_SI_INB, sizeof(rmf_FilterInfo), (void **) &pFilterInfo) != RMF_INBSI_SUCCESS)
        {
            rmf_osal_mutexRelease(g_sitp_psi_mutex);
            return RMF_INBSI_OUT_OF_MEM;
        }

        memset(pFilterInfo, 0, sizeof(rmf_FilterInfo));

        pFilterInfo->pid = psiParam->pid;
        pFilterInfo->filterId = *pFilterId;
        m_dvb_filter_list.push_back(pFilterInfo);
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBSIMGR",
                "<%s: %s> - SetFilter Failed with error: %d\n",
                PSIMODULE, __FUNCTION__, ret);
    }

    rmf_osal_mutexRelease(g_sitp_psi_mutex);

    return ret;
}
