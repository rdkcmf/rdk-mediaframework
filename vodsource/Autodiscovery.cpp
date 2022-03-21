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

/**
 * @file Autodiscovery.cpp
 * @brief Auto Discovery
 */

#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "Autodiscovery.h"
#include "T2PClient.h"
#include "jansson.h"
#include "rdk_debug.h"
#include "rmfqamsrc.h"
#include "podServer.h"
#include "rmf_osal_ipc.h"
#include "rmf_osal_util.h"
#include "rmfcore.h"
#include "canhClient.h"
#include "rmf_error.h"
#include "rmf_osal_error.h"
#include "rmf_osal_mem.h"
#include "sys/stat.h"
#ifdef TEST_WITH_PODMGR
#include "rmf_pod.h"
#endif

#ifdef GLI
#include "libIBus.h"
#include "libIBusDaemon.h"
#include "pwrMgr.h"
#include "irMgr.h"
#include "sysMgr.h"
#endif

#define AUTODISCOVERY_IP	"127.0.0.1"
#define AUTO_DISCOVERY_DFLT_PORT_NO 50001
#define AUTODISCOVERY_TUNER_TOKEN "autodiscovery_dummy_token"
#define CANH_CMD_GET_PLANTID	12
#define CANH_MAX_PLANTID_LEN 	64
#define SNDBUFSIZE		2048

#define VOD_CLIENT_IP		AUTODISCOVERY_IP
#define VOD_CLIENT_PORT		11000
static volatile uint32_t pod_server_cmd_port_no=0;
static volatile uint32_t canh_server_port_no=0;
static int cancelAutoDiscovery( const std::string reservationToken, const std::string sourceId );

extern void eventNotifier( void* implObj, uint32_t e );

AutoDiscovery *AutoDiscovery::m_adInstance = NULL;
#ifdef GLI
struct powerMode {
	IARM_Bus_PowerState_t pMode;
};
#endif

/**
 * @fn AutoDiscovery* AutoDiscovery::getInstance()
 * @brief This function is used to read the environment settings such as POD and CANH port number
 * from the configuration "POD_SERVER_CMD_PORT_NO" and "CANH_SERVER_PORT_NO" and create an instance for Auto discovery.
 * If in case the function did not find the port number from the configuration, this will set POD manager default port
 * number to 50506 and CANH default port number to 54321.
 *
 * @return Returns an instance for AutoDiscovery.
 */
AutoDiscovery* AutoDiscovery::getInstance()
{
	const char *server_port;
	const char *canh_port;
	
	server_port = rmf_osal_envGet( "POD_SERVER_CMD_PORT_NO" );
	if( NULL == server_port )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.VODSRC", "%s: NULL POINTER  received for name string\n", __FUNCTION__ );
		pod_server_cmd_port_no = POD_SERVER_CMD_DFLT_PORT_NO;
	}
	else
	{
		pod_server_cmd_port_no = atoi( server_port );
	}

	canh_port = rmf_osal_envGet( "CANH_SERVER_PORT_NO" );
	if( NULL == canh_port )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.VODSRC", "%s: NULL POINTER  received for name string\n",__FUNCTION__ );
		canh_server_port_no = CANH_SERVER_DFLT_PORT_NO;
	}
	else
	{
		canh_server_port_no = atoi( canh_port );
	}
	
	if ( m_adInstance == NULL )
	{
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.MS", "Create Auto Discovery Instance\n" );
		m_adInstance = new AutoDiscovery();
		RMFQAMSrc::registerQamsrcReleaseCallback(RMFQAMSrc::USAGE_AD, cancelAutoDiscovery );
	}
	return m_adInstance;
}

/**
 * @addtogroup vodsourceadapi
 * @{
 * @fn AutoDiscovery::AutoDiscovery()
 * @brief A default constructor for initializing the member variable of AutoDiscovery class.
 *
 * @return None
 * @}
 */
AutoDiscovery::AutoDiscovery()
{
	m_servSock = -1;
	m_done = false;
	m_adTriggered = false;
	m_adCompleted = false;
	m_adFailed    = false;
	m_adNotificationQueue = 0;
	m_adTuningInprogress = false;

	if ( RMF_SUCCESS != rmf_osal_eventqueue_create(
		(const uint8_t*)"AdNotificationQ", &m_adNotificationQueue) )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC",
			"rmf_osal_eventqueue_create(m_adNotificationQueue) failed\n" );
	}
}

/**
 * @fn AutoDiscovery::~AutoDiscovery()
 * @brief A default destructor for AutoDiscovery Class is used to clear the content of Notification
 * event queue and delete the eventqueue.
 *
 * @return None
 */
AutoDiscovery::~AutoDiscovery()
{
	rmf_osal_eventqueue_clear( m_adNotificationQueue );
	rmf_osal_eventqueue_delete( m_adNotificationQueue );
	m_adNotificationQueue = 0;
}

/**
 * @fn void threadWrapper( void *ad )
 * @brief A light weight task invoked with a default priority for handling the auto discovery
 * process by communicating with VOD client.
 *
 * @param[in] ad Thread argument
 *
 * @return None
 */
void threadWrapper( void *ad )
{
	((AutoDiscovery*)ad)->adHandler( NULL );
}

#ifdef GLI
/**
 * @fn void powerMonitorThread( void *data )
 * @brief Power monitor task to inform the VOD client about their power change state such as
 * Standby or Non Standby. The current power state updated by IARMBus depending upon the
 * application are used on it. Depending upon the current power state, this function will call
 * the appropreate message handler functions such as low power mode or full power mode handler.
 *
 * @param[in] data thread parameter for powerMode context.
 *
 * @return None
 */
void powerMonitorThread( void *data )
{
  
  struct powerMode *pContext = (powerMode *)data;
  
  if(pContext)
  {
      switch(pContext->pMode)
      {
	  case IARM_BUS_PWRMGR_POWERSTATE_STANDBY:
		  AutoDiscovery::getInstance()->handleLowPower();
		  break;
	  case IARM_BUS_PWRMGR_POWERSTATE_ON:
		  AutoDiscovery::getInstance()->handleFullPower();
		  break;
	  default:
		  break;
    }
    free(pContext);
  }
}

void partnerIdMonitorThread( void *data )
{
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC", "partnerID change detected\n" );
	AutoDiscovery::getInstance()->handlePartnerIdChange();
}

void plantIdEventProcessingThread(void *data)
{
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC",
				"Processing IARM_BUS_SYSMGR_SYSSTATE_PLANT_ID event\n" );
	bool waitForSTT = true;
	struct stat stt_file;
	int count =0;
	//Wait until STT is received
	do
	{
		waitForSTT = ((stat("/tmp/stt_received", &stt_file) != 0));
		if(!count || !(count %5) || !waitForSTT)
		RDK_LOG( RDK_LOG_INFO,"LOG.RDK.VODSRC","%s:%s\n",__FUNCTION__,(waitForSTT) ? "Waiting for STT":"STT is already processed.");

		if(!waitForSTT)
		{
		   break;
		}
		sleep(1);
		count++;

	}
	while( waitForSTT );

	while ( RMF_SUCCESS != AutoDiscovery::getInstance()->triggerAutoDiscovery() )
	{
		sleep( 2 );
	}
}
#endif

/**
 * @addtogroup vodsourceadapi
 * @{
 * @fn RMFResult AutoDiscovery::start()
 * @brief This function is used to register an RPC method in IARMBUS for monitoring
 * power mode state such as STANDBY and NO_STANDBY.
 *
 * This will read the auto discovery port number(default port no 50001) from the environment "AUTO_DISCOVERY_PORT_NO" and
 * creates a TCP socket, bind a name to the socket and listen for new connections. It creates a
 * handler for auto discovery. It will also register the application (VOD Source) with IARM Bus and connect to it.
 *
 * @return Returns 0 on success, -1 otherwise.
 * @}
 */
RMFResult AutoDiscovery::start()
{
	rmf_Error ret = RMF_RESULT_SUCCESS;
	struct sockaddr_in adServAddr;
	int bindRetryCount = 0;

	RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","[%s:%d] Starting...\n", __FUNCTION__, __LINE__ );

#ifdef GLI
	IARM_Bus_Init("VODSource");
	IARM_Bus_Connect();
	IARM_Bus_RegisterCall(IARM_BUS_COMMON_API_PowerPreChange, _PowerPreChange);
#endif
	const char *auto_port;
	int32_t auto_discovery_port_no= 0;
	auto_port = rmf_osal_envGet( "AUTO_DISCOVERY_PORT_NO" );
	if( NULL == auto_port )
	{
	       RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.VODSRC", "%s: NULL POINTER  received for name string\n", __FUNCTION__ );
		auto_discovery_port_no = AUTO_DISCOVERY_DFLT_PORT_NO;
	}
	else
	{
		auto_discovery_port_no = atoi( auto_port );
	}

	if ((m_servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC", "socket() failed\n" );
		return -1;
	}

	memset(&adServAddr, 0, sizeof(adServAddr));
	adServAddr.sin_family = AF_INET;
	adServAddr.sin_addr.s_addr = inet_addr("127.0.0.1");//htonl(INADDR_ANY);
	adServAddr.sin_port = htons( (uint16_t) auto_discovery_port_no );

	int one1 = 1;
	if ( setsockopt(m_servSock, SOL_SOCKET, SO_REUSEADDR, &one1, sizeof(one1)) < 0 )
	{
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC", "setsockopt failed\n" );
		close( m_servSock );
		return -1;
	}

	do
	{
		int status = bind(m_servSock, (struct sockaddr *)&adServAddr, sizeof(adServAddr));
		if (status == -1)
		{
			RDK_LOG( RDK_LOG_WARN, "LOG.RDK.VODSRC","bind error %s\n", strerror(errno));
			sleep(2);
			++bindRetryCount;
		}
		else
		{
			break;
		}
	} while (bindRetryCount < 30);
    if( ( bindRetryCount >= 30 ) )
	{
		close( m_servSock );
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.VODSRC", "Rebooting the system due to binderror in AutoDiscovery ..." );
		rmf_osal_threadSleep( 5000, 0 );
		system( "sh /rebootNow.sh -s AutoDiscoveryRecovery -o 'Rebooting the box due to binderror in AutoDiscovery...'" );
	}
	if (listen( m_servSock, 4 ) < 0)
	{
		RDK_LOG( RDK_LOG_INFO,"LOG.RDK.VODSRC","listen() failed\n" );
		close( m_servSock );
		return -1;
	}

	ret = rmf_osal_threadCreate( threadWrapper, this, RMF_OSAL_THREAD_PRIOR_DFLT, 
					RMF_OSAL_THREAD_STACK_SIZE, &m_tid,"AutoDiscoveryHandler" );

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC", "[%s:%d] adHandler created successfully\n", __FUNCTION__, __LINE__ );
	return 0;
}

/**
 * @fn void hexDump( char *buf, int buf_len )
 * @brief A test routine for debugging. It will print the first 16 bytes of header in hex format
 * and data part in character format.
 *
 * @param[in] buf buffer where messages are stored along with first 16 bytes header
 * @param[in] buf_len length of the buffer
 *
 * @return None
 */
void hexDump( char *buf, int buf_len )
{
	long int       addr = 0;
	long int       count = 0;
	long int       iter = 0;

	while (1)
	{
		// Print address
		printf ("%5d %08lx  ", (int) addr, addr );
		addr = addr + 16;

		// Print hex values
		for ( count = iter*16; count < (iter+1)*16; count++ ) {
			if ( count < buf_len ) {
				printf ( "%02x", buf[count] );
			}
			else {
				printf ( "  " );
			}
			printf ( " " );
		}

		printf ( " " );
		for ( count = iter*16; count < (iter+1)*16; count++ ) {
			if ( count < buf_len ) {
				if ( ( buf[count] < 32 ) || ( (unsigned char)buf[count] > 126 ) ) {
					printf ( "%c", '.' );
				}
				else {
					printf ( "%c", buf[count] );
				}
			}
			else
			{
				printf("\n");
				return;
			}
		}

		printf ("\n");

		iter++;
	}
}

/**
 * @addtogroup vodsourceadapi
 * @{
 * @fn void AutoDiscovery::adHandler( void* arg )
 * @brief This function runs in a loop and receives all messages from T2PServer and responds to them.
 *
 * This will wait till STT is acquired and will trigger auto discovery
 * and wait for it to get completed. Once auto discovery is complete, message will be sent from vod client.
 * The process is asynchronous, the response message will be received separately in T2PServer.
 *
 * @param[in] arg argument of a thread parameter, currently not being used.
 *
 * @return None
 */
void AutoDiscovery::adHandler( void* arg )
{
	struct sockaddr_in clntAddr;
	unsigned int clntLen = 0;
	int clntSock;
	int bytesRcvd = 0;
	int payloadLen = 0;
	char recvBuf[SNDBUFSIZE];
	char sendBuf[SNDBUFSIZE];
	bool waitForSTT = true;
	struct stat stt_file;
	int count =0;
	
//	do
//	{
//	    waitForSTT = ((stat("/tmp/stt_received", &stt_file) != 0));
//	    if(!count || !(count %5) || !waitForSTT)
//	    RDK_LOG( RDK_LOG_INFO,"LOG.RDK.VODSRC","%s:%s\n",__FUNCTION__,(waitForSTT) ? "Waiting for STT":"STT is already processed.");
//
//	    if(!waitForSTT)
//	    {
//	       break;
//	    }
//	    sleep(1);
//	    count++;
//
//	}
//	while( waitForSTT );
//
//
//
//	while ( RMF_SUCCESS != triggerAutoDiscovery() )
//	{
//		sleep( 2 );
//	}

	while ( !m_done ) 
	{
		clntLen = sizeof(clntAddr);
		if ((clntSock = accept( m_servSock, (struct sockaddr*)&clntAddr, &clntLen)) < 0 )
		{
			RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","accept() failed\n");
			sleep( 1 );
			continue;
		}

		RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","[%s:%d] Handling client %s\n", __FUNCTION__, __LINE__, inet_ntoa(clntAddr.sin_addr));
		bytesRcvd = recv( clntSock, recvBuf, 16, MSG_WAITALL );
		if ( bytesRcvd <= 0 )
		{
			RDK_LOG( RDK_LOG_WARN,"LOG.RDK.VODSRC","[%s:%d] Incomplete header\n", __FUNCTION__, __LINE__ );
			close( clntSock );
			continue;
		}
		else if ( (bytesRcvd < 16) || (recvBuf[0] != 't') || (recvBuf[1] != '2') ||(recvBuf[2] != 'p') )
		{
			RDK_LOG( RDK_LOG_INFO,"LOG.RDK.VODSRC","[%s:%d] Invalid message received", __FUNCTION__, __LINE__ );
			close( clntSock );
			continue;
		}
		if ( bytesRcvd > 0 )
		{
			RDK_LOG( RDK_LOG_DEBUG,"LOG.RDK.VODSRC","[%s:%d] Request header dump:\n", __FUNCTION__, __LINE__ );
            if(rdk_dbg_enabled("LOG.RDK.VODSRC", RDK_LOG_DEBUG))
            {
			    hexDump( recvBuf, bytesRcvd );
            }
		}

		payloadLen = ((recvBuf[12] << 24) & 0xFF000000) | ((recvBuf[13] << 16)  & 0x00FF0000) | 
		        ((recvBuf[14] << 8) & 0x0000FF00) | (recvBuf[15] & 0x000000FF);

		bytesRcvd = recv( clntSock, recvBuf+16, payloadLen, MSG_WAITALL );
		if ( bytesRcvd > 0 )
		{
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC", "[%s:%d] bytesRcvd: %d payloadLen: %d\n", __FUNCTION__, __LINE__, bytesRcvd, payloadLen );
			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.VODSRC", "[%s:%d] Request payload dump:\n", __FUNCTION__, __LINE__ ); 
            if(rdk_dbg_enabled("LOG.RDK.VODSRC", RDK_LOG_DEBUG))
            {
                hexDump( recvBuf+16, bytesRcvd );
            }
		}

		if ( bytesRcvd <= 0 )
		{
			RDK_LOG( RDK_LOG_WARN, "LOG.RDK.VODSRC", "[%s:%d] Failed to receive payload\n", __FUNCTION__, __LINE__ );
			close( clntSock );
			continue;
		}

		int sendMsgSz = SNDBUFSIZE - 16;
		jsonMsgHandler( &recvBuf[16], payloadLen, &sendBuf[16], &sendMsgSz );

		/* Send response */
		// Write header
		// protocol
		sendBuf[0] = 't';
		sendBuf[1] = '2';
		sendBuf[2] = 'p';

		// Version
		sendBuf[3] = 0x01;

		// Message id: Unique id
		sendBuf[4] = 's';
		sendBuf[5] = 'e';
		sendBuf[6] = 'r';
		sendBuf[7] = 'v';

		// Type : Request type = 0x1000
		sendBuf[8] = 0x00;
		sendBuf[9] = 0x00;
		sendBuf[10] = 0x10;
		sendBuf[11] = 0x00;

		// Length : Length of payload
		sendBuf[12] = (sendMsgSz & 0xFF000000) >> 24;
		sendBuf[13] = (sendMsgSz & 0x00FF0000) >> 16;
		sendBuf[14] = (sendMsgSz & 0x0000FF00) >> 8;
		sendBuf[15] = (sendMsgSz & 0x000000FF);

		/* Send message back to client */
		RDK_LOG( RDK_LOG_DEBUG,"LOG.RDK.VODSRC","[%s:%d] Response dump:\n", __FUNCTION__, __LINE__ );
        if(rdk_dbg_enabled("LOG.RDK.VODSRC", RDK_LOG_DEBUG))
        {
            hexDump( sendBuf, sendMsgSz+16 );
        }
		// Send the message to the client
		int bytesSent = send( clntSock, sendBuf, sendMsgSz+16, 0 );
		if ( bytesSent != sendMsgSz+16 )
		{
			RDK_LOG( RDK_LOG_INFO,"LOG.RDK.VODSRC","[%s:%d] Sent less than requested\n", __FUNCTION__, __LINE__ );
			sleep( 1 );
			close( clntSock );
			continue;
		}
	 
		close( clntSock ); 
	}
}

/**
 * @fn RMFResult AutoDiscovery::triggerAutoDiscovery()
 * @brief This function is used to create an instance for T2PClient for communicating with VOD Client.
 *
 * It will open a new connection with VOD Client and will send a message as "TriggerAutoDiscovery" to the VOD Client.
 * This will work only when the TEST_WITH_PODMGR is enabled. It will destroy the instance of T2PClient once communication is over.
 * It makes the flag m_adTriggered=true and m_adCompleted=false once communication successful with VOD Client.
 * Auto discovery response is received immediately from VOD client and that response means that VOD client has receved
 * the request and auto discovery will be started.
 *
 * @retval RMF_SUCCESS Trigger successful for Auto Discovery.
 * @retval RMF_RESULT_FAILURE Unable to communicate with VOD Client or invalid response received.
 */
RMFResult AutoDiscovery::triggerAutoDiscovery()
{
      	rmf_osal_Bool IpAcquired = false;
	const int msgBufSz = 1024;
	char t2p_resp_str[msgBufSz];
	char t2p_req_str[msgBufSz];
	uint8_t podmgrDSGIP[ 20 ] ;
	RMFResult result = RMF_RESULT_FAILURE;

	snprintf(t2p_req_str, sizeof (t2p_req_str), "{ \"TriggerAutoDiscovery\" : {}}\n");
#ifdef TEST_WITH_PODMGR
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC", "[%s:%d] Triggering auto discovery\n", __FUNCTION__, __LINE__ );
	T2PClient *t2pclient = new T2PClient();
	if ( NULL == t2pclient )
	{
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC", "[%s:%d] Object creation failed\n",
			 __FUNCTION__, __LINE__ );
		return RMF_RESULT_FAILURE;
	}

	if ( t2pclient->openConn(VOD_CLIENT_IP, VOD_CLIENT_PORT) == 0 )
	{
		int recvdMsgSz = 0;
		t2p_resp_str[0] = '\0';
		if ( (recvdMsgSz = t2pclient->sendMsgSync(t2p_req_str, strlen(t2p_req_str), t2p_resp_str, msgBufSz)) > 0 )
		{
			t2p_resp_str[recvdMsgSz] = '\0';
			RDK_LOG( RDK_LOG_DEBUG,"LOG.RDK.VODSRC","AutoDiscovery::Start(): Response : %s\n", t2p_resp_str );
			m_adTriggered = true;
			m_adCompleted = false;
			result = RMF_SUCCESS;
			/*
			Response:
			{
			"TriggerAutoDiscoveryResponse" : {
			<ResponseStatus>
			}
			}
			*/
		}
		t2pclient->closeConn();
	}
	delete t2pclient;
#endif
	return result;
}

/**
 * @fn RMFResult AutoDiscovery::stopAutoDiscovery()
 * @brief This function is used to stop the auto discovery by communicating with VOD Client.
 *
 * @retval RMF_SUCCESS Returns success only when the auto discovery stopped.
 * @retval RMF_RESULT_FAILURE Unable to communicate with VOD Client or invalid response received.
 * @}
 */

RMFResult AutoDiscovery::stopAutoDiscovery()
{
	RMFResult result = RMF_RESULT_FAILURE;
	const int msgBufSz = 1024;
	char t2p_resp_str[msgBufSz];
	char t2p_req_str[msgBufSz];

	sprintf(t2p_req_str, "{ \"StopAutoDiscovery\" : {}}\n");
#ifdef TEST_WITH_PODMGR
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC", "[%s:%d] Stop auto discovery\n",
		__FUNCTION__, __LINE__ );
	T2PClient *t2pclient = new T2PClient();
	if ( NULL == t2pclient )
	{
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC", "[%s:%d] Object creation failed\n",
			 __FUNCTION__, __LINE__ );
		return RMF_RESULT_FAILURE;
	}

	if ( t2pclient->openConn(VOD_CLIENT_IP, VOD_CLIENT_PORT) == 0 )
	{
		int recvdMsgSz = 0;
		t2p_resp_str[0] = '\0';
		if ( (recvdMsgSz = t2pclient->sendMsgSync(t2p_req_str,
			 strlen(t2p_req_str), t2p_resp_str, msgBufSz)) > 0 )
		{
			t2p_resp_str[recvdMsgSz] = '\0';
			m_adTriggered = false;
			m_adCompleted = false;
			result = RMF_SUCCESS;
			/*
			Response:
			{ "StopAutoDiscoveryResponse" : { <ResponseStatus> } }
			*/
		}
		t2pclient->closeConn();
	}
	delete t2pclient;
#endif
	return result;
}

#ifdef GLI
/**
 * @fn RMFResult AutoDiscovery::handleLowPower()
 * @brief This function is used to open the connection with VOD Client and send a message by filling the request string
 * "LowPower". This function works only when GLI and TEST_WITH_PODMGR is enabled.
 *
 * @retval RMF_SUCCESS Always returns success.
 */
RMFResult AutoDiscovery::handleLowPower()
{
	rmf_osal_Bool IpAcquired = false;
	const int msgBufSz = 1024;
	char t2p_resp_str[msgBufSz];
	char t2p_req_str[msgBufSz];
	uint8_t podmgrDSGIP[ 20 ] ;
	snprintf(t2p_req_str, sizeof (t2p_req_str),  "{ \"LowPower\" : {}}\n");
#ifdef TEST_WITH_PODMGR

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC", "[%s:%d] Triggering LowPower\n", __FUNCTION__, __LINE__ );
	T2PClient *t2pclient = new T2PClient();
	if ( NULL == t2pclient )
	{
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC", "[%s:%d] Object creation failed\n",
			 __FUNCTION__, __LINE__ );
		return RMF_RESULT_FAILURE;
	}

	if ( t2pclient->openConn(VOD_CLIENT_IP, VOD_CLIENT_PORT) == 0 )
	{
		int recvdMsgSz = 0;
		t2p_resp_str[0] = '\0';
		if ( (recvdMsgSz = t2pclient->sendMsgSync(t2p_req_str, strlen(t2p_req_str), t2p_resp_str, msgBufSz)) > 0 )
		{
			t2p_resp_str[recvdMsgSz] = '\0';
			RDK_LOG( RDK_LOG_DEBUG,"LOG.RDK.VODSRC","LowPower Response : %s\n", t2p_resp_str );
			m_adTriggered = true;
			m_adCompleted = false;
		}
		t2pclient->closeConn();
	}
	delete t2pclient;
#endif
	return RMF_SUCCESS;
}

/**
 * @fn RMFResult AutoDiscovery::handleFullPower()
 * @brief This function is used to open the connection with VOD Client and send a message by filling the request string
 * "FullPower". This function works only when GLI and TEST_WITH_PODMGR is enabled.
 *
 * @return None
 */
RMFResult AutoDiscovery::handleFullPower()
{
	rmf_osal_Bool IpAcquired = false;
	const int msgBufSz = 1024;
	char t2p_resp_str[msgBufSz];
	char t2p_req_str[msgBufSz];
	uint8_t podmgrDSGIP[ 20 ] ;
	snprintf(t2p_req_str, sizeof (t2p_req_str), "{ \"FullPower\" : {}}\n");
#ifdef TEST_WITH_PODMGR

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC", "[%s:%d] Triggering FullPower\n", __FUNCTION__, __LINE__ );
	T2PClient *t2pclient = new T2PClient();
	if ( NULL == t2pclient )
	{
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC", "[%s:%d] Object creation failed\n",
			 __FUNCTION__, __LINE__ );
		return RMF_RESULT_FAILURE;
	}

	if ( t2pclient->openConn(VOD_CLIENT_IP, VOD_CLIENT_PORT) == 0 )
	{
		int recvdMsgSz = 0;
		t2p_resp_str[0] = '\0';
		if ( (recvdMsgSz = t2pclient->sendMsgSync(t2p_req_str, strlen(t2p_req_str), t2p_resp_str, msgBufSz)) > 0 )
		{
			t2p_resp_str[recvdMsgSz] = '\0';
			RDK_LOG( RDK_LOG_DEBUG,"LOG.RDK.VODSRC","FullPower Response : %s\n", t2p_resp_str );

		}
		t2pclient->closeConn();
	}
	delete t2pclient;
#endif
	return RMF_SUCCESS;
}

RMFResult AutoDiscovery::handlePartnerIdChange()
{
	rmf_osal_Bool IpAcquired = false;
	const int msgBufSz = 1024;
	char t2p_resp_str[msgBufSz];
	char t2p_req_str[msgBufSz];
	uint8_t podmgrDSGIP[ 20 ] ;
	m_adCompleted=false;
	m_adFailed=true;
	snprintf(t2p_req_str, sizeof (t2p_req_str), "{ \"PartnerIdChange\" : {}}\n");
#ifdef TEST_WITH_PODMGR

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC", "[%s:%d] Triggering PartnerIdChange\n", __FUNCTION__, __LINE__ );
	T2PClient *t2pclient = new T2PClient();
	if ( NULL == t2pclient )
	{
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC", "[%s:%d] Object creation failed\n",
			 __FUNCTION__, __LINE__ );
		return RMF_RESULT_FAILURE;
	}

	if ( t2pclient->openConn(VOD_CLIENT_IP, VOD_CLIENT_PORT) == 0 )
	{
		int recvdMsgSz = 0;
		t2p_resp_str[0] = '\0';
		if ( (recvdMsgSz = t2pclient->sendMsgSync(t2p_req_str, strlen(t2p_req_str), t2p_resp_str, msgBufSz)) > 0 )
		{
			t2p_resp_str[recvdMsgSz] = '\0';
			RDK_LOG( RDK_LOG_DEBUG,"LOG.RDK.VODSRC","PartnerIdChange Response : %s\n", t2p_resp_str );

		}
		t2pclient->closeConn();
	}
	delete t2pclient;
#endif
	return RMF_SUCCESS;
}
#endif

/**
 * @fn static rmf_Error getDacId(uint16_t *pDACId)
 * @brief This function is used to sends a command through IPC to POD server to get the DAC Id and stores
 * the result in pDACId.
 *
 * @param[out] pDACId DAC id where the result will be stored
 *
 * @return RMF_SUCCESS on success, RMF_OSAL_ENODATA in case of IPC error.
 */
static rmf_Error getDacId(uint16_t *pDACId)
{
	int32_t result_recv = 0;
        int8_t res = 0;
        rmf_Error ret = RMF_SUCCESS;

        IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_DACID);
	
        res = ipcBuf.sendCmd( pod_server_cmd_port_no  );
        if ( 0 != res )
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
                        "POD Comm failure in %s:%d: ret = %d\n",
                                 __FUNCTION__, __LINE__, res );
                return RMF_OSAL_ENODATA;
        }

        result_recv = ipcBuf.getResponse( &ret );
        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
                        "POD Comm get response failed in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, result_recv );
                return RMF_OSAL_ENODATA;
        }
        
	result_recv = ipcBuf.collectData(pDACId, sizeof(uint16_t));
        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
                        "POD Comm get response failed in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, result_recv );
                return RMF_OSAL_ENODATA;
        }
	return ret;
}

/**
 * @fn static rmf_Error getplantid(uint32_t *pPlantID)
 * @brief It sends a command through IPC to CANH Server to get the Plant ID and
 * fills the result in pPlantID.
 *
 * @param[in] pPlantID Plant ID
 *
 * @return None
 */
static rmf_Error getplantid(uint32_t *pPlantID)
{
#ifndef USE_PXYSRVC
        return RMF_OSAL_ENOTIMPLEMENTED;
#else
        uint32_t response = RMF_SUCCESS;
        int32_t result_recv;
        uint8_t size= 0;
	uint8_t *psPlantId = NULL;

        if ( !isCANHReady() )
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.PXYSRVC",
                                 "Error!!Canh not started yet." );
                return RMF_OSAL_ENODATA;
        }

        IPCBuf ipcBuf = IPCBuf( ( uint32_t ) CANH_CMD_GET_PLANTID );

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

	if(RMF_SUCCESS != (rmf_osal_memAllocP(RMF_OSAL_MEM_IPC, size*sizeof(uint8_t), (void**)&psPlantId)))
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
	RDK_LOG(RDK_LOG_INFO,"LOG.RDK.PXYSRVC","%s():%d: Plant ID : %lu\n", __FUNCTION__, __LINE__, *pPlantID);

	rmf_osal_memFreeP(RMF_OSAL_MEM_IPC, psPlantId);
	psPlantId = NULL;
	return response;
#endif
}

/**
 * @addtogroup vodsourceadapi
 * @{
 * @fn int AutoDiscovery::jsonMsgHandler( char *recvMsg, int recvMsgSz, char *respMsg, int *respMsgBufSz )
 * @brief This function is used to parse the incoming message in json format depending on the following header information received.
 * <ul>
 * <li> The tag "AutoDiscoveryDone" is used to provide response from VOD client that auto discovery is completed.
 * <li> The tag "AutoDiscoveryInvalidated" is used to provide response from VOD client that auto discovery failed.
 * <li> The tag "tuneforautodiscovery" is used to provide frequency, modulation, TSID and SourceID.
 * <li> The tag "GetDacId" is used to get DAC Id. VOD client will request DAC ID.
 * <li> The tag "vodEOSNotification" is used to provide the End of Stream notification for VOD.
 * <li> The tag "vodSRMNotification" is used to provide Session Resource Manager (SRM) notification for requested Session Id.
 * </ul>
 * @param[in] recvMsg a JSON message format that need to parse and process according to the object available in it.
 * @param[in] recvMsgSz message length in bytes
 * @param[out] respMsg the result to be stored such as, for example tag "tuneforautodiscovery" the TSID to be stored, etc
 * @param[out] respMsgBufSz response message length in bytes
 *
 * @return Always returns 0
 */
int AutoDiscovery::jsonMsgHandler( char *recvMsg, int recvMsgSz, char *respMsg, int *respMsgBufSz )
{
        recvMsg[recvMsgSz] = '\0';
	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.VODSRC", "%s: %s\n", __FUNCTION__, recvMsg );

        json_t *msg_root;
        json_error_t json_error;
        json_t *Request;

        msg_root = json_loads( recvMsg, 0, &json_error );
        if ( !msg_root )
        {
        	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC", "json_error: on line %d: %s\n", json_error.line, json_error.text );
        }
        else
        {
		Request = json_object_get( msg_root, "AutoDiscoveryDone" );
		if ( Request )
		{
			RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC", "AutoDiscoveryDone\n");
			sprintf( respMsg, "{ \"AutoDiscoveryDone\" : { \"ResponseStatus\": %d }}\n", 0);
			*respMsgBufSz = strlen(respMsg) + 1;
			m_adCompleted=true;
		}
		else if( (Request = json_object_get( msg_root, "AutoDiscoveryFailed" )) != NULL)
		{
			RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC", "AutoDiscoveryFailed\n");
			sprintf( respMsg, "{ \"AutoDiscoveryFailed\" : { \"ResponseStatus\": %d }}\n", 0);
			*respMsgBufSz = strlen(respMsg) + 1;
			m_adCompleted=false;
			m_adFailed=true;
		}
		else if ( (Request = json_object_get(msg_root, "tuneforautodiscovery")) != NULL )
        	{
			json_t *frequency;
			json_t *modulation;
			int tsid = -1;
			int freq_val=-1;
			int modulation_val=-1;
			RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","Request for auto discovery tune\n");

			frequency = json_object_get(Request, "frequency");
			if( !frequency )
			{
				json_t *srcId;
				int sourceId;

				srcId = json_object_get( Request, "SourceId" );
				if ( srcId )
				{
					sourceId = json_integer_value( srcId );
					tuneAndGetTSID( sourceId, &tsid );	
				}
			}
			else
			{
				freq_val = json_integer_value( frequency );
				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC", "json_log: frequency: %d\n", freq_val );
				modulation = json_object_get( Request, "modulation" );
				if( modulation )
				{	
                        		modulation_val = json_integer_value( modulation );
					RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC", "json_log: modulation: %d\n", modulation_val );
					tuneAndGetTSID( freq_val, modulation_val, &tsid );
				}
			}
			
			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.VODSRC", "got tsid: %d\n", tsid );
			sprintf( respMsg, "{ \"tuneforautodiscoveryResponse\" : { \"ResponseStatus\": %d, \"tsid\": %d }}\n", 0, tsid );
			*respMsgBufSz = strlen( respMsg ) + 1;
			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.VODSRC", "respMsgBufSz=%d\n", *respMsgBufSz );
                }
		else if ( (Request = json_object_get(msg_root,"GetDacId")) != NULL )
		{
			uint16_t dacID = 0;
			rmf_Error error = getDacId( &dacID );
			if( error == RMF_SUCCESS )
			{								
				sprintf( respMsg, "{ \"GetDacIdResponse\" : { \"ResponseStatus\": %d, \"dacid\": \"%d\" }}\n", 0, dacID );
			}
			else
			{
				sprintf( respMsg, "{ \"GetDacIdResponse\" : { \"ResponseStatus\": %d, \"dacid\": \"%d\" }}\n", 1, dacID );
			}
			*respMsgBufSz = strlen( respMsg ) + 1;
		}
		else if ( (Request = json_object_get(msg_root, "vodEOSNotification")) != NULL )
		{
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC", "Received VOD EOS notification. Processing.. \n");
			json_t *sessionId = json_object_get(Request, "sessionId");
			
			if (sessionId)
			{
				std::string sessionIdString(json_string_value( sessionId ) );

				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC", "VOD EOS notification is for session: %s, invoking callback\n", sessionIdString.c_str());
				sprintf( respMsg, "{ \"vodEOSNotificationResponse\" : { \"ResponseStatus\": %d }}\n", 0);
				notifyEndOfStream( sessionIdString);
			}
			else
			{
				sprintf( respMsg, "{ \"vodEOSNotificationResponse\" : { \"ResponseStatus\": %d }}\n", -1);
			}
			*respMsgBufSz = strlen( respMsg ) + 1;
		}
		else if ( (Request = json_object_get(msg_root, "vodSRMNotification")) != NULL )
		{
			json_t *sessionId = json_object_get(Request, "sessionId");
			if ( sessionId )
			{
				std::string sessionIdString( json_string_value(sessionId) );
				sprintf( respMsg, "{ \"vodSRMNotificationResponse\" : { \"ResponseStatus\": %d }}\n", 0);
				json_t *srmCode = json_object_get( Request, "SRM" );
				if ( srmCode )
				{
					long long srmVal = json_integer_value( srmCode );
					RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC", "SRM notification session: %s Code: %x\n",
						sessionIdString.c_str(), srmVal );
					notifyVODSrc( sessionIdString, (uint32_t)srmVal );
				}
				else
				{
					RDK_LOG( RDK_LOG_WARN, "LOG.RDK.VODSRC", "No SRM code in notification - ignored\n" );
				}
			}
			else
			{
				sprintf( respMsg, "{ \"vodSRMNotificationResponse\" : { \"ResponseStatus\": %d }}\n", -1);
			}
			*respMsgBufSz = strlen( respMsg ) + 1;
		}
		else if ( (Request = json_object_get(msg_root, "vodPRCNotification")) != NULL )
		{
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC", "Received VOD PRC notification\n" );
			json_t *sessionId = json_object_get( Request, "sessionId" );
			if ( sessionId )
			{
				std::string sessionIdString(json_string_value( sessionId ) );
				sprintf( respMsg,
					"{ \"vodPRCNotificationResponse\" : { \"ResponseStatus\": %d }}\n", 0);
				json_t *playRate = json_object_get( Request, "playrate" );
				if ( playRate )
				{
					float playRateValue = json_real_value( playRate );
					RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC", "%s PlayRate: %f\n",
						sessionIdString.c_str(), playRateValue );
					notifyAdInsertion( RMF_VOD_AD_INSERTED );
				}
			}
			else
			{
				sprintf( respMsg, "{ \"vodPRCNotificationResponse\" : { \"ResponseStatus\": %d }}\n", -1);
			}
			*respMsgBufSz = strlen( respMsg ) + 1;
		}
		else
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.VODSRC", "json_error: Unknown message format\n" );
			*respMsgBufSz = 0;
		}
		// Release resources
		json_decref(msg_root);
        } 
        return 0;
}

/**
 * @fn RMFResult AutoDiscovery::tuneAndGetTSID( int SourceId, int *pTSID )
 * @brief This function will get the Transport Stream ID from PAT table once the SI acquisition is complete.
 *
 * It uses the QAMSrc properties and allocates a tuner for getting the Transport Stream ID. The QAMSource
 * module will be responsible for getting frequency and modulation values from xml file for the given SourceID.
 *
 * @param[in] SourceId a unique identification number of a program.
 * @param[out] pTSID a pointer for which Transport Stream ID value to be stored.
 *
 * @return RMFResult (int )
 * @retval RMF_RESULT_SUCCESS got the Transport stream ID by tuning to frequency.
 * @retval RMF_RESULT_FAILURE failed to get the Transport Stream ID from SI acquisition. It could be the reason
 * for resource unavailability, invalid source ID, unable to allocate tuners, and so on.
 */
RMFResult AutoDiscovery::tuneAndGetTSID( int SourceId, int *pTSID )
{
	rmf_Error rc = RMF_RESULT_FAILURE;
	uint32_t tsId;
	char SourceIdStr[256];
	RMFMediaSourceBase *pSource = NULL;

	snprintf( SourceIdStr, sizeof (SourceIdStr), "ocap://0x%x", SourceId );

	RDK_LOG( RDK_LOG_INFO,"LOG.RDK.VODSRC","[%s:%d] SourceId: %s\n", __FUNCTION__, __LINE__, SourceIdStr );
	if ( true == RMFQAMSrc::useFactoryMethods() )
	{
		pSource = RMFQAMSrc::getQAMSourceInstance( SourceIdStr, RMFQAMSrc::USAGE_AD, AUTODISCOVERY_TUNER_TOKEN );
	}
	else
	{
		pSource = new RMFQAMSrc();
	}
	if( NULL == pSource )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.VODSRC", "[%s:%d] Source create failed\n",
			__FUNCTION__, __LINE__ );
		return rc;
	}

	m_adTuningInprogress = true;

	rc = pSource->init();
	if ( RMF_RESULT_SUCCESS != rc )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.VODSRC", "[%s:%d] Source->init failed\n",
			__FUNCTION__, __LINE__ );
		goto err_01;
        }

        rc  = pSource->open( SourceIdStr, NULL );
        if ( RMF_RESULT_SUCCESS != rc )
        { 
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.VODSRC", "[%s:%d] Source->open failed\n",
			__FUNCTION__, __LINE__ );
		goto err_02;
        }
	
	rc = ((RMFQAMSrc*)pSource)->getTSID( tsId );
	if ( RMF_RESULT_SUCCESS != rc )
        {
		*pTSID = -1;
        }
	else
	{
		*pTSID = tsId;
	}

	pSource->close();
err_02:
	pSource->term();

err_01:
	if ( true == RMFQAMSrc::useFactoryMethods() )
	{
		RMFQAMSrc::freeQAMSourceInstance((RMFQAMSrc *)pSource, RMFQAMSrc::USAGE_AD, AUTODISCOVERY_TUNER_TOKEN);
	}
	else
	{
		delete pSource;
		pSource = NULL;
	}
	m_adTuningInprogress = false;
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC", "[%s:%d] Source id: %s tsId: 0x%x(%d)\n",
		 __FUNCTION__, __LINE__, SourceIdStr, tsId, tsId );
	return rc;
}

/**
 * @fn RMFResult AutoDiscovery::tuneAndGetTSID( int frequency, int modulation, int *pTSID )
 * @brief This function is used to get the Transport Stream ID by tuning to the input frequency and modulation.
 * It uses the QAMSrc properties to allocate a tuner for getting the Transport Stream ID.
 *
 * @param[in] frequency frequency for which need to be tuned to get the transport stream ID.
 * @param[in] modulation it is nothing but symbol rate. The listed supported modulation modes are available in structure rmf_ModulationMode.
 * @param[out] pTSID a pointer for which Transport Stream ID value to be stored.
 *
 * @return RMFResult (int )
 * @retval RMF_RESULT_SUCCESS got the Transport stream ID by tuning to frequency.
 * @retval RMF_RESULT_FAILURE failed to get the Transport Stream ID from SI acquisition. It could be the reason
 * for resource unavailability, invalid source ID, unable to allocate tuners, and so on.
 */
RMFResult AutoDiscovery::tuneAndGetTSID( int frequency, int modulation, int *pTSID )
{
	rmf_Error rc = RMF_RESULT_SUCCESS;
	char SourceIdStr[256];
	memset(SourceIdStr, 0, 256);
	snprintf(SourceIdStr, sizeof (SourceIdStr), "tune://frequency=%d&modulation=%d&", frequency, modulation);
	RDK_LOG(RDK_LOG_INFO ,  "LOG.RDK.VODSRC", " %s:%d : Autodiscovery for Cisco using locator %s\n",__FUNCTION__,__LINE__, SourceIdStr);
	//tune://frequency=<decimal frequency>&modulation=<decimal modulation code>&
	
	RMFMediaSourceBase *pSource = RMFQAMSrc::getQAMSourceInstance(SourceIdStr, RMFQAMSrc::USAGE_AD, AUTODISCOVERY_TUNER_TOKEN);
	if( NULL == pSource )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.VODSRC", "[%s:%d] Source create failed for Cisco AD\n",
                                                __FUNCTION__, __LINE__ );
		return RMF_RESULT_FAILURE;
	}

	m_adTuningInprogress = true;

	rc = pSource->init();
        if ( RMF_RESULT_SUCCESS != rc )
        {
        	RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.MS", "[%s:%d] Source->init failed\n",
                                           __FUNCTION__, __LINE__ );
	        RMFQAMSrc* qamsrc = ( RMFQAMSrc* )pSource;
               	RMFQAMSrc::freeQAMSourceInstance( qamsrc, RMFQAMSrc::USAGE_AD, AUTODISCOVERY_TUNER_TOKEN );
		m_adTuningInprogress = false;
		return RMF_RESULT_FAILURE;
        }

        rc  = pSource->open(SourceIdStr, NULL);
        if ( RMF_RESULT_SUCCESS != rc )
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.VODSRC", "[%s:%d] Source->open failed\n",
                                                __FUNCTION__, __LINE__ );
		m_adTuningInprogress = false;
                return RMF_RESULT_FAILURE;
        }
	     
	//Get TSID from qamsrc
	RMFQAMSrc* qamsrc = (RMFQAMSrc*) pSource;
	uint32_t tsId;
	rc = qamsrc->getTSID(tsId);
	*pTSID = tsId;

        if (RMF_RESULT_SUCCESS != rc )
        {
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.VODSRC", " %s:%d : getTSID failed\n",__FUNCTION__,__LINE__);
		*pTSID = -1;
		RMFQAMSrc::freeQAMSourceInstance( qamsrc );
		m_adTuningInprogress = false;
		return RMF_RESULT_FAILURE;
        }
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.VODSRC", "%s:%d getTSID - tsId = 0x%x\n",__FUNCTION__,__LINE__,tsId);

	RMFQAMSrc::freeQAMSourceInstance((RMFQAMSrc *)pSource);
	m_adTuningInprogress = false;

	return RMF_RESULT_SUCCESS;				
}

/**
 * @fn RMFResult AutoDiscovery::GetNumTunersForAD(int *pNumTuners)
 * @brief Get the number of tuner(s) available for Auto Discovery. Presently one QAM tuner
 * is dedicated for Auto discovery.
 *
 * @param[out] pNumTuners Number of tuner(s) set for auto discovery. It will be set to 1 always
 *
 * @retval RMF_RESULT_SUCCESS Returns success always.
 */
RMFResult AutoDiscovery::GetNumTunersForAD(int *pNumTuners)
{
	*pNumTuners = 1;
	
	return RMF_RESULT_SUCCESS;
}

#ifdef GLI
/**
 * @fn AutoDiscovery::_PowerPreChange(void* arg)
 * @brief This function is registered as a RPC method in IARMBUS and get notification from IARMMgr.
 * This will be called by power manager when any change
 * in power state such as Standby to Non Standby or vice-versa occurs.
 * When the power manager application sees any changes in power state, it invoke this RPC call to notify the new event.
 * These events will be passed on to a thread which will notify the same to VOD Client.
 *
 * @param[in] arg an argument which hold the current power state.
 *
 * @return On success the function will returns RMF_RESULT_SUCCESS. Returns error only when
 * it will not be able to create the task for updating the power state to VOD Client.
 */
IARM_Result_t AutoDiscovery::_PowerPreChange(void *arg)
{
	rmf_Error ret = RMF_RESULT_SUCCESS;
	rmf_osal_ThreadId tid;
	IARM_Bus_CommonAPI_PowerPreChange_Param_t *param = (IARM_Bus_CommonAPI_PowerPreChange_Param_t *)arg;
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC", "[%s:%d] Power State Changed: %d -- > %d\r\n", __FUNCTION__, __LINE__, param->curState , param->newState );
	IARM_Result_t retCode = IARM_RESULT_SUCCESS;
	struct powerMode *pwrMode = (powerMode *)malloc(sizeof(struct powerMode));
	pwrMode->pMode=param->newState;
	ret = rmf_osal_threadCreate( powerMonitorThread,(void *)pwrMode, RMF_OSAL_THREAD_PRIOR_DFLT, RMF_OSAL_THREAD_STACK_SIZE, &tid,"powerMonitorThread");	
	return retCode;
}

void AutoDiscovery::PartnerIdChange(void *data, size_t len)
{

	IARM_Bus_SYSMgr_EventData_t *sysEventData = (IARM_Bus_SYSMgr_EventData_t*)data;
	IARM_Bus_SYSMgr_SystemState_t stateId = sysEventData->data.systemStates.stateId;
	int state = sysEventData->data.systemStates.state;
	if (stateId == IARM_BUS_SYSMGR_SYSSTATE_PARTNERID_CHANGE)
	{
		rmf_osal_ThreadId tid;
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC", "[%s:%d] Partner ID Changed: \r\n", __FUNCTION__, __LINE__ );
		IARM_Result_t retCode = IARM_RESULT_SUCCESS;

		rmf_osal_threadCreate( partnerIdMonitorThread,0, RMF_OSAL_THREAD_PRIOR_DFLT, RMF_OSAL_THREAD_STACK_SIZE, &tid,"partnerIdMonitorThread");
	}
}

void AutoDiscovery::PlantIdReceived(void *data, size_t len)
{
	IARM_Bus_SYSMgr_EventData_t *sysEventData = (IARM_Bus_SYSMgr_EventData_t*)data;
	IARM_Bus_SYSMgr_SystemState_t stateId = sysEventData->data.systemStates.stateId;

	if (stateId == IARM_BUS_SYSMGR_SYSSTATE_PLANT_ID)
	{
		rmf_osal_ThreadId tid;
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC", "[%s:%d] IARM_BUS_SYSMGR_SYSSTATE_PLANT_ID event received: \r\n", __FUNCTION__, __LINE__ );
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC", "[%s:%d] Trigger AutoDiscovery: \r\n", __FUNCTION__, __LINE__ );
		rmf_osal_threadCreate( plantIdEventProcessingThread,0, RMF_OSAL_THREAD_PRIOR_DFLT, RMF_OSAL_THREAD_STACK_SIZE, &tid,"plantIdEventProcessingThread");
	}
}
#endif

/**
 * @fn bool AutoDiscovery::registerVodSource(std::string sessionId, void* objRef)
 * @brief This function is used to register its object with newly created session id into a map table.
 *
 * This gets called by RMFVODSrcImpl::open() whenever a new connection is established with client.
 * The map table m_vodSourceMap maintains the list of connections established.
 * When the application receive a VOD stop notification, it has to identify which session to stop.
 * For that mapping we are using the map table m_vodSourceMap.
 *
 * @param[in] sessionId a unique id for which communication has established between client and VOD source.
 * @param[in] objRef Object of class RMFVODSrcImpl.
 *
 * @return returns True always.
 */
bool AutoDiscovery::registerVodSource(std::string sessionId, void* objRef)
{
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC", "[%s:%d] Registering VodSRC with session ID : %s \r\n", __FUNCTION__, __LINE__, sessionId.c_str());
	m_vodSourceMap[sessionId] = objRef;
	return true;
}

/**
 * @fn bool AutoDiscovery::unregisterVodSource(std::string sessionId)
 * @brief This function is used to remove the entry from the map table for given sessionId. This is called by RMFVODSrcImpl::close().
 *
 * @param sessionId A unique id which need to be removed from the map table
 *
 * @return Returns always True.
 */
bool AutoDiscovery::unregisterVodSource(std::string sessionId)
{
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC", "[%s:%d] Unregistering VodSRC with session ID : %s \r\n", __FUNCTION__, __LINE__, sessionId.c_str());
	m_vodSourceMap.erase(sessionId);		
	return true;
}

/**
 * @fn void AutoDiscovery::notifyEndOfStream( std::string sessionId)
 * @brief This function is used to provide the end of stream notification to VOD Source.
 *
 * It is called by AutoDiscovery::jsonMsgHandler() which uses this function on receiving a message of
 * type 'vodEOSNotification'. This internally calls RMFVODSrc::EOSEventCallback() to
 * stop the current VOD serving instance.
 *
 * @param sessionId a unique identification number initially sent by client box during VOD request
 *
 * @return None
 */
void AutoDiscovery::notifyEndOfStream( std::string sessionId)
{
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC", "[%s:%d] Notifying EOS event to VodSRC with session ID : %s \r\n", __FUNCTION__, __LINE__, sessionId.c_str());
	std::map<std::string, void*>::iterator it= m_vodSourceMap.find( sessionId);
	if (it == m_vodSourceMap.end() )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.VODSRC", "[%s:%d] VodSRC with session ID : %s Not registered yet\r\n", __FUNCTION__, __LINE__, sessionId.c_str());
	}
	else
	{
		RMFVODSrc::EOSEventCallback(it->second);
	}
}

/**
 * @fn void AutoDiscovery::notifyVODSrc( std::string sessionId, uint32_t eventCode )
 * @brief This function is used to notify the VOD source with events received by auto discovery handler task.
 * When the handler receives a message with header "vodSRMNotification", this used to notify an event to VOD Source.
 *
 * @param[in] sessionId A unique identification number for a session
 * @param[in] eventcode VOD Source event code defined in rmfvodsource.h
 *
 * @return None
 * @}
 */
void AutoDiscovery::notifyVODSrc( std::string sessionId, uint32_t eventCode )
{
	std::map<std::string, void*>::iterator it = m_vodSourceMap.find( sessionId );
	if ( it == m_vodSourceMap.end() )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.VODSRC", "[%s:%d] VodSRC with session: "
			"%s Not registered yet\n", __FUNCTION__, __LINE__, sessionId.c_str());
	}
	else
	{
		eventNotifier( it->second, eventCode );
	}
}

void AutoDiscovery::notifyAdInsertion( unsigned int event )
{
	rmf_Error rc = RMF_SUCCESS;
	rmf_osal_event_handle_t eventHandle;

	rc = rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_HN,
		event, NULL, &eventHandle );
	if ( RMF_SUCCESS != rc )
	{
		RDK_LOG( RDK_LOG_FATAL, "LOG.RDK.VODSRC", "event_create failed\n" );
		return;
	}

	rc =  rmf_osal_eventqueue_add( m_adNotificationQueue, eventHandle );
	if ( RMF_SUCCESS != rc )
	{
		RDK_LOG( RDK_LOG_FATAL, "LOG.RDK.VODSRC", "eventqueue_add failed\n" );
	}
}

bool AutoDiscovery::getAdInsertionStatus()
{
	rmf_Error rc;
	rmf_osal_event_handle_t eventHandle;
	uint32_t eventType;
	rmf_osal_event_params_t	eventData;

	rc = rmf_osal_eventqueue_get_next_event_timed( m_adNotificationQueue,
		&eventHandle, NULL, &eventType, &eventData, 4500000);
	if ( RMF_SUCCESS == rc )
	{
		return (eventType == RMF_VOD_AD_INSERTED);
	}
	else if ( RMF_OSAL_ENODATA == rc )
	{
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.VODSRC", "eventqueue timed out\n" );
	}
	else
	{
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.VODSRC", "eventqueue error occured\n" );
	}

	return false;
}

void AutoDiscovery::clearAdInsertionQueue()
{
	rmf_osal_eventqueue_clear( m_adNotificationQueue );
}

RMFResult AutoDiscovery::releaseADTuner()
{
	while (true == m_adTuningInprogress)
	{
		usleep( 10000 );
	}
	return RMF_RESULT_SUCCESS;
}

static int cancelAutoDiscovery( const std::string reservationToken, const std::string sourceId )
{
	(void)reservationToken;
	(void)sourceId;

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.VODSRC", "Received cancel autodiscovery\n" );

	AutoDiscovery::getInstance()->stopAutoDiscovery();

	return AutoDiscovery::getInstance()->releaseADTuner();
}
