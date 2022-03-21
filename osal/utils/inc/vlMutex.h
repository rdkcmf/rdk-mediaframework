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



#ifndef vlMutex_H
#define vlMutex_H
#include "rmf_osal_sync.h"
#include <stdlib.h>
#include <errno.h>
//#include "vlExportTypes.h"
#include "rmf_error.h"


typedef enum
{

    C_SYS_OBJ_ACQUIRED  = 0,
    C_SYS_OBJ_TIMEDOUT  = -ETIME,
    C_SYS_OBJ_ERROR = -EINVAL

} cSysObjectAcquireResult;



class  vlMutex
{
  private:

//    long    m_MutexLastError;
    bool    m_bAlreadyExists;
    rmf_osal_Mutex m_lMutex;

    //copy constructor and assignment operator are private to prevent
    //creation of temporary objects that can result in the destructor
    //removing the global mutex resource when the temporary object
    //goes out of scope. you can still pass the mutex object by ref
    vlMutex(const vlMutex &mtxObject) {};
  public:
    enum MutexAcquireResult { ACQUIRED = C_SYS_OBJ_ACQUIRED, TIMEDOUT = C_SYS_OBJ_TIMEDOUT, ERRORED = C_SYS_OBJ_ERROR};

    vlMutex();

    ~vlMutex();
    bool   Create();
    MutexAcquireResult   Acquire() const;
    MutexAcquireResult   TryAcquire() const; /* may not be supported in all OS */
    bool   Release() const;

    MutexAcquireResult lock();
    MutexAcquireResult lock(const char *location);
    bool unlock();
    //
    // The following should be called after Create, otherwise error returned is undefined
    //
    long     GetError()
    {
        return 0;
    };

    bool    NameMutexExist()
    {
        return m_bAlreadyExists;
    }
  private:
      //this ctr calls Create
        //also. This is used by SoftwareElement alone.
        //becuase SoftwareElement has a static mutex object
        friend class SoftwareElement;
};

//////////////////////////////////////////////////////////////////
// vlAutoLock : VL_AUTO_LOCK() implementation class
// (To avoid incorrect usage, do not use by itself)
// {use with VL_AUTO_LOCK() macro only}
//////////////////////////////////////////////////////////////////
class  vlAutoLock
{
    const vlMutex & m_rLock;
public:
    vlAutoLock(const vlMutex & _rLock);
    ~vlAutoLock();
};

//////////////////////////////////////////////////////////////////
// The VL_AUTO_LOCK() macro locks the remainder of the
// C++ code block using the supplied mutex and releases
// the lock automatically when the lock goes out of scope.
// The scope may also end on a return, break, goto or
// continue statements and exceptions that unwind the stack.
// WARNING: Do not put more than one VL_AUTO_LOCK()
// statements in the same scope. The previous must go out of
// scope before the next is issued.
//
// Example:
//
//  {VL_AUTO_LOCK(m_lock); // Begin Thread Lock
//
//      // Protected Block
//
//  } // End Thread Lock
//
//////////////////////////////////////////////////////////////////
#define VL_AUTO_LOCK(lock)  vlAutoLock  _Lock_##lock##_(lock);
//////////////////////////////////////////////////////////////////


class rmf_osal_MutexClass
{

    rmf_osal_Mutex mutex_id;
public:
       rmf_osal_MutexClass();
       ~rmf_osal_MutexClass();
	rmf_Error Acquire();
       rmf_Error Release();
};

#endif  //vlMutex_H

