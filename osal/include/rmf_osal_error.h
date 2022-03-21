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

#if !defined(_RMF_OSAL_ERROR_H_)
#define _RMF_OSAL_ERROR_H_

#include "rmf_osal_types.h"
#include "rmf_error.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*RMF OSAL common error definitions*/
#define RMF_OSAL_EINVAL           (RMF_ERRBASE_OSAL+1)
#define RMF_OSAL_ENOMEM           (RMF_ERRBASE_OSAL+2)
#define RMF_OSAL_EBUSY            (RMF_ERRBASE_OSAL+3)
#define RMF_OSAL_EMUTEX           (RMF_ERRBASE_OSAL+4)
#define RMF_OSAL_ECOND            (RMF_ERRBASE_OSAL+5)
#define RMF_OSAL_EEVENT           (RMF_ERRBASE_OSAL+6)
#define RMF_OSAL_ETIMEOUT         (RMF_ERRBASE_OSAL+7)
#define RMF_OSAL_ENODATA          (RMF_ERRBASE_OSAL+8)
#define RMF_OSAL_ETHREADDEATH     (RMF_ERRBASE_OSAL+9)
#define RMF_OSAL_ENOTIMPLEMENTED  (RMF_ERRBASE_OSAL+10)

#ifdef __cplusplus
}
#endif

#endif /* _RMF_OSAL_ERROR_H_ */
