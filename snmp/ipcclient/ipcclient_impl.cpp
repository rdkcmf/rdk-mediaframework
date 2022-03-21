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


#include <string.h>
#include "ipcclient_impl.h"
#include <podimpl.h>
#include <rmf_pod.h>
#include <stdlib.h>
#ifdef USE_POD_IPC
#include <podServer.h>
#include <rmf_osal_ipc.h>
#include "QAMSrcDiag.h"
#endif

#define VL_MAX_SNMP_STR_SIZE 256

//#if 1
#ifdef USE_POD_IPC

#define POD_IS_UP 1
#define POD_IS_DOWN 0
static volatile uint32_t pod_server_cmd_port_no;
static volatile uint32_t qam_src_server_cmd_port_no = 0;

static uint32_t podClientHandle;
static rmf_osal_eventmanager_handle_t pod_client_event_manager;
static rmf_osal_ThreadId podClientMonitorThreadId = 0;
static rmf_osal_ThreadId podEventPollThreadId = 0;

//static rmf_osal_Mutex podDecryptQueueListMutex;
static rmf_osal_Mutex podHandleMutex;

static void podHeartbeatMonThread( void *data );
uint32_t getPodClientHandle(uint32_t *pHandle);
static void updatePodClientHandle(uint32_t handle);
static void podEventPollThread( void *data );

static rmf_Error registerPodClient( )
{
	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_REGISTER_CLIENT);
	
	int32_t result_recv = 0;
	uint32_t handle = 0;
	
	int8_t res = ipcBuf.sendCmd( pod_server_cmd_port_no );

	if ( 0 != res )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP", 
			"%s: Lost connection with POD. res = %d\n",
				 __FUNCTION__, res );
		return RMF_OSAL_ENODATA;
	}

	result_recv = ipcBuf.getResponse( &handle );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP", 
			"%s: No response from POD. result_recv = %d\n",
				 __FUNCTION__, result_recv );	
		return RMF_OSAL_ENODATA;
	}

	if( 0 == handle  )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP", 
			"%s: POD Handle is zero\n",
				 __FUNCTION__);			
		return RMF_OSAL_ENODATA;
	}
	
	updatePodClientHandle( handle );
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.SNMP", 
		"%s: Updated POD handle successfully \n",
			 __FUNCTION__);
	return RMF_SUCCESS;	
	
}


void updatePodClientHandle(uint32_t handle)
{
	rmf_osal_mutexAcquire( podHandleMutex );
	podClientHandle = handle;
	rmf_osal_mutexRelease( podHandleMutex );	
}

uint32_t getPodClientHandle(uint32_t *pHandle)
{
	rmf_osal_mutexAcquire( podHandleMutex );
	*pHandle = podClientHandle;
	rmf_osal_mutexRelease( podHandleMutex );		
	return *pHandle;
}

static void podHeartbeatMonThread( void *data )
{	
	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_HB);
	int32_t result_recv = 0;
	rmf_Error ret = RMF_SUCCESS;
	int8_t res = 0;
	uint8_t currentState = POD_IS_DOWN;

	while( 1 )
	{
		if( POD_IS_DOWN == currentState )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP", 
			"%s: POD_IS_DOWN\n",
				 __FUNCTION__);	

			updatePodClientHandle( 0 );
		}
		
		res = ipcBuf.sendCmd( pod_server_cmd_port_no );
		if ( 0 != res )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP", 
				"%s: Lost connection with POD. res = %d retrying...\n",
					 __FUNCTION__, res );
			currentState = POD_IS_DOWN;
			sleep(1);
			continue;
		}

		result_recv = ipcBuf.getResponse( &ret );
		if( (result_recv < 0)  ||( RMF_SUCCESS != ret ) )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP", 
				"%s: No response from POD. result_recv = %d, ret= %d retrying...\n",
					 __FUNCTION__, result_recv, ret );	
			currentState = POD_IS_DOWN;
			sleep(5);
			continue;
		}		

		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.SNMP", 
			"%s: POD process is live \n",
				 __FUNCTION__);

		if( POD_IS_DOWN == currentState )
		{
			/* Need to get handle everytime when POD is restarted */
			if ( RMF_SUCCESS !=  registerPodClient( ) )
			{
				currentState = POD_IS_DOWN;
				continue;
			}
		}
		
		currentState = POD_IS_UP;
		sleep( 10 );
	}
	return;
}

void podClientfreefn( void *data )
{
	rmf_osal_memFreeP( RMF_OSAL_MEM_POD, data );
}

static void podEventPollThread( void *data )
{	
	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_POLL_EVENT);
	uint32_t ret = RMF_SUCCESS;
	int8_t res = 0;
	uint32_t handle;

	while( 1 )
	{
		int32_t result_recv = 0;
		getPodClientHandle(&handle);
		if( handle == 0 )
		{
			/* lost heart beat. So waiting */
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
				"%s : lost heart beat. So waiting for 5 seconds \n", __FUNCTION__ );
			sleep(5);
			continue;
		}

		ipcBuf.clearBuf();
		ipcBuf.addData( (const void *) &handle, sizeof( handle));
		
		res = ipcBuf.sendCmd( pod_server_cmd_port_no );
		if ( 0 != res )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP", 
				"%s: Lost connection with POD. res = %d retrying...\n",
					 __FUNCTION__, res );
			continue;
		}	

		result_recv = ipcBuf.getResponse( &ret );
		if( result_recv < 0)
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP", 
				"%s: No response from POD. result_recv = %d retrying...\n",
					 __FUNCTION__, result_recv );	
			continue;
		}
		
		switch (ret )
		{
			case POD_RESP_NO_EVENT:
				{
					RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.SNMP", 
						"%s: POD_RESP_NO_EVENT \n",
						 __FUNCTION__);	
				}
				break;
			case POD_RESP_GEN_EVENT:
				{
					int32_t result_recv = 0;
					uint32_t event_type;
					rmf_osal_event_handle_t event_handle;
					rmf_osal_event_params_t eventData = { 0, NULL, 0, NULL };

					RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.SNMP", 
						"%s: POD_RESP_GEN_EVENT \n",
						 __FUNCTION__);	
					
					result_recv = ipcBuf.collectData( (void *)&event_type, sizeof(event_type) );
					
					RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.SNMP", 
						"%s: POD_RESP_GEN_EVENT type is %x\n",
						 __FUNCTION__, event_type);	
					
					if( result_recv < 0)
					{
						RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP", 
							"%s : POD_RESP_GEN_EVENT failed in %d\n",
								 __FUNCTION__, __LINE__ );
						break;
					}

					switch( event_type )
					{
						case RMF_PODMGR_EVENT_DSGIP_ACQUIRED:
						{
							int32_t result_recv = 0;

							RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.SNMP", 
								"%s: POD_RESP_GEN_EVENT type is RMF_PODMGR_EVENT_DSGIP_ACQUIRED \n",
								 __FUNCTION__ );

							rmf_osal_memAllocP( RMF_OSAL_MEM_POD, RMF_PODMGR_IP_ADDR_LEN, &( eventData.data ) );
							if( NULL == eventData.data )
							{
								RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP", 
									"rmf_osal_memAllocP failed in %s:%d\n",
										 __FUNCTION__, __LINE__ );
								break;/* inner switch */
							}
							
							result_recv = ipcBuf.collectData( (void *)eventData.data, RMF_PODMGR_IP_ADDR_LEN );	
							if( result_recv < 0 )
							{
								RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP", 
									"collectData failed in %s:%d\n",
										 __FUNCTION__, __LINE__ );
								break;/* inner switch */								
							}		
							RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.SNMP", 
								"%s: RMF_PODMGR_EVENT_DSGIP_ACQUIRED %s \n",
								 __FUNCTION__, eventData.data );							
							eventData.free_payload_fn = podClientfreefn;
						}
						break;/* inner switch */					
						default:
						{
							uint32_t data = 0;
							int32_t result_recv = 0;

							result_recv |= ipcBuf.collectData( (void *)&data, sizeof( data) );	
							eventData.data = (void *)data;
							result_recv |= ipcBuf.collectData( (void *)&eventData.data_extension, 
														sizeof( eventData.data_extension) );	
							if( result_recv < 0 )
							{
								RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP", 
									"collectData failed in %s:%d\n",
										 __FUNCTION__, __LINE__ );
								break;/* inner switch */								
							}
						}
						break;							
					}
					
					ret = rmf_osal_event_create( 
							RMF_OSAL_EVENT_CATEGORY_PODMGR, 
							event_type, 
							&eventData, &event_handle);
					if( RMF_SUCCESS != ret )
					{
						RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP", 
							"rmf_osal_event_create failed in %s:%d\n",
								 __FUNCTION__, __LINE__ );						
					}
					
					ret = rmf_osal_eventmanager_dispatch_event(
										pod_client_event_manager,
										event_handle);
					if( RMF_SUCCESS != ret )
					{
						RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP", 
							"rmf_osal_eventmanager_dispatch_event failed in %s:%d\n",
								 __FUNCTION__, __LINE__ );						
					}	
				}
				break;
				/* propogating, but need to handle it  */
			case POD_RESP_CORE_EVENT:
				{
					int32_t result_recv = 0;
					uint32_t event_type;
					rmf_osal_event_handle_t event_handle;
					rmf_osal_event_params_t eventData = { 0, NULL, 0, NULL };					
					uint8_t data_present = 0;
					uint32_t retVal = 0;
					
					RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.SNMP", 
						"%s: POD_RESP_CORE_EVENT \n",
						 __FUNCTION__);
					
					result_recv = ipcBuf.collectData( (void *)&event_type, sizeof(event_type) );
					
					if( result_recv < 0)
					{
						RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP", 
							"%s : POD_RESP_CORE_EVENT failed in %d\n",
								 __FUNCTION__, __LINE__ );
						break;
					}

					RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.SNMP", 
						"%s: POD_RESP_CORE_EVENT type is %x\n",
						 __FUNCTION__, event_type);	

					result_recv |= ipcBuf.collectData( (void *)&eventData.data_extension, 
												sizeof( eventData.data_extension) );
					result_recv |= ipcBuf.collectData( (void *)&data_present, 
												sizeof( data_present) );
					if( data_present )
					{
						retVal = rmf_osal_memAllocP( RMF_OSAL_MEM_POD, eventData.data_extension,
								( void ** ) &eventData.data );

						if ( RMF_SUCCESS != retVal )
						{
							RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP", 
								"%s: NO Memory \n",
								 __FUNCTION__);
						}
						result_recv |= ipcBuf.collectData( (void *)eventData.data, eventData.data_extension );
						//rmf_osal_memFreeP( RMF_OSAL_MEM_POD, eventData.data );
						eventData.free_payload_fn = podClientfreefn;
					}
					if( result_recv < 0 )
					{
						RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP", 
							"collectData failed in %s:%d\n",
								 __FUNCTION__, __LINE__ );
						break;/* inner switch */								
					}		
					ret = rmf_osal_event_create( 
							RMF_OSAL_EVENT_CATEGORY_PODMGR, 
							event_type, 
							&eventData, &event_handle);
					if( RMF_SUCCESS != ret )
					{
						RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP", 
							"rmf_osal_event_create failed in %s:%d\n",
								 __FUNCTION__, __LINE__ );						
					}
					
					ret = rmf_osal_eventmanager_dispatch_event(
										pod_client_event_manager,
										event_handle);
					if( RMF_SUCCESS != ret )
					{
						RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP", 
							"rmf_osal_eventmanager_dispatch_event failed in %s:%d\n",
								 __FUNCTION__, __LINE__ );						
					}	
				}
				break;
				
			default:
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP", 
					"%s: Invalid event received \n",
						 __FUNCTION__);						
				break;
				
		}

		RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.SNMP", 
			"%s: POD Client Event monitoring is live \n",
				 __FUNCTION__);
	}
	return;
}

void rmf_snmpmgrIPCUnInit(void)
{
	rmf_Error ret = RMF_SUCCESS;

	ret = rmf_osal_eventmanager_delete(pod_client_event_manager);
	if (RMF_SUCCESS != ret)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SNMP",
						  "rmf_osal_eventmanager_delete() failed\n");
		return;
	}	

	rmf_osal_mutexDelete( podHandleMutex );
	
}
#endif // end: USE_POD_IPC

rmf_Error rmf_snmpmgrIPCInit(void)
{
	rmf_Error ret = RMF_SUCCESS;

#ifdef USE_POD_IPC
	//printf("File - %s : Function - %s : Line - %d \n", __FILE__, __FUNCTION__, __LINE__);

	int8_t result = 0;
	const char *server_port ;
	server_port = rmf_osal_envGet( "POD_SERVER_CMD_PORT_NO" );
	if( NULL == server_port )
	{
	       RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP", "%s: NULL POINTER  received for name string\n",__FUNCTION__ );
	       pod_server_cmd_port_no = POD_SERVER_CMD_DFLT_PORT_NO;
	}
	else
	{
		pod_server_cmd_port_no = atoi( server_port );
	}
	server_port = rmf_osal_envGet( "QAM_SRC_SERVER_CMD_PORT_NO" );
	if( NULL == server_port )
	{
	       RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP", "%s: NULL POINTER  received for name string\n",__FUNCTION__ );
	       qam_src_server_cmd_port_no = QAM_SRC_SERVER_CMD_DFLT_PORT_NO;
	}
	else
	{
		qam_src_server_cmd_port_no = atoi( server_port );
	}
	rmf_osal_mutexNew( &podHandleMutex );
	
	ret = rmf_osal_eventmanager_create(&pod_client_event_manager);
	if (RMF_SUCCESS != ret)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SNMP",
						  "rmf_osal_eventmanager_create() failed\n");
		return RMF_OSAL_ENODATA;
	}
	
	if ( ( ret =
		   rmf_osal_threadCreate( podHeartbeatMonThread, NULL,
								  RMF_OSAL_THREAD_PRIOR_DFLT,
								  RMF_OSAL_THREAD_STACK_SIZE,
								  &podClientMonitorThreadId,
								  "podHeartbeatMonThread" ) ) !=
		 RMF_SUCCESS )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
				 "%s: failed to create pod worker thread (err=%d).\n",
				 __FUNCTION__, ret );
	}
	
	if ( ( ret =
		   rmf_osal_threadCreate( podEventPollThread, NULL,
								  RMF_OSAL_THREAD_PRIOR_DFLT,
								  RMF_OSAL_THREAD_STACK_SIZE,
								  &podEventPollThreadId,
								  "podEventPollThread" ) ) !=
		 RMF_SUCCESS )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
				 "%s: failed to create pod worker thread (err=%d).\n",
				 __FUNCTION__, ret );
	}

	
	//rmf_osal_RegDsgDump(rmf_podmgrDumpDsgStats);

#endif // USE_POD_IPC

	return RMF_SUCCESS;
	
}

rmf_Error rmf_snmpmgrRegisterEvents(rmf_osal_eventqueue_handle_t queueId)
{
#ifdef USE_POD_IPC
	//printf("File - %s : Function - %s : Line - %d \n", __FILE__, __FUNCTION__, __LINE__);
    return  rmf_osal_eventmanager_register_handler(
        pod_client_event_manager,
        queueId,
        RMF_OSAL_EVENT_CATEGORY_PODMGR);
#endif
}

rmf_Error rmf_snmpmgrUnRegisterEvents(rmf_osal_eventqueue_handle_t queueId)
{
#ifdef USE_POD_IPC
	//printf("File - %s : Function - %s : Line - %d \n", __FILE__, __FUNCTION__, __LINE__);
    return rmf_osal_eventmanager_unregister_handler(
        pod_client_event_manager,
        queueId );
#endif
}

//#endif // #if 0

void  IPC_CLIENT_HAL_SYS_Reboot(const char * pszRequestor, const char * pszReason)
{
	
	rmf_Error ret = RMF_SUCCESS;

#ifdef USE_POD_IPC
	int32_t result_recv = 0;
	int8_t res = 0;
	int32_t lenOfRequestor = 1; // '\0' char
	int32_t lenOfReason = 1; // '\0' char

	lenOfRequestor = strlen(pszRequestor);
	lenOfReason = strlen(pszReason);

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.SNMP", "%s:Received Reboot Request pszRequestor='%s', pszReason='%s' \n", __FUNCTION__, pszRequestor, pszReason);
	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.SNMP", "%s:length pszRequestor=%d, length pszReason=%d \n", __FUNCTION__, lenOfRequestor, lenOfReason);
	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_STB_REBOOT);

	result_recv |= ipcBuf.addData( ( const void * ) &lenOfRequestor, sizeof(int32_t));
	result_recv |= ipcBuf.addData( ( const void * ) pszRequestor, lenOfRequestor);
	result_recv |= ipcBuf.addData( ( const void * ) &lenOfReason, sizeof(int32_t));
	result_recv |= ipcBuf.addData( ( const void * ) pszReason, lenOfReason);
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"\nPOD Comm add data failed in %s:%d: %d\n",
				  __FUNCTION__, __LINE__ , result_recv);
		return ;
	}
	res = ipcBuf.sendCmd( pod_server_cmd_port_no  );
	if ( 0 != res )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm failure in %s:%d: ret = %d\n",
				  __FUNCTION__, __LINE__, res );
		return ;
	}

	result_recv = ipcBuf.getResponse( &ret );

	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm get response failed in %s:%d ret = %d\n",
				  __FUNCTION__, __LINE__, result_recv );
		return ;
	}
#endif // USE_POD_IPC
}

int IPC_CLIENT_GetCCIInfoElement(CardManagerCCIData_t* pCCIData,unsigned long LTSID,unsigned long programNumber)
{
	rmf_Error ret = RMF_SUCCESS;

#ifdef USE_POD_IPC
	int32_t result_recv = 0;
	int8_t res = 0;

	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_CCI_INFO_ELEMENT);

	result_recv |= ipcBuf.addData( ( const void * ) &LTSID, sizeof( unsigned long ) );
	result_recv |= ipcBuf.addData( ( const void * ) &programNumber, sizeof( unsigned long ) );

	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"\nPOD Comm add data failed in %s:%d: %d\n",
				  __FUNCTION__, __LINE__ , result_recv);
		return RMF_OSAL_ENODATA;
	}

	res = ipcBuf.sendCmd( pod_server_cmd_port_no  );
	if ( 0 != res )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm failure in %s:%d: ret = %d\n",
				  __FUNCTION__, __LINE__, res );
		return RMF_OSAL_ENODATA;
	}

	result_recv = ipcBuf.getResponse( &ret );

	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm get response failed in %s:%d ret = %d\n",
				  __FUNCTION__, __LINE__, result_recv );
		return RMF_OSAL_ENODATA;
	}

	result_recv = ipcBuf.collectData(pCCIData, sizeof(CardManagerCCIData_t));

	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm get response failed in %s:%d ret = %d\n",
				  __FUNCTION__, __LINE__, result_recv );
		return RMF_OSAL_ENODATA;
	}
#endif // USE_POD_IPC

	return ret;  
}

int IPC_CLIENT_SNMPGetCpIdList(unsigned long *pCP_system_id_bitmask)
{
	rmf_Error ret = RMF_SUCCESS;

#ifdef USE_POD_IPC
	int32_t result_recv = 0;
	int8_t res = 0;

	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_CP_ID_LIST);
    
	res = ipcBuf.sendCmd( pod_server_cmd_port_no  );
	
	if ( 0 != res )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm failure in %s:%d: ret = %d\n",
				  __FUNCTION__, __LINE__, res );
		return RMF_OSAL_ENODATA;
	}

	result_recv = ipcBuf.getResponse( &ret );
	
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm get response failed in %s:%d ret = %d\n",
				  __FUNCTION__, __LINE__, result_recv );
		return RMF_OSAL_ENODATA;
	}
	
	result_recv = ipcBuf.collectData( pCP_system_id_bitmask, sizeof(unsigned long ));
	
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm get response failed in %s:%d ret = %d\n",
				  __FUNCTION__, __LINE__, result_recv );
		return RMF_OSAL_ENODATA;
	}
#endif // USE_POD_IPC

	return ret;
}

int IPC_CLIENT_SNMPSetTime( unsigned short nYear, 
				unsigned short nMonth, 
				unsigned short nDay, 
				unsigned short nHour, 
				unsigned short nMin, 
				unsigned short nSec  )
{
	rmf_Error ret = RMF_SUCCESS;
	int32_t result_recv = 0;
	int8_t res = 0;

#ifdef USE_POD_IPC
	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_UPDATE_TIME_EXT);

	result_recv |= ipcBuf.addData( ( const void * ) &nYear, sizeof( nYear ) );
	result_recv |= ipcBuf.addData( ( const void * ) &nMonth, sizeof( nMonth ) );
	result_recv |= ipcBuf.addData( ( const void * ) &nDay, sizeof( nDay ) );
	result_recv |= ipcBuf.addData( ( const void * ) &nHour, sizeof( nHour ) );
	result_recv |= ipcBuf.addData( ( const void * ) &nMin, sizeof( nMin ) );
	result_recv |= ipcBuf.addData( ( const void * ) &nSec, sizeof( nSec ) );

	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"\nPOD Comm add data failed in %s:%d: %d\n",
				  __FUNCTION__, __LINE__ , result_recv);
		return RMF_OSAL_ENODATA;
	}

	res = ipcBuf.sendCmd( pod_server_cmd_port_no  );
	if ( 0 != res )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm failure in %s:%d: ret = %d\n",
				  __FUNCTION__, __LINE__, res );
		return RMF_OSAL_ENODATA;
	}

	result_recv = ipcBuf.getResponse( &ret );

	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm get response failed in %s:%d ret = %d\n",
				  __FUNCTION__, __LINE__, result_recv );
		return RMF_OSAL_ENODATA;
	}
#endif // USE_POD_IPC

	return ret;
}

int IPC_CLIENT_SNMPGetCpKeyGenerationReqCount(unsigned long *pCount)
{
	rmf_Error ret = RMF_SUCCESS;
	int32_t result_recv = 0;
	int8_t res = 0;

#ifdef USE_POD_IPC
	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_CP_KEY_GEN_REQUEST);
    
	res = ipcBuf.sendCmd( pod_server_cmd_port_no  );
	if ( 0 != res )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm failure in %s:%d: ret = %d\n",
				  __FUNCTION__, __LINE__, res );
		return RMF_OSAL_ENODATA;
	}

	result_recv = ipcBuf.getResponse( &ret );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm get response failed in %s:%d ret = %d\n",
				  __FUNCTION__, __LINE__, result_recv );
		return RMF_OSAL_ENODATA;
	}
	
	result_recv = ipcBuf.collectData( pCount, sizeof(unsigned long ));
	
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm get response failed in %s:%d ret = %d\n",
				  __FUNCTION__, __LINE__, result_recv );
		return RMF_OSAL_ENODATA;
	}
#endif // USE_POD_IPC

	return ret;
}

int IPC_CLIENT_SNMPGetCpCciChallengeCount(unsigned long *pCount)
{
	rmf_Error ret = RMF_SUCCESS;

#ifdef USE_POD_IPC
	int32_t result_recv = 0;
	int8_t res = 0;

	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_CP_CCI_CHALLENGE_COUNT);
    
	res = ipcBuf.sendCmd( pod_server_cmd_port_no  );
	if ( 0 != res )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm failure in %s:%d: ret = %d\n",
				  __FUNCTION__, __LINE__, res );
		return RMF_OSAL_ENODATA;
	}

	result_recv = ipcBuf.getResponse( &ret );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm get response failed in %s:%d ret = %d\n",
				  __FUNCTION__, __LINE__, result_recv );
		return RMF_OSAL_ENODATA;
	}
	
	result_recv = ipcBuf.collectData( pCount, sizeof(unsigned long ));
	
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm get response failed in %s:%d ret = %d\n",
				  __FUNCTION__, __LINE__, result_recv );
		return RMF_OSAL_ENODATA;
	} 
#endif // USE_POD_IPC

	return ret;
}

int IPC_CLIENT_SNMPGetCpCertCheck(int *pCerttatus)
{
	rmf_Error ret = RMF_SUCCESS;

#ifdef USE_POD_IPC
	int32_t result_recv = 0;
	int8_t res = 0;

	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_CP_CERT_STATUS);
    
	res = ipcBuf.sendCmd( pod_server_cmd_port_no  );
	if ( 0 != res )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm failure in %s:%d: ret = %d\n",
				  __FUNCTION__, __LINE__, res );
		return RMF_OSAL_ENODATA;
	}

	result_recv = ipcBuf.getResponse( &ret );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm get response failed in %s:%d ret = %d\n",
				  __FUNCTION__, __LINE__, result_recv );
		return RMF_OSAL_ENODATA;
	}
	result_recv = ipcBuf.collectData( pCerttatus, sizeof(int ));
	
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm get response failed in %s:%d ret = %d\n",
				  __FUNCTION__, __LINE__, result_recv );
		return RMF_OSAL_ENODATA;
	}
#endif // USE_POD_IPC

	return ret; 
}

int IPC_CLIENT_SNMPGetCpAuthKeyStatus(int *pKeyStatus)
{
	rmf_Error ret = RMF_SUCCESS;

#ifdef USE_POD_IPC
	int32_t result_recv = 0;
	int8_t res = 0;

	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_CP_AUTH_KEY_STATUS);
    
	res = ipcBuf.sendCmd( pod_server_cmd_port_no  );
	if ( 0 != res )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm failure in %s:%d: ret = %d\n",
				  __FUNCTION__, __LINE__, res );
		return RMF_OSAL_ENODATA;
	}

	result_recv = ipcBuf.getResponse( &ret );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm get response failed in %s:%d ret = %d\n",
				  __FUNCTION__, __LINE__, result_recv );
		return RMF_OSAL_ENODATA;
	}
	result_recv = ipcBuf.collectData( pKeyStatus, sizeof(int ));
	
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm get response failed in %s:%d ret = %d\n",
				  __FUNCTION__, __LINE__, result_recv );
		return RMF_OSAL_ENODATA;
	}
#endif // USE_POD_IPC

	return ret;
}

int IPC_CLIENT_SNMPGetGenFetVctId(unsigned short *pVctId)
{
	rmf_Error ret = RMF_SUCCESS;

#ifdef USE_POD_IPC
	int32_t result_recv = 0;
	int8_t res = 0;
	
	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_VCT_ID);
    
	res = ipcBuf.sendCmd( pod_server_cmd_port_no  );
	if ( 0 != res )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm failure in %s:%d: ret = %d\n",
				  __FUNCTION__, __LINE__, res );
		return RMF_OSAL_ENODATA;
	}

	result_recv = ipcBuf.getResponse( &ret );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm get response failed in %s:%d ret = %d\n",
				  __FUNCTION__, __LINE__, result_recv );
		return RMF_OSAL_ENODATA;
	}
	
	result_recv = ipcBuf.collectData( pVctId, sizeof(unsigned short));
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm get response failed in %s:%d ret = %d\n",
				  __FUNCTION__, __LINE__, result_recv );
		return RMF_OSAL_ENODATA;
	}
#endif // USE_POD_IPC

	return ret;
}


int IPC_CLIENT_SNMPGetGenFetEasLocationCode(unsigned char *pState,unsigned short *pDDDDxxCCCCCCCCCC)
{
	rmf_Error ret = RMF_SUCCESS;

#ifdef USE_POD_IPC
	int32_t result_recv = 0;
	int8_t res = 0;

	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_EAS_LOCATION);
    
	res = ipcBuf.sendCmd( pod_server_cmd_port_no  );
	if ( 0 != res )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm failure in %s:%d: ret = %d\n",
				  __FUNCTION__, __LINE__, res );
		return RMF_OSAL_ENODATA;
	}

	result_recv = ipcBuf.getResponse( &ret );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm get response failed in %s:%d ret = %d\n",
				  __FUNCTION__, __LINE__, result_recv );
		return RMF_OSAL_ENODATA;
	}
	
	result_recv = ipcBuf.collectData( pState, sizeof(unsigned char));
	result_recv = ipcBuf.collectData( pDDDDxxCCCCCCCCCC, sizeof(unsigned short));
	
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm get response failed in %s:%d ret = %d\n",
				  __FUNCTION__, __LINE__, result_recv );
		return RMF_OSAL_ENODATA;
	}
#endif // USE_POD_IPC

	return ret;
}

VL_HOST_GENERIC_FEATURES_RESULT IPC_CLIENT_vlPodGetGenericFeatureData(VL_HOST_FEATURE_PARAM eFeature, void * _pvStruct)
{
	rmf_Error ret = RMF_SUCCESS;

#ifdef USE_POD_IPC
	int32_t result_recv = 0;
	int8_t res = 0;

	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_GEN_FEATURE_DATA);
	
	result_recv |= ipcBuf.addData( ( const void * ) &eFeature, sizeof( eFeature ) );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"\nPOD Comm add data failed in %s:%d: %d\n",
				  __FUNCTION__, __LINE__ , result_recv);
	      return VL_HOST_GENERIC_FEATURES_RESULT_ERROR;
	}

	res = ipcBuf.sendCmd( pod_server_cmd_port_no );
	if ( 0 != res )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm failure in %s:%d: ret = %d\n",
				  __FUNCTION__, __LINE__, res );
		return VL_HOST_GENERIC_FEATURES_RESULT_ERROR;
	}

	result_recv = ipcBuf.getResponse( &ret );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm get response failed in %s:%d ret = %d\n",
				  __FUNCTION__, __LINE__, result_recv );
		return VL_HOST_GENERIC_FEATURES_RESULT_ERROR;
	}
	
	result_recv = ipcBuf.collectData( _pvStruct, sizeof(VL_POD_HOST_FEATURE_EAS));
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm get response failed in %s:%d ret = %d\n",
				  __FUNCTION__, __LINE__, result_recv );
		return VL_HOST_GENERIC_FEATURES_RESULT_ERROR;
	}
#endif // USE_POD_IPC

	return VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS;
}

int IPC_CLIENT_CHALDsg_Set_Config(int device_instance, unsigned long ulTag, void* pvConfig)
{
        
	rmf_Error ret = RMF_SUCCESS;

#ifdef USE_POD_IPC
	int32_t result_recv = 0;
	int8_t res = 0;
	int8_t ipMode = (int8_t)((unsigned long)(pvConfig));

	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_SET_DSG_CONFIG);

	result_recv |= ipcBuf.addData( ( const void * ) &device_instance, sizeof( device_instance ) );
	result_recv |= ipcBuf.addData( ( const void * ) &ulTag, sizeof( ulTag ) );
	result_recv |= ipcBuf.addData( ( const void * ) &ipMode, sizeof( ipMode ) );

	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"\nPOD Comm add data failed in %s:%d: %d\n",
				  __FUNCTION__, __LINE__ , result_recv);
		return RMF_OSAL_ENODATA;
	}

	res = ipcBuf.sendCmd( pod_server_cmd_port_no  );
	if ( 0 != res )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm failure in %s:%d: ret = %d\n",
				  __FUNCTION__, __LINE__, res );
		return RMF_OSAL_ENODATA;
	}

	result_recv = ipcBuf.getResponse( &ret );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm get response failed in %s:%d ret = %d\n",
				  __FUNCTION__, __LINE__, result_recv );
		return RMF_OSAL_ENODATA;
	}

	/*We are doing a SET not a GET.
    result_recv = ipcBuf.collectData(pvConfig, sizeof(pvConfig));
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm get response failed in %s:%d ret = %d\n",
				  __FUNCTION__, __LINE__, result_recv );
		return RMF_OSAL_ENODATA;
	}*/
#endif // USE_POD_IPC

	return ret;
}

rmf_Error IPC_CLIENT_vlMpeosDumpBuffer(rdk_LogLevel lvl, const char *mod, const void * pBuffer, int nBufSize)
{
	rmf_Error ret = RMF_SUCCESS;

#ifdef USE_POD_IPC
	int32_t result_recv = 0;
	int8_t res = 0;

	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_DUMP_BUFFER);

	result_recv |= ipcBuf.addData( ( const void * ) &lvl, sizeof( lvl ) );
	result_recv |= ipcBuf.addData( ( const void * ) &mod, sizeof( char ) );
	result_recv |= ipcBuf.addData( ( const void * ) &nBufSize, sizeof( nBufSize ) );
	result_recv |= ipcBuf.addData( ( const void * ) pBuffer, nBufSize );

	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"\nPOD Comm add data failed in %s:%d: %d\n",
				  __FUNCTION__, __LINE__ , result_recv);
		return RMF_OSAL_ENODATA;
	}

	res = ipcBuf.sendCmd( pod_server_cmd_port_no);
	if ( 0 != res )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm failure in %s:%d: ret = %d\n",
				  __FUNCTION__, __LINE__, res );
		return RMF_OSAL_ENODATA;
	}

	result_recv = ipcBuf.getResponse( &ret );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm get response failed in %s:%d ret = %d\n",
				  __FUNCTION__, __LINE__, result_recv );
		return RMF_OSAL_ENODATA;
	}
#endif // USE_POD_IPC

	return ret;
}
int IPC_CLIENT_SNMPGetGenFetDayLightSavingsTimeExit(unsigned long *pTimeExit)
{
	rmf_Error ret = RMF_SUCCESS;

#ifdef USE_POD_IPC
	int32_t result_recv = 0;
	int8_t res = 0;

	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_DST_EXIT);

	res = ipcBuf.sendCmd( pod_server_cmd_port_no );
	if ( 0 != res )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm failure in %s:%d: ret = %d\n",
				  __FUNCTION__, __LINE__, res );
		return RMF_OSAL_ENODATA;
	}

	result_recv = ipcBuf.getResponse( &ret );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm get response failed in %s:%d ret = %d\n",
				  __FUNCTION__, __LINE__, result_recv );
		return RMF_OSAL_ENODATA;
	}

	result_recv = ipcBuf.collectData(pTimeExit, sizeof(unsigned long ));
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm get response failed in %s:%d ret = %d\n",
				  __FUNCTION__, __LINE__, result_recv );
		return RMF_OSAL_ENODATA;
	}
#endif // USE_POD_IPC

	return ret;
}

int IPC_CLIENT_SNMPGetGenFetDayLightSavingsTimeEntry(unsigned long *pTimeEntry)
{
	rmf_Error ret = RMF_SUCCESS;

#ifdef USE_POD_IPC
	int32_t result_recv = 0;
	int8_t res = 0;

	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_DST_ENTRY);

	res = ipcBuf.sendCmd(pod_server_cmd_port_no);
	if ( 0 != res )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm failure in %s:%d: ret = %d\n",
				  __FUNCTION__, __LINE__, res );
		return RMF_OSAL_ENODATA;
	}

	result_recv = ipcBuf.getResponse( &ret );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm get response failed in %s:%d ret = %d\n",
				  __FUNCTION__, __LINE__, result_recv );
		return RMF_OSAL_ENODATA;
	}

	result_recv = ipcBuf.collectData(pTimeEntry, sizeof(unsigned long));
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm get response failed in %s:%d ret = %d\n",
				  __FUNCTION__, __LINE__, result_recv );
		return RMF_OSAL_ENODATA;
	}
#endif // USE_POD_IPC

	return ret;
}

int IPC_CLIENT_SNMPGetGenFetDayLightSavingsTimeDelta(char *pDelta)
{
	rmf_Error ret = RMF_SUCCESS;

#ifdef USE_POD_IPC
	int32_t result_recv = 0;
	int8_t res = 0;
	int Deltalen = 0;
	char *aDelta[12] = {0};

	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_DST_DELTA);

	res = ipcBuf.sendCmd( pod_server_cmd_port_no );
	if ( 0 != res )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm failure in %s:%d: ret = %d\n",
				  __FUNCTION__, __LINE__, res );
		return RMF_OSAL_ENODATA;
	}

	result_recv = ipcBuf.getResponse( &ret );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm get response failed in %s:%d ret = %d\n",
				  __FUNCTION__, __LINE__, result_recv );
		return RMF_OSAL_ENODATA;
	}
	ipcBuf.collectData(&Deltalen, sizeof(Deltalen));
	result_recv = ipcBuf.collectData(aDelta, Deltalen);
	if( result_recv != 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm get response failed in %s:%d ret = %d\n",
				  __FUNCTION__, __LINE__, result_recv );
		return RMF_OSAL_ENODATA;
	}

	RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				"DST delta lenght is %d, string is %s %s:%d ret = %d\n",
					  Deltalen, aDelta, __FUNCTION__, __LINE__,  result_recv );		
	//copying only one byte as in the original code to avoid the corruption
	memcpy(pDelta, aDelta, 1);

#endif // USE_POD_IPC
	return ret;
}

int IPC_CLIENT_SNMPGetGenFetTimeZoneOffset(int *pTZOffset)
{
	rmf_Error ret = RMF_SUCCESS;

#ifdef USE_POD_IPC
	int32_t result_recv = 0;
	int8_t res = 0;

	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_GEN_TIME_ZONE_OFFSET);

	res = ipcBuf.sendCmd( pod_server_cmd_port_no);
	if ( 0 != res )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm failure in %s:%d: ret = %d\n",
				  __FUNCTION__, __LINE__, res );
		return RMF_OSAL_ENODATA;
	}

	result_recv = ipcBuf.getResponse( &ret );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm get response failed in %s:%d ret = %d\n",
				  __FUNCTION__, __LINE__, result_recv );
		return RMF_OSAL_ENODATA;
	}

	result_recv = ipcBuf.collectData(pTZOffset, sizeof( int ));
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm get response failed in %s:%d ret = %d\n",
				  __FUNCTION__, __LINE__, result_recv );
		return RMF_OSAL_ENODATA;
	}
#endif // USE_POD_IPC

	return ret;
}

int IPC_CLIENT_SNMPGetGenFetResourceId(unsigned long *pRrsId)
{
	rmf_Error ret = RMF_SUCCESS;

#ifdef USE_POD_IPC
	int32_t result_recv = 0;
	int8_t res = 0;

	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_GEN_RESOURCE_ID);

	res = ipcBuf.sendCmd(pod_server_cmd_port_no);
	if ( 0 != res )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm failure in %s:%d: ret = %d\n",
				  __FUNCTION__, __LINE__, res );
		return RMF_OSAL_ENODATA;
	}

	result_recv = ipcBuf.getResponse( &ret );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm get response failed in %s:%d ret = %d\n",
				  __FUNCTION__, __LINE__, result_recv );
		return RMF_OSAL_ENODATA;
	}

	result_recv = ipcBuf.collectData(pRrsId, sizeof(unsigned long));
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm get response failed in %s:%d ret = %d\n",
				  __FUNCTION__, __LINE__, result_recv );
		return RMF_OSAL_ENODATA;
	}
#endif // USE_POD_IPC

	return ret;
}

int IPC_CLIENT_SNMPGetCardIdBndStsOobMode(vlCableCardIdBndStsOobMode_t *pCardInfo)
{
	rmf_Error ret = RMF_SUCCESS;

#ifdef USE_POD_IPC
	int32_t result_recv = 0;
	int8_t res = 0;

	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_INBAND_OOB_MODE);

	res = ipcBuf.sendCmd( pod_server_cmd_port_no );
	if ( 0 != res )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm failure in %s:%d: ret = %d\n",
				  __FUNCTION__, __LINE__, res );
		return RMF_OSAL_ENODATA;
	}

	result_recv = ipcBuf.getResponse( &ret );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm get response failed in %s:%d ret = %d\n",
				  __FUNCTION__, __LINE__, result_recv );
		return RMF_OSAL_ENODATA;
	}

	result_recv = ipcBuf.collectData(pCardInfo, sizeof(vlCableCardIdBndStsOobMode_t));
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm get response failed in %s:%d ret = %d\n",
				  __FUNCTION__, __LINE__, result_recv );
		return RMF_OSAL_ENODATA;
	}
#endif // USE_POD_IPC

	return ret;
}

int IPC_CLIENT_SNMPGetCableCardCertInfo(vlCableCardCertInfo_t *pCardCertInfo)
{
 	rmf_Error ret = RMF_SUCCESS;

#ifdef USE_POD_IPC
        int32_t result_recv = 0;
        int8_t res = 0;
	int8_t mcardCount = 0;
	
	for(mcardCount = 0 ;mcardCount < 3 ; mcardCount++)
	{	  
		IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_CABLE_CARD_CERT_INFO);
		
		result_recv |= ipcBuf.addData( ( const void * )& mcardCount, sizeof( mcardCount ) );

		if( result_recv < 0)
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
				"\nPOD Comm add data failed in %s:%d: %d\n",
					__FUNCTION__, __LINE__ , result_recv);
			return RMF_OSAL_ENODATA;
		}
		res = ipcBuf.sendCmd(pod_server_cmd_port_no );
		if ( 0 != res )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
				"POD Comm failure in %s:%d: ret = %d\n",
					__FUNCTION__, __LINE__, res );
			return RMF_OSAL_ENODATA;
		}

		result_recv = ipcBuf.getResponse( &ret );
		if( result_recv < 0)
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
				"POD Comm get response failed in %s:%d ret = %d\n",
					__FUNCTION__, __LINE__, result_recv );
			return RMF_OSAL_ENODATA;
		}

		switch(mcardCount)
		{
			case 0 :
			{   
			    result_recv = ipcBuf.collectData( ( void * ) &pCardCertInfo->szDispString,
							    sizeof( pCardCertInfo->szDispString ) );
			    result_recv = ipcBuf.collectData( ( void * ) &pCardCertInfo->szDeviceCertSubjectName,
							    sizeof( pCardCertInfo->szDeviceCertSubjectName ) );
			    result_recv = ipcBuf.collectData( ( void * ) &pCardCertInfo->szDeviceCertIssuerName,
							    sizeof( pCardCertInfo->szDeviceCertIssuerName ) );
			}
			break ;
			case 1:
			{
			    result_recv = ipcBuf.collectData( ( void * ) &pCardCertInfo->szManCertSubjectName,
							    sizeof( pCardCertInfo->szManCertSubjectName ) );
			    result_recv = ipcBuf.collectData( ( void * ) &pCardCertInfo->szManCertIssuerName,
							    sizeof( pCardCertInfo->szManCertIssuerName ) );
			    result_recv = ipcBuf.collectData( ( void * ) &pCardCertInfo->acHostId,
							    sizeof( pCardCertInfo->acHostId ) );
			}
			break;
			
			case 2:
			{
			  
			    result_recv = ipcBuf.collectData( ( void * ) &pCardCertInfo->bCertAvailable,
							    sizeof( pCardCertInfo->bCertAvailable ) );
			    result_recv = ipcBuf.collectData( ( void * ) &pCardCertInfo->bCertValid,
							    sizeof( pCardCertInfo->bCertValid ) );
			    result_recv = ipcBuf.collectData( ( void * ) &pCardCertInfo->bVerifiedWithChain,
							    sizeof( pCardCertInfo->bVerifiedWithChain ) );
			    result_recv = ipcBuf.collectData( ( void * ) &pCardCertInfo->bIsProduction,
							    sizeof( pCardCertInfo->bIsProduction ) );
			}
			break ;
			default :
			{ 
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
					  "POD Comm get response failed in %s:%d ret = %d\n",
						  __FUNCTION__, __LINE__, result_recv );
				return RMF_OSAL_ENODATA;
			    
			}
		  
		}
	}
  
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm get response failed in %s:%d ret = %d\n",
				__FUNCTION__, __LINE__, result_recv );
		return RMF_OSAL_ENODATA;
	}
#endif // USE_POD_IPC

        return ret;
}

int IPC_CLIENT_SNMPget_ocStbHostSecurityIdentifier(HostCATypeInfo_t* HostCAtypeinfo)
{
	rmf_Error ret = RMF_SUCCESS;

#ifdef USE_POD_IPC
	int32_t result_recv = 0;
	int8_t res = 0;

	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_HOST_SECURITY_ID);

	res = ipcBuf.sendCmd( pod_server_cmd_port_no  );
	if ( 0 != res )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm failure in %s:%d: ret = %d\n",
				  __FUNCTION__, __LINE__, res );
		return RMF_OSAL_ENODATA;
	}

	result_recv = ipcBuf.getResponse( &ret );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm get response failed in %s:%d ret = %d\n",
				  __FUNCTION__, __LINE__, result_recv );
		return RMF_OSAL_ENODATA;
	}

	result_recv = ipcBuf.collectData(HostCAtypeinfo, sizeof(HostCATypeInfo_t));
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm get response failed in %s:%d ret = %d\n",
				  __FUNCTION__, __LINE__, result_recv );
		return RMF_OSAL_ENODATA;
	}
#endif // USE_POD_IPC

	return ret;
}

int IPC_CLIENT_SNMPGetApplicationInfoWithPages(vlCableCardAppInfo_t *pAppInfo)
{
	rmf_Error ret = RMF_SUCCESS;

#ifdef USE_POD_IPC
        int32_t result_recv = 0;
        int8_t res = 0;
	int mmiPages = 0 ;

	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_NUMBER_OF_CC_APPS);

	res = ipcBuf.sendCmd( pod_server_cmd_port_no  );
    if ( 0 != res )
    {
        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
            "POD Comm failure in %s:%d: ret = %d\n",
                __FUNCTION__, __LINE__, res );
        return RMF_OSAL_ENODATA;
    }

    result_recv = ipcBuf.getResponse( &ret );
    if( result_recv < 0)
    {
        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
            "POD Comm get response failed in %s:%d ret = %d\n",
                __FUNCTION__, __LINE__, result_recv );
        return RMF_OSAL_ENODATA;

    }
	result_recv = ipcBuf.collectData( ( void * ) &pAppInfo->CardNumberOfApps,
                            sizeof( pAppInfo->CardNumberOfApps));
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.SNMP", "%s:%d: number of CableCard apps=%d\n", __FUNCTION__, __LINE__, pAppInfo->CardNumberOfApps );
	
	for(mmiPages = 0 ; mmiPages < pAppInfo->CardNumberOfApps ; mmiPages++)
	{
		IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_APP_INFO_WITH_PAGES);
		
		result_recv |= ipcBuf.addData( ( const void * )& mmiPages, sizeof( mmiPages ) );
		if( result_recv < 0)
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
				"POD Comm get response failed in %s:%d ret = %d\n",
					__FUNCTION__, __LINE__, result_recv );
			return RMF_OSAL_ENODATA;
		}

		res = ipcBuf.sendCmd( pod_server_cmd_port_no  );
		if ( 0 != res )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
				"POD Comm failure in %s:%d: ret = %d\n",
					__FUNCTION__, __LINE__, res );
			return RMF_OSAL_ENODATA;
		}

		result_recv = ipcBuf.getResponse( &ret );
		if( result_recv < 0)
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
				"POD Comm get response failed in %s:%d ret = %d\n",
					__FUNCTION__, __LINE__, result_recv );
			return RMF_OSAL_ENODATA;
			
		}   
		//if(mmiPages < 7)
		{
			result_recv = ipcBuf.collectData( ( void * ) &pAppInfo->Apps[mmiPages].AppName,
							sizeof( pAppInfo->Apps[mmiPages].AppName));
			
			result_recv = ipcBuf.collectData( ( void * ) &pAppInfo->Apps[mmiPages].AppType,
							sizeof( pAppInfo->Apps[mmiPages].AppType ) );
			
			result_recv = ipcBuf.collectData( ( void * ) &pAppInfo->Apps[mmiPages].VerNum,
							sizeof( pAppInfo->Apps[mmiPages].VerNum ) );
			
			result_recv = ipcBuf.collectData( ( void * ) &pAppInfo->Apps[mmiPages].AppNameLen,
							sizeof( pAppInfo->Apps[mmiPages].AppNameLen));
			
			result_recv = ipcBuf.collectData( ( void * ) &pAppInfo->Apps[mmiPages].AppUrlLen,
							sizeof( pAppInfo->Apps[mmiPages].AppUrlLen));
			
			result_recv = ipcBuf.collectData( ( void * ) &pAppInfo->Apps[mmiPages].nSubPages,
							sizeof( pAppInfo->Apps[mmiPages].nSubPages));
			result_recv = ipcBuf.collectData( ( void * ) &pAppInfo->Apps[mmiPages].AppUrl,
							sizeof( pAppInfo->Apps[mmiPages].AppUrl));
		}
#if 0
		else if(mmiPages == 7)
		{
			result_recv = ipcBuf.collectData( ( void * ) &pAppInfo->CardNumberOfApps,
					sizeof( pAppInfo->CardNumberOfApps ) );	
		}
#endif
		if( result_recv < 0)
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
				"POD Comm get response failed in %s:%d ret = %d\n",
					__FUNCTION__, __LINE__, result_recv );
			return RMF_OSAL_ENODATA;
		}		
	}
#endif // USE_POD_IPC

	return ret;
}

int IPC_CLIENT_SNMPGetApplicationInfo(vlCableCardAppInfo_t *pCardAppInfo)
{
	rmf_Error ret = RMF_SUCCESS;

#ifdef USE_POD_IPC
	int32_t result_recv = 0;
	int8_t res = 0;
	int8_t index = 0;

	for(index = 0; index <= 4; index++)
	{
		IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_APP_INFO);
		result_recv |= ipcBuf.addData( ( const void * )& index, sizeof( index ) );

		res = ipcBuf.sendCmd( pod_server_cmd_port_no );
		if ( 0 != res )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
				"POD Comm failure in %s:%d: ret = %d\n",
					  __FUNCTION__, __LINE__, res );
			return RMF_OSAL_ENODATA;
		}

		result_recv = ipcBuf.getResponse( &ret );
		if( result_recv < 0)
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
				"POD Comm get response failed in %s:%d ret = %d\n",
					  __FUNCTION__, __LINE__, result_recv );
			return RMF_OSAL_ENODATA;
		}

		switch(index)
		{
			case 0:
				result_recv = ipcBuf.collectData( ( void * ) &pCardAppInfo->CardManufactureId,
                            sizeof( pCardAppInfo->CardManufactureId));
				result_recv = ipcBuf.collectData( ( void * ) &pCardAppInfo->CardVersion,
                            sizeof( pCardAppInfo->CardVersion));
				result_recv = ipcBuf.collectData( ( void * ) &pCardAppInfo->CardMAC,
                            sizeof( pCardAppInfo->CardMAC));
				result_recv = ipcBuf.collectData( ( void * ) &pCardAppInfo->CardSerialNumberLen,
                            sizeof( pCardAppInfo->CardSerialNumberLen));
				result_recv = ipcBuf.collectData( ( void * ) &pCardAppInfo->CardSerialNumber,
                            sizeof( pCardAppInfo->CardSerialNumber));
				result_recv = ipcBuf.collectData( ( void * ) &pCardAppInfo->CardRootOidLen,
                            sizeof( pCardAppInfo->CardRootOidLen));
				result_recv = ipcBuf.collectData( ( void * ) &pCardAppInfo->CardRootOid,
                            sizeof( pCardAppInfo->CardRootOid));
				result_recv = ipcBuf.collectData( ( void * ) &pCardAppInfo->CardNumberOfApps,
                            sizeof( pCardAppInfo->CardNumberOfApps));
				result_recv = ipcBuf.collectData( ( void * ) &pCardAppInfo->Apps[0],
                            sizeof( pCardAppInfo->Apps[0]));
				result_recv = ipcBuf.collectData( ( void * ) &pCardAppInfo->Apps[1],
                            sizeof( pCardAppInfo->Apps[1]));
			break;
			case 1:
				result_recv = ipcBuf.collectData( ( void * ) &pCardAppInfo->Apps[2],
                            sizeof( pCardAppInfo->Apps[2]));
				result_recv = ipcBuf.collectData( ( void * ) &pCardAppInfo->Apps[3],
                            sizeof( pCardAppInfo->Apps[3]));
				result_recv = ipcBuf.collectData( ( void * ) &pCardAppInfo->Apps[4],
                            sizeof( pCardAppInfo->Apps[4]));
			break;
			case 2:
				result_recv = ipcBuf.collectData( ( void * ) &pCardAppInfo->Apps[5],
                            sizeof( pCardAppInfo->Apps[5]));
				result_recv = ipcBuf.collectData( ( void * ) &pCardAppInfo->Apps[6],
                            sizeof( pCardAppInfo->Apps[6]));
				result_recv = ipcBuf.collectData( ( void * ) &pCardAppInfo->Apps[7],
                            sizeof( pCardAppInfo->Apps[7]));
			break;
			case 3:
				result_recv = ipcBuf.collectData( ( void * ) &pCardAppInfo->Apps[8],
                            sizeof( pCardAppInfo->Apps[8]));
				result_recv = ipcBuf.collectData( ( void * ) &pCardAppInfo->Apps[9],
                            sizeof( pCardAppInfo->Apps[9]));
				result_recv = ipcBuf.collectData( ( void * ) &pCardAppInfo->Apps[10],
                            sizeof( pCardAppInfo->Apps[10]));
			break;
			case 4:
				result_recv = ipcBuf.collectData( ( void * ) &pCardAppInfo->Apps[11],
                            sizeof( pCardAppInfo->Apps[11]));
			break;
			default:
				result_recv = ipcBuf.collectData(pCardAppInfo, sizeof(vlCableCardAppInfo_t));
			break;
		}
		if( result_recv < 0)
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
				"POD Comm get response failed in %s:%d ret = %d\n",
					  __FUNCTION__, __LINE__, result_recv );
			return RMF_OSAL_ENODATA;
		}
	}
#endif // USE_POD_IPC

	return ret;
}

int IPC_CLIENT_SNMPGet_ocStbHostHostID(char* HostID,char len)
{
	rmf_Error ret = RMF_SUCCESS;

#ifdef USE_POD_IPC
	int32_t result_recv = 0;
	int8_t res = 0;

	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_STB_HOST_ID);

	result_recv |= ipcBuf.addData( ( const void * ) &len, sizeof( char ) );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"\nPOD Comm add data failed in %s:%d: %d\n",
				  __FUNCTION__, __LINE__ , result_recv);
		return RMF_OSAL_ENODATA;
	}

	res = ipcBuf.sendCmd( pod_server_cmd_port_no  );
	if ( 0 != res )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm failure in %s:%d: ret = %d\n",
				  __FUNCTION__, __LINE__, res );
		return RMF_OSAL_ENODATA;
	}

	result_recv = ipcBuf.getResponse( &ret );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm get response failed in %s:%d ret = %d\n",
				  __FUNCTION__, __LINE__, result_recv );
		return RMF_OSAL_ENODATA;
	}

	result_recv = ipcBuf.collectData(HostID, result_recv);
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm get response failed in %s:%d ret = %d\n",
				  __FUNCTION__, __LINE__, result_recv );
		return RMF_OSAL_ENODATA;
	}
#endif // USE_POD_IPC

	return ret;
}

int IPC_CLIENT_SNMPSendCardMibAccSnmpReq( unsigned char *pMessage, unsigned long MsgLength)
{
        rmf_Error ret = RMF_SUCCESS;

#ifdef USE_POD_IPC
        int32_t result_recv = 0;
        int8_t res = 0;

        IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_SEND_CARD_MIB_ACC_SNMP_REQ);
	
        result_recv |= ipcBuf.addData( ( const void * ) &MsgLength, sizeof( unsigned long ) );
        result_recv |= ipcBuf.addData( ( const void * ) pMessage, MsgLength );
        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "\nPOD Comm add data failed in %s:%d: %d\n",
                                 __FUNCTION__, __LINE__ , result_recv);
                return RMF_OSAL_ENODATA;
        }

        res = ipcBuf.sendCmd( pod_server_cmd_port_no  );
        if ( 0 != res )
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm failure in %s:%d: ret = %d\n",
                                 __FUNCTION__, __LINE__, res );
                return RMF_OSAL_ENODATA;
        }

        result_recv = ipcBuf.getResponse( &ret );
        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm get response failed in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, result_recv );
                return RMF_OSAL_ENODATA;
        }
#endif // USE_POD_IPC

	return ret;
}

int IPC_CLIENT_vlSnmpEcmGetIfStats(VL_HOST_IP_STATS * pIpStats)
{
        rmf_Error ret = RMF_SUCCESS;
#if 0
        int32_t result_recv = 0;
        int8_t res = 0;

        IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_ECM_GET_IF_STATUS);
        
        res = ipcBuf.sendCmd( pod_cmd_port_no  );
        if ( 0 != res )
        {
                RMF_LOG( RMF_LOG_ERROR, RMF_MOD_POD,
                        "POD Comm failure in %s:%d: ret = %d\n",
                                 __FUNCTION__, __LINE__, res );
                return RMF_OSAL_ENODATA;
        }

        result_recv = ipcBuf.getResponse( &ret );
        if( result_recv < 0)
        {
                RMF_LOG( RMF_LOG_ERROR, RMF_MOD_POD,
                        "POD Comm get response failed in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, result_recv );
                return RMF_OSAL_ENODATA;
        }
        result_recv = ipcBuf.collectData(pIpStats, sizeof(VL_HOST_IP_STATS));
                if( result_recv < 0)
        {
                RMF_LOG( RMF_LOG_ERROR, RMF_MOD_POD,
                        "POD Comm get response failed in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, result_recv );
                return RMF_OSAL_ENODATA;
        }
#endif
                return ret;
}


int IPC_CLIENT_vlXchanGetSocketFlowCount()
{
	rmf_Error ret = RMF_SUCCESS;
    int32_t gsfcRet = 0;

#ifdef USE_POD_IPC
        int32_t result_recv = 0;
        int8_t res = 0;

        IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_SOCKET_FLOW_COUNT);

        res = ipcBuf.sendCmd( pod_server_cmd_port_no );
        if ( 0 != res )
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm failure in %s:%d: ret = %d\n",
                                 __FUNCTION__, __LINE__, res );
                return RMF_OSAL_ENODATA;
        }

        result_recv = ipcBuf.getResponse( &ret );
        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm get response failed in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, result_recv );
                return RMF_OSAL_ENODATA;
        }
        result_recv = ipcBuf.collectData(&gsfcRet, sizeof(int32_t));
	if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm get response failed in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, result_recv );
                return RMF_OSAL_ENODATA;
        }
#endif // USE_POD_IPC

	return gsfcRet;
}

int IPC_CLIENT_CHalSys_snmp_request(VL_SNMP_REQUEST eRequest, void * _pvStruct)
{
	rmf_Error ret = RMF_SUCCESS;

#ifdef USE_POD_IPC
	int32_t result_recv = 0;
	int8_t res = 0;
	    VL_SNMP_API_RESULT snmpres;

	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_HALSYS_SNMP_REQUEST);
	
	result_recv = ipcBuf.addData( ( const void * ) &eRequest, sizeof( int ) );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm get response failed in %s:%d ret = %d\n",
				__FUNCTION__, __LINE__, result_recv );
		return RMF_OSAL_ENODATA;
	}
	
	res = ipcBuf.sendCmd( pod_server_cmd_port_no  );
	if ( 0 != res )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm failure in %s:%d: ret = %d\n",
				__FUNCTION__, __LINE__, res );
		return RMF_OSAL_ENODATA;
	}

	result_recv = ipcBuf.getResponse( &ret );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm get response failed in %s:%d ret = %d\n",
				__FUNCTION__, __LINE__, result_recv );
		return RMF_OSAL_ENODATA;
	}

	result_recv |= ipcBuf.collectData(&snmpres, sizeof(VL_SNMP_API_RESULT));
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"POD Comm get response failed in %s:%d ret = %d\n",
				__FUNCTION__, __LINE__, result_recv );
		return RMF_OSAL_ENODATA;
	}

	switch(eRequest)
	{
		case VL_SNMP_REQUEST_GET_SYS_SYSTEM_POWER_STATE:
		{
			result_recv |= ipcBuf.collectData(_pvStruct, sizeof(VL_SNMP_VAR_HOST_POWER_STATUS));
			if( result_recv < 0)
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
					"POD Comm get response failed in %s:%d ret = %d\n",
						__FUNCTION__, __LINE__, result_recv );
				ret = RMF_OSAL_ENODATA;
				break;
			}
		}
		break;
		case VL_SNMP_REQUEST_GET_SYS_SYSTEM_MEMORY_REPORT:
		{
			result_recv |= ipcBuf.collectData(_pvStruct, sizeof(VL_SNMP_SystemMemoryReportTable));
			if( result_recv < 0)
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
					"POD Comm get response failed in %s:%d ret = %d\n",
						__FUNCTION__, __LINE__, result_recv );
				ret = RMF_OSAL_ENODATA;
				break;
			}
		}
		break;
		default:
			ret = RMF_OSAL_ENODATA; 
		break;
	}
	if (snmpres != VL_SNMP_API_RESULT_SUCCESS)
	{
		ret = RMF_OSAL_ENODATA;
	}
#endif // USE_POD_IPC

	return ret;
}

int IPC_CLIENT_vlIsDsgMode()
{
	rmf_Error ret = RMF_SUCCESS;
    int isDsgMode = 0;

#ifdef USE_POD_IPC
        int32_t result_recv = 0;
        int8_t res = 0;

	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_IS_DSG_MODE);

        res = ipcBuf.sendCmd( pod_server_cmd_port_no  );
        if ( 0 != res )
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm failure in %s:%d: ret = %d\n",
                                 __FUNCTION__, __LINE__, res );
                return RMF_OSAL_ENODATA;
        }

        result_recv = ipcBuf.getResponse( &ret );
        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm get response failed in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, result_recv );
                return RMF_OSAL_ENODATA;
        }

        result_recv = ipcBuf.collectData(&isDsgMode, sizeof(int));
        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm get response failed in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, result_recv );
                return RMF_OSAL_ENODATA;
        }
#endif // USE_POD_IPC

	return isDsgMode;
}

rmf_Error IPC_CLIENT_podImplGetCaSystemId(unsigned short * pCaSystemId)
{
        rmf_Error ret = RMF_SUCCESS;

#ifdef USE_POD_IPC
        int32_t result_recv = 0;
        int8_t res = 0;

        IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_CASYSTEMID);

        res = ipcBuf.sendCmd( pod_server_cmd_port_no  );
        if ( 0 != res )
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm failure in %s:%d: ret = %d\n",
                                 __FUNCTION__, __LINE__, res );
                return RMF_OSAL_ENODATA;
        }

        result_recv = ipcBuf.getResponse( &ret );
        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm get response failed in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, result_recv );
                return RMF_OSAL_ENODATA;
        }

        result_recv = ipcBuf.collectData(pCaSystemId, sizeof(unsigned short));
        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm get response failed in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, result_recv );
                return RMF_OSAL_ENODATA;
        }
#endif // USE_POD_IPC

	return ret;
}

int IPC_CLIENT_CommonDownloadMgrSnmpCertificateStatus_check(CDLMgrSnmpCertificateStatus_t *pStatus)
{
	rmf_Error ret = RMF_SUCCESS;

#ifdef USE_POD_IPC
        int32_t result_recv = 0;
        int8_t res = 0;

        IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_CDL_HOST_FW_STATUS_CHECK);

        res = ipcBuf.sendCmd(pod_server_cmd_port_no  );
        if ( 0 != res )
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm failure in %s:%d: ret = %d\n",
                                 __FUNCTION__, __LINE__, res );
                return RMF_OSAL_ENODATA;
        }
        
        result_recv = ipcBuf.getResponse( &ret );
        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm get response failed in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, result_recv );
                return RMF_OSAL_ENODATA;
        }

        result_recv = ipcBuf.collectData(pStatus, sizeof(CDLMgrSnmpCertificateStatus_t));
        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm get response failed in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, result_recv );
                return RMF_OSAL_ENODATA;
        }
#endif // USE_POD_IPC

	return ret;
}

int IPC_CLIENT_CommonDownloadMgrSnmpHostFirmwareStatus(CDLMgrSnmpHostFirmwareStatus_t *pStatus)
{
	rmf_Error ret = RMF_SUCCESS;

#ifdef USE_POD_IPC
        int32_t result_recv = 0;
        int8_t res = 0;

        IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_CDL_HOST_FW_STATUS);

        res = ipcBuf.sendCmd( pod_server_cmd_port_no );
        if ( 0 != res )
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm failure in %s:%d: ret = %d\n",
                                 __FUNCTION__, __LINE__, res );
                return RMF_OSAL_ENODATA;
        }
	result_recv = ipcBuf.getResponse( &ret );
        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm get response failed in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, result_recv );
                return RMF_OSAL_ENODATA;
        }

        result_recv = ipcBuf.collectData(pStatus, sizeof(CDLMgrSnmpHostFirmwareStatus_t));
        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm get response failed in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, result_recv );
                return RMF_OSAL_ENODATA;
        }
#endif // USE_POD_IPC

	return ret;
}

int IPC_CLIENT_GetHostIdLuhn( unsigned char *pHostId,  char *pHostIdLu )
{
	rmf_Error ret = RMF_SUCCESS;

#ifdef USE_POD_IPC
        int32_t result_recv = 0;
        int8_t res = 0;

        IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_HOST_ID_LUHN);
 
	result_recv |= ipcBuf.addData( ( const void * )pHostId, sizeof( char[5] ) );
        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "\nPOD Comm add data failed in %s:%d: %d\n",
                                 __FUNCTION__, __LINE__ , result_recv);
                return RMF_OSAL_ENODATA;
        }

        res = ipcBuf.sendCmd( pod_server_cmd_port_no );
        if ( 0 != res )
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm failure in %s:%d: ret = %d\n",
                                 __FUNCTION__, __LINE__, res );
                return RMF_OSAL_ENODATA;
        }

        result_recv = ipcBuf.getResponse( &ret );
        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm get response failed in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, result_recv );
                return RMF_OSAL_ENODATA;
        }

	result_recv = ipcBuf.collectData(pHostIdLu, sizeof(char[256]));
  	if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm get response failed in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, result_recv );
                return RMF_OSAL_ENODATA;
        }
#endif // USE_POD_IPC

	return ret;
}

void IPC_CLIENT_vlXchanGetHostCardIpInfo(VL_HOST_CARD_IP_INFO * pCardIpInfo)
{
        rmf_Error ret = RMF_SUCCESS;

#ifdef USE_POD_IPC
        int32_t result_recv = 0;
        int8_t res = 0;

        IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_HOST_CABLE_CARD_INFO);

        res = ipcBuf.sendCmd( pod_server_cmd_port_no  );
        if ( 0 != res )
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm failure in %s:%d: ret = %d\n",
                                 __FUNCTION__, __LINE__, res );
                return ;
        }

        result_recv = ipcBuf.getResponse( &ret );
        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm get response failed in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, result_recv );
                return ;
        }

        result_recv = ipcBuf.collectData(pCardIpInfo, sizeof(VL_HOST_CARD_IP_INFO));
        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm get response failed in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, result_recv );
                return ;
        }
#endif // USE_POD_IPC

	return;
}

void IPC_CLIENT_vlXchanGetDsgCableCardIpInfo(VL_HOST_IP_INFO * pHostIpInfo)
{
	rmf_Error ret = RMF_SUCCESS;

#ifdef USE_POD_IPC
        int32_t result_recv = 0;
        int8_t res = 0;

        IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_DSG_CABLE_CARD_INFO);

        res = ipcBuf.sendCmd( pod_server_cmd_port_no  );
        if ( 0 != res )
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm failure in %s:%d: ret = %d\n",
                                 __FUNCTION__, __LINE__, res );
                return ;
        }

        result_recv = ipcBuf.getResponse( &ret );
        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm get response failed in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, result_recv );
                return ;
        }

        result_recv = ipcBuf.collectData(pHostIpInfo, sizeof(VL_HOST_IP_INFO));
  	if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm get response failed in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, result_recv );
                return ;
        }
#endif // USE_POD_IPC

	return;
}

int IPC_CLIENT_vlGetCcAppInfoPage(int iIndex, const char ** ppszPage, int * pnBytes)
{
 	rmf_Error ret = RMF_SUCCESS;

#ifdef USE_POD_IPC
        int32_t result_recv = 0;
        int8_t res = 0;

        IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_CCAPP_INFO);
	
        result_recv |= ipcBuf.addData( ( const void * ) &iIndex, sizeof( iIndex ) );
        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "\nPOD Comm add data failed in %s:%d: %d\n",
                                 __FUNCTION__, __LINE__ , result_recv);
                return RMF_OSAL_ENODATA;
        }
        if(NULL != pnBytes)
        {
            result_recv |= ipcBuf.addData( ( const void * ) pnBytes, sizeof( *pnBytes ) );
            if( result_recv < 0)
            {
                    RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                            "\nPOD Comm add data failed in %s:%d: %d\n",
                                     __FUNCTION__, __LINE__ , result_recv);
                    return RMF_OSAL_ENODATA;
            }
        }
        
        res = ipcBuf.sendCmd( pod_server_cmd_port_no);
        if ( 0 != res )
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm failure in %s:%d: ret = %d\n",
                                 __FUNCTION__, __LINE__, res );
                return RMF_OSAL_ENODATA;
        }

        result_recv = ipcBuf.getResponse( &ret );
        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm get response failed in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, result_recv );
                return RMF_OSAL_ENODATA;
        }
        
        result_recv |= ipcBuf.collectData(pnBytes, sizeof(int));
	if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm get response failed in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, result_recv );
                return RMF_OSAL_ENODATA;
        }
        
	rmf_osal_memAllocP( RMF_OSAL_MEM_POD, (*pnBytes)+1, ( void ** ) ppszPage );

	result_recv |= ipcBuf.collectData((void*)*ppszPage, (*pnBytes) + 1 );
  	if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm get response failed in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, result_recv );
                return RMF_OSAL_ENODATA;
        }
#endif // USE_POD_IPC

	return ret;
}

void IPC_CLIENT_vlXchanGetDsgIpInfo(VL_HOST_IP_INFO * pHostIpInfo)
{
	rmf_Error ret = RMF_SUCCESS;

#ifdef USE_POD_IPC
        int32_t result_recv = 0;
        int8_t res = 0;

        IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_DSG_IP_INFO);
      
        res = ipcBuf.sendCmd( pod_server_cmd_port_no  );
        if ( 0 != res )
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm failure in %s:%d: ret = %d\n",
                                 __FUNCTION__, __LINE__, res );
                return ;
        }
        
	result_recv = ipcBuf.getResponse( &ret );
        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm get response failed in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, result_recv );
                return ;
        }

	result_recv = ipcBuf.collectData(pHostIpInfo, sizeof(VL_HOST_IP_INFO));
  	if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm get response failed in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, result_recv );
                return ;
        }
#endif // USE_POD_IPC

	return;
}

unsigned char * IPC_CLIENT_vlXchanGetRcIpAddress()
{
    rmf_Error ret = RMF_SUCCESS;
    unsigned char  data[4];

#ifdef USE_POD_IPC
        int32_t result_recv = 0;
        int8_t res = 0;

        IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_RC_IPADDRESS);

        res = ipcBuf.sendCmd(pod_server_cmd_port_no );
        if ( 0 != res )
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm failure in %s:%d: ret = %d\n",
                                 __FUNCTION__, __LINE__, res );
                return NULL;
        }

        result_recv = ipcBuf.getResponse( &ret );
        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm get response failed in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, result_recv );
                return NULL;
        }
	result_recv = ipcBuf.collectData(&data, sizeof(data));
        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm get response failed in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, result_recv );
                return NULL;
        }
#endif // USE_POD_IPC

	return data;
}

void IPC_CLIENT_vlXchanSetEcmRoute()
{
 	rmf_Error ret = RMF_SUCCESS;

#ifdef USE_POD_IPC
        int32_t result_recv = 0;
        int8_t res = 0;

        IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_SET_ECM_ROUTE);

        res = ipcBuf.sendCmd(pod_server_cmd_port_no  );
        if ( 0 != res )
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm failure in %s:%d: ret = %d\n",
                                 __FUNCTION__, __LINE__, res );
                return ;
        }

        result_recv = ipcBuf.getResponse( &ret );
        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm get response failed in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, result_recv );
                return ;
        }
#endif // USE_POD_IPC

	return;
}

int IPC_CLIENT_vlXchanGetV6SubnetMaskFromPlen(int nPlen, unsigned char * pBuf, int nBufCapacity)
{
 	rmf_Error ret = RMF_SUCCESS;

#ifdef USE_POD_IPC
        int32_t result_recv = 0;
        int8_t res = 0;

        IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_IPV6_SUBMASK);
	
	result_recv |= ipcBuf.addData( ( const void * ) &nPlen, sizeof( nPlen ) );

        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "\nPOD Comm add data failed in %s:%d: %d\n",
                                 __FUNCTION__, __LINE__ , result_recv);
                return RMF_OSAL_ENODATA;
        }

        res = ipcBuf.sendCmd(pod_server_cmd_port_no);
        if ( 0 != res )
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm failure in %s:%d: ret = %d\n",
                                 __FUNCTION__, __LINE__, res );
                return RMF_OSAL_ENODATA;
        }

        result_recv = ipcBuf.getResponse( &ret );
        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm get response failed in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, result_recv );
                return RMF_OSAL_ENODATA;
        }

	result_recv = ipcBuf.collectData(pBuf, nBufCapacity);
  	if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm get response failed in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, result_recv );
                return RMF_OSAL_ENODATA;
        }
#endif // USE_POD_IPC

	return nBufCapacity;
}    

void IPC_CLIENT_vlXchanIndicateLostDsgIPUFlow()
{
        rmf_Error ret = RMF_SUCCESS;

#ifdef USE_POD_IPC
        int32_t result_recv = 0;
        int8_t res = 0;

        IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_LOST_DSG_IPU_INFO);

        res = ipcBuf.sendCmd(pod_server_cmd_port_no  );
        if ( 0 != res )
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm failure in %s:%d: ret = %d\n",
                                 __FUNCTION__, __LINE__, res );
                return ;
        }

        result_recv = ipcBuf.getResponse( &ret );
        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm get response failed in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, result_recv );
                return ;
        }
#endif // USE_POD_IPC

	return;
}

rmf_Error IPC_CLIENT_OobGetDacId(uint16_t *pDACId)
{
        rmf_Error ret = RMF_SUCCESS;

#ifdef USE_POD_IPC
        int32_t result_recv = 0;
        int8_t res = 0;

        IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_DACID);
	
        res = ipcBuf.sendCmd( pod_server_cmd_port_no );
        if ( 0 != res )
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm failure in %s:%d: ret = %d\n",
                                 __FUNCTION__, __LINE__, res );
                return RMF_OSAL_ENODATA;
        }

        result_recv = ipcBuf.getResponse( &ret );
        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm get response failed in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, result_recv );
                return RMF_OSAL_ENODATA;
        }
        
	result_recv = ipcBuf.collectData(pDACId, sizeof(uint16_t));
        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm get response failed in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, result_recv );
                return RMF_OSAL_ENODATA;
        }
#endif // USE_POD_IPC

	return ret;
}

rmf_Error IPC_CLIENT_OobGetChannelMapId(uint16_t *pChannelMapId)
{
        rmf_Error ret = RMF_SUCCESS;

#ifdef USE_POD_IPC
	int32_t result_recv = 0;
        int8_t res = 0;

        IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_CHANNEL_MAPID);
	
        res = ipcBuf.sendCmd( pod_server_cmd_port_no);
        if ( 0 != res )
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm failure in %s:%d: ret = %d\n",
                                 __FUNCTION__, __LINE__, res );
                return RMF_OSAL_ENODATA;
        }

        result_recv = ipcBuf.getResponse( &ret );
        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm get response failed in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, result_recv );
                return RMF_OSAL_ENODATA;
        }
        
	result_recv = ipcBuf.collectData(pChannelMapId, sizeof(uint16_t));
        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm get response failed in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, result_recv );
                return RMF_OSAL_ENODATA;
        }
#endif // USE_POD_IPC

	return ret;
}

void IPC_CLIENT_vlXchanGetDsgEthName(VL_HOST_IP_INFO * pHostIpInfo)
{
        rmf_Error ret = RMF_SUCCESS;

#ifdef USE_POD_IPC
        int32_t result_recv = 0;
        int8_t res = 0;

        IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_DSG_ETH_INFO);

        res = ipcBuf.sendCmd( pod_server_cmd_port_no  );
        if ( 0 != res )
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm failure in %s:%d: ret = %d\n",
                                 __FUNCTION__, __LINE__, res );
		return ;
        }

        result_recv = ipcBuf.getResponse( &ret);
        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm get response failed in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, result_recv );
		return ;
        }
        
 	result_recv = ipcBuf.collectData(pHostIpInfo, sizeof(VL_HOST_IP_INFO));
        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                        "POD Comm get response failed in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, result_recv );
                return ;
        }
#endif // USE_POD_IPC

	return;
}

rmf_Error IPC_CLIENT_vlMCardGetCardVersionNumberFromMMI(char * pszVersionNumber, int nCapacity)
{
    rmf_Error ret = RMF_SUCCESS;

#ifdef USE_POD_IPC
    int32_t result_recv = 0;
    int8_t res = 0;

    IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_CABLE_CARD_VERSION);

    res = ipcBuf.sendCmd( pod_server_cmd_port_no  );
    if ( 0 != res )
    {
        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                "POD Comm failure in %s:%d: ret = %d\n",
                         __FUNCTION__, __LINE__, res );
        return RMF_OSAL_ENODATA;
    }

    result_recv = ipcBuf.getResponse( &ret);
    if( result_recv < 0)
    {
        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                "POD Comm get response failed in %s:%d ret = %d\n",
                         __FUNCTION__, __LINE__, result_recv );
        return RMF_OSAL_ENODATA;
    }
    vlCableCardVersionInfo_t cardVersion;
    memset((void*)&cardVersion, 0, sizeof(cardVersion));
    result_recv = ipcBuf.collectData((void*)&cardVersion, sizeof(cardVersion));
    if( result_recv < 0)
    {
        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                "POD Comm get response failed in %s:%d ret = %d\n",
                         __FUNCTION__, __LINE__, result_recv );
        return RMF_OSAL_ENODATA;
    }
    
    strncpy(pszVersionNumber, cardVersion.szCardVersion, nCapacity);
#endif // USE_POD_IPC

	return ret;
}

int IPC_CLIENT_SNMPGetPatTimeoutCount(unsigned int * pValue)
{
	rmf_Error ret = RMF_SUCCESS;

#ifdef USE_POD_IPC
        int32_t result_recv = 0;
        int8_t res = 0;
	int mmiPages = 0 ;

	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) QAM_SRC_SERVER_CMD_GET_PAT_TIMEOUT_COUNT);

	res = ipcBuf.sendCmd( qam_src_server_cmd_port_no  );
    if ( 0 != res )
    {
        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
            "POD Comm failure in %s:%d: ret = %d\n",
                __FUNCTION__, __LINE__, res );
        return RMF_OSAL_ENODATA;
    }

    result_recv = ipcBuf.getResponse( &ret );
    if( result_recv < 0)
    {
        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
            "POD Comm get response failed in %s:%d ret = %d\n",
                __FUNCTION__, __LINE__, result_recv );
        return RMF_OSAL_ENODATA;

    }
	result_recv = ipcBuf.collectData( ( void * ) pValue,
                            sizeof( *pValue));
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.SNMP", "%s:%d: PAT_TIMEOUT_COUNT=%u\n", __FUNCTION__, __LINE__, *pValue );
#endif // USE_POD_IPC

	return ret;
}

int IPC_CLIENT_SNMPGetPmtTimeoutCount(unsigned int * pValue)
{
	rmf_Error ret = RMF_SUCCESS;

#ifdef USE_POD_IPC
        int32_t result_recv = 0;
        int8_t res = 0;
	int mmiPages = 0 ;

	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) QAM_SRC_SERVER_CMD_GET_PMT_TIMEOUT_COUNT);

	res = ipcBuf.sendCmd( qam_src_server_cmd_port_no  );
    if ( 0 != res )
    {
        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
            "POD Comm failure in %s:%d: ret = %d\n",
                __FUNCTION__, __LINE__, res );
        return RMF_OSAL_ENODATA;
    }

    result_recv = ipcBuf.getResponse( &ret );
    if( result_recv < 0)
    {
        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
            "POD Comm get response failed in %s:%d ret = %d\n",
                __FUNCTION__, __LINE__, result_recv );
        return RMF_OSAL_ENODATA;

    }
	result_recv = ipcBuf.collectData( ( void * ) pValue,
                            sizeof( *pValue));
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.SNMP", "%s:%d: PMT_TIMEOUT_COUNT=%u\n", __FUNCTION__, __LINE__, *pValue );
#endif // USE_POD_IPC

	return ret;
}

int IPC_CLIENT_SNMPGetVideoContentInfo(int iContent, struct VideoContentInfo * pInfo)
{
	rmf_Error ret = RMF_SUCCESS;

#ifdef USE_POD_IPC
        int32_t result_recv = 0;
        int8_t res = 0;
	int mmiPages = 0 ;

	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) QAM_SRC_SERVER_CMD_GET_VIDEO_CONTENT_INFO);

	result_recv |= ipcBuf.addData( ( const void * ) &iContent, sizeof( iContent ) );
    if( result_recv < 0)
    {
            RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
                    "\nPOD Comm add data failed in %s:%d: %d\n",
                             __FUNCTION__, __LINE__ , result_recv);
            return RMF_OSAL_ENODATA;
    }

	res = ipcBuf.sendCmd( qam_src_server_cmd_port_no  );
    if ( 0 != res )
    {
        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
            "POD Comm failure in %s:%d: ret = %d\n",
                __FUNCTION__, __LINE__, res );
        return RMF_OSAL_ENODATA;
    }

    result_recv = ipcBuf.getResponse( &ret );
    if( result_recv < 0)
    {
        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
            "POD Comm get response failed in %s:%d ret = %d\n",
                __FUNCTION__, __LINE__, result_recv );
        return RMF_OSAL_ENODATA;

    }
	result_recv = ipcBuf.collectData( ( void * ) pInfo,
                            sizeof( *pInfo));
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.SNMP", "%s:%d: ContentIndex=%u, TunerIndex=%u, ProgramNumber=%u,\n", __FUNCTION__, __LINE__, pInfo->ContentIndex, pInfo->TunerIndex, pInfo->ProgramNumber);
#endif // USE_POD_IPC

	return ret;
}
