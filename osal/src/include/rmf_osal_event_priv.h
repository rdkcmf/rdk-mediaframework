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

#ifndef RMF_OSAL_EVENT_PRIV_H_
#define RMF_OSAL_EVENT_PRIV_H_

#include "rmf_osal_event.h"
#include "rmf_osal_sync.h"
#include <semaphore.h>

#define DEBUG_RMF_OSAL_EVENTS

#define MAX_EVENTMANAGER_PER_QUEUE 64

/*Private structure to represent event*/
typedef struct rmf_osal_event_s
{
	uint32_t event_type;
	rmf_osal_event_category_t event_category ;
	uint32_t priority;
	rmf_osal_Bool memory_managed;
	volatile int users;
	void *data;
	uint32_t data_extension;
	rmf_osal_Mutex mutex;
	void *requestor;
	rmf_osal_event_free_payload_fn free_payload_fn;
#ifdef DEBUG_RMF_OSAL_EVENTS
	int signature;
#endif
}rmf_osal_event_t;

/*Element of a list of event pointers added to event queue*/
typedef struct rmf_osal_eventqueue_entry_s
{
	rmf_osal_event_t* event;
	struct rmf_osal_eventqueue_entry_s* next;
}rmf_osal_eventqueue_entry_t;

/*Private structure to represent eventqueue*/
typedef struct rmf_osal_eventqueue_s
{
	uint8_t* name;
	uint16_t event_count;
	rmf_osal_eventqueue_entry_t* eventlist; /*List of events stored*/
	rmf_osal_Mutex mutex;
	/*Replace with osal sem once it is implemented*/
	sem_t sem; /* Waiting / Signaling event*/
	void* eventmanager[MAX_EVENTMANAGER_PER_QUEUE];
#ifdef DEBUG_RMF_OSAL_EVENTS
	int signature;
#endif
}rmf_osal_eventqueue_t;

/*Element of a list of eventqueue pointers added to route*/
typedef struct rmf_osal_eventqueue_list_entry_s
{
	rmf_osal_eventqueue_t* eventqueue;
	struct rmf_osal_eventqueue_list_entry_s* next;
}rmf_osal_eventqueue_list_entry_t;


/*Private structure to save eventqueue list corresponding to a category*/
typedef struct rmf_osal_eventmanager_routing_entry_s
{
	rmf_osal_event_category_t event_category;
	rmf_osal_eventqueue_list_entry_t * eventqueuelist;
	struct rmf_osal_eventmanager_routing_entry_s* next;
}rmf_osal_eventmanager_routing_entry_t;

/*Private structure to represent eventmanager*/
typedef struct rmf_osal_eventmanager_s
{
	rmf_osal_eventmanager_routing_entry_t* routelist; /*List of route*/
	rmf_osal_Mutex mutex;
#ifdef DEBUG_RMF_OSAL_EVENTS
	int signature;
#endif
}rmf_osal_eventmanager_t;

#ifdef DEBUG_RMF_OSAL_EVENTS

/*Signature to check if it is a valid handle of osal event objects*/
#define RMF_OSAL_EVENT_SIG 0xABCDDCBA

/* This can be used to validate handle of osal event objects in a function
 * which returns type rmf_Error.
 */
#define RMF_OSAL_EVENT_VALIDATE_EVENT_PTR(event) {\
	if (!(event&& event->signature == RMF_OSAL_EVENT_SIG)) {\
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT", \
				"%s: %d: invalid handle (0x%x)\n",__FUNCTION__, __LINE__, \
				(unsigned int)event);\
		if (event ){\
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT", \
				"%s: %d: invalid signature (0x%x)\n",__FUNCTION__, __LINE__, \
				event->signature);\
		}\
		return RMF_OSAL_EINVAL;		\
	}\
}

/* This can be used to validate handle of osal eventmanager in a function
 * which returns type rmf_Error. Handle is not invalid if NULL and default
 * event manager can be used. If not NULL, check the signature
 */
#define RMF_OSAL_EVENT_VALIDATE_EVENTMGR_PTR(eventmanager) {\
	if ( eventmanager && eventmanager->signature != RMF_OSAL_EVENT_SIG) {\
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT", \
									  "invalid eventmanager handle\n");\
		return RMF_OSAL_EINVAL;		\
	}\
}
#else
#define RMF_OSAL_EVENT_VALIDATE_EVENT_PTR(event)
#define RMF_OSAL_EVENT_VALIDATE_EVENTMGR_PTR(eventmanager)
#endif

/* This can be used to take mutex of osal event objects in a function
 * which returns type rmf_Error.
 */
#define RMF_OSAL_EVENT_LOCK(event) {\
	int ret;\
	ret = rmf_osal_mutexAcquire(event->mutex); \
	if( RMF_SUCCESS != ret ) { \
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT", \
									  "mutex take failed\n");\
		return ret;		\
	}\
}

/* This can be used to unlock mutex of osal event objects in a function
 * which returns type rmf_Error.
 */
#define RMF_OSAL_EVENT_UNLOCK(event) {\
	int ret;\
	ret = rmf_osal_mutexRelease(event->mutex); \
	if( RMF_SUCCESS != ret ) { \
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT", \
									  "%s:mutex release failed\n",__FUNCTION__);\
		return ret;		\
	}\
}

#define RMF_OSAL_EVENT_VALIDATE_AND_GET_EVENT_PARAMS(event, p_event_category , p_event_type, p_event_params) { \
	RMF_OSAL_EVENT_VALIDATE_EVENT_PTR(event); \
	RMF_OSAL_EVENT_LOCK(event); \
	if (p_event_params) \
	{\
		p_event_params->data = event->data;\
		p_event_params->data_extension = event->data_extension; \
		p_event_params->free_payload_fn = event->free_payload_fn; \
		p_event_params->priority = event->priority; \
	}\
	if (p_event_type)\
	{\
		*p_event_type = event->event_type; \
	}\
	if (p_event_category )\
	{\
		*p_event_category  = event->event_category ; \
	}\
	RMF_OSAL_EVENT_UNLOCK(event); \
}

#endif /* RMF_OSAL_EVENT_PRIV_H_ */
