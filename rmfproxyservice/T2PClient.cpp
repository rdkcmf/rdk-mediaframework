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
#include "T2PClient.h"
#include <string>
//#include <vlMemCpy.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "rdk_debug.h"
#include "rmf_osal_mem.h"



// Constructor
T2PClient::T2PClient()
{
	sockfd = -1;
	conn_open = 0;
	host_ip = -1;
	port = -1;
	memset(&server_addr, 0, sizeof(server_addr));
}

// Destructor
T2PClient::~T2PClient()
{
	//if (conn_open)
		//close(sockfd);
}


// open connection
int T2PClient::openConn(char* host_ip, int port)
{

	if (conn_open)
	{
		RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","T2PClient::open: Already open!\n");
		return -1;
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family	= AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(host_ip);
	server_addr.sin_port = htons(port);

	conn_open = 1;


	return 0;

}

// Send a message and receive response
int T2PClient::sendMsgSync(char *sendMsg, int sendMsgSz, char *respMsg, int respMsgBufSz)
{
	int bytesSent = 0;
	int bytesRcvd = 0;
	const int msg_buf_sz = 1024;
	char t2p_send_buf[msg_buf_sz];
	char t2p_rcv_buf[msg_buf_sz];


	if(!conn_open)
	{
		RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","T2PClient::sendMsgSync: Connection not open!\n");
		return -1;
	}

	// Create a TCP socket
	sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd < 0)
	{
		RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","T2PClient::sendMsgSync: Failed to open socket!\n");
		return -1;
	}


	// Connect
	if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
	{
		close(sockfd);
		RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","T2PClient::sendMsgSync: Failed to connect!\n");
		return -1;
	}


	// Write header
	// protocol
	t2p_send_buf[0] = 't';
	t2p_send_buf[1] = '2';
	t2p_send_buf[2] = 'p';

	// Version
	t2p_send_buf[3] = 0x01;

	// Message id: Unique id
	t2p_send_buf[4] = 's';
	t2p_send_buf[5] = 'e';
	t2p_send_buf[6] = 'r';
	t2p_send_buf[7] = 'v';

	// Type : Request type = 0x1000
	t2p_send_buf[8] = 0x00;
	t2p_send_buf[9] = 0x00;
	t2p_send_buf[10] = 0x10;
	t2p_send_buf[11] = 0x00;

	// Length : Length of payload
	t2p_send_buf[12] = (sendMsgSz & 0xFF000000) >> 24;
	t2p_send_buf[13] = (sendMsgSz & 0x00FF0000) >> 16;
	t2p_send_buf[14] = (sendMsgSz & 0x0000FF00) >> 8;
	t2p_send_buf[15] = (sendMsgSz & 0x000000FF);

	rmf_osal_memcpy ((void *)&t2p_send_buf[16], (const void *)sendMsg, sendMsgSz, 
		msg_buf_sz-16, sendMsgSz );

     RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","T2PClient::sendMsgSync: Request dump:");
	hexDump(t2p_send_buf, sendMsgSz+16);
	//hexDump(t2p_send_buf, sendMsgSz);

    // Send the message to the server
    bytesSent = send(sockfd, t2p_send_buf, sendMsgSz+16, 0);
    RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","T2PClient::sendMsgSync: bytesSent %d sendMsgSz %d\n",bytesSent,sendMsgSz+16);
    if (bytesSent != sendMsgSz+16)
    {
         RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","T2PClient::sendMsgSync: Sent less than requested");
		close(sockfd);
        return -1;
    }

    // Receive response header
    bytesRcvd = recv(sockfd, t2p_rcv_buf, 16, MSG_WAITALL);

	 if (bytesRcvd > 0)
	 {
		  RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","T2PClient::sendMsgSync: Response header dump:");
	     hexDump(t2p_rcv_buf, bytesRcvd);
	 }


    if (bytesRcvd <= -1)
    {
         RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","T2PClient::sendMsgSync: Failed to receive header: Error!");
		close(sockfd);
        return -1;
    }
    else if (bytesRcvd == 0)
    {
         RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","T2PClient::sendMsgSync: Failed to receive header: Connection closed by remote server!");
		close(sockfd);
        return -1;
    }
    else if ((bytesRcvd < 16) || (t2p_rcv_buf[0] != 't') || (t2p_rcv_buf[1] != '2') ||
    		(t2p_rcv_buf[2] != 'p'))
    {
         RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","T2PClient::sendMsgSync: Invalid t2p message received from remote server!");
		close(sockfd);
        return -1;
    }

    // Get the length field
    const int payloadLength = ((unsigned char)t2p_rcv_buf[12] << 24) | ((unsigned char)t2p_rcv_buf[13] << 16) | ((unsigned char)t2p_rcv_buf[14] << 8) | (unsigned char)t2p_rcv_buf[15];


	  RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","T2PClient::sendMsgSync: Waiting for payload of size %d\n", payloadLength);

    // Receive response payload
    bytesRcvd = recv(sockfd, t2p_rcv_buf+16, payloadLength, MSG_WAITALL);

	 if (bytesRcvd > 0)
	 {
		  RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","T2PClient::sendMsgSync: Response payload dump:");
	     hexDump(t2p_rcv_buf+16, bytesRcvd);
	 }


    if (bytesRcvd <= -1)
    {
         RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","T2PClient::sendMsgSync: Failed to receive payload: Error!");
		close(sockfd);
        return -1;
    }
    else if (bytesRcvd == 0)
    {
         RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","T2PClient::sendMsgSync: Failed to receive payload: Connection closed by remote server!");
		close(sockfd);
        return -1;
    }

	memcpy ((void *)respMsg, (const void *)&t2p_rcv_buf[16], payloadLength );

	close(sockfd);
/*Response:
                        {
                        "vodPlayResponse" : {
                        <ResponseStatus>,
                        "locator": Locator
                        "sessionId": session Id
                        }
                        }

*/
/*
	 static int session_id = 0;
	 sprintf(respMsg, "{ \"vodPlayResponse\" : { \"ResponseStatus\": 0, \"locator\": %s, \"sessionId\": %d }}\n", "\"ocap://f=0x205D85C0.0x1.m=0x10\"",++session_id);

	int payloadLength = strlen(respMsg);
*/
	return payloadLength;
}

// close connection
int T2PClient::closeConn()
{
	conn_open = 0;
	return 0;
}

// Hex dump
void T2PClient::hexDump(char *buf, int buf_len)
{

	long int       addr;
	long int       count;
	long int       iter = 0;

	printf ("\n");
	addr = 0;

	while (1)
	{
		// Print address
		printf ("%5d %08lx  ", (int) addr, addr );
		addr = addr + 16;

		// Print hex values
		for ( count = iter*16; count < (iter+1)*16; count++ ) {
			if ( count < buf_len ) {
				printf ( "%02x", (unsigned char)buf[count] );
			}
			else {
				printf ( "  " );
			}
			printf ( " " );
		}

		printf ( " " );
		for ( count = iter*16; count < (iter+1)*16; count++ ) {
			if ( count < buf_len ) {
				if ( ( buf[count] < 32 ) || ( buf[count] > 126 ) ) {
					printf ( "%c", '.' );
				}
				else {
					printf ( "%c", buf[count] );
				}
			}
			else
			{
				printf("\n\n");
				return;
			}
		}

		printf ("\n");

		iter++;
	}
}


