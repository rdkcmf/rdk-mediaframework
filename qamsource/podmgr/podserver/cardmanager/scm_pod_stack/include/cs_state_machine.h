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




#ifndef _CS_STATE_MACHINE_H_
#define _CS_STATE_MACHINE_H_

#ifdef __cplusplus
extern "C" {
#endif

//#include "lcsm_userSpaceInterface.h"
#include "cs_queue_manager.h"
#include "poddef.h"

typedef enum
{
    CS_SM_OK           =  0,
    CS_SM_CREATE       = -1,  // not enough memory to create State machine
    CS_SM_NULL         = -2,  // State Machine Has Previously been destroyed
    CS_SM_BAD_STATE    = -3, // State is bigger than allocated state table
    CS_SM_BAD_SUBSTATE = -4, // requested Substate doesnot return

}CS_SM_RET;


/*
 Abstract Event List for
 State Machine Lists
*/

/* useful events start at 0 and above */
typedef enum
{
   CS_SM_INIT  = -1,
   CS_SM_TERM  = -2,


}CS_SM_EVENT_LIST;


/*
**
**
** OS Depedend Definitions
**
**
*/





/*
**
**
** End of OS Dependent Definitions
**
*/


typedef struct CS_SM_CONTROL  *CS_SM_PTR;


typedef CS_SM_PTR (*CS_SM_PROCESS_INITIALIZE )( void                   *handle, 
                                                unsigned int           queue );
                                         


typedef void  (*CS_SM_PROCESS_EVENT)( void                   *handle, 
                                      unsigned int           queue,
                                      CS_SM_PTR              smControl, 
                                      LCSM_EVENT_INFO           *eventRecord);


typedef void (*CS_SM_PROCESS_TERMINATE )( void                   *handle, 
                                          unsigned int           queue,
                                          CS_SM_PTR              smControl ); 
                                       





typedef struct CS_SM_CONTROL
{
   unsigned                numberOfStates; /* This is set when state machine is contructed */
   unsigned                currentState;
   QUEUE_CONTROL           queueControl;
   struct CS_SM_CONNECTOR  *connector;
   CS_SM_PROCESS_EVENT     firstState; /* First stateVector */


}CS_SM_CONTROL;





typedef struct CS_SM_CONNECTOR
{

    unsigned                  id;
    CS_SM_PROCESS_EVENT       cs_sm_processEvent;
    CS_SM_PROCESS_TERMINATE   cs_sm_terminate;
    CS_SM_PTR                 cs_stateMachine;
    QUEUE_ELEMENT             queueElement;

}CS_SM_CONNECTOR;







CS_SM_CONTROL *CS_SM_constructStateMachine( void *handle, unsigned   numberOfStates );


CS_SM_CONTROL *CS_SM_addTermintationHandler( CS_SM_CONTROL *smControl, CS_SM_PROCESS_TERMINATE processTerminate );

CS_SM_RET      CS_SM_terminateStateMachine( void *handle, 
                                       unsigned int queue,
                                       CS_SM_CONTROL *smControl );

CS_SM_RET      CS_SM_addState( CS_SM_CONTROL *smControl, unsigned state, CS_SM_PROCESS_EVENT stateVector );



CS_SM_RET      CS_SM_initializeStateMachine(  void      *handle, 
                                       unsigned int      queue,
                                       CS_SM_CONTROL    *smControl, 
                                       unsigned          startingState );

CS_SM_RET      CS_SM_changeState(  void *handle, unsigned int queue, CS_SM_CONTROL *smControl, unsigned state );

CS_SM_RET      CS_SM_processEvent( void *handle, unsigned int queue, CS_SM_CONTROL *smControl, LCSM_EVENT_INFO *eventRecord );

unsigned       CS_SM_getCurrentState( CS_SM_CONTROL *smControl );

CS_SM_RET      CS_SM_deleteSubState( void *handle, unsigned int queue, CS_SM_CONTROL *smControl );

CS_SM_RET      CS_SM_terminateAllSubStates( void           *handle, 
                                            unsigned int   queue,
                                            CS_SM_CONTROL  *smControl );


CS_SM_RET      CS_SM_deleteNthSubState( void          *handle,
                                        unsigned int  queue,
                                        CS_SM_CONTROL *smControl,
                                        unsigned       nthElement );


CS_SM_RET      CS_SM_deleteSubStateID( void *handle, 
                                       unsigned queue,
                                       CS_SM_CONTROL  *smControl,
                                       unsigned        subStateID );

CS_SM_RET      CS_SM_addSubState( unsigned    subStateID,
                                  void        *handle, 
                                  unsigned int queue,
                                  CS_SM_CONTROL *smControl,
                                  CS_SM_PROCESS_INITIALIZE  csProcessInitialize,
                                  CS_SM_PROCESS_EVENT       csProcessEvent,
                                  CS_SM_PROCESS_TERMINATE   csProcessTerminate );


CS_SM_RET      CS_SM_subStateProcessEvent( void *handle, unsigned int queue, CS_SM_CONTROL *smControl, LCSM_EVENT_INFO *eventRecord );










#ifdef __cplusplus
}
#endif

#endif    // _STATE_MACHINE_H_

