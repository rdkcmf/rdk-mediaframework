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
#include "rmf_osal_event.h"
#include "rmf_osal_mem.h"
#include <unistd.h>

#if USE_SYSRES_MLT
#include "rpl_malloc.h"
#endif

#define RMF_OSAL_ASSERT(err,msg) {\
	if (RMF_SUCCESS != err) {\
		printf("%s:%d : %s \n",__FUNCTION__,__LINE__,msg); \
		abort(); \
	}\
}

#define RCV_THR_NO 50
#define MAX_EVTQ_NAME_LEN 100
#define EVENT_TST_EXIT_EVT 0xFFFF
#define EVENTS_TO_DISPATCH 50
#define EVENT_TEST_TRY_TIMED 1
#define EVENT_TEST_CREATE_EM 1

static rmf_osal_ThreadId threadId[RCV_THR_NO];
static rmf_osal_eventmanager_handle_t eventmanager_handle =  0;


static void event_receive_thread2(void * arg)
{
	rmf_Error ret;
	uint32_t th_id = (uint32_t)arg;
	uint8_t name[MAX_EVTQ_NAME_LEN];
	int evt_cnt = 0;
	uint32_t event_type;
	rmf_osal_event_handle_t event_handle;
	rmf_osal_event_category_t event_category;
	rmf_osal_eventqueue_handle_t eventqueue_handle;
	rmf_osal_event_params_t event_params;

	snprintf( (char*)name, MAX_EVTQ_NAME_LEN, "RCVQ%d", th_id);

	ret = rmf_osal_eventqueue_create( name,
			&eventqueue_handle);
	RMF_OSAL_ASSERT(ret, "rmf_osal_eventqueue_create failed");

	ret = rmf_osal_eventmanager_register_handler(
			eventmanager_handle,
			eventqueue_handle,
			RMF_OSAL_EVENT_CATEGORY_MPEG);
	RMF_OSAL_ASSERT(ret, "rmf_osal_eventmanager_register_handler failed");

	ret = rmf_osal_eventqueue_delete( eventqueue_handle);
	RMF_OSAL_ASSERT(ret, "rmf_osal_eventqueue_delete failed");

	ret = rmf_osal_eventqueue_create( name,
			&eventqueue_handle);
	RMF_OSAL_ASSERT(ret, "rmf_osal_eventqueue_create failed");

	ret = rmf_osal_eventmanager_register_handler(
			eventmanager_handle,
			eventqueue_handle,
			RMF_OSAL_EVENT_CATEGORY_MPEG);
	RMF_OSAL_ASSERT(ret, "rmf_osal_eventmanager_register_handler failed");

	ret = rmf_osal_eventmanager_register_handler(
			eventmanager_handle,
			eventqueue_handle,
			RMF_OSAL_EVENT_CATEGORY_QAM);
	RMF_OSAL_ASSERT(ret, "rmf_osal_eventmanager_register_handler failed");

	do
	{
#if EVENT_TEST_TRY_TIMED
		if (th_id%10 == 1)
		{
			while (1)
			{
				ret = rmf_osal_eventqueue_try_get_next_event(  eventqueue_handle,
						&event_handle, &event_category, &event_type, &event_params);
				if ( RMF_SUCCESS == ret)
				{
					printf("rmf_osal_eventqueue_try_get_next_event success th_id = %d cat =%d\n", th_id, event_category);
					break;
				}
				if ( RMF_OSAL_ENODATA == ret)
				{
					printf("rmf_osal_eventqueue_try_get_next_event : no data th_id = %d\n", th_id);
					sleep (1);
					continue;
				}
				RMF_OSAL_ASSERT(ret, "rmf_osal_eventqueue_try_get_next_event failed");
			}		
		}
		else if (th_id%10 == 2 )
		{
			while (1)
			{
				ret = rmf_osal_eventqueue_get_next_event_timed(  eventqueue_handle,
						&event_handle, NULL, &event_type,  &event_params, 1000000);
				if ( RMF_SUCCESS == ret)
				{
					printf("rmf_osal_eventqueue_get_next_event_timed success th_id = %d\n", th_id);
					break;
				}
				if ( RMF_OSAL_ENODATA == ret)
				{
					printf("rmf_osal_eventqueue_get_next_event_timed : no data th_id = %d\n", th_id);
					continue;
				}
				RMF_OSAL_ASSERT(ret, "rmf_osal_eventqueue_get_next_event_timed failed");	
			}		
		}
		else
#endif
		{
			ret = rmf_osal_eventqueue_get_next_event(  eventqueue_handle,
					&event_handle, &event_category, &event_type,  &event_params);
			RMF_OSAL_ASSERT(ret, "rmf_osal_eventqueue_get_next_event failed");

			printf("rmf_osal_eventqueue_get_next_event success th_id = %d cat =%d\n", th_id, event_category);
		}

		printf("%s:%d : th_id = %d Received event 0x%x event data = 0x%x data count = 0x%x\n",
				__FUNCTION__,__LINE__, th_id, event_type, 
				(unsigned int)event_params.data, event_params.data_extension);

		ret = rmf_osal_event_delete( event_handle);
		RMF_OSAL_ASSERT(ret, "rmf_osal_event_delete failed");
		evt_cnt++;
		
	}while ( event_type != EVENT_TST_EXIT_EVT );
	ret = rmf_osal_eventqueue_delete( eventqueue_handle);
	RMF_OSAL_ASSERT(ret, "rmf_osal_eventqueue_delete failed");

	printf("Exiting thread th_id = %d event recieved = %d\n", th_id, evt_cnt);

	return ;
}


void free_data(void* data)
{
	printf("%s: %d Freeing ptr 0x%x\n", __FUNCTION__, __LINE__, (unsigned int)data);
	free(data);
}

int rmf_osal_event_test()
{
	rmf_Error ret;
	uint32_t i;
	char * data;
	rmf_osal_event_handle_t event_handle;
	rmf_osal_event_params_t event_params;
	rmf_osal_event_category_t event_category;

#if EVENT_TEST_CREATE_EM
	ret =  rmf_osal_eventmanager_create ( &eventmanager_handle );
	RMF_OSAL_ASSERT(ret, "rmf_osal_eventmanager_create failed");
#endif

	for (i=0 ; i < RCV_THR_NO ; i++)
	{
		if ( i < RCV_THR_NO/2)
		{
			ret = rmf_osal_threadCreate( event_receive_thread2, (void*)i,
					RMF_OSAL_THREAD_PRIOR_DFLT, 2048, &threadId[i],
			        "Thread_1");
		}
		else
		{
			ret = rmf_osal_threadCreate( event_receive_thread2, (void*)i,
					RMF_OSAL_THREAD_PRIOR_DFLT, 2048, &threadId[i],
			        "Thread_1");
			
		}
		if (ret != RMF_SUCCESS )
		{
			printf("rmf_osal_event_test failed on creating thread %d\n", i);
			return ret;
		}

		printf("Thread[%d] create success\n",i);
	}

	sleep (2);

	printf("sending events\n");

	for (i=0 ; i < EVENTS_TO_DISPATCH ; i++)
	{
		/* Send the event. */
		event_params.data = (void*)(0x100+i);
		event_params.data_extension = 0x200+i;
		event_params.free_payload_fn = NULL;
		event_params.priority = i;
		if (i%2)
			event_category = RMF_OSAL_EVENT_CATEGORY_MPEG;
		else
			event_category = RMF_OSAL_EVENT_CATEGORY_QAM;
		ret = rmf_osal_event_create( event_category , i, 
			&event_params, &event_handle);
		RMF_OSAL_ASSERT(ret, "rmf_osal_event_create failed");

		ret = rmf_osal_eventmanager_dispatch_event( eventmanager_handle ,
				event_handle);
		if (RMF_SUCCESS != ret )
		{
			printf("rmf_osal_eventmanager_dispatch_event failed. eventmanager_handle =0x%x event_handle=0x%x", 
						eventmanager_handle, event_handle);
			abort();
		}
	}
	printf("sending events complete\n");
	sleep (5);
	printf("sending exit event \n");

	data = (char*) malloc (10);
	if (NULL == data )
	{
		printf("malloc failed\n");
		abort();
	}

	event_params.data = data;
	event_params.data_extension= 10;
	event_params.free_payload_fn = free_data;
	event_params.priority = 0;

	ret = rmf_osal_event_create ( RMF_OSAL_EVENT_CATEGORY_MPEG, EVENT_TST_EXIT_EVT,
		&event_params, &event_handle);
	RMF_OSAL_ASSERT(ret, "rmf_osal_event_create failed");

	ret = rmf_osal_eventmanager_dispatch_event( eventmanager_handle ,
			event_handle);
	RMF_OSAL_ASSERT(ret, "rmf_osal_eventmanager_dispatch_event failed");

	sleep (10);

#if EVENT_TEST_CREATE_EM
	printf("Deleting event manager assuming all threads exit \n");
	ret =  rmf_osal_eventmanager_delete ( eventmanager_handle );
	RMF_OSAL_ASSERT(ret, "rmf_osal_eventmanager_delete failed");
#endif

#ifdef DEBUG_ALLOC
	print_alloc_info();
#endif

	printf("\nTest complete !!!!\n");

	return 0;
}

