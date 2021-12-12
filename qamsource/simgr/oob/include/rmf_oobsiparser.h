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
 

#ifndef _RMF_OOBSIPARSER_H_
#define _RMF_OOBSIPARSER_H_

/**
 * @file rmf_oobsiparser.h
 * @brief Parsing of SI Tables
 *
 * @defgroup OutbandMacros OOB SI Manager Macros
 * @ingroup Outofband
 * @defgroup SITableID SI Table ID's
 * @ingroup OutbandMacros
 * @defgroup SubTables SI Sub Table's
 * @ingroup OutbandMacros
 * @defgroup FECvalues FEC values
 * @ingroup OutbandMacros
 * @defgroup VideoStandards Video Standards
 * @ingroup OutbandMacros
 * @defgroup SubTablesAcquired Sub Table's Acquired
 * @ingroup OutbandMacros
 * @defgroup GETorTESTbits Get/Test Bits
 * @ingroup OutbandMacros
 */

//#include "rmf_psi.h"
//#include "rmf_si.h"

/** @addtogroup OutbandMacros
 * Macros to Get/Test bits from LSB to MSB.
 * @{
 */

/**
 * GetBits (in: Source, LSBit, MSBit)
 * <ul>
 * <li> Macro to extract values from LSBit to MSBit.
 * <li>Note :
 * <li> 1) Bits within an octet or any other unit of storage (word,
 * longword, doubleword, quadword, etc.) range from 1 through N
 * (the maximum number of bits in the unit of storage).
 * <li>
 * <li> 2) Bit #1 in any storage unit is the "absolute" least significant
 * bit(LSBit). However, depending upon where a field (a series of
 * bits) starts within a storage unit, the "virtual" LSBit associated
 * with the field may take on a value from 1 through N.
 * <li>
 * <li> 3) Bit #N in any storage unit is the "absolute" most significant
 * bit (MSBit). However, depending upon where a field (a series of
 * bits) ends within a storage unit, the "virtual" MSBit associated
 * with the field may take on a value from 1 through N.
 * <li>
 * <li> 4) For any field within any storage unit, the following inequality
 * must always be observed:
 * <li> a.)  LSBit  <= (less than or equal to) MSBit.
 * <li>
 * <li> 5) This macro extracts a field of  N bits (starting at LSBit and
 * ending at MSBit) from argument Source, moves the extract field
 * into bits 1 through N (where, N = 1 + (MSBit - LSBit)) of the register
 * before returning the extracted value to the caller.
 * <li>
 * <li> 6) This macro does not alter the contents of Source.
 * <li>
 * <li> return   (((((1 << (MSBit - LSBit + 1))-1) << (LSBit -1)) & Source)>> (LSBit -1))
 * <li>
 * <li>           |----------------------------| |------------| |----------||----------|
 * <li>                    Mask Generator          Mask Shift       Field       Right
 * <li>                                              Count      Extraction   Justifier
 * </ul>
 */
#define      GETBITS(Source, MSBit, LSBit)   ( \
            ((((1 << (MSBit - LSBit + 1))-1) << (LSBit -1)) & Source) >>(LSBit -1))

/**
 * TestBits (in: Source, LSBit, MSBit)
 * <ul>
 * <li> Macro to test the bits from LSBit to MSBit. It returns zero if none of the tested bits are set.
 * <li>
 * <li> Note :
 * <li> 1) Notes 1 through 4 from GetBits are applicable.
 * <li> 2) Source is the storage unit containing the field (defined by
 * LSBit and MSBit) to be tested. The test is a non destructive
 * test to source.
 * <li> 3) If 1 or more of the tested bits are set to "1", the value
 * returned from TestBits will be greater than or equal to 1.
 * Otherwise, the value returned is equal to zero.
 * <li>
 * <li> return    ((((1 << (MSBit - LSBit + 1))-1) << (LSBit -1)) & Source)
 * <li>
 * <li>             |----------------------------| |------------| |-------|
 * <li>                     Mask Generator           Mask Shift     Field
 * <li>                                               Count        Extraction
 * </ul>
 */
#define      TESTBITS(Source, MSBit, LSBit)   (\
         (((1 << (MSBit - LSBit + 1))-1) << (LSBit -1)) & Source)

#define      MIN_TIMEOUT(A,B)         ((A) <= (B) ? (A) : (B))

#define      PROGRAM_ASSOCIATION_TABLE_ID       0x00    // PMT_ID
#define      PROGRAM_MAP_TABLE_ID               0x02    // PAT_ID
#define      CABLE_VIRTUAL_CHANNEL_TABLE_ID     0xC9    // CVCT_ID
#define      CAT_PID                            0x0001    // conditional acess table packet identifier
#define      MIN_VALID_PID                      0x0010   // minimum valid value for packet identifier
#define      MAX_VALID_PID                      0x1FFE   // maximum valid value for packet identifier
#define      NO_PCR_PID                         0x1FFF   // Use when no PID carries the PCR
#define      MIN_VALID_PRIVATE_STREAM_TYPE      0x88   // minimum valid value for a private stream type
#define      MAX_VALID_PRIVATE_STREAM_TYPE      0xFF   // maximum valid value for a private stream type
// ECN-1176 added additional stream types supported by OCAP (ex: H.264 etc.)
// Hence the MIN and MAX values for non-private stream types is updated
#define      MIN_VALID_NONPRIVATE_STREAM_TYPE   0x01   // minimum valid value for a non-private stream type
#define      MAX_VALID_NONPRIVATE_STREAM_TYPE   0x87   // maximum valid value for a non-private stream type
#define      MAX_IB_SECTION_LENGTH              1021   //the maximum value that can be assigned to a
//PAT or PMT section_length field
#define      MAX_OOB_SMALL_TABLE_SECTION_LENGTH 1021   //the maximum value that can be assigned to a
// oob table other than MGT, L-VCT, AEIT, AETT
#define      MAX_OOB_LARGE_TABLE_SECTION_LENGTH 4093   //the maximum value that can be assigned to a
// oob table MGT, L-VCT, AEIT, AETT
#define      MAX_PROGRAM_INFO_LENGTH            1023   //the maximum value that can be assigned to a
//PMT program_info_length field
#define      MAX_ES_INFO_LENGTH                 1023   //the maximum value that can be assigned to a
//PMT es_info_length field
#define      MAX_VERSION_NUMBER                 31

#define      NO_PARSING_ERROR                   RMF_SUCCESS  //Should be equal to zero (0)
#define      LENGTH_CRC_32                      4           //length in bytes of a CRC_32
#define      LEN_OCTET1_THRU_OCTET3             3             //byte count for the 1st 3 byte in a structure,
#define      LEN_SECTION_HEADER                 3             //byte count for the 1st 3 byte in PAT/PMT
#define      SA_NON_CONFORMANCE_ISSUE

#define      NO_VERSION                         0xFF
#define      SIDB_DEFAULT_CHANNEL_NUMBER        0xFF

/////
#define        SA_NON_CONFORMANCE_ISSUE

#define        NETWORK_INFORMATION_TABLE_ID        0xC2    // NIT_ID
#define        NETWORK_TEXT_TABLE_ID               0xC3    // NTT_ID
#define        SF_VIRTUAL_CHANNEL_TABLE_ID         0xC4    // SVCT_ID
#define        SYSTEM_TIME_TABLE_ID                0xC5    // STT_ID
#define        YES                                 1
#define        NO                                  0
//#define        NIT_PID                           0x1FFC  // Network Information Table packet identifier
//#define        SVCT_PID                          0x1FFC  // Short-form Virtual Channel table
// packet identifier
#define        MIN_VALID_PID                       0x0010    // minimum valid value for packet identifier
#define        MAX_VALID_PID                       0x1FFE    // maximum valid value for packet identifier
#define        NO_PCR_PID                          0x1FFF    // Use when no PID carries the PCR
#define        MIN_VALID_PRIVATE_STREAM_TYPE       0x88    // minimum valid value for a private stream type
#define        MAX_VALID_PRIVATE_STREAM_TYPE       0xFF    // maximum valid value for a private stream type
// ECN-1176 added additional stream types supported by OCAP (ex: H.264 etc.)
// Hence the MIN and MAX values for non-private stream types is updated
#define        MIN_VALID_NONPRIVATE_STREAM_TYPE    0x01    // minimum valid value for a non-private stream type
#define        MAX_VALID_NONPRIVATE_STREAM_TYPE    0x87    // maximum valid value for a non-private stream type
#define        MAX_OOB_SI_SECTION_LENGTH           4096    //the maximum length of a NIT or SVCT table section
#define        MAX_IB_SECTION_LENGTH               1021    //the maximum value that can be assigned to a
//PAT or PMT section_length field
#define        MAX_PROGRAM_INFO_LENGTH             1023    //the maximum value that can be assigned to a
//PMT program_info_length field
#define        MAX_ES_INFO_LENGTH                  1023    //the maximum value that can be assigned to a
//PMT es_info_length field
#define        MAX_VERSION_NUMBER                  31

//#define        NO_PARSING_ERROR                    RMF_SUCCESS  //Should be equal to zero (0)
//#define        LENGTH_CRC_32                       4            //length in bytes of a CRC_32
//#define        LEN_OCTET1_THRU_OCTET3              3             //byte count for the 1st 3 byte in a structure,
#define        SIZE_OF_DESCRIPTOR_COUNT            1         //the size of the descriptor_count field in bytes
#define        CARRIER_DEFINITION_SUBTABLE         1
#define        MODULATION_MODE_SUBTABLE            2

#define        SOURCE_NAME_SUBTABLE                6

#define        FREQUENCY_UNITS_10KHz               10000
#define        FREQUENCY_UNITS_125KHz              125000
#define        MAX_CARRIER_FREQUENCY_INDEX         255

//#define        TRANSMISSION_SYSTEM_UNKNOWN                              0
//#define        TRANSMISSION_SYSTEM_RESERVED_ETSI                        1
//#define        TRANSMISSION_SYSTEM_ITUT_ANNEXB                          2
//#define        TRANSMISSION_SYSTEM_RESERVED_FOR_USE_IN_OTHER_SYSTEMS    3
//#define        TRANSMISSION_SYSTEM_ATSC                                 4
//#define        TRANSMISSION_SYSTEM_MIN_RESERVED_SATELLITE               5
//#define        TRANSMISSION_SYSTEM_MAX_RESERVED_SATELLITE               15


#define        INNER_CODING_MODE_5_11               0        // rate 5/11 coding
#define        INNER_CODING_MODE_1_2                1        // rate 1/2 coding
#define        INNER_CODING_MODE_3_5                3        // rate 3/5 coding
#define        INNER_CODING_MODE_2_3                5        // rate 2/3 coding
#define        INNER_CODING_MODE_3_4                7        // rate 3/4 coding
#define        INNER_CODING_MODE_4_5                8        // rate 4/5 coding
#define        INNER_CODING_MODE_5_6                9        // rate 5/6 coding
#define        INNER_CODING_MODE_7_8                11        // rate 7/8 coding
#define        DOES_NOT_USE_CONCATENATED_CODING     15        // rate 5/6 coding
#define        MODULATION_FORMAT_UNKNOWN            0
#define        MODULATION_FORMAT_QAM_1024           24

#define        VIRTUAL_CHANNEL_MAP                  0
#define        DEFINED_CHANNEL_MAP                  1
#define        INVERSE_CHANNEL_MAP                  2

#define        PATH_1_SELECTED                      0
#define        PATH_2_SELECTED                      1

#define        MPEG_2_TRANSPORT                     0
#define        NON_MPEG_2_TRANSPORT                 1

#define        NORMAL_CHANNEL                       0
#define        HIDDEN_CHANNEL                       1

#define        SNS_TABLE_SUBTYPE                    6

#define        NTSC_VIDEO_STANDARD                  0
#define        PAL625_VIDEO_STANDARD                1
#define        PAL525_VIDEO_STANDARD                2
#define        SECAM_VIDEO_STANDARD                 3
#define        MAC_VIDEO_STANDARD                   4
#define        MIN_RESERVED_VIDEO_STANDARD          5
#define        MAX_RESERVED_VIDEO_STANDARD          15
#define        IMMEDIATE_ACTIVATION                 0

#define        SIMODULE                             "SITP_SI"// module name
#define        OOB_START_UP_EVENT                   750
#define        OOB_VCTID_CHANGE_EVENT               OOB_START_UP_EVENT + 1

#define        SITP_SI_INIT_VERSION                 32            // range is 0-31
#define        SITP_SI_INIT_CRC_32                  0xFFFFFFFF    // -1
#define        NOT_USED                             0
#define        SYSTEM_STARTUP                       0
#define        TBS                                  0             // To Be Supplied
#define        MAX_TABLE_SUBTYPE                    10
#define        MAX_SECTION_NUMBER                   64

#define        CDS_ACQUIRED                         0x0001
#define        MMS_ACQUIRED                         0x0002
#define        NIT_ACQUIRED                         0x0003
#define        VCM_ACQUIRED                         0x0004
#define        DCM_ACQUIRED                         0x0008
#define        SVCT_ACQUIRED                        0x000C
#define        NIT_SVCT_ACQUIRED                    0x000F
#define        NTT_ACQUIRED                         0x0010
#define        TABLES_ACQUIRED                      0x001F
#define        SIDB_SIGNALED                        0x0020
#define        OOB_TABLES_ACQUIRED                  0x003F
#define        WAIT_FOREVER                         0
#define        PROFILE_CABLELABS_OCAP_1                1

#define        NUM_INIT_TABLES                      2

#define        SHORT_TIMER                          100
#define DAC_SOURCEID	0xacdd 
#define CHMAP_SOURCEID	0xacdc

/** @} */
/////


/* SI DB Supported handle types */
typedef uint32_t rmf_SiTransportHandle;
typedef uint32_t rmf_SiNetworkHandle;
typedef uintptr_t rmf_SiTransportStreamHandle;
typedef uintptr_t rmf_SiServiceHandle;
typedef uintptr_t rmf_SiProgramHandle;
typedef uint32_t rmf_SiServiceDetailsHandle;
typedef uintptr_t rmf_SiServiceComponentHandle;
typedef uintptr_t rmf_SiRatingDimensionHandle;
typedef uintptr_t rmf_SiGenericHandle; // Cast before use...

/*
 ** ***************************************************************************
 **                               enumerations
 ** ***************************************************************************
 */

/**
 * @defgroup QAMSRC_OOB_SI_PARSER_DS OOB SI Parser Data Structures
 * @ingroup Outofband
 * @{
 * @struct generic_descriptor_struct
 * @brief Defines a structure to hold the tag and length of the generic descriptor.
 */
typedef struct
{ // see document ISO/IEC 13818-1:2000(E),pg.46 and pg. 112
    uint8_t descriptor_tag_octet1;
    uint8_t descriptor_length_octet2;
} generic_descriptor_struct;

/**
 * @enum _rmf_si_profile_e
 * @brief This enum is used to indicate the different operational profiles.
 * Please refer to document : ANSI/SCTE 65 2008, pg.67
 */
typedef enum _rmf_si_profile_e
{
        RMF_SI_PROFILE_1 = 1,
        RMF_SI_PROFILE_2,
        RMF_SI_PROFILE_3,
        RMF_SI_PROFILE_4,
        RMF_SI_PROFILE_5,
        RMF_SI_PROFILE_6,
        RMF_SI_PROFILE_NONE = 0

} rmf_si_profile_e;

/**
 * @enum _rmf_si_state_e
 * @brief To defines the status of SI table acquisition.
 */
typedef enum _rmf_si_state_e
{
        RMF_SI_STATE_WAIT_TABLE_REVISION = 0,
        RMF_SI_STATE_WAIT_TABLE_COMPLETE,
        RMF_SI_STATE_NONE

} rmf_si_state_e;

/**
 * @enum _rmf_si_revisioning_type_e
 * @brief Indicates the revision type of the SI table.
 */
typedef enum _rmf_si_revisioning_type_e
{
        RMF_SI_REV_TYPE_UNKNOWN = 0, RMF_SI_REV_TYPE_RDD, RMF_SI_REV_TYPE_CRC

} rmf_si_revisioning_type_e;

#if 0 //Moved to rmf_sicache.h
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
#endif

/**
 * @enum oob_version_indices_t
 * @brief Indicates different versions or subtables of NIT and SVCT.
 */
typedef enum
{
    NIT_CDS_VERSION = 0,
    NIT_MMS_VERSION,
    SVCT_VCM_VERSION,
    SVCT_DCM_VERSION,
    SVCT_ICM_VERSION
} oob_version_indices_t;

/*
 *****************************************************************************
 * structure definitions and typedefs (RR)
 *****************************************************************************
 */
#if 0
typedef struct
{ // see document   ISO/IEC 13818-1:2000(E), pg.43
    uint8_t table_id_octet1;
    uint8_t section_length_octet2;
    uint8_t section_length_octet3;
    uint8_t transport_stream_id_octet4;
    uint8_t transport_stream_id_octet5;
    uint8_t current_next_indic_octet6;
    uint8_t section_number_octet7;
    uint8_t last_section_number_octet8;
} pat_table_struc1;

typedef struct
{ // see document ISO/IEC 13818-1:2000(E),pg.43
    uint8_t program_number_octet1;
    uint8_t program_number_octet2;
    uint8_t unknown_pid_octet3;
    uint8_t unknown_pid_octet4;
} pat_table_struc2;

typedef struct
{ // see document   ISO/IEC 13818-1:2000(E), pg.46
    uint8_t table_id_octet1;
    uint8_t section_length_octet2;
    uint8_t section_length_octet3;
    uint8_t program_number_octet4;
    uint8_t program_number_octet5;
    uint8_t current_next_indic_octet6;
    uint8_t section_number_octet7;
    uint8_t last_section_number_octet8;
    uint8_t pcr_pid_octet9;
    uint8_t pcr_pid_octet10;
    uint8_t program_info_length_octet11;
    uint8_t program_info_length_octet12;
} pmt_table_struc1;

typedef struct
{ // see document ISO/IEC 13818-1:2000(E),pg.46
    uint8_t stream_type_octet1;
    uint8_t elementary_pid_octet2;
    uint8_t elementary_pid_octet3;
    uint8_t es_info_length_octet4;
    uint8_t es_info_length_octet5;
} pmt_table_struc3;
#endif

/**
 * @struct nit_table_struc1
 * @brief Defines a structure to hold NIT information.
 * Refer to document ANSI/SCTE 65 2008, pg.12
 */
typedef struct
{ // see document ANSI/SCTE 65-2002 (formerly DVS 234), chap 5.1,pg.14
    uint8_t table_id_octet1;
    uint8_t section_length_octet2;
    uint8_t section_length_octet3;
    uint8_t protocol_version_octet4;
    uint8_t first_index_octet5;
    uint8_t number_of_records_octet6;
    uint8_t table_subtype_octet7;
} nit_table_struc1;

#if 0
typedef struct
{ // see document ISO/IEC 13818-1:2000(E),pg.46 and pg. 112
    uint8_t descriptor_tag_octet1;
    uint8_t descriptor_length_octet2;
} generic_descriptor_struc;
#endif

/**
 * @struct daylight_savings_descriptor_struc
 * @brief Defines a structure to hold Daylight Savings Time Descriptor.
 * Refer to document ANSI/SCTE 65 2008, pg.57
 */
typedef struct
{ // see document SCTE-65, pg 60
    uint8_t descriptor_tag_octet1;
    uint8_t descriptor_length_octet2;
    uint8_t DS_status;
    uint8_t DS_hour;
} daylight_savings_descriptor_struc;

/**
 * @struct revision_detection_descriptor_struc
 * @brief Defines a structure to hold Revision Detection Descriptor.
 * Refer to document ANSI/SCTE 65 2008, pg.52
 */
typedef struct
{
    // see doc ANSI/SCTE 65 2002, pg. 55.
    uint8_t descriptor_tag_octet1;
    uint8_t descriptor_length_octet2;
    uint8_t table_version_number_octet3;
    uint8_t section_number_octet4;
    uint8_t last_section_number_octet5;
} revision_detection_descriptor_struc;

/**
 * @struct cds_record_struc2
 * @brief Defines a structure to hold CDS.
 * Refer to document ANSI/SCTE 65 2008, pg.13
 */
typedef struct
{ // see document ANSI/SCTE 65-2002 (formerly DVS 234), chap 5.1.1,pg.16
    uint8_t number_of_carriers_octet1;
    uint8_t frequency_spacing_octet2;
    uint8_t frequency_spacing_octet3;
    uint8_t first_carrier_frequency_octet4;
    uint8_t first_carrier_frequency_octet5;
} cds_record_struc2;

/**
 * @struct mms_record_struc2
 * @brief Defines a structure to hold Modulation Mode Subtable.
 * Refer to document ANSI/SCTE 65 2008, pg.15
 */
typedef struct
{ // see document ANSI/SCTE 65-2002 (formerly DVS 234), chap 5.1.2,pg.18
    uint8_t inner_coding_mode_octet1;
    uint8_t modulation_format_octet2;
    uint8_t modulation_symbol_rate_octet3;
    uint8_t modulation_symbol_rate_octet4;
    uint8_t modulation_symbol_rate_octet5;
    uint8_t modulation_symbol_rate_octet6;
} mms_record_struc2;

/**
 * @struct svct_table_struc1
 * @brief Defines a structure to hold Short-form Virtual Channel Table.
 * Refer to document ANSI/SCTE 65 2008, pg.20
 */
typedef struct
{ // see document ANSI/SCTE 65-2002 (formerly DVS 234), chap 5.3,pg.23
    uint8_t table_id_octet1;
    uint8_t section_length_octet2;
    uint8_t section_length_octet3;
    uint8_t protocol_version_octet4;
    uint8_t table_subtype_octet5;
    uint8_t vct_id_octet6;
    uint8_t vct_id_octet7;
} svct_table_struc1;

/**
 * @struct dcm_record_struc2
 * @brief Defines a structure to hold Defined Channels Map's information.
 * Refer to document ANSI/SCTE 65 2008, pg.21
 */
typedef struct
{ // see document ANSI/SCTE 65-2002 (formerly DVS 234), chap 5.3.1,pg.24
    uint8_t first_virtual_chan_octet1;
    uint8_t first_virtual_chan_octet2;
    uint8_t dcm_data_length_octet_3;
} dcm_record_struc2;

/**
 * @struct dcm_record_struc2
 * @brief Defines a structure to hold Defined Channels Map's information.
 * Refer to document ANSI/SCTE 65 2008, pg.21
 */
typedef struct
{ // see document ANSI/SCTE 65-2002 (formerly DVS 234), chap 5.3.1,pg.24
    uint8_t channel_count_octet1;
} dcm_record_struc3;

/**
 * @struct vcm_record_struc2
 * @brief Defines a structure to hold Virtual Channel Map.
 * Refer to document ANSI/SCTE 65 2008, pg.22
 */
typedef struct
{ // see document ANSI/SCTE 65-2002 (formerly DVS 234), chap 5.3.2,pg.25
    uint8_t descriptors_included_octet1;
    uint8_t splice_octet2;
    uint8_t activation_time_octet3;
    uint8_t activation_time_octet4;
    uint8_t activation_time_octet5;
    uint8_t activation_time_octet6;
    uint8_t number_vc_records_octet7;
} vcm_record_struc2;

/**
 * @struct vcm_virtual_chan_record3
 * @brief  Defines a structure to hold Virtual channel record.
 * Refer to document ANSI/SCTE 65 2008, pg.22
 */
typedef struct
{ // see document ANSI/SCTE 65-2002 (formerly DVS 234), chap 5.3.2,pg.26
    uint8_t virtual_chan_number_octet1;
    uint8_t virtual_chan_number_octet2;
    uint8_t chan_type_octet3;
    uint8_t app_or_source_id_octet4;
    uint8_t app_or_source_id_octet5;
    uint8_t cds_reference_octet6;
} vcm_virtual_chan_record3;

/**
 * @struct mpeg2_virtual_chan_struc4
 * @brief Defines a structure to hold information of MPEG-2 virtual channel.
 * Refer to document ANSI/SCTE 65 2008, pg.23
 */
typedef struct
{ // see document ANSI/SCTE 65-2002 (formerly DVS 234), chap 5.3.2,pg.26
    uint8_t program_number_octet1;
    uint8_t program_number_octet2;
    uint8_t mms_number_octet3;
} mpeg2_virtual_chan_struc4;

/**
 * @struct nmpeg2_virtual_chan_struc5
 * @brief Defines a structure to hold information of non-MPEG-2 virtual channel.
 * Refer to document ANSI/SCTE 65 2008, pg.23
 */
typedef struct
{ // see document ANSI/SCTE 65-2002 (formerly DVS 234), chap 5.3.2,pg.26
    uint8_t video_standard_octet1;
    uint8_t eight_zeroes_octet2;
    uint8_t eight_zeroes_octet3;
} nmpeg2_virtual_chan_struc5;

/**
 * @struct icm_record_struc2
 * @brief Defines a structure to hold information of Inverse Channel Map.
 * Refer to document ANSI/SCTE 65 2008, pg.26
 */
typedef struct
{ // see document ANSI/SCTE 65-2002 (formerly DVS 234), chap 5.3.3,pg.29
    uint8_t first_map_index_octet1;
    uint8_t first_map_index_octet2;
    uint8_t record_count_octet3;
} icm_record_struc2;

/**
 * @struct icm_record_struc3
 * @brief Defines a structure to hold the data section of ICM.
 * Refer to document ANSI/SCTE 65 2008, pg.26
 */
typedef struct
{ // see document ANSI/SCTE 65-2002 (formerly DVS 234), chap 5.3.3,pg.29
    uint8_t source_id_octet1;
    uint8_t source_id_octet2;
    uint8_t virtual_chan_number_octet3;
    uint8_t virtual_chan_number_octet4;
} icm_record_struc3;

/**
 * @struct cvct_table_struc1
 * @brief Defines a structure to hold Cable Virtual Channel Table.
 * Refer to document ATSC Doc.A/65B - PSIP for Terrestrial Broadcast and Cable, section 6.3.2, pg.35
 */
typedef struct
{ // see document   ATSC Doc.A/65B - PSIP for Terrestrial Broadcast and Cable,
    // section 6.3.2, pg.35
    uint8_t table_id_octet1;
    uint8_t section_length_octet2;
    uint8_t section_length_octet3;
    uint8_t transport_stream_id_octet4;
    uint8_t transport_stream_id_octet5;
    uint8_t current_next_indic_octet6;
    uint8_t section_number_octet7;
    uint8_t last_section_number_octet8;
    uint8_t protocol_version_octet9;
    uint8_t num_channels_in_section_octet10;
} cvct_table_struc1;

/**
 * @struct cvct_table_struc2
 * @brief  Defines a structure to hold data section of CVCT.
 * Refer to document ATSC Doc.A/65B - PSIP for Terrestrial Broadcast and Cable, section 6.3.2, pg.35
 */
typedef struct
{ // see document   ATSC Doc.A/65B - PSIP for Terrestrial Broadcast and Cable,
    // section 6.3.2, pg.35
    uint8_t major_channel_number_octet1;
    uint8_t major_channel_number_octet2;
    uint8_t minor_channel_number_octet3;
    uint8_t modulation_mode_octet4;
    uint8_t carrier_frequency_octet5;
    uint8_t carrier_frequency_octet6;
    uint8_t carrier_frequency_octet7;
    uint8_t carrier_frequency_octet8;
    uint8_t channel_TSID_octet9;
    uint8_t channel_TSID_octet10;
    uint8_t program_number_octet11;
    uint8_t program_number_octet12;
    uint8_t access_controlled_hidden_octet13;
    uint8_t service_type_octet14;
    uint8_t source_id_octet15;
    uint8_t source_id_octet16;
    uint8_t descriptors_length_octet17;
    uint8_t descriptors_length_octet18;
} cvct_table_struc2;

/**
 * @struct cvct_table_struc3
 * @brief Defines a structure to indicate additional descriptor length for CVCT.
 * Refer to document ATSC Doc.A/65B - PSIP for Terrestrial Broadcast and Cable, section 6.3.2, pg.35
 */
typedef struct
{
    // see document   ATSC Doc.A/65B - PSIP for Terrestrial Broadcast and Cable,
    // section 6.3.2, pg.35
    uint8_t additional_descriptor_length_octet1;
    uint8_t additional_descriptor_length_octet2;
} cvct_table_struc3;

/**
 * @struct stt_table_struc1
 * @brief Defines a structure to hold System Time Table information.
 * Refer to document ANSI/SCTE 65 2008, pg.23
 */
typedef struct
{ // see document ANSI/SCTE 65-2002 (formerly DVS 234), chap 5.4,pg.30
    uint8_t table_id_octet1;
    uint8_t section_length_octet2;
    uint8_t section_length_octet3;
    uint8_t protocol_version_octet4;
    uint8_t zeroes_octet5;
    uint8_t system_time_octet6;
    uint8_t system_time_octet7;
    uint8_t system_time_octet8;
    uint8_t system_time_octet9;
    uint8_t GPS_UTC_offset_octet10;
} stt_table_struc1;

/**
 * @struct ntt_record_struc1
 * @brief Defines a structure to hold Network Text Table.
 * Refer to document ANSI/SCTE 65 2008, pg.17
 */
typedef struct
{ // see document ANSI/SCTE 65-2002 (formerly DVS 234), chap 5.2,pg.20
    uint8_t table_id_octet1;
    uint8_t section_length_octet2;
    uint8_t section_length_octet3;
    uint8_t protocol_version_octet4;
    uint8_t ISO_639_language_code_octet5;
    uint8_t ISO_639_language_code_octet6;
    uint8_t ISO_639_language_code_octet7;
    uint8_t table_subtype_octet8;
} ntt_record_struc1;

/**
 * @struct sns_record_struc1
 * @brief Defines a structure to hold Source Name Subtable.
 * Refer to document ANSI/SCTE 65 2008, pg.19
 */
typedef struct
{ // see document ANSI/SCTE 65-2002 (formerly DVS 234), chap 5.2,pg.22
    uint8_t number_of_SNS_records_octet1;
} sns_record_struc1;

/**
 * @struct sns_record_struc2
 * @brief Defines a structure to hold SNS record information.
 * Refer to document ANSI/SCTE 65 2008, pg.19
 */
typedef struct
{ // see document ANSI/SCTE 65-2002 (formerly DVS 234), chap 5.2,pg.22
    uint8_t application_type_octet1;
    uint8_t source_ID_octet2;
    uint8_t source_ID_octet3;
    uint8_t name_length_octet4;
} sns_record_struc2;

/**
 * @struct mts_format_struc1
 * @brief Defines a structure to hold Multilingual Text String information.
 * Refer to document ANSI/SCTE 65 2008, pg.59
 */
typedef struct
{ // see document ANSI/SCTE 65-2002 (formerly DVS 234), chap 7.1,pg.62
    uint8_t mode_octet1;
    uint8_t length_octet2;
} mts_format_struc1;

/*
 * @struct rmf_si_table_section_t
 * @brief Defines the structure for table section.
 */
typedef struct _rmf_si_table_section_t
{
    rmf_osal_Bool section_acquired;
    uint32_t crc_32;
    uint8_t crc_section_match_count;
    rmf_osal_TimeMillis last_seen_time;
} rmf_si_table_section_t;

/**
 * @struct _rmf_si_table_t
 * @brief Structure describing a SI table to be acquired.
 */
typedef struct _rmf_si_table_t
{
        uint8_t table_id;
        uint8_t subtype;
        rmf_si_state_e table_state;
        rmf_si_revisioning_type_e rev_type;
        uint32_t rev_sample_count;
        rmf_osal_Bool table_acquired;
        uint32_t table_unique_id;
        uint8_t version_number;
        uint8_t number_sections;
        rmf_osal_TimeMillis filter_start_time;
        rmf_si_table_section_t section[MAX_SECTION_NUMBER];
		uint32_t rolling_match_counter;
} rmf_si_table_t;

/** @} */

/**
 * @class rmf_OobSiParser
 * @brief Defines a class to manage, parsing and updating of SI tables.
 * @ingroup OutofbandClasses
 */
class rmf_OobSiParser
{
private:
	rmf_OobSiParser();
	rmf_OobSiParser(rmf_SectionFilter *pSectionFilter);

	rmf_Error parseAndUpdateSVCT(rmf_FilterSectionHandle section_handle,
			uint8_t *version, uint8_t *section_number,
			uint8_t *last_section_number, uint32_t *crc);
	rmf_Error parseAndUpdateNIT(rmf_FilterSectionHandle section_handle,
			uint8_t *version, uint8_t *section_number,
			uint8_t *last_section_number, uint32_t *crc);
	rmf_Error parseAndUpdateSTT(rmf_FilterSectionHandle section_handle,
			uint8_t *version, uint8_t *section_number,
			uint8_t *last_section_number, uint32_t *crc);
	rmf_Error parseAndUpdateNTT(rmf_FilterSectionHandle section_handle,
			uint8_t *version, uint8_t *section_number,
			uint8_t *last_section_number, uint32_t *crc);

	rmf_Error parseCDSRecords(uint16_t number_of_records, uint16_t first_index,
			uint8_t ** handle_section_data, uint32_t * section_length);
	rmf_Error parseMMSRecords(uint16_t number_of_records, uint16_t first_index,
			uint8_t ** handle_section_data, uint32_t *section_length);
	rmf_Error
		parseVCMStructure(uint8_t **ptr_TableSection, int32_t *section_length);
	rmf_Error
		parseDCMStructure(uint8_t **ptr_TableSection, int32_t *section_length);

	rmf_Error parseNIT_revision_data(rmf_FilterSectionHandle section_handle,
			uint8_t * version, uint8_t * section_number, uint8_t * number_sections,
			uint32_t * crc);
	rmf_Error parseNTT_revision_data(rmf_FilterSectionHandle section_handle,
			uint8_t * version, uint8_t * section_number, uint8_t * number_sections,
			uint32_t * crc);
	rmf_Error parseSVCT_revision_data(rmf_FilterSectionHandle section_handle,
			uint8_t * version, uint8_t * section_number, uint8_t * number_sections,
			uint32_t * crc);
	rmf_Error parseSTT_revision_data(rmf_FilterSectionHandle section_handle,
			uint8_t * version, uint8_t * section_number, uint8_t * number_sections,
			uint32_t * crc);

	rmf_Error parseSNSSubtable(uint8_t **pptr_TableSection,
			int32_t *ptr_section_length, char *language);

	void parseMTS(uint8_t **ptr_TableSection, uint8_t name_lenth, uint8_t * mode,
			uint8_t * length, uint8_t * segment);
	
	char * tableToString(uint8_t table);

	static rmf_OobSiParser *m_gpParserInst;
	rmf_SectionFilter *m_pSectionFilter;
	rmf_OobSiCache *m_pSiCache;

public:
	rmf_Error parseAndUpdateTable(rmf_si_table_t * ptr_table,
                                rmf_FilterSectionHandle section_handle, uint8_t * version,
                                uint8_t * section_number, uint8_t * last_section_number,
                                uint32_t * crc);

	rmf_Error get_revision_data(rmf_FilterSectionHandle section_handle,
			uint8_t * version, uint8_t * section_number, uint8_t * number_sections,
			uint32_t * crc);
    
	static rmf_OobSiParser* getParserInstance(rmf_SectionFilter *pSectionFilter);
};

#endif /* _RMF_OOBSIPARSER_H_ */
