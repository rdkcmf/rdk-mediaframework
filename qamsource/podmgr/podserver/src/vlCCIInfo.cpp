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

#include "sys_init.h"
#include "pod_types.h"
#include "core_events.h"
#include "cardUtils.h"
#include "rmf_osal_event.h"
#include "rdk_debug.h"
#include "rmf_osal_sync.h"
#include "rmf_osal_thread.h"
#include "cardManagerIf.h"
#include "vlCCIInfo.h"
#include <vector>
#include "vlMutex.h"

#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif

extern "C" void vlif_FirebusApp_SetCCI(unsigned char cciValue);

rmf_osal_eventqueue_handle_t vlg_cci_infoQueue;
std::vector <CardManagerCCIData_t> gvl_ccinfolist;
CardManagerCCIData_t *pcciinfo = NULL;
static int SetImageConstraint();
void setCopyProtection(CardManagerCCIData_t CCIInfo);
static vlMutex vlg_lock;
static void vl_cciinfo_init_task(void *arg)
{
    void* pEventData = NULL;
    rmf_Error status;
    rmf_osal_event_handle_t event;
    rmf_osal_event_category_t event_category;
    uint32_t event_type;
    rmf_osal_event_params_t event_params;
	
    CardManagerCCIData_t sCCIData;
    int CCIData_event_received = 0;
    if (vlg_cci_infoQueue == 0)
    {
       RDK_LOG(RDK_LOG_DEBUG,"LOG.RDK.POD","ERROR !! vlg_cci_infoQueue = %x\n", vlg_cci_infoQueue);
        return ;
    }
    while (1)
    {
        // Get next event from CCI queue
        status = rmf_osal_eventqueue_get_next_event( vlg_cci_infoQueue,
		&event, &event_category, &event_type, &event_params);
	 	if(RMF_SUCCESS != status)
	 		{
	 		continue;
	 		}

        if (event_params.data != NULL)
        {
//            auto_lock_ptr(&vlg_lock);

            RDK_LOG(RDK_LOG_DEBUG,"LOG.RDK.POD","Got event, category = %d, type = %d \n", event_category, event_type);
            pEventData = event_params.data;

            switch (event_category)
            {
                case RMF_OSAL_EVENT_CATEGORY_CARD_MANAGER:
                {
                    switch (event_type)
                    {
                        case CardMgr_CP_CCI_Changed:
                        {
                            VL_AUTO_LOCK(vlg_lock);
                            if(!pEventData)
                            {
                              //auto_unlock_ptr(vlg_lock);
                              break; 
                            }
                            sCCIData = *((CardManagerCCIData_t*)pEventData);
                            int i = 0;
                            // HACK: Clean-up old info from list first
                            for (i = 0; i < gvl_ccinfolist.size(); i++)
                            {
                                // CardManagerCCIData_t *pcciinfo = NULL;
                                CardManagerCCIData_t & rcciinfo = gvl_ccinfolist[i];
                                //update the existing item details
                                if (rcciinfo.LTSID == sCCIData.LTSID)
                                {
                                    gvl_ccinfolist.erase(gvl_ccinfolist.begin() + i);
                                    i--;
                                }
                            }
                            int bFound = 0;
                            ///Check out the if new event data is latest then add to vector if not update to old with ltsid and pgno use list itearter
                            for (i = 0; i < gvl_ccinfolist.size(); i++)
                            {
                                // CardManagerCCIData_t *pcciinfo = NULL;
                                CardManagerCCIData_t & rcciinfo = gvl_ccinfolist[i];
                                //update the existing item details
                                if ((rcciinfo.LTSID == sCCIData.LTSID) && (rcciinfo.prgNum == sCCIData.prgNum))
                                {
                                    rcciinfo = sCCIData;
                                    bFound = 1;
                                    break;
                                }
                            }
                            if(!bFound)
                            {
                                gvl_ccinfolist.push_back(sCCIData);
                            }

                            RDK_LOG(RDK_LOG_INFO,"LOG.RDK.POD","vl_cciinfo_init_task: CardMgr_CP_CCI_Changed event received. LTS ID=%d Prog #=%d, CCI=0x%02X\n", sCCIData.LTSID, sCCIData.prgNum,
                                    sCCIData.CCI);



                #ifdef USE_1394
                if(vl_isFeatureEnabled((unsigned char *)"FEATURE.1394.SUPPORT"))
                {
                    vlif_FirebusApp_SetCCI(sCCIData.CCI);
                }
                #endif //USE_1394
				//auto_unlock_ptr(vlg_lock);
                        }
                        //SetImageConstraint(); //For Setting Component Port Image Constraint
                        break;

                        default:
                        {
                            }
                            break;
                        }//case pfcEventCardMgr::CardMgr_CP_CCI_Changed:
                        
                    } //switch
                    break;
#if 0
                        case PFC_EVENT_CATEGORY_DVR:
                          switch (event->get_type())
                          {
                            case pfcEventDVR::Dvr_CP_CCI_Changed:
                               if(!pEventData)
                            {
                              break;
                            }
                              sCCIData = *((CardManagerCCIData_t*)pEventData);
                              RDK_LOG(RDK_LOG_INFO,"LOG.RDK.POD","DVR Playback CCI CCI=0x%02X\n", sCCIData.CCI);
                              setCopyProtection(sCCIData);
                              break;
                            default : break;
                          }
                          break;
#endif                
                default:
                {
                }
                break;
                } // case PFC_EVENT_CATEGORY_CARD_MANAGER:
            }            
            else
            {
            rmf_osal_threadSleep(1000,0);
                //cSleep(1000);
            }
            rmf_osal_event_delete(event);
			
        }
    	}

extern "C" uint32_t vlCCIInfoInit()
{
    printf("In vlCCIInfoInit\n");
    rmf_osal_ThreadId threadid = 0;
    rmf_osal_eventqueue_create ( (const uint8_t*)"CCIInfoInit:vlg_cci_infoQueue",
		&vlg_cci_infoQueue);	
    rmf_osal_eventmanager_handle_t em = get_pod_event_manager();
    rmf_osal_eventmanager_register_handler(em, vlg_cci_infoQueue, RMF_OSAL_EVENT_CATEGORY_CARD_MANAGER);
    rmf_osal_threadCreate(vl_cciinfo_init_task, NULL , RMF_OSAL_THREAD_PRIOR_DFLT, RMF_OSAL_THREAD_STACK_SIZE,& threadid, "vl_cciinfo_init_task");


    return RMF_SUCCESS;
}

int GetCCIInfoElement(CardManagerCCIData_t* pCCIData,unsigned long LTSID,unsigned long programNumber)
{
    VL_AUTO_LOCK(vlg_lock);
    if(NULL == pCCIData) {
		//auto_unlock_ptr(vlg_lock);
		return 1;
    	}

    int iReturn=1;
    if (gvl_ccinfolist.size() != 0)
    {
        int i = 0;
        for (i = 0; i < gvl_ccinfolist.size(); i++)
        {
            CardManagerCCIData_t & rCCIInfo = gvl_ccinfolist[i];
            RDK_LOG(RDK_LOG_TRACE9,"LOG.RDK.POD", "GetCCIInfoElement: %d) LTSID = 0x%X, prgNum = 0x%X\n", i, rCCIInfo.LTSID, rCCIInfo.prgNum);

            if((rCCIInfo.LTSID == LTSID) && (rCCIInfo.prgNum == programNumber))
            {
                //Copy and return zero
                *pCCIData = rCCIInfo;
                iReturn = 0;
                break;
            }
        }
    }

    RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "GetCCIInfoElement: gvl_ccinfolist.size() = %d\n", gvl_ccinfolist.size());
    //auto_unlock_ptr(vlg_lock);
    return iReturn;
}
#if 0
void setCopyProtection(CardManagerCCIData_t CCIInfo)
{
    bool bImageContraint = false;
   //Store this information in decode session so that on channel change we can revert it.
    //Get CCI Information
    bImageContraint = (CCIInfo.CIT) ? true : false;

    if (bImageContraint) //CIT is set , Constrained Image Trigger
    {
      ////If CCI bit is set then change component to PipeB, Otherwise PipeA
      vlDisp_ComponentPortImageConstraint(true);
    }
    else
    {
      //set to default
      vlDisp_ComponentPortImageConstraint(false);
    }
#ifdef XONE_STB
    vlDisp_SetAPSValue(CCIInfo.APS);
#endif
    vlDisp_SetEMIValue(CCIInfo.EMI);
}
#endif
#if 0
extern "C" bool CheckForCCIValue(uint32_t tunerID)
{
    CardManagerCCIData_t CCIInfo;
    bool bImageContraint = false;
    unsigned char ltsid;
    unsigned int progNumber = 0;
    RDK_LOG(RDK_LOG_DEBUG,"LOG.RDK.POD","%s: Entering ....\n",__FUNCTION__);
        vlMpeosImpl_GetProgNumber(tunerID,&progNumber);
        vlMpeosImpl_TunerGetLTSID(tunerID, &ltsid);
    memset(&CCIInfo, 0, sizeof(CardManagerCCIData_t));
    
    VL_AUTO_LOCK(vlg_lock);
    if(GetCCIInfoElement(&CCIInfo,ltsid,progNumber) == 0)
    {
    RDK_LOG(RDK_LOG_INFO,"LOG.RDK.POD","%s: CCI event already recvd before decode start. LTS ID=%d Prog #=%d, CCI=0x%x, EMI =0x%02x, APS =0x%02x, CIT =0x%02x\n", __FUNCTION__,CCIInfo.LTSID, CCIInfo.prgNum,CCIInfo.CCI,CCIInfo.EMI,CCIInfo.APS,CCIInfo.CIT);
      bImageContraint = (CCIInfo.CIT) ? true : false;
#if 0
        //Store this information in decode session so that on channel change we can revert it.
        //Get CCI Information
            bImageContraint = (CCIInfo.CIT) ? true : false;

        if (bImageContraint) //CIT is set , Constrained Image Trigger
        {
            ////If CCI bit is set then change component to PipeB, Otherwise PipeA
            vlDisp_ComponentPortImageConstraint(true);
        }
        else
        {
            //set to default
            vlDisp_ComponentPortImageConstraint(false);
        }
#ifdef XONE_STB
    vlDisp_SetAPSValue(CCIInfo.APS);
#endif
    vlDisp_SetEMIValue(CCIInfo.EMI);
#endif
      setCopyProtection(CCIInfo);
        }
    else
    {
      RDK_LOG(RDK_LOG_INFO,"LOG.RDK.POD","%s: This is a clear channel No need to do anything\n", __FUNCTION__);
    } 
    RDK_LOG(RDK_LOG_DEBUG,"LOG.RDK.POD","%s: Exiting ....\n",__FUNCTION__);
    //auto_unlock_ptr(vlg_lock);
    return bImageContraint;
}
#endif
#if 0
int SetImageConstraint()
{
    uint32_t tunerID =-1;
    int iDecodeSessionId =0;
    //Get Current Live media tuner ID
   vlMedia_Lock();
    if (vlMedia_getActiveDecodeSessionTunerID(&tunerID,&iDecodeSessionId) == MPE_SUCCESS)
    {
       vlMedia_setActiveDecodeSessionCCIInfo(iDecodeSessionId,CheckForCCIValue(tunerID));
       RDK_LOG(RDK_LOG_INFO,"LOG.RDK.POD","%s: CCI event received. Applying accordingly to AV decode \n", __FUNCTION__);
    }
    else
    {
        RDK_LOG(RDK_LOG_WARN, "LOG.RDK.POD","%s:  Information is not matching ..... \n", __FUNCTION__);
    }
    vlMedia_unLock();
    
}


//TEST routine for Intel to test out the deinterlacing issue.
extern "C" void mpeos_TestCCI(unsigned char cci)
{VL_AUTO_LOCK(vlg_lock);

  CardManagerCCIData_t CCIData;
  printf("\n Entered CCI Value is %x\n",cci);
  CCIData.CCI = cci;
  CCIData.EMI = cci & 0x03;
  CCIData.APS = (cci & 0xC) >> 2;
  CCIData.CIT = (cci & 0x10)>>4;
  gvl_ccinfolist.push_back(CCIData);
  
  SetImageConstraint();
  //auto_unlock_ptr(vlg_lock);
  return ;
}
#endif
