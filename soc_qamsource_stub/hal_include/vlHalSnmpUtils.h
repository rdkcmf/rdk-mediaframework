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




#ifndef __VL_HAL_SNMP_UTILS_H__
#define __VL_HAL_SNMP_UTILS_H__

/**
 * @defgroup HAL_SNMP_Utils_API   SNMP HAL Utils APIs
 * @ingroup  HAL_SNMP_Interface
 * @defgroup HAL_SNMP_Utils_Types  SNMP HAL Utils Datatypes
 * @ingroup  HAL_SNMP_Types
 * @{
 */

#include "cVector.h"
//#include "mpe_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_NUM_TEMP_SENSOR_TYPES   3
#define MAX_DESCR_NAME_LEN          255

/**
 * @brief Represents the type of temperature sensors used by the host system.
 */
typedef enum
{
  HOST_SYSTEM_SENSOR_TYPE_CPU = 1,
  HOST_SYSTEM_SENSOR_TYPE_HDD_INTERNAL,
  HOST_SYSTEM_SENSOR_TYPE_MAINBOARD,
  HOST_SYSTEM_SENSOR_TYPE_MAX
  
}HOST_SYSTEM_SENSOR_TYPE;

/**
 * @brief Represents the temperature info associated with the host system.
 */
typedef struct _tempDescrInfo
{
  HOST_SYSTEM_SENSOR_TYPE  sensorType;
  unsigned char name[MAX_DESCR_NAME_LEN];
  int           length;
  int           tempValue;
  int           tempLastUpdate;
  int           tempMaxVal;
  
}sTempDescrInfo;

/**
 * @brief Represents the device types associated with the snmp module.
 */
typedef enum
{
        devEthernet,
        devDISK,
	devTuner,
	dev1394

}deviceType;

/**
 * @brief Represents the device connection details.
 */
typedef struct
{
	unsigned char		isDecode;
	unsigned long 		devIndex;
	unsigned long 		decoderIndex;
	deviceType 		devType;
	unsigned char		vidDecStatus;
	unsigned char		audDecStatus;
	unsigned short          videoPid;
	unsigned short          audioPid;
	unsigned short          pcrPid;
	unsigned char		progStatus;
	unsigned long           tunerLTSID;
	unsigned int		programNum;
	unsigned char		CCIData;
        unsigned char           pcrlockStatus;
        unsigned long           dcontinuities;
        unsigned long           pts_errors;
        unsigned long           ContentPktErrors;
        unsigned long           ContentPipelineErrors;
}ConnDetails;

/** @} */

/**
 * @addtogroup  HAL_SNMP_Utils_API
 * @{
 */
 
/**
 * @brief Initializes the hal snmp module.
 *
 * @return Returns the status of the operation.
 * @retval Returns 0 on success, appropiate error code otherwise.
 */
int CHALSnmp_init(void);

/**
 * @brief This API adds the connection details to the device list.
 *
 * @param[in] pConnDetails  Connection details like Video PID, Audio PID, Local Transport Stream ID.
 *
 * @return Returns the status of the operation.
 * @retval Returns 0 on success, appropiate error code otherwise.
 */
unsigned int AddToConnList(ConnDetails* pConnDetails);

/**
 * @brief This API removes the connection details to the device list.
 *
 * @param[in] phySrcId  Index of the device list.
 * @param[in] devType   Type of connection.
 *
 * @return Returns the status of the operation.
 * @retval Returns 0 on success, appropiate error code otherwise.
 */
unsigned int RemFromConnList(unsigned long phySrcId, deviceType devType);

/**
 * @brief This API is used to return the temperature details of the system.
 *
 * @param[in] sensorType       Type of sensors used in the system.
 * @param[in] pTempDescrInfo   Saves the temperature info.
 *
 * @return Returns the status of the operation.
 * @retval Returns 0  on success, appropiate error code otherwise.
 */
int HAL_SNMP_Get_HostSystemTempDetails(HOST_SYSTEM_SENSOR_TYPE sensorType, sTempDescrInfo  *pTempDescrInfo);

/** @} */

#ifdef __cplusplus
}
#endif

#endif
