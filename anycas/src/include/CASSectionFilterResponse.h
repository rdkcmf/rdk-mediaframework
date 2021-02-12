/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2021 RDK Management
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
**/

#ifndef CASSECTIONFILTERRESPONSE_H
#define CASSECTIONFILTERRESPONSE_H

#include "CASHelperContext.h"
#include "CASSession.h"
#include "CASSectionFilterParam.h"
#include "ICasHandle.h"


namespace anycas {

class CASSectionFilterResponse : public ICasHandle
{
public:
       CASSectionFilterResponse(CASSession *session, std::shared_ptr<CASHelperContext> context, CASSectionFilterParam::FilterMode filterMode, uint16_t pid)
			: session_(session), context_(context), filterMode_(filterMode), pid_(pid) {};
       ~CASSectionFilterResponse() {};
	   std::shared_ptr<CASHelperContext> getContext() { return context_; }
	   CASSession* getSession() { return session_; }
	   CASSectionFilterParam::FilterMode getFilterMode() { return filterMode_; }
	   uint16_t getPid() { return pid_; }

private:
       CASSession *session_;
	   uint16_t pid_;
	   CASSectionFilterParam::FilterMode filterMode_;
       std::shared_ptr<CASHelperContext> context_;
};

} // namespace

#endif /* CASSECTIONFILTERRESPONSE_H */

