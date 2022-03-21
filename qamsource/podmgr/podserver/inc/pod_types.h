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

 

#ifndef __VL_POD_TYPES_H__
#define __VL_POD_TYPES_H__
/*-----------------------------------------------------------------*/

#define VL_HOST_FEATURE_MAX_STR_SIZE        260
#define VL_HOST_FEATURE_MAX_PIN_SIZE        260
#define VL_HOST_FEATURE_MAX_PC_CHANNELS     2048
#define VL_HOST_FEATURE_MAX_FEATURE_DATA    65536
#define VL_HOST_FEATURE_MAX_CABLE_URLS      256
#define VL_HOST_FEATURE_LANG_CNTRL_CHARS    3

typedef enum _VL_HOST_FEATURE_PARAM
{
    VL_HOST_FEATURE_PARAM_RF_Output_Channel            = 0x01,
    VL_HOST_FEATURE_PARAM_Parental_Control_PIN         = 0x02,
    VL_HOST_FEATURE_PARAM_Parental_Control_Settings    = 0x03,
    VL_HOST_FEATURE_PARAM_IPPV_PIN                     = 0x04,
    VL_HOST_FEATURE_PARAM_Time_Zone                    = 0x05,
    VL_HOST_FEATURE_PARAM_Daylight_Savings_Control     = 0x06,
    VL_HOST_FEATURE_PARAM_AC_Outlet                    = 0x07,
    VL_HOST_FEATURE_PARAM_Language                     = 0x08,
    VL_HOST_FEATURE_PARAM_Rating_Region                = 0x09,
    VL_HOST_FEATURE_PARAM_Reset_PINS                   = 0x0A,
    VL_HOST_FEATURE_PARAM_Cable_URLS                   = 0x0B,
    VL_HOST_FEATURE_PARAM_EAS_location_code            = 0x0C,
    VL_HOST_FEATURE_PARAM_VCT_ID                       = 0x0D,
    VL_HOST_FEATURE_PARAM_Turn_on_Channel              = 0x0E,
    VL_HOST_FEATURE_PARAM_Terminal_Association         = 0x0F,
    VL_HOST_FEATURE_PARAM_Common_Download_Group_Id     = 0x10,
    VL_HOST_FEATURE_PARAM_Zip_Code                     = 0x11,

} VL_HOST_FEATURE_PARAM;

typedef enum _VL_POD_HOST_DAY_LIGHT_SAVINGS
{
    VL_POD_HOST_DAY_LIGHT_SAVINGS_IGNORE      = 0x00,
    VL_POD_HOST_DAY_LIGHT_SAVINGS_DO_NOT_USE  = 0x01,
    VL_POD_HOST_DAY_LIGHT_SAVINGS_USE_DLS     = 0x02,

} VL_POD_HOST_DAY_LIGHT_SAVINGS;

typedef enum _VL_POD_HOST_AC_OUTLET
{
    VL_POD_HOST_AC_OUTLET_USER_SETTING    = 0x00,
    VL_POD_HOST_AC_OUTLET_SWITCHED        = 0x01,
    VL_POD_HOST_AC_OUTLET_UNSWITCHED      = 0x02,

} VL_POD_HOST_AC_OUTLET;

typedef enum _VL_POD_HOST_RATING_REGION
{
    VL_POD_HOST_RATING_REGION_FORBIDDEN   = 0x00,
    VL_POD_HOST_RATING_REGION_US          = 0x01,
    VL_POD_HOST_RATING_REGION_CANADA      = 0x02,

} VL_POD_HOST_RATING_REGION;

typedef enum _VL_POD_HOST_RESET_PIN
{
    VL_POD_HOST_RESET_PIN_NONE                = 0x00,
    VL_POD_HOST_RESET_PIN_PARENTAL_CONTROL    = 0x01,
    VL_POD_HOST_RESET_PIN_PURCHASE            = 0x02,
    VL_POD_HOST_RESET_PIN_PURCHASE_AND_PC     = 0x03,

} VL_POD_HOST_RESET_PIN;

typedef struct _VL_POD_HOST_FEATURE_ID_LIST
{
    unsigned char nNumberOfFeatures;
    unsigned char aFeatureId[32];
}VL_POD_HOST_FEATURE_ID_LIST;

typedef struct _VL_POD_HOST_FEATURE_RF_CHANNEL
{
    unsigned char    nRfOutputChannel;
    unsigned char    bEnableRfOutputChannelUI;
}VL_POD_HOST_FEATURE_RF_CHANNEL;

typedef struct _VL_POD_HOST_FEATURE_PARENTAL_CONTROL_PIN
{
    unsigned char    nParentalControlPinSize;
    unsigned char    aParentalControlPin[VL_HOST_FEATURE_MAX_PIN_SIZE];
}VL_POD_HOST_FEATURE_PARENTAL_CONTROL_PIN;

typedef struct _VL_POD_HOST_FEATURE_PARENTAL_CONTROL_SETTINGS
{
    unsigned char   ParentalControlReset;
    unsigned short  nParentalControlChannels;
    unsigned long * paParentalControlChannels;
}VL_POD_HOST_FEATURE_PARENTAL_CONTROL_SETTINGS;

typedef struct _VL_POD_HOST_FEATURE_PURCHASE_PIN
{
    unsigned char    nPurchasePinSize;
    unsigned char    aPurchasePin[VL_HOST_FEATURE_MAX_PIN_SIZE];
}VL_POD_HOST_FEATURE_PURCHASE_PIN;

typedef struct _VL_POD_HOST_FEATURE_TIME_ZONE_OFFSET
{
    short   nTimeZoneOffset;
}VL_POD_HOST_FEATURE_TIME_ZONE_OFFSET;

typedef struct _VL_POD_HOST_FEATURE_DAYLIGHT_SAVINGS
{
    VL_POD_HOST_DAY_LIGHT_SAVINGS eDaylightSavingsType;
    unsigned char    nDaylightSavingsDeltaMinutes;
    unsigned long    tmDaylightSavingsEntryTime;
    unsigned long    tmDaylightSavingsExitTime;
}VL_POD_HOST_FEATURE_DAYLIGHT_SAVINGS;

typedef struct _VL_POD_HOST_FEATURE_AC_OUTLET
{
    VL_POD_HOST_AC_OUTLET eAcOutletSetting;
}VL_POD_HOST_FEATURE_AC_OUTLET;

typedef struct _VL_POD_HOST_FEATURE_LANGUAGE
{
    int            nLanguageControlChars;
    unsigned char  szLanguageControl[VL_HOST_FEATURE_LANG_CNTRL_CHARS+1];
}VL_POD_HOST_FEATURE_LANGUAGE;

typedef struct _VL_POD_HOST_FEATURE_RATING_REGION
{
    VL_POD_HOST_RATING_REGION eRatingRegion;
}VL_POD_HOST_FEATURE_RATING_REGION;

typedef struct _VL_POD_HOST_FEATURE_RESET_PINS
{
    VL_POD_HOST_RESET_PIN eResetPins;
}VL_POD_HOST_FEATURE_RESET_PINS;

typedef enum _VL_POD_HOST_URL_TYPE
{
    VL_POD_HOST_URL_TYPE_UNDEFINED  = 0x00,
    VL_POD_HOST_URL_TYPE_WEB_PORTAL = 0x01,
    VL_POD_HOST_URL_TYPE_EPG        = 0x02,
    VL_POD_HOST_URL_TYPE_VOD        = 0x03,

    VL_POD_HOST_URL_TYPE_INVALID    = 0xFFFF,
        
} VL_POD_HOST_URL_TYPE;

typedef struct _VL_POD_HOST_FEATURE_URL
{
    VL_POD_HOST_URL_TYPE eUrlType;
    unsigned long nUrlLength;
    unsigned char szUrl[VL_HOST_FEATURE_MAX_STR_SIZE];
    
}VL_POD_HOST_FEATURE_URL;

typedef struct _VL_POD_HOST_FEATURE_CABLE_URLS
{
    int nUrls;
    VL_POD_HOST_FEATURE_URL * paUrls;
}VL_POD_HOST_FEATURE_CABLE_URLS;

typedef struct _VL_POD_HOST_FEATURE_EAS
{
    unsigned int    EMStateCodel;
    unsigned int    EMCSubdivisionCodel;
    unsigned int    EMCountyCode;
} VL_POD_HOST_FEATURE_EAS;

typedef struct _VL_POD_HOST_FEATURE_VCT_ID
{
    unsigned short   nVctId;
}VL_POD_HOST_FEATURE_VCT_ID;

typedef struct _VL_POD_HOST_FEATURE_TURN_ON_CHANNEL
{
    unsigned char    bTurnOnChannelDefined;
    unsigned short   iTurnOnChannel;
}VL_POD_HOST_FEATURE_TURN_ON_CHANNEL;

typedef struct _VL_POD_HOST_FEATURE_TERMINAL_ASSOCIATION
{
    unsigned short   nTerminalAssociationLength;
    char             szTerminalAssociationName[VL_HOST_FEATURE_MAX_STR_SIZE];
}VL_POD_HOST_FEATURE_TERMINAL_ASSOCIATION;

typedef struct _VL_POD_HOST_FEATURE_CDL_GROUP_ID
{
    unsigned short   common_download_group_id;
}VL_POD_HOST_FEATURE_CDL_GROUP_ID;

typedef struct _VL_POD_HOST_FEATURE_ZIP_CODE
{
    unsigned short nZipCodeLength;
    char           szZipCode[VL_HOST_FEATURE_MAX_STR_SIZE];
}VL_POD_HOST_FEATURE_ZIP_CODE;

#define VL_POD_ALLOC(datatype)                          (datatype*)malloc(sizeof(datatype))
#define VL_POD_ALLOC_ARRAY(datatype, count)             (datatype*)malloc((count)*sizeof(datatype))
#define VL_POD_GET_STRUCT(datatype, pStruct, voidptr)    datatype * pStruct = ((datatype*)(voidptr))

/*-----------------------------------------------------------------*/
#endif // __VL_POD_TYPES_H__
/*-----------------------------------------------------------------*/
