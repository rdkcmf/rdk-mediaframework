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

#ifndef CASHELPERENGINEIMPL_H
#define CASHELPERENGINEIMPL_H

#include <vector>

#include "CASHelperEngine.h"

namespace anycas {

class CASHelperEngineImpl : public CASHelperEngine {

public:
	void registerFactory(std::shared_ptr<CASHelperFactory> factory);

	std::shared_ptr<CASHelperFactory> findFactory(const std::vector<uint8_t>& pat, const std::vector<uint8_t>& pmt, const std::vector<uint8_t>& cat,
		const CASEnvironment& env);

	std::shared_ptr<CASHelperFactory> findFactory(const std::string& ocdmSystemId, const CASEnvironment& env);

	static CASHelperEngineImpl& getInstance();

protected:
	CASHelperEngineImpl();
	virtual ~CASHelperEngineImpl();
private:
	std::vector<std::shared_ptr<CASHelperFactory> > factories_;

};

} // namespace

#endif /* CASHELPERENGINEIMPL_H */

