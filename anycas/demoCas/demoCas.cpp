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

#include "demoCas.h"
#include "CASHelperEngine.h"

std::string DEMO_SERVER_CERTIFICATE { "Of course you can"};

const std::string& DemoCasHelper::serverCertificate() const {
    return DEMO_SERVER_CERTIFICATE;
}

bool DemoCasFactory::isSupported(const std::vector<uint8_t>& pat, const std::vector<uint8_t>& pmt, const std::vector<uint8_t>& cat,
                         const CASEnvironment& env) const {
    return true;
}

std::shared_ptr<CASHelper> DemoCasFactory::createInstance(std::shared_ptr<CASSystem> system, CASPipelineController* pipeline, const std::vector<uint8_t>& pat, const std::vector<uint8_t>& pmt, const std::vector<uint8_t>& cat,
                                                  const CASEnvironment& env) {
    std::shared_ptr<DemoCasHelper> helper = std::make_shared<DemoCasHelper>(system);
    helper->setCASEnvironment(env);
    helper->setPipeline(pipeline);
    return helper;
}

std::string DEMO_CAS_OCDM_ID { "demo.cas.system" };
const std::string& DemoCasFactory::ocdmSystemId() const {
    return DEMO_CAS_OCDM_ID;
}

static DemoCasFactoryRegister factory {};

DemoCasFactoryRegister::DemoCasFactoryRegister() {
    CASHelperEngine::getInstance().registerFactory(std::make_shared<DemoCasFactory>());
}