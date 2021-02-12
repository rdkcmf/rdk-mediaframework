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

#ifndef CASHELPERENGINE_H
#define CASHELPERENGINE_H

#include <memory>
#include "CASHelperFactory.h"

namespace anycas {

class CASHelperEngine {
public:
	/**
	 * Get the instance of the Engine
	 * @return the Engine
	 */
	static CASHelperEngine& getInstance();

	/**
	 * Register with the Engine
	 * @param factory a Factory to register
	 */
	virtual void registerFactory(std::shared_ptr<CASHelperFactory> factory) = 0;

protected:
	CASHelperEngine() {}
	virtual ~CASHelperEngine() {}
private:
	CASHelperEngine(const CASHelperEngine& orig) =delete;

};

} // namespace

#endif /* CASHELPERENGINE_H */
