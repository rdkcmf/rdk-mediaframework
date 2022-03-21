/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2018 RDK Management
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
#include <stdint.h>
#ifdef ENABLE_TIME_UPDATE
#include <semaphore.h>
#endif
#include <podServer.h>
#include <podIf.h>
#include <rmf_osal_ipc.h>
#include <rdk_debug.h>
#include <rmf_osal_mem.h>
#include <stdlib.h>
#include <string.h>
#include <podimpl.h>
#include "rmf_oobsimgr.h"
#include "core_events.h"

#ifdef SNMP_IPC_CLIENT

#include <rmf_inbsi_common.h>
#include "vlpluginapp_haldsgapi.h"
#include "cm_api.h"
#include "hostGenericFeatures.h"

#include "cardmanager.h"
#include "coreUtilityApi.h"
#include "rmf_oobService.h"
#include "CommonDownloadSnmpInf.h"
#include "dsgResApi.h"
#include "snmp_types.h"
#include "vlpluginapp_halplatformapi.h"
#include "xchanResApi.h"
#include "hostInfo.h"
#include "vlCCIInfo.h"
#include "pod_api.h"
#include "sys_api.h"
#include "vlMpeosCdlIf.h"

#define VL_MAX_SNMP_STR_SIZE 256
#define VL_MIN_SNMP_STR_SIZE 10
#define ENABLE_POD_CORE_EVENTS

#endif //SNMP_IPC_CLIENT

#ifdef ENABLE_TIME_UPDATE
typedef enum OobSiQuitStatus
{
        OOBSI_THREAD_ACTIVE = 0x200,
        OOBSI_THREAD_INACTIVE,
}OobSiQuitStatus;
#endif

typedef struct
{
	rmf_osal_eventqueue_handle_t eQhandle;
	IPCServer *pPODIPCServer;
	IPCServerBuf * pServerBuf;	
}dispatchThreadData;

typedef struct
{
	IPCServerBuf *pServerBuf;
	IPCServer *pIPCServer;

} recvAPDU;

#define CARD_TYPE_MOTO 0
#define CARD_TYPE_CISCO 1

extern "C" void vlDsgDumpDsgStats();
extern "C" void vlDsgCheckAndRecoverConnectivity();


void thread_recv_apdu( void * );
void recv_pod_cmd( IPCServerBuf * pServerBuf, void *pUserData );
void freeDescInfo( parsedDescriptors * pParseOutput );

/* Standard code deviation: MSCQ582921: end */
extern "C" rmf_Error podImplGetCaSystemId(unsigned short * pCaSystemId);
extern "C" void vlMpeosCdlUpgradeToImage(VLMPEOS_CDL_IMAGE_TYPE eImageType, VLMPEOS_CDL_IMAGE_SIGN eImageSign, int bRebootNow, const char * pszImagePathName);

static void podEventDispatchThread( void *data );
static void podSIMGREventDispatchThread( void *data );
static rmf_osal_ThreadId podEventDispatchThreadId = 0;
static rmf_osal_ThreadId podSIMGREventDispatchThreadId = 0;
IPCServer *pPODIPCServer = NULL;
static rmf_osal_ThreadId recv_apduThreadId;
static uint8_t podmgrDSGIP[ RMF_PODMGR_IP_ADDR_LEN ];

static vlCableCardAppInfo_t appInfo;

static volatile uint32_t pod_server_cmd_port_no;
#ifdef ENABLE_TIME_UPDATE
#if 0 //SS - Change of design
static rmf_osal_ThreadId OobSiEventThreadId;
static rmf_osal_eventqueue_handle_t eOobSIQhandle;
static OobSiQuitStatus g_eOobSiThreadStatus = OOBSI_THREAD_INACTIVE;
static sem_t* g_pMonitorThreadStopSem;

void oobSIEventHandlerThread( void * );
#endif
#endif

/* Seperate server for java starts */
void recv_pod_java_cmd( IPCServerBuf * pServerBuf, void *pUserData );
IPCServer *pPODIPCJAVAServer = NULL;
#define POD_SERVER_CMD_DFLT_PORT_JAVA_NO 50507
typedef struct
{
	uint64_t appId;
	uint32_t sessionId;
}stSASHandler;
#define TOTAL_SAS_HANDLERS 2
#define PROXY_SESSION_ID_INDEX 0 
#define APPID_LEN 8
#define PROXY_APDU_INDEX 4
static volatile bool gbSASTerminated;
static volatile bool gbCacheProxyAPDU;
static volatile stSASHandler sessionCache[TOTAL_SAS_HANDLERS];
static uint8_t apdu_count;

rmf_Error podJavaServerStart(	)
{
	
	uint32_t pod_server_cmd_port_java_no;
	const char *server_port;

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
			 "%s : is called \n", __FUNCTION__ );
	gbCacheProxyAPDU = true;
	server_port = rmf_osal_envGet( "POD_SERVER_CMD_PORT_JAVA_NO" );
	if( NULL == server_port)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s: NULL POINTER  received for name string\n", __FUNCTION__ );
		pod_server_cmd_port_java_no = POD_SERVER_CMD_DFLT_PORT_JAVA_NO;
	}
	else
	{
		pod_server_cmd_port_java_no = atoi( server_port );
	}
	pPODIPCJAVAServer = new IPCServer( ( int8_t * ) "podJavaServer", pod_server_cmd_port_java_no );
	int8_t ret = pPODIPCJAVAServer->start(  );

	if ( 0 != ret )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s : Server failed to start\n",
				 __FUNCTION__ );
		return RMF_OSAL_ENODATA;
	}

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
			 "%s : podJavaServerStart started successfully on port  %d\n", 
			 __FUNCTION__, pod_server_cmd_port_java_no );
	pPODIPCJAVAServer->registerCallBk( &recv_pod_java_cmd, ( void * ) pPODIPCJAVAServer );
	return RMF_SUCCESS;
}

void recv_pod_java_cmd( IPCServerBuf * pServerBuf, void *pUserData )
{
	rmf_Error ret = RMF_OSAL_ENODATA;
	uint32_t sessionId = 0, apduTag = 0;
	int32_t length = 0;
	uint16_t version;
	uint32_t identifyer = 0;
	int i=0;
	uint8_t *appId = 0;
	uint8_t *apdu = NULL;
	recvAPDU *stRecvAPDU = NULL;
	identifyer = pServerBuf->getCmd(  );
	IPCServer *pPODIPCServer = ( IPCServer * ) pUserData;

	switch ( identifyer )
	{
	 	case  POD_SERVER_CMD_GET_MANUFACTURER_ID:	
		{
			uint16_t manufacturerId;

			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				"%s : IPC called POD_SERVER_CMD_GET_MANUFACTURER_ID \n", __FUNCTION__ );			
			
			ret = podmgrGetManufacturerId( &manufacturerId );
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				"%s : POD_SERVER_CMD_GET_MANUFACTURER_ID manufacturerId from server = %x \n", __FUNCTION__, manufacturerId );	
			
			pServerBuf->addResponse( ret );
			pServerBuf->addData( ( const void * ) &manufacturerId,
										sizeof( manufacturerId ) );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );			
	  	}
		break;

		case POD_SERVER_CMD_STB_REBOOT:    
		{
			int32_t result_recv = 0;
			uint32_t ret = RMF_SUCCESS;
			int32_t lenOfRequestor = 1;
			int32_t lenOfReason = 1;
			char szRequestor [1024];
			char szReason [1024];

			memset(&szRequestor, '\0', 1024);
			memset(&szReason, '\0', 1024);
			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD","%s : IPC called POD_SERVER_CMD_STB_REBOOT \n",__FUNCTION__ );
			result_recv |= pServerBuf->collectData( (void *) &lenOfRequestor,sizeof( int32_t ) );
			result_recv |= pServerBuf->collectData( (void *) &szRequestor, lenOfRequestor );
			result_recv |= pServerBuf->collectData( (void *) &lenOfReason,sizeof( int32_t ) );
			result_recv |= pServerBuf->collectData( (void *) &szReason, lenOfReason );
			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD","%s : Requestor=%s,Reason=%s\n",
										__FUNCTION__, szRequestor, szReason);
			if( result_recv < 0 )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
						  "%s : POD_SERVER_CMD_STB_REBOOT failed due to comm issue: \n", 
						  __FUNCTION__ );

				pServerBuf->addResponse( RMF_OSAL_ENODATA);
				pServerBuf->sendResponse(  );				
				pPODIPCServer->disposeBuf( pServerBuf );				
				break;
			}
			HAL_SYS_Reboot(szRequestor,szReason);
			pServerBuf->addResponse( ret );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
		}
		break;	

	  	case POD_SERVER_CMD_SAS_CONNECT:
		{
			uint32_t result_recv = 0;
			uint16_t dataLen = 0;
			uint64_t appIdTemp = 0;
			uint8_t loopCount = TOTAL_SAS_HANDLERS;
			bool foundCache = false;
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
					"%s : rmf_podmgrSASConnect is called \n", __FUNCTION__ );
			dataLen = pServerBuf->getDataLen( );

			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD", "Data length = %d\n",			
			dataLen );
			if( dataLen != APPID_LEN )
			{
				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
						"%s : rmf_podmgrSASConnect id length is not as expected %d\n", 
						__FUNCTION__, dataLen);
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
				pServerBuf->sendResponse(  );
				pPODIPCServer->disposeBuf( pServerBuf );
				break;					
			}
			rmf_osal_memAllocP( RMF_OSAL_MEM_POD, dataLen,
								( void ** ) &appId );
			if ( appId == NULL )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					"%s: Failed to allocate memory for appId !!\n", __FUNCTION__ );
				/* send -1 response */
				pPODIPCServer->disposeBuf( pServerBuf );
				break;
			}

			result_recv = pServerBuf->collectData( appId, dataLen );
			if( 0 != result_recv )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s -Unexpected data for %d cmd in the buffer %d \n",
							__FUNCTION__, identifyer, result_recv );
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
				pServerBuf->sendResponse(  );
				pPODIPCServer->disposeBuf( pServerBuf );
				rmf_osal_memFreeP( RMF_OSAL_MEM_POD, appId );
				break;
			}		 
			memcpy(&appIdTemp, appId, sizeof(appIdTemp) );
			/* to check whether the SAS reset happened */
			if(podmgrSASResetStat( FALSE, NULL))
			{
				rmf_osal_Bool resetStatus = FALSE;
				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
					"%s: Reseting the cached SAS sessionIds\n",__FUNCTION__);
				memset((void *)sessionCache, 0, sizeof(sessionCache));
				podmgrSASResetStat( TRUE, &resetStatus );
			}
			else
			{
				for(;loopCount--;)
				{
					if(sessionCache[loopCount].appId == appIdTemp)
					{
						foundCache = true;
						sessionId = sessionCache[loopCount].sessionId;
						RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
							"%s: Found previous sessionId in cache sessionId = %d for appId = %llx\n",
							__FUNCTION__, sessionId, appIdTemp);
						break;
					}
				}
			}

			if(false == foundCache )
			{
				ret = rmf_podmgrSASConnect( appId, &sessionId, &version );						
				if ( RMF_SUCCESS != ret )
				{
					RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
						"%s: rmf_podmgrSASConnect failed",__FUNCTION__);
				}
				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
					"%s: Allocating new sessionId = %d for appId = %llx\n",
					__FUNCTION__, sessionId, appIdTemp);

				for(loopCount = TOTAL_SAS_HANDLERS;loopCount--;)
				{
					if(sessionCache[loopCount].appId == 0)
					{
						sessionCache[loopCount].sessionId = sessionId;						
						sessionCache[loopCount].appId= appIdTemp;
						break;
					}
					RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
						"%s: allocated sessionId = %d for appId = %llx\n",
					__FUNCTION__, sessionCache[loopCount].sessionId, sessionCache[loopCount].appId);
				}
			}

			pServerBuf->addResponse( identifyer );
			pServerBuf->addData( ( const void * ) &sessionId,
			sizeof( sessionId ) );
			pServerBuf->sendResponse(  );
			
			pPODIPCServer->disposeBuf( pServerBuf );
			rmf_osal_memFreeP( RMF_OSAL_MEM_POD, appId );
		}
		break;

	 	case POD_SERVER_CMD_SEND_APDU:
	  	{
			uint32_t result_recv = 0;
			
			RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.POD",
				"%s : rmf_podmgrSendAPDU is called: \n", __FUNCTION__ );

			pServerBuf->collectData( ( void * ) &sessionId,
				sizeof( sessionId ) );
			pServerBuf->collectData( ( void * ) &apduTag, sizeof( apduTag ) );
			pServerBuf->collectData( ( void * ) &length, sizeof( length ) );
			rmf_osal_memAllocP( RMF_OSAL_MEM_POD, length, ( void ** ) &apdu );
			if ( apdu == NULL )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					"%s : Error !! Failed to allocate memory for apdu !!\n",
					__FUNCTION__ );
				/*  send -1 response */
				pServerBuf->addResponse( identifyer );
				pServerBuf->sendResponse(  );
				pPODIPCServer->disposeBuf( pServerBuf );
				break;
			}

			result_recv = pServerBuf->collectData( apdu, length );
			if( 0 != result_recv )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s -Unexpected data for %d cmd in the buffer %d \n",
					__FUNCTION__, identifyer, result_recv );
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
				pServerBuf->sendResponse(  );
				pPODIPCServer->disposeBuf( pServerBuf );
				rmf_osal_memFreeP( RMF_OSAL_MEM_POD, apdu );
				break;
			}
			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
				"%s : sessionId = %u, apduTag = 0x%x, length = %u\n",
				__FUNCTION__, sessionId, apduTag, length );

			ret = rmf_podmgrSendAPDU( sessionId, apduTag, length, apdu );
			if ( RMF_SUCCESS != ret )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				"%s : Send APDU failed\n",
				__FUNCTION__ );
			}
			pServerBuf->addResponse( identifyer );
			pServerBuf->sendResponse(  );
			rmf_osal_memFreeP( RMF_OSAL_MEM_POD, apdu );
			pPODIPCServer->disposeBuf( pServerBuf );
	  	}
		break;
		
	  	case POD_SERVER_CMD_RCV_APDU:
		{
			RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.POD",
				"%s : IPC called POD_SERVER_CMD_RCV_APDU \n", __FUNCTION__ );	  	

			rmf_osal_memAllocP( RMF_OSAL_MEM_POD, sizeof( recvAPDU ), ( void ** ) &stRecvAPDU );
			stRecvAPDU->pIPCServer = pPODIPCServer;
			stRecvAPDU->pServerBuf = pServerBuf;
			if ( ( ret =
				 rmf_osal_threadCreate( thread_recv_apdu,
										( void * ) ( stRecvAPDU ),
										RMF_OSAL_THREAD_PRIOR_DFLT,
										RMF_OSAL_THREAD_STACK_SIZE,
										&recv_apduThreadId,
										"canhThread" ) ) != RMF_SUCCESS )
			{
			  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					   "%s: recv_canh_cmd: failed to create recv_apdu thread (err=%d).\n",
					   __FUNCTION__, ret );
			}

			break;
	  	}
	}
}

/* Seperate server for java ends */

rmf_Error podServerStart(	)
{
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
			 "%s : is called \n", __FUNCTION__ );
	const char *server_port;
	server_port = rmf_osal_envGet( "POD_SERVER_CMD_PORT_NO" );
	if( NULL == server_port)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s: NULL POINTER  received for name string\n", __FUNCTION__ );
		pod_server_cmd_port_no = POD_SERVER_CMD_DFLT_PORT_NO;
	}
	else
	{
		pod_server_cmd_port_no = atoi( server_port );
	}
	pPODIPCServer = new IPCServer( ( int8_t * ) "podServer", pod_server_cmd_port_no );
	int8_t ret = pPODIPCServer->start(  );

	if ( 0 != ret )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s : Server failed to start\n",
				 __FUNCTION__ );
		return RMF_OSAL_ENODATA;
	}

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
			 "%s : Server server1 started successfully\n", __FUNCTION__ );
	pPODIPCServer->registerCallBk( &recv_pod_cmd, ( void * ) pPODIPCServer );

	podJavaServerStart( );

#ifdef ENABLE_TIME_UPDATE
#if 0
	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","%s:: creating semaphore, g_pMonitorThreadStopSem\n", __FUNCTION__);
	g_pMonitorThreadStopSem = new sem_t;

	ret  = sem_init( g_pMonitorThreadStopSem, 0 , 0);
	if (0 != ret )
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD",
				"%s - sem_init failed.\n", __FUNCTION__);
	    podServerStop();
            return RMF_OSAL_ENODATA;
	}

	if(RMF_SUCCESS != rmf_osal_eventqueue_create(( const uint8_t* ) "OobSIEventHandler", &eOobSIQhandle) )
        {
            eOobSIQhandle = 0;
            RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
                                    "<%s> rmf_osal_eventqueue_create failed \n",
                                    __FUNCTION__ );
	    podServerStop();
	    return RMF_OSAL_ENODATA;
        }

	if ( ( ret =
				rmf_osal_threadCreate( oobSIEventHandlerThread, (void *)eOobSIQhandle,
					RMF_OSAL_THREAD_PRIOR_DFLT,
					RMF_OSAL_THREAD_STACK_SIZE,
					&OobSiEventThreadId,
					"oobSIEventHandlerThread" ) ) !=
			RMF_SUCCESS )
	{
	    RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				"%s: failed to create oobsi event handler thread (err=%d).\n",
				__FUNCTION__, ret );
            rmf_osal_eventqueue_delete(eOobSIQhandle);
            eOobSIQhandle = 0;
	    podServerStop();
            return RMF_OSAL_ENODATA;
	}

	g_eOobSiThreadStatus = OOBSI_THREAD_ACTIVE;

	rmf_OobSiMgr *pOobSiMgr =  rmf_OobSiMgr::getInstance();
        if ( NULL == pOobSiMgr )
        {
            RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s - rmf_OobSiMgr::getInstance() failed.\n", __FUNCTION__);
            rmf_osal_eventqueue_delete(eOobSIQhandle);
            eOobSIQhandle = 0;
	    podServerStop();
            return RMF_OSAL_ENODATA;
        }

	if(RMF_SUCCESS != pOobSiMgr->RegisterForSIEvents(RMF_SI_EVENT_STT_TIMEZONE_SET|RMF_SI_EVENT_STT_SYSTIME_SET, eOobSIQhandle))
        {
            RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s - rmf_OobSiMgr::RegisterForSIEvents() failed.\n", __FUNCTION__);
            rmf_osal_eventqueue_delete(eOobSIQhandle);
            eOobSIQhandle = 0;
	    podServerStop();
            return RMF_OSAL_ENODATA;
        }

#endif
#endif

	return RMF_SUCCESS;
}

rmf_Error podServerStop(  )
{
	int8_t ret = 0;

#ifdef ENABLE_TIME_UPDATE
#if 0
    rmf_osal_event_params_t event_params = {0};
    rmf_osal_event_handle_t eventHandle;
    rmf_Error retOsal = RMF_SUCCESS;
    uint32_t siEvent = POD_SERVER_CMD_UNREGISTER_CLIENT;

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: Enter \n", __FUNCTION__);

    if(g_eOobSiThreadStatus == OOBSI_THREAD_ACTIVE)
    {
	event_params.data = 0;
	event_params.data_extension = 0;

	retOsal = rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_OOBSI, siEvent,
	           &event_params, &eventHandle);

        if(retOsal != RMF_SUCCESS)
        {
	    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","<%s: %s>: Cannot create RMF OSAL Event \n", SIMODULE, __FUNCTION__);
	    return RMF_OSAL_ENODATA;
        }

        retOsal = rmf_osal_eventqueue_add(eOobSIQhandle, eventHandle);
        if(retOsal != RMF_SUCCESS)
	{
	    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","<%s: %s>: Cannot add event to eOobSIQhandle\n", SIMODULE, __FUNCTION__);
	    rmf_osal_event_delete(eventHandle);
	    return RMF_OSAL_ENODATA;
	}

        ret = sem_wait( g_pMonitorThreadStopSem);
	if (0 != ret )
	{
	    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD",
			    "%s: sem_wait failed.\n", __FUNCTION__);
	}
    }

    if(g_pMonitorThreadStopSem != NULL)
    {
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","%s: Deleting pMonitorThreadStopSem\n", __FUNCTION__);
        ret = sem_destroy( g_pMonitorThreadStopSem);
        if (0 != ret )
        {
	    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD",
			    "%s: sem_destroy failed.\n", __FUNCTION__);
        }

        delete g_pMonitorThreadStopSem;
    }
#endif
#endif

        ret = pPODIPCServer->stop(	);

	if ( 0 != ret )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s : Server failed to stop \n",
				 __FUNCTION__ );
		return RMF_OSAL_ENODATA;
	}

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
			 "%s : podServerStop successfully\n", __FUNCTION__ );
	return RMF_SUCCESS;
}

rmf_Error updateTimeFromExtSource(unsigned short nYear, 
				unsigned short nMonth, 
				unsigned short nDay, 
				unsigned short nHour, 
				unsigned short nMin, 
				unsigned short nSec )
{
       time_t tStb;
	struct tm stmUTC;	   
	char strTimeCmd[ 128 ];
	RMF_SI_DIAG_INFO Diagnostics;
	rmf_OobSiCache *oobSiCache = rmf_OobSiCache::getSiCache();
	int bPostDocsisTime = 0 ;
	int bUpdateFrontPanel  = 1;
	
	oobSiCache->GetSiDiagnostics( &Diagnostics );
	if( Diagnostics.g_siSTTReceived == 1 )
	{
		return RMF_SUCCESS;
	}
	
	snprintf(strTimeCmd, sizeof(strTimeCmd), "date -u -s \"%04d-%02d-%02d %02d:%02d:%02d\"", nYear, nMonth, nDay, nHour, nMin, nSec);
	system(strTimeCmd);	
	
	const char *fpVar  = rmf_osal_envGet("FEATURE.DSG_BROKER.POST_DOCSIS_TIME_TO_FP");	
	if (fpVar != NULL)
	{
		bUpdateFrontPanel = (strcasecmp(fpVar, "TRUE") == 0);
	}
	
	if(bUpdateFrontPanel)
	{
		const char * pszTimeZoneStr = NULL;
		if(NULL != (pszTimeZoneStr = vlPodGetTimeZoneString()))
		{
			if(strlen(pszTimeZoneStr) > 0)
			{
				const char * pszScratchFile = "/tmp/.fp_time_scratch";
				snprintf(strTimeCmd, sizeof(strTimeCmd), "TZ=\"%s\" date > %s", pszTimeZoneStr, pszScratchFile);
				system(strTimeCmd);
				FILE * fp = fopen(pszScratchFile, "r");
				if(NULL != fp)
				{
					char szTmp[32];
					int nFpHour = 0;
					int nFpMin  = 0;
					int nFpSec  = 0;
					if ( fscanf(fp, "%s%s%s%d:%d:%d", szTmp, szTmp, szTmp, &nFpHour, &nFpMin, &nFpSec) <= 0)
					{
						RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s: fscanf returned zero or negative value.\n", __FUNCTION__);
					}
					fclose(fp);
/* Front panel update: */
#if 0
					mpe_FpBlinkSpec     blinkSpec;
					mpe_FpScrollSpec    scrollSpec;
					vlMemSet(&blinkSpec , 0, sizeof(blinkSpec ), sizeof(blinkSpec));
					vlMemSet(&scrollSpec, 0, sizeof(scrollSpec), sizeof(scrollSpec));
					int brightness = vl_mpeos_fpGetBrightness(VL_MPEOS_FP_INDICATOR_INDEX_TEXT);
					vl_mpeos_fpUpdateTime(nFpHour, nFpMin, nFpSec, 0);
					vl_mpeos_fpSetTime(MPE_FP_TEXT_MODE_CLOCK_12HOUR, 0, NULL, 0, brightness, blinkSpec, scrollSpec);
					vl_mpeos_fpEnableTimeDisplay(1);
#endif					
					RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "%s: DSG BROKER: Updating Front Panel with time '%02d:%02d:%02d' for time-zone '%s'.\n", __FUNCTION__, nFpHour, nFpMin, nFpSec, pszTimeZoneStr);
				}
				else
				{
					RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "%s: DSG BROKER: Could not open '%s'.\n", __FUNCTION__, pszScratchFile);
				}
			}
			else
			{
				RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "%s: DSG BROKER: Not updating Front Panel with time as time-zone is not available yet.\n", __FUNCTION__);
			}
		}
		else
		{
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "%s: DSG BROKER: Not updating Front Panel with time as time-zone is not available yet.\n", __FUNCTION__);
		}
	}

	const char *sttVar  = rmf_osal_envGet("FEATURE.DSG_BROKER.POST_DOCSIS_TIME_TO_OCAP");	
	if (sttVar != NULL)
	{
		bPostDocsisTime = (strcasecmp(sttVar, "TRUE") == 0);
	}

	if(bPostDocsisTime)
	{
		oobSiCache->SetGlobalState( SI_STT_ACQUIRED );
	}
	time(&tStb);
	gmtime_r(&tStb, &stmUTC);
	stmUTC.tm_year += 1900;

	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "%s: Updated UTC time for STB is now \"%04d-%02d-%02d %02d:%02d:%02d\".\n", __FUNCTION__, stmUTC.tm_year, stmUTC.tm_mon+1, stmUTC.tm_mday, stmUTC.tm_hour, stmUTC.tm_min, stmUTC.tm_sec);

	return RMF_SUCCESS;
}

/**
 * g_overrideChannelMap
 * override channel-map data.
 */
vector <Oob_ProgramInfo_t> g_overrideChannelMap;

/**
 * loadOverideProgramInfo
 * loads file with format specified below...
 * -----------------------------------------------------------------------------
 * FORMAT FOR OVERRIDE CHANNEL MAP
 * -----------------------------------------------------------------------------
 * SOURCE-ID      : value in Hex.     e.g. 0x125d. Or 0 for use as a wild entry.
 * FREQUENCY      : value in Decimal. e.g. 195000000.
 * MODULATION     : value in Decimal. e.g. 8 for QAM64, 16 for QAM256.
 * PROGRAM-NUMBER : value in Decimal. e.g. 2.
 * -----------------------------------------------------------------------------
 * Example:
 * -----------------------------------------------------------------------------
 * SOURCE-ID:FREQUENCY:MODULATION:PROGRAM-NUMBER
 * -----------------------------------------------------------------------------
 * 0:183000000:16:1
 * 0:189000000:16:1
 * 0x125d:195000000:16:2
 * -----------------------------------------------------------------------------
 * @param pszOverrideChannelMap path-name of the file in the above format.
 * @return void.
 */
void loadOverideProgramInfo(const char * pszOverrideChannelMap)
{
    if(NULL != pszOverrideChannelMap)
    {
        Oob_ProgramInfo_t overrideProgramInfo;
        char szBuf[256];
        FILE * fp = fopen(pszOverrideChannelMap, "r");
        if(NULL != fp)
        {
            int iLine = 0;
            while(!feof(fp))
            {
                iLine++;
                const char * pszLine = fgets(szBuf, sizeof(szBuf), fp);
                if(NULL != pszLine)
                {
                    int nRet = sscanf(pszLine, "%x:%d:%d:%d", &(overrideProgramInfo.service_handle), &(overrideProgramInfo.carrier_frequency), &(overrideProgramInfo.modulation_mode), &(overrideProgramInfo.prog_num));
                    if(nRet >= 4)
                    {
                        g_overrideChannelMap.push_back(overrideProgramInfo);
                    }
                    else
                    {
                        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s: Could read only %d fields in line %d from '%s'.\n", __FUNCTION__, nRet, iLine, pszOverrideChannelMap);
                    }
                }
                else
                {
                    RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s: Could not read line %d from '%s'.\n", __FUNCTION__, iLine, pszOverrideChannelMap);
                }
            }
            fclose(fp);
            
            if(g_overrideChannelMap.size() > 0)
            {
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s : successfully loaded %d entries from '%s'.\n", __FUNCTION__, g_overrideChannelMap.size(), pszOverrideChannelMap);
            }
        }
        else
        {
            RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s: Could not open '%s'.\n", __FUNCTION__, pszOverrideChannelMap);
        }
    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s: pszOverrideChannelMap is NULL.\n", __FUNCTION__);
    }
}

/**
 * isOverrideProgramInfoAvailable
 * checks for override directive and loads the override program data.
 * @return bool indicates if an override directive was present and if the override program data was loaded successfully.
 */
bool isOverrideProgramInfoAvailable()
{
    static const char * pszOverrideChannelMap = rmf_osal_envGet( "POD_SERVER.OVERRIDE_CHANNEL_MAP" );
    static bool bOverrideLoaded = false;
    static bool bOverrideAvaliable = false;
    if((!bOverrideLoaded) && (!bOverrideAvaliable) && (NULL != pszOverrideChannelMap))
    {
        bOverrideLoaded = true;
        loadOverideProgramInfo(pszOverrideChannelMap);
        if(g_overrideChannelMap.size() > 0)
        {
            bOverrideAvaliable = true;
        }
    }
    
    return bOverrideAvaliable;
}

/**
 * getOverrideProgramInfo
 * attempts to re-map the program information if an override directive is present in rmfconfig.ini
 * @param ret the result of the override operation. The result is unchanged if override was not enabled.
 * @param sourceId the ID of the program being selected.
 * @param pProgramInfo the data structure to be populated with overridden channel data.
 * @return void.
 */
void getOverrideProgramInfo(rmf_Error & ret, uint32_t sourceId, Oob_ProgramInfo_t *pProgramInfo)
{
    static int iWild = 0;
    int i = 0;
    // check for null.
    if(NULL == pProgramInfo) { return; } // do not change ret status.
    
    // perform one time initializations.
    if(false == isOverrideProgramInfoAvailable()) { return; } // do not change ret status.
    
    // search translation map
    for(i = 0; i < g_overrideChannelMap.size(); i++)
    {
        // check if we have a match
        if(sourceId == g_overrideChannelMap[i].service_handle)
        {
            // return remapped data.
            *pProgramInfo = g_overrideChannelMap[i];
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s : ocap://0x%x successfully overridden with mapped-data[%d] (f=%d, m=%d, p=%d).\n", __FUNCTION__, sourceId, i,
                            g_overrideChannelMap[i].carrier_frequency, g_overrideChannelMap[i].modulation_mode, g_overrideChannelMap[i].prog_num);
                            
            { ret = RMF_SUCCESS; return; } // change ret to success.
        }
    }
    
    // search for wild overrides
    for(i = 0; i < g_overrideChannelMap.size(); i++)
    {
        int iWildCheck = ((i+iWild)%(g_overrideChannelMap.size()));
        // check if we have a match
        if(0 == g_overrideChannelMap[iWildCheck].service_handle)
        {
            // return wild data.
            *pProgramInfo = g_overrideChannelMap[iWildCheck];
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s : ocap://0x%x successfully overridden with wild-data[%d] (f=%d, m=%d, p=%d).\n", __FUNCTION__, sourceId, iWildCheck,
                            g_overrideChannelMap[iWildCheck].carrier_frequency, g_overrideChannelMap[iWildCheck].modulation_mode, g_overrideChannelMap[iWildCheck].prog_num);
            // increase our wildness:).
            iWild++;
            // bind the wild entry to this sourceId.
            g_overrideChannelMap[iWildCheck].service_handle = sourceId;
            { ret = RMF_SUCCESS; return; } // change ret to success.
        }
    }
    // return failure
    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s : failed to override ocap://0x%x with any data.\n", __FUNCTION__, sourceId);
    { ret = RMF_SI_NOT_FOUND; return; } // change ret to failure.
}

void recv_pod_cmd( IPCServerBuf * pServerBuf, void *pUserData )
{
	rmf_Error ret = RMF_OSAL_ENODATA;
	uint32_t sessionId = 0, apduTag = 0;
	int32_t length = 0;
	uint16_t version;
	uint32_t identifyer = 0;
	int i=0;
	uint8_t *appId = 0;
	uint8_t *apdu = NULL;
	recvAPDU *stRecvAPDU = NULL;
	
	identifyer = pServerBuf->getCmd(  );
	IPCServer *pPODIPCServer = ( IPCServer * ) pUserData;

	switch ( identifyer )
	{
	  case POD_SERVER_CMD_GET_PROGRAM_INFO:
	  	{
			Oob_ProgramInfo_t PInfo;			
			int32_t result_recv = 0;
			uint32_t decimalSrcId = 0;

			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				"%s : IPC called POD_SERVER_CMD_GET_PROGRAM_INFO \n", __FUNCTION__ );
			
			rmf_OobSiMgr *pOobSiMgr =  rmf_OobSiMgr::getInstance();


			if ( NULL == pOobSiMgr )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s - rmf_OobSiMgr::getInstance() failed.\n", __FUNCTION__);
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
				pServerBuf->sendResponse(  );
				pPODIPCServer->disposeBuf( pServerBuf );
				break;
			}
			
			result_recv = pServerBuf->collectData( (  void * ) &decimalSrcId, sizeof( decimalSrcId ) );		  
			if( 0 != result_recv )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s -Unexpected data for %d cmd in the buffer %d \n",
					__FUNCTION__, identifyer, result_recv);
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
				pServerBuf->sendResponse(  );
				pPODIPCServer->disposeBuf( pServerBuf );
				break;				
			}
			ret = pOobSiMgr->GetProgramInfo(decimalSrcId, &PInfo);
            getOverrideProgramInfo(ret, decimalSrcId, &PInfo);
            
			pServerBuf->addResponse( ret );

			pServerBuf->addData( ( const void * ) &PInfo.carrier_frequency,
										sizeof( PInfo.carrier_frequency ) );
			pServerBuf->addData( ( const void * ) &PInfo.modulation_mode,
										sizeof( PInfo.modulation_mode ) );			
			pServerBuf->addData( ( const void * ) &PInfo.prog_num,
										sizeof( PInfo.prog_num ) );
			pServerBuf->addData( ( const void * ) &PInfo.service_handle,
										sizeof( PInfo.service_handle ) );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
	  	}
		break;
		case POD_SERVER_CMD_GET_PROGRAM_INFO_BY_VCN:
		{
			Oob_ProgramInfo_t PInfo;			
			int32_t result_recv = 0;
			uint32_t decimalVCN = 0;

			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				"%s : IPC called POD_SERVER_CMD_GET_PROGRAM_INFO_BY_VCN \n", __FUNCTION__ );
			
			rmf_OobSiMgr *pOobSiMgr =  rmf_OobSiMgr::getInstance();


			if ( NULL == pOobSiMgr )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s - rmf_OobSiMgr::getInstance() failed.\n", __FUNCTION__);
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
				pServerBuf->sendResponse(  );
				pPODIPCServer->disposeBuf( pServerBuf );
				break;
			}
			
			result_recv = pServerBuf->collectData( (  void * ) &decimalVCN, sizeof( decimalVCN ) );		  
			if( 0 != result_recv )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s -Unexpected data for %d cmd in the buffer %d \n",
					__FUNCTION__, identifyer, result_recv);
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
				pServerBuf->sendResponse(  );
				pPODIPCServer->disposeBuf( pServerBuf );
				break;				
			}
			ret = pOobSiMgr->GetProgramInfoByVCN(decimalVCN, &PInfo);
            
			pServerBuf->addResponse( ret );

			pServerBuf->addData( ( const void * ) &PInfo.carrier_frequency,
										sizeof( PInfo.carrier_frequency ) );
			pServerBuf->addData( ( const void * ) &PInfo.modulation_mode,
										sizeof( PInfo.modulation_mode ) );			
			pServerBuf->addData( ( const void * ) &PInfo.prog_num,
										sizeof( PInfo.prog_num ) );
			pServerBuf->addData( ( const void * ) &PInfo.service_handle,
										sizeof( PInfo.service_handle ) );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );			
		}
		break;
#ifdef USE_PXYSRVC
	  case POD_SERVER_CMD_GET_SYSTEM_IDS:
		{
			int32_t result_recv = 0;
			uint16_t channelMapId = 0;
			uint16_t dacId = 0;
			uint16_t manufacturerId = 0;
			int32_t manufacturer = 0;
			
			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
				"%s : IPC called POD_SERVER_CMD_GET_SYSTEM_IDS \n", __FUNCTION__ );
			
			rmf_OobSiMgr *pOobSiMgr =  rmf_OobSiMgr::getInstance();
			if ( NULL == pOobSiMgr )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s - rmf_OobSiMgr::getInstance() failed.\n", __FUNCTION__);
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
				pServerBuf->sendResponse(  );
				pPODIPCServer->disposeBuf( pServerBuf );
				break;
			}
		
			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD","%s : IPC called POD_SERVER_CMD_GET_CHANNEL_MAPID \n",__FUNCTION__ );

			rmf_Error  ret = podmgrGetManufacturerId( &manufacturerId );
			
			if(RMF_SUCCESS == ret)
			{			
				manufacturer = (manufacturerId & 0xFF00) >> 8;
				if( CARD_TYPE_CISCO == manufacturer )
				{
					cCardManagerIF *pCM = NULL;
					pCM = cCardManagerIF::getInstance();
					if( NULL != pCM )
					{
						if( RMF_SUCCESS != pCM->SNMPGetGenFetVctId( &channelMapId ) )
						{
							channelMapId = 0xFFFF;						
							RDK_LOG( RDK_LOG_INFO,  "LOG.RDK.POD",
								"%s : POD_SERVER_CMD_GET_CHANNEL_MAPID  failed for Cisco\n",__FUNCTION__ );
						}
						else
						{
							RDK_LOG( RDK_LOG_DEBUG,  "LOG.RDK.POD",
								"%s : POD_SERVER_CMD_GET_CHANNEL_MAPID  is 0x%x \n",__FUNCTION__, channelMapId );
						}
					}
				}
				else if( CARD_TYPE_MOTO == manufacturer ) 
				{
					if ( RMF_SUCCESS != pOobSiMgr->GetChannelMapId( &channelMapId ) )
					{
							RDK_LOG( RDK_LOG_INFO,  "LOG.RDK.POD",
								"%s : POD_SERVER_CMD_GET_CHANNEL_MAPID  failed for moto \n",__FUNCTION__ );						
					}
				}
	
				ret = pOobSiMgr->GetDACId( &dacId );
			}

			pServerBuf->addResponse( ret );
			if(RMF_SUCCESS == ret)
			{
				pServerBuf->addData( ( const void * ) &channelMapId,
										sizeof( channelMapId ) );
				pServerBuf->addData( ( const void * ) &dacId,
										sizeof( dacId ) );
			}
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
		}
		break;
#endif

	  case POD_SERVER_CMD_GET_VIRTUAL_CHANNEL_NUMBER:
          {
                        int32_t result_recv = 0;
                        uint32_t decimalSrcId = 0;
			uint16_t virtualChannelNumber = 0;

                        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
                                "%s : IPC called POD_SERVER_CMD_GET_VIRTUAL_CHANNEL_NUMBER \n", __FUNCTION__ );

                        rmf_OobSiMgr *pOobSiMgr =  rmf_OobSiMgr::getInstance();


                        if ( NULL == pOobSiMgr )
                        {
                                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s - rmf_OobSiMgr::getInstance() failed.\n", __FUNCTION__);
                                pServerBuf->addResponse( RMF_OSAL_ENODATA );
                                pServerBuf->sendResponse(  );
                                pPODIPCServer->disposeBuf( pServerBuf );
                                break;
                        }

                     result_recv = pServerBuf->collectData( (  void * ) &decimalSrcId, sizeof( decimalSrcId ) );
			if( 0 != result_recv )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s -Unexpected data for %d cmd in the buffer %d \n",
					__FUNCTION__, identifyer, result_recv );
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
				pServerBuf->sendResponse(  );
				pPODIPCServer->disposeBuf( pServerBuf );
				break;				
			}						
			ret = pOobSiMgr->GetVirtualChannelNumber(decimalSrcId, &virtualChannelNumber);

                        pServerBuf->addResponse( ret );
                        if(RMF_SUCCESS == ret)
                        {
                                pServerBuf->addData( ( const void * ) &virtualChannelNumber,
                                                                                sizeof( virtualChannelNumber ) );
                        }
                        pServerBuf->sendResponse(  );
                        pPODIPCServer->disposeBuf( pServerBuf );
          }
          break;

#ifdef ENABLE_TIME_UPDATE
          case POD_SERVER_CMD_GET_SYSTIME:
          {
                        int32_t result_recv = 0;
			rmf_osal_TimeMillis sysTime;
 
                        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
                                "%s : IPC called POD_SERVER_CMD_GET_SYSTIME \n", __FUNCTION__ );

                        rmf_OobSiMgr *pOobSiMgr =  rmf_OobSiMgr::getInstance();


                        if ( NULL == pOobSiMgr )
                        {
                                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s - rmf_OobSiMgr::getInstance() failed.\n", __FUNCTION__);
                                pServerBuf->addResponse( RMF_OSAL_ENODATA );
                                pServerBuf->sendResponse(  );
                                pPODIPCServer->disposeBuf( pServerBuf );
                                break;
                        }

                        ret = pOobSiMgr->GetSysTime(&sysTime);
                        pServerBuf->addResponse( ret );
                        if(RMF_SUCCESS == ret)
                        {
                                pServerBuf->addData( ( const void * ) &sysTime,
                                                                                sizeof( sysTime ) );
                        }
                        pServerBuf->sendResponse(  );
                        pPODIPCServer->disposeBuf( pServerBuf );
          }
          break;

          case POD_SERVER_CMD_GET_TIMEZONE:
          {
                        int32_t result_recv = 0;
                        int32_t timezoneinhours = 0;
                        int32_t daylightflag = 0;

                        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
                                "%s : IPC called POD_SERVER_CMD_GET_TIMEZONE \n", __FUNCTION__ );
          
                        rmf_OobSiMgr *pOobSiMgr =  rmf_OobSiMgr::getInstance();

          
                        if ( NULL == pOobSiMgr )
                        {
                                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s - rmf_OobSiMgr::getInstance() failed.\n", __FUNCTION__);
                                pServerBuf->addResponse( RMF_OSAL_ENODATA );
                                pServerBuf->sendResponse(  );
                                pPODIPCServer->disposeBuf( pServerBuf );
                                break;
                        }

                        ret = pOobSiMgr->GetTimeZone(&timezoneinhours, &daylightflag);
                        pServerBuf->addResponse( ret );
                        if(RMF_SUCCESS == ret)
                        {
                                pServerBuf->addData( ( const void * ) &timezoneinhours,
                                                                                sizeof( timezoneinhours ) );
                                pServerBuf->addData( ( const void * ) &daylightflag,
                                                                                sizeof( daylightflag ) );
 
                        }
                        pServerBuf->sendResponse(  );
                        pPODIPCServer->disposeBuf( pServerBuf );

          }
          break;
#endif

	  case POD_SERVER_CMD_DUMP_DSG_STAT:
	  	{
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				"%s : IPC called POD_SERVER_CMD_DUMP_DSG_STAT \n",
                                __FUNCTION__ ); 
			vlDsgDumpDsgStats();
			pServerBuf->addResponse( RMF_SUCCESS);
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );			
		}
		case POD_SERVER_CMD_GET_CC_VERSION:
		{
			vlCableCardVersionInfo_t cardVersion;
			memset(&cardVersion, 0, sizeof(cardVersion));
			vlMCardGetCardVersionNumberFromMMI(cardVersion.szCardVersion, sizeof(cardVersion.szCardVersion));
		}
	  	break;
	  case POD_SERVER_CMD_DSG_CHECKANDRECOVER_CONNECTIVITY:
	  	{
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				"%s : IPC called POD_SERVER_CMD_DSG_CHECKANDRECOVER_CONNECTIVITY \n",
                                __FUNCTION__ ); 

			vlDsgCheckAndRecoverConnectivity();
			pServerBuf->addResponse( RMF_SUCCESS);
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );			
		}
	  	break;

	  case POD_SERVER_CMD_CDL_UPGRADE_TO_IMAGE:
	  	{
                        int32_t result_recv = 0;
                        VLMPEOS_CDL_IMAGE_TYPE eImageType;
                        VLMPEOS_CDL_IMAGE_SIGN eImageSign;
                        int bRebootNow;
                        int ImageNameLen=0;
                        char * pszImagePathName;

			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				"%s : IPC called POD_SERVER_CMD_DUMP_CDL_UPGRADE_TO_IMAGE \n",
                                __FUNCTION__ ); 

                        result_recv |= pServerBuf->collectData( (  void * ) &eImageType, sizeof( eImageType ) );
                        result_recv |= pServerBuf->collectData( (  void * ) &eImageSign, sizeof( eImageSign ) );
                        result_recv |= pServerBuf->collectData( (  void * ) &bRebootNow, sizeof( bRebootNow ) );
                        result_recv |= pServerBuf->collectData( (  void * ) &ImageNameLen, sizeof( ImageNameLen ) );

                        if ( result_recv < 0 )
                        {
                                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
                                        "%s : Error !! Failed to collect data for POD_SERVER_CMD_CDL_UPGRADE_TO_IMAGE!!\n",
                                        __FUNCTION__ );
                                /*  send -1 response */
                                pServerBuf->addResponse( RMF_OSAL_ENODATA );
                                pServerBuf->sendResponse(  );
                                pPODIPCServer->disposeBuf( pServerBuf );
                                break;
                        }


                        RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
                                        "%s : ImageNameLen: %d\n",
                                        __FUNCTION__, ImageNameLen );
                        rmf_osal_memAllocP( RMF_OSAL_MEM_POD, ImageNameLen+1, ( void ** ) &pszImagePathName );
                        if ( pszImagePathName == NULL )
                        {
                                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
                                        "%s : Error !! Failed to allocate memory for image name !!\n",
                                        __FUNCTION__ );
                                /*  send -1 response */
                                pServerBuf->addResponse( RMF_OSAL_ENODATA );
                                pServerBuf->sendResponse(  );
                                pPODIPCServer->disposeBuf( pServerBuf );
                                break;
                        }
                         memset( pszImagePathName, 0 , (ImageNameLen+1) );

                        result_recv = pServerBuf->collectData( (void *) pszImagePathName, ImageNameLen );
                        if ( result_recv < 0 )
                        {
                                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
                                        "%s : Error !! Failed to collect data for POD_SERVER_CMD_CDL_UPGRADE_TO_IMAGE!!\n",
                                        __FUNCTION__ );
                                rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pszImagePathName);
                                /*  send -1 response */
                                pServerBuf->addResponse( RMF_OSAL_ENODATA );
                                pServerBuf->sendResponse(  );
                                pPODIPCServer->disposeBuf( pServerBuf );
                                break;
                        }

			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				"%s : IPC called POD_SERVER_CMD_DUMP_CDL_UPGRADE_TO_IMAGE. Invoking vlMpeosCdlUpgradeToImage with imageName: %s\n",
                                __FUNCTION__, pszImagePathName );
			vlMpeosCdlUpgradeToImage(eImageType, eImageSign, bRebootNow, (const char*)pszImagePathName);
			pServerBuf->addResponse( RMF_SUCCESS);
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );			
                        //rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pszImagePathName);
		}
	  	break;

	  case POD_SERVER_CMD_START_HOMING_DOWNLOAD:
	  	{
			uint8_t ltsId;
			int32_t status;
			int32_t result_recv = 0;

			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				"%s : IPC called POD_SERVER_CMD_START_HOMING_DOWNLOAD \n", 
				__FUNCTION__ );
			
			result_recv |= pServerBuf->collectData( (  void * ) &ltsId, sizeof( ltsId ) );
			result_recv |= pServerBuf->collectData( (  void * ) &status, sizeof( status) );

			if( result_recv < 0 ) 
			{
				  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
						   "%s : POD_SERVER_CMD_START_HOMING_DOWNLOAD failed due to comm issue: \n", 
						   __FUNCTION__ );
				  break;
			}			
			podmgrStartHomingDownload( ltsId, status );
			pServerBuf->addResponse( RMF_SUCCESS);
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );			
	  	}
	  	break;		
#ifdef USE_IPDIRECT_UNICAST
	  case POD_SERVER_CMD_START_INBAND_TUNE:
                {
			uint8_t ltsId;
			int32_t status;
			int32_t result_recv = 0;

			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				"%s : IPC called POD_SERVER_CMD_START_INBAND_TUNE \n",
				__FUNCTION__ );

			result_recv |= pServerBuf->collectData( (  void * ) &ltsId, sizeof( ltsId ) );
			result_recv |= pServerBuf->collectData( (  void * ) &status, sizeof( status) );

			if( result_recv < 0 )
			{
				  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
						   "%s : POD_SERVER_CMD_START_INBAND_TUNE failed due to comm issue: \n",
						   __FUNCTION__ );
				  break;
			}
			podmgrStartInbandTune( ltsId, status );
			pServerBuf->addResponse( RMF_SUCCESS);
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
                }
                break;
#endif
	  case POD_SERVER_CMD_START_DECRYPT:
  		{
			int32_t result_recv = 0;
			parsedDescriptors ParseOutput;
			decryptRequestParams requestPtr;
			rmf_osal_eventqueue_handle_t queueId;
			rmf_PodSrvDecryptSessionHandle sessionHandlePtr;

			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				"%s : IPC called POD_SERVER_CMD_START_DECRYPT \n", __FUNCTION__ );
			memset( &ParseOutput, 0 , sizeof(ParseOutput) );
			memset( ParseOutput.aComps, 0 , sizeof(ParseOutput.aComps) );		
			memset( &requestPtr, 0, sizeof(requestPtr) );
			requestPtr.pDescriptor = &ParseOutput;
			  
			result_recv |= pServerBuf->collectData( (  void * ) &queueId, sizeof( queueId ) );			
			result_recv |= pServerBuf->collectData( (  void * ) &requestPtr.tunerId, sizeof( requestPtr.tunerId ) );
			result_recv |= pServerBuf->collectData( (  void * ) &requestPtr.ltsId, sizeof( requestPtr.ltsId ) );
			result_recv |= pServerBuf->collectData( (  void * ) &requestPtr.numPids, sizeof( requestPtr.numPids ) );
			rmf_osal_memAllocP( RMF_OSAL_MEM_POD,
									(requestPtr.numPids * sizeof( rmf_MediaPID )),
									( void ** ) &requestPtr.pids );		
			result_recv |= pServerBuf->collectData( (  void * ) requestPtr.pids, (requestPtr.numPids * sizeof( rmf_MediaPID )) );
			result_recv |= pServerBuf->collectData( (  void * ) &requestPtr.pDescriptor->prgNo, sizeof( requestPtr.pDescriptor->prgNo ) );
			result_recv |= pServerBuf->collectData( (  void * ) &requestPtr.pDescriptor->srcId, sizeof( requestPtr.pDescriptor->srcId ) );
			result_recv |= pServerBuf->collectData( (  void * ) &requestPtr.pDescriptor->ecmPid, sizeof( requestPtr.pDescriptor->ecmPid ) );
			result_recv |= pServerBuf->collectData( (  void * ) &requestPtr.pDescriptor->ecmPidFound, sizeof( requestPtr.pDescriptor->ecmPidFound ) );
			result_recv |= pServerBuf->collectData( (  void * ) &requestPtr.pDescriptor->caDescFound, sizeof( requestPtr.pDescriptor->caDescFound ) );
			result_recv |= pServerBuf->collectData( (  void * ) &requestPtr.pDescriptor->noES, sizeof( requestPtr.pDescriptor->noES ) );

			rmf_osal_memAllocP( RMF_OSAL_MEM_POD,
									requestPtr.pDescriptor->noES * sizeof(rmf_PodmgrStreamDecryptInfo),
									( void ** ) &requestPtr.pDescriptor->pESInfo );	
			
			result_recv |= pServerBuf->collectData( (  void * ) requestPtr.pDescriptor->pESInfo, requestPtr.pDescriptor->noES * sizeof(rmf_PodmgrStreamDecryptInfo));

			result_recv |= pServerBuf->collectData( (  void * ) &requestPtr.pDescriptor->ProgramInfoLen, sizeof( requestPtr.pDescriptor->ProgramInfoLen ) );

			rmf_osal_memAllocP( RMF_OSAL_MEM_POD,
									requestPtr.pDescriptor->ProgramInfoLen * sizeof(uint8_t) ,
									( void ** ) &requestPtr.pDescriptor->pProgramInfo );		

			result_recv |= pServerBuf->collectData( (  void * ) requestPtr.pDescriptor->pProgramInfo, requestPtr.pDescriptor->ProgramInfoLen * sizeof(uint8_t) );
			result_recv |= pServerBuf->collectData( (  void * ) &requestPtr.pDescriptor->streamInfoLen, sizeof( requestPtr.pDescriptor->streamInfoLen ) );

			rmf_osal_memAllocP( RMF_OSAL_MEM_POD,
									requestPtr.pDescriptor->streamInfoLen * sizeof(uint8_t) ,
									( void ** ) &requestPtr.pDescriptor->pStreamInfo );		

			result_recv |= pServerBuf->collectData( (  void * ) requestPtr.pDescriptor->pStreamInfo, requestPtr.pDescriptor->streamInfoLen * sizeof(uint8_t) );
			result_recv |= pServerBuf->collectData( (  void * ) requestPtr.pDescriptor->aComps, sizeof(requestPtr.pDescriptor->aComps) );
			result_recv |= pServerBuf->collectData( (  void * ) &requestPtr.pDescriptor->CAPMTCmdIdIndex, sizeof( requestPtr.pDescriptor->CAPMTCmdIdIndex ) );
			result_recv |= pServerBuf->collectData( (  void * ) &requestPtr.priority, sizeof( requestPtr.priority ) );
			result_recv |= pServerBuf->collectData( (  void * ) &requestPtr.mmiEnable, sizeof( requestPtr.mmiEnable ) );
			result_recv =  pServerBuf->collectData( (  void * ) &requestPtr.qamContext, sizeof( requestPtr.qamContext ) );
			if( 0 != result_recv )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s -Unexpected data for %d cmd in the buffer %d \n",
					__FUNCTION__, identifyer, result_recv );
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
				pServerBuf->sendResponse(  );
				pPODIPCServer->disposeBuf( pServerBuf );
				break;
			}
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				   "%s : POD_SERVER_CMD_START_DECRYPT recv rmfqam handle %d: \n", 
				   __FUNCTION__, requestPtr.qamContext );

			ret = podmgrStartDecrypt(
						&requestPtr,
						queueId, 
						&sessionHandlePtr);
			
			if ( requestPtr.pDescriptor->pESInfo )
			{
				rmf_osal_memFreeP( RMF_OSAL_MEM_POD,
								   requestPtr.pDescriptor->pESInfo );
				requestPtr.pDescriptor->pESInfo = NULL;
			}
			
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				   "%s : POD_SERVER_CMD_START_DECRYPT session handle is %x: \n", 
				   __FUNCTION__, sessionHandlePtr );

			if ( requestPtr.pDescriptor->pStreamInfo )
			{
				rmf_osal_memFreeP( RMF_OSAL_MEM_POD, requestPtr.pDescriptor->pStreamInfo );
				requestPtr.pDescriptor->pStreamInfo = NULL;
			}

			if ( requestPtr.pDescriptor->pProgramInfo )
			{
				rmf_osal_memFreeP( RMF_OSAL_MEM_POD, requestPtr.pDescriptor->pProgramInfo );
				requestPtr.pDescriptor->pProgramInfo = NULL;
			}

			rmf_osal_memFreeP( RMF_OSAL_MEM_POD,
								   requestPtr.pids );
			pServerBuf->addResponse( ret );
			pServerBuf->addData( ( const void * ) &sessionHandlePtr,
										sizeof( sessionHandlePtr ) );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
	  	}
		break;

	  case POD_SERVER_CMD_GET_CCI_BIT:
		{
			rmf_PodSrvDecryptSessionHandle sessionHandle;			
			int32_t result_recv = 0;
			uint8_t CCIBit = 0;

			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				"%s : IPC called POD_SERVER_CMD_GET_CCI_BIT \n", __FUNCTION__ );
			
			result_recv = pServerBuf->collectData( (  void * ) &sessionHandle, sizeof( sessionHandle ) );		  
			if( 0 != result_recv )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s -Unexpected data for %d cmd in the buffer %d \n",
					__FUNCTION__, identifyer, result_recv );
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
				pServerBuf->sendResponse(  );
				pPODIPCServer->disposeBuf( pServerBuf );
				break;				
			}	
			ret = podmgrGetCCIBits(sessionHandle, &CCIBit);
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				"%s : POD_SERVER_CMD_GET_CCI_BIT CCI = %x\n", __FUNCTION__, CCIBit );

			pServerBuf->addResponse( ret );
			pServerBuf->addData( ( const void * ) &CCIBit,
										sizeof( CCIBit ) );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
	  	}
		break;
	  	
	  case POD_SERVER_CMD_STOP_DECRYPT:
		{
			rmf_PodSrvDecryptSessionHandle sessionHandle;			
			int32_t result_recv = 0;

			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				"%s : IPC called POD_SERVER_CMD_STOP_DECRYPT \n", __FUNCTION__ );
					
			result_recv = pServerBuf->collectData( (  void * ) &sessionHandle, sizeof( sessionHandle ) );		  
			if( 0 != result_recv )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s -Unexpected data for %d cmd in the buffer %d \n",
					__FUNCTION__, identifyer, result_recv );
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
				pServerBuf->sendResponse(  );
				pPODIPCServer->disposeBuf( pServerBuf );
				break;				
			}	
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				   "%s : POD_SERVER_CMD_STOP_DECRYPT session handle is %x: \n", 
				   __FUNCTION__, sessionHandle );	
			
			ret = podmgrStopDecrypt(sessionHandle);

			pServerBuf->addResponse( ret );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
	  	}
		break;
	  case POD_SERVER_CMD_GET_CASYSTEMID:
		{			
			unsigned short CaSystemId;

			RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.POD",
				"%s : IPC called POD_SERVER_CMD_GET_CASYSTEMID \n", __FUNCTION__ );			
			
			ret = podImplGetCaSystemId( &CaSystemId );
			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
				"%s : POD_SERVER_CMD_GET_CASYSTEMID CaSystemId from server = %x \n", __FUNCTION__, CaSystemId );	
			
			pServerBuf->addResponse( ret );
			pServerBuf->addData( ( const void * ) &CaSystemId,
										sizeof( CaSystemId ) );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
	  	}
		break;
		
	  case  POD_SERVER_CMD_GET_MANUFACTURER_ID:	
		{
			uint16_t manufacturerId;

			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				"%s : IPC called POD_SERVER_CMD_GET_MANUFACTURER_ID \n", __FUNCTION__ );			
			
			ret = podmgrGetManufacturerId( &manufacturerId );
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				"%s : POD_SERVER_CMD_GET_MANUFACTURER_ID manufacturerId from server = %x \n", __FUNCTION__, manufacturerId );	
			
			pServerBuf->addResponse( ret );
			pServerBuf->addData( ( const void * ) &manufacturerId,
										sizeof( manufacturerId ) );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );			
	  	}
		break;
		
	  case POD_SERVER_CMD_GETMAX_ELM_STR:    
		{	
			uint16_t MaxElmStr = 0;
			rmf_Error retVal = 0;
			RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.POD",
				"%s : IPC called POD_SERVER_CMD_GETMAX_ELM_STR \n", __FUNCTION__ );	  
			
			vlMpeosPodGetParam( RMF_PODMGR_PARAM_ID_MAX_NUM_ES, &MaxElmStr );
			if( RMF_SUCCESS == retVal  )
			{
				pServerBuf->addResponse( RMF_SUCCESS );
				pServerBuf->addData( ( const void * ) &MaxElmStr,
								   sizeof( MaxElmStr ) );
			}
			else
			{
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
			}
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );						
		}
		break;		  
		
	  case POD_SERVER_CMD_CONFIG_CIPH:
	  	{
			rmf_Error ret = RMF_SUCCESS;
			int32_t result_recv = 0;
			unsigned char ltsid;
			unsigned short PrgNum;
			UINT32 AdecodePid[100];
			UINT32 numpids;
			
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				"%s : IPC called POD_SERVER_CMD_CONFIG_CIPH \n", __FUNCTION__ );

			result_recv |= pServerBuf->collectData( ( void * ) &ltsid, sizeof( ltsid ) );
			result_recv |= pServerBuf->collectData( ( void * ) &PrgNum, sizeof( PrgNum ) );
			result_recv |= pServerBuf->collectData( ( void * ) &numpids, sizeof( numpids ) );
			result_recv = pServerBuf->collectData( ( void * ) AdecodePid, (numpids * sizeof( UINT32 ))  );
			if( 0 != result_recv )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s -Unexpected data for %d cmd in the buffer %d \n",
					__FUNCTION__, identifyer, result_recv );
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
				pServerBuf->sendResponse(  );
				pPODIPCServer->disposeBuf( pServerBuf );
				break;				
			}				
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				"%s : IPC POD_SERVER_CMD_CONFIG_CIPH ltsid =%x, PrgNum = %x \n", 
								__FUNCTION__,  ltsid, PrgNum);

			for(i=numpids;i--;) 
			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
				"%s : IPC POD_SERVER_CMD_CONFIG_CIPH AdecodePid[%d] = %x \n", 
								__FUNCTION__, i, AdecodePid[i]);				
			
			ret =podmgrConfigCipher(ltsid, PrgNum, AdecodePid, numpids);
			pServerBuf->addResponse( ret );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
	  	}
	  	break;

         case POD_SERVER_CMD_HB:
		 {
			RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.POD",
				"%s : IPC called POD_SERVER_CMD_HB \n", __FUNCTION__ );
			
			pServerBuf->addResponse( RMF_SUCCESS );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
		 }
		 break;
         case POD_SERVER_CMD_IS_READY:
		 {
			RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.POD",
				"%s : IPC called POD_SERVER_CMD_IS_READY \n", __FUNCTION__ );
			
		 	ret = podmgrIsReady();
			pServerBuf->addResponse( ret );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
		 }
		 break;
         case POD_SERVER_CMD_REGISTER_CLIENT:
		 {
			rmf_osal_eventqueue_handle_t eQhandle;

			RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.POD",
				"%s : IPC called POD_SERVER_CMD_REGISTER_CLIENT \n", __FUNCTION__ );

			if(RMF_SUCCESS != rmf_osal_eventqueue_create(( const uint8_t* ) "podServer", &eQhandle) )
			{			
				eQhandle = 0;
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
						 "<%s> rmf_osal_eventqueue_create failed \n",
						 __FUNCTION__ );				
			}
			else
			{
				if(RMF_SUCCESS != podmgrRegisterEvents( eQhandle ) )
				{

					RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
						 "<%s> rmf_podmgrRegisterEvents failed \n",
						 __FUNCTION__ );				

					rmf_osal_eventqueue_delete(eQhandle );
					eQhandle = 0;
				}
#ifdef ENABLE_POD_CORE_EVENTS
			      	if(RMF_SUCCESS != rmf_osal_eventmanager_register_handler(
					get_pod_event_manager(),
					eQhandle,
					RMF_OSAL_EVENT_CATEGORY_CARD_MANAGER))
				{

					RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
						 "<%s> RMF_OSAL_EVENT_CATEGORY_CARD_MANAGER failed \n",
						 __FUNCTION__ );				

					rmf_osal_eventqueue_delete(eQhandle );
					eQhandle = 0;
				}
#endif				
#ifdef ENABLE_TIME_UPDATE
//SS - Change in Design
				rmf_OobSiMgr *pOobSiMgr =  rmf_OobSiMgr::getInstance();
				if ( NULL == pOobSiMgr )
				{
					RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s - rmf_OobSiMgr::getInstance() failed.\n", __FUNCTION__);
					rmf_osal_eventqueue_delete(eQhandle);
					eQhandle = 0;
				}

				if(RMF_SUCCESS != pOobSiMgr->RegisterForSIEvents(RMF_SI_EVENT_STT_TIMEZONE_SET|RMF_SI_EVENT_STT_SYSTIME_SET, eQhandle))
				{
					RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s - rmf_OobSiMgr::RegisterForSIEvents() failed.\n", __FUNCTION__);
					rmf_osal_eventqueue_delete(eQhandle);
					eQhandle = 0;
				}
#endif
				ret = podmgrIsReady();
				if( RMF_SUCCESS == ret )
				{
					rmf_osal_event_handle_t event_handle;
					rmf_osal_event_params_t event_params = {0};
					
					rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_POD, RMF_PODSRV_EVENT_READY, 
					&event_params, &event_handle );
					rmf_osal_eventqueue_add( eQhandle, event_handle);
					RDK_LOG( RDK_LOG_ERROR, 
						"LOG.RDK.POD", "%s - dummy event of podmgrIsReady for qHandle %x\n", 
						__FUNCTION__, eQhandle );
				}
				
			}
			pServerBuf->addResponse( eQhandle );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
		 }
		 break;

         case POD_SERVER_CMD_UNREGISTER_CLIENT:
		{
			int32_t result_recv = 0;
			rmf_osal_eventqueue_handle_t eQhandle;

			RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.POD",
				"%s : IPC called POD_SERVER_CMD_UNREGISTER_CLIENT \n", __FUNCTION__ );
			
			result_recv  = pServerBuf->collectData( ( void * ) &eQhandle, sizeof( eQhandle ) );
			if( 0 != result_recv )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s -Unexpected data for %d cmd in the buffer %d \n",
					__FUNCTION__, identifyer, result_recv );
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
				pServerBuf->sendResponse(  );
				pPODIPCServer->disposeBuf( pServerBuf );
				break;				
			}			
			
			if(RMF_SUCCESS != podmgrUnRegisterEvents( eQhandle ) )
			{
				ret = RMF_OSAL_ENODATA;
			}
			else
			{
#ifdef ENABLE_TIME_UPDATE
				//SS - Change in Design
				rmf_OobSiMgr *pOobSiMgr =  rmf_OobSiMgr::getInstance();
				if ( NULL == pOobSiMgr )
				{
					RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s - rmf_OobSiMgr::getInstance() failed.\n", __FUNCTION__);
					ret = RMF_OSAL_ENODATA;
				}

				if(RMF_SUCCESS != pOobSiMgr->UnRegisterForSIEvents(RMF_SI_EVENT_STT_TIMEZONE_SET|RMF_SI_EVENT_STT_SYSTIME_SET, eQhandle))
				{
					RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s - rmf_OobSiMgr::UnRegisterForSIEvents() failed.\n", __FUNCTION__);
					ret = RMF_OSAL_ENODATA;
				}
#endif
				if( RMF_SUCCESS != rmf_osal_eventmanager_unregister_handler(
					get_pod_event_manager(), eQhandle) )
				{
					RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s - Failed to unregister core events \n", __FUNCTION__);
					ret = RMF_OSAL_ENODATA;
				}

				if(RMF_SUCCESS != rmf_osal_eventqueue_delete( eQhandle ) )
				{
					ret = RMF_OSAL_ENODATA;
				}
				else
				{
					ret = RMF_SUCCESS;
				}
			}

			pServerBuf->addResponse( ret );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );			
		}
		break;

         case POD_SERVER_CMD_REGISTER_SIMGR_CLIENT:
		 {
			rmf_osal_eventqueue_handle_t eQhandle;

			RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.POD",
				"%s : IPC called POD_SERVER_CMD_REGISTER_SIMGR_CLIENT \n", __FUNCTION__ );

			if(RMF_SUCCESS != rmf_osal_eventqueue_create(( const uint8_t* ) "siMgrServer", &eQhandle) )
			{			
				eQhandle = 0;
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
						 "<%s> rmf_osal_eventqueue_create failed \n",
						 __FUNCTION__ );
				
				eQhandle = 0;
			}
			else
			{
			      	if( RMF_SUCCESS != rmf_osal_eventmanager_register_handler(
					get_pod_event_manager(),
					eQhandle,
					RMF_OSAL_EVENT_CATEGORY_SIMGR) )
				{

					RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
						 "<%s> RMF_OSAL_EVENT_CATEGORY_SIMGR failed \n",
						 __FUNCTION__ );				

					rmf_osal_eventqueue_delete( eQhandle );
					eQhandle = 0;
				}
			}
			pServerBuf->addResponse( eQhandle );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
		}
		break;
		
         case POD_SERVER_CMD_UNREGISTER_SIMGR_CLIENT:
		{
			int32_t result_recv = 0;
			rmf_osal_eventqueue_handle_t eQhandle;

			RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.POD",
				"%s : IPC called POD_SERVER_CMD_UNREGISTER_CLIENT \n", __FUNCTION__ );
			
			result_recv  = pServerBuf->collectData( ( void * ) &eQhandle, sizeof( eQhandle ) );
			if( 0 != result_recv )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s -Unexpected data for %d cmd in the buffer %d \n",
					__FUNCTION__, identifyer, result_recv );
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
				pServerBuf->sendResponse(  );
				pPODIPCServer->disposeBuf( pServerBuf );
				break;				
			}

			if( RMF_SUCCESS != rmf_osal_eventmanager_unregister_handler(
				get_pod_event_manager(), eQhandle) )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s - Failed to unregister SIMGR \n", __FUNCTION__);
				ret = RMF_OSAL_ENODATA;
			}

			if(RMF_SUCCESS != rmf_osal_eventqueue_delete( eQhandle ) )
			{
				ret = RMF_OSAL_ENODATA;
			}
			else
			{
				ret = RMF_SUCCESS;
			}

			pServerBuf->addResponse( ret );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );				
		}
		break;
		
         case POD_SERVER_CMD_POLL_SIMGR_EVENT:
		{
			int32_t result_recv = 0;
			rmf_osal_eventqueue_handle_t eQhandle;
			dispatchThreadData *pTdata = NULL;

			RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.POD",
				"%s : IPC called POD_SERVER_CMD_POLL_SIMGR_EVENT \n", __FUNCTION__ );
		
			result_recv = pServerBuf->collectData( ( void * ) &eQhandle, sizeof( eQhandle ) );
			if( 0 != result_recv )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s -Unexpected data for %d cmd in the buffer %d \n",
					__FUNCTION__, identifyer, result_recv );
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
				pServerBuf->sendResponse(  );
				pPODIPCServer->disposeBuf( pServerBuf );
				break;				
			}
						
			if ( RMF_SUCCESS !=
				 rmf_osal_memAllocP( RMF_OSAL_MEM_POD,
									 sizeof( dispatchThreadData ),
									 ( void ** ) &( pTdata ) ) )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
						 "<%s> Mem allocation failed, returning RMF_OUT_OF_MEM...\n",
						 __FUNCTION__ );
				pServerBuf->addResponse( RMF_OSAL_ENODATA);
				pServerBuf->sendResponse(  );				
				pPODIPCServer->disposeBuf( pServerBuf );				
				break;
			}
			pTdata->eQhandle = eQhandle;
			pTdata->pPODIPCServer = pPODIPCServer;
			pTdata->pServerBuf = pServerBuf;
			
			if ( ( ret =
				   rmf_osal_threadCreate( podSIMGREventDispatchThread, (void *)pTdata,
										  RMF_OSAL_THREAD_PRIOR_DFLT,
										  RMF_OSAL_THREAD_STACK_SIZE,
										  &podSIMGREventDispatchThreadId,
										  "podSIMGREventDispatchThread" ) ) !=
				 RMF_SUCCESS )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
						 "%s: failed to create podSIMGREventDispatchThread (err=%d).\n",
						 __FUNCTION__, ret );
			}			
		}
		break;		
         case POD_SERVER_CMD_POLL_EVENT:
		{
			int32_t result_recv = 0;
			rmf_osal_eventqueue_handle_t eQhandle;
			dispatchThreadData *pTdata = NULL;

			RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.POD",
				"%s : IPC called POD_SERVER_CMD_POLL_EVENT \n", __FUNCTION__ );
		
			result_recv = pServerBuf->collectData( ( void * ) &eQhandle, sizeof( eQhandle ) );
			if( 0 != result_recv )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s -Unexpected data for %d cmd in the buffer %d \n",
					__FUNCTION__, identifyer, result_recv );
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
				pServerBuf->sendResponse(  );
				pPODIPCServer->disposeBuf( pServerBuf );
				break;				
			}
			
			
			if ( RMF_SUCCESS !=
				 rmf_osal_memAllocP( RMF_OSAL_MEM_POD,
									 sizeof( dispatchThreadData ),
									 ( void ** ) &( pTdata ) ) )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
						 "<%s> Mem allocation failed, returning RMF_SI_OUT_OF_MEM...\n",
						 __FUNCTION__ );
				pServerBuf->addResponse( RMF_OSAL_ENODATA);
				pServerBuf->sendResponse(  );				
				pPODIPCServer->disposeBuf( pServerBuf );				
				break;
			}
			pTdata->eQhandle = eQhandle;
			pTdata->pPODIPCServer = pPODIPCServer;
			pTdata->pServerBuf = pServerBuf;
			
			if ( ( ret =
				   rmf_osal_threadCreate( podEventDispatchThread, (void *)pTdata,
										  RMF_OSAL_THREAD_PRIOR_DFLT,
										  RMF_OSAL_THREAD_STACK_SIZE,
										  &podEventDispatchThreadId,
										  "podEventDispatchThread" ) ) !=
				 RMF_SUCCESS )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
						 "%s: failed to create pod worker thread (err=%d).\n",
						 __FUNCTION__, ret );
			}			
		}
		break;

	  case POD_SERVER_CMD_RCV_APDU:

		RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.POD",
			"%s : IPC called POD_SERVER_CMD_RCV_APDU \n", __FUNCTION__ );	  	

		  rmf_osal_memAllocP( RMF_OSAL_MEM_POD, sizeof( recvAPDU ), ( void ** ) &stRecvAPDU );
		  stRecvAPDU->pIPCServer = pPODIPCServer;
		  stRecvAPDU->pServerBuf = pServerBuf;
		  if ( ( ret =
				 rmf_osal_threadCreate( thread_recv_apdu,
										( void * ) ( stRecvAPDU ),
										RMF_OSAL_THREAD_PRIOR_DFLT,
										RMF_OSAL_THREAD_STACK_SIZE,
										&recv_apduThreadId,
										"canhThread" ) ) != RMF_SUCCESS )
		  {
			  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					   "%s: recv_canh_cmd: failed to create recv_apdu thread (err=%d).\n",
					   __FUNCTION__, ret );
		  }

		  break;

	  case POD_SERVER_CMD_SEND_APDU:
	  	{
			uint32_t result_recv = 0;
			
			RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.POD",
				"%s : rmf_podmgrSendAPDU is called: \n", __FUNCTION__ );

			pServerBuf->collectData( ( void * ) &sessionId,
				sizeof( sessionId ) );
			pServerBuf->collectData( ( void * ) &apduTag, sizeof( apduTag ) );
			pServerBuf->collectData( ( void * ) &length, sizeof( length ) );
			rmf_osal_memAllocP( RMF_OSAL_MEM_POD, length, ( void ** ) &apdu );
			if ( apdu == NULL )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					"%s : Error !! Failed to allocate memory for apdu !!\n",
					__FUNCTION__ );
				/*  send -1 response */
				pServerBuf->addResponse( identifyer );
				pServerBuf->sendResponse(  );
				pPODIPCServer->disposeBuf( pServerBuf );
				break;
			}

			result_recv = pServerBuf->collectData( apdu, length );
			if( 0 != result_recv )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s -Unexpected data for %d cmd in the buffer %d \n",
					__FUNCTION__, identifyer, result_recv );
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
				pServerBuf->sendResponse(  );
				pPODIPCServer->disposeBuf( pServerBuf );
				rmf_osal_memFreeP( RMF_OSAL_MEM_POD, apdu );
				break;
			}
			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
				"%s : sessionId = %u, apduTag = 0x%x, length = %u\n",
				__FUNCTION__, sessionId, apduTag, length );

			ret = rmf_podmgrSendAPDU( sessionId, apduTag, length, apdu );
			if ( RMF_SUCCESS != ret )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				"%s : Send APDU failed\n",
				__FUNCTION__ );
			}
			pServerBuf->addResponse( identifyer );
			pServerBuf->sendResponse(  );
			rmf_osal_memFreeP( RMF_OSAL_MEM_POD, apdu );
			pPODIPCServer->disposeBuf( pServerBuf );
	  	}
		break;

	  case POD_SERVER_CMD_SAS_CONNECT:
	  	{
		uint32_t result_recv = 0;	  	
		  RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.POD",
				   "%s : rmf_podmgrSASConnect is called \n", __FUNCTION__ );
		  RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD", "Data length = %d\n",
				   pServerBuf->getDataLen(	) );
		  rmf_osal_memAllocP( RMF_OSAL_MEM_POD, pServerBuf->getDataLen(  ),
							  ( void ** ) &appId );
		  if ( appId == NULL )
		  {
			  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					   "%s: Failed to allocate memory for appId !!\n", __FUNCTION__ );
			  /*  send -1 response */
			  pPODIPCServer->disposeBuf( pServerBuf );
			  break;
		  }

		result_recv = pServerBuf->collectData( appId, pServerBuf->getDataLen(  ) );
		if( 0 != result_recv )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s -Unexpected data for %d cmd in the buffer %d \n",
				__FUNCTION__, identifyer, result_recv );
			pServerBuf->addResponse( RMF_OSAL_ENODATA );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
			rmf_osal_memFreeP( RMF_OSAL_MEM_POD, appId );
			break;
		}
		  ret = rmf_podmgrSASConnect( appId, &sessionId, &version );

		  if ( RMF_SUCCESS != ret )
		  {
			  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					   "%s: rmf_podmgrSASConnect failed",__FUNCTION__);
		  }

		  pServerBuf->addResponse( identifyer );
		  pServerBuf->addData( ( const void * ) &sessionId,
							   sizeof( sessionId ) );
		  pServerBuf->sendResponse(  );
		  RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: sessionId = %x \n",
				   __FUNCTION__, sessionId );
		  pPODIPCServer->disposeBuf( pServerBuf );
		  rmf_osal_memFreeP( RMF_OSAL_MEM_POD, appId );
		  }
		  break;
/*Start : SNMP client *******/
#ifdef SNMP_IPC_CLIENT

		case POD_SERVER_CMD_STB_REBOOT:    
		{
			int32_t result_recv = 0;
			uint32_t ret = RMF_SUCCESS;
			int32_t lenOfRequestor = 1;
			int32_t lenOfReason = 1;
			char szRequestor [1024];
			char szReason [1024];

			memset(&szRequestor, '\0', 1024);
			memset(&szReason, '\0', 1024);
			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD","%s : IPC called POD_SERVER_CMD_STB_REBOOT \n",__FUNCTION__ );
			result_recv |= pServerBuf->collectData( (void *) &lenOfRequestor,sizeof( int32_t ) );
			result_recv |= pServerBuf->collectData( (void *) &szRequestor, lenOfRequestor );
			result_recv |= pServerBuf->collectData( (void *) &lenOfReason,sizeof( int32_t ) );
			result_recv |= pServerBuf->collectData( (void *) &szReason, lenOfReason );
			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD","%s : Requestor=%s,Reason=%s\n",
										__FUNCTION__, szRequestor, szReason);
			if( result_recv < 0 )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
						  "%s : POD_SERVER_CMD_STB_REBOOT failed due to comm issue: \n", 
						  __FUNCTION__ );

				pServerBuf->addResponse( RMF_OSAL_ENODATA);
				pServerBuf->sendResponse(  );				
				pPODIPCServer->disposeBuf( pServerBuf );				
				break;
			}
			HAL_SYS_Reboot(szRequestor,szReason);
			pServerBuf->addResponse( ret );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
		}
		break;
		case POD_SERVER_CMD_UPDATE_TIME_EXT:
		{
			int32_t result_recv = 0;
			uint32_t ret = RMF_SUCCESS;
			unsigned short nYear, nMonth, nDay, nHour, nMin, nSec;			

			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD","%s : IPC called POD_SERVER_CMD_UPDATE_TIME_EXT \n",__FUNCTION__ );

			result_recv |= pServerBuf->collectData( (void *) &nYear,sizeof( nYear ) );
			result_recv |= pServerBuf->collectData( (void *) &nMonth,sizeof( nMonth ) );
			result_recv |= pServerBuf->collectData( (void *) &nDay,sizeof( nDay ) );
			result_recv |= pServerBuf->collectData( (void *) &nHour,sizeof( nHour ) );
			result_recv |= pServerBuf->collectData( (void *) &nMin,sizeof( nMin ) );
			result_recv = pServerBuf->collectData( (void *) &nSec,sizeof( nSec ) );
			if( 0 != result_recv )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s -Unexpected data for %d cmd in the buffer %d \n",
					__FUNCTION__, identifyer, result_recv );
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
				pServerBuf->sendResponse(  );
				pPODIPCServer->disposeBuf( pServerBuf );
				break;				
			}			
		
			ret = updateTimeFromExtSource( nYear, nMonth, nDay, nHour, nMin, nSec );
			pServerBuf->addResponse( ret );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
		}
		break;
		case POD_SERVER_CMD_GET_DST_EXIT:
		{
			UINT32 TimeExit = 0;
			cCardManagerIF *pCM = NULL;

			pCM = cCardManagerIF::getInstance();

			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				"%s : IPC called POD_SERVER_CMD_GET_DST_EXIT \n", __FUNCTION__ );
			if( pCM == NULL )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s - cCardManagerIF::getInstance() failed.\n", __FUNCTION__);
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
				pServerBuf->sendResponse(  );
				pPODIPCServer->disposeBuf( pServerBuf );
				break;
			}

			ret = pCM->SNMPGetGenFetDayLightSavingsTimeExit(&TimeExit);

			pServerBuf->addResponse( ret );
			pServerBuf->addData((void *)&TimeExit, sizeof( TimeExit ) );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
		}    

		break;

		case POD_SERVER_CMD_GET_DST_ENTRY:    
		{
			UINT32 TimeEntry = 0;
			cCardManagerIF *pCM = NULL;
			
			pCM = cCardManagerIF::getInstance();

			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				"%s : IPC called POD_SERVER_CMD_GET_DST_ENTRY \n", __FUNCTION__ );
			if( pCM == NULL )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s - cCardManagerIF::getInstance() failed.\n", __FUNCTION__);
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
				pServerBuf->sendResponse(  );
				pPODIPCServer->disposeBuf( pServerBuf );
				break;
			}			

			ret = pCM->SNMPGetGenFetDayLightSavingsTimeEntry(&TimeEntry);

			pServerBuf->addResponse( ret );
			pServerBuf->addData((void *)&TimeEntry, sizeof( TimeEntry ) );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
		}
		break;

		case POD_SERVER_CMD_GET_DST_DELTA:    
		{
			char  *aDelta = NULL;
			cCardManagerIF *pCM = NULL;

			pCM = cCardManagerIF::getInstance();

			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				"%s : IPC called POD_SERVER_CMD_GET_DST_DELTA \n", __FUNCTION__ );
			if( pCM == NULL )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s - cCardManagerIF::getInstance() failed.\n", __FUNCTION__);
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
				pServerBuf->sendResponse(  );
				pPODIPCServer->disposeBuf( pServerBuf );
				break;
			}
			rmf_osal_memAllocP(RMF_OSAL_MEM_POD, VL_MIN_SNMP_STR_SIZE, (void**)&aDelta);
			memset(aDelta, 0 , VL_MIN_SNMP_STR_SIZE);
			
			ret = pCM->SNMPGetGenFetDayLightSavingsTimeDelta(aDelta);
			int Deltalen = strlen(aDelta)+1 ;
			pServerBuf->addResponse( ret );
			pServerBuf->addData((void *)&Deltalen, sizeof(Deltalen) );
			pServerBuf->addData((void *)aDelta, Deltalen );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
			
			rmf_osal_memFreeP(RMF_OSAL_MEM_POD, aDelta);					
		}
		break;

		case POD_SERVER_CMD_DUMP_BUFFER: 
		{
			rmf_Error ret = RMF_SUCCESS ;
			int32_t result_recv = 0;

			rdk_LogLevel lvl;
			const char *mod =NULL;
			char Buffer[256] ;
			int32_t  nBufSize;			

			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				"%s : IPC called POD_SERVER_CMD_DUMP_BUFFER \n", __FUNCTION__ );

			result_recv |= pServerBuf->collectData( ( void * ) &lvl, sizeof( lvl ) );
			result_recv |= pServerBuf->collectData( ( void * ) &mod, sizeof( char ) );
			result_recv |= pServerBuf->collectData( ( void * ) &nBufSize, sizeof( nBufSize  )  );
			result_recv |= pServerBuf->collectData( ( void * ) Buffer, nBufSize );
			
			if( result_recv < 0 )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
						  "%s : POD_SERVER_CMD_DUMP_BUFFER failed due to comm issue: \n", 
						  __FUNCTION__ );

				pServerBuf->addResponse( RMF_OSAL_ENODATA);
				pServerBuf->sendResponse(  );				
				pPODIPCServer->disposeBuf( pServerBuf );				
				break;
			}
			vlMpeosDumpBuffer( lvl, mod, (const char *)Buffer, nBufSize );			
			pServerBuf->addResponse( ret );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
		}   
		break;
		
		case POD_SERVER_CMD_GET_GEN_FEATURE_DATA:    
		{
			int32_t result_recv = 0;

			VL_HOST_FEATURE_PARAM eFeature;
			VL_POD_HOST_FEATURE_EAS hostEasInfo;			

			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				"%s : IPC called POD_SERVER_CMD_GET_GEN_FEATURE_DATA \n", __FUNCTION__ );

			result_recv = pServerBuf->collectData( ( void * ) &eFeature, sizeof( eFeature ) );			
			if( 0 != result_recv )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s -Unexpected data for %d cmd in the buffer %d \n",
					__FUNCTION__, identifyer, result_recv );
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
				pServerBuf->sendResponse(  );
				pPODIPCServer->disposeBuf( pServerBuf );
				break;				
			}
			memset( &hostEasInfo, 0, sizeof(hostEasInfo) );
			
			ret = vlPodGetGenericFeatureData( eFeature, &hostEasInfo );
			
			pServerBuf->addResponse( ret );
			pServerBuf->addData( (void *)&hostEasInfo, sizeof( hostEasInfo ) );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
		}
		break;
		
		case POD_SERVER_CMD_GET_VCT_ID:    
		{
			rmf_Error ret = RMF_SUCCESS;
			uint16_t VctId = 0;
			cCardManagerIF *pCM = NULL;
			
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				"%s : IPC called  POD_SERVER_CMD_GET_VCT_ID \n", __FUNCTION__ );
			
			pCM = cCardManagerIF::getInstance();
			
			if( pCM == NULL )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s - cCardManagerIF::getInstance() failed.\n", __FUNCTION__);
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
				pServerBuf->sendResponse(  );
				pPODIPCServer->disposeBuf( pServerBuf );
				break;
			}

			ret = pCM->SNMPGetGenFetVctId( &VctId );
			
			pServerBuf->addResponse( ret );
			pServerBuf->addData( (void *)&VctId, sizeof( VctId ) );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );		
		}
		break;
		
		case POD_SERVER_CMD_GET_CP_ID_LIST:    
		{
			rmf_Error ret = RMF_SUCCESS;
			UINT32 CP_system_id_list = 0;
			cCardManagerIF *pCM = NULL;
					
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				"%s : IPC called  POD_SERVER_CMD_GET_CP_ID_LIST \n", __FUNCTION__ );

			pCM = cCardManagerIF::getInstance();
			
			if( pCM == NULL )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s - cCardManagerIF::getInstance() failed.\n", __FUNCTION__);
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
				pServerBuf->sendResponse(  );
				pPODIPCServer->disposeBuf( pServerBuf );
				break;
			}
			ret = pCM->SNMPGetCpIdList( &CP_system_id_list );
			
			pServerBuf->addResponse( ret );
			pServerBuf->addData((void *)&CP_system_id_list, sizeof( CP_system_id_list ) );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
		}
		break;
		
		case POD_SERVER_CMD_GET_CP_CERT_STATUS:    
		{
			rmf_Error ret = RMF_SUCCESS;
			int Certtatus = 0; 
			cCardManagerIF *pCM = NULL;
			
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				"%s : IPC called  POD_SERVER_CMD_GET_CP_CERT_STATUS \n", __FUNCTION__ );
			
			pCM = cCardManagerIF::getInstance();
			
			if( pCM == NULL )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s - cCardManagerIF::getInstance() failed.\n", __FUNCTION__);
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
				pServerBuf->sendResponse(  );
				pPODIPCServer->disposeBuf( pServerBuf );
				break;
			}
			ret = pCM->SNMPGetCpCertCheck(&Certtatus);
			
			pServerBuf->addResponse( ret );
			pServerBuf->addData( (void *)&Certtatus, sizeof( Certtatus ) );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );			
		}
		break;
		
		case POD_SERVER_CMD_GET_CP_CCI_CHALLENGE_COUNT:    
		{
			rmf_Error ret = RMF_SUCCESS;
			UINT32 Count = 0; 
			cCardManagerIF *pCM = NULL;
			
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				"%s : IPC called  POD_SERVER_CMD_GET_CP_CCI_CHALLENGE_COUNT \n", __FUNCTION__ );

			pCM = cCardManagerIF::getInstance();

			if( pCM == NULL )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s - cCardManagerIF::getInstance() failed.\n", __FUNCTION__);
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
				pServerBuf->sendResponse(  );
				pPODIPCServer->disposeBuf( pServerBuf );
				break;
			}		      
			ret = pCM->SNMPGetCpCciChallengeCount(&Count);
			
			pServerBuf->addResponse( ret );
			pServerBuf->addData( (void *)&Count, sizeof( Count ) );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );			
		}
		break;
		
		case POD_SERVER_CMD_GET_CP_KEY_GEN_REQUEST:    
		{
			rmf_Error ret = RMF_SUCCESS;
			UINT32 Count=0;
			cCardManagerIF *pCM = NULL;
			
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				"%s : IPC called  POD_SERVER_CMD_GET_CP_KEY_GEN_REQUEST \n", __FUNCTION__ );
			pCM = cCardManagerIF::getInstance();

			if( pCM == NULL )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s - cCardManagerIF::getInstance() failed.\n", __FUNCTION__);
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
				pServerBuf->sendResponse(  );
				pPODIPCServer->disposeBuf( pServerBuf );
				break;
			}
			ret = pCM->SNMPGetCpKeyGenerationReqCount( &Count );
			
			pServerBuf->addResponse( ret );
			pServerBuf->addData( (void *)&Count, sizeof( Count ) );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );			
		}
		break;
		
		case POD_SERVER_CMD_GET_CP_AUTH_KEY_STATUS:    
		{

			rmf_Error ret = RMF_SUCCESS;
			int32_t KeyStatus=0;
			cCardManagerIF *pCM = NULL;
			
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				"%s : IPC called  POD_SERVER_CMD_GET_CP_AUTH_KEY_STATUS\n", __FUNCTION__ );

			pCM = cCardManagerIF::getInstance();

			if( pCM == NULL )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s - cCardManagerIF::getInstance() failed.\n", __FUNCTION__);
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
				pServerBuf->sendResponse(  );
				pPODIPCServer->disposeBuf( pServerBuf );
				break;
			}
			ret = pCM->SNMPGetCpAuthKeyStatus( &KeyStatus );
			
			pServerBuf->addResponse( ret );
			pServerBuf->addData( (void *)&KeyStatus, sizeof( KeyStatus ) );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );			
		}
		break;
		
		case POD_SERVER_CMD_GET_EAS_LOCATION:    
		{
			rmf_Error ret = RMF_SUCCESS;
			uint8_t  State = 0; 
			uint16_t  Country = 0;
			cCardManagerIF *pCM = NULL;
			
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				"%s : IPC called POD_SERVER_CMD_GET_EAS_LOCATION \n", __FUNCTION__ );

			pCM = cCardManagerIF::getInstance();

			if( pCM == NULL )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s - cCardManagerIF::getInstance() failed.\n", __FUNCTION__);
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
				pServerBuf->sendResponse(  );
				pPODIPCServer->disposeBuf( pServerBuf );
				break;
			}
			ret = pCM->SNMPGetGenFetEasLocationCode( &State, &Country );
			
			pServerBuf->addResponse( ret );
			pServerBuf->addData( (void *)&State, sizeof( State ) );
			pServerBuf->addData( (void *)&Country, sizeof( Country ) );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );		
		}
		break;
		
		case POD_SERVER_CMD_SET_DSG_CONFIG:    
		{
			int32_t result_recv = 0;
			int32_t device_instance;
			uint32_t ulTag;
			uint8_t u8ipMode = 0;			
			VL_TLV_217_IP_MODE ipMode = VL_TLV_217_IP_MODE_IPV4;			

			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				"%s : IPC called POD_SERVER_CMD_SET_DSG_CONFIG \n", __FUNCTION__ );

			result_recv |= pServerBuf->collectData( ( void * ) &device_instance, sizeof( device_instance ) );
			result_recv |= pServerBuf->collectData( ( void * ) &ulTag, sizeof( ulTag ) );
			result_recv = pServerBuf->collectData( ( void * ) &u8ipMode, sizeof( u8ipMode ) );
			if( 0 != result_recv )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s -Unexpected data for %d cmd in the buffer %d \n",
					__FUNCTION__, identifyer, result_recv );
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
				pServerBuf->sendResponse(  );
				pPODIPCServer->disposeBuf( pServerBuf );
				break;				
			}			


            ipMode = (VL_TLV_217_IP_MODE)(u8ipMode);
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				"%s : ipMode = %d.\n", __FUNCTION__, ipMode);
			ret = CHALDsg_Set_Config(device_instance,ulTag,(void*)&ipMode);
			
			pServerBuf->addResponse( ret );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
		}
		break;
		
		case POD_SERVER_CMD_GET_GEN_TIME_ZONE_OFFSET:    
		{
			int32_t TZOffset = 0;
			cCardManagerIF *pCM = NULL;

			pCM = cCardManagerIF::getInstance();

			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				"%s : IPC called POD_SERVER_CMD_GET_GEN_TIME_ZONE_OFFSET \n", __FUNCTION__ );
			
			if( pCM == NULL )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s - cCardManagerIF::getInstance() failed.\n", __FUNCTION__);
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
				pServerBuf->sendResponse(  );
				pPODIPCServer->disposeBuf( pServerBuf );
				break;
			}
			pCM->SNMPGetGenFetTimeZoneOffset( &TZOffset );

			pServerBuf->addResponse( RMF_SUCCESS );
			pServerBuf->addData( (void *)&TZOffset, sizeof( TZOffset ) );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
		}
		break;

		case POD_SERVER_CMD_GET_GEN_RESOURCE_ID:    
		{
			UINT32 RrsId = 0;
			cCardManagerIF *pCM = NULL;

			pCM = cCardManagerIF::getInstance();

			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				"%s : IPC called POD_SERVER_CMD_GET_GEN_RESOURCE_ID \n", __FUNCTION__ );
			
			if( pCM == NULL )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s - cCardManagerIF::getInstance() failed.\n", __FUNCTION__);
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
				pServerBuf->sendResponse(  );
				pPODIPCServer->disposeBuf( pServerBuf );
				break;
			}
			pCM->SNMPGetGenFetResourceId( &RrsId );

			pServerBuf->addResponse( RMF_SUCCESS );
			pServerBuf->addData((void *)&RrsId, sizeof( RrsId ) );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
		}
		break;

		case POD_SERVER_CMD_GET_INBAND_OOB_MODE:    
		{
			vlCableCardIdBndStsOobMode_t oobInfo;
			cCardManagerIF *pCM = NULL;

			pCM = cCardManagerIF::getInstance();

			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				"%s : IPC called POD_SERVER_CMD_GET_INBAND_OOB_MODE \n", __FUNCTION__ );
			
			if( pCM == NULL )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s - cCardManagerIF::getInstance() failed.\n", __FUNCTION__);
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
				pServerBuf->sendResponse(  );
				pPODIPCServer->disposeBuf( pServerBuf );
				break;
			}
			
			memset( &oobInfo, 0, sizeof(oobInfo) );
			pCM->SNMPGetCardIdBndStsOobMode( &oobInfo );

			pServerBuf->addResponse( RMF_SUCCESS );
			pServerBuf->addData( (void *)&oobInfo, sizeof( oobInfo ) );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
		}
		break;

		case POD_SERVER_CMD_GET_APP_INFO:    
		{
			vlCableCardAppInfo_t appInfo;
			cCardManagerIF *pCM = NULL;
			int8_t index = 0;
			int32_t result_recv = 0;

			pCM = cCardManagerIF::getInstance( );

			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
				"%s : IPC called POD_SERVER_CMD_GET_APP_INFO \n", __FUNCTION__ );

			if( pCM == NULL )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s - cCardManagerIF::getInstance() failed.\n", __FUNCTION__);
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
				pServerBuf->sendResponse(  );
				pPODIPCServer->disposeBuf( pServerBuf );
				break;
			 }
		          memset(&appInfo, 0, sizeof(vlCableCardAppInfo_t));	 
			  pCM->SNMPGetApplicationInfo( &appInfo );
			  result_recv = pServerBuf->collectData( (void *)&index,sizeof( index ) );
				if( 0 != result_recv )
				{
					RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s -Unexpected data for %d cmd in the buffer %d \n",
						__FUNCTION__, identifyer, result_recv );
					pServerBuf->addResponse( RMF_OSAL_ENODATA );
					pServerBuf->sendResponse(  );
					pPODIPCServer->disposeBuf( pServerBuf );
					break;				
				}
			  RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
				  "%s: : sizeof appInfo=%d \n", __FUNCTION__, sizeof( vlCableCardAppInfo_t ));

			  pServerBuf->addResponse( RMF_SUCCESS );
			  switch(index)
			  {
				case 0:
					pServerBuf->addData( ( const void * ) &appInfo.CardManufactureId,
                      sizeof( appInfo.CardManufactureId) ); //2 bytes
					pServerBuf->addData( ( const void * ) &appInfo.CardVersion,
                      sizeof( appInfo.CardVersion) ); //2 bytes
					pServerBuf->addData( ( const void * ) &appInfo.CardMAC,
                      sizeof( appInfo.CardMAC) ); //6 bytes
					pServerBuf->addData( ( const void * ) &appInfo.CardSerialNumberLen,
                      sizeof( appInfo.CardSerialNumberLen) ); //1 bytes
					pServerBuf->addData( ( const void * ) &appInfo.CardSerialNumber,
                      sizeof( appInfo.CardSerialNumber) ); //256 bytes
					pServerBuf->addData( ( const void * ) &appInfo.CardRootOidLen,
                      sizeof( appInfo.CardRootOidLen) ); //1 bytes
					pServerBuf->addData( ( const void * ) &appInfo.CardRootOid,
                      sizeof( appInfo.CardRootOid) ); //256 bytes
					pServerBuf->addData( ( const void * ) &appInfo.CardNumberOfApps,
                      sizeof( appInfo.CardNumberOfApps) ); //256 bytes
					pServerBuf->addData( ( const void * ) &appInfo.Apps[0],
                      sizeof( appInfo.Apps[0]) ); //524 bytes
					pServerBuf->addData( ( const void * ) &appInfo.Apps[1],
                      sizeof( appInfo.Apps[1]) ); //524 bytes
				break;
				case 1:
					pServerBuf->addData( ( const void * ) &appInfo.Apps[2],
                      sizeof( appInfo.Apps[2]) ); //524 bytes
					pServerBuf->addData( ( const void * ) &appInfo.Apps[3],
                      sizeof( appInfo.Apps[3]) ); //524 bytes
					pServerBuf->addData( ( const void * ) &appInfo.Apps[4],
                      sizeof( appInfo.Apps[4]) ); //524 bytes
				break;
				case 2:
					pServerBuf->addData( ( const void * ) &appInfo.Apps[5],
                      sizeof( appInfo.Apps[5]) ); //524 bytes
					pServerBuf->addData( ( const void * ) &appInfo.Apps[6],
                      sizeof( appInfo.Apps[6]) ); //524 bytes
					pServerBuf->addData( ( const void * ) &appInfo.Apps[7],
                      sizeof( appInfo.Apps[7]) ); //524 bytes
				break;
				case 3:
					pServerBuf->addData( ( const void * ) &appInfo.Apps[8],
                      sizeof( appInfo.Apps[8]) ); //524 bytes
					pServerBuf->addData( ( const void * ) &appInfo.Apps[9],
                      sizeof( appInfo.Apps[9]) ); //524 bytes
					pServerBuf->addData( ( const void * ) &appInfo.Apps[10],
                      sizeof( appInfo.Apps[10]) ); //524 bytes
				break;
				case 4:
					pServerBuf->addData( ( const void * ) &appInfo.Apps[11],
                      sizeof( appInfo.Apps[11]) ); //524 bytes
				break;
				default:
			  		pServerBuf->addData( (void *)&appInfo, sizeof( appInfo ) );
				break;
			  }
			  pServerBuf->sendResponse(  );
			  pPODIPCServer->disposeBuf( pServerBuf );			
		}
		break;

		case POD_SERVER_CMD_GET_DSG_ETH_INFO:    
		{
			VL_HOST_IP_INFO * pHostIpInfo = NULL;
			
			rmf_osal_memAllocP( RMF_OSAL_MEM_POD, sizeof( VL_HOST_IP_INFO), (void**)&pHostIpInfo );
			memset( pHostIpInfo, 0, sizeof(VL_HOST_IP_INFO) );
			
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD","%s : IPC called POD_SERVER_CMD_GET_DSG_ETH_INFO \n",__FUNCTION__ );
			
			vlXchanGetDsgEthName(pHostIpInfo);
			
			pServerBuf->addResponse( RMF_SUCCESS);
			pServerBuf->addData((void *)pHostIpInfo, sizeof( VL_HOST_IP_INFO ) );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
			
			rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pHostIpInfo);
		}
		break;

		case POD_SERVER_CMD_GET_CHANNEL_MAPID:    
		{
			uint16_t ChannelMapId  = 0xFFFF;
			uint16_t manufacturerId = 0;
			int32_t manufacturer = 0;
				
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD","%s : IPC called POD_SERVER_CMD_GET_CHANNEL_MAPID \n",__FUNCTION__ );
			rmf_Error  ret = podmgrGetManufacturerId( &manufacturerId );
			
			manufacturer = (manufacturerId & 0xFF00) >> 8;
			if( CARD_TYPE_CISCO == manufacturer )
			{
				cCardManagerIF *pCM = NULL;
				gbCacheProxyAPDU = false;
				pCM = cCardManagerIF::getInstance();
				if( NULL != pCM )
				{
					if( RMF_SUCCESS != pCM->SNMPGetGenFetVctId( &ChannelMapId ) )
					{
						ChannelMapId = 0xFFFF;
						RDK_LOG( RDK_LOG_INFO, 	"LOG.RDK.POD",
							"%s : POD_SERVER_CMD_GET_CHANNEL_MAPID  failed \n",__FUNCTION__ );
					}
					else
					{
						RDK_LOG( RDK_LOG_INFO, 	"LOG.RDK.POD",
							"%s : POD_SERVER_CMD_GET_CHANNEL_MAPID  is 0x%x \n",__FUNCTION__, ChannelMapId );
					}
				}
			}
			else if( CARD_TYPE_MOTO == manufacturer ) 
			{
				rmf_OobSiMgr *pOobSiMgr =  rmf_OobSiMgr::getInstance();
				pOobSiMgr->GetChannelMapId( &ChannelMapId );
			}
			pServerBuf->addResponse( RMF_SUCCESS);
			pServerBuf->addData((void *)&ChannelMapId, sizeof( ChannelMapId ) );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
		}
		break;

		case POD_SERVER_CMD_GET_DACID:    
		{
			uint16_t DACId = 0;
			rmf_Error ret = RMF_SUCCESS;	
			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD","%s : IPC called POD_SERVER_CMD_GET_DACID \n",__FUNCTION__ );
			
			rmf_OobSiMgr *pOobSiMgr =  rmf_OobSiMgr::getInstance();
			ret = pOobSiMgr->GetDACId( &DACId );
			if( RMF_SUCCESS != ret )
			{
				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD","%s : POD_SERVER_CMD_GET_DACID failed \n",__FUNCTION__ );
			}
			pServerBuf->addResponse( ret );
			pServerBuf->addData((void *)&DACId, sizeof( DACId  ) );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
		}
		break;

		case POD_SERVER_CMD_GET_LOST_DSG_IPU_INFO:
		{
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD","%s : IPC called POD_SERVER_CMD_GET_LOST_DSG_IPU_INFO \n",__FUNCTION__ );
			
			vlXchanIndicateLostDsgIPUFlow();
			
			pServerBuf->addResponse( RMF_SUCCESS);
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
		}    
		break;

		case POD_SERVER_CMD_GET_IPV6_SUBMASK:    
		{

			rmf_Error ret = RMF_SUCCESS;
			int32_t result_recv = 0;
            unsigned char v6mask[VL_IPV6_ADDR_SIZE];
			int32_t  nPlen;			
			int32_t nBufCapacity;
			
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				"%s : IPC called POD_SERVER_CMD_GET_IPV6_SUBMASK \n", __FUNCTION__ );

			result_recv = pServerBuf->collectData( ( void * ) &nPlen, sizeof( nPlen ) );
			
			if( 0 != result_recv )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s -Unexpected data for %d cmd in the buffer %d \n",
					__FUNCTION__, identifyer, result_recv );
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
				pServerBuf->sendResponse(  );
				pPODIPCServer->disposeBuf( pServerBuf );
				break;				
			}	
			ret = vlXchanGetV6SubnetMaskFromPlen( nPlen, v6mask, sizeof(v6mask));	
			
			pServerBuf->addResponse( ret);
			pServerBuf->addData( (void *)v6mask, sizeof(v6mask));
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
		}
		break;

		case POD_SERVER_CMD_SET_ECM_ROUTE:    
		{
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD","%s : IPC called POD_SERVER_CMD_SET_ECM_ROUTE \n",__FUNCTION__ );
			
			ret = vlXchanSetEcmRoute();
			
			pServerBuf->addResponse( ret);
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
		}
		break;

		case POD_SERVER_CMD_GET_RC_IPADDRESS:    
		{
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD","%s : IPC called POD_SERVER_CMD_GET_RC_IPADDRESS \n",__FUNCTION__ );
			uint8_t *rcIpAddress = NULL;
			
			rcIpAddress = vlXchanGetRcIpAddress();
			
			pServerBuf->addResponse( RMF_SUCCESS);
			pServerBuf->addData( (void *)rcIpAddress, VL_IP_ADDR_SIZE );			
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
		}
		break;

		case POD_SERVER_CMD_GET_DSG_IP_INFO:    
		{
			VL_HOST_IP_INFO * pHostIpInfo = NULL;
			
			rmf_osal_memAllocP( RMF_OSAL_MEM_POD, sizeof(VL_HOST_IP_INFO), (void**)&pHostIpInfo );			
			memset( pHostIpInfo, 0, sizeof(VL_HOST_IP_INFO) );
			
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD","%s : IPC called POD_SERVER_CMD_GET_DSG_IP_INFO \n",__FUNCTION__ );
			
			vlXchanGetDsgIpInfo( pHostIpInfo );
			
			pServerBuf->addResponse( RMF_SUCCESS);
			pServerBuf->addData((void *)pHostIpInfo, sizeof( VL_HOST_IP_INFO ) );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
			
			rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pHostIpInfo);
		}
		break;
		
		case POD_SERVER_CMD_GET_DSG_IP:    
		{

			
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD","%s : DSG IP Not acquired \n",__FUNCTION__ );
			if(0 == strlen((const char*)podmgrDSGIP) )
			{
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
			} 
			else
			{			
				pServerBuf->addResponse( RMF_SUCCESS);
				pServerBuf->addData((void *)podmgrDSGIP, sizeof( podmgrDSGIP) );
			}
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
			
		}
		break;
		case POD_SERVER_CMD_GET_CCAPP_INFO:    
		{
		    	int32_t result_recv = 0;
		    	const char * pszPage = NULL; 
			int32_t nBytes = 0; 
			uint32_t iIndex = 0;
            uint32_t nSnmpBufLimit = 1472;
		    
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD","%s : IPC called POD_SERVER_CMD_GET_CCAPP_INFO \n",__FUNCTION__ );
		    
			result_recv |= pServerBuf->collectData( ( void * ) &iIndex, sizeof( iIndex )  );

			if( result_recv < 0 )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					"%s : POD_SERVER_CMD_GET_CCAPP_INFO failed due to comm issue: \n", 
					__FUNCTION__ );

				pServerBuf->addResponse( RMF_OSAL_ENODATA);
				pServerBuf->sendResponse(  );				
				pPODIPCServer->disposeBuf( pServerBuf );				
				break;
			}

			result_recv = pServerBuf->collectData( ( void * ) &nSnmpBufLimit, sizeof( nSnmpBufLimit )  );

			if( 0 != result_recv )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s -Unexpected data for %d cmd in the buffer %d \n",
					__FUNCTION__, identifyer, result_recv );
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
				pServerBuf->sendResponse(  );
				pPODIPCServer->disposeBuf( pServerBuf );
				break;				
			}
            
		    cCardManagerIF *pCM = NULL;
		    pCM = cCardManagerIF::getInstance();
		    
		    if( pCM == NULL )
		    {
			    RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s - cCardManagerIF::getInstance() failed.\n", __FUNCTION__);
			    pServerBuf->addResponse( RMF_OSAL_ENODATA );
			    pServerBuf->sendResponse(  );
			    pPODIPCServer->disposeBuf( pServerBuf );
			    break;
		    }
            
            string strPage;
			ret = pCM->vlGetPodMmiTable(iIndex, strPage, nSnmpBufLimit);
            pszPage = strPage.c_str();
            nBytes = strPage.size();

			if(NULL != pszPage)
			{
				pServerBuf->addResponse( ret);
				pServerBuf->addData( (void *)&nBytes, sizeof( nBytes ) );
				pServerBuf->addData( (void *)pszPage ,nBytes+1 );			
			}
			else
			{
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
			}
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
		}
		break;

		case POD_SERVER_CMD_GET_DSG_CABLE_CARD_INFO:    
		{
			VL_HOST_IP_INFO * pHostIpInfo = NULL;
			
			rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(VL_HOST_IP_INFO), (void**)&pHostIpInfo);
			memset( pHostIpInfo, 0, sizeof(VL_HOST_IP_INFO) );
			
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD","%s : IPC called POD_SERVER_CMD_GET_DSG_CABLE_CARD_INFO \n",__FUNCTION__ );
			
			vlXchanGetDsgCableCardIpInfo(pHostIpInfo);
			
			pServerBuf->addResponse( RMF_SUCCESS);
			pServerBuf->addData((void *)pHostIpInfo, sizeof( VL_HOST_IP_INFO  ) );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
			
			rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pHostIpInfo);
		}
		break;

		case POD_SERVER_CMD_GET_HOST_CABLE_CARD_INFO:    
		{
			VL_HOST_CARD_IP_INFO * pCardIpInfo = NULL;
			
			rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(VL_HOST_CARD_IP_INFO), (void**)&pCardIpInfo);
			memset(pCardIpInfo,0,sizeof(VL_HOST_CARD_IP_INFO));
			
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD","%s : IPC called POD_SERVER_CMD_GET_HOST_CABLE_CARD_INFO \n",__FUNCTION__ );
			
			vlXchanGetHostCardIpInfo(pCardIpInfo);
			
			pServerBuf->addResponse( RMF_SUCCESS);
			pServerBuf->addData((void *)pCardIpInfo, sizeof( VL_HOST_CARD_IP_INFO ) );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
			
			rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pCardIpInfo);
		}
		break;

		case POD_SERVER_CMD_GET_HOST_ID_LUHN:    
		{
			unsigned char *pHostId=NULL;
			char *pHostIdLu=NULL;
			int32_t result_recv = 0;
			HostCATypeInfo_t HostCaTypeInfo;
			
			rmf_osal_memAllocP(RMF_OSAL_MEM_POD, VL_MAX_SNMP_STR_SIZE , (void**)&pHostId);
			rmf_osal_memAllocP(RMF_OSAL_MEM_POD, VL_MAX_SNMP_STR_SIZE , (void**)&pHostIdLu);
			memset( pHostIdLu, 0, VL_MAX_SNMP_STR_SIZE );
			memset( pHostId, 0, VL_MAX_SNMP_STR_SIZE );

			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				"%s : IPC called POD_SERVER_CMD_GET_HOST_ID_LUHN \n", __FUNCTION__ );


			result_recv = pServerBuf->collectData( ( void * ) pHostId, sizeof(HostCaTypeInfo.SecurityID) ); 
			
			if( result_recv != 0 )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
						  "%s : POD_SERVER_CMD_START_HOMING_DOWNLOAD failed due to comm issue: \n", 
						  __FUNCTION__ );

				pServerBuf->addResponse( RMF_OSAL_ENODATA);
				pServerBuf->sendResponse(  );
				if(pHostId)
				{
					rmf_osal_memFreeP(RMF_OSAL_MEM_POD,pHostId);
					pHostId = NULL;
				}	
				if(pHostIdLu)
				{
					rmf_osal_memFreeP(RMF_OSAL_MEM_POD,pHostIdLu);
					pHostIdLu = NULL;
				}					
				pPODIPCServer->disposeBuf( pServerBuf );				
				break;
			}	
			
			GetHostIdLuhn( pHostId,  pHostIdLu );	
			
			pServerBuf->addResponse( RMF_SUCCESS);
			pServerBuf->addData( (void *)pHostIdLu, VL_MAX_SNMP_STR_SIZE );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
			
			rmf_osal_memFreeP( RMF_OSAL_MEM_POD, pHostIdLu );
			rmf_osal_memFreeP( RMF_OSAL_MEM_POD, pHostId );
		}
		break;

		case POD_SERVER_CMD_CDL_HOST_FW_STATUS:    
		{
			CDLMgrSnmpHostFirmwareStatus_t *pStatus=NULL;
			
			rmf_osal_memAllocP( RMF_OSAL_MEM_POD, 
				sizeof(CDLMgrSnmpHostFirmwareStatus_t), (void**)&pStatus);			
			memset( pStatus, 0, sizeof(CDLMgrSnmpHostFirmwareStatus_t) );
			
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD","%s : IPC called POD_SERVER_CMD_CDL_HOST_FW_STATUS \n",__FUNCTION__ );
			
			CommonDownloadMgrSnmpHostFirmwareStatus(pStatus);
			
			pServerBuf->addResponse( RMF_SUCCESS );
			pServerBuf->addData( (void *)pStatus, sizeof( CDLMgrSnmpHostFirmwareStatus_t ) );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
			
			rmf_osal_memFreeP( RMF_OSAL_MEM_POD, pStatus );
		}
		break;

		case POD_SERVER_CMD_CDL_HOST_FW_STATUS_CHECK:    
		{
			CDLMgrSnmpCertificateStatus_t *pStatus=NULL;
			
			rmf_osal_memAllocP( RMF_OSAL_MEM_POD, 
				sizeof(CDLMgrSnmpCertificateStatus_t), (void**)&pStatus );
			memset(pStatus,0,sizeof(CDLMgrSnmpCertificateStatus_t));
			
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD","%s : IPC called POD_SERVER_CMD_CDL_HOST_FW_STATUS_CHECK \n",__FUNCTION__);
			
			CommonDownloadMgrSnmpCertificateStatus_check(pStatus);
			
			pServerBuf->addResponse( RMF_SUCCESS);
			pServerBuf->addData( (void *)pStatus, sizeof( CDLMgrSnmpCertificateStatus_t ) );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
			
			rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pStatus);
		}
		break;

		case POD_SERVER_CMD_IS_DSG_MODE:
		{
			uint32_t ret = 0;
			int32_t isDsgMode = 0;

			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
			"%s : IPC called POD_SERVER_CMD_IS_DSG_MODE \n", __FUNCTION__ );

			isDsgMode = vlIsDsgOrIpduMode();

			pServerBuf->addResponse( ret );
			pServerBuf->addData( (void *)&isDsgMode, sizeof( isDsgMode ) );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
		}
		break;

		case POD_SERVER_CMD_HALSYS_SNMP_REQUEST:
		{
			int32_t result_recv = 0;
			uint32_t ret = 0;
			VL_SNMP_API_RESULT res;
			VL_SNMP_REQUEST eRequest;
			
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD","%s : IPC called POD_SERVER_CMD_HALSYS_SNMP_REQUEST \n",__FUNCTION__ );


			result_recv = pServerBuf->collectData( ( void * ) &eRequest, sizeof( eRequest )  );

			if( result_recv != 0 )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
						  "%s : POD_SERVER_CMD_HALSYS_SNMP_REQUEST failed due to comm issue: \n", 
						  __FUNCTION__ );

				pServerBuf->addResponse( RMF_OSAL_ENODATA);
				pServerBuf->sendResponse(  );
				pPODIPCServer->disposeBuf( pServerBuf );
				break;
			}

			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
		              "%s : eRequest is %d \n",
		              __FUNCTION__, eRequest );

			switch(eRequest)
			{
				case VL_SNMP_REQUEST_GET_SYS_SYSTEM_MEMORY_REPORT:
				{
					VL_SNMP_SystemMemoryReportTable snmpSystemMemoryReport;
					RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
					"%s : IPC called VL_SNMP_REQUEST_GET_SYS_SYSTEM_MEMORY_REPORT \n", __FUNCTION__ );
			
					memset(&snmpSystemMemoryReport, 0, sizeof(VL_SNMP_SystemMemoryReportTable));
					snmpSystemMemoryReport.nEntries = VL_SNMP_HOST_MIB_MEMORY_TYPE_MAX;
					res = CHalSys_snmp_request(VL_SNMP_REQUEST_GET_SYS_SYSTEM_MEMORY_REPORT, &snmpSystemMemoryReport);

					pServerBuf->addResponse( ret );
					pServerBuf->addData((void *)&res, sizeof( res ) );
					pServerBuf->addData((void *)&snmpSystemMemoryReport, sizeof( snmpSystemMemoryReport ) );					
				}
				break;
				case VL_SNMP_REQUEST_GET_SYS_SYSTEM_POWER_STATE:
				{
						VL_SNMP_VAR_HOST_POWER_STATUS SNMPInfo; 
						memset(&SNMPInfo, 0, sizeof(VL_SNMP_VAR_HOST_POWER_STATUS));
						SNMPInfo.ocStbHostPowerStatus     = VL_SNMP_HOST_POWER_STATUS_POWERON;
						res = CHalSys_snmp_request(VL_SNMP_REQUEST_GET_SYS_SYSTEM_POWER_STATE, (void*)&SNMPInfo.ocStbHostPowerStatus);
						RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "PPR_____________________ %s: AFTER calling CHalSys_snmp_request: SNMPInfo->ocStbHostPowerStatus = %d\n", __FUNCTION__, SNMPInfo.ocStbHostPowerStatus);
						pServerBuf->addResponse( ret );
						pServerBuf->addData((void *)&res, sizeof( res ) );
						pServerBuf->addData((void *)&SNMPInfo, sizeof( SNMPInfo ) );						      
				}
				break;
				default :
				{
					pServerBuf->addResponse(VL_SNMP_API_RESULT_FAILED);
				}
				break;
			}
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
		}
		break;

		case POD_SERVER_CMD_GET_SOCKET_FLOW_COUNT:    
		{			
			uint32_t ret = 0;
			int32_t res;
			
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD","%s : IPC called POD_SERVER_CMD_GET_SOCKET_FLOW_COUNT \n",__FUNCTION__ );
			
			res = vlXchanGetSocketFlowCount();
			
			pServerBuf->addResponse( ret );
			pServerBuf->addData((void *)&res, sizeof( res ) );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
		}
		break;
		case POD_SERVER_CMD_SEND_CARD_MIB_ACC_SNMP_REQ:    
		{
			int32_t result_recv = 0;
			uint32_t ret = 0;
			cCardManagerIF *pCM = NULL;
			uint8_t *pData = NULL;
			uint32_t nBytes;

			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD","%s : IPC called POD_SERVER_CMD_SEND_CARD_MIB_ACC_SNMP_REQ \n",__FUNCTION__ );
			
			pCM = cCardManagerIF::getInstance();
			
			if( pCM == NULL )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s - cCardManagerIF::getInstance() failed.\n", __FUNCTION__);
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
				pServerBuf->sendResponse(  );
				pPODIPCServer->disposeBuf( pServerBuf );
				break;
			}
			result_recv |= pServerBuf->collectData( (void *)&nBytes,sizeof( nBytes ) );
			rmf_osal_memAllocP( RMF_OSAL_MEM_POD, nBytes, ( void ** ) &pData );

			if(NULL != pData)
			{
				result_recv = pServerBuf->collectData( (void *)pData,nBytes );
				if( 0 != result_recv )
				{
					RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s -Unexpected data for %d cmd in the buffer %d \n",
					__FUNCTION__, identifyer, result_recv );
					pServerBuf->addResponse( RMF_OSAL_ENODATA);
					pServerBuf->sendResponse(  );
					pPODIPCServer->disposeBuf( pServerBuf );
					rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pData); 
					break;
				}

				pCM->SNMPSendCardMibAccSnmpReq(pData, nBytes);
				rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pData);
			}

			pServerBuf->addResponse( ret );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
		}
		break;

		case POD_SERVER_CMD_GET_STB_HOST_ID:    
		{
			int32_t result_recv = 0;
			uint32_t ret = 0;
			char len;
			char szHostId[64];
			szHostId[0] = '\0';
			cCardManagerIF *pCM = NULL;
			
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD","%s : IPC called POD_SERVER_CMD_GET_STB_HOST_ID \n",__FUNCTION__ );
			
			pCM = cCardManagerIF::getInstance();
			
			if( pCM == NULL )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s - cCardManagerIF::getInstance() failed.\n", __FUNCTION__);
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
				memcpy(szHostId,"0-000-000-000-000", sizeof("0-000-000-000-000"));
				pServerBuf->sendResponse(  );
				pPODIPCServer->disposeBuf( pServerBuf );
				break;
			}

			result_recv = pServerBuf->collectData( (void *)&len,sizeof( len ) );
			
			if( result_recv != 0 )
			{
			      RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
						  "%s : POD_SERVER_CMD_GET_STB_HOST_ID failed due to comm issue: \n", 
						  __FUNCTION__ );

			      pServerBuf->addResponse( RMF_OSAL_ENODATA);
			      pServerBuf->sendResponse(  );				
			      pPODIPCServer->disposeBuf( pServerBuf );				
			      break;
			}	
		
			unsigned int result = 0xff;
			result = pCM->SNMPGet_ocStbHostHostID( szHostId, len);
	
			pServerBuf->addResponse( ret );
			pServerBuf->addData( szHostId, strlen(szHostId) );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
		}
		break;

		case POD_SERVER_CMD_GET_CABLE_CARD_CERT_INFO:    
		{
			int32_t result_recv = 0;
			uint32_t ret = 0;
			int8_t mCardCount = 0;
			vlCableCardCertInfo_t  Mcardcertinfo;
			
			cCardManagerIF *pCM = NULL;
			
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD","%s : IPC called POD_SERVER_CMD_GET_CABLE_CARD_CERT_INFO \n",__FUNCTION__ );
			
			pCM = cCardManagerIF::getInstance();
			
			if ( pCM == NULL)
			{
			    RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s - cCardManagerIF::getInstance failed.\n", __FUNCTION__);
			    pServerBuf->addResponse( RMF_OSAL_ENODATA );
			    pServerBuf->sendResponse(  );
			    pPODIPCServer->disposeBuf( pServerBuf );
			    break;
			}
			result_recv = pServerBuf->collectData( (void *)&mCardCount,sizeof( mCardCount ) );
			
			if( result_recv != 0 )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
						  "%s : POD_SERVER_CMD_GET_CABLE_CARD_CERT_INFO failed due to comm issue: \n", 
						  __FUNCTION__ );

				pServerBuf->addResponse( RMF_OSAL_ENODATA);
				pServerBuf->sendResponse(  );				
				pPODIPCServer->disposeBuf( pServerBuf );				
				break;
			}
			
	    
			memset(&Mcardcertinfo, 0, sizeof(vlCableCardCertInfo_t));
			ret = pCM->SNMPGetCableCardCertInfo(&Mcardcertinfo);
		    	pServerBuf->addResponse( ret );
			
			
		      switch(mCardCount)
		      {
			case 0 :
			{ 
		    
	    
			  pServerBuf->addData( ( const void * ) &Mcardcertinfo.szDispString,
							  sizeof( Mcardcertinfo.szDispString ) );
			  pServerBuf->addData( ( const void * ) &Mcardcertinfo.szDeviceCertSubjectName,
							  sizeof( Mcardcertinfo.szDeviceCertSubjectName ) );
			  pServerBuf->addData( ( const void * ) &Mcardcertinfo.szDeviceCertIssuerName,
							  sizeof( Mcardcertinfo.szDeviceCertIssuerName ) );
			  pServerBuf->sendResponse(  );
			  pPODIPCServer->disposeBuf( pServerBuf );
			}
			break ;
			
			case 1 :
			{
			  pServerBuf->addData( ( const void * ) &Mcardcertinfo.szManCertSubjectName,
							  sizeof( Mcardcertinfo.szManCertSubjectName ) );
			  pServerBuf->addData( ( const void * ) &Mcardcertinfo.szManCertIssuerName,
							  sizeof( Mcardcertinfo.szManCertIssuerName ) );
			  pServerBuf->addData( ( const void * ) &Mcardcertinfo.acHostId,
							  sizeof( Mcardcertinfo.acHostId ) );
			  pServerBuf->sendResponse(  );
			  pPODIPCServer->disposeBuf( pServerBuf );
			}
			break ;
			case 2 :
			{
			    pServerBuf->addData( ( const void * ) &Mcardcertinfo.bCertAvailable,
							    sizeof( Mcardcertinfo.bCertAvailable ) );
			    pServerBuf->addData( ( const void * ) &Mcardcertinfo.bCertValid,
							    sizeof( Mcardcertinfo.bCertValid ) );
			    pServerBuf->addData( ( const void * ) &Mcardcertinfo.bVerifiedWithChain,
							    sizeof( Mcardcertinfo.bVerifiedWithChain ) );
			    pServerBuf->addData( ( const void * ) &Mcardcertinfo.bIsProduction,
							    sizeof( Mcardcertinfo.bIsProduction ) );
			    pServerBuf->sendResponse(  );
			    
			    pPODIPCServer->disposeBuf( pServerBuf );
		      }
		      break ;
		      default :
			  RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD","%s : IPC called POD_SERVER_CMD_GET_CABLE_CARD_CERT_INFO \n", __FUNCTION__ );
			  break ;
		    }
			
		}
		break;
		
		case POD_SERVER_CMD_GET_CCI_INFO_ELEMENT :
		{
			int32_t result_recv = 0;
			uint32_t ret = 0;
			uint32_t LTSID = 0 ;
			uint32_t programNumber = 0;
			CardManagerCCIData_t CCIData ;
			
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD","%s : IPC called POD_SERVER_CMD_GET_CCI_INFO_ELEMENT \n",__FUNCTION__ );
			
			result_recv = pServerBuf->collectData( ( void * ) &LTSID, sizeof( uint32_t )  );
			result_recv = pServerBuf->collectData( ( void * ) &programNumber, sizeof( uint32_t )  );
			
			if( result_recv != 0 )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
						  "%s : POD_SERVER_CMD_GET_CCI_INFO_ELEMENT failed due to comm issue: \n", 
						  __FUNCTION__ );
				pServerBuf->addResponse( RMF_OSAL_ENODATA);
				pServerBuf->sendResponse(  );				
				pPODIPCServer->disposeBuf( pServerBuf );				
				break;
			}
			
			ret = GetCCIInfoElement(&CCIData,LTSID,programNumber);
			
			pServerBuf->addResponse( ret );
			pServerBuf->addData( &CCIData, sizeof(CCIData) );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
		}
		break;
		case POD_SERVER_CMD_GET_HOST_SECURITY_ID:    
		{
			uint32_t ret = 0;
			HostCATypeInfo_t hostCaTypeInfo;
			cCardManagerIF *pCM = NULL;
			
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD","%s : IPC called POD_SERVER_CMD_GET_HOST_SECURITY_ID \n",__FUNCTION__ );
			
			pCM = cCardManagerIF::getInstance();
			
			if( pCM == NULL )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s - cCardManagerIF::getInstance() failed.\n", __FUNCTION__);
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
				pServerBuf->sendResponse(  );
				pPODIPCServer->disposeBuf( pServerBuf );
				break;
			}
			memset(&hostCaTypeInfo, 0, sizeof( hostCaTypeInfo) );
			ret = pCM->SNMPget_ocStbHostSecurityIdentifier( &hostCaTypeInfo );
			
			pServerBuf->addResponse( ret );
			pServerBuf->addData( &hostCaTypeInfo, sizeof(hostCaTypeInfo) );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );
		}
		break;
		case POD_SERVER_CMD_GET_NUMBER_OF_CC_APPS:
		{
		    RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD","%s : IPC called POD_SERVER_CMD_GET_NUMBER_OF_CC_APPS \n", __FUNCTION__ );
		    uint32_t ret = 0;
		    cCardManagerIF *pCM = NULL;
		    pCM = cCardManagerIF::getInstance();
		    
		    if( pCM == NULL )
		    {
			    RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s - cCardManagerIF::getInstance() failed.\n", __FUNCTION__);
			    pServerBuf->addResponse( RMF_OSAL_ENODATA );
			    pServerBuf->sendResponse(  );
			    pPODIPCServer->disposeBuf( pServerBuf );
			    break;
		    }
		    memset(&appInfo, 0, sizeof(vlCableCardAppInfo_t));
		    ret = pCM->SNMPGetApplicationInfoWithPages(&appInfo);
		    pServerBuf->addResponse( ret );
			pServerBuf->addData( ( const void * ) &appInfo.CardNumberOfApps,
			  		      sizeof( appInfo.CardNumberOfApps ) );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );				
            RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD","[%s:%d]SNMPGetApplicationInfoWithPages returned"
										" CardNumberOfApps=%d\n", __FUNCTION__, __LINE__, appInfo.CardNumberOfApps);
		}
		break;
		case POD_SERVER_CMD_GET_APP_INFO_WITH_PAGES:    
		{
		    int32_t result_recv = 0;
		    uint32_t ret = 0;
		    //vlCableCardAppInfo_t appInfo;
		    int32_t mmiPages = 0 ;
		    RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD","%s : IPC called POD_SERVER_CMD_GET_APP_INFO_WITH_PAGES \n", __FUNCTION__ );
		    result_recv = pServerBuf->collectData( (void *)&mmiPages,sizeof( mmiPages ) );

            if( result_recv != 0 )
            {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
                          "%s : POD_SERVER_CMD_GET_APP_INFO_WITH_PAGES failed due to comm issue: \n",
                          __FUNCTION__ );

                pServerBuf->addResponse( RMF_OSAL_ENODATA);
                pServerBuf->sendResponse(  );
                pPODIPCServer->disposeBuf( pServerBuf );
                break;
            }

			{
                RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD","[%s:%d] request for app[%d] \n",  __FUNCTION__, __LINE__, mmiPages);
				if(mmiPages >= appInfo.CardNumberOfApps)
				{
				    pServerBuf->addResponse( RMF_OSAL_ENODATA);
				    pServerBuf->sendResponse(  );				
				    pPODIPCServer->disposeBuf( pServerBuf );				
				    break;
				}
#if 0
		    	result_recv |= pServerBuf->collectData( (void *)&mmiPages,sizeof( mmiPages ) );
		
		    	if( result_recv < 0 )
		    	{
				    RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
						      "%s : POD_SERVER_CMD_GET_APP_INFO_WITH_PAGES failed due to comm issue: \n", 
						      __FUNCTION__ );

				    pServerBuf->addResponse( RMF_OSAL_ENODATA);
				    pServerBuf->sendResponse(  );				
				    pPODIPCServer->disposeBuf( pServerBuf );				
				    break;
		    	}
#endif
		    	pServerBuf->addResponse( ret );

		    	//if(mmiPages < 7)
		    	{
				      pServerBuf->addData( ( const void * ) &appInfo.Apps[mmiPages].AppName,
					      sizeof( appInfo.Apps[mmiPages].AppName) );
				      pServerBuf->addData( ( const void * ) &appInfo.Apps[mmiPages].AppType,
								      sizeof( appInfo.Apps[mmiPages].AppType ) );
				      pServerBuf->addData( ( const void * ) &appInfo.Apps[mmiPages].VerNum,
								      sizeof( appInfo.Apps[mmiPages].VerNum ) );
				      pServerBuf->addData( ( const void * ) &appInfo.Apps[mmiPages].AppNameLen,
								      sizeof( appInfo.Apps[mmiPages].AppNameLen ) );
				      pServerBuf->addData( ( const void * ) &appInfo.Apps[mmiPages].AppUrlLen,
								      sizeof( appInfo.Apps[mmiPages].AppUrlLen ) );
				      pServerBuf->addData( ( const void * ) &appInfo.Apps[mmiPages].nSubPages,
								      sizeof( appInfo.Apps[mmiPages].nSubPages ) );
				      pServerBuf->addData( ( const void * ) &appInfo.Apps[mmiPages].AppUrl,
								      sizeof( appInfo.Apps[mmiPages].AppUrl ) );
				      
				      pServerBuf->sendResponse(  );
				      pPODIPCServer->disposeBuf( pServerBuf );
		    	}
			}
	  }
	  break;
        case POD_SERVER_CMD_GET_CABLE_CARD_VERSION:
		{
		    RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD","%s : IPC called POD_SERVER_CMD_GET_CABLE_CARD_VERSION \n", __FUNCTION__ );
		    uint32_t ret = 0;
		    cCardManagerIF *pCM = NULL;
		    pCM = cCardManagerIF::getInstance();
		    
		    if( pCM == NULL )
		    {
			    RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s - cCardManagerIF::getInstance() failed.\n", __FUNCTION__);
			    pServerBuf->addResponse( RMF_OSAL_ENODATA );
			    pServerBuf->sendResponse(  );
			    pPODIPCServer->disposeBuf( pServerBuf );
			    break;
		    }
            vlCableCardVersionInfo_t cardVersion;
		    memset(&cardVersion, 0, sizeof(cardVersion));
		    ret = vlMCardGetCardVersionNumberFromMMI(cardVersion.szCardVersion, sizeof(cardVersion.szCardVersion));
            if(ret > 0)
            {
                pServerBuf->addResponse(RMF_SUCCESS);
            }
            else
            {
                pServerBuf->addResponse(RMF_OSAL_ENODATA);
            }
			pServerBuf->addData( ( const void * ) &cardVersion,
			  		      sizeof(cardVersion) );
			pServerBuf->sendResponse(  );
			pPODIPCServer->disposeBuf( pServerBuf );				
            RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD","[%s:%d]vlMCardGetCardVersionNumberFromMMI returned '%s'\n", __FUNCTION__, __LINE__, cardVersion.szCardVersion);
		}
		break;
		case POD_SERVER_CMD_NOTIFY_SAS_TERMINATION:
		{
			
		   uint16_t manufacturerId = 0;
		   unsigned short CaSystemId;
		   RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
			   "%s : IPC called for vod crashed \n", __FUNCTION__ );
		   podImplGetCaSystemId(&CaSystemId);
		   ret = podClearApduQ( CaSystemId, PROXY_APDU_INDEX - apdu_count );
		   if(RMF_SUCCESS == podmgrGetManufacturerId( &manufacturerId ))
		   {
				if(manufacturerId == CARD_TYPE_MOTO )
				{
					gbSASTerminated = true;
					RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					"%s: Setting SAS Termination \n",
					__FUNCTION__);					
				}
		   }
		   pServerBuf->addResponse( ret );
		   pServerBuf->sendResponse(  );
		   pPODIPCServer->disposeBuf( pServerBuf );
		}
		break;
#endif //SNMP_IPC_CLIENT
/*End :: SNMP Client **************/	  
	  default:
		  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				   "%s: unable to identify the identifyer from packet = %x",
				   __FUNCTION__, identifyer );
		  pPODIPCServer->disposeBuf( pServerBuf );
		  break;
	}

}

void podSIMGREventDispatchThread( void *data )
{
	dispatchThreadData *pTdata = (dispatchThreadData *) data;
	rmf_Error ret = RMF_SUCCESS; 
	rmf_osal_event_handle_t event_handle;
	rmf_osal_event_category_t event_category;
	uint32_t event_type;	
	rmf_osal_event_params_t event_params;
	int32_t timeout_usec = 1000 * 1000 * 60;
	
	ret = rmf_osal_eventqueue_get_next_event_timed(
									pTdata->eQhandle,
									&event_handle,
									&event_category,
									&event_type,
									&event_params,
									timeout_usec);
	if( RMF_SUCCESS != ret )
	{
		/* SIMGR  RDK_LOG_INFO -> RDK_LOG_DEBUG */
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
			   "%s: No event in Q\n",__FUNCTION__ );		
		pTdata->pServerBuf->addResponse(  POD_RESP_NO_EVENT );
	}
	else
	{
		switch (event_type)
		{
			/* SIMGR  Add the events here  
			case DEFINE_AN_EVENT:
			break;
			*/
			default :
			{
				/* SIMGR  
				
				uint32_t param = event_params.data;
				pTdata->pServerBuf->addResponse(  POD_RESP_GEN_EVENT );
				
				pTdata->pServerBuf->addData( ( const void * ) &param,
												sizeof( param) );
				pTdata->pServerBuf->addData( ( const void * ) &param,
												sizeof( param) );
				*/
			}
			break;
		}
		rmf_osal_event_delete( event_handle );
	}
	
	pTdata->pServerBuf->sendResponse(  );	
	pTdata->pPODIPCServer->disposeBuf( pTdata->pServerBuf );
	rmf_osal_memFreeP( RMF_OSAL_MEM_POD, pTdata );
	return;
	
}
void podEventDispatchThread( void *data )
{
	dispatchThreadData *pTdata = (dispatchThreadData *) data;
	rmf_Error ret = RMF_SUCCESS; 
	rmf_osal_event_handle_t event_handle;
	rmf_osal_event_category_t event_category;
	uint32_t event_type;	
	rmf_osal_event_params_t event_params;
	int32_t timeout_usec = 1000 * 1000 * 60;

	ret = rmf_osal_eventqueue_get_next_event_timed(
									pTdata->eQhandle,
									&event_handle,
									&event_category,
									&event_type,
									&event_params,
									timeout_usec);
	if( RMF_SUCCESS != ret )
	{
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
			   "%s: No event in Q\n",__FUNCTION__ );		
		pTdata->pServerBuf->addResponse(  POD_RESP_NO_EVENT );
	}
	else
	{
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
			   "%s: got an event %x\n",__FUNCTION__, event_type );		
		switch (event_type)
		{
			case RMF_PODSRV_DECRYPT_EVENT_CANNOT_DESCRAMBLE_ENTITLEMENT:
			case RMF_PODSRV_DECRYPT_EVENT_CANNOT_DESCRAMBLE_RESOURCES:
			case RMF_PODSRV_DECRYPT_EVENT_MMI_PURCHASE_DIALOG:
			case RMF_PODSRV_DECRYPT_EVENT_MMI_TECHNICAL_DIALOG:
			case RMF_PODSRV_DECRYPT_EVENT_FULLY_AUTHORIZED:
			case RMF_PODSRV_DECRYPT_EVENT_SESSION_SHUTDOWN:
			case RMF_PODSRV_DECRYPT_EVENT_POD_REMOVED:
			case RMF_PODSRV_DECRYPT_EVENT_RESOURCE_LOST:
			case RMF_PODSRV_DECRYPT_EVENT_CCI_STATUS:
			{
				//uint32_t data;
				//data = (uint32_t) event_params.data;

				pTdata->pServerBuf->addResponse(  POD_RESP_DECRYPT_EVENT );
				pTdata->pServerBuf->addData( ( const void * ) &event_type,
												sizeof( event_type ) );	
				qamEventData_t *eventData = (qamEventData_t *)event_params.data;
				pTdata->pServerBuf->addData( ( const void * ) &eventData->cci,
												sizeof( eventData->cci ) );
				pTdata->pServerBuf->addData( ( const void * ) &eventData->handle,
												sizeof( eventData->handle ) );
				pTdata->pServerBuf->addData( ( const void * ) &eventData->qamContext,
												sizeof( eventData->qamContext ) );
			}
		 	break;	
			case RMF_PODSRV_EVENT_DSGIP_ACQUIRED:
			{
				pTdata->pServerBuf->addResponse(  POD_RESP_GEN_EVENT );
				pTdata->pServerBuf->addData( ( const void * ) &event_type,
												sizeof( event_type ) );	
				strncpy( (char*)podmgrDSGIP, (char*)event_params.data, sizeof(podmgrDSGIP)-1 );
				pTdata->pServerBuf->addData( ( const void * ) event_params.data, RMF_PODMGR_IP_ADDR_LEN );				
			}	
			break;

#ifdef ENABLE_TIME_UPDATE
			case RMF_SI_EVENT_STT_TIMEZONE_SET:
                        {
				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
						"%s: got RMF_SI_EVENT_STT_TIMEZONE_SET event in OOB SI Q\n",__FUNCTION__ );
				rmf_timeinfo_t *pTimeInfo = (rmf_timeinfo_t*)event_params.data;
				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
						"%s: got RMF_SI_EVENT_STT_TIMEZONE_SET event in OOB SI Q: pTimeInfo: 0x%x\n",__FUNCTION__ , pTimeInfo);
				pTdata->pServerBuf->addResponse(  POD_RESP_TIMEZONE_SET_EVENT );
				pTdata->pServerBuf->addData( ( const void * ) &(pTimeInfo->timezoneinfo.timezoneinhours), sizeof( pTimeInfo->timezoneinfo.timezoneinhours ) );
				pTdata->pServerBuf->addData( ( const void * ) &(pTimeInfo->timezoneinfo.daylightflag), sizeof( pTimeInfo->timezoneinfo.daylightflag ) );

				rmf_osal_memFreeP(RMF_OSAL_MEM_SI_OOB, pTimeInfo);

				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
						"\nSending POD_RESP_TIMEZONE_SET_EVENT to rmfStreamer process. %s:%d\n",
						__FUNCTION__, __LINE__ );
                        }
                        break;

                        case RMF_SI_EVENT_STT_SYSTIME_SET:
			{
				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
						"%s: got RMF_SI_EVENT_STT_SYSTIME_SET event in OOB SI Q\n",__FUNCTION__ );
				rmf_timeinfo_t *pTimeInfo = (rmf_timeinfo_t*)event_params.data;
				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
						"%s: got RMF_SI_EVENT_STT_SYSTIME_SET event in OOB SI Q: pTimeInfo: 0x%x\n",__FUNCTION__ , pTimeInfo);
				//sendConfigTimeofDayCmd(pTimeInfo->timevalue);
				pTdata->pServerBuf->addResponse(  POD_RESP_SYSTIME_SET_EVENT );
				pTdata->pServerBuf->addData( ( const void * ) &(pTimeInfo->timevalue), sizeof( pTimeInfo->timevalue ) );
				rmf_osal_memFreeP(RMF_OSAL_MEM_SI_OOB, pTimeInfo);

				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
						"\nSending POD_RESP_SYSTIME_SET_EVENT to rmfStreamer process. %s:%d\n",
						__FUNCTION__, __LINE__);
			}
			break;

#endif
			/* not propogted directly  */
			case CardMgr_Card_Detected:
			case CardMgr_Card_Removed:
			case CardMgr_Card_Ready:	
			case CardMgr_CP_CCI_Changed:		
			case CardMgr_CP_CCI_Error:		
			case CardMgr_Generic_Feature_Changed:
			case CardMgr_DSG_Cable_Card_VCT_ID:
			case CardMgr_DSG_UCID:
			case CardMgr_Device_In_Non_DSG_Mode:
			case CardMgr_CA_Encrypt_Flag:
			case CardMgr_CA_Pmt_Reply:
			case CardMgr_CP_DHKey_Changed:
			case CardMgr_CA_Update:				
				pTdata->pServerBuf->addResponse(  POD_RESP_NO_EVENT );
			break;
			
			case CardMgr_DSG_Entering_DSG_Mode:
			case CardMgr_DSG_Leaving_DSG_Mode:
			case CardMgr_DOCSIS_TLV_217:
			case CardMgr_DSG_DownStream_Scan_Failed:
			case CardMgr_POD_IP_ACQUIRED:
            case CardMgr_DSG_IP_ACQUIRED:
			case CardMgr_Card_Mib_Access_Root_OID:
			case CardMgr_Card_Mib_Access_Snmp_Reply:
			case CardMgr_Card_Mib_Access_Shutdown:
			case CardMgr_OOB_Downstream_Acquired:
			case CardMgr_DSG_Operational_Mode:
			case CardMgr_DSG_Downstream_Acquired:
			case CardMgr_VCT_Channel_Map_Acquired:
			case CardMgr_CP_Res_Opened:
			case CardMgr_Host_Auth_Sent:
			case CardMgr_Bnd_Fail_Card_Reasons:
			case CardMgr_Bnd_Fail_Invalid_Host_Cert:
			case CardMgr_Bnd_Fail_Invalid_Host_Sig:
			case CardMgr_Bnd_Fail_Invalid_Host_AuthKey:
			case CardMgr_Bnd_Fail_Other:
			case CardMgr_Card_Val_Error_Val_Revoked:
			case CardMgr_Bnd_Fail_Incompatible_Module:
			case CardMgr_Bnd_Comp_Card_Host_Validated:
			case CardMgr_CP_Initiated:
			case CardMgr_Card_Image_DL_Complete:
			case CardMgr_CDL_Host_Img_DL_Cmplete:
			case CardMgr_CDL_CVT_Error:
			case CardMgr_CDL_Host_Img_DL_Error:
			{			
				uint8_t data_present = 0;
				pTdata->pServerBuf->addResponse(  POD_RESP_CORE_EVENT );
				pTdata->pServerBuf->addData( ( const void * ) &event_type,
												sizeof( event_type ) );
				pTdata->pServerBuf->addData( ( const void * ) &event_params.data_extension,
												sizeof( event_params.data_extension ) );				
				if ( event_params.data != NULL )
				{
					data_present = 1;	
					pTdata->pServerBuf->addData( ( const void * ) &data_present,
																		sizeof( data_present ) );					
					pTdata->pServerBuf->addData( ( const void * ) event_params.data,
													event_params.data_extension );					
				}
				else
				{
					data_present = 0;	
					pTdata->pServerBuf->addData( ( const void * ) &data_present,
																		sizeof( data_present ) );
				}
				RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
						"%s:%d: Sending Core event %d \n",
						__FUNCTION__, __LINE__, event_type);
			}
			break;
			default:
			{
				uint32_t data;
				data = (uint32_t) event_params.data;
				
				pTdata->pServerBuf->addResponse(  POD_RESP_GEN_EVENT );
				pTdata->pServerBuf->addData( ( const void * ) &event_type,
												sizeof( event_type ) );
				pTdata->pServerBuf->addData( ( const void * ) &data,
												sizeof( data ) );
				pTdata->pServerBuf->addData( ( const void * ) &event_params.data_extension,
												sizeof( event_params.data_extension ) );					
			}
			break;
		}

		rmf_osal_event_delete( event_handle );
		
	}

	pTdata->pServerBuf->sendResponse(  );	
	pTdata->pPODIPCServer->disposeBuf( pTdata->pServerBuf );
	rmf_osal_memFreeP( RMF_OSAL_MEM_POD, pTdata );
	return;
	
}


void thread_recv_apdu( void *param )
{
	uint32_t sessionId = 0;
	int32_t length = 0;
	uint32_t identifyer = 0;
	uint8_t *apdu = NULL;
	IPCServerBuf *pServerBuf = NULL;
	recvAPDU *stRecvAPDU = ( recvAPDU * ) param;
	rmf_Error ret;

	RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.POD",
			 "%s: starts \n", __FUNCTION__ );
	pServerBuf = stRecvAPDU->pServerBuf;

	ret = rmf_podmgrReceiveAPDU( &sessionId, &apdu, &length );
	if ( RMF_SUCCESS != ret )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "%s: Error while recving APDU\n",
				 __FUNCTION__);
	}
	else
	{
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD", 
			"%s: APDU length is : %d\n",
				 __FUNCTION__, length );
	}

	if((apdu == NULL) && (sessionId == 0) && (length == 0 ))
	{
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
			"%s: Thread returned with dummy APDU\n",
				 __FUNCTION__);	
		stRecvAPDU->pIPCServer->disposeBuf( pServerBuf );
		rmf_osal_memFreeP( RMF_OSAL_MEM_POD, stRecvAPDU );
		return;
	}

	if((true == gbCacheProxyAPDU) || ( gbSASTerminated == true ))
	{
		apdu_count++;
	}

	if((true == gbCacheProxyAPDU) && (PROXY_APDU_INDEX == apdu_count))
	{
		uint8_t *apduNew = NULL;
		gbCacheProxyAPDU = false;
		apdu_count = 0;
		
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
			"%s: Caching the APDU\n",
				 __FUNCTION__);			
		rmf_osal_memAllocP( RMF_OSAL_MEM_POD, length,
			( void ** ) &apduNew );
		memcpy(apduNew, apdu, length);
		podProxyApdu(&sessionId, &apduNew, &length, 1);
	}

	if(( gbSASTerminated == true ) &&
		(apdu_count == PROXY_APDU_INDEX-1 ) && 
			(sessionCache[PROXY_SESSION_ID_INDEX].appId != 0) )
	{
		uint32_t sessionIdTemp =0;
		uint8_t *apduTemp =NULL;
		int32_t lengthTemp =0;
		uint8_t *apduNew = NULL;
		apdu_count = 0;
		gbSASTerminated = false;
		
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
			"%s: Inserting the cached the APDU\n",
				 __FUNCTION__);				
		podProxyApdu(&sessionIdTemp, &apduTemp, &lengthTemp, 0);
		rmf_osal_memAllocP( RMF_OSAL_MEM_POD, lengthTemp,
			( void ** ) &apduNew );
		memcpy(apduNew, apduTemp, lengthTemp);
		podInsertAPDU(sessionIdTemp,
			apduNew, lengthTemp);
	}
	
	pServerBuf->addResponse( identifyer );
	pServerBuf->addData( ( const void * ) &sessionId, sizeof( sessionId ) );
	pServerBuf->addData( ( const void * ) &length, sizeof( length ) );
	pServerBuf->addData( ( const void * ) &apdu[0], length );
	pServerBuf->sendResponse(  );
	stRecvAPDU->pIPCServer->disposeBuf( pServerBuf );
	rmf_osal_memFreeP( RMF_OSAL_MEM_POD, stRecvAPDU );
	rmf_podmgrReleaseAPDU( apdu );

}

#ifdef ENABLE_TIME_UPDATE
#if 0
int sendConfigTimezoneCmd(int timezoneinhours,int daylightflag)
{
        rmf_Error ret = RMF_SUCCESS;
        int32_t result_recv = 0;
        int8_t res = 0;
        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.SI",
                        "\nEnter %s:%d: timezoneinhours: %d, daylightflag: %d\n",
                        __FUNCTION__, __LINE__ , timezoneinhours, daylightflag);

        IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_HAL_SERVER_CMD_CONFIG_TIMEZONE);
        result_recv |= ipcBuf.addData( ( const void * ) &timezoneinhours, sizeof( timezoneinhours ) );
        result_recv |= ipcBuf.addData( ( const void * ) &daylightflag, sizeof( daylightflag ) );

        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SI",
                        "\nPOD Comm add data failed in %s:%d: %d\n",
                                 __FUNCTION__, __LINE__ , result_recv);
                return RMF_OSAL_ENODATA;
        }

        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.SI",
                        "\nSending POD_HAL_SERVER_CMD_CONFIG_TIMEZONE to rmfStreamer process. %s:%d: %d\n",
                        __FUNCTION__, __LINE__ , result_recv);

        res = ipcBuf.sendCmd( pod_server_cmd_port_no );

        if ( 0 != res )
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SI",
                        "POD Comm failure in %s:%d: ret = %d\n",
                                 __FUNCTION__, __LINE__, res );
                return RMF_OSAL_ENODATA;
        }

        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.SI",
                        "\nWaiting for response from rmfStreamer process. %s:%d: %d\n",
                        __FUNCTION__, __LINE__ , result_recv);

        result_recv = ipcBuf.getResponse( &ret );
        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SI",
                        "POD Comm get response failed in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, result_recv );
                return RMF_OSAL_ENODATA;
        }

        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.SI",
                        "\nExit. %s:%d\n",
                        __FUNCTION__, __LINE__);

        return ret;
}

int sendConfigTimeofDayCmd(int64_t timevalue)
{
        rmf_Error ret = RMF_SUCCESS;
        int32_t result_recv = 0;
        int8_t res = 0;
        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.SI",
                        "\nEnter %s:%d: timevalule: %lld, POD_HAL_SERVER_CMD_CONFIG_TIMEOFDAY: %d\n",
                        __FUNCTION__, __LINE__ , timevalue, POD_HAL_SERVER_CMD_CONFIG_TIMEOFDAY);

        IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_HAL_SERVER_CMD_CONFIG_TIMEOFDAY);
        result_recv |= ipcBuf.addData( ( const void * ) &timevalue, sizeof( timevalue ) );

        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SI",
                        "\nPOD Comm add data failed in %s:%d: %d\n",
                                 __FUNCTION__, __LINE__ , result_recv);
                return RMF_OSAL_ENODATA;
        }

        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.SI",
                        "\nSending POD_HAL_SERVER_CMD_CONFIG_TIMEOFDAY to rmfStreamer process. %s:%d: %d\n",
                        __FUNCTION__, __LINE__ , result_recv);

        res = ipcBuf.sendCmd( pod_server_cmd_port_no );

        if ( 0 != res )
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SI",
                        "POD Comm failure in %s:%d: ret = %d\n",
                                 __FUNCTION__, __LINE__, res );
                return RMF_OSAL_ENODATA;
        }

        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.SI",
                        "\nWaiting for response from rmfStreamer process. %s:%d: %d\n",
                        __FUNCTION__, __LINE__ , result_recv);

        result_recv = ipcBuf.getResponse( &ret );
        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SI",
                        "POD Comm get response failed in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, result_recv );
                return RMF_OSAL_ENODATA;
        }

        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.SI",
                        "\nExit. %s:%d\n",
                        __FUNCTION__, __LINE__);

        return ret;
}

void oobSIEventHandlerThread( void *param )
{
        rmf_Error ret = RMF_SUCCESS;
        rmf_osal_event_handle_t event_handle;
        rmf_osal_event_category_t event_category;
        uint32_t event_type;
	bool thread_exit = FALSE;
        rmf_osal_event_params_t event_params;
	rmf_osal_eventqueue_handle_t eOobSIQhandle = (rmf_osal_eventqueue_handle_t)param;

	while(!thread_exit)
	{
		ret = rmf_osal_eventqueue_get_next_event(
				eOobSIQhandle,
				&event_handle,
				&event_category,
				&event_type,
				&event_params);
		if( RMF_SUCCESS != ret )
		{
			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
					"%s: No event in OOB SI Q\n",__FUNCTION__ );
		}
		else
		{
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
					"%s: got an event %x in OOB SI Q\n",__FUNCTION__, event_type );

			switch (event_type)
			{
				case RMF_SI_EVENT_STT_TIMEZONE_SET:
				{
					RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
						"%s: got RMF_SI_EVENT_STT_TIMEZONE_SET event in OOB SI Q\n",__FUNCTION__ );
					rmf_timeinfo_t *pTimeInfo = (rmf_timeinfo_t*)event_params.data;
					RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
						"%s: got RMF_SI_EVENT_STT_TIMEZONE_SET event in OOB SI Q: pTimeInfo: 0x%x\n",__FUNCTION__ , pTimeInfo);
					sendConfigTimezoneCmd(pTimeInfo->timezoneinfo.timezoneinhours, pTimeInfo->timezoneinfo.daylightflag);
					rmf_osal_memFreeP(RMF_OSAL_MEM_SI_OOB, pTimeInfo);					
				}
				break;

				case RMF_SI_EVENT_STT_SYSTIME_SET:
				{
					RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
						"%s: got RMF_SI_EVENT_STT_SYSTIME_SET event in OOB SI Q\n",__FUNCTION__ );
					rmf_timeinfo_t *pTimeInfo = (rmf_timeinfo_t*)event_params.data;
					RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
						"%s: got RMF_SI_EVENT_STT_SYSTIME_SET event in OOB SI Q: pTimeInfo: 0x%x\n",__FUNCTION__ , pTimeInfo);
					sendConfigTimeofDayCmd(pTimeInfo->timevalue);					
					rmf_osal_memFreeP(RMF_OSAL_MEM_SI_OOB, pTimeInfo);	
				}
				break;

				case POD_SERVER_CMD_UNREGISTER_CLIENT:
				{
					RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
							"%s: got POD_SERVER_CMD_UNREGISTER_CLIENT event in OOB SI Q\n",__FUNCTION__ );
                                        g_eOobSiThreadStatus = OOBSI_THREAD_INACTIVE;
					thread_exit = TRUE;
				}
			        break;
	
				default:
				break;
			}

		       rmf_osal_event_delete( event_handle );	
		}
	}

        rmf_OobSiMgr *pOobSiMgr =  rmf_OobSiMgr::getInstance();
        if ( NULL == pOobSiMgr )
        {
            RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s: rmf_OobSiMgr::getInstance() failed.\n", __FUNCTION__);
        }
        else if(RMF_SUCCESS != pOobSiMgr->UnRegisterForSIEvents(RMF_SI_EVENT_STT_TIMEZONE_SET|RMF_SI_EVENT_STT_SYSTIME_SET, eOobSIQhandle))
        {
            RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s: rmf_OobSiMgr::RegisterForSIEvents() failed.\n", __FUNCTION__);
        }
	
	if(RMF_SUCCESS != rmf_osal_eventqueue_delete(eOobSIQhandle) )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					"%s: Failed to delete OOB SI Q\n",__FUNCTION__ );
	}
	eOobSIQhandle = 0;

	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","%s: Posting semaphore: g_pMonitorThreadStopSem \n\n", __FUNCTION__);
	ret = sem_post( g_pMonitorThreadStopSem);
	if (0 != ret )
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD",
				"%s: sem_post failed.\n", __FUNCTION__);
	}

}
#endif
#endif
