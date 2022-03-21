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


 
#ifndef TS_H
#define TS_H

#include "cardmanagergentypes.h"
//#include "mr.h"
//#include "tsdecodeglobal.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MIN( a, b )         ( ( a < b )  ?  a : b )

#define kMAX_PSI_TABLE_SIZE     1024
#define kMAX_PRIVATE_PSI_SIZE   4096
#define TS_CRC_32_SIZE          4
#define TS_HEADER_SIZE          3

#define SECTION_NUMBER_OFFSET 7
#define NUMBER_OF_SECTIONS 32
#define EIT_TABLE_TYPE_BASE_VALUE 0x100
#define ETT_TABLE_TYPE_BASE_VALUE 0x200



// For Content Advisory Descriptor.
#define RATING_DIMENSIONS_SIZE 255
#define RATING_DESC_TEXT_SIZE   80
typedef struct
{
   uint8 rating_dimension_i;
   uint8 rating_value;
} rated_dimensions_t;

typedef struct
{
   uint8 rating_region;
   uint8 rated_dimensions;
   rated_dimensions_t dimensions[RATING_DIMENSIONS_SIZE];
   uint8 rating_descriptor_length;
   uint8 rating_descriptor_text[RATING_DESC_TEXT_SIZE];
} content_advisory_descriptor_t; 

/* PID assignment values */
typedef enum
{
    PID_NIT=0x0000,
    PID_PAS=0x0000,
    PID_CAS=0x0001,
    PID_MGT=0x1ffb,
    PID_VCT=0x1ffb,
    PID_RRT=0x1ffb,
    PID_STT=0x1ffb
} TABLE_ID_ENUM;

/* Table ID assignment values */
typedef enum
{
    TID_PAS,
    TID_CAS,
    TID_PMS,
    TID_MGT=0xC7,
    TID_TVCT,
    TID_CVCT,
    TID_RRT=0xCA,
    TID_EIT,
    TID_ETT,
    TID_STT,
    TID_PAT=0,
    TID_PMT=2,
    TID_EAS=0xD8,
    TID_forbidden = 0xFF,
} PID_ID_ENUM;

/* Service Types for VCT. */
typedef enum
{
    ANALOG_TELEVISION=1,
    ATSC_DIGITAL_TELEVISION=2,
    ATSC_AUDIO_ONLY=3,
    ATSC_DATA_BROADCAST_SERVICE=4
} SERVICE_TYPE_ENUM;

/* Adaptation Field Control Values */
typedef enum
{
    AFC_payload_only = 0x1,
    AFC_adaptation_only,
    AFC_adaptation_and_payload
} AFC_ENUM;

/* Stream ID Assignments */
typedef enum
{
    SID_PM = 0xBC,
    SID_private_1,
    SID_padding,
    SID_private_2,
    /* Audio Streams are numbered from 0xC0 - 0xDF */
    /* Video Streams are numbered from 0xE0 - 0xEF */
    SID_ECM = 0xF0,
    SID_EMM,
    SID_DSMCC,
    SID_13522,
    SID_type_A,
    SID_type_B,
    SID_type_C,
    SID_type_D,
    SID_type_E,
    SID_ancillary,
    /* Reserved streams are numbered from 0xFA - 0xFE */
    SID_PSD = 0xFF
} STREAM_ID_ENUM;

#if 0
/* Stream Type Assignments */
typedef enum
{
    /* 0x00 Reserved */
    ST_11172_Video = 0x01,
    ST_13818_2_Video,
    ST_11172_Audio,
    ST_13818_3_Audio,
    ST_13818_1_PrivateSection,
    ST_13818_1_PrivatePES,
    ST_13522_MHEG,
    ST_13818_1_DSMCC,
    ST_222_1,
    ST_13818_6_TypeA,
    ST_13818_6_TypeB,
    ST_13818_6_TypeC,
    ST_13818_6_TypeD,
    ST_13818_1_Aux,
    ST_13818_7_AAC,
    ST_ATSC_Video   = 0x80,
    ST_AC3          = 0x81,
    ST_Content_Advisory_Descriptor = 0x87
    /* 0x0F - 0x7F Reserved */
    /* 0x80 - 0xFF User Private */
} STREAM_TYPE_ENUM;
#endif

/* Descriptor Tag Assignments */
typedef enum
{
    DT_VideoStream = 0x02,
    DT_AudioStream,
    DT_Hierarchy,
    DT_Registration,
    DT_DataStreamAlign,
    DT_TargetBgGrid,
    DT_VideoWindow,
    DT_CA,
    DT_ISO639Language,
    DT_SystemClock,
    DT_MultiplexBufferUtilization,
    DT_Copyright,
    DT_MaximumBitRate,
    DT_PrivateDataIndicator,
    DT_SmoothingBuffer,
    DT_STD,
    DT_IBP,
    DT_Stuffing=0x80,
    DT_Ac3Audio,
    DT_ProgramIdentifier=0x85,
    DT_CaptionService,
    DT_ContentAdvisory,
    DT_ExtendedChannelName=0xa0,
    DT_ServiceLocation,
    DT_TimeShiftedService,
    DT_ComponentName
} DESCRIPTOR_TAG_ENUM;

/* PES Scrambling Control Values */
typedef enum
{
    PES_SCV_not_scrambled,
    PES_SCV_even_key = 0x2,
    PES_SCV_odd_key
} PES_SCV_ENUM;

/* Trick Mode Control */
typedef enum 
{
    TMC_fast_forward,
    TMC_slow_motion,
    TMC_freeze_frame,
    TMC_fast_reverse,
    TMC_slow_reverse
} TMC_ENUM;

typedef enum
{
    SRT_FIELD_VARIABLE,
    SRT_FIELD_ARRAY,
    SRT_FIELD_NOT_FOUND
} STREAM_RETURN_T;

typedef struct
{
    const char      *name;
    unsigned char   numBits;
    unsigned short  depIndex;
    unsigned char   depMask;
} PACKET_T;

typedef struct
{
    unsigned char   *pStream;
    unsigned long   *pBitOffsetArray;
    const PACKET_T  *pPacketDef;
    unsigned long   packetSize;
} TS_DECODE_T;

/*****************************************************************************
 *  Special values...
 *  numBits:    0xFF    - Field size is specified in BYTES
 *                        size = valueOf(dName)
 *              0xFE    - Field size is specified in BYTES
 *                        size = curByteOffset - byteOffset(dName)
 *              0xFD    - Field size is specified in BYTES
 *                        size = tranportPacketSize(188) - curByteOffset
 ****************************************************************************/
#define ALL_REMAINING_BYTES     0xFD
#define FROM_BYTE_OFFSET        0xFE
#define NUM_BYTES               0xFF

#define NODEP               0xFFFF
#define FIELD_NOT_FOUND     0xFFFFFFFF
#define TS_PACKET_SIZE      188

#define ts_dataSize( a )    (a & 0x0000FFFF)
#define ts_dataOffset( a )  (a >> 16)

#define ASCII_MODE            0x007f

/* Hannah
PUBLIC TS_DECODE_T      *ts_readyNewDecode( const PACKET_T *p_packetType, unsigned char *p_stream );
PUBLIC STREAM_RETURN_T  ts_getValueOfIndex( TS_DECODE_T *p_decode, unsigned short index, unsigned long *p_value );
PUBLIC STREAM_RETURN_T  ts_getValueOf( TS_DECODE_T *p_decode, char *fieldName, unsigned long *p_value );
PUBLIC void             ts_decodeDone( TS_DECODE_T *p_decode );
PUBLIC unsigned short   ts_getIndexOf( const PACKET_T *p_packet, char *fieldName );

PUBLIC STREAM_RETURN_T  ts_getInstantValueOf( const PACKET_T *p_packetType, unsigned char *p_stream, char *fieldName, unsigned long *p_value );
PUBLIC STREAM_RETURN_T  ts_getInstantValueOfIndex( const PACKET_T *p_packetType, unsigned char *p_stream, unsigned short index, unsigned long *p_value );

PUBLIC void decodeTransportPacket( unsigned char *p_stream );
PUBLIC uint32 build_content_advisory_descriptor( uint8 *ptr, content_advisory_descriptor_t *ca_ptr );
PUBLIC uint32 get_section_count( uint8 *ptr, uint8 *section_count );
PUBLIC uint32 get_version_number( uint8 *ptr, uint8 *version_number );
PUBLIC uint32 get_next_version_number( uint8 *ptr, uint8 *version_number );
PUBLIC uint32 get_short_name( TS_DECODE_T *p_decode, uint16 *short_name, uint8 *ascii_name );
PUBLIC uint32 get_table_type( uint32 table_type );
PUBLIC uint16 decode_multiple_strings( uint8 *dest, uint8 *src, uint16 length );
PUBLIC uint32 process_mgt( uint8 *data_ptr, uint8 *next_mgt_version_number, PID_ID_ENUM table_type_to_match, physical_chan_map_t *pcm );
PUBLIC uint32 process_vct( uint8 *data_ptr, physical_chan_map_t *pcm  );
PUBLIC uint32 process_pat( uint8 *data_ptr, physical_chan_map_t *pcm, uint32 *pat_TSID );
PUBLIC int process_pmt( uint8 *data_ptr, pmt_info_t *pmt_ptr, void *ca_data_ptr, uint16 data_length);
PUBLIC void   which_pid( uint8* data_ptr );
PUBLIC uint32 get_video_info( uint8 *ptr, uint32 *aspect_ratio, uint32 *horizontal_size, uint32 *vertical_size, uint32 *progressive );
*/
#ifdef __cplusplus
}
#endif

#endif
