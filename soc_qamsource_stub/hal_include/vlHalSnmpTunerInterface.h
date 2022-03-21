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




#ifndef __VL_HAL_SNMP_TUNER_INTERFACE_H__
#define __VL_HAL_SNMP_TUNER_INTERFACE_H__

/**
 * @defgroup HAL_SNMP_Tuner_Interface  SNMP HAL Tuner APIs
 * @ingroup  HAL_SNMP_Interface
 * @defgroup HAL_SNMP_Tuner_Types   SNMP HAL Tuner Datatypes
 * @ingroup  HAL_SNMP_Types
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Represents the tuner modulation methods.
 */
typedef enum
{
	TUNER_NONE,
	TUNER_QPSK,   /* The tuner is in QPSK mode for OOB */
	TUNER_BPSK,	     /* The tuner is in BPSK mode */
	TUNER_OQPSK,	     /* The tuner is in OQPSK mode */
	TUNER_VSB8,       /* The tuner is in VSB8 mode */
	TUNER_VSB16,      /* The tuner is in VSB16 mode */
	TUNER_QAM16,      /* The tuner is in QAM16 mode */
	TUNER_QAM32,      /* The tuner is in QAM32 mode */
	TUNER_QAM64,      /* The tuner is in QAM64 mode */
	TUNER_QAM80,      /* The tuner is in QAM80 mode */
	TUNER_QAM96,      /* The tuner is in QAM96 mode */
	TUNER_QAM112,     /* The tuner is in QAM112 mode */
	TUNER_QAM128,     /* The tuner is in QAM128 mode */
	TUNER_QAM160,     /* The tuner is in QAM160 mode */
	TUNER_QAM192,     /* The tuner is in QAM192 mode */
	TUNER_QAM224,     /* The tuner is in QAM224 mode */
	TUNER_QAM256,     /* The tuner is in QAM256 mode */
	TUNER_QAM320,     /* The tuner is in QAM320 mode */
	TUNER_QAM384,     /* The tuner is in QAM384 mode */
	TUNER_QAM448,     /* The tuner is in QAM448 mode */
	TUNER_QAM512,     /* The tuner is in QAM512 mode */
	TUNER_QAM640,     /* The tuner is in QAM640 mode */
	TUNER_QAM768,     /* The tuner is in QAM768 mode */
	TUNER_QAM896,     /* The tuner is in QAM896 mode */
	TUNER_QAM1024,    /* The tuner is in QAM1024 mode */
	TUNER_ANALOG,     /* The tuner is in Analog mode */
	TUNER_NUM_OF_MODES
} SNMP_INTF_TUNER_MODE_t ;

/**
 * @brief Represents the tuner lock status.
 */
typedef enum {
	TUNER_DEVICE_LOCKED,
	TUNER_DEVICE_UNLOCKED,
	TUNER_DEVICE_TIMEOUT
}SNMP_INTF_DEVICE_LOCK_t;

/**
 * @brief Represents the tuner FEC(Forward Error Correction) status.
 */
typedef struct
{
        unsigned long  ulCorrBytes;     /* Number of correctable byte errors for time period */
        unsigned long  ulUnCorrBlocks;  /* Total number of uncorrectable block errors for time period */
        unsigned long  ulUnerroreds;     /* Codewords received on this channel without errors*/
        unsigned long  ulTimeForBER;    /* Time in seconds, over which Read-Solomon was calculated */
}SNMP_INTF_TUNER_FEC_STATUS_t;

/**
 * @brief Represents the in-band tuner information.
 */
typedef struct
{
	unsigned long          		tunerIndex; 
	SNMP_INTF_TUNER_MODE_t    	enTunerMode;
	unsigned long    		ulFrequency;   // in KHz
	SNMP_INTF_DEVICE_LOCK_t    	enLockStatus;           /* Current frequency lock */
	SNMP_INTF_TUNER_FEC_STATUS_t  	stFEC_Status;           /* FEC status */
	unsigned long 			ulRangeMin;  /* The lowest valid frequency in kHz */
	unsigned long 			ulRangeMax;  /* The Highest valid frequency in kHz */
	//For SNMP MIB value support
	unsigned long              	ulAGC;                  /* AGC compared to stored values */
	unsigned long              	ulSNR;                  /* Current signal to noise ratio in DB*/
	unsigned long              	ulBER;                  /* Bit Error Rate range */
	unsigned long              	ulPower;                /* in Tenth dBmV*/
	unsigned long              	ulInterleaver;
	unsigned long              	ulEqGain;
	unsigned long              	ulMainTapCoeff;
	unsigned long              	ulPCRErrors;
	unsigned long              	ulPTSErrors;
	unsigned long              	totalTuneCount;
	unsigned long              	tuneFailCount;
	unsigned long              	lastTuneFailFreq;
	unsigned long              	lockedTimeInSecs;
	unsigned long              	carrierLockLostCount;
    int                         bandwidthCode;
	unsigned long             	ulTotalExtChBytesRead; //For QPSK only
        unsigned long                   ulCorrBytes;
        unsigned long                   ulUnCorrBlocks;
        unsigned long                   ulUnerroreds;
	unsigned long  			channelMapId;
	unsigned long			dacId;	
	//End - For SNMP MIB value support

}SNMP_INTF_TUNER_INFO_t;

/** @} */

/**
 * @addtogroup  HAL_SNMP_Tuner_Interface
 * @{
 */

/**
 * @brief This function is used to get the details about in-band tuners like frequency, bandwidth,
 * SNR Value, BER etc.
 *
 * @param[in]  tunerIndex          Index number of the available tuner.
 * @param[out] ptSnmpHALTunerInfo  Details about in-band tuner is stored
 *
 * @return  Returns the status of the operation.
 * @retval  Returns 1 on success, appriopiate error code otherwise.
 */
unsigned int HAL_SNMP_Get_Tuner_Info(void* ptSnmpHALTunerInfo, unsigned long tunerIndex);

/**
 * @brief This function is used to get the number of available tuners used by the  hal snmp.
 * In case of failure, it takes the count of tuners from the config variable "MPE.SYS.NUM.TUNERS"
 *
 * @param[out] ptTunerCount  Total number of tuners.
 *
 * @return  Returns the status of the operation.
 * @retval  Returns 1 on success, appriopiate error code otherwise
 */
unsigned int HAL_SNMP_Get_Tuner_Count(unsigned int* ptTunerCount);

/**
 * @brief This API is used to display the tuner status.
 * Invokes the HAL_SNMP_Get_Tuner_Info() to retrieve the tuner information.
 *
 * @param[in] nTunerId The index of the tuner.
 */
void HAL_SNMP_diag_dump_tuner_stats(unsigned int nTunerId);

/**
 * @brief This API dumps the mpeg2 content information like Program number,
 * TSID, Number of Streams, Copy content Information, Audio/Video/PCR pids,
 * Tuner status and also the pipeline status.
 *
 * @return  Returns the status of the operation.
 * @retval  Returns 0 on success, appriopiate error code otherwise
 */
int HAL_SNMP_dump_presentation_params();

/**
 * @brief This API displays the decoder request information.
 *
 * @param[in] pvDecodeRequest  Decoder request parameters.
 * @param[in] SessionType      This parameter is not in use.
 * @param[in] iSession         This parameter is not in use.
 */
int HAL_SNMP_dump_decode_params(void * pvDecodeRequest, int SessionType, unsigned int iSession);
/** @} */

#ifdef __cplusplus
}
#endif

#endif
