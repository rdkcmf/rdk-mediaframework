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



#ifndef __C_VECTOR_H
#define __C_VECTOR_H

#include "rmf_osal_sync.h"

typedef long cVectorElement;

typedef struct _cVector
{
    int capacity;
    int increment;
    int size;
    cVectorElement *elements;
    rmf_osal_Mutex mutexId;
} cVector;




#ifdef __cplusplus
extern "C"
{
#endif /*__cplusplus*/



/* returns NULL if error */
 extern cVector* cVectorCreate(int capacity, int increment);
/* returns number of elements in the vector */
 extern int cVectorSize(const cVector* pV);
/* clears the vector */
 extern void cVectorClear(cVector* pV);
/* returns the index if found else -1 */
 extern int cVectorSearch(const cVector* pV, cVectorElement ele);
/* returns element */
 extern cVectorElement cVectorGetElementAt(const cVector* pV, int index);
/* sets element at the given index; returns same index if success and -1 if index error */
 extern int cVectorSetElementAt(cVector* pV, cVectorElement ele, int index);
/* adds element at the end of vector, returns index of added element */
 extern int cVectorAdd(cVector* pV, cVectorElement ele);
/* inserts element at the given index; returns index of inserted element */
 extern int cVectorInsertElementAt(cVector* pV, cVectorElement ele, int index);
/* removes element, returns index of removed element and -1 if not found */
 extern int cVectorRemoveElementAt(cVector* pV, int index);
/* removes element, returns index of removed element and -1 if not found */
 extern int cVectorRemoveElement(cVector* pV, cVectorElement ele);
/* destroys and frees the vector resources */
 extern void cVectorDestroy(cVector* pV);
/* locks vector */
#define cVectorLock(pV)         if(pV) rmf_osal_mutexAcquire(pV->mutexId)
/* unlocks vector */
#define cVectorUnlock(pV)       if(pV) rmf_osal_mutexRelease(pV->mutexId)


#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /*__C_VECTOR_H*/

