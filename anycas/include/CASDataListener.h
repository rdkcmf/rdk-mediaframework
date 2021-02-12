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

#ifndef CASDATALISTENER_H
#define CASDATALISTENER_H

#include <cstdint>
#include <vector>

namespace anycas {

class CASDataListener {
public:
	CASDataListener() {};
	virtual ~CASDataListener() {};

	virtual void casPublicData(const std::vector<uint8_t>& data) = 0;
};

} // namespace

#endif /* CASDATALISTENER_H */

