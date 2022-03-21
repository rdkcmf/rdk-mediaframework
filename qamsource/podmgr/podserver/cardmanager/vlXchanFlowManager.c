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



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>


#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_arp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "vlXchanFlowManager.h"
#include "utilityMacros.h"
#include "ip_types.h"
#include "dsg_types.h"

#include "dsgResApi.h"
#include "xchanResApi.h"
#include "vlpluginapp_haldsgapi.h"
#include "vlpluginapp_halpodapi.h"
#include "cVector.h"
#include "netUtils.h"
#include "vlDsgDispatchToClients.h"
#include "sys_init.h"
#include "coreUtilityApi.h"
#include "vlEnv.h"
#include "rmf_osal_util.h"
#include "rdk_debug.h"
#include "rmf_osal_thread.h"
#include "ipdirectUnicast-main.h"


#define VL_XCHAN_LOCAL_ECM_IP_ADDRESS   0xC0A86401

//#define VL_DSG_LOCAL_FLOW_TEST
#define VL_DSG_MULTICAST_CONSTRAINED_TO_PHYSICAL_IF

#ifdef __cplusplus
extern "C" {
#endif
    
static VL_HOST_IP_STATS vlg_DsgCableCardIPUStats;
    
unsigned long vlg_IPU_svc_FlowId = 0;
unsigned long vlg_tcHostIpLastChange = 0;
unsigned long vlg_tcCardDsgIPULastChange = 0;
extern VL_XCHAN_FLOW_RESULT   vlg_eXchanFlowResult;
extern int vlg_bHostAcquiredIPAddress;
extern int vlg_bHostAcquiredQpskIPAddress;
extern int vlg_bHostAcquiredProxyAddress;

cVector * vlg_pMPEGflows = NULL;
cVector * vlg_pIPUflows = NULL;
cVector * vlg_pIPMflows = NULL;
cVector * vlg_pDSGflows = NULL;
cVector * vlg_pSOCKETflows = NULL;
cVector * vlg_pIpDmFlows = NULL;										// IP Direct Multicast
cVector * vlg_pIpDuFlows = NULL;										// IP Direct Unicast
cVector * vlg_pYetToBeCategorizedflows = NULL;

static unsigned int               vlg_bXchanFlowManagerInitialized = 0;

int                               vlg_nXchanIpMulticastFlowsAvailable = VL_MAX_IP_MULTICAST_FLOWS;
VL_XCHAN_IP_MULTICAST_FLOW_PARAMS vlg_aDsgIpMulticastFlows[VL_MAX_IP_MULTICAST_FLOWS];

int                               vlg_nXchanSocketFlowsAvailable = VL_MAX_SOCKET_FLOWS;
VL_XCHAN_SOCKET_FLOW_PARAMS       vlg_aDsgSocketFlows[VL_MAX_SOCKET_FLOWS];

#define VL_RETURN_CASE_STRING(x)   case x: return #x;

char * vlGetAddressFamilyName(int family)
{
    switch(family)
    {
        VL_RETURN_CASE_STRING(AF_INET);
        VL_RETURN_CASE_STRING(AF_INET6);
    }
    
    return "";
}

char * vlGetSockTypeName(int type)
{
    switch(type)
    {
        VL_RETURN_CASE_STRING(SOCK_STREAM);
        VL_RETURN_CASE_STRING(SOCK_DGRAM);
    }
    
    return "";
}

char * vlGetIpProtocolName(int protocol)
{
    switch(protocol)
    {
        VL_RETURN_CASE_STRING(IPPROTO_TCP);
        VL_RETURN_CASE_STRING(IPPROTO_UDP);
    }
    
    return "";
}

#define IPV6_ADDR_ANY           0x0000U
#define IPV6_ADDR_LOOPBACK      0x0010U
#define IPV6_ADDR_LINKLOCAL     0x0020U
#define IPV6_ADDR_SITELOCAL     0x0040U
#define IPV6_ADDR_COMPATv4      0x0080U
                
char * vlGetIpScopeName(int type)
{
    switch(type)
    {
        case IPV6_ADDR_ANY      : return "global";
        case IPV6_ADDR_LOOPBACK : return "loop";
        case IPV6_ADDR_LINKLOCAL: return "link";
        case IPV6_ADDR_SITELOCAL: return "site";
        case IPV6_ADDR_COMPATv4 : return "v4 compatible";
    }
    
    return "";
}

unsigned long vlGetIpuFlowId()
{
    return vlg_IPU_svc_FlowId;
}

void vlXchanGetFlowCounts(int aCounts[VL_XCHAN_FLOW_TYPE_MAX])
{
    memset(aCounts, 0, sizeof(int)*VL_XCHAN_FLOW_TYPE_MAX);
    if(NULL != vlg_pMPEGflows  ) aCounts[VL_XCHAN_FLOW_TYPE_MPEG  ] = cVectorSize(vlg_pMPEGflows  );
    if(NULL != vlg_pIPUflows   ) aCounts[VL_XCHAN_FLOW_TYPE_IP_U  ] = cVectorSize(vlg_pIPUflows   );
    if(NULL != vlg_pIPMflows   ) aCounts[VL_XCHAN_FLOW_TYPE_IP_M  ] = cVectorSize(vlg_pIPMflows   );
    if(NULL != vlg_pDSGflows   ) aCounts[VL_XCHAN_FLOW_TYPE_DSG   ] = cVectorSize(vlg_pDSGflows   );
    if(NULL != vlg_pSOCKETflows) aCounts[VL_XCHAN_FLOW_TYPE_SOCKET] = cVectorSize(vlg_pSOCKETflows);
    if(NULL != vlg_pIpDmFlows   ) aCounts[VL_XCHAN_FLOW_TYPE_IPDM   ] = cVectorSize(vlg_pIpDmFlows   );
    if(NULL != vlg_pIpDuFlows   ) aCounts[VL_XCHAN_FLOW_TYPE_IPDIRECT_UNICAST   ] = cVectorSize(vlg_pIpDuFlows   );
}

void vlXchanInitFlowManager()
{
    if(NULL == vlg_pMPEGflows  ) vlg_pMPEGflows        = cVectorCreate(10, 10);
    if(NULL == vlg_pIPUflows   ) vlg_pIPUflows         = cVectorCreate(10, 10);
    if(NULL == vlg_pIPMflows   ) vlg_pIPMflows         = cVectorCreate(10, 10);
    if(NULL == vlg_pDSGflows   ) vlg_pDSGflows         = cVectorCreate(10, 10);
    if(NULL == vlg_pSOCKETflows) vlg_pSOCKETflows      = cVectorCreate(10, 10);
    if(NULL == vlg_pIpDmFlows   ) vlg_pIpDmFlows         = cVectorCreate(10, 10);
    if(NULL == vlg_pIpDuFlows   ) vlg_pIpDuFlows         = cVectorCreate(10, 10);

    if(NULL == vlg_pYetToBeCategorizedflows) vlg_pYetToBeCategorizedflows
                                                       = cVectorCreate(10, 10);
    
    if(FALSE == vlg_bXchanFlowManagerInitialized)
    {
        vlg_bXchanFlowManagerInitialized = 1;
        VL_ZERO_STRUCT(vlg_aDsgIpMulticastFlows);
        VL_ZERO_STRUCT(vlg_aDsgSocketFlows);
    }
}

#define VL_XCHAN_CLEAR_FLOWS(vector)                            \
{                                                               \
    int i = 0;                                                  \
    for(i = 0; i < cVectorSize(vector); i++)                    \
    {                                                           \
        vlXchanUnregisterFlow(cVectorGetElementAt(vector, i));  \
    }                                                           \
    cVectorClear(vector);                                       \
}
    
void vlXchanClearFlowManager()
{
    VL_XCHAN_CLEAR_FLOWS(vlg_pMPEGflows);
    VL_XCHAN_CLEAR_FLOWS(vlg_pIPUflows);
    VL_XCHAN_CLEAR_FLOWS(vlg_pIPMflows);
    VL_XCHAN_CLEAR_FLOWS(vlg_pDSGflows);
    VL_XCHAN_CLEAR_FLOWS(vlg_pSOCKETflows);
    VL_XCHAN_CLEAR_FLOWS(vlg_pIpDmFlows);
    VL_XCHAN_CLEAR_FLOWS(vlg_pIpDuFlows);
    VL_XCHAN_CLEAR_FLOWS(vlg_pYetToBeCategorizedflows);
}

void vlXchanRegisterNewFlow(VL_XCHAN_FLOW_TYPE eFlowType, unsigned long nFlowId)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s: eFlowType= %u,  nFlowId= %u.\n", __FUNCTION__, eFlowType, nFlowId);
    if(0 == nFlowId) return; // do not register invalid flow-ids

    if(VL_XCHAN_FLOW_TYPE_INVALID != vlXchanGetFlowType(nFlowId))
    {
        // avoid duplicate registrations
        return;
    }
    
    switch(eFlowType)
    {
        case VL_XCHAN_FLOW_TYPE_MPEG:
        {
#ifdef USE_DSG
            vlSetXChanMPEGflowId(nFlowId);
#endif
            cVectorAdd(vlg_pMPEGflows, (cVectorElement)nFlowId);
        }
        break;
        
        case VL_XCHAN_FLOW_TYPE_IP_U:
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlXchanRegisterNewFlow: Registered Ext Chnl IP_U flow: id=0x%06X\n", nFlowId);
            vlg_IPU_svc_FlowId = nFlowId;
            cVectorAdd(vlg_pIPUflows, (cVectorElement)nFlowId);
            
            if(VL_DSG_BASE_HOST_FLOW_ID == (nFlowId&VL_DSG_BASE_HOST_FLOW_ID))
            {
                vlg_tcCardDsgIPULastChange = vlXchanGetSnmpSysUpTime();
		memset(&vlg_DsgCableCardIPUStats,0,sizeof(vlg_DsgCableCardIPUStats));
            }
        }
        break;
        
        case VL_XCHAN_FLOW_TYPE_IP_M:
        {
            cVectorAdd(vlg_pIPMflows, (cVectorElement)nFlowId);
        }
        break;
        
        case VL_XCHAN_FLOW_TYPE_DSG:
        {
            vlSetDsgFlowId(nFlowId);
            // currently only one DSG flow is possible so clear the vector
            cVectorClear(vlg_pDSGflows);
            cVectorAdd(vlg_pDSGflows, (cVectorElement)nFlowId);
        }
        break;
        
        case VL_XCHAN_FLOW_TYPE_SOCKET:
        {
            cVectorAdd(vlg_pSOCKETflows, (cVectorElement)nFlowId);
        }
        break;

        case VL_XCHAN_FLOW_TYPE_IPDM:
        {
            vlSetIpDmFlowId(nFlowId);
            // currently only one IPDM flow is possible so clear the vector
            cVectorClear(vlg_pIpDmFlows);
            cVectorAdd(vlg_pIpDmFlows, (cVectorElement)nFlowId);
        }
        break;

        case VL_XCHAN_FLOW_TYPE_IPDIRECT_UNICAST:
        {
						RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s: case VL_XCHAN_FLOW_TYPE_IPDIRECT_UNICAST.\n", __FUNCTION__);
            vlSetIpDirectUnicastFlowId(nFlowId);
            // currently only one IPDU flow is possible so clear the vector
            cVectorClear(vlg_pIpDuFlows);
            cVectorAdd(vlg_pIpDuFlows, (cVectorElement)nFlowId);
        }
        break;

        default:
        {
            cVectorAdd(vlg_pYetToBeCategorizedflows, (cVectorElement)nFlowId);
        }
        break;
    }
}

VL_XCHAN_FLOW_TYPE vlXchanGetFlowType(unsigned long nFlowId)
{
    // as this is most frequent check MPEG flows first
    if((cVectorSize(vlg_pMPEGflows)>0) && (cVectorSearch(vlg_pMPEGflows, (cVectorElement)nFlowId)!=-1)) return VL_XCHAN_FLOW_TYPE_MPEG;
    // as this is next most frequent check DSG flows next
    if((cVectorSize(vlg_pDSGflows)>0) && (cVectorSearch(vlg_pDSGflows, (cVectorElement)nFlowId)!=-1)) return VL_XCHAN_FLOW_TYPE_DSG;
    // as this is next most frequent check IP_M flows next
    if((cVectorSize(vlg_pIPMflows)>0) && (cVectorSearch(vlg_pIPMflows, (cVectorElement)nFlowId)!=-1)) return VL_XCHAN_FLOW_TYPE_IP_M;
    if((cVectorSize(vlg_pIPUflows)>0) && (cVectorSearch(vlg_pIPUflows, (cVectorElement)nFlowId)!=-1)) return VL_XCHAN_FLOW_TYPE_IP_U;
    if((cVectorSize(vlg_pSOCKETflows)>0) && (cVectorSearch(vlg_pSOCKETflows, (cVectorElement)nFlowId)!=-1)) return VL_XCHAN_FLOW_TYPE_SOCKET;
    if((cVectorSize(vlg_pIpDmFlows)>0) && (cVectorSearch(vlg_pIpDmFlows, (cVectorElement)nFlowId)!=-1)) return VL_XCHAN_FLOW_TYPE_IPDM;
    if((cVectorSize(vlg_pIpDuFlows)>0) && (cVectorSearch(vlg_pIpDuFlows, (cVectorElement)nFlowId)!=-1)) return VL_XCHAN_FLOW_TYPE_IPDIRECT_UNICAST;
    if((cVectorSize(vlg_pYetToBeCategorizedflows)>0) && (cVectorSearch(vlg_pYetToBeCategorizedflows, (cVectorElement)nFlowId)!=-1)) return VL_XCHAN_FLOW_TYPE_NC;
    return VL_XCHAN_FLOW_TYPE_INVALID;
}

void vlXChanDeallocateIpMulticastFlow(unsigned long nFlowId);

int vlXchanIsDsgIpuFlow(unsigned long nFlowId)
{
    if((nFlowId == vlg_IPU_svc_FlowId) || (vlXchanGetFlowType(nFlowId) == VL_XCHAN_FLOW_TYPE_IP_U))
    {
        if(VL_DSG_BASE_HOST_FLOW_ID == (nFlowId&VL_DSG_BASE_HOST_FLOW_ID))
        {
            return 1;
        }
    }
    
    return 0;
}

void vlXchanCheckIpuCleanup(unsigned long nFlowId)
{
    if(vlXchanIsDsgIpuFlow(nFlowId))
    {
#ifdef USE_DSG
        //Oct-02-2009: Commented call to maintain init-reboot state for ipu dhcp // CHALDsg_Set_Config(VL_CM_DSG_DEVICE_INSTANCE, VL_DSG_PVT_TAG_DSG_CLEAN_UP, NULL);
#endif
    }
}

void vlXchanUnregisterFlow(unsigned long nFlowId)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    VL_XCHAN_FLOW_TYPE eFlowType = vlXchanGetFlowType(nFlowId);

    switch(eFlowType)
    {
        case VL_XCHAN_FLOW_TYPE_IP_M:
        {
            vlXChanDeallocateIpMulticastFlow(nFlowId);
        }
        break;
        
        case VL_XCHAN_FLOW_TYPE_IP_U:
        {
            if(VL_DSG_BASE_HOST_FLOW_ID == (nFlowId&VL_DSG_BASE_HOST_FLOW_ID))
            {
                //Oct-02-2009: Commented call to maintain init-reboot state for ipu dhcp // vlXchanCheckIpuCleanup(nFlowId);

                vlg_tcCardDsgIPULastChange = vlXchanGetSnmpSysUpTime();
	        memset(&(vlg_DsgCableCardIPUStats),0,sizeof(vlg_DsgCableCardIPUStats));
            }
            else
            {
                vlXchanCancelQpskOobIpTask();
#ifdef USE_DSG
                if(!vlIsDsgOrIpduMode())
                {
                    vl_xchan_async_start_ip_over_qpsk();
                }
#endif
            }

            if(nFlowId == vlg_IPU_svc_FlowId)
            {
                vlg_IPU_svc_FlowId = 0;
            }
        }
        break;
        
        case VL_XCHAN_FLOW_TYPE_SOCKET:
        {
            vlXChanDeallocateSocketFlow(nFlowId);
        }
        break;

        default:
        {
        }
        break;
    }
#ifdef USE_DSG 
    if((unsigned long)vlGetDsgFlowId() == nFlowId)
    {
        vlSetDsgFlowId(0);
    }
    if((unsigned long)vlGetIpDmFlowId() == nFlowId)
    {
        vlSetIpDmFlowId(0);
    }
#endif
    
    cVectorRemoveElement(vlg_pMPEGflows, (cVectorElement)nFlowId);
    cVectorRemoveElement(vlg_pIPUflows, (cVectorElement)nFlowId);
    cVectorRemoveElement(vlg_pIPMflows, (cVectorElement)nFlowId);
    cVectorRemoveElement(vlg_pDSGflows, (cVectorElement)nFlowId);
    cVectorRemoveElement(vlg_pSOCKETflows, (cVectorElement)nFlowId);
    cVectorRemoveElement(vlg_pIpDmFlows, (cVectorElement)nFlowId);
    cVectorRemoveElement(vlg_pIpDuFlows, (cVectorElement)nFlowId);
    cVectorRemoveElement(vlg_pYetToBeCategorizedflows, (cVectorElement)nFlowId);
}

int vlXchanGetSocketFlowCount()
{
#ifdef USE_DSG
    if(vlIsDsgOrIpduMode())
    {
        return cVectorSize(vlg_pSOCKETflows);
    }
#endif // USE_DSG
    return 0;
}

void vlXchanSendSOCKETdataToHost(unsigned long nFlowId, int Size, unsigned char * pData)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    int i = 0;
    VL_XCHAN_SOCKET_FLOW_PARAMS * pFlow = NULL;

    for(i = 0; i < VL_MAX_SOCKET_FLOWS; i++)
    {
        if(nFlowId == vlg_aDsgSocketFlows[i].flowid)
        {
            pFlow = &(vlg_aDsgSocketFlows[i]);
            break;
        }
    }

    if( (NULL != pFlow) && ( pData != NULL ))
    {
        int ret = 0;
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlXchanSendSOCKETdataToHost: received SOCKET [%s] data from card: flowid = 0x%06lX ...sending to socket.\n", VL_COND_STR(pFlow->eProtocol, TCP, UDP), nFlowId);
        // Feb-06-2009: reverted back to send method for TC 170.2
        if(send(pFlow->socket, pData, Size, MSG_NOSIGNAL) < 0)
        {
            perror("vlXchanSendSOCKETdataToHost: send");
            APDU_XCHAN2POD_lostFlowInd(pFlow->sesnum, pFlow->flowid, VL_XCHAN_LOST_FLOW_REASON_SOCKET_WRITE_ERROR);
        }
    }
}

#include <global_event_map.h>
#include <global_queues.h>
#include "rmf_osal_event.h"
#include "rmf_osal_thread.h"
#include "rmf_osal_mem.h"

extern rmf_osal_eventqueue_handle_t GetIpDirectUnicastMsgQueue(void);
static unsigned char RcvFlowDataBuf[2048];
rmf_osal_event_handle_t ipdu_event_handle;
rmf_osal_event_params_t ipdu_event_params;
rmf_osal_eventqueue_handle_t ipdu_event_queue;
void vlXchanProcessIpduData(unsigned long lFlowId, unsigned long nSize, unsigned char * pData)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s: lFlowId= %u, size= %u \n", __FUNCTION__, lFlowId, nSize);
		vlMpeosDumpBuffer(RDK_LOG_INFO, "LOG.RDK.POD", pData, nSize);

	  ipdu_event_params = { 0, (void *)RcvFlowDataBuf, nSize, NULL };
	  ipdu_event_queue = GetIpDirectUnicastMsgQueue();

    memset(RcvFlowDataBuf, 0, sizeof(RcvFlowDataBuf));
    memcpy(RcvFlowDataBuf, pData, nSize);

    if(0 != ipdu_event_queue)
    {
				RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: Send IPDU send flow payload to IPDU thread.\n", __FUNCTION__);
        rmf_osal_event_create(RMF_OSAL_EVENT_CATEGORY_POD, POD_IPDU_RECV_FLOW_DATA, &ipdu_event_params, &ipdu_event_handle);
        if ( rmf_osal_eventqueue_add(ipdu_event_queue, ipdu_event_handle ) != RMF_SUCCESS )
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s: rmf_osal_eventqueue_add Failed \n", __FUNCTION__);
        }
    }
}

void vlXchanUpdateInComingDsgCardIPUCounters(int nSize)
{
    vlg_DsgCableCardIPUStats.ifInUcastPkts++;
    vlg_DsgCableCardIPUStats.ifInOctets += nSize;
}

#ifdef USE_DSG
void vlXchanDispatchFlowData(unsigned long lFlowId, unsigned long nSize, unsigned char * pData)
{
    VL_XCHAN_FLOW_TYPE eFlowType = vlXchanGetFlowType(lFlowId);
		//jeska
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD","vlXchanDispatchFlowData: Recieved data from Ext Chnl eFlowType = %d, flow id=0x%06X\n", eFlowType, lFlowId);

    if( pData == NULL) return;

    if(0xC5 == *(pData))
    {
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "vlXchanDispatchFlowData: received STT table from CableCard\n");
        vlMpeosDumpBuffer(RDK_LOG_DEBUG, "LOG.RDK.POD", pData, nSize);
    }

    vlDsgUpdateXchanTrafficCounters(eFlowType, nSize);

    switch(eFlowType)
    {
        case VL_XCHAN_FLOW_TYPE_MPEG:
        {
            RDK_LOG(RDK_LOG_TRACE8, "LOG.RDK.POD","vlXchanDispatchFlowData: Recieved data from Ext Chnl MPEG flow: id=0x%06X\n", lFlowId);
            // Oct-09-2008: Bug-Fix: Extended channel data can be received from card in DSG mode also.

            switch(*pData)
            {
                case VL_DSG_BCAST_TABLE_ID_EAS:
                {
                    vlDsgUpdateDsgCardTrafficCounters(VL_DSG_BCAST_CLIENT_ID_SCTE_18, nSize);
                    if(!vlDsgIsEasTablesFromCableCard())
                    {
                        // drop tables supplied by cable-card
                        // as dsg broadcast data takes precedence
                        return;
                    }
                    vlDsgDispatchBcastToDSGCC(
                          VL_DSG_DIR_TYPE_EXT_CHNL_MPEG_FLOW,
                          VL_DSG_CLIENT_ID_ENC_TYPE_BCAST,
                          VL_DSG_BCAST_CLIENT_ID_SCTE_18,
                          pData, nSize);
                }
                break;
                
                case VL_DSG_BCAST_TABLE_ID_CVT:
                case VL_DSG_BCAST_TABLE_ID_XAIT:
                {
                    vlDsgUpdateDsgCardTrafficCounters(VL_DSG_BCAST_CLIENT_ID_XAIT, nSize);
                    if(!vlDsgIsXaitTablesFromCableCard())
                    {
                        // drop tables supplied by cable-card
                        // as dsg broadcast data takes precedence
                        return;
                    }
                    vlDsgDispatchBcastToDSGCC(
                          VL_DSG_DIR_TYPE_EXT_CHNL_MPEG_FLOW,
                          VL_DSG_CLIENT_ID_ENC_TYPE_BCAST,
                          VL_DSG_BCAST_CLIENT_ID_XAIT,
                          pData, nSize);
                }
                break;
                
                default:
                {
                    vlDsgUpdateDsgCardTrafficCounters(VL_DSG_BCAST_CLIENT_ID_SCTE_65, nSize);
                    if(!vlDsgIsScte65TablesFromCableCard())
                    {
                        // drop tables supplied by cable-card
                        // as dsg broadcast data takes precedence
                        return;
                    }
                    vlDsgDispatchBcastToDSGCC(
                          VL_DSG_DIR_TYPE_EXT_CHNL_MPEG_FLOW,
                          VL_DSG_CLIENT_ID_ENC_TYPE_BCAST,
                          VL_DSG_BCAST_CLIENT_ID_SCTE_65,
                          pData, nSize);
                }
                break;
                
            }

            AM_EXTENDED_DATA(lFlowId,(unsigned char *)pData, nSize);
        }
        break;

        case VL_XCHAN_FLOW_TYPE_IP_U:
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlXchanDispatchFlowData: Recieved data from Ext Chnl IP_U flow: id=0x%06lX\n", lFlowId);
#ifdef USE_DSG
            if(vlIsDsgOrIpduMode())
            {
                vlDsgSendIPUdataToECM(lFlowId, nSize, pData);
                vlg_DsgCableCardIPUStats.ifOutUcastPkts++;
                vlg_DsgCableCardIPUStats.ifOutOctets   += nSize;
            }
#endif
        }
        break;

        case VL_XCHAN_FLOW_TYPE_IP_M:
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlXchanDispatchFlowData: Recieved data from Ext Chnl IP_M flow: id=0x%06X\n", lFlowId);
            vlXchanSendIPMdataToHost(lFlowId, nSize, pData);
        }
        break;
        
        case VL_XCHAN_FLOW_TYPE_SOCKET:
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlXchanDispatchFlowData: Recieved data from Ext Chnl SOCKET flow: id=0x%06X\n", lFlowId);
            vlXchanSendSOCKETdataToHost(lFlowId, nSize, pData);
        }
        break;

        case VL_XCHAN_FLOW_TYPE_NC:
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlXchanDispatchFlowData: Recieved data from non-categorized flow: id=0x%06lX\n", lFlowId);
        }
        break;

        case VL_XCHAN_FLOW_TYPE_IPDM:
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlXchanDispatchFlowData: Recieved data from VL_XCHAN_FLOW_TYPE_IPDM flow: id=0x%06lX\n", lFlowId);
        }
        break;

				case VL_XCHAN_FLOW_TYPE_IPDIRECT_UNICAST:
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlXchanDispatchFlowData: Recieved data from VL_XCHAN_FLOW_TYPE_IPDIRECT_UNICAST flow: id=0x%06lX\n", lFlowId);
            vlXchanProcessIpduData(lFlowId, nSize, pData);
        }
        break;

        default:
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlXchanDispatchFlowData: Recieved data from un-registered flow: id=0x%06lX\n", lFlowId);
            vlMpeosDumpBuffer(RDK_LOG_INFO, "LOG.RDK.POD", pData, nSize);
        }
        break;
    }
}
#endif // USE_DSG

/**************************  IP MULTICAST FLOWS ******************************/

VL_XCHAN_IP_MULTICAST_FLOW_PARAMS * vlXChanAllocateIpMulticastFlow()
{
    int i = 0, nFlowsAvailable =0;
    VL_XCHAN_IP_MULTICAST_FLOW_PARAMS * pFlow = NULL;

    for(i = 0; i < VL_MAX_IP_MULTICAST_FLOWS; i++)
    {
        if(FALSE == vlg_aDsgIpMulticastFlows[i].bInUse)
        {
            nFlowsAvailable++;
            VL_ZERO_STRUCT(vlg_aDsgIpMulticastFlows[i]);
            pFlow = &(vlg_aDsgIpMulticastFlows[i]);
        }
    }

    vlg_nXchanIpMulticastFlowsAvailable = vlMax(nFlowsAvailable-1, 0);

    return pFlow;
}

void vlXChanIpMulticastFlowTask(void *arg)
{

		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    VL_XCHAN_IP_MULTICAST_FLOW_PARAMS flow = *((VL_XCHAN_IP_MULTICAST_FLOW_PARAMS*)arg);
    unsigned char buf[4096];
    char groupAddress[128];
    struct sockaddr_in sai;
    socklen_t fromlen = sizeof(sai);

    APDU_XCHAN2POD_send_IP_M_flowCnf(flow.sesnum, VL_XCHAN_FLOW_RESULT_GRANTED, vlg_nXchanIpMulticastFlowsAvailable, flow.flowid);
    
            
    memset(&sai,0,sizeof(sai));
    sai.sin_family = AF_INET;
    sai.sin_port = htons(flow.portNum);
 /*   sprintf(groupAddress, "%u.%u.%u.%u",
            flow.groupId[0],
            flow.groupId[1],
            flow.groupId[2],
            flow.groupId[3]);*/
snprintf(groupAddress,sizeof(groupAddress), "%u.%u.%u.%u",
            flow.groupId[0],
            flow.groupId[1],
            flow.groupId[2],
            flow.groupId[3]);

    if ( inet_aton(groupAddress, &sai.sin_addr) == 0)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlXChanIpMulticastFlowTask: Invalid multicast IP addr %s\n", groupAddress);
    }
    
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlXChanIpMulticastFlowTask: Starting service for MULTICAST flow: id=0x%06lX, sock = %lu, group = %s, port = %d\n", flow.flowid, flow.socket, groupAddress, flow.portNum);
    
    while(1)
    {
        int nReceived = recvfrom(flow.socket, buf, sizeof(buf), 0,
                                 (struct sockaddr*)&sai, &fromlen);
        if(nReceived > 0)
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlXChanIpMulticastFlowTask: Received %d bytes of data for Ext Chnl MULTICAST flow: id=0x%06lX ...writing to Card\n", nReceived, flow.flowid);
            POD_STACK_SEND_ExtCh_Data(buf, nReceived, flow.flowid);
        }
        else
        {
            if(flow.socket > 0) close(flow.socket);
            
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlXChanIpMulticastFlowTask: Received %d bytes of data for Ext Chnl MULTICAST flow: id=0x%06lX ...dropping the flow.\n", nReceived, flow.flowid);
            
            if(0 == nReceived)
            {
                APDU_XCHAN2POD_lostFlowInd(flow.sesnum, flow.flowid, VL_XCHAN_LOST_FLOW_REASON_TCP_SOCKET_CLOSED);
            }
            else
            {
                APDU_XCHAN2POD_lostFlowInd(flow.sesnum, flow.flowid, VL_XCHAN_LOST_FLOW_REASON_SOCKET_READ_ERROR);
            }
            
            return;
        }
    }
}

void vlXChanServiceIpMulticastFlow(VL_XCHAN_IP_MULTICAST_FLOW_PARAMS * pFlow)
{
    rmf_osal_ThreadId threadId = 0;
    pFlow->bInUse = 1;
    pFlow->flowid = vlXchanHostAssignFlowId();
    rmf_osal_threadCreate(vlXChanIpMulticastFlowTask, (void *)pFlow,
        250, 20000, &threadId, "vlXChanIpMulticastFlowTask");

    
}

VL_XCHAN_FLOW_RESULT vlXchanRequestMulticastFromECM(VL_XCHAN_IP_MULTICAST_FLOW_PARAMS * pMulticastFlowParams)
{
    VL_XCHAN_FLOW_RESULT xchanResult = VL_XCHAN_FLOW_RESULT_DENIED_PORT_IN_USE;
    struct ifreq ifr;
    int result = 0;
//    char cmd_buffer[128];
    if(pMulticastFlowParams->socket > 0)
    {
        char host[128];
        struct sockaddr_in localifaddr;
        struct sockaddr_in mcastifaddr;
        struct in_addr group_addr;
        struct ip_mreq mreq;

    memset(&group_addr,0,sizeof(group_addr));
    memset(&ifr,0,sizeof(ifr));
    memset(&localifaddr,0,sizeof(localifaddr));
    memset(&mcastifaddr,0,sizeof(mcastifaddr));
    memset(&mreq,0,sizeof(mreq));
        
        strcpy(ifr.ifr_name, "eth0"); // test statement
#if !defined(VL_DSG_LOCAL_FLOW_TEST)
        {
            VL_HOST_IP_INFO hostIpInfo;
            vlXchanGetDsgIpInfo(&hostIpInfo);
            strncpy( ifr.ifr_name, hostIpInfo.szIfName, IFNAMSIZ );
            ifr.ifr_name[ IFNAMSIZ -1 ] = 0;					
        }
#endif // VL_DSG_LOCAL_FLOW_TEST
        ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;

        if ((result = ioctl(pMulticastFlowParams->socket, SIOCGIFADDR, &ifr)) < 0)
        {
            //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlDsgRequestMulticastFromECM: Cannot get IP addr for %s, ret %d\n", ifr.ifr_name, result);
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s():Cannot get IP addr for %s, ret %d\n", __FUNCTION__,ifr.ifr_name, result);
            xchanResult = VL_XCHAN_FLOW_RESULT_DENIED_NO_NETWORK;
        }
        else
        {
            memcpy((char *)&localifaddr, (char *) &ifr.ifr_ifru.ifru_addr, sizeof(struct sockaddr));
            char * ipAddr = inet_ntoa(localifaddr.sin_addr);
            if(NULL != ipAddr)
            {
            /*    sprintf(host, "%u.%u.%u.%u",
                        pMulticastFlowParams->groupId[0],
                        pMulticastFlowParams->groupId[1],
                        pMulticastFlowParams->groupId[2],
                        pMulticastFlowParams->groupId[3]);*/
  snprintf(host,sizeof(host), "%u.%u.%u.%u",
                        pMulticastFlowParams->groupId[0],
                        pMulticastFlowParams->groupId[1],
                        pMulticastFlowParams->groupId[2],
                        pMulticastFlowParams->groupId[3]);

                if ( inet_aton(host, &group_addr) == 0)
                {
                    //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Invalid multicast IP addr %s\n", host);
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s():Invalid multicast IP addr %s\n", __FUNCTION__,host);
                    xchanResult = VL_XCHAN_FLOW_RESULT_DENIED_DNS_FAILED;
                }
                else
                {
                    mcastifaddr.sin_family = AF_INET;
                    mcastifaddr.sin_addr   = group_addr;
                    mcastifaddr.sin_port   = htons(pMulticastFlowParams->portNum);
                    
                    mreq.imr_multiaddr = group_addr;
                    mreq.imr_interface.s_addr = htonl(INADDR_ANY); // test statement
#ifdef VL_DSG_MULTICAST_CONSTRAINED_TO_PHYSICAL_IF
                    // add constraint to physical interface
                    mreq.imr_interface = localifaddr.sin_addr;
#endif // VL_DSG_MULTICAST_CONSTRAINED_TO_PHYSICAL_IF

                    // Reuse address.
                    {
                        int reuse = 1;
                        if( setsockopt (pMulticastFlowParams->socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof (reuse)) < 0 )
                        {
                            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s():setsockopt SO_REUSEADDR error:  %s \n", __FUNCTION__,strerror(errno));

                        }
                    }
                    
                    if (bind(pMulticastFlowParams->socket, (struct sockaddr *)&mcastifaddr, sizeof(mcastifaddr)) < 0)
                    {
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s():bind to '%s' failed.\n", __FUNCTION__,host);
                        xchanResult = VL_XCHAN_FLOW_RESULT_DENIED_PORT_IN_USE;
                    }
                    else
                    {
                        if(setsockopt (pMulticastFlowParams->socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
                        {
                            perror("vlXchanRequestMulticastFromECM: setsockopt(add membership)");
                            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s():FAILED to add Multicast Membership for group %s at %s [%s]\n", __FUNCTION__,host, ifr.ifr_name, ipAddr);
                            xchanResult = VL_XCHAN_FLOW_RESULT_DENIED_NO_NETWORK;
                        }
                        else
                        {
                            // indicate success
                            pMulticastFlowParams->bSuccess = 1;
                            // Turn loopback OFF.
                            {
                                unsigned char loop = 0;
                                if( setsockopt(pMulticastFlowParams->socket, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) < 0 )
                                {
                                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s():setsockopt IP_MULTICAST_LOOP error:  %s \n", __FUNCTION__,strerror(errno));

                                }
                            }
                            #ifdef USE_DSG
                                xchanResult = (VL_XCHAN_FLOW_RESULT)CHALDsg_Set_Config(VL_CM_DSG_DEVICE_INSTANCE, VL_DSG_PVT_TAG_IP_MULTICAST_FLOW, pMulticastFlowParams);
                            #endif
                            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s():Added Multicast Membership for group %s at %s [%s]\n", __FUNCTION__,host, ifr.ifr_name, ipAddr);
                            return xchanResult;
                        }
                    }
                }
            }
        }
    }

    return xchanResult;
}

VL_XCHAN_FLOW_RESULT vlXchanCancelMulticastFromECM(VL_XCHAN_IP_MULTICAST_FLOW_PARAMS * pMulticastFlowParams)
{
    VL_XCHAN_FLOW_RESULT xchanResult = VL_XCHAN_FLOW_RESULT_DENIED_PORT_IN_USE;
    struct ifreq ifr;
    int result = 0;
//    char cmd_buffer[128];
    if(pMulticastFlowParams->socket > 0)
    {
        char host[128];
        struct sockaddr_in localifaddr;
        struct sockaddr_in mcastifaddr;
        struct in_addr group_addr;
        struct ip_mreq mreq;

	        memset(&group_addr,0,sizeof(group_addr));
	        memset(&ifr,0,sizeof(ifr));
	        memset(&localifaddr,0,sizeof(localifaddr));
	        memset(&mcastifaddr,0,sizeof(mcastifaddr));
	        memset(&mreq,0,sizeof(mreq));
        
        strcpy(ifr.ifr_name, "eth0"); // test statement
#if !defined(VL_DSG_LOCAL_FLOW_TEST)
        {
            VL_HOST_IP_INFO hostIpInfo;
            vlXchanGetDsgIpInfo(&hostIpInfo);
            strncpy(ifr.ifr_name, hostIpInfo.szIfName, IFNAMSIZ);
                ifr.ifr_name [ IFNAMSIZ - 1 ] = 0;
        }
#endif // VL_DSG_LOCAL_FLOW_TEST
        ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;

        if ((result = ioctl(pMulticastFlowParams->socket, SIOCGIFADDR, &ifr)) < 0)
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s():Cannot get IP addr for %s, ret %d\n", __FUNCTION__,ifr.ifr_name, result);
            xchanResult = VL_XCHAN_FLOW_RESULT_DENIED_NO_NETWORK;
        }
        else
        {
            memcpy((char *)&localifaddr, (char *) &ifr.ifr_ifru.ifru_addr, sizeof(struct sockaddr));
            char * ipAddr = inet_ntoa(localifaddr.sin_addr);
            if(NULL != ipAddr)
            {
      /*          sprintf(host, "%u.%u.%u.%u",
                        pMulticastFlowParams->groupId[0],
                        pMulticastFlowParams->groupId[1],
                        pMulticastFlowParams->groupId[2],
                        pMulticastFlowParams->groupId[3]);*/
snprintf(host,sizeof(host), "%u.%u.%u.%u",
                        pMulticastFlowParams->groupId[0],
                        pMulticastFlowParams->groupId[1],
                        pMulticastFlowParams->groupId[2],
                        pMulticastFlowParams->groupId[3]);

                if ( inet_aton(host, &group_addr) == 0)
                {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s():Invalid multicast IP addr %s\n", __FUNCTION__,host);
                    xchanResult = VL_XCHAN_FLOW_RESULT_DENIED_DNS_FAILED;
                }
                else
                {
                    mcastifaddr.sin_family = AF_INET;
                    mcastifaddr.sin_addr   = group_addr;
                    mcastifaddr.sin_port   = htons(pMulticastFlowParams->portNum);
                    
                    mreq.imr_multiaddr = group_addr;
                    mreq.imr_interface.s_addr = htonl(INADDR_ANY); // test statement
#ifdef VL_DSG_MULTICAST_CONSTRAINED_TO_PHYSICAL_IF
                    // add constraint to physical interface
                    mreq.imr_interface = localifaddr.sin_addr;
#endif // VL_DSG_MULTICAST_CONSTRAINED_TO_PHYSICAL_IF

                    if(setsockopt (pMulticastFlowParams->socket, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
                    {
                        perror("vlXchanCancelMulticastFromECM: setsockopt(drop membership)");
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s():FAILED to drop Multicast Membership for group %s at %s [%s]\n", __FUNCTION__,host, ifr.ifr_name, ipAddr);
                        xchanResult = VL_XCHAN_FLOW_RESULT_DENIED_NO_NETWORK;
                    }
                    else
                    {
                        // indicate success
                        pMulticastFlowParams->bSuccess = 1;
                        #ifdef USE_DSG
                            xchanResult = (VL_XCHAN_FLOW_RESULT)CHALDsg_Set_Config(VL_CM_DSG_DEVICE_INSTANCE, VL_DSG_PVT_TAG_IP_CANCEL_MULTICAST_FLOW, pMulticastFlowParams);
                        #endif
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s():Dropped Multicast Membership for group %s at %s [%s]\n", __FUNCTION__,host, ifr.ifr_name, ipAddr);
                        return xchanResult;
                    }
                }
            }
        }
    }

    return xchanResult;
}
    
void vlXChanDeallocateIpMulticastFlow(unsigned long nFlowId)
{
    int i = 0;
    VL_XCHAN_IP_MULTICAST_FLOW_PARAMS * pFlow = NULL;

    for(i = 0; i < VL_MAX_IP_MULTICAST_FLOWS; i++)
    {
        if(nFlowId == vlg_aDsgIpMulticastFlows[i].flowid)
        {
            pFlow = &(vlg_aDsgIpMulticastFlows[i]);
        }
    }

    if(NULL != pFlow)
    {
        unsigned long threadId = pFlow->threadId;
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlXChanDeallocateIpMulticastFlow: Stopping service for MULTICAST flow: id=0x%06lX, sock = %lu, group = '%d.%d.%d.%d', port = %d\n",
                            pFlow->flowid, pFlow->socket,
                            pFlow->groupId[0], pFlow->groupId[1],
                            pFlow->groupId[2], pFlow->groupId[3],
                            pFlow->portNum);
        vlXchanCancelMulticastFromECM(pFlow);
        if(pFlow->socket > 0) close(pFlow->socket);
        VL_ZERO_STRUCT(*pFlow);
//        rmf_osal_threadDestroy(threadId);
    }
}

int vlXChanCreateIpMulticastFlow(short sesnum, unsigned char * groupId)
{
    VL_XCHAN_FLOW_RESULT xchanResult = VL_XCHAN_FLOW_RESULT_DENIED_NET_BUSY;
    VL_XCHAN_IP_MULTICAST_FLOW_PARAMS * pFlow = vlXChanAllocateIpMulticastFlow();
    if(NULL == pFlow)
    {
        APDU_XCHAN2POD_send_IP_M_flowCnf(sesnum, VL_XCHAN_FLOW_RESULT_DENIED_FLOWS_EXCEEDED, vlg_nXchanIpMulticastFlowsAvailable, 0);
    }
    else
    {
       // unsigned long sock = 0;
         int sock = 0;
        sock = socket(AF_INET, SOCK_DGRAM, 0);
        if( -1 != sock )
        {
            char groupAddress[128];
            struct sockaddr_in sai;

            if( sock == 0 ) 
            {
       	 	RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD",
       	 	"\n\n\n%s: Closed file descriptor 0 !!\n\n\n\n",__FUNCTION__);
            }
            
            pFlow->socket = sock;
            pFlow->sesnum = sesnum;
            pFlow->portNum = 4321;
            memcpy(pFlow->groupId, groupId, VL_IP_ADDR_SIZE);
            
            xchanResult = vlXchanRequestMulticastFromECM(pFlow);
            
            if(VL_XCHAN_FLOW_RESULT_GRANTED != xchanResult)
            {
                pFlow->bInUse = 0;
                close(pFlow->socket);
                APDU_XCHAN2POD_send_IP_M_flowCnf(sesnum, xchanResult, vlg_nXchanIpMulticastFlowsAvailable+1, 0);
                return 0;
            }

	    memset(&sai,0,sizeof(sai));
            sai.sin_family = AF_INET;
            sai.sin_port = htons(pFlow->portNum);
 /*           sprintf(groupAddress, "%u.%u.%u.%u",
                    groupId[0],
                    groupId[1],
                    groupId[2],
                    groupId[3]);*/
   snprintf(groupAddress,sizeof(groupAddress), "%u.%u.%u.%u",
                    groupId[0],
                    groupId[1],
                    groupId[2],
                    groupId[3]);

            if ( inet_aton(groupAddress, &sai.sin_addr) == 0)
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlXChanCreateIpMulticastFlow: Invalid multicast IP addr %s\n", groupAddress);
                APDU_XCHAN2POD_send_IP_M_flowCnf(sesnum, VL_XCHAN_FLOW_RESULT_DENIED_BAD_MAC_ADDR, vlg_nXchanIpMulticastFlowsAvailable, 0);
            }
            else
            {
                vlXChanServiceIpMulticastFlow(pFlow);
                return 1;
            }
            // create flow failed
            close(pFlow->socket);
            pFlow->bInUse = 0;
            return 0;
        }
        else
        {
            APDU_XCHAN2POD_send_IP_M_flowCnf(sesnum, VL_XCHAN_FLOW_RESULT_DENIED_NO_NETWORK, vlg_nXchanIpMulticastFlowsAvailable, 0);
        }
        
        // create flow failed
        pFlow->bInUse = 0;
        return 0;
    }

    // allocate flow failed
    return 0;
}

/**************************  SOCKET FLOWS ******************************/

VL_XCHAN_SOCKET_FLOW_PARAMS * vlXChanAllocateSocketFlow()
{
    int i = 0, nFlowsAvailable =0;
    VL_XCHAN_SOCKET_FLOW_PARAMS * pFlow = NULL;

    for(i = 0; i < VL_MAX_SOCKET_FLOWS; i++)
    {
        if(FALSE == vlg_aDsgSocketFlows[i].bInUse)
        {
            nFlowsAvailable++;
            pFlow = &(vlg_aDsgSocketFlows[i]);
        }
    }

    vlg_nXchanSocketFlowsAvailable = vlMax(nFlowsAvailable-1, 0);

    return pFlow;
}

void vlXChanDeallocateSocketFlow(unsigned long nFlowId)
{
    int i = 0;
    VL_XCHAN_SOCKET_FLOW_PARAMS * pFlow = NULL;

    for(i = 0; i < VL_MAX_SOCKET_FLOWS; i++)
    {
        if(nFlowId == vlg_aDsgSocketFlows[i].flowid)
        {
            pFlow = &(vlg_aDsgSocketFlows[i]);
        }
    }

    if(NULL != pFlow)
    {
        unsigned long threadId = pFlow->threadId;
        unsigned long flowid   = pFlow->flowid;
        unsigned long socket   = pFlow->socket;
        if(pFlow->socket > 0) close(pFlow->socket);
        VL_ZERO_STRUCT(*pFlow);
        if(0 != threadId) rmf_osal_threadDestroy((rmf_osal_ThreadId)(threadId));
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlXChanDeallocateSocketFlow: de-allocated flow: id=0x%06lX, sock %lu, threadId = 0x%08X.\n", flowid, socket, threadId);
    }
}

void vlXChanSocketConnectTimeoutTask(void *lParam)
{
    VL_XCHAN_SOCKET_FLOW_PARAMS * pFlow = ((VL_XCHAN_SOCKET_FLOW_PARAMS*)lParam);

    if(NULL == pFlow) return;
    
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlXChanSocketConnectTimeoutTask: sleeping for %d seconds...\n", pFlow->connTimeout);
    sleep(pFlow->connTimeout);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlXChanSocketConnectTimeoutTask: slept for %d seconds...\n", pFlow->connTimeout);

    if((pFlow->bInUse) && (0 == pFlow->flowid) && (0 == pFlow->socket))
    {
        if(pFlow->socket > 0) close(pFlow->socket);
        pFlow->socket = 0;
        pFlow->bInUse = 0;
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlXChanSocketConnectTimeoutTask: reached %d seconds timeout\n", pFlow->connTimeout);
        APDU_XCHAN2POD_send_SOCKET_flowCnf(pFlow->sesnum, VL_XCHAN_FLOW_RESULT_DENIED_TCP_FAILED, vlg_nXchanSocketFlowsAvailable+1, 0,
                                        NULL);
    }
}

static int g_nSocketFlowRunawayCount = 0;

void vlXChanSocketFlowTask(void  *lParam)
{
    long sock = 0;
    char szHost[VL_DNS_ADDR_SIZE];
    unsigned char linkIpAddr[VL_IPV6_ADDR_SIZE];
    char szPort[VL_IPV6_ADDR_SIZE];
    
    struct addrinfo hints, *res, *res0;
    int error = 0;

    strcpy(szHost, "");
    strcpy(szPort, "");
    memset(linkIpAddr, 0, sizeof(linkIpAddr));
    
    VL_XCHAN_SOCKET_FLOW_PARAMS * pFlow = ((VL_XCHAN_SOCKET_FLOW_PARAMS*)lParam);
    
    memset(&hints, 0, sizeof(hints));
    
    // specify the family
    switch(pFlow->eRemoteAddressType)
    {
        case VL_XCHAN_SOCKET_ADDRESS_TYPE_NAME:
        {
            hints.ai_family = PF_UNSPEC;
        }
        break;

        case VL_XCHAN_SOCKET_ADDRESS_TYPE_IPV4:
        {
            hints.ai_family = AF_INET;
            hints.ai_flags |= AI_NUMERICHOST;
        }
        break;

        case VL_XCHAN_SOCKET_ADDRESS_TYPE_IPV6:
        {
            hints.ai_family = AF_INET6;
            hints.ai_flags |= AI_NUMERICHOST;
        }
        break;
            
        default:
        {
        }
        break;
    }
    
    // specify the protocol
    if(VL_XCHAN_SOCKET_PROTOCOL_TYPE_UDP == pFlow->eProtocol)
    {
        hints.ai_socktype  = SOCK_DGRAM;
        hints.ai_protocol |= IPPROTO_UDP;
        hints.ai_flags    |= AI_PASSIVE;
    }
    else if(VL_XCHAN_SOCKET_PROTOCOL_TYPE_TCP == pFlow->eProtocol)
    {
        hints.ai_socktype  = SOCK_STREAM;
        hints.ai_protocol |= IPPROTO_TCP;
    }
    // specify the port
  //  sprintf(szPort, "%u", pFlow->nRemotePortNum);
      snprintf(szPort,sizeof(szPort), "%u", pFlow->nRemotePortNum);
    // specify the host
    switch(pFlow->eRemoteAddressType)
    {
        case VL_XCHAN_SOCKET_ADDRESS_TYPE_NAME:
        {
            memset(szHost, 0, sizeof(szHost));
            strncpy(szHost, (char*)(pFlow->dnsAddress), sizeof(szHost)-1);
        }
        break;

        case VL_XCHAN_SOCKET_ADDRESS_TYPE_IPV4:
        {
            snprintf(szHost, sizeof(szHost), "%u.%u.%u.%u",
                    pFlow->ipAddress[0],
                    pFlow->ipAddress[1],
                    pFlow->ipAddress[2],
                    pFlow->ipAddress[3]);
        }
        break;

        case VL_XCHAN_SOCKET_ADDRESS_TYPE_IPV6:
        {
            snprintf(szHost, sizeof(szHost), "%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X",
                    pFlow->ipV6address[ 0], pFlow->ipV6address[ 1],
                    pFlow->ipV6address[ 2], pFlow->ipV6address[ 3],
                    pFlow->ipV6address[ 4], pFlow->ipV6address[ 5],
                    pFlow->ipV6address[ 6], pFlow->ipV6address[ 7],
                    pFlow->ipV6address[ 8], pFlow->ipV6address[ 9],
                    pFlow->ipV6address[10], pFlow->ipV6address[11],
                    pFlow->ipV6address[12], pFlow->ipV6address[13],
                    pFlow->ipV6address[14], pFlow->ipV6address[15]);
        }
        break;

        default:
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlXChanSocketFlowTask: Unexpected address type (%d) requested\n", pFlow->eRemoteAddressType);
        }
        break;
    }

    // resolve names or IP addresses
    error = getaddrinfo(szHost, szPort, &hints, &res0);
    if (error)
    {
        perror("vlXChanSocketFlowTask: getaddrinfo");
        switch(pFlow->eRemoteAddressType)
        {
            case VL_XCHAN_SOCKET_ADDRESS_TYPE_NAME:
            {
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlXChanSocketFlowTask: Remote Address '%s' could not be resolved\n", szHost);
            }
            break;

            case VL_XCHAN_SOCKET_ADDRESS_TYPE_IPV4:
            case VL_XCHAN_SOCKET_ADDRESS_TYPE_IPV6:
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlXChanSocketFlowTask: Invalid remote addr '%s'\n", szHost);
            }
            break;
            
            default:
            {
            }
            break;
        }
        APDU_XCHAN2POD_send_SOCKET_flowCnf(pFlow->sesnum, VL_XCHAN_FLOW_RESULT_DENIED_DNS_FAILED, vlg_nXchanSocketFlowsAvailable+1, 0,
                NULL);
        pFlow->bInUse = 0;
        return;
    }
    

    int iAddr = 0;
    int nIPv4addressCount = 0;
    int nIPv6addressCount = 0;
    for (res = res0; NULL != res; res = res->ai_next)
    {
        iAddr++;
        char szIpAddr[VL_IPV6_ADDR_STR_SIZE];
        char * pszIpAddrType = "IPv4";
        if(AF_INET6 == res->ai_family)
        {
            pszIpAddrType = "IPv6";
            nIPv6addressCount++;
        }
        else
        {
            nIPv4addressCount++;
        }
        getnameinfo(res->ai_addr, res->ai_addrlen, szIpAddr, sizeof(szIpAddr), NULL, 0, NI_NUMERICHOST);
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlXChanSocketFlowTask: Remote Address '%s' resolves to %d: (%s Address, '%s').\n", szHost, iAddr, pszIpAddrType, szIpAddr);
    }
    VL_HOST_IP_INFO hostIpInfo;
    vlXchanGetHostIpTypeInfo(VL_HOST_IP_IF_TYPE_DSG, 0, &hostIpInfo);
    if(VL_XCHAN_IP_ADDRESS_TYPE_IPV4 == hostIpInfo.ipAddressType)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlXChanSocketFlowTask: STB currently has IPv4 address '%s'.\n", hostIpInfo.szIpAddress);
    }
    if(VL_XCHAN_IP_ADDRESS_TYPE_IPV6 == hostIpInfo.ipAddressType)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlXChanSocketFlowTask: STB currently has IPv6 address '%s'.\n", hostIpInfo.szIpV6GlobalAddress);
    }
    
    if((nIPv6addressCount > 0) && (nIPv4addressCount <= 0) && (VL_XCHAN_IP_ADDRESS_TYPE_IPV4 == hostIpInfo.ipAddressType))
    {
        pFlow->bInUse = 0;
        freeaddrinfo(res0);
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlXChanSocketFlowTask: STB currently has IPv4 address '%s'. Returning VL_XCHAN_FLOW_RESULT_DENIED_NO_IPV6.\n", hostIpInfo.szIpAddress);
        APDU_XCHAN2POD_send_SOCKET_flowCnf(pFlow->sesnum, VL_XCHAN_FLOW_RESULT_DENIED_NO_IPV6, vlg_nXchanSocketFlowsAvailable+1, 0,
                NULL);
        return;
    }
    
    pFlow->bInUse = 1;
    pFlow->flowid = 0;
    pFlow->timeoutThreadId = 0;
    if(pFlow->connTimeout > 0)
    {
        rmf_osal_ThreadId threadId = 0;
        rmf_osal_threadCreate(vlXChanSocketConnectTimeoutTask, (void *)pFlow,
            250, 20000, &threadId, "vlXChanSocketConnectTimeoutTask");
    

    }
    
    // create network I/O
    sock = -1;
    iAddr = 0;
    for (res = res0; NULL != res; res = res->ai_next)
    {
        iAddr++;
        char szIpAddr[VL_IPV6_ADDR_STR_SIZE];
        char * pszIpAddrType = "IPv4";
        if(AF_INET6 == res->ai_family) pszIpAddrType = "IPv6";
        getnameinfo(res->ai_addr, res->ai_addrlen, szIpAddr, sizeof(szIpAddr), NULL, 0, NI_NUMERICHOST);
        
        if((VL_XCHAN_IP_ADDRESS_TYPE_IPV4 == hostIpInfo.ipAddressType) && (AF_INET6 == res->ai_family))
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlXChanSocketFlowTask: Skipping entry %d: (%s Address, '%s') for '%s' as STB is operating in IPv4 mode.\n", iAddr, pszIpAddrType, szIpAddr, szHost);
            continue;
        }
        if((VL_XCHAN_IP_ADDRESS_TYPE_IPV6 == hostIpInfo.ipAddressType) && (AF_INET == res->ai_family))
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlXChanSocketFlowTask: Skipping entry %d: (%s Address, '%s') for '%s' as STB is operating in IPv6 mode.\n", iAddr, pszIpAddrType, szIpAddr, szHost);
            continue;
        }
        
        //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlXChanSocketFlowTask: creating socket for family %d, type %d, protocol %d\n", res->ai_family, res->ai_socktype, res->ai_protocol);
        sock = socket(res->ai_family, res->ai_socktype,
                   res->ai_protocol);
        if (sock < 0)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlXChanSocketFlowTask: create socket for '%s' failed\n", szHost);
            continue;
        }
        if( sock == 0 ) 
        {
   	 	RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD",
   	 	"\n\n\n%s: Closed file descriptor 0 !!\n\n\n\n",__FUNCTION__);
        }
        
        // Jul-13-2009: Removed discriminatory impl. based on value of (pFlow->eProtocol)
        //              This change is based on clarification on TCP flows by CableLabs
        //              As per CableLabs bind and connect are required for both UDP and TCP flows.
        // Feb-06-2009: Changed impl to meet requirements of TC 170.2
        // bind
        //Sunil - Added for having separate mechanism for IPV6 and IPV4 for broadcom
        if (res->ai_family == AF_INET6)
        {
        struct sockaddr_in6 serv_addr;
	memset(&(serv_addr),0,sizeof(serv_addr));
        // specify the port
        // Feb-18-2009: Commented: Using auto selection of UDP port: //if(0 == pFlow->nLocalPortNum) pFlow->nLocalPortNum = pFlow->nRemotePortNum;
        /* Code for address family */
        serv_addr.sin6_family = AF_INET6;

        /* IP address of the host */
        serv_addr.sin6_addr = in6addr_any;
        /* Port number */
        serv_addr.sin6_port = htons((u_short)pFlow->nLocalPortNum);

        // Feb-06-2009: Changed impl to meet requirements of TC 170.2
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlXChanSocketFlowTask: binding to port %d for '%s' ('%s'), port:%s\n    using F:%d (%s), T:%d (%s), P:%d (%s)\n",
                pFlow->nLocalPortNum, szHost, szIpAddr, szPort,
                res->ai_family, vlGetAddressFamilyName(res->ai_family),
                res->ai_socktype, vlGetSockTypeName(res->ai_socktype),
                res->ai_protocol, vlGetIpProtocolName(res->ai_protocol));
        /* Binding a socket to an address of the current host and port number */
        if (bind(sock, (struct sockaddr *) &serv_addr,
            sizeof(serv_addr)) < 0)
        {
            if(EADDRINUSE == errno)
            {   // Apr-15-2009: do not allow reuse of of udp ports
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlXChanSocketFlowTask : bind on socket failed error: %s \n", strerror(errno));
                // create flow failed
                close(sock);
                sock = -1;
                pFlow->bInUse = 0;
                freeaddrinfo(res0);
                APDU_XCHAN2POD_send_SOCKET_flowCnf(pFlow->sesnum, VL_XCHAN_FLOW_RESULT_DENIED_PORT_IN_USE, vlg_nXchanSocketFlowsAvailable+1, 0,
                        NULL);
                return;
            }
            else
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlXChanSocketFlowTask : bind on socket failed error: %s \n", strerror(errno));
                close(sock);
                sock = -1;
                continue;
            }
        }
        
        // Feb-18-2009: Using auto selection of UDP/TCP port 
        if(0 == pFlow->nLocalPortNum)
        {
            struct sockaddr_in6 sai;
            int8_t ret = 0;			
            socklen_t socklen = sizeof(sai);
            ret = getsockname(sock, (struct sockaddr *)&sai, &socklen);
            if( ret == 0 )
	     {
		pFlow->nLocalPortNum = ntohs(sai.sin6_port);
            	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlXChanSocketFlowTask: Selected nLocalPort = %d\n", pFlow->nLocalPortNum);
            }
            else
            {
              RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlXChanSocketFlowTask: Selected nLocalPort Failed\n");
            }
        }
        }
        else
        {
            struct sockaddr_in serv_addr;
            memset(&serv_addr, 0, sizeof(serv_addr));
            // specify the port
            // Feb-18-2009: Commented: Using auto selection of UDP port: //if(0 == pFlow->nLocalPortNum) pFlow->nLocalPortNum = pFlow->nRemotePortNum;
            /* Code for address family */
            serv_addr.sin_family = AF_INET;
            
            /* IP address of the host */
            serv_addr.sin_addr.s_addr = INADDR_ANY;
            /* Port number */
            serv_addr.sin_port = htons((u_short)pFlow->nLocalPortNum);
            
            
            // Feb-06-2009: Changed impl to meet requirements of TC 170.2
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlXChanSocketFlowTask: binding to port %d for '%s' ('%s'), port:%s\n    using F:%d (%s), T:%d (%s), P:%d (%s)\n",
                    pFlow->nLocalPortNum, szHost, szIpAddr, szPort,
                    res->ai_family, vlGetAddressFamilyName(res->ai_family),
                    res->ai_socktype, vlGetSockTypeName(res->ai_socktype),
                    res->ai_protocol, vlGetIpProtocolName(res->ai_protocol));
            /* Binding a socket to an address of the current host and port number */
            if (bind(sock, (struct sockaddr *) &serv_addr,
                sizeof(serv_addr)) < 0)
            {
                if(EADDRINUSE == errno)
                {   // Apr-15-2009: do not allow reuse of of udp ports
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlXChanSocketFlowTask : bind on socket failed error: %s \n", strerror(errno));
                    // create flow failed
                    close(sock);
                    sock = -1;
                    pFlow->bInUse = 0;
                    freeaddrinfo(res0);
                    APDU_XCHAN2POD_send_SOCKET_flowCnf(pFlow->sesnum, VL_XCHAN_FLOW_RESULT_DENIED_PORT_IN_USE, vlg_nXchanSocketFlowsAvailable+1, 0,
                            NULL);
                    return;
                }
                else
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlXChanSocketFlowTask : bind on socket failed error: %s \n", strerror(errno));
                    close(sock);
                    sock = -1;
                    continue;
                }
            }
                
            // Feb-18-2009: Using auto selection of UDP/TCP port 
            if(0 == pFlow->nLocalPortNum)
            {
                struct sockaddr_in sai;
                int8_t ret = 0;
                socklen_t socklen = sizeof(sai);
                ret = getsockname(sock, (struct sockaddr *)&sai, &socklen);
                if ( ret == 0 ) 
                {
                    pFlow->nLocalPortNum = ntohs(sai.sin_port);
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlXChanSocketFlowTask: Selected nLocalPort = %d\n", pFlow->nLocalPortNum);
                }
                else
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlXChanSocketFlowTask: Selected nLocalPort Failed\n");
                }
            }
         }    
        // Feb-06-2009: Changed impl to meet requirements of TC 170.2
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlXChanSocketFlowTask: %s: connecting to '%s' ('%s') port:%s\n    using F:%d (%s), T:%d (%s), P:%d (%s)\n",
                VL_COND_STR(pFlow->eProtocol, TCP, UDP),
                szHost, szIpAddr, szPort,
                res->ai_family, vlGetAddressFamilyName(res->ai_family),
                res->ai_socktype, vlGetSockTypeName(res->ai_socktype),
                res->ai_protocol, vlGetIpProtocolName(res->ai_protocol));
        if (connect(sock, res->ai_addr, res->ai_addrlen) < 0)
        {
            perror("vlXChanSocketFlowTask: connect");
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlXChanSocketFlowTask: connect to '%s' ('%s') failed.\n", szHost, szIpAddr);
            close(sock);
            sock = -1;
            continue;
        }
        // Jul-13-2009: Removed discriminatory impl. based on value of (pFlow->eProtocol)
        //              This change is based on clarification on TCP flows by CableLabs
        //              As per CableLabs bind and connect are required for both UDP and TCP flows.
        /* connect succeeded */

        pFlow->nRemoteAddrLen = res->ai_addrlen;
        memcpy(pFlow->remote_addr, res->ai_addr, pFlow->nRemoteAddrLen);
        //vlMpeosDumpBuffer(RDK_LOG_DEBUG, "LOG.RDK.POD", res->ai_addr, res->ai_addrlen);
        // make a note of what we will be using
        hints.ai_family = res->ai_family;
        
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlXChanSocketFlowTask: Established Socket flow to '%s' with entry %d: (%s Address, '%s').\n", szHost, iAddr, pszIpAddrType, szIpAddr);
        break;  /* bind & connect succeeded */
    }

    freeaddrinfo(res0);
    
     // check if the flow has timed-out
    if(0 == pFlow->bInUse)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlXChanSocketFlowTask: flow creation failed or timed-out\n");
        return;
    }

    // destroy any timeout threads
    if(0 != pFlow->timeoutThreadId)
    {
//        rmf_osal_threadDestroy(pFlow->timeoutThreadId);
    }
    if(-1 != sock )
    {
        pFlow->socket = sock;
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlXChanSocketFlowTask: sock is NULL\n");
        if(VL_XCHAN_SOCKET_PROTOCOL_TYPE_TCP == pFlow->eProtocol)
        {
            APDU_XCHAN2POD_send_SOCKET_flowCnf(pFlow->sesnum, VL_XCHAN_FLOW_RESULT_DENIED_TCP_FAILED, vlg_nXchanSocketFlowsAvailable+1, 0,
                                               NULL);
        }
        else
        {
            APDU_XCHAN2POD_send_SOCKET_flowCnf(pFlow->sesnum, VL_XCHAN_FLOW_RESULT_DENIED_NO_NETWORK, vlg_nXchanSocketFlowsAvailable+1, 0,
                                            NULL);
        }
        // create flow failed
        pFlow->bInUse = 0;
        return;
    }
    pFlow->flowid = vlXchanHostAssignFlowId();
    pFlow->bInUse = 1;
    
    // notify success
    {
        unsigned char linkIpAddr[VL_IPV6_ADDR_SIZE];
        VL_HOST_IP_INFO hostIpInfo;
        
        memset(linkIpAddr, 0, sizeof(linkIpAddr));
        vlXchanGetDsgIpInfo(&hostIpInfo);
        switch(hints.ai_family)
        {
            case AF_INET6:
            {
                // Changed to global address in-lieu of link address as the STBs link-local address is not reachable from the controllers (esp. DNCS).
                // The DNCS needs to form a UDP unicast datagram directed to the Host(STB) to communicate to the cable-card via an UDP socket flow.
                memcpy(linkIpAddr, hostIpInfo.aBytIpV6GlobalAddress, sizeof(hostIpInfo.aBytIpV6GlobalAddress));
            }
            break;
            
            default:
            {
                memcpy(linkIpAddr+12, hostIpInfo.aBytIpAddress, sizeof(hostIpInfo.aBytIpAddress));
            }
        }
        
        APDU_XCHAN2POD_send_SOCKET_flowCnf(pFlow->sesnum, VL_XCHAN_FLOW_RESULT_GRANTED, vlg_nXchanSocketFlowsAvailable, pFlow->flowid,
                                           linkIpAddr);
    }
    
    g_nSocketFlowRunawayCount++;
    
    {
        VL_XCHAN_SOCKET_FLOW_PARAMS flow = *((VL_XCHAN_SOCKET_FLOW_PARAMS*)lParam);
        unsigned char buf[4096];
        while(1)
        {
            int nReceived = recv(flow.socket, buf, sizeof(buf), 0);
            
            if(nReceived > 0)
            {
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlXChanSocketFlowTask: Received %d bytes of data from Ext Chnl SOCKET [%s] flow: id=0x%06lX ...writing to Card\n", nReceived, VL_COND_STR(flow.eProtocol, TCP, UDP), flow.flowid);
                POD_STACK_SEND_ExtCh_Data(buf, nReceived, flow.flowid);
            }
            else
            {
                perror("vlXChanSocketFlowTask: recv");
                if(-1 != sock ) close(flow.socket);
                
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlXChanSocketFlowTask: Received %d bytes of data for Ext Chnl SOCKET flow: id=0x%06lX ...dropping the flow.\n", nReceived, flow.flowid);
                
                if(0 == nReceived)
                {
                    APDU_XCHAN2POD_lostFlowInd(flow.sesnum, flow.flowid, VL_XCHAN_LOST_FLOW_REASON_TCP_SOCKET_CLOSED);
                }
                else
                {
                    if(VL_XCHAN_SOCKET_PROTOCOL_TYPE_TCP == pFlow->eProtocol)
                    {
                        APDU_XCHAN2POD_lostFlowInd(flow.sesnum, flow.flowid, VL_XCHAN_LOST_FLOW_REASON_TCP_SOCKET_CLOSED);
                    }
                    else
                    {   // TC170.2 expects write error
                        APDU_XCHAN2POD_lostFlowInd(flow.sesnum, flow.flowid, VL_XCHAN_LOST_FLOW_REASON_SOCKET_WRITE_ERROR);
                    }
                }
                
                return;
            }
        }
    }
}

void vlXChanServiceSocketFlow(VL_XCHAN_SOCKET_FLOW_PARAMS * pFlow)
{
    pFlow->bInUse = 1;
    rmf_osal_ThreadId threadId = 0;
    rmf_osal_threadCreate(vlXChanSocketFlowTask, (void *)pFlow,
        250, 20000, &threadId, "vlXChanSocketFlowTask");
        
    pFlow->threadId = threadId;
}

int vlXChanCreateSocketFlow(short sesnum, VL_XCHAN_SOCKET_FLOW_PARAMS * pFlowParams)
{
    if(g_nSocketFlowRunawayCount > 0x100)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlXChanServiceSocketFlow: Cable-Card socket flow run-away condition detected. g_nSocketFlowRunawayCount = %d. Denying flow request.\n", g_nSocketFlowRunawayCount);
        APDU_XCHAN2POD_send_SOCKET_flowCnf(sesnum, VL_XCHAN_FLOW_RESULT_DENIED_FLOWS_EXCEEDED, 0, 0, NULL); // Deny socket flow and indicate no more flows available.
        return 0;
    }
    
    VL_XCHAN_SOCKET_FLOW_PARAMS * pFlow = vlXChanAllocateSocketFlow();
    if(NULL == pFlow)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlXChanCreateSocketFlow: flows exceeded\n");
        APDU_XCHAN2POD_send_SOCKET_flowCnf(sesnum, VL_XCHAN_FLOW_RESULT_DENIED_FLOWS_EXCEEDED, vlg_nXchanSocketFlowsAvailable, 0,
                                        NULL);
        return 0;
    }
    else
    {
        *pFlow = *pFlowParams;
        pFlow->sesnum = sesnum;
        vlXChanServiceSocketFlow(pFlow);
    }
    return 1;
}

#define VL_COPY_V6_ADDRESS(type)                                                                                \
    memcpy(pHostIpInfo->aBytIpV6##type##Address, ipaddr, sizeof(pHostIpInfo->aBytIpV6##type##Address)        \
                                                           );       \
    strncpy(pHostIpInfo->szIpV6##type##Address, addr6, VL_IPV6_ADDR_STR_SIZE );            \
    pHostIpInfo->szIpV6##type##Address[ VL_IPV6_ADDR_STR_SIZE -1 ] = 0; \
    pHostIpInfo->nIpV6##type##Plen = plen
    
void vlXchanGetDsgEthName(VL_HOST_IP_INFO * pHostIpInfo)
{
    memset(&(*pHostIpInfo),0,sizeof(*pHostIpInfo));
#ifdef USE_DSG
    CHALDsg_Set_Config(VL_CM_DSG_DEVICE_INSTANCE, VL_DSG_PVT_TAG_DSG_GET_IP_INFO, pHostIpInfo);
#endif // USE_DSG
}

int vlXchanCheckIfHostAcquiredIpAddress()
{
    // check if DSG or QPSK ip address has been acquired
    if(vlg_bHostAcquiredIPAddress || vlg_bHostAcquiredQpskIPAddress || vlg_bHostAcquiredProxyAddress)
    {
        return 1;
    }
    return 0;
}

void vlXchanGetHostIpTypeInfo(VL_HOST_IP_IF_TYPE eType, int iInstance, VL_HOST_IP_INFO * pHostIpInfo)
{
    if(NULL != pHostIpInfo)
    {
        memset(pHostIpInfo,0,sizeof(*pHostIpInfo));
        strncpy((char*)(pHostIpInfo->szIpAddress ), "0.0.0.0", sizeof(pHostIpInfo->szIpAddress)-1);
        strncpy((char*)(pHostIpInfo->szSubnetMask), "0.0.0.0", sizeof(pHostIpInfo->szSubnetMask)-1);

        pHostIpInfo->iInstance = iInstance;

        switch(eType&VL_HOST_IP_IF_TYPE_MASK)
        {
            case VL_HOST_IP_IF_TYPE_ETH_DEBUG:
            {
                // this case is used only during development
                strncpy(pHostIpInfo->szIfName, "eth0", sizeof(pHostIpInfo->szIfName)-1);
            }
            break;

            case VL_HOST_IP_IF_TYPE_POD_QPSK:
            {
                CHALPod_oob_control(0, VL_OOB_COMMAND_GET_IP_INFO, pHostIpInfo);
            }
            break;

            case VL_HOST_IP_IF_TYPE_HN:
            {
                const char* pszHnIf = rmf_osal_envGet("FEATURE.MRDVR.MEDIASERVER.IFNAME");
                if (NULL == pszHnIf) pszHnIf = "lan0";
                strncpy(pHostIpInfo->szIfName, pszHnIf, VL_IF_NAME_SIZE);
                pHostIpInfo->szIfName [VL_IF_NAME_SIZE-1] = 0;
            }
            break;

            case VL_HOST_IP_IF_TYPE_DSG:
            {
                #ifdef USE_DSG
                    CHALDsg_Set_Config(VL_CM_DSG_DEVICE_INSTANCE, VL_DSG_PVT_TAG_DSG_GET_IP_INFO, pHostIpInfo);
                #endif // USE_DSG
            }
            break;
        }
        
        pHostIpInfo->ipAddressType = VL_XCHAN_IP_ADDRESS_TYPE_IPV4;
        {//getip
            int result = 0;
   //         unsigned long  sock = socket(AF_INET, SOCK_DGRAM, 0);
            int  sock = socket(AF_INET, SOCK_DGRAM, 0);
            unsigned long ipAddress = 0;
            if(-1 != sock )
            {
                if( sock == 0 ) 
                {
           	 	RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD",
           	 	"\n\n\n%s: Closed file descriptor 0 !!\n\n\n\n",__FUNCTION__);
                }				
                struct ifreq ifr;
                struct sockaddr_in sai;
                memset(&ifr,0,sizeof(ifr));
                memset(&sai,0,sizeof(sai));
                struct sockaddr_in  *sin  = (struct sockaddr_in  *)&(ifr.ifr_addr);
                struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&(ifr.ifr_addr);
                sai.sin_family = AF_INET;
                sai.sin_port = 0;

//                strcpy(ifr.ifr_name, pHostIpInfo->szIfName);
                strncpy(ifr.ifr_name, pHostIpInfo->szIfName, IFNAMSIZ);
                ifr.ifr_name [ IFNAMSIZ - 1 ] = 0;		

                
                sin->sin_family = AF_INET;
                if ((result = ioctl(sock, SIOCGIFADDR, &ifr)) < 0)
                {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s(): Cannot get IPV4 addr for %s, ret %d\n", __FUNCTION__, pHostIpInfo->szIfName, result);
                    pHostIpInfo->ipAddressType = VL_XCHAN_IP_ADDRESS_TYPE_NONE;
                }
                else
                {
                    memcpy((char *)&sai, (char *) &ifr.ifr_ifru.ifru_addr, sizeof(struct sockaddr));
                    memcpy(pHostIpInfo->aBytIpAddress, &sai.sin_addr.s_addr, sizeof(sai.sin_addr.s_addr));
                    char * pIpAddr = inet_ntoa(sai.sin_addr);
                    if(NULL != pIpAddr)
                    {
//                        strcpy(pHostIpInfo->szIpAddress, pIpAddr);
                        strncpy( pHostIpInfo->szIpAddress, pIpAddr, VL_IP_ADDR_STR_SIZE );
                        pHostIpInfo->szIpAddress [VL_IP_ADDR_STR_SIZE - 1] = 0;
						
                   /*     sprintf(pHostIpInfo->szIpAddress, "%d.%d.%d.%d",
                            pHostIpInfo->aBytIpAddress[0], pHostIpInfo->aBytIpAddress[1],
                            pHostIpInfo->aBytIpAddress[2], pHostIpInfo->aBytIpAddress[3]);
                        //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s(): Got IPV4 Address '%s'\n", __FUNCTION__, pHostIpInfo->szIpAddress);*/
                     snprintf(pHostIpInfo->szIpAddress,sizeof(pHostIpInfo->szIpAddress), "%d.%d.%d.%d",
                            pHostIpInfo->aBytIpAddress[0], pHostIpInfo->aBytIpAddress[1],
                            pHostIpInfo->aBytIpAddress[2], pHostIpInfo->aBytIpAddress[3]);
                        //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s(): Got IPV4 Address '%s'\n", __FUNCTION__, pHostIpInfo->szIpAddress);
                        ipAddress = VL_VALUE_4_FROM_ARRAY(pHostIpInfo->aBytIpAddress);
                    
                        pHostIpInfo->aBytIpV6v4Address[10] = 0xFF;
                        pHostIpInfo->aBytIpV6v4Address[11] = 0xFF;
                        pHostIpInfo->aBytIpV6v4Address[12] = pHostIpInfo->aBytIpAddress[0];
                        pHostIpInfo->aBytIpV6v4Address[13] = pHostIpInfo->aBytIpAddress[1];
                        pHostIpInfo->aBytIpV6v4Address[14] = pHostIpInfo->aBytIpAddress[2];
                        pHostIpInfo->aBytIpV6v4Address[15] = pHostIpInfo->aBytIpAddress[3];
                    
                        inet_ntop(AF_INET6,pHostIpInfo->aBytIpV6v4Address,
                                  pHostIpInfo->szIpV6v4Address,sizeof(pHostIpInfo->szIpV6v4Address));

                        //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s(): Got IPV6 V4 Address '%s'\n", __FUNCTION__, pHostIpInfo->szIpV6v4Address);
                    }
                }

                ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
                if ((result = ioctl(sock, SIOCGIFNETMASK, &ifr)) < 0)
                {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s(): Cannot get subnet mask for %s, ret %d\n", __FUNCTION__, pHostIpInfo->szIfName, result);
                }
                else
                {
                    memcpy((char *)&sai, (char *) &ifr.ifr_ifru.ifru_netmask, sizeof(struct sockaddr));
                    char * pSubnetMask = inet_ntoa(sai.sin_addr);
                    memcpy(pHostIpInfo->aBytSubnetMask, &sai.sin_addr.s_addr, sizeof(sai.sin_addr.s_addr));
                    if(NULL != pSubnetMask)
                    {
                        int nMaskLen = 96, iMask = 0;
                        unsigned long subnetMask = VL_VALUE_4_FROM_ARRAY(pHostIpInfo->aBytSubnetMask);
                        strncpy(pHostIpInfo->szSubnetMask, pSubnetMask, VL_IP_ADDR_STR_SIZE );
                        pHostIpInfo->szSubnetMask [VL_IP_ADDR_STR_SIZE - 1] = 0;	
                        									
                        snprintf(pHostIpInfo->szSubnetMask, sizeof(pHostIpInfo->szSubnetMask), "%d.%d.%d.%d",
                            pHostIpInfo->aBytSubnetMask[0], pHostIpInfo->aBytSubnetMask[1],
                            pHostIpInfo->aBytSubnetMask[2], pHostIpInfo->aBytSubnetMask[3]);
                        //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s(): Got SubNetMask '%s'\n", __FUNCTION__, pHostIpInfo->szSubnetMask);
                        
                        // calculate V6 mask
                        while(0 != ((subnetMask<<iMask)&0xFFFFFFFF)) iMask++;
                        pHostIpInfo->nIpV6v4Plen = nMaskLen + iMask;
                        
                        //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s(): Got IPV6 V4 Mask '%d'\n", __FUNCTION__, pHostIpInfo->nIpV6v4Plen);
                    }
                }

                if ((result = ioctl(sock, SIOCGIFHWADDR, &ifr)) < 0)
                {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s():Cannot get MAC_ADDRESS for %s, ret %d\n", __FUNCTION__, pHostIpInfo->szIfName, result);
                }
                else
                {
                    unsigned char * pMacAddress = (unsigned char*)(ifr.ifr_hwaddr.sa_data);
                    memcpy(pHostIpInfo->aBytMacAddress, pMacAddress, sizeof(pHostIpInfo->aBytMacAddress));
                    snprintf(pHostIpInfo->szMacAddress, sizeof(pHostIpInfo->szMacAddress), "%02X:%02X:%02X:%02X:%02X:%02X",
                        pHostIpInfo->aBytMacAddress[0], pHostIpInfo->aBytMacAddress[1],
                        pHostIpInfo->aBytMacAddress[2], pHostIpInfo->aBytMacAddress[3],
                        pHostIpInfo->aBytMacAddress[4], pHostIpInfo->aBytMacAddress[5]);
                    //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s(): Got MacAddress '%s'\n", __FUNCTION__, pHostIpInfo->szMacAddress);
                    
                    pHostIpInfo->aBytIpV6LinkAddress[ 0] = 0xFE;
                    pHostIpInfo->aBytIpV6LinkAddress[ 1] = 0x80;
                    pHostIpInfo->aBytIpV6LinkAddress[ 8] = ((pHostIpInfo->aBytMacAddress[0]) ^ (0x02));
                    pHostIpInfo->aBytIpV6LinkAddress[ 9] = pHostIpInfo->aBytMacAddress[1];
                    pHostIpInfo->aBytIpV6LinkAddress[10] = pHostIpInfo->aBytMacAddress[2];
                    pHostIpInfo->aBytIpV6LinkAddress[11] = 0xFF;
                    pHostIpInfo->aBytIpV6LinkAddress[12] = 0xFE;
                    pHostIpInfo->aBytIpV6LinkAddress[13] = pHostIpInfo->aBytMacAddress[3];
                    pHostIpInfo->aBytIpV6LinkAddress[14] = pHostIpInfo->aBytMacAddress[4];
                    pHostIpInfo->aBytIpV6LinkAddress[15] = pHostIpInfo->aBytMacAddress[5];
                    
                    inet_ntop(AF_INET6,pHostIpInfo->aBytIpV6LinkAddress,
                              pHostIpInfo->szIpV6LinkAddress,sizeof(pHostIpInfo->szIpV6LinkAddress));
                    
                    //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s(): Got IPV6 Link Address '%s'\n", __FUNCTION__, pHostIpInfo->szIpV6LinkAddress);
                    
                    // calculate V6 mask
                    pHostIpInfo->nIpV6LinkPlen = 64;
                        
                    //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s(): Got IPV6 Link Mask '%d'\n", __FUNCTION__, pHostIpInfo->nIpV6LinkPlen);
                }
                
                if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0)
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","%s():SIOCGIFFLAGS failed with error '%s'\n", __FUNCTION__, strerror(errno));
                }
                else
                {
                    pHostIpInfo->lFlags      = ifr.ifr_flags;
                    pHostIpInfo->lLastChange = vlg_tcHostIpLastChange;
                }
                
                close(sock);
            } // if(sock)
            {// IPV6
                #define PROC_IFINET6_PATH "/proc/net/if_inet6"
                
                                /* refer to: linux/include/net/ipv6.h */
                FILE *fp;
                char addr[16][4];
                unsigned long ipchar = 0;
                unsigned char ipaddr[16];
                unsigned int index, plen, scope, flags;
                char ifname[128];
                char ifCorrected[128];
                char addr6[128];
                char str[128];
                int i = 0;
    
                memset(&str,0,sizeof(str));
                memset(&ifCorrected,0,sizeof(ifCorrected));
                strncpy(ifCorrected, pHostIpInfo->szIfName, sizeof(ifCorrected)-1);
                char *pszColon = strchr(ifCorrected, ':');
                if(NULL != pszColon) *pszColon = '\0';

                if ((fp = fopen(PROC_IFINET6_PATH, "r")) != NULL)
                {
                    //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Loop\n");
                    while((fgets(str, sizeof(str), fp)) != NULL)
                    {
                        sscanf(str, 
                               "%2s%2s%2s%2s%2s%2s%2s%2s%2s%2s%2s%2s%2s%2s%2s%2s %02x %02x %02x %02x %8s",
                               addr[0],addr[1],addr[2],addr[3],addr[4],addr[5],addr[6],addr[7],
                               addr[8],addr[9],addr[10],addr[11],addr[12],addr[13],addr[14],addr[15],
                               &index, &plen, &scope, &flags, ifname);
            
                        if(0 != strcmp(ifCorrected, ifname)) continue;
                        
                        for(i = 0; i < 16; i++)
                        {
                            sscanf(addr[i], "%x", &ipchar);
                            ipaddr[i] = (unsigned char)(ipchar);
                        }
            
                        inet_ntop(AF_INET6, ipaddr, addr6, sizeof(addr6));
            
                        if(IPV6_ADDR_LINKLOCAL != scope)
                        {
                            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s(): %8s: %39s/%d scope:%s\n", __FUNCTION__, ifname, addr6, plen, vlGetIpScopeName(scope));
                        }
                        
                        switch(scope)
                        {
                            case IPV6_ADDR_ANY:
                            {
                                VL_COPY_V6_ADDRESS(Global);
                                pHostIpInfo->ipAddressType = VL_XCHAN_IP_ADDRESS_TYPE_IPV6;
                            }
                            break;
                            
                            case IPV6_ADDR_LINKLOCAL:
                            {
                                VL_COPY_V6_ADDRESS(Link);
                            }
                            break;
                            
                            case IPV6_ADDR_SITELOCAL:
                            {
                                VL_COPY_V6_ADDRESS(Site);
                            }
                            break;
                            
                            case IPV6_ADDR_COMPATv4:
                            {
                                VL_COPY_V6_ADDRESS(v4);
                            }
                            break;
                        }
                    }// while
                    if(NULL != fp) fclose(fp);
                }// (NULL != fopen())
            }// IPV6
#if 0 // Mar-14-2009: VL_DISABLED_CODE
            {// STATS
                #define PROC_NET_DEV_PATH "/proc/net/dev"
                
                FILE *fp;
                char str[256];
                char szIfName[32];
                char szInUnicastPkts[32];
                char szOutUnicastPkts[32];
                char szInUnicastBytes[32];
                char szOutUnicastBytes[32];
                char szInErrors[32];
                char szOutErrors[32];
                char szInDiscards[32];
                char szOutDiscards[32];
                char szDummy[32];
                
                VL_ZERO_MEMORY(str              );
                VL_ZERO_MEMORY(szIfName         );
                
                VL_ZERO_MEMORY(szInUnicastPkts  );
                VL_ZERO_MEMORY(szOutUnicastPkts );
                VL_ZERO_MEMORY(szInUnicastBytes );
                VL_ZERO_MEMORY(szOutUnicastBytes);
                VL_ZERO_MEMORY(szInErrors       );
                VL_ZERO_MEMORY(szOutErrors      );
                VL_ZERO_MEMORY(szInDiscards     );
                VL_ZERO_MEMORY(szOutDiscards    );
                
                VL_ZERO_MEMORY(szDummy          );

                if ((fp = fopen(PROC_NET_DEV_PATH, "r")) != NULL)
                {
                    //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Loop\n");
                    while((fgets(str, sizeof(str), fp)) != NULL)
                    {
                        char * pszColon = strchr(str, ':');
                        if(NULL == pszColon) continue;
                        *pszColon = ' ';
                        sscanf(str, 
                               "%s%s%s%s%s%s%s%s%s%s%s%s%s",
                               szIfName,
                               szInUnicastBytes, szInUnicastPkts,
                               szInErrors, szInDiscards,
                               szDummy, szDummy, szDummy, szDummy,
                               szOutUnicastBytes, szOutUnicastPkts,
                               szOutErrors, szOutDiscards);
            
                        if(0 != strcmp(pHostIpInfo->szIfName, szIfName)) continue;
                        /*RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: %s, %s, %s, %s, %s, %s, %s, %s, %s\n", __FUNCTION__, szIfName,
                               szInUnicastBytes, szInUnicastPkts,
                               szInErrors, szInDiscards,
                               szOutUnicastBytes, szOutUnicastPkts,
                               szOutErrors, szOutDiscards);*/
                        pHostIpInfo->stats.ifInUcastPkts    = strtoul(szInUnicastPkts   , NULL, 10);
                        pHostIpInfo->stats.ifOutUcastPkts   = strtoul(szOutUnicastPkts  , NULL, 10);
                        pHostIpInfo->stats.ifInOctets       = strtoul(szInUnicastBytes  , NULL, 10);
                        pHostIpInfo->stats.ifOutOctets      = strtoul(szOutUnicastBytes , NULL, 10);
                        pHostIpInfo->stats.ifInErrors       = strtoul(szInErrors        , NULL, 10);
                        pHostIpInfo->stats.ifOutErrors      = strtoul(szOutErrors       , NULL, 10);
                        pHostIpInfo->stats.ifInDiscards     = strtoul(szInDiscards      , NULL, 10);
                        pHostIpInfo->stats.ifOutDiscards    = strtoul(szOutDiscards     , NULL, 10);
                        /*RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: %s, %d, %d, %d, %d\n", __FUNCTION__, szIfName,
                               pHostIpInfo->stats.ifInOctets,
                               pHostIpInfo->stats.ifInUcastPkts,
                               pHostIpInfo->stats.ifOutOctets,
                               pHostIpInfo->stats.ifOutUcastPkts);*/
                    }// while
                    if(NULL != fp) fclose(fp);
                }// (NULL != fopen())
                else
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","%s():fopen('%s') failed with error '%s'\n", __FUNCTION__, PROC_NET_DEV_PATH, strerror(errno));
                }
            }// STATS
#endif // VL_DISABLED_CODE
        }// getip
    }// (NULL != pHostIpInfo)
}

void vlXchanGetDsgIpInfo(VL_HOST_IP_INFO * pHostIpInfo)
{
#ifdef USE_DSG
    if(!vlIsDsgOrIpduMode())
    {
        vlXchanGetHostIpTypeInfo(VL_HOST_IP_IF_TYPE_POD_QPSK, 0, pHostIpInfo);
        VL_HOST_IP_INFO dsgIpInfo;
        vlXchanGetHostIpTypeInfo(VL_HOST_IP_IF_TYPE_DSG, 0, &dsgIpInfo);
        memcpy(pHostIpInfo->aBytMacAddress, dsgIpInfo.aBytMacAddress, sizeof(dsgIpInfo.aBytMacAddress));
        snprintf(pHostIpInfo->szMacAddress,sizeof(pHostIpInfo->szMacAddress), "%02X:%02X:%02X:%02X:%02X:%02X",
                 pHostIpInfo->aBytMacAddress[0], pHostIpInfo->aBytMacAddress[1],
                 pHostIpInfo->aBytMacAddress[2], pHostIpInfo->aBytMacAddress[3],
                 pHostIpInfo->aBytMacAddress[4], pHostIpInfo->aBytMacAddress[5]);
    }
    else
#endif
    vlXchanGetHostIpTypeInfo(VL_HOST_IP_IF_TYPE_DSG, 0, pHostIpInfo);
}
    
void vlXchanGetPodIpInfo(VL_HOST_IP_INFO * pHostIpInfo)
{
    vlXchanGetHostIpTypeInfo(VL_HOST_IP_IF_TYPE_POD_QPSK, 0, pHostIpInfo);
}
    
void vlXchanGetDsgCableCardIpInfo(VL_HOST_IP_INFO * pHostIpInfo)
{
    if(NULL != pHostIpInfo)
    {
	memset(&(*pHostIpInfo),0,sizeof(*pHostIpInfo));
        strncpy((char*)(pHostIpInfo->szIpAddress ), "0.0.0.0", sizeof(pHostIpInfo->szIpAddress)-1);
        strncpy((char*)(pHostIpInfo->szSubnetMask), "0.0.0.0", sizeof(pHostIpInfo->szSubnetMask)-1);

        pHostIpInfo->iInstance = 0;

        strncpy(pHostIpInfo->szIfName, "dsgmcard-pseudo0", sizeof(pHostIpInfo->szIfName)-1);
        
        // populate IP address, subnet mask and MAC address
        vlXchanGetCableCardDsgModeIPUinfo(pHostIpInfo);

        // V4 string
     /*   sprintf(pHostIpInfo->szIpAddress, "%d.%d.%d.%d",
                pHostIpInfo->aBytIpAddress[0], pHostIpInfo->aBytIpAddress[1],
                pHostIpInfo->aBytIpAddress[2], pHostIpInfo->aBytIpAddress[3]);*/
   snprintf(pHostIpInfo->szIpAddress,sizeof(pHostIpInfo->szIpAddress), "%d.%d.%d.%d",
                pHostIpInfo->aBytIpAddress[0], pHostIpInfo->aBytIpAddress[1],
                pHostIpInfo->aBytIpAddress[2], pHostIpInfo->aBytIpAddress[3]);
        //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s(): Got IPV4 Address '%s'\n", __FUNCTION__, pHostIpInfo->szIpAddress);
                    
        // V6v4 addr
        pHostIpInfo->aBytIpV6v4Address[10] = 0xFF;
        pHostIpInfo->aBytIpV6v4Address[11] = 0xFF;
        pHostIpInfo->aBytIpV6v4Address[12] = pHostIpInfo->aBytIpAddress[0];
        pHostIpInfo->aBytIpV6v4Address[13] = pHostIpInfo->aBytIpAddress[1];
        pHostIpInfo->aBytIpV6v4Address[14] = pHostIpInfo->aBytIpAddress[2];
        pHostIpInfo->aBytIpV6v4Address[15] = pHostIpInfo->aBytIpAddress[3];
        
        // V6v4 string
        inet_ntop(AF_INET6,pHostIpInfo->aBytIpV6v4Address,
                    pHostIpInfo->szIpV6v4Address,sizeof(pHostIpInfo->szIpV6v4Address));

        //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s(): Got IPV6 V4 Address '%s'\n", __FUNCTION__, pHostIpInfo->szIpV6v4Address);

        // V4 mask string
        int nMaskLen = 96, iMask = 0;
        unsigned long subnetMask = VL_VALUE_4_FROM_ARRAY(pHostIpInfo->aBytSubnetMask);
        snprintf(pHostIpInfo->szSubnetMask,sizeof(pHostIpInfo->szSubnetMask), "%d.%d.%d.%d",
                pHostIpInfo->aBytSubnetMask[0], pHostIpInfo->aBytSubnetMask[1],
                pHostIpInfo->aBytSubnetMask[2], pHostIpInfo->aBytSubnetMask[3]);
        //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s(): Got SubNetMask '%s'\n", __FUNCTION__, pHostIpInfo->szSubnetMask);
                        
        // calculate V6 mask
        while(0 != ((subnetMask<<iMask)&0xFFFFFFFF)) iMask++;
        pHostIpInfo->nIpV6v4Plen = nMaskLen + iMask;
                        
        //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s(): Got IPV6 V4 Mask '%d'\n", __FUNCTION__, pHostIpInfo->nIpV6v4Plen);

        // V4 MAC string
        snprintf(pHostIpInfo->szMacAddress,sizeof(pHostIpInfo->szMacAddress), "%02X:%02X:%02X:%02X:%02X:%02X",
                pHostIpInfo->aBytMacAddress[0], pHostIpInfo->aBytMacAddress[1],
                pHostIpInfo->aBytMacAddress[2], pHostIpInfo->aBytMacAddress[3],
                pHostIpInfo->aBytMacAddress[4], pHostIpInfo->aBytMacAddress[5]);
        //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s(): Got MacAddress '%s'\n", __FUNCTION__, pHostIpInfo->szMacAddress);
                    
        // V6 link addr
        pHostIpInfo->aBytIpV6LinkAddress[ 0] = 0xFE;
        pHostIpInfo->aBytIpV6LinkAddress[ 1] = 0x80;
        pHostIpInfo->aBytIpV6LinkAddress[ 8] = ((pHostIpInfo->aBytMacAddress[0]) ^ (0x02));
        pHostIpInfo->aBytIpV6LinkAddress[ 9] = pHostIpInfo->aBytMacAddress[1];
        pHostIpInfo->aBytIpV6LinkAddress[10] = pHostIpInfo->aBytMacAddress[2];
        pHostIpInfo->aBytIpV6LinkAddress[11] = 0xFF;
        pHostIpInfo->aBytIpV6LinkAddress[12] = 0xFE;
        pHostIpInfo->aBytIpV6LinkAddress[13] = pHostIpInfo->aBytMacAddress[3];
        pHostIpInfo->aBytIpV6LinkAddress[14] = pHostIpInfo->aBytMacAddress[4];
        pHostIpInfo->aBytIpV6LinkAddress[15] = pHostIpInfo->aBytMacAddress[5];
                    
        // V6 link string
        inet_ntop(AF_INET6,pHostIpInfo->aBytIpV6LinkAddress,
                    pHostIpInfo->szIpV6LinkAddress,sizeof(pHostIpInfo->szIpV6LinkAddress));
                    
        //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s(): Got IPV6 Link Address '%s'\n", __FUNCTION__, pHostIpInfo->szIpV6LinkAddress);
                    
        // calculate V6 mask
        pHostIpInfo->nIpV6LinkPlen = 64;
                        
        //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s(): Got IPV6 Link Mask '%d'\n", __FUNCTION__, pHostIpInfo->nIpV6LinkPlen);
        
        if(VL_XCHAN_IP_ADDRESS_TYPE_NONE != pHostIpInfo->ipAddressType)
        {
            pHostIpInfo->lFlags      = IFF_UP | IFF_RUNNING;
        }
        
        pHostIpInfo->lLastChange    = vlg_tcCardDsgIPULastChange;
        
        pHostIpInfo->stats          = vlg_DsgCableCardIPUStats;
        
        /*RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: %s, %d, %d, %d, %d\n", __FUNCTION__, pHostIpInfo->szIfName,
            pHostIpInfo->nInUnicastBytes,
            pHostIpInfo->nInUnicastPkts,
            pHostIpInfo->nOutUnicastBytes,
            pHostIpInfo->nOutUnicastPkts);*/
    }// (NULL != pHostIpInfo)
}
    
int vlXchanGetV6SubnetMaskFromPlen(int nPlen, unsigned char * pBuf, int nBufCapacity)
{
    int iByte = 0, iBit = 0;
    if(nBufCapacity != VL_IPV6_ADDR_SIZE) return 0;
    memset(pBuf, 0, nBufCapacity);
    for(iByte = 0, iBit = 7; ((iByte < nBufCapacity) && (nPlen > 0)); nPlen--)
    {
        pBuf[iByte] |= (0x1<<iBit);
        
        iBit--;
        if(iBit < 0)
        {
            iBit = 7;
            iByte++;
        }
    }
    
    return VL_IPV6_ADDR_SIZE;
}

int vlXchanSetNetworkRoute(unsigned long ipNet, unsigned long ipMask, unsigned long ipGateway, char * pszIfName)
{
    return vlNetSetNetworkRoute(ipNet, ipMask, ipGateway, pszIfName);
}

int vlXchanDelNetworkRoute(unsigned long ipNet, unsigned long ipMask, unsigned long ipGateway, char * pszIfName)
{
    return vlNetDelNetworkRoute(ipNet, ipMask, ipGateway, pszIfName);
}

int vlXchanSetEcmRoute()
{
    VL_HOST_IP_INFO hostIpInfo;
    vlXchanGetDsgEthName(&hostIpInfo);
#ifdef INTEL_CANMORE
    const char * envPtr =  vl_env_get_str("eth1:1", "VL_ECM_RPC_IF_NAME");        
    strncpy(hostIpInfo.szIfName,envPtr, VL_IF_NAME_SIZE);
    hostIpInfo.szIfName[VL_IF_NAME_SIZE - 1] = 0;
    return vlXchanSetNetworkRoute(VL_XCHAN_LOCAL_ECM_IP_ADDRESS, 0xFFFFFFFF, 0, hostIpInfo.szIfName);
#endif
    return 0;
}

int vlXchanDelDefaultEcmGatewayRoute()
{
    VL_HOST_IP_INFO hostIpInfo;
    vlXchanGetDsgEthName(&hostIpInfo);
    return vlXchanDelNetworkRoute(0, 0, VL_XCHAN_LOCAL_ECM_IP_ADDRESS, hostIpInfo.szIfName);
}

void vlXchanGetPodIpStatus(VL_POD_IP_STATUS * pPodIpStatus)
{
    if(NULL != pPodIpStatus)
    {
        pPodIpStatus->eFlowResult = vlg_eXchanFlowResult;
    //    sprintf(pPodIpStatus->szPodIpStatus, "FLOW_STATUS : %s", cExtChannelGetFlowResultName(pPodIpStatus->eFlowResult));
        snprintf(pPodIpStatus->szPodIpStatus, sizeof(pPodIpStatus->szPodIpStatus),"FLOW_STATUS : %s", cExtChannelGetFlowResultName(pPodIpStatus->eFlowResult));
    }
};

long vlXchanGetSnmpSysUpTime()
{
    FILE *fp = fopen("/proc/uptime", "r");
    long uptime = 0;
    long secs   = 0;
    long hsecs  = 0;
    if (NULL != fp)
    {
        if (2 == fscanf(fp, "%ld.%ld", &secs, &hsecs))
        {
            uptime = secs * 100 + hsecs;
        }
        fclose(fp);
    }
    return uptime;
}

#ifdef __cplusplus
}
#endif

