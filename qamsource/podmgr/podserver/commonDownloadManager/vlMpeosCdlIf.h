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
#ifndef __VLMPEOSCDLIF_H__
#define __VLMPEOSCDLIF_H__
/*-----------------------------------------------------------------*/

//#include "jni.h"

#define VL_MAX_CDL_STR_SIZE     (256)

/*-----------------------------------------------------------------*/
typedef enum _VLMPEOS_CDL_RESULT
{
    VLMPEOS_CDL_RESULT_SUCCESS               = 0,
    VLMPEOS_CDL_RESULT_NOT_IMPLEMENTED       = 1,
    VLMPEOS_CDL_RESULT_NULL_PARAM            = 2,
    VLMPEOS_CDL_RESULT_INVALID_PARAM         = 3,
    VLMPEOS_CDL_RESULT_INCOMPLETE_PARAM      = 4,
    VLMPEOS_CDL_RESULT_INVALID_IMAGE_TYPE    = 5,
    VLMPEOS_CDL_RESULT_INVALID_IMAGE_SIGN    = 6,
    VLMPEOS_CDL_RESULT_ECM_FAILURE           = 7,
    VLMPEOS_CDL_RESULT_UPGRADE_FAILED        = 8,
    VLMPEOS_CDL_RESULT_MODULE_BUSY           = 9,
    VLMPEOS_CDL_RESULT_IMAGE_VERIFICATION_FAILED = 10,

    VLMPEOS_CDL_RESULT_INVALID               = 0xFF
} VLMPEOS_CDL_RESULT;

/*-----------------------------------------------------------------*/
typedef enum _VLMPEOS_CDL_DOWNLOAD_STATUS
{
    VLMPEOS_CDL_DOWNLOAD_STATUS_IDLE                             = 0,
    VLMPEOS_CDL_DOWNLOAD_STATUS_UNKNOWN                          = 1,
    VLMPEOS_CDL_DOWNLOAD_STATUS_MANAGER_DOWNLOADING              = 2,
    VLMPEOS_CDL_DOWNLOAD_STATUS_ECM_DOWNLOADING                  = 3,
    VLMPEOS_CDL_DOWNLOAD_STATUS_UPGRADING                        = 4,
    VLMPEOS_CDL_DOWNLOAD_STATUS_REBOOTING                        = 5,
    VLMPEOS_CDL_DOWNLOAD_STATUS_BOOTUP_AFTER_SUCCESSFUL_UPGRADE  = 6,
    VLMPEOS_CDL_DOWNLOAD_STATUS_BOOTUP_AFTER_FAILED_UPGRADE      = 7,
    
    VLMPEOS_CDL_DOWNLOAD_STATUS_INVALID                          = 0x7FFFFFFF

} VLMPEOS_CDL_DOWNLOAD_STATUS;

/*-----------------------------------------------------------------*/
typedef struct _VLMPEOS_CDL_STATUS_QUESTION
{
    VLMPEOS_CDL_DOWNLOAD_STATUS eDownloadStatus;

} VLMPEOS_CDL_STATUS_QUESTION;

/*-----------------------------------------------------------------*/
typedef enum _VLMPEOS_CDL_IMAGE_TYPE
{
    VLMPEOS_CDL_IMAGE_TYPE_MONOLITHIC    = 0,
    VLMPEOS_CDL_IMAGE_TYPE_FIRMWARE      = 1,
    VLMPEOS_CDL_IMAGE_TYPE_APPLICATION   = 2,
    VLMPEOS_CDL_IMAGE_TYPE_DATA          = 3,
    
    VLMPEOS_CDL_IMAGE_TYPE_ECM           = 0x100,
    VLMPEOS_CDL_IMAGE_TYPE_INVALID       = 0x7FFFFFFF
} VLMPEOS_CDL_IMAGE_TYPE;

/*-----------------------------------------------------------------*/
typedef enum _VLMPEOS_CDL_IMAGE_SIGN
{
    VLMPEOS_CDL_IMAGE_SIGN_NOT_SPECIFIED,
    VLMPEOS_CDL_IMAGE_SIGNED,
    VLMPEOS_CDL_IMAGE_UNSIGNED,

} VLMPEOS_CDL_IMAGE_SIGN;

/*-----------------------------------------------------------------*/
typedef struct _VLMPEOS_CDL_IMAGE
{
    VLMPEOS_CDL_IMAGE_TYPE   eImageType;
    VLMPEOS_CDL_IMAGE_SIGN   eImageSign;
    char                szSoftwareImageName[VL_MAX_CDL_STR_SIZE]; // name to be stored in NVRAM
    char *              pszImagePathName;
} VLMPEOS_CDL_IMAGE;

/*-----------------------------------------------------------------*/
typedef struct _VLMPEOS_CDL_SOFTWARE_IMAGE_NAME
{
    char                szSoftwareImageName[VL_MAX_CDL_STR_SIZE];
} VLMPEOS_CDL_SOFTWARE_IMAGE_NAME;

/*-----------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif
/*-----------------------------------------------------------------*/

VLMPEOS_CDL_RESULT vlMpeosCdlGetCurrentBootImageName(VLMPEOS_CDL_SOFTWARE_IMAGE_NAME * pImageName);
VLMPEOS_CDL_RESULT vlMpeosCdlGetStatus(VLMPEOS_CDL_STATUS_QUESTION * pStatusQuestion);
VLMPEOS_CDL_RESULT vlMpeosCdlUpgradeToImageAndReboot(VLMPEOS_CDL_IMAGE * pImage, int bRebootNow);
//VLMPEOS_CDL_RESULT vlMpeosCdlSetJvmHandle(JavaVM *pJvm);

/*-----------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/*-----------------------------------------------------------------*/
#endif // __VLMPEOSCDLIF_H__
/*-----------------------------------------------------------------*/
