/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2011 RDK Management
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
 * @file rmf_sectionfilter_util.cpp
 * @brief This file contains function definations for maintaing a queue. It is used by
 * section filter to maintain the section data in a queue.
 */

#include <stdio.h>
#include <pthread.h>

#include "rmf_sectionfilter_util.h"
#include <stdlib.h>
#include "rdk_debug.h"
#include "rmf_osal_mem.h"

/**
 * @brief This function adds the section data indicated by the node to the elements of the specified
 * section data queue.
 *
 * @param[in] queue Pointer to the queue to which data needs to be added.
 * @param[in] node Contains section data to be added to teh queue.
 * @param[in] OverFlowEraseFlag This flag is set if erase on overflow of queue is enabled.
 * @param[out] retNode If OverFlowEraseFlag is set then it contains the erased node due to overflow.
 *
 * @return Returns 0 on success and -1 if the queue is empty or if the data overflows.
 */

int rmf_queue_push_tail(RMFQueue *queue, void *node, int OverFlowEraseFlag, void *retNode)
{
	if(queue == NULL)
		return -1;
	
	//pthread_mutex_lock(&(queue->mutex));
	rmf_osal_mutexAcquire(queue->mutex);
	
	if(queue->count == MAX_QUEUE_SIZE)
	{
		if(OverFlowEraseFlag)
		{		
			//tail points to Head element and overwrite with new node 
			//head points to next element in the queue...
			queue->tail = queue->head;
			retNode = queue->elements[queue->head];
			queue->head++;
			queue->elements[queue->tail] = node;
		}
		else
		{
			RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.UTIL", "rmf_queue_push_tail :: Data over flows\n\n");
			//pthread_mutex_unlock(&(queue->mutex));
			rmf_osal_mutexRelease(queue->mutex);
			return -1;
		}
	}
	else
	{
		queue->elements[queue->tail] = node;
		queue->tail++;
		if(queue->tail == MAX_QUEUE_SIZE)
			queue->tail = 0;
		
		queue->count++;
	}
	
	//pthread_mutex_unlock(&(queue->mutex));
	rmf_osal_mutexRelease(queue->mutex);
	return 0;
}

/**
 * @brief This function gets the section data which is at the head of the queue.
 *
 * @param[in] queue Pointer to queue.
 *
 * @return Returns pointer to the data
 * @retval element Pointer to the data which is at the head of the queue
 * @retval NULL If the queue is empty
 */
void* rmf_queue_pop_head(RMFQueue *queue)
{
	void* element = NULL;
	
	if(queue == NULL)
		return NULL;
	
	//pthread_mutex_lock(&(queue->mutex));
	rmf_osal_mutexAcquire(queue->mutex);
	if(queue->count > 0)
	{
		element = queue->elements[queue->head];
		queue->head++;
		if(queue->head == MAX_QUEUE_SIZE)
			queue->head = 0;
		queue->count--;
	}
	//pthread_mutex_unlock(&(queue->mutex));
	rmf_osal_mutexRelease(queue->mutex);
	return element;
}

/**
 * @brief This function creates a new node for collecting section data.
 *
 * @return Returns the newly created queue.
 * @retval pRMFQueue Indicates SectionsDataQueue is created successfully.
 * @retval NULL Indicates could not create SectionsDataQueue.
 */
RMFQueue* rmf_queue_new()
{
	RMFQueue *pRMFQueue;
	rmf_Error retOsal = RMF_SUCCESS;

//	pRMFQueue = (RMFQueue *)vlMalloc(sizeof(RMFQueue));
//	pRMFQueue = (RMFQueue *)malloc(sizeof(RMFQueue));
	retOsal = rmf_osal_memAllocP(RMF_OSAL_MEM_FILTER, sizeof(RMFQueue), (void**)&pRMFQueue);
	if ((NULL == pRMFQueue) || (retOsal != RMF_SUCCESS))
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILTER","Could not allocate pRMFQueue\n");
		return NULL;
	}
	pRMFQueue->head = 0;
	pRMFQueue->tail = 0;
	pRMFQueue->count = 0;
	pRMFQueue->mutex = NULL;
	//pthread_mutex_init(&(pRMFQueue->mutex), 0);
        if(RMF_SUCCESS != rmf_osal_mutexNew(&(pRMFQueue->mutex)))
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FILTER", "%s:: Could not create mutex\n", __FUNCTION__);
	    rmf_osal_memFreeP(RMF_OSAL_MEM_FILTER, pRMFQueue);
	    pRMFQueue = NULL;
        }

	return pRMFQueue;
}

/**
 * @brief This function is used to clear the elements of queue, which means it sets the head,
 * tail and count to zero.
 *
 * @param[in] queue Pointer to the queue whose elements are to be cleared.
 *
 * @return None
 */
void rmf_queue_clear(RMFQueue *queue)
{
	//pthread_mutex_lock(&(queue->mutex));
        rmf_osal_mutexAcquire(queue->mutex);
	queue->head = 0;
	queue->tail = 0;
	queue->count = 0;	
	//pthread_mutex_unlock(&(queue->mutex));
	rmf_osal_mutexRelease(queue->mutex);
	// Aug-02-2011: commented : free should be called by rmf_queue_free() // vlFree(queue);
}

/**
 * @brief This function is used to free the memory of queue.
 *
 * @param[in] queue Pointer to the queue to be freed.
 *
 * @return None
 */
void rmf_queue_free(RMFQueue *queue)
{
	//pthread_mutex_destroy(&(queue->mutex));
	rmf_osal_mutexAcquire(queue->mutex);
	rmf_osal_mutexRelease(queue->mutex);
	rmf_osal_mutexDelete(queue->mutex);
	queue->mutex = NULL;

//	vlFree(queue);  
       //free(queue);
	rmf_osal_memFreeP(RMF_OSAL_MEM_FILTER, queue);
	queue=NULL;
}
