/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2014 RDK Management
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
*/

#include <rmf_error.h>
#include <rmf_osal_sync.h>

#define RMF_PLATFORM_ALREADY_INITIALIZED    (RMF_ERRBASE_PLATFORM + 1)
#define RMF_PLATFORM_NOT_INITIALIZED    (RMF_ERRBASE_PLATFORM + 2)

class rmfPlatformPrivate;
class rmfPlatform
{
public:
	rmf_Error init(int argc, char * argv[]);
	rmf_Error uninit();
	bool isInitialized();
	static rmfPlatform* getInstance();
private:
	rmfPlatform();
	~rmfPlatform();
	static bool rmf_framework_initialized;
	static rmfPlatform *m_rmfPlatform;
	rmfPlatformPrivate *m_rmfPlatformPrivate;
};

