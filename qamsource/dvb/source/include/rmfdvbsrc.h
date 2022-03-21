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

#ifndef RMFDVBSOURCE_H
#define RMFDVBSOURCE_H

#include "rmfqamsrc.h"

struct rmf_ProgramInfo;
typedef rmf_ProgramInfo rmf_ProgramInfo_t;

class RMFDVBSrc: public RMFQAMSrc
{
public:
    /**
     * Constructor
     */
    RMFDVBSrc();

    /**
     * Destructor
     */
    ~RMFDVBSrc();

    /**
     * @brief Determines if a given source URI is a DVB service
     *
     * @return TRUE if the URI reporesent a DVB Service otherwise FALSE
     */
    static bool isDvb(const char *locator);
    
    static RMFResult getProgramInfo(const char *uri, rmf_ProgramInfo_t *pInfo, uint32_t *pDecimalSrcId);

    /**
     * @brief Creates private implementation and return. Used
     * by rmf base implementation
     *
     * @return RMFMediaSourcePrivate*	  Private implemention of DVB Src
     *
     */
    RMFMediaSourcePrivate *createPrivateSourceImpl();
};
#endif // RMFDVBSOURCE_H
