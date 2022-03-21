/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2013 RDK Management
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
#include <string.h>
#include <sys/mman.h>
#include "rdk_debug.h"
#include "rmf_osal_mem.h"
#include "rmf_osal_sync.h"
#include "rmfStaticMemoryPool.h"

#ifdef __cplusplus
extern "C" {
#endif 

#define RMF_MEM_POOL_INTEGRITY_SIGNATURE  0x12345678
#define RMF_MEM_POOL_END_MARKER_0         0x87
#define RMF_MEM_POOL_END_MARKER_1         0xBA
#define RMF_MEM_POOL_END_MARKER_2         0xDC
#define RMF_MEM_POOL_END_MARKER_3         0xFE
#define RMF_MEM_POOL_STATS_THROTTLE_INTERVAL 1000000
#define StrnCpyM(pDest, pSrc, nDestCapacity)            \
            strncpy(pDest, pSrc, nDestCapacity);\
            pDest[nDestCapacity-1] = '\0';

#define RMF_MEM_POOL_ARRAY_ITEM_COUNT(array)   (sizeof(array)/sizeof(array[0]))

#define RMF_MEM_POOL_ALLOCATE_FROM_SUB_POOL(type)                                                                                                        \
        /* Check if size is compatible AND also check if any blocks are available. */                                                                   \
        if((nSize <= sizeof(pMemPool->aSubPool##type.aBlocks[0])) &&                                                                                    \
           (pMemPool->aSubPool##type.nUsedBlockCount < RMF_MEM_POOL_ARRAY_ITEM_COUNT(pMemPool->aSubPool##type.aBlocks)))                                 \
        {                                                                                                                                               \
            unsigned int iBlock = 0;                                                                                                                    \
            /* Search for an available block. */                                                                                                        \
            for(iBlock = 0; iBlock < RMF_MEM_POOL_ARRAY_ITEM_COUNT(pMemPool->aSubPool##type.aBlocks); iBlock++)                                          \
            {                                                                                                                                           \
                /* Check if block is used. */                                                                                                           \
                if(0 == pMemPool->aSubPool##type.aiUsedBlocks[iBlock])                                                                                  \
                {                                                                                                                                       \
                    /* Allocate block. */                                                                                                               \
                    pAllocatedBlock = pMemPool->aSubPool##type.aBlocks[iBlock];                                                                         \
                    /* Mark as used. */                                                                                                                 \
                    pMemPool->aSubPool##type.aiUsedBlocks[iBlock] = 1;                                                                                  \
                    /* Increment used count. */                                                                                                         \
                    pMemPool->aSubPool##type.nUsedBlockCount++;                                                                                         \
                    /* Register pool usage depth. */                                                                                                    \
                    if(iBlock+1 > pMemPool->aSubPool##type.nUsageDepth) pMemPool->aSubPool##type.nUsageDepth = iBlock+1;                                \
                    /* Register requested size. */                                                                                                      \
                    pMemPool->aSubPool##type.anReqSize[iBlock] = nSize;                                                                                 \
                    /* Add buffer end buffer marker. */                                                                                                 \
                    if((sizeof(pMemPool->aSubPool##type.aBlocks[iBlock]) - nSize) >= 4)                                                                 \
                    {                                                                                                                                   \
                        pMemPool->aSubPool##type.aBlocks[iBlock][nSize+0] = RMF_MEM_POOL_END_MARKER_0;                                                   \
                        pMemPool->aSubPool##type.aBlocks[iBlock][nSize+1] = RMF_MEM_POOL_END_MARKER_1;                                                   \
                        pMemPool->aSubPool##type.aBlocks[iBlock][nSize+2] = RMF_MEM_POOL_END_MARKER_2;                                                   \
                        pMemPool->aSubPool##type.aBlocks[iBlock][nSize+3] = RMF_MEM_POOL_END_MARKER_3;                                                   \
                    }                                                                                                                                   \
                    /* Indicate sucess. */                                                                                                              \
                    eRet = RMF_MEM_POOL_RESULT_SUCCESS;                                                                                                  \
                    /* Stop the search. */                                                                                                              \
                    break;                                                                                                                              \
                }                                                                                                                                       \
            }                                                                                                                                           \
        }                                                                                                                                               \

#define RMF_MEM_POOL_RELEASE_TO_SUB_POOL(type)                                                                                                           \
        /* Check if the block belongs to this pool. */                                                                                                  \
        if((pBlockToRelease >= pMemPool->aSubPool##type.aBlocks[0]) &&                                                                                  \
           (pBlockToRelease < pMemPool->aSubPool##type.aBlocks[RMF_MEM_POOL_ARRAY_ITEM_COUNT(pMemPool->aSubPool##type.aBlocks)]))                        \
        {                                                                                                                                               \
            /* Index the block. */                                                                                                                      \
            unsigned int iBlock = ((pBlockToRelease-(pMemPool->aSubPool##type.aBlocks[0]))/sizeof(pMemPool->aSubPool##type.aBlocks[0]));                \
            unsigned int nSize  = pMemPool->aSubPool##type.anReqSize[iBlock];                                                                           \
            /* Sanity Check. */                                                                                                                         \
            if(pBlockToRelease != pMemPool->aSubPool##type.aBlocks[iBlock])                                                                             \
            {                                                                                                                                           \
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.MEM", "%s: Unaligned block address supplied to Sub-Pool '%s' of Memory Pool '%s'.\n",                   \
                    __FUNCTION__, #type, pMemPool->szPoolName);                                                                                         \
            }                                                                                                                                           \
            if(0 == pMemPool->aSubPool##type.aiUsedBlocks[iBlock])                                                                                      \
            {                                                                                                                                           \
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.MEM", "%s: Detected double-free of block '0x%08X' to Sub-Pool '%s' of Memory Pool '%s'.\n",             \
                    __FUNCTION__, (unsigned int)(pBlockToRelease), #type, pMemPool->szPoolName);                                                        \
            }                                                                                                                                           \
            else                                                                                                                                        \
            {                                                                                                                                           \
                /* Decrement used count. */                                                                                                             \
                pMemPool->aSubPool##type.nUsedBlockCount--;                                                                                             \
            }                                                                                                                                           \
            /* Mark as free. */                                                                                                                         \
            pMemPool->aSubPool##type.aiUsedBlocks[iBlock] = 0;                                                                                          \
            /* Indicate sucess */                                                                                                                       \
            eRet = RMF_MEM_POOL_RESULT_SUCCESS;                                                                                                          \
            /* Check for buffer overrun. */                                                                                                             \
            if((sizeof(pMemPool->aSubPool##type.aBlocks[iBlock]) - nSize) >= 4)                                                                         \
            {                                                                                                                                           \
                if( (pMemPool->aSubPool##type.aBlocks[iBlock][nSize+0] != RMF_MEM_POOL_END_MARKER_0) ||                                                  \
                    (pMemPool->aSubPool##type.aBlocks[iBlock][nSize+1] != RMF_MEM_POOL_END_MARKER_1) ||                                                  \
                    (pMemPool->aSubPool##type.aBlocks[iBlock][nSize+2] != RMF_MEM_POOL_END_MARKER_2) ||                                                  \
                    (pMemPool->aSubPool##type.aBlocks[iBlock][nSize+3] != RMF_MEM_POOL_END_MARKER_3))                                                    \
                {                                                                                                                                       \
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.MEM",                                                                                               \
                        "%s: Buffer overrun detected in block '0x%08X' on release to Sub-Pool '%s' of Memory Pool '%s'.\n",                             \
                        __FUNCTION__, (unsigned int)(pBlockToRelease), #type, pMemPool->szPoolName);                                                    \
                }                                                                                                                                       \
            }                                                                                                                                           \
            /* Sanity Check. */                                                                                                                         \
            if((int)(pMemPool->aSubPool##type.nUsedBlockCount) < (int)(0))                                                                              \
            {                                                                                                                                           \
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.MEM", "%s: nUsedBlockCount='%u' for Sub-Pool '%s' of Memory Pool '%s'.\n",                              \
                    __FUNCTION__, pMemPool->aSubPool##type.nUsedBlockCount, #type, pMemPool->szPoolName);                                               \
            }                                                                                                                                           \
        }                                                                                                                                               \

#define RMF_MEM_POOL_CHECK_INTEGRITY_SIGNATURE(pMemPool, signature)                                                                                      \
    if(RMF_MEM_POOL_INTEGRITY_SIGNATURE != pMemPool->lIntegritySignature##signature)                                                                     \
    {                                                                                                                                                   \
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.MEM", "%s: Memory Pool '%s' integrity is compromized at position %d. Signature is now '0x%08X.\n",              \
            __FUNCTION__, pMemPool->szPoolName, signature, pMemPool->lIntegritySignature##signature);                                                   \
    }                                                                                                                                                   \

#define RMF_MEM_POOL_CHECK_INTEGRITY(pMemPool)                                                                                                           \
    if(1 != pMemPool->bInitialized)                                                                                                                     \
    {                                                                                                                                                   \
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.MEM", "%s: Memory Pool '%s' is not initialized.\n", __FUNCTION__, pMemPool->szPoolName);                        \
    }                                                                                                                                                   \
    RMF_MEM_POOL_CHECK_INTEGRITY_SIGNATURE(pMemPool, 1)                                                                                                  \
    RMF_MEM_POOL_CHECK_INTEGRITY_SIGNATURE(pMemPool, 2)                                                                                                  \
    RMF_MEM_POOL_CHECK_INTEGRITY_SIGNATURE(pMemPool, 3)                                                                                                  \

#define RMF_MEM_POOL_DUMP_SUB_POOL_STATS(pMemPool, type)                                                                                                 \
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS", "%s: Memory Pool '%s', Sub-Pool '%s' stats: nUsageDepth = %u, nUsedBlockCount = %u.\n",                        \
        __FUNCTION__, pMemPool->szPoolName, #type, pMemPool->aSubPool##type.nUsageDepth, pMemPool->aSubPool##type.nUsedBlockCount);                     \

#define RMF_MEM_POOL_DUMP_STATS(pMemPool)                                                                                                                                        \
    if(0 == ((pMemPool->iStatsThrottle++)%RMF_MEM_POOL_STATS_THROTTLE_INTERVAL))                                                                                                 \
    {                                                                                                                                                                           \
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS", "=============================================================================================================================\n");\
        RMF_MEM_POOL_DUMP_SUB_POOL_STATS(pMemPool, HK)                                                                                                                           \
        RMF_MEM_POOL_DUMP_SUB_POOL_STATS(pMemPool, 1K)                                                                                                                           \
        RMF_MEM_POOL_DUMP_SUB_POOL_STATS(pMemPool, 2K)                                                                                                                           \
        RMF_MEM_POOL_DUMP_SUB_POOL_STATS(pMemPool, 4K)                                                                                                                           \
        RMF_MEM_POOL_DUMP_SUB_POOL_STATS(pMemPool, 8K)                                                                                                                           \
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS", "=============================================================================================================================\n");\
    }                                                                                                                                                                           \

RMF_MEM_POOL_RESULT rmfFixedH124MemPoolInit(RMF_FIXED_H124_MEM_POOL * pMemPool, const char * pszPoolName)
{
    if(NULL != pMemPool)
    {
        memset(pMemPool, 0, sizeof(*pMemPool));
        pMemPool->bInitialized  = 1;
        pMemPool->eMemPoolType  = RMF_MEM_POOL_TYPE_STATIC;
        rmf_osal_mutexNew(&pMemPool->mtxLock);
        pMemPool->iStatsThrottle = 0;
        StrnCpyM(pMemPool->szPoolName, pszPoolName, sizeof(pMemPool->szPoolName));
        pMemPool->lIntegritySignature1  = RMF_MEM_POOL_INTEGRITY_SIGNATURE;
        pMemPool->lIntegritySignature2  = RMF_MEM_POOL_INTEGRITY_SIGNATURE;
        pMemPool->lIntegritySignature3  = RMF_MEM_POOL_INTEGRITY_SIGNATURE;
        return RMF_MEM_POOL_RESULT_SUCCESS;
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.MEM", "%s: pMemPool == NULL.\n", __FUNCTION__);
    }
    return RMF_MEM_POOL_RESULT_NULL_PARAM;
}

RMF_MEM_POOL_RESULT rmfFixedH124MemPoolDeInit(RMF_FIXED_H124_MEM_POOL * pMemPool)
{
    if(NULL != pMemPool)
    {
        if(1 != pMemPool->bInitialized)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.MEM", "%s: Memory Pool '%s' is not initialized.\n", __FUNCTION__, pMemPool->szPoolName);
            return RMF_MEM_POOL_RESULT_INVALID_PARAM;
        }
        pMemPool->bInitialized  = 0;
        pMemPool->eMemPoolType  = RMF_MEM_POOL_TYPE_INVALID;
        rmf_osal_mutexDelete(pMemPool->mtxLock);
        return RMF_MEM_POOL_RESULT_SUCCESS;
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.MEM", "%s: pMemPool == NULL.\n", __FUNCTION__);
    }
    return RMF_MEM_POOL_RESULT_NULL_PARAM;
}

RMF_MEM_POOL_RESULT rmfFixedH124MemPoolCreate(RMF_FIXED_H124_MEM_POOL ** ppMemPool, const char * pszPoolName, RMF_MEM_POOL_TYPE ePoolType)
{
    int nRet = 0;
    RMF_MEM_POOL_RESULT result = RMF_MEM_POOL_RESULT_SUCCESS;
    RMF_FIXED_H124_MEM_POOL * pMemPool = NULL;

    if(NULL == ppMemPool)
    {
        return RMF_MEM_POOL_RESULT_NULL_PARAM;
    }
    
    if(RMF_MEM_POOL_TYPE_PROTECTED == ePoolType)
    {
        nRet = posix_memalign((void**)&pMemPool, RMF_MEM_POOL_4K_BLOCK_SIZE, sizeof(RMF_FIXED_H124_MEM_POOL));
        if(0 != nRet)
        {
            pMemPool = NULL;
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.MEM", "%s: Unable to Create Memory Pool '%s', posix_memalign returned '%d'.\n", __FUNCTION__, pszPoolName, nRet);
            result = RMF_MEM_POOL_RESULT_ALLOC_FAILED;
        }
    }
    else if(RMF_MEM_POOL_TYPE_HEAP == ePoolType)
    {
	    rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(RMF_FIXED_H124_MEM_POOL),(void **)&pMemPool);
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.MEM", "%s: Invalid Memory Pool Type '%d' supplied for '%s'.\n", __FUNCTION__, ePoolType, pszPoolName);
        ePoolType = RMF_MEM_POOL_TYPE_HEAP;
    }
    
    if(NULL != pMemPool)
    {
        result = rmfFixedH124MemPoolInit(pMemPool, pszPoolName);
        pMemPool->eMemPoolType  = ePoolType;
        
        if(RMF_MEM_POOL_TYPE_PROTECTED == ePoolType)
        {
            nRet = mprotect(pMemPool->a4kProtectBlock1, sizeof(pMemPool->a4kProtectBlock1), PROT_READ);
            if(0 != nRet)
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.MEM", "%s: Could not enable protection for '%s', mprotect returned '%d'.\n", __FUNCTION__, pszPoolName, nRet);
            }
        }
        RMF_MEM_POOL_CHECK_INTEGRITY(pMemPool);
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.MEM", "%s: Unable to Create Memory Pool '%s'.\n", __FUNCTION__, pszPoolName);
        result = RMF_MEM_POOL_RESULT_ALLOC_FAILED;
    }
    
    *ppMemPool = pMemPool;
    return result;
}

RMF_MEM_POOL_RESULT rmfFixedH124MemPoolDestroy(RMF_FIXED_H124_MEM_POOL * pMemPool)
{
    RMF_MEM_POOL_RESULT eRet = RMF_MEM_POOL_RESULT_NULL_PARAM;
    int nRet = 0;
    if(NULL != pMemPool)
    {
        if(1 != pMemPool->bInitialized)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.MEM", "%s: Memory Pool '%s' is not initialized.\n", __FUNCTION__, pMemPool->szPoolName);
            return RMF_MEM_POOL_RESULT_INVALID_PARAM;
        }
        if(RMF_MEM_POOL_TYPE_PROTECTED == pMemPool->eMemPoolType)
        {
            // unprotect the protect block
            nRet = mprotect(pMemPool->a4kProtectBlock1, sizeof(pMemPool->a4kProtectBlock1), PROT_READ | PROT_WRITE);
            if(0 != nRet)
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.MEM", "%s: Could not disable protection for '%s', mprotect returned '%d'.\n", __FUNCTION__, pMemPool->szPoolName, nRet);
            }
        }
        // De-initialize the memory pool
        eRet = rmfFixedH124MemPoolDeInit(pMemPool);
        // The following will crash if pMemPool not allocated by rmfFixedH124MemPoolCreate();
        // It may also crash on platforms where a pointer returned by posix_memalign() should not be passed to free().
        rmf_osal_memFreeP(RMF_OSAL_MEM_POD,pMemPool);
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.MEM", "%s: pMemPool == NULL.\n", __FUNCTION__);
    }
    return eRet;
}

RMF_MEM_POOL_RESULT rmfFixedH124MemPoolAllocFromPool(RMF_FIXED_H124_MEM_POOL * pMemPool, unsigned int nSize, void ** ppAllocatedBlock)
{
    void * pAllocatedBlock = NULL;
    RMF_MEM_POOL_RESULT eRet = RMF_MEM_POOL_RESULT_EXHAUSTED;
    
    if(NULL == pMemPool)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.MEM", "%s: pMemPool == NULL.\n", __FUNCTION__);
        return RMF_MEM_POOL_RESULT_NULL_PARAM;
    }
        
    if(NULL == ppAllocatedBlock)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.MEM", "%s: ppAllocatedBlock == NULL.\n", __FUNCTION__);
        return RMF_MEM_POOL_RESULT_NULL_PARAM;
    }
    
    *ppAllocatedBlock = NULL;
        
    RMF_MEM_POOL_CHECK_INTEGRITY(pMemPool);
    
    rmf_osal_mutexAcquire(pMemPool->mtxLock);
    {
        RMF_MEM_POOL_ALLOCATE_FROM_SUB_POOL(HK)
        else
        RMF_MEM_POOL_ALLOCATE_FROM_SUB_POOL(1K)
        else
        RMF_MEM_POOL_ALLOCATE_FROM_SUB_POOL(2K)
        else
        RMF_MEM_POOL_ALLOCATE_FROM_SUB_POOL(4K)
        else
        RMF_MEM_POOL_ALLOCATE_FROM_SUB_POOL(8K)
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.MEM", "%s: Unable to allocate '%u' bytes from Memory Pool '%s'.\n", __FUNCTION__, nSize, pMemPool->szPoolName);
        }
        
        if(NULL == pAllocatedBlock)
        {
            if(nSize > RMF_MEM_POOL_4K_BLOCK_SIZE)
            {
                eRet = RMF_MEM_POOL_RESULT_SIZE_NOT_SUPPORTED;
            }
        }
            
        RMF_MEM_POOL_DUMP_STATS(pMemPool);
    }
    rmf_osal_mutexRelease(pMemPool->mtxLock);

    *ppAllocatedBlock = pAllocatedBlock;
    return eRet;
}

RMF_MEM_POOL_RESULT rmfFixedH124MemPoolReleaseToPool(RMF_FIXED_H124_MEM_POOL * pMemPool, void * _pBlockToRelease)
{
    unsigned char * pBlockToRelease = (unsigned char*)_pBlockToRelease;
    RMF_MEM_POOL_RESULT eRet = RMF_MEM_POOL_RESULT_NOT_FROM_THIS_POOL;
    
    if(NULL == pMemPool)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.MEM", "%s: pMemPool == NULL.\n", __FUNCTION__);
        return RMF_MEM_POOL_RESULT_NULL_PARAM;
    }
        
    RMF_MEM_POOL_CHECK_INTEGRITY(pMemPool);
    
    if(NULL != pBlockToRelease)
    {
        rmf_osal_mutexAcquire(pMemPool->mtxLock);
        {
            RMF_MEM_POOL_RELEASE_TO_SUB_POOL(HK)
            else
            RMF_MEM_POOL_RELEASE_TO_SUB_POOL(1K)
            else
            RMF_MEM_POOL_RELEASE_TO_SUB_POOL(2K)
            else
            RMF_MEM_POOL_RELEASE_TO_SUB_POOL(4K)
            else
            RMF_MEM_POOL_RELEASE_TO_SUB_POOL(8K)
            else
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.MEM", "%s: Unable to release block '0x%08X' to Memory Pool '%s'.\n", __FUNCTION__, (unsigned int)(pBlockToRelease), pMemPool->szPoolName);
            }
            
            RMF_MEM_POOL_DUMP_STATS(pMemPool);
        }
        rmf_osal_mutexRelease(pMemPool->mtxLock);
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.MEM", "%s: pBlockToRelease == NULL.\n", __FUNCTION__);
        return RMF_MEM_POOL_RESULT_NULL_PARAM;
    }
    return eRet;
}

#ifdef __cplusplus
}
#endif 
