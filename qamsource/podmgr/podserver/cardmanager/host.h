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

 

  
#ifndef __CM_HOSTCONTROL_H__
#define __CM_HOSTCONTROL_H__

#include "cmThreadBase.h"
//
//#include "pfcresource.h"
#include "rmf_osal_event.h"
#ifdef GCC4_XXX
#include <list>
#else
#include <list.h>
#endif
//#include "hal_api.h"
#include "core_events.h"
#ifdef __cplusplus
extern "C" {
#endif
    extern void hostControlInit(void);
    extern void hostProc(void* par); 
#ifdef __cplusplus
}
#endif



typedef enum
{
    OOB_DS_QPSK_TUNE_OK        = 0,
    OOB_DS_QPSK_TUNE_NA        = 1,
    OOB_DS_QPSK_TUNE_FAILED    = 2,    //busy
    OOB_DS_QPSK_TUNE_BAD_PARAM = 3,
    OOB_DS_QPSK_TUNE_TIMED_OUT = 4,  //used only by POD
    OOB_DS_QPSK_TUNE_IGNORE    = 5   //used only by POD==reserved
} oob_ds_tune_status;

typedef enum
{
    OOB_US_QPSK_TUNE_OK        = 0,
    OOB_US_QPSK_TUNE_NA        = 1,
    OOB_US_QPSK_TUNE_FAILED    = 2,    //busy
    OOB_US_QPSK_TUNE_BAD_PARAM = 3,
    OOB_US_QPSK_TUNE_TIMED_OUT = 4,  //used only by POD
    OOB_US_QPSK_TUNE_IGNORE    = 5   //used only by POD==reserved
} oob_us_tune_status;
 
class cHostControl:public  CMThread
{

public:

    cHostControl(CardManager *cm,char *name);
    ~cHostControl();
    void initialize(void);
    void run(void);

    int GetModAndFreqForSourceId(unsigned short SrcId, unsigned long *pMod, unsigned long *pFreq);

//    pfcSemaphore hostControlSem; 
    //DEVICE_LOCK_t eTuneStatus;
private:
    rmf_osal_Mutex    hostControlMutex;
    CardManager *cm;
    rmf_osal_eventqueue_handle_t event_queue;    
    
};





#endif













