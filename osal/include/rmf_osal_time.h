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
 * @file rmf_osal_time.h
 * @brief The header file contains prototypes for Date and Time.
 */

/**
 * @defgroup OSAL_TIME OSAL Date and Time
 *
 * RMF OSAL time module provides support for getting current time in various formats,
 * setting time and conversion between different time formats.
 * This module is used to get and set the system time which is a value representing seconds since midnight 1970.
 * It is also dealing with the best approximation of processor time used by associated process/thread.
 * This value should represent the number of system clock ticks utilized by the thread.
 *
 * @ingroup OSAL
 *
 * @defgroup OSAL_TIME_API OSAL Date and Time API list
 * RMF OSAL Time module defines interfaces for getting time in different formats
 * @ingroup OSAL_TIME
 *
 * @addtogroup OSAL_TIME_TYPES OSAL Date and Time Data Types
 * Describe the details about Data Structure used by OSAL Date and Time.
 * @ingroup OSAL_TIME
 */

#if !defined(_RMF_OSAL_TIME_H)
#define _RMF_OSAL_TIME_H

#include <rmf_osal_types.h>		/* Resolve basic type references. */
#include "rmf_error.h"
#include <sys/time.h>
#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @addtogroup OSAL_TIME_TYPES
 * @{
 */

/***
 * Time/Date type definitions:
 */

typedef clock_t rmf_osal_TimeClock;
typedef time_t rmf_osal_Time;
typedef struct tm  rmf_osal_TimeTm;
typedef struct timeval  rmf_osal_TimeVal;
typedef long long rmf_osal_TimeMillis;

/** @} */ //end of doxygen tag OSAL_TIME_TYPES

/***
 * Time/Date API prototypes:
 */

rmf_Error rmf_osal_getTimeOfDay(rmf_osal_TimeVal *tv);

rmf_Error rmf_osal_setTimeOfDay(rmf_osal_TimeVal *tv);

rmf_Error rmf_osal_timeGet(rmf_osal_Time *time);

rmf_Error rmf_osal_timeGetMillis(rmf_osal_TimeMillis *time);

rmf_Error rmf_osal_timeToDate(rmf_osal_Time time, rmf_osal_TimeTm *local);

rmf_osal_TimeClock rmf_osal_timeClock(void);

rmf_osal_TimeClock rmf_osal_timeSystemClock(void);

rmf_osal_TimeClock rmf_osal_timeClockTicks(void);

uint32_t rmf_osal_timeClockToMillis(rmf_osal_TimeClock clock);

rmf_osal_Time rmf_osal_timeClockToTime(rmf_osal_TimeClock clock);

rmf_osal_TimeClock rmf_osal_timeMillisToClock(uint32_t milliseconds);

rmf_osal_TimeClock rmf_osal_timeTimeToClock(rmf_osal_Time time);

rmf_osal_Time rmf_osal_timeTmToTime(rmf_osal_TimeTm *tm);


#ifdef __cplusplus
}
#endif
#endif /* _RMF_OSAL_TIME_H */
