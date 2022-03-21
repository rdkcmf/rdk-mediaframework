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




//#include <pthreadLibrary.h>
#include <stdio.h>
#include <string.h>
#include "poddef.h"
#include <lcsm_log.h>           /* To get LOG_ ... #defines */
#include "utils.h"
//#include "cis.h"
//#include "module.h"
#include "link.h"
#include "transport.h"
#include "session.h"
#include "ci.h"
//#include "i2c.h"
//#include "cimax.h"
#include "podlowapi.h"
#include "podhighapi.h"
#include "appmgr.h"
#include "global_event_map.h"
#include "rdk_debug.h"
#ifdef __cplusplus
extern "C" {
#endif 

#if 0 //hannah Nov 22
ULONG TaskID;
extern    BOOL            bThreadHasToRun;
extern    START_ROUTINE    POD_TaskModule;

pthread_mutex_t    host2pod_lock;    // initialize the mutex lock

#endif


/***************************************************************************************

            H O S T   T O   P O D   F U N C T I O N S 
            Functions called by upper layers

****************************************************************************************/

/****************************************************************************************
Name:             AM_INIT 
type:            function
Description:     Initializes POD Driver

In:             Nothing                
                
Out:            Nothing
Return value:    BOOL : True if Success, otherwise False
*****************************************************************************************/
DWORD AM_INIT()
{

  //  pthread_mutex_init(&host2pod_lock, NULL);

       //f_I2cInit ();

   // REALLYopenCimax ("POD_TaskModule"); /* Must be opened once before any cimax operations */

    //f_CimaxReset (); /* Fully resets and reinitializes the Cimax */
     
    trans_Init();

     session_Init();

   // QueueInit( &TheDriverQueue );

    // Create a task for CI Messages
    //bThreadHasToRun = TRUE;
    
    //if ( 0 == ( TaskID = PODCreateTask(POD_TaskModule) ) ) 
    //{
    //    MDEBUG (DPM_ERROR, "PODCreateTask(POD_TaskModule) ERROR\r\n");
    //}

    /* 
    The following things are not required.
    This is only needed when interrupt management is chosen in implementation.
    The other way is to use the polling on DA (Data available bit in PCMCIA register)
    in extended mode
    */

    /*    Register module's IREQ interrupt handle    */
        

    /*    Enable interruptions for DA */
    
    //return TaskID;
    return 0;
}




/****************************************************************************************
Name:             AM_DELETE_TC
type:            function (called by host)
Description:     Host asks for deleting a TC. A call to AM_TC_DELETED will be made in 
                reply by driver

In:                UCHAR    Tcid
Out:            Nothing
Return value:    void
*****************************************************************************************/
void AM_DELETE_TC( UCHAR Tcid)
{
    POD_BUFFER    stBufferHandle;
    UCHAR        pData[3];

    pData[0] = TRANS_TAG_DELETE_TC;
    pData[1] = 1;
    pData[2] = Tcid;

    stBufferHandle.TcId  = Tcid;
    stBufferHandle.lSize = 3;
    stBufferHandle.pData = pData;

    trans_QueueTPDU (&stBufferHandle);   // replaced with addr - jdm
}
 


/****************************************************************************************
Name:             AM_OPEN_SS_RSP
type:            function (called by host)
Description:     Host Sends the SPDU response

In:             UCHAR    Status : Status for this resource opening
                ULONG    lResourceId :ID of the Resource 
                UCHAR    Tcid
Out:            Nothing
Return value:    void
*****************************************************************************************/
void AM_OPEN_SS_RSP( UCHAR Status, ULONG lResourceId, UCHAR Tcid)
{

    POD_BUFFER stBufferHandle;
    USHORT     usSessionNum = 0; 
    
    if(SESS_STATUS_OPENED == Status)
    {
            usSessionNum = find_available_session ();
    }
    else
    {
        usSessionNum = 0;
    }
    
    /* Build and queue response */
    trans_CreateTPDU (0x00, Tcid , 1+1+1+4+2, &stBufferHandle);
    
    stBufferHandle.pData[0] = SESS_TAG_OPEN_SESSION_RESP;
    stBufferHandle.pData[1] = 0x07;
    stBufferHandle.pData[2] = Status;
    ByteStreamPutLongWord (stBufferHandle.pData+3, lResourceId);
    ByteStreamPutWord     (stBufferHandle.pData+7, usSessionNum);
    if (Status == SESS_STATUS_OPENED)    //    session open was successful.
    {
               /* Now inform the associated resource that this session is
               ** indeed open. */
               
               AM_SS_OPENED(usSessionNum, lResourceId, Tcid);
    }
        
    if ( session_QueueSPDU (&stBufferHandle) )
    {
        
        //if (Status == SESS_STATUS_OPENED)    //    session open was successful.
        //{
            /* Now inform the associated resource that this session is
            ** indeed open. */
            
        //    AM_SS_OPENED(usSessionNum, lResourceId, Tcid);
       // }
    }
    
}

/****************************************************************************************
Name:             AM_APDU_FROM_HOST 
type:            function
Description:     Send an APDU (called by host)

In:             USHORT SessNumber    : Current Session of this resource
                UCHAR * pApdu        : Pointer on the allocated APDU
                USHORT Length        : Allocated Size
Out:            Nothing
Return value:    void
*****************************************************************************************/
void AM_APDU_FROM_HOST(USHORT SessNumber, UCHAR * pApdu, USHORT Length)
{
    POD_BUFFER stBufferHandle;
    USHORT i;
    LCSM_EVENT_INFO eventInfo;
    
    // Create SPDU frame 
    // Find here the session number according to the previous resource opening
    // Find the TCID that must be used for this session
    
    if ( session_CreateSPDU (1, SessNumber,Length, &stBufferHandle) == FAIL )
    {
        MDEBUG (DPM_ERROR, "ERROR: Could not create SPDU\n");
        
         eventInfo.event = POD_ERROR;
            eventInfo.x = POD_SESSION_NOT_FOUND ;
        //eventInfo.y = ;
        eventInfo.dataPtr = NULL;
        eventInfo.dataLength = 0;
         lcsm_send_event( 0, POD_ERROR_QUEUE, &eventInfo );
        return;
    }

    /* Copy APDU in SPDU payload */
    for (i=0;i<Length;i++)
        stBufferHandle.pData[i] = pApdu[i];

    /* Send APDU */
    if ( session_QueueSPDU (&stBufferHandle) == FAIL )
    {
        MDEBUG (DPM_ERROR, "ERROR: Could not queue SPDU\n");
        return;
    }
}

/****************************************************************************************
Name:             AM_CLOSE_SS
type:            function (called by host)
Description:     Host wants to close a session

In:             USHORT SessNumber: Status for this resource opening

Out:            Nothing
Return value:    void
*****************************************************************************************/

void AM_CLOSE_SS(USHORT SessNumber)
{    
    // Check that session is opened
    // Find the Tcid

    UCHAR Tcid = 1;
    POD_BUFFER stBufferHandle;

    /* Build and queue response */
    trans_CreateTPDU (0x00, Tcid , 1+1+2, &stBufferHandle);

    stBufferHandle.pData[0] = SESS_TAG_CLOSE_SESSION_REQ;
    stBufferHandle.pData[1] = 0x02;
    stBufferHandle.pData[2] = (UCHAR)((SessNumber & 0xFF00)>>8);
    stBufferHandle.pData[3] = (UCHAR)((SessNumber & 0x00FF));

    session_QueueSPDU (&stBufferHandle); // replaced with addr - jdm
}



/****************************************************************************************
Name:             AM_EXTENDED_WRITE
type:            function (called by host)
Description:     

In:             

Out:            Nothing
Return value:    void
*****************************************************************************************/
void AM_EXTENDED_WRITE(UCHAR Tcid, ULONG FlowID, UCHAR * pData, USHORT Size)
{    
    POD_BUFFER            Buffer;
    sCIMessage_t        sCIMsg;
    
    Buffer.TcId   = Tcid;
    Buffer.lSize  = Size;
    Buffer.pData  = pData;
    Buffer.FlowID = FlowID;
    
    sCIMsg.eMsgId = MSG_CI_EXTENDED_WRITE;
    
//    QueueAddTail (&TheDriverQueue, &sCIMsg); Add a HAL call for HAL_POD_Send_ExtData

}

 

/****************************************************************************************
Name:             AM_EXIT 
type:            function
Description:     Uninitializes POD Driver

In:             Nothing                
                
Out:            Nothing
Return value:    BOOL : True if Success, otherwise False
*****************************************************************************************/
BOOL AM_EXIT (DWORD TaskID)
{
    //if ( ! PODDeleteTask(TaskID,&bThreadHasToRun))
    //    return FALSE;
//    send a message to HAL-POD for the threads to exit //Hannah

    trans_Exit ();
    return TRUE;
}

/*
 *    Purpose: to send out an APDU from the application to the POD.
 *    AM_APDU_TO_POD is the similar to the original AM_APDU_FROM_HOST,
 *    provided by SCM in the POD Stack. The only difference is that this
 *    function provides semaphore lock to the actual queuing of the 
 *    outgoing buffer.
 *    Return: 
 *        0:successful, -1:failed, couldn't get the lock (should be retried)
 */
#if 0
int AM_APDU_TO_POD(USHORT SessNumber, UCHAR * pApdu, USHORT Length)
{
    int retval = -1;
    
    if (pthread_mutex_trylock(&host2pod_lock) == 0)        // got the lock, don't block here
    {
        AM_APDU_FROM_HOST(SessNumber, pApdu, Length);    // queue in the packet for transmission
        pthread_mutex_unlock(&host2pod_lock);        // remove the lock
        retval = 0;
    }
    
    return retval;
}
#endif

int AM_APDU_TO_POD(USHORT SessNumber, UCHAR * pApdu, USHORT Length)
{
    AM_APDU_FROM_HOST(SessNumber, pApdu, Length);    // queue in the packet for transmission
    return 0;
}
 

#ifdef __cplusplus
}
#endif 




