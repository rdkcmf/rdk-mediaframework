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
#include <stdlib.h>
#include <memory.h>
#include <time.h>
#include "utils.h"

#include "transport.h"
#include "session.h"
#include "lcsm_log.h"
#include "podlowapi.h"
#include <string.h>
#ifdef  LINUX
//# include "pthreadLibrary.h"
//# include "podmod.h"
# include "poddrv.h"
#include "rmf_osal_mem.h"
#ifdef VL_USE_MEMORY_POOL_FOR_POD_WRITE
#include "rmfStaticMemoryPool.h"
#endif
#define __MTAG__ VL_CARD_MANAGER

#if USE_SYSRES_MLT
#include "mlt_malloc.h"
#endif


#endif // LINUX

#ifdef __cplusplus
extern "C" {
#endif 

// Add file name and revision tag to object code
#define REVISION  __FILE__ " $Revision: 1.14 $"
/*@unused@*/
static char *podlowapi_tag = REVISION;

extern ULONG TaskID;



//************************************************************************************
//                    Memory management
//************************************************************************************


#define TRACE_MEM
#ifdef VL_USE_MEMORY_POOL_FOR_POD_WRITE

//#define VL_POD_MEM_POOL_TYPE_STATIC
//#define VL_POD_MEM_POOL_TYPE_HEAP
#define VL_POD_MEM_POOL_TYPE_PROTECTED

#ifdef VL_POD_MEM_POOL_TYPE_STATIC
#define VL_POD_MEM_POOL_TYPE    RMF_MEM_POOL_TYPE_STATIC
#endif

#ifdef VL_POD_MEM_POOL_TYPE_HEAP
#define VL_POD_MEM_POOL_TYPE    RMF_MEM_POOL_TYPE_HEAP
#endif

#ifdef VL_POD_MEM_POOL_TYPE_PROTECTED
#define VL_POD_MEM_POOL_TYPE    RMF_MEM_POOL_TYPE_PROTECTED
#endif

static RMF_FIXED_H124_MEM_POOL * vlg_pPodMemPool = NULL;

#ifdef VL_POD_MEM_POOL_TYPE_STATIC
    static VL_FIXED_H124_MEM_POOL vlg_staticMemPool;
#endif // not (VL_MEM_POOL_TYPE_STATIC == VL_POD_MEM_POOL_TYPE)

#endif // VL_USE_MEMORY_POOL_FOR_POD_WRITE

/****************************************************************************************
Name:             vlCardManager_PodMemFree
type:            function 
Description:     Free the allocated memory 
                MR IMPLEMENTATION NOTE: Assumes memory freed is regular target RAM (not
                    in the POD) previously allocated via PODMemAllocate

In:             Nothing

Out:            UCHAR* pU: Pointer on the allocated area

Return value:    void
*****************************************************************************************/
void vlCardManager_PodMemFree(UCHAR * pU)
{
#ifdef VL_USE_MEMORY_POOL_FOR_POD_WRITE
    if(RMF_MEM_POOL_RESULT_NOT_FROM_THIS_POOL == rmfFixedH124MemPoolReleaseToPool(vlg_pPodMemPool, pU))
    {
        // The block did not belong to the pool. Try releasing the block to the process heap.
        // This may crash due to invalid pointer, double-free, heap corruption, etc.
        free( pU);
    }
#else // not VL_USE_MEMORY_POOL_FOR_POD_WRITE
    PODMemFree(pU);
#endif // not VL_USE_MEMORY_POOL_FOR_POD_WRITE
}

/****************************************************************************************
Name:             vlCardManager_PodMemAllocate
type:            function 
Description:     Allocate memory
                MR IMPLEMENTATION NOTE: Assumes memory to be allocated is regular target
                    RAM (not in the POD) that can be freed via PODMemFree

In:             Size to be allocated

Out:            Nothing

Return value:    UCHAR* pU: Pointer on the allocated area
*****************************************************************************************/
unsigned char * vlCardManager_PodMemAllocate(USHORT size)
{
#ifdef VL_USE_MEMORY_POOL_FOR_POD_WRITE
    if(NULL == vlg_pPodMemPool)
    {
        #ifdef VL_POD_MEM_POOL_TYPE_STATIC
            rmfFixedH124MemPoolInit(&vlg_staticMemPool, "VL_POD_MEM_POOL");
            vlg_pPodMemPool = &vlg_staticMemPool;
        #else
            rmfFixedH124MemPoolCreate(&vlg_pPodMemPool, "VL_POD_MEM_POOL", VL_POD_MEM_POOL_TYPE);
        #endif
    }
    void * pBuf = NULL;
    rmfFixedH124MemPoolAllocFromPool(vlg_pPodMemPool, size, &pBuf);
    if(NULL == pBuf)
    {
        // The memory pool appears to be exausted OR unable to accomodate the buffer size.
        // Resort to process heap instead of returning NULL.
        // This may crash due to heap corruption, etc.
        return (unsigned char *)malloc(size);
    }
    return (unsigned char *)pBuf;
#else // not VL_USE_MEMORY_POOL_FOR_POD_WRITE
    return (unsigned char *)PODMemAllocate(size);
#endif // not VL_USE_MEMORY_POOL_FOR_POD_WRITE
}



/****************************************************************************************
Name:             PODMemFree
type:            function 
Description:     Free the allocated memory 
                MR IMPLEMENTATION NOTE: Assumes memory freed is regular target RAM (not
                    in the POD) previously allocated via PODMemAllocate

In:             Nothing

Out:            UCHAR* pU: Pointer on the allocated area

Return value:    void
*****************************************************************************************/
void PODMemFree(UCHAR * pU)
{
#ifdef WIN32
	rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pU);
    pU = NULL;

#elif  PSOS
    ULONG RetCode;

# ifdef TRACE_MEM
    PODFree(pU);
# endif    

    RetCode = rn_retseg(0 , (void *)(pU));
    if ( ! RetCode )
    {    
        return;
    }
    else
    {
        return ;
    }

#elif  LINUX
    // avoid free'ing nothing (even though free checks for that)
//    MDEBUG(DPM_TEMP, " pU=0x%x\n", (unsigned int) pU );
    if( pU )
		rmf_osal_memFreeP(RMF_OSAL_MEM_POD, pU);
    return;

#else   // RTOS not defined - error
# error  __FILE__ __LINE__ RTOS must be defined in utils.h

#endif  // WIN32
}

/****************************************************************************************
Name:             PODMemAllocate
type:            function 
Description:     Allocate memory
                MR IMPLEMENTATION NOTE: Assumes memory to be allocated is regular target
                    RAM (not in the POD) that can be freed via PODMemFree

In:             Size to be allocated

Out:            Nothing

Return value:    UCHAR* pU: Pointer on the allocated area
*****************************************************************************************/
unsigned char * PODMemAllocate(USHORT size)
{

#ifdef WIN32
    UCHAR* p;
	rmf_osal_memAllocP(RMF_OSAL_MEM_POD, size,(void **)&p);
    return p;

#elif  PSOS
    ULONG        RetCode;
    void *        pAddr;

    RetCode =  rn_getseg( 0, size, RN_NOWAIT, 0, (void**)(&pAddr) );

    if ( ! RetCode )
    {
# ifdef TRACE_MEM
        PODMalloc(pAddr,size);
# endif
        return ((UCHAR*)pAddr);
    }
    else
    {
        return NULL;
    }

#elif  LINUX
    // return a pointer to the beginning of a cleared memory block

    UCHAR *puc;
    
			rmf_osal_memAllocP(RMF_OSAL_MEM_POD, (size_t) size ,(void **)&puc);
				memset (puc, 0, (size_t) size);
    
    if ( puc == NULL )
    {
        MDEBUG (DPM_ERROR, "ERROR: Can't alloc size=0x%x\n", size );
    }
    
    return puc;

//    UCHAR * chunk = (UCHAR *) malloc( (size_t) size );
//    MDEBUG (DPM_GEN, " chunk=0x%x size=0x%x\n", (unsigned int) chunk, size );
//    return chunk;

#else
# error  __FILE__ __LINE__ RTOS must be defined in utils.h
    return NULL;

#endif  // WIN32
}

/****************************************************************************************
Name:             PODMemCopy
type:            function 
Description:     Copy Source buffer to Destination buffer
                MR IMPLEMENTATION NOTE: Assumes memory source and destination addresses
                    are in regular target RAM (not in the POD); i.e. always "case 1"

In:                PVOID Destination    : Destination address
                PVOID Source        : Source Address 
                USHORT Length        : Length to be copied
                
Out:            Nothing

Return value:    void
*****************************************************************************************/

void PODMemCopy( PUCHAR Destination, PUCHAR Source, USHORT Length)
{

#ifdef WIN32
//    memcpy(Destination,Source,Length);
    memcpy(Destination,Source,Length);
#elif  PSOS
    USHORT i;
    UCHAR* pD = (UCHAR*)Destination;
    UCHAR* pS = (UCHAR*)Source;

    for ( i = 0; i<Length; i++)
    {
        (pD[i]) = (UCHAR)(pS[i]);
    }

#elif  LINUX
    /* 4 possible cases of memory copy:
     *    1. from regular memory to regular memory
     *    2. from regular memory to POD
     *    3. from POD to regular memory
     *    4. from POD to POD
     * With the help of mmap(), all 4 cases handled by memcpy()
     */

//    MDEBUG (DPM_GEN, "( 0x%x, 0x%x, %d )\n", (int) Destination, (int) Source, (int) Length );

    // avoid undefined results based on bogus args
    if( Destination  &&  Source  &&  Length )
    {
//        memcpy( Destination, Source, (size_t) Length );
      memcpy( Destination, Source, (size_t) Length );
    }
    else
    {
        MDEBUG (DPM_ERROR, "ERROR:  return FALSE\r\n");
    }
    
//    MDEBUG (DPM_GEN, "after PODMemCopy()\n");

#else  // RTOS not defined - error
# error  __FILE__ __LINE__ RTOS must be defined in utils.h

#endif  // WIN32
}



//************************************************************************************
//
//
//                    MISCELLANEOUS SYSTEM FUNCTIONS...
//
//
//************************************************************************************



/****************************************************************************************
Name:             PODSleep
type:            function 
Description:     Calling task sleeps during TimeMs ms

In:             USHORT TimeMs    : Time to sleep

Out:            Nothing

Return value:    void
*****************************************************************************************/

void PODSleep(USHORT TimeMs)
{
#ifdef WIN32
    Sleep(TimeMs);

#elif  PSOS
    // SHHHHH, no trace of what we do here
    // Macros are taken from PSOS files
# define TICK_RATE          100                   /* x ticks per second */
# define TIMER_MSEC(t)      ( ((t) * TICK_RATE / 1000) + 1 )

    tm_wkafter( TIMER_MSEC(TimeMs));

# undef TICK_RATE          
# undef TIMER_MSEC

#elif  LINUX
    // nb: can't be any more granular than 1/HZ which is kernel's timer interval
    if( TimeMs )
    {
        struct timespec req, rem;       // requested, remaining
        req.tv_sec = TimeMs / 1000;
        req.tv_nsec = ( (long)TimeMs - ( req.tv_sec * 1000 ) ) * 1000 * 1000;
//        MDEBUG (DPM_TEMP, " TimeMs=%d  req.tv_sec=%d req.tv_nsec=%d\n",
//                (int) TimeMs, (int) req.tv_sec, (int) req.tv_nsec );

        // until nanosleep returns 0 (indicating requested time has elasped)
        while( nanosleep( &req, &rem ) )
        {
    
    /* commented by Hannah
            if( errno != EINTR )
                break;
    */        
            // received an interrupt which prematurely cancelled sleeping.
            // setup time left to sleep (returned in 'rem' by nanosleep)
            req.tv_sec = rem.tv_sec;
            req.tv_nsec = rem.tv_nsec;
        }
    }

#else
# error  __FILE__ __LINE__ RTOS must be defined in utils.h

#endif  // WIN32
}




//************************************************************************************
//
//                    GLOBAL STRUCTURES, VARIABLES AND FUNCTIONS 
//
//                    FOR MEMORY ALLOCATIONS CHECKING 
//
//************************************************************************************

#ifdef TRACE_MEM

typedef struct
{
    PVOID    Addr;
    USHORT    Size;
} PODAllocation;

#define MAX_ALLOCATIONS 300

PODAllocation    Alloc[MAX_ALLOCATIONS];

USHORT NumberOfAllocations = 0;


void PODMalloc(UCHAR* Ret, USHORT Size)
{
    if(Ret && ( NumberOfAllocations < MAX_ALLOCATIONS ))
    {
        MDEBUG (DPM_OFF, "%d : Allocate %d Bytes at 0x%08X  \r\n",NumberOfAllocations, Size,(unsigned int)Ret);
        Alloc[NumberOfAllocations].Size = Size;
        Alloc[NumberOfAllocations].Addr = Ret;
        NumberOfAllocations++;
    }
    
    else
    {
        MDEBUG (DPM_ERROR, "ERROR: Allocation Failed\r\n");
    }
}


void PODFree(UCHAR* pDest)
{
    unsigned short i,j;

    for ( i = 0; i < NumberOfAllocations; i++)
    {
        if (Alloc[i].Addr == pDest)
        {
            MDEBUG (DPM_TEMP, "%d : free at 0x%08X\r\n",i, (unsigned int)pDest);

            Alloc[i].Addr = NULL;
            Alloc[i].Size = 0;
            
            /* Cleanup by shifting all allocations down in the table so there
            ** are no empty spots */
            for(j=i; j<(NumberOfAllocations -1); j++)
            {
                Alloc[j].Addr = Alloc[j+1].Addr;
                Alloc[j].Size = Alloc[j+1].Size;
            }
            NumberOfAllocations--;
            return;
        }
    }
    MDEBUG (DPM_ERROR, "ERROR: cannot free memory 0x%08X\r\n", (unsigned int)pDest);
}

/* Only used for debug after we believe that all allocs should have been freed */
void PODCheck()
{
    unsigned short    i;
    BOOL        Error = FALSE; 

    MDEBUG (DPM_OFF, "-------------------------------------------------\r\n");
    MDEBUG (DPM_OFF, "Checking memory allocations   \r\n");
    for ( i = 0; i < MAX_ALLOCATIONS; i++)
    {
        if ((Alloc[i].Addr != NULL) )
        {
            MDEBUG (DPM_ERROR, "ERROR: Forget to Free memory  Address = 0x%08X    Size = %d\r\n",(unsigned int)Alloc[i].Addr,Alloc[i].Size);
            Error = TRUE;
        }
    }
    if (! Error)     
        MDEBUG (DPM_OFF, "Wonderful ! ! No Error \r\n");

    MDEBUG (DPM_OFF, "-------------------------------------------------\r\n");
}





#endif






#ifdef __cplusplus
}
#endif 


