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
#include <time.h>

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
#include "core_events.h"

#include <string.h>
#include "cVector.h"

#include "vlpluginapp_haldsgclass.h"
#include "utilityMacros.h"
#include "bufParseUtilities.h"
#include "vlDsgParser.h"
#include "traceStack.h"
#include "vlDsgObjectPrinter.h"
#include "vlDsgTunnelParser.h"
#include "vlDsgDispatchToClients.h"
#include "dsgResApi.h"
#include "xchanResApi.h"
#include <string.h>
#include "rmf_osal_mem.h"
#include "rmf_osal_sync.h"
#include "coreUtilityApi.h"

#if USE_SYSRES_MLT
#include "mlt_malloc.h"
#endif

#define __MTAG__ VL_CARD_MANAGER


#ifdef __cplusplus
extern "C" {
#endif

FILE * vlg_pFile = NULL;

extern int  vlg_nHeaderBytesToRemove;

extern int  vlg_bDsgBasicFilterValid;
extern int  vlg_bDsgConfigFilterValid;
extern int  vlg_bDsgDirectoryFilterValid;

extern VL_DSG_MODE          vlg_DsgMode     ;
extern VL_DSG_DIRECTORY     vlg_DsgDirectory;
extern VL_DSG_ADV_CONFIG    vlg_DsgAdvConfig;
extern unsigned long        vlg_ucid;

extern unsigned long vlg_nDsg_to_card_FlowId;

unsigned long vlg_xChan_MPEG_to_host_FlowId = 0;

char * vlg_aszBcastNames[] = {"zero", "SCTE_65", "SCTE_18", "three", "four", "XAIT   ",
                             "six", "seven", "eight", "nine", "ten"};

void  vlSetXChanMPEGflowId(unsigned long nFlowId)
{
    vlg_xChan_MPEG_to_host_FlowId = nFlowId;
}

int vlDsgEqualToBasicFilter(VL_DSG_ETH_HEADER * pEthHeader,
                            VL_MAC_ADDRESS    macAddress)
{
    return (macAddress == pEthHeader->dstMacAddress);
}

int vlDsgEqualToAdvFilter(VL_DSG_ETH_HEADER * pEthHeader,
                          VL_DSG_IP_HEADER  * pIpHeader,
                          VL_DSG_TCP_HEADER * pTcpHeader,
                          VL_DSG_UDP_HEADER * pUdpHeader,
                          VL_DSG_TUNNEL_FILTER * pFilter)
{
    unsigned short nDstPort = ((NULL != pTcpHeader)?(pTcpHeader->nDstPort):(pUdpHeader->nDstPort));
    int i = 0;

    // compare MAC address if specified
    if((0 != pFilter->dsgMacAddress)&&
        (pEthHeader->dstMacAddress != pFilter->dsgMacAddress)) return FALSE;

    // compare source IP address if specified
    if(0 != pFilter->srcIpAddress)
    {
        // if MASK was specified
        if(0 != pFilter->srcIpMask)
        {
            // compare source IP address with mask
            if((pIpHeader->srcIpAddress&pFilter->srcIpMask) !=
                    (pFilter->srcIpAddress&pFilter->srcIpMask  )) return FALSE;
        }
        else
        {
            // compare source IP address without mask
            if((pIpHeader->srcIpAddress) !=
                (pFilter->srcIpAddress)) return FALSE;
        }
    }

    // compare dest IP address if specified
    if(0 != pFilter->destIpAddress)
    {
        if((pIpHeader->dstIpAddress) !=
            (pFilter->destIpAddress)) return FALSE;
    }

    // compare ports if specified
    if(0 != pFilter->nPorts)
    {
        for(i = 0; i < pFilter->nPorts; i++)
        {
            // return pass if a match was found
            if(nDstPort == pFilter->aPorts[i]) return TRUE;
        }
        // return fail if no match was found
        return FALSE;
    }

    // passed all filters
    return TRUE;
}

int vlDsgEqualToAdsgFilter(VL_DSG_ETH_HEADER * pEthHeader,
                          VL_DSG_IP_HEADER  * pIpHeader,
                          VL_DSG_TCP_HEADER * pTcpHeader,
                          VL_DSG_UDP_HEADER * pUdpHeader,
                          VL_ADSG_FILTER * pFilter)
{
    unsigned short nDstPort = ((NULL != pUdpHeader)?(pUdpHeader->nDstPort):(pTcpHeader->nDstPort));
    int i = 0;

    // compare MAC address if specified
    if((0 != pFilter->macAddress)&&
        (pEthHeader->dstMacAddress != pFilter->macAddress))
    {
        //char szBuf1[32],szBuf2[32];
        //RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "macAddress %s:%s does not match\n", VL_PRINT_LONG_TO_STR(pFilter->macAddress, VL_MAC_ADDR_SIZE, szBuf1), VL_PRINT_LONG_TO_STR(pEthHeader->dstMacAddress, VL_MAC_ADDR_SIZE, szBuf2));
        // return fail
        return FALSE;
    }

    // compare source IP address if specified
    if(0 != pFilter->ipAddress)
    {
        // if MASK was specified
        if(0 != pFilter->ipMask)
        {
            // compare source IP address with mask
            if((pIpHeader->srcIpAddress&pFilter->ipMask) !=
                (pFilter->ipAddress&pFilter->ipMask  ))
            {
                //RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "ipAddress: 0x%08X ipMask: 0x%08X does not match\n", pIpHeader->srcIpAddress, pFilter->ipMask);
                // return fail
                return FALSE;
            }
        }
        else
        {
            // compare source IP address without mask
            if((pIpHeader->srcIpAddress) !=
                (pFilter->ipAddress))
            {
                //RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "ipAddress: 0x%08X does not match\n", pIpHeader->srcIpAddress);
                // return fail
                return FALSE;
            }
        }
    }

    // compare dest IP address if specified
    if(0 != pFilter->destIpAddress)
    {
        if((pIpHeader->dstIpAddress) !=
            (pFilter->destIpAddress)) return FALSE;
    }

    // compare ports if specified
    if((0 != pFilter->nPortStart) || (0 != pFilter->nPortEnd))
    {
        if((nDstPort >= pFilter->nPortStart) &&
           (nDstPort <= pFilter->nPortEnd  ))
        {
            // return pass
            return TRUE;
        }
        else
        {
            //RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "nDstPort: 0x%04X does not match\n", nDstPort);
            // return fail if no match was found
            return FALSE;
        }
    }

    // passed all filters
    return TRUE;
}

VL_DSG_DISPATCH_RESULT vlDsgDispatchToAmExtendedData(
                                  VL_DSG_DIR_TYPE        eEntryType,
                                  VL_DSG_CLIENT_ID_ENC_TYPE eClientType,
                                  unsigned long long     nClientId,
                                  int                    nDstPort,
                                  VL_DSG_IP_HEADER  *    pIpHeader,
                                  VL_DSG_UDP_HEADER *    pUdpHeader,
                                  unsigned char     *    pData,
                                  unsigned long          nLength
                                  )
{
    VL_DSG_DISPATCH_RESULT bExtChnlDispatchResult = VL_DSG_DISPATCH_RESULT_DROPPED;
    VL_DSG_DISPATCH_RESULT bDsgccDispatchResult   = VL_DSG_DISPATCH_RESULT_DROPPED;
    //vlMpeosDumpBuffer(RDK_LOG_TRACE9, "LOG.RDK.POD", pAccumulator->pSection, pAccumulator->nBytesAccumulated);
    if(VL_DSG_CLIENT_ID_ENC_TYPE_BCAST == eClientType)
    {
        /* Jul-16-2009: Payload based traffic suppression not allowed by spec.
           Jul-22-2009: impl. for the same removed.
        */

        // BCAST filtering completed
/*        RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgDispatchToAmExtendedData: Dispatching ( %s ) to Extended Channel, nPort = %d, Payload = 0x%02X\n",
                                    vlg_aszBcastNames[VL_SAFE_ARRAY_INDEX(nClientId, vlg_aszBcastNames)],
                                    nDstPort, *pData);
        RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgDispatchToAmExtendedData: Src Ip Address: %d.%d.%d.%d\n", ((pIpHeader->srcIpAddress>>24)&0xFF),
                                                            ((pIpHeader->srcIpAddress>>16)&0xFF),
                                                            ((pIpHeader->srcIpAddress>>8 )&0xFF),
                                                            ((pIpHeader->srcIpAddress>>0 )&0xFF));*/
        bExtChnlDispatchResult = VL_DSG_DISPATCH_RESULT_OK_TO_DISPATCH;
    }

    if(VL_DSG_DISPATCH_RESULT_OK_TO_DISPATCH == bExtChnlDispatchResult)
    {
        AM_EXTENDED_DATA(vlg_xChan_MPEG_to_host_FlowId, pData, nLength);
        bExtChnlDispatchResult = VL_DSG_DISPATCH_RESULT_DISPATCHED;
    }

    if(VL_DSG_CLIENT_ID_ENC_TYPE_BCAST == eClientType)
    {
        bDsgccDispatchResult = vlDsgDispatchBcastToDSGCC(eEntryType, eClientType, nClientId, pData, nLength);
    }
    else
    {
        bDsgccDispatchResult = vlDsgDispatchToDSGCC(eEntryType, eClientType, nClientId, pData, nLength);
    }

    // return any positive results
    if(VL_DSG_DISPATCH_RESULT_DISPATCHED == bExtChnlDispatchResult) return bExtChnlDispatchResult;
    if(VL_DSG_DISPATCH_RESULT_DISPATCHED == bDsgccDispatchResult  ) return bDsgccDispatchResult  ;
    // return negative result
    return VL_DSG_DISPATCH_RESULT_NO_MATCH_CLIENT;
}

VL_DSG_ACC_RESULT vlDsgAddToBcastAccumulator( int iAccumulator,
                                                    VL_DSG_BCAST_ACCUMULATOR* pAccumulator,
                                                    VL_DSG_IP_HEADER  *    pIpHeader,
                                                    VL_DSG_UDP_HEADER *    pUdpHeader,
                                                    VL_DSG_BT_HEADER  *    pBtHeader,
                                                    VL_DSG_DIR_TYPE        eEntryType,
                                                    VL_DSG_CLIENT_ID_ENC_TYPE eClientType,
                                                    unsigned long long     nClientId,
                                                    unsigned char     *    pData,
                                                    unsigned long          nLength)
{
    unsigned long nElapsedTime = (time(NULL)-pAccumulator->startTime);

    // check for reuse of accumulator
    if((!pAccumulator->bStartedAccumulation) || ( nElapsedTime > VL_DSG_BCAST_REASSEMBLY_TIMEOUT))
    {
        if((pAccumulator->bStartedAccumulation) && ( nElapsedTime > VL_DSG_BCAST_REASSEMBLY_TIMEOUT))
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "vlDsgAddToBcastAccumulator: Dropping BCAST packet ID: %lld as reassembly exceeded %d seconds\n"   , pAccumulator->eBcastId, nElapsedTime);
        }

        pAccumulator->eBcastId       =  nClientId;
        pAccumulator->srcIpAddress   =  pIpHeader->srcIpAddress;
        pAccumulator->dstIpAddress   =  pIpHeader->dstIpAddress;
        pAccumulator->nSrcPort       =  pUdpHeader->nSrcPort   ;
        pAccumulator->nDstPort       =  pUdpHeader->nDstPort   ;
        pAccumulator->segmentId      =  pBtHeader->segmentId   ;

        pAccumulator->bStartedAccumulation  = 1;
        pAccumulator->nBytesAccumulated     = 0;
        pAccumulator->iSegmentNumber        = -1;
        pAccumulator->startTime             = time(NULL);
    }
    else
    {
        // else check for match
        if(nClientId != pAccumulator->eBcastId)
        {
            /*RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgAddToBcastAccumulator: Acc %d, Mismatch BCAST ID: %lld:%lld \n", iAccumulator, nClientId, pAccumulator->eBcastId);
            RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgAddToBcastAccumulator: Src Ip Address: %d.%d.%d.%d\n", ((pIpHeader->srcIpAddress>>24)&0xFF),
                   ((pIpHeader->srcIpAddress>>16)&0xFF),
                     ((pIpHeader->srcIpAddress>>8 )&0xFF),
                       ((pIpHeader->srcIpAddress>>0 )&0xFF));
            */
            return VL_DSG_ACC_RESULT_NO_MATCH;
        }

        if( pIpHeader->srcIpAddress          != pAccumulator->srcIpAddress)
        {
            /*RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgAddToBcastAccumulator: Acc %d, Mismatch SrcIpAddress\n", iAccumulator);
            RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgAddToBcastAccumulator: Src Ip Address: %d.%d.%d.%d\n", ((pIpHeader->srcIpAddress>>24)&0xFF),
                   ((pIpHeader->srcIpAddress>>16)&0xFF),
                     ((pIpHeader->srcIpAddress>>8 )&0xFF),
                       ((pIpHeader->srcIpAddress>>0 )&0xFF));
            RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgAddToBcastAccumulator: Src Ip Address: %d.%d.%d.%d\n", ((pAccumulator->srcIpAddress>>24)&0xFF),
                   ((pAccumulator->srcIpAddress>>16)&0xFF),
                     ((pAccumulator->srcIpAddress>>8 )&0xFF),
                       ((pAccumulator->srcIpAddress>>0 )&0xFF));
            */
            return VL_DSG_ACC_RESULT_NO_MATCH;
        }

        if( pIpHeader->dstIpAddress          != pAccumulator->dstIpAddress)
        {
            /*RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgAddToBcastAccumulator: Acc %d, Mismatch DstIpAddress\n", iAccumulator);
            RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgAddToBcastAccumulator: Dst Ip Address: %d.%d.%d.%d\n", ((pIpHeader->dstIpAddress>>24)&0xFF),
                   ((pIpHeader->dstIpAddress>>16)&0xFF),
                     ((pIpHeader->dstIpAddress>>8 )&0xFF),
                       ((pIpHeader->dstIpAddress>>0 )&0xFF));
            RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgAddToBcastAccumulator: Dst Ip Address: %d.%d.%d.%d\n", ((pAccumulator->dstIpAddress>>24)&0xFF),
                   ((pAccumulator->dstIpAddress>>16)&0xFF),
                     ((pAccumulator->dstIpAddress>>8 )&0xFF),
                       ((pAccumulator->dstIpAddress>>0 )&0xFF));
            */
            return VL_DSG_ACC_RESULT_NO_MATCH;
        }

        if(pUdpHeader->nSrcPort              != pAccumulator->nSrcPort    )
        {
            //RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgAddToBcastAccumulator: Acc %d, Mismatch Src Port: %d:%d\n", iAccumulator, pUdpHeader->nSrcPort, pAccumulator->nSrcPort);
            return VL_DSG_ACC_RESULT_NO_MATCH;
        }

        if(pUdpHeader->nDstPort              != pAccumulator->nDstPort    )
        {
            //RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgAddToBcastAccumulator: Acc %d, Mismatch Dst Port: %d:%d\n", iAccumulator, pUdpHeader->nDstPort, pAccumulator->nDstPort);
            return VL_DSG_ACC_RESULT_NO_MATCH;
        }

        if( pBtHeader->segmentId             != pAccumulator->segmentId   )
        {
            //RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgAddToBcastAccumulator: Acc %d, Mismatch SegmentID: 0x%04X:0x%04X \n", iAccumulator, pBtHeader->segmentId, pAccumulator->segmentId);
            return VL_DSG_ACC_RESULT_NO_MATCH;
        }

    }

    // check for sequence error
    if(pAccumulator->iSegmentNumber+1 != (short)(pBtHeader->segmentNumber))
    {
        // fake an addition
        //RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgAddToBcastAccumulator: Segment Sequence Error (Expected = %d: Actual = %d) for BCast type:ID %d:%lld\n", pAccumulator->iSegmentNumber+1, pBtHeader->segmentNumber, eClientType,nClientId);
        return VL_DSG_ACC_RESULT_ADDED;
    }

    //RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgAddToBcastAccumulator: Segment Sequence (Expected = %d: Actual = %d) for BCast type:ID %d:%lld\n", pAccumulator->iSegmentNumber+1, pBtHeader->segmentNumber, eClientType,nClientId);

    // register sequence number
    pAccumulator->iSegmentNumber = pBtHeader->segmentNumber;

    // check for oversize accumulation
    if((pAccumulator->nBytesAccumulated + nLength) > 0x5000)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "vlDsgAddToBcastAccumulator: Dropping oversized table nBytesAccumulated = %d\n", (pAccumulator->nBytesAccumulated + nLength));
        pAccumulator->nBytesAccumulated = 0;
        return VL_DSG_ACC_RESULT_ERROR;
    }

    // check for capacity
    while((pAccumulator->nBytesAccumulated + nLength) > pAccumulator->nBytesCapacity)
    {
        unsigned char * pNewSection = NULL;
		rmf_osal_memAllocP(RMF_OSAL_MEM_POD, pAccumulator->nBytesCapacity+VL_DSG_SECTION_GROWTH_SIZE, (void **)&pNewSection);

        // check for non-initial re-allocation
        if(0 != pAccumulator->nBytesAccumulated)
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "vlDsgAddToBcastAccumulator: Acc %d, Reallocating buffer for BCAST ID: %lld:0x%04X \n", iAccumulator, nClientId, pBtHeader->segmentId);
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "vlDsgAddToBcastAccumulator: nBytesAccumulated = %d, new length = %d\n", pAccumulator->nBytesAccumulated, (pAccumulator->nBytesAccumulated + nLength));
        }

        // null check
        if(NULL != pNewSection)
        {
            // grow
            if(NULL != pAccumulator->pSection)
            {
                rmf_osal_memcpy(pNewSection, pAccumulator->pSection,
                        pAccumulator->nBytesAccumulated,
                        pAccumulator->nBytesCapacity+VL_DSG_SECTION_GROWTH_SIZE,
                        pAccumulator->nBytesAccumulated);

				rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pAccumulator->pSection);
            }

            pAccumulator->pSection = pNewSection;
            pAccumulator->nBytesCapacity += VL_DSG_SECTION_GROWTH_SIZE;
        }
        else
        {
            return VL_DSG_ACC_RESULT_ERROR;
        }
    }
/*
    RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgAddToBcastAccumulator: Src Ip Address: %d.%d.%d.%d\n", ((pIpHeader->srcIpAddress>>24)&0xFF),
           ((pIpHeader->srcIpAddress>>16)&0xFF),
             ((pIpHeader->srcIpAddress>>8 )&0xFF),
               ((pIpHeader->srcIpAddress>>0 )&0xFF));
    RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgAddToBcastAccumulator: Dest Ip Address: %d.%d.%d.%d\n", ((pIpHeader->dstIpAddress>>24)&0xFF),
           ((pIpHeader->dstIpAddress>>16)&0xFF),
             ((pIpHeader->dstIpAddress>>8 )&0xFF),
               ((pIpHeader->dstIpAddress>>0 )&0xFF));
    RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgAddToBcastAccumulator: Src Port: %d Dst Port:%d\n", pUdpHeader->nSrcPort, pUdpHeader->nDstPort);
*/
    // accumulate
    rmf_osal_memcpy(&(pAccumulator->pSection[pAccumulator->nBytesAccumulated]),
             pData,
             nLength,
             pAccumulator->nBytesCapacity-pAccumulator->nBytesAccumulated,
             nLength);

    pAccumulator->nBytesAccumulated += nLength;

    // check for overflow
    if(pAccumulator->nBytesAccumulated > pAccumulator->nBytesCapacity)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "vlDsgAddToBcastAccumulator: Buffer Overflow for BCAST ID: 0x%04X:0x%04X \n", (unsigned long)nClientId, pBtHeader->segmentId);
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "vlDsgAddToBcastAccumulator: nBytesAccumulated = %d, nBytesCapacity = %d\n", pAccumulator->nBytesAccumulated, pAccumulator->nBytesCapacity);
        return VL_DSG_ACC_RESULT_ERROR;
    }

    // check for dispatch
    if(pBtHeader->lastSegment)
    {
        //RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgAddToBcastAccumulator: Dispatched Acc %d size %d, BCAST ID: 0x%04X:0x%04X \n", iAccumulator, pAccumulator->nBytesAccumulated, (unsigned long)nClientId, pBtHeader->segmentId);
        //vlPrintDsgBtHeader(0, pBtHeader);
        vlDsgDispatchToAmExtendedData(eEntryType,
                                      eClientType,
                                      nClientId,
                                      pUdpHeader->nDstPort,
                                      pIpHeader,
                                      pUdpHeader,
                                      pAccumulator->pSection,
                                      pAccumulator->nBytesAccumulated);
        return VL_DSG_ACC_RESULT_DISPATCHED;
    }

    return VL_DSG_ACC_RESULT_ADDED;
}


void vlDsgDispatchFragmentedBtBcastToHost(VL_DSG_IP_HEADER  *    pIpHeader,
                                          VL_DSG_UDP_HEADER *    pUdpHeader,
                                          VL_DSG_BT_HEADER  *    pBtHeader,
                                          VL_DSG_DIR_TYPE        eEntryType,
                                          VL_DSG_CLIENT_ID_ENC_TYPE eClientType,
                                          unsigned long long     nClientId,
                                          unsigned char     *    pData,
                                          unsigned long          nLength)
{
    static cVector * pBcastDataVector = cVectorCreate(10, 10);

    if(NULL != pBcastDataVector)
    {
        int i = 0;
        for(i = 0; i < cVectorSize(pBcastDataVector); i++)
        {
            // try adding to any available accumulator
            VL_DSG_BCAST_ACCUMULATOR* pAccumulator =
                        (VL_DSG_BCAST_ACCUMULATOR*)cVectorGetElementAt(pBcastDataVector, i);
            VL_DSG_ACC_RESULT result =
                    vlDsgAddToBcastAccumulator( i, pAccumulator, pIpHeader, pUdpHeader, pBtHeader,
                                                eEntryType, eClientType, nClientId,
                                                pData, nLength);

            switch(result)
            {
                case VL_DSG_ACC_RESULT_ADDED:
                {
                    // accumulated
                    return;
                }
                break;

                case VL_DSG_ACC_RESULT_ERROR:
                case VL_DSG_ACC_RESULT_DROPPED:
                case VL_DSG_ACC_RESULT_DISPATCHED:
                {
                    // dispatched, dropped, or error
                    pAccumulator->bStartedAccumulation  = 0;
                    pAccumulator->nBytesAccumulated     = 0;
                    pAccumulator->iSegmentNumber        = -1;
                    pAccumulator->startTime             = time(NULL);
                    return;
                }
                break;
            }
        }

        // no usable accumulators found so far, so create and register new accumulator
        VL_DSG_BCAST_ACCUMULATOR* pAccumulator = NULL;
		rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(VL_DSG_BCAST_ACCUMULATOR),(void **)&pAccumulator);
	memset(pAccumulator,0,sizeof(*pAccumulator));
        pAccumulator->bStartedAccumulation  = 0;
        pAccumulator->nBytesAccumulated     = 0;
        pAccumulator->iSegmentNumber        = -1;
        pAccumulator->startTime             = time(NULL);
        cVectorAdd(pBcastDataVector, (cVectorElement)pAccumulator);
        vlDsgAddToBcastAccumulator( i, pAccumulator, pIpHeader, pUdpHeader, pBtHeader,
                                    eEntryType, eClientType, nClientId,
                                    pData, nLength);
    }
}

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
                         unsigned long          nPayloadLength)
{
    VL_BYTE_STREAM_INST(pPayload, ParseBuf, pPayload, nPayloadLength);
    VL_DSG_BT_HEADER   btHeader;

    {
        // drop any packets our stack is not interested in...
        //...(the if statements may be expanded in future)
        if( pPayload == NULL )  return;       
        if(0x11 == *pPayload) return;
        if((*pPayload >= 0x11) && (*pPayload <= 0x11)) return;
    }
    if(pIpHeader == NULL)
    {
        return;
    }
    // check for fragmentation
    if(vlIsIpPacketFragmented(pIpHeader))
    {
        // discard this and hope that the unfragmented version will be dispatched later.
        return;
    }

    if(13821 != nDstPort)
    {
        static int nPktCount = 0;
        nPktCount++;
        if(0 == nPktCount%1000)
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "vlDsgDispatchToHost: Dispatched %d DSG packets to HOST\n", nPktCount);
        }
    }

    switch(eClientType)
    {
        case VL_DSG_CLIENT_ID_ENC_TYPE_BCAST:
        {
            vlParseDsgBtHeader(pParseBuf, &btHeader);

            if((0xFF == btHeader.nHeaderStart) && (0x1 == btHeader.nVersion))
            {
                unsigned char * pBcastBuf = VL_BYTE_STREAM_GET_CURRENT_BUF(pParseBuf);
                unsigned long nBcastLen = nPayloadLength-4;

                //vlPrintDsgBtHeader(0, &btHeader);
                //vlMpeosDumpBuffer(RDK_LOG_TRACE9, "LOG.RDK.POD", pBcastBuf, 16);
                if((0 == btHeader.segmentNumber) && (1 == btHeader.lastSegment))
                {
                    // non-fragmented BCAST
                    //RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgDispatchToHost: Dispatching ( BCAST ID: 0x%04X ) to Extended Channel, nPort = %d\n", (unsigned long)nClientId, nDstPort);
                    //vlPrintDsgBtHeader(0, &btHeader);
                    vlDsgDispatchToAmExtendedData(  eEntryType,
                                                    eClientType,
                                                    nClientId,
                                                    nDstPort,
                                                    pIpHeader,
                                                    pUdpHeader,
                                                    pBcastBuf,
                                                    nBcastLen);
                }
                else
                {
                    // fragmented BCAST
                    if((NULL != pIpHeader) && (NULL != pUdpHeader))
                    {
                        static rmf_osal_Mutex lock;
                        static int i_lock = 0;
						if(i_lock == 0)
						{
							rmf_osal_mutexNew(&lock);	
							i_lock = 1;
						}
                        rmf_osal_mutexAcquire(lock);
                        {
                            vlDsgDispatchFragmentedBtBcastToHost(pIpHeader,
                                                                pUdpHeader,
                                                                &btHeader,
                                                                eEntryType,
                                                                eClientType,
                                                                nClientId,
                                                                pBcastBuf,
                                                                nBcastLen);
                        }
                        rmf_osal_mutexRelease(lock);
                    }
                }
            }
            else // ideally this should be dropped as per specification
            {
                //RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", " ( Client ID: 0x%04X )\n", (unsigned long)nClientId);
                //RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgDispatchToHost: Dispatching ( Client ID: 0x%04X ) to Extended Channel, nPort = %d, payload=0x%02X\n", (unsigned long)nClientId, nDstPort, *pPayload);
                // dispatch both to DSGCC and Table Manager
                vlDsgDispatchToAmExtendedData(  eEntryType,
                                                eClientType,
                                                nClientId,
                                                nDstPort,
                                                pIpHeader,
                                                pUdpHeader,
                                                pPayload,
                                                nPayloadLength);
            }
        }
        break;

        case VL_DSG_CLIENT_ID_ENC_TYPE_WKMA:
        case VL_DSG_CLIENT_ID_ENC_TYPE_CAS:
        case VL_DSG_CLIENT_ID_ENC_TYPE_APP:
        default:
        {
            //RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", " ( Client Type : 0x%04X)\n", eClientType);
            vlDsgDispatchToAmExtendedData(  eEntryType,
                                            eClientType,
                                            nClientId,
                                            nDstPort,
                                            pIpHeader,
                                            pUdpHeader,
                                            pPayload,
                                            nPayloadLength);
        }
    }
}

#define VL_DSG_TO_CARD_MPEG_HEADER_SIZE     2

unsigned char * vlDsgPrepareCardBuffer(unsigned char    * pData,
                                       unsigned long      nLength)
{
    static unsigned char * pCardBuffer = NULL;
    static int nBufCapacity = 0;

    if(nLength+VL_DSG_TO_CARD_MPEG_HEADER_SIZE > nBufCapacity)
    {
        unsigned char * pNewCardBuffer = NULL;
		rmf_osal_memAllocP(RMF_OSAL_MEM_POD, nLength+VL_DSG_TO_CARD_MPEG_HEADER_SIZE,(void **)&pNewCardBuffer);

        if(NULL != pNewCardBuffer)
        {
            void * pOldBuf = pCardBuffer; // take note of old buffer
            pCardBuffer = pNewCardBuffer;
            nBufCapacity = nLength+VL_DSG_TO_CARD_MPEG_HEADER_SIZE;
            if(NULL != pOldBuf) rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pOldBuf); // free old buffer
        }
        else
        {
            return NULL;
        }
    }

    if(NULL != pCardBuffer)
    {
        VL_BYTE_STREAM_INST(pCardBuffer, WriteBuf, pCardBuffer, nBufCapacity);

        vlWriteShort(pWriteBuf, nLength);
        vlWriteBuffer(pWriteBuf, pData, nLength, nBufCapacity-VL_DSG_TO_CARD_MPEG_HEADER_SIZE);
    }

    return pCardBuffer;
}

void vlDsgDispatchToCard(int                nDstPort,
                         unsigned char    * pData,
                         unsigned long      nLength,
                         unsigned char    * pPayload,
                         unsigned long      nPayloadLength)
{
    /*{
        // drop any packets the card is "probably" not interested in...
        //...(the if statements may be expanded in future)
        if(0x11 == *pPayload) return;
        if((*pPayload >= 0x11) && (*pPayload <= 0x11)) return;
    }*/

    if(13821 != nDstPort)
    {
        static int nPktCount = 0;
        nPktCount++;
        if(0 == nPktCount%2000)
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "vlDsgDispatchToCard: Dispatched %d DSG packets to CARD\n", nPktCount);
        }
    }

    if((0 != vlg_nDsg_to_card_FlowId) && (NULL != pData))
    {
        int nBytesRemoved = vlg_nHeaderBytesToRemove;
        if(nBytesRemoved < nLength)
        {
            static  rmf_osal_Mutex lock;
	     static int i_lock = 0;
		 if(i_lock == 0)
	 	{
			rmf_osal_mutexNew(&lock);
			i_lock = 1;
	 	}
            //RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgDispatchToCard: length = %d, payload = %d, removeBytes = %d\n", nLength, nPayloadLength, nBytesRemoved);

            if((NULL != pPayload) /*&& (0xC5 == *(pPayload))*/)
            {
                const char *mod = "LOG.RDK.POD"; rdk_LogLevel lvl = RDK_LOG_TRACE5;
                if(rdk_dbg_enabled(mod, lvl))
                {
                    //RDK_LOG(lvl, mod, "vlDsgDispatchToCard: dispatching STT table to Card\n");
                    RDK_LOG(lvl, mod, "vlDsgDispatchToCard: Dumping CA Tunnel Data from Cable Modem\n");
                    vlMpeosDumpBuffer(lvl, mod, pPayload, nPayloadLength);
                }
            }
            
            rmf_osal_mutexAcquire(lock); // begin lock
            {
                unsigned char * pCardBuf = vlDsgPrepareCardBuffer(pData+nBytesRemoved, nLength-nBytesRemoved);
                if(NULL != pCardBuf)
                {
                    POD_STACK_SEND_ExtCh_Data(pCardBuf,
                                            ((nLength-nBytesRemoved)+VL_DSG_TO_CARD_MPEG_HEADER_SIZE),
                                            vlg_nDsg_to_card_FlowId);
                                            
                    if((NULL != pPayload) && (0xC5 == *(pPayload)))
                    {
                        static int iSttCount = 0;
                        iSttCount++;
                        if(1 == iSttCount%100)
                        {
                            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: Delivered %d STT packets to CARD.\n", __FUNCTION__, iSttCount);
                        }
                    }
                }
            }
            rmf_osal_mutexRelease(lock); // end lock
        }
        else
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "vlDsgDispatchToCard: packet of %d bytes dropped\n", nLength);
        }
    }
}

void vlDsgDispatchToClients(unsigned char     * pData,
                            unsigned long       nLength,
                            unsigned char     * pPayload,
                            unsigned long       nPayloadLength,
                            VL_DSG_ETH_HEADER * pEthHeader,
                            VL_DSG_IP_HEADER  * pIpHeader,
                            VL_DSG_TCP_HEADER * pTcpHeader,
                            VL_DSG_UDP_HEADER * pUdpHeader)
{
    int i = 0;
    unsigned short nDstPort = ((NULL != pUdpHeader)?(pUdpHeader->nDstPort):(pTcpHeader->nDstPort));

    //RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "Dispatching %d, len = %d\n", nDstPort, nPayloadLength);


    //if(0x74 == *(pPayload))
    {
        /*if(NULL != pUdpHeader)
        {
            RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgDispatchToHost: nLength = %d, nPayloadLength = %d\n", nLength, nPayloadLength);
            vlPrintDsgUdpHeader(0, pUdpHeader);
        }
        if(NULL != pEthHeader)
        {
            RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgDispatchToClients: nLength = %d, nPayloadLength = %d\n", nLength, nPayloadLength);
            vlPrintDsgEthHeader(0, pEthHeader);
        }
        if(NULL != pIpHeader)
        {
            RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgDispatchToClients: nLength = %d, nPayloadLength = %d\n", nLength, nPayloadLength);
            vlPrintDsgIpHeader(0, pIpHeader);
        }*/
    }

    if((NULL != pPayload) && (0xC5 == *(pPayload)))
    {
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "vlDsgDispatchToClients: dispatching STT table to Clients\n");
        vlMpeosDumpBuffer(RDK_LOG_DEBUG, "LOG.RDK.POD", pPayload, nPayloadLength);
    }
    
    if(vlg_bDsgBasicFilterValid)
    {
        for(i = 0; i < vlg_DsgMode.nMacAddresses; i++)
        {
            if(vlDsgEqualToBasicFilter(pEthHeader, vlg_DsgMode.aMacAddresses[i]))
            {
                vlDsgUpdateDsgTrafficCounters(TRUE, i, nLength);
                
                vlDsgDispatchToCard(nDstPort,
                                    pData, nLength,
                                    pPayload, nPayloadLength);
            }

            // for basic mode DSGCC
            if((NULL != pIpHeader) && (NULL != pUdpHeader))
            {
                // check for fragmentation
                if(!vlIsIpPacketFragmented(pIpHeader))
                {
                    // dispatch only the unfragmented version
                    vlDsgDispatchBasicToDSGCC(vlg_DsgMode.aMacAddresses[i],
                                            pIpHeader->srcIpAddress,
                                            pIpHeader->dstIpAddress,
                                            pUdpHeader->nSrcPort,
                                            pUdpHeader->nDstPort,
                                            pPayload, nPayloadLength);
                }
            }
        }
    }
    else if(vlg_bDsgConfigFilterValid)
    {
        for(i = 0; i < vlg_DsgAdvConfig.nTunnelFilters; i++)
        {
            if(vlDsgEqualToAdvFilter(pEthHeader, pIpHeader, pTcpHeader, pUdpHeader,
                                    &(vlg_DsgAdvConfig.aTunnelFilters[i])))
            {
                vlDsgUpdateDsgTrafficCounters(TRUE, i, nLength);
                
                if(0 == vlg_DsgAdvConfig.aTunnelFilters[i].nAppId)
                {
                    vlDsgDispatchToCard(nDstPort,
                                        pData, nLength,
                                        pPayload, nPayloadLength);
                }
                else
                {
                    vlDsgDispatchToHost(pIpHeader,
                                        pTcpHeader,
                                        pUdpHeader,
                                        nDstPort,
                                        VL_DSG_DIR_TYPE_DIRECT_TERM_DSG_FLOW,
                                        VL_DSG_CLIENT_ID_ENC_TYPE_APP,
                                        vlg_DsgAdvConfig.aTunnelFilters[i].nAppId,
                                        pData, nLength,
                                        pPayload, nPayloadLength);
                }
            }
        }
    }
    else if(vlg_bDsgDirectoryFilterValid)
    {
        for(i = 0; i < vlg_DsgDirectory.nHostEntries; i++)
        {
            // Changed: Oct-08-2008: extended channel data is now received from card
            if(VL_DSG_DIR_TYPE_EXT_CHNL_MPEG_FLOW == vlg_DsgDirectory.aHostEntries[i].eEntryType)
            {
                // This entry is not a host terminating entry
                // the card will acquire the data from the card terminating filter and
                // forward it to the host via the extended channel MPEG flow
            }
            else if(vlDsgEqualToAdsgFilter(pEthHeader, pIpHeader, pTcpHeader, pUdpHeader,
                &(vlg_DsgDirectory.aHostEntries[i].adsgFilter)))
            {
                if(vlg_DsgDirectory.aHostEntries[i].clientId.bClientDisabled) // support for UCID based steering
                {
                    // If all is implemented correctly the following message should never be seen in the logs...
                    RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgDispatchToClients: Dropping packet for client type %d, client id %d due to UCID steering, bClientDisabled = %d\n",
                        vlg_DsgDirectory.aHostEntries[i].clientId.eEncodingType, (unsigned long)vlg_DsgDirectory.aHostEntries[i].clientId.nEncodedId, vlg_DsgDirectory.aHostEntries[i].clientId.bClientDisabled);
                }
                else
                {
                    vlDsgUpdateDsgTrafficCounters(FALSE, i, nLength);
                    
                    vlDsgDispatchToHost(pIpHeader,
                                        pTcpHeader,
                                        pUdpHeader,
                                        nDstPort,
                                        vlg_DsgDirectory.aHostEntries[i].eEntryType,
                                        vlg_DsgDirectory.aHostEntries[i].clientId.eEncodingType,
                                        vlg_DsgDirectory.aHostEntries[i].clientId.nEncodedId,
                                        pData, nLength,
                                        pPayload, nPayloadLength);
                }
            }
        }
        for(i = 0; i < vlg_DsgDirectory.nCardEntries; i++)
        {
            if(vlDsgEqualToAdsgFilter(pEthHeader, pIpHeader, pTcpHeader, pUdpHeader,
               &(vlg_DsgDirectory.aCardEntries[i].adsgFilter)))
            {
                vlDsgUpdateDsgTrafficCounters(TRUE, i, nLength);
                
                vlDsgDispatchToCard(nDstPort,
                                    pData, nLength,
                                    pPayload, nPayloadLength);
            }
        }
    }
}

void vlDsgTerminateDispatch()
{
    vlg_bDsgBasicFilterValid        = FALSE;
    vlg_bDsgConfigFilterValid       = FALSE;
    vlg_bDsgDirectoryFilterValid    = FALSE;

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "vlDsgTerminateDispatch: Dispatch Terminated.\n");
}


#ifdef __cplusplus
}
#endif

