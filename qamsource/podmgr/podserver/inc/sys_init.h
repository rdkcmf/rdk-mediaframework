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



#ifndef __VLMPEOSINIT_H__
#define __VLMPEOSINIT_H__

#include "rmf_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VL_PLATFORM_NAME_MAX_SIZE 50
#define PACE_PARKER "PACE_PARKER_INTELCE" //Pace parker
#define PACE_PARKER_DSA "PACE_PARKER_INTELCE_DSA" //Pace parker
#define INTEL_SR "INTEL_SR_INTELCE"       //Intel Shark River
#define BCM_NEXUS "BCM_NEXUS_LINUX_74XX"
typedef enum _VL_PLATFORM_NAME_VAL
{
  VL_OS_INTEL_SHARKRIVER = 0x1001,
  VL_OS_PACE_PARKER      = 0x1002,
  VL_OS_PACE_PARKER_DSA  = 0x1003,
  VL_OS_BCM_NEXUS 	 = 0x1004
}VL_PLATFORM_NAME_VAL;


rmf_Error vlMpeosSysInit();
rmf_Error vlMpeosPopulateDhcpOptions();
int	vlg_util_get_platform_name_val(void);
char * vlg_util_get_platform_name(void);
int vl_util_is_parker_device(void);
int vl_util_does_not_have_oob_tuner(void);
int vlhal_oobtuner_Capable(void);

#ifdef INTEL_PR18 
int vl_util_is_parker_device(void);
char * vlg_util_get_ecm_rpc_if_name(void);
char * vlg_util_get_docsis_wan_if_name(void);
char * vlg_util_get_docsis_dhcp_if_name(void);
#else
int vl_util_is_bcm_device(void);
#endif

void vl_InitSystemTimeTask(void);
unsigned char vl_isFeatureEnabled(unsigned char *featureString);
int vl_GetEnvValue(unsigned char *featureString);
float vl_GetEnvValue_float(unsigned char *featureString);
#ifdef __cplusplus
}
#endif

#endif // __VLMPEOSINIT_H__
