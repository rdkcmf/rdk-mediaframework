/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2013 RDK Management
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
 

#ifndef _RMF_SECTIONFILTER_UTIL_H_
#define _RMF_SECTIONFILTER_UTIL_H_

#include <stdio.h>
#include "rmf_osal_sync.h"


/**
 * @file rmf_sectionfilter_util.h
 * @brief File contains the section filter utilities data
 */


#define MAX_QUEUE_SIZE 32

// typedef unsigned char queueElement;


/**
 * @struct RMFQueue_s
 * @brief This data structure declares the rmf queue stack in FIFO method.
 * @ingroup sectionfilterdatastructures
 */
typedef struct RMFQueue_s
{
	void		 	*elements[MAX_QUEUE_SIZE];
	unsigned long 	head;
	unsigned long 	tail;
	int 			count;
	rmf_osal_Mutex mutex;
}RMFQueue;

int rmf_queue_push_tail(RMFQueue *queue, void *node, /*bool*/int OverFlowEraseFlag, void *retNode);

void* rmf_queue_pop_head(RMFQueue *queue);

RMFQueue* rmf_queue_new();

void rmf_queue_clear(RMFQueue *queue);

void rmf_queue_free(RMFQueue *queue);

#endif //_RMF_SECTIONFILTER_UTIL_H_
