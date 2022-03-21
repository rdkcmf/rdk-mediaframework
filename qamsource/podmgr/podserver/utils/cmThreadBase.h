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



/** \file pfcthread.h A thread object based on pthreads. */

#ifndef PFCTHREAD_H
#define PFCTHREAD_H

#include <rmf_osal_thread.h>

// The thread is automatically deleted when run() finishes if
// autodelete is 1.  If you don't want this, set autodelete to 0.

// REVIEW-jdm  SCHED_RR (round-robin) is used, not SCHED_FIFO
// The thread will be run in SCHED_FIFO if realtime is 1.

// If synchronous is 1 the thread must be joined.

/** A thread object based on pthreads.
 *
 * A general purpose thread object for creating and managing multiple threads of execution.
 * Generalize and hide pthread ('POSIX Threads')-specific implementation: make portable
 * to systems without pthread specific support.
 */

class CMThread
{
// REVIEW-jdm  Suggest moving private method to end of file. It is NOT
// REVIEW-jdm  part of the object's "API"
private:
  //! The entry_point to the thread. Passed to the OS specific thread routine.
  static void entry_point (void *parameters);

protected:
  //! Must be overridden: implements the body of execution for the thread's life.
  virtual void run () = 0;

public:

    CMThread (char *name); 
    
  //! Not implemented.
  virtual ~ CMThread ();

  //! Initialize and start the thread based on \a synchronous and \a realtime member var settings.
  void start ();

  //! Cancel this thread. Calls \c cancel().
  void end ();

  /**
   * Return if thread is running. If the run procedure is finished, it
   * returns 0 even if it hasn't been joined.
   */
  uint8_t running ();


private:
  uint8_t thread_running;
  rmf_osal_ThreadId tid;
  char *thread_name;

};

#endif
