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


#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string> // std::string
#include <fstream>
#include <iostream>

#include "rmf_osal_scriptexecutor.h"
#include "rmf_osal_resreg.h"
//#include "vlMemCpy.h"
//#include "cMsgQ.h"
#include "vlMap.h"
//#include "cThreadPool.h"
#include "vlMutex.h"
//#include "utilityMacros.h"
//#include "mpeos_dbg.h"
#include "pthread.h"
#include "rdk_debug.h"
#include "rdk_utils.h"
#include "rmf_osal_thread.h"

#ifdef GCC4_XXX
#include <map>
#include <list>
#else
#include <map.h>
#include <list.h>
#endif

#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif

using namespace std;
#define RMF_OSAL_DUMP_FLAG "/tmp/stat_printed"
typedef void* (*THREADFP)(void *);

#if 0 //SS - Commented
static long vl_gnResRegQueue = 0;
#endif

extern uint32_t rdk_g_logControlTbl[];
extern "C" void rdk_dbg_priv_SetLogLevelString(const char* pszModuleName, const char* pszLogLevels);
#if 0 //SS Commented
extern "C" void mpeos_TestSetDeInterlacePolicy(int policyIndex);
extern "C" void vlStartRemoteKeySequencer();
extern "C" void vlStopRemoteKeySequencer();
extern "C" void rmf_dbgNotifyLogRotation(void);
extern "C" int dbgApiCallerInterface();
extern "C" uint64_t vlg_vlOsMapIoToMemCount;	// in the vlOsalMemmap.cpp
extern "C" uint64_t vlg_vlOsUnMapIoToMemCount;	// in the vlOsalMemmap.cpp
#endif

#ifdef DEBUG_THREADS_SYS_TID
#define RMF_OSAL_RES_REG_USE_LOCAL_STORE
#endif

#ifdef RMF_OSAL_RES_REG_USE_LOCAL_STORE
static vlMap vl_gLocalResourceRegistrar;
#endif // RMF_OSAL_RES_REG_USE_LOCAL_STORE
static vlMap vl_gLocalThreadRegistrar;
static vlMutex vlg_threadRegLock;
static vlMutex vlg_threadDescriptorLock;
static vlMap vl_gMpeosThreadDescriptors;
static unsigned long vlg_logForTidOnly = 0;
static int vlg_bMutexDbInitialized = 0;
static pthread_mutex_t vlg_mutexRegLock = PTHREAD_MUTEX_INITIALIZER; // WARNING: should use only posix mutex here
static pthread_mutex_t vlg_mutexMonLock = PTHREAD_MUTEX_INITIALIZER; // WARNING: should use only posix mutex here
static unsigned int vlg_nMutexCount = 0;
static int vlg_nMutexWaitCount = 0;
static unsigned int vlg_nThreadCount = 0;

typedef struct _RMF_OSAL_THREAD_INFO
{
    unsigned long ulThreadId;
    unsigned long ulStackStart;
    unsigned long ulStackMax;
    unsigned long ulCreationTime;
    string strThreadName;
    
}RMF_OSAL_THREAD_INFO;

typedef struct _RMF_OSAL_MUTEX_INFO
{
    unsigned long  ulMutexId;
    unsigned long  ulOwnerThreadId;
    signed long    nAcquireCount;
    const char *   pszMutexName;
    const char *   pszFunction;
    unsigned long  nLineNo;
    unsigned long  ulFirstAcquiredTimeStamp;
    unsigned long  ulLastAcquiredTimeStamp;
    
}RMF_OSAL_MUTEX_INFO;

//#define RMF_OSAL_ENABLE_MUTEX_DB_TYPE_LIST
//#define RMF_OSAL_ENABLE_MUTEX_DB_TYPE_MAP

#if defined(RMF_OSAL_ENABLE_MUTEX_DB_TYPE_LIST)
static list<RMF_OSAL_MUTEX_INFO*> vl_gMpeosMutexRegistrar;
#endif // defined(RMF_OSAL_ENABLE_MUTEX_DB_TYPE_LIST)

#if defined(RMF_OSAL_ENABLE_MUTEX_DB_TYPE_MAP)
static map<unsigned long, RMF_OSAL_MUTEX_INFO*> vl_gMpeosMutexRegistrar;
#endif // defined(RMF_OSAL_ENABLE_MUTEX_DB_TYPE_MAP)

#if defined(RMF_OSAL_ENABLE_MUTEX_DB_TYPE_LIST)
RMF_OSAL_MUTEX_INFO* rmf_osal_FindMutex(unsigned long nMutexId)
{
    list<RMF_OSAL_MUTEX_INFO*>::iterator it;

    for ( it=vl_gMpeosMutexRegistrar.begin() ; it != vl_gMpeosMutexRegistrar.end(); it++ )
    {
        if(nMutexId == (*it)->ulMutexId)
        {
            return *it;
        }
    }
  
    return NULL;
}

RMF_OSAL_MUTEX_INFO* rmf_osal_DeleteMutex(unsigned long nMutexId)
{
    list<RMF_OSAL_MUTEX_INFO*>::iterator it;

    for ( it=vl_gMpeosMutexRegistrar.begin() ; it != vl_gMpeosMutexRegistrar.end(); it++ )
    {
        if(nMutexId == (*it)->ulMutexId)
        {
            delete (*it);
            vl_gMpeosMutexRegistrar.erase(it);
            break;
        }
    }
  
    return NULL;
}

#endif // defined(RMF_OSAL_ENABLE_MUTEX_DB_TYPE_LIST)

void rmf_osal_MutexRegCreated(unsigned long nMutexId, const char * pszMutexName, const char * pszFunction, unsigned long nLineNo)
{
    if(!vlg_bMutexDbInitialized) return;
    
    pthread_mutex_lock(&vlg_mutexRegLock);
    {
        vlg_nMutexCount++;
    }
    pthread_mutex_unlock(&vlg_mutexRegLock);
    
#if defined(RMF_OSAL_ENABLE_MUTEX_DB_TYPE_LIST)
    pthread_mutex_lock(&vlg_mutexRegLock);
    {
        RMF_OSAL_MUTEX_INFO* pMutexInfo = rmf_osal_FindMutex(nMutexId);
        if(NULL == pMutexInfo)
        {
            pMutexInfo = new RMF_OSAL_MUTEX_INFO;
            if(NULL == pMutexInfo) return;
            
            pMutexInfo->ulMutexId       = nMutexId;
            pMutexInfo->pszMutexName    = pszMutexName;
            pMutexInfo->ulOwnerThreadId = 0;
            pMutexInfo->nAcquireCount   = 0;
            pMutexInfo->pszFunction     = pszFunction;
            pMutexInfo->nLineNo         = nLineNo;
            vl_gMpeosMutexRegistrar.push_front(pMutexInfo);
        }
    }
    pthread_mutex_unlock(&vlg_mutexRegLock);
#endif // defined(RMF_OSAL_ENABLE_MUTEX_DB_TYPE_LIST)
    
#if defined(RMF_OSAL_ENABLE_MUTEX_DB_TYPE_MAP)
    //unsigned long nThreadId = syscall(SYS_gettid);
    RMF_OSAL_MUTEX_INFO * pMutexInfo = NULL;
    pszMutexName = pszMutexName; // for unused param warning

    pthread_mutex_lock(&vlg_mutexRegLock);
    {
        map<unsigned long, RMF_OSAL_MUTEX_INFO*>::iterator it = vl_gMpeosMutexRegistrar.find(nMutexId);
        if(vl_gMpeosMutexRegistrar.end() != it)
        {
            pMutexInfo = (*it).second;
        }
        if(NULL == pMutexInfo)
        {
            pMutexInfo = new RMF_OSAL_MUTEX_INFO;
            if(NULL == pMutexInfo) return;
            
            pMutexInfo->ulOwnerThreadId             = 0;
            pMutexInfo->nAcquireCount               = 0;
            pMutexInfo->pszFunction                 = NULL;
            pMutexInfo->nLineNo                     = 0;
            vl_gMpeosMutexRegistrar[nMutexId] = pMutexInfo;
        }
    }
    pthread_mutex_unlock(&vlg_mutexRegLock);
#endif // defined(RMF_OSAL_ENABLE_MUTEX_DB_TYPE_MAP)
}

void rmf_osal_MutexRegWaitStartImpl(unsigned long nMutexId)
{
    if(!vlg_bMutexDbInitialized) return;
    
    //commented to avoid context switch problems //pthread_mutex_lock(&vlg_mutexMonLock);
    {
        vlg_nMutexWaitCount++;
    }
    //commented to avoid context switch problems //pthread_mutex_unlock(&vlg_mutexMonLock);
    
}

void rmf_osal_MutexRegWaitStopImpl(unsigned long nMutexId)
{
    if(!vlg_bMutexDbInitialized) return;
    
    //commented to avoid context switch problems //pthread_mutex_lock(&vlg_mutexMonLock);
    {
        vlg_nMutexWaitCount--;
    }
    //commented to avoid context switch problems //pthread_mutex_unlock(&vlg_mutexMonLock);
    
}

void rmf_osal_MutexRegAcquiredImpl(unsigned long nMutexId, const char * pszFunction, unsigned long nLineNo)
{
#if defined(RMF_OSAL_ENABLE_MUTEX_DB_TYPE_MAP)
    if(!vlg_bMutexDbInitialized) return;
    unsigned long nThreadId = syscall(SYS_gettid);
    if(NULL == pszFunction) pszFunction = "(no name)";
    RMF_OSAL_MUTEX_INFO * pMutexInfo = NULL;

    pthread_mutex_lock(&vlg_mutexRegLock);
    {
        map<unsigned long, RMF_OSAL_MUTEX_INFO*>::iterator it = vl_gMpeosMutexRegistrar.find(nMutexId);
        if(vl_gMpeosMutexRegistrar.end() != it)
        {
            pMutexInfo = (*it).second;
        }
        if(NULL == pMutexInfo)
        {
            // not found so try creating entry
            pMutexInfo = new RMF_OSAL_MUTEX_INFO;
            if(NULL == pMutexInfo) return;
            
            pMutexInfo->ulOwnerThreadId             = 0;
            pMutexInfo->nAcquireCount               = 0;
            pMutexInfo->pszFunction                 = NULL;
            pMutexInfo->nLineNo                     = 0;
            vl_gMpeosMutexRegistrar[nMutexId] = pMutexInfo;
        }
        if(NULL != pMutexInfo)
        {
            pMutexInfo->nAcquireCount++;
            pMutexInfo->ulOwnerThreadId             = nThreadId;
            pMutexInfo->pszFunction                 = pszFunction;
            pMutexInfo->nLineNo                     = nLineNo;
            pMutexInfo->ulLastAcquiredTimeStamp     = time(NULL);
            if(1 == pMutexInfo->nAcquireCount)
            {
                pMutexInfo->ulFirstAcquiredTimeStamp = pMutexInfo->ulLastAcquiredTimeStamp;
            }
        }
    }
    pthread_mutex_unlock(&vlg_mutexRegLock);
#endif // defined(RMF_OSAL_ENABLE_MUTEX_DB_TYPE_MAP)
}

void rmf_osal_MutexRegReleasedImpl(unsigned long nMutexId)
{
#if defined(RMF_OSAL_ENABLE_MUTEX_DB_TYPE_MAP)
    if(!vlg_bMutexDbInitialized) return;
    unsigned long nThreadId = syscall(SYS_gettid);
    RMF_OSAL_MUTEX_INFO * pMutexInfo = NULL;

    pthread_mutex_lock(&vlg_mutexRegLock);
    {
        map<unsigned long, RMF_OSAL_MUTEX_INFO*>::iterator it = vl_gMpeosMutexRegistrar.find(nMutexId);
        if(vl_gMpeosMutexRegistrar.end() != it)
        {
            pMutexInfo = (*it).second;
        }
        if(NULL != pMutexInfo)
        {
            if(nThreadId == pMutexInfo->ulOwnerThreadId)
            {
                if(pMutexInfo->nAcquireCount <= 0)
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SYS","%s: Warning: Mutex id:%d acquired in funtion '%s', line %d has already been released\n", __FUNCTION__,
                                nMutexId, pMutexInfo->pszFunction, pMutexInfo->nLineNo);
                }
                pMutexInfo->nAcquireCount--;
                if(pMutexInfo->nAcquireCount <= 0)
                {
                    pMutexInfo->nAcquireCount = 0;
                }
            }
            else
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SYS","%s: Warning: Mutex id:%d not owned by Thread:%d\n", __FUNCTION__, nMutexId, nThreadId);
            }
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SYS","%s: Warning: Mutex id:%d not found\n", __FUNCTION__, nMutexId);
        }
    }
    pthread_mutex_unlock(&vlg_mutexRegLock);
#endif // defined(RMF_OSAL_ENABLE_MUTEX_DB_TYPE_MAP)
}

void rmf_osal_MutexRegDestroyed(unsigned long nMutexId)
{
    pthread_mutex_lock(&vlg_mutexRegLock);
    {
        vlg_nMutexCount--;
    }
    pthread_mutex_unlock(&vlg_mutexRegLock);
    
    
#if defined(RMF_OSAL_ENABLE_MUTEX_DB_TYPE_LIST)
    pthread_mutex_lock(&vlg_mutexRegLock);
    {
        rmf_osal_DeleteMutex(nMutexId);
    }
    pthread_mutex_unlock(&vlg_mutexRegLock);
#endif // defined(RMF_OSAL_ENABLE_MUTEX_DB_TYPE_LIST)
    
#if defined(RMF_OSAL_ENABLE_MUTEX_DB_TYPE_MAP)
    if(!vlg_bMutexDbInitialized) return;
    //unsigned long nThreadId = syscall(SYS_gettid);
    RMF_OSAL_MUTEX_INFO * pMutexInfo = NULL;

    pthread_mutex_lock(&vlg_mutexRegLock);
    {
        map<unsigned long, RMF_OSAL_MUTEX_INFO*>::iterator it = vl_gMpeosMutexRegistrar.find(nMutexId);
        if(vl_gMpeosMutexRegistrar.end() != it)
        {
            pMutexInfo = (*it).second;
        }
        if(NULL != pMutexInfo)
        {
            delete pMutexInfo;
            vl_gMpeosMutexRegistrar.erase(nMutexId);
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SYS","%s: Warning: Mutex id:%d not found\n", __FUNCTION__, nMutexId);
        }
    }
    pthread_mutex_unlock(&vlg_mutexRegLock);
#endif // defined(RMF_OSAL_ENABLE_MUTEX_DB_TYPE_MAP)
}

int rmf_osal_StartThreadInfoServer();

static void rmf_osal_ResRegInit()
{
#if 0 //SS Commented
    if(0 == vl_gnResRegQueue)
    {
        vl_gnResRegQueue = cMsgQCreate("vlResReg_msgQ", 1024, sizeof(RMF_OSAL_RMF_OSAL_RES_INFO), SYS_MSGQ_FIFO);
    }
#endif
}

static void rmf_osal_ResRegLookup()
{
#if 0 //SS Commented
    if(0 == vl_gnResRegQueue)
    {
        static int bLookupFailed = 0;
        if(bLookupFailed) return; // assume this functionality is disabled.
        vl_gnResRegQueue = cMsgQLookup("vlResReg_msgQ");
        if(0 == vl_gnResRegQueue)
        {
            bLookupFailed = 1;
        }
    }
#endif
}

void rmf_osal_ResRegSet(RMF_OSAL_RES_TYPE eResType, unsigned long nId, const char * pszName, int nMax, int nVal)
{
#if 0 //SS Commented
    vlResRegLookup();
    if(0 != vl_gnResRegQueue)
    {
        RMF_OSAL_RES_INFO resInfo;
    
        if(NULL == pszName)
        {
            strncpy(resInfo.szName, "", sizeof(resInfo.szName));
        }
        else
        {
            strncpy(resInfo.szName, pszName, sizeof(resInfo.szName));
        }
        
        /*if(RMF_OSAL_RES_TYPE_LMQ_MSG_Q == eResType)
        {
            struct msqid_ds msgq_stat;
            memset(&msgq_stat, 0, sizeof(msgq_stat));
            msgctl(nId, IPC_STAT, &msgq_stat);
            nVal = msgq_stat.msg_qnum;
            nMax = msgq_stat.msg_qbytes;
            if(0 != nVal) nMax /= nVal;
        }*/
        
        resInfo.eResType = eResType;
        resInfo.nId = nId;
        resInfo.nVal = nVal;
        resInfo.nMax = nMax;
        resInfo.nPeak = nVal;
        cMsgQWrite(vl_gnResRegQueue, (char*)&resInfo, sizeof(resInfo), SYS_MSGQ_NO_WAIT, SYS_MSGQ_PRI_NORMAL);
    }
    else
    {
        //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS","rmf_osal_ResRegSet: vlResRegLookup() failed \n");
    }
#endif    
    #ifdef RMF_OSAL_RES_REG_USE_LOCAL_STORE
    {
        RMF_OSAL_RES_INFO * pResInfo = (RMF_OSAL_RES_INFO*)vl_gLocalResourceRegistrar.getElementAt((vlMap::KEY)nId);
        if(NULL == pResInfo)
        {
            pResInfo = (RMF_OSAL_RES_INFO *) vlMalloc(sizeof(*pResInfo));
            if(NULL == pResInfo) return;
            if(NULL == pszName)
            {
                //vlStrCpy(pResInfo->szName, "", sizeof(pResInfo->szName));
                strncpy(pResInfo->szName, "", sizeof(pResInfo->szName));
            }
            else
            {
                //vlStrCpy(pResInfo->szName, pszName, sizeof(pResInfo->szName));
                strncpy(pResInfo->szName, pszName, sizeof(pResInfo->szName));
            }
        }
        if(NULL == pResInfo) return;
    
        pResInfo->eResType = eResType;
        pResInfo->nId = nId;
        pResInfo->nVal = nVal;
        pResInfo->nMax = nMax;
        pResInfo->nPeak = nVal;
        
        vl_gLocalResourceRegistrar.setElementAt((vlMap::KEY)nId, (vlMap::ELEMENT)pResInfo);
    }
    #endif // RMF_OSAL_RES_REG_USE_LOCAL_STORE
}

int rmf_osal_ResRegGet(int nTimeout, RMF_OSAL_RES_INFO * pResInfo)
{
    if(NULL != pResInfo)
    {
        rmf_osal_ResRegInit();
#if 0 //SS Commented
        if(0 != vl_gnResRegQueue)
        {
            if(0 == nTimeout) nTimeout = SYS_MSGQ_WAIT_FOREVER;
            return cMsgQRead(vl_gnResRegQueue, (char*)pResInfo, sizeof(*pResInfo), nTimeout);
        }
#endif
    }
    return 0;
}

void rmf_osal_ResRegLookup(unsigned long nId, RMF_OSAL_RES_INFO * pResInfoOut)
{
    #ifdef RMF_OSAL_RES_REG_USE_LOCAL_STORE
    RMF_OSAL_RES_INFO * pResInfo = (RMF_OSAL_RES_INFO*)vl_gLocalResourceRegistrar.getElementAt((vlMap::KEY)nId);
    if((NULL != pResInfo) && (NULL != pResInfoOut)) memcpy(pResInfoOut, pResInfo, sizeof(*pResInfoOut));
    #endif // RMF_OSAL_RES_REG_USE_LOCAL_STORE
}

void rmf_osal_ResRegRemove(RMF_OSAL_RES_TYPE eResType, unsigned long nId)
{
    #ifdef RMF_OSAL_RES_REG_USE_LOCAL_STORE
    RMF_OSAL_RES_INFO *pResInfo  = ((RMF_OSAL_RES_INFO*)(vl_gLocalResourceRegistrar.getElementAt(nId)));
    if(NULL != pResInfo) vlFree(pResInfo);
    vl_gLocalResourceRegistrar.removeElementAt((vlMap::KEY)nId);
    #endif // RMF_OSAL_RES_REG_USE_LOCAL_STORE
}

unsigned long rmf_osal_GetUptime()
{
    FILE *fp = fopen("/proc/uptime", "r");
    unsigned long secs   = 0;
    if (NULL != fp)
    {
        if ( 0 == fscanf(fp, "%ld", &secs))
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SYS","%s: fscanf failed\n", __FUNCTION__);
            secs = 0;
        }
        fclose(fp);
    }
    return secs;
}

unsigned long rmf_osal_GetFreePhysMem()
{
    static FILE *fp = fopen("/proc/meminfo", "r");
    unsigned long mem   = 0;
    char szBuffer[32]; // dummy buffer
    if (NULL != fp)
    {
    	setvbuf(fp, (char *) NULL, _IONBF, 0);
        fseek(fp, 0, SEEK_SET);
        if(0 == fscanf(fp, "%s%s%s%s%ld", szBuffer, szBuffer, szBuffer, szBuffer, &mem))
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SYS","%s: fscanf failed, mem=%d\n", __FUNCTION__, mem);
            mem = 0;
        }
        // changed to static //fclose(fp);
    }
    return mem;
}

void rmf_osal_ThreadRegSet(RMF_OSAL_RES_TYPE eResType, unsigned long nId, const char * pszName, int nStackMax, int nVal)
{
    if(0 == nId) nId = syscall(SYS_gettid);
    if(NULL == pszName) pszName = "(no name)";
    RMF_OSAL_THREAD_INFO * pThreadInfo = new RMF_OSAL_THREAD_INFO;
    if(NULL == pThreadInfo) return;
    
    pThreadInfo->ulThreadId     = nId;
    pThreadInfo->ulStackStart   = (unsigned long)(&eResType);
    pThreadInfo->ulStackMax     = nStackMax;
    pThreadInfo->strThreadName  = pszName;
    pThreadInfo->ulCreationTime = rmf_osal_GetUptime();

    {VL_AUTO_LOCK(vlg_threadRegLock);
        RMF_OSAL_THREAD_INFO *pThreadInfoPreExisting  = ((RMF_OSAL_THREAD_INFO*)(vl_gLocalThreadRegistrar.getElementAt(nId)));
        if(NULL == pThreadInfoPreExisting)
        {
            vl_gLocalThreadRegistrar.setElementAt((vlMap::KEY)nId, (vlMap::ELEMENT)pThreadInfo);
        }
        else
        {
            *pThreadInfoPreExisting = *pThreadInfo;
            delete pThreadInfo;
        }
    }
    
    static int bServerStarted = 0;
    if(!bServerStarted)
    {
        bServerStarted = 1;

        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS","%s: Starting the ThreadInfoServer\n", __FUNCTION__);
        rmf_osal_StartThreadInfoServer();
    }
}

void rmf_osal_ThreadRegRemove(RMF_OSAL_RES_TYPE eResType, unsigned long nId)
{VL_AUTO_LOCK(vlg_threadRegLock);

    if(0 == nId) nId = syscall(SYS_gettid);
    RMF_OSAL_THREAD_INFO *pThreadInfo  = ((RMF_OSAL_THREAD_INFO*)(vl_gLocalThreadRegistrar.getElementAt(nId)));
    if(NULL != pThreadInfo) delete pThreadInfo;
    vl_gLocalThreadRegistrar.removeElementAt(nId);
}

void rmf_osal_ThreadGetName(unsigned long nId, char * pszThreadName, int nDestChars)
{VL_AUTO_LOCK(vlg_threadRegLock);

    if(0 == nId) nId = syscall(SYS_gettid);
    RMF_OSAL_THREAD_INFO *pThreadInfo  = ((RMF_OSAL_THREAD_INFO*)(vl_gLocalThreadRegistrar.getElementAt(nId)));
    //vlStrCpy(pszThreadName, "", nDestChars);
    strncpy(pszThreadName, "", nDestChars);
    if(NULL != pThreadInfo)
    {
        strncpy(pszThreadName, pThreadInfo->strThreadName.c_str(), nDestChars);
    }
}

const char * rmf_osal_ThreadGetNamePtrUnsafe(unsigned long nId)
{VL_AUTO_LOCK(vlg_threadRegLock);

    if(0 == nId) nId = syscall(SYS_gettid);
    RMF_OSAL_THREAD_INFO *pThreadInfo  = ((RMF_OSAL_THREAD_INFO*)(vl_gLocalThreadRegistrar.getElementAt(nId)));
    if(NULL != pThreadInfo)
    {
        return pThreadInfo->strThreadName.c_str();
    }
    return NULL;
}

int rmf_osal_RegWantLogForThisThread()
{
	if(0 != vlg_logForTidOnly)
    {
        if(syscall(SYS_gettid) == (int)vlg_logForTidOnly)
        {
            // want log for this thread
            return 1;
        }
        else
        {
            // don't want log for this thread
            return 0;
        }
    }
    
    // by default want log for all threads
    return 1;
}

int rmf_osal_ThreadResGetThreadInfo(unsigned long nCallerThreadId, unsigned long nForThreadId)
{VL_AUTO_LOCK(vlg_threadRegLock);

    char szThreadId[64];
    FILE * fp = NULL;
    snprintf(szThreadId, sizeof(szThreadId), "/tmp/vl-thread-info-%lu", nCallerThreadId);
    RMF_OSAL_THREAD_INFO *pThreadInfo  = ((RMF_OSAL_THREAD_INFO*)(vl_gLocalThreadRegistrar.getElementAt(nForThreadId)));
    if(NULL != pThreadInfo)
    {
        RMF_OSAL_THREAD_INFO threadInfo = *pThreadInfo;
        char szThreadName[256];
        unsigned int i = 0;
        //strncpy(szThreadName, threadInfo.strThreadName.c_str(), sizeof(szThreadName));
        strncpy(szThreadName, threadInfo.strThreadName.c_str(), sizeof(szThreadName));
        szThreadName[255]= '\0';
        for(i = 0; (i < sizeof(szThreadName)) && ('\0' != szThreadName[i]); i++)
        {
            if(isspace(szThreadName[i])) szThreadName[i] = '_';
        }
        fp = fopen(szThreadId, "wt");
        if(NULL != fp)
        {
            fprintf(fp, "%lu %s %lu %08x %lu\n", threadInfo.ulThreadId, szThreadName,
                                                 threadInfo.ulStackMax, (unsigned int)threadInfo.ulStackStart,
                                                 threadInfo.ulCreationTime);
            fclose(fp);
        }
    }
    else
    {
        fp = fopen(szThreadId, "wt");
        if(NULL != fp)
        {
            fprintf(fp, "%lu\n", nForThreadId);
            fclose(fp);
        }
    }
    
    return 1;
}

void rmf_osal_ThreadSetDescriptor(unsigned long nId, void * pDescriptor)
{VL_AUTO_LOCK(vlg_threadDescriptorLock);

    if(0 == nId) nId = pthread_self();
    //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS","%s: registering %lu, 0x%p\n", __FUNCTION__, nId, pDescriptor);
    vl_gMpeosThreadDescriptors.setElementAt((vlMap::KEY)nId, (vlMap::ELEMENT)pDescriptor);
}

void * rmf_osal_ThreadGetDescriptor(unsigned long nId)
{VL_AUTO_LOCK(vlg_threadDescriptorLock);

    if(0 == nId) nId = pthread_self();
    void * pThreadDescriptor  = ((void*)(vl_gMpeosThreadDescriptors.getElementAt(nId)));
    //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS","%s: retrieving %lu, 0x%p\n", __FUNCTION__, nId, pThreadDescriptor);
    return pThreadDescriptor;
}

void rmf_osal_ThreadRemoveDescriptor(unsigned long nId)
{VL_AUTO_LOCK(vlg_threadDescriptorLock);

    if(0 == nId) nId = pthread_self();
    vl_gMpeosThreadDescriptors.removeElementAt(nId);
    //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS","%s: removing %lu\n", __FUNCTION__, nId);
}

unsigned int rmf_osal_ReadProcInt(const char ** ppszPtr, const char * pszTag)
{
    if((NULL != ppszPtr) && (NULL != *ppszPtr))
    {
        const char * pszLoc = strstr(*ppszPtr, pszTag);
        if(NULL != pszLoc)
        {
            static char szTag[32]; int nValue = 0;
            sscanf(pszLoc, "%s %d", szTag, &nValue);
            *ppszPtr = pszLoc;
            return nValue;
        }
    }
    return 0;
}

unsigned int rmf_osal_GetProcInt(const char * pszTag)
{
    unsigned long nProcessPID = syscall(SYS_getpid);
    char szProcessStatus[64]="";
    char szBuf[256]="";
    int nValue = 0;
    snprintf(szProcessStatus, sizeof(szProcessStatus), "/proc/%lu/status", nProcessPID);
    FILE * fp = fopen(szProcessStatus, "r");
    if(NULL != fp)
    {
        char * pszRead = NULL;
        while(!feof(fp))
        {
            pszRead = fgets(szBuf, sizeof(szBuf), fp);
            if(NULL != pszRead)
            {
                const char * pszLoc = strstr(szBuf, pszTag);
                if(NULL != pszLoc)
                {
                    char szTag[32];
                    sscanf(pszLoc, "%s %d", szTag, &nValue);
                    break;
                }
            }
        }
        fclose(fp);
    }
    return nValue;
}

void rmf_osal_flushFDNametoFile(int fd, ofstream &file)
{
    char buf[256];    
    char path[256];
    
    int fd_flags = fcntl( fd, F_GETFD ); 
    if ( fd_flags == -1 ) return;

    int fl_flags = fcntl( fd, F_GETFL ); 
    if ( fl_flags == -1 ) return;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SYS","%s: Processing FD %d.\n", 
        __FUNCTION__, fd);
    snprintf(path, sizeof(path),"/proc/self/fd/%d", fd);

    memset( &buf[0], 0, 256 );
    ssize_t s = readlink( path, &buf[0], 256 );
    if ( s == -1 )
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SYS","%s: readlink() failed. Non-fatal error.\n", 
            __FUNCTION__);
        return;
    }
    file<<buf<<"\n";
    return;
}

void rmf_osal_dumpFDList(void)
{
    int numHandles = getdtablesize();
    int fd_flags = -1;
    ofstream myfile(RMF_OSAL_RMFSTREAMER_FDLIST_FILE);
    if(!myfile.is_open())
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SYS","%s: File %s couldn't be opened.\n",
            __FUNCTION__, RMF_OSAL_RMFSTREAMER_FDLIST_FILE);
        return;
    }
    for ( int i = 0; i < numHandles; i++ )
    {
        fd_flags = fcntl(i, F_GETFD); 
        if (-1 != fd_flags)
        {
            rmf_osal_flushFDNametoFile(i, myfile);
        }
    }
    myfile.close();
}

void* rmf_osal_ResStat(void * _pServiceContext)
{
    unsigned long nProcessPID = (unsigned long)(_pServiceContext);
    static char szBuf[1024]="";
    const char* pszPtr = NULL;
    const char** ppszPtr = &pszPtr;
    char szProcessStatus[64]="";
    static char szLoadAvg[256]="";
    size_t count, result;
    int present = 0;	
    bool print_sys_status	= false;
    struct stat file1;
    rmf_osal_ThreadRegSet(RMF_OSAL_RES_TYPE_THREAD, 0, "osal_ResStat", 1024*1024, 0);

    snprintf(szProcessStatus, sizeof(szProcessStatus), "/proc/%lu/status", nProcessPID);
    FILE * fp = fopen(szProcessStatus, "r");
    if(NULL == fp)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SYS","%s: fp is NULL\n", __FUNCTION__);
        return NULL;
    }
    FILE * fpLoadAvg = fopen("/proc/loadavg", "r");
    if(NULL == fpLoadAvg)
    {
        fclose(fp);
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SYS","%s: fpLoadAvg is NULL\n", __FUNCTION__);
        return NULL;
    }
    count = sizeof(szBuf);
    result = fread(szBuf, 1, count, fp);
    if (result == 0 )
    {
        fclose(fp);
        fclose(fpLoadAvg);
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SYS","%s: fread failed result = %d, count = %d\n", __FUNCTION__, result, count);
        return NULL;
    }
    
    pszPtr = szBuf;
    rmf_osal_ReadProcInt(ppszPtr, "FDSize:");
    unsigned int nOffset = (*ppszPtr - szBuf);
    //cSleep(60000);
    rmf_osal_threadSleep(60000,0);
	
    present = stat( RMF_OSAL_DUMP_FLAG, &file1 );
    if( 0 != present )
    { 
       FILE *fp = fopen(RMF_OSAL_DUMP_FLAG, "w+");
       if(NULL != fp)
       {
           fclose( fp );
           fp =NULL;
           print_sys_status = true;
       }
    }	
	
    while(1)
    {
        //cSleep(60000);
        rmf_osal_threadSleep(600000,0);
        if (0 != fseek(fp, nOffset, SEEK_SET))
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SYS","%s: fseek failed  - continue\n", __FUNCTION__);
            continue;
        }
        result = fread(szBuf, 1, count, fp);
        if (result == 0 )
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SYS","%s: fread failed  - continue\n", __FUNCTION__);
            continue;
        }
        pszPtr = szBuf;
        unsigned int nVmPeak  = rmf_osal_ReadProcInt(ppszPtr, "VmPeak:" );
        unsigned int nVmSize  = rmf_osal_ReadProcInt(ppszPtr, "VmSize:" );
        unsigned int nVmHWM   = rmf_osal_ReadProcInt(ppszPtr, "VmHWM:"  );
        unsigned int nVmRSS   = rmf_osal_ReadProcInt(ppszPtr, "VmRSS:"  );
        unsigned int nVmData  = rmf_osal_ReadProcInt(ppszPtr, "VmData:" );
        unsigned int nVmLib   = rmf_osal_ReadProcInt(ppszPtr, "VmLib:"  );
        unsigned int nVmPTE   = rmf_osal_ReadProcInt(ppszPtr, "VmPTE:"  );
        unsigned int nThreads = rmf_osal_ReadProcInt(ppszPtr, "Threads:");
        unsigned int nVmMapped = nVmSize - (nVmData + nVmLib);
        unsigned int nUpTime   = rmf_osal_GetUptime();
        fseek(fpLoadAvg, 0, SEEK_SET);
        fgets(szLoadAvg, sizeof(szLoadAvg), fpLoadAvg);
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS","=================================RMF_OSAL Process id : %lu stats==================================\n", nProcessPID);
#if 0
		{
			int ret = system("date >> /opt/logs/top_osal.txt; top -n 1 -b >> /opt/logs/top_osal.txt");
        	RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SYS","output to /opt/logs/top_osal.txt, system cmd returned =%d \n", ret);
			//rmf_osal_threadSleep(1000,0);
		}
#endif // if 0
        if( print_sys_status )
        {
	        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS","%s: %16s = %02u:%02u:%02u, %s = %u KB, %11s = %s", __FUNCTION__, "Up Time", (nUpTime/3600), ((nUpTime%3600)/60), (nUpTime%60), "Free Mem", rmf_osal_GetFreePhysMem(), "Load Ave.", szLoadAvg);
        }		
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS","%s: %16s = %8u, %11s = %8u, %11s = %8d\n", __FUNCTION__, "Thread Count", nThreads, "Mutex Count", vlg_nMutexCount, "Deadlocks", vlg_nMutexWaitCount);
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS","%s: %8s = %5u MB, %8s = %5u MB, %8s = %5u MB, %8s = %5u MB\n", __FUNCTION__, "Heap", nVmData/1024, "Mapped", nVmMapped/1024, "Total", nVmSize/1024, "Peak Vir", nVmPeak/1024);
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS","%s: %8s = %5u MB, %8s = %5u MB, %8s = %5u KB\n", __FUNCTION__, "Physical", nVmRSS/1024, "Peak Phy", nVmHWM/1024, "Pg Tb En", nVmPTE);
        /* Print system FD stats*/
        if( print_sys_status )		
        {
            string line;
            ifstream myfile(RMF_OSAL_SYSTEM_FDINFO_FILE);
            if(myfile.is_open())
            {
                if(getline(myfile,line))
                {
                	    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS","System-wide FD stats: (total allocated) (total freed) (max allowed) %s.\n",
                		    line.c_str());
                }
                 myfile.close();
            }
        }
        /* Dump process FD list to file */
        //rmf_osal_dumpFDList();
		
#ifdef INTEL_CANMORE
#if 0 //SS Commented
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS","%s: osal_memmap: Mapped=%lld, Unmapped=%lld, diff=%lld\n", __FUNCTION__, vlg_vlOsMapIoToMemCount, vlg_vlOsUnMapIoToMemCount, vlg_vlOsMapIoToMemCount - vlg_vlOsUnMapIoToMemCount);
#endif
#endif
//        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS","====================================================================================================\n");

#if defined(RMF_OSAL_ENABLE_MUTEX_DB_TYPE_LIST)
        list<RMF_OSAL_MUTEX_INFO*>::iterator it;
        static int nPrevMutexCount = vlg_nMutexCount;

        pthread_mutex_lock(&vlg_mutexRegLock);
        for ( it=vl_gMpeosMutexRegistrar.begin() ; (it != vl_gMpeosMutexRegistrar.end()) && (nPrevMutexCount < vlg_nMutexCount); it++, nPrevMutexCount++ )
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS","%s: %5d) MutexType = %10s, Line = %5d, Func = %s\n", __FUNCTION__, nPrevMutexCount, (*it)->pszMutexName, (*it)->nLineNo, (*it)->pszFunction);
        }
        pthread_mutex_unlock(&vlg_mutexRegLock);
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS","====================================================================================================\n");
#endif // defined(RMF_OSAL_ENABLE_MUTEX_DB_TYPE_LIST)
    }
    return NULL;
}


void* vlThreadRegCleanup(void * _pServiceContext)
{
    int i = 0;
    unsigned long nProcessPID = (unsigned long)(_pServiceContext);
    struct stat tStat;
    //VL_ZERO_STRUCT(tStat);
    memset(&tStat, 0x0, sizeof(struct stat));
    rmf_osal_ThreadRegSet(RMF_OSAL_RES_TYPE_THREAD, 0, "vlThreadRegCleanup", 1024*1024, 0);
    while(1)
    {
        int nSize = vl_gLocalThreadRegistrar.size();
        vlg_nThreadCount = nSize;
        //cSleep(100);
        rmf_osal_threadSleep(100,0);
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SYS","%s: Total thread count = %d\n", __FUNCTION__, nSize);
        for(i = 0; i < nSize; i++)
        {
            //cSleep(100);
            rmf_osal_threadSleep(100,0);
            
            {VL_AUTO_LOCK(vlg_threadRegLock);
            
                unsigned long nTaskId = 0;
                RMF_OSAL_THREAD_INFO *pThreadInfo  = ((RMF_OSAL_THREAD_INFO*)(vl_gLocalThreadRegistrar.getElementAtIntegerIndex(i, nTaskId)));
                if(NULL != pThreadInfo)
                {
                    char szThread[128];
                    snprintf(szThread, sizeof(szThread), "/proc/%lu/task/%lu", nProcessPID, nTaskId);
                    if(0 != stat(szThread, &tStat))
                    {
                        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SYS","%s: Deleting info for thread '%s' (%lu)\n", __FUNCTION__, pThreadInfo->strThreadName.c_str(), nTaskId);
                        rmf_osal_ThreadRegRemove(RMF_OSAL_RES_TYPE_THREAD, nTaskId);
                        i--;
                    }
                }
            }
        }
    }
    
    return NULL;
}

int rmf_osal_SetLogLvl(const char *mod, rdk_LogLevel lvl)
{
    int mod_num = rdk_logger_envGetNum(mod);
    if(mod_num >= 0)
    {
        rdk_g_logControlTbl[(mod_num)] |= (1 << (lvl));
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS","%s: Set log level for module %s for %d, config is now 0x%08x\n", __FUNCTION__, mod, lvl, rdk_g_logControlTbl[(mod_num)]);
        return 1;
    }
    else
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS","%s: Out of bounds for module %s\n", __FUNCTION__, mod);
    }
    return 0;
}

int rmf_osal_ResetLogLvl(const char *mod, rdk_LogLevel lvl)
{
    int mod_num = rdk_logger_envGetNum(mod);
    if(mod_num >= 0)
    {
        rdk_g_logControlTbl[(mod_num)] &= ~(1 << (lvl));
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS","%s: Reset log level for module %s for %d, config is now 0x%08x\n", __FUNCTION__, mod, lvl, rdk_g_logControlTbl[(mod_num)]);
        return 1;
    }
    else
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS","%s: Out of bounds for module %s\n", __FUNCTION__, mod);
    }
    return 0;
}

int rmf_osal_SetLogFields(const char *mod, uint32_t bitfields)
{
    int mod_num = rdk_logger_envGetNum(mod);
    if(mod_num >= 0) 
    {
        rdk_g_logControlTbl[(mod_num)] = bitfields;
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS","%s: Set log fields for module %s to 0x%08x\n", __FUNCTION__, mod, rdk_g_logControlTbl[(mod_num)]);
        return 1;
    }
    else
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS","%s: Out of bounds for for module %s\n", __FUNCTION__, mod);
    }
    return 0;
}
static unsigned long selectiveCode;

unsigned long rmf_osal__getSelectiveCodeMask()
{
  return selectiveCode;
}
int rmf_osal_EnableSelectiveCode(unsigned long val)
{
	selectiveCode = val;
	return 1;
}

int rmf_osal_EnableLogForTidOnly(unsigned long ulTid)
{
    vlg_logForTidOnly = ulTid;
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS","%s: Enabled logging only for Task %lu\n", __FUNCTION__, vlg_logForTidOnly);
    return 1;
}

static RMF_OSAL_SCRIPT_EXEC_SYMBOL_ENTRY vlg_aThreadResServerSymbols[] =
{
    RMF_OSAL_SCRIPT_EXEC_MAKE_FUNC_SYMBOL_ENTRY(rmf_osal_ThreadResGetThreadInfo, 2),
    RMF_OSAL_SCRIPT_EXEC_MAKE_FUNC_SYMBOL_ENTRY(rmf_osal_SetLogLvl, 2),
    RMF_OSAL_SCRIPT_EXEC_MAKE_FUNC_SYMBOL_ENTRY(rmf_osal_ResetLogLvl, 2),
    RMF_OSAL_SCRIPT_EXEC_MAKE_FUNC_SYMBOL_ENTRY(rmf_osal_SetLogFields, 2),
    RMF_OSAL_SCRIPT_EXEC_MAKE_FUNC_SYMBOL_ENTRY(rmf_osal_EnableLogForTidOnly, 1),
    RMF_OSAL_SCRIPT_EXEC_MAKE_FUNC_SYMBOL_ENTRY(rdk_dbg_priv_SetLogLevelString, 2),
    {RMF_OSAL_SCRIPT_EXEC_SYM_TYPE_FUNC, "vlDsgDumpDsgStats", NULL, 0     },
    {RMF_OSAL_SCRIPT_EXEC_SYM_TYPE_FUNC, "vlMpeosCdlUpgradeToImage", NULL, 4     },
    {RMF_OSAL_SCRIPT_EXEC_SYM_TYPE_FUNC, "vlDsgCheckAndRecoverConnectivity", NULL, 0     },
#if 0 //SS Commented
	RMF_OSAL_SCRIPT_EXEC_MAKE_FUNC_SYMBOL_ENTRY(vlStartRemoteKeySequencer, 0),
	RMF_OSAL_SCRIPT_EXEC_MAKE_FUNC_SYMBOL_ENTRY(vlStopRemoteKeySequencer, 0),
    RMF_OSAL_SCRIPT_EXEC_MAKE_FUNC_SYMBOL_ENTRY(mpeos_TestSetDeInterlacePolicy,1),
    RMF_OSAL_SCRIPT_EXEC_MAKE_FUNC_SYMBOL_ENTRY(rmf_dbgNotifyLogRotation,0),
	RMF_OSAL_SCRIPT_EXEC_MAKE_FUNC_SYMBOL_ENTRY(dbgApiCallerInterface,0),
#endif
	RMF_OSAL_SCRIPT_EXEC_MAKE_FUNC_SYMBOL_ENTRY(rmf_osal_EnableSelectiveCode,1)
};

static void create_thread(char *pszName, THREADFP threadFunc, unsigned long arg)
{
    (void)pszName;

    pthread_attr_t td_attr;                        /* Thread attributes. */
    pthread_t td_id;

    pthread_attr_init(&td_attr);
    pthread_attr_setdetachstate(&td_attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setschedpolicy(&td_attr, SCHED_OTHER);

    pthread_create(&td_id, &td_attr, threadFunc, (void*)arg);
}

RMF_OSAL_SCRIPT_EXEC_INST_SYMBOL_TABLE(vlg_ThreadResServerSymbolTable, vlg_aThreadResServerSymbols);

#define RMF_REBOOT_REASON_LOGFILE   "/opt/logs/rebootInfo.log"

void rmf_truncate_file(const char * pszFile)
{
    FILE * fp = fopen(pszFile, "w"); // w/o
    if(NULL != fp)
    {
        fclose(fp);
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SYS","%s: Could not open '%s' for writing.\n", __FUNCTION__, RMF_REBOOT_REASON_LOGFILE);
    }
}

int rmf_osal_StartThreadInfoServer()
{
    pthread_mutex_init(&vlg_mutexRegLock, NULL);
    pthread_mutex_init(&vlg_mutexMonLock, NULL);
    vlg_bMutexDbInitialized = 1;
	selectiveCode=0x00;
    //cThreadCreateSimple("vlThreadRegCleanup", (THREADFP)vlThreadRegCleanup, (unsigned long)syscall(SYS_getpid));
    //cThreadCreateSimple("rmf_osal_ResStat", (THREADFP)rmf_osal_ResStat, (unsigned long)syscall(SYS_getpid));

    create_thread("vlThreadRegCleanup", (THREADFP)vlThreadRegCleanup, (unsigned long)syscall(SYS_getpid));
    create_thread("rmf_osal_ResStat", (THREADFP)rmf_osal_ResStat, (unsigned long)syscall(SYS_getpid));
    char * pszEnvThreadInfoServerCallPort = getenv("RMF_OSAL_THREAD_INFO_CALL_PORT");
    int nCallPort = 0;
    if(NULL != pszEnvThreadInfoServerCallPort)
    {
        nCallPort = atoi(pszEnvThreadInfoServerCallPort);
    }
    //if(0 == nCallPort) nCallPort = 54128;
    if(0 < nCallPort)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS","%s: Using '%d' as call port.\n", __FUNCTION__, nCallPort);
        rmf_osal_CreateUdpScriptServer(nCallPort, &vlg_ThreadResServerSymbolTable);
    }
    //rmf_truncate_file(RMF_REBOOT_REASON_LOGFILE);
    return 1;
}

void rmf_osal_RegDsgCheckAndRecoverConnectivity(void (DsgCheckAndRecoverConnectivityfptr)())
{
    uint32_t count = vlg_ThreadResServerSymbolTable.nSymbols;
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS","%s: Enter.\n", __FUNCTION__);
    RMF_OSAL_SCRIPT_EXEC_SYMBOL_ENTRY *pEntry = vlg_ThreadResServerSymbolTable.paSymbols;
    for(;count--;)
    {
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS","%s: pEntry[count].pszName: %s\n", __FUNCTION__, pEntry[count].pszName);
        if (0 == strcmp( pEntry[count].pszName, "vlDsgCheckAndRecoverConnectivity"))
        {
            pEntry[count].pVar = (unsigned long *)DsgCheckAndRecoverConnectivityfptr;
        }
    }
}

void rmf_osal_RegDsgDump(void (DumpDsgfptr)())
{
	uint32_t count = vlg_ThreadResServerSymbolTable.nSymbols;
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS","%s: Enter.\n", __FUNCTION__);
	RMF_OSAL_SCRIPT_EXEC_SYMBOL_ENTRY *pEntry = vlg_ThreadResServerSymbolTable.paSymbols;
	for(;count--;)
	{
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS","%s: pEntry[count].pszName: %s\n", __FUNCTION__, pEntry[count].pszName);
		if (0 == strcmp( pEntry[count].pszName, "vlDsgDumpDsgStats"))
		{
			pEntry[count].pVar = (unsigned long *)DumpDsgfptr;
		}
	}
}
void rmf_osal_RegCdlUpgrade(void (CdlUpgradefptr)(int eImageType, int eImageSign, int bRebootNow, const char * pszImagePathName))
{
	uint32_t count = vlg_ThreadResServerSymbolTable.nSymbols;
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS","%s: Enter.\n", __FUNCTION__);
	RMF_OSAL_SCRIPT_EXEC_SYMBOL_ENTRY *pEntry = vlg_ThreadResServerSymbolTable.paSymbols;
	for(;count--;)
	{
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS","%s: pEntry[count].pszName: %s\n", __FUNCTION__, pEntry[count].pszName);
		if (0 == strcmp( pEntry[count].pszName, "vlMpeosCdlUpgradeToImage"))
		{
                       RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS","%s: Assigning CdlUpgradefptr\n", __FUNCTION__);
			pEntry[count].pVar = (unsigned long *)CdlUpgradefptr;
		}
	}
}

//Changing event name from "reboot" to "osal_resourceregistrar_imageUpgrade_reboot"
#define VL_REBOOT_EVENT         "osal_resourceregistrar_imageUpgrade_reboot"

extern "C" void rmf_osal_GetLogFileName(const char * pszLocation, const char * pszEventName, char * pszFileName, int nCharCapacity)
{
    snprintf(pszFileName, nCharCapacity, "%s/mpeos_%s_log.txt", pszLocation, pszEventName);
}

extern "C" void rmf_osal_LogEvent(const char * pszEventName, const char * pszInitiator, const char * pszDoc)
{
    char szFileName[MAX_PATH_SIZE];
    rmf_osal_GetLogFileName(RMF_OSAL_EVENT_LOG_LOCATION, pszEventName, szFileName, sizeof(szFileName));
    FILE * fp = fopen(szFileName, "r"); // r/o
    int i = 0, nEntries = 0;
    char aszEvents[RMF_OSAL_MAX_EVENT_LOG_COUNT][MAX_PATH_SIZE]; // around 2.5 KB on the stack. should be ok.
    
    aszEvents[RMF_OSAL_MAX_EVENT_LOG_COUNT-1][MAX_PATH_SIZE-1] = '\0'; // safeguard
    
    if(NULL != fp)
    {
        // read existing events
        for(i = 0; i < RMF_OSAL_MAX_EVENT_LOG_COUNT; i++)
        {
            if(NULL == fgets(aszEvents[i], sizeof(aszEvents[i]), fp)) break;
            aszEvents[i][MAX_PATH_SIZE-1] = '\0'; // safeguard
            nEntries++;
        }
        fclose(fp);
    }

    // re-open for writing
    fp = fopen(szFileName, "w"); // w/o
    if(NULL != fp)
    {
        time_t t = 0;
        struct tm gm, lt;
        char szEventLog[MAX_PATH_SIZE];
        unsigned int nUpTime  = rmf_osal_GetUptime();
        
        // create current event
        time(&t);
        gmtime_r(&t, &gm);
        localtime_r(&t, &lt);
        snprintf(szEventLog, sizeof(szEventLog),
        "UTC:%02u%02u%02u-%02u:%02u:%02u, LT:%02u%02u%02u-%02u:%02u:%02u, UP:%02u:%02u:%02u: %s: %s: %s\n",
        (gm.tm_year%100), gm.tm_mon+1, gm.tm_mday, gm.tm_hour, gm.tm_min, gm.tm_sec,
        (lt.tm_year%100), lt.tm_mon+1, lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec,
        (nUpTime/3600), ((nUpTime%3600)/60), (nUpTime%60),
        pszEventName, pszInitiator, pszDoc);
        szEventLog[MAX_PATH_SIZE-1] = '\0'; // safeguard
        
        // write the latest event
        fprintf(fp, "%s", szEventLog);
        // write out all existing events except the oldest
        for(i = 0; i < ((nEntries < RMF_OSAL_MAX_EVENT_LOG_COUNT)?nEntries:RMF_OSAL_MAX_EVENT_LOG_COUNT-1); i++)
        {
            fprintf(fp, "%s", aszEvents[i]);
        }
        // flush, sync and close file.
        fflush(fp);
        fsync(fileno(fp));
        fclose(fp);
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SYS","vlLogEvent: Could not open '%s' for writing.\n", szFileName);
    }
}

extern "C" void rmf_osal_LogRebootInfo(const char * pszInitiator, const char * pszDoc)
{
    const char * pszFileName = RMF_REBOOT_REASON_LOGFILE;
    // re-open for writing
    FILE * fp = fopen(pszFileName, "w+"); // r/w+a
    if(NULL != fp)
    {
        char szEventLog[MAX_PATH_SIZE];
        
        time_t t = 0;
        struct tm gm, lt;
        unsigned int nUpTime  = rmf_osal_GetUptime();
        
        // create current event
        time(&t);
        gmtime_r(&t, &gm);
        localtime_r(&t, &lt);
        
        snprintf(szEventLog, sizeof(szEventLog),
        "%02u.%02u.20%02u_%02u:%02u.%02u RebootReason: Triggered from %s Rebooting the box due to %s..! (time='UTC:%02u%02u%02u-%02u:%02u:%02u, LT:%02u%02u%02u-%02u:%02u:%02u, UP:%02u:%02u:%02u')\n", 
                    lt.tm_mday, lt.tm_mon+1, (lt.tm_year%100), lt.tm_hour, lt.tm_min, lt.tm_sec, 
                    pszInitiator, pszDoc,
                    (gm.tm_year%100), gm.tm_mon+1, gm.tm_mday, gm.tm_hour, gm.tm_min, gm.tm_sec,
                    (lt.tm_year%100), lt.tm_mon+1, lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec,
                    (nUpTime/3600), ((nUpTime%3600)/60), (nUpTime%60));
        szEventLog[MAX_PATH_SIZE-1] = '\0'; // safeguard
        
        // write the latest event
        fprintf(fp, "%s", szEventLog);
        // flush, sync and close file.
        fflush(fp);
        fsync(fileno(fp));
        fclose(fp);
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SYS","rmf_osal_LogRebootEvent: Could not open '%s' for writing.\n", pszFileName);
    }
}

extern "C" void rmf_osal_DumpEvents(const char * pszEventName)
{
    char szFileName[MAX_PATH_SIZE];
    rmf_osal_GetLogFileName(RMF_OSAL_EVENT_LOG_LOCATION, pszEventName, szFileName, sizeof(szFileName));
    FILE * fp = fopen(szFileName, "r"); // r/o
    if(NULL != fp)
    {
        int i = 0;
        char szEventLog[MAX_PATH_SIZE];
        
        for(i = 0; i < RMF_OSAL_MAX_EVENT_LOG_COUNT; i++)
        {
            if(NULL == fgets(szEventLog, sizeof(szEventLog), fp)) break;
            szEventLog[MAX_PATH_SIZE-1] = '\0'; // safeguard
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS", " %02i) %s", i+1, szEventLog);
        }
        fclose(fp);
    }
}

extern "C" void rmf_osal_LogRebootEvent(const char * pszInitiator, const char * pszDoc)
{
    int reboot_reason = 3;
    if(NULL != pszDoc)
    {
        if(0 == strcmp("VL_CDL_MANAGER_EVENT_REBOOT", pszDoc))
        {
            reboot_reason = 8;
        }
        else if(0 == strcmp("SNMP Host Reset", pszDoc))
        {
            reboot_reason = -1; // Reboot reason already recorded elsewhere.
        }
        else if(0 == strcmp("recovery from mspod hardware errors", pszDoc))
        {
            reboot_reason = 10;
        }
    }
    if(reboot_reason > 0)
    {
        FILE * fp = fopen("/opt/etc/reboot_log", "w"); // r/w
        if(NULL != fp)
        {
            fprintf(fp, "reboot_reason=%d\n", reboot_reason);
            fflush(fp);
            fsync(fileno(fp));
            fclose(fp);
        }
    }
    rmf_osal_LogEvent(VL_REBOOT_EVENT, pszInitiator, pszDoc);
    rmf_osal_LogRebootInfo(pszInitiator, pszDoc);
}

extern "C" void rmf_osal_DumpRebootEvents()
{
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS", " #######################################################################################\n");
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS", " ## Log of previous Reboot events follows...                          ##\n");
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS", " #######################################################################################\n");
    rmf_osal_DumpEvents(VL_REBOOT_EVENT);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS", " #######################################################################################\n");
}


