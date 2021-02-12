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

#ifndef CASHELPER_H
#define CASHELPER_H


#include <vector>
#include <memory>
#include <string>
#include <cstdint>

#include "CASSystem.h"
#include "CASStatus.h"
#include "CASDataListener.h"
#include "CASHelperContext.h"
#include "CASSectionFilterParam.h"
#include "CASSessionStatus.h"

namespace anycas {


class CASHelper {
public:
	enum CASStartStatus {
		OK,    // << Everything was created, there is no need to wait for status
		WAIT,  // << Everything was created but the CAS has not started yet
		ERROR  // << Failure at some point
	};

	enum CASHelperState {
		RUNNING = 0,		// << Running
		PAUSED			// << Paused
	};

	/**
	 * Get the System that is associated with the Helper
	 * @param ocdmId the OCDM ID requested
	 * @return a pre-existing CASSystem or nullptr
	 */
	virtual std::shared_ptr<CASSystem> getSystem(const std::string& ocdmId) const { return nullptr; }

	/**
	 * Sets the System associated with the Helper for future use if desired
	 * @param ocdmId the OCDM ID used in creation
	 * @param system the system created
	 */
	virtual void setSystem(const std::string& ocdmId, std::shared_ptr<CASSystem> system) {}

	/**
	 * Get the server certificate to send to the OCDM System
	 * @return the certificate, empty string means data is not sent
	 */
	virtual const std::string& serverCertificate() const= 0;

	/**
	 * Create a CASSession for use, this is used when no media is consumed
	 */
	virtual void createSession() = 0;


	/**
	 * Control descrambling of streams
	 * @param startPids pids to start descrambling
	 * @param stopPids pid to stop
	 * @return Whether the caller needs to wait fot the CAS status to change to READY, or OK or ERROR
	 */
	virtual enum CASStartStatus startStopDecrypt(const std::vector<uint16_t>& startPids, const std::vector<uint16_t>& stopPids) = 0;

	/**
	 * Update the PSI associated with the Session
	 * @param pat a new PAT or empty if the PAT has not changed
	 * @param pmt a new PMT or empty if the PMT has not changed
	 * @param cat a new CAT or empty if the CAT has not changed
	 */
	virtual void updatePSI(const std::vector<uint8_t>& pat, const std::vector<uint8_t>& pmt, const std::vector<uint8_t>& cat) = 0;

	/**
	 * Takes the section filter command from the OCDM instance and processes the command to either create a started filter or delete one.
	 * @param context the Context used in creating the session
	 * @param jsonParams the JSON data from the OCDM instance
	 */
	virtual void processSectionFilterCommand(std::shared_ptr<CASHelperContext> context, const std::string& jsonParams) = 0;

	/**
	 * takes the section filter request from the OCDM instance and generates data required to create a Section Filter.
	 * @param context the context used when creating the session
	 * @param jsonParams The JSON data from the request
	 * @return @see CASSectionFilterParam
	 */
	virtual std::shared_ptr< CASSectionFilterParam> parseSectionFilter(std::shared_ptr<CASHelperContext> context, const std::string& jsonParams) = 0;

	/**
	 * Parse section filter data from the section filter
	 * @param pid the pid on which the data was received
	 * @param context the context provided when the filter was created
	 * @param data the data from the filter
	 * @param parsed the data to forward to the OCDM CAS session
	 * @return
	 */
	virtual bool parseSectionFilterData(uint16_t pid,  std::shared_ptr<CASHelperContext> context,  const std::vector<uint8_t> data,  std::vector<uint8_t>& parsed) = 0;

	/**
	 * Process a Request from the OCDM CAS
	 * @param context the context used when a Session was created
	 * @param destUrl the URL in the request
	 * @param dataIn the data from the request
	 * @param dataOut the data to return to the CAS
	 * @return true if the request succeeded
	 */
	virtual bool processNetworkRequest(std::shared_ptr<CASHelperContext> context, const std::string& destUrl, const std::vector<uint8_t>& dataIn, std::vector<uint8_t>& dataOut) { return false; }

	/**
	 * Process any private or public data from the OCDM instance
	 * @param context the context used when a Session was created
	 * @param isPrivate the origin of the data
	 * @param data The data from the instance
	 */
	virtual void processData(std::shared_ptr<CASHelperContext> context, bool isPrivate, const std::vector<uint8_t>& data) { }

	/**
	 * Send any data to the OCDM instance, this data is sent as a public-data payload
	 * @param data The data for the instance
	 */
	virtual void sendData(const std::vector<uint8_t>& data) { }

	/**
	 * Register a Data Listener for public data from the OCDM CAS
	 * @param listener the Listener to register
	 */
	virtual void registerDataListener(CASDataListener* listener) { }

	/**
	 * Unregister a Data Listener for public data from the OCDM CAS
	 * @param listener the Listener to remove
	 */
	virtual void unregisterDataListener(CASDataListener* listener) { }

	/**
	 * Sets if the Helper should be running or not
	 * @param state the desired state
	 */
	virtual void setState(enum CASHelperState state) = 0;

	/**
	 * Update the Helper with the status of a CAS Session as returned by the remote OCDM Instance
	 * @param context The Context for which the status is generated, used in creation of CASSession
	 * @param status The Status of the Session
	 */
	virtual void updateCasStatus(std::shared_ptr<CASHelperContext> context, const CASSessionStatus& status) = 0;

	/**
	  * Register a listener for CAS Status events
	  * @param inform
	  */
	virtual void registerStatusInformant(CASStatusInform* informant) = 0;

	/**
	  * Unregister a listener for CAS Status events
	  * @param inform
	  */
	virtual void unregisterStatusInformant(CASStatusInform* informant) = 0;

protected:
	bool parseSf(uint16_t pid, const std::vector<uint8_t> data,  std::vector<uint8_t>& parsed);
	bool serialiseSf(const std::vector<uint8_t> data,  std::vector<uint8_t>& serialised);

public:
	CASHelper() {}
	virtual ~CASHelper() {}
};

} // namespace

#endif /* CASHELPER_H */

