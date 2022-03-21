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
 * @file rmf_osal_event.h
 * @brief The header file contains prototypes for OSAL Events.
 */

/**
 * @defgroup OSAL_EVENTS OSAL Events
 * @ingroup OSAL
 * 
 * RMF OSAL event module provides interfaces for creation, dispatching, receiving
 * and managing events within a single process. It allows multiple modules to receive
 * same event. Memory management of events is also supported by this module by which
 * the event will be deleted only after all listeners have finished processing the event.
 * An event priority is used to receive the higher priority event before lower priority
 * events.
 *
 * When a module wants to send an event to the interested receivers,
 * it shall create an event object, add it to the event manager and dispatch it using API
 * provided by event manager. A common event manager must be used by both sender and receiver. 
 * If the event manager is created by the client, responsibility of handling the event at
 * both sender and receiver side lies with it. Client may also choose to use the default
 * event manager.
 *
 * <b> An OSAL event is associated with the following inter-related entities </b>
 *
 * @par Event category
 * Event category refers to a unique identifier to a group of events with a common behavior 
 * such as App, system, tuner, DVR, VOD, CA, etc.
 *
 * @par Event type
 * Each event in a particular category is identified by a unique event type. 
 * An event category and event type is required to define an event. 
 *
 * @par Event object
 * Event object represents an event of a category and an event type. It may also contain data
 * associated with the event. 
 *
 * @par Event manager
 * Event manager is responsible of notifying events to all registered receivers when a sender
 * dispatches the event. 
 *
 * @par Default Event manager
 * The event module creates an event manager by using a default handle during initialization of
 * the module. If event manager APIs apart from rmf_osal_eventmanager_delete() are called with
 * NULL handle, event module uses the default event manager handle to execute the operation.
 *
 * @par Event queue
 * Event queue is responsible for receiving an event. A module which is interested in a
 * particular event category can create an event queue and register it to receive events of that
 * event category. Receiver can wait or poll for the event using APIs provided by event queue.
 *
 * @par Sender
 * Sender is external client responsible for creating event handle, adding the event to event
 * manager for memory management and dispatching the event using event manager.
 * If sender wants to clean-up event data after all the receivers finished processing event,
 * sender has to register the payload freeing function with the event before dispatching it.
 *
 * @par Receiver
 * Receiver is the external module which is interested in the event and polls or waits for the
 * event. Receiver is responsible to create the event queue handle, register the eventqueue handle
 * with the event manager for the interested event category, get the events from the eventqueue
 * and notify the eventmanager once it has processed the event and is not using the event any
 * further if it is a memory managed event. Sender and receiver shall use common event manager handle.
 *
 * @par Memory management
 * Event manager deletes managed event only after all the receivers has processed with the event.
 * This is done using a user count for the event.
 * Before deleting a managed event, event manager calls the callback function registered by
 * the client with data pointer as the parameter so that client may free the payload. 
 *
 * @par Event Priority
 * Event priority is defined and set by dispatcher of the event. Event is added to the event queue
 * based on this priority. For example, if a number of events are present in the event queue and
 * sender want to dispatch a new event prior to all those events, then the priority of the event
 * shall be set to a higher value than priority of those events. On dispatching this new event,
 * it is added to the head of the queue. Default priority of an event is zero.
 *
 * @defgroup OSAL_EVENT_API OSAL Events API list
 * RMF OSAL Event module defines interfaces for management and use of events and event queues.
 * @ingroup OSAL_EVENTS
 *
 * @addtogroup OSAL_EVENT_TYPES OSAL Events Data Types
 * Describe the details about Data Structure used by OSAL Events.
 * @ingroup OSAL_EVENTS
 */

#ifndef RMF_OSAL_EVENT_H_
#define RMF_OSAL_EVENT_H_

#include"rmf_osal_types.h"
#include"rmf_osal_error.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define RMF_OSAL_EVENTQ_MAX_LIMIT 255

/**
 * @addtogroup OSAL_EVENT_TYPES
 * @{
 */

/*Event handle type definitions*/
typedef uint32_t rmf_osal_event_handle_t;
typedef uint32_t rmf_osal_eventqueue_handle_t;
typedef uint32_t rmf_osal_eventmanager_handle_t;

/*Call back function pointer type to be registered for freeing the payload.*/
typedef void (*rmf_osal_event_free_payload_fn) (void*);

/*Enumeration containing event categories.  All event categories shall be added to this enum.*/
typedef enum rmf_osal_event_category_e
{
	RMF_OSAL_EVENT_CATEGORY_NONE = -1,        //!< Set by default.
	RMF_OSAL_EVENT_CATEGORY_SYSTEM = 0,       //!< System category, system/kernel level items.
	RMF_OSAL_EVENT_CATEGORY_INPUT = 1,		     //!< User Input events.
	RMF_OSAL_EVENT_CATEGORY_PROCESS = 2,      //!< Process management(used by PFCThreadManager).
	RMF_OSAL_EVENT_CATEGORY_RECORD_IO = 3,     //!< RecordIO commands.
	RMF_OSAL_EVENT_CATEGORY_TUNER = 4,        //!< Tuner commands.
	RMF_OSAL_EVENT_CATEGORY_APP = 5,          //!< AppManager commands.
	RMF_OSAL_EVENT_CATEGORY_MPEG = 6,         //!< MPEG Table update.
	RMF_OSAL_EVENT_CATEGORY_TMS = 7,          //!< TMS updates.
	RMF_OSAL_EVENT_CATEGORY_MEDIA_ENGINE = 9,  //!< Error notification.
	RMF_OSAL_EVENT_CATEGORY_STORAGE = 10,  //!< Storage module.
	RMF_OSAL_EVENT_CATEGORY_POWER = 11,  //!<Power states
	RMF_OSAL_EVENT_CATEGORY_INB_FILTER = 12,  //!< Inband Section Filter events
	RMF_OSAL_EVENT_CATEGORY_OOB_FILTER = 13,  //!< OOB Section Filter events
	RMF_OSAL_EVENT_CATEGORY_DSG_FILTER = 14,  //!<b DSG Tunnel Section Filter events
	RMF_OSAL_EVENT_CATEGORY_HN_FILTER = 15,  //!<b HN Section Filter events
	RMF_OSAL_EVENT_CATEGORY_OOBSI = 16,         //!< OOB SI Table update.
	RMF_OSAL_EVENT_CATEGORY_DEBUG = 17,  //!< Debug Module
	RMF_OSAL_EVENT_CATEGORY_POD = 18,  //!< POD manager
	RMF_OSAL_EVENT_CATEGORY_HN = 19, //!< HN Sink
	RMF_OSAL_EVENT_CATEGORY_QAM = 20,  //!< QAMSRC

	RMF_OSAL_EVENT_CATEGORY_COM_DOWNL_MANAGER  = 21, //!< Common download
	RMF_OSAL_EVENT_CATEGORY_APP_INFO = 22, //!< App Info
	RMF_OSAL_EVENT_CATEGORY_CM_RAW_APDU = 23, //!< Raw APDU    
	RMF_OSAL_EVENT_CATEGORY_CardManagerAIEvents=24, //!< AI events
	RMF_OSAL_EVENT_CATEGORY_CardManagerMMIEvents = 25, //!< MMI events
	RMF_OSAL_EVENT_CATEGORY_CARD_MANAGER  = 25, //!< general events
	RMF_OSAL_EVENT_CATEGORY_TABLE_MANAGER = 26,
	RMF_OSAL_EVENT_CATEGORY_SYSTIME =27,
	RMF_OSAL_EVENT_CATEGORY_CARD_RES =28,
   	RMF_OSAL_EVENT_CATEGORY_CARD_STATUS=29,
	RMF_OSAL_EVENT_CATEGORY_RS_MGR=30,

	RMF_OSAL_EVENT_CATEGORY_MMI=31,
	RMF_OSAL_EVENT_CATEGORY_HOMING=32,
	RMF_OSAL_EVENT_CATEGORY_XCHAN=33,
	RMF_OSAL_EVENT_CATEGORY_HOST_CONTROL=34,
	RMF_OSAL_EVENT_CATEGORY_CA=35,
	RMF_OSAL_EVENT_CATEGORY_CP=36,
	RMF_OSAL_EVENT_CATEGORY_SAS=37,
	RMF_OSAL_EVENT_CATEGORY_SYS_CONTROL=38,
	RMF_OSAL_EVENT_CATEGORY_GEN_FEATURE=39,
	RMF_OSAL_EVENT_CATEGORY_GEN_DIAGNOSTIC=40,
	RMF_OSAL_EVENT_CATEGORY_GATEWAY=41,
	RMF_OSAL_EVENT_CATEGORY_POD_ERROR =42,

	RMF_OSAL_EVENT_CATEGORY_DSG = 43,

	RMF_OSAL_EVENT_CATEGORY_CARD_MIB_ACC = 44,
	RMF_OSAL_EVENT_CATEGORY_HOST_APP_PRT = 45,
	RMF_OSAL_EVENT_CATEGORY_HEAD_END_COMM = 46,
	RMF_OSAL_EVENT_CATEGORY_CardManagerSystemEvents=47, //Card Manager System events
	RMF_OSAL_EVENT_CATEGORY_CardManagerXChanEvents=48, //Ext Channel    
	RMF_OSAL_EVENT_CATEGORY_HALPOD=49,
	RMF_OSAL_EVENT_CATEGORY_BOOTMSG_DIAG = 50,
	RMF_OSAL_EVENT_CATEGORY_DOWNL_ENGINE = 51,
	RMF_OSAL_EVENT_CATEGORY_DVR = 52,
	RMF_OSAL_EVENT_CATEGORY_IPPV = 53,
	RMF_OSAL_EVENT_CATEGORY_PODMGR = 54,
	RMF_OSAL_EVENT_CATEGORY_DISCONNECTMGR = 55,
	RMF_OSAL_EVENT_CATEGORY_PRETUNE = 56,
	RMF_OSAL_EVENT_CATEGORY_SIMGR = 57,
    RMF_OSAL_EVENT_CATEGORY_IPDIRECT = 58,
}rmf_osal_event_category_t;

/*Event params structure type*/
typedef struct rmf_osal_event_params_s
{
	uint32_t priority;
	void * data;
	uint32_t  data_extension;
	rmf_osal_event_free_payload_fn free_payload_fn;
}rmf_osal_event_params_t;

/** @} */ //end of OSAL Events types

/*Interface Functions*/

/**
 * @addtogroup OSAL_EVENT_API
 * @{
 */

rmf_Error rmf_osal_event_init();

rmf_Error rmf_osal_event_create( rmf_osal_event_category_t event_category, uint32_t event_type, 
		rmf_osal_event_params_t* p_event_params, rmf_osal_event_handle_t* p_event_handle);

rmf_Error rmf_osal_event_delete( rmf_osal_event_handle_t event_handle);

rmf_Error rmf_osal_eventqueue_create ( const uint8_t* name,
		rmf_osal_eventqueue_handle_t* p_eventqueue_handle);

rmf_Error rmf_osal_eventqueue_delete( rmf_osal_eventqueue_handle_t eventqueue_handle);

rmf_Error rmf_osal_eventqueue_add( rmf_osal_eventqueue_handle_t eventqueue_handle,
		rmf_osal_event_handle_t event_handle);

/**
 * The <i>rmf_osal_eventqueue_addTimed()</i> function will add an event to specified 
 * eventqueue with in a timeout
 *
 * @param eventqueue_handle Handle of eventqueue.
 * @param event_handle Handle of event to be added to the queue.
 * @param timeout_usec the timeout with in the message should be added
 * @return Returns relevant RMF_OSAL error code on failure, otherwise <i>RMF_SUCCESS</i>
 *          is returned.
 */
rmf_Error rmf_osal_eventqueue_addTimed(rmf_osal_eventqueue_handle_t eventqueue_handle,
		rmf_osal_event_handle_t event_handle, int32_t timeout_usec);

/**
 * The <i>rmf_osal_eventqueue_get_next_event()</i> function will wait till next event is available
 * on the eventqueue and retrieves the event on return.
 *
 * @param eventqueue_handle Handle of eventqueue.
 * @param p_event_handle Pointer to retrieve the event handle from the queue.
 * @param p_event_category Event category of the retrieved event. Client shall pass NULL if data is
 *      not required.
 * @param p_event_type Pointer to retrieve event type.
 * @param p_event_params Pointer to retrieve event specific data. Client shall pass NULL if data is
 *      not required.
 * @return Returns relevant RMF_OSAL error code on failure, otherwise <i>RMF_SUCCESS</i>
 *          is returned.
 */
rmf_Error rmf_osal_eventqueue_get_next_event( rmf_osal_eventqueue_handle_t eventqueue_handle,
		rmf_osal_event_handle_t *p_event_handle, rmf_osal_event_category_t* p_event_category,
		uint32_t *p_event_type, rmf_osal_event_params_t* p_event_params);

rmf_Error rmf_osal_eventqueue_try_get_next_event( rmf_osal_eventqueue_handle_t eventqueue_handle,
		rmf_osal_event_handle_t *p_event_handle, rmf_osal_event_category_t* p_event_category, 
		uint32_t *p_event_type, rmf_osal_event_params_t* p_event_params);

rmf_Error rmf_osal_eventqueue_get_next_event_timed( rmf_osal_eventqueue_handle_t eventqueue_handle,
		rmf_osal_event_handle_t *p_event_handle, rmf_osal_event_category_t* p_event_category, uint32_t *p_event_type, 
		rmf_osal_event_params_t* p_event_params, int32_t timeout_usec);

rmf_Error rmf_osal_eventqueue_clear ( rmf_osal_eventqueue_handle_t eventqueue_handle);

rmf_Error rmf_osal_eventmanager_create( rmf_osal_eventmanager_handle_t* p_eventmanager_handle);

rmf_Error rmf_osal_eventmanager_delete( rmf_osal_eventmanager_handle_t eventmanager_handle);

rmf_Error rmf_osal_eventmanager_register_handler(
		rmf_osal_eventmanager_handle_t eventmanager_handle,
		rmf_osal_eventqueue_handle_t eventqueue_handle,
		rmf_osal_event_category_t event_category);

rmf_Error rmf_osal_eventmanager_unregister_handler(
		rmf_osal_eventmanager_handle_t eventmanager_handle,
		rmf_osal_eventqueue_handle_t eventqueue_handle);

rmf_Error rmf_osal_eventmanager_dispatch_event(
		rmf_osal_eventmanager_handle_t eventmanager_handle,
		rmf_osal_event_handle_t event_handle);

/** @} */ //End of Doxygen tag OSAL_EVENT_API

//#define DEBUG_ALLOCATED_EVENTS
#ifdef DEBUG_ALLOCATED_EVENTS
void rmf_osal_debug_eventlist_dump();
#endif

#ifdef __cplusplus
}
#endif

#endif /* RMF_OSAL_EVENT_H_ */
