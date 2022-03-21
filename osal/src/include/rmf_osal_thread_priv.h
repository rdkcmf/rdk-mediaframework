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


#ifndef _RMF_OSAL_THREAD_PRIV_H_
#define _RMF_OSAL_THREAD_PRIV_H_

#ifdef USE_THREAD_POOL
#include <semaphore.h>      // get thread synchronization primitive (posix semaphore)
#endif

#include "rmf_osal_types.h"		/* Resolve basic type references. */
//#include "os_util.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef uint32_t	os_ThreadStat;		/* Define thread status type. */

#ifdef __cplusplus
}
#endif
#endif /* _RMF_OSAL_THREAD_PRIV_H_ */
