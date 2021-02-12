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
#include <vector>

#include "CASManagerImpl.h"
#include "CASStatus.h"
#include "rdk_debug.h"

using namespace anycas;

std::string DEMO_SERVER_CERTIFICATE { "Of course you can"};
std::string DEMO_CAS_OCDM_ID { "demo.cas.system" };

class DemoSource : public CASStatusInform, ICasPipeline, ICasSectionFilter {
public:
    DemoSource() {};
    ~DemoSource() = default;
    virtual void informStatus(const CASStatus& status) override {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Inform Status back to Source = %d\n", __FUNCTION__, __LINE__, status);
    }
    virtual uint32_t setKeySlot(uint16_t pid, std::vector<uint8_t> data) override {
        return 0;
    }
    virtual uint32_t deleteKeySlot(uint16_t pid) override {
        return 0;
    }
    virtual void unmuteAudio() override {
    }
    //SectionFilter Interface
    virtual ICasFilterStatus create(uint16_t pid, ICasFilterParam &param, ICasHandle **pHandle) override {
        return ICasFilterStatus_NoError;
    }
    virtual ICasFilterStatus start(ICasHandle* handle) override {
        return ICasFilterStatus_NoError;
    }
    virtual ICasFilterStatus setState(bool isRunning, ICasHandle* handle) override {
        return ICasFilterStatus_NoError;
    }
    virtual ICasFilterStatus destroy(ICasHandle* handle) override {
        return ICasFilterStatus_NoError;
    }
};

int main() {
    rdk_logger_init("/etc/debug.ini");
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] DemoCAS Test is started ...\n", __FUNCTION__, __LINE__);
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] We're running\n", __FUNCTION__, __LINE__);
	
	CASEnvironment env{ "tune://tuner?frequency=1700000000&modulation=16&pgmno=1", CASUsageMode::CAS_USAGE_LIVE, CASUsageManagement::CAS_USAGE_MANAGEMENT_NONE, ""};
    DemoSource demoSource;

    std::shared_ptr<CASManager> manager1 = CASManager::createInstance((ICasSectionFilter *)&demoSource, (ICasPipeline *)&demoSource);

    auto helper1 = manager1->createHelper(DEMO_CAS_OCDM_ID, env);
/*    {
            CASManagerImpl manager2;
            auto helper2 = manager2.createHelper(DEMO_CAS_OCDM_ID, env);
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Cert 2 says: %s\n", __FUNCTION__, __LINE__, helper2->serverCertificate().c_str());
    }*/
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Cert 1 says: %s\n", __FUNCTION__, __LINE__, helper1->serverCertificate().c_str());

/*    CASManagerImpl manager3;
    auto helper3 = manager3.createHelper(DEMO_CAS_OCDM_ID, env);
    {
            CASManagerImpl manager4;
            auto helper4 = manager4.createHelper(DEMO_CAS_OCDM_ID, env);
        	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Cert 4 says: %s\n", __FUNCTION__, __LINE__, helper4->serverCertificate().c_str());
    }

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Cert 3 says: %s\n", __FUNCTION__, __LINE__, helper3->serverCertificate().c_str());
*/
    helper1->registerStatusInformant((CASStatusInform *)&demoSource);

    std::vector<uint16_t> startPids = { 0x101, 0x102};
    std::vector<uint16_t> stopPids = { 0x103, 0x104};
    helper1->startStopDecrypt(startPids, stopPids);

    std::vector<uint8_t> data;
    //Add Data for EMM, Private-Data, PSI Data, Public-Data
    data.clear();
    data.resize(1, 0);          //0 - For Tune - Filter Commands
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "\n[%s:%d] ProcessData for PSI data - AV Pids: %d\n", __FUNCTION__, __LINE__, data[0]);
    manager1->processData(1, data);

    data[0] = 1;                //1 - Network-Info Data - EMM Data
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "\n[%s:%d] ProcessData for network-request - EMM data: %d\n", __FUNCTION__, __LINE__, data[0]);
    manager1->processData(1, data);

    data[0] = 2;            //2 - For private-data
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "\n[%s:%d] ProcessData for Private-Data : %d\n", __FUNCTION__, __LINE__, data[0]);
    manager1->processData(1, data);

    data[0] = 3;            //3 - For public-data
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "\n[%s:%d] ProcessData for Public-Data : %d\n", __FUNCTION__, __LINE__, data[0]);
    manager1->processData(1, data);

    helper1->setState(CASHelper::CASHelperState::PAUSED);
}
