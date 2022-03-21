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
 * @file rmf_oobsimgr.cpp
 */

#include <string>
#include <string.h>
#include <stdint.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#ifdef TEST_WITH_PODMGR
#include "podimpl.h"
#endif
#include "rmf_oobsimgr.h"
#include "rmf_sectionfilter_oob.h"
#include <stdlib.h>
#include <unistd.h>
#if USE_SYSRES_MLT
#include "rpl_malloc.h"
#include "rpl_new.h"
#endif
#ifdef GLI
#include "sysMgr.h"
#include "libIBus.h"
#endif

#include <errno.h>
#ifdef TEST_WITH_PODMGR
#include "core_events.h"
#include "cardUtils.h"
#endif

#include "rfcapi.h"

// if USE_NTPCLIENT
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>           // ipdu-ntp
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#define HOST_NAME "127.0.0.1"
#define HOST_PORT "50050"
#define BUF_LEN 512
#define NTP_AUTHSERVICE_POST_URL "/authService/getBootstrapProperty"
#define NTP_POST_FIELD "ntpHost"
// end

#define MAX_EVTQ_NAME_LEN 100
#define FILTER_STATS_DUMP_INTERVAL 4*60*1000 //4 minutes.
rmf_OobSiMgr *rmf_OobSiMgr::m_pSiMgr = NULL;
static bool isNTPErrorToBePrinted = true;
static bool isIPObtainedGood = false;
static char ipAddr [ INET6_ADDRSTRLEN ];
static int IPVersion = AF_INET ; //We default to IPv4

/*
 ** ***********************************************************************************************
 ** Description:
 ** Event handler for the state RMF_SI_STATE_WAIT_TABLE_REVISION.
 ** Handles the SF events:
 ** RMF_SF_EVENT_SECTION_FOUND, RMF_SF_EVENT_LAST_SECTION_FOUND.
 **
 ** RMF_SF_EVENT_SECTION_FOUND, RMF_SF_EVENT_LAST_SECTION_FOUND - .
 **
 ** All other events are handled in the function sitp_si_get_table().
 ** ***********************************************************************************************
 */
rmf_Error rmf_OobSiMgr::sitp_si_state_wait_table_revision_handler(uint32_t si_event,
        uint32_t event_data)
{
        rmf_Error retCode = RMF_SUCCESS;
        rmf_FilterSectionHandle section_handle = 0;
        uint8_t current_section_number = 0;
        uint8_t last_section_number = 0;
        uint8_t version = 0;
        uint32_t crc = 0;
        uint32_t crc2 = 0;
        uint32_t unique_id = 0;
        uint32_t section_index = 0;
		rmf_osal_Bool all_sections_acquired = false;
        RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                        "<%s: %s> - Enter\n",
                        SIMODULE, __FUNCTION__);

#ifdef TEST_WITH_PODMGR

        switch (si_event)
        {
        case RMF_SF_EVENT_SECTION_FOUND:
        case RMF_SF_EVENT_LAST_SECTION_FOUND:
        {
        RDK_LOG(
                                        RDK_LOG_TRACE1,
                                        "LOG.RDK.SI",
                                        "<%s: %s> - got new section\n",
                                        SIMODULE, __FUNCTION__);


                /*
                 * The event data is the unique_id.  This was
                 * verified before being passed to the handler
                 */
                unique_id = event_data;

                /* get section */
                retCode = get_table_section(&section_handle, unique_id);
                if (RMF_SUCCESS != retCode)
                {
                        RDK_LOG(
                                        RDK_LOG_ERROR,
                                        "LOG.RDK.SI",
                                        "<%s: %s> - failed to get the section handle: 0x%x, unique_id: %d\n",
                                        SIMODULE, __FUNCTION__, g_table->table_id, event_data);

                        /*
                         * Release the filter, reset the section array, and go back
                         * to sitp_si_state_wait_table_revision.
                         */
                        release_filter(g_table, unique_id); // table, unquie_id
                        reset_section_array(g_table->rev_type);
                        g_table->table_state = RMF_SI_STATE_WAIT_TABLE_REVISION;
                        g_sitp_si_timeout = g_sitp_si_update_poll_interval;
                        retCode = RMF_SI_FAILURE;
                        break;
                }

                /*
                 * Parse for version and number of sections
                 * or the crc
                 */
                retCode = m_pOobSiParser->get_revision_data(section_handle, &version,
                                &current_section_number, &last_section_number, &crc);
                if (RMF_SUCCESS != retCode)
                {
                        RDK_LOG(
                                        RDK_LOG_ERROR,
                                        "LOG.RDK.SI",
                                        "<%s: %s> - failed to parse RDD - table_id: 0x%x\n",
                                        SIMODULE, __FUNCTION__, g_table->table_id);

                        reset_section_array(g_table->rev_type);
                        g_table->table_state = RMF_SI_STATE_WAIT_TABLE_REVISION;
                        g_sitp_si_timeout = g_sitp_si_update_poll_interval;
                        break;
                }

                /*
                 * determine the type of versioning that sitp_si
                 * is going to use for this table
                 */
                if (version == NO_VERSION)
                {
                        g_table->rev_type = RMF_SI_REV_TYPE_CRC;
                }
                else
                {
                        g_table->rev_type = RMF_SI_REV_TYPE_RDD;
                }
                RDK_LOG(
                                RDK_LOG_TRACE5,
                                "LOG.RDK.SI",
                                "<%s: %s> - Revision type = %s\n",
                                SIMODULE, __FUNCTION__, g_table->rev_type == RMF_SI_REV_TYPE_RDD ? "RDD"
                                                : "CRC");

                switch (g_table->rev_type)
                {
                case RMF_SI_REV_TYPE_RDD:
                {
                        release_filter(g_table, unique_id); // table, unquie_id

                        /* compare version to parsed version */
                        if (version == g_table->version_number)
                        {
                                /*
                 * versions are the same, so release_section
                 * the section and set state to wait_table_rev
                 */
                                release_table_section(g_table, section_handle);
                                RDK_LOG(
                                                RDK_LOG_TRACE5,
                                                "LOG.RDK.SI",
                                                "<%s: %s> - Version matched for table: 0x%x, subtype: %d, table version: %d rdd_version: %d\n",
                                                SIMODULE, __FUNCTION__, g_table->table_id, g_table->subtype,
                                                g_table->version_number, version);
                                /* set poll interval timer */
                                /* no state change */
                                g_table->table_state = RMF_SI_STATE_WAIT_TABLE_REVISION;
                                g_sitp_si_timeout = g_sitp_si_update_poll_interval;
                                break;
                        }
                        else
                        {
                                /*
                                 * The versions are are different, so
                                 * set version and number of sections in the table
                                 * data struct (last_section_number is 0 based)
                                 * and parse the section.
                                 */
                                g_table->version_number = version;

                                /*
                                 */
                                g_table->number_sections = (uint8_t)(last_section_number + 1);

								if (RMF_SUCCESS != m_pOobSiParser->parseAndUpdateTable(g_table,
                                                section_handle, &g_table->version_number,
                                                &current_section_number, &last_section_number, &crc2))
                                {
                                        release_table_section(g_table, section_handle);
                                        reset_section_array(g_table->rev_type);
                                        g_table->table_state = RMF_SI_STATE_WAIT_TABLE_REVISION;
                                        g_sitp_si_timeout = g_sitp_si_update_poll_interval;
                                        break;
                                }
								RDK_LOG(
										RDK_LOG_ERROR,
										"LOG.RDK.SI",
										"<%s: %s> RDD version update for table: 0x%x, subtype: %d section: %d, last section: %d, rdd_version: %d\n",
										SIMODULE, __FUNCTION__, g_table->table_id, g_table->subtype, 
										current_section_number, last_section_number, version);
                                /*
                                 * Mark the section as acquired and release the
                                 * section handle.
                                 */
                                g_table->section[current_section_number].section_acquired
                                                = true;

                                release_table_section(g_table, section_handle);

                                /*
                 * Set filter for the number of sections. Use the 'g_sitp_si_filter_multiplier'
                 * value multiplied by the number of sections to make sure we get all of the
                 * sections.
                 */
                if (true != (g_table->table_acquired
                        = isTable_acquired(g_table, &all_sections_acquired)))
                {
                    retCode = activate_filter(g_table,
                            (g_table->number_sections
                                    * g_sitp_si_filter_multiplier));
                    if (RMF_SUCCESS != retCode)
                    {
                        RDK_LOG(
                                RDK_LOG_ERROR,
                                "LOG.RDK.SI",
                                "<%s: %s> - Error - Failed to activate filter. event: %s\n",
                                SIMODULE, __FUNCTION__, sitp_si_eventToString(si_event));
                        reset_section_array(g_table->rev_type);
                    }
                    /* transition to wait_table_complete */
                    g_table->table_state = RMF_SI_STATE_WAIT_TABLE_COMPLETE;
                    g_sitp_si_timeout = g_sitp_si_status_update_time_interval;
                }
                else
                {
                    set_tablesAcquired(g_table);
                    reset_section_array(g_table->rev_type);
                }
            }
            break;
        }
        case RMF_SI_REV_TYPE_CRC:
        {
            rmf_osal_TimeMillis current_time = 0;
            rmf_osal_timeGetMillis(&current_time);
            rmf_osal_Bool filter_released = FALSE;
            /*
             * rev_sample_count holds the value of the times
             * this section has matched.  When this value ==
             * number_sections (timesToMatch in activate_filter call),
             * release the filter and try to rev again on the timeout.
             */
                        g_table->rev_sample_count++;

            RDK_LOG(
                    RDK_LOG_TRACE5,
                    "LOG.RDK.SI",
                    "<%s: %s> rev_sample_count = %d\n",
                    SIMODULE, __FUNCTION__, g_table->rev_sample_count);

            /*
             * compare crc to CRCs saved in the table data struct.
             */
                        if (true == matchedCRC32(g_table, crc, &section_index))
                        {
                                /*
                 * Matched CRC, so release_section
                 * and stay in the revisioning state
                 */
                retCode = release_table_section(g_table, section_handle);
                RDK_LOG(
                        RDK_LOG_TRACE5,
                        "LOG.RDK.SI",
                        "<%s: %s> - CRC matched for table: 0x%x, subtype: %d, crc: 0x%08x, section_array index: %d \n",
                        SIMODULE, __FUNCTION__, g_table->table_id, g_table->subtype, crc,
                        section_index);
                g_table->table_state = RMF_SI_STATE_WAIT_TABLE_REVISION;
                g_sitp_si_timeout = g_sitp_si_update_poll_interval;
                g_table->section[section_index].last_seen_time = current_time;
            }
            else
            {
                /*
                 * CRC didn't match, so this must be a new
                 * section.  Parse the new section.
                 */
				RDK_LOG(
						RDK_LOG_ERROR,
						"LOG.RDK.SI",
						"<%s: %s> - New CRC for table: 0x%x, subtype: %d, crc: 0x%08x number_sections: %d \n",
						SIMODULE, __FUNCTION__, g_table->table_id, g_table->subtype, crc, (g_table->number_sections + 1));
                                if (RMF_SUCCESS != m_pOobSiParser->parseAndUpdateTable(g_table,
                                                section_handle, &g_table->version_number,
                                                &current_section_number, &last_section_number, &crc2))
                                {
                                        release_table_section(g_table, section_handle);

                                        /*
                                         * FAIL: set poll interval timer - no state change
                                         */
                                        reset_section_array(g_table->rev_type);
                                        g_table->table_state = RMF_SI_STATE_WAIT_TABLE_REVISION;
                                        g_sitp_si_timeout = g_sitp_si_update_poll_interval;
                                        retCode = RMF_SI_FAILURE;
                                        break;
                                }

                                release_table_section(g_table, section_handle);

                                if (g_table->number_sections == MAX_SECTION_NUMBER)
                                {
                                        RDK_LOG(
                                                        RDK_LOG_ERROR,
                                                        "LOG.RDK.SI",
                                                        "<%s: %s> -  g_table->number_sections: %d reached max section number..\n",
                                                        SIMODULE, __FUNCTION__, g_table->number_sections);
                                }
                                else
                                {
                                        /*
                                         * Mark the section as acquired, if seen_count is matched after
                                         * setting crc and increment number of sections in data struct
                                         */
                                        g_table->section[g_table->number_sections].crc_32 = crc;

                    g_table->section[g_table->number_sections].crc_section_match_count = 1;
                    g_table->section[g_table->number_sections].last_seen_time = current_time;

                                        RDK_LOG(
                                                        RDK_LOG_TRACE5,
                                                        "LOG.RDK.SI",
                                                        "<%s: %s> -  crc_section_match_count: %d\n",
                                                        SIMODULE, __FUNCTION__,
                                                        g_table->section[g_table->number_sections].crc_section_match_count);

                                        if (checkCRCMatchCount(g_table, g_table->number_sections)
                                                        == true)
                                        {
                                                g_table->section[g_table->number_sections].section_acquired
                                                                = true;
                                        }
                                        g_table->number_sections++;
                                }

                                /*
                                 * set filter for the number of sections.
                                 * Don't do anything if the first section == last section
                                 */
								if (true == (g_table->table_acquired
                                                = isTable_acquired(g_table, &all_sections_acquired)))
								{
									set_tablesAcquired(g_table);
								}
								if(false == all_sections_acquired)
                                {
                                    if(0 != g_table->table_unique_id)
                                    {
                                        release_filter(g_table, g_table->table_unique_id);
                                        /* The current filter has been released and a new one is about to be started. 
                                         * Set the below flag so that we do not accidentaly release the brand new filter
                                         * if si_event is RMF_SF_EVENT_LAST_SECTION_FOUND. */
                                        filter_released = TRUE;
                                    }
                                        retCode = activate_filter(g_table,
                                                        g_sitp_si_initial_section_match_count);
                                        if (RMF_SUCCESS != retCode)
                                        {
                                                RDK_LOG(
                                                                RDK_LOG_ERROR,
                                                                "LOG.RDK.SI",
                                                                "<%s: %s> - Failed to activate filter. event: %s\n",
                                                                SIMODULE, __FUNCTION__, sitp_si_eventToString(si_event));
                                        }
                                        /* transition to wait_table_complete */
                                        g_table->table_state = RMF_SI_STATE_WAIT_TABLE_COMPLETE;
                                        g_sitp_si_timeout = g_sitp_si_status_update_time_interval;
                }
                else
                {
			        RDK_LOG(
                                                        RDK_LOG_DEBUG,
                                                        "LOG.RDK.SI",
                                                        "<%s: %s: %d> -  calling set_tablesAcquired()\n",
                                                        SIMODULE, __FUNCTION__, __LINE__);
                    reset_section_array(g_table->rev_type);
                    g_table->table_state = RMF_SI_STATE_WAIT_TABLE_REVISION;
                    if (g_stip_si_process_table_revisions)
                    {
                        /*
                         ** If the revisioning is by CRC and THE NIT has just been acquired, then
                         ** set the timer to SHORT_TIMER to expedite the acquisition of the SVCT.
                         */
                        g_sitp_si_timeout = (getTablesAcquired()
                                <= OOB_TABLES_ACQUIRED) ? SHORT_TIMER
                                : g_sitp_si_update_poll_interval;
                    }
                    else
                    {
                        g_sitp_si_timeout = g_sitp_si_status_update_time_interval;
                    }
                                }

                        }

            /* If the filter has returned its last section, purge any outdated sections and release the filter.*/
            if(RMF_SF_EVENT_LAST_SECTION_FOUND == si_event)
            {
                purgeSectionsNotMatchedByFilter(g_table);
                
                /* If the filter session that returned "last section" event has not been released yet, do that now. */
                if(FALSE == filter_released)
                {
                    RDK_LOG(
                            RDK_LOG_TRACE5,
                            "LOG.RDK.SI",
                            "<%s: %s> release filt, rev_count = 0\n",
                            SIMODULE, __FUNCTION__);
                    release_filter(g_table, unique_id); // table, unquie_id
                    g_table->rev_sample_count = 0;
                }
            }
            break;
                }
                case RMF_SI_REV_TYPE_UNKNOWN:
                {
                        RDK_LOG(
                                        RDK_LOG_ERROR,
                                        "LOG.RDK.SI",
                                        "<%s: %s> - Error - revision type unknown-event: %s\n",
                                        SIMODULE, __FUNCTION__, sitp_si_eventToString(si_event));
                        break;
                }
                }
                break;
        }
        default:
        {
                RDK_LOG(
                                RDK_LOG_ERROR,
                                "LOG.RDK.SI",
                                "<%s: %s> - event not handled in this state: %s\n",
                                SIMODULE, __FUNCTION__, sitp_si_eventToString(si_event));
                retCode = RMF_SI_FAILURE;
                break;
        }
        }
#endif //TEST_WITH_PODMGR
    return retCode;
}

/*
 ** ***********************************************************************************************
 ** Description:
 ** Event handler for the state SITP_SI_STATE_WAIT_TABLE_COMPLETE.
 ** Handles the SF events:
 ** RMF_SF_EVENT_SECTION_FOUND, RMF_SF_EVENT_LAST_SECTION_FOUND.
 **
 ** RMF_SF_EVENT_SECTION_FOUND, RMF_SF_EVENT_LAST_SECTION_FOUND - .
 **
 ** All other events are handled in the function sitp_si_get_table().
 ** ***********************************************************************************************
 */
rmf_Error rmf_OobSiMgr::sitp_si_state_wait_table_complete_handler(uint32_t si_event,
        uint32_t event_data)
{
        rmf_Error retCode = RMF_SUCCESS;
        rmf_FilterSectionHandle section_handle = 0;
        uint8_t current_section_number = 0;
        uint8_t last_section_number = 0;
        uint8_t version = 0;
        uint32_t crc = 0;
        uint32_t crc2 = 0;
        uint32_t unique_id = 0;
        uint32_t section_index = 0;
		rmf_osal_Bool all_sections_acquired = false;
        RDK_LOG(
                        RDK_LOG_TRACE4,
                        "LOG.RDK.SI",
            "<%s: %s> - Enter\n",
                        SIMODULE, __FUNCTION__);

#ifdef TEST_WITH_PODMGR

        switch (si_event)
        {
        case RMF_SF_EVENT_SECTION_FOUND:
        case RMF_SF_EVENT_LAST_SECTION_FOUND:
        {
        RDK_LOG(
                                        RDK_LOG_TRACE1,
                                        "LOG.RDK.SI",
                                        "<%s: %s> - got new section\n",
                                        SIMODULE, __FUNCTION__);

                unique_id = event_data;

                /*
         * get the new table section
         */
                retCode = get_table_section(&section_handle, event_data);
                if (RMF_SUCCESS != retCode)
                {
                        RDK_LOG(
                                        RDK_LOG_ERROR,
                                        "LOG.RDK.SI",
                                        "<%s: %s> - failed to get the section handle: 0x%x, unique_id: %d\n",
                                        SIMODULE, __FUNCTION__, g_table->table_id, event_data);
                        /*
                         * Release the filter, reset the section array, and
                         * go back to sitp_si_state_wait_table_revision.
                         */
                        release_filter(g_table, unique_id); // table, unquie_id
                        reset_section_array(g_table->rev_type);
                        g_table->table_state = RMF_SI_STATE_WAIT_TABLE_REVISION;
                        g_sitp_si_timeout = g_sitp_si_update_poll_interval;

                        break;
                }

                /*
                 * parse for the revisioning data
                 */
                retCode = m_pOobSiParser->get_revision_data(section_handle, &version,
                                &current_section_number, &last_section_number, &crc);
                if (RMF_SUCCESS != retCode)
                {
                        RDK_LOG(
                                        RDK_LOG_ERROR,
                                        "LOG.RDK.SI",
                                        "<%s: %s> - failed to parse RDD - table_id: 0x%x\n",
                                        SIMODULE, __FUNCTION__, g_table->table_id);
                        reset_section_array(g_table->rev_type);
                        g_table->table_state = RMF_SI_STATE_WAIT_TABLE_REVISION;
                        g_sitp_si_timeout = g_sitp_si_update_poll_interval;
                        break;
                }

                RDK_LOG(
                                RDK_LOG_TRACE5,
                                "LOG.RDK.SI",
                                "<%s: %s> - successfully parsed revision data - table_id: %s, version: %d, current_section_number: %d, last_section_number: %d, crc: 0x%08x\n",
                                SIMODULE, __FUNCTION__, sitp_si_tableToString(g_table->table_id), version,
                                current_section_number, last_section_number, crc);

        /*
         * determine the type of versioning that sitp_si
         * is going to use for this table
         */
        if (version == NO_VERSION)
        {
            g_table->rev_type = RMF_SI_REV_TYPE_CRC;
        }
        else
        {
            g_table->rev_type = RMF_SI_REV_TYPE_RDD;
        }
        RDK_LOG(
                RDK_LOG_TRACE5,
                "LOG.RDK.SI",
                "<%s: %s> - Revision type = %s\n",
                SIMODULE, __FUNCTION__, g_table->rev_type == RMF_SI_REV_TYPE_RDD ? "RDD"
                        : "CRC");
                switch (g_table->rev_type)
                {
                case RMF_SI_REV_TYPE_RDD:
					{
						if(version != g_table->version_number)
						{
							reset_section_array(g_table->rev_type);
							g_table->version_number = version;
							g_table->number_sections = (uint8_t)(last_section_number + 1);
						}
                        /*
                         * If sitp already parsed this table section, release the
                         * section and wait for the next event.
                         *
                         * if this duplicate section is RMF_SF_EVENT_LAST_SECTION_FOUND, there won't be a next event
                         * So check to see if need to restart filter.
                         */
                        if (true
                                        == g_table->section[current_section_number].section_acquired)
                        {
                                /* release_section */
                                release_table_section(g_table, section_handle);
                                /* transition to wait_table_complete */
                                g_table->table_state = RMF_SI_STATE_WAIT_TABLE_COMPLETE;
                g_sitp_si_timeout = g_sitp_si_status_update_time_interval;

                 if (si_event == RMF_SF_EVENT_LAST_SECTION_FOUND)
                {
                    /* Stay in wait_table_complete */
                    g_table->table_state = RMF_SI_STATE_WAIT_TABLE_COMPLETE;
                    g_sitp_si_timeout = g_sitp_si_update_poll_interval;
                    if (si_event == RMF_SF_EVENT_LAST_SECTION_FOUND && (false == (g_table->table_acquired = isTable_acquired(g_table, &all_sections_acquired))))
                    {
                        RDK_LOG(
                                RDK_LOG_INFO,
                                "LOG.RDK.SI",
                                "<%s: %s> - Continue to filter incomplete table with a new filter.\n",
                                SIMODULE, __FUNCTION__, g_table->table_id, g_table->subtype);

                        RDK_LOG(
                                RDK_LOG_TRACE5,
                                "LOG.RDK.SI",
                                "<%s: %s> - Processed RMF_SF_EVENT_LAST_SECTION_FOUND without completing acquisition - resetting filter for TID 0x%x, subtype: %d\n",
                                SIMODULE, __FUNCTION__, g_table->table_id, g_table->subtype);
                        g_table->table_unique_id = 0;
                        retCode = activate_filter(g_table,
                                (g_table->number_sections * g_sitp_si_filter_multiplier));
                        if (RMF_SUCCESS != retCode)
                        {
                            RDK_LOG(
                                    RDK_LOG_ERROR,
                                    "LOG.RDK.SI",
                                    "<%s: %s> - Failed to activate filter. event: %s\n",
                                    SIMODULE, __FUNCTION__, sitp_si_eventToString(si_event));
                        }
                    }
                }
                                break;
                        }

                        if (RMF_SUCCESS != m_pOobSiParser->parseAndUpdateTable(g_table, section_handle,
                                        &(g_table->version_number), &current_section_number,
                                        &last_section_number, &crc2))
                        {
                                /* release_section */
                                release_table_section(g_table, section_handle);
                                /* transition to wait_table_revision  to try again*/
                                reset_section_array(g_table->rev_type);
                                g_table->table_state = RMF_SI_STATE_WAIT_TABLE_REVISION;
                                g_sitp_si_timeout = g_sitp_si_update_poll_interval;
                                break;
                        }

                    
			RDK_LOG(
					RDK_LOG_ERROR,
					"LOG.RDK.SI",
					"<%s: %s> RDD version update for table: 0x%x, subtype: %d section: %d, last section: %d, rdd_version: %d\n",
					SIMODULE, __FUNCTION__, g_table->table_id, g_table->subtype, 
					current_section_number, last_section_number, version);

			/*
             * Mark the section as acquired.
             */
                        g_table->section[current_section_number].section_acquired = true;

                        /* release_section */
                        release_table_section(g_table, section_handle);

                        if (true == (g_table->table_acquired = isTable_acquired(g_table, &all_sections_acquired)))
                        {
			        RDK_LOG(
                                                        RDK_LOG_DEBUG,
                                                        "LOG.RDK.SI",
                                                        "<%s: %s: %d> -  calling set_tablesAcquired()\n",
                                                        SIMODULE, __FUNCTION__, __LINE__);

                                set_tablesAcquired(g_table);
                                release_filter(g_table, unique_id); // table, unquie_id
                                /*
                 * Reset the section array, and go to sitp_si_state_wait_table_revision.
                 */
                                reset_section_array(g_table->rev_type);
                                g_table->table_state = RMF_SI_STATE_WAIT_TABLE_REVISION;
                                if (g_stip_si_process_table_revisions)
                                {
                                        g_sitp_si_timeout = g_sitp_si_update_poll_interval;
                                }
                                else
                                {
                                        g_sitp_si_timeout = g_sitp_si_status_update_time_interval;
                                }
                                break;
                        }
            else // Table not acquired
                        {
                /* Stay in wait_table_complete */
                                g_table->table_state = RMF_SI_STATE_WAIT_TABLE_COMPLETE;
                                g_sitp_si_timeout = g_sitp_si_update_poll_interval;
                                if (si_event == RMF_SF_EVENT_LAST_SECTION_FOUND)
                                {
                                        RDK_LOG(
                                                        RDK_LOG_TRACE5,
                                                        "LOG.RDK.SI",
                                                        "<%s: %s> - Processed RMF_SF_EVENT_LAST_SECTION_FOUND without completing acquisition - resetting filter for TID 0x%x, subtype: %d\n",
                                                        SIMODULE, __FUNCTION__, g_table->table_id, g_table->subtype);
                                        g_table->table_unique_id = 0;
                                        retCode = activate_filter(g_table,
                                  (g_table->number_sections * g_sitp_si_filter_multiplier));
                    if (RMF_SUCCESS != retCode)
                    {
                        RDK_LOG(
                                RDK_LOG_ERROR,
                                "LOG.RDK.SI",
                                "<%s: %s> - Failed to activate filter. event: %s\n",
                                SIMODULE, __FUNCTION__, sitp_si_eventToString(si_event));
                    }
                }
            }
            break;
        }

         case RMF_SI_REV_TYPE_CRC:
        {
            rmf_osal_TimeMillis current_time = 0;
            rmf_osal_timeGetMillis(&current_time);
			rmf_osal_Bool new_section_parsed = FALSE;

            /* purge old section matches when doing CRC-based revisioning */
            purgeOldSectionMatches(g_table, current_time);

            /*
             * compare crc to CRCs saved in the table data struct.
             */
                        if (true == matchedCRC32(g_table, crc, &section_index))
                        {
                                /*
                 * Matched CRC, so release_section
                 * and stay in the revisioning state
                 */
                                retCode = release_table_section(g_table, section_handle);
                                RDK_LOG(
                                                RDK_LOG_TRACE5,
                                                "LOG.RDK.SI",
                                                "<%s: %s> - CRC matched for table: 0x%x, subtype: %d, crc: 0x%08x, section_array index: %d \n",
                                                SIMODULE, __FUNCTION__, g_table->table_id, g_table->subtype, crc,
                                                section_index);
                                /*
                 * increment the match count, so sitp can bail when it is
                 * confident that all of the table is acquired.
                 */
                g_table->section[section_index].crc_section_match_count++;
                g_table->section[section_index].last_seen_time = current_time;
                RDK_LOG(
                        RDK_LOG_TRACE5,
                        "LOG.RDK.SI",
                        "<%s: %s> -  crc_section_match_count: %d\n",
                        SIMODULE, __FUNCTION__,
                        g_table->section[section_index].crc_section_match_count);
                if (checkCRCMatchCount(g_table, section_index) == true)
                {
                    g_table->section[section_index].section_acquired = true;
                }
                
                RDK_LOG(
                        RDK_LOG_TRACE5,
                        "LOG.RDK.SI",
                        "<%s: %s> -  section_index: %d, section_acquired: %s\n",
                        SIMODULE, __FUNCTION__,
                        section_index,
                        g_table->section[section_index].section_acquired?"true":"false");
            }
            else
            {
                /*
                 * no match for the crc, so parse the new section
                 */
				RDK_LOG(
						RDK_LOG_ERROR,
						"LOG.RDK.SI",
						"<%s: %s> - New CRC for table: 0x%x, subtype: %d, crc: 0x%08x number_sections: %d \n",
						SIMODULE, __FUNCTION__, g_table->table_id, g_table->subtype, crc, (g_table->number_sections + 1));

                                if (RMF_SUCCESS != m_pOobSiParser->parseAndUpdateTable(g_table,
                                                section_handle, &g_table->version_number,
                                                &current_section_number, &last_section_number, &crc2))
                                {
                                        release_table_section(g_table, section_handle);
                                        /* set poll interval timer */
                                        /* no state change */
                                        reset_section_array(g_table->rev_type);
                                        g_table->table_state = RMF_SI_STATE_WAIT_TABLE_REVISION;
                                        g_sitp_si_timeout = g_sitp_si_update_poll_interval;
                                        retCode = RMF_SI_FAILURE;
                                        break;
                                }
								new_section_parsed = TRUE;
                                /* release_section */
                                release_table_section(g_table, section_handle);

                                if (g_table->number_sections == MAX_SECTION_NUMBER)
                                {
                                        RDK_LOG(
                                                        RDK_LOG_ERROR,
                                                        "LOG.RDK.SI",
                                                        "<%s: %s> -  g_table->number_sections: %d reached max section number..\n",
                                                        SIMODULE, __FUNCTION__, g_table->number_sections);
                                }
                                else
                                {
                                        /*
                     * Mark the section as acquired.
                     * set crc and increment number of sections in data struct
                     */
                    g_table->section[g_table->number_sections].crc_32 = crc;
                    g_table->section[g_table->number_sections].crc_section_match_count = 1;
                    g_table->section[g_table->number_sections].last_seen_time = current_time;

                                        if (checkCRCMatchCount(g_table, g_table->number_sections)
                                                        == true)
                                         {
                                                g_table->section[g_table->number_sections].section_acquired
                                                                = true;
                                        }
                                        g_table->number_sections++;
                                }
                        }

                        /*
             * Release the filter and notify SIDB if the table is acquired.
             * Else, just go wait for the next SECTION_FOUND event.
             */
						g_table->table_acquired = isTable_acquired(g_table, &all_sections_acquired);

                        if((true == new_section_parsed) && (true == g_table->table_acquired) ||
							(true == all_sections_acquired))
                        {
			        			RDK_LOG(
                                                        RDK_LOG_DEBUG,
                                                        "LOG.RDK.SI",
                                                        "<%s: %s: %d> -  calling set_tablesAcquired()\n",
                                                        SIMODULE, __FUNCTION__, __LINE__);

                                set_tablesAcquired(g_table);
						}
						if(true == all_sections_acquired)
						{
                                purgeSectionsNotMatchedByFilter(g_table);
                                release_filter(g_table, unique_id); // table, unquie_id
                                /*
                 * Reset the section array, and go back to sitp_si_state_wait_table_revision.
                 */
                                reset_section_array(g_table->rev_type);
                                g_table->table_state = RMF_SI_STATE_WAIT_TABLE_REVISION;
                                if (g_stip_si_process_table_revisions)
                                {

                                        /*
                     ** If the revisioning is by CRC and THE NIT has just been acquired, then
                     ** set the timer to SHORT_TIMER to expedite the acquisition of the SVCT.
                     */
                                        g_sitp_si_timeout = (getTablesAcquired()
                                                        <= OOB_TABLES_ACQUIRED) ? SHORT_TIMER
                                                        : g_sitp_si_update_poll_interval;
                                }
                                else
                                {
                                        g_sitp_si_timeout = g_sitp_si_status_update_time_interval;
                                }
                        }
                        else
                        {
                                /*
                 * Not done yet. Stay in this state - wait_table_complete
                 */
                                g_table->table_state = RMF_SI_STATE_WAIT_TABLE_COMPLETE;
                                g_sitp_si_timeout = g_sitp_si_update_poll_interval;
                                if (si_event == RMF_SF_EVENT_LAST_SECTION_FOUND)
                                {
                                        RDK_LOG(
                                                        RDK_LOG_TRACE5,
                                                        "LOG.RDK.SI",
                                                        "<%s: %s> - Processed RMF_SF_EVENT_LAST_SECTION_FOUND without completing acquisition - resetting filter for TID 0x%x, subtype: %d\n",
                                                        SIMODULE, __FUNCTION__, g_table->table_id, g_table->subtype);
                                        purgeSectionsNotMatchedByFilter(g_table);
                                        release_filter(g_table, unique_id);
                                        retCode = activate_filter(g_table,
                                                        g_sitp_si_initial_section_match_count);
                                        if (RMF_SUCCESS != retCode)
                                        {
                                                RDK_LOG(
                                                                RDK_LOG_ERROR,
                                                                "LOG.RDK.SI",
                                                                "<%s: %s> - Failed to activate filter. event: %s\n",
                                                                SIMODULE, __FUNCTION__, sitp_si_eventToString(si_event));
                                        }
                                }
                        }
                        break;
                }
                case RMF_SI_REV_TYPE_UNKNOWN:
                default:
                {
                        RDK_LOG(
                                        RDK_LOG_ERROR,
                                        "LOG.RDK.SI",
                                        "<%s: %s> - error  revision type is unknown\n",
                                        SIMODULE, __FUNCTION__);
                        retCode = RMF_SI_FAILURE;
                        break;
                }
                }
                break;
        }
        default:
        {
                RDK_LOG(
                                RDK_LOG_ERROR,
                                "LOG.RDK.SI",
                                "<%s: %s> - event not handled in this state: %s\n",
                                SIMODULE, __FUNCTION__, sitp_si_eventToString(si_event));
                retCode = RMF_SI_FAILURE;
        }
                break;
        }
#endif //TEST_WITH_PODMGR

        return retCode;
}

/*
 *  Select the appropriate by priority, abased on the event and event data.  Also, handle any
 *  general events that aren't associated with any particular table. ie ETIMEOUT.
 */
rmf_Error rmf_OobSiMgr::sitp_si_get_table(rmf_si_table_t ** si_table,
        uint32_t event, uint32_t data)
{
    uint32_t unique_id = 0;
    rmf_Error retCode = RMF_SUCCESS;
    uint32_t index = 0;
    uint32_t ii = 0;
    uint32_t jj = 0;
    uint32_t tables_to_set = 0;
    RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
            "<%s: %s> - Event: %s\n", SIMODULE, __FUNCTION__,
            sitp_si_eventToString(event));

    switch (event)
    {
        case OOB_VCTID_CHANGE_EVENT:
                rmf_si_reset_tables();
        case OOB_START_UP_EVENT:
        case RMF_OSAL_ETIMEOUT:
            {
                RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                        "<%s: %s> - Matched event: %s\n", SIMODULE, __FUNCTION__,
                        sitp_si_eventToString(event));

                /*
                 * Decide which table to set by testing which tables have already been
                 * acquired. The NIT-MMS, NIT-CDS, SVCT-DCM and SVCT-VCM are
                 * acquired first.  Based on the table acquired #define values
                 * ( ie CDS_AQUIRED 0x0001 ), calculate the number
                 * of tables in the g_table_array to iterate through and set.  If no tables have
                 * been acquired yet, just set NIT CDS and NIT MMS filter, if < NIT_ACQUIRED (0x003)
                 * tables have been acquired, don't set any filters, if >= NIT_ACQUIRED, set all filters
                 * that aren't already set.
                 */
                if ((getTablesAcquired() == 0) && ((event == OOB_START_UP_EVENT) || (event == OOB_VCTID_CHANGE_EVENT)))
                {
                    /*
                     ** Set filters for NIT_CDS, NIT_MMS, SVCT_DCM, SVCT_VCM, NTT_SNS
                     */
                    tables_to_set = g_sitp_si_profile_table_number;
                    /*RDK_LOG(
                            RDK_LOG_INFO,
                            "LOG.RDK.SI",
                            "<%s: %s> - setting NIT, SVCT and NTT table filters\n",
                            SIMODULE, __FUNCTION__);*/
                }
                else if (getTablesAcquired() < NIT_ACQUIRED)
                {
                    /*
                     ** Must have received a E_TIMEOUT while waiting for the initial NIT,
                     ** SVCT sub-tables. These need to be acquired before anything else can
                     ** happen, so just return and skip the handlers.
                     */
                    retCode = RMF_SI_FAILURE;
                    g_sitp_si_timeout = g_sitp_si_status_update_time_interval; // set default timeout period to wait forever
                    RDK_LOG(
                            RDK_LOG_TRACE5,
                            "LOG.RDK.SI",
                            "<%s: %s> - Do nothing, still waiting for the NIT subtables\n",
                            SIMODULE, __FUNCTION__);
                    return retCode;
                }
                /*else if (getTablesAcquired() == NIT_ACQUIRED)
                  {

                //NIT_CDS and NIT_MMS are the first two tables in the list ,
                //now get the SVCT_VCM, SVCT_DCM before getting anything else.
                tables_to_set = 4;
                RDK_LOG(
                RDK_LOG_TRACE5,
                "LOG.RDK.SI",
                "<%s: %s> - Now Only adding SVCT/VCM filters\n",
                SIMODULE, __FUNCTION__);
                }
                 */
                /*
                // Un-comment this block to test the case of setting VCM filter before DCM filter
                // and change the above line 3003 (block of (getTablesAcquired() == NIT_ACQUIRED)
                // to tables_to_set = 3.  Make this configurable via ini
                else if (getTablesAcquired() == 0x07)
                {
                //
                // NIT_CDS and NIT_MMS are the first two tables in the list ,
                // now get the SVCT_VCM, SVCT_DCM before getting anything else.
                //
                tables_to_set = 4;
                RDK_LOG(
                RDK_LOG_TRACE5,
                "LOG.RDK.SI",
                "<%s: %s> - Now Only adding SVCT (DCM, VCM) filters\n",
                SIMODULE, __FUNCTION__);
                }*/
                else
                {
                    if (true == checkForActiveFilters())
                    {
                        /*
                         * Timed out during normal polling with filters set.  Keep filters set, but tell PSI thread to look for PSIP(CVCT)
                         * Tell the PSI thread to start acquiring the CVCT.
                         */
                        tables_to_set = 0; //Don't set new filters. Just wait for the filters already set to be filled.
                    }
                    /*
                     ** E_TIMEOUT - normal operation.  Acquire all tables that have not yet been acquired.
                     */
                    // this is set to 5 (includes CDS, MMS, DCM, VCM, SNS)
                    tables_to_set = g_sitp_si_profile_table_number;
                    //RDK_LOG(RDK_LOG_TRACE5, "LOG.RDK.SI","<%s: %s> - REQUIRED OOB TABLES ACQUIRED - NORMAL ACQUISTION START...\n",
                    //        SIMODULE, __FUNCTION__);
                }

                /*
                 * release filters if there are any set.
                 * Since we are not handling single tables in this state, check
                 * to make sure we only act on those tables in the IDLE state.
                 */
                for (ii = 0; ii < tables_to_set; ii++)
                {
                    /*
                     * activate filters for the tables that are not already set.
                     */
                    if(g_table_array[ii].table_state == RMF_SI_STATE_NONE)
                    {
                        /*
                         * Now Activate new filters
                         */
                        RDK_LOG(
                                RDK_LOG_TRACE5,
                                "LOG.RDK.SI",
                                "<%s: %s> - Setting a filter for table: %d(0x%x), timesToMatch: %d\n",
                                SIMODULE, __FUNCTION__, g_table_array[ii].table_id,
                                g_table_array[ii].table_id, g_sitp_si_initial_section_match_count);
                        // tableToMatch, timesToMatch - initially set to 50, configurable via ini
                        retCode = activate_filter(&(g_table_array[ii]), g_sitp_si_initial_section_match_count);
                        if (RMF_SUCCESS != retCode)
                        {
                            RDK_LOG(
                                    RDK_LOG_ERROR,
                                    "LOG.RDK.SI",
                                    "<%s: %s> - failed to set a filter for table: %d(0x%x)\n",
                                    SIMODULE, __FUNCTION__, g_table_array[ii].table_id,
                                    g_table_array[ii].table_id);
                        }
                        else
                        {
                            RDK_LOG(
                                    RDK_LOG_TRACE5,
                                    "LOG.RDK.SI",
                                    "<%s: %s> - successfully set a filter for table: %s(0x%x), subtype: %d, unique_id: %d\n",
                                    SIMODULE, __FUNCTION__, sitp_si_tableToString(
                                        g_table_array[ii].table_id),
                                    g_table_array[ii].table_id,
                                    g_table_array[ii].subtype,
                                    g_table_array[ii].table_unique_id);
                        }
                        g_table_array[ii].table_state = RMF_SI_STATE_WAIT_TABLE_COMPLETE;
                        g_sitp_si_timeout = g_sitp_si_status_update_time_interval;
                    }
                    else if ( (g_table_array[ii].table_state != RMF_SI_STATE_NONE) &&
                            (g_table_array[ii].table_unique_id == 0) )
                    {
                        /*
                         * Now Activate the revision filters
                         */
                        if ((g_table_array[ii].rev_type == RMF_SI_REV_TYPE_CRC)
                                && (true == g_sitp_si_rev_sample_sections) && (0
                                    != g_table_array[ii].number_sections))
                        {
                            /*
                             *   If the revision type is CRC and sample_sections flag == True and number_sections != 0
                             *   and this is a timeout event.
                             */
                            RDK_LOG(
                                    RDK_LOG_TRACE5,
                                    "LOG.RDK.SI",
                                    "<%s: %s> - Setting a filter for table: %d(0x%x), timesToMatch: %d\n",
                                    SIMODULE, __FUNCTION__, g_table_array[ii].table_id,
                                    g_table_array[ii].table_id, (g_table_array[ii].number_sections + 1));
                            retCode = activate_filter(&(g_table_array[ii]),
                                    (g_table_array[ii].number_sections + 1)); // tableToMatch, timesToMatch
                        }
                        else
                        {
                            RDK_LOG(
                                    RDK_LOG_TRACE5,
                                    "LOG.RDK.SI",
                                    "<%s: %s> - Setting a filter for table: %d(0x%x), timesToMatch: 1\n",
                                    SIMODULE, __FUNCTION__, g_table_array[ii].table_id,
                                    g_table_array[ii].table_id);
                            retCode = activate_filter(&(g_table_array[ii]), 1); // tableToMatch, timesToMatch
                        }

                        if (RMF_SUCCESS != retCode)
                        {
                            /*
                             * go to the revision state to retry on the MPE_ETIMEOUT
                             */
                            RDK_LOG(
                                    RDK_LOG_ERROR,
                                    "LOG.RDK.SI",
                                    "<%s: %s> - failed to set a filter for table: %d(0x%x)\n",
                                    SIMODULE, __FUNCTION__, g_table_array[ii].table_id,
                                    g_table_array[ii].table_id);
                        }
                        else
                        {
                            RDK_LOG(
                                    RDK_LOG_TRACE5,
                                    "LOG.RDK.SI",
                                    "<%s: %s> - successfully set a filter for table: %s(0x%x), subtype: %d, unique_id: %d\n",
                                    SIMODULE, __FUNCTION__, sitp_si_tableToString(
                                        g_table_array[ii].table_id),
                                    g_table_array[ii].table_id,
                                    g_table_array[ii].subtype,
                                    g_table_array[ii].table_unique_id);
                        }
                        g_table_array[ii].table_state = RMF_SI_STATE_WAIT_TABLE_REVISION;
                        g_sitp_si_timeout = g_sitp_si_update_poll_interval;
                    }
                    else
                    {
                        RDK_LOG(
                                RDK_LOG_TRACE5,
                                "LOG.RDK.SI",
                                "<%s: %s> - No action - table_id: %s, subtype: %d has filters set.\n",
                                SIMODULE, __FUNCTION__, sitp_si_tableToString(
                                    g_table_array[ii].table_id),
                                g_table_array[ii].subtype);
                    }
                }
                /*
                 * skip the handlers
                 */
                retCode = RMF_SI_FAILURE;
                break;
            }
        case RMF_SF_EVENT_SOURCE_CLOSED:
            {
                /*
                 * Cleanup associated filters and set that table back to wait table revision
                 * First, release filter.
                 */
                retCode = release_filter(g_table, g_table->table_unique_id);
                if (RMF_SUCCESS != retCode)
                {
                    RDK_LOG(
                            RDK_LOG_ERROR,
                            "LOG.RDK.SI",
                            "<%s: %s> - failed to release filter: 0x%x, unique_id: %d\n",
                            SIMODULE, __FUNCTION__, g_table->table_id, g_table->table_unique_id);
                    retCode = RMF_SI_FAILURE;
                }
                g_table->table_state = RMF_SI_STATE_WAIT_TABLE_REVISION;
                g_sitp_si_timeout = g_sitp_si_update_poll_interval;
                /*
                 * skip the handlers
                 */
                retCode = RMF_SI_FAILURE;
                break;
            }
        
#ifdef TEST_WITH_PODMGR
        case RMF_PODSRV_EVENT_RESET_PENDING:
            {
                /*
                 * Cleanup associated filters and set that table back to wait table revision
                 * First, release filter.
                 */
                RDK_LOG(RDK_LOG_INFO,
                        "LOG.RDK.SI",
                        "<%s: %s> - Received MPE_POD_EVENT_RESET_PENDING, resetting table filters..\n",
                        SIMODULE, __FUNCTION__);
                for (ii = 0; ii < g_sitp_si_profile_table_number; ii++)
                {
                    /*
                     * release filters for the tables that are set.
                     */
                    if ((g_table_array[ii].table_unique_id != 0))
                    {
                        retCode = release_filter(&g_table_array[ii], g_table_array[ii].table_unique_id);
                        if (RMF_SUCCESS != retCode)
                        {
                            RDK_LOG(
                                    RDK_LOG_ERROR,
                                    "LOG.RDK.SI",
                                    "<%s: %s> - failed to release filter: 0x%x, unique_id: %d\n",
                                    SIMODULE, __FUNCTION__, g_table_array[ii].table_id, g_table_array[ii].table_unique_id);
                            retCode = RMF_SI_FAILURE;
                        }
                    }
                    g_table_array[ii].table_state = RMF_SI_STATE_NONE;
                    g_table_array[ii].rev_type = RMF_SI_REV_TYPE_UNKNOWN;
                    g_table_array[ii].rev_sample_count = 0;
                    g_table_array[ii].table_acquired = false;
                    g_table_array[ii].table_unique_id = 0;
                    g_table_array[ii].version_number = SITP_SI_INIT_VERSION;
                    g_table_array[ii].number_sections = 0;
					g_table_array[ii].rolling_match_counter = 0;
                    for (jj = 0; jj < MAX_SECTION_NUMBER; jj++)
                    {
                        g_table_array[ii].section[jj].section_acquired = false;
                        g_table_array[ii].section[jj].crc_32 = SITP_SI_INIT_CRC_32;
                        g_table_array[ii].section[jj].crc_section_match_count = 0;
                        g_table_array[ii].section[jj].last_seen_time = 0;
                    }
                }
                // Reset the global tables acquired variable
                g_sitp_siTablesAcquired = 0;
                g_sitp_si_timeout = g_sitp_si_status_update_time_interval;
                /*
                 * skip the handlers
                 */
                retCode = RMF_SI_FAILURE;
                break;
            } // End MPE_PODMGR_EVENT_RESET_PENDING
#endif      
        case RMF_SF_EVENT_SECTION_FOUND:
        case RMF_SF_EVENT_LAST_SECTION_FOUND:
            {

                RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.SI",
                        "<%s: %s> - Matched event: %s, unique_id: %d\n",
                        SIMODULE, __FUNCTION__, sitp_si_eventToString(event), data);
                unique_id = data;
                retCode = findTableIndexByUniqueId(&index, unique_id);
                if (RMF_SUCCESS == retCode)
                {
                    *si_table = &(g_table_array[index]);
                }
                else
                {

                    RDK_LOG(
                            RDK_LOG_TRACE5,
                            "LOG.RDK.SI",
                            "<%s: %s> - No table Matched event: %s, with unique_id: %d - SKIP HANDLER - tables acquired: %d\n",
                            SIMODULE, __FUNCTION__, sitp_si_eventToString(event), data,
                            getTablesAcquired());
                    /* no matching table - skip handler */
                    retCode = RMF_SI_FAILURE;
                    g_sitp_si_timeout
                        = (getTablesAcquired() <= OOB_TABLES_ACQUIRED) ? SHORT_TIMER
                        : g_sitp_si_update_poll_interval;
                }
                break;
            }
        default:
            {
                RDK_LOG(
                        RDK_LOG_TRACE5,
                        "LOG.RDK.SI",
                        "<%s: %s> - No Matched for event: %s - SKIP HANDLER\n",
                        SIMODULE, __FUNCTION__, sitp_si_eventToString(event));
                /* no matching table - skip handler */
                retCode = RMF_SI_FAILURE;
                break;
            }
    }

    return retCode;
}

void rmf_OobSiMgr::si_Shutdown(void)
{
        /*
     * delete the table data array and release all of the memory.
     */
        rmf_Error retCode = rmf_osal_memFreeP(RMF_OSAL_MEM_SI_OOB, g_table_array);
        if (RMF_SUCCESS != retCode)
        {
                RDK_LOG(
                                RDK_LOG_ERROR,
                                "LOG.RDK.SI",
                                "<%s: %s> - Error - Failed to Free SI table array: %d\n",
                                SIMODULE, __FUNCTION__, retCode);
        }

        rmf_osal_eventqueue_delete(g_sitp_si_queue); // delete the OOB global queue

        if (g_sitp_pod_queue)
        {
#ifdef TEST_WITH_PODMGR
                podmgrUnRegisterEvents(g_sitp_pod_queue);
#endif
                rmf_osal_eventqueue_delete(g_sitp_pod_queue);
                g_sitp_pod_queue = 0;
        }
    /*
     *  We are done with the condition, now delete it.
     */
    rmf_osal_condDelete(g_sitp_stt_condition);

}

void rmf_OobSiMgr::sitp_siWorkerThread(void * data)
{
    rmf_OobSiMgr *pOobSiMgr = (rmf_OobSiMgr*)data;
    pOobSiMgr->siWorkerThreadFn();
}

void rmf_OobSiMgr::sitp_siSTTWorkerThread(void * data)
{
    rmf_OobSiMgr *pOobSiMgr = (rmf_OobSiMgr*)data;
    pOobSiMgr->siSTTWorkerThreadFn();
}

// if USE_NTPCLIENT
void rmf_OobSiMgr::ntpclientWorkerThread(void * data)
{
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI","<%s: %s> - %d\n", SIMODULE, __FUNCTION__, __LINE__);
	rmf_OobSiMgr *pOobSiMgr = (rmf_OobSiMgr*)data;
    pOobSiMgr->ntpclientWorkerThreadFn();
}
// endif
void rmf_OobSiMgr::printMatchStatistics()
{
	uint32_t ii = 0;

	RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "<%s: %s> : CDS: 0x%x, MMS: 0x%x, VCM: 0x%x, DCM: 0x%x, NTT: 0x%x\n", 
		SIMODULE, __FUNCTION__, g_table_array[0].rolling_match_counter, g_table_array[1].rolling_match_counter,
		g_table_array[2].rolling_match_counter, g_table_array[3].rolling_match_counter, 
		g_table_array[4].rolling_match_counter);
}

void rmf_OobSiMgr::siWorkerThreadFn()
{
    rmf_Error retCode = RMF_SUCCESS;
    //mpe_Event si_event = 0;
    //mpe_Event pod_event = 0;
    rmf_osal_event_params_t event_params = {0};
    rmf_osal_event_handle_t si_event_handle;
    rmf_osal_event_handle_t pod_event_handle;
    uint32_t pod_event_type;
    uint32_t si_event_type;
    uint32_t event_data = 0;
    //void *poptional_eventdata = NULL;
    rmf_osal_Bool podReady = false;
    uint8_t name[MAX_EVTQ_NAME_LEN];
	rmf_osal_TimeMillis stat_timer = 0;
	rmf_osal_TimeMillis current_time  = 0;
    //***********************************************************************************************
    //***********************************************************************************************

    /*
     * Seed for the random poll interval time.
     */
    srand((unsigned) time(NULL));

    /*
     *  Get the status timer started.
     */
    rmf_osal_timeGetMillis(&g_sitp_sys_time);
    RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI","<%s: %s> - Start time = %"PRIu64"\n", SIMODULE, __FUNCTION__, g_sitp_sys_time);

    //***********************************************************************************************
    //**** Create a global event queue for the Thread
    //***********************************************************************************************
    //if ((retCode = rmf_osal_eventQueueNew(&g_sitp_si_queue, "SitpSi"))
    //              != RMF_SUCCESSi)
    snprintf( (char*)name, MAX_EVTQ_NAME_LEN, "SitpSi");
    if(RMF_INBSI_SUCCESS != rmf_osal_eventqueue_create (name,
                &g_sitp_si_queue))
    {
        RDK_LOG(
                RDK_LOG_ERROR,
                "LOG.RDK.SI",
                "<%s: %s> - unable to create event queue,... terminating thread.\n",
                SIMODULE, __FUNCTION__);

        return;
    }

    /*
     ** *************************************************************************
     ** **** Create and set the FSM structure.
     ** *************************************************************************
     */
    if (RMF_SUCCESS != createAndSetTableDataArray(&g_table_array))
    {
        RDK_LOG(
                RDK_LOG_ERROR,
                "LOG.RDK.SI",
                "<%s: %s> - ERROR: Unable to create table data array, ...terminating thread.\n",
                SIMODULE, __FUNCTION__);
        return;
    }

    /*
     ** *************************************************************************
     ** **** Set the current tuner to the OOB tuner( tuner index 0 ) at startup,
     ** **** the global index(0), the initial event(OOB_SI_ACQUISITION) to move the
     ** **** OOB tuner state to start acquiring the OOB PAT and PMT.
     ** *************************************************************************
     */
    si_event_type = OOB_START_UP_EVENT;

#ifdef TEST_WITH_PODMGR
    pod_event_type = RMF_PODSRV_EVENT_NO_EVENT;
#endif

    g_sitp_si_timeout = g_sitp_si_status_update_time_interval; // set default timeout period to wait forever
    g_table = &g_table_array[0];

#ifdef TEST_WITH_PODMGR
    /*
     * Do not start SI acquisition until the POD is considered READY
     */
    RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
            "<%s: %s> - checking rmf_podmgrIsReady\n",
            SIMODULE, __FUNCTION__);

    //if ((retCode = rmf_osal_eventQueueNew(&g_sitp_pod_queue, "SitpPOD"))
    //              != RMF_SUCCESS)
    snprintf( (char*)name, MAX_EVTQ_NAME_LEN, "SitpPOD");
    if(RMF_SUCCESS != rmf_osal_eventqueue_create (name,
                &g_sitp_pod_queue))
    {
        RDK_LOG(
                RDK_LOG_ERROR,
                "LOG.RDK.SI",
                "<%s: %s> - unable to create POD event queue,... terminating thread.\n",
                SIMODULE, __FUNCTION__);

        return;
    }
    //podImplRegisterMPELevelQueueForPodEvents(g_sitp_pod_queue);
    while( 1 )
    {
	if (RMF_SUCCESS != podmgrRegisterEvents(g_sitp_pod_queue) )
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI"," POD is not ready for registration" );
		sleep( 2 );
		continue;
	}
	break;
    }    
	
    if (RMF_SUCCESS == podmgrIsReady())
    {
        RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                "<%s: %s> - rmf_podmgrIsReady == POD READY\n",
                SIMODULE, __FUNCTION__);
        podReady = TRUE;
    }
    else
    {
        /* Wait until the POD is ready.
         *
         * There are a lot of pod events that are not ready events.  Ignore them and wait for ready event
         */
        podReady = FALSE;

        while(!podmgrIsReady())
        {
            RDK_LOG(
                    RDK_LOG_INFO,
                    "LOG.RDK.SI",
                    "<%s: %s> - rmf_podmgrIsReady != POD READY. Waiting for RMF_PODSRV_EVENT_READY...\n",
                    SIMODULE, __FUNCTION__);

            //mpeos_eventQueueWaitNext(g_sitp_pod_queue, &pod_event,
            //      &poptional_eventdata, NULL, NULL,
            //      g_sitp_pod_status_update_time_interval);
            //SS - used rmf_osal_eventqueue_get_next_event as g_sitp_pod_status_update_time_interval=0 
            // which points to indefinite wait as per definition ofmpeos_eventQueueWaitNext in OCAP RI
            if(RMF_SUCCESS != rmf_osal_eventqueue_get_next_event (g_sitp_pod_queue, &pod_event_handle, NULL, &pod_event_type, &event_params))
            {
                RDK_LOG(
                        RDK_LOG_INFO,
                        "LOG.RDK.SI",
                        "<%s: %s> - rmf_podmgrIsReady != POD READY. event get failed...\n",
                        SIMODULE, __FUNCTION__);
            }
            else
            {
                RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                        "<%s: %s> - recv'd podEvent = %d\n",
                        SIMODULE, __FUNCTION__, pod_event_type);

                if (pod_event_type == RMF_PODSRV_EVENT_READY)
                {
                    podReady = TRUE;
                }
                rmf_osal_event_delete(pod_event_handle);
            }
        }
        RDK_LOG(
                RDK_LOG_INFO,
                "LOG.RDK.SI",
                "<%s: %s> - Received RMF_PODSRV_EVENT_READY. Continuing...\n",
                SIMODULE, __FUNCTION__);
        //podImplUnregisterMPELevelQueueForPodEvents(g_sitp_pod_queue);
        podmgrUnRegisterEvents(g_sitp_pod_queue);
        //rmf_osal_eventQueueDelete(g_sitp_pod_queue);
        rmf_osal_eventqueue_delete(g_sitp_pod_queue);
        g_sitp_pod_queue = 0;
    }
#endif

    // If caching is enabled load the cache first
    if (m_pSiCache->is_si_cache_enabled())
    {
        RDK_LOG(
                RDK_LOG_DEBUG,
                "LOG.RDK.SI",
                "<%s: %s> - Caching enabled, so loading cache...\n",
                SIMODULE, __FUNCTION__);
        m_pSiCache->LockForWrite();
        m_pSiCache->LoadSiCache();
        send_si_event(si_state_to_event(m_pSiCache->GetGlobalState()),0,0);

        if(m_pSiCache->GetGlobalState() == SI_FULLY_ACQUIRED)
            sitp_si_cache_enabled();
        m_pSiCache->ReleaseWriteLock();
    }

    RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI", "CACHE: Set SI Wait time to [cds=%d][dcm=%d][vcm=%d][ntt=%d]\r\n", g_sitp_si_max_nit_cds_wait_time, g_sitp_si_max_svct_dcm_wait_time, g_sitp_si_max_svct_vcm_wait_time,g_sitp_si_max_ntt_wait_time);
    // Register the SI queue to receive POD events
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI", "<%s: %s> - DISABLING2 \n", SIMODULE, __FUNCTION__);
    //podImplRegisterMPELevelQueueForPodEvents(g_sitp_si_queue);
    //RDK_LOG(
    //        RDK_LOG_INFO,
    //        "LOG.RDK.SI",
    //        "<%s::sitp_siWorkerThread> - Sleeping 30 sec...\n",
    //        SIMODULE, __FUNCTION__);
    //mpeos_threadSleep(30000, 0);
    //***********************************************************************************************
    //*** Loop indefinitely until function/method  sipShutdown is invoked by a higher authority.
    //*** The invocation will set g_shutDown to "TRUE" and place a "MPE_ETHREADDEATH" event in the
    //*** thread's event queue.
    //***********************************************************************************************

    /*
     * Only block on the STT if we are parsing STTs and CACHE was not loaded
     */
    //if (TRUE == m_pSiCache->getParseSTT() && m_pSiCache->is_si_cache_enabled() == FALSE)
    /* Always wait for STT and synchronize system time before trapping other tables.*/
    //if (TRUE == m_pSiCache->getParseSTT() )
    {
        /*
         *  Wait until the First STT arrives before acquiring the remaining OOB Tables.
         */
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI", "<%s: %s> - before blocking on stt_condition to wait for STT arrival\n", SIMODULE, __FUNCTION__);
        rmf_osal_condGet(g_sitp_stt_condition);
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI", "<%s: %s> - after blocking on stt_condition to wait for STT arrival\n", SIMODULE, __FUNCTION__);

    }
	
	/* Clear any events that may have been generated so far. Since no section filters are active right now, 
	 * there is nothing to process. This will also flush the bogus OOB_VCTID_CHANGE_EVENT event that POD 
	 * manager generates when it comes up. */
	rmf_osal_eventqueue_clear(g_sitp_si_queue);

    while (FALSE == g_shutDown)
    {
        //event_data = (uint32_t) poptional_eventdata;
        event_data = (uint32_t) event_params.data_extension;

        /*
         ** ***********************************************************************************************
         ** Get the table associated to the event.
         ** ***********************************************************************************************
         */
        retCode = sitp_si_get_table(&g_table, si_event_type, event_data);
        RDK_LOG(
                RDK_LOG_TRACE4,
                "LOG.RDK.SI",
                "<%s: %s> - Received EVENT: %s while in state: %s table_id: %s, subtype: %d, event_handle: 0x%x.\n",
                SIMODULE, __FUNCTION__, sitp_si_eventToString(si_event_type),
                sitp_si_stateToString(g_table->table_state), sitp_si_tableToString(g_table->table_id),
                g_table->subtype, si_event_handle);

        if (RMF_SUCCESS == retCode)
        {
#if 0
            /*
             ** ***********************************************************************************************
             **  Index into the state function pointer array to the index
             **  of the current state and execute the handler.
             ** ***********************************************************************************************
             */
            retCode = g_si_state_table[g_table->table_state](si_event_type,
                    event_data);
#endif
			g_table->rolling_match_counter++;
			rmf_osal_timeGetMillis(&current_time);
			if(FILTER_STATS_DUMP_INTERVAL < (current_time - stat_timer))
			{
				//Four minutes since last data dump. Lets dump hit count again.
				printMatchStatistics();
				stat_timer = current_time;

			}

            switch (g_table->table_state)
            {
                case RMF_SI_STATE_WAIT_TABLE_COMPLETE:
                    retCode = sitp_si_state_wait_table_complete_handler(si_event_type,
                                                event_data);
                    break;

                case RMF_SI_STATE_WAIT_TABLE_REVISION:
                    retCode = sitp_si_state_wait_table_revision_handler(si_event_type,
                                                        event_data);
                    break;

                default:
                    break;
            }



            if (RMF_SUCCESS != retCode)
            {
                /*
                 ** ********************************************************************************************
                 ** There is no way of handling this error, other than printing
                 ** a error msg.
                 ** ********************************************************************************************
                 */
                RDK_LOG(
                        RDK_LOG_WARN,
                        "LOG.RDK.SI",
                        "<%s: %s> - handler failed for event: %d, state: %d\n",
                        SIMODULE, __FUNCTION__, si_event_type, g_table->table_state);
            }
            dumpTableData(g_table);
        }
        else
        {
            /*
             ** ***********************************************************************************************
             ** There is no way of handling this error, other than printing
             ** a error msg and going on to get the next event.
             ** ***********************************************************************************************
             */
            RDK_LOG(
                    RDK_LOG_TRACE4,
                    "LOG.RDK.SI",
                    "<%s: %s> - sitp_si_get_table did not get an table for event: %s, state: %s\n",
                    SIMODULE, __FUNCTION__, sitp_si_eventToString(si_event_type),
                    sitp_si_stateToString(g_table->table_state));
        }

	if((si_event_type != OOB_START_UP_EVENT) &&
			(si_event_type != RMF_OSAL_ETIMEOUT))
	{
		rmf_osal_event_delete(si_event_handle);
	}


#ifdef OOBSI_SUPPORT
        if(g_siOobFilterEnable == TRUE)
        {
		RDK_LOG(
				RDK_LOG_DEBUG,
				"LOG.RDK.SI",
				"<%s: %s : %d> -  calling checkForTableAcquisitionTimeouts()\n",
				SIMODULE, __FUNCTION__, __LINE__);

		// Check if table timers have expired
		// Each table records an initial time when the table filters are set and
		// periodically compares if max time (set via ini) has elapsed before
		// moving on to the next table.
		// Mainly to accommodate speedy acquisition of multi-section SVCT and NTT
		// tables.
		checkForTableAcquisitionTimeouts();

		RDK_LOG(
				RDK_LOG_TRACE4,
				"LOG.RDK.SI",
				"<%s: %s> - Before calculation. g_sitp_si_timeout = 0x%x(%d)), g_sitp_si_update_poll_interval = 0x%x(%d)), getTablesAcquired()=0x%x, TABLES_ACQUIRED=0x%x\n",
				SIMODULE, __FUNCTION__, g_sitp_si_timeout, g_sitp_si_timeout, g_sitp_si_update_poll_interval, g_sitp_si_update_poll_interval, getTablesAcquired(), TABLES_ACQUIRED);

		if (g_sitp_si_timeout == g_sitp_si_update_poll_interval
				&& TABLES_ACQUIRED <= getTablesAcquired())
		{
			//Assert:  SI thread is polling for changes and it has already found all of the required tables for profile 2
			//so calculate the timeout based on a random time between the max and min poll interval.
			g_sitp_si_timeout = (g_sitp_si_update_poll_interval_max
					- (((uint32_t) rand())
						% (g_sitp_si_update_poll_interval_max
							- g_sitp_si_update_poll_interval_min)));
		}
	}
#endif

        RDK_LOG(
                RDK_LOG_TRACE4,
                "LOG.RDK.SI",
                "<%s: %s> - Sleeping (mpeos_eventQueueWaitNext with g_sitp_si_timeout = 0x%x(%d)).\n",
                SIMODULE, __FUNCTION__, g_sitp_si_timeout, g_sitp_si_timeout);

        //retCode = mpeos_eventQueueWaitNext(g_sitp_si_queue, &si_event,
        //      &poptional_eventdata, NULL, NULL, g_sitp_si_timeout);
        retCode = rmf_osal_eventqueue_get_next_event_timed (g_sitp_si_queue, &si_event_handle, NULL, &si_event_type, &event_params, (g_sitp_si_timeout*1000));
        if (RMF_SUCCESS != retCode)
        {
            si_event_type = RMF_OSAL_ETIMEOUT;
        }
#ifdef TEST_WITH_PODMGR
        // This event would have been preceded by a RMF_PODSRV_EVENT_RESET_PENDING
        else if(RMF_PODSRV_EVENT_READY == si_event_type)
        {
            RDK_LOG(RDK_LOG_INFO,
                    "LOG.RDK.SI",
                    "<%s: %s> - Received RMF_PODSRV_EVENT_READY...\n",
                    SIMODULE, __FUNCTION__);
            si_event_type = OOB_START_UP_EVENT;
            rmf_osal_event_delete(si_event_handle);
        }
#endif
        RDK_LOG(RDK_LOG_TRACE5, "LOG.RDK.SI",
                "<%s: %s> - \n\n\n", SIMODULE, __FUNCTION__);

	if((TRUE == g_shutDown) && (si_event_type != OOB_START_UP_EVENT) &&
			(si_event_type != RMF_OSAL_ETIMEOUT))
	{
		rmf_osal_event_delete(si_event_handle);
	}
    } /* End while( FALSE == g_shutDown ) */

    /*
     * Cleanup what SITP SI has created.
     */
    si_Shutdown();

    return;
}

// if USE_NTPCLIENT
rmf_Error rmf_OobSiMgr::monitorAndSinkTimeWithNTP(const char *hostname, int sleepFor )
{
    rmf_Error retCode = RMF_SUCCESS;

    int maxlen = 1024; /* maximum message length */
    int i, s /*socket*/;
    unsigned char msg[48] = {010,0,0,0,0,0,0,0,0}; /* buffer for the socket request */
    unsigned long buf [maxlen]; /* buffer for the reply */
    bool sttStatus = false;
    struct timeval tv;
    unsigned long tmit; /* the time -- This is similar to time_t */
    struct addrinfo hints;
    struct addrinfo *result = NULL;
    const char *node = NULL;



    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI","<%s: %s> - %d\n", SIMODULE, __FUNCTION__, __LINE__);
    //We are trying to implement the following logic \
    We check if any successful connection is already made.If yes \
    Skip the IP aquiring steps, and reuse the IP and send data to NTP server and \
    receive time information. We will retain IP and IPversion ie IPv4 or IPv6 \
    If no successful connection is made already, convert the domain name to any \
    valid ccp ntp server IP , create a socket and connect and get time information. \
    If the full connection worked fine, mark it as a good IP for future use. Else start afresh

    if ( false == isIPObtainedGood )
    {
        if ( NULL == hostname )
        {
            RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SI","%s : Host name is NULL\n", __FUNCTION__);
            retCode = RMF_OSAL_EINVAL;
            return retCode;
        }
        node = hostname;
    }
    else
    {
        // use the saved ip address to get the address info.
        node = &ipAddr[0];
    }

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_DGRAM; /* any socket type */
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_UDP;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    s = getaddrinfo(node, "123", &hints, &result);
    if (s != 0 || result == NULL || result->ai_addr == NULL)
    {
        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SI","%s : getaddrinfo failed %d\n", __FUNCTION__, s);
        retCode = RMF_OSAL_EINVAL;
        if (result != NULL)
        {
            freeaddrinfo(result);
        }
        return retCode;
    }

    s = -1;
    for (struct addrinfo *rp = result; rp != NULL; rp = rp->ai_next)
    {
        // Convert to IP address for display
        memset ( ipAddr , 0 , INET6_ADDRSTRLEN );
        void *addr;
        if (rp->ai_addr->sa_family == AF_INET)
        {
            addr = (void *)&(((struct sockaddr_in *)(rp->ai_addr))->sin_addr);
        }
        else
        {
            addr = (void *)&(((struct sockaddr_in6 *)rp->ai_addr)->sin6_addr);
        }
        if ( NULL == inet_ntop( rp->ai_addr->sa_family, addr, ipAddr , INET6_ADDRSTRLEN ) )
        {
            int err = errno;
            RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SI","%s : inet_ntop failed with error %d\n", __FUNCTION__ , err);
            retCode = RMF_OSAL_EINVAL;
            continue;
        }

        if( -1 != s)
        {
            close (s);
        }

        RDK_LOG( RDK_LOG_WARN, "LOG.RDK.SI","%s : Trying to connect to %s at %s\n", __FUNCTION__, hostname , ipAddr);

        s = socket(rp->ai_family, SOCK_DGRAM, IPPROTO_UDP); /* open a UDP socket */
        if( -1 == s)
        {
            RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SI","%s : socket create failed, returning...\n", __FUNCTION__);
            retCode = RMF_OSAL_EINVAL;
            continue;
        }

        tv.tv_sec = 10; /* time out seconds */
        tv.tv_usec = 0;
        if(setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
        {
            RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SI","%s : Can not set the recv timeout...\n", __FUNCTION__);
            retCode = RMF_OSAL_EINVAL;
            continue;
        }

        RDK_LOG( RDK_LOG_DEBUG,"LOG.RDK.SI","%s : Sending to %s\n", __FUNCTION__, ipAddr);

        i = sendto (s, msg, sizeof(msg), 0, (struct sockaddr *)rp->ai_addr, rp->ai_addrlen);
        if ( i < 0 )
        {
            int err = errno;
            RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SI","%s : sendto ntp server %s at %s failed with %d, returning...\n",
                 __FUNCTION__, hostname, ipAddr , err );
            if ( true == isNTPErrorToBePrinted )
            {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SI","NTP Server Connection Lost\n");
                   //We have logged a fail. Now logging is not needed until a fail after the next success
                isNTPErrorToBePrinted = false;
            }
            retCode = RMF_OSAL_EINVAL;
            continue;
        }

        // get the data back
        struct sockaddr saddr;
        socklen_t saddr_l = sizeof (saddr);

        RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.SI","%s : Waiting to get reply from  %s\n", __FUNCTION__, ipAddr);
        // here we wait for the reply and fill it into our buffer
        i = recvfrom (s, buf, 48, 0, &saddr, &saddr_l);
        if ( i < 0 )
        {
            int err = errno;
            RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SI","%s : recvfrom ntp server %s at %s failed with %d, returning...\n", \
                __FUNCTION__, hostname, ipAddr , err);
            if ( true == isNTPErrorToBePrinted )
            {
                RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SI","NTP Server Connection Lost\n");
                //We have logged a fail. Now logging is not needed until a fail after the next success
                isNTPErrorToBePrinted = false;
            }
            retCode = RMF_OSAL_EINVAL;
            continue;
        }

        //Since NTP connection is successful, we need to print a connection lose in next fail
        isNTPErrorToBePrinted = true;
        //We completed a successful connection to NTP server. Mark this server as a good server
        isIPObtainedGood = true;
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI","<%s: %s> - %d\n", SIMODULE, __FUNCTION__, __LINE__);

        tmit = ntohl((time_t)buf[10]); //# get transmit time
        tmit -= 2208988800U;

        RDK_LOG( RDK_LOG_DEBUG,"LOG.RDK.SI", "%s : NTP time is updating\n", __FUNCTION__);
        // REVIEW : Have to touch /tmp/stt_received file?
        rmf_osal_TimeMillis currTimeInMillis = 0;
        rmf_osal_TimeMillis currTimeInSecs = 0;
        int ret = 0;
        struct timeval tv;
        struct timezone tz;
        ret = gettimeofday(&tv,&tz);
        if (0 == ret)
        {
            currTimeInSecs = tv.tv_sec;
        }
        currTimeInMillis = currTimeInSecs*1000;
        currTimeInMillis += (tv.tv_usec / 1000);    
        //Since NTP connection is successful, we should mark STT is received
        m_pSiCache->LockForRead();
        sttStatus = m_pSiCache->getSTTAcquiredStatus();
        m_pSiCache->UnLock();
        if ( false == sttStatus )
        {
            m_pSiCache->LockForWrite();
            m_pSiCache->setSTTAcquiredStatus( TRUE );
            m_pSiCache->ReleaseWriteLock();
            retCode = m_pOobSiSttMgr->update_systime(tmit, true);
        }
        m_pOobSiSttMgr->PostSTTArrivedEvent( SI_STT_ACQUIRED , tmit , currTimeInSecs , currTimeInMillis );
        retCode = RMF_SUCCESS;
        break;
    }

    if( -1 != s)
    {
        close (s);
    }
    if (result != NULL)
    {
        freeaddrinfo(result);
    }

    return retCode;
}

// Get the query string value.
char* rmf_OobSiMgr::get_response_val(const char* _mUrlStr, const char* _q_str, char *_value_str)
{
	string mUrlStr(_mUrlStr);
	string q_str(_q_str);
	string value_str;
    size_t start_pos;
    size_t end_pos;

    //query strings are of format "query_str=value&"
    //q_str += "=";

    start_pos = mUrlStr.find(q_str);

    if (start_pos == string::npos)
    {
        //value_str.clear();
        return NULL;
    }

    start_pos += q_str.size();
	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI","<%s: %s> start_pos=%d\n", SIMODULE, __FUNCTION__, start_pos);

    end_pos = mUrlStr.find("\r\n", start_pos);
	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI","<%s: %s> end_pos=%d\n", SIMODULE, __FUNCTION__, end_pos);

    // If not found, this must be the last query item
    if (end_pos == string::npos)
    {
        // Point to end of string
        end_pos = mUrlStr.size();
		RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI","<%s: %s> NOT FOUND end_pos=%d\n", SIMODULE, __FUNCTION__, end_pos);
    }

    value_str = mUrlStr.substr (start_pos, end_pos - start_pos);
	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI","<%s: %s> start_pos=%d; end_pos=%d\n", SIMODULE, __FUNCTION__, start_pos, end_pos);

    strncpy(_value_str, value_str.c_str(), sizeof(_value_str)-1);
	_value_str[sizeof(_value_str)-1] = '\0';
    return _value_str;
}

// ipdu-ntp
void rmf_OobSiMgr::ntpClientSleep(int32_t sleepFor)
{
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "%s: sleepFor=%d \n", __FUNCTION__, sleepFor);
    struct stat ipdu_mode_file;
    int sleepPollInterval = 2;
    int remainingSleep = sleepFor;

    while(remainingSleep > 0 && !inIpDirectMode)
    {
        inIpDirectMode = ((stat("/tmp/inIpDirectUnicastMode", &ipdu_mode_file) == 0));
        if (inIpDirectMode)
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.POD", "%s: IP Direct Unicast mode detected, terminate sleep. \n", __FUNCTION__, inIpDirectMode);
            remainingSleep = 0;
            break;
        }
   	    sleep (sleepPollInterval);
        remainingSleep -= sleepPollInterval; 
    }
}

char* rmf_OobSiMgr::getDataFromResponse(const char* _mUrlStr, const char* _q_str, char *cvalue_str)
{
    size_t start_pos;
    size_t end_pos;
	string mUrlStr(_mUrlStr);
	string q_str(_q_str);
    string value_str;

    //query strings are of format "query_str=value&"
    //q_str += "\r\n";

    start_pos = mUrlStr.find(q_str);

    if (start_pos == string::npos)
    {
        //value_str.clear();
        return NULL;
    }

    start_pos += q_str.size();

    end_pos = mUrlStr.size();

    value_str = mUrlStr.substr (start_pos, end_pos - start_pos);
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI","<%s : %s> url = '%s'\n", SIMODULE, __FUNCTION__, value_str.c_str());
    //value_str += "\r\n";
    strncpy(cvalue_str, value_str.c_str(), BUF_LEN-1);
	cvalue_str[strlen(cvalue_str)] = '\0';
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI","<%s : %s> url==== = '%s'\n", SIMODULE, __FUNCTION__, cvalue_str);

    return cvalue_str;
}

bool rmf_OobSiMgr::getNTPUrl(bool ntpURLFound, char *ntpserver, size_t bufLen)
{
    RFC_ParamData_t FAILOVER_SERVER;
    memset(&FAILOVER_SERVER, 0, sizeof(FAILOVER_SERVER));
    WDMP_STATUS wdmpStatus = getRFCParameter("NTP", "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.NTP.failoverServer", &FAILOVER_SERVER);
    if (wdmpStatus != WDMP_SUCCESS)
       RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI","<%s: %s> RFC returned empty value for NTP.failoverServer\n",SIMODULE, __FUNCTION__);

    if(bufLen < 1 || !ntpserver)
    {
        return false;
    }
    memset(ntpserver, '\0', bufLen);
    if (ntpURLFound)
    {
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI","<%s: %s> ntpURLFound=%d.\n",SIMODULE, __FUNCTION__, ntpURLFound);
        const char *t_ntpserver = FAILOVER_SERVER.value;
        if (t_ntpserver != NULL)
        {
            strncpy(ntpserver, t_ntpserver, bufLen-1);
        }
        else
        {
        	const char* t_ntpserver = "108.61.73.244";	
        	strncpy(ntpserver, t_ntpserver, bufLen-1);
        }
        ntpURLFound = false;
    }
    else
    {
        int retCode = 0;
        RFC_ParamData_t rfcParam;
        WDMP_STATUS wdmpStatus = getRFCParameter("MediaFramework", "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.BootstrapConfig.Enable", &rfcParam);
        if ((wdmpStatus == WDMP_SUCCESS || wdmpStatus == WDMP_ERR_DEFAULT_VALUE) && 0 == strcasecmp(rfcParam.value, "TRUE"))
        {
            wdmpStatus = getRFCParameter("MediaFramework", "Device.Time.NTPServer1", &rfcParam);
            if (wdmpStatus == WDMP_SUCCESS || wdmpStatus == WDMP_ERR_DEFAULT_VALUE)
            {
                if (strlen(rfcParam.value) > 0)
                {
                    strncpy(ntpserver, rfcParam.value, bufLen-1);
                }
                else
                {
                    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI","<%s: %s> RFC returned empty value for ntpHost\n",SIMODULE, __FUNCTION__);
                    retCode = -1;
                }
            }
            else
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI","<%s: %s> RFC retun for ntpHost=%s.\n",SIMODULE, __FUNCTION__, getRFCErrorString(wdmpStatus));
                retCode = -1;
            }
        }
        else
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI","<%s: %s> ntpURLFound=%d.\n",SIMODULE, __FUNCTION__, ntpURLFound);
            char recv_data_buffer[BUF_LEN] = {'\0'};
            char buf[sizeof(NTP_POST_FIELD)] = {'\0'};
            char content_val[BUF_LEN] = {'\0'};
            int sockfd = -1;
            int contentLen = 0;
            ssize_t bytes_received = -1;

            struct sockaddr_in client;
            struct hostent *host = gethostbyname(HOST_NAME);

            string field = "ntpHost\r\n";
            string request;
            string recvdBuf;

            if ( (host == NULL) || (host->h_addr == NULL) ) 
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI","<%s: %s> Error retrieving DNS information.\n",SIMODULE, __FUNCTION__);
                retCode = -1;
                goto error;
            }

            bzero(&client, sizeof(client));

            client.sin_family = AF_INET;
            client.sin_port = htons( atoi(HOST_PORT) );

            rmf_osal_memcpy(&client.sin_addr, host->h_addr, host->h_length, sizeof(client.sin_addr), host->h_length);
	    
            /* create socket */
            sockfd = socket(AF_INET, SOCK_STREAM, 0);

            if (sockfd < 0) 
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI","<%s: %s> Error creating socket.\n",SIMODULE, __FUNCTION__);
                retCode = -1;
                goto error;
            }

            /* Connet to the Host specified */
            if ( connect(sockfd, (struct sockaddr *)&client, sizeof(client)) < 0 ) {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI","<%s: %s> Error Could not connect.\n",SIMODULE, __FUNCTION__);
                retCode = -1;
                goto error;
            }

            request = "POST ";
            request += NTP_AUTHSERVICE_POST_URL;
            request += " HTTP/1.1\r\n";
            request += "Host: ";
            request += HOST_NAME;
            request += ":";
            request += HOST_PORT;
            request += "\r\n";
            request += "Accept: */*\r\n";
            request += "Content-Length: ";
            snprintf(buf, sizeof(NTP_POST_FIELD), "%d", strlen(NTP_POST_FIELD));
            request += buf;
            request += "\r\n";
            request += "Content-Type: application/x-www-form-urlencoded\r\n";
            request += "\r\n";

            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI","<%s: %s> Sending request %s.\n",SIMODULE, __FUNCTION__, request.c_str());

            if (send(sockfd, request.c_str(), request.length(), 0) != (int)request.length()) {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI","<%s: %s> Error sending request.\n",SIMODULE, __FUNCTION__);
                retCode = -1;
                goto error;
            }
            //cout << field << endl;
            if (send(sockfd, field.c_str(), field.length(), 0) != (int)field.length()) {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI","<%s: %s> Error sending post parameter.\n",SIMODULE, __FUNCTION__);
                retCode = -1;
                goto error;
            }

            bytes_received = recv(sockfd, recv_data_buffer,sizeof(recv_data_buffer)-1, 0);

            // If no data arrives, the program will just wait here until some data arrives.
            if (bytes_received == 0) 
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI","<%s: %s> Error host shut down.\n",SIMODULE, __FUNCTION__);
                retCode = -1;
                goto error;
            }
            if (bytes_received == -1)
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI","<%s: %s> Error receive error, check http response.\n",SIMODULE, __FUNCTION__);
                retCode = -1;
                goto error;
            }

            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI","<%s: %s> bytes received %d.\n",SIMODULE, __FUNCTION__, bytes_received);

            recv_data_buffer[bytes_received] = '\0';

            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI","<%s: %s> message received %s.\n",SIMODULE, __FUNCTION__, recv_data_buffer);

            //std::cout << recv_data_buffer << std::endl;

            /* form the string from received buffer */
            recvdBuf = recv_data_buffer;

            ssize_t start_pos;
            start_pos = recvdBuf.find("200 OK");
            if (start_pos == string::npos)
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI","<%s: %s> Error http post has some issue.\n",SIMODULE, __FUNCTION__);
                retCode = -1;
                goto error;
            }

            if (NULL != get_response_val(recvdBuf.c_str(), "Content-Length: ", content_val))
            {
                contentLen = atoi(content_val);
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI","<%s: %s> Content-Length=%d.\n",SIMODULE, __FUNCTION__, contentLen);
            }
            else
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI","<%s: %s> Error http post did not send any payload length.\n",SIMODULE, __FUNCTION__);
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI","<%s: %s> Trying with the default NTP.\n",SIMODULE, __FUNCTION__);
                //ntpserver = "108.61.73.244";	
                retCode = -1;
            }

            if(NULL != getDataFromResponse(recvdBuf.c_str(), "\r\n\r\n", ntpserver))
            {
                RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI","<%s: %s> Content=%s.\n",SIMODULE, __FUNCTION__, ntpserver);
            }
            else
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI","<%s: %s> Error http post did not send any payload.\n",SIMODULE, __FUNCTION__);
                RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI","<%s: %s> Trying with the default NTP.\n",SIMODULE, __FUNCTION__);
                //ntpserver = "108.61.73.244";	
                retCode = -1;
            }
            error:    
                if (sockfd != -1) 
                    close(sockfd);
        }

        if(retCode < 0) 
        {
            const char* t_ntpserver = FAILOVER_SERVER.value;
            strncpy(ntpserver, t_ntpserver, BUF_LEN-1);
            if (t_ntpserver == NULL)
            {
                const char* t_ntpserver = "108.61.73.244";	
                strncpy(ntpserver, t_ntpserver, BUF_LEN-1);
            }
            ntpURLFound = false;
        }
        else
        {
            ntpURLFound = true;
        }
        //return retCode;
    }
    ntpserver[strlen(ntpserver)] = '\0';
    RDK_LOG( RDK_LOG_DEBUG,"LOG.RDK.SI","%s:ntp hostname is %s\n",__FUNCTION__, ntpserver);
    return ntpURLFound;
}

#ifdef TEST_WITH_PODMGR
bool checkIpAcquiredFlag()
{
	bool status = false;
	FILE *fp = fopen("/tmp/ip_acquired", "rb");
	if (fp)
	{
		status = true;
		fclose(fp);
	}

	return status;
}

bool rmf_OobSiMgr::waitForDSGIPAcquiredEvent(void)
{
	bool retValue = false;
	rmf_osal_eventqueue_handle_t pEvent_queue;
	rmf_osal_event_handle_t event_handle = 0;
	rmf_osal_event_params_t event_params = {0};
	rmf_osal_event_category_t event_category;
	uint32_t event_type;
	rmf_Error ret = RMF_SUCCESS;
	time_t startTime,currentTime;
	int32_t timeout_usec = 1000 * 1000 * 60;
	time(&startTime);

	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI", " %s: \n", __FUNCTION__);
	ret  = rmf_osal_eventqueue_create((const uint8_t*) "siNTP", &pEvent_queue);
	if (pEvent_queue != NULL)
	{
		rmf_osal_eventmanager_handle_t pod_client_event_manager = get_pod_event_manager();
		if (pod_client_event_manager == NULL)
		{
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI","%s:get_pod_event_manager() returned NULL\n", __FUNCTION__);
		}
		else
		{
			ret = rmf_osal_eventmanager_register_handler(pod_client_event_manager, pEvent_queue,RMF_OSAL_EVENT_CATEGORY_CARD_MANAGER);
			if (RMF_SUCCESS != ret)
			{
				RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "%s:failed rmf_osal_eventmanager_register_handler \n",__FUNCTION__);
			}
			else
			{
				while (1)
				{
					if (checkIpAcquiredFlag())
					{
						RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI","%s:CardMgr_DSG_IP_ACQUIRED flagged\n",__FUNCTION__);
						retValue = true;
						break;
					}
					ret = rmf_osal_eventqueue_get_next_event_timed(pEvent_queue,
							&event_handle, &event_category, &event_type,&event_params,timeout_usec);

					rmf_osal_event_delete(event_handle);

					if ( RMF_SUCCESS != ret)
					{
						RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI","%s:Invalid Event \n", __FUNCTION__);
						continue;
					}
					else
					{
						time(&currentTime);
						RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI","%s: event_category:%x event_type:%x \n",
								__FUNCTION__, event_category, event_type);
						if (event_category == RMF_OSAL_EVENT_CATEGORY_CARD_MANAGER && event_type == CardMgr_DSG_IP_ACQUIRED)
						{
							RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI","%s:Received CardMgr_DSG_IP_ACQUIRED  \n",__FUNCTION__);
							retValue = true;
							break;
						}
						else if(currentTime-startTime > 300)//break loop if ip not received in 5 min
						{
							RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI","%s:Error timeout for CardMgr_DSG_IP_ACQUIRED event\n",__FUNCTION__);
							break;
						}
					}
				} //while
				////unregister event
				ret = rmf_osal_eventmanager_unregister_handler(pod_client_event_manager,pEvent_queue);
				if(RMF_SUCCESS != ret)
				{
					RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "%s:failed to rmf_osal_eventmanager_unregister_handler \n",__FUNCTION__);
				}
			}
		}
		//delete event queue
		ret = rmf_osal_eventqueue_delete(pEvent_queue);
		if (RMF_SUCCESS != ret)
		{
			RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "%s:failed to delete event queue \n",__FUNCTION__);
		}
	}
	else
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "%s:failed to create event queue \n",__FUNCTION__);
	}
	return retValue;
}
#endif

//***************************************************************************************************
//  Thread:                                     ntpclientWorkerThread ()
//***************************************************************************************************
/**
 * ntpclientWorkerThreadFn
 * @param None,
 * @return .
 */
void rmf_OobSiMgr::ntpclientWorkerThreadFn()
{
	rmf_Error retCode = RMF_SUCCESS;
	rmf_osal_Bool isSTTAcquired = FALSE;
	int sleepFor = 5; 
	int configSleepFor = 30;
	int syncsleepFor = 60; //NTP sync wait for seconds
	char ntpserver[BUF_LEN] = {'\0'};
	bool ntpURLFound = false;

	const char *ntpThreadSleepTime = rmf_osal_envGet("NTP.FAILOVER.STT.WAIT.TIME");
	if (ntpThreadSleepTime != NULL)
	{
		configSleepFor = atoi(ntpThreadSleepTime);
	}

	RDK_LOG( RDK_LOG_INFO,"LOG.RDK.SI","%s:NTP failover wait time = %d seconds\n",__FUNCTION__, configSleepFor);

	const char *ntpsyncThreadSleepTime = rmf_osal_envGet("NTP.FAILOVER.SYNC.WAIT.TIME");
	if(ntpsyncThreadSleepTime != NULL)
	{
		syncsleepFor = atoi(ntpsyncThreadSleepTime);
	}
	else
	{
		syncsleepFor = 60; 
	}

	ntpURLFound = getNTPUrl(ntpURLFound, ntpserver, sizeof(ntpserver));

	RDK_LOG( RDK_LOG_INFO,"LOG.RDK.SI","%s:ntp hostname is %s\n",__FUNCTION__, ntpserver);
	//Wait for DSGIPAcquired Event
	RDK_LOG( RDK_LOG_INFO,"LOG.RDK.SI","%s: calling waitForDSGIPAcquiredEvent\n",__FUNCTION__);
#ifdef TEST_WITH_PODMGR
	bool returnVal = waitForDSGIPAcquiredEvent();
	RDK_LOG( RDK_LOG_INFO,"LOG.RDK.SI","%s: waitForDSGIPAcquiredEvent ret:%d  Now entering process loop\n",__FUNCTION__,returnVal);
#endif
	//calling ntpClientSleep with 1 sec delay to check if STB is in IPDirect mode or not
	ntpClientSleep(1);
	while (FALSE == g_shutDown) // REVIEW : condition
	{
		// If NOT in IPDU mode must delay at start of while loop to prevent NTP time sync kicking in soon
		if(!inIpDirectMode)               // ipdu-ntp
			sleep(configSleepFor);               // Once in the proces loop we want to force sleep for this thread
#ifdef ENABLE_TIME_UPDATE
		m_pSiCache->LockForRead();
		RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI","<%s: %s> - %d\n", SIMODULE, __FUNCTION__, __LINE__);
		isSTTAcquired = m_pSiCache->IsSttAcquired(); // REVIEW: will this be updated value

		m_pSiCache->UnLock();
#endif
		m_pSiCache->LockForRead();
		isSTTAcquired = m_pSiCache->getSTTAcquiredStatus();
		m_pSiCache->UnLock();
		RDK_LOG( RDK_LOG_INFO,"LOG.RDK.SI","%s:%s\n",__FUNCTION__,(isSTTAcquired) ? "STT is already processed.":"STT not acquired");
		if( TRUE == isSTTAcquired)
		{
			RDK_LOG( RDK_LOG_INFO,"LOG.RDK.SI","%s:%s\n",__FUNCTION__,"STT is acquired.\n");
		}
		else
		{
			RDK_LOG( RDK_LOG_INFO,"LOG.RDK.SI","%s:%s\n",__FUNCTION__,"STT not acquired. FAILOVER to NTP\n");
			//if ( !isNTPused)
			{
				retCode = monitorAndSinkTimeWithNTP(ntpserver, syncsleepFor);
				RDK_LOG( RDK_LOG_DEBUG,"LOG.RDK.SI", "%s : monitorAndSinkTimeWithNTP retCode=%d \n", __FUNCTION__, retCode);
				if (RMF_OSAL_EINVAL == retCode)
				{
					RDK_LOG( RDK_LOG_DEBUG,"LOG.RDK.SI", "%s : monitorAndSinkTimeWithNTP Failed, getting the ntp url \n", __FUNCTION__);
					ntpURLFound = getNTPUrl(ntpURLFound, ntpserver, sizeof(ntpserver));
				}
				//Touching the stt_received file may cause other issues.So disabling \
				//The file will be created when STT is originally received
#define ENABLE_STTRECEIVED_FILETOUCH 1
#if ENABLE_STTRECEIVED_FILETOUCH
				else if (RMF_OSAL_ENODATA != retCode)
				{
					sleepFor = configSleepFor;
					RDK_LOG( RDK_LOG_WARN,"LOG.RDK.SI", "%s : Time updated through NTP, creating /tmp/stt_received \n", __FUNCTION__);
					m_pSiCache->TouchFile("/tmp/stt_received");
					//isNTPused = true;
				}
#endif				
			}
		}
		RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI","<%s: %s> - %d\n", SIMODULE, __FUNCTION__, __LINE__);
		// If in IPDU mode must delay at end of while loop to allow loop to process immediately upon loop entry.
		if(inIpDirectMode)               // ipdu-ntp
			sleep(sleepFor);              // Once in the proces loop we want to force sleep for this thread
	}
	return ;
}
// endif USE_NTPCLIENT

//***************************************************************************************************
//  Thread:                                     sitp_siSTTWorkerThread ()
//***************************************************************************************************
/**
 *  sitp_siSTTWorkerThread entry point (This thread is dedicated to setting the System
 *  Time Table filter and parsing it)
 *
 * @param None,
 *
 * @return MPE_SUCCESS or other error codes.
 */
void rmf_OobSiMgr::siSTTWorkerThreadFn()
{
    rmf_Error retCode = RMF_SUCCESS;
    rmf_osal_event_handle_t si_event_handle = 0;
    uint32_t si_event_type = 0;
    rmf_osal_event_params_t event_params = {0};
    //void *poptional_eventdata = NULL;

    //rmf_FilterSectionHandle section_handle = 0;
    //uint8_t current_section_number = 0;
    //uint8_t last_section_number = 0;
    //uint32_t unique_id = 0;
    //uint32_t crc2 = 0;
    //static uint32_t stt_section_count = 0;
    rmf_osal_Bool fFirstSTTArrived = FALSE;
    uint8_t name[MAX_EVTQ_NAME_LEN];

    RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
            "<%s: %s> - Enter.\n", SIMODULE, __FUNCTION__);

    //***********************************************************************************************
    //***********************************************************************************************

    //***********************************************************************************************
    //**** Create a global event queue for the Thread
    //***********************************************************************************************
    snprintf( (char*)name, MAX_EVTQ_NAME_LEN, "SitpSiSTT");
    if(RMF_INBSI_SUCCESS != rmf_osal_eventqueue_create (name,
                &g_sitp_si_stt_queue))
    {
        RDK_LOG(
                        RDK_LOG_ERROR,
                        "LOG.RDK.SI",
                        "<%s: %s> - unable to create event queue,... terminating thread.\n",
                        SIMODULE, __FUNCTION__);

        return;
    }

    int useCLSTTImpl = 1;
    const char *sitp_si_parse_vlstt = rmf_osal_envGet("FEATURE.STT.SUPPORT");

    if(sitp_si_parse_vlstt != NULL &&  (stricmp(sitp_si_parse_vlstt, "TRUE") == 0))
    {
        m_pOobSiSttMgr->sttRegisterForSTTArrivedEvent(g_sitp_si_stt_queue);
        useCLSTTImpl = 0;
        while (FALSE == g_shutDown)
        {
            int eventData1 = 0;
            //int eventFlag = 0;

            //retCode = rmf_osal_eventqueue_get_next_event_timed (g_sitp_si_stt_queue, 
              //                  &si_event_handle, NULL, &si_event_type, &event_params, (g_sitp_stt_timeout*1000));
            retCode = rmf_osal_eventqueue_get_next_event (g_sitp_si_stt_queue, 
                                &si_event_handle, NULL, &si_event_type, &event_params);

            if (RMF_SUCCESS != retCode)
            {
                        //RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
                        //"<%s: %s> - Time out\n",
                        //SIMODULE, __FUNCTION__);
                    si_event_type = RMF_OSAL_ETIMEOUT;
            }

            switch (si_event_type)
            {
                case SI_STT_ACQUIRED:
                    {

                    //long EpochTimeOfSTTArrivalMillis = eventData1;
                    //long EpochTimeOfSTTArrival = eventFlag;
                    /*
                     * Release the global STT condition if this is the first STT.  This will
                     * unblock the SI Thread and start SI acquisition.
                     */
                     eventData1 = event_params.data_extension;
                    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI",
                                    "<%s: %s> - Received EVENT: %s: %d, data %d\n",
                                    SIMODULE, __FUNCTION__, "STT ARRIVED AND PROCESSED", si_event_type, eventData1);
                     if(FALSE == fFirstSTTArrived)
                     {
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI", "<%s: %s> - UNLOCKING SI THREAD AFTER STT IS ACQUIRED\n", SIMODULE, __FUNCTION__);
                        fFirstSTTArrived = TRUE;
                        rmf_osal_condSet(g_sitp_stt_condition);
                     }
                     m_pSiCache->SetGlobalState(SI_STT_ACQUIRED);
                     send_si_event(si_state_to_event(m_pSiCache->GetGlobalState()),0,0);
                    }
                    break;

#ifdef ENABLE_TIME_UPDATE
	        case RMF_SI_EVENT_STT_TIMEZONE_SET:
		    {
                            rmf_timeinfo_t *pSttTimeInfo = (rmf_timeinfo_t *)event_params.data;
                            rmf_timeinfo_t *pTimeInfo = NULL;
				
			    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS",
					    "<%s: %s> - Received EVENT: %s: %d\n",
					    SIMODULE, __FUNCTION__, "STT TIMEZONE/SYSTIME SET", si_event_type);
			    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS",
					    "<%s: %s> - pData: 0x%x\n",
					    SIMODULE, __FUNCTION__, event_params.data);
			    retCode = rmf_osal_memAllocP(RMF_OSAL_MEM_SI_OOB, sizeof(rmf_timeinfo_t), (void**)&pTimeInfo);
			    if(RMF_SUCCESS == retCode)
                            {
                                pTimeInfo->timezoneinfo.timezoneinhours = pSttTimeInfo->timezoneinfo.timezoneinhours;
                                pTimeInfo->timezoneinfo.daylightflag = pSttTimeInfo->timezoneinfo.daylightflag;
			        send_si_stt_event(si_event_type, (void*)pTimeInfo, event_params.data_extension);				   
                            }
                            rmf_osal_memFreeP(RMF_OSAL_MEM_SI_OOB, pSttTimeInfo);
		    }
	            break;

		case RMF_SI_EVENT_STT_SYSTIME_SET:
		    {
                        rmf_timeinfo_t *pSttTimeInfo = (rmf_timeinfo_t *)event_params.data;
                        rmf_timeinfo_t *pTimeInfo = NULL;

                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS",
                                    "<%s: %s> - Received EVENT: %s: %d\n",
                                    SIMODULE, __FUNCTION__, "STT TIMEZONE/SYSTIME SET", si_event_type);
                        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SYS",
                                    "<%s: %s> - pData: 0x%x\n",
                                    SIMODULE, __FUNCTION__, event_params.data);
			retCode = rmf_osal_memAllocP(RMF_OSAL_MEM_SI_OOB, sizeof(rmf_timeinfo_t), (void**)&pTimeInfo);
			if(RMF_SUCCESS == retCode)
                        {
                            pTimeInfo->timevalue = pSttTimeInfo->timevalue;
                            send_si_stt_event(si_event_type, (void*)pTimeInfo, event_params.data_extension);
                        }
                        rmf_osal_memFreeP(RMF_OSAL_MEM_SI_OOB, pSttTimeInfo);
	 	    }
		    break;
#endif

                default:
                    /*RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                                    "<%s: %s> - Received EVENT: %s: %d\n",
                                    SIMODULE, __FUNCTION__, "UNKNOWN_EVENT", si_event_type);*/
                    break;
            }

            if(si_event_type != RMF_OSAL_ETIMEOUT)
            {
                rmf_osal_event_delete(si_event_handle);
            }
        }
    }

    if(!useCLSTTImpl)
    {
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI",
                    "<%s: %s> -returned because  useCLSTTImpl =%suseCLSTTImpl: %d\n",SIMODULE, __FUNCTION__, "",useCLSTTImpl);

        return;
    }
}

/*
 * create the table data structures and initialize the values.
 *
 *
 */
rmf_Error rmf_OobSiMgr::createAndSetTableDataArray(
        rmf_si_table_t ** handle_table_array)
{
        rmf_Error retCode = RMF_SUCCESS;
        rmf_si_table_t * ptr_table_array;
        uint32_t ii = 0, jj = 0;

        RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                        "<%s: %s> - Enter\n", SIMODULE, __FUNCTION__);

        ptr_table_array = *handle_table_array;

        // 1.NIT_CDS
        // 2.NIT_MMS
        // 3.SVCT_DCM
        // 4.SVCT_VCM
        // 5.NTT_SNS
        g_sitp_si_profile_table_number = 5;
        /*
     ** Allocate the array table array.
     */

       retCode = rmf_osal_memAllocP(RMF_OSAL_MEM_SI_OOB, (sizeof(rmf_si_table_t)
                        * g_sitp_si_profile_table_number), (void **) &(ptr_table_array));
        if (RMF_SUCCESS != retCode)
        {
                RDK_LOG(
                                RDK_LOG_ERROR,
                                "LOG.RDK.SI",
                                "<%s: %s> - Error - Failed to allocate SI table array: %d\n",
                                SIMODULE, __FUNCTION__, retCode);
                return RMF_SI_FAILURE;
        }
        RDK_LOG(RDK_LOG_TRACE5, "LOG.RDK.SI",
                        "<%s: %s> - Allocate Success: %d\n",
                        SIMODULE, __FUNCTION__, retCode);

        ptr_table_array[0].table_id = NETWORK_INFORMATION_TABLE_ID; // NIT - Network Information Table
        ptr_table_array[0].subtype = CARRIER_DEFINITION_SUBTABLE; // CDS - Carrier Definition Subtable

        ptr_table_array[1].table_id = NETWORK_INFORMATION_TABLE_ID; // NIT - Network Information Table
        ptr_table_array[1].subtype = MODULATION_MODE_SUBTABLE; // MMS - Modulation Mode Subtable

        ptr_table_array[2].table_id = SF_VIRTUAL_CHANNEL_TABLE_ID; // SVCT - Short-form Virtual Channel Table
        ptr_table_array[2].subtype = VIRTUAL_CHANNEL_MAP; // VCM - Virtual Channel Map

        ptr_table_array[3].table_id = SF_VIRTUAL_CHANNEL_TABLE_ID; // SVCT - Short-form Virtual Channel Table
        ptr_table_array[3].subtype = DEFINED_CHANNEL_MAP; // DCM - Defined Channels Map

        ptr_table_array[4].table_id = NETWORK_TEXT_TABLE_ID; // NTT - Network Text Table
        ptr_table_array[4].subtype = SOURCE_NAME_SUBTABLE; // SNS - Source Name Subtable

        if (g_sitp_si_profile == PROFILE_CABLELABS_OCAP_1)
                goto finish_initialization;

finish_initialization:
        /*
     ** Set non-specific structure members to default values.
     */
        for (ii = 0; ii < g_sitp_si_profile_table_number; ii++)
        {
                ptr_table_array[ii].table_state = RMF_SI_STATE_NONE;
                ptr_table_array[ii].rev_type = RMF_SI_REV_TYPE_UNKNOWN;
                ptr_table_array[ii].rev_sample_count = 0;
                ptr_table_array[ii].table_acquired = false;
                ptr_table_array[ii].table_unique_id = 0;
                ptr_table_array[ii].version_number = SITP_SI_INIT_VERSION;
                ptr_table_array[ii].number_sections = 0;
				ptr_table_array[ii].rolling_match_counter = 0;
        for (jj = 0; jj < MAX_SECTION_NUMBER; jj++)
        {
            ptr_table_array[ii].section[jj].section_acquired = false;
            ptr_table_array[ii].section[jj].crc_32 = SITP_SI_INIT_CRC_32;
            ptr_table_array[ii].section[jj].crc_section_match_count = 0;
            ptr_table_array[ii].section[jj].last_seen_time = 0;
        }
        RDK_LOG(
                RDK_LOG_TRACE5,
                "LOG.RDK.SI",
                "<%s: %s> - Finish init - table: %d table_id: 0x%x(%d)\n",
                SIMODULE, __FUNCTION__, ii, ptr_table_array[ii].table_id,
                ptr_table_array[ii].table_id);
    }

        *handle_table_array = ptr_table_array;

        RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                        "<%s: %s> - Exit\n", SIMODULE, __FUNCTION__);
    return RMF_SUCCESS;
}

rmf_Error rmf_OobSiMgr::createAndSetSTTDataArray(void)
{
    rmf_Error retCode = RMF_SUCCESS;
    uint32_t jj = 0;

    RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                    "<%s: %s> - Enter\n", SIMODULE, __FUNCTION__);

    // STT
    /*
    ** Allocate the array table array.
    */
    retCode = rmf_osal_memAllocP(RMF_OSAL_MEM_SI_OOB, sizeof(rmf_si_table_t),
                    (void **) &(g_stt_table));
    if (RMF_SUCCESS != retCode)
    {
            RDK_LOG(
                            RDK_LOG_ERROR,
                            "LOG.RDK.SI",
                            "<%s: %s> - Error - Failed to allocate SI table array: %d\n",
                            SIMODULE, __FUNCTION__, retCode);
            return RMF_SI_FAILURE;
    }
    RDK_LOG(RDK_LOG_TRACE5, "LOG.RDK.SI",
                    "<%s: %s> - Allocate Success: %d\n",
                    SIMODULE, __FUNCTION__, retCode);

    g_stt_table->table_id = SYSTEM_TIME_TABLE_ID;
    g_stt_table->subtype = 0; // Unknown?

    /*
    ** Set non-specific structure members to default values.
    */
    g_stt_table->table_state = RMF_SI_STATE_NONE;
    g_stt_table->rev_type = RMF_SI_REV_TYPE_UNKNOWN;
    g_stt_table->rev_sample_count = 0;
    g_stt_table->table_acquired = false;
    g_stt_table->table_unique_id = 0;
    g_stt_table->version_number = SITP_SI_INIT_VERSION;
    g_stt_table->number_sections = 0;
    for (jj = 0; jj < MAX_SECTION_NUMBER; jj++)
    {
        g_stt_table->section[jj].section_acquired = false;
        g_stt_table->section[jj].crc_32 = SITP_SI_INIT_CRC_32;
        g_stt_table->section[jj].crc_section_match_count = 0;
        g_stt_table->section[jj].last_seen_time = 0;
    }

    RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                    "<%s: %s> - Exit\n", SIMODULE, __FUNCTION__);
    
    return RMF_SUCCESS;
}

rmf_Error rmf_OobSiMgr::get_table_section(rmf_FilterSectionHandle * section_handle,
        uint32_t unique_id)
{
        rmf_Error retCode = RMF_SUCCESS;
        rmf_FilterSectionHandle sect_h = 0;
        RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI", "<%s: %s> - Enter\n",
                        SIMODULE, __FUNCTION__);

#ifdef TEST_WITH_PODMGR
        sect_h = *section_handle;

        retCode = m_pOobSectionFilter->GetSectionHandle(unique_id, 0, &sect_h);

        if (retCode == RMF_SUCCESS)
        {
                RDK_LOG(
                                RDK_LOG_TRACE5,
                                "LOG.RDK.SI",
                                "<%s: %s> - got section handle - section_handle: 0x%x(%d) unique_id: 0x%x\n",
                                SIMODULE, __FUNCTION__, sect_h, sect_h, unique_id);
                retCode = RMF_SUCCESS;
        }
        else
        {
                RDK_LOG(
                                RDK_LOG_ERROR,
                                "LOG.RDK.SI",
                                "<%s: %s> - ERROR: Problem getting table section - retCode: %d, unique_id: %d.\n",
                                SIMODULE, __FUNCTION__, retCode, unique_id);
                retCode = RMF_SI_FAILURE;
        }

        *section_handle = sect_h;
#endif
        RDK_LOG(RDK_LOG_TRACE5, "LOG.RDK.SI", "<%s: %s> - Exit\n",
                        SIMODULE, __FUNCTION__);

        return retCode;
}

rmf_Error rmf_OobSiMgr::release_table_section(rmf_si_table_t * table_data,
        rmf_FilterSectionHandle section_handle)
{
        rmf_Error retCode = RMF_SUCCESS;
        RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                        "<%s: %s> - Enter\n", SIMODULE, __FUNCTION__);

#ifdef TEST_WITH_PODMGR
        retCode = m_pOobSectionFilter->ReleaseSection(section_handle);
        if (retCode == RMF_SUCCESS)
        {
                RDK_LOG(
                                RDK_LOG_TRACE5,
                                "LOG.RDK.SI",
                                "<%s: %s> - released section handle - section_handle: 0x%x, table_id: 0x%x, subtype: %d\n",
                                SIMODULE, __FUNCTION__, section_handle, table_data->table_id,
                                table_data->subtype);
                retCode = RMF_SUCCESS;
        }
        else
        {
                RDK_LOG(
                                RDK_LOG_ERROR,
                                "LOG.RDK.SI",
                                "<%s: %s> - ERROR: Problem releasing table section - retCode: %d, section_handle: 0x%x, table_id: 0x%x, subtype: %d\n",
                                SIMODULE, __FUNCTION__, retCode, section_handle, table_data->table_id,
                                table_data->subtype);
                retCode = RMF_SI_FAILURE;
        }
#endif
        RDK_LOG(RDK_LOG_TRACE5, "LOG.RDK.SI", "<%s: %s> - Exit\n",
                        SIMODULE, __FUNCTION__);

        return retCode;
}

/*
 * release the filter for this table and cleanup the struct data.
 */
rmf_Error rmf_OobSiMgr::release_filter(rmf_si_table_t * table_data,
        uint32_t unique_id)
{
        rmf_Error retCode = RMF_SUCCESS;

#ifdef TEST_WITH_PODMGR
        RDK_LOG(
                        RDK_LOG_TRACE4,
                        "LOG.RDK.SI",
                        "<%s: %s> - Releasing filter - unique_id: %d,table_unique_id: %d, table: %s\n",
                        SIMODULE, __FUNCTION__, unique_id, table_data->table_unique_id,
                        sitp_si_tableToString(table_data->table_id));

        retCode = m_pOobSectionFilter->ReleaseFilter(table_data->table_unique_id);
        if (RMF_SUCCESS == retCode)
        {
                /*
         * reset my active_filter unique_id and filter start time
         */
                table_data->table_unique_id = 0;
                table_data->filter_start_time = 0;
                retCode = RMF_SUCCESS;
        }
        else
        {
                RDK_LOG(
                                RDK_LOG_ERROR,
                                "LOG.RDK.SI",
                                "<%s: %s> - Problem releasing filter - unique_id: %d, table_id: %d\n",
                                SIMODULE, __FUNCTION__, table_data->table_unique_id, table_data->table_id);
                retCode = RMF_SI_FAILURE;
        }
#endif
        return retCode;
}

/*
 * activate the filter associated with the table_data.
 */
rmf_Error rmf_OobSiMgr::activate_filter(rmf_si_table_t * table_data,
        uint32_t timesToMatch)
{
        rmf_Error retCode = RMF_SUCCESS;
#ifdef TEST_WITH_PODMGR        
        rmf_FilterSpec filterSpec;
		
		/*Release any existing filter set for this table*/
		if(0 != table_data->table_unique_id)
		{
			RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.SI", "<%s: %s> detected filter leak! Correcting it by releasing filt for table 0x%x-0x%x.\n",
				SIMODULE, __FUNCTION__, table_data->table_id, table_data->subtype);
			release_filter(table_data, table_data->table_unique_id);
		}
        /*
     * SI table filter component values.
     */
    uint16_t pid = 0;
        uint8_t tid_mask[] =
        { 0xFF, 0, 0, 0, 0, 0, 0x0F };
        uint8_t tid_val[] =
        { table_data->table_id, 0, 0, 0, 0, 0, 0x00 | table_data->subtype };
        uint8_t svct_tid_mask[] =
        { 0xFF, 0, 0, 0, 0x0F };
        uint8_t svct_tid_val[] =
        { table_data->table_id, 0, 0, 0, 0x00 | table_data->subtype };
        uint8_t ntt_tid_mask[] =
        { 0xFF, 0, 0, 0, 0, 0, 0, 0x0F };
        uint8_t ntt_tid_val[] =
        { table_data->table_id, 0, 0, 0, 0, 0, 0, 0x00 | table_data->subtype };
        uint8_t neg_mask[] =
        { 0 };
        uint8_t neg_val[] =
        { 0 };

        /* Set the filterspec values */
        switch (table_data->table_id)
        {
        case SF_VIRTUAL_CHANNEL_TABLE_ID:
        {
                filterSpec.pos.length = 5;
                filterSpec.pos.mask = svct_tid_mask;
                filterSpec.pos.vals = svct_tid_val;
                break;
        }
        case NETWORK_INFORMATION_TABLE_ID:
        {
                filterSpec.pos.length = 7;
                filterSpec.pos.mask = tid_mask;
                filterSpec.pos.vals = tid_val;
                break;
        }
        case NETWORK_TEXT_TABLE_ID:
        {
                filterSpec.pos.length = 8;
                filterSpec.pos.mask = ntt_tid_mask;
                filterSpec.pos.vals = ntt_tid_val;
                break;
        }
        default:
                break;
        }
        filterSpec.neg.length = 0;
        filterSpec.neg.mask = neg_mask;
        filterSpec.neg.vals = neg_val;

        /* Set the INB filter source params */
        //filterSource.sourceType = MPE_FILTER_SOURCE_OOB;
        //filterSource.parm.p_OOB.tsId = 0;
        //filterSource.pid = 0x1FFC; //Special PID value for OOB
    pid = 0x1FFC;

        /* print the filter spec */
        printFilterSpec(filterSpec);

        /* Set the filter for tables */
        retCode = m_pOobSectionFilter->SetFilter(pid, &filterSpec,
                        g_sitp_si_queue, RMF_SF_FILTER_PRIORITY_SITP, timesToMatch,
                        0, &table_data->table_unique_id);

        if (retCode == RMF_SUCCESS)
        {
                // Record filter start time
                rmf_osal_timeGetMillis(&table_data->filter_start_time);
                RDK_LOG(
                                RDK_LOG_TRACE4,
                                "LOG.RDK.SI",
                "<%s: %s> - table: %s, times to match: %d, unique_id: %d(0x%x), start_time:%"PRIu64".\n",
                                SIMODULE, __FUNCTION__, sitp_si_tableToString(table_data->table_id),
                                timesToMatch, table_data->table_unique_id,
                                table_data->table_unique_id, table_data->filter_start_time);
                return RMF_SUCCESS;
        }

        /*
     ** Failure - now reset the active filter to default values and
     ** set the correct return value for handling with a state transition
     */
        table_data->table_unique_id = 0;

        RDK_LOG(
                        RDK_LOG_ERROR,
                        "LOG.RDK.SI",
                        "<%s: %s> - FAILED to set a filter for table: 0x%x, error: %d\n",
                        SIMODULE, __FUNCTION__, table_data->table_id, retCode);

        retCode = RMF_SI_FAILURE;
#endif
        RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI", "<%s: %s> - Exit\n",
                        SIMODULE, __FUNCTION__);
        return retCode;
}

rmf_Error rmf_OobSiMgr::activate_stt_filter(rmf_si_table_t * table_data,
        uint32_t timesToMatch)
{
    return RMF_SUCCESS;
}

/*
 *
 *  Print the Section Filter filter spec struct for debugging.
 */
void rmf_OobSiMgr::printFilterSpec(rmf_FilterSpec filterSpec)
{
        uint32_t index;
        char spec[1024];

        for (index = 0; index < filterSpec.pos.length; index++)
        {
                snprintf(spec, 1024, "<%s: printFilterSpec> - Positive length: %u ",
                                SIMODULE, filterSpec.pos.length);
                if (filterSpec.pos.length > 0)
                {
                        snprintf(spec, 1024, "%s Mask: 0x", spec);
                        for (index = 0; index < filterSpec.pos.length; index++)
                        {
                                snprintf(spec, 1024, "%s%2.2x", spec,
                                                filterSpec.pos.mask[index]);
                        }
                        snprintf(spec, 1024, "%s Value: 0x", spec);
                        for (index = 0; index < filterSpec.pos.length; index++)
                        {
                                snprintf(spec, 1024, "%s%2.2x", spec,
                                                filterSpec.pos.vals[index]);
                        }
                }
        }

        RDK_LOG(RDK_LOG_TRACE5, "LOG.RDK.SI", "%s\n", spec);

        snprintf(spec, 1024, "<%s: printFilterSpec> - Negative length: %u ",
                        SIMODULE,  filterSpec.neg.length);
        if (filterSpec.neg.length > 0)
        {
                snprintf(spec, 1024, "%s Mask: 0x", spec);
                for (index = 0; index < filterSpec.neg.length; index++)
                {
                        snprintf(spec, 1024, "%s%2.2x", spec, filterSpec.neg.mask[index]);
                }
                snprintf(spec, 1024, "%s Value: 0x", spec);
                for (index = 0; index < filterSpec.neg.length; index++)
                {
                        snprintf(spec, 1024, "%s%2.2x", spec, filterSpec.neg.vals[index]);
                }
        }
        RDK_LOG(RDK_LOG_TRACE5, "LOG.RDK.SI", "%s\n", spec);

}

/*
 ** Iterate through the section list to see if
 ** all of the sections associated with this table have been
 ** acquired.
 */

rmf_osal_Bool rmf_OobSiMgr::isTable_acquired(rmf_si_table_t * ptr_table, rmf_osal_Bool *all_sections_acquired)
{
        uint32_t ii = 0;
        *all_sections_acquired = true;

        RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                        "<%s: %s> - number_of_sections: %d\n", SIMODULE, __FUNCTION__,
                        ptr_table->number_sections);

    for (ii = 0; ii < ptr_table->number_sections; ii++)
    {
        RDK_LOG(
                RDK_LOG_TRACE4,
                "LOG.RDK.SI",
                "<%s: %s> - ptr_table->table_id: 0x%x ptr_table->subtype: %d index: %d, acquired: %s\n",
                SIMODULE, __FUNCTION__, ptr_table->table_id, ptr_table->subtype, ii, true
                        == ptr_table->section[ii].section_acquired ? "true"
                        : "false");
        if (false == ptr_table->section[ii].section_acquired)
        {
            *all_sections_acquired = false;
        }
    }
    if (*all_sections_acquired)
        return true;
	else if (RMF_SI_REV_TYPE_RDD == ptr_table->rev_type)
		return false;
        // This is an optimization to expedite SVCT (VCM) acquisition.
        // If DCM is already fully acquired and if all of the 'defined'
        // channels in DCM are found 'mapped' in the VCM sections acquired
        // so far we don't need to wait for any more sections of VCM.
        // VCM can be marked as acquired.
    switch (ptr_table->table_id)
    {
        case SF_VIRTUAL_CHANNEL_TABLE_ID:
                {
                        if (ptr_table->subtype == VIRTUAL_CHANNEL_MAP)
                        {
                                // If DCM is acquired and all of the defined channels are processed from VCM,
                                // mark VCM sections as acquired
                                if (((getSVCTAcquired() & DCM_ACQUIRED) == DCM_ACQUIRED) && m_pSiCache->IsVCMComplete())
                                {
                                RDK_LOG(
                                                RDK_LOG_TRACE5,
                                                "LOG.RDK.SI",


                                                "<%s::isTable_acquired> -  DCM already acquired and IsVCMComplete() returned true, marking VCM acquired!\n",
                                                        SIMODULE, __FUNCTION__);
                ptr_table->table_acquired = true;
                                        return true;
                                }
                                else
                                {
                                        if (((getSVCTAcquired() & DCM_ACQUIRED) != DCM_ACQUIRED))
                                        {
                                         RDK_LOG(RDK_LOG_TRACE5, "LOG.RDK.SI", "<%s::isTable_acquired> -  DCM is not yet acquired \n", SIMODULE, __FUNCTION__);
                                        }
                                        else if (((getSVCTAcquired() & DCM_ACQUIRED) == DCM_ACQUIRED) && !m_pSiCache->IsVCMComplete())
                                        {
                                        RDK_LOG(RDK_LOG_TRACE5, "LOG.RDK.SI", "<%s::isTable_acquired> -  DCM is DONE but mpe_siIsVCMComplete() returned false\n", SIMODULE, __FUNCTION__);
                                        }
                                }
        }
        break;
    }
    default:
        break;
    }



        return false;
}

/*
 * interate through the section array for the table param and look for the crc.  If a match is found,
 * return the index of the section in the section array with the matching crc.
 */
rmf_osal_Bool rmf_OobSiMgr::matchedCRC32(rmf_si_table_t * table_data, uint32_t crc,
        uint32_t * index)
{
        uint32_t ii;

    for (ii = 0; ii < table_data->number_sections; ii++)
    {
        RDK_LOG(
                RDK_LOG_TRACE5,
                "LOG.RDK.SI",
                "<%s: %s> - looking for crc: 0x%08x for table_id: 0x%x, subtype: %d, section[%d] crc: 0x%08x last_seen %"PRIu64"\n",
                SIMODULE, __FUNCTION__, crc, table_data->table_id, table_data->subtype, ii,
                table_data->section[ii].crc_32, table_data->section[ii].last_seen_time);
        if (table_data->section[ii].crc_32 == crc)
        {
            RDK_LOG(RDK_LOG_TRACE5, "LOG.RDK.SI",
                    "<%s: %s> - matched crc: 0x%08x\n", SIMODULE, __FUNCTION__, crc);
            *index = ii;
            return true;
        }
    }
    return false;
}

/*
 * Notify or Update SIDB of a complete table
 * acquisition.
 */
void rmf_OobSiMgr::set_tablesAcquired(rmf_si_table_t * ptr_table)
{
        rmf_Error retCode = RMF_SUCCESS;

        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI",
                "<%s: %s> - Enter\n", SIMODULE, __FUNCTION__);
 
        m_pSiCache->LockForWrite();
        switch (ptr_table->table_id)
        {
        case NETWORK_INFORMATION_TABLE_ID:
        {
                switch (ptr_table->subtype)
                {
                case CARRIER_DEFINITION_SUBTABLE:
                {
                        RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                                        "<%s: %s> - 0x%x\n", SIMODULE, __FUNCTION__,
                                        getTablesAcquired());
                        if (getTablesAcquired() & CDS_ACQUIRED)
                        {
                                RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                                                "<%s::%s> - Setting CDS updated.\n",
                                                SIMODULE, __FUNCTION__);
                                NotifyTableChanged(OOB_NIT_CDS,
                                                RMF_SI_CHANGE_TYPE_MODIFY, 0);
                        }
                        else
                        {
                                RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                                                "<%s::%s> - Setting CDS acquired.\n",
                                                SIMODULE, __FUNCTION__);
                                setTablesAcquired(getTablesAcquired() | CDS_ACQUIRED);
                                NotifyTableChanged(OOB_NIT_CDS, RMF_SI_CHANGE_TYPE_ADD, 0);
                        }
            if((getTablesAcquired() & MMS_ACQUIRED)
                && (getTablesAcquired() & DCM_ACQUIRED)
                && (getTablesAcquired() & VCM_ACQUIRED)
                && ((getTablesAcquired() & NTT_ACQUIRED) || inIpDirectMode ))
            {
                // Update Service entries
                m_pSiCache->UpdateServiceEntries();
            }
                        break;
                }
                case MODULATION_MODE_SUBTABLE:
                {
                        RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                                        "<%s: %s> - 0x%x\n", SIMODULE, __FUNCTION__,
                                        getTablesAcquired());
                        if (getTablesAcquired() & MMS_ACQUIRED)
                        {
                                RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                                                "<%s::%s> - Setting MMS updated.\n",
                                                SIMODULE, __FUNCTION__);
                                NotifyTableChanged(OOB_NIT_MMS,
                                                RMF_SI_CHANGE_TYPE_MODIFY, 0);
                        }
                        else
                        {
                                RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                                                "<%s::%s> - Setting MMS acquired.\n",
                                                SIMODULE, __FUNCTION__);
                                setTablesAcquired(getTablesAcquired() | MMS_ACQUIRED);
                                NotifyTableChanged(OOB_NIT_MMS, RMF_SI_CHANGE_TYPE_ADD, 0);
                        }
            if((getTablesAcquired() & CDS_ACQUIRED)
                && (getTablesAcquired() & VCM_ACQUIRED)
                && (getTablesAcquired() & DCM_ACQUIRED)
                && ((getTablesAcquired() & NTT_ACQUIRED) || inIpDirectMode ))
            {
                // Update Service entries
               m_pSiCache->UpdateServiceEntries();
            }
                        break;
                }
                default:
                {
                        RDK_LOG(RDK_LOG_WARN, "LOG.RDK.SI",
                                        "<%s::%s> - no match for subtype - %d.\n",
                                        SIMODULE, __FUNCTION__, ptr_table->subtype);
                        break;
                }
                }
                break;
        } // NETWORK_INFORMATION_TABLE_ID
        case NETWORK_TEXT_TABLE_ID:
        {
                if (getTablesAcquired() & NTT_ACQUIRED)
                {
                        RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                                        "<%s::%s> - Setting NTT updated.\n",
                                        SIMODULE, __FUNCTION__);
                        NotifyTableChanged(OOB_NTT_SNS, RMF_SI_CHANGE_TYPE_MODIFY, 0);
                }
                else
                {
                        RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                                        "<%s::%s> - Setting NTT acquired.\n",
                                        SIMODULE, __FUNCTION__);
                        setTablesAcquired(getTablesAcquired() | NTT_ACQUIRED);
#ifdef MPE_FEATURE_PERFORMANCEMARKERS_EXT
            PerformanceMarker_PutMarker(PERF_MARKER_NTT_ACQUIRED);
#endif
                        NotifyTableChanged(OOB_NTT_SNS, RMF_SI_CHANGE_TYPE_ADD, 0);
                }
        if((getTablesAcquired() & CDS_ACQUIRED)
            && (getTablesAcquired() & MMS_ACQUIRED)
            && (getTablesAcquired() & DCM_ACQUIRED)
            && (getTablesAcquired() & VCM_ACQUIRED) )
        {
            // Update Service names
            m_pSiCache->UpdateServiceEntries();
        }
                break;
        } // NETWORK_TEXT_TABLE_ID

        case SF_VIRTUAL_CHANNEL_TABLE_ID:
        {
                switch (ptr_table->subtype)
                {
                case DEFINED_CHANNEL_MAP:
                {
                        // If DCM is acquired and all the entries acquired so far in DCM
                        // are found mapped in VCM then mark the VCM also acquired.

                        RDK_LOG(
                                        RDK_LOG_TRACE4,
                                        "LOG.RDK.SI",
                                        "<%s::%s> - DCM acquired, check VCM state..\n",
                                        SIMODULE, __FUNCTION__);
                        if (m_pSiCache->IsVCMComplete())
                        {
                                if (getTablesAcquired() & VCM_ACQUIRED)
                                {
                                        NotifyTableChanged(OOB_SVCT_VCM,
                                                        RMF_SI_CHANGE_TYPE_MODIFY, 0);
                                }
                                else
                                {
                                        setSVCTAcquired(getSVCTAcquired() | VCM_ACQUIRED);
                                        setTablesAcquired(getTablesAcquired() | VCM_ACQUIRED);
                    RDK_LOG(
                            RDK_LOG_TRACE4,
                            "LOG.RDK.SI",
                            "<%s::%s> - DCM acquired, mpe_siIsVCMComplete() returned true, setting VCM acquired\n",
                            SIMODULE, __FUNCTION__);
                                        NotifyTableChanged(OOB_SVCT_VCM,
                                                        RMF_SI_CHANGE_TYPE_ADD, 0);
                                }
                        }

                        // set DCM state, signal SIDB with DCM events
                        if (getTablesAcquired() & DCM_ACQUIRED)
                        {
                                //  signal event
                                // Currently un-used
                        }
                        else
                        {
                                RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                                                "<%s::%s> - Setting DCM acquired.\n",
                                                SIMODULE, __FUNCTION__);
                                setSVCTAcquired(getSVCTAcquired() | DCM_ACQUIRED);
                                setTablesAcquired(getTablesAcquired() | DCM_ACQUIRED);
                                //  signal event
                                // Currently un-used
                                //mpe_siNotifyTableChanged(OOB_SVCT_DCM, MPE_SI_CHANGE_TYPE_ADD, 0);
                        }
                }
        if((getTablesAcquired() & CDS_ACQUIRED)
            && (getTablesAcquired() & MMS_ACQUIRED)
            && (getTablesAcquired() & VCM_ACQUIRED)
            && ((getTablesAcquired() & NTT_ACQUIRED) || inIpDirectMode ))
        {
            // Update Service entries
           m_pSiCache->UpdateServiceEntries();
        }
                        break;
                case VIRTUAL_CHANNEL_MAP:
                {
                        if (getTablesAcquired() & VCM_ACQUIRED)
                        {
                                NotifyTableChanged(OOB_SVCT_VCM,
                                                RMF_SI_CHANGE_TYPE_MODIFY, 0);
                        }
                        else
                        {
                                RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                                                "<%s::%s> - Setting VCM acquired.\n",
                                                SIMODULE, __FUNCTION__);
                                setSVCTAcquired(getSVCTAcquired() | VCM_ACQUIRED);
                                setTablesAcquired(getTablesAcquired() | VCM_ACQUIRED);
                                NotifyTableChanged(OOB_SVCT_VCM, RMF_SI_CHANGE_TYPE_ADD,
                                                0);
                        }
            if((getTablesAcquired() & CDS_ACQUIRED)
                && (getTablesAcquired() & MMS_ACQUIRED)
                && (getTablesAcquired() & DCM_ACQUIRED)
                && ((getTablesAcquired() & NTT_ACQUIRED) || inIpDirectMode ))
            {
                // Update Service entries
                m_pSiCache->UpdateServiceEntries();
            }
                        break;
                }
                default:
                {
                        RDK_LOG(RDK_LOG_WARN, "LOG.RDK.SI",
                                        "<%s::%s> - no match for subtype - %d.\n",
                                        SIMODULE, __FUNCTION__, ptr_table->subtype);
                        break;
                }
                }
                RDK_LOG(
                                RDK_LOG_TRACE4,
                                "LOG.RDK.SI",
                                "<%s::%s> - getTablesAcquired(): 0x%x getSVCTAcquired: 0x%x \n",
                                SIMODULE, __FUNCTION__, getTablesAcquired(), getSVCTAcquired());
                break;
        } //SF_VIRTUAL_CHANNEL_TABLE_ID

        default:
        {
                RDK_LOG(RDK_LOG_WARN, "LOG.RDK.SI",
                                "<%s::%s> - no match for table - 0x%x.\n",
                                SIMODULE, __FUNCTION__, ptr_table->table_id);
                break;
        }
        }

    
        RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                        "<%s::%s> - getTablesAcquired(): 0x%x \n", SIMODULE, __FUNCTION__,
                        getTablesAcquired());

        if (NIT_SVCT_ACQUIRED == getTablesAcquired())
        {
            if (inIpDirectMode)
            {
                uint32_t num_services = 0;
                RDK_LOG(
                    RDK_LOG_ERROR,
                    "LOG.RDK.SI",
                    "<%s::%s> - All IPD tables acquired. Calling mpe_siSetGlobalState.\n",
                    SIMODULE, __FUNCTION__);
#ifdef MPE_FEATURE_PERFORMANCEMARKERS_EXT
                PerformanceMarker_PutMarker(PERF_MARKER_SI_ACQUIRED);
#endif
                m_pSiCache->SetGlobalState ( SI_FULLY_ACQUIRED);
                send_si_event(si_state_to_event(m_pSiCache->GetGlobalState()),0,0);
                setTablesAcquired(getTablesAcquired() | SIDB_SIGNALED);
                (void) m_pSiCache->GetTotalNumberOfServices(&num_services);
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI",
                    "<%s::%s> - Number of Services: %d\n", SIMODULE, __FUNCTION__,
                    num_services);
            }
            else
            {
                RDK_LOG(
                    RDK_LOG_TRACE4,
                    "LOG.RDK.SI",
                    "<%s::%s> - NIT, SVCT tables acquired. Calling mpe_siSetGlobalState.\n",
                                SIMODULE, __FUNCTION__);
                m_pSiCache->SetGlobalState ( SI_NIT_SVCT_ACQUIRED);
                send_si_event(si_state_to_event(m_pSiCache->GetGlobalState()),0,0);
            }
        }
        else if (TABLES_ACQUIRED == getTablesAcquired())
        {
                uint32_t num_services = 0;
                RDK_LOG(
                                RDK_LOG_TRACE4,
                                "LOG.RDK.SI",
                                "<%s::%s> - All OOB tables acquired. Calling mpe_siSetGlobalState.\n",
                                SIMODULE, __FUNCTION__);
#ifdef MPE_FEATURE_PERFORMANCEMARKERS_EXT
        PerformanceMarker_PutMarker(PERF_MARKER_SI_ACQUIRED);
#endif
        m_pSiCache->SetGlobalState ( SI_FULLY_ACQUIRED);
        send_si_event(si_state_to_event(m_pSiCache->GetGlobalState()),0,0);
        setTablesAcquired(getTablesAcquired() | SIDB_SIGNALED);
        (void) m_pSiCache->GetTotalNumberOfServices(&num_services);
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI",
                "<%s::%s> - Number of Services: %d\n", SIMODULE, __FUNCTION__,
                num_services);
    }
    // If all tables are acquired update cache
        // This will also be triggered when new sections are
        // processed for an already acquired table
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI","getTablesAcquired() returns %d\n", getTablesAcquired());
    if((getTablesAcquired() & CDS_ACQUIRED)
        && (getTablesAcquired() & MMS_ACQUIRED)
        && (getTablesAcquired() & DCM_ACQUIRED)
        && ((getTablesAcquired() & NTT_ACQUIRED) || inIpDirectMode ))
    {
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI","SI_FULLY_ACQUIRED, DO CACHING NOW from live\n");
        if((retCode = m_pSiCache->CacheSiEntriesFromLive())==RMF_SUCCESS)
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI","CACHING DONE.\n");
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI","Error during CACHING, retCode=0x%x.\n", retCode);
        }
#if 0
        /* Cache g_si_sourceName_entry, g_si_entry */
        if (g_siOOBCacheEnable && g_siOOBCachePost114Location)
        {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI","SI_FULLY_ACQUIRED, DO CACHING NOW from live\n");
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
#endif
        }
        m_pSiCache->ReleaseWriteLock();

}

/*
 * reset the table data structure section array and all values
 * that control the revisioning and table acquisition actions
 * of this state machine.
 */
void rmf_OobSiMgr::reset_section_array(rmf_si_revisioning_type_e rev_type)
{
        uint32_t ii;
        RDK_LOG(RDK_LOG_TRACE5, "LOG.RDK.SI", "<%s::%s> - Enter\n",
                        SIMODULE, __FUNCTION__);

        if (RMF_SI_REV_TYPE_RDD == rev_type)
        {
                for (ii = 0; ii < g_table->number_sections; ii++)
                {
                        g_table->section[ii].section_acquired = false;
                        g_table->section[ii].crc_32 = 0;
                        g_table->section[ii].crc_section_match_count = 0;
                }

                g_table->table_acquired = false;
                g_table->number_sections = 0;
        }
        else
        {
                for (ii = 0; ii < MAX_SECTION_NUMBER; ii++)
                {
                        g_table->section[ii].section_acquired = false;
                        g_table->section[ii].crc_section_match_count = 0;
                }
                g_table->table_acquired = false;
        }
}

/*
 * find the table in the table array with the events associated unique_id.
 */
rmf_Error rmf_OobSiMgr::findTableIndexByUniqueId(uint32_t * index, uint32_t unique_id)
{
        uint32_t table_index = 0;
        for (table_index = 0; table_index < g_sitp_si_profile_table_number; table_index++)
        {
                RDK_LOG(
                                RDK_LOG_TRACE5,
                                "LOG.RDK.SI",
                                "<%s: %s> - uniqueId: %d, g_table_array[%d].table_unique_id: %d.\n",
                                SIMODULE, __FUNCTION__, unique_id, table_index,
                                g_table_array[table_index].table_unique_id);
                if (g_table_array[table_index].table_unique_id == unique_id)
                {
                        RDK_LOG(
                                        RDK_LOG_TRACE5,
                                        "LOG.RDK.SI",
                                        "<%s: %s> - FOUND - uniqueId: %d, g_table_array[%d].table_unique_id: %d.\n",
                                        SIMODULE, __FUNCTION__, unique_id, table_index,
                                        g_table_array[table_index].table_unique_id);
                        *index = table_index;
                        return RMF_SUCCESS;
                }
        }

        /* couldn't find active filter - error */
        return RMF_SI_FAILURE;
}

/*
 * Dump the table data.  for debug.
 */
void rmf_OobSiMgr::dumpTableData(rmf_si_table_t * table_data)
{
        uint32_t ii = 0;

    RDK_LOG(
            RDK_LOG_TRACE5,
            "LOG.RDK.SI",
            "<%s::%s> - table_id: %s, subtype: %d,  state: %s, rev_type: %s, table_acquired: %s, unique_id: %d, version: %d, number_sections: %d\n",
            SIMODULE, __FUNCTION__, sitp_si_tableToString(table_data->table_id),
            table_data->subtype,
            sitp_si_stateToString(table_data->table_state),
            table_data->rev_type == RMF_SI_REV_TYPE_RDD ? "RDD" : "CRC",
            table_data->table_acquired ? "TRUE" : "FALSE",
            table_data->table_unique_id, table_data->version_number,
            table_data->number_sections);
    for (ii = 0; ii < table_data->number_sections; ii++)
    {
        if (table_data->rev_type == RMF_SI_REV_TYPE_RDD)
        {
            RDK_LOG(RDK_LOG_TRACE5, "LOG.RDK.SI",
                    "<%s::%s> - index: %d, section_acquired: %s\n",
                    SIMODULE, __FUNCTION__, ii,
                    table_data->section[ii].section_acquired ? "YES" : "NO");
        }
        else
        {
            RDK_LOG(
                    RDK_LOG_TRACE5,
                    "LOG.RDK.SI",
                    "<%s::%s> - index: %d, section_acquired: %s, crc: 0x%08x, match_count: %d, last_seen %"PRIu64"\n",
                    SIMODULE, __FUNCTION__, ii,
                    table_data->section[ii].section_acquired ? "YES" : "NO",
                    table_data->section[ii].crc_32,
                    table_data->section[ii].crc_section_match_count,
                    table_data->section[ii].last_seen_time);
        }
    }
}

rmf_osal_Bool rmf_OobSiMgr::checkForActiveFilters(void)
{
        uint32_t table_index = 0;
        RDK_LOG(RDK_LOG_TRACE5, "LOG.RDK.SI",
                        "<%s: %s> - Enter\n", SIMODULE, __FUNCTION__);
        for (table_index = 0; table_index < g_sitp_si_profile_table_number; table_index++)
        {
                if (g_table_array[table_index].table_unique_id != 0)
                {
                        RDK_LOG(RDK_LOG_TRACE5, "LOG.RDK.SI",
                                        "<%s: %s> - found filter!\n", SIMODULE, __FUNCTION__);
                        return TRUE;
                }
        }
        RDK_LOG(RDK_LOG_TRACE5, "LOG.RDK.SI",
                        "<%s: %s> - no filters!\n", SIMODULE, __FUNCTION__);
        return FALSE;
}

rmf_osal_Bool rmf_OobSiMgr::checkCRCMatchCount(rmf_si_table_t * ptr_table,
        uint32_t section_index)
{
        uint32_t section_seen_count = 0;
        switch (ptr_table->table_id)
        {
        case NETWORK_INFORMATION_TABLE_ID:
                section_seen_count = g_sitp_si_max_nit_section_seen_count;
                break;
        case SF_VIRTUAL_CHANNEL_TABLE_ID:
        {
                if (ptr_table->subtype == VIRTUAL_CHANNEL_MAP)
                        section_seen_count = g_sitp_si_max_vcm_section_seen_count;
                else if (ptr_table->subtype == DEFINED_CHANNEL_MAP)
                        section_seen_count = g_sitp_si_max_dcm_section_seen_count;
        }
                break;
        case NETWORK_TEXT_TABLE_ID:
                section_seen_count = g_sitp_si_max_ntt_section_seen_count;
                break;
        default:
                break;
        }

        RDK_LOG(RDK_LOG_TRACE5, "LOG.RDK.SI",
                        "<%s: %s> -ptr_table->section[section_index].crc_section_match_count: %d, section_seen_count: %d\n", 
                        SIMODULE, __FUNCTION__,
                        ptr_table->section[section_index].crc_section_match_count,
                        section_seen_count);
        if (ptr_table->section[section_index].crc_section_match_count
                        == section_seen_count)
        {
                return true;
        }
        else
        {
                return false;
        }

}

void rmf_OobSiMgr::checkForTableAcquisitionTimeouts(void)
{
        uint32_t table_index = 0;
        rmf_osal_TimeMillis current_time = 0;
        rmf_si_table_t *table = NULL;

        // Record the current time
        rmf_osal_timeGetMillis(&current_time);

        // Loop thru all the tables
        for (table_index = 0; table_index < g_sitp_si_profile_table_number; table_index++)
        {
                table = &g_table_array[table_index];

                RDK_LOG(
                                RDK_LOG_TRACE5,
                                "LOG.RDK.SI",
                                "<%s: %s> - index: %d, table_unique_id: %d.\n",
                                SIMODULE, __FUNCTION__, table_index, table->table_unique_id);

                // If no filters are currently set or if the rev type is RDD, the timers
                // are not utilized. This is an optimization used only for Profile-1
                // SI (CRC rev type) where he number of sections forming a table is unknown.
                if (table->table_unique_id == 0 || table->rev_type
                                == RMF_SI_REV_TYPE_RDD)
                        continue;

                // Check if the time elapsed since the first time filter was set is more than the set timeout.
                switch (table->table_id)
                {
        case NETWORK_INFORMATION_TABLE_ID:
        {
            if (table->subtype == CARRIER_DEFINITION_SUBTABLE)
            {
                if ((current_time - table->filter_start_time)
                        < g_sitp_si_max_nit_cds_wait_time)
                    continue;
            }
            else if (table->subtype == MODULATION_MODE_SUBTABLE)
            {
                if ((current_time - table->filter_start_time)
                        < g_sitp_si_max_nit_mms_wait_time)
                    continue;
            }
        }
            break;
                case SF_VIRTUAL_CHANNEL_TABLE_ID:
                {
                        if (table->subtype == VIRTUAL_CHANNEL_MAP)
                        {
                                if ((current_time - table->filter_start_time)
                                                < g_sitp_si_max_svct_vcm_wait_time)
                                        continue;
                        }
                        else if (table->subtype == DEFINED_CHANNEL_MAP)
                        {
                                if ((current_time - table->filter_start_time)
                                                < g_sitp_si_max_svct_dcm_wait_time)
                                        continue;
                        }
                }
                        break;
                case NETWORK_TEXT_TABLE_ID:
                        if ((current_time - table->filter_start_time)
                                        < g_sitp_si_max_ntt_wait_time)
                                continue;
                        break;
                default:
                        continue;
                }
                RDK_LOG(
                                RDK_LOG_TRACE5,
                                "LOG.RDK.SI",
                                "<%s: %s> - table timer expired for table_id: 0x%x subtype: %d unique_id: 0x%x\n",
                                SIMODULE, __FUNCTION__, table->table_id, table->subtype,
                                table->table_unique_id);
                RDK_LOG(
                                RDK_LOG_TRACE5,
                                "LOG.RDK.SI",
                                "<%s: %s> - current_time: %lld, table->filter_start_time: %lld, (current_time - table->filter_start_time): %lld, g_sitp_si_max_nit_cds_wait_time: %d\n",
                                SIMODULE, __FUNCTION__,
                                current_time, table->filter_start_time,
                                (current_time - table->filter_start_time),
                                g_sitp_si_max_nit_cds_wait_time);

		RDK_LOG(
				RDK_LOG_DEBUG,
				"LOG.RDK.SI",
				"<%s: %s : %d> -  calling set_tablesAcquired()\n",
				SIMODULE, __FUNCTION__, __LINE__);

		set_tablesAcquired(table);
                release_filter(table, table->table_unique_id);

                //Reset the section array, and go back to sitp_si_state_wait_table_revision.
                reset_section_array(table->rev_type);
                table->table_state = RMF_SI_STATE_WAIT_TABLE_REVISION;
                if (g_stip_si_process_table_revisions)
                {
                        // If the revisioning is by CRC set the timer to SHORT_TIMER
                        // to expedite the acquisition of next table.
                        g_sitp_si_timeout
                                        = (getTablesAcquired() <= OOB_TABLES_ACQUIRED) ? SHORT_TIMER
                                                        : g_sitp_si_update_poll_interval;
                }
                else
                {
                        g_sitp_si_timeout = g_sitp_si_status_update_time_interval;
                }
        } // for table_index
}


/*This function is run when a filter has returned its last section and is about to be released. It purges all sections that were not matched
 * at least once since the filter started. */
void rmf_OobSiMgr::purgeSectionsNotMatchedByFilter(rmf_si_table_t * table_data)
{
    uint32_t ii, jj;

    if (g_sitp_si_section_retention_time == 0)
    {
        RDK_LOG(
                RDK_LOG_TRACE5,
                "LOG.RDK.SI",
                "<%s: %s> - No section retention threshold defined - purging is disabled\n");
        return;
    }
    for (ii = 0; ii < table_data->number_sections; ii++)
    {
        RDK_LOG( RDK_LOG_TRACE5, "LOG.RDK.SI",
                 "<%s: %s> - Checking section for purge: table_id/subtype: 0x%x/%d, section[%d] with crc 0x%08x. Filt started  %"PRIu64" (%dms ago)\n",
                 SIMODULE, __FUNCTION__, table_data->table_id, table_data->subtype, ii,
                 table_data->section[ii].crc_32, table_data->section[ii].last_seen_time, table_data->filter_start_time);

        if (table_data->section[ii].last_seen_time <= table_data->filter_start_time)
        { // Section ii is old, forget about it
            RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SI",
                     "<%s: %s> - Purging table_id/subtype 0x%x/%d, section [%d] with crc 0x%08x\n",
                     SIMODULE, __FUNCTION__, table_data->table_id, table_data->subtype, ii,
                     table_data->section[ii].crc_32);

            // Move all the other records down...
            table_data->number_sections--;
            for (jj=ii; jj<table_data->number_sections;jj++)
            {
                table_data->section[jj] = table_data->section[jj+1];
            }
            ii--; // The ii element needs to be re-checked (since things have shifted)
        }
    }
}

void rmf_OobSiMgr::purgeOldSectionMatches(rmf_si_table_t * table_data, rmf_osal_TimeMillis current_time)
{
    uint32_t ii, jj;

    if (g_sitp_si_section_retention_time == 0)
    {
        RDK_LOG(
                RDK_LOG_TRACE5,
                "LOG.RDK.SI",
                "<%s: %s> - No section retention threshold defined - purging is disabled\n");
        return;
    }

    for (ii = 0; ii < table_data->number_sections; ii++)
    {
        rmf_osal_TimeMillis curLastSeenTime = table_data->section[ii].last_seen_time;
        RDK_LOG( RDK_LOG_TRACE5, "LOG.RDK.SI",
                 "<%s: %s> - Checking section for purge: table_id/subtype: 0x%x/%d, section[%d] with crc 0x%08x last_seen %"PRIu64" (%dms ago)\n",
                 SIMODULE, __FUNCTION__, table_data->table_id, table_data->subtype, ii,
                 table_data->section[ii].crc_32, curLastSeenTime, current_time - curLastSeenTime );

        if ((current_time - curLastSeenTime) > g_sitp_si_section_retention_time )
        { // Section ii is old, forget about it
            RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SI",
                     "<%s: %s> - Purging table_id/subtype 0x%x/%d, section [%d] with crc 0x%08x (it's %dms old)\n",
                     SIMODULE, __FUNCTION__, table_data->table_id, table_data->subtype, ii,
                     table_data->section[ii].crc_32, (int32_t)(current_time - curLastSeenTime) );

            // Move all the other records down...
            table_data->number_sections--;
            for (jj=ii; jj<table_data->number_sections;jj++)
            {
                table_data->section[jj] = table_data->section[jj+1];
            }
            ii--; // The ii element needs to be re-checked (since things have shifted)
        }
    }

}

void rmf_OobSiMgr::rmf_si_purge_RDD_tables(void)
{
    int i,ii;
    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI","<%s::rmf_si_purge_RDD_tables> - Purging RDD tables...\n",
                    SIMODULE);
    m_pSiCache->LockForWrite();    
    for (i = 0; i < g_sitp_si_profile_table_number; i++)
    {
        if (g_table_array[i].rev_type == RMF_SI_REV_TYPE_RDD) 
	{
            g_table_array[i].version_number = NO_VERSION;
	    
	    for(ii=0;ii<MAX_SECTION_NUMBER;ii++)
	    {
	      g_table_array[i].section[ii].section_acquired  = false;
	      g_table_array[i].section[ii].crc_32 = 0;
	      g_table_array[i].section[ii].crc_section_match_count = 0;
	    }
        }
    }
    m_pSiCache->ReleaseWriteLock();
}


void rmf_OobSiMgr::notify_VCTID_change(void)
{
	rmf_osal_event_handle_t eventHandle;
	rmf_Error retOsal = RMF_SUCCESS;

	RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI", "%s: Sending event\n", __FUNCTION__);

	retOsal = rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_OOBSI, OOB_VCTID_CHANGE_EVENT,
			NULL, &eventHandle);
	
	if(RMF_SUCCESS != retOsal)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "%s: Failed to create event. Error code: 0x%x\n", 
			__FUNCTION__, retOsal);
		return;
	}

	retOsal = rmf_osal_eventqueue_add(g_sitp_si_queue, eventHandle);
	if(RMF_SUCCESS != retOsal)
	{
		RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI", "%s: Could not add event to queue! Error code: 0x%x\n",
			__FUNCTION__, retOsal);
		rmf_osal_event_delete(eventHandle);
	}
	return;
}


void rmf_OobSiMgr::rmf_si_reset_tables(void)
{
	int i,ii;
	RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI","<%s::%s> - Resetting channel map acquisition engine.\n",
			SIMODULE, __FUNCTION__);
	m_pSiCache->LockForWrite();    

	for (i = 0; i < g_sitp_si_profile_table_number; i++)
	{
		if(0 != g_table_array[i].table_unique_id)
		{
			release_filter(&g_table_array[i], g_table_array[i].table_unique_id);
		}
		g_table_array[i].table_state = RMF_SI_STATE_NONE;
		g_table_array[i].rev_type = RMF_SI_REV_TYPE_UNKNOWN;
		g_table_array[i].rev_sample_count = 0;
		g_table_array[i].table_acquired = false;
		g_table_array[i].version_number = SITP_SI_INIT_VERSION;
		g_table_array[i].number_sections = 0;
		g_table_array[i].rolling_match_counter = 0;
		for (ii = 0; ii < MAX_SECTION_NUMBER; ii++)
		{
			g_table_array[i].section[ii].section_acquired = false;
			g_table_array[i].section[ii].crc_32 = SITP_SI_INIT_CRC_32;
			g_table_array[i].section[ii].crc_section_match_count = 0;
			g_table_array[i].section[ii].last_seen_time = 0;
		}
	}
	setSVCTAcquired(0);
	setTablesAcquired(0);
	m_pSiCache->SetDACId(VIRTUAL_CHANNEL_UNKNOWN);
	m_pSiCache->SetChannelMapId(VIRTUAL_CHANNEL_UNKNOWN);
	m_pSiCache->SetGlobalState(SI_ACQUIRING);
	m_pSiCache->SetAllSIEntriesUndefinedUnmapped();

	m_pSiCache->ReleaseWriteLock();
}

/* debug functions */
/** make these global to prevent compiler warning errors **/
const char * rmf_OobSiMgr::sitp_si_eventToString(uint32_t event)
{
        switch (event)
        {
        case RMF_SF_EVENT_SECTION_FOUND:
                return "RMF_SF_EVENT_SECTION_FOUND";
        case RMF_SF_EVENT_LAST_SECTION_FOUND:
                return "RMF_SF_EVENT_LAST_SECTION_FOUND";
        case RMF_OSAL_ETIMEOUT:
                return "RMF_OSAL_ETIMEOUT";
        case RMF_SF_EVENT_SOURCE_CLOSED:
                return "RMF_SF_EVENT_SOURCE_CLOSED";
        case RMF_OSAL_ETHREADDEATH:
                return "RMF_OSAL_ETHREADDEATH";
        case OOB_START_UP_EVENT:
                return "OOB_START_UP_EVENT";
        default:
                return "UNKNOWN EVENT";
        }
}

const char * rmf_OobSiMgr::sitp_si_stateToString(uint32_t state)
{
        switch (state)
        {
        case RMF_SI_STATE_WAIT_TABLE_REVISION:
                return "RMF_SI_STATE_WAIT_TABLE_REVISION";
        case RMF_SI_STATE_WAIT_TABLE_COMPLETE:
                return "RMF_SI_STATE_WAIT_TABLE_COMPLETE";
        case RMF_SI_STATE_NONE:
        default:
                return "RMF_SI_STATE_NONE";
        }

}

const char *rmf_OobSiMgr::sitp_si_tableToString(uint8_t table)
{
        switch (table)
        {
        case NETWORK_INFORMATION_TABLE_ID:
                return "NETWORK_INFORMATION_TABLE";
        case NETWORK_TEXT_TABLE_ID:
                return "NETWORK_TEXT_TABLE";
        case SF_VIRTUAL_CHANNEL_TABLE_ID:
                return "SF_VIRTUAL_CHANNEL_TABLE";
        case SYSTEM_TIME_TABLE_ID:
                return "SYSTEM_TIME_TABLE";
        default:
                return "UNKNOWN TABLE";
        }
}

void rmf_OobSiMgr::setTablesAcquired(uint32_t tables_acquired)
{
        g_sitp_siTablesAcquired = tables_acquired;
}


uint32_t rmf_OobSiMgr::getTablesAcquired(void)
{
        return g_sitp_siTablesAcquired;
}

rmf_OobSiMgr::rmf_OobSiMgr()
{
    rmf_Error retCode = RMF_SUCCESS;
    const char *sitp_si_max_nit_cds_wait_time = NULL;
    const char *sitp_si_max_nit_mms_wait_time = NULL;
    const char *sitp_si_max_svct_dcm_wait_time = NULL;
    const char *sitp_si_max_svct_vcm_wait_time = NULL;
    const char *sitp_si_max_ntt_wait_time = NULL;
    const char *sitp_si_update_poll_interval_min = NULL;
    const char *sitp_si_update_poll_interval_max = NULL;
    const char *sitp_si_status_update_time_interval = NULL;
    const char *sitp_si_initial_section_match_count = NULL;
    const char *sitp_si_rev_sample_sections = NULL;
    const char *sitp_si_process_table_revisions = NULL;
    const char *sitp_si_filter_multiplier = NULL;
    const char *sitp_si_section_retention_time = NULL;
    const char *sitp_si_max_nit_section_seen_count = NULL;
    const char *sitp_si_max_vcm_section_seen_count = NULL;
    const char *sitp_si_max_dcm_section_seen_count = NULL;
    const char *sitp_si_max_ntt_section_seen_count = NULL;
    const char *sitp_si_filter_enabled = NULL;

            /*  Registered SI DB listeners and queues (queues are used at MPE level only) */
        g_si_registration_list = NULL;
        g_registration_list_mutex = NULL; /* sync primitive */
        
        g_sitp_si_threadID = (rmf_osal_ThreadId) - 1;
        g_sitp_si_queue = 0; // OOB thread event queue
        g_sitp_sys_time = 0;
        g_sitp_pod_queue = 0; // POD event queue
        g_sitp_stt_queue = 0; // STT event queue

        g_table_array = NULL;
        g_table = NULL;
        g_shutDown = FALSE; // shutdown flag
        g_sitp_si_stt_threadID = (rmf_osal_ThreadId) - 1;
        g_sitp_si_stt_queue = 0;
        g_stt_table = NULL;
        g_sitp_stt_timeout = 0;
        g_sitp_stt_thread_sleep_millis = 20;
        g_sitp_stt_condition = NULL;

        g_ntpclient_threadID = os_ThreadId_Invalid;

        /*
         *****************************************************************************
         * Flags to indicate acquisition of CDS, MMS and SVCT.
         * When we have these table signal that SIDB can be accessed.
         *****************************************************************************
         */
        g_sitp_svctAcquired = 0;
        g_sitp_siTablesAcquired = 0;

        /*  Control variables for the si caching */
        //g_siOOBCacheEnable = FALSE;
        g_siOOBCacheConvertStatus = FALSE;
        g_siOOBCached = FALSE;
        //g_siOOBCachePost114Location = NULL;
        //g_siOOBCacheLocation = NULL;

        //g_siOOBSNSCacheLocation = NULL;
    g_sitp_si_timeout = 0;
    g_sitp_si_update_poll_interval = 1;
    g_sitp_si_update_poll_interval_max = 0;
    g_sitp_si_update_poll_interval_min = 0;
    g_sitp_pod_status_update_time_interval = 0;
    g_sitp_si_profile = PROFILE_CABLELABS_OCAP_1;
        g_sitp_si_profile_table_number = 0;
    g_sitp_si_status_update_time_interval = 0;
    g_sitp_si_initial_section_match_count = 0;
    g_sitp_si_rev_sample_sections = 0;
    g_stip_si_process_table_revisions = 0;
    g_sitp_si_filter_multiplier = 0;
    g_sitp_si_max_nit_section_seen_count = 0;
    g_sitp_si_max_vcm_section_seen_count = 0;
    g_sitp_si_max_dcm_section_seen_count = 0;
    g_sitp_si_max_ntt_section_seen_count = 0;
    g_siOobFilterEnable = FALSE;
    inIpDirectMode = 0;

        memset(g_badSICacheLocation, 0, sizeof(g_badSICacheLocation));
        memset(g_badSISNSCacheLocation, 0, sizeof(g_badSISNSCacheLocation));

#ifdef TEST_WITH_PODMGR
//    m_pOobSectionFilter = rmf_SectionFilter::create(rmf_filters::oob,NULL);
       m_pOobSectionFilter = new rmf_SectionFilter_OOB( NULL );
#endif

    m_pSiCache = rmf_OobSiCache::getSiCache();
    m_pSiCache->AddSiEventListener(this);
    send_si_event(si_state_to_event(m_pSiCache->GetGlobalState()),0,0);

#ifdef TEST_WITH_PODMGR    
    m_pOobSiParser = rmf_OobSiParser::getParserInstance(m_pOobSectionFilter);
#endif

    m_pOobSiSttMgr = rmf_OobSiSttMgr::getInstance();

    //Create a condition that the SI thread will block on until the
    //first STT is acquired.
    {
        retCode = rmf_osal_condNew(FALSE, FALSE, &g_sitp_stt_condition);
        if ( RMF_SUCCESS != retCode)
        {
            RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SI",
                    "<%s: %s> - failed to create stt condition, error: %ld\n",
                    SIMODULE, __FUNCTION__, retCode);

#ifdef TEST_WITH_PODMGR
	    delete m_pOobSectionFilter;
#endif
            return;
        }
    }
       /* Create global registration list mutex */
        retCode = rmf_osal_mutexNew(&g_registration_list_mutex);
        if (retCode != RMF_SUCCESS)
        {
                RDK_LOG(RDK_LOG_WARN, "LOG.RDK.SI",
                                "<rmf_si_Database_Init() Mutex creation failed, returning RMF_OSAL_EMUTEX\n");
                //return RMF_OSAL_EMUTEX;
        }

    sitp_si_filter_enabled = rmf_osal_envGet("SITP.SI.FILTER.ENABLED");
    if ((sitp_si_filter_enabled == NULL) || (stricmp(sitp_si_filter_enabled, "TRUE") == 0))
    {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI",
                            "%s: OOB Filter is enabled by SITP.SI.FILTER.ENABLED\n", __FUNCTION__);
            g_siOobFilterEnable = TRUE;
    }
    else
    {
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI",
                            "%s: OOB Filter is disabled by SITP.SI.FILTER.ENABLED\n", __FUNCTION__);
    }

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
            "<%s: %s> - Max NIT CDS wait time: %d\n", SIMODULE, __FUNCTION__,
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
            "<%s: %s> - Max NIT MMS wait time: %d\n", SIMODULE, __FUNCTION__,
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
                        "<%s: %s> - Max SVCT DCM wait time: %d\n", SIMODULE, __FUNCTION__,
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
                        "<%s: %s> - Max SVCT VCM wait time: %d\n", SIMODULE, __FUNCTION__,
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
                        "<%s: %s> - Max NTT wait time: %d\n", SIMODULE, __FUNCTION__,
                        g_sitp_si_max_ntt_wait_time);

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
                        "<%s: %s> - SI update max polling interval set to %d ms\n",
                        SIMODULE, __FUNCTION__, g_sitp_si_update_poll_interval_max);

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
                        "<%s: %s> - SI update min polling interval set to %d ms\n",
                        SIMODULE, __FUNCTION__, g_sitp_si_update_poll_interval_min);

        if (g_sitp_si_update_poll_interval_min > g_sitp_si_update_poll_interval_max)
        {
                RDK_LOG(
                                RDK_LOG_TRACE4,
                                "LOG.RDK.SI",
                                "<%s: %s> - min polling interval greater than max polling interval - using default values\n",
                                SIMODULE, __FUNCTION__);
                g_sitp_si_update_poll_interval_min = 25000;
                g_sitp_si_update_poll_interval_max = 30000;
    }

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
            "<%s: %s> - SI status update time interval set to %d ms\n",
            SIMODULE, __FUNCTION__, g_sitp_si_status_update_time_interval);

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
                        "<%s: %s> - Initial acquisition section match count: %d\n",
                        SIMODULE, __FUNCTION__, g_sitp_si_initial_section_match_count);

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
                        "<%s: %s> - sample for table revisions %s\n", SIMODULE, __FUNCTION__,
                        (g_sitp_si_rev_sample_sections ? "ON" : "OFF"));
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
                        "<%s: %s> - table revision processing is %s\n",
                        SIMODULE, __FUNCTION__, (g_stip_si_process_table_revisions ? "ON" : "OFF"));

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
                        "<%s: %s> - SI filter multiplier: %d\n", SIMODULE, __FUNCTION__,
                        g_sitp_si_filter_multiplier);

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
            "<%s: %s> - Section retention threshold: %dms\n", SIMODULE, __FUNCTION__,
            g_sitp_si_section_retention_time);

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
                        "<%s: %s> - NIT section seen count: %d\n", SIMODULE, __FUNCTION__,
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
                        "<%s: %s> - SVCT-VCM section seen count: %d\n",
                        SIMODULE, __FUNCTION__, g_sitp_si_max_vcm_section_seen_count);

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
                        "<%s: %s> - SVCT-DCM section seen count: %d\n",
                        SIMODULE, __FUNCTION__, g_sitp_si_max_dcm_section_seen_count);

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
                        "<%s: %s> - SI NTT section seen count: %d\n", SIMODULE, __FUNCTION__,
                        g_sitp_si_max_ntt_section_seen_count);

// if USE_NTPCLIENT
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI","<%s: %s> - %d\n", SIMODULE, __FUNCTION__, __LINE__);
		const char *ntpclient_enabled = rmf_osal_envGet("NTP.FAILOVER.ENABLED");
		if ((ntpclient_enabled != NULL) && (stricmp(ntpclient_enabled, "TRUE") == 0))
		{
			RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI",
		                "%s: NTPClient Enabled\n", __FUNCTION__);
			//g_ntpclientEnable = TRUE;
			// Create NTP thread.
			retCode = rmf_osal_threadCreate (ntpclientWorkerThread, this,
								RMF_OSAL_THREAD_PRIOR_SYSTEM, RMF_OSAL_THREAD_STACK_SIZE,
								&g_ntpclient_threadID, "rmfNTPClientWorkerThread");
			if (retCode != RMF_SUCCESS)
			{
				RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SI",
							"<%s: %s> - failed to create ntpclientWorkerThread, error: %d\n",
							SIMODULE, __FUNCTION__, retCode);
			}
		}
		else
		{
			RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI",
		                "%s: NTPClient  disabled\n", __FUNCTION__);
		}
// endif //USE_NTPCLIENT
 
#ifdef TEST_WITH_PODMGR
    //if (m_pSiCache->getParseSTT() == TRUE)
    {
            // First create STT thread
            retCode = rmf_osal_threadCreate(sitp_siSTTWorkerThread, this,
                            RMF_OSAL_THREAD_PRIOR_SYSTEM, RMF_OSAL_THREAD_STACK_SIZE,
                            &g_sitp_si_stt_threadID, "rmfSitpSiSTTWorkerThread");
            if (retCode != RMF_SUCCESS)
            {
                    RDK_LOG(
                                    RDK_LOG_ERROR,
                                    "LOG.RDK.SI",
                "<%s: %s> - failed to create sitp_siSTTWorkerThread, error: %d\n",
                                    SIMODULE, __FUNCTION__, retCode);

           //         return retCode;
            }
    }
#endif

    retCode = rmf_osal_threadCreate(sitp_siWorkerThread, this,
            RMF_OSAL_THREAD_PRIOR_SYSTEM, RMF_OSAL_THREAD_STACK_SIZE,
            &g_sitp_si_threadID, "SitpSiWorkerThread");
    if (retCode != RMF_SUCCESS)
    {
        RDK_LOG(
                RDK_LOG_ERROR,
                "LOG.RDK.SI",
                "<%s: %s> - failed to create sitp_siWorkerThread, error: %d\n",
                SIMODULE, __FUNCTION__, retCode);


        //return retCode;
    }
    else
    {
        RDK_LOG(
                RDK_LOG_TRACE4,
                "LOG.RDK.SI",
                "<%s: %s> - sitp_siWorkerThread thread started successfully\n",
                SIMODULE, __FUNCTION__);
    }
}

rmf_OobSiMgr::~rmf_OobSiMgr()
{
        rmf_osal_mutexDelete(g_registration_list_mutex);
}

rmf_OobSiMgr *rmf_OobSiMgr::getInstance()
{
    if(m_pSiMgr == NULL)
        m_pSiMgr = new rmf_OobSiMgr();

    return m_pSiMgr;
}

uint32_t rmf_OobSiMgr::si_state_to_event(rmf_SiGlobalState state)
{
   RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI", "si_state_to_event state: 0x%x \n",
           state);
    if (state == SI_ACQUIRING)
        return RMF_SI_EVENT_SI_ACQUIRING;
    else if (state == SI_NOT_AVAILABLE_YET)
        return RMF_SI_EVENT_SI_NOT_AVAILABLE_YET;
    else if (state == SI_NIT_SVCT_ACQUIRED)
        return RMF_SI_EVENT_NIT_SVCT_ACQUIRED;
    else if (state == SI_FULLY_ACQUIRED)
        return RMF_SI_EVENT_SI_FULLY_ACQUIRED;
    else if (state == SI_DISABLED)
        return RMF_SI_EVENT_SI_DISABLED;
    else if (state == SI_STT_ACQUIRED)
         return RMF_SI_EVENT_STT_ACQUIRED;
    return 0;
}

#ifdef ENABLE_TIME_UPDATE
void rmf_OobSiMgr::send_si_stt_event(uint32_t siEvent, void *pData, uint32_t data_extension)
{
    rmf_SiRegistrationListEntry *walker = NULL;
    rmf_osal_event_params_t event_params = {0};
    rmf_osal_event_handle_t eventHandle;
    rmf_Error retOsal = RMF_SUCCESS;
    bool found = FALSE;

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI", "<send_si_stt_event> \n");

    event_params.data = pData;
    event_params.data_extension = data_extension;

    retOsal = rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_OOBSI, siEvent,
            &event_params, &eventHandle);

    if(retOsal != RMF_SUCCESS)
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI","<%s: %s>: Cannot create RMF OSAL Event \n", SIMODULE, __FUNCTION__);
        return;
    }

    /* Acquire registration list mutex */
    rmf_osal_mutexAcquire(g_registration_list_mutex);

    walker = g_si_registration_list;

    if(walker == NULL)
    {
        rmf_osal_event_delete(eventHandle);
    }

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI", "send_si_stt_event siEvent: 0x%x \n",
           siEvent);

    while (walker)
    {
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SYS", "send_si_stt_event sending siEvent: 0x%x \n",
           siEvent);

	if((walker->si_event_type & siEvent) == siEvent)
        {
            retOsal = rmf_osal_eventqueue_add(walker->queueId, eventHandle);
            if(retOsal != RMF_SUCCESS)
            {
                RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI","<%s: %s>: Cannot add event to registered queue\n", SIMODULE, __FUNCTION__);
                rmf_osal_event_delete(eventHandle);
                break;
            }
	    found = TRUE;
        }
        walker = walker->next;
    }

    if(found == FALSE)
        rmf_osal_event_delete(eventHandle);

    /* Release registration list mutex */
    rmf_osal_mutexRelease(g_registration_list_mutex);
}
#endif

void rmf_OobSiMgr::send_si_event(uint32_t siEvent, uint32_t optional_data, uint32_t changeType)
{
    rmf_SiRegistrationListEntry *walker = NULL;
    rmf_osal_event_params_t event_params = {0};
    rmf_osal_event_handle_t eventHandle;
    rmf_Error retOsal = RMF_SUCCESS;

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI", "<send_si_event> \n");

    event_params.data = (void*)&optional_data;
    event_params.data_extension = changeType;
	//use an event manager instead of maintaining a list of queues in g_si_registration_list.

    /* Acquire registration list mutex */
    rmf_osal_mutexAcquire(g_registration_list_mutex);

    walker = g_si_registration_list;

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI", "send_si_event siEvent: 0x%x \n",
           siEvent);

    while (walker)
    {
        //uint32_t termType = walker->edHandle->terminationType;
        retOsal = rmf_osal_event_create( RMF_OSAL_EVENT_CATEGORY_OOBSI, siEvent,
                &event_params, &eventHandle);

        if(retOsal != RMF_SUCCESS)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI","<%s: %s>: Cannot create RMF OSAL Event \n", SIMODULE, __FUNCTION__);
            break;
        }

        retOsal = rmf_osal_eventqueue_add(walker->queueId, eventHandle);
        if(retOsal != RMF_SUCCESS)
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI","<%s: %s>: Cannot add event to registered queue\n", SIMODULE, __FUNCTION__);
            rmf_osal_event_delete(eventHandle);
            break;
        }

        //rmf_osal_eventQueueSend(walker->queueId, siEvent, (void*) optional_data,
          //      (void*) walker->edHandle, changeType);

        // Do we need to unregister this client?
        /*if (termType == RMF_ED_TERMINATION_ONESHOT || (termType
                == RMF_ED_TERMINATION_EVCODE && siEvent
                == walker->terminationEvent))
        {
            add_to_unreg_list(walker->edHandle);
        }*/

        walker = walker->next;
    }



    /* Unregister all clients that have been added to unregistration list */
    //unregister_clients();

    /* Release registration list mutex */
    rmf_osal_mutexRelease(g_registration_list_mutex);
}

#ifdef ENABLE_TIME_UPDATE
/**
 * @brief This function registers the specified queue to receive SI events if rmf_OobSiMgr has already
 * acquired NIT/SVCT/STT tables, the corresponding events are immediately pushed into the queue to bring
 * the listener updated with current SI acquistion state.
 *
 * @param[in] si_event_type Indicates the type of SI event.
 * Ex: OOB_START_UP_EVENT, RMF_OSAL_ETIMEOUT and so on.
 * @param[in] queueId It is a queue handle to post SI events.
 *
 * @return Returns RMF_SI_SUCCESS if the queueid is successfully added else returns RMF_SI_OUT_OF_MEM
 * indicating out of memory condition.
 */
rmf_Error rmf_OobSiMgr::RegisterForSIEvents(uint32_t si_event_type, rmf_osal_eventqueue_handle_t queueId)
#else
rmf_Error rmf_OobSiMgr::RegisterForSIEvents(rmf_osal_eventqueue_handle_t queueId)
#endif
{
        rmf_SiRegistrationListEntry *walker = NULL;
        rmf_SiRegistrationListEntry *prev_walker = NULL;
        rmf_SiRegistrationListEntry *new_list_member = NULL;
	int found = 0;

        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI",
            "<mpe_siRegisterForSIEvents> called...with queueId: 0x%p\n",
                        queueId);

        /* Acquire registration list mutex */
        rmf_osal_mutexAcquire(g_registration_list_mutex);

	walker = g_si_registration_list;
	while (walker)
	{
#ifdef ENABLE_TIME_UPDATE
		if((walker->queueId == queueId) && ((walker->si_event_type & si_event_type) != si_event_type))
		{
			walker->si_event_type |= si_event_type;
			found = 1;
		}
#else
		if(walker->queueId == queueId)
		{
			found = 1;
		}
#endif
		if (walker->next == NULL)
		{
			prev_walker = walker;
		}
		walker = walker->next;
	}

        rmf_osal_mutexRelease(g_registration_list_mutex);
        /* Release registration list mutex */

	if(!found)
	{
		if (rmf_osal_memAllocP(RMF_OSAL_MEM_SI_OOB, sizeof(rmf_SiRegistrationListEntry),
					(void **) &new_list_member) != RMF_SUCCESS)
		{
			return RMF_SI_OUT_OF_MEM;
		}

#ifdef ENABLE_TIME_UPDATE
		memset(new_list_member, 0x0, sizeof(rmf_SiRegistrationListEntry));
		new_list_member->si_event_type |= si_event_type;
#endif
		new_list_member->queueId = queueId;
		new_list_member->next = NULL;
		if(prev_walker==NULL)
			g_si_registration_list = new_list_member;
		else
			prev_walker->next = new_list_member;
	}
	
        /*
         * RI assumes transition to SI_NIT_SVCT_ACQUIRED before getting to SI_FULLY_ACQUIRE.
         * So send an event to RI to indicate NIT_SVCT are acquired before sending FULLY_ACQUIRE.
         * This avoids unnecessary waiting when SI is init'd with cache data.
         */
        if (m_pSiCache->GetGlobalState()== SI_FULLY_ACQUIRED)
        {
                send_si_event(si_state_to_event(SI_NIT_SVCT_ACQUIRED), 0, 0);
        }

        /*
         * Notify RI of STT arrival.
         */

        //if (m_pSiCache->getParseSTT()== TRUE)
        {
                send_si_event(si_state_to_event(SI_STT_ACQUIRED), 0,0);
        }
        /* Broadcast the global si state */
	send_si_event(si_state_to_event(m_pSiCache->GetGlobalState()), 0,0);

        return RMF_SI_SUCCESS;

}

#ifdef ENABLE_TIME_UPDATE
/**
 * @brief This function removes/unregisters the specified queueId of the specified si_event_type from the
 * SIDB and frees the memory associated with it.
 * <ul>
 * <li> Note : The input parameter si_event_type is considered only if the flag ENABLE_TIME_UPDATE is defined
 * esle the below function prototype is considered where only the queueId is matched in the list
 * and the corresponding entry is removed.
 * <li> UnRegisterForSIEvents(rmf_osal_eventqueue_handle_t queueId)
 * </ul>
 *
 * @param[in] queueId It is a queue handle to post SI events.
 * @param[in] si_event_type Indicates the type of SI event.
 * Ex: OOB_START_UP_EVENT, RMF_OSAL_ETIMEOUT and so on.
 *
 * @return Returns RMF_SI_SUCCESS if the entry corresponding to the queueId is removed else returns RMF_OSAL_EINVAL.
 * indicating the SIDB listerner queue is NULL.
 */
rmf_Error rmf_OobSiMgr::UnRegisterForSIEvents(uint32_t si_event_type, rmf_osal_eventqueue_handle_t queueId)
#else
rmf_Error rmf_OobSiMgr::UnRegisterForSIEvents(rmf_osal_eventqueue_handle_t queueId)
#endif
{
       rmf_SiRegistrationListEntry *walker, *prev;
        rmf_SiRegistrationListEntry *to_delete = NULL;

        /* Acquire registration list mutex */
        rmf_osal_mutexAcquire(g_registration_list_mutex);

        /* Remove from the list */
        if (g_si_registration_list == NULL)
        {
                rmf_osal_mutexRelease(g_registration_list_mutex);
                return RMF_OSAL_EINVAL;
        }
#ifdef ENABLE_TIME_UPDATE
        else if ((g_si_registration_list->queueId == queueId) && ((walker->si_event_type & si_event_type) != si_event_type))
#else
        else if (g_si_registration_list->queueId == queueId)
#endif
        {
#ifdef ENABLE_TIME_UPDATE
		g_si_registration_list->si_event_type = (g_si_registration_list->si_event_type ^ si_event_type);
		if(g_si_registration_list->si_event_type == 0)
		{
#endif
                    to_delete = g_si_registration_list;
                    g_si_registration_list = g_si_registration_list->next;
#ifdef ENABLE_TIME_UPDATE
		}
#endif
        }

        else
        {
                prev = walker = g_si_registration_list;
                while (walker)
                {
#ifdef ENABLE_TIME_UPDATE
                        if ((walker->queueId == queueId) && ((walker->si_event_type & si_event_type) != si_event_type))
			{
				walker->si_event_type = (walker->si_event_type ^ si_event_type);
				if(walker->si_event_type == 0)
				{
					to_delete = walker;
					prev->next = walker->next;
				}
				break;
			}
#else
                        if (walker->queueId == queueId)
                        {
                                to_delete = walker;
                                prev->next = walker->next;
                                break;
                        }
#endif
                        prev = walker;
                        walker = walker->next;
                }
        }

        rmf_osal_mutexRelease(g_registration_list_mutex);
        /* Release registration list mutex */

        if (to_delete != NULL)
        {
                rmf_osal_memFreeP(RMF_OSAL_MEM_SI_OOB, to_delete);
        }

        return RMF_SI_SUCCESS;
}

/*
 Called from SITP when a table is acquired/updated. SI DB uses it to
 deliver events to registered callers.
 */
rmf_Error rmf_OobSiMgr::NotifyTableChanged(rmf_SiTableType table_type,
        uint32_t changeType, uint32_t optional_data)
{
    uint32_t event = 0;
    uint32_t optionalEventData3 = changeType;

    if (table_type == 0) /* unknown table type */
    {
        return RMF_SI_INVALID_PARAMETER;
    }

    RDK_LOG(
            RDK_LOG_DEBUG,
            "LOG.RDK.SI",
            "<mpe_siNotifyTableChanged> called...with tableType: %d, changeType: %d optional_data: 0x%x\n",
            table_type, changeType, optional_data);

    switch (table_type)
    {
    case OOB_SVCT_VCM:
    {
        RDK_LOG(
                RDK_LOG_INFO,
                "LOG.RDK.SI",
                "<mpe_siNotifyTableChanged> called...with tableType: OOB_SVCT (VCM), changeType: %d\n",
                changeType);
        if (changeType == RMF_SI_CHANGE_TYPE_ADD)
        {
            event = RMF_SI_EVENT_OOB_VCT_ACQUIRED;
        }
        else if (changeType == RMF_SI_CHANGE_TYPE_MODIFY)
        {
            event = RMF_SI_EVENT_OOB_VCT_ACQUIRED;
            break;
        }
        else if (changeType == RMF_SI_CHANGE_TYPE_REMOVE)
        {
            // No support for removal of SVCT
        }
    }
        break;
    case OOB_SVCT_DCM:
    {
        RDK_LOG(
                RDK_LOG_INFO,
                "LOG.RDK.SI",
                "<mpe_siNotifyTableChanged> called...with tableType: OOB_SVCT (DCM), changeType: %d\n",
                changeType);
        if (changeType == RMF_SI_CHANGE_TYPE_ADD)
        {
            event = RMF_SI_EVENT_OOB_VCT_ACQUIRED;
        }
        else if (changeType == RMF_SI_CHANGE_TYPE_MODIFY)
        {
            event = RMF_SI_EVENT_OOB_VCT_ACQUIRED;
            break;
        }        
        else if (changeType == RMF_SI_CHANGE_TYPE_REMOVE)
        {
            // No support for removal of SVCT
        }
    }
        break;
    case OOB_NTT_SNS:
    {
        RDK_LOG(
                RDK_LOG_INFO,
                "LOG.RDK.SI",
                "<mpe_siNotifyTableChanged> called...with tableType: OOB_NTT_SNS, changeType: %d\n",
                changeType);
        switch (changeType)
        {
        case RMF_SI_CHANGE_TYPE_ADD:
        {
            break;
        }
        case RMF_SI_CHANGE_TYPE_MODIFY:
        {
            break;
        }
        case RMF_SI_CHANGE_TYPE_REMOVE:
        { // No support for removal of SVCT
            break;
        }
        default:
        {
            RDK_LOG(
                    RDK_LOG_ERROR,
                    "LOG.RDK.SI",
                    "mpe_siNotifyTableChanged: Unknown SVCT changeType (%d)!\n",
                    changeType);
            break;
        }
        } // END switch (changeType)
        break;
    }
    case OOB_NIT_CDS:
    {
                RDK_LOG(
                RDK_LOG_INFO,
                "LOG.RDK.SI",
                "<mpe_siNotifyTableChanged> called...with tableType: OOB_NIT_CDS, changeType: %d\n",
                changeType);

        m_pSiCache->SortCDSFreqList();

#if 0
        if (changeType == RMF_SI_CHANGE_TYPE_ADD)
        {
            rmf_osal_timeGetMillis(&gtime_transport_stream);
        }
        else if (changeType == RMF_SI_CHANGE_TYPE_MODIFY)
        {

            rmf_osal_timeGetMillis(&gtime_transport_stream);
            //update_transport_stream_handles();
        }
        else if (changeType == RMF_SI_CHANGE_TYPE_REMOVE)
        {

        }
#endif

        event = RMF_SI_EVENT_OOB_NIT_ACQUIRED;
    }
        break;
    case OOB_NIT_MMS:
    {
        RDK_LOG(
                RDK_LOG_INFO,
                "LOG.RDK.SI",
                "<%s: %s> called...with tableType: OOB_NIT_MMS, changeType: %d\n",
                SIMODULE, __FUNCTION__, changeType);
        if (changeType == RMF_SI_CHANGE_TYPE_ADD)
        {
        }
        else if (changeType == RMF_SI_CHANGE_TYPE_MODIFY)
        {
            //update_transport_stream_handles();
        }
        else if (changeType == RMF_SI_CHANGE_TYPE_REMOVE)
        {

        }
    }
        break;

    default:
        event = 0;
        break;
    }

    if (event == 0)
        return RMF_SI_SUCCESS;

    send_si_event(event, optional_data, optionalEventData3);

    return RMF_SI_SUCCESS;
}

/**
 * @brief This function gets the program information of the specified vcn from SI cache. The program information
 * includes frequency, modulation, program number and the service handle.
 *
 * @param[in] vcn Indicates the virtual channel number, against which the program information needs to
 * to be populated.
 * @param[out] pProgramInfo Pointer to populate the program information.
 *
 * @return Return values indicates the success or failure of the call.
 * @retval RMF_SUCCESS Indicates the call is successful and the program information is fetched.
 * @retval RMF_OSAL_EINVAL Indicates the input parameter pProgramInfo is NULL.
 * @retval RMF_SI_INVALID_PARAMETER Indicates invalid parameters passed to the functions which are called
 * internally.
 * @retval RMF_SI_NOT_AVAILABLE_YET Indicates global SI state is in SI_ACQUIRING or SI_NOT_AVAILABLE_YET.
 * @retval RMF_SI_NOT_AVAILABLE Indicates global SI state is in SI_DISABLED.
 * @retval RMF_SI_NOT_FOUND Indicates any of the following condition.
 * <li> Indicates the entry corresponding to sourceId is not present in the list of services or
 * <li> The program is not mapped in VCM or DCT.
 * <li> The TS handle corresponding to sourceId is invalid.
 */
rmf_Error rmf_OobSiMgr::GetProgramInfoByVCN(uint16_t vcn, Oob_ProgramInfo_t *pProgramInfo)
{
    rmf_SiServiceHandle service_handle;
    rmf_Error ret = RMF_SUCCESS;

    if(pProgramInfo == NULL)
    {
        return RMF_OSAL_EINVAL;
    }

    m_pSiCache->LockForRead();
   
    RDK_LOG(
            RDK_LOG_INFO,
            "LOG.RDK.SI",
            "<%s: %s> called... with vcn: 0x%x\n",
             SIMODULE, __FUNCTION__, vcn);
 
    ret = m_pSiCache->GetServiceHandleByVCN(vcn, &service_handle);
    if(ret != RMF_SUCCESS)
    {
        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SI", "<%s: %s> GetServiceEntryByVCN failed: %d\n",
             SIMODULE, __FUNCTION__, ret );
    	m_pSiCache->UnLock();
        return ret;
    }

    pProgramInfo->service_handle = service_handle;
    RDK_LOG(
            RDK_LOG_INFO,
            "LOG.RDK.SI",
            "<%s: %s> called... with vcn: 0x%x, service_handle: 0x%x\n",
             SIMODULE, __FUNCTION__, vcn, service_handle);
    ret = m_pSiCache->GetFrequencyForServiceHandle(service_handle, &(pProgramInfo->carrier_frequency));
    if(ret != RMF_SUCCESS)
    {
        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SI", "<%s: %s> GetFrequencyForServiceHandle failed: %d\n",
             SIMODULE, __FUNCTION__, ret );
        m_pSiCache->UnLock();
        return ret;
    }

    RDK_LOG(
            RDK_LOG_INFO,
            "LOG.RDK.SI",
            "<%s: %s> called... with vcn: 0x%x, pProgramInfo->carrier_frequency: %d\n",
             SIMODULE, __FUNCTION__, vcn, pProgramInfo->carrier_frequency);
    ret = m_pSiCache->GetModulationFormatForServiceHandle(service_handle, &(pProgramInfo->modulation_mode));
    if(ret != RMF_SUCCESS)
    {
        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SI", "<%s: %s> GetModulationFormatForServiceHandle failed: %d\n",
             SIMODULE, __FUNCTION__, ret );
    	m_pSiCache->UnLock();
        return ret;
    }

     RDK_LOG(
            RDK_LOG_INFO,
            "LOG.RDK.SI",
            "<%s: %s> called... with vcn: 0x%x, pProgramInfo->modulation_mode: %d\n",
             SIMODULE, __FUNCTION__, vcn, pProgramInfo->modulation_mode);
    ret = m_pSiCache->GetProgramNumberForServiceHandle(service_handle, &(pProgramInfo->prog_num));
    if(ret != RMF_SUCCESS)
    {
        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SI", "<%s: %s> GetProgramNumberForServiceHandle failed: %d\n",
             SIMODULE, __FUNCTION__, ret );
    	m_pSiCache->UnLock();
        return ret;
    }

    RDK_LOG(
            RDK_LOG_INFO,
            "LOG.RDK.SI",
            "<%s: %s> called... with vcn: 0x%x, pProgramInfo->prog_num: %d\n",
             SIMODULE, __FUNCTION__, vcn, pProgramInfo->prog_num);

    m_pSiCache->UnLock();
    return ret;
}

/**
 * @brief This function gets the program information of the specified sourceId from SI cache. The program information
 * includes frequency, modulation, program number and the service handle.
 *
 * @param[in] sourceId Indicates unique value given to a service, against which the program information needs to
 * to be populated.
 * @param[out] pProgramInfo Pointer to populate the program information.
 *
 * @return Return values indicates the success or failure of the call.
 * @retval RMF_SUCCESS Indicates the call is successful and the program information is fetched.
 * @retval RMF_OSAL_EINVAL Indicates the input parameter pProgramInfo is NULL.
 * @retval RMF_SI_INVALID_PARAMETER Indicates invalid parameters passed to the functions which are called
 * internally.
 * @retval RMF_SI_NOT_AVAILABLE_YET Indicates global SI state is in SI_ACQUIRING or SI_NOT_AVAILABLE_YET.
 * @retval RMF_SI_NOT_AVAILABLE Indicates global SI state is in SI_DISABLED.
 * @retval RMF_SI_NOT_FOUND Indicates any of the following condition.
 * <li> Indicates the entry corresponding to sourceId is not present in the list of services or
 * <li> The program is not mapped in VCM or DCT.
 * <li> The TS handle corresponding to sourceId is invalid.
 */
rmf_Error rmf_OobSiMgr::GetProgramInfo(uint32_t sourceId, Oob_ProgramInfo_t *pProgramInfo)
{
    rmf_SiServiceHandle service_handle;
    rmf_Error ret = RMF_SUCCESS;

    if(pProgramInfo == NULL)
    {
        return RMF_OSAL_EINVAL;
    }

    m_pSiCache->LockForRead();
   
    RDK_LOG(
            RDK_LOG_INFO,
            "LOG.RDK.SI",
            "<%s: %s> called... with sourceid: 0x%x\n",
             SIMODULE, __FUNCTION__, sourceId);
 
    ret = m_pSiCache->GetServiceHandleBySourceId(sourceId, &service_handle);
    if(ret != RMF_SUCCESS)
    {
        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SI", "<%s: %s> GetServiceEntryFromSourceId failed: %d\n",
             SIMODULE, __FUNCTION__, ret );
    	m_pSiCache->UnLock();
        return ret;
    }

    pProgramInfo->service_handle = service_handle;
    RDK_LOG(
            RDK_LOG_INFO,
            "LOG.RDK.SI",
            "<%s: %s> called... with sourceid: 0x%x, service_handle: 0x%x\n",
             SIMODULE, __FUNCTION__, sourceId, service_handle);
    ret = m_pSiCache->GetFrequencyForServiceHandle(service_handle, &(pProgramInfo->carrier_frequency));
    if(ret != RMF_SUCCESS)
    {
        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SI", "<%s: %s> GetFrequencyForServiceHandle failed: %d\n",
             SIMODULE, __FUNCTION__, ret );
        m_pSiCache->UnLock();
        return ret;
    }

    RDK_LOG(
            RDK_LOG_INFO,
            "LOG.RDK.SI",
            "<%s: %s> called... with sourceid: 0x%x, pProgramInfo->carrier_frequency: %d\n",
             SIMODULE, __FUNCTION__, sourceId, pProgramInfo->carrier_frequency);
    ret = m_pSiCache->GetModulationFormatForServiceHandle(service_handle, &(pProgramInfo->modulation_mode));
    if(ret != RMF_SUCCESS)
    {
        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SI", "<%s: %s> GetModulationFormatForServiceHandle failed: %d\n",
             SIMODULE, __FUNCTION__, ret );
    	m_pSiCache->UnLock();
        return ret;
    }

     RDK_LOG(
            RDK_LOG_INFO,
            "LOG.RDK.SI",
            "<%s: %s> called... with sourceid: 0x%x, pProgramInfo->modulation_mode: %d\n",
             SIMODULE, __FUNCTION__, sourceId, pProgramInfo->modulation_mode);
    ret = m_pSiCache->GetProgramNumberForServiceHandle(service_handle, &(pProgramInfo->prog_num));
    if(ret != RMF_SUCCESS)
    {
        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SI", "<%s: %s> GetProgramNumberForServiceHandle failed: %d\n",
             SIMODULE, __FUNCTION__, ret );
    	m_pSiCache->UnLock();
        return ret;
    }

    RDK_LOG(
            RDK_LOG_INFO,
            "LOG.RDK.SI",
            "<%s: %s> called... with sourceid: 0x%x, pProgramInfo->prog_num: %d\n",
             SIMODULE, __FUNCTION__, sourceId, pProgramInfo->prog_num);

    m_pSiCache->UnLock();
    return ret;
}

/**
 * @brief This function gets the DAC ID. It first gets the service handle of the DAC_SOURCEID.
 * Then using this service handle, it gets the virtual channel number of the DAC.
 *
 * @param[out] pDACId Pointer to populate the DAC ID.
 *
 * @return Return value indicates success or failure of the call.
 * @retval RMF_SUCCESS Indicates the virtual channel number is populated and the call is successful.
 * @retval RMF_OSAL_EINVAL Indicates, the input parameter pDACId is pointing to NULL.
 * @retval RMF_OSAL_ENODATA Indicates any of the below conditions.
 * <li> Indicates the entry corresponding to DAC_SOURCEID is not present in the list of services or
 * <li> The program is not mapped in VCM or DCT and the SI is not up.
 * @retval RMF_SI_INVALID_PARAMETER Indicates the parameters passed to internal function calls are
 * invalid.
 * @retval RMF_SI_NOT_AVAILABLE_YET Indicates global SI state is in SI_ACQUIRING or SI_NOT_AVAILABLE_YET.
 * @retval RMF_SI_NOT_AVAILABLE Indicates global SI state is in SI_DISABLED.
 */
rmf_Error rmf_OobSiMgr::GetDACId(uint16_t *pDACId)
{
	rmf_Error ret = RMF_SUCCESS;

	if( pDACId == NULL)
	{
	    return RMF_OSAL_EINVAL;
	}
	
	m_pSiCache->GetDACId( pDACId );
	if( *pDACId == VIRTUAL_CHANNEL_UNKNOWN )
	{	
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SI", "<%s: %s> : SI is not up ...! \n",
					SIMODULE, __FUNCTION__ );		
		ret = RMF_OSAL_ENODATA;
	}

	return ret;
}

/**
 * @brief This function gets the channel map ID from OOB SI cache.
 *
 * @param[out] pChannelMapId Pointer to populate the channel map ID.
 *
 * @return Return value indicates success or failure of the call.
 * @retval RMF_SUCCESS Indicates the channel map ID is populated and the call is successful.
 * @retval RMF_OSAL_ENODATA Indicates the SI is not up and channel map id contains invalid value.
 */
rmf_Error rmf_OobSiMgr::GetChannelMapId(uint16_t *pChannelMapId)
{
	rmf_Error ret = RMF_SUCCESS;

	if(pChannelMapId == NULL)
	{
	    return RMF_OSAL_EINVAL;
	}
	
	m_pSiCache->GetChannelMapId( pChannelMapId );
	if( *pChannelMapId == VIRTUAL_CHANNEL_UNKNOWN )
	{	
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SI", "<%s: %s> : SI is not up ...! \n",
					SIMODULE, __FUNCTION__ );		
		ret = RMF_OSAL_ENODATA;
	}

	return ret;
}

/**
 * @brief This function is used to register the specified call back function to outband SI cache.
 * The OOB SI cache uses this function to signal the global state of the SI tables like STT is acquired,
 * VCT ID is changed, SI is fully acquired and so on.
 *
 * @param[in] Indicates the function to be registered.
 *
 * @return Returns RMF_SUCCESS after registering the func in OOB SI cache.
 */
rmf_Error rmf_OobSiMgr::rmf_SI_registerSIGZ_Callback(rmf_siGZCallback_t func)
{
    return rmf_OobSiCache::rmf_SI_registerSIGZ_Callback( func );    
}

/**
 * @brief This function gets the VCT (Virtual Channel Table) Id of the SVCT table.
 *
 * @param[out] pVCTId Pointer to populate the VCT ID.
 *
 * @return Returns RMF_SUCCESS indicating VCT ID is successfully populated.
 */
rmf_Error rmf_OobSiMgr::GetVCTId( uint16_t *pVCTId )
{
	m_pSiCache->LockForRead(  );
	m_pSiCache->GetVCTId( pVCTId );
	m_pSiCache->UnLock();
	return RMF_SUCCESS;
}

/**
 * @brief This function gets the virtual channel number corresponding to the specified sourceId.
 * It first gets the service handle corresponding to the specified sourceId, then using
 * rmf_SiTableEntry of this service handle, it gets the virtual channel number.
 *
 * @param[in] sourceId Indicates unique value given to a service against which the virtual channel
 * number is required.
 * @param[out] pVCN Pointer to populate the virtual channel number.
 *
 * @return Return value indicates success or failure of the call.
 * @retval RMF_SUCCESS Indicates the virtual channel number against the sourceId is found.
 * @retval RMF_OSAL_EINVAL Indicates, the parameter pVCN is pointing to NULL.
 * @retval RMF_SI_INVALID_PARAMETER Indicate parameters passed to functions which are called
 * internally are invalid.
 * @retval RMF_SI_NOT_AVAILABLE_YET Indicates global SI state is in SI_ACQUIRING or SI_NOT_AVAILABLE_YET.
 * @retval RMF_SI_NOT_AVAILABLE Indicates global SI state is in SI_DISABLED.
 * @retval RMF_OSAL_ENODATA Indicates any of the following condition.
 * <li> Indicates the entry corresponding to sourceId is not present in the list of services or
 * <li> The program is not mapped in VCM or DCT.
 */
rmf_Error rmf_OobSiMgr::GetVirtualChannelNumber(
                uint32_t sourceId, uint16_t *pVCN)
{
    rmf_SiServiceHandle service_handle;
    rmf_Error ret = RMF_SUCCESS;

    if( pVCN == NULL)
    {
        return RMF_OSAL_EINVAL;
    }

    m_pSiCache->LockForRead();

    RDK_LOG(
            RDK_LOG_INFO,
            "LOG.RDK.SI",
            "<%s: %s> called..\n",
             SIMODULE, __FUNCTION__);

    ret = m_pSiCache->GetServiceHandleBySourceId(sourceId, &service_handle);
    if(ret != RMF_SUCCESS)
    {
        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SI", "<%s: %s> GetServiceEntryFromSourceId() failed: %d\n",
             SIMODULE, __FUNCTION__, ret );
        m_pSiCache->UnLock();
        if(RMF_SI_NOT_FOUND == ret)
            return RMF_OSAL_ENODATA;
        return ret;
    }

    ret = m_pSiCache->GetVirtualChannelNumberForServiceHandle(service_handle, pVCN);
    if(ret != RMF_SUCCESS)
    {
        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SI", "<%s: %s> GetVirtualChannelNumberForServiceHandle() failed: %d\n",
             SIMODULE, __FUNCTION__, ret );
    }
    else
    {
        RDK_LOG( RDK_LOG_INFO, "LOG.RDK.SI", "<%s: %s> VCN: %d\n",
             SIMODULE, __FUNCTION__, *pVCN);
    }

    if( *pVCN == VIRTUAL_CHANNEL_UNKNOWN )
    {
        RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.SI", "<%s: %s> : SI is not up ...! \n",
             SIMODULE, __FUNCTION__ );		
        ret = RMF_OSAL_ENODATA;
    }

    m_pSiCache->UnLock();
    return ret;
}


/**
 * Tells the SI parser that the data is coming from a local
 * cache, not an actual headend
 *
 */
void rmf_OobSiMgr::sitp_si_cache_enabled(void)
{
    g_sitp_si_max_nit_cds_wait_time = ((g_sitp_si_max_nit_cds_wait_time > 60000) ?  g_sitp_si_max_svct_dcm_wait_time : 60000);
    g_sitp_si_max_nit_mms_wait_time = ((g_sitp_si_max_nit_mms_wait_time > 60000) ?  g_sitp_si_max_svct_vcm_wait_time : 60000);
        g_sitp_si_max_svct_dcm_wait_time = ((g_sitp_si_max_svct_dcm_wait_time > 1800000) ?  g_sitp_si_max_svct_dcm_wait_time : 1800000);
        g_sitp_si_max_svct_vcm_wait_time = ((g_sitp_si_max_svct_vcm_wait_time > 1800000) ?  g_sitp_si_max_svct_vcm_wait_time : 1800000);
        g_sitp_si_max_ntt_wait_time  = ((g_sitp_si_max_ntt_wait_time  > 1800000) ?  g_sitp_si_max_ntt_wait_time  : 1800000);
        RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI", "CACHE: Set SI Wait time to [dcm=%d][vcm=%d][ntt=%d]\r\n", g_sitp_si_max_svct_dcm_wait_time, g_sitp_si_max_svct_vcm_wait_time,g_sitp_si_max_ntt_wait_time);
}

void rmf_OobSiMgr::setSVCTAcquired(uint32_t svct_acquired)
{
        g_sitp_svctAcquired = svct_acquired;
}

uint32_t rmf_OobSiMgr::getSVCTAcquired(void)
{
        return g_sitp_svctAcquired;
}

#ifdef ENABLE_TIME_UPDATE

/**
 * @brief This function gets the STT (System Time Table) acquired time in milliseconds only if the STT was acquired already.
 *
 * @param[out] pSysTime Pointer to populate the current time in milliseconds.
 *
 * @return Return value indicates success or failure of the call.
 * @retval RMF_SUCCESS Indicates the current time is populated and the call is successful.
 * @retval RMF_OSAL_EINVAL Indicates the parameter pSysTime points to NULL.
 * @retval RMF_OSAL_ENODATA Indicates STT is not acquired.
 */
rmf_Error rmf_OobSiMgr::GetSysTime(rmf_osal_TimeMillis *pSysTime)
{
    rmf_Error ret = RMF_SUCCESS;

    if( pSysTime == NULL)
    {
        return RMF_OSAL_EINVAL;
    }

    m_pSiCache->LockForRead();

    if(m_pSiCache->IsSttAcquired())
    {
	*pSysTime = m_pSiCache->getSTTStartTime();
    }
    else
        ret = RMF_OSAL_ENODATA;

    m_pSiCache->UnLock();

    return ret;
}

/**
 * @brief This function gets the time zone in hours and the day light saving information.
 * Time zone values can range from -12 to +13 hours.
 *
 * @param[out] pTimezoneinhours Pointer to populate time zone in hours.
 * @param[out] pDaylightflag Pointer to daylight flag. 0 indicates, not in daylight saving time and 1 indicates,
 * in daylight saving time.
 *
 * @return Return value indicates success or failure of the call.
 * @retval RMF_SUCCESS Indicates the call was successful.
 * @retval RMF_OSAL_EINVAL Indicates any one or both of the input parameters are pointing to NULL.
 * @retval RMF_OSAL_ENODATA Indicates STT is not acquired.
 */
rmf_Error rmf_OobSiMgr::GetTimeZone(int32_t *pTimezoneinhours, int32_t *pDaylightflag)
{
    rmf_Error ret = RMF_SUCCESS;

    if( (pTimezoneinhours == NULL) || (pDaylightflag == NULL) )
    {
        return RMF_OSAL_EINVAL;
    }

    m_pSiCache->LockForRead();

    if(m_pSiCache->IsSttAcquired())
    {
	ret = m_pSiCache->getSTTTimeZone(pTimezoneinhours, pDaylightflag);
    }
    else
        ret = RMF_OSAL_ENODATA;

    m_pSiCache->UnLock();

    return ret;
}
#endif
