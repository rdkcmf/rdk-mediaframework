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

#ifdef __cplusplus
extern "C" {
#endif
    
void vlPrintDsgInit()
{
    static int is = 0;
    is++;
    /* Fake func to force link-in of vlDsgParser.o to dsg.dsg from libDsg.a */
}

void vlPrintDsgMode         (int nLevel, VL_DSG_MODE            * pStruct      )
{
    int i = 0;
    char szBuf[256];
    
    char* dsg_modes[] =
    {
        "SCTE-55",
        "Basic DSG two-way",
        "Basic DSG one-way",
        "Advanced DSG two-way",
        "Advanced DSG one-way",
        "IP Direct Multicast",
        "IP Direct Unicast",
        "Unknown",
    };
    
    if(!rdk_dbg_enabled("LOG.RDK.POD", RDK_LOG_INFO)) return;
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_BEGIN, "VL_DSG_MODE", "Begin");
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_MODE", "eOperationalMode = 0x%02x (%s)", pStruct->eOperationalMode,
                dsg_modes[VL_SAFE_ARRAY_INDEX(pStruct->eOperationalMode, dsg_modes)]);
    
    if((VL_DSG_OPERATIONAL_MODE_BASIC_TWO_WAY == pStruct->eOperationalMode) ||
        (VL_DSG_OPERATIONAL_MODE_BASIC_ONE_WAY == pStruct->eOperationalMode))
    {
        vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_MODE", "nMacAddresses    = %d", pStruct->nMacAddresses);
        for(i = 0; i < pStruct->nMacAddresses; i++)
        {
            int ixSafe = VL_SAFE_ARRAY_INDEX(i, pStruct->aMacAddresses);
            vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_MODE", "aMacAddresses[%d] = %s", ixSafe, VL_PRINT_LONG_TO_STR(pStruct->aMacAddresses[ixSafe], VL_MAC_ADDR_SIZE, szBuf));
        }
    }
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_MODE", "nRemoveHeaderBytes  = %d", pStruct->nRemoveHeaderBytes   );
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_END, "VL_DSG_MODE", "End");
}

void vlPrintDsgAdsgFilter   (int nLevel, VL_ADSG_FILTER         * pStruct   )
{
    char szBuf[256];
    if(!rdk_dbg_enabled("LOG.RDK.POD", RDK_LOG_INFO)) return;
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_BEGIN, "VL_ADSG_FILTER", "Begin");
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_ADSG_FILTER", "nTunneId        = 0x%02x", pStruct->nTunneId   );
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_ADSG_FILTER", "nTunnelPriority = 0x%02x", pStruct->nTunnelPriority   );
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_ADSG_FILTER", "macAddress      = %s", VL_PRINT_LONG_TO_STR(pStruct->macAddress, VL_MAC_ADDR_SIZE, szBuf));
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_ADSG_FILTER", "ipAddress       = 0x%08x", pStruct->ipAddress, szBuf);
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_ADSG_FILTER", "ipMask          = 0x%08x", pStruct->ipMask, szBuf);
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_ADSG_FILTER", "destIpAddress   = 0x%08x", pStruct->destIpAddress, szBuf);
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_ADSG_FILTER", "nPortStart      = %d", pStruct->nPortStart   );
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_ADSG_FILTER", "nPortEnd        = %d", pStruct->nPortEnd   );
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_ADSG_FILTER", "hTunnelHandle   = 0x%08x", pStruct->hTunnelHandle   );
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_END, "VL_ADSG_FILTER", "End");
}

void vlPrintDsgClientId     (int nLevel, VL_DSG_CLIENT_ID       * pStruct     )
{
    char szBuf[256];
    
    char* client_types[] =
    {
        "NONE",
        "BCAST",
        "WKMA",
        "CAS",
        "APP",
        "Unknown",
        "Unknown",
        "Unknown",
        "Unknown",
    };
    
    char* client_ids[] =
    {
        "NONE",
        "SCTE_65",
        "SCTE_18",
        "",
        "",
        "XAIT",
        "",
    };
    
    if(!rdk_dbg_enabled("LOG.RDK.POD", RDK_LOG_INFO)) return;
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_BEGIN, "VL_DSG_CLIENT_ID", "Begin");
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_CLIENT_ID", "eEncodingType   = 0x%02x (%s)", pStruct->eEncodingType,
                                        client_types[VL_SAFE_ARRAY_INDEX(pStruct->eEncodingType, client_types)]);
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_CLIENT_ID", "nEncodingLength = 0x%02x", pStruct->nEncodingLength);
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_CLIENT_ID", "aEncodedId      = %s (%s)",
                                        VL_PRINT_LONG_TO_STR(pStruct->nEncodedId, pStruct->nEncodingLength, szBuf),
                                        ((VL_DSG_CLIENT_ID_ENC_TYPE_BCAST == pStruct->eEncodingType)?client_ids[SAFE_ARRAY_INDEX_UN(pStruct->nEncodedId, client_ids)]: ""));
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_CLIENT_ID", "bClientDisabled = %s", VL_TRUE_COND_STR(pStruct->bClientDisabled));
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_END, "VL_DSG_CLIENT_ID", "End");
}

void vlPrintDsgHostEntry    (int nLevel, VL_DSG_HOST_ENTRY      * pStruct    )
{
    char* entry_types[] =
    {
        "NONE",
        "DSG Filter",
        "Extended Channel MPEG Flow",
        "",
    };
    
    if(!rdk_dbg_enabled("LOG.RDK.POD", RDK_LOG_INFO)) return;
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_BEGIN, "VL_DSG_HOST_ENTRY", "Begin");
    
    vlPrintDsgClientId(nLevel+1, &(pStruct->clientId));
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_DIRECTORY", "eEntryType = 0x%02x (%s)", pStruct->eEntryType,
                entry_types[VL_SAFE_ARRAY_INDEX(pStruct->eEntryType, entry_types)]);
    
    if(VL_DSG_DIR_TYPE_DIRECT_TERM_DSG_FLOW == pStruct->eEntryType)
    {
        /** Direct Termination DSG Flow **/
        vlPrintDsgAdsgFilter(nLevel+1, &(pStruct->adsgFilter));
        vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_DIRECTORY", "ucid              = 0x%02x", pStruct->ucid);
    }
    else if(VL_DSG_DIR_TYPE_EXT_CHNL_MPEG_FLOW == pStruct->eEntryType)
    {
        /** Extended Channel MPEG Flow **/
        // changed: June-09-2008: use virtual adsg filter for extended channel host terminating flows
        // commented: Jan-30-2009: not required: vlPrintDsgAdsgFilter(nLevel+1, &(pStruct->adsgFilter));
    }
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_END, "VL_DSG_HOST_ENTRY", "End");
}

void vlPrintDsgCardEntry    (int nLevel, VL_DSG_CARD_ENTRY      * pStruct    )
{
    if(!rdk_dbg_enabled("LOG.RDK.POD", RDK_LOG_INFO)) return;
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_BEGIN, "VL_DSG_CARD_ENTRY", "Begin");
    
    vlPrintDsgAdsgFilter(nLevel+1, &(pStruct->adsgFilter));
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_END, "VL_DSG_CARD_ENTRY", "End");
}

void vlPrintDsgDirectory    (int nLevel, VL_DSG_DIRECTORY       * pStruct    )
{
    int i = 0;
    char szBuf[256];
    if(!rdk_dbg_enabled("LOG.RDK.POD", RDK_LOG_INFO)) return;
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_BEGIN, "VL_DSG_DIRECTORY", "Begin");
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_DIRECTORY", "bVctIdIncluded = %s"    , VL_TRUE_COND_STR(pStruct->bVctIdIncluded));
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_DIRECTORY", "dirVersion     = 0x%02x", pStruct->dirVersion);
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_DIRECTORY", "nHostEntries  = %d", pStruct->nHostEntries);
    for(i = 0; i < pStruct->nHostEntries; i++)
    {
        int ixSafe = VL_SAFE_ARRAY_INDEX(i, pStruct->aHostEntries);
        vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_DIRECTORY", "aHostEntries[%d]-->", ixSafe);
        vlPrintDsgHostEntry(nLevel+2, &(pStruct->aHostEntries[ixSafe]));
    }
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_DIRECTORY", "nCardEntries  = %d", pStruct->nCardEntries);
    for(i = 0; i < pStruct->nCardEntries; i++)
    {
        int ixSafe = VL_SAFE_ARRAY_INDEX(i, pStruct->aCardEntries);
        vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_DIRECTORY", "aCardEntries[%d]-->", ixSafe);
        vlPrintDsgCardEntry(nLevel+2, &(pStruct->aCardEntries[ixSafe]));
    }
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_DIRECTORY", "nRxFrequencies = %d", pStruct->nRxFrequencies);
    for(i = 0; i < pStruct->nRxFrequencies; i++)
    {
        int ixSafe = VL_SAFE_ARRAY_INDEX(i, pStruct->aRxFrequency);
        vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_DIRECTORY", "aRxFrequency[%d] = %d", ixSafe, pStruct->aRxFrequency[ixSafe]);
    }
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_DIRECTORY", "nInitTimeout   = %d", pStruct->nInitTimeout   );
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_DIRECTORY", "nOperTimeout   = %d", pStruct->nOperTimeout   );
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_DIRECTORY", "nTwoWayTimeout = %d", pStruct->nTwoWayTimeout );
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_DIRECTORY", "nOneWayTimeout = %d", pStruct->nOneWayTimeout );
    
    if(pStruct->bVctIdIncluded)
    {
        vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_DIRECTORY", "nVctId         = 0x%04x", pStruct->nVctId );
    }
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_END, "VL_DSG_DIRECTORY", "End");
}

void vlPrintDsgTunnelFilter (int nLevel, VL_DSG_TUNNEL_FILTER   * pStruct )
{
    int i = 0;
    char szBuf[256];
    if(!rdk_dbg_enabled("LOG.RDK.POD", RDK_LOG_INFO)) return;

    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_BEGIN, "VL_DSG_TUNNEL_FILTER", "Begin");
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_TUNNEL_FILTER", "nTunneId      = 0x%02x", pStruct->nTunneId   );
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_TUNNEL_FILTER", "nAppId        = 0x%04x", pStruct->nAppId   );
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_TUNNEL_FILTER", "dsgMacAddress = %s", VL_PRINT_LONG_TO_STR(pStruct->dsgMacAddress, VL_MAC_ADDR_SIZE, szBuf));
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_TUNNEL_FILTER", "srcIpAddress  = 0x%08x", pStruct->srcIpAddress);
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_TUNNEL_FILTER", "srcIpMask     = 0x%08x", pStruct->srcIpMask);
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_TUNNEL_FILTER", "destIpAddress = 0x%08x", pStruct->destIpAddress);
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_TUNNEL_FILTER", "nPorts        = %d", pStruct->nPorts);
    for(i = 0; i < pStruct->nPorts; i++)
    {
        int ixSafe = VL_SAFE_ARRAY_INDEX(i, pStruct->aPorts);
        vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_TUNNEL_FILTER", "aPorts[%d] = %d", ixSafe, pStruct->aPorts[ixSafe]);
    }
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_TUNNEL_FILTER", "nRemoveHeaderBytes = %d", pStruct->nRemoveHeaderBytes   );
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_END, "VL_DSG_TUNNEL_FILTER", "End");
}

void vlPrintDsgAdvConfig    (int nLevel, VL_DSG_ADV_CONFIG      * pStruct)
{
    int i = 0;
    if(!rdk_dbg_enabled("LOG.RDK.POD", RDK_LOG_INFO)) return;
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_BEGIN, "VL_DSG_ADV_CONFIG", "Begin");
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_ADV_CONFIG", "nTunnelFilters = %d", pStruct->nTunnelFilters);
    for(i = 0; i < pStruct->nTunnelFilters; i++)
    {
        int ixSafe = VL_SAFE_ARRAY_INDEX(i, pStruct->aTunnelFilters);
        vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_ADV_CONFIG", "aTunnelFilters[%d]-->", ixSafe);
        vlPrintDsgTunnelFilter(nLevel+2, &(pStruct->aTunnelFilters[ixSafe]));
    }
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_ADV_CONFIG", "nRxFrequencies = %d", pStruct->nRxFrequencies);
    for(i = 0; i < pStruct->nRxFrequencies; i++)
    {
        int ixSafe = VL_SAFE_ARRAY_INDEX(i, pStruct->aRxFrequency);
        vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_ADV_CONFIG", "aRxFrequency[%d] = %d", ixSafe, pStruct->aRxFrequency[ixSafe]);
    }
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_ADV_CONFIG", "nInitTimeout   = %d", pStruct->nInitTimeout   );
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_ADV_CONFIG", "nOperTimeout   = %d", pStruct->nOperTimeout   );
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_ADV_CONFIG", "nTwoWayTimeout = %d", pStruct->nTwoWayTimeout );
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_ADV_CONFIG", "nOneWayTimeout = %d", pStruct->nOneWayTimeout );
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_END, "VL_DSG_ADV_CONFIG", "End");
}

void vlPrintIpCpe(int nLevel, VL_DSG_IP_CPE * pStruct)
{
    int i = 0;
    if(!rdk_dbg_enabled("LOG.RDK.POD", RDK_LOG_INFO)) return;
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_BEGIN, "VL_DSG_IP_CPE", "Begin");
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_IP_CPE", "srcIpAddress  = 0x%08x", pStruct->srcIpAddress);
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_IP_CPE", "srcIpMask     = 0x%08x", pStruct->srcIpMask);
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_IP_CPE", "destIpAddress = 0x%08x", pStruct->destIpAddress);
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_IP_CPE", "nPortStart    = %d"    , pStruct->nPortStart       );
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_IP_CPE", "nPortEnd      = %d"    , pStruct->nPortEnd );
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_END, "VL_DSG_IP_CPE", "End");
}

void vlPrintDsgClassifier(int nLevel, VL_DSG_CLASSIFIER * pStruct)
{
    int i = 0;
    if(!rdk_dbg_enabled("LOG.RDK.POD", RDK_LOG_INFO)) return;
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_BEGIN, "VL_DSG_CLASSIFIER", "Begin");
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_CLASSIFIER", "nId       = 0x%04x", pStruct->nId       );
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_CLASSIFIER", "nPriority = 0x%02x", pStruct->nPriority );
    
    vlPrintIpCpe(nLevel+1, &(pStruct->ipCpe));
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_END, "VL_DSG_CLASSIFIER", "End");
}

void vlPrintDsgRule(int nLevel, VL_DSG_RULE * pStruct)
{
    int i = 0;
    char szBuf[256];
    if(!rdk_dbg_enabled("LOG.RDK.POD", RDK_LOG_INFO)) return;
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_BEGIN, "VL_DSG_RULE", "Begin");
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_RULE", "nId       = 0x%02x", pStruct->nId       );
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_RULE", "nPriority = 0x%02x", pStruct->nPriority );
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_RULE", "nUCIDs = %d", pStruct->nUCIDs);
    for(i = 0; i < pStruct->nUCIDs; i++)
    {
        int ixSafe = VL_SAFE_ARRAY_INDEX(i, pStruct->aUCIDs);
        vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_RULE", "aUCIDs[%d] = %d", ixSafe, pStruct->aUCIDs[ixSafe]);
    }
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_RULE", "nClients = %d", pStruct->nClients);
    for(i = 0; i < pStruct->nClients; i++)
    {
        int ixSafe = VL_SAFE_ARRAY_INDEX(i, pStruct->aClients);
        vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_RULE", "aClients[%d]-->", ixSafe);
        vlPrintDsgClientId(nLevel+2, &(pStruct->aClients[ixSafe]));
    }
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_RULE", "dsgMacAddress = %s", VL_PRINT_LONG_TO_STR(pStruct->dsgMacAddress, VL_MAC_ADDR_SIZE, szBuf));
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_RULE", "nClassifierId = 0x%04x", pStruct->nClassifierId   );
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_END, "VL_DSG_RULE", "End");
}

void vlPrintDsgConfig(int nLevel, VL_DSG_CONFIG * pStruct)
{
    int i = 0;
    if(!rdk_dbg_enabled("LOG.RDK.POD", RDK_LOG_INFO)) return;
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_BEGIN, "VL_DSG_CONFIG", "Begin");
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_CONFIG", "nRxFrequencies = %d", pStruct->nRxFrequencies);
    for(i = 0; i < pStruct->nRxFrequencies; i++)
    {
        int ixSafe = VL_SAFE_ARRAY_INDEX(i, pStruct->aRxFrequency);
        vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_CONFIG", "aRxFrequency[%d] = %d", ixSafe, pStruct->aRxFrequency[ixSafe]);
    }
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_CONFIG", "nInitTimeout   = %d", pStruct->nInitTimeout   );
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_CONFIG", "nOperTimeout   = %d", pStruct->nOperTimeout   );
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_CONFIG", "nTwoWayTimeout = %d", pStruct->nTwoWayTimeout );
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_CONFIG", "nOneWayTimeout = %d", pStruct->nOneWayTimeout );
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_END, "VL_DSG_CONFIG", "End");
}

void vlPrintDsgDCD(int nLevel, VL_DSG_DCD * pStruct)
{
    int i = 0;
    if(!rdk_dbg_enabled("LOG.RDK.POD", RDK_LOG_INFO)) return;
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_BEGIN, "VL_DSG_DCD", "Begin");
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_DCD", "nClassifiers = %d", pStruct->nClassifiers);
    for(i = 0; i < pStruct->nClassifiers; i++)
    {
        int ixSafe = VL_SAFE_ARRAY_INDEX(i, pStruct->aClassifiers);
        vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_DCD", "aClassifiers[%d]-->", ixSafe);
        vlPrintDsgClassifier(nLevel+2, &(pStruct->aClassifiers[ixSafe]));
    }
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_DCD", "nRules = %d", pStruct->nRules);
    for(i = 0; i < pStruct->nRules; i++)
    {
        int ixSafe = VL_SAFE_ARRAY_INDEX(i, pStruct->aRules);
        vlTraceStack(nLevel+1, VL_TRACE_STACK_TAG_NORMAL, "VL_DSG_DCD", "aRules[%d]-->", ixSafe);
        vlPrintDsgRule(nLevel+2, &(pStruct->aRules[ixSafe]));
    }
    
    vlPrintDsgConfig(nLevel+1, &(pStruct->Config));
    
    vlTraceStack(nLevel, VL_TRACE_STACK_TAG_END, "VL_DSG_DCD", "End");
}

#ifdef __cplusplus
}
#endif
