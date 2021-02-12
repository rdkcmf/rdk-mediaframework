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

#ifndef ICASPIPELINE_H
#define ICASPIPELINE_H

#include <vector>

namespace anycas {

class ICasPipeline {
public:
	virtual ~ICasPipeline() {};
protected:
	ICasPipeline() {};
public:
	/**
	 * Sends/Gets data to/from the pipeline to which the manager instance is attached
	 * @param pid The pid to which the opaque data applies
	 * @param data the data. This is an in/out parameter depending on the SoC architecture
	 * @param returnCode an implementation specific return value should the operation fail
	 * @return true if the operation succeeded, false if it failed.
	 */
	virtual uint32_t setKeySlot(uint16_t pid, std::vector<uint8_t> data) = 0;
	virtual uint32_t deleteKeySlot(uint16_t pid) = 0;
	virtual void unmuteAudio() = 0;
};

} // namespace

#endif /* ICASPIPELINE_H */

