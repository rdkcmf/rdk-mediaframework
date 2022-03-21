/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2013 RDK Management
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


#ifndef __CC_PROT_H__
#define __CC_PROT_H__
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif 

typedef int LCSM_DEVICE_HANDLE;
typedef struct vlCableCardCertInfo
{
    char var;
}vlCableCardCertInfo_t;

int vlCpCalReqGetStatus();
int vlGetCaAllocStateForPrgIndex(unsigned char PrgIndex, unsigned char Ltsid,unsigned short PrgNum, unsigned char *pAllocState);
int vlSetCaEncryptedForPrgIndex(unsigned char PrgIndex, unsigned char Ltsid,unsigned short PrgNum);
int vlClearCaResAllocatedForPrgIndex(unsigned char PrgIndex);
int vlCPCheckCertificates(
	unsigned char *ucPodDeviceCertData,
	unsigned long  ulPodDeviceCertDataLen,
	unsigned char *ucPodManCertData,
	unsigned long  ulPodManCertDataLen,
	unsigned char *ucHostRootCertData,
	unsigned long  ulHostRootCertDataLen);
int vlCpGetAuthKeyReqStatus();
int vlCpGetCardDevCertCheckStatus();
int vlCpGetCciAuthDoneCount();
int vlCPGetCertificatesInfo(
	unsigned char *ucPodDeviceCertData,
	unsigned long  ulPodDeviceCertDataLen,
	unsigned char *ucPodManCertData,
	unsigned long  ulPodManCertDataLen,
	vlCableCardCertInfo_t *pCardCertInfo );
int vlCpGetCpKeyGenerationReqCount();
int vlCPGetDevCert(unsigned char **pCert,int *pLen);
int vlCPGetManCert(unsigned char **pCert,int *pLen);
int vlCPRootCert(unsigned char **pCert,int *pLen);
int vlCpWriteNvRamData(int type, unsigned char * pData, int size);
int vlGetCciCAenableFlag( unsigned char Ltsid,unsigned short PrgNum, unsigned char *pCci,unsigned char *pCAEnable);
int vlPostCciCAenableEvent(unsigned char Cci, unsigned char CAflag,unsigned char Ltsid, unsigned short PrgNum);
int vlSendCardValidationStatus(uint16_t sessNum,int waitforRespTime);
int vlSetCaResAllocatedForPrgIndex(unsigned char PrgIndex, unsigned char Ltsid,unsigned short PrgNum);
int vlGetCciCAenableFlagForPrgIndex(unsigned char PrgIndex, unsigned char Ltsid,unsigned short PrgNum, unsigned char *pCci,unsigned char *pCAEnable);
void cprot_init(void);
void cprotProc(void* par);
int cprot_start(LCSM_DEVICE_HANDLE h);
int vlCPCaDescdata(unsigned char *pCaPmt,unsigned long CaPmtLen,void *pStrPointer);
void ca_init();
void caProc(void* par);
int vlCaGetMaxProgramsSupportedByHost();
int caSndApdu(unsigned short sesnum, unsigned long tag, unsigned short dataLen, unsigned char *data);
int ca_start(LCSM_DEVICE_HANDLE h);
int ca_stop();
unsigned short lookupCaSessionValue( );
#ifdef __cplusplus
}
#endif 

#endif

