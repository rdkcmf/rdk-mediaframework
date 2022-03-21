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



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <vector>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <math.h>
#include <signal.h>


#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define RMF_OSAL_THREAD_INFO_CALL_PORT    54128
#define RMF_OSAL_THREAD_INFO_MAX_CHARS    256

pthread_t pthread_server;

typedef int SOCKET;

using namespace std;

static const char * g_pszThisAppName = NULL;
static const char * g_pszProcessName = NULL;
double g_nTicksPerMS = 0;
unsigned long g_ulUpdateInterval = 0;

static unsigned long g_nThisProcessId = 0;

#define VLABS(n)    (((n) < 0) ? -(n) : (n))

#define RMF_OSAL_THREAD_INFO_RETRIEVAL_BACKOFF_INTERVAL   10000000.0   // 10 seconds

#define RMF_OSAL_THREAD_MAX_SZSTATE_SIZE 4

typedef struct _RMF_OSAL_PROC_THREAD_INFO
{
    unsigned int nId;
    char szName[RMF_OSAL_THREAD_INFO_MAX_CHARS];
    int  nMaxStack;
    char szState[RMF_OSAL_THREAD_MAX_SZSTATE_SIZE];
    unsigned int nUserCpuUsage;
    unsigned int nKernelCpuUsage;
    unsigned int nUserCpuUsagePrev;
    unsigned int nKernelCpuUsagePrev;
    double dUserCpuUsagePercent;
    double dKernelCpuUsagePercent;
    unsigned int nPriority;
    unsigned int nNice;
    unsigned int nMinorFaults;
    unsigned int nMajorFaults;
    unsigned int nMinorFaultsPrev;
    unsigned int nMajorFaultsPrev;
    unsigned int nStackStart;
    unsigned int nStackEnd;
    unsigned int nCreationTime;
    double dTimeLastUpdated;
    double dTimeLastThreadInfoRetrieveAttempted;
    int   bRefreshed;
    
}RMF_OSAL_PROC_THREAD_INFO;

static vector<RMF_OSAL_PROC_THREAD_INFO> g_aCounters;

#define RMF_OSAL_MAX_PROCESS_NAME_SIZE        RMF_OSAL_THREAD_INFO_MAX_CHARS
#define RMF_OSAL_MAX_PROCESS_FIELD_COUNT      32

static char g_paszFields[RMF_OSAL_MAX_PROCESS_FIELD_COUNT][RMF_OSAL_MAX_PROCESS_NAME_SIZE];

typedef enum _RMF_OSAL_PROC_THREAD_STAT_FIELD
{
    RMF_OSAL_PROC_THREAD_STAT_FIELD_PID               = 0,
    RMF_OSAL_PROC_THREAD_STAT_FIELD_THREAD_NAME       = 1,
    RMF_OSAL_PROC_THREAD_STAT_FIELD_STATE             = 2,
    RMF_OSAL_PROC_THREAD_STAT_FIELD_STACK_START       = 3,
    RMF_OSAL_PROC_THREAD_STAT_FIELD_CREATION_TIME     = 4,
    RMF_OSAL_PROC_THREAD_STAT_FIELD_PS_NAME           = 4,
    RMF_OSAL_PROC_THREAD_STAT_FIELD_MINOR_FAULTS      = 9,
    RMF_OSAL_PROC_THREAD_STAT_FIELD_MAJOR_FAULTS      = 11,
    RMF_OSAL_PROC_THREAD_STAT_FIELD_USER              = 13,
    RMF_OSAL_PROC_THREAD_STAT_FIELD_KERNEL            = 14,
    RMF_OSAL_PROC_THREAD_STAT_FIELD_PRIORITY          = 17,
    RMF_OSAL_PROC_THREAD_STAT_FIELD_NICE              = 18,
    RMF_OSAL_PROC_THREAD_STAT_FIELD_STACK_END         = 28,
    RMF_OSAL_PROC_THREAD_STAT_FIELD_MAX_STACK         = 28,
    
}RMF_OSAL_PROC_THREAD_STAT_FIELD;

typedef enum _RMF_OSAL_PS_EF_FIELD
{
#ifdef USE_BRCM_SOC
    RMF_OSAL_PS_EF_FIELD_PID                   = 1,
    RMF_OSAL_PS_EF_FIELD_PS_NAME           = 7,
#else
    RMF_OSAL_PS_EF_FIELD_PID                   = 0,
    RMF_OSAL_PS_EF_FIELD_PS_NAME           = 4,
#endif
}RMF_OSAL_PS_EF_FIELD;

#define RMF_OSAL_FIELD_INDEX(i)   (1<<(i))

#define RMF_OSAL_PROCESS_SEARCH_FILE_NAME     "/tmp/vlthreadanalyzer.process.name"
#define RMF_OSAL_THREAD_INFO_FILE_NAME        "/tmp/vl-thread-info-"
 
int g_bContinue = 1;

unsigned long rmf_osal_GetSystemUptime()
{
    FILE *fp = fopen("/proc/uptime", "r");
    unsigned long secs   = 0;
    if (NULL != fp)
    {
        if ( 0 == fscanf(fp, "%ld", &secs))
        {
            printf("%s: fscanf failed\n", __FUNCTION__);
            secs = 0;
        }
        fclose(fp);
    }
    return secs;
}

int rmf_osal_GetFileFields(const char * pszFile)
{
    memset(g_paszFields, 0, sizeof(g_paszFields));
    
    FILE * pFile= fopen(pszFile, "rt" );
    int nFields = 0;
    if(NULL != pFile)
    {
        nFields =
            fscanf(pFile, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
                g_paszFields[ 0], g_paszFields[ 1], g_paszFields[ 2], g_paszFields[ 3], 
                g_paszFields[ 4], g_paszFields[ 5], g_paszFields[ 6], g_paszFields[ 7], 
                g_paszFields[ 8], g_paszFields[ 9], g_paszFields[10], g_paszFields[11], 
                g_paszFields[12], g_paszFields[13], g_paszFields[14], g_paszFields[15], 
                g_paszFields[16], g_paszFields[17], g_paszFields[18], g_paszFields[19], 
                g_paszFields[20], g_paszFields[21], g_paszFields[22], g_paszFields[23], 
                g_paszFields[24], g_paszFields[25], g_paszFields[26], g_paszFields[27], 
                g_paszFields[28], g_paszFields[29], g_paszFields[30], g_paszFields[31]);
            
        fclose(pFile);
    }
    
    return nFields;
}


#if 0
int rmf_osal_GetProcessPid(const char * pszProcessName)
{
    char szBuffer[RMF_OSAL_MAX_PROCESS_NAME_SIZE];
    unsigned int nProcessId = 0;

#ifdef USE_BRCM_SOC
    snprintf(szBuffer, sizeof(szBuffer), "ps -ef | grep \"%s\" > %s", pszProcessName, RMF_OSAL_PROCESS_SEARCH_FILE_NAME);
#else
    snprintf(szBuffer, sizeof(szBuffer), "ps | grep \"%s\" > %s", pszProcessName, RMF_OSAL_PROCESS_SEARCH_FILE_NAME);
#endif

    printf("%s: Waiting for process '%s'...\n", g_pszThisAppName, pszProcessName);

    do
    {
        int i = 0;

        system(szBuffer);

        i = rmf_osal_GetFileFields(RMF_OSAL_PROCESS_SEARCH_FILE_NAME);

        if((i > 0) && (0 == strcmp(g_paszFields[RMF_OSAL_PS_EF_FIELD_PS_NAME], pszProcessName)) ||
           (0 == strcmp(g_paszFields[RMF_OSAL_PS_EF_FIELD_PS_NAME+1], pszProcessName))
          )
        {
            nProcessId = strtoul(g_paszFields[RMF_OSAL_PS_EF_FIELD_PID], NULL, 10);
            printf("%s: Found process '%s', PID = %d\n", g_pszThisAppName, pszProcessName, nProcessId);
            printf("%s: Retrieving thread information for process '%s', please wait...\n", g_pszThisAppName, pszProcessName);
            remove(RMF_OSAL_PROCESS_SEARCH_FILE_NAME);
            break;
        }

        sleep(1);
    } while (0 == nProcessId);

    return nProcessId;

#if 0
    char szBuffer[RMF_OSAL_MAX_PROCESS_NAME_SIZE];
    unsigned int nProcessId = 0;

#ifdef USE_BRCM_SOC
    snprintf(szBuffer, sizeof(szBuffer), "ps -ef | grep \"%s\" > %s", pszProcessName, RMF_OSAL_PROCESS_SEARCH_FILE_NAME);
#else
    snprintf(szBuffer, sizeof(szBuffer), "ps | grep \"%s\" > %s", pszProcessName, RMF_OSAL_PROCESS_SEARCH_FILE_NAME);
#endif
    
    printf("%s: Waiting for process '%s'...\n", g_pszThisAppName, pszProcessName);
   
    do
    {
        int i = 0;
        
        system(szBuffer);
       
        i = rmf_osal_GetFileFields(RMF_OSAL_PROCESS_SEARCH_FILE_NAME);
       
        while(i > 4)
        {
            i--;
            //printf("g_paszFields[%d] = %s, searching for pszProcessName = %s\n", i, g_paszFields[i], pszProcessName);
            if(0 == strcmp(g_paszFields[i], pszProcessName))
            {
                //printf("i-4 = %s, i-3 = %s\n", g_paszFields[i-4], g_paszFields[i-3]);
                int iPid = 0;
                if(0 == strcmp(g_paszFields[i-3], "root"))
                {
                    iPid = i-4;
                }
                else if((i > 4) && (0 == strcmp(g_paszFields[i-4], "root")))
                {
                    iPid = i-5;
                }
                else
                {
                    continue;
                }
                nProcessId = strtoul(g_paszFields[iPid], NULL, 10);
                if(nProcessId > 0)
                {
                    printf("%s: Found process '%s', PID = %d\n", g_pszThisAppName, pszProcessName, nProcessId);
                    printf("%s: Retrieving thread information for process '%s', please wait...\n", g_pszThisAppName, pszProcessName);
                    if (0 != remove(RMF_OSAL_PROCESS_SEARCH_FILE_NAME))
                    {
                        perror("remove " RMF_OSAL_PROCESS_SEARCH_FILE_NAME " failed");
                    }
                    break;
                }
            }
        }

        sleep(1);
    } while (0 == nProcessId);
    
    return nProcessId;
#endif
}
#endif

int rmf_osal_GetProcessPid(const char * pszProcessName)
{
    char szBuffer[RMF_OSAL_MAX_PROCESS_NAME_SIZE];
    unsigned int nProcessId = 0;

#ifdef USE_BRCM_SOC
    snprintf(szBuffer, sizeof(szBuffer), "ps ax | grep '%s' | grep 'grep' -v | grep 'run.sh' -v | awk '{print $1}' > %s", pszProcessName, RMF_OSAL_PROCESS_SEARCH_FILE_NAME);
#else
    snprintf(szBuffer, sizeof(szBuffer), "pgrep \"%s\" > %s", pszProcessName, RMF_OSAL_PROCESS_SEARCH_FILE_NAME);
#endif

    printf("szBuffer: '%s'", szBuffer);

    do
    {
        int i = 0;
        int iPid = 0;

        system(szBuffer);

        i = rmf_osal_GetFileFields(RMF_OSAL_PROCESS_SEARCH_FILE_NAME);
        while (i > 0)
        {
            nProcessId = strtoul(g_paszFields[iPid], NULL, 10);
            if(nProcessId > 0)
            {
                printf("%s: Found process '%s', PID = %d\n", g_pszThisAppName, pszProcessName, nProcessId);
                printf("%s: Retrieving thread information for process '%s', please wait...\n", g_pszThisAppName, pszProcessName);
                if (0 != remove(RMF_OSAL_PROCESS_SEARCH_FILE_NAME))
                {
                    perror("remove " RMF_OSAL_PROCESS_SEARCH_FILE_NAME " failed");
                }
                break;
            }

        } 
        sleep(1);
    }while(0 == nProcessId);

    return nProcessId;
}

int rmf_osal_ThreadResGetThreadInfoRpc(unsigned long nCallerThreadId, unsigned long nForThreadId)
{
    long sockfd = 0;
    int error = 0;

    if ((sockfd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        printf("rmf_osal_ThreadResGetThreadInfoRpc : Error opening socket \n");
        return 0;
    }

    else
    {
        char aSzFunctionCall[RMF_OSAL_THREAD_INFO_MAX_CHARS];
        char * pszFunctionCall = aSzFunctionCall;

        struct sockaddr_in6 serv_addr;

        /* Setting all values in a buffer to zero */
        memset((char *) &serv_addr, 0, sizeof(serv_addr));
        /* Code for address family */
        serv_addr.sin6_family = AF_INET;
        /* IP address of the host */
        inet_pton(serv_addr.sin6_family, "127.0.0.1", &(serv_addr.sin6_addr));
        /* Port number */
        serv_addr.sin6_port = htons(RMF_OSAL_THREAD_INFO_CALL_PORT);

        sprintf(pszFunctionCall, "\n;\n;rmf_osal_ThreadResGetThreadInfo(%ul, %ul);\n;\n", nCallerThreadId, nForThreadId);
        if (-1 == sendto(sockfd, pszFunctionCall, strlen(pszFunctionCall), 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr)))
        {
            printf("rmf_osal_ThreadResGetThreadInfoRpc : Error sending data to socket \n");
        }

        close(sockfd);
    }
    return 0;
}

int rmf_osal_RetrieveThreadName(unsigned int nProcessId, RMF_OSAL_PROC_THREAD_INFO & rThreadInfo)
{
    struct timeval currTime;
    gettimeofday(&currTime, NULL);
    double dCurrTime = (currTime.tv_sec*1000000.0)+currTime.tv_usec;
    int nFieldsRetrieved = 0;
    int iTimes = 9;
    if(fabs(dCurrTime - rThreadInfo.dTimeLastThreadInfoRetrieveAttempted) > RMF_OSAL_THREAD_INFO_RETRIEVAL_BACKOFF_INTERVAL)
    {
        char szFileName[RMF_OSAL_THREAD_INFO_MAX_CHARS];
        rmf_osal_ThreadResGetThreadInfoRpc(g_nThisProcessId, rThreadInfo.nId);
        usleep(10000);
        snprintf(szFileName, sizeof(szFileName), "%s%lu", RMF_OSAL_THREAD_INFO_FILE_NAME,  g_nThisProcessId);
        int bThreadInfoRetrieved = 0;
        nFieldsRetrieved = rmf_osal_GetFileFields(szFileName);
        while((iTimes > 0) && (0 == nFieldsRetrieved))
        {
            usleep(10000);
            nFieldsRetrieved = rmf_osal_GetFileFields(szFileName);
            iTimes--;
        }
        while((iTimes > 0) && (strtoul(g_paszFields[RMF_OSAL_PROC_THREAD_STAT_FIELD_PID], NULL, 10) != rThreadInfo.nId))
        {
            usleep(10000);
            nFieldsRetrieved = rmf_osal_GetFileFields(szFileName);
            iTimes--;
        }
        if(nFieldsRetrieved > 0)
        {
            if(strtoul(g_paszFields[RMF_OSAL_PROC_THREAD_STAT_FIELD_PID], NULL, 10) == rThreadInfo.nId)
            {
                strncpy(rThreadInfo.szName, g_paszFields[RMF_OSAL_PROC_THREAD_STAT_FIELD_THREAD_NAME], sizeof(rThreadInfo.szName));
                rThreadInfo.szName[RMF_OSAL_THREAD_INFO_MAX_CHARS-1] = '\0';
                if(0 == strlen(rThreadInfo.szName))
                {
                    //printf("%s: Error: Thread Name not found for Task %lu\n", g_pszThisAppName,rThreadInfo.nId);
                }
                rThreadInfo.nStackStart   = strtoul(g_paszFields[RMF_OSAL_PROC_THREAD_STAT_FIELD_STACK_START], NULL, 16);
                rThreadInfo.nCreationTime = strtoul(g_paszFields[RMF_OSAL_PROC_THREAD_STAT_FIELD_CREATION_TIME], NULL, 10);
                bThreadInfoRetrieved = 1;
            }
        }
        
        if(!bThreadInfoRetrieved)
        {
            rThreadInfo.dTimeLastThreadInfoRetrieveAttempted = dCurrTime;
        }
    }
    if(0 == rThreadInfo.nMaxStack)
    {
        char szBuffer[RMF_OSAL_THREAD_INFO_MAX_CHARS];
        sprintf( szBuffer, "/proc/%u/task/%u/limits", nProcessId, rThreadInfo.nId);
        if ( 0 == rmf_osal_GetFileFields(szBuffer))
        {
            printf("rmf_osal_GetFileFields failed\n");
        }
        rThreadInfo.nMaxStack = strtoul(g_paszFields[RMF_OSAL_PROC_THREAD_STAT_FIELD_MAX_STACK], NULL, 10);
        // rThreadInfo.nMaxStack = (10 - iTimes)*10; // Used to measure retrieval times
    }
    
    return nFieldsRetrieved;
}

int rmf_osal_GetThreadInfo(unsigned int nProcessId, RMF_OSAL_PROC_THREAD_INFO & rThreadInfo)
{
    char szBuffer[RMF_OSAL_MAX_PROCESS_NAME_SIZE];
    sprintf( szBuffer, "/proc/%u/task/%u/stat", nProcessId, rThreadInfo.nId);
    int nFields = rmf_osal_GetFileFields(szBuffer);
    
    if(nFields > 0)
    {
        rThreadInfo.bRefreshed = 1;
        struct timeval currTime;
        gettimeofday(&currTime, NULL);
        
        double dPrevTime = rThreadInfo.dTimeLastUpdated;
        double dCurrTime = (currTime.tv_sec*1000000.0)+currTime.tv_usec;
        rThreadInfo.dTimeLastUpdated    = dCurrTime;
        double dTimeDiff = fabs(dCurrTime-dPrevTime);
        if(0 == dTimeDiff) dTimeDiff = 1;
        
        rThreadInfo.nMinorFaultsPrev    = rThreadInfo.nMinorFaults;
        rThreadInfo.nMajorFaultsPrev    = rThreadInfo.nMajorFaults;
        rThreadInfo.nUserCpuUsagePrev   = rThreadInfo.nUserCpuUsage;
        rThreadInfo.nKernelCpuUsagePrev = rThreadInfo.nKernelCpuUsage;
        
        strncpy(rThreadInfo.szState, g_paszFields[RMF_OSAL_PROC_THREAD_STAT_FIELD_STATE], sizeof(rThreadInfo.szState));
        rThreadInfo.szState[RMF_OSAL_THREAD_MAX_SZSTATE_SIZE-1] = '\0';
        rThreadInfo.nUserCpuUsage   = strtoul(g_paszFields[RMF_OSAL_PROC_THREAD_STAT_FIELD_USER], NULL, 10);
        rThreadInfo.nKernelCpuUsage = strtoul(g_paszFields[RMF_OSAL_PROC_THREAD_STAT_FIELD_KERNEL], NULL, 10);
        rThreadInfo.nPriority       = strtoul(g_paszFields[RMF_OSAL_PROC_THREAD_STAT_FIELD_PRIORITY], NULL, 10);
        rThreadInfo.nNice           = strtoul(g_paszFields[RMF_OSAL_PROC_THREAD_STAT_FIELD_NICE], NULL, 10);
        rThreadInfo.nMinorFaults    = strtoul(g_paszFields[RMF_OSAL_PROC_THREAD_STAT_FIELD_MINOR_FAULTS], NULL, 10);
        rThreadInfo.nMajorFaults    = strtoul(g_paszFields[RMF_OSAL_PROC_THREAD_STAT_FIELD_MAJOR_FAULTS], NULL, 10);
        rThreadInfo.nStackEnd       = strtoul(g_paszFields[RMF_OSAL_PROC_THREAD_STAT_FIELD_STACK_END], NULL, 10);
        
        rThreadInfo.dUserCpuUsagePercent = ((fabs(rThreadInfo.nUserCpuUsage - rThreadInfo.nUserCpuUsagePrev)*g_nTicksPerMS*100000.0)/(dTimeDiff));
        rThreadInfo.dKernelCpuUsagePercent = ((fabs(rThreadInfo.nKernelCpuUsage - rThreadInfo.nKernelCpuUsagePrev)*g_nTicksPerMS*100000.0)/(dTimeDiff));
        //printf("start = %s, end = %s\n", g_paszFields[RMF_OSAL_PROC_THREAD_STAT_FIELD_STACK_START], g_paszFields[RMF_OSAL_PROC_THREAD_STAT_FIELD_STACK_END]);
        if((0 == rThreadInfo.nStackStart) || ('\0' == rThreadInfo.szName[0]))
        {
            // this should be the last call in this function as it modifies g_paszFields
            rmf_osal_RetrieveThreadName(nProcessId, rThreadInfo);
        }
    }
    usleep(10000);
    
    return nFields;
}

int rmf_osal_FindThreadCounter(int nTaskId)
{
    int i = 0;
    for(i = 0; i < g_aCounters.size(); i++)
    {
        RMF_OSAL_PROC_THREAD_INFO & threadInfo = g_aCounters[i];
        if(nTaskId == threadInfo.nId) return i;
    }
    
    return -1;
}

int rmf_osal_RefreshProcessInfo(int nProcessId, const char * pszProcessName)
{
    int iCounter = 0;
    DIR *dir = NULL;
    char szDirName[RMF_OSAL_THREAD_INFO_MAX_CHARS];
    snprintf(szDirName, sizeof(szDirName), "/proc/%lu/task", (unsigned long)nProcessId);
    dir= opendir( szDirName );
    if(NULL == dir)
    {
        printf("%s: Error: Unable to open directory '%s'\n", g_pszThisAppName, szDirName);
        sleep(1);
    }
    else
    {
        struct dirent *dirent = readdir(dir);
        int iCount = 0;
        for(; (g_bContinue) && (NULL != dirent); dirent = readdir(dir))
        {
            unsigned long nTaskId = strtoul(dirent->d_name, NULL, 10);
            if(nTaskId > 0)
            {
                iCounter = rmf_osal_FindThreadCounter(nTaskId);
                if(-1 == iCounter)
                {
                    RMF_OSAL_PROC_THREAD_INFO threadInfo;
                    memset(&threadInfo, 0, sizeof(threadInfo));
                    threadInfo.nId = nTaskId;
                    if(rmf_osal_GetThreadInfo(nProcessId, threadInfo) > 0)
                    {
                        g_aCounters.push_back(threadInfo);
                        {
                            int iRotate = iCount%4;
                            static char rChars[5] = "~\\|/";
                            printf("--> %d %c\r", iCount, rChars[iRotate]); fflush(stdout);
                    }
                }
                }
                else
                {
                    if(0 == rmf_osal_GetThreadInfo(nProcessId, g_aCounters[iCounter]))
                    {
                        g_aCounters.erase(g_aCounters.begin()+iCounter);
                    }
                }
            }
            iCount++;
        }
        closedir(dir);
    }
    printf("              \r");
    
    // clean-out zombies...
    for(iCounter = 0; iCounter < g_aCounters.size(); iCounter++)
    {
        RMF_OSAL_PROC_THREAD_INFO & threadInfo = g_aCounters[iCounter];
        if(!threadInfo.bRefreshed)
        {
            g_aCounters.erase(g_aCounters.begin()+iCounter);
            iCounter--;
        }
    }
    return 0;
}

void sigproc(int a)
{
    g_bContinue = 0;
}

int main(int argc, char * argv[])
{
    RMF_OSAL_PROC_THREAD_INFO threadInfo;
    int i = 0, nBytesRead = 0;
    time_t curtime = time(NULL);
    time_t newtime = curtime;
    int nSysClocksPerSec = 1000;
    int nSysClkTck       = sysconf(_SC_CLK_TCK);
    unsigned int nProcessId = 0;
    int bAttached = 0;
    g_nTicksPerMS = nSysClocksPerSec/nSysClkTck;
    unsigned long nReports = (unsigned long)(-1);
    
    setpriority(PRIO_PROCESS, 0, 19);
    g_pszThisAppName = strrchr(argv[0], '/');
    if(NULL == g_pszThisAppName)
    {
        g_pszThisAppName = argv[0];
    }
    else
    {
        g_pszThisAppName++;
    }

    printf("%s: Usage: %s [ name of process to be analyzed ] [ number of iterations ] [ update interval in milli-seconds ]\n", g_pszThisAppName, g_pszThisAppName);
    //g_pszProcessName = "./mpeos-main";
    if(argc > 1)
    {
        g_pszProcessName = argv[1];
        g_nThisProcessId = syscall(SYS_getpid);
    }
    else
    {
        return -1;
    }
    
    if(argc > 2)
    {
        nReports = atoi(argv[2]);
        nReports++;
    }
    
    if(argc > 3)
    {
        g_ulUpdateInterval = atoi(argv[3]);
    }   
    //printf("%s: nSysClocksPerSec = %d\n", g_pszThisAppName, nSysClocksPerSec);
    //printf("%s: nSysClkTck       = %d\n", g_pszThisAppName, nSysClkTck);
    //printf("%s: nTicksPerMS      = %d\n", g_pszThisAppName, nTicksPerMS);
    char szCleanUp[RMF_OSAL_THREAD_INFO_MAX_CHARS];
    snprintf(szCleanUp, sizeof(szCleanUp), "rm -f %s*", RMF_OSAL_THREAD_INFO_FILE_NAME);
    system(szCleanUp);
    


    nProcessId = rmf_osal_GetProcessPid(g_pszProcessName);
    
    signal(SIGTERM, sigproc);
    signal(SIGINT , sigproc);
    
    while((g_bContinue) && (nReports--))
    {
        rmf_osal_RefreshProcessInfo(nProcessId, g_pszProcessName);
        
        if(g_bContinue)
        {
            printf("\n");
            printf("================================================================================================================================================\n");
            printf("%9s | %9s | %6s %% | %6s %% | %8s | %8s | %8s | %8s | %10s | %10s | %s\n",
            "Thread ID", "State", "User", "Kernel", "Nice/Pri", "Age", "Minor PF", "Major PF", "Stack", "Max Stack", "Thread Name");
            printf("================================================================================================================================================\n");
        }
        double dTotalUserCpuUsagePercent = 0;
        double dTotalKernelCpuUsagePercent = 0;
        int nTotalMinorFaults = 0;
        int nTotalMajorFaults = 0;
        unsigned int nTotalStackUsed = 0;
        unsigned int nTotalStackMax  = 0;
        
        for(i = 0; (g_bContinue) && (i < g_aCounters.size()); i++)
        {
            RMF_OSAL_PROC_THREAD_INFO & threadInfo = g_aCounters[i];
            threadInfo.bRefreshed = 0;
            dTotalUserCpuUsagePercent   += threadInfo.dUserCpuUsagePercent;
            dTotalKernelCpuUsagePercent += threadInfo.dKernelCpuUsagePercent;
            int nMinorFaults             = VLABS((int)(threadInfo.nMinorFaults - threadInfo.nMinorFaultsPrev));
            int nMajorFaults             = VLABS((int)(threadInfo.nMajorFaults - threadInfo.nMajorFaultsPrev));
            nTotalMinorFaults           += nMinorFaults;
            nTotalMajorFaults           += nMajorFaults;
            unsigned int nStackUsed      = 0;
            if(threadInfo.nStackStart > 0)
            {
                nStackUsed = VLABS((int)(threadInfo.nStackStart-threadInfo.nStackEnd));
                nTotalStackUsed += nStackUsed;
            }
            nTotalStackMax              += threadInfo.nMaxStack;
            
            unsigned long nAge      = rmf_osal_GetSystemUptime() - threadInfo.nCreationTime;
            unsigned long nAgeHr  = (nAge/3600);
            unsigned long nAgeMin = (nAge - nAgeHr*3600)/60;
            unsigned long nAgeSec = (nAge - nAgeHr*3600 - nAgeMin*60);

            printf("%9u | %9s | %6.2f %% | %6.2f %% | %4d/%3d | %2u:%02u:%02u | %8u | %8u | %10u | %10u | %s\n",
                    threadInfo.nId,
                    threadInfo.szState,
                    (threadInfo.dUserCpuUsagePercent),
                    (threadInfo.dKernelCpuUsagePercent),
                    threadInfo.nNice,
                    threadInfo.nPriority,
                    nAgeHr,
                    nAgeMin,
                    nAgeSec,
                    nMinorFaults,
                    nMajorFaults,
                    nStackUsed,
                    threadInfo.nMaxStack,
                    (('\0' != threadInfo.szName[0]) ? threadInfo.szName : "< Under-Cover >")
                    );
            usleep(10000);
        }
        
        if(g_bContinue)
        {
            printf("================================================================================================================================================\n");
            printf("%9s | k+u=%2.2f %% | %6.2f %% | %6.2f %% | %8s | %8s | %8u | %8u | %10u | %10u | %d Threads\n",
                    "Totals",
                    (dTotalUserCpuUsagePercent+dTotalKernelCpuUsagePercent),
                    (dTotalUserCpuUsagePercent),
                    (dTotalKernelCpuUsagePercent),
                    "",
                    "",
                    nTotalMinorFaults,
                    nTotalMajorFaults,
                    nTotalStackUsed,
                    nTotalStackMax,
                    g_aCounters.size()
                    );
            printf("================================================================================================================================================\n");
        }
        
        usleep(g_ulUpdateInterval*1000);
    }

    system(szCleanUp);
    printf("\n\n");
    return 0;
}
