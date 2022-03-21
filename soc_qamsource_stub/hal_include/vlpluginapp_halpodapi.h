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



#ifndef __VLPLUGINAPP_HALPODAPI_H__
#define __VLPLUGINAPP_HALPODAPI_H__

/**
 * @file vlpluginapp_halpodapi.h
 *
 * @defgroup HAL_POD_Interface  POD APIs
 * Defines the list of POD manager APIs to perform initialization, opening and closing the
 * cable card device, configuring the cipher, APIs for controlling out of band and inband etc.
 * @ingroup  RMF_HAL_API
 * @{
 */

#include "ip_types.h"

/*-------------------------------------------------------------------
   Defines/Macros
-------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief This function initializes  Pod manager module.
 *
 * Performs a set of operations
 * - Initializes the cipher sync feature
 * - Initializes the cable card capabilities
 * - Opens the Cable card device driver and do the initalization
 * - Once the driver is ready, resets the pod manager thread
 * - Creates the message queue for reading data and populate the internal database used to cache information
 *   communicated between the Host and the POD.
 *
 * @return Returns the status of the operation.
 * @retval Returns HAL_SUCCESS on success, appropiate error code otherwise.
 */
int    CHALPod_init();

/**
 * @brief This API performs Out of Band operations.
 *
 * Operations like
 * - Initalize the network interface
 * - Stops the interface
 * - Reads the data from socket
 * - Writes data to a socket
 * - Reads the network ip
 *
 * @param[in] device_instance  Handle of the device. This parameter is not in use.
 * @param[in] eCommand         Command to perform the operation
 * @param[in] data             Data to perform the operation
 *
 * @return Returns the status of the operation.
 * @retval Returns HAL_SUCCESS on success, appropiate error code otherwise.
 */
int    CHALPod_oob_control(int device_instance, VL_OOB_COMMAND eCommand, void * pData);

#ifdef __cplusplus
}
#endif

/** @} */

#endif // __VLPLUGINAPP_HALPODAPI_H__
