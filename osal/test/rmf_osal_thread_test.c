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

#define RMF_OSAL_THR_ASSERT(err,msg) {\
	if (RMF_SUCCESS != err) {\
		printf("%s:%d : %s \n",__FUNCTION__,__LINE__,msg); \
		abort(); \
	}\
}

static rmf_osal_ThreadId threadId;
static int g_run =TRUE;

//#define STRESS_THREAD
#ifndef STRESS_THREAD
static void thread_1(void * arg)
{
	rmf_Error ret;
	rmf_osal_ThreadStat threadStatus;
	void * data;
	static rmf_osal_ThreadId thId;
	rmf_osal_ThreadPrivateData *private_data;

	printf("%s:%d : rmf_osal_threadAttach with NULL \n", __FUNCTION__, __LINE__);
	ret = rmf_osal_threadAttach( NULL);
	RMF_OSAL_THR_ASSERT( ret, "rmf_osal_threadAttach failed");

	printf("%s:%d : rmf_osal_threadAttach with thid \n", __FUNCTION__, __LINE__);
	ret = rmf_osal_threadAttach( &threadId );
	RMF_OSAL_THR_ASSERT( ret, "rmf_osal_threadAttach failed");

	printf("%s:%d : rmf_osal_threadSetPriority  \n", __FUNCTION__, __LINE__);
	ret = rmf_osal_threadSetPriority( threadId, RMF_OSAL_THREAD_PRIOR_MIN );
	RMF_OSAL_THR_ASSERT( ret, "rmf_osal_threadSetPriority failed");

	printf("%s:%d : rmf_osal_threadGetStatus  \n", __FUNCTION__, __LINE__);
	ret = rmf_osal_threadGetStatus( threadId, &threadStatus );
	RMF_OSAL_THR_ASSERT( ret, "rmf_osal_threadGetStatus failed");
	printf("%s:%d : rmf_osal_threadGetStatus : 0x%x\n", __FUNCTION__, __LINE__, threadStatus);

	printf("%s:%d : rmf_osal_threadSetStatus  \n", __FUNCTION__, __LINE__);
	ret = rmf_osal_threadSetStatus( threadId, RMF_OSAL_THREAD_STAT_EVENT );
	RMF_OSAL_THR_ASSERT( ret, "rmf_osal_threadSetStatus failed");

	printf("%s:%d : rmf_osal_threadGetStatus  \n", __FUNCTION__, __LINE__);
	ret = rmf_osal_threadGetStatus( threadId, &threadStatus );
	RMF_OSAL_THR_ASSERT( ret, "rmf_osal_threadGetStatus failed");
	printf("%s:%d : rmf_osal_threadGetStatus : 0x%x\n", __FUNCTION__, __LINE__, threadStatus);

	printf("%s:%d : rmf_osal_threadSetData  \n", __FUNCTION__, __LINE__);
	ret = rmf_osal_threadSetData( threadId, (void*)1 );
	RMF_OSAL_THR_ASSERT( ret, "rmf_osal_threadSetData failed");

	printf("%s:%d : rmf_osal_threadGetData  \n", __FUNCTION__, __LINE__);
	ret = rmf_osal_threadGetData( threadId, &data );
	RMF_OSAL_THR_ASSERT( ret, "rmf_osal_threadGetData failed");
	printf("%s:%d : rmf_osal_threadGetData returned data = 0x%x  \n", 
		__FUNCTION__, __LINE__, (unsigned int) data );

	printf("%s:%d : rmf_osal_threadGetPrivateData  \n", __FUNCTION__, __LINE__);
	ret = rmf_osal_threadGetPrivateData( threadId, &private_data );
	RMF_OSAL_THR_ASSERT( ret, "rmf_osal_threadGetPrivateData failed");
	printf("%s:%d : rmf_osal_threadGetPrivateData returned data = 0x%x  memcbid = 0x%x\n", 
		__FUNCTION__, __LINE__, (unsigned int)private_data->threadData,  
		(unsigned int)private_data->memCallbackId);

	printf("%s:%d : rmf_osal_threadGetCurrent  \n", __FUNCTION__, __LINE__);
	ret = rmf_osal_threadGetCurrent( &thId );
	RMF_OSAL_THR_ASSERT( ret, "rmf_osal_threadGetCurrent failed");
	printf("%s:%d : rmf_osal_threadGetCurrent : thid 0x%x\n", __FUNCTION__, __LINE__, thId);

	printf("%s:%d : rmf_osal_threadSleep  \n", __FUNCTION__, __LINE__);
	ret = rmf_osal_threadSleep( 1000, 10 );
	RMF_OSAL_THR_ASSERT( ret, "rmf_osal_threadSleep failed");
	printf("%s:%d : rmf_osal_threadSleep returned \n", __FUNCTION__, __LINE__);

	printf("%s:%d : rmf_osal_threadYield  \n", __FUNCTION__, __LINE__);
	 rmf_osal_threadYield( );

	printf("%s:%d : rmf_osal_threadDestroy \n", __FUNCTION__, __LINE__);
	ret = rmf_osal_threadDestroy( threadId );
	RMF_OSAL_THR_ASSERT( ret, "rmf_osal_threadDestroy failed");

	return ;
}
#else
#define NUMBER_OF_THREADS 150
static void thread_2(void * arg)
{
	int  no = (*(int *) arg) % 5;
	printf("%s:%d : *arg = %d sleeping %d sec \n", __FUNCTION__, __LINE__, *(int *) arg, no);

	sleep(no);
}
static void thread_1(void * arg)
{
	rmf_Error ret;
	static int no = 0;
	char name[20];
	int params[NUMBER_OF_THREADS];
	while (g_run )
	{
		for (int i=0; i<NUMBER_OF_THREADS; i++)
		{
			no++;
			params[i] = no;
			sprintf(name, "Thread_%d", no);
			printf("%s:%d : creating  thread with name %s \n", __FUNCTION__, __LINE__, name);
			ret = rmf_osal_threadCreate( thread_2, &params[i] ,
				RMF_OSAL_THREAD_PRIOR_DFLT, RMF_OSAL_THREAD_STACK_SIZE, &threadId,
				name);
			RMF_OSAL_THR_ASSERT( ret, "rmf_osal_threadCreate failed");
		}
		sleep(5);
	}
}
#endif


int rmf_osal_thread_test()
{
	rmf_Error ret;
	char option;

	printf("Creating thread\n");
	ret = rmf_osal_threadCreate( thread_1, NULL,
			RMF_OSAL_THREAD_PRIOR_DFLT, RMF_OSAL_THREAD_STACK_SIZE, &threadId,
	        "Thread_0");
	RMF_OSAL_THR_ASSERT( ret, "rmf_osal_threadCreate failed");
	printf("%s:%d : rmf_osal_threadCreate : threadId 0x%x\n", __FUNCTION__, __LINE__, threadId);

	printf("%s:%d : rmf_osal_threadSetName \n", __FUNCTION__, __LINE__);
	ret = rmf_osal_threadSetName( threadId, "Test1" );
	RMF_OSAL_THR_ASSERT( ret, "rmf_osal_threadSetName failed");

	while (1)
	{
		printf("%s:%d : x for Exit\n",  __FUNCTION__, __LINE__);
		option = getchar();
		if ('x' == option )
			break;
	}
	g_run = FALSE;
	printf("%s:%d : Exit\n",  __FUNCTION__, __LINE__);
	return 0;
}

