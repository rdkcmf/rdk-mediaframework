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


#ifndef __RMF_OSAL_RES_REG_H__
#define __RMF_OSAL_RES_REG_H__

typedef enum _RMF_OSAL_RES_TYPE
{
    RMF_OSAL_RES_TYPE_LMQ_MSG_Q,
    RMF_OSAL_RES_TYPE_VLC_MSG_Q,
    RMF_OSAL_RES_TYPE_PFC_MSG_Q,
    RMF_OSAL_RES_TYPE_THREAD,
    RMF_OSAL_RES_TYPE_MEMORY,

}RMF_OSAL_RES_TYPE;

typedef struct _RMF_OSAL_RES_INFO
{
    RMF_OSAL_RES_TYPE eResType;
    unsigned long nId;
    char szName[256];
    int nVal;
    int nPeak;
    int nMax;

}RMF_OSAL_RES_INFO;

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

void rmf_osal_ResRegSet(RMF_OSAL_RES_TYPE eResType, unsigned long nId, const char * pszName, int nMax, int nVal);
int rmf_osal_ResRegGet(int nTimeout, RMF_OSAL_RES_INFO * pResInfo);
void rmf_osal_ResRegRemove(RMF_OSAL_RES_TYPE eResType, unsigned long nId);
void rmf_osal_ResRegLookup(unsigned long nId, RMF_OSAL_RES_INFO * pResInfoOut);

void rmf_osal_ThreadRegSet(RMF_OSAL_RES_TYPE eResType, unsigned long nId, const char * pszName, int nStackMax, int nVal);
void rmf_osal_ThreadRegRemove(RMF_OSAL_RES_TYPE eResType, unsigned long nId);
void rmf_osal_ThreadGetName(unsigned long nId, char * pszThreadName, int nDestChars);
const char * rmf_osal_ThreadGetNamePtrUnsafe(unsigned long nId);
int rmf_osal_RegWantLogForThisThread();

void rmf_osal_ThreadSetDescriptor(unsigned long nId, void * pDescriptor);
void * rmf_osal_ThreadGetDescriptor(unsigned long nId);
void rmf_osal_ThreadRemoveDescriptor(unsigned long nId);

void rmf_osal_MutexRegCreated(unsigned long nMutexId, const char * pszMutexName, const char * pszFunction, unsigned long nLineNo);
void rmf_osal_MutexRegWaitStartImpl(unsigned long nMutexId);
void rmf_osal_MutexRegWaitStopImpl(unsigned long nMutexId);
void rmf_osal_MutexRegAcquiredImpl(unsigned long nMutexId, const char * pszFunction, unsigned long nLineNo);
void rmf_osal_MutexRegReleasedImpl(unsigned long nMutexId);
void rmf_osal_MutexRegDestroyed(unsigned long nMutexId);
void rmf_osal_RegDsgDump(void (DumpDsgfptr)());
void rmf_osal_RegDsgCheckAndRecoverConnectivity(void (DsgCheckAndRecoverConnectivityfptr)());
void rmf_osal_RegCdlUpgrade(void (CdlUpgradefptr)(int eImageType, int eImageSign, int bRebootNow, const char * pszImagePathName));
void rmf_osal_GetLogFileName(const char * pszLocation, const char * pszEventName, char * pszFileName, int nCharCapacity);
void rmf_osal_LogEvent(const char * pszEventName, const char * pszInitiator, const char * pszDoc);
void rmf_osal_DumpEvents(const char * pszEventName);
void rmf_osal_LogRebootEvent(const char * pszInitiator, const char * pszDoc);
void rmf_osal_DumpRebootEvents();
unsigned long rmf_osal_GetUptime();

#define RMF_OSAL_MUTEX_LOCK_UNLOCK_MONITORING
#ifdef RMF_OSAL_MUTEX_LOCK_UNLOCK_MONITORING
#define rmf_osal_MutexRegWaitStart(x)           rmf_osal_MutexRegWaitStartImpl(x)
#define rmf_osal_MutexRegWaitStop(x)           rmf_osal_MutexRegWaitStopImpl(x)
#define rmf_osal_MutexRegAcquired(x, y, z)
#define rmf_osal_MutexRegReleased(x)
#else
#define rmf_osal_MutexRegWaitStart(x)
#define rmf_osal_MutexRegWaitStop(x)
#define rmf_osal_MutexRegAcquired(x, y, z)
#define rmf_osal_MutexRegReleased(x)
#endif

unsigned int rmf_osal_GetProcInt(const char * pszTag);

#define RMF_OSAL_GET_PROC_VM_PEAK()       rmf_osal_GetProcInt("VmPeak:")
#define RMF_OSAL_GET_PROC_VM_SIZE()       rmf_osal_GetProcInt("VmSize:")
#define RMF_OSAL_GET_PROC_VM_HWM()        rmf_osal_GetProcInt("VmHWM:")
#define RMF_OSAL_GET_PROC_VM_RSS()        rmf_osal_GetProcInt("VmRSS:")
#define RMF_OSAL_GET_PROC_VM_DATA()       rmf_osal_GetProcInt("VmData:")
#define RMF_OSAL_GET_PROC_VM_LIB()        rmf_osal_GetProcInt("VmLib:")
#define RMF_OSAL_GET_PROC_VM_PTE()        rmf_osal_GetProcInt("VmPTE:")
#define RMF_OSAL_GET_PROC_VM_THREADS()    rmf_osal_GetProcInt("Threads:")

#define MAX_PATH_SIZE 255
#define RMF_OSAL_EVENT_LOG_LOCATION   "/opt"
#define RMF_OSAL_INIT_LOG   "hrv_init"
#define RMF_OSAL_MAX_EVENT_LOG_COUNT  10
#define RMF_OSAL_SYSTEM_FDINFO_FILE "/proc/sys/fs/file-nr"
#define RMF_OSAL_RMFSTREAMER_FDLIST_FILE "/opt/logs/rstreamer_fdlist.txt"


#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __RMF_OSAL_RES_REG_H__
