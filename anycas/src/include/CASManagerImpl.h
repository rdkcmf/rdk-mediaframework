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

#ifndef CASMANAGERIMPL_H
#define CASMANAGERIMPL_H

#include "CASManager.h"
#include "CASPipelineController.h"
#include "CASSessionImpl.h"
#include "CASSystemImpl.h"
#include "CASSectionFilterResponse.h"

namespace anycas {

class CASManagerImpl : public std::enable_shared_from_this<CASManagerImpl>, public CASManager, CASPipelineController {
public:
	virtual std::shared_ptr<CASSectionFilter> createSectionFilter(const CASSectionFilterParam& param, CASSession *session,
		std::shared_ptr<CASHelperContext> context) override;

	virtual std::shared_ptr<CASHelper> createHelper(const std::vector<uint8_t>& pat, const std::vector<uint8_t>& pmt, const std::vector<uint8_t>& cat,
		const CASEnvironment& env) override;

	virtual std::shared_ptr<CASHelper> createHelper(const std::string& ocdmSystemId,
		const CASEnvironment& env) override;

	virtual void controlStreams(std::vector<uint16_t>& pidsToSend, std::vector<uint16_t>& pidsToBlock) override;

	// Pipeline Controller implementation

	virtual bool pipelineDetails(uint16_t pid, std::vector<uint8_t> data, std::string& command, uint32_t& returnCode) override;

	virtual void processData(const uint32_t& filterId, const std::vector<uint8_t>& data) override;

	virtual ~CASManagerImpl();

private:
	ICasSectionFilter *upstreamFilter_;
	ICasPipeline *pipeline_;
	void createSystem(const std::string& ocdmSystemId);

	std::shared_ptr<CASSystemImpl> system_;
	CASManagerImpl(ICasSectionFilter *streamFilter, ICasPipeline *pipeline): upstreamFilter_(streamFilter), pipeline_(pipeline) {}
	friend class CASManager;
};

} // namespace

#endif /* CASMANAGERIMPL_H */

