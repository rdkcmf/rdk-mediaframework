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



#ifndef __VLPLUGINAPP_HALDSGAPI_H__
#define __VLPLUGINAPP_HALDSGAPI_H__

/**
 * @file vlpluginapp_haldsgapi.h
 *
 * @defgroup HAL_DSG_Interface  DOCSIS Settop Gateway(DSG) APIs
 * @ingroup  RMF_HAL_API
 * @{
 */

#include "dsg_types.h"

/*-------------------------------------------------------------------
   Defines/Macros
-------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief This function initializes  HAL DSG module.
 *
 * @return Returns the status of the operation.
 * @retval Returns 0 on success, appropiate error code otherwise.
 */
int CHALDsg_init();

/**
 * @brief This function is used to set the notify function HAL_DSG_SetNotify().
 *
 * @param[in]  device_instance       Handle of the device. This Variable is not in use.
 * @param[in]  type                  Type of callback function whether it is a tunnel data, control data, image data etc.
 * @param[in]  func_ptr              The callback function.
 *
 * @return Returns the status of the operation.
 * @retval Returns 0 on success, appropiate error code otherwise.
 */

int CHALDsg_register_callback(int device_instance, VL_DSG_NOTIFY_TYPE_t type, void *func_ptr);

/**
 * @brief This function invokes the HAL_DSG_ProcessControlMessage function and displays the DSG status.
 *
 * Moved the implementation to process the message to vlDsgSetConfig() function.
 *
 * @param[in]  device_instance       Handle of the device. This Variable is not in use.
 * @param[in]  pData                 The control message data.
 * @param[in]  length                The length of the control message
 *
 * @return Returns the status of the operation.
 * @retval Returns 0 on success, appropiate error code otherwise.
 */

int CHALDsg_Send_ControlMessage(int device_instance, unsigned char * pData, unsigned long length);

/**
 * @brief This API sets the DSG operational modes.
 *
 * Modes can be One Way Basic, Two way Basic, One way Advanced, Two way Advanced, IP direct mode and Invalid.
 *
 * @param[in]  device_instance       Handle of the device. This Variable is not in use.
 * @param[in]  ulTag                 Tag name to identify the operation.
 * @param[in]  pvConfig              Configuration to set.
 *
 * @return Returns the status of the operation.
 * @retval Returns HAL_SUCCESS on success, appropiate error code otherwise.
 */
int CHALDsg_Set_Config(int device_instance, unsigned long ulTag, void* pvConfig);

/** @} */

#ifdef __cplusplus
}
#endif

#endif // __VLPLUGINAPP_HALDSGAPI_H__
