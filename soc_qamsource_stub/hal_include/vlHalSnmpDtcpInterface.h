/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2016 RDK Management
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


#ifndef __VL_HAL_SNMP_DTCP_INTERFACE_H__
#define __VL_HAL_SNMP_DTCP_INTERFACE_H__

/**
 * @defgroup HAL_SNMP_DTCP_Interface SNMP HAL DTCP Interface API
 * @ingroup  HAL_SNMP_Interface
 * @{
 */

#include "SnmpIORM.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief This function is used to return the dtcp info and to populate the SNMP_INTF_DTCP_Info structure.
 *
 * @param[in]  ptDtcpInfo Structure which holds the DTCP SRM(System Renewability Message) details.
 *
 * @return Returns the status of the operation.
 * @retval Returns 1 on success, appropiate error code otherwise.
 */
unsigned int HAL_SNMP_Get_DTCP_info(SNMP_INTF_DTCP_Info* ptDtcpInfo);

/**
 * @brief This function is used to reset the DTCP SRM.
 *
 * SRM(System Renewability Messages) used by the DTCP-IP enabled devices to "revoke" devices
 * whose secret keys have been compromised and either publicly distributed or incorporated in unauthorized devices.
 *
 * @param[in]  bReset Flag to enable/disable SRM.  
 *
 * @return Returns the status of the operation.
 * @retval Returns 1 on success, appropiate error code otherwise.
 */
unsigned int HAL_SNMP_Reset_DTCP_SRM(int bReset);

#ifdef __cplusplus
}
#endif

#endif // __VL_HAL_SNMP_DTCP_INTERFACE_H__
