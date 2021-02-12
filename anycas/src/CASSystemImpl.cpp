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

#include <future>
#include <string>
#include <algorithm>

#include "CASSystemImpl.h"
#include "CASManagerImpl.h"
#include "CASSessionImpl.h"
#include "rdk_debug.h"
#include "json/json.h"

namespace anycas {

static void processNetworkRequestThread(CASSessionImpl *session, const std::string destUrl, std::string data)
{
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] New Thread : casSession = 0x%x, destUrl = %s, data = %s\n", __FUNCTION__, __LINE__, session, destUrl, data.c_str());
	std::vector<uint8_t> dataIn;
	std::vector<uint8_t> dataOut;
	dataIn.assign(data.begin(), data.end());
	session->processNetworkRequest(destUrl, dataIn, dataOut);
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Thread Exited\n", __FUNCTION__, __LINE__);
}

CASSystemImpl::UnderlyingSystem::UnderlyingSystem(struct OpenCDMSystem*  openCdmSystem)
: openCdmSystem_(openCdmSystem)
{
}

CASSystemImpl::UnderlyingSystem::~UnderlyingSystem() {
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Destroying expired OCDM System: %p\n", __FUNCTION__, __LINE__, openCdmSystem_);
	opencdm_destruct_system(openCdmSystem_);
}

CASSystemImpl::CASSystemImpl(std::shared_ptr<CASSystemImpl::UnderlyingSystem> underlyingSystem)
: underlyingSystem_(underlyingSystem)
{
}


CASSystemImpl::~CASSystemImpl() {
	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.ANYCAS", "[%s:%d] CASSystemImpl destructor, system = 0x%x\n", __FUNCTION__, __LINE__, this);
	for(auto& nwThread: nwReqThreads_) {
			nwThread.join();
		}
	nwReqThreads_.clear();
}

void CASSystemImpl::close()
{
	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.ANYCAS", "[%s:%d] CASSystemImpl close\n", __FUNCTION__, __LINE__);
        casManager_.reset();
}

std::shared_ptr<CASSession> CASSystemImpl::createSession(std::shared_ptr<CASHelperContext> context, 
                      const std::string& initDataType, const std::vector<uint8_t>& initData) {
	struct OpenCDMSession *pOpenCDMSession = NULL;

	memset(&openCDMSessionCallbacks_, 0, sizeof(openCDMSessionCallbacks_));

	openCDMSessionCallbacks_.process_challenge_callback = [](OpenCDMSession* session, void* userData, const char destUrl[], const uint8_t challenge[], const uint16_t challengeSize) {

		CASSystemImpl* userSession = reinterpret_cast<CASSystemImpl*>(userData);
		userSession->OnKeyMessage(session, destUrl, challenge, challengeSize);
	};

	openCDMSessionCallbacks_.key_update_callback = [](OpenCDMSession* session, void* userData, const uint8_t key[], const uint8_t keySize) {
		CASSystemImpl* userSession = reinterpret_cast<CASSystemImpl*>(userData);
		userSession->OnKeyStatusUpdate(session, key, keySize);
	};

	openCDMSessionCallbacks_.error_message_callback = [](OpenCDMSession* session, void* userData, const char message[]) {
		CASSystemImpl* userSession = reinterpret_cast<CASSystemImpl*>(userData);
		userSession->OnErrorMessage(session, message);
	};

	openCDMSessionCallbacks_.keys_updated_callback = [](const OpenCDMSession* session, void* userData) {
		CASSystemImpl* userSession = reinterpret_cast<CASSystemImpl*>(userData);
		userSession->OnKeyStatusesUpdated(session);
	};

	std::lock_guard<std::mutex> guard(sessions_mutex_);
	OpenCDMError ocdmRet = opencdm_construct_session(underlyingSystem_->openCdmSystem_, LicenseType::Temporary, initDataType.c_str(),
				  initData.data(), initData.size(),
				  nullptr, 0, //No Custom Data
				  &openCDMSessionCallbacks_,
				  static_cast<void*>(this),
				  &pOpenCDMSession);
	if (ocdmRet != ERROR_NONE)
	{
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Error constructing OCDM session. OCDM err=0x%x\n", __FUNCTION__, __LINE__, ocdmRet);
		return nullptr;
	}
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Created the opencdm session = 0x%x\n", __FUNCTION__, __LINE__, pOpenCDMSession);

	auto helper = helper_.lock();
	if(!helper)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.ANYCAS", "[%s:%d] helper is NULL\n", __FUNCTION__, __LINE__);
		return nullptr;
	}

	auto ptr = std::make_shared<CASSessionImpl>(pOpenCDMSession, context, helper, std::dynamic_pointer_cast<CASManagerImpl>(casManager_));
	casSessions_.push_back(std::weak_ptr<CASSessionImpl>(ptr));
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Created the CASSessionImpl = 0x%x\n", __FUNCTION__, __LINE__, ptr);

	return ptr;
}

void CASSystemImpl::OnKeyMessage(OpenCDMSession* session, const char destUrl[], const uint8_t challenge[], const uint16_t challengeSize) {
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] OnKeyMessage - received for ocdmsession = 0x%x\n", __FUNCTION__, __LINE__, session);
	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.ANYCAS", "[%s:%d] destURL = %s, Challenge = %s, Size = %d\n", __FUNCTION__, __LINE__, destUrl, (const char*)challenge, challengeSize);

	if(!challengeSize) {
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.ANYCAS", "[%s:%d] Invalid Challenge Data, Size is 0\n", __FUNCTION__, __LINE__);
		return;
	}

	Json::Value root;
	Json::Reader reader;

	auto casSession = getCASSession(session);
	if (casSession) {
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] CASSession found...\n", __FUNCTION__, __LINE__);

		std::string challenge_str = {reinterpret_cast<const char*>(challenge),
								challengeSize};
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Challenge String = %s\n", __FUNCTION__, __LINE__, challenge_str.c_str());

		size_t type_pos = challenge_str.find(":Type:");
		if(type_pos	!= std::string::npos) {
			std::string request_type = {challenge_str.c_str(), type_pos};
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] request_type = %s\n", __FUNCTION__, __LINE__, request_type.c_str());
			if(!request_type.empty() && (request_type == "3" || request_type =="individualization-request")) {
				size_t req_offset = type_pos + 6;
				std::string challenge_req = {challenge_str.c_str() + req_offset, challenge_str.size() - req_offset};

				if (!reader.parse( challenge_req, root ) ) {
					RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Parsing request json failed\n", __FUNCTION__, __LINE__);
					return;
				}

				RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Method  = %s\n", __FUNCTION__, __LINE__, root["method"].asString().c_str());

				if(root["method"] == "network-request") {
					RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Request for network-request\n", __FUNCTION__, __LINE__);
					std::string url(destUrl);
					std::thread nwReqThreadID(processNetworkRequestThread, casSession.get(), url, root["data"].asString());
					nwReqThreads_.push_back(std::move(nwReqThreadID));
				}
				else if(root["method"] == "private-data") {
					RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Request for private-data\n", __FUNCTION__, __LINE__);

					std::string data_str = root["data"].asString();
					std::vector<uint8_t> data;
					data.assign(data_str.begin(), data_str.end());

					casSession->processData(true, data);
				}
				else if(root["method"] == "public-data") {
					RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Request for public-data\n", __FUNCTION__, __LINE__);

					std::string data_str = root["data"].asString();
					std::vector<uint8_t> data;
					data.assign(data_str.begin(), data_str.end());

					casSession->processData(false, data);
				}
				else if(root["method"] == "filter-commands") {
					RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] process sectionfilter command\n", __FUNCTION__, __LINE__);

					casSession->processSectionFilterCommand(challenge_req);
				}
				else
					RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Unsupported Method = %s\n", __FUNCTION__, __LINE__, root["method"].asString().c_str());
			}
			else {
				RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Invalid Type Request = %s\n", __FUNCTION__, __LINE__, request_type.c_str());
			}
		}
		else {
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Invalid Challenge = %s\n", __FUNCTION__, __LINE__, challenge_str.c_str());
		}
	}

	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] OnKeyMessage - End\n\n", __FUNCTION__, __LINE__);
}
void CASSystemImpl::OnErrorMessage(OpenCDMSession* session, const char message[]) {
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] OnErrorMessage - received for ocdmsession = 0x%x\n", __FUNCTION__, __LINE__, session);
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Status Message = %s\n", __FUNCTION__, __LINE__, message);

	auto casSession = getCASSession(session);
	if (casSession) {
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] CASSession found...\n", __FUNCTION__, __LINE__);

		Json::Value root;
		Json::Reader reader;
		if (!reader.parse( message, root ) ) {
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Parsing request json failed\n", __FUNCTION__, __LINE__);
			return;
		}

		CASSessionStatus casStatus;
		if(root["status"] == "OK") {
			casStatus.cond = CAS_CONDITION_OK;
		}
		else if(root["status"] == "INFO") {
			casStatus.cond = CAS_CONDITION_INFO;
		}
		else if(root["status"] == "BLOCKED") {
			casStatus.cond = CAS_CONDITION_BLOCKED;
		}
		else if(root["status"] == "FATAL") {
			casStatus.cond = CAS_CONDITION_FATAL;
		}
		else{
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Invalid Status\n", __FUNCTION__, __LINE__);
			return;
		}
		casStatus.errorNo = root["errorNo"].asUInt();
		casStatus.message = root["message"].asString();
		casSession->updateCasStatus(casStatus);
	}
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] OnErrorMessage - End\n\n", __FUNCTION__, __LINE__);
}

std::shared_ptr<CASSessionImpl> CASSystemImpl::getCASSession(OpenCDMSession* ocdmSession) {
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] getCASSession for ocdmsession\n", __FUNCTION__, __LINE__);
	std::lock_guard<std::mutex> guard(sessions_mutex_);
	int pos = 0;
	std::shared_ptr<CASSessionImpl> casSession;

	for (auto it = casSessions_.begin();it != casSessions_.end();)
	{
                casSession = (*it).lock();
                if(!casSession)
                {
                        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.ANYCAS", "[%s:%d] casSession is NULL\n", __FUNCTION__, __LINE__);
                        casSessions_.erase(it);
                        continue;
                }
                else
                {
                        ++it;
                }
		if (ocdmSession == casSession->ocdmSession()) {
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] Matched casSession = 0x%x\n", __FUNCTION__, __LINE__, casSession);
			return casSession;
		}
	}
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.ANYCAS", "[%s:%d] No matched CAS session\n\n", __FUNCTION__, __LINE__);

	return nullptr;
}

void CASSystemImpl::OnKeyStatusesUpdated(const OpenCDMSession* session) {
}
void CASSystemImpl::OnKeyStatusUpdate(OpenCDMSession* session, const uint8_t key[], const uint8_t keySize) {
}

} // namespace
