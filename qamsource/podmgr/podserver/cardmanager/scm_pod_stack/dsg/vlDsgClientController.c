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
#include "stdio.h"
#include "dsg_types.h"
#include "dsgClientController.h"
#include "vlDsgDispatchToClients.h"
#include "dsgResApi.h"
#include "xchanResApi.h"
#include "utilityMacros.h"
#include <memory.h>
#include "applitest.h"
#include "coreUtilityApi.h"
#include "vlDsgProxyService.h"
#include "rmf_osal_sync.h"

#ifdef __cplusplus
extern "C" {
#endif

VL_DSG_CLIENT vlg_aDsgClients[VL_DSG_MAX_DSG_CLIENTS];
VL_DSG_BASIC_CLIENT vlg_aDsgBasicClients[VL_DSG_MAX_DSG_CLIENTS];
VL_VCT_ID_COUNTER   vlg_aDsgVctIdCounter[VL_MAX_DSG_ENTRIES];
int           vlg_nVctIds = 0;
int           vlg_nRegistrantId = 1;
int           vlg_nAppClients = 0;
int           vlg_nClients = 0;
int           vlg_nBasicClients = 0;
extern int vlg_bHostAcquiredIPAddress;
extern int vlg_bHostAcquiredQpskIPAddress;
extern unsigned long vlg_nDsgVctId;
extern int vlg_bVctIdReceived;
extern int vlg_bDsgProxyEnableTunnelForwarding;
    

rmf_osal_Mutex   vlGetDsgCCMutex()
{
    static rmf_osal_Mutex mutex;
    static unsigned char val;	

    if(0 == val)
    {
        val = 1;
        rmf_osal_mutexNew(&mutex);
    }

    return mutex;
}

unsigned long vlDsgRegisterClient(VL_DSG_CLIENT_CBFUNC_t    pfClientCB,
                                  unsigned long             nAppData,
                                  VL_DSG_CLIENT_ID_ENC_TYPE eClientType,
                                  unsigned long long        nClientId)
{
    unsigned long nRegistrantId = 0;
    rmf_osal_Mutex mutex = vlGetDsgCCMutex();
    rmf_osal_mutexAcquire(mutex);
    {
        int i = 0;
        for(i = 0; i < VL_DSG_MAX_DSG_CLIENTS; i++)
        {
            if(0 == vlg_aDsgClients[i].nRegistrationId)
            {
                VL_ZERO_STRUCT(vlg_aDsgClients[i]);
                vlg_aDsgClients[i].pfClientCB       = pfClientCB;
                vlg_aDsgClients[i].nAppData         = nAppData;
                vlg_aDsgClients[i].eClientType      = eClientType;
                vlg_aDsgClients[i].nClientId        = nClientId;
                vlg_aDsgClients[i].nRegistrationId  = vlg_nRegistrantId++;
                nRegistrantId = vlg_aDsgClients[i].nRegistrationId;
                vlg_nClients++;
                if(VL_DSG_CLIENT_ID_ENC_TYPE_APP == eClientType)
                {
                    vlg_nAppClients++;
                }
                break;
            }
        }
    }
    rmf_osal_mutexRelease(mutex);

    if((nRegistrantId != 0) && (VL_DSG_CLIENT_ID_ENC_TYPE_APP == eClientType))
    {
        unsigned short nAppId = ((unsigned short)(nClientId));
        vlDsgSendAppTunnelRequestToCableCard(1, &nAppId);
    }

    return nRegistrantId;
}

unsigned long vlDsgRegisterBasicClient(
                                  VL_DSG_BASIC_CLIENT_CBFUNC_t  pfClientCB,
                                  unsigned long                 nAppData,
                                  VL_MAC_ADDRESS                dsgTunnelAddress,
                                  VL_IP_ADDRESS                 srcIpAddress,
                                  VL_IP_ADDRESS                 dstIpAddress,
                                  unsigned short                nSrcPort,
                                  unsigned short                nDstPort)
{
    unsigned long nRegistrantId = 0;
    rmf_osal_Mutex mutex = vlGetDsgCCMutex();
    rmf_osal_mutexAcquire(mutex);
    {
        int i = 0;
        for(i = 0; i < VL_DSG_MAX_DSG_CLIENTS; i++)
        {
            if(0 == vlg_aDsgBasicClients[i].nRegistrationId)
            {
                VL_ZERO_STRUCT(vlg_aDsgBasicClients[i]);
                vlg_aDsgBasicClients[i].pfClientCB       = pfClientCB;
                vlg_aDsgBasicClients[i].nAppData         = nAppData;
                vlg_aDsgBasicClients[i].dsgTunnelAddress = dsgTunnelAddress;
                vlg_aDsgBasicClients[i].srcIpAddress     = srcIpAddress;
                vlg_aDsgBasicClients[i].dstIpAddress     = dstIpAddress;
                vlg_aDsgBasicClients[i].nSrcPort         = nSrcPort;
                vlg_aDsgBasicClients[i].nDstPort         = nDstPort;
                vlg_aDsgBasicClients[i].nRegistrationId  = vlg_nRegistrantId++;
                nRegistrantId = vlg_aDsgBasicClients[i].nRegistrationId;
                vlg_nBasicClients++;
                break;
            }
        }
    }
    rmf_osal_mutexRelease(mutex);

    return nRegistrantId;
}
    
int vlDsgUnregisterClient(unsigned long                    nRegistrationId)
{
    int bFound = 0;
    rmf_osal_Mutex mutex = vlGetDsgCCMutex();
    rmf_osal_mutexAcquire(mutex);
    {
        int i = 0;
        for(i = 0; i < VL_DSG_MAX_DSG_CLIENTS; i++)
        {
            if(nRegistrationId == vlg_aDsgClients[i].nRegistrationId)
            {
                if(VL_DSG_CLIENT_ID_ENC_TYPE_APP == vlg_aDsgClients[i].eClientType)
                {
                    vlg_nAppClients--;
                }
                
                VL_ZERO_STRUCT(vlg_aDsgClients[i]);
                vlg_nClients--;
                bFound = 1;
            }
        }
        for(i = 0; i < VL_DSG_MAX_DSG_CLIENTS; i++)
        {
            if(nRegistrationId == vlg_aDsgBasicClients[i].nRegistrationId)
            {
                VL_ZERO_STRUCT(vlg_aDsgBasicClients[i]);
                vlg_nBasicClients--;
                bFound = 1;
            }
        }
    }
    rmf_osal_mutexRelease(mutex);

    return bFound;
}


    
VL_DSG_DISPATCH_RESULT vlDsgDispatchToDSGCC(
                                            VL_DSG_DIR_TYPE           eEntryType,
                                            VL_DSG_CLIENT_ID_ENC_TYPE eClientType,
                                            unsigned long long        nClientId,
                                            unsigned char *           pData,
                                            unsigned long             nBytes)
{
    VL_DSG_DISPATCH_RESULT eDispatch = VL_DSG_DISPATCH_RESULT_EXT_CHANNEL_CB;
    
    RDK_LOG(RDK_LOG_TRACE5, "LOG.RDK.POD", "vlDsgDispatchToDSGCC: Dispatching client type %d, client id %d \n",
                    eClientType, (unsigned long)nClientId);
    
    if(VL_DSG_CLIENT_ID_ENC_TYPE_APP == eClientType)
    {
        eDispatch = VL_DSG_DISPATCH_RESULT_DROPPED;
    }
    
    if(vlg_bDsgProxyEnableTunnelForwarding) vlDsgProxyServer_sendMcastData(eClientType, (unsigned long)nClientId, pData, nBytes);
    
    if(vlg_nClients > 0) // it is OK to test this outside the mutex
    {
        rmf_osal_Mutex mutex = vlGetDsgCCMutex();

        // as per specification these clients contain MPEG sections, so CRC check can be performed for them...
        if((vlg_nAppClients > 0) && (VL_DSG_CLIENT_ID_ENC_TYPE_APP == eClientType))
        {
            const int nMpegHeaderBytes      = 2;
            const int nSecLenBytes          = 3;
            const int nCrc32Bytes           = 4;
            const int nMinSecBytes          = nSecLenBytes + nCrc32Bytes;
            unsigned char * pSectionData    = pData + nMpegHeaderBytes;
            int       nSectionSize          = nBytes - nMpegHeaderBytes;
            unsigned long nSectionCrc32     = VL_VALUE_4_FROM_ARRAY(pSectionData+(nSectionSize-nCrc32Bytes));
            // calculate CRC
            unsigned long nCalculatedCrc32  = Crc32(CRC_ACCUM, pSectionData, nSectionSize);

            // check CRC before passing it up
            if(CRC_ACCUM != nCalculatedCrc32)
            {
                RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD", "DEBUG: vlDsgDispatchToDSGCC: CRC FAILED for App: 0x%llX, Pid: 0x%03X size %d bytes, section dropped...\r\n", nClientId, VL_VALUE_2_FROM_ARRAY(pData)&0x1FFF, nBytes);
                return VL_DSG_DISPATCH_RESULT_CRC_ERROR;
            }
        }

        rmf_osal_mutexAcquire(mutex);
        {
            int i = 0;
            for(i = 0; i < VL_DSG_MAX_DSG_CLIENTS; i++)
            {
                if(eClientType == vlg_aDsgClients[i].eClientType)
                {
                    if((nClientId  == vlg_aDsgClients[i].nClientId) ||
                       (0          == vlg_aDsgClients[i].nClientId))
                    {
                        if(NULL        != vlg_aDsgClients[i].pfClientCB)
                        {
                            vlg_aDsgClients[i].pfClientCB(  vlg_aDsgClients[i].nRegistrationId,
                                                            vlg_aDsgClients[i].nAppData,
                                                            eClientType, nClientId,
                                                            nBytes, pData);
                                
                            //RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "DEBUG: vlDsgDispatchToDSGCC: Dispatching data for client type %d, client id %d: size %d bytes\r\n", eClientType, (unsigned long)nClientId, nBytes);
                            vlMpeosDumpBuffer(RDK_LOG_TRACE8, "LOG.RDK.SI", pData, nBytes);
                            
                            eDispatch = VL_DSG_DISPATCH_RESULT_DISPATCHED;
                        }
                        
                        vlg_aDsgClients[i].nTotalCallbacks++;
                        vlg_aDsgClients[i].nTotalBytes    += nBytes;
                    }
                }
            }
        }
        rmf_osal_mutexRelease(mutex);
    }

    return eDispatch;
}

void vlDsgGetDsgVctStats(int nMaxStats, VL_VCT_ID_COUNTER aVctStats[VL_MAX_DSG_ENTRIES], int * pnAvailableStats)
{
    int iStat = 0;
    
    for(iStat = 0; (iStat < nMaxStats) && (iStat < VL_ARRAY_ITEM_COUNT(vlg_aDsgVctIdCounter)); iStat++)
    {
        if(iStat >= vlg_nVctIds) break;
        aVctStats[iStat] = vlg_aDsgVctIdCounter[iStat];
    }
    
    *pnAvailableStats = iStat;
}

int vlDsgCheckVctId(unsigned long nTableId, unsigned char * pData, int nSize)
{
    if(0xC4 != nTableId) return 1; // not a vct table
    else if(!vlg_bVctIdReceived) return 1; // vct id not received
    else
    {
        unsigned long nVctId = VL_VALUE_2_FROM_ARRAY(pData+5);
        
        // collect vct stats
        int i = 0;
        for(i = 0; (i < vlg_nVctIds) && (i < VL_ARRAY_ITEM_COUNT(vlg_aDsgVctIdCounter)); i++)
        {
            if(nVctId == vlg_aDsgVctIdCounter[i].nVctId)
            {
                vlg_aDsgVctIdCounter[i].nCount++;
                break;
            }
        }
        if((i == vlg_nVctIds) && ((vlg_nVctIds+1) < VL_ARRAY_ITEM_COUNT(vlg_aDsgVctIdCounter)))
        {
            vlg_nVctIds++;
            if(i < VL_ARRAY_ITEM_COUNT(vlg_aDsgVctIdCounter))
            {
                vlg_aDsgVctIdCounter[i].nCount = 1;
                vlg_aDsgVctIdCounter[i].nVctId = nVctId;
            }
        }
        
        if(nVctId != vlg_nDsgVctId)
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI", "DEBUG: vlDsgCheckVctId: VCT-ID check [expected = 0x%08X: received = 0x%08X] failed for table-id 0x%02X of size %d bytes... section dropped.\r\n", vlg_nDsgVctId, nVctId, nTableId, nSize);
            return 0; // vct id does not match
        }
    }
    
    return 1; // all other cases pass
}

VL_DSG_DISPATCH_RESULT vlDsgDispatchBcastToDSGCC(
                          VL_DSG_DIR_TYPE           eEntryType,
                          VL_DSG_CLIENT_ID_ENC_TYPE eClientType,
                          unsigned long long        nClientId,
                          unsigned char *           pData,
                          unsigned long             nBytes)
{
    VL_DSG_DISPATCH_RESULT eDispatch = VL_DSG_DISPATCH_RESULT_DROPPED;
    unsigned crc;
    unsigned char *pBuf = NULL;
    const int nSecLenBytes = 3;
    const int nCrc32Bytes  = 4;
    const int nMinSecBytes = nSecLenBytes + nCrc32Bytes;
    int nSection = 0;

    int nPacketSize = nBytes;
    
    // Changed: Apr-12-2008: Split Table into sections
    while(nPacketSize > nMinSecBytes)
    {
        unsigned long Size = 0;
        int offNextSection = 0;
        unsigned long nSectionCrc32    = 0;
        unsigned long nCalculatedCrc32 = 0;
        int nTableId = 0;

        // discard any 0xFF paddings
        // check for sections starting with 0xFF
        while((nPacketSize > 4) && (0xFFFFFFFF == VL_VALUE_4_FROM_ARRAY(pData)))
        {
            pData       += 4;
            nPacketSize -= 4;
        }
        
        while((nPacketSize > 1) && (0xFF == *pData))
        {
            pData       += 1;
            nPacketSize -= 1;
        }

        // is the section done
        if(nPacketSize < nMinSecBytes) continue;

        nTableId = *pData;
        Size = (nSecLenBytes+(VL_VALUE_2_FROM_ARRAY(pData+1)&0xFFF));
        offNextSection = Size;

        if(nSection >= 1)
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI", "vlDsgDispatchBcastToDSGCC: Sending Section %d of table-id 0x%02X, Size = %d, nPacketSize = %d\n", nSection, nTableId, Size, nPacketSize);
        }
        
        if(0x74 == nTableId)
        {
            if(!vlXchanCheckIfHostAcquiredIpAddress())
            {
                return eDispatch;
            }
        }
        
        /*if(0x74 == nTableId)
        {
            vlMpeosDumpBuffer(RDK_LOG_TRACE9, "LOG.RDK.POD", pData, Size);
        }*/
        
        if(Size < nMinSecBytes)
        {
            //MDEBUG ( DPM_ERROR, "PANIC: Size = %d is too small for table-id 0x%02X, discarding rest of %d bytes.\r\n", Size, nTableId, nPacketSize);
            break;
        }
        
        if(Size > 4096)
        {
            //MDEBUG ( DPM_ERROR, "PANIC: Size = %d is too big for table-id 0x%02X, discarding rest of %d bytes.\r\n", Size, nTableId, nPacketSize);
            break;
        }

        // check if the section is a CRC checkable section
        if(Size >= nMinSecBytes)
        {
            nSectionCrc32    = VL_VALUE_4_FROM_ARRAY(pData+(Size-nCrc32Bytes));
            /* CRC needs to pass for all tables // if(0 == nSectionCrc32)
            {
                // pretend that the CRC checked out
                nCalculatedCrc32 = CRC_ACCUM;
            }
            else*/
            {
                // calculate CRC
                nCalculatedCrc32 = Crc32(CRC_ACCUM, pData, Size);
            }

            // check CRC before passing it up
            if(CRC_ACCUM == nCalculatedCrc32)
            {
                if(vlDsgCheckVctId(nTableId, pData, Size))
                {
                    //if(0xC5 == nTableId)
                    {
                        const char *mod = "LOG.RDK.POD"; rdk_LogLevel lvl = RDK_LOG_TRACE5;
                        if(rdk_dbg_enabled(mod, lvl))
                        {
                            //RDK_LOG(lvl, mod, "vlDsgDispatchBcastToDSGCC: Dispatching STT to RI stack\n");
                            RDK_LOG(lvl, mod, "vlDsgDispatchBcastToDSGCC: Dumping BCAST Mpeg Flow Data from Card\n");
                            vlMpeosDumpBuffer(lvl, mod, pData, Size);
                        }
                    }
                    
                    eDispatch = vlDsgDispatchToDSGCC(eEntryType, eClientType, nClientId, pData, Size);
                                            
                    if(0xC5 == nTableId)
                    {
                        static int iSttCount = 0;
                        iSttCount++;
                        if(1 == iSttCount%100)
                        {
                            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: Delivered %d STT packets to HOST.\n", __FUNCTION__, iSttCount);
                        }
                    }
                }
                if(nTableId < 0xF)
                {
                    RDK_LOG(RDK_LOG_TRACE5, "LOG.RDK.POD", "vlDsgDispatchBcastToDSGCC: CRC passed for table-id 0x%02X of size %d bytes\r\n", nTableId, Size);
                }
            }
            else
            {
                RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.SI", "DEBUG: vlDsgDispatchBcastToDSGCC: CRC [0x%08X:0x%08X] failed for table-id 0x%02X of size %d bytes... section dropped.\r\n", nSectionCrc32, nCalculatedCrc32, nTableId, Size);
            }
        }

        pData       += offNextSection;
        nPacketSize -= offNextSection;
        nSection++;

        // discard any 0xFF paddings
        while((nPacketSize > 4) && (0xFFFFFFFF == VL_VALUE_4_FROM_ARRAY(pData)))
        {
            pData       += 4;
            nPacketSize -= 4;
        }
        
        while((nPacketSize > 1) && (0xFF == *pData))
        {
            pData       += 1;
            nPacketSize -= 1;
        }
    }
    return eDispatch;
}


int vlIsEqualBasicClient(VL_DSG_BASIC_CLIENT* pBasicClient,
                         VL_MAC_ADDRESS       dsgTunnelAddress,
                         VL_IP_ADDRESS        srcIpAddress,
                         VL_IP_ADDRESS        dstIpAddress,
                         unsigned short       nSrcPort,
                         unsigned short       nDstPort)
{
    if(dsgTunnelAddress != pBasicClient->dsgTunnelAddress) return 0;
    if((0 != pBasicClient->srcIpAddress     ) && (srcIpAddress     != pBasicClient->srcIpAddress    )) return 0;
    if((0 != pBasicClient->dstIpAddress     ) && (dstIpAddress     != pBasicClient->dstIpAddress    )) return 0;
    if((0 != pBasicClient->nSrcPort         ) && (nSrcPort         != pBasicClient->nSrcPort        )) return 0;
    if((0 != pBasicClient->nDstPort         ) && (nDstPort         != pBasicClient->nDstPort        )) return 0;
    return 1;
}

VL_DSG_DISPATCH_RESULT vlDsgDispatchBasicToDSGCC(
                                            VL_MAC_ADDRESS            dsgTunnelAddress,
                                            VL_IP_ADDRESS             srcIpAddress,
                                            VL_IP_ADDRESS             dstIpAddress,
                                            unsigned short            nSrcPort,
                                            unsigned short            nDstPort,
                                            unsigned char *           pData,
                                            unsigned long             nBytes)
{
    VL_DSG_DISPATCH_RESULT eDispatch = VL_DSG_DISPATCH_RESULT_DROPPED;

    RDK_LOG(RDK_LOG_TRACE5, "LOG.RDK.POD", "vlDsgDispatchBasicToDSGCC: Dispatching DSG Basic Tunnel 0x%llx\n",
                    dsgTunnelAddress);

    if(vlg_nBasicClients > 0) // it is OK to test this outside the mutex
    {
        rmf_osal_Mutex mutex = vlGetDsgCCMutex();
        rmf_osal_mutexAcquire(mutex);
        {
            int i = 0;
            for(i = 0; i < VL_DSG_MAX_DSG_CLIENTS; i++)
            {
                if(vlIsEqualBasicClient(&(vlg_aDsgBasicClients[i]),
                                        dsgTunnelAddress,
                                        srcIpAddress,
                                        dstIpAddress,
                                        nSrcPort,
                                        nDstPort))
                {
                    if(NULL        != vlg_aDsgBasicClients[i].pfClientCB)
                    {
                        vlg_aDsgBasicClients[i].pfClientCB(  vlg_aDsgBasicClients[i].nRegistrationId,
                                                        vlg_aDsgBasicClients[i].nAppData,
                                                        dsgTunnelAddress,
                                                        srcIpAddress, dstIpAddress,
                                                        nSrcPort, nDstPort,
                                                        nBytes, pData);

                        eDispatch = VL_DSG_DISPATCH_RESULT_DISPATCHED;
                    }

                    vlg_aDsgBasicClients[i].nTotalCallbacks++;
                    vlg_aDsgBasicClients[i].nTotalBytes    += nBytes;
                }
            }
        }
        rmf_osal_mutexRelease(mutex);
    }

    return eDispatch;
}

#ifdef __cplusplus
}
#endif
    
//----------------------------------------------------------------------------
