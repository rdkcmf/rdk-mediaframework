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



#ifndef __VLPLUGINAPP_HALCDLAPI_H__
#define __VLPLUGINAPP_HALCDLAPI_H__

/**
 * @file vlpluginapp_halcdlapi.h
 *
 * @defgroup HAL_CDL_Interface  Code Download(CDL) APIs
 * This file provides a list of API(s) for initializing the Code download module
 * Register the callbacks and also provides API to handle the callbacks.
 * @ingroup  RMF_HAL_API
 * @{
 */

#include "vl_cdl_types.h"

/*-------------------------------------------------------------------
   Defines/Macros
-------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief This function initializes  HAL CDL module.
 *
 * @return Returns the status of the operation.
 * @retval Returns 0 on success, appropiate error code otherwise.
 */
int CHALCdl_init();

/**
 * @brief This function is used to register the callback function, type and data associated with the client.
 *
 * Invokes the notify function HAL_CDL_SetNotify() to store the callback details to a structure.
 *
 * @param[in]  type                  Type of callback function.
 * @param[in]  pfnNotifyFunc         The callback function.
 * @param[in]  pulData               Data associated with the callback functions.
 *
 * @return Returns the status of the operation.
 * @retval Returns 0 on success, appropiate error code otherwise.
 */
int CHALCdl_register_callback(VL_CDL_NOTIFY_TYPE_t type, VL_CDL_VOID_CBFUNC_t pfnNotifyFunc, void * pulData);

/**
 * @brief This API is designed to notify the code download manager events.
 *
 * CDL manager handles the set of events like:
 * - Code download Started
 * - Code download Completed
 * - Code download image verification failed
 * - Setting Public key
 * - Set Code Verification certificate
 * - Initiate the code download.
 * - Validate Image signature, type etc
 *
 * @param[in]  eEvent                Events associated with the CDL manager.
 * @param[in]  nEventData            Data associated with the CDL manager. This variable is not in use.
 *
 * @return Returns the status of the operation.
 * @retval Returns VL_CDL_RESULT_SUCCESS on success, appropiate error code otherwise.
 */
int CHALCdl_notify_cdl_mgr_event(VL_CDL_MANAGER_EVENT_TYPE eEvent, unsigned long nEventData);

/** @} */

#ifdef __cplusplus
}
#endif

#endif // __VLPLUGINAPP_HALCDLAPI_H__
