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



#include "stdio.h"
#include "stdlib.h"
//#include "vlMemCpy.h"
#include "utilityMacros.h"
#include "vlSnmpHostInfo.h"
#include "hostGenericFeatures.h"
#include <string.h>
#include "persistentconfig.h"
//#include "cSystem.h"
#include "vlSnmpClient.h"
#include "vlSnmpEcmApi.h"
#include "vl_ocStbHost_GetData.h"
#include "sys_api.h"
#include "sys_init.h"
#include "sys_api.h"
#include "ipcclient_impl.h"
#include <fstream>

#define vlStrCpy(pDest, pSrc, nDestCapacity)            \
            strcpy(pDest, pSrc)

#define vlMemCpy(pDest, pSrc, nCount, nDestCapacity)            \
            memcpy(pDest, pSrc, nCount)

#define VL_ZERO_MEMORY(x) \
            memset(&x,0,sizeof(x))                                                  
    
#ifdef __cplusplus
extern "C" {
#endif

//User preffered audio language setting.
static char vlg_UserSettingsPreferedLanguage[VL_MAX_SNMP_STR_SIZE] = "eng";

VL_HOST_SNMP_API_RESULT vlSnmpGetHostInfo(VL_SNMP_VAR_TYPE eSnmpVarType, void * _pvSNMPInfo)
{
	return VL_HOST_SNMP_API_RESULT_SUCCESS;
}

//Moving actual reboot call to a different task to allow SNMP to send ACK for the request.
static void vlSnmpRebootHost_task(void* arg)
{
	rmf_osal_threadSleep(1000,0);
    // Added new API to handle reboot
    IPC_CLIENT_HAL_SYS_Reboot(__FUNCTION__, "SNMP Host Reset");
}

VL_HOST_SNMP_API_RESULT vlSnmpRebootHost(VL_SNMP_HOST_REBOOT_INFO eRebootReason)
{
    return VL_HOST_SNMP_API_RESULT_SUCCESS;
}

VL_HOST_SNMP_API_RESULT vlSnmpSetHostInfo(VL_SNMP_VAR_TYPE eSnmpVarType, const void * _pvSNMPInfo)
{
    VL_HOST_SNMP_API_RESULT  result = VL_HOST_SNMP_API_RESULT_SUCCESS;
    char * pszStr = NULL;

    switch(eSnmpVarType)
    {
            case VL_SNMP_VAR_TYPE_OcstbHostRebootInfo_rebootResetinfo:
            {
                    VL_SNMP_VAR_REBOOT_INFO * pSNMPInfo = (VL_SNMP_VAR_REBOOT_INFO*)_pvSNMPInfo;
                    SNMP_DEBUGPRINT("vlSnmpSetHostInfo: Received VL_SNMP_VAR_TYPE_OcstbHostRebootInfo: rebootResetinfo = %d  \n", pSNMPInfo->rebootResetinfo);
                    if(VL_SNMP_HOST_REBOOT_REQUEST_do_not_reboot != pSNMPInfo->rebootResetinfo)
                    {

                            vlSnmpRebootHost(VL_SNMP_HOST_REBOOT_INFO_user);
                    }
                return VL_HOST_SNMP_API_RESULT_SUCCESS;
            }


            case VL_SNMP_VAR_TYPE_ocStbHostUserSettingsPreferedLanguage:
                {
                        VL_SNMP_VAR_USER_PREFERRED_LANGUAGE * pSNMPInfo = (VL_SNMP_VAR_USER_PREFERRED_LANGUAGE*)_pvSNMPInfo;
                        
                        vlStrCpy(vlg_UserSettingsPreferedLanguage, pSNMPInfo->UserSettingsPreferedLanguage, sizeof(pSNMPInfo->UserSettingsPreferedLanguage));

                        return VL_HOST_SNMP_API_RESULT_SUCCESS;
                }
    }

    return VL_HOST_SNMP_API_RESULT_FAILED;
}

#ifdef __cplusplus
extern "C" {
#endif
u_long gsysORLastTimeTick;
#ifdef __cplusplus
}
#endif
VL_HOST_SNMP_API_RESULT vl_SnmpHostSysteminfo(VL_SNMP_SYSTEM_VAR_TYPE systype, void *_Systeminfo)
{
    return VL_HOST_SNMP_API_RESULT_SUCCESS;
}


#ifdef __cplusplus
}
#endif
