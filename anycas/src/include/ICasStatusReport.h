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

#ifndef ICASSTATUSREPORT_H
#define ICASSTATUSREPORT_H

namespace anycas {

typedef enum ICasStatus {
	ICasStatus_NoError = 0,			// << No error, video is unblocked
	ICasStatus_UserError = 0x4000,	// << User defined non-fatal errors, blocking
	ICasStatus_UserFatal = 0x8000	// << User defined fatal errors, terminal
};

class ICasStatusReport {
public:
	virtual ~ICasStatusReport() {};
protected:
	ICasStatusReport() {};

	/**
	 * Report an event to the framework
	 * @param casStatus the status
	 * @param message a user message that can be forwarded
	 */
	virtual void reportCasStatus(ICasStatus casStatus, const std::string& message) = 0;
};

}  // namespace

#endif /* ICASSTATUSREPORT_H */

