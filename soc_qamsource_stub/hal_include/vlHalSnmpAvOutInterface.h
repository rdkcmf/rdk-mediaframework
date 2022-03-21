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



#ifndef __VL_HAL_SNMP_AVINTERFACE_H__
#define __VL_HAL_SNMP_AVINTERFACE_H__

/**
 * @file vlHalSnmpAvOutInterface.h
 *
 * @defgroup HAL_SNMP_Interface SNMP HAL Interface APIs
 * @ingroup  RMF_HAL_API
 * @defgroup HAL_SNMP_Types     SNMP HAL Interface Datatypes
 * @ingroup  RMF_HAL_Types
 * @defgroup HAL_SNMP_AVOUT_Interface  SNMP HAL Audio/Video Interface APIs
 * @ingroup  HAL_SNMP_Interface
 * @defgroup HAL_SNMP_AVOUT_Types      SNMP HAL Audio/Video Interface Datatypes
 * @ingroup  HAL_SNMP_TYPES
 */

#ifdef __cplusplus
extern "C" {
#endif

//Note: Add analog ports in the begining and digital ports at the end of the enum

/**
 * @addtogroup  HAL_SNMP_AVOUT_Types
 * @{
 */

/**
 * @brief Represents the Audio/video port types
 */
typedef enum
{
	HAL_AV_PORT_COMPOSITE,
	HAL_AV_PORT_SVIDEO,
	HAL_AV_PORT_RF,
	HAL_AV_PORT_COMPONENT,
	HAL_AV_PORT_HDMI,
	HAL_AV_PORT_L_R,
	HAL_AV_PORT_SPDIF,
	HAL_AV_PORT_1394,
	HAL_AV_PORT_DVI,
	HAL_AV_PORT_MAX
}HAL_SNMP_AV_PORT_TYPE;

/**
 *  @brief Represents the component Aspect ratio.
 */
typedef enum {
        COMPONENTASPECTRATIO_OTHER  =   1,
        COMPONENTASPECTRATIO_FOURBYTHREE =      2,
        COMPONENTASPECTRATIO_SIXTEENBYNINE = 3
}HAL_SNMP_ComponentAspectRatio_t;

/**
 *  @brief Represents the mpeg2 content PCR Lock Status.
 */
typedef enum {
        NOTLOCKED = 1,
        LOCKED   =  2
}HAL_SNMP_Mpeg2ContentPCRLockStatus_t;

/**
 *  @brief Represents the component output formats.
 */
typedef enum {
        COMPONENTOPFORMAT_480I = 1,
        COMPONENTOPFORMAT_480P = 2,
        COMPONENTOPFORMAT_720P = 3,
        COMPONENTOPFORMAT_1080I = 4,
        COMPONENTOPFORMAT_1080P = 5,
        COMPONENTOPFORMAT_INVALID
}HAL_SNMP_ComponentOutputFormat_t;

/**
 *  @brief Represents the AV port status.
 */
typedef enum
{
        HAL_AV_PORT_STATUS_DISABLED = 0,
        HAL_AV_PORT_STATUS_ENABLED
}HAL_SNMP_AV_PORT_STATUS;

/**
 *  @brief Represents the AV port info.
 */
typedef struct{

        HAL_SNMP_AV_PORT_STATUS         portStatus;
        HAL_SNMP_AV_PORT_TYPE           avOutPortType;
        unsigned long                   deviceIndex;

}HAL_SNMP_AV_PORT_INFO;

/**
 *  @brief Represents the AV output interface info.
 */
typedef struct halAVOutInterfaceInfo
{
	unsigned long	numAvOutPorts;
	HAL_SNMP_AV_PORT_INFO avOutPortInfo[HAL_AV_PORT_MAX];

}HAL_SNMP_AVOutInterfaceInfo;

/**
 *  @brief Represents the component video table.
 */
typedef struct {
	unsigned char                	   ComponentVideoConstrainedStatus;
	HAL_SNMP_ComponentOutputFormat_t   ComponentOutputFormat;
	HAL_SNMP_ComponentAspectRatio_t    ComponentAspectRatio;
	unsigned char                 	   ComponentVideoMuteStatus;
}HAL_SNMP_ComponentVideoTable_t;

/**
 *  @brief Represents the mpeg2 content CCI values.
 */
typedef enum {
	CCI_COPYFREELY	=	0,
	CCI_COPYNOMORE	=	1,
	CCI_COPYONEGENERATION =	2,
	CCI_COPYNEVER	=	3,
	CCI_UNDEFINED	=	4
}HAL_SNMP_Mpeg2ContentCCIValue_t;

/**
 *  @brief Represents the mpeg2 content APS values.
 */
typedef enum {
	APSVALUE_TYPE1		=	1,
	APSVALUE_TYPE2		=	2,
	APSVALUE_TYPE3		=	3,
	APSVALUE_NOMACROVISION	=	4,
	APSVALUE_NOTDEFINED	=	5
}HAL_SNMP_Mpeg2ContentAPSValue_t;

/**
 *  @brief Represents the mpeg2 content tables.
 */
 typedef struct {
    unsigned long           		index;                /*Range: 1..20*/
    unsigned long           		programNumber;
    unsigned long           		transportStreamID;
    unsigned long           		totalStreams;
    unsigned long              		selectedVideoPID;
    unsigned long             		selectedAudioPID; /* Range: -1 | 1..8191*/
    unsigned long                	otherAudioPIDs;
    HAL_SNMP_Mpeg2ContentCCIValue_t	cciValue;
    HAL_SNMP_Mpeg2ContentAPSValue_t	apsValue;
    unsigned long                	citStatus;
    unsigned long                	broadcastFlagStatus;
    unsigned long                	epnStatus;
    unsigned long              		pcrPID;
    HAL_SNMP_Mpeg2ContentPCRLockStatus_t PCRLockStatus;
    unsigned long           		decoderPTS;
    unsigned long           		discontinuities;
    unsigned long           		pktErrors;
    unsigned long           		pipelineErrors;
    unsigned long           		decoderRestarts;
    unsigned long 			LTSID;

}HAL_SNMP_Mpeg2ContentTable_t;

/**
 *  @brief Represents the mpeg2 SPDIF info.
 */
typedef struct {
     unsigned long		audioFormat;
     unsigned long      audioMuteStatus;
}HAL_SNMP_SpdifTable_t;

/** @} */

/**
 *  @addtogroup  HAL_SNMP_AVOUT_Interface 
 *  @{
 */

/**
 * @brief This function is used to get information about AV output ports for a given number of connected displays.
 *
 * It also provides details such as port status, port type (composite,HDMI etc.).
 *
 * @param[in]  maxNumOfDiplays       Number of displays connected to AV outport.
 * @param[out] ptAVOutInterfaceInfo  Address where AV out information is stored.
 *
 * @return Returns the status of the operation.
 * @retval Returns 1 if successful, appropiate error code otherwise.
 */
unsigned int HAL_SNMP_Get_AvOut_Info(void* ptAVOutInterfaceInfo, unsigned int maxNumOfDiplays);

/**
 * @brief This function is used to get the number of display handles used in the system.
 *
 * @param[out] ptDispHandleCount  Total count for display handles.
 *
 * @return Returns the status of the operation.
 * @retval Returns 0 if successful, appropiate error code otherwise.
 */
unsigned int HAL_SNMP_Get_Display_Handle_Count(unsigned int* ptDispHandleCount);

/**
 * @brief This API is designed to lists the display handles used in this system.
 *
 * This API is not implemented.
 *
 * @param[in]  maxNumOfDiplays  Total count for display handles.
 * @param[out] ptDispHandles    The handles to be displayed.
 *
 * @return Returns the status of the operation.
 * @retval Returns 0 if successful, appropiate error code otherwise.
 */
unsigned int HAL_SNMP_Get_Display_Handles(unsigned long* ptDispHandles, unsigned int maxNumOfDiplays);

/**
 * @brief This function is used to get the Component video information like Aspect ratio, format of video and mute status.
 *
 * @param[out] pComponentInfo  Represents the structure with current video information.
 *
 * @return Returns the status of the operation.
 * @retval Returns 1 if successful, appropiate error code otherwise.
 */
unsigned int HAL_SNMP_Get_ComponentVideo_info(void* pComponentInfo);

/**
 * @brief This function is used to get the RF Channel info like Program number, Spectral bandwidth, Symbol rate etc.
 *
 * Implementation not present.
 *
 * @return Returns the status of the operation.
 * @retval Returns 1 if successful, appropiate error code otherwise.
 */
unsigned int HAL_SNMP_Get_RFChan_info();

/**
 * @brief This function is used to get the Analog video info such as aspect ratio, resolution.
 *
 * Implementation not present.
 *
 * @return Returns the status of the operation.
 * @retval Returns 1 if successful, appropiate error code otherwise.
 */
unsigned int HAL_SNMP_Get_AnalogVideo_info();

/**
 * @brief This function is used to get the SPDIF Audio info like sample rate, Dolby/DTS support etc.
 *
 * @param[out] pSpdifInfo  Structure which holds the SPDIF Audio channel information.
 *
 * @return Returns the status of the operation
 * @retval Returns 1 if successful, appropiate error code otherwise.
 */
unsigned int HAL_SNMP_Get_Spdif_info(void *pSpdifInfo);

/**
 * @brief This function is used to get the status information of a specified program like Aspect ratio,
 * format of video and mute status.
 *
 * It also finds the tuner count, add/remove the connection details from the list,
 * updates program info like audio/video pid, program status, active program number etc.
 *
 * @param[in]  listIter    Iterator for iterating through the device list.
 * @param[out] ptConnInfo  Structure contains Component video table info
 *                         about physicalSrcId, videopid, audiopid etc.
 *
 * @return Returns the status of the operation
 * @retval On Success, it returns 1, else it returns 0.
 */
unsigned int HAL_SNMP_Get_ProgramStatus_info(void* ptConnInfo, unsigned int listIter);

/**
 * @brief This function is used to get the mpeg content info data.
 *
 * This API returns the information like program number, audio/video pid, PCR LockStatus, CCI value etc.
 *
 * @param[out] pMpeg2ContentInfo MPEG2 content table
 *
 * @return Returns the status of the operation
 * @retval Returns 1 if successful, appropiate error code otherwise.
 */
unsigned int HAL_SNMP_Get_Mpeg2Content_info(void* pMpeg2ContentInfo);

/**
 * @brief This function is used to get the device list (number of tuners, disk etc).
 *
 * This function invokes "GetConnListSize" to get the device list.
 *
 * @param[out] devListSize The device list size.
 */
void HAL_SNMP_Get_DevListSize(unsigned int* devListSize);

/** @} */

#ifdef __cplusplus
}
#endif

#endif//__VL_HAL_SNMP_AVINTERFACE_H__

