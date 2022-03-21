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

#include "poddef.h"  /* To support log file */
#include "lcsm_log.h"                   /* To support log file */
//#include "resource_mgr.h"
#include "utils.h"
//#include "cis.h"
//#include "module.h"
//#include "link.h"
#include "transport.h"
#include "session.h"
#include "ci.h"
#include "podlowapi.h"
#include "podhighapi.h"
//#include "hal_pod_api.h"
#include "applitest.h"
#include <rdk_debug.h>

#ifdef __cplusplus
extern "C" {
#endif

#define  POLLING_PERIOD                100 /* Min Frequency of Poll msgs */

T_TC  e_pstTC [TRANS_NBMAX_TC]; /* Transport Connection information */
UCHAR e_bNbTC;                  /* Number of current Transport Connections */

BOOL bTransportTimeoutEnabled = TRUE; /* Flag Transport timeouts are enabled or disabled */
ULONG  ulTCCreateTimeMs = 0;          /* Time when Transport connection was created. */
ULONG  ulLastTimeReceivedData = 0 ;   /* Last time data was received from POD on the data channel */
ULONG  ulPodPollResponseTime = 0;     /* Last time we get any response from the pod */

extern uint8 transId;
extern int POD_STACK_SEND_TPDU(void * stBufferHandle,  CHANNEL_TYPE_T value);
extern unsigned long RM_Get_Ver( unsigned long id );
extern unsigned long RM_Get_ResourceId( unsigned long id );

/****************************************************************************************
Name:             trans_AddTCSlot
type:            function
Description:     Initialize a new Transport connection ready to be used

In:             Nothing
Out:            Nothing
Return value:    the next selected TCID
*****************************************************************************************/
UCHAR trans_AddTCSlot (void)
{
    /* Initialize new slot for the Transport Connection */
    if ( e_bNbTC >= TRANS_NBMAX_TC )
    {
        MDEBUG (DPM_ERROR, "ERROR: bad TC slot: %d\r\n", e_bNbTC);
        return 0;
    }

    e_pstTC[e_bNbTC].TcId   = e_bNbTC+1;
    e_pstTC[e_bNbTC].bState  = TRANS_STATE_IN_CREATION;
    e_pstTC[e_bNbTC].lSize   = 0L;
    e_pstTC[e_bNbTC].lOffset = 0L;
    e_pstTC[e_bNbTC].pData   = NULL;

    e_pstTC[e_bNbTC].pDataFixed = PODMemAllocate(TRANS_MAXSIZE_TPDU);
    if ( ! e_pstTC[e_bNbTC].pDataFixed )
    {
        MDEBUG (DPM_ERROR, "ERROR: Can't alloc mem, size: %d\r\n", TRANS_MAXSIZE_TPDU);
        return 0;
    }

    PODMemFree(e_pstTC[e_bNbTC].pDataFixed); /* Why free here? */

    e_bNbTC++;

    if ( e_bNbTC == 1 )
    {
        /* When first connection is created then we know we are
        ** initializating the interface so make sure timeouts are
        ** enabled. */
        bTransportTimeoutEnabled = TRUE;
    }

    return (e_bNbTC);
}

/****************************************************************************************
Name:             trans_DeleteTCSlot
type:            function
Description:     Delete a Transport connection

In:             UCHAR    Tcid
Out:            Nothing
Return value:    TRUE if SUCCESS otherwise FALSE
*****************************************************************************************/
BOOL trans_DeleteTCSlot (UCHAR TcId)
{
    if ( TcId >= TRANS_NBMAX_TC )
        return FALSE;

    e_pstTC[TcId].TcId   = 0;
    e_pstTC[TcId].bState  = TRANS_STATE_NOT_USED;

    e_pstTC[e_bNbTC].lSize   = 0L;
    e_pstTC[e_bNbTC].lOffset = 0L;
    e_pstTC[e_bNbTC].pData   = NULL;

    PODMemFree (e_pstTC[TcId-1].pDataFixed);
    if (e_pstTC[TcId-1].pDataFixed)
    {
        MDEBUG (DPM_ERROR, "ERROR: TRANS - can not free memory for TC slot\n");
        return FALSE;
    }

    if ( e_bNbTC > 0 )
    {
    e_bNbTC--;
    }
    else
    {
        MDEBUG (DPM_ERROR, "ERROR: Tried to delete too many TCs\n\r");
        return FALSE;
    }

    return TRUE;
}


/****************************************************************************************
Name:             trans_OnTCClosed
type:            function
Description:     Inform Transport layer that a connection doesn't exist anymore

In:             UCHAR    Tcid
Out:            Nothing
Return value:    void
*****************************************************************************************/
void trans_OnTCClosed (UCHAR TcId)
{
    session_OnTCClosed (TcId);
}



/****************************************************************************************
Name:             trans_ParseInputTPDU
type:            function
Description:     Parses incoming TPDU and fills pstTpdu

In:             UCHAR*    pData        : Pointer on the incoming TPDU
                T_TPDU_IN *pstTpdu    : Pointer on a struct that will be filed
Out:            Nothing
Return value:    TRUE if SUCCESS otherwise FALSE
*****************************************************************************************/
BOOL trans_ParseInputTPDU (UCHAR *pData, T_TPDU_IN *pstTpdu)
{
    UCHAR bSizeLength;
    UCHAR bTCStatus;

    /* TPDU tag */

    pstTpdu->bTag        = pData[0];

    /*for(int i=0; i < (pData[1] +2); i++){
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%x  ",pData[i]);
    }

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","\n");
    */



    if ( pstTpdu->bTag != TRANS_TAG_SB )
    {
        /* TPDU with data */

        if ( (bSizeLength = bGetTLVLength (pData+1, &(pstTpdu->lDataSize))) == 0 )  {
            return FALSE;
        }

        /* The ASN.1 length field includes the TcId mandatory field, thus the real payload
        * size should be decreased by 1 */
        pstTpdu->lDataSize --;

        pstTpdu->TcId = pData[1+bSizeLength];

        pstTpdu->pData = pData+1+bSizeLength+1;

        bTCStatus      = pData[1+bSizeLength+1+pstTpdu->lDataSize+3];

    }
    else
    {
        /* Status TPDU, no data */
        bSizeLength = 0;
        pstTpdu->lDataSize = 0;
        pstTpdu->TcId     = pData[2];
        pstTpdu->pData     = NULL;
        bTCStatus          = pData[3];
    }

    /* Transport connection DA status */
    if ( bTCStatus & BIT7 )
    {
        pstTpdu->bDA = TRUE;

    }
    else
    {
        pstTpdu->bDA = FALSE;
    }

    return TRUE;
}

/****************************************************************************************
Name:             trans_GetTcId
type:            function
Description:     Return the Tcid for this raw TPDU

In:             UCHAR*    pData        : Pointer on the incoming TPDU
Out:            Nothing
Return value:    the TCID associated with the Tpdu
*****************************************************************************************/
UCHAR trans_GetTcId (UCHAR *pTpdu)
{
    UCHAR bLengthFieldSize;

    bLengthFieldSize = bGetTLVLength (pTpdu+1, NULL);

    if ( bLengthFieldSize == 0 )
        return 0;
    else
        return (pTpdu[1+bLengthFieldSize]);
}

/****************************************************************************************
Name:             trans_GetTpduSize
type:            function
Description:     Parses an incoming raw TPDU and return its length

In:             UCHAR*    pData        : Pointer on the incoming TPDU
Out:            Nothing
Return value:    The Size
*****************************************************************************************/
USHORT trans_GetTpduSize (UCHAR *pData)
{
USHORT lPayloadSize;
UCHAR bSizeLength;

  if ( pData[0] == TRANS_TAG_SB )
    return 4;

  bSizeLength = bGetTLVLength (pData+1, &lPayloadSize);

  if ( bSizeLength != 0 )
    return (    1 +             /* 1 byte tag field */
                bSizeLength +   /* Number of bytes used for length */
                lPayloadSize +  /* Length of remaining data */
                4);             /* Not sure what this is! */
  else
    return 0; /* Error */
}


/****************************************************************************************
Name:             trans_CreateTPDU
type:            function
Description:     Allocate a raw TPDU in the POD_BUFFER variable

In:             UCHAR    bTag        : tag
                UCHAR    Tcid        : Tcid
                ULONG lPayloadSize    : Size of the payload for this TPDU
                POD_BUFFER* pBufferHandle: Pointer on the struct
                that will contain infos and the Raw Tpdu


Out:            Nothing
Return value:    TRUE if SUCCESS otherwise FALSE
*****************************************************************************************/
BOOL trans_CreateTPDU (UCHAR bTag, UCHAR TcId, USHORT lPayloadSize,
                          POD_BUFFER *pBufferHandle)
{
    UCHAR i;
    UCHAR pLengthField [CI_MAXSIZE_LENGTH_FIELD];
    UCHAR bLengthFieldSize;

    //Prasad (03/02/2007) - trans_CreateTPDU function implementation is changed to accomodate S-CARD and M-CARD.

    if(vl_gCardType == POD_MODE_MCARD)
    {
        /*** Allocate memory for the SPDU ***/
        pBufferHandle->pOrigin = vlCardManager_PodMemAllocate((USHORT)(lPayloadSize));
        if ( ! pBufferHandle->pOrigin)
        {
            MDEBUG (DPM_ERROR, "ERROR: can not allocate memory\r\n");
            return FALSE;
        }

        if ( lPayloadSize != 0 )
            pBufferHandle->pData = pBufferHandle->pOrigin;

        pBufferHandle->lSize = lPayloadSize;

        return TRUE;
    }

    /*** Create ASN.1 Length field ***/
    if ( (bLengthFieldSize=ci_SetLength (pLengthField, lPayloadSize+1)) == 0 )
        return FALSE;

    /*** Allocate memory for the whole TPDU ***/
    //TPDU tag (1 byte) + length_field(1 or 3 bytes) + t_c_id (1 Byte)  => TPDU header.
    pBufferHandle->pOrigin = vlCardManager_PodMemAllocate((USHORT)(1+bLengthFieldSize+1+lPayloadSize));
    if ( ! pBufferHandle->pOrigin)
    {
        MDEBUG (DPM_ERROR, "ERROR: can not allocate memory\r\n");
        return FALSE;
    }

    /*** Create TPDU header ***/

    /* TPDU tag */
    if ( bTag == 0x00 )
    {
        /* The TPDU tag is undefined (passed by session layer), it should be a
        * "data transmission" tag */
        pBufferHandle->pOrigin[0] = TRANS_TAG_DATA_LAST;
    }
    else
    {
        pBufferHandle->pOrigin[0] = bTag;
    }

    /* length field */
    for (i=0;i<bLengthFieldSize;i++)
        pBufferHandle->pOrigin[1+i] = pLengthField[i];

    /* Transport connection ID */
    pBufferHandle->pOrigin[1+bLengthFieldSize] = TcId;

    /*** Buffer handle information***/
    if ( lPayloadSize != 0 )
        pBufferHandle->pData = pBufferHandle->pOrigin + 1 + bLengthFieldSize + 1;

    pBufferHandle->lSize = 1 + bLengthFieldSize + 1 + lPayloadSize;
    //  pBufferHandle->lRegion = 0;

    return TRUE;
}

/****************************************************************************************
Name:             trans_QueueTPDU
type:            function
Description:     Put stBufferHandle in the main queue

In:             Nothing
Out:            Nothing
Return value:    Allways TRUE
Modified:       2/24/04 - jdm - Change arg from struct to pointer when searching
                                for resolution to non-repeatabile instablity
                3/24/03 - Brendan - commented out sleep loop and PODRelease since queues
                                    are mutex protected
*****************************************************************************************/
BOOL trans_QueueTPDU (POD_BUFFER * stBufferHandle)
{


     POD_STACK_SEND_TPDU((void *)stBufferHandle, DATA_CHANNEL);

    return TRUE;
}

/****************************************************************************************
Name:             trans_CreateTC
type:            function
Description:     Queue the Create_TC request with bNewTcId

In:             bNewTcId
Out:            Nothing
Return value:    Allways TRUE
*****************************************************************************************/
BOOL trans_CreateTC (UCHAR bNewTcId)
{

    POD_BUFFER BufferHandle;

    /* Call this an error just so we get color coding */
 //    MDEBUG (DPM_ERROR, "INFO: Send Create TC #%d request to POD\r\n", bNewTcId);

    if ( trans_CreateTPDU (TRANS_TAG_CREATE_TC, bNewTcId, 0, &BufferHandle) == FALSE )
        return FALSE;

    // set timer here and check it in message loop in POD_TaskModule()
    if( ( ulTCCreateTimeMs = GetTimeSinceBootMs( FALSE ) ) == 0 )
    {
        ulTCCreateTimeMs = 1;
    }

    return (trans_QueueTPDU (&BufferHandle));   // replaced with addr - jdm
}

/****************************************************************************************
Name:             trans_NewTC
type:            function
Description:     Queue the newTC tpdu answering to the Request_TC from CAM

In:             UCHAR    TcIdb
                UCHAR    NewTcId
Out:            Nothing
Return value:    Allways TRUE
*****************************************************************************************/
BOOL trans_NewTC (UCHAR TcId, UCHAR NewTcId)
{
    POD_BUFFER BufferHandle;

    if ( trans_CreateTPDU (TRANS_TAG_NEW_TC, TcId, 1, &BufferHandle) == FALSE )
        return FALSE;

    BufferHandle.pData[0] = NewTcId;
    return (trans_QueueTPDU (&BufferHandle));   // replaced with addr - jdm
}

/****************************************************************************************
Name:             trans_DeleteTCReply
type:            function
Description:     Queue the Delete_TC reply for the Delete_TC request from CAM

In:             TcId
Out:            Nothing
Return value:    TRUE if SUCCESS otherwise FALSE
*****************************************************************************************/
BOOL trans_DeleteTCReply (UCHAR TcId)
{
    POD_BUFFER BufferHandle;

    if ( trans_CreateTPDU (TRANS_TAG_DELETE_TC_REPLY, TcId, 0, &BufferHandle) == FALSE )
        return FALSE;

    return (trans_QueueTPDU (&BufferHandle));   // replaced with addr - jdm
}

/****************************************************************************************
Name:             trans_Receive
type:            function
Description:     Queue the T_Recv TPDU answering to a the "Data Available" bit in
                the polling respones

In:             TcId
Out:            Nothing
Return value:    TRUE if SUCCESS otherwise FALSE
*****************************************************************************************/
BOOL trans_Receive (UCHAR TcId)
{
    POD_BUFFER BufferHandle;

    if ( trans_CreateTPDU (TRANS_TAG_RECEIVE, TcId, 0, &BufferHandle) == FALSE )
        return FALSE;

    return (trans_QueueTPDU (&BufferHandle));   // replaced with addr - jdm
}

/****************************************************************************************
Name:             trans_Receive
type:            function
Description:     Queue the TRANS_TAG_ERROR answering that an error occured

In:             TcId
Out:            Nothing
Return value:    TRUE if SUCCESS otherwise FALSE
*****************************************************************************************/
BOOL trans_Error (UCHAR TcId)
{
    POD_BUFFER BufferHandle;
    if ( trans_CreateTPDU (TRANS_TAG_ERROR, TcId, 0, &BufferHandle) == FALSE )
        return FALSE;

    return (trans_QueueTPDU (&BufferHandle));   // replaced with addr - jdm
}

/****************************************************************************************
Name:             trans_ProcessTPDU
type:            function
Description:     Answer to a TPDU coming from CAM depending on its TAG

In:             T_TC
Out:            Nothing
Return value:    void
*****************************************************************************************/
void trans_ProcessTPDU (T_TC *pstTC)
{
    UCHAR           NewTcId;
    T_TPDU_IN       stTpdu;
    POD_BUFFER stBufferHandle;
    int i;
    ULONG           lResourceId,Resource,ResourceId ;
    UCHAR         *pData;
    UCHAR         status;


    //Prasad (03/02/2007) - implementation is changed to accomodate S-CARD and M-CARD.

    if(vl_gCardType == POD_MODE_MCARD)
    {
            stBufferHandle.pData = pstTC->pData;
            stBufferHandle.lSize = pstTC->lSize;
#if 0
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","trans_ProcessTPDU: Data Length = %d:\n", stBufferHandle.lSize);
            for(i=0;i<stBufferHandle.lSize;i++)
                {
                if (i % 16 == 0)
                {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","\n%li: ", i);
                }

                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%02x ", stBufferHandle.pData[i]);
                }
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","\n");
#endif
#if 0
            pData = (UCHAR*)&stBufferHandle.pData[2];
            //CableCard resouce Id.
            lResourceId = ByteStreamGetLongWord (pData);
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","TRANS_TAG_DATA_LAST:lResourceId:0x%08X \n",lResourceId);

            Resource = (lResourceId &  0xFFFF0000) >> 16;
            //Host resourceId
            ResourceId = RM_Get_ResourceId(Resource);

            if( RM_Get_Ver( ResourceId ) < RM_Get_Ver( lResourceId ) )
                   {
                       status = 0xF2;  // this version < requested
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","WARN:ai:version=0x%x < req=0x%x\n",RM_Get_Ver( ResourceId ),
                    RM_Get_Ver( lResourceId ) );
                AM_OPEN_SS_RSP( status, lResourceId, transId );

                //MDEBUG (DPM_WARN, "WARN:ai:version=0x%x < req=0x%x\n",RM_Get_Ver( ResourceId ), //RM_Get_Ver( lResourceId ) );
            }
#endif
            session_ProcessSPDU (&stBufferHandle, 0);
    }
    else if(vl_gCardType == POD_MODE_SCARD)
    {


    if ( trans_ParseInputTPDU (pstTC->pData, &stTpdu) == FALSE )
    { /* Error */ }

        switch (stTpdu.bTag)
        {
            case TRANS_TAG_REQUEST_TC :
                MDEBUG (DPM_TPDU, "TRANS > Received TPDU REQUEST TC\r\n");
                /* Initialize new slot for the Transport Connection */
                if ( (NewTcId=trans_AddTCSlot ()) == 0 )
                {
                    trans_Error (stTpdu.TcId);
                }

                /* Send New TC response to module */
                trans_NewTC (stTpdu.TcId, NewTcId);

                /* Send Create TC response to module */
                trans_CreateTC (NewTcId);
                break;

            case TRANS_TAG_DELETE_TC :
                MDEBUG (DPM_WARN, "WARN: Not expecting 'Delete TC' from POD \r\n");
                //trans_DeleteTCSlot (stTpdu.TcId);
                trans_DeleteTCReply (stTpdu.TcId);
                break;

            case TRANS_TAG_CREATE_TC_REPLY :
                //MDEBUG (DPM_ALWAYS, "Received 'CREATE TC' reply\r\n");
                pstTC->bState = TRANS_STATE_ACTIVE;
                AM_TC_CREATED(pstTC->TcId);
                ulTCCreateTimeMs = 0;      // cancel timer
                break;

            case TRANS_TAG_DELETE_TC_REPLY :
                MDEBUG (DPM_WARN, "WARN: Not expecting 'Delete TC' reply from POD\r\n");
                trans_DeleteTCSlot (stTpdu.TcId);
                break;

            case TRANS_TAG_DATA_LAST :
                //MDEBUG (DPM_TPDU, "TRANS >  Received SPDU (LAST)\n");

                /* From now, CI frame buffer will be transmitted via a Buffer Handle.
                * A memory space has been allocated in the link layer for this TPDU
                * This memory space may be fixed (512 byte region) or dynamic (any size)
                * This buffer will "travel" trough upper CI layers which may decide to
                * keep (detach) this memory space. Thus, once back in the link layer,
                * memory space may not be free. */
                {
                stBufferHandle.pData = stTpdu.pData;
                stBufferHandle.lSize = stTpdu.lDataSize;

                pData = (UCHAR*)&stBufferHandle.pData[2];
                //CableCard resouce Id.
                lResourceId = ByteStreamGetLongWord (pData);
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","TRANS_TAG_DATA_LAST:lResourceId:0x%08X \n",lResourceId);



                if( transId != stTpdu.TcId )
                    {
                    status = 0xF0;// non-existent resource (on this transId)
                    MDEBUG (DPM_ERROR, "ERROR: transId in OPEN_SS=%d, exp=%d\n", stTpdu.TcId, transId );

                     AM_OPEN_SS_RSP( status, lResourceId, transId );
                    break;
                    }


                if ( pstTC->pData == pstTC->pDataFixed )
                {
                    stBufferHandle.pOrigin = NULL;

                }
                else
                    stBufferHandle.pOrigin = pstTC->pData;


                session_ProcessSPDU (&stBufferHandle, stTpdu.TcId);
                pstTC->pData = stBufferHandle.pOrigin;
                break;
                }

            case TRANS_TAG_DATA_MORE :
                MDEBUG (DPM_TPDU, "TRANS >  Received SPDU (MORE)\r\n");
                break;

            case TRANS_TAG_SB :
                //MDEBUG (DPM_TPDU, "TRANS >  Received response to a polling\r\n");
                break;

            default :
                MDEBUG (DPM_WARN, "WARN: Received unknown TPDU TAG\r\n");
                /* Tag unknown */
                break;
        }

        /* I think that any response is considered a 'poll' response and so resets
        ** our timer. */
        ulPodPollResponseTime = GetTimeSinceBootMs (FALSE);

        /* Send a Receive command if DA bit is set in status SB */
        if ( stTpdu.bDA == TRUE )
        {

            trans_Receive (stTpdu.TcId);
        }
    }
}

BOOL trans_Poll (void)
{
    int i;
    POD_BUFFER      BufferHandle;
    ULONG ulTimeSinceLastDataReceived;
//ADD check for IIR bit here      Hannah
    /*** Send polling TPDU on each active Transport connection ***/

    ulTimeSinceLastDataReceived = ElapsedTimeMs( ulPodPollResponseTime );
      //  if ( ulTimeSinceLastDataReceived >= POLLING_PERIOD ) //Hannah 01/28/05
        {

        ulPodPollResponseTime = GetTimeSinceBootMs (FALSE);
        for (i=0;i<e_bNbTC;i++)
        {
            if ( e_pstTC[i].bState != TRANS_STATE_NOT_USED )
            // if ( e_pstTC[i].bState == TRANS_STATE_ACTIVE )
            // It should be better to poll on the active tcid and not
            // on the tcid that are not "UNUSED"
            {


                if ( trans_CreateTPDU (TRANS_TAG_DATA_LAST, e_pstTC[i].TcId, 0, &BufferHandle) == FALSE )
                    return FALSE;


                return (trans_QueueTPDU (&BufferHandle));
            }
        }

    }

    return TRUE;
}




#if 0 //Hannah Nov 22
/****************************************************************************************
Name:             trans_Communicate
type:            function
Description:     Try to send TPDU to cam if possible and read CAM if not

In:             TPDU data pointer
                Length of TPDU data

Out:            Nothing
Return value:    0 : Write Error
                1 : Data to Read
                2 : Write OK
*****************************************************************************************/
UCHAR trans_Communicate (UCHAR *pbTpdu, USHORT usSize)
{
    return (link_SendTPDU (pbTpdu, usSize));
}

/****************************************************************************************
Name:             trans_Poll
type:            function
Description:     Send polling frame on all the active transport connexion ID

In:             void
Out:            Nothing
Return value:    TRUE if Success otherwise FALSE
*****************************************************************************************/
#define MIN_TPDU_SIZE 3

BOOL trans_Poll (void)
{
    UCHAR        i;
    UCHAR        baTpdu [MIN_TPDU_SIZE];

    baTpdu[0] = TRANS_TAG_DATA_LAST;
    baTpdu[1] = 1; /* Size of the following TCID */

    /* Poll the IIR bit when we are in poll mode.  When set it means
    ** the POD wants us to Initialize the Interface.  Performing this
    ** check here will not catch the IIR bit immediately but it will
    ** ultimately detect it without scattering a lot of IIR detection
    ** code all through the code. */
    if ( f_bModuleDetectIIR () )
    {
        MDEBUG (DPM_WARN, "\n\nWARN: ****** POD Set IIR -- Host now resets Interface!!! ********\n\n");

        /* Maybe IIR is being set by pod because of timeouts.  Showing the last write time
        ** and comparing it to the current time will give us a hint if pod is getting
        ** our data -- and if our pod threads are running */
        MDEBUG (DPM_WARN, "WARN: IIR reset.  Last send data at %d\n", (int)ulLastTimeReceivedData);
        CIResetPod ();
        return SUCCESS;
    }

    /*** Send polling TPDU on each active Transport connection ***/
    for (i=0;i<e_bNbTC;i++)
    {
        if ( e_pstTC[i].bState != TRANS_STATE_NOT_USED )
        // if ( e_pstTC[i].bState == TRANS_STATE_ACTIVE )
        // It should be better to poll on the active tcid and not
        // on the tcid that are not "UNUSED"
        {
            baTpdu[2] = e_pstTC[i].TcId;
            switch ( trans_Communicate ( baTpdu, sizeof (baTpdu) ))
            {
            case 0:
                // Error !: the Polling TPDU can be deleted
                // Big Failure !
                MDEBUG (DPM_ERROR, "ERROR: Bad status from trans_Communicate\r\n");
                return FALSE;
                break;

            case 1:
                // Data to read !
                if ( ! link_GetModuleResponse())
                {
                    MDEBUG (DPM_ERROR, "ERROR: Bad status from link_GetModuleResponse\r\n");
                    return FALSE;
                }
                break;
            case 2:
                break;

            default:
                MDEBUG (DPM_ERROR, "ERROR: Unknown response on polling, big failure\r\n");
                break;
            }
        }
    }


    /*MDEBUG (DPM_TPDU, "OOB Poll : Check for Data available\r\n");*/
    if ( link_ReadExtended() == FAIL )
    {
        MDEBUG (DPM_ERROR, "ERROR: Cannot read extended data channel\r\n");
        return FAIL;
    }

    return TRUE;
}


#endif //Hannah Nov 22

/****************************************************************************************
Name:             trans_Init
type:            function
Description:     Initialize the global Tab of the active TCID and create the first TCID

In:             void
Out:            Nothing
Return value:    void
*****************************************************************************************/
void trans_Init (void)
{
    UCHAR i;

    /* Reset TC slots */
    e_bNbTC = 0;

    for (i=0;i<TRANS_NBMAX_TC;i++)
    {
        e_pstTC[i].TcId   = 0;
        e_pstTC[i].bState  = TRANS_STATE_NOT_USED;
        e_pstTC[i].lSize      = 0L;
        e_pstTC[i].lOffset    = 0L;
        e_pstTC[i].pDataFixed = NULL;
        e_pstTC[i].pData      = NULL;
    }

    /* Add initial slot */
    if( trans_AddTCSlot () == 0){
        MDEBUG (DPM_ERROR, "ERROR: Cannot create a slot, Big Problems appear\r\n");
    }
}


/****************************************************************************************
Name:             trans_Exit
type:            function
Description:     Close and unallocate all the active TCID

In:             void
Out:            Nothing
Return value:    void
*****************************************************************************************/
void trans_Exit (void)
{
    UCHAR i;

    for (i=0;i<TRANS_NBMAX_TC;i++)
    {
        if ( e_pstTC[i].TcId != 0 )
        {
            /* Warn transport layer */
            trans_OnTCClosed (e_pstTC[i].TcId);

            /* Free buffers */
            if ( e_pstTC[i].pDataFixed != NULL )
                PODMemFree(e_pstTC[i].pDataFixed);

            if ( e_pstTC[i].pData != NULL )
                PODMemFree (e_pstTC[i].pData);
        }
    }
    e_bNbTC = 0;
}

#ifdef __cplusplus
}
#endif

