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


#ifndef _CDLMgr_H_
#define _CDLMgr_H_

#include "cmThreadBase.h"
//#include "pfccondition.h"
//#include "pfcresource.h"
#include "tablecvt.h"
#include "pkcs7SinedDataInfo.h"
#include "cdl_mgr_types.h"
#include "rmf_osal_event.h"

//This structure is passed with the  event "CommonDownL_CVT_Validated" from CVT validator....
typedef struct
{
    char  bCVTValid; // true or false, if CVT valid then Download params are valid otherwise invalid..
    char  bDownloadStart;//true of false if bDownloadStart is true then Download params are valid otherwise invalid..
    CVTValidationStatus_e CvtValdStatus;//Status  after the CVT process and validation including CVC
    TableCVT *pCvt;

}CVTValidationResult_t;
typedef struct
{
    unsigned char *pCLCvcRootCaCert;
    unsigned char *pCLCvcCaCert;
    unsigned char *pCLCvcDeviceCaCert;
    unsigned char *pCodeImage;

    unsigned long CLCvcRootCaCertLen;
    unsigned long CLCvcCaCertLen;
    unsigned long CLCvcDeviceCaCertLen;
    unsigned long CodeImageLen;

// Add the code
    //codeAccessStart;
    //cvcAccessStart;
}CommonDownLImage_t;

class CommonDownloadManager : public CMThread
{
public:

    CommonDownloadManager();
    ~CommonDownloadManager();
    int initialize(void);
    int is_default();
    void   run();

    static void PostCDLBootMsgDiagEvent(long msgCode, int data);
        static void PostCDLBootMsgFlashWriteDiagEvent(long msgCode); 
private:
    rmf_osal_eventqueue_handle_t event_queue;

};
static unsigned short CommonDownloadTLVTypeLen(unsigned char *pData,unsigned short size,unsigned char **pRetData, unsigned short *pSize);
static int CommonDownloadVerifyCVT(unsigned char *pData, unsigned long size,int DefDL);
static int CommonDownloadParseTLVsForCerts(unsigned char *pData, unsigned short size,unsigned char **pRetData, unsigned short *pSize,CommonDownLImage_t *pCodeImage);
int CommonDownloadCVTFree(TableCVT *pCvt);
int CommonDownloadSendSysCntlComm(CommonDownloadHostCmd_e command );
int ComDownLDefDownloadNotify();
static int CommonDownloadUpgradeDownloadParams( char *pFileName,char *pImage);
void ComDownLEngineGotResponseFromTftp(CommonDownloadTftpDLStatus_e Status,void *pParams);
void PrintfCVCInfo(cvcinfo_t cvcInfo);
int vlSetCvtTftpDLstarted(int val);
int vlSetCvtTftDLNumOfObjs(int val);
int CommonDownloadMgrSetPublicKeyAndCvc();
//void vlCommonDownloadMgrUpdateFrontPanel( VL_BOOTMSG_DIAG_EVENT_TYPE event_type, VL_BOOTMSG_DIAG_MESG_CODE msgCode, int data);
void vlCommonDownloadMgrUpdateFrontPanel( int event_type, int msgCode, unsigned long data );
void VL_CDL_RegisterCdlState(const char * cdlString);
void VL_CDL_RegisterFlashedFile(const char * szFile);
void VL_CDL_RCDL_IndicationFile();
#ifdef __cplusplus
extern "C" {
#endif
int CommonDownloadStartDeferredDownload();
#ifdef __cplusplus
}
#endif

#endif
