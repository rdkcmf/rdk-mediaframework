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


//#include "vlPlatform.h"
#ifndef NUCLEUS_OS
#ifndef MTEK_OSAL
#include <stdio.h>
#include <string.h>
#endif // MTEK_OSAL
#endif // NUCLEUS_OS

//#include "cSystem.h"
//#include "cMisc.h"
//#include "cAssert.h"
#include "vlVector.h"
#include "rdk_debug.h"

#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif

//#include "vlLogMacros.h"

#define vlMemCpy(pDest, pSrc, nCount, nDestCapacity) memcpy(pDest, pSrc, nCount)

/////////////////////////////////////////////////////////////////////////////////////////
/////// BEGIN : vlVector class
vlVector::vlVector(int capacity, int increment)
{
    if(capacity <= 0) capacity = 1;
    if(increment <= 0) increment = 1;

    //int i = true;

    m_capacity = capacity;
    m_increment = increment;
    m_size = 0;

    allocMemory(m_capacity);
//    assert(elements != NULL);
}

vlVector::~vlVector()
{
    delMemory();
}

bool vlVector::allocMemory(const int nCapacity)
{
    static int nMax = 0;
    if(nCapacity > nMax)
    {
        //DECLARE_LOG_BUFFER(log, 100);
        nMax = nCapacity;
        //LOG_SPRINTF(log, "******** vlVector ********** Max = %d\n", nMax, 0, 0, 0, 0);
        //LOG_MESSAGE(LOG_TRACELEVEL_DEBUG_2, "vlVector::allocMemory", log);
    }

    m_capacity = nCapacity;
    // Allocate from heap
    elements = new element[m_capacity +1];

    return (NULL != elements);
}

void vlVector::delMemory()
{
    if (NULL != elements)
    {
        delete[] elements;
        elements = NULL;
    }
}

vlVector::iterator vlVector::begin() const
{
    return (iterator)(&elements[0]);
}

vlVector::iterator vlVector::end() const
{
    return (iterator)(&elements[m_size]);
}

bool vlVector::remove(int index)
{
    if (m_size <= 0) return false;

    //if out of range return false
    if(index < 0 || index >= m_size) return false;
    //shift array up

    //vlMemCpy((void*)(&elements[index]), (void*)(&elements[index+1]), sizeof(void*) * (m_size - index - 1), sizeof(void*) * (m_size - index - 1));

    memmove((void*)(&elements[index]), (void*)(&elements[index+1]), sizeof(void*) * (m_size - index - 1));
    //reduce size
    m_size--;

    return true;
}

bool vlVector::removeElement(element ele)
{
    int i;

    if (m_size <= 0) return false;

    //find the element position in the array
    for(i=0; i<m_size; i++)
        if(ele == elements[i]) break;
    //if not found return false
    if(i == m_size) return false;
    //shift array up
    //vlMemCpy((void*)(&elements[i]), (void*)(&elements[i+1]), sizeof(void*) * (m_size - i - 1), sizeof(void*) * (m_size - i - 1));
    memmove((void*)(&elements[i]), (void*)(&elements[i+1]), sizeof(void*) * (m_size - i - 1));
    //reduce size
    m_size--;

    return true;
}

vlVector::iterator vlVector::erase(iterator it)
{
    int i;

    //find the iterator position in the array
    for(i=0; i<m_size; i++)
        if(it == (iterator)(&elements[i])) break;
    //if not found return end()
    if(i == m_size) return(iterator)(&elements[m_size]);
    //shift array up
    vlMemCpy((void*)(&elements[i]), (void*)(&elements[i+1]), sizeof(void*) * (m_size - i - 1), sizeof(void*) * (m_size - i - 1));
    //reduce size
    m_size--;
    //return iterator pointing to element after the erased element
    return (iterator)(&elements[i]);
}

vlVector::iterator vlVector::erase(iterator first, iterator last)
{
    int i, j;
    //find the position of first iterator
    for(i=0; i<m_size; i++)
        if(first == (iterator)(&elements[i])) break;
    //if not found return end()
    if(i == m_size) return(iterator)(&elements[m_size]);

    //find the position of second iterator
    for(j=i+1; j<m_size; j++)
        if(last == (iterator)(&elements[j])) break;
    //if not found return end()
    //if(j == m_size) return(iterator)(&elements[m_size]);
    //shift array up
    vlMemCpy((void*)(&elements[i]), (void*)(&elements[j]), sizeof(void*) * (m_size - j), sizeof(void*) * (m_size - j));
    //reduce size
    m_size -= j - i;
    //return iterator pointing to element after the erased element
    return (iterator)(&elements[i]);
}

int vlVector::size() const
{
    return m_size;
}

int vlVector::capacity() const
{
    return m_capacity;
}

void vlVector::clear()
{
    m_size = 0;
}

bool vlVector::empty() const
{
    return (m_size) ? false : true;
}

int vlVector::search(element ele) const
{
    for(int i=0; i<m_size; i++)
        if(elements[i] == ele) return i;

    return -1;
}

bool vlVector::operator==(const vlVector& rhs) const
{
    if(m_size != rhs.size()) return false;

    int i;
    for(i=0; i<m_size; i++)
        if(this->operator[](i) != rhs[i]) return false;

    return true;
}

bool vlVector::operator!=(const vlVector& rhs) const
{
    return !(*this==rhs);
}

void vlVector::add(element ele)
{
//  DECLARE_LOG_BUFFER(log, 200);
    //realloc if its full
    if(m_size == m_capacity)
    {
        int newCapacity = m_capacity + m_increment;
        element *temp = new element[newCapacity + 1];
//      LOG_SPRINTF(log, "Allocating %d elements", newCapacity +1, 0, 0, 0, 0);
//      LOG_INFO_MESSAGE("vlVector::add", log);
        if (temp == NULL)
        {
            RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.POD", "vlVector::add", "MEM ALLOC FAILED");
            return;
        }
        vlMemCpy((void*)temp, (void*)elements, sizeof(element) * m_size, sizeof(element) * newCapacity);
        delMemory();
        m_capacity = newCapacity;
        elements = temp;
    }

    //add new element at the end
    elements[m_size++] = ele;
}


// Added by Vinod S.
//element l Value
vlVector::element& vlVector::operator[](int pos) const
{
    return elements[pos];
}

//element r value
vlVector::element& vlVector::operator[](int pos)
{
    return elements[pos];
}
/////// END : vlVector class
/////////////////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////////////////
/////// BEGIN : vlByteVector class
vlByteVector::vlByteVector(int capacity, int increment)
{
    //DECLARE_LOG_BUFFER(log, 200);

    if(capacity <= 0) capacity = 1;
    if(increment <= 0) increment = 1;

    m_capacity = capacity;
    m_increment = increment;
    m_size = 0;
    elements = NULL;

    allocMemory(m_capacity);

    //LOG_SPRINTF(log, "Allocating %d elements, ptr : 0x%x", m_capacity +1, elements, 0, 0, 0);
    //LOG_MESSAGE(LOG_TRACELEVEL_DEBUG_2, "vlByteVector::vlByteVector", log);

//    assert(elements != NULL);
}

vlByteVector::~vlByteVector()
{
    //DECLARE_LOG_BUFFER(log, 200);
    //LOG_SPRINTF(log, "Deleting %d elements, ptr : 0x%x", m_capacity +1, elements, 0, 0, 0);
    //LOG_MESSAGE(LOG_TRACELEVEL_DEBUG_2, "vlByteVector::~vlByteVector", log);

    delMemory();
}

bool vlByteVector::allocMemory(const int nCapacity)
{
    static int nMax = 0;
    if(nCapacity > nMax)
    {
        //DECLARE_LOG_BUFFER(log, 100);
        nMax = nCapacity;
        //LOG_SPRINTF(log, "******** vlByteVector ********** Max = %d\n", nMax, 0, 0, 0, 0);
        //LOG_MESSAGE(LOG_TRACELEVEL_DEBUG_2, "vlByteVector::allocMemory", log);
    }

    m_capacity = nCapacity;
    // Allocate from heap
    elements = new element[m_capacity +1];

    return (NULL != elements);
}

void vlByteVector::delMemory()
{
    if (NULL != elements)
    {
        delete[] elements;
        elements = NULL;
    }
}

void vlByteVector::setArray(const unsigned char *buf, const int nBytes)
{
    delMemory();

    m_size = m_capacity = (nBytes > 0) ? nBytes : 0;

    if(allocMemory(m_capacity))
    {
        if(m_size > 0)
        {
            vlMemCpy(elements, buf, m_size, m_capacity);
        }
    }
}

vlByteVector::iterator vlByteVector::begin() const
{
    return (iterator)(&elements[0]);
}

vlByteVector::iterator vlByteVector::end() const
{
    return (iterator)(&elements[m_size]);
}

bool vlByteVector::remove(int index)
{
    //if out of range return false
    if (m_size <= 0) return false;

    if(index < 0 || index >= m_size) return false;
    //shift array up
    vlMemCpy((void*)(&elements[index]), (void*)(&elements[index+1]), sizeof(void*) * (m_size - index - 1), sizeof(void*) * (m_size - index - 1));
    //reduce size
    m_size--;

    return true;
}

bool vlByteVector::removeElement(element ele)
{
    int i;
    if (m_size <= 0) return false;

    //find the element position in the array
    for(i=0; i<m_size; i++)
        if(ele == elements[i]) break;
    //if not found return false
    if(i == m_size) return false;
    //shift array up
    vlMemCpy((void*)(&elements[i]), (void*)(&elements[i+1]), sizeof(void*) * (m_size - i - 1), sizeof(void*) * (m_size - i - 1));
    //reduce size
    m_size--;

    return true;
}

vlByteVector::iterator vlByteVector::erase(iterator it)
{
    int i;
    //find the iterator position in the array
    for(i=0; i<m_size; i++)
        if(it == (iterator)(&elements[i])) break;
    //if not found return end()
    if(i == m_size) return(iterator)(&elements[m_size]);
    //shift array up
    vlMemCpy((void*)(&elements[i]), (void*)(&elements[i+1]), sizeof(void*) * (m_size - i - 1), sizeof(void*) * (m_size - i - 1));
    //reduce size
    m_size--;
    //return iterator pointing to element after the erased element
    return (iterator)(&elements[i]);
}

vlByteVector::iterator vlByteVector::erase(iterator first, iterator last)
{
    int i, j;
    //find the position of first iterator
    for(i=0; i<m_size; i++)
        if(first == (iterator)(&elements[i])) break;
    //if not found return end()
    if(i == m_size) return(iterator)(&elements[m_size]);

    //find the position of second iterator
    for(j=i+1; j<m_size; j++)
        if(last == (iterator)(&elements[j])) break;
    //if not found return end()
    //if(j == m_size) return(iterator)(&elements[m_size]);
    //shift array up
    vlMemCpy((void*)(&elements[i]), (void*)(&elements[j]), sizeof(void*) * (m_size - j), sizeof(void*) * (m_size - j));
    //reduce size
    m_size -= j - i;
    //return iterator pointing to element after the erased element
    return (iterator)(&elements[i]);
}

int vlByteVector::size() const
{
    return m_size;
}

int vlByteVector::capacity() const
{
    return m_capacity;
}

void vlByteVector::clear()
{
    m_size = 0;
}

bool vlByteVector::empty() const
{
    return (m_size) ? false : true;
}

int vlByteVector::search(element ele) const
{
    for(int i=0; i<m_size; i++)
        if(elements[i] == ele) return i;

    return -1;
}

bool vlByteVector::operator==(const vlByteVector& rhs) const
{
    if(m_size != rhs.size()) return false;

    int i;
    for(i=0; i<m_size; i++)
        if(this->operator[](i) != rhs[i]) return false;

    return true;
}

bool vlByteVector::operator!=(const vlByteVector& rhs) const
{
    return !(*this==rhs);
}

void vlByteVector::add(element ele)
{
    //DECLARE_LOG_BUFFER(log, 200);
    //realloc if its full
    if(m_size == m_capacity)
    {
        int newCapacity = m_capacity + m_increment;
        element *temp = new element[newCapacity + 1];
        //LOG_SPRINTF(log, "Allocating %d elements", newCapacity +1, 0, 0, 0, 0);
        //LOG_MESSAGE(LOG_TRACELEVEL_DEBUG_2, "vlVector::add", log);

//        assert(temp != NULL);
        vlMemCpy((void*)temp, (void*)elements, sizeof(element) * m_size, newCapacity);
        delMemory();
        m_capacity = newCapacity;
        elements = temp;
    }

    //add new element at the end
    elements[m_size++] = ele;
}

vlByteVector::element vlByteVector::operator[](int pos) const
{
    return elements[pos];
}

/////// END : vlByteVector class
/////////////////////////////////////////////////////////////////////////////////////////




/////////////////////////////////////////////////////////////////////////////////////////
/////// BEGIN : vlShortVector class
vlShortVector::vlShortVector(int capacity, int increment)
{
    if(capacity <= 0) capacity = 1;
    if(increment <= 0) increment = 1;

    m_capacity = capacity;
    m_increment = increment;
    m_size = 0;

    allocMemory(m_capacity);
//    assert(elements != NULL);
}

vlShortVector::~vlShortVector()
{
    delMemory();
}

bool vlShortVector::allocMemory(const int nCapacity)
{
    static int nMax = 0;
    if(nCapacity > nMax)
    {
        //DECLARE_LOG_BUFFER(log, 100);
        nMax = nCapacity;
        //LOG_SPRINTF(log, "******** vlShortVector ********** Max = %d\n", nMax, 0, 0, 0, 0);
        //LOG_MESSAGE(LOG_TRACELEVEL_DEBUG_2, "vlShortVector::allocMemory", log);
    }

    m_capacity = nCapacity;

    // Allocate from heap
    elements = new element[m_capacity +1];

    return (NULL != elements);
}

void vlShortVector::delMemory()
{
    if (NULL != elements)
    {
        delete[] elements;
        elements = NULL;
    }
}

vlShortVector::iterator vlShortVector::begin() const
{
    return (iterator)(&elements[0]);
}

vlShortVector::iterator vlShortVector::end() const
{
    return (iterator)(&elements[m_size]);
}

bool vlShortVector::remove(int index)
{
    if (m_size <= 0) return false;

    //if out of range return false
    if(index < 0 || index >= m_size) return false;
    //shift array up
    vlMemCpy((void*)(&elements[index]), (void*)(&elements[index+1]), sizeof(void*) * (m_size - index - 1), sizeof(void*) * (m_size - index - 1));
    //reduce size
    m_size--;

    return true;
}

bool vlShortVector::removeElement(element ele)
{
    int i;

    if (m_size <= 0) return false;

    //find the element position in the array
    for(i=0; i<m_size; i++)
        if(ele == elements[i]) break;
    //if not found return false
    if(i == m_size) return false;
    //shift array up
    vlMemCpy((void*)(&elements[i]), (void*)(&elements[i+1]), sizeof(void*) * (m_size - i - 1), sizeof(void*) * (m_size - i - 1));
    //reduce size
    m_size--;

    return true;
}

vlShortVector::iterator vlShortVector::erase(iterator it)
{
    int i;
    //find the iterator position in the array
    for(i=0; i<m_size; i++)
        if(it == (iterator)(&elements[i])) break;
    //if not found return end()
    if(i == m_size) return(iterator)(&elements[m_size]);
    //shift array up
    vlMemCpy((void*)(&elements[i]), (void*)(&elements[i+1]), sizeof(void*) * (m_size - i - 1), sizeof(void*) * (m_size - i - 1));
    //reduce size
    m_size--;
    //return iterator pointing to element after the erased element
    return (iterator)(&elements[i]);
}

vlShortVector::iterator vlShortVector::erase(iterator first, iterator last)
{
    int i, j;
    //find the position of first iterator
    for(i=0; i<m_size; i++)
        if(first == (iterator)(&elements[i])) break;
    //if not found return end()
    if(i == m_size) return(iterator)(&elements[m_size]);

    //find the position of second iterator
    for(j=i+1; j<m_size; j++)
        if(last == (iterator)(&elements[j])) break;
    //if not found return end()
    //if(j == m_size) return(iterator)(&elements[m_size]);
    //shift array up
    vlMemCpy((void*)(&elements[i]), (void*)(&elements[j]), sizeof(void*) * (m_size - j), sizeof(void*) * (m_size - j));
    //reduce size
    m_size -= j - i;
    //return iterator pointing to element after the erased element
    return (iterator)(&elements[i]);
}

int vlShortVector::size() const
{
    return m_size;
}

int vlShortVector::capacity() const
{
    return m_capacity;
}

void vlShortVector::clear()
{
    m_size = 0;
}

bool vlShortVector::empty() const
{
    return (m_size) ? false : true;
}

int vlShortVector::search(element ele) const
{
    for(int i=0; i<m_size; i++)
        if(elements[i] == ele) return i;

    return -1;
}

bool vlShortVector::operator==(const vlShortVector& rhs) const
{
    if(m_size != rhs.size()) return false;

    int i;
    for(i=0; i<m_size; i++)
        if(this->operator[](i) != rhs[i]) return false;

    return true;
}

bool vlShortVector::operator!=(const vlShortVector& rhs) const
{
    return !(*this==rhs);
}

void vlShortVector::add(element ele)
{
    //realloc if its full
    if(m_size == m_capacity)
    {
        int newCapacity = m_capacity + m_increment;
        element *temp = new element[newCapacity + 1];
//        assert(temp != NULL);
        vlMemCpy((void*)temp, (void*)elements, sizeof(element) * m_size, sizeof(element) * newCapacity);
        delMemory();
        m_capacity = newCapacity;
        elements = temp;
    }

    //add new element at the end
    elements[m_size++] = ele;
}

vlShortVector::element vlShortVector::operator[](int pos) const
{
    return elements[pos];
}

/////// END : vlShortVector class
/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////
// vlGuid
/////////////////////////////////////////////////////////////////////////////////////////
vlGuid::vlGuid(const unsigned char * _guid)
{
    if(NULL != _guid)
    {
        vlMemCpy(m_guid, _guid, sizeof(m_guid), sizeof(m_guid));
    }
    else
    {
        memset(m_guid, 0, sizeof(m_guid));
    }
}

bool vlGuid::operator == (const vlGuid & rhs) const
{
    return (0 == memcmp(m_guid, rhs.m_guid, sizeof(m_guid)));
}

bool vlGuid::operator != (const vlGuid & rhs) const
{
    return !(*this == rhs);
}

/////////////////////////////////////////////////////////////////////////////////////////
// vlGuidVector
/////////////////////////////////////////////////////////////////////////////////////////

// default constructor
vlGuidVector::vlGuidVector(int initialCapacity) : vlVector(initialCapacity)
{
    //stl vector deos nt seem to have an interface to specify initial size.
    //so we wont do anything with capacity now.
    mdebugSize = 0;
}

vlGuidVector::vlGuidVector( )
{
    mdebugSize = 0;
}

vlGuidVector::vlGuidVector(const vlGuidVector& rhs)
{
    copyValue(rhs);
}

vlGuidVector::~vlGuidVector()
{
    clear();
}

void vlGuidVector::clear()
{
    vlGuidVector::iterator itr;

    for(itr = begin(); itr != end(); itr++)
    {
        delete (((vlGuid*)*itr));
    }
    vlVector::clear();
    mdebugSize = 0;
}

bool vlGuidVector::operator == (const vlGuidVector & rhs) const
{
    int mySize = size();
    if (mySize != rhs.size())
    {
        return false;
    }
    else
    {
        for (int ix = 0; ix < mySize; ix++)
        {
            if(*((vlGuid*)this->operator[](ix)) != *((vlGuid*)rhs[ix]))
                return false;
        }
        return true;
    }

    return false;
}

bool vlGuidVector::operator != (const vlGuidVector & rhs) const
{
    return !(*this == rhs);
}

vlGuidVector& vlGuidVector::operator = (const vlGuidVector & rhs)
{
    clear();  //delete the present content
    copyValue(rhs);
    return *this;
}

void vlGuidVector::copyValue(const vlGuidVector& rhs)
{
    for (int ix = 0; ix < rhs.size(); ix++)
    {
        vlGuid* pNewEntry = new vlGuid(*((vlGuid*)rhs[ix]));
        if (NULL == pNewEntry)
        {
            clear();
            return;
        }
        add((vlGuidVector::element)pNewEntry);
    }
    mdebugSize = rhs.size();
}

#include "cVector.h"
/////////////////////////////////////////////////////////////////////////////////////////
/////// BEGIN : vlProtectedVector class

vlProtectedVector::vlProtectedVector(int capacity, int increment)
{
    m_capacity = 0 ;
    m_increment = 0;
    m_size = 0;
    m_pVector = cVectorCreate(capacity, increment);
}

vlProtectedVector::~vlProtectedVector()
{
    cVectorDestroy(m_pVector);
}

void vlProtectedVector::lock() const
{
    //cMutexBegin(m_pVector->mutexId);
    rmf_osal_mutexAcquire(m_pVector->mutexId);
}

void vlProtectedVector::unlock() const
{
    //cMutexEnd(m_pVector->mutexId);
    rmf_osal_mutexRelease(m_pVector->mutexId);
}

bool vlProtectedVector::remove(int index)
{
    return (-1 != cVectorRemoveElementAt(m_pVector, index));
}

bool vlProtectedVector::removeElement(element ele)
{
    return (-1 != cVectorRemoveElement(m_pVector, (cVectorElement)(ele)));
}

int vlProtectedVector::size() const
{
    return cVectorSize(m_pVector);
}

int vlProtectedVector::capacity() const
{
    return m_pVector->capacity;
}

void vlProtectedVector::clear()
{
    cVectorClear(m_pVector);
}

bool vlProtectedVector::empty() const
{
    return (size()) ? false : true;
}

int vlProtectedVector::search(element ele) const
{
    return cVectorSearch(m_pVector, (cVectorElement)ele);
}

bool vlProtectedVector::operator==(const vlVector& rhs) const
{
    if(size() != rhs.size()) return false;

    bool bEquals = true;

    lock();
    {
        for(int i=0; i<m_pVector->size; i++)
        {
            if(this->operator[](i) != rhs[i]) bEquals = false;
        }
    }
    unlock();

    return bEquals;
}

bool vlProtectedVector::operator!=(const vlVector& rhs) const
{
    return !(*this==rhs);
}

void vlProtectedVector::add(element ele)
{
    cVectorAdd(m_pVector, (cVectorElement)ele);
}

void vlProtectedVector::setElementAt(int index, element ele)
{
    cVectorSetElementAt(m_pVector, (cVectorElement)ele, index);
}

vlProtectedVector::element vlProtectedVector::operator[](int pos) const
{
    return cVectorGetElementAt(m_pVector, pos);
}

vlProtectedVector::element vlProtectedVector::operator[](int pos)
{
    return cVectorGetElementAt(m_pVector, pos);
}

/////// END : vlProtectedVector class
/////////////////////////////////////////////////////////////////////////////////////////
