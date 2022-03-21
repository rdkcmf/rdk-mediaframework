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



#ifndef        __UTILS_H
#define        __UTILS_H


#ifdef __cplusplus
extern "C" {
#endif 
#include "poddef.h"
#include "rdk_debug.h"

/* define RTOS for compilation purposes.
 * Only one of these can be #define'd (obviously)!
 */
//#define WIN32   1
//#define PSOS    1
#define LINUX   1   // for Media Receiver

#ifdef WIN32
# include "Windows.h"

#elif  PSOS
# include <psos.h>

#elif  LINUX
# include <cardmanagergentypes.h>
# include "poderror.h"

#endif // WIN32


#ifdef WIN32
    // nothing extra for Windows

#elif  PSOS
        typedef void            VOID; 
        typedef void*            PVOID; 
        typedef UCHAR*            PUCHAR; 
        typedef unsigned long   DWORD;
        typedef int             BOOL;
        typedef int                HANDLE;

# ifndef FALSE
#  define FALSE               0
# endif

# ifndef TRUE
#  define TRUE                1
# endif

# ifndef NULL
#  define NULL                0
# endif    

#elif  LINUX

        typedef unsigned char   UCHAR;
        typedef unsigned short  USHORT;
        typedef unsigned long   ULONG;
        typedef Boolean         BOOL; 
       typedef unsigned long   DWORD;
        typedef void            VOID;
        typedef void *          PVOID;
        typedef unsigned char * PUCHAR;
        
#endif  // LINUX

 


 


#ifndef BIT0    
# define    BIT0    0x01
#endif

#ifndef BIT1    
# define    BIT1    0x02
#endif

#ifndef BIT2    
# define    BIT2    0x04
#endif

#ifndef BIT3    
# define    BIT3    0x08
#endif

#ifndef BIT4    
# define    BIT4    0x10
#endif

#ifndef BIT5    
# define BIT5    0x20
#endif

#ifndef BIT6    
# define    BIT6    0x40
#endif

#ifndef BIT7    
# define    BIT7    0x80
#endif

#ifndef BIT8    
# define    BIT8    0x0100
#endif

/* This is the standard min pthread stack size on our system */
#define DEFAULT_STACK_SIZE ( 16 * 1024)

/************************  Screen color codes *****************************/
// 31 is RED, 1 is bold
#define ERROR_COLOR   "[31;1m"
//#define ERROR_COLOR   "[31:47;1m"

// 34 is blue, 1 is bold
#define WARNING_COLOR "[34;1m"

#define RESET_COLOR   "[0m"

/************************  Debug masks *************************/

#define LOG_BASE_BIT    (28)
#define ALL_LOG_LEVELS  (LOG_FATAL | LOG_NON_FATAL | LOG_WARNING | LOG_DEBUG | LOG_INFO )

/* These Debug Print Masks are used to indicate what type of information
** is to be allowed to be printed.  Then the printing can be enabled or 
** disabled based on each of these settings. */

/* Any error case (or potential errors) should use these masks: */
#define DPM_FATAL     (0x00000001  | (LOG_FATAL     << LOG_BASE_BIT)) /* Any stack related error */
#define DPM_ERROR     (0x00000002  | (LOG_NON_FATAL << LOG_BASE_BIT)) /* Any stack related error */
#define DPM_WARN      (0x00000004  | (LOG_WARNING   << LOG_BASE_BIT)) /* Any stack warning -- 'Potential' error. */

/* The following masks are debug informational only: */
#define DPM_GEN       (0x00000008  | (LOG_DEBUG   << LOG_BASE_BIT)) /* Info related to general processing */
#define DPM_TPDU      (0x00000010  | (LOG_DEBUG   << LOG_BASE_BIT)) /* Info related to TPDU processing */
#define DPM_LPDU      (0x00000020  | (LOG_DEBUG   << LOG_BASE_BIT)) /* Info related to LPDU processing */
#define DPM_RW        (0x00000040  | (LOG_DEBUG   << LOG_BASE_BIT)) /* Info related to low level link read/write processing */
#define DPM_SS        (0x00000080  | (LOG_DEBUG   << LOG_BASE_BIT)) /* Info related to session processing */
#define DPM_MOD       (0x00000100  | (LOG_DEBUG   << LOG_BASE_BIT)) /* Info related to module processing */
#define DPM_APPLI     (0x00000400  | (LOG_DEBUG   << LOG_BASE_BIT)) /* General Application printing */
#define DPM_I2C       (0x00100000  | (LOG_DEBUG   << LOG_BASE_BIT)) /* General Application printing */

/* For resource manager applications: */
#define DPM_APPINFO   (0x00000400  | (LOG_DEBUG   << LOG_BASE_BIT)) /* General Application printing */
#define DPM_CA        (0x00000800  | (LOG_DEBUG   << LOG_BASE_BIT)) /* General Application printing */
#define DPM_CPROT     (0x00001000  | (LOG_DEBUG   << LOG_BASE_BIT)) /* General Application printing */
#define DPM_DIAG      (0x00002000  | (LOG_DEBUG   << LOG_BASE_BIT)) /* General Application printing */
#define DPM_FEATURE   (0x00004000  | (LOG_DEBUG   << LOG_BASE_BIT)) /* General Application printing */
#define DPM_HOMING    (0x00008000  | (LOG_DEBUG   << LOG_BASE_BIT)) /* General Application printing */
#define DPM_HOST      (0x00010000  | (LOG_DEBUG   << LOG_BASE_BIT)) /* General Application printing */
#define DPM_IPPV      (0x00020000  | (LOG_DEBUG   << LOG_BASE_BIT)) /* General Application printing */
#define DPM_LOWSPD    (0x00040000  | (LOG_DEBUG   << LOG_BASE_BIT)) /* General Application printing */
#define DPM_MMI       (0x00080000  | (LOG_DEBUG   << LOG_BASE_BIT)) /* General Application printing */
#define DPM_RSMGR     (0x00200000  | (LOG_DEBUG   << LOG_BASE_BIT)) /* General Application printing */
#define DPM_SAS       (0x00400000  | (LOG_DEBUG   << LOG_BASE_BIT)) /* General Application printing */
#define DPM_SYSTEM    (0x00800000  | (LOG_DEBUG   << LOG_BASE_BIT)) /* General Application printing */
#define DPM_SYSTIME   (0x01000000  | (LOG_DEBUG   << LOG_BASE_BIT)) /* General Application printing */
#define DPM_XCHAN     (0x02000000  | (LOG_DEBUG   << LOG_BASE_BIT)) /* General Application printing */

#define DPM_TEMP      (0x04000000  | (LOG_DEBUG   << LOG_BASE_BIT)) /* VERY temp prints related to module processing */
#define DPM_RESERVED  (0xF8000000) /* These bits are reserved for the logger */

#define DPM_ALWAYS    (-1)                                          /* All bits set -- this is always printed */
#define DPM_OFF       (0x00000000) /* No Print -- Keep line of code but don't print it.  Usually used in conjunction with DPM_TEMP */

/* This global is set to the above OR'ed bitmasked to indicate the type 
** of print statements that are currently enabled. */

/* This macro is called in one place, during initialization, to set the default
** debug print mask value.  It can be changed during runtime to change
** which debug statements are printed. */

/* CHANGE THIS BEFORE CHECKIN!!! */
//#define DEFAULT_DEBUG_PRINT_MASK (DPM_ERROR | DPM_WARN | DPM_TPDU | DPM_SS | DPM_RSMGR | DPM_APPLI | DPM_APPINFO  | DPM_MMI | DPM_SYSTIME | DPM_SAS | DPM_HOMING) //Hannah
#define DEFAULT_DEBUG_PRINT_MASK (DPM_ERROR | DPM_WARN | DPM_MOD )
//#define DEFAULT_DEBUG_PRINT_MASK (DPM_ERROR | DPM_WARN | DPM_TPDU | DPM_LPDU | DPM_MOD)


extern LCSM_DEVICE_HANDLE hLCSMdev_pod;

/* Function to send print strings to the log file */
int SendLogMsg( int level, char* format, ... );

ULONG GetTimeSinceBootMs (unsigned char bPrintAlso); // Get time since boot in milliseconds
ULONG ElapsedTimeMs (ULONG baseTime);  // elasped time since baseTime in milliseconds


#ifndef _DEBUG
//# define _DEBUG
#endif

#ifdef WIN32

# define DBGPRINT(a)            OutputDebugString(a)
# define DBGPRINT_POD(a)        OutputDebugString(a)
# define DBGPRINT_APPLI(a)        {OutputDebugString("\n\r********** APPLI*************\n\r");OutputDebugString(a);}
# define NEWLN()                OutputDebugString("\n\r")

#elif  PSOS
                extern void TRACE (char * p);
# define DBGPRINT(a)            TRACE(a)        
# define DBGPRINT_POD(a)        TRACE(a)
# define DBGPRINT_APPLI(a)        TRACE(a)    
# define NEWLN()                TRACE("\n\r")

#elif  LINUX

# define    POD_DEBUG   // enables debug output, among other things

#if (1) /* Enable/disable Prefix of all MDEBUG prints with timestamp */
#define PRINT_DEBUG_TIMESTAMP       TRUE
#else
#define PRINT_DEBUG_TIMESTAMP       FALSE
#endif

#if (1) /* Enable/disable Prefix of all MDEBUG prints with the name of the function they are called from */
#define DEBUG_PRINT_FUNCTION_NAME       PDEBUG (":%s: ", __FUNCTION__);
#else
#define DEBUG_PRINT_FUNCTION_NAME       
#endif

//# undef PDEBUG

# include <stdio.h>

/* Special print function depending on if we are in user vs kernel space */
#  ifdef __KERNEL__
#   define PDEBUG(fmt, args...) printk(fmt, ## args)
#  else
#   define PDEBUG(fmt, args...) RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", fmt, ## args)
#  endif // __KERNEL__

# ifdef POD_DEBUG
#  define DBGPRINT(a)            PDEBUG( a )        
#  define DBGPRINT_APPLI(a)        PDEBUG( a )    
#  define SDEBUG(fmt, args...)  PDEBUG(fmt, ## args)


/* This 'masked' print macro will only print if the associated
** mask is set.  (All other debug macros should ultimately be
** replaced with this macro. */
#  define MDEBUG(mask, fmt, args...)                                                 \
        if ( DEFAULT_DEBUG_PRINT_MASK & mask & (~(ALL_LOG_LEVELS << LOG_BASE_BIT)) )   \
        {                                                                            \
                                                                                     \
            GetTimeSinceBootMs (PRINT_DEBUG_TIMESTAMP);                              \
            DEBUG_PRINT_FUNCTION_NAME                                                \
            if ( (mask & ( DPM_ERROR | DPM_FATAL )) && ( mask != DPM_ALWAYS) )       \
                PDEBUG (ERROR_COLOR);                                                \
            PDEBUG ( fmt, ## args );                                                 \
            SendLogMsg ( ((mask & ALL_LOG_LEVELS) >> LOG_BASE_BIT ), fmt, ## args ); \
            if ( mask & ( DPM_ERROR | DPM_FATAL ) )                                  \
                PDEBUG (RESET_COLOR);                                                \
                                                                                     \
        }

/* This special case is for dumping bytes in a loop -- don't print timestamp 
** or function within each loop iteration. */
#  define MDEBUG2(mask, fmt, args...)         \
        if ( DEFAULT_DEBUG_PRINT_MASK & mask & (~(ALL_LOG_LEVELS << LOG_BASE_BIT)) )       \
        {                                    \
            PDEBUG ( fmt, ## args );          \
            SendLogMsg ( ((mask & ALL_LOG_LEVELS) >> LOG_BASE_BIT ), fmt, ## args );    \
        }
            

//  DBGPRINT_POD is a low level packet printer and we usually
//  do not want it on.
#   ifdef _DEBUG
#       define DBGPRINT_POD(a)        PDEBUG( a )
#   else
#       define DBGPRINT_POD(a)
#   endif

# else  // ! POD_DEBUG -- disable all printing 
#  define DBGPRINT(a)
#  define DBGPRINT_APPLI(a)
#  define DBGPRINT_POD(a)
#  define SDEBUG(fmt, args...)
#  define MDEBUG(mask, fmt, args...)
#  define MDEBUG2(mask, fmt, args...)

# endif // POD_DEBUG

/****************************  End debug stuff ***********************/

#ifndef __KERNEL__      // user-space stuff

    /***  POD stack-related  ***/

// global /dev/cimax file descriptor (if >= 0) or device not open flag (if < 0)
extern int  Cimax;      // declared in utils.c

// global user-space handle to KMS
//extern LCSM_DEVICE_HANDLE hLCSMdev_pod; // declared in main.c, used in user/

// reverse from rest of MR, but specified in POD Driver User's Guide
#define VL_MCARD_FAILURE    ( 0 )
#define VL_MCARD_SUCCESS    ( 1 )

/* The following definitions apply to all functions that return general
** succss/fail status */
#define    FAIL    0
#define SUCCESS    1

/* The following definitions apply to all pod read/write status related functions. */
#define PODST_RW_FAIL               0
#define PODST_RW_READ_DATA_AVAIL    1
#define PODST_RW_SUCCESS            2

/* Error types are USHORT.  See defines for valid error values */
#define ERROR_T  USHORT
#define STATUS_T USHORT

int  openCimax(  char * requestor );
void closeCimax( char * reuqestor );
int  REALLYopenCimax(  char * requestor );
void REALLYcloseCimax( char * reuqestor );

#define VL_POD_MAX_TIMERS  20
typedef struct vlPodTimedEvents_s
{
    LCSM_EVENT_INFO Event;
    unsigned long EventTime;
    int TimeElapsed;
    int Queue;
    unsigned char EventCanceled;
}vlPodTimedEvents_t;

typedef struct vlPodTimers
{
    vlPodTimedEvents_t *pTimer[VL_POD_MAX_TIMERS];
}vlPodMaxTimers_t;
    /***  POD stack-related  ***/

#define MIN_APDU_HEADER_BYTES   ( 3/*tag*/ +             1/*length*/ )  // 4
#define MAX_APDU_HEADER_BYTES   ( 3/*tag*/ + 1/*flag*/ + 2/*length*/ )  // 6

/*! Construct an APDU header to send to the POD
 
    Build the header for an APDU that a Host resource wants to send to the POD.
    An APDU header contains a 3-byte tag (resource-dependent and specified in
    SCTE-28 2003), an optional 1 byte flag (for 'dataLen's > 255) and a 1-2
    byte length.
    @pre none
    @param pApdu [IN] A buffer large enough to hold the specified optional
    'dataLen' APDU bytes + MAX_APDU_HEADER_BYTES. The header is stuffed into
    the first n bytes of the APDU buffer, where
             MIN_APDU_HEADER_BYTES <= n <= MAX_APDU_HEADER_BYTES.
    @param apduLen [OUT] The size of the final APDU. This includes the
    specified optional 'dataLen' APDU bytes + the size of the APDU header.
    @param tag [IN] The resource-specific APDU tag - gleaned from SCTE-28 2003.
    NB: it is *NOT* verified to be correct by this function
    @param dataLen [IN] The count of bytes in the optional APDU data payload.
    'dataLen' must be >= 0 and < 65535
    @return A pointer to the first location in the APDU buffer where optional
    data would be stored (immediately following the header) or NULL if something
    went tragically wrong in the function.
    @version 2/11/04 - prototype
 */
uint8 * buildApduHeader( uint8 * pApdu, uint16 * apduLen, uint32 tag, uint16 dataLen );

/*! Construct an APDU to send to the POD
 
    Build an APDU that a Host resource wants to send to the POD. The caller
    can pass in a pointer to the APDU data payload or, optionally, take care
    of the copying itself.
    @pre none
    @param pApdu [IN] A buffer large enough to hold the specified optional
    'dataLen' APDU bytes + MAX_APDU_HEADER_BYTES.
    @param apduLen [OUT] The size of the completed APDU. This includes the
    specified optional 'dataLen' APDU bytes + the size of the APDU header.
    @param tag [IN] The APDU tag - gleaned from SCTE-28 2003.
    NB: it is *NOT* verified to be correct by this function
    @param dataLen [IN] The count of bytes in the optional APDU data payload.
    'dataLen' must be >= 0 and < 65535
    @param data [IN] An optional pointer to the data payload to copy into the
    APDU buffer. NB: If this is NULL and 'dataLen' > 0, then it is the
    responsibility of the caller to copy the data into the APDU buffer.
    @return A pointer to the first location in the APDU buffer where optional
    data would be stored (immediately following the header) or NULL if something
    went tragically wrong in the function.
    @version 2/11/04 - prototype
 */
uint8 * buildApdu( uint8 * pApdu, uint16 * apduLen, uint32 tag, uint16 dataLen, uint8 * data );

UCHAR bGetTLVLength (UCHAR *pData, USHORT *pusPayloadLen);

/*! Setup to receive a timer event

    Use KMS to send a message to a thread's queue after a fixed period of time.
    This is usually used to detect time-outs. For example, a thread begins an
    async operation and expects to receive a completion indication within a
    certain amount of time. The thread can arrange to have an event sent to
    it after the maximum expected time of the completion indication. Then,
    if it receives the timer event FIRST, it can assume the async operation
    timed-out.
    NB: only as granular as HZ (10ms?)
    @pre none
    @param queue [IN] The ID of the message queue to send to.
    @param msg [IN] A pointer to the event buffer to send.
    MAKE SURE dataLength AND dataPtr FIELDS ARE CLEARED IF NO DATA IS SENT!
    @param periodMs [IN] The amount of time, in milliseconds, to wait before
    sending the msg.
    @return A handle to the timer so that it can be cancelled or reset before it
    expires
    @version 4/26/04 - prototype
    @see also podCancelTimer, podResetTimer
 */
LCSM_TIMER_HANDLE podSetTimer( unsigned queue, LCSM_EVENT_INFO * msg, unsigned long periodMs );

/*! Cancel (kill) current timer event

    Kill a current timer event. This is usually used to kill a timer that was
    set while waiting for an event, after the event has occurred (i.e. the
    time-out mechanism is no longer needed).
    NB: only as granular as HZ (10ms?)
    @pre podSetTimer called successfully
    @param hTimer [IN] The handle of the timer as returned by podSetTimer
    @return Status of the cancel operation (0 - OK)
    @version 4/26/04 - prototype
    @see also podSetTimer, podResetTimer
 */
LCSM_TIMER_STATUS podCancelTimer( LCSM_TIMER_HANDLE hTimer );

/*! Reset (adjust) a current timer event by a fixed amount of time

    Adjust a current timer event. This is usually used to extend a timer
    that was set while waiting for an event, after a related event has
    occurred. The passed time is added onto the timer's current time,
    effectively extending the timer event by the passed time.
    NB: only as granular as HZ (10ms?)
    @pre podSetTimer called successfully
    @param hTimer [IN] The handle of the timer as returned by podSetTimer
    @param addedMs [IN] The amount of milliseconds to add onto (i.e. extend)
    the timer event.
    @return Status of the reset operation (0 - OK)
    @version 4/26/04 - prototype
    @see also podSetTimer, podCancelTimer
 */
LCSM_TIMER_STATUS podResetTimer( LCSM_TIMER_HANDLE hTimer, unsigned long addedMs );

#endif // __KERNEL__

#else   // if no RTOS is #define'd

# define DBGPRINT(a)
# define DBGPRINT_POD(a)
# define DBGPRINT_APPLI(a)    
# define NEWLN()            

#endif  // WIN32

/*************************************************************************/

//Collect data about POD operations 
typedef struct
{
    struct
    {
        struct 
        {
            USHORT usReadByte;
            USHORT usWriteByte;
        } sInfo; //  Collect general info (non-error) 
        struct 
        {
            USHORT usReadByte;
            USHORT usWriteByte;
        } sError; // Collect error metrics 
        
    } sI2C; // Collect I2C metrics 
    
    struct
    {
        struct 
        {
            USHORT usAttemptReadPkt;
            USHORT usReadPkt;
            USHORT usWritePkt;
            USHORT usWritePk_NotFR;
            USHORT usWriteFoundRead;
        } sInfo; // Collect general info (non-error) 
        struct 
        {
            USHORT usReadPkt;
            USHORT usWritePkt;
            USHORT usWritePk_NotFR;
        } sError; // Collect error metrics 
        
    } sTrans; // Collect Transport metrics 

} sPodDebugMetrics_t;

extern sPodDebugMetrics_t sPodDebugMetrics;

/*************************************************************************/


#ifdef __cplusplus
}
#endif 

#endif  // of __UTILS_H

