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

//#include <map>
#include <future>
#include <cstdio>

//#include "open_cdm.h"

#include "CASSectionFilterImpl.h"
//#include "CASSystemImpl.h"
//#include "CASHelperEngineImpl.h"
#include "rdk_debug.h"
//#include "CASSystemFactory.h"
//#include "CASSessionImpl.h"

namespace anycas {

std::vector<std::shared_ptr<CASSectionFilterResponse>> CASSectionFilterImpl::filterResponseList_;

void CASSectionFilterImpl::startSectionFilter() {
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] startSectionFilter in CASSectionFilterImpl\n", __FUNCTION__, __LINE__);
	if(filterResponse_) {
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Found filter response\n", __FUNCTION__, __LINE__);
		if(filterResponse_->getFilterMode() == CASSectionFilterParam::FilterMode::SECTION_FILTER_ONE_SHOT)
		{
			ICasHandle *casHandle = (ICasHandle *)(filterResponse_.get());
			upstreamFilter_->start(casHandle);
		}
	}
}

void CASSectionFilterImpl::setState(CASSessionState state) {
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] setState in CASSectionFilterImpl\n", __FUNCTION__, __LINE__);
	if(filterResponse_) {
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Found filter response\n", __FUNCTION__, __LINE__);
		ICasHandle *casHandle = (ICasHandle *)(filterResponse_.get());
		bool isRunning = (state == RUNNING) ? true : false;
		upstreamFilter_->setState(isRunning, casHandle);
	}
}

void CASSectionFilterImpl::destroySectionFilter() {
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] destroySectionFilter in CASSectionFilterImpl\n", __FUNCTION__, __LINE__);

	if(filterResponse_) {
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Found filter response\n", __FUNCTION__, __LINE__);
		ICasHandle *casHandle = (ICasHandle *)(filterResponse_.get());
		upstreamFilter_->destroy(casHandle);
	}

	uint16_t pos = 0;
	for(auto filterResp : filterResponseList_) {
		if(filterResp == filterResponse_) {
			filterResponseList_.erase(filterResponseList_.begin()+pos);
			break;
		}
		pos++;
	}
}

void CASSectionFilterImpl::setFilterResponse(std::shared_ptr<CASSectionFilterResponse> filterResp) {
	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.ANYCAS", "[%s:%d] setFilterResponse in CASSectionFilterImpl\n", __FUNCTION__, __LINE__);
	filterResponse_ = filterResp;
	filterResponseList_.push_back(filterResponse_);
}

std::shared_ptr<CASSectionFilterResponse> CASSectionFilterImpl::getFilterResponse(const uint32_t& filterId) {
	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.ANYCAS", "[%s:%d] getFilterResponse in CASSectionFilterImpl\n", __FUNCTION__, __LINE__);
	for (auto filterResp : filterResponseList_)
	{
		if(((ICasHandle *)filterResp.get())->filterId == filterId) {
			RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.ANYCAS", "[%s:%d] Found filter response\n", __FUNCTION__, __LINE__);
			return filterResp;
		}
	}
	return nullptr;
}

CASSectionFilterImpl::~CASSectionFilterImpl() {
	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.ANYCAS", "[%s:%d] Destructor in CASSectionFilterImpl\n", __FUNCTION__, __LINE__);
}

} // namespace
