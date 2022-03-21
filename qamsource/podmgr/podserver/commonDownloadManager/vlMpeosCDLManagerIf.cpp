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
#include <stdlib.h>
#include "sys_init.h"
#include "vlMpeosCDLManagerIf.h"
#include "utilityMacros.h"
#include "bufParseUtilities.h"
#include "CommonDownloadMgr.h"
#include "ccodeimagevalidator.h"
#include <rdk_debug.h>
#include "vlMpeosCdlIf.h"
#include "vl_cdl_types.h"
#ifndef SNMP_IPC_CLIENT
#include "vlSnmpHostInfo.h"
#endif //SNMP_IPC_CLIENT
#include "vlpluginapp_halcdlapi.h"
#include "rmf_osal_thread.h"
#include "rmf_osal_mem.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif

#include "libIBus.h"
#include "sysMgr.h"


static CommonDownloadManager        * vlg_pCDLManager     = NULL;
extern CommonDownloadState_e eCDLM_State; // state of the commondownload manager...
static bool vlg_cdlCodeImageValidationCompleted = false;
extern "C" void mpeos_dbgSetLogLevelString(const char* pszModuleName, const char* pszLogLevels);

//#define VL_CDL_UNIT_TEST

#ifdef VL_CDL_UNIT_TEST
void vl_cdl_start_tester();
#endif

rmf_Error vlMpeosCDLManagerInit()
{
    rmf_Error result = RMF_SUCCESS;
    static int bInitialized = 0;
    if(0 == bInitialized)
    {
        bInitialized = 1;
       
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," >>>>>>>>>> calling vlMpeosCoreSetCDLDevice \n");
      
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," >>>>>>>>>> After calling vlMpeosCoreSetCDLDevice \n");

        if(NULL == vlg_pCDLManager)
        {
            vlg_pCDLManager = new CommonDownloadManager;
            if(NULL != vlg_pCDLManager)
            {
                vlg_pCDLManager->initialize();
               
               
            }
        }

        if(NULL == vlg_pCDLManager)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","%s: vlg_pCDLManager alloc failed\n", __FUNCTION__);
            return RMF_OSAL_ENOMEM;
        }

      
        #ifdef VL_CDL_UNIT_TEST
        vl_cdl_start_tester();
        #endif
       
    }

    return result;
}

VLMPEOS_CDL_RESULT vlMpeosCdlGetCurrentBootImageName(VLMPEOS_CDL_SOFTWARE_IMAGE_NAME * pImageName)
{
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","%s: Called.\n", __FUNCTION__);
    if(NULL == pImageName)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","%s : pImageName = NULL\n", __FUNCTION__);
        return VLMPEOS_CDL_RESULT_NULL_PARAM;
    }
#ifndef SNMP_IPC_CLIENT
    VL_SNMP_VAR_DEVICE_SOFTWARE Softwareobj;
    memset(&(*pImageName),0,sizeof(*pImageName));
    memset(&(Softwareobj),0,sizeof(Softwareobj));
    if(VL_HOST_SNMP_API_RESULT_SUCCESS == vlSnmpGetHostInfo(VL_SNMP_VAR_TYPE_ocStbHostDeviceSoftware, &Softwareobj))
    {
        memset(pImageName->szSoftwareImageName, 0, sizeof(pImageName->szSoftwareImageName))
        strncpy(pImageName->szSoftwareImageName, Softwareobj.SoftwareFirmwareImageName, sizeof(pImageName->szSoftwareImageName)-1);
        return VLMPEOS_CDL_RESULT_SUCCESS;
    }
#endif //SNMP_IPC_CLIENT    
    return VLMPEOS_CDL_RESULT_NOT_IMPLEMENTED;
}

VLMPEOS_CDL_RESULT vlMpeosCdlGetStatus(VLMPEOS_CDL_STATUS_QUESTION * pStatusQuestion)
{
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","%s: Called.\n", __FUNCTION__);
    if(NULL == pStatusQuestion)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","%s: pStatusQuestion = NULL\n", __FUNCTION__);
        return VLMPEOS_CDL_RESULT_NULL_PARAM;
    }
    
    VL_CDL_DOWNLOAD_STATUS_QUESTION downloadStatus;
    memset(&(*pStatusQuestion),0,sizeof(*pStatusQuestion));
    memset(&(downloadStatus),0,sizeof(downloadStatus));
    int iRet  = CHALCdl_notify_cdl_mgr_event(VL_CDL_MANAGER_EVENT_get_UPGRADE_STATUS,(unsigned long) &downloadStatus);
    if(VL_CDL_RESULT_SUCCESS != iRet)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","%s:  VL_CDL_MANAGER_EVENT_get_UPGRADE_STATUS returned Error:0x%X\n", __FUNCTION__, iRet);
        return ((VLMPEOS_CDL_RESULT)iRet);
    }
    
    if(VL_CDL_DOWNLOAD_STATUS_IDLE == downloadStatus.eDownloadStatus)
    {
        if(COM_DOWNL_IDLE != eCDLM_State)
        {
            downloadStatus.eDownloadStatus = VL_CDL_DOWNLOAD_STATUS_MANAGER_DOWNLOADING;
        }
    }
    
    pStatusQuestion->eDownloadStatus = ((VLMPEOS_CDL_DOWNLOAD_STATUS)(downloadStatus.eDownloadStatus));
    
    return VLMPEOS_CDL_RESULT_SUCCESS;
}

void  vl_cdl_img_validation_mon_task(void * arg)
{
    while(!vlg_cdlCodeImageValidationCompleted)
    {
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","%s: Waiting for image validation to complete...\n", __FUNCTION__);
        rmf_osal_threadSleep(1000, 0);
    }
}

VLMPEOS_CDL_RESULT vlMpeosCdlUpgradeToImageAndReboot(VLMPEOS_CDL_IMAGE * pImage, int bRebootNow)
{
    int ret = 0;
    IARM_Bus_SYSMgr_EventData_t eventData;

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","%s: Called.\n", __FUNCTION__);
    if(NULL == pImage)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","%s: pImage = NULL\n", __FUNCTION__);
        return VLMPEOS_CDL_RESULT_NULL_PARAM;
    }
    
    if(NULL == pImage->pszImagePathName)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","%s: pImage->pszImagePathName = NULL\n", __FUNCTION__);
        return VLMPEOS_CDL_RESULT_INVALID_PARAM;
    }
    
    if(0 == strlen(pImage->szSoftwareImageName))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","%s: length of pImage->szSoftwareImageName is zero\n", __FUNCTION__);
        return VLMPEOS_CDL_RESULT_INVALID_PARAM;
    }
    
    if(
        (VLMPEOS_CDL_IMAGE_SIGNED != pImage->eImageSign)
            &&
        (VLMPEOS_CDL_IMAGE_UNSIGNED != pImage->eImageSign)
      )
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","%s: Image sign %d not supported.\n", __FUNCTION__, pImage->eImageSign);
        VL_CDL_RegisterCdlState("RCDL Rejected, Invalid image sign");
        return VLMPEOS_CDL_RESULT_NOT_IMPLEMENTED;
    }
    
    {
        struct stat st;
        memset( &st, 0, sizeof( st ) );
        
        if(0 == stat(pImage->pszImagePathName, &st))
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL", "%s() : File Size of '%s' = %d bytes.\n", __FUNCTION__, pImage->pszImagePathName, (int)st.st_size);
            if(st.st_size <= 0)
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL", "%s() : ERROR: File Size of '%s' = %d bytes.\n", __FUNCTION__, pImage->pszImagePathName, st.st_size);
                VL_CDL_RegisterCdlState("RCDL Rejected, Zero size file");
                return VLMPEOS_CDL_RESULT_INCOMPLETE_PARAM;
            }
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL", "%s() : ERROR: Cannot stat '%s'.\n", __FUNCTION__, pImage->pszImagePathName);
            VL_CDL_RegisterCdlState("RCDL Rejected, File not found");
            return VLMPEOS_CDL_RESULT_INCOMPLETE_PARAM;
        }
    }
    
    if(VLMPEOS_CDL_IMAGE_SIGNED == pImage->eImageSign)
    {
        CCodeImageValidator imageValidator;
        int nRet = 0;
        vlg_cdlCodeImageValidationCompleted = false;
	 rmf_osal_ThreadId tid;
	 rmf_osal_threadCreate(vl_cdl_img_validation_mon_task, NULL, RMF_OSAL_THREAD_PRIOR_DFLT, RMF_OSAL_THREAD_STACK_SIZE, &tid, "vl_cdl_img_validation_mon_task");
        
        //mpeos_dbgSetLogLevelString("LOG.MPE.THREAD", "DEBUG");
        nRet = imageValidator.processCodeImage(pImage->pszImagePathName, strlen(pImage->pszImagePathName), COMM_DL_MONOLITHIC_IMAGE, false);
        if(0 != nRet)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","%s:  processCodeImage returned error : %d\n", __FUNCTION__, nRet);
            vlg_cdlCodeImageValidationCompleted = true;
            VL_CDL_RegisterCdlState("RCDL Rejected, Image Verification Failed");
            return VLMPEOS_CDL_RESULT_IMAGE_VERIFICATION_FAILED;
        }
        else
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","%s: Image '%s' (%s) passed integrity check.\n", __FUNCTION__, pImage->szSoftwareImageName, pImage->pszImagePathName);
        }
        vlg_cdlCodeImageValidationCompleted = true;
    }
    
    VL_CDL_DOWNLOAD_STATUS_QUESTION downloadStatus;
    memset(&downloadStatus,0,sizeof(downloadStatus));
    int iRet  = CHALCdl_notify_cdl_mgr_event(VL_CDL_MANAGER_EVENT_get_UPGRADE_STATUS,(unsigned long) &downloadStatus);
    if(VL_CDL_RESULT_SUCCESS != iRet)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","%s:  VL_CDL_MANAGER_EVENT_get_UPGRADE_STATUS returned Error:0x%X\n", __FUNCTION__, iRet);
    }
    
    if((VL_CDL_DOWNLOAD_STATUS_IDLE != downloadStatus.eDownloadStatus) &&
       (VL_CDL_DOWNLOAD_STATUS_BOOTUP_AFTER_SUCCESSFUL_UPGRADE != downloadStatus.eDownloadStatus) &&
       (VL_CDL_DOWNLOAD_STATUS_BOOTUP_AFTER_FAILED_UPGRADE != downloadStatus.eDownloadStatus))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","%s: CDL Driver is busy, status = %d.\n", __FUNCTION__, downloadStatus.eDownloadStatus);
        VL_CDL_RegisterCdlState("RCDL Rejected, Driver Busy");
        return VLMPEOS_CDL_RESULT_MODULE_BUSY;
    }
    
    if(COM_DOWNL_IDLE != eCDLM_State)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","%s: CDL Manager is busy, status = %d.\n", __FUNCTION__, eCDLM_State);
        VL_CDL_RegisterCdlState("RCDL Rejected, Manager Busy");
        return VLMPEOS_CDL_RESULT_MODULE_BUSY;
    }

    VL_CDL_IMAGE ImageNamePath;
    memset(&ImageNamePath,0,sizeof(ImageNamePath));
    ImageNamePath.eTriggerType = VL_CDL_TRIGGER_TYPE_STB_CVT;
    ImageNamePath.eImageType   = (VL_CDL_IMAGE_TYPE  )(pImage->eImageType  );
    ImageNamePath.eImageSign   = (VL_CDL_IMAGE_SIGN  )(pImage->eImageSign  );
    memset(ImageNamePath.szSoftwareImageName, 0, sizeof(ImageNamePath.szSoftwareImageName));
    strncpy(ImageNamePath.szSoftwareImageName, pImage->szSoftwareImageName, sizeof(ImageNamePath.szSoftwareImageName)-1); 
    ImageNamePath.pszImagePathName = pImage->pszImagePathName;
    
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," #######################  COM_DOWNL_START_UPGRADE  ################# \n");
    eCDLM_State = COM_DOWNL_CODE_IMAGE_UPDATE;
    VL_CDL_RegisterCdlState("RCDL Upgrade Started");

    eventData.data.systemStates.state = IARM_BUS_SYSMGR_IMAGE_FWDNLD_FLASH_INPROGRESS;
    eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_FIRMWARE_DWNLD;
    eventData.data.systemStates.payload[0] = 0;
    IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData)); 

    iRet = CHALCdl_notify_cdl_mgr_event(VL_CDL_MANAGER_EVENT_UPGRADE_TO_IMAGE,(unsigned long) &ImageNamePath);
    if(VL_CDL_RESULT_SUCCESS != iRet )
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","%s: VL_CDL_MANAGER_EVENT_UPGRADE_TO_IMAGE failed with Error:0x%X \n", __FUNCTION__, iRet);
        eCDLM_State = COM_DOWNL_IDLE;
        VL_CDL_RegisterCdlState("RCDL Upgrade Failed");

        eventData.data.systemStates.state = IARM_BUS_SYSMGR_IMAGE_FWDNLD_FLASH_FAILED;
        eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_FIRMWARE_DWNLD;
        eventData.data.systemStates.payload[0] = 0;
        IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));

        return VLMPEOS_CDL_RESULT_UPGRADE_FAILED;
    }
    
#if !defined(VL_CDL_UNIT_TEST)
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","%s: Deleting file after file upgrade is complete FileName:%s ", __FUNCTION__, ImageNamePath.pszImagePathName);
    ret = remove(ImageNamePath.pszImagePathName);
    VL_CDL_RegisterFlashedFile(ImageNamePath.szSoftwareImageName);
    VL_CDL_RegisterCdlState("RCDL Upgrade Succeeded");
    if( ret !=0  )        
    {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SNMP","%s() : Could not remove file \n", __FUNCTION__);
    }  
#endif 
    system("sync");
    VL_CDL_RegisterCdlState("Preparing to reboot");
  
    eventData.data.systemStates.state = IARM_BUS_SYSMGR_IMAGE_FWDNLD_FLASH_COMPLETE;
    eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_FIRMWARE_DWNLD;
    eventData.data.systemStates.payload[0] = 0;
    IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));

    if(bRebootNow)
    { 
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," #######################  COM_DOWNL_START_REBOOT  ################# \n");
	eCDLM_State = COM_DOWNL_START_REBOOT;
	iRet = CHALCdl_notify_cdl_mgr_event(VL_CDL_MANAGER_EVENT_REBOOT, 0);
	if(VL_CDL_RESULT_SUCCESS != iRet )
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL", "%s: VL_CDL_MANAGER_EVENT_REBOOT Failed with Error:0x%X \n", __FUNCTION__, iRet);
		eCDLM_State = COM_DOWNL_IDLE;
		return VLMPEOS_CDL_RESULT_UPGRADE_FAILED;
	}
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL", "%s: Not rebooting as DEFERRED reboot was requested.\n", __FUNCTION__);
    }
    RDK_LOG(RDK_LOG_INFO, "RDK.MOD.CDL"," #######################  COM_DOWNL_UPGRADE_FINISHED  ################# \n");
    eCDLM_State = COM_DOWNL_IDLE;
    return VLMPEOS_CDL_RESULT_SUCCESS;
}

//#ifdef VL_CDL_UNIT_TEST
typedef struct _VLMPEOS_CDL_UPGRADE_PARAMS
{
    VLMPEOS_CDL_IMAGE_TYPE  eImageType;
    VLMPEOS_CDL_IMAGE_SIGN  eImageSign;
    int                     bRebootNow;
    const char *            pszImagePathName;
    
}VLMPEOS_CDL_UPGRADE_PARAMS;

static VLMPEOS_CDL_UPGRADE_PARAMS vlg_cdlUpgradeParams;

void vlMpeosCdlUpgradeToImageTask(void * arg)
{
    VLMPEOS_CDL_UPGRADE_PARAMS * pCdlParams = (VLMPEOS_CDL_UPGRADE_PARAMS*)(arg);
    const char * pszImageName = NULL;
    
    VLMPEOS_CDL_SOFTWARE_IMAGE_NAME imageName;
    vlMpeosCdlGetCurrentBootImageName(&imageName);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","%s:Current imageName = %s\n", __FUNCTION__, imageName.szSoftwareImageName);
    VLMPEOS_CDL_STATUS_QUESTION statusQuestion;
    vlMpeosCdlGetStatus(&statusQuestion);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","%s:cdlStatus = %d\n", __FUNCTION__, statusQuestion.eDownloadStatus);
    
    VLMPEOS_CDL_IMAGE image;
    memset(&image,0,sizeof(image));
    
    image.eImageType = pCdlParams->eImageType;
    image.eImageSign = pCdlParams->eImageSign;
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","%s:%d, pCdlParams->pszImagePathName: %s\n", __FUNCTION__, __LINE__, pCdlParams->pszImagePathName);
    image.pszImagePathName = (char*)pCdlParams->pszImagePathName;
    
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","%s:New imageName = %s\n", __FUNCTION__, image.pszImagePathName);
    pszImageName = strrchr(image.pszImagePathName, '/');
    if(NULL != pszImageName)
    {
        pszImageName += 1;
    }
    else
    {
        pszImageName = image.pszImagePathName;
    }
    
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","%s:pszImageName = %s\n", __FUNCTION__, pszImageName);
    memset(image.szSoftwareImageName, 0, sizeof(image.szSoftwareImageName));
    strncpy(image.szSoftwareImageName, pszImageName, sizeof(image.szSoftwareImageName) - 1);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","%s:image.szImageName = %s\n", __FUNCTION__, image.szSoftwareImageName);
    
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","%s:%d\n", __FUNCTION__, __LINE__);
    //eCDLM_State = COM_DOWNL_START_REBOOT;
    VLMPEOS_CDL_RESULT eUpgradeResult = vlMpeosCdlUpgradeToImageAndReboot(&image, pCdlParams->bRebootNow);
    if(VLMPEOS_CDL_RESULT_SUCCESS != eUpgradeResult)
    {
        VL_CDL_RCDL_IndicationFile();
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL", "%s: vlMpeosCdlUpgradeToImageAndReboot returned error %d.\n", __FUNCTION__, eUpgradeResult);
    }
    VL_CDL_RCDL_IndicationFile();


   rmf_osal_memFreeP(RMF_OSAL_MEM_POD, (void*)pCdlParams->pszImagePathName);
}

extern "C" void vlMpeosCdlUpgradeToImage(VLMPEOS_CDL_IMAGE_TYPE eImageType, VLMPEOS_CDL_IMAGE_SIGN eImageSign, int bRebootNow, const char * pszImagePathName)
{
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","%s:%d, Image name to be upgraded to: %s\n", __FUNCTION__, __LINE__, pszImagePathName);
    rmf_osal_ThreadId threadId = 0;
    vlg_cdlUpgradeParams.eImageType         = eImageType;
    vlg_cdlUpgradeParams.eImageSign         = eImageSign;
    vlg_cdlUpgradeParams.bRebootNow         = bRebootNow;
    vlg_cdlUpgradeParams.pszImagePathName   = pszImagePathName;
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","%s:%d, vlg_cdlUpgradeParams->pszImagePathName: %s\n", __FUNCTION__, __LINE__, vlg_cdlUpgradeParams.pszImagePathName);
    rmf_osal_threadCreate(vlMpeosCdlUpgradeToImageTask, (void *)&vlg_cdlUpgradeParams, RMF_OSAL_THREAD_PRIOR_DFLT, RMF_OSAL_THREAD_STACK_SIZE, &threadId, "vl_cdl_if_test_thread");
}
//#endif // VL_CDL_UNIT_TEST
