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
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>

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
#include "sys_api.h"

#include "dsg-main.h"
#include "ipdirectUnicast-main.h"
#include "vlpluginapp_haldsgclass.h"
#include "utilityMacros.h"
#include "bufParseUtilities.h"
#include "vlDsgParser.h"
#include "traceStack.h"
#include "vlDsgObjectPrinter.h"
#include "vlDsgTunnelParser.h"
#include "vlDsgDispatchToClients.h"
#include "dsgClientController.h"
#include "ip_types.h"
#include "vlXchanFlowManager.h"
#include "dsgProxyTypes.h"
#include "vlDsgProxyService.h"

#include "cardUtils.h"
#include "core_events.h"
#include "rmf_osal_event.h"
#include "rmf_osal_thread.h"
#include "vlpluginapp_haldsgapi.h"
#include "dsgResApi.h"
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

#include "IPDSocketManager.h"

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


#define VL_DSG_STATS_LOG_LEVEL  RDK_LOG_DEBUG
#define VL_DSG_IP_RECOVERY_CHECK_INTERVAL       15
#define VL_DSG_IP_RECOVERY_MIN_FAIL_DURATION    600 // Lower bounds before recovery kicks in. Recovery should not kick-in earlier than this.
#define VL_OOB_MODE_MIN_FAIL_DURATION           60  // Lower bounds before recovery kicks in. Recovery should not kick-in earlier than this.
#define VL_DSG_IP_RECOVERY_FAIL_DURATION_OFFSET 150 // Offset added nOperTimeout to create the upper bounds before recovery kicks in.
#define VL_DSG_IP_RECOVERY_MIN_APP_NOTIFICATION_THRESHOLD   240 // Time threshold that causes clearing of problem state.
#define VL_DSG_IP_RECOVERY_MIN_APP_FAIL_DURATION            300 // Lower bounds before recovery kicks in. Recovery should not kick-in earlier than this.
#define VL_DSG_IP_RECOVERY_MIN_APP_RECOVERY_INTERVAL        660 // Lower bounds for interval for recovery attempts.
#define VL_DSG_IP_RECOVERY_MIN_APP_RECOVERY_INTERVAL_UPDATED 1500 // Updatedd lower bounds for interval for recovery attempts.
#define VL_DSG_ECM_RESET_BINARY                 "/bin/ecm_hw_reset"
#define VL_DSG_ALT_ECM_RESET_BINARY             "/sbin/ecm_hw_reset"
#define VL_CDL_DEV_INITIATED_RCDL_IN_PROGRESS       "/tmp/device_initiated_rcdl_in_progress"
#define VL_CDL_ECM_INITIATED_CDL_IN_PROGRESS        "/tmp/ecm_initiated_cdl_in_progress"
#define REBOOT_ON_IP_MODE_CHANGE_FLAG           "/opt/reboot_on_ip_mode_change"

static LCSM_DEVICE_HANDLE   handle = 0;
static LCSM_TIMER_HANDLE    timer = 0;
static pthread_t            dsgThread;
rmf_osal_eventqueue_handle_t                 vlg_dsgModeSerializerMsgQ;

static pthread_mutex_t      dsgLoopLock;
static int                  loop = 1;
int  vlg_nHeaderBytesToRemove = 0;

VL_DSG_MODE         vlg_Old_DsgMode;
VL_DSG_MODE         vlg_DsgMode     ;
VL_DSG_DIRECTORY    vlg_DsgDirectory;
VL_DSG_DIRECTORY    vlg_PreviousDsgDirectory;
int                 vlg_bPartialDsgDirectory = 0;
VL_DSG_ADV_CONFIG   vlg_DsgAdvConfig;
VL_DSG_DCD          vlg_DsgDcd;
VL_DSG_ERROR        vlg_DsgError;
unsigned long       vlg_ucid = 0;
static VL_DSG_TRAFFIC_STATS vlg_dsgTunnelStats[VL_MAX_DSG_ENTRIES];
static VL_DSG_TRAFFIC_STATS vlg_xchanTrafficStats[VL_XCHAN_FLOW_TYPE_MAX];
static int          vlg_bEnableTrafficStats = 1; // enabled by default
static int          vlg_bEnableDsgBroker = 0;
static unsigned int vlg_tLastDcdNotification = 0;
static unsigned int vlg_tLastOneWayNotification = 0;
static unsigned int vlg_tLastDcdResponse = 0;
static unsigned int vlg_tLastOneWayResponse = 0;
static unsigned int vlg_bDownstreamScanCompleted = 0;
static unsigned int vlg_bInvalidDsgChannelAlreadyIndicated = 0;
static unsigned int vlg_tBootTime = 0;
unsigned long       vlg_dsgBrokerUcid = 0;
#define VL_DSG_BROKER_RESPONSE_TIME_WINDOW  5
#define VL_DSG_BROKER_STATUS_CHECK_INTERVAL 5
static unsigned char vlg_aBytDcdBuffer[4096];
static unsigned int  vlg_nDcdBufferLength = 0;
static unsigned int  vlg_nDcdApduTag = 0;
int vlg_bDsgBrokerDeliverCaSTT   = 0;
int vlg_bDsgBrokerDeliverCaEAS   = 0;
int vlg_bDsgBrokerDeliverCaXAIT  = 0;
int vlg_bDsgBrokerDeliverCaCVT   = 0;
int vlg_bDsgBrokerDeliverCaSI    = 0;
int vlg_bDsgBrokerDeliverOnlyBcast = 0;
int vlg_bDsgBrokerDeliverBcastSTT  = 0;
int vlg_bDsgBrokerDeliverBcastEAS  = 0;
int vlg_bDsgBrokerDeliverBcastXAIT = 0;
int vlg_bDsgBrokerDeliverBcastCVT  = 0;
int vlg_bDsgBrokerDeliverBcastSI   = 0;
int vlg_bDsgBrokerDeliverAnySTT  = 0;
int vlg_bDsgBrokerDeliverAnyEAS  = 0;
int vlg_bDsgBrokerDeliverAnyXAIT = 0;
int vlg_bDsgBrokerDeliverAnyCVT  = 0;
int vlg_bDsgBrokerDeliverAnySI   = 0;

VL_HOST_IP_INFO vlg_HostIpInfo;
int vlg_bHostAcquiredIPAddress = 0;
int vlg_bAcquiredIpv4Address   = 0;
int vlg_dsgIpv4Address         = 0;
int vlg_dsgIpv4Gateway         = 0;
int vlg_bAcquiredIpv6Address   = 0;
extern int vlg_bDsgIpuFlowRequestPending;

extern unsigned long vlg_IPU_svc_FlowId;

int vlg_bDsgBasicFilterValid = 0;
int vlg_bDsgConfigFilterValid = 0;
int vlg_bDsgDirectoryFilterValid = 0;
unsigned long vlg_nDsgVctId = 0;
unsigned long vlg_nOldDsgVctId = 0;
unsigned long persistedDsgVctId = 0;
int vlg_bVctIdReceived = 0;

extern int vlg_bOutstandingOobTxTuneRequest;
extern int vlg_bOutstandingOobRxTuneRequest;

int vlDsgSndApdu2Pod(unsigned short sesnum, unsigned long tag, unsigned short dataLen, unsigned char *data);
ULONG vlGetXChanSession();
ULONG vlGetXChanDsgSessionNumber();
void vlMpeosClearSiDatabase();
void vlMpeosSIPurgeRDDTables();
void vlMpeosNotifyVCTIDChange();
static int  vlDsgApplyDsgDirectory();
static int  vlDsgReclassifyDirectoryForUcid();
extern  int HomingCardFirmwareUpgradeGetState();


// ******************* Session Stuff ********************************
//find our session number and are we open?
//CCIF2.0 table 9.3.4 states that only 1 session active at a time
//this makes the implementation simple; we'll use two
//static globals
// below, may need functions to read/write with mutex pretection
static USHORT vlg_nDsgSessionNumber = 0;
unsigned long vlg_nDsg_to_card_FlowId = 0;
unsigned long vlg_nIpDm_to_card_FlowId = 0;
extern unsigned long vlg_tcHostIpLastChange;
static int    dsgSessionState  = 0;
static int vlg_dsgDownstreamAcquired = 0;
int vlg_dsgInquiryPerformed = 0;
// above
static ULONG  ResourceId    = 0;
static UCHAR  Tcid          = 0;
static ULONG  tmpResourceId = 0; //tmp for binding
static UCHAR  tmpTcid       = 0; //tmp for binding


extern void PostBootEvent(int eEventType, int nMsgCode, unsigned long ulData);

int vlg_bDsgScanFailed = 0;

rmf_osal_Mutex vlDsgMainGetThreadLock()
{
    static rmf_osal_Mutex lock ; 
    static int i =0;
    if(i==0)
    {		
    	rmf_osal_mutexNew(&lock);
       i =1;		
    }
    return lock;
}

rmf_osal_Mutex vlDsgTunnelGetThreadLock()
{
    static rmf_osal_Mutex lock ; 
    static int i =0;
    if(i==0)
    {		
    	rmf_osal_mutexNew(&lock);
       i =1;		
    }
    return lock;
}

void vlDsgTouchFile(const char * pszFile)
{
    if(NULL != pszFile)
    {
#ifdef GLI
       IARM_Bus_SYSMgr_EventData_t eventData;
       eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_ESTB_IP;
       eventData.data.systemStates.state = 2;
       eventData.data.systemStates.error = 0;
       IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));

       eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_ECM_IP;
       IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));

#endif
        FILE * fp = fopen(pszFile, "w");
        if(NULL != fp)
        {
            fclose(fp);
        }
    }
}

void vlDsgUpdateXchanTrafficCounters(VL_XCHAN_FLOW_TYPE eFlowType, int nBytes)
{// this function is not protected as it is called by only one thread (pod ext chnl read)
    if(vlg_bEnableTrafficStats)
    {
        rmf_osal_mutexAcquire(vlDsgTunnelGetThreadLock());
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
        rmf_osal_mutexRelease(vlDsgTunnelGetThreadLock());
    }
}

void vlDsgUpdateDsgTrafficCounters(int bCardEntry, int iEntry, int nBytes)
{
    if(vlg_bEnableTrafficStats)
    {
        rmf_osal_mutexAcquire(vlDsgTunnelGetThreadLock());
        {
            if(bCardEntry) iEntry += vlg_DsgDirectory.nHostEntries;
            int ix = VL_SAFE_ARRAY_INDEX(iEntry, vlg_dsgTunnelStats);
            vlg_dsgTunnelStats[ix].nPkts     += 1;
            vlg_dsgTunnelStats[ix].nOctets   += nBytes;
            if(0 == vlg_dsgTunnelStats[ix].tFirstPacket)
            {
                vlg_dsgTunnelStats[ix].tFirstPacket = time(NULL);
                vlg_dsgTunnelStats[ix].tLastPacket  = vlg_dsgTunnelStats[ix].tFirstPacket;
            }
            else
            {
                vlg_dsgTunnelStats[ix].tLastPacket = time(NULL);
            }
        }
        rmf_osal_mutexRelease(vlDsgTunnelGetThreadLock());
    }
}

void vlDsgGetDsgEntryStats(int iStat, VL_DSG_TRAFFIC_TEXT_STATS * pTextStats)
{
    int bCardEntry = 0;
    memset(pTextStats,0,sizeof(*pTextStats));
    if(iStat >= (vlg_DsgDirectory.nHostEntries + vlg_DsgDirectory.nCardEntries)) return;
    if(iStat >= VL_ARRAY_ITEM_COUNT(vlg_dsgTunnelStats)) return;
    VL_DSG_HOST_ENTRY hostEntry;
    VL_DSG_CARD_ENTRY cardEntry;
    VL_DSG_TRAFFIC_STATS stats;
    
    // extract the stats
    rmf_osal_mutexAcquire(vlDsgTunnelGetThreadLock());
    {
        int iEntry = VL_SAFE_ARRAY_INDEX(iStat, vlg_DsgDirectory.aHostEntries);
        stats = vlg_dsgTunnelStats[iStat];
        
        if(iStat >= vlg_DsgDirectory.nHostEntries)
        {
            bCardEntry = 1;
            iEntry = VL_SAFE_ARRAY_INDEX(iStat-vlg_DsgDirectory.nHostEntries, vlg_DsgDirectory.aCardEntries);
        }
        if(!bCardEntry)
        {
            hostEntry = vlg_DsgDirectory.aHostEntries[iEntry];
	    memset(&(cardEntry),0,sizeof(cardEntry));
        }
        else
        {
            cardEntry = vlg_DsgDirectory.aCardEntries[iEntry];
	    memset(&hostEntry,0,sizeof(hostEntry));
        }
    }
    rmf_osal_mutexRelease(vlDsgTunnelGetThreadLock());
    
    // dump the stats
    {
        const char* client_types[] =
        {
            "    CA",
            " BCAST",
            "  WKMA",
            "   CAS",
            "   APP",
            "      ",
        };

        const char* client_ids[] =
        {
            "       ",
            "SCTE_65",
            "SCTE_18",
            "       ",
            "       ",
            "   XAIT",
            "       ",
        };

        const char* entry_types[] =
        {
            "eCm2Card",
            "eCm2Host",
            "Crd2Host",
            "        ",
        };
        
        pTextStats->pszFilterType = VL_COND_STR(bCardEntry, CARD, HOST);
        pTextStats->pszEntryType  = entry_types[VL_SAFE_ARRAY_INDEX(hostEntry.eEntryType, entry_types)];
        pTextStats->pszClientType = client_types[VL_SAFE_ARRAY_INDEX(hostEntry.clientId.eEncodingType, client_types)];
        pTextStats->pszClientId   = ((VL_DSG_CLIENT_ID_ENC_TYPE_BCAST == hostEntry.clientId.eEncodingType)?client_ids[SAFE_ARRAY_INDEX_UN(hostEntry.clientId.nEncodedId, client_ids)]: "       ");
        
        if((VL_DSG_CLIENT_ID_ENC_TYPE_BCAST != hostEntry.clientId.eEncodingType) &&
           (0 != hostEntry.clientId.nEncodedId))
        {
            VL_PRINT_LONG_TO_STR(hostEntry.clientId.nEncodedId, hostEntry.clientId.nEncodingLength, pTextStats->szClientId);
            pTextStats->pszClientId = pTextStats->szClientId;
        }
        
        if(bCardEntry)
        {
            char szBuf[32];
            snprintf(pTextStats->szClientId, sizeof(pTextStats->szClientId), "%s:%5d", VL_PRINT_LONG_TO_STR(cardEntry.adsgFilter.macAddress, VL_MAC_ADDR_SIZE, szBuf), cardEntry.adsgFilter.nPortEnd);
            pTextStats->pszClientId = pTextStats->szClientId;
        }
        
        snprintf(pTextStats->szPkts, sizeof(pTextStats->szPkts), "%u", stats.nPkts);
        pTextStats->nOctets         = stats.nOctets; 
        pTextStats->tFirstPacket    = stats.tFirstPacket;
        pTextStats->tLastPacket     = stats.tLastPacket;
    }
}

void vlDsgGetXchanEntryStats(int iStat, VL_DSG_TRAFFIC_TEXT_STATS * pTextStats)
{
	memset(pTextStats,0,sizeof(*pTextStats));
    
    if(iStat >= VL_ARRAY_ITEM_COUNT(vlg_xchanTrafficStats)) return;

    VL_DSG_TRAFFIC_STATS stats;
    stats = vlg_xchanTrafficStats[iStat];
    
    const char* flow_types[] =
    {
        "  MPEG",
        "  IP_U",
        "  IP_M",
        "   DSG",
        "SOCKET",
        "      ",
    };
    
    pTextStats->pszFilterType = "";
    pTextStats->pszEntryType  = flow_types[VL_SAFE_ARRAY_INDEX(iStat, flow_types)];
    pTextStats->pszClientType = "";
    pTextStats->pszClientId   = "";
    
    snprintf(pTextStats->szPkts, sizeof(pTextStats->szPkts), "%u", stats.nPkts);
    pTextStats->nOctets         = stats.nOctets; 
    pTextStats->tFirstPacket    = stats.tFirstPacket;
    pTextStats->tLastPacket     = stats.tLastPacket;
}

void vlDsgGetDsgTextStats(int nMaxStats, VL_DSG_TRAFFIC_TEXT_STATS aTextStats[VL_MAX_DSG_ENTRIES], int * pnAvailableStats)
{
    int iStat = 0;
    
    for(iStat = 0; (iStat < nMaxStats) && (iStat < VL_ARRAY_ITEM_COUNT(vlg_dsgTunnelStats)); iStat++)
    {
        if(iStat >= (vlg_DsgDirectory.nHostEntries + vlg_DsgDirectory.nCardEntries)) break;
        vlDsgGetDsgEntryStats(iStat, &(aTextStats[iStat]));
    }
    
    *pnAvailableStats = iStat;
}

void vlDsgGetXchanTextStats(VL_DSG_TRAFFIC_TEXT_STATS aTextStats[VL_XCHAN_FLOW_TYPE_MAX])
{
    int iStat = 0;
    
    for(iStat = 0; iStat < VL_ARRAY_ITEM_COUNT(vlg_xchanTrafficStats); iStat++)
    {
        vlDsgGetXchanEntryStats(iStat, &(aTextStats[iStat]));
    }
}

void vlDsgUpdateDsgCardTrafficCounters(VL_DSG_BCAST_CLIENT_ID eClientId, int nBytes)
{
    if(vlg_bEnableTrafficStats)
    {
        rmf_osal_mutexAcquire(vlDsgTunnelGetThreadLock());
        {
            int iStat = 0;
            for(iStat = 0; iStat < vlg_DsgDirectory.nHostEntries; iStat++)
            {
                int iEntry = VL_SAFE_ARRAY_INDEX(iStat, vlg_DsgDirectory.aHostEntries);
                
                if((VL_DSG_DIR_TYPE_EXT_CHNL_MPEG_FLOW == vlg_DsgDirectory.aHostEntries[iEntry].eEntryType) &&
                (VL_DSG_CLIENT_ID_ENC_TYPE_BCAST == vlg_DsgDirectory.aHostEntries[iEntry].clientId.eEncodingType) &&
                (eClientId == vlg_DsgDirectory.aHostEntries[iEntry].clientId.nEncodedId))
                {
                    vlDsgUpdateDsgTrafficCounters(FALSE, iEntry, nBytes);
                }
            }
        }
        rmf_osal_mutexRelease(vlDsgTunnelGetThreadLock());
    }
}

void vlDsgDumpDsgTrafficCountersTask(void * arg)
{
    VL_DSG_TRAFFIC_TEXT_STATS textStats;
    while(vlg_bEnableTrafficStats)
    {
        int iStat = 0;
        rmf_osal_threadSleep(15*1000, 0);
        if(!rdk_dbg_enabled("LOG.RDK.POD", VL_DSG_STATS_LOG_LEVEL)) continue;
        
        RDK_LOG(VL_DSG_STATS_LOG_LEVEL, "LOG.RDK.POD", "============================================================================================================\n");
        RDK_LOG(VL_DSG_STATS_LOG_LEVEL, "LOG.RDK.POD", " FLOW :   Type :     Path : Client :             ID :         Packets :            Bytes :           Rate \n");
        RDK_LOG(VL_DSG_STATS_LOG_LEVEL, "LOG.RDK.POD", "============================================================================================================\n");
        for(iStat = 0; iStat < VL_ARRAY_ITEM_COUNT(vlg_dsgTunnelStats); iStat++)
        {
            if(iStat >= (vlg_DsgDirectory.nHostEntries + vlg_DsgDirectory.nCardEntries)) break;
            vlDsgGetDsgEntryStats(iStat, &textStats);
            unsigned long tDiff = (textStats.tLastPacket - textStats.tFirstPacket);
            if(0 == tDiff) tDiff = 1;

            RDK_LOG(VL_DSG_STATS_LOG_LEVEL, "LOG.RDK.POD", "  DSG :   %s : %s : %s : %14s : %10s pkts : %10u bytes : %10u bps\n",
                   textStats.pszFilterType, textStats.pszEntryType, textStats.pszClientType, textStats.pszClientId,
                   textStats.szPkts, textStats.nOctets,
                   ((unsigned long)((((long long)textStats.nOctets)*((long long)8))/tDiff)));
        }
        for(iStat = 0; iStat < VL_ARRAY_ITEM_COUNT(vlg_xchanTrafficStats); iStat++)
        {
            if(VL_XCHAN_FLOW_TYPE_IP_M == iStat) continue;
            if(VL_XCHAN_FLOW_TYPE_DSG  == iStat) continue;
            if(VL_XCHAN_FLOW_TYPE_IPDM == iStat) continue;
            if(VL_XCHAN_FLOW_TYPE_IPDIRECT_UNICAST == iStat) continue;

            vlDsgGetXchanEntryStats(iStat, &textStats);
            unsigned long tDiff = labs(textStats.tLastPacket - textStats.tFirstPacket);
            if(0 == tDiff) tDiff = 1;
            
            RDK_LOG(VL_DSG_STATS_LOG_LEVEL, "LOG.RDK.POD", "XCHAN : %s :          :        : %14s : %10s pkts : %10u bytes : %10u bps\n",
                   textStats.pszEntryType, "", textStats.szPkts, textStats.nOctets,
                   ((unsigned long)((((long long)textStats.nOctets)*((long long)8))/((long long)tDiff))));
        }
        RDK_LOG(VL_DSG_STATS_LOG_LEVEL, "LOG.RDK.POD", "============================================================================================================\n");
    }

}

static void vl_DsgSetDsgModeSerializer_task(void * arg);

static int vl_dsg_unprotected_control_callback(VL_DSG_DEVICE_HANDLE_t hDevice,
                                   unsigned char * pData, unsigned long nLength);
static int vl_dsg_unprotected_tunnel_callback(VL_DSG_DEVICE_HANDLE_t hDevice,
                                  unsigned char * pData, unsigned long nLength);
static int vl_dsg_unprotected_image_callback(VL_DSG_DEVICE_HANDLE_t hDevice,
                                 unsigned char * pData, unsigned long nLength, unsigned long bEndOfFile);
static int vl_dsg_unprotected_download_status_callback(VL_DSG_DEVICE_HANDLE_t hDevice,
                                           VL_DSG_DOWNLOAD_STATE eStatus, VL_DSG_DOWNLOAD_RESULT eResult);
static int vl_dsg_unprotected_ecm_status_callback(VL_DSG_DEVICE_HANDLE_t hDevice,
                                      VL_DSG_ECM_STATUS eStatus);
static int vl_dsg_unprotected_async_data_callback(VL_DSG_DEVICE_HANDLE_t hDevice,
                                      unsigned long ulTag,
                                      unsigned long ulQualifier,
                                      void *        pStruct);

static int vl_dsg_protected_control_callback(VL_DSG_DEVICE_HANDLE_t hDevice,
                                             unsigned char * pData, unsigned long nLength)
{
    int ret = 0;
    rmf_osal_mutexAcquire(vlDsgMainGetThreadLock());
    {
        ret = vl_dsg_unprotected_control_callback(hDevice, pData, nLength);
    }
    rmf_osal_mutexRelease(vlDsgMainGetThreadLock());
    return ret;
}
static int vl_dsg_protected_tunnel_callback(VL_DSG_DEVICE_HANDLE_t hDevice,
                                            unsigned char * pData, unsigned long nLength)
{
    int ret = 0;
    rmf_osal_mutexAcquire(vlDsgTunnelGetThreadLock());
    {
        ret = vl_dsg_unprotected_tunnel_callback(hDevice, pData, nLength);
    }
    rmf_osal_mutexRelease(vlDsgTunnelGetThreadLock());
    return ret;
}

static int vl_dsg_protected_image_callback(VL_DSG_DEVICE_HANDLE_t hDevice,
                                           unsigned char * pData, unsigned long nLength, unsigned long bEndOfFile)
{
    int ret = 0;
    rmf_osal_mutexAcquire(vlDsgMainGetThreadLock());
    {
        ret = vl_dsg_unprotected_image_callback(hDevice, pData, nLength, bEndOfFile);
    }
    rmf_osal_mutexRelease(vlDsgMainGetThreadLock());
    return ret;
}

static int vl_dsg_protected_download_status_callback(VL_DSG_DEVICE_HANDLE_t hDevice,
        VL_DSG_DOWNLOAD_STATE eStatus, VL_DSG_DOWNLOAD_RESULT eResult)
{
    int ret = 0;
    rmf_osal_mutexAcquire(vlDsgMainGetThreadLock());
    {
        ret = vl_dsg_unprotected_download_status_callback(hDevice, eStatus, eResult);
    }
    rmf_osal_mutexRelease(vlDsgMainGetThreadLock());
    return ret;
}

static int vl_dsg_protected_ecm_status_callback(VL_DSG_DEVICE_HANDLE_t hDevice,
        VL_DSG_ECM_STATUS eStatus)
{
    int ret = 0;
    rmf_osal_mutexAcquire(vlDsgMainGetThreadLock());
    {
        ret = vl_dsg_unprotected_ecm_status_callback(hDevice, eStatus);
    }
    rmf_osal_mutexRelease(vlDsgMainGetThreadLock());
    return ret;
}

static int vl_dsg_protected_async_data_callback(VL_DSG_DEVICE_HANDLE_t hDevice,
        unsigned long ulTag,
        unsigned long ulQualifier,
        void *        pStruct)
{
    int ret = 0;
    rmf_osal_mutexAcquire(vlDsgMainGetThreadLock());
    {
        ret = vl_dsg_unprotected_async_data_callback(hDevice, ulTag, ulQualifier, pStruct);
    }
    rmf_osal_mutexRelease(vlDsgMainGetThreadLock());
    return ret;
}

USHORT vlGetDsgSessionNumber(unsigned short nSession)
{
    if(0 != vlg_nDsgSessionNumber)
    {
        return vlg_nDsgSessionNumber;
    }

    return nSession;
}

int  vlGetDsgFlowId()
{
    return vlg_nDsg_to_card_FlowId;
}

int  vlGetIpDmFlowId()
{
    return vlg_nIpDm_to_card_FlowId;
}

int  vlIsDsgOrIpduMode()
{
    switch(vlg_DsgMode.eOperationalMode)
    {
        case VL_DSG_OPERATIONAL_MODE_SCTE55:
        {
            return 0;
        }
        break;

        case VL_DSG_OPERATIONAL_MODE_BASIC_TWO_WAY:
        case VL_DSG_OPERATIONAL_MODE_BASIC_ONE_WAY:
        case VL_DSG_OPERATIONAL_MODE_ADV_TWO_WAY:
        case VL_DSG_OPERATIONAL_MODE_ADV_ONE_WAY:
        case VL_DSG_OPERATIONAL_MODE_IPDM: // Report IPDM as DSG.
        case VL_DSG_OPERATIONAL_MODE_IPDIRECT_UNICAST: // Report IPDIRECT_UNICAST as DSG.
        default:
        {
            return 1;
        }
        break;
    }

    return 0;
}

int  vlIsIpduMode()
{
    if (vlg_DsgMode.eOperationalMode == VL_DSG_OPERATIONAL_MODE_IPDIRECT_UNICAST)
    {
        return 1;
    }

    return 0;
}

VL_DSG_OPERATIONAL_MODE  vlGetDsgMode()
{
    return vlg_DsgMode.eOperationalMode;
}

unsigned long  vlDsgGetUcid()
{
    return vlg_ucid;
}

unsigned long vlDsgGetVctId()
{
    return vlg_nDsgVctId;
}

VL_OCRC_SUBTYPE vlGetOcrcSubtype()
{
    if(vlIsDsgOrIpduMode())
    {
        return VL_OCRC_SUBTYPE_DSG;
    }

    return VL_OCRC_SUBTYPE_OOB;
}

void  vlSetDsgFlowId(unsigned long nFlowId)
{
    vlg_nDsg_to_card_FlowId = nFlowId;
}

void  vlSetIpDmFlowId(unsigned long nFlowId)
{
    vlg_nIpDm_to_card_FlowId = nFlowId;
}

int vlDsgIsDifferentDsgMode(VL_DSG_MODE * pNewDsgMode)
{
    if(vlg_Old_DsgMode.eOperationalMode != pNewDsgMode->eOperationalMode) return 1;
    switch(pNewDsgMode->eOperationalMode)
    {
        case VL_DSG_OPERATIONAL_MODE_BASIC_TWO_WAY:
        case VL_DSG_OPERATIONAL_MODE_BASIC_ONE_WAY:
        {
            if(vlg_Old_DsgMode.nMacAddresses != pNewDsgMode->nMacAddresses)
            {
                return 1;
            }
            else
            {
                int i = 0;
                for(i = 0; i < pNewDsgMode->nMacAddresses; i++)
                {
                    if(vlg_Old_DsgMode.aMacAddresses[i] != pNewDsgMode->aMacAddresses[i])
                    {
                        return 1;
                    }
                }
            }
        }
        break;
    }

    return 0;
}

int vlDsgIsTunnelType(unsigned short nClassifierId, VL_DSG_CLIENT_ID_ENC_TYPE eClientType, unsigned long long * pEncodedId, unsigned long long * pMacAddress)
{
    int iRule;
    for(iRule = 0; (iRule < vlg_DsgDcd.nRules) && (iRule < VL_ARRAY_ITEM_COUNT(vlg_DsgDcd.aRules)); iRule++)
    {
        if(nClassifierId == vlg_DsgDcd.aRules[iRule].nClassifierId)
        {
            int iClient = 0;
            for(iClient = 0; (iClient < vlg_DsgDcd.aRules[iRule].nClients) && (iClient < VL_ARRAY_ITEM_COUNT(vlg_DsgDcd.aRules[iRule].aClients)); iClient++)
            {
                if(eClientType == vlg_DsgDcd.aRules[iRule].aClients[iClient].eEncodingType)
                {
                    *pMacAddress = vlg_DsgDcd.aRules[iRule].dsgMacAddress;
                    *pEncodedId = vlg_DsgDcd.aRules[iRule].aClients[iClient].nEncodedId;
                    return 1;
                }
            }
        }
    }
    return 0;
}

#define VL_APPLY_ADSG_FILTER_FROM_CLASSIFIER(type)                                                                                                              \
    pDsgDirectory->a##type##Entries[pDsgDirectory->n##type##Entries].adsgFilter.nTunneId        = iClassifier;                                                  \
    pDsgDirectory->a##type##Entries[pDsgDirectory->n##type##Entries].adsgFilter.nTunnelPriority = vlg_DsgDcd.aClassifiers[iClassifier].nPriority;               \
    pDsgDirectory->a##type##Entries[pDsgDirectory->n##type##Entries].adsgFilter.macAddress      = macAddress;                                                   \
    pDsgDirectory->a##type##Entries[pDsgDirectory->n##type##Entries].adsgFilter.ipAddress       = vlg_DsgDcd.aClassifiers[iClassifier].ipCpe.srcIpAddress;      \
    pDsgDirectory->a##type##Entries[pDsgDirectory->n##type##Entries].adsgFilter.ipMask          = vlg_DsgDcd.aClassifiers[iClassifier].ipCpe.srcIpMask;         \
    pDsgDirectory->a##type##Entries[pDsgDirectory->n##type##Entries].adsgFilter.destIpAddress   = vlg_DsgDcd.aClassifiers[iClassifier].ipCpe.destIpAddress;     \
    pDsgDirectory->a##type##Entries[pDsgDirectory->n##type##Entries].adsgFilter.nPortStart      = vlg_DsgDcd.aClassifiers[iClassifier].ipCpe.nPortStart;        \
    pDsgDirectory->a##type##Entries[pDsgDirectory->n##type##Entries].adsgFilter.nPortEnd        = vlg_DsgDcd.aClassifiers[iClassifier].ipCpe.nPortEnd;          \
    pDsgDirectory->a##type##Entries[pDsgDirectory->n##type##Entries].adsgFilter.hTunnelHandle   = 0;                                                            \


void vlDsgBuildBrokeredDirectory(VL_DSG_DIRECTORY * pDsgDirectory, int bOpenCaTunnels, int bOpenWkmaTunnels, int bOpenBCastTunnels, int bOpenAppTunnels)
{
    memset(pDsgDirectory, 0, sizeof( VL_DSG_DIRECTORY ));

    if((vlg_DsgDcd.nClassifiers > 0) && (vlg_DsgDcd.nRules > 0))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s: DSG BROKER: DCD is Valid. Creating DCD based Directory. \n", __FUNCTION__);
        int iClassifier = 0;
        for(iClassifier = 0; (iClassifier < vlg_DsgDcd.nClassifiers) && (iClassifier < VL_ARRAY_ITEM_COUNT(vlg_DsgDcd.aClassifiers)); iClassifier++)
        {
            unsigned long long macAddress = 0;
            unsigned long long nEncodedId = 0;
            if(bOpenCaTunnels && vlDsgIsTunnelType(vlg_DsgDcd.aClassifiers[iClassifier].nId, VL_DSG_CLIENT_ID_ENC_TYPE_CAS, &nEncodedId, &macAddress))
            {
                if(pDsgDirectory->nCardEntries < VL_ARRAY_ITEM_COUNT(pDsgDirectory->aCardEntries))
                {
                    VL_APPLY_ADSG_FILTER_FROM_CLASSIFIER(Card);
                    pDsgDirectory->nCardEntries++;
                }
            }
            if(bOpenWkmaTunnels && vlDsgIsTunnelType(vlg_DsgDcd.aClassifiers[iClassifier].nId, VL_DSG_CLIENT_ID_ENC_TYPE_WKMA, &nEncodedId, &macAddress))
            {
                if(pDsgDirectory->nCardEntries < VL_ARRAY_ITEM_COUNT(pDsgDirectory->aCardEntries))
                {
                    VL_APPLY_ADSG_FILTER_FROM_CLASSIFIER(Card);
                    pDsgDirectory->nCardEntries++;
                }
            }
            if(bOpenBCastTunnels && vlDsgIsTunnelType(vlg_DsgDcd.aClassifiers[iClassifier].nId, VL_DSG_CLIENT_ID_ENC_TYPE_BCAST, &nEncodedId, &macAddress))
            {
                if(pDsgDirectory->nHostEntries < VL_ARRAY_ITEM_COUNT(pDsgDirectory->aHostEntries))
                {
                    pDsgDirectory->aHostEntries[pDsgDirectory->nHostEntries].clientId.eEncodingType   = VL_DSG_CLIENT_ID_ENC_TYPE_BCAST;
                    pDsgDirectory->aHostEntries[pDsgDirectory->nHostEntries].clientId.nEncodingLength = 2;
                    pDsgDirectory->aHostEntries[pDsgDirectory->nHostEntries].clientId.nEncodedId      = nEncodedId;
                    pDsgDirectory->aHostEntries[pDsgDirectory->nHostEntries].eEntryType               = VL_DSG_DIR_TYPE_DIRECT_TERM_DSG_FLOW;
                    pDsgDirectory->aHostEntries[pDsgDirectory->nHostEntries].ucid                     = 0;
                    VL_APPLY_ADSG_FILTER_FROM_CLASSIFIER(Host);
                    pDsgDirectory->nHostEntries++;
                }
            }
            if(bOpenAppTunnels && vlDsgIsTunnelType(vlg_DsgDcd.aClassifiers[iClassifier].nId, VL_DSG_CLIENT_ID_ENC_TYPE_APP, &nEncodedId, &macAddress))
            {
                if(pDsgDirectory->nHostEntries < VL_ARRAY_ITEM_COUNT(pDsgDirectory->aHostEntries))
                {
                    pDsgDirectory->aHostEntries[pDsgDirectory->nHostEntries].clientId.eEncodingType   = VL_DSG_CLIENT_ID_ENC_TYPE_APP;
                    pDsgDirectory->aHostEntries[pDsgDirectory->nHostEntries].clientId.nEncodingLength = 2;
                    pDsgDirectory->aHostEntries[pDsgDirectory->nHostEntries].clientId.nEncodedId      = nEncodedId;
                    pDsgDirectory->aHostEntries[pDsgDirectory->nHostEntries].eEntryType               = VL_DSG_DIR_TYPE_DIRECT_TERM_DSG_FLOW;
                    pDsgDirectory->aHostEntries[pDsgDirectory->nHostEntries].ucid                     = 0;
                    VL_APPLY_ADSG_FILTER_FROM_CLASSIFIER(Host);
                    pDsgDirectory->nHostEntries++;
                }
            }
        }
        pDsgDirectory->nInitTimeout   = vlg_DsgDcd.Config.nInitTimeout;
        pDsgDirectory->nOperTimeout   = vlg_DsgDcd.Config.nOperTimeout;
        pDsgDirectory->nTwoWayTimeout = vlg_DsgDcd.Config.nTwoWayTimeout;
        pDsgDirectory->nOneWayTimeout = vlg_DsgDcd.Config.nOneWayTimeout;
    }
    else if((0 == vlg_DsgDcd.nClassifiers) && (vlg_DsgDcd.nRules > 0))
    {
        int iRule;
        for(iRule = 0; (iRule < vlg_DsgDcd.nRules) && (iRule < VL_ARRAY_ITEM_COUNT(vlg_DsgDcd.aRules)); iRule++)
        {
            switch(vlg_DsgDcd.aRules[iRule].aClients[0].eEncodingType)
            {
                case VL_DSG_CLIENT_ID_ENC_TYPE_CAS:
                case VL_DSG_CLIENT_ID_ENC_TYPE_WKMA:
                {
                    if(bOpenCaTunnels)
                    {
                        if(pDsgDirectory->nCardEntries < VL_ARRAY_ITEM_COUNT(pDsgDirectory->aCardEntries))
                        {
                            pDsgDirectory->aCardEntries[pDsgDirectory->nCardEntries].adsgFilter.macAddress = vlg_DsgDcd.aRules[iRule].dsgMacAddress;
                            pDsgDirectory->nCardEntries++;
                        }
                    }
                }
                break;
                
                case VL_DSG_CLIENT_ID_ENC_TYPE_BCAST:
                case VL_DSG_CLIENT_ID_ENC_TYPE_APP:
                {
                    if(pDsgDirectory->nHostEntries < VL_ARRAY_ITEM_COUNT(pDsgDirectory->aHostEntries))
                    {
                        pDsgDirectory->aHostEntries[pDsgDirectory->nHostEntries].clientId.eEncodingType   = vlg_DsgDcd.aRules[iRule].aClients[0].eEncodingType;
                        pDsgDirectory->aHostEntries[pDsgDirectory->nHostEntries].clientId.nEncodingLength = vlg_DsgDcd.aRules[iRule].aClients[0].nEncodingLength;
                        pDsgDirectory->aHostEntries[pDsgDirectory->nHostEntries].clientId.nEncodedId      = vlg_DsgDcd.aRules[iRule].aClients[0].nEncodedId;
                        pDsgDirectory->aHostEntries[pDsgDirectory->nHostEntries].eEntryType               = VL_DSG_DIR_TYPE_DIRECT_TERM_DSG_FLOW;
                        pDsgDirectory->aHostEntries[pDsgDirectory->nHostEntries].ucid                     = 0;
                        pDsgDirectory->aHostEntries[pDsgDirectory->nHostEntries].adsgFilter.macAddress    = vlg_DsgDcd.aRules[iRule].dsgMacAddress;
                        pDsgDirectory->nHostEntries++;
                    }
                }
                break;
                
                default:
                {
                    if(pDsgDirectory->nCardEntries < VL_ARRAY_ITEM_COUNT(pDsgDirectory->aCardEntries))
                    {
                        pDsgDirectory->aCardEntries[pDsgDirectory->nCardEntries].adsgFilter.macAddress = vlg_DsgDcd.aRules[iRule].dsgMacAddress;
                        pDsgDirectory->nCardEntries++;
                    }
                }
                break;
            }
        }
        pDsgDirectory->nInitTimeout   = vlg_DsgDcd.Config.nInitTimeout;
        pDsgDirectory->nOperTimeout   = vlg_DsgDcd.Config.nOperTimeout;
        pDsgDirectory->nTwoWayTimeout = vlg_DsgDcd.Config.nTwoWayTimeout;
        pDsgDirectory->nOneWayTimeout = vlg_DsgDcd.Config.nOneWayTimeout;
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s: DSG BROKER: DCD is Invalid or empty. Creating hard-coded Directory. \n", __FUNCTION__);
        pDsgDirectory->nCardEntries = 1;
        pDsgDirectory->aCardEntries[0].adsgFilter.macAddress = 0x01005E010101LL;
        // use optimized params for to meet always-on requirement.
        pDsgDirectory->nInitTimeout   = 2;
        pDsgDirectory->nOperTimeout   = 150;
        pDsgDirectory->nTwoWayTimeout =  0;
        pDsgDirectory->nOneWayTimeout = 150;
    }
}

void  vl_dsg_broker_task(void * arg)
{
    static VL_DSG_MODE dsgMode;
    static VL_DSG_DIRECTORY dsgDirectory;
    VL_DSG_MODE * pDsgMode = &dsgMode;
    VL_DSG_DIRECTORY * pDsgDirectory = &vlg_DsgDirectory;
    
    memset(pDsgMode,0,sizeof(*pDsgMode));
   
    rmf_osal_threadSleep(VL_DSG_BROKER_STATUS_CHECK_INTERVAL, 0);  

    pDsgMode->eOperationalMode = VL_DSG_OPERATIONAL_MODE_ADV_TWO_WAY;
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: DSG BROKER: Started service.\n", __FUNCTION__);
    CHALDsg_Set_Config(VL_CM_DSG_DEVICE_INSTANCE, VL_POD2HOST_SET_DSG_MODE, pDsgMode);
    
    int bOpenCaTunnels     = vl_env_get_bool(0, "FEATURE.DSG_BROKER.OPEN_CA_TUNNELS"); //Do not open CA tunnels by default.
    int bOpenWkmaTunnels   = vl_env_get_bool(0, "FEATURE.DSG_BROKER.OPEN_WKMA_TUNNELS"); //Do not open CA tunnels by default.
    int bOpenBCastTunnels  = vl_env_get_bool(1, "FEATURE.DSG_BROKER.OPEN_BCAST_TUNNELS");
    int bOpenAppTunnels    = vl_env_get_bool(0, "FEATURE.DSG_BROKER.OPEN_APP_TUNNELS");
    
    vlg_bDsgBrokerDeliverCaSTT     = vl_env_get_bool(0, "FEATURE.DSG_BROKER.DELIVER_CA_STT");
    vlg_bDsgBrokerDeliverCaEAS     = vl_env_get_bool(0, "FEATURE.DSG_BROKER.DELIVER_CA_EAS");
    vlg_bDsgBrokerDeliverCaXAIT    = vl_env_get_bool(0, "FEATURE.DSG_BROKER.DELIVER_CA_XAIT");
    vlg_bDsgBrokerDeliverCaCVT     = vl_env_get_bool(0, "FEATURE.DSG_BROKER.DELIVER_CA_CVT");
    vlg_bDsgBrokerDeliverCaSI      = vl_env_get_bool(0, "FEATURE.DSG_BROKER.DELIVER_CA_SI");
    
    vlg_bDsgBrokerDeliverOnlyBcast = vl_env_get_bool(0, "FEATURE.DSG_BROKER.DELIVER_ONLY_BCAST");
    
    vlg_bDsgBrokerDeliverBcastSTT  = vl_env_get_bool(0, "FEATURE.DSG_BROKER.DELIVER_BCAST_STT");
    vlg_bDsgBrokerDeliverBcastEAS  = vl_env_get_bool(0, "FEATURE.DSG_BROKER.DELIVER_BCAST_EAS");
    vlg_bDsgBrokerDeliverBcastXAIT = vl_env_get_bool(0, "FEATURE.DSG_BROKER.DELIVER_BCAST_XAIT");
    vlg_bDsgBrokerDeliverBcastCVT  = vl_env_get_bool(0, "FEATURE.DSG_BROKER.DELIVER_BCAST_CVT");
    vlg_bDsgBrokerDeliverBcastSI   = vl_env_get_bool(0, "FEATURE.DSG_BROKER.DELIVER_BCAST_SI");
    
    vlg_bDsgBrokerDeliverAnySTT    = vl_env_get_bool(1, "FEATURE.DSG_BROKER.DELIVER_ANY_STT");
    vlg_bDsgBrokerDeliverAnyEAS    = vl_env_get_bool(0, "FEATURE.DSG_BROKER.DELIVER_ANY_EAS");
    vlg_bDsgBrokerDeliverAnyXAIT   = vl_env_get_bool(0, "FEATURE.DSG_BROKER.DELIVER_ANY_XAIT");
    vlg_bDsgBrokerDeliverAnyCVT    = vl_env_get_bool(0, "FEATURE.DSG_BROKER.DELIVER_ANY_CVT");
    vlg_bDsgBrokerDeliverAnySI     = vl_env_get_bool(0, "FEATURE.DSG_BROKER.DELIVER_ANY_SI");
    
    while(1)
    {
        rmf_osal_threadSleep(VL_DSG_BROKER_STATUS_CHECK_INTERVAL*1000, 0);
        
        rmf_osal_mutexAcquire(vlDsgTunnelGetThreadLock());
        {
            if((vlg_tLastOneWayResponse >= vlg_tLastOneWayNotification) &&
               (vlg_tLastDcdResponse    >= vlg_tLastDcdNotification   ))
            {
                // All is well.
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: DSG BROKER: Cable-Card response received.\n", __FUNCTION__);
            }
            else
            {
                unsigned int tCurrentTime = rmf_osal_GetUptime();
                if(((tCurrentTime - vlg_tLastOneWayNotification) < VL_DSG_BROKER_RESPONSE_TIME_WINDOW) &&
                   ((tCurrentTime - vlg_tLastDcdNotification   ) < VL_DSG_BROKER_RESPONSE_TIME_WINDOW))
                {
                    // All is well.
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: DSG BROKER: Awaiting Cable-Card response.\n", __FUNCTION__);
                }
                else
                {
                    // Recover with cached information.
                    if((vlg_DsgDirectory.nHostEntries > 0) ||
                       (vlg_DsgDirectory.nCardEntries > 0))
                    {
                        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s: DSG BROKER: Cable-Card did not respond. Issuing Cached DIRECTORY response. \n", __FUNCTION__);
                        vlDsgApplyDsgDirectory();
                    }
                    else if(vlg_DsgAdvConfig.nTunnelFilters > 0)
                    {
                        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s: DSG BROKER: Cable-Card did not respond. Issuing Cached ADV CONFIG response. \n", __FUNCTION__);
                        CHALDsg_Set_Config(VL_CM_DSG_DEVICE_INSTANCE, VL_XCHAN_POD2HOST_CONFIG_ADV_DSG, &vlg_DsgAdvConfig);
                    }
                    else // Recovery by dsg broker.
                    {
                        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s: DSG BROKER: Cable-Card did not respond. Issuing BROKERED response. \n", __FUNCTION__);
                        vlDsgBuildBrokeredDirectory(pDsgDirectory, bOpenCaTunnels, bOpenWkmaTunnels, bOpenBCastTunnels, bOpenAppTunnels);
                        vlg_bDsgDirectoryFilterValid = vlg_bDsgBrokerDeliverOnlyBcast; // block tunnel delivery per normal paths.
                        vlg_tLastDcdResponse = rmf_osal_GetUptime();
                        vlg_tLastOneWayResponse = vlg_tLastDcdResponse;
                        
                        CHALDsg_Set_Config(VL_CM_DSG_DEVICE_INSTANCE, VL_POD2HOST_DSG_DIRECTORY, pDsgDirectory);
                        vlPrintDsgDirectory(0, pDsgDirectory);
                    }
                }
            }
        }
        rmf_osal_mutexRelease(vlDsgTunnelGetThreadLock());
    }
    
}

int  vlDsgHalSupportsIpRecovery()
{
    return (VL_DSG_RESULT_HAL_SUPPORTS == CHALDsg_Set_Config(VL_CM_DSG_DEVICE_INSTANCE, VL_DSG_PVT_TAG_GET_IP_RECOVERY_CAPS, 0));
}

rmf_osal_Bool podmgrDsgEcmIsOperational()
{
    return (VL_DSG_ECM_STATUS_OK == CHALDsg_Set_Config(VL_CM_DSG_DEVICE_INSTANCE, VL_DSG_PVT_TAG_GET_IS_ECM_OPERATIONAL, 0));
}

int  vlDsgAttemptIpRecovery()
{
    return (VL_DSG_ECM_STATUS_OK == CHALDsg_Set_Config(VL_CM_DSG_DEVICE_INSTANCE, VL_DSG_PVT_TAG_SET_ATTEMPT_IP_RECOVERY, 0));
}

unsigned int vlDsgGetIpv4Gateway()
{
    static FILE *fp = NULL;
    char * pszLine = NULL;
    char szLine[VL_DSG_MAX_STR_SIZE], szBuf[32];
    unsigned int vlg_dsgIpv4Gateway = 0;
    if(NULL == fp)
    {
        fp = fopen("/proc/net/route", "r");
        if (NULL != fp)
        {
            setvbuf(fp, (char *) NULL, _IONBF, 0);
        }
    }
    if (NULL != fp)
    {
        fseek(fp, 0, SEEK_SET);
        while(!feof(fp))
        {
            pszLine = fgets(szLine, sizeof(szLine), fp);
            if(NULL == pszLine)
            {
                break;
            }
            else // not NULL
            {
                sscanf(pszLine, "%s%s%x", szBuf, szBuf, &vlg_dsgIpv4Gateway);
                if(0 != vlg_dsgIpv4Gateway) return htonl(vlg_dsgIpv4Gateway);
            }
        }
        // not closing fp as fp is static.
    }
    return vlg_dsgIpv4Gateway;
}

unsigned short vlDsgCalculateIpChecksum(unsigned char *pData, int len)
{
    unsigned long nCalculatedChecksum = 0;
    int i = 0;
    // calculate checksum for header
    for(i = 0; i < (((len)/2)*2); i+=2)
    {
        nCalculatedChecksum += (VL_VALUE_2_FROM_ARRAY(pData));
        nCalculatedChecksum += (nCalculatedChecksum>>16);
        nCalculatedChecksum &= 0xFFFF;
        pData += 2;
    }

    // take the one's complement
    nCalculatedChecksum ^= 0xFFFF;
    // mask out the MSBs
    nCalculatedChecksum &= 0xFFFF;
    
    return (unsigned short)nCalculatedChecksum;
}

int vlDsgCheckIpv4GatewayConnectivity()
{
    if(!vlg_bHostAcquiredIPAddress || !vlg_bAcquiredIpv4Address || !vlg_dsgIpv4Address)
    {
        // Assume connectivity is OK if IPv4 address is not present.
        // i.e. this function cannot determine if IP connectivity is OK if IPv4 address is absent.
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s : No IPv4 address yet. Assuming connectivity is present.\n", __FUNCTION__);
        return 1;
    }
    
    if(0 == vlg_dsgIpv4Gateway)
    {
        vlg_dsgIpv4Gateway = vlDsgGetIpv4Gateway();
    }
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s : GW = %u.%u.%u.%u.\n", __FUNCTION__, ((vlg_dsgIpv4Gateway>>24)&0xFF), ((vlg_dsgIpv4Gateway>>16)&0xFF), ((vlg_dsgIpv4Gateway>>8)&0xFF), ((vlg_dsgIpv4Gateway>>0)&0xFF));
    if(0 == vlg_dsgIpv4Gateway)
    {
        // Gateway absent so return failure.   
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s : GW = %u.%u.%u.%u.\n", __FUNCTION__, ((vlg_dsgIpv4Gateway>>24)&0xFF), ((vlg_dsgIpv4Gateway>>16)&0xFF), ((vlg_dsgIpv4Gateway>>8)&0xFF), ((vlg_dsgIpv4Gateway>>0)&0xFF));
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s : No IPv4 gateway present. Assuming connectivity is absent.\n", __FUNCTION__);
        return 0;
    }
    
    // Setup static stuff for reuse.
    static struct iphdr* request_packet = (struct iphdr*  ) malloc(sizeof(struct iphdr) + sizeof(struct icmphdr));
    static struct iphdr* reply_packet   = (struct iphdr*  ) malloc(sizeof(struct iphdr) + sizeof(struct icmphdr));
    static struct icmphdr* icmp_request = (struct icmphdr*) (request_packet+1);
    static struct icmphdr* icmp_reply   = (struct icmphdr*) (reply_packet  +1);
    static struct sockaddr_in gw_address;
    static int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    static int bIncludeHeader = 0;
    static socklen_t addrlen = sizeof(gw_address);
    static int iSeq = 0;
    const  unsigned short sEchoId = 0xABCD;
    int request_size = 0;
    int reply_size   = 0;
    
    if (sock == -1)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s : sock = -1.\n", __FUNCTION__);
        return 1; // Assume connectivity is OK.
    }
    
    if(0 == bIncludeHeader)
    {
        // Set sock options to use supplied IP header. i.e. the buffer passed to sendto/recvfrom does not begin with payload.
        bIncludeHeader = 1;
        if ( setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &bIncludeHeader, sizeof(int)) < 0)
        {
            /* Unhandled error */
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s : setsockopt failed\n", __FUNCTION__);
        }
        // Set static request parameters
        request_packet->ihl        = 5;
        request_packet->version    = 4;
        request_packet->tos        = 0;
        request_packet->tot_len    = sizeof(struct iphdr) + sizeof(struct icmphdr);
        request_packet->id         = htons(0);
        request_packet->frag_off   = 0;
        request_packet->ttl        = 8; // actually not expecting more than one hop.
        request_packet->protocol   = IPPROTO_ICMP;
         
        // Set static ICMP parameters
        icmp_request->type           = ICMP_ECHO;
        icmp_request->code           = 0;
        icmp_request->un.echo.id     = sEchoId;
    }
 
    // Set dynamic request parameters
    request_packet->saddr      = htonl(INADDR_ANY);//vlg_dsgIpv4Address is not reliable as it could change dynamically. Use INADDR_ANY instead.
    //request_packet->saddr      = htonl(vlg_dsgIpv4Address);
    request_packet->daddr      = htonl(vlg_dsgIpv4Gateway);
    request_packet->check      = 0; // clear old checksum.
    request_packet->check      = htons(vlDsgCalculateIpChecksum((unsigned char*)request_packet, sizeof(struct iphdr)));
    
    // Set destination addresses
    gw_address.sin_family      = AF_INET;
    gw_address.sin_addr.s_addr = htonl(vlg_dsgIpv4Gateway);

    // Set dynamic ICMP parameters
    icmp_request->un.echo.sequence  = htons(iSeq++);
    icmp_request->checksum          = 0; // clear old checksum.
    icmp_request->checksum          = htons(vlDsgCalculateIpChecksum((unsigned char*)icmp_request, sizeof(struct icmphdr)));
    
    // Send the request
    if((request_size = sendto(sock, request_packet, request_packet->tot_len, 0, (struct sockaddr *)&gw_address, sizeof(struct sockaddr))) == -1)
    {
        char szErrBuf[VL_DSG_MAX_STR_SIZE];
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s : sendto returned = '%s'.\n", __FUNCTION__, strerror_r(errno, szErrBuf, sizeof(szErrBuf)));
        // return failure
        return 0;
    }
    
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s : Sent %d byte packet to: %u.%u.%u.%u.\n", __FUNCTION__, request_packet->tot_len, ((vlg_dsgIpv4Gateway>>24)&0xFF), ((vlg_dsgIpv4Gateway>>16)&0xFF), ((vlg_dsgIpv4Gateway>>8)&0xFF), ((vlg_dsgIpv4Gateway>>0)&0xFF));

    rmf_osal_threadSleep(100, 0);
    while(1)
    {
        // Check for response
        if((reply_size = recvfrom(sock, reply_packet, sizeof(struct iphdr) + sizeof(struct icmphdr), MSG_DONTWAIT, (struct sockaddr *)&gw_address, &addrlen)) == -1)
        {
            char szErrBuf[VL_DSG_MAX_STR_SIZE];
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s : recvfrom returned = '%s'.\n", __FUNCTION__, strerror_r(errno, szErrBuf, sizeof(szErrBuf)));
            // return failure
            return 0;
        }
        else
        {
            unsigned int resp_address = ntohl(reply_packet->saddr);
            //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s : Received %d byte reply from: %u.%u.%u.%u\n", __FUNCTION__, reply_size, ((resp_address>>24)&0xFF), ((resp_address>>16)&0xFF), ((resp_address>>8)&0xFF), ((resp_address>>0)&0xFF));
            if((vlg_dsgIpv4Gateway             == resp_address                ) &&
               (IPPROTO_ICMP                   == reply_packet->protocol      ) &&
               (ICMP_ECHOREPLY                 == icmp_reply->type            ) &&
               (sEchoId                        == icmp_reply->un.echo.id      ) &&
               (icmp_request->un.echo.sequence == icmp_reply->un.echo.sequence))
            {
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s : Received expected reply to ping from: %u.%u.%u.%u.\n", __FUNCTION__, ((resp_address>>24)&0xFF), ((resp_address>>16)&0xFF), ((resp_address>>8)&0xFF), ((resp_address>>0)&0xFF));
                // return success
                return 1;
            }
            else
            {
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s : Received UNEXPECTED reply from: %u.%u.%u.%u.\n", __FUNCTION__, ((resp_address>>24)&0xFF), ((resp_address>>16)&0xFF), ((resp_address>>8)&0xFF), ((resp_address>>0)&0xFF));
                // Caution no return statement here. Not expecting an infinite loop as we expect UNEXPECTED ICMP replies to dry-up quickly.
            }
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s : REPLY_ID       : %d\n"    , __FUNCTION__, ntohs(reply_packet->id)             );
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s : REPLY_TTL      : %d\n"    , __FUNCTION__, reply_packet->ttl                   );
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s : REPLY_PROTOCOL : %d\n"    , __FUNCTION__, request_packet->protocol            );
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s : ICMP_TYPE      : %d\n"    , __FUNCTION__, icmp_reply->type                    );
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s : ICMP_CODE      : %d\n"    , __FUNCTION__, icmp_reply->code                    );
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s : ECHO_ID        : 0x%04X\n", __FUNCTION__, icmp_reply->un.echo.id              );
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s : ECHO_SEQUENCE  : %d\n"    , __FUNCTION__, ntohs(icmp_reply->un.echo.sequence) );
        }
    }
    
    // return failure
    return 0;
}

int vlDsgResetEcm()
{
    struct stat st;
    int ret=0;

    if((0 == stat(VL_DSG_ECM_RESET_BINARY, &st) && (0 != st.st_ino)))
    {
         ret=1;
         RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: '%s' is available. Invoking binary to reset eCM hardware.\n",__FUNCTION__, VL_DSG_ECM_RESET_BINARY);
         system(VL_DSG_ECM_RESET_BINARY " &");
    }
    else if((0 == stat(VL_DSG_ALT_ECM_RESET_BINARY, &st) && (0 != st.st_ino)))
    {
         ret=1;
         RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: '%s' is available. Invoking binary to reset eCM hardware.\n",__FUNCTION__, VL_DSG_ALT_ECM_RESET_BINARY);
         system(VL_DSG_ALT_ECM_RESET_BINARY " &");
    }
    else
    {
         ret=0;
         RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: Neither '%s' nor '%s' are available. Defaulting to eCM state reset.\n",__FUNCTION__, VL_DSG_ECM_RESET_BINARY, VL_DSG_ALT_ECM_RESET_BINARY);
    }
    return ret;
}

bool VL_DSG_CheckIfFlagIsSet(const char * pszFlag)
{
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

void vlDsgCheckAndRecoverConnectivity()
{
    if(vlDsgHalSupportsIpRecovery())
    {
        if(podmgrDsgEcmIsOperational())
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s : ECM is Operational.\n", __FUNCTION__);
        }
        else
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s : ECM is NOT Operational.\n", __FUNCTION__);
        }
        if(vlDsgCheckIpv4GatewayConnectivity())
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s : IPv4 connectivity is OK.\n", __FUNCTION__);
        }
        else
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s : IPv4 connectivity is NOT OK.\n", __FUNCTION__);
        }
        
        static int nMinFailDuration               = vl_env_get_int(VL_DSG_IP_RECOVERY_MIN_APP_FAIL_DURATION, "FEATURE.DSG_IP_RECOVERY.MIN_APP_FAIL_DURATION");
        static int nMinRecoveryInterval           = vl_env_get_int(VL_DSG_IP_RECOVERY_MIN_APP_RECOVERY_INTERVAL, "FEATURE.DSG_IP_RECOVERY.MIN_APP_RECOVERY_INTERVAL");
        static int nMinNotificationThreshold      = vl_env_get_int(VL_DSG_IP_RECOVERY_MIN_APP_NOTIFICATION_THRESHOLD, "FEATURE.DSG_IP_RECOVERY.MIN_APP_NOTIFICATION_THRESHOLD");
        static time_t tLastRecoveryAttempt        = 0;
        static time_t tLastProblemRegistration    = 0;
        static time_t tLastProblemNotification    = 0;
        
        if(0 == tLastRecoveryAttempt    ) tLastRecoveryAttempt      = vlg_tBootTime;
        if(0 == tLastProblemRegistration) tLastProblemRegistration  = vlg_tBootTime;
        
        time_t nCurrentUpTime                     = rmf_osal_GetUptime();
        time_t nSecsSinceLastProblemNotification  = vlAbs(nCurrentUpTime - tLastProblemNotification);

        if(nSecsSinceLastProblemNotification > nMinNotificationThreshold)
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s : Registering a problem since %d seconds elapsed since last notification which is greater than a threshold of %d seconds.\n", __FUNCTION__, nSecsSinceLastProblemNotification, nMinNotificationThreshold);
            tLastProblemRegistration = nCurrentUpTime;
        }
        
        tLastProblemNotification                  = nCurrentUpTime;
        time_t nSecsSinceLastProblemRegistration  = vlAbs(nCurrentUpTime - tLastProblemRegistration);
        
        if(nSecsSinceLastProblemRegistration > nMinFailDuration)
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s : Problem peristed for %d seconds which is more than %d seconds allowed.\n", __FUNCTION__, nSecsSinceLastProblemRegistration, nMinFailDuration);
            time_t nSecsSinceLastRecovery = vlAbs(nCurrentUpTime - tLastRecoveryAttempt);
            if((int)nSecsSinceLastRecovery > nMinRecoveryInterval)
            {
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s : Last recovery attempt was %d seconds ago... Attempting DOCSIS recovery.\n", __FUNCTION__, nSecsSinceLastRecovery);
                tLastRecoveryAttempt = nCurrentUpTime;
                
                if(VL_DSG_CheckIfFlagIsSet(VL_CDL_DEV_INITIATED_RCDL_IN_PROGRESS))
                {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s : RCDL is in progress. Skipping IP recovery.\n", __FUNCTION__);
                    return;
                }
                if(VL_DSG_CheckIfFlagIsSet(VL_CDL_ECM_INITIATED_CDL_IN_PROGRESS))
                {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s : CDL is in progress. Skipping IP recovery.\n", __FUNCTION__);
                    return;
                }
                if(vlDsgResetEcm())
                {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s : Used eCM H/W Reset.\n", __FUNCTION__);
                }
                else
                {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s : eCM H/W Reset not available. Using DOCSIS state reset instead.\n", __FUNCTION__);
		     nMinRecoveryInterval = vl_env_get_int(VL_DSG_IP_RECOVERY_MIN_APP_RECOVERY_INTERVAL_UPDATED, "FEATURE.DSG_IP_RECOVERY.MIN_APP_RECOVERY_INTERVAL_UPDATED");
		     RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s : DOCSIS reset attempted , updating nMinRecoveryInterval to %d \n", __FUNCTION__,nMinRecoveryInterval);
                    vlDsgAttemptIpRecovery();
                }
            }
            else
            {
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s : Last recovery attempt was %d seconds ago which is less than minimum of %d seconds.\n", __FUNCTION__, nSecsSinceLastRecovery, nMinRecoveryInterval);
            }
        }
        else
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s : Problem peristed for %d seconds which is less than %d seconds needed to confirm that there is really a problem.\n", __FUNCTION__, nSecsSinceLastProblemRegistration, nMinFailDuration);
        }
    }
    else
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s : HAL does not support IP Recovery.\n", __FUNCTION__);
    }
}

void vl_dsg_ip_recovery_task(void * arg)
{
    int bDsgIpRecoveryEnabled  = vl_env_get_bool(0, "FEATURE.ENABLE_DSG_IP_RECOVERY");
    int bCheckIpV4Connectivity = vl_env_get_bool(0, "FEATURE.DSG_IP_RECOVERY.CHECK_IPV4_CONNECTIVITY");
    int bCheckIpV6Connectivity = vl_env_get_bool(0, "FEATURE.DSG_IP_RECOVERY.CHECK_IPV6_CONNECTIVITY");
    int nMaxRecoveryAttempts   = vl_env_get_int(0, "FEATURE.DSG_IP_RECOVERY.MAX_RECOVERY_ATTEMPTS");
    int nMinFailDuration       = vl_env_get_int(VL_DSG_IP_RECOVERY_MIN_FAIL_DURATION, "FEATURE.DSG_IP_RECOVERY.MIN_FAIL_DURATION");
    int nOobModeMinFailDuration= vl_env_get_int(VL_OOB_MODE_MIN_FAIL_DURATION, "FEATURE.CABLE_CARD_ERROR_RECOVERY.MIN_FAIL_DURATION");
    int nDsgIpRecoveryRebootAttempts = vl_env_get_int(1, "FEATURE.DSG_IP_RECOVERY.MAX_REBOOT_ATTEMPTS");
    int nCableCardRecoveryRebootAttempts = vl_env_get_int(1, "FEATURE.CABLE_CARD_ERROR_RECOVERY.MAX_REBOOT_ATTEMPTS");
    int bFailedAtBootUp = 1;
#ifdef GLI
     IARM_Bus_SYSMgr_EventData_t eventData;
#endif
    if(!bDsgIpRecoveryEnabled)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s : DOCSIS IP Recovery is disabled.\n", __FUNCTION__);
        return ; // Do not perform DSG IP recovery for poor souls who do not have DOCSIS connectivity.
    }
    
    if(vlDsgHalSupportsIpRecovery())
    {
        int nFailedCount = 0;
        int nRecoveryAttempts = 0;
        time_t tLastGoodTime = time(NULL);
        time_t tLastFailTime = tLastGoodTime;
        while(1)
        {
            if((VL_DSG_OPERATIONAL_MODE_BASIC_TWO_WAY    == vlg_DsgMode.eOperationalMode) ||
               (VL_DSG_OPERATIONAL_MODE_ADV_TWO_WAY      == vlg_DsgMode.eOperationalMode) ||
               (VL_DSG_OPERATIONAL_MODE_IPDM             == vlg_DsgMode.eOperationalMode) ||
               (VL_DSG_OPERATIONAL_MODE_IPDIRECT_UNICAST == vlg_DsgMode.eOperationalMode))
            {
                if(podmgrDsgEcmIsOperational() && (!bCheckIpV4Connectivity || vlDsgCheckIpv4GatewayConnectivity()))
                {
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s : ECM is Operational and IPv4 connectivity is OK.\n", __FUNCTION__);
                    nFailedCount = 0;
                    nRecoveryAttempts = 0;
                    bFailedAtBootUp = 0;
                    tLastGoodTime = time(NULL);
                }
                else
                {
                    nFailedCount++;
                    tLastFailTime = time(NULL);
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s : ECM is non-operational for %d seconds.\n", __FUNCTION__, (VL_DSG_IP_RECOVERY_CHECK_INTERVAL*nFailedCount));
                    if((VL_DSG_IP_RECOVERY_CHECK_INTERVAL*nFailedCount) > vlMax(nMinFailDuration, (vlg_DsgDirectory.nOperTimeout+VL_DSG_IP_RECOVERY_FAIL_DURATION_OFFSET)))
                    {
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s : Attempting DOCSIS IP recovery.\n", __FUNCTION__);
                        vlg_bAcquiredIpv4Address = 0;
                        vlg_dsgIpv4Address       = 0;
                        vlg_dsgIpv4Gateway       = 0;
                        vlDsgAttemptIpRecovery();
                        nFailedCount = 0;
                        nRecoveryAttempts++;
                        if(nRecoveryAttempts > 1)
                        {
                            vlDsgSendBootupEventToPFC(RMF_BOOTMSG_DIAG_EVENT_TYPE_ESTB_IP, RMF_BOOTMSG_DIAG_MESG_CODE_ERROR, 0);
#ifdef GLI
                                eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_ESTB_IP;
				    eventData.data.systemStates.state = 0;
				    eventData.data.systemStates.error = 1;
				    IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
#endif
                        }
                        if(bFailedAtBootUp && (nRecoveryAttempts > nMaxRecoveryAttempts))
                        {
                            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s : DSG IP failed to recover for %d seconds after %d attempts. Rebooting for recovery...\n", __FUNCTION__, (tLastFailTime - tLastGoodTime), nRecoveryAttempts);
                            nFailedCount = 0;
                            rmf_RebootHostTillLimit(__FUNCTION__, "DSG IP Recovery failed for FEATURE.MAX_DSG_IP_RECOVERY_ATTEMPTS.", "dsg_ip_recovery", nDsgIpRecoveryRebootAttempts);
                        }
                    }
                }
            }
            else if(GetPodInsertedStatus() && (VL_DSG_OPERATIONAL_MODE_SCTE55 == vlg_DsgMode.eOperationalMode) && (!vlhal_oobtuner_Capable()) && (0 == HomingCardFirmwareUpgradeGetState()))
            {
                nFailedCount++;
                tLastFailTime = time(NULL);
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s : CARD is in OOB Mode for %d seconds.\n", __FUNCTION__, (VL_DSG_IP_RECOVERY_CHECK_INTERVAL*nFailedCount));
                if(bFailedAtBootUp && ((VL_DSG_IP_RECOVERY_CHECK_INTERVAL*nFailedCount) > nOobModeMinFailDuration))
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s : CARD is in OOB Mode for %d seconds. Rebooting for recovery...\n", __FUNCTION__, (tLastFailTime - tLastGoodTime));
                    nFailedCount = 0;
                    rmf_RebootHostTillLimit(__FUNCTION__, "Card stuck in OOB Mode.", "cable_card_error", nCableCardRecoveryRebootAttempts);
                }
            }
            else
            {
                nFailedCount = 0;
                nRecoveryAttempts = 0;
                tLastGoodTime = time(NULL);
                
                if(0 == GetPodInsertedStatus())
                {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s : CableCard is not inserted.\n", __FUNCTION__);
                }
            }
            
            rmf_osal_threadSleep(VL_DSG_IP_RECOVERY_CHECK_INTERVAL*1000, 0);
        }
    }
    else
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s : HAL does not support DOCSIS IP Recovery.\n", __FUNCTION__);
        return ;
    }
    return ;
}

//void * vl_dsg_xone_2106_dsgmode_timeout_task(void * arg)
//{
//    if(vlg_dsgInquiryPerformed) // was the inquiry performed as part of new dsg flow request ?
//    {
//        rmf_osal_threadSleep(5000, 0);
//        if(VL_DSG_OPERATIONAL_MODE_SCTE55 == vlg_DsgMode.eOperationalMode) // has the cable-card responded to the inquiry ?
//        {
//            HAL_SYS_Reboot(__FUNCTION__, "No response to 'inqure dsg mode apdu' for 5 seconds.");
//        }
//    }
//    
//    return NULL;
//}

static int vlDsgSampleAppCallbackFunc(unsigned long             nRegistrationId,
                         unsigned long             nAppData,
                         VL_DSG_CLIENT_ID_ENC_TYPE eClientType,
                         unsigned long long        nClientId,
                         unsigned long             nBytes,
                         unsigned char *           pData)
{
    RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgSampleAppCallbackFunc: Received App nReg = %d, Data %d, eClientType = %d, nClientId = %lld, %d Bytes, pData = 0x%08X\n", nRegistrationId, nAppData, eClientType, nClientId, nBytes, pData);
	return 1;
}

USHORT lookupDsgSessionValue(void)
{
    if (  (dsgSessionState == 1) && (vlg_nDsgSessionNumber))
        return vlg_nDsgSessionNumber;

    return 0;
}// Inline functions

static inline void setLoop(int cont)
{
    pthread_mutex_lock(&dsgLoopLock);
    if (!cont)
    {
        loop = 0;
    }
    else
    {
        loop = 1;
    }
    pthread_mutex_unlock(&dsgLoopLock);
}
// Sub-module exit point
int dsg_stop()
{
    //RDK_LOG(RDK_LOG_TRACE7 "LOG.RDK.POD", "dsg_stop: Stopping DSG Resource: Session = %d\n", vlg_nDsgSessionNumber);
    if ( (dsgSessionState) && (vlg_nDsgSessionNumber))
    {
       //don't reject but also request that the pod close (perhaps) this orphaned session
       AM_CLOSE_SS(vlg_nDsgSessionNumber);
    }

    // Terminate message loop
    setLoop(0);

    // Wait for message processing thread to join
#if 0    
    if (pthread_join(dsgThread, NULL) != 0)
    {
        MDEBUG (DPM_ERROR, "ERROR:ca: pthread_join failed.\n");
    }
#endif
    handle = 0;
	return 1;
}


// ##################### Execution Space ##################################
// below executes in the caller's execution thread space.
// this may or may-not be in DSG thread and APDU processing Space.
// can occur before or after dsg_start and dsgProcThread
//
//this was triggered from POD via appMgr bases on ResourceID.
//appMgr has translated the ResourceID to that of DSG.
void
AM_OPEN_SS_REQ_DSG(ULONG RessId, UCHAR Tcid)

{
    if (dsgSessionState != 0)
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

void vlDsgResourceInit()
{
    rmf_osal_mutexAcquire(vlDsgMainGetThreadLock());
    {
        // register for this callback here
        CHALDsg_register_callback(VL_CM_DSG_DEVICE_INSTANCE, VL_DSG_NOTIFY_ASYNC_DATA     , (void*)vl_dsg_protected_async_data_callback       );
    }
    rmf_osal_mutexRelease(vlDsgMainGetThreadLock());
}

void vlDsgResourceRegister()
{
    rmf_osal_mutexAcquire(vlDsgMainGetThreadLock());
    {
        CHALDsg_register_callback(VL_CM_DSG_DEVICE_INSTANCE, VL_DSG_NOTIFY_CONTROL_DATA   , (void*)vl_dsg_protected_control_callback          );

        CHALDsg_register_callback(VL_CM_DSG_DEVICE_INSTANCE, VL_DSG_NOTIFY_TUNNEL_DATA    , (void*)vl_dsg_protected_tunnel_callback           );
        CHALDsg_register_callback(VL_CM_DSG_DEVICE_INSTANCE, VL_DSG_NOTIFY_IMAGE_DATA     , (void*)vl_dsg_protected_image_callback            );
        CHALDsg_register_callback(VL_CM_DSG_DEVICE_INSTANCE, VL_DSG_NOTIFY_DOWNLOAD_STATUS, (void*)vl_dsg_protected_download_status_callback  );
        CHALDsg_register_callback(VL_CM_DSG_DEVICE_INSTANCE, VL_DSG_NOTIFY_ECM_STATUS     , (void*)vl_dsg_protected_ecm_status_callback       );
    }
    rmf_osal_mutexRelease(vlDsgMainGetThreadLock());
}

void vlDsgResourceCleanup()
{
    rmf_osal_mutexAcquire(vlDsgMainGetThreadLock());
    {
        {
            // revert to scte-55 mode
            unsigned char bufSetDsgModeAPDU[]={0x9F, 0x91, 0x01, 0x01, 0x00};
            APDU_POD2DSG_SetDsgMode(0, bufSetDsgModeAPDU, sizeof(bufSetDsgModeAPDU));
        }
        vlg_ucid = 0;
        vlg_nDsgVctId = 0;
        vlg_nOldDsgVctId = 0;
        vlg_bVctIdReceived = 0;
        vlg_bHostAcquiredIPAddress = 0;
        vlg_dsgDownstreamAcquired = 0;
        vlg_dsgInquiryPerformed = 0;
	memset(&vlg_Old_DsgMode,0,sizeof(vlg_Old_DsgMode));
	memset(&vlg_dsgTunnelStats,0,sizeof(vlg_dsgTunnelStats));
	memset(&vlg_xchanTrafficStats,0,sizeof(vlg_xchanTrafficStats));
vlg_Old_DsgMode.eOperationalMode = VL_DSG_OPERATIONAL_MODE_INVALID;
        memset(&vlg_DsgDirectory, 0, sizeof(vlg_DsgDirectory) );
        memset(&vlg_PreviousDsgDirectory, 0, sizeof(vlg_PreviousDsgDirectory) );
        vlg_bPartialDsgDirectory = 0;
        memset(&vlg_DsgAdvConfig, 0, sizeof(vlg_DsgAdvConfig) );
        vlDsgTerminateDispatch();
        vlSetDsgFlowId(0);
        // issue pod removed notification to DSG driver
        CHALDsg_Set_Config(VL_CM_DSG_DEVICE_INSTANCE, VL_DSG_PVT_TAG_POD_REMOVED, NULL);
    }
    rmf_osal_mutexRelease(vlDsgMainGetThreadLock());
}

//spec say we can't fail here, but we will test and reject anyway
//verify that this open is in response to our
//previous AM_OPEN_SS_RSP...SESS_STATUS_OPENED
void AM_SS_OPENED_DSG(USHORT SessNumber, ULONG lResourceId, UCHAR Tcid)

{
    // As a precaution issue a clean-up to the DSG driver
    CHALDsg_Set_Config(VL_CM_DSG_DEVICE_INSTANCE, VL_DSG_PVT_TAG_DSG_CLEAN_UP, NULL);

    if (dsgSessionState != 0)
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

    rmf_osal_mutexAcquire(vlDsgMainGetThreadLock());
    {
        ResourceId      = lResourceId;
        Tcid            = Tcid;
        vlg_nDsgSessionNumber = SessNumber;
        dsgSessionState  = 1;
        vlg_ucid = 0;
        vlg_nDsgVctId = 0;
        vlg_nOldDsgVctId = 0;
        vlg_bVctIdReceived = 0;
        vlg_bHostAcquiredIPAddress = 0;
        vlg_dsgDownstreamAcquired = 0;
        vlg_dsgInquiryPerformed = 0;
	memset(&vlg_Old_DsgMode,0,sizeof(vlg_Old_DsgMode));
    vlg_Old_DsgMode.eOperationalMode = VL_DSG_OPERATIONAL_MODE_INVALID;
	memset(&vlg_dsgTunnelStats,0,sizeof(vlg_dsgTunnelStats));
	memset(&vlg_xchanTrafficStats,0,sizeof(vlg_xchanTrafficStats));
        memset(&vlg_DsgDirectory, 0, sizeof(vlg_DsgDirectory) );
        memset(&vlg_PreviousDsgDirectory, 0, sizeof( vlg_PreviousDsgDirectory ) );
        vlg_bPartialDsgDirectory = 0;
        memset(&vlg_DsgAdvConfig, 0, sizeof(vlg_DsgAdvConfig) );
    }
    rmf_osal_mutexRelease(vlDsgMainGetThreadLock());
    //not sure we send this here, I believe its assumed to be successful
    //AM_OPEN_SS_RSP( SESS_STATUS_OPENED, ResourceId, Tcid);
    //RDK_LOG(RDK_LOG_TRACE7, "LOG.RDK.POD", "dsg_stop: DSG Resource Opened: Session = %d\n", vlg_nDsgSessionNumber);
    return;
}

//after the fact, info-ing that closure has occured.
void AM_SS_CLOSED_DSG(USHORT SessNumber)

{
    if (  (dsgSessionState)	&& (vlg_nDsgSessionNumber)
            && (vlg_nDsgSessionNumber != SessNumber )
       )
    {
        //don't reject but also request that the pod close (perhaps) this orphaned session
        AM_CLOSE_SS(vlg_nDsgSessionNumber);
    }

    //RDK_LOG(RDK_LOG_TRACE7, "LOG.RDK.POD", "dsg_stop: DSG Resource Closed: Session = %d\n", vlg_nDsgSessionNumber);

    rmf_osal_mutexAcquire(vlDsgMainGetThreadLock());
    {
        ResourceId      = 0;
        Tcid            = 0;
        tmpResourceId   = 0;
        tmpTcid         = 0;
        vlg_nDsgSessionNumber = 0;
        dsgSessionState  = 0;

        vlDsgResourceCleanup();
    }
    rmf_osal_mutexRelease(vlDsgMainGetThreadLock());
    vlDsgProxyServer_Deregister("Card Removed");
}// ##################### Execution Space ##################################
// ************************* Session Stuff **************************

// Sub-module entry point
int dsg_start(LCSM_DEVICE_HANDLE h)
{
    int res;
    vlg_tBootTime = rmf_osal_GetUptime();
    
    vlParseDsgInit();
    rmf_osal_ThreadId ThreadIdTC, ThreadIdFDM;
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
    if (pthread_mutex_init(&dsgLoopLock, NULL) != 0)
    {
       // MDEBUG (DPM_ERROR, "ERROR:cardres: Unable to initialize mutex.\n");
        return EXIT_FAILURE;
    }

    setLoop(1);

    rmf_osal_mutexAcquire(vlDsgMainGetThreadLock());
    {
        // Initialize all configs to default values
        rmf_osal_mutexAcquire(vlDsgTunnelGetThreadLock());
        {
	    memset(&vlg_Old_DsgMode,0,sizeof(vlg_Old_DsgMode));
            vlg_Old_DsgMode.eOperationalMode = VL_DSG_OPERATIONAL_MODE_INVALID;
	    memset(&vlg_DsgMode,0,sizeof(vlg_DsgMode));
	    memset(&vlg_DsgDirectory,0,sizeof(vlg_DsgDirectory));
		memset(&vlg_PreviousDsgDirectory,0, sizeof(vlg_PreviousDsgDirectory));
            vlg_bPartialDsgDirectory = 0;
	    memset(&vlg_DsgAdvConfig,0,sizeof(vlg_DsgAdvConfig));
	    memset(&vlg_DsgDcd,0,sizeof(vlg_DsgDcd));
	    memset(&vlg_DsgError,0,sizeof(vlg_DsgError));
	    memset(&vlg_dsgTunnelStats,0,sizeof(vlg_dsgTunnelStats));
	    memset(&vlg_xchanTrafficStats,0,sizeof(vlg_xchanTrafficStats));
            vlg_bDsgBasicFilterValid = 0;
            vlg_bDsgConfigFilterValid = 0;
            vlg_bDsgDirectoryFilterValid = 0;
        }
        rmf_osal_mutexRelease(vlDsgTunnelGetThreadLock());

        // initialize all vars to default values
        vlg_ucid = 0;
        vlg_nDsgVctId = 0;
        vlg_nOldDsgVctId = 0;
        vlg_bVctIdReceived = 0;
        vlg_bHostAcquiredIPAddress = 0;
        vlg_dsgDownstreamAcquired = 0;
        vlg_dsgInquiryPerformed = 0;

        vlDsgResourceInit();
    }
    rmf_osal_mutexRelease(vlDsgMainGetThreadLock());

    vlDsgProxyServer_Start();
    /*
    int nRegistrationId = vlDsgRegisterClient(vlDsgSampleAppCallbackFunc, 0,
                        VL_DSG_CLIENT_ID_ENC_TYPE_APP, 100);
    RDK_LOG(RDK_LOG_TRACE8, "LOG.RDK.POD", "dsg_start: vlDsgRegisterClient returned registration id = %d\n", nRegistrationId);
    */
    vlg_bEnableDsgBroker  = vl_env_get_bool(0, "FEATURE.DSG_BROKER.ENABLE_DSG_BROKER");
    if(vlg_bEnableDsgBroker)
    {
        vlDsgResourceRegister();        				
        rmf_osal_threadCreate(vl_dsg_broker_task, NULL, 0, 0, &ThreadIdFDM, "vl_dsg_broker_task");
    }
    // Enable counters by default // if(rdk_dbg_enabled("LOG.RDK.POD", VL_DSG_STATS_LOG_LEVEL))
    vlg_bEnableTrafficStats = 1;
       
    rmf_osal_threadCreate(vlDsgDumpDsgTrafficCountersTask, NULL, 0, 0, &ThreadIdTC, "vlDsgDumpDsgTrafficCountersTask"); 

    rmf_osal_threadCreate(vl_dsg_ip_recovery_task, NULL, 0, 0, &ThreadIdTC, "vl_dsg_ip_recovery_task"); 
    rmf_osal_eventqueue_create ( (const uint8_t *)"VL_DSG_MODE_MSG_Q",
		&vlg_dsgModeSerializerMsgQ);
    rmf_osal_threadCreate(vl_DsgSetDsgModeSerializer_task, NULL, 0, 0, &ThreadIdTC, "vl_DsgSetDsgModeSerializer_task"); 
    return EXIT_SUCCESS;
}

void dsg_init( )
{
    REGISTER_RESOURCE(M_DSG_ID, POD_DSG_QUEUE, 1);  // (resourceId, queueId, maxSessions )
}

void dsg_test( )
{
    RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", " Test ##### dsg_test \n");
}

int vlDsgIsValidUdpChecksum(VL_DSG_IP_HEADER * pIpHeader, VL_DSG_UDP_HEADER * pUdpHeader,
                                 unsigned char * pPayload, unsigned long nPayLoadLength)
{
    static unsigned char udpChecksumHeader[32];
    static unsigned char padding[2];
    unsigned char *  pData      = udpChecksumHeader;
    VL_BYTE_STREAM_ARRAY_INST(udpChecksumHeader, WriteBuf);
    int nFinalLength = 0, i = 0;
    unsigned long nHeaderChecksum = 0;
    unsigned long nCalculatedChecksum = 0;
    if(0 == pUdpHeader->nChecksum) return 1;
    if(0xFFFF == pUdpHeader->nChecksum) nHeaderChecksum = 0;
    if(0xFFFF != pUdpHeader->nChecksum) nHeaderChecksum = pUdpHeader->nChecksum;

    nFinalLength += vlWriteLong (pWriteBuf, pIpHeader->srcIpAddress);
    nFinalLength += vlWriteLong (pWriteBuf, pIpHeader->dstIpAddress);
    nFinalLength += vlWriteByte (pWriteBuf, 0                      );
    nFinalLength += vlWriteByte (pWriteBuf, pIpHeader->eProtocol   );
    nFinalLength += vlWriteShort(pWriteBuf, nPayLoadLength         );

    // calculate checksum for pseudo-header
    pData = udpChecksumHeader;
    for(i = 0; i < nFinalLength; i+=2)
    {
        nCalculatedChecksum += (VL_VALUE_2_FROM_ARRAY(pData));
        nCalculatedChecksum += (nCalculatedChecksum>>16);
        nCalculatedChecksum &= 0xFFFF;
        pData += 2;
    }

    // calculate checksum for payload
    pData = pPayload;
    for(i = 0; i < ((nPayLoadLength/2)*2); i+=2)
    {
        if(i != 6) // skip the current checksum
        {
            nCalculatedChecksum += (VL_VALUE_2_FROM_ARRAY(pData));
            nCalculatedChecksum += (nCalculatedChecksum>>16);
            nCalculatedChecksum &= 0xFFFF;
        }
        pData += 2;
    }

    // calculate checksum for trailer
    pData = padding;
    if(1 == nPayLoadLength%2)
    {
        padding[0] = pPayload[nPayLoadLength-1];
        padding[1] = 0;
        nCalculatedChecksum += (VL_VALUE_2_FROM_ARRAY(pData));
        nCalculatedChecksum += (nCalculatedChecksum>>16);
        nCalculatedChecksum &= 0xFFFF;
    }

    // take the one's complement
    nCalculatedChecksum ^= 0xFFFF;
    // mask out the MSBs
    nCalculatedChecksum &= 0xFFFF;

    // debug print
    if(nHeaderChecksum != nCalculatedChecksum)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "vlDsgIsValidUdpChecksum: nHeaderChecksum = 0x%04X, nCalculatedChecksum = 0x%04X\n", nHeaderChecksum, nCalculatedChecksum);
    }

    // return result
    return (nHeaderChecksum == nCalculatedChecksum);
}

int vlIsIpPacketFragmented(VL_DSG_IP_HEADER * pIpPacket)
{
    return ((pIpPacket->bMoreFragments) || (pIpPacket->fragOffset > 0));
}

int vlDsgIsValidIpHeaderChecksum(VL_DSG_IP_HEADER * pIpHeader, unsigned char * pData, unsigned long nLength)
{
    int i = 0;
    unsigned long nHeaderChecksum = 0;
    unsigned long nCalculatedChecksum = 0;
    int nHeaderLength = (pIpHeader->headerLength)*4;
    nHeaderChecksum = pIpHeader->headerCRC;
    if(0xFFFF == pIpHeader->headerCRC) nHeaderChecksum = 0;

    if(nHeaderLength > 32) return 0; // fail the checksum if the header is too long

    // calculate checksum for header
    for(i = 0; i < (((nHeaderLength)/2)*2); i+=2)
    {
        if(i != 10) // skip the current checksum
        {
            nCalculatedChecksum += (VL_VALUE_2_FROM_ARRAY(pData));
            nCalculatedChecksum += (nCalculatedChecksum>>16);
            nCalculatedChecksum &= 0xFFFF;
        }
        pData += 2;
    }

    // take the one's complement
    nCalculatedChecksum ^= 0xFFFF;
    // mask out the MSBs
    nCalculatedChecksum &= 0xFFFF;

    // debug print
    if(nHeaderChecksum != nCalculatedChecksum)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "vlDsgIsValidIpChecksum: nHeaderChecksum = 0x%04X, nCalculatedChecksum = 0x%04X\n", nHeaderChecksum, nCalculatedChecksum);
    }

    // return result
    return (nHeaderChecksum == nCalculatedChecksum);
}

int vlDsgIsValidTcpChecksum(VL_DSG_IP_HEADER * pIpHeader, VL_DSG_TCP_HEADER * pTcpHeader,
                                unsigned char * pPayload, unsigned long nPayLoadLength)
{
    static unsigned char tcpChecksumHeader[32];
    static unsigned char padding[2];
    unsigned char *  pData      = tcpChecksumHeader;
    VL_BYTE_STREAM_ARRAY_INST(tcpChecksumHeader, WriteBuf);
    int nFinalLength = 0, i = 0;
    unsigned long nHeaderChecksum = 0;
    unsigned long nCalculatedChecksum = 0;
    if(0 == pTcpHeader->nChecksum) return 1;
    if(0xFFFF == pTcpHeader->nChecksum) nHeaderChecksum = 0;
    if(0xFFFF != pTcpHeader->nChecksum) nHeaderChecksum = pTcpHeader->nChecksum;

    nFinalLength += vlWriteLong (pWriteBuf, pIpHeader->srcIpAddress);
    nFinalLength += vlWriteLong (pWriteBuf, pIpHeader->dstIpAddress);
    nFinalLength += vlWriteByte (pWriteBuf, 0                      );
    nFinalLength += vlWriteByte (pWriteBuf, pIpHeader->eProtocol   );
    nFinalLength += vlWriteShort(pWriteBuf, nPayLoadLength         );

    // calculate checksum for pseudo-header
    pData = tcpChecksumHeader;
    for(i = 0; i < nFinalLength; i+=2)
    {
        nCalculatedChecksum += (VL_VALUE_2_FROM_ARRAY(pData));
        nCalculatedChecksum += (nCalculatedChecksum>>16);
        nCalculatedChecksum &= 0xFFFF;
        pData += 2;
    }

    // calculate checksum for payload
    pData = pPayload;
    for(i = 0; i < ((nPayLoadLength/2)*2); i+=2)
    {
        if(i != 16) // skip the current checksum
        {
            nCalculatedChecksum += (VL_VALUE_2_FROM_ARRAY(pData));
            nCalculatedChecksum += (nCalculatedChecksum>>16);
            nCalculatedChecksum &= 0xFFFF;
        }
        pData += 2;
    }

    // calculate checksum for trailer
    pData = padding;
    if(1 == nPayLoadLength%2)
    {
        padding[0] = pPayload[nPayLoadLength-1];
        padding[1] = 0;
        nCalculatedChecksum += (VL_VALUE_2_FROM_ARRAY(pData));
        nCalculatedChecksum += (nCalculatedChecksum>>16);
        nCalculatedChecksum &= 0xFFFF;
    }

    // take the one's complement
    nCalculatedChecksum ^= 0xFFFF;
    // mask out the MSBs
    nCalculatedChecksum &= 0xFFFF;

    // debug print
    if(nHeaderChecksum != nCalculatedChecksum)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "vlDsgIsValidTcpChecksum: nHeaderChecksum = 0x%04X, nCalculatedChecksum = 0x%04X\n", nHeaderChecksum, nCalculatedChecksum);
    }

    // return result
    return (nHeaderChecksum == nCalculatedChecksum);
}

void vlDsgDispatchBrokeredDataToHost(   unsigned char     * pData,
                                        unsigned long       nLength,
                                        unsigned char     * pPayload,
                                        unsigned long       nPayloadLength,
                                        VL_DSG_ETH_HEADER * pEthHeader,
                                        VL_DSG_IP_HEADER  * pIpHeader,
                                        VL_DSG_TCP_HEADER * pTcpHeader,
                                        VL_DSG_UDP_HEADER * pUdpHeader)
{
    if(vlg_bEnableDsgBroker)
    {
        if((0 == vlg_bDsgDirectoryFilterValid) && (0 == vlg_bDsgBrokerDeliverOnlyBcast))
        {
            unsigned short nDstPort = ((NULL != pUdpHeader)?(pUdpHeader->nDstPort):(pTcpHeader->nDstPort));
            int bDispatch = 0;
            unsigned long long nClientId = 0;
            
            if(NULL != pPayload)
            {
                unsigned char nTableId = pPayload[0];
                VL_BYTE_STREAM_INST(pPayload, ParseBuf, pPayload, nPayloadLength);
                VL_DSG_BT_HEADER   btHeader;
                vlParseDsgBtHeader(pParseBuf, &btHeader);
                int bIsBcast = 0;

                if((0xFF == btHeader.nHeaderStart) && (0x1 == btHeader.nVersion))
                {
                    nTableId = pPayload[4];
                    bIsBcast = 1;
                }
                
                switch(nTableId)
                {
                    case VL_DSG_BCAST_TABLE_ID_EAS:
                    {
                        if((!bIsBcast && vlg_bDsgBrokerDeliverCaEAS) || vlg_bDsgBrokerDeliverAnyEAS || (bIsBcast && vlg_bDsgBrokerDeliverBcastEAS))
                        {
                            bDispatch = 1;
                            nClientId = VL_DSG_BCAST_CLIENT_ID_SCTE_18;
                        }
                    }
                    break;
                    
                    case VL_DSG_BCAST_TABLE_ID_XAIT:
                    {
                        if((!bIsBcast && vlg_bDsgBrokerDeliverCaXAIT) || vlg_bDsgBrokerDeliverAnyXAIT || (bIsBcast && vlg_bDsgBrokerDeliverBcastXAIT))
                        {
                            bDispatch = 1;
                            nClientId = VL_DSG_BCAST_CLIENT_ID_XAIT;
                        }
                    }
                    break;
                    
                    case VL_DSG_BCAST_TABLE_ID_CVT:
                    {
                        if((!bIsBcast && vlg_bDsgBrokerDeliverCaCVT) || vlg_bDsgBrokerDeliverAnyCVT || (bIsBcast && vlg_bDsgBrokerDeliverBcastCVT))
                        {
                            bDispatch = 1;
                            nClientId = VL_DSG_BCAST_CLIENT_ID_XAIT;
                        }
                    }
                    break;
                    
                    case VL_DSG_BCAST_TABLE_ID_STT:
                    {
                        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: BROKER: bIsBcast                      = '%s'.\n", __FUNCTION__, VL_TRUE_COND_STR(bIsBcast));
                        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: BROKER: vlg_bDsgBrokerDeliverCaSTT    = '%s'.\n", __FUNCTION__, VL_TRUE_COND_STR(vlg_bDsgBrokerDeliverCaSTT));
                        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: BROKER: vlg_bDsgBrokerDeliverBcastSTT = '%s'.\n", __FUNCTION__, VL_TRUE_COND_STR(vlg_bDsgBrokerDeliverBcastSTT));
                        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: BROKER: vlg_bDsgBrokerDeliverAnySTT   = '%s'.\n", __FUNCTION__, VL_TRUE_COND_STR(vlg_bDsgBrokerDeliverAnySTT));
                        if((!bIsBcast && vlg_bDsgBrokerDeliverCaSTT) || vlg_bDsgBrokerDeliverAnySTT || (bIsBcast && vlg_bDsgBrokerDeliverBcastSTT))
                        {
                            bDispatch = 1;
                            nClientId = VL_DSG_BCAST_CLIENT_ID_SCTE_65;
                            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: BROKER: Delivering STT to HOST.\n", __FUNCTION__);
                        }
                    }
                    break;

                    default:
                    {
                        if((!bIsBcast && vlg_bDsgBrokerDeliverCaSI) || vlg_bDsgBrokerDeliverAnySI || (bIsBcast && vlg_bDsgBrokerDeliverBcastSI))
                        {
                            bDispatch = 1;
                            nClientId = VL_DSG_BCAST_CLIENT_ID_SCTE_65;
                        }
                    }
                    break;
                }
            }
            
            if(bDispatch && nClientId)
            {
                vlDsgDispatchToHost(pIpHeader,
                                    pTcpHeader,
                                    pUdpHeader,
                                    nDstPort,
                                    VL_DSG_DIR_TYPE_DIRECT_TERM_DSG_FLOW,
                                    VL_DSG_CLIENT_ID_ENC_TYPE_BCAST,
                                    nClientId,
                                    pData, nLength,
                                    pPayload, nPayloadLength);
            }
        }
    }
}

void vlDsgDispatchIpPayload(VL_DSG_ETH_HEADER *    pEthHeader,
                            VL_DSG_IP_HEADER  *    pIpHeader,
                            unsigned char * pData   , unsigned long nLength,
                            unsigned char * pPayload, unsigned long nPayLoadLength)
{
    // recreate the parser
    VL_BYTE_STREAM_INST(IpPayload, ParseBuf, pPayload, nPayLoadLength);

    switch(pIpHeader->eProtocol)
    {
        case VL_DSG_IP_PROTOCOL_TCP:
        {
            VL_DSG_TCP_HEADER  tcpHeader;
            vlParseDsgTcpHeader(pParseBuf, &tcpHeader);
            // check if this packet was assembled
            if(NULL == pData)
            {   // this is an assembled packet, so check the checksum
                if(!vlDsgIsValidTcpChecksum(pIpHeader, &tcpHeader, pPayload, nPayLoadLength))
                {   // drop this packet
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "vlDsgDispatchIpPayload: TCP checksum failed dropping packet id = 0x%04X, size = %d\n", pIpHeader->nIdentification, nPayLoadLength);
                    break;
                }
            }

            vlDsgDispatchToClients(pData, nLength,
                                VL_BYTE_STREAM_GET_CURRENT_BUF(pParseBuf), VL_BYTE_STREAM_REMAINDER(pParseBuf),
                                pEthHeader, pIpHeader, &tcpHeader, NULL);
        }
        break;

        case VL_DSG_IP_PROTOCOL_UDP:
        {
            VL_DSG_UDP_HEADER  udpHeader;
            vlParseDsgUdpHeader(pParseBuf, &udpHeader);
            // check if this packet was assembled
            if(NULL == pData)
            {   // this is an assembled packet, so check the checksum
                if(!vlDsgIsValidUdpChecksum(pIpHeader, &udpHeader, pPayload, nPayLoadLength))
                {   // drop this packet
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "vlDsgDispatchIpPayload: UDP checksum failed dropping packet id = 0x%04X, size = %d\n", pIpHeader->nIdentification, nPayLoadLength);
                    break;
                }
            }

            vlDsgDispatchToClients(pData, nLength,
                                VL_BYTE_STREAM_GET_CURRENT_BUF(pParseBuf), VL_BYTE_STREAM_REMAINDER(pParseBuf),
                                pEthHeader, pIpHeader, NULL, &udpHeader);
            
            if(vlg_bEnableDsgBroker)
            {
                vlDsgDispatchBrokeredDataToHost(pData, nLength,
                                    VL_BYTE_STREAM_GET_CURRENT_BUF(pParseBuf), VL_BYTE_STREAM_REMAINDER(pParseBuf),
                                    pEthHeader, pIpHeader, NULL, &udpHeader);
            }
        }
        break;
    }
}

VL_DSG_ACC_RESULT vlDsgAddToIpAccumulator(  int iAccumulator,
                                            VL_DSG_IP_ACCUMULATOR* pAccumulator,
                                            VL_DSG_ETH_HEADER *    pEthHeader,
                                            VL_DSG_IP_HEADER  *    pIpHeader,
                                            unsigned char     *    pData,
                                            unsigned long          nLength)
{
    unsigned long nElapsedTime = (time(NULL)-pAccumulator->startTime);

    // check for reuse of accumulator
    if((!pAccumulator->bStartedAccumulation) || ( nElapsedTime > VL_DSG_IP_REASSEMBLY_TIMEOUT))
    {
        if((pAccumulator->bStartedAccumulation) && ( nElapsedTime > VL_DSG_IP_REASSEMBLY_TIMEOUT))
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "vlDsgAddToIpAccumulator: Dropping IP packet ID: 0x%04X as reassembly exceeded %d seconds\n"   , pAccumulator->nIdentification, nElapsedTime);
        }

        pAccumulator->dstMacAddress  =  pEthHeader->dstMacAddress;
        pAccumulator->eTypeCode      =  pEthHeader->eTypeCode;
        pAccumulator->version        =  pIpHeader->version;
        pAccumulator->nIdentification=  pIpHeader->nIdentification;
        pAccumulator->eProtocol      =  pIpHeader->eProtocol;
        pAccumulator->srcIpAddress   =  pIpHeader->srcIpAddress;
        pAccumulator->dstIpAddress   =  pIpHeader->dstIpAddress;

        pAccumulator->bStartedAccumulation  = 1;
        pAccumulator->iOffsetBegin          = 65536;
        pAccumulator->iOffsetEnd            = 0;
        pAccumulator->nPayloadLength        = 0;
        pAccumulator->nBytesCaptured        = 0;
        pAccumulator->startTime             = time(NULL);
    }
    else
    {
        // else check for match
        if(pEthHeader->dstMacAddress != pAccumulator->dstMacAddress)
        {
            /*RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgAddToIpAccumulator: Acc %d, Mismatch MAC Address: \n", iAccumulator);
            RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgAddToIpAccumulator: Src Ip Address: %d.%d.%d.%d\n", ((pIpHeader->srcIpAddress>>24)&0xFF),
                   ((pIpHeader->srcIpAddress>>16)&0xFF),
                     ((pIpHeader->srcIpAddress>>8 )&0xFF),
                       ((pIpHeader->srcIpAddress>>0 )&0xFF));*/

            return VL_DSG_ACC_RESULT_NO_MATCH;
        }

        if(pEthHeader->eTypeCode != pAccumulator->eTypeCode)
        {
            /*RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgAddToIpAccumulator: Acc %d, Mismatch Ethernet type: %d:%d \n", iAccumulator, pEthHeader->eTypeCode, pAccumulator->eTypeCode);
            RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgAddToIpAccumulator: Src Ip Address: %d.%d.%d.%d\n", ((pIpHeader->srcIpAddress>>24)&0xFF),
                   ((pIpHeader->srcIpAddress>>16)&0xFF),
                     ((pIpHeader->srcIpAddress>>8 )&0xFF),
                       ((pIpHeader->srcIpAddress>>0 )&0xFF));*/

            return VL_DSG_ACC_RESULT_NO_MATCH;
        }

        if(pIpHeader->version != pAccumulator->version)
        {
            /*RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgAddToIpAccumulator: Acc %d, Mismatch Version: %d:%d \n", iAccumulator, pIpHeader->version, pAccumulator->version);
            RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgAddToIpAccumulator: Src Ip Address: %d.%d.%d.%d\n", ((pIpHeader->srcIpAddress>>24)&0xFF),
                   ((pIpHeader->srcIpAddress>>16)&0xFF),
                     ((pIpHeader->srcIpAddress>>8 )&0xFF),
                       ((pIpHeader->srcIpAddress>>0 )&0xFF));*/

            return VL_DSG_ACC_RESULT_NO_MATCH;
        }

        if(pIpHeader->nIdentification != pAccumulator->nIdentification)
        {
            /*RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgAddToIpAccumulator: Acc %d, Mismatch nIdentification: 0x%04X:0x%04X \n", iAccumulator, pIpHeader->nIdentification, pAccumulator->nIdentification);
            RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgAddToIpAccumulator: Src Ip Address: %d.%d.%d.%d\n", ((pIpHeader->srcIpAddress>>24)&0xFF),
                   ((pIpHeader->srcIpAddress>>16)&0xFF),
                     ((pIpHeader->srcIpAddress>>8 )&0xFF),
                       ((pIpHeader->srcIpAddress>>0 )&0xFF));*/

            return VL_DSG_ACC_RESULT_NO_MATCH;
        }

        if(pIpHeader->eProtocol != pAccumulator->eProtocol)
        {
            /*RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgAddToIpAccumulator: Acc %d, Mismatch Protocol: %d:%d \n", iAccumulator, pIpHeader->eProtocol, pAccumulator->eProtocol);
            RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgAddToIpAccumulator: Src Ip Address: %d.%d.%d.%d\n", ((pIpHeader->srcIpAddress>>24)&0xFF),
                   ((pIpHeader->srcIpAddress>>16)&0xFF),
                     ((pIpHeader->srcIpAddress>>8 )&0xFF),
                       ((pIpHeader->srcIpAddress>>0 )&0xFF));*/

            return VL_DSG_ACC_RESULT_NO_MATCH;
        }

        if( pIpHeader->srcIpAddress          != pAccumulator->srcIpAddress)
        {
            /*RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgAddToIpAccumulator: Acc %d, Mismatch SrcIpAddress\n", iAccumulator);
            RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgAddToIpAccumulator: Src Ip Address: %d.%d.%d.%d\n", ((pIpHeader->srcIpAddress>>24)&0xFF),
                   ((pIpHeader->srcIpAddress>>16)&0xFF),
                     ((pIpHeader->srcIpAddress>>8 )&0xFF),
                       ((pIpHeader->srcIpAddress>>0 )&0xFF));
            RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgAddToIpAccumulator: Src Ip Address: %d.%d.%d.%d\n", ((pAccumulator->srcIpAddress>>24)&0xFF),
                   ((pAccumulator->srcIpAddress>>16)&0xFF),
                     ((pAccumulator->srcIpAddress>>8 )&0xFF),
                       ((pAccumulator->srcIpAddress>>0 )&0xFF));*/

            return VL_DSG_ACC_RESULT_NO_MATCH;
        }

        if( pIpHeader->dstIpAddress          != pAccumulator->dstIpAddress)
        {
            /*RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgAddToIpAccumulator: Acc %d, Mismatch DstIpAddress\n", iAccumulator);
            RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgAddToIpAccumulator: Dst Ip Address: %d.%d.%d.%d\n", ((pIpHeader->dstIpAddress>>24)&0xFF),
                   ((pIpHeader->dstIpAddress>>16)&0xFF),
                     ((pIpHeader->dstIpAddress>>8 )&0xFF),
                       ((pIpHeader->dstIpAddress>>0 )&0xFF));
            RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgAddToIpAccumulator: Dst Ip Address: %d.%d.%d.%d\n", ((pAccumulator->dstIpAddress>>24)&0xFF),
                   ((pAccumulator->dstIpAddress>>16)&0xFF),
                     ((pAccumulator->dstIpAddress>>8 )&0xFF),
                       ((pAccumulator->dstIpAddress>>0 )&0xFF));*/

            return VL_DSG_ACC_RESULT_NO_MATCH;
        }
    }

    // check for oversize accumulation
    if((pIpHeader->fragOffset + nLength) > 65536)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "vlDsgAddToIpAccumulator: Dropping oversized payload iOffsetEnd = %d\n", (pIpHeader->fragOffset + nLength));
        pAccumulator->iOffsetEnd = 0;
        return VL_DSG_ACC_RESULT_ERROR;
    }

    // check for capacity
    while((pIpHeader->fragOffset + nLength) > pAccumulator->nBytesCapacity)
    {
	unsigned char * pNewPayload = NULL;
		rmf_osal_memAllocP(RMF_OSAL_MEM_POD, pAccumulator->nBytesCapacity+VL_DSG_IP_PAYLOAD_GROWTH_SIZE,(void **)&pNewPayload);

        // check for non-initial re-allocation
        if(0 != pAccumulator->iOffsetEnd)
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "vlDsgAddToIpAccumulator: Acc %d, Reallocating buffer for IP ID: 0x%04X \n", iAccumulator, pIpHeader->nIdentification);
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "vlDsgAddToIpAccumulator: iOffsetEnd = %d, new length = %d\n", pAccumulator->iOffsetEnd, (pIpHeader->fragOffset + nLength));
        }

        // null check
        if(NULL != pNewPayload)
        {
            // grow
            if(NULL != pAccumulator->pPayload)
            {
                rmf_osal_memcpy(pNewPayload, pAccumulator->pPayload,
                        pAccumulator->iOffsetEnd, pAccumulator->nBytesCapacity+VL_DSG_IP_PAYLOAD_GROWTH_SIZE,
                        pAccumulator->iOffsetEnd);

				rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pAccumulator->pPayload);
            }

            pAccumulator->pPayload = pNewPayload;
            pAccumulator->nBytesCapacity += VL_DSG_IP_PAYLOAD_GROWTH_SIZE;
        }
        else
        {
            return VL_DSG_ACC_RESULT_ERROR;
        }
    }

    /*RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgAddToIpAccumulator: Src Ip Address: %d.%d.%d.%d\n", ((pIpHeader->srcIpAddress>>24)&0xFF),
           ((pIpHeader->srcIpAddress>>16)&0xFF),
             ((pIpHeader->srcIpAddress>>8 )&0xFF),
               ((pIpHeader->srcIpAddress>>0 )&0xFF));
    RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgAddToIpAccumulator: Dest Ip Address: %d.%d.%d.%d\n", ((pIpHeader->dstIpAddress>>24)&0xFF),
           ((pIpHeader->dstIpAddress>>16)&0xFF),
             ((pIpHeader->dstIpAddress>>8 )&0xFF),
               ((pIpHeader->dstIpAddress>>0 )&0xFF));*/

    // accumulate
    rmf_osal_memcpy(&(pAccumulator->pPayload[pIpHeader->fragOffset]),
             pData, nLength,
             pAccumulator->nBytesCapacity-pIpHeader->fragOffset, 
             nLength
             );

    // register the number of bytes captured
    pAccumulator->nBytesCaptured += nLength;

    // register the starting offset
    if(pIpHeader->fragOffset < pAccumulator->iOffsetBegin)
    {
        pAccumulator->iOffsetBegin = pIpHeader->fragOffset;
    }

    // register the ending offset
    if((pIpHeader->fragOffset + nLength) > pAccumulator->iOffsetEnd)
    {
        pAccumulator->iOffsetEnd = (pIpHeader->fragOffset + nLength);
    }

    // register the payload length
    if((0 == pIpHeader->bMoreFragments) && (pIpHeader->fragOffset > 0))
    {
        pAccumulator->nPayloadLength = (pIpHeader->fragOffset + nLength);
    }

    // check for overflow
    if(pAccumulator->iOffsetEnd > pAccumulator->nBytesCapacity)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "vlDsgAddToIpAccumulator: Buffer Overflow for IP ID: 0x%04X\n"   , pIpHeader->nIdentification);
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "vlDsgAddToIpAccumulator: iOffsetEnd = %d, nBytesCapacity = %d\n", pAccumulator->iOffsetEnd, pAccumulator->nBytesCapacity);
        return VL_DSG_ACC_RESULT_ERROR;
    }

    /*RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgAddToIpAccumulator: Acc %d, ID: 0x%04X, nPayloadLength = %d, iOffsetBegin = %d, iOffsetEnd = %d, nBytesCaptured = %d\n",
            iAccumulator, pIpHeader->nIdentification, pAccumulator->nPayloadLength, pAccumulator->iOffsetBegin, pAccumulator->iOffsetEnd, pAccumulator->nBytesCaptured);*/

    // check for dispatch
    if((0                               != pAccumulator->nPayloadLength ) &&
       (0                               == pAccumulator->iOffsetBegin   ) &&
       (pAccumulator->nPayloadLength    == pAccumulator->iOffsetEnd     ) &&
       (pAccumulator->nBytesCaptured    >= pAccumulator->nPayloadLength )
      )
    {
        // make a copy of the IP header
        VL_DSG_IP_HEADER ipHeader = *pIpHeader;
        // clear the fragmentation attributes
        ipHeader.bMoreFragments = 0;
        ipHeader.fragOffset     = 0;

        // dispatch the defragmented payload
        vlDsgDispatchIpPayload(
                            pEthHeader,
                            &ipHeader,
                            NULL, 0,
                            pAccumulator->pPayload, pAccumulator->nPayloadLength);

        //RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "vlDsgAddToIpAccumulator: Dispatched Acc %d size %d, IP ID: 0x%04X\n", iAccumulator, pAccumulator->iOffsetEnd, pIpHeader->nIdentification);
        return VL_DSG_ACC_RESULT_DISPATCHED;
    }

    return VL_DSG_ACC_RESULT_ADDED;
}

void vlDsgDispatchFragmentedIpToHost(
                                    VL_DSG_ETH_HEADER *    pEthHeader,
                                    VL_DSG_IP_HEADER  *    pIpHeader,
                                    unsigned char     *    pData,
                                    unsigned long          nLength)
{
    static cVector * pIpDataVector = cVectorCreate(10, 10);

    if(NULL != pIpDataVector)
    {
        int i = 0;
        for(i = 0; i < cVectorSize(pIpDataVector); i++)
        {
            // try adding to any available accumulator
            VL_DSG_IP_ACCUMULATOR* pAccumulator =
                        (VL_DSG_IP_ACCUMULATOR*)cVectorGetElementAt(pIpDataVector, i);
            VL_DSG_ACC_RESULT result =
                        vlDsgAddToIpAccumulator( i, pAccumulator,
                                                pEthHeader, pIpHeader,
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
                    pAccumulator->iOffsetBegin          = 65536;
                    pAccumulator->iOffsetEnd            = 0;
                    pAccumulator->nPayloadLength        = 0;
                    pAccumulator->nBytesCaptured        = 0;
                    pAccumulator->startTime             = time(NULL);
                    return;
                }
                break;
            }
        }

        // no usable accumulators found so far, so create and register new accumulator
        VL_DSG_IP_ACCUMULATOR* pAccumulator = NULL;
		rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(VL_DSG_IP_ACCUMULATOR),(void **)&pAccumulator);
	memset(pAccumulator,0,sizeof(*pAccumulator));
        pAccumulator->bStartedAccumulation  = 0;
        pAccumulator->iOffsetBegin          = 65536;
        pAccumulator->iOffsetEnd            = 0;
        pAccumulator->nPayloadLength        = 0;
        pAccumulator->nBytesCaptured        = 0;
        pAccumulator->startTime             = time(NULL);
        cVectorAdd(pIpDataVector, (cVectorElement)pAccumulator);
        vlDsgAddToIpAccumulator( i, pAccumulator,
                                 pEthHeader, pIpHeader,
                                 pData, nLength);
    }
}

static int vl_dsg_unprotected_tunnel_callback(VL_DSG_DEVICE_HANDLE_t hDevice,
                                  unsigned char * pData, unsigned long nLength)
{
    //RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "received vl_dsg_tunnel_callback length = %d\n", nLength);
    VL_BYTE_STREAM_INST(EcmTunnelData, ParseBuf, pData, nLength);
    VL_DSG_ETH_HEADER ethHeader;
    vlParseDsgEthHeader(pParseBuf, &ethHeader);

    if(VL_DSG_ETH_PROTOCOL_IP == ethHeader.eTypeCode)
    {
        VL_DSG_IP_HEADER  ipHeader;
        unsigned char * pIpData = VL_BYTE_STREAM_GET_CURRENT_BUF(pParseBuf);
        vlParseDsgIpHeader(pParseBuf , &ipHeader);
        if(4 != ipHeader.version) return 1; // if version is not IPv4 drop the packet

        // if IP header checksum fails drop the packet
        if(!vlDsgIsValidIpHeaderChecksum(&ipHeader, pIpData, nLength)) return 1;

        // check for fragmentation
        if(vlIsIpPacketFragmented(&ipHeader))
        {
           static rmf_osal_Mutex lock;
  	    static int i_lock =0;
  	    if(i_lock==0)
  	    {		
  	    	rmf_osal_mutexNew(&lock);
  	       i_lock =1;		
  	    }

            rmf_osal_mutexAcquire(lock);
            {
                vlDsgDispatchFragmentedIpToHost(
                                                &ethHeader,
                                                &ipHeader,
                                                VL_BYTE_STREAM_GET_CURRENT_BUF(pParseBuf),
                                                VL_BYTE_STREAM_REMAINDER(pParseBuf));
            }
            rmf_osal_mutexRelease(lock);
        }

        vlDsgDispatchIpPayload(
                            &ethHeader,
                            &ipHeader,
                            pData, nLength,
                            VL_BYTE_STREAM_GET_CURRENT_BUF(pParseBuf),
                            VL_BYTE_STREAM_REMAINDER(pParseBuf));
    }
    return 1;
}

#define DHCP_FAILURE_TIMEOUT    180     // 3 minutes
rmf_osal_ThreadId g_prePollThreadId = 0;

void  vl_dsg_stb_dhcp_preliminary_polling_thread(void * arg)
{
    int nPollCountdown = DHCP_FAILURE_TIMEOUT; // poll for 3 minutes ( typical max DHCP timeout )
    rmf_osal_ThreadId currentThreadId = 0;
    rmf_osal_threadGetCurrent(&currentThreadId);
    static VL_HOST_IP_INFO hostIpInfo;
    vlDsgProxyServer_updateConf(DSGPROXY_SERVER_DOCSIS_STATUS_FILE, DSGPROXY_SERVER_DOCSIS_READY_STATUS);
    vlDsgSendBootupEventToPFC(RMF_BOOTMSG_DIAG_EVENT_TYPE_ESTB_IP, RMF_BOOTMSG_DIAG_MESG_CODE_START, 0);
    // poll for the specified times
    while(1)
    {
        unsigned long ipAddress = 0;
        if(vlg_bEnableDsgBroker)
        {
            vlXchanGetHostIpTypeInfo(VL_HOST_IP_IF_TYPE_DSG, 0, &hostIpInfo);
        }
        else
        {
            vlXchanGetDsgIpInfo(&hostIpInfo);
        }
        ipAddress = VL_VALUE_4_FROM_ARRAY(hostIpInfo.aBytIpAddress);
        // check if ip address is valid
        if(( (VL_DSG_CM_IP_ADDRESS   != (ipAddress & 0xFFFFFF00)) &&
             (VL_DSG_NULL_IP_ADDRESS != (ipAddress & 0xFFFFFFFF))    ) ||
           (0 != strlen(hostIpInfo.szIpV6GlobalAddress)              )    )
        {
            rmf_osal_threadSleep(5000, 0);   // 5 seconds
            vlg_bHostAcquiredIPAddress = 1;
            vlDsgTouchFile(VL_DSG_IP_ACQUIRED_TOUCH_FILE);
#ifdef USE_IPDIRECT_UNICAST
            vlIpDirectUnicastNotifyHostIpAcquired();
#endif // USE_IPDIRECT_UNICAST
            vlSendDsgEventToPFC(CardMgr_DSG_IP_ACQUIRED,
                                &(hostIpInfo), sizeof(hostIpInfo));
            /*  notfiy through POD */		
            if(VL_XCHAN_IP_ADDRESS_TYPE_IPV4 == hostIpInfo.ipAddressType)
            {
                postDSGIPEvent((uint8_t *)&hostIpInfo.szIpAddress[0]);
            }
            if(VL_XCHAN_IP_ADDRESS_TYPE_IPV6 == hostIpInfo.ipAddressType)
            {
                postDSGIPEvent((uint8_t *)&hostIpInfo.szIpV6GlobalAddress[0]);
            }
		  
            vlDsgSendBootupEventToPFC(RMF_BOOTMSG_DIAG_EVENT_TYPE_ESTB_IP, RMF_BOOTMSG_DIAG_MESG_CODE_COMPLETE, 0);
            break;
        }
        if(nPollCountdown <= 0)
        {
            nPollCountdown = DHCP_FAILURE_TIMEOUT; // reset countdown
            if(currentThreadId != g_prePollThreadId)
            {
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "vl_dsg_stb_dhcp_preliminary_polling_thread: Poll thread 0x%08X, exiting...\n", currentThreadId);
                return; // probably a stale thread so break out of loop to allow thread to exit.
            }
        }
        rmf_osal_threadSleep(1000, 0);   // sleep for one second
        nPollCountdown--;
    }
}

rmf_osal_ThreadId g_dhcpPollThreadId = 0;

void vl_dsg_stb_dhcp_polling_thread(void * arg)
{
#ifdef GLI
     IARM_Bus_SYSMgr_EventData_t eventData;
#endif
    int nPollCountdown = DHCP_FAILURE_TIMEOUT; // poll for 3 minutes ( typical max DHCP timeout )
    rmf_osal_ThreadId currentThreadId = 0;
    rmf_osal_threadGetCurrent(&currentThreadId);
    VL_DSG_ESAFE_DHCP_STATUS esafeStatus;
    memset(&esafeStatus,0,sizeof(esafeStatus));
    vlDsgProxyServer_updateConf(DSGPROXY_SERVER_DOCSIS_STATUS_FILE, DSGPROXY_SERVER_DOCSIS_READY_STATUS);
    
    rmf_osal_mutexAcquire(vlDsgTunnelGetThreadLock());
    {
        // Dec-08-2009 : Allow tunnel classification only if XChan resource version is 5 or higher
        if((vlXchanGetResourceId() >= VL_XCHAN_RESOURCE_ID_VER_5) && vlg_bDsgDirectoryFilterValid)
        {
            vlDsgReclassifyDirectoryForUcid();
        }
    }
    rmf_osal_mutexRelease(vlDsgTunnelGetThreadLock());

    esafeStatus.eProgress       = VL_DSG_ESAFE_PROGRESS_IN_PROGRESS;
    esafeStatus.eFailureFound   = VL_DSG_ESAFE_BOOL_FALSE;
    CHALDsg_Set_Config(VL_CM_DSG_DEVICE_INSTANCE, VL_DSG_PVT_TAG_ESAFE_DHCP_STATUS, &esafeStatus);
    // notify ucid to stack
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "vl_dsg_stb_dhcp_polling_thread: Notifying UCID %d to HOST\n", vlg_ucid);
    vlSendDsgEventToPFC(CardMgr_DSG_UCID,
                        &(vlg_ucid), sizeof(vlg_ucid));
    vlDsgSendBootupEventToPFC(RMF_BOOTMSG_DIAG_EVENT_TYPE_ESTB_IP, RMF_BOOTMSG_DIAG_MESG_CODE_START, 0);
#ifdef GLI
    eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_ESTB_IP;
    eventData.data.systemStates.state = 1;
    eventData.data.systemStates.error = 0;
    IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
#endif
    
    rmf_osal_mutexAcquire(vlDsgMainGetThreadLock());
    {
        // set flags
        //Jan-03-2012: This flag is reset by other mechanisms. So no need to reset it here. : vlg_bHostAcquiredIPAddress = 0;
    }
    rmf_osal_mutexRelease(vlDsgMainGetThreadLock());

    // poll for the specified times
    while(1)
    {
        unsigned long ipAddress = 0;

        if(!vlIsDsgOrIpduMode() && !vlg_bEnableDsgBroker) return; // exit thread if not in DSG mode

        rmf_osal_mutexAcquire(vlDsgMainGetThreadLock());
        {
            if(vlg_bEnableDsgBroker)
            {
                vlXchanGetHostIpTypeInfo(VL_HOST_IP_IF_TYPE_DSG, 0, &vlg_HostIpInfo);
            }
            else
            {
                vlXchanGetDsgIpInfo(&vlg_HostIpInfo);
            }
            ipAddress = VL_VALUE_4_FROM_ARRAY(vlg_HostIpInfo.aBytIpAddress);
        }
        rmf_osal_mutexRelease(vlDsgMainGetThreadLock());
        // check if ip address is valid
        if(( (VL_DSG_CM_IP_ADDRESS   != (ipAddress & 0xFFFFFF00)) &&
             (VL_DSG_NULL_IP_ADDRESS != (ipAddress & 0xFFFFFFFF))    ) ||
           (0 != strlen(vlg_HostIpInfo.szIpV6GlobalAddress)          )    )
        {
            esafeStatus.eProgress       = VL_DSG_ESAFE_PROGRESS_FINISHED;
            esafeStatus.eFailureFound   = VL_DSG_ESAFE_BOOL_FALSE;
            CHALDsg_Set_Config(VL_CM_DSG_DEVICE_INSTANCE, VL_DSG_PVT_TAG_ESAFE_DHCP_STATUS, &esafeStatus);
            CHALDsg_Set_Config(VL_CM_DSG_DEVICE_INSTANCE, VL_DSG_PVT_TAG_ESAFE_IP_ADDRESS, &vlg_HostIpInfo);
            
            vlg_tcHostIpLastChange = vlXchanGetSnmpSysUpTime();
            switch(vlg_DsgMode.eOperationalMode)
            {
                case VL_DSG_OPERATIONAL_MODE_BASIC_TWO_WAY:
                case VL_DSG_OPERATIONAL_MODE_ADV_TWO_WAY:
                case VL_DSG_OPERATIONAL_MODE_IPDM:
                case VL_DSG_OPERATIONAL_MODE_IPDIRECT_UNICAST:
                {
                    vlDsgSendBootupEventToPFC(RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD_DSG_TWOWAY, RMF_BOOTMSG_DIAG_MESG_CODE_COMPLETE, 0);
#ifdef GLI
			IARM_Bus_SYSMgr_EventData_t eventData;					
			eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_DOCSIS;
			eventData.data.systemStates.state = 2;
			eventData.data.systemStates.error = 0;
			IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
#endif

                }
                break;

                default:
                {
                }
                break;
            }
            // sleep for some time before notifying to stack
            rmf_osal_threadSleep(3000, 0);   // 3 seconds
            rmf_osal_mutexAcquire(vlDsgMainGetThreadLock());
            {
		uint8_t *ipadr = NULL;
		#if !defined (INTEL_CANMORE) && !defined (USE_BRCM_SOC)

                vlNetDelNetworkRoute(0, 0, 0, "eth0");
                #endif
                vlg_bHostAcquiredIPAddress = 1;
                vlDsgTouchFile(VL_DSG_IP_ACQUIRED_TOUCH_FILE);
                // notify new IP address to stack
                vlSendDsgEventToPFC(CardMgr_DSG_IP_ACQUIRED,
                                    &(vlg_HostIpInfo), sizeof(vlg_HostIpInfo));
                /*  notfiy through POD */		
                if(VL_XCHAN_IP_ADDRESS_TYPE_IPV4 == vlg_HostIpInfo.ipAddressType)
                {
                    postDSGIPEvent((uint8_t *)&vlg_HostIpInfo.szIpAddress[0]);
                }
                if(VL_XCHAN_IP_ADDRESS_TYPE_IPV6 == vlg_HostIpInfo.ipAddressType)
                {
                    postDSGIPEvent((uint8_t *)&vlg_HostIpInfo.szIpV6GlobalAddress[0]);
                }
		  
                vlDsgSendBootupEventToPFC(RMF_BOOTMSG_DIAG_EVENT_TYPE_ESTB_IP, RMF_BOOTMSG_DIAG_MESG_CODE_COMPLETE, 0);

                int iRepeat = 3;
                while(iRepeat-- > 0)
                {                
	                if( (VL_DSG_CM_IP_ADDRESS   != (ipAddress & 0xFFFFFF00)) &&
	                    (VL_DSG_NULL_IP_ADDRESS != (ipAddress & 0xFFFFFFFF))    )
	                {
	                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "vl_dsg_stb_dhcp_polling_thread: Host Acquired DSG IPV4 Address: %d.%d.%d.%d\n",
	                        vlg_HostIpInfo.aBytIpAddress[0], vlg_HostIpInfo.aBytIpAddress[1],
	                        vlg_HostIpInfo.aBytIpAddress[2], vlg_HostIpInfo.aBytIpAddress[3]);
                        vlg_bAcquiredIpv4Address = 1;
                        vlg_dsgIpv4Address = ipAddress;
	                }
	                if(0 != strlen(vlg_HostIpInfo.szIpV6GlobalAddress))
	                {
	                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "vl_dsg_stb_dhcp_polling_thread: Host Acquired DSG IPV6 Address: '%s'\n",
                               vlg_HostIpInfo.szIpV6GlobalAddress);
                        vlg_bAcquiredIpv6Address = 1;
	                }
            	  }
            }
            rmf_osal_mutexRelease(vlDsgMainGetThreadLock());
            break;
        }
        if(!vlg_bHostAcquiredIPAddress && (nPollCountdown <= 0))
        {
            nPollCountdown = DHCP_FAILURE_TIMEOUT; // reset countdown
            vlDsgSendBootupEventToPFC(RMF_BOOTMSG_DIAG_EVENT_TYPE_ESTB_IP, RMF_BOOTMSG_DIAG_MESG_CODE_ERROR, 0);
#ifdef GLI
            eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_ESTB_IP;
            eventData.data.systemStates.state = 0;
            eventData.data.systemStates.error = 1;
            IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
#endif
            esafeStatus.eProgress       = VL_DSG_ESAFE_PROGRESS_IN_PROGRESS;
            esafeStatus.eFailureFound   = VL_DSG_ESAFE_BOOL_TRUE;
            CHALDsg_Set_Config(VL_CM_DSG_DEVICE_INSTANCE, VL_DSG_PVT_TAG_ESAFE_DHCP_STATUS, &esafeStatus);
            if(currentThreadId != g_dhcpPollThreadId)
            {
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "vl_dsg_stb_dhcp_polling_thread: Poll thread 0x%08X, exiting...\n", currentThreadId);
                return; // probably a stale thread so break out of loop to allow thread to exit.
            }
        }
        rmf_osal_threadSleep(1000, 0);   // sleep for one second
        nPollCountdown--;
    }
    // imp: continue further down
    
    // DSG BROKER: Delay 2-Way notification for a while.
    if((long)arg) rmf_osal_threadSleep((long)arg, 0);

    rmf_osal_mutexAcquire(vlDsgMainGetThreadLock());
    {
        // notify ucid to card ( irrespective of whether the host acquired ip address )
        vlg_bHostAcquiredIPAddress = 1;
        APDU_DSG2POD_DsgMessage(vlGetXChanDsgSessionNumber(),
                            vlMapToXChanTag(vlGetXChanDsgSessionNumber(), VL_HOST2POD_DSG_MESSAGE),
                                            VL_DSG_MESSAGE_TYPE_2_WAY_OK,
                                            vlg_ucid);
    }
    rmf_osal_mutexRelease(vlDsgMainGetThreadLock());

    // Apr-05-2009: Moved from HAL: Set eCM Route for SNMP access by diagnostics
    vlXchanSetEcmRoute();
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "vl_dsg_stb_dhcp_polling_thread: Notifying UCID %d to CARD\n", vlg_ucid);

    if((VL_DSG_ESAFE_PROGRESS_FINISHED  == esafeStatus.eProgress    ) &&
       (VL_DSG_ESAFE_BOOL_FALSE         == esafeStatus.eFailureFound))
    {
        rmf_osal_threadSleep(3000, 0);  
        vlDsgProxyServer_Register();
    }
    // default return
}

static int vl_dsg_unprotected_control_callback(VL_DSG_DEVICE_HANDLE_t hDevice,
                                   unsigned char * pData, unsigned long nLength)
{
    VL_BYTE_STREAM_INST(EcmControlMessage, ParseBuf, pData, nLength);
    ULONG ulTag = vlParse3ByteLong(pParseBuf);
    USHORT nDsgSession = vlGetXChanDsgSessionNumber();
    // recalculate length and pData
    nLength = vlParseCcApduLengthField(pParseBuf);
    pData   = VL_BYTE_STREAM_GET_CURRENT_BUF(pParseBuf);

    switch(ulTag)
    {
        case VL_HOST2POD_SEND_DCD_INFO:
        case VL_XCHAN_HOST2POD_SEND_DCD_INFO:
        {
            vlg_bPartialDsgDirectory = 0; // Send full directory next time.
            vlg_bDsgScanFailed = 0;
			// Nov-09-2007: finish our parsing first
            {
                if(VL_XCHAN_HOST2POD_SEND_DCD_INFO == ulTag)
                {
                    // discard 3 bytes of config change count, no. of frags, and frag seq num
                    ULONG discard = vlParse3ByteLong(pParseBuf);
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "vl_dsg_control_callback: Received DCD : Discarding 3 bytes [0x%06X]\n", discard);
                }
                vlDsgProxyServer_UpdateDcd(VL_BYTE_STREAM_REMAINDER(pParseBuf), VL_BYTE_STREAM_GET_CURRENT_BUF(pParseBuf));
                vlParseDsgDCD(VL_BYTE_STREAM_REMAINDER(pParseBuf), VL_BYTE_STREAM_GET_CURRENT_BUF(pParseBuf), pParseBuf, &vlg_DsgDcd);
                if(rdk_dbg_enabled("LOG.RDK.POD", RDK_LOG_DEBUG)) vlPrintDsgDCD(0, &vlg_DsgDcd);
                CHALDsg_Set_Config(VL_CM_DSG_DEVICE_INSTANCE, ulTag, &vlg_DsgDcd);
                vlg_tLastDcdNotification = rmf_osal_GetUptime();
                if(vlg_bEnableDsgBroker)
                {
                    if(nLength < sizeof(vlg_aBytDcdBuffer))
                    {
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: DSG BROKER: Caching DCD.\n", __FUNCTION__);
                        rmf_osal_memcpy(vlg_aBytDcdBuffer, pData, nLength,
							sizeof(vlg_aBytDcdBuffer), nLength );
                        vlg_nDcdBufferLength = nLength;
                        vlg_nDcdApduTag = vlMapToXChanTag(nDsgSession, ulTag);
                    }
                    else
                    {
                        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s: DSG BROKER: Not Caching DCD as DCD length = %d is larger than cache capacity = %d.\n", __FUNCTION__, nLength, sizeof(vlg_aBytDcdBuffer));
                    }
                }
            }
            // REQ1513: forward DCD only in advanced mode
            switch(vlg_DsgMode.eOperationalMode)
            {
                case VL_DSG_OPERATIONAL_MODE_SCTE55:
                case VL_DSG_OPERATIONAL_MODE_BASIC_TWO_WAY:
                case VL_DSG_OPERATIONAL_MODE_BASIC_ONE_WAY:
                case VL_DSG_OPERATIONAL_MODE_IPDM:
                case VL_DSG_OPERATIONAL_MODE_IPDIRECT_UNICAST:
                {
                    // do not forward DCD to card
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "vl_dsg_control_callback: Received DCD : Not sending to POD as DSG Mode = %d\n", vlg_DsgMode.eOperationalMode);
                }
                break;

                case VL_DSG_OPERATIONAL_MODE_ADV_TWO_WAY:
                case VL_DSG_OPERATIONAL_MODE_ADV_ONE_WAY:
                default:
                {
                    // Nov-09-2007: send to pod only after we are done with our parsing
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "vl_dsg_control_callback: Received DCD : Sending to POD as DSG Mode = %d\n", vlg_DsgMode.eOperationalMode);
                    APDU_DSG2POD_SendDcdInfo(nDsgSession, vlMapToXChanTag(nDsgSession, ulTag), pData, nLength);
                }
                break;
            }
            #ifdef GLI
            {
                IARM_Bus_SYSMgr_EventData_t eventData;
                eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_RF_CONNECTED;
                eventData.data.systemStates.state = 1;
                eventData.data.systemStates.error = 0;
                IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME,
                    (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE,
                    (void *)&eventData, sizeof(eventData));
            }
            #endif
        }
        break;

        case VL_HOST2POD_DSG_MESSAGE:
        case VL_XCHAN_HOST2POD_DSG_MESSAGE:
        {
            UCHAR data = 0;
            VL_DSG_MESSAGE_TYPE eMessageType = (VL_DSG_MESSAGE_TYPE)0;

            eMessageType = (VL_DSG_MESSAGE_TYPE)vlParseByte(pParseBuf);

            if((VL_DSG_MESSAGE_TYPE_2_WAY_OK                == eMessageType) ||
                (VL_DSG_MESSAGE_TYPE_DYNAMIC_CHANNEL_CHANGE == eMessageType) ||
                (VL_DSG_MESSAGE_TYPE_CANNOT_FORWARD_2_WAY   == eMessageType) )
            {
                data = vlParseByte(pParseBuf);
            }

            if(VL_DSG_MESSAGE_TYPE_ENTERING_ONE_WAY_MODE  == eMessageType)
            {
                vlg_tLastOneWayNotification = rmf_osal_GetUptime();
            }
            // Oct-23-2008: Do not send UCID event message to CARD yet.
            if(VL_DSG_MESSAGE_TYPE_2_WAY_OK == eMessageType)
            {
                vlg_ucid = data;
                vlg_dsgBrokerUcid = vlg_ucid;
                rmf_osal_threadCreate(vl_dsg_stb_dhcp_polling_thread, NULL, 0, 0, &g_dhcpPollThreadId, "vl_dsg_stb_dhcp_polling_thread");
                 
            }
            else // Oct-23-2008: Send other Message types.
            {
                APDU_DSG2POD_DsgMessage(nDsgSession, vlMapToXChanTag(nDsgSession, ulTag), eMessageType, data);
            }

            if((VL_DSG_MESSAGE_TYPE_ECM_RESET              == eMessageType) ||
               (VL_DSG_MESSAGE_TYPE_ENTERING_ONE_WAY_MODE  == eMessageType) ||
               (VL_DSG_MESSAGE_TYPE_DYNAMIC_CHANNEL_CHANGE == eMessageType))
            {
                vlg_bPartialDsgDirectory = 0; // Send full directory next time.
                vlg_dsgBrokerUcid = 0;
            }

            if((VL_DSG_MESSAGE_TYPE_ECM_RESET             == eMessageType) ||
               (VL_DSG_MESSAGE_TYPE_ENTERING_ONE_WAY_MODE == eMessageType))
            {
                if(vlIsDsgOrIpduMode() && (0 != vlg_IPU_svc_FlowId) && vlXchanIsDsgIpuFlow(vlg_IPU_svc_FlowId))
                {
                    // Feb-04-2009: Re-enabled the following statement to meet requirements of ATP-TC-165.1
                    switch(vlg_DsgMode.eOperationalMode)
                    {
                        case VL_DSG_OPERATIONAL_MODE_BASIC_ONE_WAY:
                        case VL_DSG_OPERATIONAL_MODE_ADV_ONE_WAY:
                        {
                            // do not notify lost flow as any change in operational mode
                            // would have already notified the loss of flow. Also
                            // entering one way mode notification is not expected.
                        }
                        break;
                    
                        default:
                        case VL_DSG_OPERATIONAL_MODE_BASIC_TWO_WAY:
                        case VL_DSG_OPERATIONAL_MODE_ADV_TWO_WAY:
                        case VL_DSG_OPERATIONAL_MODE_IPDM:
                        case VL_DSG_OPERATIONAL_MODE_IPDIRECT_UNICAST:
                        {
                            APDU_XCHAN2POD_lostFlowInd(vlGetXChanSession(), vlg_IPU_svc_FlowId, VL_XCHAN_LOST_FLOW_REASON_NET_BUSY_OR_DOWN);
                        }
                        break;
                    }
                }
                // Jul-15-2009: UCID does not change for these events.
                vlg_bHostAcquiredIPAddress = 0;
            }

            if(VL_DSG_MESSAGE_TYPE_CANNOT_FORWARD_2_WAY   == eMessageType)
            {
                if(vlIsDsgOrIpduMode() && (0 != vlg_IPU_svc_FlowId) && vlXchanIsDsgIpuFlow(vlg_IPU_svc_FlowId))
                {
                    // Feb-04-2009: To meet requirements of ATP-TC-165.1
                    APDU_XCHAN2POD_lostFlowInd(vlGetXChanSession(), vlg_IPU_svc_FlowId, VL_XCHAN_LOST_FLOW_REASON_NET_BUSY_OR_DOWN);
                }
                // Jul-15-2009: UCID does not change for these events.
                vlg_bHostAcquiredIPAddress = 0;
            }

            if(VL_DSG_MESSAGE_TYPE_ECM_RESET             == eMessageType)
            {
		memset(&vlg_Old_DsgMode,0,sizeof(vlg_Old_DsgMode));
                vlg_Old_DsgMode.eOperationalMode = VL_DSG_OPERATIONAL_MODE_INVALID;
            }
            
            if(VL_DSG_MESSAGE_TYPE_DOWNSTREAM_SCAN_COMPLETED == eMessageType)
            {
                vlg_dsgBrokerUcid = 0;
                vlg_bDsgScanFailed = 1;
                
                if(vlg_bInvalidDsgChannelAlreadyIndicated)
                {
                    vlg_bDownstreamScanCompleted = 1;
                }
                switch(vlg_DsgMode.eOperationalMode)
                {
                    case VL_DSG_OPERATIONAL_MODE_BASIC_TWO_WAY:
                    case VL_DSG_OPERATIONAL_MODE_BASIC_ONE_WAY:
                    case VL_DSG_OPERATIONAL_MODE_ADV_TWO_WAY:
                    case VL_DSG_OPERATIONAL_MODE_ADV_ONE_WAY:
                    case VL_DSG_OPERATIONAL_MODE_IPDM:
                    case VL_DSG_OPERATIONAL_MODE_IPDIRECT_UNICAST:
                    {
                        // Apr-24-2009: Removed call to re-scan as some platforms do this automatically (platform dependent).
                        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "vl_dsg_control_callback: Posting CardMgr_DSG_DownStream_Scan_Failed to PFC\n");
                        vlSendDsgEventToPFC(CardMgr_DSG_DownStream_Scan_Failed,
                                            NULL, 0);
                    }
                    break;
                }
                
                switch(vlg_DsgMode.eOperationalMode)
                {
                    case VL_DSG_OPERATIONAL_MODE_BASIC_TWO_WAY:
                    case VL_DSG_OPERATIONAL_MODE_ADV_TWO_WAY:
                    case VL_DSG_OPERATIONAL_MODE_IPDM:
                    case VL_DSG_OPERATIONAL_MODE_IPDIRECT_UNICAST:
                    {
                        vlDsgSendBootupEventToPFC(RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD_DSG_TWOWAY, RMF_BOOTMSG_DIAG_MESG_CODE_ERROR, 0);
#ifdef GLI
				IARM_Bus_SYSMgr_EventData_t eventData;					
				eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_DSG_CA_TUNNEL;
				eventData.data.systemStates.state = 0;
				eventData.data.systemStates.error = 1;
				IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
#endif
                    }
                    break;
                    
                    case VL_DSG_OPERATIONAL_MODE_BASIC_ONE_WAY:
                    case VL_DSG_OPERATIONAL_MODE_ADV_ONE_WAY:
                    {
                        vlDsgSendBootupEventToPFC(RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD_DSG_ONEWAY, RMF_BOOTMSG_DIAG_MESG_CODE_ERROR, 0);
#ifdef GLI
				IARM_Bus_SYSMgr_EventData_t eventData;					
				eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_DSG_CA_TUNNEL;
				eventData.data.systemStates.state = 0;
				eventData.data.systemStates.error = 1;
				IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
#endif
                    }
                    break;
                }
            }

            {
                char* message_names[] =
                {
                    "Reserved",
                    "2-Way OK",
                    "Entering One Way Mode",
                    "Downstream scan failed",
                    "Channel Change",
                    "ECM Reset",
                    "Too many MAC Addresses",
                    "Cannot Forward Two-Way",
                    "8",
                    "9",
                    "10",
                };

                {
                    static int ePreviousEvent = VL_DSG_MESSAGE_TYPE_INVALID;
                    if(eMessageType != ePreviousEvent)
                    {
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "vl_dsg_control_callback: Received callback Tag '%s'\n", message_names[VL_SAFE_ARRAY_INDEX(eMessageType, message_names)]);
                    }
                    else
                    {
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "vl_dsg_control_callback: Received duplicate callback with event '0x%02X'.\n", eMessageType);
                    }
                    ePreviousEvent = eMessageType;
                }
                
                switch(eMessageType)
                {
                    case VL_DSG_MESSAGE_TYPE_2_WAY_OK:
                    {
                        // ok for dsg proxy
                    }
                    break;
                    
                    default:
                    {
                        vlDsgProxyServer_Deregister(message_names[VL_SAFE_ARRAY_INDEX(eMessageType, message_names)]);
                    }
                    break;
                }
#ifdef USE_IPDIRECT_UNICAST
                vlIpDirectUnicastHandleEcmMessage(VL_DSG_MESSAGE_TYPE_2_WAY_OK);
#endif // USE_IPDIRECT_UNICAST
            }
        }
        break;
        
        // Apr-23-2009 : Moved to case VL_HOST2POD_DSG_MESSAGE above.
    }
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "received vl_dsg_control_callback tag = 0x%06X, length = %d\n", ulTag, nLength);
    return 1;
}

static int vl_dsg_unprotected_image_callback(VL_DSG_DEVICE_HANDLE_t hDevice,
                                 unsigned char * pData, unsigned long nLength, unsigned long bEndOfFile)
{
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "received vl_dsg_unprotected_image_callback length = %d %s\n", nLength, VL_COND_STR(bEndOfFile, EOF, MORE));
    return 1;
}

static int vl_dsg_unprotected_download_status_callback(VL_DSG_DEVICE_HANDLE_t hDevice,
                                           VL_DSG_DOWNLOAD_STATE eStatus, VL_DSG_DOWNLOAD_RESULT eResult)
{
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "received vl_dsg_unprotected_download_status_callback eStatus = %d, eResult = 0%d\n", eStatus, eResult);
    return 1;
}

static int vl_dsg_unprotected_ecm_status_callback(VL_DSG_DEVICE_HANDLE_t hDevice,
                                      VL_DSG_ECM_STATUS eStatus)
{
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "received vl_dsg_unprotected_ecm_status_callback eStatus = %d\n", eStatus);
    return 1;
}

static bool m_bIpModeDetermined = false;

static void SetIPv4Mode()
{
    const char * pszTouchFile = "/tmp/estb_ipv4";
    FILE * fp = fopen(pszTouchFile, "w");
    char data[]="ipv4";
	
    if(NULL != fp)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "IpModeControlvlaue: TLV-217: Touched '%s'.\n", pszTouchFile);
        fclose(fp);
    }	
    m_bIpModeDetermined = true;
#ifdef GLI	
		IARM_Bus_SYSMgr_EventData_t eventData;
		eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_IP_MODE;
		eventData.data.systemStates.state = 1;
		eventData.data.systemStates.error = 0;
		strncpy(eventData.data.systemStates.payload,data,strlen(data));
		eventData.data.systemStates.payload[strlen(data)]='\0';
		IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));	
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Broadcasted IARM_BUS_SYSMGR_SYSSTATE_IP_MODE with payload = %s\n",eventData.data.systemStates.payload);
#endif
}

static void SetIPv6Mode()
{
    const char * pszTouchFile = "/tmp/estb_ipv6";
    char data[]="ipv6";
    FILE * fp = fopen(pszTouchFile, "w");
    if(NULL != fp)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "IpModeControlvlaue: TLV-217: Touched '%s'.\n", pszTouchFile);
        fclose(fp);
    }
    m_bIpModeDetermined = true;
#ifdef GLI	
			IARM_Bus_SYSMgr_EventData_t eventData;
			eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_IP_MODE;
			eventData.data.systemStates.state = 1;
			eventData.data.systemStates.error = 0;
			strncpy(eventData.data.systemStates.payload,data,strlen(data));
			eventData.data.systemStates.payload[strlen(data)]='\0';
			IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));	
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Broadcasted IARM_BUS_SYSMGR_SYSSTATE_IP_MODE with payload = %s\n",eventData.data.systemStates.payload);
#endif	
}

static VL_TLV_217_IP_MODE checkIpModeOverride(VL_TLV_217_IP_MODE eIpModeCheck)
{
    VL_TLV_217_IP_MODE eIpMode = eIpModeCheck;

    unsigned long ret = (unsigned long)CHALDsg_Set_Config(VL_CM_DSG_DEVICE_INSTANCE, VL_DSG_PVT_TAG_SET_HOST_IP_MODE, (void*)&(eIpMode));
    
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "IpModeControlvlaue: TLV-217: IPC_CLIENT_CHALDsg_Set_Config returned 0x%08X.\n", ret);
    
    VL_TLV_217_IP_MODE eOverriddenValue = (VL_TLV_217_IP_MODE)(ret&0xF);
    
    if((0x80000000 == (ret&0xFFFFFFF0)) && (eOverriddenValue != eIpMode))
    {
        eIpMode = eOverriddenValue;
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "IpModeControlvlaue: TLV-217: IP Mode change due to Override DETECTED.\n");
    }
    else
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "IpModeControlvlaue: TLV-217: IP Mode change due to Override NOT detected.\n");
    }

    return eIpMode;
}

static int vl_dsg_configure_ip_mode(VL_TLV_217_IP_MODE eIpModeControl)
{
    static VL_TLV_217_IP_MODE ePreviousIpMode = VL_TLV_217_IP_MODE_INVALID;
    static unsigned long nLastTimeStamp = 0;
    unsigned long nCurrentTimeStamp = rmf_osal_GetUptime();
    bool bRebootOnIpModeChange = false;
    
    if(0 == access(REBOOT_ON_IP_MODE_CHANGE_FLAG, F_OK))
    {
        // a reboot is expected on subsequent IP mode change.
        bRebootOnIpModeChange = true;
    }
    
    if(VL_TLV_217_IP_MODE_INVALID != ePreviousIpMode) // Ip Mode has already been determined.
    {
        if(false == bRebootOnIpModeChange) // a reboot is NOT expected on subsequent IP mode change, so ignore updated IP mode for this boot-up.
        {
            // ignore updated IP mode for this boot-up.
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "IpModeControlvlaue:1: TLV-217: IP Mode already determined for this boot-up. Ignoring new IP mode.\n");
            return 1;
        }
    }
    
    switch(eIpModeControl)
    {
        case VL_TLV_217_IP_MODE_IPV4:
        case VL_TLV_217_IP_MODE_IPV6:
        case VL_TLV_217_IP_MODE_DUAL:
        {
            eIpModeControl = checkIpModeOverride(eIpModeControl);
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "IpModeControlvlaue: TLV-217: Requested IP Mode = %d.\n", eIpModeControl);
            m_bIpModeDetermined = true;
        }
        break;

        default:
        {
            // log tlv error event
        }
        break;
    }
    
    if(VL_TLV_217_IP_MODE_INVALID != ePreviousIpMode) // Ip Mode has already been determined.
    {
        if((ePreviousIpMode != eIpModeControl) && ((nCurrentTimeStamp-nLastTimeStamp) > 60)) // Previous mode is not the same as the new mode.
        {
            if(bRebootOnIpModeChange) // a reboot is expected on subsequent IP mode change.
            {
                // immediate : reboot on IP mode change.
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "IpModeControlvlaue: TLV-217: IP Mode already determined for this boot-up. '%s' flag is set. Rebooting...\n", REBOOT_ON_IP_MODE_CHANGE_FLAG);
                HAL_SYS_Reboot(__FUNCTION__, "'" REBOOT_ON_IP_MODE_CHANGE_FLAG "' is set");
                return 1;
            }
        }
        // ignore updated IP mode for this boot-up.
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "IpModeControlvlaue:2: TLV-217: IP Mode already determined for this boot-up. Ignoring new IP mode.\n");
        return 1;
    }

    // Ip Mode check
    switch(eIpModeControl)
    {
        case VL_TLV_217_IP_MODE_IPV4: // IPv4
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "IpModeControlvlaue: TLV-217: Requested IP Mode is IPv4.\n");
            SetIPv4Mode();
        }
        break;

        case VL_TLV_217_IP_MODE_IPV6: // IPv6
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "IpModeControlvlaue: TLV-217: Requested IP Mode is IPv6.\n");
            SetIPv6Mode();
        }
        break;

        case VL_TLV_217_IP_MODE_DUAL: // Dual IPv4 + IPv6
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "IpModeControlvlaue: TLV-217: Requested IP Mode is Dual Stack (IPv4 + IPv6).\n");
            SetIPv4Mode();
            SetIPv6Mode();
        }
        break;

        default:
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "IpModeControlvlaue: TLV-217: Requested IP Mode is default case. Defaulting to IPv4.\n");
            SetIPv4Mode();
        }
        break;
    }
    
    ePreviousIpMode = eIpModeControl;
    nLastTimeStamp = nCurrentTimeStamp;
    return 1;
}

static int vl_dsg_dump_tlv_217_field(VL_BYTE_STREAM * pParseBuf, VL_DSG_TLV_217_TYPE eTag, int nBytes)
{
    VL_DSG_TLV_217_FIELD field;
    memset(&field,0,sizeof(field));
    field.eTag = eTag;
    field.nBytes = nBytes;
    vlParseBuffer(pParseBuf, field.aBytes, field.nBytes, sizeof(field.aBytes));

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "-------------------------------------------------------------------------------\n");
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "vl_dsg_dump_tlv_217_field: received Tag = %d.%d.%d.%d, length = %d\n",
                                                ((eTag>>24)&0xFF), ((eTag>>16)&0xFF),
                                                ((eTag>>8 )&0xFF), ((eTag>>0 )&0xFF),
                                                nBytes);
    vlMpeosDumpBuffer(RDK_LOG_DEBUG, "LOG.RDK.POD", field.aBytes, field.nBytes);
	return 1;
}

static int vl_dsg_parse_tlv_217(VL_BYTE_STREAM * pParseBuf, ULONG nParentTag, int nParentLength)
{
    unsigned char * pParentBuf = VL_BYTE_STREAM_GET_CURRENT_BUF(pParseBuf);

    while(((VL_BYTE_STREAM_GET_CURRENT_BUF(pParseBuf))-pParentBuf) < nParentLength)
    {
        ULONG ulTag = ((nParentTag<<8) | vlParseByte(pParseBuf));
        ULONG nLength = vlParseByte(pParseBuf);

        if((0 == ((ulTag>>24)&0xFF)) &&
           (0 == ((ulTag>>16)&0xFF)) &&
           (0 == ((ulTag>> 8)&0xFF)))
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "###############################################################################\n");
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "vl_dsg_parse_tlv_217: BEGIN TLV = %d : Length %d Bytes ----->\n",
                                ((ulTag>>0 )&0xFF), nLength);
        }
        
        /*RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "vl_dsg_parse_tlv_217: BEGIN TLV = %d : Length %d Bytes ----->\n",
                        ((ulTag>>24)&0xFF), ((ulTag>>16)&0xFF),
                        ((ulTag>>8 )&0xFF), ((ulTag>>0 )&0xFF),
                        nLength);*/
        
        switch(ulTag)
        {
            case VL_DSG_TLV_217_TYPE_SNMPv3NotificationReceiver                             :
            case VL_DSG_TLV_217_TYPE_SNMPv1v2c                                              :
            case VL_DSG_TLV_217_TYPE_SNMPv1v2c_TransportAddressAccess                       :
            case VL_DSG_TLV_217_TYPE_SNMPv3_AccessViewConfig                                :
            case VL_DSG_TLV_217_TYPE_VendorSpecific                                         :
            {
                if((0 == ((ulTag>>24)&0xFF)) &&
                   (0 == ((ulTag>>16)&0xFF)) &&
                   (0 != ((ulTag>> 8)&0xFF)))
                {
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "===============================================================================\n");
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "vl_dsg_parse_tlv_217: BEGIN TLV = %d.%d : Length %d Bytes ----->\n",
                            ((ulTag>>8 )&0xFF), ((ulTag>>0 )&0xFF),
                            nLength);
                }
                // Process Composite
                vl_dsg_parse_tlv_217(pParseBuf, ulTag, nLength);
            }
            break;

            case VL_DSG_TLV_217_TYPE_IpModeControl                                           :
            {
                VL_BYTE_STREAM_INST(IpModeBuf, IpModeParseBuf, VL_BYTE_STREAM_GET_CURRENT_BUF(pParseBuf), nLength);
                VL_TLV_217_IP_MODE eIpModeControl = (VL_TLV_217_IP_MODE)(vlParseNByteLong(pIpModeParseBuf, nLength));
                vl_dsg_dump_tlv_217_field(pParseBuf, (VL_DSG_TLV_217_TYPE)ulTag, nLength);
                vl_dsg_configure_ip_mode(eIpModeControl);
            }
            break;
            
            case VL_DSG_TLV_217_TYPE_VendorId                                                :
            case VL_DSG_TLV_217_TYPE_SNMPmibObject                                           :
                
            case VL_DSG_TLV_217_TYPE_SNMPv3NotificationReceiver_IPv4Address                  :
            case VL_DSG_TLV_217_TYPE_SNMPv3NotificationReceiver_UDPPortNumber                :
            case VL_DSG_TLV_217_TYPE_SNMPv3NotificationReceiver_TrapType                     :
            case VL_DSG_TLV_217_TYPE_SNMPv3NotificationReceiver_Timeout                      :
            case VL_DSG_TLV_217_TYPE_SNMPv3NotificationReceiver_Retries                      :
            case VL_DSG_TLV_217_TYPE_SNMPv3NotificationReceiver_FilteringParameters          :
            case VL_DSG_TLV_217_TYPE_SNMPv3NotificationReceiver_SecurityName                 :
            case VL_DSG_TLV_217_TYPE_SNMPv3NotificationReceiver_IPv6Address                  :
                
            case VL_DSG_TLV_217_TYPE_SNMPv1v2c_CommunityName                                 :

            case VL_DSG_TLV_217_TYPE_SNMPv1v2c_TransportAddressAccess_TransportAddress       :
            case VL_DSG_TLV_217_TYPE_SNMPv1v2c_TransportAddressAccess_TransportAddressMask   :
            case VL_DSG_TLV_217_TYPE_SNMPv1v2c_AccessViewType                                :
            case VL_DSG_TLV_217_TYPE_SNMPv1v2c_AccessViewName                                :

            case VL_DSG_TLV_217_TYPE_SNMPv3_AccessViewConfig_AccessViewName                  :
            case VL_DSG_TLV_217_TYPE_SNMPv3_AccessViewConfig_AccessViewSubtree               :
            case VL_DSG_TLV_217_TYPE_SNMPv3_AccessViewConfig_AccessViewMask                  :
            case VL_DSG_TLV_217_TYPE_SNMPv3_AccessViewConfig_AccessViewType                  :
            {
                vl_dsg_dump_tlv_217_field(pParseBuf, (VL_DSG_TLV_217_TYPE)ulTag, nLength);
            }
            break;

            default:
            {
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "vl_dsg_parse_tlv_217: received Invalid Tag = %d.%d.%d.%d, of Length %d Bytes\n",
                                                            ((ulTag>>24)&0xFF), ((ulTag>>16)&0xFF),
                                                            ((ulTag>>8 )&0xFF), ((ulTag>>0 )&0xFF),
                                                            nLength);
                // do a nop parse of the unrecognized tag
                vl_dsg_dump_tlv_217_field(pParseBuf, (VL_DSG_TLV_217_TYPE)ulTag, nLength);
            }
            break;
        }
    }
 	return 1;
}

static int vl_dsg_unprotected_async_data_callback(VL_DSG_DEVICE_HANDLE_t hDevice,
                                      unsigned long ulTag,
                                      unsigned long ulQualifier,
                                      void *        pStruct)
{
    switch(ulTag)
    {
        case VL_DSG_PVT_TAG_ECM_TIMEOUT:
        {
            char* timout_names[] =
            {
                "Invalid",
                "Initialization",
                "Operational",
                "Two Way Retry",
                "One Way Retry",
                "Invalid",
                "Invalid",
            };
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "received vl_dsg_async_data_callback VL_DSG_PVT_TAG_ECM_TIMEOUT: '%s' Timeout\n", timout_names[SAFE_ARRAY_INDEX_UN(ulQualifier, timout_names)]);
            return 0;
        }
        break;

        case VL_XCHAN_POD2HOST_NEW_FLOW_CNF:
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "received vl_dsg_async_data_callback VL_XCHAN_POD2HOST_NEW_FLOW_CNF, ulQualifier = %d\n", ulQualifier);
            return vlXchanHandleDsgFlowCnf(ulTag, ulQualifier, (VL_DSG_IP_UNICAST_FLOW_PARAMS*)pStruct);
        }
        break;

        case VL_DSG_PVT_TAG_IPU_PKT_ECM_TO_CARD:
        {
            //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "received vl_dsg_async_data_callback VL_DSG_PVT_TAG_IPU_PKT_ECM_TO_CARD, ulQualifier = %d\n", ulQualifier);
            switch(ulQualifier)
            {
                case VL_XCHAN_FLOW_TYPE_IP_U:
                {
                    VL_DSG_IP_FLOW_PACKET * pIpPacket = ((VL_DSG_IP_FLOW_PACKET*)pStruct);
                    vlXchanUpdateInComingDsgCardIPUCounters(pIpPacket->nLength);
                    return POD_STACK_SEND_ExtCh_Data(pIpPacket->pData, pIpPacket->nLength, vlg_IPU_svc_FlowId);
                }
                break;
            }
        }
        break;

        case VL_DSG_PVT_TAG_IPU_DHCP_TIMEOUT:
        {
            //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "received vl_dsg_async_data_callback VL_DSG_PVT_TAG_IPU_PKT_ECM_TO_CARD, ulQualifier = %d\n", ulQualifier);
            switch(ulQualifier)
            {
                case VL_XCHAN_FLOW_TYPE_IP_U:
                {
                    unsigned char ipaddr[VL_IP_ADDR_SIZE] = {0,0,0,0};
                    vlg_bDsgIpuFlowRequestPending = 0;
                    APDU_XCHAN2POD_newIPUflowCnf(vlGetXChanSession(), VL_XCHAN_FLOW_RESULT_DENIED_NO_NETWORK, 1,
                                                0, ipaddr, VL_XCHAN_IP_MAX_PDU_SIZE, NULL);
                    return 1;
                }
                break;
            }
        }
        break;

        case VL_DSG_PVT_TAG_TLV_217:
        {
            if(ulQualifier < 0x10000)
            {
                void * pEventBuf = NULL;
				rmf_osal_memAllocP(RMF_OSAL_MEM_POD, ulQualifier+4,(void **)&pEventBuf);
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "vl_dsg_async_data_callback: received VL_DSG_PVT_TAG_TLV_217, Size = %d\n", ulQualifier);
                memset(pEventBuf, 0, (ulQualifier+4));
                memcpy(pEventBuf, pStruct, ulQualifier);
                vlDsgProxyServer_UpdateTlv217Params((unsigned char *)pEventBuf, ulQualifier);
                {
                    VL_BYTE_STREAM_INST(Tlv217EventBuf, ParseBuf, pEventBuf, ulQualifier);
                    m_bIpModeDetermined = false;
                    vl_dsg_parse_tlv_217(pParseBuf, VL_DSG_TLV_217_TYPE_NONE, ulQualifier);
                    if(!m_bIpModeDetermined)vl_dsg_configure_ip_mode(VL_TLV_217_IP_MODE_IPV4);
                }
                vlSendDsgEventToPFC(CardMgr_DOCSIS_TLV_217,
                                    pEventBuf, ulQualifier);
            }
        }
        break;

        case VL_DSG_PVT_TAG_NON_DSG_MODE:
        {
            switch(vlg_DsgMode.eOperationalMode)
            {
                case VL_DSG_OPERATIONAL_MODE_BASIC_TWO_WAY:
                case VL_DSG_OPERATIONAL_MODE_BASIC_ONE_WAY:
                case VL_DSG_OPERATIONAL_MODE_ADV_TWO_WAY:
                case VL_DSG_OPERATIONAL_MODE_ADV_ONE_WAY:
                case VL_DSG_OPERATIONAL_MODE_IPDM:
                case VL_DSG_OPERATIONAL_MODE_IPDIRECT_UNICAST:
                {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "vl_dsg_control_callback: Posting CardMgr_Device_In_Non_DSG_Mode to PFC\n");
                    vlSendDsgEventToPFC(CardMgr_Device_In_Non_DSG_Mode,
                                        pStruct, ulQualifier);
                }
                break;
            }
                
            switch(vlg_DsgMode.eOperationalMode)
            {
                case VL_DSG_OPERATIONAL_MODE_BASIC_TWO_WAY:
                case VL_DSG_OPERATIONAL_MODE_ADV_TWO_WAY:
                case VL_DSG_OPERATIONAL_MODE_IPDM:
                case VL_DSG_OPERATIONAL_MODE_IPDIRECT_UNICAST:
                {
                    vlDsgSendBootupEventToPFC(RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD_DSG_TWOWAY, RMF_BOOTMSG_DIAG_MESG_CODE_ERROR, 0);
#ifdef GLI
			IARM_Bus_SYSMgr_EventData_t eventData;					
			eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_DSG_CA_TUNNEL;
			eventData.data.systemStates.state = 0;
			eventData.data.systemStates.error = 1;
			IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
#endif
                }
                break;
                    
                case VL_DSG_OPERATIONAL_MODE_BASIC_ONE_WAY:
                case VL_DSG_OPERATIONAL_MODE_ADV_ONE_WAY:
                {
                    vlDsgSendBootupEventToPFC(RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD_DSG_ONEWAY, RMF_BOOTMSG_DIAG_MESG_CODE_ERROR, 0);
#ifdef GLI
			IARM_Bus_SYSMgr_EventData_t eventData;					
			eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_DSG_CA_TUNNEL;
			eventData.data.systemStates.state = 0;
			eventData.data.systemStates.error = 1;
			IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
#endif
                }
                break;
            }
        }
        break;
        
        case VL_DSG_PVT_TAG_ECM_OPERATIONAL_NOW:
        {
            rmf_osal_threadCreate(vl_dsg_stb_dhcp_preliminary_polling_thread, NULL, 0, 0, &g_prePollThreadId, "vl_dsg_stb_dhcp_preliminary_polling_thread");
#ifdef ENABLE_SD_NOTIFY
	    sd_notifyf(0, "READY=1\n"
		"STATUS=ECM is fully initialized and operational\n"
		"MAINPID=%lu",
		(unsigned long) getpid());
#endif
        }
        break;
    }

    return 1;
}

void vlDsgSendIPUdataToECM(unsigned long FlowID, int Size, unsigned char * pData)
{
    VL_DSG_IP_FLOW_PACKET ipPacket;
    ipPacket.flowid  = FlowID;
    ipPacket.nLength = Size;
    ipPacket.pData   = pData;
    if(CHALDsg_Set_Config(VL_CM_DSG_DEVICE_INSTANCE, VL_DSG_PVT_TAG_IPU_PKT_CARD_TO_ECM, &ipPacket) < 0)
    {
        APDU_XCHAN2POD_lostFlowInd(vlGetXChanSession(), FlowID, VL_XCHAN_LOST_FLOW_REASON_NET_BUSY_OR_DOWN);
    }
}

// the dsg thread control
static void dsgUnprotectedProc( void * par )
{
 //   char *ptr=NULL;
    LCSM_EVENT_INFO       *eventInfo = (LCSM_EVENT_INFO *)par;

    if (eventInfo == NULL)
    	return;

    switch( eventInfo->event )
    {
        // POD requests open session (x=resId, y=tcId)
        case POD_OPEN_SESSION_REQ:
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "Dsg_OpenSessReq called\n");
            AM_OPEN_SS_REQ_DSG(eventInfo->x, eventInfo->y);
        }
        break;

        // POD confirms open session (x=sessNbr, y=resId, z=tcId)
        case POD_SESSION_OPENED:
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "Dsg_SessOpened  called\n");
            AM_SS_OPENED_DSG(eventInfo->x, eventInfo->y, eventInfo->z);
            APDU_DSG2POD_InquireDsgMode((USHORT)eventInfo->x, vlMapToXChanTag((USHORT)eventInfo->x, VL_HOST2POD_INQUIRE_DSG_MODE));
        }
        break;

        // POD closes session (x=sessNbr)
        case POD_CLOSE_SESSION:
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "Dsg_CloseSession  called\n");
            AM_SS_CLOSED_DSG(eventInfo->x);
            vlDsgTerminateDispatch();
        }
        break;

        // POD sends APDU (x=sessNbr, y=APDU buffer ptr, z=APDU byte length)
        case POD_APDU:
        {
            VL_BYTE_STREAM_INST(POD_APDU, ParseBuf, eventInfo->y, eventInfo->z);
            ULONG ulTag = vlParse3ByteLong(pParseBuf);
            ULONG nLength = vlParseCcApduLengthField(pParseBuf);

            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "dsgUnprotectedProc case POD_APDU: received tag 0x%06X of length %d\n", ulTag, nLength);

            if(0x9F8E00 == (ulTag&0xFFFF00))
            {
		  RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "\ndsgUnprotectedProc  calling  xchanProtectedProc");
                xchanProtectedProc(par);
                return;
            }

            switch (ulTag)
            {
                case VL_POD2HOST_SET_DSG_MODE:
                {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "pod > Host CTL: VL_POD2HOST_SET_DSG_MODE attemped \n");
                    APDU_POD2DSG_SetDsgMode((USHORT)eventInfo->x, (UCHAR *)eventInfo->y, (USHORT)eventInfo->z);
                    static VL_DSG_OPERATIONAL_MODE eOperationalMode = VL_DSG_OPERATIONAL_MODE_SCTE55;
                    eOperationalMode = vlGetDsgMode();
                    vlSendDsgEventToPFC(CardMgr_DSG_Operational_Mode,
                                        &(eOperationalMode), sizeof(eOperationalMode));
                    switch(eOperationalMode)
                    {
                        case VL_DSG_OPERATIONAL_MODE_SCTE55:
                        {
                            /* Sep-28-2009: Done on Rx and Tx tune requests
                            vlDsgSendBootupEventToPFC(RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD_OOB, RMF_BOOTMSG_DIAG_MESG_CODE_START, 0);
                            */
                        }
                        break;
                        
                        case VL_DSG_OPERATIONAL_MODE_BASIC_TWO_WAY:
                        case VL_DSG_OPERATIONAL_MODE_ADV_TWO_WAY:
                        case VL_DSG_OPERATIONAL_MODE_IPDM:
                        case VL_DSG_OPERATIONAL_MODE_IPDIRECT_UNICAST:
                        {
                            vlDsgSendBootupEventToPFC(RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD_DSG_TWOWAY, RMF_BOOTMSG_DIAG_MESG_CODE_INIT, 0);
#ifdef GLI
				IARM_Bus_SYSMgr_EventData_t eventData;					
				eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_DSG_CA_TUNNEL;
				eventData.data.systemStates.state = 1;
				eventData.data.systemStates.error = vlg_bDsgScanFailed;
				IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
				eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_DSG_BROADCAST_CHANNEL;
				IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
				system("touch /tmp/dsg_ca_channel");
#endif
                        }
                        break;

                        case VL_DSG_OPERATIONAL_MODE_BASIC_ONE_WAY:
                        case VL_DSG_OPERATIONAL_MODE_ADV_ONE_WAY:
                        {
                            vlDsgSendBootupEventToPFC(RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD_DSG_ONEWAY, RMF_BOOTMSG_DIAG_MESG_CODE_INIT, 0);
#ifdef GLI
				IARM_Bus_SYSMgr_EventData_t eventData;					
				eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_DSG_CA_TUNNEL;
				eventData.data.systemStates.state = 1;
				eventData.data.systemStates.error = vlg_bDsgScanFailed;
				IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
				eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_DSG_BROADCAST_CHANNEL;
				IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
				system("touch /tmp/dsg_ca_channel");
#endif
                        }
                        break;
                    }
                }
                break;

                case VL_POD2HOST_DSG_ERROR:
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "pod > Host CTL: VL_POD2HOST_DSG_ERROR attemped \n");
                    APDU_POD2DSG_DsgError((USHORT)eventInfo->x, (UCHAR *)eventInfo->y, (USHORT)eventInfo->z);
                }
                break;

                case VL_POD2HOST_DSG_DIRECTORY:
                {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "pod > Host CTL: VL_POD2HOST_DSG_DIRECTORY attemped \n");
                    APDU_POD2DSG_DsgDirectory((USHORT)eventInfo->x, (UCHAR *)eventInfo->y, (USHORT)eventInfo->z);
                }
                break;

                default:
                {
                 //   RDK_LOG(RDK_LOG_WARN, "LOG.RDK.POD", "WARN: pod > DSG : Receive bad POD_APDU msg %d\n", *(ptr+2));
                }
                break;
            }
        }
        break;

        default:
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "invalid DSG msg received=0x%x\n", eventInfo->event );
        }
        break;
    }
}

void dsgProtectedProc( void * par )
{
    rmf_osal_mutexAcquire(vlDsgMainGetThreadLock());
    {
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "\nBefore calling dsgUnprotectedProc\n");        		
        dsgUnprotectedProc(par);
    }
    rmf_osal_mutexRelease(vlDsgMainGetThreadLock());
}

void vlSendDsgEventToPFC(int eEventType, void * pData, int nSize)
{

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

void vlDsgSendBootupEventToPFC(int eEventType, int nMsgCode, unsigned long ulData)
{
    PostBootEvent(eEventType, nMsgCode, ulData);
}

//////////////////////////////////////////////////////////////////////////////
int  APDU_DSG2POD_InquireDsgMode (USHORT sesnum, unsigned long tag)
{
    // register this callback only when inquiry is done
    vlDsgResourceRegister();
    return vlDsgSndApdu2Pod(sesnum, tag, 0, NULL);
}

int  APDU_DSG2POD_DsgMessage     (USHORT sesnum, unsigned long tag, VL_DSG_MESSAGE_TYPE eMessageType, UCHAR bytMsgData)
{
    UCHAR aMessage[] = {eMessageType, bytMsgData};
    int nLength = 1;
    if((VL_DSG_MESSAGE_TYPE_2_WAY_OK                == eMessageType) ||
        (VL_DSG_MESSAGE_TYPE_DYNAMIC_CHANNEL_CHANGE == eMessageType) ||
        (VL_DSG_MESSAGE_TYPE_CANNOT_FORWARD_2_WAY   == eMessageType) )
    {
        nLength += 1;
    }

    return vlDsgSndApdu2Pod(sesnum, tag, nLength, aMessage);
}

int  APDU_DSG2POD_SendDcdInfo    (USHORT sesnum, unsigned long tag, UCHAR * pInfo, USHORT len)
{
    switch(vlg_DsgMode.eOperationalMode)
    {
        case VL_DSG_OPERATIONAL_MODE_ADV_TWO_WAY:
        case VL_DSG_OPERATIONAL_MODE_ADV_ONE_WAY:
        {
            // forward DCD only for advanced DSG mode
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "APDU_DSG2POD_SendDcdInfo: Received DCD : Sending to POD as DSG Mode = %d\n", vlg_DsgMode.eOperationalMode);
            return vlDsgSndApdu2Pod(sesnum, tag, len, pInfo);
        }
        break;

        default:
        {
            // do not forward DCD to card in this mode
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "APDU_DSG2POD_SendDcdInfo: Received DCD : Not sending to POD as DSG Mode = %d\n", vlg_DsgMode.eOperationalMode);
            return 1;
        }
        break;
    }

    return 1;
}

//////////////////////////////////////////////////////////////////////////////
int  APDU_POD2DSG_SetDsgMode (USHORT sesnum, UCHAR * apkt, USHORT len)
{
    if(NULL != apkt)
    {
        VL_DSG_APDU_256 *dsgModeAPDU = NULL;
	 rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(VL_DSG_APDU_256), (void **)&dsgModeAPDU);
        dsgModeAPDU->nSession = sesnum;
        dsgModeAPDU->nBytes   = len;
        rmf_osal_memcpy(dsgModeAPDU->aBytApdu, apkt, len, sizeof(dsgModeAPDU->aBytApdu), len );

	 rmf_osal_event_params_t event_params = { 0, (void *)dsgModeAPDU, 0, podmgr_freefn };
	 rmf_osal_event_handle_t event_handle;
		
        if(0 != vlg_dsgModeSerializerMsgQ)
        {
            rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_POD, dsgModeAPDU->nSession, 
		&event_params, &event_handle);
            if ( rmf_osal_eventqueue_add( vlg_dsgModeSerializerMsgQ, event_handle ) != RMF_SUCCESS )
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s: rmf_osal_eventqueue_add Failed \n", __FUNCTION__);
            }	
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s : vlg_dsgModeSerializerMsgQ is %d.\n", __FUNCTION__, vlg_dsgModeSerializerMsgQ);
        }
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s : apkt is NULL.\n", __FUNCTION__);
    }
    
    return 1;
}

//////////////////////////////////////////////////////////////////////////////
static int  sync_APDU_POD2DSG_SetDsgMode (USHORT sesnum, UCHAR * apkt, USHORT len)
{
#ifdef GLI
     IARM_Bus_SYSMgr_EventData_t eventData;
#endif
    VL_BYTE_STREAM_INST(SetDsgMode, ParseBuf, apkt, len);
    ULONG ulTag = vlParse3ByteLong(pParseBuf);
    ULONG nLength = vlParseCcApduLengthField(pParseBuf);
    int bModeJustNowChangedToDSG = 0;
    VL_DSG_OPERATIONAL_MODE previousMode;
		// Oct-09-2008: Bugfix in CCAD

    previousMode = vlg_Old_DsgMode.eOperationalMode;

    rmf_osal_mutexAcquire(vlDsgTunnelGetThreadLock());
    {
        vlParseDsgMode(pParseBuf, &vlg_DsgMode);
    }
    rmf_osal_mutexRelease(vlDsgTunnelGetThreadLock());

    vlPrintDsgMode(0, &vlg_DsgMode);

    if(vlDsgIsDifferentDsgMode(&vlg_DsgMode))
    {
		// Oct-09-2008: Bugfix in CCAD
        vlDsgTerminateDispatch();
        if((VL_DSG_OPERATIONAL_MODE_SCTE55 == vlg_Old_DsgMode.eOperationalMode) &&
           (VL_DSG_OPERATIONAL_MODE_SCTE55 != vlg_DsgMode.eOperationalMode    )     )
        {
            if(vlhal_oobtuner_Capable())
            {
                    while(vlg_bOutstandingOobTxTuneRequest || vlg_bOutstandingOobRxTuneRequest)
	            {
	                if(vlg_bOutstandingOobTxTuneRequest)
	                {
	                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "APDU_POD2DSG_SetDsgMode: Waiting for outstanding OOB TX Tune Request to complete\n");
	                    rmf_osal_threadSleep(100, 0);  
	                }
	                if(vlg_bOutstandingOobRxTuneRequest)
	                {
	                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "APDU_POD2DSG_SetDsgMode: Waiting for outstanding OOB RX Tune Request to complete\n");
	                    rmf_osal_threadSleep(100, 0);  
	                }
	            }
            }
        }

        switch ( vlg_DsgMode.eOperationalMode )
        {
            case VL_DSG_OPERATIONAL_MODE_BASIC_TWO_WAY:
            case VL_DSG_OPERATIONAL_MODE_BASIC_ONE_WAY:
            case VL_DSG_OPERATIONAL_MODE_ADV_TWO_WAY:
            case VL_DSG_OPERATIONAL_MODE_ADV_ONE_WAY:
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: DSG BROKER: Mode just changed to DSG.\n", __FUNCTION__);
                bModeJustNowChangedToDSG = 1;
                KillSocketFlowThread();
                break;
            default:
                break;
        }

        if(vlg_bHostAcquiredIPAddress)
        {
            // dsg mode stb ip address
            vlDsgSendBootupEventToPFC(RMF_BOOTMSG_DIAG_EVENT_TYPE_ESTB_IP, RMF_BOOTMSG_DIAG_MESG_CODE_START, 0);
#ifdef GLI
                eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_ESTB_IP;
    eventData.data.systemStates.state = 1;
    eventData.data.systemStates.error = 0;
    IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
#endif
        }
        vlg_Old_DsgMode = vlg_DsgMode;
        // do any cleanup
        vlg_ucid = 0;
        vlg_bHostAcquiredIPAddress = 0;
        vlg_dsgDownstreamAcquired = 0;
        if(0 == vlg_bEnableDsgBroker)
        {
            // Aug-24-2009 : clean-up IPU stack only after eCM loses sync due to set mode request below.
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: DSG BROKER: Not enabled. Allowing eCM DSG MODE configuration.\n", __FUNCTION__);
            CHALDsg_Set_Config(VL_CM_DSG_DEVICE_INSTANCE, ulTag, &vlg_DsgMode);
        }
        else if( VL_DSG_OPERATIONAL_MODE_IPDIRECT_UNICAST == vlg_DsgMode.eOperationalMode )
        {
          RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: Switching to IP Direct mode.\n", __FUNCTION__);
          CHALDsg_Set_Config(VL_CM_DSG_DEVICE_INSTANCE, ulTag, &vlg_DsgMode);
        }
        else if ( (VL_DSG_OPERATIONAL_MODE_IPDIRECT_UNICAST == previousMode) &&
                  bModeJustNowChangedToDSG )
        {
          RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: Switching from IP Direct mode to DSG.\n", __FUNCTION__);
          // must scan when switching between IP direct and DSG
          CHALDsg_Set_Config(VL_CM_DSG_DEVICE_INSTANCE, ulTag, &vlg_DsgMode);
        }
        else
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: DSG BROKER: Skipping eCM DSG MODE configuration.\n", __FUNCTION__);
        }
        CHALDsg_Send_ControlMessage(VL_CM_DSG_DEVICE_INSTANCE, apkt, len);
        // Aug-24-2009 : clean-up IPU stack only after eCM loses sync due to set mode request above.
        // Oct-02-2009: Commented call to maintain init-reboot state for ipu dhcp // CHALDsg_Set_Config(VL_CM_DSG_DEVICE_INSTANCE, VL_DSG_PVT_TAG_DSG_CLEAN_UP, NULL);

        if(0 != vlg_IPU_svc_FlowId)
        {
            if(vlXchanIsDsgIpuFlow(vlg_IPU_svc_FlowId))
            {
                APDU_XCHAN2POD_lostFlowInd(vlGetXChanSession(), vlg_IPU_svc_FlowId, VL_XCHAN_LOST_FLOW_REASON_NET_BUSY_OR_DOWN);
            }
            else
            {
                APDU_XCHAN2POD_deleteFlowReq(vlGetXChanSession(), vlg_IPU_svc_FlowId);
                // legacy mode stb ip address
                vlDsgSendBootupEventToPFC(RMF_BOOTMSG_DIAG_EVENT_TYPE_ESTB_IP, RMF_BOOTMSG_DIAG_MESG_CODE_START, 0);
#ifdef GLI
                    eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_ESTB_IP;
    eventData.data.systemStates.state = 1;
    eventData.data.systemStates.error = 0;
    IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
#endif
            }
        }
        
        switch(vlg_DsgMode.eOperationalMode)
        {
            case VL_DSG_OPERATIONAL_MODE_SCTE55:
            {
                vlSendDsgEventToPFC(CardMgr_DSG_Leaving_DSG_Mode,
                                    &(vlg_DsgMode.eOperationalMode), sizeof(vlg_DsgMode.eOperationalMode));
                vl_xchan_async_start_ip_over_qpsk();
            }
            break;

            case VL_DSG_OPERATIONAL_MODE_IPDIRECT_UNICAST:
            {
                vlSendDsgEventToPFC(CardMgr_DSG_Leaving_DSG_Mode,
                                    &(vlg_DsgMode.eOperationalMode), sizeof(vlg_DsgMode.eOperationalMode));
            }
            break;

            case VL_DSG_OPERATIONAL_MODE_BASIC_TWO_WAY:
            case VL_DSG_OPERATIONAL_MODE_BASIC_ONE_WAY:
            case VL_DSG_OPERATIONAL_MODE_ADV_TWO_WAY:
            case VL_DSG_OPERATIONAL_MODE_ADV_ONE_WAY:
            case VL_DSG_OPERATIONAL_MODE_IPDM:
            default:
            {
                vlSendDsgEventToPFC(CardMgr_DSG_Entering_DSG_Mode,
                                    &(vlg_DsgMode.eOperationalMode), sizeof(vlg_DsgMode.eOperationalMode));
            }
            break;
        }
    }

    CHALDsg_Set_Config(VL_CM_DSG_DEVICE_INSTANCE, VL_DSG_PVT_TAG_GET_DSG_STATUS, NULL);
    return bModeJustNowChangedToDSG;
}

static void vl_DsgSetDsgModeSerializer_task(void * arg)
{
    VL_DSG_APDU_256 message;
    while(1)
    {
        rmf_osal_event_handle_t event_handle;
        rmf_osal_event_category_t event_category;
        uint32_t event_type;
        rmf_osal_event_params_t  event_params;	
		
        memset(&message, 0 , sizeof( message) ); 
        if(RMF_SUCCESS ==  rmf_osal_eventqueue_get_next_event( vlg_dsgModeSerializerMsgQ,
		&event_handle, &event_category,
		&event_type, &event_params))
        {
	     memcpy( &message, event_params.data, sizeof( message ));
            int bModeJustNowChangedToDSG = sync_APDU_POD2DSG_SetDsgMode(message.nSession, message.aBytApdu, message.nBytes);
            if(vlg_bEnableDsgBroker)
            {
                if((vlg_nDcdBufferLength > 0) && (0 != vlg_nDcdApduTag))
                {
                    if(bModeJustNowChangedToDSG)
                    {
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: DSG BROKER: Mode just changed to DSG.\n", __FUNCTION__);
                        rmf_osal_threadSleep(500, 0);
                        rmf_osal_mutexAcquire(vlDsgMainGetThreadLock());
                        {
                            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: DSG BROKER: Delivering cached DCD.\n", __FUNCTION__);
                            APDU_DSG2POD_SendDcdInfo(vlGetXChanDsgSessionNumber(), vlg_nDcdApduTag, vlg_aBytDcdBuffer, vlg_nDcdBufferLength);
                            if(0 != vlg_dsgBrokerUcid)
                            {
                                vlg_ucid = vlg_dsgBrokerUcid;
                                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: DSG BROKER: Creating 2-Way Polling Thread.\n", __FUNCTION__);
                                rmf_osal_threadCreate(vl_dsg_stb_dhcp_polling_thread, (void *)5000, 0, 0, &g_dhcpPollThreadId, "vl_dsg_stb_dhcp_polling_thread");			
                            }
                            else
                            {
                                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: DSG BROKER: No previous UCID avaialble. Assuming not in 2-Way mode.\n", __FUNCTION__);
                            }
                        }
                        rmf_osal_mutexRelease(vlDsgMainGetThreadLock());
                    }
                    else
                    {
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: DSG BROKER: Mode is not DSG or did not 'change' to DSG.\n", __FUNCTION__);
                    }
                }
                else
                {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: DSG BROKER: No cached DCD available for delivery.\n", __FUNCTION__);
                }
            }
            else
            {
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: DSG BROKER: Not Enabled.\n", __FUNCTION__);
            }
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s: cMsgQRead Failed \n", __FUNCTION__);
            rmf_osal_threadSleep(500, 0);
        }
	 rmf_osal_event_delete(event_handle);
    }
}

void vlDsgGetDocsisStatus(VL_DSG_STATUS * pDsgStatus)
{
    if(NULL != pDsgStatus)
    {
        CHALDsg_Set_Config(VL_CM_DSG_DEVICE_INSTANCE, VL_DSG_PVT_TAG_GET_DSG_STATUS, pDsgStatus);
    }
}

int  APDU_POD2DSG_DsgError (USHORT sesnum, UCHAR * apkt, USHORT len)
{
    VL_BYTE_STREAM_INST(DsgError, ParseBuf, apkt, len);
    ULONG ulTag = vlParse3ByteLong(pParseBuf);
    ULONG nLength = vlParseCcApduLengthField(pParseBuf);

    vlg_DsgError.eErrorStatus = (VL_DSG_ERROR_STATUS)vlParseByte(pParseBuf);
    vlTraceStack(0, VL_TRACE_STACK_TAG_NORMAL, "APDU_POD2DSG_DsgError", "eErrorStatus   = 0x%02x", vlg_DsgError.eErrorStatus   );

    if(vlg_bEnableDsgBroker)
    {
        if(vlg_bDownstreamScanCompleted && vlg_bInvalidDsgChannelAlreadyIndicated)
        {
            if(VL_DSG_ERROR_STATUS_INVALID_DSG_CHANNEL == vlg_DsgError.eErrorStatus)
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s: DSG BROKER: Not delivering INVALID_DSG_CHANNEL error to eCM as full scan was completed.\n", __FUNCTION__);
                return 1;
            }
        }
    }

    rmf_osal_mutexAcquire(vlDsgTunnelGetThreadLock());
    {
        vlg_tLastDcdResponse = rmf_osal_GetUptime();
        vlg_tLastOneWayResponse = vlg_tLastDcdResponse;
        vlg_bInvalidDsgChannelAlreadyIndicated = 1;
    }
    rmf_osal_mutexRelease(vlDsgTunnelGetThreadLock());
    
    CHALDsg_Set_Config(VL_CM_DSG_DEVICE_INSTANCE, ulTag, &vlg_DsgError);

    CHALDsg_Send_ControlMessage(VL_CM_DSG_DEVICE_INSTANCE, apkt, len);

    return 1;
}

// Jul-22-2009: Removed unused vestigeal function: void vlDsgPrepareAdsgFiltersForExtChnlFlows();

int vlClassifyBroadcastTunnels(VL_DSG_DIRECTORY * pDsgDirectory)
{ // called from thread-protected block
   
    int bDirectoryChanged = 0;
    if(NULL == pDsgDirectory) return 0;

    int iHostEntryOuter   = 0;
    int nHostEntries = pDsgDirectory->nHostEntries;
    
    // reset flags
    for(iHostEntryOuter = 0; iHostEntryOuter < nHostEntries; iHostEntryOuter++)
    {
        // is direct term flow ?
        if(VL_DSG_DIR_TYPE_DIRECT_TERM_DSG_FLOW == pDsgDirectory->aHostEntries[iHostEntryOuter].eEntryType)
        {
            // enable by default
            pDsgDirectory->aHostEntries[iHostEntryOuter].clientId.bClientDisabled = 0;
        } // if
    }// for
        
    // for each of the tunnels
    for(iHostEntryOuter = 0; iHostEntryOuter < nHostEntries; iHostEntryOuter++)
    {
        // is direct term flow ?
        if(VL_DSG_DIR_TYPE_DIRECT_TERM_DSG_FLOW == pDsgDirectory->aHostEntries[iHostEntryOuter].eEntryType)
        {
            int iHostEntryInner = 0;
            
            // for each of the matching inner tunnels
            for(iHostEntryInner = 0; iHostEntryInner < nHostEntries; iHostEntryInner++)
            {
                // is direct term flow ?
                if(VL_DSG_DIR_TYPE_DIRECT_TERM_DSG_FLOW == pDsgDirectory->aHostEntries[iHostEntryInner].eEntryType)
                {
                    // check equivalence
                    if((pDsgDirectory->aHostEntries[iHostEntryInner].clientId.eEncodingType ==
                        pDsgDirectory->aHostEntries[iHostEntryOuter].clientId.eEncodingType) &&
                       (pDsgDirectory->aHostEntries[iHostEntryInner].clientId.nEncodedId ==
                        pDsgDirectory->aHostEntries[iHostEntryOuter].clientId.nEncodedId))
                    {
                        // is ucid zero ?
                        if(0 == vlg_ucid)
                        {
                            if(0 != pDsgDirectory->aHostEntries[iHostEntryInner].ucid)
                            {
                                // disable non-zero ucids
                                pDsgDirectory->aHostEntries[iHostEntryInner].clientId.bClientDisabled = 1;
                                bDirectoryChanged = 1;
                            }
                        }
                        else // ucid is non-zero
                        {
                            // is outer client's ucid matching current ucid ? (i.e found a matching ucid)
                            if(vlg_ucid == pDsgDirectory->aHostEntries[iHostEntryOuter].ucid)
                            {
                                // is inner client's ucid non-matching ?
                                if(vlg_ucid != pDsgDirectory->aHostEntries[iHostEntryInner].ucid)
                                {
                                    // disable non-matching ucids
                                    pDsgDirectory->aHostEntries[iHostEntryInner].clientId.bClientDisabled = 1;
                                    bDirectoryChanged = 1;
                                }//if non matched ucid
                            }// is outer client matching
                        }//else ucid non-zero
                        
                        // check if either tunnel is already disabled
                        if((0 == pDsgDirectory->aHostEntries[iHostEntryInner].clientId.bClientDisabled) &&
                            (0 == pDsgDirectory->aHostEntries[iHostEntryOuter].clientId.bClientDisabled))
                        {
                            // check priority
                            if(pDsgDirectory->aHostEntries[iHostEntryInner].adsgFilter.nTunnelPriority <
                               pDsgDirectory->aHostEntries[iHostEntryOuter].adsgFilter.nTunnelPriority)
                            {
                                // disable lower priority clients
                                pDsgDirectory->aHostEntries[iHostEntryInner].clientId.bClientDisabled = 1;
                                bDirectoryChanged = 1;
                            }// if priority
                        }// if disabled
                    }//if equivalence
                }// if direct term : inner loop
            }// for : inner loop
        }// if direct term : outer loop
    }// for : outer loop
    
    return bDirectoryChanged;
}

int vlDsgIsSpecifiedBroadcastTablesFromCableCard(VL_DSG_BCAST_CLIENT_ID eClientId)
{
    int bIsFromCableCard = TRUE;
    rmf_osal_mutexAcquire(vlDsgTunnelGetThreadLock());
    {
        int iHostEntry   = 0;
        int nHostEntries = vlg_DsgDirectory.nHostEntries;

        if(vlIsDsgOrIpduMode() && vlg_bDsgDirectoryFilterValid)
        {
            for(iHostEntry = 0; iHostEntry < nHostEntries; iHostEntry++)
            {
                if(VL_DSG_DIR_TYPE_EXT_CHNL_MPEG_FLOW == vlg_DsgDirectory.aHostEntries[iHostEntry].eEntryType)
                {
                    if((VL_DSG_CLIENT_ID_ENC_TYPE_BCAST == vlg_DsgDirectory.aHostEntries[iHostEntry].clientId.eEncodingType) &&
                        (eClientId                      == vlg_DsgDirectory.aHostEntries[iHostEntry].clientId.nEncodedId))
                    {
                        bIsFromCableCard = TRUE;
                        break;
                    }
                }
                else if(VL_DSG_DIR_TYPE_DIRECT_TERM_DSG_FLOW == vlg_DsgDirectory.aHostEntries[iHostEntry].eEntryType)
                {
                    if((VL_DSG_CLIENT_ID_ENC_TYPE_BCAST == vlg_DsgDirectory.aHostEntries[iHostEntry].clientId.eEncodingType) &&
                        (eClientId                      == vlg_DsgDirectory.aHostEntries[iHostEntry].clientId.nEncodedId))
                    {
                        bIsFromCableCard = FALSE;
                        break;
                    }
                }
            }
        }
    }
    rmf_osal_mutexRelease(vlDsgTunnelGetThreadLock());
    
    return bIsFromCableCard;
}

int vlDsgIsScte65TablesFromCableCard()
{
    return vlDsgIsSpecifiedBroadcastTablesFromCableCard(VL_DSG_BCAST_CLIENT_ID_SCTE_65);
}

int vlDsgIsEasTablesFromCableCard()
{
    return vlDsgIsSpecifiedBroadcastTablesFromCableCard(VL_DSG_BCAST_CLIENT_ID_SCTE_18);
}

int vlDsgIsXaitTablesFromCableCard()
{
    return vlDsgIsSpecifiedBroadcastTablesFromCableCard(VL_DSG_BCAST_CLIENT_ID_XAIT);
}

int vlDsgAreAdsgFiltersIdentical(VL_ADSG_FILTER * pLeftFilter, VL_ADSG_FILTER * pRightFilter)
{ // called from thread-protected block
    if(pLeftFilter->nTunneId        != pRightFilter->nTunneId       ) return 0;
    if(pLeftFilter->nTunnelPriority != pRightFilter->nTunnelPriority) return 0;
    if(pLeftFilter->macAddress      != pRightFilter->macAddress     ) return 0;
    if(pLeftFilter->ipAddress       != pRightFilter->ipAddress      ) return 0;
    if(pLeftFilter->ipMask          != pRightFilter->ipMask         ) return 0;
    if(pLeftFilter->destIpAddress   != pRightFilter->destIpAddress  ) return 0;
    if(pLeftFilter->nPortStart      != pRightFilter->nPortStart     ) return 0;
    if(pLeftFilter->nPortEnd        != pRightFilter->nPortEnd       ) return 0;
    
    // Filters are identical.
    return 1;
}

int vlDsgAreClientIdsIdentical(VL_DSG_CLIENT_ID * pLeftId, VL_DSG_CLIENT_ID * pRightId)
{ // called from thread-protected block
    if(pLeftId->eEncodingType       != pRightId->eEncodingType      ) return 0;
    if(pLeftId->nEncodingLength     != pRightId->nEncodingLength    ) return 0;
    if(pLeftId->nEncodedId          != pRightId->nEncodedId         ) return 0;
    if(pLeftId->bClientDisabled     != pRightId->bClientDisabled    ) return 0;
    
    // Ids are identical.
    return 1;
}

int vlDsgAreHostFiltersIdentical(VL_DSG_HOST_ENTRY * pLeftFilter, VL_DSG_HOST_ENTRY * pRightFilter)
{ // called from thread-protected block
    if(!vlDsgAreClientIdsIdentical(&(pLeftFilter->clientId), &(pRightFilter->clientId)))        return 0;
    if(pLeftFilter->eEntryType      != pRightFilter->eEntryType     )                           return 0;
    if(pLeftFilter->ucid            != pRightFilter->ucid           )                           return 0;
    if(VL_DSG_DIR_TYPE_DIRECT_TERM_DSG_FLOW == pLeftFilter->eEntryType)
    {
        if(!vlDsgAreAdsgFiltersIdentical(&(pLeftFilter->adsgFilter), &(pRightFilter->adsgFilter))) return 0;
    }
    
    // Filters are identical.
    return 1;
}

int vlDsgAreCardFiltersIdentical(VL_DSG_CARD_ENTRY * pLeftFilter, VL_DSG_CARD_ENTRY * pRightFilter)
{ // called from thread-protected block
    return vlDsgAreAdsgFiltersIdentical(&(pLeftFilter->adsgFilter), &(pRightFilter->adsgFilter));
}

int vlDsgRemoveOldTunnels(VL_DSG_DIRECTORY * pOldDirectory, VL_DSG_DIRECTORY * pNewDirectory)
{ // called from thread-protected block
    if(NULL == pOldDirectory) return 0;
    if(NULL == pNewDirectory) return 0;
    
    int nEntries = 0;
    int iOldEntry = 0;
    // check host entries
    for(iOldEntry = 0; iOldEntry < pOldDirectory->nHostEntries; iOldEntry++)
    {
        VL_DSG_HOST_ENTRY * pOldEntry = &(pOldDirectory->aHostEntries[iOldEntry]);
        int bFound = 0;
        int iNewEntry = 0;
        
        for(iNewEntry = 0; iNewEntry < pNewDirectory->nHostEntries; iNewEntry++)
        {
            VL_DSG_HOST_ENTRY * pNewEntry = &(pNewDirectory->aHostEntries[iNewEntry]);
            
            if(vlDsgAreHostFiltersIdentical(pOldEntry, pNewEntry))
            {
                bFound = 1;
                break;
            }
        }
        if(!bFound)
        {
            // old entry not present in new collection so remove...
            if((VL_DSG_DIR_TYPE_DIRECT_TERM_DSG_FLOW == pOldEntry->eEntryType) && (!pOldEntry->clientId.bClientDisabled))
            {
                CHALDsg_Set_Config(VL_CM_DSG_DEVICE_INSTANCE, VL_DSG_PVT_TAG_REMOVE_HOST_TUNNEL, pOldEntry);
            }
            vlPrintDsgHostEntry(0, pOldEntry);
            nEntries++;
        }
    }
    if(nEntries > 0)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s : Removed above %d HOST entries.\n", __FUNCTION__, nEntries);
    }
    nEntries = 0;
    // check card entries
    for(iOldEntry = 0; iOldEntry < pOldDirectory->nCardEntries; iOldEntry++)
    {
        VL_DSG_CARD_ENTRY * pOldEntry = &(pOldDirectory->aCardEntries[iOldEntry]);
        int bFound = 0;
        int iNewEntry = 0;

        for(iNewEntry = 0; iNewEntry < pNewDirectory->nCardEntries; iNewEntry++)
        {
            VL_DSG_CARD_ENTRY * pNewEntry = &(pNewDirectory->aCardEntries[iNewEntry]);

            if(vlDsgAreCardFiltersIdentical(pOldEntry, pNewEntry))
            {
                bFound = 1;
                break;
            }
        }
        if(!bFound)
        {
            // old entry not present in new collection so remove...
            CHALDsg_Set_Config(VL_CM_DSG_DEVICE_INSTANCE, VL_DSG_PVT_TAG_REMOVE_CARD_TUNNEL, pOldEntry);
            vlPrintDsgCardEntry(0, pOldEntry);
            nEntries++;
        }
    }
    if(nEntries > 0)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s : Removed above %d CARD entries.\n", __FUNCTION__, nEntries);
    }
    
    return 1;
}

int vlDsgAddNewTunnels(VL_DSG_DIRECTORY * pOldDirectory, VL_DSG_DIRECTORY * pNewDirectory)
{ // called from thread-protected block
    if(NULL == pOldDirectory) return 0;
    if(NULL == pNewDirectory) return 0;
    
    int nEntries = 0;
    int iNewEntry = 0;
    // check host entries
    for(iNewEntry = 0; iNewEntry < pNewDirectory->nHostEntries; iNewEntry++)
    {
        VL_DSG_HOST_ENTRY * pNewEntry = &(pNewDirectory->aHostEntries[iNewEntry]);
        int bFound = 0;
        int iOldEntry = 0;

        for(iOldEntry = 0; iOldEntry < pOldDirectory->nHostEntries; iOldEntry++)
        {
            VL_DSG_HOST_ENTRY * pOldEntry = &(pOldDirectory->aHostEntries[iOldEntry]);
            
            if(vlDsgAreHostFiltersIdentical(pOldEntry, pNewEntry))
            {
                // Register handle of already existing tunnel in the new entry.
                pNewEntry->adsgFilter.hTunnelHandle = pOldEntry->adsgFilter.hTunnelHandle;
                bFound = 1;
                break;
            }
        }
        if(!bFound)
        {
            // new entry not present in old collection so add...
            if((VL_DSG_DIR_TYPE_DIRECT_TERM_DSG_FLOW == pNewEntry->eEntryType) && (!pNewEntry->clientId.bClientDisabled))
            {
                CHALDsg_Set_Config(VL_CM_DSG_DEVICE_INSTANCE, VL_DSG_PVT_TAG_ADD_HOST_TUNNEL, pNewEntry);
            }
            vlPrintDsgHostEntry(0, pNewEntry);
            nEntries++;
        }
    }
    if(nEntries > 0)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s : Added above %d HOST entries.\n", __FUNCTION__, nEntries);
    }
    nEntries = 0;
    // check card entries
    for(iNewEntry = 0; iNewEntry < pNewDirectory->nCardEntries; iNewEntry++)
    {
        VL_DSG_CARD_ENTRY * pNewEntry = &(pNewDirectory->aCardEntries[iNewEntry]);
        int bFound = 0;
        int iOldEntry = 0;
        
        for(iOldEntry = 0; iOldEntry < pOldDirectory->nCardEntries; iOldEntry++)
        {
            VL_DSG_CARD_ENTRY * pOldEntry = &(pOldDirectory->aCardEntries[iOldEntry]);
            
            if(vlDsgAreCardFiltersIdentical(pNewEntry, pOldEntry))
            {
                // Register handle of already existing tunnel in the new entry.
                pNewEntry->adsgFilter.hTunnelHandle = pOldEntry->adsgFilter.hTunnelHandle;
                bFound = 1;
                break;
            }
        }
        if(!bFound)
        {
            // new entry not present in old collection so add...
            CHALDsg_Set_Config(VL_CM_DSG_DEVICE_INSTANCE, VL_DSG_PVT_TAG_ADD_CARD_TUNNEL, pNewEntry);
            vlPrintDsgCardEntry(0, pNewEntry);
            nEntries++;
        }
    }
    if(nEntries > 0)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s : Added above %d CARD entries.\n", __FUNCTION__, nEntries);
    }
    
    return 1;
}

int  vlDsgSetInitialDsgDirectory(unsigned long ulTag, VL_DSG_DIRECTORY * pDirectory)
{ // called from thread-protected block
    if(NULL == pDirectory) return 0;
    
    CHALDsg_Set_Config(VL_CM_DSG_DEVICE_INSTANCE, ulTag, pDirectory);
    
    // Success
    return 1;
}

int  vlDsgSetDsgDirectoryConfig(VL_DSG_DIRECTORY * pDirectory)
{ // called from thread-protected block
    if(NULL == pDirectory) return 0;
    
    CHALDsg_Set_Config(VL_CM_DSG_DEVICE_INSTANCE, VL_DSG_PVT_TAG_SET_DIR_CONFIG, pDirectory);
    
    // Success
    return 1;
}

int  vlDsgUpdateDsgDirectoryConfig(VL_DSG_DIRECTORY * pDirectory)
{ // called from thread-protected block
    if(NULL == pDirectory) return 0;
    
    CHALDsg_Set_Config(VL_CM_DSG_DEVICE_INSTANCE, VL_DSG_PVT_TAG_UPDATE_DIR_CONFIG, pDirectory);
    
    // Success
    return 1;
}

int  vlDsgHalDoesNotSupportsPartialUpdates()
{ // called from thread-protected block
    return (VL_DSG_RESULT_HAL_SUPPORTS != CHALDsg_Set_Config(VL_CM_DSG_DEVICE_INSTANCE, VL_DSG_PVT_TAG_GET_DIR_MOD_CAPS, 0));
}

int  vlDsgReclassifyDirectoryForUcid()
{
    rmf_osal_mutexAcquire(vlDsgTunnelGetThreadLock());
    {
        if(vlClassifyBroadcastTunnels(&vlg_DsgDirectory))
        {
            vlDsgApplyDsgDirectory();
        }
    }
    rmf_osal_mutexRelease(vlDsgTunnelGetThreadLock());
    return 1;
}

int  vlDsgApplyDsgDirectory()
{ // called from thread-protected block
    if(vlDsgHalDoesNotSupportsPartialUpdates() || !vlg_bPartialDsgDirectory)
    {
        memset(&vlg_PreviousDsgDirectory, 0, sizeof( vlg_PreviousDsgDirectory ) ); // Reset previous directory.
        vlDsgSetInitialDsgDirectory(VL_POD2HOST_DSG_DIRECTORY, &vlg_DsgDirectory);
        vlPrintDsgDirectory(0, &vlg_DsgDirectory);
        vlDsgUpdateDsgDirectoryConfig(&vlg_DsgDirectory);
    }
    else
    {
        vlDsgSetDsgDirectoryConfig(&vlg_DsgDirectory);
        vlDsgRemoveOldTunnels(&vlg_PreviousDsgDirectory, &vlg_DsgDirectory);
        vlDsgAddNewTunnels   (&vlg_PreviousDsgDirectory, &vlg_DsgDirectory);
        vlDsgUpdateDsgDirectoryConfig(&vlg_DsgDirectory);
        if(rdk_dbg_enabled("LOG.RDK.POD", RDK_LOG_DEBUG))
        {
            vlPrintDsgDirectory(0, &vlg_DsgDirectory);
        }
    }
    
    memset(&vlg_dsgTunnelStats,0, sizeof(vlg_dsgTunnelStats));             // Reset tunnel stats.
    vlg_PreviousDsgDirectory = vlg_DsgDirectory;    // Save new directory for future comparisons.
    vlg_bPartialDsgDirectory = 1;                   // Partial directories hence forth.
    vlg_bDsgDirectoryFilterValid = TRUE;            // Mark DSG filter as valid.
    return 1;
}

int  APDU_POD2DSG_DsgDirectory (USHORT sesnum, UCHAR * apkt, USHORT len)
{
    VL_BYTE_STREAM_INST(DsgDirectory, ParseBuf, apkt, len);
    ULONG ulTag = vlParse3ByteLong(pParseBuf);
    ULONG nLength = vlParseCcApduLengthField(pParseBuf);
    vlDsgProxyServer_UpdateDsgDirectory(VL_BYTE_STREAM_GET_CURRENT_BUF(pParseBuf), VL_BYTE_STREAM_REMAINDER(pParseBuf));

    rmf_osal_mutexAcquire(vlDsgTunnelGetThreadLock());
    {
        vlg_tLastDcdResponse = rmf_osal_GetUptime();
        vlg_tLastOneWayResponse = vlg_tLastDcdResponse;
        vlg_bDownstreamScanCompleted = 0;
        vlg_bInvalidDsgChannelAlreadyIndicated = 0;
        // Mar-11-2013: Do not terminate dispatch //vlDsgTerminateDispatch();
        vlParseDsgDirectory(pParseBuf, &vlg_DsgDirectory);
        vlClassifyBroadcastTunnels(&vlg_DsgDirectory);
        vlDsgApplyDsgDirectory();
    }
    rmf_osal_mutexRelease(vlDsgTunnelGetThreadLock());

    CHALDsg_Send_ControlMessage(VL_CM_DSG_DEVICE_INSTANCE, apkt, len);

    if(vlg_DsgDirectory.bVctIdIncluded)
    {
        vlg_nDsgVctId = vlg_DsgDirectory.nVctId;
        vlg_bVctIdReceived = 1;
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s : Card issued DSG VCT-ID = 0x%04X.\n", __FUNCTION__, vlg_nDsgVctId);
        // Aug-12-2009: The VCT-ID event needs to be posted to stack only when
        //              the scte tables are being received from eCM.
        /* To avoid false event during init */
        if( vlg_nDsgVctId != persistedDsgVctId )
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s :Calling vlMpeosClearSiDatabase because change in  VCT_ID[old vcdId=%d,new vctId=%d]\n",__FUNCTION__,vlg_nOldDsgVctId,vlg_nDsgVctId);
            vlMpeosClearSiDatabase();
            vlMpeosNotifyVCTIDChange();
            persistedDsgVctId = vlg_nOldDsgVctId = vlg_nDsgVctId;
        }

        if(FALSE == vlDsgIsScte65TablesFromCableCard())
        {
            vlSendDsgEventToPFC(CardMgr_DSG_Cable_Card_VCT_ID,
                            &(vlg_DsgDirectory.nVctId), sizeof(vlg_DsgDirectory.nVctId));
        }
    }
    
    if(!vlg_dsgDownstreamAcquired)
    {
        vlg_dsgDownstreamAcquired = 1;
        vlSendDsgEventToPFC(CardMgr_DSG_Downstream_Acquired,
                            NULL, 0);
                
        switch(vlg_DsgMode.eOperationalMode)
        {
            case VL_DSG_OPERATIONAL_MODE_BASIC_TWO_WAY:
            case VL_DSG_OPERATIONAL_MODE_ADV_TWO_WAY:
            case VL_DSG_OPERATIONAL_MODE_IPDM:
            case VL_DSG_OPERATIONAL_MODE_IPDIRECT_UNICAST:
            {
                vlDsgSendBootupEventToPFC(RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD_DSG_TWOWAY, RMF_BOOTMSG_DIAG_MESG_CODE_START, 0);
#ifdef GLI
		IARM_Bus_SYSMgr_EventData_t eventData;					
		eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_DOCSIS;
		eventData.data.systemStates.state = 1;
		eventData.data.systemStates.error = 0;
		IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));

		eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_DSG_CA_TUNNEL;
		eventData.data.systemStates.state = 2;
		eventData.data.systemStates.error = 0;
		IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));

		eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_ECM_IP;
		eventData.data.systemStates.state = 1;
		eventData.data.systemStates.error = 0;
		IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));

		eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_ESTB_IP;
		eventData.data.systemStates.state = 1;
		eventData.data.systemStates.error = 0;
		IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
#endif
            }
            break;
                    
            case VL_DSG_OPERATIONAL_MODE_BASIC_ONE_WAY:
            case VL_DSG_OPERATIONAL_MODE_ADV_ONE_WAY:
            {
                vlDsgSendBootupEventToPFC(RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD_DSG_ONEWAY, RMF_BOOTMSG_DIAG_MESG_CODE_COMPLETE, 0);
#ifdef GLI
		IARM_Bus_SYSMgr_EventData_t eventData;					
		eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_DOCSIS;
		eventData.data.systemStates.state = 2;
		eventData.data.systemStates.error = 0;
		IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
#endif

            }
            break;
        }
    }
    
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s : vlDsgIsScte65TablesFromCableCard() returned %s\n", __FUNCTION__, VL_TRUE_COND_STR(vlDsgIsScte65TablesFromCableCard()));
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s : vlDsgIsEasTablesFromCableCard   () returned %s\n", __FUNCTION__, VL_TRUE_COND_STR(vlDsgIsEasTablesFromCableCard   ()));
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s : vlDsgIsXaitTablesFromCableCard  () returned %s\n", __FUNCTION__, VL_TRUE_COND_STR(vlDsgIsXaitTablesFromCableCard  ()));

    return 1;
}

int  APDU_POD2DSG_ConfigAdvDsg (USHORT sesnum, UCHAR * apkt, USHORT len)
{
    VL_BYTE_STREAM_INST(ConfigAdvDsg, ParseBuf, apkt, len);
    ULONG ulTag = vlParse3ByteLong(pParseBuf);
    ULONG nLength = vlParseCcApduLengthField(pParseBuf);
    vlDsgProxyServer_UpdateAdvConfig(VL_BYTE_STREAM_GET_CURRENT_BUF(pParseBuf), VL_BYTE_STREAM_REMAINDER(pParseBuf));

    rmf_osal_mutexAcquire(vlDsgTunnelGetThreadLock());
    {
        vlg_tLastDcdResponse = rmf_osal_GetUptime();
        vlg_tLastOneWayResponse = vlg_tLastDcdResponse;
        vlg_bDownstreamScanCompleted = 0;
        vlg_bInvalidDsgChannelAlreadyIndicated = 0;
        // Mar-11-2013: Do not terminate dispatch //vlDsgTerminateDispatch();
        vlParseDsgAdvConfig(pParseBuf, &vlg_DsgAdvConfig);
        vlPrintDsgAdvConfig(0, &vlg_DsgAdvConfig);
        CHALDsg_Set_Config(VL_CM_DSG_DEVICE_INSTANCE, ulTag, &vlg_DsgAdvConfig);
    }
    rmf_osal_mutexRelease(vlDsgTunnelGetThreadLock());


    CHALDsg_Send_ControlMessage(VL_CM_DSG_DEVICE_INSTANCE, apkt, len);
    
    if(!vlg_dsgDownstreamAcquired)
    {
        vlg_dsgDownstreamAcquired = 1;
        vlSendDsgEventToPFC(CardMgr_DSG_Downstream_Acquired,
                            NULL, 0);
                
        switch(vlg_DsgMode.eOperationalMode)
        {
            case VL_DSG_OPERATIONAL_MODE_BASIC_TWO_WAY:
            case VL_DSG_OPERATIONAL_MODE_ADV_TWO_WAY:
            case VL_DSG_OPERATIONAL_MODE_IPDM:
            case VL_DSG_OPERATIONAL_MODE_IPDIRECT_UNICAST:
            {
                vlDsgSendBootupEventToPFC(RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD_DSG_TWOWAY, RMF_BOOTMSG_DIAG_MESG_CODE_START, 0);
#ifdef GLI
		IARM_Bus_SYSMgr_EventData_t eventData;					
		eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_DOCSIS;
		eventData.data.systemStates.state = 1;
		eventData.data.systemStates.error = 0;
		IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));

		eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_DSG_CA_TUNNEL;
		eventData.data.systemStates.state = 2;
		eventData.data.systemStates.error = 0;
		IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));

		eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_ECM_IP;
		eventData.data.systemStates.state = 1;
		eventData.data.systemStates.error = 0;
		IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));

		eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_ESTB_IP;
		eventData.data.systemStates.state = 1;
		eventData.data.systemStates.error = 0;
		IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
#endif
            }
            break;
                    
            case VL_DSG_OPERATIONAL_MODE_BASIC_ONE_WAY:
            case VL_DSG_OPERATIONAL_MODE_ADV_ONE_WAY:
            {
                vlDsgSendBootupEventToPFC(RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD_DSG_ONEWAY, RMF_BOOTMSG_DIAG_MESG_CODE_COMPLETE, 0);
#ifdef GLI
		IARM_Bus_SYSMgr_EventData_t eventData;					
		eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_DOCSIS;
		eventData.data.systemStates.state = 2;
		eventData.data.systemStates.error = 0;
		IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
#endif
            }
            break;
        }
    }

    return 1;
}

/**************************************************
 *	APDU Functions to handle DSG
 *************************************************/
int vlDsgSndApdu2Pod(unsigned short sesnum, unsigned long tag, unsigned short dataLen, unsigned char *data)
{

    if(0 == sesnum)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: Not sending APDU TAG 0x%06X as sesnum = %d.\n", __FUNCTION__, tag, sesnum);
        return APDU_DSG_FAILURE;
    }
	
    unsigned short alen;
    unsigned char papdu[MAX_APDU_HEADER_BYTES + dataLen];

    if (buildApdu(papdu, &alen, tag, dataLen, data ) == NULL)
    {
        return APDU_DSG_FAILURE;
    }

    AM_APDU_FROM_HOST(sesnum, papdu, alen);
/*
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "dsgSndApdu : APDU len = %d\n",alen);
    for(int i =0;i<alen;i++)
    {
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "0x%02x ", papdu[i]);
    }
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "\n");
*/
    return APDU_DSG_SUCCESS;
}

int vlDsgIsValidClientId(VL_DSG_CLIENT_ID_ENC_TYPE eClientType, unsigned long long nClientId)
{
    int i = 0;
    int bValid = 0;
    
    rmf_osal_mutexAcquire(vlDsgTunnelGetThreadLock());
    {
        if(vlg_bDsgConfigFilterValid)
        {
            for(i = 0; i < vlg_DsgAdvConfig.nTunnelFilters; i++)
            {
                if(nClientId == vlg_DsgAdvConfig.aTunnelFilters[i].nAppId)
                {
                    bValid = 1;
                    break;
                }
            }
        }
        else if(vlg_bDsgDirectoryFilterValid)
        {
            for(i = 0; i < vlg_DsgDirectory.nHostEntries; i++)
            {
                if((eClientType == vlg_DsgDirectory.aHostEntries[i].clientId.eEncodingType) &&
                   (nClientId   == vlg_DsgDirectory.aHostEntries[i].clientId.nEncodedId   )) 
                {
                    bValid = 1;
                    break;
                }
            }
        }
    }
    rmf_osal_mutexRelease(vlDsgTunnelGetThreadLock());
    
    return bValid;
}

int vlDsgSendAppTunnelRequestToCableCard(int nAppIds, unsigned short * paAppIds)
{
    switch(vlXchanGetResourceId())
    {
        case VL_XCHAN_RESOURCE_ID_VER_2:
        case VL_XCHAN_RESOURCE_ID_VER_3:
        case VL_XCHAN_RESOURCE_ID_VER_4:
        {
            int nRet = APDU_DSG_FAILURE;
            int nBufSize = ((nAppIds*2) + 32);
            if((NULL != paAppIds) && (nAppIds >= 0))
            {
                unsigned char * pMessage = NULL;
				rmf_osal_memAllocP(RMF_OSAL_MEM_POD, nBufSize,(void **)&pMessage);
                if(NULL != pMessage)
                {
                    int i = 0;
                    int nFinalLength = 0;
                    VL_BYTE_STREAM_INST(AppIdMessage, WriteBuf, pMessage, nBufSize);
                    
                    nFinalLength += vlWriteByte (pWriteBuf, VL_DSG_MESSAGE_TYPE_APP_TUNNEL_REQ);
                    nFinalLength += vlWriteByte (pWriteBuf, nAppIds);
                    
                    for(i = 0; i < nAppIds; i++)
                    {
                        nFinalLength += vlWriteShort(pWriteBuf, paAppIds[i]);
                    }
                    
                    nRet = vlDsgSndApdu2Pod(vlGetXChanDsgSessionNumber(), VL_XCHAN_HOST2POD_DSG_MESSAGE, nFinalLength, pMessage);
					rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pMessage);
                }
            }
            return nRet;
        }
        break;
        
        default:
        {
            // this message should be sent only for versions 2, 3, 4
        }
        break;
    }
    
    return APDU_DSG_SUCCESS;
}

void vlDsgDumpDsgStats()
{
    int nStats = 0;
    int iStat = 0;
    const char * pszFileName = "/tmp/dsg_flow_stats.txt";
    static VL_DSG_TRAFFIC_TEXT_STATS aDsgServiceStats[VL_MAX_DSG_ENTRIES];
    static VL_DSG_TRAFFIC_TEXT_STATS aXchanServiceStats[VL_XCHAN_FLOW_TYPE_MAX];
    static VL_VCT_ID_COUNTER         aDsgVctStats[VL_MAX_DSG_ENTRIES];

    vlDsgGetDsgTextStats(VL_MAX_DSG_ENTRIES, aDsgServiceStats, &nStats);

    FILE * fp = fopen(pszFileName, "w");
    if(NULL == fp)
    {
         RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: Unable to open file '%s'.\n", __FUNCTION__,  pszFileName);
        return;
    }
    else
    {
        for (iStat = 0; (iStat < nStats); iStat++)
        {
            fprintf(fp, "Service %i: Type: %s, ID: %s, Path: %s, Packets: %s\n", iStat,
                              aDsgServiceStats[iStat].pszClientType,
                              aDsgServiceStats[iStat].pszClientId,
                              aDsgServiceStats[iStat].pszEntryType,
                              aDsgServiceStats[iStat].szPkts);
        }

 	int nStats = VL_XCHAN_FLOW_TYPE_MAX;
        int iStat = 0;
        vlDsgGetXchanTextStats(aXchanServiceStats);

        for (iStat = 0; (iStat < nStats); iStat++)
        {
            fprintf(fp, "Flow %i: Type: %s, Packets: %s\n", iStat,
                              aXchanServiceStats[iStat].pszEntryType,
                              aXchanServiceStats[iStat].szPkts);
        }

        nStats = 0;
        iStat = 0;

        vlDsgGetDsgVctStats(VL_MAX_DSG_ENTRIES, aDsgVctStats, &nStats);
        char szVctId[32];
        char szCount[32];

        for (iStat = 0; (iStat < nStats); iStat++)
        {
            fprintf(fp, "VCT %c: VCT-ID: %u, Packets: %u%s\n", 'A'+iStat,
                              (unsigned int)aDsgVctStats[iStat].nVctId,
                              (unsigned int)aDsgVctStats[iStat].nCount,
                              ((vlDsgGetVctId() == aDsgVctStats[iStat].nVctId)?" : <-- Selected":""));
        }
        fclose(fp);
    }
#ifdef USE_IPDIRECT_UNICAST
    vlIpDirectUnicastDumpIpDirectUnicastStats();
#endif // USE_IPDIRECT_UNICAST
}
#ifdef __cplusplus
}
#endif


