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

#include <sys_init.h>


#include <global_event_map.h>
#include <global_queues.h>
#include "poddef.h"
#include <lcsm_log.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <linux/ip.h>
#include <linux/icmp.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

#include <net/if.h>
#include <net/if_arp.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>

#include <sys/stat.h>

#include "mrerrno.h"
#include "dsg-main.h"
#include "ipdirectUnicast-main.h"
#include "utils.h"
#include "link.h"
#include "transport.h"
#include "podhighapi.h"
#include "podlowapi.h"
#include "applitest.h"
#include "session.h"
#include "appmgr.h"
#include "sys_api.h"

#include "utilityMacros.h"
#include "bufParseUtilities.h"
#include "traceStack.h"
#include "vlDsgTunnelParser.h"
#include "ip_types.h"
#include "vlXchanFlowManager.h"
#include "dsgProxyTypes.h"
#include "vlDsgProxyService.h"

#include "cardUtils.h"
#include "core_events.h"
#include "rmf_osal_event.h"
#include "rmf_osal_thread.h"
#include "dsgResApi.h"
#include "ipdirect_unicast_ResApi.h"
#include "xchanResApi.h"
#include <string.h>
#include "cVector.h"

#include <time.h>
#include "rmf_osal_mem.h"
#include "rmf_osal_sync.h"
#include "netUtils.h"
#include "coreUtilityApi.h"
#include <unistd.h>
#include <sys/reboot.h>
#include <linux/reboot.h>
#include <stdlib.h>
#include "rmf_osal_resreg.h"
#include <vlEnv.h>

#include "IPDdownloadManager.h"							/* IPDU Feature */
#include "IPDSocketManager.h"
#include "pod_interface.h"
#include "httpFileFetcher.h"

#ifdef GCC4_XXX
#include <vector>
#else
#include <vector.h>
#endif

using namespace std;

#ifdef GLI
#include "libIBus.h"
#include "sysMgr.h"
#endif

#if USE_SYSRES_MLT
#include "mlt_malloc.h"
#endif

#ifdef ENABLE_SD_NOTIFY
#include <systemd/sd-daemon.h>
#endif

#define __MTAG__ VL_CARD_MANAGER

extern unsigned char GetPodInsertedStatus();


#ifdef __cplusplus
extern "C" {
#endif

// Remove after debugged.
extern void vlMpeosDumpBuffer(rdk_LogLevel lvl, const char *mod, const void * pBuffer, int nBufSize);

// This is the IP Direct Unicast (IPDU) feature main processing thread. The RDK_LOG information is recorded
// in file /opt/logs/ocapri_log.txt.
// Grep file ocapri_log.txt for "IPDU::" to view the high-level APDU sequence between POD and HOST.
//
#define IPDIRECT_UNICAST_STATS_LOG_LEVEL  RDK_LOG_DEBUG

static LCSM_DEVICE_HANDLE   handle = 0;
static USHORT SocketFlowLocalPortNumber = 0;

static pthread_mutex_t          ipdirect_unicastLoopLock;
static int                      loop = 1;
rmf_osal_eventqueue_handle_t    vlg_ipdirect_unicast_msgQ;
static void vl_ipdirect_unicast_task(void * arg);

static VL_DSG_TRAFFIC_STATS      vlg_xchanTrafficStats[VL_XCHAN_FLOW_TYPE_MAX];
static int          vlg_bEnableTrafficStats = 1; // enabled by default

// IPDU Feature Data memebers:
static LCSM_TIMER_HANDLE vlg_IpduDownloadTimerHandle  = 0;
static LCSM_TIMER_HANDLE vlg_IpduUploadTimerHandle  = 0;
static unsigned long vlg_IpduFileTransferPollTime = 500;						// In milliseconds
// Provide two file scope strings to persist the HttpGet and HttpPost Urls for use by the IPDdownloadManager.
// The IPDU feature supports concurrent HTTP Get flow, HttpPost flow, and Socket flow (UDP recv) requests.
// The IPDU feature does NOT support multiple, concurrent flows of the same type. The actual IPDU Interface
// spec precludes this behavior.
static char IpduGetUrl[MAX_URL_STRING_LENGTH] = "";             // Http Recv (Http Get) Flow url
static char *pIpduUrl=NULL;
IPDIRECT_UNICAST_HTTP_GET_PARAMS IpduGetParams;

static char IpduSendUrl[MAX_URL_STRING_LENGTH] = "";            // Http Send (Http Post) Flow url
IPDIRECT_UNICAST_HTTP_GET_PARAMS IpduSendParams;
static LCSM_EVENT_INFO tHttpGetReqEventInfo = {0};
static unsigned char SendFlowPlayload[2048] = {0};


static IPDIRECT_UNICAST_HTTP_GET_PARAMS vlg_pHttpGetParams = {0};
static IPDIRECT_UNICAST_HTTP_GET_PARAMS vlg_pHttpPostParams = {0};
unsigned long vlg_nIpDirectUnicastVctId = 0;
unsigned long vlg_nOldIpDirectUnicastVctId = 0;
int vlg_bIpDirectUnicastVctIdReceived = 0;
// IPDU- The feature was developed and tested with 30 sec timeouts. Typical POD specified timeouts are 60 sec.
// Default to the development timeouts.
// R1.6b IPDU Interface spec added seperate timeouts for the connection phase and actual transfer phase.
int vlg_nIpDirectHttpGetTransactionTimeout = 240;			// Recv flow timeout, HTTP Get transfer phase
int vlg_nIpDirectHttpGetConnectionTimeout = 180;			// Recv flow timeout, HTTP Get connection phase

int vlg_nIpDirectHttpPostTransactionTimeout = 240;		// Send flow timeout, HTTP Post transfer phase
int vlg_nIpDirectHttpPostConnectionTimeout = 180; 		// Send flow timeout, HTTP Post connection phase

FILE * fpInIpduDirect = NULL;                           // ipdu-ntp

void vlMpeosClearSiDatabase();
void vlMpeosNotifyVCTIDChange();

int vlIpDirectUnicastSndApdu2Pod(unsigned short sesnum, unsigned long tag, unsigned short dataLen, unsigned char *data);
ULONG vlGetXChanSession();
ULONG vlGetXChanDsgSessionNumber();
void vlMpeosClearSiDatabase();
void vlMpeosSIPurgeRDDTables();


extern unsigned long persistedDsgVctId;
extern unsigned long vlg_nOldDsgVctId;
extern unsigned long vlg_nDsgVctId;

// ******************* Session Stuff ********************************
//find our session number and are we open?
//CCIF2.0 table 9.3.4 states that only 1 session active at a time
//this makes the implementation simple; we'll use two
//static globals
// below, may need functions to read/write with mutex pretection
static USHORT vlg_nIpDirectUnicastSessionNumber = 0;
unsigned long vlg_nIpDirectUnicast_to_card_FlowId = 0;
unsigned long vlg_nIpDirectUnicast_from_card_FlowId = 0;
unsigned long vlg_nIpDirectUnicast_to_card_SocketFlowId = 0;
extern unsigned long vlg_tcHostIpLastChange;
static int    ipdirect_unicastSessionState  = 0;
static int vlg_ipdirect_unicastDownstreamAcquired = 0;
// above
static ULONG  ResourceId    = 0;
static UCHAR  Tcid          = 0;
static ULONG  tmpResourceId = 0; //tmp for binding
static UCHAR  tmpTcid       = 0; //tmp for binding

#define IPDIRECT_TRANSFER_BLOCK_SIZE    4082 // (4096 - (flow-id[4] + byte-count[2] + current-segment[2] + last-segment[2] + crc[4]))

#define UDP_PROTOCOL_FLAG	0
#define TCP_PROTOCOL_FLAG	1

static int ipdirectUnicast_sendHttpFlowCnf(USHORT sesnum, UCHAR status, UCHAR flowsRemaining, ULONG flowId);

static int ipdirectUnicast_handleHttpRecvFlowReq( USHORT sesnum, UCHAR *apkt, USHORT len );
//static int ipdirectUnicast_handleHttpFlowReq( USHORT sesnum, UCHAR *apkt, USHORT len );
static int ipdirectUnicast_handleHttpSendFlowReq( USHORT sesnum, UCHAR *apkt, USHORT len );
static int ipdirectUnicast_handleHttpGetReq( USHORT sesnum, UCHAR *apkt, USHORT len );
static int ipdirectUnicast_handleSocketFlowReq( USHORT sesnum, UCHAR *apkt, USHORT len );
static int ipdirectUnicast_sendSocketFlowCnf(USHORT sesnum, UCHAR status, UCHAR flowsRemaining, ULONG flowId);
static int ipdirectUnicast_sendHttpGetCnf(USHORT sesnum, USHORT status);
static int ipdirectUnicast_sendHttpPostCnf(USHORT sesnum, USHORT transactionId, USHORT status);
static int ipdirectUnicast_sendDeleteHttpFlowReq(USHORT sesnum, ULONG flowId);
static int ipdirectUnicast_handleDeleteHttpFlowReq( USHORT sesnum, UCHAR *apkt, USHORT len );
static int ipdirectUnicast_sendDeleteHttpFlowCnf( USHORT sesnum, ULONG flowId, UCHAR reason );
static int ipdirectUnicast_handleDeleteHttpFlowCnf( USHORT sesnum, UCHAR *apkt, USHORT len );
static int ipdirectUnicast_sendLostHttpFlowInd(USHORT sesnum, ULONG flowId, UCHAR reason);
static int ipdirectUnicast_handleLostHttpFlowCnf( USHORT sesnum, UCHAR *apkt, USHORT len );
static int ipdirectUnicast_sendDataSegInfo(USHORT sesnum, USHORT nTotalSegments, ULONG nTotalLength);
static int ipdirectUnicast_handleHttpResendSegReq( USHORT sesnum, UCHAR *apkt, USHORT len );
static int ipdirectUnicast_handleHttpDataSegsRcvd( USHORT sesnum, UCHAR *apkt, USHORT len );
static int ipdirectUnicast_handleHttpRecvStatus( USHORT sesnum, UCHAR *apkt, USHORT len );
static int ipdirectUnicast_handleIcnPidNotify( USHORT sesnum, UCHAR *apkt, USHORT len );

static int vlg_ipdirect_get_request_count       = 0;
static int vlg_ipdirect_get_fail_count          = 0;
static int vlg_ipdirect_get_segment_count       = 0;
static int vlg_ipdirect_get_seg_fail_count      = 0;
static int vlg_ipdirect_get_transfer_count      = 0;
static int vlg_ipdirect_get_xfer_fail_count     = 0;
static int vlg_ipdirect_get_byte_count          = 0;

rmf_osal_Mutex vlIpDirectUnicastMainGetThreadLock()
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    static rmf_osal_Mutex lock ; 
    static int i =0;
    if(i==0)
    {		
    	rmf_osal_mutexNew(&lock);
       i =1;		
    }
    return lock;
}

rmf_osal_Mutex vlIpDirectUnicastTunnelGetThreadLock()
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    static rmf_osal_Mutex lock ; 
    static int i =0;
    if(i==0)
    {		
    	rmf_osal_mutexNew(&lock);
       i =1;		
    }
    return lock;
}

void vlIpDirectUnicastUpdateXchanTrafficCounters(VL_XCHAN_FLOW_TYPE eFlowType, int nBytes)
{// this function is not protected as it is called by only one thread (pod ext chnl read)
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    if(vlg_bEnableTrafficStats)
    {
        rmf_osal_mutexAcquire(vlIpDirectUnicastTunnelGetThreadLock());
        {
            int ix = VL_SAFE_ARRAY_INDEX(eFlowType, vlg_xchanTrafficStats);
            vlg_xchanTrafficStats[ix].nPkts     += 1;
            vlg_xchanTrafficStats[ix].nOctets   += nBytes;
            if(0 == vlg_xchanTrafficStats[ix].tFirstPacket)
            {
                vlg_xchanTrafficStats[ix].tFirstPacket = time(NULL);
                vlg_xchanTrafficStats[ix].tLastPacket  = vlg_xchanTrafficStats[ix].tFirstPacket;
            }
            else
            {
                vlg_xchanTrafficStats[ix].tLastPacket = time(NULL);
            }
        }
        rmf_osal_mutexRelease(vlIpDirectUnicastTunnelGetThreadLock());
    }
}

void vlIpDirectUnicastDumpIpDirectUnicastTrafficCountersTask(void * arg)
{
    RDK_LOG(IPDIRECT_UNICAST_STATS_LOG_LEVEL, "LOG.RDK.POD", "%s: Running...\n", __FUNCTION__);
    /* BEGIN: UNIT-TEST
    sleep(90);
    UCHAR aMessage[4096];
    int nMessageLen = 0;
    // New flow req.
    {
        VL_BYTE_STREAM_ARRAY_INST(aMessage, MessageBuf);
        nMessageLen = 0;
        nMessageLen += vlWriteByte(pMessageBuf, 30); // connection timeout.
    }
    {
        UCHAR aAPDU[4096];
        VL_BYTE_STREAM_ARRAY_INST(aAPDU, APDU);
        int nApduLen = 0;
        nApduLen += vlWrite3ByteLong(pAPDU, IPDIRECT_UNICAST_APDU_HTTP_FLOW_REQ);
        nApduLen += vlWriteCcApduLengthField(pAPDU, nMessageLen);
        nApduLen += vlWriteBuffer(pAPDU, aMessage, nMessageLen, VL_BYTE_STREAM_REMAINDER(pAPDU));
        
        ipdirectUnicast_handleHttpFlowReq(vlg_nIpDirectUnicastSessionNumber, aAPDU, nApduLen);
    }
    // Del flow req.
    ipdirectUnicast_sendDeleteHttpFlowReq(vlg_nIpDirectUnicastSessionNumber, vlg_nIpDirectUnicast_to_card_FlowId);
    // Del flow cnf.
    {
        VL_BYTE_STREAM_ARRAY_INST(aMessage, MessageBuf);
        nMessageLen = 0;
        nMessageLen += vlWrite3ByteLong(pMessageBuf, vlg_nIpDirectUnicast_to_card_FlowId); // connection timeout.
        nMessageLen += vlWriteByte(pMessageBuf, IPDIRECT_UNICAST_DELETE_FLOW_RESULT_GRANTED); // connection timeout.
    }
    {
        UCHAR aAPDU[4096];
        VL_BYTE_STREAM_ARRAY_INST(aAPDU, APDU);
        int nApduLen = 0;
        nApduLen += vlWrite3ByteLong(pAPDU, IPDIRECT_UNICAST_APDU_HTTP_DEL_FLOW_CNF);
        nApduLen += vlWriteCcApduLengthField(pAPDU, nMessageLen);
        nApduLen += vlWriteBuffer(pAPDU, aMessage, nMessageLen, VL_BYTE_STREAM_REMAINDER(pAPDU));
        
        ipdirectUnicast_handleDeleteHttpFlowCnf(vlg_nIpDirectUnicastSessionNumber, aAPDU, nApduLen);
    }
    // New flow req.
    {
        VL_BYTE_STREAM_ARRAY_INST(aMessage, MessageBuf);
        nMessageLen = 0;
        nMessageLen += vlWriteByte(pMessageBuf, 30); // connection timeout.
    }
    {
        UCHAR aAPDU[4096];
        VL_BYTE_STREAM_ARRAY_INST(aAPDU, APDU);
        int nApduLen = 0;
        nApduLen += vlWrite3ByteLong(pAPDU, IPDIRECT_UNICAST_APDU_HTTP_FLOW_REQ);
        nApduLen += vlWriteCcApduLengthField(pAPDU, nMessageLen);
        nApduLen += vlWriteBuffer(pAPDU, aMessage, nMessageLen, VL_BYTE_STREAM_REMAINDER(pAPDU));
        
        ipdirectUnicast_handleHttpFlowReq(vlg_nIpDirectUnicastSessionNumber, aAPDU, nApduLen);
    }
    // Lost flow ind.
    ipdirectUnicast_sendLostHttpFlowInd(vlg_nIpDirectUnicastSessionNumber, vlg_nIpDirectUnicast_to_card_FlowId, IPDIRECT_UNICAST_LOST_FLOW_REASON_NET_BUSY_OR_DOWN);
    // Lost flow cnf.
    {
        VL_BYTE_STREAM_ARRAY_INST(aMessage, MessageBuf);
        nMessageLen = 0;
        nMessageLen += vlWrite3ByteLong(pMessageBuf, vlg_nIpDirectUnicast_to_card_FlowId);
        nMessageLen += vlWriteByte(pMessageBuf, IPDIRECT_UNICAST_LOST_FLOW_RESULT_ACKNOWLEDGED);
    }
    {
        UCHAR aAPDU[4096];
        VL_BYTE_STREAM_ARRAY_INST(aAPDU, APDU);
        int nApduLen = 0;
        nApduLen += vlWrite3ByteLong(pAPDU, IPDIRECT_UNICAST_APDU_HTTP_LOST_FLOW_CNF);
        nApduLen += vlWriteCcApduLengthField(pAPDU, nMessageLen);
        nApduLen += vlWriteBuffer(pAPDU, aMessage, nMessageLen, VL_BYTE_STREAM_REMAINDER(pAPDU));
        
        ipdirectUnicast_handleLostHttpFlowCnf(vlg_nIpDirectUnicastSessionNumber, aAPDU, nApduLen);
    }
    
    // Http Get
    {
        VL_BYTE_STREAM_ARRAY_INST(aMessage, MessageBuf);
        nMessageLen = 0;
        unsigned char flags = 0;
        flags |= (IPDIRECT_UNICAST_HTTP_PROCESS_BY_CARD<<2);
        flags |= (IPDIRECT_UNICAST_SERVER_ADDRESS_TYPE_BOTH<<0);
        //flags |= (IPDIRECT_UNICAST_SERVER_ADDRESS_TYPE_NAME<<0);
        nMessageLen += vlWriteByte(pMessageBuf, flags);
        RDK_LOG(IPDIRECT_UNICAST_STATS_LOG_LEVEL, "LOG.RDK.POD", "%s: IPDIRECT_UNICAST_APDU_HTTP_GET_REQ flags = 0x%02X\n", __FUNCTION__, flags);
        
        unsigned char aBytIpV4Address[] = {172, 20, 0, 22};
        nMessageLen += vlWriteBuffer(pMessageBuf, aBytIpV4Address, sizeof(aBytIpV4Address), VL_BYTE_STREAM_REMAINDER(pMessageBuf));
        unsigned char aBytIpV6Address[] = {0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0xf1, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x01,};
        nMessageLen += vlWriteBuffer(pMessageBuf, aBytIpV6Address, sizeof(aBytIpV6Address), VL_BYTE_STREAM_REMAINDER(pMessageBuf));

        //const char szDnsName[] = "http-radd";
        //int nNameLen = strlen(szDnsName);
        //nMessageLen += vlWriteByte(pMessageBuf, nNameLen);
        //nMessageLen += vlWriteBuffer(pMessageBuf, (unsigned char*)szDnsName, nNameLen, VL_BYTE_STREAM_REMAINDER(pMessageBuf));

        nMessageLen += vlWriteShort(pMessageBuf, 8080);
        {
            char szPath[] = "/dell";
            nMessageLen += vlWriteByte(pMessageBuf, strlen(szPath));
            nMessageLen += vlWriteBuffer(pMessageBuf, (unsigned char*)szPath, strlen(szPath), VL_BYTE_STREAM_REMAINDER(pMessageBuf));
        }
        {
            char szFile[] = "Dell_Inspiron_530.pdf";
            nMessageLen += vlWriteByte(pMessageBuf, strlen(szFile));
            nMessageLen += vlWriteBuffer(pMessageBuf, (unsigned char*)szFile, strlen(szFile), VL_BYTE_STREAM_REMAINDER(pMessageBuf));
        }
    }
    {
        UCHAR aAPDU[4096];
        VL_BYTE_STREAM_ARRAY_INST(aAPDU, APDU);
        int nApduLen = 0;
        nApduLen += vlWrite3ByteLong(pAPDU, IPDIRECT_UNICAST_APDU_HTTP_GET_REQ);
        nApduLen += vlWriteCcApduLengthField(pAPDU, nMessageLen);
        nApduLen += vlWriteBuffer(pAPDU, aMessage, nMessageLen, VL_BYTE_STREAM_REMAINDER(pAPDU));
        
        ipdirectUnicast_handleHttpGetReq(vlg_nIpDirectUnicastSessionNumber, aAPDU, nApduLen);
    }
    
    sleep(60);
    // Resend segment.
    {
        VL_BYTE_STREAM_ARRAY_INST(aMessage, MessageBuf);
        nMessageLen = 0;
        nMessageLen += vlWriteShort(pMessageBuf, 3597);
    }
    {
        UCHAR aAPDU[4096];
        VL_BYTE_STREAM_ARRAY_INST(aAPDU, APDU);
        int nApduLen = 0;
        nApduLen += vlWrite3ByteLong(pAPDU, IPDIRECT_UNICAST_APDU_HTTP_RESEND_SEG_REQ);
        nApduLen += vlWriteCcApduLengthField(pAPDU, nMessageLen);
        nApduLen += vlWriteBuffer(pAPDU, aMessage, nMessageLen, VL_BYTE_STREAM_REMAINDER(pAPDU));
        
        ipdirectUnicast_handleHttpResendSegReq(vlg_nIpDirectUnicastSessionNumber, aAPDU, nApduLen);
    }
    // Resend segment.
    {
        VL_BYTE_STREAM_ARRAY_INST(aMessage, MessageBuf);
        nMessageLen = 0;
        nMessageLen += vlWriteShort(pMessageBuf, 3596);
    }
    {
        UCHAR aAPDU[4096];
        VL_BYTE_STREAM_ARRAY_INST(aAPDU, APDU);
        int nApduLen = 0;
        nApduLen += vlWrite3ByteLong(pAPDU, IPDIRECT_UNICAST_APDU_HTTP_RESEND_SEG_REQ);
        nApduLen += vlWriteCcApduLengthField(pAPDU, nMessageLen);
        nApduLen += vlWriteBuffer(pAPDU, aMessage, nMessageLen, VL_BYTE_STREAM_REMAINDER(pAPDU));
        
        ipdirectUnicast_handleHttpResendSegReq(vlg_nIpDirectUnicastSessionNumber, aAPDU, nApduLen);
    }
    // Bad Ack.
    {
        VL_BYTE_STREAM_ARRAY_INST(aMessage, MessageBuf);
        nMessageLen = 0;
        nMessageLen += vlWriteByte(pMessageBuf, IPDIRECT_UNICAST_TRANSFER_STATUS_START_OVER);
    }
    {
        UCHAR aAPDU[4096];
        VL_BYTE_STREAM_ARRAY_INST(aAPDU, APDU);
        int nApduLen = 0;
        nApduLen += vlWrite3ByteLong(pAPDU, IPDIRECT_UNICAST_APDU_HTTP_DATA_SEGS_RCVD);
        nApduLen += vlWriteCcApduLengthField(pAPDU, nMessageLen);
        nApduLen += vlWriteBuffer(pAPDU, aMessage, nMessageLen, VL_BYTE_STREAM_REMAINDER(pAPDU));
        
        ipdirectUnicast_handleHttpDataSegsRcvd(vlg_nIpDirectUnicastSessionNumber, aAPDU, nApduLen);
    }
    
    sleep(60);
    // Resend segment.
    {
        VL_BYTE_STREAM_ARRAY_INST(aMessage, MessageBuf);
        nMessageLen = 0;
        nMessageLen += vlWriteShort(pMessageBuf, 3595);
    }
    {
        UCHAR aAPDU[4096];
        VL_BYTE_STREAM_ARRAY_INST(aAPDU, APDU);
        int nApduLen = 0;
        nApduLen += vlWrite3ByteLong(pAPDU, IPDIRECT_UNICAST_APDU_HTTP_RESEND_SEG_REQ);
        nApduLen += vlWriteCcApduLengthField(pAPDU, nMessageLen);
        nApduLen += vlWriteBuffer(pAPDU, aMessage, nMessageLen, VL_BYTE_STREAM_REMAINDER(pAPDU));
        
        ipdirectUnicast_handleHttpResendSegReq(vlg_nIpDirectUnicastSessionNumber, aAPDU, nApduLen);
    }
    // Good Ack.
    {
        VL_BYTE_STREAM_ARRAY_INST(aMessage, MessageBuf);
        nMessageLen = 0;
        nMessageLen += vlWriteByte(pMessageBuf, IPDIRECT_UNICAST_TRANSFER_STATUS_SUCCESSFUL);
    }
    {
        UCHAR aAPDU[4096];
        VL_BYTE_STREAM_ARRAY_INST(aAPDU, APDU);
        int nApduLen = 0;
        nApduLen += vlWrite3ByteLong(pAPDU, IPDIRECT_UNICAST_APDU_HTTP_DATA_SEGS_RCVD);
        nApduLen += vlWriteCcApduLengthField(pAPDU, nMessageLen);
        nApduLen += vlWriteBuffer(pAPDU, aMessage, nMessageLen, VL_BYTE_STREAM_REMAINDER(pAPDU));
        
        ipdirectUnicast_handleHttpDataSegsRcvd(vlg_nIpDirectUnicastSessionNumber, aAPDU, nApduLen);
    }
    //END: UNIT-TEST */
    while(vlg_bEnableTrafficStats)
    {
        //int iStat = 0;
        rmf_osal_threadSleep(15*1000, 0);
        if(!rdk_dbg_enabled("LOG.RDK.POD", IPDIRECT_UNICAST_STATS_LOG_LEVEL)) continue;
        
        RDK_LOG(IPDIRECT_UNICAST_STATS_LOG_LEVEL, "LOG.RDK.POD", "IPDIRECT stats: gets = %d, failed gets = %d, xfers = %d, failed xfers = %d, segments = %d, failed segments = %d, bytes = %d.\n",
                vlg_ipdirect_get_request_count, vlg_ipdirect_get_fail_count,
                vlg_ipdirect_get_transfer_count, vlg_ipdirect_get_xfer_fail_count,
                vlg_ipdirect_get_segment_count, vlg_ipdirect_get_seg_fail_count,
                vlg_ipdirect_get_byte_count);
    }
}

void vlIpDirectUnicastMonitorIPAddressTask(void * arg)
{
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    int rc;
    while(1)
    {
        if( access( "/tmp/ip_address_changed", F_OK ) != -1 )
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s IP Address changed/lease expired\n", __FUNCTION__);
            SocketFlowLocalPortNumber = 0;
	    // Send ipdirectUnicast_sendLostHttpFlowInd APDU
            ipdirectUnicast_sendLostHttpFlowInd(vlg_nIpDirectUnicastSessionNumber, vlg_nIpDirectUnicast_to_card_SocketFlowId, IPDIRECT_UNICAST_LOST_FLOW_REASON_LEASE_EXPIRED);
            KillSocketFlowThread();
            // Delete file from temp
            rc = remove ("/tmp/ip_address_changed");
            if (rc) {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s Failed to delete /tmp/ip_address_changed file\n", __FUNCTION__);
            }
        }
        else
        {
            // file doesn't exist
            sleep (60);
        }
    }
}

void vlIpDirectUnicastDumpIpDirectUnicastStats()
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    const char * pszFileName = "/tmp/ipdirect_unicast_stats.txt";
    FILE * fp = fopen(pszFileName, "w");
    if(NULL == fp)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: Unable to open file '%s'.\n", __FUNCTION__,  pszFileName);
        return;
    }
    else
    {
        fprintf(fp, "IPDIRECT stats: gets = %d, failed gets = %d, xfers = %d, failed xfers = %d, segments = %d, failed segments = %d, bytes = %d.\n",
        vlg_ipdirect_get_request_count, vlg_ipdirect_get_fail_count,
        vlg_ipdirect_get_transfer_count, vlg_ipdirect_get_xfer_fail_count,
        vlg_ipdirect_get_segment_count, vlg_ipdirect_get_seg_fail_count,
        vlg_ipdirect_get_byte_count);
        fclose(fp);
    }
}

USHORT vlGetIpDirectUnicastSessionNumber(unsigned short nSession)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    if(0 != vlg_nIpDirectUnicastSessionNumber)
    {
        return vlg_nIpDirectUnicastSessionNumber;
    }

    return nSession;
}

int  vlGetIpDirectUnicastFlowId()
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    return vlg_nIpDirectUnicast_to_card_FlowId;
}

unsigned long vlIpDirectUnicastGetVctId()
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    return vlg_nIpDirectUnicastVctId;
}

void  SetSocketFlowLocalPortNumber( int PortNumber)
{
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    SocketFlowLocalPortNumber = PortNumber;
}

void  vlSetIpDirectUnicastFlowId(unsigned long nFlowId)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    vlg_nIpDirectUnicast_to_card_FlowId = nFlowId;
}

bool IPDIRECT_UNICAST_CheckIfFlagIsSet(const char * pszFlag)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    struct stat st;
    bool ret=false;

    if((0 == stat(pszFlag, &st) && (0 != st.st_ino)))
    {
         ret=true;
         RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: Flag '%s' is set.\n",__FUNCTION__, pszFlag);
    }
    else
    {
         ret=false;
         RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: Flag '%s' is not set.\n",__FUNCTION__, pszFlag);
    }
    return ret;
}

USHORT lookupIpDirectUnicastSessionValue(void)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    if (  (ipdirect_unicastSessionState == 1) && (vlg_nIpDirectUnicastSessionNumber))
        return vlg_nIpDirectUnicastSessionNumber;

    return 0;
}
// Inline functions

static inline void setLoop(int cont)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    pthread_mutex_lock(&ipdirect_unicastLoopLock);
    if (!cont)
    {
        loop = 0;
    }
    else
    {
        loop = 1;
    }
    pthread_mutex_unlock(&ipdirect_unicastLoopLock);
}
// Sub-module exit point
int ipdirect_unicast_stop()
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    //RDK_LOG(RDK_LOG_TRACE7 "LOG.RDK.POD", "ipdirect_unicast_stop: Stopping IPDIRECT_UNICAST Resource: Session = %d\n", vlg_nIpDirectUnicastSessionNumber);
    if ( (ipdirect_unicastSessionState) && (vlg_nIpDirectUnicastSessionNumber))
    {
       //don't reject but also request that the pod close (perhaps) this orphaned session
       AM_CLOSE_SS(vlg_nIpDirectUnicastSessionNumber);
    }

    // Terminate message loop
    setLoop(0);

    // Wait for message processing thread to join
#if 0    
    if (pthread_join(ipdirect_unicastThread, NULL) != 0)
    {
        MDEBUG (DPM_ERROR, "ERROR:ca: pthread_join failed.\n");
    }
#endif
    handle = 0;
    // ipdu-ntp
    // Upon exiting IPDU mode remove the file that indicates we are in IPDU mode.
    if(NULL != fpInIpduDirect)
    {
        fclose(fpInIpduDirect);
        fpInIpduDirect = NULL;
        int err = remove("/tmp/inIpDirectUnicastMode");
        if(!err)
    		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: deleted inIpDirectUnicastMode file\n", __FUNCTION__);
        else
    		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s: unable to delete inIpDirectUnicastMode file\n", __FUNCTION__);
    }

	return 1;
}


// ##################### Execution Space ##################################
// below executes in the caller's execution thread space.
// this may or may-not be in IPDIRECT_UNICAST thread and APDU processing Space.
// can occur before or after ipdirect_unicast_start and ipdirect_unicastProcThread
//
//this was triggered from POD via appMgr bases on ResourceID.
//appMgr has translated the ResourceID to that of IPDIRECT_UNICAST.
void
AM_OPEN_SS_REQ_IPDIRECT_UNICAST(ULONG RessId, UCHAR Tcid)

{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    if (ipdirect_unicastSessionState != 0)
    {
        //we're already open; hence reject.
        AM_OPEN_SS_RSP( SESS_STATUS_RES_BUSY, RessId, Tcid);
        //may want to
        //don't reject but also request that the pod close (perhaps) this orphaned session
        //AM_CLOSE_SS(caSessionNumber);

        tmpResourceId = 0;
        tmpTcid = 0;
        return;
    }

    tmpResourceId = RessId;
    tmpTcid = Tcid;
    AM_OPEN_SS_RSP( SESS_STATUS_OPENED, RessId, Tcid);
    return;
}

void vlIpDirectUnicastResourceInit()
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
}

void vlIpDirectUnicastResourceRegister()
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
}

void vlIpDirectUnicastResourceCleanup()
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    rmf_osal_mutexAcquire(vlIpDirectUnicastMainGetThreadLock());
    {
        vlg_nIpDirectUnicastVctId = 0;
        vlg_nOldIpDirectUnicastVctId = 0;
        vlg_bIpDirectUnicastVctIdReceived = 0;
        vlg_ipdirect_unicastDownstreamAcquired = 0;
        memset(&vlg_xchanTrafficStats,0,sizeof(vlg_xchanTrafficStats));
        vlSetIpDirectUnicastFlowId(0);
    }
    rmf_osal_mutexRelease(vlIpDirectUnicastMainGetThreadLock());
}

//spec say we can't fail here, but we will test and reject anyway
//verify that this open is in response to our
//previous AM_OPEN_SS_RSP...SESS_STATUS_OPENED
void AM_SS_OPENED_IPDIRECT_UNICAST(USHORT SessNumber, ULONG lResourceId, UCHAR Tcid)

{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    if (ipdirect_unicastSessionState != 0)
    {//we're already open; hence reject.
        AM_OPEN_SS_RSP( SESS_STATUS_RES_BUSY, lResourceId, Tcid);
        tmpResourceId = 0;
        tmpTcid = 0;
        return;
    }
    if (lResourceId != tmpResourceId)
    {//wrong binding hence reject.
        AM_OPEN_SS_RSP( SESS_STATUS_RES_BUSY, lResourceId, Tcid);
        tmpResourceId = 0;
        tmpTcid = 0;
        return;
    }
    if (Tcid != tmpTcid)
    {//wrong binding hence reject.
        AM_OPEN_SS_RSP( SESS_STATUS_RES_BUSY, lResourceId, Tcid);
        tmpResourceId = 0;
        tmpTcid = 0;
        return;
    }

    rmf_osal_mutexAcquire(vlIpDirectUnicastMainGetThreadLock());
    {
        ResourceId      = lResourceId;
        Tcid            = Tcid;
        vlg_nIpDirectUnicastSessionNumber = SessNumber;
        ipdirect_unicastSessionState  = 1;
        vlg_nIpDirectUnicastVctId = 0;
        vlg_nOldIpDirectUnicastVctId = 0;
        vlg_bIpDirectUnicastVctIdReceived = 0;
        vlg_ipdirect_unicastDownstreamAcquired = 0;
        memset(&vlg_xchanTrafficStats,0,sizeof(vlg_xchanTrafficStats));
    }
    rmf_osal_mutexRelease(vlIpDirectUnicastMainGetThreadLock());
    //not sure we send this here, I believe its assumed to be successful
    //AM_OPEN_SS_RSP( SESS_STATUS_OPENED, ResourceId, Tcid);
    //RDK_LOG(RDK_LOG_TRACE7, "LOG.RDK.POD", "ipdirect_unicast_stop: IPDIRECT_UNICAST Resource Opened: Session = %d\n", vlg_nIpDirectUnicastSessionNumber);
    return;
}

//after the fact, info-ing that closure has occured.
void AM_SS_CLOSED_IPDIRECT_UNICAST(USHORT SessNumber)

{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    if (  (ipdirect_unicastSessionState)	&& (vlg_nIpDirectUnicastSessionNumber)
            && (vlg_nIpDirectUnicastSessionNumber != SessNumber )
       )
    {
        //don't reject but also request that the pod close (perhaps) this orphaned session
        AM_CLOSE_SS(vlg_nIpDirectUnicastSessionNumber);
    }

    SocketFlowLocalPortNumber = 0;

    //RDK_LOG(RDK_LOG_TRACE7, "LOG.RDK.POD", "ipdirect_unicast_stop: IPDIRECT_UNICAST Resource Closed: Session = %d\n", vlg_nIpDirectUnicastSessionNumber);

    rmf_osal_mutexAcquire(vlIpDirectUnicastMainGetThreadLock());
    {
        ResourceId      = 0;
        Tcid            = 0;
        tmpResourceId   = 0;
        tmpTcid         = 0;
        vlg_nIpDirectUnicastSessionNumber = 0;
        ipdirect_unicastSessionState  = 0;

        vlIpDirectUnicastResourceCleanup();
    }
    rmf_osal_mutexRelease(vlIpDirectUnicastMainGetThreadLock());
}// ##################### Execution Space ##################################
// ************************* Session Stuff **************************

// Sub-module entry point
int ipdirect_unicast_start(LCSM_DEVICE_HANDLE h)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    rmf_osal_ThreadId ThreadIdTC;
    if (h < 1)
    {
       // MDEBUG (DPM_ERROR, "ERROR:ca: Invalid LCSM device handle.\n");
        // This failure should be logged by the application manager
        return -EINVAL;
    }
    // Assign handle
    handle = h;
    // Flush queue
 //   lcsm_flush_queue(cardresLCSMHandle, POD_CA_QUEUE); Hannah

    //Initialize CA's loop mutex
    if (pthread_mutex_init(&ipdirect_unicastLoopLock, NULL) != 0)
    {
       // MDEBUG (DPM_ERROR, "ERROR:cardres: Unable to initialize mutex.\n");
        return EXIT_FAILURE;
    }

    setLoop(1);

    rmf_osal_mutexAcquire(vlIpDirectUnicastMainGetThreadLock());
    {
        // Initialize all configs to default values
        rmf_osal_mutexAcquire(vlIpDirectUnicastTunnelGetThreadLock());
        {
            memset(&vlg_xchanTrafficStats,0,sizeof(vlg_xchanTrafficStats));
        }
        rmf_osal_mutexRelease(vlIpDirectUnicastTunnelGetThreadLock());

        // initialize all vars to default values
        vlg_nIpDirectUnicastVctId = 0;
        vlg_nOldIpDirectUnicastVctId = 0;
        vlg_bIpDirectUnicastVctIdReceived = 0;
        vlg_ipdirect_unicastDownstreamAcquired = 0;
 
        vlIpDirectUnicastResourceInit();
    }
    rmf_osal_mutexRelease(vlIpDirectUnicastMainGetThreadLock());

    /*
    int nRegistrationId = vlIpDirectUnicastRegisterClient(vlIpDirectUnicastSampleAppCallbackFunc, 0,
                        IPDIRECT_UNICAST_CLIENT_ID_ENC_TYPE_APP, 100);
    RDK_LOG(RDK_LOG_TRACE8, "LOG.RDK.POD", "ipdirect_unicast_start: vlIpDirectUnicastRegisterClient returned registration id = %d\n", nRegistrationId);
    */
    // Enable counters by default // if(rdk_dbg_enabled("LOG.RDK.POD", IPDIRECT_UNICAST_STATS_LOG_LEVEL))
    vlg_bEnableTrafficStats = 1;
       
    rmf_osal_eventqueue_create ( (const uint8_t *)"ipdirect_unicast_msgQ", &vlg_ipdirect_unicast_msgQ);
    rmf_osal_threadCreate(vl_ipdirect_unicast_task, NULL, 0, 0, &ThreadIdTC, "vl_ipdirect_unicast_task");
    rmf_osal_threadCreate(vlIpDirectUnicastDumpIpDirectUnicastTrafficCountersTask, NULL, 0, 0, &ThreadIdTC, "vlIpDirectUnicastDumpIpDirectUnicastTrafficCountersTask"); 
    rmf_osal_threadCreate(vlIpDirectUnicastMonitorIPAddressTask, NULL, 0, 0, &ThreadIdTC, "vlIpDirectUnicastMonitorIPAddressTask");

    return EXIT_SUCCESS;
}

void ipdirect_unicast_init( )
{
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    SocketFlowLocalPortNumber = 0;
    REGISTER_RESOURCE(M_IPDIRECT_UNICAST_ID, POD_IPDIRECT_UNICAST_QUEUE, 1);  // (resourceId, queueId, maxSessions )
}

void ipdirect_unicast_test( )
{
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", " Test ##### ipdirect_unicast_test \n");
}

int vlIpDirectUnicastNotifyHostIpAcquired()
{
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    return 1;
}

int vlIpDirectUnicastNotifySocketFailure( IPDIRECT_UNICAST_LOST_FLOW_REASON reason )
{
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s %d\n", __FUNCTION__, reason);

    // clear the socket flow port number to allow the thread to be recreated by the MCARD.
	SocketFlowLocalPortNumber = 0;

    // send the lost flow message to the MCARD
    return ipdirectUnicast_sendLostHttpFlowInd(vlg_nIpDirectUnicastSessionNumber, VL_DSG_BASE_HOST_UDP_SOCKET_FLOW_ID,
                                               reason);
}

int vlIpDirectUnicastHandleEcmMessage(VL_DSG_MESSAGE_TYPE eMessage)
{
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    switch(eMessage)
    {
        default:
        {
            // nothing to do
        }
        break;
    }

    return 1;
}

char * vlGetAddressFamilyName(int family);
char * vlGetSockTypeName(int type);
char * vlGetIpProtocolName(int protocol);

USHORT ipdirectHttpGet(IPDIRECT_UNICAST_HTTP_GET_PARAMS* pFlow, vector<unsigned char> & aByteBuffer, vector<unsigned char> & aByteContent)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    long sock = 0;
    char szHost[VL_DNS_ADDR_SIZE] = "";
    unsigned char globalIpAddr[VL_IPV6_ADDR_SIZE];
    char szPort[VL_IPV6_ADDR_SIZE] = "";
    char szErrorStr[IPDIRECT_UNICAST_MAX_STR_SIZE] = "";

    struct addrinfo hints, *res, *res0;
    int error = 0;

    strcpy(szHost, "");
    strcpy(szPort, "");
    memset(globalIpAddr, 0, sizeof(globalIpAddr));

    VL_HOST_IP_INFO hostIpInfo;
    vlXchanGetHostIpTypeInfo(VL_HOST_IP_IF_TYPE_DSG, 0, &hostIpInfo);

    // in case BOTH was specified pre-select address type based on operating mode.
    if(IPDIRECT_UNICAST_SERVER_ADDRESS_TYPE_BOTH == pFlow->eRemoteAddressType)
    {
        if(VL_XCHAN_IP_ADDRESS_TYPE_IPV4 == hostIpInfo.ipAddressType)
        {
            pFlow->eRemoteAddressType = IPDIRECT_UNICAST_SERVER_ADDRESS_TYPE_IPV4;
        }
        if(VL_XCHAN_IP_ADDRESS_TYPE_IPV6 == hostIpInfo.ipAddressType)
        {
            pFlow->eRemoteAddressType = IPDIRECT_UNICAST_SERVER_ADDRESS_TYPE_IPV6;
        }
    }    
    
    memset(&hints, 0, sizeof(hints));
    
    // specify the family
    switch(pFlow->eRemoteAddressType)
    {
        case IPDIRECT_UNICAST_SERVER_ADDRESS_TYPE_NAME:
        {
            hints.ai_family = PF_UNSPEC;
        }
        break;

        case IPDIRECT_UNICAST_SERVER_ADDRESS_TYPE_IPV4:
        {
            hints.ai_family = AF_INET;
            hints.ai_flags |= AI_NUMERICHOST;
        }
        break;

        case IPDIRECT_UNICAST_SERVER_ADDRESS_TYPE_IPV6:
        {
            hints.ai_family = AF_INET6;
            hints.ai_flags |= AI_NUMERICHOST;
        }
        break;
            
        default:
        {
        }
        break;
    }
    
    // specify the protocol
    hints.ai_socktype  = SOCK_STREAM;
    hints.ai_protocol |= IPPROTO_TCP;
    // specify the port
  //  sprintf(szPort, "%u", pFlow->nRemotePortNum);
      snprintf(szPort,sizeof(szPort), "%u", pFlow->nRemotePortNum);
    // specify the host
    switch(pFlow->eRemoteAddressType)
    {
        case IPDIRECT_UNICAST_SERVER_ADDRESS_TYPE_NAME:
        {
            strcpy(szHost, (char*)(pFlow->dnsAddress));
        }
        break;

        case IPDIRECT_UNICAST_SERVER_ADDRESS_TYPE_IPV4:
        {
            snprintf(szHost, sizeof(szHost), "%u.%u.%u.%u",
                    pFlow->ipAddress[0],
                    pFlow->ipAddress[1],
                    pFlow->ipAddress[2],
                    pFlow->ipAddress[3]);
        }
        break;

        case IPDIRECT_UNICAST_SERVER_ADDRESS_TYPE_IPV6:
        {
            snprintf(szHost, sizeof(szHost), "%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X",
                    pFlow->ipV6address[ 0], pFlow->ipV6address[ 1],
                    pFlow->ipV6address[ 2], pFlow->ipV6address[ 3],
                    pFlow->ipV6address[ 4], pFlow->ipV6address[ 5],
                    pFlow->ipV6address[ 6], pFlow->ipV6address[ 7],
                    pFlow->ipV6address[ 8], pFlow->ipV6address[ 9],
                    pFlow->ipV6address[10], pFlow->ipV6address[11],
                    pFlow->ipV6address[12], pFlow->ipV6address[13],
                    pFlow->ipV6address[14], pFlow->ipV6address[15]);
        }
        break;

        default:
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","ipdirectHttpGet: Unexpected address type (%d) requested\n", pFlow->eRemoteAddressType);
        }
        break;
    }

    // resolve names or IP addresses
    error = getaddrinfo(szHost, szPort, &hints, &res0);
    if (error)
    {
        perror("ipdirectHttpGet: getaddrinfo");
        switch(pFlow->eRemoteAddressType)
        {
            case IPDIRECT_UNICAST_SERVER_ADDRESS_TYPE_NAME:
            {
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","ipdirectHttpGet: Remote Address '%s' could not be resolved\n", szHost);
            }
            break;

            case IPDIRECT_UNICAST_SERVER_ADDRESS_TYPE_IPV4:
            case IPDIRECT_UNICAST_SERVER_ADDRESS_TYPE_IPV6:
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","ipdirectHttpGet: Invalid remote addr '%s'\n", szHost);
            }
            break;
            
            default:
            {
            }
            break;
        }
        //ipdirectUnicast_sendHttpGetCnf(pFlow->sesnum, IPDIRECT_UNICAST_FLOW_RESULT_DENIED_DNS_FAILED);
        pFlow->bInUse = 0;
        return IPDIRECT_UNICAST_FLOW_RESULT_DENIED_DNS_FAILED;
    }
    

    int iAddr = 0;
    int nIPv4addressCount = 0;
    int nIPv6addressCount = 0;
    for (res = res0; NULL != res; res = res->ai_next)
    {
        iAddr++;
        char szIpAddr[VL_IPV6_ADDR_STR_SIZE];
        const char * pszIpAddrType = "IPv4";
        if(AF_INET6 == res->ai_family)
        {
            pszIpAddrType = "IPv6";
            nIPv6addressCount++;
        }
        else
        {
            nIPv4addressCount++;
        }
        getnameinfo(res->ai_addr, res->ai_addrlen, szIpAddr, sizeof(szIpAddr), NULL, 0, NI_NUMERICHOST);
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","ipdirectHttpGet: Remote Address '%s' resolves to %d: (%s Address, '%s').\n", szHost, iAddr, pszIpAddrType, szIpAddr);
    }
    if(VL_XCHAN_IP_ADDRESS_TYPE_IPV4 == hostIpInfo.ipAddressType)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","ipdirectHttpGet: STB currently has IPv4 address '%s'.\n", hostIpInfo.szIpAddress);
    }
    if(VL_XCHAN_IP_ADDRESS_TYPE_IPV6 == hostIpInfo.ipAddressType)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","ipdirectHttpGet: STB currently has IPv6 address '%s'.\n", hostIpInfo.szIpV6GlobalAddress);
    }
    
    if((nIPv6addressCount > 0) && (nIPv4addressCount <= 0) && (VL_XCHAN_IP_ADDRESS_TYPE_IPV4 == hostIpInfo.ipAddressType))
    {
        pFlow->bInUse = 0;
        freeaddrinfo(res0);
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","ipdirectHttpGet: STB currently has IPv4 address '%s'. Returning IPDIRECT_UNICAST_FLOW_RESULT_DENIED_NO_IPV6.\n", hostIpInfo.szIpAddress);
        //ipdirectUnicast_sendHttpGetCnf(pFlow->sesnum, IPDIRECT_UNICAST_FLOW_RESULT_DENIED_NO_IPV6);
        return IPDIRECT_UNICAST_FLOW_RESULT_DENIED_NO_IPV6;
    }
    
    pFlow->bInUse = 1;
    pFlow->flowid = 0;
    
    int nConnectFailure = 0;
    time_t tStartTime = time(NULL);
    
    // create network I/O
    sock = -1;
    iAddr = 0;
    for (res = res0; NULL != res; res = res->ai_next)
    {
        iAddr++;
        char szIpAddr[VL_IPV6_ADDR_STR_SIZE];
        const char * pszIpAddrType = "IPv4";
        if(AF_INET6 == res->ai_family) pszIpAddrType = "IPv6";
        getnameinfo(res->ai_addr, res->ai_addrlen, szIpAddr, sizeof(szIpAddr), NULL, 0, NI_NUMERICHOST);
        
        if((VL_XCHAN_IP_ADDRESS_TYPE_IPV4 == hostIpInfo.ipAddressType) && (AF_INET6 == res->ai_family))
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","ipdirectHttpGet: Skipping entry %d: (%s Address, '%s') for '%s' as STB is operating in IPv4 mode.\n", iAddr, pszIpAddrType, szIpAddr, szHost);
            continue;
        }
        if((VL_XCHAN_IP_ADDRESS_TYPE_IPV6 == hostIpInfo.ipAddressType) && (AF_INET == res->ai_family))
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","ipdirectHttpGet: Skipping entry %d: (%s Address, '%s') for '%s' as STB is operating in IPv6 mode.\n", iAddr, pszIpAddrType, szIpAddr, szHost);
            continue;
        }
        
        //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","ipdirectHttpGet: creating socket for family %d, type %d, protocol %d\n", res->ai_family, res->ai_socktype, res->ai_protocol);
        sock = socket(res->ai_family, res->ai_socktype,
                   res->ai_protocol);
        if (sock < 0)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","ipdirectHttpGet: create socket for '%s' failed\n", szHost);
            continue;
        }
        if( sock == 0 ) 
        {
   	 	RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD",
   	 	"\n\n\n%s: Closed file descriptor 0 !!\n\n\n\n",__FUNCTION__);
        }
        
        // Jul-13-2009: Removed discriminatory impl. based on value of (pFlow->eProtocol)
        //              This change is based on clarification on TCP flows by CableLabs
        //              As per CableLabs bind and connect are required for both UDP and TCP flows.
        // Feb-06-2009: Changed impl to meet requirements of TC 170.2
        // bind
        //Sunil - Added for having separate mechanism for IPV6 and IPV4 for broadcom
        if (res->ai_family == AF_INET6)
        {
            struct sockaddr_in6 serv_addr;
            memset(&(serv_addr),0,sizeof(serv_addr));
            // specify the port
            // Feb-18-2009: Commented: Using auto selection of UDP port: //if(0 == pFlow->nLocalPortNum) pFlow->nLocalPortNum = pFlow->nRemotePortNum;
            /* Code for address family */
            serv_addr.sin6_family = AF_INET6;

            /* IP address of the host */
            serv_addr.sin6_addr = in6addr_any;
            /* Port number */
            serv_addr.sin6_port = htons((u_short)pFlow->nLocalPortNum);

            // Feb-06-2009: Changed impl to meet requirements of TC 170.2
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","ipdirectHttpGet: binding to port %d for '%s' ('%s'), port:%s\n    using F:%d (%s), T:%d (%s), P:%d (%s)\n",
                    pFlow->nLocalPortNum, szHost, szIpAddr, szPort,
                    res->ai_family, vlGetAddressFamilyName(res->ai_family),
                    res->ai_socktype, vlGetSockTypeName(res->ai_socktype),
                    res->ai_protocol, vlGetIpProtocolName(res->ai_protocol));
            /* Binding a socket to an address of the current host and port number */
            if (bind(sock, (struct sockaddr *) &serv_addr,
                sizeof(serv_addr)) < 0)
            {
                if(EADDRINUSE == errno)
                {   // Apr-15-2009: do not allow reuse of of udp ports
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","ipdirectHttpGet : bind on socket failed error: %s \n", strerror_r(errno, szErrorStr, sizeof(szErrorStr)));
                    // create flow failed
                    close(sock);
                    sock = -1;
                    pFlow->bInUse = 0;
                    freeaddrinfo(res0);
                    //ipdirectUnicast_sendHttpGetCnf(pFlow->sesnum, IPDIRECT_UNICAST_FLOW_RESULT_DENIED_PORT_IN_USE);
                    return IPDIRECT_UNICAST_FLOW_RESULT_DENIED_PORT_IN_USE;
                }
                else
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","ipdirectHttpGet : bind on socket failed error: %s \n", strerror_r(errno, szErrorStr, sizeof(szErrorStr)));
                    close(sock);
                    sock = -1;
                    continue;
                }
            }
            
            // Feb-18-2009: Using auto selection of UDP/TCP port 
            if(0 == pFlow->nLocalPortNum)
            {
                struct sockaddr_in6 sai;
                int8_t ret = 0;			
                socklen_t socklen = sizeof(sai);
                ret = getsockname(sock, (struct sockaddr *)&sai, &socklen);
                if( ret == 0 )
                {
                    pFlow->nLocalPortNum = ntohs(sai.sin6_port);
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","ipdirectHttpGet: Selected nLocalPort = %d\n", pFlow->nLocalPortNum);
                }
                else
                {
                  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","ipdirectHttpGet: Selected nLocalPort Failed\n");
                }
            }
        }
        else
        {
            struct sockaddr_in serv_addr;
            memset(&serv_addr, 0, sizeof(serv_addr));
            // specify the port
            // Feb-18-2009: Commented: Using auto selection of UDP port: //if(0 == pFlow->nLocalPortNum) pFlow->nLocalPortNum = pFlow->nRemotePortNum;
            /* Code for address family */
            serv_addr.sin_family = AF_INET;
            
            /* IP address of the host */
            serv_addr.sin_addr.s_addr = INADDR_ANY;
            /* Port number */
            serv_addr.sin_port = htons((u_short)pFlow->nLocalPortNum);
            
            
            // Feb-06-2009: Changed impl to meet requirements of TC 170.2
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","ipdirectHttpGet: binding to port %d for '%s' ('%s'), port:%s\n    using F:%d (%s), T:%d (%s), P:%d (%s)\n",
                    pFlow->nLocalPortNum, szHost, szIpAddr, szPort,
                    res->ai_family, vlGetAddressFamilyName(res->ai_family),
                    res->ai_socktype, vlGetSockTypeName(res->ai_socktype),
                    res->ai_protocol, vlGetIpProtocolName(res->ai_protocol));
            /* Binding a socket to an address of the current host and port number */
            if (bind(sock, (struct sockaddr *) &serv_addr,
                sizeof(serv_addr)) < 0)
            {
                if(EADDRINUSE == errno)
                {   // Apr-15-2009: do not allow reuse of of udp ports
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","ipdirectHttpGet : bind on socket failed error: %s \n", strerror_r(errno, szErrorStr, sizeof(szErrorStr)));
                    // create flow failed
                    close(sock);
                    sock = -1;
                    pFlow->bInUse = 0;
                    freeaddrinfo(res0);
                    //ipdirectUnicast_sendHttpGetCnf(pFlow->sesnum, IPDIRECT_UNICAST_FLOW_RESULT_DENIED_PORT_IN_USE);
                    return IPDIRECT_UNICAST_FLOW_RESULT_DENIED_PORT_IN_USE;
                }
                else
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","ipdirectHttpGet : bind on socket failed error: %s \n", strerror_r(errno, szErrorStr, sizeof(szErrorStr)));
                    close(sock);
                    sock = -1;
                    continue;
                }
            }
                
            // Feb-18-2009: Using auto selection of UDP/TCP port 
            if(0 == pFlow->nLocalPortNum)
            {
                struct sockaddr_in sai;
                int8_t ret = 0;
                socklen_t socklen = sizeof(sai);
                ret = getsockname(sock, (struct sockaddr *)&sai, &socklen);
                if ( ret == 0 ) 
                {
                    pFlow->nLocalPortNum = ntohs(sai.sin_port);
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","ipdirectHttpGet: Selected nLocalPort = %d\n", pFlow->nLocalPortNum);
                }
                else
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","ipdirectHttpGet: Selected nLocalPort Failed\n");
                }
            }
         }    
        // Feb-06-2009: Changed impl to meet requirements of TC 170.2
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","ipdirectHttpGet: %s: connecting to '%s' ('%s') port:%s\n    using F:%d (%s), T:%d (%s), P:%d (%s)\n",
                "TCP",
                szHost, szIpAddr, szPort,
                res->ai_family, vlGetAddressFamilyName(res->ai_family),
                res->ai_socktype, vlGetSockTypeName(res->ai_socktype),
                res->ai_protocol, vlGetIpProtocolName(res->ai_protocol));

        if(pFlow->transactionTimeout > 0)
        {
            struct timeval timeout = {pFlow->transactionTimeout, 0};
            if( setsockopt (sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof (timeout)) < 0 )
            {
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s():setsockopt SO_SNDTIMEO error:  %s \n", __FUNCTION__,strerror_r(errno, szErrorStr, sizeof(szErrorStr)));

            }
        }
        if (connect(sock, res->ai_addr, res->ai_addrlen) < 0)
        {
            nConnectFailure = errno;
            sleep(1); // skew the time forward by a second.
            time_t tEndTime = time(NULL);
            if((pFlow->transactionTimeout > 0) && ((tEndTime - tStartTime) >= pFlow->transactionTimeout))
            {
                nConnectFailure = ETIMEDOUT;
            }
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","ipdirectHttpGet: connect to '%s' ('%s') failed. errno = '%s'.\n", szHost, szIpAddr, strerror_r(nConnectFailure, szErrorStr, sizeof(szErrorStr)));
            close(sock);
            sock = -1;
            continue;
        }
        // Jul-13-2009: Removed discriminatory impl. based on value of (pFlow->eProtocol)
        //              This change is based on clarification on TCP flows by CableLabs
        //              As per CableLabs bind and connect are required for both UDP and TCP flows.
        /* connect succeeded */

        pFlow->nRemoteAddrLen = res->ai_addrlen;
        memcpy(pFlow->remote_addr, res->ai_addr, pFlow->nRemoteAddrLen);
        //vlMpeosDumpBuffer(RDK_LOG_DEBUG, "LOG.RDK.POD", res->ai_addr, res->ai_addrlen);
        // make a note of what we will be using
        hints.ai_family = res->ai_family;
        
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","ipdirectHttpGet: Established http connection to '%s' with entry %d: (%s Address, '%s').\n", szHost, iAddr, pszIpAddrType, szIpAddr);
        break;  /* bind & connect succeeded */
    }

    freeaddrinfo(res0);
    
    // check if the flow has timed-out
    if(sock <= 0)
    {
        pFlow->bInUse = 0;
        switch(nConnectFailure)
        {
            case ETIMEDOUT:
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","ipdirectHttpGet: flow creation failed or timed-out\n");
                return IPDIRECT_UNICAST_GET_STATUS_NETWORK_CONNECT_TIMEOUT_ERROR;
            }
            break;
            
            default:
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","ipdirectHttpGet: sock is NULL\n");
                // create flow failed
                return IPDIRECT_UNICAST_FLOW_RESULT_DENIED_TCP_FAILED;
            }
            break;
        }
    }
    
    if(-1 != sock )
    {
        pFlow->socket = sock;
    }
    pFlow->bInUse = 1;
    
    // Connect is successful...
    {
        char szGetCmd[256] = "";
        const char * pszIpv6Left = "";
        const char * pszIpv6Right = "";
        const char * pszSlash = "";
        if((VL_XCHAN_IP_ADDRESS_TYPE_IPV6 == hostIpInfo.ipAddressType) && (IPDIRECT_UNICAST_SERVER_ADDRESS_TYPE_NAME != pFlow->eRemoteAddressType))
        {
            pszIpv6Left = "[";
            pszIpv6Right = "]";
        }
        if('/' != pFlow->pathName[(pFlow->nPathNameLen)-1])
        {
            pszSlash = "/";
        }
        snprintf(szGetCmd, sizeof(szGetCmd), "GET %s%s%s HTTP/1.1\r\nHost: %s%s%s\r\n\r\n", pFlow->pathName, pszSlash, pFlow->fileName, pszIpv6Left, szHost, pszIpv6Right);
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: Requesting... '%s'\n", __FUNCTION__, szGetCmd);
        
        if(send(sock, szGetCmd, strlen(szGetCmd), MSG_NOSIGNAL) < 0)
        {
            perror("ipdirectHttpGet: send");
            //ipdirectUnicast_sendHttpGetCnf(pFlow->sesnum, IPDIRECT_UNICAST_GET_STATUS_NETWORK_CONNECT_TIMEOUT_ERROR);
            if(-1 != sock ) close(sock);
            return IPDIRECT_UNICAST_GET_STATUS_NETWORK_CONNECT_TIMEOUT_ERROR;
        }
        
        unsigned char buf[1024+512]; // receive PDU size worth of data if possible.
        int nTotalReceived = 0;
        aByteBuffer.clear();
        aByteContent.clear();
        while(1)
        {
            int nReceived = recv(sock, buf, sizeof(buf), 0);
            
            if(nReceived > 0)
            {
                aByteBuffer.insert(aByteBuffer.end(), buf, buf+nReceived);
                nTotalReceived += nReceived;
            }
            else
            {
                if(0 == nTotalReceived)
                {
                    perror("ipdirectHttpGet: recv");
                    if(-1 != sock ) close(sock);
                    return IPDIRECT_UNICAST_GET_STATUS_NETWORK_READ_TIMEOUT_ERROR;
                }
                else
                {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","ipdirectHttpGet: Received %d bytes.\n", nTotalReceived);
                    const char * pszHeader = (const char*)(aByteBuffer.data());
                    const char * pszRespCode = strchr(pszHeader, ' ');
                    if(NULL != pszRespCode)
                    {
                        const char * pszBodyStart1 = strstr(pszHeader, "\n\n");
                        if(NULL != pszBodyStart1) pszBodyStart1+=2;
                        const char * pszBodyStart2 = strstr(pszHeader, "\r\n\r\n");
                        if(NULL != pszBodyStart2) pszBodyStart2+=4;
                        
                        const char * pszBodyStart = NULL;
                        if(NULL != pszBodyStart1) pszBodyStart = pszBodyStart1;
                        if(NULL != pszBodyStart2) pszBodyStart = pszBodyStart2;
                        if((NULL != pszBodyStart1) && (pszBodyStart1 < pszBodyStart2)) pszBodyStart = pszBodyStart1;
                        
                        if(NULL != pszBodyStart)
                        {
                            int nHeaderLength = (pszBodyStart - pszHeader);
                            int nAvailableContentLength = (nTotalReceived - nHeaderLength);
                            aByteContent.assign(pszBodyStart, pszBodyStart+nAvailableContentLength);
                            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","ipdirectHttpGet: Header length is........... %9d bytes.\n", nHeaderLength);
                            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","ipdirectHttpGet: Available content length is %9d bytes.\n", nAvailableContentLength);
                        }
                        else
                        {
                            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","ipdirectHttpGet: Could not locate content.\n");
                        }
                        
                        if(-1 != sock ) close(sock);
                        return atoi(pszRespCode);
                    }
                    else
                    {
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","ipdirectHttpGet: Could not locate response code.\n");
                        if(-1 != sock ) close(sock);
                        return IPDIRECT_UNICAST_GET_STATUS_NONE;
                    }
                }
            }
        }
    }
}

static void ipdirect_unicast_serialize_event(ULONG ulTag, unsigned long nParam)
{
	 RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
	 rmf_osal_event_params_t event_params = { 0, (void *)nParam, ulTag, NULL };
	 rmf_osal_event_handle_t event_handle;
		
    if(0 != vlg_ipdirect_unicast_msgQ)
    {
        rmf_osal_event_create(RMF_OSAL_EVENT_CATEGORY_POD, vlg_nIpDirectUnicastSessionNumber, &event_params, &event_handle);
        if ( rmf_osal_eventqueue_add( vlg_ipdirect_unicast_msgQ, event_handle ) != RMF_SUCCESS )
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s: rmf_osal_eventqueue_add Failed \n", __FUNCTION__);
        }	
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s : vlg_ipdirect_unicast_msgQ is %d.\n", __FUNCTION__, vlg_ipdirect_unicast_msgQ);
    }
    
    return;
}

void ipdirect_send_segment_to_card(ULONG flowid, vector<unsigned char> & aByteContent, int iSegment)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    // Check for content.
    int nContentLength = aByteContent.size();
    if(nContentLength > 0)
    {
        static unsigned char aBuf[4096];
        // Prepare for transfer.
        int nTotalSegments = (nContentLength / IPDIRECT_TRANSFER_BLOCK_SIZE);
        int nTrailerSize = (nContentLength % IPDIRECT_TRANSFER_BLOCK_SIZE);
        if(nTrailerSize > 0) nTotalSegments++;
        
        unsigned char * pBuffer = aByteContent.data();
        pBuffer += (iSegment * IPDIRECT_TRANSFER_BLOCK_SIZE);
        int nLastSegment = (nTotalSegments-1);
        
        int nSegmentSize = IPDIRECT_TRANSFER_BLOCK_SIZE;
        if(iSegment > nLastSegment)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s: iSegment = %d > nLastSegment = %d.\n", __FUNCTION__, iSegment, nLastSegment);
            return;
        }
        else if(iSegment == nLastSegment)
        {
            if(nTrailerSize > 0) nSegmentSize = nTrailerSize;
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: nTrailerSize = %d, last nSegmentSize = %d.\n", __FUNCTION__, nTrailerSize, nSegmentSize);
        }
        int nByteCount   = 2+2+nSegmentSize+4; // Last + Current + Segment + CRC.

        VL_BYTE_STREAM_ARRAY_INST(aBuf, WriteBuf);
        int nFinalLen = 0;
        nFinalLen += vlWriteShort (pWriteBuf, nByteCount  );
        nFinalLen += vlWriteShort (pWriteBuf, nLastSegment);
        nFinalLen += vlWriteShort (pWriteBuf, iSegment    );
        nFinalLen += vlWriteBuffer(pWriteBuf, pBuffer, nSegmentSize, VL_BYTE_STREAM_REMAINDER(pWriteBuf));
        unsigned long nCRC = Crc32Mpeg2(CRC_ACCUM, aBuf,  2+2+2+nSegmentSize); // ByteCount + Last + Current + Segment.
        nFinalLen += vlWriteLong  (pWriteBuf, nCRC        );

        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "IPDU::Host-->pod: send data segment to card CRC 0x%x.\n", nCRC);

        POD_STACK_SEND_ExtCh_Data(aBuf, nFinalLen, flowid);
        vlg_ipdirect_get_segment_count++;
        vlg_ipdirect_get_byte_count += nSegmentSize;
    }
}

void ipdirect_send_full_non_segmented_content_to_card(ULONG flowid, vector<unsigned char> & aByteContent)
{
  RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);

  // Check for content assumes content length is always less than IPDIRECT_TRANSFER_BLOCK_SIZE.
  int nContentLength = aByteContent.size();
  if(nContentLength > 0)
  {
    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "content to card length = %d. \n", nContentLength);
    // Notify transfer params.
    static unsigned char aBuf[4096];
    // Prepare for transfer.
    unsigned char * pBuffer = aByteContent.data();

    VL_BYTE_STREAM_ARRAY_INST(aBuf, WriteBuf);
    int nFinalLen = 0;
    int i;
    nFinalLen += vlWriteBuffer(pWriteBuf, pBuffer, nContentLength, VL_BYTE_STREAM_REMAINDER(pWriteBuf));

    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "content to card nFinalLen = %d flowid 0x%x. \n", nFinalLen, flowid);
    vlMpeosDumpBuffer(RDK_LOG_INFO, "LOG.RDK.POD", aBuf, nFinalLen);
    POD_STACK_SEND_ExtCh_Data(aBuf, nFinalLen, flowid);
    vlg_ipdirect_get_transfer_count++;
  }
}

void ipdirect_send_full_content_to_card(ULONG flowid, vector<unsigned char> & aByteContent)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);

    // Check for content.
    int nContentLength = aByteContent.size();
    if(nContentLength > 0)
    {
        // Prepare for transfer.
        int nTotalSegments = (nContentLength / IPDIRECT_TRANSFER_BLOCK_SIZE);
        int nTrailerSize = (nContentLength % IPDIRECT_TRANSFER_BLOCK_SIZE);
        if(nTrailerSize > 0) nTotalSegments++;
        
        // Notify transfer params.
        ipdirectUnicast_sendDataSegInfo(vlg_nIpDirectUnicastSessionNumber, nTotalSegments, nContentLength);
        int iSegment = 0;
        for(iSegment = 0; iSegment < nTotalSegments; iSegment++)
        {
            ipdirect_send_segment_to_card(flowid, aByteContent, iSegment);
        }
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: Finished transferring nTotalSegments = %d, for nContentLength = %d.\n", __FUNCTION__, nTotalSegments, nContentLength);
        vlg_ipdirect_get_transfer_count++;
    }
}

static void vl_ipdirect_unicast_task(void * arg)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);

    rmf_osal_event_handle_t event_handle;
    rmf_osal_event_category_t event_category;
    uint32_t event_type;
    rmf_osal_event_params_t  event_params;
    IPDIRECT_UNICAST_HTTP_GET_PARAMS* pGetParams = NULL;
    vector<unsigned char> aByteBuffer;
    vector<unsigned char> aByteContent;
    vector<unsigned char> aUdpSocketByteContent;
    
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s : Running...\n", __FUNCTION__);
    
    // Since the IPDU feature has been implemented as a runtime feature this task will start for IPD and non-IPD
    // deployments. To keep this task benign in non-IPD environments defer any initialization until we
    // actually go into IPD mode as indicated by the ICN Pid nootify.

    while(1)
    {
        if(RMF_SUCCESS ==  rmf_osal_eventqueue_get_next_event( vlg_ipdirect_unicast_msgQ,
					&event_handle, &event_category,
					&event_type, &event_params))
        {
			RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s:  new event dequeued... %u \n", __FUNCTION__, event_type);
			if (POD_UDP_SOCKET_DATA == event_type)
            {
              RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "IPDU::Host:POD_UDP_SOCKET_DATA event recvd.\n");
              const UCHAR *sock_buf;
              int sock_buf_size;
              // UDP data sent by the udp socket task.  Now send to the cable card
              RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s: Received UDP socket data. \n", __FUNCTION__);
              sock_buf = (UCHAR *)event_params.data;
              sock_buf_size = (int)event_params.data_extension;
              RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "Received %d UDP bytes. \n", sock_buf_size);
              vlMpeosDumpBuffer(RDK_LOG_INFO, "LOG.RDK.POD", sock_buf, sock_buf_size);
              aUdpSocketByteContent.clear();
              aUdpSocketByteContent.assign(sock_buf, sock_buf+sock_buf_size);
              //ipdirect_send_full_content_to_card(vlg_nIpDirectUnicast_to_card_SocketFlowId, aUdpSocketByteContent);
              ipdirect_send_full_non_segmented_content_to_card(vlg_nIpDirectUnicast_to_card_SocketFlowId, aUdpSocketByteContent);
            }
			else if (POD_IPDU_RECV_FLOW_DATA == event_type)
            {
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "IPDU::Host:POD_IPDU_RECV_FLOW_DATA event recvd.\n");
				// This event receives the http send flow playload data from the vlXchanFlowManager as received over the
				// extended channel. This is the data that must be Posted to the IpduSendUrl.
				int httpResponse;
				unsigned int payloadSize;
				IPDdownloadMgrRetCode retCode;
				UCHAR sendFlowStatus = IPDIRECT_UNICAST_FLOW_RESULT_GRANTED;
                UCHAR* rcvBuf = (UCHAR *)event_params.data;
                USHORT nSize = event_params.data_extension;
				std::string strPayload;
				RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: POD_IPDU_RECV_FLOW_DATA: nSize=%u .\n", __FUNCTION__, nSize);
				// Extract fields that are not part of the payload data.
				//	_________________________________________________
				//	|ipd_http_send flow id | transaction_id | Payload|
				//	|________________________________________________|
				// The recv'd buffer has the 4 byte flow ID already removed.
				// Therefore just strip off the 2 byte Transaction ID to isolate the buffer.

				VL_BYTE_STREAM_INST(NewFlowRequest, ParseBuf, rcvBuf, nSize);
				// ULONG ulsendFlowId   = vlParseLong(pParseBuf);
				USHORT transactionId = vlParseShort(pParseBuf);
                IpduSendParams.transactionId = transactionId;             // Must persist the ID for the asynchronous
                                                                          // Post flow confirmation.
				payloadSize = nSize - sizeof(USHORT);
			    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: Isolate Payload : Total Size=%u .\n", __FUNCTION__, nSize);
			    //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: *** ulsendFlowId=%u .\n", __FUNCTION__, ulsendFlowId);
			    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: *** transactionId=%u .\n", __FUNCTION__, transactionId);
			    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: *** payloadSize=%u .\n", __FUNCTION__, payloadSize);
				memset(SendFlowPlayload, 0, sizeof(SendFlowPlayload));
				// Isolate just the payload buffer.
			    vlParseBuffer(pParseBuf, SendFlowPlayload, payloadSize, sizeof(SendFlowPlayload));
				strPayload.clear();
				strPayload.append((char *)SendFlowPlayload, payloadSize);

                SetFileUploadTimeout(vlg_nIpDirectHttpPostTransactionTimeout);
                SetFileUploadConnectionTimeout(vlg_nIpDirectHttpPostConnectionTimeout);
				retCode = ProcessApduHttpPost(IpduSendUrl, strPayload, &httpResponse);
				if (IPD_DM_NO_ERROR != retCode)
				{
					RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s:  ProcessApduHttpPost returned error %u.\n", __FUNCTION__, retCode);
				}
				if(IsSynchronousTransferMode())
				{
					// If using the blocking transfer mode for the IPDU Download manager can send confirmation upon return.
					// Else must wait to confirm until download is complete.
					RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:  Sending IPDIRECT_UNICAST_APDU_HTTP_POST_CNF, Http Response= %u\n", __FUNCTION__, httpResponse);
                  ipdirectUnicast_sendHttpPostCnf(vlg_nIpDirectUnicastSessionNumber, transactionId, httpResponse);
                  if(retCode != IPD_DM_NO_ERROR)
                  {
                    TerminateDownload(IpduSendUrl);       // Cleanup
                  }
				}
				else
				{
					// Running in non-blocking transfer and must poll for upload complete.
					RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:  running in non-blocking transfer mode.\n", __FUNCTION__);
					vlIpduFileTransferProgressTimer(true, POD_IPDU_UPLOAD_TIMER);
				}
            }
			else if (POD_IPDU_DOWNLOAD_TIMER == event_type)
            {
                  RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "POD_IPDU_DOWNLOAD_TIMER: Download timer-tic.\n");
	                        vlg_IpduDownloadTimerHandle  = 0;									// Timer expired so clear out handle.
                  // If the timer fired that means an asynchronous transfer was started and must check status.
                  IPDdownloadMgrRetCode retCode = CheckTransferComplete(IpduGetUrl);
                  if (IPD_DM_BUSY == retCode )
                  {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "IPDU Transfer in progress.\n");
                    // Restart polling timer
                    vlIpduFileTransferProgressTimer(true, POD_IPDU_DOWNLOAD_TIMER);
                  }
                  else
                  {
                    ServiceHttpGetResponse(IpduGetUrl, &IpduGetParams);
                  }
            }
			else if (POD_IPDU_UPLOAD_TIMER == event_type)
            {
                  int httpResponse;
                  RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "POD_IPDU_UPLOAD_TIMER: Upload timer-tic.\n");
					        vlg_IpduUploadTimerHandle  = 0;									// Timer expired so clear out handle.
                  // If the timer fired that means an asynchronous transfer was started and must check status.
                  IPDdownloadMgrRetCode retCode = CheckTransferComplete(IpduSendUrl);
                  if (IPD_DM_BUSY == retCode )
                  {
                      RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "IPDU Transfer in progress.\n");
                      // Restart polling timer
                    vlIpduFileTransferProgressTimer(true, POD_IPDU_UPLOAD_TIMER);
                  }
                  else
                  {
                    ServiceHttpPostResponse(IpduSendUrl, &IpduSendParams);
                  }
            }
			else
			{
		          IPDIRECT_UNICAST_APDU_TAG eTag   = (IPDIRECT_UNICAST_APDU_TAG)(event_params.data_extension);
		          unsigned long nParam = (unsigned long)event_params.data;
		          switch(eTag)
		          {
									case IPDIRECT_UNICAST_APDU_ICN_PID_NOTIFY:
		              {
											uint16_t icnpid = nParam;
											RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:  eTag= IPDIRECT_UNICAST_APDU_ICN_PID_NOTIFY ICNPID = %d\n", __FUNCTION__, icnpid);
											HAL_Save_Icn_Pid(&icnpid);
                                            // ipdu-ntp
                                            // The ICN PID is sent from the card to host whne entering the IPDU mode.
                                            // The IPDU task uses a temp file that can be used to signal the system that
                                            // the STB is in IPDU mode. The file should be removed if transition out of 
                                            // IPDU mode.
                                            fpInIpduDirect = fopen("/tmp/inIpDirectUnicastMode", "w+");
                                            if(NULL != fpInIpduDirect)
                                            {
											    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: inIpDirectUnicastMode file created\n", __FUNCTION__);
                                            }
                                            else
                                            {
											    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s: unable to create inIpDirectUnicastMode file, errno= %d\n", __FUNCTION__, errno);
                                            }
		              }
		              break;

									case IPDIRECT_UNICAST_APDU_HTTP_RECV_FLOW_REQ:
		              {
											RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:  eTag= IPDIRECT_UNICAST_APDU_HTTP_RECV_FLOW_REQ\n", __FUNCTION__);
		              }
		              break;

									case IPDIRECT_UNICAST_APDU_HTTP_SEND_FLOW_REQ:
		              {
											RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:  eTag= IPDIRECT_UNICAST_APDU_HTTP_SEND_FLOW_REQ\n", __FUNCTION__);
											int httpResponse;
											IPDdownloadMgrRetCode retCode;
											UCHAR sendFlowStatus = IPDIRECT_UNICAST_FLOW_RESULT_GRANTED;
		                  if(NULL != pGetParams)
		                  {
		                      RDK_LOG(RDK_LOG_WARN, "LOG.RDK.POD", "WARN: %s : Switching to new Send Flow request while previous POST is in progress.\n", __FUNCTION__, eTag);
		                  }
		                  pGetParams = (IPDIRECT_UNICAST_HTTP_GET_PARAMS*)(nParam);
		                  if(NULL != pGetParams)
		                  {
													memset(IpduSendUrl, 0, sizeof(IpduSendUrl));						// Reset url string
													// Save url for use when payload is received.
													strcpy(IpduSendUrl, pGetParams->pathName);
                          // After sending HttpFlowCnf back to the card, the card will send the file data that is to be uploaded.
                          // The data is received via the POD_IPDU_RECV_FLOW_DATA event.
													ipdirectUnicast_sendHttpFlowCnf(pGetParams->sesnum, sendFlowStatus, 0, pGetParams->flowid);
                          IpduSendParams = *pGetParams;               // Must save off request params so they are available
                                                                      // when the download is complete 
											}											
		              }
		              break;

									case IPDIRECT_UNICAST_APDU_HTTP_FLOW_CNF:
		              {
											RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:  eTag= IPDIRECT_UNICAST_APDU_HTTP_FLOW_CNF\n", __FUNCTION__);
		              }
		              break;

									case IPDIRECT_UNICAST_APDU_SOCKET_FLOW_REQ:
		              {
                                unsigned long Port = (unsigned long)event_params.data;
											RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:  eTag= IPDIRECT_UNICAST_APDU_SOCKET_FLOW_REQ\n", __FUNCTION__);
                                ProcessApduSocketFlowReq(Port);
		              }
		              break;

									case IPDIRECT_UNICAST_APDU_SOCKET_FLOW_CNF:
		              {
											RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:  eTag= IPDIRECT_UNICAST_APDU_SOCKET_FLOW_CNF\n", __FUNCTION__);
		              }
		              break;

									case IPDIRECT_UNICAST_APDU_HTTP_DEL_FLOW_REQ:
		              {
											RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:  eTag= IPDIRECT_UNICAST_APDU_HTTP_DEL_FLOW_REQ\n", __FUNCTION__);
		              }
		              break;

									case IPDIRECT_UNICAST_APDU_HTTP_DEL_FLOW_CNF:
		              {
											RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:  eTag= IPDIRECT_UNICAST_APDU_HTTP_DEL_FLOW_CNF\n", __FUNCTION__);
		                  if(NULL != pGetParams)
		                  {
		                      //ProcessApduFlowDeleteCnf(char* cUrl, int* status);				// IPDU Feature
		                  }
		              }
		              break;

									case IPDIRECT_UNICAST_APDU_HTTP_LOST_FLOW_IND:
		              {
											RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:  eTag= IPDIRECT_UNICAST_APDU_HTTP_LOST_FLOW_IND\n", __FUNCTION__);
		              }
		              break;

									case IPDIRECT_UNICAST_APDU_HTTP_LOST_FLOW_CNF:
		              {
											RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:  eTag= IPDIRECT_UNICAST_APDU_HTTP_LOST_FLOW_CNF\n", __FUNCTION__);
		              }
		              break;

		              case IPDIRECT_UNICAST_APDU_HTTP_GET_REQ:
		              {
											int httpResponse;
											IPDdownloadMgrRetCode retCode;
											RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:  eTag= IPDIRECT_UNICAST_APDU_HTTP_GET_REQ\n", __FUNCTION__);
		                  if(NULL != pGetParams)
		                  {
		                      RDK_LOG(RDK_LOG_WARN, "LOG.RDK.POD", "WARN: %s : Switching to new HTTP GET request while previous GET is in progress.\n", __FUNCTION__, eTag);
		                  }
		                  pGetParams = (IPDIRECT_UNICAST_HTTP_GET_PARAMS*)(nParam);
		                  if(NULL != pGetParams)
		                  {
                          if(!IsSystemTimeAvailable() && pGetParams->isSiDataRequest)
                          {
                            // System time is not yet available and Si Data is being requested, respond with error
													  RDK_LOG(RDK_LOG_WARN, "LOG.RDK.POD", "%s:  Sending IPDIRECT_UNICAST_APDU_HTTP_GET_CNF, Http Response= %u\n", __FUNCTION__, HTTP_RESPONSE_ON_NO_STT);
			                        ipdirectUnicast_sendHttpGetCnf(vlg_nIpDirectUnicastSessionNumber, HTTP_RESPONSE_ON_NO_STT);

                          }
                          else
                          {
													  RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:  Use GetParams to assemble URL.\n", __FUNCTION__);
													  char* url;
													  url = AssembleHttpGetUrl(pGetParams);
                            SetFileDownloadTimeout(vlg_nIpDirectHttpGetTransactionTimeout);
                            SetFileDownloadConnectionTimeout(vlg_nIpDirectHttpGetConnectionTimeout);
													  retCode = ProcessApduHttpGetReq(url);
													  if(IsSynchronousTransferMode())
													  {
														  // If using the blocking transfer mode for the IPDU Download manager can send confirmation upon return.
														  // Else must wait to confirm until download is complete.
                              ServiceHttpGetResponse(url, pGetParams);
													  }
													  else
													  {
														  // Running in non-blocking transfer and must poll for download complete.
                              IpduGetParams = *pGetParams;                // Must save off request params so they are available
                                                                          // when the download is complete 
														  RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:  running in non-blocking transfer mode.\n", __FUNCTION__);
														  vlIpduFileTransferProgressTimer(true, POD_IPDU_DOWNLOAD_TIMER);
													  }
                         }
                      }
		              }
		              break;

									case IPDIRECT_UNICAST_APDU_HTTP_GET_CNF:
		              {
											RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:  eTag= IPDIRECT_UNICAST_APDU_HTTP_GET_CNF\n", __FUNCTION__);
		                  if(NULL != pGetParams)
		                  {
		                      //ProcessApduHttpGetCnf(char* cUrl, int* status);				// IPDU Feature
		                  }
		              }
		              break;

									case IPDIRECT_UNICAST_APDU_HTTP_POST_CNF:
		              {
											RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:  eTag= IPDIRECT_UNICAST_APDU_HTTP_POST_CNF\n", __FUNCTION__);
				              if(NULL != pGetParams)
				              {
					                  //ProcessApduHttpPost(char* cUrl, char* outputFile, int* status);				// IPDU Feature
				              }
		              }
		              break;

									case IPDIRECT_UNICAST_APDU_HTTP_DATA_SEG_INFO:
		              {
											RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:  eTag= IPDIRECT_UNICAST_APDU_HTTP_DATA_SEG_INFO\n", __FUNCTION__);
		                  if(NULL != pGetParams)
		                  {
		                      //ProcessApduHttpRecvDataSegInfo(char* cUrl, int* status);				// IPDU Feature
		                  }
		              }
		              break;

		              case IPDIRECT_UNICAST_APDU_HTTP_RESEND_SEG_REQ:
		              {
                      // IPDU The IPDIRECT_UNICAST_APDU_HTTP_RESEND_SEG_REQ APDU syntax has no way to identify or associate
                      // the http get url for which the resnd is being requested. This is either a missing syntax filed or there
                      // is no real plan to support multiple simultaneous connections. 
                      // Therefore the HTTP_RESEND_SEG_REQ must be sent prior to request a new HTTP_GET_REQ.
											IPDdownloadMgrRetCode retCode;
											const unsigned char* file ;
											unsigned int filesize ;
											aByteContent.clear();
											retCode = GetDownloadedFile(IpduGetUrl, &file, &filesize);
											if(IPD_DM_NO_ERROR == retCode)
											{
												// Now bridge the new IPDU Download manager code to existing framework.
												aByteContent.assign(file, file+filesize);
  		                  int iSegmentToResend = nParam;
  											RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:  eTag= IPDIRECT_UNICAST_APDU_HTTP_RESEND_SEG_REQ, resend segment %d\n", __FUNCTION__, iSegmentToResend);
		                    if(NULL != pGetParams)
		                    {
		                        ipdirect_send_segment_to_card(pGetParams->flowid, aByteContent, iSegmentToResend);
		                    }
		                    vlg_ipdirect_get_seg_fail_count++;
                      }
											else
											{
												RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s:  Failed to access downloaded file for Seg Resend.\n", __FUNCTION__);
											}
		              }
		              break;

									case IPDIRECT_UNICAST_APDU_HTTP_RECV_STATUS:
		              {
											RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:  eTag= IPDIRECT_UNICAST_APDU_HTTP_RECV_STATUS\n", __FUNCTION__);
		              }
		              break;

		              default:
		              {
		                  RDK_LOG(RDK_LOG_WARN, "LOG.RDK.POD", "WARN: %s : Receive Unsupported POD_APDU msg 0x%06X\n", __FUNCTION__, eTag);
		              }
		              break;
		          }
						} // end else
            rmf_osal_event_delete(event_handle);
        }
        else
        {
            rmf_osal_threadSleep(100, 0);
        }
    }
}

int ipdirectUnicast_sendSocketFlowCnf(USHORT sesnum, UCHAR status, UCHAR flowsRemaining, ULONG flowId)
{
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);

    USHORT maxPDU = VL_XCHAN_SOCKET_MAX_PDU_SIZE;
    UCHAR aMessage[32];
    VL_BYTE_STREAM_ARRAY_INST(aMessage, WriteBuf);
    int nFinalLen = 0;
    unsigned char globalIpAddr[VL_IPV6_ADDR_SIZE];
    VL_HOST_IP_INFO hostIpInfo;

    nFinalLen += vlWriteByte(pWriteBuf, status);
    nFinalLen += vlWriteByte(pWriteBuf, flowsRemaining);
    if(IPDIRECT_UNICAST_FLOW_RESULT_GRANTED == status)
    {
        nFinalLen += vlWrite3ByteLong(pWriteBuf, flowId);

        nFinalLen += vlWriteShort(pWriteBuf, maxPDU);

        memset(globalIpAddr, 0, sizeof(globalIpAddr));
        vlXchanGetHostIpTypeInfo(VL_HOST_IP_IF_TYPE_DSG, 0, &hostIpInfo);

        if(VL_XCHAN_IP_ADDRESS_TYPE_IPV4 == hostIpInfo.ipAddressType)
        {
            memcpy(globalIpAddr+12, hostIpInfo.aBytIpAddress, sizeof(hostIpInfo.aBytIpAddress));
        }
        else if(VL_XCHAN_IP_ADDRESS_TYPE_IPV6 == hostIpInfo.ipAddressType)
        {
            memcpy(globalIpAddr, hostIpInfo.aBytIpV6GlobalAddress, sizeof(hostIpInfo.aBytIpV6GlobalAddress));
        }

        // write IP address and options
        nFinalLen += vlWriteBuffer(pWriteBuf, globalIpAddr, VL_IPV6_ADDR_SIZE, sizeof(aMessage)-nFinalLen);
        // currently no options to report
        nFinalLen += vlWriteByte(pWriteBuf, 0);

    }

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:: status = %d, flowid = 0x%06X, len = %d\n",  __FUNCTION__, status, flowId, nFinalLen);
    return vlIpDirectUnicastSndApdu2Pod(sesnum, IPDIRECT_UNICAST_APDU_SOCKET_FLOW_CNF, nFinalLen, aMessage);
}

int ipdirectUnicast_sendHttpFlowCnf(USHORT sesnum, UCHAR status, UCHAR flowsRemaining, ULONG flowId)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);

    UCHAR aMessage[32];
    VL_BYTE_STREAM_ARRAY_INST(aMessage, WriteBuf);
    int nFinalLen = 0;

    nFinalLen += vlWriteByte(pWriteBuf, status);
    nFinalLen += vlWriteByte(pWriteBuf, flowsRemaining);
    if(IPDIRECT_UNICAST_FLOW_RESULT_GRANTED == status)
    {
        nFinalLen += vlWrite3ByteLong(pWriteBuf, flowId);
    }
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:: status = %d, flowid = 0x%06X, len = %d\n",  __FUNCTION__, status, flowId, nFinalLen);
    return vlIpDirectUnicastSndApdu2Pod(sesnum, IPDIRECT_UNICAST_APDU_HTTP_FLOW_CNF, nFinalLen, aMessage);
}

int ipdirectUnicast_handleHttpRecvFlowReq( USHORT sesnum, UCHAR *apkt, USHORT len )
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    int timeout; // Timeouts for curl should not be 0 as that means never timeout.
                 // To prevent Cable Card from from hanging the PodMgr prevent a timeout value of 0.
    VL_BYTE_STREAM_INST(NewFlowRequest, ParseBuf, apkt, len);
    ULONG ulTag   = vlParse3ByteLong(pParseBuf);
    ULONG nLength = vlParseCcApduLengthField(pParseBuf);
    timeout = vlParseByte(pParseBuf);
    if(timeout != 0)
      vlg_nIpDirectHttpGetTransactionTimeout = timeout;

    timeout = vlParseByte(pParseBuf);
    if(timeout != 0)
      vlg_nIpDirectHttpGetConnectionTimeout = timeout;           // R1.6b timeouts

    IPDIRECT_UNICAST_UNUSED_VAR(ulTag);
    IPDIRECT_UNICAST_UNUSED_VAR(nLength);

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:: vlg_nIpDirectHttpGetTransactionTimeout = %d\n",  __FUNCTION__, vlg_nIpDirectHttpGetTransactionTimeout);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:: vlg_nIpDirectHttpGetConnectionTimeout = %d\n",  __FUNCTION__, vlg_nIpDirectHttpGetConnectionTimeout);
    vlg_nIpDirectUnicast_to_card_FlowId = VL_DSG_BASE_HOST_IPDIRECT_UNICAST_RECV_FLOW_ID;
    return ipdirectUnicast_sendHttpFlowCnf(sesnum, IPDIRECT_UNICAST_FLOW_RESULT_GRANTED, 0, vlg_nIpDirectUnicast_to_card_FlowId);
}

int ipdirectUnicast_handleHttpSendFlowReq( USHORT sesnum, UCHAR *apkt, USHORT len )
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    int timeout; // Timeouts for curl should not be 0 as that means never timeout.
                 // To prevent Cable Card from from hanging the PodMgr prevent a timeout value of 0.
    VL_BYTE_STREAM_INST(NewFlowRequest, ParseBuf, apkt, len);
    ULONG ulTag   = vlParse3ByteLong(pParseBuf);
    ULONG nLength = vlParseCcApduLengthField(pParseBuf);
    timeout = vlParseByte(pParseBuf);
    if(timeout != 0)
      vlg_nIpDirectHttpPostTransactionTimeout = timeout;

    timeout = vlParseByte(pParseBuf);
    if(timeout != 0)
      vlg_nIpDirectHttpPostConnectionTimeout = timeout;

    IPDIRECT_UNICAST_UNUSED_VAR(ulTag);
    IPDIRECT_UNICAST_UNUSED_VAR(nLength);

    vlg_nIpDirectUnicast_from_card_FlowId = VL_DSG_BASE_HOST_IPDIRECT_UNICAST_SEND_FLOW_ID;

    IPDIRECT_UNICAST_HTTP_GET_PARAMS * pStruct = &vlg_pHttpPostParams;
    memset(pStruct, 0, sizeof(*pStruct));
    pStruct->flowid             = vlg_nIpDirectUnicast_from_card_FlowId;
    pStruct->sesnum             = vlg_nIpDirectUnicastSessionNumber;

    //pStruct->eRemoteAddressType = (IPDIRECT_UNICAST_SERVER_ADDRESS_TYPE)((nFlags>>0)&0x3);
		pStruct->eRemoteAddressType = IPDIRECT_UNICAST_SERVER_ADDRESS_TYPE_IPV4;

    pStruct->transactionTimeout    = vlg_nIpDirectHttpPostTransactionTimeout;
    pStruct->connTimeout           = vlg_nIpDirectHttpPostConnectionTimeout;
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: vlg_nIpDirectHttpPostTransactionTimeout        = %d.\n", __FUNCTION__, pStruct->transactionTimeout);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: vlg_nIpDirectHttpPostConnectionTimeout        = %d.\n", __FUNCTION__, pStruct->connTimeout);
		// Extract the Post URl (URl of the server to send payload to). 
    pStruct->nPathNameLen = vlParseByte(pParseBuf);
    vlParseBuffer(pParseBuf, (unsigned char*)pStruct->pathName   , pStruct->nPathNameLen  , sizeof(pStruct->pathName ));
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: nPathNameLen       = %3d, pathName = '%s'\n", __FUNCTION__, pStruct->nPathNameLen, pStruct->pathName);

    // File name not used
    pStruct->nFileNameLen = 0;
    pStruct->fileName[0]=0;

		/* Post event on vl_ipdirect_unicast_task msg Q. */
    ipdirect_unicast_serialize_event(ulTag, (unsigned long)(pStruct));
    return APDU_IPDIRECT_UNICAST_SUCCESS;
}

int ipdirectUnicast_handleSocketFlowReq( USHORT sesnum, UCHAR *apkt, USHORT len )
{
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    // This is going to parse the parameters and throw an event to the ip direct thread which
    // will call the ProcessApduSocketFlowReq to do the processing.
    VL_BYTE_STREAM_INST(NewFlowRequest, ParseBuf, apkt, len);
    ULONG ulTag   = vlParse3ByteLong(pParseBuf);
    ULONG nLength = vlParseCcApduLengthField(pParseBuf);
    UCHAR ProtocolFlag = vlParseByte(pParseBuf);
    USHORT LocalPortNumber = vlParseShort(pParseBuf);
    USHORT RemotePortNumber = vlParseShort(pParseBuf);
    UCHAR RemoteAddrType = vlParseByte(pParseBuf);
    VL_HOST_IP_INFO hostIpInfo;
    IPDIRECT_UNICAST_UNUSED_VAR(nLength);

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:: LocalPortNumber = %d\n",  __FUNCTION__, LocalPortNumber);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:: RemotePortNumber = %d RemoteAddrType %d\n",  __FUNCTION__, RemotePortNumber, RemoteAddrType);
    vlg_nIpDirectUnicast_to_card_SocketFlowId = VL_DSG_BASE_HOST_UDP_SOCKET_FLOW_ID;

    vlXchanGetHostIpTypeInfo(VL_HOST_IP_IF_TYPE_DSG, 0, &hostIpInfo);

    if(hostIpInfo.ipAddressType == VL_XCHAN_IP_ADDRESS_TYPE_NONE || !IsNetworkAvailable())
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:: Network unavailable\n",  __FUNCTION__, LocalPortNumber);
        return ipdirectUnicast_sendSocketFlowCnf(sesnum, IPDIRECT_UNICAST_FLOW_RESULT_DENIED_NO_NETWORK, 0,
                                                 vlg_nIpDirectUnicast_to_card_SocketFlowId);
    }
    else if(ProtocolFlag != UDP_PROTOCOL_FLAG)
    {
        return ipdirectUnicast_sendSocketFlowCnf(sesnum, IPDIRECT_UNICAST_FLOW_RESULT_DENIED_TCP_FAILED, 0,
                                                 vlg_nIpDirectUnicast_to_card_SocketFlowId);
    }
    else if(LocalPortNumber == SocketFlowLocalPortNumber)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:: LocalPortNumber = SocketFlowLocalPortNumber = %d\n",  __FUNCTION__, LocalPortNumber);
        return ipdirectUnicast_sendSocketFlowCnf(sesnum, IPDIRECT_UNICAST_FLOW_RESULT_DENIED_PORT_IN_USE, 0,
                                                 vlg_nIpDirectUnicast_to_card_SocketFlowId);
    }
    else
    {
        SocketFlowLocalPortNumber = LocalPortNumber;
        /* Post event on vl_ipdirect_unicast_task msg Q. */
        ipdirect_unicast_serialize_event(ulTag, (unsigned long)LocalPortNumber);

        return ipdirectUnicast_sendSocketFlowCnf(sesnum, IPDIRECT_UNICAST_FLOW_RESULT_GRANTED, 0,
                                                 vlg_nIpDirectUnicast_to_card_SocketFlowId);
    }
}

int ipdirectUnicast_handleHttpGetReq( USHORT sesnum, UCHAR *apkt, USHORT len )
{

		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s: APDU data length= %u \n", __FUNCTION__, len);

    VL_BYTE_STREAM_INST(NewFlowRequest, ParseBuf, apkt, len);
    ULONG ulTag   = vlParse3ByteLong(pParseBuf);

    ULONG nLength = vlParseCcApduLengthField(pParseBuf);
    UCHAR nFlags  = vlParseByte(pParseBuf);
    IPDIRECT_UNICAST_UNUSED_VAR(nLength);
    
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: nFlags             = 0x%02X\n",  __FUNCTION__, nFlags);
    IPDIRECT_UNICAST_HTTP_GET_PARAMS * pStruct = &vlg_pHttpGetParams;
    memset(pStruct, 0, sizeof(*pStruct));
    pStruct->flowid             = vlg_nIpDirectUnicast_to_card_FlowId;
    pStruct->sesnum             = vlg_nIpDirectUnicastSessionNumber;
    
    pStruct->eProcessBy         = (IPDIRECT_UNICAST_HTTP_PROCESS_BY    )(nFlags&0x03);
    pStruct->isSiDataRequest    = (IPDIRECT_UNICAST_HTTP_PROCESS_BY    )((nFlags>>2)&0x01);
    //pStruct->eRemoteAddressType = (IPDIRECT_UNICAST_SERVER_ADDRESS_TYPE)((nFlags>>0)&0x3);
		pStruct->eRemoteAddressType = IPDIRECT_UNICAST_SERVER_ADDRESS_TYPE_IPV4;
    
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: eProcessBy         = %d.\n", __FUNCTION__, pStruct->eProcessBy        );
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: eRemoteAddressType = %d.\n", __FUNCTION__, pStruct->eRemoteAddressType);
    
    pStruct->nRemotePortNum = vlParseShort(pParseBuf);							// == vct_id
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: vct_id             = %d.\n", __FUNCTION__, pStruct->nRemotePortNum);
    vlg_nDsgVctId = pStruct->nRemotePortNum;
    if( vlg_nDsgVctId != persistedDsgVctId )
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s :Calling vlMpeosClearSiDatabase because change in  VCT_ID[old vcdId=%d,new vctId=%d]\n",__FUNCTION__,vlg_nOldDsgVctId,vlg_nDsgVctId);
        vlMpeosClearSiDatabase();
        vlMpeosNotifyVCTIDChange();
        persistedDsgVctId = vlg_nOldDsgVctId = vlg_nDsgVctId;
    }

    pStruct->transactionTimeout    = vlg_nIpDirectHttpGetTransactionTimeout;
    pStruct->connTimeout    = vlg_nIpDirectHttpGetConnectionTimeout;
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: transactionTimeout        = %d.\n", __FUNCTION__, pStruct->transactionTimeout);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: connTimeout        = %d.\n", __FUNCTION__, pStruct->connTimeout);
 
		// Actual complete get URL 
    pStruct->nPathNameLen = vlParseByte(pParseBuf);

    vlParseBuffer(pParseBuf, (unsigned char*)pStruct->pathName   , pStruct->nPathNameLen  , sizeof(pStruct->pathName ));
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: nPathNameLen       = %3d, pathName = '%s'\n", __FUNCTION__, pStruct->nPathNameLen, pStruct->pathName);

		// File name not used
    pStruct->nFileNameLen = 0;
    pStruct->fileName[0] = 0;

		/* Post event on vl_ipdirect_unicast_task msg Q. */
    ipdirect_unicast_serialize_event(ulTag, (unsigned long)(pStruct));
    return APDU_IPDIRECT_UNICAST_SUCCESS;
}

int ipdirectUnicast_sendHttpGetCnf(USHORT sesnum, USHORT status)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);

    UCHAR aMessage[32];
    VL_BYTE_STREAM_ARRAY_INST(aMessage, WriteBuf);
    int nFinalLen = 0;

    nFinalLen += vlWriteShort(pWriteBuf, status);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:: status = %d, len = %d\n",  __FUNCTION__, status, nFinalLen);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "IPDU::Host-->pod APDU:IPDIRECT_UNICAST_APDU_HTTP_GET_CNF\n");
    return vlIpDirectUnicastSndApdu2Pod(sesnum, IPDIRECT_UNICAST_APDU_HTTP_GET_CNF, nFinalLen, aMessage);
}

static int ipdirectUnicast_sendHttpPostCnf(USHORT sesnum, USHORT transactionId, USHORT status)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);

    UCHAR aMessage[32];
    VL_BYTE_STREAM_ARRAY_INST(aMessage, WriteBuf);
    int nFinalLen = 0;

    nFinalLen += vlWriteShort(pWriteBuf, transactionId);
    nFinalLen += vlWriteShort(pWriteBuf, status);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:: status = %d, transactionId = %d, len = %d\n",  __FUNCTION__, status, transactionId, nFinalLen);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "IPDU::Host-->pod APDU:IPDIRECT_UNICAST_APDU_HTTP_POST_CNF\n");
    return vlIpDirectUnicastSndApdu2Pod(sesnum, IPDIRECT_UNICAST_APDU_HTTP_POST_CNF, nFinalLen, aMessage);
}

int ipdirectUnicast_sendDeleteHttpFlowReq(USHORT sesnum, ULONG flowId)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);

    UCHAR aMessage[32];
    VL_BYTE_STREAM_ARRAY_INST(aMessage, WriteBuf);
    int nFinalLen = 0;

    nFinalLen += vlWrite3ByteLong(pWriteBuf, flowId);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:: flowid = 0x%06X, len = %d\n",  __FUNCTION__, flowId, nFinalLen);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "IPDU::Host-->pod APDU:IPDIRECT_UNICAST_APDU_HTTP_DEL_FLOW_REQ\n");
    return vlIpDirectUnicastSndApdu2Pod(sesnum, IPDIRECT_UNICAST_APDU_HTTP_DEL_FLOW_REQ, nFinalLen, aMessage);
}

int ipdirectUnicast_handleDeleteHttpFlowReq( USHORT sesnum, UCHAR *apkt, USHORT len )
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);

    VL_BYTE_STREAM_INST(NewFlowRequest, ParseBuf, apkt, len);
    ULONG ulTag   = vlParse3ByteLong(pParseBuf);
    ULONG nLength = vlParseCcApduLengthField(pParseBuf);
    ULONG nFlowId = vlParse3ByteLong(pParseBuf);
    IPDIRECT_UNICAST_UNUSED_VAR(ulTag);
    IPDIRECT_UNICAST_UNUSED_VAR(nLength);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:: nFlowId = 0x%06X\n",  __FUNCTION__, nFlowId);
		// IPDdownloadmanager must terminate the download for the given url. This logic handles both the
    // http get and http post flow deletion.
    if(nFlowId == vlg_nIpDirectUnicast_to_card_FlowId)
    {
  		ProcessApduFlowDeleteReq(IpduGetUrl);
	  	memset(IpduGetUrl, 0, sizeof(IpduGetUrl));						// Reset url string for next download request
    	vlg_nIpDirectUnicast_to_card_FlowId = 0;
    }
    else if(nFlowId == vlg_nIpDirectUnicast_from_card_FlowId)
    {
  		ProcessApduFlowDeleteReq(IpduSendUrl);
	  	memset(IpduSendUrl, 0, sizeof(IpduSendUrl));						// Reset url string for next download request
    	vlg_nIpDirectUnicast_from_card_FlowId = 0;
    }

		//Need to derive actual enum tag_IPDIRECT_UNICAST_DELETE_FLOW_RESULT,
		//now forcing to IPDIRECT_UNICAST_DELETE_FLOW_RESULT_GRANTED
		ipdirectUnicast_sendDeleteHttpFlowCnf(sesnum, nFlowId, IPDIRECT_UNICAST_DELETE_FLOW_RESULT_GRANTED);

  	vlg_nIpDirectUnicast_to_card_FlowId = 0;
    return APDU_IPDIRECT_UNICAST_SUCCESS;
}

int ipdirectUnicast_sendDeleteHttpFlowCnf(USHORT sesnum, ULONG flowId, UCHAR reason)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);

    UCHAR aMessage[32];
    VL_BYTE_STREAM_ARRAY_INST(aMessage, WriteBuf);
    int nFinalLen = 0;

    nFinalLen += vlWrite3ByteLong(pWriteBuf, flowId);
    nFinalLen += vlWriteByte(pWriteBuf, reason);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:: flowid = 0x%06X, reason = %d, len = %d\n",  __FUNCTION__, flowId, reason, nFinalLen);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "IPDU::Host-->pod APDU:IPDIRECT_UNICAST_APDU_HTTP_DEL_FLOW_CNF\n");
    return vlIpDirectUnicastSndApdu2Pod(sesnum, IPDIRECT_UNICAST_APDU_HTTP_DEL_FLOW_CNF, nFinalLen, aMessage);
}

int ipdirectUnicast_handleDeleteHttpFlowCnf( USHORT sesnum, UCHAR *apkt, USHORT len )
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);

    VL_BYTE_STREAM_INST(NewFlowRequest, ParseBuf, apkt, len);
    ULONG ulTag   = vlParse3ByteLong(pParseBuf);
    ULONG nLength = vlParseCcApduLengthField(pParseBuf);
    ULONG nFlowId = vlParse3ByteLong(pParseBuf);
    UCHAR nStatus = vlParseByte(pParseBuf);
    IPDIRECT_UNICAST_UNUSED_VAR(ulTag);
    IPDIRECT_UNICAST_UNUSED_VAR(nLength);
    
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:: nFlowId = 0x%06X, nStatus = %d\n",  __FUNCTION__, nFlowId, nStatus);
    
    if(IPDIRECT_UNICAST_DELETE_FLOW_RESULT_GRANTED == nStatus)
    {
        vlg_nIpDirectUnicast_to_card_FlowId = 0;
        return APDU_IPDIRECT_UNICAST_SUCCESS;
    }
    return APDU_IPDIRECT_UNICAST_FAILURE;
}

int ipdirectUnicast_sendLostHttpFlowInd(USHORT sesnum, ULONG flowId, UCHAR reason)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);

    UCHAR aMessage[32];
    VL_BYTE_STREAM_ARRAY_INST(aMessage, WriteBuf);
    int nFinalLen = 0;

    nFinalLen += vlWrite3ByteLong(pWriteBuf, flowId);
    nFinalLen += vlWriteByte(pWriteBuf, reason);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:: flowid = 0x%06X, reason = %d, len = %d\n",  __FUNCTION__, flowId, reason, nFinalLen);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "IPDU::Host-->pod APDU:IPDIRECT_UNICAST_APDU_HTTP_LOST_FLOW_IND\n");
    return vlIpDirectUnicastSndApdu2Pod(sesnum, IPDIRECT_UNICAST_APDU_HTTP_LOST_FLOW_IND, nFinalLen, aMessage);
}

int ipdirectUnicast_handleLostHttpFlowCnf( USHORT sesnum, UCHAR *apkt, USHORT len )
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);

    VL_BYTE_STREAM_INST(NewFlowRequest, ParseBuf, apkt, len);
    ULONG ulTag   = vlParse3ByteLong(pParseBuf);
    ULONG nLength = vlParseCcApduLengthField(pParseBuf);
    ULONG nFlowId = vlParse3ByteLong(pParseBuf);
    UCHAR nStatus = vlParseByte(pParseBuf);
    IPDIRECT_UNICAST_UNUSED_VAR(ulTag);
    IPDIRECT_UNICAST_UNUSED_VAR(nLength);
    
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:: nFlowId = 0x%06X, nStatus = %d\n",  __FUNCTION__, nFlowId, nStatus);
    
    if(IPDIRECT_UNICAST_LOST_FLOW_RESULT_ACKNOWLEDGED == nStatus)
    {
        vlg_nIpDirectUnicast_to_card_FlowId = 0;
        return APDU_IPDIRECT_UNICAST_SUCCESS;
    }
    return APDU_IPDIRECT_UNICAST_FAILURE;
}

int ipdirectUnicast_sendDataSegInfo(USHORT sesnum, USHORT nTotalSegments, ULONG nTotalLength)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);

    UCHAR aMessage[32];
    VL_BYTE_STREAM_ARRAY_INST(aMessage, WriteBuf);
    int nFinalLen = 0;

    nFinalLen += vlWriteShort(pWriteBuf, nTotalSegments);
    nFinalLen += vlWrite3ByteLong(pWriteBuf, nTotalLength);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:: nTotalSegments = %d, nTotalLength = %d, len = %d\n",  __FUNCTION__, nTotalSegments, nTotalLength, nFinalLen);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "IPDU::pod-->Host APDU:IPDIRECT_UNICAST_APDU_HTTP_DATA_SEG_INFO\n");
    return vlIpDirectUnicastSndApdu2Pod(sesnum, IPDIRECT_UNICAST_APDU_HTTP_DATA_SEG_INFO, nFinalLen, aMessage);
}

int ipdirectUnicast_handleHttpResendSegReq( USHORT sesnum, UCHAR *apkt, USHORT len )
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);

    VL_BYTE_STREAM_INST(NewFlowRequest, ParseBuf, apkt, len);
    ULONG ulTag     = vlParse3ByteLong(pParseBuf);
    ULONG nLength   = vlParseCcApduLengthField(pParseBuf);
    USHORT nSegment = vlParseShort(pParseBuf);

    IPDIRECT_UNICAST_UNUSED_VAR(nLength);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:: nSegment = %d\n",  __FUNCTION__, nSegment);
		/* Post event on vl_ipdirect_unicast_task msg Q. */
    ipdirect_unicast_serialize_event(ulTag, (unsigned long)(nSegment));
    return APDU_IPDIRECT_UNICAST_SUCCESS;
}

int ipdirectUnicast_handleHttpDataSegsRcvd( USHORT sesnum, UCHAR *apkt, USHORT len )
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);

    VL_BYTE_STREAM_INST(NewFlowRequest, ParseBuf, apkt, len);
    ULONG ulTag   = vlParse3ByteLong(pParseBuf);
    ULONG nLength = vlParseCcApduLengthField(pParseBuf);
    UCHAR nStatus = vlParseByte(pParseBuf);
    IPDIRECT_UNICAST_UNUSED_VAR(nLength);

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:: nStatus = %d\n",  __FUNCTION__, nStatus);
		/* Post event on vl_ipdirect_unicast_task msg Q. */
    ipdirect_unicast_serialize_event(ulTag, (unsigned long)(nStatus));
    return APDU_IPDIRECT_UNICAST_SUCCESS;
}

int ipdirectUnicast_handleHttpRecvStatus( USHORT sesnum, UCHAR *apkt, USHORT len )
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);

    VL_BYTE_STREAM_INST(NewFlowRequest, ParseBuf, apkt, len);
    ULONG ulTag   = vlParse3ByteLong(pParseBuf);
    ULONG nLength = vlParseCcApduLengthField(pParseBuf);
    UCHAR nStatus = vlParseByte(pParseBuf);
    IPDIRECT_UNICAST_UNUSED_VAR(nLength);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:: nStatus = %d\n",  __FUNCTION__, nStatus);
		switch(nStatus)
		{
			case 0:
			    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: HttpRecvStatus = Successfully received all.\n",  __FUNCTION__);
				break;

			case 1:
			    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: HttpRecvStatus = Start over resend.\n",  __FUNCTION__);
				break;

			case 2:
			    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: HttpRecvStatus = Message intended for Host fails authentication.\n",  __FUNCTION__);
				break;

			default:
			    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: HttpRecvStatus = Undefined status.\n",  __FUNCTION__);
				break;
		}

    return APDU_IPDIRECT_UNICAST_SUCCESS;
}

int ipdirectUnicast_handleIcnPidNotify( USHORT sesnum, UCHAR *apkt, USHORT len )
{
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);

    VL_BYTE_STREAM_INST(NewFlowRequest, ParseBuf, apkt, len);
    ULONG ulTag     = vlParse3ByteLong(pParseBuf);
    ULONG nLength   = vlParseCcApduLengthField(pParseBuf);
    USHORT nSegment = vlParseShort(pParseBuf);

    IPDIRECT_UNICAST_UNUSED_VAR(nLength);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:: nSegment = %d\n",  __FUNCTION__, nSegment);
                /* Post event on vl_ipdirect_unicast_task msg Q. */
    ipdirect_unicast_serialize_event(ulTag, (unsigned long)(nSegment));
    return APDU_IPDIRECT_UNICAST_SUCCESS;
}

// the ipdirect_unicast thread control
static void ipdirect_unicastUnprotectedProc( void * par )
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);

 //   char *ptr=NULL;
    LCSM_EVENT_INFO       *eventInfo = (LCSM_EVENT_INFO *)par;

    if (eventInfo == NULL)
    	return;

    switch( eventInfo->event )
    {
        // POD requests open session (x=resId, y=tcId)
        case POD_OPEN_SESSION_REQ:
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "IpDirectUnicast_OpenSessReq called\n");
            AM_OPEN_SS_REQ_IPDIRECT_UNICAST(eventInfo->x, eventInfo->y);
        }
        break;

        // POD confirms open session (x=sessNbr, y=resId, z=tcId)
        case POD_SESSION_OPENED:
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "IpDirectUnicast_SessOpened  called\n");
            AM_SS_OPENED_IPDIRECT_UNICAST(eventInfo->x, eventInfo->y, eventInfo->z);
        }
        break;

        // POD closes session (x=sessNbr)
        case POD_CLOSE_SESSION:
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "IpDirectUnicast_CloseSession  called\n");
            AM_SS_CLOSED_IPDIRECT_UNICAST(eventInfo->x);
        }
        break;

        // POD sends APDU (x=sessNbr, y=APDU buffer ptr, z=APDU byte length)
        case POD_APDU:
        {
            VL_BYTE_STREAM_INST(POD_APDU, ParseBuf, eventInfo->y, eventInfo->z);
            ULONG ulTag = vlParse3ByteLong(pParseBuf);
            ULONG nLength = vlParseCcApduLengthField(pParseBuf);

            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "ipdirect_unicastUnprotectedProc case POD_APDU: received tag 0x%06X of length %d\n", ulTag, nLength);

            switch (ulTag)
            {
                case IPDIRECT_UNICAST_APDU_ICN_PID_NOTIFY:
                {
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "IPDU::pod-->Host APDU:IPDIRECT_UNICAST_APDU_ICN_PID_NOTIFY\n");
	                  bool ret = InitializeIPDdownloaderManager();			// IPDU Feature, Initialize IPD dl mgr when going
                                                                      // into IPDU mode.
	                  if(ret)
		                  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s: InitializeIPDdownloaderManager failed. \n", __FUNCTION__);
                   
                    // ICN PID notify essentially indicates we are now running in IPDU mode.
                    ipdirectUnicast_handleIcnPidNotify((USHORT)eventInfo->x, (UCHAR *)eventInfo->y, (USHORT)eventInfo->z);
                }
                break;

                case IPDIRECT_UNICAST_APDU_HTTP_RECV_FLOW_REQ:
                {
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "IPDU::pod-->Host APDU:IPDIRECT_UNICAST_APDU_HTTP_RECV_FLOW_REQ\n");
                    ipdirectUnicast_handleHttpRecvFlowReq((USHORT)eventInfo->x, (UCHAR *)eventInfo->y, (USHORT)eventInfo->z);
                }
                break;

                case IPDIRECT_UNICAST_APDU_HTTP_SEND_FLOW_REQ:// == HTTP Post 
                {
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "IPDU::pod-->Host APDU:IPDIRECT_UNICAST_APDU_HTTP_SEND_FLOW_REQ\n");
                    ipdirectUnicast_handleHttpSendFlowReq((USHORT)eventInfo->x, (UCHAR *)eventInfo->y, (USHORT)eventInfo->z);
                }
                break;

                case IPDIRECT_UNICAST_APDU_HTTP_FLOW_CNF:
                {
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "IPDU::pod-->Host APDU:IPDIRECT_UNICAST_APDU_HTTP_FLOW_CNF\n");
                }
                break;

                case IPDIRECT_UNICAST_APDU_SOCKET_FLOW_REQ:
                {
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "IPDU::pod-->Host APDU:IPDIRECT_UNICAST_APDU_SOCKET_FLOW_REQ\n");
                    ipdirectUnicast_handleSocketFlowReq((USHORT)eventInfo->x, (UCHAR *)eventInfo->y, (USHORT)eventInfo->z);
                }
                break;

                case IPDIRECT_UNICAST_APDU_SOCKET_FLOW_CNF:
                {
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "IPDU::pod-->Host APDU:IPDIRECT_UNICAST_APDU_SOCKET_FLOW_CNF\n");
                }
                break;

                case IPDIRECT_UNICAST_APDU_HTTP_DEL_FLOW_REQ:
                {
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "IPDU::pod-->Host APDU:IPDIRECT_UNICAST_APDU_HTTP_DEL_FLOW_REQ\n");
										// IPDdownloadmanager must terminate the download for the given url. 
										//ProcessApduFlowDeleteReq(IpduUrl);
										//memset(pIpduUrl, 0, sizeof(IpduUrl));						// Reset url string for next download request
                    ipdirectUnicast_handleDeleteHttpFlowReq((USHORT)eventInfo->x, (UCHAR *)eventInfo->y, (USHORT)eventInfo->z);
                }
                break;

                case IPDIRECT_UNICAST_APDU_HTTP_DEL_FLOW_CNF:
                {
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "IPDU::pod-->Host APDU:IPDIRECT_UNICAST_APDU_HTTP_DEL_FLOW_CNF\n");
                    ipdirectUnicast_handleDeleteHttpFlowCnf((USHORT)eventInfo->x, (UCHAR *)eventInfo->y, (USHORT)eventInfo->z);
                }
                break;

                case IPDIRECT_UNICAST_APDU_HTTP_LOST_FLOW_IND:
                {
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "IPDU::pod-->Host APDU:IPDIRECT_UNICAST_APDU_HTTP_LOST_FLOW_IND\n");
                    ipdirectUnicast_handleLostHttpFlowCnf((USHORT)eventInfo->x, (UCHAR *)eventInfo->y, (USHORT)eventInfo->z);
                }
                break;

                case IPDIRECT_UNICAST_APDU_HTTP_LOST_FLOW_CNF:
                {
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "IPDU::pod-->Host APDU:IPDIRECT_UNICAST_APDU_HTTP_LOST_FLOW_CNF\n");
                    ipdirectUnicast_handleLostHttpFlowCnf((USHORT)eventInfo->x, (UCHAR *)eventInfo->y, (USHORT)eventInfo->z);
                }
                break;

                case IPDIRECT_UNICAST_APDU_HTTP_GET_REQ:
                {
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "IPDU::pod-->Host APDU:IPDIRECT_UNICAST_APDU_HTTP_GET_REQ\n");
										// request processing.
										tHttpGetReqEventInfo = *eventInfo;								// Structure copy, save req event info.
                    ipdirectUnicast_handleHttpGetReq((USHORT)eventInfo->x, (UCHAR *)eventInfo->y, (USHORT)eventInfo->z);
                }
                break;

                case IPDIRECT_UNICAST_APDU_HTTP_POST_CNF:
                {
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "IPDU::pod-->Host APDU:IPDIRECT_UNICAST_APDU_HTTP_POST_CNF\n");
                }
                break;

                case IPDIRECT_UNICAST_APDU_HTTP_DATA_SEG_INFO:
                {
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "IPDU::pod-->Host APDU:IPDIRECT_UNICAST_APDU_HTTP_DATA_SEG_INFO\n");
                }
                break;

                case IPDIRECT_UNICAST_APDU_HTTP_RESEND_SEG_REQ:
                {
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "IPDU::pod-->Host APDU:IPDIRECT_UNICAST_APDU_HTTP_RESEND_SEG_REQ\n");
                    ipdirectUnicast_handleHttpResendSegReq((USHORT)eventInfo->x, (UCHAR *)eventInfo->y, (USHORT)eventInfo->z);
                }
                break;

                case IPDIRECT_UNICAST_APDU_HTTP_RECV_STATUS:
                {
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "IPDU::pod-->Host APDU:IPDIRECT_UNICAST_APDU_HTTP_RECV_STATUS\n");
										ipdirectUnicast_handleHttpRecvStatus((USHORT)eventInfo->x, (UCHAR *)eventInfo->y, (USHORT)eventInfo->z);
                }
                break;

                default:
                {
                    RDK_LOG(RDK_LOG_WARN, "LOG.RDK.POD", "WARN: %s : Receive Unsupported POD_APDU msg 0x%06X\n", __FUNCTION__, ulTag);
                }
                break;
            }
        }
        break;

        case POD_IPDU_DOWNLOAD_TIMER:
        {
            // In order for the writeCurlCallback to work correctly the download/upload file transfer progress
            // must be querried from same same thread that the curl transfer was initiated. So queue event to
            // IPDU worker thread.
            rmf_osal_event_handle_t ipdu_event_handle;
            rmf_osal_event_params_t ipdu_event_params;
            rmf_osal_eventqueue_handle_t ipdu_event_queue;
            ipdu_event_params = { 0, (void *)NULL, 0, NULL };
            ipdu_event_queue = GetIpDirectUnicastMsgQueue();
            if(0 != ipdu_event_queue)
            {
                rmf_osal_event_create(RMF_OSAL_EVENT_CATEGORY_POD, POD_IPDU_DOWNLOAD_TIMER, &ipdu_event_params, &ipdu_event_handle);
                if ( rmf_osal_eventqueue_add(ipdu_event_queue, ipdu_event_handle ) != RMF_SUCCESS )
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s: rmf_osal_eventqueue_add Failed \n", __FUNCTION__);
                }
            }
        }
        break;

		// IPDU Feature: Polling timer to monitor upload progress of IPDU files.
        case POD_IPDU_UPLOAD_TIMER:
        {
            // In order for the writeCurlCallback to work correctly the download/upload file transfer progress
            // must be querried from same same thread that the curl transfer was initiated. So queue event to
            // IPDU worker thread.
            rmf_osal_event_handle_t ipdu_event_handle;
            rmf_osal_event_params_t ipdu_event_params;
            rmf_osal_eventqueue_handle_t ipdu_event_queue;
            ipdu_event_params = { 0, (void *)NULL, 0, NULL };
            ipdu_event_queue = GetIpDirectUnicastMsgQueue();
            if(0 != ipdu_event_queue)
            {
                rmf_osal_event_create(RMF_OSAL_EVENT_CATEGORY_POD, POD_IPDU_UPLOAD_TIMER, &ipdu_event_params, &ipdu_event_handle);
                if ( rmf_osal_eventqueue_add(ipdu_event_queue, ipdu_event_handle ) != RMF_SUCCESS )
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s: rmf_osal_eventqueue_add Failed \n", __FUNCTION__);
                }
            }
        }
        break;

        default:
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "invalid IPDIRECT_UNICAST msg received=0x%x\n", eventInfo->event );
        }
        break;
    }
}

void ipdirect_unicastProtectedProc( void * par )
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);

    rmf_osal_mutexAcquire(vlIpDirectUnicastMainGetThreadLock());
    {
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "\nBefore calling ipdirect_unicastUnprotectedProc\n");        		
        ipdirect_unicastUnprotectedProc(par);
    }
    rmf_osal_mutexRelease(vlIpDirectUnicastMainGetThreadLock());
}

void vlSendIpDirectUnicastEventToPFC(int eEventType, void * pData, int nSize)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);

    rmf_osal_event_handle_t event_handle;
    rmf_osal_event_params_t event_params = {0};
    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();

    event_params.priority = 0; //Default priority;
    event_params.data = pData;
    event_params.data_extension = nSize;
    event_params.free_payload_fn = NULL;

    rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_CARD_MANAGER, (CardMgrEventType)eEventType, 
        &event_params, &event_handle);		                                     				    		
    rmf_osal_eventmanager_dispatch_event(em, event_handle);
}

void vlIpDirectUnicastSendBootupEventToPFC(int eEventType, int nMsgCode, unsigned long ulData)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);

    PostBootEvent(eEventType, nMsgCode, ulData);
}

/**************************************************
 *	APDU Functions to handle IPDIRECT_UNICAST
 *************************************************/
int vlIpDirectUnicastSndApdu2Pod(unsigned short sesnum, unsigned long tag, unsigned short dataLen, unsigned char *data)
{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);

    if(0 == sesnum)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: Not sending APDU TAG 0x%06X as sesnum = %d.\n", __FUNCTION__, tag, sesnum);
        return APDU_IPDIRECT_UNICAST_FAILURE;
    }
	
    unsigned short alen;
    unsigned char papdu[MAX_APDU_HEADER_BYTES + dataLen];

    if (buildApdu(papdu, &alen, tag, dataLen, data ) == NULL)
    {
        return APDU_IPDIRECT_UNICAST_FAILURE;
    }

    AM_APDU_FROM_HOST(sesnum, papdu, alen);
/*
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "ipdirect_unicastSndApdu : APDU len = %d\n",alen);
    for(int i =0;i<alen;i++)
    {
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "0x%02x ", papdu[i]);
    }
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "\n");
*/
    return APDU_IPDIRECT_UNICAST_SUCCESS;
}


void vlIpduFileTransferProgressTimer(bool control, long int event)
{
  LCSM_TIMER_HANDLE* pIpduFileTransferTimerHandle ;
	LCSM_EVENT_INFO eventSend;

  if(POD_IPDU_UPLOAD_TIMER == event)
  {
  	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s: event= POD_IPDU_UPLOAD_TIMER\n", __FUNCTION__);
		eventSend.event = POD_IPDU_UPLOAD_TIMER;
    pIpduFileTransferTimerHandle = &vlg_IpduUploadTimerHandle;
  }
  else//POD_IPDU_DOWNLOAD_TIMER
  {
  	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s: event= POD_IPDU_DOWNLOAD_TIMER\n", __FUNCTION__);
		eventSend.event = POD_IPDU_DOWNLOAD_TIMER;
    pIpduFileTransferTimerHandle = &vlg_IpduDownloadTimerHandle;
  }

	if (control)
	{
		/* control=true will start the timer. */
		eventSend.dataPtr = NULL;
		eventSend.dataLength = 0;
		if(0 != *pIpduFileTransferTimerHandle)
		{
			RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s- Restart Timer \n", __FUNCTION__);
			/* If timer is already started, restart/reset it before restarting. */
			/* The timer can only be reset if it has not yet expired. */
			podResetTimer(*pIpduFileTransferTimerHandle,vlg_IpduFileTransferPollTime);
		}	
		else
		{
			RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s- Start Timer \n", __FUNCTION__);

			*pIpduFileTransferTimerHandle = podSetTimer( POD_IPDIRECT_UNICAST_COMM_QUEUE, &eventSend, vlg_IpduFileTransferPollTime);
			RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s: Created POD timer, handle= %X \n", __FUNCTION__, *pIpduFileTransferTimerHandle);
		}
	}
	else
	{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s- Cancel Timer \n", __FUNCTION__);
		/* control=false will cancel the timer. */
		podCancelTimer(*pIpduFileTransferTimerHandle);
		*pIpduFileTransferTimerHandle = 0;
	}
}

char* AssembleHttpGetUrl(IPDIRECT_UNICAST_HTTP_GET_PARAMS* pParams)
{
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s\n", __FUNCTION__);

	pIpduUrl = &IpduGetUrl[0];
	memset(pIpduUrl, 0, sizeof(IpduGetUrl));						// Reset url string
	if (NULL != pParams)
	{
		// The assumption is that the APDU provides a fully qualified url, just use it as it.
		strcpy(pIpduUrl, pParams->pathName);
	}
	else
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s: Invalid IPDU parameters structure.\n", __FUNCTION__);
	}
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: HttpGetReq url= %s \n", __FUNCTION__, pIpduUrl);
	return pIpduUrl;
}

rmf_osal_eventqueue_handle_t GetIpDirectUnicastMsgQueue(void)
{
  return vlg_ipdirect_unicast_msgQ;
}


int IsSystemTimeAvailable(void)
{
  int haveTime;

  // when using IPDU the STT comes from a NTP server.
  // This must be configured in rmfconfig.ini:
  // NTP.FAILOVER.*
  struct stat stt_file;
  haveTime = ((stat("/tmp/stt_received", &stt_file) == 0));
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: haveTime=%d  \n", __FUNCTION__, haveTime);

  return haveTime;
}

int IsNetworkAvailable(void)
{
  int haveNetwork;

  struct stat ip_acquired_file;
  haveNetwork = ((stat("/tmp/ip_acquired", &ip_acquired_file) == 0));
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: haveNetwork=%d  \n", __FUNCTION__, haveNetwork);

  return haveNetwork;
}

void ServiceHttpGetResponse(char* cUrl, IPDIRECT_UNICAST_HTTP_GET_PARAMS* pRequestParams)
{
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s\n", __FUNCTION__);
    vector<unsigned char> aByteContent;
	IPDdownloadMgrRetCode retCode;
    int httpResponse;

	ProcessApduTransferConfirmation(cUrl, &httpResponse);
	// Then send confirmation response to CC.
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:  Sending IPDIRECT_UNICAST_APDU_HTTP_GET_CNF, Http Response= %u\n", __FUNCTION__, httpResponse);
    ipdirectUnicast_sendHttpGetCnf(vlg_nIpDirectUnicastSessionNumber, httpResponse);
    if(IPDIRECT_UNICAST_GET_STATUS_OK == httpResponse)
    {                            
        vlg_ipdirect_get_request_count++;
	    // IPDU: use existing data types and methods to send data to CC.
		aByteContent.clear();
		const unsigned char* file ;
		unsigned int filesize ;
		retCode = GetDownloadedFile(cUrl, &file, &filesize);
		if(IPD_DM_NO_ERROR == retCode)
		{
		    // Now bridge the new IPDU Download manager code to existing framework.
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:  Send downloaded data file to CC, file size= %u.\n", __FUNCTION__, filesize);
			aByteContent.assign(file, file+filesize);
                          TerminateDownload(cUrl);       // Cleanup
            switch(pRequestParams->eProcessBy)
            {
                case IPDIRECT_UNICAST_HTTP_PROCESS_BY_HOST:
                {
					RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:  eProcessBy= IPDIRECT_UNICAST_HTTP_PROCESS_BY_HOST\n", __FUNCTION__);
                }
                break;
            
                case IPDIRECT_UNICAST_HTTP_PROCESS_BY_CARD:
                {
				    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:  eProcessBy= IPDIRECT_UNICAST_HTTP_PROCESS_BY_CARD\n", __FUNCTION__);
                    ipdirect_send_full_content_to_card(pRequestParams->flowid, aByteContent);
                }
                break;
            
                case IPDIRECT_UNICAST_HTTP_PROCESS_BY_BOTH:
                {
				    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:  eProcessBy= IPDIRECT_UNICAST_HTTP_PROCESS_BY_BOTH\n", __FUNCTION__);
                    ipdirect_send_full_content_to_card(pRequestParams->flowid, aByteContent);
                
                }
                break;
            
                default:
                {
                    // do nothing.
				    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:  eProcessBy= Invalid\n", __FUNCTION__);
                }
                break;
            }
		  }
		  else
		  {
			  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s:  Failed to access downloaded file.\n", __FUNCTION__);
		  }
  }
  else
  {
      TerminateDownload(cUrl);       // Cleanup
	  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s:  IPDU HttpGetReq Failed.\n", __FUNCTION__);
      vlg_ipdirect_get_fail_count++;
  }
}


void ServiceHttpPostResponse(char* cUrl, IPDIRECT_UNICAST_HTTP_GET_PARAMS* pRequestParams)
{
  int httpResponse;

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s\n", __FUNCTION__);
    ProcessApduTransferConfirmation(IpduSendUrl, &httpResponse);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s:  Sending IPDIRECT_UNICAST_APDU_HTTP_POST_CNF, Http Response= %u\n", __FUNCTION__, httpResponse);
    ipdirectUnicast_sendHttpPostCnf(vlg_nIpDirectUnicastSessionNumber, IpduSendParams.transactionId, httpResponse);

    TerminateDownload(cUrl);       // Cleanup
}

#ifdef __cplusplus
}
#endif

