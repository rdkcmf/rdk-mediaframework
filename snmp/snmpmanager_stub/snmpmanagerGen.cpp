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
#include <list>

#include "ipcclient_impl.h"
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "cmhash.h"
#include "core_events.h"
#include "snmp_types.h"
#include "snmpmanager.h"
#include "SnmpIORM.h"
#include <pthread.h>
/***Start : Snmp related headers**********/
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <signal.h>
#include "Tlvevent.h"
#include "docsDevEventTable_enums.h"
#include "vlSnmpBootEventHandler.h"
#include "ocStbHostMibModule.h"
#include "vlSnmpHostInfo.h"
#include "vl_ocStbHost_GetData.h"
#include "AvInterfaceGetdata.h"
#include "vlEnv.h"
#ifdef USE_1394
#include "vl_1394_api.h"
#endif // USE_1394

#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif
#define MAX_CERTIFICATION_INFO 1
extern "C" void init_vacm_vars();
extern "C" void init_usmUser();

#ifdef USE_SOC_INIT
#ifdef __cplusplus
extern "C" void soc_init_(int id, char *appName, int bInitSurface);
#else
void soc_init_(int id, char *appName, int bInitSurface);
#endif
#endif



extern Avinterface avobj; // Avinterface calss obj
static TlvConfig * vlg_pTlvConfig = NULL;
static int keep_running;
static CvlSnmpBootEventHandler * vlg_pSnmpBootEventHandler = NULL;

RETSIGTYPE
stop_server(int a) {
    keep_running = 0;
    SNMP_DEBUGPRINT("\n SnmpManager :: Stoped &&&&&&&&&&&&&&&&& \n"); 
   
}

        SnmpManager::SnmpManager():CMThread("vlSnmpManager") /*Using default parameters for CMThread*/{
             
        //SNMP_DEBUGPRINT("\n\n ^&^&^& SnmpManager::SnmpManager()\n");
          
    }
    SnmpManager::~SnmpManager(){
        
    }

void SnmpManager::run(void)
{
/* change this if you want to use syslog */
#undef USE_SYSLOG_SNMP 
/* change this if you want to run in the background */
#undef  BACKGROUND_RUN 
/* change this if you want to be a SNMP master agent */
#define AGENTX_SUBAGENT 

  SNMP_DEBUGPRINT(" inside SnmpManager::run1");

  const char * pszSnmpLogTokens = vl_env_get_str(NULL, "RMF_SNMP_DEBUG_LOG_TOKENS");
  if((NULL == pszSnmpLogTokens) || 0 == strlen(pszSnmpLogTokens))
  {
        #ifdef USE_SYSLOG_SNMP
          /* print log errors to syslog or stderr */
            snmp_enable_calllog();
        #else
            snmp_enable_stderrlog();
        #endif
  }
  else
  {
      snmp_enable_filelog("/opt/logs/runSnmp_snmp_log.txt", 0);
      snmp_debug_init();
      snmp_set_do_debugging(1);
      debug_register_tokens((char*)(pszSnmpLogTokens));
  }
  SNMP_DEBUGPRINT(" ::::::::::::  inside SnmpManager::run 1111"); 
  /* we're an agentx subagent? */
#ifdef AGENTX_SUBAGENT
    /* make us a agentx client. */
    netsnmp_ds_set_boolean(NETSNMP_DS_APPLICATION_ID, NETSNMP_DS_AGENT_ROLE, 1);
#endif
// SNMP_DEBUGPRINT(" :::::::::::::::: inside SnmpManager::run 2"); 
  /* run in background, if requested */
#ifdef BACKGROUND_RUN
#ifdef USE_SYSLOG_SNMP
  if ( netsnmp_daemonize(1, 0))
#else
  if ( netsnmp_daemonize(1, 1))
#endif  
      exit(1);
#endif

  /* initialize tcpip, if necessary */
  SOCK_STARTUP;
   netsnmp_ds_set_string(NETSNMP_DS_APPLICATION_ID,
                                      NETSNMP_DS_AGENT_X_SOCKET,
"tcp:127.0.0.1:705");
//"tcp:192.168.100.10:705");
// "tcp:192.168.1.236:705");
 
  /* initialize the agent library */
  init_agent("VividLogi-SNMP-SUBAGENT-DEMON UP");
 // SNMP_DEBUGPRINT(" :::::::::::subagent-demon :::::::::::::   inside SnmpManager::run 3");
  /* initialize mib code here */

  /* mib code: init_ocStbHostMibModule from ocStbHostMibModule.cpp */
  init_ocStbHostMibModule(); 

  /* initialize vacm/usm access control  */
#ifndef AGENTX_SUBAGENT
      init_vacm_vars();
      init_usmUser();
#endif
  /* example-demon will be used to read example-demon.conf files. */
  init_snmp("VividLogi-SNMP-SUBAGENT-DEMON UP");

  /* If we're going to be a snmp master agent, initial the ports */
#ifndef AGENTX_SUBAGENT
    init_master_agent();  /* open the port to listen on (defaults to udp:161) */
#endif

  /* In case we recevie a request to stop (kill -TERM or kill -INT) */
  keep_running = 1;
    //signal(SIGTERM, stop_server);
    //signal(SIGINT, stop_server);

  //snmp_log(LOG_INFO,"VividLogi-SNMP-SUBAGENT-DEMON UP RUNNING.\n");
  RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP","!!!! VividLogi-SNMP-SUBAGENT-DEMON UP RUNNING !!!!\n");  
     
  EdocsDevEventhandling(ERRORCODE_B01_0);
  vlg_pTlvConfig->init_complete();
  vlg_pSnmpBootEventHandler->init_complete();
  
#if 0// Moved this to runSnmp
	printf("%s : %s : %d : Starting to Listen for POD events in SNMP module\n", __FILE__, __FUNCTION__, __LINE__);
  /* Start listening for the pod events*/
  rmf_snmpmgrIPCInit();
#endif

 // SNMP_DEBUGPRINT("\n\n &&&&&&&&& agent_check_and_process :: Created &&&&&&&&&&&&&&&&& \n\n") ;
  while(keep_running) {
    /* if you use select(), see snmp_select_info() in snmp_api(3) */
    /*     --- OR ---  */
    //SNMP_DEBUGPRINT(" inside SnmpManager :: in while = %d ",keep_running);
    agent_check_and_process(1); /* 0 == don't block */
  }

#if 0// Moved this to runSnmp
	printf("%s : %s : %d : Stopped to Listen for POD events in SNMP module\n", __FILE__, __FUNCTION__, __LINE__);
  /* Stop listening for the pod events*/
  rmf_snmpmgrIPCUnInit();
#endif

  //SNMP_DEBUGPRINT("\n\n &&&&&&&&& agent_check_and_process :: killed &&&&&&&&&&&&&&&&& \n\n") ;
  snmp_shutdown("VividLogi-SNMP-SUBAGENT-DEMON SHUTDOWN");
  RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP","!!!! VividLogi-SNMP-SUBAGENT-DEMON SHUTDOWN !!!!\n");
  SOCK_CLEANUP;
  //SNMP_DEBUGPRINT(" inside SnmpManager::run END 5 ");

 
        /* }*///while(1)
  
      
 // return 0;


}
//#endif //#if 0

int SnmpManager::initialize(void){
    vlg_pTlvConfig = new TlvConfig;
    //VLALLOC_INFO2(vlg_pTlvConfig, sizeof(TlvConfig));
    vlg_pTlvConfig->initialize();
  
    vlg_pSnmpBootEventHandler = new CvlSnmpBootEventHandler;
    vlg_pSnmpBootEventHandler->initialize();
  
return 1; //Mamidi:042209
}

static char * VL_PATCH_DHCP_OPTION_43(char* pszString)
{
    static bool bReplaceSpaceWithUnderscore = vl_env_get_bool(1, "DHCP.OPTION43.REPLACE_SPACE_WITH_UNDERSCORE");
    if(bReplaceSpaceWithUnderscore) //Enable / disable replacement of space with underscore based on mpeenv.ini flag.
    {
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP","%s(): DHCP.OPTION43.REPLACE_SPACE_WITH_UNDERSCORE is set to TRUE\n", __func__);
        static char szString[256];
        strncpy(szString, pszString, 256);
        szString [ 255 ]	= 0;

        int i = 0; int nLen = strlen(szString);
        for(i = 0; i < nLen; i++)
        {
            if(isspace(szString[i])) szString[i]='_';
            if(szString[i]<0x20) szString[i]='\0';
        }
        return szString;
    }
    else
    {
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP","%s(): DHCP.OPTION43.REPLACE_SPACE_WITH_UNDERSCORE is set to FALSE\n", __func__);
    }
    return pszString;
}

//Now controlled via DHCP.OPTION43.REPLACE_SPACE_WITH_UNDERSCORE rmfconfig.ini flag//static char * vlSnmpOption43Verbatim(char* pszString)
//Now controlled via DHCP.OPTION43.REPLACE_SPACE_WITH_UNDERSCORE rmfconfig.ini flag//{
//Now controlled via DHCP.OPTION43.REPLACE_SPACE_WITH_UNDERSCORE rmfconfig.ini flag//    return pszString;
//Now controlled via DHCP.OPTION43.REPLACE_SPACE_WITH_UNDERSCORE rmfconfig.ini flag//}
int SnmpManager::init_complete(void)
{
	/* stub */
	return 0;
}


int SnmpManager::is_default ()
{    
  //SNMP_DEBUGPRINT("\n\n & & &    SnmpManager::is_default\n");
    return 1;
}


 

#if 0
int SnmpManager::doInitialize()
{
  
}
#endif //0




