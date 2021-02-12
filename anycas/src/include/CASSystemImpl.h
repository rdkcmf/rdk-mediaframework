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

#ifndef CASSYSTEMIMPL_H
#define CASSYSTEMIMPL_H

#include "open_cdm.h"

#include "CASSystem.h"
#include "CASSessionImpl.h"
#include "CASManager.h"
#include <thread>
#include <mutex>

namespace anycas {


class CASSystemImpl : public CASSystem {
public:
    virtual std::shared_ptr<CASSession> createSession(std::shared_ptr<CASHelperContext> context,
                                                      const std::string& initDataType, const std::vector<uint8_t>& initData);
public:
	void OnKeyMessage(OpenCDMSession* session, const char destUrl[], const uint8_t challenge[], const uint16_t challengeSize);
	void OnErrorMessage(OpenCDMSession* session, const char message[]);
	void OnKeyStatusesUpdated(const OpenCDMSession* session);
	void OnKeyStatusUpdate(OpenCDMSession* session, const uint8_t key[], const uint8_t keySize);

	void setHelper(std::shared_ptr<CASHelper> helper) { helper_ = std::weak_ptr<CASHelper>(helper); }
	void setManager(std::shared_ptr<CASManager>manager) { casManager_ = manager; }
	void close();

public:
	class UnderlyingSystem {
	public:
		UnderlyingSystem(struct OpenCDMSystem* openCdmSystem);
		virtual ~UnderlyingSystem();
		struct OpenCDMSystem* openCdmSystem_;
	};

	CASSystemImpl(std::shared_ptr<UnderlyingSystem> underlyingSystem);
	virtual ~CASSystemImpl();

	std::vector<std::weak_ptr<CASSessionImpl>> casSessions_;
	std::shared_ptr<CASSessionImpl> getCASSession(OpenCDMSession* ocdmSession);

    std::shared_ptr<UnderlyingSystem> underlyingSystem_;

	OpenCDMSessionCallbacks openCDMSessionCallbacks_;
	std::mutex sessions_mutex_;
	std::weak_ptr<CASHelper> helper_;
	std::shared_ptr<CASManager>casManager_;
	std::vector<std::thread> nwReqThreads_;
};

} // namespace

#endif /* CASSYSTEMIMPL_H */

