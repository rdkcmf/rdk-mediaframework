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



#include "poddef.h"    /* To support log file */
#include "lcsm_log.h"                   /* To support log file */

#include "global_event_map.h"
#include "utils.h"
#include "appmgr.h"
#include "xchan_interface.h"
#include "bufParseUtilities.h"
#include "ip_types.h"
#include <string.h>
#include "xchanResApi.h"
#include "utilityMacros.h"
#include "rmf_osal_mem.h"
#include <coreUtilityApi.h>

#if USE_SYSRES_MLT
#include "mlt_malloc.h"
#endif

#define __MTAG__ VL_CARD_MANAGER


#define MAX_MPEG_SECTIONS 6
#define RETURN_ERROR 0xFFFFFFFF


#ifdef __cplusplus
extern "C" {
#endif

static unsigned nextMpegSection( void );
static void deleteMpegSection( unsigned sectionIndex );
static unsigned findSectionByFlowId( unsigned flow_id );

// first session can contain up to 6 MPEG_section flows
typedef struct
{
    unsigned short pid;
    unsigned flow_id;

}   MPEG_section;

typedef struct
{
    MPEG_section section[MAX_MPEG_SECTIONS];
    unsigned sessionNum;
    // for communication with Glenn's transport
    LCSM_DEVICE_HANDLE lcsmHandle;
    unsigned returnQueue;

}   Session1;

// second session can contain 1 IP_U, IP_M, or DSG? flow (2 way pod only)
typedef struct
{
    unsigned multicast_group_id;

}   XCHAN_IP_M;

typedef union
{
    VL_IP_IF_PARAMS unicast;
    XCHAN_IP_M multicast;
    unsigned sessionNum;

}   Session2;

static Session1 session1;
#ifdef TWO_WAY
static Session2 session2;
#endif

VL_IP_IF_PARAMS     unicast_ip_flow;

static unsigned sectionOpening = 0;

void sendFlowRequest( long int handle, int servicetype, long int pid, long int returnQueue )
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s:handle=%u, servicetype=%d, pid=%d, returnQueue=%d \n", __FUNCTION__, handle, servicetype, pid, returnQueue);
    LCSM_DEVICE_HANDLE deviceHandle = (LCSM_DEVICE_HANDLE)handle;
    LCSM_EVENT_INFO eventInfo;
    unsigned sectionNum;
    unsigned char packetPtr[3];

    sectionNum = nextMpegSection();
    sectionOpening = sectionNum;
    if ( sectionNum == RETURN_ERROR )
    {
        // MAX_MPEG_SECTIONS already used
        eventInfo.event = ASYNC_EC_FLOW_REQUEST_CNF;
        eventInfo.x = 0x1;      // number of flows exceeded
        eventInfo.y = -1;       // INVALID_FLOW_ID
        eventInfo.dataPtr = NULL;
        eventInfo.dataLength = 0;
        eventInfo.returnStatus = 0;//MPEG_SECTION;
        lcsm_send_event( deviceHandle, POD_XCHAN_QUEUE, &eventInfo );
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "ERROR: Exceeded Number flow reqs.\n");
        return;
    }

    session1.section[sectionNum].pid = (unsigned short)pid;
    session1.lcsmHandle     = deviceHandle;
    session1.returnQueue    = returnQueue;

    packetPtr[0] = 0; // service_type of 0 means MPEG_section (SCTE-28 2003 p. 85)
    packetPtr[1] = (unsigned char)((pid & 0x0000FF00) >> 8);
    packetPtr[2] = (unsigned char)(pid & 0x000000FF);
    //APDU_NewFlowReq( session1.sessionNum, packetPtr, 3*sizeof(unsigned char) );
    APDU_XCHAN2POD_newMpegFlowReq (session1.sessionNum, pid);
}


void sendIPFlowRequest( long int handle, int servicetype, unsigned char *apduBuffer, long int returnQueue )
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    LCSM_DEVICE_HANDLE deviceHandle = (LCSM_DEVICE_HANDLE)handle;
    LCSM_EVENT_INFO eventInfo;
    unsigned long len = *(apduBuffer+7);

    APDU_XCHAN2POD_SendIPUFlowReq(session1.sessionNum, &apduBuffer[1],
                                  apduBuffer[7], &apduBuffer[8]);
}

#ifdef USE_IPDIRECT_UNICAST
// IPDU register flow
void sendIPDUFlowRequest( long int handle, int servicetype, unsigned char *apduBuffer, long int returnQueue )
{
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    LCSM_DEVICE_HANDLE deviceHandle = (LCSM_DEVICE_HANDLE)handle;

    USHORT sesnum = 1;
    APDU_XCHAN2POD_newIPDUFlowReq(sesnum, apduBuffer, returnQueue);
}
#endif // USE_IPDIRECT_UNICAST

// got a confirmation APDU from the pod, now send an event to transport with this information
void sendFlowConfirm( unsigned char *ptr, int len)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    unsigned char status_field = (unsigned char)-1, flows_remaining = (unsigned char)-1, service_type = (unsigned char)-1;
    unsigned long flow_id = (unsigned char)-1;
    LCSM_EVENT_INFO eventInfo;
    unsigned char * org_ptr = ptr;

    ptr += 3;  // get past tag
#if (1)
        ptr += bGetTLVLength( ptr, NULL );  // get past variable sized length field
#else /* Old way to compute len: Save for now (just in case!) Delete later */
    ptr += getLengthFieldSize( ptr );  // get past variable sized length field
#endif

    status_field = *ptr;
    ptr++;
    flows_remaining = *ptr;
    ptr++;
    if ( status_field == 0 )
    {
        // get 24 bit flow_id
        flow_id = 0;
        flow_id |= (unsigned)(*ptr << 16);
        ptr++;
        flow_id |= (unsigned)(*ptr << 8);
        ptr++;
        flow_id |= (unsigned)(*ptr);
        ptr++;
        service_type = *ptr;
        ptr++;
    }
    session1.section[sectionOpening].flow_id  = flow_id;

    eventInfo.event = ASYNC_EC_FLOW_REQUEST_CNF;
    eventInfo.x = status_field;
    eventInfo.y = flow_id;
    eventInfo.z = flows_remaining;
    eventInfo.returnStatus = service_type;
    if(service_type == VL_XCHAN_FLOW_TYPE_IP_U) //IP_U
    {
    VL_IP_IF_PARAMS *pUnicastParams = NULL;
	rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(VL_IP_IF_PARAMS),(void **)&pUnicastParams);
    if(pUnicastParams == NULL)
        return;

	memset(&unicast_ip_flow,0,sizeof(unicast_ip_flow));
        *pUnicastParams = unicast_ip_flow;
        eventInfo.dataPtr = pUnicastParams;//&unicast_ip_flow;//NULL;
        eventInfo.dataLength =sizeof(unicast_ip_flow);// 0;

        if(0 == status_field)
        {
            VL_BYTE_STREAM_INST(IPU_FLOW_RESPONSE, ParseBufIpuCnf, ptr, (len - (ptr - org_ptr)));
            int nOptionLen = 0;
            unsigned char szBuf[256];
            VL_XCHAN_IPU_FLOW_CONF confParams;
            vlParseIPUconfParams(pParseBufIpuCnf, (len - (ptr - org_ptr)), &confParams);
            memcpy(unicast_ip_flow.ipAddress, confParams.ipAddress,sizeof(confParams.ipAddress));
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "IP Address  = %d.%d.%d.%d\n", unicast_ip_flow.ipAddress[0], unicast_ip_flow.ipAddress[1],
                                                 unicast_ip_flow.ipAddress[2], unicast_ip_flow.ipAddress[3]);

            VL_BYTE_STREAM_INST(DhcpOptions, ParseBuf, confParams.aOptions, confParams.nOptionSize);
            nOptionLen = confParams.nOptionSize;
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "======================================================\n");
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "Option Length = %d\n", nOptionLen);
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "======================================================\n");
            while(VL_BYTE_STREAM_USED(pParseBuf) < nOptionLen)
            {
                unsigned char cTag = vlParseByte(pParseBuf);
                unsigned long nLength = vlParseByte(pParseBuf);

                if((0 == cTag) && (0 == nLength))
                {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "sendFlowConfirm: Options: Both Tag and Length are 0.\n");
                    break;
                }

                if(0 == nLength)
                {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "sendFlowConfirm: Options: Length is 0 for Tag %d.\n", cTag);
                }

                switch(cTag)
                {
                    case VL_IP_DHCP_OPTION_CODE_SUBNET_MASK:
                    {
                        vlParseBuffer(pParseBuf, unicast_ip_flow.subnetMask, nLength, sizeof(unicast_ip_flow.subnetMask));
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "Subnet Mask = %d.%d.%d.%d\n", unicast_ip_flow.subnetMask[0], unicast_ip_flow.subnetMask[1],
                               unicast_ip_flow.subnetMask[2], unicast_ip_flow.subnetMask[3]);
                    }
                    break;

                    case VL_IP_DHCP_OPTION_CODE_ROUTER:
                    {
                        vlParseBuffer(pParseBuf, unicast_ip_flow.router, nLength, sizeof(unicast_ip_flow.router));
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "Router      = %d.%d.%d.%d\n", unicast_ip_flow.router[0], unicast_ip_flow.router[1],
                               unicast_ip_flow.router[2], unicast_ip_flow.router[3]);
                    }
                    break;

                    case VL_IP_DHCP_OPTION_CODE_DNS_SERVER:
                    {
                        vlParseBuffer(pParseBuf, unicast_ip_flow.dns, nLength, sizeof(unicast_ip_flow.dns));
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "DNS         = %d.%d.%d.%d\n", unicast_ip_flow.dns[0], unicast_ip_flow.dns[1],
                               unicast_ip_flow.dns[2], unicast_ip_flow.dns[3]);
                    }
                    break;

                    default:
                    {
                        vlParseBuffer(pParseBuf, szBuf, nLength, sizeof(szBuf));
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "======================================================\n");
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "cTag = %d, nLength = %d\n", cTag, nLength);
                        vlMpeosDumpBuffer(RDK_LOG_INFO, "LOG.RDK.POD", szBuf, nLength);
                    }
                    break;
                }
            }
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "======================================================\n");
        }
    *pUnicastParams = unicast_ip_flow;
    }
    else
    {
        eventInfo.dataPtr = NULL;
        eventInfo.dataLength = 0;
    }


    lcsm_send_event( session1.lcsmHandle, POD_XCHAN_QUEUE, &eventInfo );
}



void sendDeleteFlowRequest( long int handle, long int flowId, long int returnQueue )
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    LCSM_DEVICE_HANDLE deviceHandle = (LCSM_DEVICE_HANDLE)handle;
    unsigned char packetPtr[3];

    session1.lcsmHandle     = deviceHandle;
    session1.returnQueue    = returnQueue;

    // put flow id in a 24 bit length
    packetPtr[0] = (unsigned char)((flowId & 0x00FF0000) >> 16 );
    packetPtr[1] = (unsigned char)((flowId & 0x0000FF00) >> 8 );
    packetPtr[2] = (unsigned char)(flowId & 0x000000FF);

    APDU_DeleteFlowReq( session1.sessionNum, packetPtr, 3*sizeof(unsigned char) );
}

void deleteFlowConfirm( unsigned char *ptr )
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    LCSM_EVENT_INFO eventInfo;
    unsigned char status_field;
    unsigned flow_id;
    unsigned sectionNum;

    ptr += 3;  // get past tag
#if (1)
        ptr += bGetTLVLength( ptr, NULL );  // get past variable sized length field
#else /* Old way to compute len: Save for now (just in case!) Delete later */
    ptr += getLengthFieldSize( ptr );  // get past variable sized length field
#endif

    // get 24 bit flow_id
    flow_id = 0;
    flow_id |= (unsigned)(*ptr << 16);
    ptr++;
    flow_id |= (unsigned)(*ptr << 8);
    ptr++;
    flow_id |= (unsigned)(*ptr);
    ptr++;
    status_field = *ptr;

    sectionNum = findSectionByFlowId( flow_id );
    deleteMpegSection( sectionNum );

    eventInfo.event = ASYNC_EC_FLOW_DELETE_CNF;
    eventInfo.x = flow_id;
    eventInfo.dataPtr = NULL;
    eventInfo.dataLength = 0;
    lcsm_send_event( session1.lcsmHandle, POD_XCHAN_QUEUE, &eventInfo );
}



void sendLostFlow( unsigned char *ptr )
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    LCSM_EVENT_INFO eventInfo;
    unsigned flow_id;
    unsigned char reason_field;
    unsigned sectionNum;

    ptr += 3;  // get past tag
#if (1)
        ptr += bGetTLVLength( ptr, NULL );  // get past variable sized length field
#else /* Old way to compute len: Save for now (just in case!) Delete later */
    ptr += getLengthFieldSize( ptr );  // get past variable sized length field
#endif

    // get 24 bit flow_id
    flow_id = 0;
    flow_id |= (unsigned)(*ptr << 16);
    ptr++;
    flow_id |= (unsigned)(*ptr << 8);
    ptr++;
    flow_id |= (unsigned)(*ptr);
    ptr++;
    reason_field = *ptr;

    // confirm lost flow with pod
    sendLostFlowConfirm( flow_id );

    // delete from our list
    sectionNum = findSectionByFlowId( flow_id );
    deleteMpegSection( sectionNum );

    eventInfo.event = ASYNC_EC_FLOW_LOST_EVENT;
    eventInfo.x = flow_id;
    eventInfo.y = reason_field;
    eventInfo.dataPtr = NULL;
    eventInfo.dataLength = 0;
    lcsm_send_event( session1.lcsmHandle, POD_XCHAN_QUEUE, &eventInfo );
}

void sendLostFlowConfirm( long int flowId )
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    unsigned char packetPtr[4];

    // put flow id in a 24 bit length
    packetPtr[0] = (unsigned char)((flowId & 0x00FF0000) >> 16 );
    packetPtr[1] = (unsigned char)((flowId & 0x0000FF00) >> 8 );
    packetPtr[2] = (unsigned char)(flowId & 0x000000FF);
    packetPtr[3] = 0;       // status field of 0 means "acknowledged" (SCTE-28 2003 p. 91)

    APDU_LostFlowCnf( session1.sessionNum, packetPtr, 4*sizeof(unsigned char) );
}



// mpeg section functions
void initializeSession1( unsigned sessionNum )
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    unsigned i;

    for ( i=0; i<MAX_MPEG_SECTIONS; i++ )
    {
        session1.section[i].pid     = 0;
        session1.section[i].flow_id = 0;
    }
    session1.sessionNum     = sessionNum;
    session1.returnQueue    = 0;
    session1.lcsmHandle     = (LCSM_DEVICE_HANDLE)0;
}

static unsigned nextMpegSection( void )
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    unsigned i = 0;

    while ( session1.section[i].pid != 0 )
    {
        i++;
    }
    if ( i == MAX_MPEG_SECTIONS )
    {
        return RETURN_ERROR;
    }

    session1.section[i].pid = 0xFFFF;

    return i;
}

static void deleteMpegSection( unsigned sectionIndex )
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    if ( sectionIndex >= MAX_MPEG_SECTIONS )
    {
        return;
    }

    session1.section[sectionIndex].pid      = 0;
    session1.section[sectionIndex].flow_id  = 0;
}

static unsigned findSectionByFlowId( unsigned flow_id )
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    unsigned i;
    unsigned sectionCount = 0;

    for ( i=0; i<MAX_MPEG_SECTIONS; i++ )
    {
        if ( session1.section[i].flow_id == flow_id )
        {
            return i;
        }
    }

    // flow_id not found
    return RETURN_ERROR;
}

#if (0) /* Old way to compute len: Save for now (just in case!) Delete later */

// length_field() can vary from 1-3 bytes
// if the most significant byte is 1, then the length is > 1 byte
// if the first byte is 129, then the length is 2 bytes
// if the first byte is 130, then the length is 3 bytes
// returns length in bytes
static unsigned getLengthFieldSize( unsigned char *length )
{
    if ( *length == 0x81 )
    {
        return 2;
    }
    if ( *length == 0x82 )
    {
        return 3;
    }
    return 1;
}
#endif

#ifdef __cplusplus
}
#endif

