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
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <assert.h>
#include "rdk_debug.h"
#include "rmf_osal_util.h"
#include "rmf_osal_ipc.h"
#ifdef TEST_WITH_PODMGR
# include "podServer.h"
# include "hostGenericFeatures.h"
#endif

#include "rmf_sectionfilter_oob.h"
#include "rmf_oobsisttmgr.h"
#ifdef GLI
#include "sysMgr.h"
#include "libIBus.h"
#endif
#if USE_SYSRES_MLT
#include "rpl_malloc.h"
#include "rpl_new.h"
#endif
#ifdef YOCTO_BUILD
extern "C"{
#include "secure_wrapper.h"
}
#endif

#ifdef LOGMILESTONE
#include "rdk_logger_milestone.h"
#endif

/**
 * @file rmf_oobsisttmgr.cpp
 * @brief Out of band SI system time table managaer
 */


#define MAX_EVTQ_NAME_LEN 100
#define EPOCH_DIFF 315964819L /*difference between UNIX and GPS epoch*/
//#define TIME_TOLERANCE_VALUE 5L       /*tolerance value in seconds for setting the time*/
#define TIME_TOLERANCE_VALUE 1L /*tolerance value in seconds for setting the time*/
#define RMF_SF_FILTER_PRIORITY_SITP 2

// Defines for sending IPC commands for timezone and current time in rmfStreamer
#ifdef USE_POD_IPC
#define POD_HAL_SERVICE_CMD_DFLT_PORT_NO 50509
#define POD_HAL_SERVER_CMD_CONFIG_TIMEZONE 4
#define POD_HAL_SERVER_CMD_CONFIG_TIMEOFDAY 5
#endif
static bool post_stt_event = true;
static volatile uint32_t pod_hal_service_cmd_port_no;
rmf_OobSi_Stt_Timezone m_timezoneinfo[6] ={
                            {-11,"HST11","HST11HDT,M3.2.0,M11.1.0"},
                            {-9,"AKST","AKST09AKDT,M3.2.0,M11.1.0"},
                            {-8,"PST08","PST08PDT,M3.2.0,M11.1.0"},
                            {-7,"MST07","MST07MDT,M3.2.0,M11.1.0"},
                            {-6,"CST06","CST06CDT,M3.2.0,M11.1.0"},
                            {-5,"EST05","EST05EDT,M3.2.0,M11.1.0"}};

rmf_OobSiSttMgr *rmf_OobSiSttMgr::m_pOobSiSttMgr = NULL;

rmf_OobSiSttMgr::rmf_OobSiSttMgr()
{
    m_si_SttThreadID = (rmf_osal_ThreadId) -1;
    m_si_SttFilterQueue = 0; // OOB STT thread event queue
    m_table_unique_id = 0xFFFFFFFF;
    m_bRecievedStt = 0;
    m_bRecievedTZ  = 0;
    m_bRecievedDS  = 0;
    m_bKeepSystemTimeSynchedWithCableTime = 0;
    m_CurrentTimeInSecs = 0;
    m_GpsUtcOffset = 0;
    m_queueId = 0;
    const char *hal_port;
    hal_port = rmf_osal_envGet( "POD_HAL_SERVICE_CMD_PORT_NO" );
    if( NULL == hal_port)
    {
    	RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.POD", "%s: NULL POINTER  received for name string\n", __FUNCTION__ );
#ifdef TEST_WITH_PODMGR
    	pod_hal_service_cmd_port_no = POD_HAL_SERVICE_CMD_DFLT_PORT_NO;
#endif
    }
    else
    {
	    pod_hal_service_cmd_port_no = atoi( hal_port );
    }		

#ifdef TEST_WITH_PODMGR
//    m_pOobSectionFilter = rmf_SectionFilter::create(rmf_filters::oob,NULL);
    m_pOobSectionFilter = new rmf_SectionFilter_OOB( NULL );
#endif

    m_pSiCache = rmf_OobSiCache::getSiCache();

    InitSystemTimeTask();
}

rmf_OobSiSttMgr::~rmf_OobSiSttMgr()
{

}

unsigned char rmf_OobSiSttMgr::isFeatureEnabled(const char *featureString)
{
    const char* env = NULL;
    
    if ((env = rmf_osal_envGet((char *)featureString)) != NULL)
    {
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD","\n%s=%s\n", featureString, env);
        if(strcmp(env,"FALSE") == 0) 
        {
            //RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.MEDIA","\n featureString %s is not supported in this Platform\n", featureString);
            return 0;
        }
    }
    else
    {
      return 0;
    }
    
    return 1;
}

void rmf_OobSiSttMgr::InitSystemTimeTask()
{
#ifdef TEST_WITH_PODMGR
        rmf_FilterSpec    filterSpec;
#endif
        rmf_Error retCode = RMF_SUCCESS;
        /*
        * SI STT table filter params
        */
        uint8_t stt_tid_mask[]      = {0xFF,0,0,0,0,0,0};
        uint8_t stt_tid_val[]       = {0xC5,0,0,0,0,0,0};
        uint8_t neg_mask[]      = {0};
        uint8_t neg_val[]       = {0};
        uint16_t pid = 0;
        uint8_t name[MAX_EVTQ_NAME_LEN];

        if(isFeatureEnabled("FEATURE.KEEP_SYSTEM_TIME_SYNCED_WITH_CABLE_TIME"))
        {
            m_bKeepSystemTimeSynchedWithCableTime=1;
        }

        if(!isFeatureEnabled("FEATURE.STT.SUPPORT"))
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI","VL STT FEATURE IMPLEMENTATION IS NOT ENABLED. ENABLE FEATURE.STT.SUPPORT TO TRUE IF YOU WANT STT UPDATE FEATURE\n");
            return;
        }

        //***********************************************************************************************
        //**** Create a global event queue for the Thread
        //***********************************************************************************************
        snprintf( (char*)name, MAX_EVTQ_NAME_LEN, "SiSTT");
        if ((retCode = rmf_osal_eventqueue_create(name, &m_si_SttFilterQueue)) != RMF_SUCCESS)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI","unable to create event queue....\n");
            return ;
        }
        if (m_pSiCache->getParseSTT()== TRUE)
        {
            retCode = rmf_osal_threadCreate(siSTTWorkerThread, this, RMF_OSAL_THREAD_PRIOR_SYSTEM, RMF_OSAL_THREAD_STACK_SIZE,
                                                                &m_si_SttThreadID, "STTWorkerThread");

            if(retCode != RMF_SUCCESS)
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI","unable to create Thread for STT Monitoring.\n");
                return ;
            }
        }

#ifdef TEST_WITH_PODMGR
        filterSpec.pos.length = 7;
        filterSpec.pos.mask = stt_tid_mask;
        filterSpec.pos.vals = stt_tid_val;

        filterSpec.neg.length = 0;
        filterSpec.neg.mask = neg_mask;
        filterSpec.neg.vals = neg_val ;

        pid             =  0x1FFC; //Special PID value for OOB

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI"," Creating STT Section Filter...\n");
        /* Set and Start the filter */
        retCode = m_pOobSectionFilter->SetFilter(pid,
                                    &filterSpec,
                                    m_si_SttFilterQueue,
                                    RMF_SF_FILTER_PRIORITY_SITP,
                                    0 /*5*/,//Number of times to match. 0 is indefinite until the filter is cancelled
                                    0,
                                    &m_table_unique_id);
    if(RMF_SUCCESS != retCode)
    {
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI","Failed to create STT Section Filter...\n");
    } 
#endif  

}

void rmf_OobSiSttMgr::siSTTWorkerThread ( void   *data)
{
    rmf_OobSiSttMgr *pOobSiSttMgr = (rmf_OobSiSttMgr *)data;
    pOobSiSttMgr->siSTTWorkerThreadFn();
}

rmf_Error rmf_OobSiSttMgr::update_systime(unsigned long t, bool fromNTP)
{
#ifdef TEST_WITH_PODMGR
        static unsigned long prev_time=0L;
        int ret;
        struct timeval timesec, currtime;
        static int firstTime=1;
        int daylightflag = 0;
        
#if 0 //To be enabled if required
        const char * szTimeZoneStr = NULL;
        static time_t t_prev = 0;
        VL_POD_HOST_FEATURE_DAYLIGHT_SAVINGS dstinfo;
#endif

        VL_POD_HOST_FEATURE_TIME_ZONE_OFFSET timezoneinfo;
        int timezoneinhours = 0;
        //VL_HOST_GENERIC_FEATURES_RESULT iRet;
        int iRet;

        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SYS","Entering... update_Systime::%s:t=%lu\n", __FUNCTION__,t);

    //Prasad (11/01/2010)
    //Save away the Current time that we got in STT
    m_CurrentTimeInSecs = t; //Before adding the EPOCH_DIFF;

    //Get the TimeZone and DST info and set the TZ env variable
    iRet = vlPodGetGenericFeatureData(VL_HOST_FEATURE_PARAM_Time_Zone, &timezoneinfo);
    if(/*VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS*/ 0 != iRet)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SYS","%s: vlPodGetGenericFeatureData for VL_HOST_FEATURE_PARAM_Time_Zone returned Error !! :%d \n", __FUNCTION__, iRet);
    }
    else
    {
      timezoneinhours = timezoneinfo.nTimeZoneOffset/60;
    }

    if(timezoneinhours >= -12 && timezoneinhours <=12 )
    {
        daylightflag = CheckToApplyDST();

        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI", "%s: Calling vlSetTimeZone with daylightflag %d timezoneinhours %d\n", __FUNCTION__, daylightflag,timezoneinhours);
        setTimeZone(timezoneinhours, daylightflag);
        //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SYS", "TZ is %s\n",getenv("TZ"));
    }
    //End of Change - Prasad (11/01/2010)

    //timesec.tv_sec = t + EPOCH_DIFF; // UTC seconds since Jan 1 1970
	if (!fromNTP)
	{
    	timesec.tv_sec = t + EPOCH_DIFF - 19; // (Workaround for making  UTC time to be same as the NTP time)
	}
	else 
	{
    	RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS","update_Systime: time is from ntp...\n");
          // for the IP direct case, we need to unblock the SI worker thread to process SI data
          // because STT will not be received in this mode only time via NTP.
          rmf_osal_TimeMillis currTimeInMillis = 0;
          rmf_osal_TimeMillis currTimeInSecs = 0;
          unsigned long localTimeInSecs = 0;
          currTimeInSecs = t;
          currTimeInMillis = currTimeInSecs*1000;
          localTimeInSecs = currTimeInSecs;

          if (post_stt_event == true)
          {
#ifdef ENABLE_TIME_UPDATE
            PostSTTEvent(SI_STT_ACQUIRED,NULL,currTimeInSecs);
#else
            PostSTTArrivedEvent(SI_STT_ACQUIRED,localTimeInSecs,currTimeInSecs,currTimeInMillis);
#endif
            post_stt_event = false;
          }
		timesec.tv_sec = t; // from NTP failover server
	}

    timesec.tv_usec = 0;

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SYS","update_Systime: system time in UTC is %lu\n",timesec.tv_sec);

    ret = gettimeofday(&currtime,NULL);
    if(ret != 0)
    {
    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SYS","update_Systime: Unable to get the system time\n");
            return RMF_OSAL_ENODATA;
    }

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SYS","update_Systime: updating the system time. currtime.tv_sec = %d, timesec.tv_sec = %d\n", currtime.tv_sec, timesec.tv_sec);

    //Error Checking. We cannot assume that STT has the right time, so try to adapt and be as right(not only with regards
    //to time but with regards to stability of components dependent on time) as possible if the STT has error in time.
    //if(timesec.tv_sec > prev_time) // The STT does not go in to the past
    {
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SYS","timesec.tv_sec = %d prev_time = %d\n",timesec.tv_sec,(prev_time + (unsigned long)2));
      //if(firstTime ==1 ||timesec.tv_sec <= (prev_time + (unsigned long)2)) // This means the STT time has suddenly incremented by more than what we expected(a second to be precise but we give leeway till 2) and we dont want to process it.
      {
              if(firstTime ==1 || (currtime.tv_sec <= timesec.tv_sec - TIME_TOLERANCE_VALUE) || (currtime.tv_sec >= timesec.tv_sec + TIME_TOLERANCE_VALUE)) // Check if the clock on our system differs from the STT by more than a few seconds. If so apply it
              {
                    static int firstparse = 1;
                    static int64_t delta = 0;
                    int64_t incdelta = 0;
                    firstTime = 0;
#ifdef LOGMILESTONE
                    logMilestone("SYSTEM_TIME_UPDATED");
#else
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS","SYSTEM_TIME_UPDATED \n"); 
#endif
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS","update_Systime: updating the system time. \n"); 
                    //set the system time using pfctime..pfcTime sets using settimeofday
                    write_time((long long) timesec.tv_sec);

                    if(firstparse)
                    {
                            delta= timesec.tv_sec - currtime.tv_sec;
                            firstparse = 0;
                    }
                    else
                    {
                            incdelta = timesec.tv_sec - currtime.tv_sec;
                    }
              }
      }
    }

    //prev_time = t + EPOCH_DIFF;
    prev_time = t + EPOCH_DIFF - 19; //(Workaround for making  UTC time to be same as the NTP time)

    time_t rawTime;
    time(&rawTime);

// Need to ensure whether this is required or not
#if 0  
    // ++ RMP - the xcc_gettimeofday() API needs to be informed of the time change...
        mpeos_resynchronize_xcc_gettimeofday();
    // -- RMP
#endif

    m_bRecievedStt = 1;
#if 0
    if(NULL != (szTimeZoneStr = vlPodGetTimeZoneString()))
    {
        if(strlen(szTimeZoneStr) > 0)
        {
            setenv("TZ", szTimeZoneStr, 1);
        }
    }
#endif

//Need to enable this code after checking the status of front panel module
#if 0 
    if(vlAbs(rawTime - t_prev) > 15*60)
    {
        t_prev = rawTime;
        vlSttSetFpdTime();
    }
#endif

#endif //TEST_WITH_PODMGR
	return RMF_SUCCESS;
}

#ifndef ENABLE_TIME_UPDATE
#ifdef USE_POD_IPC
int rmf_OobSiSttMgr::sendConfigTimezoneCmd(int timezoneinhours,int daylightflag)
{
        rmf_Error ret = RMF_SUCCESS;
        int32_t result_recv = 0;
        int8_t res = 0;
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.SI",
			"\nEnter %s:%d: timezoneinhours: %d, daylightflag: %d\n",
			__FUNCTION__, __LINE__ , timezoneinhours, daylightflag);

        IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_HAL_SERVER_CMD_CONFIG_TIMEZONE);
        result_recv |= ipcBuf.addData( ( const void * ) &timezoneinhours, sizeof( timezoneinhours ) );
        result_recv |= ipcBuf.addData( ( const void * ) &daylightflag, sizeof( daylightflag ) );

        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SI",
                        "\nPOD Comm add data failed in %s:%d: %d\n",
                                 __FUNCTION__, __LINE__ , result_recv);
                return RMF_OSAL_ENODATA;
        }

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.SI",
			"\nSending POD_HAL_SERVER_CMD_CONFIG_TIMEZONE to rmfStreamer process. %s:%d: %d\n",
			__FUNCTION__, __LINE__ , result_recv);

        res = ipcBuf.sendCmd( pod_hal_service_cmd_port_no );

        if ( 0 != res )
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SI",
                        "POD Comm failure in %s:%d: ret = %d\n",
                                 __FUNCTION__, __LINE__, res );
                return RMF_OSAL_ENODATA;
        }

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.SI",
			"\nWaiting for response from rmfStreamer process. %s:%d: %d\n",
			__FUNCTION__, __LINE__ , result_recv);

        result_recv = ipcBuf.getResponse( &ret );
        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SI",
                        "POD Comm get response failed in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, result_recv );
                return RMF_OSAL_ENODATA;
        }

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.SI",
			"\nExit. %s:%d\n",
			__FUNCTION__, __LINE__);

        return ret;
}

int rmf_OobSiSttMgr::sendConfigTimeofDayCmd(int64_t timevalue)
{
        rmf_Error ret = RMF_SUCCESS;
        int32_t result_recv = 0;
        int8_t res = 0;
	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.SI",
			"\nEnter %s:%d: timevalule: %lld, POD_HAL_SERVER_CMD_CONFIG_TIMEOFDAY: %d\n",
			__FUNCTION__, __LINE__ , timevalue, POD_HAL_SERVER_CMD_CONFIG_TIMEOFDAY);

        IPCBuf ipcBuf = IPCBuf( ( uint32_t ) POD_HAL_SERVER_CMD_CONFIG_TIMEOFDAY);
        result_recv |= ipcBuf.addData( ( const void * ) &timevalue, sizeof( timevalue ) );

        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SI",
                        "\nPOD Comm add data failed in %s:%d: %d\n",
                                 __FUNCTION__, __LINE__ , result_recv);
                return RMF_OSAL_ENODATA;
        }

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.SI",
			"\nSending POD_HAL_SERVER_CMD_CONFIG_TIMEOFDAY to rmfStreamer process. %s:%d: %d\n",
			__FUNCTION__, __LINE__ , result_recv);

        res = ipcBuf.sendCmd( pod_hal_service_cmd_port_no );

        if ( 0 != res )
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SI",
                        "POD Comm failure in %s:%d: ret = %d\n",
                                 __FUNCTION__, __LINE__, res );
                return RMF_OSAL_ENODATA;
        }

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.SI",
			"\nWaiting for response from rmfStreamer process. %s:%d: %d\n",
			__FUNCTION__, __LINE__ , result_recv);

        result_recv = ipcBuf.getResponse( &ret );
        if( result_recv < 0)
        {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SI",
                        "POD Comm get response failed in %s:%d ret = %d\n",
                                 __FUNCTION__, __LINE__, result_recv );
                return RMF_OSAL_ENODATA;
        }

	RDK_LOG( RDK_LOG_INFO, "LOG.RDK.SI",
			"\nExit. %s:%d\n",
			__FUNCTION__, __LINE__);

        return ret;
}
#endif
#endif


/**
 * @fn rmf_OobSiSttMgr::setTimeZone(int timezoneinhours,int daylightflag)
 * @brief This function sets the time zone in hours and the day light saving information.
 * Time zone values can range from -12 to +13 hours.
 * IARM manager event emits the time zone information to the other modules through IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE event id.
 *
 * @param[in] Timezoneinhours Pointer to populate time zone in hours.
 * @param[in] Daylightflag Pointer to daylight flag. 0 indicates that not in daylight saving time and 1 indicates that its in daylight saving time.
 *
 * @return None.
 */
void rmf_OobSiSttMgr::setTimeZone(int timezoneinhours,int daylightflag)
{
#ifdef ENABLE_TIME_UPDATE
	rmf_timeinfo_t *pTimeInfo;
#endif
	static int8_t prevIndex = -1;
	static int prevDaylightflag = -1;
	int8_t index = -1;
        //Currently only the main time zones in US from west to east
        switch(timezoneinhours){
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
        }

	if( -1  != index )
	{
		if( daylightflag ) 
		{
			setenv( "TZ", m_timezoneinfo[ index ].dsttimezoneinchar, 1 );
		}
		else
		{
			setenv( "TZ", m_timezoneinfo[ index ].timezoneinchar, 1 );	
		}

#ifdef GLI
		IARM_Bus_SYSMgr_EventData_t eventData;
		if(( prevDaylightflag != daylightflag ) ||
			( prevIndex != index ) )
		{
			eventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_TIME_ZONE; 
			eventData.data.systemStates.state = 2; 
			eventData.data.systemStates.error = 0; 
			if( daylightflag ) 
			{
				snprintf( eventData.data.systemStates.payload, 
					sizeof(eventData.data.systemStates.payload),"%s", m_timezoneinfo[ index ].dsttimezoneinchar );
			}
			else
			{
				snprintf( eventData.data.systemStates.payload, 
					sizeof(eventData.data.systemStates.payload),"%s", m_timezoneinfo[ index ].timezoneinchar );	
			}
			IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t) IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, (void *)&eventData, sizeof(eventData));			
			prevDaylightflag = daylightflag;
			prevIndex = index;
		}
#endif
	}


#ifdef ENABLE_TIME_UPDATE
	rmf_Error ret = RMF_SUCCESS;

	ret = rmf_osal_memAllocP(RMF_OSAL_MEM_SI_OOB, sizeof(rmf_timeinfo_t), (void**)&pTimeInfo);
        if(pTimeInfo != NULL)
        {
	    pTimeInfo->timezoneinfo.timezoneinhours = timezoneinhours;
	    pTimeInfo->timezoneinfo.daylightflag = daylightflag;

            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS","%s: Sending RMF_SI_EVENT_STT_TIMEZONE_SET(0x%x) to OOB SI Manager, pTimeInfo: %x, pTimeInfo->timezoneinfo.timezoneinhours: %d, pTimeInfo->timezoneinfo.daylightflag: %d\n", __FUNCTION__, RMF_SI_EVENT_STT_TIMEZONE_SET, pTimeInfo, pTimeInfo->timezoneinfo.timezoneinhours, pTimeInfo->timezoneinfo.daylightflag);
	    PostSTTEvent(RMF_SI_EVENT_STT_TIMEZONE_SET, (void*)pTimeInfo, 0); 
        }
#else
#ifdef USE_POD_IPC
#ifdef USE_NON_IARM_METHOD
		sendConfigTimezoneCmd(timezoneinhours,daylightflag);
#endif
#endif
#endif


}

void rmf_OobSiSttMgr::storeSysTime()
{
#ifdef XONE_STB
  system("date \"+%Y%m%d%H%M\" > /tmp/.systime");
#else
  system("date \"+%m%d%H%M%Y\" > /tmp/.systime");
#endif
}

void rmf_OobSiSttMgr::siSTTWorkerThreadFn ()
{
    rmf_osal_event_handle_t si_event_handle  = 0;
    rmf_osal_event_params_t event_params = {0};

#ifdef TEST_WITH_PODMGR
    rmf_FilterSectionHandle hSection = 0;
#endif

    uint32_t si_event_type;
    rmf_Error retCode = RMF_SUCCESS;
    uint32_t numBytesCopied = 0, byteCount;
    uint8_t  inBuffer[256] ;

    while(1)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI", "%s: WAITING FOR STT EVENT\n", __FUNCTION__);

        retCode = rmf_osal_eventqueue_get_next_event (m_si_SttFilterQueue, &si_event_handle, NULL, &si_event_type, &event_params);
        if(RMF_SUCCESS != retCode)
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI", "%s: TIMED OUT IN GETTING STT EVENT. SHOULD NOT HAPPEN AS EVENT WAIT IS FOREVER\n", __FUNCTION__);
            continue;
        }

        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI", "%s: GOT  STT EVENT\n", __FUNCTION__);
        switch ( si_event_type )
        {
#ifdef TEST_WITH_PODMGR            
          case RMF_SF_EVENT_SECTION_FOUND:
          case RMF_SF_EVENT_LAST_SECTION_FOUND:
          {
                  retCode    = m_pOobSectionFilter->GetSectionHandle( m_table_unique_id, 0, &hSection);
                  if(RMF_SUCCESS != retCode)
                        continue;

                  retCode = m_pOobSectionFilter->GetSectionSize(hSection, &byteCount);
                  if(RMF_SUCCESS != retCode)
                        continue;

                  retCode = m_pOobSectionFilter->ReadSection (hSection, 0, byteCount, RMF_SF_OPTION_RELEASE_WHEN_COMPLETE /*0*/,
                                                                                        inBuffer, &numBytesCopied) ;
                  if(RMF_SUCCESS != retCode)
                        continue;

                  parse_update_time(inBuffer);
          if(!m_bKeepSystemTimeSynchedWithCableTime)
          {
              m_pOobSectionFilter->CancelFilter(m_table_unique_id);
              m_table_unique_id = 0xFFFFFFFF;
          }
        }
        break;
#endif          
        default:
            break;
        }

        rmf_osal_event_delete(si_event_handle);

    storeSysTime();
    }
}

void rmf_OobSiSttMgr::parse_update_time(unsigned char *input_buffer)
{
          unsigned long localTimeInSecs = 0;
          struct rmf_OobSi_Stt_tbl_header *pSttTableHdr;
          bool sttStatus = false;
          pSttTableHdr  = (struct rmf_OobSi_Stt_tbl_header*)input_buffer;
#if 0
          RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS","TABLE ID = 0x%x \n",  pSttTableHdr->table_id);
          RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS","Section length = %d \n",pSttTableHdr->section_len);
          RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS","Protocol Ver = %d \n", pSttTableHdr->protocol_ver);
          RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS","GPS Offset = %d \n",  pSttTableHdr->gps_utc_offset);
#endif
          unsigned long systemTime = ((pSttTableHdr->sys_time.byte1 << 24) | (pSttTableHdr->sys_time.byte2 << 16) | (pSttTableHdr->sys_time.byte3 << 8) | (pSttTableHdr->sys_time.byte4));

          RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI", "TABLE ID = 0x%x, Section length = %d, Protocol Ver = %d, GPS Offset = %d \n", pSttTableHdr->table_id,
                          pSttTableHdr->section_len, pSttTableHdr->protocol_ver, pSttTableHdr->gps_utc_offset );

          //printf("System Time = 0x%x , systemTime = 0x%x\n", pSttTableHdr->sys_time, systemTime);
          RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI", "System Time = 0x%x , systemTime = 0x%x\n", pSttTableHdr->sys_time, systemTime);

          //
          localTimeInSecs = systemTime - pSttTableHdr->gps_utc_offset;
          rmf_osal_TimeMillis currTimeInMillis = 0;
          rmf_osal_TimeMillis currTimeInSecs = 0;
          int ret;
          struct timeval tv;
          struct timezone tz;

          ret = gettimeofday(&tv,&tz);
          if (0 == ret)
          {
            currTimeInSecs = tv.tv_sec;
          }
          currTimeInMillis = currTimeInSecs*1000;
          currTimeInMillis += (tv.tv_usec / 1000);

          //Update the System time
          RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI", "currTimeInSecs = 0x%x , currTimeInMillis = 0x%x\n", currTimeInSecs,currTimeInMillis);
          m_pSiCache->LockForRead();
          sttStatus = m_pSiCache->getSTTAcquiredStatus();
          m_pSiCache->UnLock();
          if ( FALSE == sttStatus )
          {
              m_pSiCache->LockForWrite();
              m_pSiCache->setSTTAcquiredStatus( TRUE );
              m_pSiCache->ReleaseWriteLock();
              update_systime(localTimeInSecs);
          }
          RDK_LOG(RDK_LOG_WARN, "LOG.RDK.SI", "sending SI_STT_ACQUIRED event\n");
#ifdef ENABLE_TIME_UPDATE
          PostSTTEvent(SI_STT_ACQUIRED,NULL,currTimeInSecs);
#else
          PostSTTArrivedEvent(SI_STT_ACQUIRED,localTimeInSecs,currTimeInSecs,currTimeInMillis);
#endif


}

void rmf_OobSiSttMgr::write_time (int64_t value)
{
        struct timeval timesec;
        //time_t rawTime;

        timesec.tv_sec = value;
        timesec.tv_usec = 0;

        settimeofday (&timesec, NULL);
#ifdef ENABLE_TIME_UPDATE
	rmf_Error ret = RMF_SUCCESS;
	rmf_timeinfo_t *pTimeInfo = NULL;

	ret = rmf_osal_memAllocP(RMF_OSAL_MEM_SI_OOB, sizeof(rmf_timeinfo_t), (void**)&pTimeInfo);
        if(pTimeInfo != NULL)
        {
	    pTimeInfo->timevalue = value;
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS","%s: Sending RMF_SI_EVENT_STT_SYSTIME_SET(0x%x) to OOB SI Manager, pTimeInfo: 0x%x\n", __FUNCTION__, RMF_SI_EVENT_STT_SYSTIME_SET, &pTimeInfo);
	    PostSTTEvent(RMF_SI_EVENT_STT_SYSTIME_SET, (void*)pTimeInfo, sizeof(rmf_timeinfo_t));
        }
#else
#ifdef USE_POD_IPC
	sendConfigTimeofDayCmd(value);
#endif
#endif
}

int rmf_OobSiSttMgr::CheckToApplyDST(void)
{
  static int daylightFlag = 0;
#ifdef TEST_WITH_PODMGR
  //VL_HOST_GENERIC_FEATURES_RESULT iRet;
  int iRet;

  VL_POD_HOST_FEATURE_DAYLIGHT_SAVINGS dstinfo;
  iRet = vlPodGetGenericFeatureData(VL_HOST_FEATURE_PARAM_Daylight_Savings_Control, &dstinfo);
  if(/*VL_HOST_GENERIC_FEATURES_RESULT_SUCCESS*/ 0 != iRet)
  {
    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD","%s: vlPodGetGenericFeatureData for VL_HOST_FEATURE_PARAM_Daylight_Savings_Control returned Error !! :%d \n", __FUNCTION__, iRet);
    return daylightFlag;
  }
  else
  {
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: dstinfo.eDaylightSavingsType %d dstinfo.nDaylightSavingsDeltaMinutes %ld dstinfo.tmDaylightSavingsEntryTime %ld dstinfo.tmDaylightSavingsExitTime %ld\n", __FUNCTION__, dstinfo.eDaylightSavingsType,dstinfo.nDaylightSavingsDeltaMinutes,dstinfo.tmDaylightSavingsEntryTime,dstinfo.tmDaylightSavingsExitTime);
  }

  if(dstinfo.eDaylightSavingsType == 0x02)
  {
      if(dstinfo.nDaylightSavingsDeltaMinutes <= 60)
      {
        if( (m_CurrentTimeInSecs >= (dstinfo.tmDaylightSavingsEntryTime + 315964800) ) && (m_CurrentTimeInSecs <= (dstinfo.tmDaylightSavingsExitTime + 315964800) ) )
        {
          daylightFlag = 1;
        }
        else
        {
          RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: ********************* DAYLIGHT Entry and Exit times are Incorrect. Forcing the Daylight setting based on DaylightSavingsType flag alone*********************\n", __FUNCTION__);
          //daylightFlag = 0;
          daylightFlag = 1;
        }
      }
  }
  else if(dstinfo.eDaylightSavingsType == 0x01)
  {
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: Setting daylightFlag to false\n",__FUNCTION__);
    daylightFlag = 0;
  }
#endif //TEST_WITH_PODMGR

  return daylightFlag;

}

#ifdef ENABLE_TIME_UPDATE
void rmf_OobSiSttMgr::PostSTTEvent(int sttEvent, void *eventdata, int data_extension)
{
    rmf_Error err = RMF_SUCCESS;
    rmf_osal_TimeMillis sttStartTime = 0;
    rmf_osal_event_handle_t stt_event_handle ;
    rmf_osal_event_params_t event_params = {0};

    if(m_queueId != 0)
    {
        switch (sttEvent)
	{
            case SI_STT_ACQUIRED:
            {
                rmf_osal_timeGetMillis(&sttStartTime);
                m_pSiCache->setSTTStartTime(sttStartTime);
                err = RMF_SUCCESS;
            }
            break;

            case RMF_SI_EVENT_STT_TIMEZONE_SET:
            case RMF_SI_EVENT_STT_SYSTIME_SET:
            {
                err = RMF_SUCCESS;
            }
            break;

            default:
                err = RMF_OSAL_EINVAL;
                break;
	}

        if(err == RMF_SUCCESS)
        {
            event_params.data = eventdata;
            event_params.data_extension = data_extension;
            err = rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_OOBSI, sttEvent,
                    &event_params, &stt_event_handle);

            if(err != RMF_SUCCESS)
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI","%s: Cannot create RMF OSAL Event \n", __FUNCTION__);
                return;
            }
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS","%s: Sending event 0x%x to OOB SI Manager\n", __FUNCTION__, sttEvent);
            err = rmf_osal_eventqueue_add(m_queueId, stt_event_handle);
            if(err != RMF_SUCCESS)
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI","%s: Cannot add event to registered queue\n", __FUNCTION__);
                rmf_osal_event_delete(stt_event_handle);
            }
        }
    }
}
#else
void rmf_OobSiSttMgr::PostSTTArrivedEvent(int sttEvent, int nEventData,int privTimeInSec,int privTimeInMillis)
{
    rmf_Error err = RMF_SUCCESS;
    unsigned long system_time = nEventData;
    unsigned long GPSTimeInUnixEpoch = system_time + 315964800;
    rmf_osal_TimeMillis sttStartTime = 0;
    rmf_osal_event_handle_t stt_event_handle;
    rmf_osal_event_params_t event_params = {0};
    time_t rawTime;
	
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SYS","vlMpeosPostSTTArrivedEvent sttEvent =%d and nEventData =%d\n",sttEvent,nEventData);

    if(m_queueId != 0)
    {
        event_params.data = NULL;
        event_params.data_extension = privTimeInSec;

        err = rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_OOBSI, sttEvent,
                &event_params, &stt_event_handle);

        if(err != RMF_SUCCESS)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI","%s: Cannot create RMF OSAL Event \n", __FUNCTION__);
            return;
        }
        
        err = rmf_osal_eventqueue_add(m_queueId, stt_event_handle);
        if(err != RMF_SUCCESS)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI","%s: Cannot add event to registered queue\n", __FUNCTION__);
            rmf_osal_event_delete(stt_event_handle);
            return;
        }
        
    }

    rmf_osal_timeGetMillis(&sttStartTime);
    m_pSiCache->setSTTStartTime(sttStartTime);

    // get current time.
    time(&rawTime);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SYS",
            "<%s::STTArrivedEvent, setSTTTime(%ld, %lld), the current local time %s\n", __FUNCTION__,
            GPSTimeInUnixEpoch, m_pSiCache->getSTTStartTime(), ctime(&rawTime));

// Need to enable after verifying whether we need to set JVM time or not in RMF
#if 0
    // Set the stt time
    setSTTTime(GPSTimeInUnixEpoch, m_pSiCache->getSTTStartTime());
#endif

}
#endif


/**
 * @fn rmf_OobSiSttMgr::sttRegisterForSTTArrivedEvent(rmf_osal_eventqueue_handle_t eventQueueID)
 * @brief  This function is used to register the given queue to receive STT arrival events.
 *
 * @param[in] eventQueueID Default eventQueueID.
 *
 * @return rmf_Error
 * @retval RMF_SUCCESS If successfully register the STT for STT arrived event.
 */
rmf_Error rmf_OobSiSttMgr::sttRegisterForSTTArrivedEvent(rmf_osal_eventqueue_handle_t eventQueueID)
{
        if(m_queueId == 0)
        {
            m_queueId = eventQueueID;
        }

	return RMF_SUCCESS;
}


/**
 * @fn rmf_OobSiSttMgr::getInstance(void)
 * @brief This function initializes and creates the instance of Oobsistmgr.
 *
 * @return rmf_OobSiSttMgr
 * @retval m_pOobSiSttMgr Pointer to the instance of Oobsistmgr.
 */
rmf_OobSiSttMgr *rmf_OobSiSttMgr::getInstance(void)
{
    if(m_pOobSiSttMgr == NULL)
    {
        m_pOobSiSttMgr = new rmf_OobSiSttMgr();
    }

    return m_pOobSiSttMgr;
}
