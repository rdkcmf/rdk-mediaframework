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


#include <unistd.h>
#ifdef GCC4_XXX
#include <list>
#else
#include <list.h>
#endif


#include "cardUtils.h"
#include "cmhash.h"
#include "rmf_osal_event.h"
#include "core_events.h"

#include "cardmanager.h"
#include "poddriver.h"
#include "oob_rdc.h"
/////////////////////////
#include "oob.h"
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <net/if_arp.h>
#include "ip_types.h"
#include "bufParseUtilities.h"
#include "dsgResApi.h"
#include "xchanResApi.h"
#include "vlXchanFlowManager.h"
#include "utilityMacros.h"
#include "rmf_osal_sync.h"
#include "netUtils.h"
#include "rmf_osal_mem.h"
#include "coreUtilityApi.h"

#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif

#define __MTAG__ VL_CARD_MANAGER


#include <string.h>

//DECLARE_LOG_APPTYPE (LOG_APPTYPE_OCAP_PFC_PLUGINS_CORESERVICES_CARDMANAGER);



static CardManager * vlg_pCardManager = NULL;
static cExtChannelFlow *vlg_pXFlowObj = NULL;
static VL_HOST_IP_INFO vlg_PodIpInfo;
static VL_HOST_IP_INFO vlg_HostIpInfo;
int vlg_bHostAcquiredQpskIPAddress = 0;

rmf_osal_Mutex vlXChanGetOobThreadLock()
{

    static rmf_osal_Mutex lock ; 
    static int i =0;
    if(i==0)
    {		
   	rmf_osal_mutexNew(&lock);
       i =1;		
    }
    return lock;

}

void vlXchanCancelQpskOobIpTask()
{
    if(NULL != vlg_pCardManager)
    {
        rmf_osal_mutexAcquire(vlXChanGetOobThreadLock());
        {
            if(NULL != vlg_pXFlowObj)
            {
                vlg_pXFlowObj->ResetFlowID();
            }

            if(vlg_pCardManager->sockFd > 0)
            {
                close(vlg_pCardManager->sockFd);
                vlg_pCardManager->sockFd = -1;
            }

            if(NULL != vlg_pCardManager->pOOB_packet_listener_thread)
            {
                // Sep-24-2008: Allowing call to Cancel
                vlg_pCardManager->pOOB_packet_listener_thread->bCancelled = 1;
                vlg_pCardManager->pOOB_packet_listener_thread = NULL;
            }
        }
        rmf_osal_mutexRelease(vlXChanGetOobThreadLock());
    }
}


cOOB::cOOB(CardManager *c,char *name):CMThread(name)
{

    this->cm = c;
    vlg_pCardManager = c;
    bCancelled = 0;
}


void cOOB::initialize(void)
{
    bCancelled = 0;
}

#define VL_EXT_CHANNEL_IPU_TICKS_PER_SEC    (1000000)
#define VL_EXT_CHANNEL_IPU_DELAY            ((20)*(VL_EXT_CHANNEL_IPU_TICKS_PER_SEC))
#define VL_EXT_CHANNEL_IPU_RETRY            ((10)*(VL_EXT_CHANNEL_IPU_TICKS_PER_SEC))
#define VL_EXT_CHANNEL_SERVICE_INTERVAL     (( 1)*(VL_EXT_CHANNEL_IPU_TICKS_PER_SEC))

#ifndef USE_DSG //Venu
int  vlIsIpduMode(void)
{
    return 0;
}
int  vlIsDsgOrIpduMode(void)
{
    return 0;
}
#endif

void cOOB::run(void ){

//struct ifreq ifr;
int result;
//struct oob_data *pData;
//int i = 0;
cExtChannelFlow *pXFlowObj=0;
//static unsigned char udpBuf[8192];

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","cOOB::run: Thread Starting...\n");
    // inserted: Jan-16-2008: delay 60 seconds before requesting for IPU from card
    XFlowStatus_t retvalue = FLOW_NOT_STARTED;
//    ptimer.delay(VL_EXT_CHANNEL_IPU_DELAY);
    rmf_osal_threadSleep(0,  VL_EXT_CHANNEL_IPU_DELAY);
    // Jan-16-2008: avoid deletion of pXFlowObj
    pXFlowObj = new cExtChannelFlow(this->cm->pCMIF);


    pXFlowObj->SetServiceType(CM_SERVICE_TYPE_IP_U);
    pXFlowObj->ResetFlowID();
    this->cm->pIP_flow = pXFlowObj;

    vlXchanGetDsgIpInfo(&vlg_HostIpInfo);

    vlDsgSendBootupEventToPFC(RMF_BOOTMSG_DIAG_EVENT_TYPE_ESTB_IP, RMF_BOOTMSG_DIAG_MESG_CODE_START, 0);
    vlg_bHostAcquiredQpskIPAddress = 0;

    while((vl_xchan_session_is_active()) && (!bCancelled) && (retvalue != FLOW_CREATED) && (!vlIsDsgOrIpduMode()))
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","cOOB::run: sending new_flow_req-IP flow\n");

        //uint8_t    macAddr[6] = {0x00, 0x0A, 0x73, 0x6B, 0x0F, 0x8C};
        //uint8_t    macAddr[6] = {0x00, 0x0a, 0x73, 0x6b, 0x0f, 0x87};
        // Use VL OUI for OOB-QPSK RDC IPU flow
        //uint8_t macAddr[6] = {0x00, 0x09, 0x89, 0x6b, 0x0f, 0x87};
        // Oct-13-2008: Changed MAC assignment to make space for VL units
        uint8_t macAddr[6] = {0x00, 0x09, 0x89, 0xff, 0xf0, 0x01};
        static unsigned char aOptions[320];
        int nOptionLen = 0;
        VL_BYTE_STREAM_ARRAY_INST(aOptions, WriteBuf);

        /*
        // subnet mask
        nOptionLen += vlWriteShort(pWriteBuf, 0x0104);
        nOptionLen += vlWriteLong(pWriteBuf, 0xFFFFFF00);
        // router
        nOptionLen += vlWriteShort(pWriteBuf, 0x0304);
        nOptionLen += vlWriteLong(pWriteBuf, 0xFFFFFFFF);
        // time to live
        nOptionLen += vlWriteShort(pWriteBuf, 0x1701);
        nOptionLen += vlWriteByte(pWriteBuf, 0x7F);
        // ip lease time
        nOptionLen += vlWriteShort(pWriteBuf, 0x3304);
        nOptionLen += vlWriteLong(pWriteBuf, 0xFFFFF);
        // server identifier
        nOptionLen += vlWriteShort(pWriteBuf, 0x3604);
        nOptionLen += vlWriteLong(pWriteBuf, 0xFFFFFFFF);
        // Dec-06-2008: CPE option 60: used only by Card
        nOptionLen += vlWriteByte(pWriteBuf, 60);
        nOptionLen += vlWriteByte(pWriteBuf, sizeof("OpenCable2.0"));
        nOptionLen += vlWriteBuffer(pWriteBuf, (unsigned char *)"OpenCable2.0", sizeof("OpenCable2.0"), sizeof(aOptions));
        */

        // Jan-16-2008: avoid deletion of pXFlowObj

        // Jan-19-2010: use dsg if mac address
        //pXFlowObj->SetMacAddress(macAddr);
        pXFlowObj->SetMacAddress(vlg_HostIpInfo.aBytMacAddress);
        pXFlowObj->SetOptionByteLength(nOptionLen );
        pXFlowObj->SetOptionBytes( aOptions);// i
        // changed: aug-06-2007: prototype changed
        pXFlowObj->cb_fn = cOOB::OOB_RDC_cb;
        pXFlowObj->cb_obj = this->cm; //callback context
        pXFlowObj->StartFiltering();

        if((!bCancelled) && (!vlIsDsgOrIpduMode()))
        {
            retvalue = (XFlowStatus_t)(pXFlowObj->RequestExtChannelFlow());
        }
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","CardManager: creating IP flow retvalue is '%s'\n",cExtChannelGetFlowStatusName(retvalue));

        switch(retvalue)
        {
            default:
            {
                if(retvalue != FLOW_CREATED) // error
                {
                    // commeted: Jan-16-2008: avoid deletion of pXFlowObj
                    /*RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","deleting XFlowObj retvalue=%x\n",retvalue);
                    if(pXFlowObj)
                    {
                        //VLFREE_INFO2(pXFlowObj);
                        delete pXFlowObj;
                        pXFlowObj = 0;
                    }

                    this->cm->pIP_flow = 0;*/
                    // changed: Jan-16-2008: changed to 10 seconds
//                    ptimer.delay(VL_EXT_CHANNEL_IPU_RETRY);
                    rmf_osal_threadSleep(0,  VL_EXT_CHANNEL_IPU_RETRY);
                    continue;

                }
            }
        }

        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","flow ID for IP FLOW is %x status =%x\n",pXFlowObj->GetFlowID(),pXFlowObj->GetStatus());
        if(pXFlowObj->GetStatus() == 0)
        {
            //Get the IP address returned by POD
//            char cmd_buffer[50];
            VL_IP_IF_PARAMS ipParams = pXFlowObj->GetIpuParams();
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","PODMAIN: ip address = %d.%d.%d.%d\n",
                   ipParams.ipAddress[0], ipParams.ipAddress[1],
                   ipParams.ipAddress[2], ipParams.ipAddress[3]);
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","********** SETUP IP FLOW *************\n");
            //Open socket
            if(VL_VALUE_4_FROM_ARRAY(ipParams.ipAddress) != 0)
            {
                int result;
//                struct ifreq ifr;
                this->cm->sockFd = socket(AF_INET, SOCK_DGRAM, 0/*IPPROTO_UDP*/);
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","sockFd= %x\n", this->cm->sockFd);
                if(this->cm->sockFd > 0)
                {
                    ipParams.socket = this->cm->sockFd;
                    // the follwing mac address may be ignored by the hal
                    ipParams.flowid = pXFlowObj->GetFlowID();
                    if(0 == VL_VALUE_4_FROM_ARRAY(ipParams.subnetMask))
                    {
                        // set default subnet mask
                        ipParams.subnetMask[0] = 255;
                        ipParams.subnetMask[1] = 255;
                        ipParams.subnetMask[2] = 192;
                        ipParams.subnetMask[3] = 0;
                    }
                    // Jan-19-2010: use hard-coded mac address to setup the if
                    memcpy(ipParams.macAddress, macAddr, sizeof(macAddr));
                    result = this->cm->pPOD_driver->oob_control(VL_OOB_COMMAND_INIT_IF, &ipParams);
                    //start another thread that listens to packets from the oob driver
                    //    pOOB_packet_listener_thread = new cOOB(this->cm,"POD-OOB-PacketListener-thread");

                    //    pOOB_packet_listener_thread->initialize();
                    //    pOOB_packet_listener_thread->start();
                    vlXchanGetPodIpInfo(&vlg_PodIpInfo);

                    // check router
                    if(0 != VL_VALUE_4_FROM_ARRAY(ipParams.router))
                    {
                        // set route
                        vlNetSetNetworkRoute(0,
                                             VL_VALUE_4_FROM_ARRAY(ipParams.subnetMask),
                                             VL_VALUE_4_FROM_ARRAY(ipParams.router),
                                             vlg_PodIpInfo.szIfName);
                    }
                
                    // hardcoded for SA legacy headend
                    vlNetSetNetworkRoute(VL_IPV4_ADDRESS_LONG(172, 0, 0, 0),
                                         VL_IPV4_ADDRESS_LONG(255, 0, 0, 0),
                                         VL_IPV4_ADDRESS_LONG(  0, 0, 0, 0),
                                         vlg_PodIpInfo.szIfName);
#ifndef INTEL_CANMORE
                    // hardcoded for VL-SA legacy headend
                    vlNetSetNetworkRoute(VL_IPV4_ADDRESS_LONG(192, 168, 100, 0),
                                         VL_IPV4_ADDRESS_LONG(255, 255, 255, 0),
                                         VL_IPV4_ADDRESS_LONG(172,  20,   1, 1),
                                         vlg_PodIpInfo.szIfName);
#endif
                    // de-commision eCM's role as gateway
                    vlXchanDelDefaultEcmGatewayRoute();
                    // create default route to reach networks other than 10.x.x.x and 172.x.x.x
                    vlXchanSetNetworkRoute(0, 0, 0, vlg_PodIpInfo.szIfName);
                    
                    rmf_osal_threadSleep(2000, 0);   // 2 seconds
#ifdef USE_DSG
                    vlSendDsgEventToPFC(CardMgr_POD_IP_ACQUIRED,
                                        &(vlg_PodIpInfo), sizeof(vlg_PodIpInfo));
                    vlDsgSendBootupEventToPFC(RMF_BOOTMSG_DIAG_EVENT_TYPE_ESTB_IP, RMF_BOOTMSG_DIAG_MESG_CODE_COMPLETE, 0);
#endif
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","cOOB::run: POD Acquired QPSK IP Address: %d.%d.%d.%d\n",
                           vlg_PodIpInfo.aBytIpAddress[0], vlg_PodIpInfo.aBytIpAddress[1],
                           vlg_PodIpInfo.aBytIpAddress[2], vlg_PodIpInfo.aBytIpAddress[3]);
                    
                    vlg_bHostAcquiredQpskIPAddress = 1;
                }
                break;
            }
            else
            {
                retvalue = FLOW_NOT_STARTED;
                // commeted: Jan-16-2008: avoid deletion of pXFlowObj
                /*pXFlowObj->DeleteExtChannelFlow();
                //VLFREE_INFO2(pXFlowObj);
                delete pXFlowObj;
                pXFlowObj = 0;


                this->cm->pIP_flow = 0;*/
                // changed: Jan-16-2008: changed to 10 seconds
//                ptimer.delay(VL_EXT_CHANNEL_IPU_RETRY);
                rmf_osal_threadSleep(0,  VL_EXT_CHANNEL_IPU_RETRY);
                continue;

            }
        }
    }

    // prepare to clean-up after pod-removal
    VL_IP_IF_PARAMS ipParams = pXFlowObj->GetIpuParams();
    ipParams.flowid = pXFlowObj->GetFlowID();

    // register this object
    rmf_osal_mutexAcquire(vlXChanGetOobThreadLock());
    {
        vlg_pXFlowObj = pXFlowObj;
    }
    rmf_osal_mutexRelease(vlXChanGetOobThreadLock());

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","cOOB::run: Thread Waiting...\n");
    while((cExtChannelFlow::INVALID_FLOW_ID != pXFlowObj->GetFlowID()) && (vl_xchan_session_is_active()) && (!bCancelled) && (!vlIsDsgOrIpduMode()))
    {
//        ptimer.delay(VL_EXT_CHANNEL_SERVICE_INTERVAL);
        rmf_osal_threadSleep(0,  VL_EXT_CHANNEL_SERVICE_INTERVAL);

    }
    // pod-removed so de-commision the IP-U flow.
    pXFlowObj->DeleteExtChannelFlow();
    vlXchanUnregisterFlow(pXFlowObj->GetFlowID());
    result = this->cm->pPOD_driver->oob_control(VL_OOB_COMMAND_STOP_IF, &ipParams);

    // unreg this object
    rmf_osal_mutexAcquire(vlXChanGetOobThreadLock());
    {
        if(pXFlowObj == vlg_pXFlowObj)
        {
            vlg_pXFlowObj=NULL;
        }
        if(this == vlg_pCardManager->pOOB_packet_listener_thread)
        {
            vlg_pCardManager->pOOB_packet_listener_thread = NULL;
        }
        //VLFREE_INFO2(pXFlowObj);
        delete pXFlowObj;
    }
    rmf_osal_mutexRelease(vlXChanGetOobThreadLock());

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","cOOB::run: Thread Exiting...\n");
    /* Sep-24-2008: Allowing thread to exit
    while(1)
    {
        ptimer.delay(VL_EXT_CHANNEL_SERVICE_INTERVAL);
    }
    */
#if 0 //ifdef VL_UNUSED_CODE // Disabled: Sep-18-2007
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","********** START OOB UDP TEST *************\n");

    while (!this->get_cancelled())
    {
        while(this->cm->sockFd > 0)
        {
            while (!this->get_cancelled())
            {
                char szData[] = "hello how are you";
                VL_OOB_IP_DATA ipData;
                ipData.socket    = cm->sockFd;
                ipData.nPort     = 55555;
                vlStrCpy(ipData.szIpAddress, "172.20.1.14", sizeof(ipData.szIpAddress));
                ipData.nLength   = sizeof(szData);
                ipData.nCapacity = sizeof(szData);
                ipData.pData  = szData;
                result = cm->pPOD_driver->oob_control(VL_OOB_COMMAND_WRITE_DATA, &ipData);
                if(result < 0)
                {
                    perror("VL_OOB_COMMAND_WRITE_DATA failed");
                }
                ptimer.delay(1000000);
            }

            while (!this->get_cancelled())
            {
            // receive packet
            //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","WAITING FOR data from OOB driver %x\n",this->cm->sockFd);
                VL_OOB_IP_DATA ipData;
                ipData.socket    = this->cm->sockFd;
                ipData.nLength   = 0;
                ipData.nCapacity = sizeof(udpBuf);
                ipData.pData  = udpBuf;
                result = this->cm->pPOD_driver->oob_control(VL_OOB_COMMAND_READ_DATA, &ipData);
                if(result < 0)
                {
                    perror("VL_OOB_COMMAND_READ_DATA failed");
                    ptimer.delay(1000000);
                }
                else
                {
                    if(ipData.nLength)
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","data length = %d\n", ipData.nLength);

                    vlMpeosDumpBuffer(RDK_LOG_INFO, "LOG.RDK.POD", ipData.pData, ipData.nLength);

                //send this data through POD
                    if((this->cm->pIP_flow) && (ipData.nLength))
                    {
                        this->cm->pIP_flow->SendIPData((uint8_t *)ipData.pData, (unsigned int)ipData.nLength , (uint32_t)(this->cm->pIP_flow->GetFlowID()));
                    }
                }
            }
        }
        //ptimer.delay(1000000);
    }
#endif
#if 0 // defined(VL_UNUSED_CODE) // Disabled: Sep-18-2007
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","********** START OOB NET TEST *************\n");

    while(this->cm->sockFd > 0)
    {
        /*memcpy(ifr.ifr_name, "oob", 4, sizeof(ifr.ifr_name));*/
        vlStrCpy(ifr.ifr_name, "podnet0", sizeof(ifr.ifr_name));
        //ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
        //ifr.ifr_hwaddr.sa_data[0] = 0x00; ifr.ifr_hwaddr.sa_data[1] = 0xe0;
        //ifr.ifr_hwaddr.sa_data[2] = 0x36; ifr.ifr_hwaddr.sa_data[3] = 0x00;
        //ifr.ifr_hwaddr.sa_data[4] = 0xc6; ifr.ifr_hwaddr.sa_data[5] = 0xc8;

		rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(struct oob_data),(void **)&ifr.ifr_data);
        if (ifr.ifr_data == NULL)
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Error: cannot allocate memory\n");
        }

        while (!this->get_cancelled())
        {
    // receive packet
            //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","WAITING FOR data from OOB driver %x\n",this->cm->sockFd);
            result = ioctl(this->cm->sockFd, OOB_IOCTL_READ, &ifr);
            if(result < 0)
            {
                perror("ioctl(READ) failed");
            }
            else
            {
                pData = (struct oob_data*)ifr.ifr_data;

                if(pData->uiLen)
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","data length = %d\n", pData->uiLen);

                while (i < pData->uiLen)
                {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","0x%02x ", pData->pucBuf[i]);
                    i++;
                    if ((i % 0x10) == 0)
                    {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","\n");
                    }
                }

                //send this data through POD
                if((this->cm->pIP_flow) && (pData->uiLen))
                {
                    this->cm->pIP_flow->SendIPData((uint8_t *)pData->pucBuf, (unsigned int)pData->uiLen , (uint32_t)(this->cm->pIP_flow->GetFlowID()));

                }
           }


        }
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","exiting cOOB thread\n");
        if(ifr.ifr_data)
			rmf_osal_memFreeP(RMF_OSAL_MEM_POD, ifr.ifr_data);
    }
#endif // VL_UNUSED_CODE

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","cOOB::run: thread exiting...\n");
}



// changed: aug-06-2007: prototype changed
int  cOOB::OOB_RDC_cb(void *data, void *buf, int bufSize, unsigned long * pnRet)
{
int result;
//struct ifreq ifr;
//struct oob_data *pData;
CardManager *cm = (CardManager *)data;
//int i;
//unsigned char *bufPtr = (unsigned char *)buf;

#if 0
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","OOB_RDC_cb  size = %x\n",bufSize);
    for(i=0; i< bufSize; i++)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%x  ", bufPtr[i]);

    }
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","\n");
    /*memcpy(ifr.ifr_name, "oob", 4, sizeof(ifr.ifr_name));*/
    vlStrCpy(ifr.ifr_name, "podnet0", sizeof(ifr.ifr_name));
    //ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
    //ifr.ifr_hwaddr.sa_data[0] = 0x00; ifr.ifr_hwaddr.sa_data[1] = 0xe0;
    //ifr.ifr_hwaddr.sa_data[2] = 0x36; ifr.ifr_hwaddr.sa_data[3] = 0x00;
    //ifr.ifr_hwaddr.sa_data[4] = 0xc6; ifr.ifr_hwaddr.sa_data[5] = 0xc8;
	rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(struct oob_data),(void **)&pData);
    pData->uiLen = bufSize;
    ifr.ifr_data = (char *)pData;
    if (ifr.ifr_data == NULL)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Error: cannot allocate memory\n");
    }
    else
    {
        /*memcpy(pData->pucBuf , buf, bufSize, sizeof(pData->pucBuf));*/
        memcpy(pData->pucBuf , buf, bufSize, sizeof(pData->pucBuf));
         result = ioctl(cm->sockFd, OOB_IOCTL_WRITE, &ifr);
        if(ifr.ifr_data)
			rmf_osal_memFreeP(RMF_OSAL_MEM_POD, ifr.ifr_data);
    }
#else
        if((NULL != cm) && (NULL != buf))
        {
            VL_OOB_IP_DATA ipData;
            ipData.socket    = cm->sockFd;
            ipData.nPort     = 55555;
            memset(ipData.szIpAddress, 0, sizeof(ipData.szIpAddress));
            strncpy(ipData.szIpAddress, "172.20.1.7", sizeof(ipData.szIpAddress)-1);
            ipData.nLength   = bufSize;
            ipData.nCapacity = bufSize;
            ipData.pData  = buf;
            result = cm->pPOD_driver->oob_control(VL_OOB_COMMAND_WRITE_DATA, &ipData);
            if(result < 0)
            {
                perror("VL_OOB_COMMAND_WRITE_DATA failed");
            }
        }
#endif

    return 0;
}

