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
#include "rmf_osal_sync.h"
#include "rmf_osal_thread.h"
#include <unistd.h>

static rmf_osal_Mutex mutex;
static rmf_osal_ThreadId threadId;
static void mutex_thread_1(void * arg)
{
	rmf_Error err;

	printf("%s : Trying to acquire mutex \n", __FUNCTION__);
	err = rmf_osal_mutexAcquireTry(mutex);
	if (err)
	{
		printf("%s : rmf_osal_mutexAcquireTry  failed err = %d ..  "
			"Calling rmf_osal_mutexAcquire \n", __FUNCTION__, err);
		err = rmf_osal_mutexAcquire(mutex);
		if (err)
			printf("%s : rmf_osal_mutexAcquire  failed\n", __FUNCTION__);
		else
			printf("%s : rmf_osal_mutexAcquire success\n", __FUNCTION__);
	}
	else
	{
		printf("%s : rmf_osal_mutexAcquireTry  success\n", __FUNCTION__);
	}

	printf("%s : Releasing mutex\n", __FUNCTION__);
	err = rmf_osal_mutexRelease(mutex);
	if (err)
		printf("%s : mutex release failed\n", __FUNCTION__);
	else
		printf("%s : mutex release success\n", __FUNCTION__);

	return ;
}

int rmf_osal_mutex_test()
{
	rmf_Error err;

	printf("Creating mutex\n");
	err = rmf_osal_mutexNew(&mutex);
	if (err)
		printf("mutex create failed\n");
	else
		printf("mutex create success\n");

	printf("Acquiring mutex\n");
	err = rmf_osal_mutexAcquire(mutex);
	if (err)
		printf("%s : rmf_osal_mutexAcquire  failed\n", __FUNCTION__);
	else
		printf("%s : rmf_osal_mutexAcquire success\n", __FUNCTION__);

	printf("Creating thread\n");
	err = rmf_osal_threadCreate(mutex_thread_1, NULL,
			RMF_OSAL_THREAD_PRIOR_DFLT, 2048, &threadId,
	        "Thread_1");
	if (err)
		printf("Thread create failed\n");
	else
		printf("Thread create success\n");

	printf("Main : Sleeping for 10 sec\n");
	sleep (10);

	printf("%s : Releasing mutex\n", __FUNCTION__);
	err = rmf_osal_mutexRelease(mutex);
	if (err)
		printf("%s : mutex release failed\n", __FUNCTION__);
	else
		printf("%s : mutex release success\n", __FUNCTION__);
	printf("%s : Sleeping for 10 sec\n", __FUNCTION__);
	sleep (10);

	printf("%s : Trying to acquire mutex \n", __FUNCTION__);
	err = rmf_osal_mutexAcquireTry(mutex);
	if (err)
	{
		printf("%s : rmf_osal_mutexAcquireTry  failed err = %d ..  "
			"Calling rmf_osal_mutexAcquire \n", __FUNCTION__, err);
		err = rmf_osal_mutexAcquire(mutex);
		if (err)
			printf("%s : rmf_osal_mutexAcquire  failed\n", __FUNCTION__);
		else
			printf("%s : rmf_osal_mutexAcquire success\n", __FUNCTION__);
	}
	else
	{
		printf("%s : rmf_osal_mutexAcquireTry  success\n", __FUNCTION__);
	}

	printf("%s : Releasing mutex\n", __FUNCTION__);
	err = rmf_osal_mutexRelease(mutex);
	if (err)
		printf("%s : mutex release failed\n", __FUNCTION__);
	else
		printf("%s : mutex release success\n", __FUNCTION__);

	printf("rmf_osal_mutexDelete\n");
	err = rmf_osal_mutexDelete(mutex);
	if (err)
		printf("rmf_osal_mutexDelete failed\n");
	else
		printf("rmf_osal_mutexDelete success\n");

	printf("Exit Main\n");
	return 0;
}

