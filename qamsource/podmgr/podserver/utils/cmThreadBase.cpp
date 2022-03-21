/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2018 RDK Management
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

#include "cmThreadBase.h"
#include "string.h"
#include "rdk_debug.h"
#include "rmf_osal_mem.h"

#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif

CMThread::CMThread (char *name)
{
       thread_running = 0;
	thread_name	= NULL;   
       tid = 0;	   
	if (name)
	{		
		rmf_osal_memAllocP(RMF_OSAL_MEM_POD, (strlen(name) + 1), (void**) &this->thread_name);
		this->thread_name[0] = 0;
		strcpy(this->thread_name, name);			  
	}

}

CMThread::~CMThread ()
{
          rmf_osal_memFreeP(RMF_OSAL_MEM_POD, this->thread_name);	
}


void 
CMThread::entry_point (void *parameters)
{

	CMThread *pt = (CMThread *) parameters;
    
	// execute the thread
	pt->thread_running = 1;

       pt->run ();

	pt->thread_running = 0;
}

void
CMThread::start ()
{
	rmf_Error err = rmf_osal_threadCreate (CMThread::entry_point, this, 
		RMF_OSAL_THREAD_PRIOR_DFLT, RMF_OSAL_THREAD_STACK_SIZE, &tid, thread_name);

	if(RMF_SUCCESS != err)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD",
                  "cardManager thread creation Failed %s\n", thread_name);
	}   
}


void
CMThread::end ()					// need to join after this if synchronous
{

}


uint8_t
CMThread::running ()
{
	return thread_running;
}








