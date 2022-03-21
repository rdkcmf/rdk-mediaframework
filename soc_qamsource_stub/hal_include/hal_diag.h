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


#ifndef __HAL_DIAG_H__
#define __HAL_DIAG_H__

/**
 * @file hal_diag.h
 *
 * @defgroup HAL_DIAG_Common_Types  Diagnostic Common Types
 * Describes the data structures, enums and defines used for the diagnostic purpose.
 * These includes the structures for storing memory information, network information,
 * File system details, Transmitter signal information, ECM information etc.
 * @ingroup  RMF_HAL_Types
 * @{
 */

#ifdef __cplusplus
extern "C"
{
#endif

#define MAX_IP_BYTES 32                 /** Maximum IP Packet size */
#define MAX_CLIENTS  32                 /** Maximum number of clients can be connected */

#define MAX_1394_DEVICES    63           /** Maximum devices that can be connected to IEEE 1394 port */
#define MAX_MODEL_NAME_LEN        128    /** Maximum Length of model name */
#define MAX_VENDOR_NAME_LEN        128   /** Maximum Vendor name length */
#define MAX_NET_ADDRS 10                 /** Maximum number of IP address */
#define MAX_MAC_ADDRESS_LEN        16    /** Maximum MAC address length */
#define DIAG_APP_NAME_MAX_LEN    256     /** Maximum Length of Diagnostic Application */
#define DIAG_APP_SIGN_MAX_LEN    256     /** Application signature maximum length */
#define DIAG_MAX_NUM_OF_APPS     100     /** Maximum number of diagnostic applications */

/**
 * @brief Diagnostics API status.
 */
typedef    enum
{
    HAL_DIAG_API_STATUS_SUCCESS,
    HAL_DIAG_API_STATUS_FAILURE,
    HAL_DIAG_API_STATUS_NOT_SUPPORTED,
    HAL_DIAG_API_STATUS_NULL_PARAMETER,
    HAL_DIAG_API_STATUS_NOT_IMPLEMENTED
}HAL_DIAG_API_STATUS;

/**
 * @brief Type of device associated with reported MAC addres.
 */
typedef enum
{
    MAC_ADDRESS_TYPE_NONE,
    MAC_ADDRESS_TYPE_HOST,
    MAC_ADDRESS_TYPE_DOCSIS,
    MAC_ADDRESS_TYPE_MAX
}MAC_ADDRESS_TYPE;

/**
 * @brief Type of memory used by the host system.
 */
typedef enum {

    HOST_MEMORY_TYPE_MAX

}HOST_MEMORY_TYPE;

/**
 * @brief Represents the connection details about the client.
 */
typedef struct
{
    unsigned char client_mac_address[6];
    unsigned char number_of_bytes_net;
    unsigned char client_IP_address_byte[MAX_IP_BYTES];
    unsigned char client_DRM_status;
}CntClients_t;

/**
 * @brief Details about the home network info.
 */
typedef struct
{
    unsigned char max_clients;
    unsigned char host_DRM_status;
    unsigned char connected_clients;
    CntClients_t  Client[MAX_CLIENTS];
}HomeNetworkInfo_t;

/**
 * @brief Represents the host information.
 */
typedef struct sHostInformation
{
    unsigned char vendor_name_length;
    unsigned char vendor_name_character[MAX_VENDOR_NAME_LEN];
    unsigned char model_name_length;
    unsigned char model_name_character[MAX_MODEL_NAME_LEN];
}sHostInformation;

/**
 * @brief Represents the network address info.
 */
typedef struct
{
    MAC_ADDRESS_TYPE    net_address_type;
    unsigned char        number_of_bytes_net;
    unsigned char        net_address_byte[MAX_MAC_ADDRESS_LEN];

    unsigned char        number_of_bytes_subnet;
    unsigned char         sub_net_address_byte[MAX_MAC_ADDRESS_LEN];
  }netaddr_t;

/**
 * @brief Represents the OCHD2 network info.
 */
typedef struct sOCHD2NetAddressInfo
{
    unsigned char number_of_addresses;


    netaddr_t netaddr[MAX_NET_ADDRS];
}sOCHD2NetAddressInfo;

/**
 * @brief Status of RDC, includes  RDC center frequency in MHz,
 * RDC transmitter power level  in dBmV
 * and RDC data rate in kbps.
 */
typedef struct    sRDCInfo
{
    unsigned short    RDC_center_freq;
    unsigned char    RDC_transmitter_power_level;
    unsigned char    RDC_data_rate;

}sRDCInfo;

/**
 * @brief Hdmi port status report.
 */
typedef struct _s_HDMIPortStatus
{
    unsigned char    device_type;
    unsigned char    color_space;
    unsigned char    connection_status;
    unsigned char    host_HDCP_status;
    unsigned char    device_HDCP_status;

        unsigned short    horizontal_lines;
        unsigned short    vertical_lines;
        unsigned char    frame_rate;
        unsigned char    aspect_ratio;
        unsigned char    prog_inter_type;

        unsigned char    audio_sample_size;
        unsigned char    audio_format;
        unsigned char    audio_sample_freq;

}s_HDMIPortStatus;

/**
 * @brief Represents the ECM status information.
 */
typedef struct _s_eCMStatusInfo
{
    unsigned short    downstream_center_freq;
    unsigned short    downstream_power_level;
    unsigned char    downstream_carrier_lock_status;
    unsigned char    channel_s_cdma_status;
    unsigned char    upstream_modulation_type;
    unsigned short    upstream_xmt_center_freq;
    unsigned short    upstream_power_level;
    unsigned char    upstream_symbol_rate;

}s_eCMStatusInfo;

/**
 * @brief Represents the DVI(Digital Video Interface) info.
 */
typedef struct DVIInfo_s
{
    unsigned char connection_status;
    unsigned char host_HDCP_status;
    unsigned char device_HDCP_status;

       unsigned short   horizontal_lines;
       unsigned short    vertical_lines;
       unsigned char frame_rate;
       unsigned char    aspect_ratio;
      unsigned char prog_inter_type;

}DVIInfo_t;

/**
 * @brief Represents the device info.
 */
typedef struct deviceInfo
{
        unsigned char device_subunit_type;
        unsigned char device_a_d_source_selection_status;
        unsigned long eui_64_hi;
        unsigned long eui_64_lo;

}DeviceInfo_t;

/**
 * @brief Represents the IEEE 1394 port info.
 */
typedef struct sI1394PortInfo{

    unsigned char  loop_status;
    unsigned char  root_status;
    unsigned char  cycle_master_status;
    unsigned char  host_a_d_source_selection_status;
    unsigned char  port_1_connection_status;
    unsigned char  port_2_connection_status;
    unsigned short total_number_of_nodes;
    unsigned char number_of_connected_devices;
    DeviceInfo_t DeviceInfo[MAX_1394_DEVICES];

}sI1394PortInfo;

/**
 * @brief Represents the current channel information.
 */
typedef struct sCurrentChannelInfo
{
    unsigned char channel_type;
    unsigned char authorization_flag;
    unsigned char purchasable_flag;
    unsigned char purchased_flag;
    unsigned char preview_flag;
    unsigned char parental_control_flag;
    unsigned short current_channel;
}sCurrentChannelInfo_t;

/**
 * @brief Represents the FDC status information.
 */
typedef struct sFDCInfo
{
    unsigned short    FDC_center_freq;
    unsigned char    carrier_lock_status;

}sFDCInfo;

/**
 * @brief Represents the FAT information.
 */
typedef struct sFATInfo
{
    unsigned char    PCR_lock;            //00b - Not Locked, 01b - Locked
    unsigned char    modulation_mode;    //00b - Analog, 01b - QAM64, 10b - QAM256, 11b - other
    unsigned char    carrier_lock_status;//00b - Not Locked, 01b - Locked
    unsigned short    SNR;                //Signal to Noise Ratio in tenths of dB
    unsigned short    signal_level;        //value in tenths of dB
}sFATInfo;

/**
 * @brief Represents the mac address information.
 */
typedef struct _sMacAddrInfo
{
    MAC_ADDRESS_TYPE    macAddrType;
    unsigned char        macAddrLen;
    unsigned char        macAddr[MAX_MAC_ADDRESS_LEN];

}sMacAddrInfo;

/**
 * @brief Represents the mac report details.
 */
typedef struct _sMacAddrReport
{
    unsigned char    numMACAddresses;
    sMacAddrInfo    macAddrInfo[MAC_ADDRESS_TYPE_MAX];

}sMacAddrReport;

/**
 * @brief Represents the Application details like Version number, Status, Application name,
 * Application signature,  .
 */
typedef struct sft_ver_rep_s
{
    unsigned short    application_version_number;
    unsigned char application_status_flag;
    unsigned char application_name_length;
    unsigned char application_name_byte[DIAG_APP_NAME_MAX_LEN];
    unsigned char application_sign_length;
    unsigned char application_sign_byte[DIAG_APP_SIGN_MAX_LEN];
}softwareVer_t;

/**
 * @brief Represents the software version info.
 */
typedef struct softwareVerInfo_s
{
    unsigned char number_of_applications;
    softwareVer_t SoftVer[DIAG_MAX_NUM_OF_APPS];
}softwareVerInfo_t;

/**
 * @brief Represents the host memory allocation info.
 */
typedef struct _sHostMemoryAllocInfo
{
    unsigned char    numMemTypes;
    struct
    {
        HOST_MEMORY_TYPE    memType;
        unsigned long        memSizeKBytes;
    }memoryInfo[HOST_MEMORY_TYPE_MAX];

}sHostMemoryAllocInfo;

#ifdef __cplusplus
}
#endif


#endif /*__HAL_DIAG_H__*/
