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


#ifndef __OOB_RDC_H__
#define __OOB_RDC_H__

#include "cmThreadBase.h"
//
//#include "pfcresource.h"
#include "core_events.h"
//#include "pfctimer.h"
#ifdef GCC4_XXX
#include <list>
#else
#include <list.h>
#endif

typedef enum
{
    TableManagerTableSuccess,
    TableManagerTableTimeOut,
    TableManagerTableIncompleteFiltering,
    TableManagerTableVersionChangeDetected
} CardManagerTableReturnCode;

class CardManager;

class cOOB : public CMThread
{
public:
    cOOB(CardManager *c,char *name);
    void run(void );
    void initialize(void);
    ~cOOB(){}
    // changed: aug-06-2007: prototype changed
    static int   OOB_RDC_cb(void *data, void *buf, int bufSize, unsigned long * pnRet);
//    pfcTimer    ptimer;
    int bCancelled;
private:
    CardManager *cm;
};



#endif







