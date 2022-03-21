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
 * @defgroup OSAL_THREAD OSAL Thread
 * RMF OSAL thread module provides interfaces for creation and management of threads.
 * @ingroup OSAL
 *
 * @defgroup OSAL_THREAD_API OSAL Thread API list
 * RMF OSAL Thread module defines interfaces for creation and management of threads.
 * @ingroup OSAL_THREAD
 *
 * @addtogroup OSAL_THREAD_TYPES OSAL Thread Data Types
 * Described the details about Data Structure used by OSAL Thread.
 * @ingroup OSAL_THREAD
 */

/**
 * @file rmf_osal_thread.h
 * @brief The header file contains prototypes of thread specific APIs 
 *
 * @defgroup OSAL OSAL (Operating System Abstraction Layer)
 * <b> This section provides an overview of the OSAL Library</b>
 *
 * The OSAL Library provides a defined interface such that driver and middleware developers
 * will be able to create RDK components code that can safely operate in a multi-threaded environment.
 * This is being used in a RTOS & non-RTOS environment with an interrupt or non-interrupt
 * driven application model.
 * 
 * An operating system abstraction layer (OSAL) provides an application programming interface (API) 
 * to an abstract operating system making it easier and quicker to develop code for multiple software
 * or hardware platforms.
 *
 * OS abstraction layers deal with presenting an abstraction of the common system functionality 
 * that is offered by any Operating system by the means of providing meaningful and easy to use 
 * Wrapper functions that in turn encapsulate the system functions offered by the OS to which 
 * the code needs porting. The OSAL provides implementations of an API for several
 * real-time operating systems and the implementations also be provided for non real-time operating
 * systems, allowing the abstracted software to be developed and tested in a developer friendly environment.
 *
 * @ingroup RMF
 */

#if !defined(_RMF_OSAL_THREAD_H_)
#define _RMF_OSAL_THREAD_H_

#include <rmf_osal_types.h>		/* Resolve basic type references. */
#include <rmf_osal_error.h>		/* Resolve basic error definitions. */
//#include <os_thread.h>		/* Resolve target specific timer definitions. */

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @addtogroup OSAL_THREAD_TYPES
 * @{
 */

/***
 * Thread macro and type definitions:
 *
 * check Linux thingies can be referenced here or not.
 */
#if !defined(RMF_OSAL_THREAD_PRIOR_MAX)
#define RMF_OSAL_THREAD_PRIOR_MAX  			(24)	/**< Maximum thread priority. */
#endif
#if !defined(RMF_OSAL_THREAD_PRIOR_MIN)
#define RMF_OSAL_THREAD_PRIOR_MIN  			(31)   /**< Minimum thread priority. */
#endif
#if !defined(RMF_OSAL_THREAD_PRIOR_DFLT)
#define RMF_OSAL_THREAD_PRIOR_DFLT 			(28)   /**< Default thread priority. */
#endif
#if !defined(RMF_OSAL_THREAD_PRIOR_INC)
#define RMF_OSAL_THREAD_PRIOR_INC  			(-1)   /**< Priority setting increment. */
#endif
#if !defined(RMF_OSAL_THREAD_PRIOR_SYSTEM_HI)
#define RMF_OSAL_THREAD_PRIOR_SYSTEM_HI		(RMF_OSAL_THREAD_PRIOR_MAX+1)
#endif
#if !defined(RMF_OSAL_THREAD_PRIOR_SYSTEM_MED)
#define RMF_OSAL_THREAD_PRIOR_SYSTEM_MED		(RMF_OSAL_THREAD_PRIOR_MAX+2)
#endif
#if !defined(RMF_OSAL_THREAD_PRIOR_SYSTEM)
#define RMF_OSAL_THREAD_PRIOR_SYSTEM			(RMF_OSAL_THREAD_PRIOR_MAX+3)
#endif

/**
 * Thread status bit definitions: 
 */
typedef uint32_t rmf_osal_ThreadStat; /* Thread status type. */

#define RMF_OSAL_THREAD_STAT_MUTEX  (0x00000001)     /**< Acquire(d) mutext mode. */
#define RMF_OSAL_THREAD_STAT_COND   (0x00000002)     /**< Acquire(d) cond mode. */
#define RMF_OSAL_THREAD_STAT_EVENT  (0x00000004)     /**< Get event mode. */
#define RMF_OSAL_THREAD_STAT_CALLB  (0x00000008)		/**< Event callback context mode. */
#define RMF_OSAL_THREAD_STAT_DEATH  (0x00008000)		/**< Thread death mode. */


/**
 * Define standard thread stack size:
 */
#define RMF_OSAL_THREAD_STACK_SIZE			(64*1024)


typedef os_ThreadId rmf_osal_ThreadId; /**< Thread identifier type binding. */

/** @} */ //end of doxygen tag OSAL_THREAD_TYPES

/**
 * @struct rmf_osal_ThreadPrivateData
 * @brief RMF_OSAL private thread data.
 */
typedef struct rmf_osal_ThreadPrivateData
{
    void* threadData; /**< Thread data as returned by rmf_osal_threadGetData() */
    int64_t memCallbackId; /**< Resource Reclamation contextual information.  Inherited from parent. */
} rmf_osal_ThreadPrivateData;

/**
 * @addtogroup OSAL_THREAD_API
 * @{
 */

rmf_Error rmf_osal_threadCreate(void(*entry)(void *), void *data,
        uint32_t priority, uint32_t stackSize, rmf_osal_ThreadId *threadId,
        const char *name);

rmf_Error rmf_osal_threadDestroy(rmf_osal_ThreadId threadId);

rmf_Error rmf_osal_threadAttach(rmf_osal_ThreadId *threadId);

rmf_Error rmf_osal_threadSetPriority(rmf_osal_ThreadId threadId, uint32_t priority);

rmf_Error rmf_osal_threadGetStatus(rmf_osal_ThreadId threadId, rmf_osal_ThreadStat *threadStatus);

rmf_Error rmf_osal_threadSetStatus(rmf_osal_ThreadId threadId, rmf_osal_ThreadStat threadStatus);

rmf_Error rmf_osal_threadGetData(rmf_osal_ThreadId threadId, void **threadLocals);

rmf_Error rmf_osal_threadSetData(rmf_osal_ThreadId threadId, void *threadLocals);

rmf_Error rmf_osal_threadGetPrivateData(rmf_osal_ThreadId threadId,
        rmf_osal_ThreadPrivateData **data);

rmf_Error rmf_osal_threadGetCurrent(rmf_osal_ThreadId *threadId);

rmf_Error rmf_osal_threadSleep(uint32_t milliseconds, uint32_t microseconds);

void rmf_osal_threadYield(void);

rmf_Error rmf_osal_threadSetName(rmf_osal_ThreadId threadId, const char* name);
 
/** @} */ //End of Doxygen tag OSAL_THREAD_API

#ifdef __cplusplus
}
#endif
#endif /* _RMF_OSAL_THREAD_H_ */

