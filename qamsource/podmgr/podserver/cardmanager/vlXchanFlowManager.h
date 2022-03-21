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



#ifndef _VL_XCHAN_FLOW_MANAGER_H_
#define _VL_XCHAN_FLOW_MANAGER_H_

#include "ip_types.h"
#include "dsg_types.h"

#define VL_DSG_BASE_HOST_FLOW_ID                         0x800000
#define VL_DSG_BASE_HOST_DSG_FLOW_ID                     (VL_DSG_BASE_HOST_FLOW_ID + 0)
#define VL_DSG_BASE_HOST_IPU_FLOW_ID                     (VL_DSG_BASE_HOST_FLOW_ID + 1)
#define VL_DSG_BASE_HOST_IPDM_FLOW_ID                    (VL_DSG_BASE_HOST_FLOW_ID + 2)
#define VL_DSG_BASE_HOST_IPDIRECT_UNICAST_RECV_FLOW_ID   (VL_DSG_BASE_HOST_FLOW_ID + 3)
#define VL_DSG_BASE_HOST_UDP_SOCKET_FLOW_ID              (VL_DSG_BASE_HOST_FLOW_ID + 4)
#define VL_DSG_BASE_HOST_IPDIRECT_UNICAST_SEND_FLOW_ID   (VL_DSG_BASE_HOST_FLOW_ID + 5)

// All flows-ids incrementally requested by cable-card are allocated starting from this number...
#define VL_DSG_BASE_HOST_OTHER_FLOW_IDS                  (VL_DSG_BASE_HOST_FLOW_ID + 0x10001)    // No matter what this should be a larger number as compared to the rest of the above.

#ifdef __cplusplus
extern "C" {
#endif

void vlXchanInitFlowManager();
void vlXchanClearFlowManager();
void vlXchanRegisterNewFlow(VL_XCHAN_FLOW_TYPE eFlowType, unsigned long nFlowId);
VL_XCHAN_FLOW_TYPE vlXchanGetFlowType(unsigned long nFlowId);
void vlXchanUnregisterFlow(unsigned long nFlowId);
void vlXchanDispatchFlowData(unsigned long lFlowId, unsigned long nSize, unsigned char * pData);

int vlXChanCreateIpMulticastFlow(short sesnum, unsigned char * groupId);
void vlXChanDeallocateIpMulticastFlow(unsigned long nFlowId);

int vlXChanCreateSocketFlow(short sesnum, VL_XCHAN_SOCKET_FLOW_PARAMS * pFlowParams);
void vlXChanDeallocateSocketFlow(unsigned long nFlowId);

int  APDU_XCHAN2POD_newIPUflowCnf(unsigned short sesnum, unsigned char status, unsigned char flowsRemaining, unsigned long flowId,
                                   unsigned char ip_addr[VL_IP_ADDR_SIZE], unsigned short maxPduSize,
                                  VL_DSG_IP_UNICAST_FLOW_PARAMS * pStruct);
int  APDU_XCHAN2POD_lostFlowInd(unsigned short sesnum, unsigned long flowId, unsigned char reason);
int  APDU_XCHAN2POD_send_IP_M_flowCnf(unsigned short sesnum, unsigned char status, unsigned char flowsRemaining, unsigned long flowId);
int  APDU_XCHAN2POD_send_SOCKET_flowCnf(unsigned short sesnum, unsigned char status, unsigned char flowsRemaining, unsigned long flowId,
                                        unsigned char link_ip_addr[VL_IPV6_ADDR_SIZE]);

#ifdef __cplusplus
}
#endif

#endif //_VL_XCHAN_FLOW_MANAGER_H_

