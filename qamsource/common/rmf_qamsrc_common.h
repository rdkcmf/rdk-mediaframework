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
 
#ifndef __RMF_QAMSRC_COMMON_H__
#define __RMF_QAMSRC_COMMON_H__

#include <rmf_osal_types.h>


/**
 * @file rmf_qamsrc_common.h
 * @brief File contains the RMFQAM Source common inforamtions.
 */


/**
 * @enum  _rmf_ModulationMode
 * @brief This enumeration represents the different types of modulation modes.
 * @ingroup rmfqamsrcdatastructures
 */
/* Enums for Tuner Modulation mode */
typedef enum _rmf_ModulationMode
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
}rmf_ModulationMode;


/**
 * @struct rmf_ProgramInfo
 * @brief This structure representing the program information.
 * @ingroup rmfqamsrcdatastructures
 */
typedef struct rmf_ProgramInfo
{
#ifdef TEST_WITH_PODMGR
	rmf_ModulationMode modulation_mode;
	uint32_t service_handle;
#else
	uint32_t modulation_mode;
#endif
	uint32_t carrier_frequency;
	uint32_t prog_num;
	uint32_t symbol_rate;
}rmf_ProgramInfo_t;

#define QAMSRC_DEF_NUMBER_OF_TUNERS 1


#endif /*__RMF_QAMSRC_COMMON_H__*/
