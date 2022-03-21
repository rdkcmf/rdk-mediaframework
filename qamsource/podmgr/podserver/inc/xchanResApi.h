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



#ifndef _VL_XCHAN_RES_API_H_
#define _VL_XCHAN_RES_API_H_

#include "dsg_types.h"
#include "ip_types.h"
#include "bufParseUtilities.h"
#include "rmf_osal_sync.h"


#ifdef __cplusplus
extern "C" {
#endif

void xchanProtectedProc( void* );
void vlSetXChanSession(unsigned short sesnum);
void vlXchanGetFlowCounts(int aCounts[VL_XCHAN_FLOW_TYPE_MAX]);
unsigned long vlGetIpuFlowId();
unsigned long vlXchanHostAssignFlowId();
int vlXchanHandleDsgFlowCnf(unsigned long ulTag, unsigned long ulQualifier, VL_DSG_IP_UNICAST_FLOW_PARAMS * pStruct);
void vlXchanSendIPMdataToHost(unsigned long FlowID, int Size, unsigned char * pData);
void  vlSetXChanMPEGflowId(unsigned long nFlowId);
void AM_EXTENDED_DATA(unsigned long FlowID, unsigned char * pData, unsigned short Size);
int POD_STACK_SEND_ExtCh_Data(unsigned char * data,  unsigned long length, unsigned long flowID);
unsigned char * vlXchanGetRcMacAddress();
unsigned char * vlXchanGetRcIpAddress();
void vlXchanSetResourceId(unsigned long ulResId);
void vlXchanGetHostCardIpInfo(VL_HOST_CARD_IP_INFO * pCardIpInfo);
// vlXchanGetCableCardDsgModeIPUinfo returns only IP address and MAC address
void vlXchanGetCableCardDsgModeIPUinfo(VL_HOST_IP_INFO * pCardIpInfo);
void vlXchanGetDsgEthName(VL_HOST_IP_INFO * pHostIpInfo);
void vlXchanGetHostIpTypeInfo(VL_HOST_IP_IF_TYPE eType, int iInstance, VL_HOST_IP_INFO * pHostIpInfo);
int vlXchanCheckIfHostAcquiredIpAddress();
void vlXchanGetDsgIpInfo(VL_HOST_IP_INFO * pHostIpInfo);
void vlXchanGetPodIpInfo(VL_HOST_IP_INFO * pHostIpInfo);
int vlXchanGetV6SubnetMaskFromPlen(int nPlen, unsigned char * pBuf, int nBufCapacity);
// vlXchanGetDsgCableCardIpInfo returns complete info
void vlXchanGetDsgCableCardIpInfo(VL_HOST_IP_INFO * pHostIpInfo);
void vlXchanUpdateInComingDsgCardIPUCounters(int nSize);
int vlXchanIsDsgIpuFlow(unsigned long nFlowId);
void vlXchanCheckIpuCleanup(unsigned long nFlowId);
void vlXchanIndicateLostDsgIPUFlow();
void vlXchanCancelQpskOobIpTask();
void vlParseIPUconfParams(VL_BYTE_STREAM * pParseBuf, int len, VL_XCHAN_IPU_FLOW_CONF * pStruct);
int vl_xchan_session_is_active();
void vl_xchan_async_start_outband_processing();
void vl_xchan_async_stop_outband_processing();
void vl_xchan_async_start_ip_over_qpsk();
void vl_xchan_async_stop_ip_over_qpsk();
#ifdef USE_IPDIRECT_UNICAST
void vl_xchan_async_start_ipdu_processing();
void vl_xchan_async_stop_ipdu_processing();
#endif // USE_IPDIRECT_UNICAST
rmf_osal_Mutex vlXChanGetOobThreadLock();
unsigned long vlXchanGetResourceId();
unsigned long vlGetXChanSession();
unsigned long vlMapToXChanTag(unsigned short sesnum, unsigned long ulTag);
void vlXchanGetPodIpStatus(VL_POD_IP_STATUS * pPodIpStatus);
char * cExtChannelGetFlowResultName(VL_XCHAN_FLOW_RESULT status);
int vlXchanGetSocketFlowCount();
int vlXchanSetNetworkRoute(unsigned long ipNet, unsigned long ipMask, unsigned long ipGateway, char * pszIfName);
int vlXchanSetEcmRoute();
int vlXchanDelNetworkRoute(unsigned long ipNet, unsigned long ipMask, unsigned long ipGateway, char * pszIfName);
int vlXchanDelDefaultEcmGatewayRoute();
long vlXchanGetSnmpSysUpTime();

#ifdef __cplusplus
}
#endif

#endif //_VL_XCHAN_RES_API_H_

