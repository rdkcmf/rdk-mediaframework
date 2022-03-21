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
#include "rmfdvbsrc.h"

#include <sstream>
#include <string>


#include "rmfdvbsrcpriv.h"
#ifdef USE_DVBSI
#include "rmf_dvbsimgr.h"
#include "TDvbHelper.h"
#endif

#define DVBPARAMS_PRIFIX_STRING "dvb://"

RMFDVBSrc::RMFDVBSrc()
{
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.DVBSRC", " %s():%d\n", __FUNCTION__, __LINE__);
}

RMFDVBSrc::~RMFDVBSrc()
{
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.DVBSRC", " %s():%d\n", __FUNCTION__, __LINE__);
}

bool RMFDVBSrc::isDvb(const char *locator)
{
    if((locator == NULL) || (strstr(locator, "dvb") == NULL))
    {
        return false;
    }

    return true;
}

RMFMediaSourcePrivate* RMFDVBSrc::createPrivateSourceImpl()
{
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.DVBSRC", " %s():%d\n", __FUNCTION__, __LINE__);
    return new RMFDVBSrcImpl(this);
}

RMFResult RMFDVBSrc::getProgramInfo(const char *uri, rmf_ProgramInfo_t *pInfo, uint32_t *pDecimalSrcId) 
{
    RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.DVBSRCPRIV","%s RMFDVBSrcImpl::getProgramInfo called as expected..\n", __FUNCTION__);  
    RMFResult ret;
    char *tmp =  strstr ( (char* )uri, DVBPARAMS_PRIFIX_STRING );
    if ( NULL != tmp )
    {
        std::istringstream iss(uri);
        std::string val_str;
        getline(iss, val_str, '.');
        if ( val_str.empty() )
        {
            RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.DVBSRCPRIV", "open: invalid uri : %s does not contain \"onid\"\n", uri );
            return RMF_RESULT_INVALID_ARGUMENT;
        }
        uint16_t onid = atoi(val_str.c_str() + strlen(DVBPARAMS_PRIFIX_STRING));

        getline(iss, val_str, '.');
        if ( val_str.empty() )
        {
            RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.DVBSRCPRIV", "open: invalid uri : %s does not contain \"tsid\"\n", uri );
            return RMF_RESULT_INVALID_ARGUMENT;
        }
        uint16_t tsid = atoi(val_str.c_str());

        getline(iss, val_str, '.');
        if ( val_str.empty() )
        {
            RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.DVBSRCPRIV", "open: invalid uri : %s does not contain \"sid\"\n", uri );
            return RMF_RESULT_INVALID_ARGUMENT;
        }
        uint16_t sid = atoi(val_str.c_str());

        RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.DVBSRCPRIV", "open: dvb locator = dvb://%d.%d.%d\n", onid, tsid, sid );

        pInfo->carrier_frequency = 0;
        pInfo->modulation_mode = RMF_MODULATION_UNKNOWN;
        pInfo->prog_num = 0;
        pInfo->symbol_rate = 0;
#ifdef USE_DVBSI
        TDvbInterface* dvbInterface((TDvbHelper::GetDvbHelperInstance())->GetDvbInterfaceInstance());
        std::vector<std::shared_ptr<TDvbStorageNamespace::TStorageTransportStreamStruct>> tsList = dvbInterface->GetTsListByNetId(0);

        for(auto it = tsList.begin(), end = tsList.end(); it != end; ++it)
        {
            if((*it)->NetworkId == onid && (*it)->TransportStreamId == tsid)
            {
                pInfo->carrier_frequency = (*it)->Frequency;
                pInfo->modulation_mode = dvbInterface->GetMappedModulation((*it)->Modulation);
                pInfo->prog_num = sid;
                pInfo->symbol_rate = (*it)->SymbolRate;

                return RMF_RESULT_SUCCESS;
            }
        }
#endif
        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.DVBSRCPRIV", "open: failed to resolve dvb://%d.%d.%d\n", onid, tsid, sid );
        return RMF_RESULT_FAILURE;
    }
    else
    {
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.DVBSRCPRIV", "[RMFDVBSrcImpl] %s() URI is linear or autodiscovery service undesired\n", __FUNCTION__);
        ret = RMFQAMSrcImpl::getProgramInfo(uri, pInfo, pDecimalSrcId);
    }
    return ret;
}
