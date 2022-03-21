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
#ifndef __IPDIRECT_UNICAST_TYPES_H__
#define __IPDIRECT_UNICAST_TYPES_H__
/*-----------------------------------------------------------------*/

#include "dsg_types.h"
#include "ip_types.h"

#define MAX_IPDIRECT_UNICAST_DEVICES          (1)
#define CM_IPDIRECT_UNICAST_DEVICE_INSTANCE   (0)

#define MAX_URL_STRING_LENGTH				          2083

typedef void *IPDIRECT_UNICAST_HANDLE;          // handle used by the application to access the API
typedef void *IPDIRECT_UNICAST_INSTANCEHANDLE;  // handle used by the API to access the driver instance (never used by the application)
typedef unsigned long IPDIRECT_UNICAST_DEVICE_HANDLE_t;

typedef enum _IPDIRECT_UNICAST_RESULT
{
    IPDIRECT_UNICAST_RESULT_SUCCESS               = 0,
    IPDIRECT_UNICAST_RESULT_NOT_IMPLEMENTED       = 1,
    IPDIRECT_UNICAST_RESULT_NULL_PARAM            = 2,
    IPDIRECT_UNICAST_RESULT_INVALID_PARAM         = 3,
    IPDIRECT_UNICAST_RESULT_INCOMPLETE_PARAM      = 4,
    IPDIRECT_UNICAST_RESULT_INVALID_IMAGE_TYPE    = 5,
    IPDIRECT_UNICAST_RESULT_INVALID_IMAGE_SIGN    = 6,
    IPDIRECT_UNICAST_RESULT_ECM_FAILURE           = 7,
    IPDIRECT_UNICAST_RESULT_ECM_SUCCESS           = 8,
    IPDIRECT_UNICAST_RESULT_HAL_SUPPORTS          = 9,
    IPDIRECT_UNICAST_RESULT_STATUS_OK             = 10,
    IPDIRECT_UNICAST_RESULT_STATUS_NOT_OK         = 11,

    IPDIRECT_UNICAST_RESULT_INVALID               = 0xFF
} IPDIRECT_UNICAST_RESULT;

typedef struct IPDIRECT_UNICAST_CAPABILITIES_INSTANCE_s
{
    IPDIRECT_UNICAST_DEVICE_HANDLE_t hIPDIRECT_UNICASTHandle;

} IPDIRECT_UNICAST_CAPABILITIES_INSTANCE_t;

typedef struct IPDIRECT_UNICAST_CAPABILITIES_s
{
    int                             usNumIPDIRECT_UNICASTs;
    IPDIRECT_UNICAST_CAPABILITIES_INSTANCE_t  astInstanceCapabilities[MAX_IPDIRECT_UNICAST_DEVICES];
} IPDIRECT_UNICAST_CAPABILITIES_t;

typedef enum tag_IPDIRECT_UNICAST_PVT_TAG
{
    IPDIRECT_UNICAST_PVT_TAG_NONE                 = 0x30000000,

    IPDIRECT_UNICAST_PVT_TAG_INVALID              = 0x3FFFFFFF

}IPDIRECT_UNICAST_PVT_TAG;

typedef enum tag_IPDIRECT_UNICAST_APDU_TAG
{
    IPDIRECT_UNICAST_APDU_ICN_PID_NOTIFY		  = 0x9FA300,
    IPDIRECT_UNICAST_APDU_HTTP_RECV_FLOW_REQ   = 0x9FA301,
    IPDIRECT_UNICAST_APDU_HTTP_SEND_FLOW_REQ   = 0x9FA302,
    IPDIRECT_UNICAST_APDU_HTTP_FLOW_CNF   = 0x9FA303,
    IPDIRECT_UNICAST_APDU_SOCKET_FLOW_REQ 		= 0x9FA304,
    IPDIRECT_UNICAST_APDU_SOCKET_FLOW_CNF 		= 0x9FA305,
    IPDIRECT_UNICAST_APDU_HTTP_DEL_FLOW_REQ   = 0x9FA306,
    IPDIRECT_UNICAST_APDU_HTTP_DEL_FLOW_CNF   = 0x9FA307,
    IPDIRECT_UNICAST_APDU_HTTP_LOST_FLOW_IND  = 0x9FA308,
    IPDIRECT_UNICAST_APDU_HTTP_LOST_FLOW_CNF  = 0x9FA309,
    IPDIRECT_UNICAST_APDU_HTTP_GET_REQ        = 0x9FA30A,
    IPDIRECT_UNICAST_APDU_HTTP_GET_CNF        = 0x9FA30B,
    IPDIRECT_UNICAST_APDU_HTTP_POST_CNF				= 0x9FA30C,
    IPDIRECT_UNICAST_APDU_HTTP_DATA_SEG_INFO  = 0x9FA30D,
    IPDIRECT_UNICAST_APDU_HTTP_RESEND_SEG_REQ = 0x9FA30E,
    IPDIRECT_UNICAST_APDU_HTTP_RECV_STATUS	  = 0x9FA30F,

    IPDIRECT_UNICAST_APDU_INVALID             = 0x9FA3FF

}IPDIRECT_UNICAST_APDU_TAG;


#define IPDIRECT_UNICAST_MAX_STR_SIZE     0x100   /* equals    256d */

typedef enum tag_IPDIRECT_UNICAST_FLOW_RESULT
{
    IPDIRECT_UNICAST_FLOW_RESULT_GRANTED                = 0x00, /* Request granted, new flow created                        */
    IPDIRECT_UNICAST_FLOW_RESULT_DENIED_FLOWS_EXCEEDED  = 0x01, /* Request denied, number of flows exceeded                 */
    IPDIRECT_UNICAST_FLOW_RESULT_DENIED_NO_SERVICE      = 0x02, /* Request denied, service_type not available               */
    IPDIRECT_UNICAST_FLOW_RESULT_DENIED_NO_NETWORK      = 0x03, /* Request denied, network unavailable or not responding    */
    IPDIRECT_UNICAST_FLOW_RESULT_DENIED_NET_BUSY        = 0x04, /* Request denied, network busy                             */
    IPDIRECT_UNICAST_FLOW_RESULT_DENIED_BAD_MAC_ADDR    = 0x05, /* Request denied, MAC address not accepted                 */
    IPDIRECT_UNICAST_FLOW_RESULT_DENIED_NO_DNS          = 0x06, /* Request denied, DNS not supported                        */
    IPDIRECT_UNICAST_FLOW_RESULT_DENIED_DNS_FAILED      = 0x07, /* Request denied, DNS lookup failed                        */
    IPDIRECT_UNICAST_FLOW_RESULT_DENIED_PORT_IN_USE     = 0x08, /* Request denied, local port already in use or invalid     */
    IPDIRECT_UNICAST_FLOW_RESULT_DENIED_TCP_FAILED      = 0x09, /* Request denied, could not establish TCP connection       */
    IPDIRECT_UNICAST_FLOW_RESULT_DENIED_NO_IPV6         = 0x0A, /* Request denied, IPv6 not supported                       */

    IPDIRECT_UNICAST_FLOW_RESULT_INVALID                = 0xFF
}IPDIRECT_UNICAST_FLOW_RESULT;

typedef enum tag_IPDIRECT_UNICAST_DELETE_FLOW_RESULT
{
    IPDIRECT_UNICAST_DELETE_FLOW_RESULT_GRANTED         = 0x00, /* Request granted, flow deleted                            */
    IPDIRECT_UNICAST_DELETE_FLOW_RESULT_RESERVED_1      = 0x01, /* Reserved 1                                               */
    IPDIRECT_UNICAST_DELETE_FLOW_RESULT_RESERVED_2      = 0x02, /* Reserved 2                                               */
    IPDIRECT_UNICAST_DELETE_FLOW_DENIED_NO_NETWORK      = 0x03, /* Request denied, network unavailable or not responding    */
    IPDIRECT_UNICAST_DELETE_FLOW_DENIED_NET_BUSY        = 0x04, /* Request denied, network busy                             */
    IPDIRECT_UNICAST_DELETE_FLOW_RESULT_NO_FLOW_EXISTS  = 0x05, /* Request denied, flow_id does not exist                   */
    IPDIRECT_UNICAST_DELETE_FLOW_DENIED_NOT_AUTHORIZED  = 0x06, /* Request granted, not authorized                          */

    IPDIRECT_UNICAST_DELETE_FLOW_RESULT_INVALID         = 0xFF
}IPDIRECT_UNICAST_DELETE_FLOW_RESULT;

typedef enum tag_IPDIRECT_UNICAST_LOST_FLOW_REASON
{
    IPDIRECT_UNICAST_LOST_FLOW_REASON_NOT_SPECIFIED         = 0x00,
    IPDIRECT_UNICAST_LOST_FLOW_REASON_LEASE_EXPIRED         = 0x01,
    IPDIRECT_UNICAST_LOST_FLOW_REASON_NET_BUSY_OR_DOWN      = 0x02,
    IPDIRECT_UNICAST_LOST_FLOW_REASON_LOST_OR_REVOKED_AUTH  = 0x03,
    IPDIRECT_UNICAST_LOST_FLOW_REASON_TCP_SOCKET_CLOSED     = 0x04,
    IPDIRECT_UNICAST_LOST_FLOW_REASON_SOCKET_READ_ERROR     = 0x05,
    IPDIRECT_UNICAST_LOST_FLOW_REASON_SOCKET_WRITE_ERROR    = 0x06,

    IPDIRECT_UNICAST_LOST_FLOW_REASON_INVALID               = 0xFF
}IPDIRECT_UNICAST_LOST_FLOW_REASON;

typedef enum tag_IPDIRECT_UNICAST_LOST_FLOW_RESULT
{
    IPDIRECT_UNICAST_LOST_FLOW_RESULT_ACKNOWLEDGED      = 0x00, /* Indication acknowledged          */
    IPDIRECT_UNICAST_LOST_FLOW_RESULT_RESERVED_1        = 0x01, /* Reserved 1                                               */

    IPDIRECT_UNICAST_LOST_FLOW_RESULT_INVALID           = 0xFF
}IPDIRECT_UNICAST_LOST_FLOW_RESULT;

typedef enum tag_IPDIRECT_UNICAST_SERVER_ADDRESS_TYPE
{
    IPDIRECT_UNICAST_SERVER_ADDRESS_TYPE_NAME = 0x00,
    IPDIRECT_UNICAST_SERVER_ADDRESS_TYPE_IPV4 = 0x01,
    IPDIRECT_UNICAST_SERVER_ADDRESS_TYPE_IPV6 = 0x02,
    IPDIRECT_UNICAST_SERVER_ADDRESS_TYPE_BOTH = 0x03,

    IPDIRECT_UNICAST_SERVER_ADDRESS_TYPE_INVALID = 0xFF
}IPDIRECT_UNICAST_SERVER_ADDRESS_TYPE;

typedef enum tag_IPDIRECT_UNICAST_HTTP_PROCESS_BY
{
    IPDIRECT_UNICAST_HTTP_PROCESS_BY_NONE    = 0x00,
    IPDIRECT_UNICAST_HTTP_PROCESS_BY_CARD    = 0x01,
    IPDIRECT_UNICAST_HTTP_PROCESS_BY_HOST    = 0x02,
    IPDIRECT_UNICAST_HTTP_PROCESS_BY_BOTH    = 0x03,

    IPDIRECT_UNICAST_HTTP_PROCESS_BY_INVALID = 0xFF
}IPDIRECT_UNICAST_HTTP_PROCESS_BY;

typedef enum tag_IPDIRECT_UNICAST_GET_STATUS
{
    IPDIRECT_UNICAST_GET_STATUS_NONE                                 = 0,
    IPDIRECT_UNICAST_GET_STATUS_CONTINUE                             = 100,
    IPDIRECT_UNICAST_GET_STATUS_SWITCHING_PROTOCOLS                  = 101,
    IPDIRECT_UNICAST_GET_STATUS_PROCESSING                           = 102,
    IPDIRECT_UNICAST_GET_STATUS_OK                                   = 200,
    IPDIRECT_UNICAST_GET_STATUS_CREATED                              = 201,
    IPDIRECT_UNICAST_GET_STATUS_ACCEPTED                             = 202,
    IPDIRECT_UNICAST_GET_STATUS_NON_AUTHORITATIVE_INFORMATION        = 203,
    IPDIRECT_UNICAST_GET_STATUS_NO_CONTENT                           = 204,
    IPDIRECT_UNICAST_GET_STATUS_RESET_CONTENT                        = 205,
    IPDIRECT_UNICAST_GET_STATUS_PARTIAL_CONTENT                      = 206,
    IPDIRECT_UNICAST_GET_STATUS_MULTI_STATUS                         = 207,
    IPDIRECT_UNICAST_GET_STATUS_ALREADY_REPORTED                     = 208,
    IPDIRECT_UNICAST_GET_STATUS_IM_USED                              = 226,
    IPDIRECT_UNICAST_GET_STATUS_MULTIPLE_CHOICES                     = 300,
    IPDIRECT_UNICAST_GET_STATUS_MOVED_PERMANENTLY                    = 301,
    IPDIRECT_UNICAST_GET_STATUS_FOUND                                = 302,
    IPDIRECT_UNICAST_GET_STATUS_SEE_OTHER                            = 303,
    IPDIRECT_UNICAST_GET_STATUS_NOT_MODIFIED                         = 304,
    IPDIRECT_UNICAST_GET_STATUS_USE_PROXY                            = 305,
    IPDIRECT_UNICAST_GET_STATUS_SWITCH_PROXY                         = 306,
    IPDIRECT_UNICAST_GET_STATUS_TEMPORARY_REDIRECT                   = 307,
    IPDIRECT_UNICAST_GET_STATUS_PERMANENT_REDIRECT                   = 308,
    IPDIRECT_UNICAST_GET_STATUS_RESUME_INCOMPLETE                    = 308,
    IPDIRECT_UNICAST_GET_STATUS_BAD_REQUEST                          = 400,
    IPDIRECT_UNICAST_GET_STATUS_UNAUTHORIZED                         = 401,
    IPDIRECT_UNICAST_GET_STATUS_PAYMENT_REQUIRED                     = 402,
    IPDIRECT_UNICAST_GET_STATUS_FORBIDDEN                            = 403,
    IPDIRECT_UNICAST_GET_STATUS_NOT_FOUND                            = 404,
    IPDIRECT_UNICAST_GET_STATUS_METHOD_NOT_ALLOWED                   = 405,
    IPDIRECT_UNICAST_GET_STATUS_NOT_ACCEPTABLE                       = 406,
    IPDIRECT_UNICAST_GET_STATUS_PROXY_AUTHENTICATION_REQUIRED        = 407,
    IPDIRECT_UNICAST_GET_STATUS_REQUEST_TIMEOUT                      = 408,
    IPDIRECT_UNICAST_GET_STATUS_CONFLICT                             = 409,
    IPDIRECT_UNICAST_GET_STATUS_GONE                                 = 410,
    IPDIRECT_UNICAST_GET_STATUS_LENGTH_REQUIRED                      = 411,
    IPDIRECT_UNICAST_GET_STATUS_PRECONDITION_FAILED                  = 412,
    IPDIRECT_UNICAST_GET_STATUS_REQUEST_ENTITY_TOO_LARGE             = 413,
    IPDIRECT_UNICAST_GET_STATUS_REQUEST_URI_TOO_LONG                 = 414,
    IPDIRECT_UNICAST_GET_STATUS_UNSUPPORTED_MEDIA_TYPE               = 415,
    IPDIRECT_UNICAST_GET_STATUS_REQUESTED_RANGE_NOT_SATISFIABLE      = 416,
    IPDIRECT_UNICAST_GET_STATUS_EXPECTATION_FAILED                   = 417,
    IPDIRECT_UNICAST_GET_STATUS_I_M_A_TEAPOT                         = 418,
    IPDIRECT_UNICAST_GET_STATUS_AUTHENTICATION_TIMEOUT               = 419,
    IPDIRECT_UNICAST_GET_STATUS_METHOD_FAILURE                       = 420,
    IPDIRECT_UNICAST_GET_STATUS_ENHANCE_YOUR_CALM                    = 420,
    IPDIRECT_UNICAST_GET_STATUS_MISDIRECTED_REQUEST                  = 421,
    IPDIRECT_UNICAST_GET_STATUS_UNPROCESSABLE_ENTITY                 = 422,
    IPDIRECT_UNICAST_GET_STATUS_LOCKED                               = 423,
    IPDIRECT_UNICAST_GET_STATUS_FAILED_DEPENDENCY                    = 424,
    IPDIRECT_UNICAST_GET_STATUS_UPGRADE_REQUIRED                     = 426,
    IPDIRECT_UNICAST_GET_STATUS_PRECONDITION_REQUIRED                = 428,
    IPDIRECT_UNICAST_GET_STATUS_TOO_MANY_REQUESTS                    = 429,
    IPDIRECT_UNICAST_GET_STATUS_REQUEST_HEADER_FIELDS_TOO_LARGE      = 431,
    IPDIRECT_UNICAST_GET_STATUS_LOGIN_TIMEOUT                        = 440,
    IPDIRECT_UNICAST_GET_STATUS_NO_RESPONSE                          = 444,
    IPDIRECT_UNICAST_GET_STATUS_RETRY_WITH                           = 449,
    IPDIRECT_UNICAST_GET_STATUS_BLOCKED_BY_WINDOWS_PARENTAL_CONTROLS = 450,
    IPDIRECT_UNICAST_GET_STATUS_UNAVAILABLE_FOR_LEGAL_REASONS        = 451,
    IPDIRECT_UNICAST_GET_STATUS_REDIRECT                             = 451,
    IPDIRECT_UNICAST_GET_STATUS_REQUEST_HEADER_TOO_LARGE             = 494,
    IPDIRECT_UNICAST_GET_STATUS_CERT_ERROR                           = 495,
    IPDIRECT_UNICAST_GET_STATUS_NO_CERT                              = 496,
    IPDIRECT_UNICAST_GET_STATUS_HTTP_TO_HTTPS                        = 497,
    IPDIRECT_UNICAST_GET_STATUS_TOKEN_EXPIRED_OR_INVALID             = 498,
    IPDIRECT_UNICAST_GET_STATUS_CLIENT_CLOSED_REQUEST                = 499,
    IPDIRECT_UNICAST_GET_STATUS_TOKEN_REQUIRED                       = 499,
    IPDIRECT_UNICAST_GET_STATUS_INTERNAL_SERVER_ERROR                = 500,
    IPDIRECT_UNICAST_GET_STATUS_NOT_IMPLEMENTED                      = 501,
    IPDIRECT_UNICAST_GET_STATUS_BAD_GATEWAY                          = 502,
    IPDIRECT_UNICAST_GET_STATUS_SERVICE_UNAVAILABLE                  = 503,
    IPDIRECT_UNICAST_GET_STATUS_GATEWAY_TIMEOUT                      = 504,
    IPDIRECT_UNICAST_GET_STATUS_HTTP_VERSION_NOT_SUPPORTED           = 505,
    IPDIRECT_UNICAST_GET_STATUS_VARIANT_ALSO_NEGOTIATES              = 506,
    IPDIRECT_UNICAST_GET_STATUS_INSUFFICIENT_STORAGE                 = 507,
    IPDIRECT_UNICAST_GET_STATUS_LOOP_DETECTED                        = 508,
    IPDIRECT_UNICAST_GET_STATUS_BANDWIDTH_LIMIT_EXCEEDED             = 509,
    IPDIRECT_UNICAST_GET_STATUS_NOT_EXTENDED                         = 510,
    IPDIRECT_UNICAST_GET_STATUS_NETWORK_AUTHENTICATION_REQUIRED      = 511,
    IPDIRECT_UNICAST_GET_STATUS_UNKNOWN_ERROR                        = 520,
    IPDIRECT_UNICAST_GET_STATUS_NETWORK_READ_TIMEOUT_ERROR           = 598,
    IPDIRECT_UNICAST_GET_STATUS_NETWORK_CONNECT_TIMEOUT_ERROR        = 599,

    IPDIRECT_UNICAST_GET_STATUS_INVALID                              = 0xFFFF
}IPDIRECT_UNICAST_GET_STATUS;

#define IP_DIRECT_HTTP_PATH_NAME_SIZE   256
#define IP_DIRECT_HTTP_FILE_NAME_SIZE   256

typedef struct tag_IPDIRECT_UNICAST_HTTP_GET_PARAMS
{
    unsigned char  bInUse;
    unsigned short sesnum;
    unsigned long  flowid;
    unsigned long  socket;
    unsigned char  remote_addr[32];
    unsigned long  nRemoteAddrLen;
    
    IPDIRECT_UNICAST_HTTP_PROCESS_BY     eProcessBy;
    IPDIRECT_UNICAST_SERVER_ADDRESS_TYPE eRemoteAddressType;
    
    unsigned short                  nLocalPortNum;
    unsigned short                  nRemotePortNum;
    unsigned char                   nDnsNameLen;
    char                            dnsAddress [VL_DNS_ADDR_SIZE ];
    unsigned char                   ipAddress  [VL_IP_ADDR_SIZE  ];
    unsigned char                   ipV6address[VL_IPV6_ADDR_SIZE];
    unsigned char                   nPathNameLen;
    unsigned char                   nFileNameLen;
    char                            pathName   [IP_DIRECT_HTTP_PATH_NAME_SIZE];
    char                            fileName   [IP_DIRECT_HTTP_FILE_NAME_SIZE];
    //IPDU Int Spec 1.6b
    unsigned char                   transactionTimeout;         /* Actual data xfer timeout. */
    unsigned char                   connTimeout;                /* Connection timeout. */
    unsigned long int               timeoutThreadId;
    unsigned short                  isSiDataRequest;            /* True identifies the CC request as a SI data get. */
                                                                /* False = Non-si data get. */
    unsigned short                  transactionId;
}IPDIRECT_UNICAST_HTTP_GET_PARAMS;

typedef enum tag_IPDIRECT_UNICAST_TRANSFER_STATUS
{
    IPDIRECT_UNICAST_TRANSFER_STATUS_SUCCESSFUL = 0x00,
    IPDIRECT_UNICAST_TRANSFER_STATUS_START_OVER = 0x01,

    IPDIRECT_UNICAST_TRANSFER_STATUS_INVALID    = 0xFF
}IPDIRECT_UNICAST_TRANSFER_STATUS;

typedef struct tag_IPDIRECT_UNICAST_CMD
{
    IPDIRECT_UNICAST_APDU_TAG eTag;
    unsigned long  nParam;
}IPDIRECT_UNICAST_CMD;

#define IPDIRECT_UNICAST_UNUSED_VAR(var) (void)(var)

/*-----------------------------------------------------------------*/
#endif // __IPDIRECT_UNICAST_TYPES_H__
/*-----------------------------------------------------------------*/
