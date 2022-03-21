/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2018 RDK Management
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
/* file: IPDSocketManager.cpp */

// These next 3 headers are required to use RDK_LOG.
#include "poddef.h"
#include "lcsm_log.h"
#include <rdk_debug.h>

#include <global_event_map.h>
#include "IPDSocketManager.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <string.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <arpa/inet.h>

#include "mrerrno.h"

#include "libIBus.h"
#include "libIBusDaemon.h"
#include "sysMgr.h"

#include "rmf_osal_event.h"
#include "rmf_osal_thread.h"
#include "rmf_osal_mem.h"
#include "ipdirectUnicast-main.h"

#include "cardUtils.h"
#include "ip_types.h"
#include "xchanResApi.h"

static unsigned char recv_buf[2048];

static rmf_osal_ThreadId ThreadIdTC = 0;

static long sock = 0;

static void ipdirect_unicast_udp_socket_task(void * arg);

/* Globals */
extern "C" {
extern void vlMpeosDumpBuffer(rdk_LogLevel lvl, const char *mod, const void * pBuffer, int nBufSize);
}

static void ipdirect_unicast_udp_socket_task(void * arg)
{

    rmf_osal_event_handle_t event_handle;
    rmf_osal_event_params_t event_params;
    rmf_osal_eventqueue_handle_t event_queue;
    uint32_t retValue = RMF_SUCCESS;
    char *pData;
    struct sockaddr_in recvFromAddr;
    socklen_t nRecvFromLen = sizeof(recvFromAddr);
    size_t nBytes = 0;
    struct addrinfo *res = NULL;
    char szErrorStr[IPDIRECT_UNICAST_MAX_STR_SIZE] = "";
    char portname[16];
    struct addrinfo hints;
    socketThreadData *pThreadData;
    long param = (long)arg;
    pThreadData = (socketThreadData *)param;
    int exitStatus = -1;
    char *pIpAddr = NULL;
    VL_HOST_IP_INFO hostIpInfo;

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s : Running... with port %d\n", __FUNCTION__, pThreadData->local_port);

    vlXchanGetHostIpTypeInfo(VL_HOST_IP_IF_TYPE_DSG, 0, &hostIpInfo);
    if(VL_XCHAN_IP_ADDRESS_TYPE_IPV4 == hostIpInfo.ipAddressType)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: IPv4 address '%s'.\n", __FUNCTION__, hostIpInfo.szIpAddress);
        pIpAddr = &hostIpInfo.szIpAddress[0];
        exitStatus = 0;
    }
    else if(VL_XCHAN_IP_ADDRESS_TYPE_IPV6 == hostIpInfo.ipAddressType)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: IPv6 address '%s'.\n", __FUNCTION__, hostIpInfo.szIpV6GlobalAddress);
        pIpAddr = &hostIpInfo.szIpV6GlobalAddress[0];
        exitStatus = 0;
    }

    if ( exitStatus == 0 )
    {
        memset( &hints,0, sizeof(hints) );
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;
        snprintf(portname, sizeof(portname), "%u", pThreadData->local_port);
        if (getaddrinfo( pIpAddr, portname, &hints, &res))
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","ipdirect_unicast_udp_socket_task: getaddrinfo failed\n");
            exitStatus = -1;
        }
        else
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","getaddrinfo OK\n");
        }
    }

    if ( exitStatus == 0 )
    {
        char iptablesScript[100];
        IARM_Result_t iarmResult;
        IARM_Bus_SYSMgr_RunScript_t runScriptParam;

        sprintf(iptablesScript, "/lib/rdk/updateFireWall.sh runPod %d UDP WAN_INTERFACE", pThreadData->local_port);

        runScriptParam.return_value = -1;
        strcpy(runScriptParam.script_path, iptablesScript);
        iarmResult = IARM_Bus_Call(IARM_BUS_SYSMGR_NAME, IARM_BUS_SYSMGR_API_RunScript, &runScriptParam, sizeof(runScriptParam));
        if (IARM_RESULT_SUCCESS == iarmResult)
        {
            exitStatus = runScriptParam.return_value;
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "ipdirect_unicast_udp_socket_task: IARM_Bus_Call failed with %d\n", iarmResult);
        }

        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","ipdirect_unicast_udp_socket_task: RunScript = %d\n", exitStatus);
    }

    if ( exitStatus == 0 )
    {
        sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sock < 0)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","ipdirect_unicast_udp_socket_task: create socket failed\n");
            exitStatus = -1;
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","create socket passed sock %d\n", sock);
            if (bind(sock, res->ai_addr, res->ai_addrlen) < 0)
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","bind on socket failed error: %s \n", strerror_r(errno, szErrorStr, sizeof(szErrorStr)));
                exitStatus = -1;
            }
            else
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","bind on socket fine: \n");
            }
        }
    }

    if (res != NULL)
    {
        freeaddrinfo(res);
    }

    while(exitStatus == 0)
    {
        nBytes = recvfrom(sock, recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)&recvFromAddr, &nRecvFromLen);

        if(nBytes <= 0)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "ipdirect_unicast_udp_socket_task: Exiting as nBytes = %d\n", nBytes);
            break;
        }
        else if( nBytes > sizeof(recv_buf) )
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "ipdirect_unicast_udp_socket_task: Exiting as nBytes = %d\n", nBytes);
            break;
        }
        else
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "ipdirect_unicast_udp_socket_task: Received %d bytes from %s:%d\n", nBytes, inet_ntoa(recvFromAddr.sin_addr), ntohs(recvFromAddr.sin_port));
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "ipdirect_unicast_udp_socket_task: Received %d bytes at %d\n", nBytes, pThreadData->local_port);
            vlMpeosDumpBuffer(RDK_LOG_INFO, "LOG.RDK.POD", recv_buf, nBytes);
        }

        retValue = rmf_osal_memAllocP(RMF_OSAL_MEM_POD, nBytes,(void **)&pData);
        if ( RMF_SUCCESS == retValue )
        {
            memcpy(pData, recv_buf, nBytes);

            event_params.data = (void *) pData;
            event_params.priority = 0;
            event_params.data_extension = nBytes;
            event_params.free_payload_fn = podmgr_freefn;
            // send UDP payload to IP direct task queue to be sent to the card
            event_queue = GetIpDirectUnicastMsgQueue();

            if(0 != event_queue)
            {
                retValue = rmf_osal_event_create(RMF_OSAL_EVENT_CATEGORY_POD, POD_UDP_SOCKET_DATA, &event_params, &event_handle);
                if (retValue == RMF_SUCCESS)
                {
                    retValue = rmf_osal_eventqueue_add( event_queue, event_handle );
                    if (retValue != RMF_SUCCESS)
                    {
                        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s: rmf_osal_eventqueue_add Failed %d\n", __FUNCTION__, retValue);
                    }
                }
                else
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s: rmf_osal_event_create Failed %d\n", __FUNCTION__, retValue);
                }
            }
            else
            {
                retValue = -1;
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s : Problem can't find proper event queue %d.\n", __FUNCTION__, event_queue);
            }
            if (retValue != RMF_SUCCESS)
            {
                // an error occurred queuing the event
                rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pData);
            }
        }
        else
        {
            RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD","%s: rmf_osal_memAllocP failed in %s:%d\n",__FUNCTION__, __LINE__);
            break;
        }
    }

    if (sock != -1)
    {
        close(sock);
        sock = -1;
    }

    // Some type of error caused the task to exit, send the lost flow message to the MCARD
    vlIpDirectUnicastNotifySocketFailure(IPDIRECT_UNICAST_LOST_FLOW_REASON_SOCKET_READ_ERROR);

    RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD","Exiting %s\n",__FUNCTION__);
}

/* ProcessApduSocketFlowReq is the handler for socket_flow_req APDU.								*/
/*	return: void.																															*/
void ProcessApduSocketFlowReq(unsigned long Port)
{
    static unsigned long port = 0;
    socketThreadData *pThreadData;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);

    if (ThreadIdTC && (port == Port))
    {
        // preserve the thread running on the same port number
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s,Previous udp socket thread created with same port so keep\n", __FUNCTION__);
    }
    else
    {

        rmf_osal_memAllocP( RMF_OSAL_MEM_POD, sizeof( socketThreadData ), ( void ** ) &( pThreadData ));
        pThreadData->local_port = Port;
        port = Port;

        // this is where the UDP socket flow request processing will be done where a socket is going to be
        // set up and the bind and listen.  The socket, bind. listen may have to be on its own thread
        rmf_osal_threadCreate(ipdirect_unicast_udp_socket_task, (void *)pThreadData, 0, 0, &ThreadIdTC, "ipdirect_unicast_udp_socket_task"); 
    }
}

/* ProcessApduSocketFlowCnf is the handler for socket_flow_cnf APDU.								*/
/*	return: void.																															*/
void ProcessApduSocketFlowCnf(void)
{
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
}

/* KillSocketFlowThread kills the socket flow task if active
/*      return: void.                                                                                                                                                                                                                                                   */
void KillSocketFlowThread(void)
{
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD", "%s \n", __FUNCTION__);
    if (ThreadIdTC)
    {
        // kill the thread running on an old port number
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s, Destroy previous udp socket thread sock %d\n", __FUNCTION__, sock);
        // kill the thread by closing out the socket it is blocked on waiting for received data
        if (sock != -1)
        {
            close(sock);
            sock = -1;
        }
        rmf_osal_threadDestroy(ThreadIdTC);
        ThreadIdTC = 0;
        SetSocketFlowLocalPortNumber(0);
    }
}

