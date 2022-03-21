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


#include "CommonDownloadMgr.h"
#include "core_events.h"
#include "cardUtils.h"
#include "cmevents.h"
#include "pkcs7utils.h"
#include "tablecvt.h"
#include "DownloadEngine.h"
#include "CommonDownloadImageUpdate.h"
#include "cdownloadcvtvalidationmgr.h"
#include "CommonDownloadNVDataAccess.h"
#include "vlpluginapp_halcdlapi.h"
#include "ccodeimagevalidator.h"
#include "CommonDownloadSnmpInf.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "utilityMacros.h"
#include <rdk_debug.h>
#include "rmf_osal_sync.h"
#include "rmf_osal_util.h"
#include "ctype.h"
#include <semaphore.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <snmp_types.h>
#ifndef SNMP_IPC_CLIENT
#include <vlSnmpClient.h>
#endif // SNMP_IPC_CLIENT
#if USE_SYSRES_MLT
#include "rpl_new.h"
#include "rpl_malloc.h"
#endif


#ifdef GLI
#include "libIBus.h"
#include "sysMgr.h"
#endif

#define VL_CDL_FIRMWARE_STATUS_FILE             "/opt/fwdnldstatus.txt"

static int vlg_DSGCVTFlag = 0;
static unsigned char *vlg_pDefDLCVT = 0, vlg_DefDownloadPending =FALSE;
static unsigned long vlg_DefCVTSize =0;
static sem_t vlg_SnmpCertStatusEvent;
static rmf_osal_Mutex vlg_MutexSnmpCertStatus;

static unsigned long vlg_CdlInstallingCount = 0;
static unsigned char *vlg_pCvtRetry;

int vlg_CDLFirmwareImageStatus = IMAGE_AUTHORISED;
int vlg_CDLFirmwareCodeDLStatus = DOWNLOADING_COMPLETE;// Changed as per the funai's request..
int vlg_CDLFirmwareDLFailedStatus = CDL_ERROR_1;

static unsigned char FileName_TobeLoaded[255];
static int FileName_TobeLoaded_Size = 0;
CDownloadCVTValidationMgr *vlg_pCVTValidationMgr;
CCodeImageValidator *vlg_pCodeImageValidationMgr;
CommonDownloadState_e eCDLM_State; // state of the commondownload manager...
// The following data is for tesing purpose of PKCS7 CVT which static memory data
int vlg_USETestPKCSCVT = 1 ;
int vlg_FileEnable = 1;
static int vlg_UpgradeFailedState = 0;
static int vlg_ImageUpgradeState = 0;
static int vlg_SysCtrlSesOpened = 0;
static int vlg_CvtTftpDLstarted = 0;
static int vlg_SnmpCertStatusRetvalue;
static CDLMgrSnmpCertificateStatus_t vlg_SnmpCertStatus;
extern bool vlg_JavaCTPEnabled;
extern "C" void PostBootEvent(int eEventType, int nMsgCode, unsigned long ulData);
extern "C" int fpEnableUpdates(int bEnable);
int CommonDownloadMgrSnmpCertificateStatus_CDL(CDLMgrSnmpCertificateStatus_t *pStatus);
#define SNMP_ONLY
//Jul-17-2012: Removed unused Mfr certs previously used for unit testing.
//void vlTestSignedImagesStoredLocally();
///////////////// The following functions are for testing purpose....
bool vlIsCdlDeferredRebootEnabled();
static void PrintBuff(unsigned char *pData, int Size)
{
    int ii;
    
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","\nPKCS7CVT[]={ \n");
    for(ii = 0; ii < Size; ii++)
    {
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","0x%02X, ",pData[ii]);
        if( ((ii+1) % 16) == 0)
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","\n");
        }
    }
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","\n};\n");
}
#ifdef PKCS_UTILS
void PrintSignerInfo(pkcs7SignedDataInfo_t SignerInfo)
{
    int ii;

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," >>>>>>>>>>>>>>>>. MfgSignerInfo >>>>>>>>>>>>>>>>>>>>>>>> \n");
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","SignerInfo.MfgSignerInfo.bValid:%d \n",SignerInfo.MfgSignerInfo.bValid);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","SignerInfo.MfgSignerInfo.Version:%d \n",SignerInfo.MfgSignerInfo.Version);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","SignerInfo.MfgSignerInfo.CertSerialNumber:0x%X \n",SignerInfo.MfgSignerInfo.CertSerialNumber);

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","SignerInfo.MfgSignerInfo.signingTime:0x");
    for(ii = 0; ii < 12; ii++)
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","%X: ",SignerInfo.MfgSignerInfo.signingTime.Time[ii]);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","\n");

    if(SignerInfo.MfgSignerInfo.IssuerName.pCountryName != NULL)
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","SignerInfo.MfgSignerInfo.IssuerName.pCountryName:%s \n",SignerInfo.MfgSignerInfo.IssuerName.pCountryName);
    else
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","SignerInfo.MfgSignerInfo.IssuerName.pCountryName is NULL !! Error \n");

    if(SignerInfo.MfgSignerInfo.IssuerName.pOrgName != NULL)
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","SignerInfo.MfgSignerInfo.IssuerName.pOrgName:%s \n",SignerInfo.MfgSignerInfo.IssuerName.pOrgName);
    else
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","SignerInfo.MfgSignerInfo.IssuerName.pOrgName is NULL !! Error \n");

    if(SignerInfo.MfgSignerInfo.IssuerName.pComName != NULL)
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","SignerInfo.MfgSignerInfo.IssuerName.pComName:%s \n",SignerInfo.MfgSignerInfo.IssuerName.pComName);
    else
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","SignerInfo.MfgSignerInfo.IssuerName.pComName is NULL !! Error \n");

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," >>>>>>>>>>>>>>>>. CoSignerInfo >>>>>>>>>>>>>>>>>>>>>>>> \n");
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","SignerInfo.CoSignerInfo.bValid:%d \n",SignerInfo.CoSignerInfo.bValid);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","SignerInfo.CoSignerInfo.Version:%d \n",SignerInfo.CoSignerInfo.Version);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","SignerInfo.CoSignerInfo.CertSerialNumber:0x%X \n",SignerInfo.CoSignerInfo.CertSerialNumber);

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","SignerInfo.CoSignerInfo.signingTime:0x");
    for(ii = 0; ii < 12; ii++)
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","%X: ",SignerInfo.CoSignerInfo.signingTime.Time[ii]);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","\n");

    if(SignerInfo.CoSignerInfo.IssuerName.pCountryName != NULL)
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","SignerInfo.CoSignerInfo.IssuerName.pCountryName:%s \n",SignerInfo.CoSignerInfo.IssuerName.pCountryName);
    else
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","SignerInfo.CoSignerInfo.IssuerName.pCountryName is NULL !! Error \n");

    if(SignerInfo.CoSignerInfo.IssuerName.pOrgName != NULL)
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","SignerInfo.CoSignerInfo.IssuerName.pOrgName:%s \n",SignerInfo.CoSignerInfo.IssuerName.pOrgName);
    else
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","SignerInfo.CoSignerInfo.IssuerName.pOrgName is NULL !! Error \n");

    if(SignerInfo.CoSignerInfo.IssuerName.pComName != NULL)
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","SignerInfo.CoSignerInfo.IssuerName.pComName:%s \n",SignerInfo.CoSignerInfo.IssuerName.pComName);
    else
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","SignerInfo.CoSignerInfo.IssuerName.pComName is NULL !! Error \n");


    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," >>>>>>>>>>>>>>>>. MfgCvcInfo >>>>>>>>>>>>>>>>>>>>>>>> \n");
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","SignerInfo.MfgCvcInfo.bValid:%d \n",SignerInfo.MfgCvcInfo.bValid);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","SignerInfo.MfgCvcInfo.ValidityPeriod:%d \n",SignerInfo.MfgCvcInfo.ValidityPeriod);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","SignerInfo.MfgCvcInfo.ValStartTime:0x");
    for(ii = 0; ii < 12; ii++)
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","%X: ",SignerInfo.MfgCvcInfo.ValStartTime.Time[ii]);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","\n");

    if(SignerInfo.MfgCvcInfo.Subject.pCountryName != NULL)
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","SignerInfo.MfgCvcInfo.Subject.pCountryName:%s \n",SignerInfo.MfgCvcInfo.Subject.pCountryName);
    else
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","SignerInfo.MfgCvcInfo.Subject.pCountryName is NULL !! Error \n");

    if(SignerInfo.MfgCvcInfo.Subject.pOrgName != NULL)
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","SignerInfo.MfgCvcInfo.Subject.pOrgName:%s \n",SignerInfo.MfgCvcInfo.Subject.pOrgName);
    else
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","SignerInfo.CoSignerInfo.Subject.pOrgName is NULL !! Error \n");

    if(SignerInfo.MfgCvcInfo.Subject.pComName != NULL)
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","SignerInfo.MfgCvcInfo.Subject.pComName:%s \n",SignerInfo.MfgCvcInfo.Subject.pComName);
    else
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","SignerInfo.MfgCvcInfo.Subject.pComName is NULL !! Error \n");

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," >>>>>>>>>>>>>>>>. CosignerCvcInfo >>>>>>>>>>>>>>>>>>>>>>>> \n");
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","SignerInfo.CosignerCvcInfo.bValid:%d \n",SignerInfo.CosignerCvcInfo.bValid);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","SignerInfo.CosignerCvcInfo.ValidityPeriod:%d \n",SignerInfo.CosignerCvcInfo.ValidityPeriod);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","SignerInfo.CosignerCvcInfo.ValStartTime:0x");
    for(ii = 0; ii < 12; ii++)
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","%X: ",SignerInfo.CosignerCvcInfo.ValStartTime.Time[ii]);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","\n");

    if(SignerInfo.CosignerCvcInfo.Subject.pCountryName != NULL)
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","SignerInfo.CosignerCvcInfo.Subject.pCountryName:%s \n",SignerInfo.CosignerCvcInfo.Subject.pCountryName);
    else
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","SignerInfo.CosignerCvcInfo.Subject.pCountryName is NULL !! Error \n");

    if(SignerInfo.CosignerCvcInfo.Subject.pOrgName != NULL)
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","SignerInfo.CosignerCvcInfo.Subject.pOrgName:%s \n",SignerInfo.CosignerCvcInfo.Subject.pOrgName);
    else
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","SignerInfo.CosignerCvcInfo.Subject.pOrgName is NULL !! Error \n");

    if(SignerInfo.CosignerCvcInfo.Subject.pComName != NULL)
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","SignerInfo.CosignerCvcInfo.Subject.pComName:%s \n",SignerInfo.CosignerCvcInfo.Subject.pComName);
    else
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","SignerInfo.CosignerCvcInfo.Subject.pComName is NULL !! Error \n");
}
#endif
void PrintfCVCInfo(cvcinfo_t cvcInfo)
{
    int ii;

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL",">>>>>>>>>>.. CVC INFO >>>>>>>>>>>> \n");
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","cvcInfo.codeSigningExists:%d \n",cvcInfo.codeSigningExists);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","cvcInfo.validityPeriod:%0x08X \n",cvcInfo.validityPeriod);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," notBefore :0x");
    for(ii = 0; ii < 12; ii++)
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","0x%X: ",cvcInfo.notBefore[ii]);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","\n");
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," notAfter :0x");
    for(ii = 0; ii < 12; ii++)
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","0x%X: ",cvcInfo.notAfter[ii]);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","\n");

    if(cvcInfo.subjectCountryName != NULL)
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","cvcInfo.subjectCountryName:%s \n",cvcInfo.subjectCountryName);
    else
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","cvcInfo.subjectCountryName in NULL Error \n");

    if(cvcInfo.issuerCountryName != NULL)
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","cvcInfo.issuerCountryName:%s \n",cvcInfo.issuerCountryName);
    else
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","cvcInfo.issuerCountryName in NULL Error \n");

    if(cvcInfo.subjectOrgName != NULL)
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","cvcInfo.subjectOrgName:%s \n",cvcInfo.subjectOrgName);
    else
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","cvcInfo.subjectOrgName in NULL Error \n");

    if(cvcInfo.issuerOrgName != NULL)
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","cvcInfo.issuerOrgName:%s \n",cvcInfo.issuerOrgName);
    else
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","cvcInfo.issuerOrgName in NULL Error \n");

    if(cvcInfo.subjectCommonName != NULL)
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","cvcInfo.subjectCommonName:%s \n",cvcInfo.subjectCommonName);
    else
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","cvcInfo.subjectCommonName in NULL Error \n");

    if(cvcInfo.issuerCommonName != NULL)
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","cvcInfo.issuerCommonName:%s \n",cvcInfo.issuerCommonName);
    else
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","cvcInfo.issuerCommonName in NULL Error \n");

}
static void PrintBuffer(unsigned char *pBuf, int size)
{
    int ii;
    if(pBuf == NULL)
    {
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","PrintBuffer: NULL poiGnter Error \n");
        return;
    }

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","PrintBuffer: buffer pointer:0x%08X size :0x%X %d \n",pBuf,size, size);
    for( ii = 0; ii < size; ii++)
    {
        if((ii % 16) == 0)
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","\n");
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","0x%02X,",pBuf[ii]);
    }
}

/////////////////////////////////////////////////////////////////
static char *vlg_pFilePath=NULL;
static int vlg_NumObjsComplete = 0;
static int vlg_CvtTftDLNumOfObjs = 0;
static int vlg_CvtTftDLObjNum = 0;
int vlSetCvtTftpDLstarted(int val)
{
    vlg_CvtTftpDLstarted = val;
    return 0;
}
int vlSetCvtTftDLNumOfObjs(int val)
{
    vlg_CvtTftDLNumOfObjs = val;
    return 0;
}
int vlSetCvtTftDLObjNum(int val)
{
    vlg_CvtTftDLObjNum = val;
    return 0;
}
static int CommonDownloadSendMsgEvent( char *pMessage, unsigned long length)
{
    LCSM_EVENT_INFO *pEvent=NULL;
    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    char *pData;
    rmf_osal_event_handle_t event_handle;
    rmf_osal_event_params_t event_params = {0};
	
    if(pMessage == NULL || length == 0)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadSendMsgEvent: Error !!! returning pMessage == NULL || length == 0 \n");
        return -1;
    }
	rmf_osal_memAllocP(RMF_OSAL_MEM_POD, length + 1,(void **)&pData);
    if(pData == NULL)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadSendMsgEvent: Error !!! rmf_osal_memAllocP failed returned NULL \n");
        return -1;
    }
    memset(pData,0,length + 1);
    memcpy(pData,pMessage,length);


    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CommonDownloadSendMsgEvent: Posting the meesage:%s \n",pData);

    rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(LCSM_EVENT_INFO),(void **)&pEvent);    
    if(pEvent == NULL)
    {
        rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pData);
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadSendMsgEvent: Error !!! pEvent is  NULL !! \n");
        return -1;
    }
    memset(pEvent, 0, sizeof(LCSM_EVENT_INFO));
    pEvent->event = VL_CDL_DISP_MESSAGE;
    pEvent->dataPtr = pData;
    pEvent->dataLength = length;
		
    //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," before dispatch((pfcEventCableCard *)em->new_event(pEvent) \n");

    event_params.priority = 0; //Default priority;
    event_params.data = (void *)pEvent;
    event_params.data_extension = 0;
    event_params.free_payload_fn = podmgr_freefn;
    rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_GATEWAY, VL_CDL_DISP_MESSAGE, 
		&event_params, &event_handle);

    rmf_osal_eventmanager_dispatch_event(em, event_handle);

    return 0;

}


int CommonDownLoadStoreCertificates(CommonDownLImage_t *pDLImage)
{
    int iRet;
    if( (pDLImage->pCLCvcRootCaCert != NULL) && (pDLImage->CLCvcRootCaCertLen != 0) )
    {
       iRet = CommonDownloadSetCLCvcRootCA(pDLImage->pCLCvcRootCaCert,pDLImage->CLCvcRootCaCertLen);
       if(iRet != 0)
       {
           RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownLoadStoreCertificates: Error !!! Failure storing CL CVC Root CA cert \n");
       }
       else
       {
           RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CommonDownLoadStoreCertificates: SUCCESS !!! storing CL CVC Root CA cert \n");
       }
    }
    if( (pDLImage->pCLCvcCaCert != NULL) && (pDLImage->CLCvcCaCertLen != 0) )
    {
    iRet = CommonDownloadSetCLCvcCA(pDLImage->pCLCvcCaCert,pDLImage->CLCvcCaCertLen);
    if(iRet != 0)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownLoadStoreCertificates: Error !!! Failure storing CL CVC  CA cert \n");
    }
    else
    {
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CommonDownLoadStoreCertificates: SUCCESS !!! storing CL CVC  CA cert \n");
    }
    }
     if( (pDLImage->pCLCvcDeviceCaCert != NULL) && (pDLImage->CLCvcDeviceCaCertLen != 0) && (pDLImage->pCLCvcCaCert != NULL) )
    {
    iRet = CommonDownloadSetManCvc(pDLImage->pCLCvcCaCert,pDLImage->CLCvcCaCertLen);
    if(iRet != 0)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownLoadStoreCertificates: Error !!! Failure storing MAN CVC  cert \n");
    }
    else
    {
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CommonDownLoadStoreCertificates: SUCCESS !!! storing MAN CVC  cert \n");
    }
    }
    return 0;
  
}


#define VL_CDL_DEV_INITIATED_SNMP_CDL_IN_PROGRESS   "/tmp/device_initiated_snmp_cdl_in_progress" // This flag will always be ignored due to cyclic dependency. snmp cdl translates to ecm cdl.
#define VL_CDL_DEV_INITIATED_RCDL_IN_PROGRESS       "/tmp/device_initiated_rcdl_in_progress"
#define VL_CDL_ECM_INITIATED_CDL_IN_PROGRESS        "/tmp/ecm_initiated_cdl_in_progress"
#define VL_CDL_ECM_INITIATED_CDL_FILE_NAME          "/tmp/ecm_initiated_cdl_file_name"
#define VL_CDL_FLASHED_FILE_NAME                    "/opt/cdl_flashed_file_name"
#define VL_CDL_ECM_INITIATED_CDL_STATE              "/tmp/ecm_initiated_cdl_state"
#define VL_CDL_ECM_INITIATED_CDL_PROGRESS           "/tmp/ecm_initiated_cdl_progress"

bool VL_CDL_CheckIfFlagIsSet(const char * pszFlag)
{
    struct stat st;	
    bool ret=false;

    if((0 == stat(pszFlag, &st) && (0 != st.st_ino)))
    {
         ret=true;
         RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","%s: Flag '%s' is set. Denying permission for CDL.\n",__FUNCTION__, pszFlag);
    }
    else
    {
         ret=false;
         RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","%s: Flag '%s' is not set.\n",__FUNCTION__, pszFlag);
    }
    return ret;
}

void VL_CDL_TouchFile(const char * pszFile)
{
    if(NULL != pszFile)
    {
        FILE * fp = fopen(pszFile, "w");
        if(NULL != fp)
        {
            fclose(fp);
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","%s: Registered '%s'.\n",__FUNCTION__, pszFile);
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","%s: Could not open '%s'.\n",__FUNCTION__, pszFile);
        }
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","%s: pszFile is NULL.\n",__FUNCTION__);
    }
}

void VL_CDL_RemoveFile(const char * pszFile)
{
    if(NULL != pszFile)
    {
        if ( remove(pszFile) < 0 )
        {
	       RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","%s: remove call failed.\n",__FUNCTION__);
        }
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","%s: Un-Registered '%s'.\n",__FUNCTION__, pszFile);
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","%s: pszFile is NULL.\n",__FUNCTION__);
    }
}

void VL_CDL_RegisterCdlString(const char * pszFile, const char * cdlString)
{
    if(NULL != pszFile)
    {
        FILE * fp = fopen(pszFile, "w");
        if(NULL != fp)
        {
            if(NULL != cdlString)
            {
                int nRet = 0;
                if((nRet = fprintf(fp, "%s\n", cdlString)) <= 0)
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","%s: fprintf returned '%d'.\n",__FUNCTION__, nRet);
                }
                else
                {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","%s: Registered '%s' in '%s'.\n",__FUNCTION__, cdlString, pszFile);
                }
            }
            else
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","%s: cdlString is NULL.\n",__FUNCTION__);
            }
            fflush(fp);
            fsync(fileno(fp));
            fclose(fp);
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","%s: Could not open '%s'.\n",__FUNCTION__, pszFile);
        }
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","%s: pszFile is NULL.\n",__FUNCTION__);
    }
}

void VL_CDL_ReadCdlString(const char * pszFile, char * cdlString, int nCapacity)
{
    if(NULL != pszFile)
    {
        FILE * fp = fopen(pszFile, "r");
        if(NULL != fp)
        {
            if(NULL != cdlString)
            {
                if(NULL == fgets(cdlString, nCapacity, fp))
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","%s: fgets returned NULL.\n",__FUNCTION__);
                }
                else
                {
                    char * pszNewLine = strpbrk(cdlString, "\r\n");
                    if(NULL != pszNewLine)
                    {
                        if((pszNewLine - cdlString) < nCapacity)
                        {
                            *pszNewLine = '\0';
                        }
                    }
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","%s: Read '%s' from '%s'.\n",__FUNCTION__, cdlString, pszFile);
                }
            }
            else
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","%s: cdlString is NULL.\n",__FUNCTION__);
            }
            fclose(fp);
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","%s: Could not open '%s'.\n",__FUNCTION__, pszFile);
        }
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","%s: pszFile is NULL.\n",__FUNCTION__);
    }
}



void VL_CDL_RegisterFlashedFile(const char * szFile)
{
    // Begin patch: MOT7425-4198
    VL_CDL_SOFTWARE_IMAGE_NAME imageName;
    memset( &(imageName), 0, sizeof(imageName) );
    imageName.eImageType = VL_CDL_IMAGE_TYPE_MONOLITHIC;
    strncpy(imageName.szSoftwareImageName, szFile, sizeof(imageName.szSoftwareImageName));
	imageName.szSoftwareImageName[ VL_MAX_CDL_STR_SIZE-1 ] = 0;
    CHALCdl_notify_cdl_mgr_event(VL_CDL_MANAGER_EVENT_SAVE_UPGRADE_IMAGE_NAME,(unsigned long)&imageName);
    // End patch: MOT7425-4198
    
    VL_CDL_RegisterCdlString(VL_CDL_FLASHED_FILE_NAME, szFile);
}

void VL_CDL_RCDL_IndicationFile()
{
    VL_CDL_RemoveFile(VL_CDL_DEV_INITIATED_RCDL_IN_PROGRESS); // Un-register device trigerred RCDL indication file.
}

void VL_CDL_RegisterCdlProgress(int nBytes)
{
    const char * pszFile = VL_CDL_ECM_INITIATED_CDL_PROGRESS;
    
    if(NULL != pszFile)
    {
        FILE * fp = fopen(pszFile, "w");
        if(NULL != fp)
        {
            int nRet = 0;
            if((nRet = fprintf(fp, "%d\n", nBytes)) <= 0)
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","%s: fprintf return '%d'.\n",__FUNCTION__, nRet);
            }
            else
            {
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","%s: Registered '%d' in '%s'.\n",__FUNCTION__, nBytes, pszFile);
            }
            fclose(fp);
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","%s: Could not open '%s'.\n",__FUNCTION__, pszFile);
        }
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","%s: pszFile is NULL.\n",__FUNCTION__);
    }
}

bool VL_CDL_checkStringCategory(const char * cdlString)
{
   bool categoryStatus;
   char *reasonFailed = "Failed";
   char *reasonDenied = "Denied";
   char *reasonRejected = "Rejected";

   categoryStatus = TRUE;
   if(((strstr(cdlString, reasonFailed) != NULL)
        || (strstr(cdlString, reasonDenied) != NULL)
        || (strstr(cdlString, reasonRejected) != NULL)) ) {
        categoryStatus = FALSE;
   }

   return categoryStatus;
}

void sendFirmwareUpdateStateChangeEvent (int newState)
{
    RDK_LOG (RDK_LOG_ERROR, "LOG.RDK.CDL", "%s: newState = [%d]\n", __FUNCTION__, newState);

    IARM_Bus_SYSMgr_EventData_t eventData;
    eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_FIRMWARE_UPDATE_STATE;
    eventData.data.systemStates.state = newState;
    eventData.data.systemStates.payload[0] = 0;
    IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
}

void VL_CDL_UpdateFWDnldStatus(const char * status)
{
    char command[250] = {'\0'};
    snprintf(command, sizeof(command), "sed -i 's/Status|.*/Status|%s/g' %s", status, VL_CDL_FIRMWARE_STATUS_FILE);
    system(command);
}

void VL_CDL_UpdateFWStatusFile(const char * cdlString)
{
    bool categoryStatus;
    char sedCommand[250] = {'\0'};
    char fwUpdateState[40] = {'\0'};
    char readLine[100] = {'\0'};

    FILE *fp = fopen(VL_CDL_FIRMWARE_STATUS_FILE, "a+");
    if(NULL != fp)
    {
        if(NULL != cdlString)
        {
            RDK_LOG (RDK_LOG_DEBUG, "LOG.RDK.CDL", "%s: cdlString = [%s]\n", __FUNCTION__, cdlString);

            categoryStatus = VL_CDL_checkStringCategory(cdlString);
            if ( categoryStatus == FALSE)
            {
                VL_CDL_UpdateFWDnldStatus("Failure");
                snprintf( sedCommand, sizeof( sedCommand ), 
                    "sed -i 's/FwUpdateState|.*/FwUpdateState|Failed/g' %s", VL_CDL_FIRMWARE_STATUS_FILE);
                sendFirmwareUpdateStateChangeEvent (IARM_BUS_SYSMGR_FIRMWARE_UPDATE_STATE_FAILED);
            }
            else if ( strcmp(cdlString,"Preparing to reboot") == 0 ) {
                snprintf( sedCommand, sizeof( sedCommand ), "sed -i 's/FwUpdateState|.*/FwUpdateState|%s/g' %s",
                    cdlString, VL_CDL_FIRMWARE_STATUS_FILE);
                sendFirmwareUpdateStateChangeEvent (IARM_BUS_SYSMGR_FIRMWARE_UPDATE_STATE_PREPARING_TO_REBOOT);
            }
            else {
                if ( strcmp(cdlString,"RCDL Upgrade Succeeded") == 0 ) {
                    strcpy(fwUpdateState, "Validation complete");
                    VL_CDL_UpdateFWDnldStatus("Success");
                    sendFirmwareUpdateStateChangeEvent (IARM_BUS_SYSMGR_FIRMWARE_UPDATE_STATE_VALIDATION_COMPLETE);
                } else if ( strcmp(cdlString,"CDL Rebooting After Upgrade") == 0 ) {
                    strcpy(fwUpdateState, "Preparing to reboot");
                    sendFirmwareUpdateStateChangeEvent (IARM_BUS_SYSMGR_FIRMWARE_UPDATE_STATE_PREPARING_TO_REBOOT);
                }
                         
                if (fwUpdateState[0] != '\0') {
                    snprintf( sedCommand, sizeof( sedCommand ), "sed -i 's/FwUpdateState|.*/FwUpdateState|%s/g' %s",
                        fwUpdateState, VL_CDL_FIRMWARE_STATUS_FILE);
                }
            }
            system(sedCommand);
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","%s: trying to update in firmware status file, but cdlString is NULL.\n",__FUNCTION__);
        }
        fflush(fp);
        fsync(fileno(fp));
        fclose(fp);
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","%s: Could not open '%s'.\n",__FUNCTION__, VL_CDL_FIRMWARE_STATUS_FILE);
    }
}

void VL_CDL_RegisterCdlState(const char * cdlString)
{
    if ( strcmp(cdlString,"Preparing to reboot") != 0 ) {
        VL_CDL_RegisterCdlString(VL_CDL_ECM_INITIATED_CDL_STATE, cdlString);
    }
    VL_CDL_UpdateFWStatusFile(cdlString);
}

extern int vlg_bHrvInitWithResetInProgress;

int VL_CDL_IsSameFileName(const char * pszFileName)
{
    if(NULL == pszFileName)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","%s: pszFileName is NULL.\n",__FUNCTION__);
        return 0;
    }
    char szFlashedFileName[VL_MAX_CDL_STR_SIZE] = "\0";
    VL_CDL_ReadCdlString(VL_CDL_FLASHED_FILE_NAME, szFlashedFileName, sizeof(szFlashedFileName));
    if(0 == (strcmp(pszFileName, szFlashedFileName)))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","%s: New file name '%s' MATCHES previously flashed file name '%s'.\n",__FUNCTION__, pszFileName, szFlashedFileName);
        return 1;
    }
    
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","%s: New file name '%s' does not match previously flashed file name '%s'.\n",__FUNCTION__, pszFileName, szFlashedFileName);
    return 0;
}

int VL_CDL_CheckIfCdlAllowed(const char * pszFileName)
{
    char * pszFlag = NULL;
    if(vlg_bHrvInitWithResetInProgress)
    {
/*
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","%s: vlg_bHrvInitWithResetInProgress is set. Denying permission for CDL.\n",__FUNCTION__);
        VL_CDL_RegisterCdlState("CDL Denied: HRV Init in progress.");
        return 0; // deny.
*/
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","%s: vlg_bHrvInitWithResetInProgress is set. Allowing permission for CDL per MOT7425-5624.\n",__FUNCTION__);
    }
    //This check cyclically conflicting and so is NOT checked ever // snmp cdl translatest to ecm cdl //if(VL_CDL_CheckIfFlagIsSet(VL_CDL_DEV_INITIATED_SNMP_CDL_IN_PROGRESS)) return 0;
    pszFlag = VL_CDL_DEV_INITIATED_RCDL_IN_PROGRESS;
    if(VL_CDL_CheckIfFlagIsSet(pszFlag))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","%s: '%s' is set. Denying permission for CDL.\n",__FUNCTION__, pszFlag);
        VL_CDL_RegisterCdlState("CDL Denied: RCDL in progress.");
        return 0; // deny.
    }
    if(VL_CDL_IsSameFileName(pszFileName))
    {
/*
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","%s: File Name '%s' matches previously flashed file name. Denying permission for CDL.\n",__FUNCTION__, pszFileName);
        VL_CDL_RegisterCdlState("CDL Denied: Requested Image Name already flashed.");
        return 0; // deny.
*/
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL", "%s: File Name '%s' matches previously flashed file name. Allowing permission for CDL.\n",__FUNCTION__, pszFileName);
    }
    return 1; // allow.
}

void VL_CDL_Clear_Cached_FileName()
{
    memset(FileName_TobeLoaded, 0, sizeof(FileName_TobeLoaded));
    FileName_TobeLoaded_Size = 0;
}



int CDL_Callback(void * pAppData, VL_CDL_DRIVER_EVENT_TYPE eventType, int nEventData)
{
    VL_CDL_DOWNLOAD_STATUS_QUESTION downloadStatus;
    char Message[512];
    switch(eventType)
    {

    case VL_CDL_DRIVER_EVENT_IS_DOWNLOAD_PERMITTED:
    {
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CDL_Callback: received  event VL_CDL_DRIVER_EVENT_IS_DOWNLOAD_PERMITTED \n");

        VL_CDL_GET_STRUCT(VL_CDL_QUESTION_IS_DOWNLOAD_PERMITTED,pStruct, nEventData);
        //if(eCDLM_State == COM_DOWNL_IDLE)
        {
                    const char * pszFileName = NULL;
                    if((0 == pStruct->nFileNameSize) || (0 == strlen(pStruct->szSoftwareImageName)))
                    {
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CDL_Callback: No information available in pStruct->szSoftwareImageName \n");
                        
                        if((0 == FileName_TobeLoaded_Size) || (0 == strlen((const char *)FileName_TobeLoaded)))
                        {
                            FILE *fp = NULL;
                            char *temp = NULL;
                            char DataBuffer[512];
                            char getFileNameCommand[1024];
                            char *tempCDLFilePath = NULL;
                            char *SNMPbaseDirPath = NULL;
                            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CDL_Callback: No information available in FileName_TobeLoaded \n");
                            VL_CDL_Clear_Cached_FileName();
                            VL_SNMP_API_RESULT ret = VL_SNMP_API_RESULT_SUCCESS;

                            //Resolution steps
                            //1.Run the snmp command to get the filename.
                            //2.Write the name to a file <SNMP.TMP.FILEPATH>
                            //3.Read the file and get the name
                            //4.Remove the file <SNMP.TMP.FILEPATH>

                            memset(getFileNameCommand,0,1024);
                            memset(DataBuffer,0,512);
                            //The path for snmp base directory is set in rmfconfig.ini at SNMP.BASE.DIR
                            //The path for temporary file is set in rmfconfig.ini at SNMP.TMP.FILEPATH
                            //Default location for SNMP.TMP.FILEPATH is /opt/TempCDLFilename
                            //Default location for SNMP.BASE.DIR is /mnt/nfs/bin/target-snmp
							
							//Getting temporary file location
							tempCDLFilePath = (char *)rmf_osal_envGet("SNMP.TMP.FILEPATH");
							if(NULL == tempCDLFilePath)
							{
								tempCDLFilePath = "/opt/TempCDLFilename";
							}
							//Getting SNMP base location.
							SNMPbaseDirPath = (char *)rmf_osal_envGet("SNMP.BASE.DIR");
							if(NULL == SNMPbaseDirPath)
							{
								SNMPbaseDirPath = "/mnt/nfs/bin/target-snmp";
							}							
                            snprintf(getFileNameCommand,1024,
							"export SNMP_BASE_DIR=%s;export SNMP_BIN_DIR=$SNMP_BASE_DIR/bin;export PATH=$PATH:$SNMP_BIN_DIR;export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$SNMP_BASE_DIR/lib;export snmpcommunitystring=`sh /lib/rdk/snmpCommunityParser.sh`;if [ ${#snmpcommunitystring} -le 0 ];then export snmpcommunitystring=public;fi;snmpwalk -OQ -v 2c -c \"$snmpcommunitystring\" 192.168.100.1  .1.3.6.1.2.1.69.1.3.2.0 | sed 's/\"//g' >%s",\
                            SNMPbaseDirPath,tempCDLFilePath);
                            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","Get File name command is %s and temporary file is at %s\n",\
							                                     getFileNameCommand,tempCDLFilePath);
							
                            system(getFileNameCommand);
                            fp = fopen(tempCDLFilePath,"r+");
                            if(NULL != fp)
                            {
                                fgets(DataBuffer,512,fp);
                                //Sometimes you get a newline along with string in fgets.THis is to remove the newline
                                temp = strchr(DataBuffer,'\n');
                                if(temp)
                                {
                                    *temp = '\0';
                                }
                                temp = NULL;
                                //Output of snmp command is in the format
                                //DOCS-CABLE-DEVICE-MIB::docsDevSwFilename.0 = <imagename>.bin
                                temp = strchr(DataBuffer,'=');
                                if(temp != NULL)
                                {
				                    while(!isalnum(*temp))
					                    temp=temp+1;
				                    snprintf((char *)FileName_TobeLoaded,255,"%s",temp);
                                    FileName_TobeLoaded_Size = strlen((const char *)FileName_TobeLoaded);
                                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","Filename is %s \n",FileName_TobeLoaded);
                                    pszFileName = (const char *)FileName_TobeLoaded;
                                }
                                else
                                {
                                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","Failed to get file name\n");
                                }
                                fclose(fp);
                                //Now remove the file
                                if(remove(tempCDLFilePath) == 0)
                                {
                                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","Removed temp file\n");
                                }
                                else
                                {
                                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","Temp file remove failed\n");
                                }
                            }
                            else
                            {
                                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","Failed to open File with imagename\n");
                            }
                        }
                        else
                        {
                            pszFileName = (const char *)FileName_TobeLoaded;
                        }
                    }
                    else
                    {
                        pszFileName = pStruct->szSoftwareImageName;
                    }
                    
                    pStruct->bIsDownloadPermitted = VL_CDL_CheckIfCdlAllowed(pszFileName);
                    if(pStruct->bIsDownloadPermitted) VL_CDL_RegisterCdlState("CDL Permitted");
                    
                    if( (pStruct->nFileNameSize != 0) && (pStruct->szSoftwareImageName != NULL ) )
                    {
                        if(pStruct->nFileNameSize < sizeof(FileName_TobeLoaded))
                        {
                            FileName_TobeLoaded_Size = pStruct->nFileNameSize;
                    
                            rmf_osal_memcpy(FileName_TobeLoaded,pStruct->szSoftwareImageName,FileName_TobeLoaded_Size, sizeof(FileName_TobeLoaded), FileName_TobeLoaded_Size);
                            FileName_TobeLoaded[FileName_TobeLoaded_Size] = 0;
                            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CDL_Callback:File name :%s\n",FileName_TobeLoaded);
                        }
                        else
                        {
                            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CDL_Callback:Error Size of the file name pStruct->nFileNameSize:%d \n",pStruct->nFileNameSize);
                        }
                    }
                    else
                    {
                        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CDL_Callback:pStruct->nFileNameSize:%d pStruct->szSoftwareImageName:%X\n",pStruct->nFileNameSize,pStruct->szSoftwareImageName);
                    }
                    
                    if(pStruct->bIsDownloadPermitted)
                    {
                        VL_CDL_RegisterCdlString(VL_CDL_ECM_INITIATED_CDL_FILE_NAME, (const char *)FileName_TobeLoaded);
                    }
                    else
                    {
                        // if CDL request was denied clear the cached file-name;
                        VL_CDL_Clear_Cached_FileName();
                    }
                
        }
        //else
        //{
        //    pStruct-> bIsDownloadPermitted = 0;
        //}
    }
    break;
    
    case VL_CDL_DRIVER_EVENT_STARTING_DOWNLOAD_IMAGE_NAME:
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CDL_Callback: received  event VL_CDL_DRIVER_EVENT_STARTING_DOWNLOAD_IMAGE_NAME \n");
        VL_CDL_RegisterCdlString(VL_CDL_ECM_INITIATED_CDL_FILE_NAME, (const char *)nEventData);
    }
    break;

    case VL_CDL_DRIVER_EVENT_FINISHED_UPGRADE_IMAGE_NAME:
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CDL_Callback: received  event VL_CDL_DRIVER_EVENT_FINISHED_UPGRADE_IMAGE_NAME \n");
        VL_CDL_RegisterFlashedFile((const char *)nEventData);
    }
    break;

        case VL_CDL_DRIVER_EVENT_DOWNLOAD_STARTED:

    {
        VL_CDL_RegisterCdlState("CDL Download Started");
        VL_CDL_TouchFile(VL_CDL_ECM_INITIATED_CDL_IN_PROGRESS); // Register ECM CDL Event.
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CDL_Callback: received  event VL_CDL_DRIVER_EVENT_DOWNLOAD_STARTED \n");
        snprintf((char*)Message,sizeof(Message),"Host Image Download Started by eCM.\nDo not Power Off the box");
        CommonDownloadSendMsgEvent(Message,strlen(Message));
        eCDLM_State = COM_DOWNL_CODE_IMAGE_DOWNLOAD_START;
        vlg_CDLFirmwareCodeDLStatus = DOWNLOADING_STARTED;
        
        //Cancel the differed pending download 
        if(vlg_DefDownloadPending == TRUE)
        {
          
            vlg_DefCVTSize = 0;
            vlg_pDefDLCVT = NULL;
            vlg_DefDownloadPending = FALSE;
        }
        
        if(vlg_CvtTftpDLstarted == 0 )
        {
            CommonDownloadSendSysCntlComm(DOWNLOAD_STARTED);
    
            //May-24-2012: File not found not notified to boot manager // CommonDownloadManager::PostCDLBootMsgDiagEvent(RMF_BOOTMSG_DIAG_MESG_CODE_START, 0);

            
        }
        #ifdef GLI
            IARM_Bus_SYSMgr_EventData_t eventData;
            eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_FIRMWARE_DWNLD;
            eventData.data.systemStates.state =  IARM_BUS_SYSMGR_IMAGE_FWDNLD_DOWNLOAD_INPROGRESS; // Started.
            IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
        #endif
        break;
    }

    case VL_CDL_DRIVER_EVENT_DOWNLOAD_COMPLETED:
    {
        VL_CDL_RegisterCdlState("CDL Download Completed");
        VL_CDL_IMAGE  *pCdl_image;
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CDL_Callback: received  event VL_CDL_DRIVER_EVENT_DOWNLOAD_COMPLETED \n");
        pCdl_image = (VL_CDL_IMAGE *)nEventData;
        vlg_CDLFirmwareCodeDLStatus = DOWNLOADING_COMPLETE;
        if(pCdl_image->eImageSign == VL_CDL_IMAGE_SIGNED)
        {
            switch (pCdl_image->eImageType)
            {
                case VL_CDL_IMAGE_TYPE_MONOLITHIC:
                case VL_CDL_IMAGE_TYPE_FIRMWARE:
                case VL_CDL_IMAGE_TYPE_APPLICATION:
                case VL_CDL_IMAGE_TYPE_DATA:
                {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CDL_Callback: eImageType:%d eImageSign:%d pszImagePathName:%s szSoftwareImageName:%s \n",
                    pCdl_image->eImageType,
                    pCdl_image->eImageSign,
                    pCdl_image->pszImagePathName,
                    pCdl_image->szSoftwareImageName    );
                    vlg_NumObjsComplete++;
#ifdef DOWNLOAD_ENGINE_UTILS                    
                //    ComDownLEngineGotResponseFromTftp(COMM_DL_TFTP_SUCESS,(void *)pCdl_image);
#endif                    
                    break;
                }
                default:
                break;
            }
        }
        else
        {

            eCDLM_State = COM_DOWNL_CODE_IMAGE_DOWNLOAD_COMPLETE;
            vlg_CDLFirmwareCodeDLStatus = DOWNLOADING_COMPLETE;
            CommonDownloadSendSysCntlComm(DOWNLOAD_COMPLETE);
            snprintf((char*)Message,sizeof(Message),"Host Image Download Completed by eCM.\nDo not Power Off the box");
            CommonDownloadSendMsgEvent(Message,strlen(Message));
        
//            CommonDownloadManager::PostCDLBootMsgDiagEvent(RMF_BOOTMSG_DIAG_MESG_CODE_COMPLETE, 0);

        }
                CommonDownloadManager::PostCDLBootMsgDiagEvent(RMF_BOOTMSG_DIAG_MESG_CODE_COMPLETE, 0);
            #ifdef GLI
                IARM_Bus_SYSMgr_EventData_t eventData;
                eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_FIRMWARE_DWNLD;
                eventData.data.systemStates.state =  IARM_BUS_SYSMGR_IMAGE_FWDNLD_DOWNLOAD_COMPLETE; // Completed.
                IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
            #endif        
        break;
    }
        case VL_CDL_DRIVER_EVENT_IMAGE_DOWNLOAD_FAILED:
        VL_CDL_RegisterCdlState("CDL Download Failed");
        VL_CDL_DOWNLOAD_FAIL_NOTIFICATION  *pFailNotification;
        VL_CDL_RemoveFile(VL_CDL_ECM_INITIATED_CDL_IN_PROGRESS); // Un-Register ECM CDL Event.
        VL_CDL_RemoveFile(VL_CDL_DEV_INITIATED_SNMP_CDL_IN_PROGRESS);
        VL_CDL_Clear_Cached_FileName();

        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CDL_Callback: received  event VL_CDL_DRIVER_EVENT_DOWNLOAD_FAILED \n");
        pFailNotification = (VL_CDL_DOWNLOAD_FAIL_NOTIFICATION *)nEventData;
        vlg_CDLFirmwareCodeDLStatus = DOWNLOADING_FAILED;
        if(pFailNotification->eImageSign == VL_CDL_IMAGE_SIGNED)
        {
            switch (pFailNotification->eImageType)
            {
                case VL_CDL_IMAGE_TYPE_MONOLITHIC:
                case VL_CDL_IMAGE_TYPE_FIRMWARE:
                case VL_CDL_IMAGE_TYPE_APPLICATION:
                case VL_CDL_IMAGE_TYPE_DATA:
                {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CDL_Callback: eImageType:%d eImageSign:%d eFailReason:%d \n",
                    pFailNotification->eImageType,
                    pFailNotification->eImageSign,
                    pFailNotification->eFailReason);
#ifdef DOWNLOAD_ENGINE_UTILS

                    ComDownLEngineGotResponseFromTftp(COMM_DL_TFTP_FAILED,(void *)pFailNotification);
#endif            
                    break;
                }
                default:
                CommonDownloadSendSysCntlComm(DOWNLOAD_MAX_RETRY);
                snprintf((char*)Message,sizeof(Message),"Host Image Download Failed (eCm)");
                CommonDownloadSendMsgEvent(Message,strlen(Message));
            
                CommonDownloadManager::PostCDLBootMsgDiagEvent(RMF_BOOTMSG_DIAG_MESG_CODE_ERROR, COMM_DL_BTDIAG_MAX_RETRY_EXHAUSTED);

                break;
            }
        }
        else
        {

            eCDLM_State = COM_DOWNL_IDLE;
        }
        eCDLM_State = COM_DOWNL_IDLE;
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL", "CDL_Callback: reverting Manager state to IDLE\n");
        vlg_CDLFirmwareImageStatus = IMAGE_MAX_DOWNLOAD_RETRY;        

        if(pFailNotification->eFailReason == VL_CDL_DOWNLOAD_FAIL_REASON_FILE_NOT_FOUND)
        {
            CommonDownloadSendSysCntlComm(NO_DOWNLOAD_IMAGE);
        
            //May-24-2012: File not found not notified to boot manager // CommonDownloadManager::PostCDLBootMsgDiagEvent(VL_BOOTMSG_DIAG_MESG_CODE_ERROR, COMM_DL_BTDIAG_FILE_NOT_FOUND);

            #ifdef GLI
                IARM_Bus_SYSMgr_EventData_t eventData;
                eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_FIRMWARE_DWNLD;
                eventData.data.systemStates.state =  IARM_BUS_SYSMGR_IMAGE_FWDNLD_DOWNLOAD_FAILED; // File not found.
                IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
            #endif           
        }
        else if(pFailNotification->eFailReason == VL_CDL_DOWNLOAD_FAIL_REASON_TRANSFER_FAILED)
        {
            CommonDownloadSendSysCntlComm(IMAGE_DAMAGED);

            CommonDownloadManager::PostCDLBootMsgDiagEvent(RMF_BOOTMSG_DIAG_MESG_CODE_ERROR, COMM_DL_BTDIAG_DOWNLOAD_MESSAGE_CORRUPT);
            
            #ifdef GLI
                IARM_Bus_SYSMgr_EventData_t eventData;
                eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_FIRMWARE_DWNLD;
                eventData.data.systemStates.state =  IARM_BUS_SYSMGR_IMAGE_FWDNLD_DOWNLOAD_FAILED; // Transfer Failed.
                IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
            #endif
        }

        break;
    case VL_CDL_DRIVER_EVENT_IMAGE_AUTH_SUCCESS:
        VL_CDL_RegisterCdlState("CDL Auth Succeeded");
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CDL_Callback: received  event VL_CDL_DRIVER_EVENT_IMAGE_AUTH_SUCCESS \n");
            break;

    case VL_CDL_DRIVER_EVENT_IMAGE_AUTH_FAIL:
        VL_CDL_RegisterCdlState("CDL Auth Failed");
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CDL_Callback: received  event VL_CDL_DRIVER_EVENT_IMAGE_AUTH_FAIL \n");
        eCDLM_State = COM_DOWNL_IDLE;
        vlg_CDLFirmwareImageStatus = IMAGE_CERT_FAILURE;
        //vlg_CDLFirmwareDLFailedStatus = 
        CommonDownloadSendSysCntlComm(CERT_FAILURE);
        snprintf((char*)Message,sizeof(Message),"Host Image Download Auth Failed (eCm)");
        CommonDownloadSendMsgEvent(Message,strlen(Message));
        break;
    case VL_CDL_DRIVER_EVENT_UPGRADE_STARTED:
        VL_CDL_RegisterCdlState("CDL Upgrade Started");
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CDL_Callback: received  event VL_CDL_DRIVER_EVENT_UPGRADE_STARTED \n");
        snprintf((char*)Message,sizeof(Message),"Host Image Upgrade Started.\nDo not Power Off the box (eCm)");
        CommonDownloadSendMsgEvent(Message,strlen(Message));
                //send event to HostbootMgr to update front panel "FL-ST"
                CommonDownloadManager::PostCDLBootMsgFlashWriteDiagEvent(RMF_BOOTMSG_DIAG_MESG_CODE_START);
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","KELLY, DLMgr FLASH WRITE START\n"); 
            break;
    case VL_CDL_DRIVER_EVENT_UPGRADE_COMPLETED:
        VL_CDL_RegisterCdlState("CDL Upgrade Completed");
        if((0 == FileName_TobeLoaded_Size) || (0 == strlen((const char *)FileName_TobeLoaded)))
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CDL_Callback: VL_CDL_DRIVER_EVENT_UPGRADE_COMPLETED: No information available in FileName_TobeLoaded \n");
        }
        else
        {
            VL_CDL_RegisterFlashedFile((const char *)FileName_TobeLoaded);
        }
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CDL_Callback: received  event VL_CDL_DRIVER_EVENT_UPGRADE_COMPLETED \n");
        VL_CDL_RemoveFile(VL_CDL_ECM_INITIATED_CDL_IN_PROGRESS);
        VL_CDL_RemoveFile(VL_CDL_DEV_INITIATED_SNMP_CDL_IN_PROGRESS);
        eCDLM_State = COM_DOWNL_IDLE;
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL", "CDL_Callback: reverting Manager state to IDLE\n");
        snprintf((char*)Message,sizeof(Message),"Host Image Upgrade Completed.\nDo not Power Off the box (eCm)");
        CommonDownloadSendMsgEvent(Message,strlen(Message));
                //send event to HostbootMgr to update front panel "FL-DN"
                CommonDownloadManager::PostCDLBootMsgFlashWriteDiagEvent(RMF_BOOTMSG_DIAG_MESG_CODE_PROGRESS);
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","KELLY, DlMgr FLASH WRITE DONE\n");
            break;

    case VL_CDL_DRIVER_EVENT_UPGRADE_FAILED:
            VL_CDL_RegisterCdlState("CDL Upgrade Failed");
            VL_CDL_RemoveFile(VL_CDL_ECM_INITIATED_CDL_IN_PROGRESS); // Un-Register ECM CDL Event.
            VL_CDL_RemoveFile(VL_CDL_DEV_INITIATED_SNMP_CDL_IN_PROGRESS);
            VL_CDL_Clear_Cached_FileName();
            CommonDownloadSendSysCntlComm(IMAGE_DAMAGED);
            eCDLM_State = COM_DOWNL_IDLE;
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CDL_Callback: received  event VL_CDL_DRIVER_EVENT_UPGRADE_FAILED \n");
                //send event to HostbootMgr to update front panel "FL-ER"
                CommonDownloadManager::PostCDLBootMsgFlashWriteDiagEvent(RMF_BOOTMSG_DIAG_MESG_CODE_ERROR);
            break;
    case VL_CDL_DRIVER_EVENT_REBOOTING_AFTER_UPGRADE:
            VL_CDL_RegisterCdlState("CDL Rebooting After Upgrade");
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CDL_Callback: received  event VL_CDL_DRIVER_EVENT_REBOOTING_AFTER_UPGRADE \n");
            snprintf((char*)Message,sizeof(Message),"Rebooting the box (eCm)");
            CommonDownloadSendMsgEvent(Message,strlen(Message));
                //send event to HostbootMgr to update front panel "REBOOT"
                CommonDownloadManager::PostCDLBootMsgFlashWriteDiagEvent(RMF_BOOTMSG_DIAG_MESG_CODE_COMPLETE);
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","KELLY, DlMgr FLASH WRITE REBOOT\n");
            break;

    case VL_CDL_DRIVER_EVENT_IMAGE_DOWNLOAD_PROGRESS:
        VL_CDL_DOWNLOAD_PROGRESS_NOTIFICATION *pSt;
        int Percentage,factor;
        pSt = (VL_CDL_DOWNLOAD_PROGRESS_NOTIFICATION*)nEventData;
        
        VL_CDL_RegisterCdlProgress(pSt->nBytesDownloaded);
        // process VL_CDL_DOWNLOAD_PROGRESS_NOTIFICATION message
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CDL_Callback: receive VL_CDL_DRIVER_EVENT_IMAGE_DOWNLOAD_PROGRESS \n");
        factor = 0;
        switch(vlg_CvtTftDLNumOfObjs)
        {
            case 1:
            case 0:
            factor = 99;
            break;
            case 2:
            factor = 50;
            break;
            case 3:
            factor = 33;
            break;
            
        }
        Percentage = (pSt->nBytesDownloaded/1000000);
        if(Percentage == 0)
        {
            Percentage = 1;
        }
        else if(Percentage >= factor)
        {
            Percentage =  factor;
        }

        Percentage = Percentage + factor*vlg_NumObjsComplete;
        if(Percentage>=99)
            Percentage = 99;
        //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," nBytesDownloaded:%d.....  Percentage:%d \n",pSt->nBytesDownloaded,Percentage);
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," nBytesDownloaded:%d \n",pSt->nBytesDownloaded);
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," percentage:%d \n", Percentage);

        CommonDownloadManager::PostCDLBootMsgDiagEvent(RMF_BOOTMSG_DIAG_MESG_CODE_PROGRESS,Percentage);

        break;
    default:
        return VL_CDL_RESULT_NOT_IMPLEMENTED;

    }//switch
    return VL_CDL_RESULT_SUCCESS;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// CommonDownloadManager::CommonDownloadSendSysCntlComm()
// Description:
//
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

int CommonDownloadSendSysCntlComm(CommonDownloadHostCmd_e command )
{

    //pfcEventCableCard         *pEvent=NULL;
    LCSM_EVENT_INFO *pEventInfo;
    unsigned char *ptr;
    //pfcEventBase *event1=NULL;

    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    rmf_osal_event_handle_t event_handle;
    rmf_osal_event_params_t event_params = {0};	


    switch(command)
    {
         case NO_DOWNLOAD_IMAGE:// =2,// Host could not find the download image.
        case DOWNLOAD_MAX_RETRY:// =3,// Download max retries attempted during code file download
        case IMAGE_DAMAGED:// = 4,// download file damaged or corrupted

        if(command == NO_DOWNLOAD_IMAGE)
         {
           vlg_CDLFirmwareImageStatus = IMAGE_MAX_DOWNLOAD_RETRY;
           vlg_CDLFirmwareDLFailedStatus = CDL_ERROR_99;
         }
         else if(command == DOWNLOAD_MAX_RETRY)
         {
           vlg_CDLFirmwareImageStatus = IMAGE_MAX_DOWNLOAD_RETRY;
           vlg_CDLFirmwareDLFailedStatus = CDL_ERROR_99;
         }
         else if(command == IMAGE_DAMAGED)
         {
           vlg_CDLFirmwareImageStatus = IMAGE_CORRUPTED;
           vlg_CDLFirmwareDLFailedStatus = CDL_ERROR_25;
         }
          vlg_CDLFirmwareCodeDLStatus = DOWNLOADING_FAILED;
	   rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(CommonDownloadHostCmd_e),(void **)&event_params.data);    
          memcpy( event_params.data, (const void *)&command, sizeof(CommonDownloadHostCmd_e) );
          event_params.data_extension = sizeof(CommonDownloadHostCmd_e);
          event_params.free_payload_fn = podmgr_freefn;
          rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_CARD_MANAGER, CardMgr_CDL_Host_Img_DL_Error, 
        		&event_params, &event_handle);		
          rmf_osal_eventmanager_dispatch_event(em, event_handle);
//        event1 = (pfcEventCardMgr*)em->new_event(new pfcEventCardMgr(PFC_EVENT_CATEGORY_CARD_MANAGER,pfcEventCardMgr::CDL_Host_Img_DL_Error,&command,sizeof(CommonDownloadHostCmd_e)));
//        em->dispatch(event1);
        break;
        case CERT_FAILURE: //= 5,// code CVC authentication failed
        //vlg_CDLFirmwareCodeDLStatus = DOWNLOADING_FAILED;
        //vlg_CDLFirmwareImageStatus = IMAGE_CERT_FAILURE;
        vlg_CDLFirmwareDLFailedStatus = CDL_ERROR_25;

//        event1 = (pfcEventCardMgr*)em->new_event(new pfcEventCardMgr(PFC_EVENT_CATEGORY_CARD_MANAGER,pfcEventCardMgr::CDL_CVT_Error));
//        em->dispatch(event1);
        		  
        rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_CARD_MANAGER, CardMgr_CDL_CVT_Error, 
        	&event_params, &event_handle);		
        rmf_osal_eventmanager_dispatch_event(em, event_handle);
        break;
        case DOWNLOAD_COMPLETE://1
        vlg_CDLFirmwareCodeDLStatus = DOWNLOADING_COMPLETE;
        vlg_CDLFirmwareDLFailedStatus = CDL_ERROR_1;
//        event1 = (pfcEventCardMgr*)em->new_event(new pfcEventCardMgr(PFC_EVENT_CATEGORY_CARD_MANAGER,pfcEventCardMgr::CDL_Host_Img_DL_Cmplete));
//        em->dispatch(event1);
        rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_CARD_MANAGER, CardMgr_CDL_Host_Img_DL_Cmplete, 
        	&event_params, &event_handle);		
        rmf_osal_eventmanager_dispatch_event(em, event_handle);
        break;
        default:
        break;
    }


    if(vlg_SysCtrlSesOpened == 0)
        return -1;
/*    ptr = (unsigned char*)malloc(sizeof(unsigned char));
    *ptr = (unsigned char)(command & 0xFF );*/
//    eventInfo.event = POD_SYSTEM_DOWNLD_CTL;
//    eventInfo.y = command;
//    eventInfo.z = sizeof(unsigned char);
    vlg_NumObjsComplete = 0;
//    pEvent = new pfcEventCableCard(PFC_EVENT_CATEGORY_SYS_CONTROL, POD_SYSTEM_DOWNLD_CTL);
/*    memcpy(&pEvent->eventInfo ,eventInfo, sizeof (LCSM_EVENT_INFO));*/
//    vlMemCpy(&pEvent->eventInfo ,&eventInfo, sizeof (LCSM_EVENT_INFO), sizeof (LCSM_EVENT_INFO));
    //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," before dispatch((pfcEventCableCard *)em->new_event(pEvent) \n");

    rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(LCSM_EVENT_INFO), (void **)&pEventInfo);
    pEventInfo->event = POD_SYSTEM_DOWNLD_CTL;
    pEventInfo->y = command;
    pEventInfo->z = sizeof(unsigned char);    	
	
    event_params.data = (void *) pEventInfo;
    event_params.data_extension = sizeof(LCSM_EVENT_INFO);
    event_params.free_payload_fn = podmgr_freefn;    		  
    rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_SYS_CONTROL, POD_SYSTEM_DOWNLD_CTL, 
    	&event_params, &event_handle);		
    rmf_osal_eventmanager_dispatch_event(em, event_handle);


//    em->dispatch((pfcEventCableCard *)em->new_event(pEvent));



    return 0;
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// CommonDownloadManager::CommonDownloadStartDeferredDownload()
// Description:
//
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

int CommonDownloadStartDeferredDownload()
{

rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    rmf_osal_event_handle_t event_handle;


    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," CommonDownloadStartDeferredDownload: Received the call \n");
    if(vlg_DefDownloadPending == FALSE)
        return COMM_DL_DEF_DL_CANCEL;

    rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_COM_DOWNL_MANAGER, CommonDownL_Deferred_DL_start, 
    	NULL, &event_handle);		
    rmf_osal_eventmanager_dispatch_event(em, event_handle);


	
    return COMM_DL_SUCESS;
}
int ComDownLDefDownloadNotify()
{

    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    rmf_osal_event_handle_t event_handle;




    rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_COM_DOWNL_MANAGER, CommonDownL_Deferred_DL_notify, 
    	NULL, &event_handle);		
    rmf_osal_eventmanager_dispatch_event(em, event_handle);
    return COMM_DL_SUCESS;

}


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// CommonDownloadManager::CommonDownloadManager()
// Description: constructor
//
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CommonDownloadManager::CommonDownloadManager(): CMThread("vlCommonDownloadManager")
{
    event_queue = NULL;
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CommonDownloadManager: CommonDownload Plugins CommonDownloadManager Constructor called \n");
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// CommonDownloadManager::CommonDownloadManager()CommonDownloadSendMsgEvent
// Description: destructor
//
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CommonDownloadManager::~CommonDownloadManager()
{
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CommonDownloadManager: CommonDownload Plugins CommonDownloadManager destructor called \n");
}
int CommonDownloadMgrSnmpCertificateStatus();
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// CommonDownloadManager::initialize()
// Description: Registers the event queue with event Manager..
//
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
int CommonDownloadManager::initialize(void)
{
  CDLMgrSnmpCertificateStatus_t CertStatus;
    // In addition to all the initialization, start the CommonDownloadMgr
    // thread which waits for events from other modules.
    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();


    rmf_osal_eventqueue_handle_t eventqueue_handle;

    rmf_osal_eventqueue_create ( (const uint8_t* ) "CommonDownloadManager",
		&eventqueue_handle);
	
    //pfcEventManager *em = pfc_kernel_global->get_event_manager();
    //pfcEventQueue   *eq = new pfcEventQueue("CommonDownloadManager");
//    ComDownLEngine  *pDLEng= new ComDownLEngine();
//    ComDownLImageUpdate *pImageUpdate = new ComDownLImageUpdate();
        vlg_pCVTValidationMgr = new CDownloadCVTValidationMgr();
    vlg_pCodeImageValidationMgr = new CCodeImageValidator();

    eCDLM_State = COM_DOWNL_IDLE;
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CommonDownloadManager: CCommonDownloadMgr::initialize()\n");
	rmf_osal_eventmanager_register_handler(
		em,
		eventqueue_handle,
		RMF_OSAL_EVENT_CATEGORY_COM_DOWNL_MANAGER);
    //em->register_handler(eq, (pfcEventCategoryType)PFC_EVENT_CATEGORY_COM_DOWNL_MANAGER);

    CHALCdl_register_callback(VL_CDL_NOTIFY_DRIVER_EVENT, (VL_CDL_VOID_CBFUNC_t)CDL_Callback, NULL);


    this->event_queue = eventqueue_handle;

    // start the processing thread
    this->start();
    //pDLEng->initialize();
    CommonDownloadInitFilePath();
    //pImageUpdate->initialize();
    
//CommonDownloadMgrSnmpCertificateStatus();
//init the NVRam Data
CommonDownloadNvRamDataInit();
int ret = sem_init( &vlg_SnmpCertStatusEvent, 0 , 0);

    if(0 != ret)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadManager::initialize: Error !! cEventCreate failed for vlg_SnmpCertStatusEvent \n");
        return -1;
    }

	
	rmf_Error err = rmf_osal_mutexNew(&vlg_MutexSnmpCertStatus);
    if(RMF_SUCCESS != err)
    	{
      RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadManager::initialize: Error !! rmf_osal_mutexNew failed for vlg_MutexSnmpCertStatus \n");
        return -1;
    	}


    //vlTestSignedImagesStoredLocally();
    return COMM_DL_SUCESS;//Mamidi:042209
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// CommonDownloadManager::is_default()
// Description:
//
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
int CommonDownloadManager::is_default ()
{
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CommonDownloadManager::is_default\n");
    return 1;
}



void CommonDownloadManager::PostCDLBootMsgDiagEvent(long msgCode, int data)
{
    //pfcEventManager *em = pfc_kernel_global->get_event_manager();
    //pfcEventBase *pBootMsgEvent = NULL;

rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    rmf_osal_event_handle_t event_handle;
    rmf_osal_event_params_t event_params = {0};

	
#ifdef CDL_BOOT_MGR
    RMF_BOOTMSG_DIAG_EVENT_INFO *pBootMsgEventInfo;
    rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(RMF_BOOTMSG_DIAG_EVENT_INFO), (void **)&pBootMsgEventInfo);
    if(NULL == pBootMsgEventInfo)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","%s: LINE %d . OUT OF MEMORY.\n", __FUNCTION__, __LINE__);
        return;
    }

/*
    pBootMsgEvent = new pfcEventBase(PFC_EVENT_CATEGORY_BOOTMSG_DIAG, RMF_BOOTMSG_DIAG_EVENT_TYPE_CDL, NULL, pBootMsgEventInfo, 0, NULL);
    if(NULL == pBootMsgEvent)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","%s: LINE %d . OUT OF MEMORY.\n", __FUNCTION__, __LINE__);
        return;
    }

*/
   pBootMsgEventInfo->msgCode = (RMF_BOOTMSG_DIAG_MESG_CODE) msgCode; 
   pBootMsgEventInfo->data = data;
 //  em->dispatch(em->new_event(pBootMsgEvent));


   event_params.priority = 0; //Default priority;
    event_params.data = (void *)pBootMsgEventInfo;
    event_params.data_extension = 0;
    event_params.free_payload_fn = podmgr_freefn;

rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_BOOTMSG_DIAG, RMF_BOOTMSG_DIAG_EVENT_TYPE_CDL, 
		&event_params, &event_handle);

    rmf_osal_eventmanager_dispatch_event(em, event_handle);


   
#endif   
   PostBootEvent(RMF_BOOTMSG_DIAG_EVENT_TYPE_CDL, (RMF_BOOTMSG_DIAG_MESG_CODE) msgCode, data);
}

void CommonDownloadManager:: PostCDLBootMsgFlashWriteDiagEvent(long msgCode)
{
    if(RMF_BOOTMSG_DIAG_MESG_CODE_START == msgCode)
    {
        fpEnableUpdates(0); // MOT7425-8557 : 1 is re-enable updates.
    }
    else{
      fpEnableUpdates(1);
    }
    PostBootEvent (RMF_BOOTMSG_DIAG_EVENT_TYPE_FLASH_WRITE, (RMF_BOOTMSG_DIAG_MESG_CODE) msgCode, 0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// CommonDownloadGetTLVTypeLen()
// Description:
//
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
static unsigned short CommonDownloadGetTLVTypeLen(unsigned char *pData,unsigned short size,unsigned char **pRetData, unsigned short *pSize)
{
    unsigned short TLVTypeLen;
    // returns the TLV Len and increment the pointer

    TLVTypeLen = (unsigned short)( ((*pData) << 8) | *(pData + 1) );

    *pSize = size - sizeof(unsigned short);
    *pRetData =  pData + sizeof(unsigned short);
    return TLVTypeLen;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// CommonDownloadManager::run()
// Description: Common download manager thread to process the events posted to it..
//        This thread is responsible for handling the all the triggers posted to it.
//        The co-ordination with all other modules for Common download is done
//        in this thread.
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void CommonDownloadManager::run()
{

    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    rmf_osal_event_handle_t event_handle;
    rmf_osal_event_params_t event_params = {0};
    rmf_osal_event_category_t event_category;
    uint32_t event_type;





    //pfcEventManager *em = pfc_kernel_global->get_event_manager();
    //pfcEventBase *event=NULL;
    CDLMgrSnmpCertificateStatus_t CertStatus;

    int    iRet;
    VL_CDL_DOWNLOAD_STATUS_QUESTION downloadStatus;
    char Message[512];
    char FileName[64] = "/mnt/nfs/signedeval.img";
    int len = strlen(FileName);
//    cSleep(1000*10);// This sleep for Java coming up
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CommonDownloadManager::Run()\n");
    rmf_osal_threadSleep(1000* 10,0);
    CommonDownloadMgrSetPublicKeyAndCvc();
    
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," ########## CommonDownloadManager::run ####### \n");
    
#if 0
    cSleep(1000 * 10);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","########################## calling CommonDownloadMgrSnmpCertificateStatus \n");
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","########################## calling CommonDownloadMgrSnmpCertificateStatus \n");
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","########################## calling CommonDownloadMgrSnmpCertificateStatus \n");
 CommonDownloadMgrSnmpCertificateStatus(&CertStatus);
 RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","########################## After calling CommonDownloadMgrSnmpCertificateStatus \n");
 #endif

//init the NVRam Data
    vlg_ImageUpgradeState = 0;
     iRet  = CHALCdl_notify_cdl_mgr_event(VL_CDL_MANAGER_EVENT_get_UPGRADE_STATUS,(unsigned long) &downloadStatus);
    if(VL_CDL_RESULT_SUCCESS != iRet)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadManager:############# CHALCdl_notify_cdl_mgr_event retunred Error:0x%X for VL_CDL_MANAGER_EVENT_get_UPGRADE_STATUS\n",iRet);
    }
    else
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CommonDownloadManager:############# Success !!  for VL_CDL_MANAGER_EVENT_get_UPGRADE_STATUS\n");


        if(VL_CDL_DOWNLOAD_STATUS_BOOTUP_AFTER_SUCCESSFUL_UPGRADE == downloadStatus.eDownloadStatus)
        {
            
            vlg_ImageUpgradeState = DOWNLOAD_COMPLETE;// successFull upgrade
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CommonDownloadManager: Received VL_CDL_DOWNLOAD_STATUS_BOOTUP_AFTER_SUCCESSFUL_UPGRADE\n");
            iRet = CHALCdl_notify_cdl_mgr_event(VL_CDL_MANAGER_EVENT_SET_UPGRADE_SUCCEEDED, 0);
            if(VL_CDL_RESULT_SUCCESS != iRet)
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadManager:############# CHALCdl_notify_cdl_mgr_event retunred Error:0x%X for VL_CDL_MANAGER_EVENT_SET_UPGRADE_SUCCEEDED\n",iRet);
            }
            else
            {
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CommonDownloadManager:############# Success !!  for VL_CDL_MANAGER_EVENT_SET_UPGRADE_SUCCEEDED\n");
                CommonDownLUpdateCodeAccessTime();


            }
            VL_CDL_RegisterCdlState("Boot up after successful upgrade");
        }
        else if(VL_CDL_DOWNLOAD_STATUS_BOOTUP_AFTER_FAILED_UPGRADE == downloadStatus.eDownloadStatus)
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CommonDownloadManager: Received VL_CDL_DOWNLOAD_STATUS_BOOTUP_AFTER_FAILED_UPGRADE\n");
            //vlg_UpgradeFailedState = 1;
            vlg_ImageUpgradeState = REBOOT_MAX_RETRY;// reboot max retry
            iRet = CHALCdl_notify_cdl_mgr_event(VL_CDL_MANAGER_EVENT_SET_UPGRADE_FAILED, 0);
            if(VL_CDL_RESULT_SUCCESS != iRet)
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadManager:############# CHALCdl_notify_cdl_mgr_event retunred Error:0x%X for VL_CDL_MANAGER_EVENT_SET_UPGRADE_FAILED\n",iRet);
            }
            else
            {
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CommonDownloadManager:############# Success !!  for VL_CDL_MANAGER_EVENT_SET_UPGRADE_FAILED\n");


            }
            VL_CDL_RegisterCdlState("Boot up after failed upgrade");
        }
        else if(VL_CDL_DOWNLOAD_STATUS_IDLE == downloadStatus.eDownloadStatus)
        {
            vlg_ImageUpgradeState = 0;
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CommonDownloadManager: Received VL_CDL_DOWNLOAD_STATUS_IDLE\n");
            VL_CDL_RegisterCdlState("Normal Boot Up");
        }
    }
    
    vlg_pFilePath = CommonDownloadGetFilePath();
//vlTestMfrNvRamRegisters();
#if 0// commented tempararly because of the Pace nvram issue...
    CommonDownLClearCodeAccessUpgrTime();
#endif

    while (1)
    {
       int result = 0;
       RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CommonDownloadManager::Run() Waiting for PFC Events\n");

        // no data is passed for this event
       //event = event_queue->get_next_event();

		rmf_osal_eventqueue_get_next_event( event_queue,
		&event_handle, &event_category, &event_type, &event_params);
		
       int iRet;


       switch (event_category)
       {
        case RMF_OSAL_EVENT_CATEGORY_COM_DOWNL_MANAGER:
        {
           RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","Received event PFC_EVENT_CATEGORY_COM_DOWNL_MANAGER \n");
          // All the common download events are handled here.
           switch (event_type)
           {
            case CommonDownL_Sys_Ctr_Ses_Open:
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL",">>>>>>>>> CommonDownL_Sys_Ctr_Ses_Open Received \n");
                vlg_SysCtrlSesOpened = 1;
                if(vlg_ImageUpgradeState == REBOOT_MAX_RETRY)
                {
                      RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL",">>>>>>>> Sending REBOOT_MAX_RETRY to card \n");
                    vlg_CDLFirmwareImageStatus = IMAGE_MAX_REBOOT_RETRY;

                    CommonDownloadSendSysCntlComm(REBOOT_MAX_RETRY);
                }    
                else if(vlg_ImageUpgradeState == DOWNLOAD_COMPLETE)
                {
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL",">>>>>>>> Sending DOWNLOAD_COMPLETE to card \n");
                    CommonDownloadSendSysCntlComm(DOWNLOAD_COMPLETE);
                }
                //vlTestSignedImagesStoredLocally();
                vlg_ImageUpgradeState = 0;
                break;
            case CommonDownL_Sys_Ctr_Ses_Close:

                vlg_SysCtrlSesOpened = 0;
                break;
            case CommonDownL_CVT_OOB:
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," Dumping the CVT with CVC \n");
#ifdef SNMP_ONLY
break;
#else 
                //    PrintBuffer((unsigned char *)event->get_data(),(unsigned long)event->data_item_count());
                {
                // Common download  eSTB ( CVT )trigger from OOB FDC
                // The CVT type1 and type2  are handled
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CommonDownloadManager::Run() Received CVT EVENT from Card Manager CommonDownL_CVT_OOB <<<<<<<<<<<\n\n");

                if(eCDLM_State != COM_DOWNL_IDLE &&  eCDLM_State != COM_DOWNL_CVT_PROC_AND_VALIDATION)
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadManager::State is not idle <<<<<\n\n");
                    break;
                }

				
                iRet = CommonDownloadVerifyCVT((unsigned char *)event_params.data,(unsigned long) event_params.data_extension,0);
                if(iRet != COMM_DL_SUCESS)
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadManager::CommonDownloadVerifyCVT returned Error:0x%X <<<<<\n\n",iRet);
                    break;
                }
                //This function  is for tesing purpose only... will be removed later
        //        testCVT((char*)event->get_data(), event->data_item_count());
                break;
            }
#endif

            case CommonDownL_CVT_DSG:
#ifdef SNMP_ONLY
break;
#else    
            {
                unsigned char *pMpeg;
                unsigned char *pPKCS7;
                unsigned char *pCVT2;
                int iMpegSize;
                int iPKCS7Size;
                int iCVTSize;
                unsigned char table_ID, section_synctax_indicator,private_indicator,reserved;
                unsigned short private_section_length=0;
                pkcs7SignedDataInfo_t SignerInfo;
                cvcinfo_t cvcInfos;
             
                if(eCDLM_State != COM_DOWNL_IDLE &&  eCDLM_State != COM_DOWNL_CVT_PROC_AND_VALIDATION)
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadManager::State is not idle <<<<<\n\n");
                    break;
                }


                // Common download  eSTB or CVT trigger from DSG
                // MPEG section{ PKCS # 7 { CVT2 } ) received ...
                // CVT2 is in PKCS # 7 structure and PKCS#7 is in MPEG section.
                // Need to extract the CVT2 from Mpeg Section.
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","\n CommonDownloadManager::Run() Received EVENT from Card Manager CommonDownL_CVT_DSG <<<<<<<<<<<\n\n");

                pMpeg = (unsigned char *)event_params.data;
                iMpegSize = event_params.data_extension;

                //PrintBuff(pMpeg,iMpegSize);

                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CommonDownloadManager::Run()  pMpeg:0x%X iMpegSize:%d \n",pMpeg,iMpegSize);
                if(pMpeg == NULL || iMpegSize == 0)
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadManager::Run() Error !! pMpeg:0x%X iMpegSize:%d \n",pMpeg,iMpegSize);
                    break;
                }
                if(eCDLM_State != COM_DOWNL_IDLE &&  eCDLM_State != COM_DOWNL_CVT_PROC_AND_VALIDATION)
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadManager::State is not idle <<<<<\n\n");
                    break;
                }

#if 0
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," MPEG Section Details..0x%X 0x%X 0x%X\n",*pMpeg,*(pMpeg + 1),*(pMpeg + 2));
                table_ID = *pMpeg++;
                section_synctax_indicator = *pMpeg++;
                private_indicator = (section_synctax_indicator & 0x80) >> 7;
                reserved = (section_synctax_indicator & 0x30) >> 4;
                private_section_length = ((section_synctax_indicator &0x0F) << 8) | (*pMpeg++);
                section_synctax_indicator = (section_synctax_indicator & 0x40) >> 6 ;

                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","table_ID:0x%X\n section_synctax_indicator:0x%X \n",table_ID,section_synctax_indicator);
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","private_indicator:0x%X\n reserved:0x%X\n",private_indicator,reserved);
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","private_section_length:0x%X\n ",private_section_length);

#endif
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL",">>>>>>>>>>>>>>>>>>>>>>> signedCVT >>>>>>>>>>>>>>>>>>>>>>>>>>> \n");
                pPKCS7 = pMpeg;
                iPKCS7Size = iMpegSize;



                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," calling verifySignedContent() PKCS size:%d :0x%X\n",iPKCS7Size,iPKCS7Size);
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," @@@@@@@@@@@@@@@@@@@@ \n");
#ifdef PKCS_UTILS
                //this function... verifies the PKCS#7 signed data
                iRet =  verifySignedContent((char  *)pPKCS7,(int)iPKCS7Size,(char)0);
                if( 0== iRet)
                {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","\n SUCCESS verifySignedContent of PKCS#7 CVT signed data\n");

                }
                else
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","\nPKCS#7 CVT signed data Verification FAILED Ignore the trigger \n");
                    PostBootEvent(RMF_BOOTMSG_DIAG_EVENT_TYPE_CDL, RMF_BOOTMSG_DIAG_MESG_CODE_ERROR, 0xF7);
                    break;
                }


                iRet = getSignedContent((char*)pPKCS7,(int)iPKCS7Size,(char)0,(char**)&pCVT2,(int*)&iCVTSize);

                if(iRet != 0)
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","\nPKCS#7 CVT getSignedContent() returned  ERROR Ignore the trigger \n");
                    PostBootEvent(RMF_BOOTMSG_DIAG_EVENT_TYPE_CDL, RMF_BOOTMSG_DIAG_MESG_CODE_ERROR, 0xF7);
                    break;
                }
#endif                
                //    testCVT((char*)pCVT2, iCVTSize);
                iRet = CommonDownloadVerifyCVT(pCVT2, iCVTSize,0);
                if(iRet != 0)
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL"," FAILED >>>> CommonDownloadVerifyCVT  \n");
                    break;
                }

                break;
            }
#endif
            case CommonDownL_Deferred_DL_start:
#ifdef SNMP_ONLY
break;
#else    
            {         
                // This Event is notified from the Monitor Application.
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," CommonDownL_Deferred_DL_start: Event received \n");
                if( (vlg_pDefDLCVT != NULL) &&  (vlg_DefCVTSize !=0) && (vlg_DefDownloadPending == TRUE) )
                {
                    iRet = CommonDownloadVerifyCVT(vlg_pDefDLCVT,vlg_DefCVTSize,1);
                    vlg_DefCVTSize = 0;
                    vlg_pDefDLCVT = NULL;
                    vlg_DefDownloadPending = FALSE;
                    if(iRet != 0)
                    {
                        break;
                    }
                }
                break;
            }
#endif
            case CommonDownL_CVC_ConfigFile:
#ifdef SNMP_ONLY
break;
#else 
            {
                //Cancel the pending deferred download
                vlg_DefDownloadPending = FALSE;

                if(eCDLM_State != COM_DOWNL_IDLE &&  eCDLM_State != COM_DOWNL_CVT_PROC_AND_VALIDATION)
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadManager::State is not idle <<<<<\n\n");

                    break;
                }
                break;
            }
#endif			
            case CommonDownL_CVC_SNMP:
            {
                //Cancel the pending deferred download
                vlg_DefDownloadPending = FALSE;
                if(eCDLM_State != COM_DOWNL_IDLE &&  eCDLM_State != COM_DOWNL_CVT_PROC_AND_VALIDATION)
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadManager::State is not idle <<<<<\n\n");

                    break;
                }
                break;
            }
            case CommonDownL_CVT_SNMP:
#ifdef SNMP_ONLY
break;
#else 
            {             
                //Cancel the pending deferred download
                vlg_DefDownloadPending = FALSE;
                if(eCDLM_State != COM_DOWNL_IDLE &&  eCDLM_State != COM_DOWNL_CVT_PROC_AND_VALIDATION)
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadManager::State is not idle <<<<<\n\n");

                    break;
                }
                break;
            }
#endif
            case CommonDownL_CVT_Validated:
#ifdef SNMP_ONLY
break;
#else   
            {
                CVTValidationResult_t *pCVTVldRest;
                unsigned long size;
              
                FileName_TobeLoaded_Size = 0;

		  pCVTVldRest = (CVTValidationResult_t*)event_params.data;
		  size =event_params.data_extension;

                vlg_CvtTftDLNumOfObjs = 0;
                vlg_CvtTftDLObjNum = 0;
                if(pCVTVldRest == NULL || size != sizeof(CVTValidationResult_t) )
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_CVT_Validated; Error !! \n");
                    //em->delete_event(event, result);
                    break;
                }
                if(pCVTVldRest->bCVTValid)
                {
                    if(pCVTVldRest->pCvt->getCVT_Type() == CVT_T1V1)
                    {
                        //cvt_t1v1_download_data_t* pCvt1 = (cvt_t1v1_download_data_t*)pCVTVldRest->pCvt;
                        cvt_t1v1_download_data_t* pCvt1 = (cvt_t1v1_download_data_t*)pCVTVldRest->pCvt->getCVT_DataForUpdate();

                        FileName_TobeLoaded_Size = pCvt1->codefile_data.len;
                        if((FileName_TobeLoaded_Size + 1) > sizeof(FileName_TobeLoaded) )
                        {
                            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_CVT_Validated; CVT Error FileName_TobeLoaded_Size:%d > %d!! \n",(FileName_TobeLoaded_Size + 1), sizeof(FileName_TobeLoaded));
                            break;
                        }
                        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","FileName_TobeLoaded_Size:%d pCvt2->codefile_data.name:0x%X \n",FileName_TobeLoaded_Size,pCvt1->codefile_data.name);
                        memcpy(FileName_TobeLoaded,pCvt1->codefile_data.name,FileName_TobeLoaded_Size);
                        FileName_TobeLoaded[FileName_TobeLoaded_Size] = 0;
                        //free(pCvt1);
                    }
                    else if(pCVTVldRest->pCvt->getCVT_Type() == CVT_T2Vx)
                    {
                        if(pCVTVldRest->pCvt->getCVT_Version() == 1)
                        {
                            //cvt_t2v1_download_data_t* pCvt2 = (cvt_t2v1_download_data_t*)pCVTVldRest->pCvt;
                            cvt_t2v1_download_data_t* pCvt2 = (cvt_t2v1_download_data_t*)pCVTVldRest->pCvt->getCVT_DataForUpdate();

                            FileName_TobeLoaded_Size = pCvt2->codefile_data.len;
                            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","FileName_TobeLoaded_Size:%d pCvt2->codefile_data.name:0x%X \n",FileName_TobeLoaded_Size,pCvt2->codefile_data.name);
                            if(pCvt2->codefile_data.name)
                                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","pCvt2->codefile_data.name:%s \n",pCvt2->codefile_data.name);
                            else
                            {
                                FileName_TobeLoaded_Size = 0;
                            }
                            memcpy(FileName_TobeLoaded,pCvt2->codefile_data.name,FileName_TobeLoaded_Size);
                            FileName_TobeLoaded[FileName_TobeLoaded_Size] = 0;
                            //free(pCvt2);
                        }

                    }
                    //CVT is valid
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CommonDownL_CVT_Validated: CVT is Valid Received \n");
                    if(pCVTVldRest->bDownloadStart)
                    {

                        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CommonDownL_CVT_Validated: Download Start Recieved and \nNotify the Download start event to Download Engine\n");

                        eCDLM_State = COM_DOWNL_CVT_PROC_VALIDATED;//state changed to cvt validated

                        vlg_DefDownloadPending = FALSE;
                        vlg_CDLFirmwareCodeDLStatus = DOWNLOADING_STARTED;
                         CommonDownloadSendSysCntlComm(DOWNLOAD_STARTED);
                
                        //May-24-2012: File not found not notified to boot manager // CommonDownloadManager::PostCDLBootMsgDiagEvent(RMF_BOOTMSG_DIAG_MESG_CODE_START, 0);

                        CHALCdl_notify_cdl_mgr_event(VL_CDL_MANAGER_EVENT_DOWNLOAD_STARTED, 0);

                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","pfcEventComDownL:pCVTVldRest->pCvt:0x%08X \n",pCVTVldRest->pCvt);

                        if(vlg_JavaCTPEnabled == FALSE)
                        {
			
				event_params.data = pCVTVldRest->pCvt;
				event_params.data_extension = sizeof(TableCVT);
				rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_DOWNL_ENGINE, CommonDownL_Engine_start, 
				&event_params, &event_handle);
    				rmf_osal_eventmanager_dispatch_event(em, event_handle);
							
                        }
                        free(pCVTVldRest);
                        continue;

                    }
                    else
                    {
                        eCDLM_State = COM_DOWNL_IDLE;//state change to idle
                    }
                }
                else
                {
                    eCDLM_State = COM_DOWNL_IDLE;//state change to idle
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CommonDownL_CVT_Validated: Invalid  CVT Received \n");
                }
                //free CVT... here
                //free(pCVTVldRest->pCvt);
                // and free pCVTVldRest
                CommonDownloadCVTFree(pCVTVldRest->pCvt);
                free(pCVTVldRest);
                break;
            }
#endif
            case CommonDownL_Engine_started:
            {
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," >>>>>>>>>>>> Download Started >>>>>>>>>>>> \n");
                snprintf((char*)Message,sizeof(Message),"Host Image Download Started \nDo not power OFF");
                CommonDownloadSendMsgEvent(Message,strlen(Message));

                eCDLM_State = COM_DOWNL_CODE_IMAGE_DOWNLOAD_STARTED;
                vlg_CDLFirmwareCodeDLStatus = DOWNLOADING_STARTED;
            //    CommonDownloadSendSysCntlComm(DOWNLOAD_STARTED);
            //    CommonDownloadManager::PostCDLBootMsgDiagEvent(RMF_BOOTMSG_DIAG_MESG_CODE_START, 1);
                
                break;
            }
            case CommonDownL_Engine_Error:
            {
                int *pError;
                CommonDownEngineErrorCodes_e Error;
                CommonDownloadHostCmd_e Code;

                //eCDLM_State = COM_DOWNL_IDLE;

                
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," CommonDownL_Engine_Error: Received Error from CDL ENgine \n");
                pError = ( int *)event_params.data;
                if ( pError == NULL )	break;
                Error = (CommonDownEngineErrorCodes_e)*pError;
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CommonDownL_Engine_Error: Error Code :%d \n",Error);
                free(pError);
                pError = NULL;
                //CHALCdl_notify_cdl_mgr_event(VL_CDL_MANAGER_EVENT_DOWNLOAD_FAILED, 0);

                switch(Error)
                {
                    case DOWNLOAD_ENGINE_DOWNLOAD_START_TUNE:
                        snprintf((char*)Message,sizeof(Message),"Star Tuning for Host Image.\nDo not power OFF");
                        CommonDownloadSendMsgEvent(Message,strlen(Message));
                        break;
                    case DOWNLOAD_ENGINE_DOWNLOAD_TUNE_DONE_WAITING_FOR_IMAGE:
                        snprintf((char*)Message,sizeof(Message),"Tuning Done, Waiting For Host Image.\nDo not power OFF");
                        CommonDownloadSendMsgEvent(Message,strlen(Message));
                        break;
                    case DOWNLOAD_ENGINE_NO_IMAGE_FOUND:
                        snprintf((char*)Message,sizeof(Message),"No Host Image Found");
                        CommonDownloadSendMsgEvent(Message,strlen(Message));
                        Code = NO_DOWNLOAD_IMAGE;
                    
                        CommonDownloadManager::PostCDLBootMsgDiagEvent(RMF_BOOTMSG_DIAG_MESG_CODE_ERROR, COMM_DL_BTDIAG_FILE_NOT_FOUND);

                        break;
                    case DOWNLOAD_ENGINE_IMAGE_DAMAGED:
                        //Message = "Downloaded Host Image Damaged";
                        snprintf((char*)Message,sizeof(Message),"Downloaded Host Image Damaged");
                        CommonDownloadSendMsgEvent(Message,strlen(Message));
                        Code = IMAGE_DAMAGED;

                        CommonDownloadManager::PostCDLBootMsgDiagEvent(RMF_BOOTMSG_DIAG_MESG_CODE_ERROR, COMM_DL_BTDIAG_DOWNLOAD_MESSAGE_CORRUPT);

                        break;
                    case DOWNLOAD_ENGINE_DOWNLOAD_RETRY_TIMEOUT:
                        //Message = "Host Image Download Max Retry Occured";
                        snprintf((char*)Message,sizeof(Message),"Host Image Download Max Retry Occured");
                        CommonDownloadSendMsgEvent(Message,strlen(Message));
                        Code = DOWNLOAD_MAX_RETRY;
                        
                        CommonDownloadManager::PostCDLBootMsgDiagEvent(RMF_BOOTMSG_DIAG_MESG_CODE_ERROR, COMM_DL_BTDIAG_MAX_RETRY_EXHAUSTED);

                        break;
                    case DOWNLOAD_ENGINE_DOWNLOAD_TUNE_FAILED:
                        //Message = "Host Image Download Tune Failed";
                        snprintf((char*)Message,sizeof(Message),"Host Image Download Tune Failed");
                        CommonDownloadSendMsgEvent(Message,strlen(Message));
                        Code = NO_DOWNLOAD_IMAGE;

                        CommonDownloadManager::PostCDLBootMsgDiagEvent(RMF_BOOTMSG_DIAG_MESG_CODE_ERROR, COMM_DL_BTDIAG_GENERAL_ERROR);

                        break;
                    case COM_DOWNL_DSMCC_MAX_EXCEED:
                        //Message = "Host Image Download Tune Failed";
                        snprintf((char*)Message,sizeof(Message),"Host Image Exceeded the DSMCC Max num of blocks(65536)");
                        CommonDownloadSendMsgEvent(Message,strlen(Message));
                        Code = IMAGE_DAMAGED;

                        CommonDownloadManager::PostCDLBootMsgDiagEvent(RMF_BOOTMSG_DIAG_MESG_CODE_ERROR, COMM_DL_BTDIAG_GENERAL_ERROR);
                        break;
                    default:
                        //Message = "No Host Image found";
                        snprintf((char*)Message,sizeof(Message),"No Host Image Found");
                        CommonDownloadSendMsgEvent(Message,strlen(Message));
                        Code = NO_DOWNLOAD_IMAGE;
                        CommonDownloadManager::PostCDLBootMsgDiagEvent(RMF_BOOTMSG_DIAG_MESG_CODE_ERROR, COMM_DL_BTDIAG_FILE_NOT_FOUND);
                        break;
                }

                if(Error < DOWNLOAD_ENGINE_DOWNLOAD_TUNE_DONE_WAITING_FOR_IMAGE)
                {
                    eCDLM_State = COM_DOWNL_IDLE;
                    vlg_CDLFirmwareImageStatus = IMAGE_MAX_DOWNLOAD_RETRY;
                    CommonDownloadSendSysCntlComm(Code);
                    vlg_CDLFirmwareCodeDLStatus = DOWNLOADING_FAILED;
                    CHALCdl_notify_cdl_mgr_event(VL_CDL_MANAGER_EVENT_DOWNLOAD_FAILED, 0);
                    CHALCdl_notify_cdl_mgr_event(VL_CDL_MANAGER_EVENT_DOWNLOAD_COMPLETED, 0);
                }

                break;
            }
            
            case DownLEngine_DOWNLOAD_PROGRESS:
            {
                DownLEngine_DownLoadProgress *pData = (DownLEngine_DownLoadProgress*)event_params.data;
                if (NULL != pData)
                {
                    CommonDownloadManager::PostCDLBootMsgDiagEvent(RMF_BOOTMSG_DIAG_MESG_CODE_PROGRESS, (pData->Num_Acquired*100)/pData->Num_Total);
                }

                break;
            }

            case CommonDownL_Engine_complete:
            {
                char *pPKCS7CodeImage;
                char pCotentData[64];// = "/opt/image.bin";
                char pImage[64];// = "/opt/UpgradeImage.bin";
                char *pData,*pData1,*pTemp;
                unsigned short sTLVType,sLength;
                unsigned long Pkcs7CodeImageSize;
                unsigned long ContentSize,DataSize=0;
                unsigned long MaxTLVSize=0x1000*3;
                size_t readLen,writeLen;
                char *pUpgradeImagePathName;
            //    unsigned char TLVType;
                unsigned char pTVData[4];
                FILE * fp1,*fp2;
                CommonDownLImage_t *pDLImage;
                VL_CDL_IMAGE ImageNamePath;
                int flag = 0;
                //Message = "Host Image Download Complete";
                snprintf((char*)Message,sizeof(Message),"Host Image Download Complete");
                CommonDownloadSendMsgEvent(Message,strlen(Message));

                // Posting BootDiag Event
                //CommonDownloadManager::PostCDLBootMsgDiagEvent(RMF_BOOTMSG_DIAG_MESG_CODE_COMPLETE, 0);

                CHALCdl_notify_cdl_mgr_event(VL_CDL_MANAGER_EVENT_DOWNLOAD_COMPLETED, 0);
                {
                    int istrLWrote=0;
                    char imageFile[40] = "Extractedimage.img";
                    char UpgradeFile[40] = "UpgradeImage.img";

                    if( NULL == (vlg_pFilePath = CommonDownloadGetFilePath()) )
                    {
                        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_Engine_complete; Error !! in Getting File Path Error !! \n");
                        eCDLM_State = COM_DOWNL_IDLE;
                        break;
                    }
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CommonDownL_Engine_complete: Got File Path:%s \n", vlg_pFilePath);

                    memset( pCotentData, 0, sizeof(pCotentData));
                    istrLWrote = snprintf( pCotentData, sizeof(pCotentData),"%s", vlg_pFilePath);
                    if(istrLWrote > 0)
                    {
                        istrLWrote = snprintf( (pCotentData+istrLWrote), (sizeof(pCotentData) - istrLWrote), "%s", imageFile);
                        if(istrLWrote < 0)
                        {
                            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","Error!!! imageFile name Length wrote :%d \n",istrLWrote);
                        }
                    }
                    else
                    {
                        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","Error!!! vlg_pFilePath name Length wrote:%d \n",istrLWrote);
                    }

                    memset( pImage, 0, sizeof(pImage));
                    istrLWrote = snprintf( pImage, sizeof(pImage),"%s", vlg_pFilePath);
                    if(istrLWrote > 0)
                    {
                        istrLWrote = snprintf( (pImage + istrLWrote), (sizeof(pImage) - istrLWrote), "%s", UpgradeFile);
                        if(istrLWrote < 0)
                        {
                            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","Error!!! UpgradeFile name Length wrote :%d \n",istrLWrote);
                        }
                    }
                    else
                    {
                        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","Error!!! vlg_pFilePath name Length wrote:%d \n",istrLWrote);
                    }

                   RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL",">>>>>>>>>>Extracted image File Path pCotentData:%s<<<<<<<<<<<< path Len:%d\n",pCotentData,strlen(pCotentData));
                   RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL",">>>>>>>>>>File to be Upgraded Path pImage:%s<<<<<<<<<<<< path Len:%d\n",pImage,strlen(pImage));

                }




                //CVT is valid
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CommonDownL_Engine_complete: Image download is complete event received \n");


                pPKCS7CodeImage = ( char *)event_params.data;
                Pkcs7CodeImageSize = (unsigned long)event_params.data_extension;
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","\n\n################## DOWNLOAD COMPLETE RECEIVED ############### \n");
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","pPKCS7CodeImage:%s Pkcs7CodeImageSize:%d\n",pPKCS7CodeImage,Pkcs7CodeImageSize);
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","################## DOWNLOAD COMPLETE RECEIVED ############### \n\n");
                //break;
                if(pPKCS7CodeImage == NULL || Pkcs7CodeImageSize == 0)
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_Engine_complete; Error !!pPKCS7CodeImage:0x%X Pkcs7CodeImageSize:%d\n",pPKCS7CodeImage,Pkcs7CodeImageSize);
                    eCDLM_State = COM_DOWNL_IDLE;
                    CommonDownloadSendSysCntlComm(IMAGE_DAMAGED);
                    vlg_CDLFirmwareCodeDLStatus = DOWNLOADING_FAILED;
                    CommonDownloadManager::PostCDLBootMsgDiagEvent(RMF_BOOTMSG_DIAG_MESG_CODE_ERROR, COMM_DL_BTDIAG_GENERAL_ERROR);
                    //em->delete_event(event, result);
                    break;
                }
                //CommonDownloadSendSysCntlComm(DOWNLOAD_COMPLETE);
                //Status changed to Image validation
                eCDLM_State = COM_DOWNL_CODE_IMAGE_PKCS7_VALIDATION;




                snprintf((char*)Message,sizeof(Message),"Verifying the Host Image\n Do Not Power OFF");
                CommonDownloadSendMsgEvent(Message,strlen(Message));
                //Verify the the PKCS signed code image signature
                iRet = vlg_pCodeImageValidationMgr->processCodeImage((char  *)pPKCS7CodeImage,(int)Pkcs7CodeImageSize,COMM_DL_MONOLITHIC_IMAGE);

                if( iRet == 0)
                {
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","\n SUCCESS verifySignedContent\n");
                    vlg_CDLFirmwareImageStatus = IMAGE_AUTHORISED;
                    //Message = "Downloaded Host Image  Verification Success";
                    snprintf((char*)Message,sizeof(Message),"Downloaded Host Image\nVerification Success");
                    CommonDownloadSendMsgEvent(Message,strlen(Message));
                    //CommonDownloadSendSysCntlComm(DOWNLOAD_COMPLETE);
                }
                else
                {
                    vlg_CDLFirmwareImageStatus = IMAGE_CERT_FAILURE;
                    vlg_CDLFirmwareCodeDLStatus = DOWNLOADING_FAILED;
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","\nPKCS#7 CVT signed data Verification FAILED Ignore the trigger \n");
                    //Free the PKCS7 CVT memory and delete the event
                    //free((unsigned char *)event->get_data());
                    //em->delete_event(event, result);
                    //Message = "Downloaded Host Image  Verification Failed";
                    snprintf((char*)Message,sizeof(Message),"Downloaded Host Image\nVerification Failed");
                    CommonDownloadSendMsgEvent(Message,strlen(Message));
                    eCDLM_State = COM_DOWNL_IDLE;
                    CommonDownloadSendSysCntlComm(IMAGE_DAMAGED);

                    break;
                }
                ContentSize= strlen(pCotentData);
#ifdef PKCS_UTILS                
                // Get the code image data from the PKCS7
                iRet = getSignedContent(pPKCS7CodeImage,Pkcs7CodeImageSize,(char)1,(char **)&pCotentData,(int*)&ContentSize);
                if( iRet != 0)
                {
                    vlg_CDLFirmwareImageStatus = IMAGE_CORRUPTED;
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_Engine_complete .. getSignedContent() Returned Error:%d \n",iRet);
                    eCDLM_State = COM_DOWNL_IDLE;//state change to idle
                    //free(pPKCS7CodeImage);
                    //em->delete_event(event, result);
                    //Message = "Downloaded Host Image  Damaged";
                    snprintf((char*)Message,sizeof(Message),"Downloaded Host Image Damaged");
                    CommonDownloadSendMsgEvent(Message,strlen(Message));
                    CommonDownloadSendSysCntlComm(IMAGE_DAMAGED);
                    vlg_CDLFirmwareCodeDLStatus = DOWNLOADING_FAILED;
                    vlg_CDLFirmwareImageStatus = IMAGE_CORRUPTED;
                    CommonDownloadManager::PostCDLBootMsgDiagEvent(RMF_BOOTMSG_DIAG_MESG_CODE_ERROR, COMM_DL_BTDIAG_CVT_PKCS7_ValidationFailure);
                    remove(pPKCS7CodeImage);//deleting file
                    break;
                }
#endif //#ifdef PKCS_UTILS                
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," Deleting file :%s \n",pPKCS7CodeImage);
                remove(pPKCS7CodeImage);//deleting file
                //
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," Going to Open a file at %s \n",pCotentData);
                fp1 = fopen(pCotentData, "rb");
                if(fp1 == NULL)
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_Engine_complete..Error !! fopen failed at fp1 \n");
                    eCDLM_State = COM_DOWNL_IDLE;//state change to idle
                    CommonDownloadSendSysCntlComm(IMAGE_DAMAGED);
                    remove(pCotentData);
                    break;
                }

                readLen = fread(pTVData, 1, sizeof(pTVData), fp1);
                if(readLen != sizeof(pTVData))
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_Engine_complete.. Error !!  fread() retunred len:%d \n",readLen);
                    eCDLM_State = COM_DOWNL_IDLE;//state change to idle
                    //free(pPKCS7CodeImage);
                    //em->delete_event(event, result);
                    CommonDownloadSendSysCntlComm(IMAGE_DAMAGED);
                    fclose(fp1);
                    remove(pCotentData);
                    break;
                }
                if(pTVData[0] != 28 )
                {
                    vlg_CDLFirmwareImageStatus = IMAGE_CORRUPTED;
                    vlg_CDLFirmwareCodeDLStatus = DOWNLOADING_FAILED;

                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_Engine_complete Error !! in TLV:%d received\n",pTVData[0]);
                    eCDLM_State = COM_DOWNL_IDLE;//state change to idle
                    //free(pPKCS7CodeImage);
                    //em->delete_event(event, result);
                    CommonDownloadSendSysCntlComm(IMAGE_DAMAGED);
                    fclose(fp1);
                    remove(pCotentData);
                    break;
                }
                MaxTLVSize = ((pTVData[1] << 8) |pTVData[2]) + 3;
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," >>>>>>>>>>>> MaxTLVSize:%d \n",MaxTLVSize);
                DataSize = MaxTLVSize;
                pData1 = pData = (char*)malloc(DataSize);
                fseek(fp1, 0, SEEK_SET);
                readLen = fread(pData, 1, DataSize, fp1);
                if(readLen != DataSize)
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_Engine_complete.. Error !!  fread() retunred len:%d \n",readLen);
                    eCDLM_State = COM_DOWNL_IDLE;//state change to idle
                    //free(pPKCS7CodeImage);
                    //em->delete_event(event, result);
                    CommonDownloadSendSysCntlComm(IMAGE_DAMAGED);
                    vlg_CDLFirmwareCodeDLStatus = DOWNLOADING_FAILED;
                    vlg_CDLFirmwareImageStatus = IMAGE_CORRUPTED;
                    fclose(fp1);
                    remove(pCotentData);
                    break;
                }
                sTLVType = *pData++;
                DataSize--;
                //Check for any certificates received .. TLVs..
                //sTLVType = CommonDownloadGetTLVTypeLen(pData,(unsigned short)DataSize,&pData,(unsigned short*)&DataSize);

                //if((sTLVType == COM_DOWNL_TLV_TYPE_SUB_TLVS) || (DataSize < sizeof(unsigned short)) )
                if( sTLVType == COM_DOWNL_TLV_TYPE_SUB_TLVS )
                {

                    sLength = CommonDownloadGetTLVTypeLen((unsigned char*)pData,(unsigned short)DataSize,(unsigned char**)&pData,(unsigned short*)&DataSize);
                    // Sub TLVs length...
                    if(sLength > DataSize)
                    {
                        //Checking whether the length is OK
                        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CommonDownL_Engine_complete: TLV len Error sLength:%d > DataSize:%d \n",sLength, DataSize);

                        eCDLM_State = COM_DOWNL_IDLE;//state change to idle
                        CommonDownloadSendSysCntlComm(IMAGE_DAMAGED);
                        
                        //free(pPKCS7CodeImage);
                        //em->delete_event(event, result);
                        fclose(fp1);
                        remove(pCotentData);
                        break;
                    }
                    DataSize -= sLength;

                    pDLImage = (CommonDownLImage_t*)malloc(sizeof(CommonDownLImage_t));
                    if(pDLImage == (CommonDownLImage_t*)NULL)
                    {
                        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL"," pDLImage is NULL pointer !!! \n");
                        eCDLM_State = COM_DOWNL_IDLE;//state change to idle
                        CommonDownloadSendSysCntlComm(IMAGE_DAMAGED);
                        //free(pPKCS7CodeImage);
                        //em->delete_event(event, result);

                        fclose(fp1);
                        remove(pCotentData);
                        break;
                    }
                    memset(pDLImage,0,sizeof(CommonDownLImage_t));
                    if(sLength > 0)
                    {
                        //There are SUB TLVs .. parse them..
                        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CommonDownL_Engine_complete parsing the SUB_TLVs \n");
                        //&pData returns the pointer to the code image
                        //&sLength returns the remaining length of SUB TLVs
                        // expected value for return value of "sLength" is zero
                        iRet = CommonDownloadParseTLVsForCerts((unsigned char *)pData,sLength,(unsigned char**)&pData,&sLength,pDLImage);
                        if(iRet || sLength)
                        {
                            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_Engine_complete: Error..returned from  CommonDownloadParseTLVsForCerts() Error iRet:%d sLength:%d\n",iRet,sLength);
                            eCDLM_State = COM_DOWNL_IDLE;//state change to idle
                            //free(pPKCS7CodeImage);
                            //em->delete_event(event, result);
                            CommonDownloadSendSysCntlComm(IMAGE_DAMAGED);
                            fclose(fp1);
                            remove(pCotentData);
                            break;
                        }
                        else
                        {
                          
                          CommonDownLoadStoreCertificates(pDLImage);
                        }
                    }


                }
                else
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_Engine_complete Main TLV type Error... Type%d  DataSize:%d",sTLVType,DataSize);
                    eCDLM_State = COM_DOWNL_IDLE;//state change to idle
                    CommonDownloadSendSysCntlComm(IMAGE_DAMAGED);
                    fclose(fp1);
                    remove(pCotentData);
                    break;
                }

                fseek(fp1, MaxTLVSize, SEEK_SET);
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL",">>>>>> Going to Open a file at %s <<<<<\n",pImage);
                fp2 = fopen(pImage, "wb");
                if(fp2 == NULL)
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_Engine_complete..Error !! fopen failed at fp2 \n");
                    eCDLM_State = COM_DOWNL_IDLE;//state change to idle
                    CommonDownloadSendSysCntlComm(IMAGE_DAMAGED);
                    fclose(fp1);
                    remove(pCotentData);

                    break;
                }
                MaxTLVSize = 0x1000;
                pTemp = (char *)malloc(MaxTLVSize);
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," Writing file start \n");
                do
                {
                    readLen = fread(pTemp, 1, MaxTLVSize, fp1);
                    writeLen = fwrite(pTemp,1,readLen,fp2);
                    if(writeLen != readLen)
                    {
                        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_Engine_complete Error !!! writeLen:%d != readLen:%d \n",writeLen,readLen);
                        eCDLM_State = COM_DOWNL_IDLE;//state change to idle
                        CommonDownloadSendSysCntlComm(IMAGE_DAMAGED);


                        flag = 1;
                        break;
                    }
                }while(readLen == 0x1000);
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," Writing file End \n");

                free(pTemp);
                free(pData1);
                free(pDLImage);
                fclose(fp1);
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," Deleting file :%s \n",pCotentData);
                remove(pCotentData);
                fclose(fp2);

                if(flag)
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_Engine_complete Error !! Delecting file pImage:%s \n",pImage);
                    remove(pImage);
                    break;
                }
                eCDLM_State = COM_DOWNL_CODE_IMAGE_UPDATE;
                vlg_CDLFirmwareCodeDLStatus = DOWNLOADING_COMPLETE;
                CommonDownloadSendSysCntlComm(DOWNLOAD_COMPLETE);
                // Posting BootDiag Event
                CommonDownloadManager::PostCDLBootMsgDiagEvent(RMF_BOOTMSG_DIAG_MESG_CODE_COMPLETE, 0);

                // CDL spec defines something else and ATP something else

                CommonDownLSetTimeControls();
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," #######################  UPGRADING THE SOFTWARE  ################# \n");
                //break;
                eCDLM_State = COM_DOWNL_IMAGE_UPGRADE;
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","Upgrade File :%s \n",pImage);
                //Pass image data pointer, and its length
                ImageNamePath.pszImagePathName = pImage;


                if(FileName_TobeLoaded_Size == 0)
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL"," ##########  Error !!! file Name to be saved is of len:%d ########## \n",FileName_TobeLoaded_Size);
                    break;
                }

                memcpy(ImageNamePath.szSoftwareImageName, FileName_TobeLoaded,FileName_TobeLoaded_Size);

                ImageNamePath.eImageSign = VL_CDL_IMAGE_UNSIGNED;
                ImageNamePath.eImageType = VL_CDL_IMAGE_TYPE_MONOLITHIC;

                //Message = "New Host Image Upgrade Started";
                snprintf((char*)Message,sizeof(Message),"New Host Image Upgrade Started");
                CommonDownloadSendMsgEvent(Message,strlen(Message));
                {
                    int iRet;
                    iRet = CHALCdl_notify_cdl_mgr_event(VL_CDL_MANAGER_EVENT_UPGRADE_TO_IMAGE,(unsigned long) &ImageNamePath);
                    if(VL_CDL_RESULT_SUCCESS != iRet )
                    {
                        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","#######################  UPGRADING THE SOFTWARE Failed with Error:0x%X \n",iRet);
                        //Message = "New Host Image Upgrade Failed";
                        snprintf((char*)Message,sizeof(Message),"New Host Image Upgrade Failed");
                        CommonDownloadSendMsgEvent(Message,strlen(Message));

                        break;
                    }
                    //Message = "New Host Image Upgrade Complete and Going to Reboot the Host";
                    
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," #######################  COM_DOWNL_START_REBOOT  ################# \n");
                    system("sync");
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," Deleting file after file upgrade is complete FileName:%s ",pImage);
                    remove(pImage);
                    eCDLM_State = COM_DOWNL_START_REBOOT;
                    iRet = CHALCdl_notify_cdl_mgr_event(VL_CDL_MANAGER_EVENT_REBOOT, 0);
                    if(VL_CDL_RESULT_SUCCESS != iRet )
                    {
                        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","#######################  COM_DOWNL_START_REBOOT Failed with Error:0x%X \n",iRet);
                        break;
                    }
                    snprintf((char*)Message,sizeof(Message),"New Host Image Upgrade Complete\nRebooting the Host");
                    CommonDownloadSendMsgEvent(Message,strlen(Message));
                }

                break;
            }
            case CommonDownL_Engine_complete_t2v2:
            {
#if 1
                TableCVT *pCvt;
                int size,ii,iRet;
                cvt_t2v2_download_data_t* pCvt2v2;
                char *pPKCS7CodeImage[CVT2V2_MAX_OBJECTS];
                char pCotentData[CVT2V2_MAX_OBJECTS][128];// = "/opt/image.bin";
                char pImage[CVT2V2_MAX_OBJECTS][128];// = "/opt/UpgradeImage.bin";
                unsigned long Pkcs7CodeImageSize[CVT2V2_MAX_OBJECTS];
                unsigned long ContentSize[CVT2V2_MAX_OBJECTS],DataSize=0;
                char *pUpgradeImagePathName;
                VL_CDL_IMAGE ImageNamePath;
                int RebootStb = 0,breakflag = 0;
                //Message = "Host Image Download Complete";
                snprintf((char*)Message,sizeof(Message),"Host Image Download Complete");
                CommonDownloadSendMsgEvent(Message,strlen(Message));

                CHALCdl_notify_cdl_mgr_event(VL_CDL_MANAGER_EVENT_DOWNLOAD_COMPLETED, 0);
                {
                    int pathLen,ii,size=48, istrLWrote=0;
                    char imageFile[CVT2V2_MAX_OBJECTS][size]; //= "Extractedimage.img";
                    char UpgradeFile[CVT2V2_MAX_OBJECTS][size];// = "UpgradeImage.img";

                    for(ii = 0; ii < CVT2V2_MAX_OBJECTS; ii++)
                    {
                        snprintf((char*)imageFile[ii],size,"Extractedimage%d.img",ii);
                        snprintf((char*)UpgradeFile[ii],size,"UpgradeImage%d.img",ii);
                    }
                    if( NULL == (vlg_pFilePath = CommonDownloadGetFilePath()) )
                    {
                        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_Engine_complete; Error !! in Getting File Path Error !! \n");
                        eCDLM_State = COM_DOWNL_IDLE;
                        break;
                    }
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CommonDownL_Engine_complete: Got File Path:%s \n", vlg_pFilePath);
                    pathLen = strlen(vlg_pFilePath);
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," Path Length :%d \n",pathLen);

                    for(ii = 0; ii < CVT2V2_MAX_OBJECTS; ii++)
                    {
                        memset( pCotentData[ii], 0, sizeof(pCotentData[ii]));
                        istrLWrote = snprintf( pCotentData[ii], sizeof(pCotentData[ii]),"%s", vlg_pFilePath);
                        if(istrLWrote > 0)
                        {
                            istrLWrote = snprintf( (pCotentData[ii]+istrLWrote), (sizeof(pCotentData[ii]) -istrLWrote), "%s", imageFile[ii]);
                            if(istrLWrote < 0)
                            {
                                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_Engine_complete; Error !! in Copying File imageFile name[%s]!! \n", imageFile[ii]);
                            }
                        }
                        else
                        {
                           RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_Engine_complete; Error !! in Copying File Path vlg_pFilePath !! \n");
                        }

                        memset( pImage[ii], 0, sizeof(pCotentData[ii]));
                        istrLWrote = snprintf( pImage[ii], sizeof(pImage[ii]),"%s", vlg_pFilePath);
                        istrLWrote = snprintf( (pImage[ii]+istrLWrote), (sizeof(pImage[ii]) -istrLWrote), "%s", UpgradeFile[ii]);
                        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL",">>>>>>>>>>Extracted image File Path pCotentData[%d]:%s<<<<<<<<<<<< path Len:%d\n",ii,pCotentData[ii],strlen(pCotentData[ii]));
                        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL",">>>>>>>>>>File to be Upgraded Path pImage[%d]:%s<<<<<<<<<<<< path Len:%d\n",ii,pImage[ii],strlen(pImage[ii]));

                    }



                }

                vlg_CDLFirmwareCodeDLStatus = DOWNLOADING_COMPLETE;
                //CVT is valid
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CommonDownL_Engine_complete_t2v2: Image download is complete event received \n");

                pCvt = (TableCVT*)event_params.data;//Table CVT object
                size = event_params.data_extension;//size of the Table CVT..
                if(  (pCvt == NULL)  ) 
                {
					break;
                }
                if( (pCvt == NULL) || (size != sizeof(TableCVT)) )
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_Engine_complete_t2v2 Error !! pCvt:0x%X size:%d \n",pCvt,size);
                    //break;
                }
                // get the CVT Type
                if( (pCvt->getCVT_Type() !=2 ) || ( pCvt->getCVT_Version() != 2) )
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_Engine_complete_t2v2: ERROR !! getCVT_Type:%d pCvgetCVT_Version():%d \n",pCvt->getCVT_Type() ,pCvt->getCVT_Version());
                    //break;
                }

                pCvt2v2 = (cvt_t2v2_download_data_t*)pCvt->getCVT_DataForUpdate();
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","\n\n################## DOWNLOAD COMPLETE RECEIVED ############### \n");

                for(ii = 0; ii < pCvt2v2->num_of_objects; ii++)
                {
                    pCvt2v2->object_info[ii].downloadImageVerificationDone = 0;
                    pCvt2v2->object_info[ii].downloadImageExtractionDone = 0;
                    pCvt2v2->object_info[ii].downloadParamsUpgradeDone = 0;
                    pCvt2v2->object_info[ii].downloadImageUpgradeNotified = 0;

                    if(pCvt2v2->object_info[ii].downloadComplete)
                    {
                        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CommonDownL_Engine_complete: pDownloadedFileName:%s pDownloadedFileName:0x%X \n",pCvt2v2->object_info[ii].pDownloadedFileName,pCvt2v2->object_info[ii].pDownloadedFileName);
                        pPKCS7CodeImage[ii] = ( char *)pCvt2v2->object_info[ii].pDownloadedFileName;
                        Pkcs7CodeImageSize[ii] = (unsigned long)pCvt2v2->object_info[ii].downloadedFileNameSize;
                        pPKCS7CodeImage[pCvt2v2->object_info[ii].downloadedFileNameSize] = 0;
                        if(pCvt2v2->object_info[ii].pDownloadedFileName)
                        {
                        //    free(pCvt2v2->object_info[ii].pDownloadedFileName);
                        //    pCvt2v2->object_info[ii].pDownloadedFileName = NULL;
                        }
                        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","pPKCS7CodeImage[%d]:%s Pkcs7CodeImageSize[%d]:%d\n",ii,pPKCS7CodeImage[ii],ii,Pkcs7CodeImageSize[ii]);


                        ContentSize[ii]= strlen(pCotentData[ii]);

                                //break;
                        if(pPKCS7CodeImage[ii] == NULL || Pkcs7CodeImageSize[ii] == 0)
                        {
                            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_Engine_complete; Error !!pPKCS7CodeImage:0x%X Pkcs7CodeImageSize:%d\n",pPKCS7CodeImage,Pkcs7CodeImageSize);
                            eCDLM_State = COM_DOWNL_IDLE;
                            CommonDownloadSendSysCntlComm(IMAGE_DAMAGED);
                            //em->delete_event(event, result);
                            breakflag = 1;
                            break;
                        }
                        snprintf((char*)Message,sizeof(Message),"Verifying the Host Image(%d)\nPlease Do Not Power Off",ii/*( char *)pCvt2v2->object_info[ii].codefile_data.name*/);
                        CommonDownloadSendMsgEvent(Message,strlen(Message));
                
                        
                        //Status changed to Image validation
                        eCDLM_State = COM_DOWNL_CODE_IMAGE_PKCS7_VALIDATION;
                        {
                            CommonDLObjType_e type = COMM_DL_MONOLITHIC_IMAGE;
                            switch(pCvt2v2->object_info[ii].object_type)
                            {
                            case CVT2V2_FIRMWARE_OBJECT:
                                type = COMM_DL_FIRMWARE_IMAGE;
                                break;
                            case CVT2V2_APPLICATION_OBJECT:
                                type = COMM_DL_APP_IMAGE;
                                break;
                            case CVT2V2_DATA_OBJECT:
                                type = COMM_DL_DATA_IMAGE;
                                break;

                            }
                            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," >>>>>>>>>>. pPKCS7CodeImage[%d]:%s size:%d \n",ii,pPKCS7CodeImage[ii],Pkcs7CodeImageSize[ii]);
                            //Verify the the PKCS signed code image signature
                            iRet = vlg_pCodeImageValidationMgr->processCodeImage((char  *)pPKCS7CodeImage[ii],(int)Pkcs7CodeImageSize[ii],type);
                        }
                        if( iRet == 0)
                        {
                            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","\n SUCCESS verifySignedContent[%d]\n",ii);
                            vlg_CDLFirmwareImageStatus = IMAGE_AUTHORISED;
                            //Message = "Downloaded Host Image  Verification Success";
                            snprintf((char*)Message,sizeof(Message),"Downloaded Host Image(%d)\nVerification Success",ii/*( char *)pCvt2v2->object_info[ii].codefile_data.name*/);
                            CommonDownloadSendMsgEvent(Message,strlen(Message));
                            pCvt2v2->object_info[ii].downloadImageVerificationDone = 1;
                            //CommonDownloadSendSysCntlComm(DOWNLOAD_COMPLETE);
                        }
                        else
                        {
                            vlg_CDLFirmwareImageStatus = IMAGE_CERT_FAILURE;
                            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","\nPKCS#7 signed data[%d] Verification FAILED  \n",ii);
                            //Free the PKCS7 CVT memory and delete the event
                            //free((unsigned char *)event->get_data());
                            //em->delete_event(event, result);
                            //Message = "Downloaded Host Image  Verification Failed";
                            snprintf((char*)Message,sizeof(Message),"Downloaded Host Image(%d)\nVerification Failed",ii/*( char *)pCvt2v2->object_info[ii].codefile_data.name*/);
                            CommonDownloadSendMsgEvent(Message,strlen(Message));
                            eCDLM_State = COM_DOWNL_IDLE;
                            CommonDownloadSendSysCntlComm(IMAGE_DAMAGED);
                            breakflag = 1;
                            break;
                        }
                    }
                    else if(pCvt2v2->object_info[ii].downloadError)
                    {
                        
                        switch(pCvt2v2->object_info[ii].downloadError)
                        {
                            case DOWNLOAD_ENGINE_ERROR_TUNE:
                              CommonDownloadManager::PostCDLBootMsgDiagEvent(RMF_BOOTMSG_DIAG_MESG_CODE_ERROR, COMM_DL_BTDIAG_FILE_NOT_FOUND);
                              break;
                            case DOWNLOAD_ENGINE_ERROR_ATTACH:
                              CommonDownloadManager::PostCDLBootMsgDiagEvent(RMF_BOOTMSG_DIAG_MESG_CODE_ERROR, COMM_DL_BTDIAG_FILE_NOT_FOUND);

                              break;
                            case DOWNLOAD_ENGINE_ERROR_CAROUSEL_READ_NOIMAGE:
                              CommonDownloadManager::PostCDLBootMsgDiagEvent(RMF_BOOTMSG_DIAG_MESG_CODE_ERROR, COMM_DL_BTDIAG_FILE_NOT_FOUND);
                              break;
                            case DOWNLOAD_ENGINE_ERROR_TFTP_SETUP:
                              CommonDownloadManager::PostCDLBootMsgDiagEvent(RMF_BOOTMSG_DIAG_MESG_CODE_ERROR, COMM_DL_BTDIAG_SERVER_NOT_AVAILABLE);
                              break;
                            case DOWNLOAD_ENGINE_ERROR_TFTP_READ:
                              CommonDownloadManager::PostCDLBootMsgDiagEvent(RMF_BOOTMSG_DIAG_MESG_CODE_ERROR, COMM_DL_BTDIAG_FILE_NOT_FOUND);
                              break;
                            default:
                              CommonDownloadManager::PostCDLBootMsgDiagEvent(RMF_BOOTMSG_DIAG_MESG_CODE_ERROR, COMM_DL_BTDIAG_FILE_NOT_FOUND);
                            //CommonDownloadSendSysCntlComm(NO_DOWNLOAD_IMAGE);
                            break;
                        }
                        breakflag = 1;
                        break;
                    }
                    else
                    {
                        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_Engine_complete_t2v2 Error !! Error in event \n");
                        breakflag = 1;
                        break;
                    }
                }//for
                if(breakflag)
                    break;
                //break;

                //CommonDownloadSendSysCntlComm(DOWNLOAD_COMPLETE);

                //CommonDownloadManager::PostCDLBootMsgDiagEvent(RMF_BOOTMSG_DIAG_MESG_CODE_COMPLETE, 0);
                for(ii = 0; ii < pCvt2v2->num_of_objects; ii++)
                {
                    if(pCvt2v2->object_info[ii].downloadImageVerificationDone)
                    {
#ifdef PKCS_UTILS                      
                        // Get the code image data from the PKCS7
                        iRet = getSignedContent(pPKCS7CodeImage[ii],Pkcs7CodeImageSize[ii],(char)1,(char **)&pCotentData[ii],(int*)&ContentSize[ii]);
                        if( iRet != 0)
                        {
                            vlg_CDLFirmwareImageStatus = IMAGE_CORRUPTED;
                            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_Engine_complete .. getSignedContent() Returned Error:%d \n",iRet);
                            eCDLM_State = COM_DOWNL_IDLE;//state change to idle
                            //free(pPKCS7CodeImage);
                            //em->delete_event(event, result);
                            //Message = "Downloaded Host Image  Damaged";
                            snprintf((char*)Message,sizeof(Message),"Downloaded Host Image(%d)\n Damaged",ii/*( char *)pCvt2v2->object_info[ii].codefile_data.name*/);
                            CommonDownloadSendMsgEvent(Message,strlen(Message));
                            CommonDownloadSendSysCntlComm(IMAGE_DAMAGED);
                            remove(pPKCS7CodeImage[ii]);//deleting file
                            CommonDownloadManager::PostCDLBootMsgDiagEvent(RMF_BOOTMSG_DIAG_MESG_CODE_ERROR, COMM_DL_BTDIAG_CVT_PKCS7_ValidationFailure);
                            breakflag = 1;
                            break;
                        }
                        else
                        {
                            pCvt2v2->object_info[ii].downloadImageExtractionDone = 1;
                            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," Deleting file :%s \n",pPKCS7CodeImage[ii]);
                            remove(pPKCS7CodeImage[ii]);//deleting file
                        }
#endif                        
                    }

                }
#ifdef PKCS_UTILS           
                if(breakflag)
                    break;
#endif
                for(ii = 0; ii < pCvt2v2->num_of_objects; ii++)
                {
                    if(pCvt2v2->object_info[ii].downloadImageExtractionDone)
                    {

                        iRet = CommonDownloadUpgradeDownloadParams(pCotentData[ii],pImage[ii]);
                        if(iRet ==  0)
                        {
                            pCvt2v2->object_info[ii].downloadParamsUpgradeDone = 1;
                            CommonDownLSetTimeControls();
                        }
                        else
                        {
                            snprintf((char*)Message,sizeof(Message),"Downloaded Host Image(%d)\n Damaged",ii/*( char *)pCvt2v2->object_info[ii].codefile_data.name*/);
                            CommonDownloadSendMsgEvent(Message,strlen(Message));
                            breakflag = 1;
                            break;

                        }
                    }
                }
                if(breakflag)
                    break;


                eCDLM_State = COM_DOWNL_CODE_IMAGE_UPDATE;
                CommonDownloadSendSysCntlComm(DOWNLOAD_COMPLETE);

                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," #######################  UPGRADING THE SOFTWARE  ################# \n");
                //break;
                for(ii = 0; ii < pCvt2v2->num_of_objects; ii++)
                {
                    if(pCvt2v2->object_info[ii].downloadParamsUpgradeDone)
                    {
                        int iRet;

                        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","Upgrade File :%s \n",pImage[ii]);
                        //Pass image data pointer, and its length
                        ImageNamePath.pszImagePathName = pImage[ii];



                        memcpy(ImageNamePath.szSoftwareImageName, pCvt2v2->object_info[ii].codefile_data.name,pCvt2v2->object_info[ii].codefile_data.len);
                        ImageNamePath.szSoftwareImageName[pCvt2v2->object_info[ii].codefile_data.len] = 0;
                        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","ImageNamePath.szSoftwareImageName:%s \n",ImageNamePath.szSoftwareImageName);
                        ImageNamePath.eImageSign = VL_CDL_IMAGE_UNSIGNED;
                        if(pCvt2v2->object_info[ii].object_type == CVT2V2_FIRMWARE_OBJECT)
                        {
                            ImageNamePath.eImageType = VL_CDL_IMAGE_TYPE_FIRMWARE;
                        }
                        else if(pCvt2v2->object_info[ii].object_type == CVT2V2_APPLICATION_OBJECT)
                        {
                            ImageNamePath.eImageType = VL_CDL_IMAGE_TYPE_APPLICATION;
                        }
                        else if(pCvt2v2->object_info[ii].object_type == CVT2V2_DATA_OBJECT)
                        {
                            ImageNamePath.eImageType = VL_CDL_IMAGE_TYPE_DATA;
                        }
                        else
                        {
                            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","object type ERROR[%d]: %d \n",ii,pCvt2v2->object_info[ii].object_type);
                        }

                        //Message = "New Host Image Upgrade Started";
                        snprintf((char*)Message,sizeof(Message),"New Host Image[%d] Upgrade Started",ii);
                        CommonDownloadSendMsgEvent(Message,strlen(Message));
                        //break;
                        iRet = CHALCdl_notify_cdl_mgr_event(VL_CDL_MANAGER_EVENT_UPGRADE_TO_IMAGE,(unsigned long) &ImageNamePath);
                        if(VL_CDL_RESULT_SUCCESS != iRet )
                        {
                            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","#######################  UPGRADING THE SOFTWARE Failed with Error[%d]:0x%X \n",ii,iRet);
                            //Message = "New Host Image Upgrade Failed";
                            snprintf((char*)Message,sizeof(Message),"New Host Image[%d] Upgrade Failed",ii);
                            CommonDownloadSendMsgEvent(Message,strlen(Message));
\
                            breakflag = 1;
                            break;
                        }
                        else
                        {
                            pCvt2v2->object_info[ii].downloadImageUpgradeNotified = 1;

                        }
                    }
                }
                if(breakflag)
                    break;

                for(ii = 0; ii < pCvt2v2->num_of_objects; ii++)
                {
                    if(pCvt2v2->object_info[ii].downloadImageUpgradeNotified)
                    {
                        system("sync");
                        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," Deleting file after file upgrade is complete FileName:%s ",pImage);
                        remove(pImage[ii]);


                    }
                    else
                    {
                        breakflag =1;
                        break;
                    }
                }
                if(breakflag)
                    break;


                {
                    //Message = "New Host Image Upgrade Complete and Going to Reboot the Host";
                    snprintf((char*)Message,sizeof(Message),"New Host Image Upgrade Complete\nRebooting the Host");
                    CommonDownloadSendMsgEvent(Message,strlen(Message));
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," #######################  COM_DOWNL_START_REBOOT  ################# \n");

                    eCDLM_State = COM_DOWNL_START_REBOOT;
                    iRet = CHALCdl_notify_cdl_mgr_event(VL_CDL_MANAGER_EVENT_REBOOT, 0);
                    if(VL_CDL_RESULT_SUCCESS != iRet )
                    {
                        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","#######################  COM_DOWNL_START_REBOOT Failed with Error:0x%X \n",iRet);
                    }

                }
#endif
                break;
            }
            case CommonDownL_Cert_Check:
            {
              //SAK: Commented due to link error
              //vlg_SnmpCertStatusRetvalue = CommonDownloadMgrSnmpCertificateStatus_CDL(&vlg_SnmpCertStatus);
		sem_post(&vlg_SnmpCertStatusEvent);
              break;
            }
            case CommonDownL_Test_case:
            {
            unsigned char *pMpeg,*pPKCS7,*pCVT2;
            int iMpegSize,iPKCS7Size,iCVTSize;
            unsigned char table_ID, section_synctax_indicator,private_indicator,reserved;
            unsigned short private_section_length=0;
            pkcs7SignedDataInfo_t SignerInfo;
            cvcinfo_t cvcInfos;
            CDLMgrSnmpCertificateStatus_t CertStatus;
            if(eCDLM_State != COM_DOWNL_IDLE &&  eCDLM_State != COM_DOWNL_CVT_PROC_AND_VALIDATION)
            {
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CommonDownloadManager::State is not idle <<<<<\n\n");
                //Free the CVT memory and delete the event
                //free((unsigned char *)event->get_data());
                //em->delete_event(event, result);
                //continue;
                break;
            }
            //CommonDownloadMgrSnmpCertificateStatus(&CertStatus);

            break;
#if 0
            if(vlg_USETestPKCSCVT == 0)//This flag is for tesing purpose only... will be removed later
            {
                        if( vlg_DSGCVTFlag == 0)//This flag is for tesing purpose only... will be removed later
                 {
                    vlg_DSGCVTFlag = 1;
                 }
                 else
                     break;
                // Common download  eSTB or CVT trigger from DSG
                // PMPEG section{ PKCS # 7 { CVT2 } ) received ...
                // CVT2 is in PKCS # 7 structure and PKCS#7 is in MPEG section.
                // Need to extract the CVT2 from Mpeg Section.
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","\n CommonDownloadManager::Run() Received EVENT from Card Manager CommonDownL_CVT_DSG <<<<<<<<<<<\n\n");

                pMpeg = (unsigned char *)event->get_data();
                 iMpegSize = event->data_item_count();
                //PrintBuff(pMpeg,iMpegSize);

                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CommonDownloadManager::Run()  pMpeg:0x%X iMpegSize:%d \n",pMpeg,iMpegSize);
                if(pMpeg == NULL || iMpegSize == 0)
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadManager::Run() Error !! pMpeg:0x%X iMpegSize:%d \n",pMpeg,iMpegSize);

                    //em->delete_event(event, result);
                    break;
                }
                if(eCDLM_State != COM_DOWNL_IDLE &&  eCDLM_State != COM_DOWNL_CVT_PROC_AND_VALIDATION)
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadManager::State is not idle <<<<<\n\n");
                    //Free the PKCS7 CVT memory and delete the event
                    //free((unsigned char *)event->get_data());
                    //em->delete_event(event, result);
                    break;
                }


                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," MPEG Section Details..0x%X 0x%X 0x%X\n",*pMpeg,*(pMpeg + 1),*(pMpeg + 2));
                table_ID = *pMpeg++;
                section_synctax_indicator = *pMpeg++;
                private_indicator = (section_synctax_indicator & 0x80) >> 7;
                reserved = (section_synctax_indicator & 0x30) >> 4;
                private_section_length = ((section_synctax_indicator &0x0F) << 8) | (*pMpeg++);
                section_synctax_indicator = (section_synctax_indicator & 0x40) >> 6 ;

                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","table_ID:0x%X\n section_synctax_indicator:0x%X \n",table_ID,section_synctax_indicator);
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","private_indicator:0x%X\n reserved:0x%X\n",private_indicator,reserved);
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","private_section_length:0x%X\n ",private_section_length);
                }//vlg_USETestPKCSCVT
                #endif
#if 0
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL",">>>>>>>>>>>>>>>>>>>>>>> signedCVT >>>>>>>>>>>>>>>>>>>>>>>>>>> \n");
                if(vlg_USETestPKCSCVT)// Test flag for testing purpose
                {

                    pPKCS7 = EricSignedImage;//byteArray;//signedCVT;//pMpeg //EricSignedImage;
                    iPKCS7Size = sizeof(EricSignedImage);
                }
                else
                {
                    pPKCS7 = pMpeg;
                    iPKCS7Size = iMpegSize - 3;
                }
                if(vlg_FileEnable)
                {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","#####  File Name:%s file name size:%d \n",FileName,len);
                }
#endif

#if 0
                //
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," calling verifySignedContent() PKCS size:%d :0x%X\n",iPKCS7Size,iPKCS7Size);
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," @@@@@@@@@@@@@@@@@@@@ \n");

                if(vlg_FileEnable)
                {
                    iRet =  verifySignedContent((char  *)FileName,(int)len,(char)1);
                }
                else
                {
                    iRet =  verifySignedContent((char  *)pPKCS7,(int)iPKCS7Size,(char)0);
                }
                //this function... verifies the PKCS#7 signed data
                if( 0== iRet)
                {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","\n SUCCESS verifySignedContent\n");

                }
                else
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","\nPKCS#7 CVT signed data Verification FAILED Ignore the trigger \n");
                    //Free the PKCS7 CVT memory and delete the event
                    //free((unsigned char *)event->get_data());
                    //em->delete_event(event, result);
                    //continue;
                //    break;
                }
#endif
#ifdef PKCS_UTILS

                if(vlg_FileEnable)
                {
                  

                    iRet = getSignerInfo((char  *)FileName, (int)len, (char)1,&SignerInfo);
                }
                else
                {
                    iRet = getSignerInfo((char  *)pPKCS7, (int)iPKCS7Size, (char)0,&SignerInfo);
                }
                if(iRet != 0)
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","\nPKCS#7 CVT getSignerInfo() returned  ERROR Ignore the trigger \n");
                    //Free the PKCS7 CVT memory and delete the event
                    //free((unsigned char *)event->get_data());
                    //em->delete_event(event, result);
                    //continue;
                    break;
                }
                PrintSignerInfo(SignerInfo);
                //this function returns CVT on  successful decoding of the PKCS7 data. this fucntion returns 0 on success..
#endif

#if 0
                if(vlg_FileEnable)
                {
                    char pRetFile[64]="/mnt/nfs/image.bin";
                    int filelen= strlen(pRetFile,sizeof(pRetFile));
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," Calling getSignedContent: file name:%s  filename len:%d \n",pRetFile,filelen);
                    iRet = getSignedContent((char  *)FileName,(int)len,(char)1,(char**)&pRetFile,(int*)&filelen);
                }
                else
                {
                    iRet = getSignedContent((char  *)pPKCS7,(int)iPKCS7Size,(char)0,(char**)&pCVT2,(int*)&iCVTSize);
                }
                if(iRet != 0)
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","\nPKCS#7 CVT getSignedContent() returned  ERROR Ignore the trigger \n");
                    //Free the PKCS7 CVT memory and delete the event
                    //free((unsigned char *)event->get_data());
                    //em->delete_event(event, result);
                    break;
                }
                PrintBuffer(pCVT2,iCVTSize);
#endif
#if 0
                //while(1)
                        {
                    unsigned char *pData,*pTemp,TLV;
                    unsigned short TLVlen =0;
                    int size = iCVTSize;
                    pData = pTemp = pCVT2;
                    TLV = *pData++;
                    size--;
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","\n TLV type: %d \n",TLV);
                    TLVlen = (pData[0] << 8)|pData[1];
                    pData = pData +2;
                    size -=2;
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," TLV Len:%d \n",TLVlen);
                    while(1)
                    {
                        TLV = *pData++;
                        size--;
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," TLV type: %d \n",TLV);
                        if(size <=0)
                            break;
                        TLVlen = (pData[0] << 8)|pData[1];
                        pData = pData +2;
                        size -=2;
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," TLV Len:%d \n",TLVlen);
                        if(size <=0)
                            break;
                        pData = pData + TLVlen;
                        size -= TLVlen;
                        if(size <=0)
                        {
                            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," Size:%d <= 0  \n",size);
                            break;
                        }


                    }

                        }
#endif
#if 0
                //testCVT((char*)pCVT2, iCVTSize);
                iRet = vlg_pCodeImageValidationMgr->processCodeImage((char  *)pPKCS7,(int)iPKCS7Size,COMM_DL_MONOLITHIC_IMAGE);
                //iRet = CommonDownloadVerifyCVT((unsigned char *)pCVT2,(unsigned long) iCVTSize);
                if(iRet != 0)
                {
                    //Free the PKCS7 CVT memory and delete the event
                    //free((unsigned char *)event->get_data());
                    //em->delete_event(event, result);
                    //continue;
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL"," FAILED >>>> vlg_pCodeImageValidationMgr->processCodeImage  \n");
                    break;
                }
            //    testCVT((char*)pCVT2, iCVTSize);
                break;
#endif
            }
            break;
            default:
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadManager::Run()  Unhandled !!  event received\n");
                break;
            }
                       }//switch (event->get_type())
            break;
        }//case PFC_EVENT_CATEGORY_COM_DOWNL_MANAGER:

        default:
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadManager::Run()  UNKNOWN !!  event received\n");
            break;
        }
       }//switch (event->get_category())
    //Free the PKCS7 CVT memory and delete the event
       RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," CommonDownloadManager:: Exit the main switch case and going to free memory and delete the event \n");
      // if(event->get_data() != NULL)
       //    free((unsigned char *)event->get_data());
	rmf_osal_event_delete(event_handle);
    }// end of while loop
}

static int CommonDownloadUpgradeDownloadParams( char *pCotentData,char *pImage)
{
    FILE * fp1 = NULL,*fp2 = NULL;
    size_t readLen,writeLen;
    unsigned char pTVData[4];
    unsigned long MaxTLVSize,DataSize=0;
    char *pData = NULL,*pData1 = NULL,*pTemp = NULL;
    unsigned short sTLVType,sLength;
    CommonDownLImage_t *pDLImage = NULL;
    int flag = 0,iRet;    
    char Message[512];
    int ret = 0;
                
    eCDLM_State = COM_DOWNL_CODE_IMAGE_DOWNLOAD_COMPLETE;
//    CommonDownloadSendSysCntlComm(DOWNLOAD_COMPLETE);
    snprintf((char*)Message,sizeof(Message),"Host Image Download Completed\nDo not Power Off the box (eCm)");
    CommonDownloadSendMsgEvent(Message,strlen(Message));
    //
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," Going to Open a file at %s \n",pCotentData);
    fp1 = fopen(pCotentData, "rb");
    if(fp1 == NULL)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_Engine_complete..Error !! fopen failed at fp1 \n");
    //    eCDLM_State = COM_DOWNL_IDLE;//state change to idle
        CommonDownloadSendSysCntlComm(IMAGE_DAMAGED);
        ret = remove(pCotentData);
        if( ret < 0 ) 	
        {
             RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_Engine_complete..Error could not remove pCotentData !! \n");	
        }
        return -1;
    }

    readLen = fread(pTVData, 1, sizeof(pTVData), fp1);
    if(readLen != sizeof(pTVData))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_Engine_complete.. Error !!  fread() retunred len:%d \n",readLen);
        //eCDLM_State = COM_DOWNL_IDLE;//state change to idle
        CommonDownloadSendSysCntlComm(IMAGE_DAMAGED);
        fclose(fp1);
        ret = remove(pCotentData);
        if( ret < 0 ) 	
        {
             RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_Engine_complete..Error could not remove pCotentData !! \n");	
        }
        return -1;

    }
    if(pTVData[0] != 28 )
    {
        vlg_CDLFirmwareImageStatus = IMAGE_CORRUPTED;
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_Engine_complete Error !! in TLV:%d received\n",pTVData[0]);
        //eCDLM_State = COM_DOWNL_IDLE;//state change to idle
        CommonDownloadSendSysCntlComm(IMAGE_DAMAGED);
        fclose(fp1);
        ret = remove(pCotentData);
        if( ret < 0 ) 	
        {
             RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_Engine_complete..Error could not remove pCotentData !! \n");	
        }
        return -1;

    }
    MaxTLVSize = ((pTVData[1] << 8) | pTVData[2]) + 3;
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," >>>>>>>>>>>> MaxTLVSize:%d \n",MaxTLVSize);
    DataSize = MaxTLVSize;
    pData = pData1 = (char*)malloc(DataSize);
    if(pData == NULL)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_Engine_complete Error !! returing NULL Pointer Malloc failed\n");
        fclose(fp1);
        ret = remove(pCotentData);
        if( ret < 0 ) 	
        {
             RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_Engine_complete..Error could not remove pCotentData !! \n");	
        }
        return -1;
    }
    fseek(fp1, 0, SEEK_SET);
    readLen = fread(pData, 1, DataSize, fp1);
    if(readLen != DataSize)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_Engine_complete.. Error !!  fread() retunred len:%d \n",readLen);
        //eCDLM_State = COM_DOWNL_IDLE;//state change to idle
        CommonDownloadSendSysCntlComm(IMAGE_DAMAGED);
        fclose(fp1);
        ret = remove(pCotentData);
        if( ret < 0 ) 	
        {
             RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_Engine_complete..Error could not remove pCotentData !! \n");	
        }
        free(pData1);
        return -1;
    }
    sTLVType = *pData++;
    DataSize--;
    //Check for any certificates received .. TLVs..
    //sTLVType = CommonDownloadGetTLVTypeLen(pData,(unsigned short)DataSize,&pData,(unsigned short*)&DataSize);

    if( sTLVType == COM_DOWNL_TLV_TYPE_SUB_TLVS )
    {
        sLength = CommonDownloadGetTLVTypeLen((unsigned char*)pData,(unsigned short)DataSize,(unsigned char**)&pData,(unsigned short*)&DataSize);
        // Sub TLVs length...
        if(sLength > DataSize)
        {
            //Checking whether the length is OK
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_Engine_complete: TLV len Error sLength:%d > DataSize:%d \n",sLength, DataSize);
            //eCDLM_State = COM_DOWNL_IDLE;//state change to idle
            CommonDownloadSendSysCntlComm(IMAGE_DAMAGED);
            fclose(fp1);
            ret = remove(pCotentData);
            if( ret < 0 ) 	
            {
                 RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_Engine_complete..Error could not remove pCotentData !! \n");	
            }
            free(pData1);
            return -1;
        }
        DataSize -= sLength;

        pDLImage = (CommonDownLImage_t*)malloc(sizeof(CommonDownLImage_t));
        
        if(pDLImage == (CommonDownLImage_t*)NULL)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL"," pDLImage is NULL pointer !!! \n");
            //eCDLM_State = COM_DOWNL_IDLE;//state change to idle
            CommonDownloadSendSysCntlComm(IMAGE_DAMAGED);
            fclose(fp1);           
            ret = remove(pCotentData);
            if( ret < 0 ) 	
            {
                 RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_Engine_complete..Error could not remove pCotentData !! \n");	
            }
            free(pData1);
            return -1;
        }
        memset(pDLImage,0,sizeof(CommonDownLImage_t));
        if(sLength > 0)
        {
            //There are SUB TLVs .. parse them..
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CommonDownL_Engine_complete parsing the SUB_TLVs \n");
            //&pData returns the pointer to the code image
            //&sLength returns the remaining length of SUB TLVs
            // expected value for return value of "sLength" is zero
            iRet = CommonDownloadParseTLVsForCerts((unsigned char *)pData,sLength,(unsigned char**)&pData,&sLength,pDLImage);
            if(iRet || sLength)
            {         
                int ret = 0; 
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_Engine_complete: Error..returned from  CommonDownloadParseTLVsForCerts() Error iRet:%d sLength:%d\n",iRet,sLength);
                //eCDLM_State = COM_DOWNL_IDLE;//state change to idle
                CommonDownloadSendSysCntlComm(IMAGE_DAMAGED);
                fclose(fp1);
                ret = remove(pCotentData);
                if( ret < 0 ) 	
                {
                     RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_Engine_complete..Error could not remove pCotentData !! \n");	
                }
                free(pData1);
                free(pDLImage);
                return -1;
            }
        }

    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_Engine_complete Main TLV type Error... Type%d  DataSize:%d",sTLVType,DataSize);
        eCDLM_State = COM_DOWNL_IDLE;//state change to idle
         CommonDownloadSendSysCntlComm(IMAGE_DAMAGED);
        fclose(fp1);
        ret = remove(pCotentData);
        if( ret < 0 ) 	
        {
             RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_Engine_complete..Error could not remove pCotentData !! \n");	
        }
        free(pData1);
        free(pDLImage);
        return -1;
    }

    fseek(fp1, MaxTLVSize, SEEK_SET);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL",">>>>>> Going to Open a file at %s <<<<<\n",pImage);
    fp2 = fopen(pImage, "wb");
    if(fp2 == NULL)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_Engine_complete..Error !! fopen failed at fp2 \n");
        //eCDLM_State = COM_DOWNL_IDLE;//state change to idle
        CommonDownloadSendSysCntlComm(IMAGE_DAMAGED);
        fclose(fp1);
        ret = remove(pCotentData);
        if( ret < 0 ) 	
        {
             RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_Engine_complete..Error could not remove pCotentData !! \n");	
        }
        free(pData1);
        free(pDLImage);
        return -1;
    }
    MaxTLVSize = 0x1000;
    pTemp = (char *)malloc(MaxTLVSize);
    if(pTemp == NULL)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_Engine_complete Error !! returing NULL Pointer Malloc failed\n");
        free(pData1);
        free(pDLImage);
        fclose(fp1);
        fclose(fp2);		
        ret = remove(pCotentData);
        if( ret < 0 ) 	
        {
             RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_Engine_complete..Error could not remove pCotentData !! \n");	
        }
        return -1;
    }
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," Writing file start \n");
    do
    {
        readLen = fread(pTemp, 1, MaxTLVSize, fp1);
        writeLen = fwrite(pTemp,1,readLen,fp2);
        if(writeLen != readLen)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_Engine_complete Error !!! writeLen:%d != readLen:%d \n",writeLen,readLen);
            //eCDLM_State = COM_DOWNL_IDLE;//state change to idle
            CommonDownloadSendSysCntlComm(IMAGE_DAMAGED);
            flag = 1;
            break;
        }
    }while(readLen == 0x1000);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," Writing file End \n");

    free(pTemp);
    free(pData1);
    free(pDLImage);
    fclose(fp1);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," Deleting file :%s \n",pCotentData);
    ret = remove(pCotentData);
    if( ret < 0 ) 	
    {
         RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_Engine_complete..Error could not remove pCotentData !! \n");	
    }
    fclose(fp2);
    if(flag)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownL_Engine_complete Error !! Delecting file pImage:%s \n",pImage);
        return -1;
    }

    return 0;
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// CommonDownloadParseTLVsForCerts()
// Description:
//
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
static int CommonDownloadParseTLVsForCerts(unsigned char *pData, unsigned short size,unsigned char **pRetData, unsigned short *pSize,CommonDownLImage_t *pCodeImage)
{
    unsigned short *pTLV,sTVLType,sTLVLen;
    int ii,iRet;
    pTLV = (unsigned short *)pData;

    for(ii = 0; ii < 3; ii++)
    {

       if(size < 3)
       {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadParseTLVsForCerts: Error in TLV size :%d \n",size);
        return -1;
       }
     //  sTVLType = CommonDownloadGetTLVTypeLen(pData,size,&pData,&size);
       sTVLType = (unsigned short)*pData;
       pData++;
       size--;
       sTLVLen = CommonDownloadGetTLVTypeLen(pData,size,&pData,&size);

       switch(sTVLType)
       {
         case COM_DOWNL_TLV_TYPE_CL_DEVICE_CA_CERT:
        if(sTLVLen)
        {
            pCodeImage->pCLCvcDeviceCaCert = pData;
            pCodeImage->CLCvcDeviceCaCertLen = sTLVLen;

            pData += sTLVLen;
            size -= sTLVLen;
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CommonDownloadParseTLVsForCerts: Received COM_DOWNL_TLV_TYPE_CL_DEVICE_CA_CERT !! \n");
        }

        break;
         case COM_DOWNL_TLV_TYPE_CL_CVC_ROOT_CA_CERT:
        if(sTLVLen)
        {
            pCodeImage->pCLCvcRootCaCert = pData;
            pCodeImage->CLCvcRootCaCertLen = sTLVLen;

            pData += sTLVLen;
            size -= sTLVLen;
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CommonDownloadParseTLVsForCerts: Received COM_DOWNL_TLV_TYPE_CL_CVC_ROOT_CA_CERT !! \n");

        }
        break;
         case COM_DOWNL_TLV_TYPE_CL_CVC_CA_CERT:
        if(sTLVLen)
        {
            pCodeImage->pCLCvcCaCert = pData;
            pCodeImage->CLCvcCaCertLen = sTLVLen;

            pData += sTLVLen;
            size -= sTLVLen;
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CommonDownloadParseTLVsForCerts: Received COM_DOWNL_TLV_TYPE_CL_CVC_CA_CERT !! \n");
        }
        break;
         default:

        break;
       }
        *pSize =size;
      *pRetData = pData;
        if(size ==  0)
            break;

    }//for


      if(size != 0)
      {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadParseTLVsForCerts: at the END Error !!!!  in TLV size :%d... it should be zero \n",size);
        return -1;

      }
       return 0;
}

int CommonDownloadCVTFree(TableCVT *pTCvt)
{
    cvtdownload_data_t* data;

    if(pTCvt == NULL)
    {
        return COMM_DL_CVT_OBJECT_DATA_ERROR;
    }
    data = pTCvt->getCVT_DataForUpdate();
    if(pTCvt->getCVT_Type() == CVT_T1V1)
    {
        cvt_t1v1_download_data_t* pCvt1 = (cvt_t1v1_download_data_t*)data;

        if(pCvt1->codefile_data.name != NULL)
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," Freeing pCvt1->codefile_data.name:%s \n",pCvt1->codefile_data.name);
            free( pCvt1->codefile_data.name );
            pCvt1->codefile_data.name = NULL;			
        }
        else
        {
            return COMM_DL_CVT_OBJECT_DATA_ERROR;
        }
        if( pCvt1->cvc_data.data != NULL)
        {
            free( pCvt1->cvc_data.data );
            pCvt1->cvc_data.data = NULL;			
        }
        else
        {
            return COMM_DL_CVT_OBJECT_DATA_ERROR;
        }

    }
    else if((pTCvt->getCVT_Type() == CVT_T2Vx) && (pTCvt->getCVT_Version() == 1))
    {
        cvt_t2v1_download_data_t* pCvt2 = (cvt_t2v1_download_data_t*)data;

        if(pCvt2->codefile_data.name != NULL)
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," Freeing pCvt2->codefile_data.name:%s \n",pCvt2->codefile_data.name);
            free( pCvt2->codefile_data.name );
            pCvt2->codefile_data.name = NULL;			
        }
        else
        {
            return COMM_DL_CVT_OBJECT_DATA_ERROR;
        }
        if( pCvt2->cvc_data.data != NULL)
        {
            free( pCvt2->cvc_data.data );
            pCvt2->cvc_data.data = NULL;			
        }
        else
        {
            return COMM_DL_CVT_OBJECT_DATA_ERROR;
        }

    }
    else if((pTCvt->getCVT_Type() == CVT_T2Vx) && (pTCvt->getCVT_Version() == 2))
    {
        cvt_t2v2_download_data_t* pCvt2v2 = (cvt_t2v2_download_data_t*)data;
        int ii;
        for(ii = 0; ii < pCvt2v2->num_of_objects; ii++ )
        {
            if(pCvt2v2->object_info[ii].codefile_data.name != NULL)
            {
                free( pCvt2v2->object_info[ii].codefile_data.name );
                pCvt2v2->object_info[ii].codefile_data.name = NULL; 				
            }
            if(pCvt2v2->object_info[ii].pObject_data_byte != NULL)
            {
                free( pCvt2v2->object_info[ii].pObject_data_byte );
                pCvt2v2->object_info[ii].pObject_data_byte = NULL;				
            }
            if( pCvt2v2->cvc_data.data != NULL)
            {
                free( pCvt2v2->cvc_data.data );
                pCvt2v2->cvc_data.data = NULL;
            }

        }

    }


    delete pTCvt;

    return COMM_DL_SUCESS;
}
static unsigned char CodeFileName[256];
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// CommonDownloadVerifyCVT()
// Description:
//
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
static int CommonDownloadVerifyCVT(unsigned char *pData, unsigned long size,int DeferedDL)
{
    TableCVT *pCvt;
    unsigned char FileName[] = "BROADCOM";
    //unsigned char CodeFileName[256];
    unsigned char *pCodeFileName = NULL;
    unsigned long ulFileNameSize = 0;
    unsigned char *pCodeFileNameCvt2v2 = NULL;
    unsigned long ulFileNameSizeCvt2v2 = 0;
    int iRet;
    cvtdownload_data_t* data;
    CVTValidationResult_t *pCVTValid = NULL;
    //pfcEventBase *event=NULL;
    //pfcEventManager *em = pfc_kernel_global->get_event_manager();


    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    rmf_osal_event_handle_t event_handle;
    rmf_osal_event_params_t event_params = {0};	

    if( (pData == NULL) || (size == 0) )
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadVerifyCVT:: Error !!! (pData == 0x%X) || (size == %d)!\n",pData,size);
        return COMM_DL_CVT_DATA_ERROR;
    }
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CommonDownloadVerifyCVT: Entered \n");
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CommonDownloadVerifyCVT: pData:0x%X size:%d \n",pData,size);


    iRet = CommonDownloadGetCodeFileName(&pCodeFileName,&ulFileNameSize);
    if(iRet)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL"," CommonDownloadVerifyCVT : ret value:0x%X of CommonDownloadGetCodeFileName() \n",iRet);
        return iRet;
    }
    else if(pCodeFileName == NULL || ulFileNameSize == 0)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadVerifyCVT: Error pCodeFileName =0x%X || ulFileNameSize == %d \n",pCodeFileName,ulFileNameSize);
        free ( pCodeFileName );
	 pCodeFileName = NULL;	
        return COMM_DL_NV_RAM_NO_DATA_ERROR;
        //pCodeFileName = (unsigned char *)FileName;
        //ulFileNameSize = (unsigned long) strlen((const char*)FileName);
    }
    memset(CodeFileName,0 , sizeof(CodeFileName));
    memcpy(CodeFileName,pCodeFileName,ulFileNameSize);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","\n\n############ Current Image Name Stored in the system ############ \n");
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","  CodeFileName:%s \n",CodeFileName);
    ulFileNameSize = strlen((char*)pCodeFileName);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," FileNameSize:%d\n",ulFileNameSize);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","#####################################################################\n");
    pCVTValid = (CVTValidationResult_t*)malloc( sizeof(CVTValidationResult_t));
    if(pCodeFileName != NULL)
    {
        free(pCodeFileName);
        pCodeFileName = NULL;
    }

    if(pCVTValid == NULL)
    {
       RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadVerifyCVT: Error !! NULL pointer returing from here \n");
       return -1;
    }
    // Create the Table CVT object..
    pCvt = TableCVT::generateInstance((char*)pData, size);


    if(pCvt == NULL)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadVerifyCVT:: pCvt is NULL  Error !!!!\n");
        free(pCVTValid);
        return COMM_DL_CVT_DATA_ERROR;
    }

    //data = pCvt->getCVT_Data();
    data = pCvt->getCVT_DataForUpdate();
    if(pCvt->getCVT_Type() == CVT_T1V1)
    {
        cvt_t1v1_download_data_t* pCvt1 = (cvt_t1v1_download_data_t*)data;
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CommonDownloadVerifyCVT:: Received CVT1 download command:%d \n",pCvt1->download_command);
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","pCvt1->codefile_data.len:%d \n",pCvt1->codefile_data.len);
        if(pCvt1->codefile_data.name)
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","pCvt1->codefile_data.name:%s \n",pCvt1->codefile_data.name);
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","pCvt1->codefile_data.name is NULL Pointer !!!!!!!!!! \n");
            free(pCVTValid);
            return COMM_DL_CVT_DATA_ERROR;
        }
        if(pCvt1->codefile_data.len == ulFileNameSize)
        {
            if(memcmp(pCvt1->codefile_data.name,pCodeFileName,ulFileNameSize) == 0)
            {
                CommonDownloadCVTFree(pCvt);
                //if(pCodeFileName)
                //    free(pCodeFileName);
                free(pCVTValid);
                return COMM_DL_CODE_FILE_NAME_MATCH;
            }
            //Download code image file name is different from the current code image file name ... so it is new image for download
        }
        //if(pCodeFileName)
        //    free(pCodeFileName);


        // Check the download command Deferred or now
        if(pCvt1->download_command == COM_DOWNL_COMMAND_DEFERRED)
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CommonDownloadVerifyCVT1:: Deferred download CVT1 \n");
            pCvt1->download_command = COM_DOWNL_COMMAND_NOW;// change the command to download now.

            if(vlg_DefDownloadPending == FALSE)
            {
                vlg_pDefDLCVT = (unsigned char *)malloc(size);
                if(vlg_pDefDLCVT == NULL)
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadVerifyCVT: NULL POINTER Error !!!!!!!!!!! returning");
                    free(pCVTValid);
                    return iRet;
                }
                memcpy(vlg_pDefDLCVT,pData,size);


                // Save the CVT and its size for download now notification from the monitor appliation.
                // vlg_pDefDLCVT = pData;
                  vlg_DefCVTSize = size;
                 vlg_DefDownloadPending = TRUE;

            //Generate the deferred download system event to the OCAP Monitor Application
                ComDownLDefDownloadNotify();
            }
            //else
            {
                // if  Deferred download is already pending then ignore the CVT
                CommonDownloadCVTFree(pCvt);
            }

        }
        else if (pCvt1->download_command == COM_DOWNL_COMMAND_NOW)
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CommonDownloadVerifyCVT1: validate the  CVT1 \n");
            vlg_DefDownloadPending = FALSE;
                    eCDLM_State = COM_DOWNL_CVT_PROC_AND_VALIDATION;
            iRet = vlg_pCVTValidationMgr->processCVTTrigger(pCvt);
            if(iRet != 0)
            {
                eCDLM_State = COM_DOWNL_IDLE;
                CommonDownloadCVTFree(pCvt);
                CommonDownloadSendSysCntlComm(CERT_FAILURE);
                free(pCVTValid);
                return iRet;
            }
            else
            {
		//pCVTValid = (CVTValidationResult_t*)malloc( sizeof(CVTValidationResult_t));
                pCVTValid->bCVTValid = true;
                pCVTValid->bDownloadStart = true;
                pCVTValid->CvtValdStatus = CVT_VALD_SUCCESS;
                pCVTValid->pCvt = pCvt;

		  event_params.data = pCVTValid;
		  event_params.data_extension = sizeof(CVTValidationResult_t);
		  event_params.free_payload_fn = podmgr_freefn;		  
		  rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_COM_DOWNL_MANAGER, CommonDownL_CVT_Validated, 
		  &event_params, &event_handle);
    		  rmf_osal_eventmanager_dispatch_event(em, event_handle);
                return 0;
            }


        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadVerifyCVT: Received ERROR !!!  download_command:%d \n\n",pCvt1->download_command);
            CommonDownloadCVTFree(pCvt);
            free(pCVTValid);
            return COMM_DL_CVT_DATA_ERROR;
        }
    }
    else if(pCvt->getCVT_Type() == CVT_T2Vx && pCvt->getCVT_Version() == 1)
    {

        cvt_t2v1_download_data_t* pCvt2 = (cvt_t2v1_download_data_t*)data;
        cvcinfo_t cvcInfo;
        cvc_data_t cvcdata;
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CommonDownloadVerifyCVT: Received CVT2 command:%d \n",pCvt2->download_command);

        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CVC count is: %d\n", pCvt2->num_cvcs);
            cvcdata = pCvt2->cvc_data;

            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CVC data len is: %d\n", cvcdata.len);
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CommonDownloadVerifyCVT: cvc_data.data:%X cvc_data.len:%d\n",cvcdata.data,cvcdata.len);
        if( cvcdata.data == NULL || cvcdata.len == 0)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL"," Error returned from getCVCInfoFromNonSignedData()cvcdata.data =0x%X || cvcdata.len:%d \n",cvcdata.data,cvcdata.len);
            free(pCVTValid);
            pCVTValid= NULL;              			
            return -1;
        }
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","\n\n################## CVT code file name ################################### \n");
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL",">>>>>>>>>>  pCvt2->codefile_data.len:%d \n",pCvt2->codefile_data.len);
        if(pCvt2->codefile_data.name)
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," >>>>>>> pCvt2->codefile_data.name:%s \n",pCvt2->codefile_data.name);
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL"," >>>>>>>>>>>> pCvt2->codefile_data.name is NULL Pointer !!!!!!!!!! \n");
            free(pCVTValid);
            pCVTValid = NULL;			
            return -1;
        }
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","\n\n################## CVT code file name ################################### \n");
        if(pCvt2->codefile_data.len == ulFileNameSize)
        {

            if(memcmp(pCvt2->codefile_data.name,pCodeFileName,ulFileNameSize) == 0)
            {
                CommonDownloadCVTFree(pCvt);
                //if(pCodeFileName)
                //    free(pCodeFileName);
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," >>>>>>>>>>>>>>>> NO DOWN LOAD REQUIRED ,,, SAME FILE NAME<<<<<<<<<<<<<<<< \n");
                free(pCVTValid);
                pCVTValid = NULL;				
                return COMM_DL_CODE_FILE_NAME_MATCH;
            }

            //Download code image file name is different from the current code image file name ... so it is new image for download

        }
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," >>>>>>>>>>>>>>>> CODE IMAGE DOWN LOAD REQUIRED <<<<<<<<<<<<<<<< \n");
        //if(pCodeFileName)
        //    free(pCodeFileName);

        // Check the download command Deferred or now
        if( (pCvt2->download_command == COM_DOWNL_COMMAND_DEFERRED) && (DeferedDL == 0))
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CommonDownloadVerifyCVT:: Deffered download\n");

            pCvt2->download_command = COM_DOWNL_COMMAND_NOW;// change the command to download now.
            // Save the CVT and its size for download now notification from the monitor appliation.
            if(vlg_DefDownloadPending == FALSE)
            {
                vlg_pDefDLCVT = (unsigned char *)malloc(size);
                if(vlg_pDefDLCVT == NULL)
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadVerifyCVT: NULL POINTER Error !!!!!!!!!!!");
                    free(pCVTValid);
                    pCVTValid = NULL;					
                    return iRet;
                }
                memcpy(vlg_pDefDLCVT,pData,size);

                  vlg_DefCVTSize = size;
                 vlg_DefDownloadPending = TRUE;
            //Generate the deferred download system event to the OCAP Monitor Application
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," CommonDownloadVerifyCVT: Calling ComDownLDefDownloadNotify\n");
                ComDownLDefDownloadNotify();
            }
            //else
            {
                // if  Deferred download is already pending then ignore the CVT
                // Free the CVT in both case
                CommonDownloadCVTFree(pCvt);
            }
            //pCvt2->download_command = COM_DOWNL_COMMAND_NOW;
        }
        else if ( (pCvt2->download_command == COM_DOWNL_COMMAND_NOW) || DeferedDL)
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CommonDownloadVerifyCVT: validate the  CVT2 download Now\n");
            vlg_DefDownloadPending = FALSE;
            eCDLM_State = COM_DOWNL_CVT_PROC_AND_VALIDATION;

            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CommonDownloadVerifyCVT: calling the function processCVTTrigger() \n");

            iRet = vlg_pCVTValidationMgr->processCVTTrigger(pCvt);
            if(iRet != 0)
            {
                eCDLM_State = COM_DOWNL_IDLE;
                CommonDownloadCVTFree(pCvt);
                CommonDownloadSendSysCntlComm(CERT_FAILURE);
                free(pCVTValid);
                return iRet;
            }
            else
            {
                pCVTValid->bCVTValid = true;
                pCVTValid->bDownloadStart = true;
                pCVTValid->CvtValdStatus = CVT_VALD_SUCCESS;
                pCVTValid->pCvt = pCvt;
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," CommonDownloadVerifyCVT: CVTValid.pCvt :0x%08X \n",pCVTValid->pCvt );
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL",">>>>>>>>>>  pCvt2->codefile_data.len:%d \n",pCvt2->codefile_data.len);
                if(pCvt2->codefile_data.name)
                {
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," >>>>>>> pCvt2->codefile_data.name:%s \n",pCvt2->codefile_data.name);
                }
                else
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL"," >>>>>>>>>>>> pCvt2->codefile_data.name is NULL Pointer !!!!!!!!!! \n");
 		  event_params.data = pCVTValid;
		  event_params.data_extension = sizeof(CVTValidationResult_t);
		  event_params.free_payload_fn = podmgr_freefn;		  
		  rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_COM_DOWNL_MANAGER, CommonDownL_CVT_Validated, 
		  &event_params, &event_handle);
    		  rmf_osal_eventmanager_dispatch_event(em, event_handle);
                return 0;
            }
			

        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadVerifyCVT: Received ERROR !!!  download_command:%d \n\n",pCvt2->download_command);
            CommonDownloadCVTFree(pCvt);
            free(pCVTValid);
            return COMM_DL_CVT_DATA_ERROR;;
        }
    }
    else if( (pCvt->getCVT_Type() == CVT_T2Vx) && (pCvt->getCVT_Version() == 2))
    {

        cvt_t2v2_download_data_t* pCvt2v2 = (cvt_t2v2_download_data_t*)data;
        cvcinfo_t cvcInfo;
        cvc_data_t cvcdata;
        int ii;
        int downloadNow = 0;
        int downloadRequired = 0;

        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CVC count is: %d\n", pCvt2v2->num_cvcs);
        cvcdata = pCvt2v2->cvc_data;

        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CVC data len is: %d\n", cvcdata.len);
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CommonDownloadVerifyCVT: cvc_data.data:%X cvc_data.len:%d\n",cvcdata.data,cvcdata.len);
        if( cvcdata.data == NULL || cvcdata.len == 0)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL"," Error returned from getCVCInfoFromNonSignedData()cvcdata.data =0x%X || cvcdata.len:%d \n",cvcdata.data,cvcdata.len);
            free(pCVTValid);
            return -1;
        }
        for(ii = 0; ii < pCvt2v2->num_of_objects; ii++)
        {
            if( pCvt2v2->object_info[ii].download_command == COM_DOWNL_COMMAND_NOW )
            {
                downloadNow = 1;
            }
        }

        for(ii = 0; ii < pCvt2v2->num_of_objects; ii++)
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CommonDownloadVerifyCVT: Received CVT2 command[%d]:%d \n",ii,pCvt2v2->object_info[ii].download_command);


            iRet = CommonDownloadGetCodeFileNameCvt2v2(&pCodeFileNameCvt2v2, &ulFileNameSizeCvt2v2, pCvt2v2->object_info[ii].object_type);
            if(iRet)
            {
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," CommonDownloadVerifyCVT : ret value:0x%X of CommonDownloadGetCodeFileNameCvt2v2() \n",iRet);
                free(pCVTValid);
                return iRet;
            }
            else if(pCodeFileNameCvt2v2 == NULL || ulFileNameSizeCvt2v2 == 0)
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadVerifyCVT: Error pCodeFileName =0x%X || ulFileNameSize == %d \n",pCodeFileNameCvt2v2,ulFileNameSizeCvt2v2);
                free(pCVTValid);
                return COMM_DL_NV_RAM_NO_DATA_ERROR;
            //    pCodeFileNameCvt2v2 = (unsigned char *)FileName;
            //    ulFileNameSizeCvt2v2 = (unsigned long) strlen((const char*)FileName);
            }

            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","\n\n############ Current Image Name Stored in the system ############ \n");
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","  CodeFileName:%s Type:%d\n",pCodeFileNameCvt2v2,pCvt2v2->object_info[ii].object_type);
            ulFileNameSizeCvt2v2 = strlen((char*)pCodeFileNameCvt2v2);
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," FileNameSize:%d\n",ulFileNameSizeCvt2v2);
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","#####################################################################\n");



            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","\n\n################## CVT code file name ################################### \n");
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL",">>>>>>>>>>  pCvt2v2->codefile_data.len:%d \n",pCvt2v2->object_info[ii].codefile_data.len);
            if(pCvt2v2->object_info[ii].codefile_data.name)
            {
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," >>>>>>> object_type:%d pCvt2v2->codefile_data.name:%s \n",pCvt2v2->object_info[ii].object_type, pCvt2v2->object_info[ii].codefile_data.name);
            }
            else
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL"," >>>>>>>>>>>> pCvt2v2->object_info[%d].codefile_data.name is NULL Pointer !!!!!!!!!! \n",ii);
                free(pCVTValid);
                if(pCodeFileNameCvt2v2 != NULL)
                {
                    free(pCodeFileNameCvt2v2);
                    pCodeFileNameCvt2v2 = NULL;
                }
                return -1;
            }


            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","\n\n################## CVT code file name ################################### \n");
            if(pCvt2v2->object_info[ii].codefile_data.len == ulFileNameSizeCvt2v2)
            {

                if(memcmp(pCvt2v2->object_info[ii].codefile_data.name,pCodeFileNameCvt2v2,ulFileNameSizeCvt2v2) == 0)
                {
                    //CommonDownloadCVTFree(pCvt);
                    //if(pCodeFileName)
                    free(pCvt2v2->object_info[ii].codefile_data.name);
                    pCvt2v2->object_info[ii].codefile_data.name = NULL;
                    pCvt2v2->object_info[ii].downloadRequired = 0;
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," >>>>>>>>>>>>>>>> NO DOWN LOAD REQUIRED ,,, SAME FILE NAME<<<<<<<<<<<<<<<< \n");
                    //return COMM_DL_CODE_FILE_NAME_MATCH;
                }
    else
            {
                pCvt2v2->object_info[ii].downloadRequired = 1;
                downloadRequired++;
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," >>>>>>>>>>>>>>>> CODE IMAGE DOWN LOAD REQUIRED <<<<<<<<<<<<<<<< \n");
            }
                //Download code image file name is different from the current code image file name ... so it is new image for download

            }
            else
            {
                pCvt2v2->object_info[ii].downloadRequired = 1;
                downloadRequired++;
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," >>>>>>>>>>>>>>>> CODE IMAGE DOWN LOAD REQUIRED <<<<<<<<<<<<<<<< \n");
            }
            if(pCodeFileNameCvt2v2 != NULL)
            {
                free(pCodeFileNameCvt2v2);
                pCodeFileNameCvt2v2 = NULL;
            }

        }//for

        if(downloadRequired == 0)
        {
            free(pCVTValid);
            return COMM_DL_CODE_FILE_NAME_MATCH;
        }
        // Check the download command Deferred or now
        if( (downloadNow == 0) && (DeferedDL == 0))
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CommonDownloadVerifyCVT:: Deffered download\n");

            for(ii = 0; ii < pCvt2v2->num_of_objects; ii++)
            {
                pCvt2v2->object_info[ii].download_command = COM_DOWNL_COMMAND_NOW;// change the command to download now.
            }

            // Save the CVT and its size for download now notification from the monitor appliation.
            if(vlg_DefDownloadPending == FALSE)
            {
                vlg_pDefDLCVT = (unsigned char *)malloc(size);
                if(vlg_pDefDLCVT == NULL)
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadVerifyCVT: NULL POINTER Error !!!!!!!!!!!");
                    return iRet;
                }
                memcpy(vlg_pDefDLCVT,pData,size);

                vlg_DefCVTSize = size;
                vlg_DefDownloadPending = TRUE;
                //Generate the deferred download system event to the OCAP Monitor Application
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," CommonDownloadVerifyCVT: Calling ComDownLDefDownloadNotify\n");
                ComDownLDefDownloadNotify();
            }
            //else
            {
                    // if  Deferred download is already pending then ignore the CVT
                CommonDownloadCVTFree(pCvt);
            }

        }
        else if ( (downloadNow == 1) || DeferedDL)
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CommonDownloadVerifyCVT: validate the  CVT2 download Now\n");
            vlg_DefDownloadPending = FALSE;
            eCDLM_State = COM_DOWNL_CVT_PROC_AND_VALIDATION;

            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CommonDownloadVerifyCVT: calling the function processCVTTrigger() \n");

            iRet = vlg_pCVTValidationMgr->processCVTTrigger(pCvt);
            if(iRet != 0)
            {
                eCDLM_State = COM_DOWNL_IDLE;
                CommonDownloadCVTFree(pCvt);
                CommonDownloadSendSysCntlComm(CERT_FAILURE);
                free(pCVTValid);
                return iRet;
            }
            else
            {
                pCVTValid->bCVTValid = true;
                pCVTValid->bDownloadStart = true;
                pCVTValid->CvtValdStatus = CVT_VALD_SUCCESS;
                pCVTValid->pCvt = pCvt;
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," CommonDownloadVerifyCVT: CVTValid.pCvt :0x%08X \n",pCVTValid->pCvt );
                for(ii = 0; ii < pCvt2v2->num_of_objects; ii++)
                {
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL",">>>>>>>>>>  pCvt2v2->[%d]codefile_data.len:%d \n",ii,pCvt2v2->object_info[ii].codefile_data.len);
                    if(pCvt2v2->object_info[ii].codefile_data.name)
                    {
                        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," >>>>>>> pCvt2->codefile_data.name:%s \n",pCvt2v2->object_info[ii].codefile_data.name);
                    }
                    else
                        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL"," >>>>>>>>>>>> pCvt2v2->codefile_data.name is NULL Pointer !!!!!!!!!! \n");
                }

		  event_params.data = pCVTValid;
		  event_params.data_extension = sizeof(CVTValidationResult_t);
		  event_params.free_payload_fn = podmgr_freefn;		  
		  rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_COM_DOWNL_MANAGER, CommonDownL_CVT_Validated, 
		  &event_params, &event_handle);
    		  rmf_osal_eventmanager_dispatch_event(em, event_handle);
                return 0;
            }


        }

    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadVerifyCVT: Received ERROR !!!  CVT \n\n");
        CommonDownloadCVTFree(pCvt);
        PostBootEvent(RMF_BOOTMSG_DIAG_EVENT_TYPE_CDL, RMF_BOOTMSG_DIAG_MESG_CODE_ERROR, 0xF1);
        free(pCVTValid);
        return COMM_DL_CVT_DATA_ERROR;
    }
	 if(pCVTValid)
	{
		free(pCVTValid);
		pCVTValid = NULL;
	}

    return 0;
}
#if 0
int testCVT(char *buf, int len)
{
  TableCVT* cvt = TableCVT::generateInstance(buf, len);

  cvtdownload_data_t* data = cvt->getCVT_Data();

  RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CVT type is: 0x%X\n", cvt->getCVT_Type());
  if(cvt->getCVT_Type() == CVT_T1V1)
  {
    cvt_t1v1_download_data_t* cvt_t1v1 = (cvt_t1v1_download_data_t*)data;

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","VendorID is: 0x%X\n", cvt_t1v1->vendorId);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","Hardware Version is: 0x%X\n", cvt_t1v1->hardwareVersionId);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","Download Type is: 0x%X\n", data->downloadtype);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","Download Command is: 0x%X\n", cvt_t1v1->download_command);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","Freq Vector is: 0x%X\n", cvt_t1v1->freq_vector);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","Transport Value is: 0x%X\n", cvt_t1v1->transport_value);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","PID is: 0x%X\n", cvt_t1v1->PID);
    codefile_data_t codefile = cvt_t1v1->codefile_data;

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","Codefile len is %d\n", codefile.len);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","Codefile is: ");
    for(int i = 0; i < codefile.len; i++)
    {
      RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","%c", *((codefile.name)+i));
    }
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","\n");

    cvc_data_t cvcdata = cvt_t1v1->cvc_data;

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CVC data len is: %d\n", cvcdata.len);

    free(codefile.name);
    free(cvcdata.data);
  }
  else if(cvt->getCVT_Type() == CVT_T2Vx)
  {
    cvt_t2v1_download_data_t* cvt_t2v1 = (cvt_t2v1_download_data_t*)data;

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","VendorID is: 0x%X\n", cvt_t2v1->vendorId);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","Hardware Version is: 0x%X\n", cvt_t2v1->hardwareVersionId);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","Protocol Version is: 0x%X\n", cvt_t2v1->protocol_version);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","Configuration Change Count is: %d\n", cvt_t2v1->config_count_change);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","Download Command is: 0x%X\n", cvt_t2v1->download_command);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","Location Type is: 0x%X\n", cvt_t2v1->location_type);

    switch(cvt_t2v1->location_type)
    {
      case CVT_T2Vx_LOC_SOURCE_ID:
      {
        location_t0_t* locdata = (location_t0_t*)&(cvt_t2v1->loc_data);
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","Source ID is: 0x%X\n", locdata->sourceId);
        break;
      }
      case CVT_T2Vx_LOC_FREQ_PID:
      {
        location_t1_t* locdata = (location_t1_t*)&(cvt_t2v1->loc_data);
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","Freq Vector is: 0x%X\n", locdata->freq_vector);
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","Modulation Type is: 0x%X\n", locdata->modulation_type);
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","PID is: 0x%X\n", locdata->PID);
        break;
      }
      case CVT_T2Vx_LOC_FREQ_PROG:
      {
        location_t2_t* locdata = (location_t2_t*)&(cvt_t2v1->loc_data);
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","Freq Vector is: 0x%X\n", locdata->freq_vector);
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","Modulation Type is: 0x%X\n", locdata->modulation_type);
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","Prog Num is: 0x%X\n", locdata->program_num);
        break;
      }
      case CVT_T2Vx_LOC_BASIC_DSG:
      {
        location_t3_t* locdata = (location_t3_t*)&(cvt_t2v1->loc_data);
#if 0
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","Basic DSG Tunnel Addr is: 0x%04X", ((locdata->dsg_tunnel_addr) >> 32));
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","%08X\n", (locdata->dsg_tunnel_addr & 0xFFFFFFFF));
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","Src Addr is: 0x%08X", ((locdata->src_ip_addr_upper) >> 32));
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","%08X", ((locdata->src_ip_addr_upper) & 0xFFFFFFFF));
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","%08X", ((locdata->src_ip_addr_lower) >> 32));
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","%08X\n", (locdata->src_ip_addr_lower & 0xFFFFFFFF));
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","Src Port is: %d\n", locdata->src_port);
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","Dst Addr is: 0x%08X", ((locdata->dst_ip_addr_upper) >> 32));
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","%08X", ((locdata->dst_ip_addr_upper) & 0xFFFFFFFF));
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","%08X", ((locdata->dst_ip_addr_lower) >> 32));
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","%08X\n", (locdata->dst_ip_addr_lower & 0xFFFFFFFF));
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","Dst Port is: %d\n", locdata->dst_port);
#endif
        break;
      }
      case CVT_T2Vx_LOC_ADV_DSG:
      {
        location_t4_t* locdata = (location_t4_t*)&(cvt_t2v1->loc_data);
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","Adv DSG Tunnel App Id is: 0x%X\n", locdata->app_id);
        break;
      }
      case CVT_T2Vx_LOC_TFTP:
      {
        location_tftp_t* locdata = (location_tftp_t*)&(cvt_t2v1->loc_data);
#if 0
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","TFTP Server Addr is: 0x%08X", ((locdata->tftp_addr_upper) >> 32));
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","%08X", ((locdata->tftp_addr_upper) & 0xFFFFFFFF));
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","%08X", ((locdata->tftp_addr_lower) >> 32));
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","%08X\n", (locdata->tftp_addr_lower & 0xFFFFFFFF));
#endif
        break;
      }
    }

    codefile_data_t codefile = cvt_t2v1->codefile_data;

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","Codefile len is %d\n", codefile.len);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","Codefile is: ");
    for(int i = 0; i < codefile.len; i++)
    {
      RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","%c", *((codefile.name)+i));
    }
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","\n");

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CVC count is: %d\n", cvt_t2v1->num_cvcs);
    cvc_data_t cvcdata = cvt_t2v1->cvc_data;

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CVC data len is: %d\n", cvcdata.len);


    free(codefile.name);
    free(cvcdata.data);
  }

  delete cvt;
  return EXIT_SUCCESS;
}
#endif
static char tempName[25] = {"VividLogic"};
int CommonDownloadMgrSnmpHostFirmwareStatus(CDLMgrSnmpHostFirmwareStatus_t *pStatus)
{
    memset(pStatus,0, sizeof(CDLMgrSnmpHostFirmwareStatus_t));
    // Fill the real data
#if 0
    if((eCDLM_State == COM_DOWNL_IDLE) || (eCDLM_State == COM_DOWNL_CVT_PROC_VALIDATION_START) || (eCDLM_State == COM_DOWNL_CVT_PROC_AND_VALIDATION))
    {
        //FileName_TobeLoaded_Size = 0;
//         pStatus->HostFirmwareImageStatus = 0;
//         pStatus->HostFirmwareCodeDownloadStatus = 0;
//         pStatus->HostFirmwareDownloadFailedStatus = 0;
        /*  default values*/
        pStatus->HostFirmwareImageStatus = IMAGE_AUTHORISED;
         pStatus->HostFirmwareCodeDownloadStatus = DOWNLOADING_COMPLETE;
         pStatus->HostFirmwareDownloadFailedStatus = CDL_ERROR_1;
    }
    else
#endif
    {
        pStatus->HostFirmwareImageStatus = vlg_CDLFirmwareImageStatus;
        pStatus->HostFirmwareCodeDownloadStatus = vlg_CDLFirmwareCodeDLStatus;
        pStatus->HostFirmwareDownloadFailedStatus = vlg_CDLFirmwareDLFailedStatus;

    }
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL",">>>>>>>>>> HostFirmwareImageStatus:%d \n",pStatus->HostFirmwareImageStatus);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL",">>>>>>>>>> HostFirmwareCodeDownloadStatus:%d \n",pStatus->HostFirmwareCodeDownloadStatus);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL",">>>>>>>>>> HostFirmwareDownloadFailedStatus:%d \n",pStatus->HostFirmwareDownloadFailedStatus);
    
    pStatus->HostFirmwareCodeObjectNameSize = FileName_TobeLoaded_Size;
    memcpy(pStatus->HostFirmwareCodeObjectName,FileName_TobeLoaded,FileName_TobeLoaded_Size);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","HostFirmwareCodeObjectNameSize:%d \n",pStatus->HostFirmwareCodeObjectNameSize);
    
    
    return CLD_SNMP_SUCCESS;
}
void  PrintSnmpCertificateStatus(CDLMgrSnmpCertificateStatus_t *pStatus)
{
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","================ Printing the CVC Cert Info ================== \n");
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","bCLRootCACertAvailable:%d \n",pStatus->bCLRootCACertAvailable);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","bCvcCACertAvailable:%d \n",pStatus->bCvcCACertAvailable);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","bDeviceCVCCertAvailable:%d \n",pStatus->bDeviceCVCCertAvailable);

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","pCLRootCASubjectName:%s \n",pStatus->pCLRootCASubjectName);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","pCLRootCAStartDate:%s \n",pStatus->pCLRootCAStartDate);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","pCLRootCAEndDate:%s \n",pStatus->pCLRootCAEndDate);

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","pCLCvcCASubjectName:%s \n",pStatus->pCLCvcCASubjectName);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","pCLCvcCAStartDate:%s \n",pStatus->pCLCvcCAStartDate);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","pCLCvcCAEndDate:%s \n",pStatus->pCLCvcCAEndDate);

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","pDeviceCvcSubjectName:%s \n",pStatus->pDeviceCvcSubjectName);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","pDeviceCvcStartDate:%s \n",pStatus->pDeviceCvcStartDate);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","pDeviceCvcEndDate:%s \n",pStatus->pDeviceCvcEndDate);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","bVerifiedWithChain:%d \n",pStatus->bVerifiedWithChain);
}
int CommonDownloadMgrGetSnmpInfo( char *pSubjectName,  int iSizeSubjectName, char *pStart, char *pEnd, cvcinfo_t  Cvcinfo)
{
    memset(pSubjectName, 0, iSizeSubjectName);
    if(Cvcinfo.subjectCommonName[0] == '=')
    {
          strncpy( pSubjectName, (char *)((char *)Cvcinfo.subjectCommonName+1), iSizeSubjectName-1);
    }
    else
    {
        strncpy( pSubjectName, (char *)Cvcinfo.subjectCommonName, iSizeSubjectName-1);
    }  
    snprintf(pStart,CDL_MAX_NAME_STR_SIZE,"Y:20%c%c M:%c%c D:%c%c Time: %c%c:%c%c:%c%c",Cvcinfo.notBefore[0],Cvcinfo.notBefore[1],Cvcinfo.notBefore[2],Cvcinfo.notBefore[3],Cvcinfo.notBefore[4],Cvcinfo.notBefore[5],Cvcinfo.notBefore[6],Cvcinfo.notBefore[7],Cvcinfo.notBefore[8],Cvcinfo.notBefore[9],Cvcinfo.notBefore[10],Cvcinfo.notBefore[11]);

    snprintf(pEnd,CDL_MAX_NAME_STR_SIZE,"Y:20%c%c M:%c%c D:%c%c Time: %c%c:%c%c:%c%c",Cvcinfo.notAfter[0],Cvcinfo.notAfter[1],Cvcinfo.notAfter[2],Cvcinfo.notAfter[3],Cvcinfo.notAfter[4],Cvcinfo.notAfter[5],Cvcinfo.notAfter[6],Cvcinfo.notAfter[7],Cvcinfo.notAfter[8],Cvcinfo.notAfter[9],Cvcinfo.notAfter[10],Cvcinfo.notAfter[11]);
    return 0; //Mamidi:042209
}


int CommonDownloadMgrSnmpCertificateStatus(CDLMgrSnmpCertificateStatus_t *pStatus)
{
    int iRet;
    iRet = CommonDownloadMgrSnmpCertificateStatus_check(pStatus);
    PrintSnmpCertificateStatus(pStatus);

    return iRet;
}

int TestFileUpgrade()
{
      int iRet;
      VL_CDL_IMAGE ImageNamePath;
      char pImage[32] = "/home/Kernel";
      char FileName_TobeLoaded[32] = "TestFile";
      int FileName_TobeLoaded_Size;
      
      FileName_TobeLoaded_Size = strlen(FileName_TobeLoaded);
           
      rmf_osal_threadSleep(1000 * 10,0);
      RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," #######################TestFileUpgrade  UPGRADING THE SOFTWARE  ################# \n");
      RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","Upgrade File :%s \n",pImage);
        //Pass image data pointer, and its length
      ImageNamePath.pszImagePathName = pImage;

      memcpy(ImageNamePath.szSoftwareImageName, FileName_TobeLoaded,FileName_TobeLoaded_Size);

      ImageNamePath.eImageSign = VL_CDL_IMAGE_UNSIGNED;
      ImageNamePath.eImageType = VL_CDL_IMAGE_TYPE_MONOLITHIC;



    iRet = CHALCdl_notify_cdl_mgr_event(VL_CDL_MANAGER_EVENT_UPGRADE_TO_IMAGE,(unsigned long) &ImageNamePath);
    if(VL_CDL_RESULT_SUCCESS != iRet )
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","#######################TestFileUpgrade  UPGRADING THE SOFTWARE Failed with Error:0x%X \n",iRet);
        return -1;
    }
    
        rmf_osal_threadSleep(1000 * 5,0);
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," #######################TestFileUpgrade  COM_DOWNL_START_REBOOT  ################# \n");
    iRet = CHALCdl_notify_cdl_mgr_event(VL_CDL_MANAGER_EVENT_REBOOT, 0);
    if(VL_CDL_RESULT_SUCCESS != iRet )
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","#######################TestFileUpgrade  COM_DOWNL_START_REBOOT Failed with Error:0x%X \n",iRet);
        return -1;
    }
    return 0;


}
static char vlg_FirstTimeCall = 0;


int CommonDownloadMgrSetPublicKeyAndCvc()
{
    unsigned char *ManCVC = NULL;
    unsigned long  MancvcLen;
    unsigned char *PublickKey = NULL;
    unsigned long  PublickKeyLen;
    if( CommonDownloadGetManCvc(&ManCVC,&MancvcLen) != 0)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadMgrSetPublicKeyAndCvc:CommonDownloadGetManCvc returned Error \n");
        if(ManCVC)
        {
            free(ManCVC);
        }
        return -1;

    }
    //DumpBuffer(ManCVC, MancvcLen);
    if( CommonDownloadGetCvcCAPublicKey(&PublickKey,&PublickKeyLen) != 0)
    {
#ifndef USE_FILEDATA
	    //Fix for SAM7435-386.Freeing up memory to fix memory leak
	 if(ManCVC)
        {
	     free(ManCVC);
        }
        ManCVC = NULL;
	 if(PublickKey)
        {
	     free(PublickKey);
        }
        PublickKey = NULL;
#endif
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadMgrSetPublicKeyAndCvc:CommonDownloadGetManCvc returned Error \n");
        return -1;

    }
    //DumpBuffer(PublickKey, PublickKeyLen);
#if 1
    {
          
      VL_CDL_PUBLIC_KEY publicKey;
      memset(&publicKey,0,sizeof(publicKey));
      publicKey.publicKey.nBytes = PublickKeyLen;//sizeof(vlg_PacecvccaPubKey);//cvcCAKeyLen;
      publicKey.publicKey.pData  = PublickKey;//vlg_PacecvccaPubKey;//cvcCAKey;
      RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL",">>>>> CommonDownloadMgrSetPublicKeyAndCvc: From NVRAM publicKey.publicKey.nBytes:%d \n",publicKey.publicKey.nBytes);
      
      CHALCdl_notify_cdl_mgr_event( VL_CDL_MANAGER_EVENT_SET_PUBLIC_KEY, (unsigned long)&publicKey);

    }
#endif        


#if 1        
    {
      VL_CDL_CV_CERTIFICATE cvc;
      memset(&cvc,0,sizeof(cvc));
      cvc.eCvcType = VL_CDL_CVC_TYPE_MANUFACTURER;
      cvc.cvCertificate.nBytes = MancvcLen;//sizeof(vlg_VividCVC);//MancvcLen;
      cvc.cvCertificate.pData  = ManCVC;//vlg_VividCVC;//vlg_Hitron_cvc;//ManCVC;
      RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL",">>>>> CommonDownloadMgrSetPublicKeyAndCvc cvc.cvCertificate.nBytes:%d \n",cvc.cvCertificate.nBytes);
      CHALCdl_notify_cdl_mgr_event( VL_CDL_MANAGER_EVENT_SET_CV_CERTIFICATE, (unsigned long)&cvc);
    }
    //Fix for SAM7435-386.Freeing up memory to fix memory leak
    if(ManCVC)
    {
        free(ManCVC);
    }
    ManCVC = NULL;
    if(PublickKey)
    {
        free(PublickKey);
    }
    PublickKey = NULL;
    return 0;
#endif 

}
int CommonDownloadMgrSnmpCertificateStatus_check(CDLMgrSnmpCertificateStatus_t *pStatus)
{
    unsigned char *pData = NULL;
    unsigned long size;
    unsigned char     *cvcRoot = NULL, *cvcCA = NULL, *ManCVC = NULL;
      unsigned long     cvcRootLen,cvcCALen,MancvcLen;
      unsigned char     *cvcRootKey = NULL, *cvcCAKey = NULL;
      int     cvcRootKeyLen;
    unsigned long cvcCAKeyLen;
    int CDLCertCheckt = 0;
    cvcinfo_t    cvcCLRootInfo;
    cvcinfo_t    cvcCLCAInfo;
    cvcinfo_t    cvcDeviceInfo;
    
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CommonDownloadMgrSnmpCertificateStatus: Entered \n");

    memset(pStatus,0,sizeof(CDLMgrSnmpCertificateStatus_t));


    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," ######################## CommonDownloadMgrSnmpCertificateStatus ################# \n");
    if( CommonDownloadGetCLCvcRootCA(&cvcRoot, &cvcRootLen) != 0)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadMgrSnmpCertificateStatus:CommonDownloadGetCLCvcRootCA returned Error \n");
        CDLCertCheckt++;
    
    }
    else
    {
        pStatus->bCLRootCACertAvailable = 1;
    }
    if( CommonDownloadGetCLCvcCA(&cvcCA, &cvcCALen) != 0)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadMgrSnmpCertificateStatus:CommonDownloadGetCLCvcCA returned Error \n");
        CDLCertCheckt++;
    
    }
    else
    {
        pStatus->bCvcCACertAvailable = 1;
    }
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL",">>>>>>>>>>>> CVC CA certificate >>>>>>>>>>>>>>>>>>> \n");
    //DumpBuffer(cvcCA, cvcCALen);
    
    if( CommonDownloadGetManCvc(&ManCVC,&MancvcLen) != 0)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadMgrSnmpCertificateStatus:CommonDownloadGetManCvc returned Error \n");
        CDLCertCheckt++;
    

    }
    else
    {
        pStatus->bDeviceCVCCertAvailable = 1;
    }


    if(CDLCertCheckt)
    {
      if(cvcRoot)
           free(cvcRoot);
      if(cvcCA)
           free(cvcCA);
      if(ManCVC)
        free(ManCVC);
        
        return -1;
    }
    if(vlg_FirstTimeCall == 0)
    {
        vlg_FirstTimeCall = 1;
        #ifndef USE_FILEDATA
        free(cvcRoot);
        free(cvcCA);
        free(ManCVC);
          #endif
        return -1;
    }
    if(pStatus->bCLRootCACertAvailable)
    {
#ifdef PKCS_UTILS    

        if(getCVCInfoFromNonSignedData(cvcRoot, (int)cvcRootLen, &cvcCLRootInfo) < 0)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadMgrSnmpCertificateStatus::%s: Failed to get CVC Root Information!\n", __FUNCTION__);
                free(cvcRoot);
                free(cvcCA);
                free(ManCVC);
            return -1;
        }
#endif        
        CommonDownloadMgrGetSnmpInfo(pStatus->pCLRootCASubjectName,sizeof(pStatus->pCLRootCASubjectName),pStatus->pCLRootCAStartDate,pStatus->pCLRootCAEndDate,cvcCLRootInfo);
        PrintfCVCInfo(cvcCLRootInfo);
    }

    

    if(pStatus->bCvcCACertAvailable)
    {
#ifdef PKCS_UTILS      
        if(getCVCInfoFromNonSignedData(cvcCA, (int)cvcCALen, &cvcCLCAInfo) < 0)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadMgrSnmpCertificateStatus::%s: Failed to get CVC CA Information!\n", __FUNCTION__);
                free(cvcRoot);
                free(cvcCA);
                free(ManCVC);

            return -1;
        }
#endif        
        CommonDownloadMgrGetSnmpInfo(pStatus->pCLCvcCASubjectName,sizeof(pStatus->pCLCvcCASubjectName),pStatus->pCLCvcCAStartDate,pStatus->pCLCvcCAEndDate,cvcCLCAInfo);
        PrintfCVCInfo(cvcCLCAInfo);
    }
    

    if(pStatus->bDeviceCVCCertAvailable)
    {
#ifdef PKCS_UTILS      
          if(getCVCInfoFromNonSignedData(ManCVC, (int)MancvcLen, &cvcDeviceInfo) < 0)
          {
              RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadMgrSnmpCertificateStatus::%s: Failed to get CVC Device Information!\n", __FUNCTION__);
                  free(cvcRoot);
                  free(cvcCA);
                  free(ManCVC);


              return -1;
          }
#endif          
          CommonDownloadMgrGetSnmpInfo(pStatus->pDeviceCvcSubjectName,sizeof(pStatus->pDeviceCvcSubjectName),pStatus->pDeviceCvcStartDate,pStatus->pDeviceCvcEndDate,cvcDeviceInfo);
          PrintfCVCInfo(cvcDeviceInfo);
    }
    
    
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," ######################## CommonDownloadMgrSnmpCertificateStatus ################# \n");


#ifdef PKCS_UTILS
      if(CDLCertCheckt == 0)
    {
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," ######################## CommonDownloadMgrSnmpCertificateStatus Reading Cert is SUCCESS################# \n");
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL","CDownloadCVTValidationMgr: Going to Verify CLCvcCA\n");
        //PrintBuff(cvcRoot, cvcRootLen);
        
        if(getPublicKeyFromCVC(cvcRoot, cvcRootLen, &cvcRootKey, &cvcRootKeyLen))
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CDownloadCVTValidationMgr::%s: Failed to get PubKey from CL Root CVC in NVM!\n", __FUNCTION__);
            free(cvcRoot);
            free(cvcCA);
            free(ManCVC);

            return -1;
        }
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," >>>>>>... cvcRootKeyLen:%d \n",cvcRootKeyLen);
        if(validateCertSignature(cvcCA, cvcCALen, cvcRootKey, cvcRootKeyLen))
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CDownloadCVTValidationMgr::%s: CVC data failed to validate to CL CVC Root PublicKey!\n", __FUNCTION__);
            free(cvcRoot);
            free(cvcCA);
            free(ManCVC);

            return -1;
        }
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," ##################################################\n");
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," >>>> SUCESS verifying CL CVC CA signature <<<<<<<< \n");
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," ##################################################\n");
        if(getPublicKeyFromCVC(cvcCA, (int)cvcCALen, &cvcCAKey, (int*)&cvcCAKeyLen))
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CDownloadCVTValidationMgr::%s: Failed to get PubKey from CL Root CVC in NVM!\n", __FUNCTION__);
            free(cvcRoot);
            free(cvcCA);
            free(ManCVC);

            return -1;
        }

        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," >>>>>>... cvcCAKeyLen:%d \n",cvcCAKeyLen);
    //    DumpBuffer(cvcCAKey, cvcCAKeyLen);
        //cvcCAKeyLen = 270;
        if(validateCertSignature(ManCVC, MancvcLen, cvcCAKey, cvcCAKeyLen))
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CDownloadCVTValidationMgr::%s: CVC data failed to validate to CL CVC Root PublicKey!\n", __FUNCTION__);
            free(cvcRoot);
            free(cvcCA);
            free(ManCVC);

            return -1;
        }
        pStatus->bVerifiedWithChain = 1;
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," ##################################################\n");
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," >>>> SUCESS verifying  CVC signature <<<<<<<< \n");
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," ##################################################\n");
    }
#endif    
#ifndef USE_FILEDATA
    free(cvcRoot);
    free(cvcCA);
    free(ManCVC);
#endif
//TestFileUpgrade();
    return COMM_DL_SUCESS;
}

static void PrintBuff(unsigned char *pData, unsigned long Size)
{
    int ii;
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","Length:%d \n",Size);
    for(ii = 0; ii < Size; ii++)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","0x%02X, ",pData[ii]);
        if( ((ii+1) % 16) == 0)
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","\n");
        }
    }

}

#if 0
void vlTestSignedImagesStoredLocally()
{
    int iRet,ii=4;
    char InFile[] = "/cdl/TestFiles/InFile";
    char OutFile[] = "/cdl/TestFiles/OutFile";
    int InFileSize,OutFileSize;
    CommonDLObjType_e type;

    //for(ii = 0; ii < 3; ii++)
    {
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.CDL"," ##########################  TEST START:%d ########################## \n",ii);

        //sprintf(InFile,"/cdl/TftpDLFile:%d",ii);
        InFileSize = strlen(InFile);

        switch(ii)
        {
            case 0:
            type = COMM_DL_FIRMWARE_IMAGE;
            break;
            case 1:
            type = COMM_DL_APP_IMAGE;
            break;
            case 2:
            type = COMM_DL_DATA_IMAGE;
            break;
            default:
             type=COMM_DL_MONOLITHIC_IMAGE;

        }



        iRet = vlg_pCodeImageValidationMgr->processCodeImage(InFile,(InFileSize),type);
        if(iRet != 0)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","vlTestSignedImagesStoredLocally:######  Verification Failed:%d  #########\n",ii);
        }
        else
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," ##########################  Image Verification SUCCESS:%d ########################## \n",ii);
        }
    }
    //for(ii = 0; ii < 3; ii++)
    {
        //sprintf(InFile,"/cdl/TftpDLFile:%d",ii);
        InFileSize = strlen(InFile);

        OutFileSize = strlen(OutFile);
#ifdef PKCS_UTILS        
        iRet = getSignedContent(InFile,InFileSize,(char)1,(char **)&OutFile,(int*)&OutFileSize);
        if(iRet != 0)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","vlTestSignedImagesStoredLocally: ######### getSignedContent Failed:%d ######### \n",ii);
        }
        else
        {
            remove(OutFile);
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," ##########################  Image Extraction SUCCESS:%d ########################## \n",ii);
        }
#endif        
    }
    
}
bool vlIsCdlDeferredRebootEnabled()
{
    struct stat st;
    bool ret=false;

    if((0 == stat("/tmp/cdl_deferred_reboot_enabled", &st) && (0 != st.st_ino)))
    {
        ret=true;
        MPEOS_LOG(MPE_LOG_INFO, MPE_MOD_CDL,"KYW: /tmp/cdl_deferred_reboot_enabled exist, %d \n", ret );
    }
    return ret;

}
#endif

#if 0
int CommonDownloadMgrSnmpCertificateStatus()
{
    unsigned char *pData;
    unsigned long size;
    unsigned char     *cvcRoot,*cvcCA,*ManCVC;
      unsigned long     cvcRootLen,cvcCALen,MancvcLen;
      unsigned char     *cvcRootKey,*cvcCAKey;
      int     cvcRootKeyLen;
    unsigned long cvcCAKeyLen;
    CodeAccessStart_t CodeAccessTime;
    cvcAccessStart_t  CvcAccessTime;
    unsigned char *pFileName;
    unsigned long FileSize;
    int CDLCertCheckt = 0;
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CommonDownloadMgrSnmpCertificateStatus: Entered \n");



    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," ######################## CommonDownloadMgrSnmpCertificateStatus ################# \n");
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," ######################## CommonDownloadMgrSnmpCertificateStatus ################# \n");
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," ######################## CommonDownloadMgrSnmpCertificateStatus ################# \n");
    if( CommonDownloadGetCLCvcRootCA(&cvcRoot, &cvcRootLen) != 0)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CommonDownloadMgrSnmpCertificateStatus:CommonDownloadGetCLCvcRootCA returned Error \n");
        CDLCertCheckt++;
    }
    else
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," ################### CL ROOT CVC ############### \n");
        PrintBuff(cvcRoot, cvcRootLen);
    }

    if( CommonDownloadGetCLCvcCA(&cvcCA, &cvcCALen) != 0)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","CommonDownloadMgrSnmpCertificateStatus:CommonDownloadGetCLCvcCA returned Error \n");
        CDLCertCheckt++;
    }
    else
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," ################### CL  CVC  CA############### \n");
        PrintBuff(cvcCA, cvcCALen);
    }

    if( CommonDownloadGetManCvc(&ManCVC,&MancvcLen) != 0)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadMgrSnmpCertificateStatus:CommonDownloadGetManCvc returned Error \n");
        CDLCertCheckt++;

    }
    else
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," ################### MAN CVC ############### \n");
        PrintBuff(ManCVC, MancvcLen);
    }

    if( CommonDownloadGetCvcCAPublicKey(&cvcCAKey, &cvcCAKeyLen) != 0)
    {
        CDLCertCheckt++;
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadMgrSnmpCertificateStatus:CommonDownloadGetCvcCAPublicKey returned Error \n");

    }
    else
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," ################### Public Key ############### \n");
        PrintBuff(cvcCAKey, cvcCAKeyLen);
    }

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," ######################## CommonDownloadMgrSnmpCertificateStatus ################# \n");


      if(CDLCertCheckt == 0)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," ######################## CommonDownloadMgrSnmpCertificateStatus reading SUCCESS################# \n");
    }

    pFileName = CommonDownloadGetManufacturerName();
    if(pFileName == NULL)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadMgrSnmpCertificateStatus:CommonDownloadGetManufacturerName returned Error \n");

    }
    else
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," ################### Manufacturer Name ############### \n");
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," Man Name:%s \n",pFileName);
    }

    pFileName = CommonDownloadGetCosignerName();
    if(pFileName == NULL)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadMgrSnmpCertificateStatus:CommonDownloadGetCosignerName returned Error \n");

    }
    else
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," ################### Co signer Name ############### \n");
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," Co signer Name:%s \n",pFileName);
    }

    if( 0 != CommonDownloadGetCodeFileName(&pFileName,&FileSize))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadMgrSnmpCertificateStatus:CommonDownloadGetCodeFileName returned Error \n");

    }
    else
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," ################### Code File Name ############### \n");
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL","pFileName:%s\n",pFileName);
    }

    if( 0 != CommonDownloadGetVendorId(&pFileName,&FileSize))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadMgrSnmpCertificateStatus:CommonDownloadGetVendorId returned Error \n");

    }
    else
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," ################### Vendor Id ############### \n");
        PrintBuff(pFileName, FileSize);
    }


    if( 0 != CommonDownloadGetHWVerId(&pFileName,&FileSize))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadMgrSnmpCertificateStatus:CommonDownloadGetHWVerId returned Error \n");

    }
    else
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," ################### Hardware Id ############### \n");
        PrintBuff(pFileName, FileSize);
    }




    if( 0 != CommonDownloadGetCodeAccessStartTime(&CodeAccessTime,COM_DOWNL_MAN_CVC))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadMgrSnmpCertificateStatus:CommonDownloadGetCodeAccessStartTime returned Error COM_DOWNL_MAN_CVC \n");

    }
    else
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," ################### COM_DOWNL_MAN_CVC code access start time  ############### \n");
        PrintBuff(CodeAccessTime.Time, 12);
    }
    if( 0 != CommonDownloadGetCodeAccessStartTime(&CodeAccessTime,COM_DOWNL_COSIG_CVC))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadMgrSnmpCertificateStatus:CommonDownloadGetCodeAccessStartTime returned Error COM_DOWNL_COSIG_CVC\n");

    }
    else
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," ################### COM_DOWNL_COSIG_CVC code access start time  ############### \n");
        PrintBuff(CodeAccessTime.Time, 12);
    }


    if( 0 != CommonDownloadGetCvcAccessStartTime(&CvcAccessTime,COM_DOWNL_MAN_CVC))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadMgrSnmpCertificateStatus:CommonDownloadGetCvcccessStartTime returned Error COM_DOWNL_MAN_CVC\n");

    }
    else
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," ################### COM_DOWNL_MAN_CVC CVC Access start time ############### \n");
        PrintBuff(CodeAccessTime.Time, 12);
    }
    if( 0 != CommonDownloadGetCvcAccessStartTime(&CvcAccessTime,COM_DOWNL_COSIG_CVC))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.CDL","CommonDownloadMgrSnmpCertificateStatus:CommonDownloadGetCvcccessStartTime returned Error COM_DOWNL_COSIG_CVC\n");

    }
    else
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.CDL"," ################### COM_DOWNL_COSIG_CVC cCVC Access start time ############### \n");
        PrintBuff(CodeAccessTime.Time, 12);
    }


    return COMM_DL_SUCESS;
}
#endif
