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

#include "rmf_error.h"
#include "rmf_osal_init.h"
#include "rmf_osal_event.h"
#include "rdk_debug.h"

#include "rmf_qamsrc_common.h"
#include "rmf_inbsimgr.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include "ismd_core.h"
#include "ismd_demux.h"
#include "ismd_global_defs.h"
#include "sec.h"
#include "pace_tuner_api.h"
#include "dsg_rpc_h2cm_api.h"
#include "dhs_db.h"

#ifdef __cplusplus
}
#endif

#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif

#define NUMBER_OF_INB_INSTANCES 3
#define NUMBER_OF_CHANNELS 6
#define PMT_TIME_OUT 3000
#define SI_EVENT_WAIT_TIMEOUT_US  500000 /* .5 seconds*/

typedef struct
{
	unsigned int tunerID;
	unsigned int freq;
	PaceTunerModulation qamType;
	unsigned int program_no;
}tuner_info;

typedef struct
{
	unsigned char	m_demuxInstance;
	ismd_dev_handle_t   m_demux_handle;
	ismd_dev_state_t    m_demux_state;//demux current state
	ismd_clock_t m_hClock;
	rmf_InbSiMgr * pInbandSI;
	rmf_osal_eventqueue_handle_t siEventQHandle;
	sem_t* siDoneSem;
	tuner_info* pTunerInfo;
}demux_session_info;

static demux_session_info ga_demuxSession[NUMBER_OF_CHANNELS];

static const int gaPlatformTunerId[NUMBER_OF_CHANNELS] = {2, 0, 1, 3, 4, 5};

static tuner_info ga_tunerInfo[NUMBER_OF_CHANNELS] = { {2, 279000, PT_MOD_QAM64, 1},
									{0, 279000, PT_MOD_QAM64, 2},
									{1, 231000, PT_MOD_QAM256, 1},
									{3, 231000, PT_MOD_QAM256, 2},
									{4, 279000, PT_MOD_QAM64, 3},
									{5, 231000, PT_MOD_QAM256, 3}};

/*Funtion to initialize Demux module and generate Demux handle for all the available tuners */
int test_demuxInitialize(  )
{
	ismd_dev_handle_t	demux_handle;
	ismd_port_handle_t	demux_inport_handle;
	ismd_clock_t		hClock;

	int loop;

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.TEST",   "Enter %s \n", __FUNCTION__);

	/* Create only one clock for use of all LIVE decode from different tuners.
	  FIX for AV sync on LIVE decode (for PUSH mode Intel suggestion is to use the SW_CONTROLLED clock) */

	//PL_LOG(MPE_MOD_MEDIA, ismd_clock_alloc(ISMD_CLOCK_TYPE_SW_CONTROLLED, &hClock),"ismd_clock_alloc: Allocating clock ..");
	if( ISMD_SUCCESS != ismd_clock_alloc(ISMD_CLOCK_TYPE_SW_CONTROLLED, &hClock) )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.TEST",  "\n Failed to allocate clock\n");
	}

	int tsinID = 0;
	//[VL]: Transport Stream ismd initialization
	for( loop = 0; loop < NUMBER_OF_CHANNELS; loop++)
	{
		tsinID = loop;
		if(loop >=4)
		{
			tsinID += 2;
		}
		//PL_LOG(MPE_MOD_MEDIA, ismd_tsin_open(tsinID, &demux_handle),"ismd_tsin_open:TSIN %d\n",loop);
		if( ISMD_SUCCESS != ismd_tsin_open(tsinID, &demux_handle) )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.TEST",  "\n Failed to create demux handle for Demux Instance : %d\n", loop);
			return -1;
		}
		//		PL_LOG(MPE_MOD_MEDIA, ismd_demux_tsin_set_buffer_size(demux_handle, 1024*64),"ismd_demux_tsin_set_buffer_size (1024*64):TSIN %d\n", i);

		if( 0 )
		{
			//PL_LOG(MPE_MOD_MEDIA, ismd_demux_enable_leaky_filters(demux_handle), "ismd_demux_enable_leaky_filters");
			ismd_demux_enable_leaky_filters(demux_handle);
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.TEST",  " LEAKY FILTERS ENABLED\n");
		}
		else
		{
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.TEST",  " LEAKY FILTERS DISABLED\n");
		}
		
		//PL_LOG(MPE_MOD_MEDIA, ismd_dev_set_state(demux_handle,ISMD_DEV_STATE_STOP),"ismd_dev_set_state:hdemuxtsin=%x,STOP",demux_handle);
		/* set Demux handle state to stop */
		ismd_dev_set_state(demux_handle,ISMD_DEV_STATE_STOP);

		/*Initialize Demux global structure */
		ga_demuxSession[loop].m_demuxInstance	= loop;
		ga_demuxSession[loop].m_demux_handle		= demux_handle;
		ga_demuxSession[loop].m_hClock			= hClock;		
		ga_demuxSession[loop].m_demux_state		= ISMD_DEV_STATE_STOP;	
		ga_demuxSession[loop].siEventQHandle= 0;
		ga_demuxSession[loop].pInbandSI = NULL;
		ga_demuxSession[loop].pTunerInfo = &ga_tunerInfo[loop];
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.TEST",  "  loop value = %d, demux handle = %x \n", loop, demux_handle);
	}	

	/*
	*Open and start an output filter for a stream. Used ONLY to open a PES filter or a PSI filter for the specified stream. 
	*/
	//PL_LOG(MPE_MOD_MEDIA, ismd_demux_stream_open(ISMD_DEMUX_STREAM_TYPE_MPEG2_TRANSPORT_STREAM, &demux_handle,										&demux_inport_handle) ,"ismd_demux_stream_open:type=%d");
	if( ISMD_SUCCESS != ismd_demux_stream_open(ISMD_DEMUX_STREAM_TYPE_MPEG2_TRANSPORT_STREAM, &demux_handle, &demux_inport_handle) )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.TEST",  "\n ERROR ERROR .	Failed to open demux handle. \n" );
		return -1;
	}
 
	//PL_LOG(MPE_MOD_MEDIA, ismd_dev_set_state(demux_handle,ISMD_DEV_STATE_STOP),"ismd_dev_set_state:hdemuxstream=%x,STOP",demux_handle);
	ismd_dev_set_state(demux_handle, ISMD_DEV_STATE_STOP);

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.TEST",   "Exit r %s \n", __FUNCTION__);

	return 0;
}/* End of gst_demuxInitialize */


void processProgramInfo(rmf_InbSiMgr * pInbandSI , int program_no)
{
	rmf_Error ret;
	rmf_MediaInfo newMediaInfo;

	/*This can be removed once memset is corrected in inband si manager.*/
	memset(&newMediaInfo, 0, sizeof (rmf_MediaInfo));

	ret = pInbandSI->GetMediaInfo(program_no, &newMediaInfo);
	if( ret == RMF_INBSI_SUCCESS)
	{
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.TEST", "Media pids available in this Program\n");
	}
	else if( ret == RMF_INBSI_PMT_NOT_AVAILABLE)
	{
		RDK_LOG( RDK_LOG_WARN, "LOG.RDK.TEST", "PMT not available yet\n");
	}
	else
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.TEST", "InbandSI->GetMediaInfo failed\n");
		exit(0);
	}
	if( 0 == newMediaInfo.numMediaPids)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.TEST", "No Media pids available in this Program\n"); 
	}
	else
	{
		printf("\n==============Pid list============\n"
			"PMT Pid = %d PCR Pid =%d\n Media Pids: ",newMediaInfo.pmt_pid, newMediaInfo.pcrPid);
		for (int i= 0; i < newMediaInfo.numMediaPids; i++)
		{
			printf("( Pid type:  %d, Pid: %d) ",newMediaInfo.pPidList[i].pidType,newMediaInfo.pPidList[i].pid);
		}
		printf("\n");
	}
}

static void si_event_monitor_thread(void * arg)
{
	int session_id = *(int*)arg;
	rmf_osal_event_handle_t event_handle;
	uint32_t event_type;
	rmf_Error ret;
	uint8_t NotTerminated = TRUE;
	rmf_osal_event_params_t	eventData;	 
	//rmf_InbSiCache  *SICache = rmf_InbSiCache::getInbSiCache();	
	rmf_osal_TimeMillis currentTime;
	rmf_osal_TimeMillis initialTime;
	rmf_osal_Bool pmtEventSent = FALSE;
	rmf_osal_Bool patEventSent = FALSE;
	rmf_osal_eventqueue_handle_t siEventQHandle;
	rmf_InbSiMgr * pInbandSI ;

	demux_session_info* pSession = &ga_demuxSession[session_id];
	pInbandSI = pSession->pInbandSI ;
	siEventQHandle = pSession->siEventQHandle;

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.TEST", "session_id[%d] :Enter %s():%d \n", session_id , __FUNCTION__, __LINE__);
	/*	will be released while exiting; to sync with term()  */

	ret = rmf_osal_timeGetMillis( &initialTime );
	if(RMF_SUCCESS != ret )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.TEST", "session_id[%d] :rmf_osal_timeGetMillis Failed ret= %d\n", session_id, ret);
		exit(0);
	}
	while(NotTerminated) 
	{
		if ( TRUE == pmtEventSent )
		{
			ret = rmf_osal_eventqueue_get_next_event(siEventQHandle, 
					&event_handle, NULL, &event_type, &eventData );
		}
		else
		{
			ret = rmf_osal_eventqueue_get_next_event_timed(siEventQHandle, 
					&event_handle, NULL, &event_type, &eventData, SI_EVENT_WAIT_TIMEOUT_US);
			if (RMF_OSAL_ENODATA == ret )
			{
				if ( 0 == initialTime)
				{
					continue;
				}
				ret = rmf_osal_timeGetMillis( &currentTime );
				if(RMF_SUCCESS != ret )
				{
					RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.TEST", "session_id[%d] :rmf_osal_timeGetMillis Failed ret= %d\n", session_id, ret);
					continue;
				}
				RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.TEST", "session_id[%d] :%s:%d currentTime = %llu \n", session_id, __FUNCTION__, __LINE__, currentTime );
				if ( PMT_TIME_OUT < (currentTime - initialTime) )
				{
					if ( FALSE == patEventSent )
					{
						RDK_LOG( RDK_LOG_WARN, "LOG.RDK.TEST", "session_id[%d] :PAT not received after timeout\n", session_id, ret);
					}
					else
					{
						RDK_LOG( RDK_LOG_WARN, "LOG.RDK.TEST", "session_id[%d] :PMT not received after timeout\n", session_id, ret);
					}
					patEventSent = TRUE;
					pmtEventSent = TRUE;
					exit(0);
				}
				continue;
			}
		}

		if (RMF_SUCCESS != ret)
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.TEST", "session_id[%d] : rmf_osal_eventqueue_get_next_event failed\n", session_id );
			break;
		}

		if ( 0xFF == event_type )
		{
			rmf_osal_event_delete(event_handle);
			NotTerminated = FALSE;
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.TEST", "session_id[%d] :%s Stopping event reception\n", session_id, __FUNCTION__);
			continue;
		}

		switch(event_type)
		{
			/* SI module events */
			case RMF_SI_EVENT_IB_PAT_ACQUIRED:
				
				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.TEST", "session_id[%d] :Recvd PAT event\n", session_id);
				//notifyError(  GSTQAMTUNERSRC_EVENT_PAT_ACQUIRED);
				patEventSent = TRUE;

				ret = pInbandSI->RequestProgramInfo(pSession->pTunerInfo->program_no);
				if(RMF_SUCCESS != ret )
				{
					RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.TEST", "session_id[%d] :Error in RequestProgramInfo \n", session_id);
					exit(0);
				}
				RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.TEST", "session_id[%d] :%s:%d Requested ProgramInfo \n", session_id, __FUNCTION__, __LINE__);
				break;

			case RMF_SI_EVENT_IB_PMT_ACQUIRED:
				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.TEST", "session_id[%d] :PMT ACQUIRED event\n", session_id);
				pmtEventSent = TRUE;
				processProgramInfo(pInbandSI, pSession->pTunerInfo->program_no);
				//SEND_STATUS_EVENT( context, GSTQAMTUNERSRC_EVENT_PMT_ACQUIRED);
				break;

			case RMF_SI_EVENT_IB_PAT_UPDATE:
				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.TEST", "session_id[%d] :PAT update event\n", session_id);

				ret = pInbandSI->ClearProgramInfo(pSession->pTunerInfo->program_no);
				if(RMF_SUCCESS != ret )
				{
					RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.TEST", "session_id[%d] :Error in ClearProgramInfo \n", session_id);
				}
				RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.TEST", "session_id[%d] :%s:%d Cleared ProgramInfo \n", session_id, __FUNCTION__, __LINE__);

				ret = pInbandSI->RequestProgramInfo(pSession->pTunerInfo->program_no);
				if(RMF_SUCCESS != ret )
				{
					RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.TEST", "session_id[%d] :Error in RequestProgramInfo \n", session_id);
					exit(0);
				}
				RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.TEST", "session_id[%d] :%s:%d Requested ProgramInfo \n", session_id, __FUNCTION__, __LINE__);
				break;

			case RMF_SI_EVENT_IB_PMT_UPDATE:
				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.TEST", "session_id[%d] :PMT update event\n", session_id);
				processProgramInfo(pInbandSI, pSession->pTunerInfo->program_no);
				break;

			case RMF_SI_EVENT_SI_NOT_AVAILABLE_YET:
				RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.TEST", "session_id[%d] :RMF_SI_EVENT_SI_NOT_AVAILABLE_YET\n", session_id);
				break;

			default:
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.TEST", "session_id[%d] :Unknown event %d \n", session_id,event_type);
				break;
		}

		ret = rmf_osal_event_delete(event_handle);
		if (RMF_SUCCESS != ret )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.TEST", "session_id[%d] :%s - rmf_osal_event_delete failed.\n", session_id, __FUNCTION__);
		}
	}
	
	/* signal thread is exited*/
	ret = sem_post( pSession->siDoneSem);
	if (0 != ret )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.TEST", "session_id[%d] :%s - sem_post failed.\n", session_id, __FUNCTION__);
	}
	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.TEST", "session_id[%d] :Exit %s():%d \n", session_id , __FUNCTION__, __LINE__);
}/* End of gst_qamtunersrc_simonitorthread */



static void si_test_thread(void * arg)
{
	int session_id = *(int*)arg;
	PaceTunerReturnCode retValue;
	rmf_Error ret;
	rmf_osal_ThreadId siMonitorThreadId;
	rmf_InbSiMgr * pInbandSI;
	rmf_PsiSourceInfo sourceInfo;
	rmf_ModulationMode modulationMode;
	rmf_osal_eventqueue_handle_t siEventQHandle;
	rmf_osal_event_handle_t event_handle;
	rmf_osal_event_params_t p_event_params = {	1, /* Priority */
							NULL,
							0,
							NULL
						};
	sem_t * siDoneSem;

	tuner_info* p_tunerInfo = ga_demuxSession[session_id].pTunerInfo;

	retValue = Pace_ConfigureTuner(p_tunerInfo->tunerID, p_tunerInfo->freq, p_tunerInfo->qamType);
	if(PT_OK == retValue)
	{
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.TEST",  "session_id[%d] :Tune started. Pace_ConfigureTuner success\n", session_id );
	}
	else
	{
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.TEST",  "session_id[%d] : Pace_ConfigureTuner Failed. Error Code = %d. Retrying ......\n", session_id, retValue);
	}

	siDoneSem =  new sem_t;
	if (NULL == siDoneSem)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.TEST", "session_id[%d] :Failed to allocate sem in %s\n", session_id, __FUNCTION__ );
		exit(0);
	}

	ret  = sem_init( siDoneSem, 0 , 0);
	if (0 != ret )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.TEST", "session_id[%d] :%s - sem_init failed.\n", session_id, __FUNCTION__);
		exit(0);
	}
	ga_demuxSession[session_id].siDoneSem = siDoneSem;

	sourceInfo.dmxHdl = ga_demuxSession[session_id].m_demux_handle;
	sourceInfo.tunerId= session_id;
	sourceInfo.freq = p_tunerInfo->freq;
	modulationMode = ( p_tunerInfo->qamType == PT_MOD_QAM256)? RMF_MODULATION_QAM256:RMF_MODULATION_QAM64;
	sourceInfo.modulation_mode = modulationMode;

	do 
	{

		pInbandSI = new rmf_InbSiMgr(sourceInfo); 
		if (NULL ==  pInbandSI)
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.TEST", "session_id[%d] :Cannot allocte InbandSI", session_id );
			exit(0);
		}
		ga_demuxSession[session_id].pInbandSI = pInbandSI;

		ret = rmf_osal_threadCreate( si_event_monitor_thread, arg, 
			RMF_OSAL_THREAD_PRIOR_DFLT, RMF_OSAL_THREAD_STACK_SIZE, 
			&siMonitorThreadId, NULL);
		if (RMF_SUCCESS != ret )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.TEST", "session_id[%d] :%s - rmf_osal_threadCreate failed.\n", session_id, __FUNCTION__);
			exit(0);
		}
		pInbandSI->TuneInitiated( p_tunerInfo->freq, modulationMode);


		ret = rmf_osal_eventqueue_create( (const uint8_t*) "SIEvtQ", &siEventQHandle);
		if (RMF_SUCCESS != ret )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.TEST", "session_id[%d] : %s - rmf_osal_eventqueue_create failed.\n", session_id, __FUNCTION__);
			exit(0);
		}

		ga_demuxSession[session_id].siEventQHandle = siEventQHandle;

		/* Registering for SI events */
		ret = pInbandSI->RegisterForPSIEvents(siEventQHandle, p_tunerInfo->program_no);
		if(RMF_SUCCESS != ret )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.TEST", "session_id[%d] :RegisterForPSIEvents Failed ret= %d\n", session_id, ret);
			exit(0);
		}
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.TEST", "session_id[%d] :%s:%d Register For PSIEvents \n", session_id, __FUNCTION__, __LINE__);

		ret = pInbandSI->RequestTsInfo();
		if(RMF_SUCCESS != ret )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.TEST", "session_id[%d] :Could not request for TS Info ret= %d\n", session_id, ret);
			exit(0);
		}
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.TEST", "session_id[%d] :%s:%d Requested TsInfo \n", session_id, __FUNCTION__, __LINE__);

		sleep(10);
		rmf_osal_eventqueue_clear(siEventQHandle);

		if (NULL != pInbandSI)
		{
			RDK_LOG(RDK_LOG_INFO,"LOG.RDK.TEST", "session_id[%d] : RMFQAMSrcImpl::%s:%d Clear Program info \n" , session_id, __FUNCTION__, __LINE__);
			pInbandSI->ClearProgramInfo(p_tunerInfo->program_no);
			pInbandSI->UnRegisterForPSIEvents(siEventQHandle, p_tunerInfo->program_no);
			delete pInbandSI;
			pInbandSI = NULL;
			RDK_LOG(RDK_LOG_INFO,"LOG.RDK.TEST", "session_id[%d] : RMFQAMSrcImpl::%s:%d Deleted INB SI \n" , session_id, __FUNCTION__, __LINE__);
		}

		/* to exit the while loop of MonitorThread */
		ret = rmf_osal_event_create(RMF_OSAL_EVENT_CATEGORY_INB_FILTER, 0xFF, &p_event_params, &event_handle);
		if(ret!= RMF_SUCCESS)
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.TEST", "session_id[%d] :Could not create event handle:\n", session_id);
		}

		ret =  rmf_osal_eventqueue_add(siEventQHandle, event_handle);
		if(ret!= RMF_SUCCESS)
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.TEST", "session_id[%d] :rmf_osal_eventqueue_add failed\n", session_id);
		}

		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.TEST", "session_id[%d] :%s - Sent terminate event to SI event queue %d, waiting for signal \n", session_id, __FUNCTION__, siEventQHandle);
		ret = sem_wait( siDoneSem);
		if (0 != ret )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.TEST", "session_id[%d] :%s - sem_wait failed.\n", session_id, __FUNCTION__);
		}
		ga_demuxSession[session_id].siEventQHandle  = NULL;
	}while (1);
	ret  = sem_destroy( siDoneSem);
	if (0 != ret )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.TEST", "session_id[%d] :%s - sem_destroy failed.\n", session_id, __FUNCTION__);
	}
	delete siDoneSem;
	
}

int main()
{
	rmf_Error ret;
	char option;
	rmf_osal_ThreadId threadId;
	static int indices[NUMBER_OF_CHANNELS];

	rmf_osal_init( NULL, NULL );
	test_demuxInitialize();

	for (int i=0; i< NUMBER_OF_CHANNELS; i++)
		indices[i] = i;

	for(int i =0; i< NUMBER_OF_INB_INSTANCES; i++)
	{
		printf("Creating thread\n");
		ret = rmf_osal_threadCreate( si_test_thread, &indices[i],
				RMF_OSAL_THREAD_PRIOR_DFLT, RMF_OSAL_THREAD_STACK_SIZE, &threadId,
		        "Thread_0");
		if (RMF_SUCCESS != ret )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.TEST", "%s - rmf_osal_threadCreate failed.\n", __FUNCTION__);
			return -1;
		}
	}
	
	while (1)
	{
		printf("%s:%d : x for Exit\n",  __FUNCTION__, __LINE__);
		option = getchar();
		if ('x' == option )
			break;
	}
	return 0;
}

