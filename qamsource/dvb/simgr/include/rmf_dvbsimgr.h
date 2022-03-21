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

#ifndef __RMF_DVBSIMGR_H_
#define __RMF_DVBSIMGR_H_

// C++ system includes
#include <list>
#include <vector>

// Other libraries' includes

// Project's includes
#include "rmf_inbsimgr.h"
#include "TSectionParser.h"


/**
 * RMF DVB SI Manager class
 */
class rmf_DvbSiMgr: public rmf_InbSiMgr
{
public:
    rmf_DvbSiMgr(rmf_PsiSourceInfo psiSource);
    ~rmf_DvbSiMgr();

    rmf_osal_eventmanager_handle_t m_EventMgr;
    rmf_Error pause(void);
    rmf_Error resume(void);

    rmf_Error RegisterForDvbSiEvents(rmf_osal_eventqueue_handle_t queueId);
    rmf_Error UnRegisterForDvbSiEvents(rmf_osal_eventqueue_handle_t queueId);
    rmf_Error RequestDvbInfo(std::vector<rmf_PsiParam>& filters);

protected:
    rmf_Error set_filter(rmf_PsiParam *psiParam, uint32_t *filterId);
    virtual bool handle_table(uint8_t *data, uint32_t size);
    void cancel_all_filters(void);

private:
    rmf_DvbSiMgr(const rmf_DvbSiMgr& other);
    rmf_DvbSiMgr& operator=(const rmf_DvbSiMgr&);

    TSectionParser* SectionParser;
    std::list<rmf_FilterInfo*> m_dvb_filter_list;
};

#endif /* __RMF_DVBSIMGR_H_ */
