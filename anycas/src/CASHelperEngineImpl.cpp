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

#include <cstdio>
#include "CASHelperEngineImpl.h"
#include "rdk_debug.h"

namespace anycas {


CASHelperEngineImpl::CASHelperEngineImpl() {
}

CASHelperEngineImpl::~CASHelperEngineImpl() {
}

CASHelperEngine& CASHelperEngine::getInstance() {
	return CASHelperEngineImpl::getInstance();
}

CASHelperEngineImpl& CASHelperEngineImpl::getInstance() {
	static CASHelperEngineImpl impl_;
	return impl_;
}


std::shared_ptr<CASHelperFactory> CASHelperEngineImpl::findFactory(const std::vector<uint8_t>& pat, const std::vector<uint8_t>& pmt, const std::vector<uint8_t>& cat,
		const CASEnvironment& env) {
	for (auto factory : factories_) {
		if (factory->isSupported(pat, pmt, cat, env)) {
			return factory;
		}
	}
	return nullptr;
}

std::shared_ptr<CASHelperFactory> CASHelperEngineImpl::findFactory(const std::string& ocdmSystemId, const CASEnvironment& env) {
	for (auto factory : factories_) {
		if (ocdmSystemId == factory->ocdmSystemId()) {
			return factory;
		}
	}
	return nullptr;
}

void CASHelperEngineImpl::registerFactory(std::shared_ptr<CASHelperFactory> factory) {
	factories_.push_back(factory);
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Registered Factory for %s, now have %d factories stored\n", __FUNCTION__, __LINE__, factory->ocdmSystemId().c_str(), factories_.size());
}

} // namespace
