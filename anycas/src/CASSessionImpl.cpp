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

#include "CASSessionImpl.h"
#include "CASStatus.h"
#include "rdk_debug.h"
#include "CASManagerImpl.h"
#include "CASSectionFilterImpl.h"
#include <algorithm>

namespace anycas {

CASSessionImpl::CASSessionImpl(struct OpenCDMSession* openCdmSession, std::shared_ptr<CASHelperContext> context, std::shared_ptr<CASHelper> helper, std::shared_ptr<CASManagerImpl>manager)
: openCdmSession_(openCdmSession)
, helperContext_(context)
, helper_ (std::weak_ptr<CASHelper>(helper))
, casManager_(manager) {
}

CASSessionImpl::~CASSessionImpl() {
	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.ANYCAS", "[%s:%d] Destructor in Session\n", __FUNCTION__, __LINE__);

	for(auto filter : filterList_) {
	    std::shared_ptr<CASSectionFilterImpl> filterImpl = std::dynamic_pointer_cast<CASSectionFilterImpl>(filter);
	    if(filterImpl) {
	        filterImpl->destroySectionFilter();
	    }
	    else {
	        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Invalid filter instance - %p\n", __FUNCTION__, __LINE__, filter);
	    }
	}
	filterList_.clear();

	if(openCdmSession_)
	{
	    OpenCDMError status = opencdm_session_close(openCdmSession_);
	    if (status == OpenCDMError::ERROR_NONE)
	    {
	        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.ANYCAS", "[%s:%d] opencdm_session_close Success\n", __FUNCTION__, __LINE__);
		status = opencdm_destruct_session(openCdmSession_);
		if (status == OpenCDMError::ERROR_NONE)
		{
		    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.ANYCAS", "[%s:%d] opencdm_destruct_session Success\n", __FUNCTION__, __LINE__);
		}
		else
		{
		    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.ANYCAS", "[%s:%d] opencdm_destruct_session Failed status = %d\n", __FUNCTION__, __LINE__, status);
		}
	    }
	}
	else
	{
	    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.ANYCAS", "[%s:%d] NULL openCdmSession_ \n", __FUNCTION__, __LINE__);
	}
	if(openCdmSession_)
	{
            openCdmSession_ = NULL;
	}
}

std::shared_ptr<CASSectionFilter> CASSessionImpl::createSectionFilter(const CASSectionFilterParam & param, std::shared_ptr<CASHelperContext> context) {
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] createSectionFilter in CASSession\n", __FUNCTION__, __LINE__);
	auto helper = helper_.lock();
	if(!helper)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.ANYCAS", "[%s:%d] helper is NULL\n", __FUNCTION__, __LINE__);
		return nullptr;
	}

	std::shared_ptr<CASSectionFilter> filter = casManager_->createSectionFilter(param, this, context);
	if(filter) {
		filterList_.push_back(filter);
	}

	return filter;
}

void CASSessionImpl::destroySectionFilter(std::shared_ptr<CASSectionFilter> filter) {
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] destroySectionFilter in CASSession\n", __FUNCTION__, __LINE__);

	std::shared_ptr<CASSectionFilterImpl> filterImpl = std::dynamic_pointer_cast<CASSectionFilterImpl>(filter);
	if(filterImpl) {
	    filterImpl->destroySectionFilter();
	    auto position = std::find(filterList_.begin(), filterList_.end(), filter);
	    if (position != filterList_.end()) // == filterList_.end() means the element was not found
	    {
	        filterList_.erase(position);
	    }
	}
	else {
	    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Invalid filter instance - %p\n", __FUNCTION__, __LINE__, filter);
	}
}

void CASSessionImpl::startSectionFilter(std::shared_ptr<CASSectionFilter> filter) {
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] startSectionFilter in CASSession\n", __FUNCTION__, __LINE__);

	std::shared_ptr<CASSectionFilterImpl> filterImpl = std::dynamic_pointer_cast<CASSectionFilterImpl>(filter);
	if(filterImpl) {
	    filterImpl->startSectionFilter();
	}
	else {
	    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Invalid filter instance - %p\n", __FUNCTION__, __LINE__, filter);
	}
}

void CASSessionImpl::updatePSI(const std::vector<uint8_t>& data) {
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] updatePSI in Session\n", __FUNCTION__, __LINE__);
	OpenCDMError status = opencdm_session_update(openCdmSession_, data.data(), data.size());
	if (status == OpenCDMError::ERROR_NONE)
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Successfully Update the psidata\n", __FUNCTION__, __LINE__);
	else
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Failed to update psidata. status = %d\n", __FUNCTION__, __LINE__, status);

}

void CASSessionImpl::updateCasStatus(const CASSessionStatus& status) {
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] updateCasStatus in Session\n", __FUNCTION__, __LINE__);
	auto helper = helper_.lock();
	if(helper){
		helper->updateCasStatus(helperContext_, status);
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] helper_->updateCasStatus returns\n", __FUNCTION__, __LINE__);
	}
	else {
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.ANYCAS", "[%s:%d] helper is NULL\n", __FUNCTION__, __LINE__);
	}
}

void CASSessionImpl::close() {
	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.ANYCAS", "[%s:%d] close session\n", __FUNCTION__, __LINE__);
	if(openCdmSession_)
	{
	   OpenCDMError status = opencdm_session_close(openCdmSession_);
           if (status == OpenCDMError::ERROR_NONE)
           {
	       RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.ANYCAS", "[%s:%d] opencdm_session_close Success \n", __FUNCTION__, __LINE__);
           }
           else
	   {
	       RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.ANYCAS", "[%s:%d] Failed opencdm_session_close status = %d\n", __FUNCTION__, __LINE__, status);
	   }
	}
	else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.ANYCAS", "[%s:%d] NULL openCdmSession_ \n", __FUNCTION__, __LINE__);
        }
}

void CASSessionImpl::parseSectionFilterData(uint16_t pid, std::shared_ptr<CASHelperContext> context, const std::vector<uint8_t>& data){
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] parseSectionFilterData in CASSession\n", __FUNCTION__, __LINE__);

	auto helper = helper_.lock();
	if(!helper) {
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.ANYCAS", "[%s:%d] helper is NULL\n", __FUNCTION__, __LINE__);
		return;
	}

	std::vector<uint8_t> parsed;
	bool ret = helper->parseSectionFilterData(pid, context, data, parsed);
	if(ret && parsed.size()) {
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] parsed[0] = %d, parsed_size = %d\n", __FUNCTION__, __LINE__, parsed[0], parsed.size());
		OpenCDMError status = opencdm_session_update(openCdmSession_, parsed.data(), parsed.size());
		if (status == OpenCDMError::ERROR_NONE)
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Successfully Update the sfData\n", __FUNCTION__, __LINE__);
		else
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Failed to update sfData. status = %d\n", __FUNCTION__, __LINE__, status);
	}
	else
	{
		//TODO : This block is demo code for mocking the updateCasStatus.
		//		 it should be removed, once it is handled based on OnErrorMessages
		CASSessionStatus casStatus;
		casStatus.cond = CAS_CONDITION_OK;
		casStatus.errorNo = 0;
		std::string temp("Failed with Section Filter Data");
		casStatus.message = temp;
		updateCasStatus(casStatus);
	}
}

bool CASSessionImpl::processNetworkRequest(const std::string& destUrl, const std::vector<uint8_t>& dataIn, std::vector<uint8_t>& dataOut) {
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] processNetworkRequest called for session\n", __FUNCTION__, __LINE__);

	auto helper = helper_.lock();
	if(!helper) {
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.ANYCAS", "[%s:%d] helper is NULL\n", __FUNCTION__, __LINE__);
		return false;
	}

	bool ret = helper->processNetworkRequest(helperContext_, destUrl, dataIn, dataOut);

	if(!ret) {
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Failed to process network request\n", __FUNCTION__, __LINE__);
	}
	else{
		OpenCDMError status = opencdm_session_update(openCdmSession_, dataOut.data(), dataOut.size());

		if (status == OpenCDMError::ERROR_NONE) {
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Successfully Update the Network Response\n", __FUNCTION__, __LINE__);
		}
		else {
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Failed to update Network response. status = %d\n", __FUNCTION__, __LINE__, status);
		}
	}

	return ret;
}

void CASSessionImpl::processData(bool isPrivate, const std::vector<uint8_t>& data) {
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] processData called for session\n", __FUNCTION__, __LINE__);

	auto helper = helper_.lock();
	if(helper){
		helper->processData(helperContext_, isPrivate, data);
	}
	else {
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.ANYCAS", "[%s:%d] helper is NULL\n", __FUNCTION__, __LINE__);
	}
}

void CASSessionImpl::processSectionFilterCommand(std::string const& params) {
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] processSectionFilterCommand called for session\n", __FUNCTION__, __LINE__);

	auto helper = helper_.lock();
	if(helper){
		helper->processSectionFilterCommand(helperContext_, params);
	}
	else {
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.ANYCAS", "[%s:%d] helper is NULL\n", __FUNCTION__, __LINE__);
	}
}
void CASSessionImpl::setState(CASSessionState state){
	for(auto filter : filterList_) {
	    std::shared_ptr<CASSectionFilterImpl> filterImpl = std::dynamic_pointer_cast<CASSectionFilterImpl>(filter);
	    if(filterImpl) {
		    filterImpl->setState(state);
	    }
	    else {
		    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Invalid filter instance - %p\n", __FUNCTION__, __LINE__, filter);
	    }
	}
}

bool CASSessionImpl::sendData(bool, const std::vector<uint8_t> &data){
	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.ANYCAS", "[%s:%d] sendData called for session\n", __FUNCTION__, __LINE__);
	std::string strData;
	strData.assign(data.begin(), data.end());
	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.ANYCAS", "[%s:%d] Recieved Data = %s\n", __FUNCTION__, __LINE__, strData.c_str());
	OpenCDMError status = opencdm_session_update(openCdmSession_, data.data(), data.size());
	if (status == OpenCDMError::ERROR_NONE) {
		RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.ANYCAS", "[%s:%d] Successfully Update key message [public data]\n", __FUNCTION__, __LINE__);
	}
	else {
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.ANYCAS", "[%s:%d] Failed to update key message [public data]. status = %d\n", __FUNCTION__, __LINE__, status);
	}
	return true;
}

} // namespace
