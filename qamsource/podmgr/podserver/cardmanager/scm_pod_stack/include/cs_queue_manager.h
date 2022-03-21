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



#ifndef _QUEUE_MANAGER_H_
#define _QUEUE_MANAGER_H_


#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
   void *DataBuffer;
   void *next;
   void *previous;
   void *controlBlock;
   int  magic;
}QUEUE_ELEMENT;



typedef struct
{
  int queuedNumber;
  QUEUE_ELEMENT *head;
  QUEUE_ELEMENT *tail;
  int  magic;
  void *data;  // pointer to outer container of queue control
}QUEUE_CONTROL;




void createQueueControlBlock( QUEUE_CONTROL *queueControl, void *dataElement );

void createQueueElement( QUEUE_ELEMENT *queueElement , void *dataElement);

int getNumberInQueue( QUEUE_CONTROL *queueControl );

QUEUE_ELEMENT *getQueueHead( QUEUE_CONTROL *queueControl );

QUEUE_ELEMENT *getQueueTail( QUEUE_CONTROL *queueControl );


void insertAhead( QUEUE_ELEMENT *refElement, QUEUE_ELEMENT *newElement );

void insertBehind( QUEUE_ELEMENT *refElement,QUEUE_ELEMENT *newElement );

void dequeueElement( QUEUE_ELEMENT *queueElement );

void queueToHead( QUEUE_CONTROL *queueControl,
                   QUEUE_ELEMENT *queueElement);

void queueToTail( QUEUE_CONTROL *queueControl,
                  QUEUE_ELEMENT *queueElement );

void *getDataPointer( QUEUE_ELEMENT *queueElement );

void *getBackElement( QUEUE_ELEMENT *queueElement );


QUEUE_ELEMENT *getNthElement( QUEUE_CONTROL *queueControl, unsigned desiredElement, unsigned *returnElement );

QUEUE_ELEMENT *getNextElement( QUEUE_ELEMENT *queueElement );

QUEUE_ELEMENT *getPreviousElement( QUEUE_ELEMENT *queueElement );

QUEUE_CONTROL *getControlBlock( QUEUE_ELEMENT *queueElement );

void *getFreeBlock( QUEUE_CONTROL *freeQueue  );
void  returnActiveBlock( QUEUE_CONTROL *freeQueue , 
                         QUEUE_ELEMENT *link  );

void concatenateQueue( QUEUE_CONTROL *refControl, QUEUE_CONTROL *appendedControl );

#ifdef __cplusplus
}
#endif

#endif


