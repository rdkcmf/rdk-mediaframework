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
#include "rmf_osal_sync.h"
#include "rmf_osal_thread.h"
#include <unistd.h>

#define RMF_OSAL_COND_ASSERT(err,msg) {\
	if (RMF_SUCCESS != err) {\
		printf("%s:%d : %s \n",__FUNCTION__,__LINE__,msg); \
		abort(); \
	}\
}

static rmf_osal_Cond g_cond;
static rmf_osal_ThreadId threadId;
static void condition_thread_1(void * arg)
{
	rmf_Error ret;

	printf("%s:%d : rmf_osal_condGet \n", __FUNCTION__, __LINE__);
	ret = rmf_osal_condGet( g_cond);
	RMF_OSAL_COND_ASSERT( ret, "rmf_osal_condGet failed");

	printf("%s:%d : rmf_osal_condGet success\n", arg, __LINE__);
	return ;
}

int rmf_osal_cond_test()
{
	rmf_Error ret;

	printf("rmf_osal_condNew\n");
	ret = rmf_osal_condNew( TRUE, FALSE, &g_cond);
	RMF_OSAL_COND_ASSERT( ret, "rmf_osal_condNew failed");
	printf("rmf_osal_condNew success\n");

	printf("rmf_osal_condWaitFor \n");
	ret = rmf_osal_condWaitFor(g_cond, 1000);
	if (ret)
		printf("%s:%d : rmf_osal_condWaitFor  returned %d\n", __FUNCTION__, __LINE__, ret);
	else
		printf("%s:%d : rmf_osal_condWaitFor success\n", __FUNCTION__, __LINE__);

	printf("%s:%d : rmf_osal_condSet\n", __FUNCTION__, __LINE__);
	ret = rmf_osal_condSet( g_cond);
	RMF_OSAL_COND_ASSERT( ret, "rmf_osal_condSet failed");

	printf("rmf_osal_condWaitFor \n");
	ret = rmf_osal_condWaitFor(g_cond, 1000);
	if (ret)
		printf("%s:%d : rmf_osal_condWaitFor  returned %d\n", __FUNCTION__, __LINE__, ret);
	else
		printf("%s:%d : rmf_osal_condWaitFor success\n", __FUNCTION__, __LINE__);

	printf("Creating thread\n");
	ret = rmf_osal_threadCreate(condition_thread_1, (void*)"Thread_1",
			RMF_OSAL_THREAD_PRIOR_DFLT, 2048, &threadId,
	        "Thread_1");
	RMF_OSAL_COND_ASSERT( ret, "rmf_osal_threadCreate failed");

	printf("Creating thread\n");
	ret = rmf_osal_threadCreate(condition_thread_1, (void*)"Thread_2",
			RMF_OSAL_THREAD_PRIOR_DFLT, 2048, &threadId,
	        "Thread_2");
	RMF_OSAL_COND_ASSERT( ret, "rmf_osal_threadCreate failed");

	printf("%s:%d : Sleeping for 5 sec\n", __FUNCTION__, __LINE__);
	sleep (5);

	printf("%s:%d : rmf_osal_condSet\n", __FUNCTION__, __LINE__);
	ret = rmf_osal_condSet( g_cond);
	RMF_OSAL_COND_ASSERT( ret, "rmf_osal_condSet failed");

	printf("%s:%d : Sleeping for 5 sec\n", __FUNCTION__, __LINE__);
	sleep (5);

	printf("%s:%d : rmf_osal_condUnSet\n", __FUNCTION__, __LINE__);
	ret = rmf_osal_condUnset( g_cond);
	RMF_OSAL_COND_ASSERT( ret, "rmf_osal_condUnset failed");

	printf("rmf_osal_condDelete\n");
	ret = rmf_osal_condDelete( g_cond);
	RMF_OSAL_COND_ASSERT( ret, "rmf_osal_condDelete failed");

	printf("Exit Main\n");
	return 0;
}

