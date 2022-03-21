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



#ifndef __VL_SNMP_ECM_API_H__
#define __VL_SNMP_ECM_API_H__

#include "snmp_types.h"
#include "ip_types.h"

/**
 * @defgroup SNMP_ECM SNMP ECM Api
 * @ingroup SNMP_MGR
 * @ingroup SNMP_ECM
 * @{
 */
typedef enum _VL_SNMP_ECM_DS_MODULATION_TYPE
{
    VL_SNMP_ECM_DS_MODULATION_TYPE_UNKNOWN = 0,
    VL_SNMP_ECM_DS_MODULATION_TYPE_QAM_64  = 3,
    VL_SNMP_ECM_DS_MODULATION_TYPE_QAM_256 = 4,
}VL_SNMP_ECM_DS_MODULATION_TYPE;

typedef enum _VL_SNMP_ECM_US_MODULATION_TYPE
{
    VL_SNMP_ECM_US_MODULATION_TYPE_UNKNOWN  = 0,
    VL_SNMP_ECM_US_MODULATION_TYPE_TDMA     = 1,
    VL_SNMP_ECM_US_MODULATION_TYPE_ATDMA    = 2,
    VL_SNMP_ECM_US_MODULATION_TYPE_SCDMA    = 3,
}VL_SNMP_ECM_US_MODULATION_TYPE;

typedef enum _VL_SNMP_ECM_US_CHANNEL_TYPE
{
    VL_SNMP_ECM_US_CHANNEL_TYPE_UNKNOWN         = 0,
    VL_SNMP_ECM_US_CHANNEL_TYPE_TDMA            = 1,
    VL_SNMP_ECM_US_CHANNEL_TYPE_ATDMA           = 2,
    VL_SNMP_ECM_US_CHANNEL_TYPE_SCDMA           = 3,
    VL_SNMP_ECM_US_CHANNEL_TYPE_TDMAANDATDMA    = 4,
}VL_SNMP_ECM_US_CHANNEL_TYPE;

typedef enum _VL_SNMP_ECM_docsDevRole
{
    VL_SNMP_ECM_docsDevRole_none       = 0,
    VL_SNMP_ECM_docsDevRole_cm         = 1,
    VL_SNMP_ECM_docsDevRole_cmtsActive = 2,
    VL_SNMP_ECM_docsDevRole_cmtsBackup = 3,
    
}VL_SNMP_ECM_docsDevRole;

typedef enum _VL_SNMP_ECM_docsDevSTPControl
{
    VL_SNMP_ECM_docsDevSTPControl_none              = 0,
    VL_SNMP_ECM_docsDevSTPControl_stEnabled         = 1,
    VL_SNMP_ECM_docsDevSTPControl_noStFilterBpdu     = 2,
    VL_SNMP_ECM_docsDevSTPControl_noStPassBpdu         = 3,
    
}VL_SNMP_ECM_docsDevSTPControl;

typedef enum _VL_SNMP_ECM_docsDevIgmpModeControl
{
    VL_SNMP_ECM_docsDevIgmpModeControl_none     = 0,
    VL_SNMP_ECM_docsDevIgmpModeControl_passive  = 1,
    VL_SNMP_ECM_docsDevIgmpModeControl_active     = 2,
    
}VL_SNMP_ECM_docsDevIgmpModeControl;

typedef enum _VL_SNMP_ECM_docsDevSwAdminStatus
{
    VL_SNMP_ECM_docsDevSwAdminStatus_none                        = 0,
    VL_SNMP_ECM_docsDevSwAdminStatus_upgradeFromMgt             = 1,
    VL_SNMP_ECM_docsDevSwAdminStatus_allowProvisioningUpgrade     = 2,
    VL_SNMP_ECM_docsDevSwAdminStatus_ignoreProvisioningUpgrade     = 3,
    
}VL_SNMP_ECM_docsDevSwAdminStatus;

typedef enum _VL_SNMP_ECM_docsDevSwOperStatus
{
    VL_SNMP_ECM_docsDevSwOperStatus_none                        = 0,
    VL_SNMP_ECM_docsDevSwOperStatus_inProgress                     = 1,
    VL_SNMP_ECM_docsDevSwOperStatus_completeFromProvisioning     = 2,
    VL_SNMP_ECM_docsDevSwOperStatus_completeFromMgt             = 3,
    VL_SNMP_ECM_docsDevSwOperStatus_failed                         = 4,
    VL_SNMP_ECM_docsDevSwOperStatus_other                         = 5,
    
}VL_SNMP_ECM_docsDevSwOperStatus;

typedef enum _VL_SNMP_ECM_docsDevSwServerTransportProtocol
{
    VL_SNMP_ECM_docsDevSwServerTransportProtocol_none = 0,
    VL_SNMP_ECM_docsDevSwServerTransportProtocol_tftp = 1,
    VL_SNMP_ECM_docsDevSwServerTransportProtocol_http = 2,
    
}VL_SNMP_ECM_docsDevSwServerTransportProtocol;

typedef enum _VL_SNMP_ECM_docsDevServerBootState
{
    VL_SNMP_ECM_docsDevServerBootState_none                     = 0,
    VL_SNMP_ECM_docsDevServerBootState_operational                  = 1,
    VL_SNMP_ECM_docsDevServerBootState_disabled                  = 2,
    VL_SNMP_ECM_docsDevServerBootState_waitingForDhcpOffer          = 3,
    VL_SNMP_ECM_docsDevServerBootState_waitingForDhcpResponse     = 4,
    VL_SNMP_ECM_docsDevServerBootState_waitingForTimeServer     = 5,
    VL_SNMP_ECM_docsDevServerBootState_waitingForTftp              = 6,
    VL_SNMP_ECM_docsDevServerBootState_refusedByCmts              = 7,
    VL_SNMP_ECM_docsDevServerBootState_forwardingDenied          = 8,
    VL_SNMP_ECM_docsDevServerBootState_other                      = 9,
    VL_SNMP_ECM_docsDevServerBootState_unknown                      = 10,
    
}VL_SNMP_ECM_docsDevServerBootState;

#define VL_DOCS_DEV_DATE_TIME_SIZE  16

typedef struct _VL_SNMP_DOCS_DEV_INFO
{
    VL_SNMP_ECM_docsDevRole                         docsDevRole;
    char                                             docsDevSerialNumber[VL_SNMP_STR_SIZE];
    VL_SNMP_ECM_docsDevSTPControl                     docsDevSTPControl;
    VL_SNMP_ECM_docsDevIgmpModeControl                 docsDevIgmpModeControl;
    int                                             docsDevMaxCpe;
    unsigned char                                     docsDevSwServer[VL_IPV6_ADDR_SIZE];
    char                                             docsDevSwFilename[VL_SNMP_STR_SIZE];
    VL_SNMP_ECM_docsDevSwAdminStatus                 docsDevSwAdminStatus;
    VL_SNMP_ECM_docsDevSwOperStatus                 docsDevSwOperStatus;
    char                                             docsDevSwCurrentVers[VL_SNMP_STR_SIZE];
    VL_XCHAN_IP_ADDRESS_TYPE                         docsDevSwServerAddressType;
    unsigned char                                     docsDevSwServerAddress[VL_IPV6_ADDR_SIZE];
    VL_SNMP_ECM_docsDevSwServerTransportProtocol     docsDevSwServerTransportProtocol;
    VL_SNMP_ECM_docsDevServerBootState                 docsDevServerBootState;
    unsigned char                                     docsDevServerDhcp[VL_IPV6_ADDR_SIZE];
    unsigned char                                     docsDevServerTime[VL_IPV6_ADDR_SIZE];
    unsigned char                                     docsDevServerTftp[VL_IPV6_ADDR_SIZE];
    char                                             docsDevServerConfigFile[VL_SNMP_STR_SIZE];
    VL_XCHAN_IP_ADDRESS_TYPE                         docsDevServerDhcpAddressType;
    unsigned char                                     docsDevServerDhcpAddress[VL_IPV6_ADDR_SIZE];
    VL_XCHAN_IP_ADDRESS_TYPE                         docsDevServerTimeAddressType;
    unsigned char                                     docsDevServerTimeAddress[VL_IPV6_ADDR_SIZE];
    VL_XCHAN_IP_ADDRESS_TYPE                         docsDevServerConfigTftpAddressType;
    unsigned char                                     docsDevServerConfigTftpAddress[VL_IPV6_ADDR_SIZE];
    unsigned char                                     docsDevDateTime[VL_DOCS_DEV_DATE_TIME_SIZE];

}VL_SNMP_DOCS_DEV_INFO;

#ifdef __cplusplus
extern "C" {
#endif
    
VL_SNMP_API_RESULT vlSnmpEcmGetIpAddress(VL_IP_ADDRESS_STRUCT * pIpAddress);
VL_SNMP_API_RESULT vlSnmpEcmGetMacAddress(VL_MAC_ADDRESS_STRUCT * pMacAddress);

VL_SNMP_API_RESULT vlSnmpEcmGetDsFrequency(unsigned long * pValue);
VL_SNMP_API_RESULT vlSnmpEcmGetDsPower(unsigned long * pValue);
VL_SNMP_API_RESULT vlSnmpEcmGetDsModulationType(VL_SNMP_ECM_DS_MODULATION_TYPE * peValue);
VL_SNMP_API_RESULT vlSnmpEcmGetDsLockStatus(unsigned long * pValue);
VL_SNMP_API_RESULT vlSnmpEcmGetUsChannelType(VL_SNMP_ECM_US_CHANNEL_TYPE * peValue);
VL_SNMP_API_RESULT vlSnmpEcmGetUsModulationType(VL_SNMP_ECM_US_MODULATION_TYPE * peValue);
VL_SNMP_API_RESULT vlSnmpEcmGetUsFrequency(unsigned long * pValue);
VL_SNMP_API_RESULT vlSnmpEcmGetUsPower(unsigned long * pValue);

VL_SNMP_API_RESULT vlSnmpEcmGetSignalNoizeRatio(unsigned long * pValue);

VL_SNMP_API_RESULT vlSnmpEcmGetIfStats(VL_HOST_IP_STATS * pIpStats);

VL_SNMP_API_RESULT vlSnmpEcmDocsDevInfo(VL_SNMP_DOCS_DEV_INFO * pStruct);

#ifdef __cplusplus
}
#endif
/**
 @}
 */
#endif // __VL_SNMP_ECM_API_H__
