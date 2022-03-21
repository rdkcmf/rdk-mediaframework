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
#ifndef T2P_SERVER_H
#define T2P_SERVER_H

/**
 * @file T2PServer.h
 * @brief The header file contains the declaration of public member functions of class T2PServer
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include <errno.h>
#include "rmf_osal_thread.h"
#include "rmfcore.h"


#define RCVBUFSIZE 2048
#define SNDBUFSIZE 2048

typedef int (*t2pMsgHandler)(char *recvMsg, int recvMsgSz, char *respMsg, int *respMsgBufSz,void *context);

/**
 * @class T2PServer
 * @brief The class defines the functionalities required for handling the request from VOD Client.
 * The application sends an Auto discovery request to VOD client using T2PClient, but response from VOD Client will be
 * received through T2PServer after sometime with a separate connection. The ariaval message is processed by messageHandler()
 * and provide the information to the application. There are functionalities involved for this class, such as
 * create a TCP socket, start the server application by listening on a port, accept the new connection
 * from VOD client and handle the request message by extracting JSON header. There is a debugging function hexDump(),
 * which is used for printing the header and data part of message arrived from VOD Client.
 * @ingroup vodsourceclass
 */
class T2PServer 
{

public:

	// Ctor & Dtor
	T2PServer();
	~T2PServer();


	// open connection
	int openServer(char* t2pServAddrS, unsigned short t2pServPort, t2pMsgHandler pfMsgHdlr,void *context);

	int messageHandler(char *recvMsg, int recvMsgSz, char *respMsg, int *respMsgBufSz);



	RMFResult start();

	void hexDump(char *buf, int buf_len);



	// lock
	bool serv_open; //!< This attribute set to True means socket, bind and listen successful
	int servSock; //!< Socket descriptor
	bool done; //!< This attribute will set to True when instance of this class created

    char t2p_rcv_buf[RCVBUFSIZE];        /**< Buffer for recv string */
    char t2p_send_buf[SNDBUFSIZE];        /**< Buffer for send string */

    rmf_osal_ThreadId tid; //!< Thread Id for server function handler
    t2pMsgHandler fMsgHdlr; //!< A function callback for handling the message
    void* m_context;

};

#endif
