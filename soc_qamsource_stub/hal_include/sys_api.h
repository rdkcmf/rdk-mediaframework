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


#ifndef SYS_API_H
#define SYS_API_H

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @file sys_api.h
 *
 * @defgroup HAL_SYS_Types  System Data Types
 * @ingroup  RMF_HAL_Types
 * @{
 */

/*-------------------------------------------------------------------
   Include Files
-------------------------------------------------------------------*/

#include "hal_api.h"
#include "snmp_types.h"

/**
 * @brief Represents the device power states.
 */
typedef enum
{
    SYS_POWER_STATE_ON,
    SYS_POWER_STATE_STANDBY,
    SYS_POWER_STATE_UNKNOWN
}SYS_POWER_STATE;

/** @} */

/** @defgroup HAL_SYS_Interface  System API(s)
 *  @ingroup  RMF_HAL_API
 *  @{
 */

/**
 * @brief This function reboots the device.
 *
 * @param[in] pszRequestor Indicates where the reboot request is from.
 * @param[in] pszReason    Reason for reboot
 */

void HAL_SYS_Reboot(const char * pszRequestor, const char * pszReason);


/**
 * @brief This API returns the power state the system.
 *
 * @param[in] pSysPowerState Power state of the device.
 */
void HAL_SYS_GetPowerState( SYS_POWER_STATE *pSysPowerState);

/** @} */

#ifdef __cplusplus
}
#endif

#endif //SYS_API_H
