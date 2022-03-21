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



#ifndef HAL_ERROR_H
#define HAL_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

/*===================================================================
   INCLUDES AND PUBLIC DATA DECLARATIONS
===================================================================*/

/*-------------------------------------------------------------------
   Defines/Macros
-------------------------------------------------------------------*/
/**
 * @defgroup HAL_Error_Info  Error Types
 * @ingroup  RMF_HAL_Types
 * @{
 */
/***************/
/* ERROR CODES */
/***************/

#define HAL_SUCCESS                             0      /*!< Success */

/**********************/
/** General HAL Errors */
/**********************/
#define ERROR_HAL_GENERAL                       -1     /*!< General Errors */

/** An unknown error occurred */
#define ERROR_HAL_UNKNOWN                       -2

/** Feature currently not supported (hardware or software) */
#define ERROR_HAL_NOT_SUPPORTED                 -3

/** Not initialized    */
#define ERROR_HAL_NOT_INITIALIZED               -4

/** Feature is supported, but the system was unable to find it (software only).
 (Example:  looking for a specific program name, but it is not found)   */
#define ERROR_HAL_NOT_FOUND                     -5

/** Feature is supported, but hardware was not found, or does not exist.
 (Example: card not inserted, no HDD, etc) */
#define ERROR_HAL_NO_HARDWARE                   -6

/** Out of Memory Error occurred */
#define ERROR_HAL_OUT_OF_MEMORY                 -8

/** A timeout has occurred */
#define ERROR_HAL_TIMEOUT                       -9

/** A NULL pointer was received as a parameter */
#define ERROR_HAL_NULL_PARAMETER                -10

/** An invalid Handle was received */
#define ERROR_HAL_INVALID_HANDLE                -11

/** The requested device is not available */
#define ERROR_HAL_DEVICE_NOT_AVAILABLE          -12

/** The requested Source is not available */
#define ERROR_HAL_SOURCE_NOT_AVAILABLE          -13

/** The device has not been "Requested" */
#define ERROR_HAL_NOT_REQUESTED                 -14

/** The specified device is not a "Source" device */
#define ERROR_HAL_INVALID_SOURCE_DEVICE         -15

/** The specified device is already connected, either to a source or as a source. */
#define ERROR_HAL_DEVICE_CONNECTED              -16

/** The specified device does not have a "Source" */
#define ERROR_HAL_NO_SOURCE                     -17

/** The specified device is "Stopped" */
#define ERROR_HAL_STOPPED                       -18

/** The specified device is "Started" (still running) */
#define ERROR_HAL_STARTED                       -19

/** There is no data available */
#define ERROR_HAL_NO_DATA                       -20

/** Initialization has failed */
#define ERROR_HAL_INITIALIZATION_FAILED         -21

/** Item has already been initialized */
#define ERROR_HAL_ALREADY_INITIALIZED           -22

/** Power-On Self Test has failed */
#define ERROR_HAL_POST_FAILED                   -23

/** The device has not been configured since being requested. */
#define ERROR_HAL_DEVICE_NOT_CONFIGURED         -24

/** PID must be in the range of 0 - 0x1FFE */
#define ERROR_HAL_INVALID_PID                   -25

/** Failed to allocate a PID channel */
#define ERROR_HAL_PID_CHAN_ALLOC_FAILED         -26

/** No mask set */
#define ERROR_HAL_NO_MASK                       -27

/** The device is currently locked by a semaphore */
#define ERROR_HAL_SEMAPHORED                    -28

/** The requested operation failed, most likely due to an underlying hardware problem. */
#define ERROR_HAL_OPERATION_FAILED              -29

/** A parameter is out of range. */
#define ERROR_HAL_OUT_OF_RANGE                  -30


/*************/
/** OS Errors */
/************/

#define ERROR_HAL_OS_GENERAL                    -100

/** Command to create a semaphore failed */
#define ERROR_HAL_OS_SEM_CREATE_FAILED          -101

/** Command to destroy semaphore failed */
#define ERROR_HAL_OS_SEM_DESTROY_FAILED         -102

/** Command to get a semaphore failed */
#define ERROR_HAL_OS_SEM_GET_FAILED             -103

/** Command to release a semaphore failed */
#define ERROR_HAL_OS_SEM_RELEASE_FAILED         -104


/*****************/
/** HAL_ANAAUD errors */
/*****************/

#define ERROR_ANAAUD_GENERAL                    -200



/*****************/
/** HAL_ANAVID errors */
/*****************/

#define ERROR_ANAVID_GENERAL                    -300


/*********************/
/** HAL_CRYPTO errors */
/*********************/

#define ERROR_CRYPTO_GENERAL                    -400


/*******************/
/** HAL_DISP errors */
/*******************/

#define ERROR_DISP_GENERAL                      -500

/** The specified display format is not supported by the display device */
#define ERROR_DISP_FORMAT_NOT_SUPPORTED         -501

/** The specified display aspect ratio is not supported by the display device */
#define ERROR_DISP_ASPECT_RATIO_NOT_SUPPORTED   -502

/** The type of video port is not valid for the display device */
#define ERROR_DISP_INVALID_VIDEO_PORT           -503

/** The type of audio supported is not valid for the display device */
#define ERROR_DISP_INVALID_AUDIO_PORT           -504

/** The type of video port is not valid for the display device */
#define ERROR_DISP_INVALID_AUDIO_VIDEO_PORT     -505

/** RF output takes its input from the composite signal. This display does not */
/* support composite. */
#define ERROR_DISP_COMPOSITE_NOT_AVAILABLE      -506

/** There are no more windows available                                                     */
#define ERROR_DISP_WINDOW_NOT_AVAILABLE         -507

/** Invalid SPDIF mode                                                     */
#define ERROR_DISP_SPDIF_MODE_NOT_SUPPORTED     -508

/** Invalid HDMI AUDIO mode                                                     */
#define ERROR_DISP_HDMI_AUDIO_MODE_NOT_SUPPORTED -509

/** Invalid operation while in PASSTHRU mode                                                     */
#define ERROR_DISP_INVALID_IN_PASSTHRU          -510

/** Invalid Macrovision type                                                     */
#define ERROR_DISP_MACROVISION_TYPE_NOT_SUPPORTED     -511


/******************/
/** HAL_EEPROM errors */
/******************/

#define ERROR_EEPROM_GENERAL                    -600

/** Invalid address offset for NVRAM */
#define ERROR_EEPROM_INVALID_OFFSET             -601



/******************/
/** HAL_FPD errors */
/******************/

#define ERROR_FPD_GENERAL                       -700

/** Hours must be in range 0 - 23, minutes must be in the range 0 - 59 */
#define ERROR_FPD_INVALID_TIME                  -702

/** Invalid characters for front panel display */
#define ERROR_FPD_INVALID_CHARS                 -703


/******************/
/** HAL_GFX errors */
/******************/

#define ERROR_GFX_GENERAL                       -800

/** The specified color format is not supported by the graphics surface */
#define ERROR_GFX_PIXELFORMAT_NOT_SUPPORTED     -801

/** No more graphic surfaces available */
#define ERROR_GFX_OUT_OF_SURFACES               -802

/** The graphics device is not connected to the display device */
#define ERROR_GFX_NOT_CONNECTED_TO_DISPLAY      -803

/** No more graphic palettes available */
#define ERROR_GFX_OUT_OF_PALETTES               -804


/******************/
/** HAL_HID errors */
/******************/

#define ERROR_HID_GENERAL                       -900


/******************/
/** HAL_IRB errors */
/******************/

#define ERROR_IRB_GENERAL                       -1000


/********************/
/** HAL_MPAUD errors */
/********************/

#define ERROR_MPAUD_GENERAL                     -1100

/** The specified audio type is not supported by this device */
#define ERROR_MPAUD_AUDIO_NOT_SUPPORTED         -1101


/********************/
/** HAL_MPENC errors */
/********************/

#define ERROR_MPENC_GENERAL                     -1200


/********************/
/** HAL_MPVID errors */
/********************/

#define ERROR_MPVID_GENERAL                     -1300

/** The specified video type is not supported by this device */
#define ERROR_MPVID_VIDEO_NOT_SUPPORTED         -1301


/******************/
/** HAL_OOB errors */
/******************/

#define ERROR_OOB_GENERAL                       -1400



/******************/
/** HAL_PCM errors */
/******************/

#define ERROR_PCM_GENERAL                       -1500



/******************/
/** HAL_POD errors */
/******************/

#define ERROR_POD_GENERAL                       -1600


/******************/
/** HAL_RECODE errors */
/******************/

#define ERROR_RECODE_GENERAL                    -1700



/**********************/
/** HAL_SECTFLT errors */
/**********************/

#define ERROR_SECTFLT_GENERAL                   -1800


/******************/
/** HAL_SYS errors */
/******************/

#define ERROR_SYS_GENERAL                       -1900


/********************/
/** HAL_TUNER errors */
/********************/

#define ERROR_TUNER_GENERAL                     -2000

/** Frequency is out of range */
#define ERROR_TUNER_FREQ_OUT_OF_RANGE           -2001

/** Symbol Rate is out of range */
#define ERROR_TUNER_SR_OUT_OF_RANGE             -2102

/** Specified tuner mode is not supported */
#define ERROR_TUNER_MODE_NOT_SUPPORTED          -2103


/********************/
/** HAL_VBI errors */
/********************/

#define ERROR_VBI_INVALID_LINE                  -2200

/** Invalid Fields */
#define ERROR_VBI_INVALID_FIELD                 -2201

/** Invalid Data type */
#define ERROR_VBI_INVALID_DATA_TYPE             -2202


/***********************/
/** Internal HAL errors */
/***********************/

/** The errors below are for internal use, and should never end up being */
/* returned to callers of the HAL. If it is returned, then this is a bug */
/* in the HAL implementation. */
#define ERROR_INTERNAL_GENERAL                         -20000
#define ERROR_INVALID_DESTINATION                      -20001
#define ERROR_PID_NOT_IN_USE                           -20002
#define ERROR_DEVICE_STILL_ACTIVE                      -20003
#define ERROR_DEVICE_NOT_CONNECTED                     -20004


/*-------------------------------------------------------------------
   Types/Structs
-------------------------------------------------------------------*/


/*-------------------------------------------------------------------
   Global Data Declarations
-------------------------------------------------------------------*/

/*===================================================================
   FUNCTION PROTOTYPES
===================================================================*/
/** @} */

#ifdef __cplusplus
}
#endif

#endif /* HAL_ERROR_H */


