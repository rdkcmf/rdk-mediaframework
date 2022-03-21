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

#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif

//#include <rmf_sectionfilter_inb.h>

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

rmf_Tuner* tuner = NULL;
rmf_InbSiMgr *inbsimgr = NULL;
rmf_osal_Bool bStarted = FALSE;

int TuneToSource(uint32_t prog_num);
int StopTuning(uint32_t prog_num);

int main()
{
	uint8_t name[100];
	uint32_t prog_nums[] = {1, 2, 3, 5, 6};
	rmf_Error ret;
	int i=0;
	int choice=0;
	int prev_choice=0;

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

	while(1)
	{
		for(i=0;i<5;i++)
		{
			printf("%d. Tune to Program: 0x%x\n", (i+1), prog_nums[i]);
		}
		printf("0. Exit\n");
		scanf("%d", &choice);

		if(bStarted)
		{
			StopTuning(prog_nums[prev_choice-1]);			
			bStarted = FALSE;
		}

		if(choice == 0)
		{
			return 0;
		}
		else if (choice < 0 || choice > i )
                {
                        printf("Invalid option\n");
                        continue;
                }



		printf("Choice is %d, requested to tune to program number: %d", choice, prog_nums[choice-1]);
		TuneToSource(prog_nums[choice-1]);
		prev_choice = choice;
	}

}

int StopTuning(uint32_t prog_num)
{
	printf("%s(%d): Reached here\n", __FUNCTION__, __LINE__);
	inbsimgr->UnRegisterForPSIEvents(inbsi_eventqueue_handle, prog_num);
	printf("%s(%d): Reached here\n", __FUNCTION__, __LINE__);
	tuner->UnregisterQueueForEvents(tuner_eventqueue_handle);
	printf("%s(%d): Reached here\n", __FUNCTION__, __LINE__);
	delete inbsimgr;
	printf("%s(%d): Reached here\n", __FUNCTION__, __LINE__);
	delete tuner;
	printf("%s(%d): Reached here\n", __FUNCTION__, __LINE__);
}


int TuneToSource(uint32_t prog_num)
{
	rmf_Tuner_Error tuneRetCode;
	rmf_Error ret;
	rmf_TuneParams tuneReq;
	uint32_t tunerID = 0;
	ismd_dev_handle_t DmuxHdl;	
	rmf_PsiSourceInfo psiSource;

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

	/* Filling Tune request */
	tuneReq.frequency = 435000000;
	tuneReq.programNumber = prog_num;
	tuneReq.qamMode = RMF_TUNE_MODULATION_QAM256;
	tuneReq.tuneType = RMF_TUNER_TUNE_BY_TUNING_PARAMS;

	ret = inbsimgr->TuneInitiated(tuneReq.frequency, tuneReq.qamMode);
	RMF_OSAL_ASSERT_ISM(ret, "Failed to notify Inband SI Manager about frequency change ");
	printf("\nCalled TunerInitiated");


	tuneRetCode = tuner->requestForTune(&tuneReq);
	RMF_OSAL_ASSERT_ISM(tuneRetCode, "Failed to tune to the requested params\n");
	
	ret = inbsimgr->RegisterForPSIEvents(inbsi_eventqueue_handle, tuneReq.programNumber);
	RMF_OSAL_ASSERT_ISM(ret, "Failed to register for PSI Events ");
	printf("\nRegistered queue with inbsimgr for PSI events");

	ret = inbsimgr->RequestTsInfo();
	RMF_OSAL_ASSERT_ISM(ret, "RequestProgramInfo failed ");

	ret = inbsimgr->RequestProgramInfo(tuneReq.programNumber);
	RMF_OSAL_ASSERT_ISM(ret, "RequestProgramInfo failed ");
	
	printf("\nCalled RequestProgramInfo");
	bStarted = TRUE;
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
					&event_handle, &event_type,  NULL);
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
						&event_handle, &event_type,  &eventData);
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

