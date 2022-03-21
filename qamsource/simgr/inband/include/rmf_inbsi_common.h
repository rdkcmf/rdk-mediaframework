/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2014 RDK Management
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

#ifndef __RMF_INBSI_COMMON_H__
#define __RMF_INBSI_COMMON_H__

#include <rmf_osal_time.h>


/**
 * @file rmf_inbsi_common.h
 * @brief RMF common inband service information.
 */


/* SI return error codes */
#define RMF_INBSI_ERROR_BASE               (0x00005000)
#define RMF_INBSI_SUCCESS                  (RMF_SUCCESS)


/**
 * @addtogroup sihandleevents
 * The following error codes are returned when a handle requested (say using freq, prog_num).
 * @{
 */
/*  Ex: when a handle requested (say using freq, prog_num) is not found
 RMF_INBSI_ERROR_NOT_FOUND is returned
 */
#define RMF_INBSI_NOT_FOUND                (RMF_INBSI_ERROR_BASE + 1)
#define RMF_INBSI_OUT_OF_MEM               (RMF_INBSI_ERROR_BASE + 2)
#define RMF_INBSI_INVALID_PARAMETER        (RMF_INBSI_ERROR_BASE + 3)
#define RMF_INBSI_LOCKING_ERROR            (RMF_INBSI_ERROR_BASE + 4)
/** @} */


/**
 * @addtogroup psievents
 * The following events are generated when PSI info is requested.
 * @{
 */
/*  Ex: when PSI info is requested but not yet acquired
 RMF_INBSI_ERROR_NOT_AVAILABLE is returned
 */
#define RMF_INBSI_PAT_NOT_AVAILABLE            (RMF_INBSI_ERROR_BASE + 5)
#define RMF_INBSI_PAT_NOT_AVAILABLE_YET        (RMF_INBSI_ERROR_BASE + 6)
#define RMF_INBSI_PMT_NOT_AVAILABLE            (RMF_INBSI_ERROR_BASE + 7)
#define RMF_INBSI_PMT_NOT_AVAILABLE_YET        (RMF_INBSI_ERROR_BASE + 8)
#define RMF_INBSI_NO_CA_INFO_IN_PMT	       (RMF_INBSI_ERROR_BASE + 9)
#define RMF_INBSI_CAT_NOT_AVAILABLE            (RMF_INBSI_ERROR_BASE + 10)
#define RMF_INBSI_CAT_NOT_AVAILABLE_YET        (RMF_INBSI_ERROR_BASE + 11)

#define RMF_INBSI_INVALID_HANDLE                   0
/** @} */


#define TSID_UNKNOWN                    (0xFFFFFFFF) // ECR 1072

//PAT or PMT section_length field
#define INIT_TABLE_VERSION                32


/**
 * @addtogroup Inbanddatastructures
 * @{
 */
/**
 * @brief The DescriptorTag contains constant values to be read from the
 * descriptor_tag field in a Descriptor.  The constant values will
 * correspond to those specified in OCAP SI.
 */
typedef struct _rmf_StreamDecryptInfo
{
    uint16_t pid;               // PID to query (filled in by the caller)
    uint16_t status;    // ca_enable value (0x1,0x2,0x3,0x71,0x73) or 0 if no status provided (filled in by MPE)
} rmf_StreamDecryptInfo;


/**
 * @brief This represents valid values for the elementary stream_type field in the PMT.
 * For more information, please refer  ISO/IEC 13818-1 : 2000 (E) Pg.no: 66- 80.
 */
typedef enum rmf_SiElemStreamType
{
	RMF_SI_ELEM_MPEG_1_VIDEO = 0x01,
	RMF_SI_ELEM_MPEG_2_VIDEO = 0x02,
	RMF_SI_ELEM_MPEG_1_AUDIO = 0x03,
	RMF_SI_ELEM_MPEG_2_AUDIO = 0x04,
	RMF_SI_ELEM_MPEG_PRIVATE_SECTION = 0x05,
	RMF_SI_ELEM_MPEG_PRIVATE_DATA = 0x06,
	RMF_SI_ELEM_MHEG = 0x07,
	RMF_SI_ELEM_DSM_CC = 0x08,
	RMF_SI_ELEM_H_222 = 0x09,
	RMF_SI_ELEM_DSM_CC_MPE = 0x0A,
	RMF_SI_ELEM_DSM_CC_UN = 0x0B,
	RMF_SI_ELEM_DSM_CC_STREAM_DESCRIPTORS = 0x0C,
	RMF_SI_ELEM_DSM_CC_SECTIONS = 0x0D,
	RMF_SI_ELEM_AUXILIARY = 0x0E,
	RMF_SI_ELEM_AAC_ADTS_AUDIO = 0x0F,
	RMF_SI_ELEM_ISO_14496_VISUAL = 0x10,
	RMF_SI_ELEM_AAC_AUDIO_LATM = 0x11,
	RMF_SI_ELEM_FLEXMUX_PES = 0x12,
	RMF_SI_ELEM_FLEXMUX_SECTIONS = 0x13,
	RMF_SI_ELEM_SYNCHRONIZED_DOWNLOAD = 0x14,
	RMF_SI_ELEM_METADATA_PES = 0x15,
	RMF_SI_ELEM_METADATA_SECTIONS = 0x16,
	RMF_SI_ELEM_METADATA_DATA_CAROUSEL = 0x17,
	RMF_SI_ELEM_METADATA_OBJECT_CAROUSEL = 0x18,
	RMF_SI_ELEM_METADATA_SYNCH_DOWNLOAD = 0x19,
	RMF_SI_ELEM_MPEG_2_IPMP = 0x1A,
	RMF_SI_ELEM_AVC_VIDEO = 0x1B,
	RMF_SI_ELEM_HEVC_VIDEO = 0x24,
	RMF_SI_ELEM_VIDEO_DCII = 0x80,
	RMF_SI_ELEM_ATSC_AUDIO = 0x81,
	RMF_SI_ELEM_STD_SUBTITLE = 0x82,
	RMF_SI_ELEM_ISOCHRONOUS_DATA = 0x83,
	RMF_SI_ELEM_ASYNCHRONOUS_DATA = 0x84,
	RMF_SI_ELEM_SCTE_35 = 0x86,
	RMF_SI_ELEM_ENHANCED_ATSC_AUDIO = 0x87,
	RMF_SI_ELEM_ETV_SCTE_SIGNALING = 0xC0
} rmf_SiElemStreamType;



/**
 * @brief This represents the descriptors which is variable-length byte arrays that extend the functionality
 * in MPEG-2 private table sections.
 * Some descriptors are limited in purpose and scope, and others have wide utility.
 * For more information, please refer ISO/IEC 13818-1 : 2000 (E) Pg.no:81-100.
 */
typedef enum rmf_SiDescriptorTag
{
    RMF_SI_DESC_VIDEO_STREAM = 0x02,
    RMF_SI_DESC_AUDIO_STREAM = 0x03,
    RMF_SI_DESC_HIERARCHY = 0x04,
    RMF_SI_DESC_REGISTRATION = 0x05,
    RMF_SI_DESC_DATA_STREAM_ALIGNMENT = 0x06,
    RMF_SI_DESC_TARGET_BACKGROUND_GRID = 0x07,
    RMF_SI_DESC_VIDEO_WINDOW = 0x08,
    RMF_SI_DESC_CA_DESCRIPTOR = 0x09,
    RMF_SI_DESC_ISO_639_LANGUAGE = 0x0A,
    RMF_SI_DESC_SYSTEM_CLOCK = 0x0B,
    RMF_SI_DESC_MULTIPLEX_UTILIZATION_BUFFER = 0x0C,
    RMF_SI_DESC_COPYRIGHT = 0x0D,
    RMF_SI_DESC_MAXIMUM_BITRATE = 0x0E,
    RMF_SI_DESC_PRIVATE_DATA_INDICATOR = 0x0F,
    RMF_SI_DESC_SMOOTHING_BUFFER = 0x10,
    RMF_SI_DESC_STD = 0x11,
    RMF_SI_DESC_IBP = 0x12,
    /** Current spec doesn't define RMF_SI_DESC_CAROUSEL_ID descriptor tag 
     * Needs to be added to DescriptorTagImpl */
    RMF_SI_DESC_CAROUSEL_ID = 0x13,
    RMF_SI_DESC_ASSOCIATION_TAG = 0x14,
    RMF_SI_DESC_DEFERRED_ASSOCIATION_TAG = 0x15,
    RMF_SI_DESC_APPLICATION = 0x00,
    RMF_SI_DESC_APPLICATION_NAME = 0x01,
    RMF_SI_DESC_TRANSPORT_PROTOCOL = 0x02,
    RMF_SI_DESC_DVB_J_APPLICATION = 0x03,
    RMF_SI_DESC_DVB_J_APPLICATION_LOCATION = 0x04,
    RMF_SI_DESC_EXTERNAL_APPLICATION_AUTHORISATION = 0x05,
    RMF_SI_DESC_IPV4_ROUTING = 0x06,
    RMF_SI_DESC_IPV6_ROUTING = 0x07,
    RMF_SI_DESC_APPLICATION_ICONS = 0x0B,
    RMF_SI_DESC_PRE_FETCH = 0x0C,
    RMF_SI_DESC_DII_LOCATION = 0x0D,
    RMF_SI_DESC_COMPONENT_DESCRIPTOR = 0x50,
    RMF_SI_DESC_STREAM_IDENTIFIER_DESCRIPTOR = 0x52,
    RMF_SI_DESC_PRIVATE_DATA_SPECIFIER = 0x5F,
    RMF_SI_DESC_DATA_BROADCAST_ID = 0x66,
    RMF_SI_DESC_APPLICATION_SIGNALING = 0x6F,
    RMF_SI_DESC_SERVICE_IDENTIFIER = 0x0D,
    RMF_SI_DESC_LABEL = 0x70,
    RMF_SI_DESC_CACHING_PRIORITY = 0x71,
    RMF_SI_DESC_CONTENT_TYPE = 0x72,
    RMF_SI_DESC_STUFFING = 0x80,
    RMF_SI_DESC_AC3_AUDIO = 0x81,
    RMF_SI_DESC_CAPTION_SERVICE = 0x86,
    RMF_SI_DESC_CONTENT_ADVISORY = 0x87,
    RMF_SI_DESC_REVISION_DETECTION = 0x93,
    RMF_SI_DESC_TWO_PART_CHANNEL_NUMBER = 0x94,
    RMF_SI_DESC_CHANNEL_PROPERTIES = 0x95,
    RMF_SI_DESC_DAYLIGHT_SAVINGS_TIME = 0x96,
    RMF_SI_DESC_EXTENDED_CHANNEL_NAME_DESCRIPTION = 0xA0,
    RMF_SI_DESC_SERVICE_LOCATION = 0xA1,
    RMF_SI_DESC_TIME_SHIFTED_SERVICE = 0xA2,
    RMF_SI_DESC_COMPONENT_NAME = 0xA3,
    RMF_SI_DESC_MAC_ADDRESS_LIST = 0xA4,
    RMF_SI_DESC_ATSC_PRIVATE_INFORMATION_DESCRIPTOR = 0xAD
} rmf_SiDescriptorTag;


/**
 * @brief Service component represents an abstraction of an elementary
 * stream. It provides information about individual components of a
 * service. Generally speaking, a service component carries content
 * such as media (e.g. as audio or video) or data, "sections", etc.).
 */
typedef enum rmf_SiServiceComponentType
{
    RMF_SI_COMP_TYPE_UNKNOWN = 0x00,
    RMF_SI_COMP_TYPE_VIDEO = 0x01,
    RMF_SI_COMP_TYPE_AUDIO = 0x02,
    RMF_SI_COMP_TYPE_SUBTITLES = 0x03,
    RMF_SI_COMP_TYPE_DATA = 0x04,
    RMF_SI_COMP_TYPE_SECTIONS = 0x05
} rmf_SiServiceComponentType;


/**
 * @brief It retrieves the value of PIDtype of audio,video,etc from the rmf_SiElemStremaType function.
 */
typedef struct _rmf_MediaPID
{
    rmf_SiElemStreamType pidType; //!< PID type: audio, video, etc. 
    uint32_t pid; //!< PID value. 
} rmf_MediaPID;



/**
 * @brief Represents a linked list of MPEG-2 descriptors.
 * Tag is the eight bit field that identifies each descriptor.
 * descriptor_length is the eight bit field specifying the number of bytes of the
 * descriptor immediately following the descriptor_length field.
 * descriptor_content is the data contained within this descriptor.
 * Next points to the next descriptor in the list
 */
typedef struct rmf_SiMpeg2DescriptorList
{
    rmf_SiDescriptorTag tag;
    uint8_t descriptor_length;
    void* descriptor_content;
    struct rmf_SiMpeg2DescriptorList* next;
} rmf_SiMpeg2DescriptorList;


/**
 * @brief Represents a list of specific language strings.
 */
typedef struct rmf_SiLangSpecificStringList
{
    char language[4]; //!< 3-character ISO 639 language code + null terminator.
    char *string;
    struct rmf_SiLangSpecificStringList *next;
} rmf_SiLangSpecificStringList;


/**
 * @brief Elementary stream list is a linked list of elementary streams with in a given service.
 * It also contains service component specific information such as the associated language, etc.
 */
typedef struct rmf_SiElementaryStreamList
{
    rmf_SiElemStreamType elementary_stream_type; // PMT
    /** If the PMT contains a component name descriptor then this
     should be the string extracted from the component_name_string () MSS
     string in this descriptor. Otherwise, 'service_comp_type' is the
     generic name associated with this stream type */
    rmf_SiServiceComponentType service_comp_type; //
    rmf_SiLangSpecificStringList* service_component_names; // PMT (desc)
    uint32_t elementary_PID; // PMT
    char associated_language[4]; // PMT (desc)
    uint32_t es_info_length; // PMT
    uint32_t number_desc; // PMT
    uint32_t ref_count;
    rmf_osal_Bool valid;
    // refers to the service this component belongs to
    uint32_t service_reference;
    rmf_SiMpeg2DescriptorList* descriptors; // PMT
    rmf_osal_TimeMillis ptime_service_component;
    struct rmf_SiElementaryStreamList* next;
} rmf_SiElementaryStreamList;

/** @} */

#endif /* __RMF_INBSI_COMMON_H__*/

