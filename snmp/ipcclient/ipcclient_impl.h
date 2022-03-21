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



#ifndef __IPCCLIENT_IMPL_H__
#define __IPCCLIENT_IMPL_H__

#include "ip_types.h"
#include "rdk_debug.h"
#include "CommonDownloadSnmpInf.h"
#include "snmp_types.h"
#include "cm_api.h"
#include "hostGenericFeatures.h"
#include "vlSnmpHostInfo.h"
#include "rmf_error.h"
#include "rmf_osal_util.h"
void  IPC_CLIENT_HAL_SYS_Reboot(const char * pszRequestor, const char * pszReason);
int IPC_CLIENT_GetCCIInfoElement(CardManagerCCIData_t* pCCIData,unsigned long LTSID,unsigned long programNumber);
int IPC_CLIENT_CHALDsg_Set_Config(int device_instance, unsigned long ulTag, void* pvConfig);
int IPC_CLIENT_CHalSys_snmp_request(VL_SNMP_REQUEST eRequest, void * _pvStruct);

void IPC_CLIENT_vlXchanGetDsgEthName(VL_HOST_IP_INFO * pHostIpInfo);
void IPC_CLIENT_vlXchanIndicateLostDsgIPUFlow();

void IPC_CLIENT_vlXchanIndicateLostDsgIPUFlow();
rmf_Error IPC_CLIENT_OobGetDacId(uint16_t *pDACId);
rmf_Error IPC_CLIENT_OobGetChannelMapId(uint16_t *pChannelMapId);

int IPC_CLIENT_vlGetCcAppInfoPage(int iIndex, const char ** ppszPage, int * pnBytes);
void  IPC_CLIENT_vlXchanSetEcmRoute();    
unsigned char * IPC_CLIENT_vlXchanGetRcIpAddress();
int IPC_CLIENT_vlXchanGetV6SubnetMaskFromPlen(int nPlen, unsigned char * pBuf, int nBufCapacity);
void IPC_CLIENT_vlXchanGetDsgIpInfo(VL_HOST_IP_INFO * pHostIpInfo);
rmf_Error IPC_CLIENT_vlMpeosDumpBuffer(rdk_LogLevel lvl, const char *mod, const void * pBuffer, int nBufSize);
void IPC_CLIENT_vlXchanGetDsgCableCardIpInfo(VL_HOST_IP_INFO * pHostIpInfo);
void IPC_CLIENT_vlXchanGetHostCardIpInfo(VL_HOST_CARD_IP_INFO * pCardIpInfo);
int IPC_CLIENT_GetHostIdLuhn( unsigned char *pHostId,  char *pHostIdLu );
int IPC_CLIENT_CommonDownloadMgrSnmpHostFirmwareStatus(CDLMgrSnmpHostFirmwareStatus_t *pStatus);
rmf_Error IPC_CLIENT_podImplGetCaSystemId(unsigned short * pCaSystemId);
int IPC_CLIENT_CommonDownloadMgrSnmpCertificateStatus_check(CDLMgrSnmpCertificateStatus_t *pStatus);

int IPC_CLIENT_vlIsDsgMode();
int IPC_CLIENT_vlSnmpEcmGetIfStats(VL_HOST_IP_STATS * pIpStats);
int IPC_CLIENT_vlXchanGetSocketFlowCount();
int IPC_CLIENT_SNMPSendCardMibAccSnmpReq( unsigned char *pMessage, unsigned long MsgLength);
int IPC_CLIENT_SNMPGet_ocStbHostHostID(char* HostID,char len);
int IPC_CLIENT_SNMPGetApplicationInfoWithPages(vlCableCardAppInfo_t *pAppInfo);
int IPC_CLIENT_SNMPGetApplicationInfo(vlCableCardAppInfo_t *pCardAppInfo);
int IPC_CLIENT_SNMPget_ocStbHostSecurityIdentifier(HostCATypeInfo_t* HostCAtypeinfo);
int IPC_CLIENT_SNMPGetCableCardCertInfo(vlCableCardCertInfo_t *pCardCertInfo);
int IPC_CLIENT_SNMPGetApplicationInfo(vlCableCardAppInfo_t *pCardAppInfo);
int IPC_CLIENT_SNMPGetCardIdBndStsOobMode(vlCableCardIdBndStsOobMode_t *pCardInfo);
int IPC_CLIENT_SNMPGetGenFetResourceId(unsigned long *pRrsId);
int IPC_CLIENT_SNMPGetGenFetTimeZoneOffset(int *pTZOffset);
int IPC_CLIENT_SNMPGetGenFetDayLightSavingsTimeDelta(char *pDelta);
int IPC_CLIENT_SNMPGetGenFetDayLightSavingsTimeEntry(unsigned long *pTimeEntry);
int IPC_CLIENT_SNMPGetGenFetDayLightSavingsTimeExit(unsigned long *pTimeExit);
VL_HOST_GENERIC_FEATURES_RESULT IPC_CLIENT_vlPodGetGenericFeatureData(VL_HOST_FEATURE_PARAM eFeature, void * _pvStruct);
int IPC_CLIENT_SNMPGetGenFetEasLocationCode(unsigned char *pState,unsigned short *pDDDDxxCCCCCCCCCC);
int IPC_CLIENT_SNMPGetGenFetVctId(unsigned short *pVctId);
int IPC_CLIENT_SNMPGetCpAuthKeyStatus(int *pKeyStatus);
int IPC_CLIENT_SNMPGetCpCertCheck(int *pCerttatus);
int IPC_CLIENT_SNMPGetCpCciChallengeCount(unsigned long *pCount);
int IPC_CLIENT_SNMPGetCpKeyGenerationReqCount(unsigned long *pCount);
int IPC_CLIENT_SNMPGetCpIdList(unsigned long *pCP_system_id_bitmask);
int IPC_CLIENT_SNMPSetTime( unsigned short nYear, 
				unsigned short nMonth, 
				unsigned short nDay, 
				unsigned short nHour, 
				unsigned short nMin, 
				unsigned short nSec  );
rmf_Error IPC_CLIENT_vlMCardGetCardVersionNumberFromMMI(char * pszVersionNumber, int nCapacity);
int IPC_CLIENT_SNMPGetPatTimeoutCount(unsigned int * pValue);
int IPC_CLIENT_SNMPGetPmtTimeoutCount(unsigned int * pValue);
int IPC_CLIENT_SNMPGetVideoContentInfo(int iContent, struct VideoContentInfo * pInfo);
/* Start listening for the pod events*/
rmf_Error rmf_snmpmgrIPCInit(void);
/* Stop listening for the pod events*/
void rmf_snmpmgrIPCUnInit(void);
/* Register events*/
rmf_Error rmf_snmpmgrRegisterEvents(rmf_osal_eventqueue_handle_t queueId);
/* Unregister events*/
rmf_Error rmf_snmpmgrUnRegisterEvents(rmf_osal_eventqueue_handle_t queueId);
/*get the podhandle 0=unable to register with runPOD process*/
uint32_t getPodClientHandle(uint32_t *phandle);


#endif // __IPCCLIENT_H__
