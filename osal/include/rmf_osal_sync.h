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

/**
 * @file rmf_osal_sync.h
 * @brief The header file contains prototypes for Mutex and Condition.
 */

/**
 * @defgroup OSAL_SYNC OSAL Process Synchronization & Mutex
 *
 * RMF OSAL Synchronization API provides support for management and use of synchronization
 * constructs. The main two constructs provided are re-entrant mutexes and condition variables,
 * which are very similar to binary semaphores. Mutexes offer a fairly simple and optimized
 * solution for most common synchronization tasks, while the condition variable design offers
 * flexible and powerful set of synchronization features.
 *
 * @par Mutex Operation
 * A mutex or mutual exclusion is used to protect a shared resource from access by
 * multiple threads at the same time. A shared resource may be a common data structure
 * in RAM or it may be a hardware peripheral. In either case a mutex can be used to ensure
 * the integrity of the entire resource by only allowing one thread to access it at a time.
 *
 * @ingroup OSAL
 *
 * @defgroup OSAL_MUTEX_API OSAL Mutex API list
 * RMF OSAL Synchronization module defines interfaces for management and use of synchronization
 * constructs such as mutex and condition variables.
 * Described the details about OSAL Mutex interface specification.
 * @ingroup OSAL_SYNC
 *
 * @defgroup OSAL_COND_API OSAL Process Synchronization API list
 * Described the details about OSAL Process Synchronization interface specification.
 * @ingroup OSAL_SYNC
 *
 * @addtogroup OSAL_SYNC_TYPES OSAL Process Synchronization and Mutex Data Types
 * Described the details about Data Structure used by OSAL Process Synchronization & Mutex.
 * @ingroup OSAL_SYNC
 */

#if !defined(_RMF_OSAL_SYNC_H)
#define _RMF_OSAL_SYNC_H

#include "rmf_osal_types.h"	/* Resolve basic type references. */
#include "rmf_osal_error.h"	/* Return values. */

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @addtogroup OSAL_SYNC_TYPES
 * @{
 */

typedef os_Mutex rmf_osal_Mutex; /**< Mutex type binding. */
//typedef os_Condition rmf_osal_Cond; /* Condition type binding. */
typedef void* rmf_osal_Cond; /**< Condition type binding. */

/** @} */

/***
 * Mutex API prototypes:
 */

//#define DEBUG_MUTEX_ALLOC

/**
 * @addtogroup OSAL_MUTEX_API
 * @{
 */
#ifndef DEBUG_MUTEX_ALLOC
rmf_Error rmf_osal_mutexNew(rmf_osal_Mutex *mutex);
#else
rmf_Error rmf_osal_mutexNew_track(rmf_osal_Mutex *mutex, const char* file, int line);
#define rmf_osal_mutexNew(pmutex) rmf_osal_mutexNew_track(pmutex, __FILE__, __LINE__)

void rmf_osal_mutex_dump();
#endif

rmf_Error rmf_osal_mutexDelete(rmf_osal_Mutex mutex);

rmf_Error rmf_osal_mutexAcquire(rmf_osal_Mutex mutex);

rmf_Error rmf_osal_mutexAcquireTry(rmf_osal_Mutex mutex);

/**
 * The <i>rmf_osal_mutexAcquireTimed()</i> function will attempt to acquire ownership of
 * the target within the time perios specified.  If the mutex cannot zquire within the 
 * time or busy an error will be returned to indicate failure to acquire the mutex.
 *
 * @param mutex is the identifier of the mutex to acquire.
 * @param timeout_usec the timeout
 * @return Returns relevant RMF_OSAL error code on failure, otherwise RMF_SUCCESS
 *         is returned.
 */
rmf_Error rmf_osal_mutexAcquireTimed(rmf_osal_Mutex mutex, int32_t timeout_usec);

/**
 * The <i>rmf_osal_mutexRelease()</i> function will release ownership of a mutex.  The
 * current thread must own the mutex in order to release it.
 *
 * @param mutex is the identifier of the mutex to release.
 * @return Returns relevant RMF_OSAL error code on failure, otherwise RMF_SUCCESS 
 *         is returned.
 */
rmf_Error rmf_osal_mutexRelease(rmf_osal_Mutex mutex);

/** @} */ //end of doxygen tag OSAL_MUTEX_API

/***
 * Condition API prototypes:
 */

/**
 * @addtogroup OSAL_COND_API
 * @{
 */
rmf_Error rmf_osal_condNew(rmf_osal_Bool autoReset, rmf_osal_Bool initialState,
		rmf_osal_Cond *cond);

rmf_Error rmf_osal_condDelete(rmf_osal_Cond cond);

rmf_Error rmf_osal_condGet(rmf_osal_Cond cond);

rmf_Error rmf_osal_condWaitFor(rmf_osal_Cond cond, uint32_t timeout);

rmf_Error rmf_osal_condSet(rmf_osal_Cond cond);

rmf_Error rmf_osal_condUnset(rmf_osal_Cond cond);
 
/** @} */ //end of doxygen tag OSAL_COND_API

#ifdef __cplusplus
}
#endif
#endif /* _RMF_OSAL_SYNC_H */

