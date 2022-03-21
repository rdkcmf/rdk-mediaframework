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
 * @file T2PClient.h
 * @brief It contains APIs declaration for T2PClient
 */

#ifndef T2P_CLIENT_H
#define T2P_CLIENT_H


#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

/**
 * @class T2PClient
 * @brief This class defines methods required for communicating with VOD client by sending request messages such
 * as play, pause, fast forword, rewind, etc. This has several functionalities such as open a TCP connection, send the request by
 * filling the JSON header and close the connection after receving the response from VOD client.
 * The class is used by various application to communicate with VOD client.
 * @ingroup vodsourceclass
 */
class T2PClient
{
public:
	T2PClient();
	~T2PClient();

	int openConn(char * host_ip, int port);
	int sendMsgSync(char *sendMsg, int sendMsgSz, char *respMsg, int respMsgBufSz, int newTimeout=0);
	int closeConn();
	void hexDump(char *buf, int buf_len);

private:
	// lock
	int host_ip; //!< This attribute is obsolute, -1 is stored as default value
	int port; //!< This attribute is obsolute, -1 is stored as default value
	int sockfd; //!< A file descriptor for the new socket
	int conn_open; //!< This attribute is set to 1 when an application creates an instance for this class
	struct sockaddr_in server_addr; //!< A socket structure contains the details about server address such as address family, IP address and port number
	int m_maxRecvTimeout; //!< Receive timeout value in terms of seconds
};

#endif
