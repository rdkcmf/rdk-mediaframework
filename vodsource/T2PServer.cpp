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
 * @file T2PServer.c
 * @brief The source file contains the definition of public member functions of class T2PServer
 */
#include "T2PServer.h"
#include "T2PClient.h"
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>     /* for memset() */
#include "rdk_debug.h"
#ifdef TEST_WITH_PODMGR
#include "rmf_pod.h"
#else
#include <rmf_osal_types.h>
#include <rmf_osal_error.h>
#include <rmf_osal_event.h>
#endif

#define VOD_CLIENT_IP "127.0.0.1"
#define VOD_CLIENT_PORT 11000


char registerString[]= "                           \
{  \"command\" : \"register\",                     \
   \"id\"      : \"12345\",                        \
   \"item\"    :                                   \
          { \"type\"        :   \"DVRAsset\",      \
            \"attributes\"  : [                    \
                { \"name\"  : \"locator\",         \
                  \"value\" : \"dvr://0x123?0x456\"\
                },                                 \
                { \"name\"  : \"title\",           \
                  \"value\" : \"Title Goes Here\"  \
                }                                  \
                { \"name\"  : \"description\",     \
                  \"value\" : \"Description goes here\"  \
                }                                  \
              ]                                    \
           }                                       \
 }";

char unregisterString[]= "       \
{  \"command\" : \"unregister\", \
   \"id\"   :     \"12345\"      \
}" ;

char replyString[]= "                             \
{  \"command\"       : \"unregister\",            \
   \"resultCode\"    : \"abc\",                   \
   \"description\"   : \"Reply description\"      \
}" ;

/**
 * @addtogroup vodsourcet2pserverapi
 * @{
 * @fn T2PServer::T2PServer()
 * @brief A constructor for class T2PServer which will initialize its member variables.
 *
 * @return None
 */
T2PServer::T2PServer() 
{
	servSock = -1;
	serv_open = false;
	fMsgHdlr = NULL;
	done = false;
	m_context = NULL;
	t2p_rcv_buf[0]='\0';
	t2p_send_buf[0]='\0';
}

/**
 * @fn T2PServer::~T2PServer()
 * @brief A destructor which will close the server socket if it is opened.
 *
 * @return None
 */
T2PServer::~T2PServer()
{
	done = true;
	sleep (1);
	if (serv_open)
		close(servSock);
}

/**
 * @fn int T2PServer::messageHandler(char *recvMsg, int recvMsgSz, char *respMsg, int *respMsgBufSz)
 * @brief A handler function which is used to send and receive messages with the help of
 * a registered callback. The function will pass the message and will receive the response through the
 * callback function.
 *
 * @param[in] recvdMsg a buffer that received from the client.
 * @param[in] recvMsgSz length of the buffer.
 * @param[out] respMsg  response message with header part.
 * @param[out] respMsgBufSz length of the response message buffer.
 *
 * @return Returns 0 on success, -1 otherwise.
 */
int T2PServer::messageHandler(char *recvMsg, int recvMsgSz, char *respMsg, int *respMsgBufSz)
{

	if (fMsgHdlr != NULL)
	{
		return (*fMsgHdlr)(recvMsg, recvMsgSz, respMsg, respMsgBufSz,m_context);
	}

	return -1;
}

/**
 * @fn int T2PServer::openServer(char* t2pServAddrS, unsigned short t2pServPort, t2pMsgHandler pfMsgHdlr,void* context)
 * @brief This function is used to setup a server connection by using TCP socket.
 * It will bind the connection to the supplied port
 * number and will listen to incoming connection from any of the interfaces.
 *
 * @param[in] t2pServAddrS interface IP address which is not used currently.
 * @param[in] t2pServPort port number on which server will listen.
 * @param[in] pfMsgHdlr callback function for message handler.
 * @param[in] context user data.
 *
 * @return Returns 0 on success, -1 otherwise
 */
int T2PServer::openServer(char* t2pServAddrS, unsigned short t2pServPort, t2pMsgHandler pfMsgHdlr,void* context)
{
    struct sockaddr_in t2pServAddr; /* Local address */

    m_context = context;
    if (pfMsgHdlr == NULL)
    {
    	return -1;
    }
    else
    {
    	fMsgHdlr = pfMsgHdlr;
    }

    if ((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
         RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","socket() failed\n");
        return -1;
    }

    memset(&t2pServAddr, 0, sizeof(t2pServAddr));   /* Zero out structure */
    t2pServAddr.sin_family = AF_INET;                /* Internet address family */

	 //printf ("WARNING! Overriding the specified server IP address %s and using INADDR_ANY\n", t2pServAddrS);
    t2pServAddr.sin_addr.s_addr = htonl(INADDR_ANY); //inet_addr(t2pServAddrS); /* Loopback interface */
    t2pServAddr.sin_port = htons(t2pServPort);      /* Local port */

    int one = 1;
    if(setsockopt(servSock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0)
    {
	perror("setsockopt");
	RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","setsockopt failed\n");
	return -1;
    }


    if (bind(servSock, (struct sockaddr *) &t2pServAddr, sizeof(t2pServAddr)) < 0)
    {
        perror("bind");
         RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","bind() failed\n");
		close(servSock);

        return -1;
    }

    if (listen(servSock, 4) < 0)
    {
         RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","listen() failed\n");
		close(servSock);

        return -1;
    }

    serv_open = true;

    return 0;
}

/**
 * @fn void run(void* arg)
 * @brief This is a thread function for listening the card IP acquired events from POD manager,
 * In case of card IP acquired, the auto discovery process will be initated by this thread.
 * This will create an event queue named "SitpSi" for getting events from POD manager.
 * After IP is acquired, this will open a connection for sending Sync messages otherwise
 * the thread will wait in loop for IP acquired. This runs as a loop forever to receive connection
 * from T2PClient and send the response.
 *
 * @param[in] arg thread argument which is stored in server context.
 *
 * @return None
 */
void run(void* arg)
{
    struct sockaddr_in t2pClntAddr; /* Client address */
    unsigned int clntLen;            /* Length of client address data structure */
    int clntSock;                    /* Socket descriptor for client */
    int bytesRcvd;                    /* Size of received message */
    T2PServer* servContext = (T2PServer*)arg;
    

	sleep(5);
    
        // Listen for Card IP Acquired. If acquired go ahead and trigger auto discovery

        //Create Event Queue
	uint8_t name[100];
        rmf_osal_eventqueue_handle_t pod_queue;
	rmf_osal_event_handle_t pod_event_handle;
	uint32_t pod_event_type;
	rmf_osal_event_params_t event_params = {0};

	snprintf( (char*)name, 100, "SitpSi");

        if(RMF_SUCCESS != rmf_osal_eventqueue_create (name,&pod_queue))
        {
        RDK_LOG(
                RDK_LOG_ERROR,
                "LOG.RDK.VODSRC",
                "%s:: unable to create POD event queue,\n",
                 __FUNCTION__);

        }
#ifdef TEST_WITH_PODMGR
        //Register Event Queue
        rmf_podmgrRegisterEvents(pod_queue);
#endif

        //Wait for Card Acquired Event

      	rmf_osal_Bool IpAcquired = false;
	const int msgBufSz = 1024;
	char t2p_resp_str[msgBufSz];
	char t2p_req_str[msgBufSz];

	sprintf(t2p_req_str, "{ \"TriggerAutoDiscovery\" : {}}\n");

	uint8_t podmgrDSGIP[ 20 ] ;
#ifdef TEST_WITH_PODMGR
	if ( RMF_SUCCESS  == rmf_podmgrIsReady() )
	{
		IpAcquired = rmf_podmgrGetDSGIP( podmgrDSGIP );	
	}
#endif

	if (IpAcquired )
	{
		RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","DSG IP Already acquired start auto discovery\n");
		T2PClient *t2pclient = new T2PClient();
            	if (t2pclient->openConn(VOD_CLIENT_IP, VOD_CLIENT_PORT) == 0)
            	{
                    int recvdMsgSz = 0;
                    t2p_resp_str[0] = '\0';
                    if ((recvdMsgSz = t2pclient->sendMsgSync(t2p_req_str, strlen(t2p_req_str), t2p_resp_str, msgBufSz)) > 0)
                    {
                            t2p_resp_str[recvdMsgSz] = '\0';
                            RDK_LOG(RDK_LOG_TRACE1,"LOG.RDK.VODSRC","AutoDiscovery::Start(): Response : %s\n", t2p_resp_str);

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
	}
	else
	{
        	while(!IpAcquired)
	        {
		
		RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","%s:: getting next event from POD queue\n",__FUNCTION__);


	         if(RMF_SUCCESS != rmf_osal_eventqueue_get_next_event (pod_queue, &pod_event_handle, NULL, &pod_event_type, &event_params))
	            {
	                RDK_LOG(
	                        RDK_LOG_INFO,
	                        "LOG.RDK.VODSRC",
	                        "%s - POD Queue event get failed...\n",
	                         __FUNCTION__);
	            }
	            else
	            {
	                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.VODSRC",
	                        "%s - recv'd podEvent = %d\n",
	                         __FUNCTION__, pod_event_type);

#ifdef TEST_WITH_PODMGR
	                if (pod_event_type == RMF_PODMGR_EVENT_DSGIP_ACQUIRED)
	                {
	                        IpAcquired = true;
	        		rmf_podmgrUnRegisterEvents(pod_queue);
	                        // Create a T2PClient
	                        

	                        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","AutoDiscovery::Start():%s length %d\n",t2p_req_str,strlen(t2p_req_str));
	                        T2PClient *t2pclient = new T2PClient();
	                        if (t2pclient->openConn(VOD_CLIENT_IP, VOD_CLIENT_PORT) == 0)
	                        {
	                                int recvdMsgSz = 0;
	                                t2p_resp_str[0] = '\0';
	                                if ((recvdMsgSz = t2pclient->sendMsgSync(t2p_req_str, strlen(t2p_req_str), t2p_resp_str, msgBufSz)) > 0)
	                                {
	                                        t2p_resp_str[recvdMsgSz] = '\0';
	                                        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","AutoDiscovery::Start(): Response : %s\n", t2p_resp_str);

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

	                }
#endif
	                rmf_osal_event_delete(pod_event_handle);
	            }
	        }
		}
    if (!servContext->serv_open)
    {
         RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.VODSRC","server not open\n");
        return;
    }

    while (!servContext->done) /* Run forever */
    {
        /* Set the size of the in-out parameter */
        clntLen = sizeof(t2pClntAddr);

         RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","T2PServer::run(): Waiting for connections\n");

        /* Wait for a client to connect */
        if ((clntSock = accept(servContext->servSock, (struct sockaddr *) &t2pClntAddr,
                               &clntLen)) < 0)
        {
             RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","accept() failed\n");
            sleep(1);
            continue;
        }

        /* clntSock is connected to a client! */

         RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","Handling client %s\n", inet_ntoa(t2pClntAddr.sin_addr));

        /* Receive message from client */

        // Receive response header
         bytesRcvd = recv(clntSock, servContext->t2p_rcv_buf, 16, MSG_WAITALL);

     	 if (bytesRcvd > 0)
     	 {
     		  RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","T2PServer::run: Request header dump:");
     	     servContext->hexDump(servContext->t2p_rcv_buf, bytesRcvd);
     	 }


         if (bytesRcvd < -1)
         {
              RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","T2PServer::run: Failed to receive header: Error!");
				 close(clntSock);
             continue;
         }
         else if (bytesRcvd == 0)
         {
              RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","T2PServer::run: Failed to receive header: Connection closed by remote server!");
				 close(clntSock);
             continue;
         }
         else if ((bytesRcvd < 16) || (servContext->t2p_rcv_buf[0] != 't') || (servContext->t2p_rcv_buf[1] != '2') ||
         		(servContext->t2p_rcv_buf[2] != 'p'))
         {
              RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","T2PServer::run: Invalid t2p message received from remote client!");
				 close(clntSock);
             continue;
         }

         // Get the length field
         int payloadLength = ((servContext->t2p_rcv_buf[12] << 24) & 0xFF000000) | ((servContext->t2p_rcv_buf[13] << 16)  & 0x00FF0000) | 
		        ((servContext->t2p_rcv_buf[14] << 8) & 0x0000FF00) | (servContext->t2p_rcv_buf[15] & 0x000000FF);


         // Receive response payload
         bytesRcvd = recv(clntSock, servContext->t2p_rcv_buf+16, payloadLength, MSG_WAITALL);

     	 if (bytesRcvd > 0)
     	 {
     		  RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","T2PServer::run: bytesRcvd = %d payloadLength = %d Request payload dump:", bytesRcvd, payloadLength);
     	     servContext->hexDump(servContext->t2p_rcv_buf+16, bytesRcvd);
     	 }


         if (bytesRcvd < -1)
         {
              RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","T2PServer::run: Failed to receive payload: Error!");
				 close(clntSock);
             continue;
         }
         else if (bytesRcvd == 0)
         {
              RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","T2PServer::run: Failed to receive payload: Connection closed by remote client!");
				 close(clntSock);
             continue;
         }

         int sendMsgSz = SNDBUFSIZE - 16;
         servContext->messageHandler(&servContext->t2p_rcv_buf[16], payloadLength, &servContext->t2p_send_buf[16], &sendMsgSz);


         /* Send response */

         // Write header
         // protocol
         servContext->t2p_send_buf[0] = 't';
         servContext->t2p_send_buf[1] = '2';
         servContext->t2p_send_buf[2] = 'p';

         // Version
         servContext->t2p_send_buf[3] = 0x01;

         // Message id: Unique id
         servContext->t2p_send_buf[4] = 's';
         servContext->t2p_send_buf[5] = 'e';
         servContext->t2p_send_buf[6] = 'r';
         servContext->t2p_send_buf[7] = 'v';

         // Type : Request type = 0x1000
         servContext->t2p_send_buf[8] = 0x00;
         servContext->t2p_send_buf[9] = 0x00;
         servContext->t2p_send_buf[10] = 0x10;
         servContext->t2p_send_buf[11] = 0x00;

         // Length : Length of payload
         servContext->t2p_send_buf[12] = (sendMsgSz & 0xFF000000) >> 24;
         servContext->t2p_send_buf[13] = (sendMsgSz & 0x00FF0000) >> 16;
         servContext->t2p_send_buf[14] = (sendMsgSz & 0x0000FF00) >> 8;
         servContext->t2p_send_buf[15] = (sendMsgSz & 0x000000FF);

         /* Send message back to client */
          RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","T2PServer::run: send dump:");
         servContext->hexDump(servContext->t2p_send_buf, sendMsgSz+16);

         // Send the message to the client
         int bytesSent = send(clntSock, servContext->t2p_send_buf, sendMsgSz+16, 0);
         if (bytesSent != sendMsgSz+16)
         {
             RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC","T2PServer::sendMsgSync: Sent less than requested");
             sleep(1);
             close(clntSock);
             continue;
         }
				 
	close(clntSock); 

    }

}

/**
 * @fn RMFResult T2PServer::start()
 * @brief This function is used to create a thread of Async notifications for VOD Source.
 * The thread will be created with a default priority.
 *
 * @return returns 0 on success, on failure returns a value other than 0.
 */
RMFResult T2PServer::start()
{
	rmf_Error ret;
	ret = rmf_osal_threadCreate (run, this,RMF_OSAL_THREAD_PRIOR_DFLT, RMF_OSAL_THREAD_STACK_SIZE, &tid, "vodsrc_async_notifications_thread");
	if (RMF_SUCCESS != ret )
        {
        	RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.VODSRC", "%s - rmf_osal_thread_create failed.\n", __FUNCTION__);
        }
	return ret;
}

/**
 * @fn void T2PServer::hexDump(char *buf, int buf_len)
 * @brief A test routine for debugging. It will print the first 16 bytes of header in hex format
 * and remaining data part in character format.
 *
 * @param[in] buf data buffer
 * @param[in] buf_len length of buffer
 *
 * @return None
 * @}
 */
void T2PServer::hexDump(char *buf, int buf_len)
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
				 RDK_LOG(RDK_LOG_INFO,"LOG.RDK.VODSRC", "\n\n" );
				return;
			}
		}

		printf ("\n");

		iter++;
	}
}


