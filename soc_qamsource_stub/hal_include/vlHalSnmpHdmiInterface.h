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




#ifndef __VL_HAL_SNMP_HDMI_INTERFACE_H__
#define __VL_HAL_SNMP_HDMI_INTERFACE_H__

/**
 * @file vlHalSnmpHdmiInterface.h
 *
 * @defgroup HAL_SNMP_HDMI_Interface  SNMP HAL HDMI APIs
 * @ingroup  HAL_SNMP_Interface
 * @defgroup HAL_SNMP_HDMI_Types      SNMP HAL HDMI Datatypes
 * @ingroup  HAL_SNMP_Types
 * @{
 */

#include "SnmpIORM.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup  HAL_SNMP_HDMI_Types
 * @{
 */

#define MAXCHARLEN	256

/**
 * @brief Represents the HDCP device status.
 */
typedef enum
{
	HDCP_DEVICE_STATUS_NON_COMPLIANT,
	HDCP_DEVICE_STATUS_COMPLIANT,
	HDCP_DEVICE_STATUS_REVOKED,
	HDCP_DEVICE_STATUS_RESERVED
}HDCP_DEVICE_STATUS;

/**
 * @brief Represents the AVI color space information.
 */
typedef enum
{
	HDMI_AVIFRAME_COLOR_SPACE_RGB = 0,
	HDMI_AVIFRAME_COLOR_SPACE_YCbCr422,
	HDMI_AVIFRAME_COLOR_SPACE_YCbCr444,
	HDMI_AVIFRAME_COLOR_SPACE_RSVD

}HDMI_AVIFRAME_COLOR_SPACE;

/**
 * @brief Represents Audio format description.
 */
typedef enum
{
   AUDIO_FMT_MPEG12,  /**< MPEG layer 1 and layer 2*/
   AUDIO_FMT_MPEG2,
   AUDIO_FMT_MP3,     /**< MPEG Layer 3        */
   AUDIO_FMT_AAC,     /**< Advanced Audio Coding. Part of MPEG-4 */
   AUDIO_FMT_AAC_PLUS,/**< AAC plus SBR. aka MPEG-4 High Efficiency (AAC-HE) */
   AUDIO_FMT_AC3,     /**< Dolby Digital AC3 audio */
   AUDIO_FMT_AC3_PLUS,     /**< Dolby Digital Plus or Enhanced AC3 audio */
   AUDIO_FMT_DTS,      /**< Digital Surround sound. */
   AUDIO_FMT_ATRAC,    /**< Adaptive Transform Acoustic Coding */
   AUDIO_FMT_LPCM,      /**< PULSE CODE MODULATION. */
   AUDIO_FMT_OTHER
} AUDIO_FMT_t;

/**
 * @brief Represents the Audio sample rates.
 */
typedef enum
{
	AUDIO_Sample_Rate_32kHz,
	AUDIO_Sample_Rate_44kHz,
	AUDIO_Sample_Rate_48kHz,
	AUDIO_Sample_Rate_88kHz,
	AUDIO_Sample_Rate_96kHz,
	AUDIO_Sample_Rate_176kHz,
	AUDIO_Sample_Rate_192kHz,
	AUDIO_Sample_Rate_Other
} AUDIO_Sample_Rate_t;

/**
 * @brief Represents the Audio samples size.
 */
typedef enum
{
	AUDIO_Sample_Size_Unknown = 0,
    	AUDIO_Sample_Size_16,
    	AUDIO_Sample_Size_20,
    	AUDIO_Sample_Size_24
}AUDIO_Sample_Size;

/**
 * @brief Represents the Video frame rates.
 */
typedef enum
{
    VIDEO_Frame_Rate_unknown = 0,
    VIDEO_Frame_Rate_23_976,
    VIDEO_Frame_Rate_24,
    VIDEO_Frame_Rate_25,
    VIDEO_Frame_Rate_29_97,
    VIDEO_Frame_Rate_30,
    VIDEO_Frame_Rate_50,
    VIDEO_Frame_Rate_59_94,
    VIDEO_Frame_Rate_60,
    VIDEO_Frame_Rate_14_985,
#ifndef LEGACY_NEUXS
    VIDEO_Frame_Rate_7_493,
#else
    VIDEO_Frame_Rate_7_943,
#endif // LEGACY_NEUXS
    VIDEO_Frame_Rate_Max

}VIDEO_Frame_Rate;

/**
 * @brief Represents the Aspect ratio supported by the device.
 */
typedef enum
{
	ASPECT_RATIO_UNKNOWN = -1,
	ASPECT_RATIO_4_3 = 2,
	ASPECT_RATIO_16_9,
	ASPECT_RATIO_2_21_1,
	MAX_ASPECT_RATIO
}DISPLAY_ASPECT_RATIO_t ;

/**
 * @brief Represents the display formats supported by the device.
 */
typedef enum
{
	DISP_FORMAT_480I,		/* 720x480i */
	DISP_FORMAT_480P,		/* 720x480p */
	DISP_FORMAT_720P,		/* 1280x720p */
	DISP_FORMAT_1080I,		/* 1920x1080i */
	DISP_FORMAT_1080P,		/* 1920x1080p 60HZ*/
    	DISP_FORMAT_1080P_24HZ, /* 1920x1080p 24HZ*/
    	DISP_FORMAT_1080P_30HZ, /* 1920x1080p 30HZ*/
        DISP_FORMAT_2160P, /* 3840x2160p 60HZ*/
        DISP_FORMAT_2160P_30HZ, /* 3840x2160p 30HZ*/
	DISP_FORMAT_PassThru,   /* Pass through mode*/
	DISP_INVALID_FORMAT,
	DISP_MAX_DISPLAY_FORMAT
}DISPLAY_FORMAT_t ;

/**
 * @brief Represents the DVI hdmi information.
 */
typedef struct
{
	unsigned char		 isHdmi;
	unsigned long          	 index;
	unsigned long            outputType;
	unsigned long            connStatus;
	unsigned long            repStatus;
	unsigned long            videoXmisStatus;
        unsigned long            hdcpStatus;
	unsigned long            videoMuteStatus;
	DISPLAY_FORMAT_t         videoFormat;
	DISPLAY_ASPECT_RATIO_t   aspectRatio;
    unsigned char            bHDCPEnabled;
    HDCP_DEVICE_STATUS       connectedDeviceHDCPStatus;
	AUDIO_FMT_t              audioFormat;
	AUDIO_Sample_Rate_t      audioSampleRate;
	unsigned long          	 audioChanCount;
	unsigned long            audioMuteStatus;
	AUDIO_Sample_Size        audioSampleSize;
	HDMI_AVIFRAME_COLOR_SPACE   colorSpace;

    //Video format related
    int                     horizontal_lines;
    int                     vertical_lines;
	VIDEO_Frame_Rate         videoFrameRate;
    unsigned char           bInterlaced;

	unsigned long            attachedDevType;
	unsigned char            edid[3*MAXCHARLEN];
	unsigned int          	 edid_len;
	unsigned long            lipSyncDelay;
	unsigned char            cecFeatures[MAXCHARLEN];
	unsigned int          	 cecFeatures_len;
	unsigned char            features[MAXCHARLEN];
	unsigned int          	 features_len;
	unsigned long            maxDevCount;
	
}SNMP_INTF_DVIHDMI_Info;

/** @} */

/**
 * @addtogroup  HAL_SNMP_HDMI_Interface
 * @{
 */

/**
 * @brief This function is used to get DVI hdmi information.
 * This function invokes vl_Get_DviHdmi_info() to get the information.
 *
 * @param[out] ptDviHdmiInfo Represents the DVI HDMI information.
 *
 * @return Returns the status of the operation.
 * @retval Returns 1 on success, appropiate error code otherwise.
 */
unsigned int HAL_SNMP_Get_DviHdmi_info(SNMP_INTF_DVIHDMI_Info* ptDviHdmiInfo);

/**
 * @brief This function is used to get the data content in the DVI HDMI Video Format Table (from hal display module).
 *
 * @param[in]  pnCount                    Data count.
 * @param[out] vl_DVIHDMIVideoFormatTable DVI HDMI Video Table containing video information.
 *
 * @return Returns the status of the operation.
 * @retval Returns 1 on success, appropiate error code otherwise.
 */
unsigned int HAL_SNMP_ocStbHostDVIHDMIVideoFormatTableData(SNMPocStbHostDVIHDMIVideoFormatTable_t* vl_DVIHDMIVideoFormatTable, int *pnCount);

/**
 * @brief This function is used to get DVI hdmi info such as output type (dvi/hdmi), connection status,
 * repeater status, video resolution, format and then it updates to DVI HDMI Table.
 *
 * @param[out] ptDviHdmiInfo  Represents the DVI HDMI information.
 */
void vl_Get_DviHdmi_info(SNMP_INTF_DVIHDMI_Info* ptDviHdmiInfo);

/** @} */

#ifdef __cplusplus
}
#endif

#endif
