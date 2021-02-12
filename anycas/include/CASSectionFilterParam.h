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

#ifndef CASSECTIONFILTERPARAM_H
#define CASSECTIONFILTERPARAM_H

#include <vector>
#include <cstdint>
#include <string>

namespace anycas {

class CASSectionFilterParam {
public:
	typedef enum {
		SECTION_FILTER_REPEAT = 0,
		SECTION_FILTER_ONE_SHOT
	} FilterMode;

	std::string handle;
	uint16_t pid;
	std::vector<uint8_t> pos_value;
	std::vector<uint8_t> pos_mask;
	std::vector<uint8_t> neg_value;
	std::vector<uint8_t> neg_mask;
	bool downstream;
	FilterMode mode;
	CASSectionFilterParam();
	virtual ~CASSectionFilterParam() {};
};

} // namespace

#endif /* CASSECTIONFILTERPARAM_H */

