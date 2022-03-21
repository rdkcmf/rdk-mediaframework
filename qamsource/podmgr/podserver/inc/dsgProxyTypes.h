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



#ifndef _VL_DSG_PROXY_TYPES_H_
#define _VL_DSG_PROXY_TYPES_H_

#define DSGPROXY_MAX_STR_SIZE               256

#define DSGPROXY_PUBLIC_KEY                 "DSGPROXY_DCD_BUFFER"

#define DSGPROXY_DCD_BUFFER                 "DSGPROXY_DCD_BUFFER"
#define DSGPROXY_DIRECTORY_BUFFER           "DSGPROXY_DIRECTORY_BUFFER"
#define DSGPROXY_ADV_CONFIG_BUFFER          "DSGPROXY_ADV_CONFIG_BUFFER"
#define DSGPROXY_TLV_217_BUFFER             "DSGPROXY_TLV_217_BUFFER"
    
#define DSGPROXY_HOST_MAC_ADDRESS           "DSGPROXY_HOST_MAC_ADDRESS"
#define DSGPROXY_HOST_IPV4_ADDRESS          "DSGPROXY_HOST_IPV4_ADDRESS"
#define DSGPROXY_HOST_IPV4_MASK             "DSGPROXY_HOST_IPV4_MASK"
#define DSGPROXY_HOST_IPV6_GLOBAL_ADDRESS   "DSGPROXY_HOST_IPV6_GLOBAL_ADDRESS"
#define DSGPROXY_HOST_IPV6_GLOBAL_PLEN      "DSGPROXY_HOST_IPV6_GLOBAL_PLEN"
#define DSGPROXY_HOST_IPV6_LINK_ADDRESS     "DSGPROXY_HOST_IPV6_LINK_ADDRESS"
#define DSGPROXY_HOST_IPV6_LINK_PLEN        "DSGPROXY_HOST_IPV6_LINK_PLEN"
#define DSGPROXY_HOST_TIME_ZONE             "DSGPROXY_HOST_TIME_ZONE"
   
#define DSGPROXY_ECM_MAC_ADDRESS            "DSGPROXY_ECM_MAC_ADDRESS"
#define DSGPROXY_ECM_IPV4_ADDRESS           "DSGPROXY_ECM_IPV4_ADDRESS"
#define DSGPROXY_ECM_IPV4_MASK              "DSGPROXY_ECM_IPV4_MASK"
#define DSGPROXY_ECM_IPV6_ADDRESS           "DSGPROXY_ECM_IPV6_ADDRESS"
#define DSGPROXY_ECM_IPV6_MASK              "DSGPROXY_ECM_IPV6_MASK"
    
#define DSGPROXY_ECM_DS_FREQUENCY           "DSGPROXY_ECM_DS_FREQUENCY"
#define DSGPROXY_ECM_DS_POWER               "DSGPROXY_ECM_DS_POWER"
#define DSGPROXY_ECM_DS_POWER_UNITS         "dBmV"
#define DSGPROXY_ECM_DS_LOCK_STATUS         "DSGPROXY_ECM_DS_LOCK_STATUS"
#define DSGPROXY_ECM_DS_STATUS_LOCKED       "locked"
#define DSGPROXY_ECM_DS_STATUS_UNLOCKED     "unlocked"
#define DSGPROXY_ECM_US_FREQUENCY           "DSGPROXY_ECM_US_FREQUENCY"
#define DSGPROXY_ECM_US_POWER               "DSGPROXY_ECM_US_POWER"
#define DSGPROXY_ECM_DS_SNR                 "DSGPROXY_ECM_DS_SNR"
#define DSGPROXY_ECM_SNR_UNITS              "dB"

#define DSGPROXY_UCID                       "DSGPROXY_UCID"
#define DSGPROXY_VCTID                      "DSGPROXY_VCTID"

#define DSGPROXY_CA_SYSTEM_ID               "DSGPROXY_CA_SYSTEM_ID"
#define DSGPROXY_CP_SYSTEM_ID               "DSGPROXY_CP_SYSTEM_ID"
#define DSGPROXY_DSG_APP_IDS                "DSGPROXY_DSG_APP_IDS"

#define DSGPROXY_RESOLVE_CONF_FILE          "/etc/resolv.conf"
#define DSGPROXY_RESOLVE_CONF               "DSGPROXY_RESOLVE_CONF"
#define DSGPROXY_RESOLVE_DNSMASQ_FILE       "/etc/resolv.dnsmasq"
#define DSGPROXY_RESOLVE_DNSMASQ            "DSGPROXY_RESOLVE_DNSMASQ"

#define DSGPROXY_HN_IF_LIST                 "FEATURE.HN.IF.LIST"
#define DSGPROXY_HN_IF_CONF_FILE            "/tmp/dsgproxy_hn_ifs.txt"
#define DSGPROXY_GATEWAY_DOMAIN             "FEATURE.GATEWAY.DOMAIN"
    
#define DSGPROXY_SLP_ATTR_FILE              "/tmp/dsgproxy_slp_attributes.txt"

#define DSGPROXY_SLPD_PID_FILE              "/tmp/dsgproxy_slp.pid"
#define DSGPROXY_SLPD_CONF_FILE             "/tmp/slp.conf"
#define DSGPROXY_SLPD_LOG_FILE              "/opt/logs/vlDsgProxyServerSlpdLog.txt"
#define DSGPROXY_OPENVPN_LOG_FILE           "/opt/logs/vlDsgProxyServerOpenVpnLog.txt"
    
#define DSGPROXY_SLP_SERVICE_FILE           "/tmp/dsgproxy_slp_service.txt"
#define DSGPROXY_SERVER_IP_INFO_FILE        "/tmp/dsgproxy_server_ip_info.txt"
#define DSGPROXY_CLIENT_IP_INFO_FILE        "/tmp/dsgproxy_client_ip_info.txt"
#define DSGPROXY_CLIENT_SERVER_ATTR_FILE    "/tmp/dsgproxy_client_server_attributes.txt"

#define DSGPROXY_SERVER_DOCSIS_STATUS_FILE  "/tmp/dsgproxy_server_docsis_status.txt"
#define DSGPROXY_SERVER_DOCSIS_READY_STATUS "Operational"

#define DSGPROXY_SERVER_TWO_WAY_STATUS_FILE  "/tmp/dsgproxy_server_two_way_status.txt"
#define DSGPROXY_SERVER_TWO_WAY_READY_STATUS "2-Way OK"

#define DSGPROXY_CLIENT_STATUS_FILE         "/tmp/dsgproxy_client_status.txt"
#define DSGPROXY_CLIENT_DISCONNECTED_STATUS "Not Connected"
#define DSGPROXY_CLIENT_CONNECTED_STATUS    "Connected"

#define DSGPROXY_BCAST_MULTICAST_ADDRESS_HEX 0xEFEFEFEF
#define DSGPROXY_APP_MULTICAST_ADDRESS_HEX   0xEFEFEFEE
#define DSGPROXY_CA_MULTICAST_ADDRESS_HEX    0xEFEFEFED
#define DSGPROXY_SERVER_VPN_IF_ADDRESS       "10.8.0.1"

#define DSGPROXY_ENABLE_TUNNEL_FORWARDING    "FEATURE.DSG_PROXY.ENABLE_TUNNEL_FORWARDING"
#define DSGPROXY_ENABLE_TUNNEL_RECEIVING     "FEATURE.DSG_PROXY.ENABLE_TUNNEL_RECEIVING"

#define DSGPROXY_MULTICAST_SCTE65_PORT      1
#define DSGPROXY_MULTICAST_SCTE18_PORT      2
#define DSGPROXY_MULTICAST_XAIT_PORT        5

#endif // _VL_DSG_PROXY_TYPES_H_
