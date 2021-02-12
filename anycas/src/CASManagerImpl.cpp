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

#include <map>
#include <future>
#include <cstdio>

#include "open_cdm.h"

#include "CASManagerImpl.h"
#include "CASSystemImpl.h"
#include "CASHelperEngineImpl.h"
#include "rdk_debug.h"
#include "CASSystemFactory.h"
#include "CASSessionImpl.h"
#include "CASSectionFilterImpl.h"

namespace anycas {

std::shared_ptr<CASManager> CASManager::createInstance(ICasSectionFilter *streamFilter, ICasPipeline *pipeline) {
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] createInstance in CASManager\n", __FUNCTION__, __LINE__);
    std::shared_ptr<CASManager> impl(new CASManagerImpl(streamFilter, pipeline));
    return impl;
}

std::shared_ptr<CASHelper> CASManagerImpl::createHelper(const std::vector<uint8_t>& pat, const std::vector<uint8_t>& pmt, const std::vector<uint8_t>& cat, const CASEnvironment& env){
	auto factory = CASHelperEngineImpl::getInstance().findFactory(pat, pmt, cat, env);
	
	if (factory == nullptr) {
		return nullptr;
	}

	createSystem(factory->ocdmSystemId());

	auto helper = factory->createInstance(system_, this, pat, pmt, cat, env);

	if (!helper) {
		return nullptr;
	}
	system_->setHelper(helper);

	return helper;
}

std::shared_ptr<CASHelper> CASManagerImpl::createHelper(const std::string& ocdmSystemId, const CASEnvironment& env) {

	auto factory = CASHelperEngineImpl::getInstance().findFactory(ocdmSystemId, env);

	if (factory == nullptr) {
		return nullptr;
	}

	std::vector<uint8_t> fakePsi;
	createSystem(factory->ocdmSystemId());

	auto helper = factory->createInstance(system_, this, fakePsi, fakePsi, fakePsi, env);

	if (!helper) {
		return nullptr;
	}
	system_->setHelper(helper);

	//This is to create the Management Session which can be used by CAS Helper
	if(env.getUsageMode() == CASUsageMode::CAS_USAGE_NULL &&
	   (env.getUsageManagement() == CASUsageManagement::CAS_USAGE_MANAGEMENT_FULL ||
	   env.getUsageManagement() == CASUsageManagement::CAS_USAGE_MANAGEMENT_NO_PSI ||
	   env.getUsageManagement() == CASUsageManagement::CAS_USAGE_MANAGEMENT_NO_TUNE))
	{
		helper->createSession();
	}
	return helper;
}

void CASManagerImpl::createSystem(const std::string& ocdmSystemId) {

	system_ = CASSystemFactory::getSystem(ocdmSystemId);
	system_->setManager(shared_from_this());
}

std::shared_ptr<CASSectionFilter> CASManagerImpl::createSectionFilter(const CASSectionFilterParam& param, CASSession *session,
		std::shared_ptr<CASHelperContext> context) {
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] createSectionFilter in CASManager\n", __FUNCTION__, __LINE__);

    uint16_t pid = param.pid;
    ICasFilterParam_ casFilterParam;

	casFilterParam.pos_size = param.pos_value.size();
	if(casFilterParam.pos_size) {
		std::copy(std::begin(param.pos_value), std::end(param.pos_value), std::begin(casFilterParam.pos_value));
		std::copy(std::begin(param.pos_mask), std::end(param.pos_mask), std::begin(casFilterParam.pos_mask));
	}

	casFilterParam.neg_size = param.neg_value.size();
	if(casFilterParam.neg_size) {
		std::copy(std::begin(param.neg_value), std::end(param.neg_value), std::begin(casFilterParam.neg_value));
		std::copy(std::begin(param.neg_mask), std::end(param.neg_mask), std::begin(casFilterParam.neg_mask));
	}

	casFilterParam.disableCRC = true;
	casFilterParam.noPaddingBytes = true;
	CASSectionFilterParam::FilterMode filterMode = param.mode;
	if(filterMode == CASSectionFilterParam::FilterMode::SECTION_FILTER_ONE_SHOT)
        {
                casFilterParam.mode = ICasFilterParam::SECTION_FILTER_ONE_SHOT;
        }
        else
        {
                casFilterParam.mode = ICasFilterParam::SECTION_FILTER_REPEAT;
        }

	std::shared_ptr<CASSectionFilterImpl> filter = std::make_shared<CASSectionFilterImpl>(context, upstreamFilter_);
    std::shared_ptr<CASSectionFilterResponse> filter_response = std::make_shared<CASSectionFilterResponse>(session, context, filterMode, pid);

	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d]  pos_size = %d, neg_size = %d\n", __FUNCTION__, __LINE__, casFilterParam.pos_size, casFilterParam.neg_size);
	if(filter_response)
	{
	    ICasHandle *casHandle = filter_response.get();
	    upstreamFilter_->create(pid, casFilterParam,  &casHandle);

            /*TODO handle error cases here*/
	    if((filter_response.get())->filterId != 0) {
			filter->setFilterResponse(filter_response);
	        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d]  return the CASSectionFilter...\n", __FUNCTION__, __LINE__);
	    }
	    else {
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d]  return nullptr... - filterId - zero\n", __FUNCTION__, __LINE__);
			return nullptr;
	    }
    }

    return filter;
}

bool CASManagerImpl::pipelineDetails(uint16_t pid, std::vector<uint8_t> data, std::string& command, uint32_t& returnCode) {
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] pipelineDetails in CASManager\n", __FUNCTION__, __LINE__);
	if(command == "write-keys")
	{
		returnCode = pipeline_->setKeySlot(pid, data);
	}
	else if(command == "delete-keys")
	{
		returnCode = pipeline_->deleteKeySlot(pid);
	}
	if(returnCode) {
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Failed to set the keyslot in pipeline\n", __FUNCTION__, __LINE__);
		return false;
	}
	return true;
};

void CASManagerImpl::controlStreams(std::vector<uint16_t>& pidsToSend, std::vector<uint16_t>& pidsToBlock) {
}

void CASManagerImpl::processData(const uint32_t& filterId, const std::vector<uint8_t>& data) {
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] ProcessData Called in CASManager\n", __FUNCTION__, __LINE__);
	std::shared_ptr<CASSectionFilterResponse> filterResp = CASSectionFilterImpl::getFilterResponse(filterId);
	if(filterResp) {
		CASSessionImpl *casSession = (CASSessionImpl*) (filterResp->getSession());
		casSession->parseSectionFilterData(filterResp->getPid(), filterResp->getContext(), data);
	}
}

CASManagerImpl::~CASManagerImpl() {
	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.ANYCAS", "[%s:%d] Destructor in CASManagerImpl\n", __FUNCTION__, __LINE__);
}

} // namespace
