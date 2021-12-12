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
 * @file rmf_oobsicache.cpp
 * @brief File contains the out of band service inforamtion database.
 */

#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include <fstream>

#include "rmf_osal_mem.h"
#include "rmf_osal_sync.h"
#include "rdk_debug.h"
#include "rmf_osal_event.h"
#include "rmf_osal_error.h"
#include "rmf_osal_time.h"
#include "rmf_osal_util.h"
#include "rmf_osal_types.h"
#include "rmf_osal_file.h"
#include "rmf_oobsicache.h"
#include "rmf_oobsimgr.h"

#ifdef LOGMILESTONE
#include "rdk_logger_milestone.h"
#endif

#ifdef GLI
#include "sysMgr.h"
#include "libIBus.h"
#endif

#if USE_SYSRES_MLT
#include "rpl_malloc.h"
#include "rpl_new.h"
#endif

#define RMF_STT_RECEIVED_TOUCH_FILE "/tmp/stt_received"
#define RMF_SI_ACQUIRED_TOUCH_FILE  "/tmp/si_acquired"

#define        NAME_COUNT_MAX                       32768
#define        MPEG_2_TRANSPORT                     0
#define        NON_MPEG_2_TRANSPORT                 1
rmf_siGZCallback_t rmf_OobSiCache::rmf_siGZCBfunc = 0;
/*  Rating region US (50 states + possessions)
 Ratings are currently hard-coded for Region 1.
 Allowed by SCTE 65 */
static rmf_SiRatingValuesDefined g_US_RRT_val_Entire_Audience[6];
static const char* g_US_RRT_eng_vals_Entire_Audience[6] =
{ "", "None", "TV-G", "TV-PG", "TV-14", "TV-MA" };
static const char* g_US_RRT_eng_abbrev_vals_Entire_Audience[6] =
{ "", "None", "TV-G", "TV-PG", "TV-14", "TV-MA" };
rmf_SiRatingDimension g_US_RRT_dim_Entire_Audience =
{ NULL, 1, 6, g_US_RRT_val_Entire_Audience };
static const char* g_US_RRT_eng_dim_Entire_Audience = "Entire Audience";


static rmf_SiRatingValuesDefined g_US_RRT_val_Dialogue[2];
static const char* g_US_RRT_eng_vals_Dialogue[2] =
{ "", "D" };
static const char* g_US_RRT_eng_abbrev_vals_Dialogue[2] =
{ "", "D" };
rmf_SiRatingDimension g_US_RRT_dim_Dialogue =
{ NULL, 0, 2, g_US_RRT_val_Dialogue };
static const char* g_US_RRT_eng_dim_Dialogue = "Dialogue";


static rmf_SiRatingValuesDefined g_US_RRT_val_Language[2];
static const char* g_US_RRT_eng_vals_Language[2] =
{ "", "L" };
static const char* g_US_RRT_eng_abbrev_vals_Language[2] =
{ "", "L" };
rmf_SiRatingDimension g_US_RRT_dim_Language =
{ NULL, 0, 2, g_US_RRT_val_Language };
static const char *g_US_RRT_eng_dim_Language = "Language";


static rmf_SiRatingValuesDefined g_US_RRT_val_Sex[2];
static const char* g_US_RRT_eng_vals_Sex[2] =
{ "", "S" };
static const char* g_US_RRT_eng_abbrev_vals_Sex[2] =
{ "", "S" };
rmf_SiRatingDimension g_US_RRT_dim_Sex =
{ NULL, 0, 2, g_US_RRT_val_Sex };

static const char* g_US_RRT_eng_dim_Sex = "Sex";


static rmf_SiRatingValuesDefined g_US_RRT_val_Violence[2];
static const char* g_US_RRT_eng_vals_Violence[2] =
{ "", "V" };
static const char* g_US_RRT_eng_abbrev_vals_Violence[2] =
{ "", "V" };
rmf_SiRatingDimension g_US_RRT_dim_Violence =
{ NULL, 0, 2, g_US_RRT_val_Violence };
static const char* g_US_RRT_eng_dim_Violence = "Violence";


static rmf_SiRatingValuesDefined g_US_RRT_val_Children[3];
static const char* g_US_RRT_eng_vals_Children[3] =
{ "", "TV-Y", "TV-Y7" };
static const char* g_US_RRT_eng_abbrev_vals_Children[3] =
{ "", "TV-Y", "TV-Y7" };
rmf_SiRatingDimension g_US_RRT_dim_Children =
{ NULL, 1, 3, g_US_RRT_val_Children };
static const char* g_US_RRT_eng_dim_Children = "Children";


static rmf_SiRatingValuesDefined g_US_RRT_val_Fantasy_Violence[2];
static const char* g_US_RRT_eng_vals_Fantasy_Violence[2] =
{ "", "FV" };
static const char* g_US_RRT_eng_abbrev_vals_Fantasy_Violence[2] =
{ "", "FV" };
rmf_SiRatingDimension g_US_RRT_dim_Fantasy_Violence =
{ NULL, 0, 2, g_US_RRT_val_Fantasy_Violence };
static const char* g_US_RRT_eng_dim_Fantasy_Violence = "Fantasy Violence";


static rmf_SiRatingValuesDefined g_US_RRT_val_MPAA[9];
static const char* g_US_RRT_eng_vals_MPAA[9] =
{ "", "MPAA Rating Not Applicable", "Suitable for All Ages",
    "Parental Guidance Suggested", "Parents Strongly Cautioned",
    "Restricted, under 17 must be accompanied by adult",
    "No One 17 and Under Admitted", "No One 17 and Under Admitted",
    "Not Rated by MPAA" };
static const char* g_US_RRT_eng_abbrev_vals_MPAA[9] =
{ "", "N/A", "G", "PG", "PG-13", "R", "NC-17", "X", "NR" };

static rmf_SiRatingDimension g_US_RRT_dim_MPAA =
{ NULL, 0, 9, g_US_RRT_val_MPAA };
static const char* g_US_RRT_eng_dim_MPAA = "MPAA";


static rmf_SiRatingDimension g_RRT_US_dimension[8];
static rmf_SiRatingRegion g_RRT_US =
{ NULL, 8, g_RRT_US_dimension };
static const char* g_RRT_US_eng = "US (50 states + possessions)";

/* *End* Rating region US (50 states + possessions) */

/* DSG application */

static const char *modulationModes[] =
{ "UNKNOWN", "QPSK", "BPSK", "OQPSK", "VSB8", "VSB16", "QAM16", "QAM32",
    "QAM64", "QAM80", "QAM96", "QAM112", "QAM128", "QAM160", "QAM192",
    "QAM224", "QAM256", "QAM320", "QAM384", "QAM448", "QAM512", "QAM640",
    "QAM768", "QAM896", "QAM1024" };

static const char *si_status_strings[] =
{ "Not Available", "Pending", "Available" };

static char g_badSICacheLocation[250] = "";
static char g_badSISNSCacheLocation[250] = "";

static pthread_mutex_t g_pSiCacheMutex = PTHREAD_MUTEX_INITIALIZER;

rmf_OobSiCache *rmf_OobSiCache::m_pSiCache = NULL;

rmf_OobSiCache::rmf_OobSiCache()
{
        int i = 0;
    rmf_Error err = RMF_SUCCESS;
    memset(crctab,'\0',sizeof(crctab));

    g_si_state = SI_ACQUIRING;
    g_siSTTReceived = FALSE;
    sttAquiredStatus = FALSE;

    m_pSiEventListener = NULL;
       /* Points to the top of the si_table_entry linked list */
        g_si_entry = NULL;

        g_si_sourcename_entry = NULL;

    /*  Control variables for the various tables */
    g_frequenciesUpdated = FALSE;
    g_modulationsUpdated = FALSE;
    g_channelMapUpdated = FALSE;
    DACId = VIRTUAL_CHANNEL_UNKNOWN;
    ChannelMapId = VIRTUAL_CHANNEL_UNKNOWN;
    VCTId = VIRTUAL_CHANNEL_UNKNOWN;
    //g_sitp_si_status_update_time_interval = 0;
    //g_sitp_pod_status_update_time_interval = 0;
    g_sitp_si_dump_tables = 0;
/*  g_sitp_si_update_poll_interval = 1;
    g_sitp_si_update_poll_interval_max = 0;
    g_sitp_si_update_poll_interval_min = 0;
    g_stip_si_process_table_revisions = 0;*/
    g_sitp_si_timeout = 0;
/*  g_sitp_si_profile = PROFILE_CABLELABS_OCAP_1;
    g_sitp_si_profile_table_number = 0;*/
    g_sitp_si_versioning_by_crc32 = 0;
    /*g_sitp_si_max_nit_cds_wait_time = 0;
    g_sitp_si_max_nit_mms_wait_time = 0;
    g_sitp_si_max_svct_dcm_wait_time = 0;
    g_sitp_si_max_svct_vcm_wait_time = 0;
    g_sitp_si_max_ntt_wait_time = 0;
    g_sitp_si_section_retention_time = 0;*/

    //g_sitp_si_filter_multiplier = 0;
    /*g_sitp_si_max_nit_section_seen_count = 0;
    g_sitp_si_max_vcm_section_seen_count = 0;
    g_sitp_si_max_dcm_section_seen_count = 0;
    g_sitp_si_max_ntt_section_seen_count = 0;*/
    //g_sitp_si_initial_section_match_count = 0;
    //g_sitp_si_rev_sample_sections = 0;

#if 0
        /* Points to the top of the transport stream linked list */
        g_si_ts_entry = NULL;
        /* One and only OOB transport stream */
        g_si_oob_ts_entry = NULL;
        /* One and only HN transport stream */
        g_si_HN_ts_entry = NULL;
#endif
         /*  Following variables are used to determine whether or not to
            enforce frequency range check.
         */
        siOOBEnabled = NULL;
        g_frequencyCheckEnable = FALSE;
        g_SITPConditionSet = FALSE;

        /*  Maximum and minimum frequencies in the NIT_CDS table */
        g_maxFrequency = 0;
        g_minFrequency = 0;

        g_cond_counter = 0;
        g_global_si_mutex = NULL; /* sync primitive */

        /*  Lock for transport stream list */
        g_global_si_list_mutex = NULL; /* sync primitive */

        g_dsg_mutex = NULL; /* DSG sync primitive */

        g_HN_mutex = NULL; /* HN sync primitive */

        g_si_dsg_ts_entry = NULL;

        g_global_si_cond = NULL;

        g_SITP_cond = NULL;

        /*  Global variable to indicate the total number of services in the SI DB
            This variable currently only includes those services that are signalled in
            OOB SI (SVCT). */
        g_numberOfServices = 0;

        /* Initialize the frequency, mode arrays */
        for (i = 1; i <= MAX_FREQUENCIES; i++)
        {
                g_frequency[i] = 0;
                g_mode[i] = RMF_MODULATION_UNKNOWN;
        }

        /*
     * Initilize the default rating region
     */
        init_default_rating_region();

#ifdef ISV_API
        /* MOT needs to fake the SI info for now... this will be removed when
     section filtering is working.
     */
        g_si_entry = spoofSIInfo();
#endif

        /* Create global mutex */
        err = rmf_osal_mutexNew(&g_global_si_mutex);
        if (err != RMF_SUCCESS)
        {
                RDK_LOG(RDK_LOG_WARN, "LOG.RDK.SI",
                                "<rmf_si_Database_Init() Mutex creation failed, returning RMF_OSAL_EMUTEX\n");
                //return RMF_OSAL_EMUTEX;
        }

        /* Create global list mutex */
        err = rmf_osal_mutexNew(&g_global_si_list_mutex);
        if (err != RMF_SUCCESS)
        {
                RDK_LOG(RDK_LOG_WARN, "LOG.RDK.SI",
                                "<rmf_si_Database_Init() Mutex creation failed, returning RMF_OSAL_EMUTEX\n");
                //return RMF_OSAL_EMUTEX;
        }

        /* Create dsg mutex */
        err = rmf_osal_mutexNew(&g_dsg_mutex);
        if (err != RMF_SUCCESS)
        {
                RDK_LOG(RDK_LOG_WARN, "LOG.RDK.SI",
                                "<rmf_si_Database_Init() Mutex creation failed, returning RMF_OSAL_EMUTEX\n");
                //return RMF_OSAL_EMUTEX;
        }

    /* Create Hn mutex */
    err = rmf_osal_mutexNew(&g_HN_mutex);
    if (err != RMF_SUCCESS)
    {
        RDK_LOG(RDK_LOG_WARN, "LOG.RDK.SI",
                "<mpe_si_Database_Init() Mutex creation failed, returning RMF_OSAL_EMUTEX\n");
        //return RMF_OSAL_EMUTEX;
    }
        /*
     Global condition used to lock the SIDB.
     Used by readers and the writer to acquire
     exclusive lock.
     Initial state is set to TRUE. It means the condition is available
     to be acquired.
     'autoReset' is set to TRUE also, to indicate once a lock
     is acquired by a reader/writer it stays in that state until
     the lock is released by that reader/writer.
     */
        if (rmf_osal_condNew(TRUE, TRUE, &g_global_si_cond) != RMF_SI_SUCCESS)
        {
                //return RMF_OSAL_EMUTEX;
        }

        /*  Create global (SITP) condition object
     By setting the initial state of g_SITP_cond to 'FALSE'
     we are blocking all readers until SVCT is acquired and the
     condition object is set by SITP.
     'autoreset' is set to FALSE which means that once the
     condition object is in set (TRUE) state it stays in that
     state implying all the readers will be able to read from
     SI DB.
     */
        if (rmf_osal_condNew(FALSE, FALSE, &g_SITP_cond) != RMF_SI_SUCCESS)
        {
                //return RMF_OSAL_EMUTEX;
       }

        rmf_osal_timeGetMillis(&gtime_network);

        /*
     This has been added to determine whether or not to enforce
     a frequency check in GetServicehandleByFrequencyModulationProgramNumber()
     API. For dynamic services (ex:VOD) not found in SI DB, a brand new entry
     is created at the time handle is requested. However, we still need to check
     if the frequency is with in the acceptable range. The values for minimum and
     maximum frequencies are extracted from NIT_CDS table. In the case where
     there are no OOB tables (i.e. if OOB is disabled) we don't want to enforce
     the frequency check since minimum and maximum freq values are unknown.
     */
        siOOBEnabled = rmf_osal_envGet("SITP.SI.ENABLED");
        if ((siOOBEnabled == NULL) || (stricmp(siOOBEnabled, "TRUE") == 0))
        {
                RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                                "OOB is enabled by SITP.SI.ENABLED\n");
                g_frequencyCheckEnable = TRUE;
        }
        else
        {
                RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                                "OOB is disabled by SITP.SI.ENABLED\n");
        }

        m_pInbSiCache = rmf_InbSiCache::getInbSiCache();
    
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "mpe_SI_Database_Init: SI state: %d \n",
                        g_si_state);
}

rmf_OobSiCache::~rmf_OobSiCache()
{
        rmf_SiTableEntry *walker, *next;

        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<mpe_si_Database_Clear()\n");

        m_pSiEventListener = NULL;
        /* Clear the database entries */

        /* Use internal method release_si_entry() to delete service entries */
        for (walker = g_si_entry; walker; walker = next)
        {
                release_si_entry(walker);
                next = walker->next;
                rmf_osal_memFreeP(RMF_OSAL_MEM_SI_CACHE_OOB, walker);
        }

        /* delete mutex */
        rmf_osal_mutexDelete(g_global_si_mutex);
        rmf_osal_mutexDelete(g_global_si_list_mutex);
        rmf_osal_mutexDelete(g_dsg_mutex);

    rmf_osal_mutexDelete(g_HN_mutex);

        /* delete global condition object */
        rmf_osal_condDelete(g_global_si_cond);

        /* delete SITP condition object */
        rmf_osal_condDelete(g_SITP_cond);
}

rmf_Error rmf_OobSiCache::Configure()
{
    rmf_Error retCode = RMF_SUCCESS;
/*    const char *sitp_si_max_nit_cds_wait_time = NULL;
    const char *sitp_si_max_nit_mms_wait_time = NULL;
    const char *sitp_si_max_svct_dcm_wait_time = NULL;
    const char *sitp_si_max_svct_vcm_wait_time = NULL;
    const char *sitp_si_max_ntt_wait_time = NULL;
    const char *sitp_si_section_retention_time = NULL;*/
    const char *sitp_si_enabled = NULL;
/*    const char *sitp_si_update_poll_interval_min = NULL;
    const char *sitp_si_update_poll_interval_max = NULL;*/
    //const char *sitp_si_status_update_time_interval = NULL;
    const char *sitp_si_dump_tables = NULL;
    const char *sitp_si_versioning_by_crc = NULL; 
    //const char *sitp_si_filter_multiplier = NULL;
  /*  const char *sitp_si_max_nit_section_seen_count = NULL;
    const char *sitp_si_max_vcm_section_seen_count = NULL;
    const char *sitp_si_max_dcm_section_seen_count = NULL;
    const char *sitp_si_max_ntt_section_seen_count = NULL;*/
    //const char *sitp_si_initial_section_match_count = NULL;
    //const char *sitp_si_rev_sample_sections = NULL;
    //const char *sitp_si_process_table_revisions = NULL;
    const char *sitp_si_parse_stt = NULL;
    const char *sitp_si_stt_filter_match_count = NULL;
    const char *siOOBCacheEnabled = NULL;
    const char *siOOBCacheConvert = NULL;
        /*
     * Get all of the config variables from the ini.
     */
        sitp_si_parse_stt = rmf_osal_envGet("SITP.SI.STT.ENABLED");
        if ((sitp_si_parse_stt != NULL)
                        && (stricmp(sitp_si_parse_stt, "TRUE") == 0))
        {
                g_sitp_si_parse_stt = TRUE;
        }
        else
        {
                g_sitp_si_parse_stt = FALSE;
        }
        RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                        "<%s> - Parse STT is %s\n", __FUNCTION__,
                        (g_sitp_si_parse_stt ? "ON" : "OFF"));

        sitp_si_stt_filter_match_count = rmf_osal_envGet(
                        "SITP.SI.STT.FILTER.MATCH.COUNT");
        if (sitp_si_stt_filter_match_count != NULL)
        {
                g_sitp_si_stt_filter_match_count = atoi(sitp_si_stt_filter_match_count);
        }
        else
        {
                // Default match count is 1
                g_sitp_si_stt_filter_match_count = 1;
        }
        RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                        "<%s> - STT filter match count is %d\n", __FUNCTION__,
                        g_sitp_si_stt_filter_match_count);

#if 0
        sitp_si_status_update_time_interval = rmf_osal_envGet(
                        "SITP.SI.STATUS.UPDATE.TIME.INTERVAL");
        if (sitp_si_status_update_time_interval != NULL)
        {
                g_sitp_si_status_update_time_interval = atoi(
                                sitp_si_status_update_time_interval);
        }
        else
        {
                g_sitp_si_status_update_time_interval = 15000;
        }
        RDK_LOG(
                        RDK_LOG_TRACE4,
                        "LOG.RDK.SI",
                        "<%s> - SI status update time interval set to %d ms\n",
                        __FUNCTION__, g_sitp_si_status_update_time_interval);
#endif

#if 0
        sitp_si_update_poll_interval_max = rmf_osal_envGet(
                        "SITP.SI.MAX.UPDATE.POLL.INTERVAL");
        if (sitp_si_update_poll_interval_max != NULL)
        {
                g_sitp_si_update_poll_interval_max = atoi(
                                sitp_si_update_poll_interval_max);
        }
        else
        {
                g_sitp_si_update_poll_interval_max = 30000;
        }
        RDK_LOG(
                        RDK_LOG_TRACE4,
                        "LOG.RDK.SI",
                        "<%s> - SI update max polling interval set to %d ms\n",
                        __FUNCTION__, g_sitp_si_update_poll_interval_max);

        sitp_si_update_poll_interval_min = rmf_osal_envGet(
                        "SITP.SI.MIN.UPDATE.POLL.INTERVAL");
        if (sitp_si_update_poll_interval_min != NULL)
        {
                g_sitp_si_update_poll_interval_min = atoi(
                                sitp_si_update_poll_interval_min);
        }
        else
        {
                g_sitp_si_update_poll_interval_min = 25000;
        }
        RDK_LOG(
                        RDK_LOG_TRACE4,
                        "LOG.RDK.SI",
                        "<%s> - SI update min polling interval set to %d ms\n",
                        __FUNCTION__, g_sitp_si_update_poll_interval_min);

        if (g_sitp_si_update_poll_interval_min > g_sitp_si_update_poll_interval_max)
        {
                RDK_LOG(
                                RDK_LOG_TRACE4,
                                "LOG.RDK.SI",
                                "<%s> - min polling interval greater than max polling interval - using default values\n",
                                __FUNCTION__);
                g_sitp_si_update_poll_interval_min = 25000;
                g_sitp_si_update_poll_interval_max = 30000;
        }
        /*
     ** ***********************************************************************************************
     **  Acquire NIT/SVCT acquisition policy from the environment.
     **  If it is not defined, the default value is for SVCT/NIT acquisition
     **  to be continuous.
     ** ************************************************************************************************
     */
        sitp_si_process_table_revisions = rmf_osal_envGet(
                        "SITP.SI.PROCESS.TABLE.REVISIONS");
        if ((NULL == sitp_si_process_table_revisions) || (stricmp(
                        sitp_si_process_table_revisions, "TRUE") == 0))
        {
                g_stip_si_process_table_revisions = 1;
        }
        else
        {
                g_stip_si_process_table_revisions = 0;
        }
        RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                        "<%s> - table revision processing is %s\n",
                        __FUNCTION__, (g_stip_si_process_table_revisions ? "ON" : "OFF"));

        sitp_si_rev_sample_sections = rmf_osal_envGet("SITP.SI.REV.SAMPLE.SECTIONS");
        if ((sitp_si_rev_sample_sections != NULL) && (stricmp(
                        sitp_si_rev_sample_sections, "TRUE") == 0))
        {
                g_sitp_si_rev_sample_sections = 1;
        }
        else
        {
                g_sitp_si_rev_sample_sections = 0;
        }
        RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                        "<%s> - sample for table revisions %s\n", __FUNCTION__,
                        (g_sitp_si_rev_sample_sections ? "ON" : "OFF"));
#endif
        sitp_si_dump_tables = rmf_osal_envGet("SITP.SI.DUMP.CHANNEL.TABLES");
        if ((sitp_si_dump_tables != NULL) && (stricmp(sitp_si_dump_tables, "TRUE")
                        == 0))
        {
                g_sitp_si_dump_tables = 1;
        }
        else
        {
                g_sitp_si_dump_tables = 0;
        }
        RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                        "<%s> - SI channel map dumping is %s\n", __FUNCTION__,
                        (g_sitp_si_dump_tables ? "ON" : "OFF"));

        sitp_si_versioning_by_crc = rmf_osal_envGet("SITP.SI.VERSION.BY.CRC");
        if ((sitp_si_versioning_by_crc == NULL) || (stricmp(
                        sitp_si_versioning_by_crc, "FALSE") == 0))
        {
                g_sitp_si_versioning_by_crc32 = 0;
        }

        else
        {
                g_sitp_si_versioning_by_crc32 = 1;
        }
        RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                        "<%s> - SI revisioning by crc is %s\n", __FUNCTION__,
                        (g_sitp_si_versioning_by_crc32 ? "ON" : "OFF"));

#if 0
        sitp_si_filter_multiplier = rmf_osal_envGet("SITP.SI.FILTER.MULTIPLIER");
        if (sitp_si_filter_multiplier != NULL)
        {
                if (0 == atoi(sitp_si_filter_multiplier))
                        g_sitp_si_filter_multiplier = 0;
                g_sitp_si_filter_multiplier = atoi(sitp_si_filter_multiplier);
        }
        else
        {
                g_sitp_si_filter_multiplier = 2;
        }
        RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                        "<%s> - SI filter multiplier: %d\n", __FUNCTION__,
                        g_sitp_si_filter_multiplier);

        sitp_si_max_nit_section_seen_count = rmf_osal_envGet(
                        "SITP.SI.MAX.NIT.SECTION.SEEN.COUNT");
        if (sitp_si_max_nit_section_seen_count != NULL)
        {
                /*
         * Set this value to at least 2 b/c, the sections need minimal
         * verification before the table can be marked as found
         */
                if (atoi(sitp_si_max_nit_section_seen_count) < 2)
                        g_sitp_si_max_nit_section_seen_count = 2;
                else
                        g_sitp_si_max_nit_section_seen_count = atoi(
                                        sitp_si_max_nit_section_seen_count);
        }
        else
        {
                g_sitp_si_max_nit_section_seen_count = 2;
        }
        RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                        "<%s> - NIT section seen count: %d\n", __FUNCTION__,
                        g_sitp_si_max_nit_section_seen_count);

        sitp_si_max_vcm_section_seen_count = rmf_osal_envGet(
                        "SITP.SI.MAX.VCM.SECTION.SEEN.COUNT");
        if (sitp_si_max_vcm_section_seen_count != NULL)
        {
                /*
         * Set this value to at least 2 b/c, the sections need minimal
         * verification before the table can be marked as found
         */
                if (atoi(sitp_si_max_vcm_section_seen_count) < 2)
                        g_sitp_si_max_vcm_section_seen_count = 2;
                else
                        g_sitp_si_max_vcm_section_seen_count = atoi(
                                        sitp_si_max_vcm_section_seen_count);
        }
        else
        {
                g_sitp_si_max_vcm_section_seen_count = 4;
        }
        RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                        "<%s> - SVCT-VCM section seen count: %d\n",
                        __FUNCTION__, g_sitp_si_max_vcm_section_seen_count);

        sitp_si_max_dcm_section_seen_count = rmf_osal_envGet(
                        "SITP.SI.MAX.DCM.SECTION.SEEN.COUNT");
        if (sitp_si_max_dcm_section_seen_count != NULL)
        {
                g_sitp_si_max_dcm_section_seen_count = atoi(
                                sitp_si_max_dcm_section_seen_count);
        }
        else
        {
                g_sitp_si_max_dcm_section_seen_count = 2;
        }
        RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                        "<%s> - SVCT-DCM section seen count: %d\n",
                        __FUNCTION__, g_sitp_si_max_dcm_section_seen_count);

        sitp_si_max_ntt_section_seen_count = rmf_osal_envGet(
                        "SITP.SI.MAX.NTT.SECTION.SEEN.COUNT");
        if (sitp_si_max_ntt_section_seen_count != NULL)
        {
                /*
         * Set this value to at least 2 b/c, the sections need minimal
         * verification before the table can be marked as found
         */
                if (atoi(sitp_si_max_ntt_section_seen_count) < 2)
                        g_sitp_si_max_ntt_section_seen_count = 2;
                else
                        g_sitp_si_max_ntt_section_seen_count = atoi(
                                        sitp_si_max_ntt_section_seen_count);
        }
        else
        {
                g_sitp_si_max_ntt_section_seen_count = 3;
        }
        RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                        "<%s> - SI NTT section seen count: %d\n", __FUNCTION__,
                        g_sitp_si_max_ntt_section_seen_count);

        sitp_si_initial_section_match_count = rmf_osal_envGet(
                        "SITP.SI.INITIAL.SECTION.MATCH.COUNT");
        if (sitp_si_initial_section_match_count != NULL)
        {
                g_sitp_si_initial_section_match_count = atoi(
                                sitp_si_initial_section_match_count);

        }
        else
        {
            g_sitp_si_initial_section_match_count = 50;
        }
        RDK_LOG(
                        RDK_LOG_TRACE4,
                        "LOG.RDK.SI",
                        "<%s> - Initial acquisition section match count: %d\n",
                        __FUNCTION__, g_sitp_si_initial_section_match_count);
    sitp_si_max_nit_cds_wait_time = rmf_osal_envGet(
            "SITP.SI.MAX.NIT.CDS.WAIT.TIME");
    if (sitp_si_max_nit_cds_wait_time != NULL)
    {
        g_sitp_si_max_nit_cds_wait_time = atoi(sitp_si_max_nit_cds_wait_time);
    }
    else
    {
        // default wait time for NIT CDS - 1 minute
        g_sitp_si_max_nit_cds_wait_time = 60000;
    }

    RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
            "<%s> - Max NIT CDS wait time: %d\n", __FUNCTION__,
            g_sitp_si_max_nit_cds_wait_time);
    sitp_si_max_nit_mms_wait_time = rmf_osal_envGet(
            "SITP.SI.MAX.NIT.MMS.WAIT.TIME");
    if (sitp_si_max_nit_mms_wait_time != NULL)
    {
        g_sitp_si_max_nit_mms_wait_time = atoi(sitp_si_max_nit_mms_wait_time);
    }
    else
    {
        // default wait time for NIT MMS - 1 minute
        g_sitp_si_max_nit_mms_wait_time = 60000;
    }
    RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
            "<%s> - Max NIT MMS wait time: %d\n", __FUNCTION__,
            g_sitp_si_max_nit_mms_wait_time);

        sitp_si_max_svct_dcm_wait_time = rmf_osal_envGet(
                        "SITP.SI.MAX.SVCT.DCM.WAIT.TIME");
        if (sitp_si_max_svct_dcm_wait_time != NULL)
        {
                g_sitp_si_max_svct_dcm_wait_time = atoi(sitp_si_max_svct_dcm_wait_time);
        }
        else
        {
                // default wait time for DCM - 2 and a half minutes
                g_sitp_si_max_svct_dcm_wait_time = 150000;
        }
        RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                        "<%s> - Max SVCT DCM wait time: %d\n", __FUNCTION__,
                        g_sitp_si_max_svct_dcm_wait_time);

        sitp_si_max_svct_vcm_wait_time = rmf_osal_envGet(
                        "SITP.SI.MAX.SVCT.VCM.WAIT.TIME");
        if (sitp_si_max_svct_vcm_wait_time != NULL)
        {
                g_sitp_si_max_svct_vcm_wait_time = atoi(sitp_si_max_svct_vcm_wait_time);
        }
        else
        {
                // default wait time for VCM - 4 minutes
                g_sitp_si_max_svct_vcm_wait_time = 240000;
        }
        RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                        "<%s> - Max SVCT VCM wait time: %d\n", __FUNCTION__,
                        g_sitp_si_max_svct_vcm_wait_time);

        sitp_si_max_ntt_wait_time = rmf_osal_envGet("SITP.SI.MAX.NTT.WAIT.TIME");
        if (sitp_si_max_ntt_wait_time != NULL)
        {
                g_sitp_si_max_ntt_wait_time = atoi(sitp_si_max_ntt_wait_time);
        }
        else
        {
                // default wait time 2 and a half minutes
                g_sitp_si_max_ntt_wait_time = 150000;
        }
        RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                        "<%s> - Max NTT wait time: %d\n", __FUNCTION__,
                        g_sitp_si_max_ntt_wait_time);


    sitp_si_section_retention_time = rmf_osal_envGet("SITP.SI.SECTION.RETENTION.TIME");
    if (sitp_si_section_retention_time != NULL)
    {
        g_sitp_si_section_retention_time = atoi(sitp_si_section_retention_time);
    }
    else
    {
        // default section retention threshold is 2 and a half minutes
        g_sitp_si_section_retention_time = 150000;
    }
        RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
            "<%s> - Section retention threshold: %dms\n", __FUNCTION__,
            g_sitp_si_section_retention_time);

#endif


        //************************************************************************************************
        // Launch the OUT_OF-BAND (OOB)thread which will periodically acquire the 2 NIT subtables
        // (CDS, MMS) and SVCT (VCM, DCM subtables) and NTT-SNS table.
        //************************************************************************************************

        sitp_si_enabled = rmf_osal_envGet("SITP.SI.ENABLED");
        if ((sitp_si_enabled == NULL) || (stricmp(sitp_si_enabled, "TRUE") == 0))
    {
            /*
               This has been added to determine whether or not to cache the g_si_entry on persistent
               storage. If cached, the next boot wiill use the cache to populate g_si_entry immediately.
               Later acquired SI data can be used to update g_si_entry.
             */
            siOOBCacheEnabled = rmf_osal_envGet("SITP.SI.CACHE.ENABLED");
            if ( (siOOBCacheEnabled == NULL) || (stricmp(siOOBCacheEnabled,"FALSE") == 0))
            {
                RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI","OOB Caching is disabled by SITP.SI.CACHE.ENABLED\n");
            }
            else
            {
                RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI","OOB Caching is enabled by SITP.SI.CACHE.ENABLED\n");
                g_siOOBCacheEnable = TRUE;
            }
            if (g_siOOBCacheEnable)
            {
                /* Check whether the Convertion of 114 to 121 SI cache, When 121 cache is not available */
                siOOBCacheConvert =  rmf_osal_envGet("SITP.SI.CACHE.CONVERT");
                if ( (siOOBCacheConvert != NULL) && (stricmp(siOOBCacheConvert, "TRUE") == 0))
                {
                    g_siOOBCacheConvertStatus = TRUE;
                    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI","OOB Cache convertion is enabled by SITP.SI.CACHE.CONVERT\n");
                }
                else
                {
                    g_siOOBCacheConvertStatus = FALSE;
                    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI","OOB Cache convertion is disabled by SITP.SI.CACHE.CONVERT\n");
                }
            
                g_siOOBCacheLocation = rmf_osal_envGet("SITP.SI.CACHE.LOCATION");
                if ( (g_siOOBCacheLocation == NULL))
                {
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI","OOB Caching location is not specified\n");
                    g_siOOBCacheLocation = "/tmp/mnt/diska3/persistent/si/si_table_cache.bin";
                }
                RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI","OOB Caching is enabled by SITP.SI.CACHE.ENABLED\n");

                g_siOOBCachePost114Location = rmf_osal_envGet("SITP.SI.CACHE.POST114.LOCATION");

                if ( (g_siOOBCachePost114Location == NULL))
                {
                    //g_siOOBCachePost114Location = "/syscwd/persistent/si/si_table_cache.bin";
                    g_siOOBCachePost114Location = "/tmp/mnt/diska3/persistent/si/si_table_cache.bin";
                }

                g_siOOBSNSCacheLocation = rmf_osal_envGet("SITP.SNS.CACHE.LOCATION");
                if ( (g_siOOBSNSCacheLocation == NULL))
                {
                    g_siOOBSNSCacheLocation = "/tmp/mnt/diska3/persistent/si/sns_table_cache.bin";
                }

                memset (g_badSISNSCacheLocation, '\0', sizeof(g_badSISNSCacheLocation));
                memset (g_badSICacheLocation, '\0', sizeof(g_badSICacheLocation));

                snprintf( g_badSISNSCacheLocation, sizeof(g_badSISNSCacheLocation), "%s%s", g_siOOBSNSCacheLocation, ".bad");
                snprintf( g_badSICacheLocation, sizeof(g_badSICacheLocation), "%s%s", g_siOOBCachePost114Location, ".bad"); 

                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI","OOB Caching Location is [%s]\n", g_siOOBCacheLocation);
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI","OOB Post114 Caching Location is [%s]\n", g_siOOBCachePost114Location);
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI","OOB Post114 SNS Caching Location is [%s]\n", g_siOOBSNSCacheLocation);
            }
        }
        else
        {
                RDK_LOG(
                                RDK_LOG_TRACE4,
                                "LOG.RDK.SI",
                                "<%s> - sitp_siWorkerThread thread disabled by SITP.SI.ENABLED\n",
                                __FUNCTION__);

                // The OOB thread is disabled so we will not be getting a SVCT.
                // Signal that it's ok to access the SI database.
                RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                                "<%s> - Calling SetGlobalState.\n",
                                __FUNCTION__);

            SetGlobalState ( SI_DISABLED);
        }
        /*if ((sitp_si_enabled == NULL) || (atoi(sitp_si_enabled) != 0))
     {
     RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI","<%s> - sitp_siWorkerThread thread set to be started?\n",
     __FUNCTION__);
     }
     else
     {
     RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI","<%s> - sitp_siWorkerThread thread disabled by SITP.SI.ENABLED\n",
     __FUNCTION__);

     // The OOB thread is disabled so we will not be getting a SVCT.
     // Signal that it's ok to access the SI database.
     RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI","<%s> - Calling mpe_siSetGlobalState.\n",
     __FUNCTION__);

     mpe_siSetGlobalState (SI_NOT_AVAILABLE_YET);
     }
     */
        return retCode;
}

void rmf_OobSiCache::AddSiEventListener(rmf_OobSiEventListener *pSiEventListener)
{
    m_pSiEventListener = pSiEventListener;
}

void rmf_OobSiCache::RemoveSiEventListener()
{
    m_pSiEventListener = NULL;
}


rmf_Error rmf_OobSiCache::ChanMapReset(void)
{
        rmf_SiTableEntry *walker, *next;
        /* Acquire global mutex */
        rmf_osal_mutexAcquire(g_global_si_mutex);

        /* Use internal method release_si_entry() to delete service entries */
        for (walker = g_si_entry; walker; walker = next)
        {
                release_si_entry(walker);
                next = walker->next;
                rmf_osal_memFreeP(RMF_OSAL_MEM_SI_CACHE_OOB, walker);
        }
        g_si_entry = NULL;

        rmf_osal_mutexRelease(g_global_si_mutex);

        return RMF_SI_SUCCESS;
}

/* NIT_CD */
/* Populate carrier frequencies from NIT_CD table */
/*
 Called by SITP when NIT_CDS is read.
 The index into carrier frequency table is 8 bits long.
 Hence the maxium entries are set to 255.
 */
rmf_Error rmf_OobSiCache::SetCarrierFrequencies(uint32_t frequency[], uint8_t offset,
                uint8_t length)
{
        uint8_t i, j;

        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "mpe_siSetCarrierFrequencies()\n");
        /* Parameter check */
        if ((frequency == NULL) || ((uint16_t) offset + length > MAX_FREQUENCIES
                        + 1) || (length == 0))
        {
                return RMF_SI_INVALID_PARAMETER;
        }

        /* Set frequencies */
        for (i = offset, j = 0; j < length; i++, j++)
        {
                /*  Store frequencies in the global array to be looked up
         at the time of query. */
                g_frequency[i] = frequency[j];
                RDK_LOG(
                                RDK_LOG_TRACE1,
                                "LOG.RDK.SI",
                                "mpe_siSetCarrierFrequencies() g_frequency[%d] = frequency[%d] (%d)\n",
                                i, j, g_frequency[i]);
        }

        return RMF_SI_SUCCESS;
}

/* NIT_MM */
/* Populate modulation modes from NIT_MM table */
/*
 Called by SITP when NIT_MMS is read.
 The index into modulation mode table is 8 bits long.
 Hence the maxium entries for this table are set to 255.
 */
rmf_Error rmf_OobSiCache::SetModulationModes(rmf_ModulationMode mode[], uint8_t offset,
                uint8_t length)
{
        uint8_t i, j;

        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "SetModulationModes()\n");
        /* Parameter check */
        if ((mode == NULL) || ((uint16_t) offset + length > MAX_FREQUENCIES + 1)
                        || (length == 0))
        {
                return RMF_SI_INVALID_PARAMETER;
        }

        /* Set modulation modes */
        for (i = offset, j = 0; j < length; i++, j++)
        {
                /*  Store modes in the global array to be looked up
         at the time of query. */
                g_mode[i] = mode[j];
                RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                                "SetModulationModes() g_mode[%d] = mode[%d] (%d)\n", i,
                                j, mode[j]);
        }

        return RMF_SI_SUCCESS;
}


/**
 * @brief This function is used to check whether SI tables is fully acquired from out of band SI acquiring.
 * Out of band SI acquiring is capable to recieve NIT/SVCT/NTT tables.
 *
 * @return rmf_osal_Bool
 * @retval TRUE If SITPConditioSet is True.
 * @retval FALSE IF SITPConditionSet is false.
 */
rmf_osal_Bool rmf_OobSiCache::IsSIFullyAcquired( )
{
	return g_SITPConditionSet;
}


/**
 Acqure Service handles using one of the three methods below
 1. By sourceId
 2. By freq/prog_num pair
 3. By service name
 4. By service major/minor number
 */

/**
 * @brief  Acquire a service handle from the given major/minor service number.
 * To find the matching service numbers for the service handle, need to check the list of services in SI table.
 *
 * @param[in] majorNumber Service number (major) that uniquely identifies a given service.
 * @param[in] minorNumber Service number (minor) that uniquely identifies
 * a given service.  This value may be passed as -1 and it will be disregarded.
 * @param[out] service_handle Output parameter to populate the service handle.
 *
 * @retval RMF_SI_SUCCESS Returned when successfully acquired the service handle.
 * @retval RMF_SI_INVALID_PARAMETER Returned If service handle parameter is NULL.
 * @retval RMF_SI_NOT_AVAILABLE_YET Returned when SI info is acquired and not available yet.
 * @retval RMF_SI_NOT_AVAILABLE Returned when SI info is not available and SI state is disabled.
 * @retval RMF_SI_NOT_FOUND Returned when matching service numbers is not found.
 */
rmf_Error rmf_OobSiCache::GetServiceHandleByServiceNumber(uint32_t majorNumber,
        uint32_t minorNumber, rmf_SiServiceHandle *service_handle)
{
    rmf_SiTableEntry *walker = NULL;

    /* Parameter check */
    if (service_handle == NULL)
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<GetServiceHandleByServiceNumber> major=0x%x, minor=0x%x\n",
            majorNumber, minorNumber);

    /* Clear out the value */
    *service_handle = RMF_SI_INVALID_HANDLE;

    // If SI DB is in either of these states return SI_NOT_AVAILABLE_YET
    // SI_ACQUIRING state is not useful for callers
    if (g_si_state == SI_ACQUIRING || g_si_state == SI_NOT_AVAILABLE_YET)
        return RMF_SI_NOT_AVAILABLE_YET;
    else if (g_si_state == SI_DISABLED)
        return RMF_SI_NOT_AVAILABLE;

    /* Walk through the list of services to find a matching service numbers. */
    walker = g_si_entry;
    while (walker)
    {
        if (((walker->state == SIENTRY_MAPPED) || (walker->state
                == SIENTRY_DEFINED_MAPPED)) && (walker->major_channel_number
                == majorNumber) && (minorNumber == (uint32_t) - 1
                || walker->minor_channel_number == minorNumber))
        {
            *service_handle = (rmf_SiServiceHandle) walker;
            walker->ref_count++;
            return RMF_SI_SUCCESS;
        }
        walker = walker->next;
    }

    /* Matching service numbers not found, return error */
    return RMF_SI_NOT_FOUND;
}

/**
 * @brief  Acquire a service handle from the given source Id.
 * To find the matching sourceId for the service handle, need to walk through the list of services in SI table.
 * Referencing to the SCTE Specification, there may be multiple services for the same sourceId in channel map.
 * So, this function will return the first found entry that matches the sourceId.
 *
 * @param[in] sourceId  Value that uniquely identifies a given service.
 * @param[out] service_handle Output parameter to populate the service handle.
 *
 * @retval RMF_SI_SUCCESS Returned when successfully acquired the service handle.
 * @retval RMF_SI_INVALID_PARAMETER Returned when service handle is NULL.
 * @retval RMF_SI_NOT_AVAILABLE_YET Returned when SI info is not availablr yet.
 * @retval RMF_SI_NOT_AVAILABLE Returned when SI info is not available and SI state is disabled.
 * @retval RMF_SI_NOT_FOUND Returned when matching sourceId is not found.
 */
rmf_Error rmf_OobSiCache::GetServiceHandleBySourceId(uint32_t sourceId,
        rmf_SiServiceHandle *service_handle)
{
    rmf_Error err = RMF_SI_SUCCESS;
    rmf_SiTableEntry *walker = NULL;
    //rmf_SiTransportStreamEntry *ts_entry = NULL;

    /* Parameter check */
    if (service_handle == NULL)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI",
                "<GetServiceHandleBySourceId> null pointer to service_handle!\n");
        err = RMF_SI_INVALID_PARAMETER;
    }
    else
    {
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                "<GetServiceHandleBySourceId> 0x%x\n", sourceId);

        /* Clear out the value */
        *service_handle = RMF_SI_INVALID_HANDLE;

        // If SI DB is in either of these states return SI_NOT_AVAILABLE_YET
        // SI_ACQUIRING state is not useful for callers
        if ((g_si_state == SI_ACQUIRING)
                || (g_si_state == SI_NOT_AVAILABLE_YET))
        {
            err = RMF_SI_NOT_AVAILABLE_YET;
        }
        else
        {
            if (g_si_state == SI_DISABLED)
            {
                err = RMF_SI_NOT_AVAILABLE;
            }
            else
            {
                /* Walk through the list of services to find a matching source Id. */
                walker = g_si_entry;
                while (walker)
                {
                    if (((walker->state == SIENTRY_MAPPED) || (walker->state
                            == SIENTRY_DEFINED_MAPPED)) && (walker->source_id
                            == sourceId))
                    {
                        *service_handle = (rmf_SiServiceHandle) walker;
                        if(SIENTRY_DEFINED_MAPPED == walker->state)
                        {
                            break;
                        }
                        else
                        {
                            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI", "undefined match for source-id %d\n", walker->source_id);
                            /* This entry is mapped, but not defined. Save the handle in case we don't find any defined-mapped entries further down 
                             * the list, but in case a defined-mapped entry exists, that one's a better candidate for this. So keep looking. */
                        }
#if 0
                        ts_entry
                                = (rmf_SiTransportStreamEntry *) walker->ts_handle;
                        if ((ts_entry != NULL) && (ts_entry->modulation_mode
                                != RMF_MODULATION_QAM_NTSC))
                        {
                            break;
                        }
#else
#endif
                    }
                    walker = walker->next;
                }

                /* Matching sourceId is not found, return error */
                if (RMF_SI_INVALID_HANDLE == *service_handle)
                {
                    err = RMF_SI_NOT_FOUND;
                }
                else
                {
                    // Increment the ref count
                    ((rmf_SiTableEntry *) *service_handle)->ref_count++;
                }
            }
        }
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                "<GetServiceHandleBySourceId> %d / 0x%8.8x / %d\n",
                sourceId, *service_handle, err);
    }

    return err;
}

/**
 * @brief Acquire a service handle from the given virtual channel number.
 * To find the matching sourceId for the service handle, need to walk through the list of services in SI table.
 *
 * @param[in] vcn Virtual channel number to underlying  physical and sub-channels that value uniquely identifies a given service.
 * @param[out] service_handle Output parameter to populaie the service handle.
 *
 * @retval RMF_SI_SUCCESS Returned when successfully acquired the service handle.
 * @retval RMF_SI_INVALID_PARAMETER Returned when service handle is NULL.
 * @retval RMF_SI_NOT_AVAILABLE_YET Returned when SI info is not availablr yet.
 * @retval RMF_SI_NOT_AVAILABLE Returned when SI info is not available and SI state is disabled.
 * @retval RMF_SI_NOT_FOUND Returned when matching sourceId is not found.
 */
rmf_Error rmf_OobSiCache::GetServiceHandleByVCN( uint16_t vcn,
			rmf_SiServiceHandle *service_handle )
{
    rmf_Error err = RMF_SI_SUCCESS;
    rmf_SiTableEntry *walker = NULL;
    //rmf_SiTransportStreamEntry *ts_entry = NULL;

    /* Parameter check */
    if (service_handle == NULL)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI",
                "<GetServiceHandleByVCN> null pointer to service_handle!\n");
        err = RMF_SI_INVALID_PARAMETER;
    }
    else
    {
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                "<GetServiceHandleByVCN> 0x%x\n", vcn);

        /* Clear out the value */
        *service_handle = RMF_SI_INVALID_HANDLE;

        // If SI DB is in either of these states return SI_NOT_AVAILABLE_YET
        // SI_ACQUIRING state is not useful for callers
        if ((g_si_state == SI_ACQUIRING)
                || (g_si_state == SI_NOT_AVAILABLE_YET))
        {
            err = RMF_SI_NOT_AVAILABLE_YET;
        }
        else
        {
            if (g_si_state == SI_DISABLED)
            {
                err = RMF_SI_NOT_AVAILABLE;
            }
            else
            {
                /* Walk through the list of services to find a matching source Id. */
                walker = g_si_entry;
                while (walker)
                {
                    if (((walker->state == SIENTRY_MAPPED) || (walker->state
                            == SIENTRY_DEFINED_MAPPED)) && (walker->virtual_channel_number
                            == vcn))
                    {
                        *service_handle = (rmf_SiServiceHandle) walker;
#if 0
                        ts_entry
                                = (rmf_SiTransportStreamEntry *) walker->ts_handle;
                        if ((ts_entry != NULL) && (ts_entry->modulation_mode
                                != RMF_MODULATION_QAM_NTSC))
                        {
                            break;
                        }
#else
                        break;
#endif
                    }
                    walker = walker->next;
                }

                /* Matching sourceId is not found, return error */
                if (RMF_SI_INVALID_HANDLE == *service_handle)
                {
                    err = RMF_SI_NOT_FOUND;
                }
                else
                {
                    // Increment the ref count
                    ((rmf_SiTableEntry *) *service_handle)->ref_count++;
                }
            }
        }
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                "<GetServiceHandleByVCN> %d / 0x%8.8x / %d\n",
                vcn, *service_handle, err);
    }

    return err;
}

#if 0
/**
 * Acquire a service handle given app Id (for DSG only)
 *
 * <i>GetServiceHandleByAppId()</i>
 *
 *
 * @param appId is a value that uniquely identifies a DSG tunnel.
 * @param service_handle output param to populate the service handle
 *
 * @return RMF_SI_SUCCESS if successfully acquired the handle, else
 *          return appropriate rmf_Error
 *
 */
rmf_Error rmf_OobSiCache::GetServiceHandleByAppId(uint32_t appId,
        rmf_SiServiceHandle *service_handle)
{
    rmf_Error err = RMF_SI_SUCCESS;
    rmf_SiTableEntry *walker = NULL;
    int found = 0;

    /* Parameter check */
    if (service_handle == NULL)
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<GetServiceHandleByAppId> 0x%x\n", appId);

    /* Clear out the value */
    *service_handle = RMF_SI_INVALID_HANDLE;

    // If SI DB is in either of these states return SI_NOT_AVAILABLE_YET
    // SI_ACQUIRING state is not useful for callers
    if ((g_si_state == SI_ACQUIRING) || (g_si_state == SI_NOT_AVAILABLE_YET))
        return RMF_SI_NOT_AVAILABLE_YET;
    else if (g_si_state == SI_DISABLED)
        return RMF_SI_NOT_AVAILABLE;

    /* Walk through the list of services to find a matching source Id. */
    walker = g_si_entry;
    while (walker)
    {
        if (walker->isAppType && walker->app_id == appId)
        {
            *service_handle = (rmf_SiServiceHandle) walker;
            walker->ref_count++;
            found = 1;
            break;
        }
        walker = walker->next;
    }

    /* Matching sourceId is not found, return error */
    if (!found)
    {
        err = RMF_SI_NOT_FOUND;
    }

    return err;
}
#endif

#if 0
/**
 * Acquire a service handle given session (for HN Stream session
 * only)
 *
 * <i>GetServiceHandleByHNSession()</i>
 *
 *
 * @param session is a value that uniquely identifies a HN
 *                streaming session.
 * @param service_handle output param to populate the service
 *                       handle
 *
 * @return RMF_SI_SUCCESS if successfully acquired the handle, else
 *          return appropriate rmf_Error
 *
 */
rmf_Error rmf_OobSiCache::si_getServiceHandleByHNSession(mpe_HnStreamSession session,
        rmf_SiServiceHandle *service_handle)
{
    rmf_Error err = RMF_SI_SUCCESS;
    rmf_SiTableEntry *walker = NULL;
    int found = 0;

    /* Parameter check */
    if (service_handle == NULL)
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<GetServiceHandleByHNSession 0x%x\n", session);

    /* Clear out the value */
    *service_handle = RMF_SI_INVALID_HANDLE;

    // If SI DB is in either of these states return SI_NOT_AVAILABLE_YET
    // SI_ACQUIRING state is not useful for callers
    if ((g_si_state == SI_ACQUIRING) || (g_si_state == SI_NOT_AVAILABLE_YET))
        return RMF_SI_NOT_AVAILABLE_YET;
    else if (g_si_state == SI_DISABLED)
        return RMF_SI_NOT_AVAILABLE;

    /* Walk through the list of services to find a matching session Id. */
    walker = g_si_entry;
    while (walker)
    {
        if (walker->hn_stream_session == session)
        {
            *service_handle = (rmf_SiServiceHandle) walker;
            walker->ref_count++;
            found = 1;
            break;
        }
        walker = walker->next;
    }

    /* Matching sourceId is not found, return error */
    if (!found)
    {
        err = RMF_SI_NOT_FOUND;
    }

    return err;
}
#endif

#if 0
/**
 * Acquire a service handle given frequency, program_number
 *
 * <i>GetServiceHandleByFrequencyModulationProgramNumber()</i>
 *
 *
 * @param freq is the carrier frequency that a virtual channel is carried on.
 * @param prog_num is a value that identifies a program with in a service.
 *        (frequency/program_number pair CANNOT uniquely identify a service,
 *          since serveral services can point to the same program.)
 * @param service_handle output param to populate the handle
 *
 * @return RMF_SI_SUCCESS if successfully acquired the handle, else
 *          return appropriate rmf_Error
 *
 */
rmf_Error rmf_OobSiCache::GetServiceHandleByFrequencyModulationProgramNumber(
        uint32_t freq, uint32_t mode, uint32_t prog_num,
        rmf_SiServiceHandle *service_handle)
{
    rmf_Error err = RMF_SI_SUCCESS;
    rmf_SiTransportStreamEntry *walker = NULL;
    int found = 0;

    RDK_LOG(
            RDK_LOG_TRACE1,
            "LOG.RDK.SI",
            "<GetServiceHandleByFrequencyModulationProgramNumber> %d:%d:0x%x\n",
            freq, mode, prog_num);
    /* Parameter check */
    if (service_handle == NULL)
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    /* Clear out the value */
    *service_handle = RMF_SI_INVALID_HANDLE;

    /* First, check if the list is empty */
    if (g_si_entry == NULL)
    {
        if ((g_si_state == SI_ACQUIRING)
                || (g_si_state == SI_NOT_AVAILABLE_YET))
            return RMF_SI_NOT_AVAILABLE_YET;
        else if (g_si_state == SI_DISABLED)
            return RMF_SI_NOT_AVAILABLE;
    }

    if (freq == RMF_SI_OOB_FREQUENCY)
    {
        walker = g_si_oob_ts_entry;
    }
    else if (freq == RMF_SI_HN_FREQUENCY)
    {
        walker = g_si_HN_ts_entry;
    }
    else if (freq == RMF_SI_DSG_FREQUENCY)
    {
        walker = g_si_dsg_ts_entry;
    }
    else
    {
        // look up the transport stream based on freq and modulation mode
        walker = (rmf_SiTransportStreamEntry *) get_transport_stream_handle(
                freq, mode);
    }

    if (walker != NULL)
    {
        ListSI svc_list = NULL;
        RDK_LOG(
                RDK_LOG_INFO,
                "LOG.RDK.SI",
                "<GetServiceHandleByFrequencyModulationProgramNumber> ts_handle:0x%p\n",
                walker);
        rmf_osal_mutexAcquire(walker->list_lock);
        RDK_LOG(
                RDK_LOG_INFO,
                "LOG.RDK.SI",
                "<GetServiceHandleByFrequencyModulationProgramNumber> program count:%d\n",
                llist_cnt(walker->programs));
        if(walker == g_si_HN_ts_entry)
        {
            svc_list = walker->services;
        }
        if (llist_cnt(walker->programs) != 0)
        {
            // IB or OOB digital services
            rmf_SiProgramInfo *prog = NULL;
            LINK *lp = llist_first(walker->programs);
            //RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<GetServiceHandleByFrequencyModulationProgramNumber> llist_cnt(walker->programs):%d\n", llist_cnt(walker->programs));
            while (lp)
            {
                prog = (rmf_SiProgramInfo *) llist_getdata(lp);
                if (prog && prog->program_number == prog_num)
                {
                    break;
                }
                lp = llist_after(lp);
            }
            if (lp && prog)
                svc_list = prog->services;
        }
        else
        { // Analog services
            svc_list = walker->services;
        }

        if (svc_list != NULL)
        {
            LINK *lp1;
            lp1 = llist_first(svc_list);
            if (lp1 != NULL)
            {
                *service_handle = (rmf_SiServiceHandle) llist_getdata(lp1);
                found = true;
            }
        }
        rmf_osal_mutexRelease(walker->list_lock);
        // break;
    }

    /* Matching freq, prog_num are not found, return error */
    if (!found)
    {
        err = RMF_SI_NOT_FOUND;
    }

    RDK_LOG(
            RDK_LOG_TRACE1,
            "LOG.RDK.SI",
            "<GetServiceHandleByFrequencyModulationProgramNumber> done, returning ret: %d service_handle: 0x%x\n",
            err, *service_handle);

    return err;
}
#endif

#if 0
/**
 * Create a DSG service entry given source ID and service name
 *
 * <i>mpe_siCreateDSGServiceHandle()</i>
 *
 *
 * @param appID is the applcaiton Id for the DSG tunnel.
 * @param service_name is a unique name assigned to the tunnel
 * @param service_handle output param to populate the handle
 *
 * @return RMF_SI_SUCCESS if successfully created the handle, else
 *          return appropriate rmf_Error
 *
 */
rmf_Error rmf_OobSiCache::CreateDSGServiceHandle(uint32_t appID, uint32_t prog_num,
        char* service_name, char* language, rmf_SiServiceHandle *service_handle)
{
    rmf_SiTableEntry *walker = NULL;
    rmf_SiTableEntry *new_si_entry = NULL;
    rmf_SiTransportStreamEntry *ts = NULL;
    int len = 0;
    int found = 0;
    int inTheList = 0;
    rmf_SiProgramInfo *pgm = NULL;
    rmf_Error tRetVal = RMF_SI_SUCCESS;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<mpe_siCreateDSGServiceHandle> appID= %d\n", appID);

    // Parameter check
    if (service_handle == NULL)
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    // Clear out the value
    *service_handle = RMF_SI_INVALID_HANDLE;

    // Program number for application tunnels?
    ts = (rmf_SiTransportStreamEntry *) get_dsg_transport_stream_handle();

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<mpe_siCreateDSGServiceHandle> transport stream entry...0x%p\n",
            ts);

    rmf_osal_mutexAcquire(ts->list_lock);
    {
        LINK *lp = llist_first(ts->programs);
        while (lp && !found)
        {
            pgm = (rmf_SiProgramInfo *) llist_getdata(lp);
            if (pgm)
            {
                LINK *lp1 = llist_first(pgm->services);
                while (lp1)
                {
                    walker = (rmf_SiTableEntry *) llist_getdata(lp1);
                    if (walker != NULL && walker->app_id == appID)
                    {
                        new_si_entry = walker;
                        inTheList = true;
                        found = true;
                        break;
                    }
                    lp1 = llist_after(lp1);
                }
            }
            lp = llist_after(lp);
        }

        if (lp == NULL)
        {
            // Create a place holder to store program info
            pgm = create_new_program_info(ts, prog_num);
            RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                    "<mpe_siCreateDSGServiceHandle> program Entry...0x%p\n",
                    pgm);
            if (pgm == NULL)
            {
                rmf_osal_mutexRelease(ts->list_lock);
                return RMF_SI_OUT_OF_MEM;
            }
            // newly created pgm is attached to ts by create_new_program_info.
        }
    }

    if (new_si_entry == NULL)
    {
        tRetVal = rmf_osal_memAllocP(RMF_OSAL_MEM_SI_CACHE_OOB, sizeof(rmf_SiTableEntry),
                (void **) &(new_si_entry));
        if ((RMF_SI_SUCCESS != tRetVal) || (new_si_entry == NULL))
        {
            RDK_LOG(
                    RDK_LOG_WARN,
                    "LOG.RDK.SI",
                    "<mpe_siCreateDSGServiceHandle> Mem allocation failed, returning RMF_SI_OUT_OF_MEM...\n");
            rmf_osal_mutexRelease(ts->list_lock);
            return RMF_SI_OUT_OF_MEM;
        }

        RDK_LOG(
                RDK_LOG_TRACE1,
                "LOG.RDK.SI",
                "<mpe_siCreateDSGServiceHandle> creating new SI Entry...0x%p\n",
                new_si_entry);

        /* Initialize all the fields (some fields are set to default values as defined in spec) */
        init_si_entry(new_si_entry);
        new_si_entry->isAppType = TRUE;
        new_si_entry->app_id = appID;
        found = true;

        // Link into other structs.
        if (pgm != NULL)
        {
            LINK *lp = llist_mklink((void *) new_si_entry);
            RDK_LOG(RDK_LOG_TRACE7, "LOG.RDK.SI",
                    "Add service %p to program %p...\n", new_si_entry, pgm);
            llist_append(pgm->services, lp);
            pgm->service_count++;
        }
        {
            LINK *lp = llist_mklink((void *) new_si_entry);
            llist_append(ts->services, lp);
            ts->service_count++;
            RDK_LOG(RDK_LOG_TRACE7, "LOG.RDK.SI",
                    "Add service %p to transport %p...\n", new_si_entry, ts);
        }
    }
    else
    {
        found = true;
    }

    if (service_name != NULL)
        len = strlen(service_name);

    if (len > 0)
    {
        //
        // Code modified to look up source name entry as this information moved to another table.
        rmf_siSourceNameEntry *source_name_entry = NULL;

        //
        // Look up, but allow a create, the source name by appID
        GetSourceNameEntry(appID, TRUE, &source_name_entry, TRUE);
        if (source_name_entry != NULL)
            {
          SetSourceName(new_si_entry->source_name_entry, service_name, language, FALSE);
          SetSourceLongName(new_si_entry->source_name_entry, service_name, language);
        }
    }
    rmf_osal_mutexRelease(ts->list_lock);

    if (found)
    {
        // ServiceEntry pointers
        new_si_entry->ts_handle = (rmf_SiTransportStreamHandle) ts;
        new_si_entry->program = pgm;
        new_si_entry->source_id = SOURCEID_UNKNOWN;

        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                "<mpe_siCreateDSGServiceHandle> Inserting service entry in the list \n");

        if (g_SITPConditionSet == TRUE)
        {
            (ts->service_count)++;
            (ts->visible_service_count)++;
        }
        new_si_entry->ref_count++;

        *service_handle = (rmf_SiServiceHandle) new_si_entry;

        /* append new entry to the dynamic list */
        if(!inTheList)
        {
            if (g_si_entry == NULL)
            {
                g_si_entry = new_si_entry;
            }
            else
            {
                walker = g_si_entry;
                while (walker)
                {
                    if (walker->next == NULL)
                    {
                        walker->next = new_si_entry;
                        break;
                    }
                    walker = walker->next;
                }
            }
        }
    }

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<mpe_siCreateDSGServiceHandle> done, returning RMF_SI_SUCCESS\n");
    return RMF_SI_SUCCESS;
}
#endif

#if 0
rmf_Error rmf_OobSiCache::IsServiceHandleAppType(rmf_SiServiceHandle service_handle,
        rmf_osal_Bool *isAppType)
{
    rmf_SiTableEntry *si_entry = (rmf_SiTableEntry *) service_handle;

    if ((service_handle == RMF_SI_INVALID_HANDLE) || (isAppType == NULL))
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    RDK_LOG(
            RDK_LOG_TRACE1,
            "LOG.RDK.SI",
            "<mpe_siIsServiceHandleAppType> service_handle:0x%x isAppType:%d\n",
            service_handle, si_entry->isAppType);

    *isAppType = si_entry->isAppType;

    return RMF_SI_SUCCESS;
}
#endif

#if 0
/**
 * Create a HN streaming service entry given source ID and
 * service name
 *
 * <i>mpe_siCreateHNstreamServiceHandle()</i>
 *
 *
 * @param sessionID is the hn streaming session Id for the DSG
 * @param service_name is a unique name assigned to the tunnel
 * @param service_handle output param to populate the handle
 *
 * @return RMF_SI_SUCCESS if successfully created the handle, else
 *          return appropriate rmf_Error
 *
 */
rmf_Error rmf_OobSiCache::CreateHNstreamServiceHandle(mpe_HnStreamSession session, uint32_t prog_num,
        char* service_name, char* language, rmf_SiServiceHandle *service_handle)
{
    rmf_SiTableEntry *walker = NULL;
    rmf_SiTableEntry *new_si_entry = NULL;
    rmf_SiTransportStreamEntry *ts = NULL;
    int found = 0;
    int inTheList = 0;
    rmf_SiProgramInfo *pgm = NULL;
    rmf_Error tRetVal = RMF_SI_SUCCESS;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<mpe_siCreateHNstreamServiceHandle> sessionID= %x\n", session);

    // Parameter check
    if (service_handle == NULL)
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    // Clear out the value
    *service_handle = RMF_SI_INVALID_HANDLE;

    // Program number for application tunnels?
    ts = (rmf_SiTransportStreamEntry *) get_hn_transport_stream_handle();

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<mpe_siCreateHNstreamServiceHandle> transport stream entry...0x%x\n",
            ts);

    rmf_osal_mutexAcquire(ts->list_lock);
    {
        LINK *lp = llist_first(ts->programs);
        while (lp && !found)
        {
            pgm = (rmf_SiProgramInfo *) llist_getdata(lp);
            if (pgm)
            {
                LINK *lp1 = llist_first(pgm->services);
                while (lp1)
                {
                    walker = (rmf_SiTableEntry *) llist_getdata(lp1);
                    if (walker != NULL && walker->hn_stream_session == session)
                    {
                        new_si_entry = walker;
                        inTheList = true;
                        found = true;
                        break;
                    }
                    lp1 = llist_after(lp1);
                }
            }
            lp = llist_after(lp);
        }

        if (lp == NULL)
        {
            // Create a place holder to store program info
            pgm = create_new_program_info(ts, prog_num);
            RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                    "<mpe_siCreateHNstreamServiceHandle> program Entry...0x%x\n",
                    pgm);
            if (pgm == NULL)
            {
                rmf_osal_mutexRelease(ts->list_lock);
                return RMF_SI_OUT_OF_MEM;
            }
            // newly created pgm is attached to ts by create_new_program_info.
        }
    }

    if (new_si_entry == NULL)
    {
        tRetVal = rmf_osal_memAllocP(RMF_OSAL_MEM_SI_CACHE_OOB, sizeof(rmf_SiTableEntry),
                (void **) &(new_si_entry));
        if ((RMF_SI_SUCCESS != tRetVal) || (new_si_entry == NULL))
        {
            RDK_LOG(
                    RDK_LOG_WARN,
                    "LOG.RDK.SI",
                    "<mpe_siCreateHNstreamServiceHandle> Mem allocation failed, returning RMF_SI_OUT_OF_MEM...\n");
            rmf_osal_mutexRelease(ts->list_lock);
            return RMF_SI_OUT_OF_MEM;
        }

        RDK_LOG(
                RDK_LOG_TRACE1,
                "LOG.RDK.SI",
                "<mpe_siCreateHNstreamServiceHandle> creating new SI Entry...0x%x\n",
                new_si_entry);

        /* Initialize all the fields (some fields are set to default values as defined in spec) */
        init_si_entry(new_si_entry);
        new_si_entry->hn_stream_session = session;
        found = true;

        // Link into other structs.
        if (pgm != NULL)
        {
            LINK *lp = llist_mklink((void *) new_si_entry);
            RDK_LOG(RDK_LOG_TRACE7, "LOG.RDK.SI",
                    "Add service %p to program %p...\n", new_si_entry, pgm);
            llist_append(pgm->services, lp);
            pgm->service_count++;
        }
        {
            LINK *lp = llist_mklink((void *) new_si_entry);
            llist_append(ts->services, lp);
            ts->service_count++;
            RDK_LOG(RDK_LOG_TRACE7, "LOG.RDK.SI",
                    "Add service %p to transport %p...\n", new_si_entry, ts);
        }
    }
    else
    {
        found = true;
    }


    rmf_osal_mutexRelease(ts->list_lock);

    if (found)
    {
        // ServiceEntry pointers
        new_si_entry->ts_handle = (rmf_SiTransportStreamHandle) ts;
        new_si_entry->program = pgm;
        new_si_entry->source_id = SOURCEID_UNKNOWN;

        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                "<mpe_siCreateHNstreamServiceHandle> Inserting service entry in the list \n");

        if (g_SITPConditionSet == TRUE)
        {
            (ts->service_count)++;
            (ts->visible_service_count)++;
        }
        new_si_entry->ref_count++;

        *service_handle = (rmf_SiServiceHandle) new_si_entry;

        /* append new entry to the dynamic list */
        if(!inTheList)
        {
            if (g_si_entry == NULL)
            {
                g_si_entry = new_si_entry;
            }
            else
            {
                walker = g_si_entry;
                while (walker)
                {
                    if (walker->next == NULL)
                    {
                        walker->next = new_si_entry;
                        break;
                    }
                    walker = walker->next;
                }
            }
        }
    }

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<mpe_siCreateHNstreamServiceHandle> done, returning RMF_SI_SUCCESS\n");
    return RMF_SI_SUCCESS;
}


/**
 * Create a dynamic service entry given frequency, program_number, qam mode
 *
 * <i>mpe_siCreateDynamicServiceHandle()</i>
 *
 *
 * @param freq is the carrier frequency that a virtual channel is carried on.
 * @param prog_num is a value that identifies a program with in a service.
 * @param modFormat is the modulation format
 * @param service_handle output param to populate the handle
 *
 * @return RMF_SI_SUCCESS if successfully created the handle, else
 *          return appropriate rmf_Error
 *
 */
rmf_Error rmf_OobSiCache::CreateDynamicServiceHandle(uint32_t freq, uint32_t prog_num,
        rmf_ModulationMode modFormat, rmf_SiServiceHandle *service_handle)
{
    rmf_SiTableEntry *walker = NULL;
    rmf_SiTableEntry *new_si_entry = NULL;
    rmf_Error err = RMF_SI_SUCCESS;
    rmf_osal_Bool found = false;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<mpe_siCreateDynamicServiceHandle> %d:0x%x:%d\n", freq, prog_num,
            modFormat);

    /* Parameter check */
    if (service_handle == NULL)
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    /* Clear out the value */
    *service_handle = RMF_SI_INVALID_HANDLE;

    if (freq == RMF_SI_OOB_FREQUENCY)
    {
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                "<mpe_siCreateDynamicServiceHandle> OOB frequency\n");
    }
    else if (freq == RMF_SI_HN_FREQUENCY)
    {
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                "<mpe_siCreateDynamicServiceHandle> HN frequency\n");
    }
    else if (freq == RMF_SI_DSG_FREQUENCY)
    {
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                "<mpe_siCreateDynamicServiceHandle> DSG frequency\n");
    }
    else if ((g_frequencyCheckEnable == TRUE) && (g_SITPConditionSet == TRUE))
    {
        /* freq check */
        /* If for some reason NIT-CDS table is not signaled we will re-open the
         * database after the set timeout but frequency check cannot be enforced
         * in that case. Can still allow dynamic service entry creation and tuning by fpq. */
        if ( ((g_minFrequency != 0) && (freq < g_minFrequency)) || // check for valid frequency
               ((g_maxFrequency != 0) && (freq > g_maxFrequency)) )
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI",
                    "<mpe_siCreateDynamicServiceHandle> invalid frequency\n");

            return RMF_SI_INVALID_PARAMETER;
        }
    }

    err = GetServiceHandleByFrequencyModulationProgramNumber(freq,
            modFormat, prog_num, service_handle);
    if (err == RMF_SI_NOT_FOUND || err == RMF_SI_NOT_AVAILABLE || err
            == RMF_SI_NOT_AVAILABLE_YET)
    {
        rmf_SiTransportStreamEntry *ts = NULL;
        rmf_SiProgramInfo *pgm = NULL;
        rmf_Error tRetVal = RMF_SI_SUCCESS;

        // create_transport_stream_handle() returns existing, if possible.
        if ((ts
                = (rmf_SiTransportStreamEntry *) create_transport_stream_handle(
                        freq, modFormat)) == NULL)
        {
            rmf_osal_memFreeP(RMF_OSAL_MEM_SI_CACHE_OOB, new_si_entry);
            return RMF_SI_OUT_OF_MEM;
        }
        RDK_LOG(
                RDK_LOG_TRACE1,
                "LOG.RDK.SI",
                "<mpe_siCreateDynamicServiceHandle> transport stream entry...0x%p\n",
                ts);

        rmf_osal_mutexAcquire(ts->list_lock);
        if (modFormat != RMF_MODULATION_QAM_NTSC)
        {
            LINK *lp = llist_first(ts->programs);
            while (lp)
            {
                pgm = (rmf_SiProgramInfo *) llist_getdata(lp);
                if (pgm && pgm->program_number == prog_num)
                {
                    break;
                }
                lp = llist_after(lp);
            }

            if (lp == NULL)
            {
                pgm = create_new_program_info(ts, prog_num);
                if (pgm == NULL)
                {
                    rmf_osal_mutexRelease(ts->list_lock);
                    return RMF_SI_OUT_OF_MEM;
                }
                // newly created pgm is attached to ts by create_new_program_info.
            }

            if (pgm)
            {
                LINK *lp1 = llist_first(pgm->services);
                if (lp1 != NULL)
                    new_si_entry = (rmf_SiTableEntry *) llist_getdata(lp1);
            }
        }
        else
        {
            // Not sure about the need for this block?
            if (new_si_entry == NULL)
            {
                LINK *lp2 = llist_first(ts->services);
                if (lp2 != NULL)
                {
                    new_si_entry = (rmf_SiTableEntry *) llist_getdata(lp2);
                    RDK_LOG(
                            RDK_LOG_TRACE1,
                            "LOG.RDK.SI",
                            "<mpe_siCreateDynamicServiceHandle> new_si_entry...0x%p\n",
                            new_si_entry);
                }
            }
        }

        if (new_si_entry == NULL)
        {
            tRetVal = rmf_osal_memAllocP(RMF_OSAL_MEM_SI_CACHE_OOB, sizeof(rmf_SiTableEntry),
                    (void **) &(new_si_entry));
            if ((RMF_SI_SUCCESS != tRetVal) || (new_si_entry == NULL))
            {
                RDK_LOG(
                        RDK_LOG_WARN,
                        "LOG.RDK.SI",
                        "<mpe_siCreateDynamicServiceHandle> Mem allocation failed, returning RMF_SI_OUT_OF_MEM...\n");
                rmf_osal_mutexRelease(ts->list_lock);
                return RMF_SI_OUT_OF_MEM;
            }
            /* Initialize all the fields (some fields are set to default values as defined in spec) */
            init_si_entry(new_si_entry);
            // Link into other structs.
            if (pgm != NULL)
            {
                LINK *lp = llist_mklink((void *) new_si_entry);
                RDK_LOG(RDK_LOG_TRACE7, "LOG.RDK.SI",
                        "Add service %p to program %p...\n", new_si_entry, pgm);
                llist_append(pgm->services, lp);
                pgm->service_count++;
            }
            {
                LINK *lp = llist_mklink((void *) new_si_entry);
                llist_append(ts->services, lp);
                ts->service_count++;
                RDK_LOG(RDK_LOG_TRACE7, "LOG.RDK.SI",
                        "Add service %p to transport %p...\n", new_si_entry, ts);
            }
            found = true;
        }
        else
        {
            //RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<mpe_siCreateDynamicServiceHandle> re-using SI Entry...0x%x\n", new_si_entry);
            found = true;
        }
        rmf_osal_mutexRelease(ts->list_lock);

        if (found)
        {
            //RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<mpe_siCreateDynamicServiceHandle> creating new SI Entry...0x%x\n", new_si_entry);

            // ServiceEntry pointers
            new_si_entry->ts_handle = (rmf_SiTransportStreamHandle) ts;
            new_si_entry->program = pgm; // could be null.
            /* For dynamic services (ex: vod) since sourceid is unknown, just default it to SOURCEID_UNKNOWN/-1.*/
            new_si_entry->source_id = SOURCEID_UNKNOWN;

            if (g_SITPConditionSet == TRUE)
            {
                (ts->service_count)++;
                /* Increment the visible service count also
                 (channel type is not known for dynamic services, they are
                 not signaled in OOB VCT) */
                (ts->visible_service_count)++;
            }
            new_si_entry->ref_count++;

            /* append new entry to the dynamic list */
            if (g_si_entry == NULL)
            {
                g_si_entry = new_si_entry;
            }
            else
            {
                walker = g_si_entry;
                while (walker)
                {
                    if (walker->next == NULL)
                    {
                        RDK_LOG(
                                RDK_LOG_TRACE1,
                                "LOG.RDK.SI",
                                "<mpe_siCreateDynamicServiceHandle> set %p->next to %p\n",
                                walker, new_si_entry);
                        walker->next = new_si_entry;
                        break;
                    }
                    walker = walker->next;
                }
            }
            /* Increment the number of services */
            g_numberOfServices++;
        }
    }
    *service_handle = (rmf_SiServiceHandle) new_si_entry;
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<mpe_siCreateDynamicServiceHandle> done, returning RMF_SI_SUCCESS\n");

    return RMF_SI_SUCCESS;
}
#endif
/**
 * Acquire a service handle given service name
 *
 * <i>GetServiceHandleByServiceName()</i>
 *
 *
 * @param service_name is a unique name assigned to a service.
 * @param service_handle output param to populate the handle
 *
 * @return RMF_SI_SUCCESS if successfully acquired the handle, else
 *          return appropriate rmf_Error
 *
 */
rmf_Error rmf_OobSiCache::GetServiceHandleByServiceName(char* service_name,
        rmf_SiServiceHandle *service_handle)
{
    rmf_SiTableEntry *walker = NULL;
    rmf_siSourceNameEntry *sn_walker = g_si_sourcename_entry;

    /* Parameter check */
    if ((service_handle == NULL) || (service_name == NULL))
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    /* Clear out the value */
    *service_handle = RMF_SI_INVALID_HANDLE;

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI",
            "<GetServiceHandleByServiceName> %s\n", service_name);

    // If SI DB is in either of these states return SI_NOT_AVAILABLE_YET
    // SI_ACQUIRING state is not useful for callers
    if ((g_si_state == SI_ACQUIRING) || (g_si_state == SI_NOT_AVAILABLE_YET))
        return RMF_SI_NOT_AVAILABLE_YET;
    else if (g_si_state == SI_DISABLED)
        return RMF_SI_NOT_AVAILABLE;

    /*  Note: Service name (source name) comes from NTT_SNS.
     This table contains a sourceId-to-sourceName mapping.
     Hence acquiring a handle based on service name will likely
     only be found in the main tree where sourceIds are stored.
     */
    /* Walk through the list of services to find a matching service name (in any language). */
    while (sn_walker)
    {
      rmf_SiLangSpecificStringList *string_list = sn_walker->source_names;
        while (string_list != NULL)
        {
        if (strcmp(string_list->string, service_name) == 0)
        {
          rmf_SiServiceHandle handle = RMF_SI_INVALID_HANDLE;
          if ( GetServiceHandleBySourceId(sn_walker->id, &handle) != RMF_SI_SUCCESS )
          {
              /* No need to handle error since the invalid handle check done below, so just adding debug */
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "GetServiceHandleBySourceId failed\n");
          }
#if 0
          if (handle == RMF_SI_INVALID_HANDLE)
            GetServiceHandleByAppId(sn_walker->id, &handle);
#endif
          if (handle != RMF_SI_INVALID_HANDLE)
          {
            walker = (rmf_SiTableEntry *)handle;
            if (((walker->state == SIENTRY_MAPPED) || (walker->state == SIENTRY_DEFINED_MAPPED)))
            {
                walker->ref_count++;
              *service_handle = handle;
                return RMF_SI_SUCCESS;
            }
          }
        }
            string_list = string_list->next;
        }

      sn_walker = sn_walker->next;
    }

    /* Matching service name is not found, return error */
    return RMF_SI_NOT_FOUND;
}

/**
 * Acquire lock for reading
 *
 * <i>mpe_siLockForRead()</i>
 *
 * @return RMF_SI_SUCCESS if successfully acquired the lock, else
 *          return appropriate rmf_Error
 *
 */
rmf_Error rmf_OobSiCache::LockForRead()
{
    rmf_Error result = RMF_SUCCESS;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<mpe_siLockForRead> \n");

    /* Acquire global mutex */
    rmf_osal_mutexAcquire(g_global_si_mutex);

    /*
     For read only purposes acquire the lock if no module is
     currently reading. If some module is reading, just increment
     the counter.
     */
    if (g_cond_counter == 0)
    {
        rmf_osal_condGet(g_global_si_cond);
    }

    /* increment the counter in any case */
    g_cond_counter++;

    rmf_osal_mutexRelease(g_global_si_mutex);
    /* Release global mutex */

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<mpe_siLockForRead> done\n");

    return result;
}

/**
 * Release the read lock
 *
 * <i>mpe_siUnLock()</i>
 *
 * @return RMF_SI_SUCCESS if successfully released the lock, else
 *          return appropriate rmf_Error
 *
 */
rmf_Error rmf_OobSiCache::UnLock()
{
    rmf_Error result = RMF_SUCCESS;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<mpe_siUnLock> \n");

    /*  Multiple modules can be reading at the
     same time from any given Si handle. The read
     counter is incremented every time a read
     is requested (by mpe_siLockForRead).
     The same counter is decremented when handle is
     released. If the read counter reaches zero then
     the condition is also released.
     */

    /* Acquire global mutex */
    rmf_osal_mutexAcquire(g_global_si_mutex);

    if (g_cond_counter == 0)
    {
        result = RMF_SI_LOCKING_ERROR;
    }
    else
    {
        g_cond_counter--;

        if (g_cond_counter == 0)
        {
            RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                    "Lock count has hit zero; releasing g_global_si_cond.\n");
            rmf_osal_condSet(g_global_si_cond);
        }
    }

    /* Release global mutex */
    rmf_osal_mutexRelease(g_global_si_mutex);

    return result;
}

#if 0
rmf_Error rmf_OobSiCache::UnLock()
{
    rmf_Error result = RMF_SUCCESS;
    rmf_osal_Bool releaseFlag = FALSE;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<mpe_siUnLock> \n");

    /*  Multiple modules can be reading at the
     same time from any given Si handle. The read
     counter is incremented every time a read
     is requested (by mpe_siLockForRead).
     The same counter is decremented when handle is
     released. If the read counter reaches zero then
     the condition is also released.
     */

    /* Acquire global mutex */
    rmf_osal_mutexAcquire(g_global_si_mutex);

    if (g_cond_counter > 0)
    {
        g_cond_counter--;
    }

    if (g_cond_counter == 0)
    {
        releaseFlag = TRUE;
    }

    /* Release global mutex */
    rmf_osal_mutexRelease(g_global_si_mutex);

    if (releaseFlag)
    {
        rmf_osal_condSet(g_global_si_cond);
        releaseFlag = FALSE;
    }

    return RMF_SI_SUCCESS;
}
#endif

/*
 This method is called by SITP when all the OOB tables
 have been read atleast once. This condition once set, remains
 in that state. [ no longer true --mlb 12/21/2005
 see mpe_siUnsetGlobalState() ]
 */
void rmf_OobSiCache::SetGlobalState(rmf_SiGlobalState state)
{
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI", "mpe_siSetGlobalState() state: %d\n", state);
    
    //We need to make sure this doesn't break JAVA STT event handler
    if(state == SI_STT_ACQUIRED && g_siSTTReceived == FALSE)
    {
        g_siSTTReceived = TRUE; 
        TouchFile(RMF_STT_RECEIVED_TOUCH_FILE);
        //send_si_event(si_state_to_event(state),0,0);
	if(rmf_siGZCBfunc)
	{
		rmf_siGZCBfunc( RMF_OOBSI_GLI_STT_ACQUIRED, 0, 0 );
	}
        return;
    }

    /*
     signal the condition to set state
     (allow readers to read from SI DB)
     */
    if (state == SI_FULLY_ACQUIRED)
    {
	uint16_t ChannelMapId = VIRTUAL_CHANNEL_UNKNOWN;		
	uint16_t VCTId = VIRTUAL_CHANNEL_UNKNOWN;
	uint16_t dac_id = VIRTUAL_CHANNEL_UNKNOWN;
	rmf_osal_condSet(g_SITP_cond);
	g_SITPConditionSet = TRUE;
	TouchFile(RMF_SI_ACQUIRED_TOUCH_FILE);
	GetChannelMapId ( &ChannelMapId );
	GetVCTId( &VCTId  );
	GetDACId( &dac_id );
	if(rmf_siGZCBfunc)
	{
		rmf_siGZCBfunc( RMF_OOBSI_GLI_DAC_ID_CHANGED, dac_id, 0 );
		rmf_siGZCBfunc( RMF_OOBSI_GLI_SI_FULLY_ACQUIRED, ChannelMapId, VCTId );		
	}
    }
	else if(state == SI_ACQUIRING)
	{
		g_SITPConditionSet = FALSE;
	}
    g_si_state = state;

    //send_si_event(si_state_to_event(state), 0, 0);
}

rmf_Error rmf_OobSiCache::ValidateServiceHandle(rmf_SiServiceHandle service_handle)
{
    rmf_SiTableEntry *walker = NULL;
    rmf_SiTableEntry *target = (rmf_SiTableEntry *) service_handle;
    rmf_Error retVal = RMF_SI_NOT_FOUND;

    /* Parameter check */
    if (service_handle != RMF_SI_INVALID_HANDLE)
    {
        walker = g_si_entry;
        while (walker)
        {
            if (walker == target)
            {
                retVal = RMF_SI_SUCCESS;
                break;
            }
            walker = walker->next;
        }
    }
    if (retVal != RMF_SI_SUCCESS)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI",
                "mpe_siValidateServiceHandle: Handle %p did not validate!\n",
                (void *) service_handle);
    }
    return retVal;
}

/**
 * Retrieve number of components for the given service handle
 *
 * <i>GetNumberOfComponentsForServiceHandle()</i>
 *
 * @param service_handle input si handle
 * @param num_components is the output param to populate number of components
 *
 * @return RMF_SI_SUCCESS if successful, else
 *          return appropriate rmf_Error
 *
 */
rmf_Error rmf_OobSiCache::GetNumberOfComponentsForServiceHandle(
                rmf_SiServiceHandle service_handle, uint32_t *num_components)
{
        rmf_SiTableEntry *si_entry = (rmf_SiTableEntry*) service_handle;
        rmf_SiProgramInfo *pi = NULL;
        rmf_Error err = RMF_SI_SUCCESS;

        RDK_LOG(
                        RDK_LOG_TRACE1,
                        "LOG.RDK.SI",
                        "<GetNumberOfComponentsForServiceHandle> service_handle:0x%x\n",
                        service_handle);

        /* Parameter check */
        if ((service_handle == RMF_SI_INVALID_HANDLE) || (num_components == NULL))
        {
                RDK_LOG(
                                RDK_LOG_ERROR,
                                "LOG.RDK.SI",
                "<GetNumberOfComponentsForServiceHandle> bad service_handle:0x%x or num_components:0x%p\n",
                                service_handle, num_components);
                err = RMF_SI_INVALID_PARAMETER;
        }
    else
    {
                *num_components = 0;
                /* SI DB has not been populated yet */
                if (g_si_entry == NULL)
                {
			RDK_LOG(
                                RDK_LOG_ERROR,
                                "LOG.RDK.SI",
                "<GetNumberOfComponentsForServiceHandle> g_si_entry: 0x%x\n",
                                g_si_entry);
                        err = RMF_SI_NOT_AVAILABLE;
                }
                else
                {
                        pi = (rmf_SiProgramInfo*)si_entry->program;
			RDK_LOG(
                                RDK_LOG_TRACE1,
                                "LOG.RDK.SI",
                "<GetNumberOfComponentsForServiceHandle> pi: 0x%x\n", pi);
                        if (pi == NULL)
                        {
                                rmf_SiTransportStreamEntry *ts =
                                                (rmf_SiTransportStreamEntry *) si_entry->ts_handle;
				RDK_LOG(
        	                        RDK_LOG_TRACE1,
                	                "LOG.RDK.SI",
	                "<GetNumberOfComponentsForServiceHandle> ts: 0x%x\n", ts);

                                if (ts != NULL)
                                { // Since analog has no components, returning 0 makes sense.
                                        /*if (ts->modulation_mode != RMF_MODULATION_QAM_NTSC)
                                                err = RMF_SI_INVALID_PARAMETER;*/
                                }
                                else
                                {
                                        err = RMF_SI_NOT_AVAILABLE; // SPI service?
                                }
                        }
                        else
                        {
                                /* If PSI is in the process of being acquired return from here
                 until it is updated. */
                                if (pi->pmt_status == PMT_NOT_AVAILABLE)
                                {
					RDK_LOG(
                                        RDK_LOG_ERROR,
                                        "LOG.RDK.SI",
		                        "<GetNumberOfComponentsForServiceHandle>: Setting PMT status to NOT AVAILABLE\n");
		                        err = RMF_SI_NOT_AVAILABLE;
                                }
                                else if (pi->pmt_status == PMT_AVAILABLE_SHORTLY)
                                {
					RDK_LOG(
                                        RDK_LOG_ERROR,
                                        "LOG.RDK.SI",
		                        "<GetNumberOfComponentsForServiceHandle>: Setting PMT status to NOT AVAILABLE YET\n");
                                        err = RMF_SI_NOT_AVAILABLE_YET;
                                }
                                else
                                {
                                        /*
                     ** The num_components translates to the number of PMT elementary
                     ** streams at the native level, so set the passed in ptrs val to
                     ** the number of elementary streams.
                     */
                                        *num_components = pi->number_elem_streams;
                                }
                        }
        }
                RDK_LOG(
                                RDK_LOG_TRACE1,
                                "LOG.RDK.SI",
                                "<GetNumberOfComponentsForServiceHandle> returning number_elem_streams: %d (%d)\n",
                                *num_components, err); 
    }
    return err;
}

/**
 * @brief This function gets raw descriptor data for the service specified by the given service handle.
 *
 * @param[in] service_handle Service_handle represents a SCTE service/channel.
 * @param[out] descriptors A descriptor linked list will be returned in this parameter.
 *
 * @retval RMF_SI_SUCCESS Returned when successfully acquired the raw descriptors.
 * @retval RMF_SI_INVALID_PARAMETER Returned If service handle is invalid OR number of descriptors is NULL.
 * @retval RMF_SI_NOT_AVAILABLE Returned when program handle is NULL.
 */
rmf_Error rmf_OobSiCache::GetOuterDescriptorsForServiceHandle(
                rmf_SiServiceHandle service_handle, uint32_t* numDescriptors,
                rmf_SiMpeg2DescriptorList** descriptors)
{
        rmf_SiTableEntry *si_entry = (rmf_SiTableEntry *) service_handle;
        rmf_SiProgramInfo *pi = NULL;
        /* Parameter check */
        if (service_handle == RMF_SI_INVALID_HANDLE || numDescriptors == NULL)
        {
                return RMF_SI_INVALID_PARAMETER;
        }

        pi = (rmf_SiProgramInfo*)si_entry->program;

        if (pi == NULL)
        {
                return RMF_SI_NOT_AVAILABLE;
        }

        *numDescriptors = pi->number_outer_desc;
        *descriptors = pi->outer_descriptors;

        return RMF_SI_SUCCESS;
}

/**
 * Return all service component handles in the given service
 *
 * <i>GetAllComponentsForServiceHandle()</i>
 *
 * @param ts_handle input transport stream handle
 * @param comp_handle output param to populate component handles
 * @param num_components Input param: has size of comp_handle.
 * Output param: the actual number of component handles placed in comp_handle
 *
 * @return RMF_SI_SUCCESS if successful, else
 *          return appropriate rmf_Error
 *
 */
rmf_Error rmf_OobSiCache::GetAllComponentsForServiceHandle(
                rmf_SiServiceHandle service_handle,
                rmf_SiServiceComponentHandle comp_handle[], uint32_t* num_components)
{
        rmf_SiTableEntry *si_entry = (rmf_SiTableEntry*) service_handle;
        rmf_SiProgramInfo *pi = NULL;
        rmf_SiElementaryStreamList *list_ptr = NULL;
        rmf_Error err = RMF_SI_SUCCESS;
        uint32_t i = 0;

        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                        "<GetAllComponentsForServiceHandle> \n");

        /* Parameter check */
        if ((service_handle == RMF_SI_INVALID_HANDLE) || (comp_handle == NULL)
                        || (num_components == NULL) || (*num_components == 0))
        {
                err = RMF_SI_INVALID_PARAMETER;
        }
        else
        {
                /* SI DB has not been populated yet */
                if (g_si_entry == NULL)
                {
                        err = RMF_SI_NOT_AVAILABLE;
                }
                else
                {
                        *comp_handle = 0;
                        pi = (rmf_SiProgramInfo*)si_entry->program;
                        if (pi == NULL)
                        {
                                rmf_SiTransportStreamEntry *ts =
                                                (rmf_SiTransportStreamEntry *) si_entry->ts_handle;
                                if (ts != NULL)
                                {
                                        if (ts->modulation_mode != RMF_MODULATION_QAM_NTSC)
                                                err = RMF_SI_INVALID_PARAMETER;
                                        else
                                                err = RMF_SI_NOT_FOUND;
                                }
                                else
                                {
                                        err = RMF_SI_NOT_AVAILABLE; // SPI service?
                                }
                        }
                        else
                        {

                                list_ptr = pi->elementary_streams;

                                while (list_ptr)
                                {
                                        if (i >= *num_components)
                                        {
                                                RDK_LOG(
                                                                RDK_LOG_ERROR,
                                                                "LOG.RDK.SI",
                                "<GetAllComponentsForServiceHandle> array_service_handle (%d) (%p) too small for list\n",
                                                                *num_components, pi->elementary_streams);
                                                err = RMF_OSAL_ENOMEM;
                                                break;
                                        }

                                        comp_handle[i++] = (rmf_SiServiceComponentHandle) list_ptr;
                                        list_ptr->ref_count++;
                                        RDK_LOG(
                                                        RDK_LOG_TRACE1,
                                                        "LOG.RDK.SI",
                            "<GetAllComponentsForServiceHandle> comp_handle:0x%p, ref_count:%d \n",
                                                        list_ptr, list_ptr->ref_count);
                                        list_ptr = list_ptr->next;
                                }

                                *num_components = i;
                        }
                }
        }
        return err;
}

/**
 * Get raw descriptor data for a particular service component given a component handle
 *
 * <i>GetDescriptorsForServiceComponentHandle()</i>
 *
 * @param comp_handle The service component handle
 * @param descriptors A descriptor linked list will be returned
 * in this parameter
 *
 * @return RMF_SI_SUCCESS if successful, else
 *          return appropriate rmf_Error
 *
 */
rmf_Error rmf_OobSiCache::GetDescriptorsForServiceComponentHandle(
                rmf_SiServiceComponentHandle comp_handle, uint32_t* numDescriptors,
                rmf_SiMpeg2DescriptorList** descriptors)
{
        rmf_SiElementaryStreamList* si_esList;

        /* Parameter check */
        if (comp_handle == RMF_SI_INVALID_HANDLE || numDescriptors == NULL)
        {
                return RMF_SI_INVALID_PARAMETER;
        }

        si_esList = (rmf_SiElementaryStreamList*) comp_handle;

        *numDescriptors = si_esList->number_desc;
        *descriptors = si_esList->descriptors;

        return RMF_SI_SUCCESS;
}

/**
 * Get elementary stream type given service handle and component handle
 *
 * <i>GetStreamTypeForServiceComponentHandle()</i>
 *
 * @param comp_handle is the input component handle
 * @param stream_type is the output param to populate stream type
 *
 * @return RMF_SI_SUCCESS if successful, else
 *          return appropriate rmf_Error
 *
 */
rmf_Error rmf_OobSiCache::GetStreamTypeForServiceComponentHandle(
                rmf_SiServiceComponentHandle comp_handle,
                rmf_SiElemStreamType *stream_type)
{
        rmf_SiElementaryStreamList *elem_stream_list = NULL;

        /* Parameter check */
        if ((comp_handle == RMF_SI_INVALID_HANDLE) || (stream_type == NULL))
        {
                return RMF_SI_INVALID_PARAMETER;
        }

        *stream_type = (rmf_SiElemStreamType)0;
        elem_stream_list = (rmf_SiElementaryStreamList *) comp_handle;

        *stream_type = elem_stream_list->elementary_stream_type;
        RDK_LOG(
                        RDK_LOG_TRACE1,
                        "LOG.RDK.SI",
            "<GetStreamTypeForServiceComponentHandle> comp_handle: 0x%p, PID : 0x%x\n",
                        elem_stream_list, elem_stream_list->elementary_PID);

        return RMF_SI_SUCCESS;
}

/**
 * @brief This function gets the service component pids such as audio,video from the given component handle.
 *
 * @param[in] comp_handle component handle to get the Pid from.
 * @param[out] comp_pid Output parameter to populate the component pid.
 *
 * @retval RMF_SI_SUCCESS Returned when successfully acquired the component pid.
 * @retval RMF_SI_INVALID_PARAMETER Returned if component handle is invalid OR component pid is NULL.
 */
rmf_Error rmf_OobSiCache::GetPidForServiceComponentHandle(
                rmf_SiServiceComponentHandle comp_handle, uint32_t *comp_pid)
{
        rmf_SiElementaryStreamList *elem_stream_list = NULL;

        /* Parameter check */
        if ((comp_handle == RMF_SI_INVALID_HANDLE) || (comp_pid == NULL))
        {
                return RMF_SI_INVALID_PARAMETER;
        }

        *comp_pid = 0;

        elem_stream_list = (rmf_SiElementaryStreamList *) comp_handle;
        RDK_LOG(
                        RDK_LOG_TRACE1,
                        "LOG.RDK.SI",
            "<GetPidForServiceComponentHandle> comp_handle: 0x%p, PID : 0x%x\n",
                        elem_stream_list, elem_stream_list->elementary_PID);
        *comp_pid = elem_stream_list->elementary_PID;

        return RMF_SI_SUCCESS;
}

/**
 * @brief This function retrieves the program_number for the given service handle.
 *
 * @param[in] service_handle Service_handle represents a SCTE service/channel.
 * @param[out] prog_num Output parameter to populate program_number.
 *
 * @retval RMF_SI_SUCCESS Returned when successfully acquired the program number.
 * @retval RMF_SI_INVALID_PARAMETER Returned if service handle is invalid or program number is NULL.
 * @retval RMF_SI_NOT_FOUND Returned when SI info is not found amd program number is unknown.
 */
rmf_Error rmf_OobSiCache::GetProgramNumberForServiceHandle(
                rmf_SiServiceHandle service_handle, uint32_t *prog_num)
{
        rmf_SiTableEntry *si_entry = NULL;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                        "<GetProgramNumberForServiceHandle> service_handle:0x%x\n",
                        service_handle);

        /* Parameter check */
        if ((service_handle == RMF_SI_INVALID_HANDLE) || (prog_num == NULL))
        {
                return RMF_SI_INVALID_PARAMETER;
        }

        si_entry = (rmf_SiTableEntry *) service_handle;

        *prog_num = 0;
        // program == NULL when analog channel is up.
        if ((si_entry != NULL) && (RMF_SI_INVALID_HANDLE != si_entry->program))
        {
                RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                        "<GetProgramNumberForServiceHandle> si_entry->program:0x%x\n",
                        si_entry->program);
                *prog_num = ((rmf_SiProgramInfo*)si_entry->program)->program_number;
        }
        else
        {
          RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI",
                        "<GetProgramNumberForServiceHandle> Error !!! si_entry->program == NULL for si_entry %x, SrcId %x\n", si_entry, (si_entry != NULL) ? (si_entry->source_id) : 0);
          return RMF_SI_NOT_FOUND;
        }

         RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI",
                        "<GetProgramNumberForServiceHandle> prog_num:%d\n",
                        *prog_num);
        return RMF_SI_SUCCESS;
}

/**
 * Return all service handles in the given transport stream.
 *
 * <i>GetAllServicesForTransportStreamHandle()</i>
 *
 * @param ts_handle input transport stream handle
 * @param array_service_handle output param to populate service handles
 * @param num_services Input param: has size of array_service_handle.
 * Output param: the actual number of transport handles placed in array_service_handle
 *
 * @return RMF_SI_SUCCESS if successful, else
 *          return appropriate rmf_Error
 *
 */
rmf_Error rmf_OobSiCache::GetAllServicesForTransportStreamHandle(
                rmf_SiTransportStreamHandle ts_handle,
                rmf_SiServiceHandle array_service_handle[], uint32_t* num_services)
{
        rmf_Error err = RMF_SI_SUCCESS;
        rmf_SiTableEntry *walker = NULL;
        uint32_t i = 0;

        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                        "<GetAllServicesForTransportStreamHandle> ts_handle:0x%x\n",
                        ts_handle);
        /* Parameter check */
        if ((ts_handle == RMF_SI_INVALID_HANDLE) || (array_service_handle == NULL)
                        || (num_services == NULL || *num_services == 0))
        {
                return RMF_SI_INVALID_PARAMETER;
        }

        /* Acquire global list mutex */
        //rmf_osal_mutexAcquire(g_global_si_list_mutex);

        walker = g_si_entry;

        while (walker)
        {
                if ((walker->channel_type == CHANNEL_TYPE_NORMAL)
                                && ((walker->state == SIENTRY_MAPPED) || (walker->state
                                                == SIENTRY_DEFINED_MAPPED)) && (walker->ts_handle
                                == ts_handle))
                {
                        if (i == *num_services)
                        {
                                RDK_LOG(
                                                RDK_LOG_ERROR,
                                                "LOG.RDK.SI",
                                                "<GetAllServicesForTransportStreamHandle> array_service_handle (%d) too small for list\n",
                                                *num_services);
                                err = RMF_OSAL_ENOMEM;
                                break;
                        }
                        walker->ref_count++;
                        array_service_handle[i++] = (rmf_SiServiceHandle) walker;
                }
                walker = walker->next;
        }

        *num_services = i;

        //rmf_osal_mutexRelease(g_global_si_list_mutex);
        /* Release global list mutex */

        return err;
}

rmf_Error rmf_OobSiCache::FindPidInPMT(rmf_SiServiceHandle service_handle, int pid)
{
        rmf_Error err = RMF_SI_SUCCESS;
        rmf_SiTableEntry *si_entry = (rmf_SiTableEntry *) service_handle;
        rmf_SiProgramInfo *pi;
        rmf_SiElementaryStreamList *elem_stream_list = NULL;
        int found = 0;

    /* Parameter check */
        if (service_handle == RMF_SI_INVALID_HANDLE)
        {
                err = RMF_SI_INVALID_PARAMETER;
        }
        else
        {
        /* SI DB has not been populated yet */
                if (g_si_entry == NULL)
                {
                        err = RMF_SI_NOT_AVAILABLE;
                }
                else
                {
                        pi = (rmf_SiProgramInfo*)si_entry->program;
                        RDK_LOG(RDK_LOG_TRACE1,
                                        "LOG.RDK.SI",
                    "mpe_siFindPidInPMT - pi:0x%p pid:%d \n", pi, pid);
                        if (pi == NULL)
                        {
                                rmf_SiTransportStreamEntry *ts = (rmf_SiTransportStreamEntry *) si_entry->ts_handle;
                                if (ts != NULL)
                                {
                                        /*if (ts->modulation_mode != RMF_MODULATION_QAM_NTSC)
                                                err = RMF_SI_INVALID_PARAMETER;
                                        else
                                                err = RMF_SI_NOT_FOUND;*/
                                }
                                else
                                {
                                        err = RMF_SI_NOT_AVAILABLE; // SPI service?
                                }
                        }
                        else
                        {
                                RDK_LOG(RDK_LOG_TRACE1,
                                                "LOG.RDK.SI",
                                                "mpe_siFindPidInPMT - pmt_status:%d \n", pi->pmt_status);
                /* If PSI is in the process of being acquired return from here
                 until it is updated. */
                                if (pi->pmt_status == PMT_NOT_AVAILABLE)
                                {
                                        err = RMF_SI_NOT_AVAILABLE;
                                }
                                else if (pi->pmt_status == PMT_AVAILABLE_SHORTLY)
                                {
                                        err = RMF_SI_NOT_AVAILABLE_YET;
                                }
                                else
                                {
                                        elem_stream_list = pi->elementary_streams;
                                        while (elem_stream_list && !found)
                                        {
                                                if (elem_stream_list->elementary_PID == pid)
                                                {
                                                        found = 1;
                                                        err = RMF_SI_SUCCESS;
                                                        break;
                                                }
                                                elem_stream_list = elem_stream_list->next;
                                        }

                                        if (!found)
                                        {
                                                err = RMF_SI_NOT_FOUND;
                                        }
                                }
                        }
                }
        }
        return err;
}

/**
 * @brief This function returns the number of services in the given transport stream.
 *
 * @param[in] ts_handle Transport stream handle to get the number of service from.
 * @param[in] num_services Output parameter to populate the number of services.
 *
 * @retval RMF_INBSI_SUCCESS Returned when successfully acquired the number of services.
 * @retval RMF_SI_INVALID_PARAMETER Returned if transport stream handle is invalid OR number of services is NULL.
 */
rmf_Error rmf_OobSiCache::GetNumberOfServicesForTransportStreamHandle(
        rmf_SiTransportStreamHandle ts_handle, uint32_t *num_services)
{
        rmf_SiTransportStreamEntry *target_ts_entry = NULL;

        /* Parameter check */
        if ((ts_handle == RMF_SI_INVALID_HANDLE) || (num_services == NULL))
        {
                return RMF_SI_INVALID_PARAMETER;
        }

        // This method can be called for OOB (PAT/PMT) which
        // is independent of OOB SI (SVCT, NIT).
        // Don't check the g_si_state

        *num_services = 0;

        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                        "<%s> 0x%x\n",
                        __FUNCTION__, ts_handle);

        target_ts_entry = (rmf_SiTransportStreamEntry *) ts_handle;
        *num_services = target_ts_entry->visible_service_count;

        RDK_LOG(
                        RDK_LOG_TRACE1,
                        "LOG.RDK.SI",
                        "<%s> ts_entry->visible_service_count:%d\n",
                        __FUNCTION__, target_ts_entry->visible_service_count);

        return RMF_SI_SUCCESS;
}


/*  Called by SITP when it detects tune away from one frequency to a
 different frequency. This is used to signal SI_NOT_AVAILABLE
 state to registered listeners.
 */
/*
 rmf_Error rmf_OobSiCache::NotifyTunedAway(uint32_t frequency)
 {
 rmf_SiRegistrationListEntry *walker = NULL;
 uint32_t eventFrequency = 0;
 mpe_Event event = RMF_SI_EVENT_TUNED_AWAY;
 rmf_SiTransportStreamEntry *ts_entry = NULL;
 rmf_SiStatus status;

 RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI","<mpe_siNotifyTunedAway> called...with freq: %d \n", frequency);
 eventFrequency = frequency;

 // Notify registered callers only if the current status is available_shortly
 ts_entry = find_transport_stream(frequency);
 if(ts_entry)
 {
 status = ts_entry->si_status;
 RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI","<mpe_siNotifyTunedAway> ..si status: %d \n", status);
 if(status != SI_AVAILABLE_SHORTLY)
 {
 return RMF_SI_SUCCESS; // error?
 }
 }


 // Acquire registration list mutex
 rmf_osal_mutexAcquire(g_registration_list_mutex);

 walker = g_si_registration_list;

 while (walker)
 {
 rmf_osal_Bool notifyWalker = false;

 if (walker->frequency == 0 || (walker->frequency == eventFrequency))
 notifyWalker = true;

 if(notifyWalker)
 {
 uint32_t termType = walker->edHandle->terminationType;

 mpe_eventQueueSend(walker->edHandle->eventQ,
 event,
 (void*)NULL, (void*)walker->edHandle,
 0);

 // Do we need to unregister this client?
 if (termType == RMF_ED_TERMINATION_ONESHOT ||
 (termType == RMF_ED_TERMINATION_EVCODE && event == walker->terminationEvent))
 {
 add_to_unreg_list(walker->edHandle);
 }
 }
 walker = walker->next;
 }

 // Unregister all clients that have been added to unregistration list
 unregister_clients();

 notify_registered_queues(event, 0);

 // Release registration list mutex
 rmf_osal_mutexRelease(g_registration_list_mutex);

 return RMF_SI_SUCCESS;
 }
 */

/*  Called by SITP when it detects tables first but has not yet retrieved them.
 This enabled marking SI entries appropriately in anticipation of new SI
 acquisition.
 */
rmf_Error rmf_OobSiCache::NotifyTableDetected(rmf_SiTableType table_type,
        uint32_t optional_data)
{
    rmf_Error err = RMF_SI_INVALID_PARAMETER;

    /*  check this parameter used for something, or be taken out of this API? */
    RMF_OSAL_UNUSED_PARAM(optional_data);

    /* Top-level table type handler */
    switch (table_type)
    {
    case OOB_SVCT_VCM:
        //mark_si_entries(SIENTRY_OLD);
        err = RMF_SI_SUCCESS;
        break;
    case OOB_NIT_CDS:
        //mark_transport_stream_entries( TSENTRY_OLD);
        err = RMF_SI_SUCCESS;
        break;
    default:
        break;
    } // END switch (table_type)

    return err;
}

/*  Internal method to mark service entries with correct state */
/*
 static void mark_si_entries(rmf_SiEntryState new_state)
 {

 rmf_SiTableEntry *walker = NULL;

 if (!g_modulationsUpdated || !g_frequenciesUpdated || !g_channelMapUpdated)
 {
 RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI","<mark_si_entries> ..\n");
 }


 //  block here until the condition object is set

 rmf_osal_condGet(g_global_si_cond);

 walker = g_si_entry;
 while (walker)
 {
 walker->state = new_state;
 walker = walker->next;
 }

 //    release global condition

 rmf_osal_condSet(g_global_si_cond);
 }
 */


/**
 Accessor methods to retrieve individual fields from service entries
 */


/**
 * @brief  This function retrieves the service type for the given service handle.
 * Service type represents such as "digital television",
 * "digital radio", "NVOD reference service", "NVOD time-shifted service",
 * "analog television", "analog radio", "data broadcast" and "application".
 *
 * @param[in] service_handle Service_handle represents a SCTE service/channel.
 * @param[out] service_type  Output parameter to populate the service type.
 *
 * @retval RMF_SI_SUCCESS Returned if successfully acquired the service type.
 * @retval RMF_SI_INVALID_PARAMETER If service handle is invalid OR service type is NULL.
 */
rmf_Error rmf_OobSiCache::GetServiceTypeForServiceHandle(
        rmf_SiServiceHandle service_handle, rmf_SiServiceType *service_type)
{
    rmf_SiTableEntry *si_entry = (rmf_SiTableEntry *) service_handle;
//  rmf_SiTransportStreamEntry *ts = NULL;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<GetServiceTypeForServiceHandle> service_handle:0x%x\n",
            service_handle);

    /* Parameter check */
    if ((service_handle == RMF_SI_INVALID_HANDLE) || (service_type == NULL))
    {
        return RMF_SI_INVALID_PARAMETER;
    }
//  ts = (rmf_SiTransportStreamEntry *) si_entry->ts_handle;

    *service_type = si_entry->service_type;
    // According to OCAP spec return 'UNKNOWN' if service_type is not known
    // However...
/*  if ((*service_type == RMF_SI_SERVICE_TYPE_UNKNOWN) && ((ts != NULL)
            && (ts->modulation_mode == RMF_MODULATION_QAM_NTSC)))
    {
        // The WatchTvHost app needs to know which services
        // are analog, because it must filter to use only
        // those services.  If we don't know the type, but the
        // modulation mode is analog, then it should be safe
        // to presume it is in fact an analog service, so
        // report it as such.
        *service_type = RMF_SI_SERVICE_ANALOG_TV;
    }*/
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<GetServiceTypeForServiceHandle> *service_type:%d\n",
            *service_type);

    return RMF_SI_SUCCESS;
}

/**
 * @brief This function is used to get the total number of source entries available in SI table.
 *
 * @param[in] service_handle Service_handle represents a SCTE service/channel.
 * @param[out] num_long_names Returns the number of long names owned by the service.
 *
 * @retval RMF_SI_SUCCESS Returned if successfully acquired the number of entries available in SI table.
 * @retval RMF_SI_INVALID_PARAMETER Returned if service handle is invalid OR no entries found in SI table structure.
 */
rmf_Error rmf_OobSiCache::GetNumberOfLongNamesForServiceHandle(
        rmf_SiServiceHandle service_handle, uint32_t *num_long_names)
{
    uint32_t count = 0;
    rmf_SiTableEntry *si_entry = NULL;
    rmf_siSourceNameEntry *sn_entry;
    rmf_SiLangSpecificStringList *ln_walker = NULL;

    RDK_LOG(
            RDK_LOG_TRACE1,
            "LOG.RDK.SI",
            "<GetNumberOfLongNamesForServiceHandle> service_handle:0x%x\n",
            service_handle);

    /* Parameter check */
    if ((service_handle == RMF_SI_INVALID_HANDLE) || (num_long_names == NULL))
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    si_entry = (rmf_SiTableEntry *) service_handle;
    sn_entry = si_entry->source_name_entry;

    // Count the entries
    if (sn_entry != NULL)
    {
      for (ln_walker = sn_entry->source_names; ln_walker != NULL; ln_walker = ln_walker->next, count++)
        ;
    }

    *num_long_names = count;
    return RMF_SI_SUCCESS;
}

/**
 * @brief This function retrieves the long service names and associated ISO639 langauges for the given service handle.
 * The language at index 0 of the languages array is associated with the name at index 0 of
 * of the names array and so on.
 *
 * @param[in] service_handle Service_handle represents a SCTE service/channel.
 * @param[out] languages Output parameter to populate the available languages from SI Table entry.
 * @param[out] service_long_names Output parameter to populate the long service name.
 *
 * @retval RMF_SI_SUCCESS Returned when successfully acquired the long service names.
 * @retval RMF_SI_INVALID_PARAMETER Returned if service handle is invalid OR language and service long names is NULL.
 */
rmf_Error rmf_OobSiCache::GetLongNamesForServiceHandle(
        rmf_SiServiceHandle service_handle, char* languages[],
        char* service_long_names[])
{
    rmf_SiLangSpecificStringList *walker = NULL;
    rmf_SiTableEntry *si_entry = NULL;
    rmf_siSourceNameEntry *sn_entry;
    int i = 0;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<GetLongNamesForServiceHandle> service_handle:0x%x\n",
            service_handle);

    /* Parameter check */
    if ((service_handle == RMF_SI_INVALID_HANDLE) || (languages == NULL)
            || (service_long_names == NULL))
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    si_entry = (rmf_SiTableEntry *) service_handle;
    sn_entry = si_entry->source_name_entry;

    if (sn_entry != NULL && sn_entry->source_names != NULL)
    {
    for (walker = sn_entry->source_names; walker != NULL; walker = walker->next, i++)
    {
        languages[i] = walker->language;
        service_long_names[i] = walker->string;

        RDK_LOG(
                RDK_LOG_TRACE1,
                "LOG.RDK.SI",
                "<GetNamesForServiceHandle> source_name:%s, language:%s\n",
                walker->string, walker->language);
      }
    }

    return RMF_SI_SUCCESS;
}

/**
 * @brief This function retrieves the number of language variants of the service name for the given service handle.
 *
 * @param[in] service_handle Service_handle represents a SCTE service/channel.
 * @param[out] num_names Output parameter to populate the number of service name language variants.
 *
 * @retval RMF_SI_SUCCESS Returned when successfully acquired the number of names.
 * @retval RMF_SI_INVALID_PARAMETER Returned if service handle is invalid OR number of names is NULL.
 */
rmf_Error rmf_OobSiCache::GetNumberOfNamesForServiceHandle(
        rmf_SiServiceHandle service_handle, uint32_t *num_names)
{
    uint32_t count = 0;
    rmf_SiTableEntry *si_entry = NULL;
    rmf_siSourceNameEntry *sn_entry;
    rmf_SiLangSpecificStringList *ln_walker = NULL;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<GetNumberOfNamesForServiceHandle> service_handle:0x%x\n",
            service_handle);

    /* Parameter check */
    if ((service_handle == RMF_SI_INVALID_HANDLE) || (num_names == NULL))
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    si_entry = (rmf_SiTableEntry *) service_handle;
    sn_entry = si_entry->source_name_entry;

    // Count the entries
    if (sn_entry != NULL)
    {
      for (ln_walker = sn_entry->source_names; ln_walker != NULL; ln_walker = ln_walker->next, count++)
        ;
    }

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<GetNumberOfNamesForServiceHandle> service_handle:0x%x, count=%d\n",
            service_handle, count);
    *num_names = count;
    return RMF_SI_SUCCESS;
}

/**
 * @brief This function retrieves the language-specfic service names for the given service handle.
 * The language at index 0 of the languages array is associated with the name at index 0 of
 * of the names array and so on.
 *
 * @param[in] service_handle Service_handle represents a SCTE service/channel.
 * @param[out] languages Output parameter to populate the long service name languages.
 * @param[out] service_names Output parameter to populate the service names.
 *
 * @retval RMF_SI_SUCCESS Returned when successfully acquired the name for service handle.
 * @retval RMF_SI_INVALID_PARAMETER Returned if service handle is invalid OR language is NULL.
 */
rmf_Error rmf_OobSiCache::GetNamesForServiceHandle(rmf_SiServiceHandle service_handle,
        char* languages[], char* service_names[])
{
    rmf_SiLangSpecificStringList *ln_walker = NULL;
    rmf_SiTableEntry *si_entry = NULL;
    rmf_siSourceNameEntry *sn_entry;
    int i = 0;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<GetNamesForServiceHandle> service_handle:0x%x\n",
            service_handle);

    /* Parameter check */
    if (service_handle == RMF_SI_INVALID_HANDLE || languages == NULL
            || service_names == NULL)
        return RMF_SI_INVALID_PARAMETER;

    si_entry = (rmf_SiTableEntry *) service_handle;
    sn_entry = si_entry->source_name_entry;

    if (sn_entry != NULL && sn_entry->source_names != NULL)
    {
    for (ln_walker = sn_entry->source_names; ln_walker != NULL; ln_walker = ln_walker->next, i++)
    {
        languages[i] = ln_walker->language;
        service_names[i] = ln_walker->string;

        RDK_LOG(
                RDK_LOG_TRACE1,
                "LOG.RDK.SI",
                "<GetNamesForServiceHandle> source_name:%s, language:%s\n",
                  ln_walker->string, ln_walker->language);
      }
    }

    return RMF_SI_SUCCESS;
}

/**
 * @brief This function retrieves the last update time for the given service handle.
 *
 * @param[in] service_handle Service_handle represents a SCTE service/channel.
 * @param[out] time Output parameter to populate the update time.
 *
 * @retval RMF_SI_SUCCESS Returned when successfully acquired the service descriptions last update time.
 * @retval RMF_SI_INVALID_PARAMETER Returned if service handle is invalid OR pTime is NULL.
 */
rmf_Error rmf_OobSiCache::GetServiceDescriptionLastUpdateTimeForServiceHandle(
        rmf_SiServiceHandle service_handle, rmf_osal_TimeMillis *pTime)
{
    rmf_SiTableEntry *si_entry = NULL;

    /* Parameter check */
    if ((service_handle == RMF_SI_INVALID_HANDLE) || (pTime == NULL))
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    si_entry = (rmf_SiTableEntry *) service_handle;

    *pTime = si_entry->ptime_service;

    return RMF_SI_SUCCESS;
}

/**
 * @brief This function retrieves the service number for the given service handle.
 * According to OCAP specification, return '-1' indicates the service_numbers are not known.
 *
 * @param[in] service_handle Service_handle represents a SCTE service/channel.
 * @param[out] service_number Output parameter to populate the service number.
 * @param[out] minor_number  Output parameter to populate the minor service number.
 *
 * @retval RMF_SI_SUCCESS Returned when successfully acquired the service number.
 * @retval RMF_SI_INVALID_PARAMETER Returned if service handle is invali OR service number and minor service number is NULL.
 */
rmf_Error rmf_OobSiCache::GetServiceNumberForServiceHandle(
        rmf_SiServiceHandle service_handle, uint32_t* service_number,
        uint32_t* minor_number)
{
    rmf_SiTableEntry *si_entry = NULL;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<GetServiceNumberForServiceHandle> service_handle:0x%x\n",
            service_handle);

    /* Parameter check */
    if ((service_handle == RMF_SI_INVALID_HANDLE) || (service_number == NULL)
            || (minor_number == NULL))
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    si_entry = (rmf_SiTableEntry *) service_handle;

    // According to OCAP spec return '-1' is service_numbers are not known
    *service_number = si_entry->major_channel_number;
    *minor_number = si_entry->minor_channel_number;

    return RMF_SI_SUCCESS;
}

/**
 * @brief This function retrieves the virtual channel number for the given service handle.
 * The feature of VCN is to map virtual channel numbers to underlying physical and sub-channels.
 * The physical/sub-channel numbers are called the "QAM channel", and
 * the alternative channel designation is called either "mapped channel", "virtual channel", or simply "channel".
 *
 * @param[in] service_handle Service_handle represents a SCTE service/channel.
 * @param[out] virtual_channel_number Output parameter to populate the virtual channel number.
 *
 * @retval RMF_SI_SUCCESS Returned when successfully acquired the virtual channel number.
 * @retval RMF_SI_INVALID_PARAMETER Returned if service handle is invalid OR virtual channel number is NULL.
 */
rmf_Error rmf_OobSiCache::GetVirtualChannelNumberForServiceHandle(
		rmf_SiServiceHandle service_handle, uint16_t *virtual_channel_number)
{
    rmf_SiTableEntry *si_entry = NULL;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<GetVirtualChannelNumberForServiceHandle> service_handle:0x%x\n",
            service_handle);

    /* Parameter check */
    if ((service_handle == RMF_SI_INVALID_HANDLE) || (virtual_channel_number == NULL))
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    si_entry = (rmf_SiTableEntry *) service_handle;

    *virtual_channel_number = si_entry->virtual_channel_number;

    return RMF_SI_SUCCESS;
}

/**
 * @brief This function retrieves the source Id for the given service handle.
 *
 * @param[in] service_handle Service_handle represents a SCTE service/channel.
 * @param[out] sourceId  Output parameter to populate the source id.
 *
 * @retrurn rmf_Error
 * @retval RMF_SI_SUCCESS Returned when successfully acquired the sourceId.
 * @retval RMF_SI_INVALID_PARAMETER Returned if service handle is invalid OR sourceId is NULL.
 */
rmf_Error rmf_OobSiCache::GetSourceIdForServiceHandle(rmf_SiServiceHandle service_handle,
        uint32_t *sourceId)
{
    rmf_SiTableEntry *si_entry = NULL;
    //rmf_SiTableEntry *walker = NULL;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<GetSourceIdForServiceHandle> service_handle:0x%x\n",
            service_handle);

    /* Parameter check */
    if ((service_handle == RMF_SI_INVALID_HANDLE) || (sourceId == NULL))
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    si_entry = (rmf_SiTableEntry *) service_handle;

    *sourceId = si_entry->source_id;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<GetSourceIdForServiceHandle> *sourceId:0x%x\n", *sourceId);

    /*  If this handle was acquired from the main tree then sourceId would not be
     -1. Only when it is acquired from dynamic tree will it be -1. In that case
     we want to check if this freq, prog_num pair has been signalled in SVCT
     also and if so get the sourceId from there. In these cases, the service
     entry in dynamic tree takes precedence. i.e. if a service entry is created in
     the dynamic tree based on freq, prog_num and if this pair gets signalled in the
     SVCT also (with a sourceId) then the service entry in the dynamic tree takes
     precedence. The corresponding entry in the main tree will be accessed to get
     info that's not available in the dynamic tree. (ex: sourceId, modulation mode etc.)
     */
    /*  For 0.95 release, SI DB no longer uses two different trees. Static and dynamic trees
     are merged. And, a service entry can only exist once.

      Test removing the logic below...
     */
    /*
     if(si_entry->source_id == SOURCEID_UNKNOWN)
     {
     // Walk through the list of services to find a match
     walker = g_si_entry;
     while(walker)
     {
     if( (walker->frequency == si_entry->frequency) && (walker->program_number == si_entry->program_number) )
     {
     *sourceId = walker->source_id;
     break;
     }
     walker = walker->next;
     }
     }
     */

    return RMF_SI_SUCCESS;
}

/**
 * @brief This function retrieves the appID for the given service handle.
 * A 16 bit application ID does not have to be unique, but no two applications signalled in the same
 * AIT[application information table] can have the same organization ID and application ID.
 * Given the size of the ID range, it's good practice not to re-use application ID's where possible.
 *
 * @param[in] service_handle Service_handle represents a SCTE service/channel.
 * @param[out] appId Output parameter to populate the appId.
 *
 * @retval RMF_SI_SUCCESS Returned when successfully acquired the AppId.
 * @retval RMF_SI_INVALID_PARAMETER Returned if service handle is invalid OR appId is NULL.
 */
rmf_Error rmf_OobSiCache::GetAppIdForServiceHandle(rmf_SiServiceHandle service_handle,
        uint32_t* appId)
{
    rmf_SiTableEntry *si_entry = NULL;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<GetAppIDForServiceHandle> service_handle:0x%x\n",
            service_handle);

    /* Parameter check */
    if ((service_handle == RMF_SI_INVALID_HANDLE) || (appId == NULL))
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    si_entry = (rmf_SiTableEntry *) service_handle;

    //  There is a separate app_id field in 'si_entry', check which one (source_id/app_id)
    // needs to be returned.
    // if (si_entry->isAppType)
    *appId = si_entry->app_id;
    //else
    //  return RMF_SI_NOT_FOUND;

    return RMF_SI_SUCCESS;
}

/**
 * @brief This function retrieves the frequency for the given service handle.
 * If the service handle was acquired from the sourceId, then need to look out in the global array
 * populated from NIT table based on the index. For out of band, frequency is -1.
 *
 * @param[in] service_handle Service_handle represents a SCTE service/channel.
 * @param[out] freq is the Output parameter to populate the frequency.
 *
 * @retval RMF_SI_SUCCESS Returned when successfully acquired the frequency.
 * @retval RMF_SI_INVALID_PARAMETER Returned if service handle is invalid OR frequency is NULL.
 */
rmf_Error rmf_OobSiCache::GetFrequencyForServiceHandle(
        rmf_SiServiceHandle service_handle, uint32_t *freq)
{
    rmf_SiTableEntry *si_entry = NULL;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<GetFrequencyForServiceHandle> service_handle:0x%x\n",
            service_handle);

    /* Parameter check */
    if ((service_handle == RMF_SI_INVALID_HANDLE) || (freq == NULL))
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    si_entry = (rmf_SiTableEntry *) service_handle;

    *freq = 0;

    /*  If the handle was acquired based on sourceId, then look up in
     the global array (populated from NIT) based on the index.
     */
    /*  For OOB the frequency is -1 */
    if (si_entry->ts_handle != RMF_SI_INVALID_HANDLE)
    {
        *freq = ((rmf_SiTransportStreamEntry *) si_entry->ts_handle)->frequency;
    }
    else
    {
        // This should never happen..
        //*freq = g_frequency[si_entry->freq_index];
        return RMF_SI_NOT_FOUND;
    }

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<GetFrequencyForServiceHandle> freq:%d\n", *freq);

    return RMF_SI_SUCCESS;
}

/**
 * @brief This function retrieves the modulation format for the given service handle.
 *
 * @param[in] service_handle Service_handle represents a SCTE service/channel.
 * @param[out] mode Output parameter to populate the mode.
 *
 * @retval RMF_SI_SUCCESS Returned when successfully acquired the modulation format.
 * @retval RMF_SI_INVALID_PARAMETER Returned if service handle is invalid OR mode is NULL.
 * @retval RMF_SI_NOT_FOUND Returned when SI info is not found and modulation mode is unknown.
 */
rmf_Error rmf_OobSiCache::GetModulationFormatForServiceHandle(
        rmf_SiServiceHandle service_handle, rmf_ModulationMode *mode)
{
    rmf_SiTableEntry *si_entry = NULL;
    rmf_SiTransportStreamEntry *ts_entry = NULL;

    RDK_LOG(
            RDK_LOG_TRACE1,
            "LOG.RDK.SI",
            "<GetModulationFormatForServiceHandle> service_handle:0x%x\n",
            service_handle);

    /* Parameter check */
    if ((service_handle == RMF_SI_INVALID_HANDLE) || (mode == NULL))
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    si_entry = (rmf_SiTableEntry *) service_handle;

    if (si_entry->ts_handle != RMF_SI_INVALID_HANDLE)
    {
        ts_entry = (rmf_SiTransportStreamEntry *) si_entry->ts_handle;
        *mode = (rmf_ModulationMode)ts_entry->modulation_mode;
    }
    else
    {
        *mode = RMF_MODULATION_UNKNOWN;
        return RMF_SI_NOT_FOUND;
        //*mode = g_mode[si_entry->mode_index];       
    }

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<GetModulationMode> returning mode:%d\n", *mode);

    return RMF_SI_SUCCESS;
}

/**
 * @brief This function retrieves the service details last update time for the given service handle.
 *
 * @param[in] service_handle Service_handle represents a SCTE service/channel.
 * @param[out] time Output parameter to populate the last update time.
 *
 * @retval RMF_SI_SUCCESS returned when successfully acquired the service details.
 * @retval RMF_SI_INVALID_PARAMETER Returned if service handle is invalid OR pTime is NULL.
 */
rmf_Error rmf_OobSiCache::GetServiceDetailsLastUpdateTimeForServiceHandle(
        rmf_SiServiceHandle service_handle, rmf_osal_TimeMillis *pTime)
{
    rmf_SiTableEntry *si_entry = NULL;

    /* Parameter check */
    if ((service_handle == RMF_SI_INVALID_HANDLE) || (pTime == NULL))
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    si_entry = (rmf_SiTableEntry *) service_handle;

    *pTime = si_entry->ptime_service;

    return RMF_SI_SUCCESS;
}

/**
 * @brief This function retrieves the 'isFree' flag for the given service handle.
 * is_free flag indicates that its clear service.
 * Else it indicates the encrypted service.
 *
 * @param[in] service_handle Service_handle represents a SCTE service/channel.
 * @param[out] is_free Output parameter to populate the 'is_free' flag.
 *
 * @retval RMF_SI_SUCCESS Returned when successfully acquired the 'is_free' flag.
 * @retval RMF_SI_INVALID_PARAMETER Returned if service handle is invalid OR is_free flag is NULL.
 */
rmf_Error rmf_OobSiCache::GetIsFreeFlagForServiceHandle(
        rmf_SiServiceHandle service_handle, rmf_osal_Bool *is_free)
{
    /* Parameter check */
    if ((service_handle == RMF_SI_INVALID_HANDLE) || (is_free == NULL))
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    /*  
     How do we know whether a service is in the clear (free) or encrypted?
     */
    *is_free = TRUE;

    return RMF_SI_SUCCESS;
}

/**
 * @brief This function retrieves the number of language-specific descriptions for the given service handle.
 *
 * @param[in] service_handle Service_handle represents a SCTE service/channel.
 * @param[out] num_descriptions Output parameter to populate the number of descriptions.
 *
 * @retval RMF_SI_SUCCESS Returned when successfully acquired the number of descriptions.
 * @retval RMF_SI_INVALID_PARAMETER Returned if service handle is invalid OR number of descriptions is NULL.
 */
rmf_Error rmf_OobSiCache::GetNumberOfServiceDescriptionsForServiceHandle(
        rmf_SiServiceHandle service_handle, uint32_t *num_descriptions)
{
    rmf_SiTableEntry *si_entry = NULL;
    rmf_SiLangSpecificStringList *walker = NULL;
    uint32_t count = 0;

    /* Parameter check */
    if (service_handle == RMF_SI_INVALID_HANDLE || num_descriptions == NULL)
        return RMF_SI_INVALID_PARAMETER;

    si_entry = (rmf_SiTableEntry *) service_handle;

    // Count the entries
    for (walker = si_entry->descriptions; walker != NULL; walker = walker->next, count++)
        ;

    *num_descriptions = count;
    return RMF_SI_SUCCESS;
}

/**
 * @brief This function retrieves the language-specific service descriptions for the given service handle.
 * The language at index 0 of the languages array is associated with the name at index 0 of
 * of the descriptions array and so on.
 *
 * @param[in] service_handle Service_handle represents a SCTE service/channel.
 * @param[out] languages  Output parameter to populate the languages.
 * @param[out] descriptions Output parameter to populate the descriptions.
 *
 * @retval RMF_SI_SUCCESS Returned when successfully acquired the service descriptions.
 * @retval RMF_SI_INVALID_PARAMETER Returned if service handle is invalid OR languages is NULL.
 */
rmf_Error rmf_OobSiCache::GetServiceDescriptionsForServiceHandle(
        rmf_SiServiceHandle service_handle, char* languages[],
        char* descriptions[])
{
    rmf_SiTableEntry *si_entry = NULL;
    rmf_SiLangSpecificStringList *walker = NULL;
    int i = 0;

    RDK_LOG(
            RDK_LOG_TRACE1,
            "LOG.RDK.SI",
            "<GetServiceDescriptionsForServiceHandle> service_handle:0x%x\n",
            service_handle);

    /* Parameter check */
    if (service_handle == RMF_SI_INVALID_HANDLE || languages == NULL
            || descriptions == NULL)
        return RMF_SI_INVALID_PARAMETER;

    si_entry = (rmf_SiTableEntry *) service_handle;

    for (walker = si_entry->descriptions; walker != NULL; walker = walker->next, i++)
    {
        languages[i] = walker->language;
        descriptions[i] = walker->string;

        RDK_LOG(
                RDK_LOG_TRACE1,
                "LOG.RDK.SI",
                "<GetServiceDescriptionsForServiceHandle> description:%s, language:%s\n",
                walker->string, walker->language);
    }

    return RMF_SI_SUCCESS;
}

/**
 * @brief This function retrieves the delivery system types such as cable, satellite and terrestial for the given service handle.
 *
 * @param[in] service_handle Service_handle represents a SCTE service/channel.
 * @param[out] type Output parameter to populate the delivery system type.
 *
 * @retval RMF_SI_SUCCESS Returned when successfully acquired the delivery system type.
 * @retval RMF_SI_INVALID_PARAMETER Returned if service handle is invalid OR delivery system type is NULL.
 */
rmf_Error rmf_OobSiCache::GetDeliverySystemTypeForServiceHandle(
        rmf_SiServiceHandle service_handle, rmf_SiDeliverySystemType* type)
{
    if ((service_handle == RMF_SI_INVALID_HANDLE) || (type == NULL))
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    *type = RMF_SI_DELIVERY_SYSTEM_TYPE_CABLE;

    return RMF_SI_SUCCESS;
}

/**
 * Returns CA System IDs array lenth associated with this service. This
 * information may be obtained from the CAT MPEG message or a system
 * specific conditional access descriptor (such as defined by Simulcrypt
 * or ATSC).
 *
 * An empty array is returned when no CA System IDs are available.
 */
rmf_Error rmf_OobSiCache::GetCASystemIdArrayLengthForServiceHandle(
        rmf_SiServiceHandle service_handle, uint32_t *length)
{
    if ((service_handle == RMF_SI_INVALID_HANDLE) || (length == NULL))
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    *length = 0;

    return RMF_SI_SUCCESS;
}

/**
 * Returns CA System IDs array associated with this service. This
 * information may be obtained from the CAT MPEG message or a system
 * specific conditional access descriptor (such as defined by Simulcrypt
 * or ATSC).
 *
 * An empty array is returned when no CA System IDs are available.
 */
rmf_Error rmf_OobSiCache::GetCASystemIdArrayForServiceHandle(
        rmf_SiServiceHandle service_handle, uint32_t* ca_array,
        uint32_t *length)
{
    if ((service_handle == RMF_SI_INVALID_HANDLE) || (ca_array == NULL)
            || (length == NULL))
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    *ca_array = 0;
    *length = 0;

    return RMF_SI_SUCCESS;
}

/**
 * @brief This function retrieves the multiple instances flag for the given service handle.
 *
 * @param[in] service_handle Service_handle represents a SCTE service/channel.
 * @param[out] hasMultipleInstances Output parameter to populate the multiple instances flag.
 *
 * @retval RMF_SI_SUCCESS Returned when successfully acquired the multiple instances flag.
 * @retval RMF_SI_INVALID_PARAMETER Returned if service handle is invalid OR multiple instances is NULL.
 */
rmf_Error rmf_OobSiCache::GetMultipleInstancesFlagForServiceHandle(
        rmf_SiServiceHandle service_handle, rmf_osal_Bool* hasMultipleInstances)
{
    /* Parameter check */
    if ((service_handle == RMF_SI_INVALID_HANDLE) || (hasMultipleInstances
            == NULL))
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    *hasMultipleInstances = FALSE;

    return RMF_SI_SUCCESS;
}

/**
 * @brief This function gets the number of service details from the given service handle.
 *
 * @param[in] service_handle Service_handle represents a SCTE service/channel.
 * @param[out] length Output parameter to populate the number of service details in length .
 *
 * @retval RMF_SI_SUCCESS Returned when successfully acquired the number of service details.
 * @retval RMF_SI_INVALID_PARAMETER Returned if service handle is invalid OR length is NULL.
 */
rmf_Error rmf_OobSiCache::GetNumberOfServiceDetailsForServiceHandle(
        rmf_SiServiceHandle service_handle, uint32_t* length)
{
    /* Parameter check */
    if ((service_handle == RMF_SI_INVALID_HANDLE) || (length == NULL))
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<GetNumberOfServiceDetailsForServiceHandle> \n");
    *length = 1;

    return RMF_SI_SUCCESS;
}

/**
 * @brief This function gets the service details from the given service handle.
 *
 * @param[in] service_handle Service_handle represents a SCTE service/channel.
 * @param[out] arr Output parameter to populate the service details handles.
 *
 * @retval RMF_SI_SUCCESS Returned when successfully acquired the service details .
 * @retval RMF_SI_INVALID_PARAMETER Returned if service handle is invalid OR service details array is NULL and length is zero.
 */
rmf_Error rmf_OobSiCache::GetServiceDetailsForServiceHandle(
        rmf_SiServiceHandle service_handle, rmf_SiServiceDetailsHandle arr[],
        uint32_t length)
{


    /* Parameter check */
    if ((service_handle == RMF_SI_INVALID_HANDLE) || (arr == NULL) || (length
            == 0))
    {
        return RMF_SI_INVALID_PARAMETER;


    }

    arr[0] = (rmf_SiServiceDetailsHandle) service_handle;
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<GetServiceDetailsForServiceHandle>  returning RMF_SI_SUCCESS...\n");

    return RMF_SI_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////////////
/*
 Service Details handle specific (transport dependent)

 Note: For now service details handle and service handle are
 interchangeable in the native layer.

 */
///////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief This function retrieves the 'isFree' flag for the given service details handle.
 * is_free flag indicates that its clear service.
 * Else it indicates the encrypted service.
 *
 * @param[in] handle SI handle to get the 'isFree' flag from.
 * @param[out] is_free Output parameter to populate the 'is_free' flag.
 *
 * @retval RMF_SI_SUCCESS Returned when successfully acquired the 'is_free' flag.
 * @retval RMF_SI_INVALID_PARAMETER Returned if service handle is invalid OR is_free flag is NULL.
 */
rmf_Error rmf_OobSiCache::GetIsFreeFlagForServiceDetailsHandle(
        rmf_SiServiceDetailsHandle handle, rmf_osal_Bool* is_free)
{
    return GetIsFreeFlagForServiceHandle((rmf_SiServiceHandle) handle,
            is_free);
}

/**
 * @brief This function retrieves the source Id for the given ServiceDetailsHandle.
 *
 * @param[in] handle SI handle to get the service Id from.
 * @param[out] sourceId  Output parameter to populate the source id.
 *
 * @retrurn rmf_Error
 * @retval RMF_SI_SUCCESS Returned when successfully acquired the sourceId.
 * @retval RMF_SI_INVALID_PARAMETER Returned if service handle in invalid OR sourceId is NULL.
 */
rmf_Error rmf_OobSiCache::GetSourceIdForServiceDetailsHandle(
        rmf_SiServiceDetailsHandle handle, uint32_t* sourceId)
{
    return GetSourceIdForServiceHandle((rmf_SiServiceHandle) handle,
            sourceId);
}

/**
 * @brief This function retrieves the frequency for the given ServiceDetailsHandle.
 *
 * @param[in] handle SI handle to get the freq from.
 * @param[out] frequency Output parameter to populate the frequency.
 *
 * @retval RMF_SI_SUCCESS Returned when successfully acquired the frequency.
 * @retval RMF_SI_INVALID_PARAMETER Returned if service handle is invalid OR frequency is NULL.
 */
rmf_Error rmf_OobSiCache::GetFrequencyForServiceDetailsHandle(
        rmf_SiServiceDetailsHandle handle, uint32_t* frequency)
{
    return GetFrequencyForServiceHandle((rmf_SiServiceHandle) handle,
            frequency);
}

/**
 * @brief This function retrieves the program number for the given ServiceDetailsHandle.
 *
 * @param[in] handle SI handle to get the program number from.
 * @param[out] progNum Output parameter to populate the program number.
 *
 * @retval RMF_SI_SUCCESS Returned when successfully acquired the frequency.
 * @retval RMF_SI_INVALID_PARAMETER Returned if service handle is invalid OR program number is NULL.
 * @retval RMF_SI_NOT_FOUND Returned when SI info is not found.
 */
rmf_Error rmf_OobSiCache::GetProgramNumberForServiceDetailsHandle(
        rmf_SiServiceDetailsHandle handle, uint32_t* progNum)
{
    return GetProgramNumberForServiceHandle((rmf_SiServiceHandle) handle,
            progNum);
}

/**
 * @brief This function retrieves the modulation format for the given service details handle.
 *
 * @param[in] handle SI handle to get the modeformat from.
 * @param[out] modeFormat Output parameter to populate the mode format.
 *
 * @retval RMF_SI_SUCCESS Returned when successfully acquired the modulation format.
 * @retval RMF_SI_INVALID_PARAMETER Returned if service handle is invalid OR modulation mode is NULL.
 * @retval RMF_SI_NOT_FOUND Returned when SI info is not found and modulation mode is unknown.
 */
rmf_Error rmf_OobSiCache::GetModulationFormatForServiceDetailsHandle(
        rmf_SiServiceDetailsHandle handle, rmf_ModulationMode* modFormat)
{
    return GetModulationFormatForServiceHandle(
            (rmf_SiServiceHandle) handle, modFormat);
}

/**
 * @brief This function retrieves the number of language variants for the service long name for the given ServiceDetailsHandle.
 *
 * @param[in] handle SI handle to get the number of service names from.
 * @param[out] num_long_names Returns the number of long names owned by the service.
 *
 * @retval RMF_SI_SUCCESS Returned when successfully acquired the number of service long name language variants.
 * @retval RMF_SI_INVALID_PARAMETER Returned if service handle is invalid OR number of long names is NULL.
 */
rmf_Error rmf_OobSiCache::GetNumberOfLongNamesForServiceDetailsHandle(
        rmf_SiServiceDetailsHandle handle, uint32_t *num_long_names)
{
    return GetNumberOfLongNamesForServiceHandle(
            (rmf_SiServiceHandle) handle, num_long_names);
}

/**
 * @brief This function retrieves the long service names and associated ISO639 langauges for the given ServiceDetailsHandle.
 *
 * @param[in] handle SI handle to get the service name from.
 * @param[out] languages Output parameter to populate the long service name languages.
 * @param[out] longNames Output parameter to populate the long service name.
 *
 * @retval RMF_SI_SUCCESS Returned when successfully acquired the long service names.
 * @retval RMF_SI_INVALID_PARAMETER Returned if service handle is invalid OR language and service long names is NULL.
 */
rmf_Error rmf_OobSiCache::GetLongNamesForServiceDetailsHandle(
        rmf_SiServiceDetailsHandle handle, char* languages[], char* longNames[])
{
    return GetLongNamesForServiceHandle((rmf_SiServiceHandle) handle,
            languages, longNames);
}

/**
 * @brief This function retrieves the service details last update time for the given ServiceDetailsHandle.
 *
 * @param[in] handle SI handle to get the service details last update time.
 * @param[out] pTime Output parameter to populate the service details last update time.
 *
 * @retval RMF_SI_SUCCESS Returned when successfully acquired the service details last update time.
 * @retval RMF_SI_INVALID_PARAMETER Returned if service handle is invalid OR pTime is NULL.
 */
rmf_Error rmf_OobSiCache::GetServiceDetailsLastUpdateTimeForServiceDetailsHandle(
        rmf_SiServiceDetailsHandle handle, rmf_osal_TimeMillis *pTime)
{
    rmf_SiTableEntry *si_entry = NULL;
    rmf_SiServiceHandle service_handle = 0;

    /* Parameter check */
    if ((handle == RMF_SI_INVALID_HANDLE) || (pTime == NULL))
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    service_handle = (rmf_SiServiceHandle) handle;

    si_entry = (rmf_SiTableEntry *) service_handle;

    *pTime = si_entry->ptime_service;

    return RMF_SI_SUCCESS;
}

/**
 * @brief This function retrieves the service type for the given ServiceDetailsHandle.
 * Service type represents such as "digital television",
 * "digital radio", "NVOD reference service", "NVOD time-shifted service",
 * "analog television", "analog radio", "data broadcast" and "application".
 *
 * @param[in] service_handle Service_handle represents a SCTE service/channel.
 * @param[out] type  Output parameter to populate the service type.
 *
 * @retval RMF_SI_SUCCESS Returned When successfully acquired the service type.
 * @retval RMF_SI_INVALID_PARAMETER Returned if service handle is invalid OR service type is NULL.
 */
rmf_Error rmf_OobSiCache::GetServiceTypeForServiceDetailsHandle(
        rmf_SiServiceDetailsHandle handle, rmf_SiServiceType* type)
{
    return GetServiceTypeForServiceHandle((rmf_SiServiceHandle) handle,
            type);
}

/**
 * @brief This function retrieves the delivery system types such as cable, satellite and terrestial
 * for the given ServiceDetailsHandle.
 *
 * @param[in] handle SI handle to get the delivery system type.
 * @param[out] type Output parameter to populate the delivery system type.
 *
 * @retval RMF_SI_SUCCESS Returned when successfully acquired the delivery system type.
 * @retval RMF_SI_INVALID_PARAMETER Returned if service handle is invalid OR delivery system type is NULL.
 */
rmf_Error rmf_OobSiCache::GetDeliverySystemTypeForServiceDetailsHandle(
        rmf_SiServiceDetailsHandle handle, rmf_SiDeliverySystemType* type)
{
    return GetDeliverySystemTypeForServiceHandle(
            (rmf_SiServiceHandle) handle, type);
}

/**
 * @brief This function retrieves the service information types such ad SCTE, DVB for the given ServiceDetailsHandle.
 *
 * @param[in] handle SI handle to get the service information type from.
 * @param[out] type Output parameter to populate the service information type.
 *
 * @retval RMF_SI_SUCCESS Returned when successfully acquired the service information type.
 * @retval RMF_SI_INVALID_PARAMETER Returned if service handle is invalid OR service information type is NULL>
 */
rmf_Error rmf_OobSiCache::GetServiceInformationTypeForServiceDetailsHandle(
        rmf_SiServiceDetailsHandle handle, rmf_SiServiceInformationType* type)
{
    /* Parameter check */
    if ((handle == RMF_SI_INVALID_HANDLE) || (type == NULL))
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    *type = RMF_SI_SERVICE_INFORMATION_TYPE_SCTE_SI;

    return RMF_SI_SUCCESS;
}

/**
 * @brief This function retrieves the CA systemId Array length for the given ServiceDetailsHandle.
 * CA_System_ID values are allocated to Conditional Access system vendors to identify CA sytems within the
 * application area of the DVB-SI standard (EN 300 468), by insertion in the CA_System_ID field.
 *
 * @param[in] handle SI handle to get CA systemId Array length from.
 * @param[out] length Output parameter to populate the length.
 *
 * @retval RMF_SI_SUCCESS Returned when successfully acquired the CAsystemId Array length.
 * @retval RMF_SI_INVALID_PARAMETER Returned if service handle is invalid OR length is NULL.
 */
rmf_Error rmf_OobSiCache::GetCASystemIdArrayLengthForServiceDetailsHandle(
        rmf_SiServiceDetailsHandle handle, uint32_t* length)
{
    if ((handle == RMF_SI_INVALID_HANDLE) || (length == NULL))
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    *length = 0;

    return RMF_SI_SUCCESS;
}

/**
 * @brief This function retrieves the CA systemId array for the given ServiceDetailsHandle.
 * CA_System_ID values are allocated to Conditional Access system vendors to identify CA sytems within the
 * application area of the DVB-SI standard (EN 300 468), by insertion in the CA_System_ID field.
 *
 * @param[in] handle SI handle to get CA systemId Array from.
 * @param[out] length Output parameter to populate the length.
 * @param[out[ ca_array Output parameter to populate the ca_array.
 *
 * @retval RMF_SI_SUCCESS Returned when successfully acquired the CAsystemId Array.
 * @retval RMF_SI_INVALID_PARAMETER Returned if service handle is invalid OR length is NULL.
 */
rmf_Error rmf_OobSiCache::GetCASystemIdArrayForServiceDetailsHandle(
        rmf_SiServiceDetailsHandle handle, uint32_t* ca_array, uint32_t length)
{
    if ((handle == RMF_SI_INVALID_HANDLE) || (ca_array == NULL)
            || (length == 0))
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    *ca_array = 0;
    return RMF_SI_SUCCESS;
}

#if 0
rmf_Error rmf_OobSiCache::GetTransportStreamHandleForServiceDetailsHandle(
        rmf_SiServiceDetailsHandle handle,
        rmf_SiTransportStreamHandle *ts_handle)
{
    rmf_SiTableEntry *si_entry = NULL;

    /* Parameter check */
    if ((handle == RMF_SI_INVALID_HANDLE) || (ts_handle == NULL))
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    /*
     * Service details and service entries are the
     * same thing.
     */
    si_entry = (rmf_SiTableEntry *) handle;

    *ts_handle = si_entry->ts_handle;

    return RMF_SI_SUCCESS;
}
#endif

/**
 * @brief This function retrieves the service handle for the given ServiceDetailsHandle.
 * MPE[Multi protocol Encapsulation] defined in SCTE which is used for the encapsulation of IP datagrams using MPEG-2 TS packets.
 * This layer does not differentiate between service handle and service details handle.
 *
 * @param[in] handle Service details handle to get the service handle from.
 * @param[out] hanlde Output parameter to populate the service handle.
 *
 * @retval RMF_SI_SUCCESS returned when successfully acquired the service handle.
 */
rmf_Error rmf_OobSiCache::GetServiceHandleForServiceDetailsHandle(
        rmf_SiServiceDetailsHandle handle, rmf_SiServiceHandle* serviceHandle)
{
    /* MPE layer does not differentiate between service handle and service details handle */
    *serviceHandle = (rmf_SiServiceHandle) handle;

    /*  Increment the ref_count for service handle? */

    return RMF_SI_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////////////
/*
 Rating Dimension specific

 */
///////////////////////////////////////////////////////////////////////////////////////////////
rmf_Error rmf_OobSiCache::GetNumberOfSupportedRatingDimensions(uint32_t *num)
{
    if (num == NULL)
        return RMF_SI_INVALID_PARAMETER;

    /* Temp hard coded per CEA 776A April 2001 pg 2 */
    *num = (uint32_t) g_RRT_US.dimensions_defined;

    return RMF_SI_SUCCESS;
}

rmf_Error rmf_OobSiCache::GetSupportedRatingDimensions(rmf_SiRatingDimensionHandle arr[])
{
    uint8_t dim_index = 0;

    if (arr == NULL)
        return RMF_SI_INVALID_PARAMETER;

    for (dim_index = 0; dim_index < g_RRT_US.dimensions_defined; dim_index++)
    {
        arr[dim_index] = (rmf_SiRatingDimensionHandle)
                & g_RRT_US.dimensions[dim_index];
    }

    return RMF_SI_SUCCESS;
}

rmf_Error rmf_OobSiCache::GetNumberOfLevelsForRatingDimensionHandle(
        rmf_SiRatingDimensionHandle handle, uint32_t* length)
{
    rmf_SiRatingDimension *dimension = NULL;

    if (handle == RMF_SI_INVALID_HANDLE || length == NULL)
        return RMF_SI_INVALID_PARAMETER;

    dimension = (rmf_SiRatingDimension*) handle;
    *length = (uint32_t) dimension->values_defined;

    return RMF_SI_SUCCESS;
}

rmf_Error rmf_OobSiCache::GetNumberOfNamesForRatingDimensionHandle(
        rmf_SiRatingDimensionHandle handle, uint32_t *num_names)
{
    uint32_t count = 0;
    rmf_SiRatingDimension *dimension = NULL;
    rmf_SiLangSpecificStringList *walker = NULL;

    if (handle == RMF_SI_INVALID_HANDLE || num_names == NULL)
        return RMF_SI_INVALID_PARAMETER;

    RDK_LOG(
            RDK_LOG_TRACE1,
            "LOG.RDK.SI",
            "<GetNumberOfNamesForRatingDimensionHandle> rating dimension handle:0x%x\n",
            handle);

    dimension = (rmf_SiRatingDimension*) handle;

    // Count the entries
    for (walker = dimension->dimension_names; walker != NULL; walker
            = walker->next, count++)
        ;

    *num_names = count;
    return RMF_SI_SUCCESS;
}

rmf_Error rmf_OobSiCache::GetNamesForRatingDimensionHandle(
        rmf_SiRatingDimensionHandle handle, char* languages[],
        char* dimensionNames[])
{
    int i = 0;
    rmf_SiRatingDimension *dimension = NULL;
    rmf_SiLangSpecificStringList *walker = NULL;

    if (handle == RMF_SI_INVALID_HANDLE || languages == NULL || dimensionNames
            == NULL)
        return RMF_SI_INVALID_PARAMETER;

    dimension = (rmf_SiRatingDimension*) handle;

    for (walker = dimension->dimension_names; walker != NULL; walker
            = walker->next, i++)
    {
        languages[i] = walker->language;
        dimensionNames[i] = walker->string;

        RDK_LOG(
                RDK_LOG_TRACE1,
                "LOG.RDK.SI",
                "<GetNamesForServiceHandle> source_name:%s, language:%s\n",
                walker->string, walker->language);
    }

    return RMF_SI_SUCCESS;
}

rmf_Error rmf_OobSiCache::GetNumberOfNamesForRatingDimensionHandleAndLevel(
        rmf_SiRatingDimensionHandle handle, uint32_t *num_names,
        uint32_t *num_descriptions, int levelNumber)
{
    uint32_t count = 0;
    rmf_SiRatingDimension *dimension = NULL;
    rmf_SiRatingValuesDefined *ratingValues = NULL;
    rmf_SiLangSpecificStringList *walker = NULL;

    if (handle == RMF_SI_INVALID_HANDLE || num_names == NULL
            || num_descriptions == NULL)
        return RMF_SI_INVALID_PARAMETER;

    RDK_LOG(
            RDK_LOG_TRACE1,
            "LOG.RDK.SI",
            "<GetNumberOfNamesForRatingDimensionHandle> rating dimension handle:0x%x\n",
            handle);

    dimension = (rmf_SiRatingDimension*) handle;
    ratingValues = &(dimension->rating_value[levelNumber]);

    // Count the entries
    for (walker = ratingValues->abbre_rating_value_text; walker != NULL; walker
            = walker->next, count++)
        ;
    *num_names = count;

    count = 0;

    for (walker = ratingValues->rating_value_text; walker != NULL; walker
            = walker->next, count++)
        ;
    *num_descriptions = count;

    return RMF_SI_SUCCESS;
}

rmf_Error rmf_OobSiCache::GetDimensionInformationForRatingDimensionHandleAndLevel(
        rmf_SiRatingDimensionHandle handle, char* nameLanguages[],
        char* names[], char* descriptionLanguages[], char* descriptions[],
        int levelNumber)
{
    rmf_SiRatingDimension *dimension = NULL;
    rmf_SiRatingValuesDefined *ratingValues = NULL;
    rmf_SiLangSpecificStringList *walker = NULL;
    int i = 0;

    RDK_LOG(
            RDK_LOG_TRACE1,
            "LOG.RDK.SI",
            "<GetDimensionInformationForRatingDimensionHandleAndLevel> service_handle:0x%x, level:%d\n",
            handle, levelNumber);

    /* Parameter check */
    if (handle == RMF_SI_INVALID_HANDLE || nameLanguages == NULL || names
            == NULL || descriptionLanguages == NULL || descriptions == NULL)
        return RMF_SI_INVALID_PARAMETER;

    dimension = (rmf_SiRatingDimension*) handle;
    ratingValues = &(dimension->rating_value[levelNumber]);

    // Names
    for (walker = ratingValues->abbre_rating_value_text; walker != NULL; walker
            = walker->next, i++)
    {
        nameLanguages[i] = walker->language;
        names[i] = walker->string;

        RDK_LOG(
                RDK_LOG_TRACE1,
                "LOG.RDK.SI",
                "<GetDimensionInformationForRatingDimensionHandleAndLevel> name:%s, language:%s\n",
                walker->string, walker->language);
    }

    i = 0;

    // Descriptions
    for (walker = ratingValues->rating_value_text; walker != NULL; walker
            = walker->next, i++)
    {
        descriptionLanguages[i] = walker->language;
        descriptions[i] = walker->string;

        RDK_LOG(
                RDK_LOG_TRACE1,
                "LOG.RDK.SI",
                "<GetDimensionInformationForRatingDimensionHandleAndLevel> name:%s, language:%s\n",
                walker->string, walker->language);
    }

    return RMF_SI_SUCCESS;
}

rmf_Error rmf_OobSiCache::GetRatingDimensionHandleByName(char* dimensionName,
        rmf_SiRatingDimensionHandle* outHandle)
{
    uint8_t dim_index = 0;

    if (dimensionName == NULL || outHandle == NULL)
        return RMF_SI_INVALID_PARAMETER;

    for (dim_index = 0; dim_index < g_RRT_US.dimensions_defined; dim_index++)
    {
        rmf_SiLangSpecificStringList *walker =
                g_RRT_US.dimensions[dim_index].dimension_names;

        while (walker != NULL)
        {
            if (strcmp(walker->string, dimensionName) == 0)
            {
                *outHandle = (rmf_SiRatingDimensionHandle)
                        & g_RRT_US.dimensions[dim_index];
                return RMF_SI_SUCCESS;
            }

            walker = walker->next;
        }
    }

    return RMF_SI_NOT_FOUND;
}

/**
 * Retrieve number of services
 *
 * <i>GetTotalNumberOfServices()</i>
 *
 * @param num_services is the output param to populate number of services
 *
 * @return RMF_SI_SUCCESS if successful, else
 *          return appropriate rmf_Error
 *
 */
rmf_Error rmf_OobSiCache::GetTotalNumberOfServices(uint32_t *num_services)
{
    rmf_SiTableEntry *walker = NULL;
    uint32_t num = 0;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<GetTotalNumberOfServices> \n");
    /* Parameter check */
    if (num_services == NULL)
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    // If SI DB is in either of these states return SI_NOT_AVAILABLE_YET
    // SI_ACQUIRING state is not useful for callers
    if ((g_si_state == SI_ACQUIRING) || (g_si_state == SI_NOT_AVAILABLE_YET))
        return RMF_SI_NOT_AVAILABLE_YET;
    else if (g_si_state == SI_DISABLED)
        return RMF_SI_NOT_AVAILABLE;

    walker = g_si_entry;

    while (walker)
    {
//      rmf_SiTransportStreamEntry *ts =
//              (rmf_SiTransportStreamEntry *) walker->ts_handle;
        if ((walker->channel_type == CHANNEL_TYPE_NORMAL)
                && ((walker->state == SIENTRY_MAPPED) || (walker->state
                        == SIENTRY_DEFINED_MAPPED))
#if 0
 && ((ts == NULL)
                || ((ts->frequency != RMF_SI_OOB_FREQUENCY) && (ts->frequency != RMF_SI_DSG_FREQUENCY) && (ts->frequency != RMF_SI_HN_FREQUENCY)))
#endif
)
        {
            num++;
        }
        walker = walker->next;
    }

     RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<GetTotalNumberOfServices>: num: %d \n", num);
    *num_services = num;

    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI",
            "<GetTotalNumberOfServices> num_services:%d\n", *num_services);

    return RMF_SI_SUCCESS;
}

/**
 * Retrieve all the services
 *
 * <i>GetAllServices()</i>
 *
 * @param array_services is the output param to populate services
 * @param num_services Input param: has size of array_services.
 * Output param: the actual number of service handles placed in array_services
 *
 * @return RMF_SI_SUCCESS if successful, else
 *          return appropriate rmf_Error
 *
 */
rmf_Error rmf_OobSiCache::GetAllServices(rmf_SiServiceHandle array_services[],
        uint32_t* num_services)
{
    rmf_Error err = RMF_SI_SUCCESS;
    rmf_SiTableEntry *walker = NULL;
    uint32_t i = 0;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<GetAllServices> \n");

    /* Parameter check */
    if (array_services == NULL || num_services == NULL || *num_services == 0)
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    // If SI DB is in either of these states return SI_NOT_AVAILABLE_YET
    // SI_ACQUIRING state is not useful for callers
    if ((g_si_state == SI_ACQUIRING) || (g_si_state == SI_NOT_AVAILABLE_YET))
        return RMF_SI_NOT_AVAILABLE_YET;
    else if (g_si_state == SI_DISABLED)
        return RMF_SI_NOT_AVAILABLE;

    walker = g_si_entry;

    while (walker)
    {
//      rmf_SiTransportStreamEntry *ts =
//              (rmf_SiTransportStreamEntry *) walker->ts_handle;
        if ((walker->channel_type == CHANNEL_TYPE_NORMAL)
                && ((walker->state == SIENTRY_MAPPED) || (walker->state
                        == SIENTRY_DEFINED_MAPPED))
#if 0
                 && ((ts == NULL)
                || ((ts->frequency != RMF_SI_OOB_FREQUENCY) && (ts->frequency != RMF_SI_DSG_FREQUENCY) && (ts->frequency != RMF_SI_HN_FREQUENCY)))
#endif
)
        {
            //RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "GetAllServices()...channel type is NORMAL 0x%x \n", walker->source_id);
            if (i == *num_services)
            {
                RDK_LOG(
                        RDK_LOG_ERROR,
                        "LOG.RDK.SI",
                        "<GetAllServices> array_services (%d) too small for list\n",
                        *num_services);
                err = RMF_OSAL_ENOMEM;
                break;
            }

            walker->ref_count++;
            array_services[i++] = (rmf_SiServiceHandle) walker;
        }
        else
        {
            //RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "GetAllServices()...channel type is not NORMAL 0x%x \n", walker->source_id);
        }
        walker = walker->next;
    }

    *num_services = i;

    return err;
}

#if 0
/**
 * Release the service component handle
 *
 * <i>mpe_siReleaseServiceComponentHandle()</i>
 *
 * @param comp_handle component handle
 *
 * @return RMF_SI_SUCCESS if successful, else
 *          return appropriate rmf_Error
 *
 */
rmf_Error rmf_OobSiCache::ReleaseServiceComponentHandle(
        rmf_SiServiceComponentHandle comp_handle)
{
    rmf_SiElementaryStreamList *elem_stream = NULL;

    /* Parameter check */
    if (comp_handle == RMF_SI_INVALID_HANDLE)
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    elem_stream = (rmf_SiElementaryStreamList *) comp_handle;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<mpe_siReleaseServiceComponentHandle> ref_count %d\n",
            elem_stream->ref_count);

    if (elem_stream->ref_count <= 1)
    {
        // What does this mean?
        // ref_count should never be less than 1, it means that
        // there may be un-balanced get/release service component
        // handles
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                "<mpe_siReleaseServiceComponentHandle> error ref_count %d\n",
                elem_stream->ref_count);
        return RMF_SI_INVALID_PARAMETER;
    }

    --(elem_stream->ref_count);

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<mpe_siReleaseServiceComponentHandle> ref_count %d\n",
            elem_stream->ref_count);

    if ((elem_stream->ref_count == 1) && (elem_stream->valid == FALSE))
    {
        rmf_SiTableEntry *si_entry = (rmf_SiTableEntry *) elem_stream->service_reference;
        rmf_SiProgramInfo *pi = si_entry->program;
        if (remove_elementaryStream(&(pi->elementary_streams), elem_stream) == RMF_SI_SUCCESS) 
        {
            (pi->number_elem_streams)--;
        }
        delete_elementary_stream(elem_stream);
    }

    return RMF_SI_SUCCESS;
}
#endif

rmf_Error rmf_OobSiCache::GetNumberOfServiceEntriesForSourceId(uint32_t sourceId,
        uint32_t *num_entries)
{
    rmf_SiTableEntry *walker = NULL;
    rmf_Error retVal = RMF_SI_SUCCESS;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<GetNumberOfServiceEntriesForSourceId> sourceId:0x%x\n",
            sourceId);

    if ((num_entries == NULL) || ((sourceId == 0) || (sourceId > 65535)))
    {
        retVal = RMF_SI_INVALID_PARAMETER;
    }
    else
    {
        *num_entries = 0;

        walker = g_si_entry;

        while (walker)
        {
            if (walker->source_id == sourceId)
            {
                (*num_entries)++;
            }
            walker = walker->next;
        }
        RDK_LOG(
                RDK_LOG_TRACE1,
                "LOG.RDK.SI",
                "<GetNumberOfServiceEntriesForSourceId> num_entries:%d\n",
                *num_entries);
    }

    return retVal;
}

rmf_Error rmf_OobSiCache::GetAllServiceHandlesForSourceId(uint32_t sourceId,
        rmf_SiServiceHandle service_handle[],
        uint32_t* num_entries)
{
    rmf_SiTableEntry *walker = NULL;
    rmf_Error retVal = RMF_SI_SUCCESS;
    uint32_t i=0;

    if ((num_entries == NULL) || (service_handle == NULL)
            || ((sourceId == 0) || (sourceId > 65535)))
    {
        retVal = RMF_SI_INVALID_PARAMETER;
    }
    else
    {
        walker = g_si_entry;

        while (walker)
        {
            if(walker->source_id == sourceId)
            {
                if (i >= *num_entries)
                {
                    // If we filled up the array, just increment the count to find the total
                    i++;
                }
                else
                {
                    service_handle[i++] = (rmf_SiServiceHandle) walker;
                    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                            "<GetAllServiceHandlesForSourceId> handle:0x%p\n", walker);
                }
            }
            walker = walker->next;
        }
    }

    if( num_entries != NULL && i > *num_entries)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI",
                "<GetAllServiceHandlesForSourceId> array size: %d\n", i);
    }
    if(num_entries != NULL)
    {
       *num_entries = i;
    }

    return retVal;
}

/* Methods called by Table Parser */
/**
 *  This method is called when SVCT is acquired to populated sourceIds.
 *  If an SI entry does not exist this method creates a new entry and appends
 *  it to existing list.
 *
 *  Retrieve service entry handle given a source Id
 *
 * <i>GetServiceEntryFromSourceId()</i>
 *
 * @param sourceId is the input param to obtain the service entry from
 * @param si_entry_handle is the output param to populate the service handle
 *
 * @return RMF_SI_SUCCESS if successful, else
 *          return appropriate rmf_Error
 *
 */
/* 
 When new SVCT is acquired, after updating existing entries, walk the linked list and
 remove stale entries. That is, if any sourceIds are removed from SVCT then remove them from
 SI DB also.
 Currently we are not doing this but should be done as part of optimization.
 */
rmf_Error rmf_OobSiCache::GetServiceEntryFromSourceId(uint32_t sourceId,
        rmf_SiServiceHandle *service_handle)
{
    rmf_SiTableEntry *prevWalker = NULL;
    rmf_SiTableEntry *walker = NULL; 
    rmf_Error retVal = RMF_SI_SUCCESS;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<GetServiceEntryFromSourceId> sourceId:0x%x\n", sourceId);

    if ((service_handle == NULL) || ((sourceId == 0) || (sourceId > 65535)))
    {
        retVal = RMF_SI_INVALID_PARAMETER;
    }
    else
    {
        *service_handle = RMF_SI_INVALID_HANDLE;

        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<GetServiceEntryFromSourceId> g_si_entry: 0x%x\n", g_si_entry);
        
        walker = prevWalker = g_si_entry;

        while (walker)
        {
                RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<GetServiceEntryFromSourceId> walker: 0x%x, walker->sourceId:0x%x, sourceId:0x%x\n", walker, walker->source_id, sourceId);
            if (walker->source_id == sourceId)
            {
                *service_handle = (rmf_SiServiceHandle) walker;
                break;
            }
            prevWalker = walker;
            walker = walker->next;
        }
        if (walker == NULL)
        {
            rmf_osal_memAllocP(RMF_OSAL_MEM_SI_CACHE_OOB, sizeof(rmf_SiTableEntry),
                    (void **) &(walker));
            if (walker == NULL)
            {
                RDK_LOG(
                        RDK_LOG_WARN,
                        "LOG.RDK.SI",
                        "<GetServiceEntryFromSourceId> Mem allocation failed, returning RMF_SI_OUT_OF_MEM...\n");
                retVal = RMF_SI_OUT_OF_MEM;
            }
            else
            {
                init_si_entry(walker);
                walker->source_id = sourceId;
                *service_handle = (rmf_SiServiceHandle) walker;
                if (g_si_entry == NULL)
                {
                    g_si_entry = walker;
                }
                else
                {
                    //RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<GetServiceEntryFromSourceId> set %p->next to %p\n", prevWalker, walker);
                    prevWalker->next = walker;
                }
                /* Increment the number of services */
                g_numberOfServices++;
            }

        }

        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                "<GetServiceEntryFromSourceId> service_handle :0x%x\n",
                *service_handle);
    }

    return retVal;
}

#if 0
rmf_Error rmf_OobSiCache::GetServiceEntryFromAppIdProgramNumber(uint32_t appId,
        uint32_t prog_num, rmf_SiServiceHandle *service_handle)
{
    rmf_SiTableEntry *walker = NULL;
    rmf_Error retVal = RMF_SI_SUCCESS;
    int found = 0;

    //RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<GetServiceEntryFromAppIdProgramNumber> appId:0x%x\n", appId );

    if (service_handle == NULL || appId == 0)
    {
        retVal = RMF_SI_INVALID_PARAMETER;
    }
    else
    {
        *service_handle = RMF_SI_INVALID_HANDLE;
        walker = g_si_entry;
        while (walker)
        {
            if ((walker->isAppType) && (walker->app_id == appId))
            {
                if ((walker->program != NULL)
                        && ((walker->program_number
                                == PROGRAM_NUMBER_UNKNOWN)
                                || (walker->program->program_number == prog_num)))
                {
                    walker->program_number = prog_num;
                    *service_handle = (rmf_SiServiceHandle) walker;
                    found = 1;
                    break;
                }
            }
            walker = walker->next;
        }
    }

    /* Matching sourceId is not found, return error */
    if (!found)
    {
        retVal = RMF_SI_NOT_FOUND;
    }

    return retVal;
}

rmf_Error rmf_OobSiCache::GetServiceEntryFromHNstreamProgramNumber(mpe_HnStreamSession session,
        uint32_t prog_num, rmf_SiServiceHandle *service_handle)
{
    rmf_SiTableEntry *walker = NULL;
    rmf_Error retVal = RMF_SI_SUCCESS;
    int found = 0;

    if (service_handle == NULL || session == 0)
    {
        retVal = RMF_SI_INVALID_PARAMETER;
    }
    else
    {
        *service_handle = RMF_SI_INVALID_HANDLE;
        walker = g_si_entry;
        while (walker)
        {
            if (walker->hn_stream_session == session)
            {
                if ((walker->program != NULL)
                        && ((walker->program_number
                                == PROGRAM_NUMBER_UNKNOWN)
                                || (walker->program_number == prog_num)))
                {
                    walker->program->program_number = prog_num;
                    *service_handle = (rmf_SiServiceHandle) walker;
                    found = 1;
                    break;
                }
            }
            walker = walker->next;
        }
    }

    /* Matching sourceId is not found, return error */
    if (!found)
    {
        retVal = RMF_SI_NOT_FOUND;
    }

    return retVal;
}
#endif

#if 0
/**
 *  This method is called when SVCT is acquired to populated sourceIds.
 *  If an SI entry does not exist this method creates a new entry and appends
 *  it to existing list.
 *
 *  Retrieve service entry handle given a source Id
 *
 * <i>GetServiceEntryFromSourceId()</i>
 *
 * @param sourceId is the input param to obtain the service entry from
 * @param si_entry_handle is the output param to populate the service handle
 *
 * @return RMF_SI_SUCCESS if successful, else
 *          return appropriate rmf_Error
 *
 */
/* 
 When new SVCT is acquired, after updating existing entries, walk the linked list and
 remove stale entries. That is, if any sourceIds are removed from SVCT then remove them from
 SI DB also.
 Currently we are not doing this but should be done as part of optimization.
 */
rmf_Error rmf_OobSiCache::GetServiceEntryFromSourceIdAndChannel(uint32_t sourceId,
        uint32_t major_number, uint32_t minor_number,
        rmf_SiServiceHandle *service_handle, rmf_SiTransportHandle ts_handle,
        rmf_SiProgramHandle pi_handle)
{
    rmf_SiTableEntry *prevWalker = NULL;
    rmf_SiTableEntry *walker = NULL;
    rmf_Error retVal = RMF_SI_SUCCESS;

    RDK_LOG(
            RDK_LOG_TRACE1,
            "LOG.RDK.SI",
            "<GetServiceEntryFromSourceIdAndChannel> sourceId:0x%x  major_number:%d minor_number: %d, ts_handle:0x%x, pi_handle: 0x%x\n",
            sourceId, major_number, minor_number, ts_handle, pi_handle);

    /* Parameter check */
    if ((service_handle == NULL) || (ts_handle == RMF_SI_INVALID_HANDLE)
            || ((((rmf_SiTransportStreamEntry *) ts_handle)->modulation_mode
                    != RMF_MODULATION_QAM_NTSC) && (pi_handle
                    == RMF_SI_INVALID_HANDLE)))
    {
        retVal = RMF_SI_INVALID_PARAMETER;
    }
    else
    {

        *service_handle = RMF_SI_INVALID_HANDLE;

        walker = prevWalker = g_si_entry;

        while (walker)
        {
            { // Find by channel #, which must be unique, if not 0.
                if ((walker->major_channel_number == major_number)
                        && (walker->minor_channel_number == minor_number)
                        && (walker->source_id == sourceId))
                {
                    *service_handle = (rmf_SiServiceHandle) walker;
                    break;
                }
            }
            prevWalker = walker;
            walker = walker->next;
        }

        if (walker != NULL)
        {
            if (walker->ts_handle != RMF_SI_INVALID_HANDLE)
            { // exists, or move?
                if (walker->ts_handle != ts_handle)
                { // In writer locked area, so no extra locking necessary.
                    LINK
                            *lp =
                                    llist_linkof(
                                            ((rmf_SiTransportStreamEntry *) walker->ts_handle)->services,
                                            (void *) walker);
                    if (lp != NULL)
                    {
                        llist_rmlink(lp);
                        ((rmf_SiTransportStreamEntry *) walker->ts_handle)->service_count--;
                        ((rmf_SiTransportStreamEntry *) walker->ts_handle)->visible_service_count--;
                    }
                    walker->ts_handle = RMF_SI_INVALID_HANDLE;
                    if (walker->program)
                    {
                        LINK *lp1 = llist_linkof(walker->program->services,
                                (void *) walker);
                        if (lp1 != NULL)
                        {
                            llist_rmlink(lp1);
                            walker->program->service_count--;
                        }
                        walker->program = NULL;
                    }
                }
                else
                { // Same TS, moved programs?
                    if (walker->program != NULL)
                    {
                        if (walker->program != (rmf_SiProgramInfo *) pi_handle)
                        {
                            LINK *lp = llist_linkof(walker->program->services,
                                    (void *) walker);
                            llist_rmlink(lp);
                            walker->program->service_count--;
                            walker->program = NULL;
                        }
                    }
                }
            }
            if (walker->ts_handle == RMF_SI_INVALID_HANDLE)
            {
                LINK *lp;
                walker->ts_handle = ts_handle;
                lp = llist_mklink((void *) walker);
                llist_append(
                        ((rmf_SiTransportStreamEntry *) ts_handle)->services,
                        lp);
                ((rmf_SiTransportStreamEntry *) walker->ts_handle)->service_count++;
                ((rmf_SiTransportStreamEntry *) walker->ts_handle)->visible_service_count++;
            }
            if (pi_handle != RMF_SI_INVALID_HANDLE)
            {
                LINK *lp;
                walker->program = (rmf_SiProgramInfo *) pi_handle;
                lp = llist_mklink((void *) walker);
                llist_append(walker->program->services, lp);
                walker->program->service_count++;
                RDK_LOG(
                        RDK_LOG_TRACE1,
                        "LOG.RDK.SI",
                        "<GetServiceEntryFromSourceIdAndChannel> program->service_count %d...\n",
                        walker->program->service_count);
            }
        }
        else
        /* If si entry is not found, create a new one append it to list */
        {
            rmf_osal_memAllocP(RMF_OSAL_MEM_SI_CACHE_OOB, sizeof(rmf_SiTableEntry),
                    (void **) &(walker));
            if (walker == NULL)
            {
                RDK_LOG(
                        RDK_LOG_WARN,
                        "LOG.RDK.SI",
                        "<GetServiceEntryFromSourceIdAndChannel> Mem allocation failed, returning RMF_SI_OUT_OF_MEM...\n");
                retVal = RMF_SI_OUT_OF_MEM;
            }
            else
            {
                LINK *lp;
                init_si_entry(walker);
                walker->source_id = sourceId;
                walker->major_channel_number = major_number;
                walker->minor_channel_number = minor_number;
                *service_handle = (rmf_SiServiceHandle) walker;
                if (g_si_entry == NULL)
                {
                    g_si_entry = walker;
                }
                else
                {
                    RDK_LOG(
                            RDK_LOG_TRACE1,
                            "LOG.RDK.SI",
                            "<GetServiceEntryFromSourceIdAndChannel> set %p->next to %p\n",
                            prevWalker, walker);
                    prevWalker->next = walker;
                }
                if (pi_handle != RMF_SI_INVALID_HANDLE)
                {
                    LINK *lp1;
                    walker->program = (rmf_SiProgramInfo *) pi_handle;
                    lp1 = llist_mklink((void *) walker);
                    llist_append(walker->program->services, lp1);
                    walker->program->service_count++;
                    RDK_LOG(
                            RDK_LOG_TRACE1,
                            "LOG.RDK.SI",
                            "<GetServiceEntryFromSourceIdAndChannel> program->service_count %d...\n",
                            walker->program->service_count);
                }
                walker->ts_handle = ts_handle;
                lp = llist_mklink((void *) walker);
                llist_append(
                        ((rmf_SiTransportStreamEntry *) ts_handle)->services,
                        lp);
                ((rmf_SiTransportStreamEntry *) walker->ts_handle)->service_count++;
                RDK_LOG(
                        RDK_LOG_TRACE1,
                        "LOG.RDK.SI",
                        "<GetServiceEntryFromSourceIdAndChannel> ((rmf_SiTransportStreamEntry *)walker->ts_handle)->service_count %d...\n",
                        ((rmf_SiTransportStreamEntry *) walker->ts_handle)->service_count);
                /* Increment the number of services */
                g_numberOfServices++;
                RDK_LOG(
                        RDK_LOG_TRACE1,
                        "LOG.RDK.SI",
                        "<GetServiceEntryFromSourceIdAndChannel> g_numberOfServices %d...\n",
                        g_numberOfServices);
            }
        }
        RDK_LOG(
                RDK_LOG_TRACE1,
                "LOG.RDK.SI",
                "<GetServiceEntryFromSourceIdAndChannel> service_handle:0x%x\n",
                *service_handle);
    }
    return retVal;
}
#endif

/*
 * Look up a service entry based on virtual channel number (found in SVCT-DCM, SVCT-VCM)
 * This method creates a new entry if one is not found in the database.
 * This method is called during parsing of SVCT-DCM/SVCT-VCM sub-table(s).
 */
void rmf_OobSiCache::GetServiceEntryFromChannelNumber(uint16_t channel_number,
        rmf_SiServiceHandle *service_handle)
{
    rmf_SiTableEntry *prevWalker = NULL;
    rmf_SiTableEntry *walker = NULL;
   RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<GetServiceEntryFromChannelNumber> channel_number: %d\n", channel_number);

    if ((service_handle == NULL) || (channel_number > 4095))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "<GetServiceEntryFromChannelNumber> RMF_SI_INVALID_PARAMETER channel_number: %d\n", channel_number);
    }
    else
    {
        *service_handle = RMF_SI_INVALID_HANDLE;

        walker = prevWalker = g_si_entry;

        while (walker)
        {
            // If a service entry exists all the members are set to default values
            if (walker->virtual_channel_number == channel_number)
            {
                *service_handle = (rmf_SiServiceHandle) walker;
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI", "<GetServiceEntryFromChannelNumber> found entry for channel_number: %d\n", channel_number);
                break;
            }
            prevWalker = walker;
            walker = walker->next;
        }

        // Create a new entry and append to list if one is not found
        if (walker == NULL)
        {
            rmf_osal_memAllocP(RMF_OSAL_MEM_SI_CACHE_OOB, sizeof(rmf_SiTableEntry),
                    (void **) &(walker));
            if (walker == NULL)
            {
                RDK_LOG(RDK_LOG_FATAL, "LOG.RDK.SI",
                        "<GetServiceEntryFromChannelNumber> RMF_SI_OUT_OF_MEM\n");
            }
            else
            {
                //RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI", "<GetServiceEntryFromChannelNumber> creating new entry for channel_number: %d\n", channel_number);

                init_si_entry(walker);
                // Set the new channel number
                walker->virtual_channel_number = channel_number;
                *service_handle = (rmf_SiServiceHandle) walker;

                if (g_si_entry == NULL)
                {
                    g_si_entry = walker;
                }
                else
                {
                    prevWalker->next = walker;
                }
            }
        }
    }
}

void rmf_OobSiCache::InsertServiceEntryForChannelNumber(uint16_t channel_number,
        rmf_SiServiceHandle *service_handle)
{
    rmf_SiTableEntry *new_entry = NULL;
    rmf_SiTableEntry *walker = NULL;

    if ((service_handle == NULL) || (channel_number > 4095))
    {
        //RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "<mpe_siInsertServiceEntryForChannelNumber> RMF_SI_INVALID_PARAMETER channel_number: %d\n", channel_number);
    }
    else
    {
        *service_handle = RMF_SI_INVALID_HANDLE;

        rmf_osal_memAllocP(RMF_OSAL_MEM_SI_CACHE_OOB, sizeof(rmf_SiTableEntry),
                (void **) &(new_entry));
        if (new_entry == NULL)
        {
            RDK_LOG(RDK_LOG_FATAL, "LOG.RDK.SI",
                    "<mpe_siInsertServiceEntryForChannelNumber> RMF_SI_OUT_OF_MEM\n");
        }
        else
        {
            init_si_entry(new_entry);
            // Set the new channel number
            new_entry->virtual_channel_number = channel_number;
            *service_handle = (rmf_SiServiceHandle) new_entry;

            if (g_si_entry == NULL)
            {
                g_si_entry = new_entry;
            }
            else
            {
                walker = g_si_entry;
                while(walker)
                {
                    if(walker->next == NULL)
                    {
                        walker->next = new_entry;
                        break;
                    }
                    else
                    {
                        walker = walker->next;
                    }
                }
            }
        }
    }
}

/*
 * This method is called when tables are initially acquired or updated
 * to correctly map service entry's transport stream handle and
 * program info details as well as service name entry info.
 */
void rmf_OobSiCache::UpdateServiceEntries()
{
    rmf_SiTransportStreamHandle ts_handle = RMF_SI_INVALID_HANDLE;
    rmf_SiProgramHandle prog_handle = RMF_SI_INVALID_HANDLE;
    uint32_t frequency_by_cds_ref = 0;
    rmf_ModulationMode mode_by_mms_ref = RMF_MODULATION_UNKNOWN;
    rmf_SiTableEntry *walker = NULL;
    rmf_siSourceNameEntry *source_name_entry = NULL;
    
    // Lock should be acquired by the caller
    walker = g_si_entry;
    while (walker)
    {
        frequency_by_cds_ref = 0;
        mode_by_mms_ref = RMF_MODULATION_UNKNOWN;
        // For each service entry
        frequency_by_cds_ref = g_frequency[walker->freq_index];
        if (walker->transport_type == NON_MPEG_2_TRANSPORT)
        {
            mode_by_mms_ref = RMF_MODULATION_QAM_NTSC;
        }
        else
        {
            mode_by_mms_ref = g_mode[walker->mode_index];
        }
        RDK_LOG(RDK_LOG_TRACE6,
                "LOG.RDK.SI",
                "<%s> - frequency(%d) = (%d) modulation(%d) = %d.\n",
                __FUNCTION__, frequency_by_cds_ref, mode_by_mms_ref);
        if((frequency_by_cds_ref != 0)  && (mode_by_mms_ref != RMF_MODULATION_UNKNOWN))
        {
            m_pInbSiCache->GetTransportStreamEntryFromFrequencyModulation(frequency_by_cds_ref, mode_by_mms_ref, &ts_handle);
            (void) m_pInbSiCache->GetProgramEntryFromTransportStreamEntry(ts_handle, walker->program_number, &prog_handle);
            // Update the service entry with transport stream and program info
           UpdateServiceEntry((rmf_SiServiceHandle)walker, ts_handle, prog_handle);
        }
        //
        // Now, find the source name entry and update
        source_name_entry = NULL;
        GetSourceNameEntry(walker->isAppType ? walker->app_id : walker->source_id, walker->isAppType,
                                 &source_name_entry, FALSE);
        if (source_name_entry != NULL)
        {
          // If there is a pre-existing source_name_entry for this walker that means the name changed?
          // If source name info changed, signal RMF_SI_EVENT_SERVICE_DETAILS_UPDATE (MODIFY) event..??
          walker->source_name_entry = source_name_entry;
          source_name_entry->mapped = TRUE;
        }
        else
        {
          RDK_LOG(RDK_LOG_TRACE9, "LOG.RDK.SI",
                  "<%s::si_FinalizeSIAfterAcquisition> - ERROR SourceName not found\n",
                  SIMODULE);
        }
        walker = walker->next;
    }
    //
    // Go through the source name table and find all entries that have not been mapped
    source_name_entry = g_si_sourcename_entry;
    while(source_name_entry != NULL)
    {
      if (!source_name_entry->mapped)
      {
        rmf_SiServiceHandle si_database_handle;
        // Call the create DSG Service handle, but no need to pass the soure name stuff, we already have it.
        CreateDSGServiceHandle((uint32_t)source_name_entry->id, PROGRAM_NUMBER_UNKNOWN, NULL, NULL, &si_database_handle);
        // Set the newly returned database handle entry's source name entry
        if (si_database_handle != RMF_SI_INVALID_HANDLE)
        {
          ((rmf_SiTableEntry *)si_database_handle)->source_name_entry = source_name_entry;
          source_name_entry->mapped = TRUE;   // this source name now mapped to an si db entry
        }
      }
      source_name_entry = source_name_entry->next;
    }
}

/*
 * Update the given service entry with transport stream info and program info.
 * This method is called during parsing of SVCT-VCM sub-table.
 * */
void rmf_OobSiCache::UpdateServiceEntry(rmf_SiServiceHandle service_handle,
        rmf_SiTransportStreamHandle ts_handle, rmf_SiProgramHandle pi_handle)
{
    rmf_SiTableEntry * walker = NULL;
    rmf_SiTableEntry * si_entry = NULL;
    rmf_osal_Bool dupFound = false;

#if 0
#ifdef TEST_WITH_PODMGR    
#ifndef GENERATE_SI_CACHE_UTILITY
    rmf_OobSiMgr *pOobSiMgr = rmf_OobSiMgr::getInstance();
#endif
#endif
#endif


    /* Parameter check */
    if ((service_handle == RMF_SI_INVALID_HANDLE) || (ts_handle
            == RMF_SI_INVALID_HANDLE)
            || ((((rmf_SiTransportStreamEntry *) ts_handle)->modulation_mode
                    != RMF_MODULATION_QAM_NTSC) && (pi_handle
                    == RMF_SI_INVALID_HANDLE)))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI",
                "<%s> RMF_SI_INVALID_PARAMETER\n", __FUNCTION__);
    }
    else
    {
        walker = (rmf_SiTableEntry*) service_handle;

        if (walker != NULL)
        {
            if (walker->ts_handle != RMF_SI_INVALID_HANDLE)
            { // exists, or move?
                if (walker->ts_handle != ts_handle)
                { // In writer locked area, so no extra locking necessary.

                    if(is_service_entry_available(((rmf_SiTransportStreamEntry *) walker->ts_handle)->services,
                                            walker))
                    {
                        ((rmf_SiTransportStreamEntry *) walker->ts_handle)->services->remove((rmf_SiServiceHandle)walker);
                        ((rmf_SiTransportStreamEntry *) walker->ts_handle)->service_count--;
                        ((rmf_SiTransportStreamEntry *) walker->ts_handle)->visible_service_count--;
                    }
                
                     walker->ts_handle = RMF_SI_INVALID_HANDLE;
                    if (walker->program)
                    {
                        if (is_service_entry_available(((rmf_SiProgramInfo*)walker->program)->services,
                                walker))
                        {
                            ((rmf_SiProgramInfo*)walker->program)->services->remove((rmf_SiServiceHandle)walker);
                            ((rmf_SiProgramInfo*)walker->program)->service_count--;
                        }
                        walker->program = RMF_SI_INVALID_HANDLE;
                    }

//#ifdef TEST_WITH_PODMGR                    
//#ifndef GENERATE_SI_CACHE_UTILITY 
                    // Signal ServiceDetails update event (MODIFY)
                    if(m_pSiEventListener != NULL)
                    {
                        RDK_LOG(
                                RDK_LOG_INFO,
                                "LOG.RDK.SI",
                                "<%s> transport stream changed..: signaling  RMF_SI_EVENT_SERVICE_DETAILS_UPDATE for handle:0x%x sourceId:0x%x\n", 
                                __FUNCTION__, walker, walker->source_id);
                        m_pSiEventListener->send_si_event(RMF_SI_EVENT_SERVICE_DETAILS_UPDATE, (rmf_SiServiceHandle)walker, RMF_SI_CHANGE_TYPE_MODIFY);
                        //pOobSiMgr->send_si_event(RMF_SI_EVENT_SERVICE_DETAILS_UPDATE, (rmf_SiServiceHandle)walker, RMF_SI_CHANGE_TYPE_MODIFY);
                    }
//#endif
//#endif
                }
                else
                { // Same TS, moved programs?
                    if (walker->program != RMF_SI_INVALID_HANDLE)
                    {
                        if (walker->program !=  pi_handle)
                        {
                            if (is_service_entry_available(((rmf_SiProgramInfo*)walker->program)->services,
                                walker))
                            {
                                   ((rmf_SiProgramInfo*)walker->program)->services->remove((rmf_SiServiceHandle)walker);
                                   rmf_SiProgramInfo *pi = (rmf_SiProgramInfo*)walker->program;
                                    pi->service_count--;
                                    walker->program = RMF_SI_INVALID_HANDLE;
                             }
#ifdef TEST_WITH_PODMGR                                  
#ifndef GENERATE_SI_CACHE_UTILITY
                             // Signal ServiceDetails update event (MODIFY)
                            if(m_pSiEventListener != NULL)
                            {
                                RDK_LOG(
                                        RDK_LOG_INFO,
                                        "LOG.RDK.SI",
                                        "<%s> program changed..: signaling  RMF_SI_EVENT_SERVICE_DETAILS_UPDATE for handle:0x%x sourceId:0x%x pn:%d\n",
                                        __FUNCTION__, walker, walker->source_id, walker->program_number);
                                m_pSiEventListener->send_si_event(RMF_SI_EVENT_SERVICE_DETAILS_UPDATE, (rmf_SiServiceHandle)walker, RMF_SI_CHANGE_TYPE_MODIFY);
                                //pOobSiMgr->send_si_event(RMF_SI_EVENT_SERVICE_DETAILS_UPDATE, (rmf_SiServiceHandle)walker, RMF_SI_CHANGE_TYPE_MODIFY);
                            }
#endif
#endif
                        }
                    }
                }
            }


            // Increment the global number of services
            g_numberOfServices++;

            if (pi_handle != RMF_SI_INVALID_HANDLE)
            {
               // LINK *lp, *lp2;
               list<rmf_SiServiceHandle>::iterator si;
                rmf_SiProgramInfo *pgm = (rmf_SiProgramInfo *) pi_handle;
                //lp2 = llist_first(pgm->services);
                 si = pgm->services->begin();
                while (si != pgm->services->end())
                {
                   // si_entry = (rmf_SiTableEntry *) llist_getdata(lp2);
                   si_entry = (rmf_SiTableEntry *) (*si);
                    // This may be a dynamic entry created prior to acquiring OOB SI.
                    // State would be SIENTRY_UNSPECIFIED
                    // clone the fields from current entry to dynamic entry
                    if ((si_entry->state == SIENTRY_UNSPECIFIED)
                            && (si_entry->program == pi_handle)
                            && (si_entry->ts_handle == ts_handle))
                    {
                        dupFound = true;
                        clone_si_entry(si_entry, walker);
                        RDK_LOG(
                                RDK_LOG_TRACE1,
                                "LOG.RDK.SI",
                                "<%s> Found a previously created entry...updated state: %d\n",
                                __FUNCTION__, si_entry->state);
                        // Mark the state of new entry such that it is not navigable
                        walker->state = SIENTRY_UNSPECIFIED;
                        // If a dup is found, decrement the count
                        g_numberOfServices--;
                        break;
                    }
                    //lp2 = llist_after(lp2);
                    si++;
                }
                // If this is not a dup and the program handle is not already set
                if (!dupFound && ((rmf_SiProgramHandle)walker->program != pi_handle))
                {
                    walker->program = pi_handle;
                    //lp = llist_mklink((void *) walker);
                    //llist_append(walker->program->services, lp);
                    ((rmf_SiProgramInfo*)walker->program)->services->push_back((rmf_SiServiceHandle)walker);
                    ((rmf_SiProgramInfo*)walker->program)->service_count++;
                }
            }

            if (walker->ts_handle == RMF_SI_INVALID_HANDLE && !dupFound)
            {
                //LINK *lp;
                
                walker->ts_handle = ts_handle;
                ((rmf_SiTransportStreamEntry *) ts_handle)->services->push_back((rmf_SiServiceHandle)walker);
                //lp = llist_mklink((void *) walker);
                //llist_append(
                    //    ((rmf_SiTransportStreamEntry *) ts_handle)->services,
                        //lp);
                ((rmf_SiTransportStreamEntry *) walker->ts_handle)->service_count++;
                ((rmf_SiTransportStreamEntry *) walker->ts_handle)->visible_service_count++;
            }
        }
    }
}

rmf_osal_Bool rmf_OobSiCache::is_service_entry_available(list<rmf_SiServiceHandle> *services, rmf_SiTableEntry* service_entry)    
{
    list<rmf_SiServiceHandle>::iterator si;
    rmf_SiServiceHandle walker = 0;
    rmf_osal_Bool found = FALSE;

    si = services->begin();

        while (si != services->end())
        {
            walker = *si;
            if(walker == (rmf_SiServiceHandle)service_entry)
            {
                    found = TRUE;
                   break;
            }
            si++;
        }

        return found;
}

/*  Retrieve DSG transport stream handle. This method creates a new handle
 if one is not already created.
 */
uintptr_t rmf_OobSiCache::get_dsg_transport_stream_handle(void)
{
    uintptr_t retVal = 0;
    if (g_si_dsg_ts_entry == NULL)
    {
        /* Create a new rmf_SiTransportStreamEntry node */
        if (RMF_SUCCESS != rmf_osal_memAllocP(RMF_OSAL_MEM_SI_CACHE_OOB,
                sizeof(rmf_SiTransportStreamEntry),
                (void **) &(g_si_dsg_ts_entry)))
        {
            RDK_LOG(
                    RDK_LOG_WARN,
                    "LOG.RDK.SI",
                    "<%s> Mem allocation failed, returning MPE_SI_OUT_OF_MEM...\n", __FUNCTION__);
            return retVal;
        }
        if (RMF_SUCCESS != m_pInbSiCache->init_transport_stream_entry(g_si_dsg_ts_entry,
                RMF_SI_DSG_FREQUENCY))
        {
            rmf_osal_memFreeP(RMF_OSAL_MEM_SI_CACHE_OOB, g_si_dsg_ts_entry);
            g_si_dsg_ts_entry = NULL;
            return 0;
        }
    }
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<%s> g_si_dsg_ts_entry:0x%p\n",
            __FUNCTION__, g_si_dsg_ts_entry);
    return (uintptr_t) g_si_dsg_ts_entry;

}


/**
 * Create a DSG service entry given source ID and service name
 *
 * <i>CreateDSGServiceHandle()</i>
 *
 *
 * @param appID is the applcaiton Id for the DSG tunnel.
 * @param service_name is a unique name assigned to the tunnel
 * @param service_handle output param to populate the handle
 *
 * @return RMF_SI_SUCCESS if successfully created the handle, else
 *          return appropriate rmf_Error
 *
 */
rmf_Error rmf_OobSiCache::CreateDSGServiceHandle(uint32_t appID, uint32_t prog_num,
                char* service_name, char* language, rmf_SiServiceHandle *service_handle)
{
        rmf_SiTableEntry *walker = NULL;
        rmf_SiTableEntry *new_si_entry = NULL;
        rmf_SiTransportStreamEntry *ts = NULL;
        int len = 0;
        int found = 0;
        int inTheList = 0;
        rmf_SiProgramInfo *pgm = NULL;
        rmf_Error tRetVal = RMF_SUCCESS;
        list<rmf_SiProgramHandle>::iterator pi;
        list<rmf_SiServiceHandle>::iterator si;

        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                        "<%s> appID= %d\n", __FUNCTION__, appID);

        // Parameter check
        if (service_handle == NULL)
        {
                return RMF_SI_INVALID_PARAMETER;
        }

        // Clear out the value
        *service_handle = RMF_SI_INVALID_HANDLE;

        // Program number for application tunnels?
    ts = (rmf_SiTransportStreamEntry *) get_dsg_transport_stream_handle();

        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<%s> transport stream entry...0x%p\n",
                        __FUNCTION__, ts);

        rmf_osal_mutexAcquire(ts->list_lock);
        {
                pi = ts->programs->begin();
                while((pi != ts->programs->end())&& !found)
                {
                    pgm = (rmf_SiProgramInfo *) (*pi);
                    if(pgm)
                    {
                        si = pgm->services->begin();
                        while(si != pgm->services->end())
                        {
                            walker = (rmf_SiTableEntry *)(*si);
                            if (walker != NULL && walker->app_id == appID)
                            {
                                new_si_entry = walker;
                                 inTheList = true;
                                 found = true;
                                 break;
                            }
                            si++;
                        }
                    }
                    pi++;
                }

                if (pi == ts->programs->end())
                {
                        // Create a place holder to store program info
                        pgm = m_pInbSiCache->create_new_program_info(ts, prog_num);
                        
                        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                    "<%s> program Entry...0x%p\n",
                                        __FUNCTION__, pgm);
                        if (pgm == NULL)
                        {
                                rmf_osal_mutexRelease(ts->list_lock);
                                return RMF_SI_OUT_OF_MEM;
                        }
                        // newly created pgm is attached to ts by create_new_program_info.
                }
        }

        if (new_si_entry == NULL)
        {
                tRetVal = rmf_osal_memAllocP(RMF_OSAL_MEM_SI_CACHE_OOB, sizeof(rmf_SiTableEntry),
                                (void **) &(new_si_entry));
                if ((RMF_SUCCESS != tRetVal) || (new_si_entry == NULL))
                {
                        RDK_LOG(
                                        RDK_LOG_WARN,
                                        "LOG.RDK.SI",
                                        "<%s> Mem allocation failed, returning RMF_SI_OUT_OF_MEM...\n",
                                        __FUNCTION__);
                        rmf_osal_mutexRelease(ts->list_lock);
                        return RMF_SI_OUT_OF_MEM;
                }

                RDK_LOG(
                                RDK_LOG_TRACE1,
                                "LOG.RDK.SI",
                "<%s> creating new SI Entry...0x%p\n",
                                __FUNCTION__, new_si_entry);

                /* Initialize all the fields (some fields are set to default values as defined in spec) */
                init_si_entry(new_si_entry);
                new_si_entry->isAppType = TRUE;
                new_si_entry->app_id = appID;
                found = true;

                // Link into other structs.
                if (pgm != NULL)
                {
                        RDK_LOG(RDK_LOG_TRACE7, "LOG.RDK.SI",
                                        "Add service %p to program %p...\n", new_si_entry, pgm);
                        pgm->services->push_back((rmf_SiServiceHandle)new_si_entry);
                        pgm->service_count++;
                }
                {
                        ts->services->push_back((rmf_SiServiceHandle)new_si_entry);
                        ts->service_count++;
                        RDK_LOG(RDK_LOG_TRACE7, "LOG.RDK.SI",
                                        "Add service %p to transport %p...\n", new_si_entry, ts);
                }
        }
        else
        {
                found = true;
        }

        if (service_name != NULL)
                len = strlen(service_name);

        if (len > 0)
        {
        //
        // Code modified to look up source name entry as this information moved to another table.
        rmf_siSourceNameEntry *source_name_entry = NULL;

        //
        // Look up, but allow a create, the source name by appID
        GetSourceNameEntry(appID, TRUE, &source_name_entry, TRUE);
        if (source_name_entry != NULL)
                {
          SetSourceName(new_si_entry->source_name_entry, service_name, language, FALSE);
          SetSourceLongName(new_si_entry->source_name_entry, service_name, language);
                }
        }
        rmf_osal_mutexRelease(ts->list_lock);

        if (found)
        {
                // ServiceEntry pointers
                new_si_entry->ts_handle = (rmf_SiTransportStreamHandle) ts;
                new_si_entry->program = (rmf_SiProgramHandle)pgm;
                new_si_entry->source_id = SOURCEID_UNKNOWN;

                RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                                "<%s> Inserting service entry in the list \n",
                                __FUNCTION__);

                if (g_SITPConditionSet == TRUE)
                {
                        (ts->service_count)++;
                        (ts->visible_service_count)++;
                }
                new_si_entry->ref_count++;

                *service_handle = (rmf_SiServiceHandle) new_si_entry;

                /* append new entry to the dynamic list */
                if(!inTheList)
                {
                        if (g_si_entry == NULL)
                        {
                                g_si_entry = new_si_entry;
                        }
                        else
                        {
                                walker = g_si_entry;
                                while (walker)
                                {
                                        if (walker->next == NULL)
                                        {
                                                walker->next = new_si_entry;
                                                break;
                                        }
                                        walker = walker->next;
                                }
                        }
                }
        }

        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                        "<%s> done, returning MPE_SI_SUCCESS\n", __FUNCTION__);
        return RMF_SUCCESS;
}

/*
 * This method returns false if any entry is in DEFINED but UNMAPPED state.
 * Implication is that the database considers initial SI acquisition to be complete if
 * all of the DCM defined virtual channels have been found mapped in the VCM.
 */
rmf_osal_Bool rmf_OobSiCache::IsVCMComplete(void)
{
    rmf_SiTableEntry *walker = NULL;
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<mpe_siIsVCMComplete> ...g_numberOfServices: %d\n",
            g_numberOfServices);

    /* Walk through the list of services to find defined channels (DCM) that are not yet found in VCM */
    walker = g_si_entry;
    while (walker)
    {
        // This means the entry is defined in the DCM but not yet found in VCM
        if (walker->state == SIENTRY_DEFINED_UNMAPPED)
        {
            RDK_LOG(
                    RDK_LOG_TRACE1,
                    "LOG.RDK.SI",
                    "<mpe_siIsVCMComplete> ... entry defined in DCM, not yet present in VCM, returning FALSE, source_id:0x%x virtual_channel_number:%d channel_type:%d state:%d \n",
                    walker->source_id, walker->virtual_channel_number,
                    walker->channel_type, walker->state);
            return FALSE;
        }

        walker = walker->next;
    }
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<mpe_siIsVCMComplete> ...returning TRUE\n");
    return TRUE;
}

/*
 * This method sets the service entry state to 'PRESENT_IN_DCM' and 'DEFINED'
 * Called during parsing of SVCT-DCM sub-table.
 */
void rmf_OobSiCache::SetServiceEntryStateDefined(rmf_SiServiceHandle service_handle)
{
    rmf_SiTableEntry * si_entry = (rmf_SiTableEntry *) service_handle;
    int state = -1;

    RDK_LOG(RDK_LOG_TRACE7, "LOG.RDK.SI",
            "<mpe_siSetServiceEntryStateDefined> ...service_handle:0x%x\n",
            service_handle);

    /* Parameter check */
    if (service_handle == RMF_SI_INVALID_HANDLE)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI",
                "<mpe_siSetServiceEntryStateDefined> RMF_SI_INVALID_PARAMETER\n");
    }
    else
    {
        state = si_entry->state | SIENTRY_PRESENT_IN_DCM;
        state |= SIENTRY_DEFINED;
        si_entry->state = (rmf_SiEntryState) state;
//        si_entry->state |= SIENTRY_PRESENT_IN_DCM;
//        si_entry->state |= SIENTRY_DEFINED;
    }
}

/*
 * This method sets the service entry state to 'PRESENT_IN_DCM' and 'UNDEFINED'
 * Called during parsing of SVCT-DCM sub-table.
 */
void rmf_OobSiCache::SetServiceEntryStateUnDefined(rmf_SiServiceHandle service_handle)
{
    rmf_SiTableEntry * si_entry = (rmf_SiTableEntry *) service_handle;
    int state = -1;

    RDK_LOG(RDK_LOG_TRACE7, "LOG.RDK.SI",
            "<mpe_siSetServiceEntryStateUnDefined> ...service_handle:0x%x\n",
            service_handle);

    /* Parameter check */
    if (service_handle == RMF_SI_INVALID_HANDLE)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI",
                "<mpe_siSetServiceEntryStateUnDefined> RMF_SI_INVALID_PARAMETER\n");
    }
    else
    {
        state = si_entry->state | SIENTRY_PRESENT_IN_DCM;
        state &= ~((int)SIENTRY_DEFINED);
        si_entry->state = (rmf_SiEntryState) state;        
//        si_entry->state |= SIENTRY_PRESENT_IN_DCM;
//        si_entry->state &= ~SIENTRY_DEFINED;
    }
}

void rmf_OobSiCache::SetServiceEntryStateMapped(rmf_SiServiceHandle service_handle)
{
    rmf_SiTableEntry * si_entry = (rmf_SiTableEntry *) service_handle;
    int state = -1;

    RDK_LOG(RDK_LOG_TRACE7, "LOG.RDK.SI",
            "<mpe_siSetServiceEntryStateMapped> ...service_handle:0x%x\n",
            service_handle);

    /* Parameter check */
    if (service_handle == RMF_SI_INVALID_HANDLE)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI",
                "<mpe_siSetServiceEntryStateMapped> RMF_SI_INVALID_PARAMETER\n");
    }
    else
    {
        state = si_entry->state | SIENTRY_MAPPED;
        si_entry->state = (rmf_SiEntryState) state;        
//        si_entry->state |= SIENTRY_MAPPED;
    }
}

/*
 * Returns the SI Entry state
 */
void rmf_OobSiCache::GetServiceEntryState(rmf_SiServiceHandle service_handle,
        uint32_t *state)
{
    rmf_SiTableEntry *si_entry = (rmf_SiTableEntry *) service_handle;

    /* Parameter check */
    if (service_handle == RMF_SI_INVALID_HANDLE || state == NULL)
    {
        return;
    }

    *state = si_entry->state;

}

void rmf_OobSiCache::SetServiceEntryState(rmf_SiServiceHandle service_handle,
        uint32_t state)
{
    rmf_SiTableEntry *si_entry = (rmf_SiTableEntry *) service_handle;

    /* Parameter check */
    if (service_handle == RMF_SI_INVALID_HANDLE)
    {
        return;
    }

    si_entry->state = (rmf_SiEntryState)state;
}

void rmf_OobSiCache::SetServiceEntriesStateDefined(uint16_t channel_number)
{
    rmf_SiTableEntry *walker = NULL;
    int state = -1;

    // There may be other entries in the SI DB with this
    // virtual channel number. Set the DCM state for those also
    walker = g_si_entry;
    while (walker)
    {
        // If a service entry exists all the members are set to default values
        if (walker->virtual_channel_number == channel_number)
        {
            state = walker->state | SIENTRY_PRESENT_IN_DCM;
            state |= SIENTRY_DEFINED;
            walker->state = (rmf_SiEntryState) state;        
//            walker->state |= SIENTRY_PRESENT_IN_DCM;
//            walker->state |= SIENTRY_DEFINED;
        }
        walker = walker->next;
    }
}

void rmf_OobSiCache::SetServiceEntriesStateUnDefined(uint16_t channel_number)
{
    rmf_SiTableEntry *walker = NULL;
    int state = -1;

    // There may be other entries in the SI DB with this
    // virtual channel number. Set the DCM state for those also
    walker = g_si_entry;
    while (walker)
    {
        // If a service entry exists all the members are set to default values
        if (walker->virtual_channel_number == channel_number)
        {
            state = walker->state | SIENTRY_PRESENT_IN_DCM;
            state &= ~((int)SIENTRY_DEFINED);
            walker->state = (rmf_SiEntryState) state;          
//            walker->state |= SIENTRY_PRESENT_IN_DCM;
//            walker->state &= ~SIENTRY_DEFINED;
        }
        walker = walker->next;
    }
}

/**
 *  This method is called when SVCT is acquired to retrieve the corresponding
 *  frequency the 'sourceId' is carried on.
 *
 *  Get frequency given cds_ref index
 *
 * <i>GetFrequencyFromCDSRef()</i>
 *
 * @param cds_ref is the index into NIT_CDS table
 * @param frequency is the output param to populate the corresponding frequency
 *
 * @return RMF_SI_SUCCESS if successful, else
 *          return appropriate rmf_Error
 *
 */
void rmf_OobSiCache::GetFrequencyFromCDSRef(uint8_t cds_ref, uint32_t * frequency)
{
    /* Parameter check */
    if ((cds_ref == 0) || (frequency == NULL))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI",
                "<GetFrequencyFromCDSRef> RMF_SI_INVALID_PARAMETER\n");
    }
    else
    {
        *frequency = g_frequency[cds_ref];
    }
}

/**
 *  This method is called when SVCT is acquired to retrieve the corresponding
 *  modulation mode given mms_reference
 *
 * <i>GetModulationFromMMSRef()</i>
 *
 * @param mms_ref is the index into NIT_MMS table
 * @param modulation is the output param to populate the corresponding modulation
 *
 * @return RMF_SI_SUCCESS if successful, else
 *          return appropriate rmf_Error
 *
 */
void rmf_OobSiCache::GetModulationFromMMSRef(uint8_t mms_ref,
        rmf_ModulationMode * modulation)
{
    /* Parameter check */
    if ((mms_ref == 0) || (modulation == NULL))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI",
                "<GetModulationFromMMSRef> RMF_SI_INVALID_PARAMETER\n");
    }
    else
    {
        *modulation = g_mode[mms_ref];
    }
}

void rmf_OobSiCache::LockForWrite()
{
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<mpe_siLockForWrite> ...\n");

    // SITP makes this call to acquire the global SIDB lock
    // block here until the condition object is set
    rmf_osal_condGet(g_global_si_cond);
}

void rmf_OobSiCache::ReleaseWriteLock()
{
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<mpe_siReleaseWriteLock> ...\n");
    // SITP makes this call to release the global SIDB lock
    // release the lock
    rmf_osal_condSet(g_global_si_cond);
}

#if 0
/**
 *  This method will be called by 'Table Parser' layer when it has acquired PAT/PMT.
 *  By this time SVCT should have already been acquired and sourceId-to-program_number
 *  mapping should already exist and also carrier frequency and modulation modes should
 *  have been populated from NIT_CD and NIT_MM tables.
 *  The table parser upon acquiring PAT/PMT will get the corresponding frequency
 *  (current tuned) from the media layer.
 *  The frequency and program_number pair will then be used to uniquely identify a service
 *  in the table. For VOD however, the sourceId will not exist in the table as they are not
 *  signalled in SVCT. In that case a new table entry will be created to represent VOD service.
 *
 *  Retrieve service entry handle given freq, prog_num and modulation
 *
 * <i>GetServiceEntryFromFrequencyProgramNumberModulation()</i>
 *
 * @param freq is the input param to obtain the service entry from
 * @param program_number is the input param to obtain the service entry from
 * @param mode is the input modulation mode
 * @param si_entry_handle is the output param to populate the service handle
 *
 * @return RMF_SI_SUCCESS if successful, else
 *          return appropriate rmf_Error
 *
 */
// CALLER MUST BE HOLDING WRITE LOCK WHEN CALLING THIS FUNCTION!
// mpe_siLockForWrite();

//
// No longer a valid method of creating things.  A Transportstream (optionally with program) should be created and
// then a service entry can be associated with in in the next step.
//

rmf_Error rmf_OobSiCache::GetServiceEntryFromFrequencyProgramNumberModulation(
        uint32_t freq, uint32_t program_number, rmf_ModulationMode mode,
        rmf_SiServiceHandle *si_entry_handle)
{
    /*
     This method is called by the table parser layer when a new VCT or PAT/PMT is detected.
     If an entry is found in the existing list a handle to that is returned. If not found,
     a new entry is created.
     */
    rmf_SiTransportStreamEntry *ts = NULL;
    rmf_SiTableEntry *new_si_entry = NULL;
    rmf_SiProgramInfo *pgm = NULL;
    int found = 0;

    RDK_LOG(
            RDK_LOG_TRACE1,
            "LOG.RDK.SI",
            "<GetServiceEntryFromFrequencyProgramNumberModulation> Freq: %d  Program: %d  Modulation: %d\n",
            freq, program_number, mode);

    /* Parameter check */
    if (si_entry_handle == NULL)
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    *si_entry_handle = RMF_SI_INVALID_HANDLE;

    // create_transport_stream_handle() will return an existing ts, if it can, create new if it can't.
    ts = (rmf_SiTransportStreamEntry *) create_transport_stream_handle(freq,
            mode);
    if (ts == NULL)
    {
        RDK_LOG(
                RDK_LOG_WARN,
                "LOG.RDK.SI",
                "<GetServiceEntryFromFrequencyProgramNumberModulation> Mem allocation (ts) failed, returning RMF_SI_OUT_OF_MEM...\n");
        return RMF_SI_OUT_OF_MEM;
    }

    rmf_osal_mutexAcquire(ts->list_lock);

    if (mode != RMF_MODULATION_QAM_NTSC)
    {
        LINK *lp = llist_first(ts->programs);
        while (lp)
        {
            pgm = (rmf_SiProgramInfo *) llist_getdata(lp);
            if (pgm)
            {
                if (pgm->program_number == program_number)
                    break;
            }
            lp = llist_after(lp);
        }
        if (pgm == NULL)
        {
            pgm = create_new_program_info(ts, program_number);
            if (pgm == NULL)
            {
                RDK_LOG(
                        RDK_LOG_WARN,
                        "LOG.RDK.SI",
                        "<GetServiceEntryFromFrequencyProgramNumberModulation> Mem allocation (pgm) failed, returning RMF_SI_OUT_OF_MEM...\n");
                rmf_osal_mutexRelease(ts->list_lock);
                return RMF_SI_OUT_OF_MEM;
            }
        }
        {
            lp = llist_first(pgm->services);
            if (lp != NULL)
            {
                new_si_entry = (rmf_SiTableEntry *) llist_getdata(lp);
            }
        }
    }
    else
    {
        {
            LINK *lp = llist_first(ts->services);
            if (lp != NULL)
            {
                new_si_entry = (rmf_SiTableEntry *) llist_getdata(lp);
            }
        }
    }

    if (new_si_entry == NULL)
    {
        rmf_Error tRetVal = rmf_osal_memAllocP(RMF_OSAL_MEM_SI_CACHE_OOB,
                sizeof(rmf_SiTableEntry), (void **) &(new_si_entry));
        if ((RMF_SI_SUCCESS != tRetVal) || (new_si_entry == NULL))
        {
            RDK_LOG(
                    RDK_LOG_WARN,
                    "LOG.RDK.SI",
                    "<GetServiceEntryFromFrequencyProgramNumberModulation> Mem allocation (entry) failed, returning RMF_SI_OUT_OF_MEM...\n");
            rmf_osal_mutexRelease(ts->list_lock);
            return RMF_SI_OUT_OF_MEM;
        }
        if (pgm != NULL)
        {
            LINK *lp = llist_mklink((void *) new_si_entry);
            llist_append(pgm->services, lp);
            pgm->service_count++;
        }

        {
            LINK *lp = llist_mklink((void *) new_si_entry);
            llist_append(ts->services, lp);
            ts->service_count++;
        }
    }
    else
        found = 1;

    rmf_osal_mutexRelease(ts->list_lock);

    /*
     If si entry is not found, create a new one append it to list
     For VOD an si entry will not exist
     */
    if (!found)
    {
        rmf_SiTableEntry *walker = NULL;
        /* Initialize all the fields (some fields are set to default values as defined in spec) */
        init_si_entry(new_si_entry);
        /* set the source id to a known value */
        /* For dynamic services (ex: vod) since sourceid is unknown, just default it to SOURCEID_UNKNOWN/-1.*/
        new_si_entry->source_id = SOURCEID_UNKNOWN;

        new_si_entry->ts_handle = (uint32_t) ts;
        new_si_entry->program = pgm; // Might be null, if Analog service.

        RDK_LOG(
                RDK_LOG_TRACE1,
                "LOG.RDK.SI",
                "<GetServiceEntryFromFrequencyProgramNumberModulation> ts_handle:0x%x\n",
                new_si_entry->ts_handle);
        if (freq == RMF_SI_OOB_FREQUENCY || freq == RMF_SI_DSG_FREQUENCY || freq == RMF_SI_HN_FREQUENCY)
        {
            new_si_entry->channel_type = CHANNEL_TYPE_NORMAL;

            RDK_LOG(
                    RDK_LOG_INFO,
                    "LOG.RDK.SI",
                    "<GetServiceEntryFromFrequencyProgramNumberModulation> got OOB_FREQ si_entry:0x%p ts_handle:0x%x , appID=%u\n",
                    new_si_entry, new_si_entry->ts_handle,
                    new_si_entry->source_id);

        }

        *si_entry_handle = (rmf_SiServiceHandle) new_si_entry;

        /* append new entry to the dynamic list */
        if (g_si_entry == NULL)
        {
            g_si_entry = new_si_entry;
        }
        else
        {
            walker = g_si_entry;
            while (walker)
            {
                if (walker->next == NULL)
                {
                    RDK_LOG(
                            RDK_LOG_TRACE1,
                            "LOG.RDK.SI",
                            "<GetServiceEntryFromFrequencyProgramNumberModulation> set %p->next to %p\n",
                            walker, new_si_entry);
                    walker->next = new_si_entry;
                    break;
                }
                walker = walker->next;
            }
        }

        /* Increment the number of services */
        g_numberOfServices++;
    }

    *si_entry_handle = (rmf_SiServiceHandle) new_si_entry;

    return RMF_SI_SUCCESS;
}
#endif

#if 0
/* Deprecated */
rmf_Error rmf_OobSiCache::GetServiceEntryFromFrequencyProgramNumberModulationAppId(
        uint32_t freq, uint32_t program_number, rmf_ModulationMode mode,
        uint32_t app_id, rmf_SiServiceHandle *si_entry_handle)
{
    /*
     rmf_SiTableEntry *walker = NULL;
     rmf_SiTableEntry *prevWalker = NULL;
     rmf_SiTableEntry *new_si_entry = NULL;
     int found = 0;
     uint32_t freq_to_check = 0;

     // Parameter check
     if(si_entry_handle == NULL)
     {
     return RMF_SI_INVALID_PARAMETER;
     }

     *si_entry_handle = RMF_SI_INVALID_HANDLE;

     walker = prevWalker = g_si_entry;


     //   This method is called by the table parser layer when a new VCT or PAT/PMT is detected.
     //   If an entry is found in the existing list a handle to that is returned. If not found,
     //   a new entry is created.
     while(walker)
     {
     freq_to_check = (walker->freq_index != 0) ? g_frequency[walker->freq_index] : walker->frequency;
     if( (walker->program_number == program_number) &&
     (freq_to_check == freq) )
     {
     //RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<GetServiceEntryFromFrequencyProgramNumberModulation> SI Entry found...\n\n");

     if( ((app_id != 0 ) && (walker->app_id != 0) && (walker->app_id == app_id))
     || (app_id == 0))
     {
     walker->state = UPDATED;
     *si_entry_handle = (rmf_SiServiceHandle)walker;

     found = 1;
     break;
     }

     }
     prevWalker = walker;
     walker = walker->next;
     }


     //   If si entry is not found, create a new one append it to list
     if(!found)
     {
     // Can we assume this is a VOD service here?
     if( RMF_SI_SUCCESS != rmf_osal_memAllocP(RMF_OSAL_MEM_SI_CACHE_OOB, sizeof(rmf_SiTableEntry), (void **)&(new_si_entry)) )
     {
     *si_entry_handle = 0 ;

     RDK_LOG(RDK_LOG_WARN, "LOG.RDK.SI", "<GetServiceEntryFromFrequencyProgramNumberModulation> Mem allocation failed, returning RMF_SI_OUT_OF_MEM...\n");

     return RMF_SI_OUT_OF_MEM;
     }

     //RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<GetServiceEntryFromFrequencyProgramNumberModulation> creating new SI Entry...freq:%d, prog_num:%d\n", freq, program_number);

     // Initialize all the fields (some fields are set to default values as defined in spec)
     init_si_entry(new_si_entry);
     new_si_entry->program_number = program_number;
     new_si_entry->frequency = freq; // For DSG the frequecny should be RMF_SI_OOB_FREQUENCY (set in SITP)
     new_si_entry->modulation_mode = mode;

     if(freq == RMF_SI_OOB_FREQUENCY)
     {
     new_si_entry->channel_type = CHANNEL_TYPE_NORMAL;
     if (app_id > 0)
     {
     new_si_entry->isAppType = true;
     new_si_entry->app_id = app_id;
     }

     RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI", "<GetServiceEntryFromFrequencyProgramNumberModulation> got OOB_FREQ si_entry:0x%x ts_handle:0x%x , appID=%ld\n",new_si_entry, new_si_entry->ts_handle, new_si_entry->source_id);
     }

     if(mode == RMF_MODULATION_QAM_NTSC)
     {
     // We know that an analog service does, and always will, have no components
     new_si_entry->number_elem_streams = 0;
     new_si_entry->pmt_status = PMT_AVAILABLE;

     // For analog the program number is set to -1 (fix for 5724)
     //new_si_entry->program_number = (uint32_t) -1;
     }


     if( (new_si_entry->ts_handle = get_transport_stream_handle(freq, mode)) == 0)
     {
     new_si_entry->ts_handle = create_transport_stream_handle(freq, mode);
     }

     RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<GetServiceEntryFromFrequencyProgramNumberModulation> ts_handle:0x%x\n", new_si_entry->ts_handle);

     {
     rmf_SiTransportStreamEntry *ts_entry = (rmf_SiTransportStreamEntry *)new_si_entry->ts_handle;
     (ts_entry->service_count)++;
     // Increment the visible service count also
     //(channel type is not known for dynamic services, they are
     //not signaled in OOB VCT)
     (ts_entry->visible_service_count)++;
     //RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<GetServiceEntryFromFrequencyProgramNumberModulation> ts_entry->visible_service_count...%d\n", ts_entry->visible_service_count);
     }

     *si_entry_handle = (rmf_SiServiceHandle)new_si_entry;

     // append new entry to the dynamic list
     if(g_si_entry == NULL)
     {
     g_si_entry = new_si_entry;
     }
     else
     {
     walker = g_si_entry;
     while(walker)
     {
     if(walker->next == NULL)
     {
     walker->next = new_si_entry;
     break;
     }
     walker = walker->next;
     }
     }

     // Increment the number of services
     g_numberOfServices++;
     }
     */
    return RMF_SI_SUCCESS;
}
#endif

/*
 rmf_Error rmf_OobSiCache::GetServiceEntryFromFrequencyProgramNumberModulationSourceId(uint32_t freq, uint32_t program_number, rmf_ModulationMode mode, uint32_t source_id, rmf_SiServiceHandle *si_entry_handle)
 {
 rmf_SiTableEntry *walker = NULL;
 rmf_SiTableEntry *prevWalker = NULL;
 rmf_SiTableEntry *new_si_entry = NULL;
 rmf_SiTableEntry *saved_walker = NULL;
 int found = 0;
 uint32_t freq_to_check = 0;
 uint32_t appID = 0;

 // Parameter check
 if(si_entry_handle == NULL)
 {
 return RMF_SI_INVALID_PARAMETER;
 }

 *si_entry_handle = RMF_SI_INVALID_HANDLE;

 walker = prevWalker = g_si_entry;


 //   This method is called by the table parser layer when a new VCT or PAT/PMT is detected.
 //   If an entry is found in the existing list a handle to that is returned. If not found,
 //   a new entry is created.
 while(walker)
 {
 freq_to_check = (walker->freq_index != 0) ? g_frequency[walker->freq_index] : walker->frequency;
 if( (walker->program_number == program_number) &&
 (freq_to_check == freq) )
 {
 //RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<GetServiceEntryFromFrequencyProgramNumberModulation> SI Entry found...\n\n");

 // If incoming source_id does not match existing source_id
 // means we need to create a new service entry
 // This will have same frequency, program_number, mode triple
 // as this service entry, but a different source_id

 if( (walker->source_id == SOURCEID_UNKNOWN) ||
 ((walker->source_id != SOURCEID_UNKNOWN) && (walker->source_id == source_id) )
 {
 walker->state = UPDATED;
 *si_entry_handle = (rmf_SiServiceHandle)walker;

 found = 1;
 break;
 }
 else
 {
 saved_walker = walker;
 }

 }
 prevWalker = walker;
 walker = walker->next;
 }


 //   If si entry is not found, create a new one append it to list
 //   For VOD an si entry will not exist
 if(!found)
 {
 // Can we assume this is a VOD service here?
 if( RMF_SI_SUCCESS != rmf_osal_memAllocP(RMF_OSAL_MEM_SI_CACHE_OOB, sizeof(rmf_SiTableEntry), (void **)&(new_si_entry)) )
 {
 *si_entry_handle = 0 ;

 RDK_LOG(RDK_LOG_WARN, "LOG.RDK.SI", "<GetServiceEntryFromFrequencyProgramNumberModulation> Mem allocation failed, returning RMF_SI_OUT_OF_MEM...\n");

 return RMF_SI_OUT_OF_MEM;
 }

 //RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<GetServiceEntryFromFrequencyProgramNumberModulation> creating new SI Entry...freq:%d, prog_num:%d\n", freq, program_number);

 // Initialize all the fields (some fields are set to default values as defined in spec)
 init_si_entry(new_si_entry);
 new_si_entry->program_number = program_number;
 new_si_entry->frequency = freq;
 new_si_entry->source_id = source_id;
 new_si_entry->modulation_mode = mode;

 if(mode == RMF_MODULATION_QAM_NTSC)
 {
 // We know that an analog service does, and always will, have no components
 new_si_entry->number_elem_streams = 0;
 new_si_entry->pmt_status = PMT_AVAILABLE;

 // For analog the program number is set to -1 (fix for 5724)
 //new_si_entry->program_number = (uint32_t) -1;
 }


 if( (new_si_entry->ts_handle = get_transport_stream_handle(freq, mode)) == 0)
 {
 new_si_entry->ts_handle = create_transport_stream_handle(freq, mode);
 }

 RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<GetServiceEntryFromFrequencyProgramNumberModulation> ts_handle:0x%x\n", new_si_entry->ts_handle);
 if(freq == RMF_SI_OOB_FREQUENCY)
 {
 new_si_entry->channel_type = CHANNEL_TYPE_NORMAL;
 getDSGAppID(&appID);
 if (appID>0)
 {
 new_si_entry->isAppType = true;
 new_si_entry->source_id = appID;
 }

 RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI", "<GetServiceEntryFromFrequencyProgramNumberModulation> got OOB_FREQ si_entry:0x%x ts_handle:0x%x , appID=%ld\n",new_si_entry, new_si_entry->ts_handle, new_si_entry->source_id);

 }

 {
 rmf_SiTransportStreamEntry *ts_entry = (rmf_SiTransportStreamEntry *)new_si_entry->ts_handle;
 (ts_entry->service_count)++;
 // Increment the visible service count also
 //(channel type is not known for dynamic services, they are
 //not signaled in OOB VCT)
 (ts_entry->visible_service_count)++;
 //RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<GetServiceEntryFromFrequencyProgramNumberModulation> ts_entry->visible_service_count...%d\n", ts_entry->visible_service_count);
 }

 *si_entry_handle = (rmf_SiServiceHandle)new_si_entry;

 // append new entry to the dynamic list
 if(g_si_entry == NULL)
 {
 g_si_entry = new_si_entry;
 }
 else
 {
 walker = g_si_entry;
 while(walker)
 {
 if(walker->next == NULL)
 {
 walker->next = new_si_entry;
 break;
 }
 walker = walker->next;
 }
 }

 // Increment the number of services
 g_numberOfServices++;
 }

 return RMF_SI_SUCCESS;
 }
 */

/* NTT_SNS */
/** OCAP spec
 When the LVCT or the CVCT is provided the short_name is returned. When the LVCT is not provided but the
 Source Name Sub-Table is provided a component of the source_name is returned from the MTS string. When
 none of these Tables or sub-Tables are provided, NULL is returned.
 */
rmf_Error rmf_OobSiCache::SetSourceName(rmf_siSourceNameEntry *source_name_entry,
        char *sourceName, char *language, rmf_osal_Bool override_existing)
{
    rmf_SiLangSpecificStringList *entry = NULL;
    char *source_name_language = NULL;

    // Parameter check
    if (sourceName == NULL)
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    // If the language is NULL, this source name must have come from the LVCT/CVCT which
    // does not specify the channel name in a language specific way like the NTT_SNS
    if (language == NULL)
        source_name_language = (char*)"";
    else
        source_name_language = language;


    entry = source_name_entry->source_names;

    // Search for an existing entry for the given language
    while (entry != NULL)
    {
        // If a previous entry is found (and we are allowed to override the existing entry),
        // free the old string and allocate new space for the given string
        if ((strcmp(entry->language,source_name_language) == 0) )
        {
            if (override_existing)
            {
                rmf_osal_memFreeP(RMF_OSAL_MEM_SI_CACHE_OOB, entry->string);

            if (RMF_SI_SUCCESS != rmf_osal_memAllocP(RMF_OSAL_MEM_SI_CACHE_OOB,
                    strlen(sourceName) + 1, (void **) &(entry->string)))
                {
                RDK_LOG(RDK_LOG_WARN, "LOG.RDK.SI",
                        "<SetSourceName> Mem allocation failed, returning RMF_SI_OUT_OF_MEM...\n");
                    return RMF_SI_OUT_OF_MEM;
                }
                strcpy(entry->string,sourceName);

            RDK_LOG(
                    RDK_LOG_TRACE1,
                    "LOG.RDK.SI",
                    "<SetSourceName> Replaced previous entry! -- source_name:%s, language:%s\n",
                    entry->string, entry->language);

                return RMF_SI_SUCCESS;
            }
            else
            {
                
                if (strcmp(entry->string, sourceName) == 0)
                {
                    return RMF_SI_SUCCESS;
                }
            }
        }

        entry = entry->next;
    }

    // RDK_LOG(
    //      RDK_LOG_TRACE1,
    //      "LOG.RDK.SI",
    //      "<SetSourceNameForServiceHandle> source_id: 0x%x sourceName: %s\n",
    //      si_entry->source_id, sourceName);

    // No previous entry found, so allocate a new list entry
    if (RMF_SI_SUCCESS != rmf_osal_memAllocP(RMF_OSAL_MEM_SI_CACHE_OOB,
            sizeof(struct rmf_SiLangSpecificStringList), (void **) &entry))
    {
        RDK_LOG(RDK_LOG_WARN, "LOG.RDK.SI",
                "<SetSourceName> Mem allocation failed, returning RMF_SI_OUT_OF_MEM...\n");
        return RMF_SI_OUT_OF_MEM;
    }

    // Set the source_name
    if (RMF_SI_SUCCESS != rmf_osal_memAllocP(RMF_OSAL_MEM_SI_CACHE_OOB, strlen(sourceName) + 1,
            (void **) &(entry->string)))
    {
        RDK_LOG(RDK_LOG_WARN, "LOG.RDK.SI",
                "<SetSourceName> Mem allocation failed, returning RMF_SI_OUT_OF_MEM...\n");
        rmf_osal_memFreeP(RMF_OSAL_MEM_SI_CACHE_OOB, entry);		
        return RMF_SI_OUT_OF_MEM;
    }
    strcpy(entry->string, sourceName);
    strncpy(entry->language, source_name_language, 4);

    // Add new list entry to the front of list for this service
    entry->next = source_name_entry->source_names;
    source_name_entry->source_names = entry;

    RDK_LOG(
            RDK_LOG_TRACE6,
            "LOG.RDK.SI",
            "<SetSourceName> Created new entry! -- source_name:%s, language:%s\n",
            entry->string, entry->language);

    return RMF_SI_SUCCESS;
}

rmf_Error rmf_OobSiCache::SetSourceLongName(rmf_siSourceNameEntry *source_name_entry,
        char *sourceLongName, char *language)
{
    rmf_SiLangSpecificStringList *entry = NULL;

    // Parameter check
    if ((sourceLongName == NULL)
            || (language == NULL))
    {
        return RMF_SI_INVALID_PARAMETER;
    }


    entry = source_name_entry->source_long_names;

    // Search for an existing entry for the given language
    while (entry != NULL)
    {
        // If a previous entry is found, free the old string and allocate new space for the
        // given string
        if (strcmp(entry->language, language) == 0)
        {
            rmf_osal_memFreeP(RMF_OSAL_MEM_SI_CACHE_OOB, entry->string);

            if (RMF_SI_SUCCESS != rmf_osal_memAllocP(RMF_OSAL_MEM_SI_CACHE_OOB, strlen(
                    sourceLongName) + 1, (void **) &(entry->string)))
            {
                RDK_LOG(
                        RDK_LOG_WARN,
                        "LOG.RDK.SI",
                        "<SetSourceLongName> Mem allocation failed, returning RMF_SI_OUT_OF_MEM...\n");
                return RMF_SI_OUT_OF_MEM;
            }
            strcpy(entry->string, sourceLongName);

            RDK_LOG(
                    RDK_LOG_TRACE1,
                    "LOG.RDK.SI",
                    "<SetSourceLongName> Replaced previous entry! -- source_name:%s, language:%s\n",
                    entry->string, entry->language);

            return RMF_SI_SUCCESS;
        }
        entry = entry->next;
    }

    // No previous entry found, so allocate a new list entry
    if (RMF_SI_SUCCESS != rmf_osal_memAllocP(RMF_OSAL_MEM_SI_CACHE_OOB,
            sizeof(struct rmf_SiLangSpecificStringList), (void **) &entry))
    {
        RDK_LOG(
                RDK_LOG_WARN,
                "LOG.RDK.SI",
                "<SetSourceLongName> Mem allocation failed, returning RMF_SI_OUT_OF_MEM...\n");
        return RMF_SI_OUT_OF_MEM;
    }

    // Set the source_name
    if (RMF_SI_SUCCESS != rmf_osal_memAllocP(RMF_OSAL_MEM_SI_CACHE_OOB, strlen(sourceLongName)
            + 1, (void **) &(entry->string)))
    {
        RDK_LOG(RDK_LOG_WARN, "LOG.RDK.SI",
                "<SetSourceName> Mem allocation failed, returning RMF_SI_OUT_OF_MEM...\n");
	rmf_osal_memFreeP(RMF_OSAL_MEM_SI_CACHE_OOB, entry);
        return RMF_SI_OUT_OF_MEM;
    }
    strcpy(entry->string, sourceLongName);
    strncpy(entry->language, language, 4);

    // Add new list entry to the front of list for this service
    entry->next = source_name_entry->source_long_names;
    source_name_entry->source_long_names = entry;

    RDK_LOG(
            RDK_LOG_TRACE1,
            "LOG.RDK.SI",
            "<SetSourceLongName> Created new entry! -- source_name:%s, language:%s\n",
            entry->string, entry->language);

    return RMF_SI_SUCCESS;
}

/**
 *  MIS
 *
 *  Set the application type for the given service entry
 *
 * <i>mpe_siSetAppType()</i>
 *
 * @param service_handle is the service handle to set the program_number for
 * @param bIsApp is the boolean value to set
 *
 * @return RMF_SI_SUCCESS if successful, else
 *          return appropriate rmf_Error
 *
 */
void rmf_OobSiCache::SetAppType(rmf_SiServiceHandle service_handle, rmf_osal_Bool bIsApp)
{
    rmf_SiTableEntry *si_entry = (rmf_SiTableEntry *) service_handle;

    /* Parameter check */
    if (service_handle == RMF_SI_INVALID_HANDLE)
    {
        return;
    }

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<mpe_siSetAppType> bIsApp: %d...\n",
            bIsApp);

    /* Set the app type */
    si_entry->isAppType = bIsApp;
    if (si_entry->isAppType == TRUE)
    {
        // This means this is a application_virtual_channel
        // so the SourceId is not really sourceId but application_id
        // Do not set AppID here. It is set by the mpe_siSetAppId() call
        si_entry->source_id = SOURCEID_UNKNOWN;
    }
}

/**
 *  SVCT
 *
 *  Set the sourceId for the given service entry
 *
 * <i>SetSourceId()</i>
 *
 * @param service_handle is the service handle to set the program_number for
 * @param sourceId is the sourceId to set
 *
 * @return RMF_SI_SUCCESS if successful, else
 *          return appropriate rmf_Error
 *
 */
rmf_Error rmf_OobSiCache::SetSourceId(rmf_SiServiceHandle service_handle,
        uint32_t sourceId)
{
    rmf_SiTableEntry *si_entry = (rmf_SiTableEntry *) service_handle;

    /* Parameter check */
    if (service_handle == RMF_SI_INVALID_HANDLE)
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<SetSourceId> sourceId: 0x%x...state:%d\n", sourceId,
            si_entry->state);

    /* Set the sourceId */
    si_entry->source_id = sourceId;


    return RMF_SI_SUCCESS;
}

/**
 * @brief To set the appId[application Id] for the given service entry.
 *
 * @param[in] service_handle Service_handle represents a SCTE service/channel.
 * @param[in] appId  appId to set.
 * @return None.
 */
void rmf_OobSiCache::SetAppId(rmf_SiServiceHandle service_handle, uint32_t appId)
{
    rmf_SiTableEntry *si_entry = (rmf_SiTableEntry *) service_handle;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<mpe_siSetAppId> appId: 0x%x...\n",
            appId);

    /* Set the sourceId */
    si_entry->app_id = appId;
}

#if 0
/**
 *  SVCT
 *
 *  Set the frequency, program number and modulation format for the given service entry
 *
 * <i>mpe_siUpdateFreqProgNumByServiceHandle()</i>
 *
 * @param service_handle is the service handle to set
 * @param freq is the  frequency to set
 * @param prog is the  program number to set
 * @param mod is the  modulation format to set
 *
 * @return RMF_SI_SUCCESS if successful, else
 *          return appropriate rmf_Error
 *
 */
rmf_Error rmf_OobSiCache::UpdateFreqProgNumByServiceHandle(
        rmf_SiServiceHandle service_handle, uint32_t freq, uint32_t prog_num,
        uint32_t mod)
{
    rmf_SiTableEntry *si_entry = (rmf_SiTableEntry *) service_handle;
    rmf_SiTransportStreamEntry * volatile ts;
    rmf_SiProgramInfo * volatile pi;
    rmf_osal_Bool freqMove = false;
    LINK *lp;

    /* Parameter check */
    if (service_handle == RMF_SI_INVALID_HANDLE)
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    RDK_LOG(
            RDK_LOG_TRACE1,
            "LOG.RDK.SI",
            "<mpe_siUpdateFreqProgNumByServiceHandle> service_handle: 0x%x freq: %d, prg_num: %d, mod: %d \n",
            service_handle, freq, prog_num, mod);

    if ((ts = (rmf_SiTransportStreamEntry *) si_entry->ts_handle) != NULL)
    {
        if (freq != ts->frequency)
            freqMove = true;
    }

    if ((pi = si_entry->program) != NULL)
    {
        if ((pi->program_number != prog_num) || freqMove)
        {
            // Changed program # and/or Frequencies, so delete ourselves from the current programs service list.
            // A program should *always* have a stream!!
            if (pi->stream != NULL)
            {
                rmf_SiTransportStreamEntry *stream = pi->stream;
                rmf_osal_mutexAcquire(stream->list_lock);
                lp = llist_linkof(pi->services, (void *) si_entry);
                if (lp != NULL)
                {
                    llist_rmlink(lp);
                    //list_RemoveData(pi->services, si_entry);
                    pi->service_count--;
                }
                si_entry->program = NULL;
                if (llist_cnt(pi->services) == 0)
                {
                    LINK *lp1 = llist_linkof(stream->programs, (void *) pi);
                    if (lp1 != NULL)
                    {
                        llist_rmlink(lp1);
                        //list_RemoveData(stream->programs, pi);
                        stream->program_count--;
                    }
                    release_program_info_entry(pi);
                }
                rmf_osal_mutexRelease(stream->list_lock);
            }
            else
            {
                RDK_LOG(
                        RDK_LOG_FATAL,
                        "LOG.RDK.SI",
                        "<mpe_siUpdateFreqProgNumByServiceHandle> ERROR!! program without a transport stream!! si_entry/program_info (%p, %p)\n",
                        si_entry, pi);
            }
        }
    }

    if (freqMove && ts)
    {
        LINK *lp1;
        rmf_osal_Bool deleteTS = false;
        // Yup, it's a move.
        rmf_osal_mutexAcquire(ts->list_lock);
        lp1 = llist_linkof(ts->services, (void *) si_entry);
        if (lp1 != NULL)
        {
            llist_rmlink(lp1);
            (ts->service_count)--;
            (ts->visible_service_count)--;
        }
        si_entry->ts_handle = RMF_SI_INVALID_HANDLE;
        if ((llist_cnt(ts->services) == 0) && (llist_cnt(ts->programs) == 0))
            deleteTS = true;
        rmf_osal_mutexRelease(ts->list_lock);
        if (deleteTS)
            release_transport_stream_entry(ts);
        ts = NULL;
    }
    if (ts == NULL)
    {
        LINK *lp1;
        ts = (rmf_SiTransportStreamEntry *) (si_entry->ts_handle
                = create_transport_stream_handle(freq, mod));
        if (ts == NULL)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI",
                    "<mpe_siUpdateFreqProgNumByServiceHandle> ERROR!! out of memory! (ts)\n");
            return RMF_SI_OUT_OF_MEM;
        }
        rmf_osal_mutexAcquire(ts->list_lock);
        lp1 = llist_mklink((void *) si_entry);
        llist_append(ts->services, lp1);
        ts->service_count++;
        rmf_osal_mutexRelease(ts->list_lock);
    }

    if ((si_entry->program == NULL) && (ts->modulation_mode
            != RMF_MODULATION_QAM_NTSC))
    {
        rmf_osal_mutexAcquire(ts->list_lock);
        lp = llist_first(ts->programs);
        while (lp)
        {
            pi = (rmf_SiProgramInfo *) llist_getdata(lp);
            if ((pi != NULL) && (pi->program_number == prog_num))
            {
                break;
            }
            lp = llist_after(lp);
        }
        if (pi == NULL)
        {
            pi = create_new_program_info(ts, prog_num);
            if (pi == NULL)
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI",
                        "<mpe_siUpdateFreqProgNumByServiceHandle> ERROR!! out of memory! (ts)\n");
                rmf_osal_mutexRelease(ts->list_lock);
                return RMF_SI_OUT_OF_MEM;
            }
        }
        si_entry->program = pi;

        {
            LINK *lp1 = llist_mklink((void *) pi);
            LINK *lp2 = llist_mklink((void *) si_entry);
            llist_append(ts->programs, lp1);
            ts->program_count++;
            llist_append(pi->services, lp2);
            pi->service_count++;
        }

        rmf_osal_mutexRelease(ts->list_lock);
    }

    return RMF_SI_SUCCESS;
}
#endif

/**
 *  SVCT
 *
 *  Set the Program_number for the given service entry
 *
 * <i>mpe_siSetProgramNumber()</i>
 *
 * @param service_handle is the service handle to set the program_number for
 * @param prog_num is the program_number to set
 *
 * @return RMF_SI_SUCCESS if successful, else
 *          return appropriate rmf_Error
 *
 */
rmf_Error rmf_OobSiCache::SetProgramNumber(rmf_SiServiceHandle service_handle,
        uint32_t prog_num)
{
    rmf_SiTableEntry *si_entry = (rmf_SiTableEntry *) service_handle;

    /* Parameter check */
    if (service_handle == RMF_SI_INVALID_HANDLE)
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    /* check for invalid Progam number (max 16 bits) */
    if (prog_num > 0xffff)
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    /* Set the program number */
    si_entry->program_number = prog_num;

    return RMF_SI_SUCCESS;
}

/**
 *  SVCT
 *
 *  Set the activation_time for the given service entry
 *
 * <i>mpe_siSetActivationTime()</i>
 *
 * @param service_handle is the service handle to set the activation_time for
 * @param activation_time is the activation_time to set
 *
 * @return RMF_SI_SUCCESS if successful, else
 *          return appropriate rmf_Error
 *
 */
/*
 Notes from SCTE 65-2002:
 Activation_time is a 32 bit unsigned integer field providing the absolute second
 the virtual channel data carried in the table section will be valid. If activation_time
 is in the past, the data in the table section shall be considered valid immediately.
 An activation_time value of zero shall be used to indicate immediate activation.
 A Host may enter a virtual channel recors whose activation times are in the future
 into a queue. Hosts are not required to implement a pending virtual channel queue,
 and may choose to discard any data that is not currently applicable.

 In the current implementation, the SI table parser layer will check if the activation_time
 parsed from SVCT is in the future and if so will start a timer until the activation_time
 becomes current time and it will then start populating from SVCT.
 */
void rmf_OobSiCache::SetActivationTime(rmf_SiServiceHandle service_handle,
        uint32_t activation_time)
{
    rmf_SiTableEntry * si_entry = (rmf_SiTableEntry *) service_handle;

    /* Parameter check */
    if (service_handle == RMF_SI_INVALID_HANDLE)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI",
                "<mpe_siSetActivationTime> RMF_SI_INVALID_PARAMETER\n");
    }
    else
    {
        /* no way to check validity of activation_time. So just accept it */
        /* Set the Activation Time */
        si_entry->activation_time = activation_time;
    }
}

/**
 *  SVCT
 *
 *  Set the virtual channel number for the given service entry
 *
 * <i>SetVirtualChannelNumber()</i>
 *
 * @param service_handle is the service handle to set the channel number for
 * @param chan_num is the virtual channel number
 *
 * @return RMF_SI_SUCCESS if successful, else
 *          return appropriate rmf_Error
 *
 */
rmf_Error rmf_OobSiCache::SetVirtualChannelNumber(rmf_SiServiceHandle service_handle,
        uint32_t virtual_channel_number)
{
    rmf_SiTableEntry *si_entry = (rmf_SiTableEntry *) service_handle;

    if (service_handle == RMF_SI_INVALID_HANDLE)
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    RDK_LOG(
            RDK_LOG_TRACE1,
            "LOG.RDK.SI",
            "<mpe_siSetChannelNumber> Enter - service_handle: 0x%x, virtual_channel_number: %d\n",
            service_handle, virtual_channel_number);

        if (virtual_channel_number > 4095)
        {
            return RMF_SI_INVALID_PARAMETER;
        }
    
    /* Set the channel numbers */
    si_entry->virtual_channel_number = virtual_channel_number;

    return RMF_SI_SUCCESS;
}

/**
 *  SVCT
 *
 *  Set the virtual channel number for the given service entry
 *
 * <i>mpe_siSetChannelNumber()</i>
 *
 * @param service_handle is the service handle to set the channel number for
 * @param chan_num is the virtual channel number
 *
 * @return RMF_SI_SUCCESS if successful, else
 *          return appropriate rmf_Error
 *
 */
rmf_Error rmf_OobSiCache::SetChannelNumber(rmf_SiServiceHandle service_handle,
        uint32_t major_channel_number, uint32_t minor_channel_number)
{
    rmf_SiTableEntry *si_entry = (rmf_SiTableEntry *) service_handle;

    if (service_handle == RMF_SI_INVALID_HANDLE)
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    RDK_LOG(
            RDK_LOG_TRACE1,
            "LOG.RDK.SI",
            "<mpe_siSetChannelNumber> Enter - service_handle: 0x%x, major_channel_number: %d, minor_channel_number: %d\n",
            service_handle, major_channel_number, minor_channel_number);

    /*
     * If this is a one part channel number, then check the value and
     * set in in the minor field.
     */
    if (minor_channel_number == RMF_SI_DEFAULT_CHANNEL_NUMBER)
    {
        if (major_channel_number > 4095)
        {
            return RMF_SI_INVALID_PARAMETER;
        }
        si_entry->major_channel_number = major_channel_number;
        si_entry->minor_channel_number = RMF_SI_DEFAULT_CHANNEL_NUMBER;
        return RMF_SI_SUCCESS;
    }

    /*
     * This is a twor part channel number, so check each param and set the values.
     *
     * Major channel number must be between 1 and 99
     */
    if (major_channel_number > 99 || major_channel_number < 1)
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    /* Minor channel number must be between 0 and 999 */
    if (minor_channel_number > 999)
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    /* Set the channel numbers */
    si_entry->major_channel_number = major_channel_number;
    si_entry->minor_channel_number = minor_channel_number;

    return RMF_SI_SUCCESS;
}

/**
 *  SVCT
 *
 *  Set the frequency index for the given service entry
 *
 * <i>mpe_siSetCDSRef()</i>
 *
 * @param service_handle is the service handle to set the frequency for
 * @param cd_ref is the index into frequency table to retrieve the frequency
 *
 * @return RMF_SI_SUCCESS if successful, else
 *          return appropriate rmf_Error
 *
 */
void rmf_OobSiCache::SetCDSRef(rmf_SiServiceHandle service_handle, uint8_t cd_ref)
{
    rmf_SiTableEntry * si_entry = (rmf_SiTableEntry *) service_handle;

    /* Parameter check */
    if (service_handle == RMF_SI_INVALID_HANDLE)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI",
                "<mpe_siSetCDSRef> RMF_SI_INVALID_PARAMETER\n");
    }
    else
    {
        /* no way to check validity of CDS_reference. So just accept it */
        //RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<mpe_siSetCDSRef> %d\n", cd_ref);
        /* Set the CDS_reference index */
        si_entry->freq_index = cd_ref;
    }
}

/**
 *  SVCT
 *
 *  Set the modulation mode for the given service entry
 *
 * <i>mpe_siSetMMSRef()</i>
 *
 * @param service_handle is the service handle to set the mode for
 * @param mm_ref is the index into modulation_mode table to retrieve the mode
 *
 * @return RMF_SI_SUCCESS if successful, else
 *          return appropriate rmf_Error
 *
 */
void rmf_OobSiCache::SetMMSRef(rmf_SiServiceHandle service_handle, uint8_t mm_ref)
{
    rmf_SiTableEntry * si_entry = (rmf_SiTableEntry *) service_handle;

    /* Parameter check */
    if (service_handle == RMF_SI_INVALID_HANDLE)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI",
                "<mpe_siSetMMSRef> RMF_SI_INVALID_PARAMETER\n");
    }
    else
    {
        /* no way to check validity of MM_reference. So just accept it */
        /* Set the MM_reference index */
        si_entry->mode_index = mm_ref;
    }
}

/**
 *  SVCT
 *
 * Set the channel_type for the given service entry
 *
 * <i>mpe_siSetChannelType()</i>
 *
 * @param service_handle is the service handle to set the mode for
 * @param channel_type is the value of the channel_type field of the
 *        SVCT VCM record.
 *
 * @return RMF_SI_SUCCESS if successful, else
 *            return appropriate rmf_Error
 *
 */
void rmf_OobSiCache::SetChannelType(rmf_SiServiceHandle service_handle,
        uint8_t channel_type)
{
    rmf_SiTableEntry * si_entry = (rmf_SiTableEntry *) service_handle;

    /* Parameter check */
    if (service_handle == RMF_SI_INVALID_HANDLE)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI",
                "<mpe_siSetChannelType> RMF_SI_INVALID_PARAMETER\n");
    }
    else
    {
        /* Set the MM_reference index */
        si_entry->channel_type = (rmf_SiChannelType)channel_type;
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                "<mpe_siSetChannelType> channel_type: %d...\n", channel_type);
    }
}

/**
 *  SVCT
 *
 *  Set the video standard for the given analog service entry
 *
 * <i>mpe_siSetVideoStandard()</i>
 *
 * @param service_handle is the service handle to set the video standard for
 * @param video_std is the video standard (NTSC, PAL etc.)
 *
 * @return RMF_SI_SUCCESS if successful, else
 *          return appropriate rmf_Error
 *
 */
void rmf_OobSiCache::SetVideoStandard(rmf_SiServiceHandle service_handle,
        rmf_SiVideoStandard video_std)
{
    rmf_SiTableEntry * si_entry = (rmf_SiTableEntry *) service_handle;

    /* Parameter check */
    if ((service_handle == RMF_SI_INVALID_HANDLE))
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI",
                "<mpe_siSetVideoStandard> RMF_SI_INVALID_PARAMETER\n");
    }
    else
    {
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                "<mpe_siSetVideoStandard> video_std: %d\n", video_std);
        si_entry->video_standard = video_std;
        // We know that an analog service does, and always will, have no components
        //si_entry->number_elem_streams = 0;
        //si_entry->pmt_status = PMT_AVAILABLE;
    }
}

/**
 *  SVCT
 *
 *  Set the service type service entry
 *
 * <i>mpe_siSetServiceType()</i>
 *
 * @param service_handle is the service handle to set the service type
 * @param service_type is the service type
 *
 * @return RMF_SI_SUCCESS if successful, else
 *          return appropriate rmf_Error
 *
 */
void rmf_OobSiCache::SetServiceType(rmf_SiServiceHandle service_handle,
        rmf_SiServiceType service_type)
{
    rmf_SiTableEntry * si_entry = (rmf_SiTableEntry *) service_handle;

    /* Parameter check */
    if (service_handle == RMF_SI_INVALID_HANDLE)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI",
                "<mpe_siSetServiceType> RMF_SI_INVALID_PARAMETER\n");
    }
    else
    {
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                "<mpe_siSetServiceType> service_type: %d\n", service_type);
        si_entry->service_type = service_type;
    }
}

void rmf_OobSiCache::SetChannelMapId( uint16_t ChMapId )
{
	ChannelMapId = ChMapId;
}
void rmf_OobSiCache::GetChannelMapId( uint16_t *pChannelMapId )
{
	*pChannelMapId = ChannelMapId;
}

void rmf_OobSiCache::SetDACId( uint16_t dacId )
{
	DACId = dacId;
}

void rmf_OobSiCache::GetDACId( uint16_t *pDACId )
{
	*pDACId = DACId;
}

void rmf_OobSiCache::SetVCTId( uint16_t vctId )
{
	VCTId = vctId;
}

void rmf_OobSiCache::GetVCTId( uint16_t *pVCTId )
{
	*pVCTId = VCTId;
}

rmf_Error rmf_OobSiCache::rmf_SI_registerSIGZ_Callback(rmf_siGZCallback_t func)
{
    rmf_siGZCBfunc = func;
    return RMF_SUCCESS;
}

#if 0
/**
 *  SVCT
 *
 *  Set the modulation mode for the given analog service entry
 *
 * <i>mpe_siSetAnalogMode()</i>
 *
 * @param service_handle is the service handle to set the mode for
 * @param mode is the analog modulation mode
 *
 * @return RMF_SI_SUCCESS if successful, else
 *          return appropriate rmf_Error
 *
 */
void rmf_OobSiCache::SetAnalogMode(rmf_SiServiceHandle service_handle,
        rmf_ModulationMode mode)
{
    rmf_SiTableEntry * si_entry = (rmf_SiTableEntry *) service_handle;

    /* Parameter check */
    if (service_handle == RMF_SI_INVALID_HANDLE)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI",
                "<mpe_siSetAnalogMode> RMF_SI_INVALID_PARAMETER\n");
    }
    else
    {
        /* Set the analog modulation mode (always 255) */
        ((rmf_SiTransportStreamEntry *) (si_entry->ts_handle))->modulation_mode
                = mode;
    }
}
#endif

/**
 *  SVCT
 *
 *  Set the descriptor in SVCT
 *
 * <i>mpe_siSetSVCTDescriptor()</i>
 *
 * @param service_handle is the service handle to set the descriptor for
 * @param tag is the descriptor tag field
 * @param length is the length of the descriptor
 * @param content is descriptor content
 *
 * @return RMF_SI_SUCCESS if successful, else
 *          return appropriate rmf_Error
 *
 */
rmf_Error rmf_OobSiCache::SetSVCTDescriptor(rmf_SiServiceHandle service_handle,
        rmf_SiDescriptorTag tag, uint32_t length, void *content)
{
    rmf_SiTableEntry *si_entry = NULL;
    uint8_t *arr = NULL;

    /* Parameter check */
    if (service_handle == RMF_SI_INVALID_HANDLE)
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    /* Set the Descriptor Info */
    si_entry = (rmf_SiTableEntry *) service_handle;

    /*  For now we only care about channel properties descriptor (to retrieve service type)
     and two part channel number descritor for major, minor channel numbers.
     These two are optional descriptors in SVCT, hence may not be there.
     */

    /* No need to store SVCT descriptors in raw format. */

    /* Populate service type from channel_properties_descriptor */
    if (tag == RMF_SI_DESC_CHANNEL_PROPERTIES)
    {
        /* channel properties descriptor also contains transport stream Id */
        /* First 2 bytes of desc content - channel_TSID */
        //data_ptr = (uint16_t*)content;
        //si_entry->tsId = ntohs(*data_ptr);

        arr = (uint8_t*) content;
        si_entry->service_type = (rmf_SiServiceType) arr[length - 1];
    }
    /* Populate major/minor numbers from two_part_channel_number_descriptor */
    else if (tag == RMF_SI_DESC_TWO_PART_CHANNEL_NUMBER)
    {
        arr = (uint8_t*) content;
        si_entry->major_channel_number = (arr[0] & 0x03) << 8 | (arr[1] & 0xFF);
        si_entry->minor_channel_number = (arr[2] & 0x03) << 8 | (arr[3] & 0xFF);
    }

    return RMF_SI_SUCCESS;
}

rmf_Error rmf_OobSiCache::GetSourceNameEntry(uint32_t id, rmf_osal_Bool isAppId,  rmf_siSourceNameEntry **source_name_entry, rmf_osal_Bool create)
{
    rmf_siSourceNameEntry *sn_walker;
    rmf_Error retVal = RMF_SI_SUCCESS;
    RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI", "<GetSourceNameEntry> id:0x%x (%s)\n",
            id, isAppId ? "AppID" : "SourceID");
    if ((source_name_entry == NULL) || ((id == 0) || (id > 65535)))
    {
        retVal = RMF_SI_INVALID_PARAMETER;
    }
    else
    {
        // FOR NOW Assume write lock already obtained!!!!
        *source_name_entry = NULL;
        sn_walker = g_si_sourcename_entry;
        while (sn_walker != NULL)
        {
            if (sn_walker->id == id && (sn_walker->appType == isAppId))
            {
                *source_name_entry = sn_walker;
                break;
            }
            else
            {
              sn_walker = sn_walker->next;
            }
        }
        //
        // If walker is NULL, entry not found
        if (sn_walker==NULL && create)
        {
            rmf_osal_memAllocP(RMF_OSAL_MEM_SI_CACHE_OOB, sizeof(rmf_siSourceNameEntry), (void **) &(sn_walker));
            if (sn_walker == NULL)
            {
                RDK_LOG(RDK_LOG_WARN, "LOG.RDK.SI",
                        "<GetSourceNameEntry> Mem allocation failed, returning RMF_SI_OUT_OF_MEM...\n");
                retVal = RMF_SI_OUT_OF_MEM;
            }
            else
            {
                sn_walker->appType = isAppId;
                sn_walker->mapped = false;
                sn_walker->id = id;
                sn_walker->source_long_names = NULL;
                sn_walker->source_names = NULL;
                // just put it at the front.
                sn_walker->next = g_si_sourcename_entry;
                g_si_sourcename_entry = sn_walker;
                *source_name_entry = sn_walker;
                RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",  "<GetSourceNameEntry> id:0x%x (%s), *sn_entry=%p\n",
                         id, isAppId ? "AppID" : "SourceID", *source_name_entry);
            }
        }
    }
   
    return retVal;
}

rmf_osal_Bool rmf_OobSiCache::IsSourceNameLanguagePresent(rmf_siSourceNameEntry *source_name_entry, char *language)
{
  rmf_SiLangSpecificStringList *ln_walker = NULL;
    //RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<GetSourceNameEntry> id:0x%x (%s)\n",
    //        id, isAppId ? "AppID" : "SourceID");
    if ((source_name_entry != NULL) && (language != NULL))
    {
        ln_walker = source_name_entry->source_names;
        while (ln_walker != NULL)
        {
          if (!strcmp(language, ln_walker->language) )
          {
            return(TRUE);
          }
          ln_walker = ln_walker->next;
        }
    }
    return FALSE;
}
rmf_Error rmf_OobSiCache::SetTransportType(rmf_SiServiceHandle service_handle, uint8_t transportType)
{
    rmf_SiTableEntry * si_entry = (rmf_SiTableEntry *) service_handle;
    /* Parameter check */
    if (service_handle == RMF_SI_INVALID_HANDLE)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI",
                "<mpe_siSetTransportType> RMF_SI_INVALID_PARAMETER\n");
    }
    else
    {
        si_entry->transport_type = transportType;
    }
    return RMF_SI_SUCCESS;
}
rmf_Error rmf_OobSiCache::SetScrambled(rmf_SiServiceHandle service_handle, rmf_osal_Bool scrambled)
{
    rmf_SiTableEntry * si_entry = (rmf_SiTableEntry *) service_handle;
    /* Parameter check */
    if (service_handle == RMF_SI_INVALID_HANDLE)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI",
                "<mpe_siSetScrambled> RMF_SI_INVALID_PARAMETER\n");
    }
    else
    {
        si_entry->scrambled = scrambled;
    }
    return RMF_SI_SUCCESS;
}

/**
 *  Internal Method to initialize service entries to default values
 *
 * <i>init_si_entry()</i>
 *
 * @param si_entry is the service entry to set the default values for
 *
 * @return RMF_SI_SUCCESS if successful, else
 *          return appropriate rmf_Error
 *
 */
void rmf_OobSiCache::init_si_entry(rmf_SiTableEntry *si_entry)
{
    // use static struct initialization

    /* set default values as defined in OCAP spec (annex T) */
    /* These are in the order they appear in the struct, to make it
     easier to verify all have been initialized. --mlb */
    si_entry->ref_count = 1;
    si_entry->activation_time = 0;
    rmf_osal_timeGetMillis(&(si_entry->ptime_service));
#if 0
    si_entry->ts_handle = RMF_SI_INVALID_HANDLE;
    si_entry->program = NULL;
#endif
    si_entry->next = NULL;
    si_entry->isAppType = FALSE;
    si_entry->source_id = SOURCEID_UNKNOWN;
    si_entry->app_id = 0;
    si_entry->channel_type = CHANNEL_TYPE_HIDDEN; /* */
    si_entry->video_standard = VIDEO_STANDARD_UNKNOWN;
    si_entry->source_name_entry = NULL; /* default value */
    si_entry->descriptions = NULL; /* default value */
    si_entry->major_channel_number = RMF_SI_DEFAULT_CHANNEL_NUMBER; /* default value */
    si_entry->minor_channel_number = RMF_SI_DEFAULT_CHANNEL_NUMBER; /* default value */
    si_entry->service_type = RMF_SI_SERVICE_TYPE_UNKNOWN; /* default value */
    si_entry->freq_index = 0;
    si_entry->mode_index = 0;
#if 0
    si_entry->dsgAttached = FALSE;
    si_entry->dsg_attach_count = 0; // OC over DSG attach count
#endif
    si_entry->state = SIENTRY_UNSPECIFIED;
    si_entry->virtual_channel_number = VIRTUAL_CHANNEL_UNKNOWN;
    si_entry->program_number = 0;
    si_entry->transport_type = 0; // Default is MPEG2
    si_entry->scrambled = FALSE;
#if 0
    si_entry->hn_stream_session = 0;
    si_entry->hn_attach_count = 0;
#endif
}

void rmf_OobSiCache::clone_si_entry(rmf_SiTableEntry *new_entry,
        rmf_SiTableEntry *si_entry)
{
    // Clone fields from one service entry to the other
    // This method is called in cases where a dynamic entry
    // is created first (using f-p-q) and later OOB tables
    // are acquired.
    new_entry->activation_time = si_entry->activation_time;
    new_entry->isAppType = si_entry->isAppType;
    new_entry->source_id = si_entry->source_id;
    new_entry->app_id = si_entry->app_id;
    new_entry->channel_type = si_entry->channel_type;
    new_entry->video_standard = si_entry->video_standard;
    //new_entry->source_names = si_entry->source_names;
    //new_entry->source_long_names = si_entry->source_long_names;
    new_entry->source_name_entry = si_entry->source_name_entry;
    new_entry->descriptions = si_entry->descriptions;
    new_entry->major_channel_number = si_entry->major_channel_number;
    new_entry->minor_channel_number = si_entry->minor_channel_number;
    new_entry->service_type = si_entry->service_type;
    new_entry->freq_index = si_entry->freq_index;
    new_entry->mode_index = si_entry->mode_index;
#if 0
    new_entry->dsgAttached = si_entry->dsgAttached;
    new_entry->dsgAttached = si_entry->dsg_attach_count;
#endif
    new_entry->state = si_entry->state;
    new_entry->virtual_channel_number = si_entry->virtual_channel_number;
    new_entry->program_number = si_entry->program_number;
    new_entry->transport_type = si_entry->transport_type;
    new_entry->scrambled = si_entry->scrambled;
#if 0
    new_entry->hn_stream_session = si_entry->hn_stream_session;
    new_entry->hn_attach_count = si_entry->hn_attach_count;
#endif
}

void rmf_OobSiCache::init_rating_dimension(rmf_SiRatingDimension *dimension,
        char* dimName, char* values[], char* abbrevValues[])
{
    int i;

    // Dimension Name
    if (rmf_osal_memAllocP(RMF_OSAL_MEM_SI_CACHE_OOB, sizeof(rmf_SiLangSpecificStringList),
            (void **) &(dimension->dimension_names)) != RMF_SUCCESS)
    {
        RDK_LOG(RDK_LOG_WARN, "LOG.RDK.SI",
                "Mem allocation failed, returning RMF_SI_OUT_OF_MEM...\n");
        return;
    }
    dimension->dimension_names->next = NULL;
    memset(dimension->dimension_names->language, 0, sizeof(dimension->dimension_names->language));
    strncpy(dimension->dimension_names->language, "eng", sizeof(dimension->dimension_names->language)-1);
    dimension->dimension_names->string = dimName;

    // Dimension Values
    if (rmf_osal_memAllocP(RMF_OSAL_MEM_SI_CACHE_OOB, sizeof(rmf_SiRatingValuesDefined)
            * dimension->values_defined, (void **) &(dimension->rating_value))
            != RMF_SUCCESS)
    {
        RDK_LOG(RDK_LOG_WARN, "LOG.RDK.SI",
                "Mem allocation failed, returning RMF_SI_OUT_OF_MEM...\n");
        return;
    }

    for (i = 0; i < dimension->values_defined; ++i)
    {
        rmf_SiRatingValuesDefined *rating_values =
                &(dimension->rating_value[i]);

        // Rating Value
        if (rmf_osal_memAllocP(RMF_OSAL_MEM_SI_CACHE_OOB, sizeof(rmf_SiLangSpecificStringList),
                (void **) &rating_values->rating_value_text) != RMF_SUCCESS)
        {
            RDK_LOG(RDK_LOG_WARN, "LOG.RDK.SI",
                    "Mem allocation failed, returning RMF_SI_OUT_OF_MEM...\n");
            return;
        }
        rating_values->rating_value_text->next = NULL;
        memset(rating_values->rating_value_text->language, 0, sizeof(rating_values->rating_value_text->language));
        strncpy(rating_values->rating_value_text->language, "eng", sizeof(rating_values->rating_value_text->language)-1);
        rating_values->rating_value_text->string = values[i];

        // Abbreviated Rating Value
        if (rmf_osal_memAllocP(RMF_OSAL_MEM_SI_CACHE_OOB, sizeof(rmf_SiLangSpecificStringList),
                (void **) &rating_values->abbre_rating_value_text)
                != RMF_SUCCESS)
        {
            RDK_LOG(RDK_LOG_WARN, "LOG.RDK.SI",
                    "Mem allocation failed, returning RMF_SI_OUT_OF_MEM...\n");
            return;
        }
        rating_values->abbre_rating_value_text->next = NULL;
        memset(rating_values->abbre_rating_value_text->language, 0, sizeof(rating_values->abbre_rating_value_text->language));
        strncpy(rating_values->abbre_rating_value_text->language, "eng", sizeof(rating_values->abbre_rating_value_text->language)-1);
        rating_values->abbre_rating_value_text->string = abbrevValues[i];
    }
}

/**
 *  Internal Method to initialize default rating region (US (50 states + possesions))
 *
 * <i>init_default_rating_region(void)</i>
 *
 * @param  NONE
 *
 * @return NONE
 *
 * NOTE:  Future impl may take a default rating region enum type as
 * param.
 *
 */
void rmf_OobSiCache::init_default_rating_region(void)
{
    RDK_LOG(
            RDK_LOG_TRACE1,
            "LOG.RDK.SI",
            "<init_default_rating_region> Initializing the default rating region - US (50 states + possessions)\n");

    init_rating_dimension(&g_US_RRT_dim_Entire_Audience,
            (char*)g_US_RRT_eng_dim_Entire_Audience,
            (char**)g_US_RRT_eng_vals_Entire_Audience,
            (char**)g_US_RRT_eng_abbrev_vals_Entire_Audience);
    init_rating_dimension(&g_US_RRT_dim_Dialogue, (char*)g_US_RRT_eng_dim_Dialogue,
            (char**)g_US_RRT_eng_vals_Dialogue, (char**)g_US_RRT_eng_abbrev_vals_Dialogue);
    init_rating_dimension(&g_US_RRT_dim_Language, (char*)g_US_RRT_eng_dim_Language,
            (char**)g_US_RRT_eng_vals_Language, (char**)g_US_RRT_eng_abbrev_vals_Language);
    init_rating_dimension(&g_US_RRT_dim_Sex, (char*)g_US_RRT_eng_dim_Sex,
            (char**)g_US_RRT_eng_vals_Sex, (char**)g_US_RRT_eng_abbrev_vals_Sex);
    init_rating_dimension(&g_US_RRT_dim_Violence, (char*)g_US_RRT_eng_dim_Violence,
            (char**)g_US_RRT_eng_vals_Violence, (char**)g_US_RRT_eng_abbrev_vals_Violence);
    init_rating_dimension(&g_US_RRT_dim_Children, (char*)g_US_RRT_eng_dim_Children,
            (char**)g_US_RRT_eng_vals_Children, (char**)g_US_RRT_eng_abbrev_vals_Children);
    init_rating_dimension(&g_US_RRT_dim_Fantasy_Violence,
            (char*)g_US_RRT_eng_dim_Fantasy_Violence,
            (char**)g_US_RRT_eng_vals_Fantasy_Violence,
            (char**)g_US_RRT_eng_abbrev_vals_Fantasy_Violence);
    init_rating_dimension(&g_US_RRT_dim_MPAA, (char*)g_US_RRT_eng_dim_MPAA,
            (char**)g_US_RRT_eng_vals_MPAA, (char**)g_US_RRT_eng_abbrev_vals_MPAA);

    /*
     * fill the global dimension array ( size = number of US rating region dimensions)
     * with the the US rating region dimensions
     */
    g_RRT_US_dimension[0] = g_US_RRT_dim_Entire_Audience;
    g_RRT_US_dimension[1] = g_US_RRT_dim_Dialogue;
    g_RRT_US_dimension[2] = g_US_RRT_dim_Language;
    g_RRT_US_dimension[3] = g_US_RRT_dim_Sex;
    g_RRT_US_dimension[4] = g_US_RRT_dim_Violence;
    g_RRT_US_dimension[5] = g_US_RRT_dim_Children;
    g_RRT_US_dimension[6] = g_US_RRT_dim_Fantasy_Violence;
    g_RRT_US_dimension[7] = g_US_RRT_dim_MPAA;

    // Region Name
    if (rmf_osal_memAllocP(RMF_OSAL_MEM_SI_CACHE_OOB, sizeof(rmf_SiLangSpecificStringList),
            (void **) &(g_RRT_US.rating_region_name_text)) != RMF_SUCCESS)
    {
        RDK_LOG(RDK_LOG_WARN, "LOG.RDK.SI",
                "Mem allocation failed, returning RMF_SI_OUT_OF_MEM...\n");
        return;
    }
    g_RRT_US.rating_region_name_text->next = NULL;
    memset(g_RRT_US.rating_region_name_text->language, 0, sizeof(g_RRT_US.rating_region_name_text->language));
    strncpy(g_RRT_US.rating_region_name_text->language, "eng", sizeof(g_RRT_US.rating_region_name_text->language)-1);
    g_RRT_US.rating_region_name_text->string = (char*)g_RRT_US_eng;
}

rmf_Error rmf_OobSiCache::ReleaseServiceHandle(rmf_SiServiceHandle service_handle)
{
    rmf_SiTableEntry *si_entry = NULL;
    rmf_SiTableEntry *te_walker;

    // Parameter check
    if (service_handle == RMF_SI_INVALID_HANDLE)
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    si_entry = (rmf_SiTableEntry *) service_handle;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<mpe_siReleaseServicehandle> ref_count %d\n", si_entry->ref_count);

    if (si_entry->ref_count <= 1)
    {
        // ref_count should never be less than 1, it means that
        // there may be un-balanced get/release service handles
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                "<mpe_siReleaseServiceHandle> error ref_count %d\n",
                si_entry->ref_count);
        return RMF_SI_INVALID_PARAMETER;
    }

    --(si_entry->ref_count);

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
            "<mpe_siReleaseServiceHandle> ref_count %d\n", si_entry->ref_count);

    if (si_entry->ref_count == 1)
    {
        te_walker = g_si_entry;
        //first node of the list
        if (te_walker && (te_walker == si_entry))
        {
            g_si_entry = te_walker->next;
        }
        else
        {
            while (te_walker && te_walker->next)
            {
                if (te_walker->next == si_entry)
                {
                    RDK_LOG(
                            RDK_LOG_TRACE1,
                            "LOG.RDK.SI",
                            "<mpe_siReleaseServiceHandle> set %p->next to %p\n",
                            te_walker, te_walker->next->next);
                    te_walker->next = te_walker->next->next;
                    break;
                }
                te_walker = te_walker->next;
            }
        }

        delete_service_entry(si_entry);
    }

    return RMF_SI_SUCCESS;
}

void rmf_OobSiCache::delete_service_entry(rmf_SiTableEntry *si_entry)
{
    if (si_entry != NULL)
    {
        release_si_entry(si_entry);

        // decrement the service count
        --g_numberOfServices;

        //RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<send_service_update_event>  \n");
        //need to send remove event??

        // Acquire registration list mutex
        //  rmf_osal_mutexAcquire(g_registration_list_mutex);

        //  walker = g_si_registration_list;

        rmf_osal_memFreeP(RMF_OSAL_MEM_SI_CACHE_OOB, si_entry);
    }
}

#ifdef SI_DONT_COMMENT_OUT
static rmf_Error delete_marked_service_entries()
{
    rmf_SiTableEntry **walker;
    rmf_SiTableEntry *prev;
    rmf_SiTableEntry *to_delete = NULL;
    rmf_SiTableEntry *si_entry = NULL;
    rmf_osal_Bool endoflist = false;
    rmf_osal_Bool done = false;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "removeMarkedEntries()\n");

    if (!g_modulationsUpdated || !g_frequenciesUpdated || !g_channelMapUpdated)
    {
        return;
    }

    // Get global lock
    rmf_osal_condGet(g_global_si_cond);

    walker = &g_si_entry;

    while (*walker && !done)
    {
        if ( (*walker)->state == SIENTRY_OLD )
        {
            if ((*walker)->ref_count > 1)
            {
                // Send event to Java that this service has been removed!!
                // Delete it when the ref_count reaches 1
                si_entry = *walker;

                // increment the ref_count?
                ++si_entry->ref_count;

                send_service_update_event(si_entry, RMF_SI_CHANGE_TYPE_REMOVE);
            }
            else if ((*walker)->ref_count == 1)
            {
                // First delete the elementary streams and other
                // allocations made on behalf of the service entry.
                // Elementary streams themselves may have references
                // held by the java layer. But, service components can
                // float around un-coupled from the service itself and
                // their references are cleaned up independently.
                // We only need to account for cleaning up of 'service'
                // entries in this method.
                to_delete = *walker;
            }

            *walker = (*walker)->next;
            //break;
        }
        else
        {
            if ((*walker)->next != NULL)
            {
                walker = &((*walker)->next);

                if ((*walker)->next == NULL)
                endoflist = true;
            }
            else
            {
                // *walker = NULL;
                done = 1;
            }
        }

        if (to_delete != NULL)
        {
            delete_service_entry(to_delete);
        }

    }

    rmf_osal_condSet(g_global_si_cond);
    // Release global lock

    return RMF_SI_SUCCESS;
}

static void send_service_update_event(rmf_SiTableEntry *si_entry, uint32_t changeType)
{
    mpe_Event event = RMF_SI_EVENT_SERVICE_UPDATE;
    rmf_SiRegistrationListEntry *walker = NULL;

    if (si_entry == NULL)
    {
        return;
    }

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<send_service_update_event>  \n");

    // Acquire registration list mutex
    rmf_osal_mutexAcquire(g_registration_list_mutex);

    walker = g_si_registration_list;

    while (walker)
    {
        uint32_t termType = walker->edHandle->terminationType;

        mpe_eventQueueSend(walker->edHandle->eventQ,
                event,
                (void*)si_entry, (void*)walker->edHandle,
                changeType);

        // Do we need to unregister this client?
        if (termType == RMF_ED_TERMINATION_ONESHOT ||
                (termType == RMF_ED_TERMINATION_EVCODE && event == walker->terminationEvent))
        {
            if ( (*walker)->deleteSelf )
            {
                to_delete = *walker;
                *walker = (*walker)->next;
                rmf_osal_memFreeP(RMF_OSAL_MEM_SI_CACHE_OOB,to_delete);
                to_delete = NULL;
                break;
            }

            walker = walker->next;
        }

        // Unregister all clients that have been added to unregistration list
        unregister_clients();

        // Release registration list mutex
        rmf_osal_mutexRelease(g_registration_list_mutex);
    }
}
#endif /* #ifdef SI_DONT_COMMENT_OUT */


/**
 * Debug status function to acquire the number of SI entries in the SIDB.
 *
 * @param status is pointer for returning the count.
 *
 * @return RMF_SUCCESS upon successfully returning the count.
 */
rmf_Error rmf_OobSiCache::getSiEntryCount(void *status)
{
    rmf_SiTableEntry *walker;
    uint32_t count = 0;

    /* Acquire global mutex */
    rmf_osal_mutexAcquire(g_global_si_mutex);

    /* Use internal method release_si_entry() to delete service entries */
    for (walker = g_si_entry; walker; walker = walker->next)
        count++;

    rmf_osal_mutexRelease(g_global_si_mutex);

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "si_getSiEntryCount() - count = %d\n",
            count);

    *(uint32_t*) status = count;
    return RMF_SUCCESS;
}

/**
 * Debug status function to acquire status information for a specific SI entry from the SIDB.
 * The entry to acquire is identified by its index location within the SI entry list.
 *
 * @param index is the index of the entry to acquire.
 * @param size is a pointer to the size of the information buffer used to return the status.
 * @param status is a pointer to the information buffer.
 * @param statBuff_1024 is a pointer to local SIDB buffer used to generate the status information.
 *
 * @return RMF_SUCCESS upon successfully acquiring the status information.
 */
rmf_Error rmf_OobSiCache::getSiEntry(uint32_t index, uint32_t *size, void *status,
        void *statBuff_1024)
{
    rmf_SiTableEntry *si_entry;
    uint32_t i;
    uint32_t statSz = 0;
    uint32_t maxSize;

    /* Acquire global mutex */
    rmf_osal_mutexAcquire(g_global_si_mutex);

    /* Iterate to the target si entry. */
    for (i = 0, si_entry = g_si_entry; si_entry != NULL; si_entry
            = si_entry->next, ++i)
    {
        if (i == index)
        {
#if 0   
            statSz
                    = sprintf(
                            statBuff_1024,
                            "%3d: CHAN_TYP:%d F:%d MJ:%d MN:%d MOD_IDX:%d MOD_MODE:%d PRG_NUM:%d SRC_ID:%d/0x%x TNR_ID:%d TS_HAN:%u, SVC_TYP:%d, \n",
                            (int) i + 1, si_entry->channel_type,
                            si_entry->freq_index,
                            (int) (g_frequency[si_entry->freq_index]),
                            (int) si_entry->major_channel_number,
                            (int) si_entry->minor_channel_number,
                            (int) si_entry->mode_index,
                            g_mode[si_entry->mode_index],
                            (int) si_entry->source_id,
                            (int) si_entry->source_id,
                            (int) si_entry->tuner_id,
                            (unsigned int) si_entry->ts_handle,
                            si_entry->service_type);
#else
            statSz
                    = sprintf(
                            (char*)statBuff_1024,
                            "%3d: CHAN_TYP:%d F_INDX:%d F:%d MJ:%d MN:%d MOD_IDX:%d MOD_MODE:%d PRG_NUM:%d SRC_ID:%d/0x%x TNR_ID:%d SVC_TYP:%d, \n",
                            (int) i + 1, si_entry->channel_type,
                            si_entry->freq_index,
                            (int) (g_frequency[si_entry->freq_index]),
                            (int) si_entry->major_channel_number,
                            (int) si_entry->minor_channel_number,
                            (int) si_entry->mode_index,
                            g_mode[si_entry->mode_index],
                            (int) si_entry->program_number,
                            (int) si_entry->source_id,
                            (int) si_entry->source_id,
                            (int) si_entry->tuner_id,
                            si_entry->service_type);
#endif
            break;
        }
    }
    /* Get the size of the return buffer. */
    maxSize = *size;

    /* Copy the status information (up to buffer size). */
    memcpy(status, statBuff_1024, (statSz + 1 <= maxSize) ? statSz + 1
            : maxSize);

    /* Release global mutex. */
    rmf_osal_mutexRelease(g_global_si_mutex);

    /* Return the size of information copied. */
    *size = (uint32_t)(statSz + 1 <= maxSize ? statSz + 1 : maxSize);

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "si_getSiEntry(%d) - entry = %s\n",
            index, (i == index) ? (char*)statBuff_1024 : "NOT FOUND!");

    return ((statSz + 1 > maxSize) ? RMF_OSAL_EINVAL : RMF_SUCCESS);
}

#if 0
/**
 * Acquire the specified status information from the SIDB.  This function is called directly
 * by the MPE debug manager in support of the mpe_dbgStatus() API.
 *
 * @param type is the type of information identifier
 * @param size is a pointer to a size value indicating the maximum size of the buffer, which
 *        will be updated with the actual size returned.
 * @param status is the structure of buffer pointer to use to return the information.
 * @param params is an optional pointer to a structure that may further define/specify
 *        the nature or constraints of the information request.
 *
 * @return RMF_SUCCESS upon success in returning the requested information.
 */
rmf_Error rmf_OobSiCache::AcquireDbgStatus(mpe_DbgStatusId type, uint32_t *size, void *status,
        void *params)
{
    switch (type & RMF_DBG_STATUS_TYPEID_MASK)
    {
    case RMF_DBG_STATUS_SI_ENTRY_COUNT:
        if (*size > sizeof(uint32_t))
            return RMF_OSAL_EINVAL;
        *size = sizeof(uint32_t);
        return getSiEntryCount(status);
    case RMF_DBG_STATUS_SI_ENTRY:
        if (*size > sizeof(statBuff_1024))
            return RMF_OSAL_EINVAL;
        return getSiEntry(*(uint32_t*) params, size, status, statBuff_1024);
    default:
        return RMF_OSAL_EINVAL;
    }
}
#endif

/*****************************************************************
*  Retrieve Channel Number to diplay on the Boot Splash Screen
*
******************************************************************/
void rmf_OobSiCache::BootEventSIGetData(int* channel)
{
    int tmp=0;
    GetTotalNumberOfServices ((uint32_t*)&tmp);
    *channel=tmp;

    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "-------------KELLY Channel Count=%d \n", tmp);
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "-------------KELLY g_numberOfServices =%d \n", g_numberOfServices);
}

#if 0
/*****************************************************************
*  Retrieve Enhanced Diagnostics
*
******************************************************************/
rmf_Error rmf_OobSiCache::GetSiDiagnostics(RMF_SI_DIAG_INFO * pDiagnostics)
{
    pDiagnostics->g_si_state            = g_si_state;
    pDiagnostics->g_SITPConditionSet    = g_SITPConditionSet;
    
    //FLEC: this is hardcoded because we have multiple definition (probably in VL headers or code)
    //Will remove once we resolve the issue
    //Note: The global variables have now been moved to sitp_si.c instead of this file
    //pDiagnostics->g_siOOBCacheEnable    = true;
    //pDiagnostics->g_siOOBCached         = true;
    
    pDiagnostics->g_siOOBCacheEnable    = g_siOOBCacheEnable;
    pDiagnostics->g_siOOBCached         = g_siOOBCached;
    pDiagnostics->g_siSTTReceived       = g_siSTTReceived;
    pDiagnostics->g_maxFrequency        = g_maxFrequency;
    pDiagnostics->g_minFrequency        = g_minFrequency;
    pDiagnostics->g_numberOfServices    = g_numberOfServices;
    pDiagnostics->g_frequenciesUpdated  = g_frequenciesUpdated;
    pDiagnostics->g_modulationsUpdated  = g_modulationsUpdated;
    pDiagnostics->g_channelMapUpdated   = g_channelMapUpdated;
    
    return RMF_SUCCESS;
}
#endif

/**
 *  Internal Method to release service entries
 *
 * <i>release_si_entry()</i>
 *
 * @param si_entry is the service entry to delete
 *
 *
 */
void rmf_OobSiCache::release_si_entry(rmf_SiTableEntry *si_entry)
{
        if (si_entry == NULL)
        {
                return;
        }

        /* Free service descriptions */
        if (si_entry->descriptions != NULL)
        {
                rmf_SiLangSpecificStringList *walker = si_entry->descriptions;

                while (walker != NULL)
                {
                        rmf_SiLangSpecificStringList *toBeFreed = walker;

                        rmf_osal_memFreeP(RMF_OSAL_MEM_SI_CACHE_OOB, walker->string);

                        walker = walker->next;

                        rmf_osal_memFreeP(RMF_OSAL_MEM_SI_CACHE_OOB, toBeFreed);
                }

            si_entry->descriptions = NULL;
        }
}

void rmf_OobSiCache::TouchFile(const char * pszFile)
{
    if(NULL != pszFile)
    {
        FILE * fp = fopen(pszFile, "w");
        if(NULL != fp)
        {
            fclose(fp);
        }
    }
}


/*****************************************************************
*  Retrieve Enhanced Diagnostics
*
******************************************************************/
rmf_Error rmf_OobSiCache::GetSiDiagnostics(RMF_SI_DIAG_INFO * pDiagnostics)
{
    pDiagnostics->g_si_state            = g_si_state;
    pDiagnostics->g_SITPConditionSet    = g_SITPConditionSet;

    //FLEC: this is hardcoded because we have multiple definition (probably in VL headers or code)
    //Will remove once we resolve the issue
    //Note: The global variables have now been moved to sitp_si.c instead of this file
    //pDiagnostics->g_siOOBCacheEnable    = true;
    //pDiagnostics->g_siOOBCached         = true;

//    pDiagnostics->g_siOOBCacheEnable    = g_siOOBCacheEnable;
//    pDiagnostics->g_siOOBCached         = g_siOOBCached;

    pDiagnostics->g_siSTTReceived       = g_siSTTReceived;
    pDiagnostics->g_maxFrequency        = g_maxFrequency;
    pDiagnostics->g_minFrequency        = g_minFrequency;
    pDiagnostics->g_numberOfServices    = g_numberOfServices;
    pDiagnostics->g_frequenciesUpdated  = g_frequenciesUpdated;
    pDiagnostics->g_modulationsUpdated  = g_modulationsUpdated;
    pDiagnostics->g_channelMapUpdated   = g_channelMapUpdated;
    return RMF_SUCCESS;	
}

rmf_OobSiCache *rmf_OobSiCache::getSiCache()
{
    pthread_mutex_lock(&g_pSiCacheMutex);
    if(m_pSiCache == NULL)
    {
        m_pSiCache = new rmf_OobSiCache();
        m_pSiCache->Configure();
         // Initialize the CRC calculator
        m_pSiCache->init_mpeg2_crc();
        
    }
    pthread_mutex_unlock(&g_pSiCacheMutex);
    return m_pSiCache;
}

void rmf_OobSiCache::get_frequency_range(uint32_t *freqArr, int count, uint32_t *minFreq, uint32_t *maxFreq )
{
    int i = 1;
    uint32_t minVal = 0, maxVal = 0;

    if(count && freqArr[0])
    {

    	minVal = freqArr[0];
        maxVal = freqArr[0];;
        while ((i < count) && freqArr[i])
        {
            if (maxVal < freqArr[i])
            {
                maxVal = freqArr[i];
            }
            else if(minVal > freqArr[i])
            {
            	minVal = freqArr[i];
            }
            i++;
        }
    }

    *minFreq = minVal;
    *maxFreq = maxVal;
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI","<get_frequency_range> g_minFrequency: %d, g_maxFrequency: %d count:%d \n", *minFreq, *maxFreq, count);

    return;
}

rmf_Error rmf_OobSiCache::read_name_string(rmf_osal_File cache_fd, rmf_SiLangSpecificStringList *name, uint32_t *read_count, int total_bytes)
{
    uint32_t count = 0;
    rmf_Error ret = RMF_SUCCESS;
    rmf_SiLangSpecificStringList *name_walker = name;

    if (name_walker)
    {
        uint32_t size=0;
        count = sizeof(name_walker->language);
        ret = rmf_osal_fileRead(cache_fd, &count, (void *)name_walker->language);
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI", "count: %d, name_walker->language: %s, ret: %d \n", count, name_walker->language, ret);
        if(ret == RMF_SUCCESS && count == sizeof(name_walker->language))
        {
            *read_count += count;
            total_bytes -= count;

            if(total_bytes > 0)
            {
                count = sizeof(uint32_t);
                ret = rmf_osal_fileRead(cache_fd, &count, (void *)&size);
                if(NAME_COUNT_MAX < size)
                {
                    size = NAME_COUNT_MAX; /*limitted this to 32Kb; review this value*/
                }
                if (ret == RMF_SUCCESS && count == sizeof(uint32_t))
                {
                    *read_count += count;
                    total_bytes -= count;
                    rmf_osal_memAllocP(RMF_OSAL_MEM_SI_CACHE_OOB, size+1, (void **)&(name_walker->string));
                    memset(name_walker->string, 0, size + 1);
                    count = size;
                
                    ret = rmf_osal_fileRead(cache_fd, &count, (void *)name_walker->string);
                    *read_count += count;
                    total_bytes -= count;                
                }
            }
        }
    }

    if (total_bytes != 0)  {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "ERROR: totalBytes %d != 0 \n", total_bytes);
    }
            
    return ret;
}

rmf_Error rmf_OobSiCache::write_name_string(rmf_osal_File cache_fd, rmf_SiLangSpecificStringList *name, uint32_t *write_count)
{
    rmf_Error err = RMF_SUCCESS;
    int name_len = 0;
    uint32_t count = 0;
    rmf_SiLangSpecificStringList *name_walker = name;
    while(name_walker)
    {
        unsigned char buf[4] = {0};
        /*
         * 0) write total_len = 1)+2)+3)
         * 1)write lanaguage[4]
         * 2)write string_len
         * 3)write string
         */
        name_len = sizeof(name_walker->language) + (name_walker->string ? (strlen(name_walker->string) + sizeof(uint32_t)): 0);
        buf[0] = name_len     & 0xFF;
        buf[1] =(name_len>>8) & 0xFF;
        buf[2] =(name_len>>16)& 0xFF;
        buf[3] =(name_len>>24)& 0xFF;

        count = sizeof(buf);

        err = rmf_osal_fileWrite(cache_fd, &count, (void *)&buf);

        if (count == sizeof(buf))
        {
            *write_count += count;

            count = sizeof(name_walker->language);

            err = rmf_osal_fileWrite(cache_fd, &count, (void *)name_walker->language);

            if (count == sizeof(name_walker->language))
            {
                *write_count += count;

                if (name_walker->string)
                {
                    uint32_t size;
                    size = strlen(name_walker->string);
                    count = sizeof(uint32_t);

                    err = rmf_osal_fileWrite(cache_fd, &count, (void *)&size);
                    if (count == sizeof(uint32_t))
                    {
                        *write_count += count;
                        if (size > 0)
                        {
                          count = strlen(name_walker->string);

                          err = rmf_osal_fileWrite(cache_fd, &count, (void *)name_walker->string);
                          if (count != size)
                          {
                            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "writing SI name failed with %d\n", err);
                            err = RMF_SI_INVALID_PARAMETER;
                          }
                        }
                    }
                    else
                    {
                        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "writing SI name length failed with %d\n", err);
                        err = RMF_SI_INVALID_PARAMETER;
                    }
                }
            }
            else
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "writing SI name language failed with %d\n", err);
                err = RMF_SI_INVALID_PARAMETER;
            }
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "writing SI name total failed with %d\n", err);
            err = RMF_SI_INVALID_PARAMETER;
        }
        name_walker = name_walker->next;
    } // For each name

    return err;
}

rmf_Error rmf_OobSiCache::cache_si_entries(const char *siOOBCacheLocation)
{
    rmf_Error err = RMF_SUCCESS;
    rmf_SiTableEntry *walker = NULL;
    rmf_siSourceNameEntry *source_name_entry = NULL;
    rmf_osal_File cache_fd = NULL;
    rmf_osal_File fd;
    rmf_SiProgramHandle program_info = 0;
    uint32_t num_entry_written = 0;
    uint32_t defined_mapped_count = 0;
    uint32_t count = 0;
    // Parameter check
    {
        if (RMF_SUCCESS != rmf_osal_fileDelete(siOOBCacheLocation))
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "CACHE: Error deleting file...%s\n", siOOBCacheLocation);
        }
        err = rmf_osal_fileOpenBuffered(siOOBCacheLocation, RMF_OSAL_FS_OPEN_WRITE | RMF_OSAL_FS_OPEN_MUST_CREATE, &fd);
        if (err == RMF_SUCCESS)
        {
            cache_fd = fd;
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "open [%s]  failed with %d\n", siOOBCacheLocation, err);
            err = RMF_SI_INVALID_PARAMETER;
        }
    }
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI","CACHE: starting writing cache\n");
    if (err == RMF_SUCCESS)
    {
        uint32_t write_count = 0;
        uint32_t loop_count = 0;
        uint32_t version = RMF_SI_CACHE_FILE_VERSION;

        // Write header version
        count = sizeof(version);
        err = rmf_osal_fileWrite(cache_fd, &count, (void *)&version);

        if (err == RMF_SUCCESS && (count == sizeof(version)))
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI", "writing version to [%s] 0x%x count:%d\n", siOOBCacheLocation, RMF_SI_CACHE_FILE_VERSION, sizeof(version));
            write_count += count;
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "writing version to [%s] failed with %d\n", siOOBCacheLocation, err);
            err = RMF_SI_INVALID_PARAMETER;
        }

        if (err == RMF_SUCCESS)
        {
            // Write frequency table
            count = sizeof(g_frequency);
            err = rmf_osal_fileWrite(cache_fd, &count, (void *)g_frequency);
    
            if (err == RMF_SUCCESS && (count == sizeof(g_frequency)))
            {
                RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "writing FREQ to [%s] %d \n", siOOBCacheLocation, count);
                write_count += count;
            }
            else
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "writing FREQ to [%s] failed with %d\n", siOOBCacheLocation, err);
                err = RMF_SI_INVALID_PARAMETER;
            }
        }

        if (err == RMF_SUCCESS)
        {
            // Write modulation mode table
            count = sizeof(g_mode);
            err = rmf_osal_fileWrite(cache_fd, &count, (void *)g_mode);
            if (err == RMF_SUCCESS && (count == sizeof(g_mode)))
            {
                RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "writing MODE to [%s] %d \n", siOOBCacheLocation, count);
                write_count += count;
            }
            else
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "writing MODE to [%s] failed with %d\n", siOOBCacheLocation, err);
                err = RMF_SI_INVALID_PARAMETER;
            }
        }

        // Write table entries
        walker = g_si_entry;
        while (walker && err == RMF_SUCCESS)
        {
            rmf_SiLangSpecificStringList *name_list = walker->descriptions;
            int                             name_count = 0;
            rmf_SiGenericHandle     ts_handle;

            // OOB, HN??
            if (!( (walker->state == SIENTRY_DEFINED_MAPPED) ||
                   ((walker->state == SIENTRY_MAPPED) &&
                    (walker->ts_handle == (rmf_SiGenericHandle)g_si_dsg_ts_entry))) )
            {
                walker = walker->next;
                continue;
            }

            if(((DAC_SOURCEID == walker->source_id) && (DACId != walker->virtual_channel_number)) ||
                    ((CHMAP_SOURCEID == walker->source_id) && (ChannelMapId != walker->virtual_channel_number)))
            {
                walker = walker->next;
                continue; //Skip outdated entries for controller and channel map ids.
            }

            loop_count++;
            {
                source_name_entry = walker->source_name_entry;
                if (walker->state == SIENTRY_DEFINED_MAPPED)
                {
                    defined_mapped_count++;
                }
                while(name_list)
                {
                    name_count++;
                    name_list = name_list->next;
                }

                walker->source_name_entry   = (rmf_siSourceNameEntry *)0;
                walker->descriptions        = (rmf_SiLangSpecificStringList *)name_count;
                program_info = walker->program;
                if (walker->program != RMF_SI_INVALID_HANDLE)
                {
                    walker->program = PROGRAM_NUMBER_UNKNOWN;
                }
                else
                {
                    walker->program = 0;
                }
                ts_handle = walker->ts_handle;
                if(walker->ts_handle == (rmf_SiGenericHandle)g_si_dsg_ts_entry)
                {
                    walker->ts_handle = RMF_SI_DSG_FREQUENCY;
                    walker->program = PROGRAM_NUMBER_UNKNOWN;
                }

                count = sizeof(*walker);
                err = rmf_osal_fileWrite(cache_fd, &count, (void *)walker);
                walker->source_name_entry   = source_name_entry;
                walker->descriptions        = name_list;
                walker->program             = program_info;
                walker->ts_handle           = ts_handle;

                if (count == sizeof(*walker))
                {
                    write_count += count;
                    num_entry_written++;
                    // now copy names
                    {
                        // LEN-NAME, LEN-NAME...
                        err = write_name_string(cache_fd, name_list, &write_count);
                    }
                }
                else
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "writing SI %s  failed with %d\n", siOOBCacheLocation, err);
                    err = RMF_SI_INVALID_PARAMETER;
                    break;
                }
                if (1)
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI", "WChannelVCN#%06d-SRCID#%06x-Name[%s]-State-[%d]-Freq[%08d]-Mode[%04d]-Prog[%08d]-Vid[%d]-TT[%d]\n",
                        walker->virtual_channel_number,
                        walker->isAppType ? walker->app_id : walker->source_id,
                        (walker->source_name_entry && walker->source_name_entry->source_names && walker->source_name_entry->source_names->string)? walker->source_name_entry->source_names->string : "NULL",
                        walker->state,
                        g_frequency[walker->freq_index],
                        g_mode[walker->mode_index],
                        walker->program_number,
                        walker->video_standard,
                        walker->transport_type);

            }
            walker = walker->next;
        } // For each walker

        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI", "writing /persistent/si/si_table_cache DONE writing SVCT entries %d bytes %d entries\n", sizeof(*walker), loop_count);

        rmf_osal_fileClose(cache_fd);

        if (defined_mapped_count == 0)
        {
            /* Only OOB channels are cached, discard it */
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI", "CACHE: No DEFINED_MAPPED entries...\n");
            if (RMF_SUCCESS != rmf_osal_fileDelete(siOOBCacheLocation))
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "CACHE: Error deleting file...%s\n", siOOBCacheLocation);
            }
        }
    }
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI","CACHE: done writing cache %d, %d\n", num_entry_written, defined_mapped_count);

    return err;
}


rmf_Error rmf_OobSiCache::cache_sns_entries(const char *siOOBSNSCacheLocation)
{
    rmf_Error err = RMF_SUCCESS;
    rmf_siSourceNameEntry *sn_walker = NULL;
    rmf_osal_File cache_fd;
    rmf_osal_File fd;
    uint32_t count = 0;
    // Parameter check
    {
        if (RMF_SUCCESS != rmf_osal_fileDelete(siOOBSNSCacheLocation))
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "CACHE: Error deleting file...%s\n", siOOBSNSCacheLocation);
        }
        err = rmf_osal_fileOpenBuffered(siOOBSNSCacheLocation, RMF_OSAL_FS_OPEN_WRITE | RMF_OSAL_FS_OPEN_MUST_CREATE, &fd);
        if (err == RMF_SUCCESS)
        {
            cache_fd = fd;
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "open [%s]  failed with %d\n", siOOBSNSCacheLocation, err);
            err = RMF_SI_INVALID_PARAMETER;
        }
    }
    if (err == RMF_SUCCESS)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI","CACHE: starting writing SNS cache\n");
        uint32_t write_count = 0;
        uint32_t loop_count = 0;

        // write the source name entry list
        sn_walker = g_si_sourcename_entry;
        while (sn_walker && err == RMF_SUCCESS)
        {
            rmf_SiLangSpecificStringList *name_list[2] = {sn_walker->source_names, sn_walker->source_long_names};
            rmf_SiLangSpecificStringList *name_walker = NULL;
            int                          name_counts[2] = {0};

            if (!(sn_walker->mapped))
            {
                sn_walker = sn_walker->next;
                continue;
            }

            {
                uint8_t list_idx = 0;
                {
                    for (list_idx = 0; list_idx < 2; list_idx++)
                    {
                        uint8_t name_count = 0;
                        name_walker = name_list[list_idx];
                        while(name_walker)
                        {
                            name_count++;
                            name_walker = name_walker->next;
                        }
                        name_counts[list_idx] = name_count;
                    }

                    sn_walker->source_names        = (rmf_SiLangSpecificStringList   *)(name_counts[0]);
                    sn_walker->source_long_names   = (rmf_SiLangSpecificStringList   *)(name_counts[1]);
                }
                count = sizeof(*sn_walker);
                loop_count++;
                err = rmf_osal_fileWrite(cache_fd, &count, (void *)sn_walker);

                sn_walker->source_names        = name_list[0];
                sn_walker->source_long_names   = name_list[1];

                if (count == sizeof(*sn_walker))
                {
                    write_count += count;
                    {
                        // now copy names
                        for (list_idx = 0; list_idx < 2; list_idx++)
                        {
                             // LEN-NAME, LEN-NAME...
                            name_walker = name_list[list_idx];
                            err = write_name_string(cache_fd, name_walker, &write_count);

                        } // For each name list
                    }
                }
                else
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "writing SI /persistent/si/si_table_cache failed with %d\n", err);
                    err = RMF_SI_INVALID_PARAMETER;
                    break;
                }
            }

            if (1)
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI", "AppType[%d] SourceId[%d], Name[%s] \n",
                    sn_walker->appType,  sn_walker->id,
                    (sn_walker->source_names && sn_walker->source_names->string) ? sn_walker->source_names->string : "NULL");

            sn_walker = sn_walker->next;
        } // For each sn_walker
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI", "writing %s DONE writing SNS entries, %d bytes %d entries\n", siOOBSNSCacheLocation, sizeof(*sn_walker), loop_count);

        rmf_osal_fileClose(cache_fd);
    }
    return err;
}

rmf_Error rmf_OobSiCache::generate_new_si_entry(rmf_SiTableEntry *input_si_entry, rmf_SiTableEntry **new_si_entry_out)
{
    rmf_SiTableEntry *new_si_entry = NULL;
    rmf_SiProgramHandle prog_handle = RMF_SI_INVALID_HANDLE;

    m_pInbSiCache->GetTransportStreamEntryFromFrequencyModulation(g_frequency[input_si_entry->freq_index], g_mode[input_si_entry->mode_index], &input_si_entry->ts_handle);
    m_pInbSiCache->SetPATProgramForTransportStream(input_si_entry->ts_handle, input_si_entry->program_number, 0);
    m_pInbSiCache->GetProgramEntryFromTransportStreamEntry(input_si_entry->ts_handle, input_si_entry->program_number, &prog_handle);
    GetServiceEntryFromChannelNumber(input_si_entry->virtual_channel_number, (rmf_SiServiceHandle *)(&new_si_entry));
    
    //GetServiceEntryFromSourceId(input_si_entry->source_id, (rmf_SiServiceHandle *)(&new_si_entry));
    if(new_si_entry)
    {
        UpdateServiceEntry((rmf_SiServiceHandle)(new_si_entry), input_si_entry->ts_handle, prog_handle);
        SetSourceId((rmf_SiServiceHandle)new_si_entry, input_si_entry->source_id);
        
        //SetVirtualChannelNumber((rmf_SiServiceHandle)(new_si_entry), input_si_entry->virtual_channel_number);
        SetAppType((rmf_SiServiceHandle)new_si_entry, input_si_entry->isAppType);
        SetAppId((rmf_SiServiceHandle)new_si_entry, input_si_entry->app_id);
        SetActivationTime ((rmf_SiServiceHandle)new_si_entry, input_si_entry->activation_time);
        SetChannelNumber((rmf_SiServiceHandle)new_si_entry, input_si_entry->virtual_channel_number, RMF_SI_DEFAULT_CHANNEL_NUMBER);
        
        if(CHMAP_SOURCEID == input_si_entry->source_id)
        {
          RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI","Setting cahnnel map Id=%d\n", input_si_entry->virtual_channel_number);
          SetChannelMapId(input_si_entry->virtual_channel_number);
        }
        if(DAC_SOURCEID == input_si_entry->source_id)
        {
	RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI","Setting controller Id=%d\n", input_si_entry->virtual_channel_number);
    	SetDACId(input_si_entry->virtual_channel_number);
        }
        SetCDSRef((rmf_SiServiceHandle)new_si_entry, input_si_entry->freq_index);
        SetMMSRef ((rmf_SiServiceHandle)new_si_entry, input_si_entry->mode_index);
        SetChannelType((rmf_SiServiceHandle)new_si_entry, input_si_entry->channel_type);
        SetVideoStandard((rmf_SiServiceHandle)new_si_entry, input_si_entry->video_standard);
        SetServiceType((rmf_SiServiceHandle)new_si_entry, input_si_entry->service_type);
        SetProgramNumber((rmf_SiServiceHandle)new_si_entry, input_si_entry->program_number);
        SetTransportType((rmf_SiServiceHandle)new_si_entry, input_si_entry->transport_type);
        SetScrambled((rmf_SiServiceHandle)new_si_entry, input_si_entry->scrambled);
        SetServiceEntryStateMapped((rmf_SiServiceHandle)new_si_entry);
        // This field will be reconciled when updateServiceEntries is called
        if(RMF_SI_INVALID_HANDLE != (rmf_SiServiceHandle)new_si_entry)
        {        
            new_si_entry->source_name_entry = NULL;
            new_si_entry->descriptions = input_si_entry->descriptions;
            new_si_entry->state = input_si_entry->state;
            rmf_osal_timeGetMillis(&(new_si_entry->ptime_service));
            *new_si_entry_out = new_si_entry;
            RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "Generating entry for isApp %d with state %d\r\n", new_si_entry->isAppType, new_si_entry->state);
        }
    }
     return RMF_SUCCESS;
}

/**
 * De-serialize SI data from cache file int g_si_entry. The data in g_si_entry is later updated by new tables
 * acquired during SI acquisition.
 */
rmf_Error rmf_OobSiCache::load_si_entries_114(const char *siOOBCacheLocation, const char *siOOBCachePost114Location, const char *siOOBSNSCacheLocation)
{
    /* For 121 version, we need to load SICache into both SICACHE and SNSCACHE */
    rmf_SiTableEntry114 input_si_entry_buf;
    rmf_SiTableEntry114 *input_si_entry = &input_si_entry_buf;

    rmf_SiTableEntry121 input_si_entry_buf_121;
    rmf_SiTableEntry121 *input_si_entry_121 = &input_si_entry_buf_121;

    rmf_SiTableEntry121 *new_si_entry = NULL;
    int ret = -1;
    rmf_Error err = RMF_SUCCESS;
    rmf_osal_File cache_fd;
    //uint32_t program_number = 0;
    int num_entry_read = 0;
    uint32_t count = 0;

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI","CACHE: start loading SI cache 114\r\n");
    {
        rmf_osal_File fd;
        err = rmf_osal_fileOpen(siOOBCacheLocation, RMF_OSAL_FS_OPEN_READ, &fd);
        if (err == RMF_SUCCESS && fd != NULL)
        {
            cache_fd = fd;
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "open [%s] failed for [%s]\r\n", siOOBCacheLocation, strerror(errno));
            err = RMF_SI_INVALID_PARAMETER;
        }
    }
    if (err == RMF_SUCCESS)
    {
        int read_count = 0;
        count = sizeof(g_frequency);
        ret = rmf_osal_fileRead(cache_fd, &count, (void *)g_frequency);
        if (ret == RMF_SUCCESS && count == sizeof(g_frequency))
        {
            read_count += count;
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "reading FREQ to [%s] failed %s\r\n", siOOBCacheLocation, strerror(errno));
            err =  RMF_SI_INVALID_PARAMETER;
        }

        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "Version for Cache file at [%s]: %d\r\n", siOOBCacheLocation,  g_frequency[0]);
        /* Assert that the first 4 bytes are all 0s */
        if (g_frequency[0] != 0x0000) {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "Invalid Version detected for Cache file at [%s]\r\n", siOOBCacheLocation);
            err = RMF_SI_INVALID_PARAMETER;
            //rmf_osal_fileDelete(siOOBCacheLocation);
        }

        if (err == RMF_SUCCESS)
        {
            count = sizeof(g_mode);
            ret = rmf_osal_fileRead(cache_fd, &count, (void *)g_mode);
            if (ret == RMF_SUCCESS && count == sizeof(g_mode))
            {
                read_count += count;
            }
            else
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "reading MODE from [%s] failed %s\r\n", siOOBCacheLocation, strerror(errno));
                err = RMF_SI_INVALID_PARAMETER;
            }
        }

        while (err == RMF_SUCCESS)
        {
            if (err == RMF_SUCCESS && input_si_entry != NULL)
            {
                memset(input_si_entry, 0, sizeof(*input_si_entry));
                memset(input_si_entry_121, 0, sizeof(*input_si_entry_121));
                {
                    count = sizeof(*input_si_entry);
                    ret = rmf_osal_fileRead(cache_fd, &count, (void *)input_si_entry);
                    if (ret == RMF_SUCCESS && count == sizeof(*input_si_entry))
                    {
                        num_entry_read++;
                        read_count += count;
                        /* Read description names first */
                        int list_idx = 0;
                        rmf_SiLangSpecificStringList **name_list[3] = {&input_si_entry->source_names,  &input_si_entry->source_long_names, &input_si_entry->descriptions};
                        intptr_t name_counts[3] = {(intptr_t)(input_si_entry->source_names), (intptr_t)(input_si_entry->source_long_names), (intptr_t)(input_si_entry->descriptions)};
                        for (list_idx = 0; list_idx < 3; list_idx++)
                        {
                            int i = 0;
                            uintptr_t tmp_NameCount = 0;
				
                            *name_list[list_idx] = NULL;
                            rmf_SiLangSpecificStringList *name_walker = NULL;

                            if(0 > name_counts[ list_idx ] )
                            {
                                tmp_NameCount = 0;
                            }
                            else if(NAME_COUNT_MAX < name_counts[ list_idx ] )
                            {
                                tmp_NameCount = NAME_COUNT_MAX;
                            }
                            else
                            {
                                tmp_NameCount = name_counts[list_idx];
                            }
                            for (i = 0; i < tmp_NameCount; i++)
                            {
                                int name_len = 0;
                                unsigned char buf[4] = {0};
                                count = sizeof(buf);
                                ret = rmf_osal_fileRead(cache_fd, &count, (void *)buf);
                                read_count += count;
                                name_len = (buf[0]) | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
                                ret = rmf_osal_memAllocP(RMF_OSAL_MEM_SI_CACHE_OOB, sizeof(*name_walker), (void **)&(name_walker));
                                
                                if (ret == RMF_SUCCESS && name_walker)
                                {
									memset(name_walker, 0, sizeof(*name_walker));
                                    count = sizeof(name_walker->language);
                                    ret = rmf_osal_fileRead(cache_fd, &count, (void *)name_walker->language);
                                    read_count += count;
                                    rmf_osal_memAllocP(RMF_OSAL_MEM_SI_CACHE_OOB, name_len - sizeof(name_walker->language) + 1, (void **)&(name_walker->string));
                                    memset(name_walker->string, 0, name_len - sizeof(name_walker->language) + 1);

                                    count = name_len - sizeof(name_walker->language);
                                    ret = rmf_osal_fileRead(cache_fd, &count, (void *)name_walker->string);
                                    read_count += count;
                                    name_walker->next = *name_list[list_idx];
                                    *name_list[list_idx] = name_walker;
                                }
                            }
                        }

                        /* Done reading the entry. Convert it to 121 Struct */
                        {
                            input_si_entry_121->ref_count = input_si_entry->ref_count;
                            input_si_entry_121->activation_time = input_si_entry->activation_time; // SVCT
                            input_si_entry_121->ptime_service = input_si_entry->ptime_service;
                            input_si_entry_121->ts_handle =  input_si_entry->ts_handle == RMF_SI_OOB_FREQUENCY ? RMF_SI_DSG_FREQUENCY : input_si_entry->ts_handle;
                            input_si_entry_121->program = (rmf_SiProgramHandle)input_si_entry->program;
                            input_si_entry_121->tuner_id= input_si_entry->tuner_id;
                            input_si_entry_121->valid= input_si_entry->valid;
                            input_si_entry_121->virtual_channel_number= input_si_entry->virtual_channel_number;
                            input_si_entry_121->isAppType= input_si_entry->isAppType;
                            input_si_entry_121->source_id= input_si_entry->source_id;
                            input_si_entry_121->app_id= input_si_entry->app_id;
                            input_si_entry_121->dsgAttached= input_si_entry->dsgAttached;
                            input_si_entry_121->dsg_attach_count= input_si_entry->dsg_attach_count;
                            input_si_entry_121->state= input_si_entry->state;
                            input_si_entry_121->channel_type= input_si_entry->channel_type;
                            input_si_entry_121->video_standard= input_si_entry->video_standard;
                            input_si_entry_121->service_type= input_si_entry->service_type;

                            //rmf_SiLangSpecificStringList *source_names;  assigned to SNScache 
                            //rmf_SiLangSpecificStringList *source_long_names;  assigned to SNScache 
                            {
                                rmf_siSourceNameEntry input_sn_entry_buf;
                                rmf_siSourceNameEntry *input_sn_entry = &input_sn_entry_buf;
                                rmf_siSourceNameEntry *new_sn_entry = NULL;
                                memset(input_sn_entry, 0, sizeof(*input_sn_entry));
                                input_sn_entry->id = (input_si_entry_121->isAppType ? input_si_entry_121->app_id : input_si_entry_121->source_id);
                                input_sn_entry->appType = (input_si_entry_121->isAppType ? true : false);
                                input_sn_entry->source_names = input_si_entry->source_names;
                                input_sn_entry->source_long_names = input_si_entry->source_long_names;
                                GetSourceNameEntry(input_sn_entry->id, input_sn_entry->appType, &new_sn_entry, TRUE);
                                // 'Mapped' field is set when updateServiceEntries is called
                                new_sn_entry->source_names = input_sn_entry->source_names;
                                new_sn_entry->source_long_names = input_sn_entry->source_long_names;
                                if (1) {
                                    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "SI114 AppType[%d] SourceId[%d], Name[%s]\n",
                                            new_sn_entry->appType,  new_sn_entry->id,
                                            (new_sn_entry->source_names && new_sn_entry->source_names->string) ? new_sn_entry->source_names->string : "NULL");
                                }
                            }

                            input_si_entry_121->descriptions= input_si_entry->descriptions;
                            input_si_entry_121->freq_index= input_si_entry->freq_index;
                            input_si_entry_121->mode_index= input_si_entry->mode_index;
                            input_si_entry_121->major_channel_number= input_si_entry->major_channel_number;
                            input_si_entry_121->minor_channel_number= input_si_entry->minor_channel_number;
                            //input_si_entry_121->program_number; find out below 
                            input_si_entry_121->transport_type = 0; //only mpeg2, no analog supported
                            input_si_entry_121->scrambled = 0;          //only mpeg2, no analog supported
                        }

                        {
                            if (input_si_entry_121->ts_handle == RMF_SI_DSG_FREQUENCY)
                            {
                                /* 121 defer handling of this to mpe_siUpdateServiceEntries */
                                new_si_entry = NULL;
                            }
                            else
                            {
                                input_si_entry_121->program_number = (uintptr_t)(input_si_entry->program);
                                generate_new_si_entry(input_si_entry_121, &new_si_entry);
                            }
                        }

                        /*if (new_si_entry != NULL)
                            RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI","\nSI114 RChannelVCN#%06d-%s#%06x--State-[%d]-Freq[%09d]-Mode[%04d]-Prog[%08d]\n",
                                    new_si_entry->virtual_channel_number,
                                    new_si_entry->isAppType ? "APPID" : "SRCID",
                                    new_si_entry->isAppType ? new_si_entry->app_id : new_si_entry->source_id,
                                    new_si_entry->state,
                                    g_frequency[new_si_entry->freq_index],
                                    g_mode[new_si_entry->mode_index],
                                    new_si_entry->program ? new_si_entry->program->program_number : 87654321);*/
                    }
                    else if (ret < 0)
                    {
                        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "Reading [%s] failed %s\r\n", siOOBCacheLocation, strerror(errno));
                        return RMF_SI_NOT_FOUND;
                        break;
                    }
                    else
                    {
                        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "Reading [%s] reaches EOF%s\r\n", siOOBCacheLocation, strerror(errno));
                        break;
                    }
                }
            }
        }
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "reading [%s] DONE %d bytes\r\n", siOOBCacheLocation, read_count);
        rmf_osal_fileClose(cache_fd);

        //mpe_siUpdateServiceEntries();


        if (err == RMF_SUCCESS)
        {
            if (num_entry_read > 0)
            {
                g_si_state = SI_FULLY_ACQUIRED;
                g_SITPConditionSet = TRUE;
                RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "SI_FULLY_ACQUIRED FULLY via 114 CACHING\r\n");
                get_frequency_range(&g_frequency[1], MAX_FREQUENCIES, &g_minFrequency, &g_maxFrequency);
                /* Now cache file to Post114 format */
                if (0 && siOOBCachePost114Location && siOOBSNSCacheLocation)
                {
                    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI","Writing 114 Cache to 121 Format NOW\n");
                    // Cache NTT-SNS entries
                    if (cache_sns_entries(siOOBSNSCacheLocation) == RMF_SUCCESS)
                    {
                        // Cache SVCT entries
                        if(cache_si_entries(siOOBCachePost114Location) == RMF_SUCCESS)
                        {
                            RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI","NEW CACHING DONE.\n");
                        }
                    }
                }
            }
            else
            {
                err = RMF_SI_NOT_FOUND;
            }
        }
    }
    else
    {
    }
    // Reconcile service entry's program handle, transport stream handle and service names etc.
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI","CACHE: done loading 114 cache %d\r\n", num_entry_read);
    return err;
}

rmf_Error rmf_OobSiCache::load_si_entries_Post114(const char *siOOBCacheLocation)
{
    rmf_Error err = RMF_SUCCESS;
    rmf_SiTableEntry input_si_entry_buf;
    rmf_SiTableEntry *input_si_entry = &input_si_entry_buf;
    rmf_SiTableEntry *new_si_entry = NULL;
    int ret = -1;
    rmf_osal_File cache_fd;
    uint32_t program_number = 0;
    uint32_t num_entry_read = 0;
    uint32_t count = 0;

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI","CACHE: start loading cache 121\n");
    {
        rmf_osal_File fd;
        err = rmf_osal_fileOpen(siOOBCacheLocation, RMF_OSAL_FS_OPEN_READ, &fd);
        if (err == RMF_SUCCESS && fd != NULL)
        {
            cache_fd = fd;
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "open [%s] failed with %d\n", siOOBCacheLocation, err);
            return RMF_SI_INVALID_PARAMETER;
        }
    }
    // SI DB write lock is acquired by the caller!
    if (err == RMF_SUCCESS)
    {
        uint32_t read_count = 0;
        uint32_t version;

        // Read header version
        count = sizeof(version);
        err = rmf_osal_fileRead(cache_fd, &count, (void *)&version);

        if (err == RMF_SUCCESS && (count == sizeof(version)))
        {
            read_count += count;
            if (version != RMF_SI_CACHE_FILE_VERSION)
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "reading Version failed with %x, expected %x\n", version, RMF_SI_CACHE_FILE_VERSION);
                err =  RMF_SI_INVALID_PARAMETER;
            }
            else
            {
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI", "Read version 0x%x\n", version);
            }
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "reading Version [%s] failed with err:%d count:%d version:0x%x\n", siOOBCacheLocation, err, count, version);
            err =  RMF_SI_INVALID_PARAMETER;
        }
        if (err == RMF_SUCCESS)
        {
            count = sizeof(g_frequency);
            ret = rmf_osal_fileRead(cache_fd, &count, (void *)g_frequency);
            if (ret == RMF_SUCCESS && (count == sizeof(g_frequency)))
            {
                read_count += count;
            }
            else
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "reading FREQ to [%s] failed with %d\n", siOOBCacheLocation, err);
                err =  RMF_SI_INVALID_PARAMETER;
            }
       }

        if (err == RMF_SUCCESS)
        {
            count = sizeof(g_mode);
            ret = rmf_osal_fileRead(cache_fd, &count, (void *)g_mode);
            if (ret == RMF_SUCCESS && count == sizeof(g_mode))
            {
                read_count += count;
            }
            else
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "reading MODE from [%s] failed with %d\n", siOOBCacheLocation, err);
                err = RMF_SI_INVALID_PARAMETER;
            }
        }

        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI", "Reading SVCT entries..\n");
        // Read SVCT entries
        while (err == RMF_SUCCESS)
        {
            // Now read the table entries
            if (err == RMF_SUCCESS && input_si_entry != NULL)
            {
                memset(input_si_entry, 0, sizeof(*input_si_entry));
                {
                    count = sizeof(*input_si_entry);
                    ret = rmf_osal_fileRead(cache_fd, &count, (void *)input_si_entry);
                    if (ret == RMF_SUCCESS && count == sizeof(*input_si_entry))
                    {
                        num_entry_read++;
                        read_count += count;
                        /* Read description names first */
                        rmf_SiLangSpecificStringList **name_list = &input_si_entry->descriptions;
                        intptr_t name_count = 0;
                        if(0 > (intptr_t)(input_si_entry->descriptions))
                        {
                            name_count = 0;
                        }
                        else if( NAME_COUNT_MAX < (intptr_t)(input_si_entry->descriptions) )
                        {
                            name_count = NAME_COUNT_MAX;
                        }
                        else
                        {
                            name_count = (intptr_t)(input_si_entry->descriptions);
                        }
                        {
                            int i = 0;
                            rmf_SiLangSpecificStringList *name_walker = NULL;
                            for (i = 0; i < name_count; i++)
                            {
                                unsigned char buf[4] = {0};

                                count = sizeof(buf);
                                ret = rmf_osal_fileRead(cache_fd, &count, (void *)buf);

                                read_count += count;
                                ret = rmf_osal_memAllocP(RMF_OSAL_MEM_SI_CACHE_OOB, sizeof(*name_walker), (void **)&(name_walker));
                                if (ret == RMF_SUCCESS && name_walker)
                                {
                                    int name_len = (buf[0]) | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
                               	    memset(name_walker, 0, sizeof(*name_walker));
                                    ret = read_name_string(cache_fd, name_walker, &read_count, name_len);
				                    if((RMF_SUCCESS != ret) && (read_count != name_len))
				                    {
				                        rmf_osal_memFreeP(RMF_OSAL_MEM_SI_CACHE_OOB, name_walker);
				                    }
			                        else
                                        *name_list = name_walker;
                                }
                            }
                        }

                        {
                            program_number = (uint32_t)(input_si_entry->program_number);
                            generate_new_si_entry(input_si_entry, &new_si_entry);
                        }

                        if (1)
                        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "Post114 RChannelVCN#%06d-%s#%06x-State-[%d]-Freq[%09d]-Mode[%04d]-Prog[%08d]\n",
                                new_si_entry->virtual_channel_number,
                                new_si_entry->isAppType ? "APPID" : "SRCID",
                                new_si_entry->isAppType ? new_si_entry->app_id : new_si_entry->source_id,
                                new_si_entry->state,
                                g_frequency[new_si_entry->freq_index],
                                g_mode[new_si_entry->mode_index],
                                new_si_entry->program_number);
                    }
                    else
                    {
                        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "Reading [%s] reaches EOF with %d\n", siOOBCacheLocation, err);
                        break;
                    }
                }
            }
        }
        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "reading [%s] DONE %d bytes\n", siOOBCacheLocation, read_count);
        rmf_osal_fileClose(cache_fd);
        if (err == RMF_SUCCESS)
        {
            if (num_entry_read > 0)
            {
                g_si_state = SI_FULLY_ACQUIRED;
                g_SITPConditionSet = TRUE;
                RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "SI_FULLY_ACQUIRED FULLY via 121 CACHING, g_si_entry: 0x%x\n", g_si_entry);
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "Number of Services Acquired via 121 CACHING: %d\n", num_entry_read);
                get_frequency_range(&g_frequency[1], 255, &g_minFrequency, &g_maxFrequency);
		
            }
            else
            {
                err = RMF_SI_NOT_FOUND;
            }
        }
    }

    // Reconcile service entry's program handle, transport stream handle and service names etc.
    //UpdateServiceEntries();

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI","CACHE: done loading 121 SI cache %d\n", num_entry_read);

    return err;
}

rmf_Error rmf_OobSiCache::load_sns_entries(const char *siOOBSNSCacheLocation)
{
    rmf_Error err = RMF_SUCCESS;
    rmf_siSourceNameEntry input_sn_entry_buf;
    rmf_siSourceNameEntry *input_sn_entry = &input_sn_entry_buf;
    rmf_siSourceNameEntry *new_sn_entry = NULL;

    int ret = -1;
    rmf_osal_File cache_fd;
    uint32_t count = 0;

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI","CACHE: start loading SNS cache\n");
    {
        rmf_osal_File fd;
        err = rmf_osal_fileOpen(siOOBSNSCacheLocation, RMF_OSAL_FS_OPEN_READ, &fd);
        if (err == RMF_SUCCESS && fd != NULL)
        {
            cache_fd = fd;
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "open [%s] failed with %d\n", siOOBSNSCacheLocation, err);
            return RMF_SI_INVALID_PARAMETER;
        }
    }

    // SI DB write lock is acquired by the caller!
    if (err == RMF_SUCCESS)
    {
        uint32_t read_count = 0;

        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI", "Reading SNS entries..\n");
        // Read SNS entries
        while (err == RMF_SUCCESS)
        {
            if (err == RMF_SUCCESS && input_sn_entry != NULL)
            {
                // Read source name list first
                memset(input_sn_entry, 0, sizeof(*input_sn_entry));
                {
                    count = sizeof(*input_sn_entry);
                    ret = rmf_osal_fileRead(cache_fd, &count, (void *)input_sn_entry);
                    if (ret == RMF_SUCCESS && count == sizeof(*input_sn_entry))
                    {
                        read_count += count;
                        /* Read names */
                        int list_idx = 0;
                        rmf_SiLangSpecificStringList **name_list[2] = {&input_sn_entry->source_names,  &input_sn_entry->source_long_names};
                        intptr_t name_counts[2] = {(intptr_t)(input_sn_entry->source_names), (intptr_t)(input_sn_entry->source_long_names)};
                        for (list_idx = 0; list_idx < 2; list_idx++)
                        {
                            int i = 0;
                            uintptr_t tmp_NameCount =0;
                            *name_list[list_idx] = NULL;
                            rmf_SiLangSpecificStringList *name_walker = NULL;

                            if(0 > name_counts[ list_idx ] )
                            {
                                tmp_NameCount = 0;
                            }
                            else if(NAME_COUNT_MAX < name_counts[ list_idx ] )
                            {
                                tmp_NameCount = NAME_COUNT_MAX;
                            }
                            else
                            {
                                tmp_NameCount = name_counts[list_idx];
                            }
                            for (i = 0; i < tmp_NameCount; i++)
                            {
                                unsigned char buf[4] = {0};

                                count = sizeof(buf);
                                ret = rmf_osal_fileRead(cache_fd, &count, (void *)buf);

                                read_count += count;
                                ret = rmf_osal_memAllocP(RMF_OSAL_MEM_SI_CACHE_OOB, sizeof(rmf_SiLangSpecificStringList), (void **)&(name_walker));
                                if (ret == RMF_SUCCESS && name_walker)
                                {
                                    int name_len = (buf[0]) | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
                                    memset(name_walker, 0, sizeof(rmf_SiLangSpecificStringList));
                                    ret = read_name_string(cache_fd, name_walker, &read_count, name_len);
				                    if((RMF_SUCCESS != ret) && (read_count != name_len))
				                    {
				                        rmf_osal_memFreeP(RMF_OSAL_MEM_SI_CACHE_OOB, name_walker);
				                    }
			                        else
                                    {
                                        name_walker->next = *name_list[list_idx];
                                        *name_list[list_idx] = name_walker;
                                    }
                                }
                            }
                        }

                        {
                            GetSourceNameEntry(input_sn_entry->id, input_sn_entry->appType, &new_sn_entry, TRUE);
                            // 'Mapped' field is set when updateServiceEntries is called
			                if(new_sn_entry != NULL)
                            {
                                new_sn_entry->source_names = input_sn_entry->source_names;
                                new_sn_entry->source_long_names = input_sn_entry->source_long_names;
                                RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "AppType[%d] SourceId[%x], Name[%s]\n",
                                         new_sn_entry->appType,  new_sn_entry->id,
                                         (new_sn_entry->source_names && new_sn_entry->source_names->string) ? new_sn_entry->source_names->string : "NULL");
                            }
                        }

                    }
                    else
                    {
                        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "Reading reaches EOF with %d\n", err);
                        break;
                    }
                }
            }

        }

        RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "reading DONE %d bytes\n", read_count);
        rmf_osal_fileClose(cache_fd);
    }
    return err;
}

uint32_t rmf_OobSiCache::getDumpTables(void)
{
        return g_sitp_si_dump_tables;
}

rmf_osal_TimeMillis rmf_OobSiCache::getSTTStartTime(void)
{
        return g_sitp_stt_time;
}

void rmf_OobSiCache::setSTTStartTime(rmf_osal_TimeMillis sttStartTime)
{
        g_sitp_stt_time = sttStartTime;
}

uint32_t rmf_OobSiCache::getParseSTT()
{
    return g_sitp_si_parse_stt;
}

void rmf_OobSiCache::checksifolder(const char *psFileName)
{
    char *str1 = NULL;
    char *psFolderName = NULL;
    int pos=0;
    rmf_Error err = RMF_SUCCESS;
    rmf_osal_Dir dir_handle;

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI", "psFileName: %s\n", psFileName);
    str1 = strrchr((char*)psFileName, '/');
    pos = str1-psFileName;
    //psFolderName = (char*)malloc((pos+1)*sizeof(char));
    rmf_osal_memAllocP(RMF_OSAL_MEM_SI_CACHE_OOB, ((pos+1)*sizeof(char)), (void**) &psFolderName);
    memset(psFolderName, 0x0, (pos+1)*sizeof(char));
    strncpy(psFolderName, (char*)psFileName, pos);
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI", "psFolderName: %s\n", psFolderName);

    err = rmf_osal_dirOpen(psFolderName, &dir_handle);
    if(RMF_SUCCESS == err)
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI", "%s folder available.", psFolderName);
    else if(RMF_OSAL_FS_ERROR_NOT_FOUND == err)
    {
        err = rmf_osal_dirCreate(psFolderName);
        if(RMF_SUCCESS != err)
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "Cannot create directory %s.\n", psFolderName);
        else
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI", "%s folder created successfully.\n", psFolderName);
    }

    rmf_osal_memFreeP(RMF_OSAL_MEM_SI_CACHE_OOB, psFolderName);
    psFolderName = NULL;
}

/**
 * The <i>verify_crc_for_si_and_sns_cache()</i> function will verify the
 * CRC value that is written in the size with calculated CRC.
 *
 * @param pSICache Is a pointer for the name of the SI Cache file
 * @param pSISNSCache Is a pointer for the name of the SISNS Cache file
 * @param pStatus Is the result of CRC comparison.
 * @return The MPE error code if the create fails, otherwise <i>MPE_SUCCESS<i/>
 *          is returned.
 */
rmf_Error rmf_OobSiCache::verify_crc_for_si_and_sns_cache(const char* pSICache, const char* pSISNSCache, int *pStatus)
{
    rmf_Error err = RMF_SUCCESS;
    rmf_osal_File fdSNSCache = 0;
    rmf_osal_File fdSICache = 0;
    unsigned char *pCRCData = NULL;
    uint32_t sizeOfSICache = 0;
    uint32_t sizeOfSISNSCache = 0;
    uint32_t crcFileSize = 0;
    uint32_t readSNSSize = 0;
    uint32_t readSISize = 0;
    uint32_t crcValue = 0xFFFFFFFF;
    uint32_t crcRead = 0xFFFFFFFF;

    *pStatus = 0;
    /* Init should be done only once.. The main application will do init in actual product. in this test app we must call */
    //init_mpeg2_crc();

    si_getFileSize(pSICache, &sizeOfSICache);
    si_getFileSize(pSISNSCache, &sizeOfSISNSCache);

    readSNSSize = sizeOfSISNSCache;
    readSISize = sizeOfSICache;

    crcFileSize = sizeOfSISNSCache + sizeOfSICache;
    err = rmf_osal_memAllocP(RMF_OSAL_MEM_SI_CACHE_OOB, crcFileSize,(void**) &pCRCData);
    if (err == RMF_SUCCESS && pCRCData != NULL)
    {

        err = rmf_osal_fileOpen(pSISNSCache, RMF_OSAL_FS_OPEN_READ, &fdSNSCache);
        if ((err == RMF_SUCCESS) && (fdSNSCache != NULL))
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "open [%s] success\n", pSISNSCache);
            err = rmf_osal_fileRead(fdSNSCache, &readSNSSize, pCRCData);
            if ((err != RMF_SUCCESS))
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "Read less than the size of the file\n");
            }
            rmf_osal_fileClose(fdSNSCache);
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "open [%s] failed with %d\n", pSISNSCache, err);
            rmf_osal_memFreeP (RMF_OSAL_MEM_SI_CACHE_OOB, pCRCData);
            return err;
        }

        /* Read the SICache file, even if readSNSSize != sizeOfSISNSCache */
        err = rmf_osal_fileOpen(pSICache, RMF_OSAL_FS_OPEN_READ, &fdSICache);
        if ((err == RMF_SUCCESS) && (fdSICache != NULL))
        {
            /* Read from SI cache file. the buffer shd be offsetted to SNScache size */
            err = rmf_osal_fileRead(fdSICache, &readSISize, (pCRCData + readSNSSize));
            if ((err != RMF_SUCCESS))
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "Read less than the size of the file\n");
            }
            rmf_osal_fileClose(fdSICache);
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "open [%s] failed with %d\n", pSICache, err);
            rmf_osal_memFreeP (RMF_OSAL_MEM_SI_CACHE_OOB, pCRCData);			
            return err;
        }

        /* Find the CRC and write it at the end of SICache file */
        readSISize = sizeof(crcValue);
        crcValue = calc_mpeg2_crc (pCRCData, (crcFileSize - readSISize));
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI", "Calculated CRC is [%u]\n", crcValue);

        memcpy (&crcRead, &pCRCData[crcFileSize - readSISize], readSISize);

        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI", "CRC Read from file [%u]\n", crcRead);
        if(crcValue == crcRead)
        {
                *pStatus = 1;
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI", "CRC Matching.....\n");
        }
        rmf_osal_memFreeP (RMF_OSAL_MEM_SI_CACHE_OOB, pCRCData);
    }
    return err;
}


/**
 * The <i>verify_version_and_crc()</i> function will verify the version of cache and
 * CRC value that is written in the size with calculated CRC.
 *
 * @param siOOBCacheLocation Is a pointer for the name of the SI Cache file
 * @param siOOBSNSCacheLocation Is a pointer for the name of the SISNS Cache file
 * @return The result of verification of version and crc. 0 when fails and 1 when success is returned.
 */
int rmf_OobSiCache::verify_version_and_crc (const char *siOOBCacheLocation, const char *siOOBSNSCacheLocation)
{
    rmf_osal_File cache_fd = NULL;
    rmf_Error err = RMF_SUCCESS;
    int proceed = 0;
    int isCRCMatch = 0;

    rmf_osal_File fd;
    err = rmf_osal_fileOpen(siOOBCacheLocation, RMF_OSAL_FS_OPEN_READ, &fd);
    if (err == RMF_SUCCESS && fd != NULL)
    {
        cache_fd = fd;
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "open [%s] failed with %d\n", siOOBCacheLocation, err);
    }

    // SI DB write lock is acquired by the caller!
    if (err == RMF_SUCCESS)
    {
        uint32_t read_count = 0;
        uint32_t version;
        uint32_t count = 0;

        // Read header version
        count = sizeof(version);
        err = rmf_osal_fileRead(cache_fd, &count, (void *)&version);

        if (err == RMF_SUCCESS && (count == sizeof(version)))
        {
            read_count += count;
            if (version < RMF_SI_CACHE_FILE_VERSION)
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "reading Version failed with %x, expected %x\n", version, RMF_SI_CACHE_FILE_VERSION);
                proceed = 0;
            }
            else
            {
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI", "Read version 0x%x\n", version);
                proceed = 1;
            }
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "reading Version [%s] failed with err:%d count:%d version:0x%x\n", siOOBCacheLocation, err, count, version);
        }

        rmf_osal_fileClose(cache_fd);
    }

    /* check CRC only when version matches.. */
    if ((err == RMF_SUCCESS) && (proceed == 1))
    {
        err = verify_crc_for_si_and_sns_cache (siOOBCacheLocation, siOOBSNSCacheLocation, &isCRCMatch);
    }

    if ((err != RMF_SUCCESS) || (isCRCMatch == 0) || (proceed == 0))
    {
        return 0;
    }

    /* Return SUCCESS, so that the calling method can proceed */
    return 1;
}

rmf_Error rmf_OobSiCache::LoadSiCache()
{
	int files_exist = 0;
	rmf_Error err = RMF_SUCCESS;

        checksifolder(g_siOOBCachePost114Location);
        
	/* Assert both Post114 cache files exist at the same time */
	{

		rmf_osal_File fd;
		

		RDK_LOG(
				RDK_LOG_TRACE1,
				"LOG.RDK.SI",
				"<%s> - Opening cache file g_siOOBCachePost114Location: '%s'...\n",
				__FUNCTION__,
				g_siOOBCachePost114Location);            
		err = rmf_osal_fileOpen(g_siOOBCachePost114Location, RMF_OSAL_FS_OPEN_READ, &fd);
		if (err != RMF_SUCCESS) {
			RDK_LOG(
					RDK_LOG_ERROR,
					"LOG.RDK.SI",
					"<%s> - Opening cache file g_siOOBCachePost114Location: '%s' failed...\n",
					__FUNCTION__,
					g_siOOBCachePost114Location);                            
			rmf_osal_fileDelete(g_siOOBCachePost114Location);
			rmf_osal_fileDelete(g_siOOBSNSCacheLocation);
		}
		else {
			rmf_osal_fileClose(fd);
			RDK_LOG(
					RDK_LOG_TRACE1,
					"LOG.RDK.SI",
					"<%s> - Opening cache file g_siOOBSNSCacheLocation: '%s'...\n",
					__FUNCTION__,
					g_siOOBSNSCacheLocation);                            
			err = rmf_osal_fileOpen(g_siOOBSNSCacheLocation, RMF_OSAL_FS_OPEN_READ, &fd);
			if (err != RMF_SUCCESS) {
				RDK_LOG(
						RDK_LOG_ERROR,
						"LOG.RDK.SI",
						"<%s> - Opening cache file g_siOOBSNSCacheLocation: '%s' failed...\n",
						__FUNCTION__,
						g_siOOBSNSCacheLocation);                    
				rmf_osal_fileDelete(g_siOOBCachePost114Location);
				rmf_osal_fileDelete(g_siOOBSNSCacheLocation);
			}
			else {
				rmf_osal_fileClose(fd);
				files_exist = 1;
				//Assert cache_get_version(g_siOOBCachePost114Location) == MPE_SI_CACHE_FILE_VERSION;
			}
		}

		if (!files_exist)
		{
			RDK_LOG(
					RDK_LOG_ERROR,
					"LOG.RDK.SI",
					"<%s> - Cache files are deleted due to missing components...\n",
					__FUNCTION__);
		}
	}

        /* Check the version and CRC for the 121 cache */
        if (files_exist && verify_version_and_crc(g_siOOBCachePost114Location, g_siOOBSNSCacheLocation) == 0)
        {
            files_exist = 0;

            /* Rename the file as .bad for later verification */
            rmf_Error retVal,ret;
  
            ret = rmf_osal_fileRename (g_siOOBCachePost114Location, g_badSICacheLocation);
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI", " File Rename Return Value [%d] for g_siOOBCachePost114Location \n",ret);
            retVal = rmf_osal_fileRename (g_siOOBSNSCacheLocation, g_badSISNSCacheLocation);
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI", " File Rename Return Value [%d] for g_siOOBSNSCacheLocation\n",retVal);
            /* The original file shouldnt be there but still, just in case, we delete the original file */
            rmf_osal_fileDelete(g_siOOBCachePost114Location);
            rmf_osal_fileDelete(g_siOOBSNSCacheLocation);

            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "Cache files are moved as .bad files and start acquiring fresh SI...\n");
        }

	if (files_exist && load_sns_entries(g_siOOBSNSCacheLocation) == RMF_SUCCESS)
	{
		SetGlobalState (SI_NIT_SVCT_ACQUIRED);
		if (load_si_entries_Post114(g_siOOBCachePost114Location) == RMF_SUCCESS)
		{
			/* This state is required for SI Diagnostic report in simgr.c */
			g_siOOBCached = TRUE;
			SetGlobalState (SI_FULLY_ACQUIRED);
		}
	}
	else
	{

		/* Check whether the converion of 114 cache to 121 cache is enabled or not. If NO, skip the below */
		if (g_siOOBCacheConvertStatus)
		{
			/* Load Old cache */
			if (g_siOOBCacheLocation)
			{
				//Assert cache_get_version(g_siOOBCacheLocation) == 0;
				if (load_si_entries_114(g_siOOBCacheLocation, g_siOOBCachePost114Location, g_siOOBSNSCacheLocation) == RMF_SUCCESS)
				{
					/* Write to the new cache */
					if (g_siOOBCachePost114Location)
					{
						RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI","SI_FULLY_ACQUIRED, DO CACHING NOW from 114\n");
						// Cache NTT-SNS entries
						if (cache_sns_entries(g_siOOBSNSCacheLocation) == RMF_SUCCESS)
						{
							// Cache SVCT entries
							if(cache_si_entries(g_siOOBCachePost114Location) == RMF_SUCCESS)
							{
								RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI","CACHING DONE.\n");
							}
						}
					}
					g_siOOBCached = TRUE;
					SetGlobalState (SI_FULLY_ACQUIRED);
				}
			}
		}
	}

	return err;
}

rmf_Error rmf_OobSiCache::LoadSiCache(char *siOOBCachePost114Location, char *siOOBSNSCacheLocation)
{
	int files_exist = 0;
	rmf_Error err = RMF_SUCCESS;

        if((siOOBCachePost114Location == NULL) || (siOOBSNSCacheLocation == NULL))
        {
		RDK_LOG(
				RDK_LOG_TRACE1,
				"LOG.RDK.SI",
				"<%s> - Invalid arguments\n",
				__FUNCTION__);
                err = RMF_SI_INVALID_PARAMETER;
                return err;
        }

	/* Assert both Post114 cache files exist at the same time */
	{

		rmf_osal_File fd;
		

		RDK_LOG(
				RDK_LOG_TRACE1,
				"LOG.RDK.SI",
				"<%s> - Opening cache file siOOBCachePost114Location: '%s'...\n",
				__FUNCTION__,
				siOOBCachePost114Location);            
		err = rmf_osal_fileOpen(siOOBCachePost114Location, RMF_OSAL_FS_OPEN_READ, &fd);
		if (err != RMF_SUCCESS) {
			RDK_LOG(
					RDK_LOG_TRACE1,
					"LOG.RDK.SI",
					"<%s> - Opening cache file siOOBCachePost114Location: '%s' failed...\n",
					__FUNCTION__,
					siOOBCachePost114Location);                            
		}
		else {
			rmf_osal_fileClose(fd);
			RDK_LOG(
					RDK_LOG_TRACE1,
					"LOG.RDK.SI",
					"<%s> - Opening cache file siOOBSNSCacheLocation: '%s'...\n",
					__FUNCTION__,
					siOOBSNSCacheLocation);                            
			err = rmf_osal_fileOpen(siOOBSNSCacheLocation, RMF_OSAL_FS_OPEN_READ, &fd);
			if (err != RMF_SUCCESS) {
				RDK_LOG(
						RDK_LOG_TRACE1,
						"LOG.RDK.SI",
						"<%s> - Opening cache file siOOBSNSCacheLocation: '%s' failed...\n",
						__FUNCTION__,
						siOOBSNSCacheLocation);                    
			}
			else {
				rmf_osal_fileClose(fd);
				files_exist = 1;
				//Assert cache_get_version(g_siOOBCachePost114Location) == MPE_SI_CACHE_FILE_VERSION;
			}
		}

		if (!files_exist)
		{
			RDK_LOG(
					RDK_LOG_ERROR,
					"LOG.RDK.SI",
					"<%s> - Cache files do not exist...\n",
					__FUNCTION__);
			err = RMF_SI_NOT_FOUND;
		}
	}

        /* Check the version and CRC for the 121 cache */
        if (files_exist && verify_version_and_crc(siOOBCachePost114Location, siOOBSNSCacheLocation) == 0)
        {
            files_exist = 0;

            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "Cache files are bad.... \n");
            err = RMF_SI_ERROR_ACQUIRING;
        }

	if (files_exist && load_sns_entries(siOOBSNSCacheLocation) == RMF_SUCCESS)
	{
		SetGlobalState (SI_NIT_SVCT_ACQUIRED);
		if (load_si_entries_Post114(siOOBCachePost114Location) == RMF_SUCCESS)
		{
			/* This state is required for SI Diagnostic report in simgr.c */
			g_siOOBCached = TRUE;
			SetGlobalState (SI_FULLY_ACQUIRED);
            		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "Cache files loaded successfully.... \n");
		}
	}
	else
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "Could not load given cache files  \n");
                if(files_exist)
                    err = RMF_SI_ERROR_ACQUIRING;
	}

        return err;
}

rmf_Error rmf_OobSiCache::CacheSiEntriesFromLive()
{
	if (g_siOOBCacheEnable && g_siOOBCachePost114Location)
	{
		// Cache NTT-SNS entries
		if (m_pSiCache->cache_sns_entries(g_siOOBSNSCacheLocation) == RMF_SUCCESS)
		{
			// Cache SVCT entries
			if(m_pSiCache->cache_si_entries(g_siOOBCachePost114Location) == RMF_SUCCESS)
			{
				RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI","CACHING DONE.\n");
				/* Write the CRC to the SI Cache file */
				m_pSiCache->write_crc_for_si_and_sns_cache(g_siOOBCachePost114Location, g_siOOBSNSCacheLocation);
			}
		}
	}
	return RMF_SUCCESS;
}

rmf_osal_Bool rmf_OobSiCache::is_si_cache_enabled()
{
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI","<%s>: g_siOOBCached is set to %s.\n",
                    __FUNCTION__, g_siOOBCacheEnable==TRUE?"TRUE":"FALSE");
    return g_siOOBCacheEnable;
}

rmf_osal_Bool rmf_OobSiCache::is_si_cache_location_defined()
{
    RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "<%s>: g_siOOBCachePost114Location: %s\n", __FUNCTION__, g_siOOBCachePost114Location);
    if(g_siOOBCachePost114Location)
      return TRUE;
    else
      return FALSE;
}

rmf_SiGlobalState rmf_OobSiCache::GetGlobalState()
{
    return g_si_state;
}

void rmf_OobSiCache::SortCDSFreqList()
{
    get_frequency_range(g_frequency, 255, &g_minFrequency, &g_maxFrequency );
}

/**
 * The <i>si_getFileSize()</i> function will get the size the specified file.
 *
 * @param fileName Is a pointer for the name of the file
 * @param size Is the size to file to be checked.
 * @return The size of the given file. -1 if the file not present or any other failure.
 *          is returned.
 */
unsigned int rmf_OobSiCache::si_getFileSize(const char * location, unsigned int *size)
{
    struct stat buff;
    if (0 == stat(location, &buff))
    {
        *size = (unsigned int) buff.st_size;
        return *size;
    }
    return -1;
}
rmf_Error rmf_OobSiCache::write_crc (const char * pSICacheFileName, unsigned int crcValue)
{
    rmf_Error err = RMF_SUCCESS;
        uint32_t readDataSize = 0;
    rmf_osal_File cache_read_fd = 0;
    rmf_osal_File cache_write_fd = 0;
    rmf_osal_File fd = 0;
    uint32_t sizeOfSICache = 0;
    unsigned char *pActualData = NULL;
    si_getFileSize(pSICacheFileName, &sizeOfSICache);

    if (sizeOfSICache > 0)
    {
        err = rmf_osal_memAllocP(RMF_OSAL_MEM_SI_CACHE_OOB, sizeOfSICache, (void**) &pActualData);
    }

    if ((err == RMF_SUCCESS) && (pActualData != NULL))
    {
        err = rmf_osal_fileOpen(pSICacheFileName, RMF_OSAL_FS_OPEN_READ, &fd);
        if (err == RMF_SUCCESS)
        {
            cache_read_fd = fd;
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "open [%s]  failed with %d\n", pSICacheFileName, err);
            err = RMF_SI_INVALID_PARAMETER;
        }
	if (err == RMF_SUCCESS)
	{
		RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI","CACHE: starting writing cache\n");
		readDataSize = sizeOfSICache;
		err = rmf_osal_fileRead(cache_read_fd, &readDataSize, pActualData);
		if (err == RMF_SUCCESS)
		{
			// The file pointer is moved bcoz of read operations, so close and reopen the file to write the data again.
			err = rmf_osal_fileClose(cache_read_fd);
			err = rmf_osal_fileOpenBuffered(pSICacheFileName, RMF_OSAL_FS_OPEN_WRITE, &fd);

			if (err == RMF_SUCCESS)
			{
				cache_write_fd = fd;
				RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI", "File Opened with RMF_OSAL_FS_OPEN_WRITE\n");
				err = rmf_osal_fileWrite(cache_write_fd, &readDataSize, (void *)pActualData);
				/* Write the CRC value */
				if (err == RMF_SUCCESS)
				{
					uint32_t crcValueReceived = crcValue;
					readDataSize = sizeof(crcValue);
					err = rmf_osal_fileWrite(cache_write_fd, &readDataSize, (void *)&crcValueReceived);
				}
			}
			else
			{
				RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "Writing original data failed\n");
				err = RMF_SI_INVALID_PARAMETER;
			}


			if (err == RMF_SUCCESS)
				RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI", "CRC Appended to the end of file\n");
			else
				RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "Failed to append CRC to the end of file\n");
			/* Close the file */
			rmf_osal_fileClose(cache_write_fd);
		}
		else
		{
			/* Close the file */
			rmf_osal_fileClose(cache_read_fd);
		}
	}
	else
	{
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "Failed to open a file\n");
        }
        rmf_osal_memFreeP (RMF_OSAL_MEM_SI_CACHE_OOB, pActualData);
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "Failed to allocate memory");
    }
    return err;
}

/**
 * The <i>write_crc_for_si_and_sns_cache()</i> function will verify the
 * CRC value that is written in the size with calculated CRC.
 *
 * @param pSICache Is a pointer for the name of the SI Cache file
 * @param pSISNSCache Is a pointer for the name of the SISNS Cache file
 * @return The MPE error code if the create fails, otherwise <i>RMF_SUCCESS<i/>
 *          is returned.
 */
rmf_Error rmf_OobSiCache::write_crc_for_si_and_sns_cache(const char* pSICache, const char* pSISNSCache)
{
    rmf_Error err = RMF_SUCCESS;
    rmf_osal_File fdSNSCache = 0;
    rmf_osal_File fdSICache = 0;
    unsigned char *pCRCData = NULL;
    uint32_t sizeOfSICache = 0;
    uint32_t sizeOfSISNSCache = 0;
    uint32_t crcFileSize = 0;
    uint32_t readSNSSize = 0;
    uint32_t readSISize = 0;
    uint32_t crcValue = 0xFFFFFFFF;
    //uint32_t crcRead = 0xFFFFFFFF;

    /* Init should be done only once.. The main application will do init in actual product. in this test app we must call */
    //init_mpeg2_crc();

    si_getFileSize(pSICache, &sizeOfSICache);
    si_getFileSize(pSISNSCache, &sizeOfSISNSCache);

    readSNSSize = sizeOfSISNSCache;
    readSISize = sizeOfSICache;

    crcFileSize = sizeOfSISNSCache + sizeOfSICache;
    err = rmf_osal_memAllocP(RMF_OSAL_MEM_SI_CACHE_OOB, crcFileSize,(void**) &pCRCData);
    if (err == RMF_SUCCESS && pCRCData != NULL)
    {

        err = rmf_osal_fileOpen(pSISNSCache, RMF_OSAL_FS_OPEN_READ, &fdSNSCache);
        if ((err == RMF_SUCCESS) && (fdSNSCache != NULL))
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI", "open [%s] success\n", pSISNSCache);
            err = rmf_osal_fileRead(fdSNSCache, &readSNSSize, pCRCData);
            if ((err != RMF_SUCCESS))
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "Read less than the size of the file\n");
            }
            rmf_osal_fileClose(fdSNSCache);
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "open [%s] failed with %d\n", pSISNSCache, err);
            rmf_osal_memFreeP (RMF_OSAL_MEM_SI_CACHE_OOB, pCRCData);
            return err;
        }

        /* Read the SICache file, even if readSNSSize != sizeOfSISNSCache */
        err = rmf_osal_fileOpen(pSICache, RMF_OSAL_FS_OPEN_READ, &fdSICache);
        if ((err == RMF_SUCCESS) && (fdSICache != NULL))
        {
            /* Read from SI cache file. the buffer shd be offsetted to SNScache size */
            err = rmf_osal_fileRead(fdSICache, &readSISize, (pCRCData + readSNSSize));
            if ((err != RMF_SUCCESS))
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "Read less than the size of the file\n");
            }
            rmf_osal_fileClose(fdSICache);
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "open [%s] failed with %d\n", pSICache, err);
            rmf_osal_memFreeP (RMF_OSAL_MEM_SI_CACHE_OOB, pCRCData);
            return err;
        }

        /* Find the CRC and write it at the end of SICache file */
        crcValue = calc_mpeg2_crc (pCRCData, crcFileSize);
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI", "Calculated CRC is [%u]\n", crcValue);
        err = write_crc(pSICache, crcValue);
        rmf_osal_memFreeP (RMF_OSAL_MEM_SI_CACHE_OOB, pCRCData);
        return err;
    }
    return err;
}

void rmf_OobSiCache::init_mpeg2_crc(void)
{
    uint16_t i, j;
    uint32_t crc;

    for (i = 0; i < 256; i++)
    {
        crc = i << 24;
        for (j = 0; j < 8; j++)
        {
            if (crc & 0x80000000)
                crc = (crc << 1) ^ CRC_QUOTIENT;
            else
                crc = crc << 1;
        }
        crctab[i] = crc;
    }
} // END crc32_init()

/*
 *  CRC calculation function
 */
uint32_t rmf_OobSiCache::calc_mpeg2_crc(uint8_t * data, uint32_t len)
{
    uint32_t result;
    uint32_t i;

    if (len < 4)
        return 0;

    result = *data++ << 24;
    result |= *data++ << 16;
    result |= *data++ << 8;
    result |= *data++;
    result = ~result;
    len -= 4;

    for (i = 0; i < len; i++)
    {
        result = (result << 8 | *data++) ^ crctab[result >> 24];
    }

    return ~result;
} // END calc_mpeg2_crc()

rmf_osal_Bool rmf_OobSiCache::isVersioningByCRC32(void)
{
        return g_sitp_si_versioning_by_crc32;
}

void rmf_OobSiCache::dump_si_cache()
{
        rmf_SiTableEntry *walker =  g_si_entry;

    while (walker)
    {
        printf("\nDUMP RChannelVCN#%06d-%s#%06x-Name[%s]-State-[%d]-Freq[%09d]-Mode[%04d]-Prog[%08d]\n",
                walker->virtual_channel_number,
                walker->isAppType ? "APPID" : "SRCID",
                walker->isAppType ? walker->app_id : walker->source_id,
                walker->source_name_entry ? (walker->source_name_entry->source_names ? walker->source_name_entry->source_names->string : NULL) : NULL,
                walker->state,
                g_frequency[walker->freq_index],
                g_mode[walker->mode_index],
                walker->program_number);
        walker = walker->next;
    }
}

#ifdef ENABLE_TIME_UPDATE
rmf_osal_Bool rmf_OobSiCache::IsSttAcquired()
{
    if(g_si_state == SI_STT_ACQUIRED && g_siSTTReceived == TRUE)
        return TRUE;

    return FALSE;
    
}

void rmf_OobSiCache::setSTTTimeZone(int32_t timezoneinhours, int32_t daylightflag)
{
	m_timezoneinhours = timezoneinhours;
	m_daylightflag = daylightflag;
}

rmf_Error rmf_OobSiCache::getSTTTimeZone(int32_t *pTimezoneinhours, int32_t *pDaylightflag)
{
    rmf_Error ret = RMF_SUCCESS;

    if((pTimezoneinhours == NULL) || (pDaylightflag == NULL))
        ret = RMF_OSAL_EINVAL;
    else
    {
        *pTimezoneinhours = m_timezoneinhours;
        *pDaylightflag = m_daylightflag;
    }

    return ret;
}
#endif

void rmf_OobSiCache::SetAllSIEntriesUndefinedUnmapped()
{
	rmf_SiTableEntry *walker  = g_si_entry;

	while(walker)
	{
		walker->state = SIENTRY_UNSPECIFIED;
		walker = walker->next;
	}
}

void rmf_OobSiCache::setSTTAcquiredStatus( bool status )
{
	sttAquiredStatus = status;
#ifdef LOGMILESTONE
	logMilestone("STT_ACQUIRED");
#else
	RDK_LOG(RDK_LOG_WARN, "LOG.RDK.SI", "STT_ACQUIRED \n");
#endif
	RDK_LOG(RDK_LOG_WARN, "LOG.RDK.SI", "STTAquired status is set to %s. \n", (sttAquiredStatus ? "TRUE":"FALSE"));
	return;
}

bool rmf_OobSiCache::getSTTAcquiredStatus( )
{
	return sttAquiredStatus;
}


