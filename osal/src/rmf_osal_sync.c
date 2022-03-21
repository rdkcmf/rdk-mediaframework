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
 * @file rmf_osal_sync.c
 */

#include <sys/types.h> /* getpid(2) 			*/
#include <unistd.h>    /* getpid(2) 			*/
#include <errno.h>     /* EBUSY, EINVAL, EPERM 	*/
#include <stdio.h>     /* stderr 				*/
#include <sys/time.h>  /* stderr 				*/

#include <rmf_osal_error.h>
#include "rdk_debug.h"
#include <rmf_osal_mem.h>
#include "rmf_osal_sync.h"
#include "rmf_osal_sync_priv.h"
#include "rmf_osal_thread.h"

#if USE_SYSRES_MLT
#include "mlt_malloc.h"
#endif

#define RMF_OSAL_MEM_DEFAULT RMF_OSAL_MEM_MUTEX



#ifdef DEBUG_MUTEX_ALLOC
#undef rmf_osal_mutexNew
rmf_Error rmf_osal_mutexNew(rmf_osal_Mutex *mutex);

struct mutexinfo_s
{
    rmf_osal_Mutex mutex;
    const char* file;
    int line;
    int printed;
    mutexinfo_s *next;
};

mutexinfo_s * g_mutexinfo_list = NULL;
rmf_osal_Mutex g_dbg_mutex = 0;
#endif

/**
 * @brief This function is used to create a new mutex object. 
 * The mutex is created to allow for nested calling by the same thread (recursive mutex).
 *
 * @param[out] mutex is a pointer for returning the identifier of the new mutex.
 * @return The RMF_OSAL error code if the create fails, otherwise RMF_SUCCESS is returned.
 */
rmf_Error rmf_osal_mutexNew(rmf_osal_Mutex *mutex)
{
    //Do not use static attr as it introduces race condition.
    pthread_mutexattr_t attr;
    auto int ec = 0;

    if (NULL == mutex)
    {
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.MUTEX",
                "rmf_osal_mutexNew() - mutex NULL returning\n");
        return RMF_OSAL_EINVAL;
    }

    if ((ec = rmf_osal_memAllocP(RMF_OSAL_MEM_DEFAULT, sizeof(pthread_mutex_t),
            (void**)mutex)) != RMF_SUCCESS)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.COND",
                "rmf_osal_mutexNew() - failed, no memory.\n");
        return ec;
    }

    pthread_mutexattr_init( &attr );
    pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_RECURSIVE_NP);
	pthread_mutex_init( (pthread_mutex_t *) *mutex, &attr );

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.MUTEX",
            "rmf_osal_mutexNew() - succeeded, mutex: %x.\n", *mutex);
    return RMF_SUCCESS;
}


#ifdef DEBUG_MUTEX_ALLOC

void rmf_osal_mutex_dump()
{

    mutexinfo_s * info = NULL;
    printf("\n\n\n===========RMF OSAL MUTEX ALLOC INFO================\n");

    rmf_osal_mutexAcquire(g_dbg_mutex);
    info = g_mutexinfo_list;
    while (info )
    {
        if (info->printed == 0 )
        {
            printf("Mutex = 0x%x, file = %s, line = %d \n", info->mutex, info->file, info->line);
            info->printed = 1;
        }
        info = info->next;
    }
     rmf_osal_mutexRelease(g_dbg_mutex);
     printf("\n\n\n===========RMF OSAL MUTEX ALLOC INFO END ================\n");
}


rmf_Error rmf_osal_mutexNew_track(rmf_osal_Mutex *mutex, const char* file, int line)
{
    mutexinfo_s* info;
    rmf_Error ret;
    if ( 0 == g_dbg_mutex )
    {
        rmf_osal_mutexNew(&g_dbg_mutex);
    }
    ret = rmf_osal_mutexNew(mutex);
    if (RMF_SUCCESS == ret )
    {
        rmf_osal_memAllocP(RMF_OSAL_MEM_DEFAULT, sizeof(mutexinfo_s), (void**)&info);
        info->mutex = *mutex;
        info->file = file;
        info->line = line;
        info->printed = 0;
        rmf_osal_mutexAcquire(g_dbg_mutex);
        info->next =  g_mutexinfo_list;
        g_mutexinfo_list = info;
        rmf_osal_mutexRelease(g_dbg_mutex);
    }
    return ret;
}

#endif

/**
 * @brief This function is used to destroy a previously created mutex.
 *
 * @param[in] mutex is the identifier of the mutex to destroy.
 * @return The RMF_OSAL error code if mutex deletion fails, otherwise RMF_SUCCESS is returned.
 */
rmf_Error rmf_osal_mutexDelete(rmf_osal_Mutex mutex)
{
    int pRet;

	if (NULL == mutex)
	{
		RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.MUTEX", "rmf_osal_mutexDelete() - mutex NULL returning\n");
		return RMF_OSAL_EINVAL;
	}

	pRet = pthread_mutex_destroy(mutex);
    if (0 != pRet)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.MUTEX",
                "rmf_osal_mutexDelete() - failed: %d\n", pRet);
        return RMF_OSAL_EINVAL;
    }
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.MUTEX",
            "rmf_osal_mutexDelete() - succeeded.\n");

#ifdef DEBUG_MUTEX_ALLOC
    mutexinfo_s * info = NULL;
    mutexinfo_s * previnfo = NULL; 
    rmf_osal_mutexAcquire(g_dbg_mutex);
    info = g_mutexinfo_list;
    while (info )
    {
        if (info->mutex == mutex )
        {
            if (previnfo )
            {
                previnfo->next = info->next;
            }
            else
            {
                g_mutexinfo_list = info->next;
            }
            rmf_osal_memFreeP(RMF_OSAL_MEM_DEFAULT, info);
            break;
        }
        previnfo = info;
        info = info->next;
    }
     rmf_osal_mutexRelease(g_dbg_mutex);
#endif

    rmf_osal_memFreeP(RMF_OSAL_MEM_DEFAULT, mutex);

    return RMF_SUCCESS;
}

/**
 * @brief This function is used to acquire ownership of the target mutex.
 *
 * If the mutex is already owned by another thread, the calling thread will 
 * be suspended within a priority based queue until the mutex is free for this 
 * thread's acquisition.
 *
 * @param[in] mutex is the identifier of the mutex to acquire.
 * @return Returns relevant RMF_OSAL error code on failure, otherwise RMF_SUCCESS is returned.
 */
rmf_Error  rmf_osal_mutexAcquire(rmf_osal_Mutex mutex)
{

    if (mutex == NULL)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.MUTEX",
                "rmf_osal_mutexAcquire() - mutex null invalid value\n");
        return RMF_OSAL_EINVAL;
    }

    #ifdef RMF_OSAL_FEATURE_THREAD_SUSPEND
    ThreadDesc* td = NULL;
    rmf_osal_threadGetCurrent(&td);

    while (1)
    {
        int pRet;
        if (td != NULL)
        td->td_blocked = TRUE;
        if ((pRet = pthread_mutex_lock(mutex)) != 0)
        {
            if (td != NULL)
            td->td_blocked = FALSE;
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.MUTEX",
                    "rmf_osal_mutexAcquire() - failed: %d\n", pRet);
            return RMF_OSAL_EINVAL;
        }
        if (td != NULL)
        {
            td->td_blocked = FALSE;
            if (td->td_suspended)
            {
                pthread_mutex_unlock(mutex);
                suspendMe();
            }
            else
            break;
        }
        else
        break;
    }
    #else
    int pRet ;
    pRet = pthread_mutex_lock(mutex);

    if (0 != pRet)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.MUTEX", "rmf_osal_mutexAcquire() - failed: %d\n", pRet);
        return RMF_OSAL_EINVAL;
    }
    #endif

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.MUTEX",
            "rmf_osal_mutexAcquire() - succeeded, mutex: %x (PID %d TID %d).\n",
            mutex, getpid(), pthread_self());
    return RMF_SUCCESS;
}

/**
 * @brief This function is used to attempt to acquire ownership of the target mutex
 * without blocking. 
 *
 * If the mutex is busy an error will be returned to indicate failure to acquire the mutex.
 *
 * @param[in] mutex is the identifier of the mutex to acquire.
 * @return Returns relevant RMF_OSAL error code on failure, otherwise RMF_SUCCESS is returned.
 */
rmf_Error  rmf_osal_mutexAcquireTry(rmf_osal_Mutex mutex)
{
	int pRet;

	if (mutex == NULL)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.MUTEX", "rmf_osal_mutexAcquireTry() - mutex null invalid value\n");
		return RMF_OSAL_EINVAL;
	}

	pRet = pthread_mutex_trylock(mutex);

    if (EBUSY == pRet)
    {
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.MUTEX",
                "rmf_osal_mutexAcquireTry() - failed, mutex busy.\n");
        return RMF_OSAL_EBUSY;
    }
    else if (EINVAL == pRet)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.MUTEX",
                "rmf_osal_mutexAcquireTry() - failed, invalid mutex.\n");
        return RMF_OSAL_EINVAL;
    }
    else if (0 != pRet)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.MUTEX",
                "rmf_osal_mutexAcquireTry() - failed: %d\n", pRet);
        return RMF_OSAL_EINVAL;
    }
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.MUTEX",
            "rmf_osal_mutexAcquireTry() - succeeded, mutex: %x.\n", mutex);
    return RMF_SUCCESS;
}

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
rmf_Error rmf_osal_mutexAcquireTimed(rmf_osal_Mutex mutex, int32_t timeout_usec)
{
	int pRet;
	int64_t sec, nsec;
	struct timespec time_spec;
	struct timeval time_val;

	if (mutex == NULL)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.MUTEX", "rmf_osal_mutexAcquireTimed() - mutex null invalid value\n");
		return RMF_OSAL_EINVAL;
	}
	if( -1 == timeout_usec )
	{
		pRet = pthread_mutex_lock(mutex);
	}
	else
	{
		gettimeofday (&time_val, NULL);
		
		// perform calculation in 64 bit arithmetic to avoid overflow
		nsec = ((int64_t)time_val.tv_usec + timeout_usec) * 1000LL;
		sec = nsec / 1000000000LL;
		nsec = nsec % 1000000000LL;
		
		time_spec.tv_sec = time_val.tv_sec + sec;
		time_spec.tv_nsec = nsec;

		pRet = pthread_mutex_timedlock(mutex, &time_spec);
	}
	
    if (EBUSY == pRet)
    {
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.MUTEX",
                "rmf_osal_mutexAcquireTry() - failed, mutex busy.\n");
        return RMF_OSAL_EBUSY;
    }	
    else if(ETIMEDOUT == pRet)
    {
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.MUTEX",
                "rmf_osal_mutexAcquireTimed() - timedout\n");
        return RMF_OSAL_ETIMEOUT;
    }
    else if (EINVAL == pRet)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.MUTEX",
                "rmf_osal_mutexAcquireTimed() - failed, invalid mutex.\n");
        return RMF_OSAL_EINVAL;
    }
    else if (0 != pRet)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.MUTEX",
                "rmf_osal_mutexAcquireTimed() - failed: %d\n", pRet);
        return RMF_OSAL_EINVAL;
    }
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.MUTEX",
            "rmf_osal_mutexAcquireTimed() - succeeded, mutex: %x.\n", mutex);
    return RMF_SUCCESS;

}

rmf_Error  rmf_osal_mutexRelease(rmf_osal_Mutex mutex)
{
    int pRet ;

    if (mutex == NULL)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.MUTEX",
                "mutex is NULL return invalid params\n");
        return RMF_OSAL_EINVAL;
    }

    pRet = pthread_mutex_unlock(mutex);
    if (EINVAL == pRet)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.MUTEX",
                "rmf_osal_mutexRelease() - failed, invalid mutex.\n");
        return RMF_OSAL_EINVAL;
    }
    else if (EPERM == pRet)
    {
        RDK_LOG(
                RDK_LOG_DEBUG,
                "LOG.RDK.MUTEX",
                "rmf_osal_mutexRelease() - failed, caller (PID %d TID %lu) does not own mutex.\n",
                getpid(), pthread_self());
        return RMF_OSAL_EINVAL;
    }
    else if (0 != pRet)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.MUTEX",
                "rmf_osal_mutexRelease() - failed: %d.\n", pRet);
        return RMF_OSAL_EINVAL;
    }
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.MUTEX",
            "rmf_osal_mutexRelease() - succeeded, mutex: %x.\n", mutex);
    return RMF_SUCCESS;
}

#undef RMF_OSAL_MEM_DEFAULT
#define RMF_OSAL_MEM_DEFAULT RMF_OSAL_MEM_COND

/**
 * @brief This function is used to create a new condition synchronization object.
 *
 * @param[in] autoReset is a boolean variable that indicated whether the new 
 *          condition is an "autoreset" condition object, in which case the 
 *          condition object automatically resets to the unset (FALSE) state 
 *          when a thread acquires it using the rmf_osal_condGet() function.
 *          If autoReset is TRUE, the condition object is automatically reset to 
 *          the unset (FALSE) condition.
 * @param[in] initialState is a boolean variable that specifies the initial state of 
 *          the condition object. The condition object is in the set state if 
 *          initialState is TRUE and in the unset state if initialState is FALSE.
 * @param[in] condition is a pointer for returning the new condition identifier used for
 *          subsequent condition variable operations.
 * @return The RMF_OSAL error code if the create fails, otherwise RMF_SUCCESS is returned.
 */
rmf_Error rmf_osal_condNew(rmf_osal_Bool autoReset, rmf_osal_Bool initialState,
                         rmf_osal_Cond *condition)
{
    rmf_Error ec;
    struct os_Cond_s *cond;
    int pRet;      

    if (NULL == condition)
    {
        return RMF_OSAL_EINVAL;
    }

    if ((ec = rmf_osal_memAllocP(RMF_OSAL_MEM_DEFAULT, sizeof(struct os_Cond_s),
            (void**) &cond)) != RMF_SUCCESS)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.COND",
                "rmf_osal_condNew() - failed, no memory.\n");
        return ec;
    }

    cond->cd_id        = OS_COND_ID;
    cond->cd_autoReset = autoReset;
    cond->cd_state     = initialState;
    cond->cd_count     = 0;

    ec = rmf_osal_threadGetCurrent(&cond->cd_owner);
    if (RMF_SUCCESS != ec)
    {
        rmf_osal_memFreeP( RMF_OSAL_MEM_DEFAULT, cond );
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.COND",
                "rmf_osal_condNew() - failed condition: %p. (ec: %d)\n", cond, ec);
        return RMF_OSAL_EINVAL;
    }

    rmf_osal_mutexNew(&(cond->cd_mutex));

    // no LINUX support for pthread_condattr_t
    pRet= pthread_cond_init(&(cond->cd_cond), NULL);
    if (0 != pRet)
    {
        rmf_osal_memFreeP( RMF_OSAL_MEM_DEFAULT, cond );
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.COND",
                "rmf_osal_condNew() - failed condition: %p. (pRet: %d)\n", cond,
                pRet);
        return RMF_OSAL_EINVAL;
    }

    *condition = (rmf_osal_Cond) cond;
    return RMF_SUCCESS;
}

/**
 * @brief This function is used to destroy a condition object.
 *
 * There is always the potential for a race condition with any thread that might be in the process
 * of calling rmf_osal_condSet or one of the other condition APIs that utilizes the internal lock. 
 * Hence, after having marked the condition variable as no longer valid we only need to worry about
 * any threads that are already in one of the rmf_osal_cond APIs (having verified a valid condition object).
 * To overcome this situation, any threads will be activated until there are no owners.
 *
 * @param[in] condition is the identifier of the target condition to destroy.
 * @return Returns relevant RMF_OSAL error code on failure, otherwise RMF_SUCCESS 
 *         is returned.
 */
rmf_Error rmf_osal_condDelete(rmf_osal_Cond condition)
{
    int pRet;

    struct os_Cond_s *cond = (struct os_Cond_s *)condition;

    if ((NULL == cond) || (OS_COND_ID != cond->cd_id))
    {
        return RMF_OSAL_EINVAL;
    }

    cond->cd_id = 0;        /* Flag condition no longer functional. */

    /*
    * There is always the potential for a race condition with any thread
    * that might be in the process of calling rmf_osal_condSet or one of the
    * other condition APIs that utilizes the internal lock.  Hence, after
    * having marked the condition variable as no longer valid we only need
    * to worry about any threads that are already in one of the rmf_osal_cond
    * APIs (having verified a valid condition object).  To overcome this
    * situation, any threads will be activated until there are no owners.
    */
    if (cond->cd_owner != 0)
    {
        cond->cd_owner = 0;
    }
    while ( cond->cd_count != 0)
    {
        cond->cd_state = TRUE;
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.COND", "calling pthread_cond_signal\n");
        pRet = pthread_cond_signal(&(cond->cd_cond));
        if (0 != pRet)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.COND",
                      "pthread_cond_broadcast(%x: failed: %d\n", (unsigned int)cond, pRet);
        }
        usleep(10);
    }
    if (rmf_osal_mutexAcquire(cond->cd_mutex) == 0)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.COND", "calling pthread_cond_destroy\n");
        pRet = pthread_cond_destroy(&(cond->cd_cond));
        if (0 != pRet)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.COND",
                    "rmf_osal_condDelete(%p: failed: %d\n", cond, pRet);
            rmf_osal_mutexRelease(cond->cd_mutex);
            return RMF_OSAL_EINVAL;
        }

        rmf_osal_mutexRelease(cond->cd_mutex);

    }

    rmf_osal_mutexDelete(cond->cd_mutex);

    rmf_osal_memFreeP(RMF_OSAL_MEM_DEFAULT, cond);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.COND", "rmf_osal_condDelete(%p: succeeded.\n",
            cond);
    return RMF_SUCCESS;
}

/**
 * @brief This function is used to get exclusive access of the specified
 * condition object. 
 *
 * If the condition object is in the FALSE state at the time of
 * the call the calling thread is suspended until the condition is set to the
 * TRUE state.  If the calling thread already has exclusive access to the
 * condition this function does nothing.
 *
 * @param[in] condition is the identifier of the target condition object to acquire.
 * @return Returns relevant RMF_OSAL error code on failure, otherwise RMF_SUCCESS 
 *         is returned.
 */
rmf_Error  rmf_osal_condGet(rmf_osal_Cond condition)
{

    struct os_Cond_s *cond = (struct os_Cond_s *)condition;
    if ((NULL == cond) || (cond->cd_id != OS_COND_ID))
    {
        return RMF_OSAL_EINVAL;
    }

#ifdef RMF_OSAL_FEATURE_THREAD_SUSPEND
    ThreadDesc* td = NULL;
    rmf_osal_threadGetCurrent(&td);
#endif

    if (rmf_osal_mutexAcquire(cond->cd_mutex) == 0)
    {
        cond->cd_count++;
        // If condition state is TRUE, it can go ahead
        while(cond->cd_state == FALSE)
        {
#ifdef RMF_OSAL_FEATURE_THREAD_SUSPEND
            if (td != NULL)
            td->td_waiting = TRUE;
#endif
            int retval = 0;
            retval = pthread_cond_wait(&cond->cd_cond, cond->cd_mutex);
            if(retval == EINVAL || retval == EPERM )
            {
                rmf_osal_mutexRelease(cond->cd_mutex);
                cond->cd_count--;
                return RMF_OSAL_EINVAL;
             }
             RDK_LOG(RDK_LOG_INFO, "LOG.RDK.COND", "pthread_cond_wait returned\n");
#ifdef RMF_OSAL_FEATURE_THREAD_SUSPEND
            if (td != NULL)
            td->td_waiting = FALSE;
#endif
        }
        if (cond->cd_autoReset)
            cond->cd_state = FALSE;
        cond->cd_count--;
        rmf_osal_mutexRelease(cond->cd_mutex);
    }

#ifdef RMF_OSAL_FEATURE_THREAD_SUSPEND
    if (td != NULL && td->td_suspended)
    {
        suspendMe();
    }
#endif

    return RMF_SUCCESS;
}

/**
 * @brief This function is used to attempt to get exclusive access of the specified condition object.
 *
 * If the condition object is in the FALSE state 
 * at the time of the call the calling thread is suspended for a maximum period of
 * time as specified by the time out parameter until the condition is set to the
 * TRUE state.  If the calling thread already has exclusive access to the
 * condition this function does nothing.
 *
 * @param[in] condition is the identifier of the target condition object to acquire.
 * @param[in] timeout is the maximum time in milliseconds to wait for condition object 
 *          to become TRUE.
 * @return Returns relevant RMF_OSAL error code on failure, otherwise RMF_SUCCESS 
 *         is returned.
 */
rmf_Error  rmf_osal_condWaitFor(rmf_osal_Cond condition, uint32_t timeout)
{
    rmf_Error ec = RMF_SUCCESS;

    struct os_Cond_s *cond = (struct os_Cond_s *)condition;
    int retval = 0;
    

    if ((NULL == cond) || (cond->cd_id != OS_COND_ID))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.COND", "rmf_osal_condWaitFor() - failed\n");
        return RMF_OSAL_EINVAL;
    }

#ifdef RMF_OSAL_FEATURE_THREAD_SUSPEND
    ThreadDesc* td = NULL;
    rmf_osal_threadGetCurrent(&td);
#endif
    if (rmf_osal_mutexAcquire(cond->cd_mutex) == 0)
    {
        cond->cd_count++; 
        if (timeout == 0)
        {
            while (cond->cd_state == FALSE)
            {
                // indefinite wait
#ifdef RMF_OSAL_FEATURE_THREAD_SUSPEND
                if (td != NULL)
                td->td_waiting = TRUE;
#endif
                retval = pthread_cond_wait(&(cond->cd_cond),cond->cd_mutex);
#ifdef RMF_OSAL_FEATURE_THREAD_SUSPEND
                if (td != NULL)
                td->td_waiting = FALSE;
#endif
            }
        }
        else
        {
            // Timed wait.  Convert relative time parameter to absolute
            // time
            struct timespec time;
            struct timeval curTime;
            gettimeofday(&curTime, NULL);
            time.tv_nsec = curTime.tv_usec * 1000 + (timeout % 1000) * 1000000;
            time.tv_sec = curTime.tv_sec + (timeout / 1000);
            if (time.tv_nsec > 1000000000)
            {
                time.tv_nsec -= 1000000000;
                time.tv_sec++;
            }

            while (cond->cd_state == FALSE)
            {
#ifdef RMF_OSAL_FEATURE_THREAD_SUSPEND
                if (td != NULL)
                td->td_waiting = TRUE;
#endif
                retval = pthread_cond_timedwait(&(cond->cd_cond),
                        cond->cd_mutex, &time);
#ifdef RMF_OSAL_FEATURE_THREAD_SUSPEND
                if (td != NULL)
                td->td_waiting = FALSE;
#endif
                // Return an error if the condition is still false and we
                // have a non-zero error code
                if (retval != 0 && cond->cd_state == FALSE)
                {
                    ec = (retval == ETIMEDOUT) ? RMF_OSAL_EBUSY : RMF_OSAL_EINVAL;
                    break;
                }
            }
        }
        if (cond->cd_autoReset)
            cond->cd_state = FALSE;
        cond->cd_count--; 
        rmf_osal_mutexRelease(cond->cd_mutex);
    }
	else
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - failed to acquire mutex.  \n", __FUNCTION__); // Temp debug.
		ec = RMF_OSAL_EINVAL;
	}
		

#ifdef RMF_OSAL_FEATURE_THREAD_SUSPEND
    if (td != NULL && td->td_suspended)
    {
        suspendMe();
    }
#endif

    return ec;
}

/**
 * @brief This function is used to set the condition variable to TRUE state and activate
 * the first thread waiting.
 *
 * @param[in] condition is the identifier of the target condition object to set.
 * @return Returns relevant RMF_OSAL error code on failure, otherwise RMF_SUCCESS 
 *         is returned.
 */
rmf_Error  rmf_osal_condSet(rmf_osal_Cond condition)
{
	struct os_Cond_s *cond = (struct os_Cond_s *)condition;
    if ((NULL == cond) || (cond->cd_id != OS_COND_ID))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.COND", "rmf_osal_condSet() - failed\n");
        return RMF_OSAL_EINVAL;
    }

    if (rmf_osal_mutexAcquire(cond->cd_mutex) == 0)
    {
        /* set the condition code and wake up the waiting thread */
        cond->cd_state = TRUE;
        pthread_cond_signal(&(cond->cd_cond));
        rmf_osal_mutexRelease(cond->cd_mutex);
    }
    return RMF_SUCCESS;
}

/**
 * @brief This function is used to set the condition variable to the FALSE state
 * without modifying ownership of the condition variable.
 *
 * @param[in] condition it is the identifier of the target condition object to unset.
 * @return Returns relevant RMF_OSAL error code on failure, otherwise RMF_SUCCESS is returned.
 */
rmf_Error  rmf_osal_condUnset(rmf_osal_Cond condition)
{
	struct os_Cond_s *cond = (struct os_Cond_s *)condition;
    if ((NULL == cond) || (cond->cd_id != OS_COND_ID))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.COND", "rmf_osal_condUnset() - failed\n");
        return RMF_OSAL_EINVAL;
    }
    if (rmf_osal_mutexAcquire(cond->cd_mutex) == 0)
    {
        cond->cd_state = FALSE;
        rmf_osal_mutexRelease(cond->cd_mutex);
    }
    return RMF_SUCCESS;
}

