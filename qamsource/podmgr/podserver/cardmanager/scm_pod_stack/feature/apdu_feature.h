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

#ifndef _APDU_FEATURE_H_
#define _APDU_FEATURE_H_

#define MAX_OPEN_SESSIONS 1
/*
uint8  FeatureList[]= {
0x00, //Resereved
0x01,//RF Output Channel
0x02,//Parental Control PIN
#if 1
0x03,//Parental Control Settings
0x04,// IPPV PIN
0x05,// Time Zone
0x06,//Day light Savings Control
0x07,// AC Outlet
0x08,// Language
0x09,// Rating Region
0x0A,// Reset PINS
0x0B,// Cable URL
0x0C,//EAS location code
//0x0D-0x6F,Reserved for future use
//0x70-FF, Reserved for proprietary use
#endif
};
*/
void ResetCardFeatureList();
int GetCardFeatureList(unsigned char *pList, unsigned char *pListSize);
extern "C" unsigned char GetFeatureControlVer(void);
int  APDU_HostToCardFeatureParams (USHORT sesnum ,unsigned char FetureId);
typedef enum FeatureStatus
{
    ACCEPTED = 0,
    NOT_SUPPORTED = 1,
    INVALID_PARAM = 2,
}FeatureStatus_e;
typedef enum Features
{
  RESERVED        = 0,
  RF_OUTPUT_CHANNEL,
  P_C_PIN,
  P_C_SETTINGS,
  IPPV_PIN = 4,
  TIME_ZONE = 5,
  DAYLIGHT_SAVINGS =6,
  AC_OUTLET,
  LANGUAGE,
  RATING_REGION,
  RESET_PIN,
  CABLE_URLS =0xB,
  EA_LOCATION_CODE = 0xC,
  VCT_ID = 0x0D,
  TURN_ON_CHANNEL = 0x0E,
  TERMINAL_ASSOCIATION = 0x0F,
  DOWNLOAD_GP_ID = 0x10,
  ZIP_CODE= 0x11,
  
 }Features_e;
 
#define GEN_FET_MAX_PIN_LEN  32
#define GEN_FET_PIN_LEN      16
#define GEN_FET_MAX_CHNLD    05
#define GEN_FET_MAX_PUR_PIN_LEN  32
#define GEN_FET_PUR_PIN_LEN  16
#define GEN_FET_MAX_URLS_CHARS 64 // fill correct data when required
#define GEN_FET_MAX_URLS       05
#define GEN_FET_URLS           05    
static unsigned long MajorCh[GEN_FET_MAX_CHNLD]= {0,1,2,3,4};
static unsigned long MinorCh[GEN_FET_MAX_CHNLD]= {0,1,2,3,4};
typedef struct rf_out_channel
{
    unsigned char output_channel;
    unsigned char output_channel_ui;
}rf_out_channel_t;

typedef struct p_c_pin
{
    unsigned char p_c_pin_length;
    unsigned char p_c_pin_chr[GEN_FET_MAX_PIN_LEN];
}p_c_pin_t;
typedef struct p_c_chan_num
{
    unsigned char channel[3];
}p_c_chan_num_t;
typedef struct p_c_settings_t
{
    unsigned char p_c_factory_reset;
    unsigned char p_c_channel_count_lo;
    unsigned char p_c_channel_count_hi;
    p_c_chan_num_t  VirtCh[GEN_FET_MAX_CHNLD];
}p_c_settings_t;

typedef struct purchase_pin
{
    uint8 purchase_pin_length;
    uint8 purchase_pin_chr[GEN_FET_MAX_PUR_PIN_LEN];
}purchase_pin_t;
typedef struct  time_zone
{
    unsigned short time_zone_offset;
}time_zone_t;
#define daylight_savings_size  10
typedef struct daylight_savings
{
    uint8 daylight_savings_control;
    uint8 daylight_savings_delta;
    uint32 daylight_savings_entry_time;
    uint32 daylight_savings_exit_time;
}daylight_savings_t;

typedef struct ac_outlet
{
    uint8 ac_outlet_control;
}ac_outlet_t;
typedef struct language
{
    uint8 language_control[3];
}language_t;
typedef struct rating_region
{
    uint8 rating_region_setting;
}rating_region_t;
typedef struct reset_pin
{
    uint8 reset_pin_control;
}reset_pin_t;

typedef struct url
{
    uint8 url_type;
    uint8 url_length;
    uint8 url_char[GEN_FET_MAX_URLS_CHARS]; 
}url_t;
typedef struct cable_urls
{
    uint8 number_of_urls;
    url_t url[GEN_FET_MAX_URLS];
}cable_urls_t;

#define EA_location_code_Size   3
typedef struct EA_location_code
{
    unsigned char state_code;
    unsigned char county_subdivision;
    unsigned char reserved;
    unsigned short county_code;
    
}EA_location_code_t;
typedef struct vct_id
{
    
    unsigned short vct_id;
    
}vct_id_t;
#define turn_on_channel_size 2
typedef struct turn_on_channel
{
    
    unsigned char reserved;
    unsigned char turn_on_channel_defined;
    unsigned short turn_on_vertual_channel;
    
}turn_on_channel_t;

typedef struct terminal_association
{
    unsigned short identifier_length;
    unsigned char terminal_assoc_identifier[256];
}terminal_association_t;

typedef struct cdl_group_id
{
    unsigned short group_id;
    
}cdl_group_id_t;


typedef struct zip_code
{
    unsigned short zip_code_len;
    unsigned char zip_code[32];
}zip_code_t;
#endif //_FEATURE_MAIN_H_


