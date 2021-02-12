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

#include <semaphore.h>
#include <algorithm>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "rmfqamsrcpriv.h"
#include "rmfqamsrc.h"
#include "QAMSrcDiag.h"

#include "rmf_qamsrc_common.h"
#include "rmf_osal_event.h"
#include "rmf_osal_thread.h"
#include "rdk_debug.h"
#include "rmf_error.h"
#include "rmf_osal_ipc.h"
#include <gst/base/gstbasesrc.h>

#ifdef USE_DVR
#include "dvrmanager-socif.h"
#endif

#ifdef TEST_WITH_PODMGR
#include <rmf_pod.h>
#include "disconnectMgr.h"
#else
#include <rmf_oobsimanagerstub.h>
#endif

#if USE_SYSRES_MLT
#include "rpl_new.h"
#include "rpl_malloc.h"
#endif

#include "libIBus.h"
#include "sysMgr.h"

/* Events posted to Play Thread */
#define RMF_QAM_TERMINATE_THREAD    0xff

#define SOURCEID_PRIFIX_STRING "ocap://0x" 

#define TUNEPARAMS_PRIFIX_STRING "tune://" 

#define SI_EVENT_WAIT_TIMEOUT_US  500000 /* .5 seconds*/

#define PMT_TIME_OUT 3000 // 3 seconds

#define PAT_COND_TIME_OUT 1000 // 1 sec

#define CCI_WAIT_TIMEOUT_MS  1000 /* 1 seconds*/

#define AUTH_WAIT_TIMEOUT_MS  1000 /*Timeout after CCI event*/

#define PMT_MAX_SIZE 1024

#define QAMSRC_DEFAULT_QUEUE_SIZE 32

#define QAMSRC_DEFAULT_BUFFER_SIZE 131072 //(128 * 1024)

#define QAMSRC_UNSYNC_TO_FAIL_TRANSITION_TIME 2 //qamsource goes from tune unsync to tune fail after 2 seconds of non-recovery

typedef enum
{
	QAMSRC_TUNER_UNSYNC_BITMASK = 0x01,
	QAMSRC_PROGRAM_NUMBER_INVALID_BITMASK = (0x01 << 1),
	QAMSRC_SPTS_OUTAGE_BITMASK = (0x01 << 2),
	QAMSRC_MAX_BITMASK = (0x01 << 3)
}qamsrcErrorBitmask_t;


typedef enum
{
    PMT_COMPARISON_NO_MATCH = 0,
    PMT_COMPARISON_NEW_TABLE_IS_A_SUBSET,
    PMT_COMPARISON_NEW_TABLE_IS_A_FULL_MATCH
} qamsrcPmtComparisonResult_t;

typedef struct diag_tuner_info_s
{
	uint32_t frequency;
	uint32_t modulation;
	TunerState tunerState;
	uint32_t totalTuneCount;
	uint32_t tuneFailCount;
	uint32_t lastTuneFailFreq;
	uint32_t lockedTimeStamp;
	uint32_t carrierLockLostCount;
	uint16_t channelMapId;
	uint16_t dacId;
}diag_tuner_info_t;


struct qamsrcprivobjectlist
{
	RMFQAMSrcImpl* qamsrcimpl;
	struct qamsrcprivobjectlist* next;
};

typedef struct _qamEventData_t
{
    uint32_t cci;
    uint32_t qamContext;
    uint32_t handle;
} qamEventData_t;


static qamsrcprivobjectlist *g_qamsrcimpllist = NULL;

static uint32_t g_qamsrcimpl_index = 0;

static diag_tuner_info_t* ga_diag_info = NULL;

static rmf_transport_path_info *ga_transport_path_info  = NULL;

static bool bQAMSrcEIDCheck = false;
static bool bQAMSrcInited = false;
rmf_osal_Bool RMFQAMSrcImpl::supportTuningInNULLState = TRUE;

rmf_osal_Mutex RMFQAMSrcImpl::mutex ;
IPCServer *g_pQamSrcIPCServer = NULL;
static volatile uint32_t qam_src_server_cmd_port_no = 0;

/* Event queue to recieve events from POD*/
static rmf_osal_eventqueue_handle_t g_eventq = 0;

static unsigned int g_pat_timeout_count = 0;
static unsigned int g_pmt_timeout_count = 0;
static bool g_bQamSrcServerStarted = false;

void recv_qam_src_server_cmd( IPCServerBuf * pServerBuf, void *pUserData );
rmf_Error qamsrc_getVideoContentInfo(int iContent, struct VideoContentInfo * pVideoContentInfo);

rmf_Error qamSrcServerStart(	)
{
    if(g_bQamSrcServerStarted) return RMF_SUCCESS;
    g_bQamSrcServerStarted = true;
    
	const char *server_port;
	server_port = rmf_osal_envGet( "QAM_SRC_SERVER_CMD_PORT_NO" );
	if( NULL == server_port)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s: NULL POINTER  received for name string\n", __FUNCTION__ );
		qam_src_server_cmd_port_no = QAM_SRC_SERVER_CMD_DFLT_PORT_NO;
	}
	else
	{
		qam_src_server_cmd_port_no = atoi( server_port );
	}
	g_pQamSrcIPCServer = new IPCServer( ( int8_t * ) "qamSrcServer", qam_src_server_cmd_port_no );
	int8_t ret = g_pQamSrcIPCServer->start(  );

	if ( 0 != ret )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s : Server failed to start\n",
				 __FUNCTION__ );
		return RMF_OSAL_ENODATA;
	}

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC",
			 "%s : Server server1 started successfully\n", __FUNCTION__ );
	g_pQamSrcIPCServer->registerCallBk( &recv_qam_src_server_cmd, ( void * ) g_pQamSrcIPCServer );

	return RMF_SUCCESS;
}

rmf_Error qamSrcServerStop(	)
{
    if(false == g_bQamSrcServerStarted) return RMF_SUCCESS;
    g_bQamSrcServerStarted = false;

    if(NULL != g_pQamSrcIPCServer)
    {
        int8_t ret = g_pQamSrcIPCServer->stop(  );
        delete g_pQamSrcIPCServer;
        g_pQamSrcIPCServer = NULL;

        if ( 0 != ret )
        {
            RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s : Server failed to stop\n",
                     __FUNCTION__ );
            return RMF_OSAL_ENODATA;
        }

        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC",
                 "%s : Server server1 stopped successfully\n", __FUNCTION__ );
    }

	return RMF_SUCCESS;
}

void recv_qam_src_server_cmd( IPCServerBuf * pServerBuf, void *pUserData )
{
	uint32_t identifyer = 0;
	identifyer = pServerBuf->getCmd(  );
	IPCServer *pPODIPCServer = ( IPCServer * ) pUserData;
    
	switch ( identifyer )
	{
#ifdef SNMP_IPC_CLIENT
        case QAM_SRC_SERVER_CMD_GET_PAT_TIMEOUT_COUNT:
        {
            RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD","%s : IPC called QAM_SRC_SERVER_CMD_GET_PAT_TIMEOUT_COUNT \n", __FUNCTION__ );
            uint32_t ret = RMF_SUCCESS;
            uint32_t val = g_pat_timeout_count;
            pServerBuf->addResponse( ret );
            pServerBuf->addData( ( const void * ) &val, sizeof( val ) );
            pServerBuf->sendResponse(  );
            pPODIPCServer->disposeBuf( pServerBuf );				
        }
        break;

        case QAM_SRC_SERVER_CMD_GET_PMT_TIMEOUT_COUNT:
        {
            RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD","%s : IPC called QAM_SRC_SERVER_CMD_GET_PMT_TIMEOUT_COUNT \n", __FUNCTION__ );
            uint32_t ret = RMF_SUCCESS;
            uint32_t val = g_pmt_timeout_count;
            pServerBuf->addResponse( ret );
            pServerBuf->addData( ( const void * ) &val, sizeof( val ) );
            pServerBuf->sendResponse(  );
            pPODIPCServer->disposeBuf( pServerBuf );				
        }
        break;

        case QAM_SRC_SERVER_CMD_GET_VIDEO_CONTENT_INFO:
        {
            RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD","%s : IPC called QAM_SRC_SERVER_CMD_GET_VIDEO_CONTENT_INFO \n", __FUNCTION__ );
            uint32_t ret = RMF_SUCCESS;
            int iContent = 0;
            int32_t result_recv = 0;
            result_recv |= pServerBuf->collectData( (  void * ) &iContent, sizeof( iContent ) );
            if ( result_recv < 0 )
            {
                    RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
                            "%s : Error !! Failed to collect data for QAM_SRC_SERVER_CMD_GET_VIDEO_CONTENT_INFO!!\n",
                            __FUNCTION__ );
                    /* send -1 response */
                    pServerBuf->addResponse( RMF_OSAL_ENODATA );
                    pServerBuf->sendResponse(  );
                    pPODIPCServer->disposeBuf( pServerBuf );
                    break;
            }
            VideoContentInfo videoContentInfo;
            ret = qamsrc_getVideoContentInfo(iContent, &videoContentInfo);
            pServerBuf->addResponse( ret );
            pServerBuf->addData( ( const void * ) &videoContentInfo, sizeof( videoContentInfo ) );
            pServerBuf->sendResponse(  );
            pPODIPCServer->disposeBuf( pServerBuf );				
        }
        break;

#endif //SNMP_IPC_CLIENT
/*End :: SNMP Client **************/	  
        default:
        {
            RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
                    "%s: unable to identify the identifyer from packet = %x",
                    __FUNCTION__, identifyer );
            pPODIPCServer->disposeBuf( pServerBuf );
        }
        break;
	}
}

rmf_Error qamsrc_getVideoContentInfo(int iContent, struct VideoContentInfo * pVideoContentInfo)
{
    rmf_Error ret = RMF_OSAL_ENODATA;
    if(NULL != pVideoContentInfo)
    {
        memset(pVideoContentInfo, 0, sizeof(*pVideoContentInfo));
        rmf_osal_mutexAcquire( RMFQAMSrcImpl::mutex);
        {
            qamsrcprivobjectlist *pqamsrcpriv = g_qamsrcimpllist;
            int iQamObject = 0;
            while(NULL != pqamsrcpriv)
            {
                if(iContent == iQamObject)
                {
                    ret = RMF_SUCCESS;
                    break;
                }
                iQamObject++;
                pqamsrcpriv = pqamsrcpriv->next;
            }
            
            if(RMF_SUCCESS == ret)
            {
                if((NULL != pqamsrcpriv) && (NULL != pqamsrcpriv->qamsrcimpl))
                {
                    pqamsrcpriv->qamsrcimpl->getVideoContentInfo(iContent, pVideoContentInfo);
                }
            }
        }
        rmf_osal_mutexRelease( RMFQAMSrcImpl::mutex);
    }
    return ret;
}

void qamsrc_eventmonitor_thread( void* arg);

/*Arg  - pointer to RMFQAMSrcImpl object */
void qamsrc_priv_start_simonitor( void* arg);

void qamsrc_update_diag_info( GstElement* element);

static void signals_disconnected(gpointer object, GClosure *closure)
{
	((RMFQAMSrcImpl*)object)->notifySignalsDisconnected();
}

static gboolean SyncLossTimerCallback(gpointer data)
{
	return ((RMFQAMSrcImpl *)data)->syncLossTimerCallback();
}
static void SyncLossTimerCancelledCallback(gpointer object)
{
	((RMFQAMSrcImpl*)object)->syncLossTimerExited();
}

void RMFQAMSrcImpl::syncLossTimerExited()
{
	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "Sync loss timer disconnected. 0x%x\n" , this);
	rmf_osal_condSet(m_sync_loss_timer_disconnected);
}

gboolean RMFQAMSrcImpl::syncLossTimerCallback()
{
	rmf_osal_Bool sendError = FALSE;
	rmf_osal_mutexAcquire(m_mutex);
	if(RMF_QAMSRC_ERROR_TUNER_NOT_LOCKED == m_qamtuner_error)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "Signal has been down for a while. Giving up on %s\n" , saved_uri.c_str());
		sendError = TRUE;
		m_qamtuner_error = RMF_QAMSRC_ERROR_TUNER_UNLOCK_TIMEOUT;
	}
	else
	{
		RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "Cancelling sync loss timer. Source ID: %s.\n" , saved_uri.c_str());
	}
	m_sync_loss_callback_id = 0;
	rmf_osal_mutexRelease(m_mutex);

	if(sendError)
	{
		notifyError(RMF_QAMSRC_ERROR_TUNER_UNLOCK_TIMEOUT, NULL);
	}
	return FALSE;
}

/* call back funtion from gstqamtuenrsrc */
static void tunerStatusListner( GstElement * gstqamsrc, gint status, RMFQAMSrcImpl* qamPriv)
{   
	qamPriv->tunerStatusListnerImpl ( gstqamsrc, status);
}/* End of tunerStatusListner */

/* To post play event to start_play thread for feeding QAMsrc buffer to gst plugin for playback */
void RMFQAMSrcImpl::tunerStatusListnerImpl(GstElement * gstqamsrc, gint status)
{
	rmf_Error ret = RMF_SUCCESS;
	rmf_osal_Bool notifyAllClear = false;

	RDK_LOG(RDK_LOG_TRACE1,"LOG.RDK.QAMSRC", "Enter %s():%d \n" , __FUNCTION__, __LINE__);

	qamsrc_update_diag_info(src_element);

	switch (status)
	{
		case TUNERSRC_EVENT_QUEUE_CHECKPOINT:
			{
				uint32_t queueLength = 0;
				GstElement* queue = gst_bin_get_by_name(GST_BIN(getPipeline()), "qamsrc_bin-hnsink_bin-queue");
				if(queue)
				{
					g_object_get( queue, "current-level-buffers", &queueLength, NULL );
					if(queueLength > maxQueueSize)
					{
						maxQueueSize = queueLength;
					}
					gst_object_unref(queue);
				}
			}
			break;
		case TUNERSRC_EVENT_TUNE_FAIL:
			if ( pInfo.prog_num != 0 )
			{
				RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "RMFQAMSrc - Tune request failed\n");
			}
			else
			{
				RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "VOD AutoDiscovery - Tune request failed\n");
			}
			rmf_osal_mutexAcquire(m_mutex);
			setErrorBits(QAMSRC_TUNER_UNSYNC_BITMASK, TRUE);
			m_is_tuner_locked = FALSE;
			rmf_osal_mutexRelease(m_mutex);
			m_qamtuner_error = RMF_QAMSRC_ERROR_TUNE_FAILED;
			psiAcqStartTime = 0; //This will suppress all PSI timeouts.
			notifyError(RMF_QAMSRC_ERROR_TUNE_FAILED, NULL);
			break;

		case TUNERSRC_EVENT_TUNE_SYNC:
			m_timekeeper.log_time(timekeeper::TUNE_END);
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "RMFQAMSrc - Tune request -Sync tune started\n");
			m_qamtuner_error = 0;
			rmf_osal_mutexAcquire(m_mutex);
			notifyAllClear = setErrorBits(QAMSRC_TUNER_UNSYNC_BITMASK, FALSE);
			m_is_tuner_locked = TRUE;
			rmf_osal_mutexRelease(m_mutex);
			ret = rmf_osal_timeGetMillis(&psiAcqStartTime);
			if(RMF_SUCCESS != ret )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", 
					"%s: rmf_osal_timeGetMillis Failed ret= %d\n", __FUNCTION__, ret);
			}

			notifyStatusChange( RMF_QAMSRC_EVENT_TUNE_SYNC);
			if(TRUE == notifyAllClear)
			{
				RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "Sending all-clear for %s.\n", saved_uri.c_str());
				notifyStatusChange(RMF_QAMSRC_EVENT_ERRORS_ALL_CLEAR);
			}
			break;

		case TUNERSRC_EVENT_TUNE_UNSYNC:
			RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.QAMSRC", "RMFQAMSrc - Tune request returned unsync tune. Tuner not locked\n");			
			m_qamtuner_error = RMF_QAMSRC_ERROR_TUNE_FAILED; /* using RMF_QAMSRC_ERROR_TUNE_FAILED instead of 
			RMF_QAMSRC_ERROR_TUNER_UNLOCK_TIMEOUT. That way, if we get a tune request for the same service later, we send
			"tune fail" event. Player treats the two errors differently. "Tune fail" events are notified to the server sooner,
			allowing quicker reaction to the error.*/
			psiAcqStartTime = 0; //This will suppress all PSI timeouts.
			notifyError(RMF_QAMSRC_ERROR_TUNER_UNLOCK_TIMEOUT, NULL);

			rmf_osal_mutexAcquire(m_mutex);
			setErrorBits(QAMSRC_TUNER_UNSYNC_BITMASK, TRUE);

			m_is_tuner_locked = FALSE;
			rmf_osal_mutexRelease(m_mutex);
			break;

		case TUNERSRC_EVENT_FILTERS_SET:
			{
#ifdef CAMGR_PRESENT
				int ltsid;
				g_object_get( src_element, "ltsid", &ltsid, NULL );
#endif
#ifdef TEST_WITH_PODMGR
				if ( 0 != sessionHandlePtr )
				{
					int result;
					int ltsid;
					int i = 0;
					unsigned long pids_array[100] = {0}; //  correct dimension
					char cci;
					rmf_osal_Bool authorization_status = FALSE;

					g_object_get( src_element, "ltsid", &ltsid, NULL );

					for(; i<mediaInfo.numMediaPids;i++) 
					{
						pids_array[i] =mediaInfo.pPidList[i].pid;
						RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "%s:: pids_array[%d] = %d \n",__FUNCTION__, i, pids_array[i]);
					}
#ifdef QAMSRC_WAITS_FOR_CCI
					result = waitForCCI( cci );
					if( RMF_RESULT_SUCCESS  != result)
					{
						RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "RMFQAMSrc - Tune request -failed in obtaining CCI value\n");
						break;
					}
					RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "WaitforCCI - Obtained CCI value as %c\n",cci);

					if ( RMF_QAMSRC_CCI_STATUS_COPYING_IS_PERMITTED != cci )
					{
#endif
						RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s:: Calling rmf_podmgrConfigCipher :  ltsid = %d. Source ID: %s \n",
							__FUNCTION__, ltsid, saved_uri.c_str());
						//result = rmf_podmgrConfigCipher((unsigned char)ltsid, /*unsigned short PrgNum*/ 0, pids_array, mediaInfo.numMediaPids);
						result = rmf_podmgrConfigCipher((unsigned char)ltsid, /*unsigned short PrgNum*/ 0, mediaInfo.pPidList, mediaInfo.numMediaPids);
						if( 0 != result)
						{
							RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s:: vlConfigCipherPidFilterSet failed result = %d \n",__FUNCTION__, result);
						}
						else
						{
							RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s:: vlConfigCipherPidFilterSet SUCCESS \n",__FUNCTION__);
						}
#ifdef QAMSRC_WAITS_FOR_CCI
					}
					else
					{
						RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s:: Not calling configCipher since CCI = RMF_QAMSRC_CCI_STATUS_COPYING_IS_PERMITTED \n",__FUNCTION__);
					}
#endif
					/* Set "ready" if this session is already authorized. */
					rmf_osal_mutexAcquire (m_mutex); 
					if (TRUE == m_isAuthorized)
					{
						RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "%s():%d  this = %p Not waiting since already available\n" , __FUNCTION__, __LINE__, this);
						g_object_set  (src_element, "ready", NULL, NULL);
						g_object_set(src_element, "isAuthorized", TRUE, NULL);
						RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "RMFQAMSrc - Tune request -filters set and streaming started. Source ID: %s.\n", saved_uri.c_str());
						notifyStatusChange( RMF_QAMSRC_EVENT_STREAMING_STARTED);
						m_pod_error = 0;
					}
					rmf_osal_mutexRelease(m_mutex);

				}
				else
				{
					RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s:: Not calling configCipher since podsession NULL \n",__FUNCTION__);
				}
#else
				g_object_set  (src_element, "ready", NULL, NULL);
				RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "RMFQAMSrc - Tune request -filters set and streaming started. Source ID: %s.\n", saved_uri.c_str());
				notifyStatusChange( RMF_QAMSRC_EVENT_STREAMING_STARTED);
#endif
				
			}
			break;

		case TUNERSRC_EVENT_UNRECOVERABLE_ERROR:
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "RMFQAMSrc - Tune request returned Unrecoverable error\n");
			m_qamtuner_error = RMF_QAMSRC_ERROR_UNRECOVERABLE_ERROR;
			notifyError(RMF_QAMSRC_ERROR_UNRECOVERABLE_ERROR, NULL);
			break;

		case TUNERSRC_EVENT_TUNE_SUCCESS:

			if ( pInfo.prog_num != 0 )
			{
				RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "RMFQAMSrc - Tune request success\n");
			}
			else
			{
				RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "VOD AutoDiscovery - Tune request success\n");
			}
			m_qamtuner_error = 0;
			break;
		case TUNERSRC_EVENT_SPTS_TIMEOUT:
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s reports SPTS timeout\n", saved_uri.c_str());
			notifySIMonitorThread(RMF_QAMSRC_ERROR_SPTS_TIMEOUT);
			break;

		case TUNERSRC_EVENT_SPTS_OK:
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s reports SPTS is OK.\n", saved_uri.c_str());
			notifySIMonitorThread(RMF_QAMSRC_EVENT_SPTS_OK);
			break;

		default:
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "RMFQAMSrc - Tune request returned unknown\n");			
			break;
	}
	RDK_LOG(RDK_LOG_TRACE1,"LOG.RDK.QAMSRC", "Exit %s():%d \n" , __FUNCTION__, __LINE__);
}/* End of RMFQAMSrcImpl::tunerStatusListnerImpl */


/* ************ RMFQAMSrcImpl function implementation ******************** */

#ifdef CAMGR_PRESENT
static void rmfqamsrc_CaMgrNotify(camgr_ctx_t ctx, camgr_svc_t svc, camgr_dtype_t data_type, void* data_ptr, uint32_t data_size, void* user_data)
{
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "++%s(ctx: %d, svc: %d, data_type: %d, data_ptr: %p, data_size: %d, user_data: %p)\n",
			__FUNCTION__, ctx, svc, data_type, data_ptr, data_size, user_data);
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s(ctx: %d, svc: %d)\n", __FUNCTION__, ctx, svc);
}
#endif

RMFQAMSrcImpl::RMFQAMSrcImpl(RMFMediaSourceBase* parent) :RMFMediaSourcePrivate(parent)
{
	rmf_Error ret;

	RDK_LOG(RDK_LOG_TRACE1,"LOG.RDK.QAMSRC", " RMFQAMSrcImpl::%s:%d \n" , __FUNCTION__, __LINE__);
	const char * queueSize = rmf_osal_envGet("QAMSRC.QUEUE.SIZE");
	m_pat_timeout_offset = 0;
	m_validateTsId = false;
	m_tsIdForValidation = 0;
	m_pmt_optimization_active = FALSE;

	m_error_bitmap = 0;
	m_is_tuner_locked = FALSE;
	if (NULL == queueSize )
	{
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s: queue size not specified in config file. "
				" Using default value %d.\n", __FUNCTION__,  QAMSRC_DEFAULT_QUEUE_SIZE);
		setQueueSize(QAMSRC_DEFAULT_QUEUE_SIZE );
	}
	else
	{
		uint32_t sizeOfQueue =  atoi(queueSize);
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s: configuring queue size as %d.\n", __FUNCTION__, sizeOfQueue);
		setQueueSize( sizeOfQueue);
	}
#ifndef  DISABLE_INBAND_MGR
	setQueueLeaky( true);
#endif
	state = RMFQAMSrcImpl::QAM_STATE_IDLE;
	pInbandSI = NULL;
#ifdef TEST_WITH_PODMGR
	sessionHandlePtr = 0;
#endif
	m_pod_error = 0;
	m_qamtuner_error = 0;
	m_si_error = 0;
	decimalSrcId = 0;
	m_sync_loss_callback_id = 0;
	memset(&mediaInfo, 0, sizeof (rmf_MediaInfo));

	siEventQHandle = 0;

	ret = rmf_osal_eventqueue_create( (const uint8_t*) "SIEvtQ", &siEventQHandle);
	if (RMF_SUCCESS != ret )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - rmf_osal_eventqueue_create failed.\n", __FUNCTION__);
		return;
	}
	else
	{
		 RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s: the queue created is 0x%x\n", __FUNCTION__, siEventQHandle);
	}

	/* To ensure proper termination of thread() */
	siDoneSem =  new sem_t;
	if (NULL == siDoneSem)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "Failed to allocate sem in %s\n", __FUNCTION__ );
		return;
	}

	ret = rmf_osal_mutexNew(&m_mutex);
	if (RMF_SUCCESS != ret )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - rmf_osal_mutexNew failed.\n", __FUNCTION__);
		return;
	}

	ret  = sem_init( siDoneSem, 0 , 0);
	if (0 != ret )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - sem_init failed.\n", __FUNCTION__);
		return;
	}
	ret = rmf_osal_condNew( TRUE, FALSE, &pat_cond);
	if (RMF_SUCCESS != ret )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - rmf_osal_condNew failed.\n", __FUNCTION__);
		return;
	}
	ret = rmf_osal_condNew( FALSE, FALSE, &m_signals_disconnected);
	if (RMF_SUCCESS != ret )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - rmf_osal_condNew failed.\n", __FUNCTION__);
		return;
	}
	ret = rmf_osal_condNew( FALSE, TRUE, &m_sync_loss_timer_disconnected);
	if (RMF_SUCCESS != ret )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - rmf_osal_condNew failed.\n", __FUNCTION__);
		return;
	}
#ifdef TEST_WITH_PODMGR
	m_cciCond = g_cond_new();
	m_cciMutex = g_mutex_new ();
	m_cci = RMF_QAMSRC_CCI_STATUS_NONE; 
	m_isAuthorized = FALSE;
#endif

    updateSourcetoLive(true);
}/* End of RMFQAMSrcImpl */


RMFQAMSrcImpl::~RMFQAMSrcImpl()
{
	rmf_Error rmf_error;

	RDK_LOG(RDK_LOG_TRACE1,"LOG.RDK.QAMSRC", " RMFQAMSrcImpl::%s:%d \n" , __FUNCTION__, __LINE__);

	/*	At this point SIMonitorThread will exit properly */
	rmf_error = rmf_osal_eventqueue_delete(siEventQHandle);   
	if(rmf_error!= RMF_SUCCESS)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - rmf_osal_eventqueue_delete failed\n", __FUNCTION__);
	}

	rmf_error = sem_destroy( siDoneSem);
	if (0 != rmf_error )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - sem_destroy failed.\n", __FUNCTION__);
	}
	delete siDoneSem;

#ifdef TEST_WITH_PODMGR
	g_cond_free( m_cciCond);
	g_mutex_free( m_cciMutex); 
#endif
	rmf_osal_mutexDelete( m_mutex);
	rmf_osal_condDelete( pat_cond);
	rmf_osal_condDelete(m_signals_disconnected);
	rmf_osal_condDelete(m_sync_loss_timer_disconnected);
	RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", " End RMFQAMSrcImpl::%s:%d \n" , __FUNCTION__, __LINE__);
}/* End of ~RMFQAMSrcImpl */

/* Function to initialize Tuner, SI and appsrc plugin for playback */
RMFResult RMFQAMSrcImpl::init()
{
	RMFResult res;
	rmf_Error rmf_error;
	qamsrcprivobjectlist *pqamsrcpriv = NULL;
	uint32_t buffSize = QAMSRC_DEFAULT_BUFFER_SIZE;
	RDK_LOG(RDK_LOG_DEBUG,"LOG.RDK.QAMSRC", "Enter impl %s():%d \n" , __FUNCTION__, __LINE__);


	res = RMFMediaSourcePrivate::init();
	if ( RMF_RESULT_SUCCESS != res)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - private init failed.\n", __FUNCTION__);
		return res;
	}

	res = populateBin(getSource());
	if(RMF_RESULT_SUCCESS != res)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - could not make gstqamtunersrc element.\n", __FUNCTION__);
		RMFMediaSourcePrivate::term();
		return RMF_QAMSRC_ERROR_NO_TUNERS_AVAILABLE;
	}

	rmf_osal_condUnset(m_signals_disconnected);
	status_signal_handler_id = g_signal_connect_data(src_element, "qamtunersrc-status", G_CALLBACK(tunerStatusListner),
		this, &signals_disconnected, (GConnectFlags) 0);
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.QAMSRC", "%s  status_signal_handler_id=0x%x\n",__FUNCTION__, status_signal_handler_id);

	const char * bufferSize = rmf_osal_envGet("QAMSRC.BUFFER.SIZE");
	if (NULL == bufferSize )
	{
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s: buffer size not specified in config file. "
				" Using default value %d.\n", __FUNCTION__,  QAMSRC_DEFAULT_BUFFER_SIZE);
	}
	else
	{
		buffSize=  atoi(bufferSize);
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s: configuring buffer size as %d.\n", __FUNCTION__, buffSize);
	}	
	g_object_set  (src_element, "bufsize", buffSize, NULL);

	if(FALSE == supportTuningInNULLState)
	{
		/*Tuner will not function in NULL state. Move it to READY state. */
		setState(GST_STATE_READY);
	}
	rmf_error =  rmf_osal_memAllocP (RMF_OSAL_MEM_MEDIA, sizeof (qamsrcprivobjectlist), (void**)&pqamsrcpriv);
	if (RMF_SUCCESS != rmf_error )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - rmf_osal_memAllocP failed.\n", __FUNCTION__);
	}
	else
	{
		rmf_osal_mutexAcquire( RMFQAMSrcImpl::mutex);
		m_context = ++g_qamsrcimpl_index;
		pqamsrcpriv->qamsrcimpl = this;
		pqamsrcpriv->next = g_qamsrcimpllist;
		g_qamsrcimpllist = pqamsrcpriv;
		rmf_osal_mutexRelease( RMFQAMSrcImpl::mutex);
	}

#ifdef CAMGR_PRESENT
	camgr_ctx = CAMgr_ContextCreate();
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "CAMgr_ContextCreate() - camgr_ctx: %d\n", camgr_ctx);
	if (camgr_ctx != CAMGR_INVALID_CONTEXT)
	{
		camgr_mask_t mask = CAMGR_SERVICE_STATUS_MASK | CAMGR_MISSING_MASK;
		camgr_result_t result;

		result = CAMgr_ContextSetListener(camgr_ctx, mask, rmfqamsrc_CaMgrNotify, NULL);
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "CAMgr_ContextSetListener(camgr_ctx: %d, mask: %X, ..., ...) - result: %d\n",
			camgr_ctx, mask, result);
	}

	camgr_svc = CAMGR_INVALID_SERVICE;
	catTable=NULL;
	catTableSize=0;
#endif

	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "Exit impl %s():%d this = %p\n" , __FUNCTION__, __LINE__, this);
	return res;
}/* End of RMFQAMSrcImpl::init */


/* Terminates QAMSrc module */
RMFResult RMFQAMSrcImpl::term()
{
	RMFResult res;
	qamsrcprivobjectlist *pqamsrcpriv = NULL, *pqamsrcprivprev = NULL;

	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "Enter impl %s():%d this = %p\n" , __FUNCTION__, __LINE__, this);

#ifdef CAMGR_PRESENT
	if (camgr_ctx != CAMGR_INVALID_CONTEXT)
	{
		camgr_result_t result;

		result = CAMgr_ContextDispose(camgr_ctx);
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "CAMgr_ContextDispose(camgr_ctx: %d) - result: %d\n", camgr_ctx, result);
		camgr_ctx = CAMGR_INVALID_CONTEXT;
		camgr_svc = CAMGR_INVALID_SERVICE;
	}

	if (catTable != NULL)
	{
		rmf_osal_memFreeP ( RMF_OSAL_MEM_MEDIA, catTable );
	}

	catTableSize = 0;
#endif

	rmf_osal_mutexAcquire( RMFQAMSrcImpl::mutex);
	pqamsrcpriv = g_qamsrcimpllist;
	pqamsrcprivprev = g_qamsrcimpllist;
	while (pqamsrcpriv)
	{
		if (pqamsrcpriv->qamsrcimpl == this )
		{
			if ( g_qamsrcimpllist == pqamsrcpriv)
			{
				g_qamsrcimpllist = g_qamsrcimpllist->next;
			}
			else
			{
				pqamsrcprivprev->next = pqamsrcpriv->next;
			}
			rmf_osal_memFreeP ( RMF_OSAL_MEM_MEDIA, pqamsrcpriv );
			break;
		}
		pqamsrcprivprev = pqamsrcpriv;
		pqamsrcpriv = pqamsrcpriv->next;
	}
	rmf_osal_mutexRelease( RMFQAMSrcImpl::mutex);

	g_signal_handler_disconnect( src_element, status_signal_handler_id);
	rmf_osal_condWaitFor(m_signals_disconnected, 0);

	res = RMFMediaSourcePrivate::term();
	src_element = NULL;

	RDK_LOG(RDK_LOG_DEBUG,"LOG.RDK.QAMSRC", "Exit impl %s():%d \n" , __FUNCTION__, __LINE__);
	return res;
}/* End of RMFQAMSrcImpl::term */

RMFResult RMFQAMSrcImpl::setEvents(IRMFMediaEvents* events)
{
	RDK_LOG(RDK_LOG_TRACE1,"LOG.RDK.QAMSRC", "Enter %s():%d \n" , __FUNCTION__, __LINE__);

	RMFMediaSourcePrivate::setEvents( events);

	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s():%d m_qamtuner_error = 0x%x m_pod_error= 0x%x\n" ,
			__FUNCTION__, __LINE__, m_qamtuner_error, m_pod_error);

	if ( 0 != m_qamtuner_error )
		notifyError( m_qamtuner_error, NULL);

	if ( 0 != m_pod_error)
		notifyError( m_pod_error, NULL);

	RDK_LOG(RDK_LOG_TRACE1,"LOG.RDK.QAMSRC", "Exit %s():%d \n" , __FUNCTION__, __LINE__);
	return RMF_RESULT_SUCCESS;
}

RMFResult RMFQAMSrcImpl::addEventHandler(IRMFMediaEvents* events)
{
	RDK_LOG(RDK_LOG_TRACE1,"LOG.RDK.QAMSRC", "Enter %s():%d \n" , __FUNCTION__, __LINE__);

	RMFMediaSourcePrivate::addEventHandler( events);

	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s():%d m_qamtuner_error = 0x%x m_pod_error= 0x%x\n" ,
			__FUNCTION__, __LINE__, m_qamtuner_error, m_pod_error);

	if ( 0 != m_qamtuner_error )
		events->error(m_qamtuner_error, getErrorString(m_qamtuner_error));

	if ( 0 != m_pod_error)
		events->error(m_pod_error, getErrorString(m_pod_error));

	if ( 0 != m_si_error)
		events->error(m_si_error, getErrorString(m_si_error));

	RDK_LOG(RDK_LOG_TRACE1,"LOG.RDK.QAMSRC", "Exit %s():%d \n" , __FUNCTION__, __LINE__);
	return RMF_RESULT_SUCCESS;
}

const char* RMFQAMSrcImpl::getErrorString(RMFResult err)
{
    const char *pErrorStr = NULL;
    RDK_LOG(RDK_LOG_TRACE1,"LOG.RDK.QAMSRC", "Enter %s():%d \n" , __FUNCTION__, __LINE__);
    switch (err)
    {
        case RMF_QAMSRC_ERROR_TUNER_NOT_LOCKED:
            pErrorStr = "Temporarily unable to tune to channe";
            break;
        case RMF_QAMSRC_ERROR_PAT_NOT_AVAILABLE:
            pErrorStr = "PAT not available";
            break;
        case RMF_QAMSRC_ERROR_PMT_NOT_AVAILABLE:
            pErrorStr = "PMT not available";
            break;
        case RMF_QAMSRC_ERROR_UNRECOVERABLE_ERROR:
            pErrorStr = "System error";
            break;
        case RMF_QAMSRC_EVENT_CANNOT_DESCRAMBLE_ENTITLEMENT:
            pErrorStr = "Authorization failure";
            break;
        case RMF_QAMSRC_EVENT_CANNOT_DESCRAMBLE_RESOURCES:
            pErrorStr = "POD resource error";
            break;
        case RMF_QAMSRC_EVENT_MMI_PURCHASE_DIALOG:
            pErrorStr = "MMI purchase dialog";
            break;
        case RMF_QAMSRC_EVENT_MMI_TECHNICAL_DIALOG:
            pErrorStr = "MMI technical dialog";
            break;
        case RMF_QAMSRC_EVENT_POD_REMOVED:
            pErrorStr = "CARD removed";
            break;
        case RMF_QAMSRC_EVENT_POD_RESOURCE_LOST:
            pErrorStr = "CARD resource lost";
            break;
        case RMF_QAMSRC_ERROR_TUNE_FAILED:
            pErrorStr = "Tune failed";
            break;
		case RMF_QAMSRC_ERROR_TUNER_UNLOCK_TIMEOUT:
			pErrorStr = "Tuner unlock timeout";
			break;
        case RMF_QAMSRC_ERROR_PROGRAM_NUMBER_INVALID:
            pErrorStr = "Program number invalid";
            break;
        case RMF_QAMSRC_ERROR_PROGRAM_NUMBER_INVALID_ON_PAT_UPDATE:
            pErrorStr = "Program number invalid on PAT update";
            break;
        case RMF_QAMSRC_ERROR_SPTS_TIMEOUT:
            pErrorStr = "SPTS timeout";
            break;
        case RMF_QAMSRC_ERROR_NO_CA_SYSTEM_READY:
            pErrorStr = "CA_SYSTEM not ready for tune";
            break;

        default:
            pErrorStr = "Unknown Error";
            break;
    }
    RDK_LOG(RDK_LOG_TRACE1,"LOG.RDK.QAMSRC", "Exit %s():%d \n" , __FUNCTION__, __LINE__);
    return pErrorStr;
}

void RMFQAMSrcImpl::notifyError(RMFResult err, const char* msg)
{
	RDK_LOG(RDK_LOG_TRACE1,"LOG.RDK.QAMSRC", "Enter %s():%d \n" , __FUNCTION__, __LINE__);
	onError(err, getErrorString(err));
	RDK_LOG(RDK_LOG_TRACE1,"LOG.RDK.QAMSRC", "Exit %s():%d \n" , __FUNCTION__, __LINE__);
}

void RMFQAMSrcImpl::notifyCaStatus(const void* data_ptr, unsigned int data_size)
{
  RDK_LOG(RDK_LOG_TRACE1,"LOG.RDK.QAMSRC", "Enter %s():%d \n" , __FUNCTION__, __LINE__);
  onCaStatus(data_ptr, data_size);
  RDK_LOG(RDK_LOG_TRACE1,"LOG.RDK.QAMSRC", "Exit %s():%d \n" , __FUNCTION__, __LINE__);
}

void RMFQAMSrcImpl::notifyStatusChange(unsigned long status)
{
	RMFStreamingStatus rmfStatus;
	RDK_LOG(RDK_LOG_TRACE1,"LOG.RDK.QAMSRC", "Enter %s():%d \n" , __FUNCTION__, __LINE__);
	rmfStatus.m_status = status;
	onStatusChange( rmfStatus );
	RDK_LOG(RDK_LOG_TRACE1,"LOG.RDK.QAMSRC", "Exit %s():%d \n" , __FUNCTION__, __LINE__);
}

// Get the query string value.
/*Refer MediaUrl::get_querystr_val*/
RMFResult RMFQAMSrcImpl::get_querystr_val(string url, string q_str, string &value_str)
{
	size_t start_pos;
	size_t end_pos;

	//query strings are of format "query_str=value&"
	q_str += "=";

	start_pos = url.find(q_str);

	if (start_pos == string::npos)
	{
		value_str.clear();
		return RMF_RESULT_FAILURE;
	}

	start_pos += q_str.size();

	end_pos = url.find("&", start_pos);

	// If not found, this must be the last query item
	if (end_pos == string::npos)
	{
		// Point to end of string
		end_pos = url.size();
	}

	value_str = url.substr (start_pos, end_pos - start_pos);

	return RMF_RESULT_SUCCESS;
}

/* wrapper for getProgramInfo pm */
RMFResult RMFQAMSrcImpl::getSourceInfo(const char* uri, rmf_ProgramInfo_t* pProgramInfo, uint32_t* pDecimalSrcId)
{
	return getProgramInfo(uri, pProgramInfo, pDecimalSrcId);
}

/* Get required tuning parameters from uri	*/
RMFResult RMFQAMSrcImpl::getProgramInfo(const char* uri, rmf_ProgramInfo_t* pInfo, uint32_t* pDecimalSrcId)
{
	char *tmp;
	uint32_t decimalSrcId = 0;
	rmf_Error ret;
	tmp =  strcasestr ( (char* )uri, SOURCEID_PRIFIX_STRING );
	if ( NULL != tmp)
	{
		decimalSrcId = strtol( tmp+strlen(SOURCEID_PRIFIX_STRING), NULL, 16);

		RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "%s SrcId = %d\n", __FUNCTION__, decimalSrcId);	

#ifdef TEST_WITH_PODMGR
		ret = rmf_GetProgramInfo(decimalSrcId, pInfo);
#else
		ret = OOBSIManager::getInstance()->GetProgramInfo(decimalSrcId, pInfo);
#endif
		if( ret != RMF_SUCCESS)
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "GetProgramInfo failed\n");
			return RMF_QAMSRC_ERROR_CHANNELMAP_LOOKUP_FAILURE;
		}
	}
	else
	{
		tmp =  strcasestr ( (char* )uri, TUNEPARAMS_PRIFIX_STRING );
		if ( NULL != tmp)
		{
			string val_str;
			string uri_str = uri;
			std::transform(uri_str.begin(), uri_str.end(), uri_str.begin(), ::tolower);

			if ( RMF_RESULT_SUCCESS != get_querystr_val( uri_str,"frequency", val_str ))
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "open: invalid uri : %s donot contain \"frequency\"\n", uri );
				return RMF_RESULT_INVALID_ARGUMENT;
			}
			pInfo->carrier_frequency = atoi(val_str.c_str());
			if ( RMF_RESULT_SUCCESS != get_querystr_val( uri_str,"modulation", val_str ))
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "open: invalid uri : %s donot contain \"modulation\"\n", uri );
				return RMF_RESULT_INVALID_ARGUMENT;
			}
			pInfo->modulation_mode = (rmf_ModulationMode)atoi(val_str.c_str());
			if ( RMF_RESULT_SUCCESS != get_querystr_val( uri_str,"pgmno", val_str ))
			{
				RDK_LOG( RDK_LOG_WARN, "LOG.RDK.QAMSRC", "open: uri : %s donot contain \"pgmno\"\n", uri );
				pInfo->prog_num = 0;
			}
			else
			{
				pInfo->prog_num = atoi(val_str.c_str());
			}
			if ( RMF_RESULT_SUCCESS != get_querystr_val( uri_str, "symbol_rate", val_str ))
			{
				RDK_LOG( RDK_LOG_WARN, "LOG.RDK.QAMSRC", "open: uri : %s does not contain \"symbol_rate\"\n", uri );
				pInfo->symbol_rate = 0;
			}
			else
			{
				pInfo->symbol_rate = atoi(val_str.c_str());
			}
		}
		else
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "open: invalid uri : %s donot contain %s or %s \n", uri, SOURCEID_PRIFIX_STRING, TUNEPARAMS_PRIFIX_STRING);
			return RMF_RESULT_INVALID_ARGUMENT;
		}
	}
	if ( NULL != pDecimalSrcId )
	{
		*pDecimalSrcId = decimalSrcId;
	}
	return RMF_RESULT_SUCCESS;
}

void RMFQAMSrcImpl::getURI(std::string &uri)
{
	uri = saved_uri;
}

void RMFQAMSrcImpl::setTsIdForValidation(uint32_t tsid)
{
	m_validateTsId = true;
	m_tsIdForValidation = tsid;	
}

#ifdef CAMGR_PRESENT
void RMFQAMSrcImpl::ServiceListener(camgr_ctx_t ctx, camgr_svc_t svc, camgr_dtype_t data_type, void* data_ptr, uint32_t data_size, void* user_data)
{
  RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.QAMSRC", "++%s(ctx: %d [0x%X], svc: %d [0x%X], data_type: %d, data_ptr: %p, data_size: %d, user_data: %p)\n",
                  __FUNCTION__, ctx, ctx, svc, svc, data_type, data_ptr, data_size, user_data);
  if ((ctx == camgr_ctx) && (svc == camgr_svc))
  {
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "%s - [SERVICE NOTIFY]\n", __FUNCTION__);
    switch(data_type)
    {
      case CAMGR_DATA_SERVICE_STATUS:
      {
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "%s - [CAMGR_DATA_SERVICE_STATUS]\n", __FUNCTION__);
        notifyCaStatus(data_ptr, data_size);
        break;
      }
      case CAMGR_DATA_MISSING:
      {
        break;
      }
      default:
      {
        RDK_LOG(RDK_LOG_WARN, "LOG.RDK.QAMSRC", "Unknown data_type: %d\n", data_type);
        break;
      }
    }
  }
  RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.QAMSRC", "--%s()\n", __FUNCTION__);
}

static void camgr_ServiceListener(camgr_ctx_t ctx, camgr_svc_t svc, camgr_dtype_t data_type, void* data_ptr, uint32_t data_size, void* user_data)
{
  RMFQAMSrcImpl* this_ptr = (RMFQAMSrcImpl*) user_data;

  RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.QAMSRC", "++%s(ctx: %d [0x%X], svc: %d [0x%X], data_type: %d, data_ptr: %p, data_size: %d, user_data: %p)\n",
                  __FUNCTION__, ctx, ctx, svc, svc, data_type, data_ptr, data_size, user_data);
  if (this_ptr)
  {
    this_ptr->ServiceListener(ctx, svc, data_type, data_ptr, data_size, user_data);
  }
  RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.QAMSRC", "--%s()\n", __FUNCTION__);
}
#endif

/* Function to tune to the requested frequency */
RMFResult RMFQAMSrcImpl::open(const char* uri, char* mimetype)
{
	rmf_Error ret;
	RMFResult rmfResult;
	uint32_t tunerId;
	(void) mimetype;
	maxQueueSize = 0;


	RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.QAMSRC", "Enter impl  %s: uri: %s\n", __FUNCTION__, uri);

	rmfResult = getSourceInfo(uri, &pInfo, &decimalSrcId );
	if (RMF_RESULT_SUCCESS != rmfResult )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s:%d uri: %s getProgramInfo failed\n", __FUNCTION__, __LINE__, uri);
		return rmfResult;
	}
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "URI = %s prog_no: %d, frequency: %d, modulation: %d\n", uri, pInfo.prog_num, \
			pInfo.carrier_frequency, pInfo.modulation_mode);
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "URI = %s prog_no: %d, frequency: %d, modulation: %d, symbol_rate: %d\n", uri, pInfo.prog_num, \
			pInfo.carrier_frequency, pInfo.modulation_mode, pInfo.symbol_rate );

#ifdef USE_DVR
    /* TEE: Set device specific alloc and free functions on tee */
    GstElement* tee = getTeeElement();
    if(tee)
    {
    	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s::open: setting gst tee element"
			"properties for device mem alloc/free\n", __FUNCTION__);
    	g_object_set (G_OBJECT (tee), "devmem-alloc-callback", &dvrcryptoifc::allocateDeviceBufferAligned, NULL);
    	g_object_set (G_OBJECT (tee), "devmem-free-callback", &dvrcryptoifc::freeDeviceBuffer, NULL);
    }
#endif //USE_DVR

	
	QAMParams.program_no = pInfo.prog_num;
	QAMParams.frequency  = pInfo.carrier_frequency / 1000; 
	QAMParams.modulation   = pInfo.modulation_mode;
	QAMParams.symbol_rate  = pInfo.symbol_rate;

	patEventSent = FALSE;
	pmtEventSent = FALSE;
#ifndef DISABLE_INBAND_MGR
	if (NULL != pInbandSI)
	{
		delete pInbandSI;
	}
#endif
	g_object_get( src_element, "tunerid", &tunerId, NULL );
	RMF_ASSERT(numberOfTuners >= tunerId); //Safeguard against invalid array access.
	{
		g_object_get( src_element, "dmxhdl", 
				&ga_transport_path_info[tunerId].demux_handle, NULL);

#ifdef CAMGR_PRESENT
    g_object_get(src_element, "ltsid", &ga_transport_path_info[tunerId].ltsid, NULL);
    RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s:%d tunerId = %d, dmxhdl = 0x%x, ltsid = %d \n",
        __FUNCTION__, __LINE__, tunerId, ga_transport_path_info[tunerId].demux_handle, ga_transport_path_info[tunerId].ltsid );
    if ((camgr_ctx != CAMGR_INVALID_CONTEXT) && (camgr_svc == CAMGR_INVALID_SERVICE))
    {
      camgr_svc = CAMgr_ServiceStart(camgr_ctx, camgr_ServiceListener, this);
      RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "CAMgr_ServiceStart(camgr_ctx: %d) - camgr_svc: %d\n", camgr_ctx, camgr_svc);
    }
    else
    {
      RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "CAMgr_ServiceStart(camgr_ctx: %d)\n", camgr_ctx);
    }
#endif

#ifdef TEST_WITH_PODMGR
		g_object_get( src_element, 
				"ltsid", &ga_transport_path_info[tunerId].ltsid, 
				"sechdl", &ga_transport_path_info[tunerId].hSecContextFor3DES, 
				"tsdkeylocation", &ga_transport_path_info[tunerId].tsd_key_location, 
				NULL);
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s:%d tunerId = %d, dmxhdl = 0x%x, ltsid = %d \n", 
				__FUNCTION__, __LINE__, tunerId, ga_transport_path_info[tunerId].demux_handle, ga_transport_path_info[tunerId].ltsid );
#endif
	}

	saved_uri = uri; //The saved value shall be used to make error reports more descriptive.
	psiAcqStartTime = 0;
#ifndef  DISABLE_INBAND_MGR
	ret = rmf_osal_threadCreate( qamsrc_priv_start_simonitor, this, 
			RMF_OSAL_THREAD_PRIOR_DFLT, RMF_OSAL_THREAD_STACK_SIZE, 
			&siMonitorThreadId, "qamsrc_simonitor");
	if (RMF_SUCCESS != ret )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - rmf_osal_threadCreate failed.\n", __FUNCTION__);
		return RMF_QAMSRC_ERROR_UNRECOVERABLE_ERROR;
	}

	m_sourceInfo.dmxHdl = ga_transport_path_info[tunerId].demux_handle;
#endif
	m_sourceInfo.tunerId= tunerId;
	m_sourceInfo.freq = pInfo.carrier_frequency/1000;
	m_sourceInfo.modulation_mode = pInfo.modulation_mode;

	patEventSent = FALSE;
	pmtEventSent = FALSE;
	pat_available = FALSE;
#ifndef  DISABLE_INBAND_MGR
	m_pod_error = 0;
	m_qamtuner_error = 0;
	m_si_error = 0;

	pInbandSI = createInbandSiManager();
	if (NULL == pInbandSI)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "Cannot allocte InbandSI" );
		stopSIMonitorThread();
		return RMF_QAMSRC_ERROR_NOMEM;
	}

	/*Configure inband SI manager*/
	pInbandSI->TuneInitiated( pInfo.carrier_frequency, pInfo.modulation_mode);

	/* Registering for SI events */
	ret = pInbandSI->RegisterForPSIEvents(siEventQHandle, QAMParams.program_no);
	if(RMF_SUCCESS != ret )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "RegisterForPSIEvents Failed ret= %d\n", ret);
		stopSIMonitorThread();
		delete pInbandSI;
		pInbandSI = NULL;
		return RMF_QAMSRC_ERROR_DEMUX;
	}
	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "%s:%d Register For PSIEvents \n", __FUNCTION__, __LINE__);
#endif

	ret = rmf_osal_timeGetMillis( &tuneInitatedTime );
	if(RMF_SUCCESS != ret )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "rmf_osal_timeGetMillis Failed ret= %d\n", ret);
	}

	if ( pInfo.prog_num != 0 )
	{
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "RMFQAMSrc - Tune started : tunetuneInitatedTime = %llu. Source ID: %s \n", 
			tuneInitatedTime, uri );
	}
	else
	{
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "VOD AutoDiscovery - Tune started : tunetuneInitatedTime = %llu. Source ID: %s \n",
			tuneInitatedTime, uri);
	}
	state = QAM_STATE_OPEN;
	GstStructure *tuneinfo = gst_structure_new ("TuneInfo", 
			"frequency", G_TYPE_UINT, QAMParams.frequency,
			"modulation", G_TYPE_UINT, QAMParams.modulation,
			"symbol_rate", G_TYPE_UINT, QAMParams.symbol_rate,
			NULL);
	m_timekeeper.log_time(timekeeper::TUNE_START);
	g_object_set  (src_element,
			"tuneparams",tuneinfo,
			NULL);

	gst_structure_free (tuneinfo);

	g_object_set  (src_element,
			"pgmno",QAMParams.program_no,
			NULL);
#ifndef DISABLE_INBAND_MGR
	ga_diag_info[tunerId].frequency = QAMParams.frequency;
	ga_diag_info[tunerId].modulation = QAMParams.modulation;
#endif
	qamsrc_update_diag_info(src_element);
#ifndef DISABLE_INBAND_MGR
	ret = pInbandSI->RequestTsInfo();
	if(RMF_SUCCESS != ret )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "Could not request for TS Info ret= %d\n", ret);
		pInbandSI->UnRegisterForPSIEvents(siEventQHandle, QAMParams.program_no);
		stopSIMonitorThread();
		delete pInbandSI;
		pInbandSI = NULL;
		return RMF_QAMSRC_ERROR_DEMUX;
	}
#if defined(CAMGR_PRESENT) || defined(USE_EXTERNAL_CAS)
  ret = pInbandSI->RequestTsCatInfo();
  if(RMF_SUCCESS != ret )
  {
    RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "Could not request for TS CAT Info ret= %d\n", ret);
    pInbandSI->UnRegisterForPSIEvents(siEventQHandle, QAMParams.program_no);
    stopSIMonitorThread();
    delete pInbandSI;
    pInbandSI = NULL;
    return RMF_QAMSRC_ERROR_DEMUX;
  }
#endif
#endif
	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "%s:%d Requested TsInfo \n", __FUNCTION__, __LINE__);

	RDK_LOG(RDK_LOG_TRACE1,"LOG.RDK.QAMSRC", "Exit impl %s():%d \n" , __FUNCTION__, __LINE__);
	return RMF_RESULT_SUCCESS;
}/* End of RMFQAMSrcImpl::open */

void RMFQAMSrcImpl::notifySection(const void* data_ptr, unsigned int data_size)
{
  RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.QAMSRC", "Enter %s():%d \n" , __FUNCTION__, __LINE__);
  onSection(data_ptr, data_size);
  RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.QAMSRC", "Exit %s():%d \n" , __FUNCTION__, __LINE__);
}

RMFResult RMFQAMSrcImpl::enableTdtTot(bool enable)
{
  RMFResult ret = RMF_QAMSRC_ERROR_DEMUX;
  
  RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "TDT_TOT_PRESENT -> %s\n",__FUNCTION__);
  ret = pInbandSI->RequestTsTDTInfo(enable);
  if (RMF_SUCCESS != ret) {
    RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "Could not request for TS TDT Info ret= %d\n", ret);
  }
  return ret;
}

void* RMFQAMSrcImpl :: getLowLevelElement()
{
	GstElement * lowLevelElement =  gst_element_factory_make("qamtunersrc", "qsrc");
	if ( NULL == lowLevelElement )
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "gst_element_factory_make failed\n");
	}
	else
	{
		if ( TRUE == enableTTS )
		{
			g_object_set  (lowLevelElement,
					"tts",TRUE,
					NULL);
		}
	}
	return (void *)lowLevelElement;
}

/* Function to create qamtunersrc plugin*/
RMFResult RMFQAMSrcImpl::populateBin(GstElement* bin)
{
	static int count = 0;
	GstElement* last_el;
	GstPad *  srcpad;

	RDK_LOG(RDK_LOG_TRACE1,"LOG.RDK.QAMSRC", "Enter %s():%d \n" , __FUNCTION__, __LINE__);

	RMF_registerGstElementDbgModule((char*)"qamtunersrc", "LOG.RDK.GSTQAM");

	src_element = (GstElement*)getLowLevelElement();
	if (!src_element)
	{
		//  replace with error callback
		return RMF_RESULT_INTERNAL_ERROR;
	}
	if (cachingEnabled)
	{
		gst_base_src_set_live((GstBaseSrc*)src_element,  TRUE);
	}

	if (TRUE != gst_bin_add(GST_BIN(bin),src_element))
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "gst_bin_add failed\n");
		return RMF_RESULT_INTERNAL_ERROR;
	}
#if 0
	GstElement* queue = gst_element_factory_make("queue", NULL); 
	if (!queue)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC","Error instantiating queue.\n");
		return RMF_RESULT_INTERNAL_ERROR;
	}
	g_object_set(queue, "max-size-buffers", 16, "leaky", 2, NULL);
	if (TRUE !=  gst_bin_add(GST_BIN(bin), queue))
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "gst_bin_add failed\n");
		return RMF_RESULT_INTERNAL_ERROR;
	}

	if (TRUE !=  gst_element_link(src_element, queue))
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "gst_element_link failed\n");
		return RMF_RESULT_INTERNAL_ERROR;
	}

	last_el = queue;
#endif
	last_el = src_element;

	srcpad = gst_element_get_static_pad(last_el, "src");
	if (NULL == srcpad)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "gst_element_get_static_pad failed\n");
		return RMF_RESULT_INTERNAL_ERROR;
	}
	// Point the bin ghost pad to the last bin's element src pad.
	setGhostPadTarget(bin, srcpad, true);
	count++;

	RDK_LOG(RDK_LOG_TRACE1,"LOG.RDK.QAMSRC", "Exit %s():%d \n" , __FUNCTION__, __LINE__);
	return RMF_RESULT_SUCCESS;
}/* End of RMFQAMSrcImpl::populateBin */

// static
void RMFQAMSrcImpl::setGhostPadTarget(GstElement* bin, GstPad* target, bool unref_target)
{
	GstPad* bin_src_pad;

	RDK_LOG(RDK_LOG_TRACE1,"LOG.RDK.QAMSRC", "Enter %s():%d \n" , __FUNCTION__, __LINE__);
	bin_src_pad = gst_element_get_static_pad(bin, "src");
	if (NULL == bin_src_pad)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "gst_element_get_static_pad failed\n");
		return ;
	}
	if (gst_ghost_pad_set_target((GstGhostPad*) bin_src_pad, target) != true)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "gst_ghost_pad_set_target failed\n");
		return;
	}
	gst_object_unref(bin_src_pad);

	if (unref_target && target)
		gst_object_unref(target);

	RDK_LOG(RDK_LOG_TRACE1,"LOG.RDK.QAMSRC", "Exit %s():%d \n" , __FUNCTION__, __LINE__);
}/* End of RMFQAMSrcImpl::setGhostPadTarget */


/* Function to stop receving tune, SI and play events */
RMFResult RMFQAMSrcImpl::close()
{
	GArray * gpidarray;
	rmf_Error ret;

	RDK_LOG(RDK_LOG_DEBUG,"LOG.RDK.QAMSRC", "%s():%d Enter impl. \n" , __FUNCTION__, __LINE__);
	state = RMFQAMSrcImpl::QAM_STATE_IDLE;

#ifdef CAMGR_PRESENT
	g_object_set(src_element, "isAuthorized", FALSE, NULL);
	if ((camgr_ctx != CAMGR_INVALID_CONTEXT) && (camgr_svc != CAMGR_INVALID_SERVICE))
	{
		camgr_result_t result = CAMgr_ServiceStop(camgr_ctx, camgr_svc);
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "CAMgr_ServiceStop(camgr_ctx: %d, camgr_svc: %d) - result: %d\n", camgr_ctx, camgr_svc, result);
		camgr_svc = CAMGR_INVALID_SERVICE;
	}
	else
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "CAMgr_ServiceStop(camgr_ctx: %d, camgr_svc: %d)\n", camgr_ctx, camgr_svc);
	}
#endif

#ifdef TEST_WITH_PODMGR
	if ( 0 != sessionHandlePtr )
	{
		g_object_set(src_element, "isAuthorized", FALSE, NULL);
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "Calling rmf_podmgrStopDecrypt for %s\n", saved_uri.c_str());
		ret = rmf_podmgrStopDecrypt (sessionHandlePtr);
		if (RMF_SUCCESS != ret )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - rmf_podmgrStopDecrypt failed.\n", __FUNCTION__);
		}
		sessionHandlePtr = 0;
	}
#endif
	psiAcqStartTime = 0; //This will suppress all PSI timeouts.
#ifndef DISABLE_INBAND_MGR
	if (NULL != pInbandSI)
	{
		pInbandSI->UnRegisterForPSIEvents(siEventQHandle, QAMParams.program_no);
	}
#endif
	rmf_osal_eventqueue_clear(siEventQHandle);
#ifndef DISABLE_INBAND_MGR
	stopSIMonitorThread();
#endif
	if (NULL != pInbandSI)
	{
		clearINBProgramInfo();
#ifndef DISABLE_INBAND_MGR
		delete pInbandSI;
#endif
		pInbandSI = NULL;
		RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", " RMFQAMSrcImpl::%s:%d Deleted INB SI \n" , __FUNCTION__, __LINE__);
	}

	rmf_osal_condUnset(pat_cond);
	memset(&mediaInfo, 0, sizeof (rmf_MediaInfo));
	/*Clear the pids of gstqamtunersrc*/
	gpidarray = g_array_new( FALSE, FALSE, sizeof(guint16));
	if (NULL != gpidarray )
	{
		guint16 pmt_pid = 0;
		g_array_append_val (gpidarray, pmt_pid);
		g_object_set( src_element, "pidarray", gpidarray, NULL);
		g_array_free( gpidarray, TRUE );
	}
	
	rmf_osal_mutexAcquire(m_mutex);
	setErrorBits((QAMSRC_TUNER_UNSYNC_BITMASK|QAMSRC_PROGRAM_NUMBER_INVALID_BITMASK|QAMSRC_SPTS_OUTAGE_BITMASK), FALSE); //Reset error bits.
	m_is_tuner_locked = FALSE;
	if(0 < m_sync_loss_callback_id)
	{
		g_source_remove(m_sync_loss_callback_id);
		m_sync_loss_callback_id = 0;
		rmf_osal_mutexRelease(m_mutex);
		RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "%s: waiting to confirm sync-lost callback removal \n" , __FUNCTION__);
		rmf_osal_condWaitFor(m_sync_loss_timer_disconnected, 0);
		RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "%s: sync-lost callback removed \n" , __FUNCTION__);
	}
	else
	{
		rmf_osal_mutexRelease(m_mutex);
	}
	m_timekeeper.reset();	
	RDK_LOG(RDK_LOG_DEBUG,"LOG.RDK.QAMSRC", "%s():%d Exit impl. \n" , __FUNCTION__, __LINE__);

	return RMF_RESULT_SUCCESS;
}/* End of RMFQAMSrcImpl::close */


/* Function to playback AV */
RMFResult RMFQAMSrcImpl::play()
{
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.QAMSRC", "Enter impl %s():%d \n" , __FUNCTION__, __LINE__);
	rmf_osal_mutexAcquire( m_mutex);
	if (RMFQAMSrcImpl::QAM_STATE_PLAYING != state)
	{
#ifdef TEST_WITH_PODMGR
		if(0 == sessionHandlePtr) //POD session not present. Start it.
		{
			setupDecryption();
		}
#endif

#ifdef USE_GST1
        {
            GstPad *srcpad = gst_element_get_static_pad(src_element, "src");

            if (srcpad && GST_PAD_IS_EOS(srcpad)) {
                RDK_LOG(RDK_LOG_WARN,"LOG.RDK.QAMSRC", "%s: GstQamTunerSrc in EOS\n" , __FUNCTION__);
                flushPipeline();
            }

            if (srcpad)
                gst_object_unref(srcpad);
        }
#endif

		state = RMFQAMSrcImpl::QAM_STATE_PLAYING;
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "RMFQAMSrcImpl::%s():%d : Play\n" , __FUNCTION__, __LINE__);
		RMFMediaSourcePrivate::play();

#ifdef TEST_WITH_PODMGR
		notifySIMonitorThread(RMF_SI_EVENT_PIPELINE_PLAYING);
#endif
	}
	rmf_osal_mutexRelease( m_mutex);
	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "Exit impl %s():%d \n" , __FUNCTION__, __LINE__);
	return RMF_RESULT_SUCCESS;
}


/* Function to stop AV playback temporarily  */
RMFResult RMFQAMSrcImpl::pause()
{
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.QAMSRC", "Enter impl %s():%d \n" , __FUNCTION__, __LINE__);

	rmf_osal_mutexAcquire( m_mutex);
	if(1 < getSinkCount())
	{
		RDK_LOG(RDK_LOG_INFO, 
			"LOG.RDK.QAMSRC", "RMFQAMSrcImpl::%s(): won't pause pipeline. Multiple users detected.\n" , __FUNCTION__);
		rmf_osal_mutexRelease( m_mutex);
		return RMF_RESULT_SUCCESS;
	}
#ifdef TEST_WITH_PODMGR
	notifySIMonitorThread(RMF_SI_EVENT_PIPELINE_PAUSED);
#endif
	RMFMediaSourcePrivate::pause();
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "RMFQAMSrcImpl::%s():%d : Pause\n" , __FUNCTION__, __LINE__);
	state = RMFQAMSrcImpl::QAM_STATE_PAUSED;
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "Max queue size reached this streaming session of %s is %d.\n" , 
		saved_uri.c_str(), maxQueueSize + 1);
	/* The filters have been turned off. SPTS timeout is no longer a valid condition. Clear the flag if it
	 * was set.*/
	if(RMF_QAMSRC_ERROR_SPTS_TIMEOUT == m_qamtuner_error)
	{
		m_qamtuner_error = 0;
	}
	setErrorBits(QAMSRC_SPTS_OUTAGE_BITMASK, FALSE);
#ifdef TEST_WITH_PODMGR
	if ((0 != sessionHandlePtr) && (TRUE == stopCAWhenPaused()))
	{
		rmf_Error ret;
		g_object_set(src_element, "isAuthorized", FALSE, NULL);
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "Calling rmf_podmgrStopDecrypt for %s\n", saved_uri.c_str());
		ret = rmf_podmgrStopDecrypt (sessionHandlePtr);
		if (RMF_SUCCESS != ret )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - rmf_podmgrStopDecrypt failed.\n", __FUNCTION__);
		}
		sessionHandlePtr = 0;
		m_pod_error = 0;
	}
#endif

#ifdef CAMGR_PRESENT
	g_object_set(src_element, "isAuthorized", FALSE, NULL);
#endif

	rmf_osal_mutexRelease( m_mutex);

	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.QAMSRC", "Exit impl %s():%d \n" , __FUNCTION__, __LINE__);
	return RMF_RESULT_SUCCESS;
}/* End of RMFQAMSrcImpl::pause */


RMFResult RMFQAMSrcImpl::stop()
{
	RMFResult ret;
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.QAMSRC", "Enter impl %s():%d \n" , __FUNCTION__, __LINE__);

	rmf_osal_mutexAcquire( m_mutex);
	ret = pause();

	if((RMF_RESULT_SUCCESS == ret) && (RMFQAMSrcImpl::QAM_STATE_PAUSED == state))
	{
		//if state=PAUSED, it's safe to stop the pipeline.
		ret = RMFMediaSourcePrivate::stop();
	}
	rmf_osal_mutexRelease( m_mutex);

	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.QAMSRC", "Exit impl %s():%d \n" , __FUNCTION__, __LINE__);
	return ret;
}/* End of RMFQAMSrcImpl::pause */


RMFResult RMFQAMSrcImpl::getSpeed(float& speed)
{
	RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.QAMSRC", "Enter %s():%d \n" , __FUNCTION__, __LINE__);
	speed= 1.0f;
	return RMF_RESULT_SUCCESS;
}

/* How it works: 
 * Compares the PIDs in newMediaInfo against mediaInfo. If the PIDs are identical,
 * returns PMT_COMPARISON_NEW_TABLE_IS_A_FULL_MATCH. If the PIDs are not identical, but
 * all PIDs in newMediaInfo are present in mediaInfo, then we return PMT_COMPARISON_NEW_TABLE_IS_A_SUBSET.
 * If there are PIDs in newMediaInfo that aren't present in mediaInfo, we return PMT_COMPARISON_NO_MATCH. */
unsigned int RMFQAMSrcImpl::compareMediaInfo(rmf_MediaInfo *newMediaInfo)
{
	int iPid=0;
	int jPid=0;
	unsigned int ret = PMT_COMPARISON_NEW_TABLE_IS_A_FULL_MATCH;

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "Enter %s():%d \n" , __FUNCTION__, __LINE__);
	for(iPid=0;iPid<newMediaInfo->numMediaPids; iPid++)
	{
		for(jPid=0;jPid<mediaInfo.numMediaPids;jPid++)
		{
			if(newMediaInfo->pPidList[iPid].pid == mediaInfo.pPidList[jPid].pid)
				break;
		}
		/*will reach here, if new set of pids are received and will return true in such cases */
		if(jPid == mediaInfo.numMediaPids)
			return PMT_COMPARISON_NO_MATCH;
	}

	if(newMediaInfo->numMediaPids != mediaInfo.numMediaPids)
	{
		ret = PMT_COMPARISON_NEW_TABLE_IS_A_SUBSET;
	}
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "Exit %s():%d \n" , __FUNCTION__, ret);
	return ret;
}

void RMFQAMSrcImpl::setFilter(uint16_t pid, char* filterParam, uint32_t *pFilterId)
{
	RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.QAMSRC", "%s - setFilter in RMFQAMSrcImpl\n", __FUNCTION__);
#ifndef DISABLE_INBAND_MGR
	RMFResult ret = pInbandSI->setFilter(pid, filterParam, pFilterId);
	if( ret != RMF_INBSI_SUCCESS) {
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - Create filter failed ret = 0x%x\n", __FUNCTION__, ret);
	}
#else
	*pFilterId = 0;
#endif
}

void RMFQAMSrcImpl::releaseFilter(uint32_t filterId)
{
	RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.QAMSRC", "%s - releaseFilter in RMFQAMSrcImpl\n", __FUNCTION__);
#ifndef DISABLE_INBAND_MGR
	RMFResult ret = pInbandSI->releaseFilter(filterId);
	if( ret != RMF_INBSI_SUCCESS) {
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - Destroy/Release filter failed ret = 0x%x\n", __FUNCTION__, ret);
	}
#endif
}

void RMFQAMSrcImpl::resumeFilter(uint32_t filterId)
{
        RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.QAMSRC", "%s - resumeFilter in RMFQAMSrcImpl\n", __FUNCTION__);
#ifndef DISABLE_INBAND_MGR
        if(!pInbandSI)
        {
            RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.QAMSRC", "%s - pInbandSI NULL in RMFQAMSrcImpl\n", __FUNCTION__);
	    return;
        }
        RMFResult ret = pInbandSI->resumeFilter(filterId);
        if( ret != RMF_INBSI_SUCCESS) {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - resume filter failed ret = 0x%x\n", __FUNCTION__, ret);
        }
#endif
}

void RMFQAMSrcImpl::pauseFilter(uint32_t filterId)
{
        RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.QAMSRC", "%s - pauseFilter in RMFQAMSrcImpl\n", __FUNCTION__);
#ifndef DISABLE_INBAND_MGR
        if(!pInbandSI)
        {
            RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.QAMSRC", "%s - pInbandSI NULL in RMFQAMSrcImpl\n", __FUNCTION__);
            return;
        }
        RMFResult ret = pInbandSI->pauseFilter(filterId);
        if( ret != RMF_INBSI_SUCCESS) {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - pause filter failed ret = 0x%x\n", __FUNCTION__, ret);
        }
#endif
}

void RMFQAMSrcImpl::getPATBuffer(std::vector<uint8_t>& buf, uint32_t* length) {
	*length = 0;
#ifdef QAMSRC_PATBUFFER_PROPERTY
	RMFResult ret = pInbandSI->GetPATBuffer(NULL, length);
	if( ret == RMF_INBSI_SUCCESS) {
		buf.resize(*length);
		if(buf.size() != *length)
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - GetPMTBuffer failed - Not able to resize the buffer.\n", __FUNCTION__);
			*length = 0;
			buf.clear();
			return;
		}
		ret = pInbandSI->GetPATBuffer(buf.data(), length);
		if( ret != RMF_INBSI_SUCCESS)
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - GetPATBuffer failed.\n", __FUNCTION__);
		}
	}
	else {
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - GetPATBuffer failed to get length. ret = 0x%x\n", __FUNCTION__, ret);
	}
#endif
}

void RMFQAMSrcImpl::getPMTBuffer(std::vector<uint8_t>& buf, uint32_t* length) {
	*length = 0;
#ifdef QAMSRC_PMTBUFFER_PROPERTY
	RMFResult ret = pInbandSI->GetPMTBuffer(NULL, length);
	if( ret == RMF_INBSI_SUCCESS) {
		buf.resize(*length);
		if(buf.size() != *length)
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - GetPMTBuffer failed - Not able to resize the buffer.\n", __FUNCTION__);
			*length = 0;
			buf.clear();
			return;
		}
		ret = pInbandSI->GetPMTBuffer(buf.data(), length);
		if( ret != RMF_INBSI_SUCCESS)
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - GetPMTBuffer failed.\n", __FUNCTION__);
		}
	}
	else {
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - GetPMTBuffer failed.\n", __FUNCTION__);
	}
#endif
}

void RMFQAMSrcImpl::getCATBuffer(std::vector<uint8_t>& buf, uint32_t* length) {
	*length = 0;
#ifdef QAMSRC_CATBUFFER_PROPERTY
	RMFResult ret = pInbandSI->GetCATBuffer(NULL, length);
	if( ret == RMF_INBSI_SUCCESS) {
		buf.resize(*length);
		if(buf.size() != *length)
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - GetPMTBuffer failed - Not able to resize the buffer.\n", __FUNCTION__);
			*length = 0;
			buf.clear();
			return;
		}
		ret = pInbandSI->GetCATBuffer(buf.data(), length);
		if( ret != RMF_INBSI_SUCCESS)
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - GetCATBuffer failed.\n", __FUNCTION__);
		}
	}
	else {
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - GetCATBuffer failed.\n", __FUNCTION__);
	}
#endif
}

void RMFQAMSrcImpl::getSectionData(uint32_t *filterId, std::vector<uint8_t>& buf, uint32_t* length)
{
	*filterId = 0;
	*length = 0;
	uint32_t fId = 0;
	uint8_t *data = pInbandSI->get_Section(length, fId);
	if (data)
	{
		buf.resize(*length);
		if(buf.size() != *length)
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - getSectionData failed - Not able to resize the buffer.\n", __FUNCTION__);
			*length = 0;
			buf.clear();
			free(data);
			return;
		}
		buf.assign(data, data+*length);
		*filterId = fId;
		free(data);
	}
}

void RMFQAMSrcImpl::updateOutputPMT()
{
	rmf_Error ret;
	GByteArray *pmtbuf = g_byte_array_new ();
	if (NULL != pmtbuf )
	{
		uint8_t data[PMT_MAX_SIZE];
		uint32_t length = PMT_MAX_SIZE;
		ret = pInbandSI->GetPMTBuffer(data, &length);
		if( ret != RMF_INBSI_SUCCESS)
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - GetPMTBuffer failed.\n", __FUNCTION__);
		}
		else
		{
			g_byte_array_append (pmtbuf, data, length);
			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "Setting PMT buffer %p length= %d\n", pmtbuf, length);
			g_object_set  (src_element,
					"pmtbuffer", pmtbuf, NULL);
		}
	}
	else
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - gst_buffer_new_and_alloc failed.\n", __FUNCTION__);
	}
}


#ifdef CAMGR_PRESENT

typedef struct
{
	uint8_t *data_ptr;
	uint32_t data_size;
	bool     update;
} session_prv_t;

static void rmfqamsrc_CatCallback(uint32_t section_size, uint8_t *section_data, void* user_data)
{
	session_prv_t* private_ptr = (session_prv_t*) user_data;
	rmf_Error ret;

	if (private_ptr)
	{
		private_ptr->update = true;
		if (section_size && section_data)
		{
			if ((section_size == private_ptr->data_size) && (0 == memcmp(section_data, private_ptr->data_ptr, section_size)))
			{
				RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.QAMSRC", "The same CAT\n");
				private_ptr->update = false;
			}
		}
		if (private_ptr->update)
		{
			if (private_ptr->data_ptr)
			{
				rmf_osal_memFreeP(RMF_OSAL_MEM_MEDIA, private_ptr->data_ptr);
				private_ptr->data_ptr = NULL;
				private_ptr->data_size = 0;
			}
			if (section_size && section_data)
			{
				ret = rmf_osal_memAllocP(RMF_OSAL_MEM_MEDIA, section_size, (void **) &private_ptr->data_ptr);
				if (ret == RMF_SUCCESS)
				{
					memcpy(private_ptr->data_ptr, section_data, section_size);
					private_ptr->data_size = section_size;
				}
				else
				{
					RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "rmf_osal_memAllocP failed while allocating new CAT table.\n");
				}
			}
			else
			{
				RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "Wrong CAT table - ptr: %p, size: %d\n", section_data, section_size);
			}
		}
	}
}

void RMFQAMSrcImpl::processCatInfo()
{
  session_prv_t user_data = {catTable, catTableSize, false};
  rmf_Error ret;

  RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "++%s()\n", __FUNCTION__);
  rmf_osal_mutexAcquire(m_mutex);
  ret = pInbandSI->GetCatTable(rmfqamsrc_CatCallback, &user_data);
  if (user_data.update)
  {
    unsigned char LTSID;

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "CAT updated - size: %d\n", user_data.data_size);
    if (getLTSID(LTSID) == RMF_RESULT_SUCCESS)
    {
      uint32_t tunerId;

      if (src_element)
      {
        g_object_get( src_element, "tunerid", &tunerId, NULL );
        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "tunerId: %d\n", tunerId);

        RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "CAT send to the CAK with LTSID: %d, tunerid: %d\n", LTSID, tunerId);
        if (camgr_ctx != CAMGR_INVALID_CONTEXT)
        {
          uint32_t        tsId = LTSID;
          uint32_t        dmxId = ga_transport_path_info[tunerId].demux_handle;
          camgr_ntype_t   type = CAMGR_NOTIFY_CAT_NEW;
          void*           data_ptr = (void*) user_data.data_ptr;
          uint32_t        data_size = user_data.data_size;
          camgr_result_t  result;

          if ((data_ptr == NULL) || (data_size == 0))
          {
            type = CAMGR_NOTIFY_CAT_INVALID;
          }
          result = CAMgr_ContextNotify(camgr_ctx, tsId, dmxId, type, data_ptr, data_size);
          RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "CAMgr_ContextNotify(camgr_ctx: %d, tsId: %d, dmxId: %d, type: %d, data_ptr: %p, data_size: %d) - result: %d\n",
                       camgr_ctx, tsId, dmxId, type, data_ptr, data_size, result);
        }
        else
        {
          RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "Invalid CAMGR context - camgr_ctx: %d, camgr_svc: %d)\n", camgr_ctx, camgr_svc);
        }
      }
    }
    else
    {
      RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "CAT info received but unable to get tuner ID\n");
    }
  }
  else
  {
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.QAMSRC", "The same CAT\n");
  }
  rmf_osal_mutexRelease(m_mutex);
  RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "--%s()\n", __FUNCTION__);
}
#endif


void RMFQAMSrcImpl::processProgramInfo()
{
	rmf_Error ret;
	rmf_MediaInfo newMediaInfo;
	int i;
	GArray * gpidarray;
	unsigned int pmtChangeType = PMT_COMPARISON_NEW_TABLE_IS_A_FULL_MATCH;

	/*This can be removed once memset is corrected in inband si manager.*/
	memset(&newMediaInfo, 0, sizeof (rmf_MediaInfo));
	rmf_osal_mutexAcquire( m_mutex);
#ifndef DISABLE_INBAND_MGR
	ret = pInbandSI->GetMediaInfo(QAMParams.program_no, &newMediaInfo);
	if( ret == RMF_INBSI_SUCCESS)
	{
		pmtChangeType = compareMediaInfo(&newMediaInfo);
		if(PMT_HANDLING_UNOPTIMIZED == getPMTOptimizationPolicy())
		{
			if(PMT_COMPARISON_NEW_TABLE_IS_A_FULL_MATCH == pmtChangeType)
			{
				/* Release mutex and return. Nothing to do here.*/
				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "Media pids available in this PMT are already set\n");
				rmf_osal_mutexRelease( m_mutex);
				return;
			}
			else
			{
				/* Redo the CA, filters.*/
			}
		}
		else
		{
			/* We have some optimization policy in force for handling PMT updates.*/

			switch(pmtChangeType)
			{
				case PMT_COMPARISON_NO_MATCH:
					m_pmt_optimization_active = FALSE; 
					/* Clearing the above flag as any optimization currently in force will be annulled 
					 * when we redo CA, PMT and filters further down this control flow.
					 *
					 * Break to redo CA, update output PMT, filters.*/
					break;
				case PMT_COMPARISON_NEW_TABLE_IS_A_SUBSET:
					if(PMT_OPTIMIZE_CA_OPS == getPMTOptimizationPolicy())
					{
						/* Under this policy, we skip CA and filter changes, but update the output PMT. Do note that mediaInfo is unchanged. */
						RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "Media pids available in this program are a subset of previous PMT. Updating output PMT.\n");
						updateOutputPMT();
						m_pmt_optimization_active = TRUE; 
						/* This flag is necessary so that we know to re-configure the output PMT, should a 
						   subsequent PMT update bring back the original PID list. */
					}
					else
					{
						/* Optmization policy is PMT_OPTIMIZE_CA_AND_PMT_OPS. This means we completely bury the PMT update.
						 * No changes to CA, output PMT or filters. */
						RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "Media pids available in this program are a subset of previous PMT\n");
					}
					rmf_osal_mutexRelease( m_mutex);
					return;
					break;
				case PMT_COMPARISON_NEW_TABLE_IS_A_FULL_MATCH:
					/* Do nothing. Release mutex and return. */
					if(TRUE == m_pmt_optimization_active)
					{
						/* We were previously sending a PMT with a subset of mediaInfo PIDs. Looks like we're back to the full set of PIDs
						 * now. Update the output PMT so that the player knows this as well.*/
						RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "Updating PMT to reflect a larger list of media PIDs\n");
						updateOutputPMT();
						m_pmt_optimization_active = FALSE;
					}
					else
					{
						RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "Media pids available in this PMT are already set\n");
					}
					rmf_osal_mutexRelease( m_mutex);
					return;
					break;
			}
		}
		rmf_osal_memcpy(&mediaInfo, &newMediaInfo, sizeof(rmf_MediaInfo), sizeof(mediaInfo), sizeof(newMediaInfo));

		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "GetMediaInfo Success. Number of Media Pids= %d &mediaInfo is %p pmt pid = %d\n", 
				mediaInfo.numMediaPids, &mediaInfo, mediaInfo.pmt_pid);
		pmtEventSent = TRUE;
	}
	else if( ret == RMF_INBSI_PMT_NOT_AVAILABLE)
	{
		RDK_LOG( RDK_LOG_WARN, "LOG.RDK.QAMSRC", "PMT not available yet\n");
		rmf_osal_mutexRelease( m_mutex);
		return;
	}
	else
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "InbandSI->GetMediaInfo failed\n");
		rmf_osal_mutexRelease( m_mutex);
		return;
	}
	if( 0 == newMediaInfo.numMediaPids)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "No Media pids available in this Program\n"); 
		rmf_osal_mutexRelease( m_mutex);
		return ;
	}
#endif
	//Avoid starting CA session in paused state
	if(RMFQAMSrcImpl::QAM_STATE_PAUSED != state)
	{
		setupDecryption();
	}
	{
#ifndef QAMSRC_GENERATE_PMT

#ifdef QAMSRC_PMTBUFFER_PROPERTY
		updateOutputPMT();
#endif
		gpidarray = g_array_new( FALSE, FALSE, sizeof(guint16));
		if (NULL != gpidarray )
		{
			g_array_append_val (gpidarray, newMediaInfo.pmt_pid);
			g_array_append_val (gpidarray, newMediaInfo.pcrPid);

			for (i = 0; i < newMediaInfo.numMediaPids; i++)
			{
				g_array_append_val (gpidarray, newMediaInfo.pPidList[i].pid);
			}
			g_object_set  (src_element,
					"pidarray",gpidarray,
					NULL);
			g_array_free (gpidarray, TRUE );
		}
#else
		gpidarray = g_array_new( FALSE, FALSE, sizeof(GstStructure *));
		if (NULL != gpidarray )
		{
			GstStructure *ppidInfo = gst_structure_new ("PidInfo", 
					GSTQAMTUNERSRC_PID_FIELDNAME, G_TYPE_UINT, newMediaInfo.pmt_pid,
					GSTQAMTUNERSRC_STREAMTYPE_FIELDNAME, G_TYPE_UINT, GSTQAMTUNERSRC_PIDTYPE_PMT,
					NULL);
			g_array_append_val (gpidarray, ppidInfo);
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[PIDDESC: ] appended %p \n", ppidInfo);
			ppidInfo = gst_structure_new ("PidInfo", 
					GSTQAMTUNERSRC_PID_FIELDNAME, G_TYPE_UINT, newMediaInfo.pcrPid,
					GSTQAMTUNERSRC_STREAMTYPE_FIELDNAME, G_TYPE_UINT, GSTQAMTUNERSRC_PIDTYPE_PCR,
					NULL);
			g_array_append_val (gpidarray, ppidInfo);
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[PIDDESC: ] appended %p \n", ppidInfo);

			for (i = 0; i < newMediaInfo.numMediaPids; i++)
			{
				ppidInfo = gst_structure_new ("PidInfo", 
						GSTQAMTUNERSRC_PID_FIELDNAME, G_TYPE_UINT, newMediaInfo.pPidList[i].pid,
						GSTQAMTUNERSRC_STREAMTYPE_FIELDNAME, G_TYPE_UINT, newMediaInfo.pPidList[i].pidType,
						NULL);
				g_array_append_val (gpidarray, ppidInfo);
				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "[PIDDESC: ] appended %p \n", ppidInfo);
			}
			g_object_set  (src_element,
					"piddesc",gpidarray,
					NULL);

			/*Clean up*/
			while  (  gpidarray->len > 0)
			{
				ppidInfo =  g_array_index(gpidarray, GstStructure*, 0 );
				if (NULL != ppidInfo )
				{
					gst_structure_free (ppidInfo);
				}
				gpidarray = g_array_remove_index_fast( gpidarray,  0);
			}
			g_array_free (gpidarray, TRUE );
		}
#endif
	}
	rmf_osal_mutexRelease( m_mutex);

}

void RMFQAMSrcImpl::setupDecryption()
{
	rmf_Error ret;
	m_timekeeper.log_time(timekeeper::CA_START);
	rmf_osal_mutexAcquire( m_mutex);
	if(0 == mediaInfo.numMediaPids)
	{
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s: cannot start CA session for %s. No media PIDs available.\n",
		__FUNCTION__, saved_uri.c_str());
		rmf_osal_mutexRelease( m_mutex);
		return;
	}

#ifdef CAMGR_PRESENT
	if ((camgr_ctx != CAMGR_INVALID_CONTEXT) && (camgr_svc != CAMGR_INVALID_SERVICE))
	{
		uint32_t        tunerId;
		uint32_t        tsid = 0;
		uint32_t        dmxid = 0;
		uint32_t        playid = 0;
		uint32_t        srcid = camgr_ctx;
		uint8_t         pmt_tab[PMT_MAX_SIZE];
		uint32_t        pmt_size = PMT_MAX_SIZE;
		uint16_t*       pids_ptr = NULL;
		uint32_t        pids_size = (mediaInfo.numMediaPids * sizeof(uint16_t));
		camgr_result_t  result;

		g_object_get(src_element, "tunerid", &tunerId, NULL);
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "tunerId: %d\n", tunerId);

		tsid = ga_transport_path_info[tunerId].ltsid;
		dmxid = ga_transport_path_info[tunerId].demux_handle;

		result = CAMgr_ServiceSetParam(camgr_ctx, camgr_svc, CAMGR_PARAM_TSID, &tsid, sizeof(tsid));
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "CAMgr_ServiceSetParam(camgr_ctx: %d, camgr_svc: %d, param_type: %d, param_ptr: %p, param_size: %d) - result: %d\n",
			camgr_ctx, camgr_svc, CAMGR_PARAM_TSID, tsid, sizeof(tsid), result);

		result = CAMgr_ServiceSetParam(camgr_ctx, camgr_svc, CAMGR_PARAM_DMXID, &dmxid, sizeof(dmxid));
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "CAMgr_ServiceSetParam(camgr_ctx: %d, camgr_svc: %d, param_type: %d, param_ptr: %p, param_size: %d) - result: %d\n",
			camgr_ctx, camgr_svc, CAMGR_PARAM_DMXID, dmxid, sizeof(dmxid), result);

		result = CAMgr_ServiceSetParam(camgr_ctx, camgr_svc, CAMGR_PARAM_PLAYID, &playid, sizeof(playid));
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "CAMgr_ServiceSetParam(camgr_ctx: %d, camgr_svc: %d, param_type: %d, param_ptr: %p, param_size: %d) - result: %d\n",
			camgr_ctx, camgr_svc, CAMGR_PARAM_PLAYID, playid, sizeof(playid), result);

		result = CAMgr_ServiceSetParam(camgr_ctx, camgr_svc, CAMGR_PARAM_SRCID, &srcid, sizeof(srcid));
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "CAMgr_ServiceSetParam(camgr_ctx: %d, camgr_svc: %d, param_type: %d, param_ptr: %p, param_size: %d) - result: %d\n",
			camgr_ctx, camgr_svc, CAMGR_PARAM_SRCID, srcid, sizeof(srcid), result);

		/* gets list of pids */
		pids_ptr = (uint16_t*) malloc(pids_size);
		if (pids_ptr != NULL)
		{
			int pids_idx = 0;
			int pids_filter = 0;

			for (int pids_cnt = 0; pids_cnt < mediaInfo.numMediaPids; pids_cnt++)
			{
				if (pids_filter) // only A/V pids
				{
					switch (mediaInfo.pPidList[pids_cnt].pidType)
					{
					case RMF_SI_ELEM_MPEG_1_VIDEO:
					case RMF_SI_ELEM_MPEG_2_VIDEO:
					case RMF_SI_ELEM_ISO_14496_VISUAL:
					case RMF_SI_ELEM_AVC_VIDEO:
					case RMF_SI_ELEM_VIDEO_DCII:
					case RMF_SI_ELEM_MPEG_1_AUDIO:
					case RMF_SI_ELEM_MPEG_2_AUDIO:
					case RMF_SI_ELEM_AAC_ADTS_AUDIO:
					case RMF_SI_ELEM_AAC_AUDIO_LATM:
					case RMF_SI_ELEM_ATSC_AUDIO:
					case RMF_SI_ELEM_ENHANCED_ATSC_AUDIO:
					{
						pids_ptr[pids_idx++] = mediaInfo.pPidList[pids_cnt].pid;
						break;
					}
					default:
					{
					    break;
					}
					}
				}
				else
				{
					pids_ptr[pids_idx++] = mediaInfo.pPidList[pids_cnt].pid;
				}
			}

			/* sets size of pids copied to the pids_ptr */
			pids_size = (pids_idx * sizeof(uint16_t));

			result = CAMgr_ServiceSetParam(camgr_ctx, camgr_svc, CAMGR_PARAM_PIDS, pids_ptr, pids_size);
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "CAMgr_ServiceSetParam(camgr_ctx: %d, camgr_svc: %d, param_type: %d, param_ptr: %p, param_size: %d) - result: %d\n",
				camgr_ctx, camgr_svc, CAMGR_PARAM_PIDS, pids_ptr, pids_size, result);
			free(pids_ptr);
		}

		/* gets whole PMT */
		ret = pInbandSI->GetPMTBuffer(pmt_tab, &pmt_size);
		if (ret == 0)
		{
			result = CAMgr_ServiceSetParam(camgr_ctx, camgr_svc, CAMGR_PARAM_PMT, pmt_tab, pmt_size);
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "CAMgr_ServiceSetParam(camgr_ctx: %d, camgr_svc: %d, param_type: %d, param_ptr: %p, param_size: %d) - result: %d\n",
				camgr_ctx, camgr_svc, CAMGR_PARAM_PMT, pmt_tab, pmt_size, result);
		}

		/* Apply all changes */
		result = CAMgr_ServiceConnect(camgr_ctx, camgr_svc);
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "CAMgr_ServiceConnect(camgr_ctx: %d, camgr_svc: %d) - result: %d\n", camgr_ctx, camgr_svc, result);
	}
	else
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "CAMgr_ServiceConnect(camgr_ctx: %d, camgr_svc: %d)\n", camgr_ctx, camgr_svc);
	}
#endif

#ifdef TEST_WITH_PODMGR
	int tunerId;
	rmf_PodmgrDecryptRequestParams decryptRequestPtr;

	if ( 0 != sessionHandlePtr )
	{
		g_object_set(src_element, "isAuthorized", FALSE, NULL);
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "Calling rmf_podmgrStopDecrypt for %s\n", saved_uri.c_str());
		ret = rmf_podmgrStopDecrypt (sessionHandlePtr);
		if (RMF_SUCCESS != ret )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - rmf_podmgrStopDecrypt failed.  Source ID: %s\n",
				__FUNCTION__, saved_uri.c_str());
		}
		sessionHandlePtr =	0;
		m_pod_error = 0;
	}

	g_object_get( src_element, "tunerid", &tunerId, NULL );

#ifndef ENABLE_INB_SI_CACHE
	ret = pInbandSI->GetDescriptors(&decryptRequestPtr.pESList, 
			&decryptRequestPtr.pMpeg2DescList);
	if (RMF_SUCCESS != ret )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - pInbandSI->GetDescriptors failed.\n", __FUNCTION__);
	}
	
	if( decimalSrcId )
	{
		decryptRequestPtr.srcId = decimalSrcId;
	}
	else
	{
		decryptRequestPtr.srcId = 0xffff;
	}
	
	decryptRequestPtr.prgNo = QAMParams.program_no;
#else
	decryptRequestPtr.handleType =	RMF_PODMGR_SERVICE_DETAILS_HANDLE ;
	decryptRequestPtr.handle = pInfo.service_handle;
	decryptRequestPtr.isRecording = 0;
#endif
	decryptRequestPtr.tunerId = tunerId;
	decryptRequestPtr.ltsId= ga_transport_path_info[tunerId].ltsid;
	decryptRequestPtr.mmiEnable = FALSE;
	decryptRequestPtr.priority = 0;
	decryptRequestPtr.pids = (rmf_MediaPID*)mediaInfo.pPidList;
	decryptRequestPtr.numPids = mediaInfo.numMediaPids;

	is_clear_content = FALSE;
	decryptRequestPtr.bClear = FALSE;
	decryptRequestPtr.qamContext = m_context;

	m_cci = RMF_QAMSRC_CCI_STATUS_NONE;
	m_isAuthorized = FALSE;

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "Calling rmf_podmgrStartDecrypt for %s handle = %d\n", saved_uri.c_str(), m_context);

	ret =rmf_podmgrStartDecrypt(&decryptRequestPtr, g_eventq, 
			&sessionHandlePtr);
	if (RMF_SUCCESS != ret )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - rmf_podmgrStartDecrypt failed. Source ID: %s\n", __FUNCTION__
			,saved_uri.c_str());
		notifyError(RMF_QAMSRC_ERROR_NO_CA_SYSTEM_READY, NULL);
	}
	else
	{
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s - rmf_podmgrStartDecrypt Success. Source ID: %s\n",
			__FUNCTION__, saved_uri.c_str());

		if(TRUE == decryptRequestPtr.bClear)
		{
			is_clear_content = TRUE;
			
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s: This is clear content. Source ID: %s.\n",
						__FUNCTION__, saved_uri.c_str());
			m_isAuthorized = TRUE;
			m_timekeeper.log_time(timekeeper::CA_COMPLETE);
			m_timekeeper.print_stats(saved_uri.c_str());
		}
	}
	pInbandSI->ReleaseDescriptors();
#endif
	rmf_osal_mutexRelease( m_mutex);
}

void RMFQAMSrcImpl::requestINBProgramInfo()
{
	rmf_Error ret;

	if ( 0 != QAMParams.program_no)
	{
#ifndef DISABLE_INBAND_MGR
		ret = pInbandSI->RequestProgramInfo(QAMParams.program_no);
#endif
		if(RMF_SUCCESS != ret )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "Error in RequestProgramInfo. Source ID: %s.\n", saved_uri.c_str());
			if ( RMF_INBSI_INVALID_PARAMETER == ret)
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s(): PMT PID not available for program_number %d. Source ID: %s.\n", 
					__FUNCTION__, QAMParams.program_no, saved_uri.c_str());
				m_si_error =RMF_QAMSRC_ERROR_PROGRAM_NUMBER_INVALID;
			}
		}

		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "%s:%d Requested ProgramInfo \n", __FUNCTION__, __LINE__);
	}
	else
	{
		RDK_LOG( RDK_LOG_WARN, "LOG.RDK.QAMSRC", "%s:%d Not requesting ProgramInfo since pgm_no is 0 \n", __FUNCTION__, __LINE__);
	}
}

void RMFQAMSrcImpl::clearINBProgramInfo()
{
	rmf_Error ret;
	if ( 0 != QAMParams.program_no)
	{
#ifndef DISABLE_INBAND_MGR
		ret = pInbandSI->ClearProgramInfo(QAMParams.program_no);
#endif
		if(RMF_SUCCESS != ret )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "Error in ClearProgramInfo \n");
		}
		else
		{
			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "%s:%d Cleared ProgramInfo \n", __FUNCTION__, __LINE__);
		}
	}
	else
	{
		RDK_LOG( RDK_LOG_WARN, "LOG.RDK.QAMSRC", "%s:%d Not clearing ProgramInfo since pgm_no is 0 \n", __FUNCTION__, __LINE__);
	}
}

void RMFQAMSrcImpl::stopSIMonitorThread()
{
	rmf_Error ret;
	rmf_osal_event_handle_t event_handle;
	rmf_osal_event_params_t p_event_params = {	1, /* Priority */
		NULL,
		0,
		NULL
	};

	/* to exit the while loop of MonitorThread */
	ret = rmf_osal_event_create(RMF_OSAL_EVENT_CATEGORY_INB_FILTER, RMF_QAM_TERMINATE_THREAD, &p_event_params, &event_handle);
	if(ret!= RMF_SUCCESS)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "Could not create event handle:\n");
	}

	ret =  rmf_osal_eventqueue_add(siEventQHandle, event_handle);
	if(ret!= RMF_SUCCESS)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "rmf_osal_eventqueue_add failed\n");
	}

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s - Sent terminate event to SI event queue %d, waiting for signal \n", __FUNCTION__, siEventQHandle);
	ret = sem_wait( siDoneSem);
	if (0 != ret )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - sem_wait failed.\n", __FUNCTION__);
	}
	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "%s:%d after sem_wait()\n", __FUNCTION__, __LINE__); //Temp debug.
}

void RMFQAMSrcImpl::simonitorThread()
{
	rmf_osal_event_handle_t event_handle;
	uint32_t event_type;
	rmf_Error ret;
	uint8_t NotTerminated = TRUE;
	rmf_osal_event_params_t	eventData;	 
	//rmf_InbSiCache  *SICache = rmf_InbSiCache::getInbSiCache();	
	rmf_osal_TimeMillis currentTime;
	rmf_osal_Bool pipelineIsPaused = FALSE;
	rmf_osal_Bool notifyAllClear = false;

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "Enter %s():%d \n" , __FUNCTION__, __LINE__);
	/*	will be released while exiting; to sync with term()  */

	while(NotTerminated) 
	{
		if ( (TRUE == pmtEventSent ) || (TRUE == pipelineIsPaused))
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
				if ( 0 == psiAcqStartTime)
				{
					//Not expecting tables right now. There is no confirmed tuner lock.
					continue;
				}
				ret = rmf_osal_timeGetMillis( &currentTime );
				if(RMF_SUCCESS != ret )
				{
					RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "rmf_osal_timeGetMillis Failed ret= %d\n", ret);
					continue;
				}
				RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "%s:%d currentTime = %llu \n", __FUNCTION__, __LINE__, currentTime );
				if ( (PMT_TIME_OUT + m_pat_timeout_offset ) < (currentTime - psiAcqStartTime) )
				{
					if ( FALSE == patEventSent )
					{
						RDK_LOG( RDK_LOG_WARN, "LOG.RDK.QAMSRC", "PAT not received after timeout. Source ID: %s\n", saved_uri.c_str());
						m_si_error = RMF_QAMSRC_ERROR_PAT_NOT_AVAILABLE;
						notifyError(  RMF_QAMSRC_ERROR_PAT_NOT_AVAILABLE, NULL);
						g_pat_timeout_count++;
					}
					else
					{
						if ( 0 != QAMParams.program_no )
						{
							if(RMF_QAMSRC_ERROR_PROGRAM_NUMBER_INVALID_ON_PAT_UPDATE != m_si_error)
							{
								RDK_LOG( RDK_LOG_WARN, "LOG.RDK.QAMSRC", "PMT not received after timeout. Source ID: %s\n", saved_uri.c_str());
								m_si_error = RMF_QAMSRC_ERROR_PMT_NOT_AVAILABLE;
								notifyError(  RMF_QAMSRC_ERROR_PMT_NOT_AVAILABLE, NULL);
								g_pmt_timeout_count++;
							}
						}
					}
					patEventSent = TRUE;
					pmtEventSent = TRUE;
				}
				continue;
			}
		}

		if (RMF_SUCCESS != ret)
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "rmf_osal_eventqueue_get_next_event failed\n" );
			break;
		}

		if ( RMF_QAM_TERMINATE_THREAD == event_type )
		{
			rmf_osal_event_delete(event_handle);
			NotTerminated = FALSE;
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s Stopping event reception\n", __FUNCTION__);
			continue;
		}

		switch(event_type)
		{
			/* SI module events */
			case RMF_SI_EVENT_IB_PAT_ACQUIRED:

				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "Recvd PAT event. Source ID: %s\n", saved_uri.c_str());
				m_timekeeper.log_time(timekeeper::PAT_ACQ);
				//notifyError(  GSTQAMTUNERSRC_EVENT_PAT_ACQUIRED);
				patEventSent = TRUE;
#ifndef DISABLE_INBAND_MGR				
				ret  = pInbandSI->GetTSID(&tsId);
#endif
				if(RMF_SUCCESS != ret )
				{
					RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "Error in GetTSID \n");
				}
				else
				{
					RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", 
								"TSIDs : tsid from srm = %d, from pat = %d, m_validateTsId = %d  \n", 
								m_tsIdForValidation, tsId, m_validateTsId);
					if (m_validateTsId == true)
					{
						if (m_tsIdForValidation != tsId)
						{
							RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", 
								"TSID mismatch notify error: tsid from srm %d, from pat %d  \n", 
								m_tsIdForValidation, tsId);
							notifyError(RMF_QAMSRC_ERROR_TSID_MISMATCH, NULL);
						}
					}
				}
				pat_available = TRUE;

				rmf_osal_condSet(pat_cond);
				
				notifyStatusChange( RMF_QAMSRC_EVENT_PAT_ACQUIRED);
				requestINBProgramInfo();
				if(RMF_QAMSRC_ERROR_PROGRAM_NUMBER_INVALID == m_si_error)
				{
					/*Program was not present in PAT. We just tuned here and it 
					failed, so report that error right away*/
					notifyError(RMF_QAMSRC_ERROR_PROGRAM_NUMBER_INVALID, NULL);
					pmtEventSent = TRUE; //PMT acquisition timer is now meaningless. Don't timeout.
					rmf_osal_mutexAcquire(m_mutex);
					setErrorBits(QAMSRC_PROGRAM_NUMBER_INVALID_BITMASK, TRUE);
					rmf_osal_mutexRelease(m_mutex);
				}
				else
				{
					m_si_error = 0;
				}
				break;

			case RMF_SI_EVENT_IB_PMT_ACQUIRED:
				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "PMT ACQUIRED event. Source ID: %s\n", saved_uri.c_str());
				m_timekeeper.log_time(timekeeper::PMT_ACQ);
				pmtEventSent = TRUE;
				
				notifyStatusChange( RMF_QAMSRC_EVENT_PMT_ACQUIRED);
				processProgramInfo();
				m_si_error = 0;
				break;

			case RMF_SI_EVENT_IB_PAT_UPDATE:
				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "PAT update event\n");
				clearINBProgramInfo();
				if (0 != decimalSrcId)
				{
#ifdef TEST_WITH_PODMGR
					ret = getSourceInfo(saved_uri.c_str(), &pInfo, &decimalSrcId);
#else
					ret = OOBSIManager::getInstance()->GetProgramInfo(decimalSrcId, &pInfo);
#endif
					if( ret != RMF_SUCCESS)
					{
						RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "OOB GetProgramInfo failed\n");
						notifyError(  RMF_QAMSRC_ERROR_UNRECOVERABLE_ERROR, NULL);
					}
					/* Do not proceed if channel map has been updated.*/
					if((QAMParams.program_no != pInfo.prog_num) || (QAMParams.frequency != (pInfo.carrier_frequency / 1000)) || 
						(QAMParams.modulation != pInfo.modulation_mode))
					{
						RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "Error! Detected channel map update. Force release %s.\n", saved_uri.c_str());
						RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "New params: freq %d, prog num %d, modulation 0x%x\n",
								pInfo.carrier_frequency, pInfo.prog_num, pInfo.modulation_mode);
						m_qamtuner_error = RMF_QAMSRC_ERROR_UNRECOVERABLE_ERROR;
						notifyError(RMF_QAMSRC_ERROR_UNRECOVERABLE_ERROR, NULL);
						break;
					}
				}

				pmtEventSent = FALSE;
				
				ret = rmf_osal_timeGetMillis( &psiAcqStartTime);
				if(RMF_SUCCESS != ret )
				{
					RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "rmf_osal_timeGetMillis Failed ret= %d\n", ret);
				}
				m_si_error = 0;
				requestINBProgramInfo();
				if(RMF_QAMSRC_ERROR_PROGRAM_NUMBER_INVALID == m_si_error)
				{
					m_si_error = RMF_QAMSRC_ERROR_PROGRAM_NUMBER_INVALID_ON_PAT_UPDATE;
					notifyError(RMF_QAMSRC_ERROR_PROGRAM_NUMBER_INVALID_ON_PAT_UPDATE, NULL);
					pmtEventSent = TRUE; //PMT acquisition timer is now meaningless. Don't timeout.
					rmf_osal_mutexAcquire(m_mutex);
					setErrorBits(QAMSRC_PROGRAM_NUMBER_INVALID_BITMASK, TRUE);
					rmf_osal_mutexRelease(m_mutex);
				}
				else
				{
					m_si_error = 0;
					rmf_osal_mutexAcquire(m_mutex);
					rmf_osal_Bool notifyAllClear = setErrorBits(QAMSRC_PROGRAM_NUMBER_INVALID_BITMASK, FALSE);
					rmf_osal_mutexRelease(m_mutex);
					if(TRUE == notifyAllClear)
					{
						RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "Sending all-clear for %s.\n", saved_uri.c_str());
						notifyStatusChange(RMF_QAMSRC_EVENT_ERRORS_ALL_CLEAR);
					}
				}
				notifyStatusChange(RMF_QAMSRC_EVENT_PAT_UPDATE);
				break;

			case RMF_SI_EVENT_IB_PMT_UPDATE:
				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "PMT update event\n");
				processProgramInfo();
				m_si_error = 0;
				notifyStatusChange(RMF_QAMSRC_EVENT_PMT_UPDATE);
				break;

			case RMF_SI_EVENT_SI_NOT_AVAILABLE_YET:
				RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.QAMSRC", "RMF_SI_EVENT_SI_NOT_AVAILABLE_YET\n");
				break;
				
			case RMF_SI_EVENT_PIPELINE_PAUSED:
				/* Do not execute timed waits on PAT/PMT. The filters are off.*/
				RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", 
					"%s: no more PAT/PMT timeouts. Pipeline is paused.\n", __FUNCTION__);
				pipelineIsPaused = TRUE;
				break;
				
			case RMF_SI_EVENT_PIPELINE_PLAYING:
				RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", 
					"%s: PAT/PMT timeouts are now possible.\n", __FUNCTION__);
				pipelineIsPaused = FALSE;
				break;
#if defined(CAMGR_PRESENT) || defined(USE_EXTERNAL_CAS)
			case RMF_SI_EVENT_IB_CAT_ACQUIRED:
				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "CAT ACQUIRED event. Source ID: %s\n", saved_uri.c_str());

				pmtEventSent = TRUE;
				notifyStatusChange( RMF_QAMSRC_EVENT_CAT_ACQUIRED);
#ifdef CAMGR_PRESENT
				processCatInfo();
#endif
				m_si_error = 0;
				break;

			case RMF_SI_EVENT_IB_CAT_UPDATE:
				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "CAT UPDATED event. Source ID: %s\n", saved_uri.c_str());

				pmtEventSent = TRUE;
				notifyStatusChange( RMF_QAMSRC_EVENT_CAT_UPDATE);
#ifdef CAMGR_PRESENT
				processCatInfo();
#endif
				m_si_error = 0;
				break;
#endif
			case RMF_QAMSRC_ERROR_SPTS_TIMEOUT:
				rmf_osal_mutexAcquire(m_mutex);
				if(0 == m_qamtuner_error)
				{
					/*Save this error only if no other errors are reported. If there is a tuner unlock or 
					 * bad PAT/PMT update, those are the actual issues leading to this problem and ought to
					 * take precedence.*/
					m_qamtuner_error = RMF_QAMSRC_ERROR_SPTS_TIMEOUT;
				}
				setErrorBits(QAMSRC_SPTS_OUTAGE_BITMASK, TRUE);
				rmf_osal_mutexRelease(m_mutex);
				notifyError(RMF_QAMSRC_ERROR_SPTS_TIMEOUT, NULL);
				break;

			case RMF_QAMSRC_EVENT_SPTS_OK:
				rmf_osal_mutexAcquire(m_mutex);
				if(RMF_QAMSRC_ERROR_SPTS_TIMEOUT == m_qamtuner_error)
				{
					m_qamtuner_error = 0;
				}
				notifyAllClear = setErrorBits(QAMSRC_SPTS_OUTAGE_BITMASK, FALSE);
				rmf_osal_mutexRelease(m_mutex);
				if(TRUE == notifyAllClear)
				{
					RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "Sending all-clear for %s.\n", saved_uri.c_str());
					notifyStatusChange(RMF_QAMSRC_EVENT_ERRORS_ALL_CLEAR);
				}
				break;

#ifdef TEST_WITH_PODMGR
			case RMF_QAMSRC_EVENT_CA_SYSTEM_READY:
				rmf_osal_mutexAcquire(m_mutex);
				if((RMFQAMSrcImpl::QAM_STATE_PLAYING == state) && 
					(0 != m_pod_error) && (RMF_QAMSRC_EVENT_CANNOT_DESCRAMBLE_ENTITLEMENT != m_pod_error))
				{
					/* We're PLAYING, there is a POD error and it's not "cannot descramble". This means we're
					 * recovering from a CARD reset. Restart CA.*/
					RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "Initiating automatic CA recovery for for %s.\n", saved_uri.c_str());
					setupDecryption();

					int ltsid;
					g_object_get( src_element, "ltsid", &ltsid, NULL );
					RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s:: Calling rmf_podmgrConfigCipher :  ltsid = %d. Source ID: %s \n",
							__FUNCTION__, ltsid, saved_uri.c_str());
					int result = rmf_podmgrConfigCipher((unsigned char)ltsid, 0, mediaInfo.pPidList, mediaInfo.numMediaPids);
					if( 0 != result)
					{
						RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s:: vlConfigCipherPidFilterSet failed result = %d \n",__FUNCTION__, result);
					}
					else
					{
						RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s:: vlConfigCipherPidFilterSet SUCCESS \n",__FUNCTION__);
					}
				}
				rmf_osal_mutexRelease(m_mutex);
				notifyStatusChange(RMF_QAMSRC_EVENT_CA_SYSTEM_READY);
				break;
#endif
			case RMF_SI_EVENT_IB_TDT_ACQUIRED:
			{
				char* data;
				int   data_size = 0;

				data = pInbandSI->get_TDT(&data_size);
				if (data)
				{
					notifySection(data, data_size);
					free(data);
				}
				break;
			}
			case RMF_SI_EVENT_IB_TOT_ACQUIRED:
			{
				char* data;
				int   data_size = 0;

				data = pInbandSI->get_TOT(&data_size);
				if (data)
				{
					notifySection(data, data_size);
					free(data);
				}
				break;
			}
			case RMF_SI_EVENT_IB_SECTION_ACQUIRED:
			{
				RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "RMF_SI_EVENT_IB_SECTION_ACQUIRED Recieved - %d, filterId =  \n",event_type, eventData.data_extension);
				notifyStatusChange(RMF_QAMSRC_EVENT_SECTION_ACQUIRED);
				break;
			}
			default:
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "Unknown event %d \n",event_type);
				break;
		}

		ret = rmf_osal_event_delete(event_handle);
		if (RMF_SUCCESS != ret )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - rmf_osal_event_delete failed.\n", __FUNCTION__);
		}
	}

	/* signal thread is exited*/
	ret = sem_post( siDoneSem);
	if (0 != ret )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - sem_post failed.\n", __FUNCTION__);
	}
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "Exit %s():%d \n" , __FUNCTION__, __LINE__);
}/* End of gst_qamtunersrc_simonitorthread */

 RMFResult RMFQAMSrcImpl::getTSID( uint32_t &tsId)
{
	RMFResult res = RMF_RESULT_SUCCESS;
	QAMParams.program_no = 0; // Stop setting filter for PMT

	if (pat_available)
	{
		tsId = this->tsId;
	}
	else
	{
		RMFResult condWaitResult = RMF_SUCCESS;
		
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "%s - PAT isn't readily available. Waiting..\n", __FUNCTION__ );
		condWaitResult = rmf_osal_condWaitFor( pat_cond, PAT_COND_TIME_OUT);
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "%s - Conditional wait returned %d..\n", __FUNCTION__, condWaitResult );

		if (pat_available)
		{
			tsId = this->tsId;
		}
		else
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - pat not available after timeout.\n", __FUNCTION__);
			res = RMF_RESULT_FAILURE;
		}
	}
	return res;
}


RMFResult RMFQAMSrcImpl::getLTSID( unsigned char &ltsId)
{
	RMFResult res = RMF_RESULT_FAILURE;
	if ( src_element )
	{
		g_object_get( src_element, "ltsid", &ltsId, NULL );
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "%s - LTSID = 0x%x\n", __FUNCTION__, ltsId);
		res = RMF_RESULT_SUCCESS;
	}
	else
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - src_element NULL\n", __FUNCTION__);
	}
	return res;
}

rmf_osal_Bool RMFQAMSrcImpl::readyForReUse(const char* uri)
{
	if ( (0 != m_pod_error) ||(RMF_QAMSRC_ERROR_UNRECOVERABLE_ERROR == m_qamtuner_error ) ||(0 != m_si_error) || (0 == QAMParams.program_no))
	{
		RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", " %s():%d Returning false : qamsrcImpl %p m_pod_error = 0x%x m_qamtuner_error=0x%x m_si_error=0x%x pgm no=%d\n" , 
				__FUNCTION__, __LINE__, this, m_pod_error, m_qamtuner_error, m_si_error, QAMParams.program_no);
		return FALSE;
	}
	else
	{
		if (uri != NULL) {
			rmf_ProgramInfo_t programInfo;
			memset(&programInfo, 0, sizeof(programInfo));
			uint32_t _decimalSrcId;
			RMFResult rc  = getSourceInfo( uri, &programInfo, &_decimalSrcId);

			RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "FREQ_CHANGE_CHECK: %s():%d: (curr) %d vs (cached) %d for uri=%s\n",
					__FUNCTION__, __LINE__, programInfo.carrier_frequency/1000, QAMParams.frequency, uri);

			if ((rc != RMF_RESULT_SUCCESS) || ((programInfo.carrier_frequency/1000) != QAMParams.frequency)) {
				m_qamtuner_error = RMF_QAMSRC_ERROR_UNRECOVERABLE_ERROR; /*Channel map has changed. This object uses outdated tuning parameters. 
				Discourage future reuse.*/
				RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", " %s():%d NOW Returning false due to freq mismatch\n", __FUNCTION__, __LINE__);
				return FALSE;
			}
			else {
				return TRUE;
			}
		}
		else {
			RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", " %s():%d NOW Returning true\n", __FUNCTION__, __LINE__);
			return TRUE;
		}
	}
}

void RMFQAMSrcImpl::setPODError( uint32_t value)
{
	m_pod_error = value;
}

std::vector<IRMFMediaEvents*> RMFQAMSrcImpl::m_event_handler;
RMFResult RMFQAMSrcImpl::addPlatformEventHandler(IRMFMediaEvents* events)
{
	rmf_osal_mutexAcquire( mutex);
	m_event_handler.insert(m_event_handler.begin(), events);
	rmf_osal_mutexRelease( mutex);
	return RMF_SUCCESS;
}

RMFResult RMFQAMSrcImpl::removePlatformEventHandler(IRMFMediaEvents* events)
{
	rmf_osal_mutexAcquire( mutex);
	for( std::vector<IRMFMediaEvents*>::iterator it= m_event_handler.begin(); it != m_event_handler.end(); ++it )
	{
		if(*it == events)
		{
			m_event_handler.erase(it);
			break;
		}
	}
	rmf_osal_mutexRelease( mutex);
	return RMF_SUCCESS;	
}

RMFResult RMFQAMSrcImpl::sendPlatformStatusEvent( unsigned long event)
{
	RMFStreamingStatus status;
	status.m_status = event;
	rmf_osal_mutexAcquire( mutex);
	for( std::vector<IRMFMediaEvents*>::iterator it= m_event_handler.begin(); it != m_event_handler.end(); ++it )
	{
		if(*it)
		{
			(*it)->status(status);
		}
	}
	rmf_osal_mutexRelease( mutex);
	return RMF_SUCCESS;
}

RMFResult RMFQAMSrcImpl::isEIDAuthorised ( bool *pAuthStatus )
{
	rmf_osal_Bool Auth = true;
	RMFResult Ret = RMF_RESULT_FAILURE;
	rmf_Error podError = RMF_SUCCESS;

#ifdef TEST_WITH_PODMGR
	const char *readEIDStatus = NULL;
	readEIDStatus = rmf_osal_envGet("QAMSRC.DISABLE.EID");
	if ( ( readEIDStatus == NULL) || 
		( (readEIDStatus != NULL) && (strcmp(readEIDStatus, "TRUE") ) == 0))
	{
		RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", " %s():%d QAMSRC.DISABLE.EID set to TRUE not defined or, Not reading  EID Status\n"
				, __FUNCTION__, __LINE__);
		*pAuthStatus= true;
		Ret = RMF_RESULT_SUCCESS;
	}
	else
	{
		podError = rmf_podmgrGetEIDAuthStatus( POD_EID_DISCONNECT, &Auth );
		if( RMF_SUCCESS == podError )
		{
			RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", " %s():%d Got EID status from POD \n"
					, __FUNCTION__, __LINE__);			
			*pAuthStatus = Auth;
			Ret = RMF_RESULT_SUCCESS;
		}
		else if ( RMF_OSAL_ENODATA == podError )
		{
			RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", " %s():%d EID logic is not initialized \n"
					, __FUNCTION__, __LINE__);
			/* not setting pAuthStatus as the application decide the value at this stage */
			//*pAuthStatus = true;
			Ret = RMF_RESULT_NOT_INITIALIZED;
		}
		else 
		{
			RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", " %s():%d Fialed to get EID status from POD \n"
					, __FUNCTION__, __LINE__);
			*pAuthStatus = false;
			Ret = RMF_RESULT_FAILURE;
		}
	}
#endif
	RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", " %s():%d Returning EID status %s \n"
			, __FUNCTION__, __LINE__, (*pAuthStatus  == true)?"true":"false");
	return Ret;
}

uint32_t RMFQAMSrcImpl::numberOfTuners = 0; 
rmf_osal_Bool RMFQAMSrcImpl::cachingEnabled = TRUE;
rmf_osal_Bool RMFQAMSrcImpl::enableTTS = FALSE;



RMFResult RMFQAMSrcImpl::init_platform()
{
	rmf_Error rmf_error;
	rmf_osal_ThreadId thid; 

	const char *readEIDStatus = NULL;
	readEIDStatus = rmf_osal_envGet("QAMSRC.DISABLE.EID");
	if ( ( readEIDStatus == NULL) || 
		( (readEIDStatus != NULL) && (strcmp(readEIDStatus, "TRUE") ) == 0))
	{
		bQAMSrcEIDCheck = false;
	}
	else
	{
		bQAMSrcEIDCheck = true;
	}

#ifdef TEST_WITH_PODMGR
	rmf_podmgrInit();
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s - rmf_podmgrInit completed.\n", __FUNCTION__);
#endif

	rmf_error = rmf_osal_eventqueue_create( (const uint8_t*) "QamQ", &g_eventq);
	if (RMF_SUCCESS != rmf_error )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - rmf_osal_eventqueue_create failed.\n", __FUNCTION__);
		return RMF_RESULT_FAILURE;
	}

	rmf_error = rmf_osal_mutexNew(& RMFQAMSrcImpl::mutex);
	if (RMF_SUCCESS != rmf_error )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - rmf_osal_mutexNew failed.\n", __FUNCTION__);
		return RMF_RESULT_FAILURE;
	}

	const char *numTuners = rmf_osal_envGet("MPE.SYS.NUM.TUNERS");
	if (NULL == numTuners )
	{
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s - Couldnot get number of tuners from config file."
				" Using default value %d\n", __FUNCTION__, QAMSRC_DEF_NUMBER_OF_TUNERS);
		numberOfTuners = QAMSRC_DEF_NUMBER_OF_TUNERS;
	}
	else
	{
		numberOfTuners =  atoi(numTuners);
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s - Number of tuners from config file= %d\n", __FUNCTION__, numberOfTuners);
	}

	

	const char * pEnableTTS= rmf_osal_envGet("MEDIASTREAMER.LIVESTREAMING.192TS");
	if ( (pEnableTTS != NULL)
			&& (strcmp(pEnableTTS, "TRUE") == 0))
	{
		enableTTS = TRUE;
		RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", " %s():%d TTS is enabled in configuration\n"
				, __FUNCTION__, __LINE__);
	}
	else
	{
		enableTTS = FALSE;
		RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", " %s():%d TTS is disabled in configuration\n"
				, __FUNCTION__, __LINE__);
	}

	rmf_error =  rmf_osal_memAllocP (RMF_OSAL_MEM_MEDIA, (numberOfTuners+1)*sizeof (rmf_transport_path_info ), (void**)&ga_transport_path_info);
	if (RMF_SUCCESS != rmf_error )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - rmf_osal_memAllocP failed for ga_transport_path_info.\n", __FUNCTION__);
		return RMF_RESULT_FAILURE;
	}
    	rmf_error =  rmf_osal_memAllocP (RMF_OSAL_MEM_MEDIA, (numberOfTuners+1)*sizeof (diag_tuner_info_t ), (void**)&ga_diag_info);
	if (RMF_SUCCESS != rmf_error )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - rmf_osal_memAllocP failed for ga_diag_info.\n", __FUNCTION__);
		return RMF_RESULT_FAILURE;
	}
#ifdef SNMP_IPC_CLIENT
    rmf_error = qamSrcServerStart();
	if ( RMF_RESULT_SUCCESS != rmf_error)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - qamSrcServerStart failed.\n", __FUNCTION__);
	}
#endif // SNMP_IPC_CLIENT

#ifdef TEST_WITH_PODMGR
	rmf_error = rmf_podmgrRegisterEvents(g_eventq);
	if (RMF_SUCCESS != rmf_error )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - rmf_podmgrRegisterEvents failed.\n", __FUNCTION__);
		return RMF_RESULT_FAILURE;
	}
	/*
	   rmf_error = disconnectMgrSubscribeEvents(g_eventq);
	   if (RMF_SUCCESS != rmf_error )
	   {
	   RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - disconnectMgrSubscribeEvents failed.\n", __FUNCTION__);
	   return RMF_RESULT_FAILURE;
	   }
	 */
	rmf_error = rmf_osal_threadCreate( qamsrc_eventmonitor_thread, NULL, 
			RMF_OSAL_THREAD_PRIOR_DFLT, RMF_OSAL_THREAD_STACK_SIZE, 
			&thid, "QamsrcThread");
	if (RMF_SUCCESS != rmf_error )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - rmf_osal_threadCreate failed.\n", __FUNCTION__);
		return RMF_RESULT_FAILURE;
	}
#endif

	const char* qamDisallowTuningInNullState = rmf_osal_envGet("QAMSRC.DISALLOW.TUNING.IN.NULL.STATE");
	if ( (qamDisallowTuningInNullState != NULL)
			&& (strcmp(qamDisallowTuningInNullState, "TRUE") == 0))
	{
		supportTuningInNULLState = FALSE;
		RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", " %s(): Tuner will only function in READY or higher states.\n"
			, __FUNCTION__);
	}

	bQAMSrcInited = true;

    check_rf_presence();

	return RMF_RESULT_SUCCESS;
}

void RMFQAMSrcImpl::check_rf_presence()
{
    RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", " %s(): Checking RF presence...\n"
        , __FUNCTION__);
	const char * pScanList= rmf_osal_envGet("THERMO.RF_SCAN_LIST");
	if(pScanList == NULL)
    {
        pScanList = "/tmp/thermo_qam_scan_list.txt";
        FILE * fp = fopen(pScanList, "w");
        if(NULL != fp)
        {
            /*
--------------------------------------------------------------------------------------------------------------------------------
Table 29 – First Twelve Hunt Channel List Channel (as listed in CEA-542-C) Plan / Carrier
--------------------------------------------------------------------------------------------------------------------------------
81 STD
82 STD
83 STD
84 STD
85 STD
100 STD
101 STD
68 STD
69 STD
126 STD
81 HRC
82 HRC
--------------------------------------------------------------------------------------------------------------------------------
|     |    Channel Edge     |             Analog             |                            Digital                              |
--------------------------------------------------------------------------------------------------------------------------------
|  #  | Low      | High     | STD      | IRC      | HRC      | STD-VSB  | STD-QAM  | IRC-VSB  | IRC-QAM  | HRC-VSB  | HRC-QAM  |
--------------------------------------------------------------------------------------------------------------------------------
|  81 | 564.0000 | 570.0000 | 565.2500 | 565.2625 | 564.0282 | 564.3100 | 567.0000 | 564.3100 | 567.0000 | 563.0600 | 565.7500 |
|  82 | 570.0000 | 576.0000 | 571.2500 | 571.2625 | 570.0285 | 570.3100 | 573.0000 | 570.3100 | 573.0000 | 569.0600 | 571.7500 |
|  83 | 576.0000 | 582.0000 | 577.2500 | 577.2625 | 576.0288 | 576.3100 | 579.0000 | 576.3100 | 579.0000 | 575.0600 | 577.7500 |
|  84 | 582.0000 | 588.0000 | 583.2500 | 583.2625 | 582.0291 | 582.3100 | 585.0000 | 582.3100 | 585.0000 | 581.0600 | 583.7500 |
|  85 | 588.0000 | 594.0000 | 589.2500 | 589.2625 | 588.0294 | 588.3100 | 591.0000 | 588.3100 | 591.0000 | 587.0600 | 589.7500 |
| 100 | 648.0000 | 654.0000 | 649.2500 | 649.2625 | 648.0324 | 648.3100 | 651.0000 | 648.3100 | 651.0000 | 647.0600 | 649.7500 |
| 101 | 654.0000 | 660.0000 | 655.2500 | 655.2625 | 654.0327 | 654.3100 | 657.0000 | 654.3100 | 657.0000 | 653.0600 | 655.7500 |
|  68 | 486.0000 | 492.0000 | 487.2500 | 487.2625 | 486.0243 | 486.3100 | 489.0000 | 486.3100 | 489.0000 | 485.0600 | 487.7500 |
|  69 | 492.0000 | 498.0000 | 493.2500 | 493.2625 | 492.0246 | 492.3100 | 495.0000 | 492.3100 | 495.0000 | 491.0600 | 493.7500 |
| 126 | 804.0000 | 810.0000 | 805.2500 | 805.2625 | 804.0402 | 804.3100 | 807.0000 | 804.3100 | 807.0000 | 803.0600 | 805.7500 |
|  81 | 564.0000 | 570.0000 | 565.2500 | 565.2625 | 564.0282 | 564.3100 | 567.0000 | 564.3100 | 567.0000 | 563.0600 | 565.7500 |
|  82 | 570.0000 | 576.0000 | 571.2500 | 571.2625 | 570.0285 | 570.3100 | 573.0000 | 570.3100 | 573.0000 | 569.0600 | 571.7500 |
--------------------------------------------------------------------------------------------------------------------------------
             */
            fprintf(fp, "tune://frequency=%d00&modulation=%d\n", int(567.0000*10000), 16);
            fprintf(fp, "tune://frequency=%d00&modulation=%d\n", int(573.0000*10000), 16);
            fprintf(fp, "tune://frequency=%d00&modulation=%d\n", int(579.0000*10000), 16);
            fprintf(fp, "tune://frequency=%d00&modulation=%d\n", int(585.0000*10000), 16);
            fprintf(fp, "tune://frequency=%d00&modulation=%d\n", int(591.0000*10000), 16);
            fprintf(fp, "tune://frequency=%d00&modulation=%d\n", int(651.0000*10000), 16);
            fprintf(fp, "tune://frequency=%d00&modulation=%d\n", int(657.0000*10000), 16);
            fprintf(fp, "tune://frequency=%d00&modulation=%d\n", int(489.0000*10000), 16);
            fprintf(fp, "tune://frequency=%d00&modulation=%d\n", int(495.0000*10000), 16);
            fprintf(fp, "tune://frequency=%d00&modulation=%d\n", int(807.0000*10000), 16);
            fprintf(fp, "tune://frequency=%d00&modulation=%d\n", int(567.0000*10000), 16);
            fprintf(fp, "tune://frequency=%d00&modulation=%d\n", int(573.0000*10000), 16);
            fprintf(fp, "tune://frequency=%d00&modulation=%d\n", int(565.7500*10000), 16);
            fprintf(fp, "tune://frequency=%d00&modulation=%d\n", int(571.7500*10000), 16);
            fclose(fp);
        }
		RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", " %s(): THERMO.RF_SCAN_LIST Not defined so generating and using '%s'\n"
				, __FUNCTION__, pScanList);
    }
    
	if(pScanList != NULL)
	{
		RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", " %s(): THERMO.RF_SCAN_LIST = '%s'\n"
				, __FUNCTION__, pScanList);
        FILE * fp = fopen(pScanList, "r");
        if(NULL != fp)
        {
            RMFMediaSourceBase * pImpl = new RMFQAMSrc();
            if(NULL != pImpl)
            {
                RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", " %s(): new RMFQAMSrcImpl() returned 0x%08X.\n"
                    , __FUNCTION__, pImpl);
                RMFResult ret = RMF_RESULT_SUCCESS;
                if(RMF_RESULT_SUCCESS == (ret = pImpl->init()))
                {
                    RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", " %s(): RMFQAMSrcImpl::init() returned SUCCESS.\n"
                        , __FUNCTION__);
                    //const char * uri = "tune://frequency=237000000&modulation=16";
                    char szUriBuf[128];
                    while(1)
                    {
                        memset(szUriBuf, 0, sizeof(szUriBuf));
                        const char * uri = fgets(szUriBuf, sizeof(szUriBuf), fp);
                        if(NULL != uri)
                        {
                            char * pszEOL = strrchr(szUriBuf, '\r');
                            if(NULL != pszEOL) *pszEOL='\0';
                            pszEOL = strrchr(szUriBuf, '\n');
                            if(NULL != pszEOL) *pszEOL='\0';
                            if(0 == strlen(uri)) continue;
                            if(0 != strncmp(TUNEPARAMS_PRIFIX_STRING, uri, strlen(TUNEPARAMS_PRIFIX_STRING))) continue;
                            if(RMF_RESULT_SUCCESS == (ret = pImpl->open(uri, NULL)))
                            {                
                                RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", " %s(): RMFQAMSrcImpl::open('%s', NULL) returned SUCCESS.\n"
                                    , __FUNCTION__, uri);
                                rmf_osal_threadSleep(500,0);
                                TunerData tunerData;
                                memset(&tunerData, 0, sizeof(tunerData));
                                rmf_Error rmfret  = getTunerInfo(0, &tunerData);
                                if (RMF_SUCCESS != rmfret)
                                {
                                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s:%d getTunerInfo failed\n",__FUNCTION__,__LINE__);
                                }
                                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s(): frequency= %d. modulation= %d. totalTuneCount= %d. tuneFailCount= %d. tunerState= %d. carrierLockLostCount= %d. lastTuneFailFreq= %d.\n",
                                                __FUNCTION__,tunerData.frequency, tunerData.modulation, tunerData.totalTuneCount, tunerData.tuneFailCount, tunerData.tunerState, tunerData.carrierLockLostCount, tunerData.lastTuneFailFreq);
                                if(TUNER_STATE_TUNED_SYNC == tunerData.tunerState)
                                {
                                    RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "%s(): Tuner successfully locked to '%s', i.e. frequency appears to have carrier.\n"
                                        , __FUNCTION__, uri);
                                        
                                    IARM_Bus_SYSMgr_EventData_t eventData;
                                    eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_RF_CONNECTED;
                                    eventData.data.systemStates.state = 1;
                                    eventData.data.systemStates.error = 0;
                                    IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME,
                                        (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE,
                                        (void *)&eventData, sizeof(eventData));

                                    RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "%s(): RF-Check Passed. Logging 'RF Present'.\n" , __FUNCTION__);
                                    pImpl->close();
                                    break;
                                }
                                else
                                {
                                    RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "%s(): Tuner failed to lock to '%s', continuing with list.\n"
                                        , __FUNCTION__, uri);
                                }
                                pImpl->close();
                            }
                            else
                            {
                                RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.QAMSRC", " %s(): RMFQAMSrcImpl::open('%s', NULL) returned %d.\n"
                                    , __FUNCTION__, uri, ret);
                            }
                        }
                        else
                        {
                            RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", " %s(): fgets() returned NULL, probably EOF.\n"
                                , __FUNCTION__);

                            IARM_Bus_SYSMgr_EventData_t eventData;
                            eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_RF_CONNECTED;
                            eventData.data.systemStates.state = 0;
                            eventData.data.systemStates.error = 1;
                            IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME,
                                (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE,
                                (void *)&eventData, sizeof(eventData));

                            RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "%s(): RF-Check FAILED. Logging 'RF ABSENT'.\n" , __FUNCTION__);
                            break;
                        }
                    }
                    pImpl->term();
                }
                else
                {
                    RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.QAMSRC", " %s(): RMFQAMSrcImpl::init() returned %d.\n"
                        , __FUNCTION__, ret);
                }
                delete pImpl;
            }    
            else
            {
                RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.QAMSRC", " %s(): new RMFQAMSrcImpl() returned NULL.\n"
                    , __FUNCTION__);
            }
            fclose(fp);
        }
        else
        {
            RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", " %s(): Could not open '%s',\n"
                    , __FUNCTION__, pScanList);
        }
	}
    else
    {
		RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", " %s(): THERMO.RF_SCAN_LIST = NULL.\n"
				, __FUNCTION__);
    }
}

RMFResult RMFQAMSrcImpl::uninit_platform()
{
	rmf_Error rmf_error;
	bQAMSrcInited = false;
#ifdef SNMP_IPC_CLIENT
    rmf_error = qamSrcServerStop();
	if ( RMF_RESULT_SUCCESS != rmf_error)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - qamSrcServerStop failed.\n", __FUNCTION__);
	}
#endif // SNMP_IPC_CLIENT

#ifdef TEST_WITH_PODMGR
	rmf_error = rmf_podmgrUnRegisterEvents(g_eventq);
	if (RMF_SUCCESS != rmf_error )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - rmf_podmgrUnRegisterEvents failed.\n", __FUNCTION__);
	}
	rmf_podmgrUnInit();
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s - rmf_podmgrUnInit completed.\n", __FUNCTION__);
#endif
	return RMF_RESULT_SUCCESS;
}


void RMFQAMSrcImpl::disableCaching()
{
	cachingEnabled = FALSE;
}

void* RMFQAMSrcImpl::createLowLevelElement()
{
	void* lowLevelElement = NULL;

	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.QAMSRC", " %s():%d Enter\n" , __FUNCTION__, __LINE__);
	lowLevelElement = (void*)gst_element_factory_make("qamtunersrc", "qsrc");;
	if (NULL == lowLevelElement )
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "gst_element_factory_make failed\n");
	}
	else
	{
		gst_object_ref ( ( gpointer)lowLevelElement);

		if ( TRUE == enableTTS )
		{
			g_object_set  (lowLevelElement,
					"tts",TRUE,
					NULL);
		}
	}
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.QAMSRC", " %s():%d Exit \n" , __FUNCTION__, __LINE__);
	return lowLevelElement;
}

void RMFQAMSrcImpl::freeLowLevelElement( void* element)
{
	gst_object_unref ( ( gpointer)element);
}

void* RMFQAMSrcImpl::createElement()
{
	RDK_LOG(RDK_LOG_TRACE1,"LOG.RDK.QAMSRC", "Enter %s():%d \n" , __FUNCTION__, __LINE__);

	// Create an empty bin. It will be populated in open().
	GstElement* bin = gst_bin_new("qamsrc_bin");
	if (NULL != bin)
	{
		// Create a ghost source pad with no target.
		// It will be connected to a child element ().
		gst_element_add_pad(bin, gst_ghost_pad_new_no_target("src", GST_PAD_SRC));
	}
	RDK_LOG(RDK_LOG_TRACE1,"LOG.RDK.QAMSRC", "Exit %s():%d \n" , __FUNCTION__, __LINE__);
	return bin;
}/* End of createElement */

const rmf_MediaInfo* RMFQAMSrcImpl::getInbandMediaInfo()
{
	return &mediaInfo;
}

uint32_t RMFQAMSrcImpl::getSourceId()
{
	return decimalSrcId;
}

void RMFQAMSrcImpl::notifySignalsDisconnected()
{
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.QAMSRC", "Notify signals disconnected. 0x%x\n" , this);
	rmf_osal_condSet(m_signals_disconnected);
}

void RMFQAMSrcImpl::notifySIMonitorThread(uint32_t event_type)
{
	rmf_osal_event_params_t event_params = {0};
	rmf_osal_event_handle_t event_handle;
	rmf_Error ret = RMF_INBSI_SUCCESS;
	event_params.priority = 1; //Use a higher priority than PAT/PMT events.
	ret = rmf_osal_event_create(RMF_OSAL_EVENT_CATEGORY_MPEG, event_type, &event_params, &event_handle);
	if(ret != RMF_INBSI_SUCCESS)
	{
		RDK_LOG(
			RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s: Could not create event handle: 0x%x\n",
			__FUNCTION__, ret);
		return;
	}

	ret = rmf_osal_eventqueue_add(siEventQHandle, event_handle);
	if (ret != RMF_SUCCESS)
	{
		RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.QAMSRC",
		        "%s: Could not dispatch event: 0x%x\n", __FUNCTION__, ret);
		rmf_osal_event_delete(event_handle);
	}
}

#ifdef TEST_WITH_PODMGR
RMFResult RMFQAMSrcImpl::getCCIInfo( char& cciInfo)
{
	rmf_Error ret;
	uint8_t cciBits;

	RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.QAMSRC", "Enter %s():%d. Source ID: %s.\n" ,
		__FUNCTION__, __LINE__, saved_uri.c_str());
	if (0 == sessionHandlePtr )
	{
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "getCCIInfo: sessionHandlePtr is NULL. Source ID: %s\n",
				saved_uri.c_str());
		return RMF_RESULT_FAILURE;
	}
	else
	{
		ret = rmf_podmgrGetCCIBits( sessionHandlePtr, &cciBits);
		if (RMF_SUCCESS != ret)
		{
            if(TRUE== is_clear_content)
            {
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "getCCIInfo: this is a clear channel. No CCI available. Source ID: %s\n",
                        saved_uri.c_str());
            }
            else
            {
			    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "getCCI info - getting CCI bits failed. Source ID: %s\n",
				    saved_uri.c_str());
            }
			return RMF_RESULT_FAILURE;
		}
		else
		{
			cciInfo = cciBits;
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "getCCI info - successfully obtained CCI bits as %d. Source ID: %s\n",
				cciInfo, saved_uri.c_str());
			return RMF_RESULT_SUCCESS;
		}
	}
}

RMFResult RMFQAMSrcImpl::waitForCCI (char& cci)
{
	GTimeVal timeVal;
	RMFResult result = RMF_RESULT_FAILURE;

	g_mutex_lock ( m_cciMutex );
	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "%s():%d  this = %p Waiting on CCI condition var\n" , __FUNCTION__, __LINE__, this);

	if( RMF_QAMSRC_CCI_STATUS_NONE == m_cci)
	{
		g_get_current_time (&timeVal);
		g_time_val_add (&timeVal, CCI_WAIT_TIMEOUT_MS*1000);
		if (FALSE == g_cond_timed_wait (m_cciCond, 	m_cciMutex, &timeVal) )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s():%d  this = %p CCI event not received after timeout\n" ,  __FUNCTION__, __LINE__, this);
			notifyError(RMF_QAMSRC_ERROR_UNRECOVERABLE_ERROR, NULL);
		}
		else
		{
			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "%s():%d this = %p g_cond_timed_wait returned TRUE\n" , __FUNCTION__, __LINE__, this);
		}
	}
	else
	{
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "%s():%d  this = %p Not waiting since already available\n" , __FUNCTION__, __LINE__, this);
	}

	if( RMF_QAMSRC_CCI_STATUS_NONE != m_cci)
	{
		cci = m_cci;
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s():%d returning CCI  0x%x\n" , __FUNCTION__, __LINE__, m_cci);
		result = RMF_RESULT_SUCCESS;
	}
	g_mutex_unlock ( m_cciMutex);

	return result;
} 

void RMFQAMSrcImpl::sendCCIChangedEvent( )
{
	char cci;
	RMFResult result;

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s: Enter\n",__FUNCTION__ );
	result = getCCIInfo(cci);
	if (RMF_RESULT_SUCCESS == result )
	{
		g_mutex_lock ( m_cciMutex );
		m_cci = cci;
		g_cond_signal ( m_cciCond);
		g_mutex_unlock ( m_cciMutex);

		RMFMediaSourcePrivate::sendCCIChangedEvent(cci);
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s: Signalled CCI event. CCI data read= 0x%x\n", __FUNCTION__,  cci);
	}
	else
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s: getCCIInfo failed\n",__FUNCTION__ );
	}
}


void RMFQAMSrcImpl::notifyAuthorizationStatus( rmf_osal_Bool auth)
{
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s: Enter this = %p auth = 0x%x\n",__FUNCTION__, this, auth );

	/*In case of authorization failure, signal cci condition as cci event will not be available.*/
	m_timekeeper.log_time(timekeeper::CA_COMPLETE);	
	m_timekeeper.print_stats(saved_uri.c_str());
	rmf_osal_mutexAcquire ( m_mutex );
	m_isAuthorized = auth;
	if (FALSE == auth )
	{
		g_object_set(src_element, "isAuthorized", FALSE, NULL);
	 	rmf_osal_mutexRelease ( m_mutex);		
		g_mutex_lock ( m_cciMutex );
		m_cci = RMF_QAMSRC_CCI_STATUS_NONE;
		g_cond_signal ( m_cciCond);
		g_mutex_unlock ( m_cciMutex);
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "%s: this = %p Signalled CCI condition RMF_QAMSRC_CCI_STATUS_NONE\n", __FUNCTION__, this);
	}
	else
	{
		m_pod_error = 0;
		g_object_set  (src_element, "ready", NULL, NULL);
		g_object_set(src_element, "isAuthorized", TRUE, NULL);
	 	rmf_osal_mutexRelease ( m_mutex);
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s(): Tune request -filters set and streaming started. Source ID: %s.\n", 
			__FUNCTION__, saved_uri.c_str());
		notifyStatusChange( RMF_QAMSRC_EVENT_STREAMING_STARTED);
	}

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "%s: Signalled authorization status this = %p auth = 0x%x\n", __FUNCTION__, this, auth);
}


rmf_PodmgrDecryptSessionHandle RMFQAMSrcImpl::getPodSessionHandle()
{
	return sessionHandlePtr;
}

rmf_PodmgrDecryptSessionHandle RMFQAMSrcImpl::getQamContext()
{
	return m_context;
}

void RMFQAMSrcImpl::setPodSessionHandle( rmf_PodmgrDecryptSessionHandle podSessionHandle)
{
	sessionHandlePtr = podSessionHandle;
}
void RMFQAMSrcImpl::handlePODRemoval()
{
	setPODError(RMF_QAMSRC_EVENT_POD_REMOVED);
	rmf_osal_mutexAcquire( m_mutex);
	setPodSessionHandle((rmf_PodmgrDecryptSessionHandle) NULL);
	rmf_osal_mutexRelease( m_mutex);
	notifyError(RMF_QAMSRC_EVENT_POD_REMOVED, NULL);
}
void RMFQAMSrcImpl::handlePODInsertion()
{
	notifyStatusChange(RMF_QAMSRC_EVENT_POD_READY);
}


void RMFQAMSrcImpl::handleCASystemReady()
{
	notifySIMonitorThread(RMF_QAMSRC_EVENT_CA_SYSTEM_READY);
}

/* Return Local TS id for the given tuner ID */
rmf_Error rmf_qamsrc_getLtsid( int id, unsigned char *ltsid )
{
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.QAMSRC", "Enter %s():%d tunerId=%d\n" , __FUNCTION__, __LINE__, id);
	if (NULL == ltsid)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "ltsid ptr Null\n");
		return RMF_QAMSRC_ERROR_EINVAL;
	}
	if ( id < RMFQAMSrcImpl::numberOfTuners )
	{
		*ltsid = ga_transport_path_info[id].ltsid;

		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "%s:%d tunerId = %d, ltsid = %d \n", 
				__FUNCTION__, __LINE__, id,  ga_transport_path_info[id].ltsid );
	}
	else
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "Invalid tuner Id %d \n", id);
		return RMF_QAMSRC_ERROR_EINVAL;
	}
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.QAMSRC", "Exit %s():%d \n" , __FUNCTION__, __LINE__);
	return RMF_SUCCESS;
}

/* Return transport path info for the given lts ID */
rmf_Error rmf_qamsrc_getTransportPathInfoForLTSID(int ltsid, rmf_transport_path_info *pTransportPathInfo)
{
	int tunerId;

	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.QAMSRC", "Enter %s():%d \n" , __FUNCTION__, __LINE__);
	if (NULL == pTransportPathInfo)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "pTransportPathInfo Null\n");
		return RMF_QAMSRC_ERROR_EINVAL;
	}
	for (tunerId =0; tunerId < RMFQAMSrcImpl::numberOfTuners; tunerId ++)
	{
		if (ltsid == ga_transport_path_info[tunerId].ltsid )
			break;
	}
	if ( tunerId < RMFQAMSrcImpl::numberOfTuners )
	{
		pTransportPathInfo->demux_handle = ga_transport_path_info[tunerId].demux_handle;
		pTransportPathInfo->ltsid= ga_transport_path_info[tunerId].ltsid;
		pTransportPathInfo->hSecContextFor3DES = ga_transport_path_info[tunerId].hSecContextFor3DES;
		pTransportPathInfo->tsd_key_location = ga_transport_path_info[tunerId].tsd_key_location;
	}
	else
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "TransportPathInfo not yet populated\n");
		return RMF_QAMSRC_ERROR_EINVAL;
	}
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.QAMSRC", "Exit %s():%d \n" , __FUNCTION__, __LINE__);

	return RMF_SUCCESS;
}

/* Function to process POD events */
void qamsrc_eventmonitor_thread(void* arg)
{
	rmf_Error ret;
	rmf_osal_event_handle_t event_handle;
	uint32_t event_type;
	rmf_osal_event_params_t eventData;
	rmf_osal_event_category_t eventCategory;
	qamsrcprivobjectlist *pqamsrcpriv = NULL;
	while (1)
	{
		ret = rmf_osal_eventqueue_get_next_event( g_eventq, 
				&event_handle, &eventCategory, &event_type, &eventData );
		if ( RMF_SUCCESS != ret)
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "rmf_osal_eventqueue_get_next_event failed\n" );
			break;
		}

		if ( RMF_OSAL_EVENT_CATEGORY_PODMGR == eventCategory )
		{
			if( RMF_PODMGR_EVENT_ENTITLEMENT == event_type )
			{
				uint32_t eidType = POD_EID_GENERAL;
				PODEntitlementAuthEvent *pEIDEvent = NULL;				
				pEIDEvent = ( PODEntitlementAuthEvent* )eventData.data;

				if( POD_EID_DISCONNECT == pEIDEvent->extendedInfo )
				{
					if( bQAMSrcEIDCheck )
					{
						if( pEIDEvent->isAuthorized  == 1)
						{						
							RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "RMF_QAMSRC_EVENT_EID_CONNECTED recvd \n");
							RMFQAMSrcImpl::sendPlatformStatusEvent( RMF_QAMSRC_EVENT_EID_CONNECTED );
						}
						else
						{
							RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "RMF_QAMSRC_EVENT_EID_DISCONNECTED recvd \n");						
							RMFQAMSrcImpl::sendPlatformStatusEvent( RMF_QAMSRC_EVENT_EID_DISCONNECTED );
						}
					}
					else
					{
						RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "RMF_QAMSRC_EVENT_EID_%s recvd, but dispatching is disabled \n",
							( pEIDEvent->isAuthorized  == 1) ? "CONNECTED": "DISCONNECTED" );
					}
				}
				else
				{
					RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "Received EID message from POD \n");	
				}
				rmf_osal_event_delete(event_handle);
				continue;
			}
			else if((RMF_PODMGR_EVENT_READY == event_type) || (RMF_PODMGR_EVENT_REMOVED == event_type) || (RMF_PODMGR_EVENT_CA_SYSTEM_READY == event_type))
			{
				/*Walk the list of qamsource objects and start/stop decryption as necessary.*/
				RMFQAMSrcImpl* privateImpl = NULL;
				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "POD event received 0x%x\n", event_type );
				rmf_osal_mutexAcquire(RMFQAMSrcImpl::mutex);
				pqamsrcpriv = g_qamsrcimpllist;
				while( pqamsrcpriv )
				{
					privateImpl = pqamsrcpriv->qamsrcimpl;
					if ( privateImpl )
					{
						if( event_type == RMF_PODMGR_EVENT_CA_SYSTEM_READY )
						{
							/* Below action will internaly trigger recovery from INIT/card reset events. */
							privateImpl->handleCASystemReady();
						}
						else if ( event_type == RMF_PODMGR_EVENT_READY)
						{
							privateImpl->handlePODInsertion();
						}
						else
						{
							privateImpl->handlePODRemoval();
						}
						privateImpl = NULL;
					}
					pqamsrcpriv = pqamsrcpriv->next;
				}
				rmf_osal_mutexRelease(RMFQAMSrcImpl::mutex);

			}
		}

		if ( RMF_OSAL_EVENT_CATEGORY_POD == eventCategory )
		{
			rmf_PodmgrDecryptSessionHandle sessionhandle;
			RMFQAMSrcImpl* privateImpl = NULL;

			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "POD event received 0x%x\n", event_type );

			sessionhandle = (rmf_PodmgrDecryptSessionHandle)eventData.data_extension;
			qamEventData_t *qamData = (qamEventData_t *)eventData.data;
			sessionhandle = qamData->handle;
			uint32_t qamContext = qamData->qamContext;
				
			rmf_osal_mutexAcquire(RMFQAMSrcImpl::mutex);

			pqamsrcpriv = g_qamsrcimpllist;
			while( pqamsrcpriv )
			{
				privateImpl = pqamsrcpriv->qamsrcimpl;
				if ( privateImpl )
				{
					if ( event_type != RMF_PODMGR_EVENT_REMOVED)
					{
						if ( /*sessionhandle == privateImpl->getPodSessionHandle() ||*/ qamContext == privateImpl->getQamContext())
						{
							break;
						}
					}
					else
					{
						RDK_LOG( RDK_LOG_INFO, 
							"LOG.RDK.QAMSRC", "%s : RMF_PODMGR_EVENT_REMOVED - session handle 0x%x. Source ID: %s\n",
							__FUNCTION__,  sessionhandle, privateImpl->saved_uri.c_str());
						privateImpl->setPODError(RMF_QAMSRC_EVENT_POD_REMOVED);
						privateImpl->setPodSessionHandle((rmf_PodmgrDecryptSessionHandle) NULL);
						privateImpl->notifyError(RMF_QAMSRC_EVENT_POD_REMOVED, NULL);
					}
					privateImpl = NULL;
				}
				pqamsrcpriv = pqamsrcpriv->next;
			}
			if (NULL == privateImpl)
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s: Couldnot find matching instance, qamhandle = %d\n", __FUNCTION__, qamContext );
				rmf_osal_mutexRelease(RMFQAMSrcImpl::mutex);
				rmf_osal_event_delete(event_handle);
				continue;
			}
			switch ( event_type )
			{

				case RMF_PODMGR_DECRYPT_EVENT_CCI_STATUS :
					{
						RDK_LOG( RDK_LOG_INFO, 
							"LOG.RDK.QAMSRC", "%s: RMF_PODMGR_DECRYPT_EVENT_CCI_STATUS - session handle 0x%x. Source ID: %s\n"
							,__FUNCTION__,  sessionhandle, privateImpl->saved_uri.c_str());
						privateImpl->sendCCIChangedEvent();
						break;
					}

				case RMF_PODMGR_DECRYPT_EVENT_CANNOT_DESCRAMBLE_ENTITLEMENT :
					{
						RDK_LOG( RDK_LOG_INFO, 
							"LOG.RDK.QAMSRC", "%s : RMF_PODMGR_DECRYPT_EVENT_CANNOT_DESCRAMBLE_ENTITLEMENT - session handle 0x%x. Source ID: %s\n",
							__FUNCTION__,  sessionhandle, privateImpl->saved_uri.c_str());
						privateImpl->setPODError(RMF_QAMSRC_EVENT_CANNOT_DESCRAMBLE_ENTITLEMENT);
						privateImpl->notifyAuthorizationStatus(FALSE);
						privateImpl->notifyError(RMF_QAMSRC_EVENT_CANNOT_DESCRAMBLE_ENTITLEMENT, NULL);
						break;
					}

				case RMF_PODMGR_DECRYPT_EVENT_CANNOT_DESCRAMBLE_RESOURCES:
					{
						RDK_LOG( RDK_LOG_INFO, 
							"LOG.RDK.QAMSRC", "%s : RMF_PODMGR_DECRYPT_EVENT_CANNOT_DESCRAMBLE_RESOURCES - session handle 0x%x. Source ID: %s\n",
							__FUNCTION__,  sessionhandle, privateImpl->saved_uri.c_str());
						privateImpl->setPODError(RMF_QAMSRC_EVENT_CANNOT_DESCRAMBLE_RESOURCES);
						privateImpl->notifyAuthorizationStatus(FALSE);
						privateImpl->notifyError(RMF_QAMSRC_EVENT_CANNOT_DESCRAMBLE_RESOURCES, NULL);
						break;
					}

				case RMF_PODMGR_DECRYPT_EVENT_MMI_PURCHASE_DIALOG:
					{
						RDK_LOG( RDK_LOG_INFO, 
							"LOG.RDK.QAMSRC", "%s : RMF_PODMGR_DECRYPT_EVENT_MMI_PURCHASE_DIALOG - session handle 0x%x. Source ID: %s\n"
							,__FUNCTION__,  sessionhandle, privateImpl->saved_uri.c_str());
						privateImpl->setPODError(RMF_QAMSRC_EVENT_MMI_PURCHASE_DIALOG);
						privateImpl->notifyAuthorizationStatus(FALSE);
						privateImpl->notifyError(RMF_QAMSRC_EVENT_MMI_PURCHASE_DIALOG, NULL);
						break;
					}

				case RMF_PODMGR_DECRYPT_EVENT_MMI_TECHNICAL_DIALOG:
					{
						RDK_LOG( RDK_LOG_INFO, 
							"LOG.RDK.QAMSRC", "%s : RMF_PODMGR_DECRYPT_EVENT_MMI_TECHNICAL_DIALOG - session handle 0x%x. Source ID: %s\n",
							__FUNCTION__,  sessionhandle, privateImpl->saved_uri.c_str());
						privateImpl->setPODError(RMF_QAMSRC_EVENT_MMI_TECHNICAL_DIALOG);
						privateImpl->notifyError(RMF_QAMSRC_EVENT_MMI_TECHNICAL_DIALOG, NULL);
						break;
					}

				case RMF_PODMGR_DECRYPT_EVENT_FULLY_AUTHORIZED:
					{
						RDK_LOG( RDK_LOG_INFO, 
							"LOG.RDK.QAMSRC", "%s : RMF_PODMGR_DECRYPT_EVENT_FULLY_AUTHORIZED - session handle 0x%x. Source ID: %s\n",
							__FUNCTION__,  sessionhandle, privateImpl->saved_uri.c_str());
						privateImpl->notifyAuthorizationStatus(TRUE);
						break;
					}

				case RMF_PODMGR_DECRYPT_EVENT_SESSION_SHUTDOWN:
					{
						RDK_LOG( RDK_LOG_INFO, 
							"LOG.RDK.QAMSRC", "%s : RMF_PODMGR_DECRYPT_EVENT_SESSION_SHUTDOWN - session handle 0x%x. Source ID: %s\n",
							__FUNCTION__,  sessionhandle, privateImpl->saved_uri.c_str());
						break;
					}

				case RMF_PODMGR_DECRYPT_EVENT_POD_REMOVED:
					{
						RDK_LOG( RDK_LOG_INFO, 
							"LOG.RDK.QAMSRC", "%s : RMF_PODMGR_DECRYPT_EVENT_POD_REMOVED - session handle 0x%x. Source ID: %s\n",
							__FUNCTION__,  sessionhandle, privateImpl->saved_uri.c_str());
						privateImpl->setPODError(RMF_QAMSRC_EVENT_POD_REMOVED);
						privateImpl->notifyError(RMF_QAMSRC_EVENT_POD_REMOVED, NULL);
						break;
					}

				case RMF_PODMGR_DECRYPT_EVENT_RESOURCE_LOST:
					{
						RDK_LOG( RDK_LOG_INFO, 
							"LOG.RDK.QAMSRC", "%s : RMF_PODMGR_DECRYPT_EVENT_RESOURCE_LOST - session handle 0x%x. Source ID: %s\n",
							__FUNCTION__,  sessionhandle, privateImpl->saved_uri.c_str());
						privateImpl->setPODError(RMF_QAMSRC_EVENT_POD_RESOURCE_LOST);
						privateImpl->notifyError(RMF_QAMSRC_EVENT_POD_RESOURCE_LOST, NULL);
						break;
					}

				default:
					{
						RDK_LOG( RDK_LOG_INFO, 
							"LOG.RDK.QAMSRC", "%s : POD event (0x%x) - session handle 0x%x\n", __FUNCTION__, event_type, sessionhandle );
						break;
					}
			}
			rmf_osal_mutexRelease(RMFQAMSrcImpl::mutex);
		}
		else if ( RMF_OSAL_EVENT_CATEGORY_DISCONNECTMGR == eventCategory )
		{
			if ( event_type == RMF_PODMGR_EVENT_EID_DISCONNECTED )
			{
				RMFQAMSrcImpl::sendPlatformStatusEvent( RMF_QAMSRC_EVENT_EID_DISCONNECTED);
			}
			else if ( event_type == RMF_PODMGR_EVENT_EID_CONNECTED )
			{
				RMFQAMSrcImpl::sendPlatformStatusEvent( RMF_QAMSRC_EVENT_EID_CONNECTED);
			}
		}
		rmf_osal_event_delete(event_handle);
	}
}

#endif /* TEST_WITH_PODMGR*/


#ifdef CAMGR_PRESENT
/* Return Local TS id for the given tuner ID */
rmf_Error rmf_qamsrc_getLtsid( int id, unsigned char *ltsid )
{
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.QAMSRC", "Enter %s():%d tunerId=%d\n" , __FUNCTION__, __LINE__, id);
	if (NULL == ltsid)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "ltsid ptr Null\n");
		return RMF_QAMSRC_ERROR_EINVAL;
	}
	if ( id < RMFQAMSrcImpl::numberOfTuners )
	{
		*ltsid = ga_transport_path_info[id].ltsid;

		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "%s:%d tunerId = %d, ltsid = %d \n",
			__FUNCTION__, __LINE__, id,  ga_transport_path_info[id].ltsid );
	}
	else
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "Invalid tuner Id %d \n", id);
		return RMF_QAMSRC_ERROR_EINVAL;
	}
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.QAMSRC", "Exit %s():%d \n" , __FUNCTION__, __LINE__);
	return RMF_SUCCESS;
}

/* Function to process NagraCAK events */
void qamsrc_eventmonitor_thread(void* arg)
{
	rmf_Error ret;
	rmf_osal_event_handle_t event_handle;
	uint32_t event_type;
	rmf_osal_event_params_t eventData;
	rmf_osal_event_category_t eventCategory;
	qamsrcprivobjectlist *pqamsrcpriv = NULL;
	while (1)
	{
		ret = rmf_osal_eventqueue_get_next_event( g_eventq,
			&event_handle, &eventCategory, &event_type, &eventData );
		if ( RMF_SUCCESS != ret)
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "rmf_osal_eventqueue_get_next_event failed\n" );
			break;
		}
		rmf_osal_event_delete(event_handle);
	}
}
#endif


/* Function to process Tune and SI events */
void qamsrc_priv_start_simonitor(void* arg)
{
	RMFQAMSrcImpl* qimpl = (RMFQAMSrcImpl*)arg;
	if (NULL != qimpl )
	{
		qimpl->simonitorThread();
	}
}

rmf_Error getTunerInfo(uint32_t tunerIndex, TunerData* pTunerData)
{
	int ret;
	struct timeval currtime;
	struct timezone currtz;
	diag_tuner_info_t* p_diag_info;

	if( false == bQAMSrcInited )
	{
		return RMF_RESULT_NOT_INITIALIZED;
	}
	
	if ( RMFQAMSrcImpl::numberOfTuners < tunerIndex )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s Unexpected tunerId %d \n", __FUNCTION__, tunerIndex );
		return -1;
	}

	if (NULL == pTunerData )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - Null Parameter\n", __FUNCTION__ );
		return -1;
	}
	p_diag_info = &ga_diag_info[tunerIndex];

	ret = gettimeofday(&currtime, &currtz);
	if(ret != 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "gettimeofday failed\n", __FUNCTION__ );
		return -1;
	}

	pTunerData->frequency= p_diag_info->frequency;
	pTunerData->modulation= p_diag_info->modulation;
	pTunerData->totalTuneCount = p_diag_info->totalTuneCount;
	pTunerData->tuneFailCount = p_diag_info->tuneFailCount;
	pTunerData->lastTuneFailFreq = p_diag_info->lastTuneFailFreq;
	pTunerData->carrierLockLostCount = p_diag_info->carrierLockLostCount;

	if ( 0 != p_diag_info->lockedTimeStamp)
	{
		pTunerData->lockedTimeInSecs = currtime.tv_sec - p_diag_info->lockedTimeStamp;
	}
	else
	{
		pTunerData->lockedTimeInSecs = 0;
	}

        pTunerData->tunerState = p_diag_info->tunerState;
	return RMF_SUCCESS;
}

uint32_t getTSID( uint32_t tunerIndex)
{
	unsigned char ltsId = 0;
#if defined(TEST_WITH_PODMGR) || defined(CAMGR_PRESENT)
	rmf_qamsrc_getLtsid( tunerIndex, &ltsId);
#endif
	return ltsId;
}

uint32_t getTunerCount()
{
	uint32_t tunerCount = 0;
	if (0 == RMFQAMSrcImpl::numberOfTuners)
	{
		tunerCount = QAMSRC_DEF_NUMBER_OF_TUNERS;
	}
	else
	{
		tunerCount = RMFQAMSrcImpl::numberOfTuners;
	}
	return  tunerCount;
}

void qamsrc_update_diag_info( GstElement* src_element)
{
	uint32_t tunerId = 0xFF;
	diag_tuner_info_t* p_diag_info;
	g_object_get( src_element, "tunerid", &tunerId, NULL );
	if ( RMFQAMSrcImpl::numberOfTuners < tunerId )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s Unexpected tunerId %d \n", __FUNCTION__, tunerId );
		return;
	}
	p_diag_info = &ga_diag_info[tunerId];

	g_object_get( src_element, 
			"tunerstate", &p_diag_info->tunerState,
			"totaltunecount", &p_diag_info->totalTuneCount,
			"tunefailcount", &p_diag_info->tuneFailCount,
			"lockedtimestamp", &p_diag_info->lockedTimeStamp,
			"locklostcount", &p_diag_info->carrierLockLostCount,
			NULL);
}

rmf_Error getProgramInfo(uint32_t srcId, ProgramInfo* pProgramInfo)
{
	qamsrcprivobjectlist *pqamsrcpriv = NULL;
	RMFQAMSrcImpl* privateImpl = NULL;
	rmf_Error ret = RMF_SUCCESS;
	char cci = 4; //undefined

	rmf_osal_mutexAcquire( RMFQAMSrcImpl::mutex);
	pqamsrcpriv = g_qamsrcimpllist;
	while( pqamsrcpriv )
	{
		privateImpl = pqamsrcpriv->qamsrcimpl;
		if ( privateImpl )
		{
			if ( privateImpl->getSourceId() == srcId )
			{
				break;
			}
		}
		pqamsrcpriv = pqamsrcpriv->next;
	}
	if ( NULL != pqamsrcpriv )
	{

#ifdef TEST_WITH_PODMGR
		if ( RMF_RESULT_SUCCESS != privateImpl->getCCIInfo(cci))
		{
			cci = 4; //undefined
		}
#endif
		const rmf_MediaInfo* pMediaInfo = privateImpl->getInbandMediaInfo();
		if ( NULL != pMediaInfo )
		{
			pProgramInfo->pmtPid = pMediaInfo->pmt_pid;
			pProgramInfo->pcrPid = pMediaInfo->pcrPid;
			if (pMediaInfo->numMediaPids < RMF_MAX_MEDIAPIDS_PER_PGM)
			{
				pProgramInfo->numMediaPids = pMediaInfo->numMediaPids;
			}
			else
			{
				pProgramInfo->numMediaPids = RMF_MAX_MEDIAPIDS_PER_PGM;
			}
			for ( int i =0; i < pProgramInfo->numMediaPids; i++)
			{
				pProgramInfo->pPidList[i].pid =  pMediaInfo->pPidList[i].pid;
				pProgramInfo->pPidList[i].streamType=  pMediaInfo->pPidList[i].pidType;
			}
		}
		pProgramInfo->cci = (uint8_t)cci;
	}
	else
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s QAMSrc instance with srcId %d not available\n", __FUNCTION__, srcId );
		ret = -1;
	}
	rmf_osal_mutexRelease( RMFQAMSrcImpl::mutex);
	return ret;
}

void RMFQAMSrcImpl::setPatTimeoutOffset(uint32_t pat_timeout_offset)
{
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s Setting offset value %d\n", __FUNCTION__, pat_timeout_offset);	
	m_pat_timeout_offset = pat_timeout_offset ;
}

uint32_t RMFQAMSrcImpl::getTunerId()
{
    return m_sourceInfo.tunerId;
}

RMFResult RMFQAMSrcImpl::getVideoContentInfo(unsigned int iContent, struct VideoContentInfo * pVideoContentInfo)
{
    if(NULL != pVideoContentInfo)
    {
        int iPid = 0;
        int nVideo = 0;
        int nAudio = 0;
        unsigned int vPid = 0;
        unsigned int aPid = 0;
        VideoContainerFormat containerFormat = VideoContainerFormat_mpeg2;
        
        for(iPid = 0; iPid < mediaInfo.numMediaPids; iPid++)
        {
            switch(mediaInfo.pPidList[iPid].pidType)
            {
                case RMF_SI_ELEM_MPEG_1_VIDEO:
                case RMF_SI_ELEM_MPEG_2_VIDEO:
                case RMF_SI_ELEM_ISO_14496_VISUAL:
                case RMF_SI_ELEM_AVC_VIDEO:
                case RMF_SI_ELEM_VIDEO_DCII:
                {
                    if(0 == vPid)
                    {
                        vPid = mediaInfo.pPidList[iPid].pid;
                    }
                    nVideo++;
                }
                break;
    
                case RMF_SI_ELEM_MPEG_1_AUDIO:
                case RMF_SI_ELEM_MPEG_2_AUDIO:
                case RMF_SI_ELEM_AAC_ADTS_AUDIO:
                case RMF_SI_ELEM_AAC_AUDIO_LATM:
                case RMF_SI_ELEM_ATSC_AUDIO:
                case RMF_SI_ELEM_ENHANCED_ATSC_AUDIO:
                {
                    if(0 == aPid)
                    {
                        aPid = mediaInfo.pPidList[iPid].pid;
                    }
                    nAudio++;
                }
                break;
                
                default:
                {
                    
                }
                break;
            }
            
            // container format
            switch(mediaInfo.pPidList[iPid].pidType)
            {
                case RMF_SI_ELEM_AVC_VIDEO:
                {
                    containerFormat = VideoContainerFormat_mpeg4;
                }
                break;

                case RMF_SI_ELEM_HEVC_VIDEO:
                {
                    containerFormat = VideoContainerFormat_mpegh;
                }
                break;

                default:
                {
                }
                break;
            }
        char cci = 4; //undefined
#ifdef TEST_WITH_PODMGR
		if ( RMF_RESULT_SUCCESS != getCCIInfo(cci))
		{
			cci = 4; //undefined
		}
#endif
            unsigned int transportStreamId = 0;
            if(NULL != pInbandSI) pInbandSI->GetTSID(&transportStreamId);
            
            TunerData tunerData = {0};
            getTunerInfo(m_sourceInfo.tunerId, &tunerData);

            pVideoContentInfo->ContentIndex         = iContent+1;
            pVideoContentInfo->TunerIndex           = m_sourceInfo.tunerId;
            pVideoContentInfo->ProgramNumber        = pInfo.prog_num;
            pVideoContentInfo->TransportStreamID    = transportStreamId;
            pVideoContentInfo->TotalStreams         = mediaInfo.numMediaPids;
            pVideoContentInfo->SelectedVideoPID     = vPid;
            pVideoContentInfo->SelectedAudioPID     = aPid;
            pVideoContentInfo->OtherAudioPIDs       = ((nAudio>1)?(1):2);
            pVideoContentInfo->CCIValue             = (((cci<0)||(cci>4))?(4):(cci));
            pVideoContentInfo->APSValue             = 1;
            pVideoContentInfo->CITStatus            = 1;
            pVideoContentInfo->BroadcastFlagStatus  = 1;
            pVideoContentInfo->EPNStatus            = 1;
            pVideoContentInfo->PCRPID               = mediaInfo.pcrPid;
            pVideoContentInfo->PCRLockStatus        = ((TUNER_STATE_TUNED_SYNC == tunerData.tunerState)?(2):1); //  for now masquerade tuner lock as PCR lock.
            pVideoContentInfo->DecoderPTS           = 0;
            pVideoContentInfo->Discontinuities      = 0;
            pVideoContentInfo->PktErrors            = 0;
            pVideoContentInfo->PipelineErrors       = 0;
            pVideoContentInfo->DecoderRestarts      = 0;
            pVideoContentInfo->containerFormat      = containerFormat;
        }
    }
    else
    {
        return RMF_QAMSRC_ERROR_EINVAL;
    }
    
    return RMF_SUCCESS;
}

rmf_InbSiMgr* RMFQAMSrcImpl::createInbandSiManager()
{
    return new rmf_InbSiMgr(m_sourceInfo);
}

rmf_osal_Bool RMFQAMSrcImpl::setErrorBits(uint32_t bitmask, rmf_osal_Bool value)
{
	rmf_osal_Bool transition_detected = false;
	if(true == value)
	{
		if(0 == m_error_bitmap)         
		{
			/*We're going from no errors to error-state*/ 
			transition_detected = true;    
		}
		m_error_bitmap |= bitmask; 
	}
	else
	{   
		uint32_t previous_bitmap_state = m_error_bitmap; 
		m_error_bitmap &= ~bitmask; 
		if((0 != previous_bitmap_state) && (0 == m_error_bitmap))
		{
			/*We're going from error-state to no errors.*/
			transition_detected = true;    
		}
	}
	return transition_detected;
}

void RMFQAMSrcImpl::resetSptsTimer()
{
	g_object_set(src_element, "resetSptsTimer", TRUE, NULL);
}
timekeeper::timekeeper()
{
	reset();
}

void timekeeper::reset()
{
	for(int i = 0; i < STAGES_MAX; i++)
	{
		m_timestamps[i] = 0;
	}
}

void timekeeper::log_time(timekeeper_stages_t stage)
{
	rmf_osal_TimeMillis current_time = 0;
	rmf_Error ret = rmf_osal_timeGetMillis(&current_time);

	if(0 != m_timestamps[stage]) 
	{
		/*Reuse scenario. Set all previous stages to current timestamp and future stages to zero.*/
		reset();
		for(int i = 0; i < stage; i++)
		{
			m_timestamps[i] = current_time;
		}
	}
	m_timestamps[stage] = current_time;
}

void timekeeper::print_stats(const char * identifier)
{
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s QAMSRC_TUNE_TIME:" \
			"%lld,%lld,%lld,%lld\n", identifier,
			(m_timestamps[CA_COMPLETE] - m_timestamps[TUNE_START]),
			(m_timestamps[TUNE_END] - m_timestamps[TUNE_START]),
			(m_timestamps[PMT_ACQ] - m_timestamps[TUNE_END]),
			(m_timestamps[CA_COMPLETE] - m_timestamps[CA_START])
		   );
}
RMFResult RMFQAMSrcImpl::getStatus(qamsrc_status_t &status)
{
	rmf_osal_mutexAcquire(m_mutex);
	status.is_locked = m_is_tuner_locked;
	status.pat_acquired = pat_available;
	status.pmt_acquired = pmtEventSent;
	rmf_osal_mutexRelease(m_mutex);
	return RMF_RESULT_SUCCESS;
}

bool RMFQAMSrcImpl::getAudioPidFromPMT(uint32_t *pid, const std::string& audioLang)
{
        RDK_LOG( RDK_LOG_INFO,"LOG.RDK.QAMSRC", "Enter %s():%d \n" , __FUNCTION__, __LINE__);
       rmf_Error ret;
       *pid = -1;
        rmf_SiElementaryStreamList *pESList;
        rmf_SiMpeg2DescriptorList *pMPEGDescList;

        ret = pInbandSI->GetDescriptors(&pESList, &pMPEGDescList);
       if(RMF_SUCCESS != ret )
       {
           RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.QAMSRC", "getAudioPidFromPMT - return false");
           return false;
       }
       while(NULL != pESList)
        {
            if(isAudioStream(pESList->elementary_stream_type))
            {
               RDK_LOG( RDK_LOG_INFO,"LOG.RDK.QAMSRC", "getAudioPidFromPMT - pid - %p, associated_language - %s, pESList->elementary_stream_type - 0x%2X, currentLang - %s\n",pESList->elementary_PID,pESList->associated_language, pESList->elementary_stream_type, audioLang.c_str());
               if(audioLang.empty())
               {
                   *pid = pESList->elementary_PID;
                   RDK_LOG( RDK_LOG_INFO,"LOG.RDK.QAMSRC", "getAudioPidFromPMT audioLang-empty returning first pid - %p\n", pESList->elementary_PID);
                   return true;
               }
               if (strcasecmp(audioLang.c_str(), pESList->associated_language) == 0)
                {
                    *pid = pESList->elementary_PID;
                   RDK_LOG( RDK_LOG_INFO,"LOG.RDK.QAMSRC", "getAudioPidFromPMT return pESList->elementary_PID - %p\n", pESList->elementary_PID);
                   return true;
                }
            }
            pESList = pESList->next;
        }
       return false;
}

bool RMFQAMSrcImpl::isAudioStream(rmf_SiElemStreamType es_type)
{
        RDK_LOG( RDK_LOG_INFO,"LOG.RDK.QAMSRC", "Enter %s():%d \n" , __FUNCTION__, __LINE__);
        if ((es_type == 0x03)   /* MPEG1 Audio */ ||
        (es_type == 0x04)   /* MPEG2 Audio */ ||
        (es_type == 0x0f)   /* AAC Audio   */ ||
        (es_type == 0x81)   /* AC3 Audio   */ ||
        (es_type == 0x87)   /* EAC3 Audio  */ ||
        (es_type == 0x8A)   /* DTS Audio   */ )
        {
            RDK_LOG( RDK_LOG_INFO,"LOG.RDK.QAMSRC", "The stream_type is defined to 0x%2X which is audio type that this STB supports\n", es_type);
            return TRUE;
        }

       /* This will be executed only if it is not a audio type */
       return FALSE;
}
