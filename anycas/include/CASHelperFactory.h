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

#ifndef CASHELPERFACTORY_H
#define CASHELPERFACTORY_H

#include <vector>
#include <memory>

#include "CASEnvironment.h"
#include "CASHelper.h"
#include "CASPipelineController.h"

namespace anycas {

class CASHelperFactory {
public:
	/**
	 * Test if a Factory supports the detected CAS
	 * @param pat the received PAT
	 * @param pmt the received PMT
	 * @param cat the received CAT
	 * @param env the provided env see @CASEnvironment
	 * @return true if the Factory can create a Helper that supports that CAS
	 */
	virtual bool isSupported(const std::vector<uint8_t>& pat, const std::vector<uint8_t>& pmt, const std::vector<uint8_t>& cat,
		const CASEnvironment& env) const = 0;

	/**
	 * Create a Helper to assist in descrambling the media
	 * @param system the CAS System this helper will use
	 * @param pipeline an interface to control the pipeline
	 * @param pat the received PAT
	 * @param pmt the received PMT
	 * @param cat the received CAT
	 * @param env the provided env see @CASEnvironment
	 * @return a @see CASHelper or nullptr if this fails
	 */
	virtual std::shared_ptr<CASHelper> createInstance(std::shared_ptr<CASSystem> system, CASPipelineController* pipeline, const std::vector<uint8_t>& pat, const std::vector<uint8_t>& pmt, const std::vector<uint8_t>& cat,
		const CASEnvironment& env) = 0;

	/**
	 * Get the OCDM System ID used when creating a System
	 * @return the Ocdm System Id
	 */
	virtual const std::string& ocdmSystemId() const = 0;
public:
	CASHelperFactory() {}
	virtual ~CASHelperFactory() {}
};

} // namespace

#endif /* CASHELPERFACTORY_H */

