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



#include "stdio.h"
#include "cs_queue_manager.h"
#include <rdk_debug.h>
#ifdef __cplusplus
extern "C" {
#endif


#define QUEUE_CONTROL_MAGIC 0xaaaa
#define QUEUE_ELEMENT_MAGIC  0x5555

static void verifyQueueControl( QUEUE_CONTROL *queueControl );
static void verifyQueueElement( QUEUE_ELEMENT *queueElement );



void createQueueControlBlock( QUEUE_CONTROL *queueControl, void *dataElement )
{
   queueControl->head          = NULL;
   queueControl->tail          = NULL;
   queueControl->queuedNumber  = 0;
   queueControl->magic         = QUEUE_CONTROL_MAGIC;
   queueControl->data          = dataElement;


}

void createQueueElement( QUEUE_ELEMENT *queueElement , void *dataElement)
{
   queueElement->controlBlock = NULL;
   queueElement->DataBuffer = dataElement;
   queueElement->next = NULL;
   queueElement->previous = NULL;
   queueElement->magic = QUEUE_ELEMENT_MAGIC;


}

int getNumberInQueue( QUEUE_CONTROL *queueControl )
{
   int returnValue;
   verifyQueueControl( queueControl );
   returnValue = queueControl->queuedNumber;
   return returnValue;

}

QUEUE_ELEMENT *getQueueHead( QUEUE_CONTROL *queueControl )
{
    QUEUE_ELEMENT *returnValue;
    verifyQueueControl( queueControl );
    returnValue = queueControl->head;
    return returnValue;
}

QUEUE_ELEMENT *getQueueTail( QUEUE_CONTROL *queueControl )
{
   QUEUE_ELEMENT *returnValue;
   verifyQueueControl( queueControl );
   returnValue = queueControl->tail;
   return returnValue;
}

void dequeueElement( QUEUE_ELEMENT *queueElement )
{
   QUEUE_CONTROL *temp;
   QUEUE_ELEMENT *temp1;
   QUEUE_ELEMENT *temp2;

   temp = ( QUEUE_CONTROL *)queueElement->controlBlock;
   if( temp == NULL )
   {
       return;
   }
   verifyQueueElement( queueElement );
   verifyQueueControl( temp );
   temp1 = (QUEUE_ELEMENT *)queueElement->next;
   temp2 = (QUEUE_ELEMENT *)queueElement->previous;
   if( temp2 == NULL )
   {
       temp->head = temp1;
       if( temp1 != NULL )
       {
         temp1->previous = NULL;
       }
   }
   else
   {
       temp2->next = temp1;
   }
   if( temp1 == NULL )
   {
       temp->tail = temp2;
       if( temp2 != NULL )
       {
         temp2->next = NULL;
       }
   }
   else
   {
       temp1->previous = temp2;
   }

   temp->queuedNumber -= 1;
   queueElement->next = NULL;
   queueElement->previous = NULL;

}

void queueToTail( QUEUE_CONTROL *queueControl,
                  QUEUE_ELEMENT *queueElement)
{
   QUEUE_ELEMENT *temp;
   verifyQueueElement( queueElement );
   verifyQueueControl( queueControl );
   // make sure block is free
   if( ( queueElement->next != NULL ) || (queueElement->previous != NULL ))
   {
       return;

   }
   queueElement->controlBlock = queueControl;
   temp = queueControl->tail;
   queueControl->tail = queueElement;
   if( temp != NULL )
   {
       queueElement->next = NULL;
       queueElement->previous = temp;
       temp->next = queueElement;
   }
   else
   {
      queueControl->head = queueElement;
      queueElement->next = NULL;
      queueElement->previous = NULL;
   }
   queueControl->queuedNumber += 1;

}

void queueToHead( QUEUE_CONTROL *queueControl,
                  QUEUE_ELEMENT *queueElement )
{
   QUEUE_ELEMENT *temp;
   verifyQueueElement( queueElement );
   verifyQueueControl( queueControl );
   // make sure block is free
   if( ( queueElement->next != NULL ) || (queueElement->previous != NULL ))
   {
       return;

   }

   queueElement->controlBlock = queueControl;
   temp = queueControl->head;
   queueControl->head = queueElement;
   if( temp != NULL )
   {
       queueElement->previous = NULL;
       queueElement->next = temp;
       temp->previous = queueElement;
   }
   else
   {
      queueControl->tail = queueElement;
      queueElement->next = NULL;
      queueElement->previous = NULL;
   }
   queueControl->queuedNumber += 1;

}

void *getDataPointer( QUEUE_ELEMENT *queueElement )
{

   if( queueElement != NULL )
   {
       verifyQueueElement( queueElement );
       return queueElement->DataBuffer;
   }


   return NULL;

}

void *getBackElement( QUEUE_ELEMENT *queueElement )
{
   QUEUE_CONTROL *queueControl;
   void          *returnValue;

   returnValue = NULL;
   if( queueElement != NULL )
   {
       verifyQueueElement( queueElement );
       if( queueElement->controlBlock != NULL )
       {
           queueControl = (QUEUE_CONTROL *)queueElement->controlBlock;
           verifyQueueControl(queueControl );
           if(queueControl != NULL )
           {
               returnValue = queueControl->data;
           }
       }
   }


   return returnValue;

}



QUEUE_ELEMENT *getNextElement( QUEUE_ELEMENT *queueElement )
{
    QUEUE_ELEMENT *returnValue;

    if( queueElement != NULL )
    {
        returnValue = (QUEUE_ELEMENT *)queueElement->next;
    }
    else
    {
        returnValue = NULL;
    }
    //return (QUEUE_ELEMENT *)queueElement->next;
    return returnValue;
}

QUEUE_ELEMENT *getPreviousElement( QUEUE_ELEMENT *queueElement )
{
    QUEUE_ELEMENT *returnValue;

    if( queueElement != NULL )
    {
        returnValue = (QUEUE_ELEMENT *)queueElement->previous;
    }
    else
    {
        returnValue = NULL;
    }
    return returnValue;
}

QUEUE_CONTROL *getControlBlock( QUEUE_ELEMENT *queueElement )
{
    return (QUEUE_CONTROL  *)queueElement->controlBlock;
}

static void verifyQueueControl( QUEUE_CONTROL *queueControl )
{
  if( queueControl->magic != QUEUE_CONTROL_MAGIC )
  {
      RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","ERROR: Homing Queue Control Block is not valid \n");
  }
}

static void verifyQueueElement( QUEUE_ELEMENT *queueElement )
{
  if( queueElement->magic != QUEUE_ELEMENT_MAGIC )
  {
      RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","ERROR: Homing Queue Element Block is not valid \n");
  }
}

void insertBehind( QUEUE_ELEMENT *refElement, QUEUE_ELEMENT *newElement )
{

    QUEUE_ELEMENT *previous;
    QUEUE_CONTROL *queueControl;


    queueControl = (QUEUE_CONTROL *)refElement->controlBlock;


    newElement->controlBlock = refElement->controlBlock;
    newElement->next = refElement;
    previous = (QUEUE_ELEMENT *)refElement->previous;
    refElement->previous = newElement;
    if( previous != NULL )
    {
        previous->next       = newElement;
        refElement->previous = previous;
    }
    else
    {
        newElement->previous = NULL;
        queueControl->head = newElement;
    }
    queueControl->queuedNumber +=1;


}

void insertAhead( QUEUE_ELEMENT *refElement,QUEUE_ELEMENT *newElement )
{

    QUEUE_ELEMENT *next;
    QUEUE_CONTROL *queueControl;


    queueControl = (QUEUE_CONTROL *)refElement->controlBlock;

    newElement->controlBlock = refElement->controlBlock;
    newElement->previous = refElement;
    next = (QUEUE_ELEMENT *)refElement->next;
    refElement->next = newElement;
    if( next != NULL )
    {
        next->previous = newElement;
        newElement->next = next;
    }
    else
    {
        newElement->next = NULL;
        queueControl->tail = newElement;
    }
    queueControl->queuedNumber +=1;

}

QUEUE_ELEMENT *getNthElement( QUEUE_CONTROL *queueControl, unsigned desiredElement, unsigned *returnElement )
{

     QUEUE_ELEMENT *returnValue;


     *returnElement = 0;

     returnValue  =  getQueueHead( queueControl );

     while( ( returnValue != NULL ) && ( *returnElement < desiredElement ) )
     {
         returnValue = getNextElement( returnValue );
         *returnElement +=1;
     }
     return returnValue;

}

void *getFreeBlock( QUEUE_CONTROL *freeQueue  )
{
    void          *returnValue;
    QUEUE_ELEMENT *temp;

    temp = getQueueHead( freeQueue );
    if( temp == NULL )
    {
        returnValue = NULL;
    }
    else
    {
        dequeueElement( temp );
        returnValue = getDataPointer( temp );
    }
    return returnValue;

}




void  returnActiveBlock( QUEUE_CONTROL *freeQueue ,
                         QUEUE_ELEMENT *link  )
{


    if( link == NULL )
    {
        ; // do nothing
    }
    else
    {
        dequeueElement( link );
        queueToTail( freeQueue, link );
    }

}


void concatenateQueue( QUEUE_CONTROL *refControl, QUEUE_CONTROL *appendedControl )
{
   QUEUE_ELEMENT *temp1;
   verifyQueueControl( refControl );
   verifyQueueControl( appendedControl );

   temp1 = getQueueHead( appendedControl );
   while( temp1 != NULL )
   {
       dequeueElement( temp1 );
       queueToTail( refControl, temp1 );
       temp1 = getQueueHead(appendedControl );

   }

}

#ifdef __cplusplus
}
#endif

