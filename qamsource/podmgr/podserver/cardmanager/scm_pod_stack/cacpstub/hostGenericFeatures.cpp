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


#include "hostGenericFeatures.h"
#include "persistentconfig.h"
#include "cardUtils.h"
#include "core_events.h"
#include "rmf_osal_event.h"
#include "rmf_osal_sync.h"
#include "cmevents.h"
#include "cardUtils.h"

#include "utilityMacros.h"
#include "cardManagerIf.h"
#include "rmf_osal_mem.h"
#define __MTAG__ VL_CARD_MANAGER
#include <rdk_debug.h>
#include <string.h>
#include <stdlib.h>

#if USE_SYSRES_MLT
#include "rpl_new.h"
#include "rpl_malloc.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define VL_HOST_GENERIC_PARAMS_READ_INTEGER_CONFIG(struct, param, default)       struct->param = hostFeaturePersistentConfig.read("HOST_FEATURE_" #param, default)
#define VL_HOST_GENERIC_PARAMS_READ_ENUM_CONFIG(struct, type, param, default)    struct->param = (type)hostFeaturePersistentConfig.read("HOST_FEATURE_" #param, default)
#define VL_HOST_GENERIC_PARAMS_READ_STRING_CONFIG(struct, param)                 hostFeaturePersistentConfig.read("HOST_FEATURE_" #param, (char*)(struct->param), sizeof(struct->param))

#define VL_HOST_GENERIC_PARAMS_WRITE_INTEGER_CONFIG(struct, param)          hostFeaturePersistentConfig.write("HOST_FEATURE_" #param, struct->param)
#define VL_HOST_GENERIC_PARAMS_WRITE_TYPED_CONFIG(struct, type, param)      hostFeaturePersistentConfig.write("HOST_FEATURE_" #param, (type)(struct->param))
#define VL_HOST_GENERIC_PARAMS_WRITE_STRING_CONFIG( struct, param)          hostFeaturePersistentConfig.write("HOST_FEATURE_" #param, (char*)(struct->param))

#define VL_MAX_GENERIC_HOST_FEATURES    64
#define VL_POD_GET_STRUCT(datatype, pStruct, voidptr)    datatype * pStruct = ((datatype*)(voidptr))

static bool vlg_bConfigPathInitialized = false;

rmf_osal_Mutex vlhostParamThreadLock()
{
            
            static  bool mutex_initialized = 0;
                static rmf_osal_Mutex lock;
                if(mutex_initialized == 0)
                    {
                         mutex_initialized = 1;
			    rmf_osal_mutexNew(&lock);
                    }
//RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","\nvlhostParamThreadLock ##################### loack = %d ", lock);
                    return lock;
}

static bool vlInitHostFeatureParamConfigPath()
{  
    rmf_osal_mutexAcquire(vlhostParamThreadLock());
    {
        if(false == vlg_bConfigPathInitialized)
        {
            vlg_bConfigPathInitialized = true;
        }
    }
    rmf_osal_mutexRelease(vlhostParamThreadLock());
    
    return vlg_bConfigPathInitialized;
}

void vlHostGenFeatFreeMem(void *pData)
{
    if(pData)
    {
		rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pData);
    }
}
static int vlg_PostFeatureId;
void vlSendGenericFeatureChangedEventToPFC(VL_HOST_FEATURE_PARAM nFeatureId)
{
#ifdef USE_VLOCAP_STACK
    rmf_osal_event_handle_t event_handle;
    rmf_osal_event_params_t event_params = {0};
    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();

    //static VL_HOST_FEATURE_PARAM aFeatureParams[VL_MAX_GENERIC_HOST_FEATURES];
    if(nFeatureId < VL_MAX_GENERIC_HOST_FEATURES)
    {
    int *pFetureId;
	rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(int),(void **)&pFetureId);
    if(pFetureId == NULL)
        return;
    *pFetureId = nFeatureId;
        //aFeatureParams[nFeatureId] = nFeatureId;
    vlg_PostFeatureId = nFeatureId;
    event_params.priority = 0; //Default priority;
    event_params.data = (void *)pFetureId;
    event_params.data_extension = sizeof(int);
    event_params.free_payload_fn = vlHostGenFeatFreeMem;
    rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_CARD_MANAGER, CardMgr_Generic_Feature_Changed, 
		&event_params, &event_handle);

    rmf_osal_eventmanager_dispatch_event(em, event_handle);

    }
#else
    vlMpeosPostFeatureChangedEvent(nFeatureId);
    //Call the api to set the TimeZone.
    vlhal_stt_SetTimeZone(nFeatureId);
#endif
}
void vlSendGenericFeatureChangedEventToPOD(VL_HOST_FEATURE_PARAM nFeatureId)
{
    // Pass the Event to the POD   
    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    rmf_osal_event_handle_t event_handle;
    rmf_osal_event_params_t event_params = {0};
	
    LCSM_EVENT_INFO  eventInfo;
    memset(&eventInfo, 0 ,sizeof(LCSM_EVENT_INFO));
    eventInfo.event = POD_HOST_FEATURE_PARAM_CHANGE;
    eventInfo.x = nFeatureId;
    
    eventInfo.dataPtr     = NULL;
    eventInfo.dataLength = 0;
    if(nFeatureId < VL_MAX_GENERIC_HOST_FEATURES)
    {
        LCSM_EVENT_INFO *pEvent;
    
        rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(LCSM_EVENT_INFO),(void **)&pEvent);
    	
        memcpy(pEvent,(void*)&eventInfo, sizeof (LCSM_EVENT_INFO) );
    
        event_params.priority = 0; //Default priority;
        event_params.data = (void *)pEvent;
        event_params.data_extension = sizeof (LCSM_EVENT_INFO) ;
        event_params.free_payload_fn = podmgr_freefn;
        rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_GEN_FEATURE, eventInfo.event, 
    		&event_params, &event_handle);
        rmf_osal_eventmanager_dispatch_event(em, event_handle);
	
    }
}
VL_HOST_GENERIC_FEATURES_RESULT vlPodGetGenericFeatureListWithVer(unsigned char Ver,VL_POD_HOST_FEATURE_ID_LIST * pList)
{
    int nFeatures = 0;
    memset(pList,0,sizeof(*pList));
    if(Ver == 1 || Ver == 2)
    {
        pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_RF_Output_Channel        ;
        pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_Parental_Control_PIN     ;
        pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_Parental_Control_Settings;
        pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_IPPV_PIN                 ;
        pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_Time_Zone                ;
        pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_Daylight_Savings_Control ;
        pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_AC_Outlet                ;
        pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_Language                 ;
        pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_Rating_Region            ;
        pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_Reset_PINS               ;
        pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_Cable_URLS                ;
        pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_EAS_location_code        ;
    
    }
    else if( Ver == 3)
    {
        pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_RF_Output_Channel        ;
        pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_Parental_Control_PIN     ;
        pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_Parental_Control_Settings;
        pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_IPPV_PIN                 ;
        pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_Time_Zone                ;
        pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_Daylight_Savings_Control ;
        pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_AC_Outlet                ;
        pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_Language                 ;
            pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_Rating_Region            ;
            pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_Reset_PINS               ;
            pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_Cable_URLS                ;
            pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_EAS_location_code        ;
            pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_VCT_ID                   ;
            pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_Turn_on_Channel          ;
    
    }
    else if(Ver == 4)
    {    
        pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_RF_Output_Channel        ;
            pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_Parental_Control_PIN     ;
            pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_Parental_Control_Settings;
            pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_IPPV_PIN                 ;
            pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_Time_Zone                ;
            pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_Daylight_Savings_Control ;
            pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_AC_Outlet                ;
            pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_Language                 ;
            pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_Rating_Region            ;
            pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_Reset_PINS               ;
            pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_Cable_URLS                ;
            pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_EAS_location_code        ;
            pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_VCT_ID                   ;
            pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_Turn_on_Channel          ;
            pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_Terminal_Association     ;
            pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_Common_Download_Group_Id ;
            pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_Zip_Code                 ;
    
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlPodGetGenericFeatureListWithVer: Error Verions:%d \n",Ver);
    }
        pList->nNumberOfFeatures = nFeatures;
        return VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS;
}
    
VL_HOST_GENERIC_FEATURES_RESULT vlPodGetGenericFeatureList(VL_POD_HOST_FEATURE_ID_LIST * pList)
{
    int nFeatures = 0;
    memset(pList,0,sizeof(*pList));
    pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_RF_Output_Channel        ;
    pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_Parental_Control_PIN     ;
    pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_Parental_Control_Settings;
    pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_IPPV_PIN                 ;
    pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_Time_Zone                ;
    pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_Daylight_Savings_Control ;
    pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_AC_Outlet                ;
    pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_Language                 ;
    pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_Rating_Region            ;
    pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_Reset_PINS               ;
    pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_Cable_URLS                ;
    pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_EAS_location_code        ;
    pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_VCT_ID                   ;
    pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_Turn_on_Channel          ;
    pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_Terminal_Association     ;
    pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_Common_Download_Group_Id ;
    pList->aFeatureId[nFeatures++] = VL_HOST_FEATURE_PARAM_Zip_Code                 ;
    pList->nNumberOfFeatures = nFeatures;
    return VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS;
}

extern "C" const char * vlPodGetTimeZoneString()
{
    static char szTimeZone[256];
    CMpersistentConfig hostFeaturePersistentConfig( CMpersistentConfig::DYNAMIC, "/tmp", NULL  );
    rmf_osal_mutexAcquire(vlhostParamThreadLock());
    {
	memset(&szTimeZone,0,sizeof(szTimeZone));
        vlInitHostFeatureParamConfigPath();
        hostFeaturePersistentConfig.load_from_disk();
        hostFeaturePersistentConfig.read("timezone_string", szTimeZone, sizeof(szTimeZone));
    }
    rmf_osal_mutexRelease(vlhostParamThreadLock());
    return szTimeZone;
}
void vlPodSetTimeZoneString()
{
    if(NULL == getenv("TZ")) return;
    CMpersistentConfig hostFeaturePersistentConfig( CMpersistentConfig::DYNAMIC, "/tmp", NULL  );
    rmf_osal_mutexAcquire(vlhostParamThreadLock());
    {
        vlInitHostFeatureParamConfigPath();
        hostFeaturePersistentConfig.write("timezone_string", getenv("TZ"));
        hostFeaturePersistentConfig.save_to_disk();
    }
    rmf_osal_mutexRelease(vlhostParamThreadLock());
}
VL_HOST_GENERIC_FEATURES_RESULT vlPodGetGenericFeatureData(VL_HOST_FEATURE_PARAM eFeature, void * _pvStruct)
{
    int i = 0;
    char szFileName[256];
    CMpersistentConfig hostFeaturePersistentConfig( CMpersistentConfig::DYNAMIC, "/tmp", NULL  );
    rmf_osal_mutexAcquire(vlhostParamThreadLock());

    vlInitHostFeatureParamConfigPath();
  //  sprintf(szFileName, "/etc/hostgenericfeature_0x%X", eFeature);
  snprintf(szFileName,sizeof(szFileName), "/etc/hostgenericfeature_0x%X", eFeature);
    
    hostFeaturePersistentConfig.load_from_disk();
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","%s: loading generic feature data from %s\n", __FUNCTION__,  hostFeaturePersistentConfig.get_actual_path_name());

    switch(eFeature)
    {
        case VL_HOST_FEATURE_PARAM_RF_Output_Channel        :
        {
            VL_POD_GET_STRUCT(VL_POD_HOST_FEATURE_RF_CHANNEL, pStruct, _pvStruct);
	    memset(pStruct,0,sizeof(*pStruct));
            VL_HOST_GENERIC_PARAMS_READ_INTEGER_CONFIG(pStruct, nRfOutputChannel        , 3);
            VL_HOST_GENERIC_PARAMS_READ_INTEGER_CONFIG(pStruct, bEnableRfOutputChannelUI, 1);
        }
        break;

        case VL_HOST_FEATURE_PARAM_Parental_Control_PIN     :
        {
            char szParamName[256];
            VL_POD_GET_STRUCT(VL_POD_HOST_FEATURE_PARENTAL_CONTROL_PIN, pStruct, _pvStruct);
	    memset(pStruct,0,sizeof(*pStruct));
            VL_HOST_GENERIC_PARAMS_READ_INTEGER_CONFIG(pStruct, nParentalControlPinSize, 0);

#if 0
            if(pStruct->nParentalControlPinSize >= VL_HOST_FEATURE_MAX_PIN_SIZE)
         pStruct->nParentalControlPinSize = VL_HOST_FEATURE_MAX_PIN_SIZE;
#endif
            for(i = 0; i < pStruct->nParentalControlPinSize; i++)
            {
                snprintf(szParamName, sizeof(szParamName),"HOST_FEATURE_aParentalControlPin_%d", i);
                pStruct->aParentalControlPin[i] = (unsigned char)(hostFeaturePersistentConfig.read_hex(szParamName, 0));
            }
        }
        break;

        case VL_HOST_FEATURE_PARAM_Parental_Control_Settings:
        {
            char szParamName[256];
            VL_POD_GET_STRUCT(VL_POD_HOST_FEATURE_PARENTAL_CONTROL_SETTINGS, pStruct, _pvStruct);
	    memset(pStruct,0,sizeof(*pStruct));
            VL_HOST_GENERIC_PARAMS_READ_INTEGER_CONFIG(pStruct, ParentalControlReset, 0);
            VL_HOST_GENERIC_PARAMS_READ_INTEGER_CONFIG(pStruct, nParentalControlChannels, 0);
            if(pStruct->nParentalControlChannels >= VL_HOST_FEATURE_MAX_PC_CHANNELS) pStruct->nParentalControlChannels = VL_HOST_FEATURE_MAX_PC_CHANNELS;
            
            pStruct->paParentalControlChannels = VL_POD_ALLOC_ARRAY(unsigned long, pStruct->nParentalControlChannels);

            for(i = 0; i < pStruct->nParentalControlChannels; i++)
            {
                snprintf(szParamName,sizeof(szParamName), "HOST_FEATURE_ParentalControlChannel_%d", i);
                pStruct->paParentalControlChannels[i] = hostFeaturePersistentConfig.read_hex(szParamName, 0);
            }
        }
        break;

        case VL_HOST_FEATURE_PARAM_IPPV_PIN                 :
        {
            char szParamName[256];
            VL_POD_GET_STRUCT(VL_POD_HOST_FEATURE_PURCHASE_PIN, pStruct, _pvStruct);
	    memset(pStruct,0,sizeof(*pStruct));
            VL_HOST_GENERIC_PARAMS_READ_INTEGER_CONFIG(pStruct, nPurchasePinSize, 0);

#if 0
            if(pStruct->nPurchasePinSize >= VL_HOST_FEATURE_MAX_PIN_SIZE) pStruct->nPurchasePinSize = VL_HOST_FEATURE_MAX_PIN_SIZE;
#endif

            for(i = 0; i < pStruct->nPurchasePinSize; i++)
            {
                snprintf(szParamName,sizeof(szParamName), "HOST_FEATURE_aPurchasePin_%d", i);
                pStruct->aPurchasePin[i] = (unsigned char)(hostFeaturePersistentConfig.read_hex(szParamName, 0));
            }
        }
        break;

        case VL_HOST_FEATURE_PARAM_Time_Zone                :
        {
            VL_POD_GET_STRUCT(VL_POD_HOST_FEATURE_TIME_ZONE_OFFSET, pStruct, _pvStruct);
	    memset(pStruct,0,sizeof(*pStruct));
            VL_HOST_GENERIC_PARAMS_READ_INTEGER_CONFIG(pStruct, nTimeZoneOffset, -7);
        }
        break;

        case VL_HOST_FEATURE_PARAM_Daylight_Savings_Control :
        {
            VL_POD_GET_STRUCT(VL_POD_HOST_FEATURE_DAYLIGHT_SAVINGS, pStruct, _pvStruct);
	    memset(pStruct,0,sizeof(*pStruct));
            VL_HOST_GENERIC_PARAMS_READ_ENUM_CONFIG(pStruct, VL_POD_HOST_DAY_LIGHT_SAVINGS, eDaylightSavingsType        , VL_POD_HOST_DAY_LIGHT_SAVINGS_USE_DLS);
            if(VL_POD_HOST_DAY_LIGHT_SAVINGS_USE_DLS == pStruct->eDaylightSavingsType)
            {
                VL_HOST_GENERIC_PARAMS_READ_INTEGER_CONFIG(pStruct, nDaylightSavingsDeltaMinutes, (char)0);
                VL_HOST_GENERIC_PARAMS_READ_INTEGER_CONFIG(pStruct, tmDaylightSavingsEntryTime  , (long)0);
                VL_HOST_GENERIC_PARAMS_READ_INTEGER_CONFIG(pStruct, tmDaylightSavingsExitTime   , (long)0);
            }
        }
        break;

        case VL_HOST_FEATURE_PARAM_AC_Outlet                :
        {
            VL_POD_GET_STRUCT(VL_POD_HOST_FEATURE_AC_OUTLET, pStruct, _pvStruct);
	    memset(pStruct,0,sizeof(*pStruct));
            VL_HOST_GENERIC_PARAMS_READ_ENUM_CONFIG(pStruct, VL_POD_HOST_AC_OUTLET, eAcOutletSetting   , VL_POD_HOST_AC_OUTLET_UNSWITCHED);
        }
        break;

        case VL_HOST_FEATURE_PARAM_Language                 :
        {
            char szDefault[] = "eng";
            char szParamName[256];
            VL_POD_GET_STRUCT(VL_POD_HOST_FEATURE_LANGUAGE, pStruct, _pvStruct);
	    memset(pStruct,0,sizeof(*pStruct));
            VL_HOST_GENERIC_PARAMS_READ_INTEGER_CONFIG(pStruct, nLanguageControlChars, 0);
            // override the read value with hardcoded value
            pStruct->nLanguageControlChars = VL_HOST_FEATURE_LANG_CNTRL_CHARS;

            for(i = 0; i < pStruct->nLanguageControlChars; i++)
            {
                snprintf(szParamName,sizeof(szParamName), "HOST_FEATURE_szLanguageControl_%d", i);
                pStruct->szLanguageControl[i] = (unsigned char)(hostFeaturePersistentConfig.read_hex(szParamName, szDefault[i]));
            }
        }
        break;

        case VL_HOST_FEATURE_PARAM_Rating_Region            :
        {
            VL_POD_GET_STRUCT(VL_POD_HOST_FEATURE_RATING_REGION, pStruct, _pvStruct);
	    memset(pStruct,0,sizeof(*pStruct));
            VL_HOST_GENERIC_PARAMS_READ_ENUM_CONFIG(pStruct, VL_POD_HOST_RATING_REGION, eRatingRegion, VL_POD_HOST_RATING_REGION_US);
        }
        break;

        case VL_HOST_FEATURE_PARAM_Reset_PINS               :
        {
            VL_POD_GET_STRUCT(VL_POD_HOST_FEATURE_RESET_PINS, pStruct, _pvStruct);
	    memset(pStruct,0,sizeof(*pStruct));
            VL_HOST_GENERIC_PARAMS_READ_ENUM_CONFIG(pStruct, VL_POD_HOST_RESET_PIN, eResetPins, VL_POD_HOST_RESET_PIN_NONE);
        }
        break;

        case VL_HOST_FEATURE_PARAM_Cable_URLS               :
        {
            char szParamName[256];
            VL_POD_GET_STRUCT(VL_POD_HOST_FEATURE_CABLE_URLS, pStruct, _pvStruct);
	    memset(pStruct,0,sizeof(*pStruct));
            VL_HOST_GENERIC_PARAMS_READ_INTEGER_CONFIG(pStruct, nUrls, 0);
            if(pStruct->nUrls >= VL_HOST_FEATURE_MAX_CABLE_URLS) pStruct->nUrls = VL_HOST_FEATURE_MAX_CABLE_URLS;
            pStruct->paUrls = VL_POD_ALLOC_ARRAY(VL_POD_HOST_FEATURE_URL, pStruct->nUrls);

            for(i = 0; i < pStruct->nUrls; i++)
            {
                snprintf(szParamName, sizeof(szParamName),"HOST_FEATURE_Cable_URL_eUrlType_%d", i);
                pStruct->paUrls[i].eUrlType = (VL_POD_HOST_URL_TYPE) hostFeaturePersistentConfig.read(szParamName, VL_POD_HOST_URL_TYPE_UNDEFINED);
                snprintf(szParamName,sizeof(szParamName), "HOST_FEATURE_Cable_URL_nUrlLength_%d", i);
                pStruct->paUrls[i].nUrlLength = hostFeaturePersistentConfig.read(szParamName, 0);
                if(pStruct->paUrls[i].nUrlLength >= VL_HOST_FEATURE_MAX_STR_SIZE) pStruct->paUrls[i].nUrlLength = VL_HOST_FEATURE_MAX_STR_SIZE;
                snprintf(szParamName, sizeof(szParamName),"HOST_FEATURE_Cable_URL_szUrl_%d", i);
                hostFeaturePersistentConfig.read(szParamName, (char*)(pStruct->paUrls[i].szUrl));
                int iSafeIndex = SAFE_ARRAY_INDEX_UN(pStruct->paUrls[i].nUrlLength, pStruct->paUrls[i].szUrl);
                pStruct->paUrls[i].szUrl[iSafeIndex] = '\0';
            }
        }
        break;

        case VL_HOST_FEATURE_PARAM_EAS_location_code        :
        {
            VL_POD_GET_STRUCT(VL_POD_HOST_FEATURE_EAS, pStruct, _pvStruct);
            VL_HOST_GENERIC_PARAMS_READ_INTEGER_CONFIG(pStruct, EMStateCodel        , 0);
            VL_HOST_GENERIC_PARAMS_READ_INTEGER_CONFIG(pStruct, EMCSubdivisionCodel , 0);
            VL_HOST_GENERIC_PARAMS_READ_INTEGER_CONFIG(pStruct, EMCountyCode        , 0);
        }
        break;

        case VL_HOST_FEATURE_PARAM_VCT_ID:
        {
            VL_POD_GET_STRUCT(VL_POD_HOST_FEATURE_VCT_ID, pStruct, _pvStruct);
	    memset(pStruct,0,sizeof(*pStruct));
            VL_HOST_GENERIC_PARAMS_READ_INTEGER_CONFIG(pStruct, nVctId, 0);
        }
        break;

        case VL_HOST_FEATURE_PARAM_Turn_on_Channel          :
        {
            VL_POD_GET_STRUCT(VL_POD_HOST_FEATURE_TURN_ON_CHANNEL, pStruct, _pvStruct);
	    memset(pStruct,0,sizeof(*pStruct));
            VL_HOST_GENERIC_PARAMS_READ_INTEGER_CONFIG(pStruct, bTurnOnChannelDefined, 1);
            VL_HOST_GENERIC_PARAMS_READ_INTEGER_CONFIG(pStruct, iTurnOnChannel       , 3);
        }
        break;

        case VL_HOST_FEATURE_PARAM_Terminal_Association     :
        {
            VL_POD_GET_STRUCT(VL_POD_HOST_FEATURE_TERMINAL_ASSOCIATION, pStruct, _pvStruct);
	    memset(pStruct,0,sizeof(*pStruct));
            VL_HOST_GENERIC_PARAMS_READ_INTEGER_CONFIG(pStruct, nTerminalAssociationLength, 0);
            if(pStruct->nTerminalAssociationLength >= VL_HOST_FEATURE_MAX_STR_SIZE) pStruct->nTerminalAssociationLength = VL_HOST_FEATURE_MAX_STR_SIZE;
            strcpy((char*)(pStruct->szTerminalAssociationName), "");
            VL_HOST_GENERIC_PARAMS_READ_STRING_CONFIG(pStruct, szTerminalAssociationName);
            int iSafeIndex = SAFE_ARRAY_INDEX_UN(pStruct->nTerminalAssociationLength, pStruct->szTerminalAssociationName);
            pStruct->szTerminalAssociationName[iSafeIndex] = '\0';
        }
        break;

        case VL_HOST_FEATURE_PARAM_Common_Download_Group_Id:
        {
            VL_POD_GET_STRUCT(VL_POD_HOST_FEATURE_CDL_GROUP_ID, pStruct, _pvStruct);
	    memset(pStruct,0,sizeof(*pStruct));
            VL_HOST_GENERIC_PARAMS_READ_INTEGER_CONFIG(pStruct, common_download_group_id, 0x7073);
        }
        break;

        case VL_HOST_FEATURE_PARAM_Zip_Code                 :
        {
            VL_POD_GET_STRUCT(VL_POD_HOST_FEATURE_ZIP_CODE, pStruct, _pvStruct);
	    memset(pStruct,0,sizeof(*pStruct));
            VL_HOST_GENERIC_PARAMS_READ_INTEGER_CONFIG(pStruct, nZipCodeLength, 0);
            if(pStruct->nZipCodeLength >= VL_HOST_FEATURE_MAX_STR_SIZE) pStruct->nZipCodeLength = VL_HOST_FEATURE_MAX_STR_SIZE;
            strcpy((char*)(pStruct->szZipCode), "");
            VL_HOST_GENERIC_PARAMS_READ_STRING_CONFIG(pStruct, szZipCode);
            int iSafeIndex = SAFE_ARRAY_INDEX_UN(pStruct->nZipCodeLength, pStruct->szZipCode);
            pStruct->szZipCode[iSafeIndex] = '\0';
        }
        break;
    }

    rmf_osal_mutexRelease(vlhostParamThreadLock());
    return VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS;
}

VL_HOST_GENERIC_FEATURES_RESULT vlPodSetGenericFeatureData(VL_HOST_FEATURE_PARAM eFeature, void * _pvStruct, bool sendEventToJava)
{
    int i = 0;
    char szFileName[256];
    VL_HOST_GENERIC_FEATURES_RESULT ret = VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS;
    CMpersistentConfig hostFeaturePersistentConfig( CMpersistentConfig::DYNAMIC, "/tmp", NULL  );
    
    if(_pvStruct == NULL)
      return VL_HOST_GENERIC_FEATURES_RESULT_NULL_PARAM;
    
    rmf_osal_mutexAcquire(vlhostParamThreadLock());

    vlInitHostFeatureParamConfigPath();
    
    //sprintf(szFileName, "/etc/hostgenericfeature_0x%X", eFeature);
    snprintf(szFileName,sizeof(szFileName) ,"/etc/hostgenericfeature_0x%X", eFeature);
    
    hostFeaturePersistentConfig.load_from_disk();
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","%s: setting generic feature data to %s\n", __FUNCTION__, hostFeaturePersistentConfig.get_actual_path_name());

    switch(eFeature)
    {
        case VL_HOST_FEATURE_PARAM_RF_Output_Channel        :
        {

#if 0 
            pfcDisplayDriver *pDisplayDriver = NULL;
            pDisplayDriver = pfc_kernel_global->get_display_device();

            VL_POD_GET_STRUCT(VL_POD_HOST_FEATURE_RF_CHANNEL, pStruct, _pvStruct);
            //Set the RF Out Channel num
            if((NULL != pDisplayDriver) && (pDisplayDriver->setRfOutChannel(pStruct->nRfOutputChannel) != 0))
            {
                ret = VL_HOST_GENERIC_FEATURES_RESULT_INVALID;
                break;
            }

            VL_HOST_GENERIC_PARAMS_WRITE_INTEGER_CONFIG(pStruct, nRfOutputChannel        );
            VL_HOST_GENERIC_PARAMS_WRITE_INTEGER_CONFIG(pStruct, bEnableRfOutputChannelUI);
#endif			
        }
        break;

        case VL_HOST_FEATURE_PARAM_Parental_Control_PIN     :
        {
            char szParamName[256];
            VL_POD_GET_STRUCT(VL_POD_HOST_FEATURE_PARENTAL_CONTROL_PIN, pStruct, _pvStruct);

#if 0
            if(pStruct->nParentalControlPinSize >= VL_HOST_FEATURE_MAX_PIN_SIZE) pStruct->nParentalControlPinSize = VL_HOST_FEATURE_MAX_PIN_SIZE;
#endif

            VL_HOST_GENERIC_PARAMS_WRITE_INTEGER_CONFIG(pStruct, nParentalControlPinSize);

            for(i = 0; i < pStruct->nParentalControlPinSize; i++)
            {
                snprintf(szParamName,sizeof(szParamName), "HOST_FEATURE_aParentalControlPin_%d", i);
                hostFeaturePersistentConfig.write_hex(szParamName, pStruct->aParentalControlPin[i]);
            }
        }
        break;

        case VL_HOST_FEATURE_PARAM_Parental_Control_Settings:
        {
            char szParamName[256];
            VL_POD_GET_STRUCT(VL_POD_HOST_FEATURE_PARENTAL_CONTROL_SETTINGS, pStruct, _pvStruct);
            if(0xA7 == pStruct->ParentalControlReset)
            {
                pStruct->nParentalControlChannels = 0;
            }
            if(pStruct->nParentalControlChannels >= VL_HOST_FEATURE_MAX_PC_CHANNELS) pStruct->nParentalControlChannels = VL_HOST_FEATURE_MAX_PC_CHANNELS;
            
            VL_HOST_GENERIC_PARAMS_WRITE_INTEGER_CONFIG(pStruct, ParentalControlReset);
            VL_HOST_GENERIC_PARAMS_WRITE_INTEGER_CONFIG(pStruct, nParentalControlChannels);
            if(NULL != pStruct->paParentalControlChannels)
            {
                for(i = 0; i < pStruct->nParentalControlChannels; i++)
                {
                    snprintf(szParamName, sizeof(szParamName),"HOST_FEATURE_ParentalControlChannel_%d", i);
                    hostFeaturePersistentConfig.write_hex(szParamName, (long)(pStruct->paParentalControlChannels[i]));
                }
            }
        }
        break;

        case VL_HOST_FEATURE_PARAM_IPPV_PIN                 :
        {
            char szParamName[256];
            VL_POD_GET_STRUCT(VL_POD_HOST_FEATURE_PURCHASE_PIN, pStruct, _pvStruct);
#if 0
            if(pStruct->nPurchasePinSize >= VL_HOST_FEATURE_MAX_PIN_SIZE) pStruct->nPurchasePinSize = VL_HOST_FEATURE_MAX_PIN_SIZE;
#endif            
            VL_HOST_GENERIC_PARAMS_WRITE_INTEGER_CONFIG(pStruct, nPurchasePinSize);

            for(i = 0; i < pStruct->nPurchasePinSize; i++)
            {
                snprintf(szParamName,sizeof(szParamName), "HOST_FEATURE_aPurchasePin_%d", i);
                hostFeaturePersistentConfig.write_hex(szParamName, pStruct->aPurchasePin[i]);
            }
        }
        break;

        case VL_HOST_FEATURE_PARAM_Time_Zone                :
        {
            VL_POD_GET_STRUCT(VL_POD_HOST_FEATURE_TIME_ZONE_OFFSET, pStruct, _pvStruct);
            VL_HOST_GENERIC_PARAMS_WRITE_INTEGER_CONFIG(pStruct, nTimeZoneOffset);
        }
        break;

        case VL_HOST_FEATURE_PARAM_Daylight_Savings_Control :
        {
            VL_POD_GET_STRUCT(VL_POD_HOST_FEATURE_DAYLIGHT_SAVINGS, pStruct, _pvStruct);
            VL_HOST_GENERIC_PARAMS_WRITE_TYPED_CONFIG(pStruct, VL_POD_HOST_DAY_LIGHT_SAVINGS, eDaylightSavingsType        );
            VL_HOST_GENERIC_PARAMS_WRITE_TYPED_CONFIG(pStruct, char, nDaylightSavingsDeltaMinutes);
            VL_HOST_GENERIC_PARAMS_WRITE_TYPED_CONFIG(pStruct, long, tmDaylightSavingsEntryTime  );
            VL_HOST_GENERIC_PARAMS_WRITE_TYPED_CONFIG(pStruct, long, tmDaylightSavingsExitTime   );
        }
        break;

        case VL_HOST_FEATURE_PARAM_AC_Outlet                :
        {
            VL_POD_GET_STRUCT(VL_POD_HOST_FEATURE_AC_OUTLET, pStruct, _pvStruct);
            VL_HOST_GENERIC_PARAMS_WRITE_INTEGER_CONFIG(pStruct, eAcOutletSetting);
        }
        break;

        case VL_HOST_FEATURE_PARAM_Language                 :
        {
            char szParamName[256];
            VL_POD_GET_STRUCT(VL_POD_HOST_FEATURE_LANGUAGE, pStruct, _pvStruct);
            // override the given value with hardcoded value
            pStruct->nLanguageControlChars = VL_HOST_FEATURE_LANG_CNTRL_CHARS;
            VL_HOST_GENERIC_PARAMS_WRITE_INTEGER_CONFIG(pStruct, nLanguageControlChars);

            for(i = 0; i < pStruct->nLanguageControlChars; i++)
            {
                snprintf(szParamName,sizeof(szParamName),"HOST_FEATURE_szLanguageControl_%d", i);
                hostFeaturePersistentConfig.write_hex(szParamName, pStruct->szLanguageControl[i]);
            }
        }
        break;

        case VL_HOST_FEATURE_PARAM_Rating_Region            :
        {
            VL_POD_GET_STRUCT(VL_POD_HOST_FEATURE_RATING_REGION, pStruct, _pvStruct);
            VL_HOST_GENERIC_PARAMS_WRITE_INTEGER_CONFIG(pStruct, eRatingRegion);
        }
        break;

        case VL_HOST_FEATURE_PARAM_Reset_PINS               :
        {
            VL_POD_GET_STRUCT(VL_POD_HOST_FEATURE_RESET_PINS, pStruct, _pvStruct);
            VL_HOST_GENERIC_PARAMS_WRITE_INTEGER_CONFIG(pStruct, eResetPins);

            if(0 == sendEventToJava)
                break;
            switch(pStruct->eResetPins)
            {
                case VL_POD_HOST_RESET_PIN_PARENTAL_CONTROL:
                {
                    VL_POD_HOST_FEATURE_PARENTAL_CONTROL_PIN Struct;
		    memset(&Struct,0,sizeof(Struct));
                    vlPodSetGenericFeatureData(VL_HOST_FEATURE_PARAM_Parental_Control_PIN, &Struct, 0);
                }
                break;
                
                case VL_POD_HOST_RESET_PIN_PURCHASE:
                {
                    VL_POD_HOST_FEATURE_PURCHASE_PIN Struct;
		    memset(&Struct,0,sizeof(Struct));
                    vlPodSetGenericFeatureData(VL_HOST_FEATURE_PARAM_IPPV_PIN, &Struct, 0);
                }
                break;
                
                case VL_POD_HOST_RESET_PIN_PURCHASE_AND_PC:
                {
                    {
                        VL_POD_HOST_FEATURE_PARENTAL_CONTROL_PIN Struct;
			memset(&Struct,0,sizeof(Struct));
                        vlPodSetGenericFeatureData(VL_HOST_FEATURE_PARAM_Parental_Control_PIN, &Struct, 0);
                    }
                    {
                        VL_POD_HOST_FEATURE_PURCHASE_PIN Struct;
			memset(&Struct,0,sizeof(Struct));
                        vlPodSetGenericFeatureData(VL_HOST_FEATURE_PARAM_IPPV_PIN, &Struct, 0);
                    }
                }
                break;
                
                default:
                {

                }
                break;
            }
        }
        break;

        case VL_HOST_FEATURE_PARAM_Cable_URLS               :
        {
            char szParamName[256];
            VL_POD_GET_STRUCT(VL_POD_HOST_FEATURE_CABLE_URLS, pStruct, _pvStruct);
            if(pStruct->nUrls >= VL_HOST_FEATURE_MAX_CABLE_URLS) pStruct->nUrls = VL_HOST_FEATURE_MAX_CABLE_URLS;
            
            VL_HOST_GENERIC_PARAMS_WRITE_INTEGER_CONFIG(pStruct, nUrls);
//RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","\n VL_HOST_FEATURE_PARAM_Cable_URLS pStruct->nUrls =%d", pStruct->nUrls);
            for(i = 0; i < pStruct->nUrls; i++)
            {
                int iSafeIndex = SAFE_ARRAY_INDEX_UN(pStruct->paUrls[i].nUrlLength, pStruct->paUrls[i].szUrl);
                pStruct->paUrls[i].szUrl[iSafeIndex] = '\0';
                snprintf(szParamName,sizeof(szParamName), "HOST_FEATURE_Cable_URL_eUrlType_%d", i);
                hostFeaturePersistentConfig.write(szParamName, (int)(pStruct->paUrls[i].eUrlType));
//RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","\n VL_HOST_FEATURE_PARAM_Cable_URLS pStruct->paUrls[i].eUrlType =%d", pStruct->paUrls[i].eUrlType);
                snprintf(szParamName, sizeof(szParamName),"HOST_FEATURE_Cable_URL_nUrlLength_%d", i);
                if(pStruct->paUrls[i].nUrlLength >= VL_HOST_FEATURE_MAX_STR_SIZE) pStruct->paUrls[i].nUrlLength = VL_HOST_FEATURE_MAX_STR_SIZE;
                hostFeaturePersistentConfig.write(szParamName, (int)(pStruct->paUrls[i].nUrlLength));
//RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","\n VL_HOST_FEATURE_PARAM_Cable_URLS pStruct->paUrls[i].nUrlLength =%d", pStruct->paUrls[i].nUrlLength);
                snprintf(szParamName,sizeof(szParamName), "HOST_FEATURE_Cable_URL_szUrl_%d", i);
                hostFeaturePersistentConfig.write(szParamName, (char*)(pStruct->paUrls[i].szUrl));
            }
        }
        break;

        case VL_HOST_FEATURE_PARAM_EAS_location_code        :
        {
            VL_POD_GET_STRUCT(VL_POD_HOST_FEATURE_EAS, pStruct, _pvStruct);
            VL_HOST_GENERIC_PARAMS_WRITE_TYPED_CONFIG(pStruct, int, EMStateCodel       );
            VL_HOST_GENERIC_PARAMS_WRITE_TYPED_CONFIG(pStruct, int, EMCSubdivisionCodel);
            VL_HOST_GENERIC_PARAMS_WRITE_TYPED_CONFIG(pStruct, int, EMCountyCode       );
        }
        break;

        case VL_HOST_FEATURE_PARAM_VCT_ID:
        {
            VL_POD_GET_STRUCT(VL_POD_HOST_FEATURE_VCT_ID, pStruct, _pvStruct);
            VL_HOST_GENERIC_PARAMS_WRITE_TYPED_CONFIG(pStruct, short, nVctId);
        }
        break;

        case VL_HOST_FEATURE_PARAM_Turn_on_Channel          :
        {
            VL_POD_GET_STRUCT(VL_POD_HOST_FEATURE_TURN_ON_CHANNEL, pStruct, _pvStruct);
            VL_HOST_GENERIC_PARAMS_WRITE_INTEGER_CONFIG(pStruct, bTurnOnChannelDefined);
            VL_HOST_GENERIC_PARAMS_WRITE_INTEGER_CONFIG(pStruct, iTurnOnChannel       );
        }
        break;

        case VL_HOST_FEATURE_PARAM_Terminal_Association     :
        {
            VL_POD_GET_STRUCT(VL_POD_HOST_FEATURE_TERMINAL_ASSOCIATION, pStruct, _pvStruct);

//            if(NULL != pStruct->szTerminalAssociationName)
            {
                int iSafeIndex = SAFE_ARRAY_INDEX_UN(pStruct->nTerminalAssociationLength, pStruct->szTerminalAssociationName);
                pStruct->szTerminalAssociationName[iSafeIndex] = '\0';
                if(pStruct->nTerminalAssociationLength >= VL_HOST_FEATURE_MAX_STR_SIZE) pStruct->nTerminalAssociationLength = VL_HOST_FEATURE_MAX_STR_SIZE;
                VL_HOST_GENERIC_PARAMS_WRITE_INTEGER_CONFIG(pStruct, nTerminalAssociationLength);
                VL_HOST_GENERIC_PARAMS_WRITE_STRING_CONFIG(pStruct, szTerminalAssociationName);
            }
#if 0
            else

            {
                pStruct->nTerminalAssociationLength = 0;
                VL_HOST_GENERIC_PARAMS_WRITE_INTEGER_CONFIG(pStruct, nTerminalAssociationLength);
            }
#endif
        }
        break;

        case VL_HOST_FEATURE_PARAM_Common_Download_Group_Id:
        {
            VL_POD_GET_STRUCT(VL_POD_HOST_FEATURE_CDL_GROUP_ID, pStruct, _pvStruct);
            VL_HOST_GENERIC_PARAMS_WRITE_INTEGER_CONFIG(pStruct, common_download_group_id);
        }
        break;

        case VL_HOST_FEATURE_PARAM_Zip_Code                 :
        {
            VL_POD_GET_STRUCT(VL_POD_HOST_FEATURE_ZIP_CODE, pStruct, _pvStruct);
//            if(NULL != pStruct->szZipCode)
            {
                int iSafeIndex = SAFE_ARRAY_INDEX_UN(pStruct->nZipCodeLength, pStruct->szZipCode);
                pStruct->szZipCode[iSafeIndex] = '\0';
                if(pStruct->nZipCodeLength >= VL_HOST_FEATURE_MAX_STR_SIZE) pStruct->nZipCodeLength = VL_HOST_FEATURE_MAX_STR_SIZE;
                VL_HOST_GENERIC_PARAMS_WRITE_INTEGER_CONFIG(pStruct, nZipCodeLength);
                VL_HOST_GENERIC_PARAMS_WRITE_STRING_CONFIG(pStruct, szZipCode);
            }
#if 0
            else
            {
                pStruct->nZipCodeLength = 0;
                VL_HOST_GENERIC_PARAMS_WRITE_INTEGER_CONFIG(pStruct, nZipCodeLength);
            }
#endif
        }
        break;
    }

    hostFeaturePersistentConfig.save_to_disk();
    rmf_osal_mutexRelease(vlhostParamThreadLock());
    //Event to Java has to be posted only if vlPodSetGenericFeatureData is called from card manager('s apdu_feature.c)
    if(sendEventToJava)
        vlSendGenericFeatureChangedEventToPFC(eFeature);
    //Removed from here as Java will post this event.
    //vlSendGenericFeatureChangedEventToPOD(eFeature);

    
    return ret;
}

VL_HOST_GENERIC_FEATURES_RESULT vlPodGetGenericFeatureHexData(VL_HOST_FEATURE_PARAM eFeature, unsigned char * pData, int * pnBytes, int nByteCapacity)
{
    int i = 0;
    char szFileName[256];
    CMpersistentConfig hostFeaturePersistentConfig( CMpersistentConfig::DYNAMIC, "/tmp", NULL  );
    
    rmf_osal_mutexAcquire(vlhostParamThreadLock());
    {
        vlInitHostFeatureParamConfigPath();
  //      sprintf(szFileName, "/etc/hostgenericfeature_0x%X", eFeature);
        snprintf(szFileName, sizeof(szFileName),"/etc/hostgenericfeature_0x%X", eFeature);
        
        hostFeaturePersistentConfig.load_from_disk();
        //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: loading generic feature data from %s\n", __FUNCTION__,  hostFeaturePersistentConfig.get_actual_path_name());

        char szParamName[256];
        int nBytes = hostFeaturePersistentConfig.read("HOST_FEATURE_HexDataLength", 0);
        if(nBytes >= nByteCapacity)
            nBytes = nByteCapacity;
        if(nBytes >= VL_HOST_FEATURE_MAX_FEATURE_DATA)
            nBytes = VL_HOST_FEATURE_MAX_FEATURE_DATA;
        if(nBytes < 0)
            nBytes = 0;
        
        if(NULL != pnBytes)
        {
            *pnBytes = nBytes;
        }

        if(NULL != pData)
        {
            for(i = 0; (i < nBytes) && (i < nByteCapacity); i++)
            {
                snprintf(szParamName,sizeof(szParamName), "HOST_FEATURE_HexData_%d", i);
                pData[i] = (unsigned char)(hostFeaturePersistentConfig.read_hex(szParamName, 0));
            }
        }
    }
    rmf_osal_mutexRelease(vlhostParamThreadLock());
    
    return VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS;
}

VL_HOST_GENERIC_FEATURES_RESULT vlPodSetGenericFeatureHexData(VL_HOST_FEATURE_PARAM eFeature, const unsigned char * pData, int nBytes, bool sendEventToJava)
{
    int i = 0;
    char szFileName[256];
    CMpersistentConfig hostFeaturePersistentConfig( CMpersistentConfig::DYNAMIC, "/tmp", NULL  );
    
    if(NULL == pData) return VL_HOST_GENERIC_FEATURES_RESULT_NULL_PARAM;
    
    rmf_osal_mutexAcquire(vlhostParamThreadLock());
    {
        vlInitHostFeatureParamConfigPath();
 //       sprintf(szFileName, "/etc/hostgenericfeature_0x%X", eFeature);
       snprintf(szFileName,sizeof(szFileName), "/etc/hostgenericfeature_0x%X", eFeature);
        
        hostFeaturePersistentConfig.load_from_disk();
        //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: setting generic feature data to %s\n", __FUNCTION__, hostFeaturePersistentConfig.get_actual_path_name());
    
        char szParamName[256];
        if(nBytes >= VL_HOST_FEATURE_MAX_FEATURE_DATA)
            nBytes = VL_HOST_FEATURE_MAX_FEATURE_DATA;
        if(nBytes < 0)
            nBytes = 0;

        hostFeaturePersistentConfig.write("HOST_FEATURE_HexDataLength", nBytes);
        
        if(NULL != pData)
        {
            for(i = 0; i < nBytes; i++)
            {
                snprintf(szParamName,sizeof(szParamName), "HOST_FEATURE_HexData_%d", i);
                hostFeaturePersistentConfig.write_hex(szParamName, pData[i] );
            }
        }
    
        hostFeaturePersistentConfig.save_to_disk();
    }
    rmf_osal_mutexRelease(vlhostParamThreadLock());
    //Event to Java has to be posted only if vlPodSetGenericFeatureData is called from card manager('s apdu_feature.c)
    if(sendEventToJava)
        vlSendGenericFeatureChangedEventToPFC(eFeature);
    return VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS;
}

#ifdef __cplusplus
}
#endif
