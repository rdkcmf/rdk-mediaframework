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

#include <rdk_debug.h>
#include <stdio.h>
//#include "vlPlatform.h"
#include <stdlib.h>
//#include "cAssert.h"
//#include "vlDefs.h"
#include "vlMutex.h"
//#include "vlSystem.h"
#include "rmf_error.h"
#include <errno.h>
//#include "vlExportTypes.h"


/////////////////////////////////////////////////////////////////////
//
// Creates an unnamed/named mutex object. name specifies identification
// for mutex object. It is useful for synchronisation between related threads
// threads
///////////////////////////////////////////////////////////////////////

vlMutex::vlMutex()
{
    bool bRet = false;
    bRet = Create();
//  assert(false != bRet);
}

//////////////////////////////////////////////////////////////////
//
// Destructor for Mutex
//
//////////////////////////////////////////////////////////////////


vlMutex::~vlMutex()
{
rmf_osal_mutexDelete(m_lMutex);

}

bool vlMutex::Create()
{
    m_bAlreadyExists = false;
    
        rmf_osal_mutexNew(&m_lMutex);

/*
#ifdef WIN32
    m_MutexLastError = GetLastError();

    if(ERROR_ALREADY_EXISTS == m_MutexLastError)
    {
        m_bAlreadyExists = true;
    }
#endif
*/
    if (m_lMutex == 0)
        return false;
    else
    return true;
}

//////////////////////////////////////////////////////////////////
//
// Acquires the Mutex
//
//////////////////////////////////////////////////////////////////

vlMutex::MutexAcquireResult  vlMutex::Acquire () const
{
#ifndef LINUX /*Don't know why VXWORKS or WIN32 no longer need this ..BUT LINUX does */
    return (vlMutex::MutexAcquireResult) rmf_osal_mutexAcquire(m_lMutex);
#else
    rmf_Error stat = rmf_osal_mutexAcquire(m_lMutex);
    MutexAcquireResult Result = ERRORED;

        switch(stat)
        {
                case RMF_SUCCESS:
                        Result = ACQUIRED;
						//RDK_LOG(RDK_LOG_INFO, "LOG.RDK.OS","\n\n\n\n\n\n################## Mutex is aquired;success");
						
                break;

                default:
                        Result = ERRORED;
						//RDK_LOG(RDK_LOG_INFO, "LOG.RDK.OS","\n\n\n\n\n\n ##########Mutex is not aquired;Failed");
						
                break;
        }
		
  return Result;
#endif /* LINUX */
}

vlMutex::MutexAcquireResult  vlMutex::TryAcquire () const
{
rmf_Error stat = rmf_osal_mutexAcquireTry(m_lMutex);
MutexAcquireResult Result = ERRORED;
    switch(stat)
        {
                case RMF_SUCCESS:
                        Result = ACQUIRED;
                break;

                default:
                        Result = ERRORED;
                break;
        }
  return Result;
}


vlMutex::MutexAcquireResult  vlMutex::lock()
{
    return Acquire();
}

vlMutex::MutexAcquireResult  vlMutex::lock(const char *location)
{
    return Acquire();
}

//////////////////////////////////////////////////////////////////
//
// Releases the Mutex
//
//////////////////////////////////////////////////////////////////

bool vlMutex::Release() const
{
rmf_Error stat = rmf_osal_mutexRelease(m_lMutex);

  if (RMF_SUCCESS == stat){
      return true;
  	}
  else {
      return false;
  	}
}

bool vlMutex::unlock()
{
    return Release();
}

//////////////////////////////////////////////////////////////////
vlAutoLock::vlAutoLock(const vlMutex & _rLock)
    : m_rLock(_rLock)
{
    m_rLock.Acquire();
}

vlAutoLock::~vlAutoLock()
{
    m_rLock.Release();
}



rmf_osal_MutexClass::rmf_osal_MutexClass()
{
      rmf_osal_mutexNew(&mutex_id);
}

rmf_Error rmf_osal_MutexClass::Acquire()
{
    return rmf_osal_mutexAcquire(mutex_id);

}

rmf_Error rmf_osal_MutexClass::Release()
{
    return rmf_osal_mutexRelease(mutex_id);

}

rmf_osal_MutexClass::~rmf_osal_MutexClass()
{
    rmf_osal_mutexDelete(mutex_id);

}

//////////////////////////////////////////////////////////////////
