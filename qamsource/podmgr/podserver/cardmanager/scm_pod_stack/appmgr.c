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


#include <string.h>
#include <stdio.h>
#include <global_event_map.h>
#include <global_queues.h>
/* Add these two for debug logging */
#include "poddef.h"
#include <lcsm_log.h>           /* To get LOG_ ... #defines */

#include "utils.h"
//#include "module.h"
//#include "link.h"
#include "transport.h"
#include "session.h"
//#include "ci.h"
//#include "podlowapi.h"
#include "podhighapi.h"
#include "appmgr.h"
#include "applitest.h"
#include "rsmgr/rsmgr-main.h"   // RM_Get_Queue
#include <string.h>
#include <rdk_debug.h>
#include <rmf_osal_mem.h>
#include "rmf_osal_sync.h"

#ifdef __cplusplus
extern "C" {
#endif

/**********************  Session Numbers *************************/

/* Store session number status as an array.  Each index into the
** array is the 'session number'.  Then for each session we keep
** track of which session numbers are 'in use' and, if in use, 
** then what is the associated Resource ID. */

#define NB_MAX_SESSION        256 /* As per Table 8.1-B in SCTE 28 */

struct 
{
    BOOL    bfInUse;    /* Indicates if this session is in use. */
    BOOL    bfInRequest;    /* Indicates if this session is in use. */
    ULONG   ulRID;      /* Resource ID corresponding to each session */
} static asSessionStatus [NB_MAX_SESSION];

#define FIRST_SESSION_NUMBER    1   /* First valid session numbers */
#define INVALID_SESSION_NUMBER  0   /* Zero is an invalid session number */
#define INVALID_RESOURCE_ID     0   /* Zero is an invalid resource ID */


//
static rmf_osal_Mutex vlg_SessionEntryMutex;

// fwd reference
static BOOL  add_session_entry (USHORT usSessionNum, ULONG ulRID);
static ULONG delete_session_entry (USHORT usSessionNum);

/*****************************************************************/

static int ParseAPDU (USHORT session, UCHAR *apkt, USHORT len);

/*****************************************************************/

/* Turn on/off APDU debugging */
#undef DEBUG_SHOW_APDU

void ShowAPDU (int mode, UCHAR *apdu, USHORT len)
{
#ifdef DEBUG_SHOW_APDU
    int i;
    

    if (mode==0)        // inbound
        MDEBUG (DPM_APDU, "\napdu(in)=");
    else if (mode==1)    // outbound
        MDEBUG (DPM_APDU, "\napdu(out)=");
        
    for (i=0;i<len;i++)
    {
        MDEBUG2 (DPM_APDU, "%02X ", *apdu++);
    }
    MDEBUG2 (DPM_APDU, "\n");
#endif
    
}

/*
 *    AM_APDU_FROM_POD 
 *    Callback (Called by Session)
 *  Host reception of an APDU packet
*/
void AM_APDU_FROM_POD(USHORT SessNumber, UCHAR * pApdu, USHORT Length)
{
    int ii;
    ShowAPDU (0, pApdu, Length);    // 0=inbound
/*
    printf("\nIncoming data: SessNumber:%d \n",SessNumber);
    for(ii = 0; (ii < Length) && ( Length < 128); ii++)
    {
        printf("%02X ",pApdu[ii]);
        if( ((ii+1)% 16) == 0 )
          printf("\n");
    }
    printf("\n");*/
    ParseAPDU (SessNumber, pApdu, Length);
}

/*******************************************************************/

/* Static circular buffers used to store APDUs, (coming from the POD),
** to guarantee their persistance over lcsm_send_event calls until 
** the APDU can be processed by a application.
**
** WARNING: 
**  1)  These circular buffers have no mechanism to indicate 
**      if data has been overwritten without being processed.
**  2)  The number and size of buffers have been chosen 'arbitrarily'
**      and do not necessarily reflect the optimum values.
*/
#define    APDUIN_MAXBUF_LARGE     4    
#define APDUIN_LARGE_BUF_SIZE   65536
UCHAR apduinLarge  [APDUIN_MAXBUF_LARGE]  [APDUIN_LARGE_BUF_SIZE];    
                                                
#define    APDUIN_MAXBUF_MIDDLE    21    
#define APDUIN_MIDDLE_BUF_SIZE  4096
UCHAR apduinMiddle [APDUIN_MAXBUF_MIDDLE] [APDUIN_MIDDLE_BUF_SIZE];    

/* Note that MOST packets are LESS then 128 bytes so if we wanted to save 
** memory we could reduce the size of these buffers. */
#define    APDUIN_MAXBUF_SMALL     44    
#define APDUIN_SMALL_BUF_SIZE   1024
UCHAR apduinSmall  [APDUIN_MAXBUF_SMALL]  [APDUIN_SMALL_BUF_SIZE];    

/*
 *    This function parses the received APDU packet in apkt.
 *    It uses the session number to determine the resource to send the packet to.
 */
static int ParseAPDU (USHORT usSession, UCHAR *apkt, USHORT len)
{
    static int largeIndex  = 0;
    static int middleIndex = 0;
    static int smallIndex  = 0;
    int32      i32ResourceQueueId;
    int        i;
    
    STATUS_T   bStatus = FAIL;

    LCSM_EVENT_INFO eventInfo;
    
    if(usSession >= NB_MAX_SESSION)
    {
	RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","%s: Got APDU for unknown session # %d\n", __FUNCTION__, usSession);
        return bStatus;
    }
    
    //Commented and modified the code a little bit and access the asSessionStatus with in mutex.....
#if 0
    if ( (usSession != INVALID_SESSION_NUMBER) && 
         (usSession < NB_MAX_SESSION)          &&
         (asSessionStatus [usSession].bfInUse)     )
    {
        i32ResourceQueueId = RM_Get_Queue ( asSessionStatus [usSession].ulRID);
        
        if ( i32ResourceQueueId != INVALID_RESOURCE_ID )
        {

            eventInfo.event      = POD_APDU;
            eventInfo.x          = usSession;   // x contains the session number
            eventInfo.z          = len;         // z contains byte count of the apdu packet 
            eventInfo.dataPtr    = NULL;        // no copying of dataPtr
            eventInfo.dataLength = 0;           // no copying of dataPtr

            /* Store APDU in static circular buffer based on the size of the APDU. 
            ** Then other treads can access this buffer at their leisure. */
            if (len > APDUIN_MIDDLE_BUF_SIZE )  //use the large size buffers
            {
                
                MDEBUG (DPM_TPDU, "Parsed %02x%02x%02x APDU [LARGE] (len:%d, session:%d, idx:%d)\n", 
                                    len, usSession, largeIndex, apkt[0], apkt[1], apkt[2]);
              //  memcpy ((char *)apduinLarge [largeIndex], apkt, len);    // this apduin[] has to persist over the lcsm_send_event
          memcpy ((char *)apduinLarge [largeIndex], apkt, len);    // this apduin[] has to persist over the lcsm_send_event
                eventInfo.y = (long)&apduinLarge [largeIndex][0];        // y contains pointer to the actual apdu

                /* Point to next circular buffer for next APDU */
                if (++largeIndex >= APDUIN_MAXBUF_LARGE)
                {
                    largeIndex = 0;
                }
                
            }
            else if (len > APDUIN_SMALL_BUF_SIZE)
            {
                MDEBUG (DPM_TPDU, "Parsed %02x%02x%02x APDU [MEDIUM] (len:%d, session:%d, idx:%d)\n", 
                                    len, usSession, middleIndex, apkt[0], apkt[1], apkt[2]);
                                    
               // memcpy ((char *)apduinMiddle[middleIndex], apkt, len);  // this apduin[] has to persist over the lcsm_send_event
         memcpy ((char *)apduinMiddle[middleIndex], apkt, len);  // this apduin[] has to persist over the lcsm_send_event
                eventInfo.y = (long)&apduinMiddle[middleIndex][0];      // y contains pointer to the actual apdu

                /* Point to next circular buffer for next APDU */
                if (++middleIndex >= APDUIN_MAXBUF_MIDDLE)
                    middleIndex = 0;
            }
            else
            {
                MDEBUG (DPM_TPDU, "Parsed %02x%02x%02x APDU [SMALL] (len:%d, session:%d, idx:%d)\n", 
                                    apkt[0], apkt[1], apkt[2], len, usSession, smallIndex);
                                    
                RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD"," Parsed %02x%02x%02x APDU [SMALL] (len:%d, session:%d, idx:%d)\n", 
                                    apkt[0], apkt[1], apkt[2], len, usSession, smallIndex);
                                    
               // memcpy ((char *)apduinSmall[smallIndex], apkt, len);    // this apduin[] has to persist over the lcsm_send_event
     memcpy ((char *)apduinSmall[smallIndex], apkt, len);    // this apduin[] has to persist over the lcsm_send_event
                eventInfo.y = (long)&apduinSmall[smallIndex][0];        // y contains pointer to the actual apdu

                /* Point to next circular buffer for next APDU */
                if (++smallIndex >= APDUIN_MAXBUF_SMALL)
                    smallIndex = 0;
            }
       RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," Calling lcsm_send_event\n");
            lcsm_send_event (hLCSMdev_pod, i32ResourceQueueId, &eventInfo);    // wake up the resource/application thread
             
        //    if ( eventInfo.returnStatus != 0 )
        //    {
         //       MDEBUG (DPM_ERROR, "ERROR:ParseAPDU Large: Bad Send: %d\n", eventInfo.returnStatus );
        //    }
        //    else
            {
                MDEBUG (DPM_APPLI, "Sending session %d APDU to resource queue %ld\n", usSession, i32ResourceQueueId);
                bStatus = SUCCESS;
            }
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "Bad RID (%ld) returned from RM_GetQueue for session # %d\n", 
                        i32ResourceQueueId, usSession);
        }
    }
    else
    {
        if ( asSessionStatus [usSession].bfInUse == FALSE )
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "Got APDU for session # %d not in use.\n", usSession);
            for (i=0;i<len;i++)
            {
                MDEBUG2 (DPM_ERROR, "%02X ", apkt [i]);
            }
              MDEBUG2 (DPM_ERROR, "\n");
            
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","Got APDU for invalid session # %d\n", usSession);
        }
    }
#endif
    
    if(INVALID_SESSION_NUMBER == usSession)
    {
      RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","%s: Got APDU for invalid session # %d\n", __FUNCTION__, usSession);
      return bStatus;
    }
    
    //
    rmf_osal_mutexAcquire(vlg_SessionEntryMutex);
    
    if(FALSE == asSessionStatus [usSession].bfInUse)
    {
	RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s: Got APDU for session # %d not in use.\n", __FUNCTION__, usSession);
	for (i=0;i<len;i++)
	{
	    MDEBUG2 (DPM_ERROR, "%02X ", apkt [i]);
	}
	MDEBUG2 (DPM_ERROR, "\n");
	//
	rmf_osal_mutexRelease(vlg_SessionEntryMutex);
	return bStatus;
    }
    
    i32ResourceQueueId = RM_Get_Queue ( asSessionStatus [usSession].ulRID);
    //    
    rmf_osal_mutexRelease(vlg_SessionEntryMutex);
	
    
    if ( i32ResourceQueueId != INVALID_RESOURCE_ID )
    {
        eventInfo.event      = POD_APDU;
	eventInfo.x          = usSession;   // x contains the session number
	eventInfo.z          = len;         // z contains byte count of the apdu packet 
	eventInfo.dataPtr    = NULL;        // no copying of dataPtr
	eventInfo.dataLength = 0;           // no copying of dataPtr

	/* Store APDU in static circular buffer based on the size of the APDU. 
	** Then other treads can access this buffer at their leisure. */
	if (len > APDUIN_MIDDLE_BUF_SIZE )  //use the large size buffers
	{
                
	    MDEBUG (DPM_TPDU, "Parsed %02x%02x%02x APDU [LARGE] (len:%d, session:%d, idx:%d)\n", 
				len, usSession, largeIndex, apkt[0], apkt[1], apkt[2]);
	    rmf_osal_memcpy ((char *)apduinLarge [largeIndex], apkt, len,
			APDUIN_LARGE_BUF_SIZE, len );    // this apduin[] has to persist over the lcsm_send_event
	    eventInfo.y = (long)&apduinLarge [largeIndex][0];        // y contains pointer to the actual apdu

	    /* Point to next circular buffer for next APDU */
	    if (++largeIndex >= APDUIN_MAXBUF_LARGE)
	    {
		largeIndex = 0;
	    }
                
	}
	else if (len > APDUIN_SMALL_BUF_SIZE)
	{
	    MDEBUG (DPM_TPDU, "Parsed %02x%02x%02x APDU [MEDIUM] (len:%d, session:%d, idx:%d)\n", 
				len, usSession, middleIndex, apkt[0], apkt[1], apkt[2]);
				
	    memcpy ((char *)apduinMiddle[middleIndex], apkt, len);  // this apduin[] has to persist over the lcsm_send_event
	    eventInfo.y = (long)&apduinMiddle[middleIndex][0];      // y contains pointer to the actual apdu

	    /* Point to next circular buffer for next APDU */
	    if (++middleIndex >= APDUIN_MAXBUF_MIDDLE)
		middleIndex = 0;
	}
	else
	{
	    MDEBUG (DPM_TPDU, "Parsed %02x%02x%02x APDU [SMALL] (len:%d, session:%d, idx:%d)\n", 
				apkt[0], apkt[1], apkt[2], len, usSession, smallIndex);
				
	    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.POD"," Parsed %02x%02x%02x APDU [SMALL] (len:%d, session:%d, idx:%d)\n", 
				apkt[0], apkt[1], apkt[2], len, usSession, smallIndex);
				
	    rmf_osal_memcpy ((char *)apduinSmall[smallIndex], apkt, len,
			APDUIN_SMALL_BUF_SIZE, len );    // this apduin[] has to persist over the lcsm_send_event
	    eventInfo.y = (long)&apduinSmall[smallIndex][0];        // y contains pointer to the actual apdu

	    /* Point to next circular buffer for next APDU */
	    if (++smallIndex >= APDUIN_MAXBUF_SMALL)
		smallIndex = 0;
	}
	
	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," Calling lcsm_send_event\n");
	//
	lcsm_send_event (hLCSMdev_pod, i32ResourceQueueId, &eventInfo);    // wake up the resource/application thread
             
        //if ( eventInfo.returnStatus != 0 )
        //{
	//       MDEBUG (DPM_ERROR, "ERROR:ParseAPDU Large: Bad Send: %d\n", eventInfo.returnStatus );
	//}
        //else
	  {
	      MDEBUG (DPM_APPLI, "Sending session %d APDU to resource queue %ld\n", usSession, i32ResourceQueueId);
	      bStatus = SUCCESS;
	  }
    }
    else
    {
	RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s: Bad RID (%ld) returned from RM_GetQueue for session # %d\n", 
		    __FUNCTION__, i32ResourceQueueId, usSession);
    }

    
    
    return bStatus;
}

ResClass_t ResourseClassList[ ] = 
{
    { M_RES_MNGR_CLS,    "# RESOURCE MGR #" },//1
    { M_APP_INFO_CLS,    "# APP INFO #" },//2
    { M_CA_CLS,          "# CA SUPP#" },//3
    { M_DSG_CLS,         "# DSG #" },//4
    { M_HOMING_CLS,      "# HOMING #" },//5
    { M_HOST_CTL_CLS,    "# HOST CTL #" },//6
    { M_SYS_TIME_CLS,    "# SYTEM TIME#" },//7
    { M_CARD_RES_CLS,    "# CARD RES #" },//8
    { M_GEN_FET_CTL_CLS, "# GENERIC FETURE CTL #" },//9
    { M_SYTEM_CTL_CLS,   "# SYSTEM CTL #" },//10
    { M_HEAD_END_COMM_CLS, "# HEADEND COMM #" },//11
    { M_MMI_CLS,         "# MMI #" },//12
    { M_LOW_SPD_COM_CLS, "# LOW SPD COM #" },//13
    { M_GEN_IPPV_SUP_CLS,"# GEN IPPV SUP #" },//14
    { M_SAS_CLS,         "# SAS #" },//15
    { M_EXT_CHNL_CLS,    "# EXT CHANNEL #" },//16
    { M_CP_CLS,          "# CP #" },//17
    { M_GEN_DIAG_SUP_CLS,"# GEN DIAG SUPP #" },//18
    { M_CARD_MIB_ACC_CLS,"# CARD MIB ACCESS #" },//19
    { M_HOST_ADD_PRT_CLS,"# HOST ADD PRT #" },//20
    { M_IPDIRECT_UNICAST_CLS, "# IPDIRECT UNICAST #" },//21

};


CardHostResInfo_t ResInfo;

/*******************************************************************/
/*
 *    AM_OPEN_SS_REQ 
 *    Callback (Called by Session)
 *  Inform host that a POD is requiring a Session on a Resource
*/
#define VL_APP_MGR_RES_LIST_SIZE  32
static unsigned long vlg_ResoourceList[VL_APP_MGR_RES_LIST_SIZE];
static unsigned long vlg_ListSize = 0;
void AM_OPEN_SS_REQ(ULONG RessId, UCHAR Tcid)
{
    LCSM_EVENT_INFO eventInfo;
    int32    queue,ii;
    int ResCls,Version,Type;
    MDEBUG (DPM_SS, "AM_OPEN_SS_REQ: resid=0x%x tcid=%d\n", (int)RessId, (int)Tcid);
    
    /* Check with the manager of this resource and see if a(nother) 
    ** session can be opened. */
    eventInfo.event = POD_OPEN_SESSION_REQ;
    eventInfo.x = (ULONG)RessId;     // x contains the resource ID
    eventInfo.y = (ULONG)Tcid;        // y contains Tcid
    eventInfo.dataPtr    = NULL;      // no copying of dataPtr
        eventInfo.dataLength = 0;         // no copying of dataPtr
    ResCls = RessId & 0xFFFF0000;
        ResCls = ResCls >> 16;
    Version = RessId & 0x3F;
    Type  = (RessId & 0xFFC0) >> 6;

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," >>>>>>>>>> CARD Requested RsID:0x%08X \n",RessId);

    for(ii = 0; ii < sizeof(ResourseClassList)/sizeof(ResClass_t); ii++)
    {
      if(ResourseClassList[ii].ResClass == ResCls)
      {
          RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD",">>>>>> OPEN SESION REQ for <<<< %s >>>>>. RsID: 0x%08X\n",ResourseClassList[ii].ResourceName, RessId); 
      }
    }    

    //for(ii = 0; ii < VL_APP_MGR_RES_LIST_SIZE; ii++)
    for(ii = (VL_APP_MGR_RES_LIST_SIZE-1); ii >= 0; ii--)
    {
        int ResClass,Hver,Htype;
        ResClass = ((vlg_ResoourceList[ii] & 0xFFFF0000) >> 16);
        //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," vlg_ResoourceList[%02d]:0x%X \n",ii,vlg_ResoourceList[ii]);
        if( ResCls == ResClass )
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," vlg_ResoourceList[%02d]:0x%X \n",ii,vlg_ResoourceList[ii]);
            Hver = (vlg_ResoourceList[ii] & 0x3F) ;
            Htype = ((vlg_ResoourceList[ii] & 0xFFC0)>> 6);    
            if(Version == 0)
            {
                eventInfo.x = (ULONG)vlg_ResoourceList[ii];// x contains the resource ID
            }
            if(ResClass != 0x002B)// this for system control
            {
                if( (Version > Hver) || (Type != Htype) )
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD",">>> Invalid CARD Open Session Request !!!!  Pod Version(0x%08X) > Host ver(0x:%08X) \n",RessId,vlg_ResoourceList[ii]) ;
                    AM_OPEN_SS_RSP( SESS_STATUS_BAD_VERS, RessId, eventInfo.y);
                    return;
                }
            }
            else
            {
                if( (Version > Hver) || (Type > Htype) )
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD",">>> Invalid CARD Open Session Request !!!!  Pod Version(0x%08X) > Host ver(0x:%08X) \n",RessId,vlg_ResoourceList[ii]) ;
                    AM_OPEN_SS_RSP( SESS_STATUS_BAD_VERS, RessId, eventInfo.y);
                    return;
                }
            }
            break;
        }
    }
    queue = RM_Get_Queue(RessId);
    
    if (queue != -1)    
    {
//        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","AM_OPEN_SS_REQ: Calling  lcsm_send_event queue:0x%08X \n",queue);
        lcsm_send_event (hLCSMdev_pod, queue, &eventInfo);    // wake up the resource/application thread
    }
         
    else
        MDEBUG (DPM_ERROR, "ERROR: Invalid resource ID (%ld) requested\n", RessId);
}

void vlSetResourceList(unsigned long *pList, unsigned long size)
{
    int ii;
    if(size <= VL_APP_MGR_RES_LIST_SIZE)
    {
        for(ii = 0; ii < size; ii++)
        {
            vlg_ResoourceList[ii] = pList[ii];
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","vlSetResourceList: 0x%08X \n",vlg_ResoourceList[ii]);
            
        }
        
        vlg_ListSize = size;
    }
}    

/*
 *    AM_SS_OPENED 
 *    Callback (Called by Session)
 *  Inform host that a the session is opened
 *    This function will keep an association between the assigned SessNumber and lResourceId
 *  in a table. This table will be used later when AM_SS_CLOSED is requested by the POD,
 *    since AM_SS_CLOSED(SessNumber) will be called with only a SessNumber.
*/

void AM_SS_OPENED(USHORT SessNumber, ULONG lResourceId, UCHAR Tcid)
{
    LCSM_EVENT_INFO eventInfo;
    int32    queue;

    MDEBUG (DPM_SS, "AM_SS_OPENED: sessNbr=%d resId=0x%x tcid=%d\n", \
           (int)SessNumber, (int)lResourceId, (int)Tcid);

    eventInfo.event = POD_SESSION_OPENED;
    eventInfo.x = (ULONG)SessNumber;     // x contains the session number
    eventInfo.y = (ULONG)lResourceId;    // y contains the resource ID
    eventInfo.z = (ULONG)Tcid;            // z contains the Tranport Connection ID
    eventInfo.dataPtr    = NULL;      // no copying of dataPtr
    eventInfo.dataLength = 0;         // no copying of dataPtr
    
    /* Determine which queue to use to send messages to this resource */
    queue = RM_Get_Queue(lResourceId);
    
    if ( queue != -1)    
    {
        if (add_session_entry (SessNumber, lResourceId))
        {    // if entry added successfully
            lcsm_send_event (hLCSMdev_pod, queue, &eventInfo);    // wake up the resource/application thread
        }
        else{
            MDEBUG (DPM_ERROR, "ERROR: Failed adding a session number to session_entry\n");
        }            
    }
    else{
        MDEBUG (DPM_ERROR, "ERROR: Invalid resource ID (%ld) requested\n", lResourceId);
    }        
}

/*
 *    AM_SS_CLOSED
 *    Callback (Called by Session)
 *  Inform host that the session has been closed by POD
*/
void AM_SS_CLOSED(USHORT SessNumber)
{
    LCSM_EVENT_INFO eventInfo;
    int32    queue;
    ULONG    resource_id;

    MDEBUG (DPM_SS, "Closing session number %d\n", (int)SessNumber);

    eventInfo.event = POD_CLOSE_SESSION;
    eventInfo.x = (ULONG)SessNumber;     // x contains the session number
        eventInfo.dataPtr    = NULL;      // no copying of dataPtr
        eventInfo.dataLength = 0;         // no copying of dataPtr
    
    // find resource id corresponding to an opened session number
    resource_id = delete_session_entry (SessNumber);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","AM_SS_CLOSED:SessNumber:%d resource_id:0x%08X\n",SessNumber,resource_id);
    if (resource_id == INVALID_RESOURCE_ID)
    {
        MDEBUG (DPM_ERROR, "Failed - cannot find an opened resource for the session number to be deleted\n");
        return;
    }

        queue = RM_Get_Queue ( resource_id );
    
    if ( queue != -1){    
        lcsm_send_immediate_event (hLCSMdev_pod, queue, &eventInfo);    // wake up the resource/application thread
    }        
    else{
        MDEBUG (DPM_ERROR, "ERROR: Invalid resource ID for session %d\n", (int)SessNumber);
    }        

}

/*************************************************************************
 *
 *     Make all the session entries available
 */
void init_session_entry (void)
{
    int i;
    
    for (i=0;i<NB_MAX_SESSION;i++)
    {
        asSessionStatus[ i ] .bfInUse       = FALSE;   // session entry is free
        asSessionStatus[ i ] .bfInRequest   = FALSE;   // session entry is free
        asSessionStatus[ i ] .ulRID         = 0;       // No Resource ID
    }
    
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s : Create Mutex for Synchronization Purpose\n", __FUNCTION__);
    
    rmf_osal_mutexNew( &vlg_SessionEntryMutex);
    if( 0 == vlg_SessionEntryMutex) 
      RDK_LOG(RDK_LOG_WARN, "LOG.RDK.POD", "%s : Mutex creation fails\n");
}

/*************************************************************************
 *
 *     Find an empty storage in the session entry table to keep the association
 *    between session number and resource id
 *    Return: 1 - successful entry addition,  0 - failed, session_entry is full
 */
static BOOL add_session_entry (USHORT usSessionNum, ULONG ulRID)
{
    int added = FALSE;      // session number already used
    if(usSessionNum >= (NB_MAX_SESSION-1))
        return added;
    
    rmf_osal_mutexAcquire(vlg_SessionEntryMutex);
    
    if (asSessionStatus[ usSessionNum ] .bfInUse == FALSE)
    {
        asSessionStatus[ usSessionNum ] .bfInUse     = TRUE;
        asSessionStatus[ usSessionNum ] .bfInRequest = FALSE;
        asSessionStatus[ usSessionNum ] .ulRID       = ulRID;        // Success - session number & rscid added
        added = TRUE;
    }
    else
    {
        MDEBUG (DPM_ERROR, "ERROR: Could not add session entry %d\n", usSessionNum );
    }
    
    rmf_osal_mutexRelease(vlg_SessionEntryMutex);
    return added;
}

/*************************************************************************
 *
 *    Find the opened resource for the session to be deleted
 *    If found, delete the entry in session_entry[] and return the
 *    corresponding resource id.
 *    If not found, return 0
 */
static ULONG delete_session_entry (USHORT usSessionNum)
{
    ULONG ulRID;
    
    rmf_osal_mutexAcquire(vlg_SessionEntryMutex);
    
    ulRID = asSessionStatus[ usSessionNum ] .ulRID;

    asSessionStatus[ usSessionNum ] .bfInUse     = FALSE;   // session entry is free
    asSessionStatus[ usSessionNum ] .bfInRequest = FALSE;   // session entry is free
    asSessionStatus[ usSessionNum ] .ulRID       = 0;       // No Resource ID
    
    MDEBUG (DPM_SS, "session=%d rscid=0x%08x\n", usSessionNum, (int)ulRID);

    rmf_osal_mutexRelease(vlg_SessionEntryMutex);
    
    return ulRID;    // return the corresponding opened resource id - 0:no rscid, otherwise:rscid
}



/************************************************************************
**
**  Find the first available session number and return that value.
**  If none remain available then return an invalid session ID. 
**
**
**/
USHORT find_available_session ( void )
{
    USHORT usSessionNum;

    rmf_osal_mutexAcquire(vlg_SessionEntryMutex);
    
    // run through open session table and send a POD_CLOSE_SESSION
    // to the appropriate resource
    for( usSessionNum = FIRST_SESSION_NUMBER; usSessionNum < NB_MAX_SESSION; usSessionNum++ )
    {
        if(( asSessionStatus[ usSessionNum ].bfInUse     == FALSE ) &&
           ( asSessionStatus[ usSessionNum ].bfInRequest == FALSE ))
        {
            asSessionStatus[ usSessionNum ].bfInRequest = TRUE;
	    
	    rmf_osal_mutexRelease(vlg_SessionEntryMutex);
            return usSessionNum;
        }
    }
    
    rmf_osal_mutexRelease(vlg_SessionEntryMutex);
    
    return INVALID_SESSION_NUMBER;
}


//************************************************************************
//
// when stack detects that POD has been ejected,
// it calls AM_POD_EJECTED in order to force closure
// of all open sessions
int AM_POD_EJECTED( void )
{
    USHORT usSessionNum;
    int    closed = 0;     // number of sessions closed
   
    MDEBUG (DPM_SS,"AM_POD_EJECTED Here1\n");
    
    rmf_osal_mutexAcquire(vlg_SessionEntryMutex);
    
    // run through open session table and send a POD_CLOSE_SESSION
    // to the appropriate resource
    for( usSessionNum = FIRST_SESSION_NUMBER; usSessionNum < NB_MAX_SESSION; usSessionNum++ )
    {
        MDEBUG (DPM_SS,"AM_POD_EJECTED Here2 = %x\n",usSessionNum);
    
        if( asSessionStatus[ usSessionNum ] .bfInUse )
        {
            closed++;
            MDEBUG (DPM_SS,"AM_POD_EJECTED Here3 = %x\n",usSessionNum);
    
            AM_SS_CLOSED ( usSessionNum );
        }
    }
    
    rmf_osal_mutexRelease(vlg_SessionEntryMutex);
    return closed;
}


#ifdef __cplusplus
}
#endif



