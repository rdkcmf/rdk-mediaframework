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



 

  
#ifndef __RSMGR_H__
#define __RSMGR_H__

#include "cmThreadBase.h"
//
//#include "pfcresource.h"
#include "rmf_osal_event.h"
#ifdef GCC4_XXX
#include <list>
#else
#include <list.h>
#endif
 
#include "core_events.h"
class cRsMgr:public  CMThread
{

public:

    cRsMgr(CardManager *cm,char *name);
    ~cRsMgr();
    void initialize(void);
    void run(void);
    
private:
    CardManager *cm;
    rmf_osal_eventqueue_handle_t event_queue;    
};






#endif

