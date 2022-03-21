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

#ifndef __VL_MAP_H
#define __VL_MAP_H

/////////////////////////////////////////////////////////////////////////////
#include "vlVector.h"
#include "rmf_osal_sync.h"
#include "vlMutex.h"

/////////////////////////////////////////////////////////////////////////////
// vlMap: Implements a hashtable for mapping unsigned long to unsigned long
/////////////////////////////////////////////////////////////////////////////
class vlMap
{
public:
    // element data type
	typedef unsigned long ELEMENT;

    // key data type
	typedef unsigned long KEY;

    // constructor
    // (for efficiency nTableSize should
    // be a prime number greater than 16)
    vlMap(int nTableSize = 17,
          int nBucketCapacity = 5,
          int nBucketIncrement = 5);

    //destructor
    ~vlMap();

    // element access operator (read only)
    ELEMENT operator[](KEY key) const;

    // element access operator (creates new if key is not present)
    ELEMENT & operator[](KEY key);

    // returns the cached count of total elements
    int size();

    // clears the map, added elements are not deleted
    void clear();

    // returns the specified element or adds placeholder it if not present
    ELEMENT & getOrCreateElementAt(KEY key);

    // returns the specified element (read only)
    ELEMENT getElementAt(KEY key) const;

    // Returns the specified element (read only)
    // May return different elements at different
    // times depending on the state of the map
    // Hence should not be used in conjuction
    // with addition or removal of elements
    ELEMENT getElementAtIntegerIndex(int iElement, KEY & rKey) const;

    // search for element (slow search)
    bool getFirstKeyForElement(ELEMENT element, KEY & rKey) const;

    // search for key
    bool isKeyPresent(KEY key) const;

    // sets the specified element
    ELEMENT & setElementAt(KEY key, ELEMENT element);

    // removes the specified element
    bool removeElementAt(KEY key);

    // removes all entries that have the specified element value
    bool removeAllEntriesOfElement(ELEMENT element);

    // external access to map's mutex
    void lock();

    // external access to map's mutex
    void unlock();

private:
    // returns mapped index of bucket
    int map(KEY key) const;

    // empties and returns the specified bucket
    // and adds nCount with the count of items
    // just before it was emptied
    vlVector * emptyBucket(int iBucket, int & nCount);

    // the table
    vlVector m_Table;

    // cached count of total elements
    int      m_nCount;

    // protection mutex
    vlMutex  m_Lock;

    // item struct
    struct vlMapItem
    {
        // constructor
        vlMapItem(KEY key, ELEMENT element);

        // the key
        KEY     m_Key;

        // the element
        ELEMENT m_Element;
    };

    // returns the bucket holding the key
    vlVector * getBucket(KEY key) const;

    // locate specified element
    vlMapItem * locate(KEY key) const;

    // locate specified item in specified bucket
    vlMapItem * locateItemInBucket(vlVector * pBucket, KEY key, int & riItem) const;

    // locate specified element in specified bucket
    vlMapItem * locateElementInBucket(vlVector * pBucket, ELEMENT element,
                                      int & riItem, KEY key = 0,
                                      bool bElement = true) const;
};

/////////////////////////////////////////////////////////////////////////////
#endif //__VL_MAP_H
/////////////////////////////////////////////////////////////////////////////

