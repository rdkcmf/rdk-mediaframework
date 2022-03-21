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

 
/*-----------------------------------------------------------------*/
#ifndef __VL_IP_TYPES_H__
#define __VL_IP_TYPES_H__
/*-----------------------------------------------------------------*/
#include "rmf_osal_thread.h"

#define VL_MAC_ADDR_SIZE        6
#define VL_IP_ADDR_SIZE         4
#define VL_IPV6_ADDR_SIZE       16
#define VL_IF_NAME_SIZE         128
#define VL_DNS_ADDR_SIZE        256
#define VL_IP_MASK_SIZE         4
#define VL_IP_ADDR_STR_SIZE     (VL_IP_ADDR_SIZE *5)
#define VL_IPV6_ADDR_STR_SIZE   (VL_IPV6_ADDR_SIZE *5)
#define VL_MAC_ADDR_STR_SIZE    (VL_MAC_ADDR_SIZE*5)

#define VL_MAX_IP_MULTICAST_FLOWS   16
#define VL_MAX_SOCKET_FLOWS         16

#define VL_XCHAN_IP_MAX_PDU_SIZE        1500

#define VL_XCHAN_SOCKET_MAX_PDU_SIZE    1400
/*-----------------------------------------------------------------*/

typedef enum tag_VL_OOB_COMMAND
{
    VL_OOB_COMMAND_OOB_OFF        = 0x00,
    VL_OOB_COMMAND_OOB_ON         = 0x01,
    VL_OOB_COMMAND_INIT_IF        = 0x02,
    VL_OOB_COMMAND_STOP_IF        = 0x03,
    VL_OOB_COMMAND_READ_DATA      = 0x04,
    VL_OOB_COMMAND_WRITE_DATA     = 0x05,
    VL_OOB_COMMAND_GET_IP_INFO    = 0x06,

    VL_OOB_COMMAND_INVALID        = 0xFF

}VL_OOB_COMMAND;

typedef enum tag_VL_POD_IF_COMMAND
{
    VL_POD_IF_COMMAND_INIT_POD          = 0x00,
    VL_POD_IF_COMMAND_RESET_PCMCIA      = 0x01,
    VL_POD_IF_COMMAND_RESET_INTERFACE   = 0x02,
    VL_POD_IF_COMMAND_POLL_ON_SND        = 0x03,
    VL_POD_IF_COMMAND_INVALID           = 0xFF

}VL_POD_IF_COMMAND;

typedef enum tag_VL_XCHAN_RESOURCE_ID
{
    VL_XCHAN_RESOURCE_ID_NONE           =  0,
    VL_XCHAN_RESOURCE_ID_VER_1          =  0x00A00041,
    VL_XCHAN_RESOURCE_ID_VER_2          =  0x00A00042,
    VL_XCHAN_RESOURCE_ID_VER_3          =  0x00A00043,
    VL_XCHAN_RESOURCE_ID_VER_4          =  0x00A00044,
    VL_XCHAN_RESOURCE_ID_VER_5          =  0x00A00045,
    VL_XCHAN_RESOURCE_ID_VER_6          =  0x00A00046,

    VL_XCHAN_RESOURCE_ID_INVALID        = 0x7FFFFFFF

}VL_XCHAN_RESOURCE_ID;

typedef enum tag_VL_IP_DHCP_OPTION_CODE
{
    VL_IP_DHCP_OPTION_CODE_NONE         =  0,
    VL_IP_DHCP_OPTION_CODE_SUBNET_MASK  =  1,
    VL_IP_DHCP_OPTION_CODE_TIME_OFFSET  =  2,
    VL_IP_DHCP_OPTION_CODE_ROUTER       =  3,
    VL_IP_DHCP_OPTION_CODE_TIME_SERVER  =  4,
    VL_IP_DHCP_OPTION_CODE_NAME_SERVER  =  5,
    VL_IP_DHCP_OPTION_CODE_DNS_SERVER   =  6,
    VL_IP_DHCP_OPTION_CODE_HOST_NAME    = 12,
    VL_IP_DHCP_OPTION_CODE_DOMAIN_NAME  = 15,

    VL_IP_DHCP_OPTION_CODE_INVALID  = 0xFF

}VL_IP_DHCP_OPTION_CODE;

typedef enum tag_VL_XCHAN_FLOW_RESULT
{
    VL_XCHAN_FLOW_RESULT_GRANTED                = 0x00, /* Request granted, new flow created                        */
    VL_XCHAN_FLOW_RESULT_DENIED_FLOWS_EXCEEDED  = 0x01, /* Request denied, number of flows exceeded                 */
    VL_XCHAN_FLOW_RESULT_DENIED_NO_SERVICE      = 0x02, /* Request denied, service_type not available               */
    VL_XCHAN_FLOW_RESULT_DENIED_NO_NETWORK      = 0x03, /* Request denied, network unavailable or not responding    */
    VL_XCHAN_FLOW_RESULT_DENIED_NET_BUSY        = 0x04, /* Request denied, network busy                             */
    VL_XCHAN_FLOW_RESULT_DENIED_BAD_MAC_ADDR    = 0x05, /* Request denied, MAC address not accepted                 */
    VL_XCHAN_FLOW_RESULT_DENIED_NO_DNS          = 0x06, /* Request denied, DNS not supported                        */
    VL_XCHAN_FLOW_RESULT_DENIED_DNS_FAILED      = 0x07, /* Request denied, DNS lookup failed                        */
    VL_XCHAN_FLOW_RESULT_DENIED_PORT_IN_USE     = 0x08, /* Request denied, local port already in use or invalid     */
    VL_XCHAN_FLOW_RESULT_DENIED_TCP_FAILED      = 0x09, /* Request denied, could not establish TCP connection       */
    VL_XCHAN_FLOW_RESULT_DENIED_NO_IPV6         = 0x0A, /* Request denied, IPv6 not supported                       */

    VL_XCHAN_FLOW_RESULT_INVALID                = 0xFF
}VL_XCHAN_FLOW_RESULT;

typedef enum tag_VL_XCHAN_DELETE_FLOW_RESULT
{
    VL_XCHAN_DELETE_FLOW_RESULT_GRANTED         = 0x00, /* Request granted, flow deleted                            */
    VL_XCHAN_DELETE_FLOW_RESULT_RESERVED_1      = 0x01, /* Reserved 1                                               */
    VL_XCHAN_DELETE_FLOW_RESULT_RESERVED_2      = 0x02, /* Reserved 2                                               */
    VL_XCHAN_DELETE_FLOW_DENIED_NO_NETWORK      = 0x03, /* Request denied, network unavailable or not responding    */
    VL_XCHAN_DELETE_FLOW_DENIED_NET_BUSY        = 0x04, /* Request denied, network busy                             */
    VL_XCHAN_DELETE_FLOW_RESULT_NO_FLOW_EXISTS  = 0x05, /* Request denied, flow_id does not exist                   */
    VL_XCHAN_DELETE_FLOW_DENIED_NOT_AUTHORIZED  = 0x06, /* Request granted, not authorized                          */

    VL_XCHAN_DELETE_FLOW_RESULT_INVALID         = 0xFF
}VL_XCHAN_DELETE_FLOW_RESULT;

typedef enum tag_VL_XCHAN_LOST_FLOW_REASON
{
    VL_XCHAN_LOST_FLOW_REASON_NOT_SPECIFIED         = 0x00,
    VL_XCHAN_LOST_FLOW_REASON_LEASE_EXPIRED         = 0x01,
    VL_XCHAN_LOST_FLOW_REASON_NET_BUSY_OR_DOWN      = 0x02,
    VL_XCHAN_LOST_FLOW_REASON_LOST_OR_REVOKED_AUTH  = 0x03,
    VL_XCHAN_LOST_FLOW_REASON_TCP_SOCKET_CLOSED     = 0x04,
    VL_XCHAN_LOST_FLOW_REASON_SOCKET_READ_ERROR     = 0x05,
    VL_XCHAN_LOST_FLOW_REASON_SOCKET_WRITE_ERROR    = 0x06,

    VL_XCHAN_LOST_FLOW_REASON_INVALID               = 0xFF
}VL_XCHAN_LOST_FLOW_REASON;

typedef enum tag_VL_XCHAN_LOST_FLOW_RESULT
{
    VL_XCHAN_LOST_FLOW_RESULT_ACKNOWLEDGED      = 0x00, /* Indication acknowledged          */
    VL_XCHAN_LOST_FLOW_RESULT_RESERVED_1        = 0x01, /* Reserved 1                                               */

    VL_XCHAN_LOST_FLOW_RESULT_INVALID           = 0xFF
}VL_XCHAN_LOST_FLOW_RESULT;

typedef enum tag_VL_XCHAN_IPU_FLOW_TYPE
{
    VL_XCHAN_IPU_FLOW_TYPE_TCP_UDP  = 0x00,
    VL_XCHAN_IPU_FLOW_TYPE_UDP_ONLY = 0x01,
    VL_XCHAN_IPU_FLOW_TYPE_TCP_ONLY = 0x02,

    VL_XCHAN_IPU_FLOW_TYPE_INVALID = 0xFF
}VL_XCHAN_IPU_FLOW_TYPE;

typedef enum tag_VL_XCHAN_SOCKET_PROTOCOL_TYPE
{
    VL_XCHAN_SOCKET_PROTOCOL_TYPE_UDP = 0x00,
    VL_XCHAN_SOCKET_PROTOCOL_TYPE_TCP = 0x01,

    VL_XCHAN_SOCKET_PROTOCOL_TYPE_INVALID = 0xFF
}VL_XCHAN_SOCKET_PROTOCOL_TYPE;

typedef enum tag_VL_XCHAN_SOCKET_ADDRESS_TYPE
{
    VL_XCHAN_SOCKET_ADDRESS_TYPE_NAME = 0x00,
    VL_XCHAN_SOCKET_ADDRESS_TYPE_IPV4 = 0x01,
    VL_XCHAN_SOCKET_ADDRESS_TYPE_IPV6 = 0x02,

    VL_XCHAN_SOCKET_ADDRESS_TYPE_INVALID = 0xFF
}VL_XCHAN_SOCKET_ADDRESS_TYPE;

typedef enum tag_VL_XCHAN_IP_ADDRESS_TYPE
{
    VL_XCHAN_IP_ADDRESS_TYPE_NONE  = 0x00,
    VL_XCHAN_IP_ADDRESS_TYPE_IPV4  = 0x01,
    VL_XCHAN_IP_ADDRESS_TYPE_IPV6  = 0x02,
    VL_XCHAN_IP_ADDRESS_TYPE_IPV4Z = 0x03,
    VL_XCHAN_IP_ADDRESS_TYPE_IPV6Z = 0x04,
    VL_XCHAN_IP_ADDRESS_TYPE_DNS   = 0x10,

    VL_XCHAN_IP_ADDRESS_TYPE_INVALID = 0xFF
}VL_XCHAN_IP_ADDRESS_TYPE;

typedef enum tag_VL_TLV_217_IP_MODE
{
    VL_TLV_217_IP_MODE_IPV4  = 0x00,
    VL_TLV_217_IP_MODE_IPV6  = 0x01,
    VL_TLV_217_IP_MODE_DUAL  = 0x02,

    VL_TLV_217_IP_MODE_INVALID = 0xFF
}VL_TLV_217_IP_MODE;

typedef enum tag_VL_XCHAN_FLOW_TYPE
{
    VL_XCHAN_FLOW_TYPE_MPEG             =   0,
    VL_XCHAN_FLOW_TYPE_IP_U             =   1,
    VL_XCHAN_FLOW_TYPE_IP_M             =   2,
    VL_XCHAN_FLOW_TYPE_DSG              =   3,
    VL_XCHAN_FLOW_TYPE_SOCKET           =   4,
    VL_XCHAN_FLOW_TYPE_IPDM             =   5,
    VL_XCHAN_FLOW_TYPE_IPDIRECT_UNICAST =   6,
    
    VL_XCHAN_FLOW_TYPE_MAX,
    VL_XCHAN_FLOW_TYPE_NC       =   0xFF,
    VL_XCHAN_FLOW_TYPE_INVALID  =   0xFFFF

}VL_XCHAN_FLOW_TYPE;

typedef struct tag_VL_XCHAN_IP_MULTICAST_FLOW_PARAMS
{
    unsigned char  bInUse;
    unsigned char  bSuccess;
    unsigned short sesnum;
    unsigned short portNum;
    unsigned long  flowid;
    unsigned long  socket;
    rmf_osal_ThreadId  threadId;

    unsigned char  groupId[VL_IP_ADDR_SIZE];

}VL_XCHAN_IP_MULTICAST_FLOW_PARAMS;

typedef struct tag_VL_XCHAN_SOCKET_FLOW_PARAMS
{
    unsigned char  bInUse;
    unsigned short sesnum;
    unsigned long  flowid;
    unsigned long  socket;
    unsigned long  threadId;
    unsigned long  timeoutThreadId;
    unsigned char  remote_addr[32];
    unsigned long  nRemoteAddrLen;
    
    VL_XCHAN_SOCKET_PROTOCOL_TYPE   eProtocol;
    unsigned short                  nLocalPortNum;
    unsigned short                  nRemotePortNum;
    VL_XCHAN_SOCKET_ADDRESS_TYPE    eRemoteAddressType;
    unsigned char                   nDnsNameLen;
    unsigned char                   dnsAddress [VL_DNS_ADDR_SIZE ];
    unsigned char                   ipAddress  [VL_IP_ADDR_SIZE  ];
    unsigned char                   ipV6address[VL_IPV6_ADDR_SIZE];
    unsigned char                   connTimeout;
}VL_XCHAN_SOCKET_FLOW_PARAMS;

typedef struct tag_VL_XCHAN_IPU_FLOW_CONF
{
    unsigned char           ipAddress[VL_IP_ADDR_SIZE];
    unsigned char           linkIpAddress[VL_IP_ADDR_SIZE];
    VL_XCHAN_IPU_FLOW_TYPE  eFlowType;
    unsigned char           flags;
    unsigned short          maxPDUSize;
    unsigned char           nOptionSize;
    unsigned char           aOptions[256];
}VL_XCHAN_IPU_FLOW_CONF;

typedef struct tag_VL_XCHAN_SOCKET_FLOW_CONF
{
    unsigned char           ipAddress[VL_IP_ADDR_SIZE];
    unsigned short          maxPDUSize;
    unsigned char           nOptionSize;
    unsigned char           aOptions[256];
}VL_XCHAN_SOCKET_FLOW_CONF;

typedef struct tag_VL_IP_IF_PARAMS
{
    unsigned long socket;
    unsigned long flowid;
    unsigned char macAddress[VL_MAC_ADDR_SIZE];
    unsigned char filler[VL_MAC_ADDR_SIZE%VL_IP_ADDR_SIZE];
    unsigned char ipAddress[VL_IP_ADDR_SIZE];
    unsigned char subnetMask[VL_IP_ADDR_SIZE];
    unsigned char router[VL_IP_ADDR_SIZE];
    unsigned char dns[VL_IP_ADDR_SIZE];
}VL_IP_IF_PARAMS;

typedef struct tag_VL_OOB_IP_DATA
{
    int    socket;
    int    nPort;
    char   szIpAddress[VL_IP_ADDR_STR_SIZE];
    int    nLength;
    int    nCapacity;
    void * pData;

}VL_OOB_IP_DATA;

typedef struct tag_VL_HOST_CARD_IP_INFO
{
    int           ipAddressType;
    unsigned char ipAddress[VL_IP_ADDR_SIZE];
    unsigned char macAddress[VL_MAC_ADDR_SIZE];

}VL_HOST_CARD_IP_INFO;

typedef enum tag_VL_DHCP_CLIENT_TYPE
{
    VL_DHCP_CLIENT_TYPE_IPV4        =   0x00000000,
    VL_DHCP_CLIENT_TYPE_IPV6        =   0x00000001,
    
    VL_DHCP_CLIENT_TYPE_INVALID     =   0x7FFFFFFF

}VL_DHCP_CLIENT_TYPE;

typedef enum tag_VL_DHCP_CLIENT_ENTITY
{
    VL_DHCP_CLIENT_ENTITY_ESTB      =   0x00000000,
    VL_DHCP_CLIENT_ENTITY_CARD      =   0x00000001,
    
    VL_DHCP_CLIENT_ENTITY_INVALID   =   0x7FFFFFFF

}VL_DHCP_CLIENT_ENTITY;

typedef enum tag_VL_HOST_IP_IF_TYPE
{
    VL_HOST_IP_IF_TYPE_ETH_DEBUG    =   0x00000000,
        
    VL_HOST_IP_IF_TYPE_ETH          =   0x00100000,
    VL_HOST_IP_IF_TYPE_USB          =   0x00200000,
    VL_HOST_IP_IF_TYPE_1394         =   0x00300000,
    VL_HOST_IP_IF_TYPE_WIFI         =   0x00400000,
    VL_HOST_IP_IF_TYPE_MODEM        =   0x00500000,
    VL_HOST_IP_IF_TYPE_IRDA         =   0x00600000,
    VL_HOST_IP_IF_TYPE_SERIAL       =   0x00700000,
    VL_HOST_IP_IF_TYPE_DSL          =   0x00800000,
    VL_HOST_IP_IF_TYPE_CABLE        =   0x00900000,
    VL_HOST_IP_IF_TYPE_ISDN         =   0x00A00000,
    VL_HOST_IP_IF_TYPE_GSM          =   0x00B00000,
    VL_HOST_IP_IF_TYPE_BLUETOOTH    =   0x00C00000,
    VL_HOST_IP_IF_TYPE_MOCA         =   0x00D00000,
    VL_HOST_IP_IF_TYPE_HN           =   0x00E00000,
        
    VL_HOST_IP_IF_TYPE_POD_QPSK     =   0x60000000,
    VL_HOST_IP_IF_TYPE_DSG          =   0x70000000,

    VL_HOST_IP_IF_TYPE_MASK         =   0x7FFF0000,
    VL_HOST_IP_IF_TYPE_INST_MASK    =   0x0000FFFF,
    VL_HOST_IP_IF_TYPE_INVALID      =   0x7FFFFFFF

}VL_HOST_IP_IF_TYPE;

typedef struct tag_VL_HOST_IP_STATS
{
    unsigned long ifMtu            ;
    unsigned long ifSpeed          ;
    unsigned long ifInOctets       ;
    unsigned long ifInUcastPkts    ;
    unsigned long ifInNUcastPkts   ;
    unsigned long ifInDiscards     ;
    unsigned long ifInErrors       ;
    unsigned long ifInUnknownProtos;
    unsigned long ifOutOctets      ;
    unsigned long ifOutUcastPkts   ;
    unsigned long ifOutNUcastPkts  ;
    unsigned long ifOutDiscards    ;
    unsigned long ifOutErrors      ;
    unsigned long ifOutUnknownProtos;
    
}VL_HOST_IP_STATS;

typedef struct tag_VL_HOST_IP_INFO
{
    VL_XCHAN_IP_ADDRESS_TYPE     ipAddressType;
    int     iInstance;
    char    szIfName[VL_IF_NAME_SIZE]; // ASCII string
    // mac address
    unsigned char   aBytMacAddress     [VL_MAC_ADDR_SIZE       ]; // binary array
    char            szMacAddress       [VL_MAC_ADDR_STR_SIZE   ]; // ASCII string
    // v4 address
    unsigned char aBytIpAddress         [VL_IP_ADDR_SIZE    ]; // binary array
    unsigned char aBytSubnetMask        [VL_IP_ADDR_SIZE    ]; // binary array
    char szIpAddress        [VL_IP_ADDR_STR_SIZE    ]; // ASCII string
    char szSubnetMask       [VL_IP_ADDR_STR_SIZE    ]; // ASCII string
    //v6 address
    unsigned char aBytIpV6GlobalAddress [VL_IPV6_ADDR_SIZE  ]; // binary array
    unsigned char aBytIpV6v4Address     [VL_IPV6_ADDR_SIZE  ]; // binary array
    unsigned char aBytIpV6SiteAddress   [VL_IPV6_ADDR_SIZE  ]; // binary array
    unsigned char aBytIpV6LinkAddress   [VL_IPV6_ADDR_SIZE  ]; // binary array
    //v6 masks
    int nIpV6GlobalPlen ; //mask
    int nIpV6v4Plen     ; //mask
    int nIpV6SitePlen   ; //mask
    int nIpV6LinkPlen   ; //mask
    //v6 address strings
    char szIpV6GlobalAddress[VL_IPV6_ADDR_STR_SIZE  ]; // ASCII string
    char szIpV6v4Address    [VL_IPV6_ADDR_STR_SIZE  ]; // ASCII string
    char szIpV6SiteAddress  [VL_IPV6_ADDR_STR_SIZE  ]; // ASCII string
    char szIpV6LinkAddress  [VL_IPV6_ADDR_STR_SIZE  ]; // ASCII string
    
    unsigned long lFlags;
    unsigned long lLastChange;
    
    VL_HOST_IP_STATS stats;
    
}VL_HOST_IP_INFO;

typedef struct tag_VL_POD_IP_STATUS
{
    VL_XCHAN_FLOW_RESULT     eFlowResult;
    char    szPodIpStatus    [VL_DNS_ADDR_SIZE];
}VL_POD_IP_STATUS;

typedef struct tag_VL_IP_ADDRESS_STRUCT
{
    VL_XCHAN_IP_ADDRESS_TYPE     eIpAddressType;
    // v4 address
    unsigned char aBytIpAddress         [VL_IP_ADDR_SIZE    ]; // binary array
    unsigned char aBytIpV6GlobalAddress [VL_IPV6_ADDR_SIZE  ]; // binary array
    char szIpAddress        [VL_IP_ADDR_STR_SIZE    ]; // ASCII string
    char szIpV6GlobalAddress[VL_IPV6_ADDR_STR_SIZE  ]; // ASCII string

    unsigned char aBytSubnetMask        [VL_IP_ADDR_SIZE    ]; // binary array
    unsigned char aBytIpV6SubnetMask    [VL_IPV6_ADDR_SIZE  ]; // binary array
    char szSubnetMask                   [VL_IP_ADDR_STR_SIZE    ]; // ASCII string
    char szIpV6SubnetMask               [VL_IPV6_ADDR_STR_SIZE  ]; // ASCII string
}VL_IP_ADDRESS_STRUCT;

typedef struct tag_VL_MAC_ADDRESS_STRUCT
{
    // mac address
    unsigned char   aBytMacAddress     [VL_MAC_ADDR_SIZE       ]; // binary array
    char            szMacAddress       [VL_MAC_ADDR_STR_SIZE   ]; // ASCII string
    
}VL_MAC_ADDRESS_STRUCT;
/*-----------------------------------------------------------------*/
#endif // __VL_IP_TYPES_H__
/*-----------------------------------------------------------------*/
