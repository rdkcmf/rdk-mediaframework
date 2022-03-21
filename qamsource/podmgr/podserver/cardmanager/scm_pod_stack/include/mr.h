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



#ifndef _MR_H_
#define _MR_H_

//-------------------------------------------------------------------
// Include Files
//-------------------------------------------------------------------
#include "cardmanagergentypes.h"
              

#define LANGUAGE_MAX 3  
#define COMPONENT_LIST_MAX 100  
#define SHORT_NAME_MAX 7

#define VCT_MINOR_CHAN_MIN    0
#define VCT_MINOR_CHAN_MAX    99 
#define VCT_MAJOR_CHAN_MIN    1
#define VCT_MAJOR_CHAN_MAX    99 

#define OFF_AIR_MAP_SIZE  70
#define CABLE_MAP_SIZE   100
#define NUM_OF_PROGRAMS   200
#define MAX_NUMBER_OF_MINOR_CHANNELS 100
/// the max number of channels MR can receive (includes all input sources)
#define MAX_CHANNELS        ( 300 )     // ?? need to get the real value ??
#define ISO_LANG_CODE_ENG 0x00656E67
#define ISO_LANG_CODE_FRE 0x00667265
#define ISO_LANG_CODE_SPA 0x00737061
 
//-------------------------------------------------------------------
// Type Declarations
//-------------------------------------------------------------------
typedef uint16 minor_chan_t;
typedef uint16 major_chan_t;
typedef uint16 physical_chan_t;
typedef uint8 language_index_t;

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


typedef enum 
{
  VL_FEED_TYPE_NONE=1,
  VL_FEED_TYPE_PSIP,
  VL_FEED_TYPE_MPEG,
  VL_FEED_TYPE_ANALOG
} feed_type_t;

typedef struct {
  STREAM_TYPE_ENUM component_type;
  uint16           elementary_pid;
  uint8            bsmod;
  uint8            acmod;
  uint8            mainid;
  uint8            language_code[LANGUAGE_MAX];
  Boolean          caccess;
  uint16           ca_descriptor_length;
} component_list_t;

typedef struct
{
   major_chan_t     major_channel_number;
   minor_chan_t     minor_channel_number;
   uint8            short_name[SHORT_NAME_MAX];
   uint32           carrier_frequency;
   uint32           channel_TSID;
   uint16           program_number;  // Link to Pat/Pmt.
   uint8            etm_location;
   Boolean          access_control;
   Boolean          hidden;
   Boolean          hide_guide;
   uint8            service_type;
   uint16           source_id;      // Link to Eit and Ett.
   uint16           PCR_PID;
   uint32           component_count;
   component_list_t component_list[COMPONENT_LIST_MAX];
} psip_chan_t;

typedef struct
{
  uint16            program_num;
  uint16            pmt_pid;
  uint16            pcr_pid;
  uint8             version_number;
  Boolean           current_next_ind;
  uint16            num_of_components;
  Boolean           caccess;
  uint16            ca_descriptor_length;
  component_list_t  component_list[COMPONENT_LIST_MAX];
} pmt_info_t;

typedef struct
{
   feed_type_t      feed;
   uint16           psip_version_number;
   uint16           psip_channel_count;
   psip_chan_t      psip_chan[NUM_OF_PROGRAMS];
   uint16           pmt_channel_count;
   pmt_info_t       pmt_info[NUM_OF_PROGRAMS];
} physical_chan_map_t;



/*! date and time value

    mr_DateTime_t defines date and time in a Unix standard way:
    the count of seconds since Jan 1, 1970 12:00:00 midnight UTC
 */
typedef unsigned long  mr_DateTime_t;


/*! primary language indication

    The mr_Lang_t is an enum used to select/indicate the primary language
    setting. It contains all languages that are supported by the Media Receiver.
    The special value LAST_LANG serves only as a placeholder and is otherwise
    meaningless. Language is used to determine audio output and text display.
    A companion definition, MAX_LANG, is the 1-based number of different
    languages supported by Media Receiver.
 */
typedef enum
{
        // Note: values are assigned to the enums so that languages can
        //       be "de-supported" in the future without affecting the coding
        //       of the remaining languages.

            /// uninitialized
        LANG_none    = 0, 
            ///
        LANG_ENGLISH = 1,
            ///
        LANG_FRENCH = 2,
            ///
        LANG_SPANISH  = 3,

            /// only used to compute number of languages;
            /// must be last item
        LAST_LANG    = 4
} mr_Lang_t;
#define MAX_LANG    ( LAST_LANG - 1 )


/*! source of signal on Main Input (A)
    
    Signals that can be seen on Main input (A).
    The signal type influences the EPG and SI data that can be acquired and
    made available to clients
 */
typedef enum
{
            /// placeholder prior to initialization
        MAIN_none = 0,
            /// SI/EPG data is off-air PSIP (SCTE 65)
        MAIN_ANTENNA = 1,
            /// SI/EPG data is "cable PSIP" (either IB or OOB [if POD])
        MAIN_CABLE = 2
} mr_mainSrc_t;


#define MAIN_LAST       ( MAIN_CABLE )


/*! indication of which input port (Main, Sub, PC, etc) is currently selected
 */
typedef enum mr_AVInput_t {
            /// placeholder prior to initialization
        AVInput_none = 0,
            /// Antenna A input, both analog and/or digital channels to both boards
        AVInput_AntA = 1,
            /// Antenna B input, only analog channels to main board
        AVInput_AntB,

            /* TDB has no interest in the rest of these */

            /// IEEE 1394 Input, (aka iLink, Firewire)
        AVInput_IEEE_1394,
            /// PC Input
        AVInput_PC,
            /// Video 1 Input
        AVInput_VIDEO_1,
            /// Video 2 Input
        AVInput_VIDEO_2,
            /// Video 3 Input
        AVInput_VIDEO_3,
            /// Video 4 Input
        AVInput_VIDEO_4
} mr_AVInput_t;

typedef enum {
    ANALOG_MODE = 1,
    QAM_64      = 2,
    QAM_256     = 3,
    ATSC_VSB_8  = 4,
    ATSC_VSB_16 = 5
} mr_ModulationMode_t;


typedef enum
{
    CHAN_LOCK_ERROR_MIN        = 0,
    CHAN_TABLE_FAILED          = 1,
    CHAN_MPEG_UNLOCK           = 2,
    CHAN_DEMOD_UNLOCK          = 3,
    CHAN_PIDS_CHANGE           = 4,
    CHAN_LOCK_ERROR_MAX        = 5
} mr_chan_lock_error_t;


typedef struct {
    unsigned short      major_channel;
    unsigned short      minor_channel;
    unsigned long       carrier_frequency;
    mr_ModulationMode_t modulation_mode;
} mr_Cable_Map_t;

typedef struct
{
   uint16           major_channel;
   uint16           minor_channel;
   uint32           channel_TSID;
} channel_map_t;

typedef struct {
    unsigned short      number_of_channels;
    unsigned short      physical_channel;
    channel_map_t       channels[NUM_OF_PROGRAMS];
    mr_ModulationMode_t modulation_mode;
} mr_Off_Air_Map_t;


typedef struct {
    mr_mainSrc_t         main_source;
    mr_Off_Air_Map_t     off_air_map[OFF_AIR_MAP_SIZE];
    mr_Cable_Map_t       cable_map[CABLE_MAP_SIZE]; 
} mr_Source_Info_t; 


typedef struct
{
    unsigned char  oob_ds_lock_state;
    unsigned int   oob_ds_freq;
    unsigned int   oob_ds_agc;
    unsigned long  oob_ds_ber_correct;
    unsigned long  oob_ds_ber_uncorrect;
    unsigned long  oob_ds_lock_period;
} mr_oob_characteristics_t;

typedef struct
{
    unsigned char  ib_lock_state;
    unsigned int   ib_freq;
    unsigned int   ib_agc;
    unsigned int   ib_modulation_mode;
    unsigned int   mpeg_video_pid;
    unsigned int   mpeg_audio_pid;
    unsigned int   mpeg_pcr_pid;
    unsigned int   mpeg_prog_number;
    unsigned char  video_format;
    unsigned char  video_aspect_ratio;
    unsigned long  ib_ber_correct;
    unsigned long  ib_ber_uncorrect;
    unsigned long  ib_lock_period;
} mr_ib_characteristics_t;

// helpful macros
#define MAX( a, b )         ( ( a > b )  ?  a : b )
#define MIN( a, b )         ( ( a < b )  ?  a : b )

#endif // _MR_H_

