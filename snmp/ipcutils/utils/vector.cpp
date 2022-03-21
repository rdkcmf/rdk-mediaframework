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




#include "cVector.h"
#include "string.h"
#include "rmf_osal_mem.h"
#include "rdk_debug.h"
#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif

//#include "cSystem.h"

//#include "cVector.h"
//#include "cSystem.h"
//#include "cMisc.h"



/* returns NULL if error */
cVector* cVectorCreate(int capacity, int increment)
{
    cVector *pV;
	rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(cVector),(void **)&pV);

    if(pV)
    {

        if(capacity <= 0) capacity = 1;
        if(increment <= 0) increment = 1;

        pV->capacity = capacity;
        pV->increment = increment;
        pV->size = 0;

		rmf_osal_memAllocP(RMF_OSAL_MEM_POD, pV->capacity * sizeof(cVectorElement),(void **)&pV->elements);
        if(NULL == pV->elements)
        {
			rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pV);
            pV = NULL;
        }
        else
        {
            rmf_osal_mutexNew(&pV->mutexId);
        }
    }

    return pV;
}

/* returns number of elements in the vector */
int cVectorSize(const cVector* pV)
{
    if(!pV) return 0;

    return pV->size;
}

/* clears the vector */
void cVectorClear(cVector* pV)
{
    if(!pV) return;

    rmf_osal_mutexAcquire(pV->mutexId);
    pV->size = 0;
    rmf_osal_mutexRelease(pV->mutexId);
}

/* returns the index if found else -1 */
int cVectorSearch(const cVector* pV, cVectorElement ele)
{
    int i;

    if(!pV) return -1;
    if (pV->size <= 0) return -1;

    rmf_osal_mutexAcquire(pV->mutexId);
    for(i=0; i<pV->size; i++) if(pV->elements[i] == ele) break;
    if(i == pV->size) i = -1;
    rmf_osal_mutexRelease(pV->mutexId);

    return i;
}

/* returns element */
cVectorElement cVectorGetElementAt(const cVector* pV, int index)
{
    cVectorElement ele = 0;

    if(!pV) return 0;
    if (pV->size <= 0) return 0;

    rmf_osal_mutexAcquire(pV->mutexId);
    if(0 <= index && index < pV->size) ele = pV->elements[index];
    rmf_osal_mutexRelease(pV->mutexId);

    return ele;
}

/* sets element at the given index; returns same index if success and -1 if index error */
int cVectorSetElementAt(cVector* pV, cVectorElement ele, int index)
{
    if(!pV) return -1;

    /* check out of range */
    if(index < 0 || index >= pV->size) return -1;

    rmf_osal_mutexAcquire(pV->mutexId);
    pV->elements[index] = ele;
    rmf_osal_mutexRelease(pV->mutexId);

    return index;
}

/* adds element at the end of vector, returns index of added element */
int cVectorAdd(cVector* pV, cVectorElement ele)
{
    cVectorElement *temp;
    if(!pV) return -1;

    rmf_osal_mutexAcquire(pV->mutexId);

    /* realloc if its full */
    if(pV->size == pV->capacity)
    {
        pV->capacity += pV->increment;
		rmf_osal_memAllocP(RMF_OSAL_MEM_POD, (pV->capacity * sizeof(cVectorElement)),(void **)&temp);
		if (NULL != temp)
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SNMP",
			"%s: size = %d, capacity = %d \n", __FUNCTION__, pV->size, pV->capacity );			
	        rmf_osal_memcpy((void*)temp, (void*)pV->elements, sizeof(cVectorElement) * pV->size, pV->capacity * sizeof(cVectorElement), sizeof(cVectorElement) * pV->size );
		rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pV->elements);
        pV->elements = temp;
    }
    }

    /* add new element at the end */
    pV->elements[pV->size++] = ele;

    rmf_osal_mutexRelease(pV->mutexId);

    return pV->size-1;
}

/* inserts element at the given index; returns index of inserted element */
int cVectorInsertElementAt(cVector* pV, cVectorElement ele, int index)
{
    int i;

    if(!pV) return -1;

    rmf_osal_mutexAcquire(pV->mutexId);

    /* get correct index */
    if(index < 0) index = 0;
    if(index > pV->size) index = pV->size;

    /* create placeHolder for a new element (at the end of vector) */
    cVectorAdd(pV, 0/*dummy*/);

    /* shift elements back */
    for(i=pV->size-1; i>index; i--) pV->elements[i] = pV->elements[i-1];

    /* put new element at index */
    pV->elements[index] = ele;

    rmf_osal_mutexRelease(pV->mutexId);

    return index;
}

/* removes element, returns index of removed element and -1 if not found */
int cVectorRemoveElementAt(cVector* pV, int index)
{
    int i;

    if(!pV) return -1;

    if (pV->size <= 0) return -1;

    rmf_osal_mutexAcquire(pV->mutexId);

    if(index < 0 || index >= pV->size)
    {
        /* out of range */
        index = -1;
    }
    else
    {
        /* shift array up */
        //SHYAM:06032004:Karlsruhe
        //vlMemCpy((void*)(&pV->elements[index]), (void*)(&pV->elements[index+1]), sizeof(cVectorElement) * (pV->size - index - 1), sizeof(cVectorElement) * (pV->size - index - 1));
        for(i=index; i<pV->size-1; i++) pV->elements[i] = pV->elements[i+1];
        /* reduce size */
        pV->size--;
    }

    rmf_osal_mutexRelease(pV->mutexId);

    return index;
}

/* removes element, returns index of removed element and -1 if not found */
int cVectorRemoveElement(cVector* pV, cVectorElement ele)
{
    int ret;

    if(!pV) return -1;

    rmf_osal_mutexAcquire(pV->mutexId);
    ret = cVectorRemoveElementAt(pV, cVectorSearch(pV, ele));
    rmf_osal_mutexRelease(pV->mutexId);

    return ret;
}

/* destroys and frees the vector resources */
void cVectorDestroy(cVector* pV)
{
    if(!pV) return;

    if(pV->elements) rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pV->elements);
    rmf_osal_mutexDelete(pV->mutexId);
	rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pV);
}

