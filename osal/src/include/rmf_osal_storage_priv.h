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

#ifndef _RMF_OSAL_STORAGE_PRIV_H_
#define _RMF_OSAL_STORAGE_PRIV_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <rmf_osal_sync.h>
#include <rmf_osal_event.h>
#include "rmf_osal_storage.h"

#define MAX_STORAGE_DEVICES 10


#define MAX_PATH                        1024

/** 
 * This data type enumerates the set of possible storage volumes types.
 * Note that devices can be both detachable and removable 
 */
typedef enum
{
	RMF_OSAL_STORAGE_TYPE_INTERNAL = 0x0000,
    RMF_OSAL_STORAGE_TYPE_DETACHABLE = 0x0001,
    RMF_OSAL_STORAGE_TYPE_REMOVABLE = 0x0002
} rmf_osal_StorageDeviceType;


/* Definitions represents media volume free space alarm levels */
#define MAX_MEDIA_VOL_LEVEL 99
#define MIN_MEDIA_VOL_LEVEL 1

/* Represents free space Alarm status as armed, unarmed and not used */
typedef enum
{
    ARMED,
    UNARMED,
    UNUSED
} os_FreeSpaceAlarmStatus;

typedef struct _os_MediaVolumeInfo os_MediaVolumeInfo;
typedef struct _os_StorageDeviceInfo os_StorageDeviceInfo;

struct _os_MediaVolumeInfo
{
	char					rootPath[RMF_OSAL_STORAGE_MAX_PATH_SIZE+1];
	char					mediaPath[RMF_OSAL_STORAGE_MAX_PATH_SIZE+1];
	char					dataPath[RMF_OSAL_STORAGE_MAX_PATH_SIZE+1];
    uint64_t				reservedSize;
	uint64_t				usedSize;
	os_FreeSpaceAlarmStatus alarmStatus[MAX_MEDIA_VOL_LEVEL];
	os_StorageDeviceInfo	*device;
	rmf_osal_Mutex				mutex;
	os_MediaVolumeInfo		*next;
};

struct _os_StorageDeviceInfo
{
	char   					name[RMF_OSAL_STORAGE_MAX_NAME_SIZE + 1];
	char					displayName[RMF_OSAL_STORAGE_MAX_DISPLAY_NAME_SIZE + 1];
	char					rootPath[RMF_OSAL_STORAGE_MAX_PATH_SIZE+1];
	char*					mnt_type;
	uint64_t				size; // bytes
	uint64_t				mediaFsSize; // bytes
	uint64_t				defaultMediaFsSize; // bytes
	uint64_t				freeMediaSize; // bytes
	rmf_osal_StorageDeviceType	type;
	rmf_osal_StorageStatus				status;	 // holds values of type rmf_osal_StorageStatus
	uint8_t					attached;
	os_MediaVolumeInfo		*volumes;
	rmf_osal_Mutex				mutex;
};


typedef os_StorageDeviceInfo rmf_osal_Storage;

typedef struct
{
    rmf_osal_Bool queueRegistered;
    void* listenerACT; // asynchronous completion token
} StorageMgrData;


typedef enum 
{
  STORAGELIST_INVALID=0,
  STORAGELIST_INPROGRESS,
  STORAGELIST_COMPLETE
}eStorageDeviceListStatus;

struct _vlStorageDevicesList
{
  rmf_osal_Storage *m_StorageDevice; //Each device info is stored here
  struct _vlStorageDevicesList *m_next; //next Device info Stored here
  
};
typedef struct _vlStorageDevicesList vlStorageDevicesList;


/**
 * This data type enumerates the set of possible error codes that
 * can be passed to another component in the OSAL layer.
 */
typedef enum
{
    OS_STORAGE_ERR_NOERR,			// no error
	OS_STORAGE_ERR_OUT_OF_SPACE,	// no space left on device
	OS_STORAGE_ERR_DEVICE_ERR,      // a hardware device error
} os_StorageError;

char *storageGetRoot();
os_StorageDeviceInfo *storageGetDefaultDevice();
uint64_t computeSpaceUsed(char* path);

rmf_Error rmf_osal_priv_sendStorageMgrEvent(rmf_osal_StorageHandle device, rmf_osal_StorageEvent event, rmf_osal_StorageStatus status);

void rmf_osal_storageInit_IF_vl();

rmf_Error rmf_osal_storageGetDeviceCount_IF_vl(uint32_t* count);

rmf_Error rmf_osal_storageGetDeviceList_IF_vl(uint32_t* count, rmf_osal_StorageHandle *devices);

rmf_Error rmf_osal_storageRegisterQueue_IF_vl (rmf_osal_eventqueue_handle_t eventQueue);

int ri_storageMakeReadyToDetach(char* rootPath);

int ri_storageEject(char* rootPath);

#define STORAGE_TRIM_ROOT(path) ((path) + strlen(storageGetRoot()) + 1)

#ifdef __cplusplus
}
#endif

#endif // _RMF_OSAL_STORAGE_PRIV_H_
