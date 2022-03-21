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



#ifndef POD_INTERFACE_H
#define POD_INTERFACE_H

/*===================================================================
   INCLUDES AND PUBLIC DATA DECLARATIONS
===================================================================*/

/*-------------------------------------------------------------------
   Defines/Macros
-------------------------------------------------------------------*/

/*-------------------------------------------------------------------
   Include Files
-------------------------------------------------------------------*/
#include "hal_api.h"
#include <stdlib.h>
#include <unistd.h>


/*-------------------------------------------------------------------
   Types/Structs
-------------------------------------------------------------------*/


/*-------------------------------------------------------------------
   Global Data Declarations
-------------------------------------------------------------------*/


/*===================================================================
   FUNCTION PROTOTYPES
===================================================================*/


/**
 * @addtogroup HAL_POD_Interface
 * @{
 */

/*===================================================================
FUNCTION NAME:
DESCRIPTION:
   Describe general purpose of the function and usage information
DOCUMENTATION:
   Where to find design documentation
===================================================================*/

/**
 * @brief This API is used to set the IP configuration Notification (ICN) pid.
 * ICN indicates the CPE is in IPDC(IP Direct) mode or not.
 *
 * @param[in] IcnPid  ICN pid value to set.
 *
 * @return Returns status of the operation.
 * @retval HAL_SUCCESS on success, appropiate error code otherwise.
 */
INT32 HAL_Save_Icn_Pid (UINT16 *IcnPid);
/** @} */

#endif //TUNER_INTERFACE_H

