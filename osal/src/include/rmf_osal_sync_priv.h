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

#if !defined(_RMF_OSAL_SYNC_PRIV_H_)
#define _RMF_OSAL_SYNC_PRIV_H_

#ifndef __ASSEMBLY__
#include <rmf_osal_types.h>
#include <rmf_osal_thread_priv.h>


#ifdef __cplusplus
extern "C" {
#endif


/***
 * Mutex and Condition type definitions:
 */

#define OS_COND_ID (0x63636363)

/* Define condition implementation type. */
struct os_Cond_s
{
    uint32_t        cd_id;
    rmf_osal_Bool	    cd_autoReset;
    rmf_osal_Bool        cd_state;
    os_ThreadId     cd_owner;
    os_Mutex        cd_mutex;
    pthread_cond_t  cd_cond;
    uint32_t  cd_count; /*number of threads waiting on the condition*/
} ;

typedef struct os_Cond_s *	os_Condition;
typedef struct os_Cond_s	os_Condition_s;



#ifdef __cplusplus
}
#endif
#endif /* _RMF_OSAL_SYNC_PRIV_H_ */


#endif /*__ASSEMBLY__*/ /*FD-20090114*/


