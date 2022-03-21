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



#ifndef __VLMPEOSSNMPMANAGERIF_H__
#define __VLMPEOSSNMPMANAGERIF_H__

#include "rmf_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VLMPEOS_SNMP_API_CHRMAX 256

typedef enum {
    VLMPEOS_SoftwareAPP_status_loaded = 4,
    VLMPEOS_SoftwareAPP_status_notLoaded = 5,
    VLMPEOS_SoftwareAPP_status_paused = 6,
    VLMPEOS_SoftwareAPP_status_running = 7,
    VLMPEOS_SoftwareAPP_status_destroyed = 8
}VLMPEOS_SoftwareAPP_status;

typedef enum {
    VLMPEOS_SoftwareAPP_SigStatus_other = 0,
    VLMPEOS_SoftwareAPP_SigStatus_okay = 1,
    VLMPEOS_SoftwareAPP_SigStatus_error = 2
}VLMPEOS_SoftwareAPP_SigStatus;

typedef struct  {
    /* Index values  */
    unsigned long           vl_ocStbHostSoftwareApplicationInfoIndex;
    /* Column values */
    char                    vl_ocStbHostSoftwareAppNameString[VLMPEOS_SNMP_API_CHRMAX];
    int                     vl_ocStbHostSoftwareAppNameString_len;
    char                    vl_ocStbHostSoftwareAppVersionNumber[VLMPEOS_SNMP_API_CHRMAX];
    int                     vl_ocStbHostSoftwareAppVersionNumber_len;
    VLMPEOS_SoftwareAPP_status    vl_ocStbHostSoftwareStatus;
    int                     vl_ocStbHostSoftwareOrganizationId; //4-BYTES
    int                     vl_ocStbHostSoftwareApplicationId;  //2-BYTES
    char                    vl_ocStbHostSoftwareApplicationSigStatus[VLMPEOS_SNMP_API_CHRMAX];
} VLMPEOS_SNMPHOSTSoftwareApplicationInfoTable_t;

typedef struct
{
  int JVMHeapSize;
  int JVMAvailHeap;
  int JVMLiveObjects;
  int JVMDeadObjects;
}VLMPEOS_SNMP_SJVMinfo_t;

rmf_Error vlMpeosSnmpManagerInit();

rmf_Error vlMpeosSetOCAPApplicationInfoTable(int nApps, unsigned char* data,int len);
rmf_Error vlMpeosSetocStbHostSwApplicationInfoTable(int nApps, VLMPEOS_SNMPHOSTSoftwareApplicationInfoTable_t * pAppInfolist);
rmf_Error vlMpeosSetocStbHostUserSettingsPreferedLanguage(char * pszHostUserSettingsPreferredLanguage);
rmf_Error vlMpeosSetocStbHostJvmInfo(VLMPEOS_SNMP_SJVMinfo_t * pJvmInfo);
rmf_Error vlMpeosSetocStbHostPatPmtTimeoutCount(unsigned long nPatTimeout,unsigned long nPmtTimeout);
rmf_Error vlMpeosSetocStbHostOobCarouselTimeoutCount(int nTimeoutCount);
rmf_Error vlMpeosSetocStbHostInbandCarouselTimeoutCount(int nTimeoutCount);
    
#ifdef __cplusplus
}
#endif

#endif // __VLMPEOSSNMPMANAGERIF_H__

