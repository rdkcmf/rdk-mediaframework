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



#include "poddef.h"
#include <lcsm_log.h>

#include <string.h>
#include "utils.h"
#include "transport.h"
#include "session.h"
#include "podhighapi.h"
#include "appmgr.h"
#include "applitest.h"

#include "dsg-main.h"
#include "xchan_interface.h"
#include "utilityMacros.h"
#include "bufParseUtilities.h"
#include "ip_types.h"
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <net/if_arp.h>
#include "ip_types.h"
#include "vlXchanFlowManager.h"
#include "xchanResApi.h"
#include <string.h>

#ifdef USE_DSG
#include "dsgResApi.h"
#include "vlpluginapp_haldsgapi.h"
#else // USE_DSG
#define vlDsgSndApdu2Pod(a,b,c,d)   xchanSndApdu((a),(b),(c),(d))
#endif // else USE_DSG
#include "vlpluginapp_halpodapi.h"

#ifdef __cplusplus
extern "C" {
#endif


static unsigned long  vlg_xchan_pod_res_id = 0;
static VL_DSG_IP_UNICAST_FLOW_PARAMS   vlg_dsgIpUnicastFlowParams;
extern unsigned long       vlg_ucid;
extern int vlg_bHostAcquiredIPAddress;
int vlg_bDsgIpuFlowRequestPending = 0;
extern int vlg_dsgInquiryPerformed;

static char * vlg_aszFlowNames[] = {"MPEG", "IP_U", "IP_M", "DSG", "Socket",
                             "Other", "Other", "Other", "Other"};

static unsigned char vlg_acQpskRcMacAddress[VL_MAC_ADDR_SIZE];
static unsigned char vlg_acDsgRcMacAddress[VL_MAC_ADDR_SIZE];
static unsigned char vlg_acQpskRcIpAddress[VL_IP_ADDR_SIZE];
static unsigned char vlg_acDsgRcIpAddress[VL_IP_ADDR_SIZE];

static unsigned short vlg_nXChanSession = 0;

static int xchanSndApdu( unsigned short sesnum, unsigned long tag, unsigned short dataLen, unsigned char *data );
void vlXchanSetResourceId(unsigned long ulResId)
{
    vlg_xchan_pod_res_id = ulResId;
}

ULONG vlMapToXChanTag(USHORT sesnum, ULONG ulTag)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s: sesnum=%u, ulTag=%u \n", __FUNCTION__, sesnum, ulTag);
    switch(vlg_xchan_pod_res_id)
    {
        case VL_XCHAN_RESOURCE_ID_VER_1:
        case VL_XCHAN_RESOURCE_ID_VER_2:
        case VL_XCHAN_RESOURCE_ID_VER_3:
        case VL_XCHAN_RESOURCE_ID_VER_4:
        {
            switch(ulTag)
            {
                case VL_HOST2POD_INQUIRE_DSG_MODE:
                case VL_XCHAN_HOST2POD_INQUIRE_DSG_MODE:
                {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "vlMapToXChanTag returning VL_XCHAN_HOST2POD_INQUIRE_DSG_MODE\n");
                    return VL_XCHAN_HOST2POD_INQUIRE_DSG_MODE;
                }
                break;

                case VL_HOST2POD_SEND_DCD_INFO:
                case VL_XCHAN_HOST2POD_SEND_DCD_INFO:
                {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "vlMapToXChanTag returning VL_XCHAN_HOST2POD_SEND_DCD_INFO\n");
                    return VL_XCHAN_HOST2POD_SEND_DCD_INFO;
                }
                break;

                case VL_HOST2POD_DSG_MESSAGE:
                case VL_XCHAN_HOST2POD_DSG_MESSAGE:
                {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "vlMapToXChanTag returning VL_XCHAN_HOST2POD_DSG_MESSAGE\n");
                    return VL_XCHAN_HOST2POD_DSG_MESSAGE;
                }
                break;
            }
        }
        break;

        case VL_XCHAN_RESOURCE_ID_VER_5:
        case VL_XCHAN_RESOURCE_ID_VER_6:
        default:
        {
            switch(ulTag)
            {
                case VL_HOST2POD_INQUIRE_DSG_MODE:
                case VL_XCHAN_HOST2POD_INQUIRE_DSG_MODE:
                {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "vlMapToXChanTag returning VL_HOST2POD_INQUIRE_DSG_MODE\n");
                    return VL_HOST2POD_INQUIRE_DSG_MODE;
                }
                break;

                case VL_HOST2POD_SEND_DCD_INFO:
                case VL_XCHAN_HOST2POD_SEND_DCD_INFO:
                {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "vlMapToXChanTag returning VL_HOST2POD_SEND_DCD_INFO\n");
                    return VL_HOST2POD_SEND_DCD_INFO;
                }
                break;

                case VL_HOST2POD_DSG_MESSAGE:
                case VL_XCHAN_HOST2POD_DSG_MESSAGE:
                {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "vlMapToXChanTag returning VL_HOST2POD_DSG_MESSAGE\n");
                    return VL_HOST2POD_DSG_MESSAGE;
                }
                break;
            }
        }
        break;
    }

    return ulTag;
}

unsigned long vlXchanGetResourceId()
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    return vlg_xchan_pod_res_id;
}

unsigned char * vlXchanGetRcMacAddress()
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    static unsigned char vlg_acNullRcMacAddress[VL_MAC_ADDR_SIZE];
#ifdef USE_DSG
    if(vlIsDsgOrIpduMode() && (0 != vlg_ucid))
    {
        if(vlGetIpuFlowId()&0x800000)
        {
            return vlg_acDsgRcMacAddress;
        }
    }
    else
#endif // USE_DSG
    {
        if(0 == (vlGetIpuFlowId()&0x800000))
        {
            if( (vlGetIpuFlowId()&0x0FFFFF))
            {
                return vlg_acQpskRcMacAddress;
            }
        }
    }

    memset(vlg_acNullRcMacAddress, 0, sizeof(vlg_acNullRcMacAddress));
    return vlg_acNullRcMacAddress;
}

unsigned char * vlXchanGetRcIpAddress()
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    static unsigned char vlg_acNullRcIpAddress[VL_IP_ADDR_SIZE];
#ifdef USE_DSG
    if(vlIsDsgOrIpduMode() && (0 != vlg_ucid))
    {
        if(vlGetIpuFlowId()&0x800000)
        {
            return vlg_acDsgRcIpAddress;
        }
    }
    else
#endif // USE_DSG
    {
        if(0 == (vlGetIpuFlowId()&0x800000))
        {
            if( (vlGetIpuFlowId()&0x0FFFFF))
            {
                return vlg_acQpskRcIpAddress;
            }
        }
    }

    memset(vlg_acNullRcIpAddress, 0, sizeof(vlg_acNullRcIpAddress));
    return vlg_acNullRcIpAddress;
}

void vlXchanGetHostCardIpInfo(VL_HOST_CARD_IP_INFO * pCardIpInfo)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    if(NULL != pCardIpInfo)
    {
        unsigned char * pIpAddress  = vlXchanGetRcIpAddress();
        unsigned char * pMacAddress = vlXchanGetRcMacAddress();
	memset(&(*pCardIpInfo),0,sizeof(*pCardIpInfo));
        memcpy(pCardIpInfo->ipAddress, pIpAddress,
                 sizeof(pCardIpInfo->ipAddress));
#ifdef USE_DSG
    if(vlIsDsgOrIpduMode() && (0 != vlg_ucid))
    {
        memcpy(pCardIpInfo->macAddress, pMacAddress,
                 sizeof(pCardIpInfo->macAddress));
    }
#endif // USE_DSG
        pCardIpInfo->ipAddressType = VL_XCHAN_IP_ADDRESS_TYPE_IPV4;
    }
}

void vlXchanGetCableCardDsgModeIPUinfo(VL_HOST_IP_INFO * pCardIpInfo)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
#ifdef USE_DSG
    if(vlIsDsgOrIpduMode() && (0 != vlg_ucid))
    {
        if(vlGetIpuFlowId()&0x800000)
        {
            pCardIpInfo->ipAddressType = VL_XCHAN_IP_ADDRESS_TYPE_IPV4;
            memcpy(pCardIpInfo->aBytMacAddress, vlg_acDsgRcMacAddress,
                     sizeof(pCardIpInfo->aBytMacAddress));

            memcpy(pCardIpInfo->aBytIpAddress, vlg_acDsgRcIpAddress,
                     sizeof(pCardIpInfo->aBytIpAddress));

            pCardIpInfo->aBytSubnetMask[0] = 255;
            pCardIpInfo->aBytSubnetMask[1] = 255;
            pCardIpInfo->aBytSubnetMask[2] = 255;
            pCardIpInfo->aBytSubnetMask[3] = 0;
        }
    }
#endif // USE_DSG
}

/**************************************************
 *  APDU Functions to handle EXTENDED CHANNEL
 *************************************************/
void vlSetXChanSession(unsigned short sesnum)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    vlg_nXChanSession = sesnum;
}

ULONG vlGetXChanDsgSessionNumber()
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
#ifdef USE_DSG
    return vlGetDsgSessionNumber(vlg_nXChanSession);
#endif
}

ULONG vlGetXChanSession()
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    return vlg_nXChanSession;
}

ULONG vlXchanHostAssignFlowId()
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    static unsigned long vlg_nFlowId = VL_DSG_BASE_HOST_OTHER_FLOW_IDS;
    return vlg_nFlowId++;
}

void vlParseIPUconfParams(VL_BYTE_STREAM * pParseBuf, int len, VL_XCHAN_IPU_FLOW_CONF * pStruct)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    int bOldSaCard1 = 0;
    VL_ZERO_STRUCT(*pStruct);

    {
        unsigned char aIdentBuf1[] =
        {       
            0x01, 0x25, 0xdc, 0x57, 0x2b, 0x46, 0x01, 0x01, 0x00, 0x02, 0x04, 0x43, 0x41, 0x52,
            0x44, 0x03, 0x0d, 0x45, 0x43, 0x4d, 0x3a, 0x45, 0x53, 0x54, 0x42, 0x3a, 0x43, 0x41, 0x52, 0x44,
            0x04, 0x06
        };
    
        unsigned char aIdentBuf2[] =
        {       
            0x08, 0x06, 0x30, 0x30, 0x30, 0x32, 0x44, 0x45,
            0x33, 0x12, 0x53, 0x63, 0x69, 0x65, 0x6e, 0x74, 0x69, 0x66, 0x69, 0x63, 0x20, 0x41, 0x74, 0x6c,
            0x61, 0x6e, 0x74, 0x61, 0x36, 0x08, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x3c, 0x0d,
            0x4f, 0x70, 0x65, 0x6e, 0x43, 0x61, 0x62, 0x6c, 0x65, 0x20, 0x32, 0x2e, 0x30
        };
        
        if((0x5F == len) && (0x43 == ((VL_BYTE_STREAM_GET_CURRENT_BUF(pParseBuf))[0xF])) &&
            (0 == memcmp(&((VL_BYTE_STREAM_GET_CURRENT_BUF(pParseBuf))[VL_IP_ADDR_SIZE]), aIdentBuf1, sizeof(aIdentBuf1))) &&
            (0 == memcmp(&((VL_BYTE_STREAM_GET_CURRENT_BUF(pParseBuf))[VL_IP_ADDR_SIZE+sizeof(aIdentBuf1)+VL_MAC_ADDR_SIZE]), aIdentBuf2, sizeof(aIdentBuf2))))
        {
            bOldSaCard1 = 1;
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "vlParseIPUconfParams:: resID = 0x%08X: Using Workaround for Old SA Cable-Card\n", vlg_xchan_pod_res_id);
        }
        
        // Jan-19-2010: Force workaround if we do not have enough buffer to parse an IPV6 address
        if((vlg_xchan_pod_res_id >= VL_XCHAN_RESOURCE_ID_VER_6) &&
            VL_BYTE_STREAM_REMAINDER(pParseBuf) < VL_IPV6_ADDR_SIZE)
        {
            bOldSaCard1 = 1;
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "vlParseIPUconfParams:: resID = 0x%08X: Not enough buffer to parse an IPV6 address\n", vlg_xchan_pod_res_id);
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "vlParseIPUconfParams:: resID = 0x%08X: Using Workaround Xchan Resource ID Mismatch\n", vlg_xchan_pod_res_id);
        }
    }
    
    memset(vlg_acQpskRcIpAddress, 0, VL_IP_ADDR_SIZE);

    // parse ip address
    switch(vlg_xchan_pod_res_id)
    {
        case VL_XCHAN_RESOURCE_ID_VER_1:
        case VL_XCHAN_RESOURCE_ID_VER_2:
        case VL_XCHAN_RESOURCE_ID_VER_3:
        case VL_XCHAN_RESOURCE_ID_VER_4:
        case VL_XCHAN_RESOURCE_ID_VER_5:
        {
            vlParseBuffer(pParseBuf, pStruct->ipAddress    , VL_IP_ADDR_SIZE       , sizeof(pStruct->ipAddress  ));
        }
        break;

        case VL_XCHAN_RESOURCE_ID_VER_6:
        default:
        {
            if(bOldSaCard1)
            {
                // Dec-17-2008: workaround for old SA Cable-Cards : to be removed when we get newer cards
                vlParseBuffer(pParseBuf, pStruct->ipAddress, VL_IP_ADDR_SIZE       , sizeof(pStruct->ipAddress  ));
            }
            else
            {
                // discard upper 96 bits (12 bytes) as our STB does not support IPv6
                vlParseBuffer(pParseBuf, NULL, VL_IPV6_ADDR_SIZE - VL_IP_ADDR_SIZE , 0);
                vlParseBuffer(pParseBuf, pStruct->ipAddress, VL_IP_ADDR_SIZE       , sizeof(pStruct->ipAddress  ));
            }
        }
        break;
    }

    // parse other params
    switch(vlg_xchan_pod_res_id)
    {
        case VL_XCHAN_RESOURCE_ID_VER_1:
        {
            // nothing more to parse for this version
        }
        break;

        case VL_XCHAN_RESOURCE_ID_VER_2:
        case VL_XCHAN_RESOURCE_ID_VER_3:
        case VL_XCHAN_RESOURCE_ID_VER_4:
        case VL_XCHAN_RESOURCE_ID_VER_5:
        default:
        {
            pStruct->eFlowType          = (VL_XCHAN_IPU_FLOW_TYPE)vlParseByte(pParseBuf);
            pStruct->maxPDUSize         = vlParseShort(pParseBuf);
            pStruct->flags              = ((pStruct->maxPDUSize>>13)&0x7);
            pStruct->maxPDUSize        &= 0x1FFF;
            pStruct->nOptionSize        = vlParseByte(pParseBuf);
            vlParseBuffer(pParseBuf, pStruct->aOptions, pStruct->nOptionSize, sizeof(pStruct->aOptions));
        }
        break;
    }

    // parse link params
    switch(vlg_xchan_pod_res_id)
    {
        case VL_XCHAN_RESOURCE_ID_VER_1:
        case VL_XCHAN_RESOURCE_ID_VER_2:
        case VL_XCHAN_RESOURCE_ID_VER_3:
        case VL_XCHAN_RESOURCE_ID_VER_4:
        case VL_XCHAN_RESOURCE_ID_VER_5:
        {
            // nothing more to parse for these versions
        }
        break;

        case VL_XCHAN_RESOURCE_ID_VER_6:
        default:
        {
            if(0 == bOldSaCard1)
            {
                // discard upper 96 bits (12 bytes) as our STB does not support IPv6
                vlParseBuffer(pParseBuf, NULL, VL_IPV6_ADDR_SIZE - VL_IP_ADDR_SIZE , 0);
                vlParseBuffer(pParseBuf, pStruct->linkIpAddress, VL_IP_ADDR_SIZE       , sizeof(pStruct->linkIpAddress));
            }
        }
        break;
    }

    memcpy(vlg_acQpskRcIpAddress, pStruct->ipAddress, VL_IP_ADDR_SIZE);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "vlParseIPUconfParams:: Received IPU Confirmation, ip address = %d.%d.%d.%d, resID = 0x%08X\n",
                        pStruct->ipAddress[0], pStruct->ipAddress[1],
                        pStruct->ipAddress[2], pStruct->ipAddress[3],
                        vlg_xchan_pod_res_id);
}

void vlParseSOCKETconfParams(VL_BYTE_STREAM * pParseBuf, VL_XCHAN_SOCKET_FLOW_CONF * pStruct)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    VL_ZERO_STRUCT(*pStruct);

    // parse pdu size
    switch(vlg_xchan_pod_res_id)
    {
        case VL_XCHAN_RESOURCE_ID_VER_1:
        case VL_XCHAN_RESOURCE_ID_VER_2:
        case VL_XCHAN_RESOURCE_ID_VER_3:
        case VL_XCHAN_RESOURCE_ID_VER_4:
        case VL_XCHAN_RESOURCE_ID_VER_5:
        case VL_XCHAN_RESOURCE_ID_VER_6:
        default:
        {
            pStruct->maxPDUSize         = vlParseShort(pParseBuf);
        }
        break;
    }

    // parse ip address and options
    switch(vlg_xchan_pod_res_id)
    {
        case VL_XCHAN_RESOURCE_ID_VER_1:
        case VL_XCHAN_RESOURCE_ID_VER_2:
        case VL_XCHAN_RESOURCE_ID_VER_3:
        case VL_XCHAN_RESOURCE_ID_VER_4:
        case VL_XCHAN_RESOURCE_ID_VER_5:
        {
            // nothing more to parse for these versions
        }
        break;

        case VL_XCHAN_RESOURCE_ID_VER_6:
        default:
        {
            // discard upper 96 bits (12 bytes) as our STB does not support IPv6
            vlParseBuffer(pParseBuf, NULL, VL_IPV6_ADDR_SIZE - VL_IP_ADDR_SIZE , 0);
            vlParseBuffer(pParseBuf, pStruct->ipAddress, VL_IP_ADDR_SIZE       , sizeof(pStruct->ipAddress  ));
            // parse options
            pStruct->nOptionSize        = vlParseByte(pParseBuf);
            vlParseBuffer(pParseBuf, pStruct->aOptions, pStruct->nOptionSize, sizeof(pStruct->aOptions));
        }
        break;
    }
}

void vlXchanParseSocketFlowParams(VL_BYTE_STREAM * pParseBuf, VL_XCHAN_SOCKET_FLOW_PARAMS * pStruct)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    VL_ZERO_STRUCT(*pStruct);

    pStruct->eProtocol          = (VL_XCHAN_SOCKET_PROTOCOL_TYPE)vlParseByte(pParseBuf);
    pStruct->nLocalPortNum      = vlParseShort(pParseBuf);
    pStruct->nRemotePortNum     = vlParseShort(pParseBuf);
    pStruct->eRemoteAddressType = (VL_XCHAN_SOCKET_ADDRESS_TYPE)vlParseByte(pParseBuf);
    switch(pStruct->eRemoteAddressType)
    {
        case VL_XCHAN_SOCKET_ADDRESS_TYPE_NAME:
        {
            pStruct->nDnsNameLen = vlParseByte(pParseBuf);
            vlParseBuffer(pParseBuf, pStruct->dnsAddress   , pStruct->nDnsNameLen  , sizeof(pStruct->dnsAddress ));
        }
        break;

        case VL_XCHAN_SOCKET_ADDRESS_TYPE_IPV4:
        {
            vlParseBuffer(pParseBuf, pStruct->ipAddress    , VL_IP_ADDR_SIZE       , sizeof(pStruct->ipAddress  ));
        }
        break;

        case VL_XCHAN_SOCKET_ADDRESS_TYPE_IPV6:
        {
            vlParseBuffer(pParseBuf, pStruct->ipV6address  , VL_IPV6_ADDR_SIZE     , sizeof(pStruct->ipV6address));
        }
        break;
    }

    pStruct->connTimeout        = vlParseByte(pParseBuf);
}

int  APDU_NewFlowReq (USHORT sesnum, UCHAR * apkt, USHORT len)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    // bugfix: Aug-06-2007: Enabled operation of new flow request
    unsigned long tag = 0x9F8E00;
    return xchanSndApdu(sesnum, tag, len, apkt);
}

// Use this function only for requesting Mpeg flow from CARD confirmation
int  APDU_XCHAN2POD_newMpegFlowReq (USHORT sesnum, ULONG nPid)
{
		//IPDU Refactor Xchan registration. Currently the IPDU xchan hijacks the always ocurring mpeg flow request
		//			to register the IPDU flow. 
	 //	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: Force vlXchanRegisterNewFlow VL_XCHAN_FLOW_TYPE_IPDIRECT_UNICAST.\n", __FUNCTION__);
   	// vlXchanRegisterNewFlow(VL_XCHAN_FLOW_TYPE_IPDIRECT_UNICAST, VL_DSG_BASE_HOST_IPDIRECT_UNICAST_SEND_FLOW_ID);

		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    UCHAR aMessage[] = {0, (nPid>>8)&0xFF, (nPid>>0)&0xFF};

    ULONG nLength = 3;
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "APDU_XCHAN2POD_newMpegFlowReq:: Reqesting new MPEG flow for 0x%04X\n",  nPid);
    return vlDsgSndApdu2Pod(sesnum, 0x9F8E00, nLength, aMessage);
}

int  APDU_XCHAN2POD_SendIPUFlowReq (USHORT sesnum, unsigned char * pMacAddress,
                                   unsigned long nOptionLen, unsigned char * paOptions)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    UCHAR aMessage[320];
    VL_BYTE_STREAM_ARRAY_INST(aMessage, WriteBuf);
    ULONG nFinalLen = 0;
    nFinalLen += vlWriteByte(pWriteBuf, VL_XCHAN_FLOW_TYPE_IP_U);
    nFinalLen += vlWriteBuffer(pWriteBuf, pMacAddress, VL_MAC_ADDR_SIZE, sizeof(aMessage) - nFinalLen);
    nFinalLen += vlWriteByte(pWriteBuf, nOptionLen);
    nFinalLen += vlWriteBuffer(pWriteBuf, paOptions, nOptionLen, (sizeof(aMessage) - nFinalLen));

    memcpy(vlg_acQpskRcMacAddress, pMacAddress, VL_MAC_ADDR_SIZE);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "APDU_XCHAN2POD_SendIPUFlowReq:: Sending new IPU flow request\n");
    return xchanSndApdu(sesnum, 0x9F8E00, nFinalLen, aMessage);
}

int  APDU_XCHAN2POD_newIPMFlowReq (USHORT sesnum, unsigned char bytReserved, unsigned long nGroupId)
{
    UCHAR aMessage[] = {VL_XCHAN_FLOW_TYPE_IP_M,
        ((bytReserved&0xF)<<4)|((nGroupId>>24)&0xF),
          ((nGroupId>>16)&0xFF), ((nGroupId>>8)&0xFF), ((nGroupId>>0)&0xFF)};

    ULONG nLength = 5;
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "APDU_XCHAN2POD_newIPMFlowReq:: Reqesting new IP_M flow for group id 0x%04X\n",  nGroupId);
    return vlDsgSndApdu2Pod(sesnum, 0x9F8E00, nLength, aMessage);
}

// IPDU register flow
int  APDU_XCHAN2POD_newIPDUFlowReq(USHORT sesnum, unsigned char * pcReserved, long int returnQueue)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
   	vlXchanRegisterNewFlow(VL_XCHAN_FLOW_TYPE_IPDIRECT_UNICAST, VL_DSG_BASE_HOST_IPDIRECT_UNICAST_SEND_FLOW_ID);
    return 0;
}


int  APDU_XCHAN2POD_SendSOCKETFlowReq (USHORT sesnum, unsigned char protocol,
                                       unsigned short nLocalPortNum,
                                       unsigned short nRemotePort,
                                       unsigned long  nRemoteAddrType,
                                       unsigned long  nAddressLength,
                                       unsigned char * pAddress,
                                       unsigned char  nConnectionTimeout)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    UCHAR aMessage[320];
    VL_BYTE_STREAM_ARRAY_INST(aMessage, WriteBuf);
    ULONG nFinalLen = 0;
    nFinalLen += vlWriteByte (pWriteBuf, VL_XCHAN_FLOW_TYPE_SOCKET);
    nFinalLen += vlWriteByte (pWriteBuf, protocol);
    nFinalLen += vlWriteShort(pWriteBuf, nLocalPortNum);
    nFinalLen += vlWriteShort(pWriteBuf, nRemotePort);
    nFinalLen += vlWriteByte (pWriteBuf, nRemoteAddrType);
    switch(nRemoteAddrType)
    {
        case VL_XCHAN_SOCKET_ADDRESS_TYPE_NAME:
        {
            nFinalLen += vlWriteByte  (pWriteBuf, nAddressLength);
            nFinalLen += vlWriteBuffer(pWriteBuf, pAddress, nAddressLength, sizeof(aMessage) - nFinalLen);
        }
        break;

        case VL_XCHAN_SOCKET_ADDRESS_TYPE_IPV4:
        {
            nFinalLen += vlWriteBuffer(pWriteBuf, pAddress, VL_IP_ADDR_SIZE, sizeof(aMessage) - nFinalLen);
        }
        break;

        case VL_XCHAN_SOCKET_ADDRESS_TYPE_IPV6:
        {
            nFinalLen += vlWriteBuffer(pWriteBuf, pAddress, VL_IPV6_ADDR_SIZE, sizeof(aMessage) - nFinalLen);
        }
        break;
    }
    nFinalLen += vlWriteByte(pWriteBuf, nConnectionTimeout);

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "APDU_XCHAN2POD_SendSOCKETFlowReq:: Sending new SOCKET flow request\n");
    return xchanSndApdu(sesnum, 0x9F8E00, nFinalLen, aMessage);
}

int  APDU_XCHAN2POD_newMpegFlowCnf (USHORT sesnum, UCHAR status, UCHAR flowsRemaining, ULONG flowId, UCHAR servicetype)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    UCHAR aMessage[] = {status, flowsRemaining,
        (flowId>>16)&0xFF, (flowId>>8)&0xFF, (flowId>>0)&0xFF,
         servicetype};

    ULONG nLength = 2;
    if(0 == status)
    {
        nLength += 4;
        vlXchanRegisterNewFlow(VL_XCHAN_FLOW_TYPE_MPEG, flowId);
    }

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "APDU_XCHAN2POD_newMpegFlowCnf:: Sending new MPEG flow confirmation flowid = 0x%06X\n",  flowId);
    return vlDsgSndApdu2Pod(sesnum, 0x9F8E01, nLength, aMessage);
}

// Use this function only for DSG flow confirmation
int  APDU_XCHAN2POD_newDsgFlowCnf (USHORT sesnum, UCHAR status, UCHAR flowsRemaining, ULONG flowId, UCHAR servicetype)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    UCHAR aMessage[] = {status, flowsRemaining,
        (flowId>>16)&0xFF, (flowId>>8)&0xFF, (flowId>>0)&0xFF,
         servicetype};

    ULONG nLength = 2;
    if(0 == status)
    {
        nLength += 4;
        vlXchanRegisterNewFlow(VL_XCHAN_FLOW_TYPE_DSG, flowId);
    }

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "APDU_XCHAN2POD_newDsgFlowCnf:: Sending new DSG flow confirmation flowid = 0x%06X\n",  flowId);
    return vlDsgSndApdu2Pod(sesnum, 0x9F8E01, nLength, aMessage);
}

// Use this function only for IPDM flow confirmation
int  APDU_XCHAN2POD_newIpDmFlowCnf (USHORT sesnum, UCHAR status, UCHAR flowsRemaining, ULONG flowId, UCHAR servicetype)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    UCHAR aMessage[] = {status, flowsRemaining,
        (flowId>>16)&0xFF, (flowId>>8)&0xFF, (flowId>>0)&0xFF,
         servicetype};

    ULONG nLength = 2;
    if(0 == status)
    {
        nLength += 4;
        vlXchanRegisterNewFlow(VL_XCHAN_FLOW_TYPE_IPDM, flowId);
    }

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "APDU_XCHAN2POD_newIpDmFlowCnf:: Sending new IPDM flow confirmation flowid = 0x%06X\n",  flowId);
    return vlDsgSndApdu2Pod(sesnum, 0x9F8E01, nLength, aMessage);
}

void vlXchanIndicateLostDsgIPUFlow()
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    if(VL_DSG_BASE_HOST_FLOW_ID == (vlGetIpuFlowId()&VL_DSG_BASE_HOST_FLOW_ID))
    {
        APDU_XCHAN2POD_lostFlowInd(vlGetXChanSession(), vlGetIpuFlowId(), VL_XCHAN_LOST_FLOW_REASON_NET_BUSY_OR_DOWN);
    }
}

int  APDU_XCHAN2POD_lostFlowInd(USHORT sesnum, ULONG flowId, UCHAR reason)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    UCHAR aMessage[] = {
        (flowId>>16)&0xFF, (flowId>>8)&0xFF, (flowId>>0)&0xFF,
         reason};

    ULONG nLength = 4;

    //Jul-14-2009: Avoid un-registering DSG IPU flow to meet requirements for TC165.1 step 40
    //Oct-02-2009: Commented call to maintain init-reboot state for ipu dhcp // vlXchanCheckIpuCleanup(flowId);
    
    //unregister only during confirmation vlXchanUnregisterFlow(flowId);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "APDU_XCHAN2POD_lostFlowInd:: Sending lost flow indication for flowid = 0x%06X\n",  flowId);
    return vlDsgSndApdu2Pod(sesnum, 0x9F8E04, nLength, aMessage);
}

int  APDU_XCHAN2POD_deleteFlowReq (USHORT sesnum, ULONG flowId)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    UCHAR aMessage[] = {
        (flowId>>16)&0xFF, (flowId>>8)&0xFF, (flowId>>0)&0xFF};

    ULONG nLength = 3;

    //Oct-02-2009: Commented call to maintain init-reboot state for ipu dhcp // vlXchanCheckIpuCleanup(flowId);

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "APDU_XCHAN2POD_deleteFlowReq:: Sending delete flow request for flowid = 0x%06X\n",  flowId);
    return vlDsgSndApdu2Pod(sesnum, 0x9F8E02, nLength, aMessage);
}

int  APDU_XCHAN2POD_deleteFlowCnf (USHORT sesnum, UCHAR status, ULONG flowId)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    UCHAR aMessage[] = {
        (flowId>>16)&0xFF, (flowId>>8)&0xFF, (flowId>>0)&0xFF,
        status};

    ULONG nLength = 4;

    vlXchanUnregisterFlow(flowId);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "APDU_XCHAN2POD_deleteFlowCnf:: Sending delete confirmation flowid = 0x%06X\n",  flowId);
    return vlDsgSndApdu2Pod(sesnum, 0x9F8E03, nLength, aMessage);
}

int  APDU_XCHAN2POD_lostFlowCnf (USHORT sesnum, UCHAR reason, ULONG flowId)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    UCHAR aMessage[] = {
        (flowId>>16)&0xFF, (flowId>>8)&0xFF, (flowId>>0)&0xFF,
         reason};

    ULONG nLength = 4;

    vlXchanUnregisterFlow(flowId);

    return vlDsgSndApdu2Pod(sesnum, 0x9F8E05, nLength, aMessage);
}

int  APDU_XCHAN2POD_newIPUflowCnf(USHORT sesnum, UCHAR status, UCHAR flowsRemaining, ULONG flowId,
                                   unsigned char ip_addr[VL_IP_ADDR_SIZE], USHORT maxPduSize,
                                   VL_DSG_IP_UNICAST_FLOW_PARAMS * pStruct)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    UCHAR aMessage[320];
    VL_BYTE_STREAM_ARRAY_INST(aMessage, WriteBuf);
    UCHAR aConfirmation[] = {status, flowsRemaining,
        (flowId>>16)&0xFF, (flowId>>8)&0xFF, (flowId>>0)&0xFF, // flowid
         VL_XCHAN_FLOW_TYPE_IP_U};                             // service-type

    ULONG nLength = 2, nFinalLen = 0;
    if(0 == status)
    {
        nLength += 4;
        vlXchanRegisterNewFlow(VL_XCHAN_FLOW_TYPE_IP_U, flowId);
    }

    nFinalLen += vlWriteBuffer(pWriteBuf, aConfirmation, nLength, sizeof(aMessage)-nFinalLen);

    if(0 == status)
    {
        memcpy(vlg_acDsgRcIpAddress, ip_addr, VL_IP_ADDR_SIZE);
        // write IP address
        switch(vlg_xchan_pod_res_id)
        {
            case VL_XCHAN_RESOURCE_ID_VER_1:
            case VL_XCHAN_RESOURCE_ID_VER_2:
            case VL_XCHAN_RESOURCE_ID_VER_3:
            case VL_XCHAN_RESOURCE_ID_VER_4:
            case VL_XCHAN_RESOURCE_ID_VER_5:
            {
                nFinalLen += vlWriteBuffer(pWriteBuf, ip_addr, VL_IP_ADDR_SIZE, sizeof(aMessage)-nFinalLen);
            }
            break;

            case VL_XCHAN_RESOURCE_ID_VER_6:
            default:
            {
                // zero upper 96 bits (12 bytes) as our STB does not support IPv6
                nFinalLen += vlWriteBuffer(pWriteBuf, NULL, VL_IPV6_ADDR_SIZE - VL_IP_ADDR_SIZE, sizeof(aMessage)-nFinalLen);
                nFinalLen += vlWriteBuffer(pWriteBuf, ip_addr, VL_IP_ADDR_SIZE, sizeof(aMessage)-nFinalLen);
            }
            break;
        }
        // write options
        switch(vlg_xchan_pod_res_id)
        {
            case VL_XCHAN_RESOURCE_ID_VER_1:
            {
                // nothing more to write for this version
            }
            break;

            case VL_XCHAN_RESOURCE_ID_VER_2:
            case VL_XCHAN_RESOURCE_ID_VER_3:
            case VL_XCHAN_RESOURCE_ID_VER_4:
            case VL_XCHAN_RESOURCE_ID_VER_5:
            case VL_XCHAN_RESOURCE_ID_VER_6:
            default:
            {
                nFinalLen += vlWriteByte(pWriteBuf, 0);
                nFinalLen += vlWriteByte(pWriteBuf, (maxPduSize&0xFF00)>>8);
                nFinalLen += vlWriteByte(pWriteBuf, maxPduSize&0xFF);
                if(NULL != pStruct)
                {
                    nFinalLen += vlWriteByte(pWriteBuf, pStruct->nOptionBytes);
                    nFinalLen += vlWriteBuffer(pWriteBuf, pStruct->pOptionBytes, pStruct->nOptionBytes, (sizeof(aMessage) - nLength));
                }
                else
                {
                    nFinalLen += vlWriteByte(pWriteBuf, 0);
                }
            }
            break;
        }
        // write link params
        switch(vlg_xchan_pod_res_id)
        {
            case VL_XCHAN_RESOURCE_ID_VER_1:
            case VL_XCHAN_RESOURCE_ID_VER_2:
            case VL_XCHAN_RESOURCE_ID_VER_3:
            case VL_XCHAN_RESOURCE_ID_VER_4:
            case VL_XCHAN_RESOURCE_ID_VER_5:
            {

            }
            break;

            case VL_XCHAN_RESOURCE_ID_VER_6:
            default:
            {
                // zero upper 96 bits (12 bytes) as our STB does not support IPv6
                nFinalLen += vlWriteBuffer(pWriteBuf, NULL, VL_IPV6_ADDR_SIZE - VL_IP_ADDR_SIZE, sizeof(aMessage)-nFinalLen);
                nFinalLen += vlWriteBuffer(pWriteBuf, ip_addr, VL_IP_ADDR_SIZE, sizeof(aMessage)-nFinalLen);
            }
            break;
        }
    }

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "APDU_XCHAN2POD_newIPUflowCnf:: Sending new IPU flow confirmation status = %d, flowid = 0x%06X, len = %d, resID = 0x%08X\n",  status, flowId, nFinalLen, vlg_xchan_pod_res_id);
    return xchanSndApdu(sesnum, 0x9F8E01, nFinalLen, aMessage);
}

int  APDU_XCHAN2POD_send_IP_M_flowCnf(USHORT sesnum, UCHAR status, UCHAR flowsRemaining, ULONG flowId)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    UCHAR aConfirmation[] = {status, flowsRemaining,
        (flowId>>16)&0xFF, (flowId>>8)&0xFF, (flowId>>0)&0xFF, // flowid
         VL_XCHAN_FLOW_TYPE_IP_M};                             // service type

    ULONG nLength = 2, nFinalLen = 0;
    if(0 == status)
    {
        nLength += 4;
        vlXchanRegisterNewFlow(VL_XCHAN_FLOW_TYPE_IP_M, flowId);
    }

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "APDU_XCHAN2POD_send_IP_M_flowCnf:: Sending IP_M flow confirmation flowid = 0x%06X\n",  flowId);
    return xchanSndApdu(sesnum, 0x9F8E01, nLength, aConfirmation);
}

int  APDU_XCHAN2POD_send_SOCKET_flowCnf(USHORT sesnum, UCHAR status, UCHAR flowsRemaining, ULONG flowId,
                                        unsigned char link_ip_addr[VL_IPV6_ADDR_SIZE])
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    UCHAR bytReserved = 0;
    USHORT maxPDU = VL_XCHAN_SOCKET_MAX_PDU_SIZE;
    ULONG nLength = 2, nFinalLen = 0;
    UCHAR aMessage[320];
    VL_BYTE_STREAM_ARRAY_INST(aMessage, WriteBuf);
    UCHAR aConfirmation[] = {status, flowsRemaining,
        (flowId>>16)&0xFF, (flowId>>8)&0xFF, (flowId>>0)&0xFF, // flowid
         VL_XCHAN_FLOW_TYPE_SOCKET }; // Service Type

    nFinalLen += vlWriteBuffer(pWriteBuf, aConfirmation, sizeof(aConfirmation), sizeof(aMessage)-nFinalLen);

    if(0 == status)
    {
        // write pdu size
        switch(vlg_xchan_pod_res_id)
        {
            case VL_XCHAN_RESOURCE_ID_VER_1:
            case VL_XCHAN_RESOURCE_ID_VER_2:
            case VL_XCHAN_RESOURCE_ID_VER_3:
            case VL_XCHAN_RESOURCE_ID_VER_4:
            case VL_XCHAN_RESOURCE_ID_VER_5:
            case VL_XCHAN_RESOURCE_ID_VER_6:
            default:
            {
                nFinalLen += vlWriteByte(pWriteBuf, ((bytReserved&0x7)<<5)|((maxPDU>>8)&0x1F));
                nFinalLen += vlWriteByte(pWriteBuf, ((maxPDU>>0)&0xFF));
            }
            break;
        }
        // write IP address and options
        switch(vlg_xchan_pod_res_id)
        {
            case VL_XCHAN_RESOURCE_ID_VER_1:
            case VL_XCHAN_RESOURCE_ID_VER_2:
            case VL_XCHAN_RESOURCE_ID_VER_3:
            case VL_XCHAN_RESOURCE_ID_VER_4:
            case VL_XCHAN_RESOURCE_ID_VER_5:
            {
                // nothing more to write for this version
            }
            break;

            case VL_XCHAN_RESOURCE_ID_VER_6:
            default:
            {
                nFinalLen += vlWriteBuffer(pWriteBuf, link_ip_addr, VL_IPV6_ADDR_SIZE, sizeof(aMessage)-nFinalLen);
                // currently no options to report
                nFinalLen += vlWriteByte(pWriteBuf, 0);
            }
            break;
        }
    }

    if(0 == status)
    {
        nLength = nFinalLen;
        vlXchanRegisterNewFlow(VL_XCHAN_FLOW_TYPE_SOCKET, flowId);
    }

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "APDU_XCHAN2POD_send_SOCKET_flowCnf:: Sending SOCKET flow confirmation flowid = 0x%06X\n",  flowId);
    return xchanSndApdu(sesnum, 0x9F8E01, nLength, aMessage);
}

int vlXchanHandleDsgFlowCnf(unsigned long ulTag, unsigned long ulQualifier, VL_DSG_IP_UNICAST_FLOW_PARAMS * pStruct)
{
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "vlXchanHandleDsgFlowCnf:: Received ulTag = 0x%06X, ulQualifier = %d\n",  ulTag, ulQualifier);

    switch(ulTag)
    {
        case VL_XCHAN_POD2HOST_NEW_FLOW_CNF:
        {
            switch(ulQualifier)
            {
                case VL_XCHAN_FLOW_TYPE_IP_U:
                {
                    vlg_bDsgIpuFlowRequestPending = 0;
                    
                    if(0 != *(unsigned long*)(pStruct->ipAddress))
                    {
                        VL_IP_IF_PARAMS ipParams;
                        VL_ZERO_STRUCT(ipParams);
                        int sockFd = socket(AF_INET, SOCK_DGRAM, 0);
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "sockFd= %x, ipAddress = %d.%d.%d.%d\n", sockFd,
                                pStruct->ipAddress[0], pStruct->ipAddress[1],
                                pStruct->ipAddress[2], pStruct->ipAddress[3]);
                        ipParams.socket = sockFd;
                        memcpy(ipParams.macAddress, vlg_dsgIpUnicastFlowParams.macAddress, sizeof(ipParams.macAddress));
                        ipParams.flowid        = VL_DSG_BASE_HOST_IPU_FLOW_ID;
                        memcpy(ipParams.ipAddress, pStruct->ipAddress, sizeof(ipParams.ipAddress));
                        APDU_XCHAN2POD_newIPUflowCnf(vlg_dsgIpUnicastFlowParams.sesnum, VL_XCHAN_FLOW_RESULT_GRANTED, 0,
                                ipParams.flowid, ipParams.ipAddress, VL_XCHAN_IP_MAX_PDU_SIZE, pStruct);
                    }
                }
                break;
            }
        }
        break;
    }
    return 1;//Mamidi:042209

}

int vlSetupIPUflow( USHORT sesnum, unsigned char macAddr[VL_MAC_ADDR_SIZE], int nOptionBytes, unsigned char * pOptionBytes)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    UCHAR aOptions[320];
    int nOptionLen = 0;
    VL_BYTE_STREAM_ARRAY_INST(aOptions, WriteBuf);
    int result = 0;

    VL_ZERO_STRUCT(vlg_dsgIpUnicastFlowParams);
    vlg_dsgIpUnicastFlowParams.flowType = VL_XCHAN_FLOW_TYPE_IP_U;
    memcpy(vlg_dsgIpUnicastFlowParams.macAddress, macAddr, sizeof(vlg_dsgIpUnicastFlowParams.macAddress));
    vlg_dsgIpUnicastFlowParams.sesnum = sesnum;
    vlg_dsgIpUnicastFlowParams.flowid = 0;

    // subnet mask
    nOptionLen += vlWriteShort(pWriteBuf, 0x0104);
    nOptionLen += vlWriteLong(pWriteBuf, 0xFFFFFF00);
    // router
    nOptionLen += vlWriteShort(pWriteBuf, 0x0304);
    nOptionLen += vlWriteLong(pWriteBuf, 0xFFFFFFFF);
    // time to live
    nOptionLen += vlWriteShort(pWriteBuf, 0x1701);
    nOptionLen += vlWriteByte(pWriteBuf, 0x7F);
    // ip lease time
    nOptionLen += vlWriteShort(pWriteBuf, 0x3304);
    nOptionLen += vlWriteLong(pWriteBuf, 0xFFFFF);
    /* Jun-02-2011: Disabled: Server identifier 255.255.255.255 is invalid as per specification
    // server identifier
    nOptionLen += vlWriteShort(pWriteBuf, 0x3604);
    nOptionLen += vlWriteLong(pWriteBuf, 0xFFFFFFFF);
    */
    // other options requested by card
    nOptionLen += vlWriteBuffer(pWriteBuf, pOptionBytes, nOptionBytes, (sizeof(aOptions) - nOptionLen));

    vlg_dsgIpUnicastFlowParams.nOptionBytes = nOptionLen;
    vlg_dsgIpUnicastFlowParams.pOptionBytes = aOptions;

#ifdef USE_DSG
    CHALDsg_Set_Config(VL_CM_DSG_DEVICE_INSTANCE, VL_XCHAN_POD2HOST_NEW_FLOW_REQ, &vlg_dsgIpUnicastFlowParams);
#endif

    memcpy(vlg_acDsgRcMacAddress, macAddr, VL_MAC_ADDR_SIZE);
    return 1;
}

/*
 *  APDU in: 0x9F, 0x8E, 0x00, "new_flow_req". Extended Channel Support (1)
 */
int  APDU_HANDLE_NewFlowReq( USHORT sesnum, UCHAR *apkt, USHORT len )
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    //unsigned long tag = 0x9F8E00;
    //return xchanSndApdu( sesnum, tag, len, apkt );
    VL_BYTE_STREAM_INST(NewFlowRequest, ParseBuf, apkt, len);
    ULONG ulTag = vlParse3ByteLong(pParseBuf);
    ULONG nLength = vlParseCcApduLengthField(pParseBuf);
    UCHAR nServiceType = vlParseByte(pParseBuf);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "APDU_HANDLE_NewFlowReq '%s' called \n", vlg_aszFlowNames[SAFE_ARRAY_INDEX_UN(nServiceType, vlg_aszFlowNames)]);

    switch(nServiceType)
    {
        case VL_XCHAN_FLOW_TYPE_MPEG:
        {
            APDU_XCHAN2POD_newMpegFlowCnf(sesnum, VL_XCHAN_FLOW_RESULT_GRANTED, 0, vlXchanHostAssignFlowId(), nServiceType);
        }
        break;

        case VL_XCHAN_FLOW_TYPE_IP_U:
        {
            if(0 == vlGetIpuFlowId())
            {
                VL_IP_IF_PARAMS ipParams;
                unsigned char macAddress[VL_MAC_ADDR_SIZE];
                int nOptionBytes = 0;
                vlParseBuffer(pParseBuf, macAddress, sizeof(macAddress), sizeof(macAddress));
                nOptionBytes = vlParseByte(pParseBuf);
            #ifdef USE_DSG
                if(vlg_bDsgIpuFlowRequestPending)
                {
                    VL_DSG_STATUS dsgStatus;
                    unsigned char ipaddr[VL_IP_ADDR_SIZE] = {0,0,0,0};
                    APDU_XCHAN2POD_newIPUflowCnf(sesnum, VL_XCHAN_FLOW_RESULT_DENIED_NET_BUSY, 1,
                            0, ipaddr, VL_XCHAN_IP_MAX_PDU_SIZE, NULL);

                    vlDsgGetDocsisStatus(&dsgStatus);
                }
                else if(0 != vlg_bHostAcquiredIPAddress)
                {
                    vlSetupIPUflow(sesnum, macAddress, nOptionBytes, VL_BYTE_STREAM_GET_CURRENT_BUF(pParseBuf));
                    vlg_bDsgIpuFlowRequestPending = 1;
                }
                else
                {
                    VL_DSG_STATUS dsgStatus;
                    unsigned char ipaddr[VL_IP_ADDR_SIZE] = {0,0,0,0};
                    APDU_XCHAN2POD_newIPUflowCnf(sesnum, VL_XCHAN_FLOW_RESULT_DENIED_NO_NETWORK, 1,
                                                0, ipaddr, VL_XCHAN_IP_MAX_PDU_SIZE, NULL);

                    vlDsgGetDocsisStatus(&dsgStatus);
                    //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "APDU_HANDLE_NewFlowReq: %s\n", dsgStatus.szDsgStatus   );
                    //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "APDU_HANDLE_NewFlowReq: %s\n", dsgStatus.szDocsisStatus);
                }
            #endif // USE_DSG
            }
            else
            {
                // denied
                APDU_XCHAN2POD_newIPUflowCnf(sesnum, VL_XCHAN_FLOW_RESULT_DENIED_FLOWS_EXCEEDED,
                                                0, 0, NULL, VL_XCHAN_IP_MAX_PDU_SIZE,  NULL);
            }
        }
        break;

        case VL_XCHAN_FLOW_TYPE_IP_M:
        {
            unsigned char  groupId[VL_IP_ADDR_SIZE];
            vlParseBuffer(pParseBuf, groupId, VL_IP_ADDR_SIZE, sizeof(groupId));
            groupId[0] &= 0x0F;
            groupId[0] |= 0xE0;
            // Aug-31-2009: Deny the flow as it is optional.//vlXChanCreateIpMulticastFlow(sesnum, groupId);
            APDU_XCHAN2POD_newIPUflowCnf(sesnum, VL_XCHAN_FLOW_RESULT_DENIED_NO_SERVICE,
                                         0, 0, NULL, VL_XCHAN_IP_MAX_PDU_SIZE,  NULL);
        }
        break;

        case VL_XCHAN_FLOW_TYPE_DSG:
        {
#ifdef USE_DSG
            USHORT nDsgSession = vlGetDsgSessionNumber(sesnum);
            // Changed: MAY-02-2008: allow DSG flows to be overriden by card // if(FALSE == vlIsDsgOrIpduMode())
            {
                if(0 != vlGetDsgFlowId())vlXchanUnregisterFlow(vlGetDsgFlowId());
                ULONG nFlowId = VL_DSG_BASE_HOST_DSG_FLOW_ID;

                vlSetDsgFlowId(nFlowId);
                APDU_XCHAN2POD_newDsgFlowCnf(sesnum, VL_XCHAN_FLOW_RESULT_GRANTED, 0, nFlowId, nServiceType);
                //APDU_XCHAN2POD_newMpegFlowReq(sesnum, 0x1FFC);
                // Jan-29-2010: Patch for Core-Cross cable-card.
                if(!vlg_dsgInquiryPerformed)
                {
                    // Jan-29-2010: Patch for Core-Cross cable-card.
                    vlg_dsgInquiryPerformed = 1;
                    if(nDsgSession == sesnum)
                    {
                        APDU_DSG2POD_InquireDsgMode(nDsgSession, vlMapToXChanTag(sesnum, VL_XCHAN_HOST2POD_INQUIRE_DSG_MODE));
                    }
                    else
                    {
                        // Inquire using DSG session
                        APDU_DSG2POD_InquireDsgMode(nDsgSession, vlMapToXChanTag(sesnum, VL_HOST2POD_INQUIRE_DSG_MODE));
                    }
                }
            }
            /* Changed: MAY-02-2008: allow DSG flows to be overriden by card
            else
            {
                // denied
                APDU_XCHAN2POD_newDsgFlowCnf(sesnum, VL_XCHAN_FLOW_RESULT_DENIED_FLOWS_EXCEEDED, 0, 0, 0);
            }
            */
#endif
        }
        break;

        case VL_XCHAN_FLOW_TYPE_IPDM:
        {
#ifdef USE_DSG
            USHORT nDsgSession = vlGetDsgSessionNumber(sesnum);
            // Changed: MAY-02-2008: allow IPDM flows to be overriden by card // if(FALSE == vlIsDsgOrIpduMode())
            {
                if(0 != vlGetIpDmFlowId())vlXchanUnregisterFlow(vlGetIpDmFlowId());
                ULONG nFlowId = VL_DSG_BASE_HOST_IPDM_FLOW_ID;

                vlSetIpDmFlowId(nFlowId);
                APDU_XCHAN2POD_newIpDmFlowCnf(sesnum, VL_XCHAN_FLOW_RESULT_GRANTED, 0, nFlowId, nServiceType);
                // Jan-29-2010: Patch for Core-Cross cable-card.
                if(!vlg_dsgInquiryPerformed)
                {
                    // Jan-29-2010: Patch for Core-Cross cable-card.
                    vlg_dsgInquiryPerformed = 1;
                    if(nDsgSession == sesnum)
                    {
                        APDU_DSG2POD_InquireDsgMode(nDsgSession, vlMapToXChanTag(sesnum, VL_XCHAN_HOST2POD_INQUIRE_DSG_MODE));
                    }
                    else
                    {
                        // Inquire using DSG session
                        APDU_DSG2POD_InquireDsgMode(nDsgSession, vlMapToXChanTag(sesnum, VL_HOST2POD_INQUIRE_DSG_MODE));
                    }
                }
            }
            /* Changed: MAY-02-2008: allow IPDM flows to be overriden by card
            else
            {
                // denied
                APDU_XCHAN2POD_newIpDmFlowCnf(sesnum, VL_XCHAN_FLOW_RESULT_DENIED_FLOWS_EXCEEDED, 0, 0, 0);
            }
            */
#endif
        }
        break;

				// IPDU register Flow
        case VL_XCHAN_FLOW_TYPE_IPDIRECT_UNICAST:
        {
				    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: VL_XCHAN_FLOW_TYPE_IPDIRECT_UNICAST\n", __FUNCTION__);
				    vlXchanRegisterNewFlow(VL_XCHAN_FLOW_TYPE_IPDIRECT_UNICAST, VL_DSG_BASE_HOST_IPDIRECT_UNICAST_SEND_FLOW_ID);
            //APDU_XCHAN2POD_newMpegFlowCnf(sesnum, VL_XCHAN_FLOW_RESULT_GRANTED, 0, vlXchanHostAssignFlowId(), nServiceType);
        }
        break;

        case VL_XCHAN_FLOW_TYPE_SOCKET:
        {
#ifdef USE_DSG
            if(0 != vlXchanCheckIfHostAcquiredIpAddress())
            {
                VL_XCHAN_SOCKET_FLOW_PARAMS sockParams;
                vlXchanParseSocketFlowParams(pParseBuf, &sockParams);
                vlXChanCreateSocketFlow(sesnum, &sockParams);
            }
            else
#endif
            {
                VL_DSG_STATUS dsgStatus;
                APDU_XCHAN2POD_send_SOCKET_flowCnf(sesnum, VL_XCHAN_FLOW_RESULT_DENIED_NO_NETWORK, VL_MAX_SOCKET_FLOWS, 0,
                                        NULL);
#ifdef USE_DSG
                vlDsgGetDocsisStatus(&dsgStatus);
                //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "APDU_HANDLE_NewFlowReq: %s\n", dsgStatus.szDsgStatus   );
                //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "APDU_HANDLE_NewFlowReq: %s\n", dsgStatus.szDocsisStatus);
#endif
            }
        }
        break;

    }
    return EXIT_SUCCESS;//Mamidi:042209
}

/*
 *  APDU in: 0x9F, 0x8E, 0x01, "new_flow_cnf". Extended Channel Support (1)
 */
int  APDU_HANDLE_NewFlowCnf( USHORT sesnum, UCHAR *apkt, USHORT len )
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    unsigned char *ptr;
    unsigned char tag = 0;

    ptr = apkt;
    // Get / check the tag to make sure we got a good packet
    memcpy(&tag, ptr+2, sizeof(unsigned char));
    if (tag != 0x01)
    {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "ERROR:xchan: Non-matching tag in NewFlowCnf.\n");
            return EXIT_FAILURE;
    }

    // Sep-26-2007: removed commented placeholder

    // for DSG
    {
        VL_BYTE_STREAM_INST(NewFlowConfirmation, ParseBuf, apkt, len);
        ULONG ulTag = vlParse3ByteLong(pParseBuf);
        ULONG nLength = vlParseCcApduLengthField(pParseBuf);
        UCHAR nStatus = vlParseByte(pParseBuf);
        UCHAR nFlowsRemaining = vlParseByte(pParseBuf);

        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "APDU_HANDLE_NewFlowCnf called, nStatus = %d, nFlowsRemaining = %d\n", nStatus, nFlowsRemaining);
        
        if(0 == nStatus)
        {
            ULONG nFlowId = vlParse3ByteLong(pParseBuf);
            VL_XCHAN_FLOW_TYPE nServiceType = (VL_XCHAN_FLOW_TYPE)vlParseByte(pParseBuf);

            vlXchanRegisterNewFlow(nServiceType, nFlowId);

            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "APDU_HANDLE_NewFlowCnf '%s' called, nFlowId = %d\n", vlg_aszFlowNames[VL_SAFE_ARRAY_INDEX(nServiceType, vlg_aszFlowNames)], nFlowId);

            switch(nServiceType)
            {
                case VL_XCHAN_FLOW_TYPE_MPEG:
                {
#ifdef USE_DSG
                    vlSetXChanMPEGflowId(nFlowId);
#endif
                }
                break;

                case VL_XCHAN_FLOW_TYPE_IP_U:
                {
                    VL_XCHAN_IPU_FLOW_CONF ipuConfParams;
                    vlParseIPUconfParams(pParseBuf, VL_BYTE_STREAM_REMAINDER(pParseBuf), &ipuConfParams);
                }
                break;

                case VL_XCHAN_FLOW_TYPE_IP_M:
                {
                }
                break;

                case VL_XCHAN_FLOW_TYPE_SOCKET:
                {
                    VL_XCHAN_SOCKET_FLOW_CONF socketConfParams;
                    vlParseSOCKETconfParams(pParseBuf, &socketConfParams);
                }
                break;
            }
        }
    }

    sendFlowConfirm( ptr, len);
    return EXIT_SUCCESS;
}

int  APDU_HANDLE_DeleteFlowReq( USHORT sesnum, UCHAR *apkt, USHORT len )
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    //unsigned long tag = 0x9F8E02;
    //return xchanSndApdu( sesnum, tag, len, apkt );
    VL_BYTE_STREAM_INST(DeleteFlowRequest, ParseBuf, apkt, len);
    ULONG ulTag = vlParse3ByteLong(pParseBuf);
    ULONG nLength = vlParseCcApduLengthField(pParseBuf);
    ULONG nFlowId = vlParse3ByteLong(pParseBuf);

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "APDU_HANDLE_DeleteFlowReq called tag = 0x%06X, flowid: 0x%06X\n", ulTag, nFlowId);

    //Oct-02-2009: Commented call to maintain init-reboot state for ipu dhcp // vlXchanCheckIpuCleanup(nFlowId);
    
    if(VL_XCHAN_FLOW_TYPE_INVALID == vlXchanGetFlowType(nFlowId))
    {
        APDU_XCHAN2POD_deleteFlowCnf(sesnum, VL_XCHAN_DELETE_FLOW_RESULT_NO_FLOW_EXISTS, nFlowId);
    }
    else
    {
        APDU_XCHAN2POD_deleteFlowCnf(sesnum, VL_XCHAN_DELETE_FLOW_RESULT_GRANTED, nFlowId);
    }

    return EXIT_SUCCESS;
}

/*
 *  APDU in: 0x9F, 0x8E, 0x03, "delete_flow_cnf". Extended Channel Support (1)
 */
int  APDU_HANDLE_DeleteFlowCnf( USHORT sesnum, UCHAR *apkt, USHORT len )
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    unsigned char *ptr;
    unsigned char tag = 0;
        unsigned long nFlowId = ((apkt[4]<<16) | (apkt[5]<<8) | (apkt[6]<<0));

    ptr = apkt;
    // Get / check the tag to make sure we got a good packet
    memcpy(&tag, ptr+2, sizeof(unsigned char));
    if (tag != 0x03)
        {
        MDEBUG (DPM_ERROR, "ERROR:xchan: Non-matching tag in DeleteFlowCnf.\n");
        return EXIT_FAILURE;
    }

    vlXchanUnregisterFlow(nFlowId);

    deleteFlowConfirm( ptr );
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "APDU_HANDLE_DeleteFlowCnf called flowid: 0x%06X\n", nFlowId);
    return EXIT_SUCCESS;
}

/*
 *  APDU in: 0x9F, 0x8E, 0x04, "lost_flow_ind". Extended Channel Support (1)
 */
int  APDU_HANDLE_LostFlowInd( USHORT sesnum, UCHAR *apkt, USHORT len )
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    unsigned char *ptr;
    unsigned char tag = 0;
    unsigned long nFlowId = ((apkt[4]<<16) | (apkt[5]<<8) | (apkt[6]<<0));

    ptr = apkt;
    // Get / check the tag to make sure we got a good packet
    memcpy(&tag, ptr+2, sizeof(unsigned char));
    if (tag != 0x04)
    {
        MDEBUG (DPM_ERROR, "ERROR:xchan: Non-matching tag in DeleteFlowInd.\n");
        return EXIT_FAILURE;
    }

    //Jul-14-2009: Avoid un-registering DSG IPU flow to meet requirements for TC165.1 step 40
    //Oct-02-2009: Commented call to maintain init-reboot state for ipu dhcp // vlXchanCheckIpuCleanup(nFlowId);

    sendLostFlow( ptr );
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "APDU_HANDLE_LostFlowInd called flowid: 0x%06X\n", nFlowId);
    return EXIT_SUCCESS;
}

int  APDU_HANDLE_LostFlowCnf( USHORT sesnum, UCHAR *apkt, USHORT len )
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    //unsigned long tag = 0x9F8E05;
    //return xchanSndApdu( sesnum, tag, len, apkt );

    VL_BYTE_STREAM_INST(LostFlowConfirmation, ParseBuf, apkt, len);
    ULONG ulTag = vlParse3ByteLong(pParseBuf);
    ULONG nLength = vlParseCcApduLengthField(pParseBuf);
    ULONG nFlowId = vlParse3ByteLong(pParseBuf);
    ULONG nStatus = vlParseByte(pParseBuf);

    if(VL_XCHAN_LOST_FLOW_RESULT_ACKNOWLEDGED == nStatus)
    {
        vlXchanUnregisterFlow(nFlowId);
    }
    else
    {
        APDU_XCHAN2POD_deleteFlowReq(sesnum, nFlowId);
    }
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "APDU_HANDLE_LostFlowCnf called flowid: 0x%06X\n", nFlowId);
    return EXIT_SUCCESS;
}

/*
 *  APDU in: 0x9F, 0x8E, 0x06, "inq_DSG_mode". Extended Channel Support (2)
 */
int  APDU_HANDLE_InqDSGMode( USHORT sesnum, UCHAR *apkt, USHORT len )
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    // NOT SUPPORTED IN ONE WAY POD
    return EXIT_SUCCESS;
}

/*
 *  APDU in: 0x9F, 0x8E, 0x07, "set_DSG_mode". Extended Channel Support (2)
 */
int  APDU_HANDLE_SetDSGMode( USHORT sesnum, UCHAR *apkt, USHORT len )
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    // NOT SUPPORTED IN ONE WAY POD
    return EXIT_SUCCESS;
}

/*
 *  APDU in: 0x9F, 0x8E, 0x08, "DSG_packet_error". Extended Channel Support (2)
 */
int  APDU_HANDLE_DSGPacketError( USHORT sesnum, UCHAR *apkt, USHORT len )
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    // NOT SUPPORTED IN ONE WAY POD
    return EXIT_SUCCESS;
}

/*
 *  APDU in: 0x9F, 0x8E, 0x02, "delete_flow_req". Extended Channel Support (1)
 */
int  APDU_DeleteFlowReq( USHORT sesnum, UCHAR *apkt, USHORT len )
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    VL_BYTE_STREAM_INST(DeleteFlowRequest, ParseBuf, apkt, len);
    ULONG nFlowId = vlParse3ByteLong(pParseBuf);
    unsigned long tag = 0x9F8E02;

    if(VL_XCHAN_FLOW_TYPE_INVALID == vlXchanGetFlowType(nFlowId))
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "APDU_DeleteFlowReq:: NOT sending delete flow request for INVALID flowid = 0x%06X\n",  nFlowId);
        return 0;
    }

    return xchanSndApdu( sesnum, tag, len, apkt );
}

/*
 *  APDU in: 0x9F, 0x8E, 0x05, "lost_flow_cnf". Extended Channel Support (1)
 */
int  APDU_LostFlowCnf( USHORT sesnum, UCHAR *apkt, USHORT len )
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    unsigned long tag = 0x9F8E05;
    VL_BYTE_STREAM_INST(LostFlowConfirmation, ParseBuf, apkt, len);
    ULONG nFlowId = vlParse3ByteLong(pParseBuf);

    vlXchanUnregisterFlow(nFlowId);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "APDU_LostFlowCnf called flowid: 0x%06X\n", nFlowId);
    return xchanSndApdu( sesnum, tag, len, apkt );
}

static int xchanSndApdu( unsigned short sesnum, unsigned long tag, unsigned short dataLen, unsigned char *data )
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    unsigned short alen;
    unsigned char papdu[MAX_APDU_HEADER_BYTES + dataLen];

    if ( buildApdu(papdu, &alen, tag, dataLen, data) == NULL )
    {
        MDEBUG (DPM_ERROR, "ERROR:xchan: Unable to build apdu.\n");
        return EXIT_FAILURE;
    }

    AM_APDU_FROM_HOST(sesnum, papdu, alen);

    return EXIT_SUCCESS;
}

#ifdef __cplusplus
}
#endif

