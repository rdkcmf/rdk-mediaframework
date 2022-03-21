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

#include <stdlib.h>
#include <sys/mman.h>
#include <rmf_osal_sync.h>

#ifdef __cplusplus
extern "C" {
#endif 


#define RMF_MEM_POOL_STR_SIZE        256

#define RMF_MEM_POOL_HK_BLOCK_SIZE   512
#define RMF_MEM_POOL_1K_BLOCK_SIZE   1024
#define RMF_MEM_POOL_2K_BLOCK_SIZE   2048
#define RMF_MEM_POOL_4K_BLOCK_SIZE   4096
#define RMF_MEM_POOL_8K_BLOCK_SIZE   8192

#define RMF_MEM_POOL_HK_BLOCK_COUNT  256
#define RMF_MEM_POOL_1K_BLOCK_COUNT  128
#define RMF_MEM_POOL_2K_BLOCK_COUNT  64
#define RMF_MEM_POOL_4K_BLOCK_COUNT  16
#define RMF_MEM_POOL_8K_BLOCK_COUNT  8

#define RMF_DECLARE_MEM_POOL_SUB(type)                                                           \
typedef struct _RMF_MEM_POOL_SUB_##type                                                          \
{                                                                                               \
    unsigned int  nUsedBlockCount;                                                              \
    unsigned int  nUsageDepth;                                                                  \
    unsigned char aiUsedBlocks[RMF_MEM_POOL_##type##_BLOCK_COUNT];                               \
    unsigned int  anReqSize[RMF_MEM_POOL_##type##_BLOCK_COUNT];                                  \
    unsigned char aBlocks[RMF_MEM_POOL_##type##_BLOCK_COUNT][RMF_MEM_POOL_##type##_BLOCK_SIZE];   \
    unsigned char a4kGuardBlock[RMF_MEM_POOL_4K_BLOCK_SIZE];                                     \
}RMF_MEM_POOL_SUB_##type;                                                                        \

RMF_DECLARE_MEM_POOL_SUB(HK);
RMF_DECLARE_MEM_POOL_SUB(1K);
RMF_DECLARE_MEM_POOL_SUB(2K);
RMF_DECLARE_MEM_POOL_SUB(4K);
RMF_DECLARE_MEM_POOL_SUB(8K);

typedef enum _RMF_MEM_POOL_TYPE
{
    RMF_MEM_POOL_TYPE_INVALID,   // uninitialized
    RMF_MEM_POOL_TYPE_STATIC,    // global static
    RMF_MEM_POOL_TYPE_HEAP,      // process heap
    RMF_MEM_POOL_TYPE_PROTECTED, // process heap and protected
    RMF_MEM_POOL_TYPE_HEAP_LIKE, // heap like implementation
    
}RMF_MEM_POOL_TYPE;

typedef enum _RMF_MEM_POOL_RESULT
{
    RMF_MEM_POOL_RESULT_SUCCESS,
    RMF_MEM_POOL_RESULT_NULL_PARAM,
    RMF_MEM_POOL_RESULT_INVALID_PARAM,
    RMF_MEM_POOL_RESULT_ALLOC_FAILED,
    RMF_MEM_POOL_RESULT_EXHAUSTED,
    RMF_MEM_POOL_RESULT_SIZE_NOT_SUPPORTED,
    RMF_MEM_POOL_RESULT_NOT_FROM_THIS_POOL,
    
}RMF_MEM_POOL_RESULT;

#define RMF_INSTANTIATE_MEM_POOL_SUB(type)   RMF_MEM_POOL_SUB_##type  aSubPool##type

typedef struct _RMF_FIXED_H124_MEM_POOL
{
    unsigned char a4kGuardBlock1[RMF_MEM_POOL_4K_BLOCK_SIZE];
    unsigned char a4kProtectBlock1[RMF_MEM_POOL_4K_BLOCK_SIZE];
    unsigned int  lIntegritySignature1;
    long          bInitialized;
    RMF_MEM_POOL_TYPE eMemPoolType;
    rmf_osal_Mutex	mtxLock;
    unsigned int  iStatsThrottle;
    unsigned int  lIntegritySignature2;
    RMF_INSTANTIATE_MEM_POOL_SUB(HK);
    RMF_INSTANTIATE_MEM_POOL_SUB(1K);
    RMF_INSTANTIATE_MEM_POOL_SUB(2K);
    RMF_INSTANTIATE_MEM_POOL_SUB(4K);
    RMF_INSTANTIATE_MEM_POOL_SUB(8K);
    unsigned int  lIntegritySignature3;
    char          szPoolName[RMF_MEM_POOL_STR_SIZE];
    unsigned char a4kGuardBlock2[RMF_MEM_POOL_4K_BLOCK_SIZE];
}RMF_FIXED_H124_MEM_POOL;

RMF_MEM_POOL_RESULT rmfFixedH124MemPoolInit(RMF_FIXED_H124_MEM_POOL * pMemPool, const char * pszPoolName);
RMF_MEM_POOL_RESULT rmfFixedH124MemPoolDeInit(RMF_FIXED_H124_MEM_POOL * pMemPool);
RMF_MEM_POOL_RESULT rmfFixedH124MemPoolCreate(RMF_FIXED_H124_MEM_POOL ** ppMemPool, const char * pszPoolName, RMF_MEM_POOL_TYPE ePoolType);
RMF_MEM_POOL_RESULT rmfFixedH124MemPoolDestroy(RMF_FIXED_H124_MEM_POOL * pMemPool);
RMF_MEM_POOL_RESULT rmfFixedH124MemPoolAllocFromPool(RMF_FIXED_H124_MEM_POOL * pMemPool, unsigned int nSize, void ** ppAllocatedBlock);
RMF_MEM_POOL_RESULT rmfFixedH124MemPoolReleaseToPool(RMF_FIXED_H124_MEM_POOL * pMemPool, void * _pBlockToRelease);

#ifdef __cplusplus
}
#endif 
