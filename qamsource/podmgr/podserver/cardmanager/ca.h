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

 

  
#ifndef __CM_CA_H__
#define __CM_CA_H__
#include "core_events.h"
#include "cmThreadBase.h"
//
//#include "pfcresource.h"
#include "rmf_osal_event.h"
#ifdef GCC4_XXX
#include <list>
#else
#include <list.h>
#endif
//#include "pmt.h"
#ifdef __cplusplus
extern "C" {
#endif
    extern void ca_init(void);
    extern  void caProc(void*);
    //extern int ca_resopen();
#ifdef __cplusplus
}
#endif


typedef unsigned long IOResourceHandle;
//#define MAX_RESOURCE_HANDLES 4
typedef struct vlgCAResourceHandle_s
{
    int    ResourceHandle;
    unsigned char Busy;
    unsigned char NumUsersCount;
    unsigned char Ltsid;
     unsigned short PrgNum;
}CAResourceHandle_t;

typedef enum
{
ca_other_Res1            = 0x00,
ca_descrambling_possible = 0x01,
ca_purchase_dialogue     = 0x02,
ca_technical_dialogue    = 0x03,
ca_fail_no_entitlement   = 0x71,
ca_fail_technical_reason = 0x73,
//RFU other values other values 00-7F
}enum_CA_enable;

typedef enum
{
ca_list_more   = 0x00,
ca_list_first  = 0x01,
ca_list_last   = 0x02,
ca_list_only   = 0x03,
ca_list_add    = 0x04,
ca_list_update = 0x05,
//ca_list_res1  all other values
}enum_ca_pmt_list_management;

typedef enum
{
ca_pmt_cmd_ok_descrambling = 0x01,
ca_pmt_cmd_ok_mmi          = 0x02,
ca_pmt_cmd_query           = 0x03,
ca_pmt_cmd_not_selected    = 0x04,
//ca_pmt_cmd_res1  all other values
}enum_ca_pmt_cmd_id; //for ca_pmt_cmd_id_value


 
class cCA:public  CMThread
{

public:

    cCA(CardManager *cm,char *name);
    ~cCA();
    void initialize(void);
    void run(void);
    int ca_pmt_process();
    void NotifyChannelChange( IOResourceHandle    hIn,unsigned short usVideoPid,unsigned short usAudioPid);
    void dump();

    
private:
    
    CardManager         *cm;
    rmf_osal_eventqueue_handle_t         event_queue;    
    unsigned short         usVideoPid;
    unsigned short         usAudioPid;
    IOResourceHandle        hIn;

};





#endif









