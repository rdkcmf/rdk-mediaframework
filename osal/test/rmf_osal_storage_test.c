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
#include "rdk_debug.h"
#include "rmf_osal_thread.h"
#include "rmf_osal_event.h"
#include "rmf_osal_storage.h"

#if USE_SYSRES_MLT
#include "rpl_malloc.h"
#endif

#define STORAGE_EXIT_EVENT 0xABCDFFFF


#define RMF_OSAL_ASSERT_STR(ret,msg) {\
	if (RMF_SUCCESS != ret) {\
		printf("%s:%d : %s \n",__FUNCTION__,__LINE__,msg); \
		abort(); \
	}\
}

static void storage_size_to_string (uint64_t size, char * string)
{
	int bytes, kb, mb, gb;
	bytes = size% 1024;
	size = size/1024;
	kb = size% 1024;
	size = size/1024;
	mb  = size% 1024;
	gb = size/1024;
	sprintf(string, "%dG %dM %dK %d bytes", gb, mb, kb, bytes);
}

static void display_device_info(rmf_osal_StorageHandle handle)
{
	rmf_Error ret;
	char displayname[RMF_OSAL_STORAGE_MAX_DISPLAY_NAME_SIZE];
	char name[RMF_OSAL_STORAGE_MAX_NAME_SIZE];
	char path[RMF_OSAL_STORAGE_MAX_PATH_SIZE];
	uint64_t size;
	char sizestring[100];
	rmf_osal_StorageStatus status;
	uint8_t perm;
	rmf_osal_Bool bvalue;

	ret = rmf_osal_storageGetInfo(handle,
		RMF_OSAL_STORAGE_DISPLAY_NAME, displayname);
	RMF_OSAL_ASSERT_STR(ret, "rmf_osal_storageGetInfo failed");
	printf("\n*********RMF_OSAL_STORAGE_DISPLAY_NAME: %s*******\n",  displayname);

	ret = rmf_osal_storageGetInfo(handle,
		RMF_OSAL_STORAGE_NAME, name);
	RMF_OSAL_ASSERT_STR(ret, "rmf_osal_storageGetInfo failed");
	printf("RMF_OSAL_STORAGE_NAME: %s\n",  name);

	ret = rmf_osal_storageGetInfo(handle,
		RMF_OSAL_STORAGE_FREE_SPACE, &size);
	RMF_OSAL_ASSERT_STR(ret, "rmf_osal_storageGetInfo failed");
	storage_size_to_string(size, sizestring);
	printf("RMF_OSAL_STORAGE_FREE_SPACE: %s\n",  sizestring);

	ret = rmf_osal_storageGetInfo(handle,
		RMF_OSAL_STORAGE_CAPACITY, &size);
	RMF_OSAL_ASSERT_STR(ret, "rmf_osal_storageGetInfo failed");
	storage_size_to_string(size, sizestring);
	printf("RMF_OSAL_STORAGE_CAPACITY: %s\n",  sizestring);

	ret = rmf_osal_storageGetInfo(handle,
		 RMF_OSAL_STORAGE_STATUS,  &status);
	RMF_OSAL_ASSERT_STR(ret, "rmf_osal_storageGetInfo failed");
	printf("RMF_OSAL_STORAGE_STATUS: %d\n",  status);

	ret = rmf_osal_storageGetInfo(handle,
		RMF_OSAL_STORAGE_GPFS_PATH, path);
	RMF_OSAL_ASSERT_STR(ret, "rmf_osal_storageGetInfo failed");
	printf("RMF_OSAL_STORAGE_GPFS_PATH: %s\n",  path);

	ret = rmf_osal_storageGetInfo(handle,
		RMF_OSAL_STORAGE_MEDIAFS_PARTITION_SIZE, &size);
	RMF_OSAL_ASSERT_STR(ret, "rmf_osal_storageGetInfo failed");
	storage_size_to_string(size, sizestring);
	printf("RMF_OSAL_STORAGE_MEDIAFS_PARTITION_SIZE: %s\n",  sizestring);

	ret = rmf_osal_storageGetInfo(handle,
		RMF_OSAL_STORAGE_MEDIAFS_FREE_SPACE, &size);
	RMF_OSAL_ASSERT_STR(ret, "rmf_osal_storageGetInfo failed");
	storage_size_to_string(size, sizestring);
	printf("RMF_OSAL_STORAGE_MEDIAFS_FREE_SPACE: %s\n",  sizestring);

	ret = rmf_osal_storageGetInfo(handle,
		 RMF_OSAL_STORAGE_SUPPORTED_ACCESS_RIGHTS,  &perm);
	RMF_OSAL_ASSERT_STR(ret, "rmf_osal_storageGetInfo failed");
	printf("RMF_OSAL_STORAGE_SUPPORTED_ACCESS_RIGHTS: 0x%x\n",  perm);

	ret = rmf_osal_storageIsDetachable(handle, &bvalue);
	RMF_OSAL_ASSERT_STR(ret, "rmf_osal_storageIsDetachable failed");
	printf("rmf_osal_storageIsDetachable: %s\n",  (bvalue == TRUE)?"Detachable":"Not Detachable.");

	ret = rmf_osal_storageIsDvrCapable(handle, &bvalue);
	RMF_OSAL_ASSERT_STR(ret, "rmf_osal_storageIsDvrCapable failed");
	printf("rmf_osal_storageIsDvrCapable: %s\n",  (bvalue == TRUE)?"TRUE":"FALSE");

	ret = rmf_osal_storageIsRemovable(handle, &bvalue);
	RMF_OSAL_ASSERT_STR(ret, "rmf_osal_storageIsRemovable failed");
	printf("rmf_osal_storageIsRemovable: %s\n",  (bvalue == TRUE)?"TRUE":"FALSE");


	printf("******************************************************\n");
}
static void storage_event_thread(void * arg)
{
	uint32_t event_type;
	rmf_Error ret;
	rmf_osal_event_handle_t event_handle;
	rmf_osal_eventqueue_handle_t eventqueue_handle = (rmf_osal_eventqueue_handle_t)arg;
	rmf_osal_event_params_t event_params;

	printf("%s: Started\n", __FUNCTION__);

	while (1)
	{
		ret = rmf_osal_eventqueue_get_next_event(  eventqueue_handle,
				&event_handle, NULL, &event_type,  &event_params);
		RMF_OSAL_ASSERT_STR(ret, "rmf_osal_eventqueue_get_next_event failed");

		ret = rmf_osal_event_delete( event_handle);
		RMF_OSAL_ASSERT_STR(ret, "rmf_osal_event_delete failed");

		if ( RMF_OSAL_EVENT_STORAGE_ATTACHED == event_type)
		{
			printf("\n>>>>>>RMF_OSAL_EVENT_STORAGE_ATTACHED<<<<<<\n");
		}
		else if ( RMF_OSAL_EVENT_STORAGE_DETACHED == event_type)
		{
			printf("\n>>>>>>RMF_OSAL_EVENT_STORAGE_DETACHED<<<<<<\n");
		}
		else if ( RMF_OSAL_EVENT_STORAGE_CHANGED == event_type)
		{
			printf("\n>>>>>>RMF_OSAL_EVENT_STORAGE_CHANGED<<<<<<\n");
		}
		else if ( STORAGE_EXIT_EVENT == event_type)
		{
			printf("\n>>>>>>Exiting storage event thread<<<<<<\n");
			break;
		}
		else
		{
			printf("\n>>>>>>Unknown Event<<<<<<\n");
		}
	}

	printf("%s: End\n", __FUNCTION__);
}

int rmf_osal_storage_test()
{
	rmf_Error ret;
	static rmf_osal_eventqueue_handle_t eventqueue_handle;
	uint32_t count;
	rmf_osal_StorageHandle * phandles;
	rmf_osal_StorageHandle detachHandle = NULL;
	int i =0;
	rmf_osal_Bool bvalue;

	ret = rmf_osal_storageGetDeviceCount(&count);
	RMF_OSAL_ASSERT_STR(ret, "rmf_osal_storageGetDeviceCount failed");
	printf(" Storage count = %d\n", count);
	if (0 == count)
	{
		return 0;
	}

	phandles = (rmf_osal_StorageHandle*) malloc(count*sizeof( rmf_osal_StorageHandle));
	ret = rmf_osal_storageGetDeviceList(&count, phandles);
	RMF_OSAL_ASSERT_STR(ret, "rmf_osal_storageGetDeviceList failed");

	for (i = 0; i<count; i++)
	{
		display_device_info ( phandles[i]);
	}

	{
		rmf_osal_ThreadId  threadId;
		ret = rmf_osal_eventqueue_create( (const uint8_t*)"RCVQ",
				&eventqueue_handle);
		RMF_OSAL_ASSERT_STR(ret, "rmf_osal_eventqueue_create failed");

		ret = rmf_osal_storageRegisterQueue( eventqueue_handle);
		RMF_OSAL_ASSERT_STR(ret, "rmf_osal_registerForPowerKey failed");

		ret = rmf_osal_threadCreate( storage_event_thread, 
					(void*)eventqueue_handle,
					RMF_OSAL_THREAD_PRIOR_DFLT, 2048, &threadId,
					"Thread_1");
		RMF_OSAL_ASSERT_STR(ret, "rmf_osal_threadCreate failed");

		sleep (1);
	}

	for (i = 0; i<count; i++)
	{
		ret = rmf_osal_storageIsDetachable(phandles[i], &bvalue);
		RMF_OSAL_ASSERT_STR(ret, "rmf_osal_storageIsDetachable failed");
		if (bvalue == TRUE)
		{
			printf("rmf_osal_storageIsDetachable: found Detachable device\n");
			detachHandle = phandles[i];
			break;
		}
	}
	if (NULL != detachHandle)
	{
		ret = rmf_osal_storageMakeReadyForUse( detachHandle );
		RMF_OSAL_ASSERT_STR(ret, "rmf_osal_storageMakeReadyForUse failed");
		printf("rmf_osal_storageMakeReadyForUse: success - sleep 1 sec\n");

		sleep (1);

		ret = rmf_osal_storageInitializeDevice( detachHandle, TRUE, 0 );
		RMF_OSAL_ASSERT_STR(ret, "rmf_osal_storageInitializeDevice failed");
		printf("rmf_osal_storageInitializeDevice: success - sleep 1 sec\n");

		sleep (1);
		display_device_info ( detachHandle);

		ret = rmf_osal_storageEject( detachHandle );
		RMF_OSAL_ASSERT_STR(ret, "rmf_osal_storageEject failed");
		printf("rmf_osal_storageEject: success - sleep 1 sec\n");

		sleep (1);
	}

	{
		rmf_osal_event_handle_t event_handle;
		ret = rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_STORAGE, STORAGE_EXIT_EVENT, 
				NULL, &event_handle);
		RMF_OSAL_ASSERT_STR(ret, "rmf_osal_event_create failed");

		ret = rmf_osal_eventqueue_add( eventqueue_handle,
				event_handle);
		RMF_OSAL_ASSERT_STR(ret, "rmf_osal_eventqueue_add failed");
	}

	return 0;
}
