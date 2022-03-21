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
 * @file rmf_osal_storage.h
 * @brief OSAL Storage APIs
 */

/**
 * @defgroup OSAL_STORAGE OSAL Storage
 * RMF OSAL Storage module notifies users when storage devices are added, removed or on a status change. 
 * @ingroup OSAL
 *
 * @defgroup OSAL_STORAGE_API OSAL Storage API list
 * Described the details about OSAL Storage interface specification.
 * @ingroup OSAL_STORAGE
 *
 * @addtogroup OSAL_STORAGE_TYPES OSAL Storage Data Types
 * Described the details about Data Structure used by OSAL Storage.
 * @ingroup OSAL_STORAGE
 */

#ifndef _RMF_OSAL_STORAGE_H_
#define _RMF_OSAL_STORAGE_H_

#include <rmf_osal_types.h>
#include "rmf_error.h"
#include <rmf_osal_event.h>


#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @addtogroup OSAL_STORAGE_TYPES OSAL Storage Data Types
 * Describe the details about Data Structure used by OSAL Storage APIs.
 * @ingroup OSAL_TYPES
 * @{
 */

/******************************************************************************
 * Public Data Types, Constants, and Enumerations
 *****************************************************************************/

/**
 * This macro defines the maximum number of characters in the human readable
 * display name for a storage device.
 */
#define RMF_OSAL_STORAGE_MAX_DISPLAY_NAME_SIZE 40

/**
 * This macro defines the maximum number of characters in the storage device name.
 */
#define RMF_OSAL_STORAGE_MAX_NAME_SIZE 20

/**
 *  This macro defines the maximum number of characters in the root path of a
 * storage device.  
 */
#define RMF_OSAL_STORAGE_MAX_PATH_SIZE 248

/**
 * rmf_osal_Storage is an opaque data type that represents a storage device.
 * The internal representation of this data type is known only by the
 * platform-dependent RMF OSAL implementation.  rmf_osal_StorageHandle represents
 * a platform-independent handle or reference to a particular storage device.
 */
typedef void* rmf_osal_StorageHandle;

/**
 * @enum rmf_osal_StorageEvent
 * @brief This data type enumerates the set of possible event codes triggered by the Storage module
 *
 * WARNING! This enumeration must remain in synch with the event codes defined in
 * org.ocap.storage.StorageManagerEvent
 */
typedef enum
{
    RMF_OSAL_EVENT_STORAGE_ATTACHED = 0,
    RMF_OSAL_EVENT_STORAGE_DETACHED,
    RMF_OSAL_EVENT_STORAGE_CHANGED
} rmf_osal_StorageEvent;

/**
 * @enum rmf_osal_StorageInfoParam
 * @brief This data type enumerates the set of possible storage device attributes
 * that may be queried via the rmf_osal_storageGetInfo() API.
 */
typedef enum
{
    /** human friendly name of the device for display char array with size RMF_OSAL_STORAGE_MAX_DISPLAY_NAME_SIZE*/
    RMF_OSAL_STORAGE_DISPLAY_NAME, 
    /** unique device name char array with size RMF_OSAL_STORAGE_MAX_NAME_SIZE*/
    RMF_OSAL_STORAGE_NAME, 
    RMF_OSAL_STORAGE_FREE_SPACE, //!< total free space (MEDIAFS + GPFS) 
    RMF_OSAL_STORAGE_CAPACITY, //!< total capacity (MEDIAFS + GPFS - HIDDENFS)
    RMF_OSAL_STORAGE_STATUS, //!< status of the device. See rmf_osal_StorageStatus
    /** general purpose file system path for file i/o. char array with size RMF_OSAL_STORAGE_MAX_PATH_SIZE*/
    RMF_OSAL_STORAGE_GPFS_PATH, 
    RMF_OSAL_STORAGE_GPFS_FREE_SPACE, //!< free space on general purpose file system
    RMF_OSAL_STORAGE_MEDIAFS_PARTITION_SIZE, //!< size of the MEDIAFS partition
    // note that GPFS partition size is derived by
    // subtracting this value from the RMF_OSAL_STORAGE_CAPACITY
    RMF_OSAL_STORAGE_MEDIAFS_FREE_SPACE, //!< free space of the MEDIAFS partition
    /** access right supported as uint8_t bitmask. see rmf_osal_StorageSupportedAccessRights */
    RMF_OSAL_STORAGE_SUPPORTED_ACCESS_RIGHTS
} rmf_osal_StorageInfoParam;

/**
 * @enum rmf_osal_StorageStatus
 * @brief This data type enumerates the set of possible states that a storage device may be in
 *
 * WARNING! The ordering of items in this enumeration must remain in sync with the
 * status constants defined in org.ocap.storage.StorageProxy.java
 */
typedef enum
{
    /** indicates that the device is mounted, initialized and ready for use */
    RMF_OSAL_STORAGE_STATUS_READY = 0,
    /** indicates that the device is present  but requires a call to rmf_osal_storageMakeReadyForUse() before it can be used */
    RMF_OSAL_STORAGE_STATUS_OFFLINE,
    /** indicates that the device is busy (e.g. being initialized configured, checked for consistency, or made ready to detach)
     * It does not indicate that I/O operations are currently in progress 
     */
    RMF_OSAL_STORAGE_STATUS_BUSY, 
    /** the device is not supported by the platform */
    RMF_OSAL_STORAGE_STATUS_UNSUPPORTED_DEVICE,
    /** the device type and model is supported by the platform, but the current partitioning or formatting is not supported by
     * the platform without reinitialization and loss of the existin contents
     */
    RMF_OSAL_STORAGE_STATUS_UNSUPPORTED_FORMAT,
    /** the device is not formatted and contains no existing data */ 
    RMF_OSAL_STORAGE_STATUS_UNINITIALIZED,
    /** indicates that the device is in an unrecoverable error **/
    RMF_OSAL_STORAGE_STATUS_DEVICE_ERR,
    /** state and cannot be used */
    RMF_OSAL_STORAGE_STATUS_NOT_PRESENT
// indicates that a detected storage device bay does not
// contain a removable storage device
} rmf_osal_StorageStatus;

/**
 * @enum rmf_osal_StorageSupportedAccessRights
 * @brief This data type enumerates the set of bit masks for determining the set of
 * access rights supported by a given storage device.
 *
 * @see rmf_osal_storageGetInfo(RMF_OSAL_STORAGE_SUPPORTED_ACCESS_RIGHTS)
 */
typedef enum
{
    RMF_OSAL_STORAGE_ACCESS_RIGHT_WORLD_READ = 1,
    RMF_OSAL_STORAGE_ACCESS_RIGHT_WORLD_WRITE = 2,
    RMF_OSAL_STORAGE_ACCESS_RIGHT_APP_READ = 4,
    RMF_OSAL_STORAGE_ACCESS_RIGHT_APP_WRITE = 8,
    RMF_OSAL_STORAGE_ACCESS_RIGHT_ORG_READ = 16,
    RMF_OSAL_STORAGE_ACCESS_RIGHT_ORG_WRITE = 32,
    RMF_OSAL_STORAGE_ACCESS_RIGHT_OTHER_READ = 64,
    RMF_OSAL_STORAGE_ACCESS_RIGHT_OTHER_WRITE = 128
} rmf_osal_StorageSupportedAccessRights;

/**
 * @enum rmf_osal_StorageError
 * @brief This data type enumerates the set of possible error codes that
 * may be returned by Storage Manager calls.
 */
typedef enum
{
    RMF_OSAL_STORAGE_ERR_INVALID_PARAM = RMF_ERRBASE_OSAL_STORAGE+1,//!< a parameter is invalid
    RMF_OSAL_STORAGE_ERR_OUT_OF_MEM , //!< out of memory
    RMF_OSAL_STORAGE_ERR_OUT_OF_SPACE, //!< no space left on device
    RMF_OSAL_STORAGE_ERR_BUSY, //!< device or resource is busy
    RMF_OSAL_STORAGE_ERR_UNSUPPORTED, //!< operation is not supported
    RMF_OSAL_STORAGE_ERR_NOT_ALLOWED, //!< operation is not allowed
    RMF_OSAL_STORAGE_ERR_DEVICE_ERR, //!< a hardware device error
    RMF_OSAL_STORAGE_ERR_OUT_OF_STATE, //!< operation not appropriate at this time
    RMF_OSAL_STORAGE_ERR_BUF_TOO_SMALL, //!< the specified memory buffer is not large enough to fit all of the data
    RMF_OSAL_STORAGE_ERR_UNKNOWN //!< unknown error
} rmf_osal_StorageError;

/** @} */

/******************************************************************************
 * Public Function Prototypes
 *****************************************************************************/

/**
 * @addtogroup OSAL_STORAGE_API
 * @{
 */

void rmf_osal_storageInit(void);


rmf_Error rmf_osal_storageRegisterQueue(rmf_osal_eventqueue_handle_t eventQueue);

rmf_Error rmf_osal_storageGetDeviceCount(uint32_t* count);

rmf_Error rmf_osal_storageGetDeviceList(uint32_t* count,
        rmf_osal_StorageHandle *devices);

rmf_Error rmf_osal_storageGetInfo(rmf_osal_StorageHandle device,
        rmf_osal_StorageInfoParam param, void* output);

rmf_Error rmf_osal_storageIsDetachable(rmf_osal_StorageHandle device,
        rmf_osal_Bool* value);


rmf_Error rmf_osal_storageMakeReadyToDetach(rmf_osal_StorageHandle device);

rmf_Error rmf_osal_storageMakeReadyForUse(rmf_osal_StorageHandle device);

rmf_Error rmf_osal_storageIsDvrCapable(rmf_osal_StorageHandle device,
        rmf_osal_Bool* value);

rmf_Error rmf_osal_storageIsRemovable(rmf_osal_StorageHandle device,
        rmf_osal_Bool* value);

rmf_Error rmf_osal_storageEject(rmf_osal_StorageHandle device);

rmf_Error rmf_osal_storageInitializeDevice(rmf_osal_StorageHandle device,
        rmf_osal_Bool userAuthorized, uint64_t* mediaFsSize);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* _RMF_OSAL_STORAGE_H_ */

