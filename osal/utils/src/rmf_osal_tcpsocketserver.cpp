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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
//#include "cThreadPool.h"
//#include <mpeos_dbg.h>
#include "rdk_debug.h"
#include "rmf_osal_tcpsocketserver.h"

#if USE_SYSRES_MLT
#include "rpl_malloc.h"
#endif

#ifdef WIN32
#define close closesocket
#endif

#define RMF_OSAL_TCP_SS_LOG   printf


typedef int SOCKET;

static void* rmf_osal_TcpSocketServer_serviceThread(void * _pServiceContext);

typedef void* (*THREADFP)(void *);

static pthread_t create_thread(char *pszName, THREADFP threadFunc, unsigned long arg)
{
    (void)pszName;

    pthread_attr_t td_attr;                        /* Thread attributes. */
    pthread_t td_id;

    pthread_attr_init(&td_attr);
    pthread_attr_setdetachstate(&td_attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setschedpolicy(&td_attr, SCHED_OTHER);

    pthread_create(&td_id, &td_attr, threadFunc, (void*)arg);

    return td_id;
}

static void* rmf_osal_TcpSocketServer(void * _pServiceContext)
{
    int nRet = 0;
    SOCKET newsockfd, port, clilen;
    struct sockaddr_in6 serv_addr, cli_addr;
    SOCKET sockfd = 0;
    RMF_OSAL_TCPSOCKETSERVER_SERVICE_CONTEXT * pServiceContext =
            (RMF_OSAL_TCPSOCKETSERVER_SERVICE_CONTEXT*)_pServiceContext;
    
    if ((sockfd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
#ifdef WIN32
        int er = WSAGetLastError();
#endif // WIN32

        //VL_TCP_SS_LOG("rmf_osal_TcpSocketServer : Error opening socket \n");
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.UTIL","%s: Error opening socket \n",__FUNCTION__);
        return NULL;
    }

    /* Setting all values in a buffer to zero */
    memset((char *) &serv_addr, 0, sizeof(serv_addr));

    /* Converting portnumber from string of digits to an integer */
    port = pServiceContext->nPort;

    /* Code for address family */
    serv_addr.sin6_family = AF_INET6;

    /* IP address of the host */
    serv_addr.sin6_addr = in6addr_any;
    /* Port number */
    serv_addr.sin6_port = htons((u_short)port);

    // Reuse address.
    {
        int reuse = 1;
        if( setsockopt (sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof (reuse)) < 0 )
        {
            //VL_TCP_SS_LOG("rmf_osal_TcpSocketServer : setsockopt SO_REUSEADDR error:  %s \n", strerror(errno));
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.UTIL","%s: setsockopt SO_REUSEADDR error:  %s \n", __FUNCTION__,strerror(errno));
        }
    }
    
    /* Binding a socket to an address of the current host and port number */
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
        sizeof(serv_addr)) < 0)
    {
        //VL_TCP_SS_LOG("rmf_osal_TcpSocketServer : bind on socket failed \n");
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.UTIL","rmf_osal_TcpSocketServer : bind on socket failed \n");
        close(sockfd);
        return NULL;
    }

    /* Listen on socket for connections */
    nRet = listen(sockfd,256);
    
    if (nRet != 0)
    {
        //VL_TCP_SS_LOG("rmf_osal_TcpSocketServer : listen on socket failed \n");
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.UTIL","rmf_osal_TcpSocketServer : listen on socket failed \n");
        close(sockfd);
        return NULL;
    }

    clilen = sizeof(cli_addr);

    /* accept() blocks the process until a client connects to the server */
    /*  accepting connections from multiple clients */
    //VL_TCP_SS_LOG("rmf_osal_TcpSocketServer : starting service for socket %d, port %d\n", sockfd, port);
    while (1)
    {
        if ((newsockfd = accept(sockfd, NULL, NULL)) < 0)
        {
            //VL_TCP_SS_LOG("rmf_osal_TcpSocketServer : accept failed \n");
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.UTIL","rmf_osal_TcpSocketServer : accept failed \n");
            close(sockfd);
            return NULL;
        }
        else
        {
            RMF_OSAL_TCPSOCKETSERVER_SERVICE_CONTEXT * pHandlerServiceContext =
                    (RMF_OSAL_TCPSOCKETSERVER_SERVICE_CONTEXT*)malloc(sizeof(RMF_OSAL_TCPSOCKETSERVER_SERVICE_CONTEXT));
            long pthread_server = 0;
            pHandlerServiceContext->nPort           = pServiceContext->nPort;
            pHandlerServiceContext->sockfd          = newsockfd;
            pHandlerServiceContext->pServiceFunc    = pServiceContext->pServiceFunc;
            pHandlerServiceContext->pData           = pServiceContext->pData;
            //create thread here to handle requests
            //pthread_server = cThreadCreateSimple((char*)"rmf_osal_TcpSocketServer_serviceThread", (THREADFP)rmf_osal_TcpSocketServer_serviceThread, (unsigned long)pHandlerServiceContext);
            pthread_server = create_thread((char*)"rmf_osal_TcpSocketServer_serviceThread", (THREADFP)rmf_osal_TcpSocketServer_serviceThread, (unsigned long)pHandlerServiceContext);
        }
    }
    
    return NULL;
}

void* rmf_osal_TcpSocketServer_serviceThread(void * _pServiceContext)
{
    char szPeerName[128];
    char * pszPeerName = NULL;
    RMF_OSAL_TCPSOCKETSERVER_SERVICE_CONTEXT * pServiceContext =
            (RMF_OSAL_TCPSOCKETSERVER_SERVICE_CONTEXT*)_pServiceContext;
    SOCKET newsockfd = (SOCKET)pServiceContext->sockfd;
    struct sockaddr_in6 peername;
    socklen_t nLen = sizeof(peername);
    int ret = getpeername(newsockfd, (struct sockaddr*)&peername, &nLen);
    
    if(0 != ret)
    {
        perror("rmf_osal_TcpSocketServer_serviceThread: getpeername");
    }
    
    if(inet_ntop(AF_INET6, &peername.sin6_addr, szPeerName, sizeof(szPeerName))) {
        //VL_TCP_SS_LOG("rmf_osal_TcpSocketServer_serviceThread: Client address is %s, port is %d\n", szPeerName, ntohs(peername.sin6_port));
    }

    struct hostent * hostent = gethostbyaddr((char*)(&(peername.sin6_addr)), sizeof(peername.sin6_addr), AF_INET6);
    if(NULL != hostent)
    {
        pszPeerName = hostent->h_name;
    }
    else
    {
        pszPeerName = szPeerName;
    }

    pServiceContext->pServiceFunc(newsockfd, pServiceContext->pData);
            
    //VL_TCP_SS_LOG("rmf_osal_TcpSocketServer_serviceThread : ...Closing connection %d ['%s']\n", newsockfd, pszPeerName);
    close((SOCKET)newsockfd);
    free(_pServiceContext);
    return 0;
}

int rmf_osal_CreateTcpSocketServer(int nPort, RMF_OSAL_TCPSOCKETSERVER_SERVICE_FUNC pServiceFunc, void * pData)
{
    RMF_OSAL_TCPSOCKETSERVER_SERVICE_CONTEXT * pServiceContext =
            (RMF_OSAL_TCPSOCKETSERVER_SERVICE_CONTEXT*)malloc(sizeof(RMF_OSAL_TCPSOCKETSERVER_SERVICE_CONTEXT));
    long pthread_server = 0;
    pServiceContext->nPort          = nPort;
    pServiceContext->sockfd         = 0;
    pServiceContext->pServiceFunc   = pServiceFunc;
    pServiceContext->pData          = pData;
    //create thread here to handle requests
    //pthread_server = cThreadCreateSimple("rmf_osal_TcpSocketServer", (THREADFP)rmf_osal_TcpSocketServer, (unsigned long)pServiceContext);
    pthread_server = create_thread("rmf_osal_TcpSocketServer", (THREADFP)rmf_osal_TcpSocketServer, (unsigned long)pServiceContext);
    return 1;
}

int rmf_osal_TcpConnectToHost(const char * szHost, int nRemotePortNum)
{
    long sock = 0;
    char szPort[32];

    struct addrinfo hints, *res, *res0;
    int error = 0;

    memset(&hints, 0, sizeof(hints));

    // specify the family
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype  = SOCK_STREAM;
    hints.ai_protocol |= IPPROTO_TCP;
    snprintf(szPort,sizeof(szPort), "%u", nRemotePortNum);

    // resolve names or IP addresses
    error = getaddrinfo(szHost, szPort, &hints, &res0);

    if (error)
    {
        perror("rmf_osal_TcpConnectToHost: getaddrinfo");
        return 0;
    }

    // create network I/O
    sock = -1;
    for (res = res0; NULL != res; res = res->ai_next)
    {
        //printf("rmf_osal_TcpConnectToHost: creating socket for family %d, type %d, protocol %d\n", res->ai_family, res->ai_socktype, res->ai_protocol);
        sock = socket(res->ai_family, res->ai_socktype,
                   res->ai_protocol);
        if (sock < 0)
        {
            printf("rmf_osal_TcpConnectToHost: create socket for '%s' failed\n", szHost);
            continue;
        }

        // connect
        /*printf("rmf_osal_TcpConnectToHost: TCP: connecting to '%s' port:%s\n    using F:%d (%s), T:%d (%s), P:%d (%s)\n",
               szHost, szPort,
               res->ai_family, rmf_osal_GetAddressFamilyName(res->ai_family),
               res->ai_socktype, rmf_osal_GetSockTypeName(res->ai_socktype),
               res->ai_protocol, rmf_osal_GetIpProtocolName(res->ai_protocol));*/
        if (connect(sock, res->ai_addr, res->ai_addrlen) < 0)
        {
            perror("rmf_osal_TcpConnectToHost: connect");
//            printf("rmf_osal_TcpConnectToHost: connect to '%s' failed\n", szHost);
            close(sock);
            sock = -1;
            continue;
        }
        /* connect succeeded */

        break;  /* connect or bind succeeded */
    }
    freeaddrinfo(res0);

    return sock;
}
