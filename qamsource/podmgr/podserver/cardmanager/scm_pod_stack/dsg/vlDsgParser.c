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
extern int  vlg_bDsgBasicFilterValid;
extern int  vlg_bDsgConfigFilterValid;
extern int  vlg_bDsgDirectoryFilterValid;

#ifdef __cplusplus
extern "C" {
#endif

void vlParseDsgInit()
{
    static int is = 0;
    is++;
    /* Fake func to force link-in of vlDsgParser.o to dsg.dsg from libDsg.a */
    vlPrintDsgInit();
}

void vlParseDsgMode(VL_BYTE_STREAM * pParseBuf, VL_DSG_MODE * pDsgMode)
{
    int i = 0;
    VL_ZERO_STRUCT(*pDsgMode);

    pDsgMode->eOperationalMode   = (VL_DSG_OPERATIONAL_MODE)vlParseByte(pParseBuf);

    if((VL_DSG_OPERATIONAL_MODE_BASIC_TWO_WAY == pDsgMode->eOperationalMode) ||
       (VL_DSG_OPERATIONAL_MODE_BASIC_ONE_WAY == pDsgMode->eOperationalMode))
    {
        pDsgMode->nMacAddresses      = vlParseByte(pParseBuf);
        for(i = 0; i < pDsgMode->nMacAddresses; i++)
        {
            int ixSafe = VL_SAFE_ARRAY_INDEX(i, pDsgMode->aMacAddresses);
            pDsgMode->aMacAddresses[ixSafe] = vlParse6ByteLong(pParseBuf);
        }

        pDsgMode->nRemoveHeaderBytes   = vlParseShort(pParseBuf);
        vlg_nHeaderBytesToRemove = pDsgMode->nRemoveHeaderBytes;

        vlg_bDsgBasicFilterValid = 1;
    }
    else
    {
        vlg_nHeaderBytesToRemove = 0;
        vlg_bDsgBasicFilterValid = 0;
    }
}

void vlParseDsgAdsgFilter(VL_BYTE_STREAM * pParseBuf, VL_ADSG_FILTER * pAdsgFilter)
{
    VL_ZERO_STRUCT(*pAdsgFilter);

    pAdsgFilter->nTunneId = vlParseByte(pParseBuf);
    pAdsgFilter->nTunnelPriority = vlParseByte(pParseBuf);

    pAdsgFilter->macAddress     = vlParse6ByteLong(pParseBuf);
    pAdsgFilter->ipAddress      = vlParseLong(pParseBuf);
    pAdsgFilter->ipMask         = vlParseLong(pParseBuf);
    pAdsgFilter->destIpAddress  = vlParseLong(pParseBuf);

    pAdsgFilter->nPortStart     = vlParseShort(pParseBuf);
    pAdsgFilter->nPortEnd       = vlParseShort(pParseBuf);
}

void vlParseDsgClientId(VL_BYTE_STREAM * pParseBuf, VL_DSG_CLIENT_ID * pClientId)
{
    VL_ZERO_STRUCT(*pClientId);

    pClientId->eEncodingType = (VL_DSG_CLIENT_ID_ENC_TYPE)vlParseByte(pParseBuf);
    pClientId->nEncodingLength = vlParseByte(pParseBuf);
    pClientId->nEncodedId = vlParseNByteLong(pParseBuf, pClientId->nEncodingLength);
}

void vlParseDsgHostEntry(VL_BYTE_STREAM * pParseBuf, VL_DSG_HOST_ENTRY * pHostEntry)
{
    VL_ZERO_STRUCT(*pHostEntry);

    vlParseDsgClientId(pParseBuf, &(pHostEntry->clientId));

    pHostEntry->eEntryType = (VL_DSG_DIR_TYPE)vlParseByte(pParseBuf);

    if(VL_DSG_DIR_TYPE_DIRECT_TERM_DSG_FLOW == pHostEntry->eEntryType)
    {
        /** Direct Termination DSG Flow **/
        vlParseDsgAdsgFilter(pParseBuf, &(pHostEntry->adsgFilter));
        pHostEntry->ucid = vlParseByte(pParseBuf);
    }
    else if(VL_DSG_DIR_TYPE_EXT_CHNL_MPEG_FLOW == pHostEntry->eEntryType)
    {
        /** Extended Channel MPEG Flow **/
    }
}

void vlParseDsgCardEntry(VL_BYTE_STREAM * pParseBuf, VL_DSG_CARD_ENTRY * pCardEntry)
{
    VL_ZERO_STRUCT(*pCardEntry);

    vlParseDsgAdsgFilter(pParseBuf, &(pCardEntry->adsgFilter));
}

void vlParseDsgDirectory(VL_BYTE_STREAM * pParseBuf, VL_DSG_DIRECTORY * pDirectory)
{
    int i = 0;
    VL_ZERO_STRUCT(*pDirectory);

    pDirectory->bVctIdIncluded = (vlParseByte(pParseBuf)&0x01);
    pDirectory->dirVersion     = vlParseByte(pParseBuf);

    pDirectory->nHostEntries   = vlParseByte(pParseBuf);
    for(i = 0; i < pDirectory->nHostEntries; i++)
    {
        int ixSafe = VL_SAFE_ARRAY_INDEX(i, pDirectory->aHostEntries);
        vlParseDsgHostEntry(pParseBuf, &(pDirectory->aHostEntries[ixSafe]));
    }

    pDirectory->nCardEntries   = vlParseByte(pParseBuf);
    for(i = 0; i < pDirectory->nCardEntries; i++)
    {
        int ixSafe = VL_SAFE_ARRAY_INDEX(i, pDirectory->aCardEntries);
        vlParseDsgCardEntry(pParseBuf, &(pDirectory->aCardEntries[ixSafe]));
    }

    pDirectory->nRxFrequencies   = vlParseByte(pParseBuf);
    for(i = 0; i < pDirectory->nRxFrequencies; i++)
    {
        int ixSafe = VL_SAFE_ARRAY_INDEX(i, pDirectory->aRxFrequency);
        pDirectory->aRxFrequency[ixSafe] = vlParseLong(pParseBuf);
    }

    pDirectory->nInitTimeout    = vlParseShort(pParseBuf);
    pDirectory->nOperTimeout    = vlParseShort(pParseBuf);
    pDirectory->nTwoWayTimeout  = vlParseShort(pParseBuf);
    pDirectory->nOneWayTimeout  = vlParseShort(pParseBuf);

    if(pDirectory->bVctIdIncluded)
    {
        pDirectory->nVctId          = vlParseShort(pParseBuf);
    }

    vlg_nHeaderBytesToRemove = 0;

    vlg_bDsgDirectoryFilterValid = 1;
}

void vlParseDsgTunnelFilter(VL_BYTE_STREAM * pParseBuf, VL_DSG_TUNNEL_FILTER * pTunnelFilter)
{
    int i = 0;
    VL_ZERO_STRUCT(*pTunnelFilter);

    pTunnelFilter->nTunneId = vlParseByte(pParseBuf);
    pTunnelFilter->nAppId   = vlParseShort(pParseBuf);

    pTunnelFilter->dsgMacAddress = vlParse6ByteLong(pParseBuf);
    pTunnelFilter->srcIpAddress  = vlParseLong(pParseBuf);
    pTunnelFilter->srcIpMask     = vlParseLong(pParseBuf);
    pTunnelFilter->destIpAddress = vlParseLong(pParseBuf);

    pTunnelFilter->nPorts   = vlParseByte(pParseBuf);
    for(i = 0; i < pTunnelFilter->nPorts; i++)
    {
        int ixSafe = VL_SAFE_ARRAY_INDEX(i, pTunnelFilter->aPorts);
        pTunnelFilter->aPorts[ixSafe] = vlParseShort(pParseBuf);
    }

    pTunnelFilter->nRemoveHeaderBytes   = vlParseShort(pParseBuf);
    vlg_nHeaderBytesToRemove = pTunnelFilter->nRemoveHeaderBytes;
}

void vlParseDsgAdvConfig(VL_BYTE_STREAM * pParseBuf, VL_DSG_ADV_CONFIG * pAdvConfig)
{
    int i = 0;
    VL_ZERO_STRUCT(*pAdvConfig);

    pAdvConfig->nTunnelFilters   = vlParseByte(pParseBuf);
    for(i = 0; i < pAdvConfig->nTunnelFilters; i++)
    {
        int ixSafe = VL_SAFE_ARRAY_INDEX(i, pAdvConfig->aTunnelFilters);
        vlParseDsgTunnelFilter(pParseBuf, &(pAdvConfig->aTunnelFilters[ixSafe]));
    }

    pAdvConfig->nRxFrequencies   = vlParseByte(pParseBuf);
    for(i = 0; i < pAdvConfig->nRxFrequencies; i++)
    {
        int ixSafe = VL_SAFE_ARRAY_INDEX(i, pAdvConfig->aRxFrequency);
        pAdvConfig->aRxFrequency[ixSafe] = vlParseLong(pParseBuf);
    }

    pAdvConfig->nInitTimeout    = vlParseShort(pParseBuf);
    pAdvConfig->nOperTimeout    = vlParseShort(pParseBuf);
    pAdvConfig->nTwoWayTimeout  = vlParseShort(pParseBuf);
    pAdvConfig->nOneWayTimeout  = vlParseShort(pParseBuf);

    vlg_bDsgConfigFilterValid = 1;
}

void vlParseDsgIpCpe(unsigned lPrefix, int nOrgLength, unsigned char * pOrgBuf,
                     VL_BYTE_STREAM * pParseBuf, VL_DSG_IP_CPE * pStruct)
{
    int i = 0;
    unsigned long lTag = 0;
    int           nLength = 0;
    unsigned long nValue  = 0;
    VL_ZERO_STRUCT(*pStruct);

    while((VL_BYTE_STREAM_GET_CURRENT_BUF(pParseBuf)-pOrgBuf) < nOrgLength)
    {
        lTag = (lPrefix<<8)|vlParseByte(pParseBuf);
        nLength = vlParseCcApduLengthField(pParseBuf);

        switch(lTag)
        {
            case VL_DSG_TLV_TYPE_DSG_CLF_IP_PCE_SRC_IP:
            {
                pStruct->srcIpAddress = (unsigned long)vlParseNByteLong(pParseBuf, nLength);
            }
            break;

            case VL_DSG_TLV_TYPE_DSG_CLF_IP_PCE_SRC_IP_MASK:
            {
                pStruct->srcIpMask = (unsigned long)vlParseNByteLong(pParseBuf, nLength);
            }
            break;

            case VL_DSG_TLV_TYPE_DSG_CLF_IP_PCE_DST_IP:
            {
                pStruct->destIpAddress = (unsigned long)vlParseNByteLong(pParseBuf, nLength);
            }
            break;

            case VL_DSG_TLV_TYPE_DSG_CLF_IP_PCE_DST_PORT_START:
            {
                pStruct->nPortStart = (unsigned short)vlParseNByteLong(pParseBuf, nLength);
            }
            break;

            case VL_DSG_TLV_TYPE_DSG_CLF_IP_PCE_DSG_PORT_END:
            {
                pStruct->nPortEnd = (unsigned short)vlParseNByteLong(pParseBuf, nLength);
            }
            break;

            default:
            {
                // discard superfluous data (pParseBuf, NULL, length, 0 discards)
                vlParseBuffer(pParseBuf, NULL, nLength, 0);
            }
            break;
        }
    }
}

void vlParseDsgConfig(unsigned lPrefix, int nOrgLength, unsigned char * pOrgBuf,
                      VL_BYTE_STREAM * pParseBuf, VL_DSG_CONFIG * pStruct)
{
    int i = 0;
    unsigned long lTag = 0;
    int           nLength = 0;
    unsigned long nValue  = 0;
    VL_ZERO_STRUCT(*pStruct);

    while((VL_BYTE_STREAM_GET_CURRENT_BUF(pParseBuf)-pOrgBuf) < nOrgLength)
    {
        lTag = (lPrefix<<8)|vlParseByte(pParseBuf);
        nLength = vlParseCcApduLengthField(pParseBuf);

        switch(lTag)
        {
            case VL_DSG_TLV_TYPE_DSG_CONFIG_CHANNEL:
            {
                int ixSafe = VL_SAFE_ARRAY_INDEX(pStruct->nRxFrequencies, pStruct->aRxFrequency);
                pStruct->aRxFrequency[ixSafe] = (unsigned long)vlParseNByteLong(pParseBuf, nLength);
                pStruct->nRxFrequencies++;
            }
            break;

            case VL_DSG_TLV_TYPE_DSG_CONFIG_INIT_TIMEOUT:
            {
                pStruct->nInitTimeout = (unsigned short)vlParseNByteLong(pParseBuf, nLength);
            }
            break;

            case VL_DSG_TLV_TYPE_DSG_CONFIG_OPER_TIMEOUT:
            {
                pStruct->nOperTimeout = (unsigned short)vlParseNByteLong(pParseBuf, nLength);
            }
            break;

            case VL_DSG_TLV_TYPE_DSG_CONFIG_2WR_TIMEOUT:
            {
                pStruct->nTwoWayTimeout = (unsigned short)vlParseNByteLong(pParseBuf, nLength);
            }
            break;

            case VL_DSG_TLV_TYPE_DSG_CONFIG_1WR_TIMEOUT:
            {
                pStruct->nOneWayTimeout = (unsigned short)vlParseNByteLong(pParseBuf, nLength);
            }
            break;

            case VL_DSG_TLV_TYPE_DSG_CONFIG_VENDOR:
            {
                // discard vendor data (pParseBuf, NULL, length, 0 discards)
                vlParseBuffer(pParseBuf, NULL, nLength, 0);
            }
            break;

            default:
            {
                // discard superfluous data (pParseBuf, NULL, length, 0 discards)
                vlParseBuffer(pParseBuf, NULL, nLength, 0);
            }
            break;
        }
    }
}

void vlParseDsgClientIdList(int nOrgLength, unsigned char * pOrgBuf,
                            VL_BYTE_STREAM * pParseBuf, VL_DSG_RULE * pStruct)
{
    // aClients has already zeroed, should not be zeroed again

    while((VL_BYTE_STREAM_GET_CURRENT_BUF(pParseBuf)-pOrgBuf) < nOrgLength)
    {
        int ixSafe = VL_SAFE_ARRAY_INDEX(pStruct->nClients, pStruct->aClients);
        vlParseDsgClientId(pParseBuf, &(pStruct->aClients[ixSafe]));
        pStruct->nClients++;
    }
}

void vlParseDsgRule(unsigned lPrefix, int nOrgLength, unsigned char * pOrgBuf,
                    VL_BYTE_STREAM * pParseBuf, VL_DSG_RULE * pStruct)
{
    int i = 0;
    unsigned long lTag = 0;
    int           nLength = 0;
    unsigned long nValue  = 0;
    VL_ZERO_STRUCT(*pStruct);

    while((VL_BYTE_STREAM_GET_CURRENT_BUF(pParseBuf)-pOrgBuf) < nOrgLength)
    {
        lTag = (lPrefix<<8)|vlParseByte(pParseBuf);
        nLength = vlParseCcApduLengthField(pParseBuf);

        switch(lTag)
        {
            case VL_DSG_TLV_TYPE_DSG_RULE_ID:
            {
                pStruct->nId = (unsigned char)vlParseNByteLong(pParseBuf, nLength);
            }
            break;

            case VL_DSG_TLV_TYPE_DSG_RULE_PRIORITY:
            {
                pStruct->nPriority = (unsigned char)vlParseNByteLong(pParseBuf, nLength);
            }
            break;

            case VL_DSG_TLV_TYPE_DSG_RULE_UCID_LIST:
            {
                pStruct->nUCIDs = nLength;
                vlParseBuffer(pParseBuf, pStruct->aUCIDs, nLength, sizeof(pStruct->aUCIDs));
            }
            break;

            case VL_DSG_TLV_TYPE_DSG_RULE_CLIENT_ID:
            {
                vlParseDsgClientIdList(nLength, VL_BYTE_STREAM_GET_CURRENT_BUF(pParseBuf), pParseBuf, pStruct);
            }
            break;

            case VL_DSG_TLV_TYPE_DSG_RULE_DSG_MAC:
            {
                pStruct->dsgMacAddress = vlParseNByteLong(pParseBuf, nLength);
            }
            break;

            case VL_DSG_TLV_TYPE_DSG_RULE_CLF_ID:
            {
                pStruct->nClassifierId = (unsigned short)vlParseNByteLong(pParseBuf, nLength);
            }
            break;

            case VL_DSG_TLV_TYPE_DSG_RULE_VENDOR:
            {
                // discard vendor data (pParseBuf, NULL, length, 0 discards)
                vlParseBuffer(pParseBuf, NULL, nLength, 0);
            }
            break;

            default:
            {
                // discard superfluous data (pParseBuf, NULL, length, 0 discards)
                vlParseBuffer(pParseBuf, NULL, nLength, 0);
            }
            break;
        }
    }
}

void vlParseDsgClassifier(unsigned lPrefix, int nOrgLength, unsigned char * pOrgBuf,
                          VL_BYTE_STREAM * pParseBuf, VL_DSG_CLASSIFIER * pStruct)
{
    int i = 0;
    unsigned long lTag = 0;
    int           nLength = 0;
    VL_ZERO_STRUCT(*pStruct);

    while((VL_BYTE_STREAM_GET_CURRENT_BUF(pParseBuf)-pOrgBuf) < nOrgLength)
    {
        lTag = (lPrefix<<8)|vlParseByte(pParseBuf);
        nLength = vlParseCcApduLengthField(pParseBuf);

        switch(lTag)
        {
            case VL_DSG_TLV_TYPE_DSG_CLF_ID:
            {
                pStruct->nId = vlParseShort(pParseBuf);
            }
            break;

            case VL_DSG_TLV_TYPE_DSG_CLF_PRIORITY:
            {
                pStruct->nPriority = vlParseByte(pParseBuf);
            }
            break;

            case VL_DSG_TLV_TYPE_DSG_CLF_IP_PCE:
            {
                vlParseDsgIpCpe(lTag, nLength, VL_BYTE_STREAM_GET_CURRENT_BUF(pParseBuf), pParseBuf, &(pStruct->ipCpe));
            }
            break;

            default:
            {
                // discard superfluous data (pParseBuf, NULL, length, 0 discards)
                vlParseBuffer(pParseBuf, NULL, nLength, 0);
            }
            break;
        }
    }
}

void vlParseDsgDCD(int nOrgLength, unsigned char * pOrgBuf,
                   VL_BYTE_STREAM * pParseBuf, VL_DSG_DCD * pStruct)
{
    int i = 0;
    unsigned long lTag = 0;
    int           nLength = 0;
    VL_ZERO_STRUCT(*pStruct);

    while((VL_BYTE_STREAM_GET_CURRENT_BUF(pParseBuf)-pOrgBuf) < nOrgLength)
    {
        lTag = vlParseByte(pParseBuf);
        nLength = vlParseCcApduLengthField(pParseBuf);

        switch(lTag)
        {
            case VL_DSG_TLV_TYPE_DSG_CLF:
            {
                int ixSafe = VL_SAFE_ARRAY_INDEX(pStruct->nClassifiers, pStruct->aClassifiers);
                vlParseDsgClassifier(lTag, nLength, VL_BYTE_STREAM_GET_CURRENT_BUF(pParseBuf), pParseBuf, &(pStruct->aClassifiers[ixSafe]));
                pStruct->nClassifiers++;
            }
            break;

            case VL_DSG_TLV_TYPE_DSG_RULE:
            {
                int ixSafe = VL_SAFE_ARRAY_INDEX(pStruct->nRules, pStruct->aRules);
                vlParseDsgRule(lTag, nLength, VL_BYTE_STREAM_GET_CURRENT_BUF(pParseBuf), pParseBuf, &(pStruct->aRules[ixSafe]));
                pStruct->nRules++;
            }
            break;

            case VL_DSG_TLV_TYPE_DSG_CONFIG:
            {
                vlParseDsgConfig(lTag, nLength, VL_BYTE_STREAM_GET_CURRENT_BUF(pParseBuf), pParseBuf, &(pStruct->Config));
            }
            break;

            default:
            {
                // discard superfluous data (pParseBuf, NULL, length, 0 discards)
                vlParseBuffer(pParseBuf, NULL, nLength, 0);
            }
            break;
        }
    }
}

#ifdef __cplusplus
}
#endif

