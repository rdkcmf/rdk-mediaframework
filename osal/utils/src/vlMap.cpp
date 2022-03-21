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


/////////////////////////////////////////////////////////////////////////////
#include "rmf_osal_mem.h"
#include "vlMutex.h"
#include "assert.h"
//#include "cAssert.h"
//#include "vlLogMacros.h"
#include "vlMutex.h"
#include "vlMap.h"
#include "rdk_debug.h"

#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif

#ifdef AUTO_LOCKING

static void auto_lock_ptr(rmf_osal_Mutex **mutex)
{
         if(NULL == *mutex)
            {
                   rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(rmf_osal_Mutex), (void **)mutex);
            rmf_osal_mutexNew(*mutex);      
         }
         rmf_osal_mutexAcquire(**mutex);
}

static void //auto_unlock_ptr(rmf_osal_Mutex *mutex)
{
         if(mutex)
         rmf_osal_mutexRelease(*mutex);
}
#endif
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
vlMap::vlMapItem::vlMapItem(KEY key, ELEMENT element)
    : m_Key(key), m_Element(element)
{

}

/////////////////////////////////////////////////////////////////////////////
vlMap::vlMap(int nTableSize,
          int nBucketCapacity,
          int nBucketIncrement)
 : m_Table(nTableSize, 1)
{
    VL_AUTO_LOCK(m_Lock);   // for multi-thread protection

    // add buckets to the table
    for(int i = 0; i < nTableSize; i++)
    {
        // new bucket
        vlVector * pBucket = new vlVector(nBucketCapacity, nBucketIncrement);

        // check allocation
        if(NULL != pBucket)
        {
            // add bucket
            m_Table.add(vlVector::element(pBucket));
        }
        else // allocation failure
        {
            // invoke debug break
            assert(false);
        }
    }

    // set count to 0
    m_nCount = 0;
}

/////////////////////////////////////////////////////////////////////////////
vlMap::~vlMap()
{
    VL_AUTO_LOCK(m_Lock);   // for multi-thread protection

    // debug count
    int nCount = 0;

    // for each bucket in table
    for(int iBucket = 0; iBucket < m_Table.size(); iBucket++)
    {
        // get emptied bucket
        vlVector * pBucket = emptyBucket(iBucket, nCount);
        // delete bucket
        if(NULL != pBucket)
        {
            delete pBucket;
            pBucket = NULL;
        }

    }

    // verify the cached count
    assert(nCount == m_nCount);

    // empty table
    m_Table.clear();
    // set count to 0
    m_nCount = 0;
}

/////////////////////////////////////////////////////////////////////////////
int vlMap::map(KEY key) const
{
    // Note: No thread protection required for this function.
    // return hashed index of bucket
    int ix = (int)
              (
                (
                    (
                        (key << 28) & 0xF0000000
                    ) |
                    (
                        (key >>  4) & 0x0FFFFFFF
                    )
                ) %
                (unsigned long)(m_Table.size())
              );
    return ix;
}

/////////////////////////////////////////////////////////////////////////////
int vlMap::size()
{
    // Note: No thread protection required for this function.
    // return the cached count of total elements
    return m_nCount;
}

/////////////////////////////////////////////////////////////////////////////
void vlMap::clear()
{
    VL_AUTO_LOCK(m_Lock);   // for multi-thread protection

    // debug count
    int nCount = 0;

    // for each bucket in table
    for(int iBucket = 0; iBucket < m_Table.size(); iBucket++)
    {
        emptyBucket(iBucket, nCount); // empty the bucket
    }

    // verify the cached count
    assert(nCount == m_nCount);

    // set count to 0
    m_nCount = 0;
    //auto_unlock_ptr(m_Lock);
}

/////////////////////////////////////////////////////////////////////////////
vlVector * vlMap::emptyBucket(int iBucket, int & nCount)
{
    VL_AUTO_LOCK(m_Lock);   // for multi-thread protection

    // bucket pointer
    vlVector * pBucket = NULL;

    // bounds check
    if((iBucket >= 0) && (iBucket < m_Table.size()))
    {
        // get bucket
        pBucket = (vlVector*)(m_Table[iBucket]);

        // NULL check
        if(NULL != pBucket)
        {
            // get bucket size
            int nBucketSize = pBucket->size();
            // add to count
            nCount += nBucketSize;

            // for each item in bucket
            for(int iItem = 0; iItem < pBucket->size(); iItem++)
            {
                // get item
                vlMapItem * pItem = (vlMapItem*)(pBucket->operator[](iItem));
                // delete item
                if(NULL != pItem)
                {
                    delete pItem;
                    pItem = NULL;
                }

            }

            // clear the bucket
            pBucket->clear();
        }
    }

    // return the bucket
    return pBucket;
}

/////////////////////////////////////////////////////////////////////////////
vlMap::ELEMENT & vlMap::getOrCreateElementAt(vlMap::KEY key)
{
    VL_AUTO_LOCK(m_Lock);   // for multi-thread protection

    // locate element
    vlMapItem * pItem = locate(key);

    // if found
    if(NULL != pItem)
    {
        // return element
        return pItem->m_Element;
    }

    // else create placeholder and return it
    return setElementAt(key, 0);
}

/////////////////////////////////////////////////////////////////////////////
vlMap::ELEMENT vlMap::getElementAt(vlMap::KEY key) const
{
    VL_AUTO_LOCK(m_Lock);   // for multi-thread protection

    // locate element
    vlMapItem * pItem = locate(key);

    // if found
    if(NULL != pItem)
    {
        // return element
        return pItem->m_Element;
    }

    // else return nothing
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
vlMap::ELEMENT vlMap::getElementAtIntegerIndex(int iElement, KEY & rKey) const
{
    VL_AUTO_LOCK(m_Lock);   // for multi-thread protection

    // set key to 0 to start with
    rKey = 0;

    // previous count
    int nPrevCount = 0;

    // for each bucket in table
    for(int iBucket = 0; iBucket < m_Table.size(); iBucket++)
    {
        // get bucket
        vlVector * pBucket = (vlVector*)(m_Table[iBucket]);

        // get current count
        int nCurCount = nPrevCount + pBucket->size();

        // check if the element is in this bucket
        if((iElement >= nPrevCount) && (iElement < nCurCount))
        {
            // index of item in bucket
            int iItem = iElement - nPrevCount;

            // bounds check
            if((iItem >= 0) && (iItem < pBucket->size()))
            {
                // get item
                vlMapItem * pItem = (vlMapItem*)(pBucket->operator[](iItem));

                // set the return param
                rKey = pItem->m_Key;

                // return element
                return pItem->m_Element;
            }
        }

        // update previous count
        nPrevCount = nCurCount;
    }

    // return nothing
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
bool vlMap::getFirstKeyForElement(ELEMENT element, KEY & rKey) const
{
    VL_AUTO_LOCK(m_Lock);   // for multi-thread protection

    // set key to 0 to start with
    rKey = 0;

    // for each bucket in table
    for(int iBucket = 0; iBucket < m_Table.size(); iBucket++)
    {
        // get bucket
        vlVector * pBucket = (vlVector*)(m_Table[iBucket]);
        // item index
        int iItem = -1;
        // get item
        vlMapItem * pItem = locateElementInBucket(pBucket, element, iItem);

        // NULL check
        if(NULL != pItem)
        {
            // set return param
            rKey = pItem->m_Key;
            // return success
            return true;
        }
    }

    // return failure
    return false;
}

/////////////////////////////////////////////////////////////////////////////
bool vlMap::isKeyPresent(KEY key) const
{
    VL_AUTO_LOCK(m_Lock);   // for multi-thread protection

    // locate element
    vlMapItem * pItem = locate(key);

    // return status
    return (NULL != pItem);
}

/////////////////////////////////////////////////////////////////////////////
vlMap::ELEMENT vlMap::operator[](KEY key) const
{
    VL_AUTO_LOCK(m_Lock);   // for multi-thread protection

    // return element
	return getElementAt(key);
}

/////////////////////////////////////////////////////////////////////////////
vlMap::ELEMENT & vlMap::operator[](KEY key)
{
    VL_AUTO_LOCK(m_Lock);   // for multi-thread protection

    // return element or create placeholder
	return getOrCreateElementAt(key);
}

/////////////////////////////////////////////////////////////////////////////
vlMap::ELEMENT & vlMap::setElementAt(KEY key, ELEMENT element)
{
    VL_AUTO_LOCK(m_Lock);   // for multi-thread protection

    // locate element
    vlMapItem * pItem = locate(key);

    // if found
    if(NULL != pItem)
    {
        // set value
        pItem->m_Element = element;
    }
    else // create new placeholder
    {
        // find a bucket for key
        vlVector * pBucket = getBucket(key);

        // check for failure
        if(NULL != pBucket)
        {
            // create and initialize new item
            pItem = new vlMapItem(key, element);

            // check allocation
            if(NULL != pItem)
            {
                // place the item in the bucket
                pBucket->add(vlVector::element(pItem));
                // increment cached count of total elements
                m_nCount++;
            }
            else // allocation failure
            {
                // invoke debug break
                assert(false);
            }
        }
    }

    // check for failure
    if(NULL != pItem)
    {
        // return element
        return pItem->m_Element;
    }

    // else return nothing
    static ELEMENT nothing = 0;
    nothing = 0;
    return nothing;
}

/////////////////////////////////////////////////////////////////////////////
bool vlMap::removeElementAt(KEY key)
{
    VL_AUTO_LOCK(m_Lock);   // for multi-thread protection

    // get the bucket
    vlVector * pBucket = getBucket(key);

    // check for failure
    if(NULL != pBucket)
    {
        // item index
        int iItem = -1;
        // get item
        vlMapItem * pItem = locateItemInBucket(pBucket, key, iItem);

        // NULL check
        if(NULL != pItem)
        {
            // remove item
            pBucket->remove(iItem);
            // delete item
            if(NULL != pItem)
            {
                delete pItem;
                pItem = NULL;
            }

            // decrement cached count of total elements
            m_nCount--;
            // return success
            return true;
        }
    }

    // return failure
    return false;
}

/////////////////////////////////////////////////////////////////////////////
bool vlMap::removeAllEntriesOfElement(ELEMENT element)
{
    VL_AUTO_LOCK(m_Lock);   // for multi-thread protection

    // local variables
    bool bRemoved = false;

    // for each bucket in table
    for(int iBucket = 0; iBucket < m_Table.size(); iBucket++)
    {
        // get bucket
        vlVector * pBucket = (vlVector*)(m_Table[iBucket]);

        // item index
        int iItem = -1;
        // item pointer
        vlMapItem * pItem = NULL;

        // while there are more of this element in the bucket
        while(NULL != (locateElementInBucket(pBucket, element, iItem)))
        {
            // atleast one was found
            bRemoved = true;
            // remove the element
            pBucket->remove(iItem);
        }
    }

    // return result
    return bRemoved;
}

/////////////////////////////////////////////////////////////////////////////
vlVector * vlMap::getBucket(KEY key) const
{
    VL_AUTO_LOCK(m_Lock);   // for multi-thread protection

    // bucket pointer
    vlVector * pBucket = NULL;

    // get bucket index
    int iBucket = map(key);

    // bounds check
    if((iBucket >= 0) && (iBucket < m_Table.size()))
    {
        // get bucket
        pBucket = (vlVector*)(m_Table[iBucket]);
    }

    // return the bucket;
    return pBucket;
}

/////////////////////////////////////////////////////////////////////////////
vlMap::vlMapItem * vlMap::locate(KEY key) const
{
    VL_AUTO_LOCK(m_Lock);   // for multi-thread protection

    // get the bucket
    vlVector * pBucket = getBucket(key);

    // check for failure
    if(NULL != pBucket)
    {
        // item index
        int iItem = -1;
        // locate element in bucket
        return locateItemInBucket(pBucket, key, iItem);
    }

    // return NULL
    return NULL;
}

/////////////////////////////////////////////////////////////////////////////
vlMap::vlMapItem * vlMap::locateItemInBucket(vlVector * pBucket, KEY key, int & riItem) const
{
    VL_AUTO_LOCK(m_Lock);   // for multi-thread protection

    // locate key in bucket by calling locateElementInBucket()
    return locateElementInBucket(pBucket, 0, riItem, key, false);
}

/////////////////////////////////////////////////////////////////////////////
vlMap::vlMapItem * vlMap::locateElementInBucket(vlVector * pBucket,ELEMENT element,
                                                int & riItem, KEY key,
                                                bool bElement) const
{
    VL_AUTO_LOCK(m_Lock);   // for multi-thread protection

    // set riItem to invalid to start with
    riItem = -1;

    // NULL check
    if(NULL != pBucket)
    {
        // for each item in bucket
        for(int iItem = 0; iItem < pBucket->size(); iItem++)
        {
            // get item
            vlMapItem * pItem = (vlMapItem*)(pBucket->operator[](iItem));
            // check item
            if(((         bElement) && (pItem->m_Element == element)) ||    // searching for element ?
               ((false == bElement) && (pItem->m_Key     == key    ))       // searching for key     ?
              )
            {
                // set the return param
                riItem = iItem;
                // return item
                return pItem;
            }
        }
    }

    // return NULL
    return NULL;
}

/////////////////////////////////////////////////////////////////////////////
void vlMap::lock()
{
    m_Lock.Acquire();
}

/////////////////////////////////////////////////////////////////////////////
void vlMap::unlock()
{
    m_Lock.Release();
}

/////////////////////////////////////////////////////////////////////////////
