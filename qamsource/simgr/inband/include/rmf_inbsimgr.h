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

#ifndef __RMF_INBSIMGR_H__
#define __RMF_INBSIMGR_H__

#include <string.h> 
#include <list>

#include "rmf_osal_mem.h"
#include "rmf_osal_sync.h"
#include "rdk_debug.h"
#include "rmf_osal_event.h"
#include "rmf_osal_error.h"
#include "rmf_osal_thread.h"
#include "rmf_osal_time.h"
#include "rmf_osal_util.h"
#include "rmf_osal_types.h"
#ifndef ENABLE_INB_SI_CACHE
#include "rmf_qamsrc_common.h"
#include "rmf_inbsi_common.h"
#endif

#include <semaphore.h>


/**
 * @file rmf_inbsimgr.h
 * @brief RMF Inband service information manager.
 */

/**
 * @defgroup Inband QAM Source - InBand SI Manager
 * @ingroup QAM
 * @defgroup InbandClasses In-Band SI Manager Class
 * @ingroup Inband
 * @defgroup Inbanddatastructures In-Band SI Manager Data Structures
 * @ingroup Inband
 * @defgroup InbandMacros In-Band SI Manager Macros
 * @ingroup Inband
 * @defgroup rmfsitablestatus RMF SI Events Table Status
 * @ingroup InbandMacros
 * @defgroup sichangetypeevents RMF SI Change type events
 * @ingroup InbandMacros
 * @defgroup sihandleevents RMF SI Handle Events
 * @ingroup InbandMacros
 * @defgroup psievents RMF PSI Events
 * @ingroup InbandMacros
 */


using namespace std;


/**
 * @addtogroup rmfsitablestatus
 * The following events are generated when SI table is received or updated.
 * @{
 */
/* Follow the previous sitp debug statement model */
#define PSIMODULE                         "SITP_PSI"

/* PAT Timeout */
#define RMF_INBSI_PAT_TIMEOUT	1000 /* 1 Second */

/*
 ** index into the FilterSpec value array for the
 ** version  number.
 */
#define PSI_VERSION_INDEX                5

/*  SI DB Events */
#define RMF_SI_EVENT_UNKNOWN                        (0x00003000)
#define RMF_SI_EVENT_IB_PAT_ACQUIRED                (RMF_SI_EVENT_UNKNOWN + 1)
#define RMF_SI_EVENT_IB_PMT_ACQUIRED                (RMF_SI_EVENT_UNKNOWN + 2)
#define RMF_SI_EVENT_IB_CVCT_ACQUIRED               (RMF_SI_EVENT_UNKNOWN + 3)

#define RMF_SI_EVENT_SERVICE_COMPONENT_UPDATE           (RMF_SI_EVENT_UNKNOWN + 4)
#define RMF_SI_EVENT_IB_PAT_UPDATE                      (RMF_SI_EVENT_UNKNOWN + 5)
#define RMF_SI_EVENT_IB_PMT_UPDATE                      (RMF_SI_EVENT_UNKNOWN + 6)
#define RMF_SI_EVENT_IB_CVCT_UPDATE                 (RMF_SI_EVENT_UNKNOWN + 7)

#define RMF_SI_EVENT_SI_ACQUIRING                   (RMF_SI_EVENT_UNKNOWN + 8)
#define RMF_SI_EVENT_SI_NOT_AVAILABLE_YET           (RMF_SI_EVENT_UNKNOWN + 9)
#define RMF_SI_EVENT_SI_FULLY_ACQUIRED              (RMF_SI_EVENT_UNKNOWN + 10)
#define RMF_SI_EVENT_SI_DISABLED                    (RMF_SI_EVENT_UNKNOWN + 11)
#define RMF_SI_EVENT_TUNED_AWAY                     (RMF_SI_EVENT_UNKNOWN + 12)
#define RMF_SI_EVENT_PIPELINE_PAUSED                (RMF_SI_EVENT_UNKNOWN + 13)
#define RMF_SI_EVENT_PIPELINE_PLAYING               (RMF_SI_EVENT_UNKNOWN + 14)

#define RMF_SI_EVENT_IB_CAT_ACQUIRED                (RMF_SI_EVENT_UNKNOWN + 20)
#define RMF_SI_EVENT_IB_CAT_UPDATE                  (RMF_SI_EVENT_UNKNOWN + 21)

#define RMF_SI_EVENT_IB_TDT_ACQUIRED                (RMF_SI_EVENT_UNKNOWN + 22)
#define RMF_SI_EVENT_IB_TOT_ACQUIRED                (RMF_SI_EVENT_UNKNOWN + 23)


#define RMF_PSI_EVENT_SHUTDOWN           	    9999

#define RMF_SI_INVALID_HANDLE                   0

#define PROGRAM_NUMBER_UNKNOWN                  (0xFFFF)
/** @} */


/**
 * @addtogroup  sichangetypeevents
 * For service component update event, the 'event_flag' field indicates the type of change (ADD/REMOVE/MODIFY).
 * @{
 */
#define RMF_SI_CHANGE_TYPE_ADD                              0x01
#define RMF_SI_CHANGE_TYPE_REMOVE                           0x02
#define RMF_SI_CHANGE_TYPE_MODIFY                           0x03
/** @} */


#define RMF_SI_MAX_APIDS 	8
#define RMF_SI_MAX_PIDS_PER_PGM 	32
#define RMF_SI_PMT_MAX_SIZE 1024
/**
 *  Modulation modes
 */
/* Commented as this is not required for Inb SI Mgr
 typedef enum rmf_PsiModulationMode
 {
 RMF_PSI_MODULATION_UNKNOWN=0,
 RMF_PSI_MODULATION_QPSK,
 RMF_PSI_MODULATION_BPSK,
 RMF_PSI_MODULATION_OQPSK,
 RMF_PSI_MODULATION_VSB8,
 RMF_PSI_MODULATION_VSB16,
 RMF_PSI_MODULATION_QAM16,
 RMF_PSI_MODULATION_QAM32,
 RMF_PSI_MODULATION_QAM64,
 RMF_PSI_MODULATION_QAM80,
 RMF_PSI_MODULATION_QAM96,
 RMF_PSI_MODULATION_QAM112,
 RMF_PSI_MODULATION_QAM128,
 RMF_PSI_MODULATION_QAM160,
 RMF_PSI_MODULATION_QAM192,
 RMF_PSI_MODULATION_QAM224,
 RMF_PSI_MODULATION_QAM256,
 RMF_PSI_MODULATION_QAM320,
 RMF_PSI_MODULATION_QAM384,
 RMF_PSI_MODULATION_QAM448,
 RMF_PSI_MODULATION_QAM512,
 RMF_PSI_MODULATION_QAM640,
 RMF_PSI_MODULATION_QAM768,
 RMF_PSI_MODULATION_QAM896,
 RMF_PSI_MODULATION_QAM1024,
 RMF_PSI_MODULATION_QAM_NTSC // for analog mode
 } rmf_PsiModulationMode;
*/


/**
 * @enum _rmf_psi_table_type_e
 * @brief This represents the table types such as PAT,PMT,CVCT,etc.
 * @ingroup Inbanddatastructures
 */
typedef enum _rmf_psi_table_type_e
{
    TABLE_TYPE_PAT = 1,
    TABLE_TYPE_PMT,
    TABLE_TYPE_CVCT,
    TABLE_TYPE_NONE,
    TABLE_TYPE_DVB,
    TABLE_TYPE_CAT,
    TABLE_TYPE_TDT,
    TABLE_TYPE_TOT,
    TABLE_TYPE_UNKNOWN = TABLE_TYPE_NONE
} rmf_psi_table_type_e;


/**
 * @enum  _rmf_psi_state_e
 * @brief This represents the program service information table states
 * such as WAIT and IDLE for PAT and PMT .
 * @ingroup Inbanddatastructures
 */
/* SITP PSI state */
typedef enum _rmf_psi_state_e
{
    RMF_PSI_IDLE = 0,
    RMF_PSI_WAIT_INITIAL_PAT,
    RMF_PSI_WAIT_INITIAL_PRIMARY_PMT,
    RMF_PSI_WAIT_INITIAL_SECONDARY_PMT,
    RMF_PSI_WAIT_REVISION
} rmf_psi_state_e;


/**
 * @enum _rmf_psi_filter_priority_e
 * @brief This represents the priority for PAT and PMT table.Initial PAT and initial primary PMT have the same priority.
 * One should not pre-empt the other.
 * @ingroup Inbanddatastructures
 */
/* SITP PSI filter priority */
typedef enum _rmf_psi_filter_priority_e
{
    // Initial PAT and initial primary PMT have the same priority
    // One should not pre-empt the other
    RMF_PSI_FILTER_PRIORITY_INITIAL_PAT = 40,
    RMF_PSI_FILTER_PRIORITY_INITIAL_PRIMARY_PMT = 40,
    RMF_PSI_FILTER_PRIORITY_INITIAL_SECONDARY_PMT = 30,
    RMF_PSI_FILTER_PRIORITY_INITIAL_DVB = 25,
    RMF_PSI_FILTER_PRIORITY_REVISION_PAT = 20,
    RMF_PSI_FILTER_PRIORITY_REVISION_PMT = 20,
    RMF_PSI_FILTER_PRIORITY_INITIAL_CAT = 10,
    RMF_PSI_FILTER_PRIORITY_INITIAL_TDT_TOT = 9,
} sitp_psi_filter_priority_e;



/**
 * @struct _rmf_PsiParam
 * @brief  This represents the program service information parameters.
 * @ingroup Inbanddatastructures
 */
typedef struct _rmf_PsiParam
{
	rmf_psi_table_type_e table_type;
	uint16_t pid;
	uint16_t table_id_extension;
	uint16_t version_number;
	uint8_t priority;
	uint8_t table_id;
}rmf_PsiParam;


/**
 * @struct _rmf_PsiSourceInfo
 * @brief It represents the PSI source information.
 * @ingroup Inbanddatastructures
 */
typedef struct _rmf_PsiSourceInfo
{
    unsigned long dmxHdl; /**< Demux Handle to be used for Section Filtering */
    uint32_t tunerId;
    uint32_t freq; /**< Tuner frequency, in Hz (used to validate tuner state) */
    uint8_t modulation_mode; /** Modulation mode */	
}rmf_PsiSourceInfo;


/**
 * @struct _rmf_FilterInfo
 * @brief It provides the info of Filter such as program number, pid and filterId.
 * @ingroup Inbanddatastructures
 */
typedef struct _rmf_FilterInfo
{
	uint16_t program_number;
	uint16_t pid;            
       uint32_t filterId;
}rmf_FilterInfo;


/**
 * @struct _rmf_InbSiRegistrationListEntry
 * @brief It provides the info of SiRegistrationListEntry such as queueId,
 * frequency and program number .
 * @ingroup Inbanddatastructures
 */
typedef struct _rmf_InbSiRegistrationListEntry
{
    rmf_osal_eventqueue_handle_t queueId;
    uint32_t frequency;
    uint16_t program_number;
    struct _rmf_InbSiRegistrationListEntry* next;
} rmf_InbSiRegistrationListEntry;

#ifdef CAT_FUNCTIONALITY
typedef struct
{ // see document   ISO/IEC 13818-1:2000(E), pg.45
    uint8_t table_id_octet1;
    uint8_t section_length_octet2;
    uint8_t section_length_octet3;
    uint8_t reserved_octet4;
    uint8_t reserved_octet5;
    uint8_t current_next_indic_octet6;
    uint8_t section_number_octet7;
    uint8_t last_section_number_octet8;
} cat_table_struc1;
#endif

/**
 * @struct pat_table_struc1
 * @brief It provides the Program association table information.
 * Please refer document ISO/IEC 13818-1:2000(E), pg.43.
 * @ingroup Inbanddatastructures
 */
typedef struct
{ // see document   ISO/IEC 13818-1:2000(E), pg.43
    uint8_t table_id_octet1;
    uint8_t section_length_octet2;
    uint8_t section_length_octet3;
    uint8_t transport_stream_id_octet4;
    uint8_t transport_stream_id_octet5;
    uint8_t current_next_indic_octet6;
    uint8_t section_number_octet7;
    uint8_t last_section_number_octet8;
} pat_table_struc1;


/**
 * @struct pat_table_struc2
 * @brief It provides the Program association table information.
 * Please riefer document ISO/IEC 13818-1:2000(E),pg.43.
 * @ingroup Inbanddatastructures
 */
typedef struct
{ // see document ISO/IEC 13818-1:2000(E),pg.43
    uint8_t program_number_octet1;
    uint8_t program_number_octet2;
    uint8_t unknown_pid_octet3;
    uint8_t unknown_pid_octet4;
} pat_table_struc2;


/**
 * @struct pmt_table_struc1
 * @brief It provides the Program Map Table information.
 * Please refer document ISO/IEC 13818-1:2000(E), pg.46.
 * @ingroup Inbanddatastructures
 */
typedef struct
{ // see document   ISO/IEC 13818-1:2000(E), pg.46
    uint8_t table_id_octet1;
    uint8_t section_length_octet2;
    uint8_t section_length_octet3;
    uint8_t program_number_octet4;
    uint8_t program_number_octet5;
    uint8_t current_next_indic_octet6;
    uint8_t section_number_octet7;
    uint8_t last_section_number_octet8;
    uint8_t pcr_pid_octet9;
    uint8_t pcr_pid_octet10;
    uint8_t program_info_length_octet11;
    uint8_t program_info_length_octet12;
} pmt_table_struc1;


/**
 * @struct pmt_table_struc3
 * @brief It provides the PMT info of stream type,elementary pid and elementary stream info length.
 * Please refer document ISO/IEC 13818-1:2000(E),pg.46.
 * @ingroup Inbanddatastructures
 */
typedef struct
{ // see document ISO/IEC 13818-1:2000(E),pg.46
    uint8_t stream_type_octet1;
    uint8_t elementary_pid_octet2;
    uint8_t elementary_pid_octet3;
    uint8_t es_info_length_octet4;
    uint8_t es_info_length_octet5;
} pmt_table_struc3;


/**
 * @struct generic_descriptor_struc
 * @brief It provides the generic descriptor information.
 * Please refer document ISO/IEC 13818-1:2000(E),pg.46 and pg. 112.
 * @ingroup Inbanddatastructures
 */
typedef struct
{ // see document ISO/IEC 13818-1:2000(E),pg.46 and pg. 112
    uint8_t descriptor_tag_octet1;
    uint8_t descriptor_length_octet2;
} generic_descriptor_struc;


/**
 * @struct rmf_MediaInfo
 * @brief It provides the media information.
 * apid value as 8 which has been defined in RMF_SI_MAX_APIDS.
 * It declares the number of mediapids as 16 which has been defines in RMF_SI_MAX_PIDS_PER_PGM.
 * @ingroup Inbanddatastructures
 */
typedef struct
{
#if 0
        uint16_t pmt_pid;
        uint16_t aPid[RMF_SI_MAX_APIDS];
        uint16_t vPid;
        uint16_t pcrPid;
#endif
        uint16_t pmt_pid;
        uint16_t pcrPid;
        uint16_t numMediaPids;
        uint32_t pmt_crc32;
        rmf_MediaPID pPidList[RMF_SI_MAX_PIDS_PER_PGM];        
}rmf_MediaInfo;


/**
 * @struct rmf_CaInfo
 * @brief It provides the CA info of the number of streams,ecmpid
 * and streamInfo value from rmf_StreamDecryptinfo enumeration in which valid value has been defined.
 * @ingroup Inbanddatastructures
 */
typedef struct rmf_CaInfo
{
    uint32_t num_streams;
    rmf_StreamDecryptInfo *pStreamInfo;
    uint16_t ecmPid;
}rmf_CaInfo_t;


/**
 * @struct rmf_pmtInfo
 * @brief It provides the Program Map table information.
 * @ingroup Inbanddatastructures
 */
#ifndef ENABLE_INB_SI_CACHE
typedef struct rmf_pmtInfo
{
    uint16_t program_no;
    uint16_t pmt_pid;
}rmf_pmtInfo_t;
#endif

typedef void (*rmf_section_f)(uint32_t section_size, uint8_t *section_data, void* user_data);

//forward declaration
class rmf_SectionFilter;


/**
 * @class rmf_InbSiMgr
 * @brief A class of rmf Inband SI manager, contains SI DB function prototypes  and variables.
 * @ingroup InbandClasses
 */
class rmf_InbSiMgr
{
private:
	/** Registered SI DB listeners and queues (queues are used at RMF level only) */
	rmf_InbSiRegistrationListEntry *g_inbsi_registration_list;
	sem_t* psiMonitorThreadStop;
	rmf_osal_Bool m_bShutDown;
	/**  This condition is set by Inband SI Manager when PAT table is fetched and parsed */
	rmf_osal_Cond g_inbsi_pat_cond;
#ifdef CAT_FUNCTIONALITY
	rmf_osal_Cond g_inbsi_cat_cond;
	uint8_t *m_catTable;
	uint16_t m_catTableSize;
	uint16_t m_total_descriptor_length;

	volatile rmf_osal_Bool m_bIsCATFilterSet;
	volatile rmf_osal_Bool m_bIsCATAvailable;
	uint16_t catVersionNumber;
	uint32_t catCRC;
	uint32_t num_cat_sections;
#endif
	rmf_Error parse_and_update_CAT(uint32_t section_size, uint8_t *section_data);

	char* m_tdtTable;
	int   m_tdtTableSize;
	char* m_totTable;
	int   m_totTableSize;

	rmf_Error parse_TDT(uint32_t section_size, uint8_t *section_data);
	rmf_Error parse_TOT(uint32_t section_size, uint8_t *section_data);
	
	volatile rmf_osal_Bool m_bIsPATFilterSet;
	volatile rmf_osal_Bool m_bIsPATAvailable;
	static rmf_osal_Bool m_NegativeFiltersEnabled;
	static rmf_osal_Bool m_dumpPSITables ;
	rmf_osal_Mutex m_mediainfo_mutex;
#ifdef ENABLE_INB_SI_CACHE
	rmf_InbSiCache *m_pSiCache;
#else
	rmf_pmtInfo_t *m_pmtInfoList;
	int m_pmtListLen;
	int m_pmtListMaxSize;
	uint16_t patVersionNumber;
	uint16_t pmtVersionNumber;
	uint32_t patCRC;
	uint32_t pmtCRC;
	uint32_t number_outer_desc; // PMT
	rmf_SiMpeg2DescriptorList* outer_descriptors; // PMT
	uint32_t number_elem_streams;
	rmf_SiElementaryStreamList* elementary_streams; // PMT
	rmf_osal_Bool descriptors_in_use;
	uint16_t pmtPid;
	uint16_t pcrPid;
	uint16_t ts_id;
#ifdef QAMSRC_PMTBUFFER_PROPERTY
	uint8_t pmtBuf[RMF_SI_PMT_MAX_SIZE];
	uint32_t pmtBufLen;
#endif
#endif

	uint32_t m_freq; /**< Tuner frequency, in Hz (used to validate tuner state) */	
	uint8_t m_modulation_mode; /** Modulation mode */
	//list<uint32_t> m_filter_list;
	list<rmf_FilterInfo*> m_filter_list;
	uint32_t num_sections;

	rmf_Error get_pmtpid_for_program(uint16_t program_number, uint16_t *pmt_pid);
	rmf_Error start_sithread(void);
	void process_section_from_filter(uint32_t filterId);

	static void PsiThreadFn(void *arg);
	void PsiMonitor(void);
	
	rmf_Error parse_and_update_PAT(uint32_t section_size, uint8_t *section_data);
	rmf_Error parse_and_update_PMT(uint32_t section_size, uint8_t *section_data);

#ifdef ENABLE_INB_SI_CACHE
	void notifySIDB(rmf_psi_table_type_e table_type, rmf_osal_Bool init_version, 
			rmf_osal_Bool new_version, rmf_osal_Bool new_crc,
			rmf_SiGenericHandle si_handle);
	rmf_Error dispatch_event(uint32_t event, uint32_t eventFrequency, uint16_t eventProgramNumber, rmf_SiGenericHandle si_handle, uint32_t optionalEventData);
#else
	void notifySIDB(rmf_psi_table_type_e table_type, rmf_osal_Bool init_version, 
			rmf_osal_Bool new_version, rmf_osal_Bool new_crc);
	rmf_Error dispatch_event(uint32_t event, uint32_t optionalEventData);
#endif

	rmf_Error NotifyTableChanged(rmf_psi_table_type_e table_type,
		uint32_t changeType, uint32_t optional_data);
	const char* getStreamTypeString( int eValue );
	void dumpPSIData(uint8_t* section_data, uint32_t section_size);

public:
	rmf_InbSiMgr(rmf_PsiSourceInfo psiSouce);
	virtual ~rmf_InbSiMgr();

	rmf_Error RegisterForPSIEvents(rmf_osal_eventqueue_handle_t queueId, uint16_t program_number);
	rmf_Error UnRegisterForPSIEvents(rmf_osal_eventqueue_handle_t queueId, uint16_t program_number);
	rmf_Error RequestTsInfo();
	rmf_Error RequestTsCatInfo();
	rmf_Error GetCatTable(rmf_section_f section_fn, void* user_data);
	rmf_Error RequestTsTDTInfo(bool enable);
	rmf_Error RequestProgramInfo(uint16_t program_number);
	void TuneInitiated(uint32_t new_freq, uint8_t new_mode);
	rmf_Error GetCAInfoForProgram(uint16_t program_number, uint16_t ca_system_id, rmf_CaInfo_t *pCaInfo);
	rmf_Error GetMediaInfo(uint16_t program_number, rmf_MediaInfo *pMediaInfo);

#ifdef QAMSRC_PMTBUFFER_PROPERTY
	rmf_Error GetPMTBuffer(uint8_t * buf, uint32_t* length);
#endif

	rmf_Error ClearProgramInfo(uint16_t program_number);
	rmf_Error GetTSID(uint32_t* ptsid);
	rmf_Error GetDescriptors(rmf_SiElementaryStreamList ** ppESList, rmf_SiMpeg2DescriptorList** ppMpeg2DescList);
	void ReleaseDescriptors();
	virtual rmf_Error pause(void);
	virtual rmf_Error resume(void);
	char* get_TDT(int *size);
	char* get_TOT(int *size);
#ifndef ENABLE_INB_SI_CACHE
private:
	rmf_Error SetOuterDescriptor( rmf_SiDescriptorTag tag, 
                    uint32_t  length, void *content);
	rmf_Error SetDescriptor(  uint32_t elem_pid, 
			rmf_SiElemStreamType type, rmf_SiDescriptorTag tag,
			uint32_t length, void *content);
	void delete_elementary_stream(rmf_SiElementaryStreamList *elem_stream);
	void release_descriptors(rmf_SiMpeg2DescriptorList *desc);
	void release_es_list( rmf_SiElementaryStreamList *es_list);
	rmf_Error SetElementaryStream( rmf_SiElemStreamType stream_type, uint32_t elem_pid);
	void append_elementaryStream( rmf_SiElementaryStreamList *new_elem_stream);
	void get_serviceComponentType(rmf_SiElemStreamType stream_type,
			rmf_SiServiceComponentType *comp_type);
	void SetESInfoLength(uint32_t elem_pid, rmf_SiElemStreamType type, uint32_t es_info_length);
	void configure_negative_filters(rmf_psi_table_type_e type);
#endif
protected:
	// Mutex for thread
	rmf_osal_Mutex g_sitp_psi_mutex;
	rmf_osal_Mutex g_inbsi_registration_list_mutex; /* sync primitive */
	rmf_osal_Bool isPaused;
	rmf_SectionFilter *m_pInbSectionFilter;
	rmf_osal_eventqueue_handle_t m_FilterQueueId;

	void cancel_all_filters(void);
	rmf_Error set_filter(rmf_PsiParam *psiParam, uint32_t *filterId);
	virtual bool handle_table(uint8_t *data, uint32_t size)
	{
		return FALSE;
	}
};

#endif //__RMF_INBSIMGR_H__
