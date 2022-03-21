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
 * @file This file contains function which gets the IP & mac address, up nad down
 * channel frequency, power, modulation type etc.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <net/if.h>
#include <arpa/inet.h>

#include "ipcclient_impl.h"
#include "SnmpIORM.h"
#include "snmp_types.h"
#include "utilityMacros.h"
#include "bufParseUtilities.h"
#include "vlSnmpClient.h"
#include "vlSnmpEcmApi.h"
#include "ip_types.h"
#include "xchanResApi.h"
#define VL_ECM_IP_ADDR_COUNT    2
#define VL_ECM_IPV6_ADDR_COUNT  6

/**
 * @brief This function is used to get the IP address, its type (IPV4/IPV6), subnet mask,
 * checks for ip address availability.
 *
 * @param[out] pIpAddress Struct Pointer where info about ip address is stored.
 *
 * @retval VL_SNMP_API_RESULT_NULL_PARAM If address of parameter is NULL. Indicates Failed condition.
 * @retval VL_SNMP_API_RESULT_SUCCESS Indicates success to get and update IP address .
 */
VL_SNMP_API_RESULT vlSnmpEcmGetIpAddress(VL_IP_ADDRESS_STRUCT * pIpAddress)
{
    unsigned char aBytIpV6GlobalAddress[VL_ECM_IP_ADDR_COUNT][VL_IPV6_ADDR_SIZE];
    unsigned char aBytIpV6SubnetMask[VL_ECM_IP_ADDR_COUNT][VL_IPV6_ADDR_SIZE];
    VL_SNMP_CLIENT_VALUE aResults[] =
    {
        VL_SNMP_CLIENT_BYTE_ARRAY_ENTRY(sizeof(aBytIpV6GlobalAddress[0]), aBytIpV6GlobalAddress[0]),
        VL_SNMP_CLIENT_BYTE_ARRAY_ENTRY(sizeof(aBytIpV6GlobalAddress[1]), aBytIpV6GlobalAddress[1]),
        VL_SNMP_CLIENT_BYTE_ARRAY_ENTRY(0, NULL),
    };
    VL_SNMP_CLIENT_VALUE aResults2[] =
    {
        VL_SNMP_CLIENT_BYTE_ARRAY_ENTRY(sizeof(aBytIpV6SubnetMask[0]), aBytIpV6SubnetMask[0]),
        VL_SNMP_CLIENT_BYTE_ARRAY_ENTRY(sizeof(aBytIpV6SubnetMask[1]), aBytIpV6SubnetMask[1]),
        VL_SNMP_CLIENT_BYTE_ARRAY_ENTRY(0, NULL),
    };
    int nResults = VL_ARRAY_ITEM_COUNT(aResults);
    int nResults2 = VL_ARRAY_ITEM_COUNT(aResults);
    int iAddr = 0;
    unsigned char * paCableCardIpAddr = IPC_CLIENT_vlXchanGetRcIpAddress();
    
    if(NULL == pIpAddress) return VL_SNMP_API_RESULT_NULL_PARAM;
    VL_ZERO_STRUCT(*pIpAddress);
    VL_SNMP_API_RESULT ret = vlSnmpGetEcmObjects((char*)"IP-MIB::ipAdEntAddr", &nResults, aResults);
    
    for(int iAddr = 0; iAddr < VL_ECM_IP_ADDR_COUNT; iAddr++)
    {
        if(VL_SNMP_API_RESULT_SUCCESS == ret)
        {
            switch(aResults[iAddr].nValues)
            {
                case VL_IP_ADDR_SIZE:
                {
                    pIpAddress->eIpAddressType = VL_XCHAN_IP_ADDRESS_TYPE_IPV4;
                    memcpy(pIpAddress->aBytIpAddress, aBytIpV6GlobalAddress[iAddr], VL_IP_ADDR_SIZE);
                    inet_ntop(AF_INET,pIpAddress->aBytIpAddress,
                            pIpAddress->szIpAddress,sizeof(pIpAddress->szIpAddress));
                }
                break;
                
                case VL_IPV6_ADDR_SIZE:
                {
                    pIpAddress->eIpAddressType = VL_XCHAN_IP_ADDRESS_TYPE_IPV6;
                    memcpy(pIpAddress->aBytIpV6GlobalAddress, aBytIpV6GlobalAddress[iAddr], VL_IPV6_ADDR_SIZE);
                    inet_ntop(AF_INET6,pIpAddress->aBytIpV6GlobalAddress,
                            pIpAddress->szIpV6GlobalAddress,sizeof(pIpAddress->szIpV6GlobalAddress));
                }
                break;
                
            }
        }
        
        ret = vlSnmpGetEcmObjects((char*)"IP-MIB::ipAdEntNetMask", &nResults2, aResults2);
        
        if(VL_SNMP_API_RESULT_SUCCESS == ret)
        {
            switch(aResults2[iAddr].nValues)
            {
                case VL_IP_ADDR_SIZE:
                {
                    memcpy(pIpAddress->aBytSubnetMask, aBytIpV6SubnetMask[iAddr], VL_IP_ADDR_SIZE);
                    inet_ntop(AF_INET,pIpAddress->aBytSubnetMask,
                            pIpAddress->szSubnetMask,sizeof(pIpAddress->szSubnetMask));
                }
                break;
                
                case VL_IPV6_ADDR_SIZE:
                {
                    memcpy(pIpAddress->aBytIpV6SubnetMask, aBytIpV6SubnetMask[iAddr], VL_IPV6_ADDR_SIZE);
                    inet_ntop(AF_INET6,pIpAddress->aBytIpV6SubnetMask,
                            pIpAddress->szIpV6SubnetMask,sizeof(pIpAddress->szIpV6SubnetMask));
                }
                break;
                
            }
        }
        
        if(NULL != paCableCardIpAddr)
        {
            const unsigned char rpc_addr_1[VL_IP_ADDR_SIZE] = {10,10,10,1};
            if(0 == memcmp(paCableCardIpAddr, aBytIpV6GlobalAddress[iAddr], VL_IP_ADDR_SIZE)) continue;
            if(0 == memcmp(rpc_addr_1, aBytIpV6GlobalAddress[iAddr], VL_IP_ADDR_SIZE)) continue;
            break; // success
        }
    }

    // check if an IPv4 address is available
    unsigned long ipAddressOrigin[VL_ECM_IPV6_ADDR_COUNT];
    memset(ipAddressOrigin, 0, sizeof(ipAddressOrigin));
    unsigned long aLongIpV6oid[VL_ECM_IPV6_ADDR_COUNT][20+VL_IPV6_ADDR_SIZE];
    VL_SNMP_CLIENT_VALUE aResults6[] =
    {
        VL_SNMP_CLIENT_OID_NAME_INT_VAL_ENTRY(&(ipAddressOrigin[0]), VL_ARRAY_ITEM_COUNT(aLongIpV6oid[0]), aLongIpV6oid[0]),
        VL_SNMP_CLIENT_OID_NAME_INT_VAL_ENTRY(&(ipAddressOrigin[1]), VL_ARRAY_ITEM_COUNT(aLongIpV6oid[1]), aLongIpV6oid[1]),
        VL_SNMP_CLIENT_OID_NAME_INT_VAL_ENTRY(&(ipAddressOrigin[2]), VL_ARRAY_ITEM_COUNT(aLongIpV6oid[2]), aLongIpV6oid[2]),
        VL_SNMP_CLIENT_OID_NAME_INT_VAL_ENTRY(&(ipAddressOrigin[3]), VL_ARRAY_ITEM_COUNT(aLongIpV6oid[3]), aLongIpV6oid[3]),
        VL_SNMP_CLIENT_OID_NAME_INT_VAL_ENTRY(&(ipAddressOrigin[4]), VL_ARRAY_ITEM_COUNT(aLongIpV6oid[4]), aLongIpV6oid[4]),
        VL_SNMP_CLIENT_OID_NAME_INT_VAL_ENTRY(NULL, 0, NULL),
    };
    
    int nResults4 = VL_ARRAY_ITEM_COUNT(aResults6);
    VL_SNMP_API_RESULT ret4 = vlSnmpGetEcmObjects((char*)"IP-MIB::ipAddressOrigin.ipv4", &nResults4, aResults6);
    
    if(VL_SNMP_API_RESULT_SUCCESS == ret4)
    {
        for(int iAddr = 0; iAddr < VL_ARRAY_ITEM_COUNT(aResults6); iAddr++)
        {
            if(4 == ipAddressOrigin[iAddr])
            {
                pIpAddress->eIpAddressType = VL_XCHAN_IP_ADDRESS_TYPE_IPV4;
                int iOid = 0; int nAddrLen = aLongIpV6oid[iAddr][11];
                for(iOid = 0; ((iOid < nAddrLen) && (iOid < VL_IP_ADDR_SIZE)); iOid++)
                {
                    pIpAddress->aBytIpAddress[iOid] = (unsigned char)aLongIpV6oid[iAddr][iOid+12];
                }
                inet_ntop(AF_INET,pIpAddress->aBytIpAddress,
                            pIpAddress->szIpAddress,sizeof(pIpAddress->szIpAddress));
                vlXchanGetV6SubnetMaskFromPlen(24, pIpAddress->aBytSubnetMask, sizeof(pIpAddress->aBytSubnetMask));
                inet_ntop(AF_INET,pIpAddress->aBytSubnetMask,
                          pIpAddress->szSubnetMask,sizeof(pIpAddress->szSubnetMask));
                break;
            }
        }
    }
    
    // check if an IPv6 address is available
    memset(ipAddressOrigin, 0, sizeof(ipAddressOrigin));
    int nResults6 = VL_ARRAY_ITEM_COUNT(aResults6);
    VL_SNMP_API_RESULT ret6 = vlSnmpGetEcmObjects((char*)"IP-MIB::ipAddressOrigin.ipv6", &nResults6, aResults6);
    
    if(VL_SNMP_API_RESULT_SUCCESS == ret6)
    {
        for(int iAddr = 0; iAddr < VL_ARRAY_ITEM_COUNT(aResults6); iAddr++)
        {
            if(4 == ipAddressOrigin[iAddr])
            {
                pIpAddress->eIpAddressType = VL_XCHAN_IP_ADDRESS_TYPE_IPV6;
                int iOid = 0; int nAddrLen = aLongIpV6oid[iAddr][11];
                for(iOid = 0; ((iOid < nAddrLen) && (iOid < VL_IPV6_ADDR_SIZE)); iOid++)
                {
                    pIpAddress->aBytIpV6GlobalAddress[iOid] = (unsigned char)aLongIpV6oid[iAddr][iOid+12];
                }
                inet_ntop(AF_INET6,pIpAddress->aBytIpV6GlobalAddress,
                            pIpAddress->szIpV6GlobalAddress,sizeof(pIpAddress->szIpV6GlobalAddress));
                vlXchanGetV6SubnetMaskFromPlen(64, pIpAddress->aBytIpV6SubnetMask, sizeof(pIpAddress->aBytIpV6SubnetMask));
                inet_ntop(AF_INET6,pIpAddress->aBytIpV6SubnetMask,
                          pIpAddress->szIpV6SubnetMask,sizeof(pIpAddress->szIpV6SubnetMask));
                break;
            }
        }
    }

	if((VL_XCHAN_IP_ADDRESS_TYPE_IPV4 == pIpAddress->eIpAddressType)
		&& (NULL != strstr(pIpAddress->szIpAddress,"127.0.0.")))
	{
		 
	    /*
			In mini Diag screen, ECM IP appear as 127.0.0.1 .
		*/
		const char *ip_address = "0.0.0.0";
		pIpAddress->eIpAddressType = VL_XCHAN_IP_ADDRESS_TYPE_NONE;
		strncpy(pIpAddress->szIpAddress, ip_address, sizeof(pIpAddress->szIpAddress));
	}
    
    return ret;
}

/**
 * @brief This function is used to retrieve the mac address using snmp api.
 *
 * @param[out] pMacAddress Struct pointer where MAC address is stored.
 *
 * @retval VL_SNMP_API_RESULT_NULL_PARAM If address of parameter is NULL. Indicates Failed condition.
 * @retval VL_SNMP_API_RESULT_SUCCESS Indicates success to get and update mac address info.
 */
VL_SNMP_API_RESULT vlSnmpEcmGetMacAddress(VL_MAC_ADDRESS_STRUCT * pMacAddress)
{
    if(NULL == pMacAddress) return VL_SNMP_API_RESULT_NULL_PARAM;
    
    VL_ZERO_STRUCT(*pMacAddress);
    VL_SNMP_API_RESULT ret = vlSnmpGetEcmByteArray((char*)"IF-MIB::ifPhysAddress.2", sizeof(pMacAddress->aBytMacAddress), pMacAddress->aBytMacAddress);
    
    if(VL_SNMP_API_RESULT_SUCCESS == ret)
    {
        snprintf(pMacAddress->szMacAddress, sizeof(pMacAddress->szMacAddress), "%02X:%02X:%02X:%02X:%02X:%02X",
                pMacAddress->aBytMacAddress[0], pMacAddress->aBytMacAddress[1],
                pMacAddress->aBytMacAddress[2], pMacAddress->aBytMacAddress[3],
                pMacAddress->aBytMacAddress[4], pMacAddress->aBytMacAddress[5]);
    }
    
    return ret;
}

/**
 * @brief This function is used to get down channel frequency details using snmp api.
 *
 * @param[out] pValue Pointer where frequnecy details are stored.
 *
 * @retval VL_SNMP_API_RESULT_NULL_PARAM If address of parameter is NULL.
 * Indicates Failed condition.
 * @retval VL_SNMP_API_RESULT_SUCCESS Indicates success condition to get frequency details.
 */
VL_SNMP_API_RESULT vlSnmpEcmGetDsFrequency(unsigned long * pValue)
{
    return vlSnmpGetEcmLong((char*)"DOCS-IF-MIB::docsIfDownChannelFrequency.3", pValue);
}

/**
 * @brief This function is used to get down channel power details using snmp api.
 *
 * @param[out] pValue Pointer where channel power details are stored.
 *
 * @retval VL_SNMP_API_RESULT_NULL_PARAM If address of parameter is NULL. Indicates Failed condition.
 * @retval VL_SNMP_API_RESULT_SUCCESS Indicates success to get power details.
 */
VL_SNMP_API_RESULT vlSnmpEcmGetDsPower(unsigned long * pValue)
{
    return vlSnmpGetEcmLong((char*)"DOCS-IF-MIB::docsIfDownChannelPower.3", pValue);
}

/**
 * @brief This function is used to get down channel modulation type like qpsk using snmp api.
 *
 * @param[out] peValue Struct Pointer where channel modulation details are stored.
 *
 * @retval VL_SNMP_API_RESULT_NULL_PARAM If address of parameter is NULL. Indicates Failed condition.
 * @retval VL_SNMP_API_RESULT_SUCCESS Indicates success to get and update modulation info.
 */
VL_SNMP_API_RESULT vlSnmpEcmGetDsModulationType(VL_SNMP_ECM_DS_MODULATION_TYPE * peValue)
{
    return vlSnmpGetEcmLong((char*)"DOCS-IF-MIB::docsIfDownChannelModulation.3", (unsigned long*)peValue);
}

/**
 * @brief This function is used to get down channel width and lock status details
 * using snmp api.
 *
 * @param[out] pValue Pointer where lock status is stored.
 *
 * @retval VL_SNMP_API_RESULT_NULL_PARAM If address of parameter is NULL. Indicates Failed condition.
 * @retval VL_SNMP_API_RESULT_SUCCESS Indicates success to get lock status.
 */
VL_SNMP_API_RESULT vlSnmpEcmGetDsLockStatus(unsigned long * pValue)
{
    unsigned long lFreqWidth = 0;
    unsigned long lPower = 0;
    if(NULL == pValue) return VL_SNMP_API_RESULT_NULL_PARAM;
    
    VL_SNMP_API_RESULT result = VL_SNMP_API_RESULT_SUCCESS;
    if(VL_SNMP_API_RESULT_SUCCESS == (result = vlSnmpGetEcmLong((char*)"DOCS-IF-MIB::docsIfDownChannelWidth.3", &lFreqWidth)))
    {
        if(0 != lFreqWidth)
        {
            if(NULL != pValue) *pValue = 1;
            return result;
        }
    }
    
    if(NULL != pValue) *pValue = 0;
    return result;
}

/**
 * @brief This function is used to get details about types of Up channels using snmp api.
 *
 * @param[out] peValue Struct Pointer where channel type details are stored.
 *
 * @retval VL_SNMP_API_RESULT_NULL_PARAM If address of parameter is NULL. Indicates Failed condition.
 * @retval VL_SNMP_API_RESULT_SUCCESS Indicates success to get and update channel type info.
 */
VL_SNMP_API_RESULT vlSnmpEcmGetUsChannelType(VL_SNMP_ECM_US_CHANNEL_TYPE * peValue)
{
    return vlSnmpGetEcmLong((char*)"DOCS-IF-MIB::docsIfUpChannelType.4", (unsigned long*)peValue);
}

/**
 * @brief This function is used to get details about up channel Modulation used like QPSK etc.
 *
 * @param[out] peValue Struct Pointer where Modulation details are stored.
 *
 * @retval VL_SNMP_API_RESULT_NULL_PARAM If address of parameter is NULL. Indicates Failed condition.
 * @retval VL_SNMP_API_RESULT_SUCCESS Indicates success to get and update modulation details.
 */
VL_SNMP_API_RESULT vlSnmpEcmGetUsModulationType(VL_SNMP_ECM_US_MODULATION_TYPE * peValue)
{
    return vlSnmpGetEcmLong((char*)"DOCS-IF-MIB::docsIfCmStatusModulationType.2", (unsigned long*)peValue);
}

/**
 * @brief This function is used to get details about up channel frequency using snmp api.
 *
 * @param[out] peValue Pointer where frequency details are stored.
 *
 * @retval VL_SNMP_API_RESULT_NULL_PARAM If address of parameter is NULL. Indicates Failed condition.
 * @retval VL_SNMP_API_RESULT_SUCCESS Indicates success to get frequency.
 */
VL_SNMP_API_RESULT vlSnmpEcmGetUsFrequency(unsigned long * pValue)
{
    return vlSnmpGetEcmLong((char*)"DOCS-IF-MIB::docsIfUpChannelFrequency.4", pValue);
}

/**
 * @brief This function is used to get details about up channel power using snmp api.
 *
 * @param[out] pValue Pointer where power details are stored.
 *
 * @retval VL_SNMP_API_RESULT_NULL_PARAM If address of parameter is NULL. Indicates Failed condition.
 * @retval VL_SNMP_API_RESULT_SUCCESS Indicates success to get power status.
 */
VL_SNMP_API_RESULT vlSnmpEcmGetUsPower(unsigned long * pValue)
{
    return vlSnmpGetEcmLong((char*)"DOCS-IF-MIB::docsIfCmStatusTxPower.2", pValue);
}

/**
 * @brief This function is used to get Signal Noise Ratio using snmp api.
 *
 * @param[out] pValue Pointer where SNR details are stored.
 *
 * @retval VL_SNMP_API_RESULT_NULL_PARAM If address of parameter is NULL. Indicates Failed condition.
 * @retval VL_SNMP_API_RESULT_SUCCESS Indicates success condition after getting SNR value.
 */
VL_SNMP_API_RESULT vlSnmpEcmGetSignalNoizeRatio(unsigned long * pValue)
{
    VL_SNMP_API_RESULT result = vlSnmpGetEcmLong((char*)"DOCS-IF3-MIB::docsIf3SignalQualityExtRxMER.3", pValue);
    if(VL_SNMP_API_RESULT_SUCCESS != result)
    {
        result = vlSnmpGetEcmLong((char*)"DOCS-IF-MIB::docsIfSigQSignalNoise.3", pValue);
    }
    return result;
}

/**
 * @brief This function is used to get IP configuration and status like Speed, MTU num etc
 * and fill to its structure.
 * 
 * @param[out] pIpStats Pointer where IP details are stored.
 *
 * @retval VL_SNMP_API_RESULT_NULL_PARAM If address of parameter is NULL. Indicates Failed condition.
 * @retval VL_SNMP_API_RESULT_SUCCESS Indicates success condition.
 */
VL_SNMP_API_RESULT vlSnmpEcmGetIfStats(VL_HOST_IP_STATS * pIpStats)
{
    VL_SNMP_CLIENT_VALUE aResults[] =
    {
        VL_SNMP_CLIENT_MULTI_VAR_ENTRY((char*)"ifMtu.17"             , VL_SNMP_CLIENT_VALUE_TYPE_INTEGER, 1, &(pIpStats->ifMtu            )),
        VL_SNMP_CLIENT_MULTI_VAR_ENTRY((char*)"ifSpeed.17"           , VL_SNMP_CLIENT_VALUE_TYPE_INTEGER, 1, &(pIpStats->ifSpeed          )),
        VL_SNMP_CLIENT_MULTI_VAR_ENTRY((char*)"ifInOctets.17"        , VL_SNMP_CLIENT_VALUE_TYPE_INTEGER, 1, &(pIpStats->ifInOctets       )),
        VL_SNMP_CLIENT_MULTI_VAR_ENTRY((char*)"ifInUcastPkts.17"     , VL_SNMP_CLIENT_VALUE_TYPE_INTEGER, 1, &(pIpStats->ifInUcastPkts    )),
        VL_SNMP_CLIENT_MULTI_VAR_ENTRY((char*)"ifInNUcastPkts.17"    , VL_SNMP_CLIENT_VALUE_TYPE_INTEGER, 1, &(pIpStats->ifInNUcastPkts   )),
        VL_SNMP_CLIENT_MULTI_VAR_ENTRY((char*)"ifInDiscards.17"      , VL_SNMP_CLIENT_VALUE_TYPE_INTEGER, 1, &(pIpStats->ifInDiscards     )),
        VL_SNMP_CLIENT_MULTI_VAR_ENTRY((char*)"ifInErrors.17"        , VL_SNMP_CLIENT_VALUE_TYPE_INTEGER, 1, &(pIpStats->ifInErrors       )),
        VL_SNMP_CLIENT_MULTI_VAR_ENTRY((char*)"ifInUnknownProtos.17" , VL_SNMP_CLIENT_VALUE_TYPE_INTEGER, 1, &(pIpStats->ifInUnknownProtos)),
        VL_SNMP_CLIENT_MULTI_VAR_ENTRY((char*)"ifOutOctets.17"       , VL_SNMP_CLIENT_VALUE_TYPE_INTEGER, 1, &(pIpStats->ifOutOctets      )),
        VL_SNMP_CLIENT_MULTI_VAR_ENTRY((char*)"ifOutUcastPkts.17"    , VL_SNMP_CLIENT_VALUE_TYPE_INTEGER, 1, &(pIpStats->ifOutUcastPkts   )),
        VL_SNMP_CLIENT_MULTI_VAR_ENTRY((char*)"ifOutNUcastPkts.17"   , VL_SNMP_CLIENT_VALUE_TYPE_INTEGER, 1, &(pIpStats->ifOutNUcastPkts  )),
        VL_SNMP_CLIENT_MULTI_VAR_ENTRY((char*)"ifOutDiscards.17"     , VL_SNMP_CLIENT_VALUE_TYPE_INTEGER, 1, &(pIpStats->ifOutDiscards    )),
        VL_SNMP_CLIENT_MULTI_VAR_ENTRY((char*)"ifOutErrors.17"       , VL_SNMP_CLIENT_VALUE_TYPE_INTEGER, 1, &(pIpStats->ifOutErrors      )),
    };
    int nResults = VL_ARRAY_ITEM_COUNT(aResults);
    if(NULL == pIpStats) return VL_SNMP_API_RESULT_NULL_PARAM;
    VL_ZERO_STRUCT(*pIpStats);
    return vlSnmpGetEcmObjects(NULL, &nResults, aResults);
}

/**
 * @brief This function is used to get details about snmp device like Device Role, STP Control,
 * Serial Number, Admin Status, Server Address etc and saves to its structure pointer.
 *
 * @param[out] pStruct Struct pointer details about snmp devices are stored.
 *
 * @retval VL_SNMP_API_RESULT_NULL_PARAM If address of parameter is NULL. Indicates Failed condition.
 * @retval VL_SNMP_API_RESULT_SUCCESS Indicates all entry have been successfully retrieved.
 */
VL_SNMP_API_RESULT vlSnmpEcmDocsDevInfo(VL_SNMP_DOCS_DEV_INFO * pStruct)
{
    VL_SNMP_CLIENT_VALUE aResults[] =
    {
        VL_SNMP_CLIENT_MULTI_VAR_ENTRY((char*)"docsDevRole.0"                       , VL_SNMP_CLIENT_VALUE_TYPE_INTEGER, 1                                                , &(pStruct->docsDevRole                          )),
        VL_SNMP_CLIENT_MULTI_VAR_ENTRY((char*)"docsDevSerialNumber.0"               , VL_SNMP_CLIENT_VALUE_TYPE_BYTE   , sizeof(pStruct->docsDevSerialNumber           ), &(pStruct->docsDevSerialNumber                  )),
        VL_SNMP_CLIENT_MULTI_VAR_ENTRY((char*)"docsDevSTPControl.0"                 , VL_SNMP_CLIENT_VALUE_TYPE_INTEGER, 1                                                , &(pStruct->docsDevSTPControl                    )),
        VL_SNMP_CLIENT_MULTI_VAR_ENTRY((char*)"docsDevIgmpModeControl.0"            , VL_SNMP_CLIENT_VALUE_TYPE_INTEGER, 1                                                , &(pStruct->docsDevIgmpModeControl               )),
        VL_SNMP_CLIENT_MULTI_VAR_ENTRY((char*)"docsDevMaxCpe.0"                     , VL_SNMP_CLIENT_VALUE_TYPE_INTEGER, 1                                                , &(pStruct->docsDevMaxCpe                        )),
        VL_SNMP_CLIENT_MULTI_VAR_ENTRY((char*)"docsDevSwServer.0"                   , VL_SNMP_CLIENT_VALUE_TYPE_BYTE   , sizeof(pStruct->docsDevSwServer               ), &(pStruct->docsDevSwServer                      )),
        VL_SNMP_CLIENT_MULTI_VAR_ENTRY((char*)"docsDevSwFilename.0"                 , VL_SNMP_CLIENT_VALUE_TYPE_BYTE   , sizeof(pStruct->docsDevSwFilename             ), &(pStruct->docsDevSwFilename                    )),
        VL_SNMP_CLIENT_MULTI_VAR_ENTRY((char*)"docsDevSwAdminStatus.0"              , VL_SNMP_CLIENT_VALUE_TYPE_INTEGER, 1                                                , &(pStruct->docsDevSwAdminStatus                 )),
        VL_SNMP_CLIENT_MULTI_VAR_ENTRY((char*)"docsDevSwOperStatus.0"               , VL_SNMP_CLIENT_VALUE_TYPE_INTEGER, 1                                                , &(pStruct->docsDevSwOperStatus                  )),
        VL_SNMP_CLIENT_MULTI_VAR_ENTRY((char*)"docsDevSwCurrentVers.0"              , VL_SNMP_CLIENT_VALUE_TYPE_BYTE   , sizeof(pStruct->docsDevSwCurrentVers          ), &(pStruct->docsDevSwCurrentVers                 )),
        VL_SNMP_CLIENT_MULTI_VAR_ENTRY((char*)"docsDevSwServerAddressType.0"        , VL_SNMP_CLIENT_VALUE_TYPE_INTEGER, 1                                                , &(pStruct->docsDevSwServerAddressType           )),
        VL_SNMP_CLIENT_MULTI_VAR_ENTRY((char*)"docsDevSwServerAddress.0"            , VL_SNMP_CLIENT_VALUE_TYPE_BYTE   , sizeof(pStruct->docsDevSwServerAddress        ), &(pStruct->docsDevSwServerAddress               )),
        VL_SNMP_CLIENT_MULTI_VAR_ENTRY((char*)"docsDevSwServerTransportProtocol.0"  , VL_SNMP_CLIENT_VALUE_TYPE_INTEGER, 1                                                , &(pStruct->docsDevSwServerTransportProtocol     )),
        VL_SNMP_CLIENT_MULTI_VAR_ENTRY((char*)"docsDevServerBootState.0"            , VL_SNMP_CLIENT_VALUE_TYPE_INTEGER, 1                                                , &(pStruct->docsDevServerBootState               )),
        VL_SNMP_CLIENT_MULTI_VAR_ENTRY((char*)"docsDevServerDhcp.0"                 , VL_SNMP_CLIENT_VALUE_TYPE_BYTE   , sizeof(pStruct->docsDevServerDhcp             ), &(pStruct->docsDevServerDhcp                    )),
        VL_SNMP_CLIENT_MULTI_VAR_ENTRY((char*)"docsDevServerTime.0"                 , VL_SNMP_CLIENT_VALUE_TYPE_BYTE   , sizeof(pStruct->docsDevServerTime             ), &(pStruct->docsDevServerTime                    )),
        VL_SNMP_CLIENT_MULTI_VAR_ENTRY((char*)"docsDevServerTftp.0"                 , VL_SNMP_CLIENT_VALUE_TYPE_BYTE   , sizeof(pStruct->docsDevServerTftp             ), &(pStruct->docsDevServerTftp                    )),
        VL_SNMP_CLIENT_MULTI_VAR_ENTRY((char*)"docsDevServerConfigFile.0"           , VL_SNMP_CLIENT_VALUE_TYPE_BYTE   , sizeof(pStruct->docsDevServerConfigFile       ), &(pStruct->docsDevServerConfigFile              )),
        VL_SNMP_CLIENT_MULTI_VAR_ENTRY((char*)"docsDevServerDhcpAddressType.0"      , VL_SNMP_CLIENT_VALUE_TYPE_INTEGER, 1                                                , &(pStruct->docsDevServerDhcpAddressType         )),
        VL_SNMP_CLIENT_MULTI_VAR_ENTRY((char*)"docsDevServerDhcpAddress.0"          , VL_SNMP_CLIENT_VALUE_TYPE_BYTE   , sizeof(pStruct->docsDevServerDhcpAddress      ), &(pStruct->docsDevServerDhcpAddress             )),
        VL_SNMP_CLIENT_MULTI_VAR_ENTRY((char*)"docsDevServerTimeAddressType.0"      , VL_SNMP_CLIENT_VALUE_TYPE_INTEGER, 1                                                , &(pStruct->docsDevServerTimeAddressType         )),
        VL_SNMP_CLIENT_MULTI_VAR_ENTRY((char*)"docsDevServerTimeAddress.0"          , VL_SNMP_CLIENT_VALUE_TYPE_BYTE   , sizeof(pStruct->docsDevServerTimeAddress       ), &(pStruct->docsDevServerTimeAddress             )),
        VL_SNMP_CLIENT_MULTI_VAR_ENTRY((char*)"docsDevServerConfigTftpAddressType.0", VL_SNMP_CLIENT_VALUE_TYPE_INTEGER, 1                                                , &(pStruct->docsDevServerConfigTftpAddressType   )),
        VL_SNMP_CLIENT_MULTI_VAR_ENTRY((char*)"docsDevServerConfigTftpAddress.0"    , VL_SNMP_CLIENT_VALUE_TYPE_BYTE   , sizeof(pStruct->docsDevServerConfigTftpAddress), &(pStruct->docsDevServerConfigTftpAddress       )),
        VL_SNMP_CLIENT_MULTI_VAR_ENTRY((char*)"docsDevDateTime.0"                   , VL_SNMP_CLIENT_VALUE_TYPE_BYTE   , sizeof(pStruct->docsDevDateTime)               , &(pStruct->docsDevDateTime       )),
    };
    int nResults = VL_ARRAY_ITEM_COUNT(aResults);
    if(NULL == pStruct) return VL_SNMP_API_RESULT_NULL_PARAM;
    VL_ZERO_STRUCT(*pStruct);
    return vlSnmpGetEcmObjects(NULL, &nResults, aResults);
}

