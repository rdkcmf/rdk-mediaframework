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



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "net-snmp/net-snmp-config.h"
#include "net-snmp/net-snmp-includes.h"
#include "net-snmp/definitions.h"
#include "net-snmp/library/system.h"
#include "net-snmp/library/snmp_client.h"
#include "net-snmp/library/snmp_transport.h"
#include "net-snmp/library/mib.h"
#include "net-snmp/library/snmp_parse_args.h"
#include "net-snmp/agent/agent_trap.h"
#include "net-snmp/agent/net-snmp-agent-includes.h"
#include "vlEnv.h"
#include "SnmpIORM.h"
#include "snmp_types.h"
#include "utilityMacros.h"
#include "bufParseUtilities.h"
#include "vlSnmpClient.h"
#include "xchanResApi.h"
#include "rmf_osal_thread.h"
#include "rdk_debug.h"
#include "ipcclient_impl.h"

#define VL_SNMP_LOCAL_COMMUNITY ((char*)"private")
#define VL_SNMP_LOCAL_PEER      ((char*)"tcp:127.0.0.1:1161")

#define VL_SNMP_PUBLIC_COMMUNITY   ((char*)"public")

//Sep-26-2014: RDKDEV-5887 : Made ECM community name configurable.
#define VL_SNMP_ECM_PEER        ((char*)"192.168.100.1")

#define VL_SNMP_PUBLIC_COMMUNITY    "public"
#define VL_SNMP_PRIVATE_COMMUNITY   "private"

#ifdef INTEL_CANMORE
#define VL_SNMP_ECM_LOCALNAME   ((char*)"192.168.100.200")
#else
#define VL_SNMP_ECM_LOCALNAME   (NULL) // Assumes that the VL implementation takes care of this by other means.
#endif

#define VL_SNMP_CLIENT_TIMEOUT  100000L    // 100ms // 1000000 us == 1 second

char * vlSnmpGetEcmReadCommunityName()
{
    static char szCommunityName[64] = VL_SNMP_PUBLIC_COMMUNITY;
    char szTmp[256];
    FILE * fp = fopen("/tmp/snmpd.conf", "r");
    if(NULL != fp)
    {
        if ( fscanf(fp, "%s%s%s%s", szTmp, szTmp, szTmp, szCommunityName) <= 0 )
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "%s: fscanf returned zero or negative value\n", __FUNCTION__);
        }
        fclose(fp);
    }
    
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "%s: Read Community Name is '%s'.\n", __FUNCTION__, szCommunityName);
    return szCommunityName;
}

char * vlSnmpGetEcmWriteCommunityName()
{
    static char szCommunityName[64] = VL_SNMP_PUBLIC_COMMUNITY;
    char szTmp[256];
    FILE * fp = fopen("/tmp/snmpd.conf", "r");
    if(NULL != fp)
    {
        if ( fscanf(fp, "%s%s%s%s", szTmp, szTmp, szTmp, szCommunityName) <=0 )
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "%s: fscanf returned zero or negative value\n", __FUNCTION__);
        }
        fclose(fp);
    }
    
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "%s: Write Community Name is '%s'.\n", __FUNCTION__, szCommunityName);
    return szCommunityName;
}

typedef struct _VL_OID_NAME_ID_PAIR
{
    const char * pszName;
    const char * pszOID;
    
}VL_OID_NAME_ID_PAIR;

static const VL_OID_NAME_ID_PAIR vlg_SnmpOidLookupTable[] =
{
    {"SNMPv2-MIB::sysUpTime.0"                              , ".1.3.6.1.2.1.1.3.0"},
    {"SNMPv2-MIB::sysDescr.0"                               , ".1.3.6.1.2.1.1.1.0"},
    {"SNMPv2-MIB::snmpTrapOID.0"                            , ".1.3.6.1.6.3.1.1.4.1.0"},
    {"UCD-SNMP-MIB::versionUpdateConfig.0"                  , ".1.3.6.1.4.1.2021.100.11.0"},
    {"IF-MIB::ifPhysAddress.2"                              , ".1.3.6.1.2.1.2.2.1.6.2"},
    {"IP-MIB::ipAddressOrigin.ipv4"                         , ".1.3.6.1.2.1.4.34.1.6.1"},
    {"IP-MIB::ipAddressOrigin.ipv6"                         , ".1.3.6.1.2.1.4.34.1.6.2"},
    {"ifMtu.17"                                             , ".1.3.6.1.2.1.2.2.1.4.17"},
    {"ifSpeed.17"                                           , ".1.3.6.1.2.1.2.2.1.5.17"},
    {"ifInOctets.17"                                        , ".1.3.6.1.2.1.2.2.1.10.17"},
    {"ifInUcastPkts.17"                                     , ".1.3.6.1.2.1.2.2.1.11.17"},
    {"ifInNUcastPkts.17"                                    , ".1.3.6.1.2.1.2.2.1.12.17"},
    {"ifInDiscards.17"                                      , ".1.3.6.1.2.1.2.2.1.13.17"},
    {"ifInErrors.17"                                        , ".1.3.6.1.2.1.2.2.1.14.17"},
    {"ifInUnknownProtos.17"                                 , ".1.3.6.1.2.1.2.2.1.15.17"},
    {"ifOutOctets.17"                                       , ".1.3.6.1.2.1.2.2.1.16.17"},
    {"ifOutUcastPkts.17"                                    , ".1.3.6.1.2.1.2.2.1.17.17"},
    {"ifOutNUcastPkts.17"                                   , ".1.3.6.1.2.1.2.2.1.18.17"},
    {"ifOutDiscards.17"                                     , ".1.3.6.1.2.1.2.2.1.19.17"},
    {"ifOutErrors.17"                                       , ".1.3.6.1.2.1.2.2.1.20.17"},
    {"OC-STB-HOST-MIB::ocStbPanicDumpTrap.0"                , ".1.3.6.1.4.1.4491.2.3.1.0.1.0"},
    {"OC-STB-HOST-MIB::ocStbHostDumpFilePath.0"             , ".1.3.6.1.4.1.4491.2.3.1.1.4.5.4.4.0"},
    {"OC-STB-HOST-MIB::ocStbHostCardMacAddress.0"           , ".1.3.6.1.4.1.4491.2.3.1.1.4.4.1.0"},
    {"OC-STB-HOST-MIB::ocStbHostLargestAvailableBlock.0"    , ".1.3.6.1.4.1.4491.2.3.1.1.4.7.1.0"},
    {"OC-STB-HOST-MIB::ocStbHostJVMHeapSize.0"              , ".1.3.6.1.4.1.4491.2.3.1.1.4.8.1.0"},
    {"OC-STB-HOST-MIB::ocStbHostRebootType.0"               , ".1.3.6.1.4.1.4491.2.3.1.1.4.6.1.0"},
    {"DOCS-IF-MIB::docsIfDownChannelFrequency.3"            , ".1.3.6.1.2.1.10.127.1.1.1.1.2.3"},    
    {"DOCS-IF-MIB::docsIfDownChannelPower.3"                , ".1.3.6.1.2.1.10.127.1.1.1.1.6.3"},
    {"DOCS-IF-MIB::docsIfDownChannelModulation.3"           , ".1.3.6.1.2.1.10.127.1.1.1.1.4.3"},
    {"DOCS-IF-MIB::docsIfDownChannelWidth.3"                , ".1.3.6.1.2.1.10.127.1.1.1.1.3.3"},
    {"DOCS-IF-MIB::docsIfUpChannelType.4"                   , ".1.3.6.1.2.1.10.127.1.1.2.1.15.4"},
    {"DOCS-IF-MIB::docsIfCmStatusModulationType.2"          , ".1.3.6.1.2.1.10.127.1.2.2.1.16.2"},
    {"DOCS-IF-MIB::docsIfUpChannelFrequency.4"              , ".1.3.6.1.2.1.10.127.1.1.2.1.2.4"},    
    {"DOCS-IF-MIB::docsIfCmStatusTxPower.2"                 , ".1.3.6.1.2.1.10.127.1.2.2.1.3.2"},    
    {"DOCS-IF3-MIB::docsIf3SignalQualityExtRxMER.3"         , ".1.3.6.1.4.1.4491.2.1.20.1.24.1.1.3"},    
    {"DOCS-IF-MIB::docsIfSigQSignalNoise.3"                 , ".1.3.6.1.2.1.10.127.1.1.4.1.5.3"},    
    {"docsDevRole.0"                                        , ".1.3.6.1.2.1.69.1.1.1.0"},
    {"docsDevSerialNumber.0"                                , ".1.3.6.1.2.1.69.1.1.4.0"},
    {"docsDevSTPControl.0"                                  , ".1.3.6.1.2.1.69.1.1.5.0"},
    {"docsDevIgmpModeControl.0"                             , ".1.3.6.1.2.1.69.1.1.6.0"},
    {"docsDevMaxCpe.0"                                      , ".1.3.6.1.2.1.69.1.1.7.0"},
    {"docsDevSwServer.0"                                    , ".1.3.6.1.2.1.69.1.3.1.0"},
    {"docsDevSwFilename.0"                                  , ".1.3.6.1.2.1.69.1.3.2.0"},
    {"docsDevSwAdminStatus.0"                               , ".1.3.6.1.2.1.69.1.3.3.0"},
    {"docsDevSwOperStatus.0"                                , ".1.3.6.1.2.1.69.1.3.4.0"},
    {"docsDevSwCurrentVers.0"                               , ".1.3.6.1.2.1.69.1.3.5.0"},
    {"docsDevSwServerAddressType.0"                         , ".1.3.6.1.2.1.69.1.3.6.0"},
    {"docsDevSwServerAddress.0"                             , ".1.3.6.1.2.1.69.1.3.7.0"},
    {"docsDevSwServerTransportProtocol.0"                   , ".1.3.6.1.2.1.69.1.3.8.0"},
    {"docsDevServerBootState.0"                             , ".1.3.6.1.2.1.69.1.4.1.0"},
    {"docsDevServerDhcp.0"                                  , ".1.3.6.1.2.1.69.1.4.2.0"},
    {"docsDevServerTime.0"                                  , ".1.3.6.1.2.1.69.1.4.3.0"},
    {"docsDevServerTftp.0"                                  , ".1.3.6.1.2.1.69.1.4.4.0"},
    {"docsDevServerConfigFile.0"                            , ".1.3.6.1.2.1.69.1.4.5.0"},
    {"docsDevServerDhcpAddressType.0"                       , ".1.3.6.1.2.1.69.1.4.6.0"},
    {"docsDevServerDhcpAddress.0"                           , ".1.3.6.1.2.1.69.1.4.7.0"},
    {"docsDevServerTimeAddressType.0"                       , ".1.3.6.1.2.1.69.1.4.8.0"},
    {"docsDevServerTimeAddress.0"                           , ".1.3.6.1.2.1.69.1.4.9.0"},
    {"docsDevServerConfigTftpAddressType.0"                 , ".1.3.6.1.2.1.69.1.4.10.0"},
    {"docsDevServerConfigTftpAddress.0"                     , ".1.3.6.1.2.1.69.1.4.11.0"},
    {"docsDevDateTime.0"                                    , ".1.3.6.1.2.1.69.1.1.2.0"},
    {"IP-MIB::ipAdEntAddr"                                  , ".1.3.6.1.2.1.4.20.1.1"},
    {"IP-MIB::ipAdEntNetMask"                               , ".1.3.6.1.2.1.4.20.1.3"},
};                                                             

char * vlSnmpGetEcmIfIpAddress()
{
    return VL_SNMP_ECM_LOCALNAME;
}

VL_SNMP_API_RESULT vlSnmpGetPublicLong(char * pszIpAddress, char * pszOID, unsigned long * pLong)
{
    if(NULL != pLong)
    {
        VL_SNMP_CLIENT_VALUE aResults[] =
        {
            VL_SNMP_CLIENT_INTEGER_ENTRY(pLong),
        };
        int nResults = VL_ARRAY_ITEM_COUNT(aResults);
        return vlSnmpGetPublicObjects(pszIpAddress, pszOID, &nResults, aResults);
    }

    return VL_SNMP_API_RESULT_NULL_PARAM;
}

VL_SNMP_API_RESULT vlSnmpGetLocalLong(char * pszOID, unsigned long * pLong)
{
    if(NULL != pLong)
    {
        VL_SNMP_CLIENT_VALUE aResults[] =
        {
            VL_SNMP_CLIENT_INTEGER_ENTRY(pLong),
        };
        int nResults = VL_ARRAY_ITEM_COUNT(aResults);
        return vlSnmpGetLocalObjects(pszOID, &nResults, aResults);
    }

    return VL_SNMP_API_RESULT_NULL_PARAM;
}

VL_SNMP_API_RESULT vlSnmpGetEcmLong(char * pszOID, unsigned long * pLong)
{
    if(NULL != pLong)
    {
        VL_SNMP_CLIENT_VALUE aResults[] =
        {
            VL_SNMP_CLIENT_INTEGER_ENTRY(pLong),
        };
        int nResults = VL_ARRAY_ITEM_COUNT(aResults);
        return vlSnmpGetEcmObjects(pszOID, &nResults, aResults);
    }

    return VL_SNMP_API_RESULT_NULL_PARAM;
}

VL_SNMP_API_RESULT vlSnmpGetPublicByteArray(char * pszIpAddress, char * pszOID, int nCapacity, unsigned char * paBytArray)
{
    VL_SNMP_CLIENT_VALUE aResults[] =
    {
        VL_SNMP_CLIENT_BYTE_ARRAY_ENTRY(nCapacity, paBytArray),
    };
    int nResults = VL_ARRAY_ITEM_COUNT(aResults);
    if(NULL == paBytArray) return VL_SNMP_API_RESULT_NULL_PARAM;
    if(nCapacity <= 0    ) return VL_SNMP_API_RESULT_INVALID_PARAM;
    return vlSnmpGetPublicObjects(pszIpAddress, pszOID, &nResults, aResults);
}

VL_SNMP_API_RESULT vlSnmpGetLocalByteArray(char * pszOID, int nCapacity, unsigned char * paBytArray)
{
    VL_SNMP_CLIENT_VALUE aResults[] =
    {
        VL_SNMP_CLIENT_BYTE_ARRAY_ENTRY(nCapacity, paBytArray),
    };
    int nResults = VL_ARRAY_ITEM_COUNT(aResults);
    if(NULL == paBytArray) return VL_SNMP_API_RESULT_NULL_PARAM;
    if(nCapacity <= 0    ) return VL_SNMP_API_RESULT_INVALID_PARAM;
    return vlSnmpGetLocalObjects(pszOID, &nResults, aResults);
}

VL_SNMP_API_RESULT vlSnmpGetEcmByteArray(char * pszOID, int nCapacity, unsigned char * paBytArray)
{
    VL_SNMP_CLIENT_VALUE aResults[] =
    {
        VL_SNMP_CLIENT_BYTE_ARRAY_ENTRY(nCapacity, paBytArray),
    };
    int nResults = VL_ARRAY_ITEM_COUNT(aResults);
    if(NULL == paBytArray) return VL_SNMP_API_RESULT_NULL_PARAM;
    if(nCapacity <= 0    ) return VL_SNMP_API_RESULT_INVALID_PARAM;
    return vlSnmpGetEcmObjects(pszOID, &nResults, aResults);
}

VL_SNMP_API_RESULT vlSnmpGetPublicString(char * pszIpAddress, char * pszOID, int nCapacity, char * pszString)
{
    VL_SNMP_CLIENT_VALUE aResults[] =
    {
        VL_SNMP_CLIENT_STRING_ENTRY(nCapacity, pszString),
    };
    int nResults = VL_ARRAY_ITEM_COUNT(aResults);
    if(NULL == pszString) return VL_SNMP_API_RESULT_NULL_PARAM;
    if(nCapacity <= 0   ) return VL_SNMP_API_RESULT_INVALID_PARAM;
    return vlSnmpGetPublicObjects(pszIpAddress, pszOID, &nResults, aResults);
}

VL_SNMP_API_RESULT vlSnmpGetLocalString(char * pszOID, int nCapacity, char * pszString)
{
    VL_SNMP_CLIENT_VALUE aResults[] =
    {
        VL_SNMP_CLIENT_STRING_ENTRY(nCapacity, pszString),
    };
    int nResults = VL_ARRAY_ITEM_COUNT(aResults);
    if(NULL == pszString) return VL_SNMP_API_RESULT_NULL_PARAM;
    if(nCapacity <= 0   ) return VL_SNMP_API_RESULT_INVALID_PARAM;
    return vlSnmpGetLocalObjects(pszOID, &nResults, aResults);
}

VL_SNMP_API_RESULT vlSnmpGetEcmString(char * pszOID, int nCapacity, char * pszString)
{
    VL_SNMP_CLIENT_VALUE aResults[] =
    {
        VL_SNMP_CLIENT_STRING_ENTRY(nCapacity, pszString),
    };
    int nResults = VL_ARRAY_ITEM_COUNT(aResults);
    if(NULL == pszString) return VL_SNMP_API_RESULT_NULL_PARAM;
    if(nCapacity <= 0   ) return VL_SNMP_API_RESULT_INVALID_PARAM;
    return vlSnmpGetEcmObjects(pszOID, &nResults, aResults);
}

VL_SNMP_API_RESULT vlSnmpGetPublicStringDump(char * pszIpAddress, char * pszOID, int nResultCapacity, char * pszResult)
{
    VL_SNMP_CLIENT_VALUE aResults[] =
    {
        VL_SNMP_CLIENT_DUMP_ENTRY(nResultCapacity, pszResult),
    };
    int nResults = VL_ARRAY_ITEM_COUNT(aResults);
    if(NULL == pszResult    ) return VL_SNMP_API_RESULT_NULL_PARAM;
    if(nResultCapacity <= 0 ) return VL_SNMP_API_RESULT_INVALID_PARAM;
    return vlSnmpGetPublicObjects(pszIpAddress, pszOID, &nResults, aResults);
}

VL_SNMP_API_RESULT vlSnmpGetLocalStringDump(char * pszOID, int nResultCapacity, char * pszResult)
{
    VL_SNMP_CLIENT_VALUE aResults[] =
    {
        VL_SNMP_CLIENT_DUMP_ENTRY(nResultCapacity, pszResult),
    };
    int nResults = VL_ARRAY_ITEM_COUNT(aResults);
    if(NULL == pszResult    ) return VL_SNMP_API_RESULT_NULL_PARAM;
    if(nResultCapacity <= 0 ) return VL_SNMP_API_RESULT_INVALID_PARAM;
    return vlSnmpGetLocalObjects(pszOID, &nResults, aResults);
}

VL_SNMP_API_RESULT vlSnmpGetEcmStringDump(char * pszOID, int nResultCapacity, char * pszResult)
{
    /*VL_SNMP_CLIENT_VALUE aResults[] =
    {
        VL_SNMP_CLIENT_DUMP_ENTRY(nResultCapacity, pszResult),
        VL_SNMP_CLIENT_DUMP_ENTRY(nResultCapacity, pszResult),
    };*/
    
    /*VL_SNMP_CLIENT_VALUE aResults[] =
    {
        VL_SNMP_CLIENT_DUMP_ENTRY(nResultCapacity, pszResult),
        VL_SNMP_CLIENT_DUMP_ENTRY(nResultCapacity, pszResult),
        VL_SNMP_CLIENT_DUMP_ENTRY(nResultCapacity, pszResult),
        VL_SNMP_CLIENT_DUMP_ENTRY(nResultCapacity, pszResult),
        VL_SNMP_CLIENT_DUMP_ENTRY(nResultCapacity, pszResult),
        VL_SNMP_CLIENT_DUMP_ENTRY(nResultCapacity, pszResult),
        VL_SNMP_CLIENT_DUMP_ENTRY(nResultCapacity, pszResult),
        VL_SNMP_CLIENT_DUMP_ENTRY(nResultCapacity, pszResult),
        VL_SNMP_CLIENT_DUMP_ENTRY(nResultCapacity, pszResult),
        VL_SNMP_CLIENT_DUMP_ENTRY(nResultCapacity, pszResult),
        VL_SNMP_CLIENT_DUMP_ENTRY(nResultCapacity, pszResult),
        VL_SNMP_CLIENT_DUMP_ENTRY(nResultCapacity, pszResult),
        VL_SNMP_CLIENT_DUMP_ENTRY(nResultCapacity, pszResult),
        VL_SNMP_CLIENT_DUMP_ENTRY(nResultCapacity, pszResult),
        VL_SNMP_CLIENT_DUMP_ENTRY(nResultCapacity, pszResult),
        VL_SNMP_CLIENT_DUMP_ENTRY(nResultCapacity, pszResult),
    };*/
    
    VL_SNMP_CLIENT_VALUE aResults[] =
    {
        VL_SNMP_CLIENT_DUMP_ENTRY(nResultCapacity, pszResult),
    };
    int nResults = VL_ARRAY_ITEM_COUNT(aResults);
    if(NULL == pszResult    ) return VL_SNMP_API_RESULT_NULL_PARAM;
    if(nResultCapacity <= 0 ) return VL_SNMP_API_RESULT_INVALID_PARAM;
    return vlSnmpGetEcmObjects(pszOID, &nResults, aResults);
}

VL_SNMP_API_RESULT vlSnmpGetPublicObjects(char * pszIpAddress, char * pszOID, int * pnResults, VL_SNMP_CLIENT_VALUE * paResults)
{
    return vlSnmpGetObjects(VL_SNMP_PUBLIC_COMMUNITY, pszIpAddress, pszOID, pnResults, paResults);
}

VL_SNMP_API_RESULT vlSnmpGetLocalObjects(char * pszOID, int * pnResults, VL_SNMP_CLIENT_VALUE * paResults)
{
    return vlSnmpGetObjects(VL_SNMP_LOCAL_COMMUNITY, VL_SNMP_LOCAL_PEER, pszOID, pnResults, paResults);
}

VL_SNMP_API_RESULT vlSnmpGetEcmObjects(char * pszOID, int * pnResults, VL_SNMP_CLIENT_VALUE * paResults)
{
    IPC_CLIENT_vlXchanSetEcmRoute();
    return vlSnmpGetObjects(vlSnmpGetEcmReadCommunityName(), VL_SNMP_ECM_PEER, pszOID, pnResults, paResults);
}

static void vlSnmpClientCopyAsnValue(VL_SNMP_CLIENT_VALUE * pResult, netsnmp_variable_list *vars)
{
    if(NULL == pResult) return;
    if(NULL == vars   ) return;
    if(NULL == pResult->Value.pVoid) return;
    
    if(VL_SNMP_CLIENT_VALUE_TYPE_NONE == pResult->eValueType)
    {
        // do nothing;
        return;
    }

    if((NULL != pResult->palOidNameOid) && (pResult->nOidNameCapacity > 0))
    {
        int iOid = 0;
        pResult->nOidNameLength = vars->name_length;
        for(iOid = 0; (iOid < pResult->nOidNameCapacity) && (iOid < vars->name_length); iOid++)
        {
            pResult->palOidNameOid[iOid] = vars->name[iOid];
        } // for iOid
    } // pResult->nOidNameCapacity > 0
    
    if(pResult->nValueCapacity > 0)
    {
        int iVal = 0;
        for(iVal = 0; (iVal < pResult->nValueCapacity) && (iVal < vars->val_len); iVal++)
        {
            switch(pResult->eValueType)
            {
                case VL_SNMP_CLIENT_VALUE_TYPE_INTEGER:
                {
                    switch(vars->type)
                    {
                        case ASN_BOOLEAN:
                        case ASN_COUNTER:
                        case ASN_GAUGE:
                        case ASN_TIMETICKS:
                        case ASN_INTEGER:
                        {
                            pResult->Value.palValue[iVal] = vars->val.integer[iVal];
                        }
                        break;

                        case ASN_OBJECT_ID:
                        {
                            pResult->Value.palValue[iVal] = vars->val.objid[iVal];
                        }
                        break;

                        case ASN_BIT_STR:
                        case ASN_OCTET_STR:
                        case ASN_IPADDRESS:
                        {
                            pResult->Value.palValue[iVal] = vars->val.string[iVal];
                        }
                        break;

                        default:
                        {
                            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "%s: vars->type = %d\n", __FUNCTION__, vars->type);
                        }
                        break;
                    }
                }
                break; // VL_SNMP_CLIENT_VALUE_TYPE_INTEGER

                case VL_SNMP_CLIENT_VALUE_TYPE_BYTE:
                {
                    switch(vars->type)
                    {
                        case ASN_BOOLEAN:
                        case ASN_COUNTER:
                        case ASN_GAUGE:
                        case ASN_TIMETICKS:
                        case ASN_INTEGER:
                        {
                            pResult->Value.paBytValue[iVal] = (unsigned char)(vars->val.integer[iVal]);
                        }
                        break;

                        case ASN_OBJECT_ID:
                        {
                            pResult->Value.paBytValue[iVal] = (unsigned char)(vars->val.objid[iVal]);
                        }
                        break;

                        case ASN_BIT_STR:
                        case ASN_OCTET_STR:
                        case ASN_IPADDRESS:
                        {
                            pResult->Value.paBytValue[iVal] = (unsigned char)(vars->val.string[iVal]);
                        }
                        break;

                        default:
                        {
                            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "%s: vars->type = %d\n", __FUNCTION__, vars->type);
                        }
                        break;
                    }
                }
                break; // VL_SNMP_CLIENT_VALUE_TYPE_BYTE
            } // switch pResult->eValueType
        } // for iVal
        
        // add null termination
        if(iVal < pResult->nValueCapacity)
        {
            switch(pResult->eValueType)
            {
                case VL_SNMP_CLIENT_VALUE_TYPE_INTEGER:
                {
                    pResult->Value.palValue[iVal] = 0;
                }
                break; // VL_SNMP_CLIENT_VALUE_TYPE_INTEGER

                case VL_SNMP_CLIENT_VALUE_TYPE_BYTE:
                {
                    pResult->Value.paBytValue[iVal] = (unsigned char)(0);
                }
                break; // VL_SNMP_CLIENT_VALUE_TYPE_BYTE
            } // switch pResult->eValueType
        }
        
        pResult->nValues = iVal;
    } // pResult->nValueCapacity > 0
}

unsigned long * vlSnmpConvertStringToOid(const char * pszString, unsigned long * paOid, int * pnOidCapacity)
{// this function does not use strtok()

    // variables
    int iOid = 0;
    int nOidCapacity = 0;
    
    // null checks
    if(NULL == paOid) return NULL;
    if(NULL == pszString) return NULL;
    if(NULL == pnOidCapacity) return NULL;
    
    // register original capacity
    nOidCapacity = *pnOidCapacity;
    
    // zero all oids
    while(iOid < nOidCapacity)
    {
        paOid[iOid] = 0;
        iOid++;
    }
    
    // init to zero
    iOid = 0;
    *pnOidCapacity = 0;
    
    // while not end of string
    while('\0' != *pszString)
    {
        // skip leading white space
        while(isspace(*pszString))
        {
            // tolerate leading white space
            pszString++;
        }
        
        // check for end of string
        if('\0' == *pszString)
        {
            // end of string
            break;
        }
        
        // check for dot
        if('.' != *pszString)
        {
            // bad OID
            return NULL;
        }
        
        // skip dots
        while('.' == *pszString)
        {
            // tolerate multiple dots
            pszString++;
        }

        // check for end of string
        if('\0' == *pszString)
        {
            // end of string
            break;
        }
        
        // bounds check
        if(iOid < nOidCapacity)
        {
            // variable for end of parse position
            char * pszNewPosition = NULL;
            // parse oid value from string (skips leading white space)
            unsigned long nOidValue = (unsigned long)strtoul(pszString, &pszNewPosition, 10);
            // check if parse was successful
            if(pszNewPosition > pszString)
            {
                // store oid value
                paOid[iOid] = nOidValue;
                // increment index
                iOid++;
                // continue from where strtoul stopped
                pszString = pszNewPosition;
            }
            else // parse failed
            {
                // bad OID
                return NULL;
            }
        }
        else // cannot go beyond capacity
        {
            // stopping the parse
            break;
        }
    }
    
    // sanity check
    // ( number of parsed oid values should be
    //   greater than zero and less than oid capacity )
    if((iOid > 0) && (iOid < nOidCapacity))
    {
        // return number of oid values parsed
        *pnOidCapacity = iOid;
        // parse success
        return paOid;
    }
    
    // parse failure
    return NULL;
}

unsigned long * vl_snmp_parse_oid(const char * pszOID, oid * root, size_t * rootlen)
{
    if('.' == pszOID[0])
    {
        return (oid*) vlSnmpConvertStringToOid(pszOID, root, (int*)rootlen);
    }
    else
    {
        int iEntry = 0;
        for(iEntry = 0; iEntry < VL_ARRAY_ITEM_COUNT(vlg_SnmpOidLookupTable); iEntry++)
        {
            if(0 == strcmp(pszOID, vlg_SnmpOidLookupTable[iEntry].pszName))
            {
                return (oid*) vlSnmpConvertStringToOid(vlg_SnmpOidLookupTable[iEntry].pszOID, root, (int*)rootlen);
            }
        }
    }
    
    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SNMP", "%s: Bad OID: '%s'. OID should start with '.' / Else add to vlg_SnmpOidLookupTable.\n", __FUNCTION__, pszOID);
    return NULL;
}

VL_SNMP_API_RESULT vlSnmpGetObjects(char * pszCommunity, char * pszIpAddress, char * pszOID, int * pnResults, VL_SNMP_CLIENT_VALUE * paResults)
{
    if(NULL == pszOID)
    {
        return vlSnmpMultiGetObjects(pszCommunity, pszIpAddress, pnResults, paResults);
    }
    
    netsnmp_session session; void *ss = NULL;
    struct snmp_pdu *pdu = NULL;
    struct snmp_pdu *response = NULL;
    netsnmp_variable_list *vars = NULL;
    int status = 0, count = 0;
    int eGetType = SNMP_MSG_GET;
    int iResult = 0;
    int running = 1;
    int nResults = 0;
    int iOldResult = 0;
    int iLoop = 0;
    
    VL_SNMP_API_RESULT apiResult = VL_SNMP_API_RESULT_SUCCESS;
    
    VL_ZERO_STRUCT(session);
    snmp_sess_init( &session );                   /* set up defaults */

    if(NULL == pszCommunity ) return VL_SNMP_API_RESULT_NULL_PARAM;
    if(NULL == pszIpAddress ) return VL_SNMP_API_RESULT_NULL_PARAM;
    if(NULL == paResults    ) return VL_SNMP_API_RESULT_NULL_PARAM;
    if(NULL == pnResults    ) return VL_SNMP_API_RESULT_NULL_PARAM;

    session.peername = pszIpAddress;
    session.version = SNMP_VERSION_2c;
    session.community = (unsigned char*)pszCommunity;
    session.community_len = strlen((char*)(session.community));
    session.timeout = VL_SNMP_CLIENT_TIMEOUT;

    // bugfix 
    if(0 == strcmp(session.peername, VL_SNMP_ECM_PEER))
    {
        session.localname = vlSnmpGetEcmIfIpAddress();
    }    

    
    nResults = *pnResults;
    
    if(nResults <= 0 )        return VL_SNMP_API_RESULT_INVALID_PARAM;
    
    if(nResults > 1)
    {
        eGetType = SNMP_MSG_GETNEXT;
    }
    /*
    * open an SNMP session 
    */
    //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "Calling snmp_sess_open()...\n");
    ss = snmp_sess_open(&session);
    if (ss == NULL) {
        /*
        * diagnose snmp_sess_open errors with the input netsnmp_session pointer 
        */
        //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "Calling snmp_sess_perror()...\n");
        snmp_sess_perror("vlSnmpGetObjects", &session);
        return VL_SNMP_API_RESULT_OPEN_FAILED;
    }

   /*
    * Create the PDU for the data for our request.
    *   1) We're going to GET the system.sysDescr.0 node.
   */
    oid             root[MAX_OID_LEN];
    size_t          rootlen;
    //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "Calling read_objid()...\n");
    rootlen = MAX_OID_LEN;
    if (vl_snmp_parse_oid(pszOID, root, &rootlen) == NULL)
    {
        //snmp_sess_perror(__FUNCTION__);
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SNMP", "%s: Bad OID: '%s'.\n", __FUNCTION__, pszOID);
        apiResult = VL_SNMP_API_RESULT_INVALID_OID;
    }
    else
    {
        oid             name[MAX_OID_LEN];
        size_t          name_length;
        /*
        * get first object to start walk 
        */
        memmove(name, root, rootlen * sizeof(oid));
        name_length = rootlen;

        /*
        * Send the Request out.
        */
        //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "Calling snmp_sess_synch_response()...\n");
        iLoop = 0;
        while((iResult < nResults) && (running))
        {
            // infinite loop check
            if(iResult != iOldResult)
            {
                iOldResult = iResult;
                iLoop = 0;
            }
            else
            {
                iLoop++;
                if(iLoop > 10)
                {
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "%s: Infinite loop on node: '%s'.\n", __FUNCTION__, pszOID);
                    apiResult = VL_SNMP_API_RESULT_INFINITE_LOOP;
                    running = 0;
                    break;
                }
            }
            
            //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "Calling snmp_pdu_create()...\n");
            pdu = snmp_pdu_create(eGetType);
            
            if(NULL != pdu)
            {
                //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "Calling snmp_add_null_var()...\n");
                snmp_add_null_var(pdu, name, name_length);
                status = snmp_sess_synch_response(ss, pdu, &response);
    
                if((status == STAT_SUCCESS) && (NULL != response))
                {
                    switch (response->command)
                    {
                        case SNMP_MSG_GET:
                            SNMP_DEBUGPRINT("Received Get Response\n");
                            break;
                        case SNMP_MSG_GETNEXT:
                            SNMP_DEBUGPRINT("Received Getnext Response\n");
                            break;
                        case SNMP_MSG_RESPONSE:
                            SNMP_DEBUGPRINT("Received Get Response\n");
                            break;
                        case SNMP_MSG_SET:
                            SNMP_DEBUGPRINT("Received Set Response\n");
                            break;
                        case SNMP_MSG_TRAP:
                            SNMP_DEBUGPRINT("Received Trap Response\n");
                            break;
                        case SNMP_MSG_GETBULK:
                            SNMP_DEBUGPRINT("Received Bulk Response\n");
                            break;
                        case SNMP_MSG_INFORM:
                            SNMP_DEBUGPRINT("Received Inform Response\n");
                            break;
                        case SNMP_MSG_TRAP2:
                            SNMP_DEBUGPRINT("Received SNMPv2 Trap Response\n");
                            break;
                    }
                    if (response->errstat == SNMP_ERR_NOERROR)
                    {
                        /*
                        * check resulting variables 
                        */
                        for (vars = response->variables; vars;
                             vars = vars->next_variable)
                        {
                            if ((vars->name_length < rootlen)
                                 || (memcmp(root, vars->name, rootlen * sizeof(oid))
                                 != 0))
                            {
                                /*
                                * not part of this subtree 
                                */
                                
                                /*
                                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "%s: Got result for different OID. name_length=%d, rootlen=%d.\n", __FUNCTION__, vars->name_length, rootlen);
                                char szRootOID[VL_SNMP_STR_SIZE];
                                char szVarOID [VL_SNMP_STR_SIZE];
                                
                                vlSnmpConvertOidToString((const int*)(root), rootlen, szRootOID, sizeof(szRootOID));
                                vlSnmpConvertOidToString((const int*)(vars->name), vars->name_length, szVarOID , sizeof(szVarOID));
                                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "%s: Got result for different OID. Var Name = '%s', Root Name = '%s'.\n", __FUNCTION__, szVarOID, szRootOID);
                                */
                                
                                //apiResult = VL_SNMP_API_RESULT_OID_MISMATCH;
                                running = 0;
                                break;
                            }
                            //print_variable(vars->name, vars->name_length, vars);
                            if ((vars->type != SNMP_ENDOFMIBVIEW) &&
                                 (vars->type != SNMP_NOSUCHOBJECT) &&
                                 (vars->type != SNMP_NOSUCHINSTANCE))
                            {
                                /*
                                * not an exception value 
                                *\/
                                if (snmp_sess_oid_compare(name, name_length,
                                vars->name,
                                vars->name_length) >= 0)
                                {
                                    fprintf(stderr, "Error: OID not increasing: ");
                                    fprint_objid(stderr, name, name_length);
                                    fprintf(stderr, " >= ");
                                    fprint_objid(stderr, vars->name,
                                    vars->name_length);
                                    fprintf(stderr, "\n");
                                    running = 0;
                                    break;
                                }*/
                                memmove((char *) name, (char *) vars->name,
                                         vars->name_length * sizeof(oid));
                                name_length = vars->name_length;
                                
                                //print_variable(vars->name, vars->name_length, vars);
                                
                                vlSnmpClientCopyAsnValue(&(paResults[iResult]), vars);
                                
                                if((NULL != paResults[iResult].pszResult) &&
                                    (paResults[iResult].nResultCapacity > 0))
                                {
                                    int nChars = 
                                            snprint_variable(paResults[iResult].pszResult,
                                            paResults[iResult].nResultCapacity,
                                            vars->name, vars->name_length,
                                            vars);
                                    
                                    if(nChars < paResults[iResult].nResultCapacity)
                                    {
                                        apiResult = VL_SNMP_API_RESULT_SUCCESS;
                                    }
                                    else
                                    {
                                        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "%s: Result (size = %d) overflowed buffer (capacity = %d)\n", __FUNCTION__,
                                               nChars+1, paResults[iResult].nResultCapacity);
                                        apiResult = VL_SNMP_API_RESULT_BUFFER_OVERFLOW;
                                        //running = 0;
                                        //break;
                                    } // if nChars
                                } // if pszResult
                            } // if var
                            paResults[iResult].eSnmpObjectError = vars->type;
                            iResult++;
                        } // for vars
                        if(SNMP_MSG_GET == eGetType)
                        {
                            if ((paResults[0].eSnmpObjectError == SNMP_ENDOFMIBVIEW) ||
                                 (paResults[0].eSnmpObjectError == SNMP_NOSUCHOBJECT) ||
                                 (paResults[0].eSnmpObjectError == SNMP_NOSUCHINSTANCE))
                            {
                                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "%s: Tried get request on node or non-existent object: '%s'.\n", __FUNCTION__, pszOID);
                                apiResult = (VL_SNMP_API_RESULT)(paResults[0].eSnmpObjectError);
                            }
                            break;
                        } // if SNMP_MSG_GET
                    } // if response
                    else
                    {
                        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "%s: Error in packet.\n\t Reason: %s\n", __FUNCTION__,
                               snmp_errstring(response->errstat));
                        if (response->errindex != 0)
                        {
                            for (count = 1, vars = response->variables;
                                 vars && count != response->errindex;
                                 vars = vars->next_variable, count++)
                            {
                                if (vars)
                                {
                                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "%s: Failed String: ", __FUNCTION__);
                                    print_objid(vars->name, vars->name_length);
                                }
                            }
                            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "\n");
                        }
                        apiResult = VL_SNMP_API_RESULT_PROTOCOL_ERROR;
                        running = 0;
                        break;
                    } // else response
                }
                else if (status == STAT_TIMEOUT)
                {
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "%s: Timeout: No Response from %s\n", __FUNCTION__, session.peername);
                    apiResult = VL_SNMP_API_RESULT_TIMEOUT;
                    running = 0;
                    break;
                }
                else
                {   /* status == STAT_ERROR */
                    //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "Calling snmp_sess_perror()...\n");
                    snmp_sess_perror(__FUNCTION__, snmp_sess_session(ss));
                    apiResult = VL_SNMP_API_RESULT_CHECK_ERRNO;
                    running = 0;
                    break;
                }
                
                if (NULL != response)
                {
                    //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "Calling snmp_free_pdu(response)...\n");
                    snmp_free_pdu(response);
                }
                
            }// (null != pdu)
            
        }// ((iResult < nResults) && (running))
    
    }
    snmp_sess_close(ss);
    
    *pnResults = iResult;
    return apiResult;
}

VL_SNMP_API_RESULT vlSnmpMultiGetObjects(char * pszCommunity, char * pszIpAddress, int * pnResults, VL_SNMP_CLIENT_VALUE * paResults)
{
    netsnmp_session session;void *ss = NULL;
    struct snmp_pdu *pdu = NULL;
    struct snmp_pdu *response = NULL;
    netsnmp_variable_list *vars = NULL;
    int status = 0, count = 0;
    int eGetType = SNMP_MSG_GET;
    int iResult = 0;
    int running = 1;
    int nResults = 0;
    int iOldResult = 0;
    int iLoop = 0;
    
    VL_SNMP_API_RESULT apiResult = VL_SNMP_API_RESULT_SUCCESS;
    
    VL_ZERO_STRUCT(session);
    snmp_sess_init( &session );                   /* set up defaults */

    if(NULL == pszCommunity ) return VL_SNMP_API_RESULT_NULL_PARAM;
    if(NULL == pszIpAddress ) return VL_SNMP_API_RESULT_NULL_PARAM;
    if(NULL == paResults    ) return VL_SNMP_API_RESULT_NULL_PARAM;
    if(NULL == pnResults    ) return VL_SNMP_API_RESULT_NULL_PARAM;

    session.peername = pszIpAddress;
    session.version = SNMP_VERSION_2c;
    session.community = (unsigned char*)pszCommunity;
    session.community_len = strlen((char*)(session.community));
    session.timeout = VL_SNMP_CLIENT_TIMEOUT;

    // bugfix 
    if(0 == strcmp(session.peername, VL_SNMP_ECM_PEER))
    {
        session.localname = vlSnmpGetEcmIfIpAddress();
    }    
    
    nResults = *pnResults;
    
    if(nResults <= 0 )        return VL_SNMP_API_RESULT_INVALID_PARAM;
    
    /*
    * open an SNMP session 
    */
    //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "Calling snmp_sess_open()...\n");
    ss = snmp_sess_open(&session);
    if (ss == NULL) {
        /*
        * diagnose snmp_sess_open errors with the input netsnmp_session pointer 
        */
        //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "Calling snmp_sess_perror()...\n");
        snmp_sess_perror("vlSnmpMultiGetObjects", &session);
        return VL_SNMP_API_RESULT_OPEN_FAILED;
    }

    //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "Calling snmp_pdu_create()...\n");
    pdu = snmp_pdu_create(eGetType);
    
    if(NULL != pdu)
    {
        while((iResult < nResults) && (running))
        {
            oid             anOID[MAX_OID_LEN];
            size_t          oidlen = MAX_OID_LEN;
            
            if(NULL != pdu)
            {
                //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "Calling vl_snmp_parse_oid()...%s\n", paResults[iResult].pszOID);
                if(NULL == paResults[iResult].pszOID)
                {
                    apiResult = VL_SNMP_API_RESULT_NULL_PARAM;
                    snmp_free_pdu(pdu);
                    pdu = NULL;
                    running = 0;
                    break;
                }
                else if (vl_snmp_parse_oid(paResults[iResult].pszOID, anOID, &oidlen) == NULL)
                {
                    //snmp_sess_perror(__FUNCTION__);
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SNMP", "%s: Bad OID: '%s'.\n", __FUNCTION__, paResults[iResult].pszOID);
                    apiResult = VL_SNMP_API_RESULT_INVALID_OID;
                    snmp_free_pdu(pdu);
                    pdu = NULL;
                    running = 0;
                    break;
                }
                else
                {
                    //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "Calling snmp_add_null_var()...\n");
                    snmp_add_null_var(pdu, anOID, oidlen);
                    iResult++;
                }
            } // if
        } // while
    } // NULL != pdu
    
    /*
    * Send the Request out.
    */
    if((NULL != pdu) && (iResult > 0))
    {
        iResult = 0;
        //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "Calling snmp_sess_synch_response()...\n");
        status = snmp_sess_synch_response(ss, pdu, &response);

        if((status == STAT_SUCCESS) && (NULL != response))
        {
            switch (response->command)
            {
                case SNMP_MSG_GET:
                    SNMP_DEBUGPRINT("Received Get Response\n");
                    break;
                case SNMP_MSG_GETNEXT:
                    SNMP_DEBUGPRINT("Received Getnext Response\n");
                    break;
                case SNMP_MSG_RESPONSE:
                    SNMP_DEBUGPRINT("Received Get Response\n");
                    break;
                case SNMP_MSG_SET:
                    SNMP_DEBUGPRINT("Received Set Response\n");
                    break;
                case SNMP_MSG_TRAP:
                    SNMP_DEBUGPRINT("Received Trap Response\n");
                    break;
                case SNMP_MSG_GETBULK:
                    SNMP_DEBUGPRINT("Received Bulk Response\n");
                    break;
                case SNMP_MSG_INFORM:
                    SNMP_DEBUGPRINT("Received Inform Response\n");
                    break;
                case SNMP_MSG_TRAP2:
                    SNMP_DEBUGPRINT("Received SNMPv2 Trap Response\n");
                    break;
            }
            if (response->errstat == SNMP_ERR_NOERROR)
            {
                /*
                * check resulting variables 
                */
                for (vars = response->variables; vars;
                     vars = vars->next_variable)
                {
                    //print_variable(vars->name, vars->name_length, vars);
                    if ((vars->type != SNMP_ENDOFMIBVIEW) &&
                         (vars->type != SNMP_NOSUCHOBJECT) &&
                         (vars->type != SNMP_NOSUCHINSTANCE))
                    {
                        vlSnmpClientCopyAsnValue(&(paResults[iResult]), vars);
                        
                        if((NULL != paResults[iResult].pszResult) &&
                            (paResults[iResult].nResultCapacity > 0))
                        {
                            int nChars = 
                                    snprint_variable(paResults[iResult].pszResult,
                                    paResults[iResult].nResultCapacity,
                                    vars->name, vars->name_length,
                                    vars);
                            
                            if(nChars < paResults[iResult].nResultCapacity)
                            {
                                apiResult = VL_SNMP_API_RESULT_SUCCESS;
                            }
                            else
                            {
                                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "%s: Result (size = %d) overflowed buffer (capacity = %d)\n", __FUNCTION__,
                                       nChars+1, paResults[iResult].nResultCapacity);
                                apiResult = VL_SNMP_API_RESULT_BUFFER_OVERFLOW;
                                //running = 0;
                                //break;
                            } // if nChars
                        } // if pszResult
                    } // if var
                    paResults[iResult].eSnmpObjectError = vars->type;
                    iResult++;
                } // for vars
            } // if response
            else
            {
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "%s: Error in packet.\n\t Reason: %s\n", __FUNCTION__,
                       snmp_errstring(response->errstat));
                if (response->errindex != 0)
                {
                    for (count = 1, vars = response->variables;
                         vars && count != response->errindex;
                         vars = vars->next_variable, count++)
                    {
                        if (vars)
                        {
                            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "%s: Failed String: ", __FUNCTION__);
                            print_objid(vars->name, vars->name_length);
                        }
                    }
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "\n");
                }
                apiResult = VL_SNMP_API_RESULT_PROTOCOL_ERROR;
                running = 0;
            } // else response
        }
        else if (status == STAT_TIMEOUT)
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "%s: Timeout: No Response from %s\n", __FUNCTION__, session.peername);
            apiResult = VL_SNMP_API_RESULT_TIMEOUT;
            running = 0;
        }
        else
        {   /* status == STAT_ERROR */
            //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "Calling snmp_sess_perror()...\n");
            snmp_sess_perror(__FUNCTION__, snmp_sess_session(ss));
            apiResult = VL_SNMP_API_RESULT_CHECK_ERRNO;
            running = 0;
        }
        
        if (NULL != response)
        {
            //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "Calling snmp_free_pdu(response)...\n");
            snmp_free_pdu(response);
        }
        
    }// ((iResult < nResults) && (running))
    snmp_sess_close(ss);
    
    *pnResults = iResult;
    return apiResult;
}

VL_SNMP_API_RESULT vlSnmpSetPublicLong(char * pszIpAddress, char * pszOID, unsigned long nLong, VL_SNMP_CLIENT_REQUEST_TYPE eRequestType)
{
    VL_SNMP_CLIENT_VALUE aSets[] =
    {
        VL_SNMP_CLIENT_MULTI_SET_ASN_ENTRY(pszOID, VL_SNMP_CLIENT_VALUE_TYPE_INTEGER, &nLong, sizeof(nLong)),
    };
    int nSets = VL_ARRAY_ITEM_COUNT(aSets);
    return vlSnmpSetPublicObjects(pszIpAddress, &nSets, aSets, eRequestType);
}

VL_SNMP_API_RESULT vlSnmpSetLocalLong(char * pszOID, unsigned long nLong, VL_SNMP_CLIENT_REQUEST_TYPE eRequestType)
{
    VL_SNMP_CLIENT_VALUE aSets[] =
    {
        VL_SNMP_CLIENT_MULTI_SET_ASN_ENTRY(pszOID, VL_SNMP_CLIENT_VALUE_TYPE_INTEGER, &nLong, sizeof(nLong)),
    };
    int nSets = VL_ARRAY_ITEM_COUNT(aSets);
    return vlSnmpSetLocalObjects(&nSets, aSets, eRequestType);
}

VL_SNMP_API_RESULT vlSnmpSetEcmLong(char * pszOID, unsigned long nLong, VL_SNMP_CLIENT_REQUEST_TYPE eRequestType)
{
    VL_SNMP_CLIENT_VALUE aSets[] =
    {
        VL_SNMP_CLIENT_MULTI_SET_ASN_ENTRY(pszOID, VL_SNMP_CLIENT_VALUE_TYPE_INTEGER, &nLong, sizeof(nLong)),
    };
    int nSets = VL_ARRAY_ITEM_COUNT(aSets);
    return vlSnmpSetEcmObjects(&nSets, aSets, eRequestType);
}

VL_SNMP_API_RESULT vlSnmpSetPublicByteArray(char * pszIpAddress, char * pszOID, int nBytes, unsigned char * paBytArray, VL_SNMP_CLIENT_REQUEST_TYPE eRequestType)
{
    if(NULL != paBytArray)
    {
        VL_SNMP_CLIENT_VALUE aSets[] =
        {
            VL_SNMP_CLIENT_MULTI_SET_ASN_ENTRY(pszOID, VL_SNMP_CLIENT_VALUE_TYPE_BYTE, paBytArray, nBytes),
        };
        int nSets = VL_ARRAY_ITEM_COUNT(aSets);
        return vlSnmpSetPublicObjects(pszIpAddress, &nSets, aSets, eRequestType);
    }
    
    return VL_SNMP_API_RESULT_NULL_PARAM;
}

VL_SNMP_API_RESULT vlSnmpSetLocalByteArray(char * pszOID, int nBytes, unsigned char * paBytArray, VL_SNMP_CLIENT_REQUEST_TYPE eRequestType)
{
    if(NULL != paBytArray)
    {
        VL_SNMP_CLIENT_VALUE aSets[] =
        {
            VL_SNMP_CLIENT_MULTI_SET_ASN_ENTRY(pszOID, VL_SNMP_CLIENT_VALUE_TYPE_BYTE, paBytArray, nBytes),
        };
        int nSets = VL_ARRAY_ITEM_COUNT(aSets);
        return vlSnmpSetLocalObjects(&nSets, aSets, eRequestType);
    }
    
    return VL_SNMP_API_RESULT_NULL_PARAM;
}

VL_SNMP_API_RESULT vlSnmpSetEcmByteArray(char * pszOID, int nBytes, unsigned char * paBytArray, VL_SNMP_CLIENT_REQUEST_TYPE eRequestType)
{
    if(NULL != paBytArray)
    {
        VL_SNMP_CLIENT_VALUE aSets[] =
        {
            VL_SNMP_CLIENT_MULTI_SET_ASN_ENTRY(pszOID, VL_SNMP_CLIENT_VALUE_TYPE_BYTE, paBytArray, nBytes),
        };
        int nSets = VL_ARRAY_ITEM_COUNT(aSets);
        return vlSnmpSetEcmObjects(&nSets, aSets, eRequestType);
    }
    
    return VL_SNMP_API_RESULT_NULL_PARAM;
}

VL_SNMP_API_RESULT vlSnmpSetPublicString(char * pszIpAddress, char * pszOID, char * pszString, VL_SNMP_CLIENT_REQUEST_TYPE eRequestType)
{
    if(NULL != pszString)
    {
        VL_SNMP_CLIENT_VALUE aSets[] =
        {
            VL_SNMP_CLIENT_MULTI_SET_ASN_ENTRY(pszOID, VL_SNMP_CLIENT_VALUE_TYPE_BYTE, pszString, strlen(pszString)),
        };
        int nSets = VL_ARRAY_ITEM_COUNT(aSets);
        return vlSnmpSetPublicObjects(pszIpAddress, &nSets, aSets, eRequestType);
    }
    
    return VL_SNMP_API_RESULT_NULL_PARAM;
}

VL_SNMP_API_RESULT vlSnmpSetLocalString(char * pszOID, char * pszString, VL_SNMP_CLIENT_REQUEST_TYPE eRequestType)
{
    if(NULL != pszString)
    {
        VL_SNMP_CLIENT_VALUE aSets[] =
        {
            VL_SNMP_CLIENT_MULTI_SET_ASN_ENTRY(pszOID, VL_SNMP_CLIENT_VALUE_TYPE_BYTE, pszString, strlen(pszString)),
        };
        int nSets = VL_ARRAY_ITEM_COUNT(aSets);
        return vlSnmpSetLocalObjects(&nSets, aSets, eRequestType);
    }
    
    return VL_SNMP_API_RESULT_NULL_PARAM;
}

VL_SNMP_API_RESULT vlSnmpSetEcmString(char * pszOID, char * pszString, VL_SNMP_CLIENT_REQUEST_TYPE eRequestType)
{
    if(NULL != pszString)
    {
        VL_SNMP_CLIENT_VALUE aSets[] =
        {
            VL_SNMP_CLIENT_MULTI_SET_ASN_ENTRY(pszOID, VL_SNMP_CLIENT_VALUE_TYPE_BYTE, pszString, strlen(pszString)),
        };
        int nSets = VL_ARRAY_ITEM_COUNT(aSets);
        return vlSnmpSetEcmObjects(&nSets, aSets, eRequestType);
    }
    
    return VL_SNMP_API_RESULT_NULL_PARAM;
}

VL_SNMP_API_RESULT vlSnmpSetPublicStrVar(char * pszIpAddress, char * pszOID, char chValueType, char * pszValue, VL_SNMP_CLIENT_REQUEST_TYPE eRequestType)
{
    if(NULL != pszValue)
    {
        VL_SNMP_CLIENT_VALUE aSets[] =
        {
            VL_SNMP_CLIENT_MULTI_SET_STR_ENTRY(pszOID, chValueType, pszValue),
        };
        int nSets = VL_ARRAY_ITEM_COUNT(aSets);
        return vlSnmpSetPublicObjects(pszIpAddress, &nSets, aSets, eRequestType);
    }
    
    return VL_SNMP_API_RESULT_NULL_PARAM;
}

VL_SNMP_API_RESULT vlSnmpSetLocalStrVar(char * pszOID, char chValueType, char * pszValue, VL_SNMP_CLIENT_REQUEST_TYPE eRequestType)
{
    if(NULL != pszValue)
    {
        VL_SNMP_CLIENT_VALUE aSets[] =
        {
            VL_SNMP_CLIENT_MULTI_SET_STR_ENTRY(pszOID, chValueType, pszValue),
        };
        int nSets = VL_ARRAY_ITEM_COUNT(aSets);
        return vlSnmpSetLocalObjects(&nSets, aSets, eRequestType);
    }
    
    return VL_SNMP_API_RESULT_NULL_PARAM;
}

VL_SNMP_API_RESULT vlSnmpSetEcmStrVar(char * pszOID, char chValueType, char * pszValue, VL_SNMP_CLIENT_REQUEST_TYPE eRequestType)
{
    if(NULL != pszValue)
    {
        VL_SNMP_CLIENT_VALUE aSets[] =
        {
            VL_SNMP_CLIENT_MULTI_SET_STR_ENTRY(pszOID, chValueType, pszValue),
        };
        int nSets = VL_ARRAY_ITEM_COUNT(aSets);
        return vlSnmpSetEcmObjects(&nSets, aSets, eRequestType);
    }
    
    return VL_SNMP_API_RESULT_NULL_PARAM;
}

VL_SNMP_API_RESULT vlSnmpSetPublicAsnVar(char * pszIpAddress, char * pszOID, char asnValueType, void * pValue, int nBytes, VL_SNMP_CLIENT_REQUEST_TYPE eRequestType)
{
    if(NULL != pValue)
    {
        VL_SNMP_CLIENT_VALUE aSets[] =
        {
            VL_SNMP_CLIENT_MULTI_SET_ASN_ENTRY(pszOID, asnValueType, pValue, nBytes),
        };
        int nSets = VL_ARRAY_ITEM_COUNT(aSets);
        return vlSnmpSetPublicObjects(pszIpAddress, &nSets, aSets, eRequestType);
    }
    
    return VL_SNMP_API_RESULT_NULL_PARAM;
}

VL_SNMP_API_RESULT vlSnmpSetLocalAsnVar(char * pszOID, char asnValueType, void * pValue, int nBytes, VL_SNMP_CLIENT_REQUEST_TYPE eRequestType)
{
    if(NULL != pValue)
    {
        VL_SNMP_CLIENT_VALUE aSets[] =
        {
            VL_SNMP_CLIENT_MULTI_SET_ASN_ENTRY(pszOID, asnValueType, pValue, nBytes),
        };
        int nSets = VL_ARRAY_ITEM_COUNT(aSets);
        return vlSnmpSetLocalObjects(&nSets, aSets, eRequestType);
    }
    
    return VL_SNMP_API_RESULT_NULL_PARAM;
}

VL_SNMP_API_RESULT vlSnmpSetEcmAsnVar(char * pszOID, char asnValueType, void * pValue, int nBytes, VL_SNMP_CLIENT_REQUEST_TYPE eRequestType)
{
    if(NULL != pValue)
    {
        VL_SNMP_CLIENT_VALUE aSets[] =
        {
            VL_SNMP_CLIENT_MULTI_SET_ASN_ENTRY(pszOID, asnValueType, pValue, nBytes),
        };
        int nSets = VL_ARRAY_ITEM_COUNT(aSets);
        return vlSnmpSetEcmObjects(&nSets, aSets, eRequestType);
    }
    
    return VL_SNMP_API_RESULT_NULL_PARAM;
}

VL_SNMP_API_RESULT vlSnmpSetPublicObjects(char * pszIpAddress, int * pnResults, VL_SNMP_CLIENT_VALUE * paResults, VL_SNMP_CLIENT_REQUEST_TYPE eRequestType)
{
    return vlSnmpMultiSetObjects(VL_SNMP_PUBLIC_COMMUNITY, pszIpAddress, pnResults, paResults, eRequestType);
}

VL_SNMP_API_RESULT vlSnmpSetLocalObjects(int * pnResults, VL_SNMP_CLIENT_VALUE * paResults, VL_SNMP_CLIENT_REQUEST_TYPE eRequestType)
{
    return vlSnmpMultiSetObjects(VL_SNMP_LOCAL_COMMUNITY, VL_SNMP_LOCAL_PEER, pnResults, paResults, eRequestType);
}

VL_SNMP_API_RESULT vlSnmpSetEcmObjects(int * pnResults, VL_SNMP_CLIENT_VALUE * paResults, VL_SNMP_CLIENT_REQUEST_TYPE eRequestType)
{
    IPC_CLIENT_vlXchanSetEcmRoute();	
    return vlSnmpMultiSetObjects(vlSnmpGetEcmWriteCommunityName(), VL_SNMP_ECM_PEER, pnResults, paResults, eRequestType);
}

VL_SNMP_API_RESULT vlSnmpMultiSetObjects(char * pszCommunity, char * pszIpAddress, int * pnResults, VL_SNMP_CLIENT_VALUE * paResults, VL_SNMP_CLIENT_REQUEST_TYPE eRequestType)
{
    netsnmp_session session; void *ss = NULL;
    struct snmp_pdu *pdu = NULL;
    struct snmp_pdu *response = NULL;
    netsnmp_variable_list *vars = NULL;
    int status = 0, count = 0;
    int eMsgType = SNMP_MSG_SET;
    int iResult = 0;
    int running = 1;
    int nResults = 0;
    int iOldResult = 0;
    int iLoop = 0;
    
    VL_SNMP_API_RESULT apiResult = VL_SNMP_API_RESULT_SUCCESS;
    
    VL_ZERO_STRUCT(session);
    snmp_sess_init( &session );                   /* set up defaults */

    if(NULL == pszCommunity ) return VL_SNMP_API_RESULT_NULL_PARAM;
    if(NULL == pszIpAddress ) return VL_SNMP_API_RESULT_NULL_PARAM;
    if(NULL == paResults    ) return VL_SNMP_API_RESULT_NULL_PARAM;
    if(NULL == pnResults    ) return VL_SNMP_API_RESULT_NULL_PARAM;
	
    session.peername = pszIpAddress;
    session.version = SNMP_VERSION_2c;
    session.community = (unsigned char*)pszCommunity;
    session.community_len = strlen((char*)(session.community));
    session.timeout = VL_SNMP_CLIENT_TIMEOUT;

    // bugfix 
    if(0 == strcmp(session.peername, VL_SNMP_ECM_PEER))
    {
        session.localname = vlSnmpGetEcmIfIpAddress();
    }
       
    nResults = *pnResults;
    
    if(nResults <= 0 )        return VL_SNMP_API_RESULT_INVALID_PARAM;
    
    /*
    * open an SNMP session 
    */
    //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "Calling snmp_sess_open()...\n");
    ss = snmp_sess_open(&session);
    if (ss == NULL) {
        /*
        * diagnose snmp_sess_open errors with the input netsnmp_session pointer 
        */
        //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "Calling snmp_sess_perror()...\n");
        snmp_sess_perror("vlSnmpMultiSetObjects", &session);
        return VL_SNMP_API_RESULT_OPEN_FAILED;
    }

    //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "Calling snmp_pdu_create()...\n");
    pdu = snmp_pdu_create(eMsgType);
    
    if(NULL != pdu)
    {
        while((iResult < nResults) && (running))
        {
            oid             anOID[MAX_OID_LEN];
            size_t          oidlen = MAX_OID_LEN;
            
            if(NULL != pdu)
            {
                //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "Calling vl_snmp_parse_oid()...%s\n", paResults[iResult].pszOID);
                if(NULL == paResults[iResult].pszOID)
                {
                    apiResult = VL_SNMP_API_RESULT_NULL_PARAM;
                    snmp_free_pdu(pdu);
                    pdu = NULL;
                    running = 0;
                    break;
                }
                else if (vl_snmp_parse_oid(paResults[iResult].pszOID, anOID, &oidlen) == NULL)
                {
                    //snmp_sess_perror(__FUNCTION__);
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SNMP", "%s: Bad OID: '%s'.\n", __FUNCTION__, paResults[iResult].pszOID);
                    apiResult = VL_SNMP_API_RESULT_INVALID_OID;
                    snmp_free_pdu(pdu);
                    pdu = NULL;
                    running = 0;
                    break;
                }
                else
                {
                    //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "Calling snmp_pdu_add_variable()...\n");
                    if(paResults[iResult].nValues > 0)
                    {
                        snmp_pdu_add_variable(pdu, anOID, oidlen,
                                              paResults[iResult].eValueType,
                                              paResults[iResult].Value.paBytValue,
                                              paResults[iResult].nValues);
                    }
                    else
                    {
                        snmp_add_var(pdu, anOID, oidlen,
                                     paResults[iResult].eValueType,
                                     paResults[iResult].Value.pszValue);
                    }
                    iResult++;
                }
            } // if
        } // while
    } // NULL != pdu
    
    /*
    * Send the Request out.
    */
    if((NULL != pdu) && (iResult > 0))
    {
        iResult = 0;
        //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "Calling snmp_sess_synch_response()...\n");
        if(VL_SNMP_CLIENT_REQUEST_TYPE_SYNCHRONOUS == eRequestType)
        {
            status = snmp_sess_synch_response(ss, pdu, &response);
        }
        else
        {
            status = snmp_sess_send(ss, pdu);
            snmp_sess_close(ss);
            return VL_SNMP_API_RESULT_SUCCESS;
        }

        if((status == STAT_SUCCESS) && (NULL != response))
        {
            switch (response->command)
            {
                case SNMP_MSG_GET:
                    SNMP_DEBUGPRINT("Received Get Response\n");
                    break;
                case SNMP_MSG_GETNEXT:
                    SNMP_DEBUGPRINT("Received Getnext Response\n");
                    break;
                case SNMP_MSG_RESPONSE:
                    SNMP_DEBUGPRINT("Received Get Response\n");
                    break;
                case SNMP_MSG_SET:
                    SNMP_DEBUGPRINT("Received Set Response\n");
                    break;
                case SNMP_MSG_TRAP:
                    SNMP_DEBUGPRINT("Received Trap Response\n");
                    break;
                case SNMP_MSG_GETBULK:
                    SNMP_DEBUGPRINT("Received Bulk Response\n");
                    break;
                case SNMP_MSG_INFORM:
                    SNMP_DEBUGPRINT("Received Inform Response\n");
                    break;
                case SNMP_MSG_TRAP2:
                    SNMP_DEBUGPRINT("Received SNMPv2 Trap Response\n");
                    break;
            }
            if (response->errstat == SNMP_ERR_NOERROR)
            {
                /*
                * check resulting variables 
                */
                for (vars = response->variables; vars;
                     vars = vars->next_variable)
                {
                    //print_variable(vars->name, vars->name_length, vars);
                    if ((vars->type != SNMP_ENDOFMIBVIEW) &&
                         (vars->type != SNMP_NOSUCHOBJECT) &&
                         (vars->type != SNMP_NOSUCHINSTANCE))
                    {
                        //vlSnmpClientCopyAsnValue(&(paResults[iResult]), vars);
                        
                        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "===============================================================================\n");
                        print_variable(vars->name, vars->name_length,
                                       vars);
                        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "===============================================================================\n");
                        
                        if((NULL != paResults[iResult].pszResult) &&
                            (paResults[iResult].nResultCapacity > 0))
                        {
                            int nChars = 
                                    snprint_variable(paResults[iResult].pszResult,
                                    paResults[iResult].nResultCapacity,
                                    vars->name, vars->name_length,
                                    vars);
                            
                            if(nChars < paResults[iResult].nResultCapacity)
                            {
                                apiResult = VL_SNMP_API_RESULT_SUCCESS;
                            }
                            else
                            {
                                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "%s: Result (size = %d) overflowed buffer (capacity = %d)\n", __FUNCTION__,
                                       nChars+1, paResults[iResult].nResultCapacity);
                                apiResult = VL_SNMP_API_RESULT_BUFFER_OVERFLOW;
                                //running = 0;
                                //break;
                            } // if nChars
                        } // if pszResult
                    } // if var
                    if(0 == iResult)
                    {
                        apiResult = (VL_SNMP_API_RESULT)(paResults[iResult].eSnmpObjectError);
                    } // if iResult
                    paResults[iResult].eSnmpObjectError = vars->type;
                    iResult++;
                } // for vars
            } // if response
            else
            {
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "%s: Error in packet.\n\t Reason: %s\n", __FUNCTION__,
                       snmp_errstring(response->errstat));
                if (response->errindex != 0)
                {
                    for (count = 1, vars = response->variables;
                         vars && count != response->errindex;
                         vars = vars->next_variable, count++)
                    {
                        if (vars)
                        {
                            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "%s: Failed String: ", __FUNCTION__);
                            print_objid(vars->name, vars->name_length);
                        }
                    }
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "\n");
                }
                apiResult = VL_SNMP_API_RESULT_PROTOCOL_ERROR;
                running = 0;
            } // else response
        }
        else if (status == STAT_TIMEOUT)
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "%s: Timeout: No Response from %s\n", __FUNCTION__, session.peername);
            apiResult = VL_SNMP_API_RESULT_TIMEOUT;
            running = 0;
        }
        else
        {   /* status == STAT_ERROR */
            //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "Calling snmp_sess_perror()...\n");
            snmp_sess_perror(__FUNCTION__, snmp_sess_session(ss));
            apiResult = VL_SNMP_API_RESULT_CHECK_ERRNO;
            running = 0;
        }
        
        if (NULL != response)
        {
            //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "Calling snmp_free_pdu(response)...\n");
            snmp_free_pdu(response);
        }
        
    }// ((iResult < nResults) && (running))
    snmp_sess_close(ss);
    
    *pnResults = iResult;
    return apiResult;
}

static int vl_snmp_input(int operation,
               netsnmp_session * session,
               int reqid, netsnmp_pdu *pdu, void *magic)
{
    return 1;
}


VL_SNMP_API_RESULT vlSnmpSendNotificationComplex(
        const char * pszCommunity, const char * pszIpAddress, const char * pszTrapOid,
        int nRemotePort, int nTimeout, int nRetries,
        int * pnResults, VL_SNMP_CLIENT_VALUE * paResults,
        VL_SNMP_CLIENT_REQUEST_TYPE eRequestType)
{
    netsnmp_session session;void *ss = NULL;
    struct snmp_pdu *pdu = NULL;
    struct snmp_pdu *response = NULL;
    netsnmp_variable_list *vars = NULL;
    int status = 0, count = 0;
    int eMsgType = SNMP_MSG_TRAP2;
    int iResult = 0;
    int running = 1;
    int nResults = 0;
    int iOldResult = 0;
    int iLoop = 0;
    
    VL_SNMP_API_RESULT apiResult = VL_SNMP_API_RESULT_SUCCESS;
    
    VL_ZERO_STRUCT(session);
    snmp_sess_init( &session );                   /* set up defaults */

    if(NULL == pszCommunity ) return VL_SNMP_API_RESULT_NULL_PARAM;
    if(NULL == pszIpAddress ) return VL_SNMP_API_RESULT_NULL_PARAM;
    if(NULL == paResults    ) return VL_SNMP_API_RESULT_NULL_PARAM;
    if(NULL == pnResults    ) return VL_SNMP_API_RESULT_NULL_PARAM;
    if(NULL == pszTrapOid   ) return VL_SNMP_API_RESULT_NULL_PARAM;

    session.peername = (char*)pszIpAddress;
    session.remote_port = SNMP_TRAP_PORT;
    session.version = SNMP_VERSION_2c;
    session.community = (unsigned char*)pszCommunity;
    session.community_len = strlen((char*)(session.community));
    session.callback = vl_snmp_input;
    session.callback_magic = NULL;
    
    if(SNMP_TRAP_PORT != nRemotePort)
    {
        session.remote_port = nRemotePort;
    }
    
    if(VL_SNMP_CLIENT_REQUEST_TYPE_INFORM == eRequestType)
    {
        if(SNMP_DEFAULT_TIMEOUT != nTimeout)
        {
            session.timeout = nTimeout;
        }
        if(SNMP_DEFAULT_RETRIES != nRetries)
        {
            session.retries = nRetries;
        }
    }
       
    nResults = *pnResults;
    
    eMsgType = ((eRequestType == VL_SNMP_CLIENT_REQUEST_TYPE_INFORM) ? SNMP_MSG_INFORM : SNMP_MSG_TRAP2);
    
    if(VL_SNMP_CLIENT_REQUEST_TYPE_INFORM == eRequestType)
    {
        //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "Calling netsnmp_transport_open_client()...\n");
        netsnmp_transport* pTrapTransport = netsnmp_transport_open_client("snmptrap", session.peername);
        //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "Calling snmp_sess_add()...\n");
        ss = snmp_sess_add(&session, pTrapTransport, NULL, NULL);
        if (ss == NULL) {
            /*
            * diagnose snmp_sess_open errors with the input netsnmp_session pointer 
            */
            //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "Calling snmp_sess_perror()...\n");
            snmp_sess_perror("vlSnmpSendNotification", &session);
            return VL_SNMP_API_RESULT_OPEN_FAILED;
        }
    }

    //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "Calling snmp_pdu_create()...\n");
    pdu = snmp_pdu_create(eMsgType);
    
    if(NULL != pdu)
    {
        oid             anOID[MAX_OID_LEN];
        size_t          oidlen = MAX_OID_LEN;
        
        memset(&anOID,0,sizeof(anOID));
        oidlen = MAX_OID_LEN;
        
        if (vl_snmp_parse_oid("SNMPv2-MIB::sysUpTime.0", anOID, &oidlen) != NULL)
        {
            long uptime = get_uptime();
            snmp_pdu_add_variable(pdu, anOID, oidlen,
                                  ASN_TIMETICKS, (const unsigned char*)&uptime, sizeof(uptime));
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SNMP", "%s: Bad OID: '%s'.\n", __FUNCTION__, "SNMPv2-MIB::sysUpTime.0");
        }
        
        memset(&anOID,0,sizeof(anOID));
        oidlen = MAX_OID_LEN;
                
        if (vl_snmp_parse_oid("SNMPv2-MIB::snmpTrapOID.0", anOID, &oidlen) != NULL)
        {
            snmp_add_var(pdu, anOID, oidlen,
                         'o',
                         pszTrapOid);
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SNMP", "%s: Bad OID: '%s' or Bad Var: '%s'\n", __FUNCTION__, "SNMPv2-MIB::snmpTrapOID.0", pszTrapOid);
        }
        
        while((iResult < nResults) && (running))
        {
            memset(&anOID,0,sizeof(anOID));
            oidlen = MAX_OID_LEN;
                    
            if(NULL != pdu)
            {
                //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "Calling vl_snmp_parse_oid()...%s\n", paResults[iResult].pszOID);
                if(NULL == paResults[iResult].pszOID)
                {
                    apiResult = VL_SNMP_API_RESULT_NULL_PARAM;
                    snmp_free_pdu(pdu);
                    pdu = NULL;
                    running = 0;
                    break;
                }
                else if (vl_snmp_parse_oid(paResults[iResult].pszOID, anOID, &oidlen) == NULL)
                {
                    //snmp_sess_perror(__FUNCTION__);
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SNMP", "%s: Bad OID: '%s'.\n", __FUNCTION__, paResults[iResult].pszOID);
                    apiResult = VL_SNMP_API_RESULT_INVALID_OID;
                    snmp_free_pdu(pdu);
                    pdu = NULL;
                    running = 0;
                    break;
                }
                else
                {
                    //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "Calling snmp_pdu_add_variable()...\n");
                    if(paResults[iResult].nValues > 0)
                    {
                        snmp_pdu_add_variable(pdu, anOID, oidlen,
                                              paResults[iResult].eValueType,
                                              paResults[iResult].Value.paBytValue,
                                              paResults[iResult].nValues);
                    }
                    else
                    {
                        snmp_add_var(pdu, anOID, oidlen,
                                     paResults[iResult].eValueType,
                                     paResults[iResult].Value.pszValue);
                    }
                    iResult++;
                }
            } // if
        } // while
    } // NULL != pdu
    
    /*
    * Send the Request out.
    */
    if((NULL != pdu) && (iResult > 0))
    {
        iResult = 0;
        //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "Calling snmp_sess_synch_response()...\n");
        if(VL_SNMP_CLIENT_REQUEST_TYPE_INFORM == eRequestType)
        {
            status = snmp_sess_synch_response(ss, pdu, &response);
        }
        else
        {
            //status = snmp_sess_send(ss, pdu);
            //snmp_sess_close(ss);
            send_trap_pdu(pdu); // send trap to configured trap receivers (in snmpd.conf)
            return VL_SNMP_API_RESULT_SUCCESS;
        }

        if((status == STAT_SUCCESS) && (NULL != response))
        {
            switch (response->command)
            {
                case SNMP_MSG_GET:
                    SNMP_DEBUGPRINT("Received Get Response\n");
                    break;
                case SNMP_MSG_GETNEXT:
                    SNMP_DEBUGPRINT("Received Getnext Response\n");
                    break;
                case SNMP_MSG_RESPONSE:
                    SNMP_DEBUGPRINT("Received Get Response\n");
                    break;
                case SNMP_MSG_SET:
                    SNMP_DEBUGPRINT("Received Set Response\n");
                    break;
                case SNMP_MSG_TRAP:
                    SNMP_DEBUGPRINT("Received Trap Response\n");
                    break;
                case SNMP_MSG_GETBULK:
                    SNMP_DEBUGPRINT("Received Bulk Response\n");
                    break;
                case SNMP_MSG_INFORM:
                    SNMP_DEBUGPRINT("Received Inform Response\n");
                    break;
                case SNMP_MSG_TRAP2:
                    SNMP_DEBUGPRINT("Received SNMPv2 Trap Response\n");
                    break;
            }
            if (response->errstat == SNMP_ERR_NOERROR)
            {
                /*
                * check resulting variables 
                */
            } // if response
            else
            {
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "%s: Error in packet.\n\t Reason: %s\n", __FUNCTION__,
                       snmp_errstring(response->errstat));
                apiResult = VL_SNMP_API_RESULT_PROTOCOL_ERROR;
                running = 0;
            } // else response
        }
        else if (status == STAT_TIMEOUT)
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "%s: Timeout: No Response from %s\n", __FUNCTION__, session.peername);
            apiResult = VL_SNMP_API_RESULT_TIMEOUT;
            running = 0;
        }
        else
        {   /* status == STAT_ERROR */
            //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "Calling snmp_sess_perror()...\n");
            snmp_sess_perror(__FUNCTION__, snmp_sess_session(ss));
            apiResult = VL_SNMP_API_RESULT_CHECK_ERRNO;
            running = 0;
        }
        
        if (NULL != response)
        {
            //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "Calling snmp_free_pdu(response)...\n");
            snmp_free_pdu(response);
        }
        
    }// ((iResult < nResults) && (running))
    
    snmp_sess_close(ss);
    
    rmf_osal_threadSleep(100,0);
    
    *pnResults = iResult;
    return apiResult;
}

VL_SNMP_API_RESULT vlSnmpSendNotificationSimple(
        const char * pszCommunity, const char * pszIpAddress, const char * pszTrapOid,
        int * pnResults, VL_SNMP_CLIENT_VALUE * paResults,
        VL_SNMP_CLIENT_REQUEST_TYPE eRequestType)
{
    return vlSnmpSendNotificationComplex(pszCommunity, pszIpAddress, pszTrapOid,
                                  SNMP_TRAP_PORT, SNMP_DEFAULT_TIMEOUT, SNMP_DEFAULT_RETRIES,
                                  pnResults, paResults,
                                  eRequestType);
}

int vlSnmpConvertOidToString(const int * paOid, int nOid, char * pszString, int nStrCapacity)
{
    int i = 0;
    
    if((NULL != paOid) && (NULL != pszString))
    {
        char szOidVal[128];
        strcpy(pszString, "");
        for(i = 0; i < nOid; i++)
        {
            snprintf(szOidVal, sizeof(szOidVal), ".%d", paOid[i]);
            strcpy(pszString, szOidVal);
        }
    }
    
    return i;
}

#define VL_RETURN_CASE_STRING(x)   case (VL_SNMP_API_RESULT_##x): return (const char*)#x;
const char * vlSnmpGetErrorString(VL_SNMP_API_RESULT eResult)
{
    static char szUnknown[100];

    switch(eResult)
    {
        VL_RETURN_CASE_STRING(SUCCESS                  );
        VL_RETURN_CASE_STRING(ERR_TOOBIG               );
        VL_RETURN_CASE_STRING(ERR_NOSUCHNAME           );
        VL_RETURN_CASE_STRING(ERR_BADVALUE             );
        VL_RETURN_CASE_STRING(ERR_READONLY             );
        VL_RETURN_CASE_STRING(ERR_GENERR               );
        
        VL_RETURN_CASE_STRING(ERR_NOACCESS             );
        VL_RETURN_CASE_STRING(ERR_WRONGTYPE            );
        VL_RETURN_CASE_STRING(ERR_WRONGLENGTH          );
        VL_RETURN_CASE_STRING(ERR_WRONGENCODING        );
        VL_RETURN_CASE_STRING(ERR_WRONGVALUE           );
        VL_RETURN_CASE_STRING(ERR_NOCREATION           );
        VL_RETURN_CASE_STRING(ERR_INCONSISTENTVALUE    );
        VL_RETURN_CASE_STRING(ERR_RESOURCEUNAVAILABLE  );
        VL_RETURN_CASE_STRING(ERR_COMMITFAILED         );
        VL_RETURN_CASE_STRING(ERR_UNDOFAILED           );
        VL_RETURN_CASE_STRING(ERR_AUTHORIZATIONERROR   );
        VL_RETURN_CASE_STRING(ERR_NOTWRITABLE          );
        VL_RETURN_CASE_STRING(ERR_INCONSISTENTNAME     );
        
        VL_RETURN_CASE_STRING(ERR_NOSUCHOBJECT         );
        VL_RETURN_CASE_STRING(ERR_NOSUCHINSTANCE       );
        VL_RETURN_CASE_STRING(ERR_ENDOFMIBVIEW         );
        
        VL_RETURN_CASE_STRING(FAILED                   );
        VL_RETURN_CASE_STRING(CHECK_ERRNO              );
        VL_RETURN_CASE_STRING(UNSPECIFIED_ERROR        );
        VL_RETURN_CASE_STRING(ACCESS_DENIED            );
        VL_RETURN_CASE_STRING(NOT_IMPLEMENTED          );
        VL_RETURN_CASE_STRING(NOT_EXISTING             );
        VL_RETURN_CASE_STRING(NULL_PARAM               );
        VL_RETURN_CASE_STRING(INVALID_PARAM            );
        VL_RETURN_CASE_STRING(OUT_OF_RANGE             );
        VL_RETURN_CASE_STRING(OPEN_FAILED              );
        VL_RETURN_CASE_STRING(READ_FAILED              );
        VL_RETURN_CASE_STRING(WRITE_FAILED             );
        VL_RETURN_CASE_STRING(MALLOC_FAILED            );
        VL_RETURN_CASE_STRING(TIMEOUT                  );
        VL_RETURN_CASE_STRING(INFINITE_LOOP            );
        VL_RETURN_CASE_STRING(INVALID_OID              );
        VL_RETURN_CASE_STRING(GET_ON_NODE              );
        VL_RETURN_CASE_STRING(PROTOCOL_ERROR           );
        VL_RETURN_CASE_STRING(BUFFER_OVERFLOW          );
        VL_RETURN_CASE_STRING(OID_MISMATCH             );
        
        VL_RETURN_CASE_STRING(INVALID                  );

        default:
        {
        }
        break;
    }

    snprintf(szUnknown, sizeof(szUnknown), "Unknown (%d)", eResult);
    return szUnknown;
}
