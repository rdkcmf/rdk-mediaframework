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
 * @file rmf_osal_thread.c
 * @brief The source file contains definition of thread specific APIs 
 */

#include <stdio.h>
#include <unistd.h>         /* usleep(3) */
#include <string.h>

#ifdef _POSIX_PRIORITY_SCHEDULING
	#include <sched.h>
#endif

#if USE_SYSRES_MLT
#include "mlt_malloc.h"
#endif

#include <errno.h>

#include "rmf_osal_types.h"
#include "rmf_osal_error.h"
#include "rmf_osal_thread.h"
#include "rmf_osal_mem.h"
#include "rmf_osal_sync.h"
#include "rmf_osal_util.h"     /* Resolve setjmp/longjmp defs. */
#include "rdk_debug.h"
#include "rmf_osal_thread_priv.h"

//#include <platform_misc.h> /* RclaimMemToken */
#include "rmf_osal_resreg.h"

#define RMF_OSAL_MAX_THREAD_NAME 128
/***
 * Macros for binding the thread list mutex operations:
 */
#define threadMutexAcquire() rmf_osal_mutexAcquire(threadMutex)
#define threadMutexRelease() rmf_osal_mutexRelease(threadMutex)


//#define DEBUG_THREADS_SYS_TID

#ifdef DEBUG_THREADS_SYS_TID
#include <sys/syscall.h>
#endif 
#include <sys/syscall.h>

typedef struct _threadDesc threadDesc;

struct _threadDesc
{
	os_ThreadId				td_id;				/* Thread identifier. 								*/
	uint32_t         		td_refcount;	  	/* Thread attach reference count. 					*/
	void          		   *td_vmlocals;		/* VM thread local pointer. 						*/
	os_Mutex       			td_mutex;			/* Mutex for accessing thread descriptor. 			*/
	os_ThreadStat  			td_status;			/* OSAL implementation thread status. 				*/
	rmf_osal_ThreadPrivateData	td_locals;  		/* OSAL thread local storage pointer. 				*/
	jmp_buf       			td_exitJmp;			/* Termination jump buffer. 						*/
	void         		  (*td_entry)(void*);	/* Thread entry point. 								*/
	void          		   *td_entryData;		/* Thread entry point data. 						*/
	threadDesc    		   *td_prev;			/* Previous descriptor. 							*/
	threadDesc    		   *td_next;			/* Next descriptor. 								*/
	pthread_attr_t 			td_attr;			/* Thread attributes. 								*/
	char           			td_name[1];			/* Thread name. -- MUST BE LAST MEMBER OF STRUCTURE	*/
};

/****
 * Implementation specific data:
 */
static os_Mutex threadMutex = NULL;

/****
 * The threads[] array holds two pointers. These are the td_prev and td_next items
 * of the fake head of the thread descriptor list. Later in the code, you'll see the
 * RMF_OSAL_FAKEHEAD macro used to calculate a "fake" head pointer. The fake head pointer
 * is positioned in such a way that head->td_prev is threads[0] and head->td_next is
 * threads[1].
 *
 * Obivously the fake head pointer does not point to anything real, so other members
 * of the threadDesc struct must never be accessed. Only head->td_prev and head->td_next
 * can be safely written.
 */
/*static*/ threadDesc *threads[2] = {NULL, NULL};

/****
 * Implementation function prototypes:
 *
 */
static rmf_Error    threadInit(void);
static rmf_Error    threadAllocDesc(threadDesc **, const char *);
static void         threadTermDesc(threadDesc *);
static void         threadStart(void *);
static uint32_t		threadMapPriority(uint32_t);
static void         threadSetDesc(threadDesc *);
static threadDesc *	threadGetDesc(rmf_osal_ThreadId);

/**
 * @brief This function is used to create a new operating system
 * specific thread.  If allowed by the target OS the new thread's priority will
 * be set to the specified priority level.  
 *
 * @param[in] entry is a function pointer to the thread's execution entry point.  
 *          The function pointer definition specifies reception of a single parameter, 
 *          which is a void pointer to any data the thread requires.
 * @param[in] data is a void pointer to any thread specific data that is to be passed
 *          as the single parameter to the thread's execution entry point.
 * @param[in] priority is the initial execution priority of the new thread.
 * @param[in] stackSize is the size of the thread's stack, if zero is specified, a
 *          default size appropriate for the system will be used.
 * @param[in] threadId is a pointer for returning the new thread's identifier or
 *          handle.  The identifier is of rmf_osal_ThreadId type.
 * @param[in] name is the name of the new thread, which is only applicable to target
 *        operating systems that allow thread naming.  The RMF_OSAL layer does
 *        not support naming if the target operating system doesn't.
 * @return The RMF_OSAL error code if the create fails, otherwise RMF_SUCCESS is returned.
 */
rmf_Error rmf_osal_threadCreate(void 			(*entry) (void *),
							 void *			data,
							 uint32_t 		priority,
							 uint32_t 		stackSize,
							 rmf_osal_ThreadId *	threadId,
							 const char *	name)	  // the create a unique thread each time
{

	volatile rmf_Error 	ec;						// Error condition code
	volatile uint32_t   newPriority;			// mapped priority
	int                 pRet;
	threadDesc *		self;					// Current thread descriptor pointer
	threadDesc *		td         = NULL;		// New thread descriptor pointer
	char *				threadName = NULL;		// Thread name pointer
	char                threadNameBuf[16];		// Buffer for thread name generation
	static uint32_t     unnamed    = 0;			// Static variable for name gen support
	struct sched_param  param;

	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.THREAD", "rmf_osal_threadCreate() %s\n", name);

	if ((NULL == entry) || (NULL == threadId))
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.THREAD",
                  "rmf_osal_threadCreate() threadId = %p, entry = %p\n", threadId, entry);
		return RMF_OSAL_EINVAL;
	}

	/* Check for need to initialize thread list. */
	if (threads[0] == NULL)
	{
		if (RMF_SUCCESS != (ec = threadInit()))
		{
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.THREAD",
					  "rmf_osal_threadCreate() threadInit failed\n");
			return ec;
		}
	}

	/* If the thread isn't named, generate one. */
	if (NULL == name)
	{
		(void) snprintf(threadNameBuf,sizeof(threadNameBuf), "rmf_osal_%ld", (long)++unnamed);
		threadName = threadNameBuf;
	}
	else
	{
		threadName = (char*)name;
	}

	threadMutexAcquire(); /* Lock list. */
	{
		/* Allocate a new thread descriptor. */
		if (RMF_SUCCESS != (ec = threadAllocDesc(&td, threadName)))
		{
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.THREAD",
					  "rmf_osal_threadCreate() threadAllocDesc failed\n");
			threadMutexRelease(); /* Unlock list. */
			return ec;
		}

		/* Set thread entry & data. */
		td->td_entry		= entry;	 /* Thread's data passed in. */
		td->td_entryData	= data;	 /* Thread's function. */
		td->td_refcount 	= 1;	  /* Threads attach reference count. */
		td->td_id			= 0;			/* Clear thread Id. */

		/* Copy inheritable private thread local data. */
		self = threadGetDesc((rmf_osal_ThreadId)0);
		if (self != NULL)
		{
			td->td_locals.memCallbackId = self->td_locals.memCallbackId;
		}

		/* Make sure priority is suitable for target. */
		newPriority = threadMapPriority(priority);

		(void) pthread_attr_init(&td->td_attr);
		// OSAL does not currently support "join" syntax
		(void) pthread_attr_setdetachstate(&td->td_attr,
										   PTHREAD_CREATE_DETACHED);
/*		(void) pthread_attr_setschedpolicy(&td->td_attr,
		 SCHED_RR);*/
		(void) pthread_attr_setschedpolicy(&td->td_attr,
		 SCHED_OTHER);

		param.sched_priority = newPriority;
		(void) pthread_attr_setschedparam(&td->td_attr,
										  &param);

		ec = RMF_SUCCESS; /* by default, assume success */

		/* Initialize the thread's local data storage pointer. */
		/* Parker-4153: Move to here before thread creation to avoid race conditions */
		td->td_locals.threadData = 0; 

		pRet = pthread_create(&td->td_id,
							  &td->td_attr,
							  (void *(*)(void *))threadStart,
							  td);
		if (0 != pRet)
		{
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.THREAD",
					  "rmf_osal_threadCreate() failed: %d\n", pRet);
			ec = RMF_OSAL_ENOMEM;
		}

		*threadId = td->td_id;

		threadMutexAcquire(); /* Lock list. */
		{
			if (NULL == threadGetDesc(td->td_id))
			{
				/* Set the thread's descriptor storage structure pointer. */
				threadSetDesc(td);
			}
		}
		threadMutexRelease(); /* Unlock list. */
		
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.THREAD", "    new thread id:%d (%s)\n", (int)*threadId, name);


		RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.THREAD", " [%d] spawned thread id: %lu (%s)\n", getpid(),*threadId,name);
	}
	threadMutexRelease(); /* Unlock list. */
	return ec;
}


/**
 * @brief This function is used to terminate the current thread or a specified target
 * operating system thread. 
 *
 * If the target thread identifier is zero the current (calling) thread is terminated.
 * If the OS does not support the ability to terminate arbitrary threads, then this
 * function will return an error.
 *
 * @param[in] threadId This is the identifier/handle of the target thread to terminate.  
 *          It is the original handle returned from the thread creation API.  If the
 *          thread identifier is zero or mathes the identifier of the current
 *          thread, then the calling thread will be terminated.
 * @return An RMF_OSAL error code if the destroy fails, otherwise RMF_SUCCESS is returned.
 */
rmf_Error rmf_osal_threadDestroy(rmf_osal_ThreadId threadId)
{
	rmf_osal_ThreadId	 current;	/* Current thread ID. */
	threadDesc		*td = NULL;			/* Thread descriptor pointer. */

	threadMutexAcquire(); /* Lock list. */
	{
	//	  RDK_LOG(RDK_LOG_INFO, "LOG.RDK.THREAD", "rmf_osal_threadDestroy(%d)\n",threadId);
		/* Get the thread's locals. */
		if ((td = threadGetDesc(threadId)) == NULL)
		{
			threadMutexRelease(); /* Unlock list. */
			return RMF_OSAL_EINVAL;
		}

		/* Mark the target thread for death. */
		td->td_status |= RMF_OSAL_THREAD_STAT_DEATH;

		/* Check for termination of the current thread. */
		if ((threadId == 0) || ((rmf_osal_threadGetCurrent(&current) == RMF_SUCCESS)
								&& (threadId == current)))
		{
			/* VS do we need a long Jump to the termination point for current thread. */
			td->td_status |= RMF_OSAL_THREAD_STAT_DEATH;
			if (threadGetDesc(threadId) == td)
			{
				// thread descriptor still exists so clean-up.
				threadTermDesc(td);
			}
			threadMutexRelease(); /* Unlock list. */
			pthread_exit(NULL);
		}
		else
		{
			pthread_cancel((pthread_t)(threadId));
			if (threadGetDesc(threadId) == td)
			{
				// thread descriptor still exists so clean-up.
				threadTermDesc(td);
			}
		}

	}
	threadMutexRelease(); /* Unlock list. */

	return RMF_SUCCESS;
}


/**
 * @brief This function is used to get the target thread's status variable.
 * If the thread identifier parameter is zero or matches the identifier
 * of the current thread, then the calling thread's status is returned.
 *
 * @param[in] threadId is the identifier of the target thread.
 * @param[in] status is a pointer to an unsigned integer for returning the thread's current status.
 * @return The RMF_OSAL error code if the create fails, otherwise RMF_SUCCESS is returned.
 */

rmf_Error rmf_osal_threadGetStatus(rmf_osal_ThreadId threadId, rmf_osal_ThreadStat *status)
{
	threadDesc *td = NULL;		/* Thread descriptor pointer. */

	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.THREAD", "rmf_osal_threadGetStatus:%d\n", (int)threadId);
	/* Sanity check... */
	if (status == NULL)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.THREAD",
				  "rmf_osal_threadGetStatus status is NULL");
		return RMF_OSAL_EINVAL;
	}

	threadMutexAcquire(); /* Lock list. */
	{
		/* Get the thread's locals. */
		if ((td = threadGetDesc(threadId)) == NULL)
		{
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.THREAD",
					  "rmf_osal_threadGetStatus threadGetDesc returned NULL");
			threadMutexRelease(); /* Unlock list. */
			return RMF_OSAL_EINVAL;
		}

		/* Return the currect thread status. */
		*status = td->td_status;
	}
	threadMutexRelease(); /* Unlock list. */

	return RMF_SUCCESS;
}


/**
 * @brief This function is used to set the target thread's status variable.
 * If the thread identifier parameter is zero or matches the identifier of the current thread,
 * then the calling thread's status is set.
 *
 * @param[in] threadId is the identifier of the target thread.
 * @param[in] status is the threads new status.
 * @return The RMF_OSAL error code if the create fails, otherwise RMF_SUCCESS is returned.
 */
rmf_Error rmf_osal_threadSetStatus(rmf_osal_ThreadId threadId, rmf_osal_ThreadStat status)
{
	threadDesc *td = NULL;		/* Thread descriptor pointer. */

	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.THREAD", "rmf_osal_threadSetStatus:%d\n", (int)threadId);
	threadMutexAcquire(); /* Lock list. */
	{
		/* Get the thread's descriptor. */
		if ((td = threadGetDesc(threadId)) == NULL)
		{
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.THREAD",
					  "rmf_osal_threadSetStatus threadGetDesc returned NULL");
			threadMutexRelease(); /* Unlock list. */
			return RMF_OSAL_EINVAL;
		}

		/* Set the thread's status variable. */
		td->td_status = status;
	}
	threadMutexRelease(); /* Unlock list. */

	return RMF_SUCCESS;
}


/**
 * @brief This function is used to get the target thread's "thread local storage".
 * If the thread identifier parameter is zero or matches the identifier
 * of the current thread, then the calling thread's "thread local storage" is returned.
 *
 * @param[in] threadId is the identifier of the target thread.
 * @param[in] threadLocals is a void pointer for returning the thread specific data
 *          pointer.
 * @return The RMF_OSAL error code if the create fails, otherwise RMF_SUCCESS is returned.
 */
rmf_Error rmf_osal_threadGetData(rmf_osal_ThreadId threadId, void **threadLocals)
{
	threadDesc *td = NULL;		/* Thread descriptor pointer. */

	/* Sanity check. */
	if (threadId == (rmf_osal_ThreadId)-1 || threadLocals == NULL)
	{
		return RMF_OSAL_EINVAL;
	}

	threadMutexAcquire(); /* Lock list. */
	{
		/* Get the thread's locals. */
		if ((td = threadGetDesc(threadId)) == NULL)
		{
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.THREAD",
					  "rmf_osal_threadGetData threadGetDesc returned NULL for thread %x", threadId);
			threadMutexRelease(); /* Unlock list. */
			return RMF_OSAL_EINVAL;
		}

		/* Return the correct thread local storage pointer. */
		*threadLocals = td->td_locals.threadData;
		if(td->td_locals.threadData == 0)
		{
			 RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.THREAD",
					  "td_locals.threadData is not set (0)");
		}
	}
	threadMutexRelease(); /* Unlock list. */

	return RMF_SUCCESS;
}




/**
 * @brief This function is used to retrieve the RMF_OSAL private data for the given target thread.
 * If the thread identifier is zero, that the private data for the current thread is returned.
 *
 * @param[in] threadId is the identifier of the target thread.
 * @param[in] data is a pointer to the location where the pointer the the target thread's
 *         private data should be written
 * @return the RMF_OSAL error code if the operation fails, otherwise RMF_SUCCESS is returned
 */
rmf_Error rmf_osal_threadGetPrivateData(rmf_osal_ThreadId  threadId, rmf_osal_ThreadPrivateData **data)
{
	threadDesc *td;		/* Thread descriptor pointer. */

	threadMutexAcquire(); /* Lock list. */
	{
		/* Get the thread's descriptor. */
		if ((td = threadGetDesc(threadId)) == NULL)
		{
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.THREAD",
					  "rmf_osal_threadGetPrivateData threadGetDesc returned NULL");
			threadMutexRelease(); /* Unlock list. */
			return RMF_OSAL_EINVAL;
		}

		*data = (rmf_osal_ThreadPrivateData*)(&td->td_locals);
	}
	threadMutexRelease(); /* Unlock list. */

	return RMF_SUCCESS;
}


/**
 * @brief This function is used to set the target thread's local storage.
 * If the thread identifier parameter is zero or matches the identifier of the current 
 * thread, then the calling thread's "thread local storage" is set.
 *
 * @param[in] threadId is the identifier of the target thread.
 * @param[in] threadLocals is a void pointer to the thread specific data.
 * @return The RMF_OSAL error code if the create fails, otherwise RMF_SUCCESS is returned.
 */
rmf_Error rmf_osal_threadSetData(rmf_osal_ThreadId  threadId, void *threadLocals)
{
	threadDesc *td = NULL;		/* Thread descriptor pointer. */

	/* Fail for null threadLocals. */
	/*if (threadLocals == NULL)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.THREAD",
				  "rmf_osal_threadSetData NULL threadLocals parameter");
		return RMF_OSAL_EINVAL;
	}*/

	threadMutexAcquire(); /* Lock list. */
	{
		/* Get the thread's descriptor. */
		if ((td = threadGetDesc(threadId)) == NULL)
		{
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.THREAD",
					  "rmf_osal_threadSetData threadGetDesc returned NULL");
			threadMutexRelease(); /* Unlock list. */
			return RMF_OSAL_EINVAL;
		}

		/* Set the thread's local data storage pointer. */
		td->td_locals.threadData = threadLocals;
	}
	threadMutexRelease(); /* Unlock list. */

	return RMF_SUCCESS;
}

//
// VMDATA is not currently used - leave this code here in case we ever need local storage for the VM
//
//
#if defined(RMF_OSAL_FEATURE_VMDATA)
/**
 * @brief This function is used to get the target thread's "thread local storage".
 * If the thread identifier parameter is zero or matches the identifier of the current
 * thread, then the calling thread's "thread local storage" is returned.
 *
 * @param[in] threadId is the identifier of the target thread.
 * @param[in] threadLocals is a void pointer for returning the VM thread specific data
 *          pointer.
 *
 * @return The RMF_OSAL error code if the create fails, otherwise RMF_SUCCESS is returned.
 */
rmf_Error rmf_osal_threadGetVMData(rmf_osal_ThreadId threadId, void **threadLocals)
{
	threadDesc *td;		/* Thread descriptor pointer. */

	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.THREAD", "rmf_osal_threadGetVMData:%d\n",threadId);
	/* Sanity check... */
	if (threadLocals == NULL))
	{
		return RMF_OSAL_EINVAL;
	}

	threadMutexAcquire(); /* Lock list. */
	{
		/* Get the thread's locals. */
		if ((td = threadGetDesc(threadId)) == NULL)
		{
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.THREAD",
					  "rmf_osal_threadGetVMData threadGetDesc returned NULL");
			threadMutexRelease(); /* Unlock list. */
			return RMF_OSAL_EINVAL;
		}

		/* Return the currect thread's VM local storage pointer. */
		*threadLocals = td->td_vmlocals;
	}
	threadMutexRelease(); /* Unlock list. */

	return RMF_SUCCESS;
}


/**
 * @brief This function is used to set the target thread's VM local storage.
 * If the thread identifier parameter is zero or matches the identifier of the current 
 * thread, then the calling thread's VM "thread local storage" is set.
 *
 * @param[in] threadId is the identifier of the target thread.
 * @param[in] threadLocals is a void pointer to the thread specific data.
 *
 * @return The RMF_OSAL error code if the create fails, otherwise RMF_SUCCESS is returned.
 */
rmf_Error rmf_osal_threadSetVMData(rmf_osal_ThreadId threadId, void *threadLocals)
{
	threadDesc *td;		/* Thread descriptor pointer. */

	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.THREAD", "rmf_osal_threadSetVMData:%d\n",threadId);
	threadMutexAcquire(); /* Lock list. */
	{
		/* Get the thread's descriptor. */
		if ((td = threadGetDesc(threadId)) == NULL)
		{
			threadMutexRelease(); /* Unlock list. */
			return RMF_OSAL_EINVAL;
		}

		/* Set the thread's local data storage pointer. */
		td->td_vmlocals = threadLocals;
	}
	threadMutexRelease(); /* Unlock list. */

	return RMF_SUCCESS;
}
#endif /* RMF_OSAL_FEATURE_VMDATA */


/**
 * @brief This function used to retrieve the thread identifier of the current (calling) thread.
 *
 * @return The RMF_OSAL error code if the create fails, otherwise RMF_SUCCESS is returned.
 */
rmf_Error rmf_osal_threadGetCurrent(rmf_osal_ThreadId *threadId)
{
	if (NULL == threadId)
	{
		//RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.THREAD", "rmf_osal_threadGetCurrent() NULL threadId ...\n");
		return RMF_OSAL_EINVAL;
	}
	*threadId = pthread_self();
//    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.THREAD", "rmf_osal_threadGetCurrent %p\n",*threadId);
	return RMF_SUCCESS;
}


/**
 * @brief The function is used to put the current thread to sleep (i.e. suspend execution) for specified time.
 *
 * The amount of time that the thread will be suspened is the sum of the number of milliseconds 
 * and microoseconds specified.  An error will be returned if the thread is woken (activated) prematurely.
 *
 * @param[in] milliseconds is the number of milliseconds to sleep.
 * @param[in] microseconds is the number of microseconds to sleep.
 *
 * @return The RMF_OSAL error code if the create fails, otherwise RMF_SUCCESS is returned.
 */
rmf_Error rmf_osal_threadSleep(uint32_t milliseconds, uint32_t microseconds)
{

	unsigned long usec = milliseconds * 1000 + microseconds;
	usleep(usec);
	return RMF_SUCCESS;
}


/**
 * @brief The function is used to give the remainder of the calling thread's current timeslice.
 * @return None
 */

void rmf_osal_threadYield(void)
{
#ifdef _POSIX_PRIORITY_SCHEDULING
	int ret = sched_yield();
	if (0 != ret)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.THREAD",
				  "sched_yield() failed: %d (%d)\n", ret, errno);
	}
#else
#error "No rmf_osal_threadYield implementation!"
#endif
	return;
}


/**
 * @brief This function is used to initialize RMF OSAL thread specific implementation variables.
 *
 * @return The RMF_OSAL error code if the create fails, otherwise RMF_SUCCESS is returned.
 */
rmf_Error threadInit(void)
{
	rmf_Error ec;	/* Error condition code. */

	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.THREAD", "rmf_osal_thread.c:threadInit()\n");

	/* Check for need to initialize thread list. */
	if (threads[0] == NULL)
	{
		threadDesc *head = RMF_OSAL_FAKEHEAD(threadDesc, threads[0], td_prev);

		/* Allocate thread list mutex. */
		if ((ec = rmf_osal_mutexNew(&threadMutex)) != RMF_SUCCESS)
		{
			return ec;
		}
		
		threadMutexAcquire(); /* Lock list. */
		{
			head->td_prev = head->td_next = head;
		}
		threadMutexRelease(); /* Unlock list. */
	}

	threadMutexAcquire(); /* Lock list. */
	{
		/* Attach the current thread. This means that the current thread will
		 * have a valid descriptor which can be used in rmf_osal_threadCreate(). */
		rmf_osal_ThreadId tid = 0;
		ec = rmf_osal_threadAttach(&tid);
		if (ec != RMF_SUCCESS)
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.THREAD",
					"rmf_osal_thread.c:threadInit() could not attach current thread\n");
	}
	threadMutexRelease(); /* Unlock list. */

	return RMF_SUCCESS;
}


/**
 * @brief This function is used to setup for termination of the current thread prior to actually 
 * calling the thread's entry point.
 *
 * This setup allows the thread to "long jump" back
 * to this routine.  This ability to long jump will help with implementing
 * "thread death" scenarios for assisting with robust application isolation.
 *
 * @param[in] tls is the thread descriptor pointer.
 *
 * @return nothing.
 */
static void threadStart(void *tls)
{
	threadDesc *td = (threadDesc *)tls;		/* Thread descriptor pointer. */

	/* Sanity check... */
	if (tls == NULL)
	{
		return;
	}

#ifdef DEBUG_THREADS_SYS_TID
	int sysTid = (int)syscall(SYS_gettid);

	if (td->td_name != 0)
	{
		printf("\nrmf_osal_threadCreate, name : %s sys tid : %d ****************\n",td->td_name, sysTid);
		rmf_osal_ResRegSet(RMF_OSAL_RES_TYPE_THREAD, sysTid, td->td_name, 0, 0);
	}
	else
	{
		printf("\nrmf_osal_threadCreate, name NOT SPECIFIED, sys tid : %d ****************\n",td->td_name, sysTid);
		rmf_osal_ResRegSet(RMF_OSAL_RES_TYPE_THREAD, sysTid, "(null)", 0, 0);
	}
#endif

	rmf_osal_ThreadRegSet(RMF_OSAL_RES_TYPE_THREAD, 0, td->td_name, 1024*1024, 0);

	/* If parent didn't get a chance to set Id in rmf_osal_threadCreate, set it now. */
	threadMutexAcquire(); /* Lock list. Parker-4153 */
	if (td->td_id == 0)
	{
		td->td_id = (rmf_osal_ThreadId) pthread_self();
	}

	// need to check that set thread name as in PTV code or not.

	{
		if (NULL == threadGetDesc(0))
		{
			/* Set the thread's descriptor storage structure pointer. */
			threadSetDesc(td);
		}
	}
	threadMutexRelease(); /* Unlock list. */

	/* Perform exit support setjmp operation. */
	if (setjmp(td->td_exitJmp) == 0)
	{
		/* Invoke thread's execution entry point. */
		(td->td_entry)(td->td_entryData);
	}

	threadMutexAcquire(); /* Lock list. */
	{
		/*
		 * The thread is done executing, check for need to release its descriptor.
		 */
		if (threadGetDesc(0) == td)
		{
			RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.THREAD", "Thread is done id=0x%x td=0x%x\n", (unsigned int)td->td_id, (unsigned int)td);
			/* Terminate descriptor use. */
			threadTermDesc(td);
		}
	}
	threadMutexRelease(); /* Unlock list. */

	return;
}


/**
 * @brief This function is used to allocate a new thread descriptor structure and allocate fields with
 * default values.
 *
 * @param[in] name name of the thread.
 * @param[in] td is a pointer for returning the new thread descriptor pointer.
 *
 * @return The RMF_OSAL error code if the create fails, otherwise RMF_SUCCESS is returned.
 */
static rmf_Error threadAllocDesc(threadDesc **td, const char *name)
{
	rmf_Error ec;						/* Error condition code. */
	threadDesc *ntd = 0;				/* Thread descriptor pointer. */
	uint32_t size = sizeof(threadDesc);	  /* Size of thread descriptor. */

	/* Sanity check... */
	/* internal function, so *td is always valid */

	/* Check for name. */
	/*if (name != NULL)
	{
		size += strlen(name) + 1;
	}
	else Note:- thread name is set later by the JVM */
	{
		/* for thread name */
		size += RMF_OSAL_MAX_THREAD_NAME;
	}

	threadMutexAcquire(); /* Lock list. */
	{
		/* Allocate a new thread local storage structure. */
		if ((ec = rmf_osal_memAllocP(RMF_OSAL_MEM_THREAD, size, (void**)&ntd)) != RMF_SUCCESS)
		{
			threadMutexRelease(); /* Unlock list. */
			return ec;
		}


	#if 0
		/* Initialize fields. */
		(void) memset(ntd, 0, size); /* mandatory */
		ntd->td_status = 0;
		ntd->td_refcount = 0;
		ntd->td_vmlocals = 0;
		(void) memset(&(ntd->td_locals), 0, sizeof(ntd->td_locals));
		ntd->td_entry = 0;
		ntd->td_entryData = 0;
	#else
		(void) memset(ntd, 0, size); /* mandatory */
	#endif
		if (0 != name)
		{
			strncpy(&(ntd->td_name[0]), name, RMF_OSAL_MAX_THREAD_NAME-1);
		}

		*td = ntd->td_prev = ntd->td_next = ntd;
	}
	threadMutexRelease(); /* Unlock list. */

	return ec;
}


/**
 * @brief This function is used to remove the target descriptor from the thread 
 * descriptor database (e.g. thread list, hashtable, etc) and return the thread descriptor
 * memory to the system.
 *
 * @param[in] td is the thread descriptor pointer.
 */
static void threadTermDesc(threadDesc *td)
{
	/* Sanity check... */
	/* internal function, so *td is always valid */
	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.THREAD", "threadTermDesc for descriptor=0x%x, for thread 0x%x\n", (unsigned int)td, td->td_id);

	/* Remove the thread descriptor from the list. */
	threadMutexAcquire(); /* Lock list. */
	{
		td->td_prev->td_next = td->td_next;
		td->td_next->td_prev = td->td_prev;
		/* Thread has terminated, return its thread local structure. */
		rmf_osal_memFreeP(RMF_OSAL_MEM_THREAD, td);
	}
	threadMutexRelease(); /* Unlock list. */
	
	return;
}


/**
 * @brief This function is used to save a thread's descriptor pointer.
 * The current implemetation uses a doubly-linked list.  If the implementation needs to be 
 * optimized for lots of thread, the code can utilize a faster data structure (e.g. hashtable).
 *
 * Also, if the platform ABI allows for dedication of a register not used
 * by the OS and is preserved correctly by compiler generated code, then
 * this routine will utilize that register to the hold a thread's
 * descriptor pointer.  This allows for fast access to thread local
 * data with the VM and the OSAL implementation.
 *
 * @param[in] td is a pointer to the current thread's descriptor.
 */
static void threadSetDesc(threadDesc *td)
{
	threadDesc *head = NULL;   /* Thread descriptor pointer head. */

	/* Sanity check... */
	if (td == NULL)
	{
		return;
	}

	/* Link thread descriptor on to the head of the list. */
	threadMutexAcquire(); /* Lock list. */
	{
        /* Get virtual head pointer. */
        head = RMF_OSAL_FAKEHEAD(threadDesc, threads[0], td_prev);

		td->td_next = head->td_next;
		td->td_prev = head;
		head->td_next->td_prev = td;
		head->td_next = td;

        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.THREAD",
                  "threadSetDesc: set data=0x%x for id=0x%x\n", (unsigned int)td, (unsigned int)td->td_id);
	}
	threadMutexRelease(); /* Unlock list. */
    
	return;
}


/**
 * @brief This function is used to get the specified thread's descriptor structure pointer.
 * If the thread identifier is zero the current thread's descriptor is returned.
 *
 * @param[in] threadId is the identifier of the target thread.
 *
 * @return NULL if the descriptor can not be located or a pointer
 *         to the thread's descriptor.
 */
threadDesc *threadGetDesc(rmf_osal_ThreadId threadId)
{
	threadDesc *head = NULL;   /* Thread descriptor pointer (head). */
	threadDesc *td = NULL;	   /* Thread descriptor pointer. */
	
	threadMutexAcquire(); /* Lock list. */
	{
		/* Check for current thread. */
		if (threadId == 0)
		{
			/* Get the current thread identifier. */
			if (rmf_osal_threadGetCurrent(&threadId) != RMF_SUCCESS)
			{
				RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.THREAD",
						  "threadGetDesc: threadGetCurrent failed, returning NULL\n");
				threadMutexRelease(); /* Unlock list. */
				return NULL;
			}
		}

		/* Get virtual head pointer. */
		head = RMF_OSAL_FAKEHEAD(threadDesc, threads[0], td_prev);

		/* Sanity check. */
		if (head->td_next == NULL)
		{
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.THREAD", "threadGetDesc: no thread list, returning NULL\n");
			threadMutexRelease(); /* Unlock list. */
			return NULL;
		}

		/* Linear search for the target thread. */
		for (td = head->td_next; td != head; td = td->td_next)
		{
			if (td->td_id == threadId)
			{
				break;
			}
		}
	}
	threadMutexRelease(); /* Unlock list. */

	/* Return what was found/not found. */
	return((td != head) ? td : NULL);
}


/**
 * @brief This function is used to map the specified thread priority to a valid OS value.
 * If the value is coming from the VM, it will be a value from 1-10. If It's an implementation 
 * layer thread, it could be a value already appropriate for the platform.
 *
 * Note, java priorities go from 1-10 and are incremental.  PTV priorities
 * for application threads go from 24-31 decremental (i.e. the lower the value
 * the higher the priority).  For the java mappings 1-10 are mapped to 31-25
 * and 24 is reserved for the initial primordial PTV application thread and
 * potentially any native threads that specifically request it.
 *
 * Note: this function is coded without the use of macros for the priorities
 * for clarity.  The macros defined in os_thread.h are used primarily to
 * support thread priority specification independant of the platform's
 * thread prioritization implementation.
 *
 * @param[in] p is the priority to map
 *
 * @return the mapped priority value
 */
uint32_t threadMapPriority(uint32_t p)
{
	/* Java priority mapping table. */
	static const uint32_t ptvPriorities[10] =
	{
		31, 31, 30, 29, 28, 27, 26, 26, 25, 25
	};

	/* Check for out of range. */
	if (p > 31)
	{
		/* Map non-VM priority outside max. */
		return 31;
	}
	else if (p < 24)
	{
		/* VM mapping? */
		if (p >= 1 && p <= 10)
		{
			/* Map VM priority (1-10) inversely to PTV. */
			return ptvPriorities[p-1];
		}
		else
		{
			/* Map non-VM priority outside min. */
			return 24;
		}
	}
	else
	{
		return p; /* Straight 24-31 mapping. */
	}
}

/**
 * @brief The function rmf_osal_threadAttach() is used to create or release thread 
 * tracking information for a thread which is not created with the rmf_osal_threadCreate API.

 * Most likely this is only utilized for the initial primordial thread of the VM.
 * If the thread identifier pointer is NULL, the call is considered a "detach" call and all resources
 * established during the "attach" phase will be released.
 *
 * @param[in] threadId is a pointer for returning the thread's identifier or
 *          handle. The identifier is of the rmf_osal_ThreadId type.
 *
 * @return The RMF_OSAL error code if the create fails, otherwise RMF_SUCCESS is returned.
 */
rmf_Error rmf_osal_threadAttach(rmf_osal_ThreadId *threadId)
{
	rmf_Error ec;		/* Error condition code. */
	threadDesc *td;		/* Thread descriptor pointer. */

	threadMutexAcquire(); /* Lock list. */
	{
		RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.THREAD", "rmf_osal_threadAttach(%ld)\n", (threadId==0)?0:(unsigned long)*threadId);
		/* Check for need to initialize thread list. */
		if (threads[0] == NULL)
		{
			if ((ec = threadInit()) != RMF_SUCCESS)
			{
				threadMutexRelease(); /* Unlock list. */
				return ec;
			}
		}

		/* If thread Id pointer is not NULL, it's an attach call. */
		if (threadId != NULL)
		{
			/* Check for existing descriptor (i.e. allocated by rmf_osal_threadCreate) */
			if ((td = threadGetDesc(*threadId)) != NULL)
			{
				td->td_refcount++;		/* Bump the attach reference on this thread. */
				*threadId = td->td_id;	/* Return identifier. */
				threadMutexRelease(); /* Unlock list. */
				return RMF_SUCCESS;
			}
			else
			{
				/*
				 * Thread was create externally (i.e. not with rmf_osal_threadCreate),
				 * need to setup resource to make it OSAL compatible.
				 */
				/* Allocate a new thread descriptor. */
				if ((ec = threadAllocDesc(&td, NULL)) != RMF_SUCCESS)
				{
					threadMutexRelease(); /* Unlock list. */
					return ec;
				}

				/* Get native thread identifier. */
				if ((ec = rmf_osal_threadGetCurrent(threadId)) != RMF_SUCCESS)
				{
					threadTermDesc(td);	/* Release descriptor. */
					threadMutexRelease(); /* Unlock list. */
					return ec;
				}

				/* Set the thread's descriptor storage structure pointer. */
				td->td_id = *threadId;	/* Return identifier. */
				threadSetDesc(td);
			}
		}
		else
		{
			/* Thread detach call... */

			/* Get the current thread's descriptor. */
			if ((td = threadGetDesc(0)) == NULL)
			{
				threadMutexRelease(); /* Unlock list. */
				return RMF_SUCCESS;	/* Can't find descriptor, do nothing. */
			}

			/*
			 * If it's the last detach, release resources.
			 * Note, normally this will only occur for threads not
			 * originally created with rmf_osal_threadCreate, which
			 * potentially may never occur.
			 */
			td->td_refcount--;
			if (td->td_refcount == 0)
			{
				RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.THREAD", "rmf_osal_threadAttach() TermDesc(0x%x) as refcount = 0\n", (unsigned int)td);
				/* Terminate descriptor use. */
				threadTermDesc(td);
			}
		}
	}
	threadMutexRelease(); /* Unlock list. */
	return RMF_SUCCESS;
}

/**
 * @brief This function is used to set the priority of the specified thread. If the target OS does not support
 * modification of a thread's priority after creation, this function shall do nothing.
 *
 * @param[in] threadId is the identifier of the target thread.
 * @param[in] priority is the new priority level.
 *
 * @return The RMF_OSAL error code if set priority fails, otherwise RMF_SUCCESS is returned.
 */
rmf_Error rmf_osal_threadSetPriority(rmf_osal_ThreadId threadId, uint32_t priority)
{
	threadMutexAcquire(); /* Lock list. */
	{
		struct sched_param param;
		threadDesc *td = threadGetDesc(threadId);

		if (td == NULL)
		{
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.THREAD", "rmf_osal_threadSetPriority threadGetDesc returned NULL");
			threadMutexRelease(); /* Unlock list. */
			return RMF_OSAL_EINVAL;
		}

		/* set priority to sched_param */
		param.sched_priority = priority;

		/* update thread's priority */
		pthread_attr_setschedparam(&td->td_attr, &param);
	}
	threadMutexRelease(); /* Unlock list. */

	return RMF_SUCCESS;
}

/**
 * @brief The API is used to set the target thread's name.
 * If the thread identifier parameter is zero or matches the identifier
 * of the current thread, then the calling thread's name is set.
 *
 * @param[in] threadId is the identifier of the target thread.
 * @param[in] threadName is string representing the thread's name.
 * @return The RMF_OSAL error code if the rename fails, otherwise RMF_SUCCESS is returned.
 */
rmf_Error rmf_osal_threadSetName(rmf_osal_ThreadId threadId, const char* threadName)
{
	threadDesc *td = NULL;		  /* Thread descriptor pointer. */

	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.THREAD", "rmf_osal_threadSetName:threadId=%d\n", (int)threadId);

	/* if new name is not defined, don't change current name */
	if (threadName == 0)
	{
		return RMF_SUCCESS;
	}

	//RDK_LOG(RDK_LOG_INFO, "LOG.RDK.THREAD", "rmf_osal_threadSetName: strlen(threadName) = %d, threadName:'%s'\n", strlen(threadName), threadName);

	threadMutexAcquire(); /* Lock list. */
	{
		/* Get the thread's descriptor. */
		if ((td = threadGetDesc(threadId)) == NULL)
		{
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.THREAD", "rmf_osal_threadSetName threadGetDesc returned NULL");
			threadMutexRelease(); /* Unlock list. */
			return RMF_OSAL_EINVAL;
		}

		strncpy(&(td->td_name[0]), threadName, RMF_OSAL_MAX_THREAD_NAME-1);
	}
	threadMutexRelease(); /* Unlock list. */
	
	rmf_osal_ThreadRegSet(RMF_OSAL_RES_TYPE_THREAD, 0, threadName, 1024*1024, 0);
	return RMF_SUCCESS;
} 

