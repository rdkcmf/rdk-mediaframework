/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2014 RDK Management
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

#include "rmf_osal_ipc.h"
#include "rmf_pod.h"
#include "rmf_inbsimgr.h"
#include "ippvServer.h"
#include "rmfcore.h"
#include "ippvclient.h"


//#define IPPV_SERVER_PORT_NO 50501
uint32_t ippv_server_port_no=0;
static rmf_osal_eventmanager_handle_t ippv_eventmanager_handle;

void recv_canh_cmd( IPCServerBuf * pServerBuf, void *pUserData );

IPCServer *pPPVIPCServer = NULL;


void freefn( void *data );

void freefn( void *data )
{
	rmf_osal_memFreeP( RMF_OSAL_MEM_IPPV, data );
}


rmf_Error ippvServerStart(	)
{
	rmf_Error osal_ret =
	rmf_osal_eventmanager_create( &ippv_eventmanager_handle );
	const char *ippv_server;
	ippv_server=rmf_osal_envGet( "IPPV_SERVER_PORT_NO" );
	if( NULL ==ippv_server)
	{
	       RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.IPPV", "%s: NULL POINTER  received for name string\n", __FUNCTION__ );
	       return RMF_OSAL_EINVAL;
	}
	ippv_server_port_no=atoi(server_port);

	pPPVIPCServer = new IPCServer( ( int8_t * ) "ippvServer",ippv_server_port_no );
	int8_t ret = pPPVIPCServer->start(  );

	if ( 0 != ret )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV", "%s : Server failed to start\n",
				 __FUNCTION__ );
		return RMF_RESULT_FAILURE;
	}

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.IPPV",
			 "%s : Server server1 started successfully\n", __FUNCTION__ );
	pPPVIPCServer->registerCallBk( &recv_canh_cmd, ( void * ) pPPVIPCServer );
	return osal_ret;

}


rmf_Error ippvServerStop(  )
{
	rmf_Error osal_ret =
		rmf_osal_eventmanager_delete( ippv_eventmanager_handle );

	int8_t ret = pPPVIPCServer->stop(	);

	if ( 0 != ret )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV", "%s : Server failed to stop \n",
				 __FUNCTION__ );
		return RMF_RESULT_FAILURE;
	}

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.IPPV",
			 "%s : Server server1 stoped successfully\n", __FUNCTION__ );
	return osal_ret;

}

rmf_Error ippvServerRegister( rmf_osal_eventqueue_handle_t queueId,
							  uint32_t eId )
{
	rmf_Error ret;
	IppvClient ppv;

	ret =
		rmf_osal_eventmanager_register_handler( ippv_eventmanager_handle,
												queueId,
												RMF_OSAL_EVENT_CATEGORY_IPPV );
	ret = ppv.startMonitoring( eId );

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.IPPV",
			 "%s: ret from startMonitoring is = %s\n", __FUNCTION__,
			 ppv.getStatusByString( ret ) );
	if ( RMF_IPPVSRC_SUCCESS != ret )
	{
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.IPPV", "%s: returning with Error \n",
				 __FUNCTION__, ret );
		return ret;
	}

}

rmf_Error ippvServerUnRegister( rmf_osal_eventqueue_handle_t queueId,
								uint32_t eId )
{
	rmf_Error osal_ret;
	IppvClient ppv;
	int32_t ret = 0;
	rmf_osal_event_handle_t event_handle;

	osal_ret =
		rmf_osal_eventmanager_unregister_handler( ippv_eventmanager_handle,
												  queueId );

	rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_IPPV,
						   IPPV_SERVER_TERM_EVENT, NULL, &event_handle );

	/* to prevent quiting all monitors interested on same eid */
	rmf_osal_eventqueue_add( queueId, event_handle );

	ret = ppv.stopMonitoring( eId );

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.IPPV",
			 "%s: ret from stopMonitoring is = %s\n", __FUNCTION__,
			 ppv.getStatusByString( ret ) );

	if ( RMF_IPPVSRC_SUCCESS != ret )
	{
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.IPPV", "%s: returning with Error \n",
				 __FUNCTION__, ret );
		return RMF_RESULT_FAILURE;
	}

	return osal_ret;

}

void recv_canh_cmd( IPCServerBuf * pServerBuf, void *pUserData )
{
	rmf_Error ret = RMF_OSAL_ENODATA;
	uint32_t sessionId = 0, apduTag = 0;
	int32_t length = 0;
	uint16_t version;
	uint32_t identifyer = 0;
	uint8_t *appId = 0;
	uint8_t *apdu = NULL;
	int32_t result_recv = 0;

	identifyer = pServerBuf->getCmd(  );
	IPCServer *pIPCServer = ( IPCServer * ) pUserData;

	switch ( identifyer )
	{

	  case IPPV_SERVER_AUTHORIZATION_EVENT:
		  {
			  rmf_osal_event_handle_t event_handle;
			  rmf_osal_event_params_t eventData;
			  int64_t eid;

			  iPPVEvent *pEvent = NULL;
			  int32_t result = 0;

			  rmf_osal_memAllocP( RMF_OSAL_MEM_IPPV, sizeof( iPPVEvent ),
								  ( void ** ) &pEvent );
			  pServerBuf->collectData( ( void * ) &eid, sizeof( eid ) );

			  RDK_LOG( RDK_LOG_INFO, "LOG.RDK.IPPV",
					   "%s: Eid recvd from CANH is %lld", __FUNCTION__, eid );
			  pEvent->eId = eid & 0x00000000FFFFFFFF;
			  result =
				  pServerBuf->collectData( ( void * ) &pEvent->isAuthorized,
										   sizeof( pEvent->isAuthorized ) );
			  if ( result != 0 )
			  {
				  pServerBuf->collectData( ( void * ) &pEvent->purchaseStatus,
										   sizeof( pEvent->purchaseStatus ) );
			  }
			  pServerBuf->addResponse( identifyer );
			  pServerBuf->sendResponse(  );

			  if ( 0 != ippv_eventmanager_handle )
			  {
				  eventData.data = pEvent;
				  eventData.data_extension = 0;
				  eventData.free_payload_fn = freefn;
				  eventData.priority = 0;
				  rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_POD,
										 IPPV_SERVER_AUTHORIZATION_EVENT,
										 &eventData, &event_handle );
				  rmf_osal_eventmanager_dispatch_event
					  ( ippv_eventmanager_handle, event_handle );
			  }
		  }
		  break;
	  default:
		  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV",
				   "\n\n\n\n\n\n unable to identify the identifyer from packet = 0x%x  \n\n\n\n\n\n",
				   identifyer );
		  pIPCServer->disposeBuf( pServerBuf );
		  break;
	}

}

