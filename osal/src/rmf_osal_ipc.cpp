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
 * @file rmf_osal_ipc.cpp
 * @brief This source file contains the APIs for IPC communication
 */

#include <rmf_osal_ipc.h>
#include <iostream>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <rdk_debug.h>
#include "rmf_osal_resreg.h"
#include <assert.h>

using namespace std;

#define IPC_VERSION_OFFSET 0
#define IPC_COMMNAD_OFFSET ( IPC_VERSION_OFFSET + IPC_VERSION_SIZE )
#define IPC_PAYLOAD_LEN_OFFSET ( IPC_VERSION_SIZE + IPC_COMMAND_SIZE )
#define IPC_DATA_OFFSET ( IPC_PAYLOAD_LEN_OFFSET + IPC_PAYLOAD_LEN_SIZE )
#define IPC_BIND_MAX_RETRY_COUNT ( 30 )
#define IPC_BIND_RETRY_INTERVAL_MS ( 2000 )
#define IPC_DELAY_FOR_REBOOT_MS ( 5000 )
#define IPC_BIND_EPRINT_PATH "/opt/logs/bindError.txt"

//#define  AUTO_LOCK( lock ) VL_AUTO_LOCK( lock )

/**
 * @brief Class Constructor.
 *
 * It is used to initialize the client buffer.
 *
 * @param[in] command Client command will be sent to associated IPC server.
 *
 * @return None
 * @ingroup OSAL_IPC_API
 */
IPCBuf :: IPCBuf( uint32_t command )
{
	send_payload_len = recvd_payload_len = to_be_collected = 0;
	memset( recvBuff, 0, IPC_BUF_SIZE );
	memset( sendBuff, 0, IPC_BUF_SIZE );
	receivable = false;

	sendBuff[ IPC_VERSION_OFFSET ] = IPC_VERSION;
	memcpy(&sendBuff[ IPC_COMMNAD_OFFSET ], 
			&command,  
			IPC_COMMAND_SIZE);
	/* IPPV_CMD_send_payload_len_OFFSET  is already zero */
}

/**
 * @brief This function is used to collect data sent from IPC server.
 *
 * @param[out] data a response data to be stored.
 * @param[in] len Indicates how much data need to be copied in data buffer.
 *
 * @return Success if a non negative value is returned from this function indicating
 * how much data is still there in the IPC server yet to be collected.
 * It will return 0 if all the data from IPC buffer has been collected.
 * @retval -1 If the data is not receivable from IPC Buffer.
 * @retval -2 If the specified length from input parameter will be greater what the actual
 * payload available to receive.
 * @ingroup OSAL_IPC_API
 */
int32_t IPCBuf :: collectData( void* data, uint16_t len )
{
	//AUTO_LOCK( m_lock );
	if( !receivable ) 
	{
		RDK_LOG(
                  RDK_LOG_ERROR,
                  "LOG.RDK.IPC","Error: No message is receivable yet \n");
		return -1;
	}
	
	if( (recvd_payload_len - len) < 0 ) 
	{
		RDK_LOG(
                  RDK_LOG_ERROR,
                  "LOG.RDK.IPC","Error: Received less data than requested \n");
		return -1;
	}

        if ( len > to_be_collected )
        {
                RDK_LOG(
                  RDK_LOG_ERROR,
                  "LOG.RDK.IPC","Error: Requesting more data than received \n");
                assert( to_be_collected >= len );
                return -2;
        }

	
	memcpy( data, 
		&recvBuff[ IPC_DATA_OFFSET + (recvd_payload_len - to_be_collected) ], 
		len );
	to_be_collected -= len;

	return to_be_collected;
}

/**
 * @brief This function is used to read the response from IPC server.
 *
 * @param[out] pResponse The response command from IPC server which needs to be stored.
 * @return Returns the payload length if success, else returns -1 on data is not receivable.
 * @ingroup OSAL_IPC_API
 */
int32_t IPCBuf :: getResponse(uint32_t *pResponse)
{
	//AUTO_LOCK( m_lock );

	if( !receivable ) 
	{
		RDK_LOG(
                  RDK_LOG_ERROR,
                  "LOG.RDK.IPC","Error: No message is receivable yet \n");
		return -1;
	}
	memcpy( pResponse, 
		&recvBuff[ IPC_COMMNAD_OFFSET ], 
		IPC_COMMAND_SIZE );

	/* data lenght */
	return recvd_payload_len;
}

/**
 * @brief This function is used to clear the buffer. This function need to be called when
 * the application wants to reuse the object which has already created for the same command.
 * @return None
 * @ingroup OSAL_IPC_API
 */
void IPCBuf :: clearBuf(  )
{
	//AUTO_LOCK( m_lock );
	/*  Ready to send again using same buffer +1 is for pay load length */	
	memset( &sendBuff[ IPC_PAYLOAD_LEN_OFFSET ], 0, 
					(send_payload_len+1) ); 	
	send_payload_len = 0;
}

/**
 * @brief This function is used to add the data passed as parameter that can be send using send command. 
 * Application need to take care that the length of the data should not exceed
 * IPC_MAX_BUF_LEN (currently set as 2048 bytes, that can be changed).
 * @param[in] data memory pointer where user data is available
 * @param[in] len length of the data to be added
 *
 * @return Returns the payload length to be send to the IPC server, returns -1 if length exceed to max buffer lengh.
 * @ingroup OSAL_IPC_API
 */
int32_t IPCBuf :: addData( const void* data, uint16_t len )
{
	//AUTO_LOCK( m_lock );

	receivable = false;

	/* addition of data may lead to overflow */
	if( ( send_payload_len + len ) > IPC_MAX_BUF_LEN )
	{
		RDK_LOG(
                  RDK_LOG_ERROR,
                  "LOG.RDK.IPC","Error: addition of data may lead to overflow.\n");
		return -1;
	}
	memcpy( &sendBuff[ IPC_DATA_OFFSET + send_payload_len ], 
		data, len);
	send_payload_len += len;

	/* update payload length filed */
	memcpy( &sendBuff[ IPC_PAYLOAD_LEN_OFFSET ], 
		&send_payload_len, sizeof(send_payload_len) );

	/* returns total number of bytes added */
	return send_payload_len;
}

/*  if ip is NULL, address is that of loop back  */

/**
 * @brief This function is used to take the data added by addData() and will send to the IPC server.
 * @param[in] port port number to send the command to the IPC server.
 *
 * @return 0 on success, other than 0 is considered as failure
 * @ingroup OSAL_IPC_API
 */
int8_t IPCBuf :: sendCmd( uint32_t port )
{
	//AUTO_LOCK( m_lock );
	int32_t size = 0;
	size = sendRecv( port );	
	if( size < 0 ) return size;
	
	/* minimum length expected */			
	if( size < IPC_DATA_OFFSET )
	{
		memset( recvBuff, 0, IPC_BUF_SIZE );
		RDK_LOG(
                  RDK_LOG_ERROR,
                  "LOG.RDK.IPC","Error: Received less data than expected, protocol error \n");
		return -5;
	}

	memcpy( &recvd_payload_len, &recvBuff[ IPC_PAYLOAD_LEN_OFFSET ],
		sizeof(recvd_payload_len) );
				
	/* payload length indicates some data but recvd less/more */
	if( size != (IPC_DATA_OFFSET +  recvd_payload_len) )
	{
		RDK_LOG(
                  RDK_LOG_ERROR,
                  "LOG.RDK.IPC","Error: recvd less/more than expected \n");
		return -6;
	}

	to_be_collected = recvd_payload_len;
	receivable = true;
	return 0;
}

/**
 * @brief This function is used to send a message and receive response from server application
 * which is running on the same system. It uses simple TCP Socket for establishing the connection 
 * to the server application. This is an internal function which will communicate with IPC server.
 *
 * @param[in] port Port number from where an application need to be connected to.
 *
 * @return Returns the amount of data received interms of bytes, in case of error returns a negative value.
 * @retval -1 unable to create TCP socket
 * @retval -2 no loop back IP is available for this box
 * @retval -3 unable to connect to the IPC server using TCP socket
 * @retval -4 validation failed for data which need to be sent to the IPC server
 * @ingroup OSAL_IPC_API
 */
int32_t IPCBuf :: sendRecv ( uint32_t port )
{
	//AUTO_LOCK( m_lock );
	
	int32_t sockfd = 0, size = 0;	
	struct sockaddr_in serv_addr; 	

	if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		RDK_LOG(
                  RDK_LOG_ERROR,
                  "LOG.RDK.IPC","Error: Could not create socket \n");
		return -1;
	} 
	
	memset(&serv_addr, 0, sizeof(serv_addr)); 
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port); 
	if( inet_pton( AF_INET, "127.0.0.1", &serv_addr.sin_addr ) <=0 )
	{
		RDK_LOG(
                  RDK_LOG_ERROR,
                  "LOG.RDK.IPC","Error: inet_pton error occured \n");
		close( sockfd );
		return -2;
	} 

	if( connect( sockfd, (struct sockaddr *)&serv_addr, 
								sizeof(serv_addr)) < 0)
	{
		RDK_LOG(
                  RDK_LOG_ERROR,
                  "LOG.RDK.IPC","Error: Connect Failed \n");
		close( sockfd );
		return -3;
	}

	size = write( sockfd, sendBuff, 
		(send_payload_len + IPC_DATA_OFFSET) );
	if( size !=  (send_payload_len + IPC_DATA_OFFSET) )
	{
		RDK_LOG(
                  RDK_LOG_ERROR,
                  "LOG.RDK.IPC","Error: write failed \n");
		close( sockfd );
		return -4;
	}
	size = read( sockfd, recvBuff, IPC_BUF_SIZE );	
	close( sockfd );
	return size;			
}

IPCBuf :: ~IPCBuf( )
{

}


/**
 * @brief Constructor used for initializing the member variables. 
 *
 * @param[in] response_fd socket Id for which data need to be received.
 *
 * @return None
 * @ingroup OSAL_IPC_API
 */
IPCServerBuf :: IPCServerBuf( int32_t response_fd )
{
	send_payload_len = recvd_payload_len = to_be_collected = 0;
	Cmd = 0;
	connection_fd =0;
	memset( recvBuff, 0, IPC_BUF_SIZE );
	memset( sendBuff, 0, IPC_BUF_SIZE );
	sendable = false;
	this->response_fd = response_fd;
}

/**
 * @brief This function is used to accept the new connection from client. After this, the application can call
 * getCmd() to receive the command that has sent from client.
 *
 * @return Returns a non negative value indicating the length of data received from IPC server
 * @retval -1 Response fd is not yet created
 * @retval -5 unable to accept the connection from client
 * @retval -3 read the amount of data which is minimum then expected
 * @retval -4 received data length is invalid
 * @ingroup OSAL_IPC_API
 */
int32_t IPCServerBuf :: recvCmd (  )
{
	//AUTO_LOCK( m_lock );
	int32_t size;
	if( !response_fd ) return -1;
	
	connection_fd = accept( response_fd, (struct sockaddr*)NULL, NULL ); 
	RDK_LOG(RDK_LOG_DEBUG,"LOG.RDK.IPC","Accepted connection on sockfd:%d \n",connection_fd);
	if(connection_fd < 0 )
	{
		RDK_LOG(
                  RDK_LOG_ERROR,
                  "LOG.RDK.IPC","Error: Failed to accept connection \n");
		return -5;
	}
	
	if ( (size = read( connection_fd, recvBuff, IPC_BUF_SIZE )) <= 0)
	{
		close( connection_fd );
		connection_fd = 0;
		RDK_LOG(
                  RDK_LOG_ERROR,
                  "LOG.RDK.IPC","Error: Read failed \n");
		return -2;
	}

	/* minimum length expected */
	if( size < IPC_DATA_OFFSET )
	{
		memset( recvBuff, 0, IPC_BUF_SIZE );
		RDK_LOG(
                  RDK_LOG_ERROR,
                  "LOG.RDK.IPC","Error: Received less data than expected, protocol error \n");
		close( connection_fd );
		connection_fd = 0;
		return -3;
	}

	memcpy( &recvd_payload_len, 
		&recvBuff[ IPC_PAYLOAD_LEN_OFFSET ],
		sizeof(recvd_payload_len) );
	
	if( recvd_payload_len > IPC_MAX_BUF_LEN )
	{
		RDK_LOG(
                  RDK_LOG_ERROR,
                  "LOG.RDK.IPC",
                  "Error: Received more data [%d] than expected for the command is %x \n",
                  recvd_payload_len, getCmd() );
		close( connection_fd );
		connection_fd = 0;
		return -5;
	}

	memcpy( &Cmd, 
		&recvBuff[ IPC_COMMNAD_OFFSET ], sizeof(uint32_t) );

        RDK_LOG(RDK_LOG_DEBUG,"LOG.RDK.IPC","IPC cmd received %d\n",Cmd);
	/* payload length indicates some data but recvd less/more */
	if( size != (IPC_DATA_OFFSET +  recvd_payload_len) )
	{
		RDK_LOG(
                  RDK_LOG_ERROR,
                  "LOG.RDK.IPC","Error: recvd less/more than expected \n");
		close( connection_fd );
		connection_fd = 0;
		return -4;
	}

	to_be_collected = recvd_payload_len;
	return recvd_payload_len;

}

/**
 * @brief This function is used to get the last command that has been received from the application. 
 *
 * @return Returns the last command.
 * @ingroup OSAL_IPC_API
 */
uint32_t IPCServerBuf :: getCmd (  )
{
	return Cmd;
}

/**
 * @brief This function is used to get the available data length in received buffer. 
 *
 * @return Returns the payload buffer length in bytes.
 * @ingroup OSAL_IPC_API
 */
uint16_t IPCServerBuf :: getDataLen( )
{
	return recvd_payload_len;
}

/**
 * @brief Once the application receive the command from client, this function is used to send response to client.
 * @param[in] response The response command which need to sent to client.
 * @return Returns the payload buffer length in bytes.
 * @ingroup OSAL_IPC_API
 */
int32_t IPCServerBuf :: addResponse ( uint32_t response )
{
	//AUTO_LOCK( m_lock );
	/* reset send buffer to enable sending again using same buffer */
	
	memcpy(&sendBuff[ IPC_COMMNAD_OFFSET ], 
		&response,  IPC_COMMAND_SIZE);
	sendable = true;
	return 0;
}

/**
 * @brief This function is used to send the response to the client.
 *
 * @return Returns the payload buffer length in bytes in case of non negative value is returned.
 * @retval -1 Unable to write the response to the client
 * @retval -4 Verification failed for amount of data that is sent to the client
 * @ingroup OSAL_IPC_API
 */
int32_t IPCServerBuf :: sendResponse ( )
{
	//AUTO_LOCK( m_lock );
	int32_t size = 0;

	if ( !sendable ) 
	{
		RDK_LOG(
                  RDK_LOG_ERROR,
                  "LOG.RDK.IPC","Error: Response already send\n");
		return -1;
	}
	size = write( connection_fd, sendBuff, 
		(send_payload_len + IPC_DATA_OFFSET) );
	if( size !=  (send_payload_len + IPC_DATA_OFFSET) )
	{
		RDK_LOG(
                  RDK_LOG_ERROR,
                  "LOG.RDK.IPC","Error: write failed \n");
		return -4;
	}
	sendable = false;
	return size;			
}

/**
 * @brief This function is used to clear the buffer. If the application wants to reuse the object which has
 * already been created for the same command, this function need to be called.
 * 
 * @return None
 * @ingroup OSAL_IPC_API
 */
void IPCServerBuf :: clearBuf(  )
{
	//AUTO_LOCK( m_lock );
	/*  Ready to send again using same buffer +1 is for pay load length */	
	memset( &sendBuff[ IPC_PAYLOAD_LEN_OFFSET ], 0, 
					(send_payload_len+1) ); 	
	send_payload_len = 0;
}

/**
 * @brief This function is used to add the data passed as parameter that can be sent to client. 
 *
 * @param[in] data memory pointer where user data is available
 * @param[in] len length of the data that need to be sent to client
 *
 * @return total data length in bytes which need to be sent to client if the function returns a non negative value.
 * @retval -1 addition of data lead to overflow
 * @ingroup OSAL_IPC_API
 */
int32_t IPCServerBuf :: addData( const void* data, uint16_t len )
{
	//AUTO_LOCK( m_lock );

	/* addition of data may lead to overflow */
	if( ( send_payload_len + len ) > IPC_MAX_BUF_LEN )
	{
		RDK_LOG(
                  RDK_LOG_ERROR,
                  "LOG.RDK.IPC","Error: addition of data may lead to overflow.\n");
		return -1;
	}
	memcpy( &sendBuff[ IPC_DATA_OFFSET + send_payload_len ], 
		data, len);
	send_payload_len += len;

	/* update payload length filed */
	memcpy( &sendBuff[ IPC_PAYLOAD_LEN_OFFSET ], 
		&send_payload_len, sizeof(send_payload_len) );

	/* returns total number of bytes added */
	return send_payload_len;
}

/**
 * @brief This function is used to collect data sent from client.
 *
 * @param[out] data a response data to be stored
 * @param[in] len Indicates how much data need to be collected from buffer.
 * 
 * @retval -1 received payload length is lesser than the requested length.
 * @retval -2 If the specified length from input parameter is greater than the actual
 * payload available to collect.
 * @ingroup OSAL_IPC_API
 */
int32_t IPCServerBuf :: collectData( void* data, uint16_t len )
{
	//AUTO_LOCK( m_lock );
	
	if( (recvd_payload_len - len) < 0 ) 
	{
		RDK_LOG(
                  RDK_LOG_ERROR,
                  "LOG.RDK.IPC","Error: Received less data than requested for cmd %d \n", getCmd() );
		return -1;
	}

        if ( len > to_be_collected )
        {
                RDK_LOG(
                  RDK_LOG_ERROR,
                  "LOG.RDK.IPC","Error: Requesting more data than received \n");
                assert( to_be_collected >= len );
                return -2;
        }
		
	memcpy( data, 
		&recvBuff[ IPC_DATA_OFFSET + (recvd_payload_len - to_be_collected) ], 
		len );
	to_be_collected -= len;

	return to_be_collected;
}

/**
 * @brief This is a default destructor for closing the IPC connection if it is already opened.
 *
 * @return None
 */
IPCServerBuf :: ~IPCServerBuf( )
{
	if( connection_fd != 0 ) close( connection_fd );
	connection_fd = 0;		
}


/**
 * @brief A constructor used to set the application name and 
 * port number for IPCServer. 
 *
 * @param[in] name name of the application such as "ippvServer", "qamSrcServer", "podServer", etc.
 * @param[in] port port number for which the IPC server application need to bind
 *
 * @return None
 * @ingroup OSAL_IPC_API
 */
	IPCServer :: IPCServer( int8_t *name, uint32_t port )
	{
		this->port = port;
		this->name = name;
		active = false;
		pCallBk = NULL;
		pUserData = NULL;
		threadId = 0;
		thread_exited = false;
		memset(sendBuff,0,sizeof(sendBuff));		
		rmf_osal_condNew( TRUE, FALSE, &StartCond );
	}

	IPCServer :: ~IPCServer(  )
	{
		rmf_osal_condDelete(StartCond);
	}

/**
 * @brief This function is used to register a callback function for handling the request from client. Whenever data 
 * command comes from client, this callback gets invoked.
 *
 * @param[in] fun It is a pointer of the callback function that need to be registered.
 * @param[in] pUserData Instance of IPCServer.
 *
 * @return Returns 0 on successfully registered the callback, else returns -1 if some callback is already registered.
 * @ingroup OSAL_IPC_API
 */
	int8_t IPCServer :: registerCallBk( void (*fun) ( IPCServerBuf *, void *pUserData), void *pUserData  )
	{
		if( NULL != this->pCallBk )
		{
			RDK_LOG(
                  RDK_LOG_ERROR,
                  "LOG.RDK.IPC","Error: Already registered a call back \n");
			return -1;
		}
		this->pCallBk = fun;
		this->pUserData = pUserData;
		return 0;
	}

	void IPCServer :: threadFn(void *param)
	{
		
		IPCServer *ptr = (IPCServer *) param;
		ptr->listen_request( param );
	}
	
/**
 * @brief This function is used to start the server. 
 * @return Returns 0 on success, else return -1 if the server is not active
 * @ingroup OSAL_IPC_API
 */
	int8_t IPCServer :: start()
	{
		//AUTO_LOCK( m_lock );
		if( active ) return -1;		
/*
		rmf_osal_threadCreate( threadFn, NULL, 
			RMF_OSAL_THREAD_PRIOR_DFLT, 
			RMF_OSAL_THREAD_STACK_SIZE, &threadId, name );
*/
		rmf_Error ret;
		if ((ret = rmf_osal_threadCreate(threadFn, (void *) (this),
                RMF_OSAL_THREAD_PRIOR_DFLT, RMF_OSAL_THREAD_STACK_SIZE, &threadId,
                "serverThread")) != RMF_SUCCESS)
                {
                        RDK_LOG(
                        RDK_LOG_ERROR,
                        "LOG.RDK.IPC",
                        "IPCServer :: start()d: failed to create serverThread thread (err=%d).\n",
                         ret);
                }
/*
		int i = pthread_create(&threadId, NULL,
                          threadFn, this);
*/
		rmf_osal_condGet( StartCond );
		active = true;
		return 0;
	}

/**
 * @brief This function is used to send command "IPC_HALT_SERVER" to IPC server to quit from handler thread function.
 *
 * @retval 0 Successfully stopped
 * @retval -1 IPC Server is not yet started
 * @retval -3 Unable to communicate with IPC Server application
 * @retval -2 Invalid response received from IPC Server
 * @ingroup OSAL_IPC_API
 */
	int8_t IPCServer :: stop()
	{
		//AUTO_LOCK( m_lock );
		uint32_t response = 0;
		int8_t status = 0;
		
		IPCBuf buff( IPC_HALT_SERVER );
		if( !active ) return -1;
		active = false;

		status = buff.sendCmd( this->port );
		if(0 != status)
		{
			RDK_LOG(
                  	RDK_LOG_ERROR,
                  	"LOG.RDK.IPC","Error: Unable to send Stop command \n");
			return -3;
		}
		if(0 > buff.getResponse( &response ))
		{
			RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.IPC","Error: No messages recieved!! \n");
			return -1;
		}

		if( IPC_HALT_SERVER != response ) 
		{
			RDK_LOG(
                  	RDK_LOG_ERROR,
                  	"LOG.RDK.IPC","Error: Server stop failed \n");
			return -2;
		}
		rmf_osal_condUnset( StartCond );
		return 0;
	}

/**
 * @brief This function is used to free the resource allocated for IPCServer. 
 * It is a mandatory function that need to be called once user command gets processed. 
 * @return None
 * @ingroup OSAL_IPC_API
 */
	void IPCServer :: disposeBuf( IPCServerBuf *pServerBuf )
	{
		if (pServerBuf != NULL)
		{	  
		  RDK_LOG(
			  RDK_LOG_DEBUG,
			  "LOG.RDK.IPC","delete pServerBuf %p\n", pServerBuf);
		  delete pServerBuf;
		  pServerBuf = NULL;
		}
		else
		{
		  RDK_LOG(
			  RDK_LOG_ERROR,
			  "LOG.RDK.IPC","ERROR: skip delete since pServerBuf is NULL\n");
		}
	}
	
	void IPCServer :: listen_request( void *param )
	{
		struct sockaddr_in serv_addr; 
		int32_t sockFd =0;
		( void )param;
		int32_t size;
		uint32_t command;
		IPCServerBuf *pServerBuf = NULL;
		int status = 0;
		int8_t bindRetryCount = 0;//IPC_BIND_MAX_RETRY_COUNT;
		
		sockFd = socket( AF_INET, SOCK_STREAM, 0 );
		if (sockFd == -1)
		{
			RDK_LOG(
                  	RDK_LOG_ERROR,
                  	"LOG.RDK.IPC","socket error ");
			return;
		}
		
		memset( &serv_addr, 0, sizeof(serv_addr) );
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = htonl( INADDR_LOOPBACK );		
		serv_addr.sin_port = htons( port ); 
		
		do
		{
			status = bind( sockFd, (struct sockaddr*)&serv_addr, sizeof(serv_addr) );
			if (status == -1)
			{
				char procData[1024];
				memset( procData, 0, 1024 );
				RDK_LOG(
						 RDK_LOG_ERROR,
						 "LOG.RDK.IPC","bind error %s\n", strerror(errno));
				snprintf( procData, 1024, "date >> %s; netstat -tanp | grep \"%d\" >> %s", 
									IPC_BIND_EPRINT_PATH , port, IPC_BIND_EPRINT_PATH );
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPC", "%s\n", procData );
				int ret = system( procData );
				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.IPC", "exe.system returned %d\n", ret );
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPC", "Check the file - %s\n", IPC_BIND_EPRINT_PATH);
				memset( procData, 0, 1024 );
				snprintf( procData, 1024, "netstat -tanp | grep \"%d\" ", port );
				ret = system( procData );
				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.IPC", "exe.system returned %d\n", ret );
				rmf_osal_threadSleep( IPC_BIND_RETRY_INTERVAL_MS, 0 );
				++bindRetryCount;
				//continue;
			}
			else
			{
				// success;
				break;
			}

		}while( bindRetryCount < IPC_BIND_MAX_RETRY_COUNT );

		if( ( bindRetryCount >= IPC_BIND_MAX_RETRY_COUNT ) )
		{
			close( sockFd );
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPC", "Rebooting the system due to binderror in IPC ..." );
            rmf_osal_LogRebootEvent(__FUNCTION__, "Rebooting the system due to binderror in IPC ..." );
			rmf_osal_threadSleep( IPC_DELAY_FOR_REBOOT_MS, 0 );
			system( "sh /rebootNow.sh -s IPC_bindErrorRecovery -o 'Rebooting the box due to binderror in IPC...'" );
		}
		
		listen( sockFd, 100 );
		rmf_osal_condSet( StartCond );
		while( true)
		{
			int32_t ret = 0;
			pServerBuf  = new IPCServerBuf ( sockFd );
			
			// make sure pServerBuf point to a valid IPCServerBuf object
			if (pServerBuf != NULL)
			{
			  ret = pServerBuf->recvCmd( );
			  if( ret < 0 )
			  {
			      RDK_LOG(
				      RDK_LOG_ERROR,
				      "LOG.RDK.IPC","recvCmd is failed for sockFd %d\n", sockFd );
				delete pServerBuf;
				continue;
			  }
			  command = pServerBuf->getCmd();
			  //break;
			  /* if command is to exit */
			  if( (IPC_HALT_SERVER == command)  && ( !active ))
			  {
			    pServerBuf->addResponse( IPC_HALT_SERVER );
			    pServerBuf->sendResponse(  );
			    RDK_LOG(
				    RDK_LOG_DEBUG,
				    "LOG.RDK.IPC","Server going to halt....delete pServerBuf %p\n", pServerBuf);				
			    /* to ensure that thread the stop 
			       command is recvd from stop() itself */	
			    delete pServerBuf;
			    pServerBuf = NULL;
			    break;
			  }
			
			  if( NULL != pCallBk )
			  {
			    (*pCallBk) ( pServerBuf, this->pUserData );
			  }
			  else
			  {
			    if (pServerBuf != NULL)
			    {
			      RDK_LOG(
				      RDK_LOG_DEBUG,
				      "LOG.RDK.IPC","pCallBk is NULL, just delete pServerBuf %p\n", pServerBuf);
				    pServerBuf->addResponse( RMF_OSAL_EBUSY );
				    pServerBuf->sendResponse(  );				  
			      delete pServerBuf;
			      pServerBuf = NULL;
			    }
			  }
			}
			else
			{
			  RDK_LOG(
				  RDK_LOG_ERROR,
				  "LOG.RDK.IPC","ERROR: new IPCServerBuf return NULL.  Running out of memory or socket failure.  Skip processing IPC socket event....\n");  
			}
			
		}
		
		close( sockFd );
		sockFd = 0;
	}



