/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2014 RDK Management
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


/**
 * @file rmf_osal_mem.h 
 * @brief Memory API prototypes
 */

/**
 * @defgroup OSAL_MEMORY OSAL Memory
 * 
 * OSAL Memory is intendent to perform any low-level initialization required to set up the OS-level
 * memory subsystem. The memory subsystem should be initialized following the environment subsystem
 * to allow it to be configured by the environment. However, access to the memory APIs prior to
 * initialization should be allowed.
 * 
 * Standard C malloc() and free() operations have little knowledge of hardware specifics,
 * OSAL provides the same and extended functions for the user to manage dynamic memory from the heap
 * These functions should be used exclusively.
 *
 * RMF OSAL memory API provides the basic support for allocating and de-allocating memory.
 * 
 * @ingroup OSAL
 *
 * @defgroup OSAL_MEMORY_API OSAL Memory API list
 * RMF OSAL Memory module defines interfaces for managing memory.
 * @ingroup OSAL_MEMORY
 *
 * @addtogroup OSAL_MEMORY_TYPES OSAL Memory Data Types
 * Describe the details about Data Structure used by OSAL Memory.
 * @ingroup OSAL_MEMORY
 */

#if !defined(_RMF_OSAL_MEM_H)
#define _RMF_OSAL_MEM_H

#include <rmf_osal_types.h>	/* Resolve basic type references. */
#include <rmf_osal_error.h>	/* Resolve basic type references. */
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @addtogroup OSAL_MEMORY_TYPES
 * @{
 */

/**
 * @enum rmf_osal_MemColor
 * @brief rmf_osal_MemColor specifies the intended use of allocated memory. Color is a
 * loose designation of the type or intended use of the memory. Color may or
 * may not be used depending on implementation. For example, Color may
 * map to a distinct heap. For example, the uses of the implementation are to 
 * trace memory usen or to indicate distinct allocation heaps
 */
typedef enum
{
    RMF_OSAL_MEM_GENERAL = 0, /**< Generic color/type. */
    RMF_OSAL_MEM_SYSTEM = -1, /**< For GetFreeSize and GetLargestFree only */
    RMF_OSAL_MEM_TEMP, /**< Temporary memory. */
    RMF_OSAL_MEM_JVM, /**< JVM (includes system and java heaps) */
    RMF_OSAL_MEM_GFX, /**< Generic graphics memory. */
    RMF_OSAL_MEM_GFX_LL, /**< Low-level graphics memory (e.g., DFB). */
    RMF_OSAL_MEM_MUTEX, /**< Mutex related memory. */
    RMF_OSAL_MEM_COND, /**< Allocated by condition variable. */
    RMF_OSAL_MEM_THREAD, /**< Threads. */
    RMF_OSAL_MEM_FILE, /**< Generic file system memory. */
    RMF_OSAL_MEM_FILE_CAROUSEL, /**< Data and Object carousel. */
    RMF_OSAL_MEM_MEDIA, /**< Generic media. */
    RMF_OSAL_MEM_UTIL, /**< Util. */
    RMF_OSAL_MEM_EVENT, /**< Events. */
    RMF_OSAL_MEM_FILTER, /**< Section filtering memory. */
    RMF_OSAL_MEM_POD, /**< CableCard (aka POD) memory. */
    RMF_OSAL_MEM_SI_CACHE_INB, /**< Service Information memory. */
    RMF_OSAL_MEM_SI_INB, /**< Service Information memory. */
    RMF_OSAL_MEM_SI_OOB, /**< Service Information memory. */
    RMF_OSAL_MEM_SI_CACHE_OOB, /**< Service Information memory. */
    RMF_OSAL_MEM_TEST, /**< Memory used during testing. */
    RMF_OSAL_MEM_NET, /**< Networking memory. */
    RMF_OSAL_MEM_CC, /**< Closed captioning module */
    RMF_OSAL_MEM_DVR, /**< DVR manager module */
    RMF_OSAL_MEM_SND, /**< Sound Support */
    RMF_OSAL_MEM_FP, /**< Front Panel module */
    RMF_OSAL_MEM_PORT, /**< Port-specific memory */
    RMF_OSAL_MEM_STORAGE, /**< Storage manager module */
    RMF_OSAL_MEM_VBI, /**< Vbi filtering memory. */
    RMF_OSAL_MEM_HN, /**< Home networking. */
    RMF_OSAL_MEM_IPPV, /**< IPPV stack memory. */    
    RMF_OSAL_MEM_IPC,
    /* Add new ones here as necessary.
     * Generally at least one for every logical code module.
     * Potentially with additional sub-areas per code module.
     * May be a good idea to keep under 32 total (to allow use
     * in bitfield).
     */
    RMF_OSAL_PORT_MEM_COLORS_COMMA, /**< Port-specific additional memory colors */
    RMF_OSAL_MEM_NCOLORS
} rmf_osal_MemColor;

/** @} */

//#define DEBUG_ALLOC

/*
 * Memory API prototypes:
 */

/**
 * @see rmf_osal_memAllocPProf()
 */
#ifdef RMF_OSAL_FEATURE_MEM_PROF
#define rmf_osal_memAllocP(c,s,m)		rmf_osal_memAllocPProf(c, s, m, __FILE__, __LINE__)
#else
#define rmf_osal_memAllocP				rmf_osal_memAllocPGen
#endif

/**
 * @addtogroup OSAL_MEMORY_API
 * @{
 */

#ifndef DEBUG_ALLOC
rmf_Error rmf_osal_memAllocPGen(rmf_osal_MemColor color, uint32_t size, void **memory);
#else
rmf_Error rmf_osal_memAllocPGen_track(rmf_osal_MemColor color, uint32_t size, void **memory, const char* file, int line);
#define rmf_osal_memAllocPGen(c, s, mp) rmf_osal_memAllocPGen_track(c, s, mp, __FILE__, __LINE__)
#endif


/**
 * @see rmf_osal_memFreePProf()
 */
#ifdef RMF_OSAL_FEATURE_MEM_PROF
#define rmf_osal_memFreeP(c,m)			rmf_osal_memFreePProf(c, m, __FILE__, __LINE__)
#else
#define rmf_osal_memFreeP				rmf_osal_memFreePGen
#endif
rmf_Error rmf_osal_memFreePGen(rmf_osal_MemColor color, void *memory);

/**
 * @see rmf_osal_memReallocPProf()
 */
#ifdef RMF_OSAL_FEATURE_MEM_PROF
#define rmf_osal_memReallocP(c,s,m)	rmf_osal_memReallocPProf(c, s, m, __FILE__, __LINE__)
#else
#define rmf_osal_memReallocP			rmf_osal_memReallocPGen
#endif

#ifndef DEBUG_ALLOC
rmf_Error
        rmf_osal_memReallocPGen(rmf_osal_MemColor color, uint32_t size, void **memory);
#else
rmf_Error
        rmf_osal_memReallocPGen_track(rmf_osal_MemColor color, uint32_t size, void **memory, const char* file, int line);
#define rmf_osal_memReallocPGen(c, s, pm) rmf_osal_memReallocPGen_track(c, s, pm, __FILE__, __LINE__)
#endif

void rmf_osal_memInit(void);

rmf_Error rmf_osal_memAllocPProf(rmf_osal_MemColor color, uint32_t size, void **memory,
        char* fileName, uint32_t lineNum);

rmf_Error rmf_osal_memFreePProf(rmf_osal_MemColor color, void *memory, char* fileName,
        uint32_t lineNum);

rmf_Error rmf_osal_memReallocPProf(rmf_osal_MemColor color, uint32_t size,
        void **memory, char* fileName, uint32_t lineNum);

/** @} */ //end of doxygen tag OSAL_MEMORY_API

#define DEBUG_MEMCPY

#ifdef DEBUG_MEMCPY
/**
 * The <i>rmf_osal_memcpyProf()</i> function will check if there any mismatch
 * in memcpy parameters.
 *
 * @param dest the pointer to the destination memory area
 * @param src is the source memory area
 * @param size the number of bytes to be copied.
 * @param destCapacity the maximum bytes that can be copied to the pointer dest
 * @param srcCapacity the maximum bytes that can be copied from the pointer src
 * @param fileName is a pointer for the name of the .c file
 * @param lineNum is the line number in the .c file
 * @return the dest pointer
 */

void * rmf_osal_memcpyProf( void *dest, 
								const void *src,  
								size_t size, 
								size_t destCapacity, 
								size_t srcCapacity, 
								const char* fileName,
								uint32_t lineNum);
#endif


#ifdef DEBUG_MEMCPY
#define rmf_osal_memcpy(d, s, n, dc, sc)			rmf_osal_memcpyProf(d, s, n, dc, sc, __FILE__, __LINE__)
#else
#define rmf_osal_memcpy(d, s, n, dc, sc)			memcpy(d, s, n)
#endif


#ifdef DEBUG_ALLOC
void print_alloc_info();
#endif

#ifdef __cplusplus
}
#endif

#endif /* _RMF_OSAL_MEM_H */
