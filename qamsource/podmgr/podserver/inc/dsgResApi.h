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



#ifndef _VL_DSG_RES_API_H_
#define _VL_DSG_RES_API_H_

#include "dsg_types.h"
#include "ip_types.h"
#include "rmf_osal_sync.h"

typedef enum _VL_OCRC_SUBTYPE
{
    VL_OCRC_SUBTYPE_DSG     = 1,
    VL_OCRC_SUBTYPE_OOB     = 2,

    VL_OCRC_SUBTYPE_INVALID = 0xFF
        
}VL_OCRC_SUBTYPE;

#define VL_DSG_IP_ACQUIRED_TOUCH_FILE   "/tmp/ip_acquired"

#ifdef __cplusplus
extern "C" {
#endif

rmf_osal_Mutex vlDsgMainGetThreadLock();
void dsgProtectedProc(void *par);
unsigned short vlGetDsgSessionNumber(unsigned short sesnum);
int  vlGetDsgFlowId();
int  vlGetIpDmFlowId();
unsigned long vlDsgGetUcid();
unsigned long vlDsgGetVctId();
int   vlIsDsgOrIpduMode();
int   vlIsIpduMode();
VL_DSG_OPERATIONAL_MODE  vlGetDsgMode();
VL_OCRC_SUBTYPE vlGetOcrcSubtype();
void  vlSetDsgFlowId(unsigned long nFlowId);
void  vlSetIpDmFlowId(unsigned long nFlowId);
void vlDsgSendIPUdataToECM(unsigned long FlowID, int Size, unsigned char * pData);
void vlSendDsgEventToPFC(int eEventType, void * pData, int nSize);
void vlDsgSendBootupEventToPFC(int eEventType, int nMsgCode, unsigned long ulData);
void vlDsgResourceInit();
void vlDsgResourceRegister();
void vlDsgResourceCleanup();
void vlDsgGetDocsisStatus(VL_DSG_STATUS * pDsgStatus);
int vlDsgIsSpecifiedBroadcastTablesFromCableCard(VL_DSG_BCAST_CLIENT_ID eClientId);
int vlDsgIsScte65TablesFromCableCard();
int vlDsgIsEasTablesFromCableCard();
int vlDsgIsXaitTablesFromCableCard();
int vlClassifyBroadcastTunnels(VL_DSG_DIRECTORY * pDsgDirectory);
void vlDsgUpdateXchanTrafficCounters(VL_XCHAN_FLOW_TYPE eFlowType, int nBytes);
void vlDsgUpdateDsgTrafficCounters(int bCardEntry, int iEntry, int nBytes);
void vlDsgUpdateDsgCardTrafficCounters(VL_DSG_BCAST_CLIENT_ID eClientId, int nBytes);
int vlDsgSendAppTunnelRequestToCableCard(int nAppIds, unsigned short * paAppIds);
void vlDsgGetDsgTextStats(int nMaxStats, VL_DSG_TRAFFIC_TEXT_STATS aTextStats[VL_MAX_DSG_ENTRIES], int * pnAvailableStats);
void vlDsgGetXchanTextStats(VL_DSG_TRAFFIC_TEXT_STATS aTextStats[VL_XCHAN_FLOW_TYPE_MAX]);
void vlDsgGetDsgVctStats(int nMaxStats, VL_VCT_ID_COUNTER aVctStats[VL_MAX_DSG_ENTRIES], int * pnAvailableStats);
void vlDsgTouchFile(const char * pszFile);
int  vlDsgHalSupportsIpRecovery();
int vlDsgCheckIpv4GatewayConnectivity();

#ifdef __cplusplus
}
#endif

#endif //_VL_DSG_RES_API_H_

