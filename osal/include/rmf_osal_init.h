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

#ifndef RMF_OSAL_INIT_H_
#define RMF_OSAL_INIT_H_

#include "rmf_error.h"

#ifdef __cplusplus
extern "C"
{
#endif
rmf_Error rmf_osal_init( const char* envConfigFile, const char* debugConfigFile);
#ifdef __cplusplus
}
#endif

#endif /* RMF_OSAL_INIT_H_ */

