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

#ifndef CASMANAGER_H
#define CASMANAGER_H

#include <memory>
#include <vector>
#include <cstdint>

#include "CASEnvironment.h"
#include "CASHelper.h"
#include "ICasSectionFilter.h"
#include "CASSectionFilter.h"
#include "CASSectionFilterParam.h"
#include "CASHelperContext.h"
#include "ICasPipeline.h"

namespace anycas {

class CASManager {
public:
	/**
	 * Create a Helper instance based on the detected CAS
	 * @param pat An array of PAT data
	 * @param pmt An array of PMT data
	 * @param cat An array of CAT data
	 * @param env The environment the system will be used in @see CASEnvironment
	 * @return A pointer to @see CASHelper or nullptr if one cannot be created
	 */
	virtual std::shared_ptr<CASHelper> createHelper(const std::vector<uint8_t>& pat, const std::vector<uint8_t>& pmt, const std::vector<uint8_t>& cat,
		const CASEnvironment& env) = 0;

	/**
	 * Create a Helper instance based on the requested CAS
	 * @param ocdmSystemId The OCDM System ID of the CAS to create
	 * @param env The environment the system will be used in @see CASEnvironment
	 * @return A pointer to @see CASHelper or nullptr if one cannot be created
	 */
	virtual std::shared_ptr<CASHelper> createHelper(const std::string& ocdmSystemId,
		const CASEnvironment& env) = 0;

	/**Create a section filter
	 *
	 * @param param The source and SF specs to create @see CASSectionFilter
	 * @param session The session requesting the section filter
	 * @param context An anonymous class used by the caller to identify returned data
	 * @return a pointer to @see CASSectionFilter
	 */
	virtual std::shared_ptr<CASSectionFilter> createSectionFilter(const CASSectionFilterParam& param, CASSession *session,
		std::shared_ptr<CASHelperContext> context) = 0;


	/**
	 * Control streams from the source, for example for ECMs to send to the demux etc.
	 * Media PIDs referenced in the PMT cannot be changed
	 * @param pidsToSend unreferenced PIDS to forward to the sink
	 * @param pidsToBlock unreferenced PIDSs previously requested to block
	 */
	virtual void controlStreams(std::vector<uint16_t>& pidsToSend, std::vector<uint16_t>& pidsToBlock) = 0;
	static std::shared_ptr<CASManager> createInstance(ICasSectionFilter *streamFilter, ICasPipeline *pipeline);
	virtual void processData(const uint32_t& filterId, const std::vector<uint8_t>& data) = 0;
	virtual ~CASManager() {};
protected:
	CASManager() {};

};

} // namespace

#endif /* CASMANAGER_H */

