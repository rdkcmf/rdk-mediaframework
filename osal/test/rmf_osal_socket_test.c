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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "rmf_osal_sync.h"
#include "rmf_osal_thread.h"
#include "rmf_osal_socket.h"

#define RMF_OSAL_ERROR_SERVER(msg) {\
		printf("Server : %s:%d : %s : %s\n",__FUNCTION__,__LINE__,msg, strerror(rmf_osal_socketGetLastError())); \
		abort(); \
}

#define RMF_OSAL_ERROR_CLIENT(msg) {\
		printf("Client : %s:%d : %s : %s\n",__FUNCTION__,__LINE__,msg, strerror(rmf_osal_socketGetLastError())); \
		abort(); \
}

#define BUF_LEN 256

int rmf_osal_socket_test()
{
	rmf_osal_Socket sockfd, newsockfd;
	int portno;
	rmf_osal_SocketSockLen clilen;
	char buffer[BUF_LEN];
	char hostname[BUF_LEN];
	rmf_osal_SocketIPv4SockAddr serv_addr, cli_addr;
	int n, ret;
	rmf_osal_SocketHostEntry *server;
	rmf_osal_SocketFDSet fdset, read_fds;
	rmf_osal_SocketNetIfList *netIfList;

	ret = rmf_osal_socketGetInterfaces(&netIfList);
	if (ret != RMF_SUCCESS)
	{
		RMF_OSAL_ERROR_SERVER("ERROR on rmf_osal_socketGetInterfaces");
	}
	printf("rmf_osal_socketGetInterfaces Success\n");
	while ( netIfList != NULL)
	{
		ret = rmf_osal_socket_getMacAddress(netIfList->if_name, buffer);
		if (ret <0 )
		{
			RMF_OSAL_ERROR_SERVER("ERROR on rmf_osal_socket_getMacAddress");
		}
		printf("rmf_osal_socket_getMacAddress for %s (index %d)Success :%s\n", 
			netIfList->if_name, netIfList->if_index, buffer);
		netIfList = netIfList->if_next;
	}

	rmf_osal_socketFreeInterfaces( netIfList);

	ret = rmf_osal_socketGetHostName(hostname, BUF_LEN);
	if (ret <0 )
	{
		RMF_OSAL_ERROR_SERVER("ERROR on rmf_osal_socketGetHostName");
	}
	printf("rmf_osal_socketGetHostName Success %s\n", hostname);

	memset((char *) &serv_addr, 0,sizeof(serv_addr));
	portno = 1010;

	pid_t pid = fork();
	if (pid < 0 )
	{
		RMF_OSAL_ERROR_SERVER("fork failed");
	}
	else if (pid == 0)
	{
		sockfd = rmf_osal_socketCreate( RMF_OSAL_SOCKET_AF_INET4, RMF_OSAL_SOCKET_STREAM, 0);
		if (sockfd == RMF_OSAL_SOCKET_INVALID_SOCKET)
		{
			RMF_OSAL_ERROR_CLIENT("rmf_osal_socketCreate error");
		}
		printf("Client: rmf_osal_socketCreate Success\n");

		server = rmf_osal_socketGetHostByName( hostname);
		if (server == NULL) 
		{
			RMF_OSAL_ERROR_CLIENT("rmf_osal_socketGetHostByName error\n");
		}
		printf("Client: rmf_osal_socketGetHostByName Success\n");

		server = rmf_osal_socketGetHostByAddr( (char *)server->h_addr, 
		     server->h_length, RMF_OSAL_SOCKET_AF_INET4);
		if (server == NULL) 
		{
			RMF_OSAL_ERROR_CLIENT("rmf_osal_socketGetHostByAddr error\n");
		}
		printf("Client: rmf_osal_socketGetHostByAddr Success\n");

		serv_addr.sin_family = RMF_OSAL_SOCKET_AF_INET4;
		memmove( (char *)&serv_addr.sin_addr.s_addr, (char *)server->h_addr, 
		     server->h_length);
		serv_addr.sin_port = rmf_osal_socketHtoNS(portno);
		if (rmf_osal_socketConnect(sockfd,(rmf_osal_SocketSockAddr *) &serv_addr,sizeof(serv_addr)) < 0) 
		{
			RMF_OSAL_ERROR_CLIENT("Client: ERROR connecting\n");
		}
		printf("Client: rmf_osal_socketConnect Success\n");

		strcpy(buffer, "Message from client process -_-"); 
		n = rmf_osal_socketSendTo(sockfd, buffer, strlen(buffer), 0, NULL, 0);
		if (n < 0) 
		{
			RMF_OSAL_ERROR_CLIENT("Client: rmf_osal_socketSendTo error\n");
		}
		printf("Client: rmf_osal_socketSendTo Success\n");

		memset(buffer,0,BUF_LEN);
		n = rmf_osal_socketRecvFrom(sockfd,buffer,BUF_LEN, 0, NULL, 0);
		if (n < 0)
		{
			RMF_OSAL_ERROR_CLIENT("Client: rmf_osal_socketRecvFrom error\n");
		}
		printf("Client - rmf_osal_socketRecvFrom Success: %s\n",buffer);

		for (int i=0; i<10; i++)
		{
			sprintf(buffer, "client-message%d", i); 
			n = rmf_osal_socketSendTo(sockfd, buffer, strlen(buffer), 0, NULL, 0);
			if (n < 0) 
			{
				RMF_OSAL_ERROR_CLIENT("Client: rmf_osal_socketSendTo error\n");
			}
			printf("Client: rmf_osal_socketSendTo Success\n");
			usleep (100);
		}

		strcpy(buffer, "<exit>"); 
		n = rmf_osal_socketSendTo(sockfd, buffer, strlen(buffer), 0, NULL, 0);
		if (n < 0) 
		{
			RMF_OSAL_ERROR_CLIENT("Client: rmf_osal_socketSendTo error\n");
		}
		printf("Client: rmf_osal_socketSendTo Success\n");
		printf("Sleeping 1 seconds.\n");
		sleep (1);

		ret = rmf_osal_socketClose(sockfd);
		if (ret != 0)
		{
			RMF_OSAL_ERROR_CLIENT("Client: rmf_osal_socketClose failed\n");
		}
		printf("Client - rmf_osal_socketClose success - Exit client process\n");
		exit(0);
	}
	else
	{
		sockfd = rmf_osal_socketCreate( RMF_OSAL_SOCKET_AF_INET4, RMF_OSAL_SOCKET_STREAM, 0);
		if (sockfd == RMF_OSAL_SOCKET_INVALID_SOCKET) 
		   RMF_OSAL_ERROR_SERVER("ERROR opening socket");
		printf("Server -rmf_osal_socketCreate Success\n");

		serv_addr.sin_family = RMF_OSAL_SOCKET_AF_INET4;
		serv_addr.sin_addr.s_addr = RMF_OSAL_SOCKET_IN4ADDR_ANY;
		serv_addr.sin_port = rmf_osal_socketHtoNS(portno);
		if (rmf_osal_socketBind( sockfd, (rmf_osal_SocketSockAddr*)&serv_addr,
				sizeof(serv_addr)) < 0) 
		{
			RMF_OSAL_ERROR_SERVER("ERROR on binding");
		}
		printf("Server - rmf_osal_socketBind Success\n");

		ret = rmf_osal_socketListen(sockfd,5);
		if (ret <0 )
		{
			RMF_OSAL_ERROR_SERVER("ERROR on rmf_osal_socketListen");
		}
		printf("Server - rmf_osal_socketListen Success\n");

		clilen = sizeof(cli_addr);
		newsockfd = rmf_osal_socketAccept(sockfd, 
			(rmf_osal_SocketSockAddr *)&cli_addr,  &clilen);
		if (newsockfd < 0) 
		{
			RMF_OSAL_ERROR_SERVER("ERROR on rmf_osal_socketListen");
		}
		printf("Server - rmf_osal_socketAccept Success len =%d sa_family=%d\n",
				clilen, ((rmf_osal_SocketSockAddr*)&cli_addr)->sa_family);

		{ 
			rmf_osal_SocketIPv4SockAddr address;
			rmf_osal_SocketSockLen address_len = sizeof(rmf_osal_SocketIPv4SockAddr);
			ret = rmf_osal_socketGetSockName( newsockfd, 
				(rmf_osal_SocketSockAddr*)&address, &address_len);
			if (ret != 0)
			{
				RMF_OSAL_ERROR_SERVER("Client: rmf_osal_socketGetSockName failed\n");
			}
			printf("Server - rmf_osal_socketGetSockName success len =%d sa_family=%d\n",
				address_len, ((rmf_osal_SocketSockAddr*)&address)->sa_family);
		}

		{ 
			rmf_osal_SocketIPv4SockAddr address;
			rmf_osal_SocketSockLen address_len = sizeof(rmf_osal_SocketIPv4SockAddr);
			ret = rmf_osal_socketGetPeerName( newsockfd, 
				(rmf_osal_SocketSockAddr*)&address, &address_len);
			if (ret != 0)
			{
				RMF_OSAL_ERROR_SERVER("Client: rmf_osal_socketGetPeerName failed\n");
			}
			printf("Server - rmf_osal_socketGetPeerName success len =%d sa_family=%d\n",
				address_len, ((rmf_osal_SocketSockAddr*)&address)->sa_family);
		}

		memset(buffer,0, BUF_LEN);
		n = rmf_osal_socketRecv(newsockfd,buffer,BUF_LEN, 0);
		if (n < 0) 
		{
			RMF_OSAL_ERROR_SERVER("ERROR on rmf_osal_socketRecv");
		}

		printf("Server - rmf_osal_socketRecv Success: %s\n",buffer);

		char * reply = "Message from server process @_@";
		n = rmf_osal_socketSend(newsockfd, reply, strlen(reply), 0);
		if (n < 0) 
		{
			RMF_OSAL_ERROR_SERVER("ERROR on rmf_osal_socketSend");
		}
		printf("Server - rmf_osal_socketSend Success\n",buffer);

		rmf_osal_socketFDZero(&fdset);
		ret = rmf_osal_socketFDIsSet(newsockfd, &fdset);
		if (ret == 0)
		{
			printf("Server - rmf_osal_socketFDIsSet : as expected, fd is not set \n");
		}
		else
		{
			RMF_OSAL_ERROR_SERVER("rmf_osal_socketFDIsSet returned unexpected val\n");
		}

		rmf_osal_socketFDSet( newsockfd, &fdset);
		ret = rmf_osal_socketFDIsSet( newsockfd, &fdset);
		if (ret == 0)
		{
			RMF_OSAL_ERROR_SERVER("Server - rmf_osal_socketFDIsSet : fd not set \n");
		}
		else
		{
			printf("rmf_osal_socketFDIsSet Success\n");
		}

		rmf_osal_socketFDClear( newsockfd, &fdset);
		ret = rmf_osal_socketFDIsSet(newsockfd, &fdset);
		if (ret == 0)
		{
			printf("Server - rmf_osal_socketFDIsSet : as expected, fd is not set \n");
		}
		else
		{
			RMF_OSAL_ERROR_SERVER("rmf_osal_socketFDIsSet returned unexpected val\n");
		}
		rmf_osal_socketFDSet( newsockfd, &fdset);
		ret = rmf_osal_socketFDIsSet( newsockfd, &fdset);
		if (ret == 0)
		{
			RMF_OSAL_ERROR_SERVER("Server - rmf_osal_socketFDIsSet : fd not set \n");
		}
		else
		{
			printf("rmf_osal_socketFDIsSet : fd  is set with newsockfd\n");
		}

		while (1)
		{
			struct timeval tv;
			tv.tv_sec = 5;
			tv.tv_usec = 0;
			read_fds = fdset;
			ret = rmf_osal_socketSelect(newsockfd+1, &read_fds, NULL, NULL, &tv);
			if (ret < 0)
			{
				RMF_OSAL_ERROR_SERVER("Client: rmf_osal_socketSelect failed\n");
			}

			ret = rmf_osal_socketIoctl(newsockfd, RMF_OSAL_SOCKET_FIONREAD, &n);
			if (ret < 0)
			{
				RMF_OSAL_ERROR_SERVER("Client: rmf_osal_socketIoctl failed\n");
			}
			printf("Server - rmf_osal_socketIoctl Success : No of bytes avail = %d\n", n);

			memset(buffer,0, BUF_LEN);
			n = rmf_osal_socketRecv(newsockfd,buffer,BUF_LEN, 0);
			if (n < 0) 
			{
				RMF_OSAL_ERROR_SERVER("ERROR on rmf_osal_socketRecv");
			}
			printf("Server - rmf_osal_socketRecv Success(%d): %s\n",n,buffer);
			if (NULL != strstr(buffer, "<exit>"))
			{
				printf("Server - Exiting receive loop\n");
				break;
			}
		}

		ret = rmf_osal_socketShutdown(newsockfd, RMF_OSAL_SOCKET_SHUT_WR);
		if (ret != 0)
		{
			RMF_OSAL_ERROR_SERVER("Server: rmf_osal_socketShutdown failed\n");
		}
		printf("Server - rmf_osal_socketShutdown success \n");

		n = rmf_osal_socketSend(newsockfd, reply, strlen(reply), 0);
		if (n < 0) 
		{
			    printf("Server -Expected error on rmf_osal_socketSend\n");
		}
		else
		{
			RMF_OSAL_ERROR_SERVER("Server - rmf_osal_socketSend Success - Should fail\n");
		}

		rmf_osal_SocketSockLen length = sizeof(n);
		ret = rmf_osal_socketGetOpt( newsockfd, RMF_OSAL_SOCKET_SOL_SOCKET,
			RMF_OSAL_SOCKET_SO_BROADCAST, &n, &length);
		if (ret != 0)
		{
			RMF_OSAL_ERROR_SERVER("Client: rmf_osal_socketGetOpt failed\n");
		}
		printf("Server - rmf_osal_socketGetOpt success  value = 0x%x \n", n);

		n =1;
		ret = rmf_osal_socketSetOpt( newsockfd, RMF_OSAL_SOCKET_SOL_SOCKET,
			RMF_OSAL_SOCKET_SO_BROADCAST, &n, sizeof(n));
		if (ret != 0)
		{
			RMF_OSAL_ERROR_SERVER("Client: rmf_osal_socketSetOpt failed\n");
		}
		printf("Server - rmf_osal_socketGetOpt success  value = 0x%x \n", n);

		n =0xFF;
		ret = rmf_osal_socketGetOpt( newsockfd, RMF_OSAL_SOCKET_SOL_SOCKET,
			RMF_OSAL_SOCKET_SO_BROADCAST, &n, &length);
		if (ret != 0)
		{
			RMF_OSAL_ERROR_SERVER("Client: rmf_osal_socketGetOpt failed\n");
		}
		printf("Server - rmf_osal_socketGetOpt success  value = 0x%x \n", n);

		ret = rmf_osal_socketClose( newsockfd);
		if (ret != 0)
		{
			RMF_OSAL_ERROR_SERVER("Client: rmf_osal_socketClose failed\n");
		}
		printf("Server - rmf_osal_socketClose success \n");

		ret = rmf_osal_socketClose( sockfd);
		if (ret != 0)
		{
			RMF_OSAL_ERROR_SERVER("Client: rmf_osal_socketClose failed\n");
		}
		printf("Server - rmf_osal_socketClose success \n");

		{
			rmf_osal_SocketIPv4Addr addrptr;
			char * str = NULL;
			ret =  rmf_osal_socketAtoN("192.168.0.1", &addrptr);
			if (ret != 1)
			{
				RMF_OSAL_ERROR_SERVER("Client: rmf_osal_socketAtoN failed\n");
			}
			printf("Server - rmf_osal_socketAtoN success \n");
			str = rmf_osal_socketNtoA( addrptr);
			if (NULL == str)
			{
				RMF_OSAL_ERROR_SERVER("Client: rmf_osal_socketNtoA failed\n");
			}
			printf("Server - rmf_osal_socketNtoA success : [%s]\n", str);
		}

		{
			uint32_t value = 0x12345678;
			printf ("rmf_osal_socketHtoNL Input 0x%x Output 0x%x\n", 
				value, rmf_osal_socketHtoNL( value));
			
		}

		{
			uint32_t value = 0x12345678;
			printf ("rmf_osal_socketNtoHL Input 0x%x Output 0x%x\n", 
				value, rmf_osal_socketNtoHL( value));
			
		}

		{
			uint16_t value = 0x1234;
			printf ("rmf_osal_socketHtoNS Input 0x%x Output 0x%x\n", 
				value, rmf_osal_socketHtoNS( value));
			
		}

		{
			uint16_t value = 0x1234;
			printf ("rmf_osal_socketNtoHS Input 0x%x Output 0x%x\n", 
				value, rmf_osal_socketNtoHS( value));
			
		}

	}
	printf("Test Complete\n");
	return 0; 
}

