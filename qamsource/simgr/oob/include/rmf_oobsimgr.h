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
 

/**
 * @file rmf_oobsimgr.h
 * @brief RMF Outband service information manager.
 * @defgroup OutbandMacros OOB SI Manager Macros
 * @ingroup Outofband
 * @defgroup SIAcquisitionStatus SI Acquisition Status
 * @ingroup OutbandMacros
 */

#ifndef __RMF_OOBSIMGR_H__
#define __RMF_OOBSIMGR_H__

#include "rmf_osal_mem.h"
#include "rmf_osal_sync.h"
#include "rdk_debug.h"
#include "rmf_osal_error.h"
#include "rmf_osal_time.h"
#include "rmf_osal_util.h"
#include "rmf_osal_types.h"
#include "rmf_osal_thread.h"
#include "rmf_osal_event.h"
#include "rmf_osal_file.h"
#include "rmf_sectionfilter.h"
#include "rmf_oobsicache.h"
#include "rmf_oobsiparser.h"
#include "rmf_oobsieventlistener.h"

#ifdef TEST_WITH_PODMGR
#include "rmf_sectionfilter.h"
#endif
#include "rmf_oobService.h"

#include "rmf_oobsisttmgr.h"

/**
 * @addtogroup SIAcquisitionStatus
 * Events to indicate SI acquisition status
 * @{
 */
#define RMF_SI_EVENT_SI_ACQUIRING                   (RMF_OOBSI_EVENT_UNKNOWN + 8)
#define RMF_SI_EVENT_SI_NOT_AVAILABLE_YET           (RMF_OOBSI_EVENT_UNKNOWN + 9)
#define RMF_SI_EVENT_SI_FULLY_ACQUIRED              (RMF_OOBSI_EVENT_UNKNOWN + 10)
#define RMF_SI_EVENT_SI_DISABLED                    (RMF_OOBSI_EVENT_UNKNOWN + 11)
//! @}

/**
 * @ingroup Outofbanddatastructures
 * @{
 * @enum _rmf_si_error_e
 * @brief SITP component return codes are enumerated here.
 */
typedef enum _rmf_si_error_e
{
    RMF_SI_FAILURE = 1,
    RMF_SI_TUNER_PARAM_FAILURE,
    RMF_SI_RESOURCE_FAILURE
} rmf_error_e;

/**
 * Priorities used within the implementation when setting filters for specific purposes.
 * These priorities should be considered private to the implementation and not treated
 * special by MPEOS or below.
 * These definitions are subject to change based upon the whims of MPE and above.
 */
enum
{
    RMF_SF_FILTER_PRIORITY_EAS = 1, /**< Priority for IB/OOB Emergency Alert acquisition. */
    RMF_SF_FILTER_PRIORITY_SITP, /**< Priority for IB/OOB SI acquisition. */
    RMF_SF_FILTER_PRIORITY_XAIT, /**< Priority for OOB XAIT acquisition. */
    RMF_SF_FILTER_PRIORITY_AIT, /**< Priority for IB AIT acquisition. */
    RMF_SF_FILTER_PRIORITY_OC, /**< Priority for OC acquisition. */
    RMF_SF_FILTER_PRIORITY_DAVIC,
/**< Priority for Application-controlled DAVIC filtering. */
};

//! @}

/*
typedef struct Oob_ProgramInfo
{
	uint32_t carrier_frequency;
	rmf_ModulationMode modulation_mode;	
	uint32_t prog_num;
        rmf_SiServiceHandle service_handle;        	
}Oob_ProgramInfo_t;
*/

/**
 * @class rmf_OobSiMgr
 * @brief Class extending rmf_OobSiEventListener to implement rmf_OobSiMgr.
 * It handle Service Information tables such as NIT, NTT, SVCT and STT.
 * @ingroup OutofbandClasses
 */
class rmf_OobSiMgr : public rmf_OobSiEventListener
{
private:
	rmf_osal_ThreadId g_sitp_si_threadID;
	rmf_osal_eventqueue_handle_t g_sitp_si_queue; // OOB thread event queue
	rmf_osal_TimeMillis g_sitp_sys_time;
	rmf_osal_eventqueue_handle_t g_sitp_pod_queue; // POD event queue
	rmf_osal_eventqueue_handle_t g_sitp_stt_queue; // STT event queue
	rmf_si_table_t * g_table_array;
	rmf_si_table_t * g_table;
	int g_shutDown; // shutdown flag
	rmf_osal_ThreadId g_sitp_si_stt_threadID;
	rmf_osal_eventqueue_handle_t g_sitp_si_stt_queue;
	rmf_si_table_t * g_stt_table;
	uint32_t g_sitp_stt_timeout;
	uint32_t g_sitp_stt_thread_sleep_millis;
	rmf_osal_Cond g_sitp_stt_condition;

	/*
	 *****************************************************************************
	 * Flags to indicate acquisition of CDS, MMS and SVCT.
	 * When we have these table signal that SIDB can be accessed.
	 *****************************************************************************
	 */
	uint32_t g_sitp_svctAcquired;
	uint32_t g_sitp_siTablesAcquired;

	/*  Control variables for the si caching */
	//rmf_osal_Bool     g_siOOBCacheEnable;
	rmf_osal_Bool     g_siOOBCacheConvertStatus;
	rmf_osal_Bool     g_siOOBCached;
	//const char  *g_siOOBCachePost114Location;
	//const char  *g_siOOBCacheLocation;

	//const char  *g_siOOBSNSCacheLocation;

	char g_badSICacheLocation[250];
	char g_badSISNSCacheLocation[250];
	uint32_t g_sitp_si_timeout;

	uint32_t g_sitp_si_max_nit_cds_wait_time;
        uint32_t g_sitp_si_max_nit_mms_wait_time;
        uint32_t g_sitp_si_max_svct_dcm_wait_time;
        uint32_t g_sitp_si_max_svct_vcm_wait_time;
        uint32_t g_sitp_si_max_ntt_wait_time;
        uint32_t g_sitp_si_update_poll_interval;
        uint32_t g_sitp_si_update_poll_interval_max;
        uint32_t g_sitp_si_update_poll_interval_min;
	int32_t g_sitp_pod_status_update_time_interval;
	uint32_t g_sitp_si_profile;
	uint32_t g_sitp_si_profile_table_number;
	int32_t g_sitp_si_status_update_time_interval;
	uint32_t g_sitp_si_initial_section_match_count;
	uint32_t g_sitp_si_rev_sample_sections;
	uint32_t g_stip_si_process_table_revisions;
       uint32_t g_sitp_si_filter_multiplier;
       uint32_t g_sitp_si_section_retention_time;
       uint32_t g_sitp_si_max_nit_section_seen_count;
       uint32_t g_sitp_si_max_vcm_section_seen_count;
       uint32_t g_sitp_si_max_dcm_section_seen_count;
       uint32_t g_sitp_si_max_ntt_section_seen_count;
       rmf_osal_Bool g_siOobFilterEnable; 
	/*  Registered SI DB listeners and queues (queues are used at MPE level only) */
	rmf_SiRegistrationListEntry *g_si_registration_list;
	rmf_osal_Mutex g_registration_list_mutex; /* sync primitive */

	rmf_OobSiCache* m_pSiCache;
	
	static rmf_OobSiMgr *m_pSiMgr;
#ifdef TEST_WITH_PODMGR
	rmf_SectionFilter *m_pOobSectionFilter;
#endif
	rmf_OobSiParser *m_pOobSiParser;
       rmf_OobSiSttMgr *m_pOobSiSttMgr;

    // ipdu-ntp
    int inIpDirectMode;

	rmf_OobSiMgr();
	~rmf_OobSiMgr();

	rmf_Error sitp_si_state_wait_table_revision_handler(uint32_t si_event,
			uint32_t event_data);
	rmf_Error sitp_si_state_wait_table_complete_handler(uint32_t si_event,
			uint32_t event_data);
	rmf_Error sitp_si_get_table(rmf_si_table_t ** si_table,
			uint32_t si_event, uint32_t data);
	void si_Shutdown(void);
	static void sitp_siWorkerThread(void * data);
	static void sitp_siSTTWorkerThread(void * data);
	rmf_Error createAndSetTableDataArray(
			rmf_si_table_t ** handle_table_array);
	rmf_Error createAndSetSTTDataArray(void);
	rmf_Error get_table_section(rmf_FilterSectionHandle * section_handle,
			uint32_t uniqueID);
	rmf_Error release_table_section(rmf_si_table_t * table_data,
			rmf_FilterSectionHandle section_handle);
	rmf_Error release_filter(rmf_si_table_t * table_data,
			uint32_t unique_id);
	rmf_Error activate_filter(rmf_si_table_t * table_data,
			uint32_t timesToMatch);
	rmf_Error activate_stt_filter(rmf_si_table_t * table_data,
			uint32_t timesToMatch);
	void printFilterSpec(rmf_FilterSpec filterSpec);
	rmf_osal_Bool isTable_acquired(rmf_si_table_t * ptr_table, rmf_osal_Bool *all_sections_acquired);

	rmf_osal_Bool matchedCRC32(rmf_si_table_t * table_data, uint32_t crc,
			uint32_t * index);
	void set_tablesAcquired(rmf_si_table_t * ptr_table);
	void reset_section_array(rmf_si_revisioning_type_e rev_type);
	rmf_Error findTableIndexByUniqueId(uint32_t * index, uint32_t unique_id);
	void dumpTableData(rmf_si_table_t * table_data);
	rmf_osal_Bool checkForActiveFilters(void);

	rmf_osal_Bool checkCRCMatchCount(rmf_si_table_t * ptr_table,
			uint32_t section_index);
	void checkForTableAcquisitionTimeouts(void);
	void purgeOldSectionMatches(rmf_si_table_t * table_data, rmf_osal_TimeMillis current_time);
	void purgeSectionsNotMatchedByFilter(rmf_si_table_t * table_data);

	/* debug functions */
	/** make these global to prevent compiler warning errors **/
	const char * sitp_si_eventToString(uint32_t event);
	const char * sitp_si_stateToString(uint32_t state);
	const char * sitp_si_tableToString(uint8_t table);
       void setTablesAcquired(uint32_t tables_acquired);
	uint32_t getTablesAcquired(void);
	void sitp_si_cache_enabled(void);
      uint32_t si_state_to_event(rmf_SiGlobalState state);
      
      rmf_Error NotifyTableChanged(rmf_SiTableType table_type,
                    uint32_t changeType, uint32_t optional_data);

      void setSVCTAcquired(uint32_t svct_acquired);
      uint32_t getSVCTAcquired(void);

	void siWorkerThreadFn();
	void siSTTWorkerThreadFn();

//if USE_NTPCLIENT
	rmf_osal_ThreadId g_ntpclient_threadID;
	static void ntpclientWorkerThread(void*);
	void ntpclientWorkerThreadFn();
	rmf_Error monitorAndSinkTimeWithNTP(const char *hostname, int sleepFor);
        bool getNTPUrl(bool ntpURLFound, char* ntpserver, size_t bufLen);
#ifdef TEST_WITH_PODMGR
    bool waitForDSGIPAcquiredEvent(void);
#endif
    char* getDataFromResponse(const char* mUrlStr, const char* q_str, char *cvalue_str);
    char* get_response_val(const char* mUrlStr, const char* q_str, char* value_str);
    // This function is used to optimize boot time when in IP direct mode and the system time source
    // is NTP and not a STT HE message.
    void ntpClientSleep(int32_t sleepFor);              // ipdu-ntp
//endif
       void send_si_event(uint32_t siEvent, uint32_t optional_data, uint32_t changeType);
#ifdef ENABLE_TIME_UPDATE
        void send_si_stt_event(uint32_t siEvent, void *pData, uint32_t data_extension);
#endif
	void printMatchStatistics(void);
public:
	static rmf_OobSiMgr *getInstance();
	void rmf_si_reset_tables(void);
	void rmf_si_purge_RDD_tables(void);
    
#ifdef ENABLE_TIME_UPDATE
	rmf_Error RegisterForSIEvents(uint32_t si_event_type, rmf_osal_eventqueue_handle_t queueId);
        rmf_Error UnRegisterForSIEvents(uint32_t si_event_type, rmf_osal_eventqueue_handle_t queueId);
#else
	rmf_Error RegisterForSIEvents(rmf_osal_eventqueue_handle_t queueId);
        rmf_Error UnRegisterForSIEvents(rmf_osal_eventqueue_handle_t queueId);
#endif
	rmf_Error GetProgramInfo(uint32_t sourceId, Oob_ProgramInfo_t *pProgramInfo); 	
	rmf_Error GetProgramInfoByVCN(uint16_t vcn, Oob_ProgramInfo_t *pProgramInfo);
	rmf_Error GetDACId(uint16_t *pDACId); 	
	rmf_Error GetChannelMapId(uint16_t *pChannelMapId); 
	rmf_Error GetVCTId(uint16_t *pVCTId); 
	static rmf_Error rmf_SI_registerSIGZ_Callback(rmf_siGZCallback_t func);
	
	rmf_Error GetVirtualChannelNumber(
                uint32_t sourceId, uint16_t *virtual_channel_number);
	void notify_VCTID_change(void);
#ifdef ENABLE_TIME_UPDATE
	rmf_Error GetSysTime(rmf_osal_TimeMillis *pSysTime);
	rmf_Error GetTimeZone(int32_t *pTimezoneinhours, int32_t *pDaylightflag);
#endif
};

#endif //__RMF_OOBSIMGR_H__

