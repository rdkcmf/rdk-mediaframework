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


/*
**
** File: global_event_map.h
** This file defines global events from the 
**
**
**
*/


#ifndef _GLOBAL_EVENT_MAP_H_
#define _GLOBAL_EVENT_MAP_H_


// Add file name and revision tag to object code
#define REVISION  __FILE__ " $Revision: 1.94 $"
/*@unused@*/
/* srs: This is broken. This variable gets instantiated
   in every single module that includes this header
   file. At most only one copy of this variable is
   needed... This needs to be revisited.

static char *global_event_map_tag = REVISION;

*/
#undef REVISION		/* to get rid of all the compiler warnings...*/

  // events are long ints, <=   0x7FFFFFFF
#define MR_CHANNEL_OFFSET            0x100
#define TRANSPORT_OFFSET            0x1000
#define TDB_OFFSET                  0x2000
#define I2C_OFFSET                  0x3000
#define FRONTEND_CONTROL_OFFSET     0x3500
#define MPEG_OFFSET                 0x4000
#define TABLE_SCAN_OFFSET           0x5000
#define REGISTER_EVENT_OFFSET       0x6000
#define IEEE1394_EVENT_OFFSET       0x7000
#define FORTH_EVENT_OFFSET          0x8000
#define BURBANK_OFFSET              0x9000
#define IR_BLASTER_OFFSET           0xA000
#define CCI_OFFSET                  0xB000
#define GRAPHICS_OFFSET            0x10000
#define CC_OFFSET                  0x10500
#define EAS_OFFSET                 0x11000
#define POD_OFFSET                 0x11100
#define TUNE_OFFSET                0x12000
#define NVM_OFFSET                 0x13000
#define MBOARD_OFFSET              0x14000
#define PAXEL_OFFSET               0x15000
#define NEW_FORTH_OFFSET           0x16000
#define BURBANK_TABLE_OFFSET       0x17000
#define ASYNC_OFFSET               0x18000
#define ICD_MSGS_OFFSET            0x19000
#define FLASHCK_OFFSET             0x1A000
#define WATCHDOG_OFFSET            0x1B000
#define POD_ERROR_OFFSET	   0x1C000


#define VL_PFC_EVENT_ENUM_OFFSET   0x30000
#define VL_DSG_EVENT_ENUM_OFFSET   0x30100
#define CDL_OFFSET		   0x31000

typedef enum
{
    POWER_ON          = 0,
    POWER_OFF         = 1,
    MB_TX_MSG         = 2,
    THREAD_START      = 3,
    THREAD_SHUT_DOWN  = 4,
    PASS_PORT_TEST_1  = 10,
    PASS_PORT_TEST_2  = 12,
  
    START_SCAN        = 20,
    SCAN_RESULT       = 22,
    STOP_SCAN         = 24,
    SCAN_NEXT         = 25,
    OFFAIR_START_SCAN = 26,
    SCAN_DATA_UPDATE  = 27,
  
    GET_MINOR_MAP_FAILED = 29,
    GET_MAJOR_MAP_FAILED = 30,

    LOG_REQUEST       = 32,
    KMS_EXIT_REQUEST  = 33,

    GET_MINOR_CHANNEL_MAP = 34,
    GET_MAJOR_CHANNEL_MAP = 35, 
  
    // Main -> Dtv.
    // One-byte message.
    // There is a Dtv response.
    // success = 0x22  failure = 0x23
    AUTOSCAN_START_AIR_MODE = (0x22),

    // Main -> Dtv.
    // There is no Dtv response.
    // Dtv  -> Main.
    // There is no Main response.
    AUTOSCAN_STOP = (0x23),

  // these might not be implemented...
    RESTART               = 40,
    SYNC_MSG_QUEUE        = 41,
    SHUTDOWN              = 42,
    START_CHANNEL_TUNE    = 43,
    TUNE_RESULT           = 44,
    OFFAIR_START_TUNE     = 45,
  
    OFF_EVENT                    = 50,
    SCAN_AUTOSCAN_START_AIR_MODE = 51,

    OFFAIR_SCAN               = 52,
    MR_CHANNEL_TUNE_SUCCEEDED = 53,
    MR_CHANNEL_TUNE_FAILED    = 54,  
    OFFAIR_SCAN_FAILED        = 55,
    OFFAIR_SUCCESS            = 56,
    OFFAIR_FAIL               = 57, 
    OFFAIR_SCAN_GET_TABLE_FAILED  = 58,
    MR_TUNE_UI                    = 59,
    SCAN_TRANSPORT_GLITCH         = 60,
    OFFAIR_TUNE_FAILED            = 61, 
    TUNE_TO_IDLE                  = 62,
    GET_MGT_FOR_TUNE              = 63,
    OFFAIR_TUNE_GET_TABLE_FAILED  = 64,
    VSB_TUNE_DATA_UPDATE          = 65,
    GET_VCT_FOR_TUNE              = 66,
    GOT_MGT_FOR_TUNE              = 67,
    MR_CHANNEL_TABLES_FAILED      = 68,
    OFF_AIR_SCAN_DONE             = 69,
    OFF_AIR_SCAN_CHANNELS         = 70,
    GET_SCAN_DATA_SYNC            = 71,
    GET_SCAN_DATA_ASYNC           = 72,
    GET_TUNE_DATA_SYNC            = 73,
    GET_TUNE_DATA_ASYNC           = 74, 
    RESP_OFFAIR_CHANNEL_SCAN      = 75,
    RESP_OFFAIR_CHANNEL_TUNE      = 76,
    RESP_SIGNAL_STRENGTH          = 77,
    OFF_AIR_SCAN_DATA_REQUEST     = 78,
    OFF_AIR_TUNE_DATA_REQUEST     = 79,
    MR_CHANNEL_CHANGE_START       = 80,
    MR_TRANSPORT_DEAD             = 81,
    GET_DATA_TABLES               = 82,
    START_MGT_READ                = 83,
    START_VCT_READ                = 84,
    START_PAT_READ                = 85,
    GOOD_TABLE_DATA               = 86,
    NO_TABLE_DATA                 = 87,
    RESP_PROGRAM_PIDS             = 88,
    CHANNEL_SCAN_RESPONSE         = 89,
    CHANNEL_TUNE_RESPONSE         = 90,
    PROGRAM_PIDS_RESP             = 91,

    POD_ACTIVE                    = 93,
    POD_NOT_ACTIVE                = 94,
    HOMING_CONVERT_SOURCE_ID      = 95,
    MR_BB_READY                   = 96,
    

  /* events in MR Control for channel management */
    MR_SCAN_UI                    = MR_CHANNEL_OFFSET,        /* 0x100 */
    MR_TUNER_ACQ_FINISH           = MR_CHANNEL_OFFSET + 2,
    MR_DEMOD_ACQ_FINISH           = MR_CHANNEL_OFFSET + 3, 
    MR_CHAN_UNLOCK                = MR_CHANNEL_OFFSET + 4,
    MR_CHAN_MON_TIMEOUT           = MR_CHANNEL_OFFSET + 5,
    MR_VSB_CHAN_LOCK_STATUS       = MR_CHANNEL_OFFSET + 6,
    MR_CHANNEL_CHANGE_SUBSTATE    = MR_CHANNEL_OFFSET + 7,
    MR_REMOVE_CHAN_CTRL_SUBSTATE  = MR_CHANNEL_OFFSET + 8,
    MR_DOWNLOAD_COMPLETE          = MR_CHANNEL_OFFSET + 9,
    MR_VDEC_INIT_COMPLETE         = MR_CHANNEL_OFFSET + 10,
    MR_GPIO_INIT_COMPLETE         = MR_CHANNEL_OFFSET + 11,
    MR_SIGNAL_STRENGTH            = MR_CHANNEL_OFFSET + 12,/*! Setup to receive a timer event

    Use KMS to send a message to a thread's queue after a fixed period of time.
    NB: only as granular as HZ (10ms?)
    @pre none
    @param queue [IN] The ID of the message queue to send to.
    @param msg [IN] A pointer to the event buffer to send.
    MAKE SURE dataLength AND dataPtr FIELDS ARE CLEARED IF NO DATA IS SENT!
    @param periodMs [IN] The amount of time, in milliseconds, to wait before
    sending the msg.
    @return A handle to the timer so that it can be cancelled or reset before it
    expires
    @version 4/26/04 - prototype
    @see also podCancelTimer, podResetTimer
 */

    MR_DUMP_OFFAIR_DATA           = MR_CHANNEL_OFFSET + 13,
    MR_QAM_CHAN_LOCK_STATUS       = MR_CHANNEL_OFFSET + 14,
    MR_CHANNEL_CTRL_TUNE_FAILED   = MR_CHANNEL_OFFSET + 15,
    MR_CHANNEL_CTRL_IDLE          = MR_CHANNEL_OFFSET + 16,
    MR_CHANNEL_AUDIO_CTRL         = MR_CHANNEL_OFFSET + 17,
    MR_CHANNEL_VIDEO_CTRL         = MR_CHANNEL_OFFSET + 18,
    MR_CHANNEL_MONITOR_CTRL       = MR_CHANNEL_OFFSET + 19,
    MR_SET_SPDIF_OUTPUT           = MR_CHANNEL_OFFSET + 20,
    MR_SET_VIDEO_OUTPUT_FORMAT    = MR_CHANNEL_OFFSET + 21,
    MR_SET_ASPECT_RATIO           = MR_CHANNEL_OFFSET + 22,
    MR_SIGNAL_STRENGTH_REQUEST    = MR_CHANNEL_OFFSET + 23,
    MR_SET_INPUT_A_SOURCE         = MR_CHANNEL_OFFSET + 24,
    MR_GET_PROGRAM_PIDS           = MR_CHANNEL_OFFSET + 25,
    MR_SCAN_CHANNEL               = MR_CHANNEL_OFFSET + 26,
    // x=band;
    // y=physical_channel
    // z=0
    MR_SCAN_CHANNEL_DATA          = MR_CHANNEL_OFFSET + 27,
    // x=major_channel.
    // y=minor_channel.
    // z= tsid.
    MR_SCAN_CHANNEL_DONE          = MR_CHANNEL_OFFSET + 28,
    // x={0=good, 1=bad}
    // y=0
    // z=0
    MR_START_OFFAIR_SCAN             = MR_CHANNEL_OFFSET + 29,
    MR_START_CABLE_SCAN              = MR_CHANNEL_OFFSET + 30,
    MR_CABLE_IB_TUNE_SUBSTATE_START  = MR_CHANNEL_OFFSET + 31,
    MR_SCAN_CHANNEL_TUNE_FAILED      = MR_CHANNEL_OFFSET + 32,
    MR_SCAN_CHANNEL_TABLE_FAILED     = MR_CHANNEL_OFFSET + 33,
    MR_CABLE_IB_SCAN_FINISH          = MR_CHANNEL_OFFSET + 34,
    MR_OFFAIR_SCAN_FINISH            = MR_CHANNEL_OFFSET + 35,
    MR_START_OFFAIR_SCAN_TUNE        = MR_CHANNEL_OFFSET + 36,
    MR_TUNE_CHANNEL                  = MR_CHANNEL_OFFSET + 37,
    // x=band;
    // y=?
    // z=?
    MR_TUNE_CHANNEL_DATA             = MR_CHANNEL_OFFSET + 38,
    // x=?
    // y=?
    // z=?
    MR_TUNE_CHANNEL_DONE             = MR_CHANNEL_OFFSET + 39,
    // x=?
    // y=?
    // z=?
    MR_CHANNEL_TUNE_PMT                  = MR_CHANNEL_OFFSET + 40,
    MR_POD_STATUS_CHANGE                 = MR_CHANNEL_OFFSET + 41,
    MR_PAT_VER_CHG_EVENT                 = MR_CHANNEL_OFFSET + 42,
    MR_PAT_LOST_EVENT                    = MR_CHANNEL_OFFSET + 43,
    MR_STREAM_LOST_EVENT                 = MR_CHANNEL_OFFSET + 44,
    MR_PMT_VER_CHG_EVENT                 = MR_CHANNEL_OFFSET + 45,
    MR_PMT_LOST_EVENT                    = MR_CHANNEL_OFFSET + 46,
    MR_FLUSH_DONE                        = MR_CHANNEL_OFFSET + 47,
    MR_HOMING_REQ                        = MR_CHANNEL_OFFSET + 48,
    MR_HOMING_START                      = MR_CHANNEL_OFFSET + 49,
    MR_POD_HOMING_TUNE_FAILED            = MR_CHANNEL_OFFSET + 50,
    MR_CHAN_CTRL_STATE                   = MR_CHANNEL_OFFSET + 51,
    MR_TUNER_CANCELED                    = MR_CHANNEL_OFFSET + 52,
    MR_DEMOD_CANCELED                    = MR_CHANNEL_OFFSET + 53,
    MR_TUNE_SUBSTATE_TERMINATED          = MR_CHANNEL_OFFSET + 54,
    MR_CHAN_CTRL_SUBSTATE_TERMINATED     = MR_CHANNEL_OFFSET + 55,
    MR_CABLE_IB_SCAN_SUBSTATE_TERMINATED = MR_CHANNEL_OFFSET + 56,
    MR_OFFAIR_SCAN_SUBSTATE_TERMINATED   = MR_CHANNEL_OFFSET + 57,
    MR_POD_AUTHENTICATION_STATUS         = MR_CHANNEL_OFFSET + 58,
    MR_IEEE1394_PLAYBACK                 = MR_CHANNEL_OFFSET + 59,
    MR_IEEE1394_TUNE_START               = MR_CHANNEL_OFFSET + 60,
    MR_IEEE1394_TS_GET                   = MR_CHANNEL_OFFSET + 61,
    MR_POD_AUTHENTICATION_PASSED         = MR_CHANNEL_OFFSET + 62,
    MR_POD_AUTHENTICATION_FAILED         = MR_CHANNEL_OFFSET + 63,
    MR_GET_CUR_NEXT_LANGUAGES            = MR_CHANNEL_OFFSET + 64,
    MR_TUNE_AUDIO_BY_LANGUAGE            = MR_CHANNEL_OFFSET + 65,
    MR_IB_OOB_CHARACTERISTICS            = MR_CHANNEL_OFFSET + 66,
    MR_AUDIO_CHANGE_NOTIFY               = MR_CHANNEL_OFFSET + 67,
    MR_DTV_CHARACTERISTICS               = MR_CHANNEL_OFFSET + 68,
    MR_IB_CHARACTERISTICS_REQUEST        = MR_CHANNEL_OFFSET + 69,
    MR_QAM_BER_TIME_PERIOD_RESP          = MR_CHANNEL_OFFSET + 70,
    MR_VSB_BER_TIME_PERIOD_RESP          = MR_CHANNEL_OFFSET + 71,
    MR_GET_IB_BER_COUNTS                 = MR_CHANNEL_OFFSET + 72,
    
    // Transport Thread Internal Events
    TRANSPORT_SHUTDOWN_EVENT     = TRANSPORT_OFFSET,        /* 0x1000 */
    TRANSPORT_ISR_EVENT          = TRANSPORT_OFFSET + 1,
    TRANSPORT_READ_STREAM        = TRANSPORT_OFFSET + 2,
    TRANSPORT_ABORT_STREAM       = TRANSPORT_OFFSET + 3,
    TRANSPORT_CLOSE_STREAM       = TRANSPORT_OFFSET + 4,
    TRANSPORT_OPEN_STREAM        = TRANSPORT_OFFSET + 5,
    TRANSPORT_ADD_FILTER_GROUP   = TRANSPORT_OFFSET + 6,
    TRANSPORT_DELETE_LOCAL_ID    = TRANSPORT_OFFSET + 7,
    TRANSPORT_NEW_LOCAL_ID       = TRANSPORT_OFFSET + 8,
    TRANSPORT_ESTREAM            = TRANSPORT_OFFSET + 9,
    TRANSPORT_DELETE_EX_FILTERS  = TRANSPORT_OFFSET + 10,
    TRANSPORT_SET_HARWARE_FILTERS = TRANSPORT_OFFSET + 11,
    TRANSPORT_SET_COMPOSITE_DATA  = TRANSPORT_OFFSET + 12,
    TRANSPORT_INIT_READ           = TRANSPORT_OFFSET + 13,
    TRANSPORT_CLIENT_DATA         = TRANSPORT_OFFSET + 14,
    TRANSPORT_DUMP_SOFTWARE_FILTER = TRANSPORT_OFFSET + 15,
    TRANSPORT_VERIFY_SERVER        = TRANSPORT_OFFSET + 16,
    TRANSPORT_CHANGE_BAND          = TRANSPORT_OFFSET + 17,
    TRANSPORT_CHANGE_FILTER        = TRANSPORT_OFFSET + 18,
    TRANSPORT_SET_HSI              = TRANSPORT_OFFSET + 19,
    TRANSPORT_RELEASE_HSI          = TRANSPORT_OFFSET + 20,
    TRANSPORT_HSI_PRIVATE          = TRANSPORT_OFFSET + 21,   

    TRANSPORT_INT_NEW_ID            = TRANSPORT_OFFSET + 27,
    TRANSPORT_INT_SET_DATA          = TRANSPORT_OFFSET + 28,
    TRANSPORT_INT_SET_HW_FILTERS    = TRANSPORT_OFFSET + 29,
    TRANSPORT_INT_SET_SW_FILTERS    = TRANSPORT_OFFSET + 30,
    TRANSPORT_INT_READ_INIT         = TRANSPORT_OFFSET + 31,
    TRANSPORT_INT_READ              = TRANSPORT_OFFSET + 32,
    TRANSPORT_INT_ABORT             = TRANSPORT_OFFSET + 33,
    TRANSPORT_INT_CLOSE_STREAM      = TRANSPORT_OFFSET + 34,
    TRANSPORT_INT_DELETE_ID         = TRANSPORT_OFFSET + 35,
    TRANSPORT_INT_OPEN_STREAM       = TRANSPORT_OFFSET + 36,
    
    TRANSPORT_MULTI_SECTION         = TRANSPORT_OFFSET + 40,
    TRANSPORT_ASSIGN_NEW_STREAM     = TRANSPORT_OFFSET + 41,
    TRANSPORT_TIME_OUT              = TRANSPORT_OFFSET + 42,
    TRANSPORT_DELETE_ASSIGNED_STREAM = TRANSPORT_OFFSET + 43,
    
    TRANSPORT_POD_CRYPTO_CONFIG         = TRANSPORT_OFFSET + 60,
    TRANSPORT_POD_CRYPTO_CONFIG_FINAL   = TRANSPORT_OFFSET + 61,
    TRANSPORT_SET_POD_PIDS              = TRANSPORT_OFFSET + 62,
    TRANSPORT_SET_STREAM_KEY            = TRANSPORT_OFFSET + 63,

    TRANSPORT_DUMP_DMA_CHANNELS     = TRANSPORT_OFFSET + 71,

    /* event for channel manager to access GPIOs */
    CHANNEL_VSB_GPIO_AGC_CONTROL  = TRANSPORT_OFFSET + 100,
    CHANNEL_GPIO_INIT             = TRANSPORT_OFFSET + 101,

    CHANNEL_QAM_GPIO_AGC_CONTROL  = TRANSPORT_OFFSET + 103,
    GPIO_TUNER_BSC_ENABLE         = TRANSPORT_OFFSET + 104,
    GPIO_TUNER_BSC_DISABLE        = TRANSPORT_OFFSET + 105,
    MR_GPIO_INIT                  = TRANSPORT_OFFSET + 106,
    AUDIO_MUTE_CHANGE             = TRANSPORT_OFFSET + 107,
    DVO_RESET                     = TRANSPORT_OFFSET + 108,
    
    /* events for power management - standby or restore from standby */
    LOGICAL_PWR_STATE_9VA           = TRANSPORT_OFFSET + 109,
    LOGICAL_PWR_STATE_ADAC          = TRANSPORT_OFFSET + 110,
    LOGICAL_PWR_STATE_VASIC         = TRANSPORT_OFFSET + 111,
    LOGICAL_PWR_STATE_VDEC_ETC_CLK  = TRANSPORT_OFFSET + 112,
    
    SOFTWARE_DOWNLOAD_TOGGLE        = TRANSPORT_OFFSET + 113,


    SCAN_PAT             = TDB_OFFSET + 50,
    SCAN_VCT             = TDB_OFFSET + 51,
    TUNE_PAT             = TDB_OFFSET + 52,
    TUNE_VCT             = TDB_OFFSET + 53,
    TUNE_PMT             = TDB_OFFSET + 54,



    // events between TDB & Xport
    GET_MGT_TABLE_FOR_TDB = TDB_OFFSET,                  /* 0x2000 */
    STOP_MGT_TABLE_FOR_TDB,       // + 1
    GOT_MGT_TABLE_FOR_TDB,        // + 2
    GET_STT_TABLE_FOR_TDB,        // + 3    // STT
    STOP_STT_TABLE_FOR_TDB,       // + 4
    GOT_STT_TABLE_FOR_TDB,        // + 5
    GET_VCT_TABLE_FOR_TDB,        // + 6    // VCT
    SEND_VCT_TABLE_FOR_TDB,       // + 7
    STOP_VCT_TABLE_FOR_TDB,       // + 8
    GOT_VCT_TABLE_FOR_TDB,        // + 9
    GET_RRT_TABLE_FOR_TDB,        // + A    // RRT
    STOP_RRT_TABLE_FOR_TDB,       // + B
    GOT_RRT_TABLE_FOR_TDB,        // + C
    GET_PMT_TABLE_FOR_TDB,        // + D    // PMT
    STOP_PMT_TABLE_FOR_TDB,       // + E
    GOT_PMT_TABLE_FOR_TDB,        // + F
    GET_EIT_TABLE_FOR_TDB,        // + 10   // EIT
    STOP_EIT_TABLE_FOR_TDB,       // + 11
    GOT_EIT_TABLE_FOR_TDB,        // + 12
    GET_ETT_TABLE_FOR_TDB,        // + 13   // ETT
    STOP_ETT_TABLE_FOR_TDB,       // + 14
    GOT_ETT_TABLE_FOR_TDB,        // + 15

    RRT_TIMEOUT_FOR_TDB = TDB_OFFSET + 0x100,       /* 0x2100 */
    MGT_TIMEOUT_FOR_TDB,          // +   101
    STT_TIMEOUT_FOR_TDB,          // +   102
    VCT_TIMEOUT_FOR_TDB,          // +   103
    EIT1_TIMEOUT_FOR_TDB,         // +   104
    ETT1_TIMEOUT_FOR_TDB,         // +   105
    PAT_TIMEOUT_FOR_TDB,          // +   106
    PMT_TIMEOUT_FOR_TDB,          // +   107

    XPORT_TERMINATE_VCT = TDB_OFFSET + 0x200,       /* 0x2200 */
    XPORT_TERMINATE_RRT,          // +   201
    XPORT_TERMINATE_EIT1,         // +   202
    XPORT_TERMINATE_EIT2,         // +   203
    XPORT_TERMINATE_ETT1,         // +   204
    XPORT_TERMINATE_ETT2,         // +   205
    XPORT_TERMINATE_MGT,          // +   206
    XPORT_TERMINATE_PMT,          // +   207
    XPORT_RESTART_PAT,            // +   208
    XPORT_GENERIC_TIMEOUT,        // +   209

    GET_ETT1_PID_FOR_TDB = TDB_OFFSET + 0x300,      /* 0x2300 */
    GET_ETT2_PID_FOR_TDB,         //  +   301
    GET_EIT1_PID_FOR_TDB,         //  +   302
    GET_EIT2_PID_FOR_TDB,         //  +   303
    GOT_ETT1_PID_FOR_TDB,         //  +   304
    GOT_ETT2_PID_FOR_TDB,         //  +   305
    GOT_EIT1_PID_FOR_TDB,         //  +   306
    GOT_EIT2_PID_FOR_TDB,         //  +   307
    EIT2_TIMEOUT_FOR_TDB,         //  +   308
    ETT2_TIMEOUT_FOR_TDB,         //  +   309
    GET_MULTIPLE_SECTIONS,        //  +   30A
    DUMP_MGT_DATA,                //  +   30B
    PASS_STREAM_AND_REQUEST_IDS,  //  +   30C
    GET_SOME_TABLE_AGAIN,         //  +   30D

    GET_EAM_TABLE_FOR_TDB = TDB_OFFSET + 0x400,     /* 0x2400 */
    STOP_EAM_TABLE_FOR_TDB,       //   +   401
    GOT_EAM_TABLE_FOR_TDB,        //   +   402
    EAM_TIMEOUT_FOR_TDB,          //   +   403

    // events between TDB & Tuner
    XPORT_LOCK_CHANGE   = TDB_OFFSET + 0x500,       /* 0x2500 */
    TUNE_IN_PROGRESS,             // +   501
    TUNE_COMPLETE,                // +   502

    // events between TDB & UI
    INITIALIZE_TDB_FOR_UI = TDB_OFFSET + 0x600,     /* 0x2600 */
    TUNE_LOCK_CHANGE,             //   +   601
    SET_INPUT_A_SOURCE,           //   +   602
    SET_PRIMARY_LANGUAGE,         //   +   603
    RESTART_TDB,                  //   +   604
    IDLE_TDB,                     //   +   605
    SHUTDOWN_TDB,                 //   +   606

    /* note: sync/async TDB events always *MUST* be defined as a pair;
     *       SYNC first, then ASYNC
     */
    GET_RATING_INFO_SYNC = TDB_OFFSET + 0x700,      /* 0x2700 */
    GET_RATING_INFO_ASYNC,        //  +   701
    GET_CURRENT_CHAN_INFO_SYNC,   //  +   702
    GET_CURRENT_CHAN_INFO_ASYNC,  //  +   703
    GET_CHAN_INFO_SYNC,           //  +   704
    GET_CHAN_INFO_ASYNC,          //  +   705
    GET_CURRENT_PROG_INFO_SYNC,   //  +   706
    GET_CURRENT_PROG_INFO_ASYNC,  //  +   707
    GET_PROG_INFO_SYNC,           //  +   708
    GET_PROG_INFO_ASYNC,          //  +   709

//    CHAN_INFO_CHANGE    = TDB_OFFSET + 0x800,     // new T/C VCT data avail

    // TDB timer event(s)
    NOTIFY_UI_PROG_INFO = TDB_OFFSET + 0x900,

    /* events for frontend control */
    FRONTEND_CONTROL_QAM_SET                = FRONTEND_CONTROL_OFFSET,          /* 0x3500 */
    FRONTEND_READ_BYTE_EVENT                = FRONTEND_CONTROL_OFFSET + 1,
    FRONTEND_WRITE_BYTE_EVENT               = FRONTEND_CONTROL_OFFSET + 2,
    FRONTEND_QAM_READ_EVENT                 = FRONTEND_CONTROL_OFFSET + 3,
    FRONTEND_QAM_WRITE_EVENT                = FRONTEND_CONTROL_OFFSET + 4,
    FRONTEND_READ_WORD_EVENT                = FRONTEND_CONTROL_OFFSET + 5,
    FRONTEND_DEVICE_INIT                    = FRONTEND_CONTROL_OFFSET + 6,
    FRONTEND_GET_LOCK_STATUS                = FRONTEND_CONTROL_OFFSET + 7,     /* for INBAND QAM channel */
    FRONTEND_FEC_RESET                      = FRONTEND_CONTROL_OFFSET + 8,
    FRONTEND_SET_SYMBOL_RATE                = FRONTEND_CONTROL_OFFSET + 9,
    FRONTEND_SET_IF_FREQ                    = FRONTEND_CONTROL_OFFSET + 10,
    FRONTEND_INVERT_SPECTRUM                = FRONTEND_CONTROL_OFFSET + 11,
    FRONTEND_READ_LONG_EVENT                = FRONTEND_CONTROL_OFFSET + 12,
    FRONTEND_WRITE_LONG_EVENT               = FRONTEND_CONTROL_OFFSET + 13,
    FRONTEND_RUN_INIT_SCRIPT                = FRONTEND_CONTROL_OFFSET + 14,
    FRONTEND_RUN_256QAM_SCRIPT              = FRONTEND_CONTROL_OFFSET + 15,
    FRONTEND_RUN_DS_CONFIG_SCRIPT           = FRONTEND_CONTROL_OFFSET + 16,
    FRONTEND_DS_INTERRUPT_MSG               = FRONTEND_CONTROL_OFFSET + 17,
    FRONTEND_QAM_ACQ_TIMEOUT                = FRONTEND_CONTROL_OFFSET + 18,
    FRONTEND_CONTROL_DS_QAM_STATUS_GET      =  FRONTEND_CONTROL_OFFSET + 19,
    FRONTEND_OOB_DS_TUNE_REQUEST            = FRONTEND_CONTROL_OFFSET + 20,
    FRONTEND_OOB_DS_TUNE_STATUS             = FRONTEND_CONTROL_OFFSET + 21,
    FRONTEND_QAM_SNRE_REQUEST               = FRONTEND_CONTROL_OFFSET + 22,
    FRONTEND_OOB_QPSK_SNRE_REQUEST          = FRONTEND_CONTROL_OFFSET + 23,
    FRONTEND_GET_FILE_DESCRIPTOR            = FRONTEND_CONTROL_OFFSET + 24,
    FRONTEND_OOB_DS_LOCK_STATUS             = FRONTEND_CONTROL_OFFSET + 25,
    FRONTEND_OOB_DS_LOCK_STATUS_GET         = FRONTEND_CONTROL_OFFSET + 26,
    FRONTEND_ACQ_CANCELLATION               = FRONTEND_CONTROL_OFFSET + 27,
    FRONTEND_OOB_DS_CHARACTERISTICS_REQUEST = FRONTEND_CONTROL_OFFSET + 28,
    FRONTEND_OB_MULTI_REG_READ              = FRONTEND_CONTROL_OFFSET + 29,
	
    /***** From here, added by Yas *****/
    FRONTEND_READ_MULTIBYTE_EVENT   = FRONTEND_CONTROL_OFFSET + 30,
    FRONTEND_WRITE_MULTIBYTE_EVENT  = FRONTEND_CONTROL_OFFSET + 31,
/***** To here, added by Yas *****/
    FRONTEND_QAM_MULTI_REG_READ_REQUEST     = FRONTEND_CONTROL_OFFSET + 32,
    FRONTEND_QAM_REG_READ_RESP              = FRONTEND_CONTROL_OFFSET + 33,
    FRONTEND_QAM_BER_TIME_PERIOD_REQUEST    = FRONTEND_CONTROL_OFFSET + 34,
    FRONTEND_OOB_US_TUNE_REQUEST            = FRONTEND_CONTROL_OFFSET + 35,
    FRONTEND_OOB_US_TUNE_STATUS             = FRONTEND_CONTROL_OFFSET + 36,
    FRONTEND_OOB_US_TUNE_STATUS_SENT        = FRONTEND_CONTROL_OFFSET + 37,
FRONTEND_INBAND_TUNE_REQUEST            = FRONTEND_CONTROL_OFFSET + 38,
    FRONTEND_INBAND_TUNE_STATUS         = FRONTEND_CONTROL_OFFSET + 39,
    FRONTEND_INBAND_TUNE_RELEASE         = FRONTEND_CONTROL_OFFSET + 40,
    //events for Scan/Tune
    GET_PAT_TABLE_FOR_SCAN    = TABLE_SCAN_OFFSET + 0,    /* 0x5000 */
    PAT_TABLE_TO_DURING_SCAN  = TABLE_SCAN_OFFSET + 1,
    GOT_PAT_TABLE_DURING_SCAN = TABLE_SCAN_OFFSET + 2,

    GET_PMT_TABLE_FOR_SCAN    = TABLE_SCAN_OFFSET + 3,
    PMT_TABLE_TO_DURING_SCAN  = TABLE_SCAN_OFFSET + 4,
    GOT_PMT_TABLE_DURING_SCAN = TABLE_SCAN_OFFSET + 5,

    GET_VCT_TABLE_FOR_SCAN    = TABLE_SCAN_OFFSET + 6,
  
    // event->x = physical_channel
    VCT_TABLE_TO_DURING_SCAN  = TABLE_SCAN_OFFSET + 7,
    GOT_VCT_TABLE_DURING_SCAN = TABLE_SCAN_OFFSET + 8,

    GET_PAT_TABLE_FOR_TUNE    = TABLE_SCAN_OFFSET + 9,
    PAT_TABLE_TO_DURING_TUNE  = TABLE_SCAN_OFFSET + 11,
    GOT_PAT_TABLE_DURING_TUNE = TABLE_SCAN_OFFSET + 12,

    GET_PMT_TABLE_FOR_TUNE    = TABLE_SCAN_OFFSET + 13,
    PMT_TABLE_TO_DURING_TUNE  = TABLE_SCAN_OFFSET + 14,
    GOT_PMT_TABLE_DURING_TUNE = TABLE_SCAN_OFFSET + 15,

    GET_VCT_TABLE_FOR_TUNE    = TABLE_SCAN_OFFSET + 16,
  
    // event->x = physical_channel
    VCT_TABLE_TO_DURING_TUNE  = TABLE_SCAN_OFFSET + 17,
    GOT_VCT_TABLE_DURING_TUNE = TABLE_SCAN_OFFSET + 18,

    GET_MGT_TABLE_FOR_TUNE    = TABLE_SCAN_OFFSET + 19,
    MGT_TABLE_TO_DURING_TUNE  = TABLE_SCAN_OFFSET + 20,
    GOT_MGT_TABLE_DURING_TUNE = TABLE_SCAN_OFFSET + 21,

    GET_EIT_TABLE_FOR_TUNE    = TABLE_SCAN_OFFSET + 22,
    EIT_TABLE_TO_DURING_TUNE  = TABLE_SCAN_OFFSET + 23,
    GOT_EIT_TABLE_DURING_TUNE = TABLE_SCAN_OFFSET + 24,
  
    STOP_PMT_TABLE_FOR_SCAN   = TABLE_SCAN_OFFSET + 25,
    GET_VCT_FOR_SCAN          = TABLE_SCAN_OFFSET + 26,
    GOT_VCT_FOR_SCAN          = TABLE_SCAN_OFFSET + 27,
    GET_MGT_FOR_SCAN          = TABLE_SCAN_OFFSET + 28,
    GOT_MGT_FOR_SCAN          = TABLE_SCAN_OFFSET + 29,
    GET_VCT_TABLE_DURING_SCAN = TABLE_SCAN_OFFSET + 30,
    TABLE_DATA_UPDATE         = TABLE_SCAN_OFFSET + 31,
    VSB_DATA_UPDATE           = TABLE_SCAN_OFFSET + 32,
  
    /* events that multiple queues will register to */
    /* these events should be registered and posted by the receivers and sender */
    CHANNEL_CHANGE            = REGISTER_EVENT_OFFSET,     /* 0x6000 */
    CHANNEL_LOST_LOCK         = REGISTER_EVENT_OFFSET + 1,
    CHANNEL_TS_READY          = REGISTER_EVENT_OFFSET + 2,
    CHANNEL_AUDIO_MUTE        = REGISTER_EVENT_OFFSET + 3,
    CHANNEL_AUDIO_UNMUTE      = REGISTER_EVENT_OFFSET + 4,
    CHANNEL_VIDEO_MUTE        = REGISTER_EVENT_OFFSET + 5,
    CHANNEL_VIDEO_UNMUTE      = REGISTER_EVENT_OFFSET + 6,
    CHANNEL_CC_MUTE           = REGISTER_EVENT_OFFSET + 7,
    CHANNEL_CC_UNMUTE         = REGISTER_EVENT_OFFSET + 8,
    CHANNEL_MPEG_READY        = REGISTER_EVENT_OFFSET + 9,
    CHANNEL_CHANGE_FAILED     = REGISTER_EVENT_OFFSET + 10,
    CHANNEL_VSB_MON_TIMEOUT   = REGISTER_EVENT_OFFSET + 11,
    CHANNEL_REGAIN_LOCK       = REGISTER_EVENT_OFFSET + 12, 
    SYSTEM_TIME_UPDATE        = REGISTER_EVENT_OFFSET + 13,
    STANDBY_NOTIFICATION      = REGISTER_EVENT_OFFSET + 14,
    NDS_POD_DETECTED          = REGISTER_EVENT_OFFSET + 15, /* This message is only TEMPORARY! 6/8/04.  x=1 if NDS, x=0 if not NDS pod */

    // mpeg events
    MPEG_GET_VIDEO_PARAMETERS    = MPEG_OFFSET  + 0,      /* 0x4000 */
    MPEG_SET_ASPECT_RATIO        = MPEG_OFFSET  + 1,
    MPEG_SET_VIDEO_PARAMETERS    = MPEG_OFFSET  + 2,
    MPEG_SET_VIDEO_OUTPUT_FORMAT = MPEG_OFFSET  + 3,
    MPEG_AUDIO_UPDATE            = MPEG_OFFSET  + 7,
    MPEG_VIDEO_UPDATE            = MPEG_OFFSET  + 8,
    MPEG_CC_UPDATE               = MPEG_OFFSET  + 9,
    MPEG_CAPTURE_UPDATE          = MPEG_OFFSET  + 10,
    MPEG_GRAPHICS_UPDATE         = MPEG_OFFSET  + 11,
    MPEG_DECODER_LOCK            = MPEG_OFFSET  + 12,
    MPEG_DISPLAY_LOCK_TIMEOUT    = MPEG_OFFSET  + 13,
    MPEG_DECODER_UNLOCK          = MPEG_OFFSET  + 14,
    MPEG_VIDEO_LOCK_TIME_OUT     = MPEG_OFFSET  + 15,
    MPEG_GRAPHIC_CHANGE_WAIT     = MPEG_OFFSET  + 16,
    MPEG_STARTUP_TIME_OUT        = MPEG_OFFSET  + 17,
    MPEG_MODE_CHANGE             = MPEG_OFFSET  + 18,
    MPEG_MODE_CHANGE_UPDATE      = MPEG_OFFSET  + 19,
    MPEG_DECODER_DISPLAY_LOCK    = MPEG_OFFSET  + 20,
    MPEG_PIP_DECODER_LOCK        = MPEG_OFFSET  + 21,
    MPEG_TEST_INTERRUPT          = MPEG_OFFSET  + 22,
    MPEG_CC_DATA                 = MPEG_OFFSET  + 23,
    MPEG_START_VIDEO_DIGITIAL_DECODE = MPEG_OFFSET + 24,
    MPEG_START_VIDEO_ANALOG_DECODE   = MPEG_OFFSET + 25,
    MPEG_STOP_VIDEO_DECODE           = MPEG_OFFSET + 26,
    MPEG_START_AUDIO_DIGITIAL_DECODE = MPEG_OFFSET + 27,
    MPEG_START_AUDIO_ANALOG_DECODE   = MPEG_OFFSET + 28,
    MPEG_STOP_AUDIO_DECODE           = MPEG_OFFSET + 29,
    MPEG_OUTPUT_SPDIF                = MPEG_OFFSET + 30,
    MPEG_GET_AUDIO_STATUS            = MPEG_OFFSET + 31,
    MPEG_HSX_PROBLEM                 = MPEG_OFFSET + 32,
    MPEG_ENFORCE_MACROVISION        = MPEG_OFFSET + 33,
    MPEG_SET_MACROVISION_TYPE       = MPEG_OFFSET + 34,
    MPEG_GET_DIGITAL_CHANNEL_INFO   = MPEG_OFFSET + 35,
    MPEG_AUDIO_LIPSYNC_WAIT         = MPEG_OFFSET + 36,


    MPEG_VIDEO_REQUEST              = MPEG_OFFSET + 37,

    // debug events
    MPEG_DUMP_REGISTERS             = MPEG_OFFSET + 50,
        
   // legacy events not used any more
    MPEG_START_DECODE            = MPEG_OFFSET  + 4,
    MPEG_STOP                    = MPEG_OFFSET  + 6,



    // mpeg interrupt events
    BGE_INTR_STATUS_RASTER_BF_CMP_EVENT = MPEG_OFFSET + 100,
    BGE_INTR_STATUS_MPEG_INT_EVENT      = MPEG_OFFSET + 101,
    BGE_INTR_STATUS_ADP_X_INTR_EVENT    = MPEG_OFFSET + 102,
    BGE_INTR_STATUS_ADP_INTR_EVENT       = MPEG_OFFSET + 103,
    BGE_INTR_ENA_OX_INT_EVENT           = MPEG_OFFSET + 104,
    BGE_INTR_ENA_ANA_VSYNC_EVENT        = MPEG_OFFSET + 105,
    BGE_INTR_ENA_DISP_VSYNC_EVENT       = MPEG_OFFSET + 106,
    BGE_SOFTWARE_INTERRUPT              = MPEG_OFFSET + 107,
    BGE_INTR_BF_EVENT                   = MPEG_OFFSET + 108,
    BGE_INTR_TF_EVENT                   = MPEG_OFFSET + 109,
  

    TCP_ADD_COMMAND              = FORTH_EVENT_OFFSET  + 0,     /* 0x8000 */
    REMOTE_COMMAND               = FORTH_EVENT_OFFSET  + 1,
    REMOTE_EVENT                 = FORTH_EVENT_OFFSET  + 2,

    GET_RATING_INFO              = BURBANK_OFFSET + 0,         /* 0x9000 */
    RATING_INFO_RESP             = BURBANK_OFFSET + 1,

    SET_DEFAULT_LANGUAGE         = BURBANK_OFFSET + 2,

    GET_CURRENT_ALT_AUDIO_LIST   = BURBANK_OFFSET + 3,
    CURRENT_ALT_AUDIO_LIST_RESP  = BURBANK_OFFSET + 4,

    GET_SIGNAL_STRENGTH          = BURBANK_OFFSET + 5,
    SIGNAL_STRENGTH_RESP         = BURBANK_OFFSET + 6,

    START_AUTO_PROGRAM           = BURBANK_OFFSET + 7,
    AUTO_PROGRAM_RESP            = BURBANK_OFFSET + 8,
  
    //STOP_AUTO_PROGRAM            = BURBANK_OFFSET + 9,
  
    STOP_MAJOR_CHAN_MAP_UPDATE   = BURBANK_OFFSET + 10,
  
    STOP_MINOR_CHAN_MAP_UPDATE   = BURBANK_OFFSET + 11,
  
    GET_MAJOR_CHAN_MAP           = BURBANK_OFFSET + 12,
    MAJOR_CHAN_MAP_RESP          = BURBANK_OFFSET + 13,

    GET_MINOR_CHAN_MAP           = BURBANK_OFFSET + 14,
    MINOR_CHAN_MAP_RESP          = BURBANK_OFFSET + 15,

    TUNE_THE_CHANNEL             = BURBANK_OFFSET + 16,
    TUNE_THE_CHANNEL_RESP        = BURBANK_OFFSET + 17,

    IEEE1394_MERCURY_RESET       = IEEE1394_EVENT_OFFSET,         /* 0x7000 */
    IEEE1394_MERCURY_ARM_RESET   = IEEE1394_EVENT_OFFSET + 1,
    IEEE1394_MERCURY_DOWNLOAD    = IEEE1394_EVENT_OFFSET + 2,
    IEEE1394_RESET               = IEEE1394_EVENT_OFFSET + 3,
    IEEE1394_MERCURY_STARTUP     = IEEE1394_EVENT_OFFSET + 4,
    IEEE1394_MERCURY_OPEN_DEVICE = IEEE1394_EVENT_OFFSET + 5,
    IEEE1394_MERCURY_ARM_RESET_ACTIVE =  IEEE1394_EVENT_OFFSET + 6,
    IEEE1394_MERCURY_CFR_READ     = IEEE1394_EVENT_OFFSET + 7,
    IEEE1394_MERCURY_CFR_WRITE    = IEEE1394_EVENT_OFFSET + 8,
    IEEE1394_MERCURY_IF_DISABLE    = IEEE1394_EVENT_OFFSET + 9,
    IEEE1394_MERCURY_IF_ENABLE    = IEEE1394_EVENT_OFFSET + 10,
    IEEE1394_MERCURY_SEND_UNEXPECTED_EVENT = IEEE1394_EVENT_OFFSET + 11,
    IEEE1394_MERCURY_RESET_ACTIVE   = IEEE1394_EVENT_OFFSET + 12,
    IEEE1394_MERCURY_RESET_RELEASE  = IEEE1394_EVENT_OFFSET + 13,
    IEEE1394_MERCURY_DEVICE_CLOSE   = IEEE1394_EVENT_OFFSET + 14,
    IEEE1394_MERCURY_GET_GUID_LIST_EVENT    = IEEE1394_EVENT_OFFSET + 15,
    IEEE1394_MERCURY_INITIATE_RESET_EVENT   = IEEE1394_EVENT_OFFSET + 16,
    IEEE1394_MERCURY_INITIALIZE_CMD         = IEEE1394_EVENT_OFFSET + 17,
    IEEE1394_TEST_MIDDLEWARE                = IEEE1394_EVENT_OFFSET + 18,
    IEEE1394_MERCURY_INITIALIE              = IEEE1394_EVENT_OFFSET + 19,
    IEEE1394_READ_DTCP_KEY                  = IEEE1394_EVENT_OFFSET + 20,

    // CCI events
    CCI_SET_EMI                 = CCI_OFFSET,        /* 0xB000 */
    CCI_SET_APS                 = CCI_OFFSET + 1,
    CCI_BB_SET_APS              = CCI_OFFSET + 2,
    CCI_BB_1394DEVICE_IN_USE    = CCI_OFFSET + 3,

    // graphics events
    GRAPHICS_OPEN_DEVICE            = GRAPHICS_OFFSET,         /* 0x10000 */       
    GRAPHICS_CLOSE_DEVICE           = GRAPHICS_OFFSET + 1,               
                                        
    GRAPHICS_SURFACE_CREATE         = GRAPHICS_OFFSET + 2,            
    GRAPHICS_SURFACE_CREATE_FROM_SURFACE = GRAPHICS_OFFSET + 3,
    GRAPHICS_SURFACE_DESTROY        = GRAPHICS_OFFSET + 4,            
    GRAPHICS_SURFACE_SHOW           = GRAPHICS_OFFSET + 5,               
    GRAPHICS_SURFACE_MOVE           = GRAPHICS_OFFSET + 6,        
    GRAPHICS_SURFACE_HIDE           = GRAPHICS_OFFSET + 7,        
    GRAPHICS_SURFACE_FILL           = GRAPHICS_OFFSET + 8,        
                                        
    GRAPHICS_LOAD_CLUT              = GRAPHICS_OFFSET + 9,        
    GRAPHICS_UNLOAD_CLUT            = GRAPHICS_OFFSET + 10,        
                                        
    GRAPHICS_SURFACE_SET_ALPHA      = GRAPHICS_OFFSET + 11,        
    GRAPHICS_SURFACE_SET_ZORDER     = GRAPHICS_OFFSET + 12,        
    GRAPHICS_SURFACE_SET_VIEW_RECT  = GRAPHICS_OFFSET + 13,    
    GRAPHICS_SURFACE_SET_ALPHA_TYPE = GRAPHICS_OFFSET + 14,    
    GRAPHICS_SURFACE_FILL_RECT      = GRAPHICS_OFFSET + 15,       
    GRAPHICS_SURFACE_FILL_RECT_SOFT = GRAPHICS_OFFSET + 16,    
                                        
    GRAPHICS_COPY_BLIT              = GRAPHICS_OFFSET + 17,    
    GRAPHICS_BLEND_BLIT             = GRAPHICS_OFFSET + 18,    
                                        
    GRAPHICS_SEND_CMD_QUEUE         = GRAPHICS_OFFSET + 19,    
    GRAPHICS_SEND_CMD_Q_ASYNC       = GRAPHICS_OFFSET + 20,    
                                        
    GRAPHICS_ADD_TRANSACTION_DELAY  = GRAPHICS_OFFSET + 21,    
    GRAPHICS_RETURN_FROM_DELAY      = GRAPHICS_OFFSET + 22,

    GRAPHICS_COPY_RECT_DMA          = GRAPHICS_OFFSET + 23,
    GRAPHICS_FREE_DMA_BUFFER        = GRAPHICS_OFFSET + 24,
    GRAPHICS_ALLOC_DMA_BUFFER       = GRAPHICS_OFFSET + 25,

    GRAPHICS_SET_CHROMA_KEY         = GRAPHICS_OFFSET + 26,


    // CC events
    CC_FOUND_708                    = CC_OFFSET + 1,    /* 0x10500 */
    CC_708_CMD_EVENT                = CC_OFFSET + 2,
    CC_708_DELAY_EVENT              = CC_OFFSET + 3,
    CC_PROBE_FOR_708                = CC_OFFSET + 4,
    CC_SHUTDOWN_EVENT               = CC_OFFSET + 5,
    CC_STARTUP_EVENT                = CC_OFFSET + 6,
    CC_708_SHOW_EVENT               = CC_OFFSET + 7,
    CC_708_HIDE_EVENT               = CC_OFFSET + 8,
    CC_708_SERVICE_EVENT            = CC_OFFSET + 9,
    CC_708_ANALOG_DT_EVENT          = CC_OFFSET + 10,
    CC_608_TIMER_EVENT              = CC_OFFSET + 11,
    CC_608_SERVICE_EVENT            = CC_OFFSET + 12,
    CC_FORCE_CHANNEL_CHANGE         = CC_OFFSET + 13,

    CC_INIT                         = CC_OFFSET + 14,
    CC_SHUTDOWN                     = CC_OFFSET + 15,
    CC_SET_STATE                    = CC_OFFSET + 16,
    CC_GET_STATE                    = CC_OFFSET + 17,
    CC_SET_ANALOG_CHANNEL           = CC_OFFSET + 18,
    CC_GET_ANALOG_CHANNEL           = CC_OFFSET + 19,
    CC_SET_DIGITAL_SERVICE          = CC_OFFSET + 20,
    CC_GET_DIGITAL_SERVICE          = CC_OFFSET + 21,
    CC_SET_DIGITAL_FONT_SIZE        = CC_OFFSET + 22,
    CC_GET_DIGITAL_FONT_SIZE        = CC_OFFSET + 23,
    CC_SET_DIGITAL_FONT_STYLE       = CC_OFFSET + 24,
    CC_GET_DIGITAL_FONT_STYLE       = CC_OFFSET + 25,
    CC_SET_DIGITAL_FG_COLOR         = CC_OFFSET + 26,
    CC_GET_DIGITAL_FG_COLOR         = CC_OFFSET + 27,
    CC_SET_DIGITAL_FG_OPACITY       = CC_OFFSET + 28,
    CC_GET_DIGITAL_FG_OPACITY       = CC_OFFSET + 29,
    CC_SET_DIGITAL_BG_COLOR         = CC_OFFSET + 30,
    CC_GET_DIGITAL_BG_COLOR         = CC_OFFSET + 31,
    CC_SET_DIGITAL_BG_OPACITY       = CC_OFFSET + 32,
    CC_GET_DIGITAL_BG_OPACITY       = CC_OFFSET + 33,
    CC_GET_STRING_SIZE              = CC_OFFSET + 34,
    CC_DRAW_STRING                  = CC_OFFSET + 35,
    CC_DRAW_CLEAR                   = CC_OFFSET + 36,

    CC_IDLE_STATE_CHANGE            = CC_OFFSET + 50,
    CC_608_STATE_CHANGE             = CC_OFFSET + 51,
    CC_708_STATE_CHANGE             = CC_OFFSET + 52,
    CC_SET_SERVICES                 = CC_OFFSET + 53,
    CC_CLEAR_EIACMDS                = CC_OFFSET + 54,
    CC_DRAW_STRING_STATE_STARTED    = CC_OFFSET + 55,


    // EAS events
    EAS_OPEN                        = EAS_OFFSET,              // 0x11000
    EAS_CLOSE,                      //                   11001
    EAS_GET_EVENT_SYNC,             //                   11002
    EAS_GET_EVENT_ASYNC,            //                   11003
    EAS_EVENT_RECEIVED,             //                   11004
    EAS_EVENT_COMPLETE,             //                   11005

    // POD events
        /* API-related (via BB) */
    POD_OPEN             = POD_OFFSET,              // 0x11100
    POD_CLOSE,                      //                   11101

    POD_IB_TUNER_START,             //                   11102
    POD_IB_TUNER_BLOCK,             //                   11103
    POD_IB_TUNER_FREE,              //                   11104

    POD_SERVER_QUERY,               //                   11105
    POD_SERVER_REPLY,               //                   11106

    POD_OPEN_MMI_REQUEST,           //                   11107
    POD_MMI_DATA,                   //                   11108
    POD_MMI_SERVER_REPLY,           //                   11109

    POD_CA_HAS_BEEN_UPDATED,        //                   1110A
    POD_HOMING_USER_NOTIFY_MESS,            //                   1110B
    POD_HOMING_USER_END_MESS,           //                   1110C
    POD_HOST_FIRMUPGRADE_START,     //                   1110D
    POD_HOST_FIRMUPGRADE_END,      //                    1110E
    POD_DISPLAY_ERROR,              //                   1110F

    POD_CLOSE_MMI_REQUEST = POD_OFFSET + 0x10,      // 0x11110

    POD_ENTER_STANDBY     = POD_OFFSET + 0x20,      // 0x11120
    POD_EXIT_STANDBY,               //                   11121
  POD_CP_CARD_CERT_ERROR,		//                11122	
	POD_STANDBY_NOTIFICATION,//11123
    POD_FEATURE_LIST_REQ = POD_OFFSET + 0x30,   // 0x11130
    POD_FEATURE_LIST_CNF,           //                   11131
    POD_FEATURE_LIST_X,             //                   11132
    POD_FEATURE_LIST_CHANGE,        //                   11133

    POD_FEATURE_PARAMS_REQ = POD_OFFSET + 0x40, // 0x11140
    POD_FEATURE_PARAMS_CNF,         //                   11141
    POD_FEATURE_PARAMS,             //                   11142
    POD_HOST_FEATURE_PARAM_CHANGE,//11143
        /* state machine-related */
    POD_INITIALIZE        = POD_OFFSET + 0x50,      // 0x11150
    POD_RUN_STACK,                  //                   11201
    POD_INTERFACE_LOADED,           //                   11202
    POD_RESTART,                    //                   11203
    POD_TERMINATE,                  //                   11204

    POD_OPEN_MMI_CONF,           //                   11205
    POD_CLOSE_MMI_CONF,		 //		      11206
    
        /* pod resource-related */
    POD_RESOURCE_PRESENT  = POD_OFFSET + 0x200,      // 0x11300
        /* application manager originated */
    POD_OPEN_SESSION_REQ  = POD_RESOURCE_PRESENT + 0x70,      // 0x11370
    POD_SESSION_OPENED,             //                   11371
    POD_CLOSE_SESSION,              //                   11372
    POD_APDU,                       //                   11373
        /* application info additional */
    POD_AI_APPINFO_CNF,             //                   11374
    POD_HC_OOB_TUNE_RESP,           //                   11375
    POD_HC_IB_TUNE_RESP,            //                   11376

    POD_SET_EMI_VALUE,
    POD_TIMER_EXPIRES,              //                   11377
    POD_CA_PMT_READY,               //                   11378
    POD_INSERTED,		    //			 11379
    POD_REMOVED,			    //                   11380
    POD_CP_PMT_READY,			 //11381
    POD_SYSTEM_DOWNLD_CTL,	                //      11382
    POD_CA_NO_SELECT,                     //11383
		POD_IPDU_DOWNLOAD_TIMER,							//11384
		POD_IPDU_RECV_FLOW_DATA,							//11385
		POD_IPDU_UPLOAD_TIMER,  							//11386
    POD_UDP_SOCKET_DATA,                            //11387
    POD_ERROR			= POD_ERROR_OFFSET+10,



    /* event for NVM Server */
    NVMM_READ_OP                    = NVM_OFFSET,     // 0x13000
    NVMM_WRITE_OP                   = NVM_OFFSET + 1,
    NVMM_CREATE_OP                  = NVM_OFFSET + 2,
    NVMM_OPEN_OP                    = NVM_OFFSET + 3,
    NVMM_CLOSE_OP                   = NVM_OFFSET + 4,
    NVMM_REMOVE_OP                  = NVM_OFFSET + 5,
    NVMM_FLUSH_OP                   = NVM_OFFSET + 6,
    NVMM_FORCE_INITIALIZATION       = NVM_OFFSET + 7,

    /* event for MBoard Server */
    MBOARD_READ                         = MBOARD_OFFSET,     // 0x14000
    MBOARD_READ_TO                      = MBOARD_OFFSET + 1,
    MBOARD_WRITE                        = MBOARD_OFFSET + 2,
    MBOARD_WRITE_TO_SD                  = MBOARD_OFFSET + 3,
    MBOARD_PROTOCOL                     = MBOARD_OFFSET + 4,
    MBOARD_PROTOCOL_TO                  = MBOARD_OFFSET + 5,
    MBOARD_WRITE_DIRECT                 = MBOARD_OFFSET + 6,
    MBOARD_TUNE                         = MBOARD_OFFSET + 7,
    MBOARD_CHANNEL_INFO                 = MBOARD_OFFSET + 8,
    MBOARD_CHANNEL_INFO_NOTIFY          = MBOARD_OFFSET + 9,
    MBOARD_DIGITAL_AUDIO_RESPONSE       = MBOARD_OFFSET + 10,
    MBOARD_GET_DIGITAL_AUDIO_INFO       = MBOARD_OFFSET + 11,
    MBOARD_BROADCASTING_CONDITION_NOTIFY = MBOARD_OFFSET + 12,
    MBOARD_SYSTEM_QUERY_REQUEST         = MBOARD_OFFSET + 13,
    MBOARD_GET_DTV_BOARD_VERSION_REQUEST = MBOARD_OFFSET + 14,
    MBOARD_SET_MONITOR_OUTPUT_REQUEST   = MBOARD_OFFSET + 15,
    MBOARD_SET_CHANNEL_LINEUP_REQUEST   = MBOARD_OFFSET + 16,
    MBOARD_POD_STATUS_NOTIFY            = MBOARD_OFFSET + 17,
    MBOARD_MTS_REQUEST                  = MBOARD_OFFSET + 18,
    MBOARD_MTS_RESPONSE                 = MBOARD_OFFSET + 19,
    MBOARD_SET_MONITOR_OUTPUT_RESPONSE  = MBOARD_OFFSET + 20,
    MBOARD_AUDIO_CHANGE_NOTIFY          = MBOARD_OFFSET + 21,
    MBOARD_GET_DTV_CHARACTERISTICS_REQUEST      = MBOARD_OFFSET + 22,
    MBOARD_RESET_DTV_REQUEST           =  MBOARD_OFFSET + 23,
    MBOARD_PHYSICAL_POWER_ON_COMMAND    = MBOARD_OFFSET + 24,
    MBOARD_LOGICAL_STATE_CHANGE_COMMAND = MBOARD_OFFSET + 25,
    MBOARD_GET_DTV_BOARD_VERSION_RESPONSE = MBOARD_OFFSET + 26,
    MBOARD_GET_DTV_CHARACTERISTICS_RESPONSE = MBOARD_OFFSET + 27,
    MBOARD_RESET_DTV_RESPONSE           = MBOARD_OFFSET + 28,
    MBOARD_SYSTEM_QUERY_RESPONSE        = MBOARD_OFFSET + 29,
    MBOARD_LINK_LEVEL_ERROR_NOTIFY      = MBOARD_OFFSET + 30,

    kEt_IrbBlastData                    = IR_BLASTER_OFFSET + 0,
    CLOSE_BLASTER_DEVICE                = IR_BLASTER_OFFSET + 1,
    BLAST_STRING                        = IR_BLASTER_OFFSET + 2,
    OPEN_BLASTER_DEVICE                 = IR_BLASTER_OFFSET + 3,

    PAXEL_INTERRUPT                     = PAXEL_OFFSET,

    ASYNC_OPEN                          = ASYNC_OFFSET,
    ASYNC_CLOSE                         = ASYNC_OFFSET+1,
    ASYNC_INBAND_1FFB_DATA              = ASYNC_OFFSET+2,
    ASYNC_START_PAT                     = ASYNC_OFFSET+3,
    ASYNC_STOP_PAT                      = ASYNC_OFFSET+4,
    ASYNC_START_PMT                     = ASYNC_OFFSET+5,
    ASYNC_STOP_PMT                      = ASYNC_OFFSET+6,
    ASYNC_START_VCT                     = ASYNC_OFFSET+7,
    ASYNC_STOP_VCT                      = ASYNC_OFFSET+8,
    ASYNC_PMT_START                     = ASYNC_OFFSET+9,
    ASYNC_PMT_STOP                      = ASYNC_OFFSET+10,
    ASYNC_EAS_START                     = ASYNC_OFFSET+11,
    ASYNC_EAS_STOP                      = ASYNC_OFFSET+12,
    ASYNC_MGT_START                     = ASYNC_OFFSET+13,
    ASYNC_MGT_STOP                      = ASYNC_OFFSET+14,
    ASYNC_RRT_START                     = ASYNC_OFFSET+15,
    ASYNC_RRT_STOP                      = ASYNC_OFFSET+16,
    ASYNC_BB_VCT_START                  = ASYNC_OFFSET+17,
    ASYNC_BB_VCT_STOP                   = ASYNC_OFFSET+18,
    ASYNC_PMT_GET                       = ASYNC_OFFSET+19,
    ASYNC_START_CCI_PMT                 = ASYNC_OFFSET+20,
    ASYNC_STOP_CCI_PMT                  = ASYNC_OFFSET+21,
    ASYNC_GET_PMT_PID                   = ASYNC_OFFSET+22,

    ASYNC_OB_OPEN                       = ASYNC_OFFSET+30,
    ASYNC_OB_CLOSE                      = ASYNC_OFFSET+31,
    ASYNC_CHANNEL_MAP_FOUND             = ASYNC_OFFSET+32,
    ASYNC_NEW_CHANNEL_MAP_FOUND         = ASYNC_OFFSET+33,
    ASYNC_SVCT_PID_ON_LINE              = ASYNC_OFFSET+34,
    ASYNC_EC_FLOW_REQUEST               = ASYNC_OFFSET+35,
    ASYNC_EC_FLOW_REQUEST_CNF           = ASYNC_OFFSET+36,
    ASYNC_EC_FLOW_DELETE                = ASYNC_OFFSET+37,
    ASYNC_EC_FLOW_DELETE_CNF            = ASYNC_OFFSET+38, 
    ASYNC_EC_FLOW_LOST_EVENT            = ASYNC_OFFSET+39, 
    ASYNC_EC_FLOW_LOST_EVENT_CNF        = ASYNC_OFFSET+40, 
    ASYNC_EC_DATA_PACKET                = ASYNC_OFFSET+41, 
    ASYNC_OPEN_FLOW_DEVICE              = ASYNC_OFFSET+42, 
    ASYNC_PID1ffc_RESULT                = ASYNC_OFFSET+43,
    ASYNC_PID1ffc_DATA                  = ASYNC_OFFSET+44,
    ASYNC_PID1ffc_LOST                  = ASYNC_OFFSET+45,
    ASYNC_OB_PODLESS_OPEN               = ASYNC_OFFSET+46,
    ASYNC_OB_NEW_VCT_TABLES             = ASYNC_OFFSET+47,
    ASYNC_TDB_OB_DATA                   = ASYNC_OFFSET+48,
    ASYNC_STT_EVENT_RECEIVED            = ASYNC_OFFSET+49,
    ASYNC_START_STT                     = ASYNC_OFFSET+50,
    ASYNC_OB_TURN_ON_LOGGING            = ASYNC_OFFSET+51,
    ASYNC_OB_TURN_OFF_LOGGING           = ASYNC_OFFSET+52,
    ASYNC_START_OOB_QPSK_IP_FLOW        = ASYNC_OFFSET+53,
    ASYNC_STOP_OOB_QPSK_IP_FLOW         = ASYNC_OFFSET+54,
    // IPDU register flow
    ASYNC_START_IPDU_XCHAN_FLOW         = ASYNC_OFFSET+55,
    ASYNC_STOP_IPDU_XCHAN_FLOW          = ASYNC_OFFSET+56,

    ICD_RESET_DTV_REQUEST               = ICD_MSGS_OFFSET,
    ICD_ANTENNA_AND_SOURCE              = ICD_MSGS_OFFSET+1,

    FLASHCK_DEVICE_INIT                 = FLASHCK_OFFSET,          /* 0x1A000 */
    FLASHCK_CHECK_FLASH_EVENT           = FLASHCK_OFFSET + 1,
    FLASHCK_FAIL_EVENT           		= FLASHCK_OFFSET + 2,

    WATCHDOG_DEVICE_INIT                = WATCHDOG_OFFSET,         /* 0x1B000 */
    WATCHDOG_START                      = WATCHDOG_OFFSET + 1,

    GET_RESTART_INFO,

    VL_DSG_EVENT_ENTERING_DSG_MODE      = VL_DSG_EVENT_ENUM_OFFSET + 0,
    VL_DSG_EVENT_LEAVING_DSG_MODE       = VL_DSG_EVENT_ENUM_OFFSET + 1,
    VL_CDL_DISP_MESSAGE			= CDL_OFFSET,// 0x3100	//0x31000
	
    

}SYSTEM_EVENTS;


typedef enum
{

	POD_SESSION_NOT_FOUND=0x10
	



}POD_ERROR_EVENT;


#endif
 
 
 
 
