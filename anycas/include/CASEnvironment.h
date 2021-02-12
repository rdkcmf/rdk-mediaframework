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

#ifndef CASENVIRONMENT_H
#define CASENVIRONMENT_H

#include <string>

namespace anycas {

enum CASUsageMode {
	CAS_USAGE_NULL= 0,   // << The tuner is not used for presentation
	CAS_USAGE_LIVE,      // << The tuner is used for live media
	CAS_USAGE_RECORDING, // << The tuner is used for a recording
	CAS_USAGE_PLAYBACK   // << The pipeline is playing media from a recording
};

enum CASUsageManagement {
	CAS_USAGE_MANAGEMENT_NONE = 0, // << The system should also establish a management session, if req'd
	CAS_USAGE_MANAGEMENT_FULL,     // << The system should also establish a management session, if req'd
	CAS_USAGE_MANAGEMENT_NO_PSI,   // << The system should establish a management session, no PSI is expected
	CAS_USAGE_MANAGEMENT_NO_TUNE   // << The system is for management, but no tuner will be associated
};

class CASEnvironment {
public:
	/**
	 * Set the URI that is playing
	 * @param uri the URI
	 */
	void setPlayUri(const std::string& uri) { uri_ = uri; }

	/**
	 * Get the URI that is playing
	 * @return the uri
	 */
	const std::string& getPlayUri() const { return uri_; }

	/**
	 * Set the initData provided by the app, etc
	 * @param initData the data
	 */
	void setInitData(const std::string& initData) { initData_ = initData; }

	/**
	 * Get the CAS Initialisation data provided at tune
	 * @return the  data
	 */
	const std::string& getInitData() const { return initData_; }

	/**
	 * Set the usage of the environment @see CASUsageMode
	 * @param usage the reason the session is being created
	 */
	void setUsage(CASUsageMode usage) { usage_ = usage; }

	/**
	 * Get the reason for the session
	 * @return the reason
	 */
	CASUsageMode getUsageMode() const { return usage_; }

	/**
	 * Set the usage of the environment @see CASUsageManagement
	 * @param management the management requirements of the session
	 */
	void setUsageManagement(CASUsageManagement management) { management_ = management; }

	/**
	 * Get the management required for the session
	 * @return the reason
	 */
	CASUsageManagement getUsageManagement() const { return management_; }

	CASEnvironment(const std::string& uri, CASUsageMode usage, CASUsageManagement management, const std::string& initData ) :
		uri_(uri), usage_(usage), management_(management), initData_(initData) {}
	~CASEnvironment() {}
private:
	std::string uri_;
	CASUsageMode usage_;
	CASUsageManagement management_;
	std::string initData_;
};

} // namespace

#endif /* CASENVIRONMENT_H */

