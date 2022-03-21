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


 
//----------------------------------------------------------------------------
#ifndef __VL_DSG_DISPATCH_CLIENTS_H__
#define __VL_DSG_DISPATCH_CLIENTS_H__
//----------------------------------------------------------------------------

#include "dsg_types.h"
#include "dsgClientController.h"

#define VL_DSG_MAX_DSG_CLIENTS  0x20

typedef struct tag_VL_DSG_CLIENT
{
    unsigned long             nRegistrationId;
    VL_DSG_CLIENT_CBFUNC_t    pfClientCB;
    unsigned long             nAppData;
    VL_DSG_CLIENT_ID_ENC_TYPE eClientType;
    unsigned long long        nClientId;
    unsigned long             nTotalCallbacks;
    unsigned long             nTotalBytes;
    
}VL_DSG_CLIENT;

typedef struct tag_VL_DSG_BASIC_CLIENT
{
    unsigned long                   nRegistrationId;
    VL_DSG_BASIC_CLIENT_CBFUNC_t    pfClientCB;
    unsigned long                   nAppData;
    VL_MAC_ADDRESS                  dsgTunnelAddress;
    VL_IP_ADDRESS                   srcIpAddress;
    VL_IP_ADDRESS                   dstIpAddress;
    unsigned short                  nSrcPort;
    unsigned short                  nDstPort;
    unsigned long                   nTotalCallbacks;
    unsigned long                   nTotalBytes;
    
}VL_DSG_BASIC_CLIENT;

#ifdef __cplusplus
extern "C" {
#endif

VL_DSG_DISPATCH_RESULT vlDsgDispatchToDSGCC(
                          VL_DSG_DIR_TYPE           eEntryType,
                          VL_DSG_CLIENT_ID_ENC_TYPE eClientType,
                          unsigned long long        nClientId,
                          unsigned char *           pData,
                          unsigned long             nBytes);
    
VL_DSG_DISPATCH_RESULT vlDsgDispatchBcastToDSGCC(
                          VL_DSG_DIR_TYPE           eEntryType,
                          VL_DSG_CLIENT_ID_ENC_TYPE eClientType,
                          unsigned long long        nClientId,
                          unsigned char *           pData,
                          unsigned long             nBytes);
    
VL_DSG_DISPATCH_RESULT vlDsgDispatchBasicToDSGCC(
                                            VL_MAC_ADDRESS            dsgTunnelAddress,
                                            VL_IP_ADDRESS             srcIpAddress,
                                            VL_IP_ADDRESS             dstIpAddress,
                                            unsigned short            nSrcPort,
                                            unsigned short            nDstPort,
                                            unsigned char *           pData,
                                            unsigned long             nBytes);

void vlDsgDispatchToClients(unsigned char    * pData,
                            unsigned long      nLength,
                            unsigned char    * pPayload,
                            unsigned long      nPayloadLength,
                            VL_DSG_ETH_HEADER * pEthHeader,
                            VL_DSG_IP_HEADER  * pIpHeader,
                            VL_DSG_TCP_HEADER * pTcpHeader,
                            VL_DSG_UDP_HEADER * pUdpHeader);

void vlDsgDispatchToHost(VL_DSG_IP_HEADER  *    pIpHeader,
                         VL_DSG_TCP_HEADER *    pTcpHeader,
                         VL_DSG_UDP_HEADER *    pUdpHeader,
                         int                    nDstPort,
                         VL_DSG_DIR_TYPE        eEntryType,
                         VL_DSG_CLIENT_ID_ENC_TYPE eClientType,
                         unsigned long long     nClientId,
                         unsigned char     *    pData,
                         unsigned long          nLength,
                         unsigned char     *    pPayload,
                         unsigned long          nPayloadLength);

void vlDsgTerminateDispatch();
    
#ifdef __cplusplus
}
#endif
    
//----------------------------------------------------------------------------
#endif // __VL_DSG_DISPATCH_CLIENTS_H__
//----------------------------------------------------------------------------
