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



#ifndef __INTERFACE_H
#define __INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif 


 
typedef enum {
    EVENT_RECEIVED,
    TIME_OUT_EXPIRED,
    WAIT_ERROR
}WAIT_RETURN_CODE;



 
/***************************  CI Messages *********************************/

typedef struct ListElement{
    PVOID            Data;
    PVOID            pNext;
}ListElement, *pListElement;

typedef enum {
    MSG_CI_POLL,            /* Poll the pod for data */
    MSG_CI_SEND_TPDU,       /* Send an application TPDU to the pod */
    MSG_CI_READ_TPDU,       /* From Interrupt handler -- Data Available to read */
    MSG_CI_EXTENDED_READ,   /* From Interrupt handler -- Extended Data Available to read */
    MSG_CI_EXTENDED_WRITE,
    MSG_CI_RESET_PCMCIA,    /* Necessary resets for homing only */
    MSG_CI_RESET_POD_INTERFACE,       /* Necessary resets for homing only */
    MSG_CI_TRANS_BYPASS_CTRL,   /* TS bypasses the pod or is routed through it */
    
    /* The following messages are meant for debug only */
    MSG_CI_DEBUG_FLAG_POD_CHECK, /* Enable / Disable pod insert checking */
    MSG_CI_DEBUG_CIMAX_DUMP,
    MSG_CI_DEBUG_CIMAX_READ,
    MSG_CI_DEBUG_CIMAX_WRITE,
    MSG_CI_MAX_MSGS
}eMessageId_t;


typedef struct QUEUE
{
    UCHAR    NumberOfElement ;
    pListElement pFirst ;
    pListElement pLast ;
} QUEUE , *PQUEUE;

extern    QUEUE            TheDriverQueue;

typedef struct
{
    union /* union member based on which MsgId is being sent */
    {
        BOOL bEnableTSBypass;      /* Bypass the POD module -- just loop back the data via the Cimax */
    }u; /* This is a union */
}sCtrlMsg_t; 

typedef struct
{
    ULONG x;
    ULONG y;
    ULONG z;
} sGenericMsg_t;

typedef struct
{
    eMessageId_t eMsgId;
    
    union /* union member based on which MsgId is being sent */
    {
        POD_BUFFER      sPodPkt;    /* Active Union member based on MsgId */
        sCtrlMsg_t      sControlMsg;/* Active Union member based on MsgId */
        sGenericMsg_t   sGenericMsg; /* Used when lazy */
    }u; /* This is a union */
} sCIMessage_t;


BOOL            QueueRemoveHead (QUEUE* pQueue, sCIMessage_t *pElt);
BOOL            QueueAddTail    (QUEUE* pQueue, sCIMessage_t* pElt);
BOOL            QueueAddHead    (QUEUE* pQueue, sCIMessage_t* pElt);
void            QueueInit       (QUEUE* pQueue);
void            QueueDelete     (QUEUE* pQueue);
pListElement    QueueGetHead    (QUEUE* pQueue); 


/*************************** End CI Messages *********************************/




void PODMemFree(UCHAR * pU);
UCHAR *PODMemAllocate(USHORT size);
extern "C" void vlCardManager_PodMemFree(UCHAR * pU);
extern "C" UCHAR *vlCardManager_PodMemAllocate(USHORT size);

void PODMemCopy( PUCHAR Destination, PUCHAR Source, USHORT Length);
void PODSleep(USHORT TimeMs);

void PODFree(UCHAR* pDest);
void PODCheck();

#ifdef __cplusplus
}
#endif 

#endif

