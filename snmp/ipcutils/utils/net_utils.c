/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2011 RDK Management
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




/*-------------------------------------------------------------------
   Include Files 
-------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_arp.h>
#include <linux/route.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "netUtils.h"
#include <rdk_debug.h>

void vlNetAddFlags(int sock, const char * pszIfName, int nFlags)
{
    struct ifreq ifr;
    memset(&ifr,0,sizeof(ifr));
    strncpy(ifr.ifr_name, pszIfName, IFNAMSIZ);
    ifr.ifr_name [IFNAMSIZ - 1] = 0;
	
    if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0)
    {
        perror("vlNetAddFlags: SIOCGIFFLAGS err 1");
    }
    else
    {
        ifr.ifr_flags |= nFlags;
        if (ioctl(sock, SIOCSIFFLAGS, &ifr) < 0)
        {
            perror("vlNetAddFlags: SIOCSIFFLAGS err 2");
        }
    }
}

void vlNetRemoveFlags(int sock, const char * pszIfName, int nFlags)
{
    struct ifreq ifr;
    memset(&ifr,0,sizeof(ifr));
    strncpy(ifr.ifr_name, pszIfName, IFNAMSIZ);
    ifr.ifr_name [IFNAMSIZ - 1] = 0;

    if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0)
    {
        perror("vlNetRemoveFlags: SIOCGIFFLAGS err 1");
    }
    else
    {
        ifr.ifr_flags &= ~(nFlags);
        if (ioctl(sock, SIOCSIFFLAGS, &ifr) < 0)
        {
            perror("vlNetRemoveFlags: SIOCSIFFLAGS err 2");
        }
    }
}
#define MAC_ADDRESS_SIZE 6
void vlNetSetMacAddress(int sock, const char * pszIfName, unsigned char * pMacAddress)
{
    struct ifreq ifr;
    memset(&ifr,0,sizeof(ifr));

    strncpy(ifr.ifr_name, pszIfName, IFNAMSIZ);
    ifr.ifr_name [IFNAMSIZ-1] = 0;
    
    ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
    memcpy(ifr.ifr_hwaddr.sa_data, pMacAddress, MAC_ADDRESS_SIZE);

    if(ioctl(sock, SIOCSIFHWADDR, &ifr) < 0)
    {
        // this failure is probably OK
        perror("vlNetSetMacAddress : ioctl(SET HW ADDR) failed");
    }
}

void vlNetSetIpAddress(int sock, const char * pszIfName, unsigned char * pIpAddress)
{
    struct ifreq ifr;
    char host[128];
    struct sockaddr_in sai;
    memset(&ifr,0,sizeof(ifr));
    memset(&sai,0,sizeof(sai));
    int result = 0;
    strncpy(ifr.ifr_name, pszIfName, IFNAMSIZ);
    ifr.ifr_name [IFNAMSIZ-1] = 0;

    snprintf(host, sizeof(host), "%u.%u.%u.%u", pIpAddress[0], pIpAddress[1],
            pIpAddress[2], pIpAddress[3]);
    if ( inet_aton(host, &sai.sin_addr) == 0)
    {
        //printf("Invalid IP addr %s\n", host);
       // printf("%s(): Invalid IP addr %s\n", __FUNCTION__,host);
    }
    sai.sin_family = AF_INET;
    sai.sin_port = 0;
    memcpy((char *) &ifr.ifr_ifru.ifru_addr, (char *)&sai, sizeof(struct sockaddr));
                
    if ((result = ioctl(sock, SIOCSIFADDR, &ifr)) < 0)
    {
        perror("vlNetSetSubnetMask: SIOCSIFADDR");
    }
}

void vlNetSetSubnetMask(int sock, const char * pszIfName, unsigned char * pSubnetMask)
{
    struct ifreq ifr;
    char host[128];
    struct sockaddr_in netmask;
    memset(&ifr,0,sizeof(ifr));
    memset(&netmask,0,sizeof(netmask));
    strncpy(ifr.ifr_name, pszIfName, IFNAMSIZ);
    ifr.ifr_name [IFNAMSIZ-1] = 0;

    netmask.sin_family = AF_INET;
    netmask.sin_port = 0;
    snprintf(host, sizeof(host), "%u.%u.%u.%u", pSubnetMask[0], pSubnetMask[1],
            pSubnetMask[2], pSubnetMask[3]);
    if ( inet_aton(host, &netmask.sin_addr) == 0)
    {
//        printf("Invalid subnet mask %s\n", host);
    }
                    
    memcpy((char *) &ifr.ifr_ifru.ifru_netmask, (char *)&netmask, sizeof(struct sockaddr));
    if (ioctl(sock, SIOCSIFNETMASK, &ifr) < 0)
    {
        perror("vlNetSetSubnetMask: SIOCSIFNETMASK");
    }
}

int vlNetSetNetworkRoute(unsigned long ipNet, unsigned long ipMask, unsigned long ipGateway, char * pszIfName)
{
    struct ifreq ifr;
    struct sockaddr_in *sin = (struct sockaddr_in*)&(ifr.ifr_addr);
    struct rtentry rt;
    int err = 0;

    // init

    memset(&ifr,0,sizeof(ifr));
    memset(&rt,0,sizeof(rt));
    sin->sin_family = AF_INET;
    sin->sin_port = 0;

    sin->sin_addr.s_addr = htonl(ipNet);
    memcpy(&rt.rt_dst, sin, sizeof(*sin));

    sin->sin_addr.s_addr = htonl(ipMask);
    memcpy(&rt.rt_genmask, sin, sizeof(*sin));

    sin->sin_addr.s_addr = htonl(ipGateway);
    memcpy(&rt.rt_gateway, sin, sizeof(*sin));

    if(NULL != pszIfName)
    {
        strncpy(ifr.ifr_name, pszIfName, IFNAMSIZ);
        ifr.ifr_name [IFNAMSIZ-1] = 0;
        rt.rt_dev = ifr.ifr_name;
    }
    
    rt.rt_flags |= RTF_UP;
    rt.rt_metric = 0;
    
    if(0 != ipGateway)
    {
        rt.rt_flags |= RTF_GATEWAY;
    }

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock < 0) return -1;

     if( sock == 0 ) 
     {
	 	RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD",
	 	"\n\n\n%s: Closed file descriptor 0 !!\n\n\n\n",__FUNCTION__);
     }
     if (err = ioctl(sock, SIOCADDRT, &rt))
     {
         //printf("%s(): SIOCADDRT '%s' returned '%s'\n", __FUNCTION__, pszIfName, strerror(errno));
     }
    close(sock);

    
    return err;
}

int vlNetDelNetworkRoute(unsigned long ipNet, unsigned long ipMask, unsigned long ipGateway, char * pszIfName)
{
    struct ifreq ifr;
    struct sockaddr_in *sin = (struct sockaddr_in*)&(ifr.ifr_addr);
    struct rtentry rt;
    int err = 0;

    // init
    memset(&ifr,0,sizeof(ifr));
    memset(&rt,0,sizeof(rt));
    sin->sin_family = AF_INET;
    sin->sin_port = 0;

    sin->sin_addr.s_addr = htonl(ipNet);
    memcpy(&rt.rt_dst, sin, sizeof(*sin));

    sin->sin_addr.s_addr = htonl(ipMask);
    memcpy(&rt.rt_genmask, sin, sizeof(*sin));

    sin->sin_addr.s_addr = htonl(ipGateway);
    memcpy(&rt.rt_gateway, sin, sizeof(*sin));

    if(NULL != pszIfName)
    {
        strncpy(ifr.ifr_name, pszIfName, IFNAMSIZ);
        ifr.ifr_name [ IFNAMSIZ-1 ] = 0;	
        rt.rt_dev = ifr.ifr_name;
    }
    
    rt.rt_flags |= RTF_UP;
    rt.rt_metric = 0;

    if(0 != ipGateway)
    {
        rt.rt_flags |= RTF_GATEWAY;
    }

    int sock = socket(AF_INET, SOCK_DGRAM, 0);

     if(sock < 0) return -1;
     if( sock == 0 ) 
     {
	 	RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD",
	 	"\n\n\n%s: Closed file descriptor 0 !!\n\n\n\n",__FUNCTION__);
     }
     if (err = ioctl(sock, SIOCDELRT, &rt))
     {
     //            printf("%s(): SIOCDELRT '%s' returned '%s'\n", __FUNCTION__, pszIfName, strerror(errno));
     }    
     close(sock);

    return err;
}

