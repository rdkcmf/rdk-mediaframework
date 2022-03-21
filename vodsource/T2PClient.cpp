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
#include "rmf_osal_util.h"

/**
 * @file T2PClient.cpp
 * @brief Contain APIs Definition for T2PClient
 */

/**
 * @addtogroup vodsourcet2pcapi
 * @{
 * @fn T2PClient::T2PClient()
 * @brief A default constructor which will initialize all the member variables required for socket
 * creation and reads the environment variable "MEDIASTREAMER.VOD_CLIENT.MAX_RECV_TIMEOUT" from the configuration
 * file, if not present it will be default set to 20 seconds.
 *
 * @return None
 */
T2PClient::T2PClient()
{
	sockfd = -1;
	conn_open = 0;
	host_ip = -1;
	port = -1;
	memset(&server_addr, 0, sizeof(server_addr));
	const char* maxRecvTimeout = rmf_osal_envGet( "MEDIASTREAMER.VOD_CLIENT.MAX_RECV_TIMEOUT" );
	if ( NULL !=  maxRecvTimeout )
	{
		m_maxRecvTimeout = atoi(maxRecvTimeout); 
	}
	else
	{
		m_maxRecvTimeout = 20;
	}
	RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","T2PClient::T2PClient: Receive timeout: %d\n", m_maxRecvTimeout);
}

/**
 * @fn T2PClient::~T2PClient()
 * @brief A default destructor.
 *
 * @return None
 */
T2PClient::~T2PClient()
{
	//if (conn_open)
		//close(sockfd);
}

/**
 * @fn int T2PClient::openConn(char* host_ip, int port)
 * @brief This function is used to open a new connection to Host IP and Port number.
 *
 * This function will set the host_ip and port number to a global structure and make the variable
 * conn_open to 1. It will returns an error if the connection is already opened by some other
 * application. Following are the applications which uses this API such as rmfproxyservice,
 * VOD Source and Media Streamer.
 *
 * @param[in] host_ip IP Address of a remote host for which connection need to be established.
 * @param[in] port port number for which identifying a process listening on a remove host.
 *
 * @return Returns 0 on success and returns -1 in case connection is already exist between
 * client and server.
 */
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

/**
 * @fn int T2PClient::sendMsgSync(char *sendMsg, int sendMsgSz, char *respMsg, int respMsgBufSz, int newTimeout)
 * @brief This function is used to send a message and receive response from Client. This is
 * currently used by VOD client.
 *
 * This uses simple TCP Socket for establishing the connection to
 * the host. Application need to ensure that before calling this function
 * T2PClient::openConn(char* host_ip, int port) has to be called for setting the IP address and
 * port number for which data need to be sent. The type of message could be Play start, Play Stop,
 * Get Play Position, Get VOD Server ID, Auto discovery, etc. Currently following applications are
 * using this API such as VOD source, T2PServer, RMF Proxy service and Media Streamer.
 * 
 * The function will take a new buffer of size 1024 bytes, put the json header on first 16 bytes and
 * copy the input buffer after the header. The header contains 3 bytes for protocol, 1 byte for version,
 * 4 bytes for unique id, 4 bytes of request type (0x1000) and 4 bytes to keep the length of the data.
 *
 * @param[in] sendMsg buffer that need to be sent to the server.
 * @param[in] sendMsgSz length of the buffer need to be sent.
 * @param[out] respMsg reply message from server to be stored after subtracting the header part.
 * @param[out] respMsgBufSz currently it is not in use.
 * @param[in] newTimeout Send timeout value in seconds.
 *
 * @return Returns payload length in bytes indicate how much data received after substracting the json header from it.
 * It returns -1 on failure and the reason could be that the connection could not be established, send
 * or received timed out during communication with VOD Client, etc.
 */
int T2PClient::sendMsgSync(char *sendMsg, int sendMsgSz, char *respMsg, int respMsgBufSz, int newTimeout)
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

	struct timeval timeout;      
	timeout.tv_sec = (newTimeout == 0) ? m_maxRecvTimeout : newTimeout;
	timeout.tv_usec = 0;
	if (setsockopt (sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,  sizeof(timeout)) < 0)
	{
		close(sockfd);
		RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","T2PClient::sendMsgSync: Failed to set socket timeout for send!\n");
		return -1;
	}
	RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","T2PClient::sendMsgSync: Send timeout set...\n");

	timeout.tv_sec = (newTimeout == 0) ? m_maxRecvTimeout : (newTimeout == 1) ? 0 : newTimeout;
	timeout.tv_usec = 5000;
	if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
	{
		close(sockfd);
		RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","T2PClient::sendMsgSync: Failed to set socket timeout for send!\n");
		return -1;
	}
	RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","T2PClient::sendMsgSync: SO_RCVTIMEO=%ld\n", timeout.tv_sec);

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

	memcpy ((void *)&t2p_send_buf[16], (const void *)sendMsg, sendMsgSz );

     RDK_LOG(RDK_LOG_DEBUG,"LOG.RDK.VODSRC","T2PClient::sendMsgSync: Request dump:");
     if(rdk_dbg_enabled("LOG.RDK.VODSRC", RDK_LOG_DEBUG))
     {
	    hexDump(t2p_send_buf, sendMsgSz+16);
     }
	//hexDump(t2p_send_buf, sendMsgSz);

    // Send the message to the server
    bytesSent = send(sockfd, t2p_send_buf, sendMsgSz+16, 0);
    RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","T2PClient::sendMsgSync: bytesSent %d sendMsgSz %d\n",bytesSent,sendMsgSz+16);
    if (bytesSent != sendMsgSz+16)
    {
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","T2PClient::sendMsgSync: Sent less than requested\n");
		close(sockfd);
        return -1;
    }

	RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","T2PClient::sendMsgSync: Waiting for response...\n");

    // Receive response header
    memset(t2p_rcv_buf, 0, sizeof(t2p_rcv_buf));
    bytesRcvd = recv(sockfd, t2p_rcv_buf, 16, MSG_WAITALL);

	 if (bytesRcvd > 0)
	 {
		 RDK_LOG(RDK_LOG_DEBUG,"LOG.RDK.VODSRC","T2PClient::sendMsgSync: Response header dump:");
         if(rdk_dbg_enabled("LOG.RDK.VODSRC", RDK_LOG_DEBUG))
         {
            hexDump(t2p_rcv_buf, bytesRcvd);
         }
	 }


    if (bytesRcvd <= -1)
    {
		if (timeout.tv_sec == 0)
		{
			RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","T2PClient::sendMsgSync: Not waiting for response\n");
		}
		else
		{
			RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.VODSRC","T2PClient::sendMsgSync: Failed to receive header: Error!\n");
		}
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

    if((0 > payloadLength) || (payloadLength > (sizeof(t2p_rcv_buf) -16)))
    {
         RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","T2PClient::sendMsgSync: payload legth is [%ld] is out of bufer limit", payloadLength );
         close(sockfd);
         return -1;
    }

    // Receive response payload
    bytesRcvd = recv(sockfd, t2p_rcv_buf+16, payloadLength, MSG_WAITALL);

	 if (bytesRcvd > 0)
	 {
		  RDK_LOG(RDK_LOG_DEBUG,"LOG.RDK.VODSRC","T2PClient::sendMsgSync: Response payload dump:");
          if(rdk_dbg_enabled("LOG.RDK.VODSRC", RDK_LOG_DEBUG))
          {
	        hexDump(t2p_rcv_buf+16, bytesRcvd);
          }
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

/**
 * @fn int T2PClient::closeConn()
 * @brief This function is used to close the existing connection with VOD Client.
 * This will set conn_open value to 0. Any application has opened the connection should close
 * the connection at the last. Following applications are using this api such as, VOD source,
 * T2PServer, RMF Proxy service and Media Streamer.
 *
 * @return Returns 0 always.
 */
int T2PClient::closeConn()
{
	conn_open = 0;
	return 0;
}

/**
 * @fn void T2PClient::hexDump(char *buf, int buf_len)
 * @brief A test routine for debugging. It will print the first 16 bytes of header in hex format
 * and data part in character format.
 *
 * @param[in] buf buffer where messages are stored
 * @param[in] buf_len length of the buffer
 *
 * @return None
 * @}
 */
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


