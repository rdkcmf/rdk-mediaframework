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



#include "cablecarddriver.h"
#include "rmf_osal_sync.h"
#include "rmf_osal_event.h"

/**
 * @file halpodclass.h
 *
 * @defgroup HAL_POD_Types  POD Data Types
 * Describes the data types, macros and structures used by the POD manager instance.
 * @ingroup  RMF_HAL_Types
 * @{
 */

#ifndef __HALPOD_DRVPLGIN_H__
#define __HALPOD_DRVPLGIN_H__
/*-------------------------------------------------------------------
   Defines/Macros
-------------------------------------------------------------------*/
#define EXTDEV_AUDIOBB_MAX_DEVICES  2
#define EXTDEV_VIDEOBB_MAX_DEVICES  1
#define EXTDEV_POD_MAX_DEVICES      2
#define EXTDEV_IEEE1394_MAX_DEVICES 2
#define HAL_POD_MAX_PODS   (1) 	   /**<  number of cablecards */
#define MAX_CABLECARD_DEVICES   (1)



typedef unsigned long DEVICE_HANDLE_t;

/** @} */

class CHALPod : public cableCardDriver
{
public:

	CHALPod();
	~CHALPod();
  // Methods
	char* plugin_title()
	{
		return ("halpod");
	}

/**
 *  @addtogroup HAL_POD_Interface
 *  @{
 */

/**
 * @brief This API starts the Pod manager module.
 *
 * Displays the number of POD devices and the handles associated with them. Also displays
 * the external POD devices and its capabilities.
 */
	void 		initialize();

/**
 * @brief This API returns the number of  POD Devices.
 *
 * @return Returns the number of POD devices.
 */
	int  		get_device_count();

/**
 * @brief This API creates the POD manager instance.
 *
 * Also requests the POD handle associated with the device.
 *
 * @param[in] device_instance  POD manager instance.
 *
 * @return Returns the POD handle.
 */
	int		open_device(int device_instance);

/**
 * @brief This API terminate the instance created by the open_device() API.
 *
 * @param[in] device_instance  POD manager instance.
 *
 * @return Returns the status of the operation.
 * @retval Returns 0 on success, appropiate error code otherwise.
 */
	int		close_device(int device_instance);

/**
 * @brief This API registers the callback function based on the callback type.
 *
 * Following are the types of callback function.
 * NOTIFY_CARD_DETECTION_CB
 * POD_NOTIFY_SEND_POD_POLL
 * POD_NOTIFY_DATA_AVAILABLE
 * POD_NOTIFY_EXTCH_DATA_AVAILABLE
 *
 * @param[in] type              Type of callback function.
 * @param[in] func_ptr          Function to be registered.
 * @param[in] device_instance   POD device instance.
 *
 * @return Returns the status of the operation.
 * @retval Returns 0 on success, appropiate error code otherwise.
 */
	int 		register_callback(eCallbackType type, void *func_ptr, int device_instance);

/**
 * @brief This API sends the stream to the cable card.
 *
 * @param[in] ptr              The POD buffer.
 * @param[in] type             Channel type.
 * @param[in] device_instance  POD device instance
 *
 * @return Returns the status of the operation.
 * @retval Returns 0 on success, appropiate error code otherwise.
 */
	int 		send_data(void *ptr,  CHANNEL_TYPE_T  type, int device_instance);

/**
 * @brief This API returns the handle associated with the device.
 *
 * @param[in] device_instance  Pod device instance.
 *
 * @return  Returns the device handle.
 */
	DEVICE_HANDLE_t GetHandleFromInstance(int device_instance);

/**
 * @brief This API connects the tuner to the POD manager.
 *
 * @param[in] tuner_in_handle Handle corresponds to the tuner.
 */
	void		ConnectSourceToPOD(unsigned long tuner_in_handle);

/**
 * @brief This API disconnects the tuner from the POD manager.
 *
 * @param[in] tuner_in_handle Handle to be disconnected
 */
	void		DisconnectSourceFromPOD(unsigned long tuner_in_handle);

/**
 * @brief This function is used to perform Out of Band Operations.
 *
 * Operations like
 * - Initalize the network interface
 * - Stops the interface
 * - Reads the data from socket
 * - Writes data to a socket
 * - Reads the network ip
 *
 * @param[in] device_instance  POD manager Instance. This parameter is not in use.
 * @param[in] eCommand         Command to perform the operation
 * @param[in] pData            Data to perform the operation
 *
 * @return Returns the status of the operation.
 * @retval Returns HAL_SUCCESS on success, appropiate error code otherwise.
 */
    int    oob_control(int device_instance, unsigned long eCommand, void * pData);
/**
 * @brief This function is used to perform InBand Operations.
 *
 * Operations like
 * - Creates notify task.
 * - Performs PCMCIA reset.
 * - Full reset.
 *
 * @param[in] device_instance  POD manager instance. This parameter is not in use.
 * @param[in] eCommand         Command to perform the operation
 * @param[in] pData             Data to perform the operation
 *
 * @return Returns the status of the operation.
 * @retval Returns HAL_SUCCESS on success, appropiate error code otherwise.
 */
    int    if_control(int device_instance, unsigned long eCommand, void * pData);

/**
 * @brief This API configures the cipher sync feature.
 *
 * @param[in] ltsid       Local TSID used for decryption.
 * @param[in] PrgNum      Program number whose audio/video is to be decrypted.
 * @param[in] decodePid   An array of the PIDs that has to be filtered.
 * @param[in] numpids     Number of audio/video PIDs for which decryption keys are to be set.
 * @param[in] DesKeyAHi   DES key first 24 bits.
 * @param[in] DesKeyALo   DES key next 24 bits.
 * @param[in] DesKeyBHi   DES key next 24 bits.
 * @param[in] DesKeyBLo   DES last 24 bits.
 * @param[in] pStrPtr     This Variable is not in use.
 *
 * @return Returns the status of the operation.
 * @retval Returns HAL_SUCCESS on success, appropiate error code otherwise.
 */

int  configPodCipher(unsigned char ltsid,unsigned short PrgNum,unsigned long *decodePid,unsigned long numpids,unsigned long DesKeyAHi,unsigned long DesKeyALo,unsigned long DesKeyBHi,unsigned long DesKeyBLo,void *pStrPtr);

/**
 * @brief This API clears the keys used by configPodCipher function.
 *
 * @param[in] ltsid       Local TSID used for decryption.
 * @param[in] PrgNum      Program number whose audio/video is to be decrypted.
 * @param[in] decodePid   An array of the PIDs that has to be filtered.
 * @param[in] numpids     Number of audio/video PIDs for which decryption keys are to be set.
 *
 * @return Returns the status of the operation.
 * @retval Returns HAL_SUCCESS on success, appropiate error code otherwise.
 */
int  stopconfigPodCipher(unsigned char ltsid,unsigned short PrgNum,unsigned long *decodePid,unsigned long numpids);

private: // Variables

};

/** @} */

#endif // __HALPOD_DRVPLGIN_H__

