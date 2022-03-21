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
 * @file rmf_osal_ipc.h
 * @brief The header file for Inter process communication.
 */

/**
 * @defgroup OSAL_IPC OSAL IPC(Inter Process Communication)
 * The OSAL IPC is used for the communication between the processes, such as Java application and RMF.
 * Mainly this module is used to send the commands, notification (status) and to get the details
 * from the OSAL layer components.
 *
 * @par OSAL IPC Message format
 * The message format for sending the command is as follow, <br><br>
 * <table border=2 rowheight=20px>
 * <tr><td width=10%>Version</td><td width=40%>Command </td><td width=20%>Payload Length</td> <td width=30%> Payload </td> </tr>
 * </table>
 *
 * - Version: This is an 8-bit field is the version number of the OSAL IPC.
 * - Command: This is a 32-bit field which serves as a label to identify the request/response type.
 * - Payload length: This 16-bit field gives the total length in bytes of the Payload following.
 * - Payload: Actual data contains the IPC client request or IPC Server response.
 *
 * There are three main classes are used in OSAL IPC module, such as IPCBuf, IPCServer and IPCServerBuf
 *
 * @par IPCBuf Class:
 * The IPCBuf class is used by the Client application for sending and receiving the commands.
 * This class provides the following operations,
 * - Send the client message to IPC server.
 * - Client uses TCP socket for communicating with the IPC server.
 * - Receive the response from the IPC server.
 *
 * @par IPCServer Class:
 * The IPCServer class is used by the server application which actually call the HAL APIs.
 * For example POD server will create a server thread with IPCServer instance.
 * When the request is generated from the POD client through the IPCBuf, the IPCServer
 * in turn calls the actual system call and sends the response back through socket.
 *
 * @par IPCServerBuf Class:
 * The IPCServerBuf class is the friend class of IPCServer used for manipulating the buffer
 * and communicate to the client application.  It is having the following operation,
 * - Receive the request from client
 * - Process the request and send the response to the respective client.
 *
 * @ingroup OSAL
 *
 * @defgroup OSAL_IPC_API OSAL IPC API list
 * Describe the details about OSAL Inter Process Communication (IPC) API specifications.
 * @ingroup OSAL_IPC
 *
 * @defgroup OSAL_IPC_CLASSES OSAL IPC Classes
 * Describe the details about classes used in IPC Module such as IPCBuf, IPCServer and IPCServerBuf.
 * @ingroup OSAL_IPC
 */


#include <stdint.h>
//#include <pthread.h>
#include "rmf_osal_thread.h"
#include "rmf_osal_sync.h"

//#include "vlMutex.h"

/* Including payload and protocol bytes  */
#define IPC_MAX_BUF_LEN 2048

/* Major version.Minor version 1.0 */
#define IPC_VERSION 0x10

/*
|version | command | payload length | payload |

version = 1 byte
command = 4 bytes
payload lenght = 2 bytes
payload = IPC_MAX_BUF_LEN (optional)
*/

#define IPC_VERSION_SIZE sizeof( uint8_t )
#define IPC_COMMAND_SIZE sizeof( uint32_t )
#define IPC_PAYLOAD_LEN_SIZE sizeof( uint16_t )
#define IPC_BUF_SIZE ( IPC_VERSION_SIZE + IPC_COMMAND_SIZE + IPC_PAYLOAD_LEN_SIZE + IPC_MAX_BUF_LEN )

#define IPC_HALT_SERVER 0xFFFFFFFF

/**
 * @brief This class is used by the Client application for sending and receiving the commands.
 * @ingroup OSAL_IPC_CLASSES
 */
class IPCBuf{ 

	private:
		uint8_t recvBuff[ IPC_BUF_SIZE  ];
		uint8_t sendBuff[ IPC_BUF_SIZE ];
		uint16_t send_payload_len, recvd_payload_len, to_be_collected;
		bool receivable;
		//vlMutex m_lock;
		

	public: 
		IPCBuf( uint32_t command );
		~IPCBuf( );
		int32_t addData( const void* data, uint16_t len );
		//int8_t send( uint32_t port );
		int32_t sendRecv ( uint32_t port );
		int32_t getResponse(uint32_t *pResponse);
		int32_t collectData( void* data, uint16_t len );
		int8_t sendCmd( uint32_t port );
		void clearBuf( );
};

/**
 * @brief This class is the friend class of IPCServer used for processing the client request
 * and send response to client.
 * @ingroup OSAL_IPC_CLASSES
 */
class IPCServerBuf{

	private:
		uint8_t recvBuff[ IPC_BUF_SIZE  ];
		uint8_t sendBuff[ IPC_BUF_SIZE ];
		uint16_t send_payload_len, recvd_payload_len, to_be_collected;
		bool sendable;
		int32_t response_fd, connection_fd;
		//vlMutex m_lock;
		uint32_t Cmd;
		IPCServerBuf( int32_t response_fd );
		int32_t recvCmd ( );
		friend class IPCServer;
		
	public: 
		~IPCServerBuf( );
		uint32_t getCmd ( );
		uint16_t getDataLen( );
		int32_t collectData( void* data, uint16_t len );
		int32_t addResponse ( uint32_t response );
		int32_t addData( const void* data, uint16_t len );
		int32_t sendResponse ( );
		void clearBuf( );
};

/**
 * @brief This class is used by the server application which actually call the HAL APIs and send response to client.
 * @ingroup OSAL_IPC_CLASSES
 */
class IPCServer{
	uint32_t port;
	bool active;
	rmf_osal_ThreadId threadId;
	//pthread_t threadId;
	int8_t *name;
	//vlMutex m_lock;
	bool thread_exited;
	uint8_t sendBuff[ IPC_BUF_SIZE ];
	void *pUserData;
	void (*pCallBk) ( IPCServerBuf *, void *pUserData);
	rmf_osal_Cond StartCond;
public:
	IPCServer( int8_t *name, uint32_t port );
	~IPCServer();
	int8_t registerCallBk( void (*fun) ( IPCServerBuf *, void *pUserData), void *pUserData );
	static void threadFn(void *param);
	int8_t start();
	int8_t stop();
	void disposeBuf( IPCServerBuf *pServerBuf );
	void listen_request( void *param );
};


