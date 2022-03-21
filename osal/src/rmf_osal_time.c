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
 * @file rmf_osal_time.c
 */

#include <sys/time.h>       /* gettimeofday(2) */
#include <sys/times.h>      /* times(2) */
#include <stdio.h>          /* stderr */
#include <unistd.h>         /* sysconf(3) */
#include <string.h>         /* memcpy(3) */
#include <errno.h>

#include <rmf_osal_types.h>
#include <rmf_osal_error.h>
#include <rmf_osal_time.h>
#include "rdk_debug.h"

/***
 *** File-Private Function Prototypes
 ***/
static long getTicksPerSec(void);

static int xcc_gettimeofday(struct timeval *tv, struct timezone *tz);

/* ++ RMP - Fast getttimeofday() implementation */
#define USE_CC_BETTIMEOFDAY
#ifdef USE_CC_GETTIMEOFDAY
#if defined (XONE_STB)
#define TPS ((unsigned long long) 1200000000)    // Needs to be defined by platform
#define TPU ((unsigned long long) 1200)    // Needs to be defined by platform
#else
#define TPS ((unsigned long long) 800000000)    // Needs to be defined by platform
#define TPU ((unsigned long long) 800)    // Needs to be defined by platform
#endif //XONE_STB
#define reset_interval (TPS * 1)                // Needs to be configurable... //Seconds between each resynchronization with gettimeofday()

unsigned short gettimeofday_reset_required = 0;
#endif //USE_CC_GETTIMEOFDAY

#ifdef USE_CC_GETTIMEOFDAY
static inline unsigned long long xcc_rdtsc() 
{
    uint32_t lo, hi;
    __asm__ __volatile__ (      // serialize
            "xorl %%eax,%%eax \n        cpuid"
            ::: "%rax", "%rbx", "%rcx", "%rdx");
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return (unsigned long long)hi << 32 | lo;
}
#endif //USE_CC_GETTIMEOFDAY


static int xcc_gettimeofday(struct timeval *tv, struct timezone *tz)
{
#ifdef USE_CC_GETTIMEOFDAY
    static unsigned long long tsc_offset = 0;
    static unsigned long long last_tsc = 0;
    static struct timezone    last_tz;

    unsigned long long tsc;
    unsigned long long return_time;
    unsigned long long delta = 0;

    if (tv != NULL)
    {
        tsc = xcc_rdtsc();

        delta = ( (unsigned long long) ((tsc + tsc_offset) - last_tsc));


        if ( (delta > reset_interval) || (gettimeofday_reset_required ==1) )
        {
            //                      printf("JAVA !!!!!!!!!!!!!!! RESYNC !!!!!!!!!!!!!!!!!! why: %d\n", gettimeofday_reset_required);
            gettimeofday_reset_required = 0;
            unsigned long long gettimeofday_in_tsc = 0;
            struct timeval tv_comp;

            gettimeofday(&tv_comp, &last_tz);
            gettimeofday_in_tsc += ((unsigned long long) (tv_comp.tv_sec  * TPS));
            gettimeofday_in_tsc += ((unsigned long long) (tv_comp.tv_usec * TPU));

            tsc_offset = gettimeofday_in_tsc - tsc;

            last_tsc = gettimeofday_in_tsc;
        }

        return_time = tsc + tsc_offset;

        tv->tv_sec =  (unsigned long long) (return_time / TPS);
        tv->tv_usec = (unsigned long long) ( ( (return_time / TPU) ) % 1000000 );
    }
    if (tz != NULL)
    {
        tz->tz_minuteswest = last_tz.tz_minuteswest;
        tz->tz_dsttime = last_tz.tz_dsttime;
    }

    return 0; // Some sort of error provisioning
#else
    return gettimeofday(tv, tz);
#endif //USE_CC_GETTIMEOFDAY
}

/**
 * @addtogroup OSAL_TIME_API
 * @{
 */

/**
 * @brief This function is used to get the current time, which is
 * a value representing seconds since midnight 1970.
 *
 * @param[in] ptime is a pointer for returning the current time value.
 * @return Returns relevant RMF_OSAL error code on failure, otherwise RMF_SUCCESS is returned.
 */
rmf_Error  rmf_osal_timeGet(rmf_osal_Time *ptime)
{
    if (NULL == ptime)
    {
        return RMF_OSAL_EINVAL;
    }
    *ptime = time(NULL);
    return RMF_SUCCESS;
}

/**
 * @brief This function is used to get a representation of time in milliseconds with
 * no worse than 10ms precision. 
 *
 * The value returned by this function is guaranteed to increase monotonically over
 * the duration of time the system is up. If SITP.SI.STT.ENABLED is set to TRUE in the
 * rmfconfig.ini, the implementation of this API need not return any real representation of UTC
 * time. If SITP.SI.STT.ENABLED is set to FALSE, this API MUST always return
 * an accurate representation of cable-network UTC time (milliseconds since
 * midnight 1970) and MUST NOT be subject to large time corrections.
 *
 * @param[in] ptime is a pointer for returning the 64-bit current time value.
 * @return Returns relevant RMF_OSAL error code on failure, otherwise RMF_SUCCESS is returned.
 */
rmf_Error  rmf_osal_timeGetMillis(rmf_osal_TimeMillis *ptime)
{
    int ret;
    struct timeval tv;
    struct timezone tz;
    rmf_osal_TimeMillis msec;

    if (NULL == ptime)
    {
        return RMF_OSAL_EINVAL;
    }

    ret = xcc_gettimeofday(&tv,&tz);
    if (0 != ret)
    {
        (void) fprintf(stderr, "rmf_osal_timeGetMillis(): gettimeofday(): %d.\n",
                ret);
        return RMF_OSAL_EINVAL;
    }

    msec = tv.tv_sec;
    msec *= 1000;
    msec += (tv.tv_usec / 1000); 
    *ptime = msec;
    return RMF_SUCCESS;
}

/**
 * @brief This function is used to get time in rmf_osal_TimeVal format.
 *
 * @param[out] tv is a pointer to get current time.
 * @return Returns relevant RMF_OSAL error code on failure, otherwise RMF_SUCCESS is returned.
 */
rmf_Error rmf_osal_getTimeOfDay(rmf_osal_TimeVal *tv)
{
    struct timezone tz;
    if (0 !=  xcc_gettimeofday((struct timeval *)tv, &tz))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.TIME", "xcc_gettimeofday() failed\n");
        return RMF_OSAL_EINVAL;
    }
    return RMF_SUCCESS;
}

/**
 * @brief This function is used to set the time provided in rmf_osal_TimeVal format.
 *
 * @param[in] tv pointer containing time to set.
 * @return Returns relevant RMF_OSAL error code on failure, otherwise RMF_SUCCESS is returned.
 */
rmf_Error rmf_osal_setTimeOfDay(rmf_osal_TimeVal *tv)
{
#ifdef USE_CC_GETTIMEOFDAY
    gettimeofday_reset_required = 1;
#endif
    if(0 != settimeofday ((struct timeval *)tv, NULL))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.TIME", "settimeofday() failed : %s\n", strerror(errno));
        return RMF_OSAL_EINVAL;
    }
    return RMF_SUCCESS;
}

/**
 * @brief This function is used to convert a time value to a local date value.
 *
 * @param[in] time is the time value to convert.
 * @param[in] local is a pointer to a time structure to populate with the local
 *          date information as converted from the time value.
 * @return Returns relevant RMF_OSAL error code on failure, otherwise RMF_SUCCESS is returned.
 */
rmf_Error  rmf_osal_timeToDate(rmf_osal_Time time, rmf_osal_TimeTm *local)
{
    rmf_osal_TimeTm *mylocal_p;
    if (NULL == local)
    {
        return RMF_OSAL_EINVAL;
    }
    mylocal_p = localtime(&time);
    if (NULL == mylocal_p)
    {
        return RMF_OSAL_EINVAL;
    }
    (void) memcpy(local, mylocal_p, sizeof(rmf_osal_TimeTm));
    return RMF_SUCCESS;
}

/**
 * @brief This function is used to get the current value of the system clock ticks. 
 * If the target platform does not contain support for this call (-1) will be returned.  
 *
 * @return Returns the total number of system clock ticks or (-1) if
 *          the value can not be provided.
 */
rmf_osal_TimeClock  rmf_osal_timeSystemClock(void)
{
#ifdef RMF_OSAL_FEATURE_PERFORMANCEMARKERS_EXT
    /* Standard code deviation: PerformanceMarkers extension: start */
    rmf_osal_TimeClock clockTime;

#ifdef HIGH_RES_CLOCK
    /*
     * This high resolution clock offers less than a second precision but it is
     * not available on older Linux kernels.
     */
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC_RAW, &tp);
    clockTime = tp.tv_sec * getTicksPerSec();
    clockTime += tp.tv_nsec / (NS_PER_SEC / getTicksPerSec());
#else
    struct sysinfo info;
    sysinfo(&info);
    clockTime = info.uptime * getTicksPerSec();
#endif

    return clockTime;
    /* Standard code deviation: PerformanceMarkers extension: end */
#else
    clock_t elapsed_ticks = times(0); // NOTE: POSIX may require a non-NULL arg
    if (-1 == elapsed_ticks)
    {
        (void) fprintf(stderr, "%s() - returning -1 ...\n",__FUNCTION__);
        return (rmf_osal_TimeClock) -1;
    }
    return (rmf_osal_TimeClock) elapsed_ticks;
#endif
}

/**
 * @brief This function is used to get the best approximation of processor time used by 
 * associated process/thread.  This value should represent the number of system
 * clock ticks utilized by the thread.
 * <p>
 * If the target platform does  not contain support for this call (-1) will be returned.  
 *
 * @return Returns the total number of clock ticks consumed by the caller or (-1) if
 *          the value can not be provided.
 */
rmf_osal_TimeClock  rmf_osal_timeClock(void)
{
    return clock() / CLOCKS_PER_SEC * getTicksPerSec();
}

/**
 * @brief This function is used to get the number of clock ticks per second used in the system for time keeping.
 *
 * @return Returns the number of system clock ticks per second.
 */
rmf_osal_TimeClock  rmf_osal_timeClockTicks(void)
{
    return (rmf_osal_TimeClock) getTicksPerSec();
}

/**
 * @brief This function is used to convert rmf_osal_TimeClock value to a uint32_t millisecond value.
 *
 * @param[in] clk is the clock tick value to convert.
 * @return Returns the milliseconds representation.
 */
uint32_t  rmf_osal_timeClockToMillis(rmf_osal_TimeClock clk)
{
    /* N.B. in the following expression, order DOES matter! */
    return (uint32_t)(((uint64_t) clk * 1000) / getTicksPerSec());
}

/**
 * @brief This function is used to convert rmf_osal_TimeClock value to a rmf_osal_Time value.
 *
 * @param[in] clk is the clock tick value to convert.
 * @return Returns the converted time value.
 */
rmf_osal_Time   rmf_osal_timeClockToTime(rmf_osal_TimeClock clk)
{
    return (rmf_osal_Time)(clk / getTicksPerSec());
}

/**
 * @brief This function is used to convert time value expressed in milliseconds to a system clock tick value.
 *
 * @param[in] ms is time value in milliseconds to convert.
 * @return Returns the converted time value expressed in system clock ticks.
 */
rmf_osal_TimeClock  rmf_osal_timeMillisToClock(uint32_t ms)
{
    return ((ms * getTicksPerSec()) / 1000);
}

/**
 * @brief This function is used to convert rmf_osal_Time
 * time value to a system clock tick value.
 * 
 * @param[in] time is the time value to convert.
 * @return Returns the converted time value expressed in system clock ticks.
 */
rmf_osal_TimeClock  rmf_osal_timeTimeToClock(rmf_osal_Time time)
{
	return (clock_t)(time * getTicksPerSec());
}

/**
 * @brief This function is used to convert rmf_osal_TimeTm time
 * value to a rmf_osal_Time value.
 *
 * @param[in] _tm is a pointer to the time structure containing the value to convert.
 * @return Returns the converted time value.
 */
rmf_osal_Time   rmf_osal_timeTmToTime(rmf_osal_TimeTm *_tm)
{
    if (NULL == _tm)
    {
        return RMF_OSAL_EINVAL;
    }
    return mktime(_tm);
}

/** @} */ //end of doxygen tag OSAL_TIME_API

/***
 *** File-Private Functions
 ***/

/*
 * @brief If this value cannot change, a static variable could be used to "cache" it.
 * @return None
 */
static long getTicksPerSec()
{
    long ticksPerSec = sysconf(_SC_CLK_TCK);
    if (-1 == ticksPerSec)
    {
        (void) fprintf(stderr,
                       "rmf_osal_timeClockTicks() sysconf(_SC_CLK_TCK): %ld\n",
                       ticksPerSec);
    }
    return ticksPerSec;
}
