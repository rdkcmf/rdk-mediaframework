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
 * @file rmf_osal_storage.c
 * @brief OSAL Storage API.
 */

#include <rmf_osal_storage.h>        /* Resolve storage definitions. */
#include <rmf_osal_storage_priv.h>      /* Resolve storage module definitions */

#include <rmf_osal_types.h>          /* Resolve basic type references. */
#include <rmf_osal_error.h>          /* Resolve error code definitions */

#include <rmf_osal_event.h>        /* Resolve event definitions. */
#include <rmf_osal_mem.h>          // Resolve memory definitions.
#include "rdk_debug.h"        /* Resolve debug module definitions */
//#include <rmf_osal_util.h>         /* Resolve generic STB definitions */
#include <rmf_osal_file.h>

#include <sys/mount.h>
#include <errno.h>

#include <string.h> /*strcpy and strcat*/
/******************************************************************************
 * PRIVATE Data Types
 *****************************************************************************/

static rmf_osal_Storage * gDevices[MAX_STORAGE_DEVICES];
static char gStorageRoot[MAX_PATH];
static uint32_t gDevCount = 0;

/******************************************************************************
 * PRIVATE functions
 *****************************************************************************/

static rmf_osal_Bool isValidDevice(rmf_osal_StorageHandle device)
{
	rmf_osal_Bool result = FALSE;
	int ctr;

	for (ctr = 0; ctr < gDevCount; ctr++)
	{
		if (device == gDevices[ctr])
		{
			result = TRUE;
			break;
		}
	}

	return result;
}
/******************************************************************************
 * RMF_OSAL private functions
 *****************************************************************************/

// Recursively compute the space used by the contents of this directory
uint64_t computeSpaceUsed(char* path)
{
    uint64_t bytes = 0;
    rmf_osal_Dir d;
    rmf_osal_DirEntry ent;
    char entryName[RMF_OSAL_FS_MAX_PATH];
    
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "<<STORAGE>> %s Entering ...\n", __FUNCTION__);

    if (rmf_osal_dirOpen(path, &d) != RMF_SUCCESS)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE", "<<DVR>> %s - could not open directory %s\n",
                  __FUNCTION__, path);
        return 0;
    }

    // Iterate over all directory entries
    while (rmf_osal_dirRead(d,&ent) == RMF_SUCCESS)
    {
        /* Skip ".." and "." entries */
        if (strcmp(ent.name,".") == 0 || strcmp(ent.name,"..") == 0)
            continue;
        if (strlen(path)+strlen(ent.name)+2 > RMF_OSAL_FS_MAX_PATH)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE", "**%s -  path size greater than maximum. path  %s entry %s\n",
                          __FUNCTION__, path, ent.name);
            continue;            
        }
        /* Create the full path for this directory entry */
        snprintf(entryName, sizeof(entryName), "%s%s%s",path,"/",ent.name);
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.STORAGE", "**%s - entryName = %s\n",
                          __FUNCTION__, entryName);
        /* If this is a directory, recursively compute the size */
        if (ent.isDir)
            bytes += computeSpaceUsed(entryName);
        /* If this is a file, just add the size of the file */
        else
        {
            rmf_osal_FileInfo info;
            rmf_osal_File f;

            if (rmf_osal_fileOpen(entryName,RMF_OSAL_FS_OPEN_READ,&f) != RMF_SUCCESS)
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE", "<<DVR>> %s - could not open file %s\n",
                          __FUNCTION__, entryName);
                continue;
            }
            
            /* Get the size of this file on disk */
            if (rmf_osal_fileGetFStat(f, RMF_OSAL_FS_STAT_SIZE, &info) != RMF_SUCCESS)
            {
                rmf_osal_fileClose(f);
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE", "<<DVR>> %s - could not stat file %s\n",
                          __FUNCTION__, entryName);
                continue;
            }

            bytes += info.size;
            rmf_osal_fileClose(f);
        }
    }
	rmf_osal_dirClose(d);

	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "<<STORAGE>> %s Exiting ... \n", __FUNCTION__);

    return bytes;
}

char *storageGetRoot()
{
	return gStorageRoot;
}

os_StorageDeviceInfo *storageGetDefaultDevice()
{
	return *gDevices;
}

/******************************************************************************
 * PUBLIC functions
 *****************************************************************************/

/**
 * @brief This function initializes the RMF OSAL storage manager module.
 * 
 * This API must be called only once per boot cycle.  It must be called before any other OSAL
 * Storage module APIs are called.  Any RMF OSAL Storage module calls made before
 * this function is called will result in an RMF_OSAL_STORAGE_ERR_OUT_OF_STATE error
 * being returned to the caller
 */
void rmf_osal_storageInit(void)
{
	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "\n <<STORAGE>> %s: Entering ...\n", __FUNCTION__);

	rmf_osal_storageInit_IF_vl();
	rmf_osal_storageGetDeviceCount_IF_vl(&gDevCount);
	rmf_osal_storageGetDeviceList_IF_vl(&gDevCount,(rmf_osal_StorageHandle*) gDevices);
	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "\n <<STORAGE>> %s: Exiting ...\n", __FUNCTION__);
}

/**
 * @brief This function registers a specified queue to receive storage related events.
 * Only one queue may be registered at a time.  Subsequent calls replace
 * previously registered queue.
 *
 * @param[in] eventQueue specifies the destination queue for storage events
 *
 * @return RMF_SUCCESS if no error
 * @retval RMF_OSAL_STORAGE_ERR_INVALID_PARAM if specified queue is invalid
 */
rmf_Error rmf_osal_storageRegisterQueue(rmf_osal_eventqueue_handle_t eventQueue)
{
	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE",
            "<<STORAGE>> rmf_osal_storageRegisterQueue\n");
	rmf_osal_storageRegisterQueue_IF_vl (eventQueue);
	return RMF_SUCCESS;
}

/**
 * @brief This function retrieves the number of storage devices found on the platform.
 *
 * @param[out] count indicates the number of devices found on output
 *
 * @retval RMF_SUCCESS if no error
 * @retval RMF_OSAL_STORAGE_ERR_INVALID_PARAM if specified NULL parameter
 */
rmf_Error rmf_osal_storageGetDeviceCount(uint32_t* count)
{
    *count = gDevCount;

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "<<STORAGE>> %s - count = %d\n",
            __FUNCTION__, *count);

    return RMF_SUCCESS;
}

/**
 * @brief This function retrieves the list of storage devices found on the platform.
 * The first device in the list returned is the default storage device for the
 * platform (e.g. internal HDD).
 *
 * @param[out] count on input, this parameter specifies the number of device handles
 *             that can fit into the pre-allocated devices array passed as the
 *             second parameter.  On output, indicates the actual number of device
 *             handles returned in the devices array.
 * @param[in]  device_handles specifies a pre-allocated memory buffer for returning an array
 *             of native storage handles.
 *
 * @return RMF_SUCCESS if no error
 * @retval RMF_OSAL_STORAGE_ERR_INVALID_PARAM if specified NULL parameter
 * @retval RMF_OSAL_STORAGE_ERR_BUF_TOO_SMALL indicates that the pre-allocated devices
 * buffer is not large enough to fit all of the native device handles found on the platform.
 * Only as many devices as can fit in the buffer are returned.
 */
rmf_Error rmf_osal_storageGetDeviceList(uint32_t* count,
        rmf_osal_StorageHandle *device_handles)
{
    rmf_Error result = RMF_OSAL_STORAGE_ERR_OUT_OF_STATE;
    rmf_osal_Storage ** devices = (rmf_osal_Storage**)device_handles;
    uint32_t deviceCount;
    uint32_t i;

    if (count == NULL || devices == NULL)
    {
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE",
                "<<STORAGE>> %s - NULL parameters!\n", __FUNCTION__);
        return RMF_OSAL_STORAGE_ERR_INVALID_PARAM;
    }

    // Determine if the input buffer is large enough */
    (void) rmf_osal_storageGetDeviceCount(&deviceCount); // Should the return value be checked here?
    if (*count < deviceCount)
    {
        result = RMF_OSAL_STORAGE_ERR_BUF_TOO_SMALL;
        RDK_LOG(
                RDK_LOG_DEBUG,
                "LOG.RDK.STORAGE",
                "<<STORAGE>> %s - Input buffer is too small, only returning %d out of %d devices!\n",
                __FUNCTION__, *count, deviceCount);
    }
    else
    {
        *count = deviceCount;
        result = RMF_SUCCESS;
    }

    // Copy storage devices to caller's buffer
    for (i = 0; i < deviceCount; i++)
    {
        devices[i] = gDevices[i];
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE",
                "<<STORAGE>> %s - Device Name = %s\n", __FUNCTION__,
                devices[i]->displayName);
    }

    return result;
}

/**
 * @brief This function is used to retrieve information about a given storage device.
 *
 * @param[in] device specifies the device
 * @param[in] param specifies the attribute of interest
 * @param[out] output specifies the attribute value on output
 *
 * @retval RMF_SUCCESS if no error
 * @retval RMF_OSAL_STORAGE_ERR_INVALID_PARAM if invalid parameter was specified
 * @retval  RMF_OSAL_STORAGE_ERR_OUT_OF_STATE if queried path name before device has entered the ready state
 */
rmf_Error rmf_osal_storageGetInfo(rmf_osal_StorageHandle device_handle,
        rmf_osal_StorageInfoParam param, void* output)
{
    volatile rmf_Error result = RMF_OSAL_STORAGE_ERR_OUT_OF_STATE;
    rmf_osal_Storage * device = (rmf_osal_Storage*)device_handle;

    if (!output || !isValidDevice(device))
    {
        result = RMF_OSAL_STORAGE_ERR_INVALID_PARAM;
    }
    else
    {
        result = RMF_SUCCESS;
        switch (param)
        {
        case RMF_OSAL_STORAGE_DISPLAY_NAME:
            strcpy((char*) output, device->displayName);
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE",
                    "<<STORAGE>> %s - displayName = %s\n", __FUNCTION__,
                    (char*) output);
            break;

        case RMF_OSAL_STORAGE_NAME:
            strcpy((char*) output, device->name);
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE",
                    "<<STORAGE>> %s (%s) - name = %s\n", __FUNCTION__,
                    device->displayName, (char*) output);
            break;

		case RMF_OSAL_STORAGE_FREE_SPACE:
			*((uint64_t*) output) = device->size - computeSpaceUsed(device->rootPath);
			RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "<<STORAGE>> %s (%s) - free space = %llu\n", __FUNCTION__, device->displayName, *((uint64_t*) output));
			break;

		case RMF_OSAL_STORAGE_CAPACITY:
			*((uint64_t*) output) = device->size;
			RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "<<STORAGE>> %s (%s) - size = %llu\n", __FUNCTION__, device->displayName, *((uint64_t*) output));
			break;

        case RMF_OSAL_STORAGE_STATUS:
            *((rmf_osal_StorageStatus*) output) = (rmf_osal_StorageStatus)device->status;
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE",
                    "<<STORAGE>> %s (%s) - status = %d\n", __FUNCTION__,
                    device->displayName, *((uint32_t*) output));
            break;

        case RMF_OSAL_STORAGE_GPFS_PATH:
            strcpy((char*)output, device->rootPath);
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE",
                    "<<STORAGE>> %s (%s) - GPFS path = %s\n", __FUNCTION__,
                    device->displayName, (char*) output);
            break;

        case RMF_OSAL_STORAGE_GPFS_FREE_SPACE:
            // This is currently not used by any JNI or MPE code.  Not sure why we need it
            result = RMF_OSAL_STORAGE_ERR_UNSUPPORTED;
            break;

		case RMF_OSAL_STORAGE_MEDIAFS_PARTITION_SIZE:
			*((uint64_t*) output) = device->mediaFsSize;
			RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "<<STORAGE>> %s (%s) - media fs size = %llu\n", __FUNCTION__, device->displayName, *((uint64_t*) output));
			break;

		case RMF_OSAL_STORAGE_MEDIAFS_FREE_SPACE:
			*((uint64_t*) output) = device->freeMediaSize;
			RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "<<STORAGE>> %s (%s) - free media size = %llu\n", __FUNCTION__, device->displayName, *((uint64_t*) output));
			break;

        case RMF_OSAL_STORAGE_SUPPORTED_ACCESS_RIGHTS:
            // Support all access on all devices
            *((uint8_t*) output) = RMF_OSAL_STORAGE_ACCESS_RIGHT_WORLD_READ
                    | RMF_OSAL_STORAGE_ACCESS_RIGHT_WORLD_WRITE
                    | RMF_OSAL_STORAGE_ACCESS_RIGHT_APP_READ
                    | RMF_OSAL_STORAGE_ACCESS_RIGHT_APP_WRITE
                    | RMF_OSAL_STORAGE_ACCESS_RIGHT_ORG_READ
                    | RMF_OSAL_STORAGE_ACCESS_RIGHT_ORG_WRITE
                    | RMF_OSAL_STORAGE_ACCESS_RIGHT_OTHER_READ
                    | RMF_OSAL_STORAGE_ACCESS_RIGHT_OTHER_WRITE;
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE",
                    "<<STORAGE>> %s (%s) - access rights = %d\n", __FUNCTION__,
                    device->displayName, *((uint8_t*) output));
            break;

        default:
            result = RMF_OSAL_STORAGE_ERR_INVALID_PARAM;
            break;
        }
    }

    return result;
}

/**
 * @brief This function is used to determine whether the specified device is a
 * detachable storage device
 *
 * @param[in] device_handle specifies the handle to the storage device
 * @param[in] value specifies a pointer to a boolean result that, on output, will
 * indicate true if the specified device is a detachable storage device. False otherwise.
 *
 * @retval RMF_SUCCESS if no error
 * @retval RMF_OSAL_STORAGE_ERR_INVALID_PARAM otherwise
 */
rmf_Error rmf_osal_storageIsDetachable(rmf_osal_StorageHandle device_handle,
        rmf_osal_Bool* value)
{
    volatile rmf_Error result = RMF_OSAL_STORAGE_ERR_OUT_OF_STATE;
    rmf_osal_Storage * device = (rmf_osal_Storage*)device_handle;


    if (!value || !isValidDevice(device))
    {
        result = RMF_OSAL_STORAGE_ERR_INVALID_PARAM;
    }
    else
    {
        if (device->type & RMF_OSAL_STORAGE_TYPE_DETACHABLE)
            *value = TRUE;
        else
            *value = FALSE;

        result = RMF_SUCCESS;

        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "<<STORAGE>> %s (%s) - %s\n",
                __FUNCTION__, device->displayName, *value ? "TRUE" : "FALSE");
    }

    return result;
}

/**
 * @brief This function makes the specified device ready to be safely detached.
 * Any pending I/O operations on this device are immediately cancelled.
 * 
 * This call will result in the device transitioning to the
 * RMF_OSAL_STORAGE_STATUS_OFFLINE state if it is capable of being brought
 * back online without physically detaching and reattaching the device.
 * Otherwise, the device will be removed from the device list and an
 * RMF_OSAL_EVENT_STORAGE_DETACHED event will be delivered to the registered queue.
 * Further I/O attempts on this device while in this state will fail until the
 * device is reattached or brought back online via the rmf_osal_storageMakeReadyForUse() API.
 *
 * <b> Events: </b>
 * RMF_OSAL_EVENT_STORAGE_CHANGED - triggered if device is successfully taken offline
 *                                and is capable of being brought back online without
 *                                physically detaching and reattaching the device.
 *                                This event is also triggered if the device enters
 *                                an unrecoverable error state.
 *
 * RMF_OSAL_EVENT_STORAGE_DETACHED - triggered if device is successfully made ready to
 *                                detach and is incapable of being brought back online
 *                                without physically detaching and reattaching the device.
 *
 * <b> S-A platform implementation notes: </b>
 * This function will be implemented as a stub that always returns
 * RMF_OSAL_STORAGE_ERR_UNSUPPORTED.  The reason is because no detachable storage devices
 * will be presented to apps on the target S-A platforms.
 *
 * @param[in] device_handle specifies a handle to the storage device
 *
 * @retval RMF_SUCCESS if success
 * @retval RMF_OSAL_STORAGE_ERR_INVALID_PARAM if specified invalid device
 * @retval RMF_OSAL_STORAGE_ERR_UNSUPPORTED if specified non-detachable device
 * @retval RMF_OSAL_STORAGE_ERR_DEVICE_ERR if unable to make the device safe to detach
 */
rmf_Error rmf_osal_storageMakeReadyToDetach(rmf_osal_StorageHandle device_handle)
{
    volatile rmf_Error result = RMF_OSAL_STORAGE_ERR_OUT_OF_STATE;
    rmf_osal_Storage * device = (rmf_osal_Storage*)device_handle;

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE",
            "<<STORAGE>> rmf_osal_storageMakeReadyToDetach\n");

    if (!isValidDevice(device) || !(device->type
            & RMF_OSAL_STORAGE_TYPE_DETACHABLE))
    {
        result = RMF_OSAL_STORAGE_ERR_INVALID_PARAM;
    }
    else
    {
        rmf_osal_mutexAcquire(device->mutex);
        // <STMGR-COMP> Disable/remove any logical storage/media storage volumes
        device->status = RMF_OSAL_STORAGE_STATUS_OFFLINE;
        result = ri_storageMakeReadyToDetach(((rmf_osal_Storage*) device)->rootPath);

        rmf_osal_priv_sendStorageMgrEvent(device, RMF_OSAL_EVENT_STORAGE_CHANGED,
                RMF_OSAL_STORAGE_STATUS_OFFLINE);

        rmf_osal_mutexRelease(device->mutex);
    }

    return result;
}

/**
 * @brief This function makes the specified detachable storage device ready to be used
 * after having previously been made ready to detach. 
 * 
 * If the specified device is in the RMF_OSAL_STORAGE_STATUS_OFFLINE state, this call 
 * attempts to activate the device and make it available as if it was newly attached.
 * As a result of this call, the device will transition to one of the following states.
 * <br> RMF_OSAL_STORAGE_STATUS_READY
 * <br> RMF_OSAL_STORAGE_STATUS_DEVICE_ERR
 * <br> RMF_OSAL_STORAGE_STATUS_UNINITIALIZED
 * <br> RMF_OSAL_STORAGE_STATUS_UNSUPPORTED_DEVICE
 * <br> RMF_OSAL_STORAGE_STATUS_UNSUPPORTED_FORMAT
 * 
 * This function will not format an unformatted device and will not reformat a
 * device with an unsupported format.
 *
 * Events
 *    RMF_OSAL_EVENT_STORAGE_CHANGED - device changed states
 *
 * @param[in] device_handle specifies a handle to the storage device
 *
 * @return RMF_SUCCESS if device is ready for use.
 * @retval RMF_OSAL_STORAGE_ERR_INVALID_PARAM if specified invalid device
 * @retval RMF_OSAL_STORAGE_ERR_UNSUPPORTED if specified non-detachable device
 * @retval RMF_OSAL_STORAGE_ERR_DEVICE_ERR if unable to make the device ready for use
 */
rmf_Error rmf_osal_storageMakeReadyForUse(
                rmf_osal_StorageHandle device_handle)
{
	volatile rmf_Error result = RMF_OSAL_STORAGE_ERR_OUT_OF_STATE;
	rmf_osal_Storage * device = (rmf_osal_Storage*)device_handle;

	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE",
			"<<STORAGE>> rmf_osal_storageMakeReadyForUse\n");
	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE",
			"<<STORAGE>> RMF_OSAL_STORAGE_STATUS = %d \n", device->status);

	if (!isValidDevice(device))
	{
		result = RMF_OSAL_STORAGE_ERR_INVALID_PARAM;
	}
	else if ((device->status != RMF_OSAL_STORAGE_STATUS_NOT_PRESENT)
			&& (device->status != RMF_OSAL_STORAGE_STATUS_READY) && (device->status
			!= RMF_OSAL_STORAGE_STATUS_OFFLINE))
	{
		result = RMF_OSAL_STORAGE_ERR_DEVICE_ERR;
	}
	else
	{

		rmf_osal_mutexAcquire(device->mutex);

		if (mount(device->name, device->rootPath, device->mnt_type, 0, NULL) < 0)
		{
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE"," Error in mounting the device %s: %s\n", device->rootPath, strerror(errno));
			result = RMF_OSAL_STORAGE_ERR_DEVICE_ERR;
			device->status = RMF_OSAL_STORAGE_STATUS_UNINITIALIZED;
		}
		else
		{
			result = RMF_SUCCESS;
			device->status = RMF_OSAL_STORAGE_STATUS_READY;
			RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","  Success in mounting the device \n");
		}
		rmf_osal_priv_sendStorageMgrEvent(device, RMF_OSAL_EVENT_STORAGE_CHANGED, device->status);

		rmf_osal_mutexRelease(device->mutex);
	}

	return result;
}

/**
 * @brief This function is used to determine whether the specified device is capable
 * of storing DVR content within a media storage device.
 *
 * @param[in] device_handle specifies a handle to the storage device
 * @param[out] value is set to TRUE if a valid device found to be connected
 * @retval RMF_SUCCESS if no error
 * @retval RMF_OSAL_STORAGE_ERR_INVALID_PARAM otherwise
 */
rmf_Error rmf_osal_storageIsDvrCapable(rmf_osal_StorageHandle device_handle,
        rmf_osal_Bool* value)
{

    //  Add "NULL" value for mediafs size and then return false if there is no mediafs

    volatile rmf_Error result = RMF_OSAL_STORAGE_ERR_OUT_OF_STATE;
    rmf_osal_Storage * device = (rmf_osal_Storage*)device_handle;

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE",
            "<<STORAGE>> rmf_osal_storageIsDvrCapable\n");

    if (!value || !isValidDevice(device))
    {
        result = RMF_OSAL_STORAGE_ERR_INVALID_PARAM;
    }
    else
    {
        *value = TRUE;
        result = RMF_SUCCESS;
    }

    return result;
}

/**
 * @brief This function is used to determine whether the specified device is capable of
 * housing removable storage media.
 *
 * <b> S-A platform implementation notes: </b> <br>
 * This function will be implemented as a stub that always indicates FALSE and
 * returns RMF_SUCCESS.  The reason is because no removable storage
 * devices will be presented to apps on the target S-A platforms.
 *
 * @param[in] device_handle specifies a handle to the storage device
 * @param[out] value is a pointer to a boolean result that, on output, indicates true
 *              if the specified device is a physical bay that can house
 *              removable storage media.  Otherwise, false.
 *
 * @retval RMF_SUCCESS if no error
 * @retval RMF_OSAL_STORAGE_ERR_INVALID_PARAM otherwise
 */
rmf_Error rmf_osal_storageIsRemovable(rmf_osal_StorageHandle device_handle,
        rmf_osal_Bool* value)
{
    volatile rmf_Error result = RMF_OSAL_STORAGE_ERR_OUT_OF_STATE;
    rmf_osal_Storage * device = (rmf_osal_Storage*)device_handle;

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE",
            "<<STORAGE>> rmf_osal_storageIsRemovable\n");

    if (!value || !isValidDevice(device))
    {
        result = RMF_OSAL_STORAGE_ERR_INVALID_PARAM;
    }
    else
    {
        if (device->type & RMF_OSAL_STORAGE_TYPE_REMOVABLE)
            *value = TRUE;
        else
            *value = FALSE;

        result = RMF_SUCCESS;
    }

    return result;
}

/**
 * @brief This function is used to remove the media storage device to be physically ejected
 * from its bay if applicable.  If this operation is not applicable to the
 * storage device, this function does nothing and returns success.
 *
 * No support for removable devices currently exists.  Therefore, always
 * return RMF_OSAL_STORAGE_ERR_UNSUPPORTED.
 *
 * @param[in] device_handle specifies a handle to the storage device
 *
 * @retval RMF_SUCCESS if success or no op
 * @retval RMF_OSAL_STORAGE_ERR_INVALID_PARAM if specified invalid parameter
 * @retval RMF_OSAL_STORAGE_ERR_UNSUPPORTED if specified device is not capable of
 *         housing removable media
 *
 */
rmf_Error rmf_osal_storageEject(rmf_osal_StorageHandle device_handle)
{
    volatile rmf_Error result = RMF_OSAL_STORAGE_ERR_OUT_OF_STATE;
    rmf_osal_Storage * device = (rmf_osal_Storage*)device_handle;

    //  <STMGR-COMP> Remove/disable any logical storage/media storage volumes

    if (!isValidDevice(device))
    {
        result = RMF_OSAL_STORAGE_ERR_INVALID_PARAM;
    }
    else
	{
		result = ri_storageEject(((rmf_osal_Storage*)device)->rootPath);
		if (RMF_SUCCESS == result)
		{
			rmf_osal_mutexAcquire(device->mutex);
			device->size = 0;
			device->defaultMediaFsSize = 0;
			device->mediaFsSize = 0;
        	device->status = RMF_OSAL_STORAGE_STATUS_NOT_PRESENT;

        	rmf_osal_priv_sendStorageMgrEvent(device, RMF_OSAL_EVENT_STORAGE_CHANGED, device->status);

        	rmf_osal_mutexRelease(device->mutex);
		}
    }

    return result;
}

/**
 * @brief This function is used to initialize the specified storage device for use.
 * 
 * It is usually called on a newly attached storage device that is not currently
 * suitable for use (i.e., UNSUPPORTED_FORMAT or UNINITIALIZED), but may also
 * be called to reformat or repartition a storage device that is in the READY
 * state.  On successful invocation, the state of the device will enter the
 * RMF_OSAL_STORAGE_STATUS_BUSY state while initialization is in progress then will
 * enter one of the following final states when the operation is complete
 *    RMF_OSAL_STORAGE_STATUS_READY
 *    RMF_OSAL_STORAGE_STATUS_DEVICE_ERR
 *
 * An RMF_OSAL_EVENT_STORAGE_CHANGED event will be delivered to the registered queue
 * when the operation is initiated and again when it is complete.
 *
 * @param[in] device_handle specifies the storage device to (re)initialize
 * @param[in] userAuthorized is a Boolean indicating whether the (re)initialize operation
 *        was explicitly authorized by the end user.
 * @param[in] mediaFsSize is a pointer to uint64_t indicating the minimum size of the
 *        MEDIAFS partition requested for this device in bytes.  A value of zero
 *        bytes indicates that no minimum MEDIAFS size is requested.  A non-NULL
 *        value indicates that the caller is requesting the device to be repartitioned.
 *        The effects of repartitioning the device may include loss of application
 *        data on this device (On S-A platforms, repartitioning the device will
 *        definitely result in loss of all data  stored on the device).  A NULL
 *        value indicates that the caller is requesting the device to be formatted
 *        which will result in loss of all application data stored on the device
 *        (On S-A platforms, reformatting the device will result in loss of all
 *        data stored on this device).  If the device is in the READY state and
 *        NULL is specified, this call will reformat each partition without
 *        repartitioning the device.  If the device is in the UNINITIALIZED or
 *        UNSUPPORTED_FORMAT state and NULL is specified, the device will be
 *        partitioned with platform-specific default sizes for MEDIAFS and GPFS.
 *
 * @retval RMF_SUCCESS if operation was initiated successfully
 * @retval RMF_OSAL_STORAGE_ERR_INVALID_PARAM if specified invalid device or mediaFsSize
 *              greater than storage capacity of device or non-zero mediaFsSize for
 *              non-DVR capable storage device.
 * @retval RMF_OSAL_STORAGE_ERR_BUSY if initialization is already in progress for this device
 * @retval RMF_OSAL_STORAGE_ERR_UNSUPPORTED if the specified storage device is an unsupported device
 * @retval RMF_OSAL_STORAGE_ERR_DEVICE_ERR if failed to initialize the device for use
 * @retval RMF_OSAL_STORAGE_ERR_OUT_OF_STATE if device is not in a valid state for
 *              partitioning or formatting (e.g. OFFLINE, NOT_PRESENT, UNSUPPORTED_DEVICE)
 */
rmf_Error rmf_osal_storageInitializeDevice(rmf_osal_StorageHandle device_handle,
        rmf_osal_Bool userAuthorized, uint64_t* mediaFsSize)
{
    volatile rmf_Error result = RMF_SUCCESS;
    rmf_osal_Storage * device = (rmf_osal_Storage*)device_handle;

    RMF_OSAL_UNUSED_PARAM(userAuthorized);

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE",
            "<<STORAGE>> rmf_osal_storageInitializeDevice\n");

    if (!isValidDevice(device))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE"," %s: Not valid device\n", __FUNCTION__);
        result = RMF_OSAL_STORAGE_ERR_INVALID_PARAM;
    }
    else if (device->status == RMF_OSAL_STORAGE_STATUS_BUSY)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE"," %s: Device Busy\n", __FUNCTION__);
        result = RMF_OSAL_STORAGE_ERR_BUSY;
    }
    else if (device->status == RMF_OSAL_STORAGE_STATUS_OFFLINE)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.STORAGE"," %s: Device offline\n", __FUNCTION__);
        result = RMF_OSAL_STORAGE_ERR_OUT_OF_STATE;
    }
    else
    {
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE","<<STORAGE>: %s: Yet To Implement ...\n",__FUNCTION__);
        rmf_osal_priv_sendStorageMgrEvent(device, RMF_OSAL_EVENT_STORAGE_CHANGED, device->status);
    }
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.STORAGE", "<<STORAGE>> %s Exiting ... \n", __FUNCTION__);

    return result;
}

