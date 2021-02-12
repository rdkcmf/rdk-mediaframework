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

#ifndef CASSESSIONSTATUS_H
#define CASSESSIONSTATUS_H

namespace anycas {

enum CASCondition {
	CAS_CONDITION_OK = 0,
	CAS_CONDITION_INFO,
	CAS_CONDITION_BLOCKED,
	CAS_CONDITION_FATAL
};

typedef struct CASSessionStatus_ {
	enum CASCondition		cond;
	uint32_t			errorNo;
	std::string			message;
} CASSessionStatus;


}


#endif /* CASSESSIONSTATUS_H */

