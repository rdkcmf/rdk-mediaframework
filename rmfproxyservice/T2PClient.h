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

class T2PClient
{

public:

	// Ctor & Dtor
	T2PClient();
	~T2PClient();


	// open connection
	int openConn(char * host_ip, int port);

	int sendMsgSync(char *sendMsg, int sendMsgSz, char *respMsg, int respMsgBufSz);

	// close connection
	int closeConn();

	void hexDump(char *buf, int buf_len);


private:

	// lock
	int host_ip;
	int port;
	int sockfd;
	int conn_open;
	struct sockaddr_in server_addr;

};

#endif
