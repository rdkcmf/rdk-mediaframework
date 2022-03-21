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
//#include "utilityMacros.h"
#include "hostInfo.h"
//#include "vlHostGenericFeatures.h"
#include <string.h>
#include "persistentconfig.h"
//#include "vlpluginapp_halplatformapi.h"
//#include "cSystem.h"
//#include "vlSnmpClient.h"
#ifndef SNMP_IPC_CLIENT
#include "vlSnmpEcmApi.h"
#endif //SNMP_IPC_CLIENT
//#include "vl_ocStbHost_GetData.h"
#include "sys_api.h"
#include "sys_init.h"
#include <unistd.h>
#include <rdk_debug.h>

//#include <direct.h>

//#define	getcwd(x, y)		_getcwd(x, y)


//User preffered audio language setting.
//static char vlg_UserSettingsPreferedLanguage[VL_MAX_SNMP_STR_SIZE];
#if 0
VL_HOST_SNMP_API_RESULT vlSnmpGetHostInfo(VL_SNMP_VAR_TYPE eSnmpVarType, void * _pvSNMPInfo)
{
    VL_HOST_SNMP_API_RESULT  result = VL_HOST_SNMP_API_RESULT_SUCCESS;
    char * pszStr = NULL;


            VL_SNMP_VAR_DEVICE_SOFTWARE * pSNMPInfo = (VL_SNMP_VAR_DEVICE_SOFTWARE*)_pvSNMPInfo;
	     memset(&(*pSNMPInfo), 0, sizeof(*pSNMPInfo));	
            //VL_ZERO_MEMORY(*pSNMPInfo);
            
            //if(VL_MFR_API_RESULT_SUCCESS == CHALMfr_get_version(VL_PLATFORM_VERSION_TYPE_SOFTWARE_IMAGE_VERSION, &pszStr))
            //{
                char szStackVersion[128];
                char szSvnRevision[64];
                char szCwd[256];
                strcpy(szStackVersion, "");
                strcpy(szSvnRevision, "");
                CMpersistentConfig hostFeaturePersistentConfig;
                hostFeaturePersistentConfig.initialize(CMpersistentConfig::DYNAMIC, getcwd(szCwd, sizeof(szCwd)), "STACK_VERSION.txt");
                hostFeaturePersistentConfig.load_from_disk();
                hostFeaturePersistentConfig.read("STACK_VERSION", szStackVersion, sizeof(szStackVersion));
                hostFeaturePersistentConfig.read("SVN_REVISION", szSvnRevision, sizeof(szSvnRevision));
                strcpy(pSNMPInfo->SoftwareFirmwareVersion, szStackVersion);
                //Apr-20-2012: for PARKER-6177: Removed SVN Revisions
                //vlStrCat(pSNMPInfo->SoftwareFirmwareVersion, ".", sizeof(pSNMPInfo->SoftwareFirmwareVersion));
                //vlStrCat(pSNMPInfo->SoftwareFirmwareVersion, szSvnRevision, sizeof(pSNMPInfo->SoftwareFirmwareVersion));
                pSNMPInfo->nSoftwareFirmwareDay     = hostFeaturePersistentConfig.read("DATE_DAY", 1);
                pSNMPInfo->nSoftwareFirmwareMonth   = hostFeaturePersistentConfig.read("DATE_MONTH", 1);
                pSNMPInfo->nSoftwareFirmwareYear    = hostFeaturePersistentConfig.read("DATE_YEAR", 2009);
                /*RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SNMP", "Software: Firmware: Year = %d, Month = %d, Day = %d\n",
                       pSNMPInfo->nSoftwareFirmwareYear,
                       pSNMPInfo->nSoftwareFirmwareMonth,
                       pSNMPInfo->nSoftwareFirmwareDay);*/
                /*char * pszSavePtr = NULL;
                char szDate[100];
                vlStrCpy(szDate, __SVN_DATE__, sizeof(szDate));
                char * pszDate = szDate;
                //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "Software: Firmware: Date = %s\n", pszDate);
                pszDate = strtok_r(pszDate, "-", &pszSavePtr);
                //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "Software: Firmware: Year = %s\n", pszDate);
                if(NULL != pszDate) pSNMPInfo->nSoftwareFirmwareYear    = atoi(pszDate);
                pszDate = strtok_r(NULL, "-", &pszSavePtr);
                //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "Software: Firmware: Month = %s\n", pszDate);
                if(NULL != pszDate) pSNMPInfo->nSoftwareFirmwareMonth   = atoi(pszDate);
                pszDate = strtok_r(NULL, "-", &pszSavePtr);
                //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "Software: Firmware: Day = %s\n", pszDate);
                if(NULL != pszDate) pSNMPInfo->nSoftwareFirmwareDay     = atoi(pszDate);*/
            //}
            //{
                VL_NVRAM_DATA NvRamData;
                char * pszBootFileName="";
                memset(&NvRamData, 0, sizeof(NvRamData));
                if(VL_MFR_API_RESULT_SUCCESS == CHALMfr_read_normal_nvram (VL_NORMAL_NVRAM_DATA_BOOT_IMAGE_NAME, &NvRamData))
                {
                    pszBootFileName=(char*)(NvRamData.pData);
                }
                
                if(0 != strlen(pszBootFileName))
                {
                    strcpy(pSNMPInfo->SoftwareFirmwareImageName, pszBootFileName);
                }

					
               RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: ##%29s : %29s ##\n", __FUNCTION__, "Software Firmware Image Name", pSNMPInfo->SoftwareFirmwareImageName);

		//RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "%s: ##%29s : %29s ##\n", __FUNCTION__, "Software Firmware Image Name", pSNMPInfo->SoftwareFirmwareImageName);
            //}
            if(VL_MFR_API_RESULT_SUCCESS == CHALMfr_get_version(VL_PLATFORM_VERSION_TYPE_OCAP_VERSION, &pszStr))
            {
                if(NULL != pszStr) strcpy(pSNMPInfo->SoftwareFirmwareOCAPVersionl, pszStr);
            }
            strcpy(pSNMPInfo->SoftwareFirmwareOCAPVersionl, "1.1.4");
            if(!vl_isFeatureEnabled((unsigned char*)"CONFIG.REPORT_CDL_FILENAME_AS_FIRMWARE_NAME")) strcpy(pSNMPInfo->SoftwareFirmwareImageName, "");
            //Apr-20-2012: for PARKER-6177: Removed SVN Revisions
            //vlStrCat(pSNMPInfo->SoftwareFirmwareVersion, ".", sizeof(pSNMPInfo->SoftwareFirmwareVersion));
            //vlStrCat(pSNMPInfo->SoftwareFirmwareVersion, __SVN_REV__, sizeof(pSNMPInfo->SoftwareFirmwareVersion));

    
    return result;
}


#endif



