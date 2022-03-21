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

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <pthread.h>
//#include "cThreadPool.h"

#if USE_SYSRES_MLT
#include "rpl_malloc.h"
#endif

#include "rmf_osal_udpsocketserver.h"
#include "rmf_osal_resreg.h"
//#include <mpeos_dbg.h>
#include "rdk_debug.h"
#ifdef WIN32
#define close closesocket
#endif

#define RMF_OSAL_UCP_SS_LOG   printf

typedef int SOCKET;

static int rmf_osal_UdpSocketServer_serviceThread(SOCKET sockfd, RMF_OSAL_UDPSOCKETSERVER_SERVICE_CONTEXT * pServiceContext);

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

static void* rmf_osal_UdpSocketServer(void * _pServiceContext)
{
    SOCKET sockfd;
    struct sockaddr_in6 serv_addr;
    RMF_OSAL_UDPSOCKETSERVER_SERVICE_CONTEXT * pServiceContext =
            (RMF_OSAL_UDPSOCKETSERVER_SERVICE_CONTEXT*)_pServiceContext;
    rmf_osal_ThreadRegSet(RMF_OSAL_RES_TYPE_THREAD, 0, "udpSocketServer", 1024*1024, 0);
    if ((sockfd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
#ifdef WIN32
        int er = WSAGetLastError();
#endif // WIN32

       // VL_UCP_SS_LOG("rmf_osal_UdpSocketServer: Error opening socket \n");
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.UTIL","rmf_osal_UdpSocketServer : Error opening socket\n");
        return NULL;
    }

    /* Setting all values in a buffer to zero */
    memset((char *) &serv_addr, 0, sizeof(serv_addr));

    /* Code for address family */
    serv_addr.sin6_family = AF_INET6;

    /* IP address of the host */
    inet_pton(serv_addr.sin6_family, "::FFFF:127.0.0.1", &(serv_addr.sin6_addr));
    /* Port number */
    serv_addr.sin6_port = htons((u_short)pServiceContext->nLocalPort);

    // Reuse address.
    {
        int reuse = 1;
        if( setsockopt (sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof (reuse)) < 0 )
        {
           // VL_UCP_SS_LOG("rmf_osal_TcpSocketServer : setsockopt SO_REUSEADDR error:  %s \n", strerror(errno));
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.UTIL","rmf_osal_UdpSocketServer : setsockopt SO_REUSEADDR error:  %s \n", strerror(errno));

        }
    }
    /* Binding a socket to an address of the current host and port number */
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
        sizeof(serv_addr)) < 0)
    {
        //VL_UCP_SS_LOG("rmf_osal_UdpSocketServer : bind on socket failed \n");
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.UTIL","rmf_osal_UdpSocketServer : bind on socket failed \n");
        close(sockfd);
        return NULL;
    }

    rmf_osal_UdpSocketServer_serviceThread(sockfd, pServiceContext);
    return NULL;
}

int rmf_osal_UdpSocketServer_serviceThread(SOCKET sockfd, RMF_OSAL_UDPSOCKETSERVER_SERVICE_CONTEXT * pServiceContext)
{
    struct sockaddr_in6 echoclient;
    memset(&echoclient, 0, sizeof(echoclient));
    
    //VL_UCP_SS_LOG("rmf_osal_UdpSocketServer : rmf_osal_UdpSocketServer_serviceThread: starting service for socket %d, local port %d\n", sockfd, pServiceContext->nLocalPort);
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.UTIL","%s: starting service for socket %d, local port %d\n", __FUNCTION__, sockfd, pServiceContext->nLocalPort);

printf("%s(%d): pServiceContext->pServiceFunc: 0x%x\n", __FUNCTION__, __LINE__, pServiceContext->pServiceFunc);
    pServiceContext->pServiceFunc(sockfd, pServiceContext->pData);
            
    //VL_UCP_SS_LOG("rmf_osal_UdpSocketServer : ...Closing connection(%d)\n", sockfd);
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.UTIL","%s: ...Closing connection(%d)\n", __FUNCTION__,sockfd);
    close((SOCKET)sockfd);
    return 0;
}

int rmf_osal_CreateUdpSocketServer(int nLocalPort, RMF_OSAL_UDPSOCKETSERVER_SERVICE_FUNC pServiceFunc, void * pData)
{
    RMF_OSAL_UDPSOCKETSERVER_SERVICE_CONTEXT * pServiceContext =
            (RMF_OSAL_UDPSOCKETSERVER_SERVICE_CONTEXT*)malloc(sizeof(RMF_OSAL_UDPSOCKETSERVER_SERVICE_CONTEXT));
    long pthread_server = 0;
    pServiceContext->nLocalPort   = nLocalPort;
    pServiceContext->pServiceFunc = pServiceFunc;
    pServiceContext->pData        = pData;
    
printf("%s(%d): pServiceContext->pServiceFunc: 0x%x\n", __FUNCTION__, __LINE__, pServiceContext->pServiceFunc);
    //pthread_server = cThreadCreateSimple("rmf_osal_UdpSocketServer", (THREADFP)rmf_osal_UdpSocketServer, (unsigned long)pServiceContext);
    pthread_server = create_thread("rmf_osal_UdpSocketServer", (THREADFP)rmf_osal_UdpSocketServer, (unsigned long)pServiceContext);
    return 1;
}
