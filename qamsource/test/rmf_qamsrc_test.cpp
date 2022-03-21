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
#include <iostream>
#include <string.h>
#if 0
#include <gst/gst.h>
#endif

#include "rmf_oobsimanagerstub.h"
#include "rmfqamsrc.h"
#include "rmfbase.h"
#include "rmf_osal_types.h"
#include "rmf_error.h"
#include "rmfqamsrc.h"
#include "mediaplayersink.h"
#include "testsink.h"
#include "rdk_debug.h"
#include "rmf_platform.h"

#define TEST_QAMSRC_DIAG
#ifdef TEST_QAMSRC_DIAG
#include "rmf_osal_thread.h"
#include "QAMSrcDiag.h"
#endif

#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif

#include "rmf_error.h"

#define SRC_INVALID 0
#define SRC_QAMSRC 1
#define SINK_INVALID 0
#define SINK_MPSINK 1
#define SINK_HN 2

#ifndef RMF_TEST_SRC_NO
#define RMF_TEST_SRC_NO 1
#endif


//#define USE_QAMSRC_CHANGEURI

#ifndef USE_QAMSRC_CHANGEURI
#define TEST_MPSINK
#endif

class QamSrcEventHandler : public IRMFMediaEvents
{
public:
	QamSrcEventHandler(RMFQAMSrc* src) : qamsrc(src)
	{
	}
	void error(RMFResult err, const char* msg);
private:
	RMFQAMSrc* qamsrc;
};

struct connection
{
	RMFMediaSourceBase *pSource;
	RMFMediaSinkBase *pSink;
	uint32_t src_type;
	uint32_t sink_type;
	rmf_osal_eventqueue_handle_t  eventqueue;
	QamSrcEventHandler* pQamSrcEventHandler;
};

void QamSrcEventHandler::error(RMFResult err, const char* msg)
{
	RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.TEST", "%s:%d: Error 0x%x\n",__FUNCTION__,__LINE__, err);
}

/* ocap://0x<SourceId> */
rmf_Error open ( struct connection * con, uint32_t sourceId, uint32_t source_type, uint32_t sink_type)
{
	rmf_Error ret;
	char achSourceId[100];
	RMFQAMSrc * qamsrc = NULL;
	RMFResult rmfres;

	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TEST", "Enter %s:%d con = 0x%x\n",__FUNCTION__,__LINE__,con);
	snprintf(achSourceId, 99, "ocap://0x%x",sourceId);
	achSourceId[99] = '\0';

#ifndef QAMSRC_FACTORY
	//con->pSource = RMFQAMSrc::RMFQAMSrcObj( sourceId, FALSE);
	qamsrc = new RMFQAMSrc();

	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.TEST", " %s:%d: Src Created\n",__FUNCTION__,__LINE__);
	
	qamsrc->init ();
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.TEST", " %s:%d: Src Init Complete.. Opening for src id %s\n",__FUNCTION__,__LINE__, achSourceId);

	qamsrc->open ( ( char * )achSourceId,  NULL );
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.TEST", " %s:%d: Src Open Complete\n",__FUNCTION__,__LINE__);

	float speed = 1234;
	qamsrc->getSpeed( speed);
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TEST", " %s:%d: Src speed = %f\n",__FUNCTION__,__LINE__, speed);
#else
#ifndef USE_QAMSRC_CHANGEURI
	qamsrc = RMFQAMSrc::getQAMSourceInstance( ( char * )achSourceId );
#else
	if ( NULL == con->pSource )
	{
		qamsrc = RMFQAMSrc::getQAMSourceInstance( ( char * )achSourceId );
	}
	else
	{
		bool newInstance = TRUE ;
		RMFQAMSrc * old = (RMFQAMSrc *)(con->pSource );
		rmfres = RMFQAMSrc::changeURI( ( char * )achSourceId, old, &qamsrc, newInstance);
		if (RMF_RESULT_SUCCESS != rmfres )
		{
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.TEST", " %s:%d : changeURI failed\n",__FUNCTION__,__LINE__);
			exit(0);
		}
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.TEST", " %s:%d: Changed URI old = 0x%x, new =0x%x, newInstance = %d\n",
			__FUNCTION__,__LINE__, old, qamsrc, newInstance);
	}
#endif
#endif
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TEST", "%s:%d after create - con = 0x%x\n",__FUNCTION__,__LINE__,con);
#if 0
	{
		uint32_t tsId;
		rmfres = qamsrc->getTSID(tsId);
		if (RMF_RESULT_SUCCESS != rmfres )
		{
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.TEST", " %s:%d : getTSID failed\n",__FUNCTION__,__LINE__);
			exit(0);
		}
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TEST", "%s:%d getTSID - tsId = 0x%x\n",__FUNCTION__,__LINE__,tsId);
	}
#endif
	con->pSource = qamsrc;

	con->pQamSrcEventHandler = new QamSrcEventHandler(qamsrc);
	qamsrc->addEventHandler(con->pQamSrcEventHandler);

	if (SINK_MPSINK == sink_type)
	{
		con->pSink = new MediaPlayerSink;
	}
	else
	{
		con->pSink = new TestSink;
	}

	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.TEST", " %s:%d: Sink Created\n",__FUNCTION__,__LINE__);

	if(con->pSink == 0)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.TEST", " %s:%d : sink is invalid\n",__FUNCTION__,__LINE__);
		delete con->pSource;
		return 0;
	}

	con->pSink->init();
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.TEST", " %s:%d: Sink Init Complete\n",__FUNCTION__,__LINE__);

	if (SINK_MPSINK == sink_type)
	{
		(dynamic_cast<MediaPlayerSink*>(con->pSink))->setVideoRectangle(0, 0, 1280, 720);
		con->eventqueue = 0;
	}
	else
	{
		ret = rmf_osal_eventqueue_create((const uint8_t*)"EVTQ", &con->eventqueue);
		if(ret  != RMF_SUCCESS)
		{
		    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.TEST","%s rmf_osal_eventqueue_create failed(%d)\n",__FUNCTION__, ret);
		}
	}

	con->src_type = source_type;
	con->sink_type = sink_type;
	
	return RMF_SUCCESS;
}

rmf_Error stream (connection * con)
{
	RMFResult rmfresult;
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.TEST", "Enter %s:%d\n",__FUNCTION__,__LINE__);
	rmfresult = con->pSink->setSource ( con->pSource );
	if (RMF_RESULT_SUCCESS != rmfresult)
	{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.TEST", "%s:%d sink setsource failed\n",__FUNCTION__,__LINE__);
	}
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.TEST", " %s:%d: Sink setSource Complete\n",__FUNCTION__,__LINE__);
	rmfresult = con->pSource->play ();
	if (RMF_RESULT_SUCCESS != rmfresult)
	{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.TEST", "%s:%d source play failed\n",__FUNCTION__,__LINE__);
	}
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.TEST", " %s:%d: Src Play Complete\n",__FUNCTION__,__LINE__);
	return RMF_SUCCESS;
}


rmf_Error close(connection * con)
{
	RMFQAMSrc* qamsrc;
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.TEST", "Enter %s:%d\n",__FUNCTION__,__LINE__);
	con->pSource->removeEventHandler(con->pQamSrcEventHandler);
	delete con->pQamSrcEventHandler;
	con->pQamSrcEventHandler = NULL;
#ifndef QAMSRC_FACTORY
	con->pSource->close ();
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.TEST", " %s:%d: Src Close Complete\n",__FUNCTION__,__LINE__);
	con->pSource->term ();
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.TEST", " %s:%d: Src term Complete\n",__FUNCTION__,__LINE__);
#endif
	con->pSink->term ();
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.TEST", " %s:%d: Sink term Complete\n",__FUNCTION__,__LINE__);

#ifndef QAMSRC_FACTORY
	delete con->pSource;
	con->pSource = NULL;
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.TEST", " %s:%d: Src delete Complete\n",__FUNCTION__,__LINE__);
#else
	qamsrc = (RMFQAMSrc* )con->pSource;

#ifndef USE_QAMSRC_CHANGEURI
	RMFQAMSrc::freeQAMSourceInstance( qamsrc);
	con->pSource = NULL;
#endif

#endif

	delete con->pSink;
	con->pSink = NULL;
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.TEST", "Leaving %s:%d : Sink delete Complete\n",__FUNCTION__,__LINE__);

	if (con->sink_type == SINK_HN )
	{
		rmf_osal_eventqueue_delete(con->eventqueue);
	}

	return RMF_SUCCESS;
}

rmf_Error stop(connection * con)
{
	RMFResult rmfresult = RMF_RESULT_SUCCESS;
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.TEST", "Enter %s:%d\n",__FUNCTION__,__LINE__);
	rmfresult = con->pSource->pause ();
	if (RMF_RESULT_SUCCESS != rmfresult)
	{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.TEST", "%s:%d source pause failed\n",__FUNCTION__,__LINE__);
	}
	rmfresult = con->pSink->setSource ( NULL );
	if (RMF_RESULT_SUCCESS != rmfresult)
	{
		RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.TEST", "%s:%d sink setSource failed\n",__FUNCTION__,__LINE__);
	}
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.TEST", "Leaving %s:%d: Src pause Complete\n",__FUNCTION__,__LINE__);
	return RMF_SUCCESS;
}

#ifdef TEST_QAMSRC_DIAG

void printProgramInfo(uint32_t srcId )
{
	ProgramInfo pgmInfo;
	if (RMF_SUCCESS != getProgramInfo(srcId, &pgmInfo) )
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.TEST", "getProgramInfo failed srcId = %d\n", srcId);
		return;
	}
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TEST", "srcId:%d  cci = %d.pmtPid= %d.pcrPid= %d.numMediaPids= %d\n", 
			srcId, pgmInfo.cci, pgmInfo.pmtPid, pgmInfo.pcrPid, pgmInfo.numMediaPids);
	for (int i=0; i< pgmInfo.numMediaPids; i++)
	{
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TEST", "Pid =%d, Type = %d\n",
			pgmInfo.pPidList[i].pid, pgmInfo.pPidList[i].streamType);
	}
	
}

static void qamsrc_diag_test_thread(void * arg)
{
	uint32_t numberOfTuners;
	TunerData tunerData;
	rmf_Error ret;
	uint32_t ltsId;

	numberOfTuners = getTunerCount();
	while (1)
	{
		for( uint32_t i = 0; i < numberOfTuners; i++)
		{
			ret  = getTunerInfo(i, &tunerData);
			if (RMF_SUCCESS != ret)
			{
				RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.TEST", "%s:%d getTunerInfo failed\n",__FUNCTION__,__LINE__);
				exit(0);
			}
			ltsId =  getTSID( i);
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TEST", "Tuner Info for index %d \n", i);
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.TEST", "frequency= %d.modulation= %d.totalTuneCount= %d.tuneFailCount= %d.tunerState= %d.carrierLockLostCount= %d.lastTuneFailFreq = %d\n\n", tunerData.frequency, tunerData.modulation, tunerData.totalTuneCount, tunerData.tuneFailCount, tunerData.tunerState, tunerData.carrierLockLostCount, tunerData.lastTuneFailFreq);
		}
		printProgramInfo (0xA0);
		printProgramInfo (0x9E);
		sleep(10);
	}
}
#endif
void		gst_init			(int *argc, char **argv[]);
int main()
{
	int option;
	int display_menu = 1;
	rmf_Error ret;
	int i, j;
	int index = 0;
	rmf_osal_Bool b_mediastreamer_open = FALSE;
	rmf_osal_Bool b_mediastreamer_streaming = FALSE;
	uint32_t source_ids[RMF_TEST_SRC_NO];
	connection connections[RMF_TEST_SRC_NO];

	rmfPlatform *mPlatform = rmfPlatform::getInstance();
	mPlatform->init( 0, NULL);

	RMFQAMSrc::init_platform();

	OOBSIManager * oobsim = OOBSIManager::getInstance();

	memset (connections, 0, RMF_TEST_SRC_NO*sizeof (connection));

	std::vector<uint32_t> source_id_list;
	oobsim->get_source_id_vector(  source_id_list);
#ifdef QAMSRC_FACTORY
#ifdef TEST_MPSINK
	RMFQAMSrc::disableCaching();
#endif
#endif

#ifdef TEST_QAMSRC_DIAG
	rmf_osal_ThreadId thid;
	rmf_osal_threadCreate( qamsrc_diag_test_thread, NULL,
			RMF_OSAL_THREAD_PRIOR_DFLT, RMF_OSAL_THREAD_STACK_SIZE, &thid,
			"QAMDiag");
#endif
#if 0
	if (!gst_is_initialized())
	{
		char* argv[] = {(char*)"qamtest", 0};
		char** gst_argv = argv;
		int argc = 1;
		gst_init(&argc,&gst_argv);
	}
#endif
	while (1)
	{
		if (display_menu)
		{
			i = 0;
			printf ("\n************RMF Test Streamer ************\n");
			for (std::vector<uint32_t>::iterator it = source_id_list.begin(); it != source_id_list.end(); it++)
			{
				source_ids[i%RMF_TEST_SRC_NO] = *it;
				if ( i%RMF_TEST_SRC_NO == RMF_TEST_SRC_NO-1)
				{
					std::cout << i/RMF_TEST_SRC_NO+1<< " : Tune to source ids " ;
					for (j =0; j<RMF_TEST_SRC_NO; j++)
						std::cout <<source_ids[j]<<" (0x" << std::hex << source_ids[j] <<") " << std::dec;
					std::cout <<std::endl;
					i++;
				}
#if RMF_TEST_SRC_NO != 1
				else
				{
					i++;
				}
#endif
			}
			printf ("0 : exit \n");
			printf ("\n************RMF Test Streamer ************\n");
		}
		else
		{
			display_menu = 1;
		}

		option = getchar();
		display_menu = 1;
		if ( '\n' == option)
		{
			display_menu = 0;
			continue;
		}
		else if (option < '0' || option >= ('0'+i/RMF_TEST_SRC_NO+1) )
		{
			std::cout << option<< " Invalid option " << std::endl;
			continue;
		}

		index = option - '0' -1;
		for (j = 0; j<RMF_TEST_SRC_NO; j++)
		{
			source_ids[j] = source_id_list[index*RMF_TEST_SRC_NO+j];
		}
		std::cout <<"Selected source ids: ";
		for (j = 0; j<RMF_TEST_SRC_NO; j++)
		{
			std::cout <<source_ids[j] << " ";
		}
		std::cout <<std::endl;


		if ( TRUE == b_mediastreamer_streaming )
		{
			for (j = 0; j<RMF_TEST_SRC_NO; j++)
			{
				ret = stop ( &connections[j]);

				if ( RMF_SUCCESS != ret )
				{
					RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.TEST", "%s:%d stop failed\n",__FUNCTION__,__LINE__);
					return -1;
				}
			}
			b_mediastreamer_streaming = FALSE;
		}

		if ( TRUE == b_mediastreamer_open )
		{
			for (j = 0; j<RMF_TEST_SRC_NO; j++)
			{
				ret = close ( &connections[j]);

				if ( RMF_SUCCESS != ret )
				{
					RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.TEST", "%s:%d close failed\n",__FUNCTION__,__LINE__);
					return -1;
				}
			}
			b_mediastreamer_open = FALSE;
		}

		if ( '0' == option)
		{
			RMFQAMSrc::freeCachedQAMSourceInstances();
			break;
		}

		for (j = 0; j<RMF_TEST_SRC_NO; j++)
		{
			uint32_t sink_type = SINK_HN;
#ifdef TEST_MPSINK
			if ( 0 == j) sink_type = SINK_MPSINK;
#endif 
			ret = open ( &connections[j],  source_ids[j] , SRC_QAMSRC, sink_type);

			if ( RMF_SUCCESS != ret )
			{
				RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.TEST", "%s:%d open %d failed\n",__FUNCTION__,__LINE__, j);
				return -1;
			}
			RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.TEST", "%s:%d open %d success\n",__FUNCTION__,__LINE__, j);
		}
		b_mediastreamer_open = TRUE;

		for (j = 0; j<RMF_TEST_SRC_NO; j++)
		{
			ret = stream ( &connections[j]);

			if ( RMF_SUCCESS != ret )
			{
				RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.TEST", "%s:%d stream %d failed\n",__FUNCTION__,__LINE__, j);
				return -1;
			}
			RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.TEST", "%s:%d stream %d success\n",__FUNCTION__,__LINE__, j);
		}
		b_mediastreamer_streaming = TRUE;
#ifdef TEST_WITH_PODMGR
		{
			int ct;
			rmf_transport_path_info path_info;
			unsigned char ltsId;
			for (ct = 0; ct < 6; ct ++ )
			{
				rmf_qamsrc_getLtsid ( ct, &ltsId );
				printf("\n lts id of tuner %d = %d\n", ct, ltsId);
				if (0 != rmf_qamsrc_getTransportPathInfoForLTSID(ltsId ,  &path_info))
				{
					printf("Error in getting path info.\n");
				}
				else
				{
					printf("path_info - dmxhandle = 0x%x, seccontexthandle= 0x%x, tsd key location = %d\n", 
						path_info.demux_handle, path_info.hSecContextFor3DES, path_info.tsd_key_location);
				}
			}
		}
#endif
	}

	return 0;
}


