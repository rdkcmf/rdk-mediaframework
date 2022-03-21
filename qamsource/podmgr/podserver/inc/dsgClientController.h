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
#ifndef __VL_DSG_CLIENT_CONTROLLER_H__
#define __VL_DSG_CLIENT_CONTROLLER_H__
//----------------------------------------------------------------------------

#include "dsg_types.h"

#ifdef __cplusplus
extern "C" {
#endif
    
/**
 * Validates the specified DSG client
 * @param eClientType Type of the client scte 65/18/xait/wkma/cas/appid, scte and xait ids are 1 byte long, cas and appids are 2 bytes long, wkma ids are 6 bytes long.
 * @param nClientId Id of the client (data-type is identical to 64 bit java long).
 *
 * @return 0 for invalid, 1 for valid.
 */
int vlDsgIsValidClientId(VL_DSG_CLIENT_ID_ENC_TYPE eClientType, unsigned long long nClientId);

/**
 * Prototype for DSG client callback funtion.
 * @param nRegistrationId Id of the registrant.
 * @param nAppData Any registrant pointer or data to be supplied by the callback.
 * @param eClientType Type of the client scte 65/18/xait/wkma/cas/appid, scte and xait ids are 1 byte long, cas and appids are 2 bytes long, wkma ids are 6 bytes long.
 * @param nClientId Id of the client (data-type is identical to 64 bit java long).
 * @param nBytes number of bytes for this packet.
 * @param pData the pointer to the packet data, this buffer is owned by the DSGCC.
 *
 * @return 0 for processed, 1 for forwarded, -1 for dropped.
 */
typedef int (*VL_DSG_CLIENT_CBFUNC_t)(unsigned long             nRegistrationId,
                                      unsigned long             nAppData,
                                      VL_DSG_CLIENT_ID_ENC_TYPE eClientType,
                                      unsigned long long        nClientId,
                                      unsigned long             nBytes,
                                      unsigned char *           pData);

/**
 * Registers the specified client with the DSGCC.
 * @param pfClientCB The client callback function.
 * @param nAppData Any registrant pointer or data to be supplied by the callback.
 * @param eClientType Type of the client scte 65/18/xait/wkma/cas/appid, scte and xait ids are 1 byte long, cas and appids are 2 bytes long, wkma ids are 6 bytes long.
 * @param nClientId Id of the client, 0 forwards all traffic for the specified client type, (data-type is identical to 64 bit java long).
 *
 * @return Id of the registrant, 0 if too many clients.
 */
unsigned long vlDsgRegisterClient(VL_DSG_CLIENT_CBFUNC_t    pfClientCB,
                                  unsigned long             nAppData,
                                  VL_DSG_CLIENT_ID_ENC_TYPE eClientType,
                                  unsigned long long        nClientId);

/**
 * Prototype for DSG Basic client callback funtion.
 * @param nRegistrationId Id of the registrant.
 * @param nAppData Any registrant pointer or data to be supplied by the callback.
 * @param dsgTunnelAddress MAC address of the DSG "basic" tunnel.
 * @param srcIpAddress Ip Address of the source.
 * @param dstIpAddress Ip Address of the destination.
 * @param nSrcPort UDP Port of the source.
 * @param nDstPort UDP Port of the destination.
 * @param nBytes number of bytes for this packet.
 * @param pData the pointer to the packet data, this buffer is owned by the DSGCC.
 *
 * @return 0 for processed, 1 for forwarded, -1 for dropped.
 */
typedef int (*VL_DSG_BASIC_CLIENT_CBFUNC_t)
                                       (unsigned long             nRegistrationId,
                                        unsigned long             nAppData,
                                        VL_MAC_ADDRESS            dsgTunnelAddress,
                                        VL_IP_ADDRESS             srcIpAddress,
                                        VL_IP_ADDRESS             dstIpAddress,
                                        unsigned short            nSrcPort,
                                        unsigned short            nDstPort,
                                        unsigned long             nBytes,
                                        unsigned char *           pData);
    
/**
 * Registers the specified Basic client with the DSGCC.
 * @param pfClientCB The client callback function.
 * @param nAppData Any registrant pointer or data to be supplied by the callback.
 * @param dsgTunnelAddress MAC address of the DSG "basic" tunnel.
 * @param srcIpAddress Ip Address of the source.
 * @param dstIpAddress Ip Address of the destination.
 * @param nSrcPort UDP Port of the source.
 * @param nDstPort UDP Port of the destination.
 *
 * @return Id of the registrant, 0 if too many clients.
 */
unsigned long vlDsgRegisterBasicClient(
                                  VL_DSG_BASIC_CLIENT_CBFUNC_t  pfClientCB,
                                  unsigned long                 nAppData,
                                  VL_MAC_ADDRESS                dsgTunnelAddress,
                                  VL_IP_ADDRESS                 srcIpAddress,
                                  VL_IP_ADDRESS                 dstIpAddress,
                                  unsigned short                nSrcPort,
                                  unsigned short                nDstPort);
    
/**
 * Unregisters the specified client with the DSGCC.
 * @param nRegistrationId Id of the registrant.
 *
 * @return non-zero if the registrant was found.
 */
int vlDsgUnregisterClient(unsigned long                    nRegistrationId);
    
#ifdef __cplusplus
}
#endif
    
//----------------------------------------------------------------------------
#endif // __VL_DSG_CLIENT_CONTROLLER_H__
//----------------------------------------------------------------------------
