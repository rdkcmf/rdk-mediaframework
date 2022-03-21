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

#ifndef RMFDVBSRCIMPL_H
#define RMFDVBSRCIMPL_H

#include "rmfqamsrcpriv.h"

class RMFDVBSrcImpl: public RMFQAMSrcImpl
{
public:
    RMFDVBSrcImpl(RMFMediaSourceBase *parent);
    ~RMFDVBSrcImpl();

    RMFResult open(const char *uri, char *mimetype);
    RMFResult close();

protected:
    virtual rmf_InbSiMgr* createInbandSiManager();
};

#endif //RMFDVBSRCIMPL_H
