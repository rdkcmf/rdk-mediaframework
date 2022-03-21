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



#include <pthread.h>
#include <global_event_map.h>
#include <global_queues.h>
#include "poddef.h"
#include <lcsm_log.h>
#include <memory.h>

#include "mrerrno.h"
#include "dsg-main.h"
#include "utils.h"
#include "link.h"
#include "transport.h"
#include "podhighapi.h"
#include "podlowapi.h"
#include "applitest.h"
#include "session.h"
#include "appmgr.h"
#include "vlpluginapp_haldsgclass.h"
#include "utilityMacros.h"
#include "bufParseUtilities.h"
#include "vlDsgParser.h"
#include "traceStack.h"
#include "vlDsgObjectPrinter.h"
extern int  vlg_nHeaderBytesToRemove;

#ifdef __cplusplus
extern "C" {
#endif

void vlPrintDsgEthHeader(int nLevel, VL_DSG_ETH_HEADER * pStruct)
{
    char szBuf[256];
    if(!rdk_dbg_enabled("LOG.RDK.POD", RDK_LOG_TRACE8)) return;
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_BEGIN, "VL_DSG_ETH_HEADER", "Begin");
    vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "", "dstMacAddress = %s", VL_PRINT_LONG_TO_STR(pStruct->dstMacAddress, VL_MAC_ADDR_SIZE, szBuf));
    vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "", "srcMacAddress = %s", VL_PRINT_LONG_TO_STR(pStruct->srcMacAddress, VL_MAC_ADDR_SIZE, szBuf));
    vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "", "eTypeCode = 0x%04x", pStruct->eTypeCode);
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_END, "VL_DSG_ETH_HEADER", "End");
}

void vlParseDsgEthHeader(VL_BYTE_STREAM * pParseBuf, VL_DSG_ETH_HEADER * pStruct)
{
    int i = 0;
    VL_ZERO_STRUCT(*pStruct);

    pStruct->dstMacAddress = vlParse6ByteLong(pParseBuf);
    pStruct->srcMacAddress = vlParse6ByteLong(pParseBuf);

    pStruct->eTypeCode = (VL_DSG_ETH_PROTOCOL)vlParseShort(pParseBuf);

    //vlPrintDsgEthHeader(0, pStruct);
}

void vlPrintDsgIpHeader(int nLevel, VL_DSG_IP_HEADER * pStruct)
{
    if(!rdk_dbg_enabled("LOG.RDK.POD", RDK_LOG_TRACE8)) return;
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_BEGIN, "VL_DSG_IP_HEADER", "Begin");

    vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "", "version          = 0x%02x", pStruct->version         );
    vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "", "headerLength     = 0x%02x", pStruct->headerLength    );
    vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "", "serviceType      = 0x%02x", pStruct->serviceType     );
    vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "", "nTotalLength     = %d"    , pStruct->nTotalLength    );
    vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "", "nIdentification  = 0x%04x", pStruct->nIdentification );
    vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "", "bDontFragment    = 0x%04x", pStruct->bDontFragment   );
    vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "", "bMoreFragments   = 0x%04x", pStruct->bMoreFragments  );
    vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "", "fragOffset       = 0x%04x", pStruct->fragOffset      );
    vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "", "timeToLive       = %d"    , pStruct->timeToLive      );
    vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "", "eProtocol        = 0x%02x", pStruct->eProtocol       );
    vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "", "headerCRC        = 0x%04x", pStruct->headerCRC       );

    vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "", "srcIpAddress     = 0x%08x", pStruct->srcIpAddress);
    vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "", "dstIpAddress     = 0x%08x", pStruct->dstIpAddress);

    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_END, "VL_DSG_IP_HEADER", "End");
}

unsigned char vlg_TcpOptions[1024];

void vlParseDsgIpHeader(VL_BYTE_STREAM * pParseBuf, VL_DSG_IP_HEADER * pStruct)
{
    int i = 0, nIHL = 0;
    VL_ZERO_STRUCT(*pStruct);

    pStruct->version            = vlParseByte(pParseBuf);
    pStruct->headerLength       = (pStruct->version&0xF);
    pStruct->version            = ((pStruct->version>>4)&0xF);
    if(4 != pStruct->version) return; // if version is not IPv4 do not parse any further
    pStruct->serviceType        = vlParseByte(pParseBuf);
    pStruct->nTotalLength       = vlParseShort(pParseBuf);
    pStruct->nIdentification    = vlParseShort(pParseBuf);
    pStruct->fragOffset         = vlParseShort(pParseBuf);
    pStruct->bDontFragment      = ((pStruct->fragOffset>>14)&0x1);
    pStruct->bMoreFragments     = ((pStruct->fragOffset>>13)&0x1);
    pStruct->fragOffset         = 8*((pStruct->fragOffset)&0x1FFF);
    pStruct->timeToLive         = vlParseByte(pParseBuf);
    pStruct->eProtocol          = (VL_DSG_IP_PROTOCOL)vlParseByte(pParseBuf);
    pStruct->headerCRC          = vlParseShort(pParseBuf);

    nIHL = ((pStruct->headerLength)-5)*4;
    if(nIHL > 0)
    {
        vlParseBuffer(pParseBuf, vlg_TcpOptions  , nIHL, sizeof(vlg_TcpOptions));
    }

    pStruct->srcIpAddress = vlParseLong(pParseBuf);
    pStruct->dstIpAddress = vlParseLong(pParseBuf);

    //vlPrintDsgIpHeader(0, pStruct);
}

void vlPrintDsgTcpHeader(int nLevel, VL_DSG_TCP_HEADER * pStruct)
{
    if(!rdk_dbg_enabled("LOG.RDK.POD", RDK_LOG_TRACE8)) return;
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_BEGIN, "VL_DSG_TCP_HEADER", "Begin");

    vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "", "nSrcPort         = %d"    , pStruct->nSrcPort       );
    vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "", "nDstPort         = %d"    , pStruct->nDstPort       );
    vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "", "nSeqNum          = 0x%08x", pStruct->nSeqNum        );
    vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "", "nAckNum          = 0x%08x", pStruct->nAckNum        );
    vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "", "nDataOffsetRsvd  = 0x%02x", pStruct->nDataOffsetRsvd);
    vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "", "nFlags           = 0x%02x", pStruct->nFlags         );
    vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "", "nWindowSize      = %d"    , pStruct->nWindowSize    );
    vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "", "nChecksum        = 0x%04x", pStruct->nChecksum      );
    vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "", "nUrgent          = 0x%04x", pStruct->nUrgent        );

    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_END, "VL_DSG_TCP_HEADER", "End");
}

void vlParseDsgTcpHeader(VL_BYTE_STREAM * pParseBuf, VL_DSG_TCP_HEADER * pStruct)
{
    int i = 0, nIHL = 0;
    VL_ZERO_STRUCT(*pStruct);

    pStruct->nSrcPort           = vlParseShort(pParseBuf);
    pStruct->nDstPort           = vlParseShort(pParseBuf);
    pStruct->nSeqNum            = vlParseLong(pParseBuf);
    pStruct->nAckNum            = vlParseLong(pParseBuf);
    pStruct->nDataOffsetRsvd    = vlParseByte(pParseBuf);
    pStruct->nFlags             = vlParseByte(pParseBuf);
    pStruct->nWindowSize        = vlParseShort(pParseBuf);
    pStruct->nChecksum          = vlParseShort(pParseBuf);
    pStruct->nUrgent            = vlParseShort(pParseBuf);

    nIHL = (((pStruct->nDataOffsetRsvd>>4)&0xF)-5)*4;
    if(nIHL > 0)
    {
        vlParseBuffer(pParseBuf, vlg_TcpOptions  , nIHL, sizeof(vlg_TcpOptions));
    }

    //vlPrintDsgTcpHeader(0, pStruct);
}

void vlPrintDsgUdpHeader(int nLevel, VL_DSG_UDP_HEADER * pStruct)
{
    if(!rdk_dbg_enabled("LOG.RDK.POD", RDK_LOG_TRACE8)) return;
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_BEGIN, "VL_DSG_UDP_HEADER", "Begin");

    vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "", "nSrcPort         = %d", pStruct->nSrcPort       );
    vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "", "nDstPort         = %d", pStruct->nDstPort       );
    vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "", "nLength          = %d", pStruct->nLength    );
    vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "", "nChecksum        = 0x%04x", pStruct->nChecksum      );

    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_END, "VL_DSG_UDP_HEADER", "End");
}

void vlParseDsgUdpHeader(VL_BYTE_STREAM * pParseBuf, VL_DSG_UDP_HEADER * pStruct)
{
    VL_ZERO_STRUCT(*pStruct);

    pStruct->nSrcPort           = vlParseShort(pParseBuf);
    pStruct->nDstPort           = vlParseShort(pParseBuf);
    pStruct->nLength            = vlParseShort(pParseBuf);
    pStruct->nChecksum          = vlParseShort(pParseBuf);

    //vlPrintDsgUdpHeader(0, pStruct);
}

void vlPrintDsgBtHeader(int nLevel, VL_DSG_BT_HEADER * pStruct)
{
    if(!rdk_dbg_enabled("LOG.RDK.POD", RDK_LOG_TRACE8)) return;
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_BEGIN, "VL_DSG_BT_HEADER", "Begin");

    vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "", "nHeaderStart     = 0x%02x", pStruct->nHeaderStart );
    vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "", "nVersion         = 0x%01x", pStruct->nVersion     );
    vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "", "lastSegment      = 0x%01x", pStruct->lastSegment  );
    vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "", "segmentNumber    = 0x%01x", pStruct->segmentNumber);
    vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "", "segmentId        = 0x%04x", pStruct->segmentId    );

    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_END, "VL_DSG_BT_HEADER", "End");
}

void vlParseDsgBtHeader(VL_BYTE_STREAM * pParseBuf, VL_DSG_BT_HEADER * pStruct)
{
    VL_ZERO_STRUCT(*pStruct);

    pStruct->nHeaderStart      = vlParseByte(pParseBuf);
    pStruct->segmentNumber     = vlParseByte(pParseBuf);
    pStruct->nVersion          = (((pStruct->segmentNumber)>>5)&0x7);
    pStruct->lastSegment       = (((pStruct->segmentNumber)>>4)&0x1);
    pStruct->segmentNumber     = (((pStruct->segmentNumber)>>0)&0xF);
    pStruct->segmentId         = vlParseShort(pParseBuf);

    //vlPrintDsgBtHeader(0, pStruct);
}

#ifdef __cplusplus
}
#endif

