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
#include <rmf_osal_event.h>
#include <rmf_osal_init.h>
#include <rmf_tuner.h>
#include <rmf_inbsimgr.h>
//#include <rmf_sectionfilter_inb.h>
//#include <rmf_oobsicache.h>
#include <rmf_oobsimgr.h>

#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif

static rmf_osal_ThreadId threadId;
static rmf_osal_ThreadId inbsithreadId;
static void tuner_thread(void * arg);
static void inbsi_thread(void * arg);

#define RMF_OSAL_ASSERT_ISM(err,msg) {\
	if (RMF_SUCCESS != err) {\
		printf("%s:%d : %s \n",__FUNCTION__,__LINE__,msg); \
		abort(); \
	}\
}

rmf_osal_eventqueue_handle_t tuner_eventqueue_handle;
rmf_osal_eventqueue_handle_t inbsi_eventqueue_handle;
static rmf_osal_eventmanager_handle_t eventmanager_handle =  0;

#if 0
static const char  *g_siOOBCachePost114Location = NULL;
static const char  *g_siOOBCacheLocation = NULL;
static const char  *g_siOOBSNSCacheLocation = NULL;

int cache_init()
{
        rmf_SiCache *pSiCache = rmf_SiCache::getSiCache();
        rmf_Error ret = RMF_SUCCESS;
        uint32_t num_entries;
        uint32_t size=1024;
        char szStatus[1024];
        char szStatusTemp[1024];
	char c;

        g_siOOBCacheLocation = "/rmf_work/si_table_cache.bin";
        g_siOOBCachePost114Location = "/rmf_work/si/si_table_cache.bin";
        g_siOOBSNSCacheLocation = "/rmf_work/si/sns_table_cache.bin";

        ret = pSiCache->load_si_entries(g_siOOBCacheLocation, g_siOOBCachePost114Location, g_siOOBSNSCacheLocation);
        if (RMF_SUCCESS != ret )
        {
                printf("load_si_entries failed\n");
        }
        else
        {
                printf("load_si_entries success\n");
                ret = pSiCache->getSiEntryCount((void*)&num_entries);
                if(RMF_SUCCESS==ret)
                {
                        printf("No:of entries: %d\n", num_entries);
                        for(int i=0;i<num_entries;i++)
                        {
                                size=1024;
                                ret = pSiCache->getSiEntry(i, &size, &szStatus, szStatusTemp);
                                if(RMF_SUCCESS==ret)
                                        printf("%s", szStatus);
                                else
                                        printf("Failed to get si entry\n");
				if(((i+1)%10==0) && ((i+1)<num_entries))
				{
					printf("Press Enter to continue, x to exit...\n");
					scanf("%c",&c);
					if(c=='x')
						break;
				}
                       }
                }
        }
}
#endif

rmf_Tuner* tuner = NULL;
rmf_InbSiMgr *inbsimgr = NULL;
rmf_OobSiMgr *pSiMgr = NULL;

int TuneToSource(uint32_t sourceID);

int main()
{
	uint8_t name[100];
	uint32_t sourceIDs[] = {17603, 9066, 16211, 63994, 18235};
	rmf_Error ret;
	uint32_t tunerID = 0;
	ismd_dev_handle_t DmuxHdl;	
	rmf_PsiSourceInfo psiSource;
	int i=0;
	int choice=0;

	//	cache_init();
	printf("\nWelcome to test app...");
	rmf_osal_init( NULL, NULL );

	printf("\nAfter RMF_OSAL Init...");

	snprintf( (char*)name, 100, "TUNERRCVQ");
	ret = rmf_osal_eventqueue_create( name, &tuner_eventqueue_handle);
	RMF_OSAL_ASSERT_ISM(ret, "rmf_osal_eventQueue_create failed");

	ret = rmf_osal_threadCreate( tuner_thread,NULL,
					RMF_OSAL_THREAD_PRIOR_DFLT, 2048, &threadId,
			        "Thread_1");
	

	ret = rmf_osal_eventmanager_register_handler(
			eventmanager_handle,
			tuner_eventqueue_handle,
			RMF_OSAL_EVENT_CATEGORY_TUNER);
	RMF_OSAL_ASSERT_ISM(ret, "rmf_osal_eventmanager_register_handler failed");

	snprintf( (char*)name, 100, "INBSIRCVQ");

	ret = rmf_osal_eventqueue_create( name, &inbsi_eventqueue_handle);
	RMF_OSAL_ASSERT_ISM(ret, "rmf_osal_eventQueue_create failed");
	
	ret = rmf_osal_threadCreate( inbsi_thread,NULL,
					RMF_OSAL_THREAD_PRIOR_DFLT, 2048, &inbsithreadId,
			        "Thread_2");

	tuner = new rmf_Tuner();
	if( NULL == tuner )
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.TEST", "Failed to allocate memory for tuner object\n" );
	}

	printf("\nAfter Tuner Object Creation...");

	printf("\nQueue & thread created and queue registered to tuner");
	ret = tuner->RegisterQueueForEvents(tuner_eventqueue_handle);
	RMF_OSAL_ASSERT_ISM(ret, "Failed to registerQueue for Tune events ");

        ret = tuner->getTunerID( &tunerID );
	RMF_OSAL_ASSERT_ISM(ret, "Failed to get tunerid ");
        
	ret = tuner->getDemuxHandle( &DmuxHdl );  //to get demux handle for the tunerid
	RMF_OSAL_ASSERT_ISM(ret, "Failed to get demux handle ");

	psiSource.dmxHdl = DmuxHdl; /* Demux Handle to be used for Section Filtering */
    	psiSource.tunerId = tunerID;
    	psiSource.freq = 0; /**< Tuner frequency, in Hz (used to validate tuner state) */ 
    	psiSource.modulation_mode = 0; /** Modulation mode */

	inbsimgr = new rmf_InbSiMgr(psiSource);
	printf("\nCreated Inband SI Manager object");

	pSiMgr = rmf_OobSiMgr::getInstance();

	while(1)
	{
		for(i=0;i<5;i++)
		{
			printf("%d. Tune to Source URL: 0x%x\n", (i+1), sourceIDs[i]);
		}
		printf("0. Exit\n");
		scanf("%d", &choice);
		if(choice == 0)
		{
			return 0;
		}
		else if (choice < 0 || choice > i )
                {
                        printf("Invalid option\n");
                        continue;
                }

		//sourceID = 17603;

		printf("Choice is %d, requested to tune to sourceId: %d/0x%x", choice, sourceIDs[choice-1], sourceIDs[choice-1]);
		TuneToSource(sourceIDs[choice-1]);
	}

}

int TuneToSource(uint32_t sourceID)
{
	Oob_ProgramInfo_t program_info;
	rmf_Tuner_Error tuneRetCode;
	rmf_Error ret;
	rmf_TuneParams tuneReq;
	
	pSiMgr->GetProgramInfo(sourceID, &program_info);
	printf("sourceID: %d/0x%x, freq: %d, mode: %d, prog_no: %d\n", sourceID, sourceID, program_info.carrier_frequency, program_info.modulation_mode, program_info.prog_num);

	/* Filling Tune request */
	tuneReq.frequency = program_info.carrier_frequency;
	tuneReq.programNumber = program_info.prog_num;
	tuneReq.qamMode = (rmf_TuneModulationMode)program_info.modulation_mode;
	tuneReq.tuneType = RMF_TUNER_TUNE_BY_TUNING_PARAMS;

	ret = inbsimgr->TuneInitiated(tuneReq.frequency, tuneReq.qamMode);
	RMF_OSAL_ASSERT_ISM(ret, "Failed to notify Inband SI Manager about frequency change ");
	printf("\nCalled TunerInitiated");


	tuneRetCode = tuner->requestForTune(&tuneReq);
	RMF_OSAL_ASSERT_ISM(tuneRetCode, "Failed to tune to the requested params\n");
	
	ret = inbsimgr->RegisterForPSIEvents(inbsi_eventqueue_handle, tuneReq.programNumber);
	RMF_OSAL_ASSERT_ISM(ret, "Failed to register for PSI Events ");
	printf("\nRegistered queue with inbsimgr for PSI events");

	//ret = inbsimgr->RequestTsInfo();
	ret = inbsimgr->RequestProgramInfo(tuneReq.programNumber);
	RMF_OSAL_ASSERT_ISM(ret, "RequestProgramInfo failed ");
	
	printf("\nCalled RequestProgramInfo");
}

static void tuner_thread(void * arg)
{
	rmf_Error ret;
	uint32_t event_type;
	rmf_osal_event_handle_t event_handle;
	uint8_t NotTerminated = TRUE;
	
	while(NotTerminated)
	{
		ret = rmf_osal_eventqueue_get_next_event(  tuner_eventqueue_handle,
					&event_handle, NULL, &event_type,  NULL);
			RMF_OSAL_ASSERT_ISM(ret, "rmf_osal_eventqueue_get_next_event failed");

		switch(event_type)
		{
			case RMF_TUNER_EVENT_TUNE_STARTED:
	       	       RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "Tune started\n");
				break;
			
			case RMF_TUNER_EVENT_TUNE_FAIL:
		              RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "Tune Failed\n");
				break;
			
			case RMF_TUNER_EVENT_TUNE_ABORT:
		              RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "Tuning aborted\n");
				break;
			
			case RMF_TUNER_EVENT_TUNE_SYNC	:
	              	RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "Tuned\n");
				break;
			
			case RMF_TUNER_EVENT_TUNE_UNSYNC:
	       	       RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "Tuner unlocked\n");
				break;
			
			case RMF_TUNE_EVENT_SHUTDOWN:
		              RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "Tuner shutted down\n");
				NotTerminated = FALSE;
			       break;

			case 0xff:
                     	RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "Stopping tuner event reception\n");
									 
				break;	
			
			default:
		              RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.QAMSRC", "Unknown event from Tuner\n");
				 break;	

		}

		 ret = rmf_osal_event_delete(event_handle);
		 RMF_OSAL_ASSERT_ISM(ret, "rmf_osal_event_delete failed");
       }
}

static void inbsi_thread(void * arg)
{
 	(void)arg;
	rmf_osal_event_handle_t event_handle;
	uint32_t event_type;
	rmf_Error osal_error;	
	uint8_t NotTerminated = TRUE;
	rmf_osal_event_params_t	eventData;   
	
       printf("\nStarting SI event reception\n" );

       while(NotTerminated) 
       {
       	printf("\nWaiting for Event\n" );
	       osal_error = rmf_osal_eventqueue_get_next_event(inbsi_eventqueue_handle,
						&event_handle, NULL, &event_type,  &eventData);
		switch(event_type)
		{
			case RMF_SI_EVENT_IB_PAT_ACQUIRED:
       			printf("\nRxd PAT\n" );
	       	       	RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "Recvd PAT event\n");
			break;
			
			case RMF_SI_EVENT_IB_PMT_ACQUIRED:
       			printf("\nRxd PMT\n" );
		        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "Recvd PMT event\n");
			break;
			
			case RMF_SI_EVENT_IB_CVCT_ACQUIRED:
		        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "Rcvd CVCT event\n");
			break;
			
			case RMF_SI_EVENT_SERVICE_COMPONENT_UPDATE:
	              	RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "Service com change event\n");
			break;
			
			case RMF_SI_EVENT_IB_PAT_UPDATE:
       			printf("\nRxd PAT update\n" );
	       	        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "PAT update event\n");
			break;
			
			case RMF_SI_EVENT_IB_PMT_UPDATE:
       				printf("\nRxd PMT update\n" );
		              RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "PMT update event\n");
			break;

			case RMF_SI_EVENT_IB_CVCT_UPDATE:
	              	RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "CVCT update event\n");
			break;
			
			case RMF_SI_EVENT_SI_ACQUIRING:
	       	        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "Acquiring SI event\n");
			break;
			
			case RMF_SI_EVENT_SI_NOT_AVAILABLE_YET:
       				printf("\nData not yet available\n" );
		              RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "SI not yet available\n");
			break;


			case RMF_SI_EVENT_SI_FULLY_ACQUIRED:
		        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "Fully acquired SI\n");
			break;


			case RMF_SI_EVENT_SI_DISABLED:
		        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "SI disabled\n");
			break;


			case RMF_SI_EVENT_TUNED_AWAY:
		        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "Tuned away\n");
			break;

			case RMF_PSI_EVENT_SHUTDOWN:
		        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "SI shutted down\n");
			break;

			
			//case RMF_SI_EVENT_TERMINATE:
                     	//RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "Stopping SI event reception\n");
			//break;	
			
			default:
		        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.QAMSRC", "Unknown event from SI\n");
			NotTerminated = FALSE;
			break;	

		}
	       osal_error = rmf_osal_event_delete(event_handle);
       }

}

