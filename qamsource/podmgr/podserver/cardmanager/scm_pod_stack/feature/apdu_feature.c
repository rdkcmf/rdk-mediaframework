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
#include <string.h>
#include "utils.h"
#include "link.h"
#include "transport.h"
#include "session.h"
#include "core_events.h"
#include "podhighapi.h"
#include "appmgr.h"
#include "applitest.h"
#include <lcsm_log.h>
#include <global_event_map.h>
#include "apdu_feature.h"
#include "hostGenericFeatures.h"

#include "rmf_osal_event.h"
#include "cardUtils.h"

#include <string.h>
#include <rdk_debug.h>
#include "rmf_osal_mem.h"
#include <coreUtilityApi.h>
#ifdef GLI
#include "libIBus.h"
#include "sysMgr.h"
#endif	
#if USE_SYSRES_MLT
#include "mlt_malloc.h"
#endif

#define __MTAG__ VL_CARD_MANAGER


static unsigned short vlg_VctId = 0;
static unsigned short vlg_CDL_GroupId = 0;
static unsigned long vlg_Error_Set_Param_Mfr = 0;
#define MAX_MEM_SIZE_FET_LIST_PARAM  0x1000*3
#define GEN_FET_LIST_MAX_SIZE 0xFF
//#define USE_TEST_DATA
#if 1 //ndef USE_TEST_DATA // Testing purpose only...  use mfr data after the mfr is complete
static unsigned char FeatureList[GEN_FET_LIST_MAX_SIZE];
static unsigned long FeatureListSize = 0;
#else// Testing purpose only...  use mfr data after the mfr is complete
uint8  FeatureList[]= {
//0x00, //Resereved
0x01,//RF Output Channel
0x02,//Parental Control PIN
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
0x0D,//VCT Id
0x0E,//Turn-on Channel
0x0F,//Terminal Association
0x10,// download group id
0x11,// zipe code
//0x0D-0x6F,Reserved for future use
//0x70-FF, Reserved for proprietary use

};
static unsigned long FeatureListSize = 0x11;
#endif
#define MAX_FEATURE_LIST_SIZE  0x20

static unsigned char CardFeatureList[MAX_FEATURE_LIST_SIZE];
static unsigned char CardFeatureListSize =0;

extern int featureSndEvent(SYSTEM_EVENTS event, unsigned char *payload, unsigned short payload_len,
                unsigned int x, unsigned int y, unsigned int z);
unsigned short GetFeatureControlCDLGroupId(void)
{
    return vlg_CDL_GroupId;
}

void SetFeatureControlCDLGroupId(unsigned short GrpId)
{
     vlg_CDL_GroupId = GrpId;
}
unsigned short GetFeatureParamVctId()
{
    return vlg_VctId;
}
void vlFeatParamFreeMem(void *pData)
{
    if(pData)
		rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pData);
}
int featureGetList()
{
    VL_POD_HOST_FEATURE_ID_LIST  feature_id_list;
    memset(&feature_id_list,0,sizeof(feature_id_list));
    VL_HOST_GENERIC_FEATURES_RESULT iRet;
    iRet = vlPodGetGenericFeatureListWithVer(GetFeatureControlVer(),&feature_id_list);
//    iRet = vlPodGetGenericFeatureList(&feature_id_list);
    if(VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS != iRet)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","featureGetList:vlPodGetGenericFeatureList returned Error !! :%d \n",iRet);
        return -1;
    }
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","featureGetList: Number of Features:%d \n",feature_id_list.nNumberOfFeatures );

#if 1 //ndef USE_TEST_DATA
    
//    memcpy(FeatureList, &feature_id_list.aFeatureId[0], feature_id_list.nNumberOfFeatures);
    rmf_osal_memcpy(FeatureList, &feature_id_list.aFeatureId[0], feature_id_list.nNumberOfFeatures,
    	sizeof(FeatureList), feature_id_list.nNumberOfFeatures );
    //FeatureList[feature_id_list.nNumberOfFeatures] = 0x0D;// Adding vct Id.
    FeatureListSize = feature_id_list.nNumberOfFeatures;
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD",">>>>>>>>>>>> FeatureListSize:%d \n",FeatureListSize);

#endif
    return 0;
}        
unsigned char FatureSupport(unsigned char FeatureId)
{
    FeatureStatus_e status = NOT_SUPPORTED;
    int ii;

    if(vlg_Error_Set_Param_Mfr)
        return status;
    if(FeatureId > ZIP_CODE)
        return INVALID_PARAM;

    for( ii = 0 ; ii < FeatureListSize; ii++)
    {
        if(FeatureList[ii] == FeatureId)
        {
            status = ACCEPTED;
            break;
        }
    }
    return status;
}
int rf_output_channel(uint8 *pData, uint32 dataLen)
{
    //rf_out_channel_t *pRfOut;
    VL_POD_HOST_FEATURE_RF_CHANNEL rf_channel ;
    memset(&rf_channel,0,sizeof(rf_channel));
    VL_HOST_GENERIC_FEATURES_RESULT iRet;
    int size =0;
    
    if(pData == NULL || dataLen < 2 )
    {
        return -1;
    }

    iRet = vlPodGetGenericFeatureData( VL_HOST_FEATURE_PARAM_RF_Output_Channel, &rf_channel);
    if(VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS != iRet)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","rf_output_channel:vlPodGetGenericFeatureData returned Error !! :%d \n",iRet);
        return -1;
    }
    

    //pRfOut = (rf_out_channel_t*)pData;
    //pRfOut->output_channel = rf_channel.nRfOutputChannel;
    pData[0] =  rf_channel.nRfOutputChannel;  
    //pRfOut->output_channel_ui = rf_channel.bEnableRfOutputChannelUI; //Disable RF output channel user interface
    pData[1] = rf_channel.bEnableRfOutputChannelUI;
    //size = sizeof(rf_out_channel_t);
    size += 2;
    return size;

}

int p_c_pin(uint8 *pData, uint32 dataLen)
{
    int ii,size=0;
    //p_c_pin_t *pPcPin;
    VL_HOST_GENERIC_FEATURES_RESULT iRet;
    VL_POD_HOST_FEATURE_PARENTAL_CONTROL_PIN   parental_control_pin ;
	memset(&parental_control_pin,0,sizeof(parental_control_pin));
    
    if(pData == NULL || dataLen < 1 )
    {
        return -1;
    }
    iRet = vlPodGetGenericFeatureData( VL_HOST_FEATURE_PARAM_Parental_Control_PIN,      &parental_control_pin);
    if(VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS != iRet)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","rf_output_channel:vlPodGetGenericFeatureData returned Error !! :%d \n",iRet);
        return -1;
    }
    //pPcPin = (p_c_pin_t*)pData;

    //pPcPin->p_c_pin_length = (unsigned char )parental_control_pin.nParentalControlPinSize; 
    pData[0]  = (unsigned char )parental_control_pin.nParentalControlPinSize; 
    size++;
    for(ii = 0; ii < parental_control_pin.nParentalControlPinSize; ii++)
    {
        //pPcPin->p_c_pin_chr[ii]= parental_control_pin.aParentalControlPin[ii]; //Fill out the actual pin
        pData[ii +1] = parental_control_pin.aParentalControlPin[ii];
        size++;
    }
    
    return size;
}

int p_c_settings(uint8 *pData, uint32 dataLen)
{
    int ii,size=0;
    //p_c_settings_t *pPcSettings;
    VL_HOST_GENERIC_FEATURES_RESULT iRet;
    VL_POD_HOST_FEATURE_PARENTAL_CONTROL_SETTINGS  parental_control_settings ;
    memset(&parental_control_settings,0,sizeof(parental_control_settings));
    if(pData == NULL || dataLen < (GEN_FET_PIN_LEN + 1) )
    {
        return -1;
    }

    iRet = vlPodGetGenericFeatureData( VL_HOST_FEATURE_PARAM_Parental_Control_Settings, &parental_control_settings);
    if(VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS != iRet)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","rf_output_channel:vlPodGetGenericFeatureData returned Error !! :%d \n",iRet);
        return -1;
    }
 
    //pPcSettings = (p_c_settings_t*)pData;
    
    *pData++  = parental_control_settings.ParentalControlReset; //Fill the correct values as required
    *pData++  = (unsigned char)((parental_control_settings.nParentalControlChannels &0xFF00) >> 8);
    *pData++  = (unsigned char) (parental_control_settings.nParentalControlChannels &0x00FF); 
    
    size = 3;
    
    for( ii = 0; ii < parental_control_settings.nParentalControlChannels; ii++)
    {
        parental_control_settings.paParentalControlChannels[ii] = (parental_control_settings.paParentalControlChannels[ii] | 0xF00000);
        //pPcSettings->VirtCh[ii].channel[0] = (unsigned char)( (parental_control_settings.paParentalControlChannels[ii] & 0xFF0000) >> 16);
        *pData++  = (unsigned char)( (parental_control_settings.paParentalControlChannels[ii] & 0xFF0000) >> 16);
        //pPcSettings->VirtCh[ii].channel[1] = (unsigned char)( (parental_control_settings.paParentalControlChannels[ii] & 0x00FF00) >> 8);
        *pData++ = (unsigned char)( (parental_control_settings.paParentalControlChannels[ii] & 0x00FF00) >> 8);
        //pPcSettings->VirtCh[ii].channel[2] = (unsigned char)  (parental_control_settings.paParentalControlChannels[ii] & 0x0000FF);
        *pData++ = (unsigned char)  (parental_control_settings.paParentalControlChannels[ii] & 0x0000FF);
        size += 3;
    }
    if(parental_control_settings.paParentalControlChannels)
		rmf_osal_memFreeP(RMF_OSAL_MEM_POD, parental_control_settings.paParentalControlChannels);
#if 0
    for(ii = 0; ii < 5; ii++)
    {
        unsigned long  channel;
        unsigned char *pCheck;
        
        pCheck = (unsigned char *)&channel;
        
        channel = 0;
        channel = (0xF << 28) | ((MajorCh[ii]&0x3F) << 18) | ((MinorCh[ii]&0x3F) << 8) ;
        
        if((*pCheck & 0xF0) == 0xF0 && (*(pCheck+3) == 0x00))
        {
        //big endian 
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," Big Endian \n");
        }
        else
        {
        // little endian
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Little Endian\n");
            channel = ((channel & 0xFF) << 24) | ((channel & 0xFF000000)>>24) | ((channel & 0x00FF0000) >> 8)|((channel & 0x000FF000) << 8);
        }
        
        size = size + 3;
        memcpy( &pPcSettings->VirtCh[ii].channel[0], pCheck, 3);
    }
#endif    
    return size;


}


int purchase_pin(uint8 *pData, uint32 dataLen)
{
    int ii,size=0;
    //purchase_pin_t *pPurPin;
    VL_POD_HOST_FEATURE_PURCHASE_PIN  purchase_pin;
	memset(&purchase_pin,0,sizeof(purchase_pin));
    VL_HOST_GENERIC_FEATURES_RESULT iRet;
 

    if(pData == NULL || dataLen < 1 )
    {
        return -1;
    }

    iRet = vlPodGetGenericFeatureData( VL_HOST_FEATURE_PARAM_IPPV_PIN, &purchase_pin);
    if(VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS != iRet)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","purchase_pin:vlPodGetGenericFeatureData returned Error !! :%d \n",iRet);
        return -1;
    }
    //pPurPin = (purchase_pin_t *)pData;
    
    //pPurPin->purchase_pin_length = purchase_pin.nPurchasePinSize;//Fill correct data... later
    pData[0] = (unsigned char)purchase_pin.nPurchasePinSize;//Fill correct data... later
    size++;
    for(ii = 0; ii <purchase_pin.nPurchasePinSize; ii++)
    {
        //pPurPin->purchase_pin_chr[ii] = purchase_pin.aPurchasePin[ii];// fill correct data later
        pData[1+ii] = (unsigned char) purchase_pin.aPurchasePin[ii];// fill correct data later
        size++;
    }
    return size;
}


extern "C" int time_zone(uint8 *pData, uint32 dataLen)
{
    VL_POD_HOST_FEATURE_TIME_ZONE_OFFSET time_zone_offset ;
	memset(&time_zone_offset,0,sizeof(time_zone_offset));
    VL_HOST_GENERIC_FEATURES_RESULT iRet;
    int size = 0;
    
    if(pData == NULL || dataLen < 2 )
    {
        return -1;
    }
    iRet = vlPodGetGenericFeatureData( VL_HOST_FEATURE_PARAM_Time_Zone,&time_zone_offset);
    if(VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS != iRet)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","purchase_pin:vlPodGetGenericFeatureData returned Error !! :%d \n",iRet);
        return -1;
    }
    
    
    pData[0] = (unsigned char)((time_zone_offset.nTimeZoneOffset & 0xFF00) >> 8);//fill correct data
    pData[1] = (unsigned char)(time_zone_offset.nTimeZoneOffset & 0xFF);//fill correct data
    
    size = sizeof(time_zone_t);
    
    return size;
}

extern "C" int daylight_savings(uint8 *pData, uint32 dataLen)
{
    
    //daylight_savings_t *pDltSgs;
    int size = 0;
    VL_POD_HOST_FEATURE_DAYLIGHT_SAVINGS  daylight_savings ;
	memset(&daylight_savings,0,sizeof(daylight_savings));
    VL_HOST_GENERIC_FEATURES_RESULT iRet;
    
    if(pData == NULL || dataLen < 1 )
    {
        return -1;
    }
    iRet = vlPodGetGenericFeatureData( VL_HOST_FEATURE_PARAM_Daylight_Savings_Control,&daylight_savings);
    if(VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS != iRet)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","purchase_pin:vlPodGetGenericFeatureData returned Error !! :%d \n",iRet);
        return -1;
    }


    //pDltSgs = (daylight_savings_t *)pData;
        
    pData[0] = (unsigned char)daylight_savings.eDaylightSavingsType;// fill correct data
    size = 1;
    if(daylight_savings.eDaylightSavingsType == VL_POD_HOST_DAY_LIGHT_SAVINGS_USE_DLS)
    {
        pData[1] = daylight_savings.nDaylightSavingsDeltaMinutes;

        pData[2] = (unsigned char)((daylight_savings.tmDaylightSavingsEntryTime & 0xFF000000) >> 24);
        pData[3] = (unsigned char)((daylight_savings.tmDaylightSavingsEntryTime & 0xFF0000) >> 16);
        pData[4] = (unsigned char)((daylight_savings.tmDaylightSavingsEntryTime & 0xFF00) >> 8);
        pData[5] = (unsigned char)(daylight_savings.tmDaylightSavingsEntryTime & 0xFF);

        pData[6] = (unsigned char)((daylight_savings.tmDaylightSavingsExitTime & 0xFF000000) >> 24);
        pData[7] = (unsigned char)((daylight_savings.tmDaylightSavingsExitTime & 0xFF0000) >> 16);
        pData[8] = (unsigned char)((daylight_savings.tmDaylightSavingsExitTime & 0xFF00) >> 8);
        pData[9] = (unsigned char)(daylight_savings.tmDaylightSavingsExitTime & 0xFF);

        size = daylight_savings_size;//sizeof(daylight_savings_t);
    }
    
    
    return size;
}


int ac_outlet(uint8 *pData, uint32 dataLen)
{
    
//    ac_outlet_t *pAcOutlet;
    int size = 0;
    VL_POD_HOST_FEATURE_AC_OUTLET   ac_outlet ;
	memset(&ac_outlet,0,sizeof(ac_outlet));
    VL_HOST_GENERIC_FEATURES_RESULT iRet;

    if(pData == NULL || dataLen < 1 )
    {
        return -1;
    }
    //pAcOutlet = (ac_outlet_t *)pData;
    iRet = vlPodGetGenericFeatureData( VL_HOST_FEATURE_PARAM_AC_Outlet,&ac_outlet);
    if(VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS != iRet)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","purchase_pin:vlPodGetGenericFeatureData returned Error !! :%d \n",iRet);
        return -1;
    }    
    //pAcOutlet->ac_outlet_control = (unsigned char)ac_outlet.eAcOutletSetting;// fill correct data
    pData[0] = (unsigned char)ac_outlet.eAcOutletSetting;// fill correct data
    size += 1;//sizeof(ac_outlet_t);
    
    return size;
}

int language(uint8 *pData, uint32 dataLen)
{
    
    //language_t *pLang;
    int size = 0;
    VL_HOST_GENERIC_FEATURES_RESULT iRet;
    VL_POD_HOST_FEATURE_LANGUAGE language;
	memset(&language,0,sizeof(language));

    if(pData == NULL || dataLen < 3/*sizeof(language_t)*/ )
    {
        return -1;
    }
    iRet = vlPodGetGenericFeatureData( VL_HOST_FEATURE_PARAM_Language, &language);
    if(VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS != iRet)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","purchase_pin:vlPodGetGenericFeatureData returned Error !! :%d \n",iRet);
        return -1;
    }    

    //pLang = (language_t *)pData;
        
    pData[0] = language.szLanguageControl[0];// fill correct data
    pData[1] = language.szLanguageControl[1];// fill correct data
    pData[2] = language.szLanguageControl[2];// fill correct data
    size += 3;//sizeof(language_t);
    
    return size;
}



int rating_region(uint8 *pData, uint32 dataLen)
{
    
    //rating_region_t *pRatingRegion;
    int size = 0;
    VL_POD_HOST_FEATURE_RATING_REGION   rating_region ;
	memset(&rating_region,0,sizeof(rating_region));
    VL_HOST_GENERIC_FEATURES_RESULT iRet;

    if(pData == NULL || dataLen < 1 )
    {
        return -1;
    }
    iRet = vlPodGetGenericFeatureData( VL_HOST_FEATURE_PARAM_Rating_Region, &rating_region);
    if(VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS != iRet)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","purchase_pin:vlPodGetGenericFeatureData returned Error !! :%d \n",iRet);
        return -1;
    }    

    //pRatingRegion = (rating_region_t *)pData;
        
    pData[0] = (unsigned char)rating_region.eRatingRegion;// fill correct data
    size += 1;//sizeof(rating_region_t);
    
    return size;
}



int reset_pin(uint8 *pData, uint32 dataLen)
{
    
//    reset_pin_t *pResetPin;
    int size = 0;
    VL_POD_HOST_FEATURE_RESET_PINS   reset_pins ;
	memset(&reset_pins,0,sizeof(reset_pins));
    VL_HOST_GENERIC_FEATURES_RESULT iRet;

    if(pData == NULL || dataLen < 1 )
    {
        return -1;
    }
    iRet = vlPodGetGenericFeatureData( VL_HOST_FEATURE_PARAM_Reset_PINS,&reset_pins);
    if(VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS != iRet)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","purchase_pin:vlPodGetGenericFeatureData returned Error !! :%d \n",iRet);
        return -1;
    }
//    pResetPin = (reset_pin_t *)pData;
        
    //pResetPin->reset_pin_control = (unsigned char)reset_pins.eResetPins;// fill correct data
    pData[0] = (unsigned char)reset_pins.eResetPins;// fill correct data
    
    size += 1;// sizeof(reset_pin_t);
    
    return size;
}

int cable_urls(uint8 *pData, uint32 dataLen)
{
    
    //cable_urls_t *pCableUrls;
    int ii,size = 0;
    VL_HOST_GENERIC_FEATURES_RESULT iRet=VL_HOST_GENERIC_FEATURES_RESULT_ERROR;
    VL_POD_HOST_FEATURE_CABLE_URLS url_info;
	memset(&url_info,0,sizeof(url_info));

    if(pData == NULL || dataLen < sizeof(cable_urls_t))
    {
        return -1;
    }
    iRet = vlPodGetGenericFeatureData( VL_HOST_FEATURE_PARAM_Cable_URLS,&url_info);
    if(VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS != iRet)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","purchase_pin:vlPodGetGenericFeatureData returned Error !! :%d \n",iRet);
        return -1;
    }
//    pCableUrls = (cable_urls_t *)pData;
        
    *pData++ = url_info.nUrls;// fill correct data
    size++;
    
    for(ii = 0; ii < url_info.nUrls; ii++)
    {
        int jj;
        //pCableUrls->url[ii].url_type = url_info.paUrls->eUrlType; // fill correct data
        *pData++ = url_info.paUrls[ii].eUrlType; // fill correct data
        //pCableUrls->url[ii].url_length = url_info.paUrls->nUrlLength; // fill correct data
        *pData++ = url_info.paUrls[ii].nUrlLength; // fill correct data
        size = size + 2;
        for(jj=0; jj < url_info.paUrls[ii].nUrlLength; jj++)
        {
            //pCableUrls->url[ii].url_char[jj] = url_info.paUrls->szUrl[jj]; // fill correct data
            *pData++ = url_info.paUrls[ii].szUrl[jj]; // fill correct data
            size++;
        }
        
    }
    if(url_info.paUrls)
		rmf_osal_memFreeP(RMF_OSAL_MEM_POD, url_info.paUrls);
    
    return size;
}


extern "C" int EA_location_code(uint8 *pData, uint32 dataLen)
{
    
    EA_location_code_t *pEALocationCode,EALocationCode;
    int ii,size=0;
    VL_HOST_GENERIC_FEATURES_RESULT iRet;
    VL_POD_HOST_FEATURE_EAS eas_state_info ;
	memset(&eas_state_info,0,sizeof(eas_state_info));
    
    
    if(pData == NULL || dataLen < EA_location_code_Size )
    {
        return -1;
    }
    iRet = vlPodGetGenericFeatureData( VL_HOST_FEATURE_PARAM_EAS_location_code,&eas_state_info);
    if(VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS != iRet)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","purchase_pin:vlPodGetGenericFeatureData returned Error !! :%d \n",iRet);
        return -1;
    }

    pEALocationCode = (EA_location_code_t *)&EALocationCode;
//    memset(pEALocationCode, 0, sizeof(EA_location_code_t));
        memset(pEALocationCode, 0, sizeof(EA_location_code_t));
    pEALocationCode->state_code = (unsigned char)eas_state_info.EMStateCodel;// fill correct data
    pEALocationCode->county_subdivision = (unsigned char)eas_state_info.EMCSubdivisionCodel;// fill correct data
    pEALocationCode->reserved = 0x3;// fill correct data
    pEALocationCode->county_code = (unsigned short)eas_state_info.EMCountyCode;// fill correct data
RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","eas_state_info.EMStateCodel:%d eas_state_info.EMCSubdivisionCodel:%d eas_state_info.EMCountyCode:%d\n",eas_state_info.EMStateCodel,eas_state_info.EMCSubdivisionCodel,eas_state_info.EMCountyCode);
    *pData++ = (unsigned char)pEALocationCode->state_code;
    *pData++ = (unsigned char)((pEALocationCode->county_subdivision << 4) | (pEALocationCode->reserved << 2) | ((pEALocationCode->county_code & 0x300) >> 8));
    *pData = (unsigned char)(pEALocationCode->county_code & 0xFF);
    
    size = EA_location_code_Size;
    return size;
}

extern "C" int vct_id(uint8 *pData, uint32 dataLen)
{
    
    //vct_id_t *pVct_id;
    int size = 0;
    VL_HOST_GENERIC_FEATURES_RESULT iRet;

    if(pData == NULL || dataLen < 2 )
    {
        return -1;
    }
    //iRet = vlPodGetGenericFeatureData( VL_HOST_FEATURE_PARAM_Reset_PINS,&reset_pins);
    /*if(VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS != iRet)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","purchase_pin:vlPodGetGenericFeatureData returned Error !! :%d \n",iRet);
        return -1;
    }*/
    //pVct_id = (vct_id_t *)pData;
        
    *pData++ = (unsigned char)( (vlg_VctId & 0xFF00) >> 8 );
    *pData = (unsigned char)(vlg_VctId & 0xFF) ;
    
    size += 2;//sizeof(vct_id_t);
    
    return size;
}

int turn_on_channel(uint8 *pData, uint32 dataLen)
{
    
    turn_on_channel_t *pTurnOnChannel,TurnOnChannel;
    int size = 0;
    VL_POD_HOST_FEATURE_TURN_ON_CHANNEL   turn_on_channel ;
	memset(&turn_on_channel,0,sizeof(turn_on_channel));
    VL_HOST_GENERIC_FEATURES_RESULT iRet;

    if(pData == NULL || dataLen < turn_on_channel_size )
    {
        return -1;
    }
    iRet = vlPodGetGenericFeatureData( VL_HOST_FEATURE_PARAM_Turn_on_Channel,&turn_on_channel);
    if(VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS != iRet)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","purchase_pin:vlPodGetGenericFeatureData returned Error !! :%d \n",iRet);
        return -1;
    }
    pTurnOnChannel = (turn_on_channel_t *)&TurnOnChannel;
//    memset(pTurnOnChannel, 0, sizeof(turn_on_channel_t));
        memset(pTurnOnChannel, 0, sizeof(turn_on_channel_t));
    pTurnOnChannel->reserved = 0;// fill correct data
    pTurnOnChannel->turn_on_channel_defined = (unsigned char)turn_on_channel.bTurnOnChannelDefined;
    if(pTurnOnChannel->turn_on_channel_defined == 1)
        pTurnOnChannel->turn_on_vertual_channel = (unsigned short)turn_on_channel.iTurnOnChannel;
    else
        pTurnOnChannel->turn_on_vertual_channel = 0;
    *pData++ = (unsigned char )(( pTurnOnChannel->turn_on_channel_defined << 4 ) | ((pTurnOnChannel->turn_on_vertual_channel &0xF00) >> 8));
    *pData = (unsigned char )(pTurnOnChannel->turn_on_vertual_channel &0xFF);
    size = turn_on_channel_size;//sizeof(turn_on_channel_t);
    
    return size;
}
int terminal_association(uint8 *pData, uint32 dataLen)
{
    
    int size = 0,ii;
    VL_POD_HOST_FEATURE_TERMINAL_ASSOCIATION   terminal_association ;
	memset(&terminal_association,0,sizeof(terminal_association));
    VL_HOST_GENERIC_FEATURES_RESULT iRet;

    if(pData == NULL || dataLen < 1 )
    {
        return -1;
    }
    iRet = vlPodGetGenericFeatureData( VL_HOST_FEATURE_PARAM_Terminal_Association,&terminal_association);
    if(VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS != iRet)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","purchase_pin:vlPodGetGenericFeatureData returned Error !! :%d \n",iRet);
        return -1;
    }
    pData[0] = (unsigned char)((terminal_association.nTerminalAssociationLength & 0xFF00) >> 8);// fill correct data
    pData[1] = (unsigned char)(terminal_association.nTerminalAssociationLength &0xFF);// fill correct data
    size +=2;
    
    for(ii = 0; ii < terminal_association.nTerminalAssociationLength; ii++)
    {
        pData[ii+2] = terminal_association.szTerminalAssociationName[ii];
        size++;
    }
    
    return size;
}
int cdl_group_id(uint8 *pData, uint32 dataLen)
{
    
     int size = 0;
    VL_POD_HOST_FEATURE_CDL_GROUP_ID  download_group_id;
	memset(&download_group_id,0,sizeof(download_group_id));
    VL_HOST_GENERIC_FEATURES_RESULT iRet;

    if(pData == NULL || dataLen < 1 )
    {
        return -1;
    }
/*
    iRet = vlPodGetGenericFeatureData( VL_HOST_FEATURE_PARAM_Common_Download_Group_Id,&download_group_id);
    if(VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS != iRet)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","purchase_pin:vlPodGetGenericFeatureData returned Error !! :%d \n",iRet);
        return -1;
    }*/
    

    pData[0] = (unsigned char)( (vlg_CDL_GroupId & 0xFF00) >> 8);// fill correct data
    pData[1] = (unsigned char)(vlg_CDL_GroupId & 0xFF);// fill correct data
    
    size += 2;//sizeof(vlg_CDL_GroupId);
    
    return size;
}

int zip_code(uint8 *pData, uint32 dataLen)
{
    
     int size = 0,ii;
    VL_POD_HOST_FEATURE_ZIP_CODE  zip_code ;
	memset(&zip_code,0,sizeof(zip_code));
    VL_HOST_GENERIC_FEATURES_RESULT iRet;

    if(pData == NULL || dataLen < 1 )
    {
        return -1;
    }
    iRet = vlPodGetGenericFeatureData( VL_HOST_FEATURE_PARAM_Zip_Code, &zip_code);
    if(VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS != iRet)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","purchase_pin:vlPodGetGenericFeatureData returned Error !! :%d \n",iRet);
        return -1;
    }
    

    pData[0] = (unsigned char)( (zip_code.nZipCodeLength & 0xFF00) >> 8);// fill correct data
    pData[1] = (unsigned char)(zip_code.nZipCodeLength & 0xFF);// fill correct data
    size += 2;
    for(ii = 0; ii < zip_code.nZipCodeLength; ii++)
    {
        pData[ii + 2] =  zip_code.szZipCode[ii];
        size++;
    }
    
    return size;
}
int set_rf_output_channel(uint8 *pData, uint32 dataLen)
{
//     //rf_out_channel_t *pRfOut;
    VL_POD_HOST_FEATURE_RF_CHANNEL rf_channel ;
	memset(&rf_channel,0,sizeof(rf_channel));
    VL_HOST_GENERIC_FEATURES_RESULT iRet;
    int size =0;
    
    if(pData == NULL || dataLen == 0)
    {
        return 0;
    }
     
    if( dataLen < sizeof(rf_out_channel_t) )
    {
        return dataLen;
    }
    //pRfOut = (rf_out_channel_t*)pData;
    rf_channel.nRfOutputChannel = pData[0];//pRfOut->output_channel; 
    rf_channel.bEnableRfOutputChannelUI = pData[1];//pRfOut->output_channel_ui; 

    vlg_Error_Set_Param_Mfr = 0;
    iRet = vlPodSetGenericFeatureData( VL_HOST_FEATURE_PARAM_RF_Output_Channel, &rf_channel, true);
    if(VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS != iRet)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","set_rf_output_channel:vlPodSetGenericFeatureData returned Error !! :%d \n",iRet);
        //return -1;
        vlg_Error_Set_Param_Mfr = 1;
    }
        
    size += 2;//sizeof(rf_out_channel_t);
    return size;

}

int set_p_c_pin(uint8 *pData, uint32 dataLen)
{
    int ii,size=0;
    //p_c_pin_t *pPcPin;
    VL_HOST_GENERIC_FEATURES_RESULT iRet;
    VL_POD_HOST_FEATURE_PARENTAL_CONTROL_PIN   parental_control_pin ;
	memset(&parental_control_pin,0,sizeof(parental_control_pin));
    
    if(pData == NULL || dataLen == 0)
    {
        return 0;
    }
//    pPcPin = (p_c_pin_t*)pData;
    
    if(dataLen < (pData[0]/*p_c_pin_length*/ + 1) )
    {
        return dataLen;
    }

#if 0
    if(pData[0]/*p_c_pin_length*/  > VL_HOST_FEATURE_MAX_PIN_SIZE )
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","set_p_c_pin: Error in p_c_pin_length:%d \n",pData[0]/*p_c_pin_length*/ );
        return -1;
    }
#endif
    parental_control_pin.nParentalControlPinSize = pData[0]/*p_c_pin_length*/ ;//pPcPin->p_c_pin_length; 
    
    size++;
    
    for(ii = 0; ii < parental_control_pin.nParentalControlPinSize; ii++)
    {
        parental_control_pin.aParentalControlPin[ii] = pData[ii+1];//pPcPin->p_c_pin_chr[ii] ; //Fill out the actual pin
        size++;
    }
    vlg_Error_Set_Param_Mfr = 0;
    iRet = vlPodSetGenericFeatureData( VL_HOST_FEATURE_PARAM_Parental_Control_PIN,      &parental_control_pin, true);
    
    if(VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS != iRet)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","set_p_c_pin:vlPodGetGenericFeatureData returned Error !! :%d \n",iRet);
        vlg_Error_Set_Param_Mfr = 1;
        return -1;
    }
    
    
    return size;
}
int set_p_c_settings(uint8 *pData, uint32 dataLen)
{
    int ii,size=0;
    uint8 *pTempData;
    //p_c_settings_t *pPcSettings;
    VL_HOST_GENERIC_FEATURES_RESULT iRet;
    VL_POD_HOST_FEATURE_PARENTAL_CONTROL_SETTINGS  parental_control_settings ;
	memset(&parental_control_settings,0,sizeof(parental_control_settings));
    if(pData == NULL || dataLen < 3)
    {
        return 0;
    }
    //pPcSettings = (p_c_settings_t*)pData;
    
    parental_control_settings.ParentalControlReset = pData[0];//pPcSettings->p_c_factory_reset ; //Fill the correct values as required
    parental_control_settings.nParentalControlChannels = (unsigned short )((pData[1] << 8) | pData[2]);//pPcSettings->p_c_channel_count_lo;
    //parental_control_settings.nParentalControlChannels |= (unsigned short )(pPcSettings->p_c_channel_count_hi << 8 );

    size = 3;//+ sizeof(p_c_chan_num_t) * parental_control_settings.nParentalControlChannels;

#if 0    
    if(dataLen < size)
    {
        return dataLen;
    }
#endif	
    pTempData = pData + 3;
	rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(unsigned long) * parental_control_settings.nParentalControlChannels, (void **)&parental_control_settings.paParentalControlChannels);
    if(parental_control_settings.paParentalControlChannels == NULL)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","set_p_c_settings:  Error!!  rmf_osal_memAllocP() Failed \n");
        return -1;
    }
    for( ii = 0; ii < parental_control_settings.nParentalControlChannels; ii++)
    {
        parental_control_settings.paParentalControlChannels[ii] = (unsigned long)((/*pPcSettings->VirtCh[ii].channel[0]*/pTempData[0] &0x0F) << 16);
        parental_control_settings.paParentalControlChannels[ii] |= (unsigned long)(/*pPcSettings->VirtCh[ii].channel[1]*/pTempData[1]  << 8);
        parental_control_settings.paParentalControlChannels[ii] |= (unsigned long)(/*pPcSettings->VirtCh[ii].channel[2]*/pTempData[2]  );
        pTempData += 3;
        size += 3;
    
    }
    vlg_Error_Set_Param_Mfr = 0;
    iRet = vlPodSetGenericFeatureData( VL_HOST_FEATURE_PARAM_Parental_Control_Settings, &parental_control_settings, true);
    if(parental_control_settings.paParentalControlChannels)
		rmf_osal_memFreeP(RMF_OSAL_MEM_POD, parental_control_settings.paParentalControlChannels);
    if(VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS != iRet)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","set_p_c_settings:vlPodGetGenericFeatureData returned Error !! :%d \n",iRet);
        vlg_Error_Set_Param_Mfr = 1;
    }
     
    return size;
}



int set_purchase_pin(uint8 *pData, uint32 dataLen)
{
    int ii,size=0;
//    purchase_pin_t *pPurPin;
    VL_POD_HOST_FEATURE_PURCHASE_PIN  purchase_pin;
	memset(&purchase_pin,0,sizeof(purchase_pin));
    VL_HOST_GENERIC_FEATURES_RESULT iRet;
 

    if(pData == NULL || dataLen == 0)
    {
        return 0;
    }
//    pPurPin = (purchase_pin_t *)pData;
    
    size = pData[0] + 1;//pPurPin->purchase_pin_length + 1;
    
    if(dataLen < size)
    {
        return dataLen;
    }
    purchase_pin.nPurchasePinSize = pData[0];//pPurPin->purchase_pin_length;//Fill correct data... later
    size = 1;
    for(ii = 0; ii <purchase_pin.nPurchasePinSize; ii++)
    {
        purchase_pin.aPurchasePin[ii] = pData[1+ii];//pPurPin->purchase_pin_chr[ii];// fill correct data later
        size++;
    }
    vlg_Error_Set_Param_Mfr = 0;
    iRet = vlPodSetGenericFeatureData( VL_HOST_FEATURE_PARAM_IPPV_PIN, &purchase_pin, true);
    if(VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS != iRet)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","set_purchase_pin:vlPodGetGenericFeatureData returned Error !! :%d \n",iRet);
        vlg_Error_Set_Param_Mfr = 1;
    }
    
    
    return size;
}
int set_time_zone(uint8 *pData, uint32 dataLen)
{
    VL_POD_HOST_FEATURE_TIME_ZONE_OFFSET time_zone_offset ;
	memset(&time_zone_offset,0,sizeof(time_zone_offset));
    VL_HOST_GENERIC_FEATURES_RESULT iRet;
//    time_zone_t *pTimeZone;
    int size = 2;
    
    if(pData == NULL || dataLen < size)
    {
        return 0;
    }
    
    time_zone_offset.nTimeZoneOffset = (pData[0] << 8) | pData[1];
    vlg_Error_Set_Param_Mfr = 0;
    iRet = vlPodSetGenericFeatureData( VL_HOST_FEATURE_PARAM_Time_Zone,&time_zone_offset, true);
    if(VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS != iRet)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","set_time_zone:vlPodGetGenericFeatureData returned Error !! :%d \n",iRet);
        vlg_Error_Set_Param_Mfr = 1;
    }
    //pTimeZone = (time_zone_t *)pData;
    
    //time_zone_offset.nTimeZoneOffset = pTimeZone->time_zone_offset;//fill correct data
    
    
    
    return size;
}

int set_daylight_savings(uint8 *pData, uint32 dataLen)
{
    
//    daylight_savings_t *pDltSgs;
    int size = 0;
#ifdef GLI
    IARM_Bus_SYSMgr_EventData_t eventData;
#endif

    VL_POD_HOST_FEATURE_DAYLIGHT_SAVINGS  daylight_savings ;
	memset(&daylight_savings,0,sizeof(daylight_savings));
    VL_HOST_GENERIC_FEATURES_RESULT iRet;
    
    if(pData == NULL || dataLen == 0)
    {
        return 0;
    }
        
    //size = sizeof(daylight_savings_t);
    
//    if(dataLen < size)
//    {
//        return dataLen;
//    }
//    pDltSgs =(daylight_savings_t*) pData;
    daylight_savings.eDaylightSavingsType = (VL_POD_HOST_DAY_LIGHT_SAVINGS)pData[0];//(VL_POD_HOST_DAY_LIGHT_SAVINGS)pDltSgs->daylight_savings_control;
    size = 1;
    if(daylight_savings.eDaylightSavingsType == VL_POD_HOST_DAY_LIGHT_SAVINGS_USE_DLS)
    {
        daylight_savings.nDaylightSavingsDeltaMinutes = pData[1];//pDltSgs->daylight_savings_delta;
daylight_savings.tmDaylightSavingsEntryTime = (unsigned long)((pData[2] << 24)|(pData[3] << 16)|(pData[4] << 8)|pData[5]) ;//pDltSgs->daylight_savings_entry_time;
daylight_savings.tmDaylightSavingsExitTime = (unsigned long)((pData[6] << 24)|(pData[7] << 16)|(pData[8] << 8)|pData[9]) ;
    //    daylight_savings.tmDaylightSavingsExitTime = pDltSgs->daylight_savings_exit_time;
        size = daylight_savings_size;
    }
    vlg_Error_Set_Param_Mfr = 0;
#ifdef GLI
	eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_DST_OFFSET;
	eventData.data.systemStates.state = 2;
	eventData.data.systemStates.error = 0;
	snprintf( eventData.data.systemStates.payload,
	sizeof(eventData.data.systemStates.payload),"%d", daylight_savings.nDaylightSavingsDeltaMinutes );
	IARM_Bus_BroadcastEvent( IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData) );
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
		"GroundZero: DaylightSavingsDeltaMinutes 0x%x \n", daylight_savings.nDaylightSavingsDeltaMinutes );
#endif
    iRet = vlPodSetGenericFeatureData( VL_HOST_FEATURE_PARAM_Daylight_Savings_Control,&daylight_savings, true);
    if(VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS != iRet)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","set_daylight_savings:vlPodGetGenericFeatureData returned Error !! :%d \n",iRet);
        vlg_Error_Set_Param_Mfr = 1;
    }

    return size;
}

int set_ac_outlet(uint8 *pData, uint32 dataLen)
{
    
//    ac_outlet_t *pAcOutlet;
    int size = 0;
    VL_POD_HOST_FEATURE_AC_OUTLET   ac_outlet ;
	memset(&ac_outlet,0,sizeof(ac_outlet));
    VL_HOST_GENERIC_FEATURES_RESULT iRet;

    if(pData == NULL || dataLen < 1 )
    {
        return -1;
    }
//    pAcOutlet = (ac_outlet_t *)pData;
    ac_outlet.eAcOutletSetting = (VL_POD_HOST_AC_OUTLET)pData[0];//(VL_POD_HOST_AC_OUTLET)(pAcOutlet->ac_outlet_control);// fill correct data
    size += 1;//sizeof(ac_outlet_t);
    vlg_Error_Set_Param_Mfr = 0;
    iRet = vlPodSetGenericFeatureData( VL_HOST_FEATURE_PARAM_AC_Outlet,&ac_outlet, true);
    if(VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS != iRet)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","set_ac_outlet:vlPodGetGenericFeatureData returned Error !! :%d \n",iRet);
        vlg_Error_Set_Param_Mfr = 1;
    }    
    
    
    return size;
}
int set_language(uint8 *pData, uint32 dataLen)
{
    
    //language_t *pLang;
    int size = 0;
    VL_HOST_GENERIC_FEATURES_RESULT iRet;
    VL_POD_HOST_FEATURE_LANGUAGE language;
	memset(&language,0,sizeof(language));
    size = 3;//sizeof(language_t);
    if(pData == NULL || dataLen < size)
    {
        return 0;
    }

    
    
    
    //pLang = (language_t *)pData;
        
    language.szLanguageControl[0] = pData[0];//pLang->language_control[0];// fill correct data
    language.szLanguageControl[1] = pData[1];//pLang->language_control[1];// fill correct data
    language.szLanguageControl[2] = pData[2];//pLang->language_control[2];// fill correct data
    
    vlg_Error_Set_Param_Mfr = 0;
    iRet = vlPodSetGenericFeatureData( VL_HOST_FEATURE_PARAM_Language, &language, true);
    if(VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS != iRet)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","set_language:vlPodGetGenericFeatureData returned Error !! :%d \n",iRet);
        vlg_Error_Set_Param_Mfr = 1;
    }    

    
    
    return size;
}


int set_reset_pin(uint8 *pData, uint32 dataLen)
{
    
    //reset_pin_t *pResetPin;
    int size = 0;
    VL_POD_HOST_FEATURE_RESET_PINS   reset_pins ;
	memset(&reset_pins,0,sizeof(reset_pins));
    VL_HOST_GENERIC_FEATURES_RESULT iRet;

    size = 1;//sizeof(reset_pin_t);

    if(pData == NULL || dataLen < size)
    {
        return 0;
    }
    
    //pResetPin = (reset_pin_t *)pData;
        
    reset_pins.eResetPins = (VL_POD_HOST_RESET_PIN)pData[0];//(VL_POD_HOST_RESET_PIN)pResetPin->reset_pin_control;// fill correct data
    
    
    
    vlg_Error_Set_Param_Mfr = 0;
    iRet = vlPodSetGenericFeatureData( VL_HOST_FEATURE_PARAM_Reset_PINS,&reset_pins, true);
    if(VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS != iRet)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","set_reset_pin:vlPodGetGenericFeatureData returned Error !! :%d \n",iRet);
        vlg_Error_Set_Param_Mfr = 1;
    }
    
    return size;
}
int set_cable_urls(uint8 *pData, uint32 dataLen)
{
    
    //cable_urls_t *pCableUrls;
    int ii,size = 0;
    VL_HOST_GENERIC_FEATURES_RESULT iRet=VL_HOST_GENERIC_FEATURES_RESULT_ERROR;
    VL_POD_HOST_FEATURE_CABLE_URLS url_info;
	memset(&url_info,0,sizeof(url_info));

    if(pData == NULL || dataLen < 1)
    {
        
        return -1;
    }
    url_info.nUrls = *pData++;
    size++;
	rmf_osal_memAllocP(RMF_OSAL_MEM_POD, url_info.nUrls*sizeof(VL_POD_HOST_FEATURE_URL) ,(void **)&url_info.paUrls);
    if(url_info.paUrls == NULL)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","set_cable_urls:  Error !! rmf_osal_memAllocP failed\n");
        return -1;
    }
    for(ii = 0; ii < url_info.nUrls; ii++)
    {
        url_info.paUrls[ii].eUrlType = (VL_POD_HOST_URL_TYPE)*pData++;
        url_info.paUrls[ii].nUrlLength = (int)*pData++;
        size += 2;
        rmf_osal_memcpy(url_info.paUrls[ii].szUrl,pData,url_info.paUrls[ii].nUrlLength,
				sizeof(url_info.paUrls[ii].szUrl), url_info.paUrls[ii].nUrlLength );
        pData += url_info.paUrls[ii].nUrlLength;
        size += url_info.paUrls[ii].nUrlLength;
    }
    vlg_Error_Set_Param_Mfr = 0;
    iRet = vlPodSetGenericFeatureData( VL_HOST_FEATURE_PARAM_Cable_URLS,&url_info, true);
    if(url_info.paUrls)
		rmf_osal_memFreeP(RMF_OSAL_MEM_POD, url_info.paUrls);
    if(VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS != iRet)
    {
        vlg_Error_Set_Param_Mfr = 1;
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","set_cable_urls:vlPodGetGenericFeatureData returned Error !! :%d \n",iRet);
        return -1;
    }

    
    return size;
}
int set_rating_region(uint8 *pData, uint32 dataLen)
{
    
    //rating_region_t *pRatingRegion;
    int size = 0;
    VL_POD_HOST_FEATURE_RATING_REGION   rating_region ;
	memset(&rating_region,0,sizeof(rating_region));
    VL_HOST_GENERIC_FEATURES_RESULT iRet;

    size = 1;//sizeof(rating_region_t);
    if(pData == NULL || dataLen < size)
    {
        return 0;
    }

    
        
    //pRatingRegion = (rating_region_t *)pData;
        
    rating_region.eRatingRegion = (VL_POD_HOST_RATING_REGION)pData[0];//(VL_POD_HOST_RATING_REGION)pRatingRegion->rating_region_setting;// fill correct data
    
    vlg_Error_Set_Param_Mfr = 0;
    iRet = vlPodSetGenericFeatureData( VL_HOST_FEATURE_PARAM_Rating_Region, &rating_region, true);
    if(VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS != iRet)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","set_rating_region:vlPodGetGenericFeatureData returned Error !! :%d \n",iRet);
        vlg_Error_Set_Param_Mfr = 1;
    }    

    
    
    return size;
}


int set_EA_location_code(uint8 *pData, uint32 dataLen)
{
    
    EA_location_code_t *pEALocationCode;
    int ii,size=0;
    VL_HOST_GENERIC_FEATURES_RESULT iRet;
    VL_POD_HOST_FEATURE_EAS eas_state_info ;
	memset(&eas_state_info,0,sizeof(eas_state_info));
    
    size = EA_location_code_Size;
    if(pData == NULL || dataLen < size)
    {
        return 0;
    }
    
    pEALocationCode = (EA_location_code_t *)pData;
    eas_state_info.EMStateCodel = pData[0];//pEALocationCode->state_code;// fill correct data
    eas_state_info.EMCSubdivisionCodel = ((pData[1]& 0xF0) >> 4); //pEALocationCode->county_subdivision;// fill correct data
    eas_state_info.EMCountyCode = (((pData[1]& 0x3) << 8)|pData[2]);//pEALocationCode->county_code;// fill correct data
    
    
    vlg_Error_Set_Param_Mfr = 0;
    iRet = vlPodSetGenericFeatureData( VL_HOST_FEATURE_PARAM_EAS_location_code,&eas_state_info, true);
    if(VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS != iRet)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","set_EA_location_code:vlPodGetGenericFeatureData returned Error !! :%d \n",iRet);
        vlg_Error_Set_Param_Mfr = 1;
    }

    
    return size;
}
int set_vct_id(uint8 *pData, uint32 dataLen)
{
    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    rmf_osal_event_handle_t event_handle;
    rmf_osal_event_params_t event_params = {0};
	
    unsigned short *pVctIdData;
    //vct_id_t *pVct_id;
    int size = 0;
    VL_HOST_GENERIC_FEATURES_RESULT iRet;
    size = sizeof(vct_id_t);
    if(pData == NULL || dataLen < size)
    {
        return 0;
    }
    
    
    
    //pVct_id = (vct_id_t *)pData;
	rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(unsigned short), (void **)&pVctIdData);
    if(pVctIdData == NULL)
        return -1;
    vlg_VctId = (pData[0] << 8)| pData[1];//pVct_id->vct_id;// fill correct data
    *pVctIdData  = vlg_VctId;
   
    event_params.priority = 0; //Default priority;
    event_params.data = (void*)pVctIdData;
    event_params.data_extension = sizeof(vlg_VctId );
    event_params.free_payload_fn = vlFeatParamFreeMem;
    rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_CARD_MANAGER, CardMgr_DSG_Cable_Card_VCT_ID, 
		&event_params, &event_handle);

    rmf_osal_eventmanager_dispatch_event(em, event_handle);


    VL_POD_HOST_FEATURE_VCT_ID vct_info;
	memset(&vct_info,0,sizeof(vct_info));

    vct_info.nVctId = vlg_VctId;
    vlg_Error_Set_Param_Mfr = 0;
    iRet = vlPodSetGenericFeatureData( VL_HOST_FEATURE_PARAM_VCT_ID,&vct_info, true);
    if(VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS != iRet)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","set_vct_id:vlPodSetGenericFeatureData returned Error !! :%d \n",iRet);
        vlg_Error_Set_Param_Mfr = 1;
    }



    return size;
}

int set_turn_on_channel(uint8 *pData, uint32 dataLen)
{
    
    //turn_on_channel_t *pTurnOnChannel;
    int size = 0;
    VL_POD_HOST_FEATURE_TURN_ON_CHANNEL   turn_on_channel ;
	memset(&turn_on_channel,0,sizeof(turn_on_channel));
    VL_HOST_GENERIC_FEATURES_RESULT iRet;

    size = turn_on_channel_size;
    if(pData == NULL || dataLen < size)
    {
        return 0;
    }
    
    

    //pTurnOnChannel = (turn_on_channel_t *)pData;


    turn_on_channel.bTurnOnChannelDefined = ((pData[0] & 0x10) >> 4);//pTurnOnChannel->turn_on_channel_defined;
    if(turn_on_channel.bTurnOnChannelDefined == 1)
    {
        turn_on_channel.iTurnOnChannel = (pData[0] & 0x0F)  ;//pTurnOnChannel->turn_on_vertual_channel;
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","turn_on_channel.iTurnOnChannel:%d pData[1]:%d\n",turn_on_channel.iTurnOnChannel,pData[1]);
        turn_on_channel.iTurnOnChannel = (int)((turn_on_channel.iTurnOnChannel<< 8) | pData[1]);//pTurnOnChannel->turn_on_vertual_channel;
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","turn_on_channel.iTurnOnChannel:%d \n",turn_on_channel.iTurnOnChannel);
    }
    else
        turn_on_channel.iTurnOnChannel = 0;
    vlg_Error_Set_Param_Mfr = 0;

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","bTurnOnChannelDefined:%d  turn_on_channel.iTurnOnChannel:%d \n",turn_on_channel.bTurnOnChannelDefined,turn_on_channel.iTurnOnChannel);
    iRet = vlPodSetGenericFeatureData( VL_HOST_FEATURE_PARAM_Turn_on_Channel,&turn_on_channel, true);
    if(VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS != iRet)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","set_turn_on_channel:vlPodGetGenericFeatureData returned Error !! :%d \n",iRet);
        vlg_Error_Set_Param_Mfr = 1;
    }
    

    
    
    return size;
}

int set_terminal_association(uint8 *pData, uint32 dataLen)
{
    
    //terminal_association_t *pTermAss;
    int size = 0,ii;
    VL_POD_HOST_FEATURE_TERMINAL_ASSOCIATION   terminal_association ;
	memset(&terminal_association,0,sizeof(terminal_association));
    VL_HOST_GENERIC_FEATURES_RESULT iRet;

    if(pData == NULL || dataLen < 2)
    {
        return 0;
    }
    //pTermAss = (terminal_association_t *)pData;

    terminal_association.nTerminalAssociationLength = (pData[0] << 8) | pData[1];//pTermAss->identifier_length;// fill correct data
    size = terminal_association.nTerminalAssociationLength + 2;
    if(dataLen < size)
    {
        return dataLen;
    }
    
    
    for(ii = 0; ii < terminal_association.nTerminalAssociationLength; ii++)
    {
        if(ii < sizeof(terminal_association.szTerminalAssociationName))
        {
            terminal_association.szTerminalAssociationName[ii] = pData[ii+2];//pTermAss->terminal_assoc_identifier[ii];
        }
        
    }
    vlg_Error_Set_Param_Mfr = 0;
    iRet = vlPodSetGenericFeatureData( VL_HOST_FEATURE_PARAM_Terminal_Association,&terminal_association, true);
    if(VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS != iRet)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","set_terminal_association:vlPodGetGenericFeatureData returned Error !! :%d \n",iRet);
        vlg_Error_Set_Param_Mfr = 1;
    }
    
    return size;
}

int set_cdl_group_id(uint8 *pData, uint32 dataLen)
{
    
     //cdl_group_id_t *pCdlgrpId;
    int size = 0;
    VL_POD_HOST_FEATURE_CDL_GROUP_ID  download_group_id;
	memset(&download_group_id,0,sizeof(download_group_id));
    VL_HOST_GENERIC_FEATURES_RESULT iRet;
    size = 2;//sizeof(cdl_group_id_t);
    if(pData == NULL || dataLen < size)
    {
        return 0;
    }
    
    
    //pCdlgrpId = (cdl_group_id_t *)pData;

    download_group_id.common_download_group_id = (pData[0] << 8) | pData[1];//pCdlgrpId->group_id;// fill correct data
    vlg_CDL_GroupId = (pData[0] << 8) | pData[1];
    
    vlg_Error_Set_Param_Mfr = 0;
    iRet = vlPodSetGenericFeatureData( VL_HOST_FEATURE_PARAM_Common_Download_Group_Id,&download_group_id, true);
    if(VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS != iRet)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","set_cdl_group_id:vlPodGetGenericFeatureData returned Error !! :%d \n",iRet);
        vlg_Error_Set_Param_Mfr = 1;
    }

    return size;
}


int set_zip_code(uint8 *pData, uint32 dataLen)
{
    
     zip_code_t *pZipCode;
    int size = 0,ii;
    VL_POD_HOST_FEATURE_ZIP_CODE  zip_code ;
	memset(&zip_code,0,sizeof(zip_code));
    VL_HOST_GENERIC_FEATURES_RESULT iRet;

    if(pData == NULL || dataLen < 2)
    {
        return dataLen;
    }
    //pZipCode = (zip_code_t *)pData;

    zip_code.nZipCodeLength = (pData[0] << 8) | pData[1];//pZipCode->zip_code_len;// fill correct data

    size = zip_code.nZipCodeLength + 2;
    if(dataLen < size)
    {
        return dataLen;
    }
    
    
    for(ii = 0; ii < zip_code.nZipCodeLength; ii++)
    {
        if(ii < sizeof(zip_code.szZipCode))
        {
             zip_code.szZipCode[ii] = pData[ii+2];//pZipCode->zip_code[ii];
        }
    }
    vlg_Error_Set_Param_Mfr = 0;
    iRet = vlPodSetGenericFeatureData( VL_HOST_FEATURE_PARAM_Zip_Code, &zip_code, true);
    if(VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS != iRet)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","set_zip_code:vlPodGetGenericFeatureData returned Error !! :%d \n",iRet);
        vlg_Error_Set_Param_Mfr = 1;
    }
    
    return size;
}
int rf_output_channel_length(uint8 *pData, uint32 dataLen)
{
    rf_out_channel_t *pRfOut;
    int size =0;
    
    if(pData == NULL || dataLen == 0)
    {
        return 0;
    }
     
    if( dataLen < sizeof(rf_out_channel_t) )
    {
        return dataLen;
    }
    
    size = sizeof(rf_out_channel_t);
    
    return size;
}

int p_c_pin_length(uint8 *pData, uint32 dataLen)
{
    int ii,size=0;
    p_c_pin_t *pPcPin;
    
    if(pData == NULL || dataLen == 0)
    {
        return 0;
    }
    pPcPin = (p_c_pin_t*)pData;
    
    if(dataLen < (pPcPin->p_c_pin_length + 1) )
    {
        return dataLen;
    }
    size = (pPcPin->p_c_pin_length + 1);
    
    return size;


}

int p_c_settings_length(uint8 *pData, uint32 dataLen)
{
    int ii,size=0;
    p_c_settings_t *pPcSettings;
    
    if(pData == NULL || dataLen == 0)
    {
        return 0;
    }
    pPcSettings = (p_c_settings_t*)pData;
    
    
    size = 3+ sizeof(p_c_chan_num_t) * pPcSettings->p_c_channel_count_hi;
    
    if(dataLen < size)
    {
        return dataLen;
    }
    
    
    return size;


}

int purchase_pin_length(uint8 *pData, uint32 dataLen)
{
    int ii,size=0;
    purchase_pin_t *pPurPin;
    
    if(pData == NULL || dataLen == 0)
    {
        return 0;
    }
    pPurPin = (purchase_pin_t *)pData;
    
    size = pPurPin->purchase_pin_length + 1;
    
    if(dataLen < size)
    {
        return dataLen;
    }
    
    return size;
}

int time_zone_length(uint8 *pData, uint32 dataLen)
{
    
    time_zone_t *pTimeZone;
    int size = 0;
    
    if(pData == NULL || dataLen == 0)
    {
        return 0;
    }
    
    size = sizeof(time_zone_t);
    
    if(dataLen < size)
    {
        return dataLen;
    }
    
    return size;
}

int daylight_savings_length(uint8 *pData, uint32 dataLen)
{
    
    daylight_savings_t *pDltSgs;
    int size = 0;
    
    if(pData == NULL || dataLen == 0)
    {
        return 0;
    }
        
    size = daylight_savings_size;
    
    if(dataLen < size)
    {
        return dataLen;
    }
    
    return size;
}

int ac_outlet_length(uint8 *pData, uint32 dataLen)
{
    
    ac_outlet_t *pAcOutlet;
    int size = 0;
    
    if(pData == NULL || dataLen == 0)
    {
        return 0;
    }
    
    size = sizeof(ac_outlet_t);
    
    if(dataLen < size)
    {
        return dataLen;
    }
    
    return size;
}

int language_length(uint8 *pData, uint32 dataLen)
{
    
    language_t *pLang;
    int size = 0;
    
    if(pData == NULL || dataLen == 0)
    {
        return 0;
    }

    size = sizeof(language_t);
    
    if(dataLen < size)
    {
        return dataLen;
    }
    
    return size;
}


int rating_region_length(uint8 *pData, uint32 dataLen)
{
    
    rating_region_t *pRatingRegion;
    int size = 0;
    
    if(pData == NULL || dataLen == 0)
    {
        return 0;
    }

    size = sizeof(rating_region_t);
    
    if(dataLen < size)
    {
        return dataLen;
    }
    
    return size;
}



int reset_pin_length(uint8 *pData, uint32 dataLen)
{
    
    reset_pin_t *pResetPin;
    int size = 0;
    
    if(pData == NULL || dataLen == 0)
    {
        return 0;
    }
    
    size = sizeof(reset_pin_t);
    
    if(dataLen < size)
    {
        return dataLen;
    }
    
    return size;
}

int cable_urls_length(uint8 *pData, uint32 dataLen)
{
    
    cable_urls_t *pCableUrls;
    int ii,size = 0;
    
    if(pData == NULL || dataLen == 0)
    {
        return 0;
    }
    pCableUrls = (cable_urls_t *)pData;
        
//    pCableUrls->number_of_urls;
    size = 1;
    for(ii = 0; ii < pCableUrls->number_of_urls; ii++)
    {
        size = size + 2+ pCableUrls->url[ii].url_length;
    }
    
    if(dataLen < size)
    {
        return dataLen;
    }
    
    return size;
}


int EA_location_code_length(uint8 *pData, uint32 dataLen)
{
    EA_location_code_t *pEALocationCode;
    int ii,size=0;
    
    if(pData == NULL || dataLen == 0)
    {
        return 0;
    }
    
    size = EA_location_code_Size;
    if(dataLen < size)
    {
        return dataLen;
    }
    
    return size;
}
    
/******************************************************
 *    APDU Functions to handle GENERIC FEATURE CONTROL
 *****************************************************/
int fetureSndApdu(USHORT sesnum,uint32 tag, uint32 dataLen, uint8 *data)
{
    uint16 alen;
    uint8 *papdu;
    int ii;
   // uint8 papdu[MAX_APDU_HEADER_BYTES + dataLen];
	rmf_osal_memAllocP(RMF_OSAL_MEM_POD, MAX_APDU_HEADER_BYTES+dataLen,(void **)&papdu);
    if(papdu == NULL)
        return EXIT_FAILURE;
    //MDEBUG ( DPM_APPINFO, "ai:Called, tag = 0x%x, len = %d \n", tag, dataLen);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," Entered fetureSndApdu() #### \n");
    if (buildApdu(papdu, &alen, tag, dataLen, data ) == NULL)
    {
        //MDEBUG (DPM_ERROR, "ERROR:ai: Unable to build apdu.\n");
		rmf_osal_memFreeP(RMF_OSAL_MEM_POD, papdu);
        return EXIT_FAILURE;
    }    

#if 0
    if(rdk_dbg_enabled("LOG.RDK.POD", RDK_LOG_DEBUG))
    {
      for(ii = 0; ii < alen; ii++)
      {
          printf("%02X ",papdu[ii]);
          if(  ((ii+1)%16) == 0)    
              printf("\n");
      }
    }
#endif
    vlMpeosDumpBuffer(RDK_LOG_DEBUG, "LOG.RDK.POD", papdu, alen);
    
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","calling AM_APDU_FROM_HOST() ###### \n");
    AM_APDU_FROM_HOST(sesnum, papdu, alen);
	rmf_osal_memFreeP(RMF_OSAL_MEM_POD, papdu);
    return EXIT_SUCCESS;
}

 

/*
 *    APDU in: 0x9F, 0x98, 0x02, "feature_list_req"
 */
int  APDU_FeatureListReq (USHORT sesnum, UCHAR * apkt, USHORT len/*, int tagValue, int eventID*/)                     
{
    unsigned char *ptr;

    unsigned long tag = 0;
    unsigned short len_field = 0;
    unsigned char *payload;
    unsigned char tagvalue[3] = {0x9f, 0x98, 0x02};    // 

 
    ptr = (unsigned char *)apkt;
     
    if (memcmp(ptr, tagvalue, 3) != 0) {
        MDEBUG (DPM_FATAL, "pod > fc: Non-matching tag in OpenReq.\n");
        return EXIT_FAILURE;
    }

    // Let's continue going about the parsing business
    ptr += 3;
    
    // Figure out length field


        ptr += bGetTLVLength ( ptr, &len_field );

    
#if 0    
    if(len_field)
    {
        // Get the rest of the stuff
		rmf_osal_memAllocP(RMF_OSAL_MEM_POD, len_field * sizeof(unsigned char),(void **)&payload);
        if (payload == NULL)
        {
            MDEBUG (DPM_FATAL, "pod > fc: Unable to allocate memory for OpenReq.\n");
        
            return EXIT_FAILURE;
        }
        //memcpy(payload, ptr, len_field*sizeof(unsigned char));
        memcpy(payload, ptr, len_field*sizeof(unsigned char));
    }        
    else
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," Data Len is zero ################## \n");
    }
#endif
    {
        //payload = (unsigned char*)malloc(FeatureListSize + 1);
		rmf_osal_memAllocP(RMF_OSAL_MEM_POD, FeatureListSize + 1,(void **)&payload);
        if (payload == NULL)
        {
            MDEBUG (DPM_FATAL, "pod > fc: Unable to allocate memory for OpenReq.\n");
        
            return EXIT_FAILURE;
        }
        *payload = FeatureListSize;
        //memcpy((payload+1), FeatureList, FeatureListSize);
        memcpy((payload+1), FeatureList, FeatureListSize);
        fetureSndApdu(sesnum,0x9F9803, FeatureListSize + 1, payload);
        if(payload)
			rmf_osal_memFreeP(RMF_OSAL_MEM_POD, payload);
    }            
    
    // Now forward the thing to BB
    //featureSndEvent( POD_FEATURE_LIST_REQ, payload, len_field, 0, 0, 0);

    //        rmf_osal_memFreeP(RMF_OSAL_MEM_POD,payload); Hannah - done in mmi.cpp in the cardmanager resource
    return 1;
}

/*
 *    APDU in: 0x9F, 0x98, 0x03, "feature_list"
 */
int  APDU_FeatureList (USHORT sesnum, UCHAR * apkt, USHORT len)                             
{
    //uint8 apdu[4];
    UCHAR numFet;
    uint8 papdu[MAX_APDU_HEADER_BYTES ];
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," Entered APDU_FeatureList ############ \n");
    //testfunction();
    numFet = apkt[3];
    //memcpy(CardFeatureList,&apkt[4],numFet);
    rmf_osal_memcpy(CardFeatureList,&apkt[4],numFet, MAX_FEATURE_LIST_SIZE, numFet);
     CardFeatureListSize = numFet;



    //Sending the feature_list_cnf to the Card
    papdu[0] = 0x9F;
    papdu[1] = 0x98;
    papdu[2] = 0x04;
    papdu[3] = 0x00;
    AM_APDU_FROM_HOST(sesnum, papdu, 4);
    // sending the feature_list_req
    //fetureSndApdu(sesnum,0x9F9802, 4, apdu);
    return 1;
    
    //return 1;
}

void testfunction()
{
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," Entered testfunction() ######### \n");
}
/*
 *    APDU in: 0x9F, 0x98, 0x04, "feature_list_cnf"
 */
int  APDU_FeatureListCnf (USHORT sesnum, UCHAR * apkt, USHORT len)                     
{
    //uint8 apdu[4];
    uint8 papdu[MAX_APDU_HEADER_BYTES ];
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," Entered APDU_FeatureListCnf ############ \n");
    //testfunction();
    
    //Sending the feature_list_req to the Card
    papdu[0] = 0x9F;
    papdu[1] = 0x98;
    papdu[2] = 0x02;
    papdu[3] = 0x00;
    AM_APDU_FROM_HOST(sesnum, papdu, 4);
    // sending the feature_list_req
    //fetureSndApdu(sesnum,0x9F9802, 4, apdu);
    return 1;
}

/*
 *    APDU in: 0x9F, 0x98, 0x05, "feature_list_changed"
 */
int  APDU_FeatureListChanged (USHORT sesnum, UCHAR * apkt, USHORT len)         
{
    uint8 papdu[MAX_APDU_HEADER_BYTES ];
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," Entered APDU_FeatureListChanged ############ \n");
    //testfunction();
    
    //Sending the feature_list_req to the Card
    papdu[0] = 0x9F;
    papdu[1] = 0x98;
    papdu[2] = 0x02;
    papdu[3] = 0x00;
    AM_APDU_FROM_HOST(sesnum, papdu, 4);
    // sending the feature_list_req
    //fetureSndApdu(sesnum,0x9F9802, 4, apdu);
    return 1;
}

int  APDU_HostToCardFeatureParams (USHORT sesnum, unsigned char FeatureId )
{
    uint8 * pData=NULL,*pFeature=NULL,*pTemp=NULL,*pLen=NULL;
    uint32 mesize = MAX_MEM_SIZE_FET_LIST_PARAM;
    uint32 size,bytes=0; 
    uint8 numFeaturs=0;
    int ii,iRet,jj;
    int iFetureList;
 
	rmf_osal_memAllocP(RMF_OSAL_MEM_POD, mesize,(void **)&pData);
    
    size = mesize;
    if(pData == NULL)
       return -1;
    
    pFeature = pData;
    //numFeaturs = FeatureListSize;// - 1;// Temp.. sub of 1 bcoz HPNX client prog bug
    pTemp = pFeature;
    //*pFeature++ = numFeaturs;// number of features
    pFeature++;
    size--;
    bytes++;
    if(FeatureId == 0)
    {
        iFetureList = FeatureListSize;
    }
    else
    {
        iFetureList = 1;
    }
        
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","APDU_HostToCardFeatureParams:FeatureListSize:%d size:0x%X \n",FeatureListSize,size);
    for(ii = 0; ii < iFetureList; ii++)
    {
        uint32 FeatureType;
        if(FeatureId == 0)
        {
            FeatureType = *pFeature = FeatureList[ii];
        }
        else
        {
            FeatureType = *pFeature = FeatureId;
        }

        pFeature++;
        size--;
        bytes++;
        if(GetFeatureControlVer() == 4)
        {
            pLen = pFeature;
            pFeature += 2;
            size -= 2;
            bytes += 2;
        }
        //BUG FIX : 10/16/2008: SRINI //FeatureType = FeatureList[ii];
        
        iRet = 0;
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","FeatureID:%02X \n",*pFeature);
        switch(FeatureType)
        {
        case RESERVED:
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ### RESERVED ## \n");
            break;
        case RF_OUTPUT_CHANNEL:
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ### RF_OUTPUT_CHANNEL ## \n");
            iRet = rf_output_channel(pFeature,size);
            break;
        case P_C_PIN:
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ### P_C_PIN ## \n");
            iRet = p_c_pin(pFeature,size);
            break;
        case IPPV_PIN:
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ### IPPV_PIN ## \n");
            iRet = purchase_pin(pFeature,size);
            break;    
        case AC_OUTLET:
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ### AC_OUTLET ## \n");
            iRet = ac_outlet(pFeature,size);
            break;    
        case P_C_SETTINGS:
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ### P_C_SETTINGS ## \n");
            iRet = p_c_settings(pFeature,size);
            break;
        case TIME_ZONE:
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ### TIME_ZONE ## \n");
            iRet = time_zone(pFeature,size);
            break;
        case DAYLIGHT_SAVINGS:
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ### DAYLIGHT_SAVINGS ## \n");
            iRet = daylight_savings(pFeature,size);
            break;
        case LANGUAGE:
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ### LANGUAGE ## \n");
            iRet = language(pFeature,size);
            break;
        case RATING_REGION:
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ### RATING_REGION ## \n");
            iRet = rating_region(pFeature,size);
            break;
        case RESET_PIN:
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ### RESET_PIN ## \n");
            iRet = reset_pin(pFeature,size);
            break;    
        case CABLE_URLS:
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ### CABLE_URLS ## \n");
            iRet = cable_urls(pFeature,size);
            break;
        case EA_LOCATION_CODE:
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ### EA_LOCATION_CODE ## \n");
            iRet = EA_location_code(pFeature,size);
            break;
        case VCT_ID:
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ### VCT_ID ## \n");
            iRet = vct_id(pFeature,size);
            break;
          case TURN_ON_CHANNEL:
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ### TURN_ON_CHANNEL ## \n");
            iRet = turn_on_channel(pFeature,size);
            break;
          case TERMINAL_ASSOCIATION:
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ### TERMINAL_ASSOCIATION ## \n");
            iRet = terminal_association(pFeature,size);
            break;
          case DOWNLOAD_GP_ID:
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ### DOWNLOAD_GP_ID ## \n");
            iRet = cdl_group_id(pFeature,size);
            break;
          case ZIP_CODE:
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ### ZIP_CODE ## \n");
            iRet = zip_code(pFeature,size);
            break;
        default:
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD"," ---------- Error entered default --------------- \n");
            break;
        }//switch(FeatureType)    
          if(iRet < 0)
        {
            pFeature--;
            size++;
            bytes--;
            if(GetFeatureControlVer() == 4)
            {
                pFeature -= 2;
                size += 2;
                bytes -= 2;
            }
            continue;//break;//return -1;// Error
        }
        if( (GetFeatureControlVer() == 4) && ( pLen !=NULL ))
        {
            *pLen++ = (unsigned char )( (iRet&0xFF00) >> 8);
            *pLen = (unsigned char ) (iRet&0xFF);
        }
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," \nPayload of size iRet:%d \n", iRet);
        for(jj = 0; jj < iRet; jj++)
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","0x%02X ",pFeature[jj]);
            if( ((jj+1) % 4) == 0)
             RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","\n");
        }
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","\n");    
        pFeature = pFeature + iRet;
        size = size -iRet;
        bytes = bytes + iRet;
        numFeaturs++;
    }//for
    *pTemp = numFeaturs;
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","\n");
    for(ii = 0; ii < bytes; ii++)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%02X ",pTemp[ii]);
        if(  ((ii+1)%16) == 0)    
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","\n");
    }
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","\nbytes: %d numFeaturs:%d numFeaturs:%d\n",bytes,*pTemp,numFeaturs);
    // Sending the feature parameters to card...
    fetureSndApdu(sesnum,0x9F9807,(bytes), pTemp);
    
	rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pData);
    //        rmf_osal_memFreeP(RMF_OSAL_MEM_POD,payload); Hannah - done in mmi.cpp in the cardmanager resource
    return 1;
}
/*
 *    APDU in: 0x9F, 0x98, 0x06, "feature_parameters_req"
 */
int  APDU_FeatureParmReqFromCard (USHORT sesnum, UCHAR * apkt, USHORT len)             
{
     RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," Entered APDU_FeatureParmReqFromCard \n");
    APDU_HostToCardFeatureParams ( sesnum ,0);
    return 1;
}
void ResetCardFeatureList()
{
    CardFeatureListSize = 0;
}
int GetCardFeatureList(unsigned char *pList, unsigned char *pListSize)
{
    if(*pListSize < CardFeatureListSize)
        return -1;
    memcpy(pList,CardFeatureList,CardFeatureListSize);
    *pListSize = CardFeatureListSize;
    return 0;
}
/*
 *    APDU in: 0x9F, 0x98, 0x07, "feature_parameters"
 */
int  APDU_FeatureParmFromCard (USHORT sesnum, UCHAR * apkt, USHORT len)                  
{
    unsigned char *ptr;
    unsigned long tag = 0;
    unsigned short len_field = 0;
    unsigned char *payload,ii;
    unsigned char tagvalue[3] = {0x9f, 0x98, 0x07};    // 

 
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," Entered  APDU_FeatureParmFromCard ########\n");
    ptr = (unsigned char *)apkt;
     
    if (memcmp(ptr, tagvalue, 3) != 0)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","APDU_FeatureParmFromCard: fc: Non-matching tag in OpenReq.\n");
        return EXIT_FAILURE;
    }

    // Let's continue going about the parsing business
    ptr += 3;
    
    // Figure out length field


    ptr += bGetTLVLength ( ptr, &len_field );

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," Payload \n");
    #if 0
    if(rdk_dbg_enabled("LOG.RDK.POD", RDK_LOG_DEBUG))
    {
      for(ii = 0; ii < len; ii++)
      {
          printf("%02X ",apkt[ii]);
          if( (( ii + 1) % 16) == 0)
              printf("\n");
      }
      printf("\n");
    }
    #endif
    vlMpeosDumpBuffer(RDK_LOG_DEBUG, "LOG.RDK.POD", apkt,len);
    /*if(len_field)
    {
        // Get the rest of the stuff
        payload = (unsigned char*)malloc(len_field * sizeof(unsigned char));
        if (payload == NULL)
        {
            MDEBUG (DPM_FATAL, "pod > fc: Unable to allocate memory for OpenReq.\n");
        
            return EXIT_FAILURE;
        }
        memcpy(payload, ptr, len_field*sizeof(unsigned char));
    }        
    else*/
    {
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","APDU_FeatureParmFromCard: Data Len is :%d ################## \n",len_field);
    }
    {
        uint8 numOfFeats;
        int ii,PayloadSize;
        uint8 *pFeatureId,*pPayload,*pDataSnd,*pMem1,*pMem2;
        uint8 FeatureId;
        unsigned short FeatureIdLen=0;
        int iRet =  0;
        
        numOfFeats = *ptr++;
        len_field--;
		rmf_osal_memAllocP(RMF_OSAL_MEM_POD, numOfFeats,(void **)&pFeatureId);
        if(pFeatureId == NULL)
            return EXIT_FAILURE;
        pMem1 = pFeatureId;
        PayloadSize = numOfFeats*2 + 1;
        
		rmf_osal_memAllocP(RMF_OSAL_MEM_POD, PayloadSize,(void **)&pPayload);
        if(pPayload == NULL)
        {
		rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pMem1);
            return EXIT_FAILURE;
        }
        pMem2 = pPayload;
        pDataSnd = pPayload;
        *pPayload = numOfFeats;
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD",  "APDU_FeatureParmFromCard: ######## *pPayload:0x%X \n",*pPayload);
        pPayload++;
        
        
        
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","APDU_FeatureParmFromCard: number of features:%d len_field:%d\n", numOfFeats,len_field);
        //CardFeatureListSize = numOfFeats;
        for(ii = 0; ii < numOfFeats; ii++)
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," Loop : ii:%d \n",ii);
            if(len_field == 0)
                break;
                
            
            FeatureId = *ptr++;
            len_field--;
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","Feature Id:%d \n",FeatureId);
            
            if(GetFeatureControlVer() == 4)
            {
                FeatureIdLen = (ptr[0] << 8) | ptr[1];
                len_field -= 2;
                ptr += 2;
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Len:%d of Feature Id:%d ptr:0x%08X\n",FeatureIdLen,FeatureId,ptr);
            
            }
            *pFeatureId = FeatureId;
            pFeatureId++;
    
            //CardFeatureList[ii] = FeatureId;
            switch(FeatureId)
            {
            case RESERVED:
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","APDU_FeatureParmFromCard: RESERVED ######## \n");
                *pPayload = FeatureId;
                pPayload++;
                 //feature not supported for now, change it when required
                *pPayload = FatureSupport(FeatureId);
                pPayload++;        
                break;
            case RF_OUTPUT_CHANNEL:
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","APDU_FeatureParmFromCard: RF_OUTPUT_CHANNEL ######## \n");
                iRet = set_rf_output_channel(ptr,len_field);
                *pPayload = FeatureId;
                pPayload++;
                 //feature not supported for now, change it when required
                if(vlg_Error_Set_Param_Mfr)
                {
                    *pPayload = INVALID_PARAM;
                }
                else
                {
                    *pPayload = FatureSupport(FeatureId);
                }
                pPayload++;
                break;
            case P_C_PIN:
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","APDU_FeatureParmFromCard: P_C_PIN ######## \n");
                iRet = set_p_c_pin(ptr,len_field);
                *pPayload = FeatureId;
                pPayload++;
                 //feature not supported for now, change it when required
                *pPayload = FatureSupport(FeatureId);
                pPayload++;
                break;
            case IPPV_PIN:
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","APDU_FeatureParmFromCard: IPPV_PIN ######## \n");
                iRet = set_purchase_pin(ptr,len_field);
                *pPayload = FeatureId;
                pPayload++;
                 //feature not supported for now, change it when required
                *pPayload = FatureSupport(FeatureId);
                pPayload++;
                break;    
            case AC_OUTLET:
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","APDU_FeatureParmFromCard: AC_OUTLET ######## \n");
                iRet = set_ac_outlet(ptr,len_field);
                *pPayload = FeatureId;
                pPayload++;
                 //feature not supported for now, change it when required
                *pPayload = FatureSupport(FeatureId);
                pPayload++;
                break;    
            case P_C_SETTINGS:
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","APDU_FeatureParmFromCard: P_C_SETTINGS ######## \n");
                iRet = set_p_c_settings(ptr,len_field);
                *pPayload = FeatureId;
                pPayload++;
                 //feature not supported for now, change it when required
                *pPayload = FatureSupport(FeatureId);
                pPayload++;
                break;
            case TIME_ZONE:
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","APDU_FeatureParmFromCard: TIME_ZONE ######## \n");
                iRet = set_time_zone(ptr,len_field);
                *pPayload = FeatureId;
                pPayload++;
                 //feature not supported for now, change it when required
                *pPayload = FatureSupport(FeatureId);
                pPayload++;
                break;
            case DAYLIGHT_SAVINGS:
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","APDU_FeatureParmFromCard: DAYLIGHT_SAVINGS ######## \n");
                iRet = set_daylight_savings(ptr,len_field);
                *pPayload = FeatureId;
                pPayload++;
                 //feature not supported for now, change it when required
                *pPayload = FatureSupport(FeatureId);
                pPayload++;
                break;
        
            case LANGUAGE:
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","APDU_FeatureParmFromCard: LANGUAGE ######## \n");
                iRet = set_language(ptr,len_field);
                *pPayload = FeatureId;
                pPayload++;
                 //feature not supported for now, change it when required
                *pPayload = FatureSupport(FeatureId);
                pPayload++;
                break;
            case RATING_REGION:
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","APDU_FeatureParmFromCard: RATING_REGION ######## \n");
                iRet = set_rating_region(ptr,len_field);
                *pPayload = FeatureId;
                pPayload++;
                 //feature not supported for now, change it when required
                *pPayload = FatureSupport(FeatureId);
                pPayload++;
                break;
            case RESET_PIN:
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","APDU_FeatureParmFromCard: RESET_PIN ######## \n");
                iRet = set_reset_pin(ptr,len_field);
                *pPayload = FeatureId;
                pPayload++;
                 //feature not supported for now, change it when required
                *pPayload = FatureSupport(FeatureId);
                pPayload++;
                break;    
            case CABLE_URLS:
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","APDU_FeatureParmFromCard: CABLE_URLS ######## \n");
                iRet = set_cable_urls(ptr,len_field);
                *pPayload = FeatureId;
                pPayload++;
                 //feature not supported for now, change it when required
                *pPayload = FatureSupport(FeatureId);
                pPayload++;
                break;
            case EA_LOCATION_CODE:
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","APDU_FeatureParmFromCard: EA_LOCATION_CODE ######## \n");
                iRet = set_EA_location_code(ptr,len_field);
                *pPayload = FeatureId;
                pPayload++;
                 //feature not supported for now, change it when required
                *pPayload = FatureSupport(FeatureId);
                pPayload++;
                break;
            case VCT_ID:
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","APDU_FeatureParmFromCard: VCT_ID ######## \n");
                iRet = set_vct_id(ptr,len_field);
                *pPayload = FeatureId;
                pPayload++;
                 //feature not supported for now, change it when required
                *pPayload = FatureSupport(FeatureId);
                pPayload++;
                break;
              case TURN_ON_CHANNEL:
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","APDU_FeatureParmFromCard: TURN_ON_CHANNEL ######## \n");
                iRet = set_turn_on_channel(ptr,len_field);
                *pPayload = FeatureId;
                pPayload++;
                 //feature not supported for now, change it when required
                *pPayload = FatureSupport(FeatureId);
                pPayload++;
                break;
              case TERMINAL_ASSOCIATION:
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","APDU_FeatureParmFromCard: TERMINAL_ASSOCIATION ######## ptr:0x%08X\n",ptr);
                iRet = set_terminal_association(ptr,len_field);
                *pPayload = FeatureId;
                pPayload++;
                 //feature not supported for now, change it when required
                *pPayload = FatureSupport(FeatureId);
                pPayload++;
                break;
              case DOWNLOAD_GP_ID:
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","APDU_FeatureParmFromCard: DOWNLOAD_GP_ID ######## \n");
                iRet = set_cdl_group_id(ptr,len_field);
                *pPayload = FeatureId;
                pPayload++;
                 //feature not supported for now, change it when required
                *pPayload = FatureSupport(FeatureId);
                pPayload++;
                break;
              case ZIP_CODE:
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","APDU_FeatureParmFromCard: ZIP_CODE ######## \n");
                iRet = set_zip_code(ptr,len_field);
                *pPayload = FeatureId;
                pPayload++;
                 //feature not supported for now, change it when required
                *pPayload = FatureSupport(FeatureId);
                pPayload++;
                break;
            default:
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","APDU_FeatureParmFromCard: Error in the Feture type:%d \n",FeatureId);
                break;
            }//switch(FeatureType)    
            if(iRet < 0)
                continue;//return -1;

            if(GetFeatureControlVer() == 4)
            {
                if(FeatureIdLen != iRet)
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD"," Error FeatureIdLen:%d != iRet:%d \n",FeatureIdLen,iRet);
                }
            }
                len_field = len_field - iRet;
            ptr = ptr + iRet;
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," Length Returned:%d for FeatureID:%d ptr:0x%08X\n",iRet,FeatureId,ptr);
        }
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," ########## out side the for loop ######## \n");
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","Pay Load Data: \n");
        for(ii = 0; ii < PayloadSize; ii++)
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," 0x%02X ", pDataSnd[ii]);
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","\n");            
        fetureSndApdu(sesnum,0x9F9808, PayloadSize, pDataSnd);
		rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pMem1);
		rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pMem2);
    }        
    
    

    return 1;
}

/*
 *    APDU in: 0x9F, 0x98, 0x08, "feature_parameters_cnf"
 */
int  APDU_FeatureParmCnfFromCard (USHORT sesnum, UCHAR * apkt, USHORT len)             
{
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," Entered  APDU_FeatureParmCnfFromCard ########\n");    
    return 0;    
}

extern "C" int GenFetClearPersistanceStorage()
{
  uint8 pData[1] = {3};
  //0x03 Reset both parental control and purchase PINs
  RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," Reseting both parental control and purchase PINs \n");
  set_reset_pin( pData, 1);
  return 0;
}
