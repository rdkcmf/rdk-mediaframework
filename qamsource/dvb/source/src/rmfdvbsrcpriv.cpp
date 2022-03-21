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

#include "rmfdvbsrcpriv.h"

// C++ system includes
#include <sstream>
#include <string>

#ifdef USE_DVBSI
// Project's includes
#include "rmf_dvbsimgr.h"
#include "TDvbHelper.h"
#endif


RMFDVBSrcImpl::RMFDVBSrcImpl(RMFMediaSourceBase *parent)
    : RMFQAMSrcImpl(parent)
{
}

RMFDVBSrcImpl::~RMFDVBSrcImpl()
{
}


RMFResult RMFDVBSrcImpl::open(const char *uri, char *mimetype)
{
   RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBSRCPRIV", "%s: RMFDVBSrcImpl::open() Called \n", __FUNCTION__);
    RMFResult ret =  RMFQAMSrcImpl::open(uri, mimetype);
    if(RMF_SUCCESS != ret)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBSRCPRIV",
            "%s: RMFDVBSrcImpl::open() failed, ret = 0x%x\n", __FUNCTION__, ret);
        return ret;
    }

   RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBSRCPRIV", "%s: Base Open Success \n", __FUNCTION__);

#ifdef USE_DVBSI
    rmf_DvbSiMgr* pDvbSiMgr = static_cast<rmf_DvbSiMgr*>(pInbandSI);
    // Registering DvbSiStorage for DVB SI events
    TDvbInterface* dvbInterface((TDvbHelper::GetDvbHelperInstance())->GetDvbInterfaceInstance());
    ret = pDvbSiMgr->RegisterForDvbSiEvents(dvbInterface->GetEventQueueId());
    if(RMF_SUCCESS != ret)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBSRCPRIV",
            "%s: RegisterForDvbSiEvents failed, ret = 0x%x\n", __FUNCTION__, ret);
        // Logging only for now
    }

    // Let's check if we need to request the DVB info or not
    string val_str;
    string uri_str = uri;
    if(RMF_RESULT_SUCCESS == get_querystr_val(uri_str, "dvb", val_str))
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.DVBSRCPRIV", "%s:%d dvb token (%s) found in URI.\n",
            __FUNCTION__, __LINE__, val_str.c_str());
        vector<rmf_PsiParam> filters;
        std::vector<std::shared_ptr<TDvbStorageNamespace::InbandTableInfoStruct>> profile = dvbInterface->GetInbandTableInfo(val_str);
        for(auto it = profile.begin(), end = profile.end(); it != end; ++it)
        {
            rmf_PsiParam psiParam = {};

            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.DVBSRCPRIV", "%s:%d profile: pid = 0x%x, tableId = 0x%x, extensionId = 0x%x\n",
                __FUNCTION__, __LINE__, (*it)->Pid, (*it)->TableId, (*it)->ExtensionId);
            psiParam.pid = (*it)->Pid;
            psiParam.table_id = (*it)->TableId;
            psiParam.table_id_extension = (*it)->ExtensionId;
            psiParam.table_type = TABLE_TYPE_DVB;
            psiParam.version_number = INIT_TABLE_VERSION;
            psiParam.priority = RMF_PSI_FILTER_PRIORITY_INITIAL_DVB;

            filters.push_back(psiParam);
        }
        ret = pDvbSiMgr->RequestDvbInfo(filters);
        if(RMF_SUCCESS != ret)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.DVBSRCPRIV", "%s:%d Failed to request DVB info: 0x%x\n", __FUNCTION__, __LINE__, ret);
            // Logging only (for now)
        }
    }
    else
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.DVBSRCPRIV", "%s:%d No dvb token found in URI.\n", __FUNCTION__, __LINE__);
    }

#endif
    return ret;
}

RMFResult RMFDVBSrcImpl::close()
{
#ifdef USE_DVBSI
    rmf_DvbSiMgr* pDvbSiMgr = static_cast<rmf_DvbSiMgr*>(pInbandSI);

    TDvbInterface* dvbInterface((TDvbHelper::GetDvbHelperInstance())->GetDvbInterfaceInstance());
    pDvbSiMgr->UnRegisterForDvbSiEvents(dvbInterface->GetEventQueueId());
#endif
    return RMFQAMSrcImpl::close();
}

rmf_InbSiMgr* RMFDVBSrcImpl::createInbandSiManager()
{
#ifdef USE_DVBSI
    return new rmf_DvbSiMgr(m_sourceInfo);
#else
    return new rmf_InbSiMgr(m_sourceInfo);
#endif
}
