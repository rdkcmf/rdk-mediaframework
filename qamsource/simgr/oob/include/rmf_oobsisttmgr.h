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
 

#ifndef __RMF_OOBSISTTMGR_H__
#define __RMF_OOBSISTTMGR_H__

#include "rmf_osal_thread.h"
#ifdef TEST_WITH_PODMGR
#include "rmf_sectionfilter.h"
#endif
#include "rmf_oobsicache.h"
//#include <sys/time.h>
//#include "cMisc.h"
//#include "sys_init.h"
//#include "vl_pod_types.h"
//#include "mpeos_frontpanel.h"
//#include "vlMpeosFrontPanel.h"
//#include "utilityMacros.h"
//#include "parker_fpd_api.h"
//#include "sys_api.h"

/**
 * @file rmf_oobsisttmgr.h
 * @brief File contains the system time table data.
 */

/**
 * @enum rmf_OobSi_Stt_TimeZones
 * @brief This enumeration represents the valid values of the different timezones standard.
 * @ingroup Outofbanddatastructures
 */
typedef enum
{
        HST=-11,
        AKST=-9,
        PST,
        MST,
        CST,
        EST
} rmf_OobSi_Stt_TimeZones;

typedef struct {
  int timezoneinhours;
  char timezoneinchar[8];
  //char dsttimezoneinchar[16];
  char dsttimezoneinchar[26];
}rmf_OobSi_Stt_Timezone;

//Need to take care of defining these structures for BIG ENDIAN systems.
typedef struct _sysTime
{
  unsigned char                 byte1   : 8;
  unsigned char                 byte2   : 8;
  unsigned char                 byte3   : 8;
  unsigned char                 byte4   : 8;
}rmf_OobSi_Stt_SystemTime;

/**
 * STT table data.
 */
struct rmf_OobSi_Stt_tbl_header {
        unsigned char                   table_id;           //!< Table ID.
        unsigned short                  section_len : 12;       //!< Section length.
        unsigned char                   rsvd            : 2;
        unsigned char                   zero1           : 2;
        unsigned char                   protocol_ver: 5;
        unsigned char                   zero2           : 3;
        rmf_OobSi_Stt_SystemTime sys_time;
        //unsigned long                 sys_time;
        unsigned char                   gps_utc_offset;
};

class rmf_OobSiSttMgr
{
private:
    rmf_osal_ThreadId    m_si_SttThreadID;
    rmf_osal_eventqueue_handle_t m_si_SttFilterQueue; // OOB STT thread event queue
    uint32_t                m_table_unique_id;
    int m_bRecievedStt;
    int m_bRecievedTZ;
    int m_bRecievedDS;
    int m_bKeepSystemTimeSynchedWithCableTime;
    unsigned long m_CurrentTimeInSecs;
    unsigned long m_GpsUtcOffset;
    rmf_osal_eventqueue_handle_t m_queueId;
#ifdef TEST_WITH_PODMGR
    rmf_SectionFilter *m_pOobSectionFilter;
#endif
    rmf_OobSiCache* m_pSiCache;
    
    static rmf_OobSiSttMgr *m_pOobSiSttMgr;

    rmf_OobSiSttMgr();
    ~rmf_OobSiSttMgr();

    static void siSTTWorkerThread ( void   *data) ;

    void InitSystemTimeTask(void);
// ifndef USE_NTPCLIENT //uncomment following
    //rmf_Error update_systime(unsigned long t);
// endif
    void storeSysTime();
    void siSTTWorkerThreadFn ();
    void parse_update_time(unsigned char *input_buffer);
    void write_time (int64_t value);


    unsigned char isFeatureEnabled(const char *featureString);

#ifdef USE_POD_IPC
    int sendConfigTimezoneCmd(int timezoneinhours,int daylightflag);
    int sendConfigTimeofDayCmd(int64_t timevalue);
#endif
    
public:
#ifdef ENABLE_TIME_UPDATE
    void PostSTTEvent(int sttEvent, void *event_data, int data_extension);
#else   
    void PostSTTArrivedEvent(int sttEvent, int nEventData,int privTimeInSec,int privTimeInMillis);
#endif
// if USE_NTPCLIENT
    rmf_Error update_systime(unsigned long t, bool fromNTP=false);
// endif
    rmf_Error sttRegisterForSTTArrivedEvent(rmf_osal_eventqueue_handle_t eventQueueID);
    void setTimeZone(int timezoneinhours,int daylightflag);
    int CheckToApplyDST(void);	
    static rmf_OobSiSttMgr *getInstance(void);   
};

#endif //__RMF_OOBSISTTMGR_H__

