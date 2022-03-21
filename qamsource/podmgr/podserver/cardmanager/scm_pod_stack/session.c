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




#include <stdio.h>

#include "poddef.h"    /* To support log file */
#include "lcsm_log.h"                   /* To support log file */

#include "utils.h"
//#include "cis.h"
//#include "module.h"
//#include "link.h"
#include "transport.h"
#include "session.h"
#include "ci.h"
//#include "podlowapi.h"
#include "podhighapi.h"

#include "appmgr.h"
#include <rdk_debug.h>
#ifdef __cplusplus
extern "C" {
#endif 


/****************************************************************************************
Name:             session_OnTCClosed
type:            function 
Description:     Inform Host that a Tcid heas been deleted

In:             UCHAR    Tcid        : Tcid

Out:            Nothing
Return value:    void
*****************************************************************************************/
void session_OnTCClosed (UCHAR TcId)
{
    MDEBUG (DPM_SS, "A TC has been deleted, Process to Host \r\n");
    AM_TC_DELETED(TcId);
}
/****************************************************************************************
Name:             session_QueueSPDU
type:            function 
Description:     Give the down coming SPDU to the Transport Layer

In:             POD_BUFFER stBufferHandle: Struct where infos 
                about the raw SPDU and TPDU are stored
                BOOLEAN Unused

Out:            Nothing
Return value:    allways TRUE 
Modified:       2/24/04 - jdm - Change arg from struct to pointer when searching
                                for resolution to non-repeatabile instablity
*****************************************************************************************/
BOOL session_QueueSPDU (POD_BUFFER * stBufferHandle)
{
  return (trans_QueueTPDU (stBufferHandle));   // replaced with addr - jdm
}



/****************************************************************************************
Name:             session_CreateSPDU
type:            function 
Description:     Format a SPDU regarding the downcoming infos

In:             
                UCHAR TcId: Tcid
                USHORT wSessionNb:
                ULONG lPayloadSize,
                POD_BUFFER stBufferHandle: Struct where infos and pointer to 
                the raw SPDU are stored 

Out:            Nothing
Return value:    TRUE if success otherwise FALSE
*****************************************************************************************/
BOOL session_CreateSPDU (UCHAR TcId, USHORT wSessionNb, USHORT lPayloadSize,
                            POD_BUFFER *pBufferHandle)
{

    //Prasad (03/02/2007) - trans_CreateTPDU function implementation is changed to accomodate S-CARD and M-CARD.
    //Though TPDU is not required for M-CARD, the code change is kept in tranport.c instead of changing the code
    //in many places (to reduce redunancy).
 
     /* Allocate and create TPDU buffer for the "Session Number" SPDU */
    if ( trans_CreateTPDU (0x00, TcId,(USHORT)(1+1+2+lPayloadSize), pBufferHandle) == FALSE )
        return FALSE;

    /* Now pBufferHandle->pData points to SPDU (TPDU payload) */

    /* SPDU Session Number header : TAG */
    pBufferHandle->pData[0] = SESS_TAG_SESSION_NUMBER;

    /* SPDU Session Number header : Length field is "2" */
    pBufferHandle->pData[1] = 0x02;

    /* SPDU Session Number header : Session number */
    pBufferHandle->pData[2]   = (UCHAR) (wSessionNb >> 8);
    pBufferHandle->pData[3]   = (UCHAR) (wSessionNb & 0xFF);

    /* Make payload point to APDU (SPDU payload)  */
    pBufferHandle->pData += 4 ;

    return TRUE;
}



/****************************************************************************************
Name:             session_HandleCloseReq
type:            function 
Description:     Send response to POD after a Close session request 
                and transmits the Close Session request from POD to upper layers

In:             UCHAR TcId: Tcid
                UCHAR *pSPDU : The Raw SPDU 

Out:            Nothing
Return value:    TRUE if success otherwise FALSE
*****************************************************************************************/
BOOL session_HandleCloseReq (UCHAR *pSPDU, UCHAR Tcid)
{
    USHORT wSessionNb;
    POD_BUFFER stBufferHandle;

    // Extract Session Number 
    wSessionNb = ByteStreamGetWord (pSPDU+2);

    /* Build and queue response */
    if ( trans_CreateTPDU (0x00, Tcid, 1+1+1+2, &stBufferHandle) == FALSE ){
        return FALSE;
    }

    stBufferHandle.pData[0] = SESS_TAG_CLOSE_SESSION_RESP;
    stBufferHandle.pData[1] = 0x03;
    stBufferHandle.pData[2] = SESS_STATUS_CLOSED;
    ByteStreamPutWord     (stBufferHandle.pData+3, wSessionNb);

    if ( session_QueueSPDU (&stBufferHandle) == FALSE ){
        MDEBUG (DPM_ERROR, "ERROR: Can't send 'Close' session \r\n");
    }


    MDEBUG (DPM_WARN, "WARN: Resource Manager of session closing \n");

    AM_SS_CLOSED(wSessionNb);
    return TRUE;
}


/****************************************************************************************
Name:             session_HandleOpenReq
type:            function 
Description:     Transmits the Open session request to upper layers

In:             UCHAR TcId: Tcid
                UCHAR *pSPDU : The Raw SPDU 

Out:            Nothing
Return value:    Allways TRUE
*****************************************************************************************/
BOOL session_HandleOpenReq (UCHAR *pSPDU, UCHAR TcId)
{
    ULONG           lResourceId;

    // Extract Resource Id 
    lResourceId = ByteStreamGetLongWord (pSPDU+2);
    AM_OPEN_SS_REQ(lResourceId,TcId);
    return TRUE;
}




/****************************************************************************************
Name:             session_ProcessSPDU
type:            function 
Description:     Dispatch the upcoming SPDU from CAM to upper layers

In:             UCHAR TcId: Tcid
                POD_BUFFER *pstBufferHandle : Where infos  that must 
                be parsed are stored 

Out:            Nothing
Return value:    void
*****************************************************************************************/
void session_ProcessSPDU (POD_BUFFER *pstBufferHandle, UCHAR TcId)
{
    USHORT wSessionNb;
    UCHAR  bLengthFieldSize;
    UCHAR *pSPDU = pstBufferHandle->pData;
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","session_ProcessSPDU: Entered \n");
    switch (pSPDU[0])
    {
    case SESS_TAG_OPEN_SESSION_REQ :
        MDEBUG (DPM_SS, "SESSION > Received SPDU OPEN SESSION REQUEST, process to host \r\n");
        session_HandleOpenReq (pSPDU, TcId);
        break;

    case SESS_TAG_CREATE_SESSION_RESP :
        MDEBUG (DPM_SS, "SESSION > Received SPDU CREATE SESSION RESPONSE\r\n");
        break;

    case SESS_TAG_CLOSE_SESSION_REQ :
        MDEBUG (DPM_SS, "SESSION > Received SPDU CLOSE SESSION REQUEST\r\n");
        session_HandleCloseReq (pSPDU, TcId);
        break;

    case SESS_TAG_CLOSE_SESSION_RESP :
        MDEBUG (DPM_SS, "SESSION > Received SPDU CLOSE SESSION RESPONSE\r\n");
        bLengthFieldSize = bGetTLVLength (pSPDU+1, NULL);
        wSessionNb = ByteStreamGetWord (pSPDU+1+bLengthFieldSize);
        AM_SS_CLOSED(wSessionNb);
        break;

    case SESS_TAG_SESSION_NUMBER :
        bLengthFieldSize = bGetTLVLength (pSPDU+1, NULL);
        wSessionNb = ByteStreamGetWord (pSPDU+1+bLengthFieldSize);
        pstBufferHandle->pData = pSPDU+1+bLengthFieldSize+2;
        pstBufferHandle->lSize   -= 1+bLengthFieldSize+2;
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","session_ProcessSPDU: SESS_TAG_SESSION_NUMBER. wSessionNb = %d \n", wSessionNb);
        AM_APDU_FROM_POD(wSessionNb, pstBufferHandle->pData, pstBufferHandle->lSize);
        break;

    default :
        break;
    }
}




/****************************************************************************************
Name:             session_Init
type:            function 
Description:     Build the tab used to store session number

In:             Nothing

Out:            Nothing
Return value:    Nothing
*****************************************************************************************/
void session_Init (void)
{

    // clear the session entry storage (keeping track of: sessnum <-> opened resource)
    init_session_entry ();    
}

#ifdef __cplusplus
}
#endif 

