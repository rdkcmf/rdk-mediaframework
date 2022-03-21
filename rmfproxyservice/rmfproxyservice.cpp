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

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <jansson.h>

#include "rmf_osal_mem.h"
#include "rdk_debug.h"
#include "rmf_error.h"
#include "rmf_osal_ipc.h"
#include "rmf_osal_util.h"
#include "rmfproxyservice.h"
#include "rmfcore.h"
#include "T2PClient.h"
#ifdef TEST_WITH_PODMGR
#include "podServer.h"
#endif
//Macros for IPC communication with POD Server
static volatile uint32_t pod_server_cmd_port_no;
static volatile uint32_t canh_server_port_no;

#ifndef TEST_WITH_PODMGR

#define POD_SERVER_CMD_IS_READY 5
#define POD_SERVER_CMD_GET_SYSTEM_IDS 16
#define POD_SERVER_CMD_DFLT_PORT_NO 50506
#endif

#define CANH_SERVER_DFLT_PORT_NO 50508

//Macros for IPC communication with CANH Server
#define CANH_COMMAND_GET_PLANTID 12
#define COMMAND_GET_CANH_STATUS 7
#define CANH_MAX_PLANTID_LEN 64

//Macros for IPC communication with VOD-Client
#define VOD_CLIENT_IP "127.0.0.1"
#define VOD_CLIENT_PORT 11000
#define VOD_SERVER_ID "VOD_SERVER_ID"
#define DEVICE_PLANT_ID "DEVICE_PLANT_ID"
#define CANH_STATUS_READY 33


/**
 * @addtogroup rmfproxyserviceapi
 * @{
 * @fn rmf_Error rmf_podmgrIsReady(void)
 * @brief This function is used to check whether the POD manager is ready or not.
 *
 * It reads the configuration value for "POD_SERVER_CMD_PORT_NO" for getting the POD server port number and
 * "CANH_SERVER_PORT_NO" for getting the CANH server port number. IPC mechanism is used to send command
 * "POD_SERVER_CMD_IS_READY" to POD server to check whether the POD server is active.
 *
 * @return Returns the response command if case of communication success with CANH Server,
 * else returns an error value described in return value
 * @retval RMF_OSAL_ENOTIMPLEMENTED Returns failure if proxy service is not enabled for this setup.
 * @retval RMF_OSAL_ENODATA Unable to communicate with POD server.
 * @}
 */
rmf_Error rmf_podmgrIsReady(void)
{

	const char *server_port;
	const char *canh_port;

	server_port = rmf_osal_envGet( "POD_SERVER_CMD_PORT_NO" );
	if( NULL == server_port)
	{
		RDK_LOG ( RDK_LOG_ERROR, "LOG.RDK.PXYSRVC", "%s: NULL POINTER  received for name string\n",__FUNCTION__ );
		pod_server_cmd_port_no = POD_SERVER_CMD_DFLT_PORT_NO;
	}
	else
	{
		pod_server_cmd_port_no = atoi( server_port );
	}
	
	canh_port = rmf_osal_envGet( "CANH_SERVER_PORT_NO" );
	if( NULL == canh_port )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.PXYSRVC", "%s: NULL POINTER  received for name string\n", __FUNCTION__ );
		canh_server_port_no = CANH_SERVER_DFLT_PORT_NO;
	}
	else
	{
		canh_server_port_no = atoi( canh_port );
	}
	
#ifndef USE_PXYSRVC
        return RMF_OSAL_ENOTIMPLEMENTED;
#else
	rmf_Error ret = RMF_OSAL_ENODATA;

        IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_IS_READY);
        int32_t result_recv = 0;

        int8_t res = ipcBuf.sendCmd( pod_server_cmd_port_no );
        if ( 0 != res )
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.PXYSRVC",
                        "%s: Lost connection with POD. res = %d\n",
                                 __FUNCTION__, res );
                return RMF_OSAL_ENODATA;
        }

        result_recv = ipcBuf.getResponse( &ret );
        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.PXYSRVC",
                        "%s: No response from POD. result_recv = %d\n",
                                 __FUNCTION__, result_recv );
                return RMF_OSAL_ENODATA;
        }

        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.PXYSRVC",
                "%s: POD Manager ready status is = %d\n",
                         __FUNCTION__, ret );

	return ret;
#endif
}

bool IsAtOffPlant( )
{
	static bool inited = false;
	static bool AtOffPlant = false;
	const char *offPlantEnabled = NULL;
	
	if( inited )
	{
		return AtOffPlant;
	}
	
	if ( ( offPlantEnabled = (char*)rmf_osal_envGet( "ENABLE.OFFPLANT.SETUP") ) != NULL ) 
	{
		if ( 0 == strcasecmp( offPlantEnabled, "TRUE" ) )
		{	/* setting some values for offplant */
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
					"%s - The box is at off plant \n",
					__FUNCTION__ );
			AtOffPlant = true;
		}
	}

	inited = true;
	return AtOffPlant;
}


/**
 * @addtogroup rmfproxyserviceapi
 * @{
 * @fn rmf_Error getsystemids(uint16_t *pChannelMapID, uint16_t *pControllerID)
 * @brief This function is used to extract the channel map Id and controller Id from OOB SI.
 *
 * If in case of "ENABLE.OFFPLANT.SETUP" is enabled for testing, the function will read
 * two more entries from the configuration i.e. "OFFPLANT.SETUP.CHANNELMAPID" for channel map Id
 * and "OFFPLANT.SETUP.DACID" for control Id.
 *
 * In case of "ENABLE.OFFPLANT.SETUP" is disabled when it is live, this function will wait till
 * rmf_podmgrIsReady() returns True and send a command "POD_SERVER_CMD_GET_SYSTEM_IDS"
 * to POD Server application to get the channel map Id and control Id by using inter process communication.
 * If Service Information (SI) is not up, wait for 10 seconds and retries till SI become up.
 *
 * @param[out] pChannelMapID Channel map Id extracted from OOB SI
 * @param[out] pControllerID Controller Id extracted from OOB SI
 *
 * @return rmf_Error defined in rmf_osal_error.h
 * @retval RMF_SUCCESS This is updated the specified channel map Id and controller Id.
 * @retval RMF_OSAL_ENOTIMPLEMENTED Returns failure if proxy service is not enabled for this setup.
 * @retval RMF_OSAL_ENODATA Unable to communicate with CANH server, unable to get response from CANH server,
 * or the function found that the response message received from CANH server is invalid.
 * @}
 */
rmf_Error getsystemids(uint16_t *pChannelMapID, uint16_t *pControllerID)
{
       struct stat sb;

#ifndef USE_PXYSRVC
        return RMF_OSAL_ENOTIMPLEMENTED;
#else
	rmf_Error ret = RMF_SUCCESS;
        int32_t result_recv = 0;
        int8_t res = 0;

	if( IsAtOffPlant( ) )
	{
		int32_t iChannelMapId = 1902;
		int32_t iDACId = 3414;

		const char *cChannelMapId = (char*)rmf_osal_envGet( "OFFPLANT.SETUP.CHANNELMAPID" );
		const char *cDACId = (char*)rmf_osal_envGet( "OFFPLANT.SETUP.DACID" );

		if ( NULL != cChannelMapId )
		{
			iChannelMapId = atoi( cChannelMapId );
		}
		if ( NULL != cDACId )
		{
			iDACId = atoi( cDACId );
		}
			
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
				"%s - Setting fake values for off plant setup, DACId = %d, ChannelMapId = %d \n",
				__FUNCTION__, iDACId, iChannelMapId );
		*pControllerID = iDACId;
		*pChannelMapID = iChannelMapId;
		return RMF_SUCCESS;
	}

	RDK_LOG (RDK_LOG_INFO, "LOG.RDK.PXYSRVC", "%s():%d: Waiting till POD Manager is fully up\n", __FUNCTION__, __LINE__);
	while(rmf_podmgrIsReady() != RMF_SUCCESS) {
		sleep(60);
	}
	
	RDK_LOG (RDK_LOG_INFO, "LOG.RDK.PXYSRVC", "%s():%d: POD Manager is fully up. Gettomg System IDs from POD\n", __FUNCTION__, __LINE__);
	while( 1 )
	{		
		if ( stat("/tmp/si_acquired", &sb ) == -1)
		{
			sleep( 60 );
			continue;
		}
		
		IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_SYSTEM_IDS);
		res = ipcBuf.sendCmd( pod_server_cmd_port_no );

		if ( 0 != res )
		{
		        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.PXYSRVC",
		                "POD Comm failure in %s:%d: ret = %d\n",
		                         __FUNCTION__, __LINE__, res );
		        return RMF_OSAL_ENODATA;
		}
		result_recv = ipcBuf.getResponse( &ret );
		if( result_recv < 0)
		{
		        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.PXYSRVC",
		                "POD Comm get response failed in %s:%d ret = %d\n",
		                         __FUNCTION__, __LINE__, result_recv );
		        return RMF_OSAL_ENODATA;
		}

		if( ret != RMF_SUCCESS )
		{
		        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.PXYSRVC",
		                "OOB returning error in %s:%d ret = %d\n",
		                         __FUNCTION__, __LINE__, ret);
			if( RMF_OSAL_ENODATA == ret )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.PXYSRVC",
					"SI is not up, so retrying %s:%d \n",
						__FUNCTION__, __LINE__);				
				sleep( 10 );
				continue;
			}
		       ret = RMF_OSAL_ENODATA;
			break;
		}

		result_recv |= ipcBuf.collectData( (void *)pChannelMapID, sizeof(uint16_t) );
		result_recv |= ipcBuf.collectData( (void *)pControllerID, sizeof(uint16_t) );

		if( result_recv < 0)
		{
		        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.PXYSRVC",
		                "IPCBuf collect data failed in %s:%d: %d\n",
		                         __FUNCTION__, __LINE__ , result_recv);
		        return RMF_OSAL_ENODATA;
		}

		RDK_LOG (RDK_LOG_INFO, "LOG.RDK.PXYSRVC", "%s():%d: Channel Map ID: %d\n", __FUNCTION__, __LINE__,*pChannelMapID);
		RDK_LOG (RDK_LOG_INFO, "LOG.RDK.PXYSRVC", "%s():%d: Controller ID: %d\n", __FUNCTION__, __LINE__, *pControllerID);

		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.PXYSRVC",
		        "%s : Done\n",
		                 __FUNCTION__);
		break;
	}
	return ret;
#endif
}

RMFResult processProxyRequest(const char *fieldString, uint32_t *pValue)
{
#ifndef USE_PXYSRVC
        return RMF_OSAL_ENOTIMPLEMENTED;
#else

        const int msgBufSz = 1024;
        char t2p_resp_str[msgBufSz] = {0};
        char t2p_req_str[msgBufSz]= {0};
        char fieldValue[msgBufSz]= {0};
        int retVal = RMF_RESULT_INTERNAL_ERROR;

        if(!fieldString || !pValue)
                return retVal;

/*Request:       
{
"rmfProxyRequest" : {
"fieldId" : fieldId,
}
}
*/

         // Invoke vod client  to get requested field

        snprintf(t2p_req_str, sizeof (t2p_req_str), "{ \"rmfProxyRequest\" : { \"fieldid\" : \"%s\"}} }\n", fieldString);

        RDK_LOG(RDK_LOG_TRACE1,"LOG.RDK.PXYSRVC","processProxyRequest()::%s length %d\n",t2p_req_str,strlen(t2p_req_str));
        T2PClient *t2pclient = new T2PClient();
        if (t2pclient->openConn((char*)VOD_CLIENT_IP, VOD_CLIENT_PORT) == 0)
        {
                int recvdMsgSz = 0;
                t2p_resp_str[0] = '\0';
                if ((recvdMsgSz = t2pclient->sendMsgSync(t2p_req_str, strlen(t2p_req_str), t2p_resp_str, msgBufSz)) > 0)
                {
                        t2p_resp_str[recvdMsgSz] = '\0';
                        RDK_LOG(RDK_LOG_TRACE1,"LOG.RDK.PXYSRVC","processProxyRequest(): Response : %s\n", t2p_resp_str);

                        /*
                        Response:
                        {
                        "vodPlayResponse" : {
                        <ResponseStatus>,
                        "<fieldId>": <fieldId value>
                        }
                        }

                        */

                        json_t *msg_root;
                        json_error_t json_error;
                        json_t *Response;
                        json_t *status;
                        json_t *value;


                        msg_root = json_loads(t2p_resp_str, 0, &json_error);
                        if (!msg_root)
                        {
                                 RDK_LOG(RDK_LOG_INFO,"LOG.RDK.PXYSRVC","json_error: on line %d: %s\n", json_error.line, json_error.text);
                        }
                        else
                        {
                                Response = json_object_get(msg_root, "TriggerRMFProxyResponse");
                                if (!Response)
                                {
                                         RDK_LOG(RDK_LOG_INFO,"LOG.RDK.PXYSRVC","json_error: Response object not found\n");
                                }
                                else
                                {
                                        status = json_object_get(Response, "ResponseStatus");
                                        if (!status)
                                        {
                                                 RDK_LOG(RDK_LOG_INFO,"LOG.RDK.PXYSRVC","json_error: status object not found\n");
                                        }
                                        else
                                        {
                                                RDK_LOG(RDK_LOG_INFO,"LOG.RDK.PXYSRVC","json_log: Status = %d\n", json_integer_value(status));
                                                if( json_integer_value(status) == 0)
                                                {
                                                	RDK_LOG(RDK_LOG_INFO,"LOG.RDK.PXYSRVC","json_log: looking for json_object: %s\n", fieldString);
                                                        value = json_object_get(Response, fieldString);
                                                        if (!value)
                                                        {
                                                                 RDK_LOG(RDK_LOG_INFO,"LOG.RDK.PXYSRVC","json_error: %s object not found\n", fieldString);
                                                        }
                                                        else
                                                        {
                                                                strncpy(fieldValue , json_string_value(value), sizeof (fieldValue));
                                                                RDK_LOG(RDK_LOG_INFO,"LOG.RDK.PXYSRVC","json_log: fieldValue:%s\n",fieldValue);
                                                                sscanf(fieldValue,"%d",pValue);
                                                                retVal = RMF_RESULT_SUCCESS;
                                                        }
                                                }
                                        }
                                }
                                // Release resources
                                json_decref(msg_root);
                        }
                }
                t2pclient->closeConn();
                delete t2pclient;
        }

        return retVal;
#endif
}

/**
 * @addtogroup rmfproxyserviceapi
 * @{
 * @fn rmf_Error getvodserverid(uint32_t *pVodServerID)
 * @brief This function is used to extract the VOD server Id through auto discovery.
 *
 * In case of Off Plant is enabled for testing, the function will get the VOD server Id from configuration
 * "OFFPLANT.SETUP.VODSERVERID". else the function will make a communication with VOD client
 * (by sending a request message as "VOD_SERVER_ID") to get the VOD server Id.
 *
 * @param[out] pVodServerID VOD server Id need to be stored
 *
 * @return rmf_Error defined in rmf_osal_error.h
 *
 * @retval RMF_SUCCESS This has got the VOD server Id.
 * @retval RMF_OSAL_ENOTIMPLEMENTED If proxy service is not defined for this setup.
 */
rmf_Error getvodserverid(uint32_t *pVodServerID)
{
#ifndef USE_PXYSRVC
        return RMF_OSAL_ENOTIMPLEMENTED;
#else
	struct stat sb;

	if( IsAtOffPlant( ) )
	{
		int32_t iVodServerId = 61003;
		const char *cVodServerId = (char*)rmf_osal_envGet( "OFFPLANT.SETUP.VODSERVERID" );
		
		if ( NULL != cVodServerId )
		{
			iVodServerId = atoi( cVodServerId );
		}

		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
				"%s - Setting fake values for off plant setup VodServerID = %d \n",
				__FUNCTION__, iVodServerId );		
				
		*pVodServerID = iVodServerId;
		return RMF_SUCCESS;
	}

	RDK_LOG (RDK_LOG_INFO, "LOG.RDK.PXYSRVC", "%s():%d: Waiting for Auto Discovery to fetch vodserverid\n", __FUNCTION__, __LINE__);
	while(stat("/tmp/vodid_acquired", &sb) == -1) {
		//perror("stat");
		sleep(60);
	}

	RDK_LOG (RDK_LOG_INFO, "LOG.RDK.PXYSRVC", "%s():%d: Auto Discovery fetched vodserverid\n", __FUNCTION__, __LINE__);
	if(RMF_RESULT_SUCCESS == processProxyRequest(VOD_SERVER_ID, pVodServerID))
		RDK_LOG(RDK_LOG_INFO,"LOG.RDK.PXYSRVC","%s():%d: VOD Server ID: %d\n", __FUNCTION__, __LINE__, *pVodServerID);
	else
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.PXYSRVC", "%s():%d: VOD Server ID not available\n", __FUNCTION__, __LINE__);

	RDK_LOG (RDK_LOG_INFO, "LOG.RDK.PXYSRVC", "%s():%d: Waiting for vodserverid to be fetched by vod-client\n", __FUNCTION__, __LINE__);

	return RMF_SUCCESS;
#endif
}

/**
 * @fn bool canhIsReady(void)
 * @brief This function is used to check whether the Conditional Access Network Handler (CANH) Server is ready.
 *
 * CANH server port number is updated from the configuration value "CANH_SERVER_PORT_NO" in the function
 * rmf_podmgrIsReady(). IPC mechanism is used to send a command "COMMAND_GET_CANH_STATUS" to CANH server
 * to check whether the CANH server is started. If CANH server is not yet started, the function will poll
 * continuously in every 60 seconds until CANH server is started.
 *
 * @return Returns True if proxy service is disabled for this setup, else returns False.
 */
bool canhIsReady(void)
{
#ifndef USE_PXYSRVC
        return false;
#else
	rmf_Error ret = RMF_OSAL_ENODATA;
	bool canh_started = false;
	int count = 0;
	int32_t result_recv = 0;
	uint32_t response = 0;

	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) COMMAND_GET_CANH_STATUS );

	while(!canh_started)
	{
		int8_t res = ipcBuf.sendCmd( canh_server_port_no );
		if ( 0 == res )
		{
			result_recv = ipcBuf.getResponse( &response );
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.PXYSRVC", "%s: CANH status is [%x]\n",
					__FUNCTION__, __LINE__, response );

			if ( CANH_STATUS_READY == response )
			{
				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.PXYSRVC",
						"CANH started successfully.In ready state\n" );
				canh_started = true;
				break;
			}
		}

		sleep(60);
		count += 60;
	}

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.PXYSRVC",
			"CANH started successfully after %d secs\n" , count);

	return canh_started;
#endif
}

/**
 * @fn rmf_Error getplantid(uint64_t* pPlantID)
 * @brief This function is used to get the plant Id.
 *
 * In case of OFF Plant is enabled for testing the function will read the Plant Id from the configuration
 * "OFFPLANT.SETUP.PLANTID" and returns from there, otherwise the function will get the plant Id from
 * CANH server by sending a request message "CANH_COMMAND_GET_PLANTID". This uses the inter process communication
 * with CANH server for receiving the plant Id from CANH Server.
 *
 * @param[out] pPlantID Plant Id for this setup which need to be stored
 *
 * @return rmf_Error defined in rmf_osal_error.h
 *
 * @retval RMF_OSAL_ENOTIMPLEMENTED Proxy service is not defined for this setup.
 * @retval RMF_OSAL_ENODATA Following reasons for failure such as CANH server application is not ready,
 * unable to send command to CANH server, unable to receive response from CANH server or invalid
 * plant Id received from CANH server.
 */
rmf_Error getplantid(uint64_t *pPlantID)
{
#ifndef USE_PXYSRVC
        return RMF_OSAL_ENOTIMPLEMENTED;
#else
        uint32_t response = 0;
        int32_t result_recv;
        uint8_t size= 0;
	uint8_t *psPlantId = NULL;

		if( IsAtOffPlant( ) )
		{
			const char *cPlantId= (char*)rmf_osal_envGet( "OFFPLANT.SETUP.PLANTID" );
			int32_t iPlantId = 61003;
			
			if ( NULL != cPlantId )
			{
				iPlantId = atoi( cPlantId );
			}
		
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
				"%s - Setting fake values for off plant setup, plant id = %d \n",
				__FUNCTION__, iPlantId );
			/* Assigning some random value */
			*pPlantID = iPlantId ;
			return RMF_SUCCESS;
		}

        if ( !canhIsReady() )
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.PXYSRVC",
                                 "Error!!Canh not started yet." );
                return RMF_OSAL_ENODATA;
        }

        IPCBuf ipcBuf = IPCBuf( ( uint32_t ) CANH_COMMAND_GET_PLANTID );

        int8_t res = ipcBuf.sendCmd( canh_server_port_no );
        if ( 0 != res )
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.PXYSRVC",
                                 "Error!!sendCmd failed %s:%d", __FUNCTION__, __LINE__ );
                return RMF_OSAL_ENODATA;
        }

        result_recv = ipcBuf.getResponse( &response );
        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.PXYSRVC",
                                 "Error!!getResponse failed %s:%d", __FUNCTION__, __LINE__ );
                return RMF_OSAL_ENODATA;
        }

        result_recv |=
                ipcBuf.collectData( &size, sizeof( size ) );

        if( size >= CANH_MAX_PLANTID_LEN )
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.PXYSRVC",
                         "Error!! size (%d) >= CANH_MAX_PLANTID_LEN (%d) %s:%d" , size, CANH_MAX_PLANTID_LEN, __FUNCTION__, __LINE__ );
                return RMF_OSAL_ENODATA;
        }

	if(RMF_SUCCESS != (rmf_osal_memAllocP(RMF_OSAL_MEM_IPC, (size+1)*sizeof(uint8_t), (void**)&psPlantId)))
	{
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.PXYSRVC",
                                 "Error!! malloc failed %s:%d", __FUNCTION__, __LINE__ );
                return RMF_OSAL_ENODATA;
	}
 
        result_recv |=
                ipcBuf.collectData( psPlantId, size );
        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.PXYSRVC",
                                 "Error!!collectData failed %s:%d", __FUNCTION__, __LINE__ );
                return RMF_OSAL_ENODATA;
        }

        psPlantId[ size ] = 0;
	RDK_LOG(RDK_LOG_INFO,"LOG.RDK.PXYSRVC","%s():%d: Plant ID String from CANH: %s\n", __FUNCTION__, __LINE__, psPlantId);

	*pPlantID = strtoul((const char*)psPlantId, NULL, 10);
	RDK_LOG(RDK_LOG_INFO,"LOG.RDK.PXYSRVC","%s():%d: Plant ID : %llu\n", __FUNCTION__, __LINE__, *pPlantID);

	rmf_osal_memFreeP(RMF_OSAL_MEM_IPC, psPlantId);
	psPlantId = NULL;
	return RMF_SUCCESS;
#endif
}

/**
 * @fn void getDiagnostics()
 * @brief This function is used to extract the diagnostics information from the JSON diagnostics configuration file "RMFPXYSRV.JSONFILE.LOCATION".
 *
 * This will check for any update available for channel map Id, VOD server Id or plant Id by querying with the CANH Server.
 * If in case of any update found for these Id's, the function will update these information to the JSON configuration file.
 *
 * @return None
 * @addtogroup rmfproxyserviceapi
 * @}
 */
void getDiagnostics()
{
#ifdef USE_PXYSRVC
	uint16_t channelMapId=0, controllerId=0;
	uint32_t vodServerId=0;
	uint64_t plantId=0;
        const int msgBufSz = 1024;
        char json_diag_str[msgBufSz];
	FILE *fp;
	//Added to handle the change in location for json file for various platforms
	const char  *json_diag_location = NULL;

	json_diag_location = rmf_osal_envGet("RMFPXYSRV.JSONFILE.LOCATION");
	if ( (json_diag_location == NULL))
	{
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI","%s():%d: Diagnostics.json location is not specified. Hence using the default location...\n", __FUNCTION__, __LINE__);
		json_diag_location = "/tmp/mnt/diska3/persistent/usr/1112/703e/diagnostics.json";
	}

	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.PXYSRVC", "%s():%d: Diagnostis.json Location: %s\n",__FUNCTION__, __LINE__, json_diag_location);
        
	RDK_LOG (RDK_LOG_INFO, "LOG.RDK.PXYSRVC", "%s():%d: Calling getsystemids\n", __FUNCTION__, __LINE__);
	getsystemids(&channelMapId, &controllerId);
	snprintf(json_diag_str, sizeof (json_diag_str),  "{ \"systemIds\": { \"channelMapId\": %d, \"controllerId\": %d, \"plantId\": %llu, \"vodServerId\": %d}} \n", channelMapId, controllerId, plantId, vodServerId);
	RDK_LOG (RDK_LOG_INFO, "LOG.RDK.PXYSRVC", "%s():%d: json_diag_str: %s\n", __FUNCTION__, __LINE__, json_diag_str);

        //Added to handle the change in location for json file for various platforms

	
	fp = fopen(json_diag_location, "w");
	if(fp != NULL)
	{
		fprintf(fp,"%s",json_diag_str);
		fclose(fp);
	}

#ifdef USE_VODSRC  //Skip acquiring vodeserverid/plantid if VODSRC is disabled.
	RDK_LOG (RDK_LOG_INFO, "LOG.RDK.PXYSRVC", "%s():%d: Calling getvodserverid\n", __FUNCTION__, __LINE__);
	getvodserverid(&vodServerId);
	RDK_LOG (RDK_LOG_INFO, "LOG.RDK.PXYSRVC", "%s():%d: Calling getplantid\n", __FUNCTION__, __LINE__);
	getplantid(&plantId);
#endif

	RDK_LOG (RDK_LOG_INFO, "LOG.RDK.PXYSRVC", "%s():%d: Channel Map ID: %d\n", __FUNCTION__, __LINE__, channelMapId);
	RDK_LOG (RDK_LOG_INFO, "LOG.RDK.PXYSRVC", "%s():%d: Controller ID: %d\n", __FUNCTION__, __LINE__, controllerId);		
	RDK_LOG (RDK_LOG_INFO, "LOG.RDK.PXYSRVC", "%s():%d: VOD Server ID: %d\n", __FUNCTION__, __LINE__, vodServerId);
	RDK_LOG (RDK_LOG_INFO, "LOG.RDK.PXYSRVC", "%s():%d: 	 %llu\n", __FUNCTION__, __LINE__, plantId);

	snprintf(json_diag_str, sizeof (json_diag_str), "{ \"systemIds\": { \"channelMapId\": %d, \"controllerId\": %d, \"plantId\": %llu, \"vodServerId\": %d}} \n", channelMapId, controllerId, plantId, vodServerId);
	
	RDK_LOG (RDK_LOG_INFO, "LOG.RDK.PXYSRVC", "%s():%d: json_diag_str: %s\n", __FUNCTION__, __LINE__, json_diag_str);

        //Added to handle the change in location for json file for various platforms

	json_diag_location = rmf_osal_envGet("RMFPXYSRV.JSONFILE.LOCATION");
	if ( (json_diag_location == NULL))
	{
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI","%s():%d: Diagnostics.json location is not specified. Hence using the default location...\n", __FUNCTION__, __LINE__);
		json_diag_location = "/tmp/mnt/diska3/persistent/usr/1112/703e/diagnostics.json";
	}

	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.PXYSRVC", "%s():%d: Diagnostis.json Location: %s\n",__FUNCTION__, __LINE__, json_diag_location);
	fp = fopen(json_diag_location, "w");
	if(fp != NULL)
	{
		fprintf(fp,"%s",json_diag_str);
		fclose(fp);
	}
#endif	
}
