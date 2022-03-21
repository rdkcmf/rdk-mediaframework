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


#ifndef HAL_API_H
#define HAL_API_H

/**
 * @file hal_api.h
 *
 * @defgroup RMF_HAL  Media Framework HAL APIs & Datatypes
 * RMF (RDK Media Framework) is the heart of RDK. It gives the flexibility to RDK to play from Live streaming, DLNA streaming, etc.
 * Also it gives the flexibility to stream the contents to file or TV or to another connected device.
 * Depending on the configuration and the input url, RMF will build the audio and video pipeline using the RMF elements.
 * RMF elements will use gstreamer to setup the pipeline.
 * The RMF allows the platforms easily extendable by loosely coupling with any third party plug-ins.
 * The level of abstraction in RMF allows easy support for other architectures/hardware to be plugged.
 * @ingroup  RMF
 *
 * @defgroup RMF_HAL_Types Media Framework HAL Data Types
 * Describes the data types related to Code download, POD manager, Diagnostics, SNMP and Common data types
 * and the error types used by the mediaframework HAL module.
 * - Code Download describes the data types used for code download operation that describes the 
 *   code download status, image signature information, type of image to be downloaded, etc.
 * - Common Data types describes the data types that are common to the mediaframework HAL module.
 * - POD data types describes the macros and data structures used by the POD manager instance.
 * - Diagnostics describes the data structures, enums and defines used for the diagnostic purpose.
 *   These includes the structures for storing memory information, network information, File system details,
 *   Transmitter signal information, ECM information etc.
 * - Error Defines the common error types used by the mediaframework HAL module 
 * - SNMP describes the data type used for SNMP related query.
 * @ingroup  RMF_HAL
 *
 * @defgroup RMF_HAL_API Media Framework HAL APIs
 * Describes the Interfaces related to Code download, POD manager, Diagnostics, SNMP and Common data types
 * and the error types used by the mediaframework HAL module.
 * - Code Download provides a list of APIs for initializing the Code download module,
 *   register the callbacks and also provides API to handle the callbacks.
 * - DSG provides the APIs for initializing the DSG module, sends the DSG status, set the DSG modes, etc.
 * - POD defines the list of POD manager APIs to perform initialization, opening and closing 
 *   the cable card device, configuring the cipher, APIs for controlling out of band and inband etc.
 * - SNMP describes the interfaces used for SNMP related query.
 * - Mediaplayer sink provides the list of APIs for setting the program number, preffered language, 
 *   retrieving the audio language supported etc.
 * @ingroup  RMF_HAL
 * @defgroup HAL_COMMON_types  Common Data Types
 * @ingroup  RMF_HAL_Types
 */


#ifdef __cplusplus
extern "C" {
#endif

/*===================================================================
   INCLUDES AND PUBLIC DATA DECLARATIONS
===================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>


/*-------------------------------------------------------------------
   Types/Structs
-------------------------------------------------------------------*/
/**
 * @brief Common Data types.
 */
typedef unsigned char  UINT8;
typedef unsigned short UINT16;
typedef unsigned long  UINT32;
typedef signed char    INT8;
typedef signed short   INT16;
typedef signed long    INT32;
typedef unsigned char  BOOLEAN;
typedef float          FLOAT32;

/**
 * @brief Device handle type.
 */
typedef unsigned long         DEVICE_HANDLE_t;

/**
 * @brief Boolean values.
 */
#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif


/** 
 * @addtogroup HAL_COMMON_types
 * @{
 */

/**
 * @brief  Lock gained / Lock Lost enum type
 */
typedef enum {
      DEVICE_LOCKED,
      DEVICE_UNLOCKED,
      DEVICE_TIMEOUT
} DEVICE_LOCK_t;


/**
 * @brief Common callback function indicates lock lost / lock gained.
 */
typedef void (*DEVICE_LOCK_FUNC_t) (DEVICE_HANDLE_t    hDeviceHandle,
               DEVICE_LOCK_t eLockStatus,
               void*         puldata );

/**
 * @brief Output display formats
 */
typedef enum
{
    DISP_FORMAT_480I,        /**< 720x480i */
    DISP_FORMAT_480P,        /**< 720x480p */
    DISP_FORMAT_720P,        /**< 1280x720p */
    DISP_FORMAT_1080I,        /**< 1920x1080i */
    DISP_FORMAT_1080P,        /**< 1920x1080p 60HZ*/
    DISP_FORMAT_1080P_24HZ, /**< 1920x1080p 24HZ*/
    DISP_FORMAT_1080P_30HZ, /**< 1920x1080p 30HZ*/
    DISP_FORMAT_2160P, /**< 3840x2160p 60HZ*/
    DISP_FORMAT_2160P_30HZ, /**< 3840x2160p 30HZ*/
    DISP_FORMAT_PassThru,   /**< Pass through mode*/
    DISP_INVALID_FORMAT,    /**< Invalid Format*/
    DISP_MAX_DISPLAY_FORMAT /**< Maximum display format reached*/
} DISP_DISPLAY_FORMAT_t ;

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* HAL_API_H */
