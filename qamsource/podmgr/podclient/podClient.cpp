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
#include <rdk_debug.h>
#include <rmf_osal_mem.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <rmf_pod.h>
#include <rmf_osal_resreg.h>
#include <canhClient.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <rmf_osal_util.h>

#ifdef GLI
#include "sysMgr.h"
#include "libIBus.h"
#endif

static rmf_osal_eventqueue_handle_t g_qamsrc_eventq;

/*-----------------------------------------------------------------*/
typedef enum _VLMPEOS_CDL_IMAGE_TYPE
{
    VLMPEOS_CDL_IMAGE_TYPE_MONOLITHIC    = 0,
    VLMPEOS_CDL_IMAGE_TYPE_FIRMWARE      = 1,
    VLMPEOS_CDL_IMAGE_TYPE_APPLICATION   = 2,
    VLMPEOS_CDL_IMAGE_TYPE_DATA          = 3,

    VLMPEOS_CDL_IMAGE_TYPE_ECM           = 0x100,
    VLMPEOS_CDL_IMAGE_TYPE_INVALID       = 0x7FFFFFFF
} VLMPEOS_CDL_IMAGE_TYPE;

/*-----------------------------------------------------------------*/
typedef enum _VLMPEOS_CDL_IMAGE_SIGN
{
    VLMPEOS_CDL_IMAGE_SIGN_NOT_SPECIFIED,
    VLMPEOS_CDL_IMAGE_SIGNED,
    VLMPEOS_CDL_IMAGE_UNSIGNED,

} VLMPEOS_CDL_IMAGE_SIGN;

#ifdef USE_POD_IPC
#include <rmf_osal_ipc.h>
#include <rmf_osal_sync.h>
#include <podServer.h>
#include <QAMSrcDiag.h>

#ifdef XONE_STB
#include "pace_tuner_api.h"
#endif

#include <cipher.h>
#else
#include "rmf_oobService.h"

extern "C" void vlDsgDumpDsgStats();
extern "C" void vlDsgCheckAndRecoverConnectivity();
extern "C" void vlMpeosCdlUpgradeToImage(VLMPEOS_CDL_IMAGE_TYPE eImageType, VLMPEOS_CDL_IMAGE_SIGN eImageSign, int bRebootNow, const char * pszImagePathName);
#endif

static rmf_osal_ThreadId podFWUMonitorThreadId = 0;
extern void InbandCableCardFwuMonitor( void *param );
extern bool IsCFWUInProgress( );
#ifdef QUERY_CA_FROM_CANH
typedef struct 
{
	uint16_t sourceId;
	uint32_t sessionHandle;
	uint32_t qamContext;
}sourceParams;
static void podCheckSourceAuthorization( void *sourceDetails );
static rmf_osal_ThreadId podCheckSourceAuthorizationThreadId = 0;
#endif

#define EID_STATUS_UNKNOWN -1
typedef struct
{
	uint32_t eid;
	int8_t status;
}EIDStatus_t;

EIDStatus_t *pEIDStatus;

int32_t gChannelMapId = -1;
int32_t gControllerId = -1;
static volatile int32_t pod_server_cmd_port_no;
static volatile int32_t pod_hal_service_cmd_port_no;
static volatile rmf_osal_Bool g_pod_filter_enabled;
static rmf_Error updateSystemIds(  );

#ifdef USE_POD_IPC

#define POD_IS_UP 1
#define POD_IS_DOWN 0

static uint32_t podClientHandle;
static uint32_t podSIMgrClientHandle;
static rmf_osal_eventmanager_handle_t pod_client_event_manager;
static rmf_osal_eventmanager_handle_t simgr_client_event_manager;
static rmf_osal_ThreadId podClientMonitorThreadId = 0;
static rmf_osal_ThreadId podEventPollThreadId = 0;
static rmf_osal_ThreadId podSIMgrEventPollThreadId = 0;

IPCServer *pPODHALServer = NULL;

typedef enum
{
        HST=-11,
        AKST=-9,
        PST,
        MST,
        CST,
        EST
} podTimeZones;

static int64_t gTimezoneInHours = -1;
static int16_t gDayLightFlag = -1;


typedef struct {
  int timezoneinhours;
  char timezoneinchar[8];
  //char dsttimezoneinchar[16];
  char dsttimezoneinchar[26];
}podTimezone;

typedef struct _decryptQ
{
	uint32_t sessionHandle;
	rmf_osal_eventqueue_handle_t m_queue;
	struct _decryptQ *next;
} decryptQ;

podTimezone m_timezoneinfo[6] ={
                            {-11,"HST11","HST11HDT,M3.2.0,M11.1.0"},
                            {-9,"AKST","AKST09AKDT,M3.2.0,M11.1.0"},
                            {-8,"PST08","PST08PDT,M3.2.0,M11.1.0"},
                            {-7,"MST07","MST07MDT,M3.2.0,M11.1.0"},
                            {-6,"CST06","CST06CDT,M3.2.0,M11.1.0"},
                            {-5,"EST05","EST05EDT,M3.2.0,M11.1.0"}};
static int isPodMgrUp;
static rmf_osal_Cond event_thread_stop_cond;
static rmf_osal_Cond hbmon_thread_stop_cond;

static decryptQ *podDecryptQueueList;
static rmf_osal_Mutex podDecryptQueueListMutex;
static rmf_osal_Mutex podHandleMutex;
static rmf_osal_Mutex podSIMgrHandleMutex;

static rmf_Error notifyCCIChange( uint32_t qamContext, uint32_t sessionHandle, uint32_t CCI, rmf_PODMgrEvent event );
static void podHeartbeatMonThread( void *data );
static uint32_t getPodClientHandle(uint32_t *pHandle);
static uint32_t getSIMgrClientHandle(uint32_t *pHandle);
static void updatePodClientHandle(uint32_t handle);
static void updateSIMgrClientHandle(uint32_t handle);
static void podEventPollThread( void *data );
static void podSIMgrEventPollThread( void *data );
static void podHalService( IPCServerBuf * pServerBuf, void *pUserData );
#endif

static rmf_Error parseDescriptors( rmf_SiMpeg2DescriptorList * pMpeg2DescList,
							rmf_SiElementaryStreamList * pESList,
							parsedDescriptors * pParseOutput );
static rmf_Error allocateDescInfo( parsedDescriptors * pParseOutput,         rmf_SiElementaryStreamList *pESList,
        rmf_SiMpeg2DescriptorList *pMpeg2DescList,
							uint8_t caPMTCMDId );
static rmf_Error parseFilteredDescriptors( rmf_SiMpeg2DescriptorList * pMpeg2DescList,
							rmf_SiElementaryStreamList * pESList,
							parsedDescriptors * pParseOutput );
static rmf_Error allocateFilteredDescInfo( parsedDescriptors * pParseOutput,         rmf_SiElementaryStreamList *pESList,
        rmf_SiMpeg2DescriptorList *pMpeg2DescList,
							uint8_t caPMTCMDId );

static rmf_Error getCASystemId (uint16_t *pCA_system_id);


void rmf_podmgrDsgCheckAndRecoverConnectivity();
void rmf_podmgrCdlUpgradeToImage(int eImageType, int eImageSign, int bRebootNow, const char * pszImagePathName);

#ifdef USE_POD_IPC
static rmf_Error registerPodClient( )
{
	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_REGISTER_CLIENT);
	
	int32_t result_recv = 0;
	uint32_t handle = 0;
	
	int8_t res = ipcBuf.sendCmd( pod_server_cmd_port_no);

	if ( 0 != res )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"%s: Lost connection with POD. res = %d\n",
				 __FUNCTION__, res );
		return RMF_OSAL_ENODATA;
	}

	result_recv = ipcBuf.getResponse( &handle );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"%s: No response from POD. result_recv = %d\n",
				 __FUNCTION__, result_recv );	
		return RMF_OSAL_ENODATA;
	}

	if( 0 == handle  )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"%s: POD Handle is zero\n",
				 __FUNCTION__);			
		return RMF_OSAL_ENODATA;
	}
	
	updatePodClientHandle( handle );
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
		"%s: Updated POD handle successfully \n",
			 __FUNCTION__);
	return RMF_SUCCESS;	
	
}

static rmf_Error registerSIMgrClient( )
{
	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_REGISTER_SIMGR_CLIENT);
	
	int32_t result_recv = 0;
	uint32_t handle = 0;
	
	int8_t res = ipcBuf.sendCmd( pod_server_cmd_port_no );

	if ( 0 != res )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"%s: Lost connection with POD. res = %d\n",
				 __FUNCTION__, res );
		return RMF_OSAL_ENODATA;
	}

	result_recv = ipcBuf.getResponse( &handle );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"%s: No response from POD. result_recv = %d\n",
				 __FUNCTION__, result_recv );	
		return RMF_OSAL_ENODATA;
	}

	if( 0 == handle  )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"%s: POD Handle is zero\n",
				 __FUNCTION__);			
		return RMF_OSAL_ENODATA;
	}
	
	updateSIMgrClientHandle( handle );
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
		"%s: Updated SIMgr handle successfully \n",
			 __FUNCTION__);
	return RMF_SUCCESS;	
	
}

static rmf_Error unregisterPodClient( )
{
	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_UNREGISTER_CLIENT);
	
	int32_t result_recv = 0;
	uint32_t handle = 0;

	ipcBuf.addData((void *) &handle, sizeof( handle ) );
	
	int8_t res = ipcBuf.sendCmd( pod_server_cmd_port_no );

	if ( 0 != res )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"%s: Lost connection with POD. res = %d\n",
				 __FUNCTION__, res );
		return RMF_OSAL_ENODATA;
	}

	result_recv = ipcBuf.getResponse( &handle );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"%s: No response from POD. result_recv = %d\n",
				 __FUNCTION__, result_recv );	
		return RMF_OSAL_ENODATA;
	}

	return RMF_SUCCESS;	
}

static rmf_Error unregisterSIMgrClient( )
{
	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_UNREGISTER_SIMGR_CLIENT);
	
	int32_t result_recv = 0;
	uint32_t handle = 0;

	ipcBuf.addData((void *) &handle, sizeof( handle ) );
	
	int8_t res = ipcBuf.sendCmd( pod_server_cmd_port_no );

	if ( 0 != res )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"%s: Lost connection with POD. res = %d\n",
				 __FUNCTION__, res );
		return RMF_OSAL_ENODATA;
	}

	result_recv = ipcBuf.getResponse( &handle );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"%s: No response from POD. result_recv = %d\n",
				 __FUNCTION__, result_recv );	
		return RMF_OSAL_ENODATA;
	}

	return RMF_SUCCESS;	
}

void updatePodClientHandle(uint32_t handle)
{
	rmf_osal_mutexAcquire( podHandleMutex );
	podClientHandle = handle;
	rmf_osal_mutexRelease( podHandleMutex );	
}

void updateSIMgrClientHandle( uint32_t handle )
{
	rmf_osal_mutexAcquire( podSIMgrHandleMutex );
	podSIMgrClientHandle = handle;
	rmf_osal_mutexRelease( podSIMgrHandleMutex );	
}

uint32_t getPodClientHandle(uint32_t *pHandle)
{
	rmf_osal_mutexAcquire( podHandleMutex );
	*pHandle = podClientHandle;
	rmf_osal_mutexRelease( podHandleMutex );		
	return *pHandle;
}

uint32_t getSIMgrClientHandle( uint32_t *pHandle )
{
	rmf_osal_mutexAcquire( podSIMgrHandleMutex );
	*pHandle = podSIMgrClientHandle;
	rmf_osal_mutexRelease( podSIMgrHandleMutex );		
	return *pHandle;
}

static void podHeartbeatMonThread( void *data )
{	
	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_HB);
	int32_t result_recv = 0;
	rmf_Error ret = RMF_SUCCESS;
	int8_t res = 0;
	uint8_t currentState = POD_IS_DOWN;

	while( isPodMgrUp )
	{
		if( POD_IS_DOWN == currentState )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"%s: POD_IS_DOWN\n",
				 __FUNCTION__);	

			updatePodClientHandle( 0 );
			updateSIMgrClientHandle( 0 );
		}
		
		res = ipcBuf.sendCmd( pod_server_cmd_port_no );
		if ( 0 != res )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
				"%s: Lost connection with POD. res = %d retrying...\n",
					 __FUNCTION__, res );
			currentState = POD_IS_DOWN;
			sleep(1);
			continue;
		}

		result_recv = ipcBuf.getResponse( &ret );
		if( (result_recv < 0)  ||( RMF_SUCCESS != ret ) )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
				"%s: No response from POD. result_recv = %d, ret= %d retrying...\n",
					 __FUNCTION__, result_recv, ret );	
			currentState = POD_IS_DOWN;
			sleep(5);
			continue;
		}		

		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD", 
			"%s: POD process is live \n",
				 __FUNCTION__);

		if( POD_IS_DOWN == currentState )
		{
			/* Need to get handle everytime when POD is restarted */
			if ( RMF_SUCCESS !=  registerPodClient( ) )
			{
				currentState = POD_IS_DOWN;
				continue;
			}
			if ( RMF_SUCCESS !=  registerSIMgrClient( ) )
			{
				currentState = POD_IS_DOWN;
				continue;
			}			
		}
		
		currentState = POD_IS_UP;
		sleep( 10 );
	}

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD","%s::while break\n", __FUNCTION__);
    rmf_osal_condWaitFor( event_thread_stop_cond, 1000 );
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD","%s::Got event thread stopped condition\n", __FUNCTION__);
   	rmf_osal_condWaitFor( event_thread_stop_cond, 1000 );
	unregisterSIMgrClient();
	unregisterPodClient();
    	rmf_osal_condSet( hbmon_thread_stop_cond );
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD","%s::Set HBMonitor thread stop condition and exiting HBMon Thread...\n", __FUNCTION__);
	return;
}

void podClientfreefn( void *data )
{
	rmf_osal_memFreeP( RMF_OSAL_MEM_POD, data );
}

rmf_Error setTimeZone( int32_t timezoneinhours, int32_t daylightflag )
{
	int32_t ret = 0;
	int32_t index = 0;
	struct tm stm;
	time_t timeval;
	struct timeval tv;

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD", 
			"%s: timezoneinhours: %d, daylightflag: %d \n",
			__FUNCTION__, timezoneinhours, daylightflag );

	if( ( timezoneinhours == gTimezoneInHours ) && ( daylightflag == gDayLightFlag ) )
	{
		return RMF_SUCCESS;
	}
	
        //Currently only the main time zones in US from west to east
	switch( timezoneinhours ){
		case HST:
			index = 0;
		   break;
		case AKST:
			index = 1;
		   break;
		case PST:
			index = 2;
		   break;
		case MST:
			index = 3;
		   break;
		case CST:
			index = 4;
		   break;
		case EST:
			index = 5;
		   break;
		default:
			index = -1;
		   break;
	}

	if( index < 0 )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"%s: Timezone unknown \n",
			__FUNCTION__);
		return RMF_OSAL_EINVAL;
	}

	if( daylightflag )
	{
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD", 
				"%s: Calling setenv() for setting timezone as '%s'\n",
			__FUNCTION__, m_timezoneinfo[index].dsttimezoneinchar);

		ret = setenv( "TZ", m_timezoneinfo[index].dsttimezoneinchar, 1 );
	}
	else
	{
		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD", 
				"%s: Calling setenv() for setting timezone as '%s'\n",
				__FUNCTION__, m_timezoneinfo[index].timezoneinchar);

		ret = setenv( "TZ", m_timezoneinfo[index].timezoneinchar, 1 );
	}

	if( ret < 0 )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
				"%s: setenv() failed with error code: 0x%x \n",
				__FUNCTION__, ret );
		return RMF_OSAL_ENODATA;
	}
	
	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD", 
			"%s: TZ for rmfStreamer process: %s\n",
			__FUNCTION__, getenv("TZ") );
	
	time( &timeval );
	localtime_r( &timeval, &stm );
	gettimeofday( &tv, NULL );

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
		"%s: Local Time in rmfStreamer process: %d:%d\n", 
		__FUNCTION__, stm.tm_hour, stm.tm_min );

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: Exit \n",	__FUNCTION__ );
	gTimezoneInHours = timezoneinhours ;
	gDayLightFlag = daylightflag ;

	return RMF_SUCCESS;
}

rmf_Error write_time ( int64_t value )
{
       struct timeval timesec;
	int32_t ret = 0;

        //time_t rawTime;
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
			"%s: Enter - value: %lld \n",
			__FUNCTION__, value);

	timesec.tv_sec = value;
	timesec.tv_usec = 0;

	ret = settimeofday (&timesec, NULL);

	time_t rawTime;
	time( &rawTime );
	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD",
		"The current local time in rmfStreamer: %s\n", ctime( &rawTime) );
	
	if( ret < 0 ) return RMF_OSAL_EINVAL;
	
	return RMF_SUCCESS;
}

void podHalService( IPCServerBuf * pServerBuf, void *pUserData )
{
	uint32_t identifyer = 0;
	
	identifyer = pServerBuf->getCmd(  );
	IPCServer *pPODHALServer = ( IPCServer * ) pUserData;

	switch ( identifyer )
	{
	  case POD_HAL_SERVER_CMD_CONFIG_CIPHER:
	  	{
			unsigned char ltsid;
			unsigned short PrgNum;
			unsigned long *decodePid;
			unsigned long numpids;
			unsigned long DesKeyAHi;
			unsigned long DesKeyALo;
			unsigned long DesKeyBHi;
			unsigned long DesKeyBLo;
			int32_t result_recv = 0;
			int ret = 0;
			
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				"%s : IPC called POD_HAL_SERVER_CMD_CONFIG_CIPHER \n", __FUNCTION__ );

			result_recv |= pServerBuf->collectData( (void *)&ltsid, 
										sizeof( ltsid) );
			result_recv |= pServerBuf->collectData( (void *)&PrgNum, 
										sizeof( PrgNum) );							
			result_recv |= pServerBuf->collectData( (void *)&numpids, 
										sizeof( numpids) );							
			rmf_osal_memAllocP( RMF_OSAL_MEM_POD, numpids * sizeof( unsigned long ), (void **)&( decodePid) );							
			result_recv |= pServerBuf->collectData( (void *)decodePid, 
										numpids * sizeof( unsigned long) );	
			result_recv |= pServerBuf->collectData( (void *)&DesKeyAHi, 
										sizeof( DesKeyAHi) );							
			result_recv |= pServerBuf->collectData( (void *)&DesKeyALo, 
										sizeof( DesKeyALo) );							
			result_recv |= pServerBuf->collectData( (void *)&DesKeyBHi, 
										sizeof( DesKeyBHi) );							
			result_recv = pServerBuf->collectData( (void *)&DesKeyBLo, 
										sizeof( DesKeyBLo) );	
#ifndef NO_CIPHER										
			if( 0 == result_recv )
			{
				ret = configureCipher(ltsid, PrgNum, decodePid, numpids, DesKeyAHi, DesKeyALo, DesKeyBHi, DesKeyBLo, NULL);
			}			
			else
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s -Unexpected data for %d cmd in the buffer %d \n",
					__FUNCTION__, identifyer, result_recv );
				ret = RMF_OSAL_ENODATA;
			}
#endif			
			rmf_osal_memFreeP( RMF_OSAL_MEM_POD,
							   decodePid );
			pServerBuf->addResponse( ret );
			pServerBuf->sendResponse(  );
			pPODHALServer->disposeBuf( pServerBuf );
		}
	  	break;
		
	  case POD_HAL_SERVER_CMD_GET_TUNER_INFO:
	  	{
			uint32_t tunerIndex;
			TunerData tunerData;
			int32_t result_recv = 0;
			uint32_t ret = 0;

			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				"%s : IPC called POD_HAL_SERVER_CMD_GET_TUNER_INFO \n", __FUNCTION__ );
			result_recv = pServerBuf->collectData( (void *)&tunerIndex, 
										sizeof( tunerIndex) );
			if( 0 != result_recv )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s -Unexpected data for %d cmd in the buffer %d \n",
					__FUNCTION__, identifyer, result_recv);
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
				pServerBuf->sendResponse(  );
				pPODHALServer->disposeBuf( pServerBuf );			
				break;				
			}			
			memset( &tunerData, 0, sizeof( tunerData ) );
			ret = getTunerInfo(tunerIndex, &tunerData );
			pServerBuf->addResponse( ret );
			if( RMF_SUCCESS == ret )
			{
				pServerBuf->addData((void *)&tunerData, sizeof( tunerData ) );							
			}
			pServerBuf->sendResponse(  );
			pPODHALServer->disposeBuf( pServerBuf );
		}
	  	break;
		
	  case POD_HAL_SERVER_CMD_GET_PACE_TUNER_INFO:
	  	{
#ifdef XONE_STB
			int tunerIndex;
			PaceTunerParams tunerData;
			int32_t result_recv = 0;
			uint32_t ret = 0;

			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				"%s : IPC called POD_HAL_SERVER_CMD_GET_PACE_TUNER_INFO \n", __FUNCTION__ );
			result_recv = pServerBuf->collectData( (void *)&tunerIndex, 
										sizeof( tunerIndex) );
			if( 0 != result_recv )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s -Unexpected data for %d cmd in the buffer %d \n",
					__FUNCTION__, identifyer, result_recv);
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
				pServerBuf->sendResponse(  );
				pPODHALServer->disposeBuf( pServerBuf );			
				break;				
			}				
			memset(&tunerData, 0, sizeof(tunerData) );
			ret = Pace_GetTunerParams( tunerIndex, &tunerData );
			pServerBuf->addResponse( ret );
			pServerBuf->addData( (void *)&tunerData, sizeof( tunerData ) );			
			pServerBuf->sendResponse(  );
			pPODHALServer->disposeBuf( pServerBuf );			
#endif
	  	}
	  	break;
	  case POD_HAL_SERVER_CMD_GET_TUNER_COUNT:
	  	{
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				"%s : IPC called POD_HAL_SERVER_CMD_GET_TUNER_COUNT \n", __FUNCTION__ );
			pServerBuf->addResponse( getTunerCount() );
			pServerBuf->sendResponse(  );
			pPODHALServer->disposeBuf( pServerBuf );				
	  	}
	  	break;
#ifndef ENABLE_TIME_UPDATE	
	  case POD_HAL_SERVER_CMD_CONFIG_TIMEZONE:
	       {
			int32_t timezoneinhours = 0;
			int32_t daylightflag = 0;
			int32_t result_recv = 0;
			uint32_t ret = 0;
			
			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
				"%s : IPC called POD_HAL_SERVER_CMD_CONFIG_TIMEZONE \n", __FUNCTION__ );
			result_recv |= pServerBuf->collectData( (void *)&timezoneinhours, 
										sizeof( timezoneinhours) );
			result_recv = pServerBuf->collectData( (void *)&daylightflag, 
										sizeof( daylightflag) );
			if( 0 != result_recv )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s -Unexpected data for %d cmd in the buffer %d \n",
					__FUNCTION__, identifyer, result_recv);
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
				pServerBuf->sendResponse(  );
				pPODHALServer->disposeBuf( pServerBuf );			
				break;				
			}			
			pServerBuf->addResponse( setTimeZone(timezoneinhours, daylightflag) );
			pServerBuf->sendResponse(  );
			pPODHALServer->disposeBuf( pServerBuf );
	       }
          break;

	  case POD_HAL_SERVER_CMD_CONFIG_TIMEOFDAY:
               {
			int64_t timevalue = 0LL;
			int32_t result_recv = 0;
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				"%s : IPC called POD_HAL_SERVER_CMD_CONFIG_TIMEOFDAY \n", __FUNCTION__ );
			result_recv = pServerBuf->collectData( (void *)&timevalue, 
										sizeof( timevalue) );
			if( 0 != result_recv )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s -Unexpected data for %d cmd in the buffer %d \n",
					__FUNCTION__, identifyer, result_recv);
				pServerBuf->addResponse( RMF_OSAL_ENODATA );
				pServerBuf->sendResponse(  );
				pPODHALServer->disposeBuf( pServerBuf );			
				break;				
			}
			pServerBuf->addResponse( write_time( timevalue ) );
			pServerBuf->sendResponse(  );
			pPODHALServer->disposeBuf( pServerBuf );
               }
	  break;
#endif	
	  default:
	  	{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				"%s : IPC called value is invalid. identifyer: %d\n", __FUNCTION__, identifyer);
			pPODHALServer->disposeBuf( pServerBuf );			
		}
	  	break;;
	}

}

static void podSIMgrEventPollThread( void *data )
{
	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_POLL_SIMGR_EVENT);
	uint32_t ret = RMF_SUCCESS;
	int8_t res = 0;
	uint32_t handle;

	while( isPodMgrUp )
	{
		int32_t result_recv = 0;
		getSIMgrClientHandle( &handle );
		if( handle == 0 )
		{
			/* lost heart beat. So waiting */
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				"%s : lost heart beat. So waiting for 5 seconds \n", __FUNCTION__ );
			sleep(5);
			continue;
		}

		ipcBuf.clearBuf();
		ipcBuf.addData( (const void *) &handle, sizeof( handle));
		
		res = ipcBuf.sendCmd( pod_server_cmd_port_no );
		if ( 0 != res )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
				"%s: Lost connection with POD. res = %d retrying...\n",
					 __FUNCTION__, res );
			continue;
		}	

		result_recv = ipcBuf.getResponse( &ret );
		if( result_recv < 0)
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
				"%s: No response from POD. result_recv = %d retrying...\n",
					 __FUNCTION__, result_recv );	
			continue;
		}
		
		switch (ret )
		{
			case POD_RESP_NO_EVENT:
				{
					RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD", 
						"%s: POD_RESP_NO_EVENT \n",
						 __FUNCTION__);	
				}
				break;
			/*
			case NEW_EVENT:
			{
				ret = rmf_osal_event_create( 
							RMF_OSAL_EVENT_CATEGORY_PODMGR, 
							event_type, 
							&eventData, &event_handle);
				if( RMF_SUCCESS != ret )
				{
					RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
						"rmf_osal_event_create failed in %s:%d\n",
							 __FUNCTION__, __LINE__ );						
				}
				
				ret = rmf_osal_eventmanager_dispatch_event(
									simgr_client_event_manager,
									event_handle);
				if( RMF_SUCCESS != ret )
				{
					RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
						"rmf_osal_eventmanager_dispatch_event failed in %s:%d\n",
							 __FUNCTION__, __LINE__ );						
				}
			}
			break;
			*/
		}
	}
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD","%s::Setting event thread stop condition.\n", __FUNCTION__);
    	rmf_osal_condSet( event_thread_stop_cond );	
	return;
}
static void podEventPollThread( void *data )
{	
	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_POLL_EVENT);
	uint32_t ret = RMF_SUCCESS;
	int8_t res = 0;
	uint32_t handle;

	while( isPodMgrUp )
	{
		int32_t result_recv = 0;
		getPodClientHandle(&handle);
		if( handle == 0 )
		{
			/* lost heart beat. So waiting */
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				"%s : lost heart beat. So waiting for 5 seconds \n", __FUNCTION__ );
			sleep(5);
			continue;
		}

		ipcBuf.clearBuf();
		ipcBuf.addData( (const void *) &handle, sizeof( handle));
		
		res = ipcBuf.sendCmd( pod_server_cmd_port_no );
		if ( 0 != res )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
				"%s: Lost connection with POD. res = %d retrying...\n",
					 __FUNCTION__, res );
			continue;
		}	

		result_recv = ipcBuf.getResponse( &ret );
		if( result_recv < 0)
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
				"%s: No response from POD. result_recv = %d retrying...\n",
					 __FUNCTION__, result_recv );	
			continue;
		}
		
		switch (ret )
		{
			case POD_RESP_NO_EVENT:
				{
					RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.POD", 
						"%s: POD_RESP_NO_EVENT \n",
						 __FUNCTION__);	
				}
				break;
			case POD_RESP_GEN_EVENT:
				{
					int32_t result_recv = 0;
					uint32_t event_type;
					rmf_osal_event_handle_t event_handle;
					rmf_osal_event_params_t eventData = { 0, NULL, 0, NULL };

					RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.POD", 
						"%s: POD_RESP_GEN_EVENT \n",
						 __FUNCTION__);	
					
					result_recv = ipcBuf.collectData( (void *)&event_type, sizeof(event_type) );
					
					RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
						"%s: POD_RESP_GEN_EVENT type is %x\n",
						 __FUNCTION__, event_type);	
					
					if( result_recv < 0)
					{
						RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
							"%s : POD_RESP_GEN_EVENT failed in %d\n",
								 __FUNCTION__, __LINE__ );
						break;
					}

					switch( event_type )
					{
						case RMF_PODMGR_EVENT_DSGIP_ACQUIRED:
						{
							int32_t result_recv = 0;

							RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
								"%s: POD_RESP_GEN_EVENT type is RMF_PODMGR_EVENT_DSGIP_ACQUIRED \n",
								 __FUNCTION__ );

							rmf_osal_memAllocP( RMF_OSAL_MEM_POD, RMF_PODMGR_IP_ADDR_LEN, &( eventData.data ) );
							if( NULL == eventData.data )
							{
								RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
									"rmf_osal_memAllocP failed in %s:%d\n",
										 __FUNCTION__, __LINE__ );
								break;/* inner switch */
							}
							
							result_recv = ipcBuf.collectData( (void *)eventData.data, RMF_PODMGR_IP_ADDR_LEN );	
							if( result_recv != 0 )
							{
								RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
									"collectData failed in %s:%d\n",
										 __FUNCTION__, __LINE__ );
								break;/* inner switch */								
							}		
							RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
								"%s: RMF_PODMGR_EVENT_DSGIP_ACQUIRED %s \n",
								 __FUNCTION__, eventData.data );							
							eventData.free_payload_fn = podClientfreefn;
						}
						break;/* inner switch */					
						default:
						{
							uint32_t data = 0;
							int32_t result_recv = 0;

							result_recv |= ipcBuf.collectData( (void *)&data, sizeof( data) );	
							eventData.data = (void *)data;
							result_recv = ipcBuf.collectData( (void *)&eventData.data_extension, 
														sizeof( eventData.data_extension) );	
							if( result_recv < 0 )
							{
								RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
									"collectData failed in default %s:%d\n",
										 __FUNCTION__, __LINE__ );
								break;/* inner switch */								
							}
						}
						break;							
					}
					
					ret = rmf_osal_event_create( 
							RMF_OSAL_EVENT_CATEGORY_PODMGR, 
							event_type, 
							&eventData, &event_handle);
					if( RMF_SUCCESS != ret )
					{
						RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
							"rmf_osal_event_create failed in %s:%d\n",
								 __FUNCTION__, __LINE__ );						
					}
					
					ret = rmf_osal_eventmanager_dispatch_event(
										pod_client_event_manager,
										event_handle);
					if( RMF_SUCCESS != ret )
					{
						RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
							"rmf_osal_eventmanager_dispatch_event failed in %s:%d\n",
								 __FUNCTION__, __LINE__ );						
					}	
				}
				break;
				/* not propogating, but need to handle it  */
			case POD_RESP_CORE_EVENT:
				{
					int32_t result_recv = 0;
					uint32_t event_type;
					rmf_osal_event_handle_t event_handle;
					rmf_osal_event_params_t eventData = { 0, NULL, 0, NULL };					
					uint8_t data_present = 0;
					uint32_t retVal = 0;
					
					RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.POD", 
						"%s: POD_RESP_CORE_EVENT \n",
						 __FUNCTION__);
					
					result_recv = ipcBuf.collectData( (void *)&event_type, sizeof(event_type) );
					
					if( result_recv < 0)
					{
						RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
							"%s : POD_RESP_CORE_EVENT failed in %d\n",
								 __FUNCTION__, __LINE__ );
						break;
					}

					RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
						"%s: POD_RESP_CORE_EVENT type is %x\n",
						 __FUNCTION__, event_type);	

					result_recv |= ipcBuf.collectData( (void *)&eventData.data_extension, 
												sizeof( eventData.data_extension) );
					result_recv = ipcBuf.collectData( (void *)&data_present, 
												sizeof( data_present) );
					if( data_present )
					{
						retVal = rmf_osal_memAllocP( RMF_OSAL_MEM_POD, eventData.data_extension,
								( void ** ) &eventData.data );

						if ( RMF_SUCCESS != retVal )
						{
							RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
								"%s: NO Memory \n",
								 __FUNCTION__);
						}
						result_recv = ipcBuf.collectData( (void *)eventData.data, eventData.data_extension );
						rmf_osal_memFreeP( RMF_OSAL_MEM_POD, eventData.data );
					}
					if( result_recv != 0 )
					{
						RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
							"collectData failed in POD_RESP_CORE_EVENT %s:%d\n",
								 __FUNCTION__, __LINE__ );
						break;/* inner switch */								
					}		
				}
				break;
				
			case POD_RESP_DECRYPT_EVENT:
				{
					int32_t result_recv = 0;
					uint32_t event_type;

					RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
						"%s: POD_RESP_DECRYPT_EVENT \n",
						 __FUNCTION__);	
					
					result_recv = ipcBuf.collectData( (void *)&event_type, sizeof(event_type) );
					
					RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
						"%s: POD_RESP_DECRYPT_EVENT type is %x\n",
						 __FUNCTION__, event_type);	
					
					if( result_recv < 0)
					{
						RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
							"%s : POD_RESP_DECRYPT_EVENT failed in %d\n",
								 __FUNCTION__, __LINE__ );
						break;
					}

					switch( event_type )
					{
						case RMF_PODMGR_DECRYPT_EVENT_CANNOT_DESCRAMBLE_ENTITLEMENT:
						case RMF_PODMGR_DECRYPT_EVENT_CANNOT_DESCRAMBLE_RESOURCES:
						case RMF_PODMGR_DECRYPT_EVENT_MMI_PURCHASE_DIALOG:
						case RMF_PODMGR_DECRYPT_EVENT_MMI_TECHNICAL_DIALOG:
						case RMF_PODMGR_DECRYPT_EVENT_FULLY_AUTHORIZED:
						case RMF_PODMGR_DECRYPT_EVENT_SESSION_SHUTDOWN:
						case RMF_PODMGR_DECRYPT_EVENT_POD_REMOVED:
						case RMF_PODMGR_DECRYPT_EVENT_RESOURCE_LOST:
						case RMF_PODMGR_DECRYPT_EVENT_CCI_STATUS:
						{
							int32_t result_recv = 0;
							uint32_t sessionHandle = 0;
							uint32_t CCI = 0;
							uint32_t qamContext = 0;
							
							result_recv |= ipcBuf.collectData( (void *)&CCI, sizeof( CCI) );	
							result_recv |= ipcBuf.collectData( (void *)&sessionHandle, 
														sizeof( sessionHandle) );	
							result_recv = ipcBuf.collectData( (void *)&qamContext, 
														sizeof( qamContext ) );	
							if( result_recv != 0 )
							{
								RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
									"collectData failed in %s:%d\n",
										 __FUNCTION__, __LINE__ );
								break;/* inner switch */								
							}
							notifyCCIChange(qamContext, sessionHandle, CCI, (rmf_PODMgrEvent)event_type);
						}
						break;
					}					
				}
				break;

#ifdef ENABLE_TIME_UPDATE
			case POD_RESP_TIMEZONE_SET_EVENT:
			{
				int32_t timezoneinhours = 0;
				int32_t daylightflag = 0;
				int32_t result_recv = 0;
				uint32_t ret = 0;

				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
						"%s : IPC called POD_RESP_TIMEZONE_SET_EVENT \n", __FUNCTION__ );
				result_recv |= ipcBuf.collectData( (void *)&timezoneinhours,
						sizeof( timezoneinhours) );
				result_recv = ipcBuf.collectData( (void *)&daylightflag,
						sizeof( daylightflag) );
				if( result_recv != 0 )
				{
					RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
						"collectData failed in %s:%d\n",
							 __FUNCTION__, __LINE__ );
					break;/* inner switch */								
				}				
				setTimeZone(timezoneinhours, daylightflag);
			}
			break;

			case POD_RESP_SYSTIME_SET_EVENT:
			{
				int64_t timevalue = 0LL;
				int32_t result_recv = 0;
				RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
						"%s : IPC called POD_RESP_SYSTIME_SET_EVENT \n", __FUNCTION__ );
				result_recv = ipcBuf.collectData( (void *)&timevalue,
						sizeof( timevalue) );
				if( result_recv != 0 )
				{
					RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
						"collectData failed in %s:%d\n",
							 __FUNCTION__, __LINE__ );
					break;/* inner switch */								
				}				
				write_time( timevalue );

			}
			break;

#endif
			default:
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
					"%s: Invalid event received \n",
						 __FUNCTION__);						
				break;
				
		}

		RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.POD", 
			"%s: POD Client Event monitoring is live \n",
				 __FUNCTION__);
	}
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD","%s::while break and setting event thread stop condition.\n", __FUNCTION__);
    rmf_osal_condSet( event_thread_stop_cond );
	return;
}

rmf_Error addToDecryptQList( 	uint32_t sessionHandle,
								rmf_osal_eventqueue_handle_t m_queue )
{
	decryptQ *newEntry;
	rmf_Error err;

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
			 "addToDecryptQList - queue 0x%x\n",
			 m_queue );

	// Allocate a new entry
	err =
		rmf_osal_memAllocP( RMF_OSAL_MEM_POD, sizeof( decryptQ ),
							( void ** ) &newEntry );

	if ( RMF_SUCCESS != err )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "addToDecryptQList failure, queue %x, %d\n",
				 m_queue, err );
		return err;
	}

	newEntry->m_queue = m_queue;
	newEntry->sessionHandle = sessionHandle;

	// Add it to the list.	In front is OK.
	rmf_osal_mutexAcquire( podDecryptQueueListMutex );
	newEntry->next = podDecryptQueueList;
	podDecryptQueueList = newEntry;
	rmf_osal_mutexRelease( podDecryptQueueListMutex );

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
			 "addToDecryptQList podImplEventQueueList=0x%p\n",
			 podDecryptQueueList );

	return RMF_SUCCESS;

}

rmf_Error removeFromDecryptQList( uint32_t sessionHandle )
{
	decryptQ **p;
	decryptQ *save;
	rmf_Error err = RMF_OSAL_EINVAL;

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
			 "removeFromDecryptQList - sessionHandle %x\n",
			 sessionHandle );

	// Lock the list for modification
	rmf_osal_mutexAcquire( podDecryptQueueListMutex );

	// Find the entry with this ID
	for ( p = &podDecryptQueueList; NULL != *p; p = &( ( *p )->next ) )
	{
		// Find it?
		if ( ( *p )->sessionHandle == sessionHandle )
		{
			// Remove it and exit
			// Save it
			save = *p;
			// Reset the current pointer to skip it.
			( *p ) = ( *p )->next;

			// Free the node
			rmf_osal_memFreeP( RMF_OSAL_MEM_POD, save );

			// Set up the successful return
			err = RMF_SUCCESS;
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
				"%s success\n", __FUNCTION__ );
			// Drop out of the loop
			break;
		}
	}

	// Release the structure
	rmf_osal_mutexRelease( podDecryptQueueListMutex );

	return err;
}

static void FreeEventDataFn(void *pData)
{
    if(pData)
    {
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
			 "notifyCCIChange - freeing event Data, qamhndl = %d\n",
			 ((qamEventData_t *)pData)->qamContext );
        free(pData);
		pData = NULL;
    }
}

rmf_Error notifyCCIChange( uint32_t qamContext, uint32_t sessionHandle, uint32_t CCI, rmf_PODMgrEvent event )
{
	decryptQ **p;
	rmf_Error err = RMF_OSAL_EINVAL;

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
			 "notifyCCIChange - sessionHandle %x, qamhndl = %d\n",
			 sessionHandle, qamContext );

	qamEventData_t *eventData = NULL;
	eventData = (qamEventData_t *)malloc(sizeof(qamEventData_t));
	if(eventData == NULL)
    {
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
							"%s eventData memory allocation failed. \n", __FUNCTION__ );
		return err;
    }
    eventData->qamContext = qamContext;
    eventData->handle = sessionHandle;
    eventData->cci = CCI;


	rmf_osal_event_handle_t event_handle;
	rmf_osal_event_params_t event_params = { 0,
											( void * ) eventData,
											( uint32_t ) sessionHandle,
											FreeEventDataFn
											};
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
					"notifyCCIChange() Event=0x%x handle=0x%p\n",
					event, sessionHandle );
	err = rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_POD, 
									event,
									&event_params, &event_handle );
	if ( RMF_SUCCESS != err )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
							"%s rmf_osal_event_create failed \n", __FUNCTION__, err );
	}

	err = rmf_osal_eventqueue_add( g_qamsrc_eventq, event_handle );
	if ( RMF_SUCCESS != err )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
					"%s rmf_osal_eventqueue_add failed %d\n", __FUNCTION__, err );
	}
			
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
				"%s dispatched event to decryptQ %d\n", __FUNCTION__, err );
#if 0
	// Lock the list for modification
	rmf_osal_mutexAcquire( podDecryptQueueListMutex );

	// Find the entry with this ID
	for ( p = &podDecryptQueueList; NULL != *p; p = &( ( *p )->next ) )
	{
		// Find it?
		if ( ( *p )->sessionHandle == sessionHandle )
		{
			rmf_osal_event_handle_t event_handle;
			rmf_osal_event_params_t event_params = { 0,
													( void * ) CCI,
													( uint32_t ) sessionHandle,
													NULL
													};

			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
					"notifyCCIChange() Event=0x%x handle=0x%p\n",
					event, sessionHandle );
			err =
					rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_POD, 
											event,
											&event_params, &event_handle );
			if ( RMF_SUCCESS != err )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
							"%s rmf_osal_event_create failed \n", __FUNCTION__, err );
				break;
			}

			err = rmf_osal_eventqueue_add( ( *p )->m_queue, event_handle );
			if ( RMF_SUCCESS != err )
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
					"%s rmf_osal_eventqueue_add failed %d\n", __FUNCTION__, err );
			}
			
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
				"%s dispatched event to decryptQ %d\n", __FUNCTION__, err );
			
			// Drop out of the loop
			break;
		}
	}

	// Release the structure
	rmf_osal_mutexRelease( podDecryptQueueListMutex );
#endif

	return err;
}

void podmgrShutdown( )
{
	decryptQ *p;

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", "%s: Entered\n", __FUNCTION__ );
	rmf_osal_mutexAcquire( podDecryptQueueListMutex );

	// Find the entry with this ID
	while ( p = podDecryptQueueList )
	{
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", "%s: Stopping decrypt: %x \n", 
			__FUNCTION__, p->sessionHandle );		
		rmf_podmgrStopDecrypt( p->sessionHandle);
	}
	rmf_osal_mutexRelease( podDecryptQueueListMutex );
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", "%s: Exited\n", __FUNCTION__ );
}

void rmf_podmgrUnInit(void)
{
	rmf_Error ret = RMF_SUCCESS;
	decryptQ*p;
	decryptQ *save;	
	
    podmgrShutdown();
    isPodMgrUp = 0;
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD","%s::waiting for hbmon_thread_stop_cond...\n", __FUNCTION__);
    rmf_osal_condWaitFor( hbmon_thread_stop_cond, 10*1000 );
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD","%s::Both HBMonitor and EventPoll thread stopped...\n", __FUNCTION__);
    rmf_osal_condDelete( hbmon_thread_stop_cond );
    rmf_osal_condDelete( event_thread_stop_cond );
	
	rmf_osal_memFreeP( RMF_OSAL_MEM_POD, pEIDStatus );
	pEIDStatus = NULL;

	ret = rmf_osal_eventmanager_delete(pod_client_event_manager);
	if (RMF_SUCCESS != ret)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD",
						  "rmf_osal_eventmanager_delete() failed\n");
		return;
	}	
	
	ret = rmf_osal_eventmanager_delete(simgr_client_event_manager);
	if (RMF_SUCCESS != ret)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD",
						  "simgr_client_event_manager delete failed\n");
		return;
	}
	ret = pPODHALServer->stop(	);

	if ( 0 != ret )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s : Server failed to stop \n",
				 __FUNCTION__ );	}

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
			 "%s : Server server1 stoped successfully\n", __FUNCTION__ );

	if ( podDecryptQueueListMutex == NULL )
		return;

	rmf_osal_mutexAcquire( podDecryptQueueListMutex );

	// Shutdown the queues and delete the entries
	p = podDecryptQueueList;

	while ( NULL != p )
	{
		save = p;
		p = p->next;
		rmf_osal_memFreeP( RMF_OSAL_MEM_POD, save );
	}

	// Release and kill the mutex
	rmf_osal_mutexRelease( podDecryptQueueListMutex );
	rmf_osal_mutexDelete( podDecryptQueueListMutex );	
	rmf_osal_mutexDelete( podSIMgrHandleMutex );
	rmf_osal_mutexDelete( podHandleMutex );		
}
#endif 


rmf_Error parseFilteredDescriptors( rmf_SiMpeg2DescriptorList * pMpeg2DescList,
							rmf_SiElementaryStreamList * pESList,
							parsedDescriptors * pParseOutput )
{
	rmf_SiMpeg2DescriptorList *pDesc_list_walker = pMpeg2DescList;
	rmf_SiElementaryStreamList *pES_list_walker = pESList;
	uint16_t ca_system_id = 0;
	rmf_osal_Bool bAddESInfo = FALSE;
	rmf_Error ret = RMF_SUCCESS;
	uint32_t noES = 0;

	ret = getCASystemId(&ca_system_id);
	if( RMF_SUCCESS != ret )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s: getCASystemId failed \n", __FUNCTION__ );
		return ret;
	}

	while ( pDesc_list_walker )
	{
		if ( ( RMF_SI_DESC_CA_DESCRIPTOR == pDesc_list_walker->tag )
			 && ( pDesc_list_walker->descriptor_length > 0 ) )
		{
			uint16_t cas_id =
				( ( uint8_t * ) pDesc_list_walker->descriptor_content )[0] << 8 | 
				( ( uint8_t * ) pDesc_list_walker->descriptor_content )[1];

			if ( !pParseOutput->ecmPidFound )
			{
				pParseOutput->ecmPid =
					( ( uint8_t * ) pDesc_list_walker->descriptor_content )[2] << 8 | 
					( ( uint8_t * ) pDesc_list_walker->descriptor_content )[3];

				pParseOutput->ecmPid &= 0x1FFF;
				pParseOutput->ecmPidFound = TRUE;
			}

			RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.POD",
					 "CheckCADescriptors - ecmPid: 0x%x ca_system_id: 0x%x\n",
					 pParseOutput->ecmPid, ca_system_id );

			if ( cas_id == ca_system_id )
			{
				pParseOutput->caDescFound = TRUE;

				pParseOutput->ProgramInfoLen += sizeof( uint8_t );		// byte for tag
				pParseOutput->ProgramInfoLen += sizeof( uint8_t );		// byte for descriptor content length
				pParseOutput->ProgramInfoLen += pDesc_list_walker->descriptor_length;	// descriptor content
				break;
			}

			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
					 "Found descriptor tag %d (len %d) system_id: %d\n",
					 pDesc_list_walker->tag,
					 pDesc_list_walker->descriptor_length, cas_id );
		}
		pDesc_list_walker = pDesc_list_walker->next;
	}

	if ( pParseOutput->ProgramInfoLen >= 4096 )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s: Returning : pParseOutput->ProgramInfoLen >= 4096 \n", __FUNCTION__ );
		return RMF_OSAL_ENODATA;
	}

	memset( pParseOutput->aComps, 0, sizeof( pParseOutput->aComps ) );

	while ( pES_list_walker )
	{
		bAddESInfo = FALSE;
		pDesc_list_walker = pES_list_walker->descriptors;

		while ( pDesc_list_walker )
		{
			if ( RMF_SI_DESC_CA_DESCRIPTOR == pDesc_list_walker->tag )
			{
				uint16_t cas_id =
					( ( uint8_t * ) pDesc_list_walker->descriptor_content )[0] << 8 | 
					( ( uint8_t * ) pDesc_list_walker->descriptor_content )[1];

				if ( !pParseOutput->ecmPidFound )
				{
					pParseOutput->ecmPid =
						( ( uint8_t * ) pDesc_list_walker->descriptor_content )[2] << 8 | 
						( ( uint8_t * ) pDesc_list_walker->descriptor_content )[3];

					pParseOutput->ecmPid &= 0x1FFF;
					pParseOutput->ecmPidFound = TRUE;

					RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.POD",
							 "CheckCADescriptors - ecmPid: 0x%x\n",
							 pParseOutput->ecmPid );
				}

				// Match the CA system id
				if ( cas_id == ca_system_id )
				{
					pParseOutput->caDescFound = TRUE;
					bAddESInfo = TRUE;
					if ( pDesc_list_walker->descriptor_length > 0 )
					{
						/* 1 byte for the descriptor tag, 1 byte for the length field
						 * plus the rest of the descriptor
						 */
						pParseOutput->aComps[noES] +=
							1 + 1 + pDesc_list_walker->descriptor_length;
						/* Standard code deviation: ext: X1 start */
#ifdef CMCST_EXT_X1
						// 1 byte for the ca_pmt_cmd_id
						pParseOutput->aComps[noES] += 1;
#endif
						/* Standard code deviation: ext: X1 end */
					}
					break;
				}
			}
			pDesc_list_walker = pDesc_list_walker->next;
		}

		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
		 "%s: checking elementary_stream_type=  0x%x, elementary_PID =  0x%x\n",
		 __FUNCTION__,pES_list_walker->elementary_stream_type, pES_list_walker->elementary_PID );
	
		
		switch( pES_list_walker->elementary_stream_type )
		{
			case RMF_SI_ELEM_MPEG_1_VIDEO:
			case RMF_SI_ELEM_MPEG_2_VIDEO:
			case RMF_SI_ELEM_ISO_14496_VISUAL:
			case RMF_SI_ELEM_AVC_VIDEO:
			case RMF_SI_ELEM_VIDEO_DCII:
			case RMF_SI_ELEM_MPEG_1_AUDIO:
			case RMF_SI_ELEM_MPEG_2_AUDIO:
			case RMF_SI_ELEM_AAC_ADTS_AUDIO:
			case RMF_SI_ELEM_AAC_AUDIO_LATM:
			case RMF_SI_ELEM_ATSC_AUDIO:
			case RMF_SI_ELEM_ENHANCED_ATSC_AUDIO:
			case RMF_SI_ELEM_ETV_SCTE_SIGNALING:
			case RMF_SI_ELEM_MPEG_PRIVATE_SECTION:
			case RMF_SI_ELEM_SCTE_35:
			case RMF_SI_ELEM_HEVC_VIDEO:
				bAddESInfo = TRUE;
				RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
						"Added elementary_stream_type=  0x%x elementary_PID = 0x%x \n",
						pES_list_walker->elementary_stream_type, pES_list_walker->elementary_PID );

				break;
		}
		
		if( bAddESInfo )
		{
			pParseOutput->streamInfoLen += 5;
			/* Standard code deviation: ext: X1 start */
#ifdef CMCST_EXT_X1
			//comp_es_length[noES] += 1;
#else
			// 1 byte for the ca_pmt_cmd_id
			pParseOutput->aComps[noES] += 1;
#endif
			/* Standard code deviation: ext: X1 end */

			pParseOutput->streamInfoLen += pParseOutput->aComps[noES];
			noES++;
			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
				 "noES=  0x%d\n",
				 noES );
		}
		pES_list_walker = pES_list_walker->next;
	}

	if ( noES > 512 )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s: Returning : noES > 512 \n", __FUNCTION__ );
		return RMF_OSAL_EINVAL;
	}

	pParseOutput->noES = noES;

	return RMF_SUCCESS;

}

rmf_Error allocateFilteredDescInfo( parsedDescriptors * pParseOutput,         rmf_SiElementaryStreamList *pESList,
        rmf_SiMpeg2DescriptorList *pMpeg2DescList,
							uint8_t caPMTCMDId )
{
	rmf_SiMpeg2DescriptorList *pDesc_list_walker = NULL;
	rmf_SiElementaryStreamList *pES_list_walker = NULL;
	uint8_t *pTempProgramInfo = NULL;
	uint8_t *pTempStreamInfo = NULL;
	rmf_osal_Bool bAddESInfo = FALSE;	
	rmf_osal_Bool bAddDSInfo = FALSE;
	uint8_t *pTempESIndex = NULL;
	
	uint16_t ca_system_id = 0;

	rmf_Error ret = RMF_SUCCESS;
	uint32_t noES = 0;

	ret = getCASystemId(&ca_system_id);
	if( RMF_SUCCESS != ret )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s: getCASystemId failed \n", __FUNCTION__ );
		return ret;
	}

	if ( pParseOutput->ProgramInfoLen > 0 )
	{
		ret =
			rmf_osal_memAllocP( RMF_OSAL_MEM_POD,
								pParseOutput->ProgramInfoLen,
								( void ** ) &pParseOutput->pProgramInfo );
		if ( RMF_SUCCESS != ret )
		{
                        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD",
                                                  "%s() malloc failed\n",__FUNCTION__);
			return RMF_OSAL_ENOMEM;
		}

		pDesc_list_walker = pMpeg2DescList;
		pTempProgramInfo = pParseOutput->pProgramInfo;

		while ( pDesc_list_walker )
		{
			if ( ( RMF_SI_DESC_CA_DESCRIPTOR == pDesc_list_walker->tag )
				 && ( pDesc_list_walker->descriptor_length > 0 ) )
			{
				uint16_t cas_id =
					( ( uint8_t * ) pDesc_list_walker->descriptor_content )[0] << 8 | 
					( ( uint8_t * ) pDesc_list_walker->descriptor_content )[1];

				// Match the CA system id
				if ( cas_id == ca_system_id )
				{
					*pTempProgramInfo++ = pDesc_list_walker->tag;
					*pTempProgramInfo++ =
						pDesc_list_walker->descriptor_length;

					rmf_osal_memcpy( pTempProgramInfo,
							pDesc_list_walker->descriptor_content,
							pDesc_list_walker->descriptor_length, 
							( pParseOutput->ProgramInfoLen - (pTempProgramInfo - pParseOutput->pProgramInfo) ),
							pDesc_list_walker->descriptor_length
							);

					RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
							 "CA Tag: %d, CA Desc len: %d bytes, CA PID: 0x%x cas_id:%d\n",
							 pDesc_list_walker->tag,
							 pDesc_list_walker->descriptor_length + 2,
							 *pTempProgramInfo, cas_id );

					pTempProgramInfo += pDesc_list_walker->descriptor_length;
					break;
				}

			}

			pDesc_list_walker = pDesc_list_walker->next;
		}						/* end of descriptor parsing  */
	}
	/* done with outer descriptor loop	*/

	if ( pParseOutput->streamInfoLen > 0 )
	{
			// allocate storage based on what was calculated as the overall size required
			ret =
				rmf_osal_memAllocP( RMF_OSAL_MEM_POD,
									pParseOutput->streamInfoLen,
									( void ** ) &pParseOutput->pStreamInfo );
			if ( RMF_SUCCESS != ret )
			{
                                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD",
                                                  "%s() malloc failed\n",__FUNCTION__);
				return RMF_OSAL_ENOMEM;
			}
	}

	if ( pParseOutput->noES > 0 )
	{
		ret =
			rmf_osal_memAllocP( RMF_OSAL_MEM_POD,
								( pParseOutput->noES *
								  sizeof( rmf_PodmgrStreamDecryptInfo ) ),
								( void ** ) &pParseOutput->pESInfo );
		if ( RMF_SUCCESS != ret )
		{
                        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD",
                                                  "%s() malloc failed\n",__FUNCTION__);
			return RMF_OSAL_ENOMEM;
		}
	}

	pES_list_walker = pESList;
	pTempStreamInfo = pParseOutput->pStreamInfo;

	noES = 0;
	// Walk inner descriptors
	while ( pES_list_walker )
	{
		bAddESInfo = FALSE;
		if ( pParseOutput->pStreamInfo )
		{
			pTempESIndex = pTempStreamInfo;
//			if ( pParseOutput->aComps[noES] > 0 )
			{

				/* Standard code deviation: ext: X1 start */
#ifdef CMCST_EXT_X1
				//*pTempStreamInfo++ = caPMTCMDId;
#else
				/* Skipping for the ES information of this stream */
				pTempStreamInfo += 5;
				pParseOutput->CAPMTCmdIdIndex = pTempStreamInfo - pParseOutput->pStreamInfo;
				*pTempStreamInfo++ = caPMTCMDId;
#endif			
				pDesc_list_walker = pES_list_walker->descriptors;
				while ( pDesc_list_walker )
				{
					if ( ( RMF_SI_DESC_CA_DESCRIPTOR ==
						   pDesc_list_walker->tag )
						 && ( pDesc_list_walker->descriptor_length > 0 ) )
					{
						uint16_t cas_id = 
								( ( uint8_t * ) pDesc_list_walker->descriptor_content )[0] << 8 | 
								( ( uint8_t * ) pDesc_list_walker->descriptor_content )[1];

						// Match the CA system id
						if ( cas_id == ca_system_id )
						{				
							/* Standard code deviation: ext: X1 start */
#ifdef CMCST_EXT_X1
							/* Skipping for the ES information of this stream */
							pTempStreamInfo += 5;
							pParseOutput->CAPMTCmdIdIndex = pTempStreamInfo - pParseOutput->pStreamInfo;
							// updated MPE POD implementation 							
							*pTempStreamInfo++ = caPMTCMDId;
#endif
							/* Standard code deviation: ext: X1 end */
							*pTempStreamInfo++ = pDesc_list_walker->tag;
							*pTempStreamInfo++ =
								pDesc_list_walker->descriptor_length;

							uint16_t cas_pid =
								( ( ( uint8_t * ) pDesc_list_walker->
									descriptor_content )[2] ) << 8 |
								( ( uint8_t * ) pDesc_list_walker->
								  descriptor_content )[3];
							RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
									 "CA SYSTEM ID: 0x%x(%d), CA PID: 0x%x(%d) PAYLOAD: %d bytes, total CA descriptor size: %d bytes.\n",
									 cas_id, cas_id, cas_pid, cas_pid,
									 pDesc_list_walker->descriptor_length - 4,
									 pDesc_list_walker->descriptor_length );

							*pTempStreamInfo++ =
								( ( uint8_t * ) pDesc_list_walker->descriptor_content )[0];
							*pTempStreamInfo++ =
								( ( uint8_t * ) pDesc_list_walker->descriptor_content )[1];
							*pTempStreamInfo++ =
								( ( uint8_t * ) pDesc_list_walker->descriptor_content )[2];
							*pTempStreamInfo++ =
								( ( uint8_t * ) pDesc_list_walker->descriptor_content )[3];

							rmf_osal_memcpy( pTempStreamInfo,
									&( ( uint8_t * ) pDesc_list_walker->descriptor_content )[4],
									pDesc_list_walker->descriptor_length - 4,
									(pParseOutput->streamInfoLen - (pTempStreamInfo- pParseOutput->pStreamInfo)),
									pDesc_list_walker->descriptor_length - 4);

							pTempStreamInfo +=
								pDesc_list_walker->descriptor_length - 4;
							bAddDSInfo = TRUE;
							break;
						}
					}

					pDesc_list_walker = pDesc_list_walker->next;
				}
			}
		}
		else
		{						
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					 "Test:  pParseOutput->pStreamInfo is NULL !! " );
		}

		RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
		 "%s: checking elementary_stream_type=  0x%x, elementary_PID =  0x%x\n", 
		 __FUNCTION__,pES_list_walker->elementary_stream_type, pES_list_walker->elementary_PID );
		
		
		switch( pES_list_walker->elementary_stream_type )
		{
			case RMF_SI_ELEM_MPEG_1_VIDEO:
			case RMF_SI_ELEM_MPEG_2_VIDEO:
			case RMF_SI_ELEM_ISO_14496_VISUAL:
			case RMF_SI_ELEM_AVC_VIDEO:
			case RMF_SI_ELEM_VIDEO_DCII:
			case RMF_SI_ELEM_MPEG_1_AUDIO:
			case RMF_SI_ELEM_MPEG_2_AUDIO:
			case RMF_SI_ELEM_AAC_ADTS_AUDIO:
			case RMF_SI_ELEM_AAC_AUDIO_LATM:
			case RMF_SI_ELEM_ATSC_AUDIO:
			case RMF_SI_ELEM_ENHANCED_ATSC_AUDIO:
			case RMF_SI_ELEM_ETV_SCTE_SIGNALING:
			case RMF_SI_ELEM_MPEG_PRIVATE_SECTION:
			case RMF_SI_ELEM_SCTE_35:
			case RMF_SI_ELEM_HEVC_VIDEO:

				bAddESInfo = TRUE;
				break;
		}
		if( ( bAddESInfo || bAddDSInfo ) && ( pParseOutput->pStreamInfo ) )
		{
			*pTempESIndex++ =
				( uint8_t ) pES_list_walker->elementary_stream_type;
			*pTempESIndex++ = ( pES_list_walker->elementary_PID >> 8 ) & 0x1F;		// Top 3 bits are reserved
			*pTempESIndex++ = pES_list_walker->elementary_PID & 0xFF;
			*pTempESIndex++ = ( pParseOutput->aComps[noES] >> 8 ) & 0xF;		// top 4 bits are reserved
			*pTempESIndex++ = pParseOutput->aComps[noES] & 0xFF;

			pParseOutput->pESInfo[noES].pid =
				pES_list_walker->elementary_PID;
			pParseOutput->pESInfo[noES].status = 0;	// CA_ENABLE_NO_VALUE
			if( FALSE == bAddDSInfo )
			{
				pTempStreamInfo = pTempESIndex;
			}
			noES++;

			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.SI",
					 "CheckCADescriptors - pid: %d ca_status: %d stream type: 0x%x noES: %d\n",
					 pES_list_walker->elementary_PID,
					 pParseOutput->pESInfo[noES].status,
					 pES_list_walker->elementary_stream_type, noES );
		}
		pES_list_walker = pES_list_walker->next;
	}

	return RMF_SUCCESS;
}



rmf_Error parseDescriptors( rmf_SiMpeg2DescriptorList * pMpeg2DescList,
							rmf_SiElementaryStreamList * pESList,
							parsedDescriptors * pParseOutput )
{
	rmf_SiMpeg2DescriptorList *pDesc_list_walker = pMpeg2DescList;
	rmf_SiElementaryStreamList *pES_list_walker = pESList;
	uint16_t ca_system_id = 0;

	rmf_Error ret = RMF_SUCCESS;
	uint32_t noES = 0;

	ret = getCASystemId(&ca_system_id);
	if( RMF_SUCCESS != ret )
	{
		return ret;
	}

	while ( pDesc_list_walker )
	{
		if ( ( RMF_SI_DESC_CA_DESCRIPTOR == pDesc_list_walker->tag )
			 && ( pDesc_list_walker->descriptor_length > 0 ) )
		{
			uint16_t cas_id =
				( ( uint8_t * ) pDesc_list_walker->descriptor_content )[0] << 8 | 
				( ( uint8_t * ) pDesc_list_walker->descriptor_content )[1];

			if ( !pParseOutput->ecmPidFound )
			{
				pParseOutput->ecmPid =
					( ( uint8_t * ) pDesc_list_walker->descriptor_content )[2] << 8 | 
					( ( uint8_t * ) pDesc_list_walker->descriptor_content )[3];

				pParseOutput->ecmPid &= 0x1FFF;
				pParseOutput->ecmPidFound = TRUE;
			}

			RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.POD",
					 "CheckCADescriptors - ecmPid: 0x%x\n",
					 pParseOutput->ecmPid );
			// Match the CA system id
			RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.POD",
					 "local  - ca_system_id: 0x%x\n",
					 ca_system_id );
			if ( cas_id == ca_system_id )
			{
				pParseOutput->caDescFound = TRUE;


				pParseOutput->ProgramInfoLen += sizeof( uint8_t );		// byte for tag
				pParseOutput->ProgramInfoLen += sizeof( uint8_t );		// byte for descriptor content length
				pParseOutput->ProgramInfoLen += pDesc_list_walker->descriptor_length;	// descriptor content
			}

			RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
					 "Found descriptor tag %d (len %d) system_id: %d\n",
					 pDesc_list_walker->tag,
					 pDesc_list_walker->descriptor_length, cas_id );
		}
		pDesc_list_walker = pDesc_list_walker->next;
	}

	if ( pParseOutput->ProgramInfoLen >= 4096 )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "Returning : pParseOutput->ProgramInfoLen  \n" );		
		return RMF_OSAL_ENODATA;
	}

	memset( pParseOutput->aComps, 0, sizeof( pParseOutput->aComps ) );

	while ( pES_list_walker )
	{
		// stream_type(1 byte) + elementary_pid(2 bytes) + ES_info_length(2 bytes)
		pParseOutput->streamInfoLen += 5;
		pDesc_list_walker = pES_list_walker->descriptors;

		while ( pDesc_list_walker )
		{
			if ( RMF_SI_DESC_CA_DESCRIPTOR == pDesc_list_walker->tag )
			{


				uint16_t cas_id =
					( ( uint8_t * ) pDesc_list_walker->descriptor_content )[0] << 8 | 
					( ( uint8_t * ) pDesc_list_walker->descriptor_content )[1];

				if ( !pParseOutput->ecmPidFound )
				{
					pParseOutput->ecmPid =
						( ( uint8_t * ) pDesc_list_walker->descriptor_content )[2] << 8 | 
						( ( uint8_t * ) pDesc_list_walker->descriptor_content )[3];

					pParseOutput->ecmPid &= 0x1FFF;
					pParseOutput->ecmPidFound = TRUE;

					RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.POD",
							 "CheckCADescriptors - ecmPid: 0x%x\n",
							 pParseOutput->ecmPid );
				}

				// Match the CA system id
				if ( cas_id == ca_system_id )
				{
					pParseOutput->caDescFound = TRUE;

					if ( pDesc_list_walker->descriptor_length > 0 )
					{
						/* 1 byte for the descriptor tag, 1 byte for the length field
						 * plus the rest of the descriptor
						 */
						pParseOutput->aComps[noES] +=
							1 + 1 + pDesc_list_walker->descriptor_length;
						/* Standard code deviation: ext: X1 start */
#ifdef CMCST_EXT_X1
						// 1 byte for the ca_pmt_cmd_id
						pParseOutput->aComps[noES] += 1;
#endif
						/* Standard code deviation: ext: X1 end */
					}
				}
			}
			pDesc_list_walker = pDesc_list_walker->next;
		}

		/* Standard code deviation: ext: X1 start */
#ifdef CMCST_EXT_X1
		//comp_es_length[noES] += 1;
#else
		// 1 byte for the ca_pmt_cmd_id
		pParseOutput->aComps[noES] += 1;
#endif
		/* Standard code deviation: ext: X1 end */

		pParseOutput->streamInfoLen += pParseOutput->aComps[noES];
		pES_list_walker = pES_list_walker->next;
		noES++;
	}

	if ( noES > 512 )
	{
		return RMF_OSAL_EINVAL;
	}

	pParseOutput->noES = noES;

	return RMF_SUCCESS;

}

rmf_Error allocateDescInfo( parsedDescriptors * pParseOutput,         rmf_SiElementaryStreamList *pESList,
        rmf_SiMpeg2DescriptorList *pMpeg2DescList,
							uint8_t caPMTCMDId )
{
	rmf_SiMpeg2DescriptorList *pDesc_list_walker = NULL;
	rmf_SiElementaryStreamList *pES_list_walker = NULL;
	uint8_t *pTempProgramInfo = NULL;
	uint8_t *pTempStreamInfo = NULL;
	uint16_t ca_system_id = 0;

	rmf_Error ret = RMF_SUCCESS;
	uint32_t noES = 0;

	ret = getCASystemId(&ca_system_id);
	if( RMF_SUCCESS != ret )
	{
		return ret;
	}

	if ( pParseOutput->ProgramInfoLen > 0 )
	{
		ret =
			rmf_osal_memAllocP( RMF_OSAL_MEM_POD,
								pParseOutput->ProgramInfoLen,
								( void ** ) &pParseOutput->pProgramInfo );
		if ( RMF_SUCCESS != ret )
		{
			return RMF_OSAL_ENOMEM;
		}

		pDesc_list_walker = pMpeg2DescList;
		pTempProgramInfo = pParseOutput->pProgramInfo;

		while ( pDesc_list_walker )
		{
			if ( ( RMF_SI_DESC_CA_DESCRIPTOR == pDesc_list_walker->tag )
				 && ( pDesc_list_walker->descriptor_length > 0 ) )
			{
				uint16_t cas_id =
					( ( uint8_t * ) pDesc_list_walker->descriptor_content )[0] << 8 | 
					( ( uint8_t * ) pDesc_list_walker->descriptor_content )[1];

				// Match the CA system id
				if ( cas_id == ca_system_id )
				{
					*pTempProgramInfo++ = pDesc_list_walker->tag;
					*pTempProgramInfo++ =
						pDesc_list_walker->descriptor_length;

					rmf_osal_memcpy( pTempProgramInfo,
							pDesc_list_walker->descriptor_content,
							pDesc_list_walker->descriptor_length, 
							( pParseOutput->ProgramInfoLen - (pTempProgramInfo - pParseOutput->pProgramInfo) ),
							pDesc_list_walker->descriptor_length
							);

					RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
							 "CA Tag: %d, CA Desc len: %d bytes, CA PID: 0x%x cas_id:%d\n",
							 pDesc_list_walker->tag,
							 pDesc_list_walker->descriptor_length + 2,
							 *pTempProgramInfo, cas_id );

					pTempProgramInfo += pDesc_list_walker->descriptor_length;
				}

			}

			pDesc_list_walker = pDesc_list_walker->next;
		}						/* end of descriptor parsing  */
	}
	/* done with outer descriptor loop	*/

	if ( pParseOutput->streamInfoLen > 0 )
	{
			// allocate storage based on what was calculated as the overall size required
			ret =
				rmf_osal_memAllocP( RMF_OSAL_MEM_POD,
									pParseOutput->streamInfoLen,
									( void ** ) &pParseOutput->pStreamInfo );
			if ( RMF_SUCCESS != ret )
			{
				return RMF_OSAL_ENOMEM;
			}
	}

	if ( pParseOutput->noES > 0 )
	{
		ret =
			rmf_osal_memAllocP( RMF_OSAL_MEM_POD,
								( pParseOutput->noES *
								  sizeof( rmf_PodmgrStreamDecryptInfo ) ),
								( void ** ) &pParseOutput->pESInfo );
		if ( RMF_SUCCESS != ret )
		{
			return RMF_OSAL_ENOMEM;
		}
	}

	pES_list_walker = pESList;
	pTempStreamInfo = pParseOutput->pStreamInfo;

	noES = 0;
	// Walk inner descriptors
	while ( pES_list_walker )
	{
		if ( pParseOutput->pStreamInfo )
		{
			*pTempStreamInfo++ =
				( uint8_t ) pES_list_walker->elementary_stream_type;
			*pTempStreamInfo++ = ( pES_list_walker->elementary_PID >> 8 ) & 0x1F;		// Top 3 bits are reserved
			*pTempStreamInfo++ = pES_list_walker->elementary_PID & 0xFF;
			*pTempStreamInfo++ = ( pParseOutput->aComps[noES] >> 8 ) & 0xF;		// top 4 bits are reserved
			*pTempStreamInfo++ = pParseOutput->aComps[noES] & 0xFF;

			if ( pParseOutput->aComps[noES] > 0 )
			{
				pDesc_list_walker = pES_list_walker->descriptors;

				/* Standard code deviation: ext: X1 start */
#ifdef CMCST_EXT_X1
				//*pTempStreamInfo++ = caPMTCMDId;
#else
				pParseOutput->CAPMTCmdIdIndex = pTempStreamInfo - pParseOutput->pStreamInfo;
				*pTempStreamInfo++ = caPMTCMDId;
#endif

				while ( pDesc_list_walker )
				{
					if ( ( RMF_SI_DESC_CA_DESCRIPTOR ==
						   pDesc_list_walker->tag )
						 && ( pDesc_list_walker->descriptor_length > 0 ) )
					{
						uint16_t cas_id = 
								( ( uint8_t * ) pDesc_list_walker->descriptor_content )[0] << 8 | 
								( ( uint8_t * ) pDesc_list_walker->descriptor_content )[1];

						// Match the CA system id
						if ( cas_id == ca_system_id )
						{
							/* Standard code deviation: ext: X1 start */
#ifdef CMCST_EXT_X1
							pParseOutput->CAPMTCmdIdIndex = pTempStreamInfo - pParseOutput->pStreamInfo;
							// updated MPE POD implementation 							
							*pTempStreamInfo++ = caPMTCMDId;
#endif
							/* Standard code deviation: ext: X1 end */

							*pTempStreamInfo++ = pDesc_list_walker->tag;
							*pTempStreamInfo++ =
								pDesc_list_walker->descriptor_length;

							uint16_t cas_pid =
								( ( ( uint8_t * ) pDesc_list_walker->
									descriptor_content )[2] ) << 8 |
								( ( uint8_t * ) pDesc_list_walker->
								  descriptor_content )[3];
							RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
									 "CA SYSTEM ID: 0x%x(%d), CA PID: 0x%x(%d) PAYLOAD: %d bytes, total CA descriptor size: %d bytes.\n",
									 cas_id, cas_id, cas_pid, cas_pid,
									 pDesc_list_walker->descriptor_length - 4,
									 pDesc_list_walker->descriptor_length );

							*pTempStreamInfo++ =
								( ( uint8_t * ) pDesc_list_walker->descriptor_content )[0];
							*pTempStreamInfo++ =
								( ( uint8_t * ) pDesc_list_walker->descriptor_content )[1];
							*pTempStreamInfo++ =
								( ( uint8_t * ) pDesc_list_walker->descriptor_content )[2];
							*pTempStreamInfo++ =
								( ( uint8_t * ) pDesc_list_walker->descriptor_content )[3];

							rmf_osal_memcpy( pTempStreamInfo,
									&( ( uint8_t * ) pDesc_list_walker->descriptor_content )[4],
									pDesc_list_walker->descriptor_length - 4,
									(pParseOutput->streamInfoLen - (pTempStreamInfo- pParseOutput->pStreamInfo)),
									pDesc_list_walker->descriptor_length - 4);

							pTempStreamInfo +=
								pDesc_list_walker->descriptor_length - 4;

						}
					}

					pDesc_list_walker = pDesc_list_walker->next;
				}
			}
		}
		else
		{						
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					 "Test:  pParseOutput->pStreamInfo is NULL !! " );
		}
		pParseOutput->pESInfo[noES].pid =
			pES_list_walker->elementary_PID;
		pParseOutput->pESInfo[noES].status = 0;	// CA_ENABLE_NO_VALUE

		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.SI",
				 "CheckCADescriptors - pid:%d ca_status:%d \n",
				 pES_list_walker->elementary_PID,
				 pParseOutput->pESInfo[noES].status );

		pES_list_walker = pES_list_walker->next;
		noES++;
	}

	return RMF_SUCCESS;
}

void freeDescInfo( parsedDescriptors * pParseOutput )
{
	if ( pParseOutput )
	{
		if ( pParseOutput->pESInfo )
		{
			rmf_osal_memFreeP( RMF_OSAL_MEM_POD,
							   pParseOutput->pESInfo );
			pParseOutput->pESInfo = NULL;
		}

		if ( pParseOutput->pStreamInfo )
		{
			rmf_osal_memFreeP( RMF_OSAL_MEM_POD, pParseOutput->pStreamInfo );
			pParseOutput->pStreamInfo = NULL;
		}

		if ( pParseOutput->pProgramInfo )
		{
			rmf_osal_memFreeP( RMF_OSAL_MEM_POD, pParseOutput->pProgramInfo );
			pParseOutput->pProgramInfo = NULL;
		}
	}
}


rmf_Error rmf_podmgrGetDSGIP(uint8_t *pDSGIP)
{

	rmf_Error ret = RMF_SUCCESS;

		IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_DSG_IP);
		int32_t result_recv = 0;	
		
		int8_t res = ipcBuf.sendCmd( pod_server_cmd_port_no );
		if ( 0 != res )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
				"%s: Lost connection with POD. res = %d\n",
					 __FUNCTION__, res );
			return RMF_OSAL_ENODATA;
		}
		
		result_recv = ipcBuf.getResponse( &ret );
		if( result_recv < 0)
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
				"POD Comm get response failed in %s:%d ret = %d\n",
					 __FUNCTION__, __LINE__, result_recv );	
			return RMF_OSAL_ENODATA;			
		}

		if( RMF_SUCCESS != ret)
		{
				return RMF_OSAL_ENODATA;
		}

		result_recv = ipcBuf.collectData( (void *) pDSGIP, RMF_PODMGR_IP_ADDR_LEN);
		if( result_recv < 0)
		{
			return RMF_OSAL_ENODATA;
		}		
 
	return ret;
}


rmf_Error getCASystemId (uint16_t *pCA_system_id)
{
#ifndef USE_POD_IPC
	return podImplGetCaSystemId(pCA_system_id);
#else

	static uint16_t ca_system_id = 0;
	rmf_Error ret = RMF_SUCCESS;
	RDK_LOG( RDK_LOG_TRACE1, "LOG.RDK.POD",
					 "%s local  - ca_system_id: 0x%x\n",
					 __FUNCTION__, ca_system_id );
	if( ca_system_id == 0 )
	{
		IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_CASYSTEMID );
		int32_t result_recv = 0;	
		
		int8_t res = ipcBuf.sendCmd( pod_server_cmd_port_no );
		if ( 0 != res )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
				"%s: Lost connection with POD. res = %d\n",
					 __FUNCTION__, res );
			return RMF_OSAL_ENODATA;
		}
		
		result_recv = ipcBuf.getResponse( &ret );
		if( result_recv < 0)
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
				"POD Comm get response failed in %s:%d ret = %d\n",
					 __FUNCTION__, __LINE__, result_recv );	
			return RMF_OSAL_ENODATA;			
		}

		if( RMF_SUCCESS != ret)
		{
				return RMF_OSAL_ENODATA;
		}

		result_recv = ipcBuf.collectData( (void *)&ca_system_id, sizeof(ca_system_id) );
		if( result_recv < 0)
		{
			return RMF_OSAL_ENODATA;
		}		
	}
	*pCA_system_id = ca_system_id;
	return ret;
#endif	
}

rmf_Error rmf_podmgrGetMaxDecElemStream( uint16_t *pMaxElmStr )
{
        rmf_Error ret = RMF_SUCCESS;
        int32_t result_recv = 0;
        int8_t res = 0;

        IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GETMAX_ELM_STR);
        res = ipcBuf.sendCmd( pod_server_cmd_port_no );

        if ( 0 != res )
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
                        "POD Comm failure in %s:%d: ret = %d\n",
                                 __FUNCTION__, __LINE__, res );
                return RMF_OSAL_ENODATA;
        }
        result_recv = ipcBuf.getResponse( &ret );
        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
                        "POD Comm get response failed in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, result_recv );
                return RMF_OSAL_ENODATA;
        }

        if( ret != RMF_SUCCESS )
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
                        "POD returning error in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, ret);
                return RMF_OSAL_ENODATA;
        }

        result_recv = ipcBuf.collectData( (void *)pMaxElmStr, sizeof( uint16_t ) );

        if( result_recv != 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
                        "POD Comm add data failed in %s:%d: %d\n",
                                 __FUNCTION__, __LINE__ , result_recv);
                return RMF_OSAL_ENODATA;
        }
        return RMF_SUCCESS;	
}


void rmf_podmgrInit(void)
{
	rmf_Error ret = RMF_SUCCESS;
	uint8_t loopVar = 0;
    isPodMgrUp = 1;
	const char *server_port;
	const char *hal_service;
	const char *gcPodESFilter = NULL;
	
	server_port = rmf_osal_envGet( "POD_SERVER_CMD_PORT_NO" );
	if( NULL == server_port )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s: NULL POINTER  received for name string\n",__FUNCTION__ );
		pod_server_cmd_port_no = POD_SERVER_CMD_DFLT_PORT_NO ;
	}
	else
	{
		pod_server_cmd_port_no = atoi( server_port );
	}
	hal_service = rmf_osal_envGet( "POD_HAL_SERVICE_CMD_PORT_NO" );
	if( NULL == hal_service)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s: NULL POINTER  received for name string\n",__FUNCTION__ );
		pod_hal_service_cmd_port_no = POD_HAL_SERVICE_CMD_DFLT_PORT_NO;
	}
	else
	{
		pod_hal_service_cmd_port_no = atoi( hal_service );
	}
	
	gcPodESFilter = rmf_osal_envGet( "POD.DISABLE_ESPID_SOFT_FILTER" ); 
	if ( ( gcPodESFilter != NULL )
		 && ( strcasecmp( gcPodESFilter, "TRUE" ) == 0 ) )
	{
		g_pod_filter_enabled = FALSE;
	}
	else
	{
		g_pod_filter_enabled = TRUE;
	}

	
	ret = rmf_osal_memAllocP( RMF_OSAL_MEM_POD,
								( POD_EID_MAX_NO * sizeof( EIDStatus_t) ) ,
								( void ** ) &pEIDStatus );
	if( RMF_SUCCESS != ret )
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD",
						  "%s() malloc failed\n",__FUNCTION__);		
		return;
	}
	
	for( loopVar= POD_EID_GENERAL;  loopVar < POD_EID_MAX_NO; loopVar++ )
	{
		pEIDStatus[ loopVar ].status = 	EID_STATUS_UNKNOWN;
		pEIDStatus[ loopVar ].eid = 0;
	}
		
#ifndef USE_POD_IPC
	OobServiceInit();
	podmgrInit();
	rmf_osal_RegDsgDump( vlDsgDumpDsgStats );
	rmf_osal_RegCdlUpgrade( vlMpeosCdlUpgradeToImage );
	rmf_osal_RegDsgCheckAndRecoverConnectivity(vlDsgCheckAndRecoverConnectivity);

#else

	int8_t result = 0;

	podDecryptQueueList= NULL;
	rmf_osal_mutexNew( &podDecryptQueueListMutex );
	rmf_osal_mutexNew( &podHandleMutex );
	rmf_osal_mutexNew( &podSIMgrHandleMutex );
	
	ret = rmf_osal_eventmanager_create(&pod_client_event_manager);
	if (RMF_SUCCESS != ret)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD",
						  "rmf_osal_eventmanager_create() failed\n");
		return;
	}
	
	ret = rmf_osal_eventmanager_create(&simgr_client_event_manager);
	if (RMF_SUCCESS != ret)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD",
						  "rmf_osal_eventmanager_create() failed\n");
		return;
	}
	
	if ( ( ret =
		   rmf_osal_threadCreate( podHeartbeatMonThread, NULL,
								  RMF_OSAL_THREAD_PRIOR_DFLT,
								  RMF_OSAL_THREAD_STACK_SIZE,
								  &podClientMonitorThreadId,
								  "podHeartbeatMonThread" ) ) !=
		 RMF_SUCCESS )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "%s: failed to create pod worker thread (err=%d).\n",
				 __FUNCTION__, ret );
	}
	
	if ( ( ret =
		   rmf_osal_threadCreate( podEventPollThread, NULL,
								  RMF_OSAL_THREAD_PRIOR_DFLT,
								  RMF_OSAL_THREAD_STACK_SIZE,
								  &podEventPollThreadId,
								  "podEventPollThread" ) ) !=
		 RMF_SUCCESS )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "%s: failed to create pod worker thread (err=%d).\n",
				 __FUNCTION__, ret );
	}

	if ( ( ret =
		   rmf_osal_threadCreate( podSIMgrEventPollThread, NULL,
								  RMF_OSAL_THREAD_PRIOR_DFLT,
								  RMF_OSAL_THREAD_STACK_SIZE,
								  &podSIMgrEventPollThreadId,
								  "podSIMgrEventPollThread" ) ) !=
		 RMF_SUCCESS )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "%s: failed to create podSIMgrEventPollThread (err=%d).\n",
				 __FUNCTION__, ret );
	}
	
	pPODHALServer = new IPCServer( ( int8_t * ) "podServer", pod_hal_service_cmd_port_no );
	result = pPODHALServer->start(  );

	if ( 0 != result )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s : Server failed to start\n",
				 __FUNCTION__ );
	}

	RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.POD",
			 "%s : Server server1 started successfully\n", __FUNCTION__ );
	pPODHALServer->registerCallBk( &podHalService, ( void * ) pPODHALServer );
	rmf_osal_RegDsgDump(rmf_podmgrDumpDsgStats);

	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD",
			  "%s() Registering rmf_podmgrCdlUpgradeToImage()\n",__FUNCTION__);	

	rmf_osal_RegCdlUpgrade(rmf_podmgrCdlUpgradeToImage);
	rmf_osal_RegDsgCheckAndRecoverConnectivity(rmf_podmgrDsgCheckAndRecoverConnectivity);

#endif

	if ( ( ret =
		   rmf_osal_threadCreate( InbandCableCardFwuMonitor, NULL,
								  RMF_OSAL_THREAD_PRIOR_DFLT,
								  RMF_OSAL_THREAD_STACK_SIZE,
								  &podFWUMonitorThreadId,
								  "InbandCableCardFwuMonitor" ) ) !=
		 RMF_SUCCESS )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "%s: failed to create InbandCableCardFwuMonitor thread (err=%d).\n",
				 __FUNCTION__, ret );
	}

    ret = rmf_osal_condNew( TRUE, FALSE, &event_thread_stop_cond);
    if (RMF_SUCCESS != ret )
    {
        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s - rmf_osal_condNew for event_thread_stop_cond failed. (err=%d)\n", __FUNCTION__, ret);
    }

    ret = rmf_osal_condNew( TRUE, FALSE, &hbmon_thread_stop_cond);
    if (RMF_SUCCESS != ret )
    {
        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s - rmf_osal_condNew for hbmon_thread_stop_cond failed. (err=%d)\n", __FUNCTION__, ret);
    }

	ret = canhClientStart(  );
	if ( RMF_SUCCESS != ret )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.IPPV", "%s - failed.\n",
				 __FUNCTION__ );
	}
		
	return;
	
}

rmf_Error rmf_podmgrRegisterEvents(rmf_osal_eventqueue_handle_t queueId)
{
#ifndef USE_POD_IPC
	return podmgrRegisterEvents( queueId );
#else
	return  rmf_osal_eventmanager_register_handler(
		pod_client_event_manager,
		queueId,
		RMF_OSAL_EVENT_CATEGORY_PODMGR);
#endif
}

rmf_Error rmf_podmgrUnRegisterEvents(rmf_osal_eventqueue_handle_t queueId)
{
#ifndef USE_POD_IPC
	return podmgrUnRegisterEvents( queueId );
#else
	return rmf_osal_eventmanager_unregister_handler(
		pod_client_event_manager,
		queueId	);
#endif
}

rmf_Error rmf_oobsimgrRegisterEvents(rmf_osal_eventqueue_handle_t queueId)
{
	return  rmf_osal_eventmanager_register_handler(
		simgr_client_event_manager,
		queueId,
		RMF_OSAL_EVENT_CATEGORY_SIMGR);
}

rmf_Error rmf_oobsimgrUnRegisterEvents(rmf_osal_eventqueue_handle_t queueId)
{
	return rmf_osal_eventmanager_unregister_handler(
		simgr_client_event_manager,
		queueId	);
}

rmf_Error rmf_podmgrIsReady(void)
{
#ifndef USE_POD_IPC
	return podmgrIsReady();
#else

	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_IS_READY);
	int32_t result_recv = 0;
	rmf_Error ret = RMF_SUCCESS;

	int8_t res = ipcBuf.sendCmd( pod_server_cmd_port_no );
	if ( 0 != res )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"%s: Lost connection with POD. res = %d\n",
				 __FUNCTION__, res );
		return RMF_OSAL_ENODATA;
	}	

	result_recv = ipcBuf.getResponse( &ret );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"%s: No response from POD. result_recv = %d\n",
				 __FUNCTION__, result_recv );	
		return RMF_OSAL_ENODATA;
	}
	
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
		"%s: POD Manager ready status is = %d\n",
			 __FUNCTION__, ret );	
	return ret;	
#endif	
}

rmf_Error rmf_podmgrStartDecrypt(
        rmf_PodmgrDecryptRequestParams* decryptRequestPtr,
        rmf_osal_eventqueue_handle_t qam_eventq, 
        uint32_t* sessionHandlePtr)
{
	parsedDescriptors ParseOutput;
	decryptRequestParams requestPtr;
	rmf_Error ret = RMF_SUCCESS;
#ifdef QUERY_CA_FROM_CANH	
	sourceParams *pSource =	NULL;
#endif	
	g_qamsrc_eventq = qam_eventq;
	memset( &ParseOutput, 0 , sizeof(ParseOutput) );
	memset( ParseOutput.aComps, 0 , sizeof(ParseOutput.aComps) );	
	if (g_pod_filter_enabled )
	{
	
		ret = parseFilteredDescriptors(	decryptRequestPtr->pMpeg2DescList, 
							decryptRequestPtr->pESList, 
							&ParseOutput );
	}
	else
	{
		ret = parseDescriptors(	decryptRequestPtr->pMpeg2DescList, 
							decryptRequestPtr->pESList, 
							&ParseOutput );

	}
	
	if( RMF_SUCCESS != ret ) 
	{
		RDK_LOG( RDK_LOG_WARN, "LOG.RDK.POD",
				 "<%s> parseDescriptors failed \n",
					 __FUNCTION__ );		
		return ret;
	}	

	if( ParseOutput.caDescFound == FALSE )
	{
		RDK_LOG( RDK_LOG_WARN, "LOG.RDK.POD",
				 "<%s> Might be a clear channel \n",
					 __FUNCTION__ );
		decryptRequestPtr->bClear = 1;
	}
	else
	{
		decryptRequestPtr->bClear = 0;
	}

	if (g_pod_filter_enabled )
	{
		ret = allocateFilteredDescInfo(&ParseOutput, decryptRequestPtr->pESList, decryptRequestPtr->pMpeg2DescList, 0);
	}
	else
	{
		ret = allocateDescInfo(&ParseOutput, decryptRequestPtr->pESList, decryptRequestPtr->pMpeg2DescList, 0);
	}
	
	if( RMF_SUCCESS != ret ) 
	{
		RDK_LOG( RDK_LOG_WARN, "LOG.RDK.POD",
				 "<%s> allocateDescInfo failed \n",
					 __FUNCTION__ );		
		return ret;
	}

	ParseOutput.prgNo = decryptRequestPtr->prgNo;
	ParseOutput.srcId = decryptRequestPtr->srcId;
	
	requestPtr.ltsId = decryptRequestPtr->ltsId;
	requestPtr.mmiEnable = decryptRequestPtr->mmiEnable;
	requestPtr.numPids = decryptRequestPtr->numPids;
	requestPtr.pDescriptor = &ParseOutput;
	requestPtr.pids = decryptRequestPtr->pids;
	requestPtr.priority = decryptRequestPtr->priority;
	requestPtr.tunerId = decryptRequestPtr->tunerId;
	requestPtr.qamContext = decryptRequestPtr->qamContext;

#ifndef USE_POD_IPC
	ret = podmgrStartDecrypt( &requestPtr,
								  queueId,
								  sessionHandlePtr );
#else								  
	{
		int32_t result_recv = 0;
		uint32_t Handle;
		int8_t res = 0;		
		IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_START_DECRYPT );
		getPodClientHandle(&Handle);
		if( 0 == Handle )
		{
			
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					"%s - getPodClientHandle failed \n\n\n",
					__FUNCTION__);
			freeDescInfo(requestPtr.pDescriptor);
			return RMF_OSAL_ENODATA;
		}
		uint8_t loop_count1 = 0;
		uint8_t loop_count2 = 0;

		requestPtr.numPids = 0;		
		for( loop_count1 = 0; loop_count1 < ParseOutput.noES ; loop_count1++ )
		{
			for( loop_count2 = 0; loop_count2 < decryptRequestPtr->numPids ; loop_count2++ )
			{
				if( decryptRequestPtr->pids[loop_count2].pid == ParseOutput.pESInfo[loop_count1].pid )
				{	
					rmf_MediaPID tmp;
					tmp.pid = decryptRequestPtr->pids[loop_count1].pid;
					tmp.pidType = decryptRequestPtr->pids[loop_count1].pidType;
					decryptRequestPtr->pids[loop_count1].pid = decryptRequestPtr->pids[loop_count2].pid;
					decryptRequestPtr->pids[loop_count1].pidType = decryptRequestPtr->pids[loop_count2].pidType;

					decryptRequestPtr->pids[loop_count2].pid = tmp.pid;
					decryptRequestPtr->pids[loop_count2].pidType = tmp.pidType;
					requestPtr.numPids++;
				}
			}
		}
		result_recv |= ipcBuf.addData( ( const void * ) &Handle, sizeof( Handle ) );
		
		result_recv |= ipcBuf.addData( ( const void * ) &requestPtr.tunerId, sizeof( requestPtr.tunerId ) );
		result_recv |= ipcBuf.addData( ( const void * ) &requestPtr.ltsId, sizeof( requestPtr.ltsId ) );
		result_recv |= ipcBuf.addData( ( const void * ) &requestPtr.numPids, sizeof( requestPtr.numPids ) );
		result_recv |= ipcBuf.addData( ( const void * ) requestPtr.pids, (requestPtr.numPids * sizeof( rmf_MediaPID )) );
		result_recv |= ipcBuf.addData( ( const void * ) &requestPtr.pDescriptor->prgNo, sizeof( requestPtr.pDescriptor->prgNo ) );
		result_recv |= ipcBuf.addData( ( const void * ) &requestPtr.pDescriptor->srcId, sizeof( requestPtr.pDescriptor->srcId ) );
		result_recv |= ipcBuf.addData( ( const void * ) &requestPtr.pDescriptor->ecmPid, sizeof( requestPtr.pDescriptor->ecmPid ) );
		result_recv |= ipcBuf.addData( ( const void * ) &requestPtr.pDescriptor->ecmPidFound, sizeof( requestPtr.pDescriptor->ecmPidFound ) );
		result_recv |= ipcBuf.addData( ( const void * ) &requestPtr.pDescriptor->caDescFound, sizeof( requestPtr.pDescriptor->caDescFound ) );
		result_recv |= ipcBuf.addData( ( const void * ) &requestPtr.pDescriptor->noES, sizeof( requestPtr.pDescriptor->noES ) );
		result_recv |= ipcBuf.addData( ( const void * ) requestPtr.pDescriptor->pESInfo, requestPtr.pDescriptor->noES * sizeof(rmf_PodmgrStreamDecryptInfo));	
		result_recv |= ipcBuf.addData( ( const void * ) &requestPtr.pDescriptor->ProgramInfoLen, sizeof( requestPtr.pDescriptor->ProgramInfoLen ) );
		result_recv |= ipcBuf.addData( ( const void * ) requestPtr.pDescriptor->pProgramInfo, requestPtr.pDescriptor->ProgramInfoLen * sizeof(uint8_t) );
		result_recv |= ipcBuf.addData( ( const void * ) &requestPtr.pDescriptor->streamInfoLen, sizeof( requestPtr.pDescriptor->streamInfoLen ) );
		result_recv |= ipcBuf.addData( ( const void * ) requestPtr.pDescriptor->pStreamInfo, requestPtr.pDescriptor->streamInfoLen * sizeof(uint8_t) );
		result_recv |= ipcBuf.addData( ( const void * ) requestPtr.pDescriptor->aComps, sizeof(requestPtr.pDescriptor->aComps) );	
		result_recv |= ipcBuf.addData( ( const void * ) &requestPtr.pDescriptor->CAPMTCmdIdIndex, sizeof( requestPtr.pDescriptor->CAPMTCmdIdIndex ) );
		result_recv |= ipcBuf.addData( ( const void * ) &requestPtr.priority, sizeof( requestPtr.priority ) );
		result_recv |= ipcBuf.addData( ( const void * ) &requestPtr.mmiEnable, sizeof( requestPtr.mmiEnable ) );
		result_recv |= ipcBuf.addData( ( const void * ) &requestPtr.qamContext, sizeof( requestPtr.qamContext ) );

		if( result_recv < 0)
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
				"POD Comm add data failed in %s:%d: %d\n",
					 __FUNCTION__, __LINE__ , result_recv);	
			freeDescInfo(requestPtr.pDescriptor);
			return RMF_OSAL_ENODATA;			
		}
		
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				 "<%s> sending decrypt request \n",
					 __FUNCTION__ );	
		res = ipcBuf.sendCmd( pod_server_cmd_port_no );

		if ( 0 != res )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
				"POD Comm failure in %s:%d: ret = %d\n",
					 __FUNCTION__, __LINE__, res );
			freeDescInfo(requestPtr.pDescriptor);
			return RMF_OSAL_ENODATA;
		}

		result_recv = ipcBuf.getResponse( &ret );
		if( result_recv < 0)
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
				"POD Comm get response failed in %s:%d ret = %d\n",
					 __FUNCTION__, __LINE__, result_recv );	
			freeDescInfo(requestPtr.pDescriptor);
			return RMF_OSAL_ENODATA;			
		}
		
		result_recv = ipcBuf.collectData( (void *)sessionHandlePtr, sizeof(uint32_t) );	
		if( result_recv != 0)
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
				"POD Comm get sessionHandlePtr failed in %s:%d\n",
					 __FUNCTION__, __LINE__ );	
			freeDescInfo(requestPtr.pDescriptor);
			return RMF_OSAL_ENODATA;			
		}

		if( (ret == RMF_SUCCESS)  && ( 0 != *sessionHandlePtr ) )
		{				
			//addToDecryptQList( *sessionHandlePtr, queueId);
		}
	}
#endif

	freeDescInfo(requestPtr.pDescriptor);	

#ifdef QUERY_CA_FROM_CANH
	if( (ret == RMF_SUCCESS)  && ( 0 != *sessionHandlePtr ) )
	{				
		ret =
			rmf_osal_memAllocP( RMF_OSAL_MEM_POD,
								  sizeof( sourceParams ) ,
								( void ** ) &pSource );
		if ( RMF_SUCCESS != ret )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
				"Mem allocation failed %s:%d\n",
					 __FUNCTION__, __LINE__ );				
			return RMF_OSAL_ENOMEM;
		}	
		pSource->sessionHandle = *sessionHandlePtr;
		pSource->sourceId = decryptRequestPtr->srcId;
		pSource->qamContext = decryptRequestPtr->qamContext;

		if ( ( ret =
			   rmf_osal_threadCreate( podCheckSourceAuthorization, (void *)pSource,
									  RMF_OSAL_THREAD_PRIOR_DFLT,
									  RMF_OSAL_THREAD_STACK_SIZE,
									  &podCheckSourceAuthorizationThreadId,
									  "podCheckSourceAuthorization" ) ) !=
			 RMF_SUCCESS )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
					 "%s: failed to create checkSourceAuthorization thread (err=%d).\n",
					 __FUNCTION__, ret );
		}
	}
	
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
		"%s : Done\n",
			 __FUNCTION__);	
#endif
	return ret;

}

#ifdef QUERY_CA_FROM_CANH
void podCheckSourceAuthorization( void *sourceDetails )
{	
	rmf_Error ret = RMF_SUCCESS;
	uint8_t isAuthorized = 0;
	sourceParams *pSource = ( sourceParams * )sourceDetails;
	
	ret = IsCanhSourceAuthorized( pSource->sourceId, &isAuthorized );
	if( RMF_SUCCESS == ret )
	{
		if( !isAuthorized )
		{
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
					 "<%s> This content is not authorized \n",
						 __FUNCTION__ );
			notifyCCIChange( pSource->qamContext, pSource->sessionHandle, 0,  
				RMF_PODMGR_DECRYPT_EVENT_CANNOT_DESCRAMBLE_ENTITLEMENT );
		}
		else
		{
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
					 "<%s> This content is authorized \n",
						 __FUNCTION__ );
			notifyCCIChange( pSource->qamContext, pSource->sessionHandle, 0,  
				RMF_PODMGR_DECRYPT_EVENT_FULLY_AUTHORIZED );			
		}			
	} 
	else if( RMF_OSAL_EINVAL == ret ) 
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "<%s> CANH communication error, so not setting auth level \n",
					 __FUNCTION__ );		
	} 
	else if( RMF_OSAL_ENODATA == ret ) 
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
				 "<%s> CANH did not received enough data from CARD, so not setting auth level \n",
					 __FUNCTION__ );		
	}
	
	rmf_osal_memFreeP( RMF_OSAL_MEM_POD, pSource );

	return;
	
}
#endif

int rmf_podmgrConfigCipher(unsigned char ltsid,unsigned short PrgNum, rmf_MediaPID *pPidList, unsigned long numpids)
{

        unsigned long decodePid[ 64 ] = {0};
        uint8_t loop_count = 0;

        for(; loop_count <numpids; loop_count ++)
        {
                if( (
                        ( RMF_SI_ELEM_MPEG_1_VIDEO == pPidList[loop_count].pidType  ) ||
                        ( RMF_SI_ELEM_MPEG_2_VIDEO == pPidList[loop_count].pidType  ) ||
                        ( RMF_SI_ELEM_VIDEO_DCII == pPidList[loop_count].pidType  ) ||
                        ( RMF_SI_ELEM_AVC_VIDEO == pPidList[loop_count].pidType  )
                    ) &&
                        ( 0 != loop_count  ) )
                {
                        decodePid[ loop_count ] = decodePid[ 0 ];
                        decodePid[ 0 ] = pPidList[loop_count].pid;
                }
                else
                {
                        decodePid[loop_count] = pPidList[loop_count].pid;
                }
        }

#ifndef USE_POD_IPC	
	return podmgrConfigCipher( ltsid, PrgNum, decodePid, numpids );
#else
	rmf_Error ret = RMF_SUCCESS;
	int32_t result_recv = 0;
	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_CONFIG_CIPH);
	int8_t res = 0;	

	result_recv |= ipcBuf.addData( ( const void * ) &ltsid, sizeof( ltsid ) );
	result_recv |= ipcBuf.addData( ( const void * ) &PrgNum, sizeof( PrgNum ) );
	result_recv |= ipcBuf.addData( ( const void * ) &numpids, sizeof( numpids ) );
	result_recv |= ipcBuf.addData( ( const void * ) decodePid, numpids * sizeof( unsigned long )  );
	
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"POD Comm add data failed in %s:%d: %d\n",
				 __FUNCTION__, __LINE__ , result_recv);	
		return RMF_OSAL_ENODATA;			
	}		
	res = ipcBuf.sendCmd( pod_server_cmd_port_no );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"POD Comm get response failed in %s:%d ret = %d\n",
				 __FUNCTION__, __LINE__, result_recv );	
		return RMF_OSAL_ENODATA;			
	}

	result_recv = ipcBuf.getResponse( &ret );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"POD Comm get response failed in %s:%d ret = %d\n",
				 __FUNCTION__, __LINE__, result_recv );	
		return RMF_OSAL_ENODATA;			
	}

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
		"%s : Done\n",
			 __FUNCTION__);	

	return ret;
#endif
}

rmf_Error rmf_podmgrGetCCIBits( uint32_t sessionHandle,
								uint8_t * pCciBits )
{
#ifndef USE_POD_IPC
	return podmgrGetCCIBits( sessionHandle, pCciBits );
#else
	rmf_Error ret = RMF_SUCCESS;
	int32_t result_recv = 0;
	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_CCI_BIT);
	int8_t res = 0;	
	
	result_recv |= ipcBuf.addData( ( const void * ) &sessionHandle, sizeof( sessionHandle ) );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"POD Comm add data failed in %s:%d: %d\n",
				 __FUNCTION__, __LINE__ , result_recv);	
		return RMF_OSAL_ENODATA;			
	}	
	res = ipcBuf.sendCmd( pod_server_cmd_port_no );

	if ( 0 != res )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"POD Comm failure in %s:%d: ret = %d\n",
				 __FUNCTION__, __LINE__, res );
		return RMF_OSAL_ENODATA;
	}	
	
	result_recv = ipcBuf.getResponse( &ret );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"POD Comm get response failed in %s:%d ret = %d\n",
				 __FUNCTION__, __LINE__, result_recv );	
		return RMF_OSAL_ENODATA;			
	}

	result_recv = ipcBuf.collectData( (void *)pCciBits, sizeof(uint8_t) );

	if( result_recv != 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"POD Comm get CCI bit failed in %s:%d\n",
				 __FUNCTION__, __LINE__ );	
		return RMF_OSAL_ENODATA;			
	}

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
		"%s : Done\n",
			 __FUNCTION__);	

	return ret;	
#endif
}

rmf_Error rmf_podmgrStopDecrypt(uint32_t sessionHandle)
{

#ifndef USE_POD_IPC
	return podmgrStopDecrypt( sessionHandle );
#else

	rmf_Error ret = RMF_SUCCESS;
	int32_t result_recv = 0;
	int8_t res = 0;
	
	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_STOP_DECRYPT);
	result_recv |= ipcBuf.addData( ( const void * ) &sessionHandle, sizeof( sessionHandle ) );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"POD Comm add data failed in %s:%d: %d\n",
				 __FUNCTION__, __LINE__ , result_recv);	
		return RMF_OSAL_ENODATA;			
	}	

	res = ipcBuf.sendCmd( pod_server_cmd_port_no );

	if ( 0 != res )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"POD Comm failure in %s:%d: ret = %d\n",
				 __FUNCTION__, __LINE__, res );
		return RMF_OSAL_ENODATA;
	}	
	result_recv = ipcBuf.getResponse( &ret );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"POD Comm get response failed in %s:%d ret = %d\n",
				 __FUNCTION__, __LINE__, result_recv );	
		return RMF_OSAL_ENODATA;			
	}

	removeFromDecryptQList(sessionHandle);
	
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
		"%s : Done\n",
			 __FUNCTION__);	
	return ret;
#endif
}

rmf_Error rmf_GetProgramInfo( uint32_t decimalSrcId, rmf_ProgramInfo_t *pInfo)
{
#ifndef USE_POD_IPC			
			return OobGetProgramInfo(decimalSrcId, (Oob_ProgramInfo_t  *)pInfo);
#else

	rmf_Error ret = RMF_SUCCESS;
	int32_t result_recv = 0;
	int8_t res = 0;
	
	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_PROGRAM_INFO);
	result_recv |= ipcBuf.addData( ( const void * ) &decimalSrcId, sizeof( decimalSrcId ) );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"\nPOD Comm add data failed in %s:%d: %d\n",
				 __FUNCTION__, __LINE__ , result_recv);	
		return RMF_OSAL_ENODATA;			
	}	
	res = ipcBuf.sendCmd( pod_server_cmd_port_no );

	if ( 0 != res )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"POD Comm failure in %s:%d: ret = %d\n",
				 __FUNCTION__, __LINE__, res );
		return RMF_OSAL_ENODATA;
	}	
	result_recv = ipcBuf.getResponse( &ret );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"POD Comm get response failed in %s:%d ret = %d\n",
				 __FUNCTION__, __LINE__, result_recv );	
		return RMF_OSAL_ENODATA;			
	}

	if( ret != RMF_SUCCESS )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"OOB returning error in %s:%d ret = %d\n",
				 __FUNCTION__, __LINE__, ret);	
		return RMF_OSAL_ENODATA;			
	}		

	result_recv |= ipcBuf.collectData( (void *)&pInfo->carrier_frequency, sizeof(pInfo->carrier_frequency) );
	result_recv |= ipcBuf.collectData( (void *)&pInfo->modulation_mode, sizeof(pInfo->modulation_mode) );
	result_recv |= ipcBuf.collectData( (void *)&pInfo->prog_num, sizeof(pInfo->prog_num) );
	result_recv = ipcBuf.collectData( (void *)&pInfo->service_handle, sizeof(pInfo->service_handle) );

	if( result_recv != 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"POD Comm add data failed in %s:%d: %d\n",
				 __FUNCTION__, __LINE__ , result_recv);	
		return RMF_OSAL_ENODATA;			
	}	
	
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
		"%s : Done\n",
			 __FUNCTION__);	
	return ret;
#endif

}

rmf_Error rmf_GetProgramInfobyVCN( uint32_t vcn, rmf_ProgramInfo_t *pInfo)
{
#ifndef USE_POD_IPC			
			return OobGetProgramInfoByVCN(vcn, (Oob_ProgramInfo_t  *)pInfo);
#else

	rmf_Error ret = RMF_SUCCESS;
	int32_t result_recv = 0;
	int8_t res = 0;
	
	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_PROGRAM_INFO_BY_VCN);
	result_recv |= ipcBuf.addData( ( const void * ) &vcn, sizeof( vcn ) );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"\nPOD Comm add data failed in %s:%d: %d\n",
				 __FUNCTION__, __LINE__ , result_recv);	
		return RMF_OSAL_ENODATA;			
	}	
	res = ipcBuf.sendCmd( pod_server_cmd_port_no );

	if ( 0 != res )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"POD Comm failure in %s:%d: ret = %d\n",
				 __FUNCTION__, __LINE__, res );
		return RMF_OSAL_ENODATA;
	}	
	result_recv = ipcBuf.getResponse( &ret );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"POD Comm get response failed in %s:%d ret = %d\n",
				 __FUNCTION__, __LINE__, result_recv );	
		return RMF_OSAL_ENODATA;			
	}

	if( ret != RMF_SUCCESS )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"OOB returning error in %s:%d ret = %d\n",
				 __FUNCTION__, __LINE__, ret);	
		return RMF_OSAL_ENODATA;			
	}		

	result_recv |= ipcBuf.collectData( (void *)&pInfo->carrier_frequency, sizeof(pInfo->carrier_frequency) );
	result_recv |= ipcBuf.collectData( (void *)&pInfo->modulation_mode, sizeof(pInfo->modulation_mode) );
	result_recv |= ipcBuf.collectData( (void *)&pInfo->prog_num, sizeof(pInfo->prog_num) );
	result_recv = ipcBuf.collectData( (void *)&pInfo->service_handle, sizeof(pInfo->service_handle) );

	if( result_recv != 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"POD Comm add data failed in %s:%d: %d\n",
				 __FUNCTION__, __LINE__ , result_recv);	
		return RMF_OSAL_ENODATA;			
	}	
	
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
		"%s : Done\n",
			 __FUNCTION__);	
	return ret;
#endif

}

rmf_Error rmf_GetVirtualChannelNumber(uint32_t decimalSrcId, uint16_t *pVirtualChannelNumber)
{
#ifndef USE_POD_IPC                     
                        return OobGetVirtualChannelNumber(decimalSrcId, pVirtualChannelNumber);
#else

        rmf_Error ret = RMF_SUCCESS;
        int32_t result_recv = 0;
        int8_t res = 0;

        IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_VIRTUAL_CHANNEL_NUMBER);
        result_recv |= ipcBuf.addData( ( const void * ) &decimalSrcId, sizeof( decimalSrcId ) );
        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
                        "\nPOD Comm add data failed in %s:%d: %d\n",
                                 __FUNCTION__, __LINE__ , result_recv);
                return RMF_OSAL_ENODATA;
        }
        res = ipcBuf.sendCmd( pod_server_cmd_port_no );

        if ( 0 != res )
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
                        "POD Comm failure in %s:%d: ret = %d\n",
                                 __FUNCTION__, __LINE__, res );
                return RMF_OSAL_ENODATA;
        }
        result_recv = ipcBuf.getResponse( &ret );
        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
                        "POD Comm get response failed in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, result_recv );
                return RMF_OSAL_ENODATA;
        }

        if( ret != RMF_SUCCESS )
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
                        "OOB returning error in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, ret);
                return RMF_OSAL_ENODATA;
        }

        result_recv = ipcBuf.collectData( (void *)pVirtualChannelNumber, sizeof(uint16_t) );

        if( result_recv != 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
                        "POD Comm add data failed in %s:%d: %d\n",
                                 __FUNCTION__, __LINE__ , result_recv);
                return RMF_OSAL_ENODATA;
        }

        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
                "%s : Done. VCN for SourceId 0x%x: %d\n",
                         __FUNCTION__, decimalSrcId, *pVirtualChannelNumber);
        return ret;
#endif
}

rmf_Error rmf_podmgrGetManufacturerId (uint16_t *pManufacturerId)
{
#ifndef USE_POD_IPC
	return podmgrGetManufacturerId( pManufacturerId );
#else

	rmf_Error ret = RMF_SUCCESS;
	
	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_MANUFACTURER_ID);
	int32_t result_recv = 0;	
	
	int8_t res = ipcBuf.sendCmd( pod_server_cmd_port_no );
	if ( 0 != res )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"%s: Lost connection with POD. res = %d\n",
				 __FUNCTION__, res );
		return RMF_OSAL_ENODATA;
	}
	
	result_recv = ipcBuf.getResponse( &ret );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"POD Comm get response failed in %s:%d ret = %d\n",
				 __FUNCTION__, __LINE__, result_recv );	
		return RMF_OSAL_ENODATA;			
	}

	if( RMF_SUCCESS != ret)
	{
			return RMF_OSAL_ENODATA;
	}

	result_recv = ipcBuf.collectData( (void *) pManufacturerId, sizeof( uint16_t ) );
	if( result_recv != 0)
	{
		return RMF_OSAL_ENODATA;
	}		
	return ret;
#endif	
}

void rmf_podmgrCdlUpgradeToImage(int eImageType, int eImageSign, int bRebootNow, const char * pszImagePathName)
{
#ifndef USE_POD_IPC
	vlMpeosCdlUpgradeToImage(eImageType, eImageSign, bRebootNow, pszImagePathName);
#else
	rmf_Error ret = RMF_SUCCESS;
	int32_t result_recv = 0;
	int8_t res = 0;
        int ImageNameLen = 0;

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
			"%s:%d: Enter\n",
			__FUNCTION__, __LINE__ );

	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_CDL_UPGRADE_TO_IMAGE);

        result_recv |= ipcBuf.addData( ( const void * ) &eImageType, sizeof( eImageType ) );
        result_recv |= ipcBuf.addData( ( const void * ) &eImageSign, sizeof( eImageSign ) );
        result_recv |= ipcBuf.addData( ( const void * ) &bRebootNow, sizeof( bRebootNow ) );
        
        ImageNameLen = strlen(pszImagePathName);
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
			"%s:%d: Image name: %s, Image name len: %d\n",
			__FUNCTION__, __LINE__, pszImagePathName, ImageNameLen );

        result_recv |= ipcBuf.addData( ( const void * ) &ImageNameLen, sizeof( ImageNameLen ) );
        result_recv |= ipcBuf.addData( ( const void * ) pszImagePathName, ImageNameLen * sizeof( char )  );

        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
                        "POD Comm add data failed in %s:%d: %d\n",
                                 __FUNCTION__, __LINE__ , result_recv);
                return;
        }


	res = ipcBuf.sendCmd( pod_server_cmd_port_no );

	if ( 0 != res )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"POD Comm failure in %s:%d: ret = %d\n",
				 __FUNCTION__, __LINE__, res );
		return;
	}	
	result_recv = ipcBuf.getResponse( &ret );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"POD Comm get response failed in %s:%d ret = %d\n",
				 __FUNCTION__, __LINE__, result_recv );	
		return;			
	}

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
		"Triggered RCDL %s:%d ret = %d\n",
			 __FUNCTION__, __LINE__, ret );		
#endif
	return;
}

void rmf_podmgrDumpDsgStats()
{
#ifndef USE_POD_IPC
	vlDsgDumpDsgStats();
#else
	rmf_Error ret = RMF_SUCCESS;
	int32_t result_recv = 0;
	int8_t res = 0;

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
			"%s:%d: Enter\n",
			__FUNCTION__, __LINE__ );

	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_DUMP_DSG_STAT);
	res = ipcBuf.sendCmd( pod_server_cmd_port_no );

	if ( 0 != res )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"POD Comm failure in %s:%d: ret = %d\n",
				 __FUNCTION__, __LINE__, res );
		return;
	}	
	result_recv = ipcBuf.getResponse( &ret );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"POD Comm get response failed in %s:%d ret = %d\n",
				 __FUNCTION__, __LINE__, result_recv );	
		return;			
	}

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
		"Dumped DSG %s:%d ret = %d\n",
			 __FUNCTION__, __LINE__, ret );		
#endif
	return;
}

void rmf_podmgrDsgCheckAndRecoverConnectivity()
{
#ifndef USE_POD_IPC
	vlDsgCheckAndRecoverConnectivity();
#else
	rmf_Error ret = RMF_SUCCESS;
	int32_t result_recv = 0;
	int8_t res = 0;

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
			"%s:%d: Enter\n",
			__FUNCTION__, __LINE__ );

	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_DSG_CHECKANDRECOVER_CONNECTIVITY);
	res = ipcBuf.sendCmd( pod_server_cmd_port_no );

	if ( 0 != res )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"POD Comm failure in %s:%d: ret = %d\n",
				 __FUNCTION__, __LINE__, res );
		return;
	}	
	result_recv = ipcBuf.getResponse( &ret );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"POD Comm get response failed in %s:%d ret = %d\n",
				 __FUNCTION__, __LINE__, result_recv );	
		return;			
	}

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
		"%s:%d ret = %d\n",
			 __FUNCTION__, __LINE__, ret );		
#endif
	return;
}

void rmf_podmgrStartHomingDownload( uint8_t ltsId, int32_t status )
{

#ifndef USE_POD_IPC
	podmgrStartHomingDownload( ltsId, status );
#else
	rmf_Error ret = RMF_SUCCESS;
	int32_t result_recv = 0;
	int8_t res = 0;
	
	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_START_HOMING_DOWNLOAD);

	result_recv |= ipcBuf.addData( ( const void * ) &ltsId, sizeof( ltsId ) );
	result_recv |= ipcBuf.addData( ( const void * ) &status, sizeof( status ) );

	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"POD Comm add data failed in %s:%d ret = %d\n",
				 __FUNCTION__, __LINE__, result_recv );	
		return;			
	}
	
	res = ipcBuf.sendCmd( pod_server_cmd_port_no );

	if ( 0 != res )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"POD Comm failure in %s:%d: ret = %d\n",
				 __FUNCTION__, __LINE__, res );
		return;
	}	
	
	result_recv = ipcBuf.getResponse( &ret );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"POD Comm get response failed in %s:%d ret = %d\n",
				 __FUNCTION__, __LINE__, result_recv );	
		return;			
	}

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
		"Dumped DSG %s:%d ret = %d\n",
			 __FUNCTION__, __LINE__, ret );		
#endif
	return;

}

#ifdef USE_IPDIRECT_UNICAST
void rmf_podmgrStartInbandTune( uint8_t ltsId, int32_t status )
{

#ifndef USE_POD_IPC
	rmf_Error ret = RMF_SUCCESS;
	ret = podmgrStartInbandTune( ltsId, status );
        if( RMF_SUCCESS != ret )
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
                        "podmgrStartInbandTune failed in %s:%d\n",
                                 __FUNCTION__, __LINE__ ); 
		return;
        }
#else
	rmf_Error ret = RMF_SUCCESS;
	int32_t result_recv = 0;
	int8_t res = 0;

	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_START_INBAND_TUNE);

	result_recv |= ipcBuf.addData( ( const void * ) &ltsId, sizeof( ltsId ) );
	result_recv |= ipcBuf.addData( ( const void * ) &status, sizeof( status ) );

	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
			"POD Comm add data failed in %s:%d ret = %d\n",
				 __FUNCTION__, __LINE__, result_recv );
		return;
	}

	res = ipcBuf.sendCmd( pod_server_cmd_port_no );

	if ( 0 != res )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
			"POD Comm failure in %s:%d: ret = %d\n",
				 __FUNCTION__, __LINE__, res );
		return;
	}

        result_recv = ipcBuf.getResponse( &ret );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
			"POD Comm get response failed in %s:%d ret = %d\n",
				 __FUNCTION__, __LINE__, result_recv );
		return;
	}

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
		"Dumped DSG %s:%d ret = %d\n",
			 __FUNCTION__, __LINE__, ret );
#endif
	return;

}
#endif

rmf_Error rmf_podmgrGetEIDAuthStatus( PODEntitlementAuthType eidType, rmf_osal_Bool *pAuthorized )
{	
	if( pEIDStatus == NULL )
	{
		*pAuthorized = 0;
		return RMF_OSAL_ENODATA;
	}
	
	if( (eidType > POD_EID_GENERAL) && (eidType < POD_EID_MAX_NO) )
	{
		if( EID_STATUS_UNKNOWN == pEIDStatus[ eidType ].status )
		{
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
				"%s:%d EID_STATUS_UNKNOWN \n",
					 __FUNCTION__, __LINE__);				
			*pAuthorized = 0;
			return RMF_OSAL_ENODATA;
		}
		else
		{
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
				"%s:%d EID STATUS is %d \n",
					 __FUNCTION__, __LINE__, pEIDStatus[ eidType ].status );				
			*pAuthorized = ( rmf_osal_Bool )pEIDStatus[ eidType ].status;
			return RMF_SUCCESS;
		}
	}
	return RMF_OSAL_EINVAL;
	
}

rmf_Error setEIDAuthStatus( PODEntitlementAuthEvent *pAuthData )
{	
	if( pEIDStatus == NULL )
	{
		return RMF_OSAL_ENODATA;
	}
	/* Understanding:  extendedInfo is Eid type */
	if( (pAuthData->extendedInfo > POD_EID_GENERAL) && (pAuthData->extendedInfo < POD_EID_MAX_NO) )	
	{
		if( ( pAuthData->isAuthorized != 0 ) && (  pAuthData->isAuthorized != 1 ) )
		{
			RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
				"%s: Invalid Auth status %d\n",
					 __FUNCTION__, pAuthData->isAuthorized);
			return RMF_OSAL_EINVAL;
		}
		else
		{
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
				"%s: Setting Auth status of %d is %d \n",
					 __FUNCTION__, pAuthData->extendedInfo, pAuthData->isAuthorized);			
			pEIDStatus[ pAuthData->extendedInfo ].status = pAuthData->isAuthorized;
		}
	}
	return RMF_SUCCESS;
	
}

rmf_Error dispatchEntitementEvent( rmf_osal_event_params_t *pEventData )
{
	rmf_Error ret = RMF_SUCCESS;
	uint32_t event_handle;
	PODEntitlementAuthEvent *pEvent = ( PODEntitlementAuthEvent*  ) pEventData->data;
#ifdef GLI
	IARM_Result_t iResult = IARM_RESULT_SUCCESS;
#endif

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
		"%s: Dispatching EntitementEvent EIDType = 0x%x, IsAuthorized = %s EID = 0x%x \n",
			 __FUNCTION__, pEvent->extendedInfo, 
			 (pEvent->isAuthorized == 0)?"false": "true", 
			 pEvent->eId );
	
	setEIDAuthStatus( pEvent );
	
#ifdef GLI
	if( POD_EID_DISCONNECT == pEvent->extendedInfo )
	{
	        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
			"%s: IARM_Bus_BroadcastEvent IARM_RMF_POD_DISCONNECT_EVENT connected = 0x%x\n",
			__FUNCTION__, pEvent->isAuthorized);
		IARM_Bus_SYSMgr_EventData_t eventData;
		eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_DISCONNECTMGR; 

		const char *readEIDStatus = NULL;
		readEIDStatus = rmf_osal_envGet("QAMSRC.DISABLE.EID");
		if ( ( readEIDStatus == NULL) || 
			( (readEIDStatus != NULL) && (strcmp(readEIDStatus, "TRUE") ) == 0))
		{
			eventData.data.systemStates.state = 0;
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", "%s EID status update is disabled so sending connected status always",__FUNCTION__ );
		}
		else
		{	
			eventData.data.systemStates.state = ( pEvent->isAuthorized == 1 ) ? 0 : 1; 
			RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD",
				"%s: eventData.data.systemStates.state of IARM_BUS_SYSMGR_SYSSTATE_DISCONNECTMGR is = 0x%x\n",
				__FUNCTION__, eventData.data.systemStates.state );			
		}
		eventData.data.systemStates.error = 0; 
		if( IsCFWUInProgress( ) )
		{
			RDK_LOG( RDK_LOG_INFO, 
				"LOG.RDK.POD", "%s: Not sending Disconnect as cable card firmware upgrade is in progress", 
				__FUNCTION__);
		}
		else
		{
			IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));
		}
	}
#endif
	
	ret = rmf_osal_event_create( 
			RMF_OSAL_EVENT_CATEGORY_PODMGR, 
			RMF_PODMGR_EVENT_ENTITLEMENT, 
			pEventData, &event_handle);
	
	ret = rmf_osal_eventmanager_dispatch_event(
						pod_client_event_manager,
						event_handle );
	if( RMF_SUCCESS != ret )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"rmf_osal_eventmanager_dispatch_event failed in %s:%d\n",
				 __FUNCTION__, __LINE__ );						
	}

	return RMF_SUCCESS;
}

rmf_Error updateSystemIds(  )
{
	uint16_t ChannelMapID = 0;
	uint16_t ControllerID = 0;
	int32_t result_recv = 0;
	int8_t res = 0;
	struct stat sb;
	
	rmf_Error ret = RMF_SUCCESS;

	if ( rmf_podmgrIsReady() != RMF_SUCCESS )
	{
		return RMF_OSAL_ENODATA;
	}

	if ( stat("/tmp/si_acquired", &sb) == -1 )
	{
		return RMF_OSAL_ENODATA;
	}

	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_GET_SYSTEM_IDS);
	res = ipcBuf.sendCmd( pod_server_cmd_port_no );

	if ( 0 != res )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
			"POD Comm failure in %s:%d: ret = %d\n",
			__FUNCTION__, __LINE__, res );
		
		return RMF_OSAL_ENODATA;
	}
	
	result_recv = ipcBuf.getResponse( &ret );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
			"POD Comm get response failed in %s:%d ret = %d\n",
			__FUNCTION__, __LINE__, result_recv );

		return RMF_OSAL_ENODATA;
	}

	if( ret != RMF_SUCCESS )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
			"OOB returning error in %s:%d ret = %d\n",
			__FUNCTION__, __LINE__, ret);
		
		return RMF_OSAL_ENODATA;
	}

	result_recv |= ipcBuf.collectData( (void *)ChannelMapID, sizeof(uint16_t) );
	result_recv = ipcBuf.collectData( (void *)ControllerID, sizeof(uint16_t) );

	if( result_recv != 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD",
			"IPCBuf collect data failed in %s:%d: %d\n",
			__FUNCTION__, __LINE__ , result_recv);
		
		return RMF_OSAL_ENODATA;
	}

	gChannelMapId = ChannelMapID;
	gControllerId = ControllerID;

	RDK_LOG (RDK_LOG_INFO, "LOG.RDK.POD", 
		"%s() Channel Map ID: %d\n", __FUNCTION__,ChannelMapID);
	RDK_LOG (RDK_LOG_INFO, "LOG.RDK.POD", 
		"%s() Controller ID: %d\n", __FUNCTION__, ControllerID);
	
	return RMF_SUCCESS;
}

rmf_Error rmf_podmgrNotifySASTerm( )
{
	IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_SERVER_CMD_NOTIFY_SAS_TERMINATION );
	int32_t result_recv = 0;
	rmf_Error ret = RMF_SUCCESS;

	int8_t res = ipcBuf.sendCmd( pod_server_cmd_port_no );
	if ( 0 != res )
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"%s: Lost connection with POD. res = %d\n",
				 __FUNCTION__, res );
		return RMF_OSAL_ENODATA;
	}	

	result_recv = ipcBuf.getResponse( &ret );
	if( result_recv < 0)
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", 
			"%s: No response from POD. result_recv = %d\n",
				 __FUNCTION__, result_recv );	
		return RMF_OSAL_ENODATA;
	}

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.POD", 
		"%s: notfied vod crash = %d\n",
			 __FUNCTION__, ret );	
	return ret; 
}


