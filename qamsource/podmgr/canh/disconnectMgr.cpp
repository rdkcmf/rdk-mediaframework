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
#include "disconnectMgr.h"
#include "rmf_osal_thread.h"
#include "rdk_debug.h"
#include "canhClient.h"

#define DISCONNECT_MGR_CHANGED_EID 1
#define DISCONNECT_MGR_TERM 2
static rmf_osal_Bool bInited;
static rmf_osal_eventmanager_handle_t disconnect_event_manager;
static rmf_osal_Bool bAuthStatus = FALSE;
static rmf_osal_eventqueue_handle_t gMonQ ;

static rmf_osal_ThreadId disconnectEventMonThreadId = 0;

static void disconnectEventMonThread( void *data );

static void disconnectEventMonThread( void *data )
{
	rmf_osal_eventqueue_handle_t monQ = ( rmf_osal_eventqueue_handle_t ) data ;
	rmf_osal_event_handle_t event_handle;
	rmf_osal_event_category_t event_category ;
	uint32_t event_type;	
	rmf_osal_event_params_t event_params;
	rmf_osal_Bool eIDRecvd = FALSE;
	uint32_t eID = 0;
	rmf_Error ret = 0;
	rmf_osal_Bool bLoop = TRUE;
		
	while( bLoop )
	{
		ret = rmf_osal_eventqueue_get_next_event(monQ, 
											&event_handle, 
											&event_category, 
											&event_type, 
											&event_params);
		if( RMF_SUCCESS != ret )
		{
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.IPPV",
							  "rmf_osal_eventqueue_get_next_event() failed\n");
			continue;

		}
		switch (event_category)
		{
			case RMF_OSAL_EVENT_CATEGORY_IPPV:
				{
					switch ( event_type )
					{
						case CANH_RESOURCE_AUTHORIZATION_EVENT:
							{
								if( eIDRecvd == FALSE ) break;
								if( (eID == ((uint32_t) event_params.data)) && 
									( bAuthStatus != event_params.data_extension ) )
								{									
									rmf_osal_event_handle_t new_event_handle;									
									bAuthStatus = event_params.data_extension;
									rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_DISCONNECTMGR,
															 (bAuthStatus == 0) ? RMF_PODMGR_EVENT_EID_DISCONNECTED: RMF_PODMGR_EVENT_EID_CONNECTED,
															 NULL, &new_event_handle );	
					  				rmf_osal_eventmanager_dispatch_event
										  ( disconnect_event_manager, new_event_handle );
								}
							}
							break;
						default:
							{

							}
							break;
					}//inner switch 1					
				}
				break;//RMF_OSAL_EVENT_CATEGORY_IPPV
			case RMF_OSAL_EVENT_CATEGORY_DISCONNECTMGR:
				{
					switch ( event_type )
					{
						case DISCONNECT_MGR_CHANGED_EID:
						{
							uint8_t isAuthorized = 0;
							/* To start monitoring, eIDRecvd significance is at first time only */
							if( eIDRecvd == TRUE )
							{
								canhStopEIDMonitoring( eID, CANH_TARGET_RESOURCE );
							}
							eIDRecvd = TRUE;							
							eID = (uint32_t) event_params.data;
							canhStartEIDMonitoring( eID, CANH_TARGET_RESOURCE );								
							IsCanhResourceAuthorized( eID, &isAuthorized );
							if( bAuthStatus != isAuthorized )
							{									
								rmf_osal_event_handle_t new_event_handle;									
								bAuthStatus = isAuthorized;
								rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_DISCONNECTMGR,
														 (bAuthStatus == 0) ? RMF_PODMGR_EVENT_EID_DISCONNECTED : RMF_PODMGR_EVENT_EID_CONNECTED,
														 NULL, &new_event_handle );	
				  				rmf_osal_eventmanager_dispatch_event
									  ( disconnect_event_manager, new_event_handle );
							}
						}	
						break;
						case DISCONNECT_MGR_TERM:
							if( eIDRecvd == TRUE )
							{
								canhStopEIDMonitoring( eID, CANH_TARGET_RESOURCE );
							}							
							bLoop = FALSE;
						break;
					}//inner switch 2
				}
				break;
		}//outer switch
		rmf_osal_event_delete( event_handle );
	}

	canhClientUnRegisterEvents( monQ );
	rmf_osal_eventqueue_delete( monQ );
	
}

rmf_Error disconnectMgrInit( )
{
	rmf_Error ret;
	
	ret = rmf_osal_eventmanager_create( &disconnect_event_manager );
	if (RMF_SUCCESS != ret)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.IPPV",
						  "rmf_osal_eventmanager_create() failed\n");
		return ret;
	}	

	ret = rmf_osal_eventqueue_create( (const uint8_t *) "DisconnectMgr", &gMonQ );
	if (RMF_SUCCESS != ret)
	{
		RDK_LOG(RDK_LOG_ERROR,  "LOG.RDK.IPPV",
						  "rmf_osal_eventqueue_create() failed\n");
		return RMF_OSAL_ENODATA;
	}	

	canhClientRegisterEvents( gMonQ );
	
	if ( ( ret =
		   rmf_osal_threadCreate( disconnectEventMonThread, (void *) gMonQ,
								  RMF_OSAL_THREAD_PRIOR_DFLT,
								  RMF_OSAL_THREAD_STACK_SIZE,
								  &disconnectEventMonThreadId,
								  "disconnectEventMonThread" ) ) !=
		 RMF_SUCCESS )
	{
		RDK_LOG( RDK_LOG_ERROR,  "LOG.RDK.IPPV",
				 "%s: failed to create disconnectEventMonThread (err=%d).\n",
				 __FUNCTION__, ret );
	}
	bInited = TRUE;	
	return ret;
}

rmf_Error disconnectMgrUnInit( )
{

	rmf_osal_event_handle_t event_handle;
	rmf_osal_event_category_t event_category ;
	uint32_t event_type;	
	rmf_osal_event_params_t event_params;
	rmf_Error ret = RMF_SUCCESS;

	if( !bInited )
	{
		RDK_LOG(RDK_LOG_ERROR,  "LOG.RDK.IPPV",
						  "%s: not Initred \n",__FUNCTION__);	
		return RMF_OSAL_ENODATA;
	}

	rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_DISCONNECTMGR,
							DISCONNECT_MGR_TERM,
							NULL, &event_handle );	
	rmf_osal_eventqueue_add( gMonQ, event_handle );

	ret = rmf_osal_eventmanager_delete( disconnect_event_manager );
	bInited = TRUE;
	return ret;

}
rmf_osal_Bool disconnectMgrGetAuthStatus( )
{
	return bAuthStatus;
}


rmf_Error disconnectMgrSubscribeEvents( rmf_osal_eventqueue_handle_t queueId )
{
	if( !bInited )
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.IPPV",
						  "%s: not Initred \n",__FUNCTION__);	
		return RMF_OSAL_ENODATA;
	}
	
	return  rmf_osal_eventmanager_register_handler(
		disconnect_event_manager,
		queueId,
		RMF_OSAL_EVENT_CATEGORY_DISCONNECTMGR);

}
rmf_Error disconnectMgrUnSubscribeEvents( rmf_osal_eventqueue_handle_t queueId )
{
	return rmf_osal_eventmanager_unregister_handler(
		disconnect_event_manager,
		queueId	);
}


