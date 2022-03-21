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
#include <string.h>
#include <stdlib.h>
 
 
#include "poddef.h"
                 /* To support log file */
#include "global_event_map.h"
#include "global_queues.h"     //Hannah
#include "utils.h"
#include "lcsm_log.h"  
//#include "cis.h"
//#include "module.h"
//#include "link.h"
#include "transport.h"
#include "session.h"
//#include "ci.h"
//#include "podlowapi.h"
#include "podhighapi.h"
#include "applitest.h"
#include <string.h>
#include "utilityMacros.h"
#include "rmf_osal_mem.h"
#include "rdk_debug.h"

#if USE_SYSRES_MLT
#include "mlt_malloc.h"
#endif

#define __MTAG__ VL_CARD_MANAGER


#ifdef __cplusplus
extern "C" {
#endif 

//------------------------------------GLOBAL------------------------------------
// To be changed according to specific implementation

    USHORT    SesNbForResMan,
            SesNbForHostControl,
            SesNbForAppInfo,
            SesNbForMMI,
            SesNbForExtChan,
            SesNbForDateTime;
            
    UCHAR   transId    = 0;    // non-zero open transport connection ID, if any
    UCHAR   vl_gCardType = 0xFF; 


/****************************************************************************************


            P O D   T O   H O S T   F U N C T I O N S 

            Callbacks called by Session layer, to be implemented regarding
            Host capabilities


****************************************************************************************/


/****************************************************************************************
Name:             AM_TC_CREATED
type:            Callback (Called by Session)
Description:     Inform host of the creation of a new Transport Connexion

In:             UCHAR    Tcid
Out:            Nothing
Return value:    void
*****************************************************************************************/
void AM_TC_CREATED(UCHAR Tcid)
{
    transId = Tcid;     // make transport connection ID accessible to POD resources

    MDEBUG ( DPM_APPLI, "***************************************************\r\n");
    MDEBUG ( DPM_APPLI,"POD Driver call AM_TC_CREATED Tcid = %d\r\n",Tcid);
    MDEBUG ( DPM_APPLI, "Update host variables here\r\n");
    MDEBUG ( DPM_APPLI, "***************************************************\r\n");
}

/****************************************************************************************
Name:             AM_OPEN_SS_REQ
type:            Callback (Called by Session)
Description:     Inform host that a POD is requiring a Session on a Resource

In:             ULONG    Resource ID
                UCHAR    Tcid
Out:            Nothing
Return value:    void
*****************************************************************************************/
#if 0    // replaced by a real AM_OPEN_SS_REQ in appmgr.c
void AM_OPEN_SS_REQ(ULONG RessId, UCHAR Tcid)
{

    MDEBUG ( DPM_APPLI, "***************************************************\r\n");
    MDEBUG ( DPM_APPLI,"POD requests to open a session on Resource 0x%08X\r\n",(unsigned int)RessId);
    MDEBUG ( DPM_APPLI, "Update host variables here, respond OK\r\n");
    MDEBUG ( DPM_APPLI, "And call AM_OPEN_SS_RSP to reply\r\n");
    AM_OPEN_SS_RSP( SESS_STATUS_OPENED, RessId, Tcid);
    MDEBUG ( DPM_APPLI, "***************************************************\r\n");
}
#endif

/****************************************************************************************
Name:             AM_SS_OPENED 
type:            Callback (Called by Session)
Description:     Inform host that a the session is opened

In:             ULONG    Resource ID on which the session has been opened
                USHORT  SessNumber given by seesion layer for this resource
                UCHAR   Tcid on which this session has been opened

Out:            Nothing
Return value:    void
*****************************************************************************************/
#if 0    // replaced by a real AM_OPEN_SS_OPENED in appmgr.c
void AM_SS_OPENED(USHORT SessNumber, ULONG lResourceId, UCHAR Tcid)
{

    UCHAR pApduRespResMan[]  = {0x9F, 0x80, 0x10, 0x00};
    
    UCHAR pApduRespExtChan[7];


    MDEBUG ( DPM_APPLI, "***************************************************\r\n");
    
    if (lResourceId == RSC_RESOURCEMANAGER)
    {
        SesNbForResMan = SessNumber;
        MDEBUG ( DPM_APPLI, "The session to Resource Manager has been opened\r\n");
        MDEBUG ( DPM_APPLI, "An APDU ProfileEnq must be sent to POD\r\n");
        AM_APDU_FROM_HOST(SessNumber,pApduRespResMan,sizeof(pApduRespResMan));
    }
    else if (lResourceId == RSC_HOSTCONTROL)
    {
        SesNbForHostControl = SessNumber;
        MDEBUG ( DPM_APPLI, "The session to HostControl has been opened\r\n");
    }
    else if (lResourceId == RSC_APPINFO    )
    {
        SesNbForAppInfo = SessNumber;
        MDEBUG ( DPM_APPLI, "The session to AppInfo has been opened\r\n");
    }
    else if (lResourceId == RSC_DATETIME )
    {
        SesNbForDateTime = SessNumber;
        MDEBUG ( DPM_APPLI, "The session to Date and time has been opened\r\n");
    }
    else if (lResourceId == RSC_EXTENDEDCHANNEL )
    {
        SesNbForExtChan = SessNumber;
        MDEBUG ( DPM_APPLI, "The session to Extended Channel has been opened\r\n");
        MDEBUG ( DPM_APPLI, "Send a NewFlowRequest Apdu to test out of band Data\r\n");
/*
        Start by connecting the HPNX extender into the receiver.
        Go to the HPNX Device panel and connect to the HPNX node
        under POD behavior mode.
        Go to the Low Level Test/Setup index, click Play and verify that
        the HOST is properly responding to the HPNX
        Open_session_req() message.
        Go to the Extended Channel resource, open the session, and
        verify that the HOST is sending a New_flow_req() APDU with
        service type MPEG_section, write down the Flow_ID value
        granted by the HPNX.
        Expand the Flow Feed command, and click Browse to select the
        file where you have recorded your table session. Enter the
        Flow_ID value that has been granted in the previous step and click
        Send.
*/
        pApduRespExtChan[0] = 0x9F;
        pApduRespExtChan[1] = 0x8E;
        pApduRespExtChan[2] = 0x00;
        
        pApduRespExtChan[3] = 3;            // Size
        pApduRespExtChan[4] = 0x00;            // MPEG Section
        pApduRespExtChan[5] = 0x00;            // if (Service_type == MPEG_section) 
        pApduRespExtChan[6] = 0xFF;            // {                                
                                            //        reserved    3 bslbf
                                            //        PID            13 uimsbf
                                            // }

        AM_APDU_FROM_HOST(SessNumber,pApduRespExtChan,sizeof(pApduRespExtChan));
    }

    else
    {
        MDEBUG ( DPM_APPLI,"A session to ResId 0x%08X has been opened\r\n",(unsigned int)lResourceId);
    }
}
#endif

/****************************************************************************************
Name:             AM_APDU_FROM_POD 
type:            Callback (Called by Session)
Description:     Send an APDU to HOST 

In:             USHORT SessNumber    : Current Session of this resource
                UCHAR * pApdu        : Pointer on the allocated APDU
                USHORT Length        : Allocated Size
Out:            Nothing
Return value:    void
*****************************************************************************************/
#if 0    // replaced by a real AM_APDU_FROM_POD in appmgr.c
void AM_APDU_FROM_POD(USHORT SessNumber, UCHAR * pApdu, USHORT Length)
{
    char Mess[200];
    char temp[10];
    unsigned short k;
    
    MDEBUG ( DPM_APPLI, "***************************************************\r\n");
    MDEBUG ( DPM_APPLI, "An APDU has been sent to host\r\n");
    MDEBUG ( DPM_APPLI, "Process it to appropriate resource...\r\n");
    snprintf(Mess, sizeof(Mess), "APDU = ");
    for ( k=0; k<Length;k++)
    {
        snprintf    (temp, sizeof(temp), "%02X ",pApdu[k]);
        if ( (!(k+2%16)) && (k != 0)) strcat(temp,"\r\n             ");
        strcat(Mess,temp);
    }
    MDEBUG ( DPM_APPLI, Mess);

    
    // Test what kind of APDU

    // Manage some of them as a sample....
    // For minimal test...


    if ( (pApdu[0] == 0x9F) && (pApdu[1] == 0x80) && (pApdu[2] == 0x11) )
    {
        // Received ProfileReply,Host has now to answer with a ProfileChange (9f 80 12 00)
        UCHAR ApduResp[] = {0x9F, 0x80, 0x12, 0x00};
        MDEBUG ( DPM_APPLI, "\r\nThis is a ProfileReply, POD describe its own embedded applications\r\n");
        AM_APDU_FROM_HOST(1,ApduResp, sizeof(ApduResp));
    }

    else if( (pApdu[0] == 0x9F) && (pApdu[1] == 0x80) && (pApdu[2] == 0x10) )
    {
        // Received ProfileEnquiry from CAM, host shall describe it's own resources
        // Host has now to answer with a ProfileReply
        UCHAR ApduResp[] = {    0x9f, 0x80, 0x11,        // Apdu 
                                0x82, 0x00, 0x18,        // Size (to be modified according to declared resources)
                                0x00, 0x01, 0x00, 0x41,    // RSC_RESOURCEMANAGER
                                0x00, 0x03, 0x00, 0x41,    // RSC_CASUPPORT
                                0x00, 0x02, 0x00, 0x41,    // RSC_APPINFO
                                //0x00, 0x24, 0x00, 0x41,// RSC_DATETIME
                                0x00, 0x40, 0x00, 0x41, // RSC_MMI
                                0x00, 0x20, 0x00, 0x41, // RSC_HOSTCONTROL
                                0x00, 0xA0, 0x00, 0x42    // RSC_EXTENDEDCHANNEL
                            };

        MDEBUG ( DPM_APPLI, "\r\nThis is a ProfileEnquiry, Host must describe its own embedded applications\r\n");
        MDEBUG ( DPM_APPLI, "Simulate a typical response\r\n");
        AM_APDU_FROM_HOST(1,ApduResp,sizeof(ApduResp));
    }

    else
    {
        if ( SessNumber == SesNbForHostControl)
        {
            MDEBUG ( DPM_APPLI, "\r\nThis is an APDU for host Control, check what has to be done\r\n");
        }
        else if ( SessNumber == SesNbForDateTime )
        {
            MDEBUG ( DPM_APPLI, "\r\nThis is an APDU for Date Time, check what has to be done\r\n");
        }
        else
        {
            MDEBUG ( DPM_APPLI, "\r\nAPDU not processed, to be implemented\r\n");
        }
    }

    MDEBUG ( DPM_APPLI, "\r\n***************************************************\r\n");
}
#endif

/****************************************************************************************
Name:             AM_SS_CLOSED 
type:            Callback (Called by Session)
Description:     Inform host that the session has been closed by POD

In:             USHORT SessNumber    : Session closed
Out:            Nothing
Return value:    void
*****************************************************************************************/
#if 0    // replaced by a real AM_SS_CLOSED in appmgr.c
void AM_SS_CLOSED(USHORT SessNumber)
{
    char Mess[200];

    MDEBUG ( DPM_APPLI, "***************************************************\r\n");
    snprintf(Mess, sizeof(Mess), "POD informs host that the session number %d has been closed\r\n",SessNumber);
    MDEBUG ( DPM_APPLI, Mess);
    MDEBUG ( DPM_APPLI, "Update Host Variables here\r\n");
    
    // Find which resource is associated to this Session Number
    if ( SessNumber == SesNbForAppInfo)
    {
        MDEBUG ( DPM_APPLI, "The session to App Info has been closed\r\n");
    }
    else if ( SessNumber == SesNbForHostControl)
    {
        MDEBUG ( DPM_APPLI, "The session to Host Control has been closed\r\n");
    }
    else if ( SessNumber == SesNbForResMan)
    {
        MDEBUG ( DPM_APPLI, "The session to Res manager has been closed\r\n");
    }
    else if ( SessNumber == SesNbForMMI)
    {
        MDEBUG ( DPM_APPLI, "The session to MMI has been closed\r\n");
    }
    else if ( SessNumber == SesNbForExtChan)
    {
        MDEBUG ( DPM_APPLI, "The session to Extended Channel has been closed\r\n");
    }
    else
        MDEBUG ( DPM_APPLI, "Don't know what resource was associated to this Session Number\r\n");

    MDEBUG ( DPM_APPLI, "***************************************************\r\n");
}
#endif



/****************************************************************************************
Name:             AM_TC_DELETED 
type:            Callback (Called by Session)
Description:     Inform host that a Transport Connexion has been deleted
In:             UCHAR Tcid    
Out:            Nothing
Return value:    void
*****************************************************************************************/
void AM_TC_DELETED(UCHAR Tcid)
{
    MDEBUG ( DPM_APPLI, "***************************************************\r\n");
    MDEBUG ( DPM_APPLI,"POD informs host that the the TCID %d has been closed\r\n",Tcid);
    MDEBUG ( DPM_APPLI, "Update Host Variables here\r\n");
    MDEBUG ( DPM_APPLI, "***************************************************\r\n");

    if( Tcid == transId )
        transId = 0;        // transport connection is closed now
}


/****************************************************************************************
Name:             AM_POD_STATUS
type:            Callback (Called by Session)
Description:     Inform host of the status of the POD

In:             
Out:            Nothing
Return value:    void
*****************************************************************************************/
void AM_POD_STATUS(POD_STATUS_ERROR Status)
{
    MDEBUG ( DPM_APPLI, "***************************************************\r\n");

    MDEBUG ( DPM_APPLI, "New Status for POD\r\n");
    switch (Status)
    {
    case POD_NOT_INITIALIZED:
        MDEBUG ( DPM_APPLI, "ERROR:POD_NOT_INITIALIZED\r\n");
        break;
    case POD_INIT_OK:
        MDEBUG ( DPM_APPLI, "POD_INIT_OK\r\n");
        break;
    case READ_FAILURE:
        MDEBUG ( DPM_APPLI, "ERROR:READ_FAILURE\r\n");
        break;
    case WRITE_FAILURE:
        MDEBUG ( DPM_APPLI, "ERROR:WRITE_FAILURE -- Can't write to POD\r\n");
        break;
    case PROCESS_TO_POD_FAILURE:
        MDEBUG ( DPM_APPLI, "ERROR:PROCESS_TO_POD_FAILURE\r\n");
        break;
    case CANNOT_CREATE_TC:
        MDEBUG ( DPM_APPLI, "ERROR:CANNOT_CREATE_TC\r\n");
        break;
    
    default:
        MDEBUG ( DPM_APPLI, "UNKNOWN_ERROR\r\n");
        break;
    }
    MDEBUG ( DPM_APPLI, "***************************************************\r\n");

}

/****************************************************************************************
Name:             AM_EXTENDED_DATA ()
type:            function (called by Session)
Description:     

In:             

Out:            Nothing
Return value:    void
*****************************************************************************************/
#define POLYNOMIAL        0x04c11db7        // for Crc32
#define    CRC_TABLE_SIZE    256

static unsigned    guiCrcTable[CRC_TABLE_SIZE];

// proto moved to applitest.h
void GenerateCrcTable( void );

extern "C" int vlXchanPostOobAcquired();

static int vlg_bOobAcquired = 0;

void AM_EXTENDED_DATA( ULONG FlowID, UCHAR* pData, USHORT usnPacketSize)
{    
    LCSM_EVENT_INFO eventInfo;
    unsigned crc;
    unsigned char *pBuf = NULL;
    const int nSecLenBytes = 3;
    const int nCrc32Bytes  = 4;
    const int nMinSecBytes = nSecLenBytes + nCrc32Bytes;
    int nSection = 0;

    int nPacketSize = usnPacketSize;
    
    if(!vlg_bOobAcquired)
    {
        vlg_bOobAcquired = 1;
        vlXchanPostOobAcquired();
    }

// Changed: Apr-12-2008: Split Table into sections
    while(nPacketSize > nMinSecBytes)
    {
        unsigned long Size = 0;
        int offNextSection = 0;
        unsigned long nSectionCrc32    = 0;
        unsigned long nCalculatedCrc32 = 0;
        int nTableId = 0;

        // discard any 0xFF paddings
        // check for sections starting with 0xFF
        while((nPacketSize > 4) && (0xFFFFFFFF == VL_VALUE_4_FROM_ARRAY(pData)))
        {
            pData       += 4;
            nPacketSize -= 4;
        }
        
        while((nPacketSize > 1) && (0xFF == *pData))
        {
            pData       += 1;
            nPacketSize -= 1;
        }

        // is the section done
        if(nPacketSize < nMinSecBytes) continue;

        nTableId = *pData;
        Size = (nSecLenBytes+(VL_VALUE_2_FROM_ARRAY(pData+1)&0xFFF));
        offNextSection = Size;

        if(nSection >= 1)
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "AM_EXTENDED_DATA: Sending Section %d of table-id 0x%02X, Size = %d, nPacketSize = %d\n", nSection, nTableId, Size, nPacketSize);
        }
        
        /*if(0x74 == nTableId)
        {
            vlMpeosDumpBuffer(RDK_LOG_TRACE9, "LOG.RDK.POD", pData, Size);
        }*/
        
        if(Size < nMinSecBytes)
        {
            //MDEBUG ( DPM_ERROR, "PANIC: Size = %d is too small for table-id 0x%02X, discarding rest of %d bytes.\r\n", Size, nTableId, nPacketSize);
            break;
        }
        
        if(Size > 4096)
        {
            //MDEBUG ( DPM_ERROR, "PANIC: Size = %d is too big for table-id 0x%02X, discarding rest of %d bytes.\r\n", Size, nTableId, nPacketSize);
            break;
        }

        // check if the section is a CRC checkable section
        if(Size >= nMinSecBytes)
        {
            nSectionCrc32    = VL_VALUE_4_FROM_ARRAY(pData+(Size-nCrc32Bytes));
            /* CRC needs to pass for all tables // if(0 == nSectionCrc32)
            {
                // pretend that the CRC checked out
                nCalculatedCrc32 = CRC_ACCUM;
            }
            else*/
            {
                // calculate CRC
                nCalculatedCrc32 = Crc32(CRC_ACCUM, pData, Size);
            }

            // check CRC before passing it up
            if(CRC_ACCUM == nCalculatedCrc32)
            {
                #ifdef VL_USE_VLOCAP_STACK
				rmf_osal_memAllocP(RMF_OSAL_MEM_POD, Size * sizeof(UCHAR),(void **)&pBuf);
                if (pBuf )
                {
                    //memcpy( pBuf, (const void*)pData, Size );
                    memcpy( pBuf, (const void*)pData, Size );
                    eventInfo.event = ASYNC_TDB_OB_DATA;
                    eventInfo.x = (long int)FlowID;
                    eventInfo.y = (long int)pBuf;
                    eventInfo.z = (long int)Size;
                    eventInfo.dataPtr    = NULL;
                    eventInfo.dataLength = 0;

                    lcsm_send_event( hLCSMdev_pod, POD_XCHAN_QUEUE, &eventInfo );
                }
                else
                {
                    MDEBUG ( DPM_ERROR, "ERROR: Can't malloc for Extended Packet \r\n");
                    break;
                }
                #endif

                if(nTableId < 0xF)
                {
                    //RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.POD", "AM_EXTENDED_DATA: CRC passed for table-id 0x%02X of size %d bytes\r\n", nTableId, Size);
                }
            }
            else
            {
                //MDEBUG ( DPM_ERROR, "ERROR: CRC [0x%08X:0x%08X] failed for table-id 0x%02X of size %d bytes... section dropped.\r\n", nSectionCrc32, nCalculatedCrc32, nTableId, Size);
            }
        }

        pData       += offNextSection;
        nPacketSize -= offNextSection;
        nSection++;

        // discard any 0xFF paddings
        while((nPacketSize > 4) && (0xFFFFFFFF == VL_VALUE_4_FROM_ARRAY(pData)))
        {
            pData       += 4;
            nPacketSize -= 4;
        }
        
        while((nPacketSize > 1) && (0xFF == *pData))
        {
            pData       += 1;
            nPacketSize -= 1;
        }
    }
}


void GenerateCrcTable()
{
    register unsigned    crcAccum;
    register int    i, j; 
    
    for (i = 0; i < CRC_TABLE_SIZE; i++) 
    {
        crcAccum = (unsigned)i << 24;
        for (j = 0; j < 8; j++) 
        {
            if (crcAccum & 0x80000000)
            {
                crcAccum <<= 1;
                crcAccum  = (crcAccum ) ^ POLYNOMIAL;
            }
            else
               // crcAccum  = (crcAccum <<= 1);
             crcAccum <<= 1;
        }
        guiCrcTable[i] = crcAccum;
    }
}
/* ----------------------------------------------------------------------------
 *    Calculate the CRC32 on the data block one byte at a time.
 *    Accumulation is useful when computing over all ATM cells in an AAL5 packet.
 *    NOTE: The initial value of crcAccum must be 0xffffffff
 * ----------------------------------------------------------------------------
 */
unsigned long Crc32(unsigned crcAccum, unsigned char *pData, int len)
{
   static unsigned    long tableDone = 0;

    register int i, j;

    if ( !tableDone) 
    {
        GenerateCrcTable();
        tableDone = 1;
    }

    for (j = 0; j < len; j++ ) 
    {
        i = ((int)(crcAccum >> 24) ^ *pData++) & 0xff;
        crcAccum = (crcAccum << 8) ^ guiCrcTable[i];
    }
    return ~crcAccum;
}

unsigned long Crc32Mpeg2(unsigned crcAccum, unsigned char *pData, int len)
{
   static unsigned    long tableDone = 0;

    register int i, j;

    if ( !tableDone)
    {
        GenerateCrcTable();
        tableDone = 1;
    }

    for (j = 0; j < len; j++ )
    {
        i = ((int)(crcAccum >> 24) ^ *pData++) & 0xff;
        crcAccum = (crcAccum << 8) ^ guiCrcTable[i];
    }
    return (crcAccum & 0xffffffff);
}


/* ----------------------------------------------------------------------------
 *    Calculate the CRC32 on one single ATM cell's data block, one byte at a time.
 * ----------------------------------------------------------------------------
 */
unsigned Crc32Atm(unsigned char *pData)
{
    // ATM header not included in data block
    return Crc32(0xffffffff, pData, 44);
}


#ifdef __cplusplus
}
#endif 

