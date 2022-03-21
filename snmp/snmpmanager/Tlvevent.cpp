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



#include "ipcclient_impl.h"
#include "ip_types.h"
#include "Tlvevent.h"
#include "SnmpIORM.h"
#include "vlpluginapp_haldsgapi.h"
#include "vlSnmpClient.h"
#include "utilityMacros.h"
#include <arpa/inet.h>
#include "snmpTargetAddrTable.h"
#include "snmpNotifyTable.h"
#include "snmpTargetParamsTable.h"
#include "vlSnmpEcmApi.h"
#include "sys_init.h"
#include "vlMutex.h"
#include "coreUtilityApi.h"
#include "cardManagerIf.h"
#define __MTAG__ VL_SNMP_MANAGER
#include "cardMibAccessProxyServer.h"
#include "errno.h"
#include "rfcapi.h"
#include <fstream>
#ifdef __cplusplus
extern "C" {
#endif
#include"docsDevEventTable_enums.h"
#include"snmpAccessInclude.h"
#include "cardUtils.h"
#include "core_events.h"
#include "dsg_types.h"
#include "vlEnv.h"

#if USE_SYSRES_MLT
#include "rpl_new.h"
#include "rpl_malloc.h"
#endif

       #include <sys/types.h>
       #include <sys/stat.h>
       #include <unistd.h>

//extern snmpV1V2CommunityTableEventhandling(char *tlvparseCm);

#ifdef __cplusplus
}
#endif

#define vlMemSet(p,v,s,t) memset(p,v,s)


/** Listens to TVL217config file and dispatches them for configureation */
//#include "cardMibAccessProxyServer.h"


#define vlStrCpy(pDest, pSrc, nDestCapacity)            \
            strcpy(pDest, pSrc)

#define vlMemCpy(pDest, pSrc, nCount, nDestCapacity)            \
            memcpy(pDest, pSrc, nCount)

#define VL_ZERO_MEMORY(x) \
            memset(&x,0,sizeof(x))  

#define vlStrCat(pDest, pSrc, nCount)    strcat(pDest,pSrc)

extern "C" void vlInitCardMibAccessProxy(char * pszRootOID, int nOidLen);

static vlMutex & vlg_TlvEventDblock = TlvConfig::vlGetTlvEventDbLock();
rmf_osal_eventqueue_handle_t m_pEvent_queue;
//#undef SNMP_DEBUGPRINT
//#define SNMP_DEBUGPRINT(a,args...) fprintf( stderr, "%s:%d:"#a"\n", __FUNCTION__,__LINE__, ##args)
vlMutex & TlvConfig::vlGetTlvEventDbLock()
{
    static vlMutex lock;
    return lock;
}


//! Constructor.
TlvConfig::TlvConfig():CMThread("vlTlvConfig")
{VL_AUTO_LOCK(vlg_TlvEventDblock);

     m_pEvent_queue = NULL;
     rmf_osal_eventqueue_create((const uint8_t*)"TlvConfig", &m_pEvent_queue);
     m_pTlv217Data      = NULL;
     m_nTlv217DataBytes = 0;
     registerBootupConfig();
}

TlvConfig::~TlvConfig()
{VL_AUTO_LOCK(vlg_TlvEventDblock);
     m_pTlv217Data      = NULL;
     m_nTlv217DataBytes = 0;
rmf_osal_eventqueue_delete(m_pEvent_queue);
}
#if 0
void TlvConfig::setVolatileConfig(VL_TLV_SNMP_STRING_VAR_BIND_LIST & raConfig)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    int nCount = raConfig.size();

    VL_SNMP_CLIENT_VALUE * paClientValues = new VL_SNMP_CLIENT_VALUE[nCount];
    VLALLOC_INFO2(paClientValues, (sizeof(VL_SNMP_CLIENT_VALUE)*nCount));

    for(int i = 0; i < nCount; i++)
    {
        VL_ZERO_STRUCT(paClientValues[i]);
        paClientValues[i].pszOID = raConfig[i].strOid.c_str();
        paClientValues[i].eValueType =
                (VL_SNMP_CLIENT_VALUE_TYPE)(raConfig[i].eVarBindType);
        paClientValues[i].Value.pszValue = raConfig[i].strValue.c_str();
    }

    vlSnmpSetLocalObjects(&nCount, paClientValues, VL_SNMP_CLIENT_REQUEST_TYPE_SYNCHRONOUS);

    VLFREE_INFO2(paClientValues);
    delete [] paClientValues;
}
#endif
bool TlvConfig::m_bUpdateSnmpdConf = true;
void TlvConfig::DisableSnmpdConfUpdate(const char * pszFunction, int nLineNo)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

      m_bUpdateSnmpdConf = false;
      SNMP_DEBUGPRINT("m_bUpdateSnmpdConf ====  %d \n",m_bUpdateSnmpdConf);
      SNMP_DEBUGPRINT("%s(): called from %s(): line %d:\n", __FUNCTION__, pszFunction, nLineNo);
}

void TlvConfig::RejectV1V2CoexistenceConfig(const char * pszFunction, int nLineNo)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    SNMP_DEBUGPRINT("%s(): called from %s(): line %d:\n", __FUNCTION__, pszFunction, nLineNo);
}

void TlvConfig::RejectV3AccessViewConfig(const char * pszFunction, int nLineNo)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    SNMP_DEBUGPRINT("%s(): called from %s(): line %d:\n", __FUNCTION__, pszFunction, nLineNo);
}

void TlvConfig::ProcessTlv217Data()
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    if((NULL != m_pTlv217Data) && (m_nTlv217DataBytes > 0))
    {
        // clear data first
        V3AccviewLs.clear();
        SNMPmibObjectList.clear();
        VendorIdList.clear();
        IpModeControlList.clear();
        VendorSpecificList.clear();
        tlvdefault_UIlist.clear();
        Tlv217loopList.clear();
        v3NotificationReceiver_list.clear();
        
        //cSleep(5000);
        VL_BYTE_STREAM_INST(pTlv217Data, ParseBuf, m_pTlv217Data, m_nTlv217DataBytes);
        SNMP_DEBUGPRINT("\n TLV parse calling :: vlSnmpParseTlv217Data \n ");
        //vlMpeosDumpBuffer(RDK_LOG_DEBUG, "LOG.RDK.SNMP", m_pTlv217Data, m_nTlv217DataBytes);
        vlSnmpParseTlv217Data(pParseBuf, /*VL_DSG_TLV_217_TYPE_NONE,*/ m_nTlv217DataBytes);
        free(m_pTlv217Data);
        m_pTlv217Data = NULL;
        m_nTlv217DataBytes = 0;
    }
}

void TlvConfig::taskProcessTlv217Data(void *pParam)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    TlvConfig * pThis = ((TlvConfig*)pParam);
    pThis->ProcessTlv217Data();
}

void TlvConfig::loadLocalTlv217ConfigFile()
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    int ret = 0;
    int len	= 0;
    char tlv217LocalFile[API_CHRMAX];
    char * pfcroot_fsnmp = getenv("PFC_ROOT");
    char file_name[] = "/bin/target-snmp/sbin/localTlv217.cfg";	
    strcpy(tlv217LocalFile, "");
    if( pfcroot_fsnmp == NULL )
    {
       RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SNMP", "PFC_ROOT is NULL \n");
	return;
    }		
    len = strlen (pfcroot_fsnmp);
    if ( API_CHRMAX <= ( (len + 1) + (sizeof(file_name)) ) )
    {
       RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SNMP", "PFC_ROOT is not correct  \n");
	return;
    }
    snprintf( tlv217LocalFile, sizeof(tlv217LocalFile), "%s%s", pfcroot_fsnmp,file_name);

    struct stat st;
    VL_ZERO_MEMORY(st);

    FILE * fp = fopen(tlv217LocalFile, "rb");
    if( NULL == fp ) return;	

    ret = stat(tlv217LocalFile, &st);
    if( ret != 0 ) 
    {
              fclose(fp);    	
		return;
    }

    if(st.st_size <= 0)
    {
              fclose(fp);    	
		return;    
    }

        rmf_osal_threadSleep(5000,0);
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "%s(): stat '%s', size = %lld .\n", __FUNCTION__, tlv217LocalFile, st.st_size);

        if(NULL != fp)
        {
            // TLV 217 present in config file
            unsigned char * pAlloc = NULL;
            int nSize   = 0;
            int nOffset = 0;
            struct stat * pSt = &st;

            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "%s(): fopen('%s') Succeeded .\n", __FUNCTION__, tlv217LocalFile);

            nSize   = pSt->st_size;
            nOffset = 0;

            pAlloc = (unsigned char*)malloc(nSize+4);

            // allocate buffer
            if(NULL != pAlloc)
            {
                vlMemSet(pAlloc, 0, nSize+4, nSize+4);
                // read file
                int nRead = 0;
                fseek(fp, nOffset, SEEK_SET);
                nRead = fread(pAlloc, 1, nSize, fp);

                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "%s(): read %d bytes from '%s' .\n", __FUNCTION__, nRead, tlv217LocalFile);

                if(nRead > 0)
                {
                    VL_BYTE_STREAM_INST(pLocalTlv217Data, ParseBuf, pAlloc, nRead);

                    unsigned long ulTlv = -1;
                    int nTotalTlvLength = 0;

                    while((VL_BYTE_STREAM_REMAINDER(pParseBuf) > 0) && (ulTlv != 0xFF))
                    {
                        ulTlv = vlParseByte(pParseBuf);
                        int nTlvLength = vlParseByte(pParseBuf);

                        if(TSnmpTlv217 == ulTlv)
                        {
                            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "%s(): Accumulating %d bytes from '%s' .\n", __FUNCTION__, nTlvLength, tlv217LocalFile);
                            // extract the TLV-217 data (overwrites source buffer)
                            vlParseBuffer(pParseBuf, pAlloc+nTotalTlvLength, nTlvLength, nSize-nTotalTlvLength);
                            nTotalTlvLength += nTlvLength;
                        }
                        else
                        {
                            // discard ir-relevant data
                            vlParseBuffer(pParseBuf, NULL, nTlvLength, 0);
                        }
                    }

                    if(nTotalTlvLength > 0)
                    {                        
                        rmf_osal_ThreadId threadId;
                        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "%s(): Accumulated a total of %d bytes from '%s' .\n", __FUNCTION__, nTotalTlvLength, tlv217LocalFile);
                        m_pTlv217Data = (unsigned char*)pAlloc;
                        m_nTlv217DataBytes = nTotalTlvLength;
//                        cThreadCreateSimple("vlTaskProcessTlv217Data", taskProcessTlv217Data, (unsigned long)(this));
                        rmf_osal_threadCreate(taskProcessTlv217Data, (void *) (this), RMF_OSAL_THREAD_PRIOR_DFLT, RMF_OSAL_THREAD_STACK_SIZE, &threadId, "vlTaskProcessTlv217Data");
                    }
                    else
                    {
                        free(pAlloc);
                    }
                }
                else
                {
                    free(pAlloc);
                }
            }
            fclose(fp);
        }
//auto_unlock(&vlg_TlvEventDblock);

}

#define VL_STORE_ECM_CONFIG(fp, pszTag, val)                                        \
{                                                                                   \
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "    ## eCM " pszTag " ##\n", (val));     \
    if(NULL != fp) fprintf(fp, pszTag "\n", (val));                                 \
}                                                                                   \

//! Main thread routine. Pulls events from the #event_queue and dispatches them in #epgm.
void TlvConfig::run(void)
{
    /*For TLV217*/
//    void* pEventData = NULL;
//    pfcEventManager *em = pfc_kernel_global->get_event_manager();
//    pfcEventBase *event=NULL;
    int result = 0;
	rmf_Error ret;

    rmf_osal_eventmanager_handle_t em = 0;// vivek IPC_CLIENT_get_pod_event_manager();
    rmf_osal_event_handle_t event_handle;
    rmf_osal_event_params_t event_params = {0};    
    rmf_osal_event_category_t event_category;
    uint32_t event_type;

    // check if a local config file is available for use
    loadLocalTlv217ConfigFile();
    
    while (1)
    {
        SNMP_DEBUGPRINT("\n TlvConfig ::: Start 1::::::::::: pfcEventManager for TLV217 parsing \n");
        if(m_pEvent_queue == NULL)
        {

            SNMP_DEBUGPRINT("\n FATAL FATAL: npEvent-queue is NULL  \n");
            break;
        }

        ret = rmf_osal_eventqueue_get_next_event(m_pEvent_queue, &event_handle, &event_category, &event_type, &event_params );

//        event = m_pEvent_queue->get_next_event();
	if( RMF_SUCCESS != ret )
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SNMP", "%s(): event is NULL\n", __FUNCTION__);
		rmf_osal_event_delete( event_handle );
		rmf_osal_threadSleep(100, 0);
		continue;		
	}
        VL_AUTO_LOCK(vlg_TlvEventDblock);
        SNMP_DEBUGPRINT("\n m_pEvent_queue->get_next_event() :: event %0x \n", event_handle);
//        pEventData = (void*)event->get_data();
        SNMP_DEBUGPRINT("TlvConfig   :run:event=0x%x,category=%d;type=%d\n",event_handle, event_category, event_type);
        switch (event_category)
        {
			case RMF_OSAL_EVENT_CATEGORY_PODMGR: /*Added for verification, to remove : please post appropriate event from ipcclient_impl.cpp*/
            case RMF_OSAL_EVENT_CATEGORY_CARD_MANAGER:
                switch (event_type)
                {
                    case CardMgr_DOCSIS_TLV_217:
                    {
                        unsigned char * pTlv217Data = NULL;
			   pTlv217Data = (unsigned char *)malloc(event_params.data_extension);
                        //rmf_osal_memAllocP(RMF_OSAL_MEM_POD,event_params.data_extension, (void **)&pTlv217Data);
                        memcpy(pTlv217Data, event_params.data, event_params.data_extension);	
//                        pTlv217Data  =  (unsigned char *)event_params.data; //(unsigned char*)event->get_data();
                        int nLength =event_params.data_extension;
                        //int nLength = ((int)(((pfcEventCardMgr*)pEventData)->data_item_count()));
                        SNMP_DEBUGPRINT("received CardMgr_DOCSIS_TLV_217, Size = %d\n", nLength);
                        // For testing not sending to parse
                        if(NULL == pTlv217Data)
                        {
                            // no TLV 217 present in config file
                            //SNMP_DEBUGPRINT("\n NO TLV FOUND :: pTlv217Data == NULL \n ");
                            //SNMP_DEBUGPRINT("\n ERROR :: NO TLV FOUND :: :::::: \n");//,ERRORCODE_I401_1);
                            EdocsDevEventhandling(ERRORCODE_I401_1, TSnmpTlv217);
                            DisableSnmpdConfUpdate(__FUNCTION__, __LINE__); // Reset-the global m_bUpdateSnmpdConf to false
                        }
                        else
                        {
                            if(0 == nLength)
                            {
                                SNMP_DEBUGPRINT("\n ERROR ::  NO TLV FOUND :: nLength == 0\n");//,ERRORCODE_I401_1);
                                EdocsDevEventhandling(ERRORCODE_I401_1, TSnmpTlv217);
                                DisableSnmpdConfUpdate(__FUNCTION__, __LINE__); // Reset-the global m_bUpdateSnmpdConf to false
                                // no TLV 217 present in config file
                                free(pTlv217Data);
                            }
                            else
                            {
                                static bool bTlv217Processed = false;
                                if(!bTlv217Processed)
                                {
                                    rmf_osal_ThreadId threadId;
                                    bTlv217Processed = true;
                                    // TLV 217 present in config file
                                    m_pTlv217Data = pTlv217Data;
                                    m_nTlv217DataBytes = nLength;
                                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "%s(): TLV-217 Not yet processed for this bootup. Processing new TLV-217 of %d bytes\n", __FUNCTION__, nLength);
                                    rmf_osal_threadCreate(taskProcessTlv217Data, (void *) (this), RMF_OSAL_THREAD_PRIOR_DFLT, RMF_OSAL_THREAD_STACK_SIZE, &threadId, "vlTaskProcessTlv217Data");
                                    //cThreadCreateSimple("vlTaskProcessTlv217Data", taskProcessTlv217Data, (unsigned long)(this));
                                }
                                else
                                {
                                    free(pTlv217Data);
                                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "%s(): TLV-217 Already processed for this bootup. Ignoring duplicate TLV-217 of %d bytes\n", __FUNCTION__, nLength);
                                }
                            }
                        }
                    }  //CardMgr_DOCSIS_TLV_217:
                    break;

                    case CardMgr_Card_Mib_Access_Root_OID:
                    {
                        unsigned char * pCardMibRootOid = NULL;
                        pCardMibRootOid  =   (unsigned char*)event_params.data;
                        int nLength =(int)event_params.data_extension;
                        if((NULL != pCardMibRootOid) && (nLength > 0))
                        {
                            vlInitCardMibAccessProxy((char*)pCardMibRootOid, nLength);
                        }
                    }
                    break;

                    case CardMgr_Card_Mib_Access_Snmp_Reply:
                    {
                        unsigned char * pCardMibReply = NULL;
                        pCardMibReply  =   (unsigned char*)event_params.data;
                        int nLength =(int)event_params.data_extension;
                        if((NULL != pCardMibReply) && (nLength > 0))
                        {
                            vlReturnCardMibAccessReply(pCardMibReply, nLength);
                        }
                    }
                    break;

                    case CardMgr_Card_Mib_Access_Shutdown:
                    {
                        vlShutdownCardMibAccessProxy();
                    }
                    break;

                    case CardMgr_POD_IP_ACQUIRED:
                    {
                        VL_HOST_IP_INFO * pPodIpInfo = (VL_HOST_IP_INFO*)(event_params.data);

                        if(NULL != pPodIpInfo)
                        {
                            if(VL_XCHAN_IP_ADDRESS_TYPE_IPV4 == pPodIpInfo->ipAddressType)
                            {
                                Set_AllListV1v2v3objvalus(pPodIpInfo);
                            }
                            else
                            {
                                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "%s(): Received unsupported IP Config '%d' for in CardMgr_POD_IP_ACQUIRED\n", __FUNCTION__, pPodIpInfo->ipAddressType);
                            }
                        }
                        else
                        {
                            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "%s(): Received NULL IP Config for in CardMgr_POD_IP_ACQUIRED\n", __FUNCTION__);
                        }
                    }
                    break;

                    case CardMgr_OOB_Downstream_Acquired:
                    {
                        EdocsDevEventhandling(ERRORCODE_B14_0);
                    }
                    break;

                    case CardMgr_DSG_Operational_Mode:
                    {
                        VL_DSG_OPERATIONAL_MODE * pOperationalMode = (VL_DSG_OPERATIONAL_MODE *)(event_params.data);
                        if(NULL != pOperationalMode)
                        {
                            switch(*pOperationalMode)
                            {
                                case VL_DSG_OPERATIONAL_MODE_SCTE55:
                                {
                                    EdocsDevEventhandling(ERRORCODE_B15_0);
                                }
                                break;
                            }
                        }
                    }
                    break;

                    case CardMgr_DSG_Entering_DSG_Mode:
                    {
                        VL_DSG_OPERATIONAL_MODE * pOperationalMode = (VL_DSG_OPERATIONAL_MODE *)(event_params.data);
                        if(NULL != pOperationalMode)
                        {
                            switch(*pOperationalMode)
                            {
                                case VL_DSG_OPERATIONAL_MODE_ADV_TWO_WAY:
                                {
                                    EdocsDevEventhandling(ERRORCODE_B16_0);
                                }
                                break;

                                case VL_DSG_OPERATIONAL_MODE_ADV_ONE_WAY:
                                {
                                    EdocsDevEventhandling(ERRORCODE_B17_0);
                                }
                                break;
                            }
                        }
                    }
                    break;

                    case CardMgr_DSG_Downstream_Acquired:
                    {
                        EdocsDevEventhandling(ERRORCODE_B18_0);
                    }
                    break;

                    case CardMgr_DSG_IP_ACQUIRED:
                    {
                        EdocsDevEventhandling(ERRORCODE_B19_0);
                        static bool bDocsDevInfoDumped = false;
                        if(!bDocsDevInfoDumped)
                        {
                            bDocsDevInfoDumped = true;
                        VL_SNMP_DOCS_DEV_INFO docsDevInfo;
                        vlSnmpEcmDocsDevInfo(&docsDevInfo);
                        char szIpAddress[VL_IPV6_ADDR_STR_SIZE];
                            const char * pszComma = "";
                        //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "vlSnmpEcmDocsDevInfo() returned %d\n", ret);
                        //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", " docsDevInfo.docsDevRole                          = %d\n", docsDevInfo.docsDevRole                          );
                            FILE * fp = fopen("/tmp/docsisConfig.txt", "w");
                            char szDocsDateTime[VL_SNMP_STR_SIZE];
                            signed short nYear  = (docsDevInfo.docsDevDateTime[0]<<8)|(docsDevInfo.docsDevDateTime[1]<<0);
                            signed short nMonth = docsDevInfo.docsDevDateTime[2];
                            signed short nDay   = docsDevInfo.docsDevDateTime[3];
                            signed short nHour  = docsDevInfo.docsDevDateTime[4];
                            signed short nMin   = docsDevInfo.docsDevDateTime[5];
                            signed short nSec   = docsDevInfo.docsDevDateTime[6];
                            bool bIsUTC = (0 == docsDevInfo.docsDevDateTime[8]);
                            if(!bIsUTC) pszComma = ",";
                            snprintf(szDocsDateTime, sizeof(szDocsDateTime), "%04d-%02d-%02d,%02d:%02d:%02d.%1d%s%c%d:%d", nYear, docsDevInfo.docsDevDateTime[2], docsDevInfo.docsDevDateTime[3],
                                                                                                                               docsDevInfo.docsDevDateTime[4], docsDevInfo.docsDevDateTime[5], docsDevInfo.docsDevDateTime[6], docsDevInfo.docsDevDateTime[7],
                                                                                                                               pszComma, docsDevInfo.docsDevDateTime[8], docsDevInfo.docsDevDateTime[9], docsDevInfo.docsDevDateTime[10]);
                            
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "    ##################################################################\n");
                            VL_STORE_ECM_CONFIG(fp, "SerialNumber                : %26s", docsDevInfo.docsDevSerialNumber                  );
                            VL_STORE_ECM_CONFIG(fp, "STPControl                  : %26d", docsDevInfo.docsDevSTPControl                    );
                            VL_STORE_ECM_CONFIG(fp, "IgmpModeControl             : %26d", docsDevInfo.docsDevIgmpModeControl               );
                            VL_STORE_ECM_CONFIG(fp, "MaxCpe                      : %26d", docsDevInfo.docsDevMaxCpe                        );
                            VL_STORE_ECM_CONFIG(fp, "SwServer                    : %26s", inet_ntop(AF_INET, docsDevInfo.docsDevSwServer, szIpAddress, sizeof(szIpAddress)));
                            VL_STORE_ECM_CONFIG(fp, "SwFilename                  : %26s", docsDevInfo.docsDevSwFilename                    );
                            VL_STORE_ECM_CONFIG(fp, "SwAdminStatus               : %26d", docsDevInfo.docsDevSwAdminStatus                 );
                            VL_STORE_ECM_CONFIG(fp, "SwOperStatus                : %26d", docsDevInfo.docsDevSwOperStatus                  );
                            VL_STORE_ECM_CONFIG(fp, "SwCurrentVers               : %26s", docsDevInfo.docsDevSwCurrentVers                 );
                            VL_STORE_ECM_CONFIG(fp, "SwServerAddressType         : %26d", docsDevInfo.docsDevSwServerAddressType           );
                            VL_STORE_ECM_CONFIG(fp, "SwServerAddress             : %26s", inet_ntop(((docsDevInfo.docsDevSwServerAddressType==VL_XCHAN_IP_ADDRESS_TYPE_IPV6)?AF_INET6:AF_INET), docsDevInfo.docsDevSwServerAddress, szIpAddress, sizeof(szIpAddress)));
                            VL_STORE_ECM_CONFIG(fp, "SwServerTransportProtocol   : %26d", docsDevInfo.docsDevSwServerTransportProtocol     );
                            VL_STORE_ECM_CONFIG(fp, "ServerBootState             : %26d", docsDevInfo.docsDevServerBootState               );
                            VL_STORE_ECM_CONFIG(fp, "ServerDhcp                  : %26s", inet_ntop(AF_INET, docsDevInfo.docsDevServerDhcp, szIpAddress, sizeof(szIpAddress)));
                            VL_STORE_ECM_CONFIG(fp, "ServerTime                  : %26s", inet_ntop(AF_INET, docsDevInfo.docsDevServerTime, szIpAddress, sizeof(szIpAddress)));
                            VL_STORE_ECM_CONFIG(fp, "ServerTftp                  : %26s", inet_ntop(AF_INET, docsDevInfo.docsDevServerTftp, szIpAddress, sizeof(szIpAddress)));
                            VL_STORE_ECM_CONFIG(fp, "ServerConfigFile            : %26s", docsDevInfo.docsDevServerConfigFile              );
                            VL_STORE_ECM_CONFIG(fp, "ServerDhcpAddressType       : %26d", docsDevInfo.docsDevServerDhcpAddressType         );
                            VL_STORE_ECM_CONFIG(fp, "ServerDhcpAddress           : %26s", inet_ntop(((docsDevInfo.docsDevServerDhcpAddressType==VL_XCHAN_IP_ADDRESS_TYPE_IPV6)?AF_INET6:AF_INET), docsDevInfo.docsDevServerDhcpAddress, szIpAddress, sizeof(szIpAddress)));
                            VL_STORE_ECM_CONFIG(fp, "ServerTimeAddressType       : %26d", docsDevInfo.docsDevServerTimeAddressType         );
                            VL_STORE_ECM_CONFIG(fp, "ServerTimeAddress           : %26s", inet_ntop(((docsDevInfo.docsDevServerTimeAddressType==VL_XCHAN_IP_ADDRESS_TYPE_IPV6)?AF_INET6:AF_INET), docsDevInfo.docsDevServerTimeAddress, szIpAddress, sizeof(szIpAddress)));
                            VL_STORE_ECM_CONFIG(fp, "ServerConfigTftpAddressType : %26d", docsDevInfo.docsDevServerConfigTftpAddressType   );
                            VL_STORE_ECM_CONFIG(fp, "ServerConfigTftpAddress     : %26s", inet_ntop(((docsDevInfo.docsDevServerConfigTftpAddressType==VL_XCHAN_IP_ADDRESS_TYPE_IPV6)?AF_INET6:AF_INET), docsDevInfo.docsDevServerConfigTftpAddress, szIpAddress, sizeof(szIpAddress)));
                            VL_STORE_ECM_CONFIG(fp, "DateTime                    : %26s", szDocsDateTime                                   );
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "    ##################################################################\n");
                            if(NULL != fp) fclose(fp);
                            
                            const int nEpochYear = 1970;
                            if(nYear >= nEpochYear)
                            {
                                struct tm stmUTC;
                                time_t tStb;
                                // get stb time
                                time(&tStb);
                                gmtime_r(&tStb, &stmUTC);
                                int nYearNorm  = 0;
                                int nMonthNorm = 0;
                                int nDayNorm   = 0;
                                if(!bIsUTC)
                                {
                                    int nSign = -1; // Assume local time is positive to UTC so a negative adjustment is required to get to UTC time.
                                    // Convert to UTC
                                    if('-' == docsDevInfo.docsDevDateTime[8])
                                    {
                                        nSign = 1; // Local time is negative to UTC so a positive adjustment is needed to get to UTC time.
                                    }
                                    nMin  = nMin + nSign*(docsDevInfo.docsDevDateTime[10]);
                                    if(nMin < 0) // values > 60 are handled transparently
                                    {
                                        nMin += 60;
                                        nHour -= 1;
                                    }
                                    nHour = nHour + nSign*(docsDevInfo.docsDevDateTime[9]);
                                    if(nHour < 0) // values > 24 are handled transparently
                                    {
                                        nHour += 24;
                                        nDay -= 1; // a value of 0 adjusts everything else automatically
                                    }
                                }
                                // normalize docsis time
                                if(nYear  >= nEpochYear) nYearNorm  = nYear - nEpochYear;
                                if(nMonth >  0         ) nMonthNorm = nMonth - 1;
                                if(nDay   >  0         ) nDayNorm   = nDay - 1;
                                // normalize stb time
                                stmUTC.tm_year += 1900;
                                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "%s: Current UTC time for STB is now \"%04d-%02d-%02d %02d:%02d:%02d\".\n", __FUNCTION__, stmUTC.tm_year, stmUTC.tm_mon+1, stmUTC.tm_mday, stmUTC.tm_hour, stmUTC.tm_min, stmUTC.tm_sec);
                                if(stmUTC.tm_year >= nEpochYear) stmUTC.tm_year -= nEpochYear;
                                if(stmUTC.tm_mday >  0         ) stmUTC.tm_mday -= 1;
#define VL_ESTIMATE_TIME(year, month, day, hour, min, sec) (((year)*365*24*60*60) + ((month)*30*24*60*60) + ((day)*1*24*60*60) + ((hour)*1*1*60*60) + ((min)*1*1*1*60) + ((sec)*1*1*1*1))
                                time_t tStbEst    = VL_ESTIMATE_TIME(stmUTC.tm_year, stmUTC.tm_mon, stmUTC.tm_mday, stmUTC.tm_hour, stmUTC.tm_min, stmUTC.tm_sec);
                                time_t tDocsisEst = VL_ESTIMATE_TIME(nYearNorm, nMonthNorm, nDayNorm, nHour, nMin, nSec);
                                time_t tDiff = vlAbs(tDocsisEst-tStbEst);
                                int bEnableDsgBroker = vl_env_get_bool(0, "FEATURE.DOCSIS_SETTOP_TIME_BASE");
                                if(bEnableDsgBroker)
                                {
                                    if(tDocsisEst > tStbEst)
                                    {
                                        if(tDiff > (25*60*60)) // Use DOCSIS adjustment only if the time is off by more than 25 hours.
                                        {
                                                char strTimeCmd[VL_SNMP_STR_SIZE];
                                                snprintf(strTimeCmd, sizeof(strTimeCmd), "date -u -s \"%04d-%02d-%02d %02d:%02d:%02d\"", nYear, nMonth, nDay, nHour, nMin, nSec);
                                                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "%s: DSG BROKER: Detected a time difference of %d seconds. Updating system time with '%s'.\n", __FUNCTION__, (int)(tDocsisEst-tStbEst), strTimeCmd);
                                                system(strTimeCmd);
							IPC_CLIENT_SNMPSetTime( nYear, nMonth, nDay, nHour, nMin, nSec);
                                                time(&tStb);
                                                gmtime_r(&tStb, &stmUTC);
                                                stmUTC.tm_year += 1900;
                                                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "%s: Updated UTC time for STB is now \"%04d-%02d-%02d %02d:%02d:%02d\".\n", __FUNCTION__, stmUTC.tm_year, stmUTC.tm_mon+1, stmUTC.tm_mday, stmUTC.tm_hour, stmUTC.tm_min, stmUTC.tm_sec);

                                        }
                                        else
                                        {
                                            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "%s: Detected a time difference of only %d seconds. Not updating system time.\n", __FUNCTION__, (int)(tDocsisEst-tStbEst));
                                        }
                                    }
                                    else
                                    {
                                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "%s: DSG BROKER: Docsis time is behind STB time by %d seconds. Not updating STB time.\n", __FUNCTION__, vlAbs(tDocsisEst-tStbEst));
                                    }
                                }
                                else
                                {
                                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "%s: DSG BROKER: Detected a time difference of %d seconds. Not updating system time as DSG BROKER is disabled.\n", __FUNCTION__, (int)(tDocsisEst-tStbEst));
                                }
                            }
                        }
                    }
                    break;

                    case CardMgr_VCT_Channel_Map_Acquired:
                    {
                        EdocsDevEventhandling(ERRORCODE_B20_0);
                    }
                    break;

                    case CardMgr_CP_Res_Opened                 : EdocsDevEventhandling(ERRORCODE_B03_0); break;
                    case CardMgr_Host_Auth_Sent                : EdocsDevEventhandling(ERRORCODE_B04_0); break;
                    case CardMgr_Bnd_Fail_Card_Reasons         : EdocsDevEventhandling(ERRORCODE_B05_0); break;
                    case CardMgr_Bnd_Fail_Invalid_Host_Cert    : EdocsDevEventhandling(ERRORCODE_B06_0); break;
                    case CardMgr_Bnd_Fail_Invalid_Host_Sig     : EdocsDevEventhandling(ERRORCODE_B07_0); break;
                    case CardMgr_Bnd_Fail_Invalid_Host_AuthKey : EdocsDevEventhandling(ERRORCODE_B08_0); break;
                    case CardMgr_Bnd_Fail_Other                : EdocsDevEventhandling(ERRORCODE_B09_0); break;
                    case CardMgr_Card_Val_Error_Val_Revoked    : EdocsDevEventhandling(ERRORCODE_B10_0); break;
                    case CardMgr_Bnd_Fail_Incompatible_Module  : EdocsDevEventhandling(ERRORCODE_B11_0); break;
                    case CardMgr_Bnd_Comp_Card_Host_Validated  : EdocsDevEventhandling(ERRORCODE_B12_0); break;
                    case CardMgr_CP_Initiated                  : EdocsDevEventhandling(ERRORCODE_B13_0); break;

                    case CardMgr_CDL_Host_Img_DL_Cmplete               : EdocsDevEventhandling(ERRORCODE_B28_0); break;
                    case CardMgr_CDL_CVT_Error                         : EdocsDevEventhandling(ERRORCODE_B29_0); break;

                    case CardMgr_CDL_Host_Img_DL_Error                 :
                    {
                        int * pParam = (int*)(event_params.data);
                        if(NULL != pParam)
                        {
                            EdocsDevEventhandling(ERRORCODE_B30_0, *pParam);
                        }
                    }
                    break;

                    case CardMgr_Card_Image_DL_Complete        : EdocsDevEventhandling(ERRORCODE_B31_0); break;

                    default:
                    {
                        SNMP_DEBUGPRINT("\n Not handling event type 0x%08X\n", event_type);
                    }
                    break;
                }
                break;//PFC_EVENT_CATEGORY_CARD_MANAGER
                
                default:
                {
                    SNMP_DEBUGPRINT("\n Not handling event category %d.\n ", event_category);
                }
                break;
        }

        rmf_osal_event_delete(event_handle);
//auto_unlock(&vlg_TlvEventDblock);

          //#endif //if 0
    }//while

#ifdef TEST_WITH_PODMGR
	rmf_snmpmgrUnRegisterEvents(m_pEvent_queue);
#endif
    SNMP_DEBUGPRINT("\n TlvConfig ::: End::::::::::: pfcEventManager for TLV217 parsing \n");
    /** End of pfcEventMgr For the TLV 217 parsing and config the SNMP*/
}

VL_TLV_217_FIELD_DATA TlvConfig::vlSnmpProcessTlv217Field (VL_BYTE_STREAM * pParentStream, int nParentLength)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    VL_BYTE_STREAM_PARENT_CHILD_INST(Tlv217Field, ParseBuf, pParentStream, nParentLength);
    VL_tlv217Field field;
    //VL_ZERO_MEMORY(field);
    memset(&field, 0 , sizeof(field));
    field.numBytes = nParentLength;
    vlParseBuffer(pParseBuf, field.acollectBytes, field.numBytes, sizeof(field.acollectBytes));
    VL_TLV_217_FIELD_DATA tlvFieldData(field.acollectBytes, field.acollectBytes+vlMin(vlAbs(field.numBytes), MAX_TLV_217_SIZE));
//auto_unlock(&vlg_TlvEventDblock);

    return  tlvFieldData;
}
string TlvConfig::vlGetCharParseBuff(VL_TLV_217_FIELD_DATA buffparse, int lenght)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    string valueofTag;
    int i = 0;
#define VL_DUMP_BUFFER_CHAR(ch)    ((((ch)>=32) && ((ch)<=127))?(ch):'.')

    for(i = 0; i < lenght; i++)
    {
        valueofTag.push_back(VL_DUMP_BUFFER_CHAR(buffparse[i]));
        //SNMP_DEBUGPRINT("VL dumpbuff hex-value %02x\n", buf[i]);
    }
//auto_unlock(&vlg_TlvEventDblock);

    return valueofTag;
}

VL_TLV_217_OID TlvConfig::vlGetAsnEncodedOidParseBuff(VL_TLV_217_FIELD_DATA buffparse, int nLength)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    VL_TLV_217_OID valueofTag;

    if(buffparse.size() > 0)
    {
        int i = 0;

        if(buffparse[0] >= 40)
        {
            valueofTag.push_back(buffparse[0]/40);
            valueofTag.push_back(buffparse[0]%40);
            i++;
        }

        while(i < buffparse.size())
        {
            int nOidNum = 0;
            do
            {
                nOidNum <<= 7;
                nOidNum |= (buffparse[i]&0x7F);
                i++;

            }while((i < nLength)&&(0x80 == (buffparse[i]&0x80)));

            valueofTag.push_back(nOidNum);
        }
    }
//auto_unlock(&vlg_TlvEventDblock);

    return valueofTag;
}

VL_TLV_217_OID TlvConfig::vlGetTlvOid(VL_BYTE_STREAM * pParentStream,  int nParentLength, int eTlvType)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    VL_BYTE_STREAM_PARENT_CHILD_INST(TlvOid, ParseBuf, pParentStream, nParentLength);

    unsigned long OIDsubtreeTag = vlParseByte(pParseBuf);
    unsigned long OIDlength = vlParseByte(pParseBuf);
    VL_TLV_217_OID decodedOID;

    if(OIDlength != VL_BYTE_STREAM_REMAINDER(pParseBuf))
    {
        // ERROR: illegal encoding
        EdocsDevEventhandling(ERRORCODE_I401_2, eTlvType);
    }
    else
    {
        if(OIDsubtreeTag == 6)
        {
            VL_TLV_217_FIELD_DATA encodedOID =  vlSnmpProcessTlv217Field(pParseBuf, vlMin(OIDlength, VL_BYTE_STREAM_REMAINDER(pParseBuf)));
            decodedOID = vlGetAsnEncodedOidParseBuff(encodedOID, encodedOID.size());

            if(decodedOID.size() > 1)
            {
                if((1 == decodedOID[0]) && (3 == decodedOID[1]))
                {
//auto_unlock(&vlg_TlvEventDblock);

                    return decodedOID;
                }
                else
                {
                    // ERROR: illegal encoding
                    EdocsDevEventhandling(ERRORCODE_I401_2, eTlvType);
                }
            }
        }
        else
        {
            // ERROR: illegal encoding
            SNMP_DEBUGPRINT("%s(): Line %d: Posting ERRORCODE_I401_2\n", __FUNCTION__, __LINE__);
            EdocsDevEventhandling(ERRORCODE_I401_2, eTlvType);
        }
    }
//auto_unlock(&vlg_TlvEventDblock);

    // return an empty vector
    return decodedOID;
}

#define ViewTypeMax 1
unsigned long TlvConfig::vlGetInt_typeParseBuff(VL_TLV_217_FIELD_DATA buffparse, int nLength)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    VL_BYTE_STREAM_INST(Tlv217Integer, ParseBuf, &(*(buffparse.begin())), buffparse.size());
    unsigned long nValue = vlParseNByteLong(pParseBuf, buffparse.size());;
    SNMP_DEBUGPRINT("vlGetInt_typeParseBuff() nBytes = %d, nLength = %d, nValue = %d\n", buffparse.size(), nLength, nValue);
//auto_unlock(&vlg_TlvEventDblock);

    return nValue;
}

int TlvConfig::vlSnmpParseTlv217Data(VL_BYTE_STREAM * pParentStream,  int nParentLength)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    VL_BYTE_STREAM_PARENT_CHILD_INST(Tlv217Data, ParseBuf, pParentStream, nParentLength);

    int offsetbuff = 0; // var for the buffer count
    int tlv217loopcount_53 = 0;
    SNMP_DEBUGPRINT("\nTlvConfig::vlSnmpParseTlv217Data :::::::: Start's with length:: %d \n", nParentLength);
    while(VL_BYTE_STREAM_REMAINDER(pParseBuf) > 0)
    {
        unsigned long ulTag = /*((nParentTag<<8) |*/ vlParseByte(pParseBuf);
        offsetbuff++;//vlParseByte() parse one byte, when ever vlParseByte come i will increament the offsetbuff
        unsigned long nLength = vlParseByte(pParseBuf);
        offsetbuff++;
        SNMP_DEBUGPRINT(" vlSnmpParseTlv217Data : received Tag = %d.%d.%d.%d, of Length %d Bytes\n",
                        ((ulTag>>24)&0xFF), ((ulTag>>16)&0xFF),
                          ((ulTag>>8 )&0xFF), ((ulTag>>0 )&0xFF),
                            nLength);

        switch( ulTag )
        {

            case TSnmpV1V2Coexistence:
            {  //tlv- 53
                // call helper function.
                /* tlv217loopcount_53 will give the number of items in the each 53-set*/

                int v1v2parselenght = 0;
                v1v2parselenght = vlSNMPv1v2cParse(pParseBuf, nLength);

                if( v1v2parselenght != nLength)
                {
                    // only a syntax error or memory allocation failure can cause
                    // parse failure for ParseSnmpV1V2Coexistence().
                    // Generate an Event , don't stop the parse we need to cover all the event's  I4047
                    //offsetbuff+=nLength;
                    SNMP_DEBUGPRINT("\n ERROR :: unable to parse full lenght vlSNMPv1v2cParse \n");

                }
                else
                {
                    //offsetbuff+=v1v2parselenght;
                }
                offsetbuff+=nLength;
            }
            break;

            case TSnmpV3AccessView://tlv-54
            {// call helper function.
                int v3AccessvparseLength = 0;
                v3AccessvparseLength = vlSNMPv3_AccessViewConfigParse(pParseBuf, nLength);
                if( v3AccessvparseLength != nLength )
                {
                    // only a syntax error or memory allocation failure can cause
                    // parse failure for ParseSnmpV3AccessView().
                    // Generate an Event , don't stop the parse we need to cover all the event's
                    //offsetbuff+=nLength;
                    SNMP_DEBUGPRINT("\n ERROR :: unable to parse full lenght vlSNMPv3_AccessViewConfigParse \n");
                }
                else
                {
                    //offsetbuff+=v3AccessvparseLength;
                }
                offsetbuff+=nLength;
            }
            break;
            case VLSnmpMibObject: //tlv - 11
            {
                int Mibobjparselenght = 0;
                Mibobjparselenght = vlSNMPmibObjectParse(pParseBuf, nLength);
                if( Mibobjparselenght != nLength)
                {
                    // only a syntax error or memory allocation failure can cause
                    // parse failure for ParseOctetBufferListItem().
                    // Generate an Event , don't stop the parse we need to cover all the event's
                    //offsetbuff+=nLength;
                    SNMP_DEBUGPRINT("\n ERROR :: unable to parse full lenght VLSnmpMibObject \n");
                }
                else
                {
                    //offsetbuff+=Mibobjparselenght;
                }
                offsetbuff+=nLength;
            }
            break;

            case TSnmpIpModeControl:
            {
                int ipModeControlLength = 0;
                ipModeControlLength = vlSNMPIpModeControlParse(pParseBuf, nLength);
                if( ipModeControlLength  != nLength)
                {
                    // only a syntax error or memory allocation failure can cause
                    // parse failure for ParseOctetBufferListItem().
                    // Generate an Event , don't stop the parse we need to cover all the event's
                    //offsetbuff+=nLength;
                    SNMP_DEBUGPRINT("\n ERROR :: unable to parse full lenght TSnmpIpModeControl \n");
                }
                else
                {
                    //offsetbuff+=ipModeControlLength;
                }
                offsetbuff+=nLength;
            }
            break;

            case VLVendorId : //8
            {
                VL_TLV_217_FIELD_DATA vendorId = vlSNMPVendorIdParse(pParseBuf, nLength);
            }
            break;
            case VLVendorSpecific : //43
            {
                int vendorpecficlenght = 0;
                vendorpecficlenght = vlSNMPVendorSpecificParse(pParseBuf, nLength);
                if( vendorpecficlenght  != nLength)
                {
                    // only a syntax error or memory allocation failure can cause
                    // parse failure for ParseOctetBufferListItem().
                    // Generate an Event , don't stop the parse we need to cover all the event's
                    //offsetbuff+=nLength;
                    SNMP_DEBUGPRINT("\n ERROR :: unable to parse full lenght VLVendorSpecific \n");
                }
                else
                {
                    //offsetbuff+=vendorpecficlenght;
                }
                offsetbuff+=nLength;
            }
            break;
            case VLSNMPv3NotificationReceiver : //38
            {
                int nVLSNMPv3NotificationReceiverLength = 0;
                nVLSNMPv3NotificationReceiverLength = vlSNMPv3_NotificationReceiver_Parse(pParseBuf, nLength);
                if( nVLSNMPv3NotificationReceiverLength  != nLength)
                {
                    // only a syntax error or memory allocation failure can cause
                    // parse failure for ParseOctetBufferListItem().
                    // Generate an Event , don't stop the parse we need to cover all the event's
                    //offsetbuff+=nLength;
                    SNMP_DEBUGPRINT("\n ERROR :: unable to parse full length VLSNMPv3NotificationReceiver \n");
                }
                else
                {
                    //offsetbuff+=nVLSNMPv3NotificationReceiverLength;
                }
                offsetbuff+=nLength;
            }
            break;
            default:
            {
                SNMP_DEBUGPRINT("\n vlSnmpParseTlv217Data:: I will move the buffer based on the lenght its has ::: received Invalid Tag = %d.%d.%d.%d, of Length %d Bytes\n",
                                ((ulTag>>24)&0xFF), ((ulTag>>16)&0xFF),
                                  ((ulTag>>8 )&0xFF), ((ulTag>>0 )&0xFF),
                                    nLength);
                vlSnmpProcessTlv217Field (pParseBuf, nLength);//for buffer pointer increment
                offsetbuff += nLength; //moving the buff len for unkonw tlv-fuond
                //tlvdefault_UIlist.push_back(ulTag);//tells about the number of default items
                EdocsDevEventhandling(ERRORCODE_I401_1, ulTag);
            }
            break;


        }//switch

    }//while check untill buff lenght is equal
    if(offsetbuff == nParentLength)
    {

        SNMP_DEBUGPRINT("\n parse completed collect the last item-lists of 53-list also \n");
    }
    /** print and check the events in that list*/
    checkEventlistInTlv217list();
    SNMP_DEBUGPRINT("\nTlvConfig::vlSnmpParseTlv217Data :::::::: End with length:: %d \n", offsetbuff);
//auto_unlock(&vlg_TlvEventDblock);

    return 1;
}

int TlvConfig::vlSNMPv3_NotificationReceiver_Parse(VL_BYTE_STREAM * pParentStream,int nParentLength)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    VL_BYTE_STREAM_PARENT_CHILD_INST(V3AccessConfig, ParseBuf, pParentStream, nParentLength);
    int offsetbuff = 0; // var for the buffer count
    SNMP_DEBUGPRINT("\n vlSNMPv3_NotificationReceiver_Parse :::::::: Start's with length:: %d \n", nParentLength);

    v3NotificationReceiver_t objV3NotificationReceiver;

    bool b_38_1_present = false;
    bool b_38_2_present = false;
    bool b_38_3_present = false;
    bool b_38_4_present = false;
    bool b_38_5_present = false;
    bool b_38_6_present = false;
    bool b_38_7_present = false;
    bool b_38_8_present = false;

    while(VL_BYTE_STREAM_REMAINDER(pParseBuf) > 0)
    {
        unsigned long ulTag = vlParseByte(pParseBuf);
        offsetbuff++;//vlParseByte() parse one byte, when ever vlParseByte come i will increament the offsetbuff
        unsigned long nLength = vlParseByte(pParseBuf);
        offsetbuff++;
        SNMP_DEBUGPRINT("\n vlSNMPv3_NotificationReceiver_Parse : received Tag = %d.%d.%d.%d, of Length %d Bytes\n",
                        ((ulTag>>24)&0xFF), ((ulTag>>16)&0xFF),
                          ((ulTag>>8 )&0xFF), ((ulTag>>0 )&0xFF),
                            nLength);
        switch( ulTag )
        {
            case VLSNMPv3NotificationReceiverIPv4Address :
            {
                VL_TLV_217_FIELD_DATA valbuf = vlSnmpProcessTlv217Field (pParseBuf, nLength);
                if(!b_38_1_present)
                {
                    objV3NotificationReceiver.v3ipV4address = valbuf;
                    b_38_1_present = true;
                }
            }
            break;

            case VLSNMPv3NotificationReceiverUDPPortNumber :
            {
                VL_TLV_217_FIELD_DATA valbuf = vlSnmpProcessTlv217Field (pParseBuf, nLength);
                if(!b_38_2_present)
                {
                    objV3NotificationReceiver.nV3UdpPortNumber = vlGetInt_typeParseBuff(valbuf, nLength);
                    b_38_2_present = true;
                }
            }
            break;

            case VLSNMPv3NotificationReceiverTrapType :
            {
                VL_TLV_217_FIELD_DATA valbuf = vlSnmpProcessTlv217Field (pParseBuf, nLength);
                int eV3TrapType = vlGetInt_typeParseBuff(valbuf, nLength);
                if(!b_38_3_present)
                {
                    objV3NotificationReceiver.eV3TrapType = eV3TrapType;
                    b_38_3_present = true;
                }
                else
                {
                    if(VL_TLV_217_NOTIFY_TRAP_V2C == eV3TrapType)
                    {
                        // always prefer TRAP_V2C
                        objV3NotificationReceiver.eV3TrapType = VL_TLV_217_NOTIFY_TRAP_V2C;
                    }
                }
                objV3NotificationReceiver.anV3TrapTypes.push_back(eV3TrapType);
            }
            break;

            case VLSNMPv3NotificationReceiverTimeout :
            {
                VL_TLV_217_FIELD_DATA valbuf = vlSnmpProcessTlv217Field (pParseBuf, nLength);
                if(!b_38_4_present)
                {
                    objV3NotificationReceiver.nV3Timeout = vlGetInt_typeParseBuff(valbuf, nLength);
                    b_38_4_present = true;
                }
            }
            break;

            case VLSNMPv3NotificationReceiverRetries :
            {
                VL_TLV_217_FIELD_DATA valbuf = vlSnmpProcessTlv217Field (pParseBuf, nLength);
                if(!b_38_5_present)
                {
                    objV3NotificationReceiver.nV3Retries = vlGetInt_typeParseBuff(valbuf, nLength);
                    b_38_5_present = true;
                }
            }
            break;

            case VLSNMPv3NotificationReceiverFilteringParameters :
            {
                VL_TLV_217_OID v3subtreels = vlGetTlvOid(pParseBuf, nLength, VLSNMPv3NotificationReceiver);
                if(!b_38_6_present)
                {
                    if(v3subtreels.size() > 1)
                    {
                        objV3NotificationReceiver.v3FilterOID = v3subtreels;
                        b_38_6_present = true;
                    }
                }
            }
            break;

            case VLSNMPv3NotificationReceiverSecurityName :
            {
                VL_TLV_217_FIELD_DATA valbuf = vlSnmpProcessTlv217Field (pParseBuf, nLength);
                if(!b_38_7_present)
                {
                    objV3NotificationReceiver.v3securityName = vlGetCharParseBuff(valbuf, nLength);
                    b_38_7_present = true;
                }
            }
            break;

            case VLSNMPv3NotificationReceiverIPv6Address :
            {
                VL_TLV_217_FIELD_DATA valbuf = vlSnmpProcessTlv217Field (pParseBuf, nLength);
                if(!b_38_8_present)
                {
                    objV3NotificationReceiver.v3ipV6address = valbuf;
                    b_38_8_present = true;
                }
            }
            break;

            default:
            {
                SNMP_DEBUGPRINT(" \n vlSNMPv3_NotificationReceiver_Parse : received Invalid Tag = %d.%d.%d.%d, of Length %d Bytes\n",
                                ((ulTag>>24)&0xFF), ((ulTag>>16)&0xFF),
                                  ((ulTag>>8 )&0xFF), ((ulTag>>0 )&0xFF),
                                    nLength);
                vlSnmpProcessTlv217Field (pParseBuf, nLength);//for buffer pointer increment
                EdocsDevEventhandling(ERRORCODE_I401_1, VLSNMPv3NotificationReceiver);
                offsetbuff += nLength;
            }
            break;
        }//switch
    }//while loop

    if(!b_38_2_present)
    {
        objV3NotificationReceiver.nV3UdpPortNumber = 162;
    }

    if(b_38_1_present || b_38_8_present)
    {
        int iloop = 0;
        bool bDuplicateFound = false;

        for(iloop= 0; iloop < v3NotificationReceiver_list.size(); iloop++)
        {//check for duplicates
            if( v3NotificationReceiver_list[iloop] == objV3NotificationReceiver)
            {
                SNMP_DEBUGPRINT(" Duplicate V3 Notification Receiver %d \n", iloop);
                EdocsDevEventhandling(ERRORCODE_I438_1);
                bDuplicateFound = true;
            }
        }

        if(!bDuplicateFound)
        {
            v3NotificationReceiver_list.push_back(objV3NotificationReceiver);
        }

        if(b_38_3_present)
        {
            bool bv2cTrapTypePresent = false;

            for(iloop = 0; iloop < objV3NotificationReceiver.anV3TrapTypes.size(); iloop++)
            {
                if(VL_TLV_217_NOTIFY_TRAP_V2C == objV3NotificationReceiver.anV3TrapTypes[iloop])
                {
                    bv2cTrapTypePresent = true;
                }
            }

            if(false == bv2cTrapTypePresent)
            {
                EdocsDevEventhandling(ERRORCODE_I438_3);
            }
        }
    }
    else
    {
        EdocsDevEventhandling(ERRORCODE_I438_2);
    }

    if(!b_38_3_present)
    {
        EdocsDevEventhandling(ERRORCODE_I438_3);
    }

    SNMP_DEBUGPRINT("\n vlSNMPv3_NotificationReceiver_Parse :::::::: END's with length:: %d \n", offsetbuff);
//auto_unlock(&vlg_TlvEventDblock);

    return nParentLength;
}

int TlvConfig::vlSNMPv1v2cParse(VL_BYTE_STREAM * pParentStream, int nParentLength)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    VL_BYTE_STREAM_PARENT_CHILD_INST(V1V2Config, ParseBuf, pParentStream, nParentLength);
    int offsetbuff = 0; // var for the buffer count
    SNMP_DEBUGPRINT("\n  vlSNMPv1v2cParse :::::::: Start's  with length:: %d \n", nParentLength);

    //53.3
    TLV53_V1V2_CONFIG objtvl217;

    bool b_53_1_present = false;
    bool b_53_2_present = false;
    bool b_53_3_present = false;
    bool b_53_4_present = false;

    while(VL_BYTE_STREAM_REMAINDER(pParseBuf) > 0)
    {
        unsigned long ulTag = (vlParseByte(pParseBuf));
        offsetbuff++;//vlParseByte() parse one byte, when ever vlParseByte come i will increament the offsetbuff
        unsigned long nLength = vlParseByte(pParseBuf);
        offsetbuff++;
        SNMP_DEBUGPRINT(" \n vlSNMPv1v2cParse : received Tag = %d.%d.%d.%d, of Length %d Bytes\n",
                        ((ulTag>>24)&0xFF), ((ulTag>>16)&0xFF),
                          ((ulTag>>8 )&0xFF), ((ulTag>>0 )&0xFF),
                            nLength);

        switch( ulTag )
        {

            case TSnmpV1V2CommunityName : //53 -> 1
            {
                string strCommunity;
                //collect the communitystring;
                VL_TLV_217_FIELD_DATA communitystr = vlSnmpProcessTlv217Field(pParseBuf, nLength);
                strCommunity = vlGetCharParseBuff(communitystr, nLength);
                SNMP_DEBUGPRINT("\n vlSNMPv1v2cParse: string after passing into CommunityList:: %s lenght-by-strlen is:: %d\n", strCommunity.c_str(), strCommunity.size());
                if(!b_53_1_present)
                {
                    objtvl217.strCommunity = strCommunity;
                    b_53_1_present = true;
                }
                else
                {
                    EdocsDevEventhandling(ERRORCODE_I453_3);
                }

                offsetbuff += nLength;
            }
            break;
            case TSnmpV1V2Transport: //53-> 2
            {
                VL_TLV_TRANSPORT_ADDR_AND_MASK v12Transport = vlSNMPv1v2cTransportAddressAccessParse(pParseBuf, nLength);

                if(v12Transport.TpAddress.size() > 0)
                {
                    v12Transport.iAccessViewTlv = Tlv217loopList.size();
                    v12Transport.iTlvSubEntry   = objtvl217.AVTransportAddressList.size();
                    objtvl217.AVTransportAddressList.push_back(v12Transport);
                    b_53_2_present = true;
                }
            }
            break;
            case TSnmpV1V2ViewType ://53 -> 3
            {
                int eAccessViewType = 0;
                //offsetbuff increase after collection the valuen  increment the pParseBuf
                VL_TLV_217_FIELD_DATA AccessViewType = vlSnmpProcessTlv217Field (pParseBuf, nLength);
                if(!b_53_3_present)
                {
                    eAccessViewType = vlGetInt_typeParseBuff(AccessViewType, nLength);
                    SNMP_DEBUGPRINT("\n vlSNMPv1v2cParse ::string before passing into eAccessViewType:: %d \n", eAccessViewType);
                    switch(eAccessViewType)
                    {
                        case 1: // read-only
                        case 2: // read-write
                        {
                            objtvl217.eAccessViewType = eAccessViewType;
                            b_53_3_present = true;
                        }
                        break;

                        default:
                        {
                            SNMP_DEBUGPRINT("%s(): Line %d: Posting ERRORCODE_I453_4\n", __FUNCTION__, __LINE__);
                            EdocsDevEventhandling(ERRORCODE_I453_4);
                            // reject the entire TLV
                            return nParentLength;
                        }
                        break;
                    }
                }
                else
                {
                    EdocsDevEventhandling(ERRORCODE_I453_3);
                }
            }
            break;
            case TSnmpV1V2ViewName ://53 -> 4
            {
                string strAccessViewName;
                VL_TLV_217_FIELD_DATA AccessViewName = vlSnmpProcessTlv217Field (pParseBuf, nLength);
                strAccessViewName = vlGetCharParseBuff(AccessViewName, nLength);
                SNMP_DEBUGPRINT("\nvlSNMPv1v2cParse: string  passing into AccessViewNameList:: %s \n", strAccessViewName.c_str());
                if(!b_53_4_present)
                {
                    objtvl217.strAccessViewName = strAccessViewName;
                    b_53_4_present = true;
                }
                else
                {
                    EdocsDevEventhandling(ERRORCODE_I453_3);
                }
                offsetbuff += nLength;
            }
            break;
            default:
            {
                //increment the buff and repete the parse for inviled parse
                vlSnmpProcessTlv217Field (pParseBuf, nLength);//for buffer pointer increment
                offsetbuff += nLength;
                //tlvdefault_UIlist.push_back(ulTag);//tells about the number of default items
                EdocsDevEventhandling(ERRORCODE_I401_1, TSnmpV1V2Coexistence);
            }
            break;
        }

    }

    if(!b_53_1_present)
    {
        EdocsDevEventhandling(ERRORCODE_I453_1);
    }

    if(!b_53_2_present)
    {
        EdocsDevEventhandling(ERRORCODE_I453_2);
    }

    if(!b_53_3_present)
    {
        objtvl217.eAccessViewType = VL_TLV_217_ACCESS_READ_ONLY;
    }

    if(b_53_1_present && b_53_2_present)
    {
        /** Collect the list of sub-listtvl items into the global tlv217loopconut-53list and then we can generate the events by using those lists */
        SNMP_DEBUGPRINT("\nvlSNMPv1v2cParse: Number of list came depends on lopp tlv217loopcount_53 \n");
        /**collect the all list into the main tvl217looplist*/
        bool bDuplicateFound = false;

        for(int iloop= 0; iloop < Tlv217loopList.size(); iloop++)
        {//check for duplicates

            if( Tlv217loopList[iloop] == objtvl217)
            {
                SNMP_DEBUGPRINT("vlSNMPv1v2cParse: Duplicate V1V2 Access Config %d \n", iloop);
                EdocsDevEventhandling(ERRORCODE_I453_6);
                bDuplicateFound = true;
            }
        }

        if(!bDuplicateFound)
        {
            SNMP_DEBUGPRINT("vlSNMPv1v2cParse: Adding %d Transports\n", objtvl217.AVTransportAddressList.size());
            Tlv217loopList.push_back(objtvl217);
        }
    }

    SNMP_DEBUGPRINT("\n  vlSNMPv1v2cParse :::::::: End's  with length:: %d \n", offsetbuff);
//auto_unlock(&vlg_TlvEventDblock);

    return nParentLength;
}

VL_TLV_TRANSPORT_ADDR_AND_MASK TlvConfig::vlSNMPv1v2cTransportAddressAccessParse(VL_BYTE_STREAM * pParentStream, int nParentLength)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    VL_BYTE_STREAM_PARENT_CHILD_INST(V1V2TransportAddress, ParseBuf, pParentStream, nParentLength);
    int offsetbuff = 0; // var for the buffer count
    SNMP_DEBUGPRINT("\nvlSNMPv1v2cTransportAddressAccessParse :::::::: Start' with length:: %d \n", nParentLength);
    VL_TLV_TRANSPORT_ADDR_AND_MASK TranpsortAddandMask;

    bool b_53_2_1_present = false;
    bool b_53_2_2_present = false;

    while(VL_BYTE_STREAM_REMAINDER(pParseBuf) > 0)
    {
        unsigned long ulTag = /*((nParentTag<<8) |*/ vlParseByte(pParseBuf);
        offsetbuff++;//vlParseByte() parse one byte, when ever vlParseByte come i will increament the offsetbuff
        unsigned long nLength = vlParseByte(pParseBuf);
        offsetbuff++;
        SNMP_DEBUGPRINT(" \n vlSNMPv1v2cTransportAddressAccessParse: received Tag = %d.%d.%d.%d, of Length %d Bytes\n",
                        ((ulTag>>24)&0xFF), ((ulTag>>16)&0xFF),
                          ((ulTag>>8 )&0xFF), ((ulTag>>0 )&0xFF),
                            nLength);

        switch( ulTag )
        {
            case TSnmpV1V2TransportAddress :
            {
                //collecting the communitystring
                VL_TLV_217_FIELD_DATA TransportAddress = vlSnmpProcessTlv217Field (pParseBuf, nLength);
                if( nLength  == 6 )//it should be 6bytes if more than 6 generate Error event
                {
                    /* at present only IP is taking , port not considering , after real Table exits then put the PORT also*/
                }
                else
                {
                    //generate an error /or the list element will be zero there we are generateing an error if no element is not tere in the list
                }
                if(!b_53_2_1_present)
                {
                    TranpsortAddandMask.TpAddress = TransportAddress;

                    if(!b_53_2_2_present)
                    {
                        // generate default mask
                        for(int i = 0; i < TransportAddress.size(); i++)
                        {
                            TranpsortAddandMask.TpMask.push_back((TransportAddress[i]&0xFF)?255:0);
                        }
                    }
                    b_53_2_1_present = true;
                }
                else
                {
                    EdocsDevEventhandling(ERRORCODE_I453_3);
                }
                offsetbuff += nLength;
            }
            break;
            case TSnmpV1V2TransportAddressMask :
            {
                //collect the TransportAddressMask
                /* at present only IP is taking , port not considering , after real Table exits then put the PORT also*/
                VL_TLV_217_FIELD_DATA TportAddmask =  vlSnmpProcessTlv217Field (pParseBuf, nLength);
                if( nLength  == 6 )//it should be 6bytes if more than 6 generate Error event
                {
                }
                else
                {
                    //generate an error /or the list element will be zero there we are generateing an error if no element is not tere in the list
                }
                //SNMP_DEBUGPRINT("\n string before passing into TSnmpV1V2TransportAddressMask:: %s \n", szTemp);
                if(!b_53_2_2_present)
                {
                    TranpsortAddandMask.TpMask = TportAddmask;
                    b_53_2_2_present = true;
                }
                else
                {
                    EdocsDevEventhandling(ERRORCODE_I453_3);
                }
                offsetbuff += nLength;
            }
            break;
            default:
            {
                SNMP_DEBUGPRINT("\n  Defaultcondition:: vlSNMPv1v2cTransportAddressAccessParse : received Invalid Tag = %d.%d.%d.%d, of Length %d Bytes\n",
                                ((ulTag>>24)&0xFF), ((ulTag>>16)&0xFF),
                                  ((ulTag>>8 )&0xFF), ((ulTag>>0 )&0xFF),
                                    nLength);
                vlSnmpProcessTlv217Field (pParseBuf, nLength);//for buffer pointer increment
                //tlvdefault_UIlist.push_back(ulTag);//tells about the number of default items
                EdocsDevEventhandling(ERRORCODE_I401_1, TSnmpV1V2Coexistence);
                offsetbuff += nLength;
            }
            break;

        }//switch

    }//while

    if(b_53_2_1_present)
    {
        SNMP_DEBUGPRINT("vlSNMPv1v2cTransportAddressAccessParse() Adding Address and Mask\n");
        return TranpsortAddandMask;
    }
    else
    {
        EdocsDevEventhandling(ERRORCODE_I453_2);
    }

    SNMP_DEBUGPRINT("\nvlSNMPv1v2cTransportAddressAccessParse :::::::: Ends with length:: %d \n", offsetbuff);

    // return an empty transport
//auto_unlock(&vlg_TlvEventDblock);

    return VL_TLV_TRANSPORT_ADDR_AND_MASK();
}

int TlvConfig::vlSNMPv3_AccessViewConfigParse(VL_BYTE_STREAM * pParentStream,int nParentLength)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    VL_BYTE_STREAM_PARENT_CHILD_INST(V3AccessConfig, ParseBuf, pParentStream, nParentLength);
    int offsetbuff = 0; // var for the buffer count
    SNMP_DEBUGPRINT("\n vlSNMPv3_AccessViewConfigParse :::::::: Start's with length:: %d \n", nParentLength);

    v3AccessviewList_t objV3accesitems;

    bool b_54_1_present = false;
    bool b_54_2_present = false;
    bool b_54_3_present = false;
    bool b_54_4_present = false;

    while(VL_BYTE_STREAM_REMAINDER(pParseBuf) > 0)
    {
        unsigned long ulTag = vlParseByte(pParseBuf);
        offsetbuff++;//vlParseByte() parse one byte, when ever vlParseByte come i will increament the offsetbuff
        unsigned long nLength = vlParseByte(pParseBuf);
        offsetbuff++;
        SNMP_DEBUGPRINT("\n vlSNMPv3_AccessViewConfigParse : received Tag = %d.%d.%d.%d, of Length %d Bytes\n",
                        ((ulTag>>24)&0xFF), ((ulTag>>16)&0xFF),
                          ((ulTag>>8 )&0xFF), ((ulTag>>0 )&0xFF),
                            nLength);

        switch( ulTag )
        {
            case TSnmpV3AccessViewName :
            {

                string v3accviewname;
                //collect the TransportAddressMask
                VL_TLV_217_FIELD_DATA v3AccessViewName = vlSnmpProcessTlv217Field (pParseBuf, nLength);

                if(!b_54_1_present)
                {
                    v3accviewname = vlGetCharParseBuff(v3AccessViewName, nLength);
                    SNMP_DEBUGPRINT("\n string before passing into v3AccessViewNameList:: %s \n", v3accviewname.c_str());
                    objV3accesitems.strAccessViewName = v3accviewname;

                    b_54_1_present = true;
                }
                else
                {
                    EdocsDevEventhandling(ERRORCODE_I454_2);
                }
            }
            break;
            case TSnmpV3AccessViewSubtree :
            {
                VL_TLV_217_OID v3subtreels = vlGetTlvOid(pParseBuf, nLength, TSnmpV3AccessView);

                if(!b_54_2_present)
                {
                    if(v3subtreels.size() > 1)
                    {
                        objV3accesitems.v3AccessViewSubtree = v3subtreels;
                        b_54_2_present = true;
                    }
                    else
                    {
                        // reject the entire TLV
                        return nParentLength;
                    }
                }
                else
                {
                    EdocsDevEventhandling(ERRORCODE_I454_2);
                }
            }
            break;
            case TSnmpV3AccessViewMask :
            {
                //collect the TransportAddressMask
                VL_TLV_217_FIELD_DATA v3AccessViewMask = vlSnmpProcessTlv217Field (pParseBuf, nLength);

                if(!b_54_3_present)
                {
                    char v3AViewMask[API_CHRMAX+16];
                    VL_TLV_217_OID v3subtreels;
                    for(int subtrei = 0; (subtrei < nLength) && (subtrei < (API_CHRMAX)); subtrei++)
                    {
                        SNMP_DEBUGPRINT("v3AccessViewMask Byte value is::%02x\n", v3AccessViewMask[subtrei]);
                        snprintf(v3AViewMask+subtrei, sizeof(v3AViewMask), "%d.", v3AccessViewMask[subtrei]);
                        v3subtreels.push_back(v3AccessViewMask[subtrei]&0x00FF);
                    }
                    objV3accesitems.v3AccessViewMask = v3subtreels;
                    b_54_3_present = true;
                    // SNMP_DEBUGPRINT("\n Check the list string before passing into v3AccessViewMaskList:: %s \n", v3AViewMask);
                    SNMP_DEBUGPRINT("\n Check the list string before passing into v3AccessViewMaskList:: %s \n", v3AViewMask);
                    offsetbuff += nLength;
                }
                else
                {
                    EdocsDevEventhandling(ERRORCODE_I454_2);
                }
            }
            break;
            case TSnmpV3AccessViewType :
            {
                int AccVType = 0;
                //offsetbuff increase after collection the valuen  increment the pParseBuf
                VL_TLV_217_FIELD_DATA AccessViewType = vlSnmpProcessTlv217Field (pParseBuf, nLength);

                if(!b_54_4_present)
                {
                    AccVType = vlGetInt_typeParseBuff(AccessViewType, nLength);
                    SNMP_DEBUGPRINT("\nTSnmpV1V2ViewType ::string before passing into AccVType:: %d \n", AccVType);
                    switch(AccVType)
                    {
                        case 1: // included
                        case 2: // excluded
                        {
                            objV3accesitems.v3AccessViewType = AccVType;
                            b_54_4_present = true;
                        }
                        break;

                        default:
                        {
                            SNMP_DEBUGPRINT("%s(): Line %d: Posting ERRORCODE_I454_3\n", __FUNCTION__, __LINE__);
                            EdocsDevEventhandling(ERRORCODE_I454_3);
                            // reject the entire TLV
                            return nParentLength;
                        }
                        break;
                    }
                }
                else
                {
                    EdocsDevEventhandling(ERRORCODE_I454_2);
                }
            }
            break;
            default:
            {
                SNMP_DEBUGPRINT(" \n vlSNMPv3_AccessViewConfigParse : received Invalid Tag = %d.%d.%d.%d, of Length %d Bytes\n",
                                ((ulTag>>24)&0xFF), ((ulTag>>16)&0xFF),
                                  ((ulTag>>8 )&0xFF), ((ulTag>>0 )&0xFF),
                                    nLength);
                vlSnmpProcessTlv217Field (pParseBuf, nLength);//for buffer pointer increment
                //tlvdefault_UIlist.push_back(ulTag);//tells about the number of default items
                EdocsDevEventhandling(ERRORCODE_I401_1, TSnmpV3AccessView);
                offsetbuff += nLength;
            }
            break;
        }//switch
    }//while loop

    if(!b_54_1_present)
    {
        EdocsDevEventhandling(ERRORCODE_I454_1);
    }

    if(!b_54_2_present)
    {
        VL_TLV_217_OID v3subtreels;
        v3subtreels.clear();
        // 1.3.6 if this was not specified
        v3subtreels.push_back(1);
        v3subtreels.push_back(3);
        v3subtreels.push_back(6);
        objV3accesitems.v3AccessViewSubtree = v3subtreels;
    }

    if(!b_54_3_present)
    {
        VL_TLV_217_OID v3subtreels;
        v3subtreels.clear();
        objV3accesitems.v3AccessViewMask = v3subtreels;
    }

    if(!b_54_4_present)
    {
        objV3accesitems.v3AccessViewType = VL_TLV_217_ACCESS_INCLUDED;
    }

    if(b_54_1_present)
    {
        bool bDuplicateFound = false;

        for(int iloop= 0; iloop < V3AccviewLs.size(); iloop++)
        {//check for duplicates

            if( V3AccviewLs[iloop] == objV3accesitems)
            {
                SNMP_DEBUGPRINT(" Duplicate V3 Access Config %d \n", iloop);
                EdocsDevEventhandling(ERRORCODE_I454_5);
                bDuplicateFound = true;
            }
        }

        if(!bDuplicateFound)
        {
            V3AccviewLs.push_back(objV3accesitems);
        }
    }

    SNMP_DEBUGPRINT("\n vlSNMPv3_AccessViewConfigParse :::::::: END's with length:: %d \n", offsetbuff);
//auto_unlock(&vlg_TlvEventDblock);

    return nParentLength;
}

void TlvConfig::processTlv217SnmpSetRequest(char * szOidStr, int eVarBindType, unsigned char * mibOIDvalue, int nVarBindLength)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    unsigned long lValue = 0;

    switch(eVarBindType)
    {
        case ASN_BOOLEAN:
        case ASN_INTEGER:
        case ASN_BIT_STR:
        case ASN_OCTET_STR:
        case ASN_OBJECT_ID:
        case ASN_SEQUENCE:
        case ASN_SET:
        {
            VL_SNMP_API_RESULT eGetResult = vlSnmpGetLocalLong(szOidStr, &lValue);
            // Jun-15-2009: Resolved comm. problem between master & sub agent processes by using tcp and larger timeouts // cSleep(100);

            switch(eGetResult)
            {
                // node is valid
                case VL_SNMP_API_RESULT_SUCCESS:
                {
                    VL_SNMP_API_RESULT eSetResult = vlSnmpSetLocalAsnVar(szOidStr, eVarBindType, mibOIDvalue, nVarBindLength, VL_SNMP_CLIENT_REQUEST_TYPE_SYNCHRONOUS);
                    // Jun-15-2009: Resolved comm. problem between master & sub agent processes by using tcp and larger timeouts // cSleep(100);
                    switch(eSetResult)
                    {
                        // non-existing
                        case VL_SNMP_API_RESULT_ERR_NOSUCHOBJECT:
                        case VL_SNMP_API_RESULT_ERR_NOSUCHINSTANCE:
                        case VL_SNMP_API_RESULT_ERR_ENDOFMIBVIEW:
                        case VL_SNMP_API_RESULT_INVALID_OID:
                        {
                            EdocsDevEventhandling(ERRORCODE_I411_1);
                        }
                        break;

                        // illegal set
                        case VL_SNMP_API_RESULT_ERR_READONLY:
                        case VL_SNMP_API_RESULT_ERR_NOACCESS:
                        case VL_SNMP_API_RESULT_ERR_NOTWRITABLE:
                        case VL_SNMP_API_RESULT_ERR_NOCREATION:
                        case VL_SNMP_API_RESULT_PROTOCOL_ERROR:
                        {
                            EdocsDevEventhandling(ERRORCODE_I411_3);
                        }
                        break;

                        // bad varbind
                        case VL_SNMP_API_RESULT_ERR_TOOBIG:
                        case VL_SNMP_API_RESULT_ERR_BADVALUE:
                        case VL_SNMP_API_RESULT_ERR_WRONGTYPE:
                        case VL_SNMP_API_RESULT_ERR_WRONGLENGTH:
                        case VL_SNMP_API_RESULT_ERR_WRONGENCODING:
                        case VL_SNMP_API_RESULT_ERR_WRONGVALUE:
                        case VL_SNMP_API_RESULT_ERR_INCONSISTENTVALUE:
                        {
                            EdocsDevEventhandling(ERRORCODE_I411_3);
                        }
                        break;

                        // what happened ?
                        default:
                        {
                            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "%s : vlSnmpSetLocalAsnVar() returned %d : %s\n", __FUNCTION__, eSetResult, vlSnmpGetErrorString(eSetResult));
                        }
                        break;
                    } // switch eSetResult
                } // case eGetResult == VL_SNMP_API_RESULT_SUCCESS
                break;

                // node is invalid
                case VL_SNMP_API_RESULT_ERR_NOSUCHOBJECT:
                case VL_SNMP_API_RESULT_ERR_NOSUCHINSTANCE:
                case VL_SNMP_API_RESULT_ERR_ENDOFMIBVIEW:
                case VL_SNMP_API_RESULT_INVALID_OID:
                case VL_SNMP_API_RESULT_GET_ON_NODE:
                case VL_SNMP_API_RESULT_PROTOCOL_ERROR:
                {
                    EdocsDevEventhandling(ERRORCODE_I411_1);
                }
                break;

                // what happened ?
                default:
                {
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "%s : vlSnmpGetLocalLong() returned %d : %s\n", __FUNCTION__, eGetResult, vlSnmpGetErrorString(eGetResult));
                }
                break;
            } // switch eGetResult
        } // case supported varbinds
        break;

        // whats this varbind
        default:
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "Got unsupported varbind 0x%08X\n", eVarBindType);
            EdocsDevEventhandling(ERRORCODE_I411_3);
        }
        break;
    } // switch eVarBindType
//auto_unlock(&vlg_TlvEventDblock);

}

VL_TLV_SNMP_VAR_BIND TlvConfig::vlGetSnmpVarBind(VL_BYTE_STREAM * pParentStream,  int nParentLength)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    VL_BYTE_STREAM_PARENT_CHILD_INST(SnmpVarValue, ParseBuf, pParentStream, nParentLength);

    VL_TLV_SNMP_VAR_BIND snmpVarBind;

    int ulTag = vlParseByte(pParseBuf);
    int nLength = vlParseByte(pParseBuf);

    SNMP_DEBUGPRINT("\n vlGetSnmpVarBind : received Tag = %d, of Length %d Bytes\n",
                    ulTag, nLength);

    if(ulTag == TLV11_OID)//06-tag
    {
        VL_TLV_217_FIELD_DATA encodedOID = vlSnmpProcessTlv217Field(pParseBuf, nLength);
        VL_TLV_217_OID decodedOID = vlGetAsnEncodedOidParseBuff(encodedOID, encodedOID.size());

        int eVarBindType = vlParseByte(pParseBuf);
        int nVarBindLength = vlParseByte(pParseBuf);

        SNMP_DEBUGPRINT("\n vlGetSnmpVarBind : received VarType = 0x%02X, of Length %d Bytes\n",
                        eVarBindType, nLength);

        //Apr-30-2009: checking of ASN type done in processTlv217SnmpSetRequest()
        {
            VL_TLV_217_FIELD_DATA mibOIDvalue = vlSnmpProcessTlv217Field (pParseBuf, nVarBindLength);

            /* Collect the list which is having good OID and value */
            snmpVarBind.eVarBindType = eVarBindType;
            snmpVarBind.OIDmib       = decodedOID;
            snmpVarBind.OIDvalue     = mibOIDvalue;
        }
    }
    else
    {
        SNMP_DEBUGPRINT("\n ERROR ::NO oid 06-tag - oid TLV11_OID::::::: \n");//,ERRORCODE_I4010);
        EdocsDevEventhandling(ERRORCODE_I401_2, VLSnmpMibObject);
        /*TO increment the buff and lenght*/
        vlSnmpProcessTlv217Field (pParseBuf, nLength);
    }
//auto_unlock(&vlg_TlvEventDblock);

    return snmpVarBind;
}

int TlvConfig::vlSNMPmibObjectParse(VL_BYTE_STREAM * pParentStream, int nParentLength)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    VL_BYTE_STREAM_PARENT_CHILD_INST(MibObject, ParseBuf, pParentStream, nParentLength);

    int offsetbuff =0;
    unsigned long ulTag = vlParseByte(pParseBuf);
    offsetbuff++;//vlParseByte() parse one byte, when ever vlParseByte come i will increament the offsetbuff
    unsigned long nTlvLength = vlParseByte(pParseBuf);
    offsetbuff++;
    SNMP_DEBUGPRINT("\n vlSNMPmibObjectParse : received Tag = %d.%d.%d.%d, of Length %d Bytes\n",
                    ((ulTag>>24)&0xFF), ((ulTag>>16)&0xFF),
                      ((ulTag>>8 )&0xFF), ((ulTag>>0 )&0xFF),
                        nTlvLength);

    switch( ulTag )
    {
        case TLV11OID_HEADER://0x30 ---48
        {
            VL_TLV_SNMP_VAR_BIND snmpVarBind = vlGetSnmpVarBind(pParseBuf, nTlvLength);

            if(snmpVarBind.OIDmib.size() > 0)
            {
                char szOidStr[API_CHRMAX];
                vlSnmpConvertOidToString(&(*(snmpVarBind.OIDmib.begin())), snmpVarBind.OIDmib.size(), szOidStr, sizeof(szOidStr));
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "===============================================================================\n");
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "OID = %s, vartype = 0x%02X, value = \n", szOidStr, ulTag);
				IPC_CLIENT_vlMpeosDumpBuffer(RDK_LOG_DEBUG, "LOG.RDK.SNMP", &(*(snmpVarBind.OIDvalue.begin())), snmpVarBind.OIDvalue.size());

                bool bDuplicateFound = false;

                for(int iloop= 0; iloop < SNMPmibObjectList.size(); iloop++)
                {//check for duplicates
                    if(SNMPmibObjectList[iloop].OIDmib == snmpVarBind.OIDmib)
                    {
                        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "Duplicate SNMP MIB object assignment %d \n", iloop);
                        EdocsDevEventhandling(ERRORCODE_I411_2);
                        bDuplicateFound = true;
                    }
                }

                if(false == bDuplicateFound)
                {
                    SNMPmibObjectList.push_back(snmpVarBind);
                    processTlv217SnmpSetRequest(szOidStr, snmpVarBind.eVarBindType, &(*(snmpVarBind.OIDvalue.begin())), snmpVarBind.OIDvalue.size());
                } // if duplicate not found
            }
        }break;

        default:
        {

            SNMP_DEBUGPRINT("\n ERROR :: invalid header TLV11OID_HEADER::::::::\n");//,ERRORCODE_I4010);
            EdocsDevEventhandling(ERRORCODE_I401_2, VLSnmpMibObject);
            /*TO increment the buff and lenght*/
            vlSnmpProcessTlv217Field(pParseBuf, nTlvLength);
        }break;
    }//switch

    SNMP_DEBUGPRINT(" vlSNMPmibObjectParse of N bytes == %d ",offsetbuff );
    //offsetbuff +=nParentLength;
//auto_unlock(&vlg_TlvEventDblock);

    return nParentLength;
}

int TlvConfig::vlSNMPIpModeControlParse(VL_BYTE_STREAM * pParentStream, int nParentLength)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    VL_BYTE_STREAM_PARENT_CHILD_INST(IpMode, ParseBuf, pParentStream, nParentLength);
    int offsetbuff =0;
    // 1 byte of data
    //1byte of ID
    VL_TLV_217_FIELD_DATA IpModeControlvlaue = vlSnmpProcessTlv217Field (pParseBuf, nParentLength);
    for(int i = 0; i < nParentLength; i++)
    {
        SNMP_INFOPRINT("IpModeControlvlaue Byte value = %02x", IpModeControlvlaue[i]);
    }
#define IPMODECONTROL_LENGTH 1
    if(nParentLength  ==  IPMODECONTROL_LENGTH)
    {
        SNMP_INFOPRINT(" vlSNMPIpModeControlParse of N bytes  = %d\n", nParentLength);
        if(0 == IpModeControlList.size())
        {
            switch(IpModeControlvlaue[0])
            {
                case 0:
                case 1:
                {
                    IpModeControlList.push_back(IpModeControlvlaue[0]);

                }
                break;

                default:
                {
                    //  log tlv error event
                }
                break;
            }
        }
        else
        {
            // ignore the repeat config and log an error
            EdocsDevEventhandling(ERRORCODE_I401_11);
        }
    }
    else
    {
        SNMP_DEBUGPRINT("ERROR :: Ip Mode Control is more than 3 bytes :: error event ");
        SNMP_DEBUGPRINT("%s(): Line %d: Posting ERRORCODE_I401_2\n", __FUNCTION__, __LINE__);
        EdocsDevEventhandling(ERRORCODE_I401_2, TSnmpIpModeControl);
    }
    offsetbuff +=nParentLength;
//auto_unlock(&vlg_TlvEventDblock);

    return nParentLength;
}

VL_TLV_217_FIELD_DATA TlvConfig::vlSNMPVendorIdParse(VL_BYTE_STREAM * pParentStream, int nParentLength)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    VL_BYTE_STREAM_PARENT_CHILD_INST(VendorId, ParseBuf, pParentStream, nParentLength);
    char VIdvlaue[API_CHRMAX];
    int offsetbuff =0;
    // 3 byte of data
    //3byte of ID
    VL_TLV_217_FIELD_DATA VendorIdvlaue = vlSnmpProcessTlv217Field (pParseBuf, nParentLength);
    for(int i = 0; i < nParentLength; i++)
    {
        SNMP_DEBUGPRINT("VendorIdvlaue Byte value is::%02x\n", VendorIdvlaue[i]);
    }
#define VENDORID_LENGTH 3
    if(nParentLength  ==  VENDORID_LENGTH)
    {
        snprintf(VIdvlaue, sizeof(VIdvlaue), "%02X%02X%02X",VendorIdvlaue[0],VendorIdvlaue[1],VendorIdvlaue[3]);//only three bytes
        SNMP_DEBUGPRINT("\n string before passing into v3AccessViewMaskList:: %s \n", VIdvlaue);
        SNMP_DEBUGPRINT(" vlSNMPVendorIdParse of N bytes  = %d", nParentLength);
        VendorIdList.push_back(VendorIdvlaue);
    }
    else
    {
        SNMP_DEBUGPRINT("%s(): Line %d: Posting ERRORCODE_I401_2\n", __FUNCTION__, __LINE__);
        EdocsDevEventhandling(ERRORCODE_I401_2, VLVendorId);
        SNMP_DEBUGPRINT("ERROR :: vendor id is more than 3 bytes :: error event ");
    }
//auto_unlock(&vlg_TlvEventDblock);

    return VendorIdvlaue;
}
int TlvConfig::vlSNMPVendorSpecificParse(VL_BYTE_STREAM * pParentStream, int nParentLength)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    VL_BYTE_STREAM_PARENT_CHILD_INST(VendorSpecific, ParseBuf, pParentStream, nParentLength);
    VL_TLV_217_FIELD_DATA vendorId;

    int iTag = 0;

    while(VL_BYTE_STREAM_REMAINDER(pParseBuf) > 0)
    {
        unsigned long ulTag = vlParseByte(pParseBuf);
        unsigned long nLength = vlParseByte(pParseBuf);
        SNMP_DEBUGPRINT("\n vlSNMPVendorSpecificParse : received Tag = %d.%d.%d.%d, of Length %d Bytes\n",
                        ((ulTag>>24)&0xFF), ((ulTag>>16)&0xFF),
                          ((ulTag>>8 )&0xFF), ((ulTag>>0 )&0xFF),
                            nLength);

        iTag++;

        switch( ulTag )
        {
            case VLVendorId:
            {
                vendorId = vlSNMPVendorIdParse(pParseBuf, nParentLength);
            }
            break;

            default:
            {
                if(1 == iTag)
                {
                    // vendor id should be the first sub-tlv in vendor-specific tlv
                    EdocsDevEventhandling(ERRORCODE_I443_1);
                }

                SNMP_DEBUGPRINT(" \n vlSNMPVendorSpecificParse : received Invalid Tag = %d.%d.%d.%d, of Length %d Bytes\n",
                                ((ulTag>>24)&0xFF), ((ulTag>>16)&0xFF),
                                  ((ulTag>>8 )&0xFF), ((ulTag>>0 )&0xFF),
                                    nLength);
                vlSnmpProcessTlv217Field (pParseBuf, nLength);//for buffer pointer increment
                EdocsDevEventhandling(ERRORCODE_I401_1, VLVendorSpecific);
            }
            break;
        }//switch
    }//while loop
//auto_unlock(&vlg_TlvEventDblock);

    return nParentLength;
}

void TlvConfig::Generate_Events_V1V2Community()
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    // Apr-17-2009: deprecated code
//auto_unlock(&vlg_TlvEventDblock);

}
void TlvConfig::Generate_Events_V1V2TransportAddress_MaxAdd()
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    // Apr-17-2009: deprecated code
//auto_unlock(&vlg_TlvEventDblock);

}

/**Doces Error Events for V1V2Type-NameAccess value:: This Generate an Errors Events if the TLV-217 has in-valied TLV-v1v2TypeNameAcess value or duplicates  */
void TlvConfig::Generate_Events_V1V2TypeNameAccess()
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    // Apr-17-2009: deprecated code
//auto_unlock(&vlg_TlvEventDblock);

}
/** This is to collect the 53.4 value , v1v2 access name --- */
void TlvConfig::Generate_Events_V1V2NameAccess()
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    // Apr-17-2009: deprecated code
//auto_unlock(&vlg_TlvEventDblock);

}

void TlvConfig::Generate_Events_V3AVMaskTypeEvent()
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    // Apr-17-2009: deprecated code
//auto_unlock(&vlg_TlvEventDblock);

}
void TlvConfig::Generate_Events_Genral()
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    // Apr-17-2009: deprecated code
//auto_unlock(&vlg_TlvEventDblock);

}

/**
 *  @param OIDset  -    In - pass a OID to set the snmp-Agent
 *  @param oidValue  -  In - pass a OID Value to set
 *  @param valueType  - In - pass a Type of the Value embeded in OID to set eg: i-integer,s-string etc
 *  @param Ipaddress   - In - Ip address of the Agent-Running
 *  @param CommunityString  In - Security String given in configure file eg:for vividloig "Vividlogic123"
 *  Result - It will try to set the corresponding OID value ,using system() call with snmpset  binary"
*/
void TlvConfig::vlSnmpSet_OIDvalue(char* OIDset, void *oidValue, char valuetype, char *IPaddress, char *CommunityString)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    // Apr-17-2009: replace with new impl
    if(valuetype == SNMPSET_INTEGER)//'i'
    {
        char szValue[256];
        snprintf(szValue, sizeof(szValue), "%d", *((int*)oidValue));
        vlSnmpSetLocalStrVar(OIDset, valuetype, szValue, VL_SNMP_CLIENT_REQUEST_TYPE_SYNCHRONOUS);
    }
    else if(valuetype == SNMPSET_STRING)//'s'
    {
        vlSnmpSetLocalStrVar(OIDset, valuetype, (char*)(oidValue), VL_SNMP_CLIENT_REQUEST_TYPE_SYNCHRONOUS);
    }
    else if(valuetype == SNMPSET_BITS)//'b'
    {
        vlSnmpSetLocalStrVar(OIDset, valuetype, (char*)(oidValue), VL_SNMP_CLIENT_REQUEST_TYPE_SYNCHRONOUS);
    }
    else
    {
        vlSnmpSetLocalStrVar(OIDset, valuetype, (char*)(oidValue), VL_SNMP_CLIENT_REQUEST_TYPE_SYNCHRONOUS);
    }
//auto_unlock(&vlg_TlvEventDblock);

}

void TlvConfig::Generate_Events_MIB_OID_VALUE()
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    // Apr-17-2009: deprecated code
//auto_unlock(&vlg_TlvEventDblock);

}

void TlvConfig::Generate_Events_VendorID()
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    // Apr-29-2009: deprecated code
//auto_unlock(&vlg_TlvEventDblock);

}

void TlvConfig::Generate_Events_VendorSpec()
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    // Apr-29-2009: deprecated code
//auto_unlock(&vlg_TlvEventDblock);

}
void TlvConfig::Generate_Events_IpModeControl()
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    // Apr-24-2009: deprecated code
//auto_unlock(&vlg_TlvEventDblock);

}

void TlvConfig::Generate_Events_v3NotificationReceiver()
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    if(IpModeControlList.size() > 0)
    {
        int ipMode = IpModeControlList[0];

        for(int iloop= 0; iloop < v3NotificationReceiver_list.size(); iloop++)
        {//check for duplicates

            switch(ipMode)
            {
                case 0:
                {
                    if(v3NotificationReceiver_list[iloop].v3ipV6address.size() > 0)
                    {
                        EdocsDevEventhandling(ERRORCODE_I438_4);
                    }
                }
                break;

                case 1:
                {
                    if(v3NotificationReceiver_list[iloop].v3ipV4address.size() > 0)
                    {
                        EdocsDevEventhandling(ERRORCODE_I438_4);
                    }
                }
                break;
            }
        }
    }
//auto_unlock(&vlg_TlvEventDblock);

}

void TlvConfig::registerBootupConfig()
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    unsigned char localhost[] = {127, 0, 0, 0};
    unsigned char localhostmask[] = {255, 0, 0, 0};
    unsigned char localhost6[] =
    {
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 1
    };
    unsigned char localhost6mask[] =
    {
        255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255
    };

    snmpClearCommunityTableEvents();
    snmpClearTargetAddrTableEvents();
    snmpClearTargetParamsTableEvents();

    snmpV1V2CommunityTableEventhandling(VL_SNMP_ACCESS_CONFIG_LOCAL_PUBLIC, "local_", "public", true);
    snmpV1V2CommunityTableEventhandling(VL_SNMP_ACCESS_CONFIG_LOCAL_PRIVATE, "local_", "private", true);

    VL_TLV_TRANSPORT_ADDR_AND_MASK localTransportAddNMask;
    localTransportAddNMask.bIsPermanent = true;

    // for v4 public
    localTransportAddNMask.strPrefix = "v4";
    localTransportAddNMask.iAccessViewTlv = VL_SNMP_ACCESS_CONFIG_LOCAL_PUBLIC;
    localTransportAddNMask.iTlvSubEntry = 0;
    localTransportAddNMask.TpAddress.assign(localhost, &localhost[4]);
    localTransportAddNMask.TpMask.assign(localhostmask, &localhostmask[4]);
    snmpTargetAddrTableEventhandling(localTransportAddNMask);

    // for v6 public
    localTransportAddNMask.strPrefix = "v6";
    localTransportAddNMask.iAccessViewTlv = VL_SNMP_ACCESS_CONFIG_LOCAL_PUBLIC;
    localTransportAddNMask.iTlvSubEntry = 1;
    localTransportAddNMask.TpAddress.assign(localhost6, &localhost6[16]);
    localTransportAddNMask.TpMask.assign(localhost6mask, &localhost6mask[16]);
    snmpTargetAddrTableEventhandling(localTransportAddNMask);

    // for v4 private
    localTransportAddNMask.strPrefix = "v4";
    localTransportAddNMask.iAccessViewTlv = VL_SNMP_ACCESS_CONFIG_LOCAL_PRIVATE;
    localTransportAddNMask.iTlvSubEntry = 0;
    localTransportAddNMask.TpAddress.assign(localhost, &localhost[4]);
    localTransportAddNMask.TpMask.assign(localhostmask, &localhostmask[4]);
    snmpTargetAddrTableEventhandling(localTransportAddNMask);

    // for v6 private
    localTransportAddNMask.strPrefix = "v6";
    localTransportAddNMask.iAccessViewTlv = VL_SNMP_ACCESS_CONFIG_LOCAL_PRIVATE;
    localTransportAddNMask.iTlvSubEntry = 1;
    localTransportAddNMask.TpAddress.assign(localhost6, &localhost6[16]);
    localTransportAddNMask.TpMask.assign(localhost6mask, &localhost6mask[16]);
    snmpTargetAddrTableEventhandling(localTransportAddNMask);
    //auto_unlock(&vlg_TlvEventDblock);
}

VL_SNMP_API_RESULT tlvConfig_generateDefaultConfig(vector<string> & snmpdconf_buff)
{
    /** Default snmpd.conf line for vividlogic SUPPORT , community string is Vividlogic123 */
    snmpdconf_buff.push_back("com2sec  v4config_0 127.0.0.0/255.0.0.0 public");
    snmpdconf_buff.push_back("com2sec6 v6config_0 ::1 public");
    snmpdconf_buff.push_back("com2sec  v4config_1 127.0.0.0/255.0.0.0 private");
    snmpdconf_buff.push_back("com2sec6 v6config_1 ::1 private\n");

    snmpdconf_buff.push_back("vacmGroup 1 4 1 v4config_0  v4groupV1_0");
    snmpdconf_buff.push_back("vacmGroup 1 4 2 v4config_0  v4groupV2_0");
    snmpdconf_buff.push_back("vacmView 1 4 1 local_view .1");
    snmpdconf_buff.push_back("vacmAccess 1 4 1 1 1 v4groupV1_0  \"\" local_view");
    snmpdconf_buff.push_back("vacmAccess 1 4 2 1 1 v4groupV2_0  \"\" local_view\n");

    snmpdconf_buff.push_back("vacmGroup 1 4 1 v6config_0  v6groupV1_0");
    snmpdconf_buff.push_back("vacmGroup 1 4 2 v6config_0  v6groupV2_0");
    snmpdconf_buff.push_back("vacmView 1 4 1 local_view .1");
    snmpdconf_buff.push_back("vacmAccess 1 4 1 1 1 v6groupV1_0  \"\" local_view");
    snmpdconf_buff.push_back("vacmAccess 1 4 2 1 1 v6groupV2_0  \"\" local_view\n");

    snmpdconf_buff.push_back("vacmGroup 1 4 1 v4config_1 v4groupV1_1");
    snmpdconf_buff.push_back("vacmGroup 1 4 2 v4config_1 v4groupV2_1");
    snmpdconf_buff.push_back("vacmView 1 4 1 local_view .1");
    snmpdconf_buff.push_back("vacmAccess 1 4 1 1 1 v4groupV1_1 \"\" local_view local_view");
    snmpdconf_buff.push_back("vacmAccess 1 4 2 1 1 v4groupV2_1 \"\" local_view local_view\n");

    snmpdconf_buff.push_back("vacmGroup 1 4 1 v6config_1 v6groupV1_1");
    snmpdconf_buff.push_back("vacmGroup 1 4 2 v6config_1 v6groupV2_1");
    snmpdconf_buff.push_back("vacmView 1 4 1 local_view .1");
    snmpdconf_buff.push_back("vacmAccess 1 4 1 1 1 v6groupV1_1 \"\" local_view local_view");
    snmpdconf_buff.push_back("vacmAccess 1 4 2 1 1 v6groupV2_1 \"\" local_view local_view\n");

    if(vl_isFeatureEnabled((unsigned char*)"SNMP.ENABLE_DVL_ACCESS"))
    {
        snmpdconf_buff.push_back("com2sec  STBconfig     default Vividlogic123");
        snmpdconf_buff.push_back("com2sec6 STBIPv6config default Vividlogic123\n");

        snmpdconf_buff.push_back("com2sec  v4_no_access_config default public");
        snmpdconf_buff.push_back("com2sec6 v6_no_access_config default public");
        snmpdconf_buff.push_back("com2sec  v4_no_access_config default private");
        snmpdconf_buff.push_back("com2sec6 v6_no_access_config default private");
        snmpdconf_buff.push_back("com2sec  v4_no_access_config default supermax");
        snmpdconf_buff.push_back("com2sec6 v6_no_access_config default supermax\n");

        snmpdconf_buff.push_back("group VLgroup     v1   STBconfig");
        snmpdconf_buff.push_back("group VLgroup     v2c  STBconfig");
        snmpdconf_buff.push_back("group VLIPV6group v1   STBIPv6config");
        snmpdconf_buff.push_back("group VLIPV6group v2c  STBIPv6config\n");

        snmpdconf_buff.push_back("group v4_no_access_group   v1         v4_no_access_config");
        snmpdconf_buff.push_back("group v4_no_access_group   v2c        v4_no_access_config");
        snmpdconf_buff.push_back("group v6_no_access_group   v1         v6_no_access_config");
        snmpdconf_buff.push_back("group v6_no_access_group   v2c        v6_no_access_config\n");

        snmpdconf_buff.push_back("view  _dvlaccess_  included .1");

        snmpdconf_buff.push_back("access VLgroup     \"\"   v1  noauth    exact  _dvlaccess_     _dvlaccess_     none");
        snmpdconf_buff.push_back("access VLgroup     \"\"   v2c noauth    exact  _dvlaccess_     _dvlaccess_     none");
        snmpdconf_buff.push_back("access VLIPV6group \"\"   v1  noauth    exact  _dvlaccess_     _dvlaccess_     none");
        snmpdconf_buff.push_back("access VLIPV6group \"\"   v2c noauth    exact  _dvlaccess_     _dvlaccess_     none\n");

        snmpdconf_buff.push_back("access v4_no_access_group \"\"   v1  noauth    exact  none           none           none");
        snmpdconf_buff.push_back("access v4_no_access_group \"\"   v2c noauth    exact  none           none           none");
        snmpdconf_buff.push_back("access v6_no_access_group \"\"   v1  noauth    exact  none           none           none");
        snmpdconf_buff.push_back("access v6_no_access_group \"\"   v2c noauth    exact  none           none           none\n");
    }
    
    snmpdconf_buff.push_back("agentaddress udp:161,udp6:161,tcp:1161,dtlsudp:10161,tlstcp:10161,dtlsudp6:10161,tlstcp6:10161");
    snmpdconf_buff.push_back("master agentx");
    snmpdconf_buff.push_back("agentXSocket tcp:127.0.0.1:705\n");
    snmpdconf_buff.push_back("agentXTimeout 30\n");
    return VL_SNMP_API_RESULT_SUCCESS;
}

VL_SNMP_API_RESULT tlvConfig_appendDefaultConfig(vector<string> & snmpdconf_buff, const char * pszFilePath)
{
    if(NULL == pszFilePath) return VL_SNMP_API_RESULT_NULL_PARAM;
    
    FILE * fp = fopen(pszFilePath, "r");
    if(NULL == fp)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "%s: Could not open '%s'.\n", __FUNCTION__, pszFilePath);
        return VL_SNMP_API_RESULT_OPEN_FAILED;
    }
    else
    {
        char szBuf[1024];
        
        int nLines = 0;
        while(!feof(fp))
        {
            VL_ZERO_MEMORY(szBuf);
            char * str = fgets(szBuf, sizeof(szBuf), fp);
            if(NULL == str) break;
            char * pszNewLine = strrchr(str, '\n');
            if((NULL != pszNewLine) && (pszNewLine < (szBuf+sizeof(szBuf))))
            {
                pszNewLine[0] = '\0'; // Trim '\n' character.
            }
            snmpdconf_buff.push_back(szBuf);
            nLines++;
        }
        
        fclose(fp);
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "%s: Appendend %d lines from '%s'.\n", __FUNCTION__, nLines, pszFilePath);
    }
    return VL_SNMP_API_RESULT_SUCCESS;
}


void find_and_replace(std::string &buffer, std::string toSearch, std::string replaceWith)
{
    size_t pos = buffer.find(toSearch);
    while( pos != std::string::npos)
    {
        buffer.replace(pos, toSearch.size(), replaceWith);
        pos =  buffer.find(toSearch, pos + replaceWith.size());
    }
}


void disableSnmpV2( char *snmpdconf )
{
    fstream confFile(snmpdconf, fstream::in);
    string conf_data;
    getline (confFile, conf_data, (char) confFile.eof());
    confFile.close();

    find_and_replace(conf_data, "0.0.0.0/0", "127.0.0.0/255.0.0.0");
    find_and_replace(conf_data, "::/0", "::1");

    fstream outFile(snmpdconf, fstream::out);
    outFile << conf_data;
    outFile.close();
}

/** Read-Write the configure if TLV217 has no errosEvents file and usning snmpset update the UCD-SNMP-MIB::versionUpdateConfig.0 to update the changed configure file */
void TlvConfig::Set_AllListV1v2v3objvalus(VL_HOST_IP_INFO * pPodIpInfo)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    vector<string> snmpdconf_buff;
    int oidvaues = 1;
    /*snmpset support values*/
    string strConfig;

    registerBootupConfig();

    if(NULL != pPodIpInfo)
    {
        char config_buff[API_CHRMAX];

        snprintf(config_buff, sizeof(config_buff), "com2sec  pod_config_0  %d.%d.%d.%d/%d.%d.%d.%d  public\n",
                (pPodIpInfo->aBytIpAddress[0])&(pPodIpInfo->aBytSubnetMask[0]),
                (pPodIpInfo->aBytIpAddress[1])&(pPodIpInfo->aBytSubnetMask[1]),
                (pPodIpInfo->aBytIpAddress[2])&(pPodIpInfo->aBytSubnetMask[2]),
                (pPodIpInfo->aBytIpAddress[3])&(pPodIpInfo->aBytSubnetMask[3]),
                (pPodIpInfo->aBytSubnetMask[0]),
                (pPodIpInfo->aBytSubnetMask[1]),
                (pPodIpInfo->aBytSubnetMask[2]),
                (pPodIpInfo->aBytSubnetMask[3]));
        snmpdconf_buff.push_back(config_buff);

        snmpdconf_buff.push_back("vacmGroup 1 2 1 pod_config_0 pod_configV1_0");
        snmpdconf_buff.push_back("vacmGroup 1 2 2 pod_config_0 pod_configV2_0\n");

        snmpdconf_buff.push_back("vacmView 1 2 1 pod_view .1");
        snmpdconf_buff.push_back("vacmAccess 1 2 1 1 1 pod_configV1_0 \"\" pod_view");
        snmpdconf_buff.push_back("vacmAccess 1 2 2 1 1 pod_configV2_0 \"\" pod_view\n");

        snprintf(config_buff, sizeof(config_buff), "com2sec  pod_config_1 %d.%d.%d.%d/%d.%d.%d.%d  private\n",
                (pPodIpInfo->aBytIpAddress[0])&(pPodIpInfo->aBytSubnetMask[0]),
                (pPodIpInfo->aBytIpAddress[1])&(pPodIpInfo->aBytSubnetMask[1]),
                (pPodIpInfo->aBytIpAddress[2])&(pPodIpInfo->aBytSubnetMask[2]),
                (pPodIpInfo->aBytIpAddress[3])&(pPodIpInfo->aBytSubnetMask[3]),
                (pPodIpInfo->aBytSubnetMask[0]),
                (pPodIpInfo->aBytSubnetMask[1]),
                (pPodIpInfo->aBytSubnetMask[2]),
                (pPodIpInfo->aBytSubnetMask[3]));
        snmpdconf_buff.push_back(config_buff);

        snmpdconf_buff.push_back("vacmGroup 1 2 1 pod_config_1 pod_configV1_1");
        snmpdconf_buff.push_back("vacmGroup 1 2 2 pod_config_1 pod_configV2_1\n");

        snmpdconf_buff.push_back("vacmView 1 2 1 pod_view .1");
        snmpdconf_buff.push_back("vacmAccess 1 2 1 1 1 pod_configV1_1 \"\" pod_view pod_view");
        snmpdconf_buff.push_back("vacmAccess 1 2 2 1 1 pod_configV2_1 \"\" pod_view pod_view\n");

        snmpV1V2CommunityTableEventhandling(0, "pod_", "public", false);
        snmpV1V2CommunityTableEventhandling(1, "pod_", "private", false);

        VL_TLV_TRANSPORT_ADDR_AND_MASK localTransportAddNMask;
        localTransportAddNMask.bIsPermanent = false;

        // for v4 public
        localTransportAddNMask.strPrefix = "pod_";
        localTransportAddNMask.iAccessViewTlv = 0;
        localTransportAddNMask.iTlvSubEntry = 0;
        localTransportAddNMask.TpAddress.clear();
        localTransportAddNMask.TpAddress.push_back((pPodIpInfo->aBytIpAddress[0])&(pPodIpInfo->aBytSubnetMask[0]));
        localTransportAddNMask.TpAddress.push_back((pPodIpInfo->aBytIpAddress[1])&(pPodIpInfo->aBytSubnetMask[1]));
        localTransportAddNMask.TpAddress.push_back((pPodIpInfo->aBytIpAddress[2])&(pPodIpInfo->aBytSubnetMask[2]));
        localTransportAddNMask.TpAddress.push_back((pPodIpInfo->aBytIpAddress[3])&(pPodIpInfo->aBytSubnetMask[3]));
        localTransportAddNMask.TpMask.assign(pPodIpInfo->aBytSubnetMask, &(pPodIpInfo->aBytSubnetMask[4]));
        snmpTargetAddrTableEventhandling(localTransportAddNMask);

        // for v4 private
        localTransportAddNMask.strPrefix = "pod_";
        localTransportAddNMask.iAccessViewTlv = 1;
        localTransportAddNMask.iTlvSubEntry = 0;
        localTransportAddNMask.TpAddress.clear();
        localTransportAddNMask.TpAddress.push_back((pPodIpInfo->aBytIpAddress[0])&(pPodIpInfo->aBytSubnetMask[0]));
        localTransportAddNMask.TpAddress.push_back((pPodIpInfo->aBytIpAddress[1])&(pPodIpInfo->aBytSubnetMask[1]));
        localTransportAddNMask.TpAddress.push_back((pPodIpInfo->aBytIpAddress[2])&(pPodIpInfo->aBytSubnetMask[2]));
        localTransportAddNMask.TpAddress.push_back((pPodIpInfo->aBytIpAddress[3])&(pPodIpInfo->aBytSubnetMask[3]));
        localTransportAddNMask.TpMask.assign(pPodIpInfo->aBytSubnetMask, &(pPodIpInfo->aBytSubnetMask[4]));
        snmpTargetAddrTableEventhandling(localTransportAddNMask);
    }

    /*End snmpset support values*/
    if(m_bUpdateSnmpdConf)
    {
        SNMP_DEBUGPRINT("m_bUpdateSnmpdConf ====  %d \n",m_bUpdateSnmpdConf);
        SNMP_DEBUGPRINT("\n write the collect list into the snmpd.cnfg as u got the GOOD TLV-217 \n");

        /*THe Tlv217loop list gives the communtiy list of n'53list and ip and ip max list of n'53list  */
        for(int tlvcount=0;  tlvcount < Tlv217loopList.size(); tlvcount++)
        {
            SNMP_DEBUGPRINT("\n ||||||||||||||||||||   TVL COM2SEC and GROUP LIST ||||||||||||\n ");
            /*To populate the snmpcommunity Table*/
            snmpV1V2CommunityTableEventhandling(tlvcount, "@STB", (char*)Tlv217loopList[tlvcount].strCommunity.c_str());

            for(int tlvIpconunt = 0;  tlvIpconunt < Tlv217loopList[tlvcount].AVTransportAddressList.size(); tlvIpconunt++)
            {
                VL_TLV_TRANSPORT_ADDR_AND_MASK transportAddNMask = Tlv217loopList[tlvcount].AVTransportAddressList[tlvIpconunt];
                transportAddNMask.strPrefix = "@STB";
                snmpTargetAddrTableEventhandling(transportAddNMask);

                strConfig = transportAddNMask.getCom2SecConfig(tlvcount, Tlv217loopList[tlvcount].strCommunity);
                snmpdconf_buff.push_back(strConfig);
                strConfig = transportAddNMask.getVacmGroupConfig(SNMP_SEC_MODEL_SNMPv1, tlvcount);
                snmpdconf_buff.push_back(strConfig);
                strConfig = transportAddNMask.getVacmGroupConfig(SNMP_SEC_MODEL_SNMPv2c, tlvcount);
                snmpdconf_buff.push_back(strConfig);
                snmpdconf_buff.push_back("\n");
            }//for inner

        }//for top

        snmpdconf_buff.push_back("vacmView 1 2 2 readonly .1.3.6.1.6\n");

        for(int tlvcount=0;  tlvcount < Tlv217loopList.size(); tlvcount++)
        {
            strConfig = Tlv217loopList[tlvcount].getVacmViewConfig(SNMP_SEC_MODEL_SNMPv1, tlvcount);
            snmpdconf_buff.push_back(strConfig);
            strConfig = Tlv217loopList[tlvcount].getVacmAccessConfig(SNMP_SEC_MODEL_SNMPv1, tlvcount);
            snmpdconf_buff.push_back(strConfig);
            strConfig = Tlv217loopList[tlvcount].getVacmAccessConfig(SNMP_SEC_MODEL_SNMPv2c, tlvcount);
            snmpdconf_buff.push_back(strConfig);
            snmpdconf_buff.push_back("\n");

        }//for

        for(int tlvcount=0;  tlvcount < V3AccviewLs.size(); tlvcount++)
        {
            strConfig = V3AccviewLs[tlvcount].getVacmViewConfig(SNMP_SEC_MODEL_USM, tlvcount);
            snmpdconf_buff.push_back(strConfig);
            /* May-12-2009: Not required for now (as per OC-TP-HOST2.1-ATP-D05-090312)
            strConfig = V3AccviewLs[tlvcount].getVacmAccessConfig(SNMP_SEC_MODEL_USM, tlvcount);
            snmpdconf_buff.push_back(strConfig);*/
            snmpdconf_buff.push_back("\n");
        }

        if(v3NotificationReceiver_list.size() > 0)
        {
            strConfig = v3NotificationReceiver_t::getVacmGroupConfig(SNMP_SEC_MODEL_SNMPv1);
            snmpdconf_buff.push_back(strConfig);
            strConfig = v3NotificationReceiver_t::getVacmGroupConfig(SNMP_SEC_MODEL_SNMPv2c);
            snmpdconf_buff.push_back(strConfig);
            strConfig = v3NotificationReceiver_t::getVacmViewConfig(SNMP_SEC_MODEL_SNMPv1);
            snmpdconf_buff.push_back(strConfig);
            strConfig = v3NotificationReceiver_t::getVacmAccessConfig(SNMP_SEC_MODEL_SNMPv1);
            snmpdconf_buff.push_back(strConfig);
            strConfig = v3NotificationReceiver_t::getVacmAccessConfig(SNMP_SEC_MODEL_SNMPv2c);
            snmpdconf_buff.push_back(strConfig);
            snmpdconf_buff.push_back("\n");
        }

        for(int iNotify=0;  iNotify < v3NotificationReceiver_list.size(); iNotify++)
        {
            char notify_buff[API_CHRMAX];

            v3NotificationReceiver_t & rReceiver = v3NotificationReceiver_list[iNotify];

            for(int iTrapType = 0; iTrapType < rReceiver.anV3TrapTypes.size(); iTrapType++)
            {
                char szTrapType[VL_IPV6_ADDR_SIZE];
                vlStrCpy(szTrapType, "", sizeof(szTrapType));

                switch(rReceiver.anV3TrapTypes[iTrapType])
                {
                    case VL_TLV_217_NOTIFY_TRAP_V1:
                    {
                        vlStrCpy(szTrapType, "trapsink", sizeof(szTrapType));
                    }
                    break;

                    case VL_TLV_217_NOTIFY_TRAP_V2C:
                    {
                        vlStrCpy(szTrapType, "trap2sink", sizeof(szTrapType));
                    }
                    break;

                    case VL_TLV_217_NOTIFY_INFORM_V2C:
                    {
                        vlStrCpy(szTrapType, "informsink", sizeof(szTrapType));
                    }
                    break;
                }

                if(strlen(szTrapType) > 0)
                {
                    if(VL_IP_ADDR_SIZE == rReceiver.v3ipV4address.size())
                    {
                        string strIpAddress = VL_TLV_TRANSPORT_ADDR_AND_MASK::getAddrString(rReceiver.v3ipV4address);
                        snprintf(notify_buff, sizeof(notify_buff), "%s %s:%d %s", szTrapType, strIpAddress.c_str(), rReceiver.nV3UdpPortNumber, "public");
                        snmpdconf_buff.push_back(notify_buff);
                    }

                    if(VL_IPV6_ADDR_SIZE == rReceiver.v3ipV6address.size())
                    {
                        string strIpAddress = VL_TLV_TRANSPORT_ADDR_AND_MASK::getAddrString(rReceiver.v3ipV6address);
                        snprintf(notify_buff, sizeof(notify_buff), "%s %s:%d %s", szTrapType, strIpAddress.c_str(), rReceiver.nV3UdpPortNumber, "public");
                        snmpdconf_buff.push_back(notify_buff);
                    }
                }
                snmpNotifyTableEventhandling(NULL);
                snmpTargetParamsTableEventhandling(rReceiver);
            }
            snmpdconf_buff.push_back("\n");
        }
        
        {//Populate /tmp/snmpd.conf from the default /etc/snmp/snmpd.conf parameters in during runtime.

            const char * aDefaultSnmpdPaths[]
            {
                "/etc/snmp/snmpd.conf",
                "/mnt/nfs/bin/target-snmp/bin/snmpd.conf"
            };
            
            int iPath = 0;
            bool bSnmpdConfFound = false;
            for(iPath = 0; iPath < VL_ARRAY_ITEM_COUNT(aDefaultSnmpdPaths); iPath++)
            {
                if(VL_SNMP_API_RESULT_SUCCESS == tlvConfig_appendDefaultConfig(snmpdconf_buff, aDefaultSnmpdPaths[iPath]))
                {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "%s: Loaded default snmpd.conf from '%s'.\n", __FUNCTION__, aDefaultSnmpdPaths[iPath]);
                    bSnmpdConfFound = true;
                    break;
                }
                else
                {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "%s: Could not load default snmpd.conf from '%s'.\n", __FUNCTION__, aDefaultSnmpdPaths[iPath]);
                }
            }

            if(false == bSnmpdConfFound)
            {
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "%s: Generating default snmpd.conf. Unlikely to be useful due to inaccessiblity for versionUpdateConfig.\n", __FUNCTION__);
                tlvConfig_generateDefaultConfig(snmpdconf_buff);
            }
        }
        
        SNMP_DEBUGPRINT("\n ||||||||  Write this line into the Snmpd.conf file |||||||| snmpdconf_buff.size():%d", snmpdconf_buff.size());

        FILE* file_rd, *file_wr;
        int snmpdconf = 0;

        char cmd_buffer[256];
        char snmpdconf_Backupfilepath_buff[API_CHRMAX];
        const char * pfcroot_fsnmp = "/tmp"; // Correction of SNMPCONFPATH for Yocto builds
        int len = 0;
        char file_name[] = "/../bin/snmpd.conf";
        if( pfcroot_fsnmp == NULL ) 
        {
             RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SNMP", "%s:%d: SNMPCONFPATH is NULL !! \n", __FUNCTION__,__LINE__);
             return;			 
        }
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "%s(): SNMPCONFPATH='%s'\n", __FUNCTION__, pfcroot_fsnmp);
        len = strlen( pfcroot_fsnmp ); 
        if( API_CHRMAX <= ( (len + 1) + ( sizeof(file_name) ) ) ) 
        {
              RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SNMP", "%s:%d: SNMPCONFPATH causes buffer over run !!\n", __FUNCTION__,__LINE__);
              return;
        }
        vlStrCpy(snmpdconf_Backupfilepath_buff, "", sizeof(snmpdconf_Backupfilepath_buff));
        vlStrCat(snmpdconf_Backupfilepath_buff, pfcroot_fsnmp, sizeof(snmpdconf_Backupfilepath_buff));
        vlStrCat(snmpdconf_Backupfilepath_buff, file_name, sizeof(snmpdconf_Backupfilepath_buff));

        char snmpdconf_filepath_buff[API_CHRMAX];
        vlStrCpy(snmpdconf_filepath_buff, "", sizeof(snmpdconf_filepath_buff));
        vlStrCat(snmpdconf_filepath_buff, pfcroot_fsnmp, sizeof(snmpdconf_filepath_buff));
        /*wking with mnt/nfs path ::: /mnt/nfs/bin/target-snmp/share/snmp/snmpd.conf*/
        vlStrCat(snmpdconf_filepath_buff,"/snmpd.conf", sizeof(snmpdconf_filepath_buff));
        /*So that every thim it will replaced by the backup-snmpdfile which is of less lines - so that no need to care about the number of lines writen (as default back-upfiles contains less number of lines data && Even if file open fails then our default conf will be applid to snmpd*/
        snprintf(cmd_buffer, sizeof(cmd_buffer), "cp -f  %s  %s",
                snmpdconf_filepath_buff, snmpdconf_Backupfilepath_buff);
        // Apr-17-2009: Avoid overwriting of default conf file //system((const char *)&cmd_buffer);
        SNMP_DEBUGPRINT("%s:%d: SNMPDCONFPATH %s\n", __FUNCTION__,__LINE__, snmpdconf_filepath_buff);

        file_rd = fopen(snmpdconf_filepath_buff, "w+");
        if(file_rd ==  NULL)
        {
            SNMP_DEBUGPRINT("\n Error file_rd Failed   %d\n ",errno);
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SNMP", "%s(): fopen('%s') FAILED.\n", __FUNCTION__, snmpdconf_filepath_buff);
        }
        else
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "%s(): fopen('%s') SUCCEEDED.\n", __FUNCTION__, snmpdconf_filepath_buff);
            while(snmpdconf < snmpdconf_buff.size() /*!feof(file_rd)*/)
            {
                                            //      line[0]='\n';
                SNMP_DEBUGPRINT("line in writing into the snmpd.conf are:: %s\n",snmpdconf_buff[snmpdconf].c_str());
                fputs(snmpdconf_buff[snmpdconf].c_str(),file_rd);
                fputs("\n",file_rd);
                snmpdconf++;
            }
              //while(!feof(file_rd))

            RFC_ParamData_t snmpv3Data, snmpv2Data;
            WDMP_STATUS snmpv3rfc = getRFCParameter("SNMP", "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.SNMP.V3Support", &snmpv3Data);
            if ( WDMP_SUCCESS == snmpv3rfc )
            {
                    if ( 0 == strncmp(snmpv3Data.value, "true", 4) )
                    {
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "RFC Settings for SNMPv3 is : true\n");
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "Enabling support for SNMPv3 queries\n");
                        fputs("includeFile /tmp/.snmp/tls/certs/snmpd_v3.conf", file_rd);
                        fputs("\n",file_rd);
                    }
            } else {
                    //RFC configuration for SNMPv3 not available. Defaulting SNMPv3 to true
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "RFC configuration for SNMPv3 not available. Defaulting to SNMPv3 true \n");
                    fputs("includeFile /tmp/.snmp/tls/certs/snmpd_v3.conf", file_rd);
                    fputs("\n",file_rd);
            }

            fclose(file_rd);
          
            WDMP_STATUS snmpv2rfc = getRFCParameter("SNMP", "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.SNMP.V2Support", &snmpv2Data);
            if ( WDMP_SUCCESS == snmpv2rfc )
            {
                    if ( 0 == strncmp(snmpv2Data.value, "false", 5) )
                    {
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "RFC Settings for SNMPv2 is : false\n");
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "Disabling support for SNMPv2 queries\n");
                        disableSnmpV2(snmpdconf_filepath_buff);
                    }
            } else {
                    //RFC configuration for SNMPv2 not available. Defaulting SNMPv2 to true
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "RFC Settings for SNMPv2 is : true\n");
            }
          
            /*After re-writing the file update the snmpdconfversionUpdate file by using snmpset */
            vlSnmpSet_OIDvalue("UCD-SNMP-MIB::versionUpdateConfig.0", &oidvaues, 'i', "127.0.0.1", "Vividlogic123");
            //vlSnmpSetLocalLong("UCD-SNMP-MIB::versionUpdateConfig.0", 1, VL_SNMP_CLIENT_REQUEST_TYPE_SYNCHRONOUS);
            //vlSnmpSetLocalAsnVar("UCD-SNMP-MIB::versionUpdateConfig.0", ASN_INTEGER, &oidvaues, sizeof(oidvaues), VL_SNMP_CLIENT_REQUEST_TYPE_SYNCHRONOUS);
            //vlSnmpSetLocalStrVar("UCD-SNMP-MIB::versionUpdateConfig.0", 'i', "1", VL_SNMP_CLIENT_REQUEST_TYPE_SYNCHRONOUS);

            if(v3NotificationReceiver_list.size() > 0)
            {
                char szTrap[64];
                char szInform[64];
                VL_SNMP_CLIENT_VALUE aResults[] =
                {
                    VL_SNMP_CLIENT_MULTI_VAR_ENTRY((char*)"SNMP-NOTIFICATION-MIB::snmpNotifyTag.'@STBnotifyconfig_inform'", VL_SNMP_CLIENT_VALUE_TYPE_BYTE    , sizeof(szInform), szInform),
                    VL_SNMP_CLIENT_MULTI_VAR_ENTRY((char*)"SNMP-NOTIFICATION-MIB::snmpNotifyTag.'@STBnotifyconfig_trap'"  , VL_SNMP_CLIENT_VALUE_TYPE_BYTE    , sizeof(szTrap  ), szTrap  ),
                };
                int nResults = VL_ARRAY_ITEM_COUNT(aResults);
                // perform a dummy get to initialize the snmpNotifyTable cache
                vlSnmpGetLocalObjects(NULL, &nResults, aResults);
            }
        }/*filw writing completes*/
        /*calling a set snmpset to reload the snmpd.conf file , set UCD-SNMP-MIB::versionUpdateConfig.0*/


        for(int snmpdconf = 0; snmpdconf < snmpdconf_buff.size(); snmpdconf++)
        {
            SNMP_DEBUGPRINT("line in snmpdconf_buff.size() are:: %s\n",snmpdconf_buff[snmpdconf].c_str());

        }

    }//if
    else
    {
        SNMP_DEBUGPRINT("m_bUpdateSnmpdConf ====  %d \n",m_bUpdateSnmpdConf);
        SNMP_DEBUGPRINT("\n  AS the TLV is has some Error Events so , TLV-217 is not good to form a snmpd.conf file for SNMP access control \n");
    }
    /** Collected List of v1 v2c and v3 list items are write into the snmpd.cnfg file if gsetflag is true*/

    // initialize any SNMP caches
    {
        int  nVal = 0;
        VL_SNMP_CLIENT_VALUE aResults[] =
        {
            VL_SNMP_CLIENT_MULTI_VAR_ENTRY((char*)"OC-STB-HOST-MIB::ocStbHostRebootType.0" , VL_SNMP_CLIENT_VALUE_TYPE_INTEGER , 1 , &nVal   ),
        };
        int nResults = VL_ARRAY_ITEM_COUNT(aResults);
        // perform a dummy get to initialize any caches
        vlSnmpGetLocalObjects(NULL, &nResults, aResults);
    }
//auto_unlock(&vlg_TlvEventDblock);

}

void TlvConfig::checkEventlistInTlv217list()
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    int iloop;
    if(Tlv217loopList.size() == 0 )
    {
        SNMP_DEBUGPRINT("\n ERROR: generate an event Nolist found in Tlv217 :: so no events:: \n");
    }

    //vector<Tlv217loopList> Tvl53Nlist_it;

    //for(TLV53_V1V2_CONFIG objtvl217 = Tlv217loopList.begin(); tlvlist_it !=Tlv217loopList.end(); tlvlist_it++)
    // #if 0
    Generate_Events_Genral();
    //53.1-TLV
    Generate_Events_V1V2Community();
    //53.2-TLV
    Generate_Events_V1V2TransportAddress_MaxAdd();
     //53.3-TLV && 53.4-TLV
    Generate_Events_V1V2TypeNameAccess();
    //53.4
    Generate_Events_V1V2NameAccess();
    //54-TLV
    Generate_Events_V3AVMaskTypeEvent();
     //11-TLV
    Generate_Events_MIB_OID_VALUE();
    //8-TLV
    Generate_Events_VendorID();
    //48-TLV
    Generate_Events_VendorSpec();
    //1-TLV
    Generate_Events_IpModeControl();
    //38-TLV
    Generate_Events_v3NotificationReceiver();
    Set_AllListV1v2v3objvalus();
//auto_unlock(&vlg_TlvEventDblock);

}

int TlvConfig::initialize(void)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

    rmf_osal_eventmanager_handle_t em = 0;// vivek IPC_CLIENT_get_pod_event_manager();
#ifdef TEST_WITH_PODMGR
	rmf_Error ret = rmf_snmpmgrRegisterEvents(m_pEvent_queue);

	if( ret == RMF_SUCCESS )
	{
  		printf ("%s : SNMP module : registering to pod event listener is success \n",__FUNCTION__);
	}
	else
#endif
  	{
    	//void* pEventData = NULL;
      	rmf_osal_eventmanager_register_handler(
		em,
		m_pEvent_queue,
		RMF_OSAL_EVENT_CATEGORY_CARD_MANAGER);
	}

    return 1;
}

int TlvConfig::init_complete(void)
{VL_AUTO_LOCK(vlg_TlvEventDblock);

 	SNMP_DEBUGPRINT(" \n\n TLV::initialize..\n");

	//PodMgrEventListener    *listener;
	rmf_osal_eventmanager_handle_t em = 0;//vivek IPC_CLIENT_get_pod_event_manager();
#if 0 // Moved to run function
  	{

    	//void* pEventData = NULL;
      	rmf_osal_eventmanager_register_handler(
		em,
		m_pEvent_queue,
		RMF_OSAL_EVENT_CATEGORY_CARD_MANAGER);
	}
#endif
    	start();
	//auto_unlock(&vlg_TlvEventDblock);

    	return 0;

}


int TlvConfig::is_default ()
{VL_AUTO_LOCK(vlg_TlvEventDblock);
    SNMP_DEBUGPRINT("TlvConfig::is_default\n");

    //SNMP_DEBUGPRINT("\n\n & & &    SnmpManager::is_default\n");
    //auto_unlock(&vlg_TlvEventDblock);
    return 1;
}
