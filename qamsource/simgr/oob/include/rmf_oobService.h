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
 

/**
 * @file rmf_oobService.h
 * @brief Contains wrapper function declarations for some outband SImgr functionalities.
 */

#ifndef OOB_SERVICE_H
#define OOB_SERVICE_H

#include "rmf_error.h"

#ifdef ENABLE_TIME_UPDATE
#include "rmf_osal_time.h"
#endif

#include "rmf_qamsrc_common.h"

typedef uint32_t rmf_SiServiceHandle;

/**
 * @struct Oob_ProgramInfo
 * @brief Defines a structure to store program information.
 * @ingroup Outofbanddatastructures
 */
typedef struct Oob_ProgramInfo
{
	uint32_t carrier_frequency;
	rmf_ModulationMode modulation_mode;	
	uint32_t prog_num;
        rmf_SiServiceHandle service_handle;        	
}Oob_ProgramInfo_t;

rmf_Error OobServiceInit();
rmf_Error OobGetProgramInfo(uint32_t decimalSrcId, Oob_ProgramInfo_t *pInfo);
rmf_Error OobGetProgramInfoByVCN(uint32_t vcn, Oob_ProgramInfo_t *pInfo);
rmf_Error OobGetDacId(uint16_t *pDACId);
rmf_Error OobGetChannelMapId(uint16_t *pChannelMapId);
rmf_Error OobGetVirtualChannelNumber(uint32_t decimalSrcId, uint16_t *pVCN);

#ifdef ENABLE_TIME_UPDATE
rmf_Error OobGetSysTime(rmf_osal_TimeMillis *pSysTime);
rmf_Error OobGetTimeZone(int32_t *pTimezoneinhours, int32_t *pDaylightflag);
#endif

#endif

