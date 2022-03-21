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



#ifdef GCC4_XXX
#include <vector>
#include <string>
#include <map>
#include <list>
#include <fstream>
#include <sstream>
#else
#include <vector.h>
#include <string.h>
#include <map.h>
#include <list.h>
#include <fstream.h>
#include <sstream.h>
#endif

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_arp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#include "dsgProxyTypes.h"
#include "vlDsgProxyService.h"
#include "persistentconfig.h"
#include "rmf_osal_sync.h"
#include "utilityMacros.h"
#include <rdk_debug.h>
#include "xchanResApi.h"
#include "hostGenericFeatures.h"
#ifndef SNMP_IPC_CLIENT
#include "vlSnmpEcmApi.h"
#include "vlSnmpHostInfo.h"
#else // else defined : SNMP_IPC_CLIENT 
#include "podimpl.h" // required to get CA system ID
#include "cardmanager.h" // required to get CP system ID
#endif //SNMP_IPC_CLIENT
#include "netUtils.h"
#include <rmf_osal_mem.h>
#include <string.h>
#include <errno.h>
#include "dsg_types.h"
#include "sys_init.h"
#include "vlDsgDispatchToClients.h"
#include "dsgResApi.h"
#include "rmf_osal_util.h"
#include "rmf_osal_thread.h"
#include <stdlib.h>
#include "cardUtils.h"
#include "vlMutex.h"
#include "vlEnv.h"
#ifdef GLI
#include "libIBus.h"
#include "sysMgr.h"
#endif
#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif

using namespace std;

static vector<unsigned char> vlg_dsgDcdBuffer;
static vector<unsigned char> vlg_dsgDirBuffer;
static vector<unsigned char> vlg_dsgAdvConfigBuffer;
static vector<unsigned char> vlg_dsgTlv217Buffer;
extern VL_HOST_IP_INFO vlg_HostIpInfo;
extern unsigned long vlg_nDsgVctId;
extern unsigned long vlg_ucid;
static VL_HOST_IP_INFO vlg_hnIpInfo;
static vlMutex vlg_dsgProxyLock;
static bool vlg_bConfigPathInitialized = false;
static char vlg_szBuffer[VL_DSG_MAX_STR_SIZE];
int vlg_bHostAcquiredProxyAddress = 0;
int vlg_bHostSetupProxyServerAddress = 0;
int vlg_bDsgProxyEnableTunnelForwarding = 0;
int vlg_bDsgProxyEnableTunnelReceiving  = 0;
static char vlg_szErrorBuffer[VL_DSG_MAX_STR_SIZE];

extern VL_DSG_DIRECTORY    vlg_DsgDirectory;
extern VL_DSG_ADV_CONFIG   vlg_DsgAdvConfig;
static int vlg_nApps = 0;
static unsigned long vlg_aApps[VL_MAX_DSG_HOST_ENTRIES];
static int vlg_nMcastSendSocket = -1;
static int vlg_nScte65McastReceiveSocket = -1;
static int vlg_nScte18McastReceiveSocket = -1;
static int vlg_nXaitMcastReceiveSocket = -1;
static rmf_osal_ThreadId  vlg_nScte65McastReceiveThread = 0;
static rmf_osal_ThreadId vlg_nScte18McastReceiveThread = 0;
static rmf_osal_ThreadId vlg_nXaitMcastReceiveThread = 0;
static int vlg_nAppsReceiveSockets = 0;
static int vlg_aAppsReceiveSockets[VL_MAX_DSG_HOST_ENTRIES];
static rmf_osal_ThreadId vlg_aAppsReceiveThreads[VL_MAX_DSG_HOST_ENTRIES] = {0,};
static struct sockaddr_in vlg_mcastifaddr;
#define VL_DSG_PROXY_PRINT_ERROR_STRING()   vlDsgProxyServer_printSysErrorString(__FUNCTION__, __LINE__, errno)
#define VL_DSG_PROXY_CLIENT_VPN_IF_NAME "tun0"
#define VL_EXPAND_IP_ADDRESS_FOR_PRINTF(nIpAddress) (int)(((nIpAddress)>>24)&0xFF), (int)(((nIpAddress)>>16)&0xFF), (int)(((nIpAddress)>>8)&0xFF), (int)(((nIpAddress)>>0)&0xFF)

static list < pair<string, string> > vlg_dsgProxyAttributes;

static const char vlg_pszDsgProxyPublicKey[512]={0};

#define vlStrCpy(pDest, pSrc, nDestCapacity)            \
            memset(pDest, 0, nDestCapacity); strncpy(pDest, pSrc, nDestCapacity-1)

static bool vlInitDsgProxyConfigPath()
{VL_AUTO_LOCK(vlg_dsgProxyLock);
    return vlg_bConfigPathInitialized;
}

void vlDsgProxyServer_printSysErrorString(const char * pszFuncName, int nLine, int nErrno)
{VL_AUTO_LOCK(vlg_dsgProxyLock);

    strerror_r(nErrno, vlg_szErrorBuffer, sizeof(vlg_szErrorBuffer));
    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","%s():L:%d:errno=%d:%s\n", pszFuncName, nLine, nErrno, vlg_szErrorBuffer);
    //auto_unlock_ptr(vlg_dsgProxyLock);	
}

void vlDsgProxyServer_ConvertBufferToAscii(string & strAsciiBuffer, const unsigned char * pBytes, int nBytes)
{VL_AUTO_LOCK(vlg_dsgProxyLock);

    char szBuf[8];
    int i = 0;
    strAsciiBuffer.clear();
    for(i = 0; i < nBytes; i++)
    {
        snprintf(szBuf, sizeof(szBuf), "%02X%s", pBytes[i], ((i+1 != nBytes)?":":""));
        strAsciiBuffer.append(szBuf);
    }
}

void vlDsgProxyServer_ConvertBufferToAscii(string & strAsciiBuffer, const vector<unsigned char> & rVector)
{VL_AUTO_LOCK(vlg_dsgProxyLock);

    vlDsgProxyServer_ConvertBufferToAscii(strAsciiBuffer, (&(*(rVector.begin()))), rVector.size());
}

string vlDsgProxyServer_getNextToken(istream & rFile, char delim = '\n')
{
    char szStr[DSGPROXY_MAX_STR_SIZE];
    szStr[0] = '\0';
    rFile.getline(szStr, sizeof(szStr), delim);
    char * pszCr = strrchr(szStr, '\n');
    if(NULL != pszCr) *pszCr = '\0';
    return szStr;
}

void vlDsgProxyServer_BuildServiceAttributes(string & strAttributes)
{VL_AUTO_LOCK(vlg_dsgProxyLock);

    strAttributes.clear();
    list < pair<string, string> >::iterator it;

    strAttributes = " \"";
    for(it = vlg_dsgProxyAttributes.begin(); it != vlg_dsgProxyAttributes.end(); it++)
    {
        if(vlg_dsgProxyAttributes.begin() != it) strAttributes += ",";
        strAttributes += "(";
        strAttributes += (*it).first;
        strAttributes += "=";
        strAttributes += (*it).second;
        strAttributes += ")";
    }
    strAttributes += "\"\n";
}

const char * vlDsgProxyServer_printPower(long nPower, char* units)
{VL_AUTO_LOCK(vlg_dsgProxyLock);

    snprintf(vlg_szBuffer, sizeof(vlg_szBuffer), "%s%d.%d %s", ((nPower<0)?"-":""), (vlAbs(nPower)/10), (vlAbs(nPower)%10), units);
    return vlg_szBuffer;
}

const char * vlDsgProxyServer_printInt(long nValue)
{VL_AUTO_LOCK(vlg_dsgProxyLock);

    snprintf(vlg_szBuffer, sizeof(vlg_szBuffer), "%d", nValue);
    return vlg_szBuffer;
}

const char * vlDsgProxyServer_printHex(unsigned long nValue)
{VL_AUTO_LOCK(vlg_dsgProxyLock);
    
    snprintf(vlg_szBuffer, sizeof(vlg_szBuffer), "0x%X", nValue);
    return vlg_szBuffer;
}

void vlDsgProxyServer_updateConf(const char * file, const char * str)
{VL_AUTO_LOCK(vlg_dsgProxyLock);

    FILE * fp = fopen(file, "w");
    if(NULL != fp)
    {
        fputs(str, fp);
        fputc('\n', fp);
        fclose(fp);
    }
}

int vlDsgProxyServer_readConf(const char * pszFile, char * pszBuffer, int nBytesCapacity)
{VL_AUTO_LOCK(vlg_dsgProxyLock);

    FILE * fp = fopen(pszFile, "r");
    int nRead = 0;
    if(NULL != fp)
    {
        memset(pszBuffer, 0, nBytesCapacity);
        nRead = fread(pszBuffer, 1, nBytesCapacity, fp);
        fclose(fp);
    }
    
    return nRead;
}

int vlDsgProxyServer_createMulticastReceiveSocket(const char * pszIfName, unsigned long nGroup, int nPort)
{
    struct ifreq ifr;
    int result = 0;
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock != -1)
    {
        struct sockaddr_in localifaddr;
        struct sockaddr_in mcastifaddr;
        struct in_addr group_addr;
        struct ip_mreq mreq;

	memset(&group_addr,0,sizeof(group_addr));
	memset(&ifr,0,sizeof(ifr));
	memset(&localifaddr,0,sizeof(localifaddr));
	memset(&mcastifaddr,0,sizeof(mcastifaddr));
	memset(&mreq,0,sizeof(mreq));
        strncpy(ifr.ifr_name, pszIfName, IFNAMSIZ);
        ifr.ifr_name[IFNAMSIZ - 1] = 0;
        ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;

        if ((result = ioctl(sock, SIOCGIFADDR, &ifr)) < 0)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","%s():Cannot get IP addr for %s, ret %d\n", __FUNCTION__,ifr.ifr_name, result);
            VL_DSG_PROXY_PRINT_ERROR_STRING();
        }
        else
        {
            memcpy((char *)&localifaddr, (char *) &ifr.ifr_ifru.ifru_addr, sizeof(struct sockaddr));
            mcastifaddr.sin_family      = AF_INET;
            mcastifaddr.sin_addr.s_addr = htonl(nGroup);
            mcastifaddr.sin_port        = htons(nPort);
            
            mreq.imr_multiaddr.s_addr = htonl(nGroup);
            mreq.imr_interface.s_addr = htonl(INADDR_ANY); // test statement
            // add constraint to physical interface
            mreq.imr_interface = localifaddr.sin_addr;

            // Reuse address.
            {
                int reuse = 1;
                if( setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof (reuse)) < 0 )
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","%s():setsockopt SO_REUSEADDR error:  %s \n", __FUNCTION__);
                    VL_DSG_PROXY_PRINT_ERROR_STRING();
                }
            }
            
            if (bind(sock, (struct sockaddr *)&mcastifaddr, sizeof(mcastifaddr)) < 0)
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","%s():bind to '%d.%d.%d.%d' failed.\n", __FUNCTION__, VL_EXPAND_IP_ADDRESS_FOR_PRINTF(nGroup));
                VL_DSG_PROXY_PRINT_ERROR_STRING();
            }
            else
            {
                if(setsockopt (sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","%s():FAILED to add Multicast Membership for group '%d.%d.%d.%d' at %s [%s]\n", __FUNCTION__,VL_EXPAND_IP_ADDRESS_FOR_PRINTF(nGroup), ifr.ifr_name, inet_ntoa(localifaddr.sin_addr));
                    VL_DSG_PROXY_PRINT_ERROR_STRING();
                }
                else
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","%s():Added Multicast Membership for group '%d.%d.%d.%d' at %s [%s]\n", __FUNCTION__,VL_EXPAND_IP_ADDRESS_FOR_PRINTF(nGroup), ifr.ifr_name, inet_ntoa(localifaddr.sin_addr));
                    return sock;
                }
            }
        }
        close(sock);
    }

    return -1;
}

int vlDsgProxyServer_createMulticastSendSocket(const char * pszIpAddr, unsigned long nGroup, int nPort)
{
    int result = 0;
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s(): Creating Send Socket\n", __FUNCTION__);
    if(sock != -1)
    {
        struct sockaddr_in localifaddr;
        struct in_addr group_addr;

	memset(&group_addr,0,sizeof(group_addr));
	memset(&localifaddr,0,sizeof(localifaddr));
	memset(&vlg_mcastifaddr,0,sizeof(vlg_mcastifaddr));
        
        if(result = inet_aton(pszIpAddr, &localifaddr.sin_addr) == 0)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","%s():Cannot convert IP addr for %s, ret %d\n", __FUNCTION__,pszIpAddr, result);
            VL_DSG_PROXY_PRINT_ERROR_STRING();
        }
        else
        {
            vlg_mcastifaddr.sin_family      = AF_INET;
            vlg_mcastifaddr.sin_addr.s_addr = htonl(nGroup);
            vlg_mcastifaddr.sin_port        = htons(nPort);
            unsigned char loop = 0;
            
            if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) == -1)
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","%s():setsockopt IP_MULTICAST_LOOP failed.\n", __FUNCTION__);
                VL_DSG_PROXY_PRINT_ERROR_STRING();
            }
            if (bind(sock, (struct sockaddr *)&localifaddr, sizeof(struct sockaddr_in)) == -1)
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","%s():bind to '%s' failed.\n", __FUNCTION__,inet_ntoa(localifaddr.sin_addr));
                VL_DSG_PROXY_PRINT_ERROR_STRING();
            }
            else if (setsockopt (sock, IPPROTO_IP, IP_MULTICAST_IF, &localifaddr.sin_addr, sizeof(localifaddr.sin_addr)) < 0)
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","%s():setsockopt IP_MULTICAST_IF to '%s' failed.\n", __FUNCTION__,inet_ntoa(localifaddr.sin_addr));
                VL_DSG_PROXY_PRINT_ERROR_STRING();
            }
            else
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","%s(): Created multicast socket for '%d.%d.%d.%d' at [%s]\n", __FUNCTION__,VL_EXPAND_IP_ADDRESS_FOR_PRINTF(nGroup), inet_ntoa(localifaddr.sin_addr));
                return sock;
            }
        }
        close(sock);
    }

    return -1;
}

void vlDsgProxyServer_sendMcastData(VL_DSG_CLIENT_ID_ENC_TYPE encType, unsigned long nPort, unsigned char * pData, int nBytes)
{
    if(-1 != vlg_nMcastSendSocket)
    {
        struct sockaddr_in sendtoAddr = vlg_mcastifaddr;
        unsigned long nAddress = DSGPROXY_BCAST_MULTICAST_ADDRESS_HEX;
        switch(encType)
        {
            case VL_DSG_CLIENT_ID_ENC_TYPE_APP:
            {
                nAddress = DSGPROXY_APP_MULTICAST_ADDRESS_HEX;
            }
            break;
            
            default:
            {
                nAddress = DSGPROXY_BCAST_MULTICAST_ADDRESS_HEX;
            }
            break;
        }
        sendtoAddr.sin_addr.s_addr = htonl(nAddress);
        sendtoAddr.sin_port = htons(nPort);
        int nSendBytes = sendto(vlg_nMcastSendSocket, pData, nBytes, 0, (struct sockaddr*)&sendtoAddr, sizeof(sendtoAddr));
        if(nSendBytes <= 0)
        {
            VL_DSG_PROXY_PRINT_ERROR_STRING();
        }
        else
        {
            if(rdk_dbg_enabled("LOG.RDK.POD", RDK_LOG_TRACE7))
            {
                RDK_LOG(RDK_LOG_TRACE7, "LOG.RDK.POD", "Sent %d bytes to %s:%d\n", nBytes, inet_ntoa(sendtoAddr.sin_addr), nPort);
            }
        }
    }
}

string vlDsgProxyServer_getServerIfAddress()
{VL_AUTO_LOCK(vlg_dsgProxyLock);

    string strIfName;
    char szBuf[DSGPROXY_MAX_STR_SIZE]  ={0};
    char szIfName[DSGPROXY_MAX_STR_SIZE] = {0};
    int result = 0;
    int nScanned = 0;

    result = vlDsgProxyServer_readConf(DSGPROXY_SERVER_IP_INFO_FILE, szBuf, sizeof(szBuf));
    if(result < 0)
    {
    	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s(): Failed to read from server ip info file \n", __FUNCTION__);
    }
	
    nScanned = sscanf(szBuf, "%s", szIfName);

    if(nScanned > 0)
    {
        strIfName = szIfName;
    }
    
    return strIfName;
}

void vlDsgProxyServer_closeSocket(int & rSock)
{VL_AUTO_LOCK(vlg_dsgProxyLock);
    
    if((-1 != rSock) && (0 != rSock))
    {
        close(rSock);
        rSock = -1;
    }
}

void vlDsgProxyServer_closeMulticastSendSocket()
{VL_AUTO_LOCK(vlg_dsgProxyLock);
    
    vlDsgProxyServer_closeSocket(vlg_nMcastSendSocket);
}

void vlDsgProxyServer_closeMulticastReceiveSockets()
{VL_AUTO_LOCK(vlg_dsgProxyLock);
    
    int i = 0;
    if(0 != vlg_nScte65McastReceiveThread)
    {
//        rmf_osal_threadDestroy(vlg_nScte65McastReceiveThread);
        vlg_nScte65McastReceiveThread = 0;
    }
    if(0 != vlg_nScte18McastReceiveThread)
    {
//        rmf_osal_threadDestroy(vlg_nScte18McastReceiveThread);
        vlg_nScte18McastReceiveThread = 0;
    }
    if(0 != vlg_nXaitMcastReceiveThread)
    {
//        rmf_osal_threadDestroy(vlg_nXaitMcastReceiveThread);
        vlg_nXaitMcastReceiveThread = 0;
    }
    vlDsgProxyServer_closeSocket(vlg_nScte65McastReceiveSocket);
    vlDsgProxyServer_closeSocket(vlg_nScte18McastReceiveSocket);
    vlDsgProxyServer_closeSocket(vlg_nXaitMcastReceiveSocket);
    for(i = 0; i < vlg_nAppsReceiveSockets; i++)
    {
        if(0 != vlg_aAppsReceiveThreads[i])
        {
//            rmf_osal_threadDestroy(vlg_aAppsReceiveThreads[i]);
            vlg_aAppsReceiveThreads[i] = 0;
        }
        vlDsgProxyServer_closeSocket(vlg_aAppsReceiveSockets[i]);
    }
    vlg_nAppsReceiveSockets = 0;
}

void vlDsgProxyServer_setupMulticastSendSocket()
{VL_AUTO_LOCK(vlg_dsgProxyLock);
    
    if(!vlg_bDsgProxyEnableTunnelForwarding) return;
    
    vlDsgProxyServer_closeMulticastSendSocket();
    vlDsgProxyServer_closeMulticastReceiveSockets();
    
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s(): Setting up Send Socket\n", __FUNCTION__);
    vlg_nMcastSendSocket = vlDsgProxyServer_createMulticastSendSocket(DSGPROXY_SERVER_VPN_IF_ADDRESS, DSGPROXY_BCAST_MULTICAST_ADDRESS_HEX, 0);
}

void  vl_dsg_proxy_mcast_receive_thread(void * arg)
{
    int sock = (int)arg;
    struct sockaddr_in recvFromAddr;
    socklen_t nRecvFromLen = sizeof(recvFromAddr);
    size_t nBytes = 0;
    unsigned char buf[2048]; // needs to be automatic variable on stack
    struct sockaddr_in localAddr;
    socklen_t socklen = sizeof(localAddr);
    unsigned long nIpAddress = 0;
    VL_DSG_CLIENT_ID_ENC_TYPE encType = VL_DSG_CLIENT_ID_ENC_TYPE_BCAST;
    unsigned long nLocalPort = 0;
    unsigned long nLocalAddr = 0;

    if( 0 != getsockname(sock, (struct sockaddr *)&localAddr, &socklen) )
    {
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "vl_dsg_proxy_mcast_receive_thread: getsockname failed\n");
    }

    nLocalPort = ntohs(localAddr.sin_port);
    nLocalAddr = ntohl(localAddr.sin_addr.s_addr);


    switch(nLocalAddr)
    {
        case DSGPROXY_APP_MULTICAST_ADDRESS_HEX:
        {
            encType = VL_DSG_CLIENT_ID_ENC_TYPE_APP;
        }
        break;
        
        default:
        {
            encType = VL_DSG_CLIENT_ID_ENC_TYPE_BCAST;
        }
        break;
    }
    unsigned long nClientId = nLocalPort;
    
    while(1)
    {
        nBytes = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr*)&recvFromAddr, &nRecvFromLen);
        nIpAddress = ntohl(recvFromAddr.sin_addr.s_addr);
        if(nBytes <= 0)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "vl_dsg_proxy_mcast_receive_thread: Exiting as nBytes = %d\n", nBytes, nBytes);
            break;
        }      		
	 else if( nBytes > 2048 )
	 {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "vl_dsg_proxy_mcast_receive_thread: Exiting as nBytes = %d\n", nBytes, nBytes);
            break;			
	 }
        else
        {
            if(rdk_dbg_enabled("LOG.RDK.POD", RDK_LOG_TRACE7))
            {
                RDK_LOG(RDK_LOG_TRACE7, "LOG.RDK.POD", "vl_dsg_proxy_mcast_receive_thread: Received %d bytes from %s:%d\n", nBytes, inet_ntoa(recvFromAddr.sin_addr), ntohs(recvFromAddr.sin_port));
                RDK_LOG(RDK_LOG_TRACE7, "LOG.RDK.POD", "vl_dsg_proxy_mcast_receive_thread: Received %d bytes at %s:%d\n", nBytes, inet_ntoa(localAddr.sin_addr), nLocalPort);
            }
            
            if(VL_IPV4_ADDRESS_LONG(10,8,0,1) == nIpAddress)
            {
                vlDsgDispatchToDSGCC(VL_DSG_DIR_TYPE_DIRECT_TERM_DSG_FLOW, encType, nClientId, buf, nBytes);
            }
        }
    }
    
}

void vlDsgProxyServer_setupMulticastReceiveSockets()
{VL_AUTO_LOCK(vlg_dsgProxyLock);
    
    if(!vlg_bDsgProxyEnableTunnelReceiving) return;
    
    vlDsgProxyServer_closeMulticastSendSocket();
    vlDsgProxyServer_closeMulticastReceiveSockets();
    
    vlg_nScte65McastReceiveSocket = vlDsgProxyServer_createMulticastReceiveSocket(VL_DSG_PROXY_CLIENT_VPN_IF_NAME, DSGPROXY_BCAST_MULTICAST_ADDRESS_HEX, VL_DSG_BCAST_CLIENT_ID_SCTE_65);
    vlg_nScte18McastReceiveSocket = vlDsgProxyServer_createMulticastReceiveSocket(VL_DSG_PROXY_CLIENT_VPN_IF_NAME, DSGPROXY_BCAST_MULTICAST_ADDRESS_HEX, VL_DSG_BCAST_CLIENT_ID_SCTE_18);
    vlg_nXaitMcastReceiveSocket   = vlDsgProxyServer_createMulticastReceiveSocket(VL_DSG_PROXY_CLIENT_VPN_IF_NAME, DSGPROXY_BCAST_MULTICAST_ADDRESS_HEX, VL_DSG_BCAST_CLIENT_ID_XAIT);
    
    if(-1 != vlg_nScte65McastReceiveSocket)
    {
        rmf_osal_threadCreate(vl_dsg_proxy_mcast_receive_thread, (void *)vlg_nScte65McastReceiveSocket, 0, 0, &vlg_nScte65McastReceiveThread, "vl_dsg_proxy_mcast_receive_thread");    
    }
    
    if(-1 != vlg_nScte18McastReceiveSocket)
    {
            rmf_osal_threadCreate(vl_dsg_proxy_mcast_receive_thread, (void *)vlg_nScte18McastReceiveSocket, 0, 0, &vlg_nScte18McastReceiveThread, "vl_dsg_proxy_scte18_mcast_receiver"); 
    }
    
    if(-1 != vlg_nXaitMcastReceiveSocket)
    {
        rmf_osal_threadCreate(vl_dsg_proxy_mcast_receive_thread, (void *)vlg_nXaitMcastReceiveSocket, 0, 0, &vlg_nXaitMcastReceiveThread, "vl_dsg_proxy_scteXait_mcast_receiver");
    }

    vlg_nAppsReceiveSockets = 0;
    CMHash dsgServerAttributesFile;
    dsgServerAttributesFile.load_from_disk(DSGPROXY_CLIENT_SERVER_ATTR_FILE);
    char szAppIds[8*VL_MAX_DSG_HOST_ENTRIES];

    memset(szAppIds,0,sizeof(szAppIds));
    dsgServerAttributesFile.read(DSGPROXY_DSG_APP_IDS, szAppIds, sizeof(szAppIds));
    istringstream file(szAppIds);
    while(!file.eof())
    {
        if(vlg_nAppsReceiveSockets < VL_ARRAY_ITEM_COUNT(vlg_aAppsReceiveSockets))
        {
            string strAppId = vlDsgProxyServer_getNextToken(file, ':');
            unsigned long nAppId = strtoul(strAppId.c_str(), NULL, 16);
            if(0 != nAppId)
            {
                vlg_aAppsReceiveSockets[vlg_nAppsReceiveSockets] = vlDsgProxyServer_createMulticastReceiveSocket(VL_DSG_PROXY_CLIENT_VPN_IF_NAME, DSGPROXY_APP_MULTICAST_ADDRESS_HEX, nAppId) ;
                rmf_osal_threadCreate(vl_dsg_proxy_mcast_receive_thread, (void *)vlg_aAppsReceiveSockets[vlg_nAppsReceiveSockets], 0, 0, &vlg_aAppsReceiveThreads[vlg_nAppsReceiveSockets], "vl_dsg_proxy_app_mcast_receiver");

                vlg_nAppsReceiveSockets++;
            }
        }
    }
}

int vlDsgProxyServer_CheckIfClientConnectedToServer()
{VL_AUTO_LOCK(vlg_dsgProxyLock);
    char szBuf[DSGPROXY_MAX_STR_SIZE] = {0};
    int result = vlDsgProxyServer_readConf(DSGPROXY_CLIENT_STATUS_FILE, szBuf, sizeof(szBuf));
    if(result < 0)
    {
    	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s(): Failed to read from client status file \n", __FUNCTION__);
    }	
    return (0 == strcmp(szBuf, DSGPROXY_CLIENT_CONNECTED_STATUS));
}

unsigned long vlDsgProxyServer_getInterfaceIpAddress(const char * szIfName)
{VL_AUTO_LOCK(vlg_dsgProxyLock);

    int result = 0;
    static int  sock = socket(AF_INET, SOCK_DGRAM, 0);
    unsigned long nIpAddress = 0;

    if(-1 != sock)
    {
        struct ifreq ifr;
        struct sockaddr_in sai;
        bzero(&ifr, sizeof(ifr));
        bzero(&sai, sizeof(sai));
        struct sockaddr_in  *sin  = (struct sockaddr_in  *)&(ifr.ifr_addr);
        sai.sin_family = AF_INET;
        sai.sin_port = 0;

        strncpy(ifr.ifr_name, szIfName, sizeof(ifr.ifr_name));
	 ifr.ifr_name[IFNAMSIZ - 1] = 0;
        
        sin->sin_family = AF_INET;
        if ((result = ioctl(sock, SIOCGIFADDR, &ifr)) < 0)
        {
            //VL_DSGPC_LOG("Cannot get IPV4 addr for %s, ret %d, err='%s'\n", ifr.ifr_name, result, strerror(errno));
        }
        else
        {
            memcpy((char *)&sai, (char *) &ifr.ifr_ifru.ifru_addr, sizeof(sai));
            nIpAddress = ntohl(sai.sin_addr.s_addr);
        }
    }
    
    return nIpAddress;
}

void  vl_dsg_proxy_service_thread(void * arg)
{
    while(1)
    {
        sleep(1);
        // check if any VPN interface has acquired any IP Address
        unsigned long nIpAddress = vlDsgProxyServer_getInterfaceIpAddress(VL_DSG_PROXY_CLIENT_VPN_IF_NAME);
        if(0 == nIpAddress) nIpAddress = vlDsgProxyServer_getInterfaceIpAddress("tun1");
        if(VL_IPV4_ADDRESS_LONG(10,8,0,0) == (nIpAddress&0xFFFFFF00))
        {
            if(VL_IPV4_ADDRESS_LONG(10,8,0,1) != nIpAddress)
            {
                vlg_bHostSetupProxyServerAddress = 0;
                if(0 == vlg_bHostAcquiredProxyAddress)
                {
                    vlDsgProxyServer_setupMulticastReceiveSockets();
                }
                vlg_bHostAcquiredProxyAddress = 1;
                vlDsgTouchFile(VL_DSG_IP_ACQUIRED_TOUCH_FILE);
                // notify new IP address to stack
                vlSendDsgEventToPFC(CardMgr_DSG_IP_ACQUIRED,
                                    &(vlg_HostIpInfo), sizeof(vlg_HostIpInfo));
		  /*  notfiy through POD */		
		  postDSGIPEvent((uint8_t *)&vlg_HostIpInfo.szIpAddress[0]);
		  
                vlDsgSendBootupEventToPFC(RMF_BOOTMSG_DIAG_EVENT_TYPE_ESTB_IP, RMF_BOOTMSG_DIAG_MESG_CODE_COMPLETE, 0);				
            }
            else
            {
                vlg_bHostAcquiredProxyAddress = 0;
                
                if(0 == vlg_bHostSetupProxyServerAddress)
                {
                    vlDsgProxyServer_setupMulticastSendSocket();
                }
                
                vlg_bHostSetupProxyServerAddress = 1;
            }
        }
        else
        {
            vlg_bHostAcquiredProxyAddress = 0;
            vlg_bHostSetupProxyServerAddress = 0;
        }
        RDK_LOG(RDK_LOG_TRACE2, "LOG.RDK.POD", "%s(): If Address = 0x%08X:%d.%d.%d.%d\n", __FUNCTION__, nIpAddress, VL_EXPAND_IP_ADDRESS_FOR_PRINTF(nIpAddress));
    }
    
}

VL_DSG_PROXY_RESULT vlDsgProxyServer_Start()
{VL_AUTO_LOCK(vlg_dsgProxyLock);

    static int bStarted = 0;
    if(bStarted) return VL_DSG_PROXY_RESULT_SUCCESS;
    bStarted = 1;
    const char * pszHnIfList = rmf_osal_envGet(DSGPROXY_HN_IF_LIST);
    if(NULL != pszHnIfList) vlDsgProxyServer_updateConf(DSGPROXY_HN_IF_CONF_FILE, pszHnIfList);
    vlg_bDsgProxyEnableTunnelForwarding = vl_env_get_bool(0, "FEATURE.DSG_PROXY.ENABLE_TUNNEL_FORWARDING");
    vlg_bDsgProxyEnableTunnelReceiving = vl_env_get_bool(0, "FEATURE.DSG_PROXY.ENABLE_TUNNEL_RECEIVING");
    rmf_osal_ThreadId threadid = 0;
    rmf_osal_threadCreate(vl_dsg_proxy_service_thread, NULL, 0, 0, &threadid, "vl_dsg_proxy_service_thread");
    //auto_unlock_ptr(vlg_dsgProxyLock);
    return VL_DSG_PROXY_RESULT_SUCCESS;
}

VL_DSG_PROXY_RESULT vlDsgProxyServer_Register()
{VL_AUTO_LOCK(vlg_dsgProxyLock);

    int i = 0;
    char szBuffer[VL_DSG_MAX_STR_SIZE];
    string strBuf;
    string strAttributes;
    vlg_dsgProxyAttributes.clear();
    //vlg_dsgProxyAttributes.push_back(make_pair(DSGPROXY_PUBLIC_KEY, vlg_pszDsgProxyPublicKey));
    //vlDsgProxyServer_ConvertBufferToAscii(strBuf, vlg_dsgDcdBuffer);
    //vlg_dsgProxyAttributes.push_back(make_pair(DSGPROXY_DCD_BUFFER, strBuf.c_str()));
    //vlDsgProxyServer_ConvertBufferToAscii(strBuf, vlg_dsgDirBuffer);
    //vlg_dsgProxyAttributes.push_back(make_pair(DSGPROXY_DIRECTORY_BUFFER, strBuf.c_str()));
    //vlDsgProxyServer_ConvertBufferToAscii(strBuf, vlg_dsgAdvConfigBuffer);
    //vlg_dsgProxyAttributes.push_back(make_pair(DSGPROXY_ADV_CONFIG_BUFFER, strBuf.c_str()));
    //vlDsgProxyServer_ConvertBufferToAscii(strBuf, vlg_dsgTlv217Buffer);
    //vlg_dsgProxyAttributes.push_back(make_pair(DSGPROXY_TLV_217_BUFFER, strBuf.c_str()));
    
    vlg_dsgProxyAttributes.push_back(make_pair(DSGPROXY_HOST_MAC_ADDRESS, vlg_HostIpInfo.szMacAddress));
    vlg_dsgProxyAttributes.push_back(make_pair(DSGPROXY_HOST_IPV4_ADDRESS, vlg_HostIpInfo.szIpAddress));
    vlg_dsgProxyAttributes.push_back(make_pair(DSGPROXY_HOST_IPV4_MASK, vlg_HostIpInfo.szSubnetMask));
    vlg_dsgProxyAttributes.push_back(make_pair(DSGPROXY_HOST_IPV6_GLOBAL_ADDRESS, vlg_HostIpInfo.szIpV6GlobalAddress));
    vlg_dsgProxyAttributes.push_back(make_pair(DSGPROXY_HOST_IPV6_GLOBAL_PLEN, vlDsgProxyServer_printInt(vlg_HostIpInfo.nIpV6GlobalPlen)));
    vlg_dsgProxyAttributes.push_back(make_pair(DSGPROXY_HOST_IPV6_LINK_ADDRESS, vlg_HostIpInfo.szIpV6LinkAddress));
    vlg_dsgProxyAttributes.push_back(make_pair(DSGPROXY_HOST_IPV6_LINK_PLEN, vlDsgProxyServer_printInt(vlg_HostIpInfo.nIpV6LinkPlen)));
    const char * pszTZ = getenv("TZ");
    if(NULL == pszTZ) pszTZ = vlPodGetTimeZoneString( );
    if(NULL != pszTZ)
    {
        vlg_dsgProxyAttributes.push_back(make_pair(DSGPROXY_HOST_TIME_ZONE, pszTZ));
    }
    
    VL_IP_ADDRESS_STRUCT ecmIpAddress;
    VL_MAC_ADDRESS_STRUCT ecmMacAddress;
#ifndef SNMP_IPC_CLIENT
    int ret=vlSnmpEcmGetIpAddress(&ecmIpAddress);
#ifdef GLI
    if (!ret)
    {
       IARM_Bus_SYSMgr_EventData_t eventData;
       eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_ECM_IP;
       eventData.data.systemStates.state = 1;
       eventData.data.systemStates.error = 0;
       IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
    }
#endif
    vlSnmpEcmGetMacAddress(&ecmMacAddress);
#endif //SNMP_IPC_CLIENT

    vlg_dsgProxyAttributes.push_back(make_pair(DSGPROXY_ECM_MAC_ADDRESS, ecmMacAddress.szMacAddress));
    vlg_dsgProxyAttributes.push_back(make_pair(DSGPROXY_ECM_IPV4_ADDRESS, ecmIpAddress.szIpAddress));
    vlg_dsgProxyAttributes.push_back(make_pair(DSGPROXY_ECM_IPV4_MASK, ecmIpAddress.szSubnetMask));
    vlg_dsgProxyAttributes.push_back(make_pair(DSGPROXY_ECM_IPV6_ADDRESS, ecmIpAddress.szIpV6GlobalAddress));
    vlg_dsgProxyAttributes.push_back(make_pair(DSGPROXY_ECM_IPV6_MASK, ecmIpAddress.szIpV6SubnetMask));
    
    unsigned long dsFreq  = 0;
    unsigned long dsPower = 0;
    unsigned long dsLock  = 0;
    unsigned long usFreq  = 0;
    unsigned long usPower = 0;
    unsigned long dsSnr   = 0;
#ifndef SNMP_IPC_CLIENT    
    vlSnmpEcmGetDsFrequency(&dsFreq);
    vlg_dsgProxyAttributes.push_back(make_pair(DSGPROXY_ECM_DS_FREQUENCY, vlDsgProxyServer_printInt(dsFreq)));
    vlSnmpEcmGetDsPower(&dsPower);
    vlg_dsgProxyAttributes.push_back(make_pair(DSGPROXY_ECM_DS_POWER, vlDsgProxyServer_printPower(dsPower, DSGPROXY_ECM_DS_POWER_UNITS)));
    vlSnmpEcmGetDsLockStatus(&dsLock);
    vlg_dsgProxyAttributes.push_back(make_pair(DSGPROXY_ECM_DS_LOCK_STATUS, (dsLock?DSGPROXY_ECM_DS_STATUS_LOCKED:DSGPROXY_ECM_DS_STATUS_UNLOCKED)));
    vlSnmpEcmGetUsFrequency(&usFreq);
    vlg_dsgProxyAttributes.push_back(make_pair(DSGPROXY_ECM_US_FREQUENCY, vlDsgProxyServer_printInt(usFreq)));
    vlSnmpEcmGetUsPower(&usPower);
    vlg_dsgProxyAttributes.push_back(make_pair(DSGPROXY_ECM_US_POWER, vlDsgProxyServer_printPower(usPower, DSGPROXY_ECM_DS_POWER_UNITS)));
    vlSnmpEcmGetSignalNoizeRatio(&dsSnr);
    vlg_dsgProxyAttributes.push_back(make_pair(DSGPROXY_ECM_DS_SNR, vlDsgProxyServer_printPower(dsSnr, DSGPROXY_ECM_SNR_UNITS)));
#endif // SNMP_IPC_CLIENT
    vlg_dsgProxyAttributes.push_back(make_pair(DSGPROXY_UCID, vlDsgProxyServer_printHex(vlg_ucid)));
    vlg_dsgProxyAttributes.push_back(make_pair(DSGPROXY_VCTID, vlDsgProxyServer_printHex(vlg_nDsgVctId)));


#ifndef SNMP_IPC_CLIENT
    VL_SNMP_Cable_System_Info cableSystemInfo;
    SocStbHostSnmpProxy_CardInfo_t ProxyCardInfo = {0};
    memset(&cableSystemInfo,0,sizeof(cableSystemInfo));

    vlGet_ocStbHostCard_Details(&ProxyCardInfo);
    HostCATInfo_t HostCATobj;
    vlGet_HostCaTypeInfo(&HostCATobj);

    snprintf(cableSystemInfo.szCaSystemId, sizeof(cableSystemInfo.szCaSystemId), "0x%s", HostCATobj.S_CASystemID);
    snprintf(cableSystemInfo.szCpSystemId, sizeof(cableSystemInfo.szCpSystemId), "0x%02X%02X%02X%02X", (unsigned char)ProxyCardInfo.ocStbHostCardCpIdList[0] ,(unsigned char)ProxyCardInfo.ocStbHostCardCpIdList[1],(unsigned char) ProxyCardInfo.ocStbHostCardCpIdList[2],(unsigned char)ProxyCardInfo.ocStbHostCardCpIdList[3] );
#endif SNMP_IPC_CLIENT

//    vlSnmpGetHostInfo(VL_SNMP_VAR_TYPE_CableSystemInfo, &cableSystemInfo);
#ifndef SNMP_IPC_CLIENT
    vlg_dsgProxyAttributes.push_back(make_pair(DSGPROXY_CA_SYSTEM_ID, cableSystemInfo.szCaSystemId));
    vlg_dsgProxyAttributes.push_back(make_pair(DSGPROXY_CP_SYSTEM_ID, cableSystemInfo.szCpSystemId));
#else // defined : SNMP_IPC_CLIENT
    // to get CA_SYSTEM_ID
    rmf_Error ret = RMF_OSAL_ENODATA;
	unsigned long CP_system_id_list = 0;
	unsigned short CaSystemId = 0;
	char ocStbHostCardCpIdList[4+1];//4-Bytes Display the CP_system_id_bitmask field
	char szCaSystemId[256] = {0};
	char szCpSystemId[256] = {0};
	cCardManagerIF *pCM = NULL;

	ret = podImplGetCaSystemId( &CaSystemId );
	// copy the data to buffer
	snprintf(szCaSystemId, sizeof(szCaSystemId), "0x%02X%02X", ((CaSystemId >> 8)& 0xFF), ((CaSystemId >> 0)& 0xFF));

    // to get CP_SYSTEM_ID
	pCM = cCardManagerIF::getInstance();
	
	if( pCM == NULL )
	{
		//
	}
	ret = pCM->SNMPGetCpIdList( &CP_system_id_list );
	ocStbHostCardCpIdList[0] = (CP_system_id_list>>24)&0xFF;
	ocStbHostCardCpIdList[1] = (CP_system_id_list>>16)&0xFF;
	ocStbHostCardCpIdList[2] = (CP_system_id_list>> 8)&0xFF;
	ocStbHostCardCpIdList[3] = (CP_system_id_list>> 0)&0xFF;
	// copy the data to buffer
    snprintf(szCpSystemId, sizeof(szCpSystemId), "0x%02X%02X%02X%02X", (unsigned char)ocStbHostCardCpIdList[0] ,(unsigned char)ocStbHostCardCpIdList[1],(unsigned char) ocStbHostCardCpIdList[2],(unsigned char)ocStbHostCardCpIdList[3] );

    vlg_dsgProxyAttributes.push_back(make_pair(DSGPROXY_CA_SYSTEM_ID, szCaSystemId));
    vlg_dsgProxyAttributes.push_back(make_pair(DSGPROXY_CP_SYSTEM_ID, szCpSystemId));
#endif // SNMP_IPC_CLIENT
    string strApps;
    vlg_nApps = 0;
    for(i = 0; i < vlg_DsgDirectory.nHostEntries; i++)
    {
        if(VL_DSG_CLIENT_ID_ENC_TYPE_APP == vlg_DsgDirectory.aHostEntries[i].clientId.eEncodingType)
        {
            strApps += vlDsgProxyServer_printHex((unsigned long)vlg_DsgDirectory.aHostEntries[i].clientId.nEncodedId);
            strApps += ":";
            if(vlg_nApps < VL_MAX_DSG_HOST_ENTRIES)
            {
                vlg_aApps[vlg_nApps] = (unsigned long)vlg_DsgDirectory.aHostEntries[i].clientId.nEncodedId;
                vlg_nApps++;
            }
        }
    }
    
    for(i = 0; i < vlg_DsgAdvConfig.nTunnelFilters; i++)
    {
        if(0 != vlg_DsgAdvConfig.aTunnelFilters[i].nAppId)
        {
            strApps += vlDsgProxyServer_printHex((unsigned long)vlg_DsgAdvConfig.aTunnelFilters[i].nAppId);
            strApps += ":";
            if(vlg_nApps < VL_MAX_DSG_HOST_ENTRIES)
            {
                vlg_aApps[vlg_nApps] = (unsigned long)vlg_DsgDirectory.aHostEntries[i].clientId.nEncodedId;
                vlg_nApps++;
            }
        }
    }

    vlg_dsgProxyAttributes.push_back(make_pair(DSGPROXY_DSG_APP_IDS, strApps));

    char szResolve[1024];
    int result = 0;
    result = vlDsgProxyServer_readConf(DSGPROXY_RESOLVE_CONF_FILE, szResolve, sizeof(szResolve));
    if(result < 0)
    {
    	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s(): Failed to read from resolve conf file \n", __FUNCTION__);
    }	
    vlg_dsgProxyAttributes.push_back(make_pair(DSGPROXY_RESOLVE_CONF, szResolve));
    result = vlDsgProxyServer_readConf(DSGPROXY_RESOLVE_DNSMASQ_FILE, szResolve, sizeof(szResolve));
    if(result < 0)
    {
    	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s(): Failed to read from DNSMASQ file \n", __FUNCTION__);
    }	

    vlg_dsgProxyAttributes.push_back(make_pair(DSGPROXY_RESOLVE_DNSMASQ, szResolve));

    snprintf(szBuffer, sizeof(szBuffer), "slptool register service:dsgproxy.%s://", rmf_osal_envGet(DSGPROXY_GATEWAY_DOMAIN));
    
    vlDsgProxyServer_BuildServiceAttributes(strAttributes);
    vlDsgProxyServer_updateConf(DSGPROXY_SLP_ATTR_FILE, strAttributes.c_str());
    
    vlDsgProxyServer_updateConf(DSGPROXY_SLP_SERVICE_FILE, szBuffer);
    vlDsgProxyServer_updateConf(DSGPROXY_SERVER_TWO_WAY_STATUS_FILE, DSGPROXY_SERVER_TWO_WAY_READY_STATUS);
    
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "Registered DSG Proxy Service '%s'.\n", szBuffer);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "DSG Proxy Attributes '%s'.\n", strAttributes.c_str());

    system("pgrep -f \"openvpn.*--config.*dsg_proxy_client.conf\" | xargs kill");
    vlDsgProxyServer_closeMulticastReceiveSockets();
    
    return VL_DSG_PROXY_RESULT_SUCCESS;
}

VL_DSG_PROXY_RESULT vlDsgProxyServer_Deregister(const char * pszReason)
{VL_AUTO_LOCK(vlg_dsgProxyLock);
    
    vlDsgProxyServer_updateConf(DSGPROXY_SERVER_DOCSIS_STATUS_FILE, pszReason);
    vlDsgProxyServer_updateConf(DSGPROXY_SERVER_TWO_WAY_STATUS_FILE, pszReason);
    char szBuffer[VL_DSG_MAX_STR_SIZE];
    
    snprintf(szBuffer, sizeof(szBuffer), "slptool deregister service:dsgproxy.%s://", rmf_osal_envGet("FEATURE.GATEWAY.DOMAIN"));
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "De-registered DSG Proxy Service '%s'.\n", szBuffer);
    vlDsgProxyServer_updateConf(DSGPROXY_SLP_SERVICE_FILE, szBuffer);
    vlDsgProxyServer_closeMulticastSendSocket();
    
    return VL_DSG_PROXY_RESULT_SUCCESS;
}

VL_DSG_PROXY_RESULT vlDsgProxyServer_UpdateCableSystemParams()
{VL_AUTO_LOCK(vlg_dsgProxyLock);
    
    return VL_DSG_PROXY_RESULT_SUCCESS;
}

VL_DSG_PROXY_RESULT vlDsgProxyServer_UpdateDcd(int nBytes, const unsigned char * pData)
{VL_AUTO_LOCK(vlg_dsgProxyLock);

    vlg_dsgDcdBuffer.assign(pData, pData+nBytes);

    //auto_unlock_ptr(vlg_dsgProxyLock);
	return VL_DSG_PROXY_RESULT_SUCCESS;
}

VL_DSG_PROXY_RESULT vlDsgProxyServer_UpdateAdvConfig(const unsigned char * pData, int nBytes)
{VL_AUTO_LOCK(vlg_dsgProxyLock);

    vlg_dsgAdvConfigBuffer.assign(pData, pData+nBytes);
    return VL_DSG_PROXY_RESULT_SUCCESS;
}

VL_DSG_PROXY_RESULT vlDsgProxyServer_UpdateDsgDirectory(const unsigned char * pData, int nBytes)
{VL_AUTO_LOCK(vlg_dsgProxyLock);

    vlg_dsgDirBuffer.assign(pData, pData+nBytes);
    return VL_DSG_PROXY_RESULT_SUCCESS;
}

VL_DSG_PROXY_RESULT vlDsgProxyServer_UpdateDocsisParams()
{VL_AUTO_LOCK(vlg_dsgProxyLock);
    
    return VL_DSG_PROXY_RESULT_SUCCESS;
}

VL_DSG_PROXY_RESULT vlDsgProxyServer_UpdateTlv217Params(const unsigned char * pData, int nBytes)
{VL_AUTO_LOCK(vlg_dsgProxyLock);

    vlg_dsgTlv217Buffer.assign(pData, pData+nBytes);
    return VL_DSG_PROXY_RESULT_SUCCESS;
}
