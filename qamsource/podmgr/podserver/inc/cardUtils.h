/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2018 RDK Management
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


#ifndef __PFC_DEPEND__ 
#define __PFC_DEPEND__


#include "rmf_osal_event.h"
#include "rmf_osal_mem.h"
#ifndef SNMP_IPC_CLIENT
#include "SnmpIORM.h"
#endif //SNMP_IPC_CLIENT
#ifdef __cplusplus
extern "C" {
#endif
#ifndef SNMP_IPC_CLIENT
void vlGet_ocStbHostCard_Details(SocStbHostSnmpProxy_CardInfo_t *vl_ProxyCardInfo);
#endif //SNMP_IPC_CLIENT
void PostBootEvent(int eEventType, int nMsgCode, unsigned long ulData);
int fpSetTextSimple(const char* pszText);
int fpEnableUpdates(int bEnable);
#ifndef SNMP_IPC_CLIENT
void vlGet_HostCaTypeInfo(HostCATInfo_t* CAtypeinfo);
#endif //SNMP_IPC_CLIENT
#if 1
rmf_Error create_pod_event_manager();
rmf_osal_eventmanager_handle_t get_pod_event_manager();
void podmgr_freefn(void *data);
int checkTime(unsigned char one[12], unsigned char two[12]);
int verifySignedContent(void *code, int len, char bFile);
void vlhal_stt_SetTimeZone(int featureId);
void postDSGIPEvent(uint8_t *ip);
int rmf_RebootHostTillLimit(const char* pszRequestor, const char* pszReason, const char* pszCounterName, int nLimit);

#endif




#ifdef __cplusplus
}
#endif



#endif





