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


#include <stdio.h>

#include "rmf_osal_mem.h"
#include "rmf_symboltable.h"

/**
 * @file    rmf_symboltable.cpp
 * @brief This file contains all the function definations of the class rmf_SymbolMapTable which
 * helps to maintain a linked list for section requests and outstanding sections.
 */

/**
 * @brief This function is a default constructor. It initializes the
 * head pointer of the symbol table to NULL.
 *
 * @return None
 */
rmf_SymbolMapTable::rmf_SymbolMapTable()
{
	head = NULL;
}

/**
 * @brief This function is a default constructor.
 *
 * @return None
 */
rmf_SymbolMapTable::~rmf_SymbolMapTable()
{

}

/**
 * @brief This function creates a new node with the specified data and key and adds
 * it to the symbol table pointed by the head element of the class rmf_SymbolMapTable.
 *
 * @param[in] key Indicates value for the key element of the node.
 * @param[in] data Indicates data to be updated for the data element of the node.
 *
 * @return None
 */
void rmf_SymbolMapTable::Insert(rmf_SymbolMapKey key, void *data)
{
        int index;
	rmf_Error retOsal = RMF_SUCCESS;
        
        node_t *newNode;
        
        //newNode = (node_t *)vlMalloc(sizeof(node_t));   //should free at the time of remove
        retOsal = rmf_osal_memAllocP(RMF_OSAL_MEM_FILTER, sizeof(node_t), (void**)&newNode);   //should free at the time of remove
	if(newNode!=NULL)
	{
        	newNode->key = key;
        	newNode->data = data;
        	newNode->next = head;
        
        	head = newNode;
	}
}

/**
 * @brief This function provides the pointer to the data of the node corresponding to the specified key.
 *
 * @param[in] key Indicates unique ID of the node against which the data needs to be provided.
 *
 * @return Returns a pointer to the data element of the node on success, else NULL.
 */
void* rmf_SymbolMapTable::Lookup(rmf_SymbolMapKey key)
{
        node_t *currentNode;
        int found = 0;
        
        currentNode = head;

        while(NULL != currentNode)
        {
                if(currentNode->key == key)
                {
                        found = 1;
                        break;
                }
                else
                {
                        currentNode = currentNode->next;
                }
        }

        if(found)
                return currentNode->data;
        else
                return NULL;

}

/**
 * @brief This function removes the node associated with the key from the symbol table pointed by the
 * head element of the class rmf_SymbolMapTable.
 *
 * @param[in] key Unique ID to find node to be removed.
 *
 * @return Returns a pointer to the node which is removed on success, else NULL.
 */
void* rmf_SymbolMapTable::Remove(rmf_SymbolMapKey key)
{
	node_t *currentNode = NULL, *foundNode = NULL, *prevNode = NULL;
	void *data = NULL;

	if(NULL == head)
		return NULL;

	currentNode = head;

	if(currentNode->key == key)
	{
		foundNode = currentNode;
		head = currentNode->next;
	}
	else
	{
#if 0
		while(NULL != currentNode->next)
		{
			if(currentNode->next->key == key)
			{
				foundNode = currentNode->next;
				currentNode->next = currentNode->next->next;
				break;
			}
			else
			{
				currentNode = currentNode->next;
			}
		}
#else
                prevNode = currentNode;
                currentNode = currentNode->next;

		while(NULL != currentNode)
                {
			if(currentNode->key == key)
                        {
                            foundNode = currentNode;
                            if(prevNode != NULL)
                                prevNode->next = currentNode->next;
                            currentNode->next = NULL;
                            break;
                        }

                        prevNode = currentNode;
                        currentNode = currentNode->next;
                }
#endif
	}

	if(NULL != foundNode)
	{
		data = foundNode->data;
		rmf_osal_memFreeP(RMF_OSAL_MEM_FILTER, foundNode);
	}

	return data;
}

/**
 * @brief This function returns the head pointer to the symbol table.
 *
 * @param[out] iter Pointer to the head of the symbol table.
 *
 * @return None
 */
void rmf_SymbolMapTable::IterInit(rmf_SymbolMapTableIter *iter)
{
	*iter = head;
}

/**
 * @brief This function gets the data element of the current node specified by iter parameter.
 *
 * @param[out] key Indicates the Unique ID of the current node pointed by the input parameter iter.
 * @param[in,out] iter As an input parameter it contains pointer to a node whose data element
 * needs to be returned. As an output parameter it contains pointer to the next node
 * in the symbol table.
 *
 * @return Returns data element of the node pointed by iter on success else NULL.
 */
void* rmf_SymbolMapTable::IterNext(rmf_SymbolMapTableIter *iter, rmf_SymbolMapKey *key)
{
       node_t *currentNode;
        currentNode = *iter;

        if(NULL != currentNode)
        {
                *key = currentNode->key;
                *iter = currentNode->next;
                return currentNode->data;
        }
        else
                return NULL;
}

