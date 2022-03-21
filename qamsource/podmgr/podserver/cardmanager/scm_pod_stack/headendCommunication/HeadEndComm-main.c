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


 
//----------------------------------------------------------------------------
//
// System includes
#include <errno.h>

#include <pthread.h>
#include <global_event_map.h>
#include <global_queues.h>
#include "poddef.h"
#include <lcsm_log.h>

#include "utils.h"
#include "link.h"
#include "transport.h"
#include "session.h"
#include "appmgr.h"
#include "vlEnv.h"


#include "HeadEndComm.h"
#include "HeadEndComm-main.h"
#include "podhighapi.h"
#include <memory.h>
#include "core_events.h"
#include "cardUtils.h"
#include "rmf_osal_event.h"
#include <string.h>
#ifndef SNMP_IPC_CLIENT
#include "vlSnmpHostInfo.h"
#endif //SNMP_IPC_CLIENT
#include "hal_pod_api.h"
#include <rdk_debug.h>
#include "vlMutex.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <rmf_osal_thread.h>
#include <rmf_osal_resreg.h>
#include <rmf_osal_util.h>
#include <podimpl.h>

#ifdef GLI
#include "libIBus.h"
#include "sysMgr.h"
#endif
#include <coreUtilityApi.h>
#define VL_INIT_TEXT    "init"
#define VL_COLD_INIT_TEXT    "cold.init."
#define VL_WAREHOUSE_INIT_TEXT      "wh.reset."
#define VL_REFRESH_TEXT      "refresh."

#define DAC_INIT_LOG_ENTRY          "DAC_INIT"
#define WAREHOUSE_INIT_LOG_ENTRY    "WAREHOUSE_INIT"
#define COLD_INIT_LOG_ENTRY         "COLD_INIT"
#define REFRESH_INIT_LOG_ENTRY      "REFRESH"
extern "C" int fpEnableUpdates(int bEnable);
#ifdef __cplusplus
extern "C" {
#endif


#define MAX_CMD_LENGTH 2048
#define VL_ADD_HRV_SCRIPT_PARAMETER(szCommand, param)  { \
                                                            const char * pszParam = ((param)?(" " #param "=Y"):(" " #param "=N"));\
                                                            if(( strnlen(szCommand, MAX_CMD_LENGTH) + strnlen(pszParam, MAX_CMD_LENGTH) )<MAX_CMD_LENGTH)\
                                                            {\
                                                                strncat( szCommand, pszParam, (MAX_CMD_LENGTH - strnlen(szCommand, MAX_CMD_LENGTH)));\
                                                                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "  %s\n", pszParam); \
                                                            }\
                                                            else\
                                                            {\
                                                                RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.POD","%s No memory Currentsize[%d], Req. Size [%d] \n",  __FUNCTION__, MAX_CMD_LENGTH, ( strnlen(szCommand, MAX_CMD_LENGTH) + strnlen(pszParam, MAX_CMD_LENGTH) ));\
                                                            }\
                                                       }
#define VL_HRV_ENABLE 1

#define handle headEndCommLCSMHandle


#define CARD_MOTO     0
#define CARD_CISCO    1
#define CARD_UNKNOWN  (-1)


// Prototypes
void headEndCommProc(void*);
static int process_head_end_comm_vector_ack(uint16 sessNum, uint8 TranId,int Time);
// Globals
LCSM_DEVICE_HANDLE           headEndCommLCSMHandle = 0;
LCSM_TIMER_HANDLE            headEndCommTimer = 0;
static pthread_t             headEndCommThread;
static pthread_mutex_t       headEndCommLoopLock;
static int                   loop;
int vlg_bHrvInitWithResetInProgress = 0;

static LCSM_TIMER_HANDLE vlg_HETimeHandle  = 0;

// ******************* Session Stuff ********************************
//find our session number and are we open?
//scte 28 states that only 1 session active at a time
//this makes the implementation simple; we'll use two
//static globals
//jdmc
// below, may need functions to read/write with mutex pretection
static USHORT headEndCommSessionNumber = 0;
static int    headEndCommSessionState  = 0;
// above
static ULONG  ResourceId    = 0; 
static UCHAR  Tcid          = 0;       
static ULONG  tmpResourceId = 0; //tmp for binding
static UCHAR  tmpTcid       = 0; //tmp for binding
static UCHAR  Vector_data[20];
static UCHAR  vlg_Vector_size = 0;
static UCHAR vlg_TranId = 0;
static unsigned long vlg_headEndCommWaitTime = 0;
static unsigned long gHeadCommInProgress = 0;
#define POD_HEAD_END_COMM_TIMEOUT 0x10
#define VL_EE_FPRTTAI_ENABLED   "/opt/gzenabled"

rmf_osal_Bool IsHeadendCommInProgress( )
{
	return gHeadCommInProgress;
}

static void rmfCardManagerPostInitEventSetTime_task(void *param)
{
#define TIME_RESET_FRONTPANEL_AFTER_INIT_EVENT       60

    struct stat st;
    bool ret=true;
    RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD","%s: \n", __FUNCTION__ );  
    gHeadCommInProgress =0;	
    if((0 == stat(VL_EE_FPRTTAI_ENABLED, &st) && (0 != st.st_ino)))
    {
        int i =0;
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD",
                  "%s: wait for 60 seconds to set the front panel...\n", __FUNCTION__);

  /*      for ( i=0 ; i< TIME_RESET_FRONTPANEL_AFTER_INIT_EVENT ; i++)
        {
          sleep(1);
        }*/
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: '%s' is set. Reverting to Time.\n", __FUNCTION__, VL_EE_FPRTTAI_ENABLED );
	 fpEnableUpdates( 1 );
    }
    else
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: '%s' is NOT set. Not reverting to Time.\n", __FUNCTION__, VL_EE_FPRTTAI_ENABLED );
    }
}

rmf_Error rmfCardManagerPostInitEventSetTime ()
{
   rmf_osal_ThreadId threadId;
   RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD","%s: \n", __FUNCTION__ );   
   rmf_Error ret = rmf_osal_threadCreate( rmfCardManagerPostInitEventSetTime_task, NULL,
            RMF_OSAL_THREAD_PRIOR_DFLT, RMF_OSAL_THREAD_STACK_SIZE,
            &threadId, "rmf_card_manager_post_Init_event_set_time_task" );	
    return ret;
}

extern int GenFetClearPersistanceStorage();


USHORT //sesnum
lookupheadEndCommSessionValue(void)
{
	if(  (headEndCommSessionState == 1) && (headEndCommSessionNumber) )
   		return headEndCommSessionNumber; 
//else
	return 0;
}


// ##################### Execution Space ##################################
// below executes in the caller's execution thread space.
// this may or may-not be in CA thread and APDU processing Space. 
// can occur before or after ca_start and caProcThread
//
//this was triggered from POD via appMgr bases on ResourceID.
//appMgr has translated the ResourceID to that of CA.
void AM_OPEN_SS_REQ_HEAD_END_COMM(ULONG RessId, UCHAR Tcid)
{
	if (headEndCommSessionState != 0)
   	{//we're already open; hence reject.
   		AM_OPEN_SS_RSP( SESS_STATUS_RES_BUSY, RessId, Tcid);
	//may want to
	//don't reject but also request that the pod close (perhaps) this orphaned session
	//AM_CLOSE_SS(caSessionNumber);
		tmpResourceId = 0;
   		tmpTcid = 0;
   		return;
   	}
	tmpResourceId = RessId;
	tmpTcid = Tcid;
	AM_OPEN_SS_RSP( SESS_STATUS_OPENED, RessId, Tcid);
	return;
}


//spec say we can't fail here, but we will test and reject anyway
//verify that this open is in response to our
//previous AM_OPEN_SS_RSP...SESS_STATUS_OPENED

void AM_SS_OPENED_HEAD_END_COMM(USHORT SessNumber, ULONG lResourceId, UCHAR Tcid)
{
	if (headEndCommSessionState != 0)
   	{//we're already open; hence reject.
   		AM_OPEN_SS_RSP( SESS_STATUS_RES_BUSY, lResourceId, Tcid);
   		tmpResourceId = 0;
   		tmpTcid = 0;
   		return;
   	}
	if (lResourceId != tmpResourceId)
   	{//wrong binding hence reject.
   		AM_OPEN_SS_RSP( SESS_STATUS_RES_BUSY, lResourceId, Tcid);
   		tmpResourceId = 0;
   		tmpTcid = 0;
   		return;
   	}
	if (Tcid != tmpTcid)
   	{//wrong binding hence reject.
   		AM_OPEN_SS_RSP( SESS_STATUS_RES_BUSY, lResourceId, Tcid);
   		tmpResourceId = 0;
   		tmpTcid = 0;
   		return;
   	}
	ResourceId      = lResourceId;
	Tcid            = Tcid;
	headEndCommSessionNumber = SessNumber;
	headEndCommSessionState  = 1;
//not sure we send this here, I believe its assumed to be successful
//AM_OPEN_SS_RSP( SESS_STATUS_OPENED, ResourceId, Tcid);
	return;
}

//after the fact, info-ing that closure has occured.
void AM_SS_CLOSED_HEAD_END_COMM(USHORT SessNumber)
{
	if((headEndCommSessionState)  && (headEndCommSessionNumber)  && (headEndCommSessionNumber != SessNumber ))
   	{
   		//don't reject but also request that the pod close (perhaps) this orphaned session
   		AM_CLOSE_SS(headEndCommSessionNumber);
   	}
	ResourceId      = 0;
	Tcid            = 0;
	tmpResourceId   = 0;
	tmpTcid         = 0;
	headEndCommSessionNumber = 0;
	headEndCommSessionState  = 0;
}
// ##################### Execution Space ##################################
// ************************* Session Stuff **************************

// Inline functions
static inline void setLoop(int cont)
{
    pthread_mutex_lock(&headEndCommLoopLock);
    if (!cont)
    {
        loop = 0;
    }
    else
    {
        loop = 1;
    }
    pthread_mutex_unlock(&headEndCommLoopLock);
}

// Sub-module entry point
int headEndComm_start(LCSM_DEVICE_HANDLE h)
{
    int res;

    if (h < 1)
    {
        MDEBUG (DPM_ERROR, "ERROR:headEndComm: Invalid LCSM device handle.\n");
        // This failure should be logged by the application manager
        return -EINVAL;
    }
    // Assign handle
    headEndCommLCSMHandle = h;
    // Flush queue
 //   lcsm_flush_queue(cardresLCSMHandle, POD_CA_QUEUE); Hannah

    //Initialize CA's loop mutex
    if (pthread_mutex_init(&headEndCommLoopLock, NULL) != 0) 
    {
        MDEBUG (DPM_ERROR, "ERROR:headEndComm_start: Unable to initialize mutex.\n");
        return EXIT_FAILURE;
    }
	
    return EXIT_SUCCESS;
}

// Sub-module exit point
int headEndComm_stop()
{
    if ( (headEndCommSessionState) && (headEndCommSessionNumber))
    {
       //don't reject but also request that the pod close (perhaps) this orphaned session
       AM_CLOSE_SS(headEndCommSessionNumber);
    }

    // Terminate message loop
    setLoop(0);

    // Wait for message processing thread to join
    if (pthread_join(headEndCommThread, NULL) != 0)
    {
        MDEBUG (DPM_ERROR, "ERROR:ca: headEndComm_stop headEndComm_stop failed.\n");
    }
    headEndCommLCSMHandle = 0;
    return EXIT_SUCCESS;//Mamidi:042209
}


void headEndComm_init(void)
{
	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," ########## headEndComm_init:: calling REGISTER_RESOURCE ############## \n");
	REGISTER_RESOURCE(M_HEAD_END_COMM, POD_HEAD_END_COMM_QUEUE, 1);  // (resourceId, queueId, maxSessions )
}
static unsigned long headEndCommTimedEvent(unsigned long TimeInMilSec)
{
		LCSM_EVENT_INFO eventSend;
   	
    	eventSend.event = POD_HEAD_END_COMM_TIMEOUT;
    	eventSend.dataPtr = NULL;
    	eventSend.dataLength = 0;
		if ( vlg_headEndCommWaitTime != 0 )
        {
			if(vlg_HETimeHandle)
			{
        		if(LCSM_TIMER_OP_OK == podResetTimer(vlg_HETimeHandle,TimeInMilSec))
					return TimeInMilSec;
			}	
			else
			{
				RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","headEndCommTimedEvent: vlg_TimeHandle in NULL Error !!! \n");
			}
        }

		vlg_HETimeHandle  = podSetTimer( POD_HEAD_END_COMM_QUEUE, &eventSend, TimeInMilSec );

		return TimeInMilSec;
}
static int headEndCommCancelTimer()
{
	podCancelTimer( vlg_HETimeHandle );
	vlg_HETimeHandle = (LCSM_TIMER_HANDLE)0;
	vlg_headEndCommWaitTime = 0;
	return 0;
}
static void postState()
{
#if GLI
  struct stat initLogFile;
  int initLogAvaialble;
  
  //*scriptName="sh /sysint/getStateDetails.sh ";
  IARM_Bus_SYSMgr_EventData_t eventData;
  
  initLogAvaialble = ((stat("/opt/mpeos_hrv_init_log.txt", &initLogFile) == 0));
  RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "postState:initLogAvaialble=%d\n",initLogAvaialble);
  
  if(initLogAvaialble)
  {
    FILE *fp = fopen("/opt/mpeos_hrv_init_log.txt","r");
    char data[25]={0};

    if (NULL != fp)
    {
       if ( fscanf(fp,"%s",data)<=0 )
       RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "fscanf failed to read from /opt/mpeos_hrv_init_log.txt");

      fclose(fp);
    }
    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "postState:initLogAvaialble OK1 strlen(data) =%d ,data =%s\n",strlen(data),data);
    
    if(strlen(data) > 0)
    {
	IARM_Bus_SYSMgr_EventData_t eventData;
	eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_DAC_INIT_TIMESTAMP;
	eventData.data.systemStates.state = 1;
	eventData.data.systemStates.error = 0;
	strncpy(eventData.data.systemStates.payload,data,strlen(data)-1);
	eventData.data.systemStates.payload[strlen(data)-1]='\0';
	IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","postState:eventData.data.systemStates.payload = %s\n",eventData.data.systemStates.payload);
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","data = %s\n",data);
    }
  }
  #endif
}
static int process_Vector(unsigned char *pInData, int DataLen,int Time,int dispflag)
{
	unsigned char *pData;
	unsigned char *pDataAll;

	unsigned char ResetEcm;
	unsigned char ResetSecurityElm;
	unsigned char ResetExternDev;
	unsigned char ClearPersistGenFetParams;   //bit 5
	unsigned char Clear_Org_Dvb_PersistFs;  // bit 4
	unsigned char ClearCachedUnbndOcapApp; //bit 3
	unsigned char ClearRegLibs;			//bit 2
	unsigned char ClearPersistHostMem;  //bit 1
	unsigned char ClearSecElmPassedValues; //bit0
	unsigned char ClearNonAsdDrvContent; //bit7
	unsigned char ClearAsdDrvContent;  //bit 6
	unsigned char ClearNetworkDrvContent;  //bit 5
	unsigned char ClearMedVolsInterHdd; // bit 4
	unsigned char ClearMedVolsExterHdd;// bit 3
	unsigned char ClearGPFSInterHdd;// bit 2
	unsigned char ClearGPFSExterHdd;//bit 1
	unsigned char ClearAllStorage; //bit 0
	unsigned char ReloadAllOcapApp;
	unsigned char ReloadOcapStack;
	unsigned char ReloadHostFirmWare;
	unsigned char ResetHost;
	unsigned char ResetAll;
	unsigned char RestartOcapStack;
	unsigned char RestartAll;
	int mode = 0;
	char aPathDir[1024] = {0};

	const char *hrvScriptPath = rmf_osal_envGet("PODMGR.HRVSCRIPT.PATH");
	if (NULL == hrvScriptPath )
	{
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", "%s: PODMGR.HRVSCRIPT.PATH not specified in config file. Using default path "
				, __FUNCTION__);
		hrvScriptPath = "/mnt/nfs/bin/scripts";
	}
	else
	{
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", "%s: PODMGR.HRVSCRIPT.PATH is %s "
				, __FUNCTION__, hrvScriptPath );
	}

	int TotalVectorSize = 8 + 8;// 8 bytes(Res+delay)  , 8 bytes( All vectores size)
	pDataAll  = pInData;

	pDataAll = pDataAll + 8;// Reserved and Delay is removed
	if(DataLen < TotalVectorSize)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","process_Vector: DataLen:%d Error < (8+ 8) \n",DataLen);
		return -1;
	}

	// Storage Clearing Field// 3 bytes  size , 5 bytes offset
	{

		pData = pDataAll + 5;
		pData += 1;// Reserved

		ClearPersistGenFetParams = ((*pData) >> 5) & 1;
		Clear_Org_Dvb_PersistFs  = ((*pData) >> 4) & 1;
		ClearCachedUnbndOcapApp  = ((*pData) >> 3) & 1;
		ClearRegLibs 		 = ((*pData) >> 2) & 1;
		ClearPersistHostMem 	 = ((*pData) >> 1) & 1;
		ClearSecElmPassedValues	 =  (*pData) & 1 ;

		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Storage Clearing Field\n{\n\t");
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Clear Persist GenFetParams:%d \n\t",ClearPersistGenFetParams);
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Clear_Org_Dvb_PersistFs:%d \n\t",Clear_Org_Dvb_PersistFs);
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Clear CachedUnbnd OcapApp:%d \n\t",ClearCachedUnbndOcapApp);
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Clear Registered Libs:%d \n\t",ClearRegLibs);
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Clear Persist HostMem:%d \n\t",ClearPersistHostMem);
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Clear Sec Elm PassedValues:%d \n",ClearSecElmPassedValues);
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","}\n");

		pData += 1;// Reserved

		ClearNonAsdDrvContent 	= ((*pData) >> 7) & 1;
		ClearAsdDrvContent 		= ((*pData) >> 6) & 1;
		ClearNetworkDrvContent 	= ((*pData) >> 5) & 1;
		ClearMedVolsInterHdd  	= ((*pData) >> 4) & 1;
		ClearMedVolsExterHdd  	= ((*pData) >> 3) & 1;
		ClearGPFSInterHdd 		= ((*pData) >> 2) & 1;
		ClearGPFSExterHdd 	 	= ((*pData) >> 1) & 1;
		ClearAllStorage	 		=  (*pData) & 1 ;

		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Storage Clearing Field\n{\n\t");
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Clear NonAsd DrvContent:%d \n\t",ClearNonAsdDrvContent);
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Clear Asd DrvContent:%d \n\t",ClearAsdDrvContent);
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Clear Network DrvContent:%d \n\t",ClearNetworkDrvContent);
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Clear MedVols InterHdd:%d \n\t",ClearMedVolsInterHdd);
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Clear MedVols ExterHdd:%d \n\t",ClearMedVolsExterHdd);
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Clear GPFS InterHdd:%d \n\t",ClearGPFSInterHdd);
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Clear GPFS ExterHdd:%d \n\t",ClearGPFSExterHdd);
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Clear All Storage:%d \n",ClearAllStorage);
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","}\n");
#ifdef  VL_CARD_HRV		
		if(ClearAllStorage)
		{
			//The Clear All Storage flag, when set, shall force the clearing of all storage resources as defined by all other flags in
			//the field. When this flag is set, all other flags in the field shall be ignored.

		}
		else
		{
			if(ClearPersistGenFetParams)
			{
				//The clear_persistant_gen_feature_params flag, when set, shall cause the Host to clear persistent stores of Generic Feature Parameters.
				//When set, this flag will cause persistent memory related to the Host area specified to be cleared, regardless of
				//location of storage. This includes, but is not limited to, physical memory and drive storage.
				GenFetClearPersistanceStorage();
				snprintf(aPathDir, sizeof(aPathDir),"%s/clear_persistant_gen_feature_params.sh",(char *)hrvScriptPath);
				system(aPathDir);

			}
			if(Clear_Org_Dvb_PersistFs)
			{
				//The org.dvb.persistent_fs flag, when set, shall cause the Host to clear persistent fields for org.dvb methods.
				snprintf(aPathDir, sizeof(aPathDir), "%s/org.dvb.persistent_fs.sh",(char *)hrvScriptPath);
				system(aPathDir);
			}
			if(ClearCachedUnbndOcapApp)
			{
				//The clear_cached_unbound_ocap_apps flag, when set, shall cause the Host to clear all unbound cached OCAP applications, regardless of current status.
				snprintf(aPathDir, sizeof(aPathDir), "%s/clear_cached_unbound_ocap_apps.sh",(char *)hrvScriptPath);
				system(aPathDir);
			}
			if(ClearRegLibs)
			{
				//The clear_registered_libraries flag, when set, shall cause the Host to clear all registered libraries, regardless of current status.
				snprintf(aPathDir, sizeof(aPathDir), "%s/clear_registered_libraries.sh",(char *)hrvScriptPath);
				system(aPathDir);
			}
			if(ClearPersistHostMem)
			{
				//The clear_persistent_host_memory flag, when set, shall cause the Host to clear all persistent memory allocations
				//made by the Host, except the information required for pairing the Host with the CableCARD. Where the OCAP
				//stack is integrated into the Host firmware, this flag will also force the clearing of any OCAP-specific memory allocations.
				snprintf(aPathDir, sizeof(aPathDir), "%s/clear_persistent_host_memory.sh",(char *)hrvScriptPath);
				system(aPathDir);
			}
			if(ClearSecElmPassedValues)
			{
				//The clear_element_passed_values, when set, shall cause the Host to clear any memory locations allocated to the CA
				//systems, including Conditional Access, Authorized Service Domain, and Digital Rights Management.
				snprintf(aPathDir, sizeof(aPathDir), "%s/clear_element_passed_values.sh",(char *)hrvScriptPath);
				system(aPathDir);
			}


			if(ClearNonAsdDrvContent)
			{
				//The clear_non_asd_dvr_content flag, when set, shall force the deletion of all content recorded on the DVR that is
				//not protected by ASD. Content deleted from the DVR when this flag is set includes network and private content.
				//Network includes any content provided by the cable operator’s network that has been stored and is not protected by
				//ASD. Private content is any content that has been uploaded by the user to the DVR unit. DVR clearing shall not
				//clear any non-content data, such as operating parameters, stored on the drive.
				snprintf(aPathDir, sizeof(aPathDir), "%s/clear_non_asd_dvr_content.sh",(char *)hrvScriptPath);
				system(aPathDir);
			}
			if(ClearAsdDrvContent)
			{
				//The clear_asd_dvr_content flag, when set, shall force the deletion of all content recorded on the DVR that is
				//protected by ASD. Content deleted from the DVR when this flag is set includes ASD-protected network content.
				//Network includes any content provided by the cable operator’s network that has been stored. DVR clearing shall
				//not clear any non-content data, such as operating parameters, stored on the drive.
				snprintf(aPathDir, sizeof(aPathDir), "%s/clear_asd_dvr_content.sh",(char *)hrvScriptPath);
				system(aPathDir);
			}
			if(ClearNetworkDrvContent)
			{
				//The clear_network_dvr_content flag, when set, shall force the deletion of all content recorded on the DVR that has
				//been recorded from a network source. Network includes any content provided by the cable operator’s network that
				//has been stored. DVR clearing shall not clear any non-content data, such as operating parameters, stored on the
				//drive.
				snprintf(aPathDir, sizeof(aPathDir), "%s/clear_network_dvr_content.sh",(char *)hrvScriptPath);
				system(aPathDir);
			}
			if(ClearMedVolsInterHdd)
			{
				//The clear_media_volumes_int_hdd flag, when set, shall force all media volumes on internal drives to be cleared of
				//all data. Data cleared shall include all data, without regard to source or purpose.
				snprintf(aPathDir, sizeof(aPathDir), "%s/clear_media_volumes_int_hdd.sh",(char *)hrvScriptPath);
				system(aPathDir);
			}
			if(ClearMedVolsExterHdd)
			{
				//The clear_media_volumes_ext_hdd flag, when set, shall force all media volumes on external drives to be cleared of
				//all data. Data cleared shall include all data, without regard to source or purpose.
				snprintf(aPathDir, sizeof(aPathDir), "%s/clear_media_volumes_ext_hdd.sh",(char *)hrvScriptPath);
				system(aPathDir);
			}
			if(ClearGPFSInterHdd)
			{
				//The clear_GPFS_internal_hdd flag, when set, shall force GPFSs on internal drives to be cleared of all data. Data
				//cleared shall include all data, without regard to source or purpose.
				snprintf(aPathDir, sizeof(aPathDir), "%s/clear_GPFS_internal_hdd.sh",(char *)hrvScriptPath);
				system(aPathDir);
			}
			if(ClearGPFSExterHdd)
			{
				//The clear_GPFS_external_hdd flag, when set, shall force GPFSs on external drives to be cleared of all data. Data
				//cleared shall include all data, without regard to source or purpose.
				snprintf(aPathDir, sizeof(aPathDir), "%s/clear_GPFS_external_hdd.sh",(char *)hrvScriptPath);
				system(aPathDir);
			}
		}
#endif
	}
	/////////////
	// Reload Application filed // 1byte size 3 bytes offset
	{
		pData = pDataAll + 3;

		ReloadAllOcapApp = ((*pData) >> 1) & 1;
		ReloadOcapStack		 =  (*pData) & 1 ;

		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Restart field\n{\n\t");
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Reload All Ocap Apps:%d \n\t",ReloadAllOcapApp);
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Reload Ocap Stack:%d \n",ReloadOcapStack);
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","}\n");
#ifdef  VL_CARD_HRV
		if(ReloadAllOcapApp)
		{
			//The reload_all_OCAP_apps flag, when set, shall cause the Host to delete all OCAP applications, allowing them to
			//reload via normal Open Cable methods.
			snprintf(aPathDir, sizeof(aPathDir), "%s/reload_all_OCAP_apps.sh",(char *)hrvScriptPath);
			system(aPathDir);
		}
		if(ReloadOcapStack)
		{
			//The reload_of_OCAP_stack flag, when set, shall cause the Host to delete all OCAP applications and the stack,
			//where the stack is separate from the firmware. Upon completion of the deletion, the stack and applications will
			//reload via normal OpenCable methods.
			snprintf(aPathDir, sizeof(aPathDir), "%s/reload_of_OCAP_stack.sh",(char *)hrvScriptPath);
			system(aPathDir);
		}
#endif		
	}
	///////////////////
	// Reload Firmware field// 1byte size  4 bytes offset
	{
		pData = pDataAll + 4;
		ReloadHostFirmWare	 =  (*pData) & 1 ;

		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Reload Frimware field\n{\n\t");
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Reload Host FirmWare:%d \n",ReloadHostFirmWare);
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","}\n");
#ifdef  VL_CARD_HRV
		if(ReloadHostFirmWare)
		{
			//The Reload Host Firmware flag, when set, shall force the Host to reload its firmware. If a reload of firmware entails
			//a deletion of the Host firmware, it may force the Host back to a “factory” state. A factory state can either be a
			//ROM’d version of code or a validated default version of code. If only one version of the Host firmware is
			//maintained, then the Host shall delete the old firmware only if it has already received and verified the availability
			//and viability of the new replacement firmware. If a Reset Host or Reset All flag is set in the Reset Field, the Host
			//shall be capable of processing the reload, while continuing through execution of the Reset Vector.
			snprintf(aPathDir, sizeof(aPathDir), "%s/reload_host_firmware.sh",(char *)hrvScriptPath);
			system(aPathDir);
		}
#endif
	}

	//Reset  field // 2bytes size , 0 bytes offset.
	{
		pData = pDataAll + 0;
		pData += 1;// 1byte reserved

		ResetEcm 		= ((*pData) >> 7) & 1;
		ResetSecurityElm 	= ((*pData) >> 6) & 1;
		ResetHost 		= ((*pData) >> 5) & 1;
		ResetExternDev 		= ((*pData) >> 4) & 1;
		ResetAll 		= ( *pData & 1 );

		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Reset field \n{\n\t");
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Reset Embedded Cable Modem:%d \n\t", ResetEcm);
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Reset Security Element      :%d\n\t", ResetSecurityElm);
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Reset Host                :%d \n\t", ResetHost);
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Reset External Devices    :%d \n\t", ResetExternDev);
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Reset All                 :%d \n", ResetAll );
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","}\n");
		pData += 1;// 1 byte

#ifdef  VL_CARD_HRV		
		if(ResetAll || ResetHost)
		{
			VL_HOST_SNMP_API_RESULT ret ;
			if(ResetAll)
			{
				//Need to send reset to external devices before resetting the host 
				//The method for resetting an external device is not currently defined, but this flag is being defined as a placeholder.

			}
			//#ifdef ENABLE_SNMP

			// Reboot host is responsible for rebooting eSTB and eCM and Security Element
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","process_Vector:Calling vlSnmpRebootHost(VL_SNMP_HOST_REBOOT_INFO_cablecardError)...\n");
			ret = vlSnmpRebootHost(VL_SNMP_HOST_REBOOT_INFO_cablecardError);

			if(ret != VL_HOST_SNMP_API_RESULT_SUCCESS )
			{
				RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","process_Vector:vlSnmpRebootHost retured Error :%d \n", ret );
			}
			//#endif // ENABLE_SNMP
			return 0;
		}
#endif
	}
	//Restart Field // 1 byte size, 2 bytes offset.
	{
		pData = pDataAll + 2;

		RestartOcapStack = ((*pData) >> 1) & 1;
		RestartAll	 =  (*pData) & 1 ;
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Restart field\n{\n\t");
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Restart Ocap Stack:%d \n\t",RestartOcapStack);
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Restart All:%d \n",RestartAll);
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","}\n");
#ifdef  VL_CARD_HRV
		if(RestartOcapStack)
		{
			//The restart_OCAP_stack flag, when set, will force all the OCAP applications running on the stack to be restarted.
			snprintf(aPathDir, sizeof(aPathDir), "%s/restart_OCAP_stack.sh",(char *)hrvScriptPath);
			system(aPathDir);

		}
		if(RestartAll)
		{
			//The restart_all flag, when set, shall cause all software sub-systems as defined in the Restart Field OCAP and all
			//applications implemented on the sub-system to be restarted.
			snprintf(aPathDir, sizeof(aPathDir), "%s/restart_all.sh",(char *)hrvScriptPath);
			system(aPathDir);

		}
		/////////////// Reset pending flags.../////////////////
		if(ResetEcm)
		{
			// Reset the embeded cable modem
			snprintf(aPathDir, sizeof(aPathDir), "%s/reset_ecm.sh",(char *)hrvScriptPath);
			system(aPathDir);
		}
		if(ResetSecurityElm)
		{
			// Reset cable card
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," Issue the Reset to cable card \n");

			HAL_POD_RESET_PCMCIA(0,0);
		}
		if(ResetExternDev)
		{
			// Reset the external devices
			snprintf(aPathDir, sizeof(aPathDir), "%s/reset_ext_devices.sh",(char *)hrvScriptPath);
			system(aPathDir);
		}
#endif		
	}

#ifdef VL_HRV_ENABLE
	if(ResetAll || ResetHost)
	{
		vlg_bHrvInitWithResetInProgress = 1;
	}

	int bNotRefreshEvent = 1;

	bNotRefreshEvent= ResetEcm |
		ResetSecurityElm |
		ResetHost |
		ResetExternDev |
		ResetAll |
		RestartOcapStack |
		RestartAll |
		ReloadAllOcapApp |
		ReloadOcapStack |
		ReloadHostFirmWare |
		ClearPersistGenFetParams |
		Clear_Org_Dvb_PersistFs |
		ClearCachedUnbndOcapApp |
		ClearRegLibs |
		ClearPersistHostMem |
		ClearSecElmPassedValues |
		ClearNonAsdDrvContent |
		ClearAsdDrvContent |
		ClearNetworkDrvContent |
		ClearMedVolsInterHdd |
		ClearMedVolsExterHdd |
		ClearGPFSInterHdd |
		ClearGPFSExterHdd |
		ClearAllStorage;

	if (FALSE == bNotRefreshEvent)
	{
		if(dispflag == 1)
		{
			// Refresh Event
			rmf_osal_LogEvent(RMF_OSAL_INIT_LOG, REFRESH_INIT_LOG_ENTRY, "Completed");
			// MOT7425-2493: Disabling FP update. // vl_mpeos_fpSetTextSimple(VL_REFRESH_TEXT);
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Refresh Event Received.\n");
		}
		else
		{
			// Not Refresh Eventa
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Refresh Event Timeout.\n" );
		}
	}
	else
	{
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Not a Refresh Event.\n" );
	}
	//SA (Cisco) HRV support - NV Memory reset
	//Warehouse hubs are using the Non-volatile memory reset option through the DNCS api's to clean returned boxes for SA. 
	//This instructs the host to clean all persistent storage. From an X1 perspective this amounts to a warehouse reset. 
	//Please note the reload firmware and reset host attributes are set to false - this is is what differentiates it 
	//from an INIT or Cold INIT on the Moto
	if (ClearAllStorage && !ReloadHostFirmWare && !ResetHost)
	{
		if (ClearNonAsdDrvContent && ClearAsdDrvContent
			&& ClearNetworkDrvContent && ClearMedVolsInterHdd
			&& ClearMedVolsExterHdd && ClearGPFSInterHdd && ClearGPFSExterHdd)
		{
			uint16_t manufacturerId = 0;
			int32_t manufacturer = CARD_UNKNOWN;

			rmf_Error rc = podmgrGetManufacturerId(&manufacturerId);
			if (RMF_SUCCESS == rc)
			{
				manufacturer = (manufacturerId & 0xFF00) >> 8;
			}

			if (dispflag == 1 && manufacturer == CARD_CISCO)
			{
				rmf_osal_LogEvent(RMF_OSAL_INIT_LOG, COLD_INIT_LOG_ENTRY, "Completed");
				fpSetTextSimple(VL_COLD_INIT_TEXT);
				fpEnableUpdates(0);

				RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," Invoking fpdisp 2 \n");
				gHeadCommInProgress = 1;
				system("/./HrvInitScripts/clear_persistent_host_memory.sh");

				struct stat st;
				bool ret = false;

				if ((0 == stat(VL_EE_FPRTTAI_ENABLED, &st) && (0 != st.st_ino)))
				{
					RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: '%s' is set. Not calling fdisp\n",
							__FUNCTION__, VL_EE_FPRTTAI_ENABLED);
					ret = true;
				}
				if (!ret)
				{
					system("/./fpdisp 2 &");
				}
				rmfCardManagerPostInitEventSetTime();
			}
			else if (dispflag == 0 && manufacturer == CARD_CISCO)
			{
				RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Delete SI Cache 2 %d 0 &\n",Time);
				system("/./HrvInitScripts/clear_persistent_host_memory.sh");
				RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","CISCO NVRAM reset triggered"
						"Invoking /lib/rdk/warehouse-reset.sh\n");
				system("/lib/rdk/warehouse-reset.sh");
			}
		}
		else if (dispflag == 1)
		{
			rmf_osal_LogEvent(RMF_OSAL_INIT_LOG, WAREHOUSE_INIT_LOG_ENTRY, "Completed");
			fpSetTextSimple(VL_WAREHOUSE_INIT_TEXT);

			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," Delete SI Cache 1 %d 0 &\n",Time);
			system("/./HrvInitScripts/clear_persistent_host_memory.sh");
		}
		else {
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","rm -rf /opt/data \n ");
			system("rm -rf /opt/data");

			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","rm -rf /opt/persistent/dvr \n ");
			system("rm -rf /opt/persistent/dvr");

			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","rm -rf /tmp/mnt/diska3/persistent/dvr\n ");
			system("rm -rf /tmp/mnt/diska3/persistent/dvr");

			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","rm /opt/www/whitebox/wbdevice.dat"
				" /opt/www/authService/deviceid.dat /opt/logger.conf /opt/hosts"
				" /opt/receiver.conf /opt/proxy.conf /opt/mpeenv.ini"
				" /tmp/mnt/diska3/persistent/hostData1\n");
			system("rm /opt/www/whitebox/wbdevice.dat /opt/www/authService/deviceid.dat /opt/logger.conf /opt/hosts /opt/receiver.conf /opt/proxy.conf /opt/mpeenv.ini /tmp/mnt/diska3/persistent/hostData1");

			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","echo 0 > /opt/.rebootFlag\n ");
			system("echo 0 > /opt/.rebootFlag");

			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," Delete SI Cache 2 %d 0 &\n",Time);
			system("/./HrvInitScripts/clear_persistent_host_memory.sh");
			struct stat st;
			bool ret=false;

			if ((0 == stat(VL_EE_FPRTTAI_ENABLED, &st) && (0 != st.st_ino)))
			{
				RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: '%s' is set.Not calling /./hrvinit %d 0 &\n", __FUNCTION__, VL_EE_FPRTTAI_ENABLED,Time );
				ret=true;
			}
			if (!ret)
			{
				RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Invoking hrvinit 30 2 &\n");
#ifdef HEADED_GW
				system("/./hrvinit 30 2 0 &");
#elif HEADLESS_GW
#ifdef USE_CISCO_MFR
				system("source /HrvInitScripts/hrvCombinedColdInit.sh &");
#else
				system("/./hrvinit 30 2 1 &");
#endif		
#elif CLIENT_TSB
				system("/./hrvinit 30 2 2 &");
#elif CLIENT
				system("/./hrvinit 30 2 3 &");
#elif NODVR_GW
				system("/./hrvinit 30 2 4 &");
#endif
				//system("/./hrvinit Time 0 &");
			}
			// reset FP time after 60 sec.
			rmfCardManagerPostInitEventSetTime();
		}

	}

	if (ClearPersistGenFetParams && Clear_Org_Dvb_PersistFs && ClearCachedUnbndOcapApp && ClearRegLibs)
	{
		if (ClearPersistHostMem&& ClearSecElmPassedValues && ResetHost)
		{
			char TempBuff[64];

			//mnt/nfs/env
			if(dispflag == 1)
			{
				rmf_osal_LogEvent(RMF_OSAL_INIT_LOG, DAC_INIT_LOG_ENTRY, "Completed");
				fpSetTextSimple(VL_INIT_TEXT);
				fpEnableUpdates(0);
				postState();
				RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," Invoking fpdisp 1 \n");
				gHeadCommInProgress = 1;
				fpSetTextSimple(VL_INIT_TEXT);
				fpEnableUpdates(0);
				RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," Delete SI Cache 1 %d 0 &\n",Time);
				system("/./HrvInitScripts/clear_persistent_host_memory.sh");
				struct stat st;
				bool ret=false;

				if((0 == stat(VL_EE_FPRTTAI_ENABLED, &st) && (0 != st.st_ino)))
				{
					RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: '%s' is set. not calling fdisp.\n", __FUNCTION__, VL_EE_FPRTTAI_ENABLED );
					ret=true;
				}
				if(!ret)
				{
					system("/./fpdisp 1 &");
				}
				rmfCardManagerPostInitEventSetTime( );

			}
			else
			{
#ifdef GLI
				//postState();
				//IARM_Bus_SYSMgr_EventData_t eventData;
				//eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_MOTO_HRV_RX;
				//eventData.data.systemStates.state = 1;
				//exec -b 0xc1400000 -l 0x400000 -c "console=ttyS0,115200 rw memmap=exactmap  memmap=128K@128K memmap=575M@1M vmalloc=586M rootwait panic=5 nmi_watchdog=1  reboot=acpi  root=/dev/sdb2"
				//IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
#endif
				RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," Delete SI Cache 2 %d 0 &\n",Time);
				system("/./HrvInitScripts/clear_persistent_host_memory.sh");
				struct stat st;
				bool ret=false;
				if((0 == stat(VL_EE_FPRTTAI_ENABLED, &st) && (0 != st.st_ino)))
				{
					RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: '%s' is set.Not calling /./hrvinit %d 0 &\n", __FUNCTION__, VL_EE_FPRTTAI_ENABLED,Time );
					ret=true;
				}
				if(!ret)
				{
					RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," Invoking hrvinit %d 0 &\n",Time);
#ifdef HEADED_GW
					sprintf(TempBuff,"/./hrvinit %d 0 0 &",Time);
#elif HEADLESS_GW
#ifdef USE_CISCO_MFR
					snprintf(TempBuff, sizeof(TempBuff), "source /HrvInitScripts/hrvCombinedColdInit.sh &");
#else
					snprintf(TempBuff, sizeof(TempBuff),"/./hrvinit %d 0 1 &",Time);
#endif		
#elif CLIENT_TSB
					snprintf(TempBuff, sizeof(TempBuff),"/./hrvinit %d 0 2 &",Time);
#elif CLIENT
					snprintf(TempBuff, sizeof(TempBuff),"/./hrvinit %d 0 3 &",Time);
#elif NODVR_GW
					snprintf(TempBuff, sizeof(TempBuff),"/./hrvinit %d 0 4 &",Time);
#endif
					//system("/./hrvinit Time 0 &");
					system(TempBuff);
				}
				rmfCardManagerPostInitEventSetTime();
			}
		}
	}
	if (ClearAllStorage && ReloadHostFirmWare && ReloadAllOcapApp )
	{
		char TempBuff[64];
		if (ResetAll && RestartAll)
		{
			if (dispflag == 1)
			{
				rmf_osal_LogEvent(RMF_OSAL_INIT_LOG, COLD_INIT_LOG_ENTRY, "Completed");
				fpSetTextSimple(VL_COLD_INIT_TEXT);
				fpEnableUpdates(0);
				RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," Invoking fpdisp 2 \n");
				gHeadCommInProgress = 1;
				system("/./HrvInitScripts/clear_persistent_host_memory.sh");
				struct stat st;
				bool ret=false;

				if((0 == stat(VL_EE_FPRTTAI_ENABLED, &st) && (0 != st.st_ino)))
				{
					RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: '%s' is set.Not calling fdisp.\n", __FUNCTION__, VL_EE_FPRTTAI_ENABLED );
					ret=true;
				}
				if(!ret)
				{
					system("/./fpdisp 2 &");
				}
				rmfCardManagerPostInitEventSetTime( );
			}
			else
			{
				RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: Capability to wipe code banks disabled!\n", __FUNCTION__);
				RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," Delete SI Cache 2 %d 0 &\n",Time);
				system("/./HrvInitScripts/clear_persistent_host_memory.sh");
				RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Invoking /lib/rdk/warehouse-reset.sh\n ");
				system("/lib/rdk/warehouse-reset.sh");
				struct stat st;
				bool ret=false;

				if((0 == stat(VL_EE_FPRTTAI_ENABLED, &st) && (0 != st.st_ino)))
				{
					RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: '%s' is set.Not calling /./hrvinit %d 0 &\n", __FUNCTION__, VL_EE_FPRTTAI_ENABLED,Time );
					ret=true;
				}
				if(!ret)
				{
					RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," Invoking hrvinit %d 0 &\n",Time);
					//sprintf(TempBuff,"/./hrvinit %d 0 &",Time);
#ifdef HEADED_GW
					snprintf(TempBuff, sizeof(TempBuff),"/./hrvinit %d 0 0 &",Time);
#elif HEADLESS_GW
#ifdef USE_CISCO_MFR
					snprintf(TempBuff, sizeof(TempBuff),"source /HrvInitScripts/hrvCombinedColdInit.sh &");
#else
					snprintf(TempBuff, sizeof(TempBuff),"/./hrvinit %d 0 1 &",Time);
#endif		
#elif CLIENT_TSB
					snprintf(TempBuff, sizeof(TempBuff),"/./hrvinit %d 0 2 &",Time);
#elif CLIENT
					snprintf(TempBuff, sizeof(TempBuff),"/./hrvinit %d 0 3 &",Time);
#elif NODVR_GW
					snprintf(TempBuff, sizeof(TempBuff),"/./hrvinit %d 0 4 &",Time);
#endif
					//system("/./hrvinit Time 0 &");
					system(TempBuff);
				}
				// reset FP time after 60 sec
				rmfCardManagerPostInitEventSetTime();
			}
		}
		else if (ResetAll)
		{
			if (dispflag == 1)
			{
				rmf_osal_LogEvent(RMF_OSAL_INIT_LOG, COLD_INIT_LOG_ENTRY, "Completed");
				fpSetTextSimple(VL_COLD_INIT_TEXT);
				fpEnableUpdates(0);

				RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," Invoking fpdisp 2 \n");
				gHeadCommInProgress = 1;
				system("/./HrvInitScripts/clear_persistent_host_memory.sh");
				struct stat st;
				bool ret=false;

				if((0 == stat(VL_EE_FPRTTAI_ENABLED, &st) && (0 != st.st_ino)))
				{
					RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: '%s' is set.Not calling fdisp\n", __FUNCTION__, VL_EE_FPRTTAI_ENABLED );
					ret=true;
				}
				if(!ret)
				{
					system("/./fpdisp 2 &");
				}
				rmfCardManagerPostInitEventSetTime( );
			}
			else
			{
				RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: Capability to wipe code banks disabled!\n", __FUNCTION__);
				RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," Delete SI Cache 2 %d 0 &\n",Time);
				system("/./HrvInitScripts/clear_persistent_host_memory.sh");
				RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Invoking /lib/rdk/warehouse-reset.sh\n ");
				system("/lib/rdk/warehouse-reset.sh");
				struct stat st;
				bool ret=false;

				if((0 == stat(VL_EE_FPRTTAI_ENABLED, &st) && (0 != st.st_ino)))
				{
					RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: '%s' is set.Not calling /./hrvinit %d 0 &\n", __FUNCTION__, VL_EE_FPRTTAI_ENABLED,Time );
					ret=true;
				}
				if(!ret)
				{
					RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," Invoking hrvinit %d 0 &\n",Time);
					//sprintf(TempBuff,"/./hrvinit %d 0 &",Time);
					//system("/./hrvinit Time 0 &");
#ifdef HEADLESS_GW
#ifdef USE_CISCO_MFR
					system( "source /HrvInitScripts/hrvCombinedColdInit.sh &");
#else
					system("/./HrvInitScripts/xg5_init.sh");
					RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Calling xg5_init.sh\n");
#endif		
#endif

#ifdef HEADED_GW
					snprintf(TempBuff, sizeof(TempBuff),"/./hrvinit %d 0 0 &",Time);
#elif HEADLESS_GW
#ifdef USE_CISCO_MFR
					snprintf(TempBuff, sizeof(TempBuff), "source /HrvInitScripts/hrvCombinedColdInit.sh &");
#else
					snprintf(TempBuff, sizeof(TempBuff),"/./hrvinit %d 0 1 &",Time);
#endif		
#elif CLIENT_TSB
					snprintf(TempBuff, sizeof(TempBuff),"/./hrvinit %d 0 2 &",Time);
#elif CLIENT
					snprintf(TempBuff, sizeof(TempBuff),"/./hrvinit %d 0 3 &",Time);
#elif NODVR_GW
					snprintf(TempBuff, sizeof(TempBuff),"/./hrvinit %d 0 4 &",Time);
#endif
					system(TempBuff);
				}
				// reset FP time after 60 sec
				rmfCardManagerPostInitEventSetTime();
			}

		}
	}
#endif

	return 0;
}
int process_head_end_comm_Request(uint16 sessNum, uint8 *apdu, uint16 apduLen)
{
   int ii;
   unsigned long Tag;
	unsigned char Len;
	unsigned char TranId;
	unsigned char Delay;
	unsigned char *pData;
	int waitDelay = 10;// 10 secs
	
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","process_head_end_comm_Request : ENTERED @@@@@@@@@@@ \n");
    
	if(apdu == NULL || apduLen == 0)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","process_head_end_comm_Request: apdu == NULL || apduLen == 0 \n");
		return -1;
	}
	pData = apdu;
    Tag =  ( (apdu[0] << 16 ) | ( apdu[1] << 8 ) | apdu[2] );
	pData += 3;//Tag 3 bytes
	if(Tag != 0x9F9E00)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","process_head_end_comm_Request: Error in TAG:0x%X received \n",Tag);
		return -1;
	}
	if((apdu[3] & 0x80 ))
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","process_head_end_comm_Request: Len field is too big Expecting only one byte \n");
		return -1;
	}
	Len = *pData;//  1 byte length
	pData += 1;//1 byte length
	
	if( Len < 9 || Len > 17)// Expecting at least 9 bytes not more than 17 bytes
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","process_head_end_comm_Request:  Dat len is not correct Len:%d < 9 or > 17\n",Len);
		return -1;
	}
	TranId = *pData;// 1 byte Transaction Id.
	vlg_TranId = TranId;

	pData += 1;//1 byte length
// Vector is copied  to process 
	memcpy(Vector_data,pData, (Len -1));
	vlg_Vector_size = Len -1;

	pData += 56/8; // 7 bytes reserved

	Delay = *pData; // 1 byte
	pData += 1;
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","process_head_end_comm_Request: Len:%d TranId:%d Delay:%d \n",Len,TranId,Delay);
	
	//process_Vector(Vector_data,vlg_Vector_size);
	Delay = 180;// secs
//#ifdef XONE_STB
//Changed the delay to 90 secs from 180 secs
      //Delay = 90;// secs
// Now the delay is made configurable
        unsigned int nDelay = vl_env_get_int(180, "DELAY_RESET_VECTOR_TIMING_IN_SECS");
	if(nDelay == 0) nDelay = 180;
	Delay = nDelay;
//#endif
	
      RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","process_head_end_comm_Request: Received time delay:%d secs \n",Delay);
      {
	      unsigned short alen;
	      unsigned long tag = 0x9F9E01;
	      unsigned char papdu[MAX_APDU_HEADER_BYTES + 2];

	      memset(papdu, 0,sizeof(papdu));
	      
	      if (buildApdu(papdu, &alen, tag, 1, &TranId ) == NULL)
	      {
		MDEBUG (DPM_ERROR, "ERROR:process_head_end_comm_vector_ack: Unable to build apdu.\n");
		return APDU_HAEDEND_COMM_FAILURE;
	      }
	      RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","process_head_end_comm_Request: Sending Resposnse for HE commn Req\n");
	      AM_APDU_FROM_HOST(sessNum, papdu, alen);
      }
      //fpdisp  display the message here.
      if(gHeadCommInProgress == 0)
      {
	  process_Vector(Vector_data,vlg_Vector_size,Delay,1);
      }
		
#if 0
#if 0
	process_head_end_comm_vector_ack(sessNum,TranId,(int)Delay);
#else
	if(Delay == 0)
	{
		process_head_end_comm_vector_ack(sessNum,TranId,(int)Delay);
		
	}
	else
	{

      RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," >>>>>>>>>>>>>>>> waiting for the Delay:%d sec <<<<<<<<<<<<<<<<<<<<< \n",Delay);

	 // printf(" >>>>>>>>>>>>>>>> After waiting for the Delay:%d sec <<<<<<<<<<<<<<<<<<<<< \n",Delay);
	  //printf(" calling process_head_end_comm_vector_ack \n");
	  //process_head_end_comm_vector_ack(sessNum,TranId,(int)Delay);


	//	Delay = Delay - waitDelay;
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," >>>>>>>>>> calling headEndCommTimedEvent with time:%d secs delay \n",Delay);
		vlg_headEndCommWaitTime = headEndCommTimedEvent(1000*Delay);
		
		
	}
#endif
#else
      RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," >>>>>>>>>> calling headEndCommTimedEvent with time:%d secs delay \n",Delay);
      vlg_headEndCommWaitTime = headEndCommTimedEvent(1000*Delay);  
#endif
	return 0;
  
}
static int process_head_end_comm_vector_ack(uint16 sessNum, uint8 TranId,int Time)
{
#if 0
	unsigned short alen;
    unsigned long tag = 0x9F9E01;
	unsigned char papdu[MAX_APDU_HEADER_BYTES + 2];

//    memset(papdu, 0,sizeof(papdu));
    vlMemSet(papdu, 0,sizeof(papdu),sizeof(papdu));
    if (buildApdu(papdu, &alen, tag, 1, &TranId ) == NULL)
    {
       MDEBUG (DPM_ERROR, "ERROR:process_head_end_comm_vector_ack: Unable to build apdu.\n");
       return APDU_HAEDEND_COMM_FAILURE;
    }
	
    AM_APDU_FROM_HOST(sessNum, papdu, alen);
#endif
    process_Vector(Vector_data,vlg_Vector_size,Time,0);

    return APDU_HAEDEND_COMM_SUCCESS;
  
}

void headEndCommProc(void* par)
{
	unsigned char *ptr;
	LCSM_EVENT_INFO       *eventInfo = (LCSM_EVENT_INFO       *)par;

	setLoop(1);

//	handle = headEndCommLCSMHandle;		// because REGISTER_RESOURCE macro expects 'handle'

	switch (eventInfo->event)
	{

        // POD requests open session (x=resId, y=tcId)
	case POD_OPEN_SESSION_REQ:
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","headEndCommProc: << POD_OPEN_SESSION_REQ >>\n");
		AM_OPEN_SS_REQ_HEAD_END_COMM((uint32)eventInfo->x, (uint8)eventInfo->y);
        	break;

		// POD confirms open session (x=sessNbr, y=resId, z=tcId)
	case POD_SESSION_OPENED:
		
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","headEndCommProc: << POD_SESSION_OPENED >>\n");
		AM_SS_OPENED_HEAD_END_COMM((uint16)eventInfo->x, (uint32)eventInfo->y, (uint8)eventInfo->z);
	//	vlg_headEndCommWaitTime = 0;
	   	break;

		// POD closes session (x=sessNbr)
	case POD_CLOSE_SESSION:
    {
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","headEndCommProc: << POD_CLOSE_SESSION >>\n");
		AM_SS_CLOSED_HEAD_END_COMM((uint16)eventInfo->x);
		//headEndCommCancelTimer();
      
    }
    break;

	case POD_APDU:
    {
        unsigned long apduTag = 0;
		ptr = (unsigned char *)eventInfo->y;	// y points to the complete APDU
        
        vlMpeosDumpBuffer(RDK_LOG_TRACE1, "LOG.RDK.POD", ptr, eventInfo->z);
        
        apduTag =  ( (ptr[0] << 16 ) | ( ptr[1] << 8 ) | ptr[2] );
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","headEndCommProc: APDU TAG << 0x%08x >>, 0x%02x%02x%02x\n", apduTag, ptr[0], ptr[1], ptr[2]);
		switch (apduTag)
		{
		case HOST_RESET_VEC_REQ_TAG:
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Received HOSTADRPRT_TAG_REQ \n");
			process_head_end_comm_Request((uint16)eventInfo->x, (uint8 *)eventInfo->y, (uint16)eventInfo->z);
			break;

		case HOST_RESET_VEC_REPLY_TAG:
		/*	if(vlg_headEndCommWaitTime == 0)
				break;
			vlg_headEndCommWaitTime = 0;
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Received HOST_RESET_VEC_REPLY_TAG \n");
			process_head_end_comm_vector_ack(lookupheadEndCommSessionValue(), vlg_TranId);*/
			break;

		
		default:
			break;
		}
		break;
    }
	case POD_HEAD_END_COMM_TIMEOUT:
    {
		if(vlg_headEndCommWaitTime == 0)
				break;
			vlg_headEndCommWaitTime = 0;
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","Received POD_HEAD_END_COMM_TIMEOUT \n");
			process_head_end_comm_vector_ack(lookupheadEndCommSessionValue(), vlg_TranId,0);
			break;


		case EXIT_REQUEST:
		//	setLoop(0);
			break;

		default:
			break;
		}
    }
   // }
	//pthread_exit(NULL);
	setLoop(0);
}


#ifdef __cplusplus
}
#endif


