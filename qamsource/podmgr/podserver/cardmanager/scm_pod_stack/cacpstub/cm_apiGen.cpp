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

#include <unistd.h>
#ifdef GCC4_XXX
#include <list>
#include <string>
#include <vector>
#else
#include <list.h>
#include <string.h>
#include <vector.h>
#endif
//#include "pfcresource.h"
#include "cardUtils.h"
#include "core_events.h"
#include "cardmanager.h"
#include "cmevents.h"
#include "cm_api.h"
//#include "pfcplugin.h"
#include "poddriver.h"
#include "rmf_osal_sync.h"
#include "ca.h"
#include "rmf_osal_mem.h"
#include "coreUtilityApi.h"
#include <semaphore.h>
#include "bufParseUtilities.h"
#include "utilityMacros.h"
#include "vlMutex.h"
#include "vlEnv.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef GLI
#include "libIBus.h"
#include "sysMgr.h"
#endif

#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif

#define __MTAG__ VL_CARD_MANAGER



//#include "cprot-main.h"
//#define RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET", a, ...)    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", a, ## __VA_ARGS__ )

#define VL_GZ_ENABLED   "/opt/gzenabled"
#define MMI_THREAD_MONITOR_INTERVAL_S 15
#define MMI_FAILURE_TIME_FOR_REBOOT_MINUTE 14
#define MMI_MAX_RETRY_COUNT (int)(60.0 / (MMI_THREAD_MONITOR_INTERVAL_S ) * MMI_FAILURE_TIME_FOR_REBOOT_MINUTE)

#define HOST_POD_ID_STRING_LEN    18 //bytes
//#define RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET", a, ...)        { }

extern "C" int vlCaGetMaxProgramsSupportedByHost();
extern "C" unsigned char vlCardResGetMaxProgramsSupportedByCard();
extern "C" void HAL_SYS_Reboot(const char * pszRequestor, const char * pszReason);
extern "C" rmf_osal_Bool IsHeadendCommInProgress();
extern "C" int vlCPCheckCertificates(

    unsigned char *inp_ucPodDeviceCertData,
    unsigned long  in_ulPodDeviceCertDataLenInByte,

    unsigned char *inp_ucPodManCertData,
    unsigned long  in_ulPodManCertDataLenInByte,

    unsigned char *inp_ucHostRootCertData,
    unsigned long  in_ulHostRootCertDataLenInByte);

extern "C" int vlCPGetCertificatesInfo(
    unsigned char *inp_ucPodDeviceCertData,
    unsigned long  in_ulPodDeviceCertDataLenInByte,
    unsigned char *inp_ucPodManCertData,
    unsigned long  in_ulPodManCertDataLenInByte,
    vlCableCardCertInfo_t *pCardCertInfo );
extern "C" int vlCPGetDevCert(unsigned char **pCert,int *pLen);
extern "C" int vlCPGetManCert(unsigned char**pCert, int *pLen);
extern "C" int vlCPRootCert(unsigned char**pCert, int *pLen);

extern "C" int vlCpWriteNvRamData(int type, unsigned char * pData, int size);
extern "C" unsigned long  RmGetGenFetParamResId();
extern "C" unsigned long fetureGetOpenedResourceId();

extern "C" int time_zone(unsigned char *pData, unsigned long dataLen);
extern "C" int daylight_savings(unsigned char *pData, unsigned long dataLen);
extern "C" int EA_location_code(unsigned char *pData, unsigned long dataLen);
extern "C" int vct_id(unsigned char *pData, unsigned long dataLen);
extern "C" int vlCpGetAuthKeyReqStatus();
extern "C" int vlCpGetCardDevCertCheckStatus();
extern "C" int vlCpGetCciAuthDoneCount();
extern "C" int vlCpGetCpKeyGenerationReqCount();
extern "C" int HomingCardFirmwareUpgradeGetState();
int vlgInitCAResources();
void vlCAPostNoPrgSelectEvent(int PrgIndx);
int GetCableCardCertInfo(vlCableCardCertInfo_t *pCardCertInfo);
extern unsigned char GetPodInsertedStatus();
extern unsigned short vlg_CaSystemId;
extern char Host_ID[];
extern char POD_ID[];
extern vlCardAppInfo_t CardAppInfo;
extern char    g_nvmAuthStatus;
//Start:Added by Mamidi for SNMP purpose
HostCATypeInfo_t vlg_HostCaTypeInfo;
static vlMutex g_AppInfoPageLock;

extern unsigned char vlg_CardMibRootOidLen;
extern unsigned char vlg_CardMibRootOid[];
//char vlg_SerialNumber[255];
//char vlg_HostID[255];
extern "C" int   vlIsDsgOrIpduMode();
extern "C" int   vlIsIpduMode();
extern "C" int vlSendCardValidationStatus(unsigned short sessNum,int respTime);

extern "C" int vlClearCaResAllocatedForPrgIndex(unsigned char PrgIndex);
extern "C" int vlSetCaResAllocatedForPrgIndex(unsigned char PrgIndex, unsigned char Ltsid,unsigned short PrgNum);
extern "C" int vlGetCaAllocStateForPrgIndex(unsigned char PrgIndex, unsigned char Ltsid,unsigned short PrgNum, unsigned char *pAllocState);

extern "C" int vlGetCciCAenableFlagForPrgIndex(unsigned char PrgIndex, unsigned char Ltsid,unsigned short PrgNum, unsigned char *pCci,unsigned char *pCAEnable);

extern "C" int vlGetCciCAenableFlag( unsigned char Ltsid,unsigned short PrgNum, unsigned char *pCci,unsigned char *pCAEnable);
extern "C" int vlPostCciCAenableEvent(unsigned char Cci,unsigned char CAenable, unsigned char Ltsid, unsigned short PrgNum);


extern unsigned char vlg_CardValidationStatus;
unsigned short GetFeatureParamVctId();
  cCardManagerIF  *cCardManagerIF::_instance = 0;
  rmf_osal_Mutex      cCardManagerIF::mutex;
  uint8_t      cExtChannelFlow::flows_Remaining=0;
  rmf_osal_Mutex      cCardManagerIF::extChMutex;
  sem_t    cCardManagerIF::extChSem;

static CAResourceHandle_t *ResourceHandles;//[MAX_RESOURCE_HANDLES];
static int vlg_MaxProgramsSupportedByHost =0;
static int vlg_MaxProgramsSupportedByCard = 0;

#define VL_RETURN_CASE_STRING(x)   case x: return #x;
extern int GetCardFeatureList(unsigned char *pList, unsigned char *pListSize);

static rmf_osal_ThreadId rmf_cc_mmi_monitorThreadId = 0;

void rmf_cc_mmi_page_monitor_task(void * arg)
{
    int iPage = 0;
    const char * pszTag  = NULL;
    const char * pszSkip = ":</bB> ";
    const char * pszVal  = NULL;
    const char * pszPtr = NULL;
    string strPage;
    int bPrevValue = -1;
    int bCurrValue = 0;
    cCardManagerIF * pCmIf = NULL;
    unsigned short retryCount = 0;
    unsigned short maxRetryCount = MMI_MAX_RETRY_COUNT;

    bool retVal = false;
#ifdef GLI
     IARM_Bus_SYSMgr_EventData_t eventData;
#endif

    rmf_osal_threadSleep(MMI_THREAD_MONITOR_INTERVAL_S * 1000, 0); // Wait for a while before we start hassling the card.
    while(1)
    {
        rmf_osal_threadSleep(MMI_THREAD_MONITOR_INTERVAL_S * 1000, 0); // 15 Seconds
        if(false == GetPodInsertedStatus()) continue;
        if(NULL == pCmIf) pCmIf = cCardManagerIF::getInstance();
        if(NULL == pCmIf) continue;

        switch((CardAppInfo.CardManufactureId & 0xFF00) >> 8)
        {
        case 0:
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: cablecard manufacturer: motorola\n", __FUNCTION__);
            iPage  = 5;
            pszTag = "Entitlements";
            //For Testing//pszVal = "Incomplete";
            pszVal = "Complete";
#ifdef GLI
                eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_MOTO_ENTITLEMENT;
#endif
            break;

        case 1:
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: cablecard manufacturer: cisco\n", __FUNCTION__);
            iPage  = 0;
            pszTag = "Status";
            //For Testing//pszVal = "Not Staged";
            pszVal = "Ready";
#ifdef GLI
                eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_CARD_CISCO_STATUS;
#endif
            break;

        case 2:
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: cablecard manufacturer: SCM\n", __FUNCTION__);
            break;

        default:
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: cablecard manufacturer: UNKNOWN\n", __FUNCTION__);
            break;
        }

        bCurrValue = 0;
                /* temp fix to check with debug card */
                if(bCurrValue == bPrevValue)
                {
                        break;
                }
        retVal = pCmIf->vlGetPodMmi(strPage, CardAppInfo.Apps[iPage].AppUrl, CardAppInfo.Apps[iPage].AppUrlLen);
        if( ( false == retVal ) && ( 0 == IsHeadendCommInProgress() ) && ( 1 != HomingCardFirmwareUpgradeGetState() ) )
        {
                retryCount ++;
        }
        else
        {
                retryCount = 0;
        }

        if( retryCount == maxRetryCount )
        {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "%s: Cable card is not responding, rebooting \n", __FUNCTION__);
                rmf_osal_threadSleep(MMI_THREAD_MONITOR_INTERVAL_S * 1000, 0);
                HAL_SYS_Reboot( "SASWatchDog", "CardNotResponding" );
        }
        pszPtr = strPage.c_str();
        if((NULL != pszPtr) && (NULL != pszTag) && (NULL != pszSkip) && (NULL != pszVal))
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: Scanning '%s'.\n", __FUNCTION__, pszPtr);
            pszPtr = strstr(pszPtr, pszTag);
            if(NULL != pszPtr)
            {
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: Found Tag '%s'.\n", __FUNCTION__, pszTag);
                pszPtr += strlen(pszTag);
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: Skipping '%s' for '%s'.\n", __FUNCTION__, pszSkip, pszPtr);
                pszPtr = pszPtr + strspn(pszPtr, pszSkip);
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: Looking at '%s'.\n", __FUNCTION__, pszPtr);
                if(0 == strncmp(pszPtr, pszVal, strlen(pszVal)))
                {
                    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: Found Value '%s'.\n", __FUNCTION__, pszVal);
                    bCurrValue = 1;
                }
                if(bCurrValue != bPrevValue)
                {
                    bPrevValue = bCurrValue;
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: Posting Changed Event. Current status = %d\n", __FUNCTION__, bCurrValue);
#ifdef GLI
                    eventData.data.systemStates.state = bCurrValue;
                    eventData.data.systemStates.error = 0;
                    IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
#endif
                }
            }
        }
    }
    return ;
}

extern "C" int rmfStartCableCardAppInfoPageMonitor()
{VL_AUTO_LOCK(g_AppInfoPageLock); // Thread Protection.

#ifdef GLI
    struct stat st;
    bool ret=true;

    if((0 == stat(VL_GZ_ENABLED, &st) && (0 != st.st_ino)))
    {
        if(0 == rmf_cc_mmi_monitorThreadId)
        {
                rmf_Error ret = RMF_SUCCESS;
                if ( ( ret =
                        rmf_osal_threadCreate( rmf_cc_mmi_page_monitor_task,
                        NULL,
                        RMF_OSAL_THREAD_PRIOR_DFLT,
                        RMF_OSAL_THREAD_STACK_SIZE,
                        &rmf_cc_mmi_monitorThreadId,
                        "rmf_cc_mmi_page_monitor_task" ) ) != RMF_SUCCESS )
                {
                        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
                        "%s: failed to create rmf_cc_mmi_page_monitor_task thread (err=%d).\n",
                        __FUNCTION__, ret );
                }
        }
    }
    else
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","%s: '%s' is NOT set. Not starting vl_cc_mmi_page_monitor_task().\n", __FUNCTION__, VL_GZ_ENABLED );
    }
#endif
        return 0;
}

extern "C" char * cExtChannelGetFlowStatusName(XFlowStatus_t status)
{
    static char szUnknown[50];

    switch(status)
    {
        VL_RETURN_CASE_STRING(FLOW_CREATED                  );
        VL_RETURN_CASE_STRING(FLOW_DENIED                   );
        VL_RETURN_CASE_STRING(INVALID_SERVICE_TYPE          );
        VL_RETURN_CASE_STRING(NETWORK_UNAVAILABLE           );
        VL_RETURN_CASE_STRING(NETWORK_BUSY                  );
        VL_RETURN_CASE_STRING(FLOW_CREATION_IN_PROGRESS     );
        VL_RETURN_CASE_STRING(FLOW_UNINITIALIZED            );
        VL_RETURN_CASE_STRING(EXTCH_RESOURCE_NOT_OPEN       );
        VL_RETURN_CASE_STRING(EXTCH_NO_RESPONSE             );
        VL_RETURN_CASE_STRING(EXTCH_RESOURCE_DELETE_DENIED  );
        VL_RETURN_CASE_STRING(EXTCH_PID_FLOW_EXISTS         );

        default:
        {
        }
        break;
    }
   // sprintf(szUnknown, "Unknown (%d)", status);
     snprintf(szUnknown,sizeof(szUnknown), "Unknown (%d)", status);
    return szUnknown;
}

cCardManagerIF::cCardManagerIF(/*pfcKernel *pKernel*/)
{

    rmf_osal_mutexNew(&mutex);
    rmf_osal_mutexNew(&extChMutex);
    sem_init(&extChSem, 0 , 0);

    this->refCount++;
    this->CMInstance = NULL; // this is initialized when CardManager starts
    this->mmi_handler = 0;
    this->cableCardVersion =  0;
    this->mfgID = 0;
    this->cableCardSerialNumLen=0;
    this->cb_fn= NULL;
    this->refCount = 0;
    memset(this->cardMAC,0,sizeof(this->cardMAC));
    memset(&this->caID,0,sizeof(this->caID) );
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD"," ###Init cCardManagerIF::cCardManagerIF \n");
    vlgInitCAResources();


}

/*
cCardManagerIF::cCardManagerIF()
{

    this->refCount++;
    this->CMInstance = NULL; // this is initialized when CardManager starts
    this->mmi_handler = 0;
    this->cableCardVersion =  0;
    this->mfgID = 0;
    this->cableCardSerialNumLen=0;
    this->cb_fn= NULL;
    this->refCount = 0;
    memset(this->cardMAC,0,sizeof(this->cardMAC));
    memset(&this->caID,0,sizeof(this->caID) );

}
*/

cCardManagerIF::~cCardManagerIF()
{

                rmf_osal_mutexDelete(mutex);
              rmf_osal_mutexDelete(extChMutex);
              sem_destroy(&extChSem);
}

cCardManagerIF* cCardManagerIF::getInstance(/*pfcKernel *pKernel*/)
{

     // check-lock-check pattern for thread safety
//    if(_instance == 0)
//    {

//        rmf_osal_mutexAcquire(mutex); // lock static class mutex (another thread may be getting the instance at same time
                // and may actually create the instance , this lock will ensure that which ever thread has
                // the mutex actually creates the object, when we get past this lock the object is either
                // guranteed created or not, so we need to check again

        if(_instance == 0)
        {
            // we have the lock, the instance was not yet created so we create it
            _instance=new cCardManagerIF(/*pKernel*/);

        }
//        rmf_osal_mutexRelease(mutex); // unlock static class mutex

//    }
     RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET", "cCardManagerIF ::_instance = %p\n",(void *)_instance);
    return _instance;

}


cCardManagerIF* cCardManagerIF::deleteInstance(/*pfcKernel *pKernel*/)
{


      rmf_osal_mutexAcquire(mutex);
      if(refCount != 0)
      {
          refCount--;
      }
      rmf_osal_mutexRelease(mutex);
      if(refCount == 0)
      {
          //VLFREE_INFO2(_instance);
           delete _instance;
        _instance = 0;
      }

         RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.TARGET", "cCardManagerIF ::_instance = %p\n",(void *)_instance);
    return _instance;

}

#if 0
int cCardManagerIF::GetCableCardFeatures(unsigned char *pList, unsigned char *pSize)
{

    if(POD_REMOVED == this->CMInstance->cablecardFlag)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","cCardManagerIF::GetCableCardFeatures: Card Not Prasent >>> POD_REMOVED \n");
        return -1;
    }

    return GetCardFeatureList(pList, pSize);
}
#endif

unsigned char cCardManagerIF::GetCableCardState()
{
    if(CMInstance)
    {
        return this->CMInstance->CmcablecardFlag;
    }
    else
    {
        return CARD_INVALID;
    }
}

void cCardManagerIF::SetDisplayCapabilities()
{

}



CMStatus_t cCardManagerIF::SendAPDU(short sesnId ,int tag, uint8_t *apdu, uint32_t apduLength)
{
    CMStatus_t retValue = CM_NO_ERROR;
    if(cPOD_driver::PODSendRawAPDU(sesnId, tag,  apdu, apduLength) == -1)
        retValue = CM_SEND_APDU_ERROR;

    return retValue;

}


void cCardManagerIF::AssumeOCAPHandlers(bool mode)
{

    this->CMInstance->ocapMode = mode;   //does this have to persist over reboots?
}
void cCardManagerIF::AssumeCaAPDUHandlers(bool mode)
{

    this->CMInstance->CaAPDURoute = mode;   //does this have to persist over reboots?
}


void cCardManagerIF::AssumeMMIHandler(bool)
{

    this->mmi_handler = 1;

}

void cCardManagerIF::ReleaseMMIHandler(bool)
{

    this->mmi_handler = 0;

}

bool cCardManagerIF::IsMMIhandlerPresent(void)
{

    return this->mmi_handler;
}


void    cCardManagerIF::NotifyChannelChange( IOResourceHandle    hIn,unsigned short usVideoPid,unsigned short usAudioPid)
{

    this->CMInstance->pCA->NotifyChannelChange(hIn, usVideoPid, usAudioPid);
}

// RAJASREE: 092606: Added new methods needed by OCAP [start]

int cCardManagerIF::GetCardManufacturerId()
{
    return mfgID;
}

int cCardManagerIF::GetCardVersionNumber()
{
    return cableCardVersion;
}

void cCardManagerIF::GetApplicationInfo(vector<cApplicationInfoList *> &list)
{
    list = AppInfoList;
}

int cCardManagerIF::GetLatestMMIMessageFromCableCard(unsigned char **pData,unsigned long *pSize)
{
    if(CMInstance)
    {

        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","cCardManagerIF::GetLatestMMIMessageFromCableCard calling vlAppInfoGetMMIMessageFromCableCard #### \n");
        return CMInstance->pAppInfo->vlAppInfoGetMMIMessageFromCableCard(pData,pSize);
    }
    return -1;
}
static rmf_osal_Mutex vlg_vlCaPmtMutex;
int vlCardCaPmtInit()
{
  rmf_Error err;
  err= rmf_osal_mutexNew(&vlg_vlCaPmtMutex);
  if(RMF_SUCCESS != err)
  {
    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","vlCardCaPmtInit: Error !! Creating the Mutex \n");
    return -1;
  }
  return 0;
}

void vlCardCaPmtMutexBegin()
{
  if(vlg_vlCaPmtMutex != 0)
  {
      rmf_osal_mutexAcquire(vlg_vlCaPmtMutex);
  }
}
void vlCardCaPmtMutexEnd()
{
  if(vlg_vlCaPmtMutex != 0)
  {
      rmf_osal_mutexRelease(vlg_vlCaPmtMutex);
  }
}
int vlgInitCAResources()
{
    return 0;
}
extern "C" int vlgInitHostCertInfo()
{
    /* stub  */
    return 0;
}
/* Returns  CM_IF_CA_RES_ALLOC_SUCCESS on successful allocation */
/* Returns  CM_IF_CA_RES_BUSY_WITH_LTSID_PRG if the resource is already allocated with given ltsid and prg number
the called will get the resource handle, it means the caller is granted the resource for given ltsid and program number
but the caller  should not call the ca_pmt call to the cardmanager */
/* Returns CM_IF_CA_RES_ALLOC_FAILED_ALL_RES_BUSY if all the CA resources are busy */
int cCardManagerIF::vlAllocateCaFreeResource(int *pResourceHandle,unsigned char Ltsid, unsigned short PrgNum)
{
	return CM_IF_CA_RES_ALLOC_FAILED_ALL_RES_BUSY;
}

int cCardManagerIF::vlGetCMCciCAenableFlag( unsigned char Ltsid,unsigned short PrgNum, unsigned char *pCci,unsigned char *pCAEnable)
{
  return 0;
}
int cCardManagerIF::vlDeAllocCaResource(int ResourceHandle)
{
	return 0;
}

int cCardManagerIF::InvokeCableCardReset()
{
  return 0;
}


int GetLuhnForPodIdHostId(unsigned short ManId, unsigned long DeviceId,char *pHostIdLu)
{
	return 0;
}
int GetHostIdLuhn( unsigned char *pHostId,  char *pHostIdLu )
{
	return 0;
}
int GetPodIdLuhn( unsigned char *pPodId,  char *pPodIdLu )
{
	return 0;
}



int cCardManagerIF::SNMPGet_ocStbHostHostID(char* HostID,char len)
{
	return 0;
}

int cCardManagerIF::SNMPGet_ocStbHostCapabilities(int*  pHostCapabilities_val)
{
	return 0;
}
int cCardManagerIF::SNMPget_ocStbHostSecurityIdentifier(HostCATypeInfo_t* pHostCAtypeinfo)
{
	return 0;
}


//End:Added by Mamidi for SNMP purpose
void vlPrintCertInfo(vlCableCardCertInfo_t *pCardCertInfo)
{
}
int GetCableCardCertInfo(vlCableCardCertInfo_t *pCardCertInfo)
{
	return 0;
}
int cCardManagerIF::SNMPGetCableCardCertInfo(vlCableCardCertInfo_t *pCardCertInfo)
{
    return 0;
}
int vlg_ServerQueryReply = 0;
int vlg_ServerQueryNum = 201;
int vlg_SrvQryReplyLen = 0;
unsigned char *vlg_SrvQryReplyData;
static char vlg_pszCardNotInsertedName[] = "Cable Card Not Inserted";
static char vlg_pszCardNotInsertedPage[] = "<html><body><table border=\"1\"><tr><td><CENTER>Cable Card Not Inserted<BR><U><B>Cable Card Not Inserted</B></U><BR></td></tr></table></body></html>";

int vlMCardGetSerialNumberFromMMI(char * pszSerialNumber, int nCapacity)
{
	return 0;
}

static char vlg_szFwVersion[128] = "";

int vlMCardGetCardVersionNumberFromMMI(char * pszVersionNumber, int nCapacity)
{
	return 0;
}

int cCardManagerIF::SNMPGetApplicationInfo(vlCableCardAppInfo_t *pAppInfo)
{
	return 0;
}


#define VL_MMI_PAGE_DEBUG_LEVEL     RDK_LOG_DEBUG

size_t ccApp_findStringCaseInsensitive(const string & rString, const string strNamedTag)
{
    const char * str = strcasestr(rString.c_str(), strNamedTag.c_str()); // case insensitive
    if(NULL != str) return (str - rString.c_str());
    return string::npos;
}

void ccApp_removeNamedOuterTag(string & rStrHtml, const string & rNamedTag)
{
}

void ccApp_addHtmlHeaders(string & rStrHtml)
{
    rStrHtml = "<html><body>" + rStrHtml + "</body></html>";
}

void ccApp_addHtmlTable(string & rStrHtml)
{
    rStrHtml = "<table border=\"1\">" + rStrHtml + "</table>";
}

void ccApp_addHtmlTableColumn(string & rStrHtml)
{
    rStrHtml = "<tr><td>" + rStrHtml + "</td></tr>";
}

bool ccApp_getLink(const string & rStrHtml, string & rStrLink, string & rStrLinkName)
{
    return true;
}

static string vlg_strAppInfoTransferPage;

bool cCardManagerIF::vlGetPodMmi(string & rStrReply, const unsigned char* message, int msgLength)
{
	return true;
}

bool ccApp_isLinkPresent(vector<string> & rListLinks, string & rString, string & rStrLinkName)
{
    return false;
}

bool cCardManagerIF::vlGetPodSingleMmi(string & rStrReply, const unsigned char* message, int msgLength)
{
    return true;
}

extern "C" int vlTransferCcAppInfoPage(const char * pszPage, int nBytes)
{
        return 0;
}

static char vlg_pszNoResponse[] = "<tr><td><CENTER><u><b>Page %d</b></u><BR><b>URL: </b>'%s'<BR><U><B>Cable-Card Did Not Respond</B></U><BR>Cable-Card may need fixed firmware.<BR><b>FW Ver:</b> '%s' / 0x%04X<br></td></tr>";
static char vlg_pszSubPageNoResponse[] = "<tr><td><CENTER><u><b>Page %d, Sub-Page %d</b></u><BR><b>URL: </b>'%s'<BR><U><B>Cable-Card Did Not Respond</B></U><BR>Cable-Card may need fixed firmware.<BR><b>FW Ver:</b> '%s' / 0x%04X<br></td></tr>";
static char vlg_pszSubPagesStubbed[] = "<tr><td><CENTER><u><b>Page %d, Sub-Page %d</b></u><BR><U><B>Additional Sub-Pages present.</B></U><BR>Sub-Pages trimmed.<BR></td></tr>";
static char vlg_pszPageDoesNotExist[] = "<html><body><table border=\"1\"><tr><td><CENTER>Non-Existing Page<BR><U><B>This page does not exist.</B></U><BR></td></tr></table></body></html>";
static char vlg_pszPatchInfo[] = "<U><B>Buffer Too Big - NetSNMP Patch Required<BR></B></U>\
<U><B>Index:</B> net-snmp-x.x.x/include/net-snmp/library/snmp_api.h</U><BR>\
- #define SNMP_MAX_MSG_SIZE          1472 // ethernet MTU minus IP/UDP header <BR>\
+ #define SNMP_MAX_MSG_SIZE          10240 // ethernet MTU minus IP/UDP header <BR><BR>";

#define HTML_HEAD_BODY_SIZE 64

bool cCardManagerIF::vlGetPodMmiTable(int iIndex, string & rStrReply, int nSnmpBufLimit)
{
	return true;
}

int cCardManagerIF::SNMPGetApplicationInfoWithPages(vlCableCardAppInfo_t *pAppInfo)
{
	return 0;
}
int cCardManagerIF::GetCableCardVctId(unsigned short *pVctId)
{
    return 0;
}
int cCardManagerIF::GetCableCardDetails(vlCableCardDetails_t *pCardDetails)
{
    return 0;
}

int cCardManagerIF::GetCableCardId(vlCableCardId_t *pId)
{
	return 0;
}
int cCardManagerIF::SNMPGetCardIdBndStsOobMode(vlCableCardIdBndStsOobMode_t *pCardInfo)
{
	return 0;
}



int cCardManagerIF::SNMPSendCardMibAccSnmpReq( unsigned char *pMessage, unsigned long MsgLength)
{
   return 0;
}

int cCardManagerIF::SNMPGetGenFetResourceId(  unsigned long *pData)
{
	return 0;
}
int cCardManagerIF::SNMPGetGenFetTimeZoneOffset(  int *pData)
{
    unsigned char TimeZone[4];
    int TZ=0;
    if(false == GetPodInsertedStatus() )
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD"," POD NOT INSERTED: return Error \n");

        return -1;
    }
    if(pData == NULL)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","SNMPGetGenFetTimeZoneOffset: return Null pointer passed \n");
        return -1;

    }
    if( 2 != time_zone(TimeZone,2) )
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD"," SNMPGetGenFetTimeZoneOffset: Error Geting timezone \n");
        return -1;
    }
    TZ = (int)( ( TimeZone[0] << 8 ) | TimeZone[1] );
//    TZ = TZ/60;//in hours

    *pData = TZ;
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," SNMPGetGenFetTimeZoneOffset: Time zone :%d returned \n",*pData);
/*    if( TZ < -12 || TZ > 12)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD"," SNMPGetGenFetTimeZoneOffset: Error in Time zone value \n");
        return -1;
    }*/

RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","cCardManagerIF::SNMPGetGenFetTimeZoneOffset: Returning Time Zone offset:0x%X \n",*pData);
    return 0;

}
int cCardManagerIF::SNMPGetGenFetDayLightSavingsTimeDelta(  char *pData)
{
    unsigned char DLSav[12];
    if(false == GetPodInsertedStatus() )
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD"," POD NOT INSERTED: return Error \n");

        return -1;
    }
    if(pData == NULL)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","SNMPGetGenFetDayLightSavingsTimeDelta: return Null pointer passed \n");
        return -1;

    }
    if(10 != daylight_savings(DLSav,10))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","SNMPGetGenFetDayLightSavingsTimeDelta: daylight_savings returned Error !! \n");
        return -1;
    }
    if(DLSav[0] != 2 )
    {
        *pData = 0;
    }
    else
        *pData = DLSav[1];

RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","cCardManagerIF::SNMPGetGenFetDayLightSavingsTimeDelta: Returning DayLight Savings Delta:0x%X \n",*pData);
    return 0;

}
int cCardManagerIF::SNMPGetGenFetDayLightSavingsTimeEntry(  unsigned long *pData)
{
    unsigned char DLSav[12];
    if(false == GetPodInsertedStatus() )
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD"," POD NOT INSERTED: return Error \n");

        return -1;
    }
    if(pData == NULL)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","SNMPGetGenFetDayLightSavingsTimeEntry: return Null pointer passed \n");
        return -1;

    }
    if(10 != daylight_savings(DLSav,10))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","SNMPGetGenFetDayLightSavingsTimeEntry: daylight_savings returned Error !! \n");
        return -1;
    }
    if(DLSav[0] != 2 )
    {
        *pData = 0;
    }
    else
        *pData = ((DLSav[2] << 24)|(DLSav[3] << 16)|(DLSav[4] << 8)|(DLSav[5] << 0));

RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","cCardManagerIF::SNMPGetGenFetDayLightSavingsTimeEntry: Returning DayLight Savings Entry:0x%X \n",*pData);

    return 0;

}
int cCardManagerIF::SNMPGetGenFetDayLightSavingsTimeExit(  unsigned long *pData)
{
    unsigned char DLSav[12];
    if(false == GetPodInsertedStatus() )
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD"," POD NOT INSERTED: return Error \n");

        return -1;
    }
    if(pData == NULL)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","SNMPGetGenFetDayLightSavingsTimeExit: return Null pointer passed \n");
        return -1;

    }
    if(10 != daylight_savings(DLSav,10))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","SNMPGetGenFetDayLightSavingsTimeExit: daylight_savings returned Error !! \n");
        return -1;
    }
    if(DLSav[0] != 2 )
    {
        *pData = 0;
    }
    else
        *pData = ((DLSav[6] << 24)|(DLSav[7] << 16)|(DLSav[8] << 8)|(DLSav[9] << 0));

RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD","cCardManagerIF::SNMPGetGenFetDayLightSavingsTimeExit: Returning DayLight Savings Exit:0x%X \n",*pData);
    return 0;

}
int cCardManagerIF::SNMPGetGenFetEasLocationCode(unsigned char *pState,unsigned short *pDDDDxxCCCCCCCCCC)
{
    return 0;
}
int cCardManagerIF::SNMPGetGenFetVctId(  unsigned short *pData)
{
	return 0;
}
int cCardManagerIF::SNMPGetCpInfo(  unsigned short *pData)
{
    return 0;
}
int cCardManagerIF::SNMPGetCpAuthKeyStatus(  int *pData)
{
    return 0;

}
int cCardManagerIF::SNMPGetCpCertCheck(  int *pData)
{
	return 0;
}
int cCardManagerIF::SNMPGetCpCciChallengeCount(  unsigned long *pData)
{
    return 0;
}
int cCardManagerIF::SNMPGetCpKeyGenerationReqCount(  unsigned long *pData)
{
	return 0;
}
int cCardManagerIF::SNMPGetCpIdList(  unsigned long *pData)
{
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// implementing pfcEventCableCard_RawAPDU class



void EventCableCard_RawAPDU_free_fn(void *ptr)
{

}




