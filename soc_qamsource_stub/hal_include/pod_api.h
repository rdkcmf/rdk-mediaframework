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



#ifndef POD_API_H
#define POD_API_H
//#include "mspod.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long DEVICE_HANDLE_t;
typedef signed long    INT32;
typedef unsigned long  UINT32;


/*-------------------------------------------------------------------
   Types/Structs
-------------------------------------------------------------------*/
/**
 * @addtogroup HAL_POD_Types
 * @{
 */
/**
 * @brief Structure that holds the cable card status.
 */
typedef enum
{
    POD_CARD_INSERTED = 0,   /**< The cablecard is present/inserted */
    POD_CARD_REMOVED = 1,    /**< The cablecard is removed*/
    POD_CARD_ERROR = 2,      /**< CableCard generated a error*/
    POD_PERSONALITY_CHANGE_COMPLETE = 3, /**< CableCard personality change completed*/
    POD_DATA_AVAILABLE = 4 /**< data is available */
} POD_STATUS_t;

/** @} */

/*===================================================================
   FUNCTION PROTOTYPES
===================================================================*/
/*===================================================================
FUNCTION NAME: HAL_POD_<APIs>
DESCRIPTION:
   HAL POD APIs
DOCUMENTATION:
   "PDT Hardware Abstraction Layer" document
===================================================================*/
/**
 *  @addtogroup HAL_POD_Interface
 *  @{
 */
/**
 * @brief This API resets the POD interface by setting the RS bit.
 *
 * @param[in] hPODHandle  Device handle.
 * @param[in] pData       This parameter is not in use.
 *
 * @return Returns the status of the operation.
 * @retval Returns HAL_SUCCESS on success, appropiate error code otherwise.
 */

INT32 HAL_POD_INTERFACE_RESET (DEVICE_HANDLE_t hPODHandle, UINT32  *pData); //POD Interface reset - by setting RS bit

/** @} */

#ifdef __cplusplus
}
#endif

#endif //TUNER_API_H





