/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2011 RDK Management
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

 
/*-----------------------------------------------------------------*/
#ifndef __VL_CDL_TYPES_H__
#define __VL_CDL_TYPES_H__
/*-----------------------------------------------------------------*/

/**
 * @file vl_cdl_types.h
 *
 * @defgroup HAL_CDL_Types  CDL Data Types
 * @ingroup  RMF_HAL_Types
 * @{
 */

#define VL_MAX_CDL_DEVICES      (1)
#define VL_CDL_DEVICE_INSTANCE  (0)
#define VL_MAX_CDL_STR_SIZE     (256)

/*-----------------------------------------------------------------*/
#define VL_CDL_GET_STRUCT(datatype, pStruct, voidptr)    datatype * pStruct = ((datatype*)(voidptr))
#define VL_CDL_EVENT(eventdir, event)                    ((eventdir)|(event))
#define VL_IP_ADDR_SIZE         4

/*-----------------------------------------------------------------*/
typedef void *VL_CDL_HANDLE;          //!< handle used by the application to access the API
typedef void *VL_CDL_INSTANCEHANDLE;  //!<  handle used by the API to access the driver instance (never used by the application)
typedef unsigned long VL_CDL_DEVICE_HANDLE_t;

/*-----------------------------------------------------------------*/
/**
 * @brief Instances of CDL Capabilities.
 */
typedef struct VL_CDL_CAPABILITIES_INSTANCE_s
{
    VL_CDL_DEVICE_HANDLE_t hCDLHandle;

} VL_CDL_CAPABILITIES_INSTANCE_t;

/*-----------------------------------------------------------------*/
/**
 * @brief CDL Capabilities.
 */
typedef struct VL_CDL_CAPABILITIES_s
{
    int                             usNumCDLs;
    VL_CDL_CAPABILITIES_INSTANCE_t  astInstanceCapabilities[VL_MAX_CDL_DEVICES];
} VL_CDL_CAPABILITIES_t;

/*-----------------------------------------------------------------*/
/**
 *  @brief CDL notification type.
 */
typedef enum
{
    VL_CDL_NOTIFY_NONE = 0,
    VL_CDL_NOTIFY_DRIVER_EVENT,

    VL_CDL_NOTIFY_MAX_TYPES,
    VL_CDL_NOTIFY_INVALID = 0xFF,

} VL_CDL_NOTIFY_TYPE_t;

/*******************************************************************/
/**
 *  @brief CDL event type.
 */
typedef enum _VL_CDL_COMMON_EVENT_TYPE
{
    VL_CDL_EVENT_NONE                       = 0x00000000,

    // event directions
    VL_CDL_MANAGER_EVENT                    = 0x10000000,
    VL_CDL_DRIVER_EVENT                     = 0x20000000,

    VL_CDL_EVENT_INVALID                    = 0x7FFFFFFF

} VL_CDL_COMMON_EVENT_TYPE;

/*-----------------------------------------------------------------*/
/** Events sent from CDL manager to hal CDL */
typedef enum _VL_CDL_MANAGER_EVENT_TYPE
{
    VL_CDL_MANAGER_EVENT_SET_CV_CERTIFICATE,
    VL_CDL_MANAGER_EVENT_SET_PUBLIC_KEY,

    VL_CDL_MANAGER_EVENT_START_DOWNLOAD,

    VL_CDL_MANAGER_EVENT_VALIDATE_CV_TABLE,
    VL_CDL_MANAGER_EVENT_VALIDATE_IMAGE_SIGNATURE,
    VL_CDL_MANAGER_EVENT_VALIDATE_IMAGE_CONTENT,

    VL_CDL_MANAGER_EVENT_UPGRADE_TO_IMAGE,
    VL_CDL_MANAGER_EVENT_SET_UPGRADE_SUCCEEDED,
    VL_CDL_MANAGER_EVENT_SET_UPGRADE_FAILED,
	VL_CDL_MANAGER_EVENT_SAVE_UPGRADE_IMAGE_NAME,

    VL_CDL_MANAGER_EVENT_get_PREVIOUS_IMAGE_NAME, // deprecated
    VL_CDL_MANAGER_EVENT_get_CURRENT_BOOT_IMAGE_NAME,
    VL_CDL_MANAGER_EVENT_get_UPGRADE_IMAGE_NAME,
    VL_CDL_MANAGER_EVENT_get_UPGRADE_STATUS,

    VL_CDL_MANAGER_EVENT_REBOOT,

    VL_CDL_MANAGER_EVENT_DOWNLOAD_STARTED,
    VL_CDL_MANAGER_EVENT_DOWNLOAD_COMPLETED,
    VL_CDL_MANAGER_EVENT_DOWNLOAD_FAILED,

    VL_CDL_MANAGER_EVENT_IMAGE_AUTH_FAIL,
    VL_CDL_MANAGER_EVENT_IMAGE_AUTH_SUCCESS,

    VL_CDL_MANAGER_EVENT_get_DOWNLOAD_PROGRESS,
            
} VL_CDL_MANAGER_EVENT_TYPE;

/*-----------------------------------------------------------------*/
/** Events sent from hal CDL to CDL manager */
typedef enum _VL_CDL_DRIVER_EVENT_TYPE
{
    VL_CDL_DRIVER_EVENT_IS_DOWNLOAD_PERMITTED,
    VL_CDL_DRIVER_EVENT_IS_VALID_IMAGE_HEADER,
    VL_CDL_DRIVER_EVENT_IS_VALID_FULL_IMAGE,
    VL_CDL_DRIVER_EVENT_DOWNLOAD_STARTED,
    VL_CDL_DRIVER_EVENT_DOWNLOAD_COMPLETED,
    VL_CDL_DRIVER_EVENT_IMAGE_DOWNLOAD_FAILED,
    VL_CDL_DRIVER_EVENT_IMAGE_AUTH_FAIL,
    VL_CDL_DRIVER_EVENT_IMAGE_AUTH_SUCCESS,
    VL_CDL_DRIVER_EVENT_UPGRADE_STARTED,
    VL_CDL_DRIVER_EVENT_UPGRADE_COMPLETED,
    VL_CDL_DRIVER_EVENT_UPGRADE_FAILED,
    VL_CDL_DRIVER_EVENT_REBOOTING_AFTER_UPGRADE,
    VL_CDL_DRIVER_EVENT_IMAGE_DOWNLOAD_PROGRESS,
    VL_CDL_DRIVER_EVENT_STARTING_DOWNLOAD_IMAGE_NAME,
    VL_CDL_DRIVER_EVENT_FINISHED_UPGRADE_IMAGE_NAME

} VL_CDL_DRIVER_EVENT_TYPE;

/*-----------------------------------------------------------------*/
/** CDL buffer */
typedef struct _VL_CDL_BUFFER
{
    int nBytes;
    unsigned char * pData;
} VL_CDL_BUFFER;

/*-----------------------------------------------------------------*/
/** Code Download status */
typedef enum _VL_CDL_RESULT
{
    VL_CDL_RESULT_SUCCESS               = 0,
    VL_CDL_RESULT_NOT_IMPLEMENTED       = 1,
    VL_CDL_RESULT_NULL_PARAM            = 2,
    VL_CDL_RESULT_INVALID_PARAM         = 3,
    VL_CDL_RESULT_INCOMPLETE_PARAM      = 4,
    VL_CDL_RESULT_INVALID_IMAGE_TYPE    = 5,
    VL_CDL_RESULT_INVALID_IMAGE_SIGN    = 6,
    VL_CDL_RESULT_ECM_FAILURE           = 7,
    VL_CDL_RESULT_UPGRADE_FAILED        = 8,
    VL_CDL_RESULT_ECM_BUSY_DOWNLOADING  = 9,

    VL_CDL_RESULT_INVALID               = 0xFF
} VL_CDL_RESULT;

/*-----------------------------------------------------------------*/
typedef VL_CDL_RESULT ( *VL_CDL_DRIVER_EVENT_CBFUNC_t)(void * pAppData, VL_CDL_DRIVER_EVENT_TYPE eCdlEventType, int nEventData);
typedef void          ( *VL_CDL_VOID_CBFUNC_t        )(void);

/*******************************************************************/
/** Code Verification Certificate type */
typedef enum _VL_CDL_CVC_TYPE
{
    VL_CDL_CVC_TYPE_NONE            = 0,
    VL_CDL_CVC_TYPE_MANUFACTURER,
    VL_CDL_CVC_TYPE_CO_SIGNER,

    VL_CDL_CVC_TYPE_INVALID         = 0xFF

} VL_CDL_CVC_TYPE;

/*******************************************************************/

/** Represents how download is triggered */

typedef enum _VL_CDL_TRIGGER_TYPE
{
    VL_CDL_TRIGGER_TYPE_NONE            = -1,
    VL_CDL_TRIGGER_TYPE_ECM_CONFIG_FILE =  0,
    VL_CDL_TRIGGER_TYPE_ECM_SNMP            ,
    VL_CDL_TRIGGER_TYPE_STB_CVT             ,

    VL_CDL_TRIGGER_TYPE_INVALID         = 0xFF

} VL_CDL_TRIGGER_TYPE;

/*-----------------------------------------------------------------*/
/** CDL status */
typedef enum _VL_CDL_DOWNLOAD_STATUS
{
    VL_CDL_DOWNLOAD_STATUS_IDLE                             = 0,
    VL_CDL_DOWNLOAD_STATUS_UNKNOWN                          = 1,
    VL_CDL_DOWNLOAD_STATUS_MANAGER_DOWNLOADING              = 2,
    VL_CDL_DOWNLOAD_STATUS_ECM_DOWNLOADING                  = 3,
    VL_CDL_DOWNLOAD_STATUS_UPGRADING                        = 4,
    VL_CDL_DOWNLOAD_STATUS_REBOOTING                        = 5,
    VL_CDL_DOWNLOAD_STATUS_BOOTUP_AFTER_SUCCESSFUL_UPGRADE  = 6,
    VL_CDL_DOWNLOAD_STATUS_BOOTUP_AFTER_FAILED_UPGRADE      = 7,
    
    VL_CDL_DOWNLOAD_STATUS_INVALID                          = 0x7FFFFFFF

} VL_CDL_DOWNLOAD_STATUS;

/*-----------------------------------------------------------------*/
/**
 * @brief CDL failure reason.
 */
typedef enum _VL_CDL_DOWNLOAD_FAIL_REASON
{
    VL_CDL_DOWNLOAD_FAIL_REASON_NONE            = 0,
    VL_CDL_DOWNLOAD_FAIL_REASON_FILE_NOT_FOUND  = 1,
    VL_CDL_DOWNLOAD_FAIL_REASON_TRANSFER_FAILED = 2,
    
    VL_CDL_DOWNLOAD_FAIL_REASON_INVALID         = 0x7FFFFFFF

} VL_CDL_DOWNLOAD_FAIL_REASON;

/*-----------------------------------------------------------------*/
/**
 * @brief Code Download status.
 */
typedef struct _VL_CDL_DOWNLOAD_STATUS_QUESTION
{
    VL_CDL_DOWNLOAD_STATUS eDownloadStatus;

} VL_CDL_DOWNLOAD_STATUS_QUESTION;

/*-----------------------------------------------------------------*/


/*-----------------------------------------------------------------*/

/**
 * @brief Code Verification Certificate.
 * Refers to the certificate used to verify the signature on a firmware image
 * to authenticate the image and check for image consistency.
 */
typedef struct _VL_CDL_CV_TABLE
{
    VL_CDL_BUFFER   cvTable;
} VL_CDL_CV_TABLE;

/*-----------------------------------------------------------------*/

/**
 * @brief Signature of the image.
 */

typedef struct _VL_CDL_IMAGE_SIGNATURE
{
    VL_CDL_BUFFER   imageSignature;
} VL_CDL_IMAGE_SIGNATURE;

/*-----------------------------------------------------------------*/
/**
 * @brief  CDL Image types.
 */
typedef enum _VL_CDL_IMAGE_TYPE
{
    VL_CDL_IMAGE_TYPE_MONOLITHIC    = 0,
    VL_CDL_IMAGE_TYPE_FIRMWARE      = 1,
    VL_CDL_IMAGE_TYPE_APPLICATION   = 2,
    VL_CDL_IMAGE_TYPE_DATA          = 3,
    
    VL_CDL_IMAGE_TYPE_ECM           = 0x100,
    VL_CDL_IMAGE_TYPE_INVALID       = 0x7FFFFFFF
} VL_CDL_IMAGE_TYPE;

/*-----------------------------------------------------------------*/
/**
 * @brief  CDL Image signature status.
 */
typedef enum _VL_CDL_IMAGE_SIGN
{
    VL_CDL_IMAGE_SIGN_NOT_SPECIFIED,
    VL_CDL_IMAGE_SIGNED,
    VL_CDL_IMAGE_UNSIGNED,

} VL_CDL_IMAGE_SIGN;

/*-----------------------------------------------------------------*/
/**
 * @brief Saves the information about the CDL query Is download permitted?.
 */
typedef struct _VL_CDL_QUESTION_IS_DOWNLOAD_PERMITTED
{
    VL_CDL_TRIGGER_TYPE eTriggerType;
    VL_CDL_IMAGE_TYPE   eImageType;
    VL_CDL_IMAGE_SIGN   eImageSign;
    unsigned char       bIsDownloadPermitted;
    int                 nFileNameSize;
    char                szSoftwareImageName[VL_MAX_CDL_STR_SIZE]; // name to be stored in NVRAM

} VL_CDL_QUESTION_IS_DOWNLOAD_PERMITTED;

/*-----------------------------------------------------------------*/
/**
 * @brief Image header information.
 */
typedef struct _VL_CDL_IMAGE_HEADER
{
    VL_CDL_TRIGGER_TYPE eTriggerType;
    VL_CDL_IMAGE_TYPE   eImageType;
    VL_CDL_IMAGE_SIGN   eImageSign;
    VL_CDL_BUFFER       header;
} VL_CDL_IMAGE_HEADER;

/*-----------------------------------------------------------------*/
/**
 * @brief Saves the information about the CDL query Is valid image header?.
 */
typedef struct _VL_CDL_QUESTION_IS_VALID_IMAGE_HEADER
{
    VL_CDL_IMAGE_HEADER imageHeader;
    unsigned char bIsValidImageHeader;

} VL_CDL_QUESTION_IS_VALID_IMAGE_HEADER;

/*-----------------------------------------------------------------*/
/**
 * @brief Indicates the CDL progress notification.
 */
typedef struct _VL_CDL_DOWNLOAD_PROGRESS_NOTIFICATION
{
    VL_CDL_TRIGGER_TYPE eTriggerType;
    VL_CDL_IMAGE_TYPE   eImageType;
    VL_CDL_IMAGE_SIGN   eImageSign;
    int                 nBytesDownloaded;
    
} VL_CDL_DOWNLOAD_PROGRESS_NOTIFICATION;

/*-----------------------------------------------------------------*/
/**
 * @brief Indicates the CDL fail notification.
 */
typedef struct _VL_CDL_DOWNLOAD_FAIL_NOTIFICATION
{
    VL_CDL_TRIGGER_TYPE             eTriggerType;
    VL_CDL_IMAGE_TYPE               eImageType;
    VL_CDL_IMAGE_SIGN               eImageSign;
    VL_CDL_DOWNLOAD_FAIL_REASON     eFailReason;
} VL_CDL_DOWNLOAD_FAIL_NOTIFICATION;

/*-----------------------------------------------------------------*/
/**
 * @brief Indicates the code download  details.
 */
typedef struct _VL_CDL_IMAGE
{
    VL_CDL_TRIGGER_TYPE eTriggerType;
    VL_CDL_IMAGE_TYPE   eImageType;
    VL_CDL_IMAGE_SIGN   eImageSign;
    char                szSoftwareImageName[VL_MAX_CDL_STR_SIZE]; // name to be stored in NVRAM
    char *              pszImagePathName;
    int                 nFileBytes;
} VL_CDL_IMAGE;

/*-----------------------------------------------------------------*/
/**
 * @brief CDL image name and its type.
 */
typedef struct _VL_CDL_SOFTWARE_IMAGE_NAME
{
    VL_CDL_IMAGE_TYPE   eImageType;
    char                szSoftwareImageName[VL_MAX_CDL_STR_SIZE];
} VL_CDL_SOFTWARE_IMAGE_NAME;

/*-----------------------------------------------------------------*/
/**
 * @brief Saves the code download params .
 */
typedef struct _VL_CDL_DOWNLOAD_PARAMS
{
    VL_CDL_IMAGE_TYPE   eImageType;
    VL_CDL_IMAGE_SIGN   eImageSign;
    unsigned char       tftpIpAddress[VL_IP_ADDR_SIZE];
    char *              pszImagePath;
    char *              pszImageName;
} VL_CDL_DOWNLOAD_PARAMS;

/*-----------------------------------------------------------------*/
/**
 * @brief Saves the code download image and its validity .
 */
typedef struct _VL_CDL_QUESTION_IS_VALID_IMAGE
{
    VL_CDL_IMAGE image;
    unsigned char bIsValidImage;

} VL_CDL_QUESTION_IS_VALID_IMAGE;

/**
 * @brief Stores the CDL Public key.
 */
typedef struct _VL_CDL_PUBLIC_KEY
{
    VL_CDL_BUFFER   publicKey;
} VL_CDL_PUBLIC_KEY;


/**
 * @brief Description about code verification certificate.
 */
typedef struct _VL_CDL_CV_CERTIFICATE
{
    VL_CDL_CVC_TYPE eCvcType;
    VL_CDL_BUFFER   cvCertificate;
} VL_CDL_CV_CERTIFICATE;

/** @} */
/*-----------------------------------------------------------------*/
#endif // __VL_CDL_TYPES_H__
/*-----------------------------------------------------------------*/
