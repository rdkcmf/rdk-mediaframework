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

#else
#include <list.h>

#endif
#include "sys_init.h"
#include "cardUtils.h"
#include "cmhash.h"
#include "rmf_osal_event.h"
#include "cardmanager.h"
#include "cmevents.h"
#include "appinfo.h"
#include "poddriver.h"
#include "cardManagerIf.h"
#include "rmf_osal_mem.h"
#include <string.h>
#ifdef GLI
#include "libIBus.h"
#include "sysMgr.h"
#endif
#include "rmf_osal_sync.h"
#include <assert.h>
#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif

#define __MTAG__ VL_CARD_MANAGER

vlCardAppInfo_t CardAppInfo;

extern "C" int aiGetOpenedResId();
extern "C" void aiRcvServerQuery (unsigned char *data, int dataLen);
extern "C" int TestServerQuery(unsigned char *data, int dataLen);    
cAppInfo::cAppInfo(CardManager *cm,char *name):CMThread(name)
{
    aiMutex = 0;	
    rmf_osal_mutexNew(&aiMutex);
    event_queue = 0;
    this->cm = cm;
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","cAppInfo::cAppInfo() =%p\n",(void *)this);
    
}
        
    
cAppInfo::~cAppInfo(){
    rmf_osal_mutexDelete(aiMutex);
}
    

void cAppInfo::initialize(void){

    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    rmf_osal_eventqueue_handle_t eventqueue_handle;

    rmf_osal_eventqueue_create ( (const uint8_t* ) "cAppInfo",
		&eventqueue_handle);

//PodMgrEventListener    *listener;

     rmf_osal_eventmanager_register_handler(
		em,
		eventqueue_handle,
		RMF_OSAL_EVENT_CATEGORY_APP_INFO);

        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","cAppInfo::initialize()\n");

        this->event_queue = eventqueue_handle;
        //listener = new PodMgrEventListener(cm, eq,"ResourceManagerThread);

        
        //listener->start();
        
    memset(&CardAppInfo,0, sizeof(vlCardAppInfo_t));
}

 
void    cAppInfo::Send_app_info_request(void){
    pod_appInfo_t  appInfo;
    
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","cAppInfo::Send_app_info_request Entered >>>>>>>>>> \n");
    appInfo.displayRows = this->cDispCapabilities.GetRows();  
    
        appInfo.displayCols = this->cDispCapabilities.GetColumns();
    this->cDispCapabilities.GetScrolling(appInfo.vertScroll, appInfo.horizScroll);
     
    appInfo.multiWin = this->cDispCapabilities.GetDisplayTypeSupport();
    appInfo.dataEntry = this->cDispCapabilities.GetDataEntrySupport();
    this->cDispCapabilities.GetHTMLSupport(appInfo.html,appInfo.link,appInfo.form,appInfo.table,appInfo.list, appInfo.image);
    //appInfo.html = 0;
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," ## Send_app_info_request:appInfo.html:%X appInfo.link:%X appInfo.form : %X appInfo.table:%X appInfo.list:%X  appInfo.image:%X \n",appInfo.html,appInfo.link,appInfo.form,appInfo.table,appInfo.list, appInfo.image);
    cPOD_driver::POD_Send_app_info_request(&appInfo);
}

#define VL_MMI_STORAGE_SIZE_FOR_JAVA 0x1000//4k
static unsigned char *vlg_pMMIMessageStorage=NULL;
static unsigned long vlg_MMIMessageSizeReceived=0;

int cAppInfo::vlAppInfoGetMMIMessageFromCableCard(unsigned char **pData,unsigned long *pSize)
{
    *pSize = vlg_MMIMessageSizeReceived;
    if((vlg_pMMIMessageStorage == NULL) ||  ( vlg_MMIMessageSizeReceived == 0 ) )
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","vlAppInfoGetMMIMessageFromCableCard:vlg_pMMIMessageStorage:0x%08X vlg_MMIMessageSizeReceived:%d bytes \n",vlg_pMMIMessageStorage,vlg_MMIMessageSizeReceived);
        
        return -1;
    }
    *pData = vlg_pMMIMessageStorage;
    
    return 0;
}
extern int vlg_ServerQueryReply;
extern int vlg_ServerQueryNum;
extern int vlg_SrvQryReplyLen;
extern unsigned char *vlg_SrvQryReplyData;
void cAppInfo::run(void )
{
    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
	
    vlg_MMIMessageSizeReceived = 0;
	rmf_osal_memAllocP(RMF_OSAL_MEM_POD, VL_MMI_STORAGE_SIZE_FOR_JAVA,(void **)&vlg_pMMIMessageStorage);
    if(vlg_pMMIMessageStorage == NULL)
    {
        
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","ppInfoEventListener::Run() Mem Allocation failed !!! \n");
    }
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","AppInfoEventListener::Run()=%p\n",(void *)this);
    aiInit();
    while (1)
    {
        LCSM_EVENT_INFO *pEventInfo;
        rmf_osal_event_handle_t event_handle_rcv, event_handle_snd;
        rmf_osal_event_params_t event_params_rcv = {0}, event_params_snd = {0};
        rmf_osal_event_category_t event_category;
        uint32_t event_type;	
    
        int result = 0;
        char *ptr;
        //for RMF_OSAL_EVENT_CATEGORY_APP_INFO  
        rmf_osal_eventqueue_get_next_event( event_queue,
		&event_handle_rcv, &event_category, &event_type, &event_params_rcv);
       
        pEventInfo = (LCSM_EVENT_INFO *) event_params_rcv.data;

        switch (event_category)
        {
            case RMF_OSAL_EVENT_CATEGORY_APP_INFO:
        
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","cAppInfo::run: RMF_OSAL_EVENT_CATEGORY_APP_INFO ### \n");
                if( pEventInfo == NULL ) 
                {     
                       RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","cAppInfo::run: pEventInfo is NULL !!\n");
                       break;				
                }

                if((pEventInfo->event)  == POD_AI_APPINFO_CNF)
                {
                    CreateAppInfoList(pEventInfo);
                    //Save this data      
                }
                else if((pEventInfo->event)  == POD_SERVER_REPLY)
                {
                    if (ParseServerReply(pEventInfo) != 0)
                    {
                    
                        //resend Query
                    }
                        
                }
                else if( pEventInfo->event  == POD_APDU)
                {
                            ptr = (char *)pEventInfo->y;// y points to the complete APDU

                    if(*(ptr+2) == 0x21/*POD_APPINFO_CNF*/)
                    {                  // APDU is Application_Info_Cnf
                        
                        aiProc((void *)pEventInfo);
                            
                    }
                    else if((*(ptr+2) == 0x23 )&& 1/*this->cm->IsOCAP()*/)
                    {    //POD_APPINFO_SERVERREPLY

                        //Send Raw Response APDU to OCAP
                        if(this->cm->pCMIF->cb_fn)
                        //callback
                        {
                        //lock mutex
                            rmf_osal_mutexAcquire(aiMutex);
                            this->cm->pCMIF->cb_fn ((uint8_t *)pEventInfo->y,(uint16_t)pEventInfo->z );
                        
                            // unlock mutex
                            rmf_osal_mutexRelease(aiMutex);
                        }
                        else //send event
                        {
                            // moved event creation below to fix memory leak
                            if(vlg_pMMIMessageStorage)
                            {
                                rmf_osal_memcpy(vlg_pMMIMessageStorage , (uint8_t *)pEventInfo->y, (uint16_t)pEventInfo->z,
										VL_MMI_STORAGE_SIZE_FOR_JAVA, pEventInfo->z );
                                vlg_MMIMessageSizeReceived = (unsigned long)pEventInfo->z;
                            }
                            if((vlg_MMIMessageSizeReceived != 0) && (vlg_pMMIMessageStorage!= NULL))
                            {
                              unsigned char Len,LenSize;
                              unsigned char *pData,status;
                              unsigned short Length,TrNum,HdrLen,fileLen;
                              pData = vlg_pMMIMessageStorage;
                              pData += 3;// for tag
                              Len = *pData;// length
                              if(Len & 0x80)
                              {
                                //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","reply data is more than 127 bytes \n");
                                LenSize = (*pData & 0x7F);
                                if(LenSize <= 2)
                                {
                                  if(LenSize == 1)
                                  {
                                    pData++;
                                    Length = *pData;
                                  }
                                  else if(LenSize == 2)
                                  {
                                    pData++;
                                    Length = (*pData << 8) &0xFF00;
                                    pData++;
                                    Length |= *pData;
                                  }
                            
                                //  RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," >>>>>>> Length:%d <<<<<<<<< \n",Length);
                                }
                                else
                                {
                                  pData++;
                                  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD"," Length size error :%d \n",LenSize);
                                } 
                              }
                              else
                              {
                                Length= *pData;
                                //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," >>>>>>> @Length:%d <<<<<<<<< \n",Length);
                              }
                              pData++;
                              TrNum = *pData;
                              
                              pData++;
                              status = *pData;
                              pData++;
                             
                              HdrLen = (unsigned short)((*pData << 8) & 0xFF00);
                              pData++;
                              HdrLen |= *pData;
                              pData++;
                              RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD",">>>> TrNum:%d status:%d HdrLen:%d \n",TrNum,status,HdrLen);
                              pData += HdrLen;//Usually HdrLen = 0
                              
                              fileLen = (unsigned short)((*pData << 8) & 0xFF00);
                              pData++;
                              fileLen |= *pData;
                              pData++;
                              if(TrNum == vlg_ServerQueryNum)
                              {
                                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," vlg_ServerQueryNum Mathc found\n ");
                                  vlg_SrvQryReplyLen = fileLen;
                                vlg_SrvQryReplyData = pData;
                                vlTransferCcAppInfoPage((const char*)(pData), fileLen);
                                vlg_ServerQueryReply = 1;//signal completion only after vlg_SrvQryReplyLen and vlg_SrvQryReplyData are populated.
                              }
                              else
                              {
                                //do a dispatch event here
                                EventCableCard_RawAPDU *pEventApdu;
				    rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(EventCableCard_RawAPDU),(void **)&pEventApdu);				

                                if(NULL != pEventApdu)
                                {
                                    pEventApdu->resposeData = new uint8_t[(uint16_t)pEventInfo->z];

                                    if(NULL != pEventApdu->resposeData)
                                    {
                                        vlg_ServerQueryReply = 0;
                                        vlg_SrvQryReplyLen = 0;
                                        vlg_SrvQryReplyData = NULL;
                                        rmf_osal_memcpy(pEventApdu->resposeData , (uint8_t *)pEventInfo->y, (uint16_t)pEventInfo->z,
											pEventInfo->z, pEventInfo->z); //The size is same as denoted in the ticket.
                                        pEventApdu->dataLength  =  (uint16_t)pEventInfo->z;
                                        pEventApdu->sesnId = (short)pEventInfo->x;
    					     event_params_snd.priority = 0; //Default priority;
   					     event_params_snd.data = (void *)pEventApdu;
   					     event_params_snd.data_extension = sizeof(EventCableCard_RawAPDU);
   					     event_params_snd.free_payload_fn = EventCableCard_RawAPDU_free_fn;
   					     rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_CM_RAW_APDU, card_Raw_APDU_response_avl, 
   							&event_params_snd, &event_handle_snd);   
   					     rmf_osal_eventmanager_dispatch_event(em, event_handle_snd);
						 

                                    }
                                    else
                                    {
                                        rmf_osal_memFreeP(RMF_OSAL_MEM_POD, (void *)pEventApdu);
                                    }
                                }
                              }
                            }
                            
                    
                        }                        
                
                    }
                    break;
                }
                else
                {
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","AppInfoEventListener::PFC_EVENT_CATEGORY_APP_INFO\n");
                    rmf_osal_mutexAcquire(aiMutex);
                    aiProc((void *)pEventInfo);
                    rmf_osal_mutexRelease(aiMutex);
                }                    
                break;
            default:
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","AppInfoEventListener::default\n");
                break;
        }
		  
        rmf_osal_event_delete(event_handle_rcv);       
        
    }
}
#define TEST_CARD_DIAG
#ifdef TEST_CARD_DIAG

static unsigned char TestDiagBuff[20][250];
static unsigned char NumberOfDiagPages = 0;
static unsigned char DiagReport = 0;
#endif



static void PrintCardAppInfo(vlCardAppInfo_t *pAppInfo)
{
    int ii;
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","\n  ########### Cable Card AppInfo Received ####### \n");
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","  CardManufactureId:0x%X \n",pAppInfo->CardManufactureId);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","  CardVersion:0x%X \n",pAppInfo->CardVersion);
if( (aiGetOpenedResId() & 0x3F) == 2)
{
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","  Card MAC:0x%02X:%02X:%02X:%02X:%02X:%02X \n",pAppInfo->CardMAC[0],pAppInfo->CardMAC[1],pAppInfo->CardMAC[2],pAppInfo->CardMAC[3],pAppInfo->CardMAC[4],pAppInfo->CardMAC[5]);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","  CardSerialNumberLen:0x%X \n",pAppInfo->CardSerialNumberLen);
    pAppInfo->CardSerialNumber[pAppInfo->CardSerialNumberLen] =  0;
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","  CardSerialNumber:%s:0x",pAppInfo->CardSerialNumber);
    for(ii = 0; ii < pAppInfo->CardSerialNumberLen; ii++)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%02X ",pAppInfo->CardSerialNumber[ii]);
    }
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","\n");
}
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD",">>>>>>>  CardNumberOfApps:0x%X  <<<<<<<<\n",pAppInfo->CardNumberOfApps);
    for(ii=0; ii < pAppInfo->CardNumberOfApps; ii++)
    {
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","{ AppIndex:%d\n",ii);
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","    AppType:0x%X\n",pAppInfo->Apps[ii].AppType);
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","    VerNum:0x%X\n",pAppInfo->Apps[ii].VerNum);
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","    AppNameLen:0x%X\n",pAppInfo->Apps[ii].AppNameLen);
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","    AppName:%s\n",pAppInfo->Apps[ii].AppName);
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","    AppUrlLen:0x%X\n",pAppInfo->Apps[ii].AppUrlLen);
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","    AppUrl:%s\n",pAppInfo->Apps[ii].AppUrl);
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","}\n");
    }
}

static void cAppInfo_getModelName(char * pszModelName, int nCapacity)
{
    if((NULL == pszModelName) || (nCapacity <= 0)) return;
    /* stub */
}

void cAppInfo::CreateAppInfoList(LCSM_EVENT_INFO *pEventInfo)
{
//    uint8_t    *dataPtr = (uint8_t    *)event->eventInfo.dataPtr;
//    pfcEventCableCard_AI    *eventAI = 0;
    cApplicationInfoList    *pAppListPtr;    
//    uint16_t    ManfID, verNum; 
    int         i,count=0;
    unsigned char vlURL[200]; 
    int vlURLLen,ii;    
#ifdef GLI 
    IARM_Bus_SYSMgr_EventData_t eventData;
    eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_CA_SYSTEM;
#endif
    memset(&CardAppInfo,0, sizeof(vlCardAppInfo_t));
    cSimpleBitstream b((unsigned char*)pEventInfo->dataPtr, pEventInfo->dataLength);    /* point to our new packet */
     #ifdef TEST_CARD_DIAG
        NumberOfDiagPages = 0;
    #endif
    this->cm->pCMIF->mfgID = b.get_bits(16);    //b.skip_bits(4);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","cAppInfo::CreateAppInfoList: ManfID = %x\n", this->cm->pCMIF->mfgID);


    /* stub */
    int manufacturer = (this->cm->pCMIF->mfgID & 0xFF00) >> 8;

    switch (manufacturer)
    {
        case 0:
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: cablecard manufacturer: motorola\n", __FUNCTION__);
#ifdef GLI
            eventData.data.systemStates.state = 2 ;
#endif
            break;

        case 1:
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: cablecard manufacturer: cisco\n", __FUNCTION__);
#ifdef GLI
            eventData.data.systemStates.state = 1 ;
#endif
            break;

        case 2:
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: cablecard manufacturer: SCM\n", __FUNCTION__);
#ifdef GLI
            eventData.data.systemStates.state = 0 ;
#endif
            break;

        default:
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: cablecard manufacturer: UNKNOWN\n", __FUNCTION__);
#ifdef GLI
            eventData.data.systemStates.state = 0 ;
#endif
            break;
    }
#ifdef GLI
    IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
#endif

    #define MODEL_NAME_STR_SIZE 64
    char szOldModelName[MODEL_NAME_STR_SIZE] = "";
    cAppInfo_getModelName(szOldModelName, sizeof(szOldModelName));
    /* stub data set */ 
    
    char szNewModelName[MODEL_NAME_STR_SIZE] = "";
    cAppInfo_getModelName(szNewModelName, sizeof(szNewModelName));
    
    if(0 != strncmp(szOldModelName, szNewModelName, MODEL_NAME_STR_SIZE))
    {
        int i = 10;
        do
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s: Model Name changed from '%s' to '%s'. This should not happen at next boot.\n", __FUNCTION__, szOldModelName, szNewModelName);
            
        }while(i-- >= 0);
        
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: Repopulating DHCP options as Model Name changed from '%s' to '%s'...\n", __FUNCTION__, szOldModelName, szNewModelName);
        rmf_Error result = RMF_SUCCESS;
        if ( RMF_SUCCESS != (result = vlMpeosPopulateDhcpOptions()) )
        {
            RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SYS",
                     "vlMpeosPopulateDhcpOptions() returned %d\n", result );
        }
    }
    else
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: No need to repopulate DHCP options as the previous Model Name: '%s' ...matches the current one: '%s'.\n", __FUNCTION__, szOldModelName, szNewModelName);
    }


    CardAppInfo.CardManufactureId = (unsigned short)this->cm->pCMIF->mfgID;

    this->cm->pCMIF->cableCardVersion = b.get_bits(16);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","cAppInfo::CreateAppInfoList: Version# = %x\n", this->cm->pCMIF->cableCardVersion);

    CardAppInfo.CardVersion = (unsigned short)this->cm->pCMIF->cableCardVersion;

if( (aiGetOpenedResId() & 0x3F) >= 2)
{
    for(ii = 0; ii < 6; ii++)
        CardAppInfo.CardMAC[ii] = b.get_bits(8);

    this->cm->pCMIF->cableCardSerialNumLen = b.get_bits(8);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","cAppInfo::CreateAppInfoList: cableCardSerialNumLen# = %x\n", this->cm->pCMIF->cableCardSerialNumLen);

    CardAppInfo.CardSerialNumberLen = 0;
    for(ii = 0; ii < this->cm->pCMIF->cableCardSerialNumLen; ii++)
    {
        if(ii < sizeof(CardAppInfo.CardSerialNumber) )
        {
            CardAppInfo.CardSerialNumber[ii] = b.get_bits(8);
            CardAppInfo.CardSerialNumberLen++;
        }
        else
        {
            b.get_bits(8);
        }
    }
    

}
    i = b.get_bits(8);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","cAppInfo::CreateAppInfoList: Apps# = %x\n", i);

    this->cm->pCMIF->AppInfoList.clear();
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","cAppInfo::CreateAppInfoList: AppInfoList = %d\n", this->cm->pCMIF->AppInfoList.size());
    CardAppInfo.CardNumberOfApps = (unsigned char)i;

    while(i--){
        pAppListPtr = new cApplicationInfoList;


        pAppListPtr->type      = b.get_bits(8);
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","cAppInfo::CreateAppInfoList: App type = %x\n",pAppListPtr->type  );
        CardAppInfo.Apps[count].AppType = (unsigned char) pAppListPtr->type;

        pAppListPtr->version   = b.get_bits(16);
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","cAppInfo::CreateAppInfoList: App version = %x\n",pAppListPtr->version   );
        CardAppInfo.Apps[count].VerNum = (unsigned short)pAppListPtr->version;

        pAppListPtr->nameLength       = b.get_bits(8);
        CardAppInfo.Apps[count].AppNameLen = (unsigned char)pAppListPtr->nameLength;

        pAppListPtr->Name = new uint8_t[(pAppListPtr->nameLength) + 1]; //+1 is for adding NULL for displaying string

        
/*        memcpy(pAppListPtr->Name,(void *) ((unsigned long)(event->eventInfo.dataPtr) +((b.get_pos()) >> 3)), pAppListPtr->nameLength);*/
        memcpy(pAppListPtr->Name,(void *) ((unsigned long)(pEventInfo->dataPtr) +((b.get_pos()) >> 3)), pAppListPtr->nameLength);

        rmf_osal_memcpy((void *)CardAppInfo.Apps[count].AppName,(void *) pAppListPtr->Name,pAppListPtr->nameLength, 
						sizeof(CardAppInfo.Apps[count].AppName), pAppListPtr->nameLength );

        
        b.skip_bits((pAppListPtr->nameLength) * 8);// 8= bits in  a Byte
         
        
        pAppListPtr->urlLength = b.get_bits(8);

        CardAppInfo.Apps[count].AppUrlLen = (unsigned char)pAppListPtr->urlLength;
        
        pAppListPtr->URL       = new uint8_t[(pAppListPtr->urlLength) + 1];

        
/*        memcpy(pAppListPtr->URL,(void *)((unsigned long)(event->eventInfo.dataPtr) +((b.get_pos()) >> 3)), pAppListPtr->urlLength);*/
        memcpy(pAppListPtr->URL,(void *)((unsigned long)(pEventInfo->dataPtr) +((b.get_pos()) >> 3)), pAppListPtr->urlLength);
        rmf_osal_memcpy((void *)CardAppInfo.Apps[count].AppUrl,(void *) pAppListPtr->URL, pAppListPtr->urlLength,
			sizeof(CardAppInfo.Apps[count].AppUrl), pAppListPtr->urlLength );

        
#ifdef TEST_CARD_DIAG
	if( NumberOfDiagPages > 19 )
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","cAppInfo::Exceeded NumberOfDiagPages \n");
		assert(0);
	}
        memcpy(&TestDiagBuff[NumberOfDiagPages][0],(void *)((unsigned long)(pEventInfo->dataPtr) +((b.get_pos()) >> 3)), pAppListPtr->urlLength);
        TestDiagBuff[NumberOfDiagPages][pAppListPtr->urlLength]=0;
        NumberOfDiagPages++;
#endif        
        if(i == 1)
        {
        rmf_osal_memcpy(vlURL,(void *)((unsigned long)(pEventInfo->dataPtr) +((b.get_pos()) >> 3)), pAppListPtr->urlLength,
						sizeof(vlURL), pAppListPtr->urlLength);
        vlURLLen= pAppListPtr->urlLength;
        }

        pAppListPtr->URL[pAppListPtr->urlLength] = 0; //adding Null char
        pAppListPtr->Name[pAppListPtr->nameLength] = 0;//adding Null char
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","cAppInfo::CreateAppInfoList: App URL=%s\n",pAppListPtr->URL);
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","cAppInfo::CreateAppInfoList :App Name=%s\n",pAppListPtr->Name);
        b.skip_bits((pAppListPtr->urlLength) * 8); // 8= bits in  a Byte
        
        this->cm->pCMIF->AppInfoList.push_back(pAppListPtr);
        #ifdef TEST_CARD_DIAG
        DiagReport = 1;
        #endif // TEST_CARD_DIAG
        count++;
    }
        
    if((uint8_t *)pEventInfo->dataPtr) // allocated in POD stack
    {   //PPR (10/04/2008)
        //delete((uint8_t    *)event->eventInfo.dataPtr);
		rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pEventInfo->dataPtr);
    }
    PrintCardAppInfo(&CardAppInfo);    
    //aiRcvServerQuery (vlURL, vlURLLen);    
    //do a dispatch event here
    rmfStartCableCardAppInfoPageMonitor();          
    vlMpeosPostAppInfoChangedEvent();
}

void vlAppInfoFreeMem(void *pData)
{
    if(pData)
    {
		rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pData);
    }
}

#if 0
void CDLTriggerTest()
{
    rmf_osal_eventmanager_handle_t  em = get_pod_event_manager();
    rmf_osal_event_handle_t event_handle;
    rmf_osal_event_params_t event_params = {0};	
    char apkt[16];

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","\n >>>>>>>>>>>>>>>>>>>>   CDL Trigger Test <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n");

    event_params.priority = 0; //Default priority;
    event_params.data =(void *) apkt;
    event_params.data_extension = 0;
    event_params.free_payload_fn = vlAppInfoFreeMem;
    rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_COM_DOWNL_MANAGER, pfcEventComDownL::CommonDownL_Test_case, 
		&event_params, &event_handle);
    rmf_osal_eventmanager_dispatch_event(em, event_handle);
}
#endif

#ifdef TEST_CARD_DIAG
void CableCardDiagTest( int opt, int Index)
{
    unsigned char  NumPages = NumberOfDiagPages;
    int pageLen;
    int ii;
    
   RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","\n\n\n%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n");
   RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","  CableCardDiagTest Function  Entered MenuOpt:%d and Index:%d _______________ \n",opt,Index);
   RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n\n\n");
    if(DiagReport == 0)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD"," CableCardDiagTest: Diag Appinfo is not Reported by the Card \n");
        return;
    }
   switch(opt)
   {
    case 1:
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","\n---------------- Display Diage Pages -------------------------\n\n");
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","    Number of pages Available:%d \n\n",NumberOfDiagPages);
        for(ii = 0; ii < NumPages; ii ++)
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Index:%d Page:%s \n",ii,&TestDiagBuff[ii][0]);
        }
        break;
    case 2:
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","\n---------------- Selected Diag Paga  -------------------------\n");
        pageLen = strlen((const char*)&TestDiagBuff[Index][0]);
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Index:%d Page:%s pageLen:%d\n",Index,&TestDiagBuff[Index][0],pageLen);
        TestServerQuery(&TestDiagBuff[Index][0],pageLen);
        break;
    default:
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","CableCardDiagTest:: Incorrect Menu Option \n");
        break;
   }
}
#endif// TEST_CARD_DIAG
uint8_t cAppInfo::ParseServerReply(LCSM_EVENT_INFO *pEventInfo)
{
 
    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    EventCableCard_AI    *pEventAI = 0;
//    uint8_t    *dataPtr = (uint8_t    *)event->eventInfo.dataPtr;
//    int i;
    rmf_osal_event_handle_t event_handle;
    rmf_osal_event_params_t event_params = {0};
	
    cSimpleBitstream b((unsigned char*)pEventInfo->dataPtr, pEventInfo->dataLength);
    uint8_t                  dialogNum,fileStatus;
    uint16_t         headerLength,fileLength;

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","ParseServerReply\n");
    

    //for(i=0; i< event->eventInfo.dataLength ; i++)
    //{
    //    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%c  ", *dataPtr++);
    //}    

    rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(EventCableCard_AI),(void **)&pEventAI);
    
    pEventAI->fileLength = fileLength = 0;
    
    
    dialogNum    = b.get_bits(8);
    pEventAI->dialogNum = dialogNum;
    fileStatus   = b.get_bits(8);
    pEventAI->fileStatus = (ServerReplyStatus_t )fileStatus;
    headerLength = b.get_bits(16);
    pEventAI->header = 0;
    pEventAI->file = 0;
    if(fileStatus == FILE_OK)
    {
        //get header bytes
        pEventAI->header = new uint8_t[headerLength];


        pEventAI->headerLength = headerLength;
/*        memcpy(pEventAI->header, ((unsigned char*)event->eventInfo.dataPtr) + (b.get_pos() >> 3) , headerLength);*/
        memcpy(pEventAI->header, ((unsigned char*)pEventInfo->dataPtr) + (b.get_pos() >> 3) , headerLength );
        b.skip_bits( headerLength * 8 ); // 8= bits in  a Byte
        
        fileLength = b.get_bits(16);
        
        //get file data bytes
        
        pEventAI->file = new uint8_t[fileLength];


        pEventAI->fileLength = fileLength;
/*        memcpy(pEventAI->file, ((unsigned char*)event->eventInfo.dataPtr) + (b.get_pos() >> 3) , fileLength);*/
        memcpy(pEventAI->file, ((unsigned char*)pEventInfo->dataPtr) + (b.get_pos() >> 3) , fileLength );
        
        
    }
    else
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","File STatus not OK\n");    //if FIle Status is not ok resend query
     
    
    if((uint8_t *)pEventInfo->dataPtr)
    {
        //PPR. (10/04/2008)
        //delete [] (uint8_t    *)event->eventInfo.dataPtr;
		rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pEventInfo->dataPtr);
    }

    event_params.priority = 0; //Default priority;
    event_params.data = (void *) pEventAI;
    event_params.data_extension = sizeof(EventCableCard_AI);
    event_params.free_payload_fn = EventCableCard_AI_free_fn;
    rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_CardManagerAIEvents, card_server_reply, 
		&event_params, &event_handle);
    
    //do a dispatch event here
    rmf_osal_eventmanager_dispatch_event(em, event_handle);
     
    return fileStatus;
}



//------------------------------------------------------------------------------------------------------------------------------------------

//Implementing class    cApplicationInfoList


cApplicationInfoList::cApplicationInfoList()
{

    this->URL = 0;
    this->urlLength = 0;
    this->Name = 0;
    this->nameLength = 0;
    this->type =   0;
    this->version = 0;

}


cApplicationInfoList::~cApplicationInfoList()
{
    if(this->URL)
    {
        //VLFREE_INFO2(this->URL);
        delete [](this->URL);
    }
        
    if(this->Name)
    {
        //VLFREE_INFO2(this->Name);
        delete [](this->Name);        
    }
}


uint16_t cApplicationInfoList::getURLLength()
{
    return urlLength;
}

CMStatus_t cApplicationInfoList::getURL(uint8_t *appUrl)
{
    memcpy(appUrl, URL, urlLength+1); 
    return CM_NO_ERROR;
}

uint16_t   cApplicationInfoList::getNameLength()
{
    return nameLength;
}


CMStatus_t cApplicationInfoList::getName(uint8_t *appName)
{
    memcpy (appName, Name, nameLength+1);
    return CM_NO_ERROR;
}

PodAppType_t     cApplicationInfoList::getType()
{
    return (PodAppType_t) type;
}

int cApplicationInfoList::getVersionNumber()
{
    return version;
    
}
// VL: RAJASREE: 09262007: Added method implementations [end]

//------------------------------------------------------------------------------------------------------------------------------------------


void EventCableCard_AI_free_fn(void *ptr)
{

    EventCableCard_AI *event = (EventCableCard_AI *)ptr;
    
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","EventCableCard_AI:deleting allocated memory\n");
    if(event)
    {
          if(event->file)
          {
              delete [] event->file;
          }
      
          if(event->header)
          {
              delete [] event->header;    
          }
          rmf_osal_memFreeP(RMF_OSAL_MEM_POD, event);		  
    }            
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET","EventCableCard_AI:deleting allocated memory -done\n");
}



