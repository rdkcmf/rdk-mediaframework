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



#include <string.h>         // memset
#include <netinet/in.h>

#include "poddef.h"
#include <global_event_map.h>
#include <global_queues.h>

#include "utils.h"
//#include "podmod.h"
#include <sys/times.h>
#include <errno.h>
#include "lcsm_log.h"       // event logging
#undef FUNAI_JEDI_LINUX_740X
#define FUNAI_JEDI_LINUX_740X

#include "rmf_osal_sync.h"
#include "rmf_osal_thread.h"
#include <string.h>
#include <rdk_debug.h>
#include "rmf_osal_mem.h"
#define __MTAG__ VL_CARD_MANAGER

#ifdef __cplusplus
extern "C" {
#endif 

// Add file name and revision tag to object code
#define REVISION  __FILE__ " $Revision: 1.19 $"
/*@unused@*/
static char *utils_tag = REVISION;

    /***  POD stack-related  ***/

ULONG GetTimeSinceBootMs (unsigned char bPrintAlso) // Get time since boot in milliseconds
{
    ULONG ulTime;
    
    struct tms stTimesBuffer;
    
    /* 'times' returns time in system 'ticks'.  1 tick is 
    ** (currently) 10 milliseconds so adjust time for mills. */
    ulTime = times (&stTimesBuffer) * 10;
    
    if ( bPrintAlso )
    {
        /* Display as seconds.millseconds (using integer truncation to get milliseconds) */
        PDEBUG ( "[%6ld.%3ld]", ulTime/1000, (ulTime - ((ulTime/1000)*1000)));
    }    

    return (ulTime);
}

// does not handle huge (>1193 hours [49 days]) elapsed times properly
// due to multiple wrap-arounds
ULONG ElapsedTimeMs( ULONG baseTime )
{
    ULONG   now = GetTimeSinceBootMs( FALSE );
    ULONG   elapsed = 0;

    if( baseTime <= now )   // assume: no wrap
    {
        elapsed = now - baseTime;
    } else  // baseTime > now   /* so account for a single wrap-around */
    {
        elapsed = ( ( (unsigned long) 0xFFFFFFFF ) - baseTime ) + now;
    }
    return elapsed;
}
void vlReportPodError( int error )
{
    LCSM_EVENT_INFO msg;
    memset( &msg, 0, sizeof( msg ) );
        
        msg.event = POD_DISPLAY_ERROR;
        msg.x = error;      // error code from Table E.1-A
        msg.y = 0;          // unused
        msg.z = 0;          // unused



    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlReportPodError: >>>>>>>>>> reportPodError  Error No:%d \n",error);
   lcsm_send_event( hLCSMdev_pod, GATEWAY_QUEUE, &msg );
}
/* Display POD error to the viewer */
// error codes and prototype defined in pod/include/poderror.h;
// refer to SCTE-28 2003 Appendix E Table E.1-A for error codes and messages
void reportPodError( int error )
{
    LCSM_EVENT_INFO msg;
    memset( &msg, 0, sizeof( msg ) );
      
        msg.event = POD_DISPLAY_ERROR;
        msg.x = error;      // error code from Table E.1-A
        msg.y = 0;          // unused
        msg.z = 0;          // unused



    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD"," >>>>>>>>>> reportPodError  Error No:%d \n",error);
   lcsm_send_event( hLCSMdev_pod, GATEWAY_QUEUE, &msg );
}


int         Cimax = -1;     // file descriptor for /dev/cimax on I2C bus
static char opener[ 80 ];   // owner of CIMaX device

int openCimax( char * requestor )
{
    /* This is left over from the old code opening and closing the cimax
    ** device each time there was an I2C access.  We now have a dedicated
    ** I2C bus and so we can get rid of this.  To minimize code changes
    ** for now (5/19/04) we will just make a stub function */
    
    return Cimax;
}
int REALLYopenCimax( char * requestor )
{
    int status = -1;    // open failed
    int errorNum;
    
    if( Cimax < 0 )
    {
        if( ( Cimax = open( "/dev/i2c_7021", O_RDWR) ) >= 0 )
        {
            status = Cimax;
            strncpy( opener, requestor, sizeof( opener )-2 );
            opener[ sizeof( opener )-1 ] = 0;   // raving paranoia
        } 
        else
        {
            errorNum = errno;
            MDEBUG (DPM_ERROR, "ERROR: openCimax: %s fails to open CIMaX, error = %d\n", requestor, errorNum );
        }
    } else
        MDEBUG (DPM_ERROR, "ERROR: openCimax: %s tries to open CIMaX already-opened by %s\n",
                requestor, opener );

    return status;
}

void closeCimax( char * requestor )
{
    /* This is left over from the old code opening and closing the cimax
    ** device each time there was an I2C access.  We now have a dedicated
    ** I2C bus and so we can get rid of this.  To minimize code changes
    ** for now (5/19/04) we will just make a stub function */
}

void REALLYcloseCimax( char * requestor )
{    
    if( Cimax >= 0 )
    {
        close( Cimax ); Cimax = -1;
        opener[ 0 ] = 0;
    } else
        MDEBUG (DPM_ERROR, "ERROR: closeCimax: %s tries to close CIMaX not opened\n", requestor );
}


    /***  Host resource-related  ***/

// construct the APDU header;
// return a pointer to the first location where data can go
uint8 * buildApduHeader( uint8 * pApdu, uint16 * apduLen, uint32 tag, uint16 dataLen )
{
    uint8 * hdr = pApdu;
    
        *hdr++ = ( ( tag >> 16 ) & 0xFF );
    *hdr++ = ( ( tag >>  8 ) & 0xFF );
    *hdr++ = (   tag         & 0xFF );
        *apduLen = 3 + dataLen;
    if( dataLen < 128 )             // 0..127
        {
        *hdr++ = dataLen;
            *apduLen += 1;
        } else if( dataLen < 256 )      // 128..255
    {
        *hdr++ = 0x81;
        *hdr++ = dataLen;
            *apduLen += 2;
    } else if( dataLen < 65535 )    // 256..65534
    {
        *hdr++ = 0x82;
        *hdr++ = ( ( dataLen >> 8 ) & 0xFF );
        *hdr++ = (   dataLen        & 0xFF);
            *apduLen += 3;
    } else  // dataLen >= 65535
    {
        MDEBUG (DPM_ERROR, "ERROR: invalid APDU dataLen=%d\n", dataLen );
        return NULL;
    }
    return hdr;     // the start of optional, variable-length APDU data storage
}

// construct the APDU;
// return a pointer to the first location where data can go
// nb: if data is NULL, it is assumed caller copies the data portion
uint8 * buildApdu( uint8 * pApdu, uint16 * apduLen, uint32 tag, uint16 dataLen, uint8 * data )
{
    int     ii;
    uint8 * pApduData;
    uint8 * pApduDataStart = NULL;
    if( ( pApduData = buildApduHeader( pApdu, apduLen, tag, dataLen ) ) )
    {
        pApduDataStart = pApduData;
        if( data )
        {
            for( ii = 0; ii < dataLen; ii++ )
                *pApduData++ = *data++;
        } else if( dataLen )
            memset( pApduData, 0, dataLen );    // caller is responsible for data
    }
    return pApduDataStart;  // the start of optional, variable-length APDU data storage
}

// WARNING: DO NOT USE MDEBUG IN HERE!
int SendLogMsg( int level, char* format, ... )
{
    LogInfo * loginf;
    int res=0;

    //PDEBUG( "%s: Attempting to send log message log message: %s\n", __FUNCTION__, format );
    
    if( hLCSMdev_pod < 1 )
    {
       // PDEBUG( "ERROR: device, Failed to log message: %s\n", format );
        return EXIT_FAILURE;
    }
	rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof( LogInfo ),(void **)&loginf);
    if( loginf == NULL )
    {
        PDEBUG( "ERROR: malloc, Failed to log message: %s\n", format );
        return EXIT_FAILURE;
    }

    loginf->diagLevel = level;
    loginf->diagMsg   = format;

   // res = lcsm_send_log( hLCSMdev_pod, loginf ); Hannah
	rmf_osal_memFreeP(RMF_OSAL_MEM_POD, loginf);
    return res;
}
static int vlg_NumOfPodTimersCreated = 0,vlg_MutexCreated = 0;
static vlPodMaxTimers_t stMaxTimers;
static rmf_osal_Mutex vlg_PodTimerMutex;
static void vlPodTimerListClear(vlPodTimedEvents_t *pEventTimer)
{
    int ii;
    for(ii = 0; ii< VL_POD_MAX_TIMERS; ii++)
    {
        if(stMaxTimers.pTimer[ii] == pEventTimer)
        {
            rmf_osal_mutexAcquire(vlg_PodTimerMutex);
            stMaxTimers.pTimer[ii] = NULL;
            vlg_NumOfPodTimersCreated--;
            rmf_osal_mutexRelease(vlg_PodTimerMutex);
            return;
        }
    }
}
static unsigned char vlPodIsAvailInTimerList(vlPodTimedEvents_t *pEventTimer)
{
    int ii;
    for(ii = 0; ii< VL_POD_MAX_TIMERS; ii++)
    {
        if(stMaxTimers.pTimer[ii] == pEventTimer)
        {
            return 1;
        }
    }
    return 0;
}


static void vlPodTimedEventTask(void *arg)
{
    vlPodTimedEvents_t *pTimedEvent;
    int sleepTime = 10;// in miili seconds
    long param = (long)arg;
    pTimedEvent = (vlPodTimedEvents_t*)param;
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","vlPodTimedEventTask:Number:%d Created with time:%d For Event:0x%X\n",vlg_NumOfPodTimersCreated,pTimedEvent->EventTime,pTimedEvent->Event.event);
    while(1)
    {

	 rmf_osal_threadSleep(sleepTime, 0);    

        rmf_osal_mutexAcquire(vlg_PodTimerMutex);
        if(pTimedEvent->EventCanceled)
        {
            rmf_osal_mutexRelease(vlg_PodTimerMutex);
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","vlPodTimedEventTask: Exiting the thread with time elapsed time:%d For Event:0x%X\n",pTimedEvent->TimeElapsed,pTimedEvent->Event.event);
            vlPodTimerListClear(pTimedEvent);
			rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pTimedEvent);
            return;
        }
        
        pTimedEvent->TimeElapsed -= sleepTime;
        rmf_osal_mutexRelease(vlg_PodTimerMutex);
        if(pTimedEvent->TimeElapsed <= 0)
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","vlPodTimedEventTask: Posting the EVENT !!!  elapsed time:%d For Event:0x%X\n",pTimedEvent->TimeElapsed,pTimedEvent->Event.event);
            lcsm_send_timed_event(0, pTimedEvent->Queue,  &pTimedEvent->Event);
            vlPodTimerListClear(pTimedEvent);
            rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pTimedEvent);
            
            return;
        }
    }
}
LCSM_TIMER_HANDLE podSetTimer( unsigned queue, LCSM_EVENT_INFO * msg, unsigned long periodMs )
{
    LCSM_TIMER_HANDLE hTimer = -1;
    vlPodTimedEvents_t *pTimedEvent;
    msg->dataPtr = NULL;
    msg->dataLength = 0;
    msg->x = periodMs;    
    msg->y = 0;
    rmf_Error err;
    rmf_osal_ThreadId threadId = 0;	 
	
    if(vlg_NumOfPodTimersCreated == 0)
    {
    memset(&stMaxTimers,0,sizeof(vlPodMaxTimers_t));
    }
    if(!vlg_MutexCreated )
    {
    err = rmf_osal_mutexNew(&vlg_PodTimerMutex);
    if(RMF_SUCCESS != err)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","podSetTimer: Error!! Failed to create Mutex \n");
        return hTimer;
    }
    vlg_MutexCreated = 1;
    }
    if(vlg_NumOfPodTimersCreated >= VL_POD_MAX_TIMERS)
    return hTimer;

	rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(vlPodTimedEvents_t),(void **)&pTimedEvent);
    if(pTimedEvent == NULL)
     return hTimer;

    pTimedEvent->TimeElapsed = periodMs;
    pTimedEvent->EventTime = periodMs;
    pTimedEvent->EventCanceled = 0;
    pTimedEvent->Queue = (int)queue;
    memcpy(&pTimedEvent->Event,msg,sizeof(LCSM_EVENT_INFO));

    err = rmf_osal_threadCreate(vlPodTimedEventTask, (void *)pTimedEvent,
        250, 0x1000, &threadId, "vlPodTimedEventTask");

	
    if(RMF_SUCCESS != err)
    {
    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","podSetTimer: Error!! Failed to create vlPodTimedEventTask \n");
    rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pTimedEvent);
    return hTimer;
    }
    rmf_osal_mutexAcquire(vlg_PodTimerMutex);
    
    int ii;
    for(ii = 0; ii< VL_POD_MAX_TIMERS; ii++)
    {
        if(stMaxTimers.pTimer[ii] == NULL)
        {
            stMaxTimers.pTimer[ii] = pTimedEvent;
            break;
        }
    }
    vlg_NumOfPodTimersCreated++;
    
    rmf_osal_mutexRelease(vlg_PodTimerMutex);
    
    hTimer = (LCSM_TIMER_HANDLE)pTimedEvent;
    return hTimer;
}

// cancel (kill) current timer associated with 'hTimer' handle
LCSM_TIMER_STATUS podCancelTimer( LCSM_TIMER_HANDLE hTimer )
{
    vlPodTimedEvents_t *pTimedEvent;
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","podCancelTimer: Entered \n");
    if(hTimer ==0)
        return LCSM_NO_TIMERS;
    
    pTimedEvent = (vlPodTimedEvents_t*)hTimer;
    
    if( 0 == vlPodIsAvailInTimerList(pTimedEvent))
        return LCSM_TIMER_EXPIRED;
    
    rmf_osal_mutexAcquire(vlg_PodTimerMutex);
    pTimedEvent->EventCanceled = 1;
    rmf_osal_mutexRelease(vlg_PodTimerMutex);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","podCancelTimer: SUCCESS \n");
    return LCSM_TIMER_OP_OK;
}

// reset (adjust) by 'addedMs' milliseconds the current timer associated with 'hTimer' handle
LCSM_TIMER_STATUS podResetTimer( LCSM_TIMER_HANDLE hTimer, unsigned long addedMs )
{
    vlPodTimedEvents_t *pTimedEvent;
    if(hTimer ==0)
        return LCSM_NO_TIMERS;

    pTimedEvent = (vlPodTimedEvents_t*)hTimer;
    
    if( 0 == vlPodIsAvailInTimerList(pTimedEvent))
        return LCSM_TIMER_EXPIRED;
    rmf_osal_mutexAcquire(vlg_PodTimerMutex);
    pTimedEvent->TimeElapsed = addedMs;
    rmf_osal_mutexRelease(vlg_PodTimerMutex);
    pTimedEvent->EventTime = addedMs;
    
    return LCSM_TIMER_OP_OK;
}

 

/**********************************************************************************
**
** bGetTLVLength
**
** Determine the length of the payload a given packet based on TLV notation.
**
** Inputs:  pointer to data payload
**          pointer to a variable that will be set to the payload size
**              (this pointer can be NULL if caller does not care about length)
**
** Outputs  Payload size 
**
** Returns  Number of bytes used to encode the length field.  Can be used to 
**           move the packet payload pointer past the TLV field
*/
UCHAR bGetTLVLength (UCHAR *pData, USHORT *pusPayloadLen)
{
    UCHAR  bLenFieldSize = 0; /* Number of bytes used to encode the length field */
    USHORT usPayloadLength = 0;
    
    if ( pData == 0)
        return 0;
        
    // Figure out length field
    if ((*pData & 0x82) == 0x82)
    { // two bytes
        usPayloadLength = ntohs (*(USHORT *)(&pData[1]));
        bLenFieldSize = 3;
    }
    else if ((*pData & 0x81) == 0x81) 
    { // one byte
        usPayloadLength = pData [1];
        bLenFieldSize = 2;
    }
    else { // lower 7 bits
        usPayloadLength = pData[0] & 0x7F; /* Only the lower 7 bits contain the length */
        bLenFieldSize = 1;
    }
//    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","bGetTLVLength\n");
    if ( pusPayloadLen != 0 ) /* Does caller care? */
    {
        /* Not all callers to this function care about length */
        *pusPayloadLen = usPayloadLength;
    }
    
    if ( usPayloadLength != 1 )
        MDEBUG (DPM_OFF, "PayLen = %d, Size = %d (%02x %02x %02x\n", 
                usPayloadLength, bLenFieldSize, pData[0], pData [1], pData[2]);
    
    return bLenFieldSize;
}

/*********************************************************************************/


/* trimming generic implementation(commented) to the specific conversion( just below ) */
//#define ToHex(c) ((c) <= 9) ? ((c) + 48) : (((c) <=15)? ((c) + 55): c)
#define ToHex(c) ((c) <= 9) ? ((c) + 48) : ((c) + 55)

char * vlPrintLongToString(int             nBytes,
                           unsigned long long nLong,
                           int             nStrSize,
                           char      *    pszString)
{
    /* initialize index */
    int ix = 0;

    /* add the prefix */
    strcpy(pszString, ("0x"));

    /* for each char until string limit */
    for(ix = 0; (ix < nBytes) && (ix < sizeof(nLong)) && (ix*2 < (nStrSize-3)); ix++)
    {
        /* get character */
        char c = ((nLong>>((nBytes-(ix+1))*8))&0xFF);

        /* translate character */
        pszString[ix * 2 + 2] = ToHex(((c >> 4) & 0x0F));
        pszString[ix * 2 + 3] = ToHex(((c >> 0) & 0x0F));
    }

    /* terminate string */
    pszString[ix * 2 + 2] = ('\0');

    /* return string */
    return pszString;
}


#ifdef __cplusplus
}
#endif 

