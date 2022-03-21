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

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>

//#include "pfcresource.h"
#include "cardUtils.h"
#include "cmhash.h"
#include "rmf_osal_event.h"
#include "core_events.h"
#include "tunerdriver.h"
#include "cardmanager.h"
#include "poddriver.h"
#define __MTAG__ VL_CARD_MANAGER

#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif


//#include "pfcpluginbase.h"



//#define RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET",a, ...)    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", a, ## __VA_ARGS__ )
//#define RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET",a, ...)        { }

//PFC_REGISTER_PLUGIN(CardManager)
static cableCardDriver* pCableCardDriver;


    CardManager::CardManager(){//: pfcResource("CardManager"){
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","CardManager::CardManager()\n");
        ptuner_driver = NULL; 
        pPOD_driver = NULL; 
        pCP_Protection = NULL; 
        pCardMibAcc = NULL;  
        pHostAddPrt = NULL;
        pHeadEndComm = NULL;
        pDsg = NULL;
#ifdef USE_IPDIRECT_UNICAST
        pIpDirectUnicast = NULL;
#endif
        pDiagnostic = NULL;
        pGenFeatureControl = NULL;
        pSysControl = NULL;
        pMMI_handler = NULL;
        pCMIF = NULL;
        pOOB_packet_listener_thread = NULL;
        pPodMainThread = NULL;
        pRsMgr   = NULL;
         pAppInfo = NULL;
        pMMI     = NULL;
        pHoming     = NULL;
        pXChan     = NULL;
        pSysTime = NULL;
        pHostControl = NULL;
        pCA     = NULL;
        pCardRes     = NULL;
        pSAS     = NULL;
        ocapMode = 0;
        CaAPDURoute = 0;
        ulPhysicalTunerID[0] = 0;
        cablecardFlag = CARD_REMOVED;
        CmcablecardFlag = CM_CARD_REMOVED;
        pIP_flow = 0;
        sockFd   = -1;

    }
    CardManager::~CardManager(){
        //delete card_lock;
        ////VLFREE_INFO2(card_lock);
        //delete get_card_lock;
        ////VLFREE_INFO2(get_card_lock);
        if(pPodMainThread)
        {
            //VLFREE_INFO2(pPodMainThread);
            delete pPodMainThread;
        }

    }


int CardManager::initialize(void){

//    RegisterCallBack();
    pOOB_packet_listener_thread = NULL;
    pCMIF = cCardManagerIF::getInstance(/*pfc_kernel_global*/);
     pCMIF->CMInstance = this;

//Prasad - Moved this part of initialization to init_complete.
#if 0
    pPodMainThread = new cPodPresenceEventListener(this,"vlPOD-POLLINGThread");


    pPodMainThread->initialize();
    pPodMainThread->start();

    ptuner_driver      = new ctuner_driver(this);


      pPOD_driver = new cPOD_driver(this);


     if(pPOD_driver->initialize() == -1)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD"," POD_driver initialize failed..\n");
         return 1;
    }
#endif
    return 0;
}

int CardManager::init_complete()
{
    pPodMainThread = new cPodPresenceEventListener(this,"vlPOD-POLLINGThread");


    pPodMainThread->initialize();
    pPodMainThread->start();

    ptuner_driver     = new ctuner_driver(this);


    pPOD_driver = new cPOD_driver(this);


    if(pPOD_driver->initialize() == -1)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD"," POD_driver initialize failed..\n");
        return 1;
    }

    return 0;
}
//






int CardManager::is_default ()
{
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","CardManager::is_default\n");
    return 1;
}



void    CardManager::StartPODThreads(CardManager *cm)
{


    pRsMgr    = new cRsMgr(cm,"vlCardManager_ResourceManagerThread");


    pRsMgr->initialize();
    pRsMgr->start();
    while(!pRsMgr->running());

    pAppInfo = new cAppInfo(cm,"vlCardManager_AppInfoThread");


    pAppInfo->initialize();
    pAppInfo->start();
    while(!pAppInfo->running());

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","\n Calling the  Send_app_info_request >>> \n");
    pAppInfo->Send_app_info_request();


     pMMI     = new cMMI(cm,"vlCardManager_MMI_Thread");


    pMMI->initialize();
    pMMI->start();
    while(!pMMI->running());

    pXChan   = new cExtChannelThread(cm,"vlCardManager_ExtCh_Thread");


    pXChan->initialize();
    pXChan->start();
    while(!pXChan->running());


    pCA     = new cCA(cm,"vlCardManager_CA_Thread");


    pCA->initialize();
    pCA->start();
    while(!pCA->running());


    /*RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ####################################################################\n");
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ####################################################################\n");
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ####################################################################\n");
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ####################################################################\n");
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ####################################################################\n");
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," ############### Initializing the CARD RES   ########################\n");*/
    pCardRes = new cCardRes(cm,"vlCardManager_CardRes_Thread");


    pCardRes->initialize();
    pCardRes->start();
    while(!pCardRes->running());

    pDsg = new cDsgThread(cm,"vlCardManager_Dsg_Thread");


    pDsg->initialize();
    pDsg->start();
    while(!pDsg->running());

#ifdef USE_IPDIRECT_UNICAST    
    pIpDirectUnicast = new cIpDirectUnicastThread(cm,"vlCardManager_IpDirectUnicast_Thread");
    pIpDirectUnicast->initialize();
    pIpDirectUnicast->start();
    while(!pIpDirectUnicast->running());
#endif // USE_IPDIRECT_UNICAST    
    
#if 1
     pHoming  = new cHoming(cm,"vlCardManager_Homing_Thread");

    pHoming->initialize();
    pHoming->start();


    while(!pHoming->running());
 #endif

    pSysTime = new cSysTime(cm,"vlCardManager_SysTime_Thread");


    pSysTime->initialize();
    pSysTime->start();
    while(!pSysTime->running());


    pHostControl = new cHostControl(cm,"vlCardManager_HostControl_Thread");


     pHostControl->initialize();
     pHostControl->start();


    while(!pHostControl->running());
#if 1
RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," ########### calling  cCP_Protection ################# \n");
     pCP_Protection     = new cCP_Protection(cm,"vlCardManager_CP_Thread");


    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," ########### calling  pCP_Protection->initialize ################# \n");
    pCP_Protection->initialize();
    pCP_Protection->start();
    while(!pCP_Protection->running());
#endif

#if 1
    pSAS     = new cSAS(cm,"vlCardManager_SAS_Thread");


    pSAS->initialize();
    pSAS->start();
      while(!pSAS->running());


     pDiagnostic = new cDiagnostic(cm,"vlCardManager_Diag_Thread");


    pDiagnostic->initialize();
    pDiagnostic->start();
    while(!pDiagnostic->running());


    pGenFeatureControl = new cGenFeatureControl(cm,"vlCardManager_Feature_Thread");


    pGenFeatureControl->initialize();
    pGenFeatureControl->start();
    while(!pGenFeatureControl->running());

     pSysControl = new cSysControl(cm,"vlCardManager_SystemControl_Thread");


     pSysControl->initialize();
     pSysControl->start();
    while(!pSysControl->running());
#endif
    pCardMibAcc = new cCardMibAcc(cm,"vlCardManager_CardMibAcc_Thread");


    pCardMibAcc->initialize();
    pCardMibAcc->start();
    while(!pCardMibAcc->running());


    pHostAddPrt = new cHostAddPrt(cm,"vlCardManager_hostAddPrt_Thread");


    pHostAddPrt->initialize();
    pHostAddPrt->start();
    while(!pHostAddPrt->running());

    pHeadEndComm = new cHeadEndComm(cm,"vlCardManager_headEndComm_Thread");


    pHeadEndComm->initialize();
    pHeadEndComm->start();
    while(!pHeadEndComm->running());


    pMMI_handler = new cMMI_Handler(cm,"vlCardManager_MMI_Handler_Thread");


    pMMI_handler->initialize();
    pMMI_handler->start();

     while(!pMMI_handler->running());

}

void CardManager::StopPODThreads()
{
    pRsMgr->end();
    pRsMgr = 0;

    pAppInfo->end();
    pAppInfo  = 0;

    pMMI->end();
    pMMI = 0;

    pXChan->end();
    pXChan = 0;

    pCA->end();
     pCA = 0;

    pHoming->end();
    pHoming = 0;

    pSysTime->end();
    pSysTime = 0;

    pHostControl->end();
    pHostControl = 0;

 //   pCP_Protection->end();
 //   pCP_Protection = 0;

    pSAS->end();
    pSAS = 0;

     pDiagnostic->end();
    pDiagnostic = 0;

    pGenFeatureControl->end();
    pGenFeatureControl = 0;

//    pSysControl->end();
//    pSysControl = 0;

    pMMI_handler->end();
    pMMI_handler = 0;


}

void set_cablecard_device(cableCardDriver* pDevice)
{
     pCableCardDriver = pDevice;

}
cableCardDriver* get_cablecard_device()
{
           return pCableCardDriver;
}



#if 0
void  pdtTunerNotifyFn( DEVICE_HANDLE_t         hDevicehandle,
                       DEVICE_LOCK_t        eStatus,
                       UINT32                ulFrequency,
                       TUNER_MODE_t            enTunerMode,
                       UINT32                ulSymbolRate,
                       void                 *pulData)
{

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","CardManager: Tuner Notification status=%x\  mode=%x  symRate=%x\n",eStatus,enTunerMode,ulSymbolRate);
}

#endif






