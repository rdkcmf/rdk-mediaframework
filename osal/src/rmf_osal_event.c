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

#include "rmf_osal_event.h"
#include "rmf_osal_event_priv.h"
#include "rmf_osal_sync.h"
#include "rmf_osal_mem.h"
#include "rdk_debug.h"
#include <string.h> /*strdup()*/
#include <stdlib.h> /*free()*/
#include <errno.h> /*perror*/
#include <sys/time.h> /*gettimeofday*/

#if USE_SYSRES_MLT
#include "mlt_malloc.h"
#endif

/**
 * @file rmf_osal_event.c
 * @brief The rmf osal event manager creates, manages and deletes events with associated 
 * event category such as App, system, tuner, process, DVR, VOD, CA, etc.
 */

/*Common Event Manager*/
static rmf_osal_eventmanager_t * g_eventmanager = NULL;


#ifdef DEBUG_ALLOCATED_EVENTS
struct osal_event_list
{
	rmf_osal_event_t * event;
	struct osal_event_list* next;
};
struct osal_eventq_list
{
	rmf_osal_eventqueue_t * eventq;
	struct osal_eventq_list* next;
};
struct osal_eventm_list
{
	rmf_osal_eventmanager_t* eventm;
	struct osal_eventm_list* next;
};
static osal_event_list *g_osal_event_list = NULL;
static osal_eventq_list *g_osal_eventq_list = NULL;
static osal_eventm_list *g_osal_eventm_list = NULL;

static rmf_osal_Mutex g_osal_event_list_mutex = NULL;

void rmf_osal_debug_eventlist_dump ()
{
	printf("\n\n ===============Allocated Event List=====\n");
	rmf_osal_mutexAcquire(g_osal_event_list_mutex);
	osal_event_list * list = g_osal_event_list;
	while (list)
	{
		printf("Event type = %d, event category =%d\n", list->event->event_type, list->event->event_category);
		list = list->next;
	}
	printf("\n\n ===============Allocated EventQueue List=====\n");
	osal_eventq_list * qlist = g_osal_eventq_list;
	while (qlist)
	{
		printf("Eventqueue = 0x%x, Name =%s\n", qlist->eventq, qlist->eventq->name);
		qlist = qlist->next;
	}
	printf("\n\n ===============Allocated EventManager List=====\n");
	osal_eventm_list * mlist = g_osal_eventm_list;
	while (mlist)
	{
		printf("Eventmanager = 0x%x\n", mlist->eventm);
		mlist = mlist->next;
	}
	rmf_osal_mutexRelease(g_osal_event_list_mutex);
	printf("\n\n =================================\n");
}
#endif

/**
 * @brief This function initializes the RMF OSAL event manager module. This API must
 * be called only once per boot cycle. It must be called before any other OSAL
 * Event module APIs are called.  
 *
 * @return The RMF_OSAL error code is returned if the initialization fails, otherwise RMF_SUCCESS is returned.
 */
rmf_Error rmf_osal_event_init()
{
#ifdef DEBUG_ALLOCATED_EVENTS
	rmf_osal_mutexNew(&g_osal_event_list_mutex);
#endif
	/*Create the common event manager handle*/
	return rmf_osal_eventmanager_create( (rmf_osal_eventmanager_handle_t*) &g_eventmanager);
}

/**
 * @brief This function is used to create an event with the specified event category, event type and parameters.
 *
 * @param event_category The event category of this event. New category shall be
 * added to rmf_osal_event_category_t enum in event header file.
 * @param event_type A unique number to specify an event.
 * @param p_event_params Pointer to event parameter structure which contains event specific 
 *           metadata to be set. If any member variable of this structure is not used, it shall be set to 0/NULL. 
 *           If params is not required for this event, NULL shall be passed.
 * @param p_event_handle A pointer for returning the handle of the new event.
 * @return Returns relevant RMF_OSAL error code if the create fails, otherwise RMF_SUCCESS is returned.
 */
rmf_Error rmf_osal_event_create( rmf_osal_event_category_t event_category , uint32_t event_type, 
		rmf_osal_event_params_t* p_event_params, rmf_osal_event_handle_t* p_event_handle)
{
	rmf_Error ret = RMF_SUCCESS;
	rmf_osal_event_t * event;

	ret = rmf_osal_memAllocP(RMF_OSAL_MEM_EVENT, sizeof(rmf_osal_event_t), (void**)&event);
	if (RMF_SUCCESS != ret)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
							  "rmf_osal_memAllocP() failed\n");
		return ret;
	}

	ret = rmf_osal_mutexNew(&event->mutex);
	if (RMF_SUCCESS != ret)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
							  "rmf_osal_mutexNew() failed\n");
		rmf_osal_memFreeP(RMF_OSAL_MEM_EVENT, event);
		return ret;
	}

	event->event_type = event_type;
	event->event_category  = event_category ;

	/*Fill event specific data if provided*/
	if ( NULL  != p_event_params)
	{
		event->free_payload_fn = p_event_params->free_payload_fn;
		event->data = p_event_params->data;
		event->data_extension = p_event_params->data_extension;
		event->priority = p_event_params->priority;
	}
	else
	{
		event->free_payload_fn = NULL;
		event->data = NULL;
		event->data_extension = 0;
		event->priority = 0;
	}

#ifdef DEBUG_RMF_OSAL_EVENTS
	event->signature = RMF_OSAL_EVENT_SIG;
#endif
	* p_event_handle = (rmf_osal_event_handle_t)event;

#ifdef DEBUG_ALLOCATED_EVENTS
	osal_event_list* eventlistentry;
	ret =  rmf_osal_memAllocP (RMF_OSAL_MEM_EVENT, sizeof (osal_event_list), (void**)&eventlistentry);
	if (RMF_SUCCESS != ret )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.EVENT", "%s - rmf_osal_memAllocP failed.\n", __FUNCTION__);
		return ret;
	}
	rmf_osal_mutexAcquire(g_osal_event_list_mutex);
	eventlistentry->event = event;
	eventlistentry->next = g_osal_event_list;

	g_osal_event_list = eventlistentry;
	rmf_osal_mutexRelease(g_osal_event_list_mutex);
#endif
	return ret;
}

/**
 * @brief This function is used to delete previously created event and free 
 * up the resources associated with it. 
 *
 * If event is dispatched through event manager, this  function notifies event manager
 * that the caller has finished processing the event. Event manager deletes the event after
 * all users call this function.
 *
 * @param event_handle Handle of event to be deleted.
 * @return Returns relevant RMF_OSAL error code on failure, otherwise RMF_SUCCESS
 *          is returned.
 */
rmf_Error rmf_osal_event_delete( rmf_osal_event_handle_t event_handle)
{
	rmf_Error ret = RMF_SUCCESS;
	rmf_osal_event_t * event = (rmf_osal_event_t *)event_handle;
	RMF_OSAL_EVENT_VALIDATE_EVENT_PTR(event);

	RMF_OSAL_EVENT_LOCK(event);

	/* If memory managed, decrement user count and return success if there is positive 
	number of users still accessing the event.*/
	if ( TRUE == event->memory_managed )
	{
		event->users --;
		if ( event->users > 0)
		{
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.EVENT",
							  "rmf_osal_event_delete() users = %d\n", event->users);
			RMF_OSAL_EVENT_UNLOCK( event);
			return ret;
		}
	}
#ifdef DEBUG_RMF_OSAL_EVENTS
	event->signature = 0;
#endif

	RMF_OSAL_EVENT_UNLOCK(event);

#ifdef DEBUG_ALLOCATED_EVENTS
	osal_event_list* eventlistentry, *eventlistentryprev = NULL , *eventlistentrytodelete = NULL;
	rmf_osal_mutexAcquire(g_osal_event_list_mutex);
	eventlistentry = g_osal_event_list;

	while ( NULL != eventlistentry)
	{
		if ( event ==  eventlistentry->event)
		{
			break;
		}
		eventlistentryprev = eventlistentry;
		eventlistentry = eventlistentry->next;
	}
	if (NULL == eventlistentry )
	{
		RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.EVENT", " %s():%d Cannot found matching entry\n"
			, __FUNCTION__, __LINE__);
		rmf_osal_mutexRelease(g_osal_event_list_mutex);
		return ret;
	}

	if (eventlistentryprev )
	{
		eventlistentryprev->next = eventlistentry->next;
	}
	if (g_osal_event_list == eventlistentry)
	{
		g_osal_event_list = eventlistentry->next;
	}
	eventlistentrytodelete = eventlistentry;
	rmf_osal_memFreeP ( RMF_OSAL_MEM_EVENT, eventlistentrytodelete );
	rmf_osal_mutexRelease(g_osal_event_list_mutex);
#endif

	/*Invoke the callback function to free the payload if registered*/
	if (event->free_payload_fn )
	{
		event->free_payload_fn (event->data);
	}

	ret = rmf_osal_mutexDelete(event->mutex);
	if (RMF_SUCCESS != ret)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
							  "rmf_osal_mutexNew() failed\n");
	}
	ret = rmf_osal_memFreeP(RMF_OSAL_MEM_EVENT, event);
	if (RMF_SUCCESS != ret)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
							  "rmf_osal_memFreeP() failed\n");
		return ret;
	}
	return ret;
}

/**
 * @brief This function will create an eventqueue with specified name.
 *
 * @param name The name of eventqueue.
 * @param p_eventqueue_handle A pointer for returning the handle of the new eventqueue.
 * @return Returns relevant RMF_OSAL error code if the create fails, otherwise RMF_SUCCESS is returned.
 */
rmf_Error rmf_osal_eventqueue_create(const uint8_t* name,
		rmf_osal_eventqueue_handle_t* p_eventqueue_handle)
{
	rmf_Error ret = RMF_SUCCESS;
	rmf_osal_eventqueue_t * eventqueue;
	int i;

	ret = rmf_osal_memAllocP(RMF_OSAL_MEM_EVENT, sizeof(rmf_osal_eventqueue_t), (void**)&eventqueue);
	if (RMF_SUCCESS != ret)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
							  "rmf_osal_memAllocP() failed\n");
		return ret;
	}
	ret = rmf_osal_mutexNew(&eventqueue->mutex);
	if (RMF_SUCCESS != ret)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
							  "rmf_osal_mutexNew() failed\n");
		rmf_osal_memFreeP(RMF_OSAL_MEM_EVENT, eventqueue);
		return ret;
	}
	eventqueue->name = (uint8_t*)strdup((char*)name);
	if (NULL == eventqueue->name)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
							  "strdup() failed\n");
		rmf_osal_mutexDelete(eventqueue->mutex);
		rmf_osal_memFreeP(RMF_OSAL_MEM_EVENT, eventqueue);
		return RMF_OSAL_ENOMEM;
	}
	if (sem_init (&eventqueue->sem, 0, 0) != 0)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
							  "sem_init() failed\n");
		free(eventqueue->name);
		rmf_osal_mutexDelete(eventqueue->mutex);
		rmf_osal_memFreeP(RMF_OSAL_MEM_EVENT, eventqueue);
		return RMF_OSAL_ENOMEM;
	}
	eventqueue->eventlist = NULL;
	eventqueue->event_count = 0;
	for (i = 0; i<MAX_EVENTMANAGER_PER_QUEUE; i++ )
	{
		eventqueue->eventmanager[i] = NULL;
	}
#ifdef DEBUG_RMF_OSAL_EVENTS
	eventqueue->signature = RMF_OSAL_EVENT_SIG;
#endif
	* p_eventqueue_handle = (rmf_osal_eventqueue_handle_t)eventqueue;

#ifdef DEBUG_ALLOCATED_EVENTS
	osal_eventq_list* eventlistentry;
	ret =  rmf_osal_memAllocP (RMF_OSAL_MEM_EVENT, sizeof (osal_eventq_list), (void**)&eventlistentry);
	if (RMF_SUCCESS != ret )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.EVENT", "%s - rmf_osal_memAllocP failed.\n", __FUNCTION__);
		return ret;
	}
	rmf_osal_mutexAcquire(g_osal_event_list_mutex);
	eventlistentry->eventq = eventqueue;
	eventlistentry->next = g_osal_eventq_list;

	g_osal_eventq_list = eventlistentry;
	rmf_osal_mutexRelease(g_osal_event_list_mutex);
#endif

	return ret;
}

/**
 * @brief This function is used to delete previously created eventqueue.
 *
 * @param eventqueue_handle Handle of eventqueue to be deleted.
 * @return Returns relevant RMF_OSAL error code on failure, otherwise RMF_SUCCESS is returned.
 */
rmf_Error rmf_osal_eventqueue_delete(rmf_osal_eventqueue_handle_t eventqueue_handle)
{
	rmf_Error ret = RMF_SUCCESS;
	rmf_osal_eventqueue_t * eventqueue = (rmf_osal_eventqueue_t *)eventqueue_handle;
	int i;
	int num_eventmanagers = 0;
	void* eventmanager[MAX_EVENTMANAGER_PER_QUEUE];

	RMF_OSAL_EVENT_VALIDATE_EVENT_PTR(eventqueue);

	RMF_OSAL_EVENT_LOCK(eventqueue);
	/*Unregister from event managers associated with this queue*/
	for (i = 0; i<MAX_EVENTMANAGER_PER_QUEUE; i++ )
	{
		if (eventqueue->eventmanager[i] != NULL)
		{
			eventmanager[num_eventmanagers] = eventqueue->eventmanager[i];
			num_eventmanagers++;
		}
	}
	RMF_OSAL_EVENT_UNLOCK(eventqueue);

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.EVENT",
			" Number of eventmanagers for queue(0x%x)  is %d\n", eventqueue, num_eventmanagers);
	for ( i = 0; i < num_eventmanagers; i++ )
	{

		RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.EVENT",
			" Unregister queue(0x%x)  from em(0x%x)\n", eventqueue, eventmanager[i]);
		ret = rmf_osal_eventmanager_unregister_handler (
				(rmf_osal_eventmanager_handle_t)eventmanager[i], eventqueue_handle);
		if (RMF_SUCCESS != ret)
		{
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
								  "rmf_osal_eventmanager_unregister_handler() failed\n");
			return ret;
		}
	}
	rmf_osal_eventqueue_clear(eventqueue_handle);
#ifdef DEBUG_RMF_OSAL_EVENTS
	eventqueue->signature = 0;
#endif
	sem_destroy (&eventqueue->sem) ;
	free(eventqueue->name);
	ret = rmf_osal_mutexDelete(eventqueue->mutex);
	if (RMF_SUCCESS != ret)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
							"rmf_osal_mutexDelete() failed\n");
	}
	ret |= rmf_osal_memFreeP(RMF_OSAL_MEM_EVENT, eventqueue);
	if (RMF_SUCCESS != ret)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
							  "rmf_osal_memFreeP() failed\n");
	}
	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.EVENT",
			"%s complete, handle = 0x%x\n", __FUNCTION__, eventqueue_handle);
#ifdef DEBUG_ALLOCATED_EVENTS
	osal_eventq_list* eventlistentry, *eventlistentryprev = NULL , *eventlistentrytodelete = NULL;
	rmf_osal_mutexAcquire(g_osal_event_list_mutex);
	eventlistentry = g_osal_eventq_list;

	while ( NULL != eventlistentry)
	{
		if ( eventqueue ==  eventlistentry->eventq)
		{
			break;
		}
		eventlistentryprev = eventlistentry;
		eventlistentry = eventlistentry->next;
	}
	if (NULL == eventlistentry )
	{
		RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.EVENT", " %s():%d Cannot found matching entry\n"
			, __FUNCTION__, __LINE__);
		rmf_osal_mutexRelease(g_osal_event_list_mutex);
		return ret;
	}

	if (eventlistentryprev )
	{
		eventlistentryprev->next = eventlistentry->next;
	}
	if (g_osal_eventq_list == eventlistentry)
	{
		g_osal_eventq_list = eventlistentry->next;
	}
	eventlistentrytodelete = eventlistentry;
	rmf_osal_memFreeP ( RMF_OSAL_MEM_EVENT, eventlistentrytodelete );
	rmf_osal_mutexRelease(g_osal_event_list_mutex);
#endif
	return ret;
}

/**
 * @brief This function is is used to associate an event manager handle to event queue. This handle will be used to unregister
 * event queue from event manager prior to event queue deletion.
 *
 * @param[in] eventqueue_handle is handle of event queue.
 * @param[in] eventmanager is pointer to  event manager structure.
 * @return The RMF_OSAL error code on failure, otherwise RMF_SUCCESS is returned.
 */
static rmf_Error rmf_osal_eventqueue_associate_eventmanager (
		rmf_osal_eventqueue_handle_t eventqueue_handle ,
		rmf_osal_eventmanager_t* eventmanager)
{
	rmf_Error ret = RMF_SUCCESS;
	rmf_osal_eventqueue_t * eventqueue = (rmf_osal_eventqueue_t *)eventqueue_handle;
	void** em_location = NULL;
	rmf_osal_Bool already_associated = FALSE;
	int i;

	RMF_OSAL_EVENT_VALIDATE_EVENT_PTR(eventqueue);
	RMF_OSAL_EVENT_LOCK(eventqueue);

	for (i = 0; i<MAX_EVENTMANAGER_PER_QUEUE; i++ )
	{
		if (eventqueue->eventmanager[i] == NULL)
		{
			if (NULL == em_location)
				em_location = &eventqueue->eventmanager[i];
		}
		else if (eventqueue->eventmanager[i] == eventmanager)
		{
			already_associated = TRUE;
			break;
		}
	}

	if( FALSE == already_associated)
	{
		if ( NULL == em_location)
		{
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
						"Already associated MAX_EVENTMANAGER_PER_QUEUE (%d) EMs\n", MAX_EVENTMANAGER_PER_QUEUE);
			ret = RMF_OSAL_EINVAL;
		}
		else
		{
			*em_location = (void*)eventmanager;
			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.EVENT",
						  "Assosiated eventmanager 0x%x to  eventqueue 0x%x\n", eventmanager, eventqueue_handle);
		}
	}
	else
	{
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.EVENT",
					"Already associated eventmanager 0x%x to  eventqueue 0x%x\n", eventmanager, eventqueue_handle);
	}

	RMF_OSAL_EVENT_UNLOCK(eventqueue);
	return ret;
}

/**
 * @brief This function is used to dissociate an event manager handle to event queue. 
 *
 * @param eventqueue_handle is handle of event queue.
 * @param eventmanager is pointer to  event manager structure.
 * @return The RMF_OSAL error code on failure, otherwise RMF_SUCCESS
 *          is returned.
 */
static rmf_Error rmf_osal_eventqueue_dissociate_eventmanager (
		rmf_osal_eventqueue_handle_t eventqueue_handle ,
		rmf_osal_eventmanager_t* eventmanager)
{
	rmf_Error ret = RMF_SUCCESS;
	int i;
	rmf_osal_eventqueue_t * eventqueue = (rmf_osal_eventqueue_t *)eventqueue_handle;
	RMF_OSAL_EVENT_VALIDATE_EVENT_PTR(eventqueue);
	RMF_OSAL_EVENT_LOCK(eventqueue);

	for (i = 0; i<MAX_EVENTMANAGER_PER_QUEUE; i++ )
	{
		if (eventqueue->eventmanager[i] == eventmanager )
		{
			eventqueue->eventmanager[i] = NULL;
			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.EVENT",
						  "removing eventmanager 0x%x from eventqueue 0x%x\n", eventmanager, eventqueue_handle);
		}
	}

	RMF_OSAL_EVENT_UNLOCK(eventqueue);
	return ret;
}

/**
 * @brief This function is used used to add an event element to the queue.
 *
 * @param eventqueue is pointer to event queue structure.
 * @param event_el is pointer of element to be added to the queue.
 * @return The RMF_OSAL error code on failure, otherwise RMF_SUCCESS is returned.
 */
static rmf_Error rmf_osal_eventqueue_list_add_element(rmf_osal_eventqueue_t * eventqueue,
		rmf_osal_eventqueue_entry_t* event_el, int32_t timeout_usec)
{
	rmf_osal_eventqueue_entry_t* itr;
	rmf_osal_eventqueue_entry_t* event_el_prev;
	int ret;

	ret = rmf_osal_mutexAcquireTimed(eventqueue->mutex, timeout_usec);
	if( RMF_SUCCESS != ret ) 
	{ 
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT", 
									  "timed mutex take failed\n");
		return ret;
	}

	if( RMF_OSAL_EVENTQ_MAX_LIMIT == eventqueue->event_count )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.EVENT",
			"Maximum limit for message queue [%s] reached\n", 
					eventqueue->name);
		RMF_OSAL_EVENT_UNLOCK(eventqueue);
		return RMF_OSAL_EBUSY;
	}
		
	event_el_prev = eventqueue->eventlist;
	if (NULL == event_el_prev)
	{
		event_el->next = NULL;
		eventqueue->eventlist = event_el;
	}
	else
	{
		if (event_el_prev->event->priority < event_el->event->priority)
		{
			event_el->next = event_el_prev;
			eventqueue->eventlist = event_el;
		}
		else
		{
			for (itr = event_el_prev ; itr ; itr = itr->next)
			{
				if (itr->event->priority < event_el->event->priority)
				{
					break;
				}
				event_el_prev = itr;
			}
			event_el->next = event_el_prev->next;
			event_el_prev->next = event_el;
		}
	}
	
	eventqueue->event_count++;
	ret = sem_post (&eventqueue->sem);
	if (ret)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
						  "sem_post() failed\n");
	}
	RMF_OSAL_EVENT_UNLOCK(eventqueue);
	return RMF_SUCCESS;
}

/**
 * @brief This function is used to add an event to specified eventqueue.
 *
 * @param eventqueue_handle Handle of eventqueue.
 * @param event_handle Handle of event to be added to the queue.
 * @return Returns relevant RMF_OSAL error code on failure, otherwise RMF_SUCCESS is returned.
 */
rmf_Error rmf_osal_eventqueue_add(rmf_osal_eventqueue_handle_t eventqueue_handle,
		rmf_osal_event_handle_t event_handle)
{
	rmf_Error ret = RMF_SUCCESS;
	rmf_osal_eventqueue_entry_t* event_el;
	rmf_osal_Bool memory_managed = FALSE;
	rmf_osal_eventqueue_t * eventqueue = (rmf_osal_eventqueue_t *)eventqueue_handle;
	rmf_osal_event_t * event = (rmf_osal_event_t *)event_handle;
	RMF_OSAL_EVENT_VALIDATE_EVENT_PTR(eventqueue);
	RMF_OSAL_EVENT_VALIDATE_EVENT_PTR(event);

	RMF_OSAL_EVENT_LOCK(event);
	memory_managed = event->memory_managed;
	RMF_OSAL_EVENT_UNLOCK(event);

	if (TRUE == memory_managed )
	{
		/*Increment user count of the event*/
		RMF_OSAL_EVENT_LOCK(event);
		event->users++;
		RMF_OSAL_EVENT_UNLOCK(event);
	}

	ret = rmf_osal_memAllocP(RMF_OSAL_MEM_EVENT, sizeof(rmf_osal_eventqueue_entry_t), (void**)&event_el);
	if (RMF_SUCCESS != ret)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
							  "rmf_osal_memAllocP() failed\n");
		return ret;
	}
	event_el->event = event;
	ret = rmf_osal_eventqueue_list_add_element(eventqueue, event_el, -1);
	return ret;
}

/**
 * The <i>rmf_osal_eventqueue_addTimed()</i> function will add an event to specified eventqueue with in the time out.
 *
 * @param eventqueue_handle is handle of eventqueue.
 * @param event_handle is handle of event to be added to the queue.
 * @return The RMF_OSAL error code on failure, otherwise <i>RMF_SUCCESS<i/>
 *          is returned.
 */
rmf_Error rmf_osal_eventqueue_addTimed(rmf_osal_eventqueue_handle_t eventqueue_handle,
		rmf_osal_event_handle_t event_handle, int32_t timeout_usec)
{
	rmf_Error ret = RMF_SUCCESS;
	rmf_osal_eventqueue_entry_t* event_el;
	rmf_osal_Bool memory_managed = FALSE;
	rmf_osal_eventqueue_t * eventqueue = (rmf_osal_eventqueue_t *)eventqueue_handle;
	rmf_osal_event_t * event = (rmf_osal_event_t *)event_handle;
	RMF_OSAL_EVENT_VALIDATE_EVENT_PTR(eventqueue);
	RMF_OSAL_EVENT_VALIDATE_EVENT_PTR(event);

	RMF_OSAL_EVENT_LOCK(event);
	memory_managed = event->memory_managed;
	RMF_OSAL_EVENT_UNLOCK(event);

	if (TRUE == memory_managed )
	{
		/*Increment user count of the event*/
		RMF_OSAL_EVENT_LOCK(event);
		event->users++;
		RMF_OSAL_EVENT_UNLOCK(event);
	}

	ret = rmf_osal_memAllocP(RMF_OSAL_MEM_EVENT, sizeof(rmf_osal_eventqueue_entry_t), (void**)&event_el);
	if (RMF_SUCCESS != ret)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
							  "rmf_osal_memAllocP() failed\n");
		return ret;
	}
	event_el->event = event;
	ret = rmf_osal_eventqueue_list_add_element(eventqueue, event_el, timeout_usec);
	return ret;
}


/**
 * This is used to pop one element from the top of event queue. If the queue is empty,
 * the call will not block and the return will be RMF_OSAL_ENODATA.
 *
 * @param eventqueue is pointer of eventqueue structure.
 * @param event_el is pointer to retrieve the event element from the queue.
 * @return The RMF_OSAL error code on failure, otherwise RMF_SUCCESS is returned.
 */
static rmf_Error rmf_osal_eventqueue_list_pop_element(rmf_osal_eventqueue_t * eventqueue,
		rmf_osal_eventqueue_entry_t** event_el)
{
	rmf_Error ret = RMF_SUCCESS;
	RMF_OSAL_EVENT_LOCK(eventqueue);
	if ( NULL == eventqueue->eventlist)
	{
		ret = RMF_OSAL_ENODATA;
	}
	else
	{
		* event_el = eventqueue->eventlist;
		eventqueue->eventlist = eventqueue->eventlist->next;
		if( 0 == eventqueue->event_count )
		{
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
						  "Invalid message count in [%s] queue\n", eventqueue->name);			
		}
		else
		{
			eventqueue->event_count--;
		}
	}
	RMF_OSAL_EVENT_UNLOCK(eventqueue);
	return ret;
}

/**
 * @brief This function is used to wait till next event is available on the eventqueue and 
 * retrieves the event on return. If the event list is empty, this will wait for the next event
 * to be available in the eventqueue. 
 *
 * @param eventqueue_handle Handle of eventqueue.
 * @param p_event_handle Pointer to retrieve the event handle from the queue.
 * @param p_event_category Event category of the retrieved event. Client shall pass NULL if data is
 *      not required.
 * @param p_event_type Pointer to retrieve event type.
 * @param p_event_params Pointer to retrieve event specific data. Client shall pass NULL if data is
 *      not required.
 * @return Returns relevant RMF_OSAL error code on failure, otherwise RMF_SUCCESS is returned.
 */
rmf_Error rmf_osal_eventqueue_get_next_event( rmf_osal_eventqueue_handle_t eventqueue_handle,
		rmf_osal_event_handle_t *p_event_handle, rmf_osal_event_category_t* p_event_category ,
		uint32_t *p_event_type, rmf_osal_event_params_t* p_event_params)
{
	rmf_Error ret = RMF_SUCCESS;
	rmf_osal_eventqueue_entry_t* event_el;
	rmf_osal_event_t * event;
	rmf_osal_eventqueue_t * eventqueue = (rmf_osal_eventqueue_t *)eventqueue_handle;

	RMF_OSAL_EVENT_VALIDATE_EVENT_PTR(eventqueue);

	while (1)
	{
		if (sem_wait(&eventqueue->sem) != -1)
		{
			ret = rmf_osal_eventqueue_list_pop_element(eventqueue,
					&event_el);
			if (RMF_SUCCESS != ret )
			{
				continue;
			}
			break;
		}
		else
		{
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
								  "sem_wait() failed\n");
			return RMF_OSAL_EINVAL;
		}
	}
	event = event_el->event;
	ret = rmf_osal_memFreeP(RMF_OSAL_MEM_EVENT, (void*)event_el);
	if (RMF_SUCCESS != ret)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
							  "rmf_osal_memFreeP() failed\n");
		return ret;
	}

	RMF_OSAL_EVENT_VALIDATE_AND_GET_EVENT_PARAMS(event, p_event_category , p_event_type, p_event_params);
	*p_event_handle =  (rmf_osal_event_handle_t)event; 

	return ret;
}

/**
 * @brief This function is used to retrieve the event if present, else return error.
 *
 * @param eventqueue_handle Handle of eventqueue.
 * @param p_event_handle Pointer to retrieve the event handle from the queue.
 * @param p_event_category  Event category of the retrieved event. Client shall pass NULL if data is
 *      not required.
 * @param p_event_type Pointer to retrieve event type.
 * @param p_event_params Pointer to retrieve event specific data. Client shall pass NULL if data is
 *      not required.
 * @return Returns the RMF_OSAL_ENODATA error code if no event is in queue,  RMF_OSAL error
 * code on failure, otherwise RMF_SUCCESS is returned.
 */
rmf_Error rmf_osal_eventqueue_try_get_next_event( rmf_osal_eventqueue_handle_t eventqueue_handle,
		rmf_osal_event_handle_t *p_event_handle, rmf_osal_event_category_t* p_event_category , 
		uint32_t *p_event_type, rmf_osal_event_params_t* p_event_params)
{
	rmf_Error ret = RMF_SUCCESS;
	rmf_osal_eventqueue_entry_t* event_el;
	rmf_osal_event_t * event;
	rmf_osal_eventqueue_t * eventqueue = (rmf_osal_eventqueue_t *)eventqueue_handle;
	RMF_OSAL_EVENT_VALIDATE_EVENT_PTR(eventqueue);
	sem_trywait(&eventqueue->sem);
	ret = rmf_osal_eventqueue_list_pop_element(eventqueue,
			&event_el);
	if (RMF_SUCCESS == ret )
	{
		event = event_el->event;
		ret = rmf_osal_memFreeP(RMF_OSAL_MEM_EVENT, (void*)event_el);
		if (RMF_SUCCESS != ret)
		{
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
								  "rmf_osal_memFreeP() failed\n");
			return ret;
		}
		RMF_OSAL_EVENT_VALIDATE_AND_GET_EVENT_PARAMS(event, p_event_category , p_event_type, p_event_params);
		*p_event_handle =  (rmf_osal_event_handle_t)event; 
	}
	return ret;
}

/**
 * @brief This function is used to retrieve the event if present, else wait for the event for the 
 * specified time. If event is added during the wait, retrieves the event else return error.
 *
 * @param eventqueue_handle Handle of eventqueue.
 * @param p_event_handle Pointer to retrieve the event handle from the queue.
 * @param p_event_category  Event category of the retrieved event. Client shall pass NULL if data is
 *      not required.
 * @param p_event_type Pointer to retrieve event type.
 * @param p_event_params Pointer to retrieve event specific data. Client shall pass NULL if data is
 *      not required.
 * @param timeout_usec The time in micro seconds.
 * @return Returns the RMF_OSAL_ENODATA error code if no event is in queue,  RMF_OSAL error
 * code on failure, otherwise RMF_SUCCESS is returned.
 */
rmf_Error rmf_osal_eventqueue_get_next_event_timed( rmf_osal_eventqueue_handle_t eventqueue_handle,
		rmf_osal_event_handle_t *p_event_handle, rmf_osal_event_category_t* p_event_category , uint32_t *p_event_type, 
		rmf_osal_event_params_t* p_event_params, int32_t timeout_usec)
{
 	int64_t sec, nsec;
	struct timespec time_spec;
	struct timeval time_val;
	struct timezone time_zone;
	rmf_Error ret = RMF_SUCCESS;
	rmf_osal_eventqueue_entry_t* event_el;
	rmf_osal_event_t * event;
	rmf_osal_eventqueue_t * eventqueue = (rmf_osal_eventqueue_t *)eventqueue_handle;
	RMF_OSAL_EVENT_VALIDATE_EVENT_PTR(eventqueue);

	if (0 != gettimeofday (&time_val, &time_zone))
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
							  "gettimeofday() failed %s\n",strerror(errno));
		return ret;
	}

	// perform calculation in 64 bit arithmetic to avoid overflow
	nsec = ((int64_t)time_val.tv_usec + timeout_usec) * 1000LL;
	sec = nsec / 1000000000LL;
	nsec = nsec % 1000000000LL;

	time_spec.tv_sec = time_val.tv_sec + sec;
	time_spec.tv_nsec = nsec;

	if(( sem_timedwait (&eventqueue->sem, &time_spec)) == 0)
	{
		ret = rmf_osal_eventqueue_list_pop_element(eventqueue,
				&event_el);
		if (RMF_SUCCESS == ret )
		{
			event = event_el->event;
			ret = rmf_osal_memFreeP(RMF_OSAL_MEM_EVENT, (void*)event_el);
			if (RMF_SUCCESS != ret)
			{
				RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
									  "rmf_osal_memFreeP() failed\n");
				return ret;
			}
			RMF_OSAL_EVENT_VALIDATE_AND_GET_EVENT_PARAMS(event, p_event_category , p_event_type, p_event_params);
			*p_event_handle =  (rmf_osal_event_handle_t)event; 
		}
	}
	else
	{
		RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.EVENT", "%s : Sem timeout. eventlist = 0x%x\n", 
			__FUNCTION__, eventqueue->eventlist);
		ret = RMF_OSAL_ENODATA;
	}
	return ret;
}

/**
 * @brief This function is used to clear the contents of eventqueue.
 *
 * @param eventqueue_handle is handle of eventqueue.
 * @return  Relevant RMF_OSAL error code is returned on failure, otherwise RMF_SUCCESS is returned.
 */
rmf_Error rmf_osal_eventqueue_clear ( rmf_osal_eventqueue_handle_t eventqueue_handle)
{
	rmf_Error ret = RMF_SUCCESS;
	rmf_osal_eventqueue_entry_t* event_el;
	rmf_osal_event_t * event;

	rmf_osal_eventqueue_t * eventqueue = (rmf_osal_eventqueue_t *)eventqueue_handle;

	RMF_OSAL_EVENT_VALIDATE_EVENT_PTR(eventqueue);
	do
	{
		sem_trywait(&eventqueue->sem);

		ret = rmf_osal_eventqueue_list_pop_element(eventqueue,
				&event_el);
		if (RMF_SUCCESS == ret )
		{
			event = event_el->event;
			ret = rmf_osal_memFreeP(RMF_OSAL_MEM_EVENT, (void*)event_el);
			if (RMF_SUCCESS != ret)
			{
				RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
									  "rmf_osal_memFreeP() failed\n");
				return ret;
			}
			/*Signal event deletion since event queue has done with event*/
			rmf_osal_event_delete( (rmf_osal_event_handle_t) event);
		}
	}while ( RMF_SUCCESS == ret);
	
	if (RMF_OSAL_ENODATA == ret )
	{
		/* All elements cleared */
		ret = RMF_SUCCESS;
	}
	return ret;
}

/**
 * @brief This function is used to create an eventmanager and provides the handle.
 *
 * @param p_eventmanager_handle A pointer for returning the handle of created event manager.
 * @return Returns relevant RMF_OSAL error code if the create fails, else returns RMF_SUCCESS.
 */
rmf_Error rmf_osal_eventmanager_create( rmf_osal_eventmanager_handle_t* p_eventmanager_handle)
{
	rmf_Error ret = RMF_SUCCESS;
	rmf_osal_eventmanager_t * eventmanager;
	ret = rmf_osal_memAllocP(RMF_OSAL_MEM_EVENT, sizeof(rmf_osal_eventmanager_t), (void**)&eventmanager);
	if (RMF_SUCCESS != ret)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
							  "rmf_osal_memAllocP() failed\n");
		return ret;
	}
	ret = rmf_osal_mutexNew(&eventmanager->mutex);
	if (RMF_SUCCESS != ret)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
							  "rmf_osal_mutexNew() failed\n");
		rmf_osal_memFreeP(RMF_OSAL_MEM_EVENT, eventmanager);
		return ret;
	}
	eventmanager->routelist = NULL;
#ifdef DEBUG_RMF_OSAL_EVENTS
	eventmanager->signature = RMF_OSAL_EVENT_SIG;
#endif
	* p_eventmanager_handle = (rmf_osal_event_handle_t)eventmanager;

#ifdef DEBUG_ALLOCATED_EVENTS
	osal_eventm_list* eventlistentry;
	ret =  rmf_osal_memAllocP (RMF_OSAL_MEM_EVENT, sizeof (osal_eventm_list), (void**)&eventlistentry);
	if (RMF_SUCCESS != ret )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.EVENT", "%s - rmf_osal_memAllocP failed.\n", __FUNCTION__);
		return ret;
	}
	rmf_osal_mutexAcquire(g_osal_event_list_mutex);
	eventlistentry->eventm = eventmanager;
	eventlistentry->next = g_osal_eventm_list;

	g_osal_eventm_list = eventlistentry;
	rmf_osal_mutexRelease(g_osal_event_list_mutex);
	RDK_LOG(RDK_LOG_NOTICE, "LOG.RDK.EVENT", "%s : Added  0x%x eventmanager to list\n", 
			__FUNCTION__, eventmanager);
#endif

	return ret;
}

/**
 * @brief This function is used to delete previously created event manager.
 *
 * @param eventmanager_handle Handle of event manager to be deleted.
 * @return Returns relevant RMF_OSAL error code on failure, otherwise RMF_SUCCESS is returned.
 */
rmf_Error rmf_osal_eventmanager_delete( rmf_osal_eventmanager_handle_t eventmanager_handle)
{
	rmf_Error ret = RMF_SUCCESS;
	rmf_osal_eventmanager_t * eventmanager = (rmf_osal_eventmanager_t *) eventmanager_handle;
	RMF_OSAL_EVENT_VALIDATE_EVENT_PTR(eventmanager);
#ifdef DEBUG_RMF_OSAL_EVENTS
	eventmanager->signature = 0;
#endif

#ifdef DEBUG_ALLOCATED_EVENTS
	osal_eventm_list* eventlistentry, *eventlistentryprev = NULL , *eventlistentrytodelete = NULL;
	rmf_osal_mutexAcquire(g_osal_event_list_mutex);
	eventlistentry = g_osal_eventm_list;

	while ( NULL != eventlistentry)
	{
		if ( eventmanager ==  eventlistentry->eventm)
		{
			break;
		}
		eventlistentryprev = eventlistentry;
		eventlistentry = eventlistentry->next;
	}
	if (NULL == eventlistentry )
	{
		RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.EVENT", " %s():%d Cannot found matching entry\n"
			, __FUNCTION__, __LINE__);
		rmf_osal_mutexRelease(g_osal_event_list_mutex);
		return ret;
	}

	if (eventlistentryprev )
	{
		eventlistentryprev->next = eventlistentry->next;
	}
	if (g_osal_eventm_list == eventlistentry)
	{
		g_osal_eventm_list = eventlistentry->next;
	}
	eventlistentrytodelete = eventlistentry;
	rmf_osal_memFreeP ( RMF_OSAL_MEM_EVENT, eventlistentrytodelete );
	rmf_osal_mutexRelease(g_osal_event_list_mutex);
#endif
	ret = rmf_osal_mutexDelete(eventmanager->mutex);
	if (RMF_SUCCESS != ret)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
							  "rmf_osal_mutexDelete() failed\n");
	}
	ret = rmf_osal_memFreeP(RMF_OSAL_MEM_EVENT,(void*)eventmanager);
	if (RMF_SUCCESS != ret)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
							  "rmf_osal_memFreeP() failed\n");
	}
	return ret;
}

/**
 * @brief This function is used to return the routing entry corresponding to the event category.
 *
 * @param eventmanager is pointer of event manager.
 * @param event_category  specifies category of which route to be returned.
 * @return The RMF_OSAL error code on failure, otherwise RMF_SUCCESS is returned.
 */
static rmf_osal_eventmanager_routing_entry_t* rmf_osal_eventmanager_get_route (
		rmf_osal_eventmanager_t * eventmanager , rmf_osal_event_category_t event_category)
{
	rmf_osal_eventmanager_routing_entry_t* entry = eventmanager->routelist;
	for (entry = eventmanager->routelist ; NULL != entry; entry=entry->next)
	{
		if (entry->event_category  == event_category)
		{
			break;
		}
	}
	return entry;
}

/**
 * @brief This function is used to register an event queue to event manager for a particular event category.
 *
 * @param eventmanager_handle Handle of event manager. If this is NULL, default eventmanager
 *     is used.
 * @param eventqueue_handle Handle of eventqueue.
 * @param event_category  The event category interested by the event queue.
 * @return Returns relevant RMF_OSAL error code on failure, otherwise RMF_SUCCESS is returned.
 */
rmf_Error rmf_osal_eventmanager_register_handler(
		rmf_osal_eventmanager_handle_t eventmanager_handle,
		rmf_osal_eventqueue_handle_t eventqueue_handle,
		rmf_osal_event_category_t event_category)
{
	rmf_Error ret = RMF_SUCCESS;
	rmf_osal_eventmanager_routing_entry_t* routing_entry;
	rmf_osal_eventqueue_list_entry_t* eventqueue_list_entry;
	rmf_osal_eventmanager_t * eventmanager = (rmf_osal_eventmanager_t *) eventmanager_handle;
	rmf_osal_eventqueue_t * eventqueue = (rmf_osal_eventqueue_t *)eventqueue_handle;
	RMF_OSAL_EVENT_VALIDATE_EVENTMGR_PTR(eventmanager);
	RMF_OSAL_EVENT_VALIDATE_EVENT_PTR(eventqueue);

	if (NULL == eventmanager )
	{
		if ( NULL == g_eventmanager )
		{
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
								  "Default event manager NULL. Module not initialized.\n");
			return RMF_OSAL_EINVAL;
		}
		eventmanager = g_eventmanager;
	}

	ret = rmf_osal_memAllocP(RMF_OSAL_MEM_EVENT, sizeof(rmf_osal_eventqueue_list_entry_t),
			(void**)&eventqueue_list_entry);
	if (RMF_SUCCESS != ret)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
							  "rmf_osal_memAllocP() failed\n");
		return ret;
	}
	eventqueue_list_entry->eventqueue = eventqueue;

//	RMF_OSAL_EVENT_LOCK(eventmanager);
	ret = rmf_osal_mutexAcquire(eventmanager->mutex); 
	if( RMF_SUCCESS != ret ) { 
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT", "mutex take failed\n");
		rmf_osal_memFreeP(RMF_OSAL_MEM_EVENT, (void*)eventqueue_list_entry);
		return ret;             
	}


	/* Add entry to the head of eventqueuelist*/
	routing_entry = rmf_osal_eventmanager_get_route(eventmanager, event_category);
	if ( routing_entry )
	{
		eventqueue_list_entry->next = routing_entry->eventqueuelist;
		routing_entry->eventqueuelist = eventqueue_list_entry;
	}
	else
	{
		/* Create new route entry*/
		ret = rmf_osal_memAllocP(RMF_OSAL_MEM_EVENT, sizeof(rmf_osal_eventmanager_routing_entry_t),
				(void**)&routing_entry);
		if (RMF_SUCCESS != ret)
		{
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
								  "rmf_osal_memAllocP() failed\n");
			rmf_osal_memFreeP(RMF_OSAL_MEM_EVENT, (void*)eventqueue_list_entry);
		}
		else
		{
			eventqueue_list_entry->next = NULL;
			routing_entry->event_category = event_category;
			routing_entry->eventqueuelist = eventqueue_list_entry;
			/*Add new route entry to head of routelist*/
			routing_entry->next = eventmanager->routelist;
			eventmanager->routelist = routing_entry;
		}
	}
	/* associate  event manager handle to event queue*/
	ret = rmf_osal_eventqueue_associate_eventmanager ( eventqueue_handle ,
			eventmanager);
	if (RMF_SUCCESS != ret)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
							"rmf_osal_eventqueue_associate_eventmanager() failed\n");
	}
//	RMF_OSAL_EVENT_UNLOCK(eventmanager);
	ret = rmf_osal_mutexRelease(eventmanager->mutex); 

	return ret;
}


/**
 * @brief This function is used to unregister an event queue from event manager. 
 * This shall be called if event queue does not want to receive any more events from the event manager.
 *
 * @param eventmanager_handle Handle of event manager. If this is NULL, default eventmanager
 *     is used.
 * @param eventqueue_handle Handle of eventqueue.
 * @return Returns relevant RMF_OSAL error code on failure, otherwise RMF_SUCCESS is returned.
 */
rmf_Error rmf_osal_eventmanager_unregister_handler(
		rmf_osal_eventmanager_handle_t eventmanager_handle,
		rmf_osal_eventqueue_handle_t eventqueue_handle)
{
	rmf_Error ret = RMF_SUCCESS;
	rmf_osal_eventmanager_routing_entry_t* routing_entry;
	rmf_osal_eventmanager_routing_entry_t* routing_entry_prev;
	rmf_osal_eventmanager_routing_entry_t* routing_entry_next;
	rmf_osal_eventqueue_list_entry_t* eventqueue_list_entry;
	rmf_osal_eventqueue_list_entry_t* eventqueue_list_entry_prev;
	rmf_osal_eventmanager_t * eventmanager = (rmf_osal_eventmanager_t *) eventmanager_handle;
	rmf_osal_eventqueue_t * eventqueue = (rmf_osal_eventqueue_t *)eventqueue_handle;
	rmf_osal_Bool eventqueue_entry_found = FALSE;

	RMF_OSAL_EVENT_VALIDATE_EVENTMGR_PTR(eventmanager);
	RMF_OSAL_EVENT_VALIDATE_EVENT_PTR(eventqueue);

	if (NULL == eventmanager )
	{
		/* Use common event manager*/
		if ( NULL == g_eventmanager )
		{
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
								  "Default event manager NULL. Module not initialized.\n");
			return RMF_OSAL_EINVAL;
		}
		eventmanager = g_eventmanager;
	}

	RMF_OSAL_EVENT_LOCK(eventmanager);

	routing_entry_prev = eventmanager->routelist;
	routing_entry = eventmanager->routelist;

	while ( NULL != routing_entry)
	{
		eventqueue_list_entry_prev  = routing_entry->eventqueuelist;
		eventqueue_list_entry = routing_entry->eventqueuelist;

		while( NULL != eventqueue_list_entry)
		{
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.EVENT",
								  "list->eventqueue = 0x%x, eventqueue = 0x%x\n", eventqueue_list_entry->eventqueue, eventqueue);
			if (eventqueue_list_entry->eventqueue == eventqueue)
			{
				RDK_LOG(RDK_LOG_INFO, "LOG.RDK.EVENT",
						" Found eventqueue = 0x%x eventqueue_list_entry = 0x%x routing_entry =0x%x\n",
						eventqueue,eventqueue_list_entry,routing_entry);
				eventqueue_entry_found = TRUE;

				/*Check if this is first entry in the routing_entry */
				if ( eventqueue_list_entry_prev == eventqueue_list_entry )
				{
					RDK_LOG(RDK_LOG_INFO, "LOG.RDK.EVENT",
							"This is first entry in the routing_entry\n");
					/*Check if this is the only entry in routing_entry*/
					if ( NULL == eventqueue_list_entry->next)
					{
						RDK_LOG(RDK_LOG_INFO, "LOG.RDK.EVENT",
							"This is only entry in the routing_entry\n");
						/*The routing_entry can be deleted */

						/*Check if this is first entry in the routelist */
						if ( routing_entry_prev == routing_entry )
						{
							RDK_LOG(RDK_LOG_INFO, "LOG.RDK.EVENT",
									"This is first entry in the routelist\n");
							/*Check if this is the only entry in routelist*/
							if ( NULL == routing_entry->next)
							{
								RDK_LOG(RDK_LOG_INFO, "LOG.RDK.EVENT",
										"This is only entry in the routelist\n");
								/*No more data in routelist. */
								eventmanager->routelist = NULL;
							}
							else
							{
								eventmanager->routelist = routing_entry->next;
							}
							/* routing_entry will be deleted. Update routing_entry_prev*/
							routing_entry_prev = routing_entry->next;
						}
						else /*( routing_entry_prev == routing_entry )*/
						{
							RDK_LOG(RDK_LOG_INFO, "LOG.RDK.EVENT",
									"This is not first entry in the routelist :"
									"routing_entry_prev = 0x%x routing_entry = 0x%x\n",routing_entry_prev,routing_entry);
							routing_entry_prev->next = routing_entry->next;
						}
						routing_entry_next = routing_entry->next;
						/*deleting routing_entry */
						ret = rmf_osal_memFreeP(RMF_OSAL_MEM_EVENT, (void*)routing_entry);
						if (RMF_SUCCESS != ret)
						{
							RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
											"rmf_osal_memFreeP(routing_entry = 0x%x) failed\n",
											(unsigned int) routing_entry);
						}
						routing_entry = NULL;
					}
					else /*( NULL == eventqueue_list_entry->next)*/
					{
						routing_entry->eventqueuelist = eventqueue_list_entry->next;
					}
				}
				else
				{
					/* Not the first entry in routing_entry*/
					eventqueue_list_entry_prev->next = eventqueue_list_entry->next;
				}
				/* Delete the eventqueue list entry since it is allocated at the time of registering handle.*/
				ret = rmf_osal_memFreeP(RMF_OSAL_MEM_EVENT, (void*)eventqueue_list_entry);
				if (RMF_SUCCESS != ret)
				{
					RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
										  "rmf_osal_memFreeP(eventqueue_list_entry=0x%x) failed\n", 
										  (unsigned int )eventqueue_list_entry);
				}
				break;
			}
			eventqueue_list_entry_prev = eventqueue_list_entry;
			eventqueue_list_entry = eventqueue_list_entry->next;
		}

		if (NULL != routing_entry)
		{
			routing_entry_prev = routing_entry;
			routing_entry = routing_entry->next;
		}
		else
		{
			/*routing entry deleted*/
			routing_entry = routing_entry_next;
		}
	}

	if ( FALSE == eventqueue_entry_found )
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
				"Entry couldn't be found eventqueue_list_entry = 0x%x "
				"routing_entry =0x%x\n",eventqueue_list_entry,routing_entry);
		ret = RMF_OSAL_ENODATA;
	}
	else
	{
		/* Unregister the event manager handle from event queue*/
		ret = rmf_osal_eventqueue_dissociate_eventmanager (
				eventqueue_handle ,
				(rmf_osal_eventmanager_t*) eventmanager);
		if (RMF_SUCCESS != ret)
		{
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
								"rmf_osal_eventqueue_dissociate_eventmanager() failed\n");
		}
	}
	RMF_OSAL_EVENT_UNLOCK(eventmanager);

	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.EVENT",
			"%s complete, handle = 0x%x\n", __FUNCTION__, eventqueue_handle);
	return ret;
}

/**
 * @brief This function is used to dispatch an event. Event manager will add this event to all
 * the event queues registered for the same event category  as that of the event.
 *
 * @param eventmanager_handle Handle of event manager. If this is NULL, default eventmanager
 *     is used.
 * @param event_handle Handle of event to be dispatched.
 * @return Returns relevant RMF_OSAL error code on failure, otherwise RMF_SUCCESS is returned.
 */
 rmf_Error rmf_osal_eventmanager_dispatch_event(
		rmf_osal_eventmanager_handle_t eventmanager_handle,
		rmf_osal_event_handle_t event_handle)
{
	rmf_Error ret = RMF_SUCCESS;
	rmf_osal_eventmanager_routing_entry_t* routing_entry;
	rmf_osal_eventqueue_list_entry_t* eventqueue_list_entry;
	rmf_osal_eventmanager_t * eventmanager = (rmf_osal_eventmanager_t *) eventmanager_handle;
	rmf_osal_event_t * event = (rmf_osal_event_t *)event_handle;

	RMF_OSAL_EVENT_VALIDATE_EVENTMGR_PTR(eventmanager);
	RMF_OSAL_EVENT_VALIDATE_EVENT_PTR(event);

	if (NULL == eventmanager )
	{
		if ( NULL == g_eventmanager )
		{
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
								  "Default event manager NULL. Module not initialized.\n");
			return RMF_OSAL_EINVAL;
		}
		eventmanager = g_eventmanager;
	}

	RMF_OSAL_EVENT_LOCK(event);
	event->memory_managed = TRUE;
	/*Event shall not be deleted during dispatch. So add a reference.*/
	event->users++;
	RMF_OSAL_EVENT_UNLOCK(event);

	RMF_OSAL_EVENT_LOCK(eventmanager);
	routing_entry = rmf_osal_eventmanager_get_route(eventmanager, event->event_category );
	if ( routing_entry )
	{
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.EVENT",
			"rmf_osal_eventmanager_get_route() returned 0x%x\n", (unsigned int)routing_entry);
		for ( eventqueue_list_entry = routing_entry->eventqueuelist;
				eventqueue_list_entry ;
				eventqueue_list_entry = eventqueue_list_entry->next)
		{
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.EVENT",
				"Adding to eventqueue 0x%x\n", (unsigned int)eventqueue_list_entry->eventqueue);
			ret = rmf_osal_eventqueue_add (
					(rmf_osal_eventqueue_handle_t)eventqueue_list_entry->eventqueue ,
					event_handle  );
			if (RMF_SUCCESS != ret)
			{
				RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
									  "rmf_osal_eventqueue_add() failed eventqueue=0x%x\n",(unsigned int) eventqueue_list_entry->eventqueue);
			}
		}
	}
	else
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
			"No routes found. Possibly no registered event queues\n");
		ret = RMF_OSAL_EINVAL;
	}
	RMF_OSAL_EVENT_UNLOCK(eventmanager);

	/*Delete the reference added at the beginning of dispatch function.
	This helps also in deleting the events if no eventqueue is subscribed for that category*/
	ret = rmf_osal_event_delete( event_handle);
	if (RMF_SUCCESS != ret)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.EVENT",
							  "rmf_osal_event_delete() failed event_handle=0x%x\n", event_handle);
	}
	return ret;
}

