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
#include <unistd.h>
#include<stdlib.h>
#include<string.h>
#include "rmf_osal_ipc.h"
#include "rmf_pod.h"
#include "rmf_inbsimgr.h"
#include "canhClient.h"
#include "rmfcore.h"
#include "disconnectMgr.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef GLI
#include "sysMgr.h"
#include "libIBus.h"
#endif

#define CANH_CLIENT_DFLT_PORT_NO 50501
#define VOD_CRASH_MONPIPE "/tmp/vod_crash_monpipe"
#define FORCED_VOD_CRASH_MONPIPE "/tmp/froced_vod_crash_monpipe"
#define VOD_CRASHED "VOD_CRASHED"
#define VOD_FORCED_CRASH "VOD_FORCED_CRASH"
#define CANH_CLOSED "CANH_CLOSED"
extern rmf_Error dispatchEntitementEvent( rmf_osal_event_params_t *pEventData );
static rmf_Error canhGetPlantId( uint8_t *pPlantId );
static volatile uint32_t canh_server_port_no;
static volatile uint32_t canh_client_port_no;

static const uint32_t COMMAND_START_CANH = 6;
static const uint32_t COMMAND_GET_CANH_STATUS = 7;
static const uint32_t COMMAND_START_MONITOR_EID = 8;
static const uint32_t COMMAND_STOP_MONITOR_EID = 9;
static const uint32_t COMMAND_STOP_CANH = 10;
static const uint32_t COMMAND_IS_RESOURCE_AUTHORIZED = 11;
static const uint32_t COMMAND_GET_PLANTID = 12;
static const uint32_t COMMAND_IS_SOURCE_AUTHORIZED = 13;
static const uint32_t CANH_STATUS_READY = 33;
static const uint32_t COMMAND_CANH_CARD_REMOVED = 22;
static const uint32_t COMMAND_CANH_POD_READY = 23;
static const uint32_t COMMAND_CANH_CARD_RESET_PENDING =24;

static rmf_osal_eventmanager_handle_t canh_eventmanager_handle;

void recv_canh_cmd( IPCServerBuf * pServerBuf, void *pUserData );
static rmf_Error canhStartMonitors(  );
static void canhStateMonitorThread( void *param );
static void canhPODMonitorThread( void *param );
	
IPCServer *pPPVIPCServer = NULL;

static rmf_osal_Bool canh_started;
static rmf_osal_Bool pod_mon_started;
static rmf_osal_Bool canh_restarted;

void freefn( void *data );

void freefn( void *data )
{
	rmf_osal_memFreeP( RMF_OSAL_MEM_IPPV, data );
}


rmf_Error canhClientStart(	)
{
	const char *canh_client;
	const char *canh_server;
	rmf_Error ret = RMF_SUCCESS;
	int8_t ipc_ret = 0;

	canh_client = rmf_osal_envGet( "CANH_CLIENT_PORT_NO" );
	if( NULL == canh_client )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV", "%s: NULL POINTER  received for name string\n", __FUNCTION__ );
		canh_client_port_no = CANH_CLIENT_DFLT_PORT_NO;
	}
	else
	{
		canh_client_port_no = atoi( canh_client ); 
	}
	canh_server = rmf_osal_envGet( "CANH_SERVER_PORT_NO" );
	if( NULL == canh_server )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV", "%s: NULL POINTER  received for name string\n", __FUNCTION__);
		canh_server_port_no = CANH_SERVER_DFLT_PORT_NO;
	}
	else
	{
		canh_server_port_no = atoi( canh_server );
	}

	ipc_ret = mkfifo( VOD_CRASH_MONPIPE, 0666 );
	if( 0 != ipc_ret )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV", "%s: Could not create FIFO \n", __FUNCTION__);
		return RMF_RESULT_FAILURE;		
	}

	ipc_ret = mkfifo( FORCED_VOD_CRASH_MONPIPE, 0666 );
	if( 0 != ipc_ret )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV", "%s: Could not create FORCED_VOD_CRASH_MONPIPE FIFO \n", __FUNCTION__);
		return RMF_RESULT_FAILURE;		
	}
	
	ret = canhStartMonitors();
	if( RMF_SUCCESS != ret )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV", "%s : Monitors failed to start\n",
				 __FUNCTION__ );
		return RMF_RESULT_FAILURE;
	}

	ret =
		rmf_osal_eventmanager_create( &canh_eventmanager_handle );
	pPPVIPCServer = new IPCServer( ( int8_t * ) "ippvServer", canh_client_port_no );
	ipc_ret = pPPVIPCServer->start(  );

	if ( 0 != ipc_ret )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV", "%s : Server failed to start\n",
				 __FUNCTION__ );
		return RMF_RESULT_FAILURE;
	}

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.IPPV",
			 "%s : Server server1 started successfully\n", __FUNCTION__ );
	pPPVIPCServer->registerCallBk( &recv_canh_cmd, ( void * ) pPPVIPCServer );

	ret = disconnectMgrInit( );
	if ( RMF_SUCCESS != ret )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV", "%s - failed.\n",
				 __FUNCTION__ );
	}
	
	return RMF_SUCCESS;
}

rmf_Error canhClientStop(  )
{
	int pipeFd;
	rmf_Error osal_ret =
		rmf_osal_eventmanager_delete( canh_eventmanager_handle );
	
	int8_t ret = pPPVIPCServer->stop(	);

	if ( 0 != ret )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV", "%s : Server failed to stop \n",
				 __FUNCTION__ );
		return RMF_RESULT_FAILURE;
	}

	pipeFd = open( VOD_CRASH_MONPIPE, O_WRONLY );
	write( pipeFd, CANH_CLOSED, sizeof(CANH_CLOSED)-1 );
	close( pipeFd );
	unlink( VOD_CRASH_MONPIPE );

	pipeFd = open( FORCED_VOD_CRASH_MONPIPE, O_WRONLY );
	write( pipeFd, CANH_CLOSED, sizeof(CANH_CLOSED)-1 );
	close( pipeFd );
	unlink( FORCED_VOD_CRASH_MONPIPE );

	
	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.IPPV",
			 "%s : Server server1 stoped successfully\n", __FUNCTION__ );
	return osal_ret;

}

rmf_Error canhClientRegisterEvents( rmf_osal_eventqueue_handle_t queueId )
{
	rmf_Error ret;
	ret =
		rmf_osal_eventmanager_register_handler( canh_eventmanager_handle,
												queueId,
												RMF_OSAL_EVENT_CATEGORY_IPPV );
	if ( RMF_SUCCESS != ret )
	{
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.IPPV", "%s: returning with Error \n",
				 __FUNCTION__, ret );
		return ret;
	}
	return ret;
}

rmf_Error canhClientUnRegisterEvents( rmf_osal_eventqueue_handle_t queueId )
{
	rmf_Error osal_ret;
	int32_t ret = 0;
	rmf_osal_event_handle_t event_handle;

	osal_ret =
		rmf_osal_eventmanager_unregister_handler( canh_eventmanager_handle,
												  queueId );

	rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_IPPV,
						   CANH_CLIENT_TERM_EVENT, NULL, &event_handle );

	/* to prevent quiting all monitors interested on same eid */
	rmf_osal_eventqueue_add( queueId, event_handle );

	if ( RMF_SUCCESS != ret )
	{
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.IPPV", "%s: returning with Error \n",
				 __FUNCTION__, ret );
		return RMF_RESULT_FAILURE;
	}

	return osal_ret;

}

void recv_canh_cmd( IPCServerBuf * pServerBuf, void *pUserData )
{
	uint32_t identifyer = 0;
	rmf_osal_event_handle_t event_handle;
	rmf_osal_event_params_t eventData;
	uint8_t isAuthorized = 0 ;
	int64_t eid = 0;
	int32_t result = 0;

	identifyer = pServerBuf->getCmd(  );
	IPCServer *pIPCServer = ( IPCServer * ) pUserData;

	switch ( identifyer )
	{
		case VOD_SERVER_ID_RECVD_EVENT:
		  {	
		  		
                       int32_t vodId = 0;			  			  				

			  pServerBuf->collectData( ( void * ) &vodId,
									   sizeof( vodId ) );
			  RDK_LOG( RDK_LOG_INFO, "LOG.RDK.IPPV",
					   "%s: vod server id recvd from VOD is %lld", __FUNCTION__, vodId );

			  pServerBuf->addResponse( identifyer );
			  pServerBuf->sendResponse(  );
#ifdef GLI
			IARM_Bus_SYSMgr_EventData_t eventData;
			eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_VOD_AD; 
			eventData.data.systemStates.state = 2; 
			eventData.data.systemStates.error = 0; 
			snprintf( eventData.data.systemStates.payload, sizeof(eventData.data.systemStates.payload),
				"%d", vodId );
			IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
#endif				   	  		  			
	  		  pIPCServer->disposeBuf( pServerBuf );

		  }
			break;
	  case CANH_PPV_AUTHORIZATION_EVENT:
		  {
			  PODEntitlementAuthEvent *pEvent = NULL;

			  rmf_osal_memAllocP( RMF_OSAL_MEM_IPPV, sizeof( PODEntitlementAuthEvent ),
								  ( void ** ) &pEvent );
			  pServerBuf->collectData( ( void * ) &eid, sizeof( eid ) );

			  RDK_LOG( RDK_LOG_INFO, "LOG.RDK.IPPV",
					   "%s: Eid recvd from CANH is %lld\n", __FUNCTION__, eid );
			  pEvent->eId = eid & 0x00000000FFFFFFFF;
			  result =
				  pServerBuf->collectData( ( void * ) &pEvent->isAuthorized,
										   sizeof( pEvent->isAuthorized ) );
			  if ( result != 0 )
			  {
				  pServerBuf->collectData( ( void * ) &pEvent->extendedInfo,
										   sizeof( pEvent->extendedInfo ) );
				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.IPPV", "%s: Purchase status: %d\n", __FUNCTION__, pEvent->extendedInfo );
			  }
			  pServerBuf->addResponse( identifyer );
			  pServerBuf->sendResponse(  );

			  if ( 0 != canh_eventmanager_handle )
			  {
				  eventData.data = pEvent;
				  eventData.data_extension = sizeof( PODEntitlementAuthEvent );
				  eventData.free_payload_fn = freefn;
				  eventData.priority = 0;
				  rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_IPPV,
										 CANH_PPV_AUTHORIZATION_EVENT,
										 &eventData, &event_handle );
				  rmf_osal_eventmanager_dispatch_event
					  ( canh_eventmanager_handle, event_handle );
			  }
			  else
			  {
				rmf_osal_memFreeP( RMF_OSAL_MEM_IPPV, pEvent );
			  }
	  		  pIPCServer->disposeBuf( pServerBuf );

		  }
		  break;
	  case CANH_RESOURCE_AUTHORIZATION_EVENT:
		  {
			  int32_t eidType = POD_EID_GENERAL;

			  PODEntitlementAuthEvent *pEvent = NULL;

			  rmf_osal_memAllocP( RMF_OSAL_MEM_IPPV, sizeof( PODEntitlementAuthEvent ),
								  ( void ** ) &pEvent );
			  
			  pServerBuf->collectData( ( void * ) &eid, sizeof( eid ) );
			  pEvent->eId = eid & 0x00000000FFFFFFFF;
			  
			  result =
				  pServerBuf->collectData( ( void * ) &pEvent->isAuthorized,
										   sizeof( pEvent->isAuthorized ) );
			  if ( result != 0 )
			  {
				  pServerBuf->collectData( ( void * ) &pEvent->extendedInfo,
										   sizeof( pEvent->extendedInfo ) );
			  }
			  
			  pServerBuf->addResponse( identifyer );
			  pServerBuf->sendResponse(  );
	  		  pIPCServer->disposeBuf( pServerBuf );

			  RDK_LOG( RDK_LOG_INFO, "LOG.RDK.IPPV",
					   "%s: Eid recvd from CANH is %lld, authorization is :%d, type is %d", 
					   __FUNCTION__, eid, pEvent->isAuthorized, pEvent->extendedInfo );

			  eventData.data = pEvent;
			  eventData.data_extension = sizeof( PODEntitlementAuthEvent );
			  eventData.free_payload_fn = freefn;
			  eventData.priority = 0;
			  dispatchEntitementEvent( &eventData );
#if 0
			  if ( 0 != canh_eventmanager_handle )
			  {
				  rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_IPPV,
										 CANH_RESOURCE_AUTHORIZATION_EVENT,
										 &eventData, &event_handle );
				  rmf_osal_eventmanager_dispatch_event
					  ( canh_eventmanager_handle, event_handle );
				  
			  }
#endif

		  }
		  break;
		  
	  default:
		  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV",
				   "unable to identify the identifyer from packet = 0x%x\n",
				   identifyer );
		  pIPCServer->disposeBuf( pServerBuf );
		  break;
	}
}

rmf_Error canhStartEIDMonitoring( uint32_t eID, canhTargets targetType )
{
	uint32_t response = 0;
	int32_t result_recv;
	uint16_t sourceID = 0;

	if( CANH_TARGET_IPPV == targetType  )
	{
		sourceID = ( 0xFFFF0000 & eID ) >> 16;
	}
	
	if ( !canh_started )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV",
				 "Error!!Canh not started yet." );
		return RMF_OSAL_ENODATA;
	}

	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) COMMAND_START_MONITOR_EID );
	ipcBuf.addData( ( const void * ) &targetType, sizeof( targetType ) );	
	ipcBuf.addData( ( const void * ) &eID, sizeof( eID ) );
	if( CANH_TARGET_IPPV == targetType  )
	{
		ipcBuf.addData( ( const void * ) &sourceID, sizeof( sourceID ) );
	}	
	int8_t res = ipcBuf.sendCmd( canh_server_port_no );

	if ( 0 == res )
	{
		result_recv = ipcBuf.getResponse( &response );
	}
	else
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV", "CANH Comm failure in %s:%d\n",
				 __FUNCTION__, __LINE__ );
		return RMF_OSAL_ENODATA;
	}

	return RMF_SUCCESS;
}

rmf_Error canhStopEIDMonitoring( uint32_t eID, canhTargets targetType )
{
	uint32_t response = 0;
	int32_t result_recv;
	uint16_t sourceID = 0;

	if( CANH_TARGET_IPPV == targetType  )
	{
		sourceID = ( 0xFFFF0000 & eID ) >> 16;
	}

	if ( !canh_started )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV",
				 "Error!!Canh not started yet." );
		return RMF_OSAL_ENODATA;
	}

	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) COMMAND_STOP_MONITOR_EID );
	ipcBuf.addData( ( const void * ) &targetType, sizeof( targetType ) );	
	ipcBuf.addData( ( const void * ) &eID, sizeof( eID ) );
	if( CANH_TARGET_IPPV == targetType  )
	{
		ipcBuf.addData( ( const void * ) &sourceID, sizeof( sourceID ) );
	}	
	int8_t res = ipcBuf.sendCmd( canh_server_port_no );

	if ( 0 == res )
	{
		result_recv = ipcBuf.getResponse( &response );
	}
	else
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV", "CANH Comm failure in %s:%d\n",
				 __FUNCTION__, __LINE__ );
		return RMF_OSAL_ENODATA;
	}

	return RMF_SUCCESS;
}

rmf_Error IsCanhResourceAuthorized( uint32_t eID,
											uint8_t * isAuthorized )
{
	uint32_t response = 0;
	int32_t result_recv;

	if ( !isCANHReady() )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV",
				 "Error!!Canh not started yet." );
		return RMF_OSAL_ENODATA;
	}

	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) COMMAND_IS_RESOURCE_AUTHORIZED );
	ipcBuf.addData( ( const void * ) &eID, sizeof( eID ) );
	int8_t res = ipcBuf.sendCmd( canh_server_port_no );

	if ( 0 == res )
	{
		result_recv = ipcBuf.getResponse( &response );
		result_recv = ipcBuf.collectData( isAuthorized, sizeof( uint8_t ) );
	}
	else
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV", "CANH Comm failure in %s:%d\n",
				 __FUNCTION__, __LINE__ );
		return RMF_OSAL_ENODATA;
	}

	return RMF_SUCCESS;
}

rmf_Error IsCanhSourceAuthorized( uint16_t sourceId,
											uint8_t * isAuthorized )
{
	uint32_t response = 0;
	int32_t result_recv;

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				 "%s: Enter\n", __FUNCTION__ );	
	if ( !isCANHReady() )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "Error!!Canh not started yet." );
		return RMF_OSAL_EINVAL;
	}

	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) COMMAND_IS_SOURCE_AUTHORIZED );
	ipcBuf.addData( ( const void * ) &sourceId, sizeof( sourceId ) );
	int8_t res = ipcBuf.sendCmd( canh_server_port_no );

	if ( 0 == res )
	{
		result_recv = ipcBuf.getResponse( &response );
		result_recv = ipcBuf.collectData( isAuthorized, sizeof( uint8_t ) );
	}
	else
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "CANH Comm failure in %s:%d\n",
				 __FUNCTION__, __LINE__ );
		return RMF_OSAL_EINVAL;
	}

	if( response != 0 ) 
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"Not enough data recvd by CANH for Authorization %s:%d\n",
				 __FUNCTION__, __LINE__ );
		return RMF_OSAL_ENODATA;
	}
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				 "%s: Exit\n", __FUNCTION__ );	

	return RMF_SUCCESS;
}
#ifdef GLI
rmf_Error canhGetPlantId( uint8_t *pPlantId )
{
	uint32_t response = 0;
	int32_t result_recv;
	uint8_t size= 0;

	if ( !canh_started )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV",
				 "Error!!Canh not started yet." );
		return RMF_OSAL_ENODATA;
	}
	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) COMMAND_GET_PLANTID );

	int8_t res = ipcBuf.sendCmd( canh_server_port_no );
	if ( 0 != res )
	{	
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV",
				 "Error!!sendCmd failed %s:%d", __FUNCTION__, __LINE__ );		
		return RMF_OSAL_ENODATA;
	}
	
	result_recv = ipcBuf.getResponse( &response );	
	if( result_recv < 0)			
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV",
				 "Error!!getResponse failed %s:%d", __FUNCTION__, __LINE__ );				
		return RMF_OSAL_ENODATA;
	}

	result_recv |=
		ipcBuf.collectData( &size, sizeof( size ) );

	if( size >= RMF_IPPVSRC_MAX_PLANTID_LEN )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV",
			 "Error!! size >= RMF_IPPVSRC_MAX_PLANTID_LEN" );
		return RMF_OSAL_ENODATA;
	}

	result_recv |=
		ipcBuf.collectData( pPlantId, size );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV",
				 "Error!!collectData failed %s:%d", __FUNCTION__, __LINE__ );			
		return RMF_OSAL_ENODATA;
	}

	pPlantId[ size ] = 0;
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.IPPV",
				 "%s:%d: Plant ID is %s", __FUNCTION__, __LINE__, pPlantId );			
	return RMF_SUCCESS;
	
}
#endif

rmf_Error onCANHRestart( )
{
	canh_started = false;
	RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
			 "%s: Restarting vodclient \n",
			 __FUNCTION__ );
	
	canhStartMonitors(  );
	return RMF_SUCCESS;
}

rmf_Error canhStartMonitors(  )
{
	const char canh_monitor_thread_name[40] = "CANH state monitor thread";
	const char pod_monitor_thread_name[40] = "POD state monitor thread";
	
	rmf_osal_ThreadId tid_canh_monitor;
	rmf_osal_ThreadId tid_pod_monitor;
	
	rmf_Error err;

	if ( canh_started )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV",
				 "Error!!Canh is started already." );
		return RMF_SUCCESS;
	}
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				 "canhStartMonitors: Starting\n" );
	//Creating thread for monitoring canh state
	err =
		rmf_osal_threadCreate( canhStateMonitorThread, NULL,
							   RMF_OSAL_THREAD_PRIOR_DFLT,
							   RMF_OSAL_THREAD_STACK_SIZE, &tid_canh_monitor,
							   canh_monitor_thread_name );
	if ( RMF_SUCCESS != err )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV", "Thread creation Failed %s\n",
				 canh_monitor_thread_name );
		return RMF_OSAL_ENODATA;
	}

	if( pod_mon_started )
	{
		return RMF_SUCCESS;
	}
	//Creating thread for monitoring pod state
	err =
		rmf_osal_threadCreate( canhPODMonitorThread, NULL,
							   RMF_OSAL_THREAD_PRIOR_DFLT,
							   RMF_OSAL_THREAD_STACK_SIZE, &tid_pod_monitor,
							   pod_monitor_thread_name );
	if ( RMF_SUCCESS != err )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV", "Thread creation Failed %s\n",
				 pod_monitor_thread_name );
		return RMF_OSAL_ENODATA;
	}

	return RMF_SUCCESS;
}

rmf_Error canhSendOneShotEvent( uint32_t command )
{
	int32_t result_recv;
	uint32_t response;	
	IPCBuf ipcBufStart = IPCBuf( ( uint32_t ) command );

	if ( !isCANHReady() )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "Error!!Canh not started yet." );
		return RMF_OSAL_EINVAL;
	}

	int8_t res = ipcBufStart.sendCmd( canh_server_port_no );
	if ( 0 != res )
	{
		return RMF_OSAL_ENODATA;
	}
	
	result_recv = ipcBufStart.getResponse( &response );
	if ( command != response )
	{
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.IPPV",
					 "Sent canhSendOneShotEvent failed %s:%d\n",
					 __FUNCTION__, __LINE__ );
			return RMF_OSAL_ENODATA;
	}

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.IPPV",
			 "Sent canhSendOneShotEvent successfully %s:%d\n",
			 __FUNCTION__, __LINE__ );
	
	return RMF_SUCCESS;
	
}

rmf_osal_Bool isCANHReady(  )
{
	return canh_started;
}

void canhPODMonitorThread( void *param )
{
	rmf_Error err;
	rmf_osal_eventqueue_handle_t ippv_pod_queue;
	rmf_osal_event_handle_t pod_event_handle;
	uint32_t pod_event_type;
	rmf_osal_event_params_t event_params = { 0 };

	if ( RMF_SUCCESS !=
		 rmf_osal_eventqueue_create( ( const uint8_t * ) "ippv_client_podQ",
									 &ippv_pod_queue ) )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV",
				 "<IppvClient :: %s> - unable to create POD event queue,... terminating thread.\n",
				 __FUNCTION__ );
		return;
	}

	rmf_podmgrRegisterEvents( ippv_pod_queue );
	pod_mon_started = true;
	while ( 1 )
	{

		if ( RMF_SUCCESS !=
			 rmf_osal_eventqueue_get_next_event( ippv_pod_queue,
												 &pod_event_handle, NULL,
												 &pod_event_type,
												 &event_params ) )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV",
					 "<IppvClient :: %s> - rmf_podmgrIsReady != POD READY. event get failed...\n",
					 __FUNCTION__ );
		}
		
		rmf_osal_event_delete( pod_event_handle );
		if ( pod_event_type == RMF_PODMGR_EVENT_REMOVED )
		{
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.IPPV",
					 "<IppvClient :: %s> - sending COMMAND_CANH_CARD_REMOVED...\n",
					 __FUNCTION__ );				
			canhSendOneShotEvent( COMMAND_CANH_CARD_REMOVED );				
		}
		else if ( pod_event_type == RMF_PODMGR_EVENT_READY )
		{
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.IPPV",
					 "<IppvClient :: %s> - sending COMMAND_CANH_POD_READY...\n",
					 __FUNCTION__ );				
			canhSendOneShotEvent( COMMAND_CANH_POD_READY );
		}
		else if ( pod_event_type == RMF_PODMGR_EVENT_RESET_PENDING )
		{
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.IPPV",
					 "<IppvClient :: %s> - sending RMF_PODMGR_EVENT_RESET_PENDING...\n",
					 __FUNCTION__ );				
			canhSendOneShotEvent( COMMAND_CANH_CARD_RESET_PENDING );
		}

	}

	rmf_podmgrUnRegisterEvents( ippv_pod_queue );
	rmf_osal_eventqueue_delete( ippv_pod_queue );

}

rmf_osal_Bool monitor_vod_crash(rmf_osal_Bool bForceCrash)
{
	char pipeBuf[ 64 ] = {0};
	int32_t pipeFd = 0, size = 0;
	
	/* waiting for SI script to write "VOD_CRASHED" or 
	a termination messae "CLOSED" from canh module itself*/
	
        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV",
                "%s: Started \n",
                __FUNCTION__ );
	if(bForceCrash == 1)
	{
                char pipeBuf[ 64 ] = {0};
                int32_t pipeFd = 0, size = 0;

		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV",
			 "%s: Killing the vodClientApp forcefully \n",
			 __FUNCTION__ );
                pipeFd = open( FORCED_VOD_CRASH_MONPIPE, O_RDONLY );
                size = read( pipeFd, pipeBuf, sizeof(pipeBuf) );
		close( pipeFd );
                if( !strncmp( VOD_FORCED_CRASH, pipeBuf, sizeof( VOD_FORCED_CRASH ) - 1 ) )
                {
                  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV",
                          "%s: vodClient force crashed \n",
                          __FUNCTION__ );
                }
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV",
	 		"%s: Killed the vodClientApp forcefully \n",
	 		__FUNCTION__ );	
	}
        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV",
                "%s: Trying to read from VOD_CRASH_MONPIPE \n",
                __FUNCTION__ );	
	pipeFd = open( VOD_CRASH_MONPIPE, O_RDONLY );
	size = read( pipeFd, pipeBuf, sizeof(pipeBuf) );
	close( pipeFd );
	
	if ((size == 0) ||
		(size != (sizeof(VOD_CRASHED) -1) && size != (sizeof(CANH_CLOSED) - 1)))
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD",
				"%s: PIPE is accessed illegally size=%d pipe[0]=%c pipe[10]=%c\n",
				__FUNCTION__, size, pipeBuf[0], pipeBuf[10]);
		rmf_podmgrNotifySASTerm();
		onCANHRestart();
		return 0;
	}
	else if( !strncmp( VOD_CRASHED, pipeBuf, sizeof( VOD_CRASHED ) - 1 ) )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "%s: vodClient crashed \n",
				 __FUNCTION__ );
		rmf_podmgrNotifySASTerm();
		onCANHRestart();
		return 1;
	}
	else if( !strncmp( CANH_CLOSED, pipeBuf, sizeof( CANH_CLOSED ) -1) )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "%s: Received termination message \n",
				 __FUNCTION__ );					
		return 1;
	}
	return 0;

}


void canhStateMonitorThread( void *param )
{
	int count = 20;
	int32_t result_recv;
	uint32_t response;
	bool bSendStart = false;
	rmf_Error err;
	rmf_osal_eventqueue_handle_t ippv_pod_queue;
	rmf_osal_event_handle_t pod_event_handle;
	uint32_t pod_event_type;
	rmf_osal_event_params_t event_params = { 0 };

	uint32_t sl_time = 0;
	const char* envVar = rmf_osal_envGet("CANH_START_DELAY");
	if (envVar != NULL) 
	{ 
		sl_time = atoi(envVar); 
		if(0 == sl_time) 
		{
			sl_time = 1;
		}
	}
	else
	{ 
		sl_time = 90; 
	}

	if ( RMF_SUCCESS !=
		 rmf_osal_eventqueue_create( ( const uint8_t * ) "ippv_client",
									 &ippv_pod_queue ) )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV",
				 "<IppvClient :: %s> - unable to create POD event queue,... terminating thread.\n",
				 __FUNCTION__ );
		return;
	}

	rmf_podmgrRegisterEvents( ippv_pod_queue );

	while ( 1 )
	{
		err = rmf_podmgrIsReady(  );
		if ( RMF_SUCCESS != err )
		{
			if ( RMF_SUCCESS !=
				 rmf_osal_eventqueue_get_next_event( ippv_pod_queue,
													 &pod_event_handle, NULL,
													 &pod_event_type,
													 &event_params ) )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV",
						 "<IppvClient :: %s> - rmf_podmgrIsReady != POD READY. event get failed...\n",
						 __FUNCTION__ );
			}

			rmf_osal_event_delete( pod_event_handle );
			if ( pod_event_type != RMF_PODMGR_EVENT_READY )
			{
				continue;
			}
		}
		rmf_podmgrUnRegisterEvents( ippv_pod_queue );
		rmf_osal_eventqueue_delete( ippv_pod_queue );
		break;
	}

	while ( count > 0 )
	{
		if( true == canh_started )
		{
			count = 20;
			rmf_osal_Bool mon_result = monitor_vod_crash( 0 );
			if(mon_result)
			{
				return;
			}
			else
			{
				continue;
			}
		}
		
		if ( !bSendStart )
		{
			if(canh_restarted)
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV",
					"%s: sleeping %ds before sending CANH start command \n",
					__FUNCTION__, sl_time);
				rmf_osal_threadSleep(sl_time * 1000, 0);
			}
			IPCBuf ipcBufStart = IPCBuf( ( uint32_t ) COMMAND_START_CANH );

			int8_t res = ipcBufStart.sendCmd( canh_server_port_no );
			if ( 0 == res )
			{
				result_recv = ipcBufStart.getResponse( &response );
				bSendStart = true;

				if ( COMMAND_START_CANH != response )
				{
					RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV",
							 "%s: Java CANH module failed on boot-up [%d], setting RDK-03002 error\n",
							 __FUNCTION__, response );
					IARM_Bus_SYSMgr_EventData_t eventData;
					eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_CMAC;
					eventData.data.systemStates.state = 1;
					eventData.data.systemStates.error = 1;
					IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));					
					return;
				}
			}
			else if ( -3 == res )		/* connection failure */
			{
				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.IPPV",
						 "CANH Server not started %s:%d\n", __FUNCTION__,
						 __LINE__ );

				if ( 0 == count )
				{
					RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV",
							 "Exiting CANH communication %s:%d\n",
							 __FUNCTION__, __LINE__ );
				}
				sleep( 5 );
				continue;
			}
			else
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV",
						 "CANH Comm failure in %s:%d\n", __FUNCTION__,
						 __LINE__ );
				return;
			}
		}

		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.IPPV",
				 "Sent COMMAND_START_CANH successfully %s:%d\n", __FUNCTION__,
				 __LINE__ );

		sleep( 5 );
		IPCBuf ipcBuf = IPCBuf( ( uint32_t ) COMMAND_GET_CANH_STATUS );

		int8_t res = ipcBuf.sendCmd( canh_server_port_no );
		if ( 0 == res )
		{
			result_recv = ipcBuf.getResponse( &response );
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV", "%s:%d CANH status is [%x]\n",
					 __FUNCTION__, __LINE__, response );

			if ( CANH_STATUS_READY == response )
			{
				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.IPPV",
						 "CANH started successfully.In ready state\n" );
				canh_started = true;
#ifdef GLI				
				IARM_Bus_SYSMgr_EventData_t eventData;
				eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_PLANT_ID; 
				eventData.data.systemStates.state = 2; 
				eventData.data.systemStates.error = 0; 				
				err = canhGetPlantId( (uint8_t* )eventData.data.systemStates.payload );
				if( RMF_SUCCESS == err )
				{
					IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
					eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_CMAC; 
					eventData.data.systemStates.state = 2; 
					eventData.data.systemStates.error = 0; 
					IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
				}
#endif				
				//return;
			}
		}
		else
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV",
					 "CANH Comm failure in %s:%d\n", __FUNCTION__, __LINE__ );
			bSendStart = false;
			continue;
		}
		count--;
	}


	canh_started = true;
	canh_restarted = true;
  	RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV",
			 "Failed to get canh status from CANHServer.Request timed out, restarting the vodClientApp" );	
	monitor_vod_crash(1);

	RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV",
			 "Completed the vodClientApp restart" );

	return;
}

