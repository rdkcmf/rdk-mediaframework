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


#ifndef __VL_SNMP_HOST_INFO_H__
#define __VL_SNMP_HOST_INFO_H__
#include "snmp_types.h"
#include "SnmpIORM.h"
/*-----------------------------------------------------------------*/
#define VL_MAX_SNMP_STR_SIZE    256
#define VL_MAX_SNMP_IOD_LENGTH  128
typedef enum _VL_HOST_SNMP_API_RESULT
{
    VL_HOST_SNMP_API_RESULT_SUCCESS,
    VL_HOST_SNMP_API_RESULT_FAILED,
    VL_HOST_SNMP_API_RESULT_NOT_IMPLEMENTED,
    VL_HOST_SNMP_API_RESULT_NULL_PARAM,
    VL_HOST_SNMP_API_RESULT_INVALID_PARAM,
    VL_HOST_SNMP_API_RESULT_OUT_OF_RANGE,
    VL_HOST_SNMP_API_RESULT_MALLOC_FAILED,

} VL_HOST_SNMP_API_RESULT;

typedef enum _VL_SNMP_VAR_TYPE
{
     VL_SNMP_VAR_TYPE_ocStbHostSerialNumber                             ,
     VL_SNMP_VAR_TYPE_ocStbHostCapabilities                             ,
     VL_SNMP_VAR_TYPE_ocStbEasMessageStateInfo                          ,
     VL_SNMP_VAR_TYPE_ocStbHostDeviceSoftware                           ,
     VL_SNMP_VAR_TYPE_ocStbHostPowerInfo                                ,
     VL_SNMP_VAR_TYPE_ocStbHostUserSettingsPreferedLanguage             ,
     VL_SNMP_VAR_TYPE_OcstbHostRebootInfo                               ,
     VL_SNMP_VAR_TYPE_OcstbHostRebootInfo_rebootResetinfo               ,
     VL_SNMP_VAR_TYPE_CableSystemInfo                                   ,

} VL_SNMP_VAR_TYPE;

typedef enum _VL_SNMP_HOST_CAPABILITIES
{
    VL_SNMP_HOST_CAPABILITIES_OTHER     = 1,
    VL_SNMP_HOST_CAPABILITIES_OCHD2     = 2,
    VL_SNMP_HOST_CAPABILITIES_EMBEDDED  = 3,
    VL_SNMP_HOST_CAPABILITIES_DCAS      = 4,
    VL_SNMP_HOST_CAPABILITIES_OCHD21    = 5,
    VL_SNMP_HOST_CAPABILITIES_BOCR      = 6,
    VL_SNMP_HOST_CAPABILITIES_OCHD21TC  = 7,

} VL_SNMP_HOST_CAPABILITIES;
/*
typedef enum _VL_SNMP_HOST_POWER_STATUS
{
    VL_SNMP_HOST_POWER_STATUS_POWERON = 1,
    VL_SNMP_HOST_POWER_STATUS_STANDBY = 2,

} VL_SNMP_HOST_POWER_STATUS;
*/
typedef enum _VL_SNMP_HOST_AC_OUTLET_STATUS
{
    VL_SNMP_HOST_AC_OUTLET_STATUS_UNSWITCHED    = 1,
    VL_SNMP_HOST_AC_OUTLET_STATUS_SWITCHEDON    = 2,
    VL_SNMP_HOST_AC_OUTLET_STATUS_SWITCHEDOFF   = 3,
    VL_SNMP_HOST_AC_OUTLET_STATUS_NOTINSTALLED  = 4,

} VL_SNMP_HOST_AC_OUTLET_STATUS;

typedef enum _VL_SNMP_HOST_REBOOT_INFO
{
    VL_SNMP_HOST_REBOOT_INFO_unknown            = 0,
    VL_SNMP_HOST_REBOOT_INFO_davicDocsis        = 1,
    VL_SNMP_HOST_REBOOT_INFO_user               = 2,
    VL_SNMP_HOST_REBOOT_INFO_system             = 3,
    VL_SNMP_HOST_REBOOT_INFO_trap               = 4,
    VL_SNMP_HOST_REBOOT_INFO_silentWatchdog     = 5,
    VL_SNMP_HOST_REBOOT_INFO_bootloader         = 6 ,
    VL_SNMP_HOST_REBOOT_INFO_powerup            = 7,
    VL_SNMP_HOST_REBOOT_INFO_hostUpgrade        = 8,
    VL_SNMP_HOST_REBOOT_INFO_hardware           = 9,
    VL_SNMP_HOST_REBOOT_INFO_cablecardError     = 10,

} VL_SNMP_HOST_REBOOT_INFO;

typedef enum _VL_SNMP_HOST_REBOOT_REQUEST
{
    VL_SNMP_HOST_REBOOT_REQUEST_do_not_reboot   = 0,
    VL_SNMP_HOST_REBOOT_REQUEST_reboot          = 1,

} VL_SNMP_HOST_REBOOT_REQUEST;

typedef struct _VL_SNMP_VAR_SERIAL_NUMBER
{
    char            szSerialNumber[VL_MAX_SNMP_STR_SIZE];
} VL_SNMP_VAR_SERIAL_NUMBER;

typedef struct _VL_SNMP_VAR_HOST_CAPABILITIES
{
    VL_SNMP_HOST_CAPABILITIES   HostCapabilities_val;
} VL_SNMP_VAR_HOST_CAPABILITIES;

typedef struct _VL_SNMP_VAR_EAS_STATE_CONFIG
{
    unsigned int    EMStateCodel;
    unsigned int    EMCountyCode;
    unsigned int    EMCSubdivisionCodel;
} VL_SNMP_VAR_EAS_STATE_CONFIG;

typedef struct _VL_SNMP_VAR_DEVICE_SOFTWARE
{
    char            SoftwareFirmwareVersion[VL_MAX_SNMP_STR_SIZE];
    char            SoftwareFirmwareOCAPVersionl[VL_MAX_SNMP_STR_SIZE];
    char            SoftwareFirmwareImageName[VL_MAX_SNMP_STR_SIZE];
    int             nSoftwareFirmwareDay;
    int             nSoftwareFirmwareMonth;
    int             nSoftwareFirmwareYear;
} VL_SNMP_VAR_DEVICE_SOFTWARE;

typedef struct _VL_SNMP_VAR_HOST_POWER_STATUS
{
    VL_SNMP_HOST_POWER_STATUS       ocStbHostPowerStatus;
    VL_SNMP_HOST_AC_OUTLET_STATUS   ocStbHostAcOutletStatus;
} VL_SNMP_VAR_HOST_POWER_STATUS;

typedef struct _VL_SNMP_VAR_USER_PREFERRED_LANGUAGE
{
    char            UserSettingsPreferedLanguage[VL_MAX_SNMP_STR_SIZE];
} VL_SNMP_VAR_USER_PREFERRED_LANGUAGE;

typedef struct _VL_SNMP_VAR_REBOOT_INFO
{
    VL_SNMP_HOST_REBOOT_INFO    HostRebootInfo;
    VL_SNMP_HOST_REBOOT_REQUEST rebootResetinfo;
} VL_SNMP_VAR_REBOOT_INFO;

/** SYSTEM DETAILS*/
typedef enum _VL_SNMP_SYSTEM_VAR_TYPE
{
     VL_SNMP_VAR_TYPE_sysDescr                      ,
     VL_SNMP_VAR_TYPE_sysObjectID                   ,
     VL_SNMP_VAR_TYPE_sysUpTime                     ,
     VL_SNMP_VAR_TYPE_sysContact                    ,
     VL_SNMP_VAR_TYPE_sysName                       ,
     VL_SNMP_VAR_TYPE_sysLocation                   ,
     VL_SNMP_VAR_TYPE_sysServices                   ,
     VL_SNMP_VAR_TYPE_sysORLastChange               ,

} VL_SNMP_SYSTEM_VAR_TYPE;

typedef struct _VL_SYSTEMINFO{

    char gsysdecvar[VL_MAX_SNMP_STR_SIZE];
    apioid gsysObjid[VL_MAX_SNMP_IOD_LENGTH];
    int objidlenght;
    int gsysUpTime;
    char gsysContact[VL_MAX_SNMP_STR_SIZE];
    char gsysName[VL_MAX_SNMP_STR_SIZE];
    char gsysLocation[VL_MAX_SNMP_STR_SIZE];
    int gsysServices;
    int gsysORLastChange;
} VL_SYSTEMINFO_t;

typedef struct _VL_SNMP_Cable_System_Info
{
    char szCaSystemId[VL_MAX_SNMP_STR_SIZE];
    char szCpSystemId[VL_MAX_SNMP_STR_SIZE];

}VL_SNMP_Cable_System_Info;

/*-----------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

VL_HOST_SNMP_API_RESULT vlSnmpGetHostInfo(VL_SNMP_VAR_TYPE eSnmpVarType, void * _pvSNMPInfo);
VL_HOST_SNMP_API_RESULT vlSnmpSetHostInfo(VL_SNMP_VAR_TYPE eSnmpVarType, const void * _pvSNMPInfo);
VL_HOST_SNMP_API_RESULT vl_SnmpHostSysteminfo(VL_SNMP_SYSTEM_VAR_TYPE systype, void *_Systeminfo);
VL_HOST_SNMP_API_RESULT vlSnmpRebootHost(VL_SNMP_HOST_REBOOT_INFO eRebootReason);
    
#ifdef __cplusplus
}
#endif

#endif // __VL_SNMP_HOST_INFO_H__
