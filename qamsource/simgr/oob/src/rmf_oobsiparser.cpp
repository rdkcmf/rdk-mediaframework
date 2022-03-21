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
 * @file rmf_oobsiparser.cpp
 */

#include <stdint.h>
#include <string.h>

#include "rdk_debug.h"
#include "rmf_osal_error.h"
#include "rmf_osal_types.h"
#include "rmf_osal_time.h"
#include "rmf_osal_thread.h"
#include "rmf_osal_event.h"
#include "rmf_osal_file.h"
#include "rmf_sectionfilter.h"
#include "rmf_oobsicache.h"
#include "rmf_oobsiparser.h"
#ifdef GLI
#include "sysMgr.h"
#include "libIBus.h"
#endif
#if USE_SYSRES_MLT
#include "rpl_malloc.h"
#include "rpl_new.h"
#endif

rmf_OobSiParser *rmf_OobSiParser::m_gpParserInst = NULL;

rmf_OobSiParser::rmf_OobSiParser()
{
    m_pSectionFilter = NULL;
    m_pSiCache = NULL;
}

rmf_OobSiParser::rmf_OobSiParser(rmf_SectionFilter *pSectionFilter)
{
    m_pSectionFilter = pSectionFilter;
    m_pSiCache = rmf_OobSiCache::getSiCache();
}

/*
 ** *****************************************************************************
 ** Subroutine/Method:                       parseAndUpdateSVCT()
 ** *****************************************************************************
 **
 ** Parse SVCT table section and update SI database.
 **
 ** @param  ptr_TableSection,  a pointer to the start of the NIT table section.
 ** @param  sectiontable_size, The number of bytes in the table section per
 **         the PowerTV OS.
 **
 ** @return RMF_SI_SUCCESS if table section was successfully processed/parsed .
 ** Otherwise an error code greater than zero (RMF_SI_SUCCESS).
 ** *****************************************************************************
 */
rmf_Error rmf_OobSiParser::parseAndUpdateSVCT(rmf_FilterSectionHandle section_handle,
        uint8_t *version, uint8_t *section_number,
        uint8_t *last_section_number, uint32_t *crc)
{

    rmf_Error retCode = RMF_SI_SUCCESS;
    svct_table_struc1 *ptr_table_struc1 = NULL;
    revision_detection_descriptor_struc * ptr_rdd_table_struc1 = NULL;
    int32_t section_length = 0;
    uint16_t table_subtype = 0;
    static uint16_t prev_vct_id = 0;	
    uint16_t vct_id = 0;
	
    uint32_t number_bytes_copied = 0;
    generic_descriptor_struct *ptr_descriptor_struc;
    rmf_Error svct_error = NO_PARSING_ERROR;
    uint8_t input_buffer[MAX_OOB_SMALL_TABLE_SECTION_LENGTH];
    uint8_t *ptr_input_buffer = NULL;

    //***********************************************************************************************
    //***********************************************************************************************

    //**********************************************************************************************
    // Parse and validate the first 56 bytes (Table_ID thru Last_Section_Number)of the scvt
    // table (see document ANSI/SCTE 65-2002 (formerly DVS 234), chap 5.1.1,pg.15) .
    //**********************************************************************************************
    RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
            "<%s> - Entered method parseAndUpdateSVCT.\n",
            __FUNCTION__);
    RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
            "<%s> - section_handle = (%u).\n", __FUNCTION__,
            section_handle);
    //    Dump_Utility("parseAndUpdateSVCT", sectiontable_size, ptr_TableSection) ;

//    retCode = mpeos_filterSectionRead(section_handle, 0,
  //          sizeof(svct_table_struc1), 0, input_buffer,
    //        (uint32_t *) &number_bytes_copied);

    retCode = m_pSectionFilter->ReadSection(section_handle, 0,
            sizeof(svct_table_struc1), 0, input_buffer,
            (uint32_t *) &number_bytes_copied);

    if ((RMF_SI_SUCCESS != retCode) || (number_bytes_copied
            != sizeof(svct_table_struc1)))
    {
        svct_error = 1;
        goto PARSING_DONE;
    }
    else
    {
        ptr_input_buffer = input_buffer;
        ptr_table_struc1 = (svct_table_struc1 *) ptr_input_buffer;

        //**** validate table Id field ****
        if (ptr_table_struc1->table_id_octet1 != SF_VIRTUAL_CHANNEL_TABLE_ID)
        {
            svct_error = 2;
        }
        //**** validate "Zero" (2-bits) field ****
        if (TESTBITS(ptr_table_struc1->section_length_octet2, 8, 7))
        {
            // bits not set to 0.
            //svct_error = 3 ;
            RDK_LOG(
                    RDK_LOG_TRACE6,
                    "LOG.RDK.SI",
                    "<%s> - *** WARNING *** 2-Bit-Zero Field #1 not set to all zeroes(0)\n.",
                    __FUNCTION__);
        }
        //**** validate 2 reserved bits ****
        if (GETBITS(ptr_table_struc1->section_length_octet2, 6, 5) != 3)
        {
            // bits 5 and 6 are not set to 1 per ANSI/SCTE 65-2002,pg.12, para 4.3
            //svct_error = 4 ;
            RDK_LOG(
                    RDK_LOG_TRACE6,
                    "LOG.RDK.SI",
                    "<%s> - *** WARNING *** 2-Bit-Reserved Field #2 not set to all ones(1)\n.",
                    __FUNCTION__);
        }
        //**** extract and validate section_length field (octets 2 & 3)         ****
        //**** note, the maximum size of a scvt excluding octet 1 thru 3 is 1021.****
        //**** and occupies only bits 1 thru of the 12 bits                     ****

        section_length = ((GETBITS(ptr_table_struc1->section_length_octet2, 4,
                1)) << 8) | ptr_table_struc1->section_length_octet3;
        RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
                "<%s> - section_length2 = (%d).\n",
                __FUNCTION__, section_length);

        if (section_length > 1021)
        { // warning, section_length too large.
            // NOTE: SVCTs larger than 1024 have been seen on test networks.
            // Since length is 12 bits - and we have no dependancy on it being smaller - just warn
            RDK_LOG(
                    RDK_LOG_TRACE6,
                    "LOG.RDK.SI",
                    "<%s> - *** WARNING *** Section length larger than 1021\n.",
                    __FUNCTION__);
        }

        //**** verify that the "Zero" (3-bits) and the Protocol Version fields ****
        //**** (5-bits) are set to zero. -                                     ****
        if (ptr_table_struc1->protocol_version_octet4 != 0)
        {
            // one or more of the eight bits are set to 1.
            //svct_error = 6 ;
            RDK_LOG(
                    RDK_LOG_TRACE6,
                    "LOG.RDK.SI",
                    "<%s> - *** WARNING *** 8-Bit-Zero Field #3 not set to all zeroes(0)\n.",
                    __FUNCTION__);
        }

        //**** extract and validate the transmission_medium ****
        if (TESTBITS(ptr_table_struc1->table_subtype_octet5, 8, 5) != 0)
        {
            // bits 8 thru 5 shall be set to zero (0) per ANSI/SCTE 65-2002,pg.14, para 5.1
            svct_error = 9;
        }

        //**** extract and validate the table_subtype field ****
        table_subtype = (uint16_t)(GETBITS(
                ptr_table_struc1->table_subtype_octet5, 4, 1));
        RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
                "<%s> - table_subtype = (%d).\n", __FUNCTION__,
                table_subtype);

        if (table_subtype > INVERSE_CHANNEL_MAP)
        {
            // error, invalid table_subtype value.
            svct_error = 10;
        }

        //**** extract the 16-bit vct_id ****
        vct_id = (uint16_t)((ptr_table_struc1->vct_id_octet6 << 8)
                | ptr_table_struc1->vct_id_octet7);

        RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
                "<%s> - vct_id = (%d).\n", __FUNCTION__, vct_id);
        if( vct_id != prev_vct_id )
        {
		prev_vct_id = vct_id;
        	m_pSiCache->SetVCTId( vct_id );
		if( rmf_OobSiCache::rmf_siGZCBfunc )
		{
			rmf_OobSiCache::rmf_siGZCBfunc( RMF_OOBSI_GLI_VCT_ID_CHANGED, vct_id, 
				((m_pSiCache->IsSIFullyAcquired() == TRUE ) ?  1 : 0 ) );
		}
        }
        //**** section_length identifies the total number of bytes/octets remaining ****
        //**** in the scvt after octet3. Before processing the VCM/DCM/ICM record,  ****
        //**** adjust section_length to account for having processed octets4 thru   ****
        //**** octet7 and account for the future processing of the CRC_32 field/    ****
        //**** octets. Adjust ptr_TableSection to account for the header processed  ****
        //**** data.                                                                ****

        section_length -= (sizeof(svct_table_struc1) - LEN_OCTET1_THRU_OCTET3);
        ptr_input_buffer += sizeof(svct_table_struc1);

        RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
                "<%s> - section_length3 = (%d).\n",
                __FUNCTION__, section_length);
        RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
                "<%s> - ptr_input_buffer = (%p).\n",
                __FUNCTION__, ptr_input_buffer);

        if ((svct_error == NO_PARSING_ERROR)
                && (section_length < LENGTH_CRC_32))
        { // error, either a portion of header is missing, or all or a portion of the
            // CRC_32 value is missing.
            svct_error = 11;
        }
    }

    //**********************************************************************************************
    // Parse the remaining bytes/octets in the scvt.
    //**********************************************************************************************

    if (svct_error == NO_PARSING_ERROR)
    {
//        retCode = mpeos_filterSectionRead(section_handle, 7, section_length, 0,
  //              input_buffer, (uint32_t *) &number_bytes_copied);

    retCode = m_pSectionFilter->ReadSection(section_handle, 7, section_length, 0,
                input_buffer, (uint32_t *) &number_bytes_copied);

        if ((RMF_SI_SUCCESS != retCode) || (number_bytes_copied != section_length))
        {
            svct_error = 12;
            goto PARSING_DONE;
        }
        //****************************************************************************************
        //**** Process the VCM/DCM/ICM record .
        //****************************************************************************************

        ptr_input_buffer = input_buffer;
        RDK_LOG(
                RDK_LOG_TRACE4,
                "LOG.RDK.SI",
                "<%s> - before vcm/dcm parse call - ptr_input_buffer = (0x%p bytes copied: %d\n",
                __FUNCTION__, ptr_input_buffer, number_bytes_copied);
        if (table_subtype == VIRTUAL_CHANNEL_MAP)
        {
            svct_error = parseVCMStructure(&ptr_input_buffer, &section_length);
            // if svct_error == NO_PARSING_ERROR, ptr_input_buffer has been properly updated
        }
        else if (table_subtype == DEFINED_CHANNEL_MAP)
        {
            // table_subtype is DEFINED_CHANNEL_MAP
            svct_error = parseDCMStructure(&ptr_input_buffer, &section_length);
        }
        else
        {
            // table_subtype is INVERSE_CHANNEL_MAP,
            // ignore this type for now
            svct_error = 13;
        }

    }

    RDK_LOG(
            RDK_LOG_TRACE4,
            "LOG.RDK.SI",
            "<%s> - sectionlength after exit from parseVCMStructure records: %d, ptr: 0x%p.\n",
            __FUNCTION__, section_length, ptr_input_buffer);

    //*************************************************************************************************
    //* At this point, there may be a series a descriptors present.However, we are not required to
    //* process them for the 06-07-04 PowerTV release. If there are descriptors present, we will
    //* "ignore" them.
    //*************************************************************************************************
    if ((svct_error == NO_PARSING_ERROR) && (section_length > LENGTH_CRC_32))
    {
        ptr_rdd_table_struc1
                = (revision_detection_descriptor_struc *) ptr_input_buffer;

        if (ptr_rdd_table_struc1->descriptor_tag_octet1 == 0x93)
        {
            *version = (ptr_rdd_table_struc1->table_version_number_octet3
                    & 0x1F);
            *section_number = ptr_rdd_table_struc1->section_number_octet4;
            *last_section_number
                    = ptr_rdd_table_struc1->last_section_number_octet5;
            RDK_LOG(
                    RDK_LOG_TRACE4,
                    "LOG.RDK.SI",
                    "<%s> - version: %d, section_number: %d, last_section_number: %d\n",
                    __FUNCTION__, *version, *section_number, *last_section_number);
            ptr_input_buffer += sizeof(revision_detection_descriptor_struc);
            section_length -= sizeof(revision_detection_descriptor_struc);
        }
    }

    if (section_length > LENGTH_CRC_32)
    {
        do
        {
            ptr_descriptor_struc
                    = (generic_descriptor_struct *) ptr_input_buffer;
            ptr_input_buffer += sizeof(generic_descriptor_struct)
                    + ptr_descriptor_struc->descriptor_length_octet2;
            section_length -= (sizeof(generic_descriptor_struct)
                    + ptr_descriptor_struc->descriptor_length_octet2);

            if (section_length <= 0)
            {
                if (section_length < 0)
                { //error, section_length not large enough to accommodate descriptor(s) .
                    svct_error = 90;
                }
                break;
            }
        } while (section_length > LENGTH_CRC_32);
    }

    RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
            "<%s> - ptr_input_buffer = 0x%p\n", __FUNCTION__,
            ptr_input_buffer);
    if (section_length == LENGTH_CRC_32)
    {
        *crc = ((*ptr_input_buffer) << 24 | (*(ptr_input_buffer + 1)) << 16
                | (*(ptr_input_buffer + 2)) << 8 | (*(ptr_input_buffer + 3)));
        RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                "<%s> - CRC32 = 0x%08x\n", __FUNCTION__, *crc);
    }

    PARSING_DONE:

    if (svct_error == NO_PARSING_ERROR)
    {
        RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
                "<%s> - SVCT records parsed, set flag.\n",
                __FUNCTION__);

        //setTablesAcquired(getTablesAcquired() | SVCT_ACQUIRED) ;
    }

    /*
     ** **************************************************************************
     ** * At this point, if section_length = 0, the CRC_32 is present and the table section has been
     ** * processed successfully. A section_length > 0 or section_length < 0  implies that an error has
     ** * occurred and svct_error should identify the error condition.
     ** **************************************************************************
     */
    RDK_LOG(
            RDK_LOG_TRACE4,
            "LOG.RDK.SI",
            "<%s> - Exiting with a SVCT_Error_Code = (%d).\n",
            __FUNCTION__, svct_error);

    return svct_error;
}

//***************************************************************************************************
//  Subroutine/Method:                       parseAndUpdateNIT()
//***************************************************************************************************
/**
 * Parse NIT table section and update SI database.
 *
 * @param  ptr_TableSection,  a pointer to the start of the NIT table section.
 * @param  sectiontable_size, The number of bytes in the table section per the PowerTV OS.
 *
 * @return RMF_SUCCESS if table section was successfully processed/parsed .Otherwise an error code
 *         greater than zero (RMF_SUCCESS).
 */
rmf_Error rmf_OobSiParser::parseAndUpdateNIT(rmf_FilterSectionHandle section_handle,
        uint8_t *version, uint8_t *section_number,
        uint8_t *last_section_number, uint32_t *crc)
{
    rmf_Error retCode = RMF_SUCCESS;
    nit_table_struc1 *ptr_nit_table_struc1;
    generic_descriptor_struct *ptr_descriptor_struc;
    revision_detection_descriptor_struc *ptr_rdd_table_struc1 = NULL;

    uint16_t number_of_records = 0, table_subtype = 0, first_index = 0;
    uint32_t section_length = 0, number_bytes_copied = 0;
    rmf_Error nit_error = NO_PARSING_ERROR;
    uint8_t input_buffer[MAX_OOB_SMALL_TABLE_SECTION_LENGTH];
    uint8_t *ptr_input_buffer = NULL;

    //***********************************************************************************************
    //***********************************************************************************************

    //**********************************************************************************************
    // Parse and validate the first 56 bytes (Table_ID thru Last_Section_Number) of the NIT
    // table (see document ANSI/SCTE 65-2002 (formerly DVS 234), chap 5.1.1,pg.15) .
    //**********************************************************************************************
    RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
            "<%s::parseAndUpdateNIT> - Entered method parseAndUpdateNIT.\n",
            __FUNCTION__);

/*  retCode = mpeos_filterSectionRead(section_handle, 0,
            sizeof(nit_table_struc1), 0, input_buffer,
            (uint32_t *) &number_bytes_copied);
*/
    retCode = m_pSectionFilter->ReadSection(section_handle, 0,
                        sizeof(nit_table_struc1), 0, input_buffer,
                        (uint32_t *) &number_bytes_copied);

    if ((RMF_SUCCESS != retCode) || (number_bytes_copied
                != sizeof(nit_table_struc1)))
    {
        nit_error = 1;
        goto PARSING_DONE;
    }
    else
    {
        ptr_input_buffer = input_buffer;
        ptr_nit_table_struc1 = (nit_table_struc1 *) ptr_input_buffer;

        //**** validate table Id field ****
        if (ptr_nit_table_struc1->table_id_octet1
                != NETWORK_INFORMATION_TABLE_ID)
        {
            nit_error = 2;
        }
        //**** validate "Zero" (2-bits) field ****
        if (TESTBITS(ptr_nit_table_struc1->section_length_octet2, 8, 7))
        {
            // bits not set to 0.
            //nit_error = 3 ;
            RDK_LOG(
                    RDK_LOG_WARN,
                    "LOG.RDK.SI",
                    "<%s::parseAndUpdateNIT> - *** WARNING *** 2-Bit-Zero Field #1 not set to all zeroes(0)\n.",
                    __FUNCTION__);
        }
        //**** validate 2 reserved bits ****
        if (GETBITS(ptr_nit_table_struc1->section_length_octet2, 6, 5) != 3)
        {
            // bits 5 and 6 are not set to 1 per ANSI/SCTE 65-2002,pg.12, para 4.3
            //nit_error = 4 ;
            RDK_LOG(
                    RDK_LOG_WARN,
                    "LOG.RDK.SI",
                    "<%s::parseAndUpdateNIT> - *** WARNING *** 2-Bit-Reserve Field #2 not set to all ones(1)\n.",
                    __FUNCTION__);
        }
        //**** extract and validate section_length field (octets 2 & 3)         ****
        //**** note, the maximum size of a NIT excluding octet 1 thru 3 is 1021.****
        //**** and occupies only bits 1 thru of the 12 bits                     ****

        section_length = ((GETBITS(ptr_nit_table_struc1->section_length_octet2,
                        4, 1)) << 8) | ptr_nit_table_struc1->section_length_octet3;
        RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
                "<%s::parseAndUpdateNIT> - Section_Length = (%d)\n.", __FUNCTION__,
                section_length);

        if (section_length > 1021)
        { // warning, section_length too large.
            RDK_LOG(
                    RDK_LOG_WARN,
                    "LOG.RDK.SI",
                    "<%s::parseAndUpdateNIT> - *** WARNING *** Section length larger than 1021\n.",
                    __FUNCTION__);
        }

        //**** verify that the "Zero" (3-bits) and the Protocol Version fields ****
        //**** are set to zero.                                                ****

        if (ptr_nit_table_struc1->protocol_version_octet4 != 0)
        {
            // one or more of the eight bits are set to 1.
            nit_error = 6;

        }
        //**** validate the first_index field) *****

        if ((first_index = ptr_nit_table_struc1->first_index_octet5) == 0)
        {
            // error, must be > zero .
            nit_error = 7;
        }

        //**** number_of_records field ****

        number_of_records = ptr_nit_table_struc1->number_of_records_octet6;

        // Fix for bug 3251 (empty NIT table is allowed)
        /*
           if (number_of_records == 0)
           {
        // error, number_of_records must be > zero .
        nit_error = 8 ;
        }
         */
        RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                "<%s::parseAndUpdateNIT> - Number_Records = (%d).\n", __FUNCTION__,
                number_of_records);

        //**** extract and validate the transmission_medium ****

        if (TESTBITS(ptr_nit_table_struc1->table_subtype_octet7, 8, 5) != 0)
        {
            // bits 8 thru 5 shall be set to zero (0) per ANSI/SCTE 65-2002,pg.14, para 5.1
            nit_error = 9;
        }

        //**** extract and validate the table_subtype field ****

        table_subtype = (uint16_t)(GETBITS(
                    ptr_nit_table_struc1->table_subtype_octet7, 4, 1));
        RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                "<%s::parseAndUpdateNIT> - Table_Subtype = (%d).\n", __FUNCTION__,
                table_subtype);

        if ((table_subtype < CARRIER_DEFINITION_SUBTABLE) || (table_subtype
                    > MODULATION_MODE_SUBTABLE))
        {
            // error, invalid table_subtype value.
            nit_error = 10;
        }

        /*
         ** ** section_length identifies the total number of bytes/octets remaining **
         ** ** in the NIT after octet3. Before processing CDS/MMS records ,we must  **
         ** ** adjust section_length to account for having  processed octets4 thru  **
         ** ** octet7 and octet1 thru octet7, respectively.                         **
         */
        section_length -= (sizeof(nit_table_struc1) - LEN_OCTET1_THRU_OCTET3);
        RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
                "<%s::parseAndUpdateNIT> - section_length = (%d).\n", __FUNCTION__,
                section_length);

        /*
         ** read the rest of the table section
         */
        /*retCode = mpeos_filterSectionRead(section_handle, 7, section_length, 0,
                input_buffer, (uint32_t *) &number_bytes_copied);*/

        retCode = m_pSectionFilter->ReadSection(section_handle, 7, section_length, 0,
                input_buffer, (uint32_t *) &number_bytes_copied);

        if ((RMF_SUCCESS != retCode) || (number_bytes_copied != section_length))
        {
            nit_error = 11;
            goto PARSING_DONE;
        }
    }

    ptr_input_buffer = input_buffer;

    //**********************************************************************************************
    // Parse the remaining bytes/octets in the NIT.
    //**********************************************************************************************

    if (nit_error == NO_PARSING_ERROR)
    {
        //****************************************************************************************
        //**** Process each CDS or MMS Record
        //****************************************************************************************

        if (table_subtype == CARRIER_DEFINITION_SUBTABLE)
        {
            nit_error = parseCDSRecords(number_of_records, first_index,
                    &ptr_input_buffer, &section_length);
        }
        else
        {
            nit_error = parseMMSRecords(number_of_records, first_index,
                    &ptr_input_buffer, &section_length);
        }
    }

    //*************************************************************************************************
    //* At this point, there may be a series a descriptors present.However, we are not required to
    //* process them for the 06-07-04 PowerTV release. If there are descriptors present, we will
    //* "ignore" them.
    //*************************************************************************************************
    if ((nit_error == NO_PARSING_ERROR) && (section_length > LENGTH_CRC_32))
    {
        ptr_rdd_table_struc1
            = (revision_detection_descriptor_struc *) ptr_input_buffer;

        if (ptr_rdd_table_struc1->descriptor_tag_octet1 == 0x93)
        {
            RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
                    "<%s::parseAndUpdateNIT> - ptr to version: 0x%p\n",
                    __FUNCTION__,
                    &(ptr_rdd_table_struc1->table_version_number_octet3));
            *version = (uint8_t)(
                    ptr_rdd_table_struc1->table_version_number_octet3 & 0x1F);
            *section_number = ptr_rdd_table_struc1->section_number_octet4;
            *last_section_number
                = ptr_rdd_table_struc1->last_section_number_octet5;
            RDK_LOG(
                    RDK_LOG_TRACE4,
                    "LOG.RDK.SI",
                    "<%s::parseAndUpdateNIT> - version: %d, section_number: %d, last_section_number: %d\n",
                    __FUNCTION__, *version, *section_number, *last_section_number);
            ptr_input_buffer += sizeof(revision_detection_descriptor_struc);
            section_length -= sizeof(revision_detection_descriptor_struc);
        }
    }

    if (section_length > LENGTH_CRC_32)
    {
        do
        {
            uint32_t tempSize;

            ptr_descriptor_struc
                = (generic_descriptor_struct *) ptr_input_buffer;
            tempSize = sizeof(generic_descriptor_struct)
                + ptr_descriptor_struc->descriptor_length_octet2;
            ptr_input_buffer += tempSize;

            if (section_length < tempSize)
            { //error, section_length not large enough to accommodate descriptor(s) .
                nit_error = 90;
                break;
            }

            section_length -= tempSize;

        } while (section_length > LENGTH_CRC_32);
    }

    RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
            "<%s::parseAndUpdateNIT> - ptr = 0x%p\n", __FUNCTION__,
            ptr_input_buffer);
    if (section_length == LENGTH_CRC_32)
    {
        //RDK_LOG (RDK_LOG_TRACE6, "LOG.RDK.SI","<%s::parseAndUpdateNIT> - crc bytes= %x = %d,%x = %d,%x = %d,%x = %d\n",
        //     __FUNCTION__,
        //    );

        *crc = ((*ptr_input_buffer) << 24 | (*(ptr_input_buffer + 1)) << 16
                | (*(ptr_input_buffer + 2)) << 8 | (*(ptr_input_buffer + 3)));
        RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                "<%s::parseAndUpdateNIT> - CRC32 = 0x%08x\n", __FUNCTION__, *crc);
    }

    /*
     ** **************************************************************************
     ** At this point, if section_length = 0, the CRC_32 is present and the table
     ** section has been processed successfully. A section_length > 0 or
     ** section_length < 0  implies that an error has occurred and nit_error should
     ** identify the error condition.
     ** **************************************************************************
     */
PARSING_DONE: RDK_LOG(
                  RDK_LOG_TRACE4,
                  "LOG.RDK.SI",
                  "<%s::parseAndUpdateNIT> - Exiting with a Nit_Error_Code = (%d).\n",
                  __FUNCTION__, nit_error);

          return nit_error;

}

/*
 ** *****************************************************************************
 ** Subroutine/Method:                       parseAndUpdateSTT()
 ** *****************************************************************************
 **
 ** Parse STT table section and update SI database.
 **
 ** @param  ptr_TableSection,  a pointer to the start of the NTT table section.
 ** @param  sectiontable_size, The number of bytes in the table section per
 **
 **
 ** @return RMF_SI_SUCCESS if table section was successfully processed/parsed .
 ** Otherwise an error code greater than zero (RMF_SI_SUCCESS).
 ** *****************************************************************************
 */
rmf_Error rmf_OobSiParser::parseAndUpdateSTT(rmf_FilterSectionHandle section_handle,
        uint8_t *version, uint8_t *section_number,
        uint8_t *last_section_number, uint32_t *crc)
{
    rmf_Error retCode = RMF_SI_SUCCESS;
    stt_table_struc1 * ptr_stt_table_struc1 = NULL;
    uint8_t input_buffer[MAX_OOB_SMALL_TABLE_SECTION_LENGTH];
    uint8_t *ptr_input_buffer = NULL;
    uint16_t section_length = 0;
    uint32_t number_bytes_copied = 0;
    uint32_t gps_utc_offset = 0;
    uint32_t system_time = 0;
    uint32_t offset = 0;
    uint32_t protocol_version = 0;
    uint32_t crc2 = 0;
    uint32_t GPSTimeInUnixEpoch = 0;

    /*
     uint16_t DS_status                                           = FALSE;
     uint8_t DS_hour                                              = 0;
     generic_descriptor_struc *ptr_generic_descriptor_struc       = NULL;
     daylight_savings_descriptor_struc *ds_descriptor_struc       = NULL;
     uint16_t ds_desc_length                                      = 0;
     */
    rmf_Error stt_error = NO_PARSING_ERROR;

    RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI", "<%s> - Enter\n",
            __FUNCTION__);

    retCode = m_pSectionFilter->ReadSection(section_handle, 0,
            MAX_OOB_SMALL_TABLE_SECTION_LENGTH, 0, input_buffer,
            (uint32_t *) &number_bytes_copied);

    if (RMF_SI_SUCCESS != retCode)
    {
        stt_error = 1;
        goto PARSING_DONE;
    }
    else
    {
        ptr_stt_table_struc1 = (stt_table_struc1 *) input_buffer;

        //**** validate table Id field ****
        if (ptr_stt_table_struc1->table_id_octet1 != SYSTEM_TIME_TABLE_ID)
        {
            stt_error = 2;
            goto PARSING_DONE;
        }

        if (TESTBITS(ptr_stt_table_struc1->section_length_octet2, 8, 7))
        {
            RDK_LOG(
                    RDK_LOG_TRACE6,
                    "LOG.RDK.SI",
                    "<%s> - *** WARNING *** 2-Bit-Zero Field #1 not set to all zeroes(0)\n.",
                    __FUNCTION__);
        }

        //**** validate 2 reserved bits ****
        if (GETBITS(ptr_stt_table_struc1->section_length_octet2, 6, 5) != 3)
        {
            RDK_LOG(
                    RDK_LOG_TRACE6,
                    "LOG.RDK.SI",
                    "<%s> - *** WARNING *** 2-Bit-Reserved Field #2 not set to all ones(1)\n.",
                    __FUNCTION__);
        }
        section_length = ((GETBITS(ptr_stt_table_struc1->section_length_octet2,
                4, 1)) << 8) | ptr_stt_table_struc1->section_length_octet3;

        RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
                "<%s> - section_length = (%d).\n", __FUNCTION__,
                section_length);

        protocol_version = ptr_stt_table_struc1->protocol_version_octet4;
        if (protocol_version != 0)
        {
            RDK_LOG(
                    RDK_LOG_TRACE6,
                    "LOG.RDK.SI",
                    "<%s> - *** WARNING *** 8-Bit-Zero Field #4 not set to all zeroes(0)\n.",
                    __FUNCTION__);
        }
        RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                "<%s> - protocol_version = %d.\n", __FUNCTION__,
                protocol_version);

        if (ptr_stt_table_struc1->zeroes_octet5 != 0)
        {
            RDK_LOG(
                    RDK_LOG_TRACE6,
                    "LOG.RDK.SI",
                    "<%s> - *** WARNING *** 8-Bit-Zero Field #5 not set to all zeroes(0)\n.",
                    __FUNCTION__);
        }

        // Extract system time
        system_time = (ptr_stt_table_struc1->system_time_octet6 << 24)
                | (ptr_stt_table_struc1->system_time_octet7 << 16)
                | (ptr_stt_table_struc1->system_time_octet8 << 8)
                | (ptr_stt_table_struc1->system_time_octet9);

        // Extract GPS_UTC_Offset
        gps_utc_offset = ptr_stt_table_struc1->GPS_UTC_offset_octet10;

        RDK_LOG(
                RDK_LOG_TRACE4,
                "LOG.RDK.SI",
                "<%s> - system_time = %d gps_utc_offset = %d.\n",
                __FUNCTION__, system_time, gps_utc_offset);

        /*
         * STT time -  Seconds since 0UTC 1980-01-06 ("GPS time")
         * Convert to UNIX time (seconds since 0UTC 1970-01-01)
         *              t(unix) = t + 315964800
         */
        GPSTimeInUnixEpoch = (system_time - gps_utc_offset) + 315964800;

        // Set the stt time
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SI",
                "<%s, setSTTTime(%u, %lld)\n", __FUNCTION__,
                GPSTimeInUnixEpoch, m_pSiCache->getSTTStartTime());

    // Need to renable if we need to set JVM time using STT time
        //setSTTTime(GPSTimeInUnixEpoch, m_pSiCache->getSTTStartTime());

        // Parse the remaining descriptors
        offset += sizeof(stt_table_struc1);

        ptr_input_buffer = input_buffer + offset;
        section_length -= (sizeof(stt_table_struc1) - LEN_OCTET1_THRU_OCTET3);

        if (section_length < LENGTH_CRC_32)
        {
            stt_error = 3;
            goto PARSING_DONE;
        }

        if (section_length == LENGTH_CRC_32)
        {
            crc2 = ((*ptr_input_buffer) << 24 | (*(ptr_input_buffer + 1)) << 16
                    | (*(ptr_input_buffer + 2)) << 8
                    | (*(ptr_input_buffer + 3)));
            RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                    "<%s> - CRC32 = 0x%08x\n", __FUNCTION__,
                    crc2);
            goto PARSING_DONE;
        }

        // Un-comment the following block if descriptor parsing is enabled
        /*
         while (section_length > LENGTH_CRC_32)
         {
         ptr_generic_descriptor_struc = (generic_descriptor_struc *) ptr_input_buffer ;

         if ( ptr_generic_descriptor_struc->descriptor_tag_octet1 == 0x96 )
         {
         // Extract the daylight savings info
         ds_descriptor_struc = (daylight_savings_descriptor_struc *) ptr_input_buffer ;
         ds_desc_length = ds_descriptor_struc->descriptor_length_octet2;

         if (TESTBITS(ds_descriptor_struc->DS_status,8,8))
         {
         DS_status = YES ;
         }
         else
         {
         DS_status = NO ;
         }
         DS_hour = ds_descriptor_struc->DS_hour;

         RDK_LOG (RDK_LOG_TRACE4, "LOG.RDK.SI","<%s> - DS_status = %d  DS_hour = %d.\n",
         __FUNCTION__, DS_status, DS_hour);

         ptr_input_buffer     +=  sizeof(ds_descriptor_struc) + ds_desc_length ;
         section_length       -= (sizeof(ds_descriptor_struc) + ds_desc_length) ;
         }
         else
         {
         ptr_input_buffer     +=  sizeof(generic_descriptor_struc) + ptr_generic_descriptor_struc->descriptor_length_octet2 ;
         section_length       -= (sizeof(generic_descriptor_struc) + ptr_generic_descriptor_struc->descriptor_length_octet2) ;
         }


         if (section_length <= 0)
         {
         RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI","<%s::parseSTT_revision_data> - error, section_length not large enough to accommodate descriptor(s) and crc.\n",
         __FUNCTION__);
         break ;
         }

         }

         if ( section_length == LENGTH_CRC_32 )
         {
         crc2 = ((*ptr_input_buffer)     << 24
         | (*(ptr_input_buffer + 1))<< 16
         | (*(ptr_input_buffer + 2))<< 8
         | (*(ptr_input_buffer + 3)));

         RDK_LOG (RDK_LOG_TRACE4, "LOG.RDK.SI","<%s::parseNTT_revision_data> - CRC32 = 0x%08x\n", __FUNCTION__, crc2);
         }
         */
    }

    PARSING_DONE:

    RDK_LOG(
            RDK_LOG_TRACE4,
            "LOG.RDK.SI",
            "<%s> - Exiting with a STT_Error_Code = (%d).\n",
            __FUNCTION__, stt_error);

    return stt_error;
}

//***************************************************************************************************
//  Subroutine/Method:                       parseAndUpdateNTT()
//***************************************************************************************************
/**
 * Parse NTT table section and update SI database.
 *
 * @param  ptr_TableSection,  a pointer to the start of the NTT table section.
 * @param  sectiontable_size, The number of bytes in the table section per the PowerTV OS.
 *
 * @return RMF_SUCCESS if table section was successfully processed/parsed .Otherwise an error code
 *         greater than zero (RMF_SUCCESS).
 */
rmf_Error rmf_OobSiParser::parseAndUpdateNTT(rmf_FilterSectionHandle section_handle,
        uint8_t *version, uint8_t *section_number,
        uint8_t *last_section_number, uint32_t *crc)
{
    revision_detection_descriptor_struc * ptr_rdd_table_struc1 = NULL;
    rmf_Error ntt_error = NO_PARSING_ERROR;
    rmf_Error retCode = RMF_SI_SUCCESS;
    ntt_record_struc1 *ptr_table_struc1 = NULL;
    generic_descriptor_struct *ptr_descriptor_struc = NULL;
    int32_t section_length = 0;
    char language_code[4];
    uint16_t table_subtype = 0;
    uint32_t number_bytes_copied = 0;
    uint8_t input_buffer[MAX_IB_SECTION_LENGTH];
    uint8_t *ptr_input_buffer = NULL;
    /*
     ** ********************************************************************************************
     ** Parse and validate the first 8 bytes (Table_ID thru table_subtype) of the NTT
     ** table (see document ANSI/SCTE 65-2002 (formerly DVS 234), chap 5.2,pg.20) .
     ** *********************************************************************************************
     */
    RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
            "<%s> - Entered method parseAndUpdateNTT.\n",
            __FUNCTION__);

    retCode =  m_pSectionFilter->ReadSection(section_handle, 0,
            sizeof(ntt_record_struc1), 0, input_buffer, &number_bytes_copied);
    if (retCode != RMF_SI_SUCCESS || number_bytes_copied
            != sizeof(ntt_record_struc1))
    {
        ntt_error = 1;
        goto PARSING_DONE;
    }
    ptr_table_struc1 = (ntt_record_struc1 *) input_buffer;

    // **** validate table Id field ****
    if (ptr_table_struc1->table_id_octet1 != NETWORK_TEXT_TABLE_ID)
    {
        ntt_error = 2;
    }

    // **** extract and validate section_length field (octets 2 & 3)         ****
    // **** note, the maximum size of a NTT excluding octet 1 thru 3 is 1021.****
    // **** and occupies only bits 1 thru of the 12 bits                     ****

    section_length = ((GETBITS(ptr_table_struc1->section_length_octet2, 4, 1))
            << 8) | ptr_table_struc1->section_length_octet3;
    RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
            "<%s> - Section_Length = (%d)\n.", __FUNCTION__,
            section_length);
    //the following check will fail since the actual section_length will be 1024 during test
    // need to follow up on this issue
    /* if (section_length > MAX_IB_SECTION_LENGTH)
     {
     //error, sectionTable is not large enough to hold a section of size section_length
     ntt_error = 3 ;
     goto PARSING_DONE;
     }
     */

    // get the language_code
    language_code[0] = (char) (ptr_table_struc1->ISO_639_language_code_octet5);
    language_code[1] = (char) (ptr_table_struc1->ISO_639_language_code_octet6);
    language_code[2] = (char) (ptr_table_struc1->ISO_639_language_code_octet7);

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI",
            "<%s> - language code = 0x%x 0x%x 0x%x\n.",
            __FUNCTION__, (unsigned char) language_code[0],
            (unsigned char) language_code[1], (unsigned char) language_code[2]);

    // SCTE65 Section 5.2 indicates that the ISO639 language code may be specified as
    // 0xFFFFFF if the NTT is specified in only a single language
    if (language_code[0] == (char) 0xFF && language_code[1] == (char) 0xFF
            && language_code[2] == (char) 0xFF)
        language_code[0] = '\0';
    else
        language_code[3] = '\0';

    if (TESTBITS(ptr_table_struc1->table_subtype_octet8, 8, 5) != 0)
    {
        // bits 8 thru 5 shall be set to zero (0) per ANSI/SCTE 65-2002,pg.14, para 5.1
        ntt_error = 5;
    }

    //**** extract and validate the table_subtype field ****

    table_subtype = GETBITS(ptr_table_struc1->table_subtype_octet8, 4, 1);


    if (table_subtype != SOURCE_NAME_SUBTABLE)
    {
        // error, invalid table_subtype value.
        ntt_error = 6;
    }
    section_length -= (sizeof(ntt_record_struc1) - LEN_OCTET1_THRU_OCTET3);
    ptr_table_struc1 += 1;
    RDK_LOG(
            RDK_LOG_TRACE6,
            "LOG.RDK.SI",
            "<%s> - Table_Subtype = (%d) section_length: %d, ptr: 0x%p\n",
            __FUNCTION__, table_subtype, section_length, ptr_table_struc1);

    if ((ntt_error == NO_PARSING_ERROR) && (section_length < LENGTH_CRC_32))
    { // error, either a portion of header is missing, or all or a portion of the
        // CRC_32 value is missing.
        ntt_error = 7;
    }

    //**********************************************************************************************
    // Parse the remaining bytes/octets in the NTT.
    //**********************************************************************************************

    retCode =  m_pSectionFilter->ReadSection(section_handle, 8, section_length, 0,
            input_buffer, (uint32_t *) &number_bytes_copied);

    if ((RMF_SI_SUCCESS != retCode) || (number_bytes_copied
            != (uint32_t) section_length))
    {
        ntt_error = 8;
        goto PARSING_DONE;
    }

    ptr_input_buffer = input_buffer;
    RDK_LOG(
            RDK_LOG_TRACE6,
            "LOG.RDK.SI",
            "<%s> - before sns parse call - ptr_table_struc1 = (0x%p bytes copied: %d\n",
            __FUNCTION__, ptr_input_buffer, number_bytes_copied);

    //****************************************************************************************
    //**** Process the SNS subtable
    //****************************************************************************************
    ntt_error = parseSNSSubtable(&ptr_input_buffer, &section_length,
            language_code);

    // if ntt_error == NO_PARSING_ERROR, ptr_TableSection has been properly updated
    RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
            "<%s> - section_length: %d, ptr: 0x%p\n",
            __FUNCTION__, section_length, ptr_input_buffer);

    //*************************************************************************************************
    //* At this point, there may be a series a descriptors present.However, we are not required to
    //* process them for the 06-07-04 PowerTV release. If there are descriptors present, we will
    //* "ignore" them.
    //*************************************************************************************************
    if ((ntt_error == NO_PARSING_ERROR) && (section_length > LENGTH_CRC_32))
    {
        ptr_rdd_table_struc1
                = (revision_detection_descriptor_struc *) ptr_input_buffer;

        if (ptr_rdd_table_struc1->descriptor_tag_octet1 == 0x93)
        {
            *version = (ptr_rdd_table_struc1->table_version_number_octet3
                    & 0x1F);
            *section_number = ptr_rdd_table_struc1->section_number_octet4;
            *last_section_number
                    = ptr_rdd_table_struc1->last_section_number_octet5;
            RDK_LOG(
                    RDK_LOG_TRACE4,
                    "LOG.RDK.SI",
                    "<%s> - version: %d, section_number: %d, last_section_number: %d\n",
                    __FUNCTION__, *version, *section_number, *last_section_number);
            ptr_input_buffer += sizeof(revision_detection_descriptor_struc);
            section_length -= sizeof(revision_detection_descriptor_struc);
        }
    }

    if (section_length > LENGTH_CRC_32)
    {
        do
        {
            ptr_descriptor_struc
                    = (generic_descriptor_struct *) ptr_input_buffer;
            ptr_input_buffer += sizeof(generic_descriptor_struct)
                    + ptr_descriptor_struc->descriptor_length_octet2;
            section_length -= (sizeof(generic_descriptor_struct)
                    + ptr_descriptor_struc->descriptor_length_octet2);

            if (section_length <= 0)
            {
                if (section_length < 0)
                { //error, section_length not large enough to accommodate descriptor(s) .
                    ntt_error = 90;
                }
                break;
            }
        } while (section_length > LENGTH_CRC_32);
    }

    RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
            "<%s> - ptr_input_buffer = 0x%p\n", __FUNCTION__,
            ptr_input_buffer);
    if (section_length == LENGTH_CRC_32)
    {
        *crc = ((*ptr_input_buffer) << 24 | (*(ptr_input_buffer + 1)) << 16
                | (*(ptr_input_buffer + 2)) << 8 | (*(ptr_input_buffer + 3)));
        RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                "<%s> - CRC32 = %u\n", __FUNCTION__, *crc);
    }

    PARSING_DONE:

    if (ntt_error == NO_PARSING_ERROR)
    {
        RDK_LOG(RDK_LOG_TRACE3, "LOG.RDK.SI",
                "<%s> - SVCT records parsed, set flag.\n",
                __FUNCTION__);
    }

    /*
     ** **************************************************************************
     ** * At this point, if section_length = 0, the CRC_32 is present and the table section has been
     ** * processed successfully. A section_length > 0 or section_length < 0  implies that an error has
     ** * occurred and ntt_error should identify the error condition.
     ** **************************************************************************
     */
    RDK_LOG(
            RDK_LOG_TRACE4,
            "LOG.RDK.SI",
            "<%s> - Exiting with a NTT_Error_Code = (%d).\n",
            __FUNCTION__, ntt_error);
    return ntt_error;
}

/*
 ** **************************************************************************************************
 **  Subroutine/Method:                       parseCDSRecords()
 ** **************************************************************************************************
 ** Parses CDS (Carriers Defination Subtable) records, validates the fileds within a CDS record,
 ** generates/decompresses the carrier frequencies and stores them in the SI database .
 **
 ** @param  number_of_records,  identifies the number of CDS records to be processed.
 ** @param  first_index,        identifies the index to be associated with the ist CDS record
 **                             to be processed.
 ** @param  pptr_TableSection,  the address of memeory location which contains a pointer to the NIT
 **                             table section.
 ** @param  sectiontable_size,  The number of bytes in the table section per the PowerTV OS.
 ** @param  ptr_section_length, a pointer to the caller's section_length variable.
 **
 ** @return RMF_SUCCESS if table section was successfully processed/parsed .Otherwise an error code
 **         greater than zero (RMF_SUCCESS).
 ** **************************************************************************************************
 */
rmf_Error rmf_OobSiParser::parseCDSRecords(uint16_t number_of_records, uint16_t first_index,
        uint8_t ** handle_section_data, uint32_t * ptr_section_length)
{
    rmf_Error nit_error = NO_PARSING_ERROR;
    uint32_t i, j, descriptor_count;

    uint16_t number_of_carrier_frequencies = 0, carrier_frequency_spacing = 0,
            first_frequency = 0, carrier_frequency_count = 0,
            carrier_frequency_record = first_index;

    uint32_t carrier_frequency_spacing_units = 0, first_frequency_units = 0,
            array_of_frequencies[255], *ptr_frequency_array =
                    array_of_frequencies, base_frequency, delta_frequency;

    cds_record_struc2 *ptr_cds_record_struc2 = NULL;
    generic_descriptor_struct *ptr_descriptor_struc = NULL;

    //***********************************************************************************************
    //***********************************************************************************************
    RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
            "<%s::parseCDSRecords> - Entered method parseCDSRecords.\n",
            __FUNCTION__);


    /*
     ** read the rest of the table section
     */
    for (i = 0; (i < number_of_records) && (nit_error == NO_PARSING_ERROR); ++i)
    {
        //*******************************************************************************************
        // Extract and validate data from the specified cds records per table 5.3, pg. 16 in document
        // ANSI/SCTE 65-2002 (formerly DVS 234)
        //*******************************************************************************************

        ptr_cds_record_struc2 = (cds_record_struc2 *) *handle_section_data;

        // range is 1 thru 255
        number_of_carrier_frequencies
                = ptr_cds_record_struc2->number_of_carriers_octet1;
        RDK_LOG(
                RDK_LOG_TRACE4,
                "LOG.RDK.SI",
                "<%s::parseCDSRecords> - number_of_carrier_frequencies(%d) = %d.\n",
                __FUNCTION__, i, number_of_carrier_frequencies);

        if (number_of_carrier_frequencies == 0)
        { // error, number_of_carriers should be > 0
            nit_error = 14;
            break;
        }

        if ((carrier_frequency_count + number_of_carrier_frequencies
                + first_index - 1) > MAX_CARRIER_FREQUENCY_INDEX)
        {
            /*
             ** error, will generate an index to a carrier frequence which is greater than 255.
             ** see spec ANSI/SCTE 65-2002 (formerly DVS 234), para 5.1.1, para. 3, pg. 16
             */
            nit_error = 15;
            break;
        }

        carrier_frequency_spacing_units = FREQUENCY_UNITS_10KHz;

        if (TESTBITS(ptr_cds_record_struc2->frequency_spacing_octet2, 8, 8))
        {
            carrier_frequency_spacing_units = FREQUENCY_UNITS_125KHz;
        }
        RDK_LOG(
                RDK_LOG_TRACE6,
                "LOG.RDK.SI",
                "<%s::parseCDSRecords> - carrier_frequency_spacing_units(%d) = %d.\n",
                __FUNCTION__, i, carrier_frequency_spacing_units);

        if (TESTBITS(ptr_cds_record_struc2->frequency_spacing_octet2, 7, 7))
        { // error, "zero field" is set to "1"
            //nit_error = 16 ;
            //break ;
            RDK_LOG(
                    RDK_LOG_TRACE6,
                    "LOG.RDK.SI",
                    "<%s::parseCDSRecords> - *** WARNING *** 1-Bit-Zero Field #1 not set to all zeroes(0)\n.",
                    __FUNCTION__);
        }

        carrier_frequency_spacing = (uint16_t)(TESTBITS(
                ptr_cds_record_struc2->frequency_spacing_octet2, 6, 1) << 8)
                | ptr_cds_record_struc2->frequency_spacing_octet3;
        RDK_LOG(
                RDK_LOG_TRACE6,
                "LOG.RDK.SI",
                "<%s::parseCDSRecords> - carrier_frequency_spacing(%d) = %d.\n",
                __FUNCTION__, i, carrier_frequency_spacing);

        if ((number_of_carrier_frequencies != 1) && (carrier_frequency_spacing
                == 0))
        { // error, carrier_frequency_spacing must be > 0
            nit_error = 17;
            break;
        }

        first_frequency_units = FREQUENCY_UNITS_10KHz;

        if (TESTBITS(ptr_cds_record_struc2->first_carrier_frequency_octet4, 8,
                8))
        {
            first_frequency_units = FREQUENCY_UNITS_125KHz;
        }
        RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
                "<%s::parseCDSRecords> - first_frequency_units(%d) = %d.\n",
                __FUNCTION__, i, first_frequency_units);

        // range is 0 thru 32,767 ((2**15) -1)
        first_frequency = (uint16_t)(TESTBITS(
                ptr_cds_record_struc2->first_carrier_frequency_octet4, 7, 1)
                << 8) | ptr_cds_record_struc2->first_carrier_frequency_octet5;
        RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
                "<%s::parseCDSRecords> - first_frequency(%d) = %d.\n",
                __FUNCTION__, i, first_frequency);

        //****************************************************************************************************
        // Generate/uncompress the specified number of frequencies.
        //*****************************************************************************************************
        base_frequency = first_frequency * first_frequency_units;
        delta_frequency = carrier_frequency_spacing
                * carrier_frequency_spacing_units;

        if (m_pSiCache->getDumpTables())
            RDK_LOG(
                    RDK_LOG_INFO,
                    "LOG.RDK.SI",
                    "<%s::parseCDSRecords> - Ent %-3u[#Carriers:%u SU:%u FirstCF:%ukHz FreqU:%u FreqS:%ukHz]\n",
                    __FUNCTION__, i, number_of_carrier_frequencies,
                    carrier_frequency_spacing_units, base_frequency / 1000, // Note: Is unit-adjusted first_frequency value
                    first_frequency_units, delta_frequency / 1000); // Note: Is unit-adjusted carrier_frequency_spacing value

        for (j = 0; j < number_of_carrier_frequencies; ++j, ++ptr_frequency_array, carrier_frequency_record++)
        {
            // compute and store the carrier frequency

            *ptr_frequency_array = base_frequency + (j * delta_frequency);
            if (m_pSiCache->getDumpTables())
            {
                RDK_LOG(
                        RDK_LOG_INFO,
                        "LOG.RDK.SI",
                        "<%s::parseCDSRecords> -   CDS Record %3u: %ukHz channel at %ukHz\n",
                        __FUNCTION__, carrier_frequency_record, delta_frequency
                                / 1000, (*ptr_frequency_array) / 1000);
                RDK_LOG(
                        RDK_LOG_INFO,
                        "LOG.RDK.SI",
                        "<%s::parseCDSRecords> - Record(%d)   Frequency(%d) = %u.\n",
                        __FUNCTION__, i, j + 1, *ptr_frequency_array);
            }
        }

        carrier_frequency_count = (uint16_t)(carrier_frequency_count
                + number_of_carrier_frequencies);

        RDK_LOG(
                RDK_LOG_TRACE6,
                "LOG.RDK.SI",
                "<%s::parseCDSRecords> - carrier_frequency_count = %d, first_index = %d.\n",
                __FUNCTION__, carrier_frequency_count, first_index);

        //****************************************************************************************************
        // Prepare to process the descriptors if they are present.
        //****************************************************************************************************
        *handle_section_data = (*handle_section_data)
                + sizeof(cds_record_struc2);
        *ptr_section_length = (*ptr_section_length) - sizeof(cds_record_struc2);

        descriptor_count = **handle_section_data; // get 8-bit descriptor count and advance pointer
        *handle_section_data = ((*handle_section_data) + 1);
        *ptr_section_length = (*ptr_section_length) - 1;
        RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
                "<%s::parseCDSRecords> - descriptor count = %d\n", __FUNCTION__,
                descriptor_count);

        while (descriptor_count > 0)
        { // ignore all descriptors .

            ptr_descriptor_struc
                    = (generic_descriptor_struct *) *handle_section_data;
            *handle_section_data += sizeof(generic_descriptor_struct)
                    + ptr_descriptor_struc->descriptor_length_octet2;

            //*** decrement ptr_section_length value as we did not account for descriptor tag, length
            //*** and data fields length and data fields prior to calling this subroutine .

            --descriptor_count;
            *ptr_section_length -= (sizeof(generic_descriptor_struct)
                    + ptr_descriptor_struc->descriptor_length_octet2);

            if ((int32_t) * ptr_section_length < 0)
            { //error, section_length not large enough to account for descriptor tags, length and data fields
                nit_error = 18;
                break;
            }
        } //end_of_while_loop
    } //end_of_for_loop


    //****************************************************************************************************
    //    Update the SI database with the frequencies.
    //****************************************************************************************************

    if (nit_error == NO_PARSING_ERROR)
    {
        m_pSiCache->LockForWrite();
        nit_error = m_pSiCache->SetCarrierFrequencies(array_of_frequencies,
                (uint8_t) first_index, (uint8_t) carrier_frequency_count);
        //nit_error = mpe_siSetCarrierFrequencies(array_of_frequencies,
          //      (uint8_t) first_index, (uint8_t) carrier_frequency_count);
        m_pSiCache->ReleaseWriteLock();

        if (nit_error != NO_PARSING_ERROR)
        {
            RDK_LOG(
                    RDK_LOG_TRACE6,
                    "LOG.RDK.SI",
                    "<%s::parseCDSRecords> - Call to m_pSiCache->SetCarrierFrequencies failed,ErrorCode = %d\n",
                    __FUNCTION__, nit_error);
        }
        else
        {
            RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
                    "<%s::parseCDSRecords> - CDS records parsed, set flag.\n",
                    __FUNCTION__);

            //setTablesAcquired(getTablesAcquired() | CDS_ACQUIRED) ;
            // SI DB re-factor changes (7/11/05)
            //mpe_siNotifyTableAcquired(OOB_NIT_CDS, 0);
        }
    }

    RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
            "<%s::parseCDSRecords> - section_length = %d.\n", __FUNCTION__,
            *ptr_section_length);
    RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
            "<%s::parseCDSRecords> - Exiting with a Nit_Error_Code = %d.\n",
            __FUNCTION__, nit_error);
    
    return nit_error;
}

//***************************************************************************************************
//  Subroutine/Method:                       parseMMSRecords()
//***************************************************************************************************
/**
 * Parses MMS(Modulation Mode Subtable) records, validates the fields within a MMS record
 * and stores the data in the SI database .
 *
 * @param  number_of_records,  identifies the number of MMS records to be processed.
 * @param  first_index,        identifies the index to be associated with the ist CDS record
 *                             to be processed.
 * @param  pptr_TableSection,  the address of memeory location which contains a pointer to the NIT
 *                             table section.
 * @param  sectiontable_size,  The number of bytes in the table section per the PowerTV OS.
 * @param  ptr_section_length, a pointer to the caller's section_length variable.
 *
 * @return RMF_SI_SUCCESS if table section was successfully processed/parsed .Otherwise an error code
 *         greater than zero (RMF_SI_SUCCESS).
 */
rmf_Error rmf_OobSiParser::parseMMSRecords(uint16_t number_of_records, uint16_t first_index,
        uint8_t ** handle_section_data, uint32_t *ptr_section_length)
{
    mms_record_struc2 *ptr_mms_record_struc2;
    uint16_t transmission_system = 0, inner_coding_mode = 0,
            split_bitstream_mode;
    uint32_t i = 0, descriptor_count = 0;
    uint32_t symbol_rate;

    rmf_ModulationMode array_of_modulation_format[255],
            *ptr_modulation_array = array_of_modulation_format,
            modulation_format = RMF_MODULATION_UNKNOWN;
    generic_descriptor_struct *ptr_descriptor_struc;
    rmf_Error nit_error = NO_PARSING_ERROR;
    char modformatstring[255];

    //***********************************************************************************************
    //***********************************************************************************************
    RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
            "<%s::parseMMSRecords> - Entered method parseMMSRecords.\n",
            __FUNCTION__);

    RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
            "<%s::parseMMSRecords> - first_index = %d.\n", __FUNCTION__,
            first_index);

    for (i = 0; (i < number_of_records) && (nit_error == NO_PARSING_ERROR); ++i)
    {
        //*******************************************************************************************
        // Extract and validate data from the specified cds records per table 5.6, pg. 18 in document
        // ANSI/SCTE 65-2002 (formerly DVS 234) //.
        //*******************************************************************************************

        ptr_mms_record_struc2 = (mms_record_struc2 *) *handle_section_data;

        transmission_system = (uint16_t)(GETBITS(
                ptr_mms_record_struc2->inner_coding_mode_octet1, 8, 5));
        RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
                "<%s::parseMMSRecords> - transmission_system(%d) = (%d).\n",
                __FUNCTION__, i, transmission_system);

        /*        if (!((transmission_system == TRANSMISSION_SYSTEM_ITUT_ANNEXB)||
         (transmission_system == TRANSMISSION_SYSTEM_ATSC)         ) )
         {  // error,  invalid transmission_system value .
         nit_error = 30 ;
         break ;
         }
         */
        inner_coding_mode = (uint16_t)(GETBITS(
                ptr_mms_record_struc2->inner_coding_mode_octet1, 4, 1));
        RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
                "<%s::parseMMSRecords> - inner_coding_mode(%d) = (%d).\n",
                __FUNCTION__, i, inner_coding_mode);

        if (!((inner_coding_mode <= INNER_CODING_MODE_1_2)
                || (inner_coding_mode == INNER_CODING_MODE_3_5)
                || (inner_coding_mode == INNER_CODING_MODE_2_3)
                || ((inner_coding_mode >= INNER_CODING_MODE_3_4)
                        && (inner_coding_mode <= INNER_CODING_MODE_5_6))
                || (inner_coding_mode == INNER_CODING_MODE_7_8)
                || (inner_coding_mode == DOES_NOT_USE_CONCATENATED_CODING)))
        { // error, inner_coding_mode.
            nit_error = 31;
            break;
        }

        split_bitstream_mode = NO;

        if (GETBITS(ptr_mms_record_struc2->modulation_format_octet2, 8, 8))
        {
            split_bitstream_mode = YES;
        }
        RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
                "<%s::parseMMSRecords> - split_bitstream_mode(%d) = (%d).\n",
                __FUNCTION__, i, split_bitstream_mode);

        if (TESTBITS(ptr_mms_record_struc2->modulation_format_octet2, 7, 6))
        { // error, "one or more bits are set to "1".
            //nit_error = 32 ;
            //break ;
            RDK_LOG(
                    RDK_LOG_TRACE6,
                    "LOG.RDK.SI",
                    "<%s::parseMMSRecords> - *** WARNING *** 2-Bit-Zero Field #1 not set to all zeroes(0)\n.",
                    __FUNCTION__);
        }

        modulation_format = (rmf_ModulationMode)GETBITS(
                ptr_mms_record_struc2->modulation_format_octet2, 5, 1);
        RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
                "<%s::parseMMSRecords> - modulation_format(%d) = (%d).\n",
                __FUNCTION__, i, modulation_format);

        if (modulation_format > MODULATION_FORMAT_QAM_1024)
        { // error, invalid modulation_format .
            nit_error = 33;
            break;
        }

        if (TESTBITS(ptr_mms_record_struc2->modulation_symbol_rate_octet3, 8, 5))
        { // error, "one or more bits are set to "1"
            //nit_error = 34 ;
            //break ;
            RDK_LOG(
                    RDK_LOG_TRACE6,
                    "LOG.RDK.SI",
                    "<%s::parseMMSRecords> - *** WARNING *** 4-Bit-Zero Field #2 not set to all zeroes(0)\n.",
                    __FUNCTION__);
        }

        symbol_rate = ((TESTBITS(
                ptr_mms_record_struc2->modulation_symbol_rate_octet3, 4, 1))
                << 24) | (ptr_mms_record_struc2->modulation_symbol_rate_octet4
                << 16) | (ptr_mms_record_struc2->modulation_symbol_rate_octet5
                << 8) | ptr_mms_record_struc2->modulation_symbol_rate_octet6;
        RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
                "<%s::parseMMSRecords> - symbol_rate(%d) = (%d).\n", __FUNCTION__,
                i, symbol_rate);

        //****************************************************************************************************
        // Move Modulation mode/format into the modulation array.
        //****************************************************************************************************
        if (m_pSiCache->getDumpTables())
        {
            memset(modformatstring, 0, sizeof(modformatstring));
            switch (modulation_format)
            {
            case RMF_MODULATION_UNKNOWN:
                strncpy(modformatstring, "unknown", sizeof(modformatstring)-1);
                break;
            case RMF_MODULATION_QPSK:
                strncpy(modformatstring, "QPSK", sizeof(modformatstring)-1);
                break;
            case RMF_MODULATION_BPSK:
                strncpy(modformatstring, "BPSK", sizeof(modformatstring)-1);
                break;
            case RMF_MODULATION_OQPSK:
                strncpy(modformatstring, "OQPSK",sizeof(modformatstring)-1);
                break;
            case RMF_MODULATION_VSB8:
                strncpy(modformatstring, "VSB-8",sizeof(modformatstring)-1);
                break;
            case RMF_MODULATION_VSB16:
                strncpy(modformatstring, "VSB-16",sizeof(modformatstring)-1);
                break;
            case RMF_MODULATION_QAM16:
                strncpy(modformatstring, "QAM-16",sizeof(modformatstring)-1);
                break;
            case RMF_MODULATION_QAM32:
                strncpy(modformatstring, "QAM-32",sizeof(modformatstring)-1);
                break;
            case RMF_MODULATION_QAM64:
                strncpy(modformatstring, "QAM-64",sizeof(modformatstring)-1);
                break;
            case RMF_MODULATION_QAM80:
                strncpy(modformatstring, "QAM-80",sizeof(modformatstring)-1);
                break;
            case RMF_MODULATION_QAM96:
                strncpy(modformatstring, "QAM-96",sizeof(modformatstring)-1);
                break;
            case RMF_MODULATION_QAM112:
                strncpy(modformatstring, "QAM-112",sizeof(modformatstring)-1);
                break;
            case RMF_MODULATION_QAM128:
                strncpy(modformatstring, "QAM-128",sizeof(modformatstring)-1);
                break;
            case RMF_MODULATION_QAM160:
                strncpy(modformatstring, "QAM-160",sizeof(modformatstring)-1);
                break;
            case RMF_MODULATION_QAM192:
                strncpy(modformatstring, "QAM-192",sizeof(modformatstring)-1);
                break;
            case RMF_MODULATION_QAM224:
                strncpy(modformatstring, "QAM-224",sizeof(modformatstring)-1);
                break;
            case RMF_MODULATION_QAM256:
                strncpy(modformatstring, "QAM-256",sizeof(modformatstring)-1);
                break;
            case RMF_MODULATION_QAM320:
                strncpy(modformatstring, "QAM-320",sizeof(modformatstring)-1);
                break;
            case RMF_MODULATION_QAM384:
                strncpy(modformatstring, "QAM-384",sizeof(modformatstring)-1);
                break;
            case RMF_MODULATION_QAM448:
                strncpy(modformatstring, "QAM-448",sizeof(modformatstring)-1);
                break;
            case RMF_MODULATION_QAM512:
                strncpy(modformatstring, "QAM-512",sizeof(modformatstring)-1);
                break;
            case RMF_MODULATION_QAM640:
                strncpy(modformatstring, "QAM-640",sizeof(modformatstring)-1);
                break;
            case RMF_MODULATION_QAM768:
                strncpy(modformatstring, "QAM-768",sizeof(modformatstring)-1);
                break;
            case RMF_MODULATION_QAM896:
                strncpy(modformatstring, "QAM-896",sizeof(modformatstring)-1);
                break;
            case RMF_MODULATION_QAM1024:
                strncpy(modformatstring, "QAM-1024",sizeof(modformatstring)-1);
                break;
#if 0 /*Dead Code, never reached as loop breaks on modulation_format > MODULATION_FORMAT_QAM_1024 */
            case RMF_MODULATION_QAM_NTSC:
                strncpy(modformatstring, "Analog",sizeof(modformatstring)-1);
                break;
#endif
            default:
                strncpy(modformatstring, "RESERVED",sizeof(modformatstring)-1);
                break;
            }

            RDK_LOG(
                    RDK_LOG_INFO,
                    "LOG.RDK.SI",
                    "<%s::parseMMSRecords> - Ent %-3d[TranSys:%d ICM:%d SBM:%d Mod:%d(%s) SymRate:%d]\n",
                    __FUNCTION__, i + first_index, transmission_system,
                    inner_coding_mode, split_bitstream_mode, modulation_format,
                    modformatstring, symbol_rate);
        }

        *ptr_modulation_array = modulation_format;

        RDK_LOG(
                RDK_LOG_TRACE6,
                "LOG.RDK.SI",
                "<%s::parseMMSRecords> - Record(%d)   Modulation_Format(%d) = %d.\n",
                __FUNCTION__, i, i, *ptr_modulation_array);

        ++ptr_modulation_array;

        //****************************************************************************************************
        // Prepare to process the descriptors if they are present.
        //****************************************************************************************************
        *handle_section_data += sizeof(mms_record_struc2);
        *ptr_section_length -= sizeof(mms_record_struc2);

        descriptor_count = **handle_section_data; // get 8-bit descriptor count and advance pointer
        ++(*handle_section_data);
        *ptr_section_length -= 1;

        while (descriptor_count > 0)
        { // ignore all descriptors .

            ptr_descriptor_struc
                    = (generic_descriptor_struct *) *handle_section_data;
            *handle_section_data += sizeof(generic_descriptor_struct)
                    + ptr_descriptor_struc->descriptor_length_octet2;

            //*** decrement section_length as we did not account for descriptor tag, length and data fields
            //*** length and data fields prior to calling subroutine .

            --descriptor_count;
            *ptr_section_length -= (sizeof(generic_descriptor_struct)
                    + ptr_descriptor_struc->descriptor_length_octet2);

            if ((int32_t) * ptr_section_length < 0)
            { //error, section_length not large enough to account for descriptor tags, length and data fields
                nit_error = 35;
                break;
            }
        }

    } //end_of_outer_loop

    //*******************************************************************************************************
    //    Update the SI database with the frequencies.
    //*******************************************************************************************************

    if (nit_error == NO_PARSING_ERROR)
    {
        m_pSiCache->LockForWrite();
        nit_error =  m_pSiCache->SetModulationModes(array_of_modulation_format,
                (uint8_t) first_index, (uint8_t) number_of_records);
        m_pSiCache->ReleaseWriteLock();

        if (nit_error != NO_PARSING_ERROR)
        {
            RDK_LOG(
                    RDK_LOG_ERROR,
                    "LOG.RDK.SI",
                    "<%s::parseMMSRecords> - Call to m_pSiCache->SetModulationModes failed,ErrorCode = %d\n",
                    __FUNCTION__, nit_error);
        }
        else
        {
            RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
                    "<%s::parseMMSRecords> - MMS records parsed, set flag.\n",
                    __FUNCTION__);

            //setTablesAcquired(getTablesAcquired() | MMS_ACQUIRED) ;
            // SI DB re-factor changes (7/11/05)
            //mpe_siNotifyTableAcquired(OOB_NIT_MMS, 0);
        }
    }

    RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
            "<%s::parseMMSRecords> - Exiting with a Nit_Error_Code = (%d).\n",
            __FUNCTION__, nit_error);

    return nit_error;
}

/*
 ** ****************************************************************************
 **  Subroutine/Method:     parseVCMStructure()
 ** ****************************************************************************
 **
 ** Parses Virtual Channel Map(VCM) structure, validates the fields within the structure
 **
 ** @param  pptr_TableSection,  the address of memeory location which contains a pointer to the SVCT
 **                             table section.
 ** @param  ptr_section_length, a pointer to the caller's section_length variable.
 **
 ** @return RMF_SI_SUCCESS if table section was successfully processed/parsed .Otherwise an error code
 **         greater than zero (RMF_SI_SUCCESS).
 **
 */
rmf_Error rmf_OobSiParser::parseVCMStructure(uint8_t **pptr_TableSection, int32_t *ptr_section_length)
{
    vcm_record_struc2 *ptr_vcm_record_struc2;
    vcm_virtual_chan_record3 *ptr_vcm_virtual_chan_record3;
    mpeg2_virtual_chan_struc4 *ptr_mpeg2_virtual_chan_struc4;
    nmpeg2_virtual_chan_struc5 *ptr_nmpeg2_virtual_chan_struc5;
    generic_descriptor_struct *ptr_descriptor_struc;
    rmf_SiServiceHandle si_database_handle = 0; // a handle into ths SI database

    uint32_t i, descriptor_count = 0;
    uint16_t program_number = 0, number_of_vc_records = 0,
            virtual_channel_number = 0, channel_type = 0, application_id = 0,
            source_id = 0, video_standard = 0, size_struc_used = 0,
            application_virtual_channel = NO, path_select = PATH_1_SELECTED,
            transport_type = MPEG_2_TRANSPORT, descriptors_included = NO,
            scrambled = NO, splice = NO;
    uint8_t cds_reference = 0, mms_reference = 0;

    uint32_t activation_time = 0;
    rmf_Error svct_error = NO_PARSING_ERROR, saved_svct_error =
            NO_PARSING_ERROR;
    //***********************************************************************************************
    //***********************************************************************************************

    //***********************************************************************************************
    //**** Parse the  VCM Structure.
    //***********************************************************************************************
    RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
            "<%s> - Entered method parseVCMStructure.\n",
            __FUNCTION__);
    RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
            "<%s> - ptr_TableSection = (%p).\n", __FUNCTION__,
            *pptr_TableSection);

    ptr_vcm_record_struc2 = (vcm_record_struc2 *) *pptr_TableSection;

    //**** validate "Zero" (2-bits) field ****
    if (TESTBITS(ptr_vcm_record_struc2->descriptors_included_octet1, 8, 7))
    {
        // error, bits not set to 0.
        //svct_error = 20 ;
        RDK_LOG(
                RDK_LOG_TRACE4,
                "LOG.RDK.SI",
                "<%s> - *** WARNING *** 2-Bit-Zero Field #1 not set to all zeroes(0)\n.",
                __FUNCTION__);
    }

    if (TESTBITS(ptr_vcm_record_struc2->descriptors_included_octet1, 6, 6))
    {
        descriptors_included = YES;
    }

    RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
            "<%s> - descriptors_included = (%d).\n",
            __FUNCTION__, descriptors_included);

    //**** validate "Zero" (5-bits) field ****
    if (TESTBITS(ptr_vcm_record_struc2->descriptors_included_octet1, 5, 1))
    {
        // error, bits not set to 0.
        //svct_error = 21 ;
        RDK_LOG(
                RDK_LOG_TRACE4,
                "LOG.RDK.SI",
                "<%s> - *** WARNING *** 5-Bit-Zero Field #2 not set to all zeroes(0)\n.",
                __FUNCTION__);
    }

    if (TESTBITS(ptr_vcm_record_struc2->splice_octet2, 8, 8))
    {
        splice = YES;
    }

    RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
            "<%s> - splice = (%d).\n", __FUNCTION__, splice);

    //**** validate "Zero" (7-bits) field ****
    if (TESTBITS(ptr_vcm_record_struc2->splice_octet2, 7, 1))
    {
        // error, 1 or more bits are set to 1.
        //svct_error = 22 ;
        RDK_LOG(
                RDK_LOG_TRACE4,
                "LOG.RDK.SI",
                "<%s> - *** WARNING *** 7-Bit-Zero Field #3 not set to all zeroes(0)\n.",
                __FUNCTION__);
    }

    activation_time = (ptr_vcm_record_struc2->activation_time_octet3 << 24)
            | (ptr_vcm_record_struc2->activation_time_octet4 << 16)
            | (ptr_vcm_record_struc2->activation_time_octet5 << 8)
            | ptr_vcm_record_struc2->activation_time_octet6;
    RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
            "<%s> - activation_time = (%d).\n", __FUNCTION__,
            activation_time);

    /*
     if (activation_time != IMMEDIATE_ACTIVATION)
     {
     // per Debra H. for 06-07-04 release.
     svct_error = 23 ;
     }
     */

    number_of_vc_records = ptr_vcm_record_struc2->number_vc_records_octet7;

    RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
            "<%s> - number_of_vc_records = (%d).\n",
            __FUNCTION__, number_of_vc_records);

    if (number_of_vc_records == 0)
    {
        // error,must be > 0.
        svct_error = 24;
    }

    //***********************************************************************************************
    //*** Adjust pointers to reflect the parsing of the VCM Structure (vcm_record_struc2).
    //***********************************************************************************************
    *pptr_TableSection += sizeof(vcm_record_struc2);
    *ptr_section_length -= sizeof(vcm_record_struc2);
    RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
            "<%s> - section_length11 = (%d).\n", __FUNCTION__,
            *ptr_section_length);
    RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
            "<%s> - ptr_TableSection11 = (%p).\n", __FUNCTION__,
            *pptr_TableSection);

    if (*ptr_section_length <= 0)
    {
        // error, either VCM structure was too short (*ptr_section_length < 0) or
        //        there is no virtual channel record (*ptr_section_length = 0).
        svct_error = 25;
    }

    //***********************************************************************************************
    //**** Parse each Virtual Channel Record
    //***********************************************************************************************
    if (svct_error == NO_PARSING_ERROR)
    {
        for (i = 0; i < number_of_vc_records; ++i)
        {
            application_virtual_channel = NO;

            RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
                    "<%s> - index = %d.\n", __FUNCTION__, i);

            ptr_vcm_virtual_chan_record3
                    = (vcm_virtual_chan_record3 *) *pptr_TableSection;

            //**** validate "Zero" (4-bits) field ****
            if (TESTBITS(
                    ptr_vcm_virtual_chan_record3->virtual_chan_number_octet1,
                    8, 5))
            {
                // error, 1 or more bits are set to 1.
                //svct_error = 26;
                RDK_LOG(
                        RDK_LOG_TRACE6,
                        "LOG.RDK.SI",
                        "<%s> - *** WARNING (%d) *** 4-Bit-Zero Field #4 not set to all zeroes(0)\n.",
                        __FUNCTION__, i);
            }

            // range 0 thru 4095 ((2**12)-1)
            virtual_channel_number = (uint16_t)((GETBITS(
                    ptr_vcm_virtual_chan_record3->virtual_chan_number_octet1,
                    4, 1) << 8)
                    | ptr_vcm_virtual_chan_record3->virtual_chan_number_octet2);

            if (TESTBITS(ptr_vcm_virtual_chan_record3->chan_type_octet3, 8, 8))
            {
                application_virtual_channel = YES;
            }

            //RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI","<%s> - application_virtual_channel(%d) = (%d).\n",
            //      __FUNCTION__,i, application_virtual_channel);

            //**** validate "Zero" (1-bit) field ****
            if (TESTBITS(ptr_vcm_virtual_chan_record3->chan_type_octet3, 7, 7))
            {
                // error, bits not set to 0.
                //svct_error = 27;
                RDK_LOG(
                        RDK_LOG_TRACE6,
                        "LOG.RDK.SI",
                        "<%s> - *** WARNING (%d) *** 1-Bit-Zero Field #5 not set to all zeroes(0)\n.",
                        __FUNCTION__, i);
            }
            if (TESTBITS(ptr_vcm_virtual_chan_record3->chan_type_octet3, 6, 6))
            {
                path_select = PATH_2_SELECTED;
            }

            RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
                    "<%s> - path_select(%d) = (%d).\n",
                    __FUNCTION__, i, path_select);
            RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
                    "\n<parseVCMStructure> - transport_type = %d\n",
                    transport_type);

            if (TESTBITS(ptr_vcm_virtual_chan_record3->chan_type_octet3, 5, 5))
            {
                transport_type = NON_MPEG_2_TRANSPORT;
                // svct_error     = 28;        //per Prasanna, remove when we want to process this type
                program_number = 0; // HACK - need to grok non-MPEG2 types, need set-er on SIDB
                mms_reference = 0;

                RDK_LOG(
                        RDK_LOG_TRACE6,
                        "LOG.RDK.SI",
                        "<%s> - *** WARNING *** VCM record %i is non-MPEG_2 transport_type - entering as program 0/mms 0.\n",
                        __FUNCTION__, i);
            }
            else
            {
                transport_type = MPEG_2_TRANSPORT;
            }

            RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
                    "<%s> - transport_type(%d) = (%d).\n",
                    __FUNCTION__, i, transport_type);

            channel_type = (uint16_t)(GETBITS(
                    ptr_vcm_virtual_chan_record3->chan_type_octet3, 4, 1));

            RDK_LOG(RDK_LOG_TRACE3, "LOG.RDK.SI",
                    "<%s> - channel_type(%d) = (%d).\n",
                    __FUNCTION__, i, channel_type);

            if (channel_type > HIDDEN_CHANNEL)
            {
                // error, reserved channel type.
                //svct_error = 29;
                RDK_LOG(
                        RDK_LOG_TRACE6,
                        "LOG.RDK.SI",
                        "<%s> - *** WARNING (%d) *** Reserved Channel_Type Value (2...15) used.\n",
                        __FUNCTION__, i);
            }

            RDK_LOG(
                    RDK_LOG_TRACE6,
                    "LOG.RDK.SI",
                    "<%s> - virtual_channel_number(%d) = %d channel_type(%d) = %d.\n",
                    __FUNCTION__, i, virtual_channel_number, i, channel_type);

            source_id
                    = (uint16_t)(
                            (ptr_vcm_virtual_chan_record3->app_or_source_id_octet4
                                    << 8)
                                    | ptr_vcm_virtual_chan_record3->app_or_source_id_octet5);
            application_id = 0;

            if (application_virtual_channel)
            {
                application_id = source_id;
                RDK_LOG(
                        RDK_LOG_TRACE6,
                        "LOG.RDK.SI",
                        "<%s> - application_id(%d)(0x%x) = (%d).\n",
                        __FUNCTION__, i, application_id, application_id);

                if (application_id == 0)
                {
                    // error, must be > 0 and <= 0xFFFF.
                    svct_error = 30;
                }
                // application virtual channels are supported (not considered error)
                RDK_LOG(
                        RDK_LOG_TRACE6,
                        "LOG.RDK.SI",
                        "<%s> - application_id(%d) = (%d).\n",
                        __FUNCTION__, i, application_id);
            }
            else
            {
                RDK_LOG(
                        RDK_LOG_TRACE6,
                        "LOG.RDK.SI",
                        "<%s> - source_id(%d)(0x%x) = (%d).\n",
                        __FUNCTION__, i, source_id, source_id);
            }

            cds_reference = ptr_vcm_virtual_chan_record3->cds_reference_octet6;

            //         RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI","<%s> - cds_reference(%d) = (%d).\n", __FUNCTION__,i, cds_reference);

            if ((transport_type == MPEG_2_TRANSPORT) && (cds_reference == 0))
            {
                // error, must be >=1
                svct_error = 32;
            }

            //         RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI","\n<parseVCMStructure> - transport_type = %d\n", transport_type);
            //         RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI","\n<parseVCMStructure> - program_number = %d\n", program_number);

            //***********************************************************************************************
            //**** Adjust section length and pointer to table section to account for having parsed
            //**** the vcm_virtual_chan_record3. Now parse either the mpeg2_virtual_chan_struc4
            //**** or nmpeg2_virtual_chan_struc5 portion of the the Virtual Channel Record .
            //***********************************************************************************************
            *pptr_TableSection += sizeof(vcm_virtual_chan_record3);
            *ptr_section_length -= sizeof(vcm_virtual_chan_record3);

            RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
                    "<%s> - section_length12 = (%d).\n",
                    __FUNCTION__, *ptr_section_length);
            RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
                    "<%s> - ptr_TableSection12 = (%p).\n",
                    __FUNCTION__, *pptr_TableSection);

            if (transport_type == MPEG_2_TRANSPORT)
            {
                ptr_mpeg2_virtual_chan_struc4
                        = (mpeg2_virtual_chan_struc4 *) *pptr_TableSection;
                size_struc_used = sizeof(mpeg2_virtual_chan_struc4);

                program_number
                        = (uint16_t)(
                                (ptr_mpeg2_virtual_chan_struc4->program_number_octet1
                                        << 8)
                                        | ptr_mpeg2_virtual_chan_struc4->program_number_octet2);
                RDK_LOG(
                        RDK_LOG_TRACE6,
                        "LOG.RDK.SI",
                        "<%s> - program_number(%d) = (%d).\n",
                        __FUNCTION__, i, program_number);

                mms_reference
                        = ptr_mpeg2_virtual_chan_struc4->mms_number_octet3;

                RDK_LOG(
                        RDK_LOG_TRACE6,
                        "LOG.RDK.SI",
                        "<%s> - mms_reference(%d) = (%d).\n",
                        __FUNCTION__, i, mms_reference);

                if (mms_reference == 0)
                {
                    // error, must be >=1
                    svct_error = 33;
                }
            }
            else
            {
                ptr_nmpeg2_virtual_chan_struc5
                        = (nmpeg2_virtual_chan_struc5 *) *pptr_TableSection;
                size_struc_used = sizeof(nmpeg2_virtual_chan_struc5);

                if (TESTBITS(
                        ptr_nmpeg2_virtual_chan_struc5->video_standard_octet1,
                        8, 8))
                {
                    scrambled = YES;
                }

                RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
                        "<%s> - scrambled(%d) = (%d).\n",
                        __FUNCTION__, i, scrambled);

                //**** validate "Zero" (3-bit) field ****
                if (TESTBITS(
                        ptr_nmpeg2_virtual_chan_struc5->video_standard_octet1,
                        7, 5))
                {
                    // error, 1 or more bits are set to 1 .
                    //svct_error = 34;
                    RDK_LOG(
                            RDK_LOG_WARN,
                            "LOG.RDK.SI",
                            "<%s> - *** WARNING (%d) *** 3-Bit-Zero Field #6 not set to all zeroes(0)\n.",
                            __FUNCTION__, i);
                }

                video_standard = (uint16_t)(GETBITS(
                        ptr_nmpeg2_virtual_chan_struc5->video_standard_octet1,
                        4, 1));

                RDK_LOG(
                        RDK_LOG_TRACE6,
                        "LOG.RDK.SI",
                        "<%s> - video_standard(%d) = (%d).\n",
                        __FUNCTION__, i, video_standard);

                if (video_standard > MAC_VIDEO_STANDARD)
                {
                    // error, range is NTSC_VIDEO_STANDARD (0) thru MAC_VIDEO_STANDARD (4) .
                    svct_error = 35;
                }

                if ((ptr_nmpeg2_virtual_chan_struc5->eight_zeroes_octet2)
                        || (ptr_nmpeg2_virtual_chan_struc5->eight_zeroes_octet3))
                {
                    // error, 1 or more bits are set to 1.
                    //svct_error = 36;
                    RDK_LOG(
                            RDK_LOG_TRACE6,
                            "LOG.RDK.SI",
                            "<%s> - *** WARNING (%d) *** 16-Bit-Zero Field #7 not set to all zeroes(0)\n.",
                            __FUNCTION__, i);
                }
            }

            //***********************************************************************************************
            //**** Adjust section length and pointer to table section buffer to account for having parsed
            //**** the mpeg2_virtual_chan_struc4/mpeg2_virtual_chan_struc5 stucture and prepare to
            //**** to process descriptors.
            //***********************************************************************************************
            *pptr_TableSection += size_struc_used;
            *ptr_section_length -= size_struc_used;

            RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
                    "<%s> - section_length13 = (%d).\n",
                    __FUNCTION__, *ptr_section_length);
            RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
                    "<%s> - ptr_TableSection13 = (%p).\n",
                    __FUNCTION__, *pptr_TableSection);

            if (*ptr_section_length <= 0)
            {
                if (*ptr_section_length < 0)
                {
                    // error, section length was shorter than the number of octets/bytes parsed.
                    svct_error = 37;
                    break;
                }
                else if (descriptors_included == YES)
                {
                    // error, section length (0) is not large enough to hold any descriptors
                    svct_error = 38;
                    break;
                }
            }

            //*******************************************************************************************
            //    Process descriptors.
            //*******************************************************************************************
            if (descriptors_included == YES)
            {
                descriptor_count = **pptr_TableSection; // get 8-bit descriptor count
                ++(*pptr_TableSection);
                --(*ptr_section_length);

                RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
                        "<%s> - section_length14 = (%d).\n",
                        __FUNCTION__, *ptr_section_length);
                RDK_LOG(
                        RDK_LOG_TRACE6,
                        "LOG.RDK.SI",
                        "<%s> - ptr_TableSection14 = (%p).\n",
                        __FUNCTION__, *pptr_TableSection);
                RDK_LOG(
                        RDK_LOG_TRACE6,
                        "LOG.RDK.SI",
                        "<%s> - descriptor_count14 = (%d).\n",
                        __FUNCTION__, descriptor_count);

                while (descriptor_count > 0)
                { // ignore all descriptors .

                    ptr_descriptor_struc
                            = (generic_descriptor_struct *) *pptr_TableSection;
                    *pptr_TableSection += sizeof(generic_descriptor_struct)
                            + ptr_descriptor_struc->descriptor_length_octet2;

                    //*** decrement section_length as we did not account for descriptor tag, length and data fields
                    //*** length and data fields prior to calling subroutine .

                    --descriptor_count;
                    *ptr_section_length -= (sizeof(generic_descriptor_struct)
                            + ptr_descriptor_struc->descriptor_length_octet2);

                    RDK_LOG(
                            RDK_LOG_TRACE6,
                            "LOG.RDK.SI",
                            "<%s> - section_length15 = (%d).\n",
                            __FUNCTION__, *ptr_section_length);
                    RDK_LOG(
                            RDK_LOG_TRACE6,
                            "LOG.RDK.SI",
                            "<%s> - ptr_TableSection15 = (%p).\n",
                            __FUNCTION__, *pptr_TableSection);
                    RDK_LOG(
                            RDK_LOG_TRACE6,
                            "LOG.RDK.SI",
                            "<%s> - descriptor_count14 = (%d).\n",
                            __FUNCTION__, descriptor_count);

                    if (*ptr_section_length < 0)
                    { //error, section_length not large enough to account for descriptor tags, length and data fields
                        svct_error = 40;
                        break;
                    }
                } // end_of_while loop
            }

            //*******************************************************************************************
            //    Update the SI database with the reqired data for this Virtual Channel Record.
            //*******************************************************************************************
            RDK_LOG(
                    RDK_LOG_TRACE6,
                    "LOG.RDK.SI",
                    "<%s> - Virtual Channel Record (%d) parse %s.\n",
                    __FUNCTION__, i, svct_error == 0 ? "complete - no errors"
                            : "complete - errors");

            // - Add digital only processing here (skip analog channels)

            if (svct_error == NO_PARSING_ERROR)
            {
                si_database_handle = RMF_SI_INVALID_HANDLE;
                uint32_t si_state = 0;

                RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
                        "<%s> - source_id(%d) = (%d).\n",
                        __FUNCTION__, i, source_id);

                if (m_pSiCache->getDumpTables())
                {
                    if (transport_type == MPEG_2_TRANSPORT)
                    {
                        //printf
                        RDK_LOG(
                                RDK_LOG_INFO,
                                "LOG.RDK.SI",
                                "<%s> - Ent %-3u[VC:%u(0x%x) AVC:%d PS:%d CT:%d %s:%d(0x%x) "
                                    "TT:MPEG2(dig) CDS:%d Prog#:%d(0x%x) MMS:%d]\n",
                                __FUNCTION__, i, virtual_channel_number,
                                virtual_channel_number,
                                application_virtual_channel, path_select,
                                channel_type,
                                (application_virtual_channel ? "AID" : "SID"),
                                (application_virtual_channel ? application_id
                                        : source_id),
                                (application_virtual_channel ? application_id
                                        : source_id), cds_reference,
                                program_number, program_number, mms_reference);
                        ;
                    }
                    else
                    {
                        //printf
                        RDK_LOG(
                                RDK_LOG_INFO,
                                "LOG.RDK.SI",
                                "<%s> - Ent %-3u[VC:%d(0x%x) AVC:%d PS:%d CT:%d %s:%d(0x%x) "
                                    "TT:non-MPEG2(analog) CDS:%d scram:%d VS:%d]\n",
                                __FUNCTION__, i, virtual_channel_number,
                                virtual_channel_number,
                                application_virtual_channel, path_select,
                                channel_type,
                                (application_virtual_channel ? "AID" : "SID"),
                                (application_virtual_channel ? application_id
                                        : source_id),
                                (application_virtual_channel ? application_id
                                        : source_id), cds_reference, scrambled,
                                video_standard);
                        ;
                    }
                }

                m_pSiCache->LockForWrite();

                // Look up SI DB handle based on channel number
                // This creates a new handle if one is not found
                m_pSiCache->GetServiceEntryFromChannelNumber(virtual_channel_number,
                        &si_database_handle);
                if (si_database_handle != RMF_SI_INVALID_HANDLE)
                {
                    uint32_t tmpSourceId=0;
                    uint32_t tmp_app_id=0;
                    uint32_t tmp_prog_num=0;

                    m_pSiCache->GetServiceEntryState(si_database_handle, &si_state);
                    RDK_LOG(RDK_LOG_TRACE6,
                            "LOG.RDK.SI",
                            "<%s> - si_database_handle:0x%x si_state:%d\n",
                            __FUNCTION__, si_database_handle, si_state);
                    if((si_state & SIENTRY_MAPPED) == SIENTRY_MAPPED)
                    {
                        m_pSiCache->GetSourceIdForServiceHandle(si_database_handle, &tmpSourceId);
                        m_pSiCache->GetAppIdForServiceHandle(si_database_handle, &tmp_app_id);
                        m_pSiCache->GetProgramNumberForServiceHandle(si_database_handle, &tmp_prog_num);

                        if(application_virtual_channel == NO)
                        {
                            RDK_LOG(RDK_LOG_TRACE6,
                                    "LOG.RDK.SI",
                                    "<%s> - virtual_channel_number:%d source_id:%d tmpSourceId:%d  program_number:%d tmp_prog_num:%d\n",
                                    __FUNCTION__, virtual_channel_number, source_id, tmpSourceId, program_number, tmp_prog_num);
                        }
                        else
                        {
                            RDK_LOG(RDK_LOG_TRACE6,
                                    "LOG.RDK.SI",
                                    "<%s> - virtual_channel_number:%d appId:%d tmp_app_id:%d \n",
                                    __FUNCTION__, virtual_channel_number, application_id, tmp_app_id);
                        }

                        // Clone the entry only if the virtual channel number is 0
                        // (It was observed on RI that virtual channel 0 appeared many times
                        // in the VCM with diff sourceId and/or program number)
                        // This may be a special virtual channel number which the cable
                        // operators use to signal discrete sourceIds that are still discoverable
                        // by apps.
                        // In all other cases (virtual_channel_number != 0) if an existing SI DB entry
                        // is found with this virtual channel number it is updated
                        if( ((application_virtual_channel == YES) && (tmp_app_id != application_id))
                            || ( (application_virtual_channel == NO)
                                 &&  (virtual_channel_number == 0)
                                 && ((tmpSourceId != source_id)
                                      || (tmp_prog_num != program_number)) ) )
                        {

                            RDK_LOG(
                                    RDK_LOG_TRACE6,
                                    "LOG.RDK.SI",
                                    "<%s> - Found an mapped entry with this virtual channel number (but diff sourceId, fpq), cloning.. \n",
                                    __FUNCTION__);
#if 0
                            if (1) {
                                RDK_LOG(
                                        RDK_LOG_ERROR,
                                        "LOG.RDK.SI",
                                        "<%s> - Parker-6049: Updating an mapped entry with this virtual channel number (but diff sourceId, fpq), cloning.. \n",
                                        __FUNCTION__);
                                rmf_SiTransportStreamHandle ts_handle = RMF_SI_INVALID_HANDLE;
                                rmf_SiProgramHandle prog_handle = RMF_SI_INVALID_HANDLE;
                                uint32_t frequency_by_cds_ref = 0;
                                rmf_ModulationMode modulation;
                                if (transport_type == NON_MPEG_2_TRANSPORT)
                                {
                                    modulation = RMF_MODULATION_QAM_NTSC;
                                }
                                else
                                {
                                    rmf_ModulationMode mode_by_mms_ref = RMF_MODULATION_UNKNOWN;
                                    mpe_siGetModulationFromMMSRef(mms_reference, &mode_by_mms_ref);
                                    modulation = mode_by_mms_ref;
                                }
                                RDK_LOG(
                                        RDK_LOG_TRACE4,
                                        "LOG.RDK.SI",
                                        "<%s> - modulation %d, transport type %d.. \n", __FUNCTION__, modulation, transport_type);
                                mpe_siGetFrequencyFromCDSRef(cds_reference, &frequency_by_cds_ref);
                                mpe_siGetTransportStreamEntryFromFrequencyModulation( frequency_by_cds_ref, modulation, &ts_handle);
                                (void) mpe_siGetProgramEntryFromTransportStreamEntry(ts_handle, program_number, &prog_handle);
                                mpe_siUpdateServiceEntry((rmf_SiServiceHandle)si_database_handle, ts_handle, prog_handle);
                            }
                            else {
                                (void)mpe_siInsertServiceEntryForChannelNumber(virtual_channel_number, &si_database_handle);
                            }
#endif
                            (void)m_pSiCache->SetServiceEntryState(si_database_handle, si_state);
                        }
                    }
                    if(application_virtual_channel == NO)
                    {
                        (void) m_pSiCache->SetSourceId(si_database_handle, source_id);
			if(virtual_channel_number != 0)
			{
			   if ( CHMAP_SOURCEID == source_id ) 
			   {
				   uint16_t chmap_id;
				   m_pSiCache->GetChannelMapId(&chmap_id);
				if( virtual_channel_number != chmap_id )
				{
	                        RDK_LOG(
                               RDK_LOG_NOTICE,
                               "LOG.RDK.SI",					
                               "virtual_channel_number changed  for channel map id old = %x, new = %x\n", chmap_id, virtual_channel_number );
				   m_pSiCache->SetChannelMapId( virtual_channel_number );
				   if( rmf_OobSiCache::rmf_siGZCBfunc )
				   {
					   rmf_OobSiCache::rmf_siGZCBfunc( RMF_OOBSI_GLI_CHANNELMAP_ID_CHANGED, virtual_channel_number, 
									((m_pSiCache->IsSIFullyAcquired() == TRUE ) ?  1 : 0 ) );
				   }
				}
			   }
                        else if( DAC_SOURCEID == source_id )
                        {
							uint16_t dac_id;
							m_pSiCache->GetDACId(&dac_id);
				if( virtual_channel_number != dac_id )
				{ 
                               RDK_LOG(
                               RDK_LOG_NOTICE,
                               "LOG.RDK.SI",	
                               "virtual_channel_number changed  for channel dac_id old = %x, new = %x\n", dac_id, virtual_channel_number );
				   m_pSiCache->SetDACId( virtual_channel_number );		
				   if( rmf_OobSiCache::rmf_siGZCBfunc )
				   {
					   rmf_OobSiCache::rmf_siGZCBfunc( RMF_OOBSI_GLI_DAC_ID_CHANGED, virtual_channel_number, 0 );
				   }				   
				}
                        }
			}
                    }
                    else
                    {
                        (void) m_pSiCache->SetAppId(si_database_handle, application_id);	  
                    }

                    m_pSiCache->SetAppType(si_database_handle, application_virtual_channel);
                    m_pSiCache->SetActivationTime(si_database_handle, activation_time);
                    //Since we do not have the two part channel number, use the virtual as the the major and the DEFAULT as
                    //the minor
                    (void) m_pSiCache->SetChannelNumber(si_database_handle,
                            virtual_channel_number,
                            RMF_SI_DEFAULT_CHANNEL_NUMBER);
                    m_pSiCache->SetCDSRef(si_database_handle, (uint8_t) cds_reference);
                    m_pSiCache->SetMMSRef(si_database_handle, (uint8_t) mms_reference);
                    m_pSiCache->SetChannelType(si_database_handle,
                            (uint8_t) channel_type);
                    /* We don't want to merge this Axiom fix, but rather allow multiple transport streams
                     be created with same frequency but different modulation modes.*/
                    /*
                     if (channel_type != CHANNEL_TYPE_HIDDEN)
                     {   // Ensure non-hidden sources 'win' if there's a modulation conflict.
                     rmf_SiTransportStreamEntry *ts = (rmf_SiTransportStreamEntry *)((rmf_SiTableEntry *)si_database_handle)->ts_handle;
                     rmf_ModulationMode modulation = (transport_type == NON_MPEG_2_TRANSPORT) ? RMF_MODULATION_QAM_NTSC : mode_by_mms_ref;
                     if (ts != NULL)
                     {
                     if (ts->modulation_mode != modulation)
                     {
                     RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI","<%s> - reset modulation mode for frequency %d from %d to %d\n", __FUNCTION__, ts->frequency, ts->modulation_mode, mode_by_mms_ref);
                     ts->modulation_mode = modulation;
                     }
                     }
                     }
                     */
                    m_pSiCache->SetProgramNumber(si_database_handle, program_number);
                    m_pSiCache->SetTransportType(si_database_handle, transport_type);
                    m_pSiCache->SetScrambled(si_database_handle, scrambled);
                    if (transport_type == NON_MPEG_2_TRANSPORT)
                    {
                        RDK_LOG(
                                RDK_LOG_TRACE6,
                                "LOG.RDK.SI",
                                "<%s> - setting analog modulation mode.\n",
                                __FUNCTION__);
                        m_pSiCache->SetVideoStandard(si_database_handle,
                                (rmf_SiVideoStandard)video_standard);

                        // Kludge!!! (Since we cannot determine service_type from profile-1 SI
                        // we are just going to use the 'transport_type' field in SVCT to
                        // determine the service type.
                        //mpe_siSetServiceType(si_database_handle, RMF_SI_SERVICE_ANALOG_TV);
                    }
                    else if (transport_type == MPEG_2_TRANSPORT)
                    {
                        // Not sure if we can assume that if transport_type is
                        // MPEG2 that the service is digital.
                        // How to differentiate between DIGITAL_TV, DIGITAL_RADIO
                        // and DATA_BROADCAST etc.

                        // Kludge!!! (Since we cannot determine service_type from profile-1 SI
                        // we are just going to use the 'transport_type' field in SVCT to
                        // determine the service type.  It is important to set to unknown
                        // here, in case we're doing an update and the si element was previously
                        // used by an analog service.
                        m_pSiCache->SetServiceType(si_database_handle,
                                RMF_SI_SERVICE_TYPE_UNKNOWN);
                    }
                    m_pSiCache->SetServiceEntryStateMapped(si_database_handle);
                }
                m_pSiCache->ReleaseWriteLock();
            }
            else
            {
                saved_svct_error = svct_error;
                svct_error = NO_PARSING_ERROR;
                if (saved_svct_error == 40)
                { //error, section_length not large enough To account for descriptor tags, length and data fields
                    break;
                }
            }
        } //end_of_for_loop
    }

    (void) svct_error;

    RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
            "<%s> - Exiting with a SVCT_Error_Code = %d.\n",
            __FUNCTION__, saved_svct_error);
    return saved_svct_error;
}

/*
 ** ****************************************************************************
 **  Subroutine/Method:     parseDCMStructure()
 ** ****************************************************************************
 **
 ** Parses Defined Channels Map(DCM) structure, validates the fields within the structure
 **
 ** @param  pptr_TableSection,  the address of memeory location which contains a pointer to the SVCT
 **                             table section.
 ** @param  ptr_section_length, a pointer to the caller's section_length variable.
 **
 ** @return RMF_SI_SUCCESS if table section was successfully processed/parsed .Otherwise an error code
 **         greater than zero (RMF_SI_SUCCESS).
 **
 */
rmf_Error rmf_OobSiParser::parseDCMStructure(uint8_t **pptr_TableSection, int32_t *ptr_section_length)
{
    dcm_record_struc2 * ptr_dcm_record_struc2;
    dcm_record_struc3 * ptr_dcm_virtual_chan_record3;
    uint16_t first_virtual_channel = 0, i = 0, j = 0, range_defined = NO,
            channels_count = 0;
    uint8_t dcm_data_length = 0;
    rmf_Error svct_error = NO_PARSING_ERROR;
    rmf_Error saved_svct_error = NO_PARSING_ERROR;
    uint16_t virtual_channel_count = 0, virtual_channel = 0;
    rmf_SiServiceHandle si_database_handle = 0;
    uint32_t total_count = 0;
    //***********************************************************************************************
    //***********************************************************************************************

    //***********************************************************************************************
    //**** Parse the  DCM Structure.
    //***********************************************************************************************
    RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
            "<%s> - ptr_TableSection = (%p).\n", __FUNCTION__,
            *pptr_TableSection);

    ptr_dcm_record_struc2 = (dcm_record_struc2 *) *pptr_TableSection;

    //**** validate "Zero" (2-bits) field ****
    if (TESTBITS(ptr_dcm_record_struc2->first_virtual_chan_octet1, 8, 5))
    {
        // error, 1 or more bits are set to 1.
        //svct_error = 26;
        RDK_LOG(
                RDK_LOG_TRACE6,
                "LOG.RDK.SI",
                "<%s> - *** WARNING (%d) *** 4-Bit-Zero Field #4 not set to all zeroes(0)\n.",
                __FUNCTION__, i);
    }

    // range 0 thru 4095 ((2**12)-1)
    first_virtual_channel = (uint16_t)((GETBITS(
            ptr_dcm_record_struc2->first_virtual_chan_octet1, 4, 1) << 8)
            | ptr_dcm_record_struc2->first_virtual_chan_octet2);
    RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
            "<%s> - first_virtual_channel(%d) = (%d).\n",
            __FUNCTION__, i, first_virtual_channel);

    dcm_data_length = ptr_dcm_record_struc2->dcm_data_length_octet_3;

    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SI",
            "<%s> - dcm_data_length = (%d).\n", __FUNCTION__,
            dcm_data_length);

    if (dcm_data_length == 0)
    {
        // error,must be > 0.
        svct_error = 24;
    }

    virtual_channel = first_virtual_channel;

    //***********************************************************************************************
    //*** Adjust pointers to reflect the parsing of the DCM Structure (dcm_record_struc2).
    //***********************************************************************************************
    *pptr_TableSection += sizeof(dcm_record_struc2);
    *ptr_section_length -= sizeof(dcm_record_struc2);
    RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
            "<%s> - section_length11 = (%d).\n", __FUNCTION__,
            *ptr_section_length);
    RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
            "<%s> - ptr_TableSection11 = (%p).\n", __FUNCTION__,
            *pptr_TableSection);

    if (*ptr_section_length <= 0)
    {
        // error, either DCM structure was too short (*ptr_section_length < 0) or
        //        there is no virtual channel record (*ptr_section_length = 0).
        svct_error = 25;
    }

    //***********************************************************************************************
    //**** Parse DCM data section
    //***********************************************************************************************
    if (svct_error == NO_PARSING_ERROR)
    {
        do
        {
            ptr_dcm_virtual_chan_record3
                    = (dcm_record_struc3 *) *pptr_TableSection;

            if (TESTBITS(ptr_dcm_virtual_chan_record3->channel_count_octet1, 8,
                    8))
            {
                range_defined = YES;
            }
            else
            {
                range_defined = NO;
            }

            RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
                    "<%s> - range_defined(%d) = %d\n",
                    __FUNCTION__, i, range_defined);

            channels_count = GETBITS(
                    ptr_dcm_virtual_chan_record3->channel_count_octet1, 7, 1);

            total_count += channels_count;
            RDK_LOG(
                    RDK_LOG_TRACE6,
                    "LOG.RDK.SI",
                    "<%s> - range_defined = %d, channels_count = %d, total_count = %d\n",
                    __FUNCTION__, range_defined, channels_count, total_count);

            m_pSiCache->LockForWrite();
            for (j = 0; j < channels_count; j++)
            {
                // Look up SI DB handle for each virtual channel starting at first channel
                // If a match is not found this will create a new entry
                m_pSiCache->GetServiceEntryFromChannelNumber(virtual_channel,
                        &si_database_handle);

                if (range_defined)
                {
                    RDK_LOG(
                            RDK_LOG_TRACE6,
                            "LOG.RDK.SI",
                            "<%s> - virtual_channel(%d) = %d\n",
                            __FUNCTION__, virtual_channel_count, virtual_channel);
                    // If 'range_defined' is set, mark this channel as defined
                    m_pSiCache->SetServiceEntryStateDefined(si_database_handle);
                    // If duplicate entries exist in the database with this channel number
                    // set the defined state
                    m_pSiCache->SetServiceEntriesStateDefined(virtual_channel);

                    virtual_channel_count++;
                }
                else
                {
                    // If 'range_defined' is NOT set, mark this channel as undefined
                    m_pSiCache->SetServiceEntryStateUnDefined(si_database_handle);
                    // If duplicate entries exist in the database with this channel number
                    // set the undefined state
                    m_pSiCache->SetServiceEntriesStateUnDefined(virtual_channel);
                }
                // Increment the channel counter
                virtual_channel++;
            }
            m_pSiCache->ReleaseWriteLock();

            *pptr_TableSection += sizeof(dcm_record_struc3);
            *ptr_section_length -= sizeof(dcm_record_struc3);

            dcm_data_length--;

            if (*ptr_section_length <= 0)
            {
                // error, either DCM structure was too short (*ptr_section_length < 0) or
                //        there is no virtual channel record (*ptr_section_length = 0).
                // not used svct_error = 26 ;
            }
        } while (dcm_data_length != 0);
    }
    else
    {
        saved_svct_error = svct_error;
    }

    RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
            "<%s> - Exiting with a SVCT_Error_Code = %d.\n",
            __FUNCTION__, saved_svct_error);
    return saved_svct_error;
}

/**
 * @brief This function identifies the SI table such as NTT, NIT, STT and so. It also parses the
 * Revision Detection Descriptor of it to get its version, section_number, crc and last_section_number.
 *
 * @param[in] section_handle Indicates the handle to section data.
 * @param[out] version Indicates version of the current table.
 * @param[out] section_number Identifies the current table section number.
 * @param[out] last_section_number Identifies the number of sections in a table.
 * @param[out] crc Indicates the Cyclic Redundancy Check value specified in the table.
 *
 * @return Returns RMF_SUCCESS if the call is successfull else returns RMF_SI_PARSE_ERROR.
 */
rmf_Error rmf_OobSiParser::get_revision_data(rmf_FilterSectionHandle section_handle,
        uint8_t * version, uint8_t * section_number, uint8_t * last_section_number,
        uint32_t * crc)
{
    rmf_Error retCode = RMF_SUCCESS;
    uint8_t table_id = 0;
    uint32_t number_bytes_copied = 0;
    uint8_t input_buffer[1];

    RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI", "<%s::get_revision_data> - Enter\n",
            SIMODULE);

    retCode = m_pSectionFilter->ReadSection(section_handle, 0, sizeof(uint8_t), 0,
            input_buffer, &number_bytes_copied);
    
    /*
     ** get the table_id.
     */
    table_id =  input_buffer[0];
    switch (table_id)
    {
    case NETWORK_INFORMATION_TABLE_ID:
        retCode = parseNIT_revision_data(section_handle, version,
                section_number, last_section_number, crc);
        break;
    case SF_VIRTUAL_CHANNEL_TABLE_ID:
        retCode = parseSVCT_revision_data(section_handle, version,
                section_number, last_section_number, crc);
        break;
    case NETWORK_TEXT_TABLE_ID:
        retCode = parseNTT_revision_data(section_handle, version,
                section_number, last_section_number, crc);
        break;
    case SYSTEM_TIME_TABLE_ID:
        retCode = parseSTT_revision_data(section_handle, version,
                section_number, last_section_number, crc);
        break;
    default:
        RDK_LOG(
                RDK_LOG_ERROR,
                "LOG.RDK.SI",
                "<%s::get_revision_data> - matching version parser not found for table_id: 0x%x\n",
                SIMODULE, table_id);
        return RMF_SI_PARSE_ERROR;
    }

    if (true == m_pSiCache->isVersioningByCRC32())
    {
        RDK_LOG(
                RDK_LOG_TRACE4,
                "LOG.RDK.SI",
                "<%s::get_revision_data> - Versioning by CRC only. Resetting version value- crc: 0x%08x\n",
                SIMODULE, *crc);
        *version = NO_VERSION;
        *section_number = NO_VERSION;
        *last_section_number = NO_VERSION;
    }
    else
    {
        RDK_LOG(
                RDK_LOG_TRACE4,
                "LOG.RDK.SI",
                "<%s::get_revision_data> - version: %d, section_number: %d, last_section_number: %d, crc = 0x%08x\n",
                SIMODULE, *version, *section_number, *last_section_number, *crc);
    }
    RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI", "<%s::get_revision_data> - Exit\n",
            SIMODULE);

    return retCode;
}

/*
 ** *****************************************************************************
 ** Subroutine/Method:                       parseNIT_revision_data()
 ** *****************************************************************************
 **
 ** Parse the RDD descriptor in the NIT table section.
 **
 ** @param  section_handle    -  the section handle for the NIT table section.
 ** @param  * version         -  ptr to the version uint8_t to be filled in.
 ** @param  * section_number  -  ptr to the index of this section in the total
 **                                number of sections.
 ** @param  * number_sections -  ptr to the the total number of sections to be
 **                              filled out in this function.
 **
 ** @return RMF_SUCCESS if RDD section was successfully processed/parsed .
 ** Otherwise an error code greater than zero (RMF_SI_PARSE_ERROR).
 ** *****************************************************************************
 */
rmf_Error rmf_OobSiParser::parseNIT_revision_data(rmf_FilterSectionHandle section_handle,
        uint8_t * version, uint8_t * section_number, uint8_t * number_sections,
        uint32_t * crc)
{
    rmf_Error retCode = RMF_SUCCESS;

    nit_table_struc1 * ptr_nit_table_struc1 = NULL;
    revision_detection_descriptor_struc * ptr_rdd_table_struc1 = NULL;
    generic_descriptor_struct * ptr_gen_desc_struc = NULL;

    uint8_t record_size = 0;
    uint8_t descriptor_count = 0;
    uint8_t number_of_records = 0;
    uint8_t table_subtype = 0;
    uint16_t section_length = 0;
    uint32_t number_bytes_copied = 0;
    uint32_t offset = 0;
    size_t ii;

    uint8_t input_buffer[MAX_OOB_SMALL_TABLE_SECTION_LENGTH];

    RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
            "<%s::parseNIT_revision_data> - Enter\n", SIMODULE);

    retCode = m_pSectionFilter->ReadSection(section_handle, 0,
            MAX_OOB_SMALL_TABLE_SECTION_LENGTH, 0, input_buffer,
            &number_bytes_copied);
    if (retCode != RMF_SUCCESS)
        return RMF_SI_PARSE_ERROR;

    ptr_nit_table_struc1 = (nit_table_struc1 *) (input_buffer + offset);

    section_length = ((GETBITS(ptr_nit_table_struc1->section_length_octet2, 4,
            1)) << 8) | ptr_nit_table_struc1->section_length_octet3;

    number_of_records = ptr_nit_table_struc1->number_of_records_octet6;
    RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
            "<%s::parseNIT_revision_data> - Number_Records = (%d).\n",
            SIMODULE, number_of_records);

    //**** extract and validate the table_subtype field ****
    table_subtype = GETBITS(ptr_nit_table_struc1->table_subtype_octet7, 4, 1);
    RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
            "<%s::parseNIT_revision_data> - Table_Subtype = (%d).\n", SIMODULE,
            table_subtype);

    if ((table_subtype < CARRIER_DEFINITION_SUBTABLE) || (table_subtype
            > MODULATION_MODE_SUBTABLE))
    {
        RDK_LOG(
                RDK_LOG_ERROR,
                "LOG.RDK.SI",
                "<%s::parseNIT_revision_data> - Error Table_Subtype not handled: (%d).\n",
                SIMODULE, table_subtype);
        return RMF_SI_PARSE_ERROR;
    }

    offset += sizeof(nit_table_struc1);

    /* subtract the header and the CRC */
    section_length -= (sizeof(nit_table_struc1) - LEN_OCTET1_THRU_OCTET3);

    record_size
            = (table_subtype == CARRIER_DEFINITION_SUBTABLE) ? sizeof(cds_record_struc2)
                    : sizeof(mms_record_struc2);

    RDK_LOG(
            RDK_LOG_TRACE6,
            "LOG.RDK.SI",
            "<%s::parseNIT_revision_data> - section length after header - section_length: %d\n",
            SIMODULE, section_length);

    while (number_of_records--)
    {
        /* skip the record */
        offset += record_size;
        section_length -= record_size;

        RDK_LOG(
                RDK_LOG_TRACE6,
                "LOG.RDK.SI",
                "<%s::parseNIT_revision_data> - record count: %d - section length: %d\n",
                SIMODULE, number_of_records, section_length);

        descriptor_count = *(input_buffer + offset);
        RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
                "<%s::parseNIT_revision_data> - descriptor_count: %d\n",
                SIMODULE, descriptor_count);

        offset++;
        section_length--;
        ptr_gen_desc_struc = (generic_descriptor_struct *) (input_buffer
                + offset);

        /* skip the inner descriptors */
        while (descriptor_count--)
        {
            offset += (ptr_gen_desc_struc->descriptor_length_octet2
                    + sizeof(generic_descriptor_struct));
            section_length -= (ptr_gen_desc_struc->descriptor_length_octet2
                    + sizeof(generic_descriptor_struct));
        }
    }

    /*
     * This is the beginning of the outer descriptor.  The RDD should
     * be the first one.  Verify that the remaining section_length is
     * >= the RDD descriptor before we attempt to parse it.
     */

    RDK_LOG(
            RDK_LOG_TRACE6,
            "LOG.RDK.SI",
            "<%s::parseNIT_revision_data> - section_length: %d, sizeof(rdd):%d\n",
            SIMODULE, section_length,
            sizeof(revision_detection_descriptor_struc));

    /*
     * Parse and return the RDD values.
     */
    RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
            "<%s::parseNIT_revision_data> - number bytes read: %d, offset: %d",
            SIMODULE, number_bytes_copied, offset);

    RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
            "<%s::parseNIT_revision_data> - RDD and crc bytes: ", SIMODULE);
    for (ii = 0; ii < (sizeof(revision_detection_descriptor_struc)
            + LENGTH_CRC_32); ii++)
        RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI", "0x%x ", input_buffer[offset + ii]);
    RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI", "\n");

    /*
     * Get the RDD or skip the descriptors and then get the CRC
     */

    if (section_length > LENGTH_CRC_32)
    {
        ptr_rdd_table_struc1
                = (revision_detection_descriptor_struc *) (input_buffer
                        + offset);

        if (ptr_rdd_table_struc1->descriptor_tag_octet1 != 0x93)
        {
            RDK_LOG(
                    RDK_LOG_DEBUG,
                    "LOG.RDK.SI",
                    "<%s::parseNIT_revision_data> - descriptor not a revision descriptor - tag: 0x%x - skip all descriptors\n",
                    SIMODULE, ptr_rdd_table_struc1->descriptor_tag_octet1);

            /*
             * Set the RDD values to the NO_VERSION macro
             */
            *version = NO_VERSION;
            *section_number = NO_VERSION;
            *number_sections = NO_VERSION;

            while (section_length > LENGTH_CRC_32)
            {
                //There must be a descriptor other than the RDD
                ptr_gen_desc_struc = (generic_descriptor_struct *) (input_buffer
                        + offset);
                offset += sizeof(generic_descriptor_struct)
                        + ptr_gen_desc_struc->descriptor_length_octet2;
                section_length -= (sizeof(generic_descriptor_struct)
                        + ptr_gen_desc_struc->descriptor_length_octet2);

                if (section_length <= 0)
                {
                    RDK_LOG(
                            RDK_LOG_ERROR,
                            "LOG.RDK.SI",
                            "<%s::parseNIT_revision_data> - error, section_length not large enough to accommodate descriptor(s) and crc.\n",
                            SIMODULE);
                    break;
                }
            }
        }
        else
        {
            *version = (ptr_rdd_table_struc1->table_version_number_octet3
                    & 0x1F);
            *section_number = ptr_rdd_table_struc1->section_number_octet4;
            *number_sections = ptr_rdd_table_struc1->last_section_number_octet5;
            RDK_LOG(
                    RDK_LOG_TRACE4,
                    "LOG.RDK.SI",
                    "<%s::parseNIT_revision_data> - version: %d, section_number: %d, last_section_number: %d\n",
                    SIMODULE, *version, *section_number, *number_sections);
            offset += sizeof(revision_detection_descriptor_struc);
            section_length -= sizeof(revision_detection_descriptor_struc);
        }
        if (section_length == LENGTH_CRC_32)
        {
            *crc = ((*(input_buffer + offset)) << 24 | (*(input_buffer + offset
                    + 1)) << 16 | (*(input_buffer + offset + 2)) << 8
                    | (*(input_buffer + offset + 3)));
            RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                    "<%s::parseNIT_revision_data> - CRC32 = 0x%08x\n",
                    SIMODULE, *crc);

            if ((*crc == 0xFFFFFFFF) || (*crc == 0x00000000))
            {
                RDK_LOG(
                        RDK_LOG_TRACE4,
                        "LOG.RDK.SI",
                        "<%s::parseNIT_revision_data> - Invalid CRC value (0x%08x) - calculating CRC\n",
                        SIMODULE, *crc);
                // Note: We don't really need to use a particular CRC here - just a decent one - since we're (presumably) going to be checking
                //       against our calculated CRCs in this case (unless a crazy plant *and* POD send us some with and some without CRCs...)
                *crc = m_pSiCache->calc_mpeg2_crc(input_buffer, offset + 4); // Need to account for the 4 bytes of CRC in the length
            }
        }
    }
    else if (section_length == LENGTH_CRC_32)
    {
        /*
         * Set the RDD values to the NO_VERSION macro
         */
        *version = NO_VERSION;
        *section_number = NO_VERSION;
        *number_sections = NO_VERSION;

        *crc = ((*(input_buffer + offset)) << 24 | (*(input_buffer + offset
                + 1)) << 16 | (*(input_buffer + offset + 2)) << 8
                | (*(input_buffer + offset + 3)));
        RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                "<%s::parseNIT_revision_data> - CRC32 = 0x%08x\n",
                SIMODULE, *crc);

        if ((*crc == 0xFFFFFFFF) || (*crc == 0x00000000))
        {
            RDK_LOG(
                    RDK_LOG_TRACE4,
                    "LOG.RDK.SI",
                    "<%s::parseNIT_revision_data> - Invalid CRC value (0x%08x) - calculating CRC\n",
                    SIMODULE, *crc);
            // Note: We don't really need to use a particular CRC here - just a decent one - since we're (presumably) going to be checking
            //       against our calculated CRCs in this case (unless a crazy plant *and* POD send us some with and some without CRCs...)
            *crc = m_pSiCache->calc_mpeg2_crc(input_buffer, offset + 4); // Need to account for the 4 bytes of CRC in the length
        }
    }
    else
    {
        RDK_LOG(
                RDK_LOG_ERROR,
                "LOG.RDK.SI",
                "<%s::parseNIT_revision_data> - Error - section not long enough for CRC32\n",
                SIMODULE);
    }
    return retCode;
}

/*
 ** *****************************************************************************
 ** Subroutine/Method:                       parseNTT_revision_data()
 ** *****************************************************************************
 **
 ** Parse the RDD descriptor in the NTT table section.
 **
 ** @param  section_handle    -  the section handle for the NTT table section.
 ** @param  * version         -  ptr to the version uint8_t to be filled in.
 ** @param  * section_number  -  ptr to the index of this section in the total
 **                                number of sections.
 ** @param  * number_sections -  ptr to the the total number of sections to be
 **                              filled out in this function.
 **
 ** @return RMF_SUCCESS if RDD section was successfully processed/parsed .
 ** Otherwise an error code greater than zero (RMF_SI_PARSE_ERROR).
 ** *****************************************************************************
 */
rmf_Error rmf_OobSiParser::parseNTT_revision_data(rmf_FilterSectionHandle section_handle,
        uint8_t * version, uint8_t * section_number, uint8_t * number_sections,
        uint32_t * crc)
{
    rmf_Error retCode = RMF_SUCCESS;

    revision_detection_descriptor_struc * ptr_rdd_table_struc1 = NULL;
    ntt_record_struc1 * ptr_ntt_record_struc1 = NULL;
    sns_record_struc1 * ptr_sns_record_struc1 = NULL;
    sns_record_struc2 * ptr_sns_record_struc2 = NULL;
    generic_descriptor_struct * ptr_generic_descriptor_struc = NULL;

    uint8_t input_buffer[MAX_OOB_SMALL_TABLE_SECTION_LENGTH];
    uint8_t *ptr_input_buffer = NULL;
    uint8_t number_of_SNS_records = 0;
    uint8_t name_length = 0;
    uint8_t table_subtype = 0;
    uint8_t SNS_descriptor_count = 0;
    uint8_t descriptor_length = 0;
    uint16_t section_length = 0;
    uint32_t number_bytes_copied = 0;
    uint32_t offset = 0;

    RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
            "<%s::parseNTT_revision_data> - Enter\n", SIMODULE);
    retCode = m_pSectionFilter->ReadSection(section_handle, 0,
            MAX_OOB_SMALL_TABLE_SECTION_LENGTH, 0, input_buffer,
            &number_bytes_copied);
    if (retCode != RMF_SUCCESS)
        return RMF_SI_PARSE_ERROR;

    ptr_ntt_record_struc1 = (ntt_record_struc1 *) input_buffer;

    section_length = ((GETBITS(ptr_ntt_record_struc1->section_length_octet2, 4,
            1)) << 8) | ptr_ntt_record_struc1->section_length_octet3;
    RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
            "<%s::parseNTT_revision_data> - section_length = (%d).\n",
            SIMODULE, section_length);

    //**** extract and validate the table_subtype field ****
    table_subtype = GETBITS(ptr_ntt_record_struc1->table_subtype_octet8, 4, 1);
    RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
            "<%s::parseNTT_revision_data> - Table_Subtype = (%d).\n", SIMODULE,
            table_subtype);

    offset += sizeof(ntt_record_struc1);

    /* subtract the header */
    section_length -= (sizeof(ntt_record_struc1) - LEN_OCTET1_THRU_OCTET3);

    /* fail if this doesn't have an SNS subtable */
    if (table_subtype != SNS_TABLE_SUBTYPE)
        return RMF_SI_PARSE_ERROR;

    ptr_input_buffer = input_buffer + offset;
    ptr_sns_record_struc1 = (sns_record_struc1 *) ptr_input_buffer;
    number_of_SNS_records = ptr_sns_record_struc1->number_of_SNS_records_octet1;

    /* Calculate new offset and section length */
    ptr_input_buffer += sizeof(sns_record_struc1);
    section_length -= sizeof(sns_record_struc1);

    RDK_LOG(
            RDK_LOG_TRACE4,
            "LOG.RDK.SI",
            "<%s::parseNTT_revision_data> - number of sns records = (%d), offset = %d.\n",
            SIMODULE, number_of_SNS_records, offset);

    while (number_of_SNS_records--)
    {
        ptr_sns_record_struc2 = (sns_record_struc2 *) ptr_input_buffer;
        name_length = ptr_sns_record_struc2->name_length_octet4;
        RDK_LOG(
                RDK_LOG_TRACE6,
                "LOG.RDK.SI",
                "<%s::parseNTT_revision_data> - name_length of sns record %d = (%d).\n",
                SIMODULE, number_of_SNS_records, name_length);
        ptr_input_buffer += (sizeof(sns_record_struc2) + name_length);
        section_length -= (sizeof(sns_record_struc2) + name_length);

        SNS_descriptor_count = *ptr_input_buffer;
        RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
                "<%s::parseNTT_revision_data> - SNS_descriptor_count %d.\n",
                SIMODULE, SNS_descriptor_count);
        ptr_input_buffer++;
        section_length--;

        while (SNS_descriptor_count--)
        {
            ptr_generic_descriptor_struc
                    = (generic_descriptor_struct *) ptr_input_buffer;
            descriptor_length
                    = ptr_generic_descriptor_struc->descriptor_length_octet2;
            RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
                    "<%s::parseNTT_revision_data> - descriptor length %d.\n",
                    SIMODULE, SNS_descriptor_count);

            ptr_input_buffer += (descriptor_length
                    + sizeof(generic_descriptor_struct));
            section_length -= (descriptor_length
                    + sizeof(generic_descriptor_struct));
        }
    }
    /*
     * This is the beginning of the outer descriptor.  The RDD should
     * be the first one.  Verify that the remaining section_length is
     * >= the RDD descriptor before we attempt to parse it.
     */
    RDK_LOG(
            RDK_LOG_TRACE6,
            "LOG.RDK.SI",
            "<%s::parseNTT_revision_data> - section_length: %d, sizeof(rdd):%d\n",
            SIMODULE, section_length,
            sizeof(revision_detection_descriptor_struc));

    /*
     * Now skip the other descriptors and get the CRC
     */
    RDK_LOG(
            RDK_LOG_TRACE4,
            "LOG.RDK.SI",
            "<%s::parseNTT_revision_data> - section_length = %d, crc length = %d\n",
            SIMODULE, section_length, LENGTH_CRC_32);

    if (section_length > LENGTH_CRC_32)
    {
        ptr_rdd_table_struc1
                = (revision_detection_descriptor_struc *) ptr_input_buffer;

        if (ptr_rdd_table_struc1->descriptor_tag_octet1 != 0x93)
        {
            RDK_LOG(
                    RDK_LOG_DEBUG,
                    "LOG.RDK.SI",
                    "<%s::parseNTT_revision_data> - descriptor not a revision descriptor - tag: 0x%x - skip all descriptors\n",
                    SIMODULE, ptr_rdd_table_struc1->descriptor_tag_octet1);

            /*
             * Set the RDD values to the NO_VERSION macro
             */
            *version = NO_VERSION;
            *section_number = NO_VERSION;
            *number_sections = NO_VERSION;
            while (section_length > LENGTH_CRC_32)
            {
                ptr_generic_descriptor_struc
                        = (generic_descriptor_struct *) ptr_input_buffer;
                ptr_input_buffer
                        += sizeof(generic_descriptor_struct)
                                + ptr_generic_descriptor_struc->descriptor_length_octet2;
                section_length
                        -= (sizeof(generic_descriptor_struct)
                                + ptr_generic_descriptor_struc->descriptor_length_octet2);

                if (section_length <= 0)
                {
                    RDK_LOG(
                            RDK_LOG_ERROR,
                            "LOG.RDK.SI",
                            "<%s::parseNTT_revision_data> - error, section_length not large enough to accommodate descriptor(s) and crc.\n",
                            SIMODULE);
                    break;
                }
            }
        }
        else
        {
            *version = (ptr_rdd_table_struc1->table_version_number_octet3
                    & 0x1F);
            *section_number = ptr_rdd_table_struc1->section_number_octet4;
            *number_sections = ptr_rdd_table_struc1->last_section_number_octet5;
            RDK_LOG(
                    RDK_LOG_TRACE4,
                    "LOG.RDK.SI",
                    "<%s::parseNTT_revision_data> - version: %d, section_number: %d, last_section_number: %d\n",
                    SIMODULE, *version, *section_number, *number_sections);
            ptr_input_buffer += sizeof(revision_detection_descriptor_struc);
            section_length -= sizeof(revision_detection_descriptor_struc);
        }

        RDK_LOG(
                RDK_LOG_TRACE4,
                "LOG.RDK.SI",
                "<%s::parseNTT_revision_data> - before CRC section_length = %d\n",
                SIMODULE, section_length);

        if (section_length == LENGTH_CRC_32)
        {

            *crc = ((*ptr_input_buffer) << 24 | (*(ptr_input_buffer + 1)) << 16
                    | (*(ptr_input_buffer + 2)) << 8
                    | (*(ptr_input_buffer + 3)));
            RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                    "<%s::parseNTT_revision_data> - CRC32 = 0x%08x\n",
                    SIMODULE, *crc);

            if ((*crc == 0xFFFFFFFF) || (*crc == 0x00000000))
            {
                RDK_LOG(
                        RDK_LOG_TRACE4,
                        "LOG.RDK.SI",
                        "<%s::parseNTT_revision_data> - Invalid CRC value (0x%08x) - calculating CRC\n",
                        SIMODULE, *crc);
                // Note: We don't really need to use a particular CRC here - just a decent one - since we're (presumably) going to be checking
                //       against our calculated CRCs in this case (unless a crazy plant *and* POD send us some with and some without CRCs...)
                *crc = m_pSiCache->calc_mpeg2_crc(input_buffer, offset + 4); // Need to account for the 4 bytes of CRC in the length
            }
        }
    }
    else if (section_length == LENGTH_CRC_32)
    {
        /*
         * Set the RDD values to the NO_VERSION macro
         */
        *version = NO_VERSION;
        *section_number = NO_VERSION;
        *number_sections = NO_VERSION;

        *crc = ((*(input_buffer + offset)) << 24 | (*(input_buffer + offset
                + 1)) << 16 | (*(input_buffer + offset + 2)) << 8
                | (*(input_buffer + offset + 3)));
        RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                "<%s::parseNTT_revision_data> - CRC32 = 0x%08x\n",
                SIMODULE, *crc);

        if ((*crc == 0xFFFFFFFF) || (*crc == 0x00000000))
        {
            RDK_LOG(
                    RDK_LOG_TRACE4,
                    "LOG.RDK.SI",
                    "<%s::parseNTT_revision_data> - Invalid CRC value (0x%08x) - calculating CRC\n",
                    SIMODULE, *crc);
            // Note: We don't really need to use a particular CRC here - just a decent one - since we're (presumably) going to be checking
            //       against our calculated CRCs in this case (unless a crazy plant *and* POD send us some with and some without CRCs...)
            *crc = m_pSiCache->calc_mpeg2_crc(input_buffer, offset + 4); // Need to account for the 4 bytes of CRC in the length
        }
    }
    else
    {
        RDK_LOG(
                RDK_LOG_ERROR,
                "LOG.RDK.SI",
                "<%s::parseNTT_revision_data> - No enough bytes left for a CRC32\n",
                SIMODULE);
    }
    return retCode;
}

/*
 ** *****************************************************************************
 ** Subroutine/Method:                       parseSVCT_revision_data()
 ** *****************************************************************************
 **
 ** Parse the RDD descriptor in the SVCT table section.
 **
 ** @param  section_handle    -  the section handle for the SVCT table section.
 ** @param  * version         -  ptr to the version uint8_t to be filled in.
 ** @param  * section_number  -  ptr to the index of this section in the total
 **                                number of sections.
 ** @param  * number_sections -  ptr to the the total number of sections to be
 **                              filled out in this function.
 **
 ** @return RMF_SUCCESS if RDD section was successfully processed/parsed .
 ** Otherwise an error code greater than zero (RMF_SI_PARSE_ERROR).
 ** *****************************************************************************
 */
rmf_Error rmf_OobSiParser::parseSVCT_revision_data(rmf_FilterSectionHandle section_handle,
        uint8_t * version, uint8_t * section_number, uint8_t * number_sections,
        uint32_t * crc)
{
    rmf_Error retCode = RMF_SUCCESS;

    svct_table_struc1 * ptr_svct_table_struc1 = NULL;
    vcm_record_struc2 * ptr_vcm_structure = NULL;
    dcm_record_struc2 * ptr_dcm_record_struc = NULL;
    icm_record_struc2 * ptr_icm_record_struc = NULL;
    revision_detection_descriptor_struc * ptr_rdd_table_struc1 = NULL;
    generic_descriptor_struct * ptr_gen_desc_struc = NULL;

    uint8_t descriptor_count = 0;
    uint8_t number_of_VC_records = 0;
    uint8_t DCM_data_length = 0;
    uint8_t icm_record_count = 0;
    uint8_t table_subtype = 0;
    uint8_t descriptors_included = 0;
    uint8_t input_buffer[MAX_OOB_SMALL_TABLE_SECTION_LENGTH];
    uint16_t section_length = 0;
    uint32_t number_bytes_copied = 0;
    uint32_t offset = 0;
    uint32_t ii = 0;

    RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
            "<%s::parseSVCT_revision_data> - Enter\n", SIMODULE);
    retCode = m_pSectionFilter->ReadSection(section_handle, 0,
            MAX_OOB_SMALL_TABLE_SECTION_LENGTH, 0, input_buffer,
            &number_bytes_copied);
    if (retCode != RMF_SUCCESS)
        return RMF_SI_PARSE_ERROR;

    ptr_svct_table_struc1 = (svct_table_struc1 *) input_buffer;

    section_length = ((GETBITS(ptr_svct_table_struc1->section_length_octet2, 4,
            1)) << 8) | ptr_svct_table_struc1->section_length_octet3;
    RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
            "<%s::parseSVCT_revision_data> - section_length = (%d).\n",
            SIMODULE, section_length);

    //**** extract and validate the table_subtype field ****
    table_subtype = GETBITS(ptr_svct_table_struc1->table_subtype_octet5, 4, 1);
    RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
            "<%s::parseSVCT_revision_data> - Table_Subtype = (%d).\n",
            SIMODULE, table_subtype);

    offset += sizeof(svct_table_struc1);

    /* subtract the header */
    section_length -= (sizeof(svct_table_struc1) - LEN_OCTET1_THRU_OCTET3);

    switch (table_subtype)
    {
    case VIRTUAL_CHANNEL_MAP:
    {
        ptr_vcm_structure = (vcm_record_struc2 *) (input_buffer + offset);

        descriptors_included = ((ptr_vcm_structure->descriptors_included_octet1
                & 0x04) >> 2);
        number_of_VC_records = ptr_vcm_structure->number_vc_records_octet7;

        /*
         * increment section/offset past vcm_record_struc2
         */
        offset += sizeof(vcm_record_struc2);
        section_length -= sizeof(vcm_record_struc2);

        for (ii = 0; ii < number_of_VC_records; ii++)
        {
            /* skip the record */
            offset += (sizeof(vcm_virtual_chan_record3)
                    + sizeof(mpeg2_virtual_chan_struc4));
            section_length -= (sizeof(vcm_virtual_chan_record3)
                    + sizeof(mpeg2_virtual_chan_struc4));

            if (descriptors_included)
            {
                descriptor_count = *(input_buffer + offset);
                offset++;
                section_length--;

                ptr_gen_desc_struc = (generic_descriptor_struct *) (input_buffer
                        + offset);

                /* skip the inner descriptors */
                while (descriptor_count--)
                {
                    offset += (ptr_gen_desc_struc->descriptor_length_octet2
                            + sizeof(generic_descriptor_struct));
                    section_length
                            -= (ptr_gen_desc_struc->descriptor_length_octet2
                                    + sizeof(generic_descriptor_struct));
                }
            }
        }//end for ii < numb_of_VC_records
        break;
    }
    case DEFINED_CHANNEL_MAP:
        ptr_dcm_record_struc = (dcm_record_struc2 *) (input_buffer + offset);
        DCM_data_length = ptr_dcm_record_struc->dcm_data_length_octet_3 & 0x7F;
        offset += ((sizeof(dcm_record_struc3) * DCM_data_length)
                + sizeof(dcm_record_struc2));
        section_length -= ((sizeof(dcm_record_struc3) * DCM_data_length)
                + sizeof(dcm_record_struc2));
        break;
    case INVERSE_CHANNEL_MAP:
        ptr_icm_record_struc = (icm_record_struc2 *) (input_buffer + offset);
        icm_record_count = ptr_icm_record_struc->record_count_octet3 & 0x7F;
        offset += ((sizeof(icm_record_struc3) * icm_record_count)
                + sizeof(icm_record_struc2));
        section_length -= ((sizeof(icm_record_struc3) * icm_record_count)
                + sizeof(icm_record_struc2));
        break;
    default:
        RDK_LOG(
                RDK_LOG_ERROR,
                "LOG.RDK.SI",
                "<%s::parseSVCT_revision_data> - Error Table_Subtype not handled: (%d).\n",
                SIMODULE, table_subtype);
        return RMF_SI_PARSE_ERROR;

    }

    /*
     * This is the beginning of the outer descriptor.  The RDD should
     * be the first one.  Verify that the remaining section_length is
     * >= the RDD descriptor before we attempt to parse it.
     */
    RDK_LOG(
            RDK_LOG_TRACE4,
            "LOG.RDK.SI",
            "<%s::parseSVCT_revision_data> - section_length: %d, sizeof(rdd):%d\n",
            SIMODULE, section_length,
            sizeof(revision_detection_descriptor_struc));

    /*
     * Parse and return the RDD values.
     */
    RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
            "<%s::parseSVCT_revision_data> - number bytes read: %d\n",
            SIMODULE, number_bytes_copied);

    RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
            "<%s::parseSVCT_revision_data> - RDD bytes: ", SIMODULE);
    for (ii = 0; ii < sizeof(revision_detection_descriptor_struc); ii++)
        RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI", "0x%x ", input_buffer[offset + ii]);

    RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI", "\n");

    /*
     * Now skip the other descriptors and get the CRC
     */
    RDK_LOG(
            RDK_LOG_TRACE4,
            "LOG.RDK.SI",
            "<%s::parseSVCT_revision_data> - section_length = %d, crc length = %d\n",
            SIMODULE, section_length, LENGTH_CRC_32);
    if (section_length > LENGTH_CRC_32)
    {
        ptr_rdd_table_struc1
                = (revision_detection_descriptor_struc *) (input_buffer
                        + offset);

        RDK_LOG(
                RDK_LOG_DEBUG,
                "LOG.RDK.SI",
                "<%s::parseSVCT_revision_data> - Found descriptor tag: 0x%x (length %d)\n",
                SIMODULE, ptr_rdd_table_struc1->descriptor_tag_octet1,
                ptr_rdd_table_struc1->descriptor_length_octet2);

        if (ptr_rdd_table_struc1->descriptor_tag_octet1 != 0x93)
        {
            RDK_LOG(
                    RDK_LOG_DEBUG,
                    "LOG.RDK.SI",
                    "<%s::parseSVCT_revision_data> - descriptor not a revision descriptor - tag: 0x%x - skip all descriptors\n",
                    SIMODULE, ptr_rdd_table_struc1->descriptor_tag_octet1);

            /*
             * Set the RDD values to the NO_VERSION macro
             */
            *version = NO_VERSION;
            *section_number = NO_VERSION;
            *number_sections = NO_VERSION;
            while (section_length > LENGTH_CRC_32)
            {
                ptr_gen_desc_struc = (generic_descriptor_struct *) (input_buffer
                        + offset);
                offset += sizeof(generic_descriptor_struct)
                        + ptr_gen_desc_struc->descriptor_length_octet2;
                section_length -= (sizeof(generic_descriptor_struct)
                        + ptr_gen_desc_struc->descriptor_length_octet2);

                RDK_LOG(
                        RDK_LOG_INFO,
                        "LOG.RDK.SI",
                        "<%s::parseSVCT_revision_data> - Found descriptor tag: 0x%x (length %d)\n",
                        SIMODULE, ptr_gen_desc_struc->descriptor_tag_octet1,
                        ptr_gen_desc_struc->descriptor_length_octet2);

                if (section_length <= 0)
                {
                    RDK_LOG(
                            RDK_LOG_ERROR,
                            "LOG.RDK.SI",
                            "<%s::parseSVCT_revision_data> - error, section_length not large enough to accommodate descriptor(s) and crc.\n",
                            SIMODULE);
                    break;
                }
            }
        }
        else
        {
            *version = (ptr_rdd_table_struc1->table_version_number_octet3
                    & 0x1F);
            *section_number = ptr_rdd_table_struc1->section_number_octet4;
            *number_sections = ptr_rdd_table_struc1->last_section_number_octet5;
            RDK_LOG(
                    RDK_LOG_TRACE4,
                    "LOG.RDK.SI",
                    "<%s::parseSVCT_revision_data> - version: %d, section_number: %d, last_section_number: %d\n",
                    SIMODULE, *version, *section_number, *number_sections);
            offset += sizeof(revision_detection_descriptor_struc);
            section_length -= sizeof(revision_detection_descriptor_struc);
        }

        RDK_LOG(
                RDK_LOG_TRACE4,
                "LOG.RDK.SI",
                "<%s::parseSVCT_revision_data> - before CRC section_length = %d\n",
                SIMODULE, section_length);

        if (section_length == LENGTH_CRC_32)
        {
            *crc = ((*(input_buffer + offset)) << 24 | (*(input_buffer + offset
                    + 1)) << 16 | (*(input_buffer + offset + 2)) << 8
                    | (*(input_buffer + offset + 3)));
            RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                    "<%s::parseSVCT_revision_data> - CRC32 = 0x%08x\n",
                    SIMODULE, *crc);

            if ((*crc == 0xFFFFFFFF) || (*crc == 0x00000000))
            {
                RDK_LOG(
                        RDK_LOG_TRACE4,
                        "LOG.RDK.SI",
                        "<%s::parseSVCT_revision_data> - Invalid CRC value (0x%08x) - calculating CRC\n",
                        SIMODULE, *crc);
                // Note: We don't really need to use a particular CRC here - just a decent one - since we're (presumably) going to be checking
                //       against our calculated CRCs in this case (unless a crazy plant *and* POD send us some with and some without CRCs...)
                *crc = m_pSiCache->calc_mpeg2_crc(input_buffer, offset + 4); // Need to account for the 4 bytes of CRC in the length
            }
        }
    }
    else if (section_length == LENGTH_CRC_32)
    {
        /*
         * Set the RDD values to the NO_VERSION macro
         */
        *version = NO_VERSION;
        *section_number = NO_VERSION;
        *number_sections = NO_VERSION;

        *crc = ((*(input_buffer + offset)) << 24 | (*(input_buffer + offset
                + 1)) << 16 | (*(input_buffer + offset + 2)) << 8
                | (*(input_buffer + offset + 3)));
        RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
                "<%s::parseSVCT_revision_data> - CRC32 = 0x%08x\n",
                SIMODULE, *crc);

        if ((*crc == 0xFFFFFFFF) || (*crc == 0x00000000))
        {
            RDK_LOG(
                    RDK_LOG_TRACE4,
                    "LOG.RDK.SI",
                    "<%s::parseSVCT_revision_data> - Invalid CRC value (0x%08x) - calculating CRC\n",
                    SIMODULE, *crc);
            // Note: We don't really need to use a particular CRC here - just a decent one - since we're (presumably) going to be checking
            //       against our calculated CRCs in this case (unless a crazy plant *and* POD send us some with and some without CRCs...)
            *crc = m_pSiCache->calc_mpeg2_crc(input_buffer, offset + 4); // Need to account for the 4 bytes of CRC in the length
        }
    }
    else
    {
        RDK_LOG(
                RDK_LOG_ERROR,
                "LOG.RDK.SI",
                "<%s::parseSVCT_revision_data> - No enough bytes left for a CRC32\n",
                SIMODULE);
    }
    return retCode;
}

/*
 ** *****************************************************************************
 ** Subroutine/Method:                       parseSTT_revision_data()
 ** *****************************************************************************
 **
 ** Parse the RDD descriptor in the STT table section.
 **
 ** @param  section_handle    -  the section handle for the STT table section.
 ** @param  * version         -  ptr to the version uint8_t to be filled in.
 ** @param  * section_number  -  ptr to the index of this section in the total
 **                                number of sections.
 ** @param  * number_sections -  ptr to the the total number of sections to be
 **                              filled out in this function.
 **
 ** @return RMF_SUCCESS if RDD section was successfully processed/parsed .
 ** Otherwise an error code greater than zero (RMF_SI_PARSE_ERROR).
 ** *****************************************************************************
 */
rmf_Error rmf_OobSiParser::parseSTT_revision_data(rmf_FilterSectionHandle section_handle,
        uint8_t * version, uint8_t * section_number, uint8_t * number_sections,
        uint32_t * crc)
{
    rmf_Error retCode = RMF_SUCCESS;
    stt_table_struc1 * ptr_stt_table_struc1 = NULL;
    uint8_t input_buffer[MAX_OOB_SMALL_TABLE_SECTION_LENGTH];
    uint16_t section_length = 0;
    uint32_t number_bytes_copied = 0;

    RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
            "<%s::parseSTT_revision_data> - Enter\n", SIMODULE);
    retCode = m_pSectionFilter->ReadSection(section_handle, 0,
            MAX_OOB_SMALL_TABLE_SECTION_LENGTH, 0, input_buffer,
            &number_bytes_copied);
    if (retCode != RMF_SUCCESS)
        return RMF_SI_PARSE_ERROR;

    ptr_stt_table_struc1 = (stt_table_struc1 *) input_buffer;

    section_length = ((GETBITS(ptr_stt_table_struc1->section_length_octet2, 4,
            1)) << 8) | ptr_stt_table_struc1->section_length_octet3;
    RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
            "<%s::parseSTT_revision_data> - section_length = (%d).\n",
            SIMODULE, section_length);

    *version = NO_VERSION;
    *section_number = NO_VERSION;
    *number_sections = NO_VERSION;

    RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.SI",
            "<%s::parseSTT_revision_data> - Exit.\n", SIMODULE);
    return retCode;
}

//***************************************************************************************************
//  Subroutine/Method:                       parseSNSSubtable()
//***************************************************************************************************
/**
 * Parses SNS (Source Name Subtable) records, validates the fileds within a SNS record.
 *
 * @param  pptr_TableSection,  the address of memeory location which contains a pointer to the NIT
 *                             table section.
 * @param  ptr_section_length, a pointer to the caller's section_length variable.
 *
 * @param  language_code, the 3-character ISO 639 language code associated with this SNS
 *
 * @return RMF_SUCCESS if table section was successfully processed/parsed .Otherwise an error code
 *         greater than zero (RMF_SUCCESS).
 */
rmf_Error rmf_OobSiParser::parseSNSSubtable(uint8_t **pptr_TableSection,
        int32_t *ptr_section_length, char *language_code)
{
    int i, descriptor_count;

    uint8_t number_of_SNS_records = 0;
    uint8_t segment[256] =
    { 0 };
    uint8_t mode = 0;
    uint8_t segment_length = 0;
    uint8_t name_length = 0;
    uint16_t source_ID = 0;
    rmf_osal_Bool bIsAppType;
    sns_record_struc1 *ptr_sns_record_struc1 = NULL;
    sns_record_struc2 *ptr_sns_record_struc2 = NULL;
    generic_descriptor_struct *ptr_descriptor_struc = NULL;
    rmf_Error ntt_error = NO_PARSING_ERROR;

    //***********************************************************************************************
    //***********************************************************************************************
    RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
            "<%s> - Entered method parseSNSSubtable.\n",
            __FUNCTION__);

    /*
     ** first get the number of SNS records
     */
    ptr_sns_record_struc1 = (sns_record_struc1 *) *pptr_TableSection;

    number_of_SNS_records = ptr_sns_record_struc1->number_of_SNS_records_octet1;
    RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
            "<%s> - number of sns records: %d\n", __FUNCTION__,
            number_of_SNS_records);

    /* increment the ptr in the table sectio to the next struct*/
    *pptr_TableSection += sizeof(sns_record_struc1);
    *ptr_section_length -= sizeof(sns_record_struc1);

    RDK_LOG(
            RDK_LOG_TRACE6,
            "LOG.RDK.SI",
            "<%s> - section_length: %d, sizeof(sns_record_struc1): %d \n",
            __FUNCTION__, *ptr_section_length, sizeof(sns_record_struc1));

    for (i = 0; (i < number_of_SNS_records) && (ntt_error == NO_PARSING_ERROR); ++i)
    {
        //*******************************************************************************************
        // Extract and validate data from the specified sns records per table 5.12, pg. 22 in document
        // ANSI/SCTE 65-2002 (formerly DVS 234)
        //*******************************************************************************************

        // overlay the second sns structure
        ptr_sns_record_struc2 = (sns_record_struc2 *) *pptr_TableSection;
        bIsAppType = false;

        if (TESTBITS(ptr_sns_record_struc2->application_type_octet1, 8, 8))
        {
            bIsAppType = true;
            RDK_LOG(RDK_LOG_TRACE3, "LOG.RDK.SI",
                    "<%s> - name is for app id \n", __FUNCTION__);
        }
        source_ID = (uint16_t)(((ptr_sns_record_struc2->source_ID_octet2) << 8)
                | ptr_sns_record_struc2->source_ID_octet3);
        RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
                "<%s> - sourceId: 0x%x(%d).\n", __FUNCTION__,
                source_ID, source_ID);

        name_length = ptr_sns_record_struc2->name_length_octet4;

        /* Move the table section ptr to the beginning of the name */
        *pptr_TableSection += sizeof(sns_record_struc2);
        *ptr_section_length -= (sizeof(sns_record_struc2)
                + ptr_sns_record_struc2->name_length_octet4);
        RDK_LOG(
                RDK_LOG_TRACE6,
                "LOG.RDK.SI",
                "<%s> - name length: %d, sectionLength: %d.\n",
                __FUNCTION__, name_length, *ptr_section_length);

        /* parse the multilingual text string NOTE: this method will advance the section ptr*/
        parseMTS(pptr_TableSection, name_length, &mode, &segment_length,
                segment);
        RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
                "<%s> - MTS mode: 0x%x segment_length: %d, ",
                __FUNCTION__, mode, segment_length);
        m_pSiCache->LockForWrite();
        if (segment_length != 0)
        {
            segment[segment_length] = '\0';

            RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
                    "<%s> - sourceId: 0x%x(%d), name: %s \n",
                    __FUNCTION__, source_ID, source_ID, (char*) segment);
            {
                rmf_siSourceNameEntry *entry = NULL;

                // see if this entry exists (ie don't create one if it doesn't
                m_pSiCache->GetSourceNameEntry(source_ID, bIsAppType, &entry, FALSE);  // try to find w/o a create
                //
                // If the entry exists, it can only be for a new language, otherwise, it's a duplicate within
                // the table, which is an error.
                if (entry != NULL)
                {
                    // See if this is a different language. If not, it could be an error
                    if (!m_pSiCache->IsSourceNameLanguagePresent(entry, language_code))
                    {
                        m_pSiCache->SetSourceName(entry, (char*)segment, language_code, TRUE);
                        m_pSiCache->SetSourceLongName(entry, (char*)segment, language_code);
                    }
                    else
                    {
                        // Not an error, just skip
                        //ntt_error = 18;
                    }
                }
                // Entry was not found, go create one
                else
                {
                    m_pSiCache->GetSourceNameEntry(source_ID, bIsAppType, &entry, TRUE);   // create the entry

                    if (entry != NULL)
                    {
                        m_pSiCache->SetSourceName(entry, (char*)segment, language_code, FALSE);
                        m_pSiCache->SetSourceLongName(entry, (char*)segment, language_code);
                    }
                    else
                    {
                        ntt_error = 18;   // error detected, could not create entry
                    }
                }
            }
        }
        else
        {
            RDK_LOG(RDK_LOG_TRACE3, "LOG.RDK.SI", "%s\n", segment);
        }

        m_pSiCache->ReleaseWriteLock();

        //****************************************************************************************************
        // Prepare to process the descriptors if they are present.
        //****************************************************************************************************

        descriptor_count = **pptr_TableSection; // get 8-bit descriptor count and advance pointer
        RDK_LOG(RDK_LOG_TRACE6, "LOG.RDK.SI",
                "<%s> - descriptor_count: %d\n", __FUNCTION__,
                descriptor_count);
        ++(*pptr_TableSection);
        *ptr_section_length -= 1;

        while (descriptor_count > 0)
        { // ignore all descriptors .

            ptr_descriptor_struc
                    = (generic_descriptor_struct *) *pptr_TableSection;
            *pptr_TableSection += sizeof(generic_descriptor_struct)
                    + ptr_descriptor_struc->descriptor_length_octet2;

            RDK_LOG(
                    RDK_LOG_TRACE6,
                    "LOG.RDK.SI",
                    "<%s> - ptr: 0x%p, descriptor_tag: 0x%x, descriptor_length: %d\n",
                    __FUNCTION__, ptr_descriptor_struc,
                    ptr_descriptor_struc->descriptor_tag_octet1,
                    ptr_descriptor_struc->descriptor_length_octet2);

            //*** decrement section_length as we did not account for descriptor tag, length
            //*** and data fields length and data fields prior to calling this subroutine .

            --descriptor_count;
            *ptr_section_length -= (sizeof(generic_descriptor_struct)
                    + ptr_descriptor_struc->descriptor_length_octet2);

            if (*ptr_section_length < 0)
            { //error, section_length not large enough to account for descriptor tags, length and data fields
                ntt_error = 18;
                break;
            }
        } //end_of_while_loop
    } //end_of_for_loop


    //****************************************************************************************************
    //    Update the SI database with the frequencies.
    //****************************************************************************************************

    if (ntt_error == NO_PARSING_ERROR)
    {
        //call sidb here
    }
    RDK_LOG(RDK_LOG_TRACE4, "LOG.RDK.SI",
            "<%s> - Exiting with a NTT_Error_Code = %d.\n",
            __FUNCTION__, ntt_error);

    return ntt_error;
}

void rmf_OobSiParser::parseMTS(uint8_t **ptr_TableSection, uint8_t name_lenth, uint8_t * mode,
        uint8_t * length, uint8_t * segment)
{
    mts_format_struc1 * ptr_mts_format_struc;
    int ii = 0;

    RMF_OSAL_UNUSED_PARAM(name_lenth); /* if this param is not used, should it be removed from this function? */

    ptr_mts_format_struc = (mts_format_struc1 *) *ptr_TableSection;

    /* parse the mode and set the value */
    *mode = ptr_mts_format_struc->mode_octet1;

    /*
     ** If 'mode' is in the range 0x40 to 0x9F, then the 'length/segment portion
     ** is omitted. Formate effector codes in the range 0x40 to 0x9F involve no
     ** associated parametric data; hence the ommision of the 'length/segment portion.
     ** Format effector codes in the range 0xA0 through 0xFF include one or more
     ** parameter specific to the particular format effector function.
     **
     ** So set the length to 0 and return if the value is above the 0x40 range.
     */
    if (*mode >= 0x40)
    {
        *length = 0;
        return;
    }

    /* parse the length and set the value */
    *length = ptr_mts_format_struc->length_octet2;

    RDK_LOG(RDK_LOG_TRACE3, "LOG.RDK.SI",
            "<%s> - mode: 0x%x length: 0x%x\n", __FUNCTION__, *mode,
            *length);
    /* recalc the ptr position past the mode and length fields */
    *ptr_TableSection += sizeof(mts_format_struc1);

    RDK_LOG(RDK_LOG_TRACE3, "LOG.RDK.SI",
            "<%s> - segment_length: %d\n", __FUNCTION__, *length);
    /* Copy the name to the segment param */
    for (ii = 0; ii < *length; ii++)
    {
        segment[ii] = **ptr_TableSection;
        *ptr_TableSection = *ptr_TableSection + 1;
    }
}

/**
 * @brief This function identifies and parses the SI table passed and updates the crc, version, section_number and
 * last_section_number. As part of parsing the table, it also updates the SI database. The table passed can be
 * NIT, NTT, SVCT and STT.
 *
 * @param[in] ptr_table Pointer to the SI table using which the type of table and its sub table are identified.
 * @param[in] section_handle Indicates handle to get the section data.
 * @param[out] version Indicates version of the current table.
 * @param[out] section_number Identifies the current table section number.
 * @param[out] last_section_number Identifies the number of sections in a table.
 * @param[out] crc Indicates the Cyclic Redundancy Check value specified in the table.
 *
 * @return Returns RMF_SI_SUCCESS if the table is parsed and updated successfully else returns appropriate
 * rmf_Error codes
 */
rmf_Error rmf_OobSiParser::parseAndUpdateTable(rmf_si_table_t * ptr_table,
        rmf_FilterSectionHandle section_handle, uint8_t * version,
        uint8_t * section_number, uint8_t * last_section_number,
        uint32_t * crc)
{
        rmf_Error retCode = RMF_SI_SUCCESS;

        switch (ptr_table->table_id)
        {
        case NETWORK_INFORMATION_TABLE_ID:
            {
                retCode = parseAndUpdateNIT(section_handle, version, section_number,
                        last_section_number, crc);
                break;
            }
        case NETWORK_TEXT_TABLE_ID:
            {
                retCode = parseAndUpdateNTT(section_handle, version, section_number,
                        last_section_number, crc);
                break;
            }
        case SF_VIRTUAL_CHANNEL_TABLE_ID:
            {
                retCode = parseAndUpdateSVCT(section_handle, version, section_number,
                        last_section_number, crc);
                break;
            }
        case SYSTEM_TIME_TABLE_ID:
            {
                retCode = parseAndUpdateSTT(section_handle, version, section_number,
                        last_section_number, crc);
                break;
            }
        default:
            {
                RDK_LOG(
                        RDK_LOG_ERROR,
                        "LOG.RDK.SI",
                        "<%s> - Error parsing - tableid: 0x%x(%d), subtype: %d, section_handle: %d\n",
                        __FUNCTION__, ptr_table->table_id, ptr_table->table_id,
                        ptr_table->subtype, section_handle);
                retCode = RMF_SI_PARSE_ERROR;

            }
            break;
    }

        if (RMF_SI_SUCCESS != retCode)
        {
                RDK_LOG(
                RDK_LOG_TRACE6,
                                "LOG.RDK.SI",
                                "<%s> - Error parsing - tableid: 0x%x(%d), subtype: %d, section_handle: %d\n",
                                __FUNCTION__, ptr_table->table_id, ptr_table->table_id,
                                ptr_table->subtype, section_handle);
        }
        RDK_LOG(
                        RDK_LOG_TRACE4,
                        "LOG.RDK.SI",
                        "<%s> - Successfully parsed table - tableid: %s, subtype: %d\n",
                        __FUNCTION__, tableToString(ptr_table->table_id),
                        ptr_table->subtype);
        return retCode;
}

/**
 * @brief This function gets a new instance of rmf_OobSiParser and initialises it with the specified
 * section filter. It also creates and initialises OOB SI cache.
 *
 * @param[in] pSectionFilter Indicates pointer to the section filter.
 *
 * @return Returns a pointer to the newly created instance of rmf_OobSiParser.
 */
rmf_OobSiParser* rmf_OobSiParser::getParserInstance(rmf_SectionFilter *pSectionFilter)
{
    if(m_gpParserInst == NULL)
        m_gpParserInst = new rmf_OobSiParser(pSectionFilter);

    return m_gpParserInst;
}

/* debug functions */
/** make these global to prevent compiler warning errors **/
char * rmf_OobSiParser::tableToString(uint8_t table)
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
