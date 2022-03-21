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



#ifndef _VL_IPDIRECT_UNICAST_RES_API_H_
#define _VL_IPDIRECT_UNICAST_RES_API_H_

#include "dsg_types.h"
#include "ipdirect_unicast_types.h"
#include "ip_types.h"
#include "rmf_osal_sync.h"

#ifdef __cplusplus
extern "C" {
#endif

rmf_osal_Mutex vlIpDirectUnicastMainGetThreadLock();
void ipdirect_unicastProtectedProc(void *par);
unsigned short vlGetIpDirectUnicastSessionNumber(unsigned short sesnum);
int  vlGetIpDirectUnicastFlowId();
unsigned long vlIpDirectUnicastGetUcid();
unsigned long vlIpDirectUnicastGetVctId();
void  vlSetIpDirectUnicastFlowId(unsigned long nFlowId);
void vlSendIpDirectUnicastEventToPFC(int eEventType, void * pData, int nSize);
void vlIpDirectUnicastSendBootupEventToPFC(int eEventType, int nMsgCode, unsigned long ulData);
void vlIpDirectUnicastResourceInit();
void vlIpDirectUnicastResourceRegister();
void vlIpDirectUnicastResourceCleanup();
void vlIpDirectUnicastUpdateXchanTrafficCounters(VL_XCHAN_FLOW_TYPE eFlowType, int nBytes);
void vlIpDirectUnicastTouchFile(const char * pszFile);

#ifdef __cplusplus
}
#endif

#endif //_VL_IPDIRECT_UNICAST_RES_API_H_

