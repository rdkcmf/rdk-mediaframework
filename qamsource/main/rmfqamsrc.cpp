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

/**
 * @file rmfqamsrc.cpp
 * @brief Push based source element and manages the tuner and demux.
 * Resposible for tuning to a frequency .
 */ 

#include <rmfqamsrc.h>
#include <rmfqamsrcpriv.h>

#ifdef USE_SDVSRC
#include <rmfsdvsource.h>
#endif

#ifdef USE_DVBSRC
#include <rmfdvbsrc.h>
#endif

#include <rmf_osal_sync.h>
#include <rmf_osal_mem.h>
#include <rdk_debug.h>

#include <typeinfo>

#include <stdlib.h>
#include <string.h>

#include "rmf_qamsrc_common.h"
#include "libIBus.h"
#include "sysMgr.h"


#if USE_SYSRES_MLT
#include "rpl_new.h"
#include "rpl_malloc.h"
#endif

#ifdef USE_TRM
/* Start of changes for TRM-pretune integration. */
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <errno.h>
#include "rmf_osal_thread.h"
#include "rmf_osal_util.h"
#include "rdk_debug.h"
#include "rmf_osal_sync.h"
#include <map>
#include <string>
#include <uuid/uuid.h>
#include<string>
#include <list>
#include <vector>
#include <glib.h>
#include <unistd.h>
#include <uuid/uuid.h>
#include <stdio.h>

#include "trm/Messages.h"
#include "trm/MessageProcessor.h"
#include "trm/Activity.h"
#include "trm/JsonEncoder.h"
#include "trm/JsonDecoder.h"
/* End of changes for TRM-pretune integration. */
#endif

#define QAM_MAX_URI_SIZE 128
#define UPDATE_ERROR_CODE(var, code) do{ if(var) *var = (code); }while(0);
#define MAX_PAYLOAD_LEN 32767

struct qamsrcobjectlist
{
	char* uri;
	RMFQAMSrc* qamsrc;
	int refcount;
	uint32_t frequency;
	struct qamsrcobjectlist* next;
};

#ifdef USE_TRM
/* Start of changes for TRM-pretune integration. */
#define PRETUNE_DUMMY_TOKEN "pretune_dummy_token"
enum Type {
	REQUEST = 0x1234,
	RESPONSE = 0x1800,
	NOTIFICATION = 0x1400,
	UNKNOWN,
};

class QAMSrcMessageProcessor: public TRM::MessageProcessor
{
public :
	void operator() (const TRM:: NotifyTunerPretune &msg) ;
	void operator() (const TRM::UpdateTunerActivityStatus &msg);
	void GetVersion(){}
};
static int is_connected = 0;
static rmf_osal_Mutex g_mutex = 0;
static int trm_socket_fd = -1;
static const char* ip ="127.0.0.1";
static int  port = 9987;
static void QamsrcTunerReservationHelperThread (void* arg);

/* End of changes for TRM-pretune integration. */
#endif

static qamsrcobjectlist *g_qamsrclist = NULL;
static qamsrcobjectlist *g_qamsrc_list_last_entry = NULL;
static uint32_t g_qamsrcobjectno = 0;
static uint32_t g_lowlevelelementsno = 0;
static rmf_osal_Mutex g_qamlistmutex = NULL;
std::map <RMFQAMSrc::usage_t, RMFQAMSrc::QamsrcReleaseCallback_t> RMFQAMSrc::releaseCallbackList;
bool RMFQAMSrc::livePreemptionEnabled = FALSE;
RMFQAMSrc::UpdateTunerActivityCallback_t RMFQAMSrc::tunerActivityUpdateCallback = NULL;

/* The below table is maintained in the decreasing order of priority */
static uint32_t usage_priority_table[][2] = {
	{RMFQAMSrc::USAGE_PODMGR, 6},
	{RMFQAMSrc::USAGE_LIVE, 5},
	{RMFQAMSrc::USAGE_RECORD, 5},
	{RMFQAMSrc::USAGE_RECORD_UNRESERVED, 4},
	{RMFQAMSrc::USAGE_LIVE_UNRESERVED, 4},
	{RMFQAMSrc::USAGE_TSB, 3},
	{RMFQAMSrc::USAGE_AD, 2},
	{RMFQAMSrc::USAGE_PRETUNE, 1},
	{RMFQAMSrc::USAGE_END_MARKER, 0}
	};

#ifdef USE_TRM
rmf_osal_eventqueue_handle_t RMFQAMSrc::pretuneEventQHandle = 0;
bool RMFQAMSrc::enablePretune = FALSE;
#endif

qamsrcobjectlist* qam_factory_get_entry(RMFQAMSrc * qamsource);
rmf_osal_Bool check_for_free_qamsrc();


static void log_tuner_usage()
{
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "Enter %s():%d \n" , __FUNCTION__, __LINE__);
	qamsrcobjectlist* qamlistobj = g_qamsrclist;
	while(NULL != qamlistobj)
	{
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", 
				"qamsrc 0x%x, references: %d, URI: %s\n" , qamlistobj->qamsrc, qamlistobj->refcount, qamlistobj->uri);
		qamlistobj = qamlistobj->next;
	}
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "Total tuners: %d. Factory-managed: %d. Non-factory-managed: %d\n" , RMFQAMSrcImpl::numberOfTuners,
			g_qamsrcobjectno, g_lowlevelelementsno);
}

bool RMFQAMSrc::useFactory = false;

RMFQAMSrc::RMFQAMSrc()
{
	RDK_LOG(RDK_LOG_TRACE1,"LOG.RDK.QAMSRC", " %s():%d \n" , __FUNCTION__, __LINE__);
	if(RMF_SUCCESS != rmf_osal_mutexNew(&m_mutex))
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - rmf_osal_mutexNew failed.\n", __FUNCTION__);
	}
	factoryManaged = 0;  
}

RMFQAMSrc::~RMFQAMSrc()
{
	RDK_LOG(RDK_LOG_TRACE1,"LOG.RDK.QAMSRC", " %s():%d \n" , __FUNCTION__, __LINE__);
	rmf_osal_mutexDelete(m_mutex);
}


/**
 * @fn RMFQAMSrc::init_platform()
 * @brief This function is used to initialize the RMFQAMSrc platform functionality.
 *
 * @return RMFResult (int)
 * @retval RMF_RESULT_SUCCESS If the platform initialized successfully.
 * @retval RMF_RESULT_FAILURE If it is failed to allocate resources for QAMSrc or PODMgr initialization failed.
 * @retval RMF_RESULT_INTERNAL_ERROR If SDV Sources couldnot be able to register itself for force tune events handler.
 * @addtogroup qamsrcapi
 */
RMFResult RMFQAMSrc::init_platform()
{
	const char *useFactoryStr = NULL;
	rmf_Error rmf_error;
	RMFResult result = RMF_RESULT_SUCCESS;

#ifdef USE_TRM
	rmf_osal_ThreadId thid = 0;
#endif

	useFactoryStr = rmf_osal_envGet("QAMSRC.FACTORY.ENABLED");
	if ( (useFactoryStr != NULL)
			&& (strcmp(useFactoryStr, "TRUE") == 0))
	{
		useFactory = TRUE;
		RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", " %s():%d Factory is enabled in configuration\n"
				, __FUNCTION__, __LINE__);
	}
	else
	{
		useFactory = FALSE;
		RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", " %s():%d Factory is disabled in configuration\n"
				, __FUNCTION__, __LINE__);
	}

#ifdef USE_SDVSRC
	RMFSDVSrc::init_platform();
#endif
	result = RMFQAMSrcImpl::init_platform();

	rmf_error = rmf_osal_mutexNew(&g_qamlistmutex);
	if (RMF_SUCCESS != rmf_error )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - rmf_osal_mutexNew failed.\n", __FUNCTION__);
		return RMF_RESULT_FAILURE;
	}

#ifdef USE_TRM
	const char* enablePretuneStr = rmf_osal_envGet("QAMSRC.TRM.PRETUNE.ENABLED");
	if ( (enablePretuneStr != NULL)
			&& (strcmp(enablePretuneStr, "TRUE") == 0))
	{
		enablePretune = TRUE;
		
		RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", " %s():%d TRM pretune is enabled.\n"
				, __FUNCTION__, __LINE__);
		
		if(RMF_SUCCESS != rmf_osal_threadCreate( pretune_enabler_thread, NULL, 
		RMF_OSAL_THREAD_PRIOR_DFLT, RMF_OSAL_THREAD_STACK_SIZE, 
		&thid, "qamsrc_pretuner") )
		{
			RDK_LOG(RDK_LOG_WARN, 
				"LOG.RDK.QAMSRC", "%s(): couldn't create pretuner thread. Pre-tune feature won't work.\n", 
				__FUNCTION__);
		}
	}
	else
	{
		enablePretune = FALSE;
		RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", " %s():%d TRM pretune is disabled.\n"
				, __FUNCTION__, __LINE__);
	}

#endif
	const char* liveTunerPreemption = rmf_osal_envGet("QAMSRC.LIVE.TUNER.PREEMPTABLE");
	if ((liveTunerPreemption != NULL)
			&& (strcmp(liveTunerPreemption, "TRUE") == 0))
	{
		livePreemptionEnabled = TRUE;
		RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", " %s(): Live tuner pre-emption enabled.\n",
			 __FUNCTION__);
	}
	else
	{
		RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", " %s(): Live tuner pre-emption disabled.\n",
			__FUNCTION__);
	}
#ifndef USE_SDVSRC //Issue QAM READY right away if we have no SDV. All dependencies are met.
	notifyQAMReady();
#endif
	return result;
}


/**
 * @brief This function is used to uninitialize the RMFQAMSrc platform functionality.
 *
 * @return RMFResult (int)
 * @retval RMF_RESULT_SUCCESS Successfully uninitialized the platform functionality.
 * @retval RMF_RESULT_INTERNAL_ERROR If SDV Sources couldnot be able to unregister itself for force tune events handler.
 * @ingroup qamsrcapi
 */
RMFResult RMFQAMSrc::uninit_platform()
{
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s starts.\n", __FUNCTION__);
	freeCachedQAMSourceInstances();
#ifdef USE_SDVSRC
    RMFSDVSrc::uninit_platform();
#endif   
	RMFQAMSrcImpl::uninit_platform();
	return RMF_RESULT_SUCCESS;

}

/**
 * @brief Listen to the events which are not specific to qamsrc instances.
 * Using insert modifiers, it inserts the events in the beginning of the queue by using begin iterator it fetches the event.
 *
 * @param[in] events  Pointer to event object
 *
 * @return RMFResult
 * @ingroup qamsrcapi
 */
RMFResult RMFQAMSrc::addPlatformEventHandler(IRMFMediaEvents* events)
{
	return RMFQAMSrcImpl::addPlatformEventHandler(events);
}

/**
 * @brief Removes event handler added using addPlatformEventHandler.
 *
 * @param[in] events  Pointer to event object to be removed.
 *
 * @return RMFResult
 * @ingroup qamsrcapi
 */
RMFResult RMFQAMSrc::removePlatformEventHandler(IRMFMediaEvents* events)
{
	return RMFQAMSrcImpl::removePlatformEventHandler(events);
}

/**
 * @brief This function is used to check whether the qamsource is EID authorized or not.
 *
 * @param[in] pAuthSatus  Pointer to get Authorization is through or not
 *
 * @return RMFResult
 * @retval RMF_RESULT_SUCCESS  If successfully got EID status from POD or successfully read the environment
 * of QAMSRC.DISABLE.EID from rmfconfig.ini file.
 * @retval RMF_RESULT_FAILURE  If failed to get EID status from POD.
 * @retval RMF_RESULT_NOT_INITIALIZED If EID logic is not initialized.
 * @ingroup qamsrcapi
 */
RMFResult RMFQAMSrc::isEIDAuthorised ( bool *pAuthSatus )
{
	return RMFQAMSrcImpl::isEIDAuthorised( pAuthSatus );
}

/**
 * @brief This API returns the factory method if it is used or not. 
 * All qamsource objects created using getQAMSourceInstance() function are factory-managed
 *
 * @return useFactory
 * @retval true Indicates Qamsrc factory is enabled in the environment
 * @retval false Indicates Qamsrc factory is disabled in the environment
 * @ingroup qamsrcapi
 */
bool RMFQAMSrc::useFactoryMethods()
{
	return useFactory;
}

void RMFQAMSrc::registerQamsrcReleaseCallback(RMFQAMSrc::QamsrcReleaseCallback_t callback)
{
	//This API is now deprecated.
	RDK_LOG( RDK_LOG_WARN, "LOG.RDK.QAMSRC", "%s: this version of API is deprecated.\n", __FUNCTION__);
}

void RMFQAMSrc::registerUpdateTunerActivityCallback(RMFQAMSrc::UpdateTunerActivityCallback_t callback)
{
	RDK_LOG( RDK_LOG_WARN, "LOG.RDK.QAMSRC", "%s: Registered Tuner Activity update callback\n", __FUNCTION__);
        tunerActivityUpdateCallback = callback;
}

void RMFQAMSrc::registerQamsrcReleaseCallback(usage_t usage, QamsrcReleaseCallback_t callback)
{
	rmf_osal_mutexAcquire(g_qamlistmutex);
	releaseCallbackList[usage] = callback;
	
	/*QAMSource objects with usages of type USAGE_X_UNRESERVED (where X is LIVE/RECORD etc) are 
	 * released by the same entities that release the USAGE_X usage type. But the applications
	 * won't register explicitly for these deprioritized usages. We'll do it for them.*/
	switch(usage)
	{
		case USAGE_LIVE:
			releaseCallbackList[USAGE_LIVE_UNRESERVED] = callback;
			break;
		case USAGE_RECORD:
			releaseCallbackList[USAGE_RECORD_UNRESERVED] = callback;
			break;
		default:
			break;
	}
	rmf_osal_mutexRelease(g_qamlistmutex);
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s: executed for type %d.\n", __FUNCTION__, usage);
}

#ifdef USE_TRM
void RMFQAMSrc::pretune_enabler_thread(void* arg)
{
	rmf_Error ret;
	rmf_osal_event_handle_t event_handle;
	uint32_t event_type;
	rmf_osal_event_params_t	eventData;	
	uint8_t keepRunning = TRUE;
	RMFQAMSrc* qamsrc = NULL;
	IRMFMediaEvents* handler = NULL;
	rmf_osal_ThreadId threadId = 0;
	int existingReferences = 0;
	std::string token = PRETUNE_DUMMY_TOKEN;
	
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s starts.\n", __FUNCTION__);
	
	ret = rmf_osal_eventqueue_create( (const uint8_t*) "pretuneQ", &pretuneEventQHandle);
	if (RMF_SUCCESS != ret )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - rmf_osal_eventqueue_create failed.\n", __FUNCTION__);
		return;
	}
	setlinebuf(stderr);
	rmf_osal_threadCreate( QamsrcTunerReservationHelperThread, NULL,
								RMF_OSAL_THREAD_PRIOR_DFLT, RMF_OSAL_THREAD_STACK_SIZE,
								&threadId,"QAMSrcTunerRes" );

	while (keepRunning)
	{
		ret = rmf_osal_eventqueue_get_next_event(pretuneEventQHandle, 
					&event_handle, NULL, &event_type, &eventData );

		if(RMF_SUCCESS != ret)
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - queue returned %d.\n",
				__FUNCTION__, ret);
			continue;
		}
		
		if(RMF_QAMSRC_PRETUNE_REQUEST == event_type)
		{
			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "%s: Processing pre-tune request.\n",
				__FUNCTION__);
			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "%s: URI: %s.\n",
				__FUNCTION__, (char *)(eventData.data_extension));

#ifdef USE_SDVSRC
		if (RMFSDVSrc::isSwitched((char *)(eventData.data_extension))) {
			   RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s: tuner is for sdv. Skip pre-tune.\n", __FUNCTION__);
			   goto SKIP;
	   }
#endif
			/* Check whether any cached instances exist. */
			existingReferences = 0;
			rmf_osal_mutexAcquire(g_qamlistmutex);
			qamsrc =  getExistingInstance((char *)(eventData.data_extension),FALSE, &existingReferences);

			if(qamsrc)
			{
				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s: tuner is already hot.\n",
				__FUNCTION__);
				if(existingReferences)
				{
					RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s: skip pretune.\n",
						__FUNCTION__);
				}
				else
				{
					RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s:  start CA session.\n",
						__FUNCTION__);
					((RMFQAMSrcImpl *)(qamsrc->getPrivateSourceImpl()))->setupDecryption();
				}
				rmf_osal_mutexRelease(g_qamlistmutex);
				goto SKIP;
			}
			rmf_osal_mutexRelease(g_qamlistmutex);
			


			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s: attempting pre-tune.\n",
				__FUNCTION__);
			qamsrc = getQAMSourceInstance((char *)(eventData.data_extension), USAGE_PRETUNE, token);
			if(qamsrc)
			{
				/* Add the pretune event handler and let it be. */
				handler = new RMFPretuneEventHandler(pretuneEventQHandle, qamsrc);
				if(NULL== handler)
				{
					RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s: Couldn't create handler. Aborting pre-tune.\n",
						__FUNCTION__);
					freeQAMSourceInstance(qamsrc, USAGE_LIVE, token);
				}
				else
				{
					qamsrc->addEventHandler(handler);
					RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "%s: Installed handler 0x%x in qamsrc 0x%x.\n",
						__FUNCTION__, handler, qamsrc);
				}		
			}
			else
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s: Pre-tune failed. No qamsrc available.\n",
					__FUNCTION__);
			}
SKIP:
			rmf_osal_memFreeP ( RMF_OSAL_MEM_MEDIA, (void*)eventData.data_extension );
		}
		else if( RMF_QAMSRC_PRETUNE_RELEASE == event_type)
		{
			rmf_pretune_event_payload_t * payload = (rmf_pretune_event_payload_t *)eventData.data;
			handler = payload->handler;
			qamsrc = payload->source;
			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "%s: Removing handler 0x%x from qamsrc 0x%x.\n",
						__FUNCTION__, handler, qamsrc);
			qamsrc->removeEventHandler(handler);
			delete handler;
			handler = NULL;
			
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s: Releasing pretune referece to qamsrc object 0x%x.\n",
						__FUNCTION__, qamsrc);
			freeQAMSourceInstance(qamsrc, USAGE_PRETUNE, token);
			qamsrc = NULL;
			delete payload;
		}
		else if( RMF_QAMSRC_PRETUNE_SHUTDOWN == event_type)
		{
			keepRunning = FALSE;
		}
		rmf_osal_event_delete(event_handle);
	}

	rmf_osal_eventqueue_clear(pretuneEventQHandle);
	rmf_osal_eventqueue_delete(pretuneEventQHandle);
	pretuneEventQHandle = 0;
	
}

#ifdef USE_TRM
/**
 * @brief This function is used to perform the pretune features.
 * While switched from one channel to other favorite channel, pretune feature will be used to increase the channel change speed.
 * The pretune feature enables TRM to request a tuner for a channel through back-door.
 * QAMSource responds to pretune request by fetching a spare tuner, tuning to that channel and completing PSI acquisition.
 * It also kick-starts CA descrambling. When the actual tune request from application arrives later,
 * it finds a qamsource object ready to stream data and saves the time it takes to tune and set up the filters.
 *
 * @param[in] uri  Source Id in the format ocap://0x\<source Id in hex\>.
 *
 * @return None.
 * @ingroup qamsrcapi
 */
void RMFQAMSrc::performPretune( const char * uri )
{
	if(!enablePretune)
	{
		return;
	}

	rmf_Error ret;
	rmf_osal_event_handle_t event_handle;
	char * source = NULL;
	uint8_t uriLength = strlen(uri) + 1;

	ret =  rmf_osal_memAllocP (RMF_OSAL_MEM_MEDIA, uriLength, (void**)&source);
	if (RMF_SUCCESS != ret )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - rmf_osal_memAllocP failed. Size: %d. Returned 0x%x\n",
			__FUNCTION__, strlen(uri), ret);
		return;
	}
	strncpy(source, uri, uriLength);

	rmf_osal_event_params_t p_event_params = {	1, 
		NULL,
		(uint32_t)source,
		NULL
		};
	
	ret = rmf_osal_event_create(RMF_OSAL_EVENT_CATEGORY_PRETUNE, RMF_QAMSRC_PRETUNE_REQUEST, &p_event_params, &event_handle);
	if(ret!= RMF_SUCCESS)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "Could not create event handle:\n");
	}

	ret =  rmf_osal_eventqueue_add(pretuneEventQHandle, event_handle);
	if(ret!= RMF_SUCCESS)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "rmf_osal_eventqueue_add failed\n");
	}
#endif
}
#endif


/**
 * @brief This API get's low level element/GElement from qamtunersrc factory. It is used with VOD and IPPV.
 * If all qamsrc instances are used then it searches for the unused instance and frees it. And then creates
 * a low level element/GElement.
 *
 * @return pointer to low level element/GElement
 * @retval lowLevelElement Typecasted pointer of low level element
 * @retval NULL insufficient resources or failure.
 * @ingroup qamsrcapi
 */
void* RMFQAMSrc::getLowLevelElement()
{
	int used_qamsrc_elements = 0;
	qamsrcobjectlist* qamlistobj, *qamlistprev = NULL;
	RMFQAMSrc* qamsrc = NULL;
	void* lowLevelElement = NULL;

	rmf_osal_mutexAcquire(g_qamlistmutex);
	RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", " %s():%d Enter g_qamsrcobjectno = %d, g_lowlevelelementsno = %d\n" , __FUNCTION__, __LINE__, g_qamsrcobjectno, g_lowlevelelementsno);

	used_qamsrc_elements = g_qamsrcobjectno + g_lowlevelelementsno;
	if ( used_qamsrc_elements == RMFQAMSrcImpl::numberOfTuners)
	{
		if (TRUE == RMFQAMSrcImpl::cachingEnabled )
		{
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s - All qamsrc are cached.\n", __FUNCTION__);
			qamlistobj = g_qamsrclist;
			while (qamlistobj)
			{
				if( qamlistobj->refcount == 0 )
				{
					break;
				}
				qamlistprev = qamlistobj;
				qamlistobj = qamlistobj->next;
			}
			if (NULL == qamlistobj )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - All qamsrc instances are being used.\n", __FUNCTION__);
				log_tuner_usage();
				rmf_osal_mutexRelease(g_qamlistmutex);
				return NULL;
			}
			qamsrc = qamlistobj->qamsrc;
			qamsrc->setFactoryManaged(0);
			if (NULL != qamlistobj->uri)
				free ( qamlistobj->uri);
			qamsrc->pause();
			qamsrc->close();
			qamsrc->term();
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s Deleting qamsrc.%p - list entry= %p\n", __FUNCTION__, qamlistobj->qamsrc, qamlistobj);
			delete qamsrc;
			qamsrc = NULL;

			if (qamlistprev )
			{
				qamlistprev->next = qamlistobj->next;
			}
			if (g_qamsrclist == qamlistobj)
			{
				g_qamsrclist = qamlistobj->next;
			}
			if (g_qamsrc_list_last_entry == qamlistobj)
			{
				g_qamsrc_list_last_entry = qamlistprev;
			}
			g_qamsrcobjectno --;
			rmf_osal_memFreeP ( RMF_OSAL_MEM_MEDIA, qamlistobj );
		}
		else
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - All qamsrc instances are being used.\n", __FUNCTION__);
			log_tuner_usage();
			rmf_osal_mutexRelease(g_qamlistmutex);
			return NULL;
		}
	}
	lowLevelElement = RMFQAMSrcImpl::createLowLevelElement();
	if (NULL == lowLevelElement )
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "RMFQAMSrcImpl::createLowLevelElement failed\n");
		log_tuner_usage();
	}
	else
	{
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s returning element %p\n", __FUNCTION__, lowLevelElement);
		g_lowlevelelementsno++;
	}
	RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", " %s():%d Exit g_qamsrcobjectno = %d, g_lowlevelelementsno = %d\n" , __FUNCTION__, __LINE__, g_qamsrcobjectno, g_lowlevelelementsno);
	rmf_osal_mutexRelease(g_qamlistmutex);
	return lowLevelElement;
}

/**
 * @brief Frees low level element of qamsrc.
 * Internally it decreases the reference count of the object and destroyes it when the count reaches zero.
 *
 * @param[in] element Typecasted pointer of low level element.
 *
 * @return None
 * @ingroup qamsrcapi
 */

void RMFQAMSrc::freeLowLevelElement( void* element)
{
	if( NULL == element )
	{
		RDK_LOG(RDK_LOG_WARN,"LOG.RDK.QAMSRC", " %s(): element is NULL. Cannot free element.\n" , __FUNCTION__);
		return;
	}
	rmf_osal_mutexAcquire(g_qamlistmutex);
	RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", " %s():%d Enter g_qamsrcobjectno = %d, g_lowlevelelementsno = %d\n" , __FUNCTION__, __LINE__, g_qamsrcobjectno, g_lowlevelelementsno);

	RMFQAMSrcImpl::freeLowLevelElement( element);
	g_lowlevelelementsno--;

	RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", " %s():%d Exit g_qamsrcobjectno = %d, g_lowlevelelementsno = %d\n" , __FUNCTION__, __LINE__, g_qamsrcobjectno, g_lowlevelelementsno);
	rmf_osal_mutexRelease(g_qamlistmutex);
}

/**
 * @fn RMFQAMSrc::getQAMSourceInstance( const char* uri_ip )
 * @brief This API gets qamsrc instance corresponding to the specified uri. It uses qamsrc factory
 * to create an object and returns RMFQAMSrc or RMFSDVSrc instance depending on the flag USE_SDVSRC.
 * A qamsrc object returned by this function would have already be tuned to the specified URI and SI
 * acqisition would have also started. It also manages the qamsrc caching mechanism.
 * - If the uri requested is already open then it returns the same qamsrc instance.
 * - If not it checks if its a SDVService, if yes then it creates, initializes and returns RMFSDVSrc instance
 *  (This step is performed only if USE_SDVSRC flag is set)
 * - Else it checks for unused qamsrc instances,if caching is enabled. If found it closes and reopens the
 *   qamsrc instance with the specified uri and returns a pointer to it.
 * - If no unused instances are found then it creates RMFQAMSrc instance and returns the pointer
 *
 * @param[in] uri  Source Id in the format ocap://0x\<source Id in hex\>.
 *                 It is also possible to specify complete tuning parameters in the following format:
 *                 tune://frequency=<frequency>&modulation=<modulation code>&pgmno=<program number>&
 *                 Please substitute decimal values for the above placeholders.
 *
 * @return RMFQAMSrc instance
 * @retval qamsrc RMFQAMSrc instance on success
 * @retval NULL Indicates could not get a qamsrc instance due to insufficient memory
 *  or No unused qamsrc available or initialising qamsrc failed and etc...
 * @ingroup qamsrcapi
 */
RMFQAMSrc* RMFQAMSrc::getQAMSourceInstance( const char* uri_ip , usage_t requestedUsage, const std::string &token, RMFResult *errorCode)
{
	rmf_Error rmf_error;
	qamsrcobjectlist* qamlistobj, *qamlistprev = NULL;
	RMFQAMSrc* qamsrc = NULL;
	RMFResult rc;
	int uri_len;
	char uri[QAM_MAX_URI_SIZE+1];
	int used_qamsrc_elements = 0;
	rmf_ProgramInfo_t programInfo;

	if ((NULL == uri_ip ) || (QAM_MAX_URI_SIZE < (uri_len = strlen(uri_ip))))
	{
		RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.QAMSRC", " %s():%d Invalid Params\n"
				, __FUNCTION__, __LINE__);
		return NULL;
	}
	
	strncpy(uri, uri_ip, QAM_MAX_URI_SIZE);
	if(QAM_MAX_URI_SIZE < uri_len)
	{
		uri[QAM_MAX_URI_SIZE] = '\0';
	}
	else
	{
		uri[uri_len] = '\0';
	}
	
	adjustUsageForLowPriorityTokens(requestedUsage, token);

	rmf_osal_mutexAcquire(g_qamlistmutex);
	RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", " %s():%d Enter g_qamsrcobjectno = %d, g_lowlevelelementsno = %d\n" , __FUNCTION__, __LINE__, g_qamsrcobjectno, g_lowlevelelementsno);
	used_qamsrc_elements = g_qamsrcobjectno + g_lowlevelelementsno;

	qamsrc = getExistingInstance(uri, TRUE );

	if (NULL != qamsrc )
	{
		qamsrc->attachUsage(requestedUsage, token);
		rmf_osal_mutexRelease(g_qamlistmutex);
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "Tune started - %s - Reusing qamsrc. %p RMFQAMSrc - Tune request success\n", __FUNCTION__, qamsrc);
		return qamsrc;
	}

    bool isSdvService = false;
#ifdef USE_SDVSRC
    isSdvService = RMFSDVSrc::isSwitched(uri);
    //
   if(isSdvService){
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s is Switched, frequency match in cache will be ignored\n", uri);
        programInfo.carrier_frequency = 0;	// real SDV carrier frequency acquired later on during QAMSrc open
    } 
    else {
        rc = RMFQAMSrcImpl::getProgramInfo( uri, &programInfo, NULL);
        if (RMF_RESULT_SUCCESS != rc )
        {
            rmf_osal_mutexRelease(g_qamlistmutex);
            RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s:%d uri: %s getProgramInfo failed\n", __FUNCTION__, __LINE__, uri);
            UPDATE_ERROR_CODE(errorCode, rc);
            return NULL;
        }
    }
#elif USE_DVBSRC 
    bool uriIsDvb = RMFDVBSrc::isDvb(uri);
    if (uriIsDvb) {
      RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s:%d DVB uri: %s \n", __FUNCTION__, __LINE__, uri);
      rc = RMFDVBSrc::getProgramInfo( uri, &programInfo, NULL);
      if (RMF_RESULT_SUCCESS != rc ) {
        rmf_osal_mutexRelease(g_qamlistmutex);
	RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s:%d uri: %s RMFDVBSrcImpl::getProgramInfo failed\n", __FUNCTION__, __LINE__, uri);
	return NULL;
      }
    }
    else {
        rc = RMFQAMSrcImpl::getProgramInfo( uri, &programInfo, NULL);
        if (RMF_RESULT_SUCCESS != rc )
        {
            rmf_osal_mutexRelease(g_qamlistmutex);
            RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s:%d uri: %s Normal getProgramInfo failed\n", __FUNCTION__, __LINE__, uri);
            return NULL;
        }
    }
#else
	rc = RMFQAMSrcImpl::getProgramInfo( uri, &programInfo, NULL);
	if (RMF_RESULT_SUCCESS != rc )
	{
		rmf_osal_mutexRelease(g_qamlistmutex);
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s:%d uri: %s getProgramInfo failed\n", __FUNCTION__, __LINE__, uri);
		UPDATE_ERROR_CODE(errorCode, rc);
		return NULL;
	}
#endif
	if (RMFQAMSrcImpl::cachingEnabled )
	{
		if ( used_qamsrc_elements > 0)
		{
			qamsrcobjectlist* qamlistobjcached = NULL;
			qamsrcobjectlist* qamlistobjcachedprev = NULL;

			//Find a cached free qamsrc with same frequency.
			qamlistobj = g_qamsrclist;
			while (qamlistobj)
			{
				if( qamlistobj->refcount == 0 )
				{
					if ( !isSdvService && qamlistobj->frequency == programInfo.carrier_frequency)
					{
						RMFQAMSrcImpl* privateImpl = (RMFQAMSrcImpl*)qamlistobj->qamsrc->getPrivateSourceImpl();
						if(TRUE == privateImpl->readyForReUse())
						{
							RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s - Found qamsrc instance %p locked to frequency %d.\n", __FUNCTION__, qamlistobj->qamsrc, qamlistobj->frequency);
							break;
						}
					}
					if ( NULL == qamlistobjcached )
					{
						qamlistobjcachedprev = qamlistprev;
						//cached free qamsrc if locked with same frequency not available.
						qamlistobjcached = qamlistobj;
					}
				}
				qamlistprev = qamlistobj;
				qamlistobj = qamlistobj->next;
			}
			if (NULL == qamlistobj )
			{
				if (  used_qamsrc_elements == RMFQAMSrcImpl::numberOfTuners)
				{
					if ( NULL == qamlistobjcached )
					{
						RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - No Qamsrc available.\n", __FUNCTION__);
						rmf_osal_mutexRelease(g_qamlistmutex);
						UPDATE_ERROR_CODE(errorCode, RMF_QAMSRC_ERROR_NO_TUNERS_AVAILABLE); //pre-fill error code in case preempt fails.
						return preempt(uri_ip, requestedUsage, token);
					}
					else
					{
						RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s - All qamsrc are cached.\n", __FUNCTION__);
						qamlistobj = qamlistobjcached;
						qamlistprev = qamlistobjcachedprev;
					}
				}

			}
			if ( NULL != qamlistobj )
			{
				qamsrc = qamlistobj->qamsrc;
				qamsrc->setFactoryManaged(0);
				if (NULL != qamlistobj->uri)
					free ( qamlistobj->uri);
				qamsrc->pause();
				qamsrc->close();
				/*Move entry to end of list*/
				if (g_qamsrc_list_last_entry != qamlistobj)
				{
					if (NULL == qamlistprev )
					{
						g_qamsrclist = qamlistobj->next;
					}
					else
					{
						qamlistprev->next = qamlistobj->next;
					}
					g_qamsrc_list_last_entry->next = qamlistobj;
					qamlistobj->next = NULL;
					g_qamsrc_list_last_entry = qamlistobj;
				}
			}
		}
	}
	else if ( used_qamsrc_elements == RMFQAMSrcImpl::numberOfTuners )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - No Qamsrc available.\n", __FUNCTION__);
		rmf_osal_mutexRelease(g_qamlistmutex);
		UPDATE_ERROR_CODE(errorCode, RMF_QAMSRC_ERROR_NO_TUNERS_AVAILABLE); //pre-fill error code in case preempt fails.
		return preempt(uri_ip, requestedUsage, token);
	}

	if ( NULL == qamsrc )
	{
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s - Creating new qamsrc.\n", __FUNCTION__);
		rmf_error =  rmf_osal_memAllocP (RMF_OSAL_MEM_MEDIA, sizeof (qamsrcobjectlist), (void**)&qamlistobj);
		if (RMF_SUCCESS != rmf_error )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - rmf_osal_memAllocP failed.\n", __FUNCTION__);
			rmf_osal_mutexRelease(g_qamlistmutex);
			return NULL;
		}
#ifdef USE_SDVSRC
#ifdef USE_DVBSRC
		if(RMFDVBSrc::isDvb(uri))
		{
			qamsrc = new RMFDVBSrc();
		}
		else
		{
#endif
			qamsrc = new RMFSDVSrc();
#ifdef USE_DVBSRC
		}
#endif
#else
#ifdef USE_DVBSRC
			qamsrc = new RMFDVBSrc();
#else
			qamsrc = new RMFQAMSrc();
#endif
#endif
		if( NULL == qamsrc )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - RMFQAMSrc create failed.\n", __FUNCTION__);
			rmf_osal_mutexRelease(g_qamlistmutex);
			return NULL;
		}

		rc = qamsrc->init();
		if ( RMF_RESULT_SUCCESS != rc )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s: %d: RMFQAMSrc->init failed\n",
					__FUNCTION__, __LINE__ );
			rmf_osal_mutexRelease(g_qamlistmutex);
			delete qamsrc;
			rmf_osal_memFreeP ( RMF_OSAL_MEM_MEDIA, qamlistobj );
			UPDATE_ERROR_CODE(errorCode, rc);
			return NULL;
		}
		qamlistobj->qamsrc = qamsrc;
		qamlistobj->next = NULL;
		if (NULL == g_qamsrclist )
		{
			g_qamsrclist = qamlistobj;
		}
		else
		{
			g_qamsrc_list_last_entry->next = qamlistobj;
		}
		g_qamsrc_list_last_entry = qamlistobj;

		g_qamsrcobjectno++;
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s - Created qamsrc list entry. qamsrc.%p - list entry= %p\n", __FUNCTION__, qamlistobj->qamsrc, qamlistobj);
	}

	rc = qamsrc->open( uri , NULL);
	if ( RMF_RESULT_SUCCESS != rc )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s: %d: RMFQAMSrc->open failed\n",
				__FUNCTION__, __LINE__ );
		removeFromList( (void*) qamlistobj );
		qamsrc->term();
		rmf_osal_mutexRelease(g_qamlistmutex);
		delete qamsrc;
		rmf_osal_memFreeP ( RMF_OSAL_MEM_MEDIA, qamlistobj );
		UPDATE_ERROR_CODE(errorCode, rc);
		return NULL;
	}
	qamsrc->setFactoryManaged(1);
	qamlistobj->refcount = 1;
	qamlistobj->uri = strdup( uri );
	qamlistobj->frequency = programInfo.carrier_frequency;

	RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", " %s():%d Exit qamsrc = %p g_qamsrcobjectno = %d, g_lowlevelelementsno = %d\n" , __FUNCTION__, __LINE__, qamsrc, g_qamsrcobjectno, g_lowlevelelementsno);
	qamsrc->attachUsage(requestedUsage, token);
	rmf_osal_mutexRelease(g_qamlistmutex);
	return qamsrc;
}

/**
 * @brief This API is used to delete the qamsource object corresponding to the specified qamsrc instance,
 * which is obtained using factory method.
 * The qamsrc object is not deleted if:
 * <ul>
 * <li> The qamsource object is being used or
 * <li> The qamsrc instance can be reused
 * </ul>
 *
 * @param[in] qamsrc  RMFQAMSrc instance to be deleted
 *
 * @return None
 * @ingroup qamsrcapi
 */
void RMFQAMSrc::freeQAMSourceInstance( RMFQAMSrc* qamsrc, usage_t usage, const std::string &token)
{
	qamsrcobjectlist* qamlistobj, *qamlistobjprev = NULL, *qamlistobjtodelete = NULL;
	RMFQAMSrcImpl* privateImpl = NULL;

	RDK_LOG(RDK_LOG_TRACE1,"LOG.RDK.QAMSRC", " %s():%d \n" , __FUNCTION__, __LINE__);
	
	rmf_osal_mutexAcquire(g_qamlistmutex);
	RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", " %s():%d Enter g_qamsrcobjectno = %d, g_lowlevelelementsno = %d\n" , __FUNCTION__, __LINE__, g_qamsrcobjectno, g_lowlevelelementsno);
	qamlistobj = g_qamsrclist;

	while ( NULL != qamlistobj)
	{
		if ( qamsrc ==  qamlistobj->qamsrc )
		{
			break;
		}
		qamlistobjprev = qamlistobj;
		qamlistobj = qamlistobj->next;
	}
	if (NULL == qamlistobj )
	{
		RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.QAMSRC", " %s():%d Cannot found matching entry\n"
				, __FUNCTION__, __LINE__);
		rmf_osal_mutexRelease(g_qamlistmutex);
		return;
	}
	qamlistobj->refcount --;
	qamlistobj->qamsrc->detachUsage(usage, token);
	RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", " %s():%d qamsrc %p refcount = %d.\n" , __FUNCTION__, __LINE__, qamlistobj->qamsrc, qamlistobj->refcount);

	/* There have been instances of qamsource instances getting released by entities that don't own it. In such cases,
	refcount can become negative and all subsequent sessions using that instance will be problemmatic. */
	RMF_ASSERT( 0 <= qamlistobj->refcount );
	
	privateImpl = (RMFQAMSrcImpl*)qamsrc->getPrivateSourceImpl();

	/* Delete unused object if caching is disabled, some error has occurred, or if SDV tuned RMFQAMSrc (indicated by freq 0).
	   	   SDV tuned QAMSrc must always be deleted so that the SDV session is closed with the server */
	if ((0 == qamlistobj->refcount) &&
			((qamlistobj->frequency == 0) || (FALSE == RMFQAMSrcImpl::cachingEnabled) || (FALSE == privateImpl->readyForReUse())))
	{
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s Deleting qamsrc.%p - list entry= %p\n", __FUNCTION__, qamlistobj->qamsrc, qamlistobj);
		if (qamlistobjprev )
		{
			qamlistobjprev->next = qamlistobj->next;
		}
		if (g_qamsrclist == qamlistobj)
		{
			g_qamsrclist = qamlistobj->next;
		}
		if (g_qamsrc_list_last_entry == qamlistobj)
		{
			g_qamsrc_list_last_entry = qamlistobjprev;
		}
		qamlistobjtodelete = qamlistobj;
	}

	if (NULL != qamlistobjtodelete )
	{
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s - deleting list entry= %p uri = %s\n", __FUNCTION__, qamlistobjtodelete, qamlistobjtodelete->uri);
		qamsrc->setFactoryManaged(0);
		if (NULL != qamlistobjtodelete->uri)
			free ( qamlistobjtodelete->uri);
		qamlistobjtodelete->qamsrc->close();
		qamlistobjtodelete->qamsrc->term();
		delete qamlistobjtodelete->qamsrc;
		rmf_osal_memFreeP ( RMF_OSAL_MEM_MEDIA, qamlistobjtodelete );

		g_qamsrcobjectno --;
	}

	RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", " %s():%d Exit g_qamsrcobjectno = %d, g_lowlevelelementsno = %d\n" , __FUNCTION__, __LINE__, g_qamsrcobjectno, g_lowlevelelementsno);
	rmf_osal_mutexRelease(g_qamlistmutex);
	return;
}

uint32_t RMFQAMSrc::getUsage()
{
	uint32_t complete_usage = 0;
	rmf_osal_mutexAcquire(m_mutex);
	if(0 != usages.size())
	{
		std::map<std::string,uint32_t>::iterator it = usages.begin();
		while (it != usages.end())
		{
			complete_usage |= it->second;
			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "%s - Usage token=[%s] uses=[%x] \n", __FUNCTION__, (it)->first.c_str(), (it)->second);
			it++;
		}
	}
	else
	{
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "%s - qamsrc has no usage \n", __FUNCTION__);
	}
	rmf_osal_mutexRelease(m_mutex);
	return complete_usage;
}

uint32_t RMFQAMSrc::mapUsageToPriority(uint32_t usage)
{
	int i = 0;
	while(usage_priority_table[i][0] != USAGE_END_MARKER)
	{
		if(0 != (usage & usage_priority_table[i][0]))
		{
			//This is the highest priority usage attached to this qamsource.
			return usage_priority_table[i][1];
		}
		i++;
	}

	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.QAMSRC", "%s: usage %d could not be matched in the table.\n",
		__FUNCTION__, usage);
	return 0;
}

/* Go through g_qamsrclist one by one; find what objects have lower priority than the incoming
 * request. Return the object with the least priority among them. Update retrievedUsage with
 * the usage matching that qamsrc object. */
RMFQAMSrc* RMFQAMSrc::findQAMSrcToPreempt(usage_t requestedUsage, uint32_t &retrievedUsage)
{
	qamsrcobjectlist* qamlistobj = g_qamsrclist;
	uint32_t eachUsage;
	uint32_t priorityToBeat = mapUsageToPriority((uint32_t) requestedUsage);
	RMFQAMSrc *preemptionCandidate = NULL;
	uint32_t retrievedPriority = 0;

	while(qamlistobj)
	{
		retrievedUsage = qamlistobj->qamsrc->getUsage();
		if(0 == retrievedUsage)
		{
			//We cannot pre-empt an object with no attached usages.
			qamlistobj = qamlistobj->next;
			continue;
		}
		retrievedPriority = mapUsageToPriority(retrievedUsage);
		if(priorityToBeat > retrievedPriority)
		{
			int32_t numPreemptableUsages = 0;
			RDK_LOG(RDK_LOG_DEBUG,"LOG.RDK.QAMSRC", "Found object of lower priority 0x%x\n",
				qamlistobj->qamsrc);

			/*Check whether it supports pre-emption*/
			for(uint32_t mask = 0x1; mask < USAGE_END_MARKER; mask = mask<<0x01)
			{
				eachUsage = mask & retrievedUsage;
				if(eachUsage)
				{
					//Check whether this usage type is pre-emptable.
					if(NULL == releaseCallbackList[(usage_t)eachUsage])
					{
						//No callback registered for type. Not pre-emptable.
						RDK_LOG(RDK_LOG_DEBUG,"LOG.RDK.QAMSRC", "Non pre-emptable type. Skip object.\n");
						numPreemptableUsages = -1;
						break;
					}
					else
					{
						numPreemptableUsages++;
					}
				}
			}

			if(0 < numPreemptableUsages)
			{
				/*All attached usages are pre-emptable.*/
				RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "QAMSource  0x%x can be reclaimed.\n", qamlistobj->qamsrc);
				preemptionCandidate = qamlistobj->qamsrc;
				priorityToBeat = retrievedPriority;
			}
		}
		qamlistobj = qamlistobj->next;
	}
	if(NULL !=  preemptionCandidate)
	{
		retrievedUsage = preemptionCandidate->getUsage();
		return preemptionCandidate;
	}

	/*Found no objects with lower priority. If live is allowed to pre-empt live, look for objects fulfilling that criterion. */
	if((TRUE == livePreemptionEnabled) && (USAGE_LIVE == requestedUsage) && (NULL != releaseCallbackList[USAGE_LIVE]))
	{
		qamlistobj = g_qamsrclist;
		while(qamlistobj)
		{
			retrievedUsage = qamlistobj->qamsrc->getUsage();

			if(USAGE_LIVE == retrievedUsage)
			{
				RDK_LOG(RDK_LOG_DEBUG,"LOG.RDK.QAMSRC", "Found disposable LIVE object 0x%x that can be pre-empted.\n",
						qamlistobj->qamsrc);

				return qamlistobj->qamsrc;
			}
			qamlistobj = qamlistobj->next;
		}
	}
	return NULL;
}

void RMFQAMSrc::prepForPreemption(void)
{
	/*Flush existing usages so that this is no longer a preemption candidate for 
	 * any requests that follow.*/
	flushUsages();
	
	/* Increment the refcount so that this tuner isn't fully "freed" when the application
	 * processes the release callback. After release callback returns, preempt() would
	 * lock g_qamlistmutex, free the tuner for good and call getQAMSOurceInstance(). 
	 * That way, we can be sure that the original "preemptor" is allocated this tuner
	 * that was just freed.*/
	qamsrcobjectlist* qamlistobj = qam_factory_get_entry(this);
	qamlistobj->refcount++;
}

RMFQAMSrc* RMFQAMSrc::preempt(const char* uri, usage_t requestedUsage, const std::string &token)
{
	RMFQAMSrc* newInstance = NULL;
	uint32_t retrievedUsage;
	std::string releaseToken;
	std::string releaseUri;
	RMFQAMSrc* qamsrc = NULL;

	RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "Looking for pre-emptable qamsource objects.\n");
	
	rmf_osal_mutexAcquire(g_qamlistmutex);
		
	qamsrc = findQAMSrcToPreempt(requestedUsage, retrievedUsage); 
	if(qamsrc)
	{
		int32_t numSuccessfulReleases = 0;
		/* There is a candidate qamsource object that is disposable. It has one or more
		registered usages. Call the release callback.
		*/
		for(uint32_t mask = 0x1; mask < USAGE_END_MARKER; mask = mask<<0x01)
		{
			uint32_t eachUsage = mask & retrievedUsage;
			if(eachUsage)
			{
				if(RMF_RESULT_SUCCESS == qamsrc->getMatchingToken((usage_t)eachUsage, releaseToken))
				{
					((RMFQAMSrcImpl *)(qamsrc->getPrivateSourceImpl()))->getURI(releaseUri);

					QamsrcReleaseCallback_t callbackFn = releaseCallbackList[(usage_t)eachUsage];
					qamsrc->prepForPreemption();
					
					rmf_osal_mutexRelease(g_qamlistmutex);
					RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "Requesting release. Usage is 0x%x\n", eachUsage);
					if(0 != (*callbackFn)(releaseToken, releaseUri))
					{
						RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.QAMSRC", "Release failed. Aborting.\n");
						//Reverse the additional refcount added by prepForPreemption
						freeQAMSourceInstance(qamsrc, USAGE_LIVE, QAMSRC_DEFAULT_TOKEN); //Dummy tokens are harmless.
						numSuccessfulReleases = -1;
						break;
					}
					else
					{
						numSuccessfulReleases++;
					}
					rmf_osal_mutexAcquire(g_qamlistmutex);
				}
				else
				{
					//Do nothing. The token was probably removed.
				}
				/*Check global qamlist to see whether one tuner made it to the free pool.
				If it didn't, cycle through any remaining usages attached to this qamsource object
				and trigger release. Do this until a tuner is back in the pool or all usages are 
				exhausted.

				This additional logic does not seem necessary at the moment for the comibinations that 
				can forseeably occur. MS supports pre-emption and it owns LIVE/TSB usages; Autodiscovery 
				is expected to support pre-emption in future and it controls AD usage.
				A release enacted by either MS or Autodiscovery applications will remove all tokens attached 
				to this qamsource. So the chances of finding a remaining token after release are nil.

				Until the above is implemented, just break after the first release call. If there's a
				free tuner, the getQAMSourceInstance() call will find it. If not, give up.
				*/
				break;
			}
		}

		if(0 < numSuccessfulReleases)
		{
			freeQAMSourceInstance(qamsrc, USAGE_LIVE, QAMSRC_DEFAULT_TOKEN); //Dummy tokens are harmless.
		}
	}

	/* Check for tuners that may now be available. */
	if(TRUE == check_for_free_qamsrc())
	{
		newInstance = getQAMSourceInstance(uri, requestedUsage, token);
		RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "Repurpose outcome is 0x%x\n", newInstance);
	}
	else
	{
		RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", "Repurpose outcome is 0x%x\n", newInstance);
		log_tuner_usage();
	}
	rmf_osal_mutexRelease(g_qamlistmutex);
	return newInstance;
}

RMFResult RMFQAMSrc::getMatchingToken(usage_t usage, std::string &token)
{
	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "Finding matching token for usage 0x%x\n", usage);
	rmf_osal_mutexAcquire(m_mutex);
	if((0 != usages.size()) && (0 != usage))
	{
		std::map<std::string,uint32_t>::iterator it = usages.begin();
		while (it != usages.end())
		{
			if(0 != (usage & it->second))
			{
				token = it->first;
				rmf_osal_mutexRelease(m_mutex);
				return RMF_RESULT_SUCCESS;
			}
			it++;
		}
	}
	rmf_osal_mutexRelease(m_mutex);
	return RMF_RESULT_FAILURE;
}

/**
 * @brief This API is used to free all unused qamsrc instances.
 * I removes the unused qamsrc object in the cache and then frees the memory
 *
 * @return None
 * @ingroup qamsrcapi
 */
void RMFQAMSrc::freeCachedQAMSourceInstances( )
{
	qamsrcobjectlist* qamlistobj, *qamlistobjprev = NULL, *qamlistobjtodelete = NULL;
	RMFQAMSrcImpl* privateImpl = NULL;

	RDK_LOG(RDK_LOG_TRACE1,"LOG.RDK.QAMSRC", " %s():%d \n" , __FUNCTION__, __LINE__);

	rmf_osal_mutexAcquire(g_qamlistmutex);
	RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", " %s():%d Enter g_qamsrcobjectno = %d, g_lowlevelelementsno = %d\n" , __FUNCTION__, __LINE__, g_qamsrcobjectno, g_lowlevelelementsno);
	qamlistobj = g_qamsrclist;

	while ( NULL != qamlistobj)
	{
		if ( 0 == qamlistobj->refcount)
		{
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s Deleting qamsrc.%p - list entry= %p\n", __FUNCTION__, qamlistobj->qamsrc, qamlistobj);
			if (qamlistobjprev )
			{
				qamlistobjprev->next = qamlistobj->next;
			}
			if (g_qamsrclist == qamlistobj)
			{
				g_qamsrclist = qamlistobj->next;
			}
			if (g_qamsrc_list_last_entry == qamlistobj)
			{
				g_qamsrc_list_last_entry = qamlistobjprev;
			}
			qamlistobjtodelete = qamlistobj;
			g_qamsrcobjectno --;
			qamlistobj = qamlistobj->next;

			if ( NULL != qamlistobjtodelete->qamsrc)
			{
				RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "%s - deleting list entry= %p uri = %s\n", __FUNCTION__, qamlistobjtodelete, qamlistobjtodelete->uri);
				qamlistobjtodelete->qamsrc->setFactoryManaged(0);
				if (NULL != qamlistobjtodelete->uri)
					free ( qamlistobjtodelete->uri);
				qamlistobjtodelete->qamsrc->close();
				qamlistobjtodelete->qamsrc->term();
				delete qamlistobjtodelete->qamsrc;
				rmf_osal_memFreeP ( RMF_OSAL_MEM_MEDIA, qamlistobjtodelete );
			}
			continue;
		}
		qamlistobjprev = qamlistobj;
		qamlistobj = qamlistobj->next;
	}

	RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", " %s():%d Exit g_qamsrcobjectno = %d, g_lowlevelelementsno = %d\n" , __FUNCTION__, __LINE__, g_qamsrcobjectno, g_lowlevelelementsno);
	rmf_osal_mutexRelease(g_qamlistmutex);

}

void RMFQAMSrc::setFactoryManaged( int fm)
{
	RDK_LOG(RDK_LOG_TRACE1,"LOG.RDK.QAMSRC", " %s():%d \n" , __FUNCTION__, __LINE__);
        rmf_osal_mutexAcquire( m_mutex );
	factoryManaged = fm;
        rmf_osal_mutexRelease( m_mutex );
}

/**
 * @brief This API is used to disable caching.  It is used by SDV source. If the caching is disabled on a factory generated
 * object then it is not released to the cache for future use but it will be destroyed when the object is freed.
 *
 * @return None
 */
void RMFQAMSrc::disableCaching()
{
	RMFQAMSrcImpl::disableCaching();
}

/**
 * @fn RMFQAMSrc::changeURI( const char* uri , RMFQAMSrc* old, RMFQAMSrc** updated, bool &newInstance )
 * @brief This API is responsible for changing the channel.
 * If a channel is currently playing then stops it and free's all the resources allocated to it.
 * It is used by TDK for testing purpose.
 * - It returns the RMFQAMSrc instance with the uri parameter specified if its already open and ready for reuse.
 * - If not the function searches for qamsrcobjectlist correspoding to the given parameter old and
 *   checks if it can be reused, if yes then it updates the qamsrcobjectlist with the specified uri
 *   and returns the corresponding RMFQAMSrc instance
 * - If not a new RMFQAMSrc instance is created and returned
 *
 * @param[in] uri Url for new channel in the format ocap://0x\<source Id in hex\>
 * @param[in] old RMFQAMSrc instance of the channel currently being played
 * @param[out] updated Holds RMFQAMSrc instance with the uri parameter received. This can be same as old.
 * @param[out] newInstance True if a different qam instance is returned otherwise false
 *
 * @return RMFResult (int)
 * @retval RMF_RESULT_SUCCESS Uri is successfully changed
 * @retval RMF_RESULT_INVALID_ARGUMENT Invalid arguements supplied such as incorrect url, incorrent RMFQAMSrc instance etc...
 * @retval RMF_RESULT_FAILURE The instance is not registered in qam factory list
 * @retval RMF_RESULT_INTERNAL_ERROR Indicates failed to link source and tee
 * @ingroup qamsrcapi
 */

RMFResult RMFQAMSrc::changeURI( const char* uri , RMFQAMSrc* old, RMFQAMSrc** updated, 
	bool &newInstance , usage_t usage, const std::string &token)
{
	RMFQAMSrc* qamsrc = NULL;
	RMFResult ret = RMF_RESULT_SUCCESS;
	qamsrcobjectlist* qamlistobj = NULL;

	RDK_LOG(RDK_LOG_TRACE1,"LOG.RDK.QAMSRC", "Enter %s():%d \n" , __FUNCTION__, __LINE__);

	if (NULL == uri || NULL == old || NULL == updated)
	{
		RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.QAMSRC", " %s():%d Invalid Params\n"
				, __FUNCTION__, __LINE__);
		return RMF_RESULT_INVALID_ARGUMENT;
	}
	adjustUsageForLowPriorityTokens(usage, token);

	/*Recursive mutex*/
	rmf_osal_mutexAcquire(g_qamlistmutex);

	if (NULL != getExistingInstance(uri, FALSE))
	{
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s - Reusing qamsrc. %p. Freeing old %p\n", __FUNCTION__, qamsrc, old);
		freeQAMSourceInstance(old, usage, token);
		qamsrc = getQAMSourceInstance(uri, usage, token);
		if(NULL == qamsrc)
		{
			ret = RMF_RESULT_FAILURE;
		}
	}
	else
	{
		qamsrc = old;
		qamlistobj = qam_factory_get_entry( qamsrc);
		if (NULL == qamlistobj )
		{
			RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.QAMSRC", " %s():%d Cannot found matching entry\n"
					, __FUNCTION__, __LINE__);
			rmf_osal_mutexRelease(g_qamlistmutex);
			return RMF_RESULT_FAILURE;
		}

		if ( qamlistobj->refcount == 1 )
		{
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s - Reconfiguring qamsrc. %p. \n", __FUNCTION__, qamsrc);
			qamsrc->setFactoryManaged(0);
			if (NULL != qamlistobj->uri)
				free ( qamlistobj->uri);
			qamsrc->close();

			ret = qamsrc->open( uri , NULL);
			if ( RMF_RESULT_SUCCESS != ret )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s: %d: RMFQAMSrc->open failed\n",
						__FUNCTION__, __LINE__ );
				rmf_osal_mutexRelease(g_qamlistmutex);
				return ret;
			}
			qamsrc->setFactoryManaged(1);
			qamlistobj->uri = strdup( uri );
		}
		else
		{
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s - qamsrc %p is being used. Getting another one \n", __FUNCTION__, qamsrc);
			qamsrc = getQAMSourceInstance( uri );
			if ( NULL == qamsrc )
			{
				RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.QAMSRC", " %s():%d No unused instance\n"
						, __FUNCTION__, __LINE__);
				rmf_osal_mutexRelease(g_qamlistmutex);
				return RMF_RESULT_FAILURE;
			}
			freeQAMSourceInstance( old );
		}
	}

	rmf_osal_mutexRelease(g_qamlistmutex);
	*updated = qamsrc;
	newInstance = (qamsrc != old)? true: false;

	RDK_LOG(RDK_LOG_TRACE1,"LOG.RDK.QAMSRC", "Exit %s():%d \n" , __FUNCTION__, __LINE__);
	return ret;
}

RMFQAMSrc* RMFQAMSrc::getExistingInstance (const char * uri, bool addRef, int * references)
{
	qamsrcobjectlist* qamlistobj, *qamlistprev = NULL;
	RMFQAMSrc* qamsrc = NULL;
	RMFQAMSrcImpl* privImpl = NULL;
	qamlistobj = g_qamsrclist;

	RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.QAMSRC", "%s - uri %s \n", __FUNCTION__, uri);

#if 0
	while ( qamlistobj)
	{
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s - qamsrc(%p)->uri %s \n", __FUNCTION__, qamlistobj->qamsrc, qamlistobj->uri);
		qamlistobj = qamlistobj->next;
	}
	qamlistobj = g_qamsrclist;
#endif

	while (qamlistobj)
	{
		if (0 == strncasecmp(uri, qamlistobj->uri, QAM_MAX_URI_SIZE))
		{
			qamsrc = qamlistobj->qamsrc;
			privImpl = (RMFQAMSrcImpl*)qamsrc->getPrivateSourceImpl();

			/* Criteria to pick or drop a cached object tuned to the specified channel:
			 * 1. If it's "ready for reuse", always pick it up.
			 * 2. If it's not "ready for reuse", but has a non-zero refcount, pick it
			 *    up anyway because TRM would expect that. We don't want multiple applications 
			 *    tuning to the same service to use more than one tuner.
			 * 3. If it's not "ready for reuse" and has a zero ref-count, do NOT pick
			 *    it up, but destroy it. */

			if ((FALSE == privImpl->readyForReUse(uri)) && (0 == qamlistobj->refcount))
			{
				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s - found qamsrc %p for the same locator in a bad state. Destroying it now.\n", __FUNCTION__, qamsrc);
				qamlistobj->refcount = 1; //Increase refcount from 0 to 1 so that freeQAMSourceInstance() won't reject it.
				qamlistobj = qamlistobj->next;
				freeQAMSourceInstance(qamsrc);
				qamsrc = NULL;
				continue;
			}

			if (addRef )
			{
				qamlistobj->refcount++;
				/*Move entry to end of list*/
				if (g_qamsrc_list_last_entry != qamlistobj)
				{
					if (NULL == qamlistprev )
					{
						g_qamsrclist = qamlistobj->next;
					}
					else
					{
						qamlistprev->next = qamlistobj->next;
					}
					g_qamsrc_list_last_entry->next = qamlistobj;
					g_qamsrc_list_last_entry = qamlistobj;
					qamlistobj->next = NULL;
				}
			}

		if(references)
		{
			*references = qamlistobj->refcount;
		}
			break;
		}
		qamlistprev = qamlistobj;
		qamlistobj = qamlistobj->next;
	}

	RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.QAMSRC", "Exit %s\n", __FUNCTION__);
	return qamsrc;
}

void RMFQAMSrc::removeFromList (void* obj)
{
	qamsrcobjectlist* qamlistobj, *qamlistprev = NULL;
	qamsrcobjectlist* qamlistobjtoremove = (qamsrcobjectlist*)obj;
	qamlistobj = g_qamsrclist;
	while (qamlistobj)
	{
		if (qamlistobj == qamlistobjtoremove )
		{
			break;
		}
		qamlistprev = qamlistobj;
		qamlistobj = qamlistobj->next;
	}
	if (qamlistobj )
	{
		if ( qamlistprev == NULL )
		{
			g_qamsrclist = qamlistobj->next;
		}
		else
		{
			qamlistprev->next = qamlistobjtoremove->next;
		}
		if (g_qamsrc_list_last_entry == qamlistobj )
		{
			g_qamsrc_list_last_entry = qamlistprev;
		}
		g_qamsrcobjectno--;
	}
}

/**
 * @brief This function is used to initializes the RMFQAMSrc instance and configures the
 * qamtunersrc gstreamer plugin. It allocates the tuner and creats the gstreamer pipeline element.
 * It creates source bin, tee object and attached it to the pipeline.
 * It registers callback for gstreamer bus events and translate to RMF events.
 * If factoryManaged is set, it is already initialized in getqamsource instance. So, no need to initialize again.
 *
 * @return RMFResult (int)
 * @retval RMF_RESULT_SUCCESS Successfully initialized the gstreamer pipeline.
 * @retval RMF_RESULT_INTERNAL_ERROR If failed to link source and tee element.
 * @retval RMF_RESULT_FAILURE If Gstreamer pipeline is failed to be instantiated.
 * @ingroup qamsrcapi
 */
RMFResult RMFQAMSrc::init()
{
	RDK_LOG(RDK_LOG_DEBUG,"LOG.RDK.QAMSRC", " %s():%d Enter\n" , __FUNCTION__, __LINE__);
	//rmf_osal_mutexAcquire(g_qamlistmutex);
	//Changing mutex from g_qamlistmutex to m_mutex
	//as g_qamlistmutex is used for globals and m_mutex for objectmembers
	rmf_osal_mutexAcquire( m_mutex );
	if ( factoryManaged )
	{
		//rmf_osal_mutexRelease(g_qamlistmutex);
		rmf_osal_mutexRelease( m_mutex );
		return RMF_RESULT_SUCCESS;
	}
	//rmf_osal_mutexRelease(g_qamlistmutex);
	rmf_osal_mutexRelease( m_mutex );
	return RMFMediaSourceBase::init();
}

/**
 * @fn RMFQAMSrc::term()
 * @brief This function is used to terminates the RMFQAMSrc instance and deletes the gstreamer object.
 * It terminates all the playback, removes all the sinks from the pipeline and sets the gst pipeline state as "GST_STATE_NULL".
 *
 * @return RMFResult (int)
 * @retval RMF_RESULT_SUCCESS Terminated gstreamer pipeline successfully.
 * @retval RMF_RESULT_NOT_INITIALIZED If the gstreamer pipeline is not initialized.
 * @ingroup qamsrcapi
 */
RMFResult RMFQAMSrc::term()
{
	RDK_LOG(RDK_LOG_DEBUG,"LOG.RDK.QAMSRC", " %s():%d Enter \n" , __FUNCTION__, __LINE__);
	//rmf_osal_mutexAcquire(g_qamlistmutex);
	//Changing mutex from g_qamlistmutex to m_mutex
	//as g_qamlistmutex is used for globals and m_mutex for objectmembers
	rmf_osal_mutexAcquire( m_mutex );
	if ( factoryManaged )
	{
		//rmf_osal_mutexRelease(g_qamlistmutex);
		rmf_osal_mutexRelease( m_mutex );
		return RMF_RESULT_SUCCESS;
	}
	//rmf_osal_mutexRelease(g_qamlistmutex);
	rmf_osal_mutexRelease( m_mutex );
	return RMFMediaSourceBase::term();
}


/**
 * @brief This function gets the tuning parameters and program number from OOB SI manager .
 * Sets the tuning parameters of qamtunersrc gstreamer element.
 * It starts the PSI acquisition and used to filter the PAT and PMT.
 * Initiates decryption by calling POD manager API's
 *
 * @param[in] uri  Source Id in the format ocap://0x\<source Id in hex\>.
 * @param[in] mimetype  Not used
 *
 * @return RMFResult (int)
 * @retval RMF_RESULT_SUCCESS  Successfully opened gstreamer pipeline.
 * @retval RMF_RESULT_INVALID_ARGUMENT Invalid URI supplied as an argument.
 * @retval RMF_RESULT_NOT_INITIALIZED  If the gstreamer pipeline is not initialized.
 * @ingroup qamsrcapi
 */
RMFResult RMFQAMSrc::open(const char* uri, char* mimetype)
{
	RDK_LOG(RDK_LOG_DEBUG,"LOG.RDK.QAMSRC", " %s():%d Enter \n" , __FUNCTION__, __LINE__);
	//rmf_osal_mutexAcquire(g_qamlistmutex);
	//Changing mutex from g_qamlistmutex to m_mutex
	//as g_qamlistmutex is used for globals and m_mutex for objectmembers
	rmf_osal_mutexAcquire( m_mutex );
	if ( factoryManaged )
	{
		//rmf_osal_mutexRelease(g_qamlistmutex);
		rmf_osal_mutexRelease( m_mutex );
		return RMF_RESULT_SUCCESS;
	}
	//rmf_osal_mutexRelease(g_qamlistmutex);
	rmf_osal_mutexRelease( m_mutex );
	return RMFMediaSourceBase::open( uri, mimetype);
}


/**
 * @brief This function stops acquire the PSI table information of PAT and PMT.
 * It gives the signal to POD manager to stop decryption.
 *
 * @return RMFResult (int)
 * @retval RMF_RESULT_SUCCESS Closed the gstreamer pipeline successfully.
 * @retval RMF_RESULT_NOT_INITIALIZED  If the gstreamer pipeline is not initialized.
 * @ingroup qamsrcapi
 */
RMFResult RMFQAMSrc::close()
{
	RDK_LOG(RDK_LOG_DEBUG,"LOG.RDK.QAMSRC", " %s():%d Enter \n" , __FUNCTION__, __LINE__);
	//rmf_osal_mutexAcquire(g_qamlistmutex);
	//Changing mutex from g_qamlistmutex to m_mutex
	//as g_qamlistmutex is used for globals and m_mutex for objectmembers
	rmf_osal_mutexAcquire( m_mutex );
	if ( factoryManaged )
	{
		//rmf_osal_mutexRelease(g_qamlistmutex);
		rmf_osal_mutexRelease( m_mutex );
		return RMF_RESULT_SUCCESS;
	}
	//rmf_osal_mutexRelease(g_qamlistmutex);
	rmf_osal_mutexRelease( m_mutex );
	return RMFMediaSourceBase::close();
}

/**
 * @fn RMFQAMSrc::play(float& speed, double& time)
 * @brief This function starts the gstreamer pipeline and provides the SPTS buffer to sink.
 * It sets the gstreamer state to "GST_STATE_PLAYING".
 *
 * @param[in] speed  Speed of play. Speed of 0 pause and 1 plays.
 * @param[in] time  Not used
 *
 * @return RMFResult (int)
 * @retval RMF_RESULT_NOT_INITIALIZED If the gstreamer pipeline is not initialized.
 * @retval GST_STATE_PLAYING GST pipeline set to be playing state.
 * @retval GST_STATE_PAUSED If speed is 0 and GST pipeline set to be paused state.
 * @retval RMF_RESULT_NO_SINK If queue size is 0.
 * @ingroup qamsrcapi
 */
RMFResult RMFQAMSrc::play(float& speed, double& time)
{
	RMFResult ret;

	RDK_LOG(RDK_LOG_DEBUG,"LOG.RDK.QAMSRC", " Enter %s():%d speed=%f \n" , 
			__FUNCTION__, __LINE__, speed);
	if ( 0 == speed )
	{
		ret = pause();
	}
	else
	{
		ret = RMFMediaSourceBase::play();
	} 
	RDK_LOG(RDK_LOG_TRACE1,"LOG.RDK.QAMSRC", "Exit %s():%d \n" , __FUNCTION__, __LINE__);
	return ret;
}


/**
 * @brief This function stops the gstreamer pipeline and stops pushing the SPTS buffer to sink.
 * It sets the gstreamer state to "GST_STATE_PAUSED".
 *
 * @return RMFResult (int)
 * @retval RMF_RESULT_NOT_INITIALIZED If the gstreamer pipeline is not initialized.
 * @retval GST_STATE_PAUSED If speed is 0 and GST pipeline set to be pauesed state.
 * @ingroup qamsrcapi
 */
RMFResult RMFQAMSrc::pause()
{
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", " %s():%d Invoking RMFMediaSourceBase::pause()\n" , __FUNCTION__, __LINE__);

		if(0 != factoryManaged)
		{
			qamsrcobjectlist* qamlistobj = NULL;

			rmf_osal_mutexAcquire(g_qamlistmutex);
			qamlistobj = qam_factory_get_entry( this);
			if (NULL == qamlistobj )
			{
				RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.QAMSRC", " %s():%d qamlistobj is NULL\n"
						, __FUNCTION__, __LINE__);
				rmf_osal_mutexRelease(g_qamlistmutex);
				return RMF_RESULT_FAILURE;
			}
			RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", " %s():%d qamlistobj->refcount %d\n"
					, __FUNCTION__, __LINE__, qamlistobj->refcount);

			if ( qamlistobj->refcount > 1 )
			{

				rmf_osal_mutexRelease(g_qamlistmutex);
				return RMF_RESULT_SUCCESS;
			}
			rmf_osal_mutexRelease(g_qamlistmutex);
			RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", " %s():%d Calling RMFMediaSourceBase::pause\n"
					, __FUNCTION__, __LINE__, qamlistobj->refcount);
		}
        return RMFMediaSourceBase::pause();
}

RMFResult RMFQAMSrc::stop()
{
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", " %s():%d Invoking RMFMediaSourceBase::stop()\n" , __FUNCTION__, __LINE__);
		if(0 != factoryManaged)
		{
			qamsrcobjectlist* qamlistobj = NULL;

			rmf_osal_mutexAcquire(g_qamlistmutex);
			qamlistobj = qam_factory_get_entry( this);
			if (NULL == qamlistobj )
			{
				RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.QAMSRC", " %s():%d qamlistobj is NULL\n"
						, __FUNCTION__, __LINE__);
				rmf_osal_mutexRelease(g_qamlistmutex);
				return RMF_RESULT_FAILURE;
			}
			RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", " %s():%d qamlistobj->refcount %d\n"
					, __FUNCTION__, __LINE__, qamlistobj->refcount);

			if ( qamlistobj->refcount > 1 )
			{

				rmf_osal_mutexRelease(g_qamlistmutex);
				return RMF_RESULT_SUCCESS;
			}
			rmf_osal_mutexRelease(g_qamlistmutex);
			RDK_LOG(RDK_LOG_INFO,"LOG.RDK.QAMSRC", " %s():%d Calling RMFMediaSourceBase::stop\n"
					, __FUNCTION__, __LINE__, qamlistobj->refcount);
		}
        return RMFMediaSourceBase::stop();
}

/**
 * @fn RMFQAMSrc::createPrivateSourceImpl()
 * @brief This API is used to create a private implementation of QAM src.
 *  RMFQAMSrcImpl is actually embedded inside RMFQAMSrc object and all the calls like
 *  init, play, pause etc... are all passed on to this impl object as function calls.
 * <ul>
 * <li> Creates and initialises RMFQAMSrcImpl instance
 *  (Ex : sets the queue size if not specified in config file, initialise the semaphore and etc...)
 * </ul>
 *
 * @param None
 *
 * @return Pointer to the Private implemention of QAM Src
 */
RMFMediaSourcePrivate* RMFQAMSrc::createPrivateSourceImpl()
{
	RDK_LOG(RDK_LOG_TRACE1,"LOG.RDK.QAMSRC", " %s():%d \n" , __FUNCTION__, __LINE__);
	return new RMFQAMSrcImpl(this);
}


/**
 * @brief This API is used to create a new GstBin with name qamsrc_bin for the source used by rmf base implementation
 *
 * @param None
 *
 * @return Returns Pointer to GstBin on Success else NULL
 * @ingroup qamsrcapi
 */
void* RMFQAMSrc::createElement()
{
	RMFQAMSrcImpl* privateImpl = NULL;
	privateImpl = (RMFQAMSrcImpl*)getPrivateSourceImpl();
	if ( privateImpl )
	{
		return privateImpl->createElement();
	}
	else
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - private impl not available\n", __FUNCTION__);
		return NULL;
	}
}/* End of createElement */


/**
 * @brief This API gets the transport stream ID from PAT once the acquisition is complete.
 *
 * @param[out] tsId Specifies the transport  stream ID
 *
 * @return Return values indicates success or failure
 * @retval RMF_SUCCESS Indicates tsId was successfully populated
 * @retval RMF_RESULT_FAILURE Indicates either private implementation of QAM src or PAT was not available
 * @ingroup qamsrcapi
 */
RMFResult RMFQAMSrc::getTSID( uint32_t &tsId)
{
	RMFQAMSrcImpl* privateImpl = NULL;
	privateImpl = (RMFQAMSrcImpl*)getPrivateSourceImpl();
	if ( privateImpl )
	{
		return privateImpl->getTSID(tsId);
	}
	else
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - private impl not available\n", __FUNCTION__);
		return RMF_RESULT_FAILURE;
	}
}

RMFResult RMFQAMSrc::enableTdtTot( bool enable)
{
	RMFQAMSrcImpl* privateImpl = NULL;
	privateImpl = (RMFQAMSrcImpl*)getPrivateSourceImpl();
	if ( privateImpl )
	{
		return privateImpl->enableTdtTot(enable);
	}
	else
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - private impl not available\n", __FUNCTION__);
		return RMF_RESULT_FAILURE;
	}
}

int32_t RMFQAMSrc::getTunerId(void)
{
  RMFQAMSrcImpl* privateImpl = NULL;
  privateImpl = (RMFQAMSrcImpl*)getPrivateSourceImpl();
  if (privateImpl)
  {
    return privateImpl->getTunerId();
  }
  return -1;
}


#ifdef USE_TRM
/**
 * @brief Constructor which sets an Id for pre-tune event and qam source element.
 *
 * @param[in] pretune_queue queue id of pretune events
 * @param[in] source RmfQamsrc instance
 *
 * @return None.
 */
RMFPretuneEventHandler::RMFPretuneEventHandler( rmf_osal_eventqueue_handle_t  pretune_queue, RMFQAMSrc* source)
	: eventQueue(pretune_queue), m_source(source)
{
	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "%s: object created. Queue is 0x%x. Source: 0x%x\n", 
		__FUNCTION__, eventQueue, m_source);
}

/**
 * @brief Default Destructor of RMFPretuneEventHandler.
 *
 * @return None.
 */
RMFPretuneEventHandler::~RMFPretuneEventHandler()
{
	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "%s: object being deleted.\n", __FUNCTION__);
}

/**
 * @fn RMFPretuneEventHandler::status( const RMFStreamingStatus& status )
 * @brief This function is used to call releasePretuneReference() for adding the current status event to the event handler.
 *
 * @param[in] status status defined by rmf elements.
 *
 * @return None.
 */
void RMFPretuneEventHandler::status( const RMFStreamingStatus& status )
{
	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "Pretune status handler: got 0x%x\n", status.m_status);	
	releasePretuneReference();
}

/**
 * @brief This function is used to call releasePretuneReference() for adding the current error status to the event handler queue.
 *
 * @param[in] err Pretune Error
 * @param[in] msg Pretune Error Handler Message.
 *
 * @return None.
 */
void RMFPretuneEventHandler::error(RMFResult err, const char* msg)
{
	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "Pretune error handler: %s.\n", msg);
	releasePretuneReference();
}

/**
 * @brief This function is used to call releasePretuneReference() for adding the stop status to the event handler queue.
 *
 * @return None.
 */
void RMFPretuneEventHandler::stopped()
{
	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "Pretune 'stopped' event handler.\n");
	releasePretuneReference();
}

void RMFPretuneEventHandler::releasePretuneReference( )
{
	rmf_Error ret;
	rmf_osal_event_handle_t event_handle;

	if(!m_source)
	{
		/* This handler has already signalled once. */
		return;
	}
	rmf_pretune_event_payload_t*  payload = new rmf_pretune_event_payload_t;
	payload->handler = this;
	payload->source= m_source;
	m_source = 0;
	
	rmf_osal_event_params_t p_event_params = {	1, 
		payload,
		0,
		NULL
		};
		
	ret = rmf_osal_event_create(RMF_OSAL_EVENT_CATEGORY_PRETUNE, RMF_QAMSRC_PRETUNE_RELEASE, &p_event_params, &event_handle);
	if(ret!= RMF_SUCCESS)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "Could not create event handle:\n");
	}

	ret =  rmf_osal_eventqueue_add(eventQueue, event_handle);
	if(ret!= RMF_SUCCESS)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "rmf_osal_eventqueue_add failed\n");
	}
}
#endif


/**
 * @brief This API is used to get the local Transport-Stream-ID (LTSID) of low level element, GstElement.
 *  LTSID is specific to the tuner and not the channel. It is used by the POD manager to enable descrambling using cable card.
 *
 * @param[out] ltsId  LTSID of GstElement to be populated
 *
 * @return [RMFResult] Return value indicates success or failure
 * @retval RMF_RESULT_SUCCESS Indicates that LTSID was successfully populated
 * @retval RMF_RESULT_FAILURE Indicates either private implementation of QAM src or low level element/GElement was not valid
 * @ingroup qamsrcapi
 */
RMFResult RMFQAMSrc::getLTSID( unsigned char &ltsId)
{
	RMFQAMSrcImpl* privateImpl = NULL;
	privateImpl = (RMFQAMSrcImpl*)getPrivateSourceImpl();
	if ( privateImpl )
	{
		return privateImpl->getLTSID(ltsId);
	}
	else
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - private impl not available\n", __FUNCTION__);
		return RMF_RESULT_FAILURE;
	}
}

/**
 * @brief This function is used to attach the usages in the token.
 * The "usage" identifier represents that in what purpose the QAM source object is used for.
 *
 * @param[in] usage RMFqamsrc usages as LIVE, TSB and RECORD
 * @param[in] token Unknown
 *
 * @return None.
 * @ingroup qamsrcapi
 */
void RMFQAMSrc::attachUsage(usage_t usage, const std::string &token)
{
	if(0 == token.compare(QAMSRC_DEFAULT_TOKEN))
	{
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "%s: skip attaching default token\n", __FUNCTION__);
		return;
	}
	adjustUsageForLowPriorityTokens(usage, token);
	rmf_osal_mutexAcquire(m_mutex);
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s - attaching Usage [%s][%x] to [%x]\n", __FUNCTION__, token.c_str(), usage, usages[token]);
	usages[token] = usages[token] | usage;
	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "%s - after attaching token count %d\n", __FUNCTION__, usages.size());
	rmf_osal_mutexRelease(m_mutex);
}

/**
 * @brief This function is used to detach the usage. If token is *found*, its usages are removed.
 *
 * @param[in] usage RMFqamsrc usages as LIVE, TSB and RECORD
 * @param[in] token UNKNOWN
 *
 * @return None.
 * @ingroup qamsrcapi
 */
void RMFQAMSrc::detachUsage(usage_t usage, const std::string &token)
{
	if(0 == token.compare(QAMSRC_DEFAULT_TOKEN))
	{
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "%s: skip detaching default token\n", __FUNCTION__);
		return;
	}
	adjustUsageForLowPriorityTokens(usage, token);
	rmf_osal_mutexAcquire(m_mutex);
	std::map<std::string,uint32_t>::iterator it = usages.find(token);
	if (it != usages.end()) {
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.QAMSRC", "%s - detaching Usage [%s][%x] from [%x]\n", __FUNCTION__, token.c_str(), usage, usages[token]);
	    usages[token] = usages[token] &  (~usage);
	    if (usages[token] == 0) {
	    	/* no more usages. Remove it */
	    	usages.erase(it);
	    }
 	}
	else {
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - detaching Usage [%s] token not found\n", __FUNCTION__, token.c_str());
	}
	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "%s - after detaching token count %d\n", __FUNCTION__, usages.size());
	rmf_osal_mutexRelease(m_mutex);
}

/**
 * @fn RMFQAMSrc::dumpUsage(void)
 * @brief This function is used to print all existing usages.
 *
 * @return None.
 * @ingroup qamsrcapi
 */
void RMFQAMSrc::dumpUsage(void)
{
	rmf_osal_mutexAcquire(m_mutex);
    if (usages.size() != 0) {
        std::map<std::string,uint32_t>::iterator it = usages.begin();
        while (it != usages.end()) {
            RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "%s - Usage token=[%s] uses=[%x] \n", __FUNCTION__, (it)->first.c_str(), (it)->second);
            it++;
        }
    }
    else {
        RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "%s - qamsrc has no usage \n", __FUNCTION__);
    }
	rmf_osal_mutexRelease(m_mutex);
}

void RMFQAMSrc::flushUsages(void)
{
	rmf_osal_mutexAcquire(m_mutex);
	usages.clear();
	rmf_osal_mutexRelease(m_mutex);
}
/* If there is a TRM outage, applications may still request LIVE and RECORD tuners,
 * but using a fake token. Adjust the usages for these qamsource objects so that
 * they can be preempted when TRM is up again and in control of tuner management.*/
void RMFQAMSrc::adjustUsageForLowPriorityTokens(usage_t &usage, const std::string &token)
{
	uint32_t shorterStringLength = (strlen(QAMSRC_UNRESERVED_TOKEN) > token.size() ? token.size() : strlen(QAMSRC_UNRESERVED_TOKEN));
	if(0 == strncmp(QAMSRC_UNRESERVED_TOKEN, token.c_str(), shorterStringLength) && (0 < shorterStringLength))
	{
		//This is a fake token. It deserves a lower priority.
		if(USAGE_LIVE == usage)
		{
			usage = USAGE_LIVE_UNRESERVED;
			RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.QAMSRC", "%s: TRM dummy token detected. Usage adjusted to 0x%x\n", __FUNCTION__, usage);
		}
		else if (USAGE_RECORD == usage)
		{
			usage = USAGE_RECORD_UNRESERVED;
			RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.QAMSRC", "%s: TRM dummy token detected. Usage adjusted to 0x%x\n", __FUNCTION__, usage);
		}
		//Note: all other usages are untouched.

	}
}

RMFResult RMFQAMSrc::getStatus(qamsrc_status_t &status)
{
	return ((RMFQAMSrcImpl*)getPrivateSourceImpl())->getStatus(status);
}

qamsrcobjectlist* qam_factory_get_entry(RMFQAMSrc * qamsrc)
{
	qamsrcobjectlist* qamlistobj;

	qamlistobj = g_qamsrclist;
	while ( NULL != qamlistobj)
	{
		if ( qamsrc ==  qamlistobj->qamsrc )
		{
			break;
		}
		qamlistobj = qamlistobj->next;
	}
	return qamlistobj;
}

rmf_osal_Bool check_for_free_qamsrc()
{
	/* Check for any cached qamsource objects with no users.*/
	qamsrcobjectlist* qamlistobj = g_qamsrclist;
	while(NULL != qamlistobj)
	{
		if(0 == qamlistobj->refcount)
		{
			return TRUE;
		}
		qamlistobj = qamlistobj->next;
	}
	/* No unused cached tuners exist. Check whether we have tuners
	 * that aren't used at all. */
	if(RMFQAMSrcImpl::numberOfTuners > (g_qamsrcobjectno + g_lowlevelelementsno))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

void RMFQAMSrc::notifyQAMReady()
{
	IARM_Bus_SYSMgr_EventData_t eventData;
	eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_QAM_READY;
	eventData.data.systemStates.state = 1;
	eventData.data.systemStates.error = 0;
	IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
}

#ifdef USE_TRM
/* Start of changes for TRM-pretune integration. */
static void getTRMVersion() ;

static int connect_to_trm()
{
	int socket_fd ;
	int socket_error = 0;
	struct sockaddr_in trm_address;

	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.QAMSRC", "Enter %s():%d \n" , __FUNCTION__, __LINE__);

	rmf_osal_mutexAcquire( g_mutex);

	if (0 == is_connected)
	{
		RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "%s:%d : Connecting to remote\n" , __FUNCTION__, __LINE__);
        
		trm_address.sin_family = AF_INET;
		trm_address.sin_addr.s_addr = inet_addr(ip);
		trm_address.sin_port = htons(port);
		if (-1 == trm_socket_fd )
		{
			socket_fd = socket(AF_INET, SOCK_STREAM, 0);
		}
		else
		{
			socket_fd = trm_socket_fd;
		}
        
		while(1)
		{
			int retry_count = 10;
			socket_error = connect(socket_fd, (struct sockaddr *) &trm_address, sizeof(struct sockaddr_in));
			if (socket_error == ECONNREFUSED  && retry_count > 0) 
			{
				RDK_LOG(RDK_LOG_WARN, "LOG.RDK.QAMSRC", "%s:%d : TRM Server is not started...retry to connect\n" , __FUNCTION__, __LINE__);
				sleep(2);
				retry_count--;
			}
			else 
			{
			   break;
			}
		}

		if (socket_error == 0)
		{
			RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "%s:%d : Connected\n" , __FUNCTION__, __LINE__);

			int current_flags = fcntl(socket_fd, F_GETFL, 0);
			current_flags &= (~O_NONBLOCK);
			if ( fcntl(socket_fd, F_SETFL, current_flags) < 0 )
			{
				RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s:%d : fcntl error\n" , __FUNCTION__, __LINE__);
				close(socket_fd);
				trm_socket_fd = -1;
				socket_error = -1;
			}
			else
			{
				trm_socket_fd = socket_fd;
				is_connected = 1;
				getTRMVersion();
			}
		}
		else 
		{
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s:%d : socket_error %d, closing socket\n" , __FUNCTION__, __LINE__, socket_error);
			close(socket_fd);
			trm_socket_fd = -1;
		}
	}
	rmf_osal_mutexRelease( g_mutex);
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.QAMSRC", "%s:%d : returning %d\n",__FUNCTION__, __LINE__, socket_error);
	return socket_error;
}

static void processBuffer( const char* buf, int len)
{
	if (buf != NULL)
	{
		RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.QAMSRC","Response %s\n", buf);
		std::vector<uint8_t> response;
		response.insert( response.begin(), buf, buf+len);
		QAMSrcMessageProcessor qamProc;
		TRM::JsonDecoder jdecoder( qamProc);
		jdecoder.decode( response);
	}
}

static bool url_request_post( const char *payload, int payload_length)
{
	unsigned char *buf = NULL;
	bool ret = false;
//	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "POSTing data to URL at %s", url);
	connect_to_trm();
	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.QAMSRC", "Connection status to TRM  %d\n", is_connected);
	
	if (is_connected ) 
	{
		if (payload_length != 0) 
		{
			/* First prepend header */
			static int message_id = 0x1000;
			const int header_length = 16;
			buf = (unsigned char *) malloc(payload_length + header_length);
			int idx = 0;
			/* Magic Word */
			buf[idx++] = 'T';
			buf[idx++] = 'R';
			buf[idx++] = 'M';
			buf[idx++] = 'S';
			/* Type, set to UNKNOWN, as it is not used right now*/
			buf[idx++] = (UNKNOWN & 0xFF000000) >> 24;
			buf[idx++] = (UNKNOWN & 0x00FF0000) >> 16;
			buf[idx++] = (UNKNOWN & 0x0000FF00) >> 8;
			buf[idx++] = (UNKNOWN & 0x000000FF) >> 0;
			/* Message id */
			++message_id;
            const unsigned int recorder_connection_id = 0XFFFFFF02;
			buf[idx++] = (recorder_connection_id & 0xFF000000) >> 24;
			buf[idx++] = (recorder_connection_id & 0x00FF0000) >> 16;
			buf[idx++] = (recorder_connection_id & 0x0000FF00) >> 8;
			buf[idx++] = (recorder_connection_id & 0x000000FF) >> 0;
			/* Payload length */
			buf[idx++] = (payload_length & 0xFF000000) >> 24;
			buf[idx++] = (payload_length & 0x00FF0000) >> 16;
			buf[idx++] = (payload_length & 0x0000FF00) >> 8;
			buf[idx++] = (payload_length & 0x000000FF) >> 0;

			for (int i =0; i< payload_length; i++)
				buf[idx+i] = payload[i];
			RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.QAMSRC", "========REQUEST MSG================================================\n[");
			for (idx = 0; idx < (header_length); idx++) {
				printf( "%02x", buf[idx]);
			}
			for (; idx < (payload_length + header_length); idx++) {
				printf("%c", buf[idx]);
			}

			/* Write payload from fastcgi to TRM */
			int write_trm_count = write(trm_socket_fd, buf, payload_length + header_length);
			RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.QAMSRC", "Send to TRM  %d vs expected %d\n", write_trm_count, payload_length + header_length);
			free(buf);
			buf = NULL;

			if (write_trm_count == 0) 
			{
				is_connected = 0;
				RDK_LOG(RDK_LOG_WARN, "LOG.RDK.QAMSRC", "%s:%d : write_trm_count 0\n", __FUNCTION__, __LINE__);
				/* retry connect after write failure*/
			}
			else
			{
				ret = true;
			}
		}
	}
	return ret;
}

static void getTRMVersion() 
{
	uuid_t value;
	char guid[37];
	uuid_generate(value);
	uuid_unparse(value, guid);
	TRM::GetVersion msg(guid);
	std::vector<uint8_t> out;
	JsonEncode(msg, out);
	out.push_back( 0 );
	url_request_post( (char *) &out[0], out.size());
 }

static void QamsrcTunerReservationHelperThread (void* arg)
{
	int read_trm_count = 0;
	char *buf = NULL;
	const int header_length = 16;
	int idx = 0;
	unsigned int payload_length = 0;
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "Enter %s():%d \n" , __FUNCTION__, __LINE__);
	connect_to_trm();
	if( !is_connected)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s(): couldn't connect to TRM server. Exiting\n" , __FUNCTION__);
		return;
	}
	
	while (1)
	{

		if ( is_connected )
		{
			buf = (char *) malloc(header_length);
			if (buf == NULL)
			{
				RDK_LOG(RDK_LOG_WARN, "LOG.RDK.QAMSRC", "%s:%d :  Malloc failed for %d bytes \n", __FUNCTION__, __LINE__, header_length);
				continue;
			}

			/* Read Response from TRM, read header first, then payload */
			memset(buf, 0, header_length);
			read_trm_count = read(trm_socket_fd, buf, header_length);
			RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.QAMSRC", "Read Header from TRM %d vs expected %d\n", read_trm_count, header_length);
			RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.QAMSRC", "=====RESPONSE HEADER===================================================\n[");

			for (idx = 0; idx < (header_length); idx++) {
				printf( "%02x", buf[idx]);
			}
			printf("\n");

			if (read_trm_count == header_length) 
			{
				int payload_length_offset = 12;
				payload_length =((((unsigned char)(buf[payload_length_offset+0])) << 24) |
								 (((unsigned char)(buf[payload_length_offset+1])) << 16) |
								 (((unsigned char)(buf[payload_length_offset+2])) << 8 ) |
								 (((unsigned char)(buf[payload_length_offset+3])) << 0 ));
				if ((payload_length > 0) && (payload_length < MAX_PAYLOAD_LEN))
				{
					free( buf);
					buf = NULL;
					RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.QAMSRC", "TRM Response payloads is %d and header %d\n", payload_length, header_length);

					buf = (char *) malloc(payload_length+1);
					memset(buf, 0, payload_length+1);
					read_trm_count = read(trm_socket_fd, buf, payload_length);
					RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.QAMSRC", "Read Payload from TRM %d vs expected %d\n", read_trm_count, payload_length);

					if (read_trm_count != 0) 
					{
						buf[payload_length] = '\0';
						processBuffer(buf, read_trm_count);
						free(buf);
						buf = NULL;
					}
					else 
					{
						/* retry connect after payload-read failure*/
						is_connected = 0;
						free(buf);
						buf = NULL;
						if(payload_length > MAX_PAYLOAD_LEN)
						{
							RDK_LOG(RDK_LOG_WARN, "LOG.RDK.QAMSRC", "%s:%d : Oversized payload_length = 0x%X\n", __FUNCTION__, __LINE__, payload_length);
						}
						RDK_LOG(RDK_LOG_WARN, "LOG.RDK.QAMSRC", "%s:%d : read_trm_count 0\n", __FUNCTION__, __LINE__);
					}
				}
				else
				{
					/* retry connect after payload-read failure*/
					is_connected = 0;
					free(buf);
					buf = NULL;
					RDK_LOG(RDK_LOG_WARN, "LOG.RDK.QAMSRC", "%s:%d : read_trm_count 0\n", __FUNCTION__, __LINE__);

				}
			}
			else 
			{
				RDK_LOG(RDK_LOG_WARN, "LOG.RDK.QAMSRC", "%s:%d : read_trm_count %d\n", __FUNCTION__, __LINE__, read_trm_count);
				free(buf);
				buf = NULL;
				/* retry connect after header-read failure */
				is_connected = 0;
			}

		}
		else
		{
			RDK_LOG( RDK_LOG_WARN, "LOG.RDK.QAMSRC", "%s - Not connected- Sleep and retry\n", __FUNCTION__);
			sleep(1);
			connect_to_trm();
		}
	}
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.QAMSRC", "Exit %s():%d \n" , __FUNCTION__, __LINE__);
}

void QAMSrcMessageProcessor::operator() (const TRM:: NotifyTunerPretune &msg)
{
 	RMFQAMSrc::performPretune(msg.getServiceLocator().c_str());
}

void QAMSrcMessageProcessor::operator() (const TRM::UpdateTunerActivityStatus &msg)
{
      //Notify tuner activity status to registered client 
      if(RMFQAMSrc::tunerActivityUpdateCallback)
      {
          RMFQAMSrc::tunerActivityUpdateCallback(msg.getDevice(),msg.getTunerActivityStatus(),msg.getLastActivityTimeStamp(),msg.getLastActivityAction());
      }
}

/* End of changes for TRM-pretune integration. */
#endif

