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
#ifndef __VL_DSG_TYPES_H__
#define __VL_DSG_TYPES_H__
/*-----------------------------------------------------------------*/

#define VL_MAX_DSG_DEVICES          (1)
#define VL_CM_DSG_DEVICE_INSTANCE   (0)

typedef void *VL_DSG_HANDLE;          // handle used by the application to access the API
typedef void *VL_DSG_INSTANCEHANDLE;  // handle used by the API to access the driver instance (never used by the application)
typedef unsigned long VL_DSG_DEVICE_HANDLE_t;

typedef enum _VL_DSG_DOWNLOAD_STATE
{
    VL_DSG_DOWNLOAD_STATE_IDLE,
    VL_DSG_DOWNLOAD_STATE_IN_PROGRESS
} VL_DSG_DOWNLOAD_STATE;

typedef enum _VL_DSG_DOWNLOAD_RESULT
{
    VL_DSG_DOWNLOAD_RESULT_SUCCEEDED,
    VL_DSG_DOWNLOAD_RESULT_FAILED

} VL_DSG_DOWNLOAD_RESULT;

typedef enum _VL_DSG_ECM_MODE
{
    VL_DSG_ECM_MODE_DOCSIS,
    VL_DSG_ECM_MODE_DSG
} VL_DSG_ECM_MODE;

typedef enum _VL_DSG_ECM_STATUS
{
    VL_DSG_ECM_STATUS_OK,
    VL_DSG_ECM_STATUS_NOT_OK
} VL_DSG_ECM_STATUS;

typedef enum _VL_DSG_RESULT
{
    VL_DSG_RESULT_SUCCESS               = 0,
    VL_DSG_RESULT_NOT_IMPLEMENTED       = 1,
    VL_DSG_RESULT_NULL_PARAM            = 2,
    VL_DSG_RESULT_INVALID_PARAM         = 3,
    VL_DSG_RESULT_INCOMPLETE_PARAM      = 4,
    VL_DSG_RESULT_INVALID_IMAGE_TYPE    = 5,
    VL_DSG_RESULT_INVALID_IMAGE_SIGN    = 6,
    VL_DSG_RESULT_ECM_FAILURE           = 7,
    VL_DSG_RESULT_ECM_SUCCESS           = 8,
    VL_DSG_RESULT_HAL_SUPPORTS          = 9,
    VL_DSG_RESULT_STATUS_OK             = 10,
    VL_DSG_RESULT_STATUS_NOT_OK         = 11,

    VL_DSG_RESULT_INVALID               = 0xFF
} VL_DSG_RESULT;

typedef int ( *VL_DSG_TUNNEL_CBFUNC_t          )(VL_DSG_DEVICE_HANDLE_t , unsigned char *, unsigned long);
typedef int ( *VL_DSG_CTRL_CBFUNC_t            )(VL_DSG_DEVICE_HANDLE_t , unsigned char *, unsigned long);
typedef int ( *VL_DSG_IMAGE_CBFUNC_t           )(VL_DSG_DEVICE_HANDLE_t , unsigned char *, unsigned long , unsigned long);
typedef int ( *VL_DSG_DOWNLOAD_STATUS_CBFUNC_t )(VL_DSG_DEVICE_HANDLE_t , VL_DSG_DOWNLOAD_STATE , VL_DSG_DOWNLOAD_RESULT );
typedef int ( *VL_DSG_ECMSTATUS_CBFUNC_t       )(VL_DSG_DEVICE_HANDLE_t , VL_DSG_ECM_STATUS );
typedef int ( *VL_DSG_ASYNC_CBFUNC_t           )(VL_DSG_DEVICE_HANDLE_t , unsigned long ulTag,
                                                                          unsigned long ulQualifier,
                                                                          void * pStruct);

typedef enum
{
    VL_DSG_NOTIFY_TUNNEL_DATA,
    VL_DSG_NOTIFY_CONTROL_DATA,
    VL_DSG_NOTIFY_IMAGE_DATA,
    VL_DSG_NOTIFY_DOWNLOAD_STATUS,
    VL_DSG_NOTIFY_ECM_STATUS,
    VL_DSG_NOTIFY_ASYNC_DATA,

    VL_DSG_NOTIFY_INVALID = 0xFF,

} VL_DSG_NOTIFY_TYPE_t;

typedef struct VL_DSG_CAPABILITIES_INSTANCE_s
{
    VL_DSG_DEVICE_HANDLE_t hDSGHandle;
    VL_DSG_ECM_MODE  hDSGMode;

} VL_DSG_CAPABILITIES_INSTANCE_t;


typedef struct VL_DSG_CAPABILITIES_s
{
    int                             usNumDSGs;
    VL_DSG_CAPABILITIES_INSTANCE_t  astInstanceCapabilities[VL_MAX_DSG_DEVICES];
} VL_DSG_CAPABILITIES_t;

typedef enum tag_VL_DSG_PVT_TAG
{
    VL_DSG_PVT_TAG_NONE                 = 0x10000000,

    VL_DSG_PVT_TAG_IPU_PKT_CARD_TO_ECM  = 0x10000001,
    VL_DSG_PVT_TAG_IPU_PKT_ECM_TO_CARD  = 0x10000002,
    VL_DSG_PVT_TAG_IPU_DHCP_TIMEOUT     = 0x10000003,
        
    VL_DSG_PVT_TAG_IP_MULTICAST_FLOW        = 0x10000011,
    VL_DSG_PVT_TAG_IP_CANCEL_MULTICAST_FLOW = 0x10000012,

    VL_DSG_PVT_TAG_SOCKET_FLOW          = 0x10000021,

    VL_DSG_PVT_TAG_ECM_TIMEOUT          = 0x10000031,

    VL_DSG_PVT_TAG_POD_REMOVED          = 0x10000040,
    VL_DSG_PVT_TAG_DSG_CLEAN_UP         = 0x10000041,
        
    VL_DSG_PVT_TAG_DSG_GET_IP_INFO      = 0x10000050,
        
    VL_DSG_PVT_TAG_GET_DSG_STATUS       = 0x10000060,
    VL_DSG_PVT_TAG_NON_DSG_MODE         = 0x10000061,

    VL_DSG_PVT_TAG_TLV_217              = 0x10000070,
    VL_DSG_PVT_TAG_DOCS_DEV_EVENT       = 0x10000071,
    VL_DSG_PVT_TAG_ESAFE_DHCP_STATUS    = 0x10000072,
    VL_DSG_PVT_TAG_ECM_OPERATIONAL_NOW  = 0x10000073,
    VL_DSG_PVT_TAG_ESAFE_IP_ADDRESS     = 0x10000074,
        
    VL_DSG_PVT_TAG_get_ECM_IP_MODE      = 0x10000080,
    VL_DSG_PVT_TAG_SET_HOST_IP_MODE     = 0x10000081,
    
    VL_DSG_PVT_TAG_GET_DIR_MOD_CAPS     = 0x10000100,
    VL_DSG_PVT_TAG_SET_DIR_CONFIG       = 0x10000101,
    VL_DSG_PVT_TAG_ADD_HOST_TUNNEL      = 0x10000102,
    VL_DSG_PVT_TAG_REMOVE_HOST_TUNNEL   = 0x10000103,
    VL_DSG_PVT_TAG_ADD_CARD_TUNNEL      = 0x10000104,
    VL_DSG_PVT_TAG_REMOVE_CARD_TUNNEL   = 0x10000105,
    VL_DSG_PVT_TAG_UPDATE_DIR_CONFIG    = 0x10000106,
    
    VL_DSG_PVT_TAG_GET_IP_RECOVERY_CAPS     = 0x10000200,
    VL_DSG_PVT_TAG_GET_IS_ECM_OPERATIONAL   = 0x10000201,
    VL_DSG_PVT_TAG_SET_ATTEMPT_IP_RECOVERY  = 0x10000202,
    
    VL_DSG_PVT_TAG_ECM_UPDATE_DHCP_OPTION_43 = 0x10000300,
    VL_DSG_PVT_TAG_ECM_UPDATE_DHCP_OPTION_17 = 0x10000301,
    
    VL_DSG_PVT_TAG_INVALID              = 0x1FFFFFFF

}VL_DSG_PVT_TAG;

typedef enum tag_VL_HOST2POD_DSG_APDU_NAMES
{
    VL_HOST2POD_INQUIRE_DSG_MODE =   0x9F9100,
    VL_HOST2POD_DSG_MESSAGE      =   0x9F9103,
    VL_HOST2POD_SEND_DCD_INFO    =   0x9F9105,

    VL_HOST2POD_INVALID          =   0x9F91FF

}VL_HOST2POD_DSG_APDU_NAMES;

typedef enum tag_VL_POD2HOST_DSG_APDU_NAMES
{
    VL_POD2HOST_SET_DSG_MODE     =   0x9F9101,
    VL_POD2HOST_DSG_ERROR        =   0x9F9102,
    VL_POD2HOST_DSG_DIRECTORY    =   0x9F9104,

    VL_POD2HOST_INVALID          =   0x9F91FF

}VL_POD2HOST_DSG_APDU_NAMES;

typedef enum tag_VL_XCHAN_HOST2POD_DSG_APDU_NAMES
{
    VL_XCHAN_HOST2POD_INQUIRE_DSG_MODE =   0x9F8E06,
    VL_XCHAN_HOST2POD_DSG_MESSAGE      =   0x9F8E09,
    VL_XCHAN_HOST2POD_SEND_DCD_INFO    =   0x9F8E0B,

    VL_XCHAN_HOST2POD_INVALID          =   0x9F8EFF

}VL_XCHAN_HOST2POD_DSG_APDU_NAMES;

typedef enum tag_VL_XCHAN_POD2HOST_DSG_APDU_NAMES
{
    VL_XCHAN_POD2HOST_NEW_FLOW_REQ     =   0x9F8E00,
    VL_XCHAN_POD2HOST_NEW_FLOW_CNF     =   0x9F8E01,
    VL_XCHAN_POD2HOST_SET_DSG_MODE     =   0x9F8E07,
    VL_XCHAN_POD2HOST_DSG_ERROR        =   0x9F8E08,
    VL_XCHAN_POD2HOST_CONFIG_ADV_DSG   =   0x9F8E0A,

    VL_XCHAN_POD2HOST_INVALID          =   0x9F8EFF

}VL_XCHAN_POD2HOST_DSG_APDU_NAMES;

#define VL_MAC_ADDR_SIZE        6
#define VL_IP_ADDR_SIZE         4
#define VL_IP_MASK_SIZE         4
#define VL_DSG_ETH_CRC_SIZE     4
#define VL_MAX_DSG_ENTRIES      0x20    /* equals    32d */
#define VL_MAX_DSG_HOST_ENTRIES 0x20    /* equals    32d */
#define VL_MAX_DSG_CARD_ENTRIES 0x20    /* equals    32d , card entries are usually small   */
#define VL_MAX_DSG_MAC_ENTRIES  0x20    /* equals    32d */
#define VL_MAX_DSG_TUNNELS      0x20    /* equals    32d , used only for legacy             */
#define VL_MAX_DSG_CONFIGS      0x20    /* equals    32d */
#define VL_MAX_DSG_ID_CHARS     0x10    /* equals    16d */
#define VL_DSG_SECTION_GROWTH_SIZE      0x1100  /* equals  4352d */
#define VL_DSG_IP_PAYLOAD_GROWTH_SIZE   0x600   /* equals  1536d */

#define VL_DSG_IP_REASSEMBLY_TIMEOUT         10   /* seconds */
#define VL_DSG_BCAST_REASSEMBLY_TIMEOUT      60   /* seconds */

#define VL_DSG_MAX_STR_SIZE     0x100    /* equals    256d */

#define VL_DSG_CM_IP_ADDRESS        0xC0A86400  // "192.168.100.x"
#define VL_DSG_NULL_IP_ADDRESS      0x00000000  // "0.0.0.0"

typedef enum tag_VL_DSG_DISPATCH_TO
{
    VL_DSG_DISPATCH_TO_CARD     = 0x01,
    VL_DSG_DISPATCH_TO_HOST     = 0x02,

    VL_DSG_DISPATCH_TO_INVALID  = 0xFF

}VL_DSG_DISPATCH_TO;

typedef enum tag_VL_DSG_2_WAY_FAIL_REASON
{
    VL_DSG_2_WAY_FAIL_REASON_RESERVED                   =   0x00,
    VL_DSG_2_WAY_FAIL_REASON_NET_ACCESS_DISABLED        =   0x01,
    VL_DSG_2_WAY_FAIL_REASON_MAX_CPE_LIMIT              =   0x02,
    VL_DSG_2_WAY_FAIL_REASON_INTERFACE_DOWN             =   0x04,

    VL_DSG_2_WAY_FAIL_REASON_INVALID                    =   0xFF

}VL_DSG_2_WAY_FAIL_REASON;

typedef enum tag_VL_DSG_MESSAGE_TYPE
{
    VL_DSG_MESSAGE_TYPE_APP_TUNNEL_REQ                  =   0x00,
    VL_DSG_MESSAGE_TYPE_2_WAY_OK                        =   0x01,
    VL_DSG_MESSAGE_TYPE_ENTERING_ONE_WAY_MODE           =   0x02,
    VL_DSG_MESSAGE_TYPE_DOWNSTREAM_SCAN_COMPLETED       =   0x03,
    VL_DSG_MESSAGE_TYPE_DYNAMIC_CHANNEL_CHANGE          =   0x04,
    VL_DSG_MESSAGE_TYPE_ECM_RESET                       =   0x05,
    VL_DSG_MESSAGE_TYPE_TOO_MANY_MAC_ADDR               =   0x06,
    VL_DSG_MESSAGE_TYPE_CANNOT_FORWARD_2_WAY            =   0x07,

    VL_DSG_MESSAGE_TYPE_INVALID                         =   0xFF

}VL_DSG_MESSAGE_TYPE;

typedef enum tag_VL_DSG_OPERATIONAL_MODE
{
    VL_DSG_OPERATIONAL_MODE_SCTE55          =   0x00,
    VL_DSG_OPERATIONAL_MODE_BASIC_TWO_WAY   =   0x01,
    VL_DSG_OPERATIONAL_MODE_BASIC_ONE_WAY   =   0x02,
    VL_DSG_OPERATIONAL_MODE_ADV_TWO_WAY     =   0x03,
    VL_DSG_OPERATIONAL_MODE_ADV_ONE_WAY     =   0x04,
    VL_DSG_OPERATIONAL_MODE_IPDIRECT_UNICAST=   0x05,
    VL_DSG_OPERATIONAL_MODE_IPDM            =   0x06,

    VL_DSG_OPERATIONAL_MODE_INVALID         =   0xFF

}VL_DSG_OPERATIONAL_MODE;

typedef enum tag_VL_DSG_ERROR_STATUS
{
    VL_DSG_ERROR_STATUS_BYTE_COUNT_ERROR    =   0x00,
    VL_DSG_ERROR_STATUS_INVALID_DSG_CHANNEL =   0x01,

    VL_DSG_ERROR_STATUS_INVALID             =   0xFF

}VL_DSG_ERROR_STATUS;

typedef unsigned long long VL_MAC_ADDRESS;
typedef long VL_IP_ADDRESS;
typedef long VL_IP_MASK;

typedef enum tag_VL_DSG_BCAST_CLIENT_ID
{
    VL_DSG_BCAST_CLIENT_ID_SCTE_65 = 0x01,
    VL_DSG_BCAST_CLIENT_ID_SCTE_18 = 0x02,
    VL_DSG_BCAST_CLIENT_ID_XAIT    = 0x05,

    VL_DSG_BCAST_CLIENT_ID_INVALID = 0xFF

}VL_DSG_BCAST_CLIENT_ID;

typedef enum tag_VL_DSG_BCAST_TABLE_ID
{
    VL_DSG_BCAST_TABLE_ID_EAS     = 0xD8,
    VL_DSG_BCAST_TABLE_ID_XAIT    = 0x74,
    VL_DSG_BCAST_TABLE_ID_CVT     = 0xD9,
    VL_DSG_BCAST_TABLE_ID_STT     = 0xC5,

    VL_DSG_BCAST_TABLE_ID_INVALID = 0xFF

}VL_DSG_BCAST_TABLE_ID;

typedef enum tag_VL_DSG_CLIENT_ID_ENC_TYPE
{
    VL_DSG_CLIENT_ID_ENC_TYPE_NONE  = 0x00,
    VL_DSG_CLIENT_ID_ENC_TYPE_BCAST = 0x01,
    VL_DSG_CLIENT_ID_ENC_TYPE_WKMA  = 0x02,
    VL_DSG_CLIENT_ID_ENC_TYPE_CAS   = 0x03,
    VL_DSG_CLIENT_ID_ENC_TYPE_APP   = 0x04,

    VL_DSG_CLIENT_ID_ENC_TYPE_INVALID= 0xFF

}VL_DSG_CLIENT_ID_ENC_TYPE;

typedef struct tag_VL_DSG_CLIENT_ID
{
    VL_DSG_CLIENT_ID_ENC_TYPE eEncodingType;
    unsigned char nEncodingLength;
    unsigned long long nEncodedId;
    unsigned long bClientDisabled;

}VL_DSG_CLIENT_ID;

typedef enum tag_VL_DSG_DIR_TYPE
{
    VL_DSG_DIR_TYPE_DIRECT_TERM_DSG_FLOW = 0x01,
    VL_DSG_DIR_TYPE_EXT_CHNL_MPEG_FLOW   = 0x02,

    VL_DSG_DIR_TYPE_INVALID              = 0xFF

}VL_DSG_DIR_TYPE;

typedef enum tag_VL_DSG_DISPATCH_RESULT
{
    VL_DSG_DISPATCH_RESULT_DROPPED          = 0x00,
    VL_DSG_DISPATCH_RESULT_DISPATCHED       = 0x01,
    VL_DSG_DISPATCH_RESULT_EXT_CHANNEL_CB   = 0x02,
    VL_DSG_DISPATCH_RESULT_NO_MATCH_PAYLOAD = 0x03,
    VL_DSG_DISPATCH_RESULT_NO_MATCH_CLIENT  = 0x04,
    VL_DSG_DISPATCH_RESULT_OK_TO_DISPATCH   = 0x05,
    VL_DSG_DISPATCH_RESULT_CRC_ERROR        = 0x06,

    VL_DSG_DISPATCH_RESULT_INVALID          = 0xFF

}VL_DSG_DISPATCH_RESULT;

typedef enum tag_VL_DSG_TLV_TYPE
{
    VL_DSG_TLV_TYPE_NONE                            = 0x00,
    VL_DSG_TLV_TYPE_DSG_CLF                         = 0x17,
    VL_DSG_TLV_TYPE_DSG_CLF_ID                      = 0x1702,
    VL_DSG_TLV_TYPE_DSG_CLF_PRIORITY                = 0x1705,
    VL_DSG_TLV_TYPE_DSG_CLF_IP_PCE                  = 0x1709,
    VL_DSG_TLV_TYPE_DSG_CLF_IP_PCE_SRC_IP           = 0x170903,
    VL_DSG_TLV_TYPE_DSG_CLF_IP_PCE_SRC_IP_MASK      = 0x170904,
    VL_DSG_TLV_TYPE_DSG_CLF_IP_PCE_DST_IP           = 0x170905,
    VL_DSG_TLV_TYPE_DSG_CLF_IP_PCE_DST_PORT_START   = 0x170909,
    VL_DSG_TLV_TYPE_DSG_CLF_IP_PCE_DSG_PORT_END     = 0x17090A,
    VL_DSG_TLV_TYPE_DSG_RULE                        = 0x32,
    VL_DSG_TLV_TYPE_DSG_RULE_ID                     = 0x3201,
    VL_DSG_TLV_TYPE_DSG_RULE_PRIORITY               = 0x3202,
    VL_DSG_TLV_TYPE_DSG_RULE_UCID_LIST              = 0x3203,
    VL_DSG_TLV_TYPE_DSG_RULE_CLIENT_ID              = 0x3204,
    VL_DSG_TLV_TYPE_DSG_RULE_CLIENT_ID_BC           = 0x320401,
    VL_DSG_TLV_TYPE_DSG_RULE_CLIENT_ID_MAC          = 0x320402,
    VL_DSG_TLV_TYPE_DSG_RULE_CLIENT_ID_CA           = 0x320403,
    VL_DSG_TLV_TYPE_DSG_RULE_CLIENT_ID_APP          = 0x320404,
    VL_DSG_TLV_TYPE_DSG_RULE_DSG_MAC                = 0x3205,
    VL_DSG_TLV_TYPE_DSG_RULE_CLF_ID                 = 0x3206,
    VL_DSG_TLV_TYPE_DSG_RULE_VENDOR                 = 0x322B,
    VL_DSG_TLV_TYPE_DSG_CONFIG                      = 0x33,
    VL_DSG_TLV_TYPE_DSG_CONFIG_CHANNEL              = 0x3301,
    VL_DSG_TLV_TYPE_DSG_CONFIG_INIT_TIMEOUT         = 0x3302,
    VL_DSG_TLV_TYPE_DSG_CONFIG_OPER_TIMEOUT         = 0x3303,
    VL_DSG_TLV_TYPE_DSG_CONFIG_2WR_TIMEOUT          = 0x3304,
    VL_DSG_TLV_TYPE_DSG_CONFIG_1WR_TIMEOUT          = 0x3305,
    VL_DSG_TLV_TYPE_DSG_CONFIG_VENDOR               = 0x332B,

    VL_DSG_TLV_TYPE_INVALID                         = 0xFF

}VL_DSG_TLV_TYPE;

typedef struct tag_VL_DSG_MODE
{
    VL_DSG_OPERATIONAL_MODE eOperationalMode;
    int            nMacAddresses;
    VL_MAC_ADDRESS aMacAddresses[VL_MAX_DSG_MAC_ENTRIES];
    int            nRemoveHeaderBytes;

}VL_DSG_MODE;

typedef struct tag_VL_ADSG_FILTER
{
    unsigned char  nTunneId;
    unsigned char  nTunnelPriority;
    VL_MAC_ADDRESS macAddress;
    VL_IP_ADDRESS  ipAddress;
    VL_IP_MASK     ipMask;
    VL_IP_ADDRESS  destIpAddress;
    unsigned short nPortStart;
    unsigned short nPortEnd;
    unsigned long  hTunnelHandle;

}VL_ADSG_FILTER;

typedef struct tag_VL_DSG_HOST_ENTRY
{
    VL_DSG_CLIENT_ID clientId;
    VL_DSG_DIR_TYPE  eEntryType;
    VL_ADSG_FILTER   adsgFilter;
    unsigned char    ucid;

}VL_DSG_HOST_ENTRY;

typedef struct tag_VL_DSG_CARD_ENTRY
{
    VL_ADSG_FILTER   adsgFilter;

}VL_DSG_CARD_ENTRY;

typedef struct tag_VL_DSG_ERROR
{
    VL_DSG_ERROR_STATUS  eErrorStatus;

}VL_DSG_ERROR;

typedef struct tag_VL_DSG_DIRECTORY
{
    unsigned char  bVctIdIncluded;
    unsigned char  dirVersion;
    int            nHostEntries;
    VL_DSG_HOST_ENTRY aHostEntries[VL_MAX_DSG_HOST_ENTRIES];
    int            nCardEntries;
    VL_DSG_CARD_ENTRY aCardEntries[VL_MAX_DSG_CARD_ENTRIES];
    int            nRxFrequencies;
    unsigned long  aRxFrequency[VL_MAX_DSG_ENTRIES];
    unsigned short nInitTimeout;
    unsigned short nOperTimeout;
    unsigned short nTwoWayTimeout;
    unsigned short nOneWayTimeout;
    unsigned short nVctId;

}VL_DSG_DIRECTORY;

typedef struct tag_VL_DSG_TUNNEL_FILTER
{
    unsigned char  nTunneId;
    unsigned short nAppId;
    VL_MAC_ADDRESS dsgMacAddress;
    VL_IP_ADDRESS  srcIpAddress;
    VL_IP_MASK     srcIpMask;
    VL_IP_ADDRESS  destIpAddress;
    int            nPorts;
    unsigned short aPorts[VL_MAX_DSG_ENTRIES];
    int            nRemoveHeaderBytes;

}VL_DSG_TUNNEL_FILTER;

typedef struct tag_VL_DSG_ADV_CONFIG
{
    int            nTunnelFilters;
    VL_DSG_TUNNEL_FILTER aTunnelFilters[VL_MAX_DSG_TUNNELS];
    int            nRxFrequencies;
    unsigned long  aRxFrequency[VL_MAX_DSG_ENTRIES];
    unsigned short nInitTimeout;
    unsigned short nOperTimeout;
    unsigned short nTwoWayTimeout;
    unsigned short nOneWayTimeout;

}VL_DSG_ADV_CONFIG;

typedef struct tag_VL_DSG_IP_CPE
{
    VL_IP_ADDRESS   srcIpAddress;
    VL_IP_MASK      srcIpMask;
    VL_IP_ADDRESS   destIpAddress;
    unsigned short  nPortStart;
    unsigned short  nPortEnd;

}VL_DSG_IP_CPE;

typedef struct tag_VL_DSG_CLASSIFIER
{
    unsigned short  nId;
    unsigned char   nPriority;
    VL_DSG_IP_CPE   ipCpe;

}VL_DSG_CLASSIFIER;

typedef struct tag_VL_DSG_RULE
{
    unsigned char       nId;
    unsigned char       nPriority;
    int                 nUCIDs;
    unsigned char       aUCIDs[VL_MAX_DSG_ENTRIES];
    int                 nClients;
    VL_DSG_CLIENT_ID    aClients[VL_MAX_DSG_ENTRIES];
    VL_MAC_ADDRESS      dsgMacAddress;
    unsigned short      nClassifierId;

}VL_DSG_RULE;

typedef struct tag_VL_DSG_CONFIG
{
    int             nRxFrequencies;
    unsigned long   aRxFrequency[VL_MAX_DSG_ENTRIES];
    unsigned short  nInitTimeout;
    unsigned short  nOperTimeout;
    unsigned short  nTwoWayTimeout;
    unsigned short  nOneWayTimeout;

}VL_DSG_CONFIG;

typedef struct tag_VL_DSG_DCD
{
    int                 nClassifiers;
    VL_DSG_CLASSIFIER   aClassifiers[VL_MAX_DSG_CONFIGS];
    int                 nRules;
    VL_DSG_RULE         aRules[VL_MAX_DSG_CONFIGS];
    VL_DSG_CONFIG       Config;

}VL_DSG_DCD;

typedef struct tag_VL_DSG_STATUS
{
    int     nDsgStatus;
    int     nDocsisStatus;
    char    szDsgStatus     [VL_DSG_MAX_STR_SIZE];
    char    szDocsisStatus  [VL_DSG_MAX_STR_SIZE];
}VL_DSG_STATUS;

typedef enum tag_VL_DSG_ESAFE_BOOL
{
    VL_DSG_ESAFE_BOOL_TRUE  = 1,
    VL_DSG_ESAFE_BOOL_FALSE = 2
}VL_DSG_ESAFE_BOOL;

typedef enum tag_VL_DSG_ESAFE_PROGRESS
{
    VL_DSG_ESAFE_PROGRESS_NOT_INITIATED = 1,
    VL_DSG_ESAFE_PROGRESS_IN_PROGRESS   = 2,
    VL_DSG_ESAFE_PROGRESS_FINISHED      = 3
}VL_DSG_ESAFE_PROGRESS;

typedef struct tag_VL_DSG_ESAFE_DHCP_STATUS
{
    VL_DSG_ESAFE_PROGRESS     eProgress;
    VL_DSG_ESAFE_BOOL         eFailureFound;
    unsigned long             nEventId;
}VL_DSG_ESAFE_DHCP_STATUS;

typedef struct tag_VL_DSG_ECM_DHCP_OPTION_43
{
    unsigned char * pBytes;
    int nBytes;
    
}VL_DSG_ECM_DHCP_OPTION_43;

typedef struct tag_VL_DSG_ECM_DHCP_OPTION_17
{
    unsigned char * pBytes;
    int nBytes;

}VL_DSG_ECM_DHCP_OPTION_17;

typedef enum tag_VL_DSG_ETH_PROTOCOL
{
    VL_DSG_ETH_PROTOCOL_NONE    = 0x0000,
    VL_DSG_ETH_PROTOCOL_IP      = 0x0800,
    VL_DSG_ETH_PROTOCOL_IPv6    = 0x86DD,
    VL_DSG_ETH_PROTOCOL_INVALID = 0xFFFF

}VL_DSG_ETH_PROTOCOL;

typedef struct tag_VL_DSG_ETH_HEADER
{
    VL_MAC_ADDRESS dstMacAddress;
    VL_MAC_ADDRESS srcMacAddress;
    VL_DSG_ETH_PROTOCOL eTypeCode;
}VL_DSG_ETH_HEADER;

typedef enum tag_VL_DSG_IP_PROTOCOL
{
    VL_DSG_IP_PROTOCOL_NONE     = 0x00,
    VL_DSG_IP_PROTOCOL_TCP      = 0x06,
    VL_DSG_IP_PROTOCOL_UDP      = 0x11,
    
    VL_DSG_IP_PROTOCOL_INVALID  = 0xFF
}VL_DSG_IP_PROTOCOL;

typedef enum tag_VL_DSG_ACC_RESULT
{
    VL_DSG_ACC_RESULT_NO_MATCH    = 0x00,
    VL_DSG_ACC_RESULT_NEW         = 0x01,
    VL_DSG_ACC_RESULT_ADDED       = 0x02,
    VL_DSG_ACC_RESULT_DISPATCHED  = 0x03,
    VL_DSG_ACC_RESULT_ERROR       = 0x04,
    VL_DSG_ACC_RESULT_DROPPED     = 0x05,

    VL_DSG_ACC_RESULT_INVALID     = 0xFF
}VL_DSG_ACC_RESULT;

typedef struct tag_VL_DSG_IP_HEADER
{
    unsigned char  version;
    unsigned char  headerLength;
    unsigned char  serviceType;
    unsigned short nTotalLength;
    unsigned short nIdentification;
    unsigned char  bDontFragment;
    unsigned char  bMoreFragments;
    unsigned short fragOffset;
    unsigned char  timeToLive;
    VL_DSG_IP_PROTOCOL eProtocol;
    unsigned short headerCRC;

    VL_IP_ADDRESS srcIpAddress;
    VL_IP_ADDRESS dstIpAddress;

}VL_DSG_IP_HEADER;

typedef struct tag_VL_DSG_TCP_HEADER
{
    unsigned short nSrcPort;
    unsigned short nDstPort;
    unsigned long  nSeqNum;
    unsigned long  nAckNum;
    unsigned char  nDataOffsetRsvd;
    unsigned char  nFlags;
    unsigned short nWindowSize;
    unsigned short nChecksum;
    unsigned short nUrgent;

}VL_DSG_TCP_HEADER;

typedef struct tag_VL_DSG_UDP_HEADER
{
    unsigned short nSrcPort;
    unsigned short nDstPort;
    unsigned short nLength;
    unsigned short nChecksum;

}VL_DSG_UDP_HEADER;

typedef struct tag_VL_DSG_BT_HEADER
{
    unsigned char  nHeaderStart;
    unsigned char  nVersion;
    unsigned char  lastSegment;
    unsigned char  segmentNumber;
    unsigned short segmentId;

}VL_DSG_BT_HEADER;

typedef struct tag_VL_DSG_BCAST_ACCUMULATOR
{
    unsigned long long      eBcastId;
    VL_IP_ADDRESS           srcIpAddress;
    VL_IP_ADDRESS           dstIpAddress;
    unsigned short          nSrcPort;
    unsigned short          nDstPort;
    unsigned short          segmentId;
    short                   iSegmentNumber;
    long                    bStartedAccumulation;
    long                    nBytesAccumulated;
    long                    nBytesCapacity;
    unsigned long           startTime;
    unsigned char         * pSection;

}VL_DSG_BCAST_ACCUMULATOR;

typedef struct tag_VL_DSG_IP_ACCUMULATOR
{
    unsigned char           version;
    unsigned short          nIdentification;
    VL_MAC_ADDRESS          dstMacAddress;
    VL_DSG_ETH_PROTOCOL     eTypeCode;
    VL_DSG_IP_PROTOCOL      eProtocol;
    VL_IP_ADDRESS           srcIpAddress;
    VL_IP_ADDRESS           dstIpAddress;
    long                    bStartedAccumulation;
    long                    iOffsetBegin;
    long                    iOffsetEnd;
    long                    nPayloadLength;
    long                    nBytesCaptured;
    long                    nBytesCapacity;
    unsigned long           startTime;
    unsigned char         * pPayload;

}VL_DSG_IP_ACCUMULATOR;

typedef struct tag_VL_DSG_IP_UNICAST_FLOW_PARAMS
{
    unsigned short sesnum;
    unsigned long flowType;
    unsigned long flowid;
    unsigned char macAddress[VL_MAC_ADDR_SIZE];
    unsigned char ipAddress[VL_IP_ADDR_SIZE];
    int           nOptionBytes;
    unsigned char * pOptionBytes;
}VL_DSG_IP_UNICAST_FLOW_PARAMS;

typedef struct tag_VL_DSG_IP_FLOW_PACKET
{
    unsigned long flowid;
    int           nLength;
    unsigned char * pData;
}VL_DSG_IP_FLOW_PACKET;

typedef enum tag_VL_DSG_TLV_217_TYPE
{
    VL_DSG_TLV_217_TYPE_NONE                                                    = 0x00,
    VL_DSG_TLV_217_TYPE_IpModeControl                                           = 0x01,
    VL_DSG_TLV_217_TYPE_VendorId                                                = 0x08,
    VL_DSG_TLV_217_TYPE_SNMPmibObject                                           = 0x0B,
    VL_DSG_TLV_217_TYPE_SNMPv3NotificationReceiver                              = 0x26,
    VL_DSG_TLV_217_TYPE_SNMPv3NotificationReceiver_IPv4Address                  = 0x2601,
    VL_DSG_TLV_217_TYPE_SNMPv3NotificationReceiver_UDPPortNumber                = 0x2602,
    VL_DSG_TLV_217_TYPE_SNMPv3NotificationReceiver_TrapType                     = 0x2603,
    VL_DSG_TLV_217_TYPE_SNMPv3NotificationReceiver_Timeout                      = 0x2604,
    VL_DSG_TLV_217_TYPE_SNMPv3NotificationReceiver_Retries                      = 0x2605,
    VL_DSG_TLV_217_TYPE_SNMPv3NotificationReceiver_FilteringParameters          = 0x2606,
    VL_DSG_TLV_217_TYPE_SNMPv3NotificationReceiver_SecurityName                 = 0x2607,
    VL_DSG_TLV_217_TYPE_SNMPv3NotificationReceiver_IPv6Address                  = 0x2608,
    VL_DSG_TLV_217_TYPE_VendorSpecific                                          = 0x2B,
    VL_DSG_TLV_217_TYPE_SNMPv1v2c                                               = 0x35,
    VL_DSG_TLV_217_TYPE_SNMPv1v2c_CommunityName                                 = 0x3501,
    VL_DSG_TLV_217_TYPE_SNMPv1v2c_TransportAddressAccess                        = 0x3502,
    VL_DSG_TLV_217_TYPE_SNMPv1v2c_TransportAddressAccess_TransportAddress       = 0x350201,
    VL_DSG_TLV_217_TYPE_SNMPv1v2c_TransportAddressAccess_TransportAddressMask   = 0x350202,
    VL_DSG_TLV_217_TYPE_SNMPv1v2c_AccessViewType                                = 0x3503,
    VL_DSG_TLV_217_TYPE_SNMPv1v2c_AccessViewName                                = 0x3504,
    VL_DSG_TLV_217_TYPE_SNMPv3_AccessViewConfig                                 = 0x36,
    VL_DSG_TLV_217_TYPE_SNMPv3_AccessViewConfig_AccessViewName                  = 0x3601,
    VL_DSG_TLV_217_TYPE_SNMPv3_AccessViewConfig_AccessViewSubtree               = 0x3602,
    VL_DSG_TLV_217_TYPE_SNMPv3_AccessViewConfig_AccessViewMask                  = 0x3603,
    VL_DSG_TLV_217_TYPE_SNMPv3_AccessViewConfig_AccessViewType                  = 0x3604,

    VL_DSG_TLV_217_TYPE_INVALID                     = 0xFF

}VL_DSG_TLV_217_TYPE;

#define VL_DSG_MAX_TLV_217_SIZE     0x100

typedef struct tag_VL_DSG_TLV_217_FIELD
{
    VL_DSG_TLV_217_TYPE eTag;
    int           nBytes;
    unsigned char aBytes[VL_DSG_MAX_TLV_217_SIZE];
}VL_DSG_TLV_217_FIELD;

typedef struct tag_VL_DSG_TRAFFIC_STATS
{
    unsigned long               nPkts         ;
    unsigned long               nOctets       ;
    unsigned long               tFirstPacket  ;
    unsigned long               tLastPacket   ;
    
}VL_DSG_TRAFFIC_STATS;

typedef struct tag_VL_DSG_TRAFFIC_TEXT_STATS
{
    const char    * pszFilterType;
    const char    * pszEntryType;
    const char    * pszClientType;
    const char    * pszClientId;
    char            szClientId[32];
    char            szPkts[16];
    unsigned long   nOctets       ;
    unsigned long   tFirstPacket  ;
    unsigned long   tLastPacket   ;
    
}VL_DSG_TRAFFIC_TEXT_STATS;

typedef struct tag_VL_VCT_ID_COUNTER
{
    unsigned long nVctId;
    unsigned long nCount;
    
}VL_VCT_ID_COUNTER;

typedef struct tag_VL_DSG_APDU_256
{
    unsigned short nSession;
    unsigned long  nBytes;
    unsigned char  aBytApdu[256];
}VL_DSG_APDU_256;
    
/*-----------------------------------------------------------------*/
#endif // __VL_DSG_TYPES_H__
/*-----------------------------------------------------------------*/
