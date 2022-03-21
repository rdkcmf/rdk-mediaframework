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
 * @file rmf_osal_mem.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <rmf_osal_types.h>
#include <rmf_osal_mem.h>
#include <rmf_osal_error.h>
#include "rdk_debug.h"
#include <string.h>
#include <assert.h>

#define USE_GUARD_ALLOC

#ifdef DEBUG_ALLOC
#undef rmf_osal_memReallocPGen
#undef rmf_osal_memAllocPGen
rmf_Error rmf_osal_memAllocPGen(rmf_osal_MemColor color, uint32_t size, void **memory);
rmf_Error rmf_osal_memReallocPGen(rmf_osal_MemColor color, uint32_t size, void **memory);

pthread_mutex_t g_mem_mutex = PTHREAD_MUTEX_INITIALIZER;
struct meminfo_s
{
    uint32_t allocs;
    uint64_t alloc_size;
    uint32_t frees;    
};
struct meminfo_s meminfo[RMF_OSAL_MEM_NCOLORS]= {0};
struct meminfo_s meminfo_bak[RMF_OSAL_MEM_NCOLORS]= {0};

struct mem_detailed_info_s
{
    void* mem;
    const char* file;
    int line;
    rmf_osal_MemColor color;
    uint32_t size;
    int printed;
    mem_detailed_info_s *next;
};
mem_detailed_info_s * g_meminfo_list = NULL;
#endif

#ifdef USE_GUARD_ALLOC
static uint8_t g_rmf_osal_memGuard_en = 0;
static volatile uint8_t g_rmf_osal_memset = 0;

#define MEM_GUARD_SIZE 32
#define MEM_GUARD_FILL 0xAC
#define MEM_SET_SIZE 4
#endif

#ifdef RMF_OSAL_FEATURE_MEM_PROF
#define USE_SYSRES_MLT 1
#include <pthread.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#endif

#if USE_SYSRES_MLT
#include <assert.h>
#include <ctype.h>
#include "mlt_malloc.h"
#endif

#if USE_SYSRES_MLT
#undef  USE_FILE_LINE_INFO
#undef  RETURN_ADDRESS_LEVEL
#define RETURN_ADDRESS_LEVEL 1
#define     MLT_CALLER_INDEX    0
#define     MLT_OBSIZE_INDEX    1
#define     MLT_MGCN        0x123ab678
#ifdef DEBUG_ALLOC
#define PRINT_MEME_ENABLE 1
#define PRINT_MEMG_ENABLE 1
#endif
#if PRINT_MEME_ENABLE
#define PRINT_MEME(x)   x
#else
#define PRINT_MEME(x)
#endif
#if PRINT_MEMG_ENABLE
#define PRINT_MEM(x)    x
#else
#define PRINT_MEM(x)
#endif

#define     BYTES_PER_ROW       32
//#include "rpl_malloc.h"
//#include "mlt_malloc.h"
#define lock()      pthread_mutex_lock(&mltMutex);
#define unlock()    pthread_mutex_unlock(&mltMutex);

typedef struct MLTHeaderTag
{
    unsigned long       data[3];
    unsigned long       mgcn;
    char            *file;
    int             line;
    struct MLTHeaderTag *next;
    struct MLTHeaderTag *prev;
}MLTHeader;

static MLTHeader    *allocsList=NULL;   // Current iteration's allocs list
static unsigned long    allocsSize=0;       // Current number of active allocations
static unsigned long    allocsMaxSize=0;    // Maximum number of active allocations
static unsigned long    totalMallocCalls=0; // Total number of malloc calls
static unsigned long    totalFreeCalls=0;   // Total number of free calls
static unsigned long    totalReallocCalls=0;    // Total number of realloc calls
static unsigned long    totalCallocCalls=0; // Total number of calloc calls
static pthread_mutex_t  mltMutex = PTHREAD_MUTEX_INITIALIZER;

static MLTHeader    *allocsList1=NULL;  // Allocs list for the 1st N iterations
static MLTHeader    *allocsList2=NULL;  // Allocs list for the after N iterations
#endif

// static void memtkCommand(eMon_Why, Mon_InputFunc, Mon_OutputFunc, ui8 *);
#ifdef RMF_OSAL_FEATURE_MEM_PROF
#if !USE_SYSRES_MLT
static const char *color2String( rmf_osal_MemColor );
static const char *extractFilename (const char *); 
static void addMemInTrackList(void*, char*, int);
static void removeMemInTrackList(void*);
#endif  //!USE_SYSRES_MLT
#endif
#ifdef NEW_REALLOC
static void* memRealloc(Mem_Pointer, uint32_t);
#endif

#if USE_SYSRES_MLT
static rmf_Error  rmf_osal_memAllocPGenMLT(rmf_osal_MemColor color, uint32_t size, void **mem, void* caller, char* fileName, uint32_t lineNum);
static rmf_Error  rmf_osal_memFreePGenMLT(rmf_osal_MemColor color, void *mem, void* caller, char* fileName, uint32_t lineNum);
static rmf_Error  rmf_osal_memReallocPGenMLT(rmf_osal_MemColor color, uint32_t size, void **mem, void* caller, char* fileName, uint32_t lineNum);
#endif

/* PORT095 */
/**
 * @brief Use this API to allocate a block of system memory of the specified size.  
 * The address of the memory block allocated is returned via the pointer.
 *
 * @param[in] color a somewhat loose designation of the type or intended use of the
 *          memory; may or may not be used depending upon implementation.
 *          May map to a distinct heap (for example).
 * @param[in] size the size of the memory block to allocate
 * @param[out] memory a pointer for returning the pointer to the newly allocated 
 *          memory block.
 * @param[in] fileName a pointer for the name of the .c file
 * @param[in] lineNum the line number in the .c file
 * @return Returns the RMF_OSAL error code if the create fails, otherwise RMF_SUCCESS is returned.
 */
rmf_Error  rmf_osal_memAllocPProf(rmf_osal_MemColor color, uint32_t size, void **memory, char* fileName, uint32_t lineNum)
{
#if USE_SYSRES_MLT
    return rmf_osal_memAllocPGenMLT(color, size, memory, __builtin_return_address(0), fileName, lineNum);
#else
    printf("STUB: <rmf_osal_mem.c::rmf_osal_memAllocPProf> implementation pending\n");
    return RMF_SUCCESS;
#endif  //USE_SYSRES_MLT
}

/**
 * @brief This API is used to free the specified block of system memory.
 *
 * @param[in] color the original color specified on allocation
 * @param[in] memory a pointer to the memory block to free.
 * @param[in] fileName a pointer for the name of the .c file
 * @param[in] lineNum the line number in the .c file
 * @return Returns the RMF_OSAL error code if the create fails, otherwise RMF_SUCCESS is returned.
 */
rmf_Error  rmf_osal_memFreePProf(rmf_osal_MemColor color, void *memory, char* fileName, uint32_t lineNum)
{
#if USE_SYSRES_MLT
    return rmf_osal_memFreePGenMLT(color, memory, __builtin_return_address(0), fileName, lineNum);
#else
    printf("STUB: <rmf_osal_mem.c::rmf_osal_memFreePProf> implementation pending\n");
    return RMF_SUCCESS;
#endif  //USE_SYSRES_MLT
}

/**
 * @brief This function will resize the specified block of memory to the specified size. 
 *
 * @param color a somewhat loose designation of the type or intended use of the
 *          memory; may or may not be used depending upon implementation.
 *          May map to a distinct heap (for example).
 * @param size is the size to resize to.
 * @param memory is a pointer to the pointer to the memory block to resize.  The
 *          new memory block pointer is returned via this pointer.  If the pointer
 *          is NULL, then this method has the same effect as a call to rmf_osal_memAllocPProf().
 * @param fileName is a pointer for the name of the .c file
 * @param lineNum is the line number in the .c file
 * @return Returns the RMF_OSAL error code if the create fails, otherwise RMF_SUCCESS is returned.
 */
rmf_Error  rmf_osal_memReallocPProf(rmf_osal_MemColor color, uint32_t size, void **memory, char* fileName, uint32_t lineNum)
{
#if USE_SYSRES_MLT
    return rmf_osal_memReallocPGenMLT(color, size, memory, __builtin_return_address(0), fileName, lineNum);
#else
    printf("STUB: <rmf_osal_mem.c::rmf_osal_memReallocPProf> implementation pending\n");
    return RMF_SUCCESS;
#endif  //USE_SYSRES_MLT
}

#ifdef DEBUG_MEMCPY
/**
 * The <i>rmf_osal_memFreePProf()</i> function will free the specified block of system
 * memory.
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

void * rmf_osal_memcpyProf(void *dest, const void *src,  size_t size, size_t destCapacity, size_t srcCapacity, const char* fileName,
        uint32_t lineNum)
{
        if ((size > destCapacity) || (size > srcCapacity))
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "memory corruption due to memcpy in %s:%d size=%d, destCapacity=%d, srcCapacity=%d ",
                        fileName, lineNum, size, destCapacity, srcCapacity);

                assert( size < destCapacity );
                assert( size < srcCapacity );
        }
        memcpy(dest,src,size);
        return dest;
}

#endif

#if 0 //USE_SYSRES_MLT
#if PRINT_MEMG_ENABLE || PRINT_MEME_ENABLE
static void mlt_print_memory_raw(void *p, size_t size)
{
    int i=0;
    int msize=size;
    unsigned int  *ip=(unsigned int *)p;
    unsigned char *cp=(unsigned char *)p;
    while (msize > 0)
    {
        RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.MEM", "%p: ", ip);
        for (i=0; i<BYTES_PER_ROW/4 && i<msize; i++) RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.MEM", "%8.8x ", *ip++);
        for (i=0; i<BYTES_PER_ROW && i<msize; i++) 
        {
            if (isprint(*cp) !=0 ) RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.MEM", "%c", *cp++);
            else               RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.MEM", " ",  *cp++);
        }
        RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.MEM", "\n");
        msize -= BYTES_PER_ROW;
    }
}

static void mlt_print_memory(void *p, size_t size)
{
    PRINT_MEME(RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.MEM", "mlt_print_memory: p = %p size = %d\n",p, size));
    PRINT_MEME(mlt_print_memory_raw((unsigned char *)p - BYTES_PER_ROW, BYTES_PER_ROW));
    PRINT_MEME(mlt_print_memory_raw(p, size));
    size = (size % BYTES_PER_ROW) ? ((size / BYTES_PER_ROW) + 1)*BYTES_PER_ROW : size;
    PRINT_MEME(mlt_print_memory_raw((unsigned char *)p + size, BYTES_PER_ROW));
    PRINT_MEME(RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.MEM", "\n"));
}
#endif
static unsigned long * mlt_add (unsigned long *p, unsigned long size, void *caller, const char *file, int line)
{
    lock();
    ++allocsSize;
    if (allocsMaxSize < allocsSize) allocsMaxSize = allocsSize;
    MLTHeader   *object = (MLTHeader *)p;
    object->prev    = NULL;
    object->data[MLT_OBSIZE_INDEX]  = size;
    object->data[MLT_CALLER_INDEX]  = (unsigned long)caller;
    object->line    = line;
    object->file    = (char*)file;
    object->mgcn    = MLT_MGCN;
    if (file)
    {
        int len = strlen(file)+1;
        object->file = (char*)malloc(len);
        strncpy(object->file, file, len);
    }

    object->next    = allocsList;
    if (allocsList) allocsList->prev = object;
    allocsList = object;
    RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.MEM", "mlt_add:    thread = %p caller = %p size = %8.8d file = %s:%d p = %p o = %p\n", pthread_self(), caller, size, file, line, object+1, p);
    unlock();

    return (unsigned long *)(object+1);
}

static void * mlt_remove (void *p, void *caller, const char *file, int line)
{
    MLTHeader   *object, *next, *prev;

    lock();
    object = (MLTHeader *)p - 1;
    if (object == NULL || object->mgcn != MLT_MGCN)
    {
        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.MEM", "mlt_rem:    mlt mismatch or possible memory corruption: thread = %p caller = %p file = %s:%d p = %p\n", pthread_self(), caller, file, line, p);
        PRINT_MEM(mlt_print_memory(object, 64));
        //*(int*)(0)=1;
        unlock();
        return p;
    }

    RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.MEM", "mlt_rem:    thread = %p caller = %p size = %8.8d file = %s:%d p = %p o = %p\n", pthread_self(), object->data[MLT_CALLER_INDEX], object->data[MLT_OBSIZE_INDEX], object->file, object->line, p, object);
    PRINT_MEM(mlt_print_memory(object, object->data[MLT_OBSIZE_INDEX] + sizeof(MLTHeader)));

    object->mgcn    = 0;
    if (object->file) free(object->file);
    next = object->next;
    prev = object->prev;
    if (next != NULL) next->prev = prev;

    if (prev != NULL)
        prev->next = next;
    else if (object == allocsList)
        allocsList = next;
    else if (object == allocsList1)
        allocsList1 = next;
    else if (object == allocsList2)
        allocsList2 = next;
    --allocsSize;
    unlock();

    return (void *)object;
}

#if USE_SYSRES_MLT
void *mlt_malloc(unsigned int size, void *caller, const char *file, int line)
{
    RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.MEM", "mlt_malloc: thread = %p caller = %p size = %8.8d file = %s:%d\n", pthread_self(), caller, size, file, line);
    ++totalMallocCalls;
    void *p=malloc(size + sizeof(MLTHeader));
    if (p)
        p = (void*)mlt_add ((unsigned long *)p, size, caller, file, line);
    else
    {
        printf("mlt_malloc: Memory Allocation Failure: thread = %p caller = %p size = %8.8d file = %s:%d\n", pthread_self(), caller, size, file, line);
        assert(false);
    }
    RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.MEM", "mlt_malloc: thread = %p caller = %p size = %8.8d file = %s:%d p = %p\n", pthread_self(), caller, size, file, line, p);
    return p;
}
#else
static void *mlt_malloc(unsigned int size, void *caller, const char *file, int line)
{
    RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.MEM", "mlt_malloc: thread = %p caller = %p size = %8.8d file = %s:%d\n", pthread_self(), caller, size, file, line);
    ++totalMallocCalls;
    void *p=malloc(size + sizeof(MLTHeader));
    if (p)
        p = (void*)mlt_add ((unsigned long *)p, size, caller, file, line);
    else
    {
        printf("mlt_malloc: Memory Allocation Failure: thread = %p caller = %p size = %8.8d file = %s:%d\n", pthread_self(), caller, size, file, line);
        assert(false);
    }
    RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.MEM", "mlt_malloc: thread = %p caller = %p size = %8.8d file = %s:%d p = %p\n", pthread_self(), caller, size, file, line, p);
    return p;
}
#endif

#if USE_SYSRES_MLT
void mlt_free(void *p, void *caller, const char *file, int line)
{
    RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.MEM", "mlt_free:   thread = %p caller = %p file = %s:%d p = %p\n", pthread_self(), caller, file, line, p);
    ++totalFreeCalls;
    if (p) free (mlt_remove(p, caller, file, line));
    RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.MEM", "mlt_free:   thread = %p caller = %p file = %s:%d p = %p ...done\n", pthread_self(), caller, file, line, p);
}
#else
static void mlt_free(void *p, void *caller, const char *file, int line)
{
    RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.MEM", "mlt_free:   thread = %p caller = %p file = %s:%d p = %p\n", pthread_self(), caller, file, line, p);
    ++totalFreeCalls;
    if (p) free (mlt_remove(p, caller, file, line));
    RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.MEM", "mlt_free:   thread = %p caller = %p file = %s:%d p = %p ...done\n", pthread_self(), caller, file, line, p);
}
#endif


#if USE_SYSRES_MLT
void *mlt_realloc(void *p, unsigned int size, void *caller, const char *file, int line)
#else
static void *mlt_realloc(void *p, unsigned int size, void *caller, const char *file, int line)
#endif
{
    RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.MEM", "mlt_realloc:thread = %p caller = %p size = %8.8d file = %s:%d p = %p\n", pthread_self(), caller, size, file, line, p);
    ++totalReallocCalls;
        // ANSI: realloc(NULL, size) is equivalent to malloc(size)
    if (!p)
    {
        return mlt_malloc(size, caller, file, line);
    }

    // ANSI: realloc(p, 0) is equivalent to free(p) except that NULL is returned
    if (!size)
    {
        mlt_free(p, caller, file, line);
        return NULL;
    }

    void *m=mlt_malloc(size, caller, file, line);
    if (!m)
    {
        RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.MEM", "mlt_realloc: Memory Allocation Failure: thread = %p caller = %p size = %8.8d file = %s:%d p = %p\n", pthread_self(), caller, size, file, line, p);
        return m;
    }

    MLTHeader *object = (MLTHeader *)p - 1;
    memcpy(m, p, size > object->data[MLT_OBSIZE_INDEX] ? object->data[MLT_OBSIZE_INDEX] : size);

    mlt_free(p, caller, file, line);

    return m;
}

#if USE_SYSRES_MLT
void *mlt_calloc(size_t num, size_t size, void *caller, const char *file, int line)
{
    RDK_LOG ( RDK_LOG_DEBUG, "LOG.RDK.MEM", "mlt_calloc: thread = %p caller = %p size = %8.8d file = %s:%d\n", pthread_self(), caller, num*size, file, line);
    ++totalCallocCalls;
    void *p=mlt_malloc(num*size, caller, file, line);
    if (p) memset(p, 0, num*size);

    return p;
}
#else
static void *mlt_calloc(size_t num, size_t size, void *caller, const char *file, int line)
{
    RDK_LOG ( RDK_LOG_DEBUG, "LOG.RDK.MEM", "mlt_calloc: thread = %p caller = %p size = %8.8d file = %s:%d\n", pthread_self(), caller, num*size, file, line);
    ++totalCallocCalls;
    void *p=mlt_malloc(num*size, caller, file, line);
    if (p) memset(p, 0, num*size);

    return p;
}
#endif

#if USE_SYSRES_MLT
char *mlt_strdup(const char *str, void *caller, const char *file, int line)
{
    if (str == NULL) return NULL;

    size_t size=strlen(str)+1;
    RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.MEM", "mlt_strdup: thread = %p caller = %p size = %8.8d file = %s:%d\n", pthread_self(), caller, size, file, line);
    void *p=mlt_malloc(size, caller, file, line);

    return p ? (char *)memcpy(p, str, size) : NULL;
}
#else
static char *mlt_strdup(const char *str, void *caller, const char *file, int line)
{
    if (str == NULL) return NULL;

    size_t size=strlen(str)+1;
    RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.MEM", "mlt_strdup: thread = %p caller = %p size = %8.8d file = %s:%d\n", pthread_self(), caller, size, file, line);
    void *p=mlt_malloc(size, caller, file, line);

    return p ? (char *)memcpy(p, str, size) : NULL;
}
#endif
#endif

#if USE_SYSRES_MLT
static rmf_Error  rmf_osal_memAllocPGenMLT(rmf_osal_MemColor color, uint32_t size, void **mem, void* caller, char* fileName, uint32_t lineNum)
{
    rmf_Error retval = RMF_SUCCESS;

    if ( (RMF_OSAL_MEM_NCOLORS < color) || (NULL == mem) || (0 == size) )
    {
        retval = RMF_OSAL_EINVAL;
    }
    else
    {
      if ((*mem = mlt_malloc(size, caller, fileName, lineNum)) == NULL)
        {
            retval = RMF_OSAL_ENOMEM;
        }
    }
    return(retval);
}

static rmf_Error  rmf_osal_memFreePGenMLT(rmf_osal_MemColor color, void *mem, void* caller, char* fileName, uint32_t lineNum)
{
    rmf_Error retval = RMF_SUCCESS;

    if ( (RMF_OSAL_MEM_NCOLORS < color) || (NULL == mem) )
    {
        retval = RMF_OSAL_EINVAL;
    }
    else
    {
        mlt_free(mem, caller, fileName, lineNum);
    }


    return(retval);
}

static rmf_Error  rmf_osal_memReallocPGenMLT(rmf_osal_MemColor color, uint32_t size, void **mem, void* caller, char* fileName, uint32_t lineNum)
{
    rmf_Error retval = RMF_SUCCESS;

    if ( RMF_OSAL_MEM_NCOLORS < color )
    {
        retval = RMF_OSAL_EINVAL;
    }
    else if ( (NULL == mem) || ((NULL == *mem) && (0 == size)) )
    {
        retval = RMF_OSAL_EINVAL;
    }

    if ( retval == RMF_SUCCESS )
    {
        void *p;
        if ((p = mlt_realloc(*mem, size, caller, fileName, lineNum)) == NULL)
    {
        retval = RMF_OSAL_ENOMEM;
    }
        else
    {
        *mem = p;
    }
    }

    return (retval);
}

#endif  //USE_SYSRES_MLT


#ifdef DEBUG_ALLOC

rmf_Error rmf_osal_memAllocPGen_track(rmf_osal_MemColor color, uint32_t size, void **memory, const char* file, int line)
{
	rmf_Error ret;
	mem_detailed_info_s * info;
	ret = rmf_osal_memAllocPGen( color, size, memory);
	if (RMF_SUCCESS == ret)
	{
		info = (mem_detailed_info_s *)malloc(sizeof(mem_detailed_info_s));
		if (NULL != info)
		{
			info->file = file;
			info->line = line;
			info->mem = *memory;
			info->color = color;
			info->size = size;
			info->printed = 0;
			pthread_mutex_lock(&g_mem_mutex);
			info->next = g_meminfo_list;
			g_meminfo_list = info;
			pthread_mutex_unlock(&g_mem_mutex);
		}
	}
	return ret;
}


rmf_Error rmf_osal_memReallocPGen_track(rmf_osal_MemColor color, uint32_t size, void **memory, const char* file, int line)
{
	rmf_Error ret;
	mem_detailed_info_s * info;
	pthread_mutex_lock(&g_mem_mutex);
	info = g_meminfo_list;
	while ( info )
	{
		if (info->mem == *memory)
		{
			break;
		}
		info =info->next;
	}
	ret = rmf_osal_memReallocPGen( color, size, memory);
	if (RMF_SUCCESS == ret)
	{
		if (NULL != info)
		{
			info->file = file;
			info->line = line;
			info->color = color;
			info->size = size;
			info->mem = *memory;
			info->printed = 0;
		}
	}
	pthread_mutex_unlock(&g_mem_mutex);
	return ret;
}

/**
 * @brief This function dumps minimal data regarding memory allocation. 
 * @return None
 */
void print_alloc_info()
{
    int i;
    printf("\n\n\n===========RMF OSAL MEM ALLOC INFO================\n");
    for (i=0; i<RMF_OSAL_MEM_NCOLORS; i++)
    {
        if (meminfo[i].allocs >0)
        {
            printf("color = %d allocs = %d frees = %d alloc_size = %llu\n", i, meminfo[i].allocs, meminfo[i].frees, meminfo[i].alloc_size);
        }
    }
    printf("\n\n\n===========RMF OSAL PREV MEM ALLOC INFO================\n");
    for (i=0; i<RMF_OSAL_MEM_NCOLORS; i++)
    {
        if (meminfo_bak[i].allocs >0)
        {
            printf("color = %d allocs = %d frees = %d alloc_size = %llu\n", i, meminfo_bak[i].allocs, meminfo_bak[i].frees, meminfo_bak[i].alloc_size);
        }
    }

    printf("\n\n\n===========RMF OSAL PROBABLE LEAKS================\n");
    for (i=0; i<RMF_OSAL_MEM_NCOLORS; i++)
    {
        if ((meminfo_bak[i].allocs >0) && (meminfo[i].allocs >0))
        {
            int previous_alloc_free_diff  =  (meminfo_bak[i].allocs- meminfo_bak[i].frees);
            int current_alloc_free_diff = (meminfo[i].allocs- meminfo[i].frees);
            if ( previous_alloc_free_diff < current_alloc_free_diff)
            {
                printf("color = %d current_alloc_free_diff = %d previous_alloc_free_diff = %d \n", 
                    i, current_alloc_free_diff, previous_alloc_free_diff);
            }
        }
    }
    {
        mem_detailed_info_s * info;
        pthread_mutex_lock(&g_mem_mutex);
        info = g_meminfo_list;
        printf("\n====List of allocs since last call to this that are not freed yet====\n");
        while ( info )
        {
            if (info->printed == 0)
            {
                printf(" MEM = 0x%x, file = %s, line =%d color = %d size = %d\n", 
                           info->mem, info->file, info->line, (unsigned int)info->color, info->size);
                    
            }
            info->printed  = 1;
            info =info->next;
        }
        pthread_mutex_unlock(&g_mem_mutex);
    }
    printf("\n\n\n===========End================\n");
    memcpy(meminfo_bak, meminfo, RMF_OSAL_MEM_NCOLORS*sizeof(meminfo_s));
}
#endif

/**
 * @brief This function is used allocate a block of system memory of the specified size. The address of
 * the memory block allocated is returned via the pointer.
 *
 * @param[in] color the original color specified on allocation
 * @param[in] size the size of the memory block to allocate.
 * @param[out] mem a pointer for returning the pointer to the newly allocated memory block
 *
 * @return The RMF_OSAL error code if allocation fails, otherwise RMF_SUCCESS is returned.
 */
rmf_Error  rmf_osal_memAllocPGen(rmf_osal_MemColor color, uint32_t size, void **mem)
{
    rmf_Error retval = RMF_SUCCESS;

    if ( (RMF_OSAL_MEM_NCOLORS <= color) || (NULL == mem) || (0 == size) )
    {
        retval = RMF_OSAL_EINVAL;
    }
    else
    {
#ifdef USE_GUARD_ALLOC
		if(g_rmf_osal_memGuard_en)
		{
        	size = size + MEM_GUARD_SIZE;
		}
		else if(g_rmf_osal_memset)
		{
        	size = size + MEM_GUARD_SIZE;
		}
#endif
      if ((*mem = calloc(size, 1)) == NULL)
        {
            retval = RMF_OSAL_ENOMEM;
        }
#if defined(DEBUG_ALLOC) || defined(USE_GUARD_ALLOC)
        else
        {
		if(g_rmf_osal_memGuard_en)
		{
	            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.MEM",
	                "Memory allocated at 0x%x \n",
	                (unsigned int)*mem);
	            char * start = (char*)*mem;
	            int i =0;
	            for ( i =0; i<MEM_GUARD_SIZE; i++)
	            {
	                *( start + i)  = MEM_GUARD_FILL;
	            }
	            *mem = start + MEM_GUARD_SIZE;
	            
	            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.MEM",
	                "mem = 0x%x, start =0x%x \n",
	                (unsigned int)*mem, (unsigned int )start);
		}
		else if(g_rmf_osal_memset)
		{		
			char * start = (char*)*mem;
			memcpy(start, &size, MEM_SET_SIZE);
			*mem = start + MEM_SET_SIZE;
		}
#ifdef DEBUG_ALLOC
            meminfo[color].allocs++;
            meminfo[color].alloc_size += size;
#endif			
        }
#endif
    }
    return(retval);
}

/**
 * @brief This function is used to free the specified block of system memory.
 *
 * @param[in] color the original color specified on allocation.
 * @param[in] mem a pointer to the memory block to free.
 *
 * @return The RMF_OSAL error code if free fails, otherwise RMF_SUCCESS is returned.
 */
rmf_Error  rmf_osal_memFreePGen(rmf_osal_MemColor color, void *mem)
{
    rmf_Error retval = RMF_SUCCESS;

    if ( (RMF_OSAL_MEM_NCOLORS <= color) || (NULL == mem) )
    {
        retval = RMF_OSAL_EINVAL;
    }
    else
    {
#ifdef DEBUG_ALLOC
        mem_detailed_info_s * info;
        mem_detailed_info_s * previnfo = NULL;
        pthread_mutex_lock(&g_mem_mutex);
        info = g_meminfo_list;
        while ( info )
        {
            if (info->mem == mem)
            {
                if (info->color != color )
                {
                      printf("%s:%d: Mismatch in memcolor. Aborting\n", __FUNCTION__, __LINE__);
                      printf(" MEM = 0x%x, file = %s, line =%d color = %d  color requested = %d size = %d\n", 
                           info->mem, info->file, info->line, (unsigned int)info->color, color, info->size);
                      abort();
                }
                break;
            }
            previnfo = info;
            info =info->next;
        }
        if (info )
        {
            if (previnfo)
            {
                previnfo->next = info->next;
            }
            else
            {
                g_meminfo_list = info->next;
            }
            free(info);
        }
        else
        {
             printf("%s:%d: Entry not in list - possible corruption.. color=%d aborting\n", __FUNCTION__, __LINE__, color);
             abort();
        }
        pthread_mutex_unlock(&g_mem_mutex);
#endif
#ifdef USE_GUARD_ALLOC
	if(g_rmf_osal_memGuard_en)
	{
            char * start = (char*)mem - MEM_GUARD_SIZE;
            int i =0;
            for ( i = 0 ; i<MEM_GUARD_SIZE; i++)
            {
                //printf("[%d]0x%x = %d\n", i, ( start + i), *( start + i));
                if( *( start + i)  != MEM_GUARD_FILL)
                {
                    int j;
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD",
                        "Memory corruption !! Dumping guard bytes. mem = 0x%x, start =0x%x color = %d \n",
                        (unsigned int)mem, (unsigned int )start, color);
                    for (j=0 ; j<MEM_GUARD_SIZE; j++)
                    {
                        printf("0x%x  ", *( start + j));
                    }
                    printf("\n ");

                    /* The following two lines will be uncommented  
                    once we confirm there is no false alarm: starts */        
                    retval = RMF_OSAL_EINVAL;
                    /* abort(); */
                    assert(0);
                    break;
                }
            }
            if( i == MEM_GUARD_SIZE )
            {				
                    for (i=0 ; i<MEM_GUARD_SIZE; i++)
                    {
                   		*( start + i) = 0;				
                    }
                    mem = start;
            }
	}
	else if(g_rmf_osal_memset)
	{
		char * start = (char*)mem - MEM_SET_SIZE;
		uint32_t size = 0;
		memcpy(&size, start, MEM_SET_SIZE);
		memset(start, MEM_GUARD_FILL, size);
		mem = start;
	}
#endif 
        free(mem);
#ifdef DEBUG_ALLOC
         meminfo[color].frees++;
#endif
    }

    return(retval);
}

/**
 * @brief This function is used to resize the specified block of memory to the specified size.
 *
 * @param[in] color the original color specified on allocation
 * @param[in] size size to which the memory block need to be resized.
 * @param[out] mem a pointer to the pointer to the memory block to resize. The
 *        new memory block pointer is returned via this pointer.
 *
 * @return The RMF_OSAL error code if resize fails, otherwise RMF_SUCCESS is returned.
 */
rmf_Error  rmf_osal_memReallocPGen(rmf_osal_MemColor color, uint32_t size, void **mem)
{
    rmf_Error retval = RMF_SUCCESS;

    if ( RMF_OSAL_MEM_NCOLORS < color )
    {
        retval = RMF_OSAL_EINVAL;
    }
    else if ( (NULL == mem) || ((NULL == *mem) && (0 == size)) )
    {
        retval = RMF_OSAL_EINVAL;
    }

    if ( retval == RMF_SUCCESS )
    {
#ifdef USE_GUARD_ALLOC
		if(g_rmf_osal_memGuard_en)
		{
            if( size )
            {
                char * start = (char*)*mem - MEM_GUARD_SIZE;
                int i =0;
    
                for ( i = 0 ; i<MEM_GUARD_SIZE; i++)
                {
                    if( *( start + i)  != MEM_GUARD_FILL)
                    {
                        int j;
                        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD",
                            "Memory corruption !! Dumping guard bytes. mem = 0x%x, start =0x%x \n",
                            (unsigned int)*mem, (unsigned int )start);
                        for (j=0 ; j<MEM_GUARD_SIZE; j++)
                        {
                            printf("0x%x  ", *( start + j));
                        }
                        printf("\n ");        
                        /* The following two lines will be uncommented  
                        once we confirm there is no false alarm: starts */        
                        /* retval = RMF_OSAL_EINVAL;                      */
                        /* abort(); */
                        assert(0);						
                        /* ends */			
			   break;			
                    }				
                }
                if( i == MEM_GUARD_SIZE )
                {				
                        for (i=0 ; i<MEM_GUARD_SIZE; i++)
                        {
                       		*( start + i) = 0;				
                        }	
                        *mem = start;
                }
            }
            size = size + MEM_GUARD_SIZE;
    	}
	else if(g_rmf_osal_memset)
	{
		if( size )
		{
			uint32_t size_old = 0;
			char * start = (char*)mem - MEM_SET_SIZE;
			memcpy(&size_old, start, MEM_SET_SIZE);
			memset(start, MEM_GUARD_FILL, size_old);
	                mem = start;
		}
		size = size + MEM_SET_SIZE;
	}
#endif 
        void *p;
        if ((p = realloc(*mem, size)) == NULL)
        {
            retval = RMF_OSAL_ENOMEM;
        }
        else
        {
#ifdef USE_GUARD_ALLOC
		if(g_rmf_osal_memGuard_en)
		{
	            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.MEM",
	                "Memory allocated at 0x%x \n",
	                (unsigned int)p);
	            char * start = (char*)p;
	            int i =0;
	            for ( i =0; i<MEM_GUARD_SIZE; i++)
	            {
	                *( start + i)  = MEM_GUARD_FILL;
	            }
	            p = start + MEM_GUARD_SIZE;
	            
	            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.MEM",
	                "p = 0x%x, start =0x%x \n",
	                (unsigned int)p, (unsigned int )start);
		}
                else if(g_rmf_osal_memset)
                {
						char * start = (char*)p;
                        memcpy(start, &size, MEM_SET_SIZE);
                        p = start + MEM_SET_SIZE;
                }

#endif
            *mem = p;
        }
    }

    return (retval);
}

/**
 * @brief This function is intendent to to perform any low-level initialization required
 * to set up the OS-level memory subsystem. The memory subsystem should be initialized following
 * the environment subsystem to allow it to be configured by the environment.  However, access
 * to the memory APIs prior to initialization should be allowed.
 * @return None
 */
void rmf_osal_memInit(void)
{
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.MEM", "rmf_osal_memInit()\n");
#ifdef USE_GUARD_ALLOC	
    char const* const fileName = "/opt/memGuard";
    char const* const fileNameMemSet = "/opt/memset";
    FILE* file = fopen(fileName, "r");
	if( file == NULL )
	{
		g_rmf_osal_memGuard_en = 0;
	}	
	else
	{
		g_rmf_osal_memGuard_en = 1;
	}
	
    FILE* fileMemSet = fopen(fileNameMemSet, "r");
	if( fileMemSet == NULL )
	{
		g_rmf_osal_memset = 0;
	}	
	else
	{
		g_rmf_osal_memset = 1;
	}	
#endif	
    return;
}

/**
 * Registers a callback which will be called when memory is low and the memory
 * manager needs the various components to release memory.  When memory is low,
 * the memory manager will request that the components free memory, and will ask
 * specify the amount of memory it's looking for.  The callback function should then
 * attempt to free the memory, and, when finished, return the amount of memory it
 * released. At the very least, the callback should return 0 if unable to free any
 * memory, and non-zero if it successfully freed something.
 *
 * @param color     The color this callback will attempt to free.
 * @param function  The callback function.
 * 
 * return error value or RMF_SUCCESS if successful
 */
#if 0 /* PORT095 */

rmf_Error rmf_osal_memRegisterMemFreeCallback(rmf_osal_MemColor color,  uint32_t (*function)(uint32_t))
#endif

/* PORT095 PHH 2/28/07 */
#ifdef RMF_OSAL_FEATURE_MEM_PROF
#if !USE_SYSRES_MLT

#define MEM_SYS_MINIMUM_SIZE_KB     (1024)


/**
 * Returns Color string for easy debugging.
 *
 * @param color the color for which a heap is requested
 *
 * @return a string corresponds to color type
 */
static const char *color2String( rmf_osal_MemColor color )
{
    printf("STUB: <rmf_osal_mem.c::rmf_osal_MemColor> implementation pending\n");
    return "UNIMPLEMENTED";

/* code from 0.9.5 powertv */
    return (color >= RMF_OSAL_MEM_SYSTEM && color < RMF_OSAL_MEM_NCOLORS)
        ? colors[COLOR_TO_INDEX(color)] : "UNKNOWN";
}

static const char *extractFilename (const char *path)
{
    char *name;
    printf("STUB: <rmf_osal_mem.c::extractFilename> implementation pending\n");
    return "NULL";

/* code from 0.9.5 powertv */
    if (path == NULL)
        return "NULL";

    if ((name = strrchr(path, '/')) == NULL)
        return path;
    else
        return name+1;
}

#ifdef NEW_REALLOC
static void* memRealloc(Mem_Pointer oldPtr, uint32_t size)
{
    void* ptr;

    printf("STUB: <rmf_osal_mem.c::memRealloc> implementation pending\n");
    return NULL;

#ifdef PORTED_PLACEHOLDER_CODE
/* code from 0.9.5 powertv */
    pk_Try
    {
        ptr = mem_ResizePointer((Mem_Pointer)oldPtr, size, TRUE);
    }
    pk_Catch(kPk_ExceptionAll)
    {
        ptr = NULL;
    }
    pk_EndCatch;

    return ptr;
#endif
}
#endif

void newMemtkMutex(void)
{
    printf("STUB: <rmf_osal_mem.c::newMemtkMutex> implementation pending\n");
    return NULL;

#ifdef PORTED_PLACEHOLDER_CODE
/* code from 0.9.5 powertv */
    static rmf_osal_Bool init = false;

    if (init == false)
    {
        gMemTrackMutex   = pk_NewMutex(kPtv_Mutex_Inherit, "MemTrackMutex");
        init = true;
    }
#endif
}

void setStartTime(void)
{
    printf("STUB: <rmf_osal_mem.c::setStartTime> implementation pending\n");

#ifdef PORTED_PLACEHOLDER_CODE
/* code from 0.9.5 powertv */
    static rmf_osal_Bool init = false;

    if (init == false)
    {
        gStartTime = pk_Time();
        init = true;
    }
#endif
}

#ifdef DEBUG_MEM_TRACK
static void checkMemTrackList (void)
{
    printf("STUB: <rmf_osal_mem.c::checkMemTrackList> implementation pending\n");

#ifdef PORTED_PLACEHOLDER_CODE
/* code from 0.9.5 powertv */
    TrackHeader *p, *q;

    pk_GrabMutex(gMemTrackMutex);
    p = q = gMemTrackList;

    while (p && p->reserved == 0xaabbccdd)
    {
        q = p;
        p = p->next;
    }
    if (p == NULL)
    {
        p = q;
        while (p && p->reserved == 0xaabbccdd)
        {
            p = p->prev;
        }
        if (p == NULL)
        {
            pk_ReleaseMutex(gMemTrackMutex);
            return;
        }
        else
            assert(p->reserved == 0xaabbccdd);
    }
    else
        assert(p->reserved == 0xaabbccdd);
#endif
}
#endif

/**
 * <i>addMemInTrackList()</i> function put memory tracking info into the allocated
 * memory and add it into the memory tracking link list.
 *
 * Description:
 *      Always add the memory to the head of the list. The list maintains
 * the order of the allocations. Freeing memory in the middle of list 
 * will not affect the order of the allocations.
 *
 * @param ptr is an OS memory address including the TrackHeader.
 * @param fileName is a string of caller's file name.
 * @param lineNum is an integer of line number in the caller's file.
 * @return none
 */
static void addMemInTrackList(void * ptr, char *fileName, int lineNum)
{
    printf("STUB: <rmf_osal_mem.c::addMemInTrackList> implementation pending\n");

#ifdef PORTED_PLACEHOLDER_CODE
/* code from 0.9.5 powertv */
    TrackHeader *pTHeader = (TrackHeader*)ptr;

    CHECK_MEM_TRACKLIST();
    pk_GrabMutex(gMemTrackMutex);

    pTHeader->timeStamp = pk_Time() - gStartTime;
    pTHeader->filename = fileName;
    pTHeader->lineno = lineNum;
    pTHeader->checkptNum = gCheckPtNum;
    pTHeader->checkptStr = gCheckPtStr;
    pTHeader->reserved = 0xaabbccdd;
    pTHeader->next = gMemTrackList;
    pTHeader->prev = NULL;
    gMemTrackList = pTHeader;
    if (pTHeader->next)
        pTHeader->next->prev = pTHeader;

    pk_ReleaseMutex(gMemTrackMutex);
    CHECK_MEM_TRACKLIST();
#endif
}

/**
 * <i>removeMemInTrackList()</i> function remove memory from the memory 
 * tracking link list.
 *
 * @param ptr is an OS memory address including the TrackHeader.
 * @return none
 */
static void removeMemInTrackList(void * ptr)
{
    printf("STUB: <rmf_osal_mem.c::removeMemInTrackList> implementation pending\n");

#ifdef PORTED_PLACEHOLDER_CODE
/* code from 0.9.5 powertv */
    TrackHeader *pTHeader = (TrackHeader*)ptr;

    CHECK_MEM_TRACKLIST();
    pk_GrabMutex(gMemTrackMutex);

    assert(pTHeader->reserved == 0xaabbccdd);

    if (pTHeader == gMemTrackList)
    { 
        gMemTrackList = pTHeader->next;
        gMemTrackList->prev = NULL;
    } 
    else 
    {
#if defined(qDebug) && (qDebug == 1)
        assert(pTHeader->prev && pTHeader->next);
#endif
        pTHeader->prev->next = pTHeader->next;
        pTHeader->next->prev = pTHeader->prev;
    }
    pTHeader->reserved = 0x11223344;
    pTHeader->next = NULL;
    pTHeader->prev = NULL;
    pk_ReleaseMutex(gMemTrackMutex);
    CHECK_MEM_TRACKLIST();
#endif
}

void dumpMemtkStats (void)
{
    printf("STUB: <rmf_osal_mem.c::dumpMemtkStats> implementation pending\n");

#ifdef PORTED_PLACEHOLDER_CODE
/* code from 0.9.5 powertv */

#ifdef RMF_OSAL_FEATURE_TRACK_MEM_FULL
    MemRange total = { 0 };
#endif
    int n = sizeof(stats) / sizeof(ColorStat);
    int i, j/*, currSize, maxSize, deallocSize*/;
    uint32_t free;
    uint32_t largest;
   
    MEMTK_OUTPUT("\n");
    MEMTK_OUTPUT("          COLOR        ALLOCS DEALLOCS OUTSTAND FAILURES FAILSIZE   SIZE     TOTAL     HEAP      FRAG\n");
    MEMTK_OUTPUT("      -------------- -------- -------- -------- -------- -------- -------- --------- ---------- ------\n");

    /* loop through all colors[] & stats[] */
    for(i=0; i < n; ++i)
    {
        rmf_osal_memGetFreeSize(INDEX_TO_COLOR(i), &free);
        rmf_osal_memGetLargestFree(INDEX_TO_COLOR(i), &largest);

#ifdef RMF_OSAL_FEATURE_TRACK_MEM_FULL
        memset((void*)&total, 0, sizeof(MemRange));
        /* loop through all the usage[] in each stats[] */
        for (j=0; j<94; j++)
    {
            total.alloc     += stats[i].usage[j].alloc;
            total.dealloc   += stats[i].usage[j].dealloc;
            total.failure   += stats[i].usage[j].failure;
            total.failSize  += stats[i].usage[j].failSize;
            total.currSize  += stats[i].usage[j].currSize;
            total.totalSize += stats[i].usage[j].totalSize;
    }
        MEMTK_OUTPUT("memtk:%-14s %8lu %8lu %8lu %8lu %8lu %8lu %9lu 0x%08x %5.2f%%\n",
            color2String(INDEX_TO_COLOR(i)), total.alloc, total.dealloc, total.alloc-total.dealloc,
            total.failSize, total.failSize, total.currSize, total.totalSize, (unsigned int)color2Heap(INDEX_TO_COLOR(i)), 
            (float)(free - largest) * 100 / (float)free);
        stats[i].currSize = total.currSize;
        stats[i].currSize = total.totalSize;
#else
        MEMTK_OUTPUT("memtk:%-14s %8lu                                               %9lu 0x%08x %5.2f%%\n",
            color2String(INDEX_TO_COLOR(i)), stats[i].currSize, stats[i].currSize, (unsigned int)color2Heap(INDEX_TO_COLOR(i)), 
            (float)(free - largest) * 100 / (float)free);
#endif
    }
#endif
}

static void x_PrintMemtkUsage(Mon_OutputFunc output)
{   
    printf("STUB: <rmf_osal_mem.c::x_PrintMemtkUsage> implementation pending\n");

#ifdef PORTED_PLACEHOLDER_CODE
/* code from 0.9.5 powertv */
    // Display a detailed description
    output("\n----------------\nmemtk\n----------------\n");
    output("\nDescription:\n");
    output("  This command gives you information about memory tracking in OCAP middleware.\n");

    output("\nSyntax:\n");
    output("  memtk               - displays memory tracking usage\n");
    output("  memtk #             - displays outstanding memory allocated with check point number (#)\n");
    output("  memtk all           - displays all outstanding memory allocated\n");
    output("  memtk latest        - displays the outstanding memory allocated that match the current check point\n");
    output("  memtk last          - displays the outstanding memory allocated that match the last check point\n");
    output("  memtk \"a string\"    - displays the outstanding memory allocated that match the check point string\n");
    output("  memtk mark          - mark a new check point\n");
    output("  memtk checkpt       - displays the latest check point\n");
    output("  memtk stat          - displays the sums of all individual colors\n");
#endif
}

/**
 * Installed PowerTV monitor command.
 */
static void memtkCommand(eMon_Why why, Mon_InputFunc input, Mon_OutputFunc output, ui8 *buf)
{
    printf("STUB: <rmf_osal_mem.c::memtkCommand> implementation pending\n");

#ifdef PORTED_PLACEHOLDER_CODE
/* code from 0.9.5 powertv */
    char    cmd[10] = "\0";
    char    arg1[64] = "\0";
    ui32    nArgs=0;
    ui32    checkpt;

    switch(why)
    {
        case kMon_Verbose:
            output("memtk - "); 
            /* FALLTHROUGH */
        case kMon_Help:
            x_PrintMemtkUsage(output);
            break;
        case kMon_Perform:
            output("Got command: ***%s***\n", buf);

            if (buf != NULL)
                nArgs = sscanf(buf, "%s %s", cmd, arg1);

            if (nArgs <= 1)
                x_PrintMemtkUsage(output);

            else if (!strcmp(arg1, "all"))
            {
                DisplayMemProfiling(ALL_CHECKPT);
            }
            else if (!strcmp(arg1, "latest"))
            {
                DisplayMemProfiling(LATEST_CHECKPT);
            }
            else if (!strcmp(arg1, "last"))
            {
                DisplayMemProfiling(LAST_CHECKPT);
            }
            else if (!strcmp(arg1, "stat"))
            {
                dumpMemtkStats();
            }
            else if (!strcmp(arg1, "mark"))
            {
                setMemCheckptStr(NULL);
                output("The new check point is %d\n", gCheckPtNum);
            }
            else if (!strcmp(arg1, "checkpt"))
            {
                pk_GrabMutex(gMemTrackMutex);
                output("The latest check point is %d\n", gCheckPtNum);
                pk_ReleaseMutex(gMemTrackMutex);
            }
            else if ((checkpt = strtoul(arg1, NULL, 10)) != 0)
            {
                if (checkpt > gCheckPtNum)
                    output("Check point is not recognized.\n");
                else
                    DisplayMemProfiling(checkpt);
            }
            else if (!strcmp(arg1, "0"))
            {
                    DisplayMemProfiling(0);
            }
            else    // use it as a string
            {
                TrackHeader     *pCurr = gMemTrackList;

                while (pCurr && (pCurr->checkptStr == NULL || strcmp(pCurr->checkptStr, arg1)))
                    pCurr = pCurr->next;
                if (pCurr)
                    DisplayMemProfiling(pCurr->checkptNum);
                else
                    output("The \"%s\" string is not found.\n", arg1);
            }
            //dumpUsageMap(stdout, arg);
            break;
        case kMon_Repeat:
            output("I'm not sure what to do with %d\n", why);
            break;
    }
#endif
}

void setMemCheckptStr (const char *str)
{
    printf("STUB: <rmf_osal_mem.c::setMemCheckptStr> implementation pending\n");

#ifdef PORTED_PLACEHOLDER_CODE
/* code from 0.9.5 powertv */
    pk_GrabMutex(gMemTrackMutex);
    gCheckPtStr = (char*)str;
    gCheckPtNum++;
    pk_ReleaseMutex(gMemTrackMutex);
#endif
}

void DisplayMemProfiling(ui32 checkpt)
{
    printf("STUB: <rmf_osal_mem.c::DisplayMemProfiling> implementation pending\n");

#ifdef PORTED_PLACEHOLDER_CODE
/* code from 0.9.5 powertv */
    TrackHeader     *pCurr = gMemTrackList;
    TrackHeader     *pStart = NULL;
    TrackHeader     *pEnd = NULL;
    int             i;
    char            buffer[256];

    if (pCurr == NULL)
        return;

    pk_GrabMutex(gMemTrackMutex);

    // startPtr
    if (checkpt == ALL_CHECKPT)
    {
        pStart = pCurr;
        pEnd = NULL;
    }
    else
    {
        // Assign the right check point
        if (checkpt == LATEST_CHECKPT)
            checkpt = gCheckPtNum;
        else if (checkpt == LAST_CHECKPT)
        {
            if ((checkpt = gCheckPtNum - 1) < 0)
            {
                MEMTK_OUTPUT("No memory allocations with this check point (%d)\n", checkpt);
                pk_ReleaseMutex(gMemTrackMutex);
                return; // no last
            }
        }

        pCurr = gMemTrackList;

        // Find the start pointer
        if (pCurr->checkptNum == checkpt)
            pStart = pCurr;
        else
        {
            while (pCurr && pCurr->checkptNum != checkpt)
                pCurr = pCurr->next;

            if (pCurr && pCurr->checkptNum == checkpt)
                pStart = pCurr;
            else
            {
                MEMTK_OUTPUT("No memory allocations with this check point (%d)\n", checkpt);
                pk_ReleaseMutex(gMemTrackMutex);
                return; // no checkpt
            }
        }

        // Find the end pointer (still using the last pCurr)
        while (pCurr && pCurr->checkptNum == checkpt)
            pCurr = pCurr->next;

        pEnd = pCurr;
    }

    MEMTK_OUTPUT("\n");
    MEMTK_OUTPUT("COUNT   MEMORY    SIZE       COLOR      CK#    TIME     FILENAME:LINENO       (CHECK PT STRING)\n");
    MEMTK_OUTPUT("----- ---------- ------- -------------- --- ----------\n");

    i = 0;
    pCurr = pStart;
    while (pCurr != pEnd)
    {
        snprintf(buffer,sizeof(buffer), "%5d 0x%08x %7d %-14s %3d 0x%08x %s:%d (%s)\n",
            i++, (int)pCurr,
            (int)pCurr->size, color2String(pCurr->color), (int)pCurr->checkptNum, (int)pCurr->timeStamp,
            extractFilename(pCurr->filename), (int)pCurr->lineno, pCurr->checkptStr?pCurr->checkptStr:" ");
        MEMTK_OUTPUT(buffer);

        pCurr = pCurr->next;
    }
    gCheckPtStr = NULL;
    pk_ReleaseMutex(gMemTrackMutex);
#endif
}

void configAvailableMemory(void)
{
    printf("STUB: <rmf_osal_mem.c::configAvailableMemory> implementation pending\n");

#ifdef PORTED_PLACEHOLDER_CODE
/* code from 0.9.5 powertv */
    const char *rsString = rmf_osal_envGet("MEM.SYS.RESTRICT.SIZE");
    int32_t size, throwAwayHeapSize, freeSize, spaceLeftInByte;
    Mem_Pointer memPtr;
    Mem_Heap memHeap;
    static rmf_osal_Bool init = false;
    
    if (init == false)
    {
        if (rsString == NULL)
            return;
        else
        {
            char *temp;
            long temp2;
            temp2 = strtol( rsString, &temp, 0 );
            if (rsString == temp)
                return;
            else
                spaceLeftInByte = (temp2 < MEM_SYS_MINIMUM_SIZE_KB ? MEM_SYS_MINIMUM_SIZE_KB : temp2 ) * 1024;
        }

        freeSize = mem_GetFreeSize(kMem_SystemHeap, 0);
        throwAwayHeapSize = freeSize - spaceLeftInByte;
        size = mem_GetLargestPointer(kMem_SystemHeap);

        if (throwAwayHeapSize < 0)
        {
            /* the intended size is bigger than the free system size, no op */
            init = true;
            return;
        }
        if (size > throwAwayHeapSize)
            size = throwAwayHeapSize;

        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.MEM", "<<MEM INIT>>: free Size(%d), throw away size(%d), space left(%d)\n", freeSize, size, spaceLeftInByte );

        {
            int32_t allocSize = size;
            pk_Try 
            {
                memPtr = mem_NewPointer(kMem_SystemHeap, allocSize);
                memHeap = mem_NewHeap(memPtr, allocSize);
            }
            pk_Catch(kPk_ExceptionAll)
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.MEM", "<<MEM INIT>>: could not create heap of %d bytes, freeSize(%d)\n", allocSize, freeSize);
                /* skip this heap. */
            }
            pk_EndCatch;
        }
        init = true;
    }
#endif
}
#endif  //!USE_SYSRES_MLT
#endif
