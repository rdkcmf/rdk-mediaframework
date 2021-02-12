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

#ifndef CASSYSTEM_H
#define CASSYSTEM_H

#include <string>
#include <vector>
#include <memory>

#include "CASSession.h"
#include "CASHelperContext.h"

namespace anycas {

class CASSystem  {
public:
	/**
	 * Create a CASSession for use
	 * @param context the context that will be used to identify the session
	 * @param initDataType the type of initData to send, e.g. "cenc"
	 * @param initData the initData to send to the OCDM CAS instance created
	 * @return a shared pointer to the newly create Session, or nullptr
	 */
	virtual std::shared_ptr<CASSession> createSession(std::shared_ptr<CASHelperContext> context,
		const std::string& initDataType, const std::vector<uint8_t>& initData) = 0;

	CASSystem() {};
	virtual ~CASSystem() {};

private:

};

} // namespace

#endif /* CASSYSTEM_H */

