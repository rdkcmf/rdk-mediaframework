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
 
#include <stdlib.h>
//#include <cprot.h>
#include <cacp.h>

#ifdef __cplusplus
extern "C" {
#endif 

#define UNUSED_VAR(v) ((void)v)
unsigned char vlg_CardValidationStatus;
char g_nvmAuthStatus;
unsigned short vlg_CaSystemId;
#define SIZE_HOST_ID            4
#define SIZE_POD_ID             4

uint8_t Host_ID        [SIZE_HOST_ID];
uint8_t POD_ID        [SIZE_POD_ID];

int vlCpCalReqGetStatus()
{
        return 0;
}

int vlGetCaAllocStateForPrgIndex(unsigned char PrgIndex, unsigned char Ltsid,unsigned short PrgNum, unsigned char *pAllocState)
{
	UNUSED_VAR(PrgIndex);
	UNUSED_VAR(Ltsid);
	UNUSED_VAR(PrgNum);
	UNUSED_VAR(pAllocState);
	return -1;
}

int vlSetCaEncryptedForPrgIndex(unsigned char PrgIndex, unsigned char Ltsid,unsigned short PrgNum)
{
	UNUSED_VAR(PrgIndex);
	UNUSED_VAR(Ltsid);
	UNUSED_VAR(PrgNum);
	return -1;
}

int vlClearCaResAllocatedForPrgIndex(unsigned char PrgIndex)
{
	UNUSED_VAR(PrgIndex);
	return -1;
}

int vlCPCheckCertificates(
	unsigned char *ucPodDeviceCertData,
	unsigned long  ulPodDeviceCertDataLen,

	unsigned char *ucPodManCertData,
	unsigned long  ulPodManCertDataLen,

	unsigned char *ucHostRootCertData,
	unsigned long  ulHostRootCertDataLen)
{
	UNUSED_VAR(ucPodDeviceCertData);
	UNUSED_VAR(ulPodDeviceCertDataLen);
	UNUSED_VAR(ucPodManCertData);
	UNUSED_VAR(ulPodManCertDataLen);
	UNUSED_VAR(ucHostRootCertData);
	UNUSED_VAR(ulHostRootCertDataLen);
	return -1;
}
int vlCpGetAuthKeyReqStatus()
{
    return 0;
}

int vlCpGetCardDevCertCheckStatus()
{
	return 0;
}

int vlCpGetCciAuthDoneCount()
{
    return 0;
}
int vlCPGetCertificatesInfo(
	unsigned char *ucPodDeviceCertData,
	unsigned long  ulPodDeviceCertDataLen,
	unsigned char *ucPodManCertData,
	unsigned long  ulPodManCertDataLen,
	vlCableCardCertInfo_t *pCardCertInfo )
{
	UNUSED_VAR(ucPodDeviceCertData);
	UNUSED_VAR(ulPodDeviceCertDataLen);
	UNUSED_VAR(ucPodManCertData);
	UNUSED_VAR(ulPodManCertDataLen);
	UNUSED_VAR(pCardCertInfo);
	return -1;
}

int vlCpGetCpKeyGenerationReqCount()
{
    return 0;
}

int vlCPGetDevCert(unsigned char **pCert,int *pLen)
{
	UNUSED_VAR(pCert);
	UNUSED_VAR(pLen);		
	return -1;
}

int vlCPGetManCert(unsigned char **pCert,int *pLen)
{
	UNUSED_VAR(pCert);
	UNUSED_VAR(pLen);		
	return -1;	
}

int vlCPRootCert(unsigned char **pCert,int *pLen)
{
	UNUSED_VAR(pCert);
	UNUSED_VAR(pLen);		
	return -1;
}

int vlCpWriteNvRamData(int type, unsigned char * pData, int size)
{
	UNUSED_VAR(type);
	UNUSED_VAR(pData);
	UNUSED_VAR(size);
	return -1;
}

int vlGetCciCAenableFlag( unsigned char Ltsid,unsigned short PrgNum, unsigned char *pCci,unsigned char *pCAEnable)
{
	UNUSED_VAR(Ltsid);
	UNUSED_VAR(PrgNum);
	UNUSED_VAR(pCci);
	UNUSED_VAR(pCAEnable);
	return -1;
}

int vlGetCciCAenableFlagForPrgIndex(unsigned char PrgIndex, unsigned char Ltsid,unsigned short PrgNum, unsigned char *pCci,unsigned char *pCAEnable)
{
	UNUSED_VAR(PrgIndex);
	UNUSED_VAR(Ltsid);
	UNUSED_VAR(PrgNum);
	UNUSED_VAR(pCci);
	UNUSED_VAR(pCAEnable);
	return -1;
}

int vlPostCciCAenableEvent(unsigned char Cci, unsigned char CAflag,unsigned char Ltsid, unsigned short PrgNum)
{
	UNUSED_VAR(Cci);
	UNUSED_VAR(CAflag);
	UNUSED_VAR(Ltsid);
	UNUSED_VAR(PrgNum);
	return -1;
}

int vlSendCardValidationStatus(uint16_t sessNum,int waitforRespTime)
{
	UNUSED_VAR(sessNum);
	UNUSED_VAR(waitforRespTime);
	return -1;
}

int vlSetCaResAllocatedForPrgIndex(unsigned char PrgIndex, unsigned char Ltsid,unsigned short PrgNum)
{
	UNUSED_VAR(PrgIndex);
	UNUSED_VAR(Ltsid);
	UNUSED_VAR(PrgNum);
	return -1;
}

void cprot_init(void) 
{
}

void cprotProc(void* par)
{
	UNUSED_VAR(par);
}

int cprot_start(LCSM_DEVICE_HANDLE h)
{
	UNUSED_VAR(h);
	return EXIT_FAILURE;
}

int vlCPCaDescdata(unsigned char *pCaPmt,unsigned long CaPmtLen,void *pStrPointer)
{
	UNUSED_VAR(pCaPmt);
	UNUSED_VAR(CaPmtLen);
	UNUSED_VAR(pStrPointer);
	return -1;
}
void ca_init();
void caProc(void* par);
int vlCaGetMaxProgramsSupportedByHost();
int caSndApdu(unsigned short sesnum, unsigned long tag, unsigned short dataLen, unsigned char *data);
int ca_start(int h);
int ca_stop();
unsigned short lookupCaSessionValue( );

void ca_init(void)
{
	return;
}

void caProc(void* par)
{
	UNUSED_VAR(par);
	return;
}

int vlCaGetMaxProgramsSupportedByHost()
{
    return 0;;
}

int caSndApdu(unsigned short sesnum, unsigned long tag, unsigned short dataLen, unsigned char *data)
{
	UNUSED_VAR(sesnum);
	UNUSED_VAR(tag);
	UNUSED_VAR(dataLen);
	UNUSED_VAR(data);
	return -1;
}

int ca_start(LCSM_DEVICE_HANDLE h)
{
	UNUSED_VAR(h);
	return EXIT_FAILURE;
}

int ca_stop()
{
	return -1;
}

unsigned short lookupCaSessionValue(void)
{
	return 0;
}

#ifdef __cplusplus
}
#endif 


