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




#include <stdlib.h>
#include <stdio.h>
#include "cs_state_machine.h"
#include <rdk_debug.h>
#include "rmf_osal_mem.h"

#if USE_SYSRES_MLT
#include "mlt_malloc.h"
#endif

#define __MTAG__ VL_CARD_MANAGER

//#include <string.h>
#ifdef __cplusplus
extern "C" {
#endif


#define CS_SM_UNITIALIZED_STATE  0xffffffff

// Add file name and revision tag to object code
#define REVISION  __FILE__ " $Revision: 1.2 $"
static char *cs_state_machine_tag = REVISION;


static CS_SM_RET checkSM( CS_SM_CONTROL *smControl, unsigned state );






/*
   This function constructs a state machine Control Structure

*/

CS_SM_CONTROL *CS_SM_constructStateMachine(void *handle, unsigned   numberOfStates )
{

    CS_SM_CONTROL *returnValue;

	rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(CS_SM_CONTROL ) + ( (numberOfStates )*sizeof(CS_SM_PROCESS_EVENT )),(void **)&returnValue);
    if ( returnValue != NULL )
    {
        returnValue->numberOfStates = numberOfStates;
        returnValue->currentState   = CS_SM_UNITIALIZED_STATE;
        returnValue->connector      = NULL;
        createQueueControlBlock( &returnValue->queueControl,returnValue );

    }
    return returnValue;


}





/*
   This function terminates a state machine

*/

CS_SM_RET CS_SM_terminateStateMachine( void *handle,
                                 unsigned int queue,
                                 CS_SM_CONTROL *smControl )
{
    CS_SM_RET     returnValue = CS_SM_OK;
    LCSM_EVENT_INFO eventRecord;



    eventRecord.event = CS_SM_TERM;


    if ( smControl != NULL )
    {

        if ( smControl->currentState !=  CS_SM_UNITIALIZED_STATE )
        {

            // Send terminating Event to current state
            returnValue = CS_SM_processEvent( handle, queue, smControl, &eventRecord );

        }
        // free state machine memory
		rmf_osal_memFreeP(RMF_OSAL_MEM_POD, smControl);

    }
    else
    {
        returnValue = CS_SM_NULL;
    }
    return returnValue;


}









/*
    This function adds a state and the function to handle processing for this state
*/
CS_SM_RET CS_SM_addState( CS_SM_CONTROL *smControl, unsigned state, CS_SM_PROCESS_EVENT stateVector )
{
    CS_SM_RET        returnValue;
    CS_SM_PROCESS_EVENT  *vectorPtr;

    returnValue = checkSM( smControl , state );
    if ( returnValue == CS_SM_OK )
    {
        vectorPtr = &smControl->firstState;
        vectorPtr =  vectorPtr + state;
        *vectorPtr = stateVector;

    }
    return returnValue;

}


/*
   This function initializes the state machine to a starting state
*/
CS_SM_RET     CS_SM_initializeStateMachine(  void          *handle,
                                       unsigned int  queue,
                                       CS_SM_CONTROL    *smControl,
                                       unsigned      startingState )
{
    CS_SM_RET       returnValue;
    LCSM_EVENT_INFO eventRecord;

    eventRecord.event = CS_SM_INIT;



    returnValue = checkSM( smControl, startingState );
    if ( returnValue == CS_SM_OK )
    {
        smControl->currentState = startingState;
        // send initialization event to the starting state
        returnValue = CS_SM_processEvent( handle, queue, smControl, &eventRecord );
    }
    return returnValue;


}


/*
   This function changes a state of the state machine
*/
CS_SM_RET CS_SM_changeState(  void *handle, unsigned int queue, CS_SM_CONTROL *smControl, unsigned newState )
{
    CS_SM_RET returnValue;
    LCSM_EVENT_INFO eventRecord;

    eventRecord.event = CS_SM_TERM;


     RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","CS_SM_changeState: Entered \n");
    returnValue = checkSM( smControl, newState );
    if ( returnValue == CS_SM_OK )
    {

        // send termination event to current state
        CS_SM_processEvent( handle, queue, smControl, &eventRecord );
        smControl->currentState = newState;
        // send initialization event to new state
        eventRecord.event = CS_SM_INIT;
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","CS_SM_changeState: eventRecord.event = CS_SM_INIT \n");
        returnValue = CS_SM_processEvent( handle, queue, smControl, &eventRecord );
    }
    else
    {
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","CS_SM_changeState: eventRecord.event = CS_SM_TERM \n");
    }
    return returnValue;


}



/*
  Processes an event message sent to state machine
*/
CS_SM_RET CS_SM_processEvent( void *handle, unsigned int queue, CS_SM_CONTROL *smControl, LCSM_EVENT_INFO *eventRecord )
{
    CS_SM_RET         returnValue;
    CS_SM_PROCESS_EVENT   *vectorPtr;
    CS_SM_PROCESS_EVENT   stateVector;
RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","CS_SM_processEvent: Entered \n");
    if ( smControl != NULL )
    {
        returnValue = checkSM( smControl, smControl->currentState );
        if ( returnValue == CS_SM_OK )
        {
            vectorPtr = &smControl->firstState;
            vectorPtr = vectorPtr + smControl->currentState;
            stateVector = *vectorPtr;
            stateVector( handle, queue, smControl, eventRecord );


        }


    }
    else
    {
        returnValue = CS_SM_NULL;
    }
    return returnValue;

}

/*
  Performs validation checks on the state machine
*/
static CS_SM_RET checkSM( CS_SM_CONTROL *smControl, unsigned state )
{
    CS_SM_RET returnValue;

    // make sure that control is not NULL
    if ( smControl == NULL )
    {
        returnValue = CS_SM_NULL;
    }
    // make sure state is in valid range
    else if ( smControl->numberOfStates >= state )
    {
        returnValue = CS_SM_OK;
    }
    else
    {
        returnValue = CS_SM_BAD_STATE;
    }
    return returnValue;
}




unsigned CS_SM_getCurrentState( CS_SM_CONTROL *smControl )
{
    return smControl->currentState;
}



CS_SM_RET      CS_SM_addSubState( unsigned    subStateID,
                                  void        *handle,
                                  unsigned int queue,
                                  CS_SM_CONTROL *smControl,
                                  CS_SM_PROCESS_INITIALIZE  csProcessInitialize,
                                  CS_SM_PROCESS_EVENT       csProcessEvent,
                                  CS_SM_PROCESS_TERMINATE   csProcessTerminate )
{

    CS_SM_CONNECTOR *smConnector;
    CS_SM_RET        returnValue;

	rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof( CS_SM_CONNECTOR ),(void **)&smConnector);
    if( smConnector == NULL )
    {
        returnValue =  CS_SM_CREATE;
    }
    else
    {
        smConnector->id                     = subStateID;
        smConnector->cs_sm_processEvent     = csProcessEvent;
        smConnector->cs_sm_terminate        = csProcessTerminate;
        smConnector->cs_stateMachine        = csProcessInitialize( handle, queue );
        createQueueElement( &smConnector->queueElement, smConnector );
        queueToTail( &smControl->queueControl, &smConnector->queueElement );
        smConnector->cs_stateMachine->connector  = smConnector;
    }
    //smConnector->cs_sm_processEvent = csProcessEvent;

    return CS_SM_OK;

}









CS_SM_RET      CS_SM_deleteSubState( void *handle, unsigned int queue, CS_SM_CONTROL *smControl )
{
    CS_SM_CONNECTOR *smConnector;

    if( smControl != NULL )
    {
        smConnector = smControl->connector;
        if( smConnector != NULL )
        {
            smConnector->cs_sm_terminate( handle, queue, smConnector->cs_stateMachine );
            dequeueElement( &smConnector->queueElement );
			rmf_osal_memFreeP(RMF_OSAL_MEM_POD, smConnector);

        }
    }

    return CS_SM_OK;

}

CS_SM_RET      CS_SM_terminateAllSubStates( void           *handle,
                                            unsigned int   queue,
                                            CS_SM_CONTROL  *smControl )
{
    QUEUE_ELEMENT    *queueElement;
    QUEUE_ELEMENT    *temp;
    CS_SM_CONNECTOR  *smConnector;

    queueElement = getQueueHead( &smControl->queueControl  );
    if( queueElement != NULL )
    {

      while( queueElement != NULL )
      {

          temp         = getNextElement( queueElement );
          smConnector = (CS_SM_CONNECTOR *)getDataPointer( queueElement );
          smConnector->cs_sm_terminate( handle, queue, smConnector->cs_stateMachine );
          dequeueElement( queueElement );
		  rmf_osal_memFreeP(RMF_OSAL_MEM_POD, smConnector);
          queueElement = temp;
      }


    }

    return CS_SM_OK;


}

CS_SM_RET      CS_SM_subStateProcessEvent( void *handle, unsigned int queue, CS_SM_CONTROL *smControl, LCSM_EVENT_INFO *eventRecord )
{
    QUEUE_ELEMENT  *queueElement;
    QUEUE_ELEMENT  *temp;
    CS_SM_CONNECTOR  *smConnector;

    queueElement = getQueueHead( &smControl->queueControl  );
    if( queueElement != NULL )
    {

      while( queueElement != NULL )
      {

          temp         = getNextElement( queueElement );
          smConnector = (CS_SM_CONNECTOR *)getDataPointer( queueElement );
          smConnector->cs_sm_processEvent( handle, queue, smConnector->cs_stateMachine ,eventRecord );
          queueElement = temp;
      }


    }

    return CS_SM_OK;


}




CS_SM_RET      CS_SM_deleteNthSubState( void          *handle,
                                        unsigned int  queue,
                                        CS_SM_CONTROL *smControl,
                                        unsigned       nthElement )
{
    QUEUE_ELEMENT      *queueElement;
    unsigned           returnElement;
    CS_SM_CONNECTOR   *smConnector;
    CS_SM_RET          returnValue;

    queueElement = getNthElement( &smControl->queueControl, nthElement, &returnElement );
    if( queueElement == NULL )
    {
        returnValue =  CS_SM_BAD_SUBSTATE;

    }
    else if( nthElement != returnElement )
    {
        returnValue =  CS_SM_BAD_SUBSTATE;
    }
    else
    {

          smConnector = (CS_SM_CONNECTOR *)getDataPointer( queueElement );
          smConnector->cs_sm_terminate( handle, queue, smConnector->cs_stateMachine );
          dequeueElement( queueElement );
		  rmf_osal_memFreeP(RMF_OSAL_MEM_POD, smConnector);
          returnValue = CS_SM_OK;

    }
    return returnValue;
}


CS_SM_RET      CS_SM_deleteSubStateID( void *handle,
                                       unsigned queue,
                                       CS_SM_CONTROL  *smControl,
                                       unsigned        subStateID )
{
    QUEUE_ELEMENT    *queueElement;
    QUEUE_ELEMENT    *temp;
    CS_SM_CONNECTOR  *smConnector;
    unsigned         loopFlag;

    loopFlag = 1;
    queueElement = getQueueHead( &smControl->queueControl  );
    if( queueElement != NULL )
    {

      while( ( queueElement != NULL ) && (loopFlag != 0 ))
      {

          temp         = getNextElement( queueElement );
          smConnector = (CS_SM_CONNECTOR *)getDataPointer( queueElement );
          if( smConnector->id == subStateID )
          {
           smConnector->cs_sm_terminate( handle, queue, smConnector->cs_stateMachine );
           dequeueElement( queueElement );
           rmf_osal_memFreeP(RMF_OSAL_MEM_POD, smConnector);
           loopFlag = 0;
          }
          else
          {
            queueElement = temp;
          }
      }


    }

    return CS_SM_OK;


}


#ifdef __cplusplus
}
#endif

