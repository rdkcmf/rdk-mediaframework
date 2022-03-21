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


#if !defined(_OS_UTIL_COMMON_H)
#define _OS_UTIL_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "rmf_osal_types.h"	/* Resolve basic type references. */
#include "rmf_osal_util.h"	/* Resolve basic type references. */

#define OS_NAME "Linux"
#define RMF_OSAL_VERSION "0.01"


/**
 * <i>removeFiles</i>
 *
 * Recursively remove files and directories from the given directory path.
 *
 * @param dirPath Directory to be processed.
 *
 * @return RMF_SUCCESS on success, else RMF_OSAL error codes.
 */
rmf_Error removeFiles(char *dirPath);

#ifdef __cplusplus
}
#endif

#endif /* _OS_UTIL_COMMON_H */

