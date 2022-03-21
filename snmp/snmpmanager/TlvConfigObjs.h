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
* \file TlvConfigObjs.h
*/

#ifndef TLV217_CONFIG_OBJS_H
#define TLV217_CONFIG_OBJS_H

//#include "pfckernel.h"
//#include "pfccondition.h"
//#include "cmThreadBase.h"
//#include "pfctimer.h"
//#include "pfceventmanager.h"
//#include "pfceventqueue.h"
//#include "core_events.h"

#include "dsg_types.h" //TVL parse
//#include "vlSnmpTLVparse.h" //TLV217 parsing
#include "bufParseUtilities.h"//TLV217 parsing
//#include"cMsgQ.h"//TLV217 parsing
#include <vector>
#undef min
#undef max
#include <string>
//#include "vlMemCpy.h"
#include "utilityMacros.h"


using namespace std;

/** Listens to TVL217config file and dispatches them for configureation */
/*oid header starts with 48 and oid tag is 6 and oid-value-tag is 4(string)*/
#define TLV11OID_HEADER 48
#define TLV11_OID 6
#define TLV11OID_VALUE 4

// Top level type codes specific to STB configuration file.
typedef enum
{
    VLNONTag = 0,
    //TPad = 0,

    TSnmpIpModeControl     = 1,
    VLVendorId     = 8,
    VLSnmpMibObject = 11,
    VLSNMPv3NotificationReceiver = 38,
    VLSNMPv3NotificationReceiverIPv4Address         = 1,
    VLSNMPv3NotificationReceiverUDPPortNumber       = 2,
    VLSNMPv3NotificationReceiverTrapType            = 3,
    VLSNMPv3NotificationReceiverTimeout             = 4,
    VLSNMPv3NotificationReceiverRetries             = 5,
    VLSNMPv3NotificationReceiverFilteringParameters = 6,
    VLSNMPv3NotificationReceiverSecurityName        = 7,
    VLSNMPv3NotificationReceiverIPv6Address         = 8,
    VLVendorSpecific = 43,
    TSnmpV1V2Coexistence = 53,
    TSnmpV1V2CommunityName = 1,
    TSnmpV1V2Transport = 2,
    TSnmpV1V2TransportAddress = 1,
    TSnmpV1V2TransportAddressMask = 2,
    TSnmpV1V2ViewType = 3,
    TSnmpV1V2ViewName = 4,
    TSnmpV3AccessView = 54,
    TSnmpV3AccessViewName = 1,
    TSnmpV3AccessViewSubtree = 2,
    TSnmpV3AccessViewMask = 3,
    TSnmpV3AccessViewType = 4,
    
    TSnmpTlv217 = 217

} vl217ConfigSpecificTypeCodes;

typedef enum _VL_TLV_217_ACCESS_TYPE
{
    VL_TLV_217_ACCESS_READ_ONLY  = 1,
    VL_TLV_217_ACCESS_READ_WRITE = 2,
    VL_TLV_217_ACCESS_INCLUDED   = 1,
    VL_TLV_217_ACCESS_EXCLUDED   = 2,
    
} VL_TLV_217_ACCESS_TYPE;

typedef enum _VL_TLV_217_NOTIFY_TYPE
{
    VL_TLV_217_NOTIFY_TRAP_V1           = 1,
    VL_TLV_217_NOTIFY_TRAP_V2C          = 2,
    VL_TLV_217_NOTIFY_INFORM_V2C        = 3,
    VL_TLV_217_NOTIFY_TRAP_V2C_IN_V3    = 4,
    VL_TLV_217_NOTIFY_INFORM_V2C_IN_V3  = 5,
    
} VL_TLV_217_NOTIFY_TYPE;

#define MAX_TLV_217_SIZE     256
typedef struct tag_VL_tlvField
{
    int           numBytes;
    unsigned char acollectBytes[MAX_TLV_217_SIZE];
}VL_tlv217Field;

typedef vector<unsigned char> VL_TLV_217_FIELD_DATA;
typedef vector<unsigned char> VL_TLV_217_IP_ADDR;
typedef vector<unsigned char> VL_TLV_217_ASN_VALUE;
typedef vector<int>           VL_TLV_217_OID;

struct VL_TLV_SNMP_VAR_BIND
{
    VL_TLV_SNMP_VAR_BIND();
    
    int                     eVarBindType;
    VL_TLV_217_OID          OIDmib;
    VL_TLV_217_ASN_VALUE    OIDvalue;
    
};

struct VL_TLV_SNMP_STRING_VAR_BIND
{
    VL_TLV_SNMP_STRING_VAR_BIND();
    
    int    eVarBindType;
    string strOid;
    string strValue;
};

typedef vector<VL_TLV_SNMP_STRING_VAR_BIND> VL_TLV_SNMP_STRING_VAR_BIND_LIST;

struct VL_TLV_TRANSPORT_ADDR_AND_MASK
{
//53.2
    VL_TLV_TRANSPORT_ADDR_AND_MASK();
    VL_TLV_217_IP_ADDR TpAddress;
    VL_TLV_217_IP_ADDR TpMask;
    string getAddressString();
    string getAddressStringHex();
    string getMaskStringHex();
    static string getHexString(VL_TLV_217_IP_ADDR & rAddress);
    static string getAddrString(VL_TLV_217_IP_ADDR & rAddress);
    int getPlenMask();
    int iAccessViewTlv;
    int iTlvSubEntry;
    bool bIsPermanent;
    string strPrefix;
    bool operator == (const VL_TLV_TRANSPORT_ADDR_AND_MASK & rRight) const;
    
    VL_TLV_SNMP_STRING_VAR_BIND_LIST getGroupRowVarBinds(int nVersion);
    
    string getCom2SecConfig(int iConfig, string strCommunity);
    string getVacmGroupConfig(int nVersion, int iConfig);
};

struct v3AccessviewList_t
{
    v3AccessviewList_t();
    //54.4
    string strAccessViewName;
    VL_TLV_217_OID v3AccessViewSubtree;
    vector<int> v3AccessViewMask;
    int v3AccessViewType;
    bool operator == (const v3AccessviewList_t & rRight) const;
    
    string getVacmViewConfig(int nVersion, int iConfig);
    string getVacmAccessConfig(int nVersion, int iConfig);
};

struct v3NotificationReceiver_t
{
    v3NotificationReceiver_t();
    //54.4
    VL_TLV_217_IP_ADDR    v3ipV4address;
    int                   nV3UdpPortNumber;
    int                   eV3TrapType;
    vector<int>           anV3TrapTypes;
    int                   nV3Timeout;
    int                   nV3Retries;
    VL_TLV_217_OID        v3FilterOID;
    string                v3securityName;
    VL_TLV_217_IP_ADDR    v3ipV6address;
    bool operator == (const v3NotificationReceiver_t & rRight) const;
    
    static string getVacmGroupConfig(int nVersion);
    static string getVacmViewConfig(int nVersion);
    static string getVacmAccessConfig(int nVersion);
    
    static VL_TLV_SNMP_STRING_VAR_BIND_LIST getNotifyTableRowVarBinds(int eType);
};

struct TLV53_V1V2_CONFIG
{
    TLV53_V1V2_CONFIG();
    //53.1
    string strCommunity;
    //53.2
    vector<VL_TLV_TRANSPORT_ADDR_AND_MASK> AVTransportAddressList;
    //53.3
    string strAccessViewName;
    int    eAccessViewType;
    
    bool operator == (const TLV53_V1V2_CONFIG & rRight) const;
    
    VL_TLV_SNMP_STRING_VAR_BIND_LIST getViewVarBinds(int nVersion, int iConfig);
    VL_TLV_SNMP_STRING_VAR_BIND_LIST getAccessVarBinds(int nVersion, int iConfig);
    string getVacmViewConfig(int nVersion, int iConfig);
    string getVacmAccessConfig(int nVersion, int iConfig);
};

typedef enum _VL_SNMP_ACCESS_CONFIG
{
    VL_SNMP_ACCESS_CONFIG_LOCAL_PUBLIC  = 0,
    VL_SNMP_ACCESS_CONFIG_LOCAL_PRIVATE = 1,
    VL_SNMP_ACCESS_CONFIG_POD_PUBLIC    = 2,
    VL_SNMP_ACCESS_CONFIG_POD_PRIVATE   = 3,
    
} VL_SNMP_ACCESS_CONFIG;

#endif // TLV217_CONFIG_OBJS_H
