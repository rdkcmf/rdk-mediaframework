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

#ifndef CASSESSIONIMPL_H
#define CASSESSIONIMPL_H

#include "open_cdm.h"

#include "CASSession.h"
#include "CASHelper.h"
#include "CASManager.h"

namespace anycas {

class CASManagerImpl;

class CASSessionImpl : public CASSession {
public:
	CASSessionImpl(struct OpenCDMSession* openCdmSession, std::shared_ptr<CASHelperContext> context, std::shared_ptr<CASHelper> helper, std::shared_ptr<CASManagerImpl>manager);
	virtual ~CASSessionImpl();

	virtual void processSectionFilterCommand(const std::string& jsonParams) override;

	virtual std::shared_ptr<CASSectionFilter> createSectionFilter(const CASSectionFilterParam & param, std::shared_ptr<CASHelperContext> context) override;

	virtual void destroySectionFilter(std::shared_ptr<CASSectionFilter> filter) override;

	virtual void startSectionFilter(std::shared_ptr<CASSectionFilter> filter) override;

	virtual void updatePSI(const std::vector<uint8_t>& data) override;

	virtual void setState(enum CASSessionState state) override;

	virtual bool sendData(bool isPrivate, const std::vector<uint8_t>& data) override;

	virtual void updateCasStatus(const CASSessionStatus& status) override;

	virtual void close() override;

	void parseSectionFilterData(uint16_t pid, std::shared_ptr<CASHelperContext> context, const std::vector<uint8_t>& data);

	virtual bool processNetworkRequest(const std::string& destUrl, const std::vector<uint8_t>& dataIn, std::vector<uint8_t>& dataOut) override;

	virtual void processData(bool isPrivate, const std::vector<uint8_t>& data) override;

	struct OpenCDMSession* ocdmSession() { return openCdmSession_;}
	std::shared_ptr<CASHelperContext> helperContext() { return helperContext_; }

private:
	struct OpenCDMSession* openCdmSession_;
	std::shared_ptr<CASHelperContext> helperContext_;
	std::weak_ptr<CASHelper> helper_;
	std::shared_ptr<CASManagerImpl>casManager_;
	std::vector<std::shared_ptr<CASSectionFilter>> filterList_;
};

} // namespace

#endif /* CASSESSIONIMPL_H */

