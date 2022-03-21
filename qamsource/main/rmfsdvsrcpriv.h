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
#ifndef RMF_SDV_SRC_PRIV_H_
#define RMF_SDV_SRC_PRIV_H_

#include "rmfqamsrcpriv.h"
#include "sdv_iarm.h"
#include "SDVSectionFilter.h"
#include "SDVSectionFilterRegistrar.h"
#include "libIBus.h"

class RMFSDVSrcImpl:public RMFQAMSrcImpl
{
public:
    RMFSDVSrcImpl(RMFMediaSourceBase * parent );
    ~RMFSDVSrcImpl();
    RMFResult getSourceInfo( const char *uri, rmf_ProgramInfo_t * pProgramInfo, uint32_t * pDecimalSrcId );

    RMFResult open( const char *uri, char *mimetype );
    RMFResult close(  );
    virtual rmf_osal_Bool readyForReUse(const char* uri = NULL);

private:
    SDVSectionFilter * sdvSectionFilter;

    RMFResult getSdvSourceInfo(const char *uri, rmf_ProgramInfo_t* pProgramInfo, uint32_t* pDecimalSrcId);
    rmf_ModulationMode getSDVModulationMode(uint32_t modulation);
    bool m_isSwitched;

	protected:
    bool isSdvDiscoveryUri(const char * uri);
};
#endif

