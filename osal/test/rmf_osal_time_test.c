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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "rmf_osal_time.h"
#define RMF_OSAL_ASSERT_TIME(ret,msg) {\
	if (RMF_SUCCESS != ret) {\
		printf("%s:%d : %s \n",__FUNCTION__,__LINE__,msg); \
		abort(); \
	}\
}
int rmf_osal_time_test()
{
	rmf_osal_TimeMillis timeMillis;
	rmf_osal_TimeVal tv;
	rmf_Error ret;
	rmf_osal_Time time;
	rmf_osal_TimeTm local;
	rmf_osal_TimeClock timeClock;
	uint32_t timeMillies32;

	ret = rmf_osal_getTimeOfDay( &tv);
	RMF_OSAL_ASSERT_TIME(ret, "rmf_osal_getTimeOfDay Failed");
	printf("rmf_osal_getTimeOfDay time %llu\n", tv.tv_sec);

	tv.tv_sec +=10;
	printf("rmf_osal_setTimeOfDay time %llu\n", tv.tv_sec);
	ret = rmf_osal_setTimeOfDay( &tv);
	RMF_OSAL_ASSERT_TIME(ret, "rmf_osal_setTimeOfDay Failed");

	ret = rmf_osal_getTimeOfDay( &tv);
	RMF_OSAL_ASSERT_TIME(ret, "rmf_osal_getTimeOfDay Failed");
	printf("rmf_osal_getTimeOfDay time %llu\n", tv.tv_sec);
	sleep(1);

	ret = rmf_osal_getTimeOfDay( &tv);
	RMF_OSAL_ASSERT_TIME(ret, "rmf_osal_getTimeOfDay Failed");
	printf("rmf_osal_getTimeOfDay time %llu\n", tv.tv_sec);

	ret = rmf_osal_timeGet( &time);
	RMF_OSAL_ASSERT_TIME(ret, "rmf_osal_timeGet Failed");
	printf("rmf_osal_timeGet time %llu\n", time);

	ret = rmf_osal_timeGetMillis( &timeMillis);
	RMF_OSAL_ASSERT_TIME(ret, "rmf_osal_timeGetMillis Failed");
	printf("rmf_osal_timeGetMillis time %llu\n", timeMillis);

	ret = rmf_osal_timeToDate( time, &local);
	RMF_OSAL_ASSERT_TIME(ret, "rmf_osal_timeToDate Failed");
	printf("rmf_osal_timeToDate time %s \n", asctime(&local));

	timeClock = rmf_osal_timeClock();
	printf("rmf_osal_timeClock clock %lu \n", timeClock);

	timeClock = rmf_osal_timeSystemClock();
	printf("rmf_osal_timeSystemClock clock %d \n", timeClock);

	timeClock = rmf_osal_timeClockTicks();
	printf("rmf_osal_timeClockTicks ticks %d \n", timeClock);

	timeClock = rmf_osal_timeSystemClock();
	timeMillies32 = rmf_osal_timeClockToMillis( timeClock);
	printf("rmf_osal_timeClockToMillis millis %d \n", timeMillies32);

	time = rmf_osal_timeClockToTime( timeClock);
	printf("rmf_osal_timeClockToTime time %d \n", time);

	timeClock = rmf_osal_timeMillisToClock ( timeMillies32);
	printf("rmf_osal_timeMillisToClock clock %d \n", timeClock);

	timeClock = rmf_osal_timeTimeToClock (time );
	printf("rmf_osal_timeTimeToClock clock %d \n", timeClock);

	time = rmf_osal_timeTmToTime ( &local );
	printf("rmf_osal_timeTmToTime time %llu\n", time);
	

	printf ( "\n test complete\n");
	return 0;
}
