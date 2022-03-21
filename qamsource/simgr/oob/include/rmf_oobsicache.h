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
 

#ifndef _RMF_OOBSICACHE_H_
#define _RMF_OOBSICACHE_H_

#include "rmf_osal_mem.h"
#include "rmf_osal_sync.h"
#include "rdk_debug.h"
#include "rmf_osal_event.h"
#include "rmf_osal_error.h"
#include "rmf_osal_time.h"
#include "rmf_osal_util.h"
#include "rmf_osal_types.h"
#include "rmf_osal_file.h"
#include "rmf_sicache.h"
#include "rmf_qamsrc_common.h"
#include "rmf_oobsieventlistener.h"


/**
 * @file rmf_oobsicache.h
 * @brief File contains the out of band SI cache information
 */

/**
 * @defgroup rmfsistatus RMF SI Status
 * @ingroup OutbandMacros
 * @defgroup rmfpsistatus RMF PSI Status
 * @ingroup OutbandMacros
 * @defgroup rmfsidbevents RMF SIDB Events
 * @ingroup OutbandMacros
 * @defgroup rmfsifields RMF SI Fields
 * @ingroup OutbandMacros
 * @defgroup rmfsifrequency RMF SI Frequency
 * @ingroup OutbandMacros
 * @defgroup rmfsihandle RMF SI Handle
 * @ingroup OutbandMacros
 */

#define	stricmp	strcasecmp

#define RMF_SI_CACHE_FILE_VERSION   	0x101u

/* SI return error codes */
#define RMF_SI_ERROR_BASE               (0x00004000)
#define RMF_SI_SUCCESS                  (RMF_SUCCESS)


/**
 * @addtogroup rmfsistatus
 * The following events are generated when a handle (say using freq, prog_num) is requested.
 * @{
 */
/*  Ex: when a handle requested (say using freq, prog_num) is not found
 RMF_SI_ERROR_NOT_FOUND is returned
 */
#define RMF_SI_NOT_FOUND                (RMF_SI_ERROR_BASE + 1)
#define RMF_SI_OUT_OF_MEM               (RMF_SI_ERROR_BASE + 2)
#define RMF_SI_INVALID_PARAMETER        (RMF_SI_ERROR_BASE + 3)
#define RMF_SI_LOCKING_ERROR            (RMF_SI_ERROR_BASE + 4)

/*  Ex: when PSI info is requested but not yet acquired
 RMF_SI_ERROR_NOT_AVAILABLE is returned
 */
#define RMF_SI_NOT_AVAILABLE            (RMF_SI_ERROR_BASE + 5)
#define RMF_SI_NOT_AVAILABLE_YET        (RMF_SI_ERROR_BASE + 6)
/** @} */
/* Use this error case when PSI for a service entry is being updated */


/*  SI DB Events */
/**
 * @addtogroup rmfpsistatus
 * The following error case is used when PSI for a service entry is being updated
 * @{
 */
#define RMF_SI_PSI_INVALID              (RMF_SI_ERROR_BASE + 7)
#define RMF_SI_ERROR_ACQUIRING          (RMF_SI_ERROR_BASE + 8)
#define RMF_SI__IN_USE                  (RMF_SI_ERROR_BASE + 9)
#define RMF_SI_DSG_UNAVAILABLE          (RMF_SI_ERROR_BASE + 10)
#define RMF_SI_PARSE_ERROR          	(RMF_SI_ERROR_BASE + 11)
/** @} */


/**
 * @addtogroup rmfsidbevents
 * The following events are generated for all SI DB events, optional data 1 field contains the 'si handle' where
 * appropriate and the optional data 2 field contains the 'ACT' value
 * passed in at registration time. ('ACT' is the 'ed' handle).
 * @{
 */ 
#define RMF_OOBSI_EVENT_UNKNOWN                        (0x00003000)
#define RMF_SI_EVENT_OOB_VCT_ACQUIRED               (RMF_OOBSI_EVENT_UNKNOWN + 12)
#define RMF_SI_EVENT_OOB_NIT_ACQUIRED               (RMF_OOBSI_EVENT_UNKNOWN + 13)
#define RMF_SI_EVENT_OOB_PAT_ACQUIRED               (RMF_OOBSI_EVENT_UNKNOWN + 14)
#define RMF_SI_EVENT_OOB_PMT_ACQUIRED               (RMF_OOBSI_EVENT_UNKNOWN + 15)

#define RMF_SI_EVENT_TRANSPORT_STREAM_UPDATE        (RMF_OOBSI_EVENT_UNKNOWN + 16)
#define RMF_SI_EVENT_NETWORK_UPDATE                 (RMF_OOBSI_EVENT_UNKNOWN + 17)
#define RMF_SI_EVENT_SERVICE_DETAILS_UPDATE         (RMF_OOBSI_EVENT_UNKNOWN + 18)
/*
 For service component update event, the 'event_flag' field indicates the
 type of change (ADD/REMOVE/MODIFY)
 */
#if 0
#define RMF_SI_EVENT_SERVICE_COMPONENT_UPDATE           (RMF_SI_EVENT_UNKNOWN + 10)

#endif
#define RMF_SI_EVENT_OOB_PAT_UPDATE                     (RMF_OOBSI_EVENT_UNKNOWN + 19)
#define RMF_SI_EVENT_OOB_PMT_UPDATE                     (RMF_OOBSI_EVENT_UNKNOWN + 20)

/* This event is used to terminate the ED session and delete the associatd ED handle */
#if 0
#define RMF_SI_EVENT_SI_ACQUIRING                   (RMF_SI_EVENT_UNKNOWN + 15)
#define RMF_SI_EVENT_SI_NOT_AVAILABLE_YET           (RMF_SI_EVENT_UNKNOWN + 16)
#define RMF_SI_EVENT_SI_FULLY_ACQUIRED              (RMF_SI_EVENT_UNKNOWN + 17)
#define RMF_SI_EVENT_SI_DISABLED                    (RMF_SI_EVENT_UNKNOWN + 18)
#define RMF_SI_EVENT_TUNED_AWAY                     (RMF_SI_EVENT_UNKNOWN + 19)
#define RMF_SI_EVENT_TERMINATE_SESSION              (RMF_SI_EVENT_UNKNOWN + 20)
#endif
#define RMF_SI_EVENT_NIT_SVCT_ACQUIRED              (RMF_OOBSI_EVENT_UNKNOWN + 21)
#define RMF_SI_EVENT_STT_ACQUIRED		    (RMF_OOBSI_EVENT_UNKNOWN + 22)

#ifdef ENABLE_TIME_UPDATE
#define RMF_SI_EVENT_STT_TIMEZONE_SET		    (RMF_OOBSI_EVENT_UNKNOWN + 23)
#define RMF_SI_EVENT_STT_SYSTIME_SET		    (RMF_OOBSI_EVENT_UNKNOWN + 24)
#endif

#define RMF_SI_CHANGE_TYPE_ADD                              0x01
#define RMF_SI_CHANGE_TYPE_REMOVE                           0x02
#define RMF_SI_CHANGE_TYPE_MODIFY                           0x03
/** @} */


/**
 * @addtogroup rmfsihandle
 * The following handles represents the default values of SI Handles.
 * @{
 */
/* SI Handles, default values */
#define RMF_SI_NUM_TRANSPORTS                   0x1
#define RMF_SI_DEFAULT_TRANSPORT_ID             0x1
#define RMF_SI_DEFAULT_TRANSPORT_HANDLE         0x1000

#define RMF_SI_NUM_NETWORKS                     0x1
#define RMF_SI_DEFAULT_NETWORK_ID               0x1
#define RMF_SI_DEFAULT_NETWORK_HANDLE           0x2000
#define RMF_SI_DEFAULT_CHANNEL_NUMBER           0xFFFFFFFF

#define RMF_SI_INVALID_HANDLE                   0
/** @} */


/**
 * @addtogroup rmfsifields
 * The following macros represents the default values for SI fields (OCAP specified)
 * sourceId and tsId are set by default to -1
 * @{
 */
/* Default values for SI fields (OCAP specified) */
/* sourceId and tsId are set by default to -1 */
#define SOURCEID_UNKNOWN                (0xFFFFFFFF)
#define TSID_UNKNOWN                    (0xFFFFFFFF) // ECR 1072
#define RMF_SI_DEFAULT_PROGRAM_NUMBER           (0xFFFFFFFF)
#define PROGRAM_NUMBER_UNKNOWN                  (0xFFFF)
#define VIRTUAL_CHANNEL_UNKNOWN                 (0xFFFF)

#define NUMBER_NON_UNIQUE_SOURCEIDS    10
/** @} */


/**
 * @addtogroup rmfsifrequency
 * The following handles represents the frequency values of OOB manager.
 * @{
 */
/** OOB frequency -1 */
#define RMF_SI_OOB_FREQUENCY                0xFFFFFFFF
/** HN frequency -2 */
#define RMF_SI_HN_FREQUENCY                 0xFFFFFFFE
/** Separate OOB and DSG such that they have different
 * frequencies. We may need to support legacy OOB as well
 * as DSG at which point it will become necessary that
 * they have different internal frequency values
 * DSG frequency -3
 */
#define RMF_SI_DSG_FREQUENCY                 0xFFFFFFFD

#define MAX_FREQUENCIES 255
/*  Data structures and Methods defined by SI DB */

#define  MIN_DSG_APPID  1
#define  MAX_DSG_APPID  65535

#define MOD_STRING(mode) ((mode == RMF_MODULATION_QAM_NTSC) ? "NTSC" : (((mode <= 0) || (mode > RMF_MODULATION_QAM1024)) ? "UNKNOWN" : modulationModes[mode]))

#define        PROFILE_CABLELABS_OCAP_1                1

#define CRC_QUOTIENT 0x04C11DB7
/** @} */


/**
 * @enum  rmf_SiGlobalState
 * @brief This enumeration represents the SI table global states such as acquring, SI_NOT_AVAILABLE_YET, SI_NIT_SVCT_ACQUIRED and so on.
 * @ingroup Outofbanddatastructures
 */
typedef enum rmf_SiGlobalState
{
	SI_ACQUIRING = 0x00,
	SI_NOT_AVAILABLE_YET = 0x01,
	SI_NIT_SVCT_ACQUIRED = 0x02,
	SI_FULLY_ACQUIRED = 0x03,
	SI_DISABLED = 0x04,
	SI_STT_ACQUIRED = 0x05
// when OOB SI is disabled
} rmf_SiGlobalState;

#if 0
typedef enum rmf_SiDeliverySystemType
{
	RMF_SI_DELIVERY_SYSTEM_TYPE_CABLE = 0x00,
	RMF_SI_DELIVERY_SYSTEM_TYPE_SATELLITE = 0x01,
	RMF_SI_DELIVERY_SYSTEM_TYPE_TERRESTRIAL = 0x02,
	RMF_SI_DELIVERY_SYSTEM_TYPE_UNKNOWN = 0x03
} rmf_SiDeliverySystemType;
#endif

#if 0
typedef enum rmf_SiServiceInformationType
{
	RMF_SI_SERVICE_INFORMATION_TYPE_ATSC_PSIP = 0x00,
	RMF_SI_SERVICE_INFORMATION_TYPE_DVB_SI = 0x01,
	RMF_SI_SERVICE_INFORMATION_TYPE_SCTE_SI = 0x02,
	RMF_SI_SERVICE_INFORMATION_TYPE_UNKNOWN = 0x03
} rmf_SiServiceInformationType;
#endif

#if 0
/**
 * The DescriptorTag contains constant values to be read from the
 * descriptor_tag field in a Descriptor.  The constant values will
 * correspond to those specified in OCAP SI.
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
    /* Current spec doesn't define RMF_SI_DESC_CAROUSEL_ID descriptor tag */
    /* Needs to be added to DescriptorTagImpl */
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
    RMF_SI_DESC_LABEL
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
#endif


/**
 * @enum rmf_SiServiceType
 * @brief Service type values represents "digital television",
 * "digital radio", "NVOD reference service", "NVOD time-shifted service",
 * "analog television", "analog radio", "data broadcast" and "application".
 *
 * (These values are mappable to the ATSC service type in the VCT table and
 * the DVB service type in the Service Descriptor.)
 * @ingroup Outofbanddatastructures
 */
typedef enum rmf_SiServiceType
{
    RMF_SI_SERVICE_TYPE_UNKNOWN = 0x00,
    RMF_SI_SERVICE_ANALOG_TV = 0x01,
    RMF_SI_SERVICE_DIGITAL_TV = 0x02,
    RMF_SI_SERVICE_DIGITAL_RADIO = 0x03,
    RMF_SI_SERVICE_DATA_BROADCAST = 0x04,
    RMF_SI_SERVICE_NVOD_REFERENCE = 0x05,
    RMF_SI_SERVICE_NVOD_TIME_SHIFTED = 0x06,
    RMF_SI_SERVICE_ANALOG_RADIO = 0x07,
    RMF_SI_SERVICE_DATA_APPLICATION = 0x08
} rmf_SiServiceType;

#if 0
/**
 * Service component represents an abstraction of an elementary
 * stream. It provides information about individual components of a
 * service.  Generally speaking, a service component carries content
 * such as media (e.g. as audio or video) or data.
 * ServiceComponent stream types (e.g., "video", "audio", "subtitles", "data",
 * "sections", etc.).
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
 * This represents valid values for the elementarty stream_type field
 * in the PMT.
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
 RMF_SI_ELEM_VIDEO_DCII = 0x80,
 RMF_SI_ELEM_ATSC_AUDIO = 0x81,
 RMF_SI_ELEM_STD_SUBTITLE = 0x82,
 RMF_SI_ELEM_ISOCHRONOUS_DATA = 0x83,
 RMF_SI_ELEM_ASYNCHRONOUS_DATA = 0x84
 } rmf_SiElemStreamType;
#endif

//SS - Commented
#if 0
/**
 *  Modulation modes
 */
 typedef enum rmf_SiModulationMode
 {
 RMF_SI_MODULATION_UNKNOWN=0,
 RMF_SI_MODULATION_QPSK,
 RMF_SI_MODULATION_BPSK,
 RMF_SI_MODULATION_OQPSK,
 RMF_SI_MODULATION_VSB8,
 RMF_SI_MODULATION_VSB16,
 RMF_SI_MODULATION_QAM16,
 RMF_SI_MODULATION_QAM32,
 RMF_SI_MODULATION_QAM64,
 RMF_SI_MODULATION_QAM80,
 RMF_SI_MODULATION_QAM96,
 RMF_SI_MODULATION_QAM112,
 RMF_SI_MODULATION_QAM128,
 RMF_SI_MODULATION_QAM160,
 RMF_SI_MODULATION_QAM192,
 RMF_SI_MODULATION_QAM224,
 RMF_SI_MODULATION_QAM256,
 RMF_SI_MODULATION_QAM320,
 RMF_SI_MODULATION_QAM384,
 RMF_SI_MODULATION_QAM448,
 RMF_SI_MODULATION_QAM512,
 RMF_SI_MODULATION_QAM640,
 RMF_SI_MODULATION_QAM768,
 RMF_SI_MODULATION_QAM896,
 RMF_SI_MODULATION_QAM1024,
 RMF_SI_MODULATION_QAM_NTSC // for analog mode
 } rmf_SiModulationMode;
#else
/*typedef enum rmf_ModulationMode
 {
 RMF_MODULATION_UNKNOWN=0,
 RMF_MODULATION_QPSK,
 RMF_MODULATION_BPSK,
 RMF_MODULATION_OQPSK,
 RMF_MODULATION_VSB8,
 RMF_MODULATION_VSB16,
 RMF_MODULATION_QAM16,
 RMF_MODULATION_QAM32,
 RMF_MODULATION_QAM64,
 RMF_MODULATION_QAM80,
 RMF_MODULATION_QAM96,
 RMF_MODULATION_QAM112,
 RMF_MODULATION_QAM128,
 RMF_MODULATION_QAM160,
 RMF_MODULATION_QAM192,
 RMF_MODULATION_QAM224,
 RMF_MODULATION_QAM256,
 RMF_MODULATION_QAM320,
 RMF_MODULATION_QAM384,
 RMF_MODULATION_QAM448,
 RMF_MODULATION_QAM512,
 RMF_MODULATION_QAM640,
 RMF_MODULATION_QAM768,
 RMF_MODULATION_QAM896,
 RMF_MODULATION_QAM1024,
 RMF_MODULATION_QAM_NTSC // for analog mode
 } rmf_ModulationMode;*/
#endif

#if 0
typedef enum rmf_SiStatus
{
    SI_NOT_AVAILABLE = 0x00, SI_AVAILABLE_SHORTLY = 0x01, SI_AVAILABLE = 0x02
} rmf_SiStatus;
#endif


/**
 * @enum rmf_SiEntryState_
 * @brief This enumeration represents the state of a service entry based on of SVCT-DCM, SVCT-VCM signaling.
 * @ingroup Outofbanddatastructures
 */
typedef enum rmf_SiEntryState_
{
    SIENTRY_UNSPECIFIED = 0, SIENTRY_PRESENT_IN_DCM = 0x01, // Present in DCM
    SIENTRY_DEFINED = 0x02, // Defined in DCM
    SIENTRY_MAPPED = 0x04, // Mapped in VCM
    SIENTRY_DEFINED_MAPPED = SIENTRY_PRESENT_IN_DCM | SIENTRY_DEFINED
            | SIENTRY_MAPPED, // Marked defined in SVCT-DCM, mapped in SVCT-VCM
    SIENTRY_DEFINED_UNMAPPED = SIENTRY_PRESENT_IN_DCM | SIENTRY_DEFINED, // Marked defined in SVCT-DCM, not mapped in SVCT-VCM
    SIENTRY_UNDEFINED_MAPPED = SIENTRY_PRESENT_IN_DCM | SIENTRY_MAPPED, // Marked undefined in SVCT-DCM, mapped in SVCT-VCM
    SIENTRY_UNDEFINED_UNMAPPED = SIENTRY_PRESENT_IN_DCM
// Marked undefined SVCT-DCM, not mapped in SVCT-VCM
} rmf_SiEntryState;

#if 0
typedef enum rmf_TsEntryState
{
    TSENTRY_NEW, TSENTRY_OLD, TSENTRY_UPDATED
} rmf_TsEntryState;
#endif


/**
 * @enum  rmf_SiSourceType
 * @brief This enumeration represents the service information source types such as Out of Band, InBand and so on.
 * @ingroup Outofbanddatastructures
 */
typedef enum rmf_SiSourceType
{
    RMF_SOURCE_TYPE_UNKNOWN = 0x00, 
    RMF_SOURCE_TYPE_OOB = 0x01, 
    RMF_SOURCE_TYPE_IB = 0x02, 
    RMF_SOURCE_TYPE_PVR_FILE = 0x03, 
    RMF_SOURCE_TYPE_HN_STREAM = 0x04, 
    RMF_SOURCE_TYPE_DSG = 0x05
} rmf_SiSourceType;


/**
 * @struct rmf_SiRegistrationListEntry
 * @brief This represents the data structure of service information registration list entry.
 * @ingroup Outofbanddatastructures
 */
typedef struct rmf_SiRegistrationListEntry
{
#ifdef ENABLE_TIME_UPDATE
    uint32_t si_event_type;
#endif
    rmf_osal_eventqueue_handle_t queueId;
    struct rmf_SiRegistrationListEntry* next;
} rmf_SiRegistrationListEntry;



/**
 * @enum rmf_SiTableType
 * @brief This enumeration represents the service information table types such as NIT, PAT and so on.
 * @ingroup Outofbanddatastructures
 */
typedef enum rmf_SiTableType
{
    TABLE_UNKNOWN = 0,
    OOB_SVCT_VCM = 1,
    OOB_NTT_SNS = 2,
    OOB_NIT_CDS = 3,
    OOB_NIT_MMS = 4,
    OOB_PAT = 5,
    OOB_PMT = 6,
    IB_PAT = 7,
    IB_PMT = 8,
    IB_CVCT = 9,
    IB_EIT = 10,
    OOB_SVCT_DCM = 11
} rmf_SiTableType;


/**
 * @struct RMF_SI_DIAG_INFO
 * @brief This represents the data structure of the service information diagnostics such as frequency and modulation status.
 * @ingroup Outofbanddatastructures
 */
typedef struct RMF_SI_DIAG_INFO
{
    rmf_SiGlobalState g_si_state;
    rmf_osal_Bool g_SITPConditionSet;
    rmf_osal_Bool g_siOOBCacheEnable;
    rmf_osal_Bool g_siOOBCached;
    int g_siSTTReceived;
    uint32_t g_minFrequency;
    uint32_t g_maxFrequency;
    rmf_osal_Bool g_frequenciesUpdated;
    rmf_osal_Bool g_modulationsUpdated;
    rmf_osal_Bool g_channelMapUpdated;
    uint32_t g_numberOfServices;
}RMF_SI_DIAG_INFO;

#if 0
/**
 MPEG2 Descriptor List
 Represents a linked list of MPEG-2 descriptors.
 Tag is the eight bit field that identifies each descriptor.
 descriptor_length is the eight bit field specifying the number of bytes of the descriptor immediately following the descriptor_length field.
 descriptor_content is the data contained within this descriptor.
 Next points to the next descriptor in the list
 */
typedef struct rmf_SiMpeg2DescriptorList
{
    rmf_SiDescriptorTag tag;
    uint8_t descriptor_length;
    void* descriptor_content;
    struct rmf_SiMpeg2DescriptorList* next;
} rmf_SiMpeg2DescriptorList;

typedef struct rmf_SiLangSpecificStringList
{
    char language[4]; /* 3-character ISO 639 language code + null terminator */
    char *string;
    struct rmf_SiLangSpecificStringList *next;
} rmf_SiLangSpecificStringList;
#endif

#if 0
/**
 Elementary Stream List
 Elementary stream list is a linked list of elementary streams with in a given service.
 It also contains service component specific information such as the associated
 language, etc.
 */
typedef struct rmf_SiElementaryStreamList
{
    rmf_SiElemStreamType elementary_stream_type; // PMT
    /* If the PMT contains a component name descriptor then this
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

typedef enum rmf_SiPMTStatus
{
    PMT_NOT_AVAILABLE = 0x00,
    PMT_AVAILABLE_SHORTLY = 0x01,
    PMT_AVAILABLE = 0x02
} rmf_SiPMTStatus;

typedef enum rmf_SiVideoStandard
{
    VIDEO_STANDARD_NTSC = 0x00,
    VIDEO_STANDARD_PAL625 = 0x01,
    VIDEO_STANDARD_PAL525 = 0x02,
    VIDEO_STANDARD_SECAM = 0x03,
    VIDEO_STANDARD_MAC = 0x04,
    VIDEO_STANDARD_UNKNOWN = 0x05
} rmf_SiVideoStandard;
#endif


/**
 * @enum rmf_SiChannelType
 * @brief This enumeration represents the service information channel types such as normal, hidden and so on.
 * @ingroup Outofbanddatastructures
 */
typedef enum rmf_SiChannelType
{
    CHANNEL_TYPE_NORMAL = 0x00,
    CHANNEL_TYPE_HIDDEN = 0x01,
    CHANNEL_TYPE_RESERVED_2 = 0x02,
    CHANNEL_TYPE_RESERVED_3 = 0x03,
    CHANNEL_TYPE_RESERVED_4 = 0x04,
    CHANNEL_TYPE_RESERVED_5 = 0x05,
    CHANNEL_TYPE_RESERVED_6 = 0x06,
    CHANNEL_TYPE_RESERVED_7 = 0x07,
    CHANNEL_TYPE_RESERVED_8 = 0x08,
    CHANNEL_TYPE_RESERVED_9 = 0x09,
    CHANNEL_TYPE_RESERVED_10 = 0x0A,
    CHANNEL_TYPE_RESERVED_11 = 0x0B,
    CHANNEL_TYPE_RESERVED_12 = 0x0C,
    CHANNEL_TYPE_RESERVED_13 = 0x0D,
    CHANNEL_TYPE_RESERVED_14 = 0x0E,
    CHANNEL_TYPE_RESERVED_15 = 0x0F,
} rmf_SiChannelType;


/**
 * @enum rmf_SiGZGlobalState
 * @brief This enumeration represents the service information global states such as DAC_ID_CHANGED, STT_ACQUIRED  and so on.
 * @ingroup Outofbanddatastructures
 */
typedef enum
{
	RMF_OOBSI_GLI_SI_FULLY_ACQUIRED = 0x00,
	RMF_OOBSI_GLI_CHANNELMAP_ID_CHANGED = 0x01,
	RMF_OOBSI_GLI_DAC_ID_CHANGED = 0x02,	
	RMF_OOBSI_GLI_VCT_ID_CHANGED = 0x03,		
	RMF_OOBSI_GLI_STT_ACQUIRED = 0x04,			
}rmf_SiGZGlobalState;

typedef void (* rmf_siGZCallback_t) (rmf_SiGZGlobalState , uint16_t , uint32_t  );


#if 0
typedef struct rmf_SiPMTReference
{
    uint32_t pcr_pid;
    uint32_t number_outer_desc;
    uint32_t outer_descriptors_ref;
    uint32_t number_elem_streams;
    uint32_t elementary_streams_ref;
} rmf_SiPMTReference;
#endif


/**
 * @struct rmf_SiSourceIdList
 * @brief This represents the data structure of SiSourceIdList.
 * @ingroup Outofbanddatastructures
 */
typedef struct rmf_SiSourceIdList
{
    uint32_t source_id; //!< SVCT
    struct rmf_SiSourceIdList* next;
} rmf_SiSourceIdList;

/**
 * @struct rmf_siSourceNameEntry
 * @brief Source name table, by sourceID/AppID.
 * @ingroup Outofbanddatastructures
 */
typedef struct rmf_siSourceNameEntry
{
  rmf_osal_Bool appType;
  uint32_t id;              // if appType=FALSE, sourceID otherwise appID
  rmf_osal_Bool mapped;          // true if mapped by a virtual channel false for DSG service name
  rmf_SiLangSpecificStringList *source_names;      // NTT_SNS, CVCT
  rmf_SiLangSpecificStringList *source_long_names; // NTT_SNS
  struct rmf_siSourceNameEntry* next;              // next source name entry
} rmf_siSourceNameEntry;

/**
 * @struct _rmf_HnStreamSessionH
 * @brief This represents the data structure of HNstream session.
 * @ingroup Outofbanddatastructures
 */
typedef struct _rmf_HnStreamSessionH
{
    int unused1;
}
*rmf_HnStreamSession; /* Opaque streaming session handle. */


/**
 * @stuct rmf_SiTableEntry
 * @brief  Service represents an abstract view of what is generally referred to as
 * a television "service" or "channel".
 * The following data structure encapsulates all the information for a
 * service in one location.
 * @ingroup Outofbanddatastructures
 */
typedef struct rmf_SiTableEntry
{
    uint32_t ref_count;
    uint32_t activation_time; // SVCT
    rmf_osal_TimeMillis ptime_service;
    rmf_SiTransportStreamHandle ts_handle;
    rmf_SiProgramHandle program; // Program info for this service. Can be shared with other rmf_SiTableEntry elements,
  //  rmf_SiTransportStreamHandle ts_handle;
//    rmf_SiProgramInfo *program; // Program info for this service. Can be shared with other rmf_SiTableEntry elements,
    // and can be null (analog channels)
    uint32_t tuner_id; // '0' for OOB, start from '1' for IB
    rmf_osal_Bool valid;
    uint16_t virtual_channel_number;
    rmf_osal_Bool isAppType; // for DSG app
    uint32_t source_id; // SVCT
    uint32_t app_id; //
    rmf_osal_Bool dsgAttached;
    uint32_t dsg_attach_count; // DSG attach count
    rmf_SiEntryState state;
    //uint32_t state;
    rmf_SiChannelType channel_type; // SVCT
    //uint8_t channel_type; // SVCT
    rmf_SiVideoStandard video_standard; // SVCT (NTSC, PAL etc.)
    rmf_SiServiceType service_type; // SVCT (desc)

    rmf_siSourceNameEntry *source_name_entry;      // Reference to the corresponding NTT_SNS entry
    rmf_SiLangSpecificStringList *descriptions; // LVCT/CVCT (desc)
    uint8_t freq_index; // SVCT
    uint8_t mode_index; // SVCT
    uint32_t major_channel_number;
    uint32_t minor_channel_number; // SVCT (desc)

    uint16_t program_number;       // SVCT
    uint8_t  transport_type;       // SVCT (0==MPEG2, 1==non-MPEG2)
    rmf_osal_Bool scrambled;            // SVCT
    rmf_HnStreamSession hn_stream_session;  // HN stream session handle
    uint32_t hn_attach_count; // HN stream session PSI attach / registration count
    struct rmf_SiTableEntry* next; // next service entry
} rmf_SiTableEntry;

/*
 * *****************************************************************************
 *               Rating Region structures
 * *****************************************************************************
 */


/**
 * @struct  rmf_SiRatingValuesDefined
 * @brief This represents the data structure of SiRatingValuesDefined.
 * @ingroup Outofbanddatastructures
 */
typedef struct rmf_SiRatingValuesDefined
{
    rmf_SiLangSpecificStringList *abbre_rating_value_text;
    rmf_SiLangSpecificStringList *rating_value_text;
} rmf_SiRatingValuesDefined;


/**
 * @struct rmf_SiRatingDimension
 * @brief This represents the data structure of SiRatingDimensions such as scale and rating values.
 * @ingroup Outofbanddatastructures
 */
typedef struct rmf_SiRatingDimension
{
    rmf_SiLangSpecificStringList *dimension_names;
    uint8_t graduated_scale;
    uint8_t values_defined;
    rmf_SiRatingValuesDefined *rating_value;

} rmf_SiRatingDimension;


/**
 * @struct rmf_SiRatingRegion
 * @brief This represents the data structure of Service information rating region.
 * @ingroup Outofbanddatastructures
 */
typedef struct rmf_SiRatingRegion
{
    rmf_SiLangSpecificStringList *rating_region_name_text;
    uint8_t dimensions_defined;
    rmf_SiRatingDimension *dimensions;
} rmf_SiRatingRegion;



/**
 * @struct publicSIIterator
 * @brief This data structure represents the iterator for services attached to a program.
 * @ingroup Outofbanddatastructures
 */
//
// TransportStreams's list_lock is held from the return of 'FindFirst' until 'Release' is called.
//
typedef struct publicSIIterator
{
    uint32_t dummy1;
    uint32_t dummy2;
} publicSIIterator; //  Size/alignment must agree with private version, in simgr.c.

//Moved this structure to SiMgr class
#if 0
typedef struct RMF_SI_DIAG_INFO
{
    rmf_SiGlobalState g_si_state;
    rmf_osal_Bool g_SITPConditionSet;
    rmf_osal_Bool g_siOOBCacheEnable;
    rmf_osal_Bool g_siOOBCached;
    int g_siSTTReceived;
    uint32_t g_minFrequency;
    uint32_t g_maxFrequency;
    rmf_osal_Bool g_frequenciesUpdated;
    rmf_osal_Bool g_modulationsUpdated;
    rmf_osal_Bool g_channelMapUpdated;
    uint32_t g_numberOfServices;
}RMF_SI_DIAG_INFO;
#endif


/**
 * @struct rmf_SiTableEntry114
 * @brief This represents the data structures of service information table entry such as video standard, channel types and so on.
 * @ingroup Outofbanddatastructures
 */
typedef struct rmf_SiTableEntry114
{
    uint32_t ref_count;
    uint32_t activation_time; // SVCT
    rmf_osal_TimeMillis ptime_service;
    rmf_SiGenericHandle ts_handle;
    void *program; // Program info for this service. Can be shared with other rmf_SiTableEntry elements,
    // and can be null (analog channels)
    uint32_t tuner_id; // '0' for OOB, start from '1' for IB
    rmf_osal_Bool valid;
    uint16_t virtual_channel_number;
    rmf_osal_Bool isAppType; // for DSG app
    uint32_t source_id; // SVCT
    uint32_t app_id; //
    rmf_osal_Bool dsgAttached;
    uint32_t dsg_attach_count; // DSG attach count
    rmf_SiEntryState state;
    rmf_SiChannelType channel_type; // SVCT
    rmf_SiVideoStandard video_standard; // SVCT (NTSC, PAL etc.)
    rmf_SiServiceType service_type; // SVCT (desc)

    rmf_SiLangSpecificStringList *source_names; // NTT_SNS, CVCT
    rmf_SiLangSpecificStringList *source_long_names; // NTT_SNS
    rmf_SiLangSpecificStringList *descriptions; // LVCT/CVCT (desc)
    uint8_t freq_index; // SVCT
    uint8_t mode_index; // SVCT
    uint32_t major_channel_number;
    uint32_t minor_channel_number; // SVCT (desc)

    struct rmf_SiTableEntry114* next; // next service entry
} rmf_SiTableEntry114;

typedef rmf_SiTableEntry rmf_SiTableEntry121;

/* *End* Rating region US (50 states + possessions) */


/**
 * @union _rmf_timeinfo_t
 * @brief This union represents the structure of time zone information.
 * @ingroup Outofbanddatastructures
 */
#ifdef ENABLE_TIME_UPDATE
typedef union _rmf_timeinfo_t {
        struct _timezoneinfo_t {
                int timezoneinhours;
                int daylightflag;
        }timezoneinfo;
        int64_t timevalue;
}rmf_timeinfo_t;
#endif


/**
 * @class rmf_OobSiCache
 * @brief Class implements the out of band si cache information such as NIT, SVCT table parsing.
 * @ingroup OutofbandClasses
 */
class rmf_OobSiCache
{
private:
	rmf_OobSiCache();
	~rmf_OobSiCache();

	static rmf_OobSiCache *m_pSiCache;
	bool sttAquiredStatus;

        //int32_t g_sitp_si_status_update_time_interval;
        //int32_t g_sitp_pod_status_update_time_interval;
        uint32_t g_sitp_si_dump_tables;
/*        uint32_t g_sitp_si_update_poll_interval;
        uint32_t g_sitp_si_update_poll_interval_max;
        uint32_t g_sitp_si_update_poll_interval_min;
        uint32_t g_stip_si_process_table_revisions;*/
        uint32_t g_sitp_si_timeout;
        //uint32_t g_sitp_si_profile;
        //uint32_t g_sitp_si_profile_table_number;
        uint32_t g_sitp_si_versioning_by_crc32;
/*        uint32_t g_sitp_si_max_nit_cds_wait_time;
        uint32_t g_sitp_si_max_nit_mms_wait_time;
        uint32_t g_sitp_si_max_svct_dcm_wait_time;
        uint32_t g_sitp_si_max_svct_vcm_wait_time;
        uint32_t g_sitp_si_max_ntt_wait_time;*/
        //uint32_t g_sitp_si_section_retention_time;
        //uint32_t g_sitp_si_filter_multiplier;
/*        uint32_t g_sitp_si_max_nit_section_seen_count;
        uint32_t g_sitp_si_max_vcm_section_seen_count;
        uint32_t g_sitp_si_max_dcm_section_seen_count;
        uint32_t g_sitp_si_max_ntt_section_seen_count;*/
        //uint32_t g_sitp_si_initial_section_match_count;
        //uint32_t g_sitp_si_rev_sample_sections;
	rmf_osal_TimeMillis g_sitp_stt_time;
	uint32_t g_sitp_si_parse_stt;
        uint32_t g_sitp_si_stt_filter_match_count;
 

	/* Points to the top of the si_table_entry linked list */
	rmf_SiTableEntry *g_si_entry;

	rmf_siSourceNameEntry *g_si_sourcename_entry;

	/*  Frequency and modulation modes populated from NIT_CD and NIT_MM tables
	    are stored in the following arrays.  Note that we reference these arrays
	    starting at index 1, so make them one location bigger. */
	uint32_t g_frequency[MAX_FREQUENCIES + 1];
	rmf_ModulationMode g_mode[MAX_FREQUENCIES + 1];

	/*  Following variables are used to determine whether or not to
	    enforce frequency range check.
	 */
	const char *siOOBEnabled;
	rmf_osal_Bool g_frequencyCheckEnable;
	rmf_osal_Bool g_SITPConditionSet;

	/*  Maximum and minimum frequencies in the NIT_CDS table */
	uint32_t g_maxFrequency;
	uint32_t g_minFrequency;

	/*  Global SI lock used by all readers and writer (SITP)
	    Multiple readers can read at the same time, but all
	    readers are blocked when writer has the lock.
	 */
	rmf_osal_Cond g_global_si_cond;
	uint32_t g_cond_counter ;
	rmf_osal_Mutex g_global_si_mutex; /* sync primitive */

	/*  Lock for transport stream list */
	rmf_osal_Mutex g_global_si_list_mutex; /* sync primitive */

	/*  This condition is set by SITP when all OOB tables are discovered */
	rmf_osal_Cond g_SITP_cond;

	rmf_osal_Mutex g_dsg_mutex; /* DSG sync primitive */

	rmf_osal_Mutex g_HN_mutex; /* HN sync primitive */

	//rmf_SIQueueListEntry *g_si_queue_list;


	/*  System times at which transport streams and networks are created/updated */
	rmf_osal_TimeMillis gtime_transport_stream;
	rmf_osal_TimeMillis gtime_network;

	/*  Global variable to indicate the total number of services in the SI DB
	    This variable currently only includes those services that are signalled in
	    OOB SI (SVCT). */
	uint32_t g_numberOfServices;
	/* Global SI state */
	rmf_SiGlobalState g_si_state;

	/*  Control variables for the various tables */
	rmf_osal_Bool g_frequenciesUpdated;
	rmf_osal_Bool g_modulationsUpdated;
	rmf_osal_Bool g_channelMapUpdated;
        int g_siSTTReceived;

#ifdef ENABLE_TIME_UPDATE
	int32_t m_timezoneinhours;
	int32_t m_daylightflag;
#endif

	/*  Control variables for the si caching */
        rmf_osal_Bool     g_siOOBCacheEnable;
        rmf_osal_Bool     g_siOOBCacheConvertStatus;
        rmf_osal_Bool     g_siOOBCached;
        const char  *g_siOOBCachePost114Location;
        const char  *g_siOOBCacheLocation;

        const char  *g_siOOBSNSCacheLocation;

        char g_badSICacheLocation[250];
        char g_badSISNSCacheLocation[250];
        	/*
	 * This table should be initialized once
	 */
	uint32_t crctab[256];
       rmf_InbSiCache *m_pInbSiCache;
       /* One and only DSG transport stream */
      rmf_SiTransportStreamEntry *g_si_dsg_ts_entry;
      rmf_OobSiEventListener *m_pSiEventListener;
      uint16_t ChannelMapId;
      uint16_t DACId;	    
      uint16_t VCTId;	    
            
public:
	rmf_osal_Bool IsSIFullyAcquired( );
	bool getSTTAcquiredStatus();
	void setSTTAcquiredStatus( bool status );
	// service handle specific
	rmf_Error GetServiceHandleByServiceNumber(uint32_t majorNumber,
			uint32_t minorNumber, rmf_SiServiceHandle *service_handle);
	rmf_Error GetServiceHandleBySourceId(uint32_t sourceId,
			rmf_SiServiceHandle *service_handle);
	rmf_Error GetServiceHandleByVCN(uint16_t vcn,
			rmf_SiServiceHandle *service_handle);	
#if 0
	rmf_Error GetServiceHandleByAppId(uint32_t appId,
			rmf_SiServiceHandle *service_handle);
	rmf_Error GetServiceHandleByFrequencyModulationProgramNumber(
			uint32_t freq, uint32_t mode, uint32_t prog_num,
			rmf_SiServiceHandle *service_handle);
	rmf_Error GetServiceHandleByServiceName(char* service_name,
			rmf_SiServiceHandle *service_handle);

#endif
#if 0
	rmf_Error CreateDynamicServiceHandle(uint32_t freq, uint32_t prog_num,
			rmf_ModulationMode modFormat, rmf_SiServiceHandle *service_handle);
	rmf_Error
		CreateDSGServiceHandle(uint32_t appId, uint32_t prog_num,
				char* service_name, char* language,
				rmf_SiServiceHandle *service_handle);
	rmf_Error CreateHNstreamServiceHandle(mpe_HnStreamSession session, uint32_t prog_num,
			char* service_name, char* language,
			rmf_SiServiceHandle *service_handle);

#endif	// service handle based queries
	rmf_Error GetServiceTypeForServiceHandle(
			rmf_SiServiceHandle service_handle, rmf_SiServiceType *service_type);
	rmf_Error GetNumberOfLongNamesForServiceHandle(
			rmf_SiServiceHandle service_handle, uint32_t *num_long_names);
	rmf_Error GetLongNamesForServiceHandle(
			rmf_SiServiceHandle service_handle, char* languages[],
			char* service_long_names[]);
	rmf_Error GetNumberOfNamesForServiceHandle(
			rmf_SiServiceHandle service_handle, uint32_t *num_names);
	rmf_Error GetNamesForServiceHandle(rmf_SiServiceHandle service_handle,
			char* languages[], char* service_names[]);
	rmf_Error GetServiceNumberForServiceHandle(
			rmf_SiServiceHandle service_handle, uint32_t* service_number,
			uint32_t *minor_number);
        rmf_Error GetVirtualChannelNumberForServiceHandle(
                rmf_SiServiceHandle service_handle, uint16_t *virtual_channel_number);

	rmf_Error GetSourceIdForServiceHandle(rmf_SiServiceHandle service_handle,
			uint32_t* sourceId);
	rmf_Error GetFrequencyForServiceHandle(rmf_SiServiceHandle handle,
			uint32_t* frequency);
	rmf_Error GetProgramNumberForServiceHandle(
			rmf_SiServiceHandle service_handle, uint32_t *prog_num);
	rmf_Error GetModulationFormatForServiceHandle(rmf_SiServiceHandle handle,
			rmf_ModulationMode* modFormat);
	rmf_Error GetServiceDetailsLastUpdateTimeForServiceHandle(
			rmf_SiServiceHandle service_handle, rmf_osal_TimeMillis *time);
	rmf_Error GetIsFreeFlagForServiceHandle(
			rmf_SiServiceHandle service_handle, rmf_osal_Bool *is_free);
	rmf_Error GetNumberOfServiceDescriptionsForServiceHandle(
			rmf_SiServiceHandle service_handle, uint32_t *num_descriptions);
	rmf_Error GetServiceDescriptionsForServiceHandle(
			rmf_SiServiceHandle service_handle, char* languages[],
			char *descriptions[]);
	rmf_Error GetServiceDescriptionLastUpdateTimeForServiceHandle(
			rmf_SiServiceHandle service_handle, rmf_osal_TimeMillis *time);
;
	rmf_Error GetMultipleInstancesFlagForServiceHandle(
			rmf_SiServiceHandle service_handle, rmf_osal_Bool* hasMultipleInstances);
	rmf_Error GetOuterDescriptorsForServiceHandle(
			rmf_SiServiceHandle service_handle, uint32_t* numDescriptors,
			rmf_SiMpeg2DescriptorList** descriptors);
	rmf_Error GetNumberOfServiceDetailsForServiceHandle(
			rmf_SiServiceHandle serviceHandle, uint32_t* length);
	rmf_Error GetServiceDetailsForServiceHandle(
			rmf_SiServiceHandle serviceHandle, rmf_SiServiceDetailsHandle arr[],
			uint32_t length);
	rmf_Error GetAppIdForServiceHandle(rmf_SiServiceHandle service_handle,
			uint32_t* appId);
	rmf_Error ReleaseServiceHandle(rmf_SiServiceHandle service_handle);
	void SetAppId(rmf_SiServiceHandle service_handle, uint32_t appId);
	//rmf_Error UpdateFreqProgNumByServiceHandle(
	//		rmf_SiServiceHandle service_handle, uint32_t freq, uint32_t prog_num,
	//		uint32_t mod);

	// service details handle based queries
	rmf_Error GetIsFreeFlagForServiceDetailsHandle(
			rmf_SiServiceDetailsHandle handle, rmf_osal_Bool* is_free);
	rmf_Error GetSourceIdForServiceDetailsHandle(
			rmf_SiServiceDetailsHandle handle, uint32_t* sourceId);
	rmf_Error GetFrequencyForServiceDetailsHandle(
			rmf_SiServiceDetailsHandle handle, uint32_t* frequency);
	rmf_Error GetProgramNumberForServiceDetailsHandle(
			rmf_SiServiceDetailsHandle handle, uint32_t* progNum);
	rmf_Error GetModulationFormatForServiceDetailsHandle(
			rmf_SiServiceDetailsHandle handle, rmf_ModulationMode* modFormat);
	rmf_Error GetServiceTypeForServiceDetailsHandle(
			rmf_SiServiceDetailsHandle handle, rmf_SiServiceType* type);
	rmf_Error GetNumberOfLongNamesForServiceDetailsHandle(
			rmf_SiServiceDetailsHandle comp_handle, uint32_t *num_long_names);
	rmf_Error
		GetLongNamesForServiceDetailsHandle(
				rmf_SiServiceDetailsHandle handle, char* languages[],
				char* longNames[]);
	rmf_Error GetDeliverySystemTypeForServiceDetailsHandle(
			rmf_SiServiceDetailsHandle handle, rmf_SiDeliverySystemType* type);
	rmf_Error GetServiceInformationTypeForServiceDetailsHandle(
			rmf_SiServiceDetailsHandle handle, rmf_SiServiceInformationType* type);
	rmf_Error GetServiceDetailsLastUpdateTimeForServiceDetailsHandle(
			rmf_SiServiceDetailsHandle handle, rmf_osal_TimeMillis* lastUpdate);
	rmf_Error GetCASystemIdArrayLengthForServiceDetailsHandle(
			rmf_SiServiceDetailsHandle handle, uint32_t* length);
	rmf_Error GetCASystemIdArrayForServiceDetailsHandle(
			rmf_SiServiceDetailsHandle handle, uint32_t* ca_array, uint32_t length);
	//rmf_Error GetTransportStreamHandleForServiceDetailsHandle(
	//		rmf_SiServiceDetailsHandle handle,
	//		rmf_SiTransportStreamHandle *ts_handle);
	rmf_Error GetServiceHandleForServiceDetailsHandle(
			rmf_SiServiceDetailsHandle handle, rmf_SiServiceHandle* serviceHandle);
	rmf_Error GetTotalNumberOfServices(uint32_t *num_services);
	rmf_Error GetAllServices(rmf_SiServiceHandle array_services[],
			uint32_t* num_services);
        rmf_Error GetNumberOfServicesForTransportStreamHandle(
              rmf_SiTransportStreamHandle ts_handle, uint32_t *num_services);
	rmf_Error GetPidForServiceComponentHandle(
                rmf_SiServiceComponentHandle comp_handle, uint32_t *comp_pid);
	rmf_Error GetAllServicesForTransportStreamHandle(
                rmf_SiTransportStreamHandle ts_handle,
                rmf_SiServiceHandle array_service_handle[], uint32_t* num_services);

	rmf_Error FindPidInPMT(rmf_SiServiceHandle service_handle, int pid);

	// Rating dimension specific
	rmf_Error GetNumberOfSupportedRatingDimensions(uint32_t *num);
	rmf_Error GetSupportedRatingDimensions(rmf_SiRatingDimensionHandle arr[]);
	rmf_Error GetNumberOfLevelsForRatingDimensionHandle(
			rmf_SiRatingDimensionHandle handle, uint32_t *length);
	rmf_Error GetNumberOfNamesForRatingDimensionHandle(
			rmf_SiRatingDimensionHandle handle, uint32_t *num_names);
	rmf_Error GetNamesForRatingDimensionHandle(
			rmf_SiRatingDimensionHandle handle, char* languages[],
			char* dimensionNames[]);
	rmf_Error GetNumberOfNamesForRatingDimensionHandleAndLevel(
			rmf_SiRatingDimensionHandle handle, uint32_t *num_names,
			uint32_t *num_descriptions, int levelNumber);
	rmf_Error GetDimensionInformationForRatingDimensionHandleAndLevel(
			rmf_SiRatingDimensionHandle handle, char* nameLanguages[],
			char* names[], char* descriptionLanguages[], char* descriptions[],
			int levelNumber);
	rmf_Error GetRatingDimensionHandleByName(char* dimensionName,
			rmf_SiRatingDimensionHandle* outHandle);

	/* These methods are called from Table Parser layer to populate SI data as
	   various tables are acquired and parsed */
	/* SVCT */
	rmf_Error GetServiceEntryFromSourceId(uint32_t sourceId,
			rmf_SiServiceHandle *service_handle);
	//rmf_Error GetServiceEntryFromSourceIdAndChannel(uint32_t sourceId,
	//		uint32_t major_number, uint32_t minor_number,
	//		rmf_SiServiceHandle *service_handle, rmf_SiTransportHandle ts_handle,
	//		rmf_SiProgramHandle pi_handle);
	void GetFrequencyFromCDSRef(uint8_t cds_ref, uint32_t *frequency);
	void GetModulationFromMMSRef(uint8_t mms_ref,
			rmf_ModulationMode *modulation);
	
	rmf_Error LockForRead();
	rmf_Error UnLock();

	void LockForWrite(void);
	void ReleaseWriteLock(void);
	//rmf_Error GetServiceEntryFromFrequencyProgramNumberModulationAppId(
	//		uint32_t freq, uint32_t program_number, rmf_ModulationMode mode,
	//		uint32_t app_id, rmf_SiServiceHandle *si_entry_handle);
	//rmf_Error GetTransportStreamEntryFromFrequencyModulation(uint32_t freq,
	//		rmf_ModulationMode mode, rmf_SiTransportStreamHandle *ts_handle);
	rmf_Error GetServiceEntryFromFrequencyProgramNumberModulation(
			uint32_t frequency, uint32_t program_number, rmf_ModulationMode mode,
			rmf_SiServiceHandle *service_handle);

	rmf_Error GetNumberOfServiceEntriesForSourceId(uint32_t sourceId,
			uint32_t *num_entries);
	rmf_Error GetAllServiceHandlesForSourceId(uint32_t sourceId,
			rmf_SiServiceHandle service_handle[],
			uint32_t* num_entries);
	void InsertServiceEntryForChannelNumber(uint16_t channel_number,
			rmf_SiServiceHandle *service_handle);
	void GetServiceEntryFromChannelNumber(uint16_t channel_number,
			rmf_SiServiceHandle *service_handle);
	void UpdateServiceEntry(rmf_SiServiceHandle service_handle,
			rmf_SiTransportStreamHandle ts_handle, rmf_SiProgramHandle pi_handle);
	//rmf_Error GetServiceEntryFromAppIdProgramNumber(uint32_t appId,
	//		uint32_t program_number, rmf_SiServiceHandle *service_handle);
	//rmf_Error GetProgramEntryFromAppId(rmf_SiTransportStreamHandle ts_handle,
	//		uint32_t appId, rmf_SiProgramHandle *pi_handle);
	//rmf_Error GetTransportStreamEntryFromAppId(uint32_t appId,
	//		rmf_SiTransportStreamHandle *ts_handle);
	uint32_t GetDSGTransportStreamHandle(void);
	rmf_Error ReleaseServiceEntry(rmf_SiServiceHandle service_handle);
	//rmf_Error GetProgramEntryFromTransportStreamAppIdProgramNumber(
	//		rmf_SiTransportStreamHandle ts_handle, uint32_t appId,
	//		uint32_t program_number, rmf_SiProgramHandle *pi_handle);

	//rmf_Error
	//	GetProgramHandleFromServiceEntry(
	//			rmf_SiServiceHandle service_handle,
	//			rmf_SiProgramHandle *program_handle);

	//uint32_t GetHNTransportStreamHandle(void);
	//rmf_Error GetServiceEntryFromHNstreamProgramNumber(mpe_HnStreamSession session,
	//		uint32_t program_number, rmf_SiServiceHandle *service_handle);
	//rmf_Error GetProgramEntryFromTransportStreamHNstreamProgramNumber(
	//		rmf_SiTransportStreamHandle ts_handle, mpe_HnStreamSession session,
	//		uint32_t program_number, rmf_SiProgramHandle *pi_handle);
	/**
	  Set Methods: To populate individual fields from various tables
	 */
	rmf_Error GetNumberOfComponentsForServiceHandle(
                rmf_SiServiceHandle service_handle, uint32_t *num_components);
	rmf_Error GetAllComponentsForServiceHandle(
                rmf_SiServiceHandle service_handle,
                rmf_SiServiceComponentHandle comp_handle[], uint32_t* num_components);	
	rmf_Error GetDescriptorsForServiceComponentHandle(
                rmf_SiServiceComponentHandle comp_handle, uint32_t* numDescriptors,
                rmf_SiMpeg2DescriptorList** descriptors);
	rmf_Error GetStreamTypeForServiceComponentHandle(
                rmf_SiServiceComponentHandle comp_handle,
                rmf_SiElemStreamType *stream_type);

	/* SVCT */
	rmf_Error SetProgramNumber(rmf_SiServiceHandle service_handle,
			uint32_t prog_num);
	rmf_Error SetSourceId(rmf_SiServiceHandle service_handle,
			uint32_t sourceId);
	void SetAppType(rmf_SiServiceHandle service_handle, rmf_osal_Bool bIsApp);
	void GetServiceEntryState(rmf_SiServiceHandle service_handle,
			uint32_t *state);
	void SetServiceEntryState(rmf_SiServiceHandle service_handle,
			uint32_t state);
	void SetServiceEntryStateDefined(rmf_SiServiceHandle service_handle);
	void SetServiceEntryStateUnDefined(rmf_SiServiceHandle service_handle);
	void SetServiceEntryStateMapped(rmf_SiServiceHandle service_handle);
	void SetServiceEntriesStateDefined(uint16_t channel_number);
	void SetServiceEntriesStateUnDefined(uint16_t channel_number);
	void SetActivationTime(rmf_SiServiceHandle service_handle,
			uint32_t activation_time);
        rmf_Error SetVirtualChannelNumber(rmf_SiServiceHandle service_handle,
        uint32_t virtual_channel_number);
	rmf_Error SetChannelNumber(rmf_SiServiceHandle service_handle,
			uint32_t major_number, uint32_t minor_number);
	void SetCDSRef(rmf_SiServiceHandle service_handle, uint8_t cds_ref);
	void SetMMSRef(rmf_SiServiceHandle service_handle, uint8_t mms_ref);
	void SetChannelType(rmf_SiServiceHandle service_handle,
			uint8_t channel_type);
	void SetVideoStandard(rmf_SiServiceHandle service_handle,
			rmf_SiVideoStandard video_std);
	void SetServiceType(rmf_SiServiceHandle service_handle,
			rmf_SiServiceType service_type);
	void SetChannelMapId( uint16_t ChannelMapId );
	void GetChannelMapId( uint16_t *pChannelMapId );
	void SetDACId( uint16_t DACId );
	void GetDACId( uint16_t *pDACId );
	void SetVCTId( uint16_t VCTId );
	void GetVCTId( uint16_t *pVCTId );

	static rmf_siGZCallback_t rmf_siGZCBfunc;
	static rmf_Error rmf_SI_registerSIGZ_Callback(rmf_siGZCallback_t func);


#if 0
	void SetAnalogMode(rmf_SiServiceHandle service_handle,
			rmf_ModulationMode mode);
#endif
	rmf_Error SetSVCTDescriptor(rmf_SiServiceHandle service_handle,
			rmf_SiDescriptorTag tag, uint32_t length, void *content);
	rmf_osal_Bool IsVCMComplete(void);
	rmf_Error SetTransportType(rmf_SiServiceHandle service_handle, uint8_t transportType);
	rmf_Error SetScrambled(rmf_SiServiceHandle service_handle, rmf_osal_Bool scrambled);

	void UpdateServiceEntries();

	/* NTT_SNS / CVCT */
	rmf_Error SetSourceName(rmf_siSourceNameEntry *source_name_entry,
			char *sourceName, char *language, rmf_osal_Bool override_existing);

	/* NTT_SNS */
	rmf_Error SetSourceLongName(rmf_siSourceNameEntry *source_name_entry,
			char *sourceLongName, char *language);
	//rmf_Error SetSourceNameForSourceId(uint32_t source_id, char *sourceName,
	//        char *language, rmf_osal_Bool override_existing);
	//rmf_Error SetSourceLongNameForSourceId(uint32_t source_id,
	//        char *sourceName, char *language);
	rmf_Error GetSourceNameEntry(uint32_t id, rmf_osal_Bool isAppId,  rmf_siSourceNameEntry **source_name_entry, rmf_osal_Bool create);
	rmf_osal_Bool IsSourceNameLanguagePresent(rmf_siSourceNameEntry *source_name_entry, char *language);

	/* NIT_CD */
	/* Populate carrier frequencies from NIT_CD table */
	/*  Not sure about the signature, Talk to danny about how
	    frequencies might be populated */
	rmf_Error SetCarrierFrequencies(uint32_t frequency[], uint8_t offset,
			uint8_t length);

	/* NIT_MM */
	/* Populate modulation modes from NIT_MM table */
	/*  Not sure about the signature, Talk to danny about how
	    modes might be populated */
	rmf_Error SetModulationModes(rmf_ModulationMode mode[], uint8_t offset,
			uint8_t length);

	rmf_Error NotifyTableDetected(rmf_SiTableType table_type,
			uint32_t optional_data);

	rmf_Error NotifyTunedAway(uint32_t frequency);

	void SetGlobalState(rmf_SiGlobalState state);
	rmf_Error UnsetGlobalState(void);
	rmf_Error ChanMapReset(void);

	/*  Add methods for NTT_SNS (source name subtable)
	    Service(source) name is obtained from this table */

	//
	// Use these to validate when a handle is held through a time when there were
	// (potentially) no readers in the SIDB, so it might have been deleted!
	//
	// The case I stumbled on, where this occurs, is when an update occurs, and
	// SIDatabaseImpl or dcCarousel code is notified through an event.
	//
	// This begs the question, 'what do we DO in that situation?'
	// SIDatabaseImpl should cause the cache to be invalidated,
	// OC code should unmount the affected carousel, or signal an
	// error up somehow.
	//
	// These should be called after acquiring the reader/writer lock.
	//
	rmf_Error ValidateServiceHandle(rmf_SiServiceHandle service_handle);

//	rmf_SiServiceHandle FindFirstServiceFromProgram(
//			rmf_SiProgramHandle si_program_handle, SIIterator *iter);
//	rmf_SiServiceHandle GetNextServiceFromProgram(SIIterator *iter);

//	void TakeFromProgram(rmf_SiProgramHandle program_handle,
//			rmf_SiServiceHandle service_handle);
//	void GiveToProgram(rmf_SiProgramHandle program_handle,
//			rmf_SiServiceHandle service_handle);

//Moved this function to SiMgr class
#if 0
	rmf_Error GetSiDiagnostics(RMF_SI_DIAG_INFO * pDiagnostics);
#endif

	void BootEventSIGetData(int* channel);
#if 0
	rmf_Error IsServiceHandleAppType(rmf_SiServiceHandle service_handle,
                rmf_osal_Bool *isAppType);
#endif

	rmf_Error GetServiceHandleByServiceName(char* service_name,
                rmf_SiServiceHandle *service_handle);

	rmf_Error GetDeliverySystemTypeForServiceHandle(rmf_SiServiceHandle service_handle, rmf_SiDeliverySystemType *delivery_system_type);

	rmf_Error GetCASystemIdArrayLengthForServiceHandle(rmf_SiServiceHandle service_handle, uint32_t* length);

	rmf_Error GetCASystemIdArrayForServiceHandle(rmf_SiServiceHandle service_handle, uint32_t *ca_array, uint32_t *length);
	rmf_Error getSiEntryCount(void *status);
	rmf_Error getSiEntry(uint32_t index, uint32_t *size, void *status,
                void *statBuff_1024);
#if 0
	rmf_Error AcquiredbgStatus(mpe_DbgStatusId type, uint32_t *size, void *status,
                void *params);
#endif

	void TouchFile(const char * pszFile);

        rmf_Error GetSiDiagnostics(RMF_SI_DIAG_INFO * pDiagnostics);

	static rmf_OobSiCache *getSiCache();
	uint32_t getDumpTables(void);
	rmf_osal_TimeMillis getSTTStartTime(void);
	void setSTTStartTime(rmf_osal_TimeMillis sttStartTime);
	uint32_t getParseSTT();	
#ifdef ENABLE_TIME_UPDATE
	rmf_osal_Bool IsSttAcquired();
	void setSTTTimeZone(int32_t timezoneinhours, int32_t daylightflag);
	rmf_Error getSTTTimeZone(int32_t *pTimezoneinhours, int32_t *pDaylightflag);
#endif
	rmf_osal_Bool is_si_cache_enabled();
	rmf_osal_Bool is_si_cache_location_defined();
	rmf_Error LoadSiCache();
        rmf_Error LoadSiCache(char *siOOBCachePost114Location, char *siOOBSNSCacheLocation);
        rmf_Error CacheSiEntriesFromLive();
	rmf_SiGlobalState GetGlobalState();
       void SortCDSFreqList();

        void dump_si_cache();
	rmf_Error load_si_entries_114(const char *siOOBCacheLocation, const char *siOOBCachePost114Location, const char *siOOBSNSCacheLocation);
	rmf_Error load_si_entries_Post114(const char *siOOBCacheLocation);
	rmf_Error load_sns_entries(const char *siOOBSNSCacheLocation);       
       rmf_Error cache_si_entries(const char *siOOBCacheLocation);
	rmf_Error cache_sns_entries(const char *siOOBSNSCacheLocation);
        rmf_Error write_crc_for_si_and_sns_cache(const char* pSICache, const char* pSISNSCache);
        uint32_t get_dsg_transport_stream_handle(void);
        rmf_osal_Bool isVersioningByCRC32(void);
        uint32_t calc_mpeg2_crc(uint8_t * data, uint32_t len);
	void AddSiEventListener(rmf_OobSiEventListener *pSiEventListener);
	void RemoveSiEventListener();
	void SetAllSIEntriesUndefinedUnmapped(void);
private:

	void init_si_entry(rmf_SiTableEntry *si_entry);
	void clone_si_entry(rmf_SiTableEntry *new_entry,
                rmf_SiTableEntry *si_entry);

	void init_default_rating_region(void);
	void init_rating_dimension(rmf_SiRatingDimension *dimension,
                char* dimName, char* values[], char* abbrevValues[]);
	void delete_service_entry(rmf_SiTableEntry *si_entry);
	void release_si_entry(rmf_SiTableEntry *si_entry);

	rmf_Error read_name_string(rmf_osal_File cache_fd, rmf_SiLangSpecificStringList *name, uint32_t *read_count, int total_bytes);
	rmf_Error write_name_string(rmf_osal_File cache_fd, rmf_SiLangSpecificStringList *name, uint32_t *write_count);
	void get_frequency_range(uint32_t *freqArr, int count, uint32_t *minFreq, uint32_t *maxFreq );
	rmf_Error generate_new_si_entry(rmf_SiTableEntry *input_si_entry, rmf_SiTableEntry **new_si_entry_out);
	rmf_Error Configure();
       unsigned si_getFileSize(const char * location, unsigned int *size);
        void init_mpeg2_crc(void);
       rmf_Error write_crc (const char * pSICacheFileName, unsigned int crcValue);
       rmf_osal_Bool is_service_entry_available(list<rmf_SiServiceHandle> *services, rmf_SiTableEntry* walker);
       rmf_Error CreateDSGServiceHandle(uint32_t appID, uint32_t prog_num,
                char* service_name, char* language, rmf_SiServiceHandle *service_handle);
#if 0
	rmf_Error si_getServiceHandleByHNSession(mpe_HnStreamSession session,
        rmf_SiServiceHandle *service_handle);
#endif
        void checksifolder(const char *psFileName);
	rmf_Error verify_crc_for_si_and_sns_cache(const char* pSICache, const char* pSISNSCache, int *pStatus);
	int verify_version_and_crc (const char *siOOBCacheLocation, const char *siOOBSNSCacheLocation);
};

#endif /* _RMF_OOBSICACHE_H_ */
