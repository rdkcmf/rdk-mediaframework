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
#include <podimpl.h>
#include <podServer.h>
#include <rmf_osal_init.h>
#include <stdio.h>
#include <unistd.h>
#include <rmf_oobsimgr.h>
#include "rmfproxyservice.h"
#include <stdlib.h>
#include <fstream>
#include "rmf_osal_util.h"

#include <signal.h>
#include <sys/types.h>
#ifdef GLI
#include "libIBus.h"
#include "sysMgr.h"
#endif

#ifdef INCLUDE_BREAKPAD
#include "breakpad_wrapper.h"
const int kExceptionSignals[] = {
  SIGSEGV, SIGABRT, SIGFPE, SIGILL, SIGBUS, SIGTERM, SIGINT, SIGALRM
};
const int kNumHandledSignals =
    sizeof(kExceptionSignals) / sizeof(kExceptionSignals[0]);
struct sigaction old_handlers[kNumHandledSignals];
#endif

#ifdef USE_FRONTPANEL
#include "manager.hpp"
#include "frontpanel.h"
#endif

#if defined GLI | defined USE_FRONTPANEL
//#include "rmf_gli.h"
#include "libIBus.h"
#endif

#ifdef LOGMILESTONE
#include "rdk_logger_milestone.h"
#endif

#define CARD_TYPE_MOTO 0
#define CARD_TYPE_CISCO 1
#define CARD_TYPE_UNKNOWN (-1)

static volatile int32_t manufacturer = CARD_TYPE_UNKNOWN;
static volatile rmf_osal_Bool bServerInited = FALSE;

void dumpStatOnCrash(int signum)
{
    system("top -n 1 -b > /opt/logs/crashStatus.txt");
    printf("runPod is about to crash, got signal %d, written the system top on /opt/logs/crashStatus.txt \n", signum);
    signal(signum, SIG_DFL);
    kill(getpid(), signum);
#ifdef INCLUDE_BREAKPAD
         //allow signal to be processed normally for correct core dump
         printf("Restore breakpad signal handlers\n");
         for (int i = 0; i < kNumHandledSignals; ++i) {
             if (sigaction(kExceptionSignals[i], &old_handlers[i], NULL) == -1) {
                 perror ("sigaction");
                 signal(kExceptionSignals[i], SIG_DFL);
             }
         }
#endif    
}


void rmf_PodmgrOOBSICb( rmf_SiGZGlobalState state, uint16_t param, uint32_t optData );
typedef struct 
{
	rmf_SiGZGlobalState state;
	uint16_t value; 
	uint32_t optData;
}sCardInitParam;
static rmf_osal_ThreadId CardInitMonThreadThreadId = 0;

//SunilS - Added to perform SOC-Specific initialization at the beginning of application
#ifdef USE_SOC_INIT
#ifdef __cplusplus
extern "C" void soc_init_(int id, char *appName, int bInitSurface);
#else
void soc_init_(int id, char *appName, int bInitSurface);
#endif
#endif

#ifdef USE_SOC_SIGNAL_MASK
extern "C" {
void SetSignalMask(void);
}
#endif

static void CardInitMonThread( void *data )
{
	sCardInitParam *param = ( sCardInitParam * )data;
	uint16_t manufacturerId = 0;

	while( ( CARD_TYPE_UNKNOWN == manufacturer )  ||  ( bServerInited == FALSE ) )
	{
		rmf_Error  ret = podmgrGetManufacturerId( &manufacturerId );
		if( ( RMF_SUCCESS == ret ) && ( bServerInited == TRUE ) )
		{
			manufacturer = (manufacturerId & 0xFF00) >> 8;
		}
		else
		{
			RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "GroundZero: retrying till card is up \n");
			usleep( 100000 );
		}
	}

	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "GroundZero: Card type is %s\n",
			(CARD_TYPE_MOTO == manufacturer)? "MOTOROLA" : "CISCO" );
	if(param)
	{
	  rmf_PodmgrOOBSICb( param->state, param->value, param->optData );
	  rmf_osal_memFreeP( RMF_OSAL_MEM_POD, param );
	  param = NULL;
  }
}

void rmf_PodmgrOOBSICb( rmf_SiGZGlobalState state, uint16_t value, uint32_t optData )
{
#ifdef GLI

	IARM_Bus_SYSMgr_EventData_t eventData;
	
	if (
		
		( ( CARD_TYPE_UNKNOWN == manufacturer )  || ( bServerInited == FALSE )  )&& 
		( ( RMF_OOBSI_GLI_STT_ACQUIRED != state  ) &&  ( RMF_OOBSI_GLI_DAC_ID_CHANGED  != state ) )
		)
	{
		sCardInitParam *param = NULL;
		rmf_Error ret = RMF_SUCCESS;

		rmf_osal_memAllocP( RMF_OSAL_MEM_POD, sizeof( sCardInitParam ), ( void ** ) &param );
		if( NULL == param ) RDK_LOG( RDK_LOG_ERROR,  "LOG.RDK.POD", "Malloc failed\n");
		param->optData = optData;
		param->value = value;
		param->state = state;
		ret = rmf_osal_threadCreate( CardInitMonThread, (void *) param,
									  RMF_OSAL_THREAD_PRIOR_DFLT,
									  RMF_OSAL_THREAD_STACK_SIZE,
									  &CardInitMonThreadThreadId ,
									  "CardInitMonThread" );
		if ( RMF_SUCCESS != ret ) RDK_LOG( RDK_LOG_ERROR,  "LOG.RDK.POD", "failed to create CardInitMonThread\n" );
		return;
	}
	
	switch( state )
	{
		case RMF_OOBSI_GLI_SI_FULLY_ACQUIRED:
		{
			uint16_t ChannelMapId = value;
			uint16_t VCTId = optData;
			eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_CHANNELMAP; 
			eventData.data.systemStates.state =  2;  //0 - Not Available, 1 - Fully Acquired.
			eventData.data.systemStates.error = 0; 

			if ( ( CARD_TYPE_MOTO == manufacturer) && ( VIRTUAL_CHANNEL_UNKNOWN != ChannelMapId ) )
			{
				snprintf( eventData.data.systemStates.payload, 
				sizeof(eventData.data.systemStates.payload),"%d", ChannelMapId );
				RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD",  "GroundZero: channel map id of MOTOROLA  is acquired 0x%x\n", ChannelMapId);
				IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
#ifdef LOGMILESTONE
				logMilestone("CHANNEL_MAP_ACQUIRED");
#else
				RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "CHANNEL_MAP_ACQUIRED\n");
#endif
				RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD",  "GroundZero: channel map acquired info is broadcasted.\n");
			}
			else if ( ( CARD_TYPE_CISCO == manufacturer) && ( VIRTUAL_CHANNEL_UNKNOWN != VCTId ) )
			{
				snprintf( eventData.data.systemStates.payload, 
				sizeof(eventData.data.systemStates.payload),"%d", VCTId );
				RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD",  "GroundZero: VCTId is acquired 0x%x\n", VCTId);
				IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));	
#ifdef LOGMILESTONE
				logMilestone("CHANNEL_MAP_ACQUIRED");
#else
				RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD",  "CHANNEL_MAP_ACQUIRED\n"); 
#endif
				RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD",  "GroundZero: channel map acquired info is broadcasted.\n"); 
			}
			else
			{
				//In CISCO case VCT ID is the channel map Id.
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "<%s: %s> : GroundZero:  ChannelMapId is not available, Critical ERROR ...! \n",
					SIMODULE, __FUNCTION__ );
			}
		}	
		break;			
		
		case RMF_OOBSI_GLI_CHANNELMAP_ID_CHANGED:
		{
			uint16_t ChannelMapId = value;	
			if ( ( CARD_TYPE_MOTO == manufacturer) && ( VIRTUAL_CHANNEL_UNKNOWN != ChannelMapId ) )
			{
				eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_CHANNELMAP; 
				eventData.data.systemStates.state = ((optData== 1 ) ?  2 : 0 );
				eventData.data.systemStates.error = 0; 
				snprintf( eventData.data.systemStates.payload, 
					sizeof(eventData.data.systemStates.payload),"%d", ChannelMapId );
				IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));			
				RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD",  "GroundZero: Channel map ID of motorola is Changed 0x%x \n", ChannelMapId );
			}
		}
		break;
		case RMF_OOBSI_GLI_DAC_ID_CHANGED:
		{
			uint16_t DACId = value;
			eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_DAC_ID; 
			eventData.data.systemStates.state = 2; 
			eventData.data.systemStates.error = 0; 
			snprintf( eventData.data.systemStates.payload, sizeof(eventData.data.systemStates.payload),
				"%d", DACId );
			IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD",  "GroundZero: DAC ID is Changed 0x%x \n", DACId );
		}	
		break;			
		case RMF_OOBSI_GLI_VCT_ID_CHANGED:
		{
			uint16_t ChannelMapId = value;			
			if ( ( CARD_TYPE_CISCO == manufacturer) && ( VIRTUAL_CHANNEL_UNKNOWN != ChannelMapId ) )
			{
				eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_CHANNELMAP; 
				eventData.data.systemStates.state = ((optData== 1 ) ?  2 : 0 );
				eventData.data.systemStates.error = 0; 
				snprintf( eventData.data.systemStates.payload, 
					sizeof(eventData.data.systemStates.payload),"%d", ChannelMapId );
				IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));			
				RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD",  "GroundZero: Channel map id of CISCO is Changed 0x%x \n", ChannelMapId );		
			}
		}
		break;
		case RMF_OOBSI_GLI_STT_ACQUIRED:
		{
			eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_DSG_BROADCAST_CHANNEL;
			eventData.data.systemStates.state =  2; 
			IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
			eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_TIME_SOURCE;
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD",  "GroundZero: STT is now acquired\n");
			eventData.data.systemStates.error=0;
			eventData.data.systemStates.state =  1;  //0 - Not Available, 1 - Fully Acquired.
			IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));   
		}
		break;
		default:
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD",  "GroundZero: Got unknown event \n");
		break;
	}
#endif     
}

void OffPlantProxySetup( )
{
	const char *offPlantEnabled = NULL;
	if ( ( offPlantEnabled = (char*)rmf_osal_envGet( "ENABLE.OFFPLANT.SETUP") ) != NULL ) 
	{	
		if ( 0 == strcasecmp( offPlantEnabled, "TRUE" ) )
		{
			int32_t iChannelMapId = 1902;
			int32_t iDACId = 3414;
			int32_t iVodServerId = 61003;

			const char *cChannelMapId = (char*)rmf_osal_envGet( "OFFPLANT.SETUP.CHANNELMAPID" );
			const char *cDACId = (char*)rmf_osal_envGet( "OFFPLANT.SETUP.DACID" );
			const char *cVodServerId = (char*)rmf_osal_envGet( "OFFPLANT.SETUP.VODSERVERID" );

			if ( NULL != cChannelMapId )
			{
				iChannelMapId = atoi( cChannelMapId );
			}
			if ( NULL != cDACId )
			{
				iDACId = atoi( cDACId );
			}
			if (  NULL != cVodServerId )
			{
				iVodServerId = atoi( cVodServerId );
			}
		
			/* Assigning as MOTO, just to broadcast the channel map id */  			
			manufacturer = CARD_TYPE_MOTO;

			FILE * fp = fopen( "/tmp/stt_received", "w" );
			if(NULL != fp)
			{
				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
					"%s - Created fake stt_received \n",
					__FUNCTION__ );			
			    fclose( fp );
			}
			
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
				"%s - Sending fake IARM events for off plant setup ChannelMapId = %d, DACId = %d,  vodId = %d\n",
				__FUNCTION__, iChannelMapId,  iDACId, iVodServerId );

			rmf_PodmgrOOBSICb( RMF_OOBSI_GLI_CHANNELMAP_ID_CHANGED,0 , 1 );
			rmf_PodmgrOOBSICb(RMF_OOBSI_GLI_DAC_ID_CHANGED, (uint16_t)iDACId, 2 );
			rmf_PodmgrOOBSICb( RMF_OOBSI_GLI_STT_ACQUIRED, 0, 0);
			rmf_PodmgrOOBSICb( RMF_OOBSI_GLI_SI_FULLY_ACQUIRED, (uint16_t)iChannelMapId, 1 );
#ifdef GLI						
			IARM_Bus_SYSMgr_EventData_t eventData;
			eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_VOD_AD;
			eventData.data.systemStates.state = 2;
			eventData.data.systemStates.error = 0;
			snprintf( eventData.data.systemStates.payload, sizeof(eventData.data.systemStates.payload),"%d", cVodServerId );
			IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
#endif
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
				"%s - Sending fake IARM events for off plant setup Finished \n",
				__FUNCTION__ );	
		}		
	}

}

bool checkMatch(std::string line)
{
        std::size_t found = line.find("DAC_INIT: Completed");
        if (found == std::string::npos)
                return false;
        else
                return true;
}

void postDacInitTimeStamp()
{
    bool found = false;
    std::string line;
    std::fstream initLogFile ("/opt/mpeos_hrv_init_log.txt",std::fstream::in);

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s \n",__FUNCTION__);

    if (initLogFile.is_open())
    {
          while ( std::getline (initLogFile,line) )
          {
                if( !(checkMatch(line)) )
                        continue;
                else
                {
                        found = true;
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s - DAC INIT timestamp - %s\n", __FUNCTION__,line.c_str());
                        IARM_Bus_SYSMgr_EventData_t eventData;
                        eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_DAC_INIT_TIMESTAMP;
                        eventData.data.systemStates.state = 1;
                        eventData.data.systemStates.error = 0;
                        strncpy(eventData.data.systemStates.payload,line.c_str(),strlen(line.c_str())-1);
                        eventData.data.systemStates.payload[strlen(line.c_str())-1]='\0';
                        IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
                        break;
                }
           }
           if( !found )
           {
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "mpeos_hrv_init_log.txt doesn't have DAC_INIT\n");
           }
           initLogFile.close();
    }
    else
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s - mpeos_hrv_init_log.txt doesn't exists\n",__FUNCTION__);
    }

}

main(int argc, char * argv[])
{
	rmf_Error ret;
	const char* envConfigFile = NULL;
	const char* debugConfigFile = NULL;
	int itr=0;

#ifdef INCLUDE_BREAKPAD
breakpad_ExceptionHandler();
#endif

//SunilS - Added to perform SOC-Specific initialization at the beginning of application
	while (itr < argc)
	{
		if(strcmp(argv[itr],"--config")==0)
		{
			itr++;
			if (itr < argc)
			{
				envConfigFile = argv[itr];
			}
			else
			{
				break;
			}
		}
		if(strcmp(argv[itr],"--debugconfig")==0)
		{
			itr++;
			if (itr < argc)
			{
				debugConfigFile = argv[itr];
			}
			else
			{
				break;
			}
		}
		itr++;
	}
#ifdef USE_SOC_SIGNAL_MASK
	SetSignalMask();
#endif
//	printf("%s:%d : envConfigFile= %s, debugConfigFile= %s\n", __FUNCTION__, __LINE__, envConfigFile, debugConfigFile);
	ret = rmf_osal_init( envConfigFile, debugConfigFile);
	if( RMF_SUCCESS != ret)
	{
		printf("\nrmf_osal_init() failed, ret = %x\n", ret);
	}

#if defined GLI | defined USE_FRONTPANEL
	IARM_Result_t iResult = IARM_RESULT_SUCCESS;
//	IARM_Bus_Init( "PodServer" ); 
	IARM_Bus_Init( "rmfStreamerClient" );
	IARM_Bus_Connect();
#endif

//	sleep( 20 );
#ifdef USE_SOC_INIT
        printf("runPod Calling soc_init_()\n");
        soc_init_(-1, "Podmanager", 0);
#endif


#ifdef USE_FRONTPANEL
	// Initialize IARM to receive IARM bus events 

#ifdef INCLUDE_BREAKPAD
     //Backup breakpad signal handlers in old_handlers
     for (int i = 0; i < kNumHandledSignals; ++i) {
         if (sigaction(kExceptionSignals[i], NULL, &old_handlers[i]) == -1) {
              printf("sigaction failed while trying to fetch existing handlers");
              return 1;
         }
     }
#endif
	device::Manager::Initialize();
    signal(SIGSEGV, dumpStatOnCrash);

	// Initialize front panel
	fp_init();
	fp_setupBootSeq();
#endif  

	rmf_OobSiMgr::rmf_SI_registerSIGZ_Callback( rmf_PodmgrOOBSICb );
	rmf_OobSiMgr *pOobSiMgr =  rmf_OobSiMgr::getInstance();
	if ( NULL == pOobSiMgr )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.QAMSRC", "%s - rmf_OobSiMgr::getInstance() failed.\n", __FUNCTION__);
		return 1;
	}
	podmgrInit();
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "Calling podServerStart \n");
	podServerStart();
	bServerInited = TRUE;
	OffPlantProxySetup( );

	postDacInitTimeStamp();

#ifdef USE_PXYSRVC
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.QAMSRC", "Getting Diagnostics Data\n");
        getDiagnostics();
#endif
	char procData[1024];
	memset(procData,0,1024);
	snprintf(procData,1024,"for variable in $(fuser %s/tcp); do ps -ef | grep $variable; ls -l /proc/$variable/exe; done;","50506");
	RDK_LOG(RDK_LOG_INFO,"LOG.RDK.IPC","%s\n",procData);
	system(procData);

	while(1)
	{
		sleep(5000);
	}
}

