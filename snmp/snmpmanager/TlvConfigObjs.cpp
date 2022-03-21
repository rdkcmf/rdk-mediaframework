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



#include "ip_types.h"
#include "TlvConfigObjs.h"
#include "vlMutex.h"
#include "Tlvevent.h"
#include "SnmpIORM.h"
#include "vlpluginapp_haldsgapi.h"
#include "vlSnmpClient.h"
#include "utilityMacros.h"
//#include "cThreadPool.h"
#include <arpa/inet.h>
#include "snmpTargetAddrTable.h"
#include "snmpNotifyTable.h"
#include "snmpTargetParamsTable.h"
#define vlStrCpy(pDest, pSrc, nDestCapacity)            \
            strcpy(pDest, pSrc)

#define vlMemCpy(pDest, pSrc, nCount, nDestCapacity)            \
            memcpy(pDest, pSrc, nCount)

#define VL_ZERO_MEMORY(x) \
            memset(&x,0,sizeof(x))  

static vlMutex & vlg_TlvEventDblock = TlvConfig::vlGetTlvEventDbLock();
#ifdef AUTO_LOCKING

static void auto_lock(rmf_osal_Mutex *mutex)
{

               if(!mutex) 
			   	RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","\n\n\n\n %s: Mutex is NULL \n\n\n", __FUNCTION__);
		 rmf_osal_mutexAcquire(*mutex);
}

static void auto_unlock(rmf_osal_Mutex *mutex)
{
         if(mutex)
		 rmf_osal_mutexRelease(*mutex);
}
#endif
#ifdef __cplusplus
extern "C" {
#endif
#include"docsDevEventTable_enums.h"
#include"snmpAccessInclude.h"
//extern snmpV1V2CommunityTableEventhandling(char *tlvparseCm);

#ifdef __cplusplus
}
#endif
/** Listens to TVL217config file and dispatches them for configureation */
#include "cardMibAccessProxyServer.h"

//#undef SNMP_DEBUGPRINT
//#define SNMP_DEBUGPRINT(a,args...) fprintf( stderr, "%s:%d:"#a"\n", __FUNCTION__,__LINE__, ##args)

VL_TLV_SNMP_VAR_BIND::VL_TLV_SNMP_VAR_BIND()
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    eVarBindType = 0;
//auto_unlock(&vlg_TlvEventDblock);
}

VL_TLV_SNMP_STRING_VAR_BIND::VL_TLV_SNMP_STRING_VAR_BIND()
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    eVarBindType = 0;
//auto_unlock(&vlg_TlvEventDblock);

}

VL_TLV_TRANSPORT_ADDR_AND_MASK::VL_TLV_TRANSPORT_ADDR_AND_MASK()
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    iAccessViewTlv = 0;
    iTlvSubEntry = 0;
    bIsPermanent = false;
//auto_unlock(&vlg_TlvEventDblock);

}

string VL_TLV_TRANSPORT_ADDR_AND_MASK::getAddrString(VL_TLV_217_IP_ADDR & rAddress)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    char szIpAddress[VL_IPV6_ADDR_SIZE];
    vlStrCpy(szIpAddress, "", sizeof(szIpAddress));
    
    switch(rAddress.size())
    {
        case VL_IP_ADDR_SIZE:
        case VL_IP_ADDR_SIZE+2:
        {
            inet_ntop(AF_INET, &(*(rAddress.begin())), szIpAddress, sizeof(szIpAddress));
        }
        break;
        
        case VL_IPV6_ADDR_SIZE:
        case VL_IPV6_ADDR_SIZE+2:
        {
            inet_ntop(AF_INET6, &(*(rAddress.begin())), szIpAddress, sizeof(szIpAddress));
        }
        break;
    }

    //auto_unlock(&vlg_TlvEventDblock);    
    return string(szIpAddress);
}

string VL_TLV_TRANSPORT_ADDR_AND_MASK::getAddressString()
{VL_AUTO_LOCK(vlg_TlvEventDblock);
    string temp;
    temp = getAddrString(TpAddress);
    //auto_unlock(&vlg_TlvEventDblock);     	
    return temp;
}

string VL_TLV_TRANSPORT_ADDR_AND_MASK::getAddressStringHex()
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    string temp;
     temp =  getHexString(TpAddress);
    //auto_unlock(&vlg_TlvEventDblock);     	
    return temp;
}

string VL_TLV_TRANSPORT_ADDR_AND_MASK::getMaskStringHex()
{VL_AUTO_LOCK(vlg_TlvEventDblock);
    string temp;
     temp =   getHexString(TpMask);
    //auto_unlock(&vlg_TlvEventDblock);     	
    return temp;
}

string VL_TLV_TRANSPORT_ADDR_AND_MASK::getHexString(VL_TLV_217_IP_ADDR & rAddress)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    char szByte[10];
    string strIpAddress;
    int nLength = rAddress.size();
    for(int i = 0; i < nLength; i++)
    {
        snprintf(szByte, sizeof(szByte), "%02X", rAddress[i]);
        strIpAddress += szByte;
        if(i < nLength - 1) strIpAddress += " ";
    }
    //auto_unlock(&vlg_TlvEventDblock);        
    return strIpAddress;
}

int VL_TLV_TRANSPORT_ADDR_AND_MASK::getPlenMask()
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    int nPlen = 0;
    int nLength = 0;
    switch(TpMask.size())
    {
        case VL_IP_ADDR_SIZE:
        case VL_IPV6_ADDR_SIZE:
        {
            nLength = TpMask.size();
        }
        break;
        
        case VL_IP_ADDR_SIZE+2:
        case VL_IPV6_ADDR_SIZE+2:
        {
            nLength = TpMask.size()-2;
        }
        break;
    }
    for(int i = 0; i < nLength; i++)
    {
        int nByte = TpMask[i];
        for(int j = 7; j >= 0; j--)
        {
            if((nByte>>j)&0x1) nPlen = (i*8)+(8-j);
        }
    }

    //auto_unlock(&vlg_TlvEventDblock);        
    return nPlen;
}

bool VL_TLV_TRANSPORT_ADDR_AND_MASK::operator == (const VL_TLV_TRANSPORT_ADDR_AND_MASK & rRight) const
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    if((TpAddress == rRight.TpAddress) &&
        (TpMask   == rRight.TpMask))
    {
       //auto_unlock(&vlg_TlvEventDblock);    
        return true;
    }
    //auto_unlock(&vlg_TlvEventDblock);    
    return false;
}

string VL_TLV_TRANSPORT_ADDR_AND_MASK::getCom2SecConfig(int iConfig, string strCommunity)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    char szConfig[API_CHRMAX];
    string strIpAddress = getAddressString();
    // Added support for com2sec6 for IPv6 in addtion to com2sec for IPv4.
    snprintf(szConfig, sizeof(szConfig), "com2sec%s   @STBconfig_%d  %s/%d  %s \n", ((TpAddress.size() >= VL_IPV6_ADDR_SIZE) ? "6":""), iConfig, strIpAddress.c_str(), getPlenMask(), strCommunity.c_str() );
    //auto_unlock(&vlg_TlvEventDblock);    
    return szConfig;
}

VL_TLV_SNMP_STRING_VAR_BIND_LIST VL_TLV_TRANSPORT_ADDR_AND_MASK::getGroupRowVarBinds(int nVersion)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    VL_TLV_SNMP_STRING_VAR_BIND_LIST varBindList;
    VL_TLV_SNMP_STRING_VAR_BIND varBind;
    
    char szName[API_CHRMAX];
    char szValue[API_CHRMAX];
    
    snprintf(szName, sizeof(szName), ".%d.\"@STBconfig_%d\"", nVersion, iAccessViewTlv);
    
    varBind.strOid = "SNMP-VIEW-BASED-ACM-MIB::vacmSecurityToGroupStatus";
    varBind.strOid += szName;
    varBind.eVarBindType = 'i';
    varBind.strValue = "4";
    varBindList.push_back(varBind);
    
    snprintf(szValue, sizeof(szValue), "\"@STBconfigV%d_%d\"", nVersion, iAccessViewTlv);
    varBind.strOid = "SNMP-VIEW-BASED-ACM-MIB::vacmGroupName";
    varBind.strOid += szName;
    varBind.eVarBindType = 's';
    varBind.strValue = szValue;
    varBindList.push_back(varBind);
    
    varBind.strOid = "SNMP-VIEW-BASED-ACM-MIB::vacmSecurityToGroupStorageType";
    varBind.strOid += szName;
    varBind.eVarBindType = 'i';
    varBind.strValue = "2";
    varBindList.push_back(varBind);
	
    //auto_unlock(&vlg_TlvEventDblock);        
    return varBindList;
}

string VL_TLV_TRANSPORT_ADDR_AND_MASK::getVacmGroupConfig(int nVersion, int iConfig)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    char szConfig[API_CHRMAX];
    snprintf(szConfig, sizeof(szConfig), "vacmGroup 1 2 %d @STBconfig_%d @STBconfigV%d_%d", nVersion, iConfig, nVersion, iConfig);
    //auto_unlock(&vlg_TlvEventDblock);    
    return szConfig;
}

v3AccessviewList_t::v3AccessviewList_t()
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    v3AccessViewType = VL_TLV_217_ACCESS_INCLUDED;
    //auto_unlock(&vlg_TlvEventDblock);    
}

bool v3AccessviewList_t::operator == (const v3AccessviewList_t & rRight) const
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    if((strAccessViewName == rRight.strAccessViewName) &&
        (v3AccessViewSubtree == rRight.v3AccessViewSubtree) &&
        (v3AccessViewMask == rRight.v3AccessViewMask) &&
        (v3AccessViewType == rRight.v3AccessViewType))
    {
       //auto_unlock(&vlg_TlvEventDblock);    
        return true;
    }
    //auto_unlock(&vlg_TlvEventDblock);    
    return false;
}

string v3AccessviewList_t::getVacmViewConfig(int nVersion, int iConfig)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    char subtreeOID[API_CHRMAX];
    char subtreMax[API_CHRMAX];
    
    vlSnmpConvertOidToString(&(*(v3AccessViewSubtree.begin())),
                                 v3AccessViewSubtree.size(),
                                 subtreeOID, sizeof(subtreeOID));
    
    vlStrCpy(subtreMax, "", sizeof(subtreMax));
    
    if(v3AccessViewMask.size() > 0)
    {
        vlStrCpy(subtreMax, "0x", sizeof(subtreMax));
        for(int subtre = 0; subtre < v3AccessViewMask.size(); subtre++)
        {
            snprintf(subtreMax+ (strlen(subtreMax)), sizeof(subtreMax), "%02x",v3AccessViewMask[subtre]);
            if(subtre < v3AccessViewMask.size()-1)
            {
                snprintf(subtreMax+ strlen(subtreMax), sizeof(subtreMax), ":"); //to eliminate the last ":"
            }
            SNMP_DEBUGPRINT("\n SUB-MAX-OID %d \n ",v3AccessViewMask[subtre]);
        }
    }
    
    char szConfig[API_CHRMAX];
    snprintf(szConfig, sizeof(szConfig), "vacmView 1 2 %d %s %s %s", v3AccessViewType,
                      strAccessViewName.c_str(),
                      subtreeOID,
                      subtreMax);
    //auto_unlock(&vlg_TlvEventDblock);    
    return szConfig;
}

string v3AccessviewList_t::getVacmAccessConfig(int nVersion, int iConfig)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    char szConfig[API_CHRMAX];
    snprintf(szConfig, sizeof(szConfig), "vacmAccess 1 2 %d 1 1 @STBconfigV%d_%d \"\" %s %s %s", nVersion, nVersion, iConfig,
                      strAccessViewName.c_str(),
                      "",
                      "");
    //auto_unlock(&vlg_TlvEventDblock);    
    return szConfig;
}

TLV53_V1V2_CONFIG::TLV53_V1V2_CONFIG()
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    eAccessViewType = VL_TLV_217_ACCESS_READ_ONLY;
    //auto_unlock(&vlg_TlvEventDblock);    
}

bool TLV53_V1V2_CONFIG::operator == (const TLV53_V1V2_CONFIG & rRight) const
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    if((strCommunity == rRight.strCommunity) &&
        (strAccessViewName == rRight.strAccessViewName) &&
        (eAccessViewType == rRight.eAccessViewType) &&
        (AVTransportAddressList == rRight.AVTransportAddressList))
    {
       //auto_unlock(&vlg_TlvEventDblock);    
        return true;
    }
    //auto_unlock(&vlg_TlvEventDblock);    
    return false;
}

VL_TLV_SNMP_STRING_VAR_BIND_LIST TLV53_V1V2_CONFIG::getViewVarBinds(int nVersion, int iConfig)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    VL_TLV_SNMP_STRING_VAR_BIND_LIST varBindList;
    VL_TLV_SNMP_STRING_VAR_BIND varBind;
    
    char szName[API_CHRMAX];
    char szValue[API_CHRMAX];
    
    snprintf(szName, sizeof(szName), ".\"%s\".3.1.3.6", strAccessViewName.c_str());
    
    varBind.strOid = "SNMP-VIEW-BASED-ACM-MIB::vacmViewTreeFamilyStatus";
    varBind.strOid += szName;
    varBind.eVarBindType = 'i';
    varBind.strValue = "4";
    varBindList.push_back(varBind);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "%s\n", varBind.strOid.c_str());
    
    snprintf(szValue, sizeof(szValue), "%d", 1);
    varBind.strOid = "SNMP-VIEW-BASED-ACM-MIB::vacmViewTreeFamilyType";
    varBind.strOid += szName;
    varBind.eVarBindType = 'i';
    varBind.strValue = szValue;
    varBindList.push_back(varBind);
    
    varBind.strOid = "SNMP-VIEW-BASED-ACM-MIB::vacmViewTreeFamilyStorageType";
    varBind.strOid += szName;
    varBind.eVarBindType = 'i';
    varBind.strValue = "2";
    varBindList.push_back(varBind);

    //auto_unlock(&vlg_TlvEventDblock);        
    return varBindList;
}

string TLV53_V1V2_CONFIG::getVacmViewConfig(int nVersion, int iConfig)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    char szConfig[API_CHRMAX];
    snprintf(szConfig, sizeof(szConfig), "vacmView 1 2 %d %s %s %s", 1,
                      strAccessViewName.c_str(),
                      ".1.3.6",
                      "");

    //auto_unlock(&vlg_TlvEventDblock);    
    return szConfig;
}

VL_TLV_SNMP_STRING_VAR_BIND_LIST TLV53_V1V2_CONFIG::getAccessVarBinds(int nVersion, int iConfig)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    VL_TLV_SNMP_STRING_VAR_BIND_LIST varBindList;
    VL_TLV_SNMP_STRING_VAR_BIND varBind;
    
    char szName[API_CHRMAX];
    
    snprintf(szName, sizeof(szName), ".\"@STBconfigV%d_%d\".\"\".%d.1", nVersion, iConfig, nVersion);
    
    varBind.strOid = "SNMP-VIEW-BASED-ACM-MIB::vacmAccessStatus";
    varBind.strOid += szName;
    varBind.eVarBindType = 'i';
    varBind.strValue = "4";
    varBindList.push_back(varBind);
    
    varBind.strOid = "SNMP-VIEW-BASED-ACM-MIB::vacmAccessContextMatch";
    varBind.strOid += szName;
    varBind.eVarBindType = 'i';
    varBind.strValue = "1";
    varBindList.push_back(varBind);
    
    varBind.strOid = "SNMP-VIEW-BASED-ACM-MIB::vacmAccessReadViewName";
    varBind.strOid += szName;
    varBind.eVarBindType = 's';
    varBind.strValue = strAccessViewName;
    varBindList.push_back(varBind);
    
    switch(eAccessViewType)
    {
        case VL_TLV_217_ACCESS_READ_WRITE:
        {
            varBind.strOid = "SNMP-VIEW-BASED-ACM-MIB::vacmAccessWriteViewName";
            varBind.strOid += szName;
            varBind.eVarBindType = 's';
            varBind.strValue = strAccessViewName;
            varBindList.push_back(varBind);
        }
        break;
    }
    /*
    varBind.strOid = "SNMP-VIEW-BASED-ACM-MIB::vacmAccessNotifyViewName";
    varBind.strOid += szName;
    varBind.eVarBindType = 's';
    varBind.strValue = "";
    varBindList.push_back(varBind);
    */
    varBind.strOid = "SNMP-VIEW-BASED-ACM-MIB::vacmAccessStorageType";
    varBind.strOid += szName;
    varBind.eVarBindType = 'i';
    varBind.strValue = "2";
    varBindList.push_back(varBind);

    //auto_unlock(&vlg_TlvEventDblock);        
    return varBindList;
}

string TLV53_V1V2_CONFIG::getVacmAccessConfig(int nVersion, int iConfig)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    char szConfig[API_CHRMAX];
    snprintf(szConfig, sizeof(szConfig), "vacmAccess 1 2 %d 1 1 @STBconfigV%d_%d \"\" %s %s %s", nVersion, nVersion, iConfig,
            strAccessViewName.c_str(),
            ((VL_TLV_217_ACCESS_READ_WRITE==eAccessViewType)? strAccessViewName.c_str():""),
            "");

    //auto_unlock(&vlg_TlvEventDblock);    
    return szConfig;
}

v3NotificationReceiver_t::v3NotificationReceiver_t()
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    eV3TrapType = 0;
    nV3UdpPortNumber = 162;
    nV3Timeout = 1500;
    nV3Retries = 3;

    //auto_unlock(&vlg_TlvEventDblock);    
}

bool v3NotificationReceiver_t::operator == (const v3NotificationReceiver_t & rRight) const
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    if( (v3ipV4address == rRight.v3ipV4address) &&
        (nV3UdpPortNumber == rRight.nV3UdpPortNumber) &&
        (eV3TrapType == rRight.eV3TrapType) &&
        (anV3TrapTypes == rRight.anV3TrapTypes) &&
        (nV3Timeout == rRight.nV3Timeout) &&
        (nV3Retries == rRight.nV3Retries) &&
        (v3FilterOID == rRight.v3FilterOID) &&
        (v3securityName == rRight.v3securityName) &&
        (v3ipV6address == rRight.v3ipV6address))
    {
        //auto_unlock(&vlg_TlvEventDblock);    
        return true;
    }

    //auto_unlock(&vlg_TlvEventDblock);    
    return false;
}

VL_TLV_SNMP_STRING_VAR_BIND_LIST v3NotificationReceiver_t::getNotifyTableRowVarBinds(int eType)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    VL_TLV_SNMP_STRING_VAR_BIND_LIST varBindList;
    VL_TLV_SNMP_STRING_VAR_BIND varBind;
    
    char szName[API_CHRMAX];
    vlStrCpy(szName, "", sizeof(szName));
    switch(eType)
    {
        case SNMPNOTIFYTYPE_TRAP:
        {
            vlStrCpy(szName, ".'@STBnotifyconfig_trap'", sizeof(szName));
        }
        break;
        
        case SNMPNOTIFYTYPE_INFORM:
        {
            vlStrCpy(szName, ".'@STBnotifyconfig_inform'", sizeof(szName));
        }
        break;
    }
    
    varBind.strOid = "SNMP-NOTIFICATION-MIB::snmpNotifyRowStatus";
    varBind.strOid += szName;
    varBind.eVarBindType = 'i';
    varBind.strValue = "4";
    varBindList.push_back(varBind);
    
    varBind.strOid = "SNMP-NOTIFICATION-MIB::snmpNotifyTag";
    varBind.strOid += szName;
    varBind.eVarBindType = 's';
    varBind.strValue = "@STBnotifyconfig_";
    varBind.strValue += ((SNMPNOTIFYTYPE_TRAP==eType)?"trap":"inform");
    varBindList.push_back(varBind);
    
    varBind.strOid = "SNMP-NOTIFICATION-MIB::snmpNotifyType";
    varBind.strOid += szName;
    varBind.eVarBindType = 'i';
    varBind.strValue = ((SNMPNOTIFYTYPE_TRAP==eType)?"1":"2");
    varBindList.push_back(varBind);
    
    varBind.strOid = "SNMP-NOTIFICATION-MIB::snmpNotifyStorageType";
    varBind.strOid += szName;
    varBind.eVarBindType = 'i';
    varBind.strValue = "2";
    varBindList.push_back(varBind);

    //auto_unlock(&vlg_TlvEventDblock);        
    return varBindList;
}

string v3NotificationReceiver_t::getVacmGroupConfig(int nVersion)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    char szConfig[API_CHRMAX];
    snprintf(szConfig, sizeof(szConfig), "vacmGroup 1 2 %d @STBnotifyconfig @STBnotifyconfigV%d", nVersion, nVersion);

    //auto_unlock(&vlg_TlvEventDblock);    
    return szConfig;
}

string v3NotificationReceiver_t::getVacmViewConfig(int nVersion)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    char szConfig[API_CHRMAX];
    snprintf(szConfig, sizeof(szConfig), "vacmView 1 2 %d %s %s %s", 1,
                      "@STBnotifyconfig",
                      "1.3",
                      "");

    //auto_unlock(&vlg_TlvEventDblock);    
    return szConfig;
}

string v3NotificationReceiver_t::getVacmAccessConfig(int nVersion)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    char szConfig[API_CHRMAX];
    snprintf(szConfig, sizeof(szConfig), "vacmAccess 1 2 %d 1 1 @STBnotifyconfigV%d \"\" %s %s %s", nVersion, nVersion,
                      "\"\"",
                      "\"\"",
                      "@STBnotifyconfig");

    //auto_unlock(&vlg_TlvEventDblock);    
    return szConfig;
}
