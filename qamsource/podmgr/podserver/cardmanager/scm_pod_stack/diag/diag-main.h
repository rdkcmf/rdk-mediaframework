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
#ifndef _DIAG_MAIN_H_

#define _DIAG_MAIN_H_
#ifdef __cplusplus
extern "C" {
#endif
unsigned long vldiagGetOpenedRsId();
#ifdef __cplusplus
}
#endif
typedef enum  Diag_Ids_t
{
    DIAG_HOST_MEM_ALLOC,//0x00
    DIAG_APP_VER_NUM,//0x01
    DIAG_FIRM_VER,//0x02
    DIAG_MAC_ADDR,//0x03
    DIAG_FAT_STATUS,//0x04
    DIAG_FDC_STATUS,//0x05
    DIAG_CUR_CHNL_REP,//0x06
    DIAG_1394_PORT,//0x07
    DIAG_DVI_STARUS,//0x08
    DIAG_ECM,//0x09
    DIAG_HDMI_PORT_STATUS,//0x0a
    DIAG_RDC_STATUS,//0x0b
    DIAG_OCHD2_NET_STATUS,//0x0c
    DIAG_HOME_NET_STATUS,//0x0d
    DIAG_HOST_INFO,//0x0e
    DIAG_MAX_TYPES
}Diag_Ids_s;

typedef enum Diag_status
{
    DIAG_GRANTED,
    DIAG_NOT_IMPLEMENTED,
    DIAG_DEV_BUSY,
    DIAG_OTHER_REASON,
}Diag_status_e;
typedef struct mem_types_s
{
    unsigned char memType;
    unsigned long memSize;
}mem_types_t;
typedef struct mem_rep_s
{
    unsigned char numMemTypes;
    mem_types_t MemTypes[DIAG_MAX_TYPES];
}memory_report_t;
#if 0
#define DIAG_APP_NAME_MAX_LEN    256
#define DIAG_APP_SIGN_MAX_LEN    256
#define DIAG_MAX_NUM_OF_APPS     100

typedef struct sft_ver_rep_s
{
    unsigned short    application_version_number;
    unsigned char application_status_flag;
    unsigned char application_name_length;
    unsigned char application_name_byte[DIAG_APP_NAME_MAX_LEN];
    unsigned char application_sign_length;
    unsigned char application_sign_byte[DIAG_APP_SIGN_MAX_LEN];
}sft_ver_rep_t;
typedef struct software_ver_report_s
{
    unsigned char number_of_applications;
    sft_ver_rep_t SorftVer[DIAG_MAX_NUM_OF_APPS];
}software_ver_report_t;
#endif
typedef struct firmware_ver_report_s
{
    unsigned char firmware_version_len;
    unsigned char firmware_version_bytes[256];
    unsigned short firmware_year;
    unsigned char firmware_month;
    unsigned char firmware_day;
    
}firmware_ver_report_t;

#endif //_DIAG_MAIN_H_
