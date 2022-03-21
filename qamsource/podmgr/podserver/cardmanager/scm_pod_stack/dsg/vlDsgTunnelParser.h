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
#ifndef __VL_DSG_TUNNEL_PARSER_H__
#define __VL_DSG_TUNNEL_PARSER_H__
//----------------------------------------------------------------------------

#include "dsg_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void vlPrintDsgEthHeader(int nLevel, VL_DSG_ETH_HEADER * pStruct);
void vlParseDsgEthHeader(VL_BYTE_STREAM * pParseBuf, VL_DSG_ETH_HEADER * pStruct);

void vlPrintDsgIpHeader(int nLevel, VL_DSG_IP_HEADER * pStruct);
void vlParseDsgIpHeader(VL_BYTE_STREAM * pParseBuf, VL_DSG_IP_HEADER * pStruct);

void vlPrintDsgTcpHeader(int nLevel, VL_DSG_TCP_HEADER * pStruct);
void vlParseDsgTcpHeader(VL_BYTE_STREAM * pParseBuf, VL_DSG_TCP_HEADER * pStruct);

void vlPrintDsgUdpHeader(int nLevel, VL_DSG_UDP_HEADER * pStruct);
void vlParseDsgUdpHeader(VL_BYTE_STREAM * pParseBuf, VL_DSG_UDP_HEADER * pStruct);

void vlPrintDsgBtHeader(int nLevel, VL_DSG_BT_HEADER * pStruct);
void vlParseDsgBtHeader(VL_BYTE_STREAM * pParseBuf, VL_DSG_BT_HEADER * pStruct);

#ifdef __cplusplus
}
#endif

//----------------------------------------------------------------------------
#endif // __VL_DSG_TUNNEL_PARSER_H__
//----------------------------------------------------------------------------
