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
 
#ifndef __QAMSRC_DIAG_H__
#define __QAMSRC_DIAG_H__

#include "rmf_error.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define QAM_SRC_SERVER_CMD_DFLT_PORT_NO 50512

typedef enum 
{
	QAM_SRC_SERVER_CMD_GET_PAT_TIMEOUT_COUNT,
	QAM_SRC_SERVER_CMD_GET_PMT_TIMEOUT_COUNT,
    QAM_SRC_SERVER_CMD_GET_VIDEO_CONTENT_INFO,
	
}QAM_SRC_SERVER_CMD;

/* Enumeration of valid tuner states */
typedef enum 
{
	TUNER_STATE_IDLE,
	TUNER_STATE_BUSY,
	TUNER_STATE_TIMEOUT,
	TUNER_STATE_TUNED_SYNC,
	TUNER_STATE_TUNED_NOSYNC
} TunerState;

typedef enum 
{
	TUNERSRC_EVENT_TUNE_UNKNOWN,
	TUNERSRC_EVENT_TUNE_FAIL = 0x01,
	TUNERSRC_EVENT_TUNE_SYNC, 
	TUNERSRC_EVENT_TUNE_UNSYNC, 
	TUNERSRC_EVENT_FILTERS_SET,
	TUNERSRC_EVENT_UNRECOVERABLE_ERROR, 
	TUNERSRC_EVENT_TUNE_SUCCESS,
	TUNERSRC_EVENT_SPTS_TIMEOUT,
	TUNERSRC_EVENT_SPTS_OK,
	TUNERSRC_EVENT_QUEUE_CHECKPOINT
} TunerEvent;

typedef struct
{
	uint32_t modulation;
	uint32_t frequency;
	uint32_t totalTuneCount;
	uint32_t tuneFailCount;
	uint32_t lastTuneFailFreq;
	TunerState tunerState;
	uint32_t lockedTimeInSecs;
	uint32_t carrierLockLostCount;
	uint16_t channelMapId;
	uint16_t dacId;
}TunerData;

#define RMF_MAX_MEDIAPIDS_PER_PGM 32

typedef struct
{
	uint32_t streamType; /* elementarty stream_type field in the PMT.*/
	uint32_t pid; /* PID value. */
} MediaPid;


typedef struct
{
	uint8_t cci;
	uint16_t pmtPid;
	uint16_t pcrPid;
	uint16_t numMediaPids;
	MediaPid pPidList[RMF_MAX_MEDIAPIDS_PER_PGM]; 
}ProgramInfo;

typedef enum
{
    VideoContainerFormat_None = 0,
    VideoContainerFormat_Other,
    VideoContainerFormat_mpeg2,
    VideoContainerFormat_mpeg4,
    VideoContainerFormat_vc1,
    VideoContainerFormat_mpegh,
    
    VideoContainerFormat_invalid = 0xFF
    
}VideoContainerFormat;

typedef struct VideoContentInfo
{
    unsigned int ContentIndex;
    unsigned int TunerIndex;
    unsigned int ProgramNumber;
    unsigned int TransportStreamID;
    unsigned int TotalStreams;
    unsigned int SelectedVideoPID;
    unsigned int SelectedAudioPID;
    unsigned int OtherAudioPIDs;
    unsigned int CCIValue;
    unsigned int APSValue;
    unsigned int CITStatus;
    unsigned int BroadcastFlagStatus;
    unsigned int EPNStatus;
    unsigned int PCRPID;
    unsigned int PCRLockStatus;
    unsigned int DecoderPTS;
    unsigned int Discontinuities;
    unsigned int PktErrors;
    unsigned int PipelineErrors;
    unsigned int DecoderRestarts;
    VideoContainerFormat containerFormat;
    
}VideoContentInfo_t;

rmf_Error getTunerInfo(uint32_t tunerIndex, TunerData* pTunerData);
uint32_t getTSID( uint32_t tunerIndex);
uint32_t getTunerCount();
rmf_Error getProgramInfo(uint32_t srcId, ProgramInfo* pProgramInfo);

#ifdef __cplusplus
}
#endif

#endif
