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



 

  
#ifndef __CM_FEATURE_CONTROL_H__
#define __CM_FEATURE_CONTROL_H__

#include "cmThreadBase.h"
//
//#include "pfcresource.h"
#include "rmf_osal_event.h"
#include "core_events.h"
#ifdef GCC4_XXX
#include <list>
#else
#include <list.h>
#endif 

class CardManager;

#ifdef __cplusplus
extern "C" {
#endif
    void feature_init(void);
     void featureProc( void * par );
#ifdef __cplusplus
}
#endif

typedef enum
{
    CM_FEATURE_DEFAULT,
    CM_FEATURE_CHANGED,
    CM_FEATURE_SET,
    CM_FEATURE_RESET

}State_t;

 
class cGenFeatureControl:public  CMThread
{

public:

    cGenFeatureControl(CardManager *cm,char *name);
    ~cGenFeatureControl();
    void initialize(void);
    void run(void);
     
private:
    
    CardManager *cm;
    rmf_osal_eventqueue_handle_t event_queue;    
};



class cGenericFeatureBaseClass 
{
public:
    cGenericFeatureBaseClass();
    cGenericFeatureBaseClass(uint8_t *destBuffer);   
    ~cGenericFeatureBaseClass();
    
    bool     IsSupported();  // indicates whether host cares about this parameter 
    uint8_t GetFeatureID();
    void    Set_FeatureSupported();
    void    Clear_FeatureSupported();
    
    //PackBits(uint8_t  *destPtr, uint8_t *valuePtr, uint32_t 
    void    SetFeatureState( State_t);
    State_t    GetFeatureState();
    
    int put_bits(int32_t value,int n);
    
    /**
 * Set #curr_bit_index to #curr_bit_index + \e num.
 */
    void skip_bits(int num //!< Number of bits to skip.
                 );

/**
 * \return #curr_bit_index.
 */
      unsigned long get_pos() { return curr_bit_index; }


private:        
    uint8_t          *buffer;
    
/**
 * Current bit index.
 */
      unsigned long curr_bit_index;
    uint8_t         featureID;
    bool         supported;
    State_t     state; //default - changed - set - reset
    

};



#endif





























