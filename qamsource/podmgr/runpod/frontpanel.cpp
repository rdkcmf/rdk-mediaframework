/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2018 RDK Management
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

#include <glib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>

#include "rdk_debug.h"
#include "rmf_osal_event.h"
#include "rmf_osal_init.h"
#include "rmf_osal_thread.h"

#include "frontPanelIndicator.hpp"
#include "frontPanelConfig.hpp"
#include "frontPanelSettings.hpp"
#include "manager.hpp"
#include "dsMgr.h"
#include "libIBus.h"
#include "libIBusDaemon.h"
#include <semaphore.h>
#include "diagMsgEvent.h"
#include "podimpl.h"
#include "rmf_osal_util.h"
#ifdef GLI
#include "sysMgr.h"
#include "libIBus.h"
#endif

#include "rmf_oobsicache.h"

#define VL_INIT_TEXT    "init"
#define VL_COLD_INIT_TEXT    "cold.init."

//using namespace rdkutil;

rmf_Error setupTime(void);
// 
static int g_timeSet;
static bool isBootComplete = false;
void cdlThread (void *arg);
bool getBootStatus();
void setBootStatus(bool status);
static rmf_osal_ThreadId g_cdlThreadId;
static rmf_osal_ThreadId g_stackMonThreadId;
static sem_t* threadSync;
static bool isInit = false;
static bool gEnableFPUpdate = true;
static volatile bool bTimeUpdatedStarted = false;
static bool g_fp_supports_text = false;
static void enableFPUpdates(bool flag);
#define TEXT_DELAY 3
#define BOOT_PROGRESS_TEXT_COUNT 2
static rmf_osal_ThreadId g_cd2ThreadId;
static void cdlEventThread(void *arg);
static void handle_time_limited_event_driven_boot_thread( void *arg );
static void _RDKEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);

//#define Vprintf printf
/**
  * Stack boot progress.
  */
extern "C" rmf_osal_eventmanager_handle_t get_pod_event_manager();
extern "C" rmf_Error create_pod_event_manager();
extern unsigned char GetPodInsertedStatus();
extern "C" int vlIsDsgOrIpduMode();
extern "C" int HomingCardFirmwareUpgradeGetState();

static rmf_OobSiCache* m_pSiCache;

/**
* Error message codes
*/
typedef enum _BootErrorCode
{
	BOOT_ERROR_CARD_CP,
	BOOT_ERROR_PROXY_DOWN_TOO_LONG,
	BOOT_ERROR_CARD_DSG,
	BOOT_ERROR_ESTB_IP,
	BOOT_ERROR_SI,
	BOOT_ERROR_STT,
	BOOT_ERROR_BOX_DISCONNECTED,
	BOOT_ERROR_ECM_OPERATIONAL,	
	BOOT_ERROR_MAX,	
	BOOT_ERROR_NONE,
} BootErrorCode;

static BootErrorCode eBroadcastedBootError = BOOT_ERROR_NONE;
/**
 * Error messages from RDK-1235:
 *   RDK-03001 BOX Stuck at 1 Dot - Card Initialization failure
 *   RDK-03002 BOX Stuck in 3 Dots - Proxy application or CMAC application doesn't load
 *   RDK-03003 DSG Acquisition(this step passes very quickly and is hardly noticeable)
 *   RDK-03004 BOX Stuck in 3 Dots - STB IP Acquisition [covered in PMHW-221]
 *   RDK-03005 BOX Stuck in 3 Dots - Channel Map Acquisition
 *   RDK-03006 BOX Stuck in 3 Dots - STT not available
 *   RDK-03007 BOX Stuck in 3 Dots - STB is in Disconnected mode
 */
const char *bootErrorMessage[BOOT_ERROR_MAX]=
        {"RDK-03001", "RDK-03002","RDK-03003","RDK-03009", "RDK-03005", "RDK-03006", "RDK-03007", "RDK-03004"};
		
static bool bootErrorTable[BOOT_ERROR_MAX]={false, false, false, false, false, false, false, false};
time_t start_time_point = 0;

typedef enum _BootProgress
{
    BOOT_PROGRESS_START,
    BOOT_PROGRESS_GET_CARDHOST_DSG,
    BOOT_PROGRESS_GET_DOCIS_IP,
    BOOT_PROGRESS_GET_IP,
    BOOT_PROGRESS_PROXY,
    BOOT_PROGRESS_DONE,

    BOOT_PROGRESS_LAST
} BootProgress;

typedef enum _EventStatus
{
    EVENT_STATUS_NOT_START,
    EVENT_STATUS_START,
    EVENT_STATUS_COMPLETE,
    EVENT_STATUS_FLST,
    EVENT_STATUS_FLDN,
    EVENT_STATUS_RBOOT,
    EVENT_STATUS_FAIL
} EventStatus;

typedef enum _AppState
{
    APP_STATE_BOOT,
    APP_STATE_IMAGE,
    APP_STATE_TEXT,
    APP_STATE_WATCHTV,
    /* APP_STATE_CDL, */
    APP_STATE_DONE,
    APP_STATE_ERROR,
    APP_STATE_EXIT,

    APP_STATE_LAST
} AppState;

#define EAS_FP_TEXT "EAS"	


#define UPDATE_DELAY 250 
#define STT_DOWN_MSG_DELAY 300 //Delay is 300seconds
#define SI_DOWN_MSG_DELAY 600 //Delay is 600 seconds
static int PROXY_DOWN_MSG_DELAY = 720; //seconds to wait before displaying Proxy down msg.

static rmf_osal_Bool SIReceived = false;	
static rmf_osal_Bool isProxyUp = false;

static rmf_osal_Bool siSTTReceived = false;

const char *fp_messages[BOOT_PROGRESS_LAST] =
{
    "boot",
    "cAst",
    "iPst",
    "sIst",
    "dOnE",
    ""
};

/**
 * Top line progress text headers.
 */
const char *top_labels[BOOT_PROGRESS_LAST][BOOT_PROGRESS_TEXT_COUNT] =
{
    {NULL,                              NULL},
    {"Startup and Activation",          "Startup and Activation"},
    {"",                                ""},
    {"DHCP STB IP Address Acquisition", "DHCP STB IP Address Acquisition"},
    {"Application Launch",              "Application Launch"},
    {"",                                ""}
};

/**
 * Middle line progress text labels.
 */
const char *mid_labels[BOOT_PROGRESS_LAST][BOOT_PROGRESS_TEXT_COUNT] =
{
    {NULL,                            NULL},
    {"Card/Host Initialization",      "DSG Acquisition"},
    {"DOCSIS Registration",           "Firmware Download Not Started"},
    {"eSTB_IP_Acquisition",           "Channel Map Acquisition"},
    {"XRE Receiver Launching",        "XRE Receiver Launching"},
    {"",                                ""}
};

/**
 * Bottom line progress text labels.
 */
const char *lower_labels[BOOT_PROGRESS_LAST][BOOT_PROGRESS_TEXT_COUNT] =
{
    {NULL,                     NULL},
    {NULL,                     NULL},
    {NULL,                     NULL},
    {NULL,                     "Number of Channels Acquired:"},
    {"XRE Receiver Launching", "XRE Receiver Launching"},
    {NULL,                     NULL}
};

EventStatus card_host_init_status;
EventStatus dsg_acquire_status;
EventStatus docsis_reg_status;
EventStatus ip_acquire_status;
EventStatus channel_map_acquire_status;

AppState app_state;
static BootProgress boot_progress;
int streamer_up_flag = 1;
void update_boot_status(void);
static time_t stack_elapsed_time = 0;

static void _EventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);

static IARM_Result_t _SysModeChange(void *);

static bool EAS_mode_on = false;

int text_index;
int boot_index;
int text_status_index;
int boot_transition_times[BOOT_PROGRESS_LAST] = {0, 5, 5, 5, 1, 1};

//#define TEST_FP
#ifdef TEST_FP 

#endif
static int fp_Initialized = 0;
rmf_Error fp_setText(const char *fp_text);
rmf_Error fp_setText_Scroll(char *fp_text);

/*
The Global LED Brightness for power and clock 
*/
static int clockBrightness = 100;

static char vlg_szPreviousText[64] = "";

void vl_mpeos_fpSetSnmpText(const char * pszText)
{
	if(NULL != pszText)
    	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.FP", "%s :: text = %s \n", __FUNCTION__, pszText );
    if((NULL != pszText) && (0 != strcmp(vlg_szPreviousText, pszText)))
    {
        snprintf(vlg_szPreviousText,sizeof(vlg_szPreviousText),"%s",pszText);
        FILE *fp = NULL;
		fp = fopen("/tmp/.frontpaneltext", "w");
        if(NULL != fp)
        {
            fprintf(fp, "%s", vlg_szPreviousText);
            fclose(fp);
			fp = NULL;
        }
    }
}

bool vlIsCdlDeferredRebootEnabled();
bool vlIsCdlDisplayEnabled();

bool vlIsCdlDisplayEnabled()
{
    struct stat st;
    bool ret=true;

    if((0 == stat("/tmp/front_panel_update_disabled", &st) && (0 != st.st_ino)))
    {
        ret=false;
        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.FP", "KYW: front_panel_update_disabled, %d \n", ret );
    }
    return ret;
}

bool vlIsCdlDeferredRebootEnabled()
{
    struct stat st;
    bool ret=false;

    if((0 == stat("/tmp/cdl_deferred_reboot_enabled", &st) && (0 != st.st_ino)))
    {
        ret=true;
        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.FP", "KYW: /tmp/cdl_deferred_reboot_enabled exist, %d \n", ret );
    }
    return ret;
}

rmf_Error fp_init(void)
{
  if (! fp_Initialized)
  {
    rmf_Error rmf_error = 0;
    app_state = APP_STATE_BOOT;
    boot_progress = BOOT_PROGRESS_START;
    try {
//#ifdef USE_FPTEXT
        device::List <device::FrontPanelTextDisplay> supported_text_displays = device::FrontPanelConfig::getInstance().getTextDisplays();
        if(0 != supported_text_displays.size())
        {
            g_fp_supports_text = true;
            clockBrightness =  device::FrontPanelTextDisplay::getInstance("Text").getTextBrightness();
            device::FrontPanelTextDisplay::getInstance("Text").setTextBrightness(clockBrightness);	
            RDK_LOG( RDK_LOG_INFO, "LOG.RDK.FP","%s: front panel supports text display.\n",__FUNCTION__);
        }
        else
        {
            RDK_LOG( RDK_LOG_INFO, "LOG.RDK.FP","%s: front panel does not support text display.\n",__FUNCTION__);
        }

//#endif		
		IARM_Bus_RegisterCall(IARM_BUS_COMMON_API_SysModeChange, _SysModeChange);
    IARM_Bus_RegisterEventHandler(IARM_BUS_DSMGR_NAME,
                                    IARM_BUS_DSMGR_EVENT_TIME_FORMAT_CHANGE,
                                    _EventHandler);


    }
    catch (...) {
	printf("Exception Caught during [%s]\r\n", __FUNCTION__);
	return -1;
    }
    fp_setText("boot");
	vl_mpeos_fpSetSnmpText("boot"); 
    g_timeSet = 1;
    if (setupTime() != RMF_SUCCESS )
    {
      printf("%s : time setup failed \n",__FUNCTION__);
    }
    fp_Initialized = 1;
    return rmf_error;
  }
  else
    return RMF_SUCCESS;
}

rmf_Error fp_setText(const char *fp_text)
{
  rmf_Error rmf_error = 0;
//#ifdef USE_FPTEXT 
  try {
        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.FP","%s :: text = %s\n",__FUNCTION__,fp_text);
        if(g_fp_supports_text)
        {
            device::FrontPanelTextDisplay::Scroll s(0,0,0);
            device::FrontPanelConfig::getInstance().getTextDisplay("Text").setText(fp_text);
            device::FrontPanelConfig::getInstance().getTextDisplay("Text").setScroll(s);
            RDK_LOG( RDK_LOG_INFO, "LOG.RDK.FP","%s :: Set text %s with scroll 0\n",__FUNCTION__,fp_text);
        }
		//device::FrontPanelTextDisplay::getInstance("Text").setText(fp_text);
  }
  catch (...) {
     printf("Exception Caught during [%s]\r\n", __FUNCTION__);
     rmf_error = -1;
  }
//#endif
  return rmf_error;
}
rmf_Error fp_setText_Scroll(char *fp_text)
{
  rmf_Error rmf_error = 0;
//#ifdef USE_FPTEXT 
  try {
        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.FP","%s :: text = %s\n",__FUNCTION__,fp_text);
        if(g_fp_supports_text)
        {
            device::FrontPanelTextDisplay::Scroll scroll(40,10,0);
            device::FrontPanelConfig::getInstance().getTextDisplay("Text").setText(fp_text);
            device::FrontPanelConfig::getInstance().getTextDisplay("Text").setScroll(scroll);	
            RDK_LOG( RDK_LOG_INFO, "LOG.RDK.FP","%s :: Set text %s with scroll enabled\n",__FUNCTION__,fp_text);
        }
  }
  catch (...) {
      printf("Exception Caught during [%s]\r\n", __FUNCTION__);
      rmf_error = -1;
  }
//#endif
  return rmf_error;
}

rmf_Error fp_setTime(int hour, int min, int sec)
{
  rmf_Error rmf_error = 0;
  const char * tz = getenv("TZ");
  RDK_LOG( RDK_LOG_INFO, "LOG.RDK.FP", "%s ::Timezone is %s Time is %02d:%02d:%02d\n", __FUNCTION__,tz , hour , min , sec);
//#ifdef USE_FPTEXT 
  try {
    if((EAS_mode_on == false) && (g_fp_supports_text))
    {
        device::FrontPanelTextDisplay::getInstance("Text").setTime(hour,min);
    }
  }
  catch (...) {
    printf("Exception Caught during [%s]\r\n", __FUNCTION__);
    rmf_error = -1;
  }
//#endif
  return rmf_error;
}

void syncFPDTime(void * arg)
{
    uint32_t brightness = _MAX_BRIGHTNESS;
    
    struct tm stm;
    time_t timeval;
    struct timeval tv;
    bool stt_error_flag;
//USE_QAMSRC macro is not needed for this module. So removing usage of this macro
//This should prevent builds without USE_QAMSRC defined in Makefile to 
//wait for stt_received file creation before time updation in FP
//#ifdef USE_QAMSRC
    struct stat stt_buf;

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.FP", "%s ::Starts here\n", __FUNCTION__);
    while (1)
    {
      stt_error_flag = ((stat("/tmp/stt_received", &stt_buf) != 0));
      if (stt_error_flag)
      {
	sleep(1);
      }
      else 
	break;
    }
//#endif
	sleep(2);
    while(1)
    {
	  sleep(1);
      if (app_state == APP_STATE_DONE || app_state != APP_STATE_TEXT)
      {	
          time(&timeval);
          localtime_r(&timeval, &stm);
          gettimeofday(&tv, NULL);
          if(getenv("TZ") == NULL)
          {
              RDK_LOG( RDK_LOG_INFO, "LOG.RDK.FP", "%s ::Timezone is not set yet, waiting\n", __FUNCTION__);	  
              continue;
          }

          /*The clock brightness is set only on startup and whne USER changes it through DS Menu.*/
          //#ifdef USE_FPTEXT
          if(g_fp_supports_text)
          {
              try {
                  int timeFormat = device::FrontPanelConfig::getInstance().getTextDisplay("Text").getCurrentTimeFormat();
                  RDK_LOG( RDK_LOG_INFO, "LOG.RDK.FP", "%s ::Time Fromat  is %d \n", __FUNCTION__,timeFormat);
                  device::FrontPanelTextDisplay::getInstance("Text").setTimeFormat(timeFormat);
              }
              catch (...) {
                  RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.FP", "Exception Caught during Get/Set Time format [%s]\r\n", __FUNCTION__);
              }
          }
          RDK_LOG( RDK_LOG_INFO, "LOG.RDK.FP", "%s ::Timezone is %s Time is %02d:%02d:%02d\n", __FUNCTION__,getenv("TZ"), stm.tm_hour , stm.tm_min , stm.tm_sec);
          if(EAS_mode_on == false)
          {
              char fpTimeString[8];
              memset(fpTimeString,0,8);
              if(g_fp_supports_text)
              {
                  try {
                      device::FrontPanelConfig::getInstance().getTextDisplay("Text").setTime(stm.tm_hour,stm.tm_min);
                  }
                  catch (...) {
                      RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.FP", "Exception Caught during Set Time format [%s]\r\n", __FUNCTION__);
                  }
              }
              snprintf(fpTimeString,8,"%d:%d",stm.tm_hour,stm.tm_min);
              vl_mpeos_fpSetSnmpText(fpTimeString);
          }			
          bTimeUpdatedStarted = true;
          //vl_mpeos_fpSetSnmpText("Time"); 
          //#endif
          break;
      }	  
    }
    
    while(1)
    {
      sleep(1);
      if( gEnableFPUpdate == false )
      {
		continue;
      }

      time(&timeval);
      localtime_r(&timeval, &stm);
      gettimeofday(&tv, NULL);
      if(getenv("TZ") == NULL)
      {
            RDK_LOG( RDK_LOG_INFO, "LOG.RDK.FP", "%s ::Timezone is not set, waiting\n", __FUNCTION__);	  
            continue;
      }	  
      if(0 == (stm.tm_sec%60))
      {
	  if (app_state == APP_STATE_DONE || app_state != APP_STATE_TEXT)
	  {
			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.FP", "%s ::Timezone now %s Time is %02d:%02d:%02d\n", __FUNCTION__,getenv("TZ"), stm.tm_hour , stm.tm_min , stm.tm_sec);
            if(g_fp_supports_text)
            {
                try {
                    device::FrontPanelTextDisplay::Scroll s(0,0,0);
                    device::FrontPanelConfig::getInstance().getTextDisplay("Text").setScroll(s);
                    int timeFormat = device::FrontPanelConfig::getInstance().getTextDisplay("Text").getCurrentTimeFormat();
                    RDK_LOG( RDK_LOG_INFO, "LOG.RDK.FP", "%s ::Time Fromat  is %d \n", __FUNCTION__,timeFormat);
                    device::FrontPanelTextDisplay::getInstance("Text").setTimeFormat(timeFormat);
                }
                catch (...) {
                    RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.FP", "Exception Caught during Get/Set Time format(2) [%s]\r\n", __FUNCTION__);
                }
            }
            RDK_LOG( RDK_LOG_INFO, "LOG.RDK.FP", "%s ::Timezone = %s Time is %02d:%02d:%02d\n", __FUNCTION__,getenv("TZ"), stm.tm_hour , stm.tm_min , stm.tm_sec);
			if(EAS_mode_on == false)
            { 
                //device::FrontPanelTextDisplay::getInstance("Text").setTime(stm.tm_hour,stm.tm_min);
                char fpTimeString[8];
                memset(fpTimeString,0,8);
                if(g_fp_supports_text)
                {
                    try {
                        device::FrontPanelConfig::getInstance().getTextDisplay("Text").setTime(stm.tm_hour,stm.tm_min);
                    }
                    catch (...) {
                        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.FP", "Exception Caught during Set Time format [%s]\r\n", __FUNCTION__);
                    }
                }
                snprintf(fpTimeString,8,"%d:%d",stm.tm_hour,stm.tm_min);
                vl_mpeos_fpSetSnmpText(fpTimeString);
            }			
			bTimeUpdatedStarted = true;
			//vl_mpeos_fpSetSnmpText("Time"); 
//#endif
	  }
      }
    }
}

extern "C" unsigned long rmf_osal_GetUptime();

static void taskLogStbTimeBeatInSerialConsole(void * pParam)
{
    char szEventLog[256];
    time_t t = 0;
    struct tm gm, lt;
    time(&t);
    gmtime_r(&t, &gm);
    
    const char * pszSerialConsole = "/dev/ttyS0";
    FILE * fp = fopen(pszSerialConsole, "a");
    
    if(NULL == fp)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SNMP", "%s(): Unable to open '%s'.\n", __FUNCTION__, pszSerialConsole);
        return;
    }
    
    rmf_osal_threadSleep(5*60*1000, 0); // 5 minutes
    
    while(1)
    {
        unsigned int nUpTime  = rmf_osal_GetUptime();
        
        // create current event
        time(&t);
        gmtime_r(&t, &gm);
        localtime_r(&t, &lt);
        fprintf(fp, "TIME-LOG: UTC:%02u%02u%02u-%02u:%02u:%02u, LT:%02u%02u%02u-%02u:%02u:%02u, UP:%02u:%02u:%02u.\n",
        (gm.tm_year%100), gm.tm_mon+1, gm.tm_mday, gm.tm_hour, gm.tm_min, gm.tm_sec,
        (lt.tm_year%100), lt.tm_mon+1, lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec,
        (nUpTime/3600), ((nUpTime%3600)/60), (nUpTime%60));
        rmf_osal_threadSleep(10*60*1000, 0); // 10 minutes
    }
}

rmf_Error setupTime()
{
    rmf_osal_ThreadId g_syncFPDTime;
     rmf_Error rmf_error = RMF_SUCCESS;
    if(g_timeSet)
    {
        static int bStartedFpdSyncThread = 0;
        if(!bStartedFpdSyncThread)
        {
	    bStartedFpdSyncThread = 1;
	    rmf_error = rmf_osal_threadCreate(syncFPDTime, NULL, RMF_OSAL_THREAD_PRIOR_DFLT, RMF_OSAL_THREAD_STACK_SIZE, &g_syncFPDTime, "syncFPDTime");
	    if ( RMF_SUCCESS != rmf_error )
	    {
	      g_print( "%s - rmf_osal_threadCreate failed.\n", __FUNCTION__);
	    }
        }
    }
    static rmf_osal_ThreadId threadId = 0;
    if(0 == threadId)
    {
        rmf_osal_threadCreate(taskLogStbTimeBeatInSerialConsole, (void *) (NULL), RMF_OSAL_THREAD_PRIOR_DFLT, RMF_OSAL_THREAD_STACK_SIZE, &threadId, "taskLogStbTimeBeatInSerialConsole");
    }
    return rmf_error;
}
rmf_Error fp_setupBootSeq(void)
{
  rmf_Error rmf_error = 0;
#ifdef TEST_FP    
  uint8_t threadCounts = 3;
#else
  uint8_t threadCounts = 2;
#endif

  app_state = APP_STATE_TEXT;
  threadSync =  new sem_t;
  if (NULL == threadSync)
  {
   	RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.FP", "Failed to allocate sem in %s\n", __FUNCTION__ );
   	return RMF_OSAL_ENOMEM;
  }  

    rmf_error  = sem_init( threadSync, 0 , 0);
    if (0 != rmf_error )
    {
        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.FP", "%s - sem_init failed.\n", __FUNCTION__);
        return rmf_error;
    }
	IARM_Bus_RegisterEventHandler(IARM_BUS_SYSMGR_NAME, IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, _RDKEventHandler);  
//USE_QAMSRC macro is not needed for this module. So removing usage of this macro 
//#ifdef USE_QAMSRC
  /* to ensure synchronization with podmgr init */
  create_pod_event_manager();

    rmf_error = rmf_osal_threadCreate(handle_time_limited_event_driven_boot_thread, 
		NULL, RMF_OSAL_THREAD_PRIOR_DFLT, RMF_OSAL_THREAD_STACK_SIZE, &g_stackMonThreadId, 
		"handle_time_limited_event_driven_boot_thread");  
    if ( RMF_SUCCESS != rmf_error )
    {
	g_print( "handle_time_limited_event_driven_boot_thread - rmf_osal_threadCreate failed.\n");
    }

    rmf_error = rmf_osal_threadCreate(cdlThread, NULL, RMF_OSAL_THREAD_PRIOR_DFLT, RMF_OSAL_THREAD_STACK_SIZE, &g_cdlThreadId, "cdThread");
    if ( RMF_SUCCESS != rmf_error )
    {
	g_print( "cdThread - rmf_osal_threadCreate failed.\n");
    }
//#endif
  #ifdef TEST_FP  
    rmf_error = rmf_osal_threadCreate(cdlEventThread, NULL, RMF_OSAL_THREAD_PRIOR_DFLT, RMF_OSAL_THREAD_STACK_SIZE, &g_cd2ThreadId, "cdlEventThread");
    if ( RMF_SUCCESS != rmf_error )
    {
	g_print( "cdThread - rmf_osal_threadCreate failed.\n");
    }
  #endif

    for(; threadCounts--; )
   {
        rmf_error = sem_wait( threadSync);
        if (0 != rmf_error )
        {
             RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.FP", "%s - sem_wait failed.\n", __FUNCTION__);
        }
        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.FP", "%s - Got %d thread up .\n", __FUNCTION__, threadCounts );		 	
   }
	
  return rmf_error;

}
//cdl thread
void cdlThread (void *arg)
{
    rmf_osal_eventqueue_handle_t m_pEvent_queue;
    //pfcEventBase *event=NULL;
    rmf_Error ret;
    int result = 0;
    rmf_osal_event_handle_t event_handle;
    rmf_osal_event_params_t event_params = {0};
    rmf_osal_event_category_t event_category;
    uint32_t event_type;
    uint32_t data;
    char fp_text[8] = "";
    static bool boot_cp_started = false;
    rmf_osal_eventmanager_handle_t eventmanager_handle =  get_pod_event_manager();
    bool show_fp=true;

  ret = rmf_osal_eventqueue_create( (const uint8_t*)"CDL_MsgHandler", &m_pEvent_queue);
  if (ret != RMF_SUCCESS )
  {
    printf ("%s : rmf_osal_eventqueue_create failed \n",__FUNCTION__);
  }

    rmf_osal_eventmanager_register_handler(eventmanager_handle, m_pEvent_queue, RMF_OSAL_EVENT_CATEGORY_BOOTMSG_DIAG);
    printf ("%s : pod event handler is based on internal event manager \n",__FUNCTION__);

   ret = sem_post( threadSync);
   if (0 != ret )
   {
       RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.FP", "%s - sem_post failed.\n", __FUNCTION__);
   }

  while (1)
  {
    //printf("\n cdlThread ::: Start 1\n");
    if(m_pEvent_queue == NULL)
    {
	printf("\n FATAL FATAL: npEvent-queue is NULL  \n");
	break;
    }
    ret = rmf_osal_eventqueue_get_next_event(m_pEvent_queue, &event_handle, &event_category, &event_type, &event_params);
    if (ret != RMF_SUCCESS )
    {
      printf ("%s : rmf_osal_eventqueue_get_next_event failed \n",__FUNCTION__);
    }
    
    //printf("\n rmf_osal_eventqueue_get_next_event() :: event_params %0x \n", event_params);
    //printf("cdlhandler   :run:event=0x%x,category=%d;type=%d\n",event_params, event_category, event_type);
	if (!vlIsCdlDisplayEnabled())
	{
       RDK_LOG( RDK_LOG_INFO, "LOG.RDK.FP", "%s: vlIsCdlDisplayEnabled is false \n", __FUNCTION__ );
	   if (event_type == RMF_BOOTMSG_DIAG_EVENT_TYPE_CDL || event_type == RMF_BOOTMSG_DIAG_EVENT_TYPE_FLASH_WRITE)
       {
           RDK_LOG( RDK_LOG_INFO, "LOG.RDK.FP", "%s: event_type matches with CDL or FLASH_WRITE \n", __FUNCTION__ );
           // make sure displays time
           if(g_fp_supports_text)
           {
               try {
                   device::FrontPanelTextDisplay::getInstance("Text").enableDisplay(1);
               }
               catch (...) {
                   RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.FP", "Exception Caught during [%s]\r\n", __FUNCTION__);
               }
           }
           show_fp = false;
           RDK_LOG( RDK_LOG_INFO, "LOG.RDK.FP", "%s: Deleting the event, setting SnmpText and continuing \n", __FUNCTION__ );
           rmf_osal_event_delete(event_handle);
           vl_mpeos_fpSetSnmpText("Time"); 
           continue;
       }
	}

#ifdef TEST_WITH_PODMGR
	data = (unsigned long) event_params.data;
	show_fp= true;
        printf("cdlhandler   :run:category=%d;type=%x;data_extension=%d\n",event_category, event_type, event_params.data_extension);
	switch (event_type)
	{
	  case RMF_BOOTMSG_DIAG_EVENT_TYPE_CDL :
	  {
	    show_fp = false;
	    switch (  (RMF_BOOTMSG_DIAG_MESG_CODE) event_params.data_extension )
	    {
	      case RMF_BOOTMSG_DIAG_MESG_CODE_START:
	      case RMF_BOOTMSG_DIAG_MESG_CODE_PROGRESS:
		if (app_state == APP_STATE_DONE || app_state != APP_STATE_TEXT)
		{
		  snprintf(fp_text, 8, "dL%02u", data);
      RDK_LOG( RDK_LOG_INFO, "LOG.RDK.FP", "vl_mpeos_fpSetText :: text = %s \n",fp_text );
	  printf("cdlhandler   : vl_mpeos_fpSetText :: text = %s \n",fp_text );

//#ifdef USE_FPTEXT
		fp_setText(fp_text);
		vl_mpeos_fpSetSnmpText(fp_text); 
//#endif
		}
		break;
	      case RMF_BOOTMSG_DIAG_MESG_CODE_COMPLETE:
		if (app_state == APP_STATE_DONE || app_state != APP_STATE_TEXT)
		{
		  snprintf(fp_text, 8, "dLdn");
      RDK_LOG( RDK_LOG_INFO, "LOG.RDK.FP", "vl_mpeos_fpSetText :: text = %s \n",fp_text );
//#ifdef USE_FPTEXT
		fp_setText(fp_text);
		vl_mpeos_fpSetSnmpText(fp_text); 
//#endif
		}
		//If deferred reboot,it will display time now onwards.Otherwise box will reboot
		enableFPUpdates(true);
		break;

	      case RMF_BOOTMSG_DIAG_MESG_CODE_ERROR:
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FP","%s: Firmware download failed\n",__FUNCTION__);
		if (app_state == APP_STATE_DONE || app_state != APP_STATE_TEXT)
		{
		  snprintf(fp_text, 8, "Ed%02d", data);
		   RDK_LOG( RDK_LOG_INFO, "LOG.RDK.FP", "vl_mpeos_fpSetText :: text = %s \n",fp_text );
//#ifdef USE_FPTEXT
		    fp_setText(fp_text);
		vl_mpeos_fpSetSnmpText(fp_text); 
//#endif
		  break;
		}
		//If deferred reboot,it will display time now onwards.Otherwise box will reboot		
		enableFPUpdates(true);

		  default:
	      break;
	    }
	    break;
	  }
	  case RMF_BOOTMSG_DIAG_EVENT_TYPE_FLASH_WRITE :
	  {
	    switch ((RMF_BOOTMSG_DIAG_MESG_CODE) event_params.data_extension)
	    {
		case RMF_BOOTMSG_DIAG_MESG_CODE_START:
		    snprintf(fp_text, 8, "fLst");
			show_fp = false;
        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.FP", "vl_mpeos_fpSetText :: text = %s \n",fp_text );

//#ifdef USE_FPTEXT
		     fp_setText(fp_text);
		vl_mpeos_fpSetSnmpText(fp_text); 
//#endif
		break;

		case RMF_BOOTMSG_DIAG_MESG_CODE_PROGRESS:
		    snprintf(fp_text, 8, "fLdn");
		    show_fp = false;
        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.FP", "vl_mpeos_fpSetText :: text = %s \n",fp_text );
		    if (vlIsCdlDeferredRebootEnabled())
		    {
   		    	// make sure displays time
                if(g_fp_supports_text)
                {
                    try {
                        device::FrontPanelTextDisplay::getInstance("Text").enableDisplay(1);
                    }
                    catch (...) {
                        printf("Exception Caught during [%s]\r\n", __FUNCTION__);
                    }
                }
		    	show_fp = false;
    		    //rmf_osal_event_delete(event_handle);
		    	vl_mpeos_fpSetSnmpText("Time"); 
		    	//continue;
		    }
		    else
		    {
//#ifdef USE_FPTEXT
		      fp_setText(fp_text);
		vl_mpeos_fpSetSnmpText(fp_text); 
//#endif
		     }
		break;

		case RMF_BOOTMSG_DIAG_MESG_CODE_COMPLETE:
		    snprintf(fp_text, 8, "rBOOT");
        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.FP", "vl_mpeos_fpSetText :: text = %s \n",fp_text );
//#ifdef USE_FPTEXT
		         fp_setText(fp_text);
		vl_mpeos_fpSetSnmpText(fp_text); 
//#endif
		    sleep (2);
		//If deferred reboot,it will display time now onwards.Otherwise box will reboot
		enableFPUpdates(true);

		break;
		case RMF_BOOTMSG_DIAG_MESG_CODE_ERROR:
		    show_fp = false;			
		    snprintf(fp_text, 8, "fLEr");
        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.FP", "vl_mpeos_fpSetText :: text = %s \n",fp_text );
//#ifdef USE_FPTEXT
		         fp_setText(fp_text);
		vl_mpeos_fpSetSnmpText(fp_text); 
//#endif
		//If deferred reboot,it will display time now onwards.Otherwise box will reboot
		enableFPUpdates(true);

		break;		
	    }
	    break;
	  }
	  case RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD_CA:
	  {
	    switch( (RMF_BOOTMSG_DIAG_MESG_CODE) event_params.data_extension)
	    {
		case RMF_BOOTMSG_DIAG_MESG_CODE_COMPLETE:
		    card_host_init_status = EVENT_STATUS_START;
		    break;

		default:
		    break;
	    }
	  }
	  break;
	  
	  case RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD_CP:
	  {
	    switch( (RMF_BOOTMSG_DIAG_MESG_CODE) event_params.data_extension)
	    {
		case RMF_BOOTMSG_DIAG_MESG_CODE_INIT:
		case RMF_BOOTMSG_DIAG_MESG_CODE_START:
		    card_host_init_status = EVENT_STATUS_COMPLETE;
		    boot_cp_started       = true;
		    //Vprintf("----- @file:%s.. %s : %d --------\n",__FILE__,__FUNCTION__,__LINE__);
		    break;

		case RMF_BOOTMSG_DIAG_MESG_CODE_ERROR:
		    if (boot_cp_started)
		    {
			if (((data >= 0x01) && (data < 0x06)) ||
			    (data == 0x08))
			{
			    card_host_init_status = EVENT_STATUS_FAIL;
			    //Vprintf("----- @file:%s.. %s : %d --------\n",__FILE__,__FUNCTION__,__LINE__);
			}
			else if (data == 0x07)
			{
			}
			else if (data == 0x00)
			{
			    card_host_init_status = EVENT_STATUS_COMPLETE;
			    //Vprintf("----- @file:%s.. %s : %d --------\n",__FILE__,__FUNCTION__,__LINE__);
			}
		    }
		    break;

		case RMF_BOOTMSG_DIAG_MESG_CODE_COMPLETE:
		    if (boot_cp_started)
		    {
			card_host_init_status = EVENT_STATUS_COMPLETE;
			//Vprintf("----- @file:%s.. %s : %d --------\n",__FILE__,__FUNCTION__,__LINE__);
		    }
		    break;

		default:
		    break;
	      }
	      break;
	  }

	  case RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD_DSG_ONEWAY:
	  case RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD_DSG_TWOWAY:
	  case RMF_BOOTMSG_DIAG_EVENT_TYPE_CARD_OOB:
	    switch((RMF_BOOTMSG_DIAG_MESG_CODE) event_params.data_extension)
	    {
		case RMF_BOOTMSG_DIAG_MESG_CODE_INIT:
		    dsg_acquire_status = EVENT_STATUS_START;
		    //Vprintf("----- @file:%s.. %s : %d --------\n",__FILE__,__FUNCTION__,__LINE__);
		    break;

		case RMF_BOOTMSG_DIAG_MESG_CODE_START:
		    dsg_acquire_status = EVENT_STATUS_COMPLETE;
		    docsis_reg_status  = EVENT_STATUS_START;
		    //Vprintf("----- @file:%s.. %s : %d --------\n",__FILE__,__FUNCTION__,__LINE__);
		    break;

		case RMF_BOOTMSG_DIAG_MESG_CODE_COMPLETE:
		    dsg_acquire_status = EVENT_STATUS_COMPLETE; // Mar-10-2015: Fix for invalid error determination.
		    docsis_reg_status = EVENT_STATUS_COMPLETE;
		    printf("----- @file:%s.. %s : %d -------- DOCSIS Acquistion Complete\n",__FILE__,__FUNCTION__,__LINE__);
		    break;

		case RMF_BOOTMSG_DIAG_MESG_CODE_ERROR:
		    dsg_acquire_status = EVENT_STATUS_FAIL;
		    //Vprintf("----- @file:%s.. %s : %d --------\n",__FILE__,__FUNCTION__,__LINE__);
		    break;

		default:
		    break;
	    }
	    break;

	  case RMF_BOOTMSG_DIAG_EVENT_TYPE_ESTB_IP:
      printf("cdlhandler   :run:category=%d;type=%x,RMF_BOOTMSG_DIAG_EVENT_TYPE_ESTB_IP;data_extension=%d\n",event_category, event_type, event_params.data_extension);
	    switch((RMF_BOOTMSG_DIAG_MESG_CODE) event_params.data_extension)
	    {
	      case RMF_BOOTMSG_DIAG_MESG_CODE_INIT:
	      case RMF_BOOTMSG_DIAG_MESG_CODE_START:
		ip_acquire_status = EVENT_STATUS_START;
                printf("cdlhandler   :run:category=%d;type=%x,RMF_BOOTMSG_DIAG_EVENT_TYPE_ESTB_IP;data_extension=%d, setting ip_acquire_status to EVENT_STATUS_START\n",event_category, event_type, event_params.data_extension);
          if(false == getBootStatus())
          {
                // Fix for missing iPst string.
                const char *_text = "iPst"; // There seems to be a mismatch between fp_messages[] and enumeration BOOT_PROGRESS_GET_IP
                                            // due to insertion of BOOT_PROGRESS_GET_DOCIS_IP and absence of BOOT_PROGRESS_SI, so hard-code for now.
                fp_setText(_text);
                vl_mpeos_fpSetSnmpText(_text); 	
                fp_setText( (char *) _text );
          }
		break;

	      case RMF_BOOTMSG_DIAG_MESG_CODE_COMPLETE:
                printf("cdlhandler   :run:category=%d;type=%x,RMF_BOOTMSG_DIAG_EVENT_TYPE_ESTB_IP;data_extension=%d, setting ip_acquire_status to EVENT_STATUS_COMPLETE\n",event_category, event_type, event_params.data_extension);
		ip_acquire_status = EVENT_STATUS_COMPLETE;
		break;

	      case RMF_BOOTMSG_DIAG_MESG_CODE_ERROR:
                printf("cdlhandler   :run:category=%d;type=%x,RMF_BOOTMSG_DIAG_EVENT_TYPE_ESTB_IP;data_extension=%d, setting ip_acquire_status to EVENT_STATUS_FAIL\n",event_category, event_type, event_params.data_extension);
		ip_acquire_status = EVENT_STATUS_FAIL;
		break;

	      default:
		break;
	    }
	    break;

	case RMF_BOOTMSG_DIAG_EVENT_TYPE_SI:
	  switch((RMF_BOOTMSG_DIAG_MESG_CODE) event_params.data_extension)
	  {
	      case RMF_BOOTMSG_DIAG_MESG_CODE_INIT:
	      case RMF_BOOTMSG_DIAG_MESG_CODE_START:
		  channel_map_acquire_status = EVENT_STATUS_START;
		  break;

	      case RMF_BOOTMSG_DIAG_MESG_CODE_COMPLETE:
		  channel_map_acquire_status = EVENT_STATUS_COMPLETE;
		  break;

	      case RMF_BOOTMSG_DIAG_MESG_CODE_ERROR:
		  channel_map_acquire_status = EVENT_STATUS_FAIL;
		  break;

	      default:
		  break;
	    }
	    break;

                case RMF_BOOTMSG_DIAG_EVENT_TYPE_OCAP_XAIT:
                    break;

	  case RMF_BOOTMSG_DIAG_EVENT_TYPE_INIT:
	  	printf("Got Init Event to frontpanel from POD\n");
		vl_mpeos_fpSetSnmpText( VL_INIT_TEXT );
		fp_setText( VL_INIT_TEXT );		
		isInit = true;
		printf("Printed Init in Frontpanel\n");
	    break;
	  case RMF_BOOTMSG_DIAG_EVENT_TYPE_COLD_INIT:
	  	{
		    printf("Got Cold Init Event to frontpanel from POD\n");
		    vl_mpeos_fpSetSnmpText( VL_COLD_INIT_TEXT);
		    //Set scrolling for cold.init. 
		    fp_setText_Scroll( VL_COLD_INIT_TEXT);
		    isInit = true;
		    show_fp= false;
		    printf("Printed Cold Init in Frontpanel\n");
	  	}
	    break;

	  case RMF_BOOTMSG_DIAG_EVENT_TYPE_FP_STATUS:
		if ( RMF_BOOTMSG_DIAG_MESG_CODE_FP_TIME_ENABLE == event_params.data_extension )
		{
		  	printf("FP time update is enabled \n");
			gEnableFPUpdate = true;

		}
		else
		{
		  	printf("FP time update is disabled \n");
			gEnableFPUpdate = false;
		}
		show_fp= false;
	    break;

	  default:
	  	show_fp= false;
	    break;
	} //end of switch (event_type)
	//If boot complete,suppress further boot messages
	if (show_fp) {
	  update_boot_status();
	  const char *_text = fp_messages[boot_progress];
	  if(BOOT_PROGRESS_DONE == boot_progress)
	  {
		setBootStatus(true);
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.FP", "Got boot progress Done.Disabling Done in FP\n");
	  }
	  if(false == getBootStatus())
	  {
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.FP", "vl_mpeos_fpSetText :: text = %s \n",_text );
		fp_setText(_text);
		vl_mpeos_fpSetSnmpText(_text); 		
		fp_setText( (char *) _text );
		if(APP_STATE_DONE == app_state)
		{
			setBootStatus(true);
		}
	  }
	  else
	  {
		//Boot already over.no need to display another fp boot sequence message
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.FP", "vl_mpeos_fpSetText :: text = %s Rejected\n",_text );
	  }

	}
#endif // End : TEST_WITH_PODMGR
    rmf_osal_event_delete(event_handle);
  } //end of while (1)

  printf("\n cdlThread ::: Stop 1\n");
} //end of function
bool getBootStatus()
{
	return isBootComplete;
}
void setBootStatus(bool status)
{
	isBootComplete = status;
	return;
}
void update_boot_status()
{
    BootProgress new_progress;

    static time_t prev_time          = 0;
    static time_t prev_progress_time = 0;
    static time_t prev_text_time     = 0;
    FILE *fp;
    time_t current_time;
    char file[50];
    time(&current_time);


    if (prev_progress_time == 0)
    {
        prev_progress_time = current_time;
        prev_text_time     = current_time;
    }

    if (current_time != prev_time)
    {
        prev_time = current_time;
   
        switch (app_state)
        {
            case APP_STATE_IMAGE:
            case APP_STATE_TEXT:
              new_progress = BOOT_PROGRESS_START;
              if (boot_progress >= BOOT_PROGRESS_DONE)
              {
                  app_state = APP_STATE_DONE;
              }

              if (ip_acquire_status == EVENT_STATUS_COMPLETE)
              {
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.FP", "progress is BOOT_PROGRESS_DONE [%d] \n",boot_index );
                  new_progress = BOOT_PROGRESS_DONE;
                  app_state = APP_STATE_DONE;
		    fp = fopen("/tmp/stage4","w");
		    if(fp)
		      fclose(fp);
              }
              else if ((docsis_reg_status == EVENT_STATUS_COMPLETE) ||
                        (ip_acquire_status != EVENT_STATUS_NOT_START))
              {
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.FP", "progress is BOOT_PROGRESS_GET_IP [%d] \n",boot_index );
                  new_progress = BOOT_PROGRESS_GET_IP;
		    fp = fopen("/tmp/stage3","w");
		    if(fp)
		      fclose(fp);
              }
              else if ((card_host_init_status == EVENT_STATUS_COMPLETE) ||
                        (dsg_acquire_status    == EVENT_STATUS_COMPLETE) ||
                        (docsis_reg_status     != EVENT_STATUS_NOT_START))
              {
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.FP", "progress is BOOT_PROGRESS_GET_DOCIS_IP [%d]\n",boot_index );
                  new_progress = BOOT_PROGRESS_GET_DOCIS_IP;
		    fp = fopen("/tmp/stage2","w");
		    if(fp)
		      fclose(fp);
              }
              else if ((card_host_init_status == EVENT_STATUS_START) ||
                        (dsg_acquire_status    == EVENT_STATUS_START))
              {
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.FP", "progress is BOOT_PROGRESS_GET_CARDHOST_DSG [%d]\n", boot_index );
                  new_progress = BOOT_PROGRESS_GET_CARDHOST_DSG;
		    fp = fopen("/tmp/stage1","w");
		    if(fp)
		      fclose(fp);
              }
              if (boot_progress < new_progress)
              {
                if (boot_index < BOOT_PROGRESS_DONE)
                    boot_index++;
                  boot_progress      = new_progress;//(BootProgress) (boot_progress + 1);
                  RDK_LOG( RDK_LOG_INFO, "LOG.RDK.FP", "progress is %d [%d]\n",boot_progress, boot_index );
                  prev_progress_time = current_time;
                  prev_text_time     = current_time;
                  text_index         = 0;
                  if (boot_progress == BOOT_PROGRESS_DONE)
                  {
                        app_state = APP_STATE_DONE;
                  }
                  RDK_LOG( RDK_LOG_INFO, "LOG.RDK.FP", "creating /tmp/stage%d \n",boot_index );
                  snprintf(file, sizeof(file),"touch /tmp/stage%d",boot_index);
                  system(file);
#ifdef GLI				
		IARM_Bus_SYSMgr_EventData_t eventData;
		eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_BOOTUP; 
		eventData.data.systemStates.state = boot_index; 
		eventData.data.systemStates.error = 0; 
		IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
#endif				
              }
              text_status_index = current_time - prev_text_time;
              if (text_status_index > TEXT_DELAY)
              {
                  prev_text_time    = current_time;
                  text_status_index = 0;

                  if (text_index < (BOOT_PROGRESS_TEXT_COUNT - 1))
                  {
                      text_index++;
                  }
              }

              if(top_labels[boot_index][text_index] != NULL)
              {
                  snprintf(file, sizeof(file),"echo %s > /tmp/.top_labels",top_labels[boot_index][text_index]);
                  system(file);
              }
              if(mid_labels[boot_index][text_index] != NULL)
              {
                  snprintf(file, sizeof(file),"echo %s > /tmp/.mid_labels",mid_labels[boot_index][text_index]);
                  system(file);
              }
              if(lower_labels[boot_index][text_index] != NULL)
              {
                  snprintf( file, sizeof(file), "echo %s > /tmp/.lower_labels",lower_labels[boot_index][text_index]);
                  system(file);
              }
              break;

            default:
              break;
        }
    }
}

/* Four time limits for event-driven error conditions. Make them all different, in increasing order 
   to match their reversed order of display priority,  and all less than
   value of STT_DOWN_MSG_DELAY. This is to make possible that if multiple error conditions are in effect 
   then each code will be displayed as it takes effect, before being displaced by error code of 
   higher display priority. */
static int BOOT_ERROR_ESTB_IP_MSG_DELAY = 330; 
static int BOOT_ERROR_ECM_OPERATIONAL_MSG_DELAY = 270; 
static int BOOT_ERROR_CARD_DSG_MSG_DELAY = 160; 
static int BOOT_ERROR_CARD_CP_MSG_DELAY = 60; 
static int STACK_DOWN_MAX_TIME_INTERVAL = 1; 

/**
 * Periodic updates to event-driven error conditions
 * that can be raised while Proxy has not yet come up.
 */
void handle_time_limited_event_driven_boot_thread( void *arg )
{
	time_t current_time = 0;
	BootErrorCode prevBroadcastedBootError = BOOT_ERROR_NONE;
    int nEcmFailureCount = 0; 

	time( &start_time_point );
       rmf_Error ret = sem_post( threadSync);
	if (0 != ret )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.FP", "%s - sem_post failed.\n", __FUNCTION__);
	}
	m_pSiCache = rmf_OobSiCache::getSiCache();
	while( true )
	{
		/* 
		**	We are comparing stack_elapsed_time with timeout of 270 seconds. 
		**	Hence we are waiting for 270 sec*5 sec i.e. 1350 seconds ~=22 minutes.
		**	Decreased the sleep from 5 seconds to 1 second.
		*/
		const char *offPlantEnabled = NULL;

		sleep( 1 );
		time ( &current_time );
		int time_diff = current_time - (start_time_point + stack_elapsed_time);

		RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.FP",
			"%s: Proxy down elapsed time is %d seconds\n", __FUNCTION__, stack_elapsed_time);

		if ( ( offPlantEnabled = (char*)rmf_osal_envGet( "ENABLE.OFFPLANT.SETUP") ) != NULL ) 
		{				
			if ( 0 == strcasecmp( offPlantEnabled, "TRUE" ) )
			{
				RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD", 
					"%s - Skiping IARM error events to fake off plant setup \n",
					__FUNCTION__ );
				continue;
			}
		}

		if (abs (time_diff) < STACK_DOWN_MAX_TIME_INTERVAL)
		{
			stack_elapsed_time += abs(time_diff);
		}
		else
		{
			stack_elapsed_time += STACK_DOWN_MAX_TIME_INTERVAL;
		}

		RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.FP",
			"%s: enter, Proxy elapsed time is %d seconds\n", 
			__FUNCTION__, stack_elapsed_time);
		if ( false == siSTTReceived )
		{
			m_pSiCache->LockForRead();
			if ( TRUE == m_pSiCache->getSTTAcquiredStatus() )
			{
				siSTTReceived = true;
				bootErrorTable[ BOOT_ERROR_STT ] = false;
			}
			m_pSiCache->UnLock();
		}
		if (stack_elapsed_time > BOOT_ERROR_ECM_OPERATIONAL_MSG_DELAY &&
			(!podmgrDsgEcmIsOperational()) )
		{
            nEcmFailureCount++; // buffer transient failures
			if (!bootErrorTable[BOOT_ERROR_ECM_OPERATIONAL] && nEcmFailureCount > 2) //check if the failure is persisting
			{
				RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FP",
					"%s: %d, %s---- ECM Operational / Sign-on Failure \n", 
					__FUNCTION__, stack_elapsed_time, 
					bootErrorMessage[BOOT_ERROR_ECM_OPERATIONAL]);
				bootErrorTable[BOOT_ERROR_ECM_OPERATIONAL]=true;

			}
		}
        else
        {
            nEcmFailureCount = 0; 
            if(bootErrorTable[BOOT_ERROR_ECM_OPERATIONAL])
            {   // in errored state, change to un-errored state.
                bootErrorTable[BOOT_ERROR_ECM_OPERATIONAL]=false; // Register recovery.
                #ifdef GLI
                    // Post recovered event.
                    IARM_Bus_SYSMgr_EventData_t eventData;
                    eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_ECM_IP;
                    eventData.data.systemStates.state = 2; // Recovered.
                    eventData.data.systemStates.error = 0; // Recovered.
                    IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
                #endif			
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.FP", "%s: eCM has recovered from failure.\n", __FUNCTION__);
            }
        }
		
		if ( stack_elapsed_time > BOOT_ERROR_ESTB_IP_MSG_DELAY &&
			( ip_acquire_status != EVENT_STATUS_COMPLETE) )
		{
			if (!bootErrorTable[ BOOT_ERROR_ESTB_IP ] )
			{
				RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FP",
				"%s: %d, %s---- STB DOCSIS/DHCP IP Acquisition Failure \n", 
				__FUNCTION__, stack_elapsed_time, bootErrorMessage[BOOT_ERROR_ESTB_IP]);
				bootErrorTable[BOOT_ERROR_ESTB_IP]=true;
			}
		}
        else
        {
            if(bootErrorTable[BOOT_ERROR_ESTB_IP])
            {   // in errored state, change to un-errored state.
                bootErrorTable[BOOT_ERROR_ESTB_IP]=false;
                #ifdef GLI
                    IARM_Bus_SYSMgr_EventData_t eventData;
                    eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_ESTB_IP;
                    eventData.data.systemStates.state = 2; // Recovered.
                    eventData.data.systemStates.error = 0; // Recovered.
                    IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
                #endif			
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.FP", "%s: eSTB IP has recovered from failure.\n", __FUNCTION__);
            }
        }

		if (stack_elapsed_time > BOOT_ERROR_CARD_DSG_MSG_DELAY &&
			(dsg_acquire_status != EVENT_STATUS_COMPLETE))
		{
			if (!bootErrorTable[BOOT_ERROR_CARD_DSG])
			{
				RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FP",
				"%s:  %d, %s----DOCSIS Acquisition Failure \n", 
				__FUNCTION__, stack_elapsed_time, bootErrorMessage[BOOT_ERROR_CARD_DSG] );
				bootErrorTable[BOOT_ERROR_CARD_DSG]=true;
			}
		}
        else
        {
            if(bootErrorTable[BOOT_ERROR_CARD_DSG])
            {   // in errored state, change to un-errored state.
                bootErrorTable[BOOT_ERROR_CARD_DSG]=false;
                #ifdef GLI
                    IARM_Bus_SYSMgr_EventData_t eventData;
                    eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_DSG_CA_TUNNEL;
                    eventData.data.systemStates.state = 2; // Recovered.
                    eventData.data.systemStates.error = 0; // Recovered.
                    IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
                #endif			
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.FP", "%s: DSG has recovered from failure.\n", __FUNCTION__);
            }
        }
        
		/*SAM150-2832 : Commented as this if statement is still undeveloped. (The && part seems to be missing when compared with the if statements above.)
		if (stack_elapsed_time > PROXY_DOWN_MSG_DELAY)
		{
			if (!bootErrorTable[BOOT_ERROR_PROXY_DOWN_TOO_LONG])
			{
				RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FP",
				"%s:  %d, %s----Proxy application or CMAC application doesn't load \n",__FUNCTION__,\
				stack_elapsed_time, bootErrorMessage[BOOT_ERROR_PROXY_DOWN_TOO_LONG] );
				bootErrorTable[BOOT_ERROR_PROXY_DOWN_TOO_LONG]=true;
			}
		}*/

		if (stack_elapsed_time > BOOT_ERROR_CARD_CP_MSG_DELAY)
		{
			uint8_t card_inserted = GetPodInsertedStatus();
			if (!card_inserted && !HomingCardFirmwareUpgradeGetState())
			{
				if (!bootErrorTable[BOOT_ERROR_CARD_CP])
				{
					RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FP",
					"%s:  %d, %s----Card Initialization Failure (NOT PRESENT).\n",
					__FUNCTION__, stack_elapsed_time, bootErrorMessage[BOOT_ERROR_CARD_CP] );
					bootErrorTable[BOOT_ERROR_CARD_CP]=true;
				}
			}
			else
			{
				bool card_not_in_error = vlIsDsgOrIpduMode();
				if (card_not_in_error)
				{
					RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.FP",
					"%s: Card is inserted, so not setting flag for card bootup error\n",
					__FUNCTION__);
                    if(bootErrorTable[BOOT_ERROR_CARD_CP])
                    {   // in errored state, change to un-errored state.
                        bootErrorTable[BOOT_ERROR_CARD_CP]=false;
                        #ifdef GLI
                            IARM_Bus_SYSMgr_EventData_t eventData;
                            eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_CABLE_CARD;
                            eventData.data.systemStates.state = 1; // Recovered.
                            eventData.data.systemStates.error = 0; // Recovered.
                            IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
                        #endif			
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.FP", "%s: Cable-card has recovered from failure.\n", __FUNCTION__);
                    }
				}
				else if(!HomingCardFirmwareUpgradeGetState())
				{
					if (!bootErrorTable[BOOT_ERROR_CARD_CP] )
					{
						RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FP",
						"%s:  %d, %s----Card Initialization Failure (NOT DSG).\n",
						__FUNCTION__, stack_elapsed_time, bootErrorMessage[BOOT_ERROR_CARD_CP] );

						bootErrorTable[BOOT_ERROR_CARD_CP]=true;
					}
				}
			}
		}
		if ( stack_elapsed_time > STT_DOWN_MSG_DELAY &&( siSTTReceived!= true) )
		{
			bootErrorTable[ BOOT_ERROR_STT] = true;
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FP",
						"%s: %d, %s---- STT Not Available\n",
						__FUNCTION__, stack_elapsed_time, bootErrorMessage[BOOT_ERROR_STT]);
		}
		if ( stack_elapsed_time > SI_DOWN_MSG_DELAY &&( SIReceived!= true) )
		{
			bootErrorTable[BOOT_ERROR_SI] = true;
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FP",
								"%s: %d, %s----Channel Map Acquisition Failure \n",
								__FUNCTION__, stack_elapsed_time, bootErrorMessage[BOOT_ERROR_SI]);
		}
		
		if( BOOT_ERROR_NONE != eBroadcastedBootError )
		{
			/* Since the status is marked as false from another context */
			if( false == bootErrorTable[ eBroadcastedBootError ] )
			{
				prevBroadcastedBootError = eBroadcastedBootError = BOOT_ERROR_NONE;
			}
			else
			{
				if( prevBroadcastedBootError != eBroadcastedBootError )
				{
					prevBroadcastedBootError = eBroadcastedBootError;
					/* Once stucked up on boot, further error broadcasting will be switched off 
					     till the current error code is cleared */
					RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FP",
								"%s: ---- Box is in %s state ---- \n",
								__FUNCTION__, bootErrorMessage[ eBroadcastedBootError ]);			
				}
				continue;
			}
		}

	    if (bootErrorTable[BOOT_ERROR_CARD_CP])
	    {
#ifdef GLI
	      if(eBroadcastedBootError != BOOT_ERROR_CARD_CP)
	      {
			IARM_Bus_SYSMgr_EventData_t eventData;
			eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_CABLE_CARD;
			eventData.data.systemStates.state = 0;
			eventData.data.systemStates.error = 1;
			IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
	      }
	      eBroadcastedBootError = BOOT_ERROR_CARD_CP;
#endif	        
	    }
	    else if (bootErrorTable[BOOT_ERROR_CARD_DSG])
	    {
#ifdef GLI
	      if(eBroadcastedBootError != BOOT_ERROR_CARD_DSG)
	      {
			IARM_Bus_SYSMgr_EventData_t eventData;
			eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_DSG_CA_TUNNEL;
			eventData.data.systemStates.state = 0;
			eventData.data.systemStates.error = 1;
			IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
			eBroadcastedBootError = BOOT_ERROR_CARD_DSG;
	      }
#endif	        
	    }
	    else if (bootErrorTable[BOOT_ERROR_ECM_OPERATIONAL])
	    {
#ifdef GLI
	      if(eBroadcastedBootError != BOOT_ERROR_ECM_OPERATIONAL)
	      {
			IARM_Bus_SYSMgr_EventData_t eventData;
			eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_ECM_IP;
			eventData.data.systemStates.state = 0;
			eventData.data.systemStates.error = 1;
			IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
	      }
			eBroadcastedBootError = BOOT_ERROR_ECM_OPERATIONAL;
#endif	        
	    }		
	    else if (bootErrorTable[BOOT_ERROR_SI] ) //IP,STT,SI has same priority,display in the order of Time-Out
	    {
#ifdef GLI
	      if(eBroadcastedBootError !=BOOT_ERROR_SI)
	      {
			IARM_Bus_SYSMgr_EventData_t eventData;
			eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_CHANNELMAP;
			eventData.data.systemStates.state = 0;
			eventData.data.systemStates.error = 1;
			IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
	      }
	      eBroadcastedBootError = BOOT_ERROR_SI;
#endif	        
	    }
	    else if (bootErrorTable[BOOT_ERROR_STT])
	    {
#ifdef GLI
	      if(eBroadcastedBootError=BOOT_ERROR_STT)
	      {
			IARM_Bus_SYSMgr_EventData_t eventData;
			eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_TIME_SOURCE;
			eventData.data.systemStates.state = 0;
			eventData.data.systemStates.error = 1;
			IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
	      }
	      eBroadcastedBootError = BOOT_ERROR_STT;
#endif	        
	    }
		else if (bootErrorTable[BOOT_ERROR_ESTB_IP])
	    {
#ifdef GLI
		      if(eBroadcastedBootError != BOOT_ERROR_ESTB_IP)
		      {
			IARM_Bus_SYSMgr_EventData_t eventData;
			eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_ESTB_IP;
			eventData.data.systemStates.state = 0;
			eventData.data.systemStates.error = 1;
			IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
		      }
		      eBroadcastedBootError = BOOT_ERROR_ESTB_IP;
#endif	        
	    }
	    else if (bootErrorTable[BOOT_ERROR_BOX_DISCONNECTED])
	    {
#ifdef GLI
	      if(eBroadcastedBootError != BOOT_ERROR_BOX_DISCONNECTED)
	      {
			IARM_Bus_SYSMgr_EventData_t eventData;
			eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_DISCONNECTMGR;
			eventData.data.systemStates.state = 1;
			eventData.data.systemStates.error = 1;
			IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
	      }
	      eBroadcastedBootError = BOOT_ERROR_BOX_DISCONNECTED;
#endif	        
	    }
	    else if (bootErrorTable[BOOT_ERROR_PROXY_DOWN_TOO_LONG])
	    {
	      //irrelevant in 2.0
	    }

	}
}

#ifdef TEST_FP 

void TestPostBootEvent(int eEventType, int nMsgCode, unsigned long ulData){

      rmf_osal_eventmanager_handle_t  em = get_pod_event_manager();   
      rmf_osal_event_handle_t event_handle;
      rmf_osal_event_params_t event_params = {0};

      event_params.data = (void *)ulData;
      event_params.data_extension = nMsgCode;
      rmf_osal_event_create(RMF_OSAL_EVENT_CATEGORY_BOOTMSG_DIAG, eEventType, 
                                 &event_params, &event_handle);
      rmf_osal_eventmanager_dispatch_event(em, event_handle);
}

void cdlEventThread(void *arg)
{
   int data;
   data = 0;
   sleep(20);
   printf("------------- CDL starting --------------\n");
   RMF_BOOTMSG_DIAG_MESG_CODE msgCode;

   rmf_Error ret = sem_post( threadSync);
   if (0 != ret )
   {
       RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.FP", "%s - sem_post failed.\n", __FUNCTION__);
   }
   
   msgCode = RMF_BOOTMSG_DIAG_MESG_CODE_START;
   PostBootEvent(RMF_BOOTMSG_DIAG_EVENT_TYPE_CDL, RMF_BOOTMSG_DIAG_MESG_CODE_START, data);
   sleep (1);
   while (data < 20 )
   {
     TestPostBootEvent(RMF_BOOTMSG_DIAG_EVENT_TYPE_CDL, RMF_BOOTMSG_DIAG_MESG_CODE_PROGRESS, ++data);
     sleep(5);
   }
   TestPostBootEvent(RMF_BOOTMSG_DIAG_EVENT_TYPE_CDL, RMF_BOOTMSG_DIAG_MESG_CODE_COMPLETE, ++data);
   
  printf("------------- CDL completed --------------\n");
}
#endif
static void _RDKEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
{
	if (eventId != IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE)
	{
		return;
	}
	if ( strcmp ( owner, IARM_BUS_SYSMGR_NAME ) == 0 )
	{
		RDK_LOG(RDK_LOG_INFO,"LOG.RDK.FP","_RDKEventHandler is invoked\n");
		IARM_Bus_SYSMgr_EventData_t *sysEventData = (IARM_Bus_SYSMgr_EventData_t*)data;
		IARM_Bus_SYSMgr_SystemState_t stateId = sysEventData->data.systemStates.stateId;
		RDK_LOG(RDK_LOG_INFO,"LOG.RDK.FP","_RDKeventHandler invoked for stateid %d of state %d\r\n",\
							stateId, sysEventData->data.systemStates.state);
		switch(stateId)
		{
			case IARM_BUS_SYSMGR_SYSSTATE_CMAC:
				isProxyUp = (sysEventData->data.systemStates.error == 0) ? true : false;
				if( 2 == sysEventData->data.systemStates.state )
				{
					bootErrorTable[ BOOT_ERROR_PROXY_DOWN_TOO_LONG ] = false;
				}
			break;
			case IARM_BUS_SYSMGR_SYSSTATE_DSG_CA_TUNNEL:
				if( 2 == sysEventData->data.systemStates.state )
				{
					bootErrorTable[ BOOT_ERROR_CARD_DSG ] = false;
				}
			break;
			case IARM_BUS_SYSMGR_SYSSTATE_ESTB_IP:
				if( 2 == sysEventData->data.systemStates.state )
				{
					bootErrorTable[ BOOT_ERROR_ESTB_IP ] = false;
				}
			break;				
			case IARM_BUS_SYSMGR_SYSSTATE_CHANNELMAP:
				SIReceived = (sysEventData->data.systemStates.error == 0) ? true : false;
				if( 2 == sysEventData->data.systemStates.state )
				{
					bootErrorTable[ BOOT_ERROR_SI ] = false;
				}
			break;
			case IARM_BUS_SYSMGR_SYSSTATE_TIME_SOURCE:
				siSTTReceived = (sysEventData->data.systemStates.error == 0) ? true : false;
				if( 1 == sysEventData->data.systemStates.state )
				{
					bootErrorTable[ BOOT_ERROR_STT ] = false;
				}
			break;
			case IARM_BUS_SYSMGR_SYSSTATE_DISCONNECTMGR:
				if( 1 == sysEventData->data.systemStates.state )
				{
					bootErrorTable[ BOOT_ERROR_BOX_DISCONNECTED] = true;
					RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FP",
								"%s: %s----STB is in Disconnected mode \n",__FUNCTION__,\
                                                                 bootErrorMessage[BOOT_ERROR_BOX_DISCONNECTED]);
				}
				else if( 0 == sysEventData->data.systemStates.state )
				{
					bootErrorTable[ BOOT_ERROR_BOX_DISCONNECTED] = false;
					RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FP",
								"%s: ----STB is in Connected mode \n",__FUNCTION__);
				}
				else
				{
					RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FP","Unknown state value.Maintaining current status\n");
				}
			break;

			default:
			break;
		}
	}
}

void _EventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
{
    if (strcmp(owner,IARM_BUS_DSMGR_NAME) == 0)
    {
          RDK_LOG(RDK_LOG_INFO, "LOG.RDK.FP",
            "%s: Ds Mgr Event Handler called for Event Id - %d\n",
            __FUNCTION__, eventId );

            switch (eventId) {
            case IARM_BUS_DSMGR_EVENT_TIME_FORMAT_CHANGE:
                {
                  IARM_Bus_DSMgr_EventData_t *eventData = (IARM_Bus_DSMgr_EventData_t *)data;
                  RDK_LOG(RDK_LOG_INFO, "LOG.RDK.FP","Time Format change to  %s \n",eventData->data.FPDTimeFormat.eTimeFormat == dsFPD_TIME_12_HOUR ?"12_HR":"24_HR");
                  if(NULL != getenv("TZ") && (true == bTimeUpdatedStarted))
                  {
                    struct tm stm;
                    time_t timeval;
                    time(&timeval);
                    localtime_r(&timeval, &stm);
                    if(g_fp_supports_text)
                    {
                        try{
                            device::FrontPanelTextDisplay::getInstance("Text").setTime(stm.tm_hour,stm.tm_min);
                        }
                        catch(...)
                        {
                            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FP","Exception caught on function - %s\n", __FUNCTION__);
                            return;
                        }
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.FP","%s() : Set FP as Time\n",__FUNCTION__);
                    }
                  }
                }
                break;
            default:
               break;
        }
    }
}


static IARM_Result_t _SysModeChange(void *arg)
{
    IARM_Bus_CommonAPI_SysModeChange_Param_t *param = (IARM_Bus_CommonAPI_SysModeChange_Param_t *)arg;
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.FP",
            "%s: Sys Mode change notification received - New mode - %d: Old mode - %d\n",
            __FUNCTION__, param->newMode, param->oldMode );
    try{
      
	if(param->newMode == IARM_BUS_SYS_MODE_WAREHOUSE)
	{
	    rmf_osal_envSet("PARSER.BANDSETTING.ALLPASS","TRUE");
	}

        if(param->newMode == IARM_BUS_SYS_MODE_EAS)
        {
            EAS_mode_on = true;
            if(g_fp_supports_text)
            {
                device::FrontPanelTextDisplay::getInstance("Text").setText(EAS_FP_TEXT);
                fp_setText(EAS_FP_TEXT);
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.FP","%s() : Set FP as %s\n",__FUNCTION__,EAS_FP_TEXT);
            }
			vl_mpeos_fpSetSnmpText(EAS_FP_TEXT);
        }
        else 
        {
            if(NULL != getenv("TZ") && (true == bTimeUpdatedStarted))
            {
                struct tm stm;
                time_t timeval;

                time(&timeval);
                localtime_r(&timeval, &stm);
                if(g_fp_supports_text)
                {
                    int timeFormat = device::FrontPanelConfig::getInstance().getTextDisplay("Text").getCurrentTimeFormat();
                    device::FrontPanelTextDisplay::getInstance("Text").setTimeFormat(timeFormat);
                    device::FrontPanelTextDisplay::getInstance("Text").setTime(stm.tm_hour,stm.tm_min);

                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.FP","%s() : Set FP as Time\n",__FUNCTION__);
                }
            }
            EAS_mode_on = false;
        }


    }
    catch(...)
    {
        
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.FP","Exception caught on function - %s\n", __FUNCTION__);
        return IARM_RESULT_IPCCORE_FAIL;
    }
    return IARM_RESULT_SUCCESS;
}
static void enableFPUpdates(bool flag)
{
	gEnableFPUpdate = flag;
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.FP","Set FPUpdate status as %s\n",(flag == true)?"enabled":"disabled");
}

