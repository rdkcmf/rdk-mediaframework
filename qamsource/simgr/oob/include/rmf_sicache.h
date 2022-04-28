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
 

#ifndef __RMF_INBSICACHE_H__
#define __RMF_INBSICACHE_H__

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
#include "rmf_qamsrc_common.h"
#include "rmf_inbsi_common.h"


/**
 * @file rmf_sicache.h
 * @brief Inband service information database
 */

/**
 * @defgroup Outofband QAM Source - OOB SI Manager
 * @ingroup QAM
 * @defgroup OutofbandClasses OOB SI Manager Classes
 * @ingroup Outofband
 */


using namespace std;


/**
 * @defgroup Outofbanddatastructures OOB SI Manager Data Structures
 * @ingroup Outofband
 *
 * @{
 * @enum rmf_SiStatus
 * @brief This represents valid values for the service information status as available, shortly available or not.
 */
typedef enum rmf_SiStatus
{
    SI_NOT_AVAILABLE = 0x00, SI_AVAILABLE_SHORTLY = 0x01, SI_AVAILABLE = 0x02
} rmf_SiStatus;


/**
 * @enum rmf_TsEntryState
 * @brief This represents the transport stream entry state as new, old and updated.
 */
typedef enum rmf_TsEntryState
{
    TSENTRY_NEW, TSENTRY_OLD, TSENTRY_UPDATED
} rmf_TsEntryState;


/**
 * @enum rmf_SiDeliverySystemType
 * @brief This represents the valid values for the  SI Delivery system type.
 */
typedef enum rmf_SiDeliverySystemType
{
	RMF_SI_DELIVERY_SYSTEM_TYPE_CABLE = 0x00,
	RMF_SI_DELIVERY_SYSTEM_TYPE_SATELLITE = 0x01,
	RMF_SI_DELIVERY_SYSTEM_TYPE_TERRESTRIAL = 0x02,
	RMF_SI_DELIVERY_SYSTEM_TYPE_UNKNOWN = 0x03
} rmf_SiDeliverySystemType;


/**
 * @enum rmf_SiServiceInformationType
 * @brief This represents the valid values for the service information type.
 */
typedef enum rmf_SiServiceInformationType
{
	RMF_SI_SERVICE_INFORMATION_TYPE_ATSC_PSIP = 0x00,
	RMF_SI_SERVICE_INFORMATION_TYPE_DVB_SI = 0x01,
	RMF_SI_SERVICE_INFORMATION_TYPE_SCTE_SI = 0x02,
	RMF_SI_SERVICE_INFORMATION_TYPE_UNKNOWN = 0x03
} rmf_SiServiceInformationType;

/* PSI DB Supported handle types */
typedef uintptr_t rmf_SiTransportStreamHandle;
typedef uintptr_t rmf_SiProgramHandle;
typedef uintptr_t rmf_SiServiceComponentHandle;
typedef uintptr_t rmf_SiRatingDimensionHandle;

/* SI DB Supported handle types */
typedef uintptr_t rmf_SiServiceHandle;
typedef uint32_t rmf_SiServiceDetailsHandle;
typedef uintptr_t rmf_SiRatingDimensionHandle;

typedef uintptr_t rmf_SiGenericHandle; // Cast before use...


/**
 * @struct rmf_SiPatProgramList
 * @brief Represents a linked list of SiPat Program list.
 */
typedef struct rmf_SiPatProgramList
{
    uint32_t programNumber;
    uint32_t pmt_pid;
    rmf_osal_Bool matched;
    struct rmf_SiPatProgramList* next;
} rmf_SiPatProgramList;

struct rmf_SiProgramInfo;


/**
 * @struct rmf_SiTransportStreamEntry
 * @brief Represents a linked list of transport stream entry.
 */
typedef struct rmf_SiTransportStreamEntry
{
    rmf_TsEntryState state;

    uint32_t frequency;
    uint32_t modulation_mode;

    uint32_t ts_id;
    char *ts_name;
    char *description;
    rmf_osal_TimeMillis ptime_transport_stream;
    uint16_t service_count;
    uint16_t program_count;
    uint16_t visible_service_count;
    rmf_SiStatus siStatus;
    uint32_t pat_version; // PAT
    uint32_t pat_crc; // PAT
    uint32_t cvct_version; // CVCT
    uint32_t cvct_crc; // CVCT
    rmf_osal_Bool check_crc;
    uintptr_t pat_reference;
    rmf_SiPatProgramList* pat_program_list; // PAT
    uint8_t* pat_byte_array;
    //ListSI programs;
    //list<rmf_SiProgramInfo> *programs;
    list<rmf_SiProgramHandle> *programs;
    list<rmf_SiServiceHandle> *services;
    rmf_osal_Mutex list_lock;
    struct rmf_SiTransportStreamEntry* next;

} rmf_SiTransportStreamEntry;


/**
 * @enum rmf_SiPMTStatus
 * @brief When PMT is requested, NOT_AVAILABLE if the program number is not in the PAT.
 * AVAILABLE if the PMT is already acquired.
 * AVAILABLE_SHORTLY if PMT exist but is still being acquired.
 */
typedef enum rmf_SiPMTStatus
{
    PMT_NOT_AVAILABLE = 0x00,
    PMT_AVAILABLE_SHORTLY = 0x01,
    PMT_AVAILABLE = 0x02
} rmf_SiPMTStatus;


/**
 * @enum rmf_SiVideoStandard
 * @brief It represents the NTSC video standard(See NTSC_Specification_1.0_Table 29).
 */
typedef enum rmf_SiVideoStandard
{
    VIDEO_STANDARD_NTSC = 0x00,
    VIDEO_STANDARD_PAL625 = 0x01,
    VIDEO_STANDARD_PAL525 = 0x02,
    VIDEO_STANDARD_SECAM = 0x03,
    VIDEO_STANDARD_MAC = 0x04,
    VIDEO_STANDARD_UNKNOWN = 0x05
} rmf_SiVideoStandard;


/**
 * @struct rmf_SiPMTReference
 * @brief It retrieves the value of PMT reference descriptors list.
 */
typedef struct rmf_SiPMTReference
{
    uint32_t pcr_pid;
    uint32_t number_outer_desc;
    uintptr_t outer_descriptors_ref;
    uint32_t number_elem_streams;
    uintptr_t elementary_streams_ref;
} rmf_SiPMTReference;


/**
 * @struct rmf_SiProgramInfo
 * @brief It defines the values for the SI program inforamtion .
 * @}
 */
typedef struct rmf_SiProgramInfo
{
    // These will only be valid for digital services.
    rmf_SiPMTStatus pmt_status; // move to program_info structure.
    uint32_t program_number; // PAT, PMT, SVCT
    uint32_t pmt_pid; // PAT
    uint32_t pmt_version; // PMT
    rmf_osal_Bool crc_check;
    uint32_t pmt_crc; // PMT
    rmf_SiPMTReference* saved_pmt_ref;
    uint8_t* pmt_byte_array; // PMT
    // Duplicated in saved_pmt_ref, why 2 copies?
    uint32_t number_outer_desc; // PMT
    rmf_SiMpeg2DescriptorList* outer_descriptors; // PMT
    uint32_t number_elem_streams;
    rmf_SiElementaryStreamList* elementary_streams; // PMT
    uint32_t pcr_pid; // PMT
    list<rmf_SiServiceHandle> *services;
    // Can be Empty.
    uint32_t service_count;
    rmf_SiTransportStreamEntry *stream; // Transport stream this program is part of.
} rmf_SiProgramInfo;


/**
 * @class rmf_InbSiCache
 * @brief A class of Inband SiCache database, contains SI DB function prototypes  and variables.
 * Inband si cache database is responsible for parsing the service inforamtion tables as PAT, PMT info.
 * @ingroup OutofbandClasses
 */
class rmf_InbSiCache
{
private:
	rmf_SiTransportStreamEntry *g_si_ts_entry;
	rmf_osal_Mutex g_global_si_mutex;
	rmf_osal_Cond g_global_si_cond;
	uint32_t g_cond_counter;
	
	static rmf_InbSiCache *m_pSiCache;
	static uint32_t cache_count;

	rmf_InbSiCache();
	~rmf_InbSiCache();

	rmf_SiTransportStreamHandle create_transport_stream_handle(
                uint32_t frequency, uint8_t mode);

	uintptr_t get_transport_stream_handle(uint32_t freq,
		uint8_t mode);

	rmf_osal_Bool find_program_in_ts_pat_programs(rmf_SiPatProgramList *pat_list,
		uint32_t pn);

	void compare_ts_pat_programs(rmf_SiPatProgramList *new_pat_list,
		rmf_SiPatProgramList *saved_pat_list, rmf_osal_Bool setNewProgramFlag);
	void signal_program_removed_event(rmf_SiTransportStreamEntry *ts_entry,
		rmf_SiPatProgramList *pat_list);

	rmf_Error init_program_info(rmf_SiProgramInfo *pi);

	void append_elementaryStream(rmf_SiProgramInfo *pi,
		rmf_SiElementaryStreamList *new_elem_stream);
	rmf_Error remove_elementaryStream(rmf_SiElementaryStreamList **elem_stream_list_head,
	        rmf_SiElementaryStreamList *rmv_elem_stream);
	void release_elementary_streams(rmf_SiElementaryStreamList *eStream);
	void delete_elementary_stream(rmf_SiElementaryStreamList *elem_stream);
	void release_descriptors(rmf_SiMpeg2DescriptorList *desc);
	void mark_elementary_stream(rmf_SiElementaryStreamList *elem_stream);
	void get_serviceComponentType(rmf_SiElemStreamType stream_type,
		rmf_SiServiceComponentType *comp_type);
	void release_program_info_entry(rmf_SiProgramInfo *pi);
	void release_transport_stream_entry(rmf_SiTransportStreamEntry *ts_entry);
public:	
	static rmf_InbSiCache *getInbSiCache();
	static void clearCache();

	// lock/unlock
	rmf_Error LockForRead(void);
	rmf_Error UnLock(void);

	void LockForWrite(void);
	void ReleaseWriteLock(void);

	rmf_Error init_transport_stream_entry(
		rmf_SiTransportStreamEntry *ts_entry, uint32_t Frequency);

	rmf_SiProgramInfo * create_new_program_info(
	rmf_SiTransportStreamEntry *ts_entry, uint32_t prog_num);

	// program info handle specific
	rmf_Error GetPMTVersionForProgramHandle(
		rmf_SiProgramHandle program_handle, uint32_t* pmt_version);
	rmf_Error GetPMTCRCForProgramHandle(rmf_SiProgramHandle program_handle,
		uint32_t* pmt_crc);
        rmf_Error GetNumberOfComponentsForServiceHandle(
                rmf_SiProgramHandle pi_handle, uint32_t *num_components);
	rmf_osal_Bool CheckCADescriptors(rmf_SiProgramHandle pi_handle, uint16_t ca_system_id,
        uint32_t *numStreams, uint16_t *ecmPid, rmf_StreamDecryptInfo streamInfo[]);


	// transport stream handle specific
	rmf_Error GetTransportStreamIdForTransportStreamHandle(
		rmf_SiTransportStreamHandle ts_handle, uint32_t *ts_id);
	rmf_Error GetDescriptionForTransportStreamHandle(
		rmf_SiTransportStreamHandle ts_handle, char **description);
	rmf_Error GetTransportStreamNameForTransportStreamHandle(
		rmf_SiTransportStreamHandle ts_handle, char **ts_name);
	rmf_Error GetFrequencyForTransportStreamHandle(
		rmf_SiTransportStreamHandle ts_handle, uint32_t* freq);
	rmf_Error GetModulationFormatForTransportStreamHandle(
		rmf_SiTransportStreamHandle ts_handle, uint8_t *mode);
	rmf_Error GetTransportStreamServiceInformationType(
		rmf_SiTransportStreamHandle ts_handle,
		rmf_SiServiceInformationType* serviceInformationType);
	rmf_Error GetTransportStreamLastUpdateTimeForTransportStreamHandle(
		rmf_SiTransportStreamHandle ts_handle, rmf_osal_TimeMillis *time);

	rmf_Error GetPATCRCForTransportStreamHandle(
		rmf_SiTransportStreamHandle ts_handle, uint32_t* pat_crc);
	rmf_Error GetPATVersionForTransportStreamHandle(
		rmf_SiTransportStreamHandle ts_handle, uint32_t* pat_version);

	rmf_Error GetTransportStreamEntryFromFrequencyModulation(uint32_t freq,
		uint8_t mode, rmf_SiTransportStreamHandle *ts_handle);
	rmf_Error GetProgramEntryFromFrequencyProgramNumberModulation(
		uint32_t frequency, uint32_t program_number, uint8_t mode,
		rmf_SiProgramHandle *prog_handle);
	rmf_Error GetProgramEntryFromTransportStreamEntry(
		rmf_SiTransportStreamHandle ts_handle, uint32_t program_number,
		rmf_SiProgramHandle *prog_handle);

	/* PAT */
	void SetTSIdForTransportStream(rmf_SiTransportStreamHandle ts_handle,
		uint32_t tsId);
	void SetTSIdStatusNotAvailable(rmf_SiTransportStreamHandle ts_handle);
	void SetTSIdStatusAvailableShortly(rmf_SiTransportStreamHandle ts_handle);
	void SetPATVersionForTransportStream(
		rmf_SiTransportStreamHandle ts_handle, uint32_t pat_version);
	void SetPATCRCForTransportStream(rmf_SiTransportStreamHandle ts_handle,
		uint32_t crc);
	rmf_Error SetPATProgramForTransportStream(
		rmf_SiTransportStreamHandle ts_handle, uint32_t program_number,
		uint32_t pmt_pid);
	void SetPATVersion(rmf_SiTransportStreamHandle ts_handle,
		uint32_t pat_version);
	void SetPMTPid(rmf_SiProgramHandle program_handle, uint32_t pmt_pid);

	void SetPATStatusNotAvailable(
		rmf_SiTransportStreamHandle transport_handle);
	void SetPATStatusAvailable(rmf_SiTransportStreamHandle transport_handle);
	void ResetPATProgramStatus(rmf_SiTransportStreamHandle ts_handle);

	/* PMT */
	void SetPCRPid(rmf_SiProgramHandle program_handle, uint32_t pcr_pid);
	rmf_Error SetPMTVersion(rmf_SiProgramHandle program_handle,
		uint32_t pmt_version);
	rmf_Error SetOuterDescriptor(rmf_SiProgramHandle program_handle,
		rmf_SiDescriptorTag tag, uint32_t length, void *content);
	rmf_Error SetDescriptor(rmf_SiProgramHandle program_handle,
		uint32_t elem_pid, rmf_SiElemStreamType type, rmf_SiDescriptorTag tag,
		uint32_t length, void *content);
	void SetESInfoLength(rmf_SiProgramHandle program_handle,
		uint32_t elem_pid, rmf_SiElemStreamType type, uint32_t es_info_length);
	rmf_Error SetElementaryStream(rmf_SiProgramHandle program_handle,
		rmf_SiElemStreamType stream_type, uint32_t elem_pid);

	void SetPMTStatus(rmf_SiProgramHandle program_handle);
	void SetPMTStatusAvailableShortly(rmf_SiTransportStreamHandle ts_handle);
	rmf_Error SetPMTStatusNotAvailable(rmf_SiProgramHandle pi_handle);
	rmf_Error SetPMTProgramStatusNotAvailable(rmf_SiProgramHandle pi_handle);
	rmf_Error GetPMTProgramStatus(rmf_SiProgramHandle pi_handle, rmf_SiPMTStatus *pmt_status);
	rmf_Error RevertPMTStatus(rmf_SiProgramHandle pi_handle);
	void SetPMTCRC(rmf_SiProgramHandle pi_handle, uint32_t new_crc);
#ifdef OPTIMIZE_INBSI_CACHE
        void ReleaseProgramInfoEntry(rmf_SiProgramHandle pi_handle);
	void ReleaseTransportStreamEntry(rmf_SiTransportStreamHandle ts_handle);
#endif
};

#endif //__RMF_INBSICACHE_H__
