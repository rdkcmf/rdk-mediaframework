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


#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <fstream>

#include "ocStbHostMibModule.h"
#include "docsDevEventTable_enums.h"
#include "docsDevDateTime.h"
#include "docsDevEvControl.h"
#include "docsDevEvControlTable.h"
#include "docsDevEventTable.h"
#include "ocStbHostSpecificationsInfo.h"
#include "snmpCommunityTable.h"
#include "snmpTargetAddrExtTable.h"
#include "snmpTargetAddrTable.h"
//#include "vacmAccessTable.h"
//#include "vacmSecurityToGroupTable.h"
//#include "vacmViewTreeFamilyTable.h"
#include "ocStbHostDumpTrapInfo.h" // for traps
#include "ocStbPanicDumpTrap.h"    //for traps path
#include "snmpTargetParamsTable.h"
#include "snmpNotifyTable.h"
#include "vl_ocStbHost_GetData.h"
#include "host_scalar.h" // hrStroage Memeroy size
#include "host.h" // for hrStorageTable,hrDeviceTable,hrSWR.etc..tables
#include "sysORTable.h"
#include "ocStbHostContentErrorSummaryInfo.h" //New MIB items
#include "cardMibAccessProxyServer.h"
#include "system.h"
#include "rdk_debug.h"
#include "tablevividlogicmib.h" //M-Card add by MANU
#include "xcaliburClient.h" // for proxymib
#include "stbHostUtils.h" // for stb-host-utils
#include "ipcclient_impl.h"
#include "vlEnv.h"

#define HOST_SNMP_MAX_SIZE 18


extern void  init_host(void); //some how host.h was not including
extern u_long  gsysORUpTime_OcSTB_hostMib;
extern u_long  gsysORUpTime_DocsDevieMib;
extern u_long  gsysORUpTime_HostResourcesMib;
extern u_long  gsysORUpTime_ipNetToPhysicalTableMib;
extern u_long  gsysORUpTime_ifTableMib;
extern u_long  gsysORUpTime_SNMPv2Mib;
struct timeval  OR_uptime;
extern Status_t gcardsnmpAccCtl;
/** Initializes the ocStbHostMibModule module */

#define REBOOT_ON_IP_MODE_CHANGE_FLAG   "/opt/reboot_on_ip_mode_change"

/*
 * enums for all column Status "REBOOT_ON_IP_MODE_CHANGE" value
 */

 typedef enum {
    REBOOT_ON_IP_MODE_CHANGE_IGNORE =  1,
    REBOOT_ON_IP_MODE_CHANGE_IMMEDIATE = 2
 } REBOOT_ON_IP_MODE_CHANGE_t;

bool snmp_touch_file(const char * pszFile)
{
    if(NULL != pszFile)
    {
        FILE * fp = fopen(pszFile, "w");
        if(NULL != fp)
        {
            fclose(fp);
            return true;
        }
    }
    
    return false;
}

bool snmp_remove_file(const char * pszFile)
{
    return (0 == remove(pszFile));
}

bool snmp_file_exists(const char * pszFile)
{
    return (0 == access(pszFile, F_OK));
}

void
init_ocStbHostMibModule(void)
{

//#if 1  // Scalar OID intilization are don here
   /* we intilization all Scalar Objects here by using the  net-snmp API netsnmp_register_scalar */

    static oid      ocStbHostSerialNumber_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 1, 1 };
    static oid      ocStbHostHostID_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 1, 2 };
    static oid      ocStbHostCapabilities_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 1, 3 };
    static oid      ocStbHostAvcSupport_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 1, 4 };
    static oid      ocStbHostQpskFDCFreq_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 8, 1 };
    static oid      ocStbHostQpskRDCFreq_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 8, 2 };
    static oid      ocStbHostQpskFDCBer_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 8, 3 };
    static oid      ocStbHostQpskFDCStatus_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 8, 4 };
    static oid      ocStbHostQpskFDCBytesRead_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 8, 5 };
    static oid      ocStbHostQpskFDCPower_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 8, 6 };
    static oid      ocStbHostQpskFDCLockedTime_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 8, 7 };
    static oid      ocStbHostQpskFDCSNR_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 8, 8 };
    static oid      ocStbHostQpskAGC_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 8, 9 };
    static oid      ocStbHostQpskRDCPower_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 8, 10 };
    static oid      ocStbHostQpskRDCDataRate_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 2, 8, 11 };
    static oid      ocStbEasMessageStateCode_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 3, 1, 1 };
    static oid      ocStbEasMessageCountyCode_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 3, 1, 2 };
    static oid      ocStbEasMessageCountySubdivisionCode_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 1, 3, 1, 3 };
    static oid      ocStbHostSecurityIdentifier_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 2, 2 };
    static oid      ocStbHostCASystemIdentifier_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 2, 3 };
    static oid      ocStbHostCAType_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 2, 4 };
    static oid      ocStbHostSoftwareFirmwareVersion_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 3, 1, 1 };
    static oid      ocStbHostSoftwareOCAPVersion_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 3, 1, 2 };
    static oid      ocStbHostSoftwareFirmwareReleaseDate_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 3, 1, 3 };
    static oid ocStbHostSoftwareApplicationInfoSigLastReadStatus_oid[] = { 1, 3 , 6, 1, 4, 1, 4491, 2, 3, 1, 1, 3, 3, 3 };
    static oid ocStbHostSoftwareApplicationInfoSigLastReceivedTime_oid[] = { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 3, 3, 2 };
    static oid      ocStbHostFirmwareImageStatus_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 3, 2, 1 };
    static oid      ocStbHostFirmwareCodeDownloadStatus_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 3, 2, 2 };
    static oid      ocStbHostFirmwareCodeObjectName_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 3, 2, 3 };
    static oid      ocStbHostFirmwareDownloadFailedStatus_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 3, 2, 4 };
    static oid      ocStbHostFirmwareDownloadFailedCount_oid[] = 
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 3, 2, 5 };
    static oid      ocStbHostPowerStatus_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 1, 1 };
    static oid      ocStbHostAcOutletStatus_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 1, 2 };
    static oid      ocStbHostUserSettingsPreferedLanguage_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 2, 1 };
    static oid      ocStbHostCardMacAddress_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 4, 1 };
    static oid      ocStbHostCardIpAddressType_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 4, 2 };
    static oid      ocStbHostCardIpAddress_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 4, 3 };
    static oid      ocStbHostCardMfgId_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 4, 6, 1 };
    static oid      ocStbHostCardVersion_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 4, 6, 2 };
    static oid      ocStbHostCardRootOid_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 4, 6, 3 };
    static oid      ocStbHostCardSerialNumber_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 4, 6, 4 };
    static oid ocStbHostCardSnmpAccessControl_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 4, 6, 5 };
    static oid      ocStbHostCardId_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 4, 7 };
    static oid      ocStbHostCardBindingStatus_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 4, 8 };
    static oid      ocStbHostCardOpenedGenericResource_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 4, 9 };
    static oid      ocStbHostCardTimeZoneOffset_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 4, 10 };
    static oid      ocStbHostCardDaylightSavingsTimeDelta_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 4, 11 };
    static oid      ocStbHostCardDaylightSavingsTimeEntry_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 4, 12 };
    static oid      ocStbHostCardDaylightSavingsTimeExit_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 4, 13 };
    static oid      ocStbHostCardEaLocationCode_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 4, 14 };
    static oid      ocStbHostCardVctId_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 4, 15 };
    static oid      ocStbHostCardCpAuthKeyStatus_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 4, 16, 1 };
    static oid      ocStbHostCardCpCertificateCheck_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 4, 16, 2 };
    static oid      ocStbHostCardCpCciChallengeCount_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 4, 16, 3 };
    static oid      ocStbHostCardCpKeyGenerationReqCount_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 4, 16, 4 };
    static oid      ocStbHostCardCpIdList_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 4, 16, 5 };
    static oid      ocStbHostIpAddressType_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 5, 1 };
    static oid      ocStbHostIpSubNetMask_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 5, 2 };
    static oid      ocStbHostOobMessageMode_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 5, 3 };
    static oid      ocStbHostBootStatus_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 5, 6 };
    static oid      ocStbHostRebootType_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 6, 1 };
    static oid      ocStbHostRebootReset_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 6, 2 };
    static oid      ocStbHostRebootIpModeTlvChange_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 6, 3 };
    static oid      ocStbHostLargestAvailableBlock_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 7, 1 };
    static oid      ocStbHostTotalVideoMemory_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 7, 2 };
    static oid      ocStbHostAvailableVideoMemory_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 7, 3 };
    static oid      ocStbHostTotalSystemMemory_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 7, 4 };
    static oid      ocStbHostAvailableSystemMemory_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 7, 5 };
    static oid      ocStbHostJVMHeapSize_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 8, 1 };
    static oid      ocStbHostJVMAvailHeap_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 8, 2 };
    static oid      ocStbHostJVMLiveObjects_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 8, 3 };
    static oid      ocStbHostJVMDeadObjects_oid[] =
        { 1, 3, 6, 1, 4, 1, 4491, 2, 3, 1, 1, 4, 8, 4 };
 

    DEBUGMSGTL(("ocStbHostMibModule", "Initializing\n"));

    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostSerialNumber",
                             handle_ocStbHostSerialNumber,
                             ocStbHostSerialNumber_oid,
                             OID_LENGTH(ocStbHostSerialNumber_oid),
                             HANDLER_CAN_RONLY));

    gsysORUpTime_OcSTB_hostMib = get_uptime();

    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostHostID", handle_ocStbHostHostID,
                             ocStbHostHostID_oid,
                             OID_LENGTH(ocStbHostHostID_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostCapabilities",
                             handle_ocStbHostCapabilities,
                             ocStbHostCapabilities_oid,
                             OID_LENGTH(ocStbHostCapabilities_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostAvcSupport",
                             handle_ocStbHostAvcSupport,
                             ocStbHostAvcSupport_oid,
                             OID_LENGTH(ocStbHostAvcSupport_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostQpskFDCFreq",
                             handle_ocStbHostQpskFDCFreq,
                             ocStbHostQpskFDCFreq_oid,
                             OID_LENGTH(ocStbHostQpskFDCFreq_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostQpskRDCFreq",
                             handle_ocStbHostQpskRDCFreq,
                             ocStbHostQpskRDCFreq_oid,
                             OID_LENGTH(ocStbHostQpskRDCFreq_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostQpskFDCBer",
                             handle_ocStbHostQpskFDCBer,
                             ocStbHostQpskFDCBer_oid,
                             OID_LENGTH(ocStbHostQpskFDCBer_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostQpskFDCStatus",
                             handle_ocStbHostQpskFDCStatus,
                             ocStbHostQpskFDCStatus_oid,
                             OID_LENGTH(ocStbHostQpskFDCStatus_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostQpskFDCBytesRead",
                             handle_ocStbHostQpskFDCBytesRead,
                             ocStbHostQpskFDCBytesRead_oid,
                             OID_LENGTH(ocStbHostQpskFDCBytesRead_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostQpskFDCPower",
                             handle_ocStbHostQpskFDCPower,
                             ocStbHostQpskFDCPower_oid,
                             OID_LENGTH(ocStbHostQpskFDCPower_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostQpskFDCLockedTime",
                             handle_ocStbHostQpskFDCLockedTime,
                             ocStbHostQpskFDCLockedTime_oid,
                             OID_LENGTH(ocStbHostQpskFDCLockedTime_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostQpskFDCSNR",
                             handle_ocStbHostQpskFDCSNR,
                             ocStbHostQpskFDCSNR_oid,
                             OID_LENGTH(ocStbHostQpskFDCSNR_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostQpskAGC", handle_ocStbHostQpskAGC,
                             ocStbHostQpskAGC_oid,
                             OID_LENGTH(ocStbHostQpskAGC_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostQpskRDCPower",
                             handle_ocStbHostQpskRDCPower,
                             ocStbHostQpskRDCPower_oid,
                             OID_LENGTH(ocStbHostQpskRDCPower_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostQpskRDCDataRate",
                             handle_ocStbHostQpskRDCDataRate,
                             ocStbHostQpskRDCDataRate_oid,
                             OID_LENGTH(ocStbHostQpskRDCDataRate_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbEasMessageStateCode",
                             handle_ocStbEasMessageStateCode,
                             ocStbEasMessageStateCode_oid,
                             OID_LENGTH(ocStbEasMessageStateCode_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbEasMessageCountyCode",
                             handle_ocStbEasMessageCountyCode,
                             ocStbEasMessageCountyCode_oid,
                             OID_LENGTH(ocStbEasMessageCountyCode_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbEasMessageCountySubdivisionCode",
                             handle_ocStbEasMessageCountySubdivisionCode,
                             ocStbEasMessageCountySubdivisionCode_oid,
                             OID_LENGTH
                             (ocStbEasMessageCountySubdivisionCode_oid),
                             HANDLER_CAN_RONLY));
/* Deprecated
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostSecurityIdentifier",
                             handle_ocStbHostSecurityIdentifier,
                             ocStbHostSecurityIdentifier_oid,
                             OID_LENGTH(ocStbHostSecurityIdentifier_oid),
                             HANDLER_CAN_RONLY));
*/
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostCASystemIdentifier",
                             handle_ocStbHostCASystemIdentifier,
                             ocStbHostCASystemIdentifier_oid,
                             OID_LENGTH(ocStbHostCASystemIdentifier_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostCAType", handle_ocStbHostCAType,
                             ocStbHostCAType_oid,
                             OID_LENGTH(ocStbHostCAType_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostSoftwareFirmwareVersion",
                             handle_ocStbHostSoftwareFirmwareVersion,
                             ocStbHostSoftwareFirmwareVersion_oid,
                             OID_LENGTH
                             (ocStbHostSoftwareFirmwareVersion_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostSoftwareOCAPVersion",
                             handle_ocStbHostSoftwareOCAPVersion,
                             ocStbHostSoftwareOCAPVersion_oid,
                             OID_LENGTH(ocStbHostSoftwareOCAPVersion_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostSoftwareFirmwareReleaseDate",
                             handle_ocStbHostSoftwareFirmwareReleaseDate,
                             ocStbHostSoftwareFirmwareReleaseDate_oid,
                             OID_LENGTH
                             (ocStbHostSoftwareFirmwareReleaseDate_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostSoftwareApplicationInfoSigLastReceivedTime",      
                             handle_ocStbHostSoftwareApplicationInfoSigLastReceivedTime,
                             ocStbHostSoftwareApplicationInfoSigLastReceivedTime_oid,
                             OID_LENGTH
                             (ocStbHostSoftwareApplicationInfoSigLastReceivedTime_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostSoftwareApplicationInfoSigLastReadStatus",
                             handle_ocStbHostSoftwareApplicationInfoSigLastReadStatus,
                             ocStbHostSoftwareApplicationInfoSigLastReadStatus_oid,
                             OID_LENGTH
                             (ocStbHostSoftwareApplicationInfoSigLastReadStatus_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostFirmwareImageStatus",
                             handle_ocStbHostFirmwareImageStatus,
                             ocStbHostFirmwareImageStatus_oid,
                             OID_LENGTH(ocStbHostFirmwareImageStatus_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostFirmwareCodeDownloadStatus",
                             handle_ocStbHostFirmwareCodeDownloadStatus,
                             ocStbHostFirmwareCodeDownloadStatus_oid,
                             OID_LENGTH
                             (ocStbHostFirmwareCodeDownloadStatus_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostFirmwareCodeObjectName",
                             handle_ocStbHostFirmwareCodeObjectName,
                             ocStbHostFirmwareCodeObjectName_oid,
                             OID_LENGTH
                             (ocStbHostFirmwareCodeObjectName_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostFirmwareDownloadFailedStatus",
                             handle_ocStbHostFirmwareDownloadFailedStatus,
                             ocStbHostFirmwareDownloadFailedStatus_oid,
                             OID_LENGTH
                             (ocStbHostFirmwareDownloadFailedStatus_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostFirmwareDownloadFailedCount",
                             handle_ocStbHostFirmwareDownloadFailedCount,
                             ocStbHostFirmwareDownloadFailedCount_oid, OID_LENGTH(ocStbHostFirmwareDownloadFailedCount_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostPowerStatus",
                             handle_ocStbHostPowerStatus,
                             ocStbHostPowerStatus_oid,
                             OID_LENGTH(ocStbHostPowerStatus_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostAcOutletStatus",
                             handle_ocStbHostAcOutletStatus,
                             ocStbHostAcOutletStatus_oid,
                             OID_LENGTH(ocStbHostAcOutletStatus_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostUserSettingsPreferedLanguage",
                             handle_ocStbHostUserSettingsPreferedLanguage,
                             ocStbHostUserSettingsPreferedLanguage_oid,
                             OID_LENGTH
                             (ocStbHostUserSettingsPreferedLanguage_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostCardMacAddress",
                             handle_ocStbHostCardMacAddress,
                             ocStbHostCardMacAddress_oid,
                             OID_LENGTH(ocStbHostCardMacAddress_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostCardIpAddressType",
                             handle_ocStbHostCardIpAddressType,
                             ocStbHostCardIpAddressType_oid,
                             OID_LENGTH(ocStbHostCardIpAddressType_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostCardIpAddress",
                             handle_ocStbHostCardIpAddress,
                             ocStbHostCardIpAddress_oid,
                             OID_LENGTH(ocStbHostCardIpAddress_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostCardOpenedGenericResource",
                             handle_ocStbHostCardOpenedGenericResource,
                             ocStbHostCardOpenedGenericResource_oid,
                             OID_LENGTH
                             (ocStbHostCardOpenedGenericResource_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostCardTimeZoneOffset",
                             handle_ocStbHostCardTimeZoneOffset,
                             ocStbHostCardTimeZoneOffset_oid,
                             OID_LENGTH(ocStbHostCardTimeZoneOffset_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostCardDaylightSavingsTimeDelta",
                             handle_ocStbHostCardDaylightSavingsTimeDelta,
                             ocStbHostCardDaylightSavingsTimeDelta_oid,
                             OID_LENGTH
                             (ocStbHostCardDaylightSavingsTimeDelta_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostCardDaylightSavingsTimeEntry",
                             handle_ocStbHostCardDaylightSavingsTimeEntry,
                             ocStbHostCardDaylightSavingsTimeEntry_oid,
                             OID_LENGTH
                             (ocStbHostCardDaylightSavingsTimeEntry_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostCardDaylightSavingsTimeExit",
                             handle_ocStbHostCardDaylightSavingsTimeExit,
                             ocStbHostCardDaylightSavingsTimeExit_oid,
                             OID_LENGTH
                             (ocStbHostCardDaylightSavingsTimeExit_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostCardEaLocationCode",
                             handle_ocStbHostCardEaLocationCode,
                             ocStbHostCardEaLocationCode_oid,
                             OID_LENGTH(ocStbHostCardEaLocationCode_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostCardVctId",
                             handle_ocStbHostCardVctId,
                             ocStbHostCardVctId_oid,
                             OID_LENGTH(ocStbHostCardVctId_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostCardCpAuthKeyStatus",
                             handle_ocStbHostCardCpAuthKeyStatus,
                             ocStbHostCardCpAuthKeyStatus_oid,
                             OID_LENGTH(ocStbHostCardCpAuthKeyStatus_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostCardCpCertificateCheck",
                             handle_ocStbHostCardCpCertificateCheck,
                             ocStbHostCardCpCertificateCheck_oid,
                             OID_LENGTH
                             (ocStbHostCardCpCertificateCheck_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostCardCpCciChallengeCount",
                             handle_ocStbHostCardCpCciChallengeCount,
                             ocStbHostCardCpCciChallengeCount_oid,
                             OID_LENGTH
                             (ocStbHostCardCpCciChallengeCount_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostCardCpKeyGenerationReqCount",
                             handle_ocStbHostCardCpKeyGenerationReqCount,
                             ocStbHostCardCpKeyGenerationReqCount_oid,
                             OID_LENGTH
                             (ocStbHostCardCpKeyGenerationReqCount_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostCardCpIdList",
                             handle_ocStbHostCardCpIdList,
                             ocStbHostCardCpIdList_oid,
                             OID_LENGTH(ocStbHostCardCpIdList_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostIpAddressType",
                             handle_ocStbHostIpAddressType,
                             ocStbHostIpAddressType_oid,
                             OID_LENGTH(ocStbHostIpAddressType_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostIpSubNetMask",
                             handle_ocStbHostIpSubNetMask,
                             ocStbHostIpSubNetMask_oid,
                             OID_LENGTH(ocStbHostIpSubNetMask_oid),
                             HANDLER_CAN_RONLY));
   netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostBootStatus",
                             handle_ocStbHostBootStatus,
                             ocStbHostBootStatus_oid,
                             OID_LENGTH(ocStbHostBootStatus_oid),
                             HANDLER_CAN_RONLY));

    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostRebootType",
                             handle_ocStbHostRebootType,
                             ocStbHostRebootType_oid,
                             OID_LENGTH(ocStbHostRebootType_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostRebootReset",
                             handle_ocStbHostRebootReset,
                             ocStbHostRebootReset_oid,
                             OID_LENGTH(ocStbHostRebootReset_oid),
                             HANDLER_CAN_RWRITE));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostRebootIpModeTlvChange",
                             handle_ocStbHostRebootIpModeTlvChange,
                             ocStbHostRebootIpModeTlvChange_oid,
                             OID_LENGTH(ocStbHostRebootIpModeTlvChange_oid),
                             HANDLER_CAN_RWRITE));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostLargestAvailableBlock",
                             handle_ocStbHostLargestAvailableBlock,
                             ocStbHostLargestAvailableBlock_oid,
                             OID_LENGTH
                             (ocStbHostLargestAvailableBlock_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostTotalVideoMemory",
                             handle_ocStbHostTotalVideoMemory,
                             ocStbHostTotalVideoMemory_oid,
                             OID_LENGTH(ocStbHostTotalVideoMemory_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostAvailableVideoMemory",
                             handle_ocStbHostAvailableVideoMemory,
                             ocStbHostAvailableVideoMemory_oid,
                             OID_LENGTH(ocStbHostAvailableVideoMemory_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostTotalSystemMemory",
                             handle_ocStbHostTotalSystemMemory,
                             ocStbHostTotalSystemMemory_oid,
                             OID_LENGTH(ocStbHostTotalSystemMemory_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostAvailableSystemMemory",
                             handle_ocStbHostAvailableSystemMemory,
                             ocStbHostAvailableSystemMemory_oid,
                             OID_LENGTH(ocStbHostAvailableSystemMemory_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostJVMHeapSize",
                             handle_ocStbHostJVMHeapSize,
                             ocStbHostJVMHeapSize_oid,
                             OID_LENGTH(ocStbHostJVMHeapSize_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostJVMAvailHeap",
                             handle_ocStbHostJVMAvailHeap,
                             ocStbHostJVMAvailHeap_oid,
                             OID_LENGTH(ocStbHostJVMAvailHeap_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostJVMLiveObjects",
                             handle_ocStbHostJVMLiveObjects,
                             ocStbHostJVMLiveObjects_oid,
                             OID_LENGTH(ocStbHostJVMLiveObjects_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostJVMDeadObjects",
                             handle_ocStbHostJVMDeadObjects,
                             ocStbHostJVMDeadObjects_oid,
                             OID_LENGTH(ocStbHostJVMDeadObjects_oid),
                             HANDLER_CAN_RONLY));
     /*ocStbHostCardBindingStatus module */
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostCardBindingStatus",
                             handle_ocStbHostCardBindingStatus,
                             ocStbHostCardBindingStatus_oid,
                             OID_LENGTH(ocStbHostCardBindingStatus_oid),
                             HANDLER_CAN_RONLY));
     netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostCardId", handle_ocStbHostCardId,
                             ocStbHostCardId_oid,
                             OID_LENGTH(ocStbHostCardId_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration                    ("ocStbHostCardMfgId",
                             handle_ocStbHostCardMfgId,
                             ocStbHostCardMfgId_oid,
                             OID_LENGTH(ocStbHostCardMfgId_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostCardRootOid",
                             handle_ocStbHostCardRootOid,
                             ocStbHostCardRootOid_oid,
                             OID_LENGTH(ocStbHostCardRootOid_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostCardSerialNumber",
                             handle_ocStbHostCardSerialNumber,
                             ocStbHostCardSerialNumber_oid,
                             OID_LENGTH(ocStbHostCardSerialNumber_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostCardSnmpAccessControl", handle_ocStbHostCardSnmpAccessControl,
                            ocStbHostCardSnmpAccessControl_oid, OID_LENGTH(ocStbHostCardSnmpAccessControl_oid),
                            HANDLER_CAN_RWRITE));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostCardVersion",
                             handle_ocStbHostCardVersion,
                             ocStbHostCardVersion_oid,
                             OID_LENGTH(ocStbHostCardVersion_oid),
                             HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration
                            ("ocStbHostOobMessageMode",
                             handle_ocStbHostOobMessageMode,
                             ocStbHostOobMessageMode_oid,
                             OID_LENGTH(ocStbHostOobMessageMode_oid),
                             HANDLER_CAN_RONLY));
    init_system();   //FOR SYSTEM mib
    gsysORUpTime_SNMPv2Mib = get_uptime();
    init_ocStbHostSpecificationsInfo();//nitializes the ocStbHostSpecificationsInfo to Main

    init_host_scalar();
    init_ocStbHostDumpTrapInfo();
    send_ocStbPanicDumpTrap_trap();
    init_ocStbHostContentErrorSummaryInfo();
//#endif // scalar

   /**
     * here we initialize all the tables we're planning on supporting
   */


    initialize_table_ocStbHostAVInterfaceTable();
    initialize_table_ocStbHostIEEE1394Table();
    initialize_table_ocStbHostIEEE1394ConnectedDevicesTable();
    initialize_table_ocStbHostInBandTunerTable();
    initialize_table_ocStbHostDVIHDMITable();
    initialize_table_ocStbHostDVIHDMIAvailableVideoFormatTable();

    initialize_table_ocStbHostComponentVideoTable();
//    initialize_table_ocStbHostRFChannelOutTable();
//    initialize_table_ocStbHostAnalogVideoTable();
    initialize_table_ocStbHostProgramStatusTable();
    initialize_table_ocStbHostMpeg2ContentTable();
    initialize_table_ocStbHostMpeg4ContentTable();
    initialize_table_ocStbHostVc1ContentTable();
    initialize_table_ocStbHostSPDIfTable();

    initialize_table_ocStbHostCCAppInfoTable();
   // initialize_table_ocStbHostSoftwareApplicationInfoTable();
    initialize_table_ocStbHostSystemTempTable();
    initialize_table_ocStbHostSystemMemoryReportTable();
    initialize_table_ifTable();
    gsysORUpTime_ifTableMib = get_uptime();
    initialize_table_ipNetToPhysicalTable();
    gsysORUpTime_ipNetToPhysicalTableMib = get_uptime();
    //vividloigc certification Table
    ///removed to avoid carsh, no more use
   // init_vividlogicmib(); 
    initialize_table_McardcertificationTable();

    /*DocesEvent Table*/

    init_docsDevDateTime();
    init_docsDevEvControl();
    init_docsDevEvControlTable();
    init_docsDevEventTable();
    gsysORUpTime_DocsDevieMib = get_uptime();
    /* For hrStorage,Device,Pricess SWRumTable's..  */
    init_host();
    gsysORUpTime_HostResourcesMib = get_uptime();
    /*access controal tables*/
    init_snmpCommunityTable();
    init_snmpTargetAddrExtTable();
    init_snmpTargetAddrTable();
    initialize_table_snmpTargetParamsTable();
    initialize_table_snmpNotifyTable();

    init_sysORTable();

	// initialize the PROXYMIB
	init_xcaliburClient();
	init_stbHostUtils();


    /*form snmpd.configure file the below table get the displayed*/
    //init_vacmSecurityToGroupTable();
    //init_vacmAccessTable();
    //init_vacmViewTreeFamilyTable();

}





  char SerialNumber[255];
  char snmpHostID[HOST_SNMP_MAX_SIZE];
  HostCapabilities_snmpt  SHostCapabilities;
  Status_t htAvcSupport;
  EasInfo_t easObjectsdata;
  //memset(&easObjectsdata, 0, sizeof(EasInfo_t));
  HostCATInfo_t HostCATobj;
  //memset(&HostCATobj, 0, sizeof(HostCATInfo_t));
  HostPowerInfo_t HostPowerobj;
  //memset(&HostPowerobj, 0, sizeof(HostPowerInfo_t));
  char UserSettingsPreferedLanguage[255]; // audio streams indicated as the 3-Octet */
  //memset(&UserSettingsPreferedLanguage, 0, 255);
  CardInfo_t HostCardobj;
  //memset(&HostCardobj, 0, sizeof(CardInfo_t));
  ocStbHostQpsk_t QpskObj;
   //memset(&QpskObj, 0, sizeof(ocStbHostQpsk_t));
  DeviceSInfo_t DevSwObj;
     //memset(&DevSwObj, 0, sizeof(DeviceSInfo_t));
  FDownloadStatus_t FrimDLObj;
  //memset(&FrimDLObj, 0, sizeof(FDownloadStatus_t));

  SNMPocStbHostSoftwareApplicationInfoTable_t Swappsigtimestatus;
  SOcstbHOSTInfo_t HostIpinfoObj;
//   memset(&HostIpinfoObj, 0,sizeof(SOcstbHOSTInfo_t));

  SOcstbHostRebootInfo_t reboottypedObj;
//memset(&reboottypedObj, 0,sizeof(SOcstbHostRebootInfo_t));
 SMemoryInfo_t StorageMemoryObj;
//memset(&StorageMemoryObj, 0, sizeof(SMemoryInfo_t));
 SJVMinfo_t  JVMstorageObj;
//memset(&JVMstorageObj, 0, sizeof(SJVMinfo_t));
 SocStbHostSnmpProxy_CardInfo_t gProxyCardInfo;
int
handle_ocStbHostSerialNumber(netsnmp_mib_handler *handler,
                             netsnmp_handler_registration *reginfo,
                             netsnmp_agent_request_info *reqinfo,
                             netsnmp_request_info *requests)
{

    switch (reqinfo->mode){

    case MODE_GET:
        /********************** VL CODE  start*********************/
        if(0 == Sget_ocStbHostSerialNumber(SerialNumber, sizeof(SerialNumber)))
        {
              SNMP_DEBUGPRINT("ERROR:vlget_ocStbHostSerialNumber");
              return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/
        snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                 (u_char *)SerialNumber
                                  ,strlen(SerialNumber));
        break;
    default:
        /*
         * we should never vlget here, so this is a really bad error
         */
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostSerialNumber\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}


int
handle_ocStbHostHostID(netsnmp_mib_handler *handler,
                       netsnmp_handler_registration *reginfo,
                       netsnmp_agent_request_info *reqinfo,
                       netsnmp_request_info *requests)
{
    return SNMP_ERR_NOERROR;
}

int
handle_ocStbHostCapabilities(netsnmp_mib_handler *handler,
                             netsnmp_handler_registration *reginfo,
                             netsnmp_agent_request_info *reqinfo,
                             netsnmp_request_info *requests)
{
    switch (reqinfo->mode) {

    case MODE_GET:

    if(0 == Sget_ocStbHostCapabilities(&SHostCapabilities))
        {
              SNMP_DEBUGPRINT("ERROR:vlget_ocStbHostCapabilities");
              return SNMP_ERR_GENERR;
        }
        snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                                 (u_char *)&SHostCapabilities
                                 ,sizeof(SHostCapabilities));
        break;


    default:
        /*
         * we should never vlget here, so this is a really bad error
         */
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostCapabilities\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
handle_ocStbHostAvcSupport(netsnmp_mib_handler *handler,
                            netsnmp_handler_registration *reginfo,
                            netsnmp_agent_request_info   *reqinfo,
                            netsnmp_request_info         *requests)
{
    /* We are never called for a GETNEXT if it's registered as a
    "instance", as it's "magically" handled for us.  */

    /* a instance handler also only hands us one request at a time, so
    we don't need to loop over a list of requests; we'll only get one. */
    
    switch(reqinfo->mode) {

        case MODE_GET:
            if(0 == Sget_ocStbHostAvcSupport(&htAvcSupport))
            {
                SNMP_DEBUGPRINT("ERROR:vlget_ocStbHostCapabilities");
                return SNMP_ERR_GENERR;
            }
            snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                                     (u_char *) &htAvcSupport,
                                      sizeof(htAvcSupport));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
			"unknown mode (%d) in handle_ocStbHostAvcSupport\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}



int
handle_ocStbHostQpskFDCFreq(netsnmp_mib_handler *handler,
                            netsnmp_handler_registration *reginfo,
                            netsnmp_agent_request_info *reqinfo,
                            netsnmp_request_info *requests)
{

    switch (reqinfo->mode) {

    case MODE_GET:
        if(0 == Sget_ocStbHostQpskObjects(&QpskObj))
        {
              SNMP_DEBUGPRINT("ERROR:Sget_ocStbHostQpskObjects");
              return SNMP_ERR_GENERR;
        }

        snmp_set_var_typed_value(requests->requestvb, ASN_UNSIGNED,
                                 (u_char *)&QpskObj.vl_ocStbHostQpskFDCFreq
                                 ,sizeof(QpskObj.vl_ocStbHostQpskFDCFreq) );
        break;

    default:
        /*
         * we should never get here, so this is a really bad error
         */
    	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostQpskFDCFreq\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}





int
handle_ocStbHostQpskRDCFreq(netsnmp_mib_handler *handler,
                            netsnmp_handler_registration *reginfo,
                            netsnmp_agent_request_info *reqinfo,
                            netsnmp_request_info *requests)
{

    switch (reqinfo->mode) {

    case MODE_GET:
         if(0 == Sget_ocStbHostQpskObjects(&QpskObj))
        {
              SNMP_DEBUGPRINT("ERROR:Sget_ocStbHostQpskObjects");
              return SNMP_ERR_GENERR;
        }
        snmp_set_var_typed_value(requests->requestvb, ASN_UNSIGNED,
                                 (u_char *)&QpskObj.vl_ocStbHostQpskRDCFreq
                                 ,sizeof(QpskObj.vl_ocStbHostQpskRDCFreq));
        break;


    default:
        /*
         * we should never get here, so this is a really bad error
         */
    	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostQpskRDCFreq\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
handle_ocStbHostQpskFDCBer(netsnmp_mib_handler *handler,
                           netsnmp_handler_registration *reginfo,
                           netsnmp_agent_request_info *reqinfo,
                           netsnmp_request_info *requests)
{

    switch (reqinfo->mode) {

    case MODE_GET:
         if(0 == Sget_ocStbHostQpskObjects(&QpskObj))
        {
              SNMP_DEBUGPRINT("ERROR:Sget_ocStbHostQpskObjects");
              return SNMP_ERR_GENERR;
        }
        snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                                 (u_char *)&QpskObj.vl_ocStbHostQpskFDCBer,
                                 sizeof(QpskObj.vl_ocStbHostQpskFDCBer) );
        break;


    default:
        /*
         * we should never get here, so this is a really bad error
         */
    	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostQpskFDCBer\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
handle_ocStbHostQpskFDCStatus(netsnmp_mib_handler *handler,
                              netsnmp_handler_registration *reginfo,
                              netsnmp_agent_request_info *reqinfo,
                              netsnmp_request_info *requests)
{

    switch (reqinfo->mode) {

    case MODE_GET:
         if(0 == Sget_ocStbHostQpskObjects(&QpskObj))
        {
              SNMP_DEBUGPRINT("ERROR:Sget_ocStbHostQpskObjects");
              return SNMP_ERR_GENERR;
        }
        snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                                 (u_char *)&QpskObj.vl_ocStbHostQpskFDCStatus
                                 ,sizeof(QpskObj.vl_ocStbHostQpskFDCStatus)
                                  );
        break;


    default:
        /*
         * we should never get here, so this is a really bad error
         */
    	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostQpskFDCStatus\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}


int
handle_ocStbHostQpskFDCBytesRead(netsnmp_mib_handler *handler,
                                 netsnmp_handler_registration *reginfo,
                                 netsnmp_agent_request_info *reqinfo,
                                 netsnmp_request_info *requests)
{
    switch (reqinfo->mode) {

    case MODE_GET:
         if(0 == Sget_ocStbHostQpskObjects(&QpskObj))
        {
              SNMP_DEBUGPRINT("ERROR:Sget_ocStbHostQpskObjects");
              return SNMP_ERR_GENERR;
        }
        snmp_set_var_typed_value(requests->requestvb, ASN_UNSIGNED,
                                 (u_char *)&QpskObj.vl_ocStbHostQpskFDCBytesRead
                                 ,sizeof(QpskObj.vl_ocStbHostQpskFDCBytesRead)
                                  );
        break;


    default:
        /*
         * we should never get here, so this is a really bad error
         */
    	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostQpskFDCBytesRead\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
handle_ocStbHostQpskFDCPower(netsnmp_mib_handler *handler,
                             netsnmp_handler_registration *reginfo,
                             netsnmp_agent_request_info *reqinfo,
                             netsnmp_request_info *requests)
{


    switch (reqinfo->mode) {

    case MODE_GET:
         if(0 == Sget_ocStbHostQpskObjects(&QpskObj))
        {
              SNMP_DEBUGPRINT("ERROR:Sget_ocStbHostQpskObjects");
              return SNMP_ERR_GENERR;
        }
        snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                                 (u_char *)&QpskObj.vl_ocStbHostQpskFDCPower
                                 ,sizeof(QpskObj.vl_ocStbHostQpskFDCPower)
                                  );
        break;


    default:
        /*
         * we should never get here, so this is a really bad error
         */
    	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostQpskFDCPower\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
handle_ocStbHostQpskFDCLockedTime(netsnmp_mib_handler *handler,
                                  netsnmp_handler_registration *reginfo,
                                  netsnmp_agent_request_info *reqinfo,
                                  netsnmp_request_info *requests)
{


    switch (reqinfo->mode) {

    case MODE_GET:
      if(0 == Sget_ocStbHostQpskObjects(&QpskObj))
        {
              SNMP_DEBUGPRINT("ERROR:Sget_ocStbHostQpskObjects");
              return SNMP_ERR_GENERR;
        }
        snmp_set_var_typed_value(requests->requestvb, ASN_UNSIGNED,
                                 (u_char *)&QpskObj.vl_ocStbHostQpskFDCLockedTime
                                 ,sizeof(QpskObj.vl_ocStbHostQpskFDCLockedTime)
                                  );
        break;


    default:
        /*
         * we should never get here, so this is a really bad error
         */
    	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostQpskFDCLockedTime\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}


int
handle_ocStbHostQpskFDCSNR(netsnmp_mib_handler *handler,
                           netsnmp_handler_registration *reginfo,
                           netsnmp_agent_request_info *reqinfo,
                           netsnmp_request_info *requests)
{

    switch (reqinfo->mode) {

    case MODE_GET:
         if(0 == Sget_ocStbHostQpskObjects(&QpskObj))
        {
              SNMP_DEBUGPRINT("ERROR:Sget_ocStbHostQpskObjects");
              return SNMP_ERR_GENERR;
        }
        snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                                 (u_char *)&QpskObj.vl_ocStbHostQpskFDCSNR
                                 ,sizeof(QpskObj.vl_ocStbHostQpskFDCSNR)
                                  );
        break;


    default:
        /*
         * we should never get here, so this is a really bad error
         */
    	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostQpskFDCSNR\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
handle_ocStbHostQpskAGC(netsnmp_mib_handler *handler,
                        netsnmp_handler_registration *reginfo,
                        netsnmp_agent_request_info *reqinfo,
                        netsnmp_request_info *requests)
{

    switch (reqinfo->mode) {

    case MODE_GET:
         if(0 == Sget_ocStbHostQpskObjects(&QpskObj))
        {
              SNMP_DEBUGPRINT("ERROR:Sget_ocStbHostQpskObjects");
              return SNMP_ERR_GENERR;
        }
        snmp_set_var_typed_value(requests->requestvb, ASN_UNSIGNED,
                                 (u_char *)&QpskObj.vl_ocStbHostQpskAGC
                                 ,sizeof(QpskObj.vl_ocStbHostQpskAGC)
                                  );
        break;


    default:
        /*
         * we should never get here, so this is a really bad error
         */
    	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
        	"unknown mode (%d) in handle_ocStbHostQpskAGC\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
handle_ocStbHostQpskRDCPower(netsnmp_mib_handler *handler,
                             netsnmp_handler_registration *reginfo,
                             netsnmp_agent_request_info *reqinfo,
                             netsnmp_request_info *requests)
{

    switch (reqinfo->mode) {

    case MODE_GET:
         if(0 == Sget_ocStbHostQpskObjects(&QpskObj))
        {
              SNMP_DEBUGPRINT("ERROR:Sget_ocStbHostQpskObjects");
              return SNMP_ERR_GENERR;
        }
        snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                                 (u_char *)&QpskObj.vl_ocStbHostQpskRDCPower
                                 ,sizeof(QpskObj.vl_ocStbHostQpskRDCPower)
                                  );
        break;


    default:
        /*
         * we should never get here, so this is a really bad error
         */
    	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostQpskRDCPower\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
handle_ocStbHostQpskRDCDataRate(netsnmp_mib_handler *handler,
                                netsnmp_handler_registration *reginfo,
                                netsnmp_agent_request_info *reqinfo,
                                netsnmp_request_info *requests)
{

    switch (reqinfo->mode) {

    case MODE_GET:
         if(0 == Sget_ocStbHostQpskObjects(&QpskObj))
        {
              SNMP_DEBUGPRINT("ERROR:Sget_ocStbHostQpskObjects");
              return SNMP_ERR_GENERR;
        }
        snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                                 (u_char *)&QpskObj.vl_ocStbHostQpskRDCDataRate
                                 ,sizeof(QpskObj.vl_ocStbHostQpskRDCDataRate)
                                  );
        break;


    default:
        /*
         * we should never get here, so this is a really bad error
         */
    	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostQpskRDCDataRate\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
handle_ocStbEasMessageStateCode(netsnmp_mib_handler *handler,
                                netsnmp_handler_registration *reginfo,
                                netsnmp_agent_request_info *reqinfo,
                                netsnmp_request_info *requests)
{

    switch (reqinfo->mode) {

    case MODE_GET:
        if(0 == Sget_ocStbEasMessageStateInfo(&easObjectsdata))
        {
              SNMP_DEBUGPRINT("ERROR:SNMPget_ocStbEasMessageStateInfo");
              return SNMP_ERR_GENERR;
        }

        snmp_set_var_typed_value(requests->requestvb, ASN_UNSIGNED,
                                 (u_char *)&easObjectsdata.EMSCodel                                 ,sizeof(easObjectsdata.EMSCodel));
        break;


    default:
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbEasMessageStateCode\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
handle_ocStbEasMessageCountyCode(netsnmp_mib_handler *handler,
                                 netsnmp_handler_registration *reginfo,
                                 netsnmp_agent_request_info *reqinfo,
                                 netsnmp_request_info *requests)
{

    switch (reqinfo->mode) {

    case MODE_GET:
         if(0 == Sget_ocStbEasMessageStateInfo(&easObjectsdata))
        {
              SNMP_DEBUGPRINT("ERROR:SNMPget_ocStbEasMessageStateInfo");
              return SNMP_ERR_GENERR;
        }
        snmp_set_var_typed_value(requests->requestvb, ASN_UNSIGNED,
                                 (u_char *)&easObjectsdata.EMCCode
                                 /* XXX: a pointer to the scalar's data */
                                 ,sizeof(easObjectsdata.EMCCode)
                                 /*
                                  * XXX: the length of the data in bytes
                                  */ );
        break;


    default:
        /*
         * we should never get here, so this is a really bad error
         */
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbEasMessageCountyCode\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
handle_ocStbEasMessageCountySubdivisionCode(netsnmp_mib_handler *handler,
                                            netsnmp_handler_registration
                                            *reginfo,
                                            netsnmp_agent_request_info
                                            *reqinfo,
                                            netsnmp_request_info *requests)
{

    switch (reqinfo->mode) {

    case MODE_GET:
         if(0 == Sget_ocStbEasMessageStateInfo(&easObjectsdata))
        {
              SNMP_DEBUGPRINT("ERROR:SNMPget_ocStbEasMessageStateInfo");
              return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/
        snmp_set_var_typed_value(requests->requestvb, ASN_UNSIGNED,
                                 (u_char *)& easObjectsdata.EMCSCodel
                                 /* XXX: a pointer to the scalar's data */
                                 ,sizeof(easObjectsdata.EMCSCodel)
                                 /*
                                  * XXX: the length of the data in bytes
                                  */ );
        break;

    default:
        /*
        * we should never get here, so this is a really bad error
        */
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
            "unknown mode (%d) in handle_ocStbEasMessageCountySubdivisionCode\n",
            reqinfo->mode);
        return SNMP_ERR_GENERR;
     }

    return SNMP_ERR_NOERROR;
}



int
handle_ocStbHostSecurityIdentifier(netsnmp_mib_handler *handler,
                                   netsnmp_handler_registration *reginfo,
                                   netsnmp_agent_request_info *reqinfo,
                                   netsnmp_request_info *requests)
{

    switch (reqinfo->mode) {

    case MODE_GET:


        if(0 == Sget_ocStbHostSecurityIdentifier(&HostCATobj))
        {
              SNMP_DEBUGPRINT("ERROR:SNMPget_ocStbHostSecurityIdentifier");
              return SNMP_ERR_GENERR;
        }



        snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                 (u_char *)HostCATobj.S_SecurityID
                                 /* XXX: a pointer to the scalar's data */
                                 ,strlen((char*)HostCATobj.S_SecurityID)
                                 /*
                                  * XXX: the length of the data in bytes
                                  */ );
        break;


    default:
        /*
         * we should never get here, so this is a really bad error
         */
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostSecurityIdentifier\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
handle_ocStbHostCASystemIdentifier(netsnmp_mib_handler *handler,
                                   netsnmp_handler_registration *reginfo,
                                   netsnmp_agent_request_info *reqinfo,
                                   netsnmp_request_info *requests)
{

    switch (reqinfo->mode) {

    case MODE_GET:
/** vlget_ocStbHostCASystemIdentifier module will get the value of ocStbHostCASystemIdentifier_val */
        if(0 ==  Sget_ocStbHostSecurityIdentifier(&HostCATobj))
        {
              SNMP_DEBUGPRINT("ERROR:SNMPget_ocStbHostSecurityIdentifier");
              return SNMP_ERR_GENERR;
        }
        snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                 (u_char *)HostCATobj.S_CASystemID
                                 /* XXX: a pointer to the scalar's data */
                                 ,strlen(HostCATobj.S_CASystemID)
                                 /*
                                  * XXX: the length of the data in bytes
                                  */ );
        break;


    default:
        /*
         * we should never get here, so this is a really bad error
         */
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostCASystemIdentifier\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
handle_ocStbHostCAType(netsnmp_mib_handler *handler,
                       netsnmp_handler_registration *reginfo,
                       netsnmp_agent_request_info *reqinfo,
                       netsnmp_request_info *requests)
{

    switch (reqinfo->mode) {

    case MODE_GET:
        if(0 == Sget_ocStbHostSecurityIdentifier(&HostCATobj) )
        {
              SNMP_DEBUGPRINT("ERROR:SNMPget_ocStbHostSecurityIdentifier");
              //return SNMP_ERR_GENERR;
        }

        snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                                 (u_char *)&HostCATobj.S_HostCAtype
                                  ,sizeof(HostCATobj.S_HostCAtype)
                                 );
        break;


    default:
        /*
         * we should never get here, so this is a really bad error
         */
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
        	"unknown mode (%d) in handle_ocStbHostCAType\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}


int
handle_ocStbHostSoftwareFirmwareVersion(netsnmp_mib_handler *handler,
                                        netsnmp_handler_registration
                                        *reginfo,
                                        netsnmp_agent_request_info
                                        *reqinfo,
                                        netsnmp_request_info *requests)
{

    switch (reqinfo->mode) {

    case MODE_GET:
	{
        	string strImageName="";
        	string line;
        	string strToFind = "imagename:";
        	ifstream versionFile ("/version.txt");
        	if (versionFile.is_open())
        	{
        	        while ( getline (versionFile,line) )
        	        {
        	                //cout << line << '\n';
    	    			RDK_LOG(RDK_LOG_DEBUG,"LOG.RDK.SNMP", 
						"version.txt line=%s\n", line.c_str());
        	                std::size_t found = line.find(strToFind);
        	                if (found!=std::string::npos)
        	                {
        	                        strImageName = line.substr(strToFind.length());
    	    				RDK_LOG(RDK_LOG_DEBUG,"LOG.RDK.SNMP",
						"version.txt line=%s\n", strImageName.c_str());
        	                }
        	        }
        	        versionFile.close();
        	}
        	else
        	{
        		RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP","Unable to open version.txt file");
        	}
        	snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
               	                  (u_char *)(strImageName.c_str())
               	                  ,strImageName.length()
               	                   );
	}
#if 0
         if(0 == Sget_ocStbHostDeviceSoftware(&DevSwObj) )
        {
              SNMP_DEBUGPRINT("ERROR:Sget_ocStbHostDeviceSoftware");
              return SNMP_ERR_GENERR;
        }
        snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                 (u_char *)DevSwObj.SFirmwareVersion
                                 ,strlen(DevSwObj.SFirmwareVersion)
                                  );
#endif
        break;


    default:
        /*
         * we should never get here, so this is a really bad error
         */
	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostSoftwareFirmwareVersion\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
handle_ocStbHostSoftwareOCAPVersion(netsnmp_mib_handler *handler,
                                    netsnmp_handler_registration *reginfo,
                                    netsnmp_agent_request_info *reqinfo,
                                    netsnmp_request_info *requests)
{
    /*
     * We are never called for a GETNEXT if it's registered as a
     * "instance", as it's "magically" handled for us.
     */

    /*
     * a instance handler also only hands us one request at a time, so
     * we don't need to loop over a list of requests; we'll only get one.
     */

    switch (reqinfo->mode) {

    case MODE_GET:
         if(0 == Sget_ocStbHostDeviceSoftware(&DevSwObj) )
        {
              SNMP_DEBUGPRINT("ERROR:Sget_ocStbHostDeviceSoftware");
              return SNMP_ERR_GENERR;
        }
        snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                 (u_char *)DevSwObj.SFirmwareOCAPVersionl
                                 ,strlen(DevSwObj.SFirmwareOCAPVersionl)
                                  );
        break;


    default:
        /*
         * we should never get here, so this is a really bad error
         */
	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostSoftwareOCAPVersion\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
handle_ocStbHostSoftwareFirmwareReleaseDate(netsnmp_mib_handler *handler,
                                            netsnmp_handler_registration
                                            *reginfo,
                                            netsnmp_agent_request_info
                                            *reqinfo,
                                            netsnmp_request_info *requests)
{
    /*
     * We are never called for a GETNEXT if it's registered as a
     * "instance", as it's "magically" handled for us.
     */

    /*
     * a instance handler also only hands us one request at a time, so
     * we don't need to loop over a list of requests; we'll only get one.
     */

    switch (reqinfo->mode) {

    case MODE_GET:
        if(0 == Sget_ocStbHostDeviceSoftware(&DevSwObj) )
        {
            SNMP_DEBUGPRINT("ERROR:Sget_ocStbHostDeviceSoftware");
            return SNMP_ERR_GENERR;
        }
	/* 
	 ** The function Avinterface::vlGet_ocStbHostDeviceSoftware updates the 
	 ** sofware firmware release date to this DevSwObj.ocStbHostSoftwareFirmwareReleaseDate  
	*/
        snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                 (u_char *)DevSwObj.ocStbHostSoftwareFirmwareReleaseDate,
                                         DevSwObj.ocStbHostSoftwareFirmwareReleaseDate_len );
        break;


    default:
        /*
         * we should never get here, so this is a really bad error
         */
	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostSoftwareFirmwareReleaseDate\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
handle_ocStbHostFirmwareImageStatus(netsnmp_mib_handler *handler,
                                    netsnmp_handler_registration *reginfo,
                                    netsnmp_agent_request_info *reqinfo,
                                    netsnmp_request_info *requests)
{
    /*
     * We are never called for a GETNEXT if it's registered as a
     * "instance", as it's "magically" handled for us.
     */

    /*
     * a instance handler also only hands us one request at a time, so
     * we don't need to loop over a list of requests; we'll only get one.
     */

    switch (reqinfo->mode) {

    case MODE_GET:
        if(0 == Sget_ocStbHostFirmwareDownloadStatus(&FrimDLObj) )
        {
              SNMP_DEBUGPRINT("ERROR:Sget_ocStbHostFirmwareDownloadStatus");
              return SNMP_ERR_GENERR;
        }
        snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                                 (u_char *)&FrimDLObj.vl_FirmwareImageStatus
                                 ,sizeof(FrimDLObj.vl_FirmwareImageStatus)
                                  );
        break;


    default:
        /*
         * we should never get here, so this is a really bad error
         */
	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostFirmwareImageStatus\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
handle_ocStbHostFirmwareCodeDownloadStatus(netsnmp_mib_handler *handler,
                                           netsnmp_handler_registration
                                           *reginfo,
                                           netsnmp_agent_request_info
                                           *reqinfo,
                                           netsnmp_request_info *requests)
{
    /*
     * We are never called for a GETNEXT if it's registered as a
     * "instance", as it's "magically" handled for us.
     */

    /*
     * a instance handler also only hands us one request at a time, so
     * we don't need to loop over a list of requests; we'll only get one.
     */

    switch (reqinfo->mode) {

    case MODE_GET:
        if(0 == Sget_ocStbHostFirmwareDownloadStatus(&FrimDLObj))
        {
              SNMP_DEBUGPRINT("ERROR:Sget_ocStbHostFirmwareDownloadStatus");
              return SNMP_ERR_GENERR;
        }
        snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                                 (u_char *)&FrimDLObj.vl_FirmwareCodeDownloadStatus
                                 ,sizeof(FrimDLObj.vl_FirmwareCodeDownloadStatus)
                                  );
        break;


    default:
        /*
         * we should never get here, so this is a really bad error
         */
	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostFirmwareCodeDownloadStatus\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
handle_ocStbHostFirmwareCodeObjectName(netsnmp_mib_handler *handler,
                                       netsnmp_handler_registration
                                       *reginfo,
                                       netsnmp_agent_request_info *reqinfo,
                                       netsnmp_request_info *requests)
{
    /*
     * We are never called for a GETNEXT if it's registered as a
     * "instance", as it's "magically" handled for us.
     */

    /*
     * a instance handler also only hands us one request at a time, so
     * we don't need to loop over a list of requests; we'll only get one.
     */

    switch (reqinfo->mode) {

    case MODE_GET:
        if(0 == Sget_ocStbHostFirmwareDownloadStatus(&FrimDLObj))
        {
              SNMP_DEBUGPRINT("ERROR:Sget_ocStbHostFirmwareDownloadStatus");
              return SNMP_ERR_GENERR;
        }
        snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                 (u_char *)FrimDLObj.vl_FirmwareCodeObjectName
                                 ,strlen(FrimDLObj.vl_FirmwareCodeObjectName)
                                  );
        break;


    default:
        /*
         * we should never get here, so this is a really bad error
         */
	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostFirmwareCodeObjectName\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
handle_ocStbHostFirmwareDownloadFailedStatus(netsnmp_mib_handler *handler,
                                             netsnmp_handler_registration
                                             *reginfo,
                                             netsnmp_agent_request_info
                                             *reqinfo,
                                             netsnmp_request_info
                                             *requests)
{
    /*
     * We are never called for a GETNEXT if it's registered as a
     * "instance", as it's "magically" handled for us.
     */

    /*
     * a instance handler also only hands us one request at a time, so
     * we don't need to loop over a list of requests; we'll only get one.
     */

    switch (reqinfo->mode) {

    case MODE_GET:
        if(0 == Sget_ocStbHostFirmwareDownloadStatus(&FrimDLObj))
        {
              SNMP_DEBUGPRINT("ERROR:Sget_ocStbHostFirmwareDownloadStatus");
              return SNMP_ERR_GENERR;
        }
        snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                                 (u_char *)&FrimDLObj.vl_FrimwareFailedStatus
                                 ,sizeof(FrimDLObj.vl_FrimwareFailedStatus)
                                  );
        break;


    default:
        /*
         * we should never get here, so this is a really bad error
         */
	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostFirmwareDownloadFailedStatus\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
        handle_ocStbHostFirmwareDownloadFailedCount(netsnmp_mib_handler *handler,
        netsnmp_handler_registration *reginfo,
        netsnmp_agent_request_info   *reqinfo,
        netsnmp_request_info         *requests)
{
    /* We are never called for a GETNEXT if it's registered as a
    "instance", as it's "magically" handled for us.  */

    /* a instance handler also only hands us one request at a time, so
    we don't need to loop over a list of requests; we'll only get one. */
    switch(reqinfo->mode) {

        case MODE_GET:
            if(0 == Sget_ocStbHostFirmwareDownloadStatus(&FrimDLObj))
            {
                SNMP_DEBUGPRINT("ERROR:Sget_ocStbHostFirmwareDownloadStatus");
                return SNMP_ERR_GENERR;
            }
            snmp_set_var_typed_value(requests->requestvb, ASN_COUNTER,
                                     (u_char *)&FrimDLObj.vl_FirmwareDownloadFailedCount
                                     ,sizeof(FrimDLObj.vl_FirmwareDownloadFailedCount));
            break;


        default:
            /* we should never get here, so this is a really bad error */
	    RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
			"unknown mode (%d) in handle_ocStbHostFirmwareDownloadFailedCount\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
        handle_ocStbHostSoftwareApplicationInfoSigLastReadStatus(netsnmp_mib_handler *handler,
        netsnmp_handler_registration *reginfo,
        netsnmp_agent_request_info   *reqinfo,
        netsnmp_request_info         *requests)
{
    /* We are never called for a GETNEXT if it's registered as a
    "instance", as it's "magically" handled for us.  */

    /* a instance handler also only hands us one request at a time, so
    we don't need to loop over a list of requests; we'll only get one. */
    
    switch(reqinfo->mode) {

        case MODE_GET:
            if(0 == Sget_ocStbHostSoftwareApplicationSigstatus(&Swappsigtimestatus) )
            {
                SNMP_DEBUGPRINT("ERROR:Sget_ocStbHostDeviceSoftware");
                return SNMP_ERR_GENERR;
            }
            snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER, (u_char*)&Swappsigtimestatus.vl_ocStbHostSoftwareApplicationInfoSigLastReadStatus,sizeof(Swappsigtimestatus.vl_ocStbHostSoftwareApplicationInfoSigLastReadStatus));
            break;
        default:
            /* we should never get here, so this is a really bad error */
	    RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
            		"unknown mode (%d) in handle_ocStbHostSoftwareApplicationInfoSigLastReadStatus\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
        handle_ocStbHostSoftwareApplicationInfoSigLastReceivedTime(netsnmp_mib_handler *handler,
        netsnmp_handler_registration *reginfo,
        netsnmp_agent_request_info   *reqinfo,
        netsnmp_request_info         *requests)
{
    /* We are never called for a GETNEXT if it's registered as a
    "instance", as it's "magically" handled for us.  */

    /* a instance handler also only hands us one request at a time, so
    we don't need to loop over a list of requests; we'll only get one. */
    
    switch(reqinfo->mode) {

        case MODE_GET:
            if(0 == Sget_ocStbHostSoftwareApplicationSigstatus(&Swappsigtimestatus) )
            {
                SNMP_DEBUGPRINT("ERROR:Sget_ocStbHostDeviceSoftware");
                return SNMP_ERR_GENERR;
            }
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char*)Swappsigtimestatus.vl_ocStbHostSoftwareApplicationInfoSigLastReceivedTime                                          
                                             ,strlen(Swappsigtimestatus.vl_ocStbHostSoftwareApplicationInfoSigLastReceivedTime));
            break;
        default:
            /* we should never get here, so this is a really bad error */
	    RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
            		"unknown mode (%d) in handle_ocStbHostSoftwareApplicationInfoSigLastReceivedTime\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
handle_ocStbHostPowerStatus(netsnmp_mib_handler *handler,
                            netsnmp_handler_registration *reginfo,
                            netsnmp_agent_request_info *reqinfo,
                            netsnmp_request_info *requests)
{

    switch (reqinfo->mode) {

    case MODE_GET:

        if(0 == Sget_ocStbHostPowerInf(&HostPowerobj))
        {
              SNMP_DEBUGPRINT("ERROR:Sget_ocStbHostPowerInf");
              return SNMP_ERR_GENERR;
        }

        snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                                 (u_char *)&HostPowerobj.ocStbHostPowerStatus
                                 /* XXX: a pointer to the scalar's data */
                                 ,sizeof(HostPowerobj.ocStbHostPowerStatus)
                                 /*
                                  * XXX: the length of the data in bytes
                                  */ );
        break;


    default:
        /*
         * we should never get here, so this is a really bad error
         */
    	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostPowerStatus\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
handle_ocStbHostAcOutletStatus(netsnmp_mib_handler *handler,
                               netsnmp_handler_registration *reginfo,
                               netsnmp_agent_request_info *reqinfo,
                               netsnmp_request_info *requests)
{

    switch (reqinfo->mode) {

    case MODE_GET:
/** vlget_ocStbHostAcOutletStatus module will get the value of ocStbHostAcOutletStatus_val */
         if(0 == Sget_ocStbHostPowerInf(&HostPowerobj))
        {
              SNMP_DEBUGPRINT("ERROR:SNMPget_ocStbHostPowerInf");
              return SNMP_ERR_GENERR;
        }

        snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                                 (u_char *)&HostPowerobj.ocStbHostAcOutletStatus
                                 /* XXX: a pointer to the scalar's data */
                                 ,sizeof(HostPowerobj.ocStbHostAcOutletStatus)
                                 /*
                                  * XXX: the length of the data in bytes
                                  */ );
        break;


    default:
        /*
         * we should never get here, so this is a really bad error
         */
    	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostAcOutletStatus\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
handle_ocStbHostUserSettingsPreferedLanguage(netsnmp_mib_handler *handler,
                                             netsnmp_handler_registration
                                             *reginfo,
                                             netsnmp_agent_request_info
                                             *reqinfo,
                                             netsnmp_request_info
                                             *requests)
{


    switch (reqinfo->mode) {

    case MODE_GET:
        if(0 == Sget_ocStbHostUserSettingsPreferedLanguage(UserSettingsPreferedLanguage))
        {
              SNMP_DEBUGPRINT("ERROR:vlget_ocStbHostUserSettingsPreferedLanguage");
              return SNMP_ERR_GENERR;
        }

        snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                 (u_char *)UserSettingsPreferedLanguage
                                 /* XXX: a pointer to the scalar's data */
                                 ,strlen(UserSettingsPreferedLanguage)
                                 /*
                                  * XXX: the length of the data in bytes
                                  */ );
        break;


    default:
        /*
         * we should never get here, so this is a really bad error
         */
    	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostUserSettingsPreferedLanguage\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

// // name change as MAC address in new spec
int
handle_ocStbHostCardMacAddress(netsnmp_mib_handler *handler,
                               netsnmp_handler_registration *reginfo,
                               netsnmp_agent_request_info *reqinfo,
                               netsnmp_request_info *requests)
{

    switch (reqinfo->mode) {

    case MODE_GET:

        if(0 == Sget_ocStbHostCardInfo(&HostCardobj))
        {
              SNMP_DEBUGPRINT("ERROR:SNMPget_ocStbHostCardInfo");
              return SNMP_ERR_GENERR;
        }
     /********************** VL CODE  end*********************/
       // SNMP_DEBUGPRINT("\n HostCardobj.ocStbHostCardMACAddress = %s\n", HostCardobj.ocStbHostCardMACAddress);
       // SNMP_DEBUGPRINT("\n strlen(HostCardobj.ocStbHostCardMACAddress) = %d\n", strlen(HostCardobj.ocStbHostCardMACAddress));
        snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR/*ASN_PRIV_IMPLIED_OCTET_STR*/,
                                 /*(u_char *)*/(const u_char *)HostCardobj.ocStbHostCardMACAddress
                                 ,6*sizeof(char)/*strlen(HostCardobj.ocStbHostCardMACAddress*/);
        break;


    default:
        /*
         * we should never get here, so this is a really bad error
         */
    	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostCardMacAddress\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
handle_ocStbHostCardIpAddressType(netsnmp_mib_handler *handler,
                                  netsnmp_handler_registration *reginfo,
                                  netsnmp_agent_request_info *reqinfo,
                                  netsnmp_request_info *requests)
{
    switch (reqinfo->mode) {

    case MODE_GET:
        if(0 == Sget_ocStbHostCardInfo(&HostCardobj))
        {
              SNMP_DEBUGPRINT("ERROR:SNMPget_ocStbHostCardInfo");
              return SNMP_ERR_GENERR;
        }     /********************** VL CODE  end*********************/
        snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                                 (u_char *)&HostCardobj.ocStbHostCardIpAddressType
                                 /* XXX: a pointer to the scalar's data */
                                 ,sizeof(HostCardobj.ocStbHostCardIpAddressType)
                                 /*
                                  * XXX: the length of the data in bytes
                                  */ );
        break;


    default:
        /*
         * we should never get here, so this is a really bad error
         */
    	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostCardIpAddressType\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
handle_ocStbHostCardIpAddress(netsnmp_mib_handler *handler,
                              netsnmp_handler_registration *reginfo,
                              netsnmp_agent_request_info *reqinfo,
                              netsnmp_request_info *requests)
{

    switch (reqinfo->mode) {

    case MODE_GET:
        if(0 == Sget_ocStbHostCardInfo(&HostCardobj))
        {
              SNMP_DEBUGPRINT("ERROR:SNMPget_ocStbHostCardInfo");
              return SNMP_ERR_GENERR;
        }
        /********************** VL CODE  end*********************/

        snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                 /*(u_char *)*/ (const u_char *)HostCardobj.ocStbHostCardIpAddress
                                 /* XXX: a pointer to the scalar's data */
                                 ,4
                                 /*
                                  * XXX: the length of the data in bytes
                                  */ );
        break;


    default:
        /*
         * we should never get here, so this is a really bad error
         */
    	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostCardIpAddress\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
handle_ocStbHostIpAddressType(netsnmp_mib_handler *handler,
                              netsnmp_handler_registration *reginfo,
                              netsnmp_agent_request_info *reqinfo,
                              netsnmp_request_info *requests)
{
    /*
     * We are never called for a GETNEXT if it's registered as a
     * "instance", as it's "magically" handled for us.
     */

    /*
     * a instance handler also only hands us one request at a time, so
     * we don't need to loop over a list of requests; we'll only get one.
     */

    switch (reqinfo->mode) {

    case MODE_GET:
         if(0 == Sget_ocstbHostInfo(&HostIpinfoObj))
        {
              SNMP_DEBUGPRINT("ERROR:Sget_ocstbHostInfo");
              return SNMP_ERR_GENERR;
        }
        snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                                 (u_char *)&HostIpinfoObj.IPAddressType
                                 ,sizeof(HostIpinfoObj.IPAddressType));
        break;


    default:
        /*
         * we should never get here, so this is a really bad error
         */
    	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostIpAddressType\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}


int
handle_ocStbHostIpSubNetMask(netsnmp_mib_handler *handler,
                             netsnmp_handler_registration *reginfo,
                             netsnmp_agent_request_info *reqinfo,
                             netsnmp_request_info *requests)
{
    /*
     * We are never called for a GETNEXT if it's registered as a
     * "instance", as it's "magically" handled for us.
     */

    /*
     * a instance handler also only hands us one request at a time, so
     * we don't need to loop over a list of requests; we'll only get one.
     */

    switch (reqinfo->mode) {

    case MODE_GET:
         if(0 == Sget_ocstbHostInfo(&HostIpinfoObj))
        {
              SNMP_DEBUGPRINT("ERROR:Sget_ocstbHostInfo");
              return SNMP_ERR_GENERR;
        }
        {
            int nSize = 0;
            switch(HostIpinfoObj.IPAddressType)
            {
                case S_CARDIPADDRESSTYPE_UNKNOWN: nSize =  0; break;
                case S_CARDIPADDRESSTYPE_IPV4:    nSize =  4; break;
                case S_CARDIPADDRESSTYPE_IPV6:    nSize = 16; break;
                default:                          nSize =  0; break;
            }
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     /*(u_char *)*/(const u_char *)HostIpinfoObj.IPSubNetMask
                                     ,nSize);
        }
        break;


    default:
        /*
         * we should never get here, so this is a really bad error
         */
    	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostIpSubNetMask\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
handle_ocStbHostBootStatus(netsnmp_mib_handler *handler,
                           netsnmp_handler_registration *reginfo,
                           netsnmp_agent_request_info *reqinfo,
                           netsnmp_request_info *requests)
{
    /*
     * We are never called for a GETNEXT if it's registered as a
     * "instance", as it's "magically" handled for us.
     */

    /*
     * a instance handler also only hands us one request at a time, so
     * we don't need to loop over a list of requests; we'll only get one.
     */

    switch (reqinfo->mode) {

    case MODE_GET:
        if(0 == Sget_OcstbHostRebootInfo(&reboottypedObj))
        {
            SNMP_DEBUGPRINT("ERROR:Sget_OcstbHostRebootInfo");
            return SNMP_ERR_GENERR;
        }
        snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                                 (u_char *)&reboottypedObj.BootStatus
                                         ,sizeof(reboottypedObj.BootStatus)           );
        break;


    default:
        /*
         * we should never get here, so this is a really bad error
         */
    	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostBootStatus\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
handle_ocStbHostRebootType(netsnmp_mib_handler *handler,
                           netsnmp_handler_registration *reginfo,
                           netsnmp_agent_request_info *reqinfo,
                           netsnmp_request_info *requests)
{
    /*
     * We are never called for a GETNEXT if it's registered as a
     * "instance", as it's "magically" handled for us.
     */

    /*
     * a instance handler also only hands us one request at a time, so
     * we don't need to loop over a list of requests; we'll only get one.
     */

    switch (reqinfo->mode) {

    case MODE_GET:
          if(0 == Sget_OcstbHostRebootInfo(&reboottypedObj))
        {
              SNMP_DEBUGPRINT("ERROR:Sget_OcstbHostRebootInfo");
              return SNMP_ERR_GENERR;
        }
        snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                                 (u_char *)&reboottypedObj.rebootinfo
                                 ,sizeof(reboottypedObj.rebootinfo)
                                  );
        break;


    default:
        /*
         * we should never get here, so this is a really bad error
         */
    	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostRebootType\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
handle_ocStbHostRebootReset(netsnmp_mib_handler *handler,
                            netsnmp_handler_registration *reginfo,
                            netsnmp_agent_request_info *reqinfo,
                            netsnmp_request_info *requests)
{
    int             ret;
    /*
     * We are never called for a GETNEXT if it's registered as a
     * "instance", as it's "magically" handled for us.
     */

    /*
     * a instance handler also only hands us one request at a time, so
     * we don't need to loop over a list of requests; we'll only get one.
     */

    switch (reqinfo->mode) {

    case MODE_GET:
        if(0 == Sget_OcstbHostRebootInfo(&reboottypedObj))
        {
              SNMP_DEBUGPRINT("ERROR:Sget_OcstbHostRebootInfo");
              return SNMP_ERR_GENERR;
        }
        snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                                 (u_char *)&reboottypedObj.rebootResetinfo
                                ,sizeof(reboottypedObj.rebootResetinfo));
        break;
        /*
         * SET REQUEST
         *
         * multiple states in the transaction.  See:
         * http://www.net-snmp.org/tutorial-5/toolkit/mib_module/set-actions.jpg
         */
    case MODE_SET_RESERVE1:
        /*
         * or you could use netsnmp_check_vb_type_and_size instead
         */

        ret = netsnmp_check_vb_type(requests->requestvb, ASN_INTEGER);

        //if(reboottypedObj.rebootResetinfo == 1)
        //{

        //This step is to update the golbal variable to keep reference of updated value, Even though here it is not required we are modifying.
        if(*requests->requestvb->val.integer == STATUS_TRUE){

            reboottypedObj.rebootResetinfo = STATUS_TRUE;
           }
           else{

               reboottypedObj.rebootResetinfo = STATUS_FALSE;
           }
        //netsnmp_strdup_and_null(requests->requestvb->val.integer,
         //requests->requestvb->val_len));

         SNMP_DEBUGPRINT("\n 1::handle_ocStbHostRebootReset::requests->requestvb = %d\n",reboottypedObj.rebootResetinfo);
         if(reboottypedObj.rebootResetinfo == STATUS_TRUE)
         {
             SSet_OcstbHostRebootInfo();
             return SNMP_ERR_NOERROR;
         }
         else{}
         if(reboottypedObj.rebootResetinfo == STATUS_FALSE)
         {
             return SNMP_ERR_NOERROR;
         }
         else{}
         if (ret == SNMP_ERR_NOERROR) {
            netsnmp_set_request_error(reqinfo, requests, ret);
         }
        else{}
        break;

    case MODE_SET_RESERVE2:
        /*
         * XXX malloc "undo" storage buffer
         */
        //if ( /* XXX if malloc, or whatever, failed: */ ) {
          //  netsnmp_set_request_error(reqinfo, requests,
            //      SNMP_ERR_RESOURCEUNAVAILABLE);
        //}
        break;

    case MODE_SET_FREE:
        /*
         * XXX: free resources allocated in RESERVE1 and/or
         * RESERVE2.  Something failed somewhere, and the states
         * below won't be called.
         */
        break;

    case MODE_SET_ACTION:
        /*
         * XXX: perform the value change here
         */
         // reboottypedObj.rebootResetinfo
        //if (reboottypedObj.rebootResetinfo == 0 ) {
          //    //NO CHANGE
            // }
        if(*requests->requestvb->val.integer == STATUS_TRUE){

            reboottypedObj.rebootResetinfo = STATUS_TRUE;
        }
        else{

            reboottypedObj.rebootResetinfo = STATUS_FALSE;
        }

        SNMP_DEBUGPRINT("\n 2::handle_ocStbHostRebootReset::requests->requestvb = %d\n",reboottypedObj.rebootResetinfo);
        return SNMP_ERR_NOERROR;
#if 0
         if(reboottypedObj.rebootResetinfo == STATUS_TRUE)
         {
             SSet_OcstbHostRebootInfo();
             return SNMP_ERR_NOERROR;
         }

         if(reboottypedObj.rebootResetinfo == STATUS_FALSE){
               snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER, (u_char*)&reboottypedObj.rebootResetinfo
                                ,sizeof(reboottypedObj.rebootResetinfo));

         }
#endif
        //}

        break;

    case MODE_SET_COMMIT:
        /*
         * XXX: delete temporary storage
         */
//         if ( /* XXX: error? */ ) {
//             /*
//              * try _really_really_ hard to never get to this point
//              */
//             netsnmp_set_request_error(reqinfo, requests,
//                                       SNMP_ERR_COMMITFAILED);
//         }
        break;

    case MODE_SET_UNDO:
        /*
         * XXX: UNDO and return to previous value for the object
         */
//         if ( /* XXX: error? */ ) {
//             /*
//              * try _really_really_ hard to never get to this point
//              */
//             netsnmp_set_request_error(reqinfo, requests,
//                                       SNMP_ERR_UNDOFAILED);
//         }
        break;

    default:
        /*
         * we should never get here, so this is a really bad error
         */
    	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostRebootReset\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
handle_ocStbHostRebootIpModeTlvChange(netsnmp_mib_handler *handler,
                            netsnmp_handler_registration *reginfo,
                            netsnmp_agent_request_info *reqinfo,
                            netsnmp_request_info *requests)
{
    int             ret;
    /*
     * We are never called for a GETNEXT if it's registered as a
     * "instance", as it's "magically" handled for us.
     */

    /*
     * a instance handler also only hands us one request at a time, so
     * we don't need to loop over a list of requests; we'll only get one.
     */

    static REBOOT_ON_IP_MODE_CHANGE_t reboot_SET_value = REBOOT_ON_IP_MODE_CHANGE_IGNORE;
    REBOOT_ON_IP_MODE_CHANGE_t reboot_get_value = REBOOT_ON_IP_MODE_CHANGE_IGNORE; 
    
    switch (reqinfo->mode)
    {
        case MODE_GET:
        {
            if(0 == access(REBOOT_ON_IP_MODE_CHANGE_FLAG, F_OK))
            {
                  reboot_get_value = REBOOT_ON_IP_MODE_CHANGE_IMMEDIATE;
            }
            snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER, (u_char *)&reboot_get_value, sizeof(reboot_get_value));
        }
        break;
        
        /*
         * SET REQUEST
         *
         * multiple states in the transaction.  See:
         * http://www.net-snmp.org/tutorial-5/toolkit/mib_module/set-actions.jpg
         */
        case MODE_SET_RESERVE1:
        {
            /*
             * or you could use netsnmp_check_vb_type_and_size instead
             */

            ret = netsnmp_check_vb_type(requests->requestvb, ASN_INTEGER);

            //This step is to update the golbal variable to keep reference of updated value, Even though here it is not required we are modifying.
            if(*requests->requestvb->val.integer == REBOOT_ON_IP_MODE_CHANGE_IGNORE)
            {
                reboot_SET_value = REBOOT_ON_IP_MODE_CHANGE_IGNORE;
            }
            else if(*requests->requestvb->val.integer == REBOOT_ON_IP_MODE_CHANGE_IMMEDIATE)
            {
                reboot_SET_value = REBOOT_ON_IP_MODE_CHANGE_IMMEDIATE;
            }
            else
            {
                RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP", "handle_ocStbHostRebootIpModeTlvChange: invalid value %d in set reserve.\n", *requests->requestvb->val.integer);
                netsnmp_set_request_error(reqinfo, requests, SNMP_ERR_WRONGVALUE);
                return SNMP_ERR_WRONGVALUE;
            }
        }
        break;

        case MODE_SET_RESERVE2:
        {
            /*
             * XXX malloc "undo" storage buffer
             */
            //if ( /* XXX if malloc, or whatever, failed: */ ) {
              //  netsnmp_set_request_error(reqinfo, requests,
                //      SNMP_ERR_RESOURCEUNAVAILABLE);
            //}
        }
        break;

        case MODE_SET_FREE:
        {
            /*
             * XXX: free resources allocated in RESERVE1 and/or
             * RESERVE2.  Something failed somewhere, and the states
             * below won't be called.
             */
        }
        break;

        case MODE_SET_ACTION:
        {
            /*
             * XXX: perform the value change here
             */
            if(*requests->requestvb->val.integer == REBOOT_ON_IP_MODE_CHANGE_IGNORE)
            {
                reboot_SET_value = REBOOT_ON_IP_MODE_CHANGE_IGNORE;
            }
            else if(*requests->requestvb->val.integer == REBOOT_ON_IP_MODE_CHANGE_IMMEDIATE)
            {
                reboot_SET_value = REBOOT_ON_IP_MODE_CHANGE_IMMEDIATE;
            }
            else
            {
                RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP", "handle_ocStbHostRebootIpModeTlvChange: invalid value %d in set action.\n", *requests->requestvb->val.integer);
                netsnmp_set_request_error(reqinfo, requests, SNMP_ERR_WRONGVALUE);
                return SNMP_ERR_WRONGVALUE;
            }
            
            if(REBOOT_ON_IP_MODE_CHANGE_IGNORE == reboot_SET_value)
            {
                if(!snmp_remove_file(REBOOT_ON_IP_MODE_CHANGE_FLAG))
                {
                    // remove failed, so check if file still exists
                    if(0 == access(REBOOT_ON_IP_MODE_CHANGE_FLAG, F_OK))
                    {
                        // file still exists. so we have a problem.
                        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP", "handle_ocStbHostRebootIpModeTlvChange: unable to remove '%s'.\n", REBOOT_ON_IP_MODE_CHANGE_FLAG);
                        netsnmp_set_request_error(reqinfo, requests, SNMP_ERR_GENERR);
                        return SNMP_ERR_GENERR;
                    }
                }
            }
            else if(REBOOT_ON_IP_MODE_CHANGE_IMMEDIATE == reboot_SET_value)
            {
                if(!snmp_touch_file(REBOOT_ON_IP_MODE_CHANGE_FLAG))
                {
                    // touch failed, so check if file still exists
                    if(0 == access(REBOOT_ON_IP_MODE_CHANGE_FLAG, F_OK))
                    {
                        // file exists. so everything is fine.
                    }
                    else
                    {
                        // file does not exist. so we have a problem.
                        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP", "handle_ocStbHostRebootIpModeTlvChange: unable to touch '%s'.\n", REBOOT_ON_IP_MODE_CHANGE_FLAG);
                        netsnmp_set_request_error(reqinfo, requests, SNMP_ERR_GENERR);
                        return SNMP_ERR_GENERR;
                    }
                }
            }
        }
        break;

        case MODE_SET_COMMIT:
        {
            /*
             * XXX: delete temporary storage
             */
    //         if ( /* XXX: error? */ ) {
    //             /*
    //              * try _really_really_ hard to never get to this point
    //              */
    //             netsnmp_set_request_error(reqinfo, requests,
    //                                       SNMP_ERR_COMMITFAILED);
    //         }
        }
        break;

        case MODE_SET_UNDO:
        {
            /*
             * XXX: UNDO and return to previous value for the object
             */
            if(0 == access(REBOOT_ON_IP_MODE_CHANGE_FLAG, F_OK))
            {
                  reboot_get_value = REBOOT_ON_IP_MODE_CHANGE_IMMEDIATE;
            }
            reboot_SET_value = reboot_get_value;
        }
        break;

        default:
        {
            /*
             * we should never get here, so this is a really bad error
             */
            RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                     "unknown mode (%d) in handle_ocStbHostRebootIpModeTlvChange\n",
                     reqinfo->mode);
        }
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
handle_ocStbHostLargestAvailableBlock(netsnmp_mib_handler *handler,
                                      netsnmp_handler_registration
                                      *reginfo,
                                      netsnmp_agent_request_info *reqinfo,
                                      netsnmp_request_info *requests)
{
    /*
     * We are never called for a GETNEXT if it's registered as a
     * "instance", as it's "magically" handled for us.
     */

    /*
     * a instance handler also only hands us one request at a time, so
     * we don't need to loop over a list of requests; we'll only get one.
     */

    switch (reqinfo->mode) {

    case MODE_GET:
          if(0 == Sget_OcstbHostMemoryInfo(&StorageMemoryObj))
        {
              SNMP_DEBUGPRINT("ERROR:Sget_OcstbHostMemoryInfo");
              return SNMP_ERR_GENERR;
        }
        snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                                 (u_char *)&StorageMemoryObj.AvilabelBlock
                                 ,sizeof(StorageMemoryObj.AvilabelBlock)
                                 );
        break;


    default:
        /*
         * we should never get here, so this is a really bad error
         */
    	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostLargestAvailableBlock\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
handle_ocStbHostTotalVideoMemory(netsnmp_mib_handler *handler,
                                 netsnmp_handler_registration *reginfo,
                                 netsnmp_agent_request_info *reqinfo,
                                 netsnmp_request_info *requests)
{
    /*
     * We are never called for a GETNEXT if it's registered as a
     * "instance", as it's "magically" handled for us.
     */

    /*
     * a instance handler also only hands us one request at a time, so
     * we don't need to loop over a list of requests; we'll only get one.
     */

    switch (reqinfo->mode) {

    case MODE_GET:
        if(0 == Sget_OcstbHostMemoryInfo(&StorageMemoryObj))
        {
              SNMP_DEBUGPRINT("ERROR:Sget_OcstbHostMemoryInfo");
              return SNMP_ERR_GENERR;
        }
        snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                                 (u_char *)&StorageMemoryObj.videoMemory
                                 ,sizeof(StorageMemoryObj.videoMemory)
                                  );
        break;


    default:
        /*
         * we should never get here, so this is a really bad error
         */
    	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostTotalVideoMemory\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
handle_ocStbHostAvailableVideoMemory(netsnmp_mib_handler *handler,
                                     netsnmp_handler_registration *reginfo,
                                     netsnmp_agent_request_info *reqinfo,
                                     netsnmp_request_info *requests)
{
    /*
     * We are never called for a GETNEXT if it's registered as a
     * "instance", as it's "magically" handled for us.
     */

    /*
     * a instance handler also only hands us one request at a time, so
     * we don't need to loop over a list of requests; we'll only get one.
     */

    switch (reqinfo->mode) {

    case MODE_GET:
        if(0 == Sget_OcstbHostMemoryInfo(&StorageMemoryObj))
        {
              SNMP_DEBUGPRINT("ERROR:Sget_OcstbHostMemoryInfo");
              return SNMP_ERR_GENERR;
        }
        snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                                 (u_char *)&StorageMemoryObj.AvailableVideoMemory
                                  ,sizeof(StorageMemoryObj.AvailableVideoMemory)
                                  );
        break;


    default:
        /*
         * we should never get here, so this is a really bad error
         */
    	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostAvailableVideoMemory\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
handle_ocStbHostTotalSystemMemory(netsnmp_mib_handler *handler,
                                 netsnmp_handler_registration *reginfo,
                                 netsnmp_agent_request_info *reqinfo,
                                 netsnmp_request_info *requests)
{
    /*
     * We are never called for a GETNEXT if it's registered as a
     * "instance", as it's "magically" handled for us.
     */

    /*
     * a instance handler also only hands us one request at a time, so
     * we don't need to loop over a list of requests; we'll only get one.
     */

    switch (reqinfo->mode) {

    case MODE_GET:
        if(0 == Sget_OcstbHostMemoryInfo(&StorageMemoryObj))
        {
              SNMP_DEBUGPRINT("ERROR:Sget_OcstbHostMemoryInfo");
              return SNMP_ERR_GENERR;
        }
        snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                                 (u_char *)&StorageMemoryObj.systemMemory
                                 ,sizeof(StorageMemoryObj.systemMemory)
                                  );
        break;


    default:
        /*
         * we should never get here, so this is a really bad error
         */
    	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostTotalSystemMemory\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
handle_ocStbHostAvailableSystemMemory(netsnmp_mib_handler *handler,
                                     netsnmp_handler_registration *reginfo,
                                     netsnmp_agent_request_info *reqinfo,
                                     netsnmp_request_info *requests)
{
    /*
     * We are never called for a GETNEXT if it's registered as a
     * "instance", as it's "magically" handled for us.
     */

    /*
     * a instance handler also only hands us one request at a time, so
     * we don't need to loop over a list of requests; we'll only get one.
     */

    switch (reqinfo->mode) {

    case MODE_GET:
        if(0 == Sget_OcstbHostMemoryInfo(&StorageMemoryObj))
        {
              SNMP_DEBUGPRINT("ERROR:Sget_OcstbHostMemoryInfo");
              return SNMP_ERR_GENERR;
        }
        snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                                 (u_char *)&StorageMemoryObj.AvailableSystemMemory
                                  ,sizeof(StorageMemoryObj.AvailableSystemMemory)
                                  );
        break;


    default:
        /*
         * we should never get here, so this is a really bad error
         */
    	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostAvailableSystemMemory\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
handle_ocStbHostJVMHeapSize(netsnmp_mib_handler *handler,
                            netsnmp_handler_registration *reginfo,
                            netsnmp_agent_request_info *reqinfo,
                            netsnmp_request_info *requests)
{
    /*
     * We are never called for a GETNEXT if it's registered as a
     * "instance", as it's "magically" handled for us.
     */

    /*
     * a instance handler also only hands us one request at a time, so
     * we don't need to loop over a list of requests; we'll only get one.
     */

    switch (reqinfo->mode) {

    case MODE_GET:
         if(0 == Sget_OcstbHostJVMInfo(&JVMstorageObj))
        {
              SNMP_DEBUGPRINT("ERROR:Sget_OcstbHostJVMInfo");
              return SNMP_ERR_GENERR;
        }
        snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                                 (u_char *)&JVMstorageObj.JVMHeapSize
                                 ,sizeof(JVMstorageObj.JVMHeapSize)
                                 );
        break;


    default:
        /*
         * we should never get here, so this is a really bad error
         */
    	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostJVMHeapSize\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
handle_ocStbHostJVMAvailHeap(netsnmp_mib_handler *handler,
                             netsnmp_handler_registration *reginfo,
                             netsnmp_agent_request_info *reqinfo,
                             netsnmp_request_info *requests)
{
    /*
     * We are never called for a GETNEXT if it's registered as a
     * "instance", as it's "magically" handled for us.
     */

    /*
     * a instance handler also only hands us one request at a time, so
     * we don't need to loop over a list of requests; we'll only get one.
     */

    switch (reqinfo->mode) {

    case MODE_GET:
        if(0 == Sget_OcstbHostJVMInfo(&JVMstorageObj))
        {
              SNMP_DEBUGPRINT("ERROR:Sget_OcstbHostJVMInfo");
              return SNMP_ERR_GENERR;
        }
        snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                                 (u_char *)&JVMstorageObj.JVMAvailHeap
                                 ,sizeof(JVMstorageObj.JVMAvailHeap)
                                  );
        break;


    default:
        /*
         * we should never get here, so this is a really bad error
         */
    	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostJVMAvailHeap\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
handle_ocStbHostJVMLiveObjects(netsnmp_mib_handler *handler,
                               netsnmp_handler_registration *reginfo,
                               netsnmp_agent_request_info *reqinfo,
                               netsnmp_request_info *requests)
{
    /*
     * We are never called for a GETNEXT if it's registered as a
     * "instance", as it's "magically" handled for us.
     */

    /*
     * a instance handler also only hands us one request at a time, so
     * we don't need to loop over a list of requests; we'll only get one.
     */

    switch (reqinfo->mode) {

    case MODE_GET:
        if(0 == Sget_OcstbHostJVMInfo(&JVMstorageObj))
        {
              SNMP_DEBUGPRINT("ERROR:Sget_OcstbHostJVMInfo");
              return SNMP_ERR_GENERR;
        }
        snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                                 (u_char *)&JVMstorageObj.JVMLiveObjects
                                  ,sizeof(JVMstorageObj.JVMLiveObjects)
                                 );
        break;


    default:
        /*
         * we should never get here, so this is a really bad error
         */
    	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostJVMLiveObjects\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
handle_ocStbHostJVMDeadObjects(netsnmp_mib_handler *handler,
                               netsnmp_handler_registration *reginfo,
                               netsnmp_agent_request_info *reqinfo,
                               netsnmp_request_info *requests)
{
    /*
     * We are never called for a GETNEXT if it's registered as a
     * "instance", as it's "magically" handled for us.
     */

    /*
     * a instance handler also only hands us one request at a time, so
     * we don't need to loop over a list of requests; we'll only get one.
     */

    switch (reqinfo->mode) {

    case MODE_GET:
        if(0 == Sget_OcstbHostJVMInfo(&JVMstorageObj))
        {
              SNMP_DEBUGPRINT("ERROR:Sget_OcstbHostJVMInfo");
              return SNMP_ERR_GENERR;
        }
        snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                                 (u_char *)&JVMstorageObj.JVMDeadObjects
                                 ,sizeof(JVMstorageObj.JVMDeadObjects)
                                  );
        break;


    default:
        /*
         * we should never get here, so this is a really bad error
         */
    	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostJVMDeadObjects\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
handle_ocStbHostCardBindingStatus(netsnmp_mib_handler *handler,
                                  netsnmp_handler_registration *reginfo,
                                  netsnmp_agent_request_info *reqinfo,
                                  netsnmp_request_info *requests)
{

    return SNMP_ERR_NOERROR;
}
int
handle_ocStbHostCardId(netsnmp_mib_handler *handler,
                       netsnmp_handler_registration *reginfo,
                       netsnmp_agent_request_info *reqinfo,
                       netsnmp_request_info *requests)
{

    return SNMP_ERR_NOERROR;
}
int
handle_ocStbHostCardMfgId(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info *reqinfo,
                          netsnmp_request_info *requests)
{
    switch (reqinfo->mode) {

    case MODE_GET:
       if(0 == Sget_ocStbHostSnmpProxyCardInfo(&gProxyCardInfo))
        {
              SNMP_DEBUGPRINT("Sget_ocStbHostSnmpProxyCardInfo");
              return SNMP_ERR_GENERR;
        }
        snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                 (u_char *)
                                 gProxyCardInfo.ocStbHostCardMfgId
                                 ,
                                 2);
       
        break;
    default:
        /*
         * we should never get here, so this is a really bad error
         */
    	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostCardMfgId\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
handle_ocStbHostCardRootOid(netsnmp_mib_handler *handler,
                            netsnmp_handler_registration *reginfo,
                            netsnmp_agent_request_info *reqinfo,
                            netsnmp_request_info *requests)
{
    switch (reqinfo->mode) {

    case MODE_GET:
        if(0 == Sget_ocStbHostSnmpProxyCardInfo(&gProxyCardInfo))
        {
              SNMP_DEBUGPRINT("Sget_ocStbHostSnmpProxyCardInfo");
              return SNMP_ERR_GENERR;
        }
        snmp_set_var_typed_value(requests->requestvb, ASN_OBJECT_ID,
                                 (u_char *)
                                 gProxyCardInfo.ocStbHostCardRootOid
                                 ,  gProxyCardInfo.CardRootOidLen*sizeof(oid));//10*sizeof(oid)                       /*sizeof(oid)*(sizeof(gProxyCardInfo.ocStbHostCardRootOid)/sizeof(unsigned long))*/);
        break;
  default:
        /*
         * we should never get here, so this is a really bad error
         */
    	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostCardRootOid\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
handle_ocStbHostCardSerialNumber(netsnmp_mib_handler *handler,
                                 netsnmp_handler_registration *reginfo,
                                 netsnmp_agent_request_info *reqinfo,
                                 netsnmp_request_info *requests)
{
   switch (reqinfo->mode) {

    case MODE_GET:
        if(0 == Sget_ocStbHostSnmpProxyCardInfo(&gProxyCardInfo))
        {
              SNMP_DEBUGPRINT("Sget_ocStbHostSnmpProxyCardInfo");
              return SNMP_ERR_GENERR;
        }
        snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                 (u_char *)
                                 gProxyCardInfo.ocStbHostCardSerialNumber
                                 ,
                                 strlen( gProxyCardInfo.ocStbHostCardSerialNumber));
      
      break;
    default:
        /*
         * we should never get here, so this is a really bad error
         */
    	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostCardSerialNumber\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

void EstbContorlAcess_HostcardrootID(int status)
{
    if(status == STATUS_TRUE)
    {
        vlStartupCardMibAccessProxy();
    }
    else if(status == STATUS_FALSE)
    {
        vlDisableCardMibAccessProxy();
    }
}
int
handle_ocStbHostCardSnmpAccessControl(netsnmp_mib_handler *handler,
                                      netsnmp_handler_registration *reginfo,
                                      netsnmp_agent_request_info   *reqinfo,
                                      netsnmp_request_info         *requests)
{
    int ret;
    /* We are never called for a GETNEXT if it's registered as a
    "instance", as it's "magically" handled for us.  */

    /* a instance handler also only hands us one request at a time, so
    we don't need to loop over a list of requests; we'll only get one. */
    
    switch(reqinfo->mode) {

        case MODE_GET:
            if(0 == Sget_ocStbHostSnmpProxyCardInfo(&gProxyCardInfo))
            {
                SNMP_DEBUGPRINT("Sget_ocStbHostSnmpProxyCardInfo");
                return SNMP_ERR_GENERR;
            }
            snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                                     (u_char*)
                                     &gProxyCardInfo.ocStbHostCardSnmpAccessControl,
                                    sizeof(gProxyCardInfo.ocStbHostCardSnmpAccessControl));
            break;
        /*
            * SET REQUEST
            *
         * multiple states in the transaction.  See:
            * http://www.net-snmp.org/tutorial-5/toolkit/mib_module/set-actions.jpg
        */
        case MODE_SET_RESERVE1:
            /* or you could use netsnmp_check_vb_type_and_size instead */
            ret = netsnmp_check_vb_type(requests->requestvb, ASN_INTEGER);
            //This step is to update the golbal variable to keep reference of updated value, Even though here it is not required we are modifying.
            if(*requests->requestvb->val.integer == STATUS_FALSE){

                gcardsnmpAccCtl = STATUS_FALSE;
            }
            if(*requests->requestvb->val.integer == STATUS_TRUE)
            {
                gcardsnmpAccCtl = STATUS_TRUE;
                return SNMP_ERR_NOERROR;
            }
            if ( ret != SNMP_ERR_NOERROR ) {
                netsnmp_set_request_error(reqinfo, requests, ret );
            }
            break;

        case MODE_SET_RESERVE2:
            /* XXX malloc "undo" storage buffer */
//             if (/* XXX if malloc, or whatever, failed: */) {
//                 netsnmp_set_request_error(reqinfo, requests, SNMP_ERR_RESOURCEUNAVAILABLE);
//             }
             break;

        case MODE_SET_FREE:
            /* XXX: free resources allocated in RESERVE1 and/or
            RESERVE2.  Something failed somewhere, and the states
            below won't be called. */
            break;

        case MODE_SET_ACTION:
            /* XXX: perform the value change here */
            ret = netsnmp_check_vb_type(requests->requestvb, ASN_INTEGER);
            if(*requests->requestvb->val.integer == STATUS_TRUE){

                gProxyCardInfo.ocStbHostCardSnmpAccessControl = STATUS_TRUE;
                //EstbContorlAcess_HostcardrootID(gcardsnmpAccCtl);
            }
            if(*requests->requestvb->val.integer == STATUS_FALSE)
            {
                gProxyCardInfo.ocStbHostCardSnmpAccessControl = STATUS_FALSE;
               // EstbContorlAcess_HostcardrootID(gcardsnmpAccCtl);
            }
            if (ret != SNMP_ERR_NOERROR) {
                netsnmp_set_request_error(reqinfo, requests, SNMP_ERR_RESOURCEUNAVAILABLE);
            }
            break;

        case MODE_SET_COMMIT:
             /* XXX: delete temporary storage */
            ret = netsnmp_check_vb_type(requests->requestvb, ASN_INTEGER);
            if(*requests->requestvb->val.integer == STATUS_TRUE)
            {

                gProxyCardInfo.ocStbHostCardSnmpAccessControl = STATUS_TRUE;
                gcardsnmpAccCtl = STATUS_TRUE;
                EstbContorlAcess_HostcardrootID(gcardsnmpAccCtl);
            }
            if(*requests->requestvb->val.integer == STATUS_FALSE)
            {
                gProxyCardInfo.ocStbHostCardSnmpAccessControl = STATUS_FALSE;
                gcardsnmpAccCtl = STATUS_FALSE;
                EstbContorlAcess_HostcardrootID(gcardsnmpAccCtl);
            }
            if (ret != SNMP_ERR_NOERROR) {
                /* try _really_really_ hard to never get to this point */
                netsnmp_set_request_error(reqinfo, requests, SNMP_ERR_COMMITFAILED);
            }
            break;

        case MODE_SET_UNDO:
            /* XXX: UNDO and return to previous value for the object */
            
//             if (/* XXX: error? */) {
//                 /* try _really_really_ hard to never get to this point */
//                 netsnmp_set_request_error(reqinfo, requests, SNMP_ERR_UNDOFAILED);
//             }
            break;

        default:
            /* we should never get here, so this is a really bad error */
    	    RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
            		"unknown mode (%d) in handle_ocStbHostCardSnmpAccessControll\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
handle_ocStbHostCardVersion(netsnmp_mib_handler *handler,
                            netsnmp_handler_registration *reginfo,
                            netsnmp_agent_request_info *reqinfo,
                            netsnmp_request_info *requests)
{
    switch (reqinfo->mode) {

    case MODE_GET:
               
        if(RMF_SUCCESS != IPC_CLIENT_vlMCardGetCardVersionNumberFromMMI((char*)(gProxyCardInfo.ocStbHostCardVersion), sizeof(gProxyCardInfo.ocStbHostCardVersion)))
        {
              SNMP_DEBUGPRINT("IPC_CLIENT_vlMCardGetCardVersionNumberFromMMI() returned ERROR\n");
              return SNMP_ERR_GENERR;
        }
        snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                 (u_char *)
                                 gProxyCardInfo.ocStbHostCardVersion
                                 ,
                                 strlen(gProxyCardInfo.ocStbHostCardVersion));
      break;
    default:
        /*
         * we should never get here, so this is a really bad error
         */
    	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostCardVersion\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
        handle_ocStbHostCardOpenedGenericResource(netsnmp_mib_handler *handler,
        netsnmp_handler_registration
                *reginfo,
        netsnmp_agent_request_info
                *reqinfo,
        netsnmp_request_info *requests)
{
    /*
    * We are never called for a GETNEXT if it's registered as a
    * "instance", as it's "magically" handled for us.
    */

    /*
    * a instance handler also only hands us one request at a time, so
    * we don't need to loop over a list of requests; we'll only get one.
    */

    switch (reqinfo->mode) {

        case MODE_GET:
                      
            if(0 == Sget_ocStbHostSnmpProxyCardInfo(&gProxyCardInfo))
            {
                SNMP_DEBUGPRINT("Sget_ocStbHostSnmpProxyCardInfo");
                return SNMP_ERR_GENERR;
            }
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                (u_char*)gProxyCardInfo.ocStbHostCardOpenedGenericResource,4 );
           break;


        default:
        /*
            * we should never get here, so this is a really bad error
        */
    	    RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                     "unknown mode (%d) in handle_ocStbHostCardOpenedGenericResource\n",
                     reqinfo->mode);
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
        handle_ocStbHostCardTimeZoneOffset(netsnmp_mib_handler *handler,
                                           netsnmp_handler_registration *reginfo,
                                           netsnmp_agent_request_info *reqinfo,
                                           netsnmp_request_info *requests)
{
    /*
    * We are never called for a GETNEXT if it's registered as a
    * "instance", as it's "magically" handled for us.
    */

    /*
    * a instance handler also only hands us one request at a time, so
    * we don't need to loop over a list of requests; we'll only get one.
    */

    switch (reqinfo->mode) {

        case MODE_GET:
        {
            if(0 == Sget_ocStbHostSnmpProxyCardInfo(&gProxyCardInfo))
            {
                SNMP_DEBUGPRINT("Sget_ocStbHostSnmpProxyCardInfo");
                return SNMP_ERR_GENERR;
            }
            ///To get correct time zone , do 2's complement to the comming value
            ///logic to implement x,y  x = value; find x -ve | +ve value and y = ~x+1;
            short time2scomplement;
            short timeZone ;
            time2scomplement = 0;
            timeZone  = 0;
            time2scomplement = gProxyCardInfo.ocStbHostCardTimeZoneOffset;
            int nTimeZoneScalingFactor = 60;
            static bool bTimeZoneScaleToMinutes = vl_env_get_bool(false, "SNMP.OC_STB_TIME_ZONE.SCALE_TO_MINUTES");
            if(bTimeZoneScaleToMinutes)
            {
                nTimeZoneScalingFactor = 1;
            }
            
            if(time2scomplement & 0x8000)
            {
                //add -ve;
                    timeZone = ~time2scomplement;
                    timeZone  = timeZone +1;
                    timeZone = -(timeZone/nTimeZoneScalingFactor);
            }
            else
            {
                    timeZone = (time2scomplement/nTimeZoneScalingFactor);
            }
            snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                                     (u_char *)&timeZone
                                             ,sizeof(timeZone));
                                             
        }
        break;

       default:
       {
        /*
            * we should never get here, so this is a really bad error
        */
    	    RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                     "unknown mode (%d) in handle_ocStbHostCardTimeZoneOffset\n",
                     reqinfo->mode);
            return SNMP_ERR_GENERR;
       }
       break;
    }

    return SNMP_ERR_NOERROR;
}

int
        handle_ocStbHostCardDaylightSavingsTimeDelta(netsnmp_mib_handler *handler,
        netsnmp_handler_registration
                *reginfo,
        netsnmp_agent_request_info
                *reqinfo,
        netsnmp_request_info
                *requests)
{
    /*
    * We are never called for a GETNEXT if it's registered as a
    * "instance", as it's "magically" handled for us.
    */

    /*
    * a instance handler also only hands us one request at a time, so
    * we don't need to loop over a list of requests; we'll only get one.
    */

    switch (reqinfo->mode) {

        case MODE_GET:
            
            if(0 == Sget_ocStbHostSnmpProxyCardInfo(&gProxyCardInfo))
            {
                SNMP_DEBUGPRINT("Sget_ocStbHostSnmpProxyCardInfo");
                return SNMP_ERR_GENERR;
            }
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                       (u_char*)gProxyCardInfo.ocStbHostCardDaylightSavingsTimeDelta  , 1);//only one byte 
          

            break;


        default:
        /*
            * we should never get here, so this is a really bad error
        */
    	    RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                     "unknown mode (%d) in handle_ocStbHostCardDaylightSavingsTimeDelta\n",
                     reqinfo->mode);
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
        handle_ocStbHostCardDaylightSavingsTimeEntry(netsnmp_mib_handler *handler,
        netsnmp_handler_registration
                *reginfo,
        netsnmp_agent_request_info
                *reqinfo,
        netsnmp_request_info
                *requests)
{
    /*
    * We are never called for a GETNEXT if it's registered as a
    * "instance", as it's "magically" handled for us.
    */

    /*
    * a instance handler also only hands us one request at a time, so
    * we don't need to loop over a list of requests; we'll only get one.
    */

    switch (reqinfo->mode) {

        case MODE_GET:
            
            if(0 == Sget_ocStbHostSnmpProxyCardInfo(&gProxyCardInfo))
            {
                SNMP_DEBUGPRINT("Sget_ocStbHostSnmpProxyCardInfo");
                return SNMP_ERR_GENERR;
            }
            snmp_set_var_typed_value(requests->requestvb, ASN_UNSIGNED,
                                     (u_char *)&gProxyCardInfo.ocStbHostCardDaylightSavingsTimeEntry ,sizeof(gProxyCardInfo.ocStbHostCardDaylightSavingsTimeEntry) );
          
            break;


        default:
        /*
            * we should never get here, so this is a really bad error
        */
    	    RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                     "unknown mode (%d) in handle_ocStbHostCardDaylightSavingsTimeEntry\n",
                     reqinfo->mode);
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
        handle_ocStbHostCardDaylightSavingsTimeExit(netsnmp_mib_handler *handler,
        netsnmp_handler_registration
                *reginfo,
        netsnmp_agent_request_info
                *reqinfo,
        netsnmp_request_info *requests)
{
    /*
    * We are never called for a GETNEXT if it's registered as a
    * "instance", as it's "magically" handled for us.
    */

    /*
    * a instance handler also only hands us one request at a time, so
    * we don't need to loop over a list of requests; we'll only get one.
    */

    switch (reqinfo->mode) {

        case MODE_GET:
             
            if(0 == Sget_ocStbHostSnmpProxyCardInfo(&gProxyCardInfo))
            {
                SNMP_DEBUGPRINT("Sget_ocStbHostSnmpProxyCardInfo");
                return SNMP_ERR_GENERR;
            }
            snmp_set_var_typed_value(requests->requestvb, ASN_UNSIGNED,
                                     (u_char *)&gProxyCardInfo.ocStbHostCardDaylightSavingsTimeExit
                                             ,sizeof(gProxyCardInfo.ocStbHostCardDaylightSavingsTimeExit)          );
            break;


        default:
        /*
            * we should never get here, so this is a really bad error
        */
    	    RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                     "unknown mode (%d) in handle_ocStbHostCardDaylightSavingsTimeExit\n",
                     reqinfo->mode);
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
        handle_ocStbHostCardEaLocationCode(netsnmp_mib_handler *handler,
                                           netsnmp_handler_registration *reginfo,
                                           netsnmp_agent_request_info *reqinfo,
                                           netsnmp_request_info *requests)
{
    /*
    * We are never called for a GETNEXT if it's registered as a
    * "instance", as it's "magically" handled for us.
    */

    /*
    * a instance handler also only hands us one request at a time, so
    * we don't need to loop over a list of requests; we'll only get one.
    */

    switch (reqinfo->mode) {

        case MODE_GET:
            
            if(0 == Sget_ocStbHostSnmpProxyCardInfo(&gProxyCardInfo))
            {
                SNMP_DEBUGPRINT("Sget_ocStbHostSnmpProxyCardInfo");
                return SNMP_ERR_GENERR;
            }
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *)gProxyCardInfo.ocStbHostCardEaLocationCode,
                                     3  );
           
            break;


        default:
        /*
            * we should never get here, so this is a really bad error
        */
    	    RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                     "unknown mode (%d) in handle_ocStbHostCardEaLocationCode\n",
                     reqinfo->mode);
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
        handle_ocStbHostCardVctId(netsnmp_mib_handler *handler,
                                  netsnmp_handler_registration *reginfo,
                                  netsnmp_agent_request_info *reqinfo,
                                  netsnmp_request_info *requests)
{
    /*
    * We are never called for a GETNEXT if it's registered as a
    * "instance", as it's "magically" handled for us.
    */

    /*
    * a instance handler also only hands us one request at a time, so
    * we don't need to loop over a list of requests; we'll only get one.
    */

    switch (reqinfo->mode) {

        case MODE_GET:
             
            if(0 == Sget_ocStbHostSnmpProxyCardInfo(&gProxyCardInfo))
            {
                SNMP_DEBUGPRINT("Sget_ocStbHostSnmpProxyCardInfo");
                return SNMP_ERR_GENERR;
            }
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *)gProxyCardInfo.ocStbHostCardVctId
                                             ,2 );
          
            break;


        default:
        /*
            * we should never get here, so this is a really bad error
        */
    	    RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                     "unknown mode (%d) in handle_ocStbHostCardVctId\n",
                     reqinfo->mode);
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int
        handle_ocStbHostCardCpAuthKeyStatus(netsnmp_mib_handler *handler,
                                            netsnmp_handler_registration *reginfo,
                                            netsnmp_agent_request_info *reqinfo,
                                            netsnmp_request_info *requests)
{

    return SNMP_ERR_NOERROR;
}

int
        handle_ocStbHostCardCpCertificateCheck(netsnmp_mib_handler *handler,
                                               netsnmp_handler_registration
                                                       *reginfo,
                                               netsnmp_agent_request_info *reqinfo,
                                               netsnmp_request_info *requests)
{

    return SNMP_ERR_NOERROR;
}

int
        handle_ocStbHostCardCpCciChallengeCount(netsnmp_mib_handler *handler,
        netsnmp_handler_registration
                *reginfo,
        netsnmp_agent_request_info
                *reqinfo,
        netsnmp_request_info *requests)
{

    return SNMP_ERR_NOERROR;
}

int
        handle_ocStbHostCardCpKeyGenerationReqCount(netsnmp_mib_handler *handler,
        netsnmp_handler_registration
                *reginfo,
        netsnmp_agent_request_info
                *reqinfo,
        netsnmp_request_info *requests)
{

    return SNMP_ERR_NOERROR;
}

int
        handle_ocStbHostCardCpIdList(netsnmp_mib_handler *handler,
                                     netsnmp_handler_registration *reginfo,
                                     netsnmp_agent_request_info *reqinfo,
                                     netsnmp_request_info *requests)
{

    return SNMP_ERR_NOERROR;
}
int
handle_ocStbHostOobMessageMode(netsnmp_mib_handler *handler,
                               netsnmp_handler_registration *reginfo,
                               netsnmp_agent_request_info *reqinfo,
                               netsnmp_request_info *requests)
{

    switch (reqinfo->mode) {

    case MODE_GET:
        if(0 == Sget_ocStbHostSnmpProxyCardInfo(&gProxyCardInfo))
        {
              SNMP_DEBUGPRINT("Sget_ocStbHostSnmpProxyCardInfo");
              return SNMP_ERR_GENERR;
        }
        snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                                 (u_char *)
                                 &gProxyCardInfo.ocStbHostOobMessageMode
                                 ,
                                 sizeof(gProxyCardInfo.ocStbHostOobMessageMode));
        break;


    default:
        /*
         * we should never get here, so this is a really bad error
         */
    	    RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP",
                 "unknown mode (%d) in handle_ocStbHostOobMessageMode\n",
                 reqinfo->mode);
        return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
//#endif//scalar


