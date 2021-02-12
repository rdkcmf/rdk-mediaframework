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

#include "CASHelper.h"
#include "CASHelperFactory.h"
#include "glib.h"
#include "rdk_debug.h"
#include <mutex>
#include "CASEnvironment.h"
#include "json/json.h"
#include "CASSectionFilterParam.h"
#include "CASSystemImpl.h"
#include <algorithm>

#define      TDT_TABLE_ID                       0x70
#define      TOT_TABLE_ID                       0x73
#define      TDT_TOT_PID                        0x0014   // TOD/TOT PID

using namespace anycas;

class DemoCasHelper : public CASHelper {
    std::shared_ptr<CASSystem> casSystem_;
    std::vector<std::shared_ptr<CASSession>> casSessions_;
    std::map<std::shared_ptr<CASSectionFilter>, std::shared_ptr<CASHelperContext>> contextFilterMap_;
    std::map<uint16_t, std::pair<std::shared_ptr<CASSession>,std::shared_ptr<CASHelperContext>>>pidSessionMap_;

public:

    DemoCasHelper(std::shared_ptr<CASSystem> system): casSystem_(system) {
    }

    ~DemoCasHelper() {
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.ANYCAS", "[%s:%d] DemoCasHelper destructor\n", __FUNCTION__, __LINE__);
        contextFilterMap_.clear();
	pidSessionMap_.clear();
	casSessions_.clear();
	auto system = std::dynamic_pointer_cast<CASSystemImpl>(casSystem_);
	system->close();
    }
    virtual const std::string& serverCertificate() const override;

    virtual enum CASStartStatus startStopDecrypt(const std::vector<uint16_t>& startPids, const std::vector<uint16_t>& stopPids) override {
        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );
	for (uint16_t pid_stop : stopPids)
        {
            RDK_LOG( RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] stopPid - 0x%x\n", __FUNCTION__, __LINE__, pid_stop);
            std::shared_ptr<CASSession> filterSession;
            std::shared_ptr<CASSectionFilter> filter;

            auto stopPid = pidSessionMap_.find(pid_stop);
            if(stopPid != pidSessionMap_.end())
            {
               RDK_LOG( RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Session Matches for stopPid - 0x%x\n", __FUNCTION__, __LINE__, pid_stop);
               filterSession = stopPid->second.first;
               filterSession->close();
               for (auto &j : contextFilterMap_) {
                  if (j.second == stopPid->second.second) {
                  filter = j.first;
                  RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Destroyed the CASFilter = 0x%x, context = 0x%x", __FUNCTION__, __LINE__, filter,stopPid->second.second);
                  filterSession->destroySectionFilter(filter);
                  contextFilterMap_.erase(filter);
                  break;
                  }
               }
	       uint16_t pos = 0;

	       for (auto &k: casSessions_)
	       {
		   if(k == filterSession)
		   {
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d]Destroyed the casSessions_ = 0x%x\n", __FUNCTION__, __LINE__, k);
			casSessions_.erase(casSessions_.begin()+pos);
			break;
		   }
		   pos++;
	       }
               pidSessionMap_.erase(stopPid);
            }
        }
	for (uint16_t pid : startPids)
        {
            std::shared_ptr<CASHelperContext> context = std::make_shared<CASHelperContext>();
            RDK_LOG( RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Session creates for Pid - 0x%x\n", __FUNCTION__, __LINE__, pid);
            std::string initDataType;
            std::vector<uint8_t> initData;
            auto session = casSystem_->createSession(context, initDataType, initData);
	    if(session)
	    {
                casSessions_.push_back(session);
                pidSessionMap_.emplace(pid, std::make_pair(session,context));
                std::vector<uint8_t> psiData;
                psiData.push_back((uint8_t)0);  //PSI Data Simulation
                session->updatePSI(psiData);
	    }
	    else
	    {
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Failed to create the session\n", __FUNCTION__, __LINE__ );
	    }
        }

        return CASStartStatus::OK;
    }
    virtual void updatePSI(const std::vector<uint8_t>& pat, const std::vector<uint8_t>& pmt, const std::vector<uint8_t>& cat) override {
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d]Enter updatePSI\n", __FUNCTION__, __LINE__);
        std::vector<uint8_t> psiData;
        psiData.push_back((uint8_t)0);
        for(auto casSession : casSessions_)
	{
            casSession->updatePSI(psiData);
        }
    }

    virtual void processSectionFilterCommand(std::shared_ptr<CASHelperContext> context, const std::string& jsonParams) override {
        CASSectionFilterParam params;
        std::shared_ptr<CASSectionFilter> filter;
	std::shared_ptr<CASSession> casSession;
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] processSectionFilterCommand called in Helper\n", __FUNCTION__, __LINE__);
	for(auto &it : pidSessionMap_){
                if(it.second.second == context)
                {
                        casSession = it.second.first;
                        break;
                }
        }
        if(!casSession)
        {
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.ANYCAS", "[%s:%d] casSession Not found \n", __FUNCTION__, __LINE__);
                return;
        }

//            if(context == casSession->helperContext()){
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.ANYCAS", "[%s:%d] casSession = 0x%x\n", __FUNCTION__, __LINE__, casSession);
                Json::Value root;
                Json::Reader reader;

                if (!reader.parse( jsonParams, root ) ) {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Parsing request json failed\n", __FUNCTION__, __LINE__);
                    return;
                }

				if(root["method"] == "filter-commands") {
					RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] process sectionfilter command\n", __FUNCTION__, __LINE__);

					const Json::Value command = root["commands"];
					RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] command size = %d\n", __FUNCTION__, __LINE__, command.size());
					for ( int index = 0; index < command.size(); ++index )
					{
						RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] command = %s\n", __FUNCTION__, __LINE__, command[index].toStyledString().c_str());
//						casSession->processSectionFilterCommand(command[index].toStyledString());
                        Json::Value cmd_root;
                        Json::Reader cmd_reader;
                        if (!reader.parse( command[index].toStyledString(), cmd_root ) ) {
                            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Parsing request json failed\n", __FUNCTION__, __LINE__);
                            continue;
                        }
                        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.ANYCAS", "[%s:%d] Received command in helper = %s\n", __FUNCTION__, __LINE__, cmd_root["command"].asString().c_str());
                        if (cmd_root["command"] == "create")
                        {
                            //Simulate the filter params
                            params.pos_mask.push_back(0xFC);
                            params.pos_value.push_back(TDT_TABLE_ID | TOT_TABLE_ID);

                            params.downstream = false;
                            params.mode = CASSectionFilterParam::FilterMode::SECTION_FILTER_REPEAT;
                            params.pid = TDT_TOT_PID;

                            auto casFilter = casSession->createSectionFilter(params, context);
                            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Received the CASFilter = 0x%x\n", __FUNCTION__, __LINE__, casFilter);
			    if(casFilter)
			    {
                                contextFilterMap_.insert(std::make_pair(casFilter, context));
			    }
                        }
                        else if (cmd_root["command"] == "start")
                        {
                            for (auto &i : contextFilterMap_) {
                                if (i.second == context) {
                                    filter = i.first;
                                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Started the CASFilter = 0x%x\n", __FUNCTION__, __LINE__, filter);
                                    casSession->startSectionFilter(filter);
                                    break;
                                }
                            }
                        }
                        else if (cmd_root["command"] == "destroy")
                        {
                            for (auto &i : contextFilterMap_) {
                                if (i.second == context) {
                                    filter = i.first;
                                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Destroyed the CASFilter = 0x%x\n", __FUNCTION__, __LINE__, filter);
                                    casSession->destroySectionFilter(filter);
                                    contextFilterMap_.erase(filter);
                                    break;
                                }
                            }
                        }
                        else
                        {
                            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Invalid filter command\n", __FUNCTION__, __LINE__);
                        }
					}
				}

//            }
    }

    virtual std::shared_ptr< CASSectionFilterParam> parseSectionFilter(std::shared_ptr<CASHelperContext> context, const std::string& jsonParams) override {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] parseSectionFilter in Helper\n", __FUNCTION__, __LINE__);
        return std::make_shared<CASSectionFilterParam>();
    }

    /**
     * Parse section filter data from the section filter
     * @param pid the pid on which the data was received
     * @param context the context provided when the filter was created
     * @param data the data from the filter
     * @param parsed the data to forward to the OCDM CAS session
     * @return
     */
    virtual bool parseSectionFilterData(uint16_t pid,  std::shared_ptr<CASHelperContext> context,  const std::vector<uint8_t> data,  std::vector<uint8_t>& parsed) override {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] parseSectionFilterData in Helper\n", __FUNCTION__, __LINE__);
        if(data.size() > 0) {
            parsed.push_back(data[0]);
        }
        //Get the Data and check for the simulated Data in parsed.
        return true;
    }

    /**
     * Process a Request from the OCDM CAS
     * @param context the context provided when the session was created
     * @param destUrl the URL in the request
     * @param dataIn the data from the request
     * @param dataOut the data to return to the CAS
     * @return true if the request succeeded
     */
    virtual bool processNetworkRequest(std::shared_ptr<CASHelperContext> context, const std::string& destUrl, const std::vector<uint8_t>& dataIn, std::vector<uint8_t>& dataOut) override {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] processNetworkRequest in Helper\n", __FUNCTION__, __LINE__);

        std::string data_str;
        data_str.assign(dataIn.begin(), dataIn.end());

        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] destURL = %s, dataIn = %s\n", __FUNCTION__, __LINE__, destUrl.c_str(), data_str.c_str());

        std::string str("NetworkResponse");
        dataOut.assign(str.begin(), str.end());

        return true;
    }

    /**
     * Process any private data from the OCDM instance
     * @param context the context provided when the session was created
     * @param isPrivate If the data is from a private-data command
     * @param data The data from the instance
     */
    virtual void processData(std::shared_ptr<CASHelperContext> context, bool isPrivate, const std::vector<uint8_t>& data) override {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] processData in Helper\n", __FUNCTION__, __LINE__);

        std::string data_str;
        data_str.assign(data.begin(), data.end());
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] isPrivate = %d, dataIn = %s\n", __FUNCTION__, __LINE__, isPrivate, data_str.c_str());
        if(!isPrivate) {
            std::lock_guard<std::mutex> guard(listener_mutex_);
            for (CASDataListener* listener: listenerList_) {
                listener->casPublicData(data);
            }
        }
        else {
            uint32_t retCode;
            std::vector<uint8_t> keyHandle;
            std::string cmd = "write-keys";
            char *pvtData = (char *) malloc(32);
            if(pvtData)
            {
                sprintf(pvtData, "write keys data");
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] pvtData = 0x%x\n", __FUNCTION__, __LINE__, pvtData);
                uint16_t pid = 0x1001;
                uint32_t addr = (uint32_t)pvtData;
                keyHandle.push_back((addr & 0xFF000000) >> 24);
                keyHandle.push_back((addr & 0xFF0000) >> 16);
                keyHandle.push_back((addr & 0xFF00) >> 8);
                keyHandle.push_back((addr & 0xFF));
                pipeline_->pipelineDetails(pid, keyHandle, cmd, retCode);
                free(pvtData);
            }
	    std::string deleteCmd = "delete-keys";
	    uint16_t pid = 0x1001;
	    std::vector<uint8_t> dummyKeyHandle;
            dummyKeyHandle.push_back((uint8_t)0);
	    pipeline_->pipelineDetails(pid, dummyKeyHandle, deleteCmd, retCode);
        }
    }

    virtual void sendData(const std::vector<uint8_t>& data) override {
        for(auto casSession : casSessions_){
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] casSession = 0x%x\n", __FUNCTION__, __LINE__, casSession);
            casSession->sendData(false, data);
            break;
        }
    }

	virtual void registerDataListener(CASDataListener* listener) override {
        std::lock_guard<std::mutex> guard(listener_mutex_);
        listenerList_.push_back(listener);
    }

	virtual void unregisterDataListener(CASDataListener* listener) override {
        std::lock_guard<std::mutex> guard(listener_mutex_);
	auto it = std::find(listenerList_.begin(), listenerList_.end(), listener);
        if(it != listenerList_.end())
        {
            RDK_LOG( RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d]Erase listener\n", __FUNCTION__, __LINE__ );
            listenerList_.erase(it);
        }
    }

    virtual void setState(enum CASHelperState state) override {
        if(state == CASHelperState::PAUSED) {
            for(auto casSession : casSessions_){
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] casSession = 0x%x\n", __FUNCTION__, __LINE__, casSession);
                casSession->setState(CASSessionState::PAUSED);
            }
        }
    }

    virtual void updateCasStatus(std::shared_ptr<CASHelperContext> session, const CASSessionStatus& status) override {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] updateCasStatus in Helper\n", __FUNCTION__, __LINE__);
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] status = %d, errorNo = %d, msg = %s\n", __FUNCTION__, __LINE__, status.cond, status.errorNo, status.message.c_str());

        std::lock_guard<std::mutex> guard(informant_mutex_);
        for (CASStatusInform* informant: informantList_) {
            informant->informStatus(CASStatusInform::CASStatus_OK);
        }
    }

    virtual void registerStatusInformant(CASStatusInform* informant) override {
        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );
        std::lock_guard<std::mutex> guard(informant_mutex_);
        informantList_.push_back(informant);
    }

    virtual void unregisterStatusInformant(CASStatusInform* informant) override {
        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );
        std::lock_guard<std::mutex> guard(informant_mutex_);
	auto it = std::find(informantList_.begin(), informantList_.end(), informant);
        if(it != informantList_.end())
        {
	    RDK_LOG( RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d]Erase informant\n", __FUNCTION__, __LINE__ );
            informantList_.erase(it);
        }
    }

    virtual void createSession() override {
        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );
        std::shared_ptr<CASHelperContext> context = std::make_shared<CASHelperContext>();

        std::string initDataType = "cenc";  // Mocked data
        std::vector<uint8_t> initData;
        std::string strData = env_->getInitData();
        initData.assign(strData.begin(), strData.end());
        auto session = casSystem_->createSession(context, initDataType, initData);
        if(session) {
            casSessions_.push_back(session);
        }
        else{
            RDK_LOG( RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Failed to create the session\n", __FUNCTION__, __LINE__ );
        }
    }

    void setCASEnvironment(const CASEnvironment& env) {
        env_ = std::make_shared<CASEnvironment>(env.getPlayUri(), env.getUsageMode(), env.getUsageManagement(), env.getInitData());
    }

    void setPipeline(CASPipelineController* pipeline) {
        pipeline_ = pipeline;
    }

private:
    std::vector<CASStatusInform*> informantList_;
    std::mutex informant_mutex_;
    std::vector<CASDataListener*> listenerList_;
    std::mutex listener_mutex_;
    std::shared_ptr<CASEnvironment> env_;
    CASPipelineController* pipeline_;
};

class DemoCasFactory : public CASHelperFactory {
public:
    virtual bool isSupported(const std::vector<uint8_t>& pat, const std::vector<uint8_t>& pmt, const std::vector<uint8_t>& cat,
                             const CASEnvironment& env) const override;

    virtual std::shared_ptr<CASHelper> createInstance(std::shared_ptr<CASSystem> system, CASPipelineController* pipeline, const std::vector<uint8_t>& pat, const std::vector<uint8_t>& pmt, const std::vector<uint8_t>& cat,
                                                      const CASEnvironment& env) override;
    virtual const std::string& ocdmSystemId() const override;

};

class DemoCasFactoryRegister {
public:
    DemoCasFactoryRegister();
};
