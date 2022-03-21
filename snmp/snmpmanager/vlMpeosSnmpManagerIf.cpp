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

/**
 * @file This file contains the initialization and starting of boot event handler
 * and snmp manager init task thread.
 */

#include "sys_init.h"
#include "snmpmanager.h"
#include "utilityMacros.h"
#include "bufParseUtilities.h"
#include <vector>
#include "cardUtils.h"
#include "rmf_error.h"

#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif

#define __MTAG__ VL_SNMP_MANAGER

static SnmpManager        * vlg_pSnmpManager     = NULL;

/**
 * @brief This function is used to do initialization and start BootEventHandler
 * thread.
 *
 * @param[in] arg Void pointer, currently not in use. 
 *
 * @return None.
 */
static void vl_snmp_manager_init_task(void* arg)
{
    rmf_osal_threadSleep(10000,0);
    static int bInitialized = 0;
    if(0 == bInitialized)
    {
        bInitialized = 1;
        if(NULL == vlg_pSnmpManager)
        {
            vlg_pSnmpManager = new SnmpManager;
            if(NULL != vlg_pSnmpManager)
            {
                vlg_pSnmpManager->initialize();
                vlg_pSnmpManager->init_complete();
            }
        }
    }
}

/**
 * @brief This function is used to create the snmp manager init task thread.
 *
 * @retval RMF_SUCCESS Default return value. Indicates success.
 */
rmf_Error vlMpeosSnmpManagerInit()
{
rmf_osal_ThreadId threadid = 0;
rmf_osal_threadCreate(vl_snmp_manager_init_task, NULL , RMF_OSAL_THREAD_PRIOR_DFLT, RMF_OSAL_THREAD_STACK_SIZE,& threadid, "vl_snmp_manager_init_task");
 return RMF_SUCCESS;
}
