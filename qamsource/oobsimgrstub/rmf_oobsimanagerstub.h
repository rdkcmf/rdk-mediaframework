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

#ifndef _RMF_OOBSIMANAGER_STUB_H_
#define _RMF_OOBSIMANAGER_STUB_H_

/**
 * @file rmf_oobsimanagerstub.h
 *
 * @defgroup OOBSimgrstub QAM Source - OOB SI Manager Stub
 * @ingroup QAM
 * @defgroup OOBStubClasses OOB SI Manager Stub Class
 * @ingroup OOBSimgrstub
 * @defgroup OOBStubdatastructures OOB SI Manager Stub Data Structures
 * @ingroup OOBSimgrstub
 */

#include "rmf_osal_types.h"
#include "rmf_error.h"
#include "rmf_qamsrc_common.h"
#include <vector>

/**
 * @enum oob_modulation_mode
 * @brief This enum indicates the modulation modes.
 * @ingroup OOBStubdatastructures
 */
enum oob_modulation_mode
{
	OOB_MODULATION_MODE_ANALOG =1,
	OOB_MODULATION_MODE_SCTE_mode_1=8, //!< 64QAM
	OOB_MODULATION_MODE_SCTE_mode_2=16, //!< 256QAM
};


/**
 * @class OOBSIManager
 * @ingroup OOBStubClasses
 * @brief This class defines the functionalities to manage the program information list.
 */
class OOBSIManager
{
	private:
		static OOBSIManager* oobSIManager;
		OOBSIManager();
		OOBSIManager(const char* file);

	public:
		static OOBSIManager* getInstance();
		rmf_Error  GetProgramInfo ( uint32_t sourceID, rmf_ProgramInfo_t* p_info);
		static rmf_Error  add_program_info ( uint32_t sourceID, rmf_ProgramInfo_t* p_info);
		rmf_Error  dump_program_info ( const char* path );
		void get_source_id_vector(std::vector<uint32_t> &source_id_list);
};

#endif /*_RMF_OOBSIMANAGER_STUB_H_*/
