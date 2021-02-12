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

#ifndef CASSESSION_H
#define CASSESSION_H

#include <vector>
#include <memory>

#include "CASSectionFilter.h"
#include "CASSectionFilterParam.h"
#include "CASStatus.h"
#include "CASSessionStatus.h"

namespace anycas {

enum CASSessionState {
	RUNNING = 0,	// << The Session is running, default
	PAUSED		// << The Session is stopped
};

class CASSession {
public:
	/**
	 * Processes the Filter command from the OCDM CAS
	 * @param jsonParams the command
	 */

	virtual void processSectionFilterCommand(const std::string& jsonParams) = 0;

	/**
	 * Create a section filter
	 * @param param the parameters required
	 * @param context Context used to notify helper of data
	 * @return a shared pointer to the newly created @see CASSectionFilter
	 */
	virtual std::shared_ptr<CASSectionFilter> createSectionFilter(const CASSectionFilterParam & param, std::shared_ptr<CASHelperContext> context) = 0;

	/**
	 * Destroy a section filter
	 * @param filter the filter to destroy
	 */
	virtual void destroySectionFilter(std::shared_ptr<CASSectionFilter> filter) = 0;

	/**
	 * Starts a section filter
	 * @param filter the filter to start
	 */
	virtual void startSectionFilter(std::shared_ptr<CASSectionFilter> filter) = 0;

	/**
	 * Sends an Update message to the remote OCDM CAS instance when new PSI is received 
	 * @param data the data to send
	 */
	virtual void updatePSI(const std::vector<uint8_t>& data) = 0;

	/**
	 * Set the running or paused state of the session
	 * @param state the state desired see @CASSessionState
	 */
	virtual void setState(enum CASSessionState state) = 0;

	/**
	 * Send private data to the OCDM CAS instance
	 * @param isPrivate true if the data has come from a private request
	 * @param data the data to send
	 * @return true if the data was sent, false otherwise
	 */
	virtual bool sendData(bool isPrivate, const std::vector<uint8_t>& data) = 0;

	/**
	 * Process a Request from the OCDM CAS
	 * @param destUrl the URL in the request
	 * @param dataIn the data from the request
	 * @param dataOut the data to return to the CAS
	 * @return true if the request succeeded
	 */
	virtual bool processNetworkRequest(const std::string& destUrl, const std::vector<uint8_t>& dataIn, std::vector<uint8_t>& dataOut) { return false; }

	/**
	 * Process any private or public data from the OCDM instance
	 * @param isPrivate the origin of the data
	 * @param data The data from the instance
	 */
	virtual void processData(bool isPrivate, const std::vector<uint8_t>& data) { }

	/**
	 * Update the Session with the status of a Session as returned by the remote OCDM Instance
	 * @param status The Status of the Session
	 */
    virtual void updateCasStatus(const CASSessionStatus& status) = 0;

	/**
	 * Close the session. This is called during the destructor
	 */
	virtual void close() = 0;

public:
	CASSession() {};
	virtual ~CASSession() {};
};

} // namespace

#endif /* CASSESSION_H */

